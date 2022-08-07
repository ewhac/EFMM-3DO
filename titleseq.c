/*  :ts=8 bk=0
 *
 * titleseq.c:	Title sequence.
 *
 * Leo L. Schwab					9309.12
 */
#include <types.h>
#include <graphics.h>

#include "castle.h"
#include "imgfile.h"
#include "flow.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define	XMARGIN		((320 - 256) / 2)
#define	YMARGIN		((240 - 196) / 2)


/***************************************************************************
 * Globals.
 */
extern RastPort	*rpvis, *rprend;
extern JoyData	jd;
extern Item	vblIO;
extern int32	wide, high;

extern uint32	ccbextra;


/***************************************************************************
 * Code
 */
int
dotitle ()
{
	register CCB	*ccb;
	CelArray	*ca_logo;

	/*
	 * Load stupid damn contemptability logo.
	 */
	if (!(ca_logo = parse3DO ("$progdir/LaurieProbstMadeMeUseThis")))
		die ("Laurie's out to lunch.\n");

	ccb = ca_logo->celptrs[0];
	ccb->ccb_Flags |= CCB_LAST | CCB_NPABS | CCB_LDSIZE |
			  CCB_LDPRS | CCB_LDPPMP | CCB_YOXY |
			  CCB_ACW | ccbextra;
	ccb->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

	/*
	 * Position and render logo, and let it languish there for three
	 * seconds.
	 */
	ccb->ccb_XPos = ((wide - REALCELDIM (ccb->ccb_Width)) / 2) << 16;
	ccb->ccb_YPos = (high - YMARGIN - REALCELDIM (ccb->ccb_Height)) << 16;

	fadetolevel (rpvis, 0);
	SetRast (rpvis, 0);
	DisplayScreen (rpvis->rp_ScreenItem, 0);
	DrawCels (rpvis->rp_BitmapItem, ccb);

	fadeup (rpvis, 32);
	WaitVBL (vblIO, 180);
	fadetoblank (rpvis, 32);

	freecelarray (ca_logo);

	/*
	 * Play CinePak intro.
	 */
	if (playcpak ("$progdir/Streams/PublisherLogo"))
		return (FC_USERABORT);

	if (playcpak ("$progdir/Streams/Intro"))
		return (FC_USERABORT);

	return (FC_NOP);
}
