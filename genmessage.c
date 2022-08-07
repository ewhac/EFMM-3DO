/*  :ts=8 bk=0
 *
 * genmessage.c:	Generic message routines
 *
 * Jon Leupp						9307.21
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <event.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "imgfile.h"

#include "app_proto.h"

#include "font.h"

extern int32	wide, high;
extern uint32	ccbextra;
extern RastPort *rprend, *rpvis;
extern Item	vblIO;


#define MAXMSGWIDTH	17
#define MAXMSGLINES	8


CelArray	*ca_msgbg;
CCB		*msgbgcel;
CCB		MsgFontCCB;
int32		message_duration;
char		genmsgs[MAXMSGLINES][MAXMSGWIDTH+1];
int32		ngenmsgs, genmsgsx[MAXMSGLINES];


#define RGB5(r,g,b) ((r<<10)|(g<<5)|b)

/***************************************************************************
 * Static globals.
 */
static int32	MsgFontPLUT[] = {
//	0x00006318, 0x63186318, 0x63186318, 0x63186318,
	0x00000fff, 0x00000000, 0x00000000, 0x00000000,
};

static CCB	MsgFontCCBTemplate = {
	/* ulong ccb_Flags; */
	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS
	 | CCB_LDPPMP | CCB_LDPLUT | CCB_YOXY | CCB_ACW | CCB_ACCW
	 | PMODE_ZERO,
/*???CCB_PACKED*/

	/* struct CCB *ccb_NextPtr; CelData *ccb_CelData; void *ccb_PLUTPtr; */
	&MsgFontCCB, NULL, MsgFontPLUT,

	/* Coord ccb_X; Coord ccb_Y; */
	/* long ccb_HDX, ccb_HDY, ccb_VDX, ccb_VDY, ccb_DDX, ccb_DDY; */
	0,0,
	ONE_12_20,0, 0,ONE_16_16, 0,0,

	/* ulong ccb_PPMPC; */
	(PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),
};

static Font	MsgFont = {
	/* uint8     font_Height; */
	8,
	/* uint8     font_Flags; */
	FONT_ASCII | FONT_ASCII_UPPERCASE,
	/* CCB      *font_CCB; */
	&MsgFontCCB,
//	/* FontChar *font_FontCharArray[]; */
	/* font_FontEntries (significant only with RAM-resident fonts) */
	NULL,
};

static int32 MsgPLUT[] =
{
	(RGB5(0,0,0)<<16)|RGB5(0,2,0),
	(RGB5(11,7,13)<<16)|RGB5(12,11,17),
	(RGB5(13,15,22)<<16)|RGB5(15,18,25),
	(RGB5(18,20,27)<<16)|RGB5(20,23,28),
};

static int32 OptHiLitePLUT[] =
{
	(RGB5(0,0,0)<<16)|RGB5(0,2,0),
	(RGB5(10,16,16)<<16)|RGB5(8,19,12),
	(RGB5(6,22,8)<<16)|RGB5(5,24,5),
	(RGB5(6,26,6)<<16)|RGB5(5,28,5),
};

static int32 TestPLUT[] =
{
	(RGB5(0,0,0)<<16)|RGB5(0,2,0),
//	(RGB5(6,8,14)<<16)|RGB5(5,5,7),
//	(RGB5(6,7,11)<<16)|RGB5(5,5,7),
//	(RGB5(4,4,5)<<16)|RGB5(3,3,5),
 	(RGB5(12,14,17)<<16)|RGB5(7,7,7),
	(RGB5(9,9,9)<<16)|RGB5(12,12,12),
	(RGB5(14,14,14)<<16)|RGB5(16,16,16),
};

extern FontStruct	TestText;
extern JoyData		jd;
extern int32		joytrigger;

void
initfont()
{
	register Font	*sysfont;

	MsgFontCCB = MsgFontCCBTemplate;

	sysfont = GetCurrentFont ();
	MsgFont.font_FontEntries = sysfont->font_FontEntries;
	SetCurrentFontCCB (&MsgFontCCB);
}

void
closefont()
{
	ResetCurrentFont ();
}


