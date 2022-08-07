/*
	Loafer (Leo Opera Art Format maker)

	Format: loafer <image list file> <output file>

	Created by Jon Leupp		June 18, 1993
	Modified by ewhac		9307.19
*/

#define	DBUG(x)		{ printf x ; }
#define	FULLDBUG(x)	/* { printf x ; } */

#include <types.h>
#include <ctype.h>

#include <debug.h>
#include <nodes.h>
#include <kernelnodes.h>
#include <list.h>
#include <folio.h>
#include <task.h>
#include <kernel.h>
#include <mem.h>
#include <semaphore.h>
#include <io.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <graphics.h>
#include <operamath.h>
#include <Form3DO.h>

#include "loafit.h"


/***************************************************************************
 * #defines
 */
#define	MAXIMAGES	100

#define	GETPDAT		0
#define	GETPLUT		1
#define	GETPPMP		2


/***************************************************************************
 * Prototypes.
 */
extern int	fwrite (void *, int, uint32, FILE *);

int32		loadcel (ubyte * spname);
void		usage (void);
void		terminate (int i);
int		getnextname (char *toptr);
uint32		getPDAT (void);
uint32		getPLUT (void);
void		writeLOAF (char *filename);
void		*loadfile (char *filename, int32 *sizptr);
int32		memcmp (void *this, void *that, int32 siz);


/***************************************************************************
 * Globals.
 */
char		BuildDate[] = "9307.19";

Item		timerior;
IOReq		*timerioreq;


char		*buffptr;

//loadcel vars
int32		celbufsize, *celbuffer, *p;
PixelChunk	*pxlc;
CCC		*ccc;
PLUTChunk	*plutc;



CCB			*ccbs[MAXIMAGES];
uint32			*cels[MAXIMAGES], *pluts[MAXIMAGES];


int32			nimages;
struct DiskShortCCB	DiskShortCCBs[MAXIMAGES];

int32			npdats;
struct DiskImageEntry	DiskImageEntries[MAXIMAGES];

int32			npluts;
uint32			totalplutsize;
int32			PLUTSizes[MAXIMAGES];




/***************************************************************************
 * Code.
 */
int
main (int argc, char **argv)
{
	int32	filesize;
	char	infile[80], outfile[80], imagename[80], PDATname[80],
		PLUTname[80], PPMPname[80];
	char	*iobuffer;

	DBUG (("Loafit\n%s\n", BuildDate));

	if (argc < 3)
		usage ();

	/*
	 * load list of PDATs PLUTs and PPMPs into memory.
	 */
	strcpy (infile, argv[1]);

	if (!(iobuffer = loadfile (infile, &filesize))) {
		DBUG (("loadfile failed at toplevel.\n"));
		return (-1);
	}
	buffptr = iobuffer;

//	*(buffptr + filesize) = 0;	// add terminating zero to buffer

	// load appropriate PDATs, PLUTs, and PPMPs into memory
	nimages = npdats = npluts = totalplutsize = 0;
	do {
		getnextname (imagename);
		if (strlen (imagename)  &&
		    (imagename[0] != '*')  &&
		    (imagename[0] != '#'))
		{
			printf ("[%02d] %s: ", nimages, imagename);
			if (!(getnextname (PDATname))) {
				DBUG (("* Error * Image %s missing PDAT name\n", imagename));
				terminate (1);
			}

			if (!strcasecmp (PDATname, "empty")) {
				/*
				 * Create empty placeholder.
				 */
				DiskImageEntries[nimages].die_ShortCCBIdx =
				DiskImageEntries[nimages].die_PLUTIdx =
				DiskImageEntries[nimages].die_PPMP = ~0;
				printf ("<empty>");
				goto imagedone;	// Look down.
			}

			loadcel (PDATname);
			DiskImageEntries[nimages].die_ShortCCBIdx =
			 getPDAT ();

			if (!(getnextname (PLUTname)))
				strcpy (PLUTname, PDATname);
			if (strcmp (PLUTname, PDATname)) {
				// what value means equal ? ? ? ?
				free (celbuffer);
				loadcel (PLUTname);
			}
			DiskImageEntries[nimages].die_PLUTIdx = getPLUT ();

			if (!(getnextname (PPMPname)))
				strcpy (PPMPname, PLUTname);
			if (PPMPname[0] == '0'  &&
			    ((PPMPname[1] == 'x')  || (PPMPname[1] == 'X')))
 			{
				DiskImageEntries[nimages].die_PPMP =
				 strtol (PPMPname, 0, 0);
			} else {
				if (strcmp (PPMPname, PLUTname)) {
					free (celbuffer);
					loadcel (PPMPname);
				}
				DiskImageEntries[nimages].die_PPMP =
				 ccc->ccb_PPMPC;
			}
// printf ("PPMP = 0x%lx", (&DiskImageEntries[nimages])->die_PPMP);
			free (celbuffer);

imagedone:		nimages++;
			printf ("\n");
		}

		/*
		 * Flush remainder of input line.
		 */
		while (*buffptr  &&  *buffptr != '\n'  &&  *buffptr != '\r')
			buffptr++;
		buffptr++;

	} while (strlen (imagename)  &&  strlen (PDATname));

	strcpy (outfile, argv[2]);
	writeLOAF (outfile);

	printf ("All done!\n");
	return (0);
}


