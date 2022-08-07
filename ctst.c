/*  :ts=8 bk=0
 *
 * ctst.c:	Refine grid box selection algorithms.
 *
 * Leo L. Schwab					9305.12
 */
#include <types.h>
#include <operamath.h>
#include <filefunctions.h>
#include <mem.h>
#include <event.h>
#include <time.h>
#include <graphics.h>
#include <audio.h>
#include <init3do.h>
#include <debug.h>
#include <string.h>

#include "castle.h"
#include "objects.h"
#include "imgfile.h"
#include "loaf.h"
#include "sound.h"
#include "flow.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define	CELSIZ		6
#define	XOFF		(320 / 2)
#define	YOFF		50

#define	NUNITVERTS	18


#define	ANGSTEP		ONE_F16
#define	QUARTER_F16	(ONE_F16 >> 2)

#define	BIGSTEP		(0x7FFFFFF - (128 << 16))

#define	DEFAULT_SCALE	0x24000


#define	JOYDIRS		(ControlLeft | ControlRight | ControlUp | ControlDown)
#define	JOYSHIFTBITS	(ControlStart | \
			 ControlLeftShift | ControlRightShift | JOYDIRS)
#define	JOYABC		(ControlA | ControlB | ControlC)

#define	NSCREENS	2	// Jon's title screen needs three (?!?!!)

#define	DAMAGEFADEDECAY	(ONE_F16 >> 6)

#define	SETBIT(a,n)	((a)[(n) >> 5] |= (1 << ((n) & 31)))


#define	MAX(a,b)	((a) > (b) ?  (a)  :  (b))
#define	MIN(a,b)	((a) < (b) ?  (a)  :  (b))


/***************************************************************************
 * Cell extraction data structure.
 */
typedef struct ExtDat {
	frac16	x, z,
		angl, angr,
		sinl, cosl,
		sinr, cosr,
		liml, limr,
		stepl, stepr;
	ubyte	minx, maxx,
		minz, maxz;
	ubyte	xcnt, zcnt;
	ubyte	chopcone;
} ExtDat;


/***************************************************************************
 * $(EXPLETIVE) forward references to placate $(EXPLETIVE) compiler.
 */
static void extract_north(struct ExtDat *ed);
static void extract_west(struct ExtDat *ed);
static void extract_south(struct ExtDat *ed);
static void extract_east(struct ExtDat *ed);


/***************************************************************************
 * Globals.
 */
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];
extern JoyData	jd;


/*
 * Unit square indexed as follows for no particular reason:
 *
 *  +Z
 *
 * 1.0	6---7---8
 *	|   |   |
 *	|   |   |
 * 0.5	3---4---5
 *	|   |   |
 *	|   |   |
 *  0	0---1---2
 *
 *	0  0.5 1.0  +X
 */
Vertex		unitsquare[NUNITVERTS] = {
	{ 0,		0,		0 },
	{ HALF_F16,	0,		0 },
	{ ONE_F16,	0,		0 },
	{ 0,		0,		HALF_F16 },
	{ HALF_F16,	0,		HALF_F16 },
	{ ONE_F16,	0,		HALF_F16 },
	{ 0,		0,		ONE_F16 },
	{ HALF_F16,	0,		ONE_F16 },
	{ ONE_F16,	0,		ONE_F16 },
	{ 0,		-ONE_F16,	0 },
	{ HALF_F16,	-ONE_F16,	0 },
	{ ONE_F16,	-ONE_F16,	0 },
	{ 0,		-ONE_F16,	HALF_F16 },
	{ HALF_F16,	-ONE_F16,	HALF_F16 },
	{ ONE_F16,	-ONE_F16,	HALF_F16 },
	{ 0,		-ONE_F16,	ONE_F16 },
	{ HALF_F16,	-ONE_F16,	ONE_F16 },
	{ ONE_F16,	-ONE_F16,	ONE_F16 }
};
Vector		unitvects[4] = {
	{ ONE_F16,	0,		0 },
	{ 0,		0,		ONE_F16 },
	{ -ONE_F16,	0,		0 },
	{ 0,		0,		-ONE_F16 }
};

Vector		plusx, plusz, minusx, minusz;
Vertex		xformsquare[NUNITVERTS];
Vector		xformvects[4];
Matrix		camera;


Vertex		xfverts[MAXWALLVERTS],		// Transformed wall vertices
		projverts[MAXWALLVERTS];	// Projected wall vertices
int16		grididxs[NGRIDPOINTS];

VisOb		visobs[MAXVISOBS];	// Visible rendered objects
VisOb		*curviso;		// Cached pointer to current entry.
uint32		vertsused[(NGRIDPOINTS + 31) >> 5];	// Bit array.
Vertex		obverts[NOBVERTS], xfobverts[NOBVERTS];
Vertex		*curobv;
int32		nobverts;

int32		nvisv, nviso;	// # of visverts, visobs;

Vertex		playerpos, campos;
frac16		playerdir;
int32		playerhealth;
int32		playerlives;
int32		gunpower;
int32		nkeys;
int32		score;
int32		xlifethresh;
int32		xlifeincr = 500000;

ImageEnv	*walliev;
ImageEntry	*wallimgs;
CelArray	*ca_zombie, *ca_gun, *ca_ray;
CCB		*guncel;
uint32		ccbextra;

int32		floorcolor, ceilingcolor;
frac16		damagefade;	/*  INVERTED!  */

frac16		scale = DEFAULT_SCALE;
int32		throttleshift = 4;

int32		cy = CY;

int8		skiptitle;
int8		laytest;
int8		practice;
int8		domusic = TRUE;
int8		dosfx = TRUE;
int8		exitedlevel;
int8		gottalisman;

Item		joythread;
Item		vblIO, sportIO;

char		*levelseqbuf;
char		**seqnames;
int32		nseq;
int32		level;	// jml


Item		sgrpitem;
RastPort	rports[NSCREENS];
RastPort	*rpvis = &rports[0],
		*rprend = &rports[1];
int32		wide, high;
int32		screensize, screenpages;

TagArg		scrtags[] = {
	CSG_TAG_SPORTBITS,	0,	/*  Gets filled in later.  */
	CSG_TAG_SCREENCOUNT,	(void *) NSCREENS,
	CSG_TAG_DONE,		0
};


/***************************************************************************
 * Strings
 */
char		defaultseq[] = "$progdir/LevelSequence";
char		*sequencefile = defaultseq;

char		levelname[] = "FloorPlan";

char		wallimagefile[80];
char		spoolmusicfile[80];


/***************************************************************************
 * Code.
 */
