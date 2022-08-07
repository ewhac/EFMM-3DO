/*  :ts=8 bk=0
 *
 * ob_george.c:	George the ghost object.
 *
 * Jon Leupp						9307.14
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "anim.h"
#include "imgfile.h"

#include "app_proto.h"

#define RGB5(r,g,b) ((r<<10)|(g<<5)|b)

/***************************************************************************
 * Globals.
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];

extern Vertex	playerpos;
extern int32	playerhealth;


/***************************************************************************
 * Structure definitions for this object.
 */
#define	MINDIST_GEORGE		(ONE_F16) // << 1)
#define	STEPLEN_GEORGE		((ONE_F16 + HALF_F16) >> 4)
#define	CIRCLELEN_GEORGE	((ONE_F16) >> 5)
#define	GEORGEVERTS		2

#define	GEORGEDEATHSND		104


// put here for now to keep this all together
#define OBS_CIRCLE_LEFT		(OBS_DEAD + 1)
#define	OBS_CIRCLE_RIGHT	(OBS_CIRCLE_LEFT + 1)


typedef struct DefGeorge {
	struct ObDef	od;
	struct CelArray	*dm_Images;
	struct CelArray	*dm_Spin,
			*dm_Spot,
			*dm_Approach,
			*dm_Stab,
			*dm_Death;
} DefGeorge;

typedef struct ObGeorge {
	struct Object	ob;
	struct CelArray	*om_CurSeq;
	struct MapEntry	*om_ME;
	struct AnimDef	om_AnimDef;
	Vertex		om_Pos;
	frac16		om_Dir;
	int32		om_CurFrame;
	int32		om_AtkDly;
	int32		om_Hits;
} ObGeorge;

/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 george (struct ObGeorge *, int, void *);
static void animgeorge(struct ObGeorge *om,int nvbls);


/***************************************************************************
 * Object definition instance.
 */