void
writeLOAF (char *filename)
{
	FILE	*file;
	char	LOAFhdr[] = { 'L', 'O', 'A', 'F', 0, 0, 0, 1 };
	int	i;

	printf ("Opening file %s for writing; %d ImageEntries.\n",
		filename, nimages);
	if (!(file = fopen (filename, "w"))) {
		printf ("Could not open file %s.\n", filename);
		terminate (1);
	}

// need:	write out 'LOAF', 0, nimages
// for (i = 0; i < nimages; i++)
	fwrite (LOAFhdr, 1, sizeof (LOAFhdr), file);
	fwrite (&nimages, 1, sizeof (nimages), file);
	fwrite (DiskImageEntries, 1, nimages * sizeof (struct DiskImageEntry), file);
	fwrite (&npdats, 1, sizeof (npdats), file);
	fwrite (DiskShortCCBs, 1, npdats * sizeof (struct DiskShortCCB), file);
	for (i = 0;  i < npdats;  i++)
		fwrite (cels[i], 1, (&DiskShortCCBs[i])->dsc_PDATSize, file);
	fwrite (&npluts, 1, sizeof (npluts), file);
	fwrite (&totalplutsize, 1, sizeof (totalplutsize), file);
	for (i = 0;  i < npluts;  i++)
		fwrite (pluts[i], 1, PLUTSizes[i] * sizeof (uint16), file);
	fwrite (PLUTSizes, 1, npluts * sizeof (PLUTSizes[0]), file);

	fclose (file);
}


int
getnextname (char *toptr)
{
	char	c, *str;

	str = toptr;

	while ((c = (*buffptr++)) == ' ')
		;	// skip spaces

	-- buffptr;

	if (c)
		while ((c = (*buffptr))  &&
		       c != ','  &&  c != '\n'  &&  c != '\r')
		{
			*toptr++ = *buffptr++;
		}
	*toptr = 0;
// printf ("\n/*%s*\\\n", str);
	if (c == ',')
		buffptr++;
	return ((int) strlen (str));
}



int32
loadcel (ubyte *spname)
{
	ubyte	fname[80];

	strcpy (fname, spname);

	FULLDBUG (("Attempt to load cel file %s\n", fname));

	if (!(celbuffer = loadfile (fname, &celbufsize))) {
		DBUG (("Loadfile failed\n"));
		terminate (1);
	}
	p = celbuffer;

	ccc = 0;
	pxlc = 0;
	plutc = 0;
	while (celbufsize > 0) {
		int32	token, chunksize;

		token = p[0];
		chunksize = p[1];
		switch (token) {
		case CHUNK_CCB:
			ccc = (CCC *) p;
			FULLDBUG (("Cel header @ %lx\n", ccc));
			break;
		case CHUNK_PDAT:
			pxlc = (PixelChunk *) p;
			FULLDBUG (("Pixel data @ %lx\n", pxlc));
			break;
		case CHUNK_PLUT:
			plutc = (PLUTChunk *) p;
			FULLDBUG (("PLUT data @ %lx\n", plutc));
			break;
		default:
			break;
		}
		p += (chunksize) / 4;
		celbufsize -= chunksize;
	}
	if ((celbufsize < 0)  ||  (!ccc)  ||  (!pxlc)) {
		DBUG (("Error - invalid cel file (%s)\n", fname));
		terminate (1);
	}
	return (0);
}

uint32
getPDAT (void)
{
	int32	size, match, i;
	char	*srcptr, *destptr;

	size = pxlc->chunk_size - 8;

	for (i = 0, match = npdats;  i < match;  i++) {
// printf ("* i = %lx, size = %lx, dsc_PDATSize = %lx *\n", i, size, DiskShortCCBs[i].dsc_PDATSize);

		if (DiskShortCCBs[i].dsc_PDATSize >= size) {
			srcptr = (char *) pxlc->pixels;
			destptr = (char *) cels[i];
			if (!memcmp (srcptr, destptr, size)) {
				match = i;
				break;
			}
		}
	}

	if (match == npdats) {
		/*
		 * No match; create new one.
		 */
		DiskShortCCBs[npdats].dsc_Flags		= ccc->ccb_Flags;
		DiskShortCCBs[npdats].dsc_PRE0		= ccc->ccb_PRE0;
		DiskShortCCBs[npdats].dsc_PRE1		= ccc->ccb_PRE1;
		DiskShortCCBs[npdats].dsc_Width		= ccc->ccb_Width;
		DiskShortCCBs[npdats].dsc_Height	= ccc->ccb_Height;
		DiskShortCCBs[npdats].dsc_PDATSize	= size;
		cels[npdats] = (uint32 *) AllocMem (size, MEMTYPE_CEL);
		if (!cels[npdats]) {
			DBUG (("Error - unable to allocate cel image memory\n"));
			terminate (1);
		}
		memcpy (cels[npdats], pxlc->pixels, size);
		FULLDBUG (("celdata = %lx\n", cels[npdats]));
		npdats++;
		printf ("PDAT created, ");
	} else
		printf ("PDAT folded (%d), ", match);

// printf ("match = %lx, npdats = %lx\n", match, npdats);
	return (match);
}

