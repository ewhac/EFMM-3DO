/***************************************************************\
*								*
* Monster Manor (Alice in ZombieLand) title sequence            *
*								*
* By:  Jon Leupp						*
*								*
* Last update:  1-June-93					*
*								*
* Copyright (c) 1993, The 3DO Company, Inc.                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/
#define SOUNDSTUFF

#include <types.h>
#include <ctype.h>

#include <kernel.h>
#include <kernelnodes.h>
#include <mem.h>
#include <io.h>
#include <graphics.h>
#include <operamath.h>
#include <event.h>
#include <time.h>
#include <Form3DO.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

// Sound Includes
#include <operror.h>
#include <filefunctions.h>
#include <audio.h>
#include <soundfile.h>


#include "castle.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define DBUG(x)		{ printf x ; }
#define FULLDBUG(x)	{ printf x ; }

#define MAKETAG(a,b,c,d) (((int32)(a)<<24)|((int32)(b)<<16)|((int32)(c)<<8)|(int32)(d))

#define MAXCELS		100
#define NBUFFERS	3
#define NUMSPRITES	(MAXCELS+1)

#define	CHUNK_SPRH	CHAR4LITERAL('S','P','R','H')
#define	CHUNK_PIPT	CHAR4LITERAL('P','I','P','T')

// Sound equates

#define	PRT(x)		{ printf x; }
#define	ERR(x)		PRT(x)

#define MAXAMPLITUDE	0x7FFF
#define AUDIODIR	"^"



//void opentimer (void);
//void gettime (uint32 timebuffer[2]);

void opentitlestuff (void);
void closetitlestuff (void);
void loadcel (ubyte *spname, int32 index);
void loadbackground (ubyte *backname);
void ccbtodispccb (uint32 ccbn, uint32 dispccbn);

void ClearScreen (void);
void terminate (int i);
void play (void);

void *malloctype (int32 size, uint32 memtype);
void freetype (void *ptr);


//extern char	BuildDate[];
extern RastPort	rports[];
extern Item	vblIO, sportIO;


static GrafCon	gc;

static Item	ScreenItems[NBUFFERS], BitmapItems[NBUFFERS],
		ScreenGroupItem;
static Bitmap	*Bitmaps[NBUFFERS];

static CCB	*ccbs[NUMSPRITES];
static uint32	*pluts[NUMSPRITES];
static uint32	*cels[NUMSPRITES];
static CCB	*dispccbs[NUMSPRITES];
static uint32	dispccb;

static TagArg	ScreenTags[] = {
	CSG_TAG_SPORTBITS,	(void*)0,
	CSG_TAG_DISPLAYHEIGHT,	(void*)240,
	CSG_TAG_SCREENCOUNT,	(void*)NBUFFERS,
	CSG_TAG_SCREENHEIGHT,	(void*)240,
	CSG_TAG_DONE,		(void*)0,
};

static uint32	savebuffer, currentbuffer, tempbuffer;

static PixelChunk	*pxlc;
static CCC		*ccc;
static PLUTChunk	*pipc;

static int32		numcels;

static int32		xtable[] = { 0, 0, 0, 0,  25,  50,   50,  30,  30 };
static int32		ytable[] = { 0, 0, 0, 0,  40,   0,   20,   0,  20 };

static ControlPadEventData cped;
static int32		bloodtextht;
static int32		EndSound;


static uint32		ScreenPages;


/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE	(10000)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto error; \
	}

#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto error; \
	}

#define NUMBLOCKS	(64)
#define BLOCKSIZE	(2048)
#define BUFSIZE		(NUMBLOCKS*BLOCKSIZE)
#define NUMBUFFS	(2)

int32 PlaySoundFile (char *FileName, int32 BufSize );
void SpoolSoundFileThread( void );

/********* Globals for Thread **********/
static Item	SpoolerThread;
static char	*gFileName;
static char	gCurrentDir[80];
static int32	gSignal1;
static Item	gMainTaskItem;


// End of Sound Equates







/***************************************************************\
*								*
* Code								*
*								*
\***************************************************************/