int
main (ac, av)
int	ac;
char	**av;
{
	register int	retval;

	/*
	 * Excuse me a minute while I misuse wallimagefile[]...
	 */
	GetDirectory (wallimagefile, sizeof (wallimagefile));
	CreateAlias ("progdir", wallimagefile);


	parseargs (ac, av);
	openstuff ();

	dotitle ();

	/*
	 * Stream ran out on its own; format variables and play game.
	 */
	score = 0;
	playerlives = 3;
	xlifethresh = xlifeincr;
	level = 0;

kprintf ("scale: 0x%lx\nthrottleshift: %ld\nxlifeincr: %ld\n",
	 scale, throttleshift, xlifeincr);

	while (1) {
		retval = dispoptionscreen (0);
		if (retval == FC_BUCKY_QUIT  ||  retval == FC_RESTART)
			goto doquit;	// Look down.

		resetjoydata ();

		switch (retval = gamemain ()) {
		case FC_DIED:
			playcpak ("$progdir/Streams/FinalDeath");
			break;

		case FC_COMPLETED:
			if (!practice)
				playcpak ("$progdir/Streams/Outro");
			break;

		case FC_RESTART:
		case FC_BUCKY_QUIT:
doquit:			closestuff ();
			kprintf ("Normal exit.\n");
			exit (retval);

		case FC_BUCKY_NEXTLEVEL:
			break;

		default:
			kprintf ("What does main() do with %d?\n", retval);
			break;
		}

		if (!practice) {
			DoHighScoreScr (score);
			showcredits ();
		}
	}
	/*  Ye Ghods, this looks so Pascalian...  Ack!  */
}


int
gamemain ()
{
	register int	retval;

	opengamestuff ();

	/*
	 * 'level' is a global set outside.
	 */
	while (level < nseq) {
		loadshard (); // jml
		ChangeDirectory (seqnames[level]);

		retval = levelmain ();
		switch (retval) {
		case FC_DIED:
			if (--playerlives <= 0)
				goto restart;	// Look down.
			else
				continue;	// Stay on same level.
			break;

		case FC_COMPLETED:
		case FC_BUCKY_NEXTLEVEL:
			if (level != nseq - 1  &&  !practice) {
				retval = dointerlev ();
				if (retval == FC_LOADGAME  ||
				    retval == FC_NEWGAME)
					continue;
				else if (retval == FC_RESTART)
					goto restart;	// Look down.
			}
			if (practice)
				goto restart;

			break;

		case FC_LOADGAME:
		case FC_NEWGAME:
			continue;

		case FC_BUCKY_QUIT:
		case FC_RESTART:
restart:		level = nseq;	// Force loop exit.
			break;
		}

		level++;
	}

	level = 0;
	closegamestuff ();
	ChangeDirectory ("$progdir");

	return (retval);
}



int
levelmain ()
{
	Matrix	tmpmat;
	Vector	trans;
	int32	startscore;
	int	retval;
	ubyte	preva, prevb, prevc, prevs, framecount;
	void	*tmp;

	retval = 0;
	playerhealth = gunpower = 100;
	startscore = score;

	playerpos.X = playerpos.Z = 16 << 16;
	playerpos.Y = -HALF_F16;
	playerdir = 0;
	damagefade = 0;
	nkeys = 0;
	exitedlevel = FALSE;
	gottalisman = FALSE;
	openlevelstuff ();
	fullstop ();

	damagefade = ONE_F16;	// Oh, I'm so ashamed...

	preva = prevb = prevc = 0;
	resetjoydata ();
	while (1) {
		if (exitedlevel  ||  playerhealth <= 0) {
			/*
			 * Cheesy hack; got to think about a real solution.
			 */
			if (playerhealth <= 0) {
				installclut (rpvis);
				installclut (rprend);
				if (playerlives > 1) {
					score = startscore;
					playerhealth = gunpower = 100;
					simpledeath ();
				}
				retval = FC_DIED;
			}
			if (exitedlevel)
				retval = FC_COMPLETED;
			break;
		}
		moveplayer (&playerpos, &playerdir);

		if (!prevs  &&  jd.jd_StartDown) {
			if (laytest) {
				retval = FC_BUCKY_NEXTLEVEL;
				break;
			} else {
				stopspoolsound (0);
				rendermessage (0, "Paused.");
				spoolsound (spoolmusicfile, 9999);
				jd.jd_StartDown = 1;
			}
		}
		prevs = jd.jd_StartDown;

		if (jd.jd_XDown) {
			if ((retval = attemptoptions ()) == FC_LOADGAME  ||
			    retval == FC_NEWGAME  ||
			    retval == FC_RESTART)
				break;
		}

		if (!preva  &&  jd.jd_ADown)
			shoot ();
		preva = jd.jd_ADown;

		if (!prevb  &&  jd.jd_BDown)
			probe ();
		prevb = jd.jd_BDown;

		if (!prevc  &&  jd.jd_CDown) {
			if ((retval = dostatmap ()) == FC_LOADGAME  ||
			    retval == FC_NEWGAME  ||
			    retval == FC_RESTART)
				break;
			jd.jd_CDown = 1;	// Force interlock.
		}
		prevc = jd.jd_CDown;

		framecount = jd.jd_FrameCount;
		resetjoydata ();
		moveobjects (framecount);
		cyclewalls (framecount);

		/*
		 * Process extra lives.
		 */
		if (score >= xlifethresh) {
			playerlives++;
			playsound (SFX_GRABLIFE);
			xlifethresh += xlifeincr;
		}

		/*
		 * Do fade back to normal from damage hit.
		 */
		if (damagefade) {
			if ((damagefade -= DAMAGEFADEDECAY * framecount) < 0)
				damagefade = 0;
			if (!damagefade)
				fadetolevel (rpvis, ONE_F16 - damagefade);
			fadetolevel (rprend, ONE_F16 - damagefade);
		}
		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);

		/*
		 * Transform unit vectors for maneuvering.
		 */
		newmat (&tmpmat);
		applyyaw (&tmpmat, &camera, playerdir);
		MulManyVec3Mat33_F16 ((VECTCAST) xformvects,
				      (VECTCAST) unitvects,
				      (MATCAST) &camera,
				      4);

		/*
		 * Pull camera back 0.5 units from player position.
		 * Transform points for display.
		 */
		applyyaw (&tmpmat, &camera, -playerdir);
		campos.X = playerpos.X + (xformvects[3].X >> 1);
		campos.Y = playerpos.Y + (xformvects[3].Y >> 1);
		campos.Z = playerpos.Z + (xformvects[3].Z >> 1);

		trans.X = -campos.X;
		trans.Y = -campos.Y;
		trans.Z = -campos.Z;
		copyverts (unitsquare, xformsquare, NUNITVERTS);
		translatemany (&trans, xformsquare, NUNITVERTS);
		MulManyVec3Mat33_F16 ((VECTCAST) xformsquare,
				      (VECTCAST) xformsquare,
				      (MATCAST) &camera,
				      NUNITVERTS);



		minusx.X = -(plusx.X = xformsquare[2].X - xformsquare[0].X);
		minusx.Y = plusx.Y = 0;
		minusx.Z = -(plusx.Z = xformsquare[2].Z - xformsquare[0].Z);

		minusz.X = -(plusz.X = xformsquare[6].X - xformsquare[0].X);
		minusz.Y = plusz.Y = 0;
		minusz.Z = -(plusz.Z = xformsquare[6].Z - xformsquare[0].Z);

		extractcels (campos.X, campos.Z, playerdir);
	}

	closelevelstuff ();

	return (retval);
}


/***************************************************************************
 * Player movement.
 */
static int32	zthrottle, xthrottle, athrottle;