void
initmessages()
{
	message_duration = 0;

	if (!(ca_msgbg = parse3DO ("$progdir/message screen.cel")))
		die ("Couldn't load message screen.cel.\n");

	msgbgcel = ca_msgbg->celptrs[0];
	msgbgcel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra;
	msgbgcel->ccb_Flags &= ~CCB_ACCW;

	msgbgcel->ccb_Flags |= CCB_LAST;
	msgbgcel->ccb_XPos = ((wide - REALCELDIM (msgbgcel->ccb_Width)) >> 1) << 16;
	msgbgcel->ccb_YPos = (((high - REALCELDIM (msgbgcel->ccb_Height)) >> 1)-25) << 16;
}


void
closemessages ()
{
	if (ca_msgbg) {
		freecelarray (ca_msgbg);  ca_msgbg = NULL;  msgbgcel = NULL;
	}
}


// dlay = number of frames to display message.  If <0 then prompts yes no
//  with "no" highlighted if dlay == -1 ("yes" highlighted if dlay != -1)
int
rendermessage (dlay, str)
int32	dlay;
char	*str;
{
	static Point	corner[4];
	GrafCon		gc;
	int		i, j, takedown;
	char		c;
	int		yesorno = 0;

// printf("rendermessage dlay = %d, str = '%s'\n",dlay,str);
	if (dlay != -1)
		yesorno = 1;

	TestText.PLUTPtr = MsgPLUT;

	takedown = FALSE;
	if (!msgbgcel) {
		initmessages ();
		takedown = TRUE;
	}

	ngenmsgs = i = 0;
	do {
		j = 0;
		while((c=str[i++]) && c!='\n')
			genmsgs[ngenmsgs][j++]=c;
//		genmsgsx[ngenmsgs] = 160-(j<<2);
		genmsgsx[ngenmsgs] = (320-(j*7))>>1;
		genmsgs[ngenmsgs++][j]=0;
	} while (c);

// do this here so it's easy to make variable later
	fadetolevel (rpvis, ONE_F16);	// Make sure we can read it.

	corner[0].pt_X = corner[3].pt_X = (msgbgcel->ccb_XPos)>>16;
	corner[0].pt_Y = corner[1].pt_Y = (msgbgcel->ccb_YPos)>>16;

	corner[1].pt_X = corner[2].pt_X = corner[0].pt_X+(msgbgcel->ccb_Width);
	corner[2].pt_Y = corner[3].pt_Y = corner[0].pt_Y+(msgbgcel->ccb_Height);
	FasterMapCel (msgbgcel, corner);
	DrawCels (rpvis->rp_BitmapItem, msgbgcel);

	TestText.BItem = rpvis->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

	TestText.CoordY = 70+7*(4-ngenmsgs);


	for (i = 0;  i < ngenmsgs;  i++) {
			TestText.TextPtr = genmsgs[i];
			TestText.CoordX = genmsgsx[i];
//			TestText.CoordY = 70+i*14;
			FontPrint(&TestText);
			TestText.CoordY += 14;
	}


	if (dlay < 0) {
		resetjoydata ();

		while (!(joytrigger & (ControlA | ControlB | ControlC))) {
			TestText.TextPtr = "Yes";
			TestText.CoordX = 130;
			TestText.CoordY = 60+4*14;
			if (yesorno)
				TestText.PLUTPtr = OptHiLitePLUT;
			else
				TestText.PLUTPtr = TestPLUT;
			FontPrint(&TestText);

			TestText.TextPtr = "No";
			TestText.CoordX = 170;
			if (yesorno)
				TestText.PLUTPtr = TestPLUT;
			else
				TestText.PLUTPtr = OptHiLitePLUT;
			FontPrint(&TestText);
			resetjoydata ();
			WaitVBL (vblIO, 1);

			if (joytrigger & ControlLeft)	yesorno = TRUE;
			if (joytrigger & ControlRight)	yesorno = FALSE;
		}
		dlay = yesorno;
	}


	else if (dlay > 0)
		/*
		 * Pause for specified number of VBlanks.
		 * (Gad, this is an easy habit to fall into...)
		 */
		WaitVBL (vblIO, dlay);
	else {
		/*
		 * Wait for user to press START/PAUSE button.
		 */
		resetjoydata ();
		while (!(joytrigger & ControlStart))
			WaitVBL (vblIO, 1);
	}

	if (takedown)
		closemessages ();
	resetjoydata ();

	return(dlay);
}

void
clearmessage()
{
	message_duration = 0;
}