void
dotitlesequence ()
{
	int32 i;
// Sound variables
//
//	gFileName = "/remote/aiff/EerieBoing.aiff";

	gFileName = "aiff/LongTitle.aiff";
	EndSound = 0;

//  DBUG (("Zombie title program\n%s\n", BuildDate));

	i = GetDirectory (gCurrentDir, sizeof (gCurrentDir));
	opentitlestuff ();

	DisplayScreen (ScreenItems[savebuffer], ScreenItems[savebuffer]);


	play ();

	EndSound = 1;
	WaitSignal (gSignal1);

#ifdef SOUNDSTUFF
// Sound end code
	PRT(("Playback complete.\n"));
//
#endif

	closetitlestuff ();
}

void
opentitlestuff ()
{
	register int	i;
 	int32		Result;
  	int32		Priority;

#if 0
	FULLDBUG (("Create screen group\n"));
	ScreenTags[0].ta_Arg = (void*)GETBANKBITS(GrafBase->gf_ZeroPage);
	ScreenGroupItem = CreateScreenGroup (ScreenItems, ScreenTags);
	if (ScreenGroupItem<0) {
		DBUG (("Error: CreateScreenGroup() == %lx\n", ScreenGroupItem));
		terminate (1);
	}
	FULLDBUG (("Add screen group\n"));
	AddScreenGroup (ScreenGroupItem, NULL);
#endif

	for (i = 0;  i < NBUFFERS;  i++) {
#if 0
		Screen *screen;
		screen = (Screen*)LookupItem (ScreenItems[i]);
		if (screen==0) {
			DBUG (("Error: could not locate screen\n"));
			terminate (1);
		}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;
#endif

		BitmapItems[i] = rports[i].rp_BitmapItem;
		Bitmaps[i] = rports[i].rp_Bitmap;
		ScreenItems[i] = rports[i].rp_ScreenItem;
		FULLDBUG (("Screen[%d] buffer @ %08lx\n", i, Bitmaps[i]->bm_Buffer));
	}

	ScreenPages = (Bitmaps[0]->bm_Width * Bitmaps[0]->bm_Height * 2 +
		       GrafBase->gf_VRAMPageSize-1) /
		      GrafBase->gf_VRAMPageSize;


#ifdef SOUNDSTUFF

	// Sound code
	/* Get parent task Item so that thread can signal back. */
	gMainTaskItem = KernelBase->kb_CurrentTask->t.n_Item;

	/* Allocate a signal for each thread to notify parent task. */
	gSignal1 = AllocSignal( 0 );
	CHECKRESULT(gSignal1,"AllocSignal");

	Priority = 180;
	SpoolerThread = CreateThread ("SoundSpooler",
				      Priority,
				      SpoolSoundFileThread,
				      STACKSIZE);
	CHECKRESULT (SpoolerThread, "CreateThread");
#endif


	numcels = 0;

	savebuffer = 2;
	currentbuffer = 0;

	FULLDBUG (("Clear screen\n"));
	SetVRAMPages (sportIO,
		      Bitmaps[savebuffer]->bm_Buffer,
		      MakeRGB15Pair (0, 0, 0),
		      ScreenPages,
		      0xffffffff);
	ClearScreen ();

	// allocate display ccb memory
	for (i=0; i<NUMSPRITES; i++) {
		dispccbs[i] = (CCB *) ALLOCMEM (sizeof (CCB), MEMTYPE_CEL);
		if (!dispccbs[i]) {
			DBUG (("Error allocating CCBs\n"));
			terminate (1);
		}
		if (i) dispccbs[i-1]->ccb_NextPtr = dispccbs[i];
	}

	loadcel ("skull.dark", numcels++);
	loadcel ("skull.light", numcels++);

	loadcel ("hellhouse.dark", numcels++);
	loadcel ("hellhouse.light", numcels++);

	loadcel ("bloodtext", numcels++);
	bloodtextht = REALCELDIM (ccbs[numcels-1]->ccb_Height);
	loadcel ("lightning1a", numcels++);
	loadcel ("lightning1b", numcels++);
	loadcel ("lightning2a", numcels++);
	loadcel ("lightning2b", numcels++);

	return;

error:
	die ("CHECKRESULT failure.\n");
}


