/*  :ts=8 bk=0
 *
 * objects.c:	Object handling.
 *
 * Leo L. Schwab					9305.25
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "anim.h"
#include "imgfile.h"
#include "sound.h"

#include "app_proto.h"

#define RGB5(r,g,b) ((r<<10)|(g<<5)|b)

/***************************************************************************
 * Globals.
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];

extern Vertex	playerpos;
extern int32	playerhealth;

extern uint32	ccbextra;

extern Vertex	obverts[];
extern int32	nobverts;

extern CCB	*curccb;

extern uint32	linebuf[];
extern Object	**obtab;
extern int32	obtabsiz;


/***************************************************************************
 * Zombie
 */
#define	MINDIST_ZOMBIE	(7 * (ONE_F16>>3))
#define	STEPLEN_ZOMBIE	0x440	/* upped from 0x380  /* upped from 0x280 */
#define	ZOMBIE_SIZ	(3 * (ONE_F16>>3))
#define	ZOMBIEANIMSPEED	12
#define	ZOMBIEVERTS	2

#define	OBS_ZAPPED	OBS_DEAD+1

#define	ZOMBIEDEATHSND	100

typedef struct DefZombie {
	struct ObDef	od;
	struct CelArray	*dz_Images;
	struct CelArray	*dz_WakeUp,
			*dz_Walk,
			*dz_Attack,
			*dz_Death,
			*dz_Zapped;
} DefZombie;

typedef struct ObZombie {
	struct Object	ob;
	struct CelArray	*oz_CurSeq;
	struct MapEntry	*oz_ME;
	struct AnimDef	oz_AnimDef;
	Vertex		oz_Pos;
	frac16		oz_Dir;
	int32		oz_CurFrame;
	int32		oz_AtkDly;
	int32		oz_Hits;
} ObZombie;




static int32 zombie (struct ObZombie *, int, void *);
static void cycleframes (struct ObZombie *, int);


