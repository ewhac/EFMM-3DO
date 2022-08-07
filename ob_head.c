/*  :ts=8 bk=0
 *
 * ob_head.c:
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


/***************************************************************************
 * Globals.
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];

extern Vertex	playerpos;
extern int32	playerhealth;


/***************************************************************************
 * Structure definitions for this object.
 */
#define	MINDIST_HEAD		(ONE_F16 << 1)
#define	MINDIST_HEADSHOT	(ONE_F16 >> 3) // was >> 1
#define	STEPLEN_HEAD		((ONE_F16) >> 4)
#define	CIRCLELEN_HEAD		((ONE_F16) >> 5)
#define	SHOTLEN_HEAD		(0x4000)
#define	HEADANIMSPEED		10
#define	HEADATTACKDELAY		1200
#define	ANTICDELAY		800
#define	SNARLDELAY		400
#define	HEADVERTS		2

#define HEADDEATHSND	112

// put here for now to keep this all together
#define OBS_CIRCLE_LEFT		(OBS_DEAD + 1)
#define	OBS_CIRCLE_RIGHT	(OBS_DEAD + 2)
#define	OBS_SHOT		(OBS_DEAD + 3)
#define	OBS_SHOTDYING		(OBS_DEAD + 4)
#define	OBS_SPIT		(OBS_DEAD + 5)


typedef struct DefHead {
	struct ObDef	od;
	struct CelArray	*dm_Images;
	struct CelArray	*dm_Wait,
			*dm_Anticipate,
			*dm_Snarl,
			*dm_Transform,
			*dm_BadHead,
			*dm_Spit,
			*dm_Recharge,
			*dm_Sploogie,
			*dm_Death,
			*dm_SploogieDeath;
} DefHead;

typedef struct ObHead {
	struct Object	ob;
	struct CelArray	*om_CurSeq;
	struct MapEntry	*om_ME;
	struct AnimDef	om_AnimDef;
	Vertex		om_Pos;
	frac16		om_Dir;
	int32		om_CurFrame;
	int32		om_AtkDly;
	int32		om_Hits;
} ObHead;


/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 head (struct ObHead *, int, void *);
static void animhead(struct ObHead *om,int nvbls);


/***************************************************************************
 * Object definition instance.
 */
