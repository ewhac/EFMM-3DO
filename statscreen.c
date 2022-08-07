/*  :ts=8 bk=0
 *
 * statscreen.c:	Display status screen
 *
 * Jon Leupp						9307.21
 */
#include <types.h>
#include <event.h>
#include <mem.h>
#include <graphics.h>
#include <operamath.h>
#include "string.h"
#include "stdio.h"

#include "castle.h"
#include "objects.h"
#include "imgfile.h"
#include "font.h"
#include "sound.h"
#include "flow.h"
#include "option.h"

#include "app_proto.h"


/***************************************************************************
 * Globals.
 */
extern int32	wide, high;
extern uint32	ccbextra;
extern RastPort *rprend, *rpvis;
extern Item	vblIO;

extern int8	domusic, dosfx, practice;
extern frac16	damagefade;
extern int32	nseq;

extern JoyData	jd;
extern CCB		MsgFontCCB;
extern CCB		*backwallcel;
extern int32	joytrigger;
extern int32	oldjoybits;
extern int32	playerhealth, gunpower, playerlives, level, score, nkeys;
extern int32	xlifethresh, xlifeincr;

//extern int strcpy(char *,char *), strcat(char *,char *), strlen(char *);

static CelArray	*ca_statbg = NULL, *ca_shard = NULL;
static CelArray *ca_mini_health = NULL, *ca_mini_gun = NULL,
				*ca_mini_life = NULL, *ca_intr_lev_shard;
CCB				*statbgcel;



static char *levelnames[] = {
		"Beastly Belfry",
		"Evil Attic",
		  "Rooms of Gloom",
		  "Ghostly Gallery",
		"Demonic Dungeon",
		"Prisoner's Peril",
		  "Grisly Grotto",
		  "Grave Cave",
		"Marrow Mall",
		"Lone Catacomb",
		  "Tombs of Doom",
		  "Ghastly Graveyard",
	};


#define RGB5(r,g,b) ((r<<10)|(g<<5)|b)

	static int32 TestPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(6,8,14)<<16)|RGB5(5,5,7),
		(RGB5(6,7,11)<<16)|RGB5(5,5,7),
		(RGB5(4,4,5)<<16)|RGB5(3,3,5),
	};

/*
	static int32 HighlightPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(12,14,17)<<16)|RGB5(7,7,7),
		(RGB5(9,9,9)<<16)|RGB5(12,12,12),
		(RGB5(14,14,14)<<16)|RGB5(16,16,16),
	};
*/
	static int32 BrassPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(21,19,1)<<16)|RGB5(20,18,0),
		(RGB5(14,11,10)<<16)|RGB5(10,8,0),
		(RGB5(12,10,0)<<16)|RGB5(8,7,0),
	};

	static int32 BrassHiPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(26,24,5)<<16)|RGB5(27,25,12),
		(RGB5(27,26,17)<<16)|RGB5(28,27,22),
		(RGB5(29,29,26)<<16)|RGB5(28,27,22),
	};

	static int32 OrangePLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(10,7,8)<<16)|RGB5(15,10,5),
		(RGB5(18,14,2)<<16)|RGB5(22,18,0),
		(RGB5(25,19,0)<<16)|RGB5(26,21,5),
	};

	static int32 DkPurplePLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(9,4,8)<<16)|RGB5(8,3,6),
		(RGB5(7,2,5)<<16)|RGB5(5,2,4),
		(RGB5(7,3,5)<<16)|RGB5(4,0,2),
	};

	static int32 LtBluePLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(11,7,13)<<16)|RGB5(12,11,17),
		(RGB5(13,15,22)<<16)|RGB5(15,18,25),
		(RGB5(18,20,27)<<16)|RGB5(20,23,28),
	};

	static int32 PukeGreenPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(10,6,7)<<16)|RGB5(9,9,5),
		(RGB5(9,11,3)<<16)|RGB5(10,13,0),
		(RGB5(11,15,0)<<16)|RGB5(13,17,0),
	};

// Yellow/green
	static int32 OptHiLitePLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(10,16,16)<<16)|RGB5(8,19,12),
		(RGB5(6,22,8)<<16)|RGB5(5,24,5),
		(RGB5(6,26,6)<<16)|RGB5(5,28,5),
	};

// Blue/gray
	static int32 HiScoreHiLitePLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(21,19,5)<<16)|RGB5(18,17,10),
		(RGB5(15,14,14)<<16)|RGB5(12,12,18),
		(RGB5(13,13,19)<<16)|RGB5(14,14,20),
	};


	FontStruct TestText =
	{
		NULL,
		0,
		40,
		140,
		10,
		0,
		TestPLUT
	};

int32	fontbufsize;

void
openmmfont(void)
{
	FontInit (40);	// init font to allow max of 40 chars/line
	if (!(TestText.FontPtr = allocloadfile ("$progdir/mm.font", MEMTYPE_CEL, &fontbufsize)))
		die ("Couldn't load font.\n");
}