struct DefZombie	def_Zombie = {
 {	zombie,
	OTYP_ZOMBIE,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL, NULL
};


struct DefZombie	def_BossZombie = {
 {	zombie,
	OTYP_BOSSZOMBIE,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL, NULL
};




static int32
zombie (oz, op, dat)
register struct ObZombie	*oz;
int				op;
void				*dat;
{
	switch (op) {
	case OP_FIATLUX:
		loadzombie ();
		break;

	case OP_DESTRUCT:
		if (def_Zombie.od.od_ObCount > 0)
			/*  You haven't deleted them all...  */
			return (def_Zombie.od.od_ObCount);
		else
			destructzombie ();
		break;

	case OP_CREATEOB:
		if (oz == (struct ObZombie *)&def_Zombie)
			return ((int32) createStdObject ((ObDef *) &def_Zombie,
						 OTYP_ZOMBIE,
						 sizeof (ObZombie),
						 0,
						 dat));
		else
			return ((int32) createStdObject ((ObDef *) &def_BossZombie,
						 OTYP_BOSSZOMBIE,
						 sizeof (ObZombie),
						 0,
						 dat));

	case OP_DELETEOB:
		deleteStdObject ((Object *) oz);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;

		id = dat;
		oz->ob.ob_State	= OBS_ASLEEP;
		oz->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER |
				  OBF_SHOOT | OBF_CONTACT;
		oz->oz_CurSeq	= def_Zombie.dz_WakeUp;
		oz->oz_Dir	=
		oz->oz_CurFrame	= 0;
		oz->oz_ME	= id->id_MapEntry;
		oz->oz_Pos.X	= Convert32_F16 (id->id_XIdx) + HALF_F16;
		oz->oz_Pos.Z	= Convert32_F16 (id->id_ZIdx) + HALF_F16;
		oz->oz_AnimDef.ad_FPS = ZOMBIEANIMSPEED; //12;
		oz->oz_AnimDef.ad_Counter = VBLANKFREQ;
		oz->oz_Hits	= 1;
		if (oz->ob.ob_Type == OTYP_BOSSZOMBIE)
			oz->oz_Hits	= 12;
		break;
	 }

	case OP_MOVE:
	 {
		register frac16	dist, dir;
		register int	x, z;
		MapEntry	*me;
		Vector		step;
		int		nframes;

		nframes = (int) dat;
		cycleframes (oz, nframes);

		if ((x = oz->ob.ob_State) != OBS_WALKING  &&
		    x != OBS_CHASING)
			/*  Nuthin' more to do.  */
			return (0);

		step.X = playerpos.X - oz->oz_Pos.X;
		step.Y = 0;
		step.Z = playerpos.Z - oz->oz_Pos.Z;
		if (x == OBS_CHASING) {
			/*
			 * Retarget zombie at player.
			 */
			oz->oz_Dir = Atan2F16 (step.X, step.Z);
		}
		dist = approx2dist (step.X, step.Z, 0, 0);

		if (dist <= MINDIST_ZOMBIE) {
			if (!oz->oz_CurFrame) oz->oz_CurSeq = def_Zombie.dz_Attack;
			if ((oz->oz_AtkDly -= nframes) < 0) {
				/*
				 * Zombie takes from 1 - 15 points off.
				 */
				takedamage ((rand () % 15) + 1);
				oz->oz_AtkDly = VBLANKFREQ +
						rand () % VBLANKFREQ;
			}
			return (0);
		}
		else {
			if (!oz->oz_CurFrame) oz-> oz_CurSeq = def_Zombie.dz_Walk;
		}

		dir = oz->oz_Dir;
		step.X = MulSF16 (CosF16 (dir), STEPLEN_ZOMBIE * nframes);
		step.Z = MulSF16 (SinF16 (dir), STEPLEN_ZOMBIE * nframes);

		moveposition (&oz->oz_Pos, &step, (Object *) oz, TRUE, FALSE);

		x = ConvertF16_32 (oz->oz_Pos.X);
		z = ConvertF16_32 (oz->oz_Pos.Z);
		if ((me = &levelmap[z][x]) != oz->oz_ME) {
			removeobfromme ((Object *) oz, oz->oz_ME);
			addobtome ((Object *) oz, me);
			oz->oz_ME = me;
		}
		break;
	 }
	case OP_REGISTER:
	 {
	 	register Vertex	*zv;

		if (nobverts + ZOMBIEVERTS > NOBVERTS)
			break;

		oz->ob.ob_VertIdx = nobverts;
		zv = obverts + nobverts;

		zv->X = oz->oz_Pos.X;
		zv->Y = oz->oz_Pos.Y +0x400;
		zv->Z = oz->oz_Pos.Z;
		zv++;

		zv->X = oz->oz_Pos.X;
		zv->Y = oz->oz_Pos.Y - ONE_F16 +0x400;
		zv->Z = oz->oz_Pos.Z;

		nobverts += ZOMBIEVERTS;

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
		0x7fff3dd1,0x294f1cec,
		0x14ad108f,0x086a0006,
		0x0c6a252f,0x1cef1cd0,
		0x49b55217,0x629b7758,
		0x67185a93,0x4a2c41e9,
		0x39a42923,0x35a52e05,
		0x24c72928,0x294b5272,
		0xfa61ff41,0x10424cdf,
	};

		vp = obverts + oz->ob.ob_VertIdx;
		if (vp->Z < (ZCLIP >> 8))
			return (0);

		srcccb = oz->oz_CurSeq->celptrs[oz->oz_CurFrame];

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

		if (oz->oz_Hits > 1) {
			ccb->ccb_PLUTPtr = TestPLUT;
		}

		ccb->ccb_PIXC		= srcccb->ccb_PIXC;
		ccb->ccb_PRE0		= srcccb->ccb_PRE0;
		ccb->ccb_PRE1		= srcccb->ccb_PRE1;
		ccb->ccb_Width		= srcccb->ccb_Width;
		ccb->ccb_Height		= srcccb->ccb_Height;

		FasterMapCel (ccb, corner);
		ccb->ccb_Flags &= ~CCB_LAST;

		/*
		 * Disgusting hack for shot targeting.
		 */
		fudge = (corner[1].pt_X-corner[0].pt_X)>>2;
		vp[0].X = corner[0].pt_X+fudge;
		vp[0].Y = corner[1].pt_X-fudge;

		/*
		 * We saw him, so he sees us.
		 */
		if (oz->ob.ob_State == OBS_ASLEEP)
			oz->ob.ob_State = OBS_AWAKENING;
		else if (oz->ob.ob_State == OBS_WALKING)
			oz->ob.ob_State = OBS_CHASING;

//// Comment out these four lines to leave residual zombie
//		else if (oz->ob.ob_State == OBS_DEAD) {
//			removeobfromme ((Object *) oz, oz->oz_ME);
//			oz->ob.ob_State = OBS_INVALID;	// Remove from list.
//		}


		break;
	 }
	case OP_SHOT:
	 {
		register Vertex	*ov;

		ov = obverts + oz->ob.ob_VertIdx;
		if (ov->X <= CX  &&  ov->Y >= CX) {
			if (oz->oz_Hits) {
				oz->oz_CurFrame	= 0;
				if (!(--(oz->oz_Hits))) {
					oz->ob.ob_State	= OBS_DYING;
					oz->oz_CurSeq	= def_Zombie.dz_Death;
					playsound (ZOMBIEDEATHSND);
					return (obverts[oz->ob.ob_VertIdx].Z << 8);
				}
				else {
					oz->ob.ob_State	= OBS_ZAPPED;
					oz->oz_CurSeq	= def_Zombie.dz_Zapped;
					return (obverts[oz->ob.ob_VertIdx].Z << 8); //????
				}
			}
		}
		break;
	 }
	case OP_CONTACT:
	 {
		register PathBox	*pb;
		BBox			zb;

		pb = dat;

		zb.MinX = oz->oz_Pos.X - ZOMBIE_SIZ;
		zb.MinZ = oz->oz_Pos.Z - ZOMBIE_SIZ;
		zb.MaxX = oz->oz_Pos.X + ZOMBIE_SIZ;
		zb.MaxZ = oz->oz_Pos.Z + ZOMBIE_SIZ;


		if (oz->ob.ob_State == OBS_DEAD)
			return(0);
		else
			return (checkcontact (pb, &zb, TRUE));
	 }
	}	// switch()
	return (0);
}