void
closetitlestuff ()
{
	register int	i;

	for (i = numcels;  --i >= 0; ) {
		freetype (pluts[i]);  pluts[i] = NULL;
		freetype (cels[i]);  cels[i] = NULL;
		freetype (ccbs[i]);  ccbs[i] = NULL;
	}

	for (i = NUMSPRITES;  --i >= 0; ) {
		if (dispccbs[i]) {
			FreeMem (dispccbs[i], sizeof (CCB));
			dispccbs[i] = NULL;
		}
	}

	if (SpoolerThread) {
		DeleteThread (SpoolerThread);
		SpoolerThread = 0;
	}
#if 0
	if (ScreenGroupItem) {
		RemoveScreenGroup (ScreenGroupItem);
		DeleteScreenGroup (ScreenGroupItem);
		ScreenGroupItem = 0;
	}
#endif
}


void
play (void)
{
	int32 lightning_X, flash, flashdelay=32, phase=0, count=0;
	int32 joy=0, oldjoy=0xffffffff, joyedge=0, frame;
	//  int32 currentcel=0;
	struct timeval	timein, timeout;
	int32			timerend;
	Point q[4];

	ccbtodispccb(0,0);	// 0 <- skull.dark
	ccbtodispccb(1,2);	// 1 <- hellhouse.dark
	ccbtodispccb(2,4);	// 2 <- bloodtext
	dispccbs[2]->ccb_Flags |= CCB_LAST;

	for (frame=3;frame<4000;frame++) {
		oldjoy = joy;
		GetControlPad (1,0,&cped);
		joy = cped.cped_ButtonBits;
		joyedge = joy&~oldjoy;

		if (joyedge&ControlStart) {
			return;
		}

		if (GetCurrentSignals () & gSignal1)
			return;

	/*****
		if (joyedge&JOYFIREA) {
			currentcel++;
			if (currentcel>=numcels) {
				currentcel = 0;
			}
		}

		if (joy&JOYLEFT) {
			ccbs[currentcel]->ccb_XPos -= 1<<16;
		}
		if (joy&JOYRIGHT) {
			ccbs[currentcel]->ccb_XPos += 1<<16;
		}

		if (joy&JOYUP) {
			ccbs[currentcel]->ccb_YPos -= 1<<16;
		}
		if (joy&JOYDOWN) {
			ccbs[currentcel]->ccb_YPos += 1<<16;
		}
	*****/

		flash=0;

		switch (phase) {
		case 0:
			count++;
	//		dispccbs[2]->ccb_PRE0 = ((dispccbs[2]->ccb_PRE0)& ~0xffc0);
	//		if (dispccbs[2]->ccb_PRE0 = ((dispccbs[2]->ccb_PRE0)& ~0xffc0)|((count-20)<<6);

			if (count < REALCELDIM (dispccbs[2]->ccb_Width) / 2) {
				q[0].pt_X = q[3].pt_X = 110-count;
				q[1].pt_X = q[2].pt_X = 110+count;
			}
			q[0].pt_Y = q[1].pt_Y = 83+count/4;
			if (q[0].pt_Y>40) q[0].pt_Y = q[1].pt_Y = 40;
			q[2].pt_Y = q[3].pt_Y = q[0].pt_Y+count;

			FasterMapCel (dispccbs[2], q);

			if (count == bloodtextht) phase++;
			break;

	// wait for lightning
		case 1:
			switch (--flashdelay) {
			case 7:
				lightning_X = ((rand()&255)-90)<<16;
			case 6:
				case 5:
				flash = 1;
			case 3:
			case 2:
			case 1:
				ccbtodispccb(0,1);	// 0 <- skull.light
				ccbtodispccb(1,3);	// 1 <- hellhouse.light
				ccbtodispccb(3,5+flash+(rand()&2));	// 3 <- lightning
				dispccbs[2]->ccb_Flags &= ~CCB_LAST;
				dispccbs[3]->ccb_Flags |= CCB_LAST;
				dispccbs[3]->ccb_XPos = lightning_X;
				break;
			case 0:
				flashdelay=(rand()&63)+(rand()&63)+10;
			case 4:
				ccbtodispccb(0,0);	// 0 <- skull.dark
				ccbtodispccb(1,2);	// 1 <- hellhouse.dark
				dispccbs[2]->ccb_Flags |= CCB_LAST;
				break;
			default:
				break;
			}
			break;
		}

		CopyVRAMPages (sportIO,
			       Bitmaps[currentbuffer]->bm_Buffer,
			       Bitmaps[savebuffer]->bm_Buffer,
			       ScreenPages,
			       0xffffffff);

		gettime (&timein);
		DrawCels (BitmapItems[currentbuffer], dispccbs[0]);
		gettime (&timeout);

		timerend = subtime (&timeout, &timein);

		if (joyedge&JOYFIREC) {
			DBUG (("%ld\n", timerend));
		}

		DisplayScreen (ScreenItems[currentbuffer],
			       ScreenItems[currentbuffer]);
		currentbuffer = 1-currentbuffer;
	}
}