void
moveplayer (ppos, vang)
struct Vertex	*ppos;
frac16		*vang;
{
	register int32	nframes;
	register int32	maxthrottle;
	Vector		trans;

	trans.X = trans.Y = trans.Z = 0;
	nframes = jd.jd_FrameCount;
	maxthrottle = 1 << throttleshift;

#if 0
	/*
	 * This is the original "PosiDrive" version.
	 */
	*vang += ANGSTEP * jd.jd_DAng;

	trans.X = xformvects[1].X * jd.jd_DZ  +  xformvects[0].X * jd.jd_DX;
	trans.Y = xformvects[1].Y * jd.jd_DZ  +  xformvects[0].Y * jd.jd_DX;
	trans.Z = xformvects[1].Z * jd.jd_DZ  +  xformvects[0].Z * jd.jd_DX;

	jd.jd_DX = jd.jd_DZ = jd.jd_DAng = 0;

	trans.X >>= 4;
	trans.Y >>= 4;
	trans.Z >>= 4;

	moveposition (ppos, &trans, NULL, TRUE, TRUE);
#endif


	/*
	 * Adjust throttles.  (Slush-O-Vision!)
	 */
	zthrottle += ConvertF16_32 (jd.jd_DZ * scale);
	if (zthrottle > maxthrottle)		zthrottle = maxthrottle;
	else if (zthrottle < -maxthrottle)	zthrottle = -maxthrottle;

	xthrottle += ConvertF16_32 (jd.jd_DX * scale);
	if (xthrottle > maxthrottle)		xthrottle = maxthrottle;
	else if (xthrottle < -maxthrottle)	xthrottle = -maxthrottle;

	athrottle += ConvertF16_32 (jd.jd_DAng * scale);
	if (athrottle > maxthrottle)		athrottle = maxthrottle;
	else if (athrottle < -maxthrottle)	athrottle = -maxthrottle;

	/*
	 * Compute thrust and direction.
	 */
	*vang += (ANGSTEP * athrottle * nframes) >> throttleshift;

	trans.X = xformvects[1].X * zthrottle * nframes +
		  xformvects[0].X * xthrottle * nframes;
	trans.Y = xformvects[1].Y * zthrottle * nframes +
		  xformvects[0].Y * xthrottle * nframes;
	trans.Z = xformvects[1].Z * zthrottle * nframes +
		  xformvects[0].Z * xthrottle * nframes;

	jd.jd_DX = jd.jd_DZ = jd.jd_DAng = 0;

	/*
	 * Dampen throttles.
	 */
	if (zthrottle > 0) {
		if ((zthrottle -= nframes) < 0)	zthrottle = 0;
	} else
		if ((zthrottle += nframes) > 0)	zthrottle = 0;

	if (xthrottle > 0) {
		if ((xthrottle -= nframes) < 0)	xthrottle = 0;
	} else
		if ((xthrottle += nframes) > 0)	xthrottle = 0;

	if (athrottle > 0) {
		if ((athrottle -= nframes) < 0)	athrottle = 0;
	} else
		if ((athrottle += nframes) > 0)	athrottle = 0;

	trans.X >>= throttleshift + 4;
	trans.Y >>= throttleshift + 4;
	trans.Z >>= throttleshift + 4;

	moveposition (ppos, &trans, NULL, TRUE, TRUE);
}

void
fullstop ()
{
	/*
	 * This pulls the throttles out, halting drift.  User movement
	 * is still possible.  This is primarily to keep from having an
	 * initial "push" at level start.
	 */
	zthrottle = xthrottle = athrottle = 0;
}

void
recoil ()
{
	if ((zthrottle -= 1 << (throttleshift - 1)) > -1)
		zthrottle = -1;
}



int
moveposition (ppos, trans, ignoreob, checkobs, isplayer)
struct Vertex	*ppos;
struct Vector	*trans;
struct Object	*ignoreob;
int		checkobs, isplayer;
{
#define	PLAYERAD	(3 * ONE_F16 / 8)

	static int		xoffs[] = { 1, -1, 1, -1, 0, 1, -1, 0 };
	static int		zoffs[] = { 1, 1, -1, -1, 1, 0, 0, -1 };
	register MapEntry	*me;
	register Object		*ob;
	register int32		n, xv, zv;
	Object			*next;
	PathBox			pb;
	BBox			celb;
	Vertex			newpos;
	int32			x, z, redo;
	int			passflags;

	/*
	 * Cheap HACK to defend against falling through walls.
	 * (Dammit, I'd thought I'd *SOLVED* this!)
	 */
	if (trans->X > ONE_F16)		trans->X = ONE_F16;
	else if (trans->X < -ONE_F16)	trans->X = -ONE_F16;

	if (trans->Z > ONE_F16)		trans->Z = ONE_F16;
	else if (trans->Z < -ONE_F16)	trans->Z = -ONE_F16;

	/*
	 * Construct PathBox.
	 */
	pb.Start.MinX = ppos->X - PLAYERAD;
	pb.Start.MaxX = ppos->X + PLAYERAD;
	pb.Start.MinZ = ppos->Z - PLAYERAD;
	pb.Start.MaxZ = ppos->Z + PLAYERAD;
	pb.DX = trans->X;
	pb.DZ = trans->Z;

	newpos.X = ppos->X + trans->X;
	newpos.Y = ppos->Y + trans->Y;
	newpos.Z = ppos->Z + trans->Z;

	pb.End.MinX = newpos.X - PLAYERAD;
	pb.End.MaxX = newpos.X + PLAYERAD;
	pb.End.MinZ = newpos.Z - PLAYERAD;
	pb.End.MaxZ = newpos.Z + PLAYERAD;
	genpathbox (&pb.Path, &pb.Start, &pb.End);

	x = ConvertF16_32 (ppos->X);
	z = ConvertF16_32 (ppos->Z);

	passflags = OBF_CONTACT;
	if (!isplayer)
		passflags |= OBF_PLAYERONLY;

	redo = 0;
	for (n = 8;  --n >= 0; ) {
		xv = x + xoffs[n];
		zv = z + zoffs[n];
		me = &levelmap[zv][xv];

		if (me->me_Flags & MEF_WALKSOLID) {
			celb.MaxX = (celb.MinX = Convert32_F16 (xv)) +
				    ONE_F16;
			celb.MaxZ = (celb.MinZ = Convert32_F16 (zv)) +
				    ONE_F16;

			redo += checkcontact (&pb, &celb, TRUE);
		}

		if (checkobs  &&  (ob = me->me_Obs)) {
			while (ob) {
				next = ob->ob_Next;
				if (ob != ignoreob  &&
				    (ob->ob_Flags & passflags) ==
				     OBF_CONTACT)
					redo += (ob->ob_Def->od_Func)
						 (ob, OP_CONTACT, &pb);
				ob = next;
			}
		}
	}
	me = &levelmap[z][x];
	if (checkobs  &&  (ob = me->me_Obs)) {
		while (ob) {
			next = ob->ob_Next;
			if (ob != ignoreob  &&
			    (ob->ob_Flags & passflags) == OBF_CONTACT)
				redo += (ob->ob_Def->od_Func)
					 (ob, OP_CONTACT, &pb);
			ob = next;
		}
	}

	if (redo) {
		ppos->X = pb.End.MinX + PLAYERAD;
		ppos->Z = pb.End.MinZ + PLAYERAD;
	} else
		*ppos = newpos;

	return (redo);
}


/*
 * Not a complete solution, but should be enough for this application.
 * (Thanx, -=RJ=-.)
 */