struct DefGeorge		def_George = {
 {	george,
	OTYP_GEORGE,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL, NULL, NULL, NULL
};

struct DefGeorge		def_BossGeorge = {
 {	george,
	OTYP_GEORGE,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL, NULL, NULL, NULL
};

/***************************************************************************
 * External globals.
 */
extern uint32	ccbextra;

extern Vertex	obverts[];
extern int32	nobverts;

extern CCB	*curccb;

extern uint32	linebuf[];


/***************************************************************************
 * George
 */

static int32
george (om, op, dat)
register struct ObGeorge	*om;
int				op;
void				*dat;
{
/*
static ulong	pixctable[] = {
	0x9f009f00,	//   0%
	0x9f009f00,	//   0%
	0x9ae19ae1,	//  12.5%
	0x9ae19ae1,	//  12.5%
	0x97e097e0,	//  25%
	0x97e097e0,	//  25%
	0x97e097e0,	//  25%
	0x1f811f81,	//  50%
	0x1f811f81,	//  50%
	0x1f811f81,	//  50%
	0x17a017a0,	//  75%
	0x17a017a0,	//  75%
	0x17a017a0,	//  75%
	0x1aa11aa1,	//  87.5%
	0x1aa11aa1,	//  87.5%
	0x1aa11aa1,	//  87.5%
	0x1f001f00,	// 100%
	0x1f001f00,	// 100%
	0x1f001f00,	// 100%
};
*/

Vertex oldpos;

	switch (op) {
	case OP_FIATLUX:
		loadgeorge ();
		break;

	case OP_DESTRUCT:
		if (def_George.od.od_ObCount > 0)
			/*  You haven't deleted them all...  */
			return (def_George.od.od_ObCount);
		else
			destructgeorge ();
		break;

	case OP_CREATEOB:
		if (om == (struct ObGeorge *) &def_George)
			return ((int32) createStdObject ((ObDef *) &def_George,
						 OTYP_GEORGE,
						 sizeof (ObGeorge),
						 0,
						 dat));
		else
			return ((int32) createStdObject ((ObDef *) &def_BossGeorge,
						 OTYP_BOSSGEORGE,
						 sizeof (ObGeorge),
						 0,
						 dat));

	case OP_DELETEOB:
// printf("* Delete in *\n");
		deleteStdObject ((Object *) om);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;

		id = dat;
		om->ob.ob_State	= OBS_ASLEEP;
		om->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER |
				  OBF_SHOOT;
		om->om_CurSeq	= def_George.dm_Spin;
		om->om_Dir	=
		om->om_CurFrame	= 0;
		om->om_ME	= id->id_MapEntry;
		om->om_Pos.X	= Convert32_F16 (id->id_XIdx) + HALF_F16;
		om->om_Pos.Z	= Convert32_F16 (id->id_ZIdx) + HALF_F16;
		om->om_AnimDef.ad_FPS = 20;
		om->om_AnimDef.ad_Counter = VBLANKFREQ;
		om->om_Hits	= 1;
		if (om->ob.ob_Type == OTYP_BOSSGEORGE)
			om->om_Hits	= 12;
		break;
	 }

	case OP_MOVE:
	 {
		register frac16	dist, dir;
		register int	x, z;
		MapEntry	*me;
		Vector		step;
		int		nframes,stepsize;

		nframes = (int) dat;
		animgeorge (om,nframes);

		if (((x = om->ob.ob_State) != OBS_CHASING)  &&
		    (x != OBS_ATTACKING)  &&
		    (x != OBS_CIRCLE_RIGHT)  &&
		    (x != OBS_CIRCLE_LEFT))
			/*  Nuthin' to do.  */
			return (0);

		step.X = playerpos.X - om->om_Pos.X;
		step.Y = 0;
		step.Z = playerpos.Z - om->om_Pos.Z;
		om->om_Dir = Atan2F16 (step.X, step.Z);
		dist = approx2dist (step.X, step.Z, 0, 0);
		dir = om->om_Dir;

		if (dist < MINDIST_GEORGE) {
			dir ^= 0x800000;
			if (x == OBS_CHASING) {
				if (rand()&1) x = OBS_CIRCLE_RIGHT;
				else x = OBS_CIRCLE_LEFT;
			}
 printf("**** om->om_AtkDly = %lx\n",om->om_AtkDly);

			if ((om->om_AtkDly -= nframes) < 0) {
				/*
				 * George takes from 8 - 15 points off.
				 */
				takedamage ((rand () & 7) + 8);
				om->om_AtkDly = (rand () & 0xf) + 10;
				om->om_CurFrame	= 0;
				om->om_CurSeq	= def_George.dm_Stab;
			}
		}
		else {
			if (dist > MINDIST_GEORGE + (STEPLEN_GEORGE<<1)) x = OBS_CHASING;

			if (x == OBS_CIRCLE_LEFT) {
				if (!(rand()&0x1f)) x = OBS_CIRCLE_RIGHT;
				else dir = (dir - 0x400000)& 0xffffff; // turn 90 degrees
			}
			else if	(x == OBS_CIRCLE_RIGHT) {
				if (!(rand()&0x1f)) x = OBS_CIRCLE_LEFT;
				else dir = (dir + 0x400000)& 0xffffff;
			}
		}
		om->ob.ob_State = x;

////		if ((dist < MINDIST_GEORGE) || (dist > MINDIST_GEORGE + STEPLEN_GEORGE)) {
			if ((x == OBS_CIRCLE_RIGHT) || (x == OBS_CIRCLE_LEFT)) stepsize = CIRCLELEN_GEORGE;
			else stepsize = STEPLEN_GEORGE;
			step.X = MulSF16 (CosF16 (dir), stepsize);
			step.Z = MulSF16 (SinF16 (dir), stepsize);

////		}
		if (moveposition
		     (&om->om_Pos, &step, (Object *) om, TRUE, FALSE))
		{
			if (x == OBS_CIRCLE_LEFT) x = OBS_CIRCLE_RIGHT;
			else if (x == OBS_CIRCLE_RIGHT) x = OBS_CIRCLE_LEFT;
		}

		x = ConvertF16_32 (om->om_Pos.X);
		z = ConvertF16_32 (om->om_Pos.Z);
		if ((me = &levelmap[z][x]) != om->om_ME) {
			removeobfromme ((Object *) om, om->om_ME);
			addobtome ((Object *) om, me);
			om->om_ME = me;
		}
		break;
	 }
	case OP_REGISTER:
	 {
	 	register Vertex	*zv;

		if (nobverts + GEORGEVERTS > NOBVERTS)
			break;

		om->ob.ob_VertIdx = nobverts;
		zv = obverts + nobverts;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y + (ONE_F16>>3);
		zv->Z = om->om_Pos.Z;
		zv++;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y - (ONE_F16+HALF_F16)+(ONE_F16>>2);
		zv->Z = om->om_Pos.Z;

		nobverts += GEORGEVERTS;

		break;
	 }
	case OP_RENDER:
	 {
		register Vertex	*vp;
		register frac16	x, y;
		register CCB	*ccb, *srcccb;
		Point		corner[4];

	int32	fudge;

	static int32 TestPLUT[] =
	{
/*
	WARNING!  If ever there's a future system where there's some RAM
	that is not MEMTYPE_CEL, then THIS WILL NOT WORK!  (No time to
	fix it now.  9310.21)
*/
		0x7fff6fbb,0x3e6f29ea,
		0x3a2d360e,0x3e4f3ecf,
		0x47363e76,0x46d932b4,
		0x5f3d5b35,0x18ea254f,
		0x3a153a75,0x46785efb,
		0x52d95efa,0x1d0d41ec,
		0x2d0b45f0,0x18a21ca6,
		0x4a571023,0x35b10000,
	};

		vp = obverts + om->ob.ob_VertIdx;
		if (vp->Z < (ZCLIP >> 8))
			return (0);

		srcccb = om->om_CurSeq->celptrs[om->om_CurFrame];

		x = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Width));
		y = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Height));

		x = ConvertF16_32 ((vp[0].Y - vp[1].Y) * x);
		y = ConvertF16_32 ((vp[0].Y - vp[1].Y) * y);

		corner[0].pt_X = vp[0].X - (x >> 1);
		corner[0].pt_Y = vp[0].Y - y;

		corner[1].pt_X = corner[0].pt_X + x;
		corner[1].pt_Y = vp[0].Y - y;

		corner[2].pt_X = corner[1].pt_X;
		corner[2].pt_Y = vp[0].Y;

		corner[3].pt_X = corner[0].pt_X;
		corner[3].pt_Y = vp[0].Y;

		if (!testlinebuf (linebuf, corner[0].pt_X, corner[1].pt_X))
			break;

		ccb = curccb++;
		ccb->ccb_Flags		= srcccb->ccb_Flags;
		ccb->ccb_SourcePtr	= srcccb->ccb_SourcePtr;
		ccb->ccb_PLUTPtr	= srcccb->ccb_PLUTPtr;

		if (om->om_Hits > 1) {
			ccb->ccb_PLUTPtr = TestPLUT;
		}

		ccb->ccb_PIXC		= srcccb->ccb_PIXC;
		ccb->ccb_PRE0		= srcccb->ccb_PRE0;
		ccb->ccb_PRE1		= srcccb->ccb_PRE1;
		ccb->ccb_Width		= srcccb->ccb_Width;
		ccb->ccb_Height		= srcccb->ccb_Height;