void
closemmfont(void)
{
	FontFree();
	FreeMem(TestText.FontPtr,fontbufsize);
}


/***************************************************************************
 * // Interlevel Screen Stuff \\
 */

static CelArray	*ca_intrlev = NULL;
CCB				*intrlevcel;

void
loadintrlevscreen()
{
static char *talismanstr[] = {
	"IL 1.C6",
	"IL 2.C6",
	"IL 3.C6",
	"IL 4.C6",
	"IL 5.C6",
	"IL 6.C6",
	"IL 7.C6",
	"IL 8.C6",
	"IL 9.C6",
	"IL 10.C6",
	"IL 11.C6",
	"IL 12.C6",
};
char fname[80];

	if (!(ca_intrlev = parse3DO ("$progdir/intrlev.cel")))
		die ("Couldn't load intrlev.cel.\n");

	intrlevcel = ca_intrlev->celptrs[0];
	intrlevcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND;
	intrlevcel->ccb_Flags &= ~CCB_ACCW;

	intrlevcel->ccb_Flags |= CCB_LAST;
	intrlevcel->ccb_XPos = ((wide - REALCELDIM (intrlevcel->ccb_Width)) >> 1) << 16;
	intrlevcel->ccb_YPos = (((high - REALCELDIM (intrlevcel->ccb_Height)) >> 1)) << 16;

	strcpy(fname,"$progdir/");
	strcat(fname,talismanstr[level]);

	if (!(ca_intr_lev_shard = parse3DO (fname)))
		die ("Couldn't load talisman.\n");

//	if (!(ca_button1 = parse3DO ("$progdir/OPS-SML.P.Unc16")))
//		die ("Couldn't load ops button.\n");

//	if (!(ca_button2 = parse3DO ("$progdir/CONT-SML.P.Unc16")))
//		die ("Couldn't load cont button.\n");

//	loadsound ("TallyPtsShort.aifc",120);
//	loadsound ("TallyPtsLong.aifc",120);
}
void
closeintrlevscreen ()
{
	if (ca_intrlev) {
		freecelarray (ca_intrlev);
		ca_intrlev = NULL;
	}

	if (ca_intr_lev_shard) {
		freecelarray (ca_intr_lev_shard);
		ca_intr_lev_shard = NULL;
	}

//	if (ca_button1) {
//		freecelarray (ca_button1);
//		ca_button1 = NULL;
//	}

//	if (ca_button2) {
//		freecelarray (ca_button2);
//		ca_button2 = NULL;
//	}
//	unloadsound (120);
}




int
dointerlev ()
{
	int	dosound, retval;

	dosound = TRUE;
	retval = 0;
	do {
		retval = dispintrlev (dosound);
		dosound = FALSE;

		switch (retval) {
		case FC_LOADGAME:
			if (loadgame () < 0) {
				/*
				 * Operation failed; return to interlevel.
				 */
				retval = 0;
				continue;
			}

		case FC_NEWGAME:
		case FC_RESTART:
		case FC_COMPLETED:
			break;

		case FC_SAVEGAME:
			savegame ();
			retval = 0;	// Show interlevel again.
			continue;
		}
	} while (!retval);

	return (retval);
}