struct DefHead		def_Head = {
 {	head,
	OTYP_HEAD,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL, NULL, NULL, NULL
};

struct DefHead		def_BossHead = {
 {	head,
	OTYP_BOSSHEAD,
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
extern Object	**obtab;
extern int32	obtabsiz;


/***************************************************************************
 * Head
 */

static int32
head (om, op, dat)
register struct ObHead	*om;
int				op;
void				*dat;
{
	switch (op) {
	case OP_FIATLUX:
		loadhead ();
		break;

	case OP_DESTRUCT:
		if (def_Head.od.od_ObCount > 0)
			/*  You haven't deleted them all...  */
			return (def_Head.od.od_ObCount);
		else
			destructhead ();
		break;

	case OP_CREATEOB:
		if (om == (struct ObHead *)&def_Head)
			return ((int32) createStdObject ((ObDef *) &def_Head,
						 OTYP_HEAD,
						 sizeof (ObHead),
						 0,
						 dat));
		else
			return ((int32) createStdObject ((ObDef *) &def_BossHead,
						 OTYP_BOSSHEAD,
						 sizeof (ObHead),
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
		om->om_CurSeq	= def_Head.dm_Wait;
		om->om_Dir	=
		om->om_CurFrame	= 0;
		om->om_ME	= id->id_MapEntry;
		om->om_Pos.X	= Convert32_F16 (id->id_XIdx) + HALF_F16;
		om->om_Pos.Z	= Convert32_F16 (id->id_ZIdx) + HALF_F16;
		om->om_AnimDef.ad_FPS = HEADANIMSPEED;
		om->om_AnimDef.ad_Counter = VBLANKFREQ;
		om->om_Hits	= 1;
		if (om->ob.ob_Type == OTYP_BOSSHEAD)
			om->om_Hits	= 12;
		om->om_AtkDly	= HEADATTACKDELAY;
		break;
	 }
	case OP_MOVE:
	 {
		register frac16	dist, dir;
		register int	x, z;
		MapEntry	*me;
		Vector		step;
		int		nframes,stepsize;
		int		i; //headshot
		register Object	**obt; // for shot
		register ObDef	*od;
		ObHead		*oh;

		nframes = (int) dat;
		om->om_AtkDly-=nframes;
		if (om->om_AtkDly<0) om->om_AtkDly = 0;
		animhead (om,nframes);

		if (om->ob.ob_State == OBS_DEAD) {
			om->ob.ob_State = OBS_INVALID;	// Mark inactive.
			removeobfromme ((Object *) om, om->om_ME);
			break;
		}

		if (((x = om->ob.ob_State) != OBS_CHASING) &&
		    (x != OBS_SHOT) &&
		    (x != OBS_SPIT) &&
		    (x != OBS_ATTACKING) &&
		    (x != OBS_CIRCLE_RIGHT) &&
		    (x != OBS_CIRCLE_LEFT))
			/*  Nuthin' to do.  */
			return (0);

		step.X = playerpos.X - om->om_Pos.X;
		step.Y = 0;
		step.Z = playerpos.Z - om->om_Pos.Z;
		dist = approx2dist (step.X, step.Z, 0, 0);
		dir = om->om_Dir;



// generate shot
		if ((om->om_CurSeq == def_Head.dm_Spit) &&
		    (om->om_CurFrame > 2))
		     {
//printf ("Trying to create head shot. obtabsiz = %d: ", obtabsiz);
			for (obt = obtab, i = 0;  i < obtabsiz;  obt++, i++) {
				oh = (ObHead *) *obt;
				if (!oh)
					break;
				if (((oh->ob.ob_Type == OTYP_HEAD)||(oh->ob.ob_Type == OTYP_BOSSHEAD))  &&
				    oh->ob.ob_State == OBS_INVALID)
					break;
			}

			if (i < obtabsiz) { // found empty object slot (or dead head)
				if (oh) {
					od = oh->ob.ob_Def;
					if ((od->od_Func)
					     (oh, OP_DELETEOB, NULL) < 0)
						die ("Failed to delete object.\n");
				}

				if ((!(oh = (ObHead *) (head (NULL,OP_CREATEOB,NULL)))))
					die ("Can't create object.\n");
				me = om->om_ME;
				oh->ob.ob_State	= OBS_SHOT;
				oh->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER |
						  OBF_SHOOT;
				oh->om_CurSeq	= def_Head.dm_Sploogie;
				oh->om_Dir = om->om_Dir = Atan2F16 (step.X, step.Z);
				oh->om_CurFrame = 0;
				oh->om_ME	= me;
				oh->om_Pos.X	= om->om_Pos.X;
				oh->om_Pos.Z	= om->om_Pos.Z;
				oh->om_AnimDef.ad_FPS = om->om_AnimDef.ad_FPS;
				oh->om_AnimDef.ad_Counter = om->om_AnimDef.ad_Counter;
				addobtome ((Object *) oh, me);

				*obt = (Object *) oh;
			}
		}
// end shot generation code

		if (x == OBS_SHOT) { // shot
//	 printf(">>>Head shot dist = 0x%lx\n",dist);
			if (dist < MINDIST_HEADSHOT) {
				/*
				 * Head takes from 8 - 15 points off.
				 */
//	 printf("Head shot should be doing damage here\n");
	 			takedamage ((rand () & 7) + 8);
				x = OBS_DYING;
				om->om_CurFrame	= 0;
				om->om_CurSeq	= def_Head.dm_SploogieDeath;

			}
		}
		else if (x != OBS_SPIT) {
			dir = om->om_Dir = Atan2F16 (step.X, step.Z);

			if (dist < MINDIST_HEAD) {
				dir ^= 0x800000;
				if (x == OBS_CHASING) {
					if (rand()&1) x = OBS_CIRCLE_RIGHT;
					else x = OBS_CIRCLE_LEFT;
				}

			}

			else {
				if (dist > MINDIST_HEAD + (STEPLEN_HEAD<<1)) x = OBS_CHASING;

				if (x == OBS_CIRCLE_LEFT) {
					if (!(rand()&0x1f)) x = OBS_CIRCLE_RIGHT;
					else dir = (dir - 0x400000)& 0xffffff; // turn 90 degrees
				}
				else if	(x == OBS_CIRCLE_RIGHT) {
					if (!(rand()&0x1f)) x = OBS_CIRCLE_LEFT;
					else dir = (dir + 0x400000)& 0xffffff;
				}
			}

		}
//		else
//			dist = 0;

		if (x == OBS_CIRCLE_RIGHT || x == OBS_CIRCLE_LEFT)
			stepsize = CIRCLELEN_HEAD;
		else if (x == OBS_SHOT)
			stepsize = SHOTLEN_HEAD;
		else if (x == OBS_SPIT)
			stepsize = 0;
		else
			stepsize = STEPLEN_HEAD;

		step.X = MulSF16 (CosF16 (dir), stepsize);
		step.Z = MulSF16 (SinF16 (dir), stepsize);

		if (moveposition
		     (&om->om_Pos, &step, (Object *) om, TRUE, FALSE))
		{
			if (x == OBS_CIRCLE_LEFT) x = OBS_CIRCLE_RIGHT;
			else if (x == OBS_CIRCLE_RIGHT) x = OBS_CIRCLE_LEFT;
			else if (x == OBS_SHOT) {
				x = OBS_DYING;
// printf("Head shot hit wall ----\n");
 				om->om_CurFrame	= 0;
				om->om_CurSeq	= def_Head.dm_SploogieDeath;
			}
		}

		om->ob.ob_State = x;

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

		if (nobverts + HEADVERTS > NOBVERTS)
			break;

		om->ob.ob_VertIdx = nobverts;
		zv = obverts + nobverts;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y - (ONE_F16>>2);
		zv->Z = om->om_Pos.Z;
		zv++;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y - (ONE_F16>>2) - ONE_F16;
		zv->Z = om->om_Pos.Z;

		nobverts += HEADVERTS;

		break;
	 }
	case OP_RENDER:
	 {
		register CCB	*ccb, *srcccb;
		register Vertex	*vp;
		register frac16	x, y;
		Point		corner[4];

	static int32 TestPLUT[] =
	{
		0x29084391,0x4d5009c5,
		0x422259e6,0x53f21521,
		0x39e22883,0x2e4728e9,
		0x2e6a29a9,0x264b1dc5,
		0x6e945374,0x2a060d63,
		0x00001846,0x2d234a2d,
		0x45e63148,0x4ed32d50,
		0x5ded2468,0x18650000,
	};

		vp = obverts + om->ob.ob_VertIdx;
		if (vp->Z < (ZCLIP >> 8))
			return(0);

		srcccb = om->om_CurSeq->celptrs[om->om_CurFrame];

		x = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Width));
		y = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Height));