//		if (om->om_CurSeq == def_George.dm_Stab)
//			ccb->ccb_PIXC = pixctable[om->om_CurFrame];
//			ccb->ccb_PIXC = 0x1f811f81;	//  50%

		FasterMapCel (ccb, corner);
		ccb->ccb_Flags &= ~CCB_LAST;

		/*
		 * Disgusting hack for shot targeting.
		 */
		fudge = (corner[1].pt_X-corner[0].pt_X)>>2;
// printf("corner[0].pt_X = 0x%lx, corner[1].pt_X = 0x%lx, fudge = 0x%lx\n",
// 	 corner[0].pt_X, corner[1].pt_X, fudge);

		vp[0].X = corner[0].pt_X+fudge;
		vp[0].Y = corner[1].pt_X-fudge;

		/*
		 * We saw him, so he sees us.
		 */
		if (om->ob.ob_State == OBS_ASLEEP)
			om->ob.ob_State = OBS_AWAKENING;
		else if (om->ob.ob_State == OBS_DEAD) {
			om->ob.ob_State = OBS_INVALID;	// Remove from list.
			removeobfromme ((Object *) om, om->om_ME);
		}
		break;
	 }
	case OP_SHOT:
	 {
		register Vertex	*ov;

		ov = obverts + om->ob.ob_VertIdx;
		if (ov->X <= CX  &&  ov->Y >= CX) {
			if (om->om_Hits) {
				om->om_CurFrame	= 0;
				if (!(--(om->om_Hits))) {
					om->ob.ob_State	= OBS_DYING;
					om->om_CurSeq	= def_George.dm_Death;
					playsound (GEORGEDEATHSND);
					return (obverts[om->ob.ob_VertIdx].Z << 8);
				}
				else {
					om->ob.ob_State	= OBS_CIRCLE_LEFT;
					om->om_CurSeq	= def_George.dm_Spin;
					return (obverts[om->ob.ob_VertIdx].Z << 8);

				}
			}
		}
		break;
	 }
	}	// switch()
	return (0);
}