int
checkcontact (pb, bb, block)
register struct PathBox	*pb;
register struct BBox	*bb;
int			block;
{
	/*
	 * Trivial rejection.
	 */
	if (pb->Path.MinX >= bb->MaxX  ||  pb->Path.MaxX <= bb->MinX  ||
	    pb->Path.MinZ >= bb->MaxZ  ||  pb->Path.MaxZ <= bb->MinZ)
		return (FALSE);

	/*
	 * Trivial acceptance.
	 */
	if (pb->Start.MinX < bb->MaxX  &&  pb->Start.MaxX > bb->MinX  &&
	    pb->Start.MinZ < bb->MaxZ  &&  pb->Start.MaxZ > bb->MinZ)
		/*
		 * Impossible to block in this case, so we ignore it and
		 * cross our fingers.
		 */
		return (TRUE);

	if (pb->End.MinX < bb->MaxX  &&  pb->End.MaxX > bb->MinX  &&
	    pb->End.MinZ < bb->MaxZ  &&  pb->End.MaxZ > bb->MinZ)
	{
		if (block)
			blockpath (pb, bb);
		return (TRUE);
	}

	/*
	 * Oh, gonna make it tough, are ya?
	 * WARNING!  Some of this is known to be broken.  More work needed...
	 */
	if (pb->DX * pb->DZ > 0) {
		register frac16	boxdx, boxdz;

		if (pb->Path.MinZ < bb->MinZ) {
			boxdx = bb->MaxX - pb->Start.MinX;
			boxdz = DivSF16 (MulSF16 (pb->DZ, boxdx), pb->DX);
			if (pb->Start.MaxZ + boxdz > bb->MinZ) {
				if (block)
					blockpath (pb, bb);
				return (TRUE);
			}
		} else {
			boxdx = bb->MinX - pb->Start.MaxX;
			boxdz = DivSF16 (MulSF16 (pb->DZ, boxdx), pb->DX);
			if (pb->Start.MinZ + boxdz < bb->MaxZ) {
				if (block)
					blockpath (pb, bb);
				return (TRUE);
			}
		}
	} else {
		register frac16	boxdx, boxdz;

		if (pb->Path.MinZ < bb->MinZ) {
			boxdx = bb->MinX - pb->Start.MaxX;
			boxdz = DivSF16 (MulSF16 (pb->DZ, boxdx), pb->DX);
			if (pb->Start.MaxZ + boxdz > bb->MinZ) {
				if (block)
					blockpath (pb, bb);
				return (TRUE);
			}
		} else {
			boxdx = bb->MaxX - pb->Start.MinX;
			boxdz = DivSF16 (MulSF16 (pb->DZ, boxdx), pb->DX);
			if (pb->Start.MinZ + boxdz < bb->MaxZ) {
				if (block)
					blockpath (pb, bb);
				return (TRUE);
			}
		}
	}
	return (FALSE);		// And after all that work...
}


void
blockpath (pb, obst)
register struct PathBox	*pb;
register struct BBox	*obst;
{
	register frac16	px, pz,
			bx, bz;
	frac16		box, path;

	if (pb->DX >= 0) {
		px = pb->End.MaxX;
		bx = obst->MinX;
	} else {
		px = pb->End.MinX;
		bx = obst->MaxX;
	}
	if (pb->DZ >= 0) {
		pz = pb->End.MaxZ;
		bz = obst->MinZ;
	} else {
		pz = pb->End.MinZ;
		bz = obst->MaxZ;
	}

	/*
	 * Cross-multiply, take absolute value, and compare results.
	 * (REMARKABLE PROOF!  See my notes...)
	 */
	if ((box = MulSF16 (pz - bz, pb->DX)) < 0)
		box = -box;
	if ((path = MulSF16 (px - bx, pb->DZ)) < 0)
		path = -path;

	if (box > path) {
		/*
		 * Obstruct in X.
		 */
		bx -= px;
		pb->End.MinX += bx;
		pb->End.MaxX += bx;
		goto regen;	// Look down.
	} else {
		/*
		 * Obstruct in Z.
		 */
		bz -= pz;
		pb->End.MinZ += bz;
		pb->End.MaxZ += bz;
regen:		genpathbox (&pb->Path, &pb->Start, &pb->End);
		regenpath (pb);
	}
}



void
genpathbox (pathbox, fred, barney)
register struct BBox	*pathbox, *fred, *barney;
{
	if (fred->MinX < barney->MinX)	pathbox->MinX = fred->MinX;
	else				pathbox->MinX = barney->MinX;

	if (fred->MinZ < barney->MinZ)	pathbox->MinZ = fred->MinZ;
	else				pathbox->MinZ = barney->MinZ;

	if (fred->MaxX > barney->MaxX)	pathbox->MaxX = fred->MaxX;
	else				pathbox->MaxX = barney->MaxX;

	if (fred->MaxZ > barney->MaxZ)	pathbox->MaxZ = fred->MaxZ;
	else				pathbox->MaxZ = barney->MaxZ;
}

void
regenpath (pathbox)
register struct PathBox	*pathbox;
{
	pathbox->DX = pathbox->End.MinX - pathbox->Start.MinX;
	pathbox->DZ = pathbox->End.MinZ - pathbox->Start.MinZ;
}





/***************************************************************************
 * Visibility extractor.  Also calls renderer.
 */
void
extractcels (x, z, dir)
frac16	x, z, dir;
{
	register Vertex	*vert;
	register int	i;
	GrafCon	gc;
	ExtDat	ed;
struct timeval	time1, time2;
int32		timeext, timeproc, timerend;

	ed.x = x;
	ed.z = z;

	ed.angl = (dir + (32 << 16)) & 0xFFFFFF;
	ed.angr = (dir - (32 << 16)) & 0xFFFFFF;

	ed.sinl = SinF16 (ed.angl);
	ed.cosl = CosF16 (ed.angl);

	ed.sinr = SinF16 (ed.angr);
	ed.cosr = CosF16 (ed.angr);



	clearvertsused ();
	curviso = visobs;
	curobv = obverts;
	nvisv = nviso = nobverts = 0;

	gettime (&time1);
	switch (ed.angl >> 22) {
	case 0:
		extract_north (&ed);
		break;
	case 1:
		extract_west (&ed);
		break;
	case 2:
		extract_south (&ed);
		break;
	case 3:
		extract_east (&ed);
		break;
	}
	gettime (&time2);
	timeext = subtime (&time2, &time1);

	processgrid ();
	processvisobs ();
	gettime (&time1);
	timeproc = subtime (&time1, &time2);

	WaitVBL (vblIO, 1);
	clearscreen (rprend);
//	SetRast (rprend, 0);
	rendercels ();
	gettime (&time2);
	timerend = subtime (&time2, &time1);

#if 0
	drawnumxy (rprend->rp_BitmapItem, timeext, 100, 50);
	drawnumxy (rprend->rp_BitmapItem, timeproc, 100, 60);
	drawnumxy (rprend->rp_BitmapItem, timerend, 100, 70);

	SetFGPen (&gc, MakeRGB15 (31, 31, 31));
	vert = xfverts;
	for (i = 0;  i < nvisv;  i++) {
		WritePixel (rprend->rp_BitmapItem, &gc,
			    160 + ConvertF16_32 (vert->X * 6),
			    200 - ConvertF16_32 (vert->Z * 6));
		vert += 2;
	}
#endif
}



static void
extract_north (ed)
struct ExtDat	*ed;
{
	register VisOb	*vo;
	register int32	i, l, r, vidx, wvidx;
	register int32	x, z, zcnt;
	register frac16	stepl, stepr, liml, limr;
	MapEntry	*me;
	int32		prevl, prevr, oq;
	int32		stopz, stopxl, stopxr;
	int		chopcone;

