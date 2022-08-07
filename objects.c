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


/***************************************************************************
 * Globals.
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];

extern CCB	ccbpool[];
extern CCB	*curccb;

extern Vertex	unitsquare[], xformsquare[];
extern Vector	plusx, plusz;

extern Vertex	obverts[], xfobverts[], *curobv;
extern int32	nobverts;

extern Vertex	playerpos;
extern int32	playerhealth;
extern int32	nkeys;
extern frac16	damagefade;

extern uint32	linebuf[];

extern uint32	ccbextra;

extern int32	cy;


Object		**obtab;
int32		obtabsiz;


/***************************************************************************
 * Forward references (see end of file).
 */
extern ObDef	*obdefs[];
extern int32	nobdefs;


/***************************************************************************
 * Code, mon.
 */
#if 0
/*  I don't appear to use this anymore...	*/
void
generateobjects (numobs)
int32	numobs;
{
	register MapEntry	*me;
	register ObDef		*od;
	register Object		*ob, **obt;
	register int		x, z;
	InitData		id;

	if (!(obtab = malloc (sizeof (Object *) * numobs)))
		die ("Can't allocate object table.\n");
	obtabsiz = numobs;

	obt = obtab;
	for (z = WORLDSIZ;  --z >= 0; ) {
		for (x = WORLDSIZ;  --x >= 0; ) {
			me = &levelmap[z][x];

			/*
			 * MapEntries initially contain a pointer to the
			 * ObDef.  This is used to generate an instance.
			 */
			if (!(od = (ObDef *) me->me_Obs))
				continue;

			id.id_MapEntry	= me;
			id.id_XIdx	= x;
			id.id_ZIdx	= z;

			if (!(ob = (Object *) ((od->od_Func) (NULL,
							      OP_CREATEOB,
							      NULL))))
				die ("Can't create object.\n");

			if ((od->od_Func) (ob, OP_INITOB, &id) < 0)
				die ("Error initializing object.\n");

			me->me_Obs = ob;		// Stomp over ObDef
			me->me_Flags |= MEF_ARTWORK;	// Do I need this?

			*obt++ = ob;
		}
	}
}
#endif


void
freeobjects ()
{
	register ObDef	*od;
	register Object	*ob, **obt;
	register int	i;

	if (obtab) {
		obt = obtab;
		for (i = obtabsiz;  --i >= 0; ) {
			if (ob = *obt++) {
				od = ob->ob_Def;
				if ((od->od_Func) (ob, OP_DELETEOB, NULL)< 0)
					die ("Failed to delete object.\n");
			}
		}
		freetype (obtab);  obtab = NULL;
	}
}


struct Object *
createStdObject (od, type, size, flags, dupsrc)
struct ObDef	*od;
int		type, size, flags;
struct Object	*dupsrc;
{
	register Object	*ob;

	if (ob = malloctype (size, MEMTYPE_FILL)) {
		od->od_ObCount++;
		if (dupsrc  &&  dupsrc->ob_Type == type) {
			/*  Make a copy of supplied object.  */
			*ob = *dupsrc;
			ob->ob_Next	= NULL;
		} else {
			ob->ob_Def	= od;
			ob->ob_Type	= type;
			ob->ob_State	= OBS_INVALID;
			ob->ob_Flags	= flags;
		}
	}
	return (ob);
}


void
deleteStdObject (ob)
register struct Object	*ob;
{
	ob->ob_Def->od_ObCount--;
	freetype (ob);
}





void
initobdefs ()
{
	register ObDef	*od;
	register int	i;

	for (i = nobdefs;  --i >= 0; ) {
		od = obdefs[i];
		if (od->od_ObCount) {
			/*
			 * od_ObCount was set up by the level loader to
			 * inform us whether or not we need the object.
			 * Clear it and initialize the object.
			 */
			od->od_ObCount = 0;
			(od->od_Func) (NULL, OP_FIATLUX, NULL);
		}
	}
}

void
destructobdefs ()
{
	register ObDef	*od;
	register int	i;

	for (i = nobdefs;  --i >= 0; ) {
		od = obdefs[i];
		(od->od_Func) (NULL, OP_DESTRUCT, NULL);
	}
}