int
dispintrlev (dosound)
int	dosound;
{
	static Point	corner[4];
	int				i;
//	static int32			reallevel;
	char		spoolfile[80];
	int8		fadeflag, spoollatch;

#define INTRX 32
#define INTRY 0

	static struct StatText {
		int32	x,y;
		char	*str;
		int32	*plut;
	} stattext[] = {
		96,INTRY+30,"You Survived The",DkPurplePLUT,	//[0]
		INTRX+144,INTRY+42,NULL,DkPurplePLUT,			//[1]
		INTRX+6,INTRY+86,"Health",PukeGreenPLUT,		//[2]
		INTRX+6,INTRY+114,"Gun",PukeGreenPLUT,			//[3]
		INTRX+6,INTRY+142,"Lives",PukeGreenPLUT,		//[4]
		INTRX+44,INTRY+186,"Score",PukeGreenPLUT,		//[5]
		INTRX+163, INTRY+182,"A Load Game",PukeGreenPLUT,
		INTRX+164, INTRY+195,"B Save Game",PukeGreenPLUT,
		INTRX+164, INTRY+208,"C Continue",PukeGreenPLUT,
		0,
	};

	static struct StatVals {
		int32	x,y;
		int32	*valptr;
		int32	*plut;
		int32	len;
	} statvals[] = {
		INTRX+62,INTRY+86,&playerhealth,LtBluePLUT,3,
		INTRX+62,INTRY+114,&gunpower,LtBluePLUT,3,
		INTRX+62,INTRY+142,&playerlives,LtBluePLUT,3,
		INTRX+20,INTRY+201,&score,LtBluePLUT,7,
		0,
	};

	static struct StatIcons {
		int32		x,y;
		CelArray	**ca;
	} staticons[] = {
		INTRX+98,INTRY+80,&ca_mini_health,
		INTRX+98,INTRY+108,&ca_mini_gun,
		INTRX+98,INTRY+136,&ca_mini_life,
		INTRX+161,INTRY+73,&ca_intr_lev_shard,
//		INTRX+140,INTRY+183,&ca_button1,
//		INTRX+206,INTRY+183,&ca_button2,
		0,
	};

	char	str[10]; // for value display
	CCB		*iconccb;
	int32	iconframe,tallydelay;
	RastPort *tmp;


	loadintrlevscreen ();

	iconframe = 0;
	fadeflag = 0;
	spoollatch = TRUE;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame
	stattext[1].str = levelnames[level];
	stattext[1].x = 160+8-4*strlen(levelnames[level]);
	tallydelay = 15;

	if (dosound) {
		sprintf (spoolfile, "$progdir/aiff/InterLevel/Level%02d.aiff",
			 level + 1);
		spoolsound (spoolfile, 1);
	}

	SetRast (rpvis, 0);
	fadetolevel (rprend, 0);

	do {

		resetjoydata ();

		if (domusic  &&  spoollatch  &&  !issoundspooling ()) {
			spoolsound ("$progdir/aiff/ColorEcho.aiff", 9999);
			spoollatch = FALSE;
		}

//		DrawCels (rprend->rp_BitmapItem, backwallcel);
		corner[0].pt_X = corner[3].pt_X = (intrlevcel->ccb_XPos)>>16;
		corner[0].pt_Y = corner[1].pt_Y = (intrlevcel->ccb_YPos)>>16;

		corner[1].pt_X = corner[2].pt_X = corner[0].pt_X+(intrlevcel->ccb_Width);
		corner[2].pt_Y = corner[3].pt_Y = corner[0].pt_Y+(intrlevcel->ccb_Height);
		FasterMapCel (intrlevcel, corner);
		DrawCels (rprend->rp_BitmapItem, intrlevcel);

		TestText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

// bonus for gun and health code
		if (tallydelay)
			tallydelay--;
		else if (playerhealth  ||  gunpower) {
			// make sound call here
			if (playerhealth) {
				if (playerhealth >= 5) {
					score += 500;
					playerhealth -= 5;
				} else {
					score += playerhealth * 100;
					playerhealth = 0;
				}

//				playsound (120);
			}
			if (gunpower) {
				if (gunpower >= 5) {
					score += 500;
					gunpower -= 5;
				} else {
					score += gunpower * 100;
					gunpower = 0;
				}

//				playsound (120);
			}
			if (!(oldjoybits & ControlC)) tallydelay = 3;

			if (score >= xlifethresh) {
				playerlives++;
				playsound (SFX_GRABLIFE);
				xlifethresh += xlifeincr;
			}
		}


		for (i = 0;  (stattext[i].x);  i++) {
			if (practice  &&  i == 7)
				/*  Don't let user save.  */
				continue;

			TestText.TextPtr = stattext[i].str;
			TestText.CoordX = stattext[i].x;
			TestText.CoordY = stattext[i].y;
			TestText.PLUTPtr = stattext[i].plut;
			FontPrint(&TestText);
		}

		for (i=0; (statvals[i].x); i++) {
			sprintf(str,"%ld",*(statvals[i].valptr));
			TestText.TextPtr = str;
			TestText.CoordX = statvals[i].x + 8*(statvals[i].len-strlen(str));
			if (TestText.CoordX < statvals[i].x)
				TestText.CoordX = statvals[i].x;
			TestText.CoordY = statvals[i].y;
			TestText.PLUTPtr = statvals[i].plut;
			FontPrint(&TestText);
		}

		for (i=0; (staticons[i].x); i++) {
			if (i>2)
				iconccb = (*(staticons[i].ca))->celptrs[0];
			else
				iconccb = (*(staticons[i].ca))->celptrs[iconframe];
			corner[0].pt_X = corner[3].pt_X = staticons[i].x;
			corner[0].pt_Y = corner[1].pt_Y = staticons[i].y;

			corner[1].pt_X = corner[2].pt_X = corner[0].pt_X+(iconccb->ccb_Width);
			corner[2].pt_Y = corner[3].pt_Y = corner[0].pt_Y+(iconccb->ccb_Height);
			FasterMapCel (iconccb, corner);
			DrawCels (rprend->rp_BitmapItem, iconccb);
		}
		if (++iconframe == 12) iconframe = 0;

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);
		WaitVBL (vblIO, 3);
		if (!fadeflag) {
			fadeup (rpvis, 32);
			fadeflag++;
		}

	} while (playerhealth  ||  gunpower || tallydelay  ||
		 (!(joytrigger & (ControlA | ControlB | ControlC | ControlX | ControlStart)) &&
		 !(oldjoybits & ControlC) ));


	stopspoolsound (1);
	fadetoblank (rpvis, 32);
	closeintrlevscreen ();

	i = joytrigger;
	resetjoydata ();
	if (i & ControlA) { // load game
		return (FC_LOADGAME);
	}
	else if (i & ControlB  &&  !practice) { // save game
		return (FC_SAVEGAME);
	}
	else if (i & ControlX) { // options
		return (dispoptionscreen (1));
	}

	return (FC_COMPLETED);
}