	if (!ed->cosl)	stepl = -BIGSTEP;
	else		stepl = -DivSF16 (ed->sinl, ed->cosl);	// Tangent
	if (!ed->cosr)	stepr = BIGSTEP;
	else		stepr = -DivSF16 (ed->sinr, ed->cosr);

	z = ONE_F16 - F_FRAC (ed->z);	// Temporary use...
	liml = ed->x + MulSF16 (stepl, z);
	limr = ed->x + MulSF16 (stepr, z);

	chopcone = TRUE;
	if (z <= ZCLIP)
		/*
		 * Camera is in a wall; don't permit this to chop viewcone.
		 */
		chopcone = FALSE;

	x = ConvertF16_32 (ed->x);
	z = ConvertF16_32 (ed->z);
	prevl = prevr = x;

	stopz = z + GRIDCUTOFF;
	if (stopz > WORLDSIZ)	stopz = WORLDSIZ;
	stopxl = x - GRIDCUTOFF;
	if (stopxl < 0)		stopxl = 0;
	stopxr = x + GRIDCUTOFF;
	if (stopxr >= WORLDSIZ)	stopxr = WORLDSIZ - 1;

	/*
	 * Determine visible cels and left/right limits.
	 */
	vidx = z * (WORLDSIZ + 1) + x;
	vo = visobs;
	for (zcnt = z;  zcnt < stopz;  zcnt++) {
		if (liml < 0)
			liml = 0;
		if (limr >= WORLDSIZ_F16)
			limr = WORLDSIZ_F16 - 1;

		if ((l = ConvertF16_32 (liml)) < stopxl)	l = stopxl;
		if ((r = ConvertF16_32 (limr)) > stopxr)	r = stopxr;

		/*
		 * Write visible cels on right.
		 */
		wvidx = vidx;
		oq = -1;
		me = &levelmap[zcnt][x];
		for (i = x;  i <= r;  i++, wvidx++, me++) {
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq <= prevr)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (i != x  &&  (me->me_VisFlags & VISF_WEST)) {
				/*
				 * Process west face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + WORLDSIZ + 1));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_WEST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_SOUTH) {
				/*
				 * Process south face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= (i == x)  ?
						  VISF_SOUTH  :
						  VISF_SOUTH | VISF_WEST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute right edge limit and slope.
			 */
			stepr = DivSF16 (Convert32_F16 (oq) - ed->x,
					 Convert32_F16 (zcnt + 1) - ed->z);
			limr = Convert32_F16 (oq);
		}



		/*
		 * Write visible cels on left.
		 */
		wvidx = vidx - 1;
		oq = -1;
		me = &levelmap[zcnt][x - 1];
		for (i = x - 1;  i >= l;  i--, wvidx--, me--) {
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq >= prevl)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (me->me_VisFlags & VISF_EAST) {
				/*
				 * Process east face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + 1 + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_EAST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_SOUTH) {
				/*
				 * Process south face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH | VISF_EAST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute left edge limit and slope.
			 */
			stepl = DivSF16 (Convert32_F16 (oq + 1) - ed->x,
					 Convert32_F16 (zcnt + 1) - ed->z);
			liml = Convert32_F16 (oq + 1);
		}

		prevl = l;
		prevr = r;
		liml += stepl;
		limr += stepr;
		vidx += WORLDSIZ + 1;
		chopcone = TRUE;
	}
}


static void
extract_west (ed)
struct ExtDat	*ed;
{
	register VisOb	*vo;
	register int32	i, l, r, vidx, wvidx;
	register int32	x, z, xcnt;
	register frac16	stepl, stepr, liml, limr;
	MapEntry	*me;
	int32		prevl, prevr, oq;
	int32		stopx, stopzl, stopzr;
	int		chopcone;

	if (!ed->sinl)	stepl = -BIGSTEP;
	else		stepl = DivSF16 (ed->cosl, ed->sinl);	// Inverse tangent
	if (!ed->sinr)	stepr = BIGSTEP;
	else		stepr = DivSF16 (ed->cosr, ed->sinr);

	x = F_FRAC (ed->x);	// Temporary use...
	liml = ed->z + MulSF16 (stepl, x);
	limr = ed->z + MulSF16 (stepr, x);

	chopcone = TRUE;
	if (x <= ZCLIP)
		/*
		 * Camera is in a wall; don't permit this to chop viewcone.
		 */
		chopcone = FALSE;

	x = ConvertF16_32 (ed->x);
	z = ConvertF16_32 (ed->z);
	prevl = prevr = z;

	stopx = x - GRIDCUTOFF;
	if (stopx < 0)		stopx = 0;
	stopzl = z - GRIDCUTOFF;
	if (stopzl < 0)		stopzl = 0;
	stopzr = z + GRIDCUTOFF;
	if (stopzr >= WORLDSIZ)	stopzr = WORLDSIZ - 1;

	/*
	 * Determine visible cels and left/right limits.
	 */
	vidx = z * (WORLDSIZ + 1) + x;
	vo = visobs;
	for (xcnt = x;  xcnt >= stopx;  xcnt--) {
		if (liml < 0)
			liml = 0;
		if (limr >= WORLDSIZ_F16)
			limr = WORLDSIZ_F16 - 1;

		if ((l = ConvertF16_32 (liml)) < stopzl)	l = stopzl;
		if ((r = ConvertF16_32 (limr)) > stopzr)	r = stopzr;

		/*
		 * Write visible cels on right.
		 */
		wvidx = vidx;
		oq = -1;
		me = &levelmap[z][xcnt];
		for (i = z;
		     i <= r;
		     i++, wvidx += WORLDSIZ + 1, me += WORLDSIZ)
		{
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq <= prevr)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (i != z  &&  (me->me_VisFlags & VISF_SOUTH)) {
				/*
				 * Process south face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_EAST) {
				/*
				 * Process east face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + 1 + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_EAST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= (i == z)  ?
						  VISF_EAST  :
						  VISF_SOUTH | VISF_EAST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute right edge limit and slope.
			 */
			stepr = DivSF16 (Convert32_F16 (oq) - ed->z,
					 ed->x - Convert32_F16 (xcnt));
			limr = Convert32_F16 (oq);
		}



		/*
		 * Write visible cels on left.
		 */
		wvidx = vidx - (WORLDSIZ + 1);
		oq = -1;
		me = &levelmap[z - 1][xcnt];
		for (i = z - 1;
		     i >= l;
		     i--, wvidx -= WORLDSIZ + 1, me -= WORLDSIZ)
		{
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq >= prevl)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (me->me_VisFlags & VISF_NORTH) {
				/*
				 * Process north face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + 1 + WORLDSIZ + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_EAST) {
				/*
				 * Process east face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + 1 + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_EAST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH | VISF_EAST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute left edge limit and slope.
			 */
			stepl = DivSF16 (Convert32_F16 (oq + 1) - ed->z,
					 ed->x - Convert32_F16 (xcnt));
			liml = Convert32_F16 (oq + 1);
		}

		prevl = l;
		prevr = r;
		liml += stepl;
		limr += stepr;
		vidx--;
		chopcone = TRUE;
	}
}


