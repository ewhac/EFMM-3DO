/*  :ts=8 bk=0
 *
 * timing.c:	Temporary stuff for performance evaluation.
 *
 * Leo L. Schwab					9305.07
 */
#include <types.h>
#include <io.h>
#include <kernelnodes.h>
#include <graphics.h>
#include <time.h>

#include "castle.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define ONE_12_20  (1<<20)
#define ONE_16_16  (1<<16)


/***************************************************************************
 * Static globals.
 */
static int32	MyFontPLUT[] = {
	0x00006318, 0x63186318, 0x63186318, 0x63186318,
};

static CCB	MenuFontCCB = {
	/* ulong ccb_Flags; */
	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS
	 | CCB_LDPPMP | CCB_LDPLUT | CCB_YOXY | CCB_ACW | CCB_ACCW
	 | PMODE_ZERO,
/*???CCB_PACKED*/

	/* struct CCB *ccb_NextPtr; CelData *ccb_CelData; void *ccb_PLUTPtr; */
	&MenuFontCCB, NULL, MyFontPLUT,

	/* Coord ccb_X; Coord ccb_Y; */
	/* long ccb_HDX, ccb_HDY, ccb_VDX, ccb_VDY, ccb_DDX, ccb_DDY; */
	0,0,
	ONE_12_20,0, 0,ONE_16_16, 0,0,

	/* ulong ccb_PPMPC; */
	(PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),
};

static Font	MenuFont = {
	/* uint8     font_Height; */
	8,
	/* uint8     font_Flags; */
	FONT_ASCII | FONT_ASCII_UPPERCASE,
	/* CCB      *font_CCB; */
	&MenuFontCCB,
	/* FontChar *font_FontCharArray[]; */
	NULL,
};

static Item	timerior;
static IOReq	*timerioreq;


/***************************************************************************
 * Code.
 */
void
drawnumxy (bmi, num, x, y)
Item	bmi;
int32	num, x, y;
{
	GrafCon	gc;
	char	str[64];

	sprintf (str, "%ld", num);
	MoveTo (&gc, x, y);
	DrawText8 (&gc, bmi, str);
}


#if 0
void
initfont()
{
	register Font	*sysfont;

	sysfont = GetCurrentFont ();
	MenuFont.font_FontCharArray = sysfont->font_FontCharArray;
	SetCurrentFont (&MenuFont);
}



void
closefont()
{
	ResetCurrentFont ();
}
#endif



void
opentimer ()
{
	TagArg	targs[2];
	Item	TimerDevice;

	if ((TimerDevice = OpenItem
			    (FindNamedItem (MKNODEID (KERNELNODE,DEVICENODE),
					    "timer"), 0)) < 0)
	{
		kprintf ("Error opening timer device: ");
		PrintfSysErr (TimerDevice);
		die ("Ackphft!\n");
	}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *) TimerDevice;
	targs[1].ta_Tag = TAG_END;

	timerior = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), targs);
	timerioreq = LookupItem (timerior);
	if (!timerioreq) {
		kprintf ("Error creating timer IOReq: ");
		PrintfSysErr (timerior);
		die ("Ackphft!\n");
	}
}


void
gettime (struct timeval *tv)
{
	IOInfo	ioi;
	int32	error;

	ioi.ioi_Flags		=
	ioi.ioi_Flags2		= 0;
	ioi.ioi_Unit		= TIMER_UNIT_USEC;
	ioi.ioi_Command		= CMD_READ;
	ioi.ioi_Offset		= 0;
	ioi.ioi_Recv.iob_Buffer	= tv;
	ioi.ioi_Recv.iob_Len	= sizeof (*tv);
	ioi.ioi_Send.iob_Buffer	= NULL;
	ioi.ioi_Send.iob_Len	= 0;

	error = DoIO (timerior, &ioi);
	if (error < 0) {
		kprintf ("Error reading timer: ");
		PrintfSysErr (error);
	}
}


int32
subtime (tv1, tv2)
register struct timeval	*tv1, *tv2;
{
	register int32	result = 0;

	if (tv1->tv_sec != tv2->tv_sec)
		result = (tv1->tv_sec - tv2->tv_sec) * 1000000;

	result += tv1->tv_usec - tv2->tv_usec;

	return (result);
}
