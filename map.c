/*  :ts=8 bk=0
 *
 * map.c:	Automap systems.
 *
 * Leo L. Schwab					9310.18
 */
#include <types.h>
#include <graphics.h>
#include <event.h>
#include <operamath.h>

#include "castle.h"
#include "objects.h"
#include "imgfile.h"
#include "ob_powerup.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define	CORNER_NW	0
#define	CORNER_SW	1
#define	CORNER_SE	2
#define	CORNER_NE	3


#define	CORNF_NW	1
#define	CORNF_NE	(1<<1)
#define	CORNF_WN	(1<<2)
#define	CORNF_WS	(1<<3)
#define	CORNF_SW	(1<<4)
#define	CORNF_SE	(1<<5)
#define	CORNF_EN	(1<<6)
#define	CORNF_ES	(1<<7)

#define	CORNF_NW_WN	(CORNF_NW | CORNF_WN)
#define	CORNF_SW_WS	(CORNF_SW | CORNF_WS)
#define	CORNF_SE_ES	(CORNF_SE | CORNF_ES)
#define	CORNF_NE_EN	(CORNF_NE | CORNF_EN)


#define	XMARGIN		((320 - 256) / 2)
#define	YMARGIN		((240 - 192) / 2)

#define	XORG		(320 - (320 - 256) / 2 - 192)
#define	YORG		((240 + 192) >> 1)

#define	CX_NEEDLE	(XMARGIN + 32)
#define	CY_NEEDLE	(YMARGIN + 34)

#define	COLR_BG		MakeRGB15Pair (5, 3, 7)
#define	COLR_SURF	MakeRGB15 (22, 16, 26)
#define	COLR_MID	MakeRGB15 (16, 12, 18)
#define	COLR_OUTER	MakeRGB15 (8, 2, 16)

enum	GlyphTypes {
	GLYPH_PLAYER = 0,
	GLYPH_NSDOOR,
	GLYPH_EWDOOR,
	GLYPH_TALISMAN,
	GLYPH_EXIT,
	MAX_GLYPH
};


/***************************************************************************
 * Forward function declarations.  (Grrrr...)
 */
static void	drawcorner (struct RastPort *, int, int32, int32);
static void	checkglyph (struct RastPort *, struct MapEntry *, int32, int32);
static void	drawglyph (struct RastPort *, int, int32, int32);
static void	drawpoints (struct RastPort *, int32, int32, ubyte *, int32, int32);


/***************************************************************************
 * Globals.
 */
extern MapEntry		levelmap[WORLDSIZ][WORLDSIZ];
extern JoyData		jd;
extern int32		joytrigger;
extern Vertex		playerpos;
extern frac16		playerdir;

extern RastPort		*rprend, *rpvis;
extern Item		vblIO;

extern uint32		ccbextra;


static Vertex		needlequad[4], xfneedle[4];

static CelArray		*ca_glyphs, *ca_compass, *ca_needle;


/***************************************************************************
 * Code.
 */