		x = ConvertF16_32 ((vp[0].Y - vp[1].Y) * x);
		y = ConvertF16_32 ((vp[0].Y - vp[1].Y) * y);

		corner[0].pt_X = vp[0].X - (x >> 1);
		corner[0].pt_Y = corner[1].pt_Y = vp[0].Y - y;

		corner[1].pt_X = corner[0].pt_X + x;

		corner[2].pt_X = corner[1].pt_X;
		corner[2].pt_Y = corner[3].pt_Y = vp[0].Y;

		corner[3].pt_X = corner[0].pt_X;

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

		FasterMapCel (ccb, corner);
		ccb->ccb_Flags &= ~CCB_LAST;

		/*
		 * Disgusting hack for shot targeting.
		 */
		vp[0].X = corner[0].pt_X;
		vp[0].Y = corner[1].pt_X;

		/*
		 * We saw him, so he sees us.
		 */
		if (om->ob.ob_State == OBS_ASLEEP)
			om->ob.ob_State = OBS_AWAKENING;
// why not making dead = invalid anymore????????
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
		if (ov->X <= CX  &&  ov->Y >= CX && (om->ob.ob_State != OBS_DYING)) {
			if (om->om_CurSeq == def_Head.dm_Sploogie) {
				om->om_CurSeq	= def_Head.dm_SploogieDeath;
				om->ob.ob_State	= OBS_DYING;
				om->om_CurFrame	= 0;
				return (obverts[om->ob.ob_VertIdx].Z << 8);
			} else
			{
				if (om->om_AtkDly) {
					om->om_AtkDly = 0;
					return (0);
				}
				else {
					om->om_CurFrame	= 0;
					if (!(--(om->om_Hits))) {
						om->om_CurSeq	= def_Head.dm_Death;
						om->ob.ob_State	= OBS_DYING;
						playsound (HEADDEATHSND);
						return (obverts[om->ob.ob_VertIdx].Z << 8);
					}
					else {
						om->ob.ob_State	= OBS_AWAKENING;
						om->om_CurSeq	= def_Head.dm_Snarl;
						return (obverts[om->ob.ob_VertIdx].Z << 8); //????
					}
				}
			}
		}
		break;
	 }
	}	// switch()
	return (0);
}