// end Interlevel Stuff \\


/***************************************************************************
 * Status screen.
 */
void
killshard()
{
	if (ca_shard) {
		freecelarray (ca_shard);
		ca_shard = NULL;
	}
}

void
loadshard()
{
static char *shardstr[] = {
	"DS1.C6",
	"DS2.C6",
	"DS3.C6",
	"DS4.C6",
	"DS5.C6",
	"DS6.C6",
	"DS7.C6",
	"DS8.C6",
	"DS9.C6",
	"DS10.C6",
	"DS11.C6",
	"DS12.C6",
	};
#define	NSHARDS		(sizeof (shardstr) / sizeof (char *))

char fname[80];

	if (level < NSHARDS) {
		killshard();
		strcpy (fname, "$progdir/");
		strcat (fname, shardstr[level]);

		if (!(ca_shard = parse3DO (fname)))
			die ("Couldn't load shard.\n");
		(ca_shard->celptrs[0])->ccb_Height = 41;  // kluge for now
	}
}

void
loadstatscreen()
{
	if (!(ca_statbg = parse3DO ("$progdir/statscreen.cel")))
		die ("Couldn't load statscreen.cel.\n");

	statbgcel = ca_statbg->celptrs[0];
	statbgcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
	statbgcel->ccb_Flags &= ~CCB_ACCW;

	statbgcel->ccb_Flags |= CCB_LAST;
	statbgcel->ccb_XPos = ((wide - REALCELDIM (statbgcel->ccb_Width)) >> 1) << 16;
	statbgcel->ccb_YPos = (((high - REALCELDIM (statbgcel->ccb_Height)) >> 1)) << 16;

	if (!(ca_mini_health = parse3DO ("$progdir/mini_health.cel")))
		die ("Couldn't load mini_health.cel.\n");

	if (!(ca_mini_gun = parse3DO ("$progdir/mini_gun.cel")))
		die ("Couldn't load mini_gun.cel.\n");

	if (!(ca_mini_life = parse3DO ("$progdir/mini_life.cel")))
		die ("Couldn't load mini_life.cel.\n");

}


void
closestatscreen ()
{
	if (ca_statbg) {
		freecelarray (ca_statbg);
		ca_statbg = NULL;
	}
	if (ca_mini_health) {
		freecelarray (ca_mini_health);
		ca_statbg = NULL;
	}
	if (ca_mini_gun) {
		freecelarray (ca_mini_gun);
		ca_mini_gun = NULL;
	}
	if (ca_mini_life) {
		freecelarray (ca_mini_life);
		ca_mini_life = NULL;
	}

// test stuff
/*
	if (ca_sbutton1) {
		freecelarray (ca_sbutton1);
		ca_sbutton1 = NULL;
	}

	if (ca_sbutton2) {
		freecelarray (ca_sbutton2);
		ca_sbutton2 = NULL;
	}
*/

	killshard();
}




int
dostatmap ()
{
	int	retval;

	do {
		if ((retval = dispstats ()) <= 0)
			break;
		if ((retval = drawmap ()) <= 0)
			break;
	} while (1);

	if (retval < 0)
		return (attemptoptions ());
	else
		return (FC_NOP);
}