void
registerobs (vo)
struct VisOb	*vo;
{
	register Object	*ob, *next;
	register int32	(*func)();
	register int	retval;

	ob = vo->vo_ME->me_Obs;
	while (ob) {
		next = ob->ob_Next;
		if (ob->ob_State  &&  (ob->ob_Flags & OBF_REGISTER)) {
			if (func = ob->ob_Def->od_Func)
				retval = func (ob, OP_REGISTER, vo);
		}
		ob = next;
	}
}

void
renderobs (vo)
struct VisOb	*vo;
{
	register Object	*ob, *next;
	register int32	(*func)();
	register int	retval;

	ob = vo->vo_ME->me_Obs;
	while (ob) {
		next = ob->ob_Next;
		if (ob->ob_State  &&  (ob->ob_Flags & OBF_RENDER)) {
			if (func = ob->ob_Def->od_Func)
				retval = func (ob, OP_RENDER, vo);

		}
		ob = next;
	}
}



void
moveobjects (nframes)
int	nframes;
{
	register Object	*ob, **obt;
	register int	i;
	int32		(*func)();

	obt = obtab;
	for (i = obtabsiz;  --i >= 0; ) {
		ob = *obt++;
		if (ob  &&  ob->ob_State  &&  (ob->ob_Flags & OBF_MOVE)) {
			if (func = ob->ob_Def->od_Func)
				func (ob, OP_MOVE, nframes);
		}
	}
}





void
removeobfromme (ob, me)
struct Object	*ob;
struct MapEntry	*me;
{
	register Object	*tstob;

	tstob = me->me_Obs;

	if (tstob == ob) {
		me->me_Obs = ob->ob_Next;
		return;
	}

	while (tstob) {
		if (tstob->ob_Next == ob) {
			tstob->ob_Next = ob->ob_Next;
			return;
		}
		tstob = tstob->ob_Next;
	}
}

void
addobtome (ob, me)
struct Object	*ob;
struct MapEntry	*me;
{
	/*
	 * Simple insertion at the head.  Sorting has to be done at
	 * projection.
	 */
	ob->ob_Next = me->me_Obs;
	me->me_Obs = ob;
}



void
removeobfromslot (ob)
register struct Object	*ob;
{
	register int	i;
	register Object	**obt;

	for (i = obtabsiz, obt = obtab;  --i >= 0;  obt++) {
		if (*obt == ob) {
			*obt = NULL;
			break;
		}
	}
}




void
takedamage (hitpoints)
int32	hitpoints;
{
	if ((playerhealth -= hitpoints) < 0)
		/*  Should I play death sound here or out in gamemain()?  */
		playerhealth = 0;

	if (hitpoints) {
		if (hitpoints <= 3)
			playsound (SFX_OUCHSMALL);
		else if (hitpoints <= 10)
			playsound (SFX_OUCHMED);
		else
			playsound (SFX_OUCHBIG);

		if ((damagefade += hitpoints << 11) > 0xC000)
			damagefade = 0xC000;
	}
}



/***************************************************************************
 * Compute approximation to 2D and 3D euclidian distance.
 * Extracted from Graphics Gems volume 1.
 * (Am I actually going to use these?)
 */
frac16
approx2dist (x1, y1, x2, y2)
register frac16	x1, y1, x2, y2;
{
	if ((x2 -= x1) < 0)	x2 = -x2;
	if ((y2 -= y1) < 0)	y2 = -y2;

	if (x2 > y2)
		return (x2 + y2 - (y2 >> 1));
	else
		return (x2 + y2 - (x2 >> 1));
}


