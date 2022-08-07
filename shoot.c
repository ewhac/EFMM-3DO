/*  :ts=8 bk=0
 *
 * shoot.c:	How to kill things.
 *
 * Leo L. Schwab					9305.31
 */
#include <types.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "imgfile.h"
#include "sound.h"

#include "app_proto.h"



typedef struct ShotInfo {
	struct MapEntry	*si_me;
	struct Object	*si_Ob;
	frac16		si_Dist;
	int		si_Op;
	int		si_CheckFlag;
} ShotInfo;


/***************************************************************************
 * Local prototypes.
 */
static void doshoot(struct ShotInfo *si);
static void shoot_north(struct ShotInfo *si);
static void shoot_west(struct ShotInfo *si);
static void shoot_south(struct ShotInfo *si);
static void shoot_east(struct ShotInfo *si);


/***************************************************************************
 * Globals
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];

extern Vertex	obverts[], xfobverts[];
extern Vertex	playerpos;
extern frac16	playerdir;
extern int32	gunpower;

extern int32	wide, high;

extern RastPort	*rprend;



void
shoot ()
{
	struct ShotInfo	si;

	if (gunpower <= 0) {
		playsound (SFX_GUNEMPTY);
		return;
	}

	if ((gunpower -= 5) < 0)
		gunpower = 0;

	si.si_me = NULL;
	si.si_Ob = NULL;
	si.si_Dist = -1;
	si.si_Op = OP_SHOT;
	si.si_CheckFlag = OBF_SHOOT;

	doshoot (&si);
	sequencegun (si.si_Dist);
	recoil ();

	if (gunpower)
		playsound (SFX_GUNZAP);
	else
		playsound (SFX_GUNDRAINED);
}


void
probe ()
{
	struct ShotInfo	si;

	si.si_me = NULL;
	si.si_Ob = NULL;
	si.si_Dist = -1;
	si.si_Op = OP_PROBE;
	si.si_CheckFlag = OBF_PROBE;

	doshoot (&si);
}


static void
doshoot (si)
struct ShotInfo	*si;
{
	register int32	tang;

	tang = ((playerdir + Convert32_F16 (256 / 8)) >> 22) & 0x3;
	switch (tang) {
	case 0:
		shoot_north (si);
		break;
	case 1:
		shoot_west (si);
		break;
	case 2:
		shoot_south (si);
		break;
	case 3:
		shoot_east (si);
		break;
	}
}



static int
checkshot (si, me)
struct ShotInfo	*si;
struct MapEntry	*me;
{
	register Object	*ob, *next;
	frac16		distance;

	ob = me->me_Obs;
	while (ob) {
		next = ob->ob_Next;

		if (ob->ob_State == OBS_DYING  ||
		    ob->ob_State == OBS_DEAD  ||
		    !ob->ob_State  ||
		    !(ob->ob_Flags & si->si_CheckFlag))
			/*  No need to kill it twice.  */
			goto donext;	// Look down.

		if (distance = (frac16) ((ob->ob_Def->od_Func)
					  (ob, si->si_Op, NULL)))
		{
			si->si_Ob = ob;
			si->si_Dist = distance;
			return (TRUE);
		}

donext:		ob = next;
	}
	return (FALSE);
}






static void
shoot_north (si)
struct ShotInfo	*si;
{
	register MapEntry	*me;
	register int		x, z;
	register frac16		fx, dx;

	dx = -DivSF16 (SinF16 (playerdir), CosF16 (playerdir));	// -Tangent

	x = ConvertF16_32 (playerpos.X);
	z = ConvertF16_32 (playerpos.Z);

	me = &levelmap[z][x];
	if (me->me_Obs  &&  checkshot (si, me))
		/*  Got him!!  */
		return;

	fx = playerpos.X + MulSF16 (dx, ONE_F16 - F_FRAC (playerpos.Z));

	while (1) {
		if (ConvertF16_32 (fx) != x) {
			if ((x = ConvertF16_32 (fx)) < 0  ||  x >= WORLDSIZ)
				break;

			me = &levelmap[z][x];
			if (me->me_Obs  &&  checkshot (si, me))
				/*  Got him!!  */
				return;

			if (me->me_Flags & MEF_SHOTSOLID) {
				/*  Shot a wall obliquely.  */
				register frac16	delta;

				if (dx < 0) {
					x++;
					delta = ONE_F16 - F_FRAC (fx);
				} else
					delta = F_FRAC (fx);

				z = Convert32_F16 (z + 1) -
				    DivSF16 (delta, dx);

				si->si_Dist =
				 approx2dist (playerpos.X, playerpos.Z,
				 	      Convert32_F16 (x), z);
				si->si_me = me;
				return;
			}
		}

		if (++z >= WORLDSIZ)
			/*  End of the world.  Stop.  */
			break;

		me = &levelmap[z][x];
		if (me->me_Obs  &&  checkshot (si, me))
			/*  Got him!!  */
			return;

		if (me->me_Flags & MEF_SHOTSOLID) {
			si->si_me = me;
			si->si_Dist = approx2dist (playerpos.X, playerpos.Z,
						   fx, Convert32_F16 (z));
			return;
		}

		fx += dx;
	}
}