static void
extract_south (ed)
struct ExtDat	*ed;
{
	register VisOb	*vo;
	register int32	i, l, r, vidx, wvidx;
	register int32	x, z, zcnt;
	register frac16	stepl, stepr, liml, limr;
	MapEntry	*me;
	int32		prevl, prevr, oq;
	int32		stopz, stopxl, stopxr;
	int		chopcone;

	if (!ed->cosl)	stepl = BIGSTEP;
	else		stepl = DivSF16 (ed->sinl, ed->cosl);	// Tangent
	if (!ed->cosr)	stepr = -BIGSTEP;
	else		stepr = DivSF16 (ed->sinr, ed->cosr);

	z = F_FRAC (ed->z);	// Temporary use...
	liml = ed->x + MulSF16 (stepl, z);
	limr = ed->x + MulSF16 (stepr, z);

	chopcone = TRUE;
	if (z <= ZCLIP)
		/*
		 * Camera is in a wall; don't permit this to chop viewcone.
		 */
		chopcone = FALSE;

	x = ConvertF16_32 (ed->x);
	z = ConvertF16_32 (ed->z);
	prevl = prevr = x;

	stopz = z - GRIDCUTOFF;
	if (stopz < 0)		stopz = 0;
	stopxl = x + GRIDCUTOFF;
	if (stopxl >= WORLDSIZ)	stopxl = WORLDSIZ - 1;
	stopxr = x - GRIDCUTOFF;
	if (stopxr < 0)		stopxr = 0;

	/*
	 * Determine visible cels and left/right limits.
	 */
	vidx = z * (WORLDSIZ + 1) + x;
	vo = visobs;
	for (zcnt = z;  zcnt >= stopz;  zcnt--) {
		if (liml >= WORLDSIZ_F16)
			liml = WORLDSIZ_F16 - 1;
		if (limr < 0)
			limr = 0;

		if ((l = ConvertF16_32 (liml)) > stopxl)	l = stopxl;
		if ((r = ConvertF16_32 (limr)) < stopxr)	r = stopxr;

		/*
		 * Write visible cels on right.
		 */
		wvidx = vidx;
		oq = -1;
		me = &levelmap[zcnt][x];
		for (i = x;  i >= r;  i--, wvidx--, me--) {
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq >= prevr)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (i != x  &&  (me->me_VisFlags & VISF_EAST)) {
				/*
				 * Process east face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + 1 + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_EAST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_NORTH) {
				/*
				 * Process north face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + 1 + WORLDSIZ + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= (i == x)  ?
						  VISF_NORTH  :
						  VISF_NORTH | VISF_EAST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute right edge limit and slope.
			 */
			stepr = DivSF16 (Convert32_F16 (oq + 1) - ed->x,
					 ed->z - Convert32_F16 (zcnt));
			limr = Convert32_F16 (oq + 1);
		}



		/*
		 * Write visible cels on left.
		 */
		wvidx = vidx + 1;
		oq = -1;
		me = &levelmap[zcnt][x + 1];
		for (i = x + 1;  i <= l;  i++, wvidx++, me++) {
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq <= prevl)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (me->me_VisFlags & VISF_WEST) {
				/*
				 * Process west face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + WORLDSIZ + 1));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_WEST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_NORTH) {
				/*
				 * Process north face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + 1 + WORLDSIZ + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH | VISF_WEST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute left edge limit and slope.
			 */
			stepl = DivSF16 (Convert32_F16 (oq) - ed->x,
					 ed->z - Convert32_F16 (zcnt));
			liml = Convert32_F16 (oq);
		}

		prevl = l;
		prevr = r;
		liml += stepl;
		limr += stepr;
		vidx -= WORLDSIZ + 1;
		chopcone = TRUE;
	}
}

static void
extract_east (ed)
struct ExtDat	*ed;
{
	register VisOb	*vo;
	register int32	i, l, r, vidx, wvidx;
	register int32	x, z, xcnt;
	register frac16	stepl, stepr, liml, limr;
	MapEntry	*me;
	int32		prevl, prevr, oq;
	int32		stopx, stopzl, stopzr;
	int		chopcone;

	if (!ed->sinl)	stepl = BIGSTEP;
	else		stepl = -DivSF16 (ed->cosl, ed->sinl);	// Inverse tangent
	if (!ed->sinr)	stepr = -BIGSTEP;
	else		stepr = -DivSF16 (ed->cosr, ed->sinr);

	x = ONE_F16 - F_FRAC (ed->x);	// Temporary use...
	liml = ed->z + MulSF16 (stepl, x);
	limr = ed->z + MulSF16 (stepr, x);

	chopcone = TRUE;
	if (x <= ZCLIP)
		/*
		 * Camera is in a wall; don't permit this to chop viewcone.
		 */
		chopcone = FALSE;

	x = ConvertF16_32 (ed->x);
	z = ConvertF16_32 (ed->z);
	prevl = prevr = z;

	stopx = x + GRIDCUTOFF;
	if (stopx > WORLDSIZ)	stopx = WORLDSIZ;
	stopzl = z + GRIDCUTOFF;
	if (stopzl >= WORLDSIZ)	stopzl = WORLDSIZ - 1;
	stopzr = z - GRIDCUTOFF;
	if (stopzr < 0)		stopzr = 0;

	/*
	 * Determine visible cels and left/right limits.
	 */
	vidx = z * (WORLDSIZ + 1) + x;
	vo = visobs;
	for (xcnt = x;  xcnt < stopx;  xcnt++) {
		if (liml >= WORLDSIZ_F16)
			liml = WORLDSIZ_F16 - 1;
		if (limr < 0)
			limr = 0;

		if ((l = ConvertF16_32 (liml)) > stopzl)	l = stopzl;
		if ((r = ConvertF16_32 (limr)) < stopzr)	r = stopzr;

		/*
		 * Write visible cels on right.
		 */
		wvidx = vidx;
		oq = -1;
		me = &levelmap[z][xcnt];
		for (i = z;
		     i >= r;
		     i--, wvidx -= WORLDSIZ + 1, me -= WORLDSIZ)
		{
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq >= prevr)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (i != z  &&  (me->me_VisFlags & VISF_NORTH)) {
				/*
				 * Process north face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + 1 + WORLDSIZ + 1));
				SETBIT (vertsused,
					(vo->vo_RIdx = wvidx + WORLDSIZ + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_NORTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_WEST) {
				/*
				 * Process west face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + WORLDSIZ + 1));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_WEST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= (i == z)  ?
						  VISF_WEST  :
						  VISF_NORTH | VISF_WEST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute right edge limit and slope.
			 */
			stepr = DivSF16 (Convert32_F16 (oq + 1) - ed->z,
					 Convert32_F16 (xcnt + 1) - ed->x);
			limr = Convert32_F16 (oq + 1);
		}



		/*
		 * Write visible cels on left.
		 */
		wvidx = vidx + (WORLDSIZ + 1);
		oq = -1;
		me = &levelmap[z + 1][xcnt];
		for (i = z + 1;
		     i <= l;
		     i++, wvidx += WORLDSIZ + 1, me += WORLDSIZ)
		{
			if (me->me_Flags & MEF_OPAQUE) {
				if (oq < 0)
					oq = i;
			} else if (oq <= prevl)
				oq = -1;

			if (!(me->me_Flags & MEF_ARTWORK)  &&  !me->me_Obs)
				continue;

			if (me->me_VisFlags & VISF_SOUTH) {
				/*
				 * Process south face.
				 */
				SETBIT (vertsused, (vo->vo_LIdx = wvidx));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx + 1));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH;
				vo->vo_ImgIdx	= me->me_NSImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_VisFlags & VISF_WEST) {
				/*
				 * Process west face.
				 */
				SETBIT (vertsused,
					(vo->vo_LIdx = wvidx + WORLDSIZ + 1));
				SETBIT (vertsused, (vo->vo_RIdx = wvidx));
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_WEST;
				vo->vo_ImgIdx	= me->me_EWImage;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}