frac16
approx3dist (x1, y1, z1, x2, y2, z2)
register frac16	x1, y1, z1, x2, y2, z2;
{
	/*
	 * Compute abs([xyz]2 - [xyz]1);
	 */
	if ((x1 -= x2) < 0)	x1 = -x1;
	if ((y1 -= y2) < 0)	y1 = -y1;
	if ((z1 -= z2) < 0)	z1 = -z1;

	/*
	 * Sort x1, y1, z1 into descending order.
	 */
	if (x1 < y1) {
		x1 ^= y1;  y1 ^= x1;  x1 ^= y1;	/*  Swap  */
	}
	if (x1 < z1) {
		x1 ^= z1;  z1 ^= x1;  x1 ^= z1;	/*  Swap  */
	}

	/*
	 * Compute approximation: d == max + 5/16 * med + 1/4 * min;
	 */
	y1 = (y1 + (y1 << 2)) >> 4;	/*  y1 * 5 / 16  */
	z1 >>= 2;
	return (x1 + y1 + z1);
}


/***************************************************************************
 * Animation cycling tools.
 */
int32
nextanimframe (ad, fnum, nvbls)
register struct AnimDef	*ad;
int32			fnum, nvbls;
{
	while (--nvbls >= 0) {
		if ((ad->ad_Counter -= ad->ad_FPS) <= 0) {
			ad->ad_Counter += VBLANKFREQ;
			fnum++;
		}
	}
	return (fnum);
}









/***************************************************************************
 * Object Processors.
 ***************************************************************************
 * Each routine has knowledge of how many verticies it uses and how to use
 * them.
 */


int
setupanimarray (
struct CelArray	**captr,
int		*animseq,
struct CCB	**ccbp
)
{
	CelArray	*ca;
	int		i,count;

	for (count = 0;  animseq[count] >= 0;  count++)
		;

	if (count) {
// printf("* count = %d\n",count);
		if (!(ca = alloccelarray (count)))
			die ("Can't allocate monster's celarray.\n");

// printf("Got here\n");

		*captr = ca;
		ca->ncels = count;
		ca->ca_Buffer = NULL;
		ca->ca_Type = 0;
		for (i = 0;  i < count;  i++ ) {
			ca->celptrs[i] = ccbp[animseq[i]];
		}
	} else
		*captr++ = NULL;
}





/***************************************************************************
 * Simple door.
 */
#define	SWINGANGSTEP	(ONE_F16 * 2)
#define	MAXSWINGANG	((64 - 5) * ONE_F16)
#define	DOORWIDE	HALF_F16
#define	PROBEDISTQ	(25 * ONE_F16 / 4)	/*  6.25  */
#define	WALLIMGBASE	54			/*  After the walls.  */
#define	DOORVERTS	16


typedef struct DefNSDoor {
	struct ObDef		od;
	struct ImageEntry	*dd_IE;	// Imagery for door (?).
	ubyte			dd_SpareFlags[6];
	ubyte			dd_RegIdxs[6];
	ubyte			dd_ImgIdxs[6];
} DefNSDoor;

typedef struct ObNSDoor {
	struct Object	ob;
	frac16		od_SwingAng;
	struct MapEntry	*od_ME;
	ubyte		od_XIdx,
			od_ZIdx;
	ubyte		od_Unlocked;
} ObNSDoor;


static int32 simpleNSdoor (struct ObNSDoor *, int, void *);



struct DefNSDoor	def_NSDoor = {
 {	simpleNSdoor,
	OTYP_NSDOOR,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL,
	{ VISF_WEST, VISF_WEST, VISF_EAST, VISF_EAST, 0, 0 },
	{ 0, 3, 6, 2, 5, 8 },
	{ 1, 3, 1, 3, 4, 6 }
};
struct DefNSDoor	def_EWDoor = {
 {	simpleNSdoor,
	OTYP_EWDOOR,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL,
	{ VISF_NORTH, VISF_NORTH, VISF_SOUTH, VISF_SOUTH, 0, 0 },
	{ 6, 7, 8, 0, 1, 2 },
	{ 0, 2, 0, 2, 5, 7 }
};





static int32
simpleNSdoor (od, op, dat)
register struct ObNSDoor	*od;
int				op;
void				*dat;
{
	static ubyte rendidxs[6][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 5, 4 },
		{ 4, 3 },
		{ 1, 6 },
		{ 7, 4 },
	};