uint32
getPLUT (void)
{
	int32	size, len, match, i;
	char	*srcptr, *destptr;

	len = plutc->numentries;
	size = len * sizeof (uint16);

	for (i = 0, match = npluts;  i < match;  i++) {
// printf ("* i = %lx, size = %lx, dsc_PDATSize = %lx *\n", i, size, PLUTSizes[i]);
		if (PLUTSizes[i] >= len) {
			srcptr = (char *) plutc->PLUT;
			destptr = (char *) pluts[i];
			if (!memcmp (srcptr, destptr, size)) {
				match = i;
				break;
			}
		}
	}

	if (match == npluts) {
		/*
		 * No match; create new one.
		 */
		pluts[npluts] = (uint32 *) AllocMem (size, MEMTYPE_CEL);
		if (!pluts[npluts]) {
			DBUG (("Error - unable to allocate plut memory\n"));
			terminate (1);
		}
		memcpy (pluts[npluts], plutc->PLUT, size);
		FULLDBUG (("plutdata = %lx\n", pluts[npluts]));
		totalplutsize += size;
		PLUTSizes[npluts++] = len;
		printf ("PLUT created");
	} else
		printf ("PLUT folded (%d)", match);

// printf ("match = %lx, npluts = %lx\n", match, npluts);
	return (match);
}


int32
memcmp (this, that, siz)
void		*this, *that;
register int32	siz;
{
	register int32	*a, *b, diff, mod;

	a = this;
	b = that;
	mod = siz & 3;
	siz >>= 2;

	while (--siz >= 0)
		if (diff = *a++ - *b++)
			break;

	while (--mod >= 0) {
		if (diff = *(ubyte *) a - *(ubyte *) b)
			break;
		a = (int32 *) ((ubyte *) a + 1);
		b = (int32 *) ((ubyte *) b + 1);
	}

	return (diff);
}


void
usage (void)
{
	DBUG (("\n********************\n"));
	DBUG (("\nUsage:\n"));
	DBUG (("\tloafit <image list> <output file>\n"));
	DBUG (("<image list> specifies a text file containing a list of the images to be\n"));
	DBUG ((" combined into a LOAF file.  The format for the input file list is:\n"));
	DBUG (("  image name, PDAT cel name [,PLUT cel name] [,PPMP cel name (or 0xPPMP)]\n\n"));
	DBUG (("  image name is an arbitrary name by which this image will be referenced internally.\n"));
	DBUG (("  If the PDAT cel name is \"empty\", then a blank placeholder will be saved.\n"));
	DBUG (("  If the PLUT cel name is not included it will default to the PDAT cel name.\n"));
	DBUG (("  If the PPMP cel name is not included it will default to the PLUT cel name.\n"));
	DBUG (("  If PPMP begins with 0x then it will treated as the actual PPMP value, not a cel name.\n\n"));
	DBUG (("Ex. pict1, pict1.cel, pict1.cel, pict1.cel\n"));
	DBUG (("or  pict1, pict1.cel\n"));
	DBUG ((" will save an image named pict1 with pixel, PLUT, and PPMP data all coming from pict1.cel.\n"));
	DBUG (("    pict2, pict2.cel, pict3.cel, 0x1f001f00\n"));
	DBUG ((" will save pict2 with pixel data from pict2.cel, PLUT from pict3.cel, and PPMP = 0x1f001f00\n"));
	DBUG (("    pict3, empty\n"));
	DBUG ((" will save an empty ImageEntry (for use with AnimLoaf)\n"));
	DBUG (("\n********************\n"));
	terminate (1);
}

void
terminate (int i)
{
	exit (i);
}




void *
loadfile (filename, sizptr)
char	*filename;
int32	*sizptr;
{
	FILE	*fd;
	int32	fsiz;
	ubyte	*buf;

	if (!(fd = fopen (filename, "r"))) {
		DBUG (("Error - unable to open file (%s)\n", filename));
		terminate (1);
	}

	/*
	 * Determine file size.
	 */
	fseek (fd, 0, 2);	// Seek to end.
	fsiz = ftell (fd);
	fseek (fd, 0, 0);	// Seek to beginning.

	if (!(buf = malloc (fsiz + 1))) {
		DBUG (("Error - can't allocate %d bytes for file %s\n", fsiz, filename));
		terminate (1);
	}

	if (fread (buf, fsiz, 1, fd) != 1) {
		DBUG (("Error - file I/O error loading %s\n", filename));
		terminate (1);
	}
	*(buf + fsiz) = '\0';	// Force NULL after whatever.

	fclose (fd);
	*sizptr = fsiz;

	return (buf);
}
