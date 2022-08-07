/*  :ts=8 bk=0
 *
 * rend.c:	Rendering stuff.
 *
 * Leo L. Schwab					9305.18
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "imgfile.h"
#include "loaf.h"
#include "anim.h"

#include "app_proto.h"


/***************************************************************************
 * Globals.
 */
extern RastPort	*rprend, *rpvis;
extern Item	vblIO;

extern Vertex	xfverts[], projverts[];
extern int16	grididxs[];
extern VisOb	visobs[];
extern int32	nviso;
extern uint32	vertsused[];
extern int32	nvisv;
extern Vertex	obverts[], xfobverts[], *curobv;
extern int32	nobverts;

extern Matrix	camera;
extern Vertex	playerpos, campos;
extern Vertex	xformsquare[];
extern Vector	plusx, plusz, minusx, minusz;

extern ImageEntry	*wallimgs;
extern CCB	*wallcel, *statpanel, *guncel;
extern int32	wide, high, cy;
extern uint32	ccbextra;

extern int32	floorcolor, ceilingcolor;

extern JoyData	jd;


AnimLoaf	*wallanims;
int32		nwallanims;
int16		*scaledpluts, *curscaledplut;

CCB		ccbpool[MAXVISOBS];
CCB		*curccb;

CCB		*backwallcel;

uint32		linebuf[10];


/***************************************************************************
 * Code.
 */
void
rendercels ()
{
	resetlinebuf (linebuf);
	curccb = ccbpool;
	curscaledplut = scaledpluts;

	buildcellist (visobs, nviso, projverts, xfverts, TRUE);

	if (curccb > ccbpool) {
		backwallcel->ccb_NextPtr = curccb - 1;
		ccbpool->ccb_NextPtr = guncel;
	} else
		backwallcel->ccb_NextPtr = guncel;
	DrawCels (rprend->rp_BitmapItem, backwallcel);

	sequencegun (0);
}



void
buildcellist (vo, nvo, verts, xfverts, indirect)
register struct VisOb	*vo;
register int		nvo;
register struct Vertex	*verts;
struct Vertex		*xfverts;
int			indirect;
{
#define	ZFADENEAR_F16	(8 * ONE_F16)
#define	ZFADEFAR_F16	(GRIDCUTOFF * ONE_F16)

	register CCB		*ccb;
	register int32		lidx, ridx;
	Vertex			*vptrs[4];
	Point			corner[4];
	int			zflags;

	for ( ;  --nvo >= 0;  vo++) {
		if (islinefull (linebuf))
			break;

		lidx = vo->vo_LIdx;
		ridx = vo->vo_RIdx;

		if (lidx < 0) {
			renderobs (vo);
			continue;
		}

		if (indirect) {
			lidx = grididxs[lidx];
			ridx = grididxs[ridx];
		}
		lidx <<= 1;
		ridx <<= 1;

		zflags = 0;
		if (verts[lidx].Z < (ZCLIP >> 8))	zflags |= 0x3;
		if (verts[ridx].Z < (ZCLIP >> 8))	zflags |= 0xC;
		if (zflags == 0xF)
			continue;

		if (!zflags)
			if (vo->vo_MEFlags & MEF_OPAQUE) {
				if (!testmarklinebuf (linebuf,
						      verts[lidx].X,
						      verts[ridx].X))
					continue;
			} else {
				if (!testlinebuf (linebuf,
						  verts[lidx].X,
						  verts[ridx].X))
					continue;
			}

		if (verts[lidx].Z > (ZFADEFAR_F16 >> 8))
			/*
			 * Too far away, don't render.
			 */
			continue;

		corner[0].pt_X = verts[lidx].X;
		corner[0].pt_Y = verts[lidx].Y;

		corner[1].pt_X = verts[lidx + 1].X;
		corner[1].pt_Y = verts[lidx + 1].Y;

		corner[2].pt_X = verts[ridx + 1].X;
		corner[2].pt_Y = verts[ridx + 1].Y;

		corner[3].pt_X = verts[ridx].X;
		corner[3].pt_Y = verts[ridx].Y;

		ccb = curccb;
		img2ccb (&wallimgs[vo->vo_ImgIdx], ccb);

		if (verts[lidx].Z > (ZFADENEAR_F16 >> 8)) {
			frac16	value;

			value = DivSF16 (ZFADEFAR_F16 - (verts[lidx].Z << 8),
					 ZFADEFAR_F16 - ZFADENEAR_F16);
			fadecel (ccb, value);
		}

		if (zflags) {
			register frac16	factor;

			mkVertPtrs
			 (xfverts, vptrs, lidx, lidx + 1, ridx + 1, ridx);

			factor = zclip (vptrs, zflags, corner);

			if (vo->vo_MEFlags & MEF_OPAQUE) {
				if (!testmarklinebuf (linebuf,
						      corner[0].pt_X,
						      corner[3].pt_X))
				{
					/*
					 * Oops, it's invisible after all.
					 */
					continue;
				}
			} else {
				if (!testlinebuf (linebuf,
						  corner[0].pt_X,
						  corner[3].pt_X))
					continue;
			}

			zClipCCB (ccb, factor);
		}


		FasterMapCel (ccb, corner);

		if (vo->vo_ME)
			vo->vo_ME->me_VisFlags |= vo->vo_VisFlags << 4;

		ccb->ccb_Flags &= ~CCB_LAST;
		curccb++;
	}
}


