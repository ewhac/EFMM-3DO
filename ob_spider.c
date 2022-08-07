/*  :ts=8 bk=0
 *
 * ob_spider.c:	Creepy, crawly, deadly, and legs everywhere.
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

#define	WAKEUPDIST		(ONE_F16*8)
#define	MINDIST_SPIDER		(ONE_F16)             // (ONE_F16+(ONE_F16>>2))
#define	MINDIST_SPIDERSHOT	(ONE_F16 >> 1)
#define	STEPLEN_SPIDER		((ONE_F16) >> 4)
#define	CIRCLELEN_SPIDER	((ONE_F16) >> 5)
#define	SHOTLEN_SPIDER		(0x4000)
#define	SPIDERANIMSPEED		12
#define	SPIDER_SIZ		(2 * ONE_F16 / 8)
#define	SPIDERVERTS		2

#define SPIDERDEATHSND	108

// put here for now to keep this all together
#define OBS_CIRCLE_LEFT		(OBS_DEAD + 1)
#define	OBS_CIRCLE_RIGHT	(OBS_DEAD + 2)
#define	OBS_SHOT		(OBS_DEAD + 3)
#define	OBS_SHOOTING		(OBS_DEAD + 4)


typedef struct DefSpider {
	struct ObDef	od;
	struct CelArray	*dm_Images;
	struct CelArray	*dm_Drop,
			*dm_Approach,
			*dm_Attack,
			*dm_Shoot,
			*dm_Death,
			*dm_SpiderShot,
			*dm_ShotDeath;
} DefSpider;

typedef struct ObSpider {
	struct Object	ob;
	struct CelArray	*om_CurSeq;
	struct MapEntry	*om_ME;
	struct AnimDef	om_AnimDef;
	Vertex		om_Pos;
	frac16		om_Dir;
	int32		om_CurFrame;
	int32		om_AtkDly;
	int32		om_Height;
} ObSpider;

/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 spider (struct ObSpider *, int, void *);
static void animspider(struct ObSpider *om,int nvbls);


/***************************************************************************
 * Object definition instance.
 */