void
loadcel (ubyte *spname, int32 index)
{
  ubyte fname[80];
  int32 celbufsize, *celbuffer, *p;


  strcpy (fname, "zombie/");
  strcat (fname, spname);

  FULLDBUG (("Attempt to load cel file %s\n", fname));

	if (!(celbuffer = allocloadfile (fname, 0, &celbufsize))) {
		filerr (fname, celbufsize);
		terminate (1);
	}

#if 0
/*  The old stuff...  */
  celbufsize = getfilesize (fname);
  if (celbufsize < 8) {
    DBUG (("Error - unable to load cel file (%s)\n", fname));
    terminate (1);
  }
  celbuffer = (int32 *)malloc(celbufsize);
  if (!celbuffer) {
    DBUG (("Unable to allocate buffer memory for cel (%s)\n", fname));
    terminate (1);
  }
  FULLDBUG (("celbuffer = %lx\n", celbuffer));
  if (!loadfile(fname, celbuffer, celbufsize, 0)) {
    DBUG (("Error - unable to load cel file (%s)\n", fname));
    terminate (1);
  }
#endif

  p = celbuffer;

  ccc = 0;
  pxlc = 0;
  pipc = 0;
  while (celbufsize > 0) {
    int32 token, chunksize;

    token = p[0];
    chunksize = p[1];
    switch (token) {
    case CHUNK_CCB:
    case CHUNK_SPRH:
      ccc = (CCC *)p;
      FULLDBUG (("Cel header @ %lx\n", ccc));
      break;

    case CHUNK_PDAT:
      pxlc = (PixelChunk *)p;
      FULLDBUG (("Pixel data @ %lx\n", pxlc));
      break;

    case CHUNK_PLUT:
    case CHUNK_PIPT:
      pipc = (PLUTChunk *)p;
      FULLDBUG (("PLUT data @ %lx\n", pipc));
      break;

    default:
      break;
    }
    p += (chunksize)/4;
    celbufsize -= chunksize;
  }
  if ((celbufsize<0) || (!ccc) || (!pxlc)) {
    DBUG (("Error - invalid cel file (%s)\n", fname));
    terminate (1);
  }

  ccbs[index] = (CCB *) malloctype (sizeof(CCB), MEMTYPE_DMA|MEMTYPE_CEL);
  if (!ccbs[index]) {
    DBUG (("Error - unable to allocate ccb memory (%s)\n", fname));
    terminate (1);
  }
  memcpy (ccbs[index], &ccc->ccb_Flags, sizeof (CCB));
  FULLDBUG (("ccb = %lx\n", ccbs[index]));

  cels[index] = (uint32 *) malloctype (pxlc->chunk_size-8, MEMTYPE_DMA|MEMTYPE_CEL);
  if (!cels[index]) {
    DBUG (("Error - unable to allocate cel image memory (%s)\n", fname));
    terminate (1);
  }
  memcpy (cels[index], pxlc->pixels, pxlc->chunk_size-8);
  FULLDBUG (("celdata = %lx\n", cels[index]));

  if (pipc) {
    pluts[index] = (uint32 *) malloctype (pipc->chunk_size-8, MEMTYPE_DMA|MEMTYPE_CEL);
    if (!pluts[index]) {
      DBUG (("Error - unable to allocate plut memory (%s)\n", fname));
      terminate (1);
    }
    memcpy (pluts[index], pipc->PLUT, pipc->chunk_size-8);
  }
  FULLDBUG (("plut = %lx\n", pluts[index]));

  ccbs[index]->ccb_SourcePtr = (CelData *)cels[index];
  ccbs[index]->ccb_PLUTPtr = pluts[index];

  ccbs[index]->ccb_Flags |= CCB_SPABS|CCB_PPABS|CCB_NPABS;
  ccbs[index]->ccb_XPos = xtable[index%(sizeof(xtable)/sizeof(*xtable))]<<16;
  ccbs[index]->ccb_YPos = ytable[index%(sizeof(ytable)/sizeof(*ytable))]<<16;
  ccbs[index]->ccb_HDX = 1<<20;
  ccbs[index]->ccb_HDY = 0<<20;
  ccbs[index]->ccb_VDX = 0<<16;
  ccbs[index]->ccb_VDY = 1<<16;
  ccbs[index]->ccb_HDDX = 0<<20;
  ccbs[index]->ccb_HDDY = 0<<20;

  FreeMem (celbuffer, celbufsize);

  return;
}