void
img2ccb (ie, ccb)
register struct ImageEntry	*ie;
register struct CCB		*ccb;
{
	register struct ShortCCB	*sccb;

	sccb = ie->ie_SCCB;

	ccb->ccb_Flags		= sccb->sccb_Flags;
	ccb->ccb_PRE0		= sccb->sccb_PRE0;
	ccb->ccb_PRE1		= sccb->sccb_PRE1;
	ccb->ccb_Width		= sccb->sccb_Width;
	ccb->ccb_Height		= sccb->sccb_Height;
	ccb->ccb_SourcePtr	= sccb->sccb_PDAT;
	ccb->ccb_PLUTPtr	= ie->ie_PLUT;
	ccb->ccb_PIXC		= ie->ie_PPMPC;
}





void
cyclewalls (nvbls)
int32	nvbls;
{
	cycleanimloafs (wallanims, nwallanims, nvbls);
}


void
initwallanim (iev)
struct ImageEnv	*iev;
{
	if ((nwallanims = createanimloafs (iev, &wallanims)) < 0)
		die ("Can't allocate AnimLoaf table.\n");
}

void
closewallanim ()
{
	if (wallanims)	deleteanimloafs (wallanims), wallanims = NULL;
}





void
cycleanimloafs (al, nal, nvbls)
register struct AnimLoaf	*al;
register int32			nal, nvbls;
{
	register AnimEntry	*ae;
	register int		n;

	while (--nal >= 0) {
		ae = al->al_AE;
		n = nextanimframe (&al->al_Def, al->al_CurFrame, nvbls);
		while (n >= ae->ae_NFrames)
			n -= ae->ae_NFrames;
		al->al_CurFrame = n;

		*ae->ae_Target = ae->ae_Base[n];
		al++;
	}
}

int32
createanimloafs (iev, alp)
struct ImageEnv *iev;
struct AnimLoaf	**alp;
{
	register AnimLoaf	*al;
	register AnimEntry	*ae;
	register int32		i, n;

	if (n = iev->iev_NAnimEntries) {
		if (!(al = malloc (sizeof (AnimLoaf) * n)))
			return (-1);

		for (i = 0, ae = iev->iev_AnimEntries;  i < n;  i++, ae++) {
			al[i].al_Def.ad_FPS	= ae->ae_FPS;
			al[i].al_Def.ad_Counter	= VBLANKFREQ;
			al[i].al_CurFrame	= 0;
			al[i].al_AE		= ae;
		}
		*alp = al;
	} else
		*alp = NULL;

	return (n);
}

void
deleteanimloafs (al)
struct AnimLoaf	*al;
{
	free (al);
}