struct DefSpider		def_Spider = {
 {	spider,
	OTYP_SPIDER,
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
 * Spider
 */

static int32
spider (om, op, dat)
register struct ObSpider	*om;
int				op;
void				*dat;
{
	switch (op) {
	case OP_FIATLUX:
		loadspider ();
		break;

	case OP_DESTRUCT:
		if (def_Spider.od.od_ObCount > 0)
			/*  You haven't deleted them all...  */
			return (def_Spider.od.od_ObCount);
		else
			destructspider ();
		break;

	case OP_CREATEOB:
		return ((int32) createStdObject ((ObDef *) &def_Spider,
						 OTYP_SPIDER,
						 sizeof (ObSpider),
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
		om->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER | OBF_CONTACT; // |  OBF_SHOOT;

		om->om_CurSeq	= def_Spider.dm_Drop;
		om->om_Dir	=
		om->om_CurFrame	= 0;
		om->om_ME	= id->id_MapEntry;
		om->om_Pos.X	= Convert32_F16 (id->id_XIdx) + HALF_F16;
		om->om_Pos.Z	= Convert32_F16 (id->id_ZIdx) + HALF_F16;
		om->om_AnimDef.ad_FPS = SPIDERANIMSPEED;
		om->om_AnimDef.ad_Counter = VBLANKFREQ;
		break;
	 }

	case OP_MOVE:
	 {
		register frac16	dist, dir;
		register int	x, z;
		register Object	**obt; // for shot
		ObSpider	*oh;
		register ObDef	*od;
		MapEntry	*me;
		Vector		step;
		int		nframes,stepsize;
		int		i; //spidershot

		nframes = (int) dat;
		animspider (om,nframes);

		if (om->ob.ob_State == OBS_DEAD) {
			om->ob.ob_State = OBS_INVALID;	// Mark inactive.
			removeobfromme ((Object *) om, om->om_ME);
			break;
		}

		step.X = playerpos.X - om->om_Pos.X;
		step.Y = 0;
		step.Z = playerpos.Z - om->om_Pos.Z;
		dist = approx2dist (step.X, step.Z, 0, 0);

		if ((om->ob.ob_State == OBS_ASLEEP)&&(dist<WAKEUPDIST)) {
			om->om_CurFrame	= 0;
			om->ob.ob_State = OBS_AWAKENING;
		}

		if (((x = om->ob.ob_State) != OBS_CHASING) &&
		    (x != OBS_SHOT) &&
		    (x != OBS_SHOOTING) &&
		    (x != OBS_ATTACKING) &&
		    (x != OBS_CIRCLE_RIGHT) &&
		    (x != OBS_CIRCLE_LEFT))
			/*  Nuthin' to do.  */
			return (0);

// if (x == OBS_SHOOTING) printf ("Trying to create spider shot. om->om_CurFrame = %d\n", om->om_CurFrame);


		if ((x == OBS_SHOOTING) && (om->om_CurFrame > 4) && (!om->om_AtkDly)) {
			for (obt = obtab, i = 0;  i < obtabsiz;  obt++, i++)
			{
				oh = (ObSpider *) *obt;
				if (!oh)
					break;
				if (oh->ob.ob_Type == OTYP_SPIDER  &&
				    oh->ob.ob_State == OBS_INVALID)
					break;
			}

//printf ("using #%d.\n oh = 0x%lx, _State = %d,",
//	i, oh, oh->ob.ob_State);
//printf (" _Type = %d, obtabsiz = %d.\n", oh->ob.ob_Type, obtabsiz);

			if (i < obtabsiz) { // found empty object slot (or dead thing)
				if (oh) {
					od = oh->ob.ob_Def;
					if ((od->od_Func)
					     (oh, OP_DELETEOB, NULL) < 0)
						die ("Failed to delete object.\n");
				}

				if (!(oh = (ObSpider *) (spider (NULL, OP_CREATEOB, NULL))))
					die ("Can't create object.\n");
				me = om->om_ME;
 				oh->ob.ob_State	= OBS_SHOT;
				oh->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER | OBF_SHOOT | OBF_CONTACT;
				oh->om_CurSeq	= def_Spider.dm_SpiderShot;
				oh->om_Dir	= om->om_Dir;
				oh->om_CurFrame = 0;
				oh->om_ME	= me;
				oh->om_Pos.X	= om->om_Pos.X;
				oh->om_Pos.Z	= om->om_Pos.Z;
				oh->om_AnimDef.ad_FPS = om->om_AnimDef.ad_FPS;
				oh->om_AnimDef.ad_Counter = om->om_AnimDef.ad_Counter;
//				om->om_AnimDef.ad_Counter = VBLANKFREQ;

				addobtome ((Object *) oh, me); //correct?

				*obt = (Object *) oh;
				om->om_AtkDly = 1;
			}
// printf("Created spider shot.\n");
		}


		if (x == OBS_SHOT) { // shot move at you (heat seeking?)
			dir = om->om_Dir;
			if (dist < MINDIST_SPIDERSHOT) {
				/*
				 * Spider takes from 8 - 15 points off.
				 */
				takedamage ((rand () & 3) + 4);
//				om->om_AtkDly = VBLANKFREQ + rand () % VBLANKFREQ;
// need to create separate spider shot death sequence
				x = OBS_DYING;
				om->om_CurFrame	= 0;
				om->om_CurSeq	= def_Spider.dm_Death;

			}
		}
		else if (x != OBS_SHOOTING) {
			dir = om->om_Dir = Atan2F16 (step.X, step.Z);

			if (dist < MINDIST_SPIDER) {
				dir ^= 0x800000;
				if (x == OBS_CHASING) {
					if (rand()&1) x = OBS_CIRCLE_RIGHT;
					else x = OBS_CIRCLE_LEFT;
				}

			}

			else {
				if (dist > MINDIST_SPIDER + (STEPLEN_SPIDER<<1)) x = OBS_CHASING;

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

		if ((x == OBS_CIRCLE_RIGHT) || (x == OBS_CIRCLE_LEFT)) {
			stepsize = CIRCLELEN_SPIDER;
			if (!(rand()&0x1f)) {
				x = OBS_ATTACKING;
				om->om_CurSeq	= def_Spider.dm_Attack;
				om->om_CurFrame = 0;
				om->om_AtkDly = 0;
			}
		}
		else if (x == OBS_SHOT)
			stepsize = SHOTLEN_SPIDER;
		else {
			stepsize = STEPLEN_SPIDER;
			if ((x == OBS_ATTACKING) && (om->om_CurFrame > 5)) {
				if ((!om->om_AtkDly) && (dist < MINDIST_SPIDER))
					takedamage ((rand () & 3) + 4);
				om->om_AtkDly = 1;
			}
		}

		step.X = MulSF16 (CosF16 (dir), stepsize);
		step.Z = MulSF16 (SinF16 (dir), stepsize);

		if (moveposition
		     (&om->om_Pos, &step, (Object *) om, TRUE, FALSE))
		{
			if (x == OBS_CIRCLE_LEFT) x = OBS_CIRCLE_RIGHT;
			else if (x == OBS_CIRCLE_RIGHT) x = OBS_CIRCLE_LEFT;
			else if (x == OBS_SHOT) {
				x = OBS_DYING;
				om->om_CurFrame	= 0;
				om->om_CurSeq	= def_Spider.dm_ShotDeath;
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
		register int32 height;

	int	dropht[] = {0x10000,0xf000,0xe000,0xd000,0xc000,0xb000,0xa000,0x9000,
			     0x8800,0x8400,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
			     0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x7c000,0x7800,
			     0x7000,0x6000,0x5000,0x4000,0x3000,0x2000,0x1000,0x0000,
		};
	int	attackht[] = {0xa00,0x1400,0x1e00,0x2800,0x3200,0x3c00,0x4200,
			0x4600,0x4200,0x3c00,0x3200,0x2800,0x1e00,0xa00};

		if (nobverts + SPIDERVERTS > NOBVERTS)
			break;

		om->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER | OBF_CONTACT;
		height = 0;
		if (om->om_CurSeq == def_Spider.dm_Drop) {
			if (om->ob.ob_State == OBS_ASLEEP)
				height = dropht[0];
			else if (height = dropht[om->om_CurFrame])
				om->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER | OBF_SHOOT;
		}
		else if ((om->om_CurSeq == def_Spider.dm_Attack) ||
			(om->om_CurSeq == def_Spider.dm_Shoot)) {
			height = attackht[om->om_CurFrame];
			om->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER | OBF_SHOOT;
		}
		if (om->ob.ob_State == OBS_DYING) height = om->om_Height;
		om->om_Height = height;

		om->ob.ob_VertIdx = nobverts;
		zv = obverts + nobverts;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y + (ONE_F16>>2) - (ONE_F16>>3) - height;
		zv->Z = om->om_Pos.Z;
		zv++;

		zv->X = om->om_Pos.X;
		zv->Y = om->om_Pos.Y - 0xc000 + (ONE_F16>>2) - (ONE_F16>>3) - height;
		zv->Z = om->om_Pos.Z;

		nobverts += SPIDERVERTS;

		break;
	 }
	case OP_RENDER:
	 {
		register CCB	*ccb, *srcccb;
		register Vertex	*vp;
		register frac16	x, y;
		Point		corner[4];

		vp = obverts + om->ob.ob_VertIdx;
		if (vp->Z < (ZCLIP >> 8))
			break;

		srcccb = om->om_CurSeq->celptrs[om->om_CurFrame];

		x = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Width));
		y = PIXELS2UNITS (REALCELDIM (srcccb->ccb_Height));

		x = (ConvertF16_32 ((vp[0].Y - vp[1].Y) * x))>>1;
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
//		if (om->ob.ob_State == OBS_ASLEEP)
//			om->ob.ob_State = OBS_AWAKENING;

//		else if (om->ob.ob_State == OBS_DEAD) {
//			om->ob.ob_State = OBS_INVALID;	// Remove from list.
//			removeobfromme ((Object *) om, om->om_ME);
//		}

// generate shot
		if ((om->ob.ob_State == OBS_CHASING)  &&
		    (!(rand () & 0x3f))) {
			om->ob.ob_State = OBS_SHOOTING;
			om->om_CurSeq = def_Spider.dm_Shoot;
			om->om_CurFrame = 0;
			om->om_AtkDly = 0;
		}

		break;
	 }
	case OP_SHOT:
	 {
		register Vertex	*ov;

		ov = obverts + om->ob.ob_VertIdx;
		if (ov->X <= CX  &&  ov->Y >= CX) {
			om->ob.ob_State	= OBS_DYING;
			om->om_CurFrame	= 0;
			if (om->om_CurSeq == def_Spider.dm_SpiderShot) {
				om->om_CurSeq	= def_Spider.dm_ShotDeath;
			} else
			{
				om->om_CurSeq	= def_Spider.dm_Death;
				playsound (SPIDERDEATHSND);
			}
			return (obverts[om->ob.ob_VertIdx].Z << 8);
		}
		break;
	 }
	case OP_CONTACT:
	 {
		register PathBox	*pb;
		BBox			zb;

		pb = dat;

		zb.MinX = om->om_Pos.X - SPIDER_SIZ;
		zb.MinZ = om->om_Pos.Z - SPIDER_SIZ;
		zb.MaxX = om->om_Pos.X + SPIDER_SIZ;
		zb.MaxZ = om->om_Pos.Z + SPIDER_SIZ;

		return (checkcontact (pb, &zb, TRUE));
	 }

	}	// switch()
	return (0);
}



void
loadspider ()
{
	register CCB	*ccb;
	register int	i;

	int	dropanim[] =   {25,26,27,28,29,25,26,27,28,29,
				25,26,27,28,29,25,26,27,28,29,
				25,26,27,28,29,25,26,27,28,29,
				25,26,27,28,29,25,-1};  // sync w/dropht[] in OP_REGISTER
	int	approachanim[] = {12,13,14,15,16,17,18,19,20,21,22,23,24,-1};
	int	attackanim[] = {30,31,32,33,34,35,36,39,36,37,38,39,40,41,-1}; // sync w/attackht[]
	int	shootanim[] = {41,41,42,43,44,45,46,47,48,48,48,-1}; // sync w/attackht[]
	int	deathanim[] = {0,1,2,3,4,5,6,7,8,9,10,11,-1};
	int	spidershotanim[] = {49,50,51,52,53,54,55,-1};
	int	shotdeathanim[] = {9,10,11,-1};

	/*
	 * Load and initialize cels.  (Will get converted to LOAF.)
	 */
	if (!(def_Spider.dm_Images = parse3DO ("$progdir/spider.cels")))
		die ("Couldn't load spider.\n");

 printf("*** spider ncels = %d\n",def_Spider.dm_Images->ncels);

	for (i = def_Spider.dm_Images->ncels;  --i >= 0; ) {
		ccb = def_Spider.dm_Images->celptrs[i];
		ccb->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
				  CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
		ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);
	}

	/*
	 * Set up animation sequencing descriptors.
	 */
 	setupanimarray(&def_Spider.dm_Drop,dropanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_Approach,approachanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_Attack,attackanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_Shoot,shootanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_Death,deathanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_SpiderShot,spidershotanim,def_Spider.dm_Images->celptrs);
	setupanimarray(&def_Spider.dm_ShotDeath,shotdeathanim,def_Spider.dm_Images->celptrs);

	loadsound ("SpiderExplodes.aifc",SPIDERDEATHSND);
}

void
destructspider ()
{
	if (def_Spider.dm_Death) {
		freecelarray (def_Spider.dm_Death);
		def_Spider.dm_Death = NULL;
	}
	if (def_Spider.dm_Drop) {
		freecelarray (def_Spider.dm_Drop);
		def_Spider.dm_Drop = NULL;
	}
	if (def_Spider.dm_Attack) {
		freecelarray (def_Spider.dm_Attack);
		def_Spider.dm_Attack = NULL;
	}
	if (def_Spider.dm_Shoot) {
		freecelarray (def_Spider.dm_Shoot);
		def_Spider.dm_Shoot = NULL;
	}
	if (def_Spider.dm_SpiderShot) {
		freecelarray (def_Spider.dm_SpiderShot);
		def_Spider.dm_SpiderShot = NULL;
	}
	if (def_Spider.dm_ShotDeath) {
		freecelarray (def_Spider.dm_ShotDeath);
		def_Spider.dm_ShotDeath = NULL;
	}
	if (def_Spider.dm_Approach) {
		freecelarray (def_Spider.dm_Approach);
		def_Spider.dm_Approach = NULL;
	}
	if (def_Spider.dm_Images) {
		freecelarray (def_Spider.dm_Images);
		def_Spider.dm_Images = NULL;
	}
	unloadsound (SPIDERDEATHSND);
}


static void
animspider (om, nvbls)
register struct ObSpider	*om;
int				nvbls;
{
	register CelArray	*ca;
	register int		i;

	ca = om->om_CurSeq;
//	i = nextanimframe (&om->om_AnimDef, om->om_CurFrame, nvbls);
	i = om->om_CurFrame+1;

	switch (om->ob.ob_State) {
	case OBS_AWAKENING:
	{
		if (i >= ca->ncels) {
			i = 0;
			om->ob.ob_State	= OBS_CHASING;
			om->om_CurSeq	= def_Spider.dm_Approach;
		}
		om->om_CurFrame = i;

		break;
	}

	case OBS_SHOOTING:
	case OBS_ATTACKING:
// printf(">> i = %d\n",i);
		if (i >= ca->ncels) {
			i = 0;
			om->ob.ob_State	= OBS_CHASING;
			om->om_CurSeq	= def_Spider.dm_Approach;
		}
		om->om_CurFrame = i;

		break;

	case OBS_ASLEEP:
	case OBS_CHASING:
		if (i >= ca->ncels)
			i = 0;
		om->om_CurFrame = i;

		break;

	case OBS_SHOT:
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

	case OBS_DEAD:
		break;
	}
}