int
dispstats ()
{
	static Point	corner[4];
	int				i;

#define STATX 64
#define STATY 54

static char look_for_str[] = "Look\nFor:";
static char found_str[]   = "Found:";

extern int8 gottalisman;

	static struct StatText {
		int32	x,y;
		char	*str;
	} stattext[] = {
		STATX,STATY-2,NULL,
		STATX+8,STATY+20,"Health",
		STATX+8,STATY+44,"Gun",
		STATX+8,STATY+68,"Lives",
		STATX+8,STATY+92,"Score",
		STATX+132,STATY+20,"Keys",
		STATX+132,STATY+40,look_for_str,
		STATX+20,STATY+114,"Press",
		STATX+66,STATY+110,"A for map",
		STATX+66,STATY+122,"C to continue",
		0,
	};

/*
	char *HealthText[] = {
		"WALKING DEAD",
		"DECIMATED",
		"WASTED",
		"RAVAGED",
		"PRETTY BAD",
		"NOT GOOD",
		"HURTIN",
		"DAMAGED",
		"INJURED",
		"BRUISED",
		"SCRATCHED",
		"UNSCATHED",
	};
*/

	static struct StatVals {
		int32	x,y;
		int32	*valptr;
		int32	len;
	} statvals[] = {
		STATX+60,STATY+20,&playerhealth,3,
		STATX+60,STATY+44,&gunpower,3,
		STATX+60,STATY+68,&playerlives,3,
		STATX+60-8,STATY+92,&score,4,
		STATX+172,STATY+20,&nkeys,2,
		0,
	};

	static struct StatIcons {
		int32		x,y;
		CelArray	**ca;
	} staticons[] = {
		STATX+100,STATY+16,&ca_mini_health,
		STATX+100,STATY+40,&ca_mini_gun,
		STATX+100,STATY+64,&ca_mini_life,
		STATX+138,STATY+65,&ca_shard,
//		INTRX-4,INTRY+186,&ca_sbutton1,
//		INTRX+66,INTRY+186,&ca_sbutton2,
		0,
	};

	char	str[10]; // for value display
	CCB		*iconccb;
	int32	iconframe;
	RastPort *tmp;

	iconframe = 0;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame

	stattext[0].str = levelnames[level];
	stattext[0].x = 160+8-4*strlen(levelnames[level]);

	if (gottalisman)
		stattext[6].str = found_str;
	else
		stattext[6].str = look_for_str;

	/*
	 * Make sure we can see it clearly.
	 */
	fadetolevel (rprend, ONE_F16);
	fadetolevel (rpvis, ONE_F16);


	do {
		resetjoydata ();

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);
		WaitVBL (vblIO, 2);

		clearscreen (rprend);
		DrawCels (rprend->rp_BitmapItem, backwallcel);
		corner[0].pt_X = corner[3].pt_X = (statbgcel->ccb_XPos)>>16;
		corner[0].pt_Y = corner[1].pt_Y = (statbgcel->ccb_YPos)>>16;

		corner[1].pt_X = corner[2].pt_X = corner[0].pt_X+(statbgcel->ccb_Width);
		corner[2].pt_Y = corner[3].pt_Y = corner[0].pt_Y+(statbgcel->ccb_Height);
		FasterMapCel (statbgcel, corner);
		DrawCels (rprend->rp_BitmapItem, statbgcel);

		TestText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		TestText.PLUTPtr = PukeGreenPLUT;

		for (i=0; (stattext[i].x); i++) {
			TestText.TextPtr = stattext[i].str;
			TestText.CoordX = stattext[i].x;
			TestText.CoordY = stattext[i].y;
			FontPrint(&TestText);
		}

		TestText.PLUTPtr = LtBluePLUT;

		for (i=0; (statvals[i].x); i++) {
			sprintf(str,"%ld",*(statvals[i].valptr));
			TestText.TextPtr = str;
			TestText.CoordX = statvals[i].x + 8*(statvals[i].len-strlen(str));
			if (TestText.CoordX < statvals[i].x)
				TestText.CoordX = statvals[i].x;
//			TestText.CoordX = statvals[i].x;
			TestText.CoordY = statvals[i].y;
			FontPrint(&TestText);
		}

		for (i=0; (staticons[i].x); i++) {
//			if (staticons[i].ca == &ca_shard) {
			if (i>2) {
				iconccb = (*(staticons[i].ca))->celptrs[0];
				iconccb->ccb_XPos = (staticons[i].x)<<16;
				iconccb->ccb_YPos = (staticons[i].y)<<16;
			}
			else {
				iconccb = (*(staticons[i].ca))->celptrs[iconframe];
				corner[0].pt_X = corner[3].pt_X = staticons[i].x;
				corner[0].pt_Y = corner[1].pt_Y = staticons[i].y;

				corner[1].pt_X = corner[2].pt_X = corner[0].pt_X+(iconccb->ccb_Width);
				corner[2].pt_Y = corner[3].pt_Y = corner[0].pt_Y+(iconccb->ccb_Height);
// printf("i = %d,iconccb->ccb_Width/Height = %d / %d\n",i,iconccb->ccb_Width,iconccb->ccb_Height);
				FasterMapCel (iconccb, corner);
			}
			DrawCels (rprend->rp_BitmapItem, iconccb);
		}
		if (++iconframe == 12) iconframe = 0;

	} while (!(joytrigger & (ControlC | ControlA | ControlX)));

	i = joytrigger;
	resetjoydata ();

	if (i & ControlA)
		return (1);
	else if (i & ControlX)
		return (-1);

	return (0);
}



/***************************************************************************
 * // Option Screen Stuff \\
 */
extern int32	xlifethresh, playerlives, score;

static CelArray	*ca_optionscr = NULL;
static CCB	*optionscrcel;


void
loadoptionscreen()
{
	if (!(ca_optionscr = parse3DO ("$progdir/optionscr.cel")))
		die ("Couldn't load optionscr.cel.\n");

	optionscrcel = ca_optionscr->celptrs[0];
	optionscrcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND;
	optionscrcel->ccb_Flags &= ~CCB_ACCW;

	optionscrcel->ccb_Flags |= CCB_LAST;
	optionscrcel->ccb_XPos = ((wide - REALCELDIM (optionscrcel->ccb_Width)) >> 1) << 16;
	optionscrcel->ccb_YPos = (((high - REALCELDIM (optionscrcel->ccb_Height)) >> 1)) << 16;
}

void
closeoptionscreen ()
{
	if (ca_optionscr) {
		freecelarray (ca_optionscr);
		ca_optionscr = NULL;
	}
	fadetoblank (rpvis, 32);
}

//////////////////////////////////////
extern SaveGameRec	GameState;
extern char		spoolmusicfile[];