void
processgrid ()
{
#define	NVU		((NGRIDPOINTS + 31) >> 5)

	register Vertex	*vert;
	register uint32	mask, *mptr;
	register int16	*idxp;
	register int	i, n, x;
	Vector		backuprow;
	Vector		skip32;
	Vertex		point;

	skip32.X = plusx.X * 32;
	skip32.Y = plusx.Y * 32;
	skip32.Z = plusx.Z * 32;

	backuprow.X = minusx.X * GRIDSIZ + plusz.X;
	backuprow.Y = minusx.Y * GRIDSIZ + plusz.Y;
	backuprow.Z = minusx.Z * GRIDSIZ + plusz.Z;

	vert = xfverts;
	idxp = grididxs;
	point = xformsquare[0];
	mptr = vertsused;
	x = 0;
	for (i = NVU;  --i >= 0; ) {
		mask = *mptr++;
		if (!mask) {
			idxp += 32;
			point.X += skip32.X;
			point.Z += skip32.Z;
			if ((x += 32) >= GRIDSIZ) {
				point.X += backuprow.X;
				point.Z += backuprow.Z;
				x -= GRIDSIZ;
			}
			continue;
		}

		for (n = 32;  --n >= 0;  mask >>= 1) {
			if (mask & 1) {
				*vert++ = point;
				vert->X = point.X;
				vert->Y = point.Y - ONE_F16;
				vert->Z = point.Z;
				vert++;

				*idxp = nvisv++;
			}

			point.X += plusx.X;
			point.Z += plusx.Z;
			if (++x >= GRIDSIZ) {
				point.X += backuprow.X;
				point.Z += backuprow.Z;
				x -= GRIDSIZ;
			}
			idxp++;
		}
	}

if (nvisv >= MAXWALLVERTS/2)
 kprintf ("EEEK!  xfverts[] overflow at %d\n", nvisv);

	project (xfverts, projverts, MAGIC, 0, CX, cy, nvisv + nvisv);
}


void
processvisobs ()
{
	register VisOb	*vo;
	register int	i;

	curobv = obverts;
	nobverts = 0;
	for (vo = visobs, i = nviso;  --i >= 0;  vo++) {
		if (vo->vo_LIdx < 0)
			registerobs (vo);
	}
if (nviso > MAXVISOBS)
 kprintf ("EEEK!  visobs[] overflow at %d\n", nviso);
if (nobverts > NOBVERTS)
 kprintf ("EEEK!  obverts[] overflow! at %d\n", nobverts);

	if (nobverts) {
		Vector	trans;

		trans.X = -campos.X;
		trans.Y = -campos.Y;
		trans.Z = -campos.Z;
		translatemany (&trans, obverts, nobverts);
		MulManyVec3Mat33_F16 ((VECTCAST) xfobverts,
				      (VECTCAST) obverts,
				      (MATCAST) &camera,
				      nobverts);

		project (xfobverts, obverts, MAGIC, 0, CX, cy, nobverts);
	}
}




void
initrend ()
{
	register CCB	*ccb;
	register int	i;

	for (i = MAXVISOBS, ccb = ccbpool;  --i >= 0;  ccb++)
		ccb->ccb_NextPtr = ccb - 1;

	if (!(scaledpluts = malloctype (1024, MEMTYPE_CEL)))
		die ("Can't allocate memory for scaled PLUTs.\n");

	createbackwall ();
}

void
closerend ()
{
	if (scaledpluts)	freetype (scaledpluts), scaledpluts = NULL;
	if (backwallcel)	freetype (backwallcel), backwallcel = NULL;
}




/***************************************************************************
 * This is the bit that plays out the "death" stream to a CCB, which we
 * then render over the display.
 */
uint32		celPPMPs[] = {
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_1 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_2 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_3 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_4 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_16 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1,
	PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1
};
uint32		fadePPMPs[] = {
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_1 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_2 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_3 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_4 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8 | PPMPC_2S_0 | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_1 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_2 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_3 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_4 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2,
	PPMPC_1S_CFBD | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_2
};
static CelArray	*ca_skull;
static CCB	*fader;