void
ccbtodispccb (uint32 dispccbn, uint32 ccbn)
{
  memcpy (dispccbs[dispccbn], ccbs[ccbn], sizeof(CCB));
  dispccbs[dispccbn]->ccb_NextPtr = dispccbs[dispccbn+1];
  ++dispccb;

// any of this necessary?
	dispccbs[dispccbn]->ccb_Flags=ccbs[ccbn]->ccb_Flags;
	dispccbs[dispccbn]->ccb_SourcePtr=ccbs[ccbn]->ccb_SourcePtr;
	dispccbs[dispccbn]->ccb_Width=ccbs[ccbn]->ccb_Width;
	dispccbs[dispccbn]->ccb_Height=ccbs[ccbn]->ccb_Height;
	dispccbs[dispccbn]->ccb_PLUTPtr=ccbs[ccbn]->ccb_PLUTPtr;
	dispccbs[dispccbn]->ccb_PIXC=ccbs[ccbn]->ccb_PIXC;
	dispccbs[dispccbn]->ccb_PRE0=ccbs[ccbn]->ccb_PRE0;
	dispccbs[dispccbn]->ccb_PRE1=ccbs[ccbn]->ccb_PRE1;

  (dispccbs[dispccbn]->ccb_Flags) &= ~CCB_LAST;
}


void
loadbackground (ubyte *backname)
{
  ubyte fname[80];
  int32 bufsize, *buffer, *p;
  ImageCC *icc;
  PixelChunk *pxlc;

  strcpy (fname, backname);

  FULLDBUG (("Attempt to load background image file\n"));

	if (!(buffer = allocloadfile (fname, MEMTYPE_CEL, &bufsize))) {
		filerr (fname, bufsize);
		terminate (1);
	}

#if 0
/*  The old stuff...  */
  bufsize = getfilesize (fname);
  if (bufsize < 8) {
    DBUG (("Error - unable to load background image file (%s)\n", fname));
    terminate (1);
  }
  buffer = (int32 *)malloc(bufsize);
  if (!buffer) {
    DBUG (("Unable to allocate buffer memory for background (%s)\n", fname));
    terminate (1);
  }
  FULLDBUG (("buffer = %lx\n", buffer));
  if (!loadfile(fname, buffer, bufsize, 0)) {
    DBUG (("Error - unable to load background image file (%s)\n", fname));
    terminate (1);
  }
#endif

  p = buffer;

  icc = 0;
  pxlc = 0;
  while (bufsize > 0) {
    int32 token, chunksize;

    token = p[0];
    chunksize = p[1];
    switch (token) {
    case CHUNK_IMAG:
      icc = (ImageCC *)p;
      FULLDBUG (("Image header chunk at %lx\n", icc));
      break;

    case CHUNK_PDAT:
      pxlc = (PixelChunk *)p;
      FULLDBUG (("Image data chunk at %lx\n", pxlc->pixels));
      break;

    default:
      break;
    }
    p += (chunksize)/4;
    bufsize -= chunksize;
  }
  if ((bufsize<0) || (!icc) || (!pxlc)) {
    DBUG (("Error - invalid image file (%s)\n", fname));
    terminate (1);
  }

  if ((icc->w!=Bitmaps[savebuffer]->bm_Width) || (icc->h!=Bitmaps[savebuffer]->bm_Height)
      || (icc->bitsperpixel!=16) || (icc->numcomponents!=3) || (icc->numplanes!=1)
      || (icc->colorspace!=0) || (icc->comptype!=0) || (icc->pixelorder!=1)) {
    DBUG (("Error - image must be %dx%d normal screen format image\n",
	   Bitmaps[savebuffer]->bm_Width, Bitmaps[savebuffer]->bm_Height));
    terminate (1);
  }

  memcpy (Bitmaps[savebuffer]->bm_Buffer, pxlc->pixels,
	  Bitmaps[savebuffer]->bm_Width*Bitmaps[savebuffer]->bm_Height*2);

  FreeMem (buffer, bufsize);
}


