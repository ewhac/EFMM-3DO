/*  :ts=8 bk=0
 *
 * clip.c:	Clipping routines.
 *
 * Leo L. Schwab					9304.06
 */
#include <types.h>
#include <graphics.h>
#include <operamath.h>

#include "castle.h"

#include "app_proto.h"


extern int32	cy;

static frac16	clipconstant;



/***************************************************************************
 * Z Clipper.
 */
frac16
zclip (xv, zflags, crnr)
struct Vertex	**xv;
int		zflags;
struct Point	*crnr;
{
	static uint8	whatzit[16] = {
		0, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0
	};
	register int32	factor;

	if (!(factor = whatzit[zflags]))
		/*
		 * Either the wrong two verticies are crossing the Z plane,
		 * or more than two are crossing.  Reject the entire
		 * polygon.
		 */
		return (0);

	if (factor == 1) {
		/*
		 * P0 and/or P1 are behind the clipping plane.
		 * Clip imagery by early start & truncated count.
		 */
		if (ZCLIP - xv[1]->Z > ZCLIP - xv[0]->Z) {
			factor = DivSF16 (xv[2]->Z - ZCLIP,
					  xv[2]->Z - xv[1]->Z);
		} else {
			factor = DivSF16 (xv[3]->Z - ZCLIP,
					  xv[3]->Z - xv[0]->Z);
		}

		/*
		 * Compute new coordinates based on clip proportion.
		 */
		crnr[0].pt_X = xv[3]->X -
				MulSF16 (xv[3]->X - xv[0]->X, factor);
		crnr[0].pt_Y = xv[3]->Y -
				MulSF16 (xv[3]->Y - xv[0]->Y, factor);

		crnr[1].pt_X = xv[2]->X -
				MulSF16 (xv[2]->X - xv[1]->X, factor);
		crnr[1].pt_Y = xv[2]->Y -
				MulSF16 (xv[2]->Y - xv[1]->Y, factor);

		/*
		 * Project coordinates.
		 */
		crnr[0].pt_X =
		 ConvertF16_32 (MulSF16 (crnr[0].pt_X, clipconstant)) + CX;
		crnr[0].pt_Y =
		 ConvertF16_32 (MulSF16 (crnr[0].pt_Y, clipconstant)) + cy;

		crnr[1].pt_X =
		 ConvertF16_32 (MulSF16 (crnr[1].pt_X, clipconstant)) + CX;
		crnr[1].pt_Y =
		 ConvertF16_32 (MulSF16 (crnr[1].pt_Y, clipconstant)) + cy;

		return (-factor);

	} else {
		/*
		 * P2 and/or P3 are behind the clipping plane;
		 * Clip imagery by truncated count.
		 */
		if (ZCLIP - xv[3]->Z > ZCLIP - xv[2]->Z) {
			factor = DivSF16 (xv[0]->Z - ZCLIP,
					  xv[0]->Z - xv[3]->Z);
		} else {
			factor = DivSF16 (xv[1]->Z - ZCLIP,
					  xv[1]->Z - xv[2]->Z);
		}

		/*
		 * Compute new coordinates based on clip proportion.
		 */
		crnr[2].pt_X = xv[1]->X -
				MulSF16 (xv[1]->X - xv[2]->X, factor);
		crnr[2].pt_Y = xv[1]->Y -
				MulSF16 (xv[1]->Y - xv[2]->Y, factor);

		crnr[3].pt_X = xv[0]->X -
				MulSF16 (xv[0]->X - xv[3]->X, factor);
		crnr[3].pt_Y = xv[0]->Y -
				MulSF16 (xv[0]->Y - xv[3]->Y, factor);

		/*
		 * Project coordinates.
		 */
		crnr[2].pt_X =
		 ConvertF16_32 (MulSF16 (crnr[2].pt_X, clipconstant)) + CX;
		crnr[2].pt_Y =
		 ConvertF16_32 (MulSF16 (crnr[2].pt_Y, clipconstant)) + cy;

		crnr[3].pt_X =
		 ConvertF16_32 (MulSF16 (crnr[3].pt_X, clipconstant)) + CX;
		crnr[3].pt_Y =
		 ConvertF16_32 (MulSF16 (crnr[3].pt_Y, clipconstant)) + cy;

		return (factor);
	}
}


/***************************************************************************
 * Chops lines off top or bottom of cel using a proportional factor.
 *
 * 'factor' is a fixed-point fraction having SHIFT_FRAC bits of precision.
 * If factor<0, delete pixels from top; >0, delete from bottom.
 */