void
loadskull ()
{
	register struct CCB	*ccb;
	register int		i;

	if (!(ca_skull = parse3DO ("skull.cel")))
		die ("Couldn't load skull image.\n");

	if (!(fader = malloctype (sizeof (CCB), MEMTYPE_CEL | MEMTYPE_FILL)))
		die ("Couldn't create fader CCB.\n");

	ccb = ca_skull->celptrs[0];
	ccb->ccb_Flags |= CCB_LAST | CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
	ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

	memcpy (fader, ccb, sizeof (*ccb));
	fader->ccb_Flags &= ~CCB_LAST;
	fader->ccb_NextPtr = ccb;

	for (i = 16;  --i >= 0; ) {
		fadePPMPs[i] = fadePPMPs[i] | (fadePPMPs[i] << PPMP_1_SHIFT);
		celPPMPs[i] = celPPMPs[i] | (celPPMPs[i] << PPMP_1_SHIFT);
	}
}

void
freeskull ()
{
	freetype (fader);  fader = NULL;
	freecelarray (ca_skull);  ca_skull = NULL;
}


void
simpledeath ()
{
	register struct CCB	*ccb;
	register int		i, idx;
	Point			quad[4];
	int32			checkspool;
	int			cwide, chigh, w, h;
	void			*tmp;

	stopspoolsound (1);

	ccb = ca_skull->celptrs[0];

	cwide = REALCELDIM (ccb->ccb_Width);
	chigh = REALCELDIM (ccb->ccb_Height);
	checkspool = TRUE;

	for (i = 0;  i < 128;  i++) {
		w = (cwide * (3 * i / 4 + 32)) >> 6;
		h = (chigh * (3 * i / 4 + 32)) >> 6;

		quad[0].pt_X =
		quad[3].pt_X = (wide - w) >> 1;

		quad[1].pt_X =
		quad[2].pt_X = (wide + w) >> 1;

		quad[0].pt_Y =
		quad[1].pt_Y = (high - h) >> 1;

		quad[2].pt_Y =
		quad[3].pt_Y = (high + h) >> 1;

		FasterMapCel (ccb, quad);
		FasterMapCel (fader, quad);

		if ((idx = i >> 3) > 15)	idx = 15;
		ccb->ccb_PIXC = celPPMPs[idx];
		fader->ccb_PIXC = fadePPMPs[15 - idx];

		clearscreen (rprend);
		DrawCels (rprend->rp_BitmapItem, backwallcel);
		DrawCels (rprend->rp_BitmapItem, fader);

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);

		if (checkspool  &&  !issoundspooling ()) {
			checkspool = FALSE;
			spoolsound ("$progdir/aiff/LaughStatic.aiff", 1);
		}
		WaitVBL (vblIO, 1);
	}
	WaitVBL (vblIO, 60);

	fadetoblank (rpvis, 30);
}








#if 0

/*  It won't work...  */

void
playdeath ()
{
	static Point	fullscr[4] = {
		0, 0,  320, 0,  320, 240,  0, 240
	};
	register CCB	*ccb;

	openstream ("$progdir/Ganymede");

	startstream ();
	while (!endofstream ()) {
		WaitVBL (vblIO, 1);

		if (ccb = getstreamccb ()) {
			ccb->ccb_Flags	|= CCB_LAST;
			ccb->ccb_NextPtr= NULL;
			ccb->ccb_PIXC	=
			 PPMP_MODE_AVERAGE | (PPMP_MODE_AVERAGE << 16);
			FasterMapCel (ccb, fullscr);

			DrawCels (rprend->rp_BitmapItem, backwallcel);
			DrawCels (rprend->rp_BitmapItem, ccb);
		}
	}
	stopstream ();
	closestream ();
}
#endif


/***************************************************************************
 * The following code has been converted to assembly code, which probably
 * doesn't run much faster than the compiled stuff, since I don't seem to
 * be quite a good enough ARM programmer to reliably outstrip the compiler.
 *
 * If you need to revert to something more malleable, take out the #if
 * and #endif statements.
 */