	switch (op) {
	case OP_FIATLUX:
		/*  Remains to be defined.  */
		/*  Load separate art files?  */
	case OP_DESTRUCT:
		/*  TBD  */
		break;

	case OP_CREATEOB:
	 {	register ObDef	*def;

		def = (ObDef *) od;
		return ((int32) createStdObject (def,
						 def->od_Type,
						 sizeof (ObNSDoor),
						 0,
						 dat));
	 }
	case OP_DELETEOB:
		deleteStdObject ((Object *) od);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;

		id = dat;
		od->ob.ob_State	= OBS_CLOSED;
		od->ob.ob_Flags	= OBF_MOVE | OBF_REGISTER | OBF_RENDER |
				  OBF_PROBE;
		od->od_ME = id->id_MapEntry;
		od->od_XIdx = id->id_XIdx;
		od->od_ZIdx = id->id_ZIdx;
		od->od_SwingAng = 0;
		od->od_Unlocked = FALSE;	// ### EXPAND THIS OUT!!
		break;
	 }
	case OP_MOVE:
	 {
		register int	nframes;

		nframes = (int) dat;

		if (od->ob.ob_State == OBS_OPENING) {
			od->od_SwingAng += SWINGANGSTEP * nframes;
			if (od->od_SwingAng >= MAXSWINGANG) {
				od->od_SwingAng = MAXSWINGANG;
				od->ob.ob_State = OBS_OPEN;
				od->od_ME->me_Flags &=
				 ~(MEF_WALKSOLID | MEF_SHOTSOLID);
			}
		} else if (od->ob.ob_State == OBS_CLOSING) {
			od->od_SwingAng -= SWINGANGSTEP * nframes;
			if (od->od_SwingAng <= 0) {
				od->od_SwingAng = 0;
				od->ob.ob_State = OBS_CLOSED;
				playsound (SFX_CLOSEDOOR);
			}
		}
		break;
	 }
	case OP_REGISTER:
	 {
		register Vertex	*dv;	// Hi, Dave!
		register frac16	c, s;
		ubyte		*regidxs;
	 	Vector		trans;

		if (nobverts + DOORVERTS > NOBVERTS)
			break;

		od->ob.ob_VertIdx = nobverts;
		dv = obverts + nobverts;
		regidxs = ((DefNSDoor *) (od->ob.ob_Def))->dd_RegIdxs;

		/*  Excuse me while I misuse c and s for a moment...  */
		for (c = 0;  c < 6;  c++) {
			s = *regidxs++;
			*dv++ = unitsquare[s];
			*dv++ = unitsquare[s+9];
		}

		/*
		 * Compute and plot positions of doors.
		 */
		c = MulSF16 (DOORWIDE, CosF16 (od->od_SwingAng));
		s = MulSF16 (DOORWIDE, SinF16 (od->od_SwingAng));
		if (od->ob.ob_Type == OTYP_EWDOOR) {
			/*
			 * East-west door.
			 */
			dv[0].X = dv[1].X = unitsquare[7].X + s;
			dv[0].Z = dv[1].Z = unitsquare[7].Z - c;
			dv[0].Y = 0;  dv[1].Y = -ONE_F16;
			dv += 2;

			dv[0].X = dv[1].X = unitsquare[1].X + s;
			dv[0].Z = dv[1].Z = c;
			dv[0].Y = 0;  dv[1].Y = -ONE_F16;
		} else {
			/*
			 * North-south door.
			 */
			dv[0].X = dv[1].X = c;
			dv[0].Z = dv[1].Z = unitsquare[3].Z + s;
			dv[0].Y = 0;  dv[1].Y = -ONE_F16;
			dv += 2;

			dv[0].X = dv[1].X = unitsquare[5].X - c;
			dv[0].Z = dv[1].Z = unitsquare[5].Z + s;
			dv[0].Y = 0;  dv[1].Y = -ONE_F16;
		}


	 	trans.X = Convert32_F16 (od->od_XIdx);
	 	trans.Y = 0;
	 	trans.Z = Convert32_F16 (od->od_ZIdx);
	 	translatemany (&trans, obverts + od->ob.ob_VertIdx, 16);

		nobverts += DOORVERTS;

		break;
	 }
	case OP_RENDER:
	 {
		register int	i, nvo;
		register ubyte	*spareflags, *imgidxs;
		VisOb		vo[6], *vop, *argvo;
		CCB		*prevccb;

		argvo = dat;

		spareflags = ((DefNSDoor *) (od->ob.ob_Def))->dd_SpareFlags;
		imgidxs = ((DefNSDoor *) (od->ob.ob_Def))->dd_ImgIdxs;
		vop = vo;
		nvo = 0;
		for (i = 6;  --i >= 0; ) {
			if (argvo->vo_VisFlags & spareflags[i])
				continue;

			vop->vo_LIdx = rendidxs[i][0];
			vop->vo_RIdx = rendidxs[i][1];
			vop->vo_MEFlags = 0;
			vop->vo_ImgIdx = imgidxs[i] + WALLIMGBASE;
			vop->vo_ME = NULL;
			vop++;
			nvo++;
		}
		prevccb = curccb;
		buildcellist (vo, nvo,
			      obverts + od->ob.ob_VertIdx,
			      xfobverts + od->ob.ob_VertIdx,
			      FALSE);

		if (prevccb != curccb)
			/*  Something was actually rendered.  */
			od->ob.ob_Flags |= OBF_SAWME;

		break;
	 }
	case OP_PROBE:
	 {
	 	register frac16	distq;
	 	register int	state;

		distq = SquareSF16 (playerpos.X -
				    Convert32_F16 (od->od_XIdx) - HALF_F16) +
			SquareSF16 (playerpos.Z -
				    Convert32_F16 (od->od_ZIdx) - HALF_F16);

		if (distq >= PROBEDISTQ)
			/*  Not close enough.  */
			break;

		state = od->ob.ob_State;
		if (state == OBS_CLOSING  ||  state == OBS_CLOSED) {
			if (od->od_Unlocked) {
opendoor:			od->ob.ob_State = OBS_OPENING;
				playsound (SFX_OPENDOOR);
			} else if (nkeys) {
				nkeys--;
				od->od_Unlocked = TRUE;
				goto opendoor;	// Look up;
			} else
				playsound (SFX_DOORLOCKED);
		}
		else if ((state == OBS_OPENING  ||  state == OBS_OPEN)  &&
			 (ConvertF16_32 (playerpos.X) != od->od_XIdx  ||
			  ConvertF16_32 (playerpos.Z) != od->od_ZIdx))
		{
			od->ob.ob_State = OBS_CLOSING;
			od->od_ME->me_Flags |= MEF_WALKSOLID | MEF_SHOTSOLID;
		}

		return (TRUE);
	 }
	}
	return (0);
}