static void
shoot_west (si)
struct ShotInfo	*si;
{
	register MapEntry	*me;
	register int		x, z;
	register frac16		fz, dz;

	dz = DivSF16 (CosF16 (playerdir), SinF16 (playerdir));	// 1/Tangent

	x = ConvertF16_32 (playerpos.X);
	z = ConvertF16_32 (playerpos.Z);

	me = &levelmap[z][x];
	if (me->me_Obs  &&  checkshot (si, me))
		/*  Got him!!  */
		return;

	fz = playerpos.Z + MulSF16 (dz, F_FRAC (playerpos.X));

	while (1) {
		if (ConvertF16_32 (fz) != z) {
			if ((z = ConvertF16_32 (fz)) < 0  ||  z >= WORLDSIZ)
				break;

			me = &levelmap[z][x];
			if (me->me_Obs  &&  checkshot (si, me))
				/*  Got him!!  */
				return;

			if (me->me_Flags & MEF_SHOTSOLID) {
				/*  Shot a wall obliquely.  */
				register frac16	delta;

				if (dz < 0) {
					z++;
					delta = ONE_F16 - F_FRAC (fz);
				} else
					delta = F_FRAC (fz);

				x = Convert32_F16 (x) + DivSF16 (delta, dz);

				si->si_Dist =
				 approx2dist (playerpos.X, playerpos.Z,
				 	      x, Convert32_F16 (z));
				si->si_me = me;
				return;
			}
		}

		if (--x < 0)
			/*  End of the world.  Stop.  */
			break;

		me = &levelmap[z][x];
		if (me->me_Obs  &&  checkshot (si, me))
			/*  Got him!!  */
			return;

		if (me->me_Flags & MEF_SHOTSOLID) {
			si->si_me = me;
			si->si_Dist = approx2dist (playerpos.X, playerpos.Z,
						   Convert32_F16 (x + 1), fz);
			return;
		}

		fz += dz;
	}
}


static void
shoot_south (si)
struct ShotInfo	*si;
{
	register MapEntry	*me;
	register int		x, z;
	register frac16		fx, dx;

	dx = DivSF16 (SinF16 (playerdir), CosF16 (playerdir));	// Tangent

	x = ConvertF16_32 (playerpos.X);
	z = ConvertF16_32 (playerpos.Z);

	me = &levelmap[z][x];
	if (me->me_Obs  &&  checkshot (si, me))
		/*  Got him!!  */
		return;

	fx = playerpos.X + MulSF16 (dx, F_FRAC (playerpos.Z));

	while (1) {
		if (ConvertF16_32 (fx) != x) {
			if ((x = ConvertF16_32 (fx)) < 0  ||  x >= WORLDSIZ)
				break;

			me = &levelmap[z][x];
			if (me->me_Obs  &&  checkshot (si, me))
				/*  Got him!!  */
				return;

			if (me->me_Flags & MEF_SHOTSOLID) {
				/*  Shot a wall obliquely.  */
				register frac16	delta;

				if (dx < 0) {
					x++;
					delta = ONE_F16 - F_FRAC (fx);
				} else
					delta = F_FRAC (fx);

				z = Convert32_F16 (z) + DivSF16 (delta, dx);

				si->si_Dist =
				 approx2dist (playerpos.X, playerpos.Z,
				 	      Convert32_F16 (x), z);
				si->si_me = me;
				return;
			}
		}

		if (--z < 0)
			/*  End of the world.  Stop.  */
			break;

		me = &levelmap[z][x];
		if (me->me_Obs  &&  checkshot (si, me))
			/*  Got him!!  */
			return;

		if (me->me_Flags & MEF_SHOTSOLID) {
			si->si_me = me;
			si->si_Dist = approx2dist (playerpos.X, playerpos.Z,
						   fx, Convert32_F16 (z + 1));
			return;
		}

		fx += dx;
	}
}


static void
shoot_east (si)
struct ShotInfo	*si;
{
	register MapEntry	*me;
	register int		x, z;
	register frac16		fz, dz;

	dz = -DivSF16 (CosF16 (playerdir), SinF16 (playerdir));	// -1/Tangent

	x = ConvertF16_32 (playerpos.X);
	z = ConvertF16_32 (playerpos.Z);

	me = &levelmap[z][x];
	if (me->me_Obs  &&  checkshot (si, me))
		/*  Got him!!  */
		return;

	fz = playerpos.Z + MulSF16 (dz, ONE_F16 - F_FRAC (playerpos.X));