static uint32	leftmasks[] = {
	0xFFFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF, 0x1FFFFFFF,
	0x0FFFFFFF, 0x07FFFFFF, 0x03FFFFFF, 0x01FFFFFF,
	0x00FFFFFF, 0x007FFFFF, 0x003FFFFF, 0x001FFFFF,
	0x000FFFFF, 0x0007FFFF, 0x0003FFFF, 0x0001FFFF,
	0x0000FFFF, 0x00007FFF, 0x00003FFF, 0x00001FFF,
	0x00000FFF, 0x000007FF, 0x000003FF, 0x000001FF,
	0x000000FF, 0x0000007F, 0x0000003F, 0x0000001F,
	0x0000000F, 0x00000007, 0x00000003, 0x00000001
};

static uint32	rightmasks[] = {
	0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
	0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
	0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
	0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
	0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
	0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
	0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
	0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF
};


int
testlinebuf (linebuf, lx, rx)
register uint32	*linebuf;
register int32	lx, rx;
{
	register uint32	lm, rm;

	if (lx == rx)
		return (FALSE);

	rx--;
	if (lx > rx) {
		lx ^= rx;  rx ^= lx;  lx ^= rx;
	}
	if (lx >= 320  ||  rx < 0)
		return (FALSE);

	if (lx < 0)	lx = 0;
	if (rx >= 320)	rx = 320;

	lm = leftmasks[lx & 31];
	rm = rightmasks[rx & 31];

	lx >>= 5;
	rx >>= 5;

	if (lx == rx) {
		lm &= rm;
		return ((linebuf[lx] & lm) != 0);
	} else {
		if (linebuf[lx] & lm)
			return (TRUE);

		lm = ~0;
		do {
			if (++lx == rx)
				lm = rm;
			if (linebuf[lx] & lm)
				return (TRUE);
		} while (lx < rx);

		return (FALSE);
	}
}


#if 0
/*
 * Returns TRUE if you need to draw a polygon; FALSE if you don't.
 * The specified region is marked as used both cases.
 * Note: Marking a region actually means clearing the bits.
 */
int
testmarklinebuf (dummy, lx, rx)
uint32		*dummy;
register int32	lx, rx;
{
	register uint32	lm, rm;

	if (lx == rx)
		return (FALSE);

	rx--;
	if (lx > rx) {
		lx ^= rx;  rx ^= lx;  lx ^= rx;
	}
	if (lx >= 320  ||  rx < 0)
		return (FALSE);

	if (lx < 0)	lx = 0;
	if (rx >= 320)	rx = 320;

	lm = leftmasks[lx & 31];
	rm = rightmasks[rx & 31];

	lx >>= 5;
	rx >>= 5;

	if (lx == rx) {
		lm &= rm;
		if (linebuf[lx] & lm) {
			linebuf[lx] &= ~lm;
			return (TRUE);
		} else
			return (FALSE);
	} else {
		register int	result = FALSE;

		if (linebuf[lx] & lm)
			result = TRUE;

		linebuf[lx] &= ~lm;
		lm = ~0;
		do {
			if (++lx == rx)
				lm = rm;
			if (linebuf[lx] & lm)
				result = TRUE;
			linebuf[lx] &= ~lm;
		} while (lx < rx);

		return (result);
	}
}


void
resetlinebuf (dummy)
uint32	*dummy;
{
	register int32	*lb;
	register int	i;

	lb = (int32 *) linebuf;
	for (i = 10;  --i >= 0; )
		*lb++ = ~0;
}


int
islinefull (dummy)
uint32	*dummy;
{
	register int32	*lb;
	register int	i;

	lb = (int32 *) linebuf;
	for (i = 10;  --i >= 0; )
		if (*lb++)
			return (FALSE);

	return (TRUE);
}

#endif





/***************************************************************************
 * Fade a cel into the distance.
 * value is a fixed-point quantity.  0 == totally black, 1.0 == totally
 * visible.
 *
 * This is an expensive operation.  I'm relying on the probability that this
 * will be done seldom enough to not impact the frame rate significantly.
 */