int
drawmap ()
{
	register CCB		*ccb;
	register MapEntry	*me;
	register int		i, x, z;
	Matrix			unit, cam;
	int32			px, pz;
	int			mf;
	RastPort		*tmp;
	Point			quad[4];

	SetRast (rprend, COLR_BG);

	for (z = WORLDSIZ;  --z >= 0; ) {
		for (x = WORLDSIZ;  --x >= 0; ) {
			me = &levelmap[z][x];
			px = XORG + x + x + x;
			pz = YORG - z - z - z;

			if (me->me_Obs)
				checkglyph (rprend, me, px, pz);

			if (!(me->me_Flags & MEF_WALKSOLID))
				continue;

			if (i = me->me_VisFlags >> 4) {
				ccb = ca_glyphs->celptrs[i];

				ccb->ccb_XPos = px << 16;
				ccb->ccb_YPos = pz << 16;
				DrawCels (rprend->rp_BitmapItem, ccb);
			}

			i = 0;
			if (z < WORLDSIZ - 1) {
				mf = levelmap[z+1][x].me_VisFlags;
				if (mf & MAPF_WEST)	i |= CORNF_NW;
				if (mf & MAPF_EAST)	i |= CORNF_NE;
			}

			if (z) {
				mf = levelmap[z-1][x].me_VisFlags;
				if (mf & MAPF_WEST)	i |= CORNF_SW;
				if (mf & MAPF_EAST)	i |= CORNF_SE;
			}

			if (x < WORLDSIZ - 1) {
				mf = levelmap[z][x+1].me_VisFlags;
				if (mf & MAPF_NORTH)	i |= CORNF_EN;
				if (mf & MAPF_SOUTH)	i |= CORNF_ES;
			}

			if (x) {
				mf = levelmap[z][x-1].me_VisFlags;
				if (mf & MAPF_NORTH)	i |= CORNF_WN;
				if (mf & MAPF_SOUTH)	i |= CORNF_WS;
			}


			if ((i & CORNF_NW_WN) == CORNF_NW_WN)
				drawcorner (rprend, CORNER_NW, px, pz);

			if ((i & CORNF_SW_WS) == CORNF_SW_WS)
				drawcorner (rprend, CORNER_SW, px, pz);

			if ((i & CORNF_SE_ES) == CORNF_SE_ES)
				drawcorner (rprend, CORNER_SE, px, pz);

			if ((i & CORNF_NE_EN) == CORNF_NE_EN)
				drawcorner (rprend, CORNER_NE, px, pz);
		}
	}

	/*
	 * Draw player.
	 */
	px = XORG + ConvertF16_32 (playerpos.X) * 3;
	pz = YORG - ConvertF16_32 (playerpos.Z) * 3;
	drawglyph (rprend, GLYPH_PLAYER, px, pz);

	/*
	 * Draw compass and needle.
	 */
	DrawCels (rprend->rp_BitmapItem, ca_compass->celptrs[0]);


	newmat (&unit);
	applyyaw (&unit, &cam, playerdir);
	MulManyVec3Mat33_F16 ((VECTCAST) xfneedle,
			      (VECTCAST) needlequad,
			      (MATCAST) &cam,
			      4);

	for (i = 4;  --i >= 0; ) {
		quad[i].pt_X = ConvertF16_32 (xfneedle[i].X) + CX_NEEDLE;
		quad[i].pt_Y = ConvertF16_32 (-xfneedle[i].Z) + CY_NEEDLE;
	}

	ccb = ca_needle->celptrs[0];
	FasterMapCel (ccb, quad);
	DrawCels (rprend->rp_BitmapItem, ccb);

	/*
	 * Because of the way "upstairs" is written, this is the correct
	 * thing to do to prevent the status screen from blinking in for
	 * one frame before resuming the game.
	 */
	DisplayScreen (rprend->rp_ScreenItem, 0);


	/*
	 * Sit and wait for button press.
	 */
	resetjoydata ();

	while (!(joytrigger & (ControlA | ControlC | ControlX)))
		WaitVBL (vblIO, 2);

	i = joytrigger;
	resetjoydata ();

	if (i & ControlA)
		mf = 1;
	else if (i & ControlX)
		mf = -1;
	else
		mf = 0;

	return (mf);
}




/***************************************************************************
 * The bit that draws the corners.
 */
static ubyte	surfoffs[4][2] = {
	0, 0,
	0, 2,
	2, 2,
	2, 0
};

static ubyte	midoffs[4][2][2] = {
 {
	0, 1,	1, 0
 }, {
	0, 1,	1, 2
 }, {
	2, 1,	1, 2
 }, {
	2, 1,	1, 0
 }
};

static ubyte	outoffs[4][5][2] = {
 {
	0, 2,	1, 2,	2, 2,	2, 1,	2, 0
 }, {
	0, 0,	1, 0,	2, 0,	2, 1,	2, 2
 }, {
	0, 2,	0, 1,	0, 0,	1, 0,	2, 0
 }, {
	0, 0,	0, 1,	0, 2,	1, 2,	2, 2
 }
};


static void
drawcorner (rp, type, x, z)
RastPort	*rp;
int		type;
int32		x, z;
{
	register int	i, p;
	register int32	px, pz;
	GrafCon		gc;
	Item		bmi;

	bmi = rp->rp_BitmapItem;

	/*
	 * Draw "surface" of corner.
	 */
	SetFGPen (&gc, COLR_SURF);
	WritePixel (bmi, &gc, x + surfoffs[type][0], z + surfoffs[type][1]);

	/*
	 * Draw middle band.
	 */
	SetFGPen (&gc, COLR_MID);
	WritePixel (bmi, &gc, x + 1, z + 1);

	for (i = 2;  --i >= 0; ) {
		px = x + midoffs[type][i][0];
		pz = z + midoffs[type][i][1];
		if (ReadPixel (bmi, &gc, px, pz) != COLR_SURF)
			WritePixel (bmi, &gc, px, pz);
	}

	/*
	 * Draw outer band.
	 */
	SetFGPen (&gc, COLR_OUTER);
	for (i = 5;  --i >= 0; ) {
		px = x + outoffs[type][i][0];
		pz = z + outoffs[type][i][1];
		p = ReadPixel (bmi, &gc, px, pz);
		if (p != COLR_SURF  &&  p != COLR_MID)
			WritePixel (bmi, &gc, px, pz);
	}
}


/***************************************************************************
 * Routine to draw specific glyphs.
 */
#define	COLR_PLAYER	MakeRGB15 (31, 8, 8)
#define	COLR_DOOR	MakeRGB15 (8, 20, 8)
#define	COLR_TALISMAN	MakeRGB15 (24, 24, 0)
#define	COLR_EXIT	MakeRGB15 (31, 31, 31)


static ubyte	player[] = {
	1, 0,	1, 1,	1, 2,	0, 1,	2, 1
};
#define	NOFFS_PLAYER	(sizeof (player) / 2)


static ubyte	nsdoor[] = {
	0, 1,	1, 1,	2, 1
};

