/*  :ts=8 bk=0
 *
 * thread.c:	Background timing/joypad reading.
 *
 * Leo L. Schwab					9307.08
 */
#include <types.h>
#include <io.h>
#include <kernelnodes.h>
#include <time.h>
#include <event.h>
#include <graphics.h>

#include "castle.h"

#include "app_proto.h"




/***************************************************************************
 * Globals.  (All one of 'em.)
 */
JoyData	jd;
int32	joytrigger;	// jml
int32	oldjoybits = 0; // jml


/***************************************************************************
 * Execûtiõnièr des Köde.
 */
void
joythreadfunc ()
{
	register int32		nframes, fcount, result;
	register int32		joybits;
	ControlPadEventData	cped;
	TagArg			targs[2];
	IOInfo			ioi;
	IOReq			*timerioreq;
	Item			timerior;
	Item			TimerDevice;

	/*
	 * Initialization.
	 */
	if (InitEventUtility (1, 0, LC_FocusListener) < 0) {
		kprintf ("Can't open event broker.\n");
		return;
	}

	if ((TimerDevice = OpenItem
			    (FindNamedItem (MKNODEID (KERNELNODE,DEVICENODE),
					    "timer"), 0)) < 0)
	{
		kprintf ("Error opening timer device: ");
		PrintfSysErr (TimerDevice);
		return;
	}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *) TimerDevice;
	targs[1].ta_Tag = TAG_END;

	timerior = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), targs);
	timerioreq = LookupItem (timerior);
	if (!timerioreq) {
		kprintf ("Error creating timer IOReq: ");
		PrintfSysErr (timerior);
		return;
	}

	while (1) {
		/*
		 * Delay for one VBlank.
		 */
		ioi.ioi_Flags		=
		ioi.ioi_Flags2		= 0;
		ioi.ioi_Unit		= 0;
		ioi.ioi_Command		= TIMERCMD_DELAY;
		ioi.ioi_Offset		= 1;
		ioi.ioi_Recv.iob_Buffer	= NULL;
		ioi.ioi_Recv.iob_Len	= 0;
		ioi.ioi_Send.iob_Buffer	= NULL;
		ioi.ioi_Send.iob_Len	= 0;
		if ((result = DoIO (timerior, &ioi)) < 0) {
			kprintf ("IO transaction error: ");
			PrintfSysErr (result);
			return;
		}
		if (timerioreq->io_Error) {
			kprintf ("Timer I/O error %d\n",
				 timerioreq->io_Error);
			return;
		}

		/*
		 * Calculate number of vblanks that have happened.
		 */
		result = GrafBase->gf_VBLNumber;
		nframes = result - fcount;
		fcount = result;

		/*
		 * Accumulate joypad moves.
		 */
		if (GetControlPad (1, FALSE, &cped) < 0) {
			kprintf ("GetControlPad() failed.\n");
			return;
		}
		joybits = cped.cped_ButtonBits;
		joytrigger |= (joybits ^ oldjoybits) & joybits; // jml
		oldjoybits = joybits; 				// jml

		if (joybits & ControlLeft)	 jd.jd_DAng += nframes;
		else if (joybits & ControlRight) jd.jd_DAng -= nframes;

		if (joybits & ControlUp)	 jd.jd_DZ += nframes;
		else if (joybits & ControlDown)	 jd.jd_DZ -= nframes;

		if (joybits & ControlLeftShift)	 jd.jd_DX -= nframes;
		if (joybits & ControlRightShift) jd.jd_DX += nframes;

		if (joybits & ControlA)		jd.jd_ADown += nframes;
		if (joybits & ControlB)		jd.jd_BDown += nframes;
		if (joybits & ControlC)		jd.jd_CDown += nframes;
		if (joybits & ControlX)		jd.jd_XDown += nframes;
		if (joybits & ControlStart)	jd.jd_StartDown += nframes;

		/*
		 * Accumulate time.
		 */
		jd.jd_FrameCount += nframes;
	}
}

void
resetjoydata ()
{
	jd.jd_DX	=
	jd.jd_DZ	=
	jd.jd_DAng	=
	jd.jd_ADown	=
	jd.jd_BDown	=
	jd.jd_CDown	=
	jd.jd_XDown	=
	jd.jd_StartDown	=
	jd.jd_FrameCount= 0;

	joytrigger = 0; // jml
}