int
attemptoptions ()
{
	int	retval;

	stopspoolsound (1);
	fadetoblank (rpvis, 32);
	unloadwalls ();		/*  AIEEEEE!!!  */

	if ((retval = dispoptionscreen (1)) == FC_RESUMEGAME) {
		loadwalls ();
		spoolsound (spoolmusicfile, 9999);
		resetjoydata ();
	}
	damagefade = ONE_F16;

	return (retval);
}




int
dispoptionscreen (startresume)
int32	startresume; // 0 = START GAME, 1 = RESUME GAME no SAVE, 2 = RESUME with SAVE GAME
{
	int				i;

#define OPTX 16
#define OPTY 0

static char	music_on_str[]  = "Music On";
static char	music_off_str[] = "Music Off";

static char	sfx_on_str[]  = "Sound Effects On";
static char	sfx_off_str[] = "Sound Effects Off";

static char	play_for_keeps_str[]  = "Play for keeps";
static char	practice_level_str[40];

static char	*start_game[]  = {"Start Game",
							 "Resume Game"};
static char	*quit_restart[] = {"  Replay Intro",  "   Quit Game"};

	static struct StatText {
		int32	x,y;
		char	*str;
		int32	color;
	} stattext[] = {
		OPTX+116,OPTY+108,music_on_str,RGB5(1,1,1),
		OPTX+88,OPTY+122,sfx_on_str,RGB5(1,1,1),

		OPTX+85,OPTY+136,"View High Scores",RGB5(1,1,1),
		OPTX+104,OPTY+150,"View Credits",RGB5(1,1,1),

		OPTX+99,OPTY+164,play_for_keeps_str,RGB5(1,1,1),
		OPTX+112,OPTY+178,"Load Game",RGB5(1,1,1),
		0,OPTY+192,NULL,RGB5(1,1,1),			// START or RESUME GAME
		OPTX+95,OPTY+206,NULL,RGB5(1,1,1),

//		100,96,"ABCDEFGHIJKLMNOPQR",RGB5(1,1,1),  // test display of entire font
//		100,108,"STUVWXYZabcdefghij",RGB5(1,1,1),
//		100,120,"klmnopqrstuvwxyz(.'!)",RGB5(1,1,1),

		0,
	};

#define	MUSICOPT 0
#define	SFXOPT   1
#define HISCORES 2
#define CREDITS	 3
#define PRACTICE 4
#define	LOADOPT  5
#define GOTOGAME 6
#define QUIT	 7
#define	NUMCHOICES 8

	char *music_strings[] = { music_off_str, music_on_str };
	char *sfx_strings[]   = { sfx_off_str, sfx_on_str };


	int32	active_string = GOTOGAME;
	int32	frame;
	RastPort *tmp;
	int32	fadeflag = 0;
	int32	practice_level = 0; // 0 = play, 1-12 = practice that level
	int	retval;

	frame = 0;
	retval = 0;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame
	stattext[GOTOGAME].str = start_game[startresume];
	stattext[GOTOGAME].x = OPTX+112 - (8*startresume);
	stattext[MUSICOPT].str = music_strings[domusic];
	stattext[SFXOPT].str = sfx_strings[dosfx];

	if (!startresume) {
		practice = FALSE;
		stattext[PRACTICE].str = play_for_keeps_str;
	}
	stattext[QUIT].str = quit_restart[startresume];
	stattext[PRACTICE].x =
	 (328 - (7 * strlen (stattext[PRACTICE].str))) >> 1;


	loadoptionscreen();

	SetRast (rpvis, 0);
	fadetolevel (rprend, 0);

	do {
		resetjoydata ();

		DrawCels (rprend->rp_BitmapItem, optionscrcel);

		TestText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		for (i=0; (stattext[i].x); i++) {
// printf("i = %d, stattext[i].x = %d\n",i,stattext[i].x);
			if (i == active_string)
				TestText.PLUTPtr = OptHiLitePLUT;
			else {
				TestText.PLUTPtr = TestPLUT;
			}
			if (!(startresume && (i==PRACTICE)))
			{
				TestText.TextPtr = stattext[i].str;
				TestText.CoordX = stattext[i].x;
				TestText.CoordY = stattext[i].y;
				FontPrint(&TestText);
			}
		}

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);
		WaitVBL (vblIO, 1);
		if (!fadeflag) {
			fadeup (rpvis, 32);
			fadeflag++;
		}
		frame++;

		if ((joytrigger & ControlDown)||((!(frame&15))&&(oldjoybits & ControlDown))) {
			if (++active_string>=NUMCHOICES)
				active_string = 0;
			if (startresume && (active_string==PRACTICE))
				++active_string;
			frame = 0;
		}
		if ((joytrigger & ControlUp)||((!(frame&15))&&(oldjoybits & ControlUp))) {
			if (--active_string<0)
				active_string = NUMCHOICES-1;
			if (startresume && (active_string==PRACTICE))
				--active_string;
			frame = 0;
		}
		if (joytrigger & (ControlA | ControlB | ControlC | ControlStart)) {
			if (joytrigger & ControlStart)
				active_string = GOTOGAME;

			switch (active_string) {
			case MUSICOPT:
				domusic ^= 1;
				stattext[active_string].str = music_strings[domusic];
				break;

			case SFXOPT:
				dosfx ^= 1;
				stattext[active_string].str = sfx_strings[dosfx];
				break;

			case HISCORES:
				closeoptionscreen ();
				DoHighScoreScr (-1);	// If newScore is -1, show high score screen
				loadoptionscreen ();
				fadetolevel (rprend, 0);
				fadeflag = FALSE;
				break;

			case CREDITS:
				closeoptionscreen ();
				showcredits ();
				loadoptionscreen ();
				fadetolevel (rprend, 0);
				fadeflag = FALSE;
				break;

			case LOADOPT:
				closeoptionscreen ();
				if (loadgame () < 0)
					loadoptionscreen ();
				else {
					practice_level = 0;
					retval = FC_LOADGAME;
				}
				break;

			case PRACTICE:
				if (++practice_level > nseq)
					practice_level = 0;
				stattext[active_string].str = play_for_keeps_str;
				if (practice_level) {
					strcpy (practice_level_str,"Practice ");
					strcat (practice_level_str,levelnames[practice_level-1]);
					stattext[active_string].str = practice_level_str;
				}
				stattext[PRACTICE].x = (328 - (7*strlen(stattext[PRACTICE].str)))>>1;

				break;

			case GOTOGAME:
				if (!startresume) {
					score = 0;
					playerlives = 3;
					xlifethresh = xlifeincr;
					if (practice_level) {
						level = practice_level - 1;
						practice = TRUE;
					} else {
						level = 0;
						practice = FALSE;
					}
					retval = FC_NEWGAME;
				} else
					retval = FC_RESUMEGAME;
				break;

			case QUIT:
				if (startresume) {
					if (rendermessage (-1, "Abandon\nCurrent Game?"))
						retval = FC_RESTART;
				} else
					retval = FC_RESTART;
	           		break;

			}

		}

	} while (!retval);

	oldjoybits = 0xff;
	jd.jd_ADown = jd.jd_BDown = jd.jd_CDown = 0;
	fadetoblank (rpvis, 30);
	closeoptionscreen ();
	resetjoydata ();

	return (retval);
}