void
loadgeorge ()
{
	register CCB	*ccb;
	register int	i;

	int	spinanim[] = {14,15,0,7,8,9,10,11,12,13,-1};
	int	spotanim[] = {13,12,11,10,9,8,7,0,-1};
	int	approachanim[] = {1,2,3,4,5,6,5,4,3,2,-1};
	int	stabanim[] = {16,17,18,19,20,21,22,23,24,25,26,27,28,
				29,30,31,32,33,34,-1};
	int	deathanim[] = {35,35,36,36,37,37,38,38,39,39,40,40,41,41,42,42,
				43,42,43,44,45,45,46,46,47,47,48,48,49,
				49,50,50,-1};

	/*
	 * Load and initialize cels.  (Will get converted to LOAF.)
	 */
	if (!(def_George.dm_Images = parse3DO ("$progdir/george.cels")))
		die ("Couldn't load george.\n");

 printf("*** ghost ncels = %d\n",def_George.dm_Images->ncels);

	for (i = def_George.dm_Images->ncels;  --i >= 0; ) {
		ccb = def_George.dm_Images->celptrs[i];
		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);
	}

	/*
	 * Set up animation sequencing descriptors.
	 */
	setupanimarray(&def_George.dm_Spin,spinanim,def_George.dm_Images->celptrs);
 	setupanimarray(&def_George.dm_Spot,spotanim,def_George.dm_Images->celptrs);
	setupanimarray(&def_George.dm_Approach,approachanim,def_George.dm_Images->celptrs);
	setupanimarray(&def_George.dm_Stab,stabanim,def_George.dm_Images->celptrs);
	setupanimarray(&def_George.dm_Death,deathanim,def_George.dm_Images->celptrs);

//	loadsound ("MonsterAppears2.aifc",GEORGEDEATHSND);
	loadsound ("GhostCryAll.aifc",GEORGEDEATHSND);

}

void
destructgeorge ()
{
	if (def_George.dm_Death) {
		freecelarray (def_George.dm_Death);
		def_George.dm_Death = NULL;
	}
	if (def_George.dm_Spin) {
		freecelarray (def_George.dm_Spin);
		def_George.dm_Spin = NULL;
	}
	if (def_George.dm_Spot) {
		freecelarray (def_George.dm_Spot);
		def_George.dm_Spot = NULL;
	}
	if (def_George.dm_Stab) {
		freecelarray (def_George.dm_Stab);
		def_George.dm_Stab = NULL;
	}
	if (def_George.dm_Approach) {
		freecelarray (def_George.dm_Approach);
		def_George.dm_Approach = NULL;
	}
	if (def_George.dm_Images) {
		freecelarray (def_George.dm_Images);
		def_George.dm_Images = NULL;
	}

	unloadsound (GEORGEDEATHSND);
}


static void
animgeorge (om, nvbls)
register struct ObGeorge	*om;
int				nvbls;
{
	register CelArray	*ca;
	register int		i;
	int rnd;

	ca = om->om_CurSeq;
//	i = om->om_CurFrame;
	i = nextanimframe (&om->om_AnimDef, om->om_CurFrame, nvbls);

	switch (om->ob.ob_State) {
	case OBS_AWAKENING:
		if (i >= ca->ncels) {
			i = 0;
			om->ob.ob_State	= OBS_CHASING;
			om->om_CurSeq	= def_George.dm_Approach;
		}
		om->om_CurFrame = i;

		break;

	case OBS_CHASING:
		if (i >= ca->ncels) {
			i = 0;
//			om->ob.ob_State	= OBS_ATTACKING;
			rnd = (rand() & 0xff);
			if (rnd<64)
				om->om_CurSeq	= def_George.dm_Spot;
			if (rnd<90)
				om->om_CurSeq	= def_George.dm_Spin;
			else
				om->om_CurSeq	= def_George.dm_Approach;

		}
		om->om_CurFrame = i;

		break;

	case OBS_ATTACKING:
	case OBS_CIRCLE_LEFT:
	case OBS_CIRCLE_RIGHT:

		if (i >= ca->ncels) {
			i = 0;
		}
		om->om_CurFrame = i;

		break;

	case OBS_DYING:
		if (i >= ca->ncels)
			om->ob.ob_State = OBS_DEAD;
		else
			om->om_CurFrame = i;

		break;

	case OBS_ASLEEP:
// check for proximity then switch to spot
/*		if (i >= ca->ncels) {
			i = 0;
			om->ob.ob_State	= OBS_AWAKENING;
			om->om_CurSeq	= def_George.dm_Spot;
		}
		om->om_CurFrame = i;
*/
		break;
	case OBS_DEAD:
		break;
	}
}