void
loadhead ()
{
	register CCB	*ccb;
	register int	i;

	int	waitanim[] = {0,1,2,3,4,5,4,3,2,1,-1};
	int	anticipateanim[] = {6,7,8,9,8,7,6,-1}; // from 1 or 2
	int	snarlanim[] = {10,11,12,13,14,15,16,15,14,-1}; // from 8
	int	transformanim[] = {24,25,26,27,28,28,29,30,31,31,32,
						33,34,35,36,37,38,-1}; // from 8 or 12
	int	badheadanim[] = {40,41,42,43,44,45,46,47,48,-1};
	int	spitanim[] = {48,49,50,51,56,55,56,55,54,53,52,51,50,49,48,-1};
	int	rechargeanim[] = {57,58,59,60,61,62,63,64,65,66,58,-1};
	int	sploogieanim[] = {81,82,83,84,85,-1};
	int	deathanim[] = {67,68,69,70,71,72,73,74,75,76,77,78,79,80,-1};
	int	sploogiedeathanim[] = {86,87,88,-1};

	/*
	 * Load and initialize cels.  (Will get converted to LOAF.)
	 */
	if (!(def_Head.dm_Images = parse3DO ("$progdir/head.cels")))
		die ("Couldn't load head.\n");

  printf("*** head ncels = %d\n",def_Head.dm_Images->ncels);

	for (i = def_Head.dm_Images->ncels;  --i >= 0; ) {
		ccb = def_Head.dm_Images->celptrs[i];
		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);
	}

	/*
	 * Set up animation sequencing descriptors.
	 */
	setupanimarray(&def_Head.dm_Wait,waitanim,def_Head.dm_Images->celptrs);
 	setupanimarray(&def_Head.dm_Anticipate,anticipateanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Snarl,snarlanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Transform,transformanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_BadHead,badheadanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Spit,spitanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Recharge,rechargeanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Death,deathanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_Sploogie,sploogieanim,def_Head.dm_Images->celptrs);
	setupanimarray(&def_Head.dm_SploogieDeath,sploogiedeathanim,def_Head.dm_Images->celptrs);

	loadsound ("HEADEXPLODE1.aifc",HEADDEATHSND);
}