static void
cycleframes (oz, nvbls)
register struct ObZombie	*oz;
int				nvbls;
{
	register CelArray	*ca;
	register int		i;

	ca = oz->oz_CurSeq;

//	i = nextanimframe (&oz->oz_AnimDef, oz->oz_CurFrame, nvbls);
	i = oz->oz_CurFrame+1;
	switch (oz->ob.ob_State) {
	case OBS_AWAKENING:
		if (i >= ca->ncels) {
			oz->ob.ob_State = OBS_WALKING;
			oz->oz_CurFrame = 0;
			oz->oz_CurSeq = def_Zombie.dz_Walk;
		} else
			oz->oz_CurFrame = i;

		break;

	case OBS_WALKING:
	case OBS_CHASING:
		if (i >= ca->ncels)
			i -= ca->ncels;
		oz->oz_CurFrame = i;

		break;

	case OBS_ZAPPED:
		if (i >= ca->ncels) {
			oz->ob.ob_State = OBS_CHASING;
			oz->oz_CurSeq =	def_Zombie.dz_Walk;
			i = 0;
		}
		oz->oz_CurFrame = i;
		break;

	case OBS_DYING:
		if (i >= ca->ncels)
			oz->ob.ob_State = OBS_DEAD;
		else
			oz->oz_CurFrame = i;
		break;

	case OBS_ASLEEP:
	case OBS_DEAD:
		break;
	}
}





void
loadzombie ()
{
	register CCB	*ccb;
	register int	i;

	int	wakeupanim[] = {0,1,0,1,-1}; // why bother?
	int	walkanim[] = {0,1,2,3,4,5,6,7,8,9,10,-1};
	int	attackanim[] = {11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,-1};
	int	zappedanim[] = {30,31,31,30,30,31,31,-1};
	int	deathanim[] = {30,31,32,33,32,33,32,33,34,35,36,37,38,39,40,41,40,
				41,39,40,41,39,40,41,40,39,41,39,40,41,40,42,-1};


	/*
	 * Load and initialize cels.  (Will get converted to LOAF.)
	 */
	if (!(def_Zombie.dz_Images = parse3DO ("$progdir/zombie.cels")))
		die ("Couldn't load zombie.\n");
 printf("*** zombie ncels = %d\n",def_Zombie.dz_Images->ncels);
	for (i = def_Zombie.dz_Images->ncels;  --i >= 0; ) {
		ccb = def_Zombie.dz_Images->celptrs[i];
		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);
	}

	/*
	 * Set up animation sequencing descriptors.
	 */
	setupanimarray(&def_Zombie.dz_WakeUp,wakeupanim,def_Zombie.dz_Images->celptrs);
	setupanimarray(&def_Zombie.dz_Walk,walkanim,def_Zombie.dz_Images->celptrs);
	setupanimarray(&def_Zombie.dz_Attack,attackanim,def_Zombie.dz_Images->celptrs);
	setupanimarray(&def_Zombie.dz_Death,deathanim,def_Zombie.dz_Images->celptrs);
	setupanimarray(&def_Zombie.dz_Zapped,zappedanim,def_Zombie.dz_Images->celptrs);

	loadsound ("ZombieDeathShort22k.aifc",ZOMBIEDEATHSND);

}


void
destructzombie ()
{
	if (def_Zombie.dz_Death) {
		freecelarray (def_Zombie.dz_Death);
		def_Zombie.dz_Death = NULL;
	}
	if (def_Zombie.dz_Walk) {
		freecelarray (def_Zombie.dz_Walk);
		def_Zombie.dz_Walk = NULL;
	}
	if (def_Zombie.dz_Attack) {
		freecelarray (def_Zombie.dz_Attack);
		def_Zombie.dz_Attack = NULL;
	}
	if (def_Zombie.dz_WakeUp) {
		freecelarray (def_Zombie.dz_WakeUp);
		def_Zombie.dz_WakeUp = NULL;
	}
	if (def_Zombie.dz_Zapped) {
		freecelarray (def_Zombie.dz_Zapped);
		def_Zombie.dz_Zapped = NULL;
	}
	if (def_Zombie.dz_Images) {
		freecelarray (def_Zombie.dz_Images);
		def_Zombie.dz_Images = NULL;
	}
	unloadsound (ZOMBIEDEATHSND);
}