			if (me->me_Obs) {
				/*
				 * Process objects
				 */
				vo->vo_LIdx	=
				vo->vo_RIdx	= -1;
				vo->vo_MEFlags	= me->me_Flags;
				vo->vo_VisFlags	= VISF_SOUTH | VISF_WEST;
				vo->vo_ME	= me;
				vo++;
				nviso++;
			}
		}

		if (oq >= 0  &&  chopcone) {
			/*
			 * Recompute left edge limit and slope.
			 */
			stepl = DivSF16 (Convert32_F16 (oq) - ed->z,
					 Convert32_F16 (xcnt + 1) - ed->x);
			liml = Convert32_F16 (oq);
		}

		prevl = l;
		prevr = r;
		liml += stepl;
		limr += stepr;
		vidx++;
		chopcone = TRUE;
	}
}




void
clearvertsused ()
{
	register uint32	*zap;
	register int	i;

	for (i = sizeof (vertsused) / sizeof (uint32), zap = vertsused;
	     --i >= 0;
	    )
		*zap++ = 0;
}




/***************************************************************************
 * 3D rotation.
 */
void
applyyaw (mat, newmat, angle)
struct Matrix	*mat, *newmat;
frac16		angle;
{
	register frac16	sin, cos;
	Matrix		tmpmat;

	sin = SinF16 (angle);
	cos = CosF16 (angle);

	tmpmat.X0 = cos;
	tmpmat.Y0 = 0;
	tmpmat.Z0 = sin;

	tmpmat.X1 = 0;
	tmpmat.Y1 = ONE_F16;
	tmpmat.Z1 = 0;

	tmpmat.X2 = -sin;
	tmpmat.Y2 = 0;
	tmpmat.Z2 = cos;

	MulMat33Mat33_F16 ((MATCAST) newmat, (MATCAST) mat, (MATCAST) &tmpmat);
}

void
newmat (mat)
struct Matrix *mat;
{
	mat->X0 = ONE_F16;	mat->Y0 = 0;		mat->Z0 = 0;
	mat->X1 = 0;		mat->Y1 = ONE_F16;	mat->Z1 = 0;
	mat->X2 = 0;		mat->Y2 = 0;		mat->Z2 = ONE_F16;
}

void
translatemany (transvec, verts, n)
struct Vector	*transvec;
struct Vertex	*verts;
int32		n;
{
	while (--n >= 0) {
		verts->X += transvec->X;
		verts->Y += transvec->Y;
		verts->Z += transvec->Z;
		verts++;
	}
}

void
copyverts (src, dest, n)
struct Vertex	*src, *dest;
register int32	n;
{
	register frac16	*fsrc, *fdest;

	fsrc = (frac16 *) src;
	fdest = (frac16 *) dest;
	n *= sizeof (Vertex) / sizeof (frac16);

	while (--n >= 0)
		*fdest++ = *fsrc++;
}




/***************************************************************************
 * Parse arguments.
 */
void
parseargs (ac, av)
int	ac;
char	**av;
{
	register char	*arg;
	int32		val;

	while (++av, --ac) {
		arg = *av;
		if (*arg == '-') {
			switch (*(arg+1)) {
			case 'l':
			case 'L':
				sequencefile = arg + 2;
				break;

			case 't':
			case 'T':
				skiptitle = TRUE;
				break;

			case 'a':
			case 'A':
				/*  Set acceleration.  */
				if ((val = strtol (arg + 2, NULL, 0)) > 0)
					scale = val;
				break;

			case 's':
			case 'S':
				/*  Set slushiness.  */
				if ((val = strtol (arg + 2, NULL, -1)) >= 0)
					throttleshift = val;
				break;

			case 'x':
			case 'X':
				/*  Set extra life increment.  */
				if ((val = strtol (arg + 2, NULL, 0)) > 0)
					xlifeincr = val;
				break;

			case 'c':
			case 'C':
				/*  Set "cheat" layout testing mode.  */
				laytest = TRUE;
kprintf ("You are in LAYTEST mode.  PAUSE button advances level.\n");
				break;

			default:
				kprintf ("Unknown option: %s\n", arg);
			}
		}
	}
}


/***************************************************************************
 * A more pleasant front-end to SetVRAMPages().
 */
void
SetRast (rp, val)
struct RastPort	*rp;
int32		val;
{
	SetVRAMPages
	 (sportIO, rp->rp_Bitmap->bm_Buffer, val, screenpages, ~0L);
}

void
CopyRast (src, dest)
struct RastPort	*src, *dest;
{
	CopyVRAMPages
	 (sportIO,
	  dest->rp_Bitmap->bm_Buffer, src->rp_Bitmap->bm_Buffer,
	  screenpages, ~0L);
}


/***************************************************************************
 * Clears the screen in a two-tone manner.
 * (Dave said the VRAM page size would never change.  Was he lying?
 * We're about to find out...  The Hard Way!)
 */
int
clearscreen (rp)
struct RastPort	*rp;
{
	register ubyte	*buffer;
	register int32	npages;

	buffer = rp->rp_Bitmap->bm_Buffer;
	npages = (320 * sizeof (uint16) * cy +
		  (320 * sizeof (uint16) * 2) - 1) >> 11;

	SetVRAMPages (sportIO, buffer, ceilingcolor, npages, ~0L);
	buffer += npages * GrafBase->gf_VRAMPageSize;
	SetVRAMPages (sportIO, buffer, floorcolor, screenpages - npages, ~0L);
}


/***************************************************************************
 * Fairly useful stuff.
 */
static uint8	clutmap[] = {
	 26,  32,  38,  44,  50,  56,  61,  67,
	 73,  78,  83,  90,  99, 107, 115, 123,
	132, 140, 148, 156, 165, 173, 181, 189,
	198, 206, 214, 222, 231, 239, 247, 255
};


void
installclut (rp)
register struct RastPort	*rp;
{
	register int32	i, n;

	for (i = 32;  --i >= 0; ) {
		n = clutmap[i];
		SetScreenColor (rp->rp_ScreenItem,
				MakeCLUTColorEntry (i, n, n, n));
	}
	n = clutmap[0];
	SetScreenColor (rp->rp_ScreenItem, MakeCLUTColorEntry (32, n, n, n));
}

void
fadetolevel (rp, level)
register struct RastPort	*rp;
frac16				level;
{
	register int32	l, m;

	for (l = 0;  l < 32;  l++) {
		m = ConvertF16_32 (level * clutmap[l]);
		SetScreenColor (rp->rp_ScreenItem,
				MakeCLUTColorEntry (l, m, m, m));
	}
	m = ConvertF16_32 (level * clutmap[0]);
	SetScreenColor (rp->rp_ScreenItem, MakeCLUTColorEntry (32, m, m, m));
}