	while (1) {
		if (ConvertF16_32 (fz) != z) {
			if ((z = ConvertF16_32 (fz)) < 0  ||  z >= WORLDSIZ)
				break;

			me = &levelmap[z][x];
			if (me->me_Obs  &&  checkshot (si, me))
				/*  Got him!!  */
				return;

			if (me->me_Flags & MEF_SHOTSOLID) {
				/*  Shot a wall obliquely.  */
				register frac16	delta;

				if (dz < 0) {
					z++;
					delta = F_FRAC (fz) - ONE_F16;
				} else
					delta = F_FRAC (fz);

				x = Convert32_F16 (x + 1) -
				    DivSF16 (delta, dz);

				si->si_Dist =
				 approx2dist (playerpos.X, playerpos.Z,
				 	      x, Convert32_F16 (z));
				si->si_me = me;
				return;
			}
		}

		if (++x >= WORLDSIZ)
			/*  End of the world.  Stop.  */
			break;

		me = &levelmap[z][x];
		if (me->me_Obs  &&  checkshot (si, me))
			/*  Got him!!  */
			return;

		if (me->me_Flags & MEF_SHOTSOLID) {
			si->si_me = me;
			si->si_Dist = approx2dist (playerpos.X, playerpos.Z,
						   Convert32_F16 (x), fz);
			return;
		}

		fz += dz;
	}
}


/***************************************************************************
 * Gun firing animation code.
 */
#define	QUARTER_F16	(ONE_F16 >> 2)
#define	BARGRAPH_X	13
#define	BARGRAPH_Y	25
#define	BARGRAPH_WIDE	22


extern CelArray	*ca_gun, *ca_ray;
extern CCB	*statpanel, *guncel;
extern uint32	ccbextra;
extern int32	cy;
extern int32	playerhealth;

static int32	gunbasey;



void
sequencegun (dist)
frac16 dist;
{
#define	NGUNYS		(sizeof (gunyoffs) / sizeof (int32))

	static int32	gunyoffs[] = {
		0, 1, 3, 5, 6, 5
	};
	static Point	corner[4];
	static int	gunidx;
	register int32	gcx;
	GrafCon		gc;
	Rect		rect;


	gcx = (playerhealth * (ca_gun->ncels - 1) + 50) / 100;
	gcx = ca_gun->ncels - 1 - gcx;
	guncel = ca_gun->celptrs[gcx];

	gcx = (guncel->ccb_XPos >> 16) + (REALCELDIM (guncel->ccb_Width) >> 1);

	if (dist) {
		register frac16	disp;

		gunidx = NGUNYS;
		guncel->ccb_Flags &= ~CCB_LAST;

		disp = ConvertF16_32 (DivSF16 (QUARTER_F16 * MAGIC, dist));

		corner[0].pt_X = gcx - disp;
		corner[0].pt_Y = cy + (disp >> 1);

		corner[1].pt_X = gcx + disp;
		corner[1].pt_Y = cy + (disp >> 1);
	}
	else if (gunpower) {
		register int32	x, y;

		x = guncel->ccb_XPos >> 16;
		y = gunbasey - gunyoffs[gunidx];

		rect.rect_XLeft	= x + BARGRAPH_X;
		rect.rect_YTop	= y + BARGRAPH_Y;
		rect.rect_XRight	= rect.rect_XLeft +
					  (gunpower * (BARGRAPH_WIDE - 1) +
					   50) / 100;
		rect.rect_YBottom	= rect.rect_YTop + 1;

		SetFGPen (&gc, MakeRGB15 (20, 5, 5));
		FillRect (rprend->rp_BitmapItem, &gc, &rect);
	}

	if (gunidx) {
		register CCB	*ray;

		gunidx--;
		ray = ca_ray->celptrs[rand () % ca_ray->ncels];
		guncel->ccb_NextPtr = ray;

		corner[2].pt_X = gcx + 20;
		corner[2].pt_Y = gunbasey - gunyoffs[gunidx];

		corner[3].pt_X = gcx - 20;
		corner[3].pt_Y = gunbasey - gunyoffs[gunidx];

		FasterMapCel (ray, corner);
	} else
		guncel->ccb_Flags |= CCB_LAST;

	guncel->ccb_YPos = (gunbasey - gunyoffs[gunidx]) << 16;
}

void
loadgun ()
{
	register CCB	*ccb;
	register int	i;

	if (!(ca_gun = parse3DO ("gun.cel")))
		die ("Couldn't load image.\n");

	if (!(ca_ray = parse3DO ("ray.cel")))
		die ("Couldn't load image.\n");

	for (i = ca_gun->ncels;  --i >= 0; ) {
		ccb = ca_gun->celptrs[i];

		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

		ccb->ccb_Flags |= CCB_LAST;	// Usually the last thing.
		ccb->ccb_XPos = ((wide - REALCELDIM (ccb->ccb_Width)) >> 1) << 16;
		ccb->ccb_YPos = (high - REALCELDIM (ccb->ccb_Height)) << 16;
	}

//	gunbasey = high - REALCELDIM (statpanel->ccb_Height) -
//		   REALCELDIM (guncel->ccb_Height) + 6;
	guncel = ca_gun->celptrs[0];
	gunbasey = high - REALCELDIM (guncel->ccb_Height) + 24;


	for (i = ca_ray->ncels;  --i >= 0; ) {
		ccb = ca_ray->celptrs[i];
		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW |
				  ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

		ccb->ccb_Flags |= CCB_LAST;	// Always the last thing.
	}
}