void
ClearScreen (void)
{
	CopyVRAMPages (sportIO,
		       Bitmaps[currentbuffer]->bm_Buffer,
		       Bitmaps[savebuffer]->bm_Buffer,
		       ScreenPages,
		       ~0);
}


void
fadetoblack (rp, frames)
register struct RastPort	*rp;
int32				frames;
{
	int32	j, k, l, m;

	for (j = frames;  --j >= 0; ) {
		WaitVBL (vblIO, 1);
		k = j * 255 / (frames - 1);
		for (l = 0;  l < 32;  l++) {
			m = k * l / 31;
			SetScreenColor (rp->rp_ScreenItem,
					MakeCLUTColorEntry (l, m, m, m));
		}
		SetScreenColor (rp->rp_ScreenItem,
				MakeCLUTColorEntry (32, 0, 0, 0));
	}
	SetRast (rp, 0);
	ResetScreenColors (rp->rp_ScreenItem);
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



void
terminate (int i)
{
  if (!i) {
    fadetoblack (&rports[1-currentbuffer], 32);
  }
  closestuff ();
  exit (i);
}



#ifdef SOUNDSTUFF

/**************************************************************************
** PlaySoundFile - hangs until finished so rewrite and stick in a thread
**************************************************************************/
void SpoolSoundFileThread( void )
{
	Item NewDir;
	int32 Result;

	/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
	}

/* Set directory for audio data files. */
	NewDir = ChangeDirectory(gCurrentDir);
	if (NewDir > 0)
	{
		PRT(("Directory changed to %s for Mac based file-system.\n", AUDIODIR));
	}
	else
	{
		ERR(("Directory %s not found! Err = $%x\n", AUDIODIR, NewDir));
	}


	Result = PlaySoundFile ( gFileName, BUFSIZE );

	CloseAudioFolio();

	SendSignal( gMainTaskItem, gSignal1 );
	WaitSignal(0);   /* Waits forever. Don't return! */

}

int32 PlaySoundFile (char *FileName, int32 BufSize )
{
	int32 Result;
	SoundFilePlayer *sfp;
	int32 SignalIn=0, SignalsNeeded=0;

	sfp = OpenSoundFile(FileName, NUMBUFFS, BufSize);
	CHECKPTR(sfp, "OpenSoundFile");

	Result = StartSoundFile( sfp, MAXAMPLITUDE );
	CHECKRESULT(Result,"StartSoundFile");

/* Keep playing until no more samples. */
	do
	{
		if (SignalsNeeded) SignalIn = WaitSignal(SignalsNeeded);
		Result = ServiceSoundFile(sfp, SignalIn, &SignalsNeeded);
		CHECKRESULT(Result,"ServiceSoundFile");
	} while (SignalsNeeded  &&  (!EndSound));

	Result = StopSoundFile (sfp);
	CHECKRESULT(Result,"StopSoundFile");

	Result = CloseSoundFile (sfp);
	CHECKRESULT(Result,"CloseSoundFile");

	return 0;

error:
	return (Result);
}

#endif