void
destructhead ()
{
	if (def_Head.dm_Wait) {
		freecelarray (def_Head.dm_Wait);
		def_Head.dm_Wait = NULL;
	}
	if (def_Head.dm_Anticipate) {
		freecelarray (def_Head.dm_Anticipate);
		def_Head.dm_Anticipate = NULL;
	}
	if (def_Head.dm_Snarl) {
		freecelarray (def_Head.dm_Snarl);
		def_Head.dm_Snarl = NULL;
	}
	if (def_Head.dm_Transform) {
		freecelarray (def_Head.dm_Transform);
		def_Head.dm_Transform = NULL;
	}
	if (def_Head.dm_BadHead) {
		freecelarray (def_Head.dm_BadHead);
		def_Head.dm_BadHead = NULL;
	}
	if (def_Head.dm_Spit) {
		freecelarray (def_Head.dm_Spit);
		def_Head.dm_Spit = NULL;
	}
	if (def_Head.dm_Recharge) {
		freecelarray (def_Head.dm_Recharge);
		def_Head.dm_Recharge = NULL;
	}
	if (def_Head.dm_Death) {
		freecelarray (def_Head.dm_Death);
		def_Head.dm_Death = NULL;
	}
	if (def_Head.dm_SploogieDeath) {
		freecelarray (def_Head.dm_SploogieDeath);
		def_Head.dm_SploogieDeath = NULL;
	}
	if (def_Head.dm_Images) {
		freecelarray (def_Head.dm_Images);
		def_Head.dm_Images = NULL;
	}
	unloadsound (HEADDEATHSND);
}


static void
animhead (om, nvbls)
register struct ObHead	*om;
int			nvbls;
{
	register CelArray	*ca;
	register int		i;

	ca = om->om_CurSeq;
//	i = nextanimframe (&om->om_AnimDef, om->om_CurFrame, nvbls);
	i = om->om_CurFrame + 1;


	switch (om->ob.ob_State) {
	case OBS_AWAKENING:
		if (i >= ca->ncels) {
			i = 0;

			if (om->om_CurSeq == def_Head.dm_Wait) {
				if (om->om_AtkDly < ANTICDELAY) {
					om->om_CurSeq	= def_Head.dm_Anticipate;
				}
			}
			else if (om->om_CurSeq == def_Head.dm_Anticipate) {
				if (om->om_AtkDly < SNARLDELAY) {
					om->om_CurSeq	= def_Head.dm_Snarl;
				}
			}
			else if (om->om_CurSeq == def_Head.dm_Snarl) {
				if (!om->om_AtkDly) {
					om->om_CurSeq	= def_Head.dm_Transform;
				}
			}
			else if (om->om_CurSeq == def_Head.dm_Transform) {
				om->om_CurSeq	= def_Head.dm_BadHead;
				om->ob.ob_State = OBS_CHASING;
			}
		}

		om->om_CurFrame = i;

		break;

	case OBS_CHASING:
	case OBS_CIRCLE_LEFT:
	case OBS_CIRCLE_RIGHT:
		if (i >= ca->ncels) {
			i = 0;
		    	if (!(rand () & 0x3)) {
				om->om_CurSeq	= def_Head.dm_Spit;
				om->ob.ob_State = OBS_SPIT;
			}
		}
		om->om_CurFrame = i;

		break;


	case OBS_SPIT:
		if (i >= ca->ncels) {
// printf("ob_State = OBS_SPIT, om->om_CurSeq = %lx, (def_Head.dm_Spit = %lx)\n",
//  	om->om_CurSeq,def_Head.dm_Spit);
 			i = 0;
			if (om->om_CurSeq == def_Head.dm_Spit)
				om->om_CurSeq	= def_Head.dm_Recharge;
			else {
				om->ob.ob_State = OBS_CHASING;
				om->om_CurSeq	= def_Head.dm_BadHead;
			}
		}
		om->om_CurFrame = i;

		break;


	case OBS_SHOT:
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
			om->om_CurSeq	= def_Head.dm_Spot;
		}
		om->om_CurFrame = i;
*/
		break;
	case OBS_DEAD:
		break;
	}
}