void
fadetoblank (rp, frames)
register struct RastPort	*rp;
int32				frames;
{
	fadeout (rp, frames);
	SetRast (rp, 0);
	installclut (rp);
}

void
fadeout (rp, frames)
register struct RastPort	*rp;
int32				frames;
{
	int32	j, k;

	for (j = frames;  --j >= 0; ) {
		WaitVBL (vblIO, 1);
		k = Convert32_F16 (j) / (frames - 1);
		fadetolevel (rp, k);
	}
}

void
fadeup (rp, frames)
register struct RastPort	*rp;
int32				frames;
{
	int32	j, k;

	for (j = 0;  j < frames;  j++) {
		WaitVBL (vblIO, 1);
		k = Convert32_F16 (j) / (frames - 1);
		fadetolevel (rp, k);
	}
}


void *
malloctype (size, memtype)
int32	size;
uint32	memtype;
{
	register int32	*ptr;

	size += sizeof (int32);

	if (ptr = AllocMem (size, memtype))
		*ptr++ = size;

	return ((void *) ptr);
}

void
freetype (ptr)
void *ptr;
{
	register int32	*lptr;

	if (lptr = (int32 *) ptr) {
		lptr--;
		FreeMem (lptr, *lptr);
	}
}


/***************************************************************************
 * Housekeeping.
 */
void
loadlevelsequence ()
{
	register int	idx;
	register char	c, *s, *name;
	int32		err_len;

	/*
	 * Load file.
	 */
	if (!(levelseqbuf = allocloadfile (sequencefile, 0, &err_len)))
		die ("Can't load level sequence.\n");

	/*
	 * Scan for number of lines in file by counting newlines.
	 */
	s = levelseqbuf;
	nseq = 0;
	while (c = *s++, s - levelseqbuf <= err_len)
		if (c == '\n'  ||  c == '\r')
			nseq++;

	/*
	 * Create table to hold pointers to directory names.
	 */
	if (!(seqnames = AllocMem (nseq * sizeof (char *), 0)))
		die ("Can't allocate sequence table.\n");

	/*
	 * Build table of name pointers.
	 */
	s = levelseqbuf;
	name = s;
	idx = 0;
	while (c = *s++, s - levelseqbuf <= err_len)
		if (c == '\n'  ||  c == '\r') {
			s[-1] = '\0';
			seqnames[idx++] = name;
			name = s;
		}
}


void
loadwalls ()
{
	if (!(walliev = loadloaf (wallimagefile)))
		die ("Can't load wall images.\n");

	wallimgs = walliev->iev_ImageEntries;
	initwallanim (walliev);
}

void
unloadwalls ()
{
	closewallanim ();
	freeloaf (walliev);  walliev = NULL;
}



void
openlevelstuff ()
{
	cy = CY;

	/*  Load level geometry.  */
	loadlevelmap (levelname);

	/*
	 * Load wall art.
	 */
	loadwalls ();

	initrend ();
	SetRast (rpvis, 0);
	SetRast (rprend, 0);

	/*
	 * Load/start sound FX.
	 */
	spoolsound (spoolmusicfile, 9999);
}

void
closelevelstuff ()
{
	stopspoolsound (1);
	fadetoblank (rpvis, 32);

	closerend ();
	unloadwalls ();
	freelevelmap ();
}


void
opengamestuff ()
{
	loadsfx ();
	initclip ();
	initmessages ();

	/*  Special cases  */
	loadstatscreen ();
	loadgun ();
	loadskull ();
	loadmap ();
}

void
closegamestuff ()
{
	freemap ();
	freeskull ();

	freecelarray (ca_ray);  ca_ray = NULL;
	freecelarray (ca_gun);  ca_gun = NULL;

	closestatscreen ();
	closemessages ();

	freesfx ();
}


void
openstuff ()
{
	register int	w, h;
	RastPort	*rp;
	Item		scritems[NSCREENS];

	InitFileFolioGlue ();

	if (OpenGraphicsFolio() < 0)
		die ("Can't open graphics folio.\n");

	if (OpenMathFolio () < 0)
		die ("Can't open math folio; did you run operamath?\n");

	if (KernelBase->kb_CPUFlags & (KB_RED | KB_REDWW | KB_GREEN))
		ccbextra |= CCB_ACE;

//	if (KernelBase->kb_CPUFlags & (KB_GREEN | KB_GREENWW))
//		ccbextra |= CCB_ALSC;

	/*
	 * Initialize convenience globals.
	 */
	vblIO = GetVBLIOReq ();
	sportIO = GetVRAMIOReq ();

	/*
	 * Create displays.
	 */
	scrtags->ta_Arg = (void *) GETBANKBITS (GrafBase->gf_ZeroPage);
	if ((sgrpitem = CreateScreenGroup (scritems, scrtags)) < 0)
		die ("Can't create screengroup.\n");

	/*
	 * Fill in my RastPort thingies.  (And turn on interpolation for
	 * some reason...)
	 */
	for (rp = rports, w = NSCREENS;  w--;  rp++) {
		rp->rp_ScreenItem = scritems[w];
		if (!(rp->rp_Screen = LookupItem (rp->rp_ScreenItem)))
			die ("Woah!  Where's the screen?\n");
		rp->rp_Bitmap = rp->rp_Screen->scr_TempBitmap;
		rp->rp_BitmapItem = rp->rp_Bitmap->bm.n_Item;
		EnableHAVG (rp->rp_ScreenItem);
		EnableVAVG (rp->rp_ScreenItem);
//		if (KernelBase->kb_CPUFlags & (KB_GREEN | KB_GREENWW))
//			SetCEControl (rp->rp_BitmapItem,
//				      rp->rp_Bitmap->bm_CEControl | ASCALL);
	}

	/*
	 * Compute a few things.
	 */
	wide = rpvis->rp_Bitmap->bm_ClipWidth;
	high = rpvis->rp_Bitmap->bm_ClipHeight;
	w = rpvis->rp_Bitmap->bm_Width;
	h = rpvis->rp_Bitmap->bm_Height;
	screenpages = (w * h * sizeof (int16) +
		       GrafBase->gf_VRAMPageSize - 1) /
		       GrafBase->gf_VRAMPageSize;
	screensize = screenpages * GrafBase->gf_VRAMPageSize;

	installclut (rpvis);
	installclut (rprend);
	SetRast (rpvis, 0);
	SetRast (rprend, 0);

	/*
	 * Open event port.
	 */
	if ((joythread = CreateThread
			  ("Joypad Reader", 180, joythreadfunc, 2048)) < 0)
		die ("Can't open joypad reading thread.\n");

	if (OpenAudioFolio () < 0)
		die ("Can't open audio folio; did you run it?\n");

	if (initElKabong ("$progdir"))
		die ("Can't prepare for El Kabong.\n");

	opentimer ();
	initsound ();
	initstreaming ();
	openmmfont ();

	loadlevelsequence ();
}

void
closestuff ()
{
	closelevelstuff ();
	closegamestuff ();

	closeElKabong ();
	shutdownstreaming ();
	CloseAudioFolio ();

	if (joythread) {
		DeleteThread (joythread);  joythread = 0;
	}

	closemmfont ();
//	closefont ();
}




void
die (str)
char *str;
{
	kprintf (str);
	closestuff ();
	exit (20);
}