void
fadecel (ccb, value)
register struct CCB	*ccb;
frac16			value;
{
	register uint32	pre0;

	if (ccb->ccb_Flags & CCB_CCBPRE)
		pre0 = ccb->ccb_PRE0;
	else
		pre0 = * (int32 *) ccb->ccb_SourcePtr;

	if ((pre0 & PRE0_BPP_MASK) == PRE0_BPP_8  &&  !(pre0 & PRE0_LINEAR))
	{
		/*
		 * Ack!  Coded-8.  We have to hack the whole PLUT.
		 **
		 * ewhac 9308.03:  This has yet to be, like, actually tested.
		 */
		register int16	*src, *dest;
		register int	i, r, g, b;

		src = (int16 *) ccb->ccb_PLUTPtr;
		dest = curscaledplut;
		ccb->ccb_PLUTPtr = dest;
		for (i = 32;  --i >= 0; ) {
			b = *src++;
			r = (b >> 10) & 0x1F;
			g = (b >> 5) & 0x1F;
			b &= 0x1F;

			r = r * value >> 16;
			g = g * value >> 16;
			b = b * value >> 16;

			*dest++ = b | (g << 5) | (r << 10);
		}
		curscaledplut = dest;
	} else {
		/*
		 * Ahhh, pleasanter.  We need only dink the PpIpXmCp.
		 * This will screw up Doloresizing, but at that distance,
		 * who cares?
		 */
		register int32	val0, val1;

		val0 = ccb->ccb_PIXC & PPMPC_MF_MASK;
		val1 = (ccb->ccb_PIXC >> PPMP_1_SHIFT) & PPMPC_MF_MASK;

		val0 = (val0 * value >> 16) & PPMPC_MF_MASK;
		val1 = (val1 * value >> 16) & PPMPC_MF_MASK;
		ccb->ccb_PIXC &= ~(PPMPC_MF_MASK |
				   (PPMPC_MF_MASK << PPMP_1_SHIFT));
		ccb->ccb_PIXC |= val0 | (val1 << PPMP_1_SHIFT);
	}
}




/***************************************************************************
 * Generate "backwall" cel that will fade ceiling and floor nicely to black.
 * (Part of initrend()?)
 *
 * Ideally, this routine will if() out the entire CCB creation and
 * initialization section, making it part of the openstuff() path.
 */
void
createbackwall ()
{
	static Vertex	stuff[4] = {
		{ 0,	-HALF_F16,	ZFADENEAR_F16	},
		{ 0,	-HALF_F16,	ZFADEFAR_F16	},
		{ 0,	HALF_F16,	ZFADEFAR_F16	},
		{ 0,	HALF_F16,	ZFADENEAR_F16	},
	};
	register int32	i;
	int32		*pdat;
	int32		ydim, size, diff, color, r, g, b;

	if (!stuff[0].X) {
		project (stuff, stuff, MAGIC, 0, CX, cy, 4);
		stuff[0].X = 1;
	}

	ydim = stuff[3].Y - stuff[0].Y;

	size = sizeof (CCB) + ydim * sizeof (int32) * 2;
	if (!(backwallcel = malloctype (size, MEMTYPE_CEL)))
		die ("Can't create backwall.\n");

	pdat = (int32 *) (backwallcel + 1);

	backwallcel->ccb_Flags	= CCB_NPABS | CCB_SPABS | CCB_LDSIZE |
				  CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE |
				  CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_ACE |
				  CCB_BGND | CCB_NOBLK;
	backwallcel->ccb_SourcePtr = (CelData *) pdat;
	backwallcel->ccb_PLUTPtr= NULL;
	backwallcel->ccb_PIXC	= 0x1F001F00;

	backwallcel->ccb_PRE0	= PRE0_LITERAL | PRE0_BGND | PRE0_LINEAR |
				  PRE0_BPP_16 |
				  ((ydim - PRE0_VCNT_PREFETCH) <<
				   PRE0_VCNT_SHIFT);
	backwallcel->ccb_PRE1	= (0 << PRE1_WOFFSET10_SHIFT) |
				  PRE1_TLLSB_PDC0 |
				  (2 - PRE1_TLHPCNT_PREFETCH);

	backwallcel->ccb_Width	= 2;
	backwallcel->ccb_Height	= ydim;
	backwallcel->ccb_XPos	= 0;
	backwallcel->ccb_YPos	= (cy - ydim / 2) << 16;
	backwallcel->ccb_HDX	= 160 << 20;
	backwallcel->ccb_VDX	= 0;
	backwallcel->ccb_HDY	= 0;
	backwallcel->ccb_VDY	= 1 << 16;
	backwallcel->ccb_HDDX	=
	backwallcel->ccb_HDDY	= 0;


	/*
	 * Gradation from ceiling color to black.
	 */
	r = (ceilingcolor >> 10) & 0x1f;
	g = (ceilingcolor >> 5) & 0x1f;
	b = ceilingcolor & 0x1f;

	diff = stuff[1].Y - stuff[0].Y;
	for (i = diff;  --i >= 0; ) {
		color = ((r * i / diff) << 10) +
			((g * i / diff) << 5) +
			(b * i / diff);
		color |= color << 16;

		*pdat++ = color;
		*pdat++ = color;
	}

	/*
	 * Middle black band.
	 */
	diff = stuff[2].Y - stuff[1].Y;
	for (i = diff;  --i >= 0; ) {
		*pdat++ = 0;
		*pdat++ = 0;
	}

	/*
	 * Gradation from black to floor color.
	 */
	r = (floorcolor >> 10) & 0x1f;
	g = (floorcolor >> 5) & 0x1f;
	b = floorcolor & 0x1f;

	diff = stuff[3].Y - stuff[2].Y;

	for (i = 0;  i < diff;  i++) {
		color = ((r * i / diff) << 10) +
			((g * i / diff) << 5) +
			(b * i / diff);
		color |= color << 16;

		*pdat++ = color;
		*pdat++ = color;
	}
}





