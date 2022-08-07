/*  :ts=8 bk=0
 *
 * loadloaf.c:	Load a LOAF file.
 *
 * Leo L. Schwab					9304.26
 */
#include <types.h>
#include <mem.h>
#include <io.h>
#include <filestream.h>
#include <filestreamfunctions.h>
#include <hardware.h>
#include <debug.h>

#include "castle.h"
#include "loaf.h"

#include "app_proto.h"


#define	VERSION		2


/***************************************************************************
 * Forward function declarations.  (I hate ANSI.)
 */
static char *loadImageEntries (struct ImageEnv *, struct Stream *, int32);
static char *loadShortCCBs (struct ImageEnv *, struct Stream *, int32);
static char *loadPLUTs (struct ImageEnv *, struct Stream *, int32);


/***************************************************************************
 * Globals.
 */
extern uint32	ccbextra;


/***************************************************************************
 * Load a LOAF file.
 */
struct ImageEnv *
loadloaf (filename)
char	*filename;
{
	ImageEnv	*iev;
	Stream		*stream;
	int32		lval;
	char		*errstr = NULL;


	if (!(iev = AllocMem (sizeof (*iev), MEMTYPE_FILL))) {
		kprintf ("Can't allocate ImageEnv.\n");
		return (NULL);
	}

	if (!(stream = OpenDiskStream (filename, 0))) {
		kprintf (filename);
		errstr = ": File wouldn't open.\n";
		goto err;
	}

	/*
	 * Read ID.
	 */
	if (ReadDiskStream (stream, (char *) &lval, sizeof (lval)) !=
	    sizeof (lval))
		goto readerr;

	if (lval != 'LOAF') {
		kprintf (filename);
		errstr = " is not a LOAF file.\n";
		goto err;
	}

	/*
	 * Read version.
	 */
	if (ReadDiskStream (stream, (char *) &lval, sizeof (lval)) !=
	    sizeof (lval))
		goto readerr;

	if (lval > VERSION) {	// Current version.
		kprintf ("%s is version %d,", filename, lval);
		errstr = " which is too new for me.\n";
		goto err;
	}

	/*
	 * Read number of DiskImageEntries.
	 */
	if (ReadDiskStream (stream, (char *) &lval, sizeof (lval)) !=
	    sizeof (lval))
		goto readerr;

	if (lval)
		if (errstr = loadImageEntries (iev, stream, lval))
			goto err;

	/*
	 * Read number of PDATs.
	 */
	if (ReadDiskStream (stream, (char *) &lval, sizeof (lval)) !=
	    sizeof (lval))
		goto readerr;

	if (lval)
		if (errstr = loadShortCCBs (iev, stream, lval))
			goto err;


	/*
	 * Read number of PLUTs.
	 */
	if (ReadDiskStream (stream, (char *) &lval, sizeof (lval)) !=
	    sizeof (lval))
		goto readerr;

	if (lval)
		if (errstr = loadPLUTs (iev, stream, lval))
			goto err;


	if (0)
readerr:	errstr = "Error reading LOAF file.\n";
err:
	if (stream)	CloseDiskStream (stream);

	if (errstr) {
		kprintf (errstr);
		freeloaf (iev);
		iev = NULL;
	}

	return (iev);
}








static char *
loadImageEntries (iev, stream, n)
struct ImageEnv	*iev;
struct Stream	*stream;
int32		n;
{
	register ImageEntry	*ie;
	register int32		i, size;

	/*
	 * Load ImageEntries.
	 */
	size = sizeof (ImageEntry) * n;
	if (!(iev->iev_ImageEntries = AllocMem (size, 0)))
		return ("Can't allocate ImageEntry table.\n");
	iev->iev_NImageEntries = n;

	if (ReadDiskStream (stream, (char *) iev->iev_ImageEntries, size) !=
	    size)
		return ("Error reading ImageEntries.\n");

	/*
	 * Count AnimEntries.
	 */
	for (i = iev->iev_NImageEntries, ie = iev->iev_ImageEntries;
	     --i >= 0;
	     ie++)
		if (((DiskImageEntry *) ie)->die_SCCBIdx == ~0)
			iev->iev_NAnimEntries++;

	/*
	 * Load and initialize AnimEntries.
	 */
	if (iev->iev_NAnimEntries) {
		register AnimEntry	*ae;
		register DiskAnimEntry	*dae;

		size = sizeof (AnimEntry) * iev->iev_NAnimEntries;
		if (!(ae = AllocMem (size, 0)))
			return ("Can't allocate AnimEntry table.\n");
		iev->iev_AnimEntries = ae;

		for (i = iev->iev_NImageEntries, ie = iev->iev_ImageEntries;
		     --i >= 0;
		     ie++)
		{
			dae = (DiskAnimEntry *) ie;
			if (dae->dae_Marker == ~0) {
				/*
				 * Initialize AnimEntry.
				 */
				ae->ae_Base	= iev->iev_ImageEntries +
						  dae->dae_ImageIdx;
				ae->ae_Target	= ie;
				ae->ae_NFrames	= dae->dae_NFrames;
				ae->ae_FPS	= dae->dae_FPS;

				/*
				 * Set target ImageEntry to first frame.
				 */
				*ie = *ae->ae_Base;

				ae++;
			}
		}
	}

	return (NULL);
}