// end Option Screen Stuff \\


/***************************************************************************
 * Game load and save.
 */
int
loadgame()
{
	int	retval;

	if ((retval = DoLoadGameScr (&GameState)) >= 0) {
		score		= GameState.sg_score;
		xlifethresh	= GameState.sg_XLifeThresh;
		level		= GameState.sg_Level;
		playerlives	= GameState.sg_NLives;
	}

	return (retval);

//printf("score = %d, level = %d, playerlives = %d\n",score, level, playerlives);
}

int
savegame()
{
	GameState.sg_score	= score;
	GameState.sg_XLifeThresh= xlifethresh;
	GameState.sg_Level	= level + 1;	// Level player will be on.
	GameState.sg_NLives	= playerlives;
	return (DoSaveGameScr (&GameState));
}


/***************************************************************************
 * // credits stuff \\
 */

void
showcredits ()
{
RastPort *tmp;
static CelArray	*ca_creditsrear = NULL, *ca_creditsfront = NULL;
CCB			*creditsrearcel, *creditsfrontcel;

static char	blankstr[] = {"                    "};


	static struct Credit {
		int32	y;
		char	*str;
		int32	*plut;
	} credits[] = {

		-174,blankstr,LtBluePLUT,
		-154,blankstr,LtBluePLUT,
		-116,blankstr,LtBluePLUT,
		-96,blankstr,LtBluePLUT,
		-58,blankstr,LtBluePLUT,
		-38,blankstr,LtBluePLUT,
		0,"Lead Programmer:",LtBluePLUT,
		20,"Leo L. Schwab",LtBluePLUT,

		58," Artwork, Level Design:",LtBluePLUT,
		78,"Stefan Henry-Biskup",LtBluePLUT,

		116," Animatrix:",LtBluePLUT,
		136,"Kim Tempest",LtBluePLUT,

		174,"Programming:",LtBluePLUT,
		194,"Jon Leupp",LtBluePLUT,

		232,"  Artiste Noire:",LtBluePLUT,
		252," Liz Beatrice",LtBluePLUT,

		290,"   Additional Art:",LtBluePLUT,
		310,"Greg Savoia",LtBluePLUT,

		348,"Music, Sound FX and",LtBluePLUT,
		361,"  Voice-Over Scripts:",LtBluePLUT,
		381,"Bob Vieira",LtBluePLUT,

		419,"Additional Programming:",LtBluePLUT,
		439,"Peter Commons",LtBluePLUT,

		477,"Intro 3D Animation:",LtBluePLUT,
		497,"Paul Barton",LtBluePLUT,

		535," Level Design:",LtBluePLUT,
		555,"Dan Duncalf",LtBluePLUT,
		575,"Mike Lopez",LtBluePLUT,

		613,"Testers:",LtBluePLUT,
		633,"Eric Carney",LtBluePLUT,
		653,"Casey Grimm",LtBluePLUT,
		673,"Michael Jablonn",LtBluePLUT,
		693,"Mike Lopez",LtBluePLUT,
		713,"Rich Shane",LtBluePLUT,
		733,"Brian Walker",LtBluePLUT,

		771,"  Evil Voice:",LtBluePLUT,
		791,"Les Hedger",LtBluePLUT,

		829,"Chief Zombie:",LtBluePLUT,
		849,"RJ Mical",LtBluePLUT,

		887,"Producers:",LtBluePLUT,
		907,"Stewart Bonn",LtBluePLUT,
		927,"Trip Hawkins",LtBluePLUT,

		965,"Assistant Producer:",LtBluePLUT,
		985,"Mike Lopez",LtBluePLUT,

		1023,"Special Thanks To:",LtBluePLUT,
		1043,"Dave Needle",LtBluePLUT,
		1063,"Dale Luck",LtBluePLUT,
		1083,"Stephen Landrum",LtBluePLUT,
		1103,"Dave Platt",LtBluePLUT,
		1123,"Phil Burk",LtBluePLUT,
		1143,"Stan Shepard",LtBluePLUT,
		1163,"Don Gray",LtBluePLUT,
		1183,"Paul Brandt",LtBluePLUT,
		1203,"  Jani Pettit",LtBluePLUT,
		1223,"Michael Conway",LtBluePLUT,
		1243,"Diane Hunt",LtBluePLUT,
		1263,"Pamela Henry-Biskup",LtBluePLUT,
		1283,"Mark Holmes",LtBluePLUT,
		1303," Penelope Terry",LtBluePLUT,
		1323," Joel and The Bots",LtBluePLUT,
		1343,"Amigans Everywhere",LtBluePLUT,

		1360,blankstr,LtBluePLUT,
		1390,blankstr,LtBluePLUT,
		1420,blankstr,LtBluePLUT,
		1450,blankstr,LtBluePLUT,
		1480,blankstr,LtBluePLUT,
		-1,
   };

	int32	absy,i,y,numshown;

// open credits
	if (!(ca_creditsrear = parse3DO ("$progdir/Credits rear")))	// make creditscr
		die ("Couldn't load optionscr.cel.\n");

	creditsrearcel = ca_creditsrear->celptrs[0];
	creditsrearcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND;
	creditsrearcel->ccb_Flags &= ~CCB_ACCW;

	creditsrearcel->ccb_Flags |= CCB_LAST;
	creditsrearcel->ccb_XPos = ((wide - REALCELDIM (creditsrearcel->ccb_Width)) >> 1) << 16;
	creditsrearcel->ccb_YPos = (((high - REALCELDIM (creditsrearcel->ccb_Height)) >> 1)) << 16;

	if (!(ca_creditsfront = parse3DO ("$progdir/Credits front")))	// make creditscr
		die ("Couldn't load optionscr.cel.\n");

	creditsfrontcel = ca_creditsfront->celptrs[0];
	creditsfrontcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND;
	creditsfrontcel->ccb_Flags &= ~CCB_ACCW;

	creditsfrontcel->ccb_Flags |= CCB_LAST;

	creditsfrontcel->ccb_XPos = ((wide - REALCELDIM (creditsfrontcel->ccb_Width)) >> 1) << 16;
	creditsfrontcel->ccb_YPos = (((high - REALCELDIM (creditsfrontcel->ccb_Height)) >> 1)) << 16;

// show them
	absy = 220;

	spoolsound ("$progdir/aiff/HauntHopTwo.aiff", 9999);

	do {
		resetjoydata ();

		DrawCels (rprend->rp_BitmapItem, creditsrearcel);

		TestText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		for (i=0, numshown=0; credits[i].y!=-1; i++ ) {
			y = credits[i].y+absy;
			if (y>0 && y<240) {
				numshown++;
				TestText.TextPtr = credits[i].str;
				TestText.CoordX = (320-7*strlen(credits[i].str))>>1;
				TestText.CoordY = y;
				TestText.PLUTPtr = credits[i].plut;
				FontPrint(&TestText);
			}
		}

		DrawCels (rprend->rp_BitmapItem, creditsfrontcel);

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);
			WaitVBL (vblIO, 1);
	}
	while (!(joytrigger & (ControlA|ControlB|ControlC))&&(--absy>-1313));
	oldjoybits = 0xff;
	jd.jd_ADown = jd.jd_BDown = jd.jd_CDown = 0;

	stopspoolsound (1);
	fadetoblank (rpvis, 32);

// close credits
	if (ca_creditsrear) {
		freecelarray (ca_creditsrear);
		ca_creditsrear = NULL;
	}
	if (ca_creditsfront) {
		freecelarray (ca_creditsfront);
		ca_creditsfront = NULL;
	}
}