static ubyte	ewdoor[] = {
	1, 0,	1, 1,	1, 2
};
#define NOFFS_DOOR	(sizeof (nsdoor) / 2)


static ubyte	exit[] = {
	0, 0,	1, 1,	2, 2,	2, 0,	0, 2
};
#define	NOFFS_EXIT	(sizeof (exit) / 2)


static void
checkglyph (rp, me, x, z)
RastPort	*rp;
MapEntry	*me;
int32		x, z;
{
	register Object	*ob;
	register int	type, obtype;

	type = -1;
	for (ob = me->me_Obs;  ob;  ob = ob->ob_Next) {
		obtype = ob->ob_Type;
		if (obtype == OTYP_NSDOOR)	type = GLYPH_NSDOOR;
		else if (obtype == OTYP_EWDOOR)	type = GLYPH_EWDOOR;
		else if (obtype == OTYP_EXIT)	type = GLYPH_EXIT;
		else if (obtype == OTYP_POWERUP  &&
			 ((ObPowerup *) ob)->ob_PowerType == PWR_TALISMAN)
			type = GLYPH_TALISMAN;

		if (type >= 0)
			if ((ob->ob_Flags & OBF_SAWME)  ||
			    (obtype == OTYP_EXIT  &&
			     me->me_VisFlags & MAPF_ALLDIRS))
				drawglyph (rp, type, x, z);
	}
}



static void
drawglyph (rp, type, x, z)
RastPort	*rp;
int		type;
int32		x, z;
{
	switch (type) {
	case GLYPH_PLAYER:
		drawpoints (rp, x, z, player, NOFFS_PLAYER, COLR_PLAYER);
		break;

	case GLYPH_TALISMAN:
		drawpoints (rp, x, z, player, NOFFS_PLAYER, COLR_TALISMAN);
		break;

	case GLYPH_EXIT:
		drawpoints (rp, x, z, exit, NOFFS_EXIT, COLR_EXIT);
		break;

	case GLYPH_NSDOOR:
	case GLYPH_EWDOOR:
		drawpoints (rp, x, z,
			    type == GLYPH_NSDOOR  ?  nsdoor  :  ewdoor,
			    NOFFS_DOOR, COLR_DOOR);
		break;
	}
}



static void
drawpoints (rp, x, z, offs, noffs, color)
RastPort	*rp;
register int32	x, z;
register ubyte	*offs;
register int32	noffs;
int32		color;
{
	GrafCon	gc;
	Item	bmi;

	SetFGPen (&gc, color);
	bmi = rp->rp_BitmapItem;

	while (--noffs >= 0) {
		WritePixel (bmi, &gc, x + offs[0], z + offs[1]);
		offs += 2;
	}
}


/***************************************************************************
 * Housekeeping.
 */
void
loadmap ()
{
	register CCB	*ccb;
	register int	i;
	int32		nw, nh;

	if (!(ca_glyphs = parse3DO ("MapGlyphs.cel")))
		die ("Couldn't load map glyphs.\n");

	if (!(ca_compass = parse3DO ("Compass.cel")))
		die ("Couldn't load compass image.\n");

	if (!(ca_needle = parse3DO ("Needle.cel")))
		die ("Couldn't load needle image.\n");


	for (i = ca_glyphs->ncels;  --i >= 0; ) {
		ccb = ca_glyphs->celptrs[i];

		ccb->ccb_Flags |= CCB_LAST | CCB_NPABS | CCB_LDSIZE |
				  CCB_LDPRS | CCB_LDPPMP | CCB_YOXY |
				  CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);
	}


	ccb = ca_compass->celptrs[0];
	ccb->ccb_Flags |= CCB_LAST | CCB_NPABS | CCB_LDSIZE |
			  CCB_LDPRS | CCB_LDPPMP | CCB_YOXY |
			  CCB_ACW | ccbextra;
	ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

	ccb->ccb_XPos = XMARGIN << 16;
	ccb->ccb_YPos = YMARGIN << 16;


	ccb = ca_needle->celptrs[0];
	ccb->ccb_Flags |= CCB_LAST | CCB_NPABS | CCB_LDSIZE |
			  CCB_LDPRS | CCB_LDPPMP | CCB_YOXY |
			  CCB_ACW | ccbextra;
	ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

	nw = REALCELDIM (ccb->ccb_Width) >> 1;
	nh = REALCELDIM (ccb->ccb_Height) >> 1;

	needlequad[0].X =
	needlequad[3].X = Convert32_F16 (-nw);

	needlequad[1].X =
	needlequad[2].X = Convert32_F16 (nw);

	needlequad[0].Z =
	needlequad[1].Z = Convert32_F16 (nh);

	needlequad[2].Z =
	needlequad[3].Z = Convert32_F16 (-nh);
}


void
freemap ()
{
	if (ca_needle) {
		freecelarray (ca_needle);
		ca_needle = NULL;
	}
	if (ca_compass) {
		freecelarray (ca_compass);
		ca_compass = NULL;
	}
	if (ca_glyphs) {
		freecelarray (ca_glyphs);
		ca_glyphs = NULL;
	}
}