static char *
loadShortCCBs (iev, stream, n)
struct ImageEnv	*iev;
struct Stream	*stream;
int32		n;
{
	register ShortCCB	*sccb;
	register int		i;
	ImageEntry		*ie;
	int32			size;
	ubyte			*pdatptr;

	/*
	 * Allocate memory for ShortCCB table.
	 */
	size = sizeof (ShortCCB) * n;
	if (!(iev->iev_SCCBs = AllocMem (size, 0)))
		return ("Can't allocate ShortCCB buffer.\n");
	iev->iev_NSCCBs = n;


	/*
	 * Load DiskShortCCBs as if they were ShortCCBs (because they are
	 * (trust me, I'm a professional)).
	 */
	if (ReadDiskStream (stream, (char *) iev->iev_SCCBs, size) != size)
		return ("Error reading DiskShortCCBs.\n");

	/*
	 * Compute size of PDAT buffer.
	 */
	for (i = n, size = 0, sccb = iev->iev_SCCBs;  --i >= 0;  sccb++)
		size += ((DiskShortCCB *) sccb)->dsc_PDATSize;
	iev->iev_PDATBufSiz = size;

	/*
	 * Allocate PDAT buffer.
	 */
	if (!(iev->iev_PDATBuf = AllocMem (size, MEMTYPE_CEL)))
		return ("Can't allocate PDAT buffer.\n");

	/*
	 * Load PDAT buffer.
	 */
	if (ReadDiskStream (stream, (char *) iev->iev_PDATBuf, size) != size)
		return ("Error reading PDAT buffer.\n");

	/*
	 * Convert DiskShortCCBs to ShortCCBs.
	 */
	for (pdatptr = iev->iev_PDATBuf, i = n, sccb = iev->iev_SCCBs;
	     --i >= 0;
	     sccb++)
	{
		size = ((DiskShortCCB *) sccb)->dsc_PDATSize;
		sccb->sccb_PDAT = pdatptr;
		pdatptr += size;

		/*
		 * Fiddle with the flags 'n stuff.
		 */
		sccb->sccb_Flags |= CCB_NPABS | CCB_SPABS | CCB_PPABS |
				    CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP |
				    CCB_YOXY | CCB_ACW | CCB_ACCW |
				    ccbextra;
		sccb->sccb_Flags &= ~CCB_TWD;
		sccb->sccb_Width  = cvt2power (sccb->sccb_Width);
		sccb->sccb_Height = cvt2power (sccb->sccb_Height);
	}

	/*
	 * Convert ImageEntry ShortCCB indicies to pointers.
	 */
	for (i = iev->iev_NImageEntries, ie = iev->iev_ImageEntries;
	     --i >= 0;
	     ie++)
		ie->ie_SCCB = iev->iev_SCCBs +
			      ((DiskImageEntry *) ie)->die_SCCBIdx;

	return (NULL);
}


static char *
loadPLUTs (iev, stream, n)
struct ImageEnv	*iev;
struct Stream	*stream;
int32		n;
{
	register uint16	*p, **pp;
	register int	i;
	ImageEntry	*ie;
	int32		size;
	uint16		**pptrs;


	/*
	 * Read total size of PLUTs.
	 */
	if (ReadDiskStream
	     (stream, (char *) &size, sizeof (size)) != sizeof (size))
		return ("Error reading pdatsize.\n");
	iev->iev_PLUTBufSiz = size;

	/*
	 * Allocate PLUT buffer.
	 */
	if (!(iev->iev_PLUTBuf = AllocMem (size, MEMTYPE_CEL)))
		return ("Can't allocate PLUT buffer.\n");

	/*
	 * Allocate temporary pointer table.
	 */
	size = sizeof (int32) * n;
	if (!(pptrs = malloc (size)))
		return ("Can't allocate temporary buffer.\n");

	if (ReadDiskStream
	     (stream, (char *) iev->iev_PLUTBuf, iev->iev_PLUTBufSiz) !=
	    iev->iev_PLUTBufSiz)
		return ("Error reading PLUTs.\n");

	if (ReadDiskStream (stream, (char *) pptrs, size) != size)
		return ("Error reading PLUT sizes.\n");

	/*
	 * Convert table of PLUT sizes into pointers.
	 */
	p = iev->iev_PLUTBuf;
	pp = pptrs;
	for (i = n;  --i >= 0; ) {
		size = (int32) *pp;
		*pp++ = p;
		p += size;
	}

	/*
	 * Convert PLUT indicies in ImageEntries to pointers.
	 */
	for (i = iev->iev_NImageEntries, ie = iev->iev_ImageEntries;
	     --i >= 0;
	     ie++)
		ie->ie_PLUT = pptrs[((DiskImageEntry *) ie)->die_PLUTIdx];

	free (pptrs);

	return (NULL);
}


void
freeloaf (iev)
register struct ImageEnv	*iev;
{
	if (iev) {
		if (iev->iev_PLUTBuf) {
			FreeMem (iev->iev_PLUTBuf, iev->iev_PLUTBufSiz);
			iev->iev_PLUTBuf = NULL;
		}
		if (iev->iev_PDATBuf) {
			FreeMem (iev->iev_PDATBuf, iev->iev_PDATBufSiz);
			iev->iev_PDATBuf = NULL;
		}
		if (iev->iev_SCCBs) {
			FreeMem (iev->iev_SCCBs,
				 sizeof (ShortCCB) * iev->iev_NSCCBs);
			iev->iev_SCCBs = NULL;
		}
		if (iev->iev_AnimEntries) {
			FreeMem (iev->iev_AnimEntries,
				 sizeof (AnimEntry) * iev->iev_NAnimEntries);
			iev->iev_AnimEntries = NULL;
		}
		if (iev->iev_ImageEntries) {
			FreeMem (iev->iev_ImageEntries,
				 sizeof (ImageEntry) * iev->iev_NImageEntries);
			iev->iev_ImageEntries = NULL;
		}
		FreeMem (iev, sizeof (*iev));
	}
}
