/*  :ts=8 bk=0
 *
 * imgfile.c:	Routines to parse and load 3DO image files.
 *
 * Leo L. Schwab					9301.04
 */
#include <types.h>
#include <mem.h>
#include <graphics.h>
#include <form3do.h>

#include "castle.h"
#include "imgfile.h"

#include "app_proto.h"



typedef struct memfile {
	int32	*start;
	int32	*end;
	int32	*scan;
} memfile;


/***************************************************************************
 * Local function prototypes.  (I hate ANSI.)
 */
static CelArray *parseanim(memfile *);
static CelArray *parsecat(memfile *);
static void filloutccb(CCB *, void *, int32, void *, int32);


/***************************************************************************
 * Parsing code.
 */
struct CelArray *
parse3DO (filename)
char *filename;
{
	memfile	mf;
	int32	errlen;

	/*
	 * Load file.
	 */
	if (!(mf.start = allocloadfile (filename, MEMTYPE_CEL, &errlen)))
	{
		filerr (filename, errlen);
		return (NULL);
	}

	mf.scan = mf.start;
	mf.end = (int32 *) ((ubyte *) mf.start + errlen);

	if (*mf.scan == CHUNK_ANIM)
		return (parseanim (&mf));
	else if (*mf.scan == CHUNK_CCB)
		return (parsecat (&mf));
	else
		die ("WTFIGO?\n");
}


struct CelArray *
alloccelarray (nentries)
int32	nentries;
{
	return (AllocMem (sizeof (CelArray) +
			   (nentries - 1) * sizeof (CCB *),
			  0));
}


static CelArray *
parseanim (mf)
memfile	*mf;
{
	register AnimChunk	*ac;
	register CelArray	*cels;
	register int32		*buf;
	register int		i;
	CCB			*ccb;
	int32			curplutsiz;
	void			*curplut;

	/*
	 * Allocate CelArray.
	 */
	buf = mf->scan;

	ac = (AnimChunk *) buf;
	i = ac->numFrames;
	if (!(cels = alloccelarray (i)))
		die ("Can't allocate cel array (parseanim).\n");

	cels->ncels = i;
	if (ac->animType == 1) {
		/*
		 * The file contains only one CCB.  Create new ones.
		 */
		if (!(ccb = ALLOCMEM (sizeof (CCB) * (i - 1),
				      MEMTYPE_CEL | MEMTYPE_FILL)))
			die ("Can't allocate CCB's to load imagery.\n");

		/*
		 * Initialize pointer array.
		 */
		while (--i > 0)
			cels->celptrs[i] = ccb + i - 1;

		cels->ca_Buffer = mf->start;
		cels->ca_BufSiz = (int) mf->end - (int) mf->start;
		cels->ca_Type = CHUNK_ANIM;
	}

	ccb = curplut = NULL;
	i = 0;

	buf = (int32 *) ((ubyte *) buf + *(buf + 1));
	while (i >= 0) {
		switch (*buf) {
		case CHUNK_CCB:
		 {
			/*
			 * We've found the one and only CCB in this file.
			 * Replicate it.
			 */
			register int	n;

			ccb = cels->celptrs[0] =
			 (CCB *) (&((CCC *) buf)->ccb_Flags);
			for (n = ac->numFrames;  --n > 0; )
				*cels->celptrs[n] = *ccb;
			break;
		 }
		case CHUNK_PLUT:
			curplut = &((PLUTChunk *) buf)->PLUT;
			curplutsiz = ((PLUTChunk *) buf)->numentries;
			break;

		case CHUNK_PDAT:
			filloutccb (ccb,
				    curplut, curplutsiz,
				    buf + 2, *(buf + 1) - 8);
			if (++i >= ac->numFrames)
				i = -1;		/*  Force loop exit  */
			ccb = cels->celptrs[i];
			break;
		}
		buf = (int32 *) ((ubyte *) buf + *(buf + 1));
	}
	mf->scan = buf;

	return (cels);
}


static CelArray *
parsecat (mf)
memfile	*mf;
{
	register CelArray	*cels;
	register int32		*buf;
	register int		ncels, i;
	CCB			*curccb;
	int32			curplutsiz, curpdatsiz;
	void			*curplut, *curpdat;

	buf = mf->scan;
	ncels = 0;

	/*
	 * Count number of cels in file.
	 */
	while (1) {
		if (buf >= mf->end  ||
		    *buf == CHUNK_ANIM)
			break;

		if (*buf == CHUNK_CCB)
			ncels++;

		buf = (int32 *) ((ubyte *) buf + *(buf + 1));
	}

	/*
	 * Allocate CelArray.
	 */
	if (!(cels = alloccelarray (ncels)))
		die ("Can't allocate cel array (parsecat).\n");

	/*
	 * Fill CelArray.
	 */
	cels->ca_Buffer = mf->start;
	cels->ca_BufSiz = (int) mf->end - (int) mf->start;
	cels->ca_Type = CHUNK_CCB;
	cels->ncels = ncels;
	buf = mf->scan;
	i = 0;
	curccb = curplut = curpdat = NULL;
	while (buf < mf->end) {
		switch (*buf) {
		case CHUNK_CCB:
			if (curccb) {
				filloutccb (curccb,
					    curplut, curplutsiz,
					    curpdat, curpdatsiz);
				cels->celptrs[i] = curccb;
				ncels--;
				i++;
			}
			curccb = (CCB *) (&((CCC *) buf)->ccb_Flags);
			break;

		case CHUNK_PLUT:
			curplut = &((PLUTChunk *) buf)->PLUT;
			curplutsiz = ((PLUTChunk *) buf)->numentries;
			break;

		case CHUNK_PDAT:
			curpdat = buf + 2;
			curpdatsiz = *(buf + 1) - 8;	// This are the size.
			break;
		}

		buf = (int32 *) ((ubyte *) buf + *(buf + 1));
	}
	/*
	 * Possibly redundant step, depending on file layout.
	 */
	filloutccb (curccb, curplut, curplutsiz, curpdat, curpdatsiz);
	cels->celptrs[i] = curccb;

	return (cels);
}


static void
filloutccb (ccb, plut, plutsiz, pdat, pdatsiz)
register CCB	*ccb;
void		*plut, *pdat;
int32		plutsiz, pdatsiz;
{
	if (ccb->ccb_Flags & CCB_LDPLUT) {
		ccb->ccb_PLUTPtr = plut;
		ccb->ccb_Flags |= CCB_PPABS;
	}

	ccb->ccb_SourcePtr = pdat;
	ccb->ccb_Flags |= CCB_SPABS;

	ccb->ccb_HDX = ONE_HD;
	ccb->ccb_VDY = ONE_VD;

	ccb->ccb_HDY =
	ccb->ccb_VDX =
	ccb->ccb_HDDX =
	ccb->ccb_HDDY = 0;

	ccb->ccb_Width	= cvt2power (ccb->ccb_Width);
	ccb->ccb_Height	= cvt2power (ccb->ccb_Height);
}


void
freecelarray (ca)
register struct CelArray *ca;
{
	if (ca) {
		if (ca->ca_Type == CHUNK_ANIM)
			FreeMem (ca->celptrs[1],
				 sizeof (CCB) * (ca->ncels - 1));

		if (ca->ca_Buffer)
			FreeMem (ca->ca_Buffer, ca->ca_BufSiz);

		FreeMem (ca, sizeof(*ca) + sizeof (CCB *) * (ca->ncels - 1));
	}
}

int32
cvt2power (val)
register int32	val;
{
	register int32	i;

	for (i = 32;  --i >= 0; )
		if (val == (1 << i))
			return (-i);

	return (val);
}