void
toggledoor (ob)
struct Object	*ob;
{
	register ObNSDoor	*od;
	register int		state;

	od = (ObNSDoor *) ob;
	state = od->ob.ob_State;

	if (state == OBS_CLOSING  ||  state == OBS_CLOSED)
		od->ob.ob_State = OBS_OPENING;
	else if ((state == OBS_OPENING  ||  state == OBS_OPEN)  &&
		 (ConvertF16_32 (playerpos.X) != od->od_XIdx  ||
		  ConvertF16_32 (playerpos.Z) != od->od_ZIdx))
	{
		od->ob.ob_State = OBS_CLOSING;
		od->od_ME->me_Flags |= MEF_WALKSOLID | MEF_SHOTSOLID;
	}
}



/***************************************************************************
 * A very important static table.
 */
extern ObDef	def_Zombie;	// These are lies.
extern ObDef	def_George;
extern ObDef	def_Head;
extern ObDef	def_Spider;
extern ObDef	def_Powerup;
extern ObDef	def_Exit;
extern ObDef	def_Trigger;

ObDef	*obdefs[] = {
	(ObDef *) &def_Zombie,
	(ObDef *) &def_George,
	(ObDef *) &def_Head,
	(ObDef *) &def_Spider,
	(ObDef *) &def_NSDoor,
	(ObDef *) &def_Powerup,
	(ObDef *) &def_Exit,
	(ObDef *) &def_Trigger,
};
#define	NOBDEFS		(sizeof (obdefs) / sizeof (ObDef *))

int32	nobdefs = NOBDEFS;