/***************************************************************************
 * Take a cel and a cel and create position and delta values to map
 * its corners onto the specified quadrilateral.
 **
 * ewhac 9306.10: Optimizes for power-of-two edges, which are flagged by
 * negative Width/Height fields.
 *
 * If Width/Height are positive, the dimension is the value.
 * If negative or zero, the dimension is 1 << -value, i.e. it's a shift
 * count.
 */
void
FasterMapCel (ccb, p)
register struct CCB	*ccb;
register struct Point	*p;
{
	register int32	wide, high;
	register int	i = 2;

	ccb->ccb_XPos = (p[0].pt_X << 16) + 0x8000;
	ccb->ccb_YPos = (p[0].pt_Y << 16) + 0x8000;

	if ((wide = ccb->ccb_Width) <= 0) {
		ccb->ccb_HDX = (p[1].pt_X - p[0].pt_X) << (20 + wide);
		ccb->ccb_HDY = (p[1].pt_Y - p[0].pt_Y) << (20 + wide);
		--i;
	} else {
		ccb->ccb_HDX = ((p[1].pt_X - p[0].pt_X) << 20) / wide;
		ccb->ccb_HDY = ((p[1].pt_Y - p[0].pt_Y) << 20) / wide;
	}

	if ((high = ccb->ccb_Height) <= 0) {
		ccb->ccb_VDX = (p[3].pt_X - p[0].pt_X) << (16 + high);
		ccb->ccb_VDY = (p[3].pt_Y - p[0].pt_Y) << (16 + high);
		--i;
	} else {
		ccb->ccb_VDX = ((p[3].pt_X - p[0].pt_X) << 16) / high;
		ccb->ccb_VDY = ((p[3].pt_Y - p[0].pt_Y) << 16) / high;
	}

	if (!i) {
		wide += high;
		ccb->ccb_HDDX =
		 (p[2].pt_X - p[3].pt_X - p[1].pt_X + p[0].pt_X) <<
		 (20 + wide);
		ccb->ccb_HDDY =
		 (p[2].pt_Y - p[3].pt_Y - p[1].pt_Y + p[0].pt_Y) <<
		 (20 + wide);
	} else {
		/*
		 * Regenerate actual dimensions if necessary.
		 */
		if (wide <= 0)
			wide = 1 << -wide;
		else if (high <= 0)
			high = 1 << -high;

		wide *= high;
		ccb->ccb_HDDX =
		 ((p[2].pt_X - p[3].pt_X - p[1].pt_X + p[0].pt_X) << 20) /
		 wide;
		ccb->ccb_HDDY =
		 ((p[2].pt_Y - p[3].pt_Y - p[1].pt_Y + p[0].pt_Y) << 20) /
		 wide;
	}
}