void
zClipCCB (ccb, factor)
register struct CCB	*ccb;
frac16			factor;
{
	register int32	high, newhigh, *img;

	img = (int32 *) ccb->ccb_SourcePtr;
	high = REALCELDIM (ccb->ccb_Height);

	if (!(ccb->ccb_Flags & CCB_CCBPRE)) {
		/*
		 * Oh really.  Well, you're a CCBPRE cel NOW, dude!!
		 */
		ccb->ccb_Flags |= CCB_CCBPRE;
		ccb->ccb_PRE0 = *img++;
		if (!(ccb->ccb_Flags & CCB_PACKED))
			ccb->ccb_PRE1 = *img++;

		ccb->ccb_SourcePtr = (CelData *) img;
	}

	if (factor < 0) {
		/*
		 * Cel must be chopped from top.  Do this by adjusting the
		 * CelData pointer to start at the line we want.
		 */
		register int	widebpp = FALSE;

		newhigh = ConvertF16_32 (high * -factor) + 1;
		if (newhigh == high)
			/*
			 * No change required.
			 */
			return;

		if ((ccb->ccb_PRE0 & PRE0_BPP_MASK) >= 5)
			/*  8 or 16 bits per pixel  */
			widebpp = TRUE;

		if (ccb->ccb_Flags & CCB_PACKED) {
			/*
			 * The icky version.  Iteratively skip variable-
			 * length lines until N lines are skipped.
			 */
			register int	i;

			for (i = high - newhigh;  --i >= 0; ) {
				if (widebpp)
					img += (*img >> 16) + 2;
				else
					img += (*img >> 24) + 2;
			}
		} else {
			/*
			 * The cleaner version.  Multiply the word width by
			 * the number of lines to skip to yield offset.
			 */
			register int	wpr;

			if (widebpp)
				wpr = ccb->ccb_PRE1 >> PRE1_WOFFSET10_SHIFT;
			else
				wpr = ccb->ccb_PRE1 >> PRE1_WOFFSET8_SHIFT;

			img += (wpr + 2) * (high - newhigh);
		}
		ccb->ccb_SourcePtr = (CelData *) img;

	} else {
		/*
		 * Very simple; all we need is early termination.
		 */
		newhigh = ConvertF16_32 (high * factor) + 1;
		if (newhigh == high)
			/*
			 * No change required.
			 */
			return;
	}

	/*
	 * Alter VCNT (CelData height) to force early termination, and
	 * adjust ccb_Height for MapCel().
	 */
	ccb->ccb_PRE0 = (ccb->ccb_PRE0 & ~PRE0_VCNT_MASK) |
			((newhigh - 1) << PRE0_VCNT_SHIFT);
	ccb->ccb_Height = newhigh;
}



/*
 * Assumes a 16-bit literal non-LRFORM cel.  IT DOESN'T CHECK!!  DO IT
 * RIGHT, OK?
 *
 * 'factor' is a fixed-point fraction having SHIFT_FRAC bits of precision.
 * If factor<0, delete pixels from left; >0, delete from right.
 */
void
xClipCCB (ccb, factor)
register struct CCB	*ccb;
int32			factor;
{
	register int32	*img, wide, newwide;

	img = (int32 *) ccb->ccb_SourcePtr;
	wide = REALCELDIM (ccb->ccb_Width);

	if (!(ccb->ccb_Flags & CCB_CCBPRE)) {
		/*
		 * Oh really.  Well, you're a CCBPRE cel NOW, dude!!
		 */
		ccb->ccb_Flags |= CCB_CCBPRE;
		ccb->ccb_PRE0 = *img++;
		if (!(ccb->ccb_Flags & CCB_PACKED))
			ccb->ccb_PRE1 = *img++;

		ccb->ccb_SourcePtr = (CelData *) img;
	}
	wide = (ccb->ccb_PRE1 & PRE1_TLHPCNT_MASK) + 1;

	if (factor < 0) {
		/*
		 * Delete pixels from left.  Decrease pixel count
		 * and advance imagery pointer.
		 */
		newwide = ConvertF16_32 (wide * -factor) + 1;

		/*
		 * Because we can only advance the pointer by a multiple of
		 * two pixels, we have to confine newwide to this limit,
		 * too.  Otherwise, the last pixel on the right may be
		 * mysteriously chopped.
		 */
		newwide += (wide - newwide) & 1;

		if (newwide == wide)
			// No change required.
			return;

		img += (wide - newwide) >> 1;
		ccb->ccb_SourcePtr = (CelData *) img;

	} else {
		/*
		 * Delete pixels from right.  Just a simple count reduction.
		 */
		newwide = ConvertF16_32 (wide * factor) + 1;

		if (newwide == wide)
			return;
	}

	ccb->ccb_PRE1 = (ccb->ccb_PRE1 & ~PRE1_TLHPCNT_MASK) | (newwide - 1);
	ccb->ccb_Width = newwide;
}


/***************************************************************************
 * Please call this function before attempting to do any Z clipping.
 */
void
initclip ()
{
	clipconstant = DivSF16 (Convert32_F16 (MAGIC), ZCLIP);
}
