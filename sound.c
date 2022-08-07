/*  :ts=8 bk=0
 *
 * sound.c:	Sound initialization and access.
 *
 * Leo L. Schwab					9308.04
 */
#include <types.h>
#include <strings.h>

#include "castle.h"
#include "sound.h"
#include "soundinterface.h"

#include "app_proto.h"


#define	SOUNDINTERFACE_WORKS


/***************************************************************************
 * Local prototypes.
 */
#ifndef	SOUNDINTERFACE_WORKS
static void loaddummies(void);
static void dropdummies(void);
#endif


/***************************************************************************
 * Externals.
 */
extern int8	domusic, dosfx;


/***************************************************************************
 * Sound effects filenames.
 */
static char	*ramfx[] = {
	"gunzap.aifc",				/* SFX_GUNZAP = 1, */
	"gundrained.aifc",			/* SFX_GUNDRAINED, */
	"gunempty.aifc",			/* SFX_GUNEMPTY, */
	"doorlocked.aifc",			/* SFX_DOORLOCKED, */
	"opendoor.aifc",			/* SFX_OPENDOOR, */
	"closedoor.aifc",			/* SFX_CLOSEDOOR, */
	"floortrigger.aifc",			/* SFX_FLOORTRIGGER, */
	"grabkey.aifc",				/* SFX_GRABKEY, */
	"grabhealth.aifc",			/* SFX_GRABHEALTH, */
	"grabammo.aifc",			/* SFX_GRABAMMO, */
	"grabmoney.aifc",			/* SFX_GRABMONEY, */
	"grablife.aifc",			/* SFX_GRABLIFE, */
	"grabtalisman.aifc",			/* SFX_GRABTALISMAN, */
	"fullup.aifc",				/* SFX_CANTGRAB, */
	"ouchsmall.aifc",			/* SFX_OUCHSMALL, */
	"ouchmed.aifc",				/* SFX_OUCHMED, */
	"ouchbig.aifc",				/* SFX_OUCHBIG, */
};

#ifndef	SOUNDINTERFACE_WORKS
static char	*dummyfx[] = {
//	"DummyStereo.aiff",
	"DummyStereo.aifc",
};
#define	NDUMMIES	(sizeof (dummyfx) / sizeof (char *))
#endif

static char	sfxdir[] = "$progdir/aiff/";

static int8	dummiesloaded;


/***************************************************************************
 * Code.
 */
void
playsound (id)
int id;
{
	if (dosfx) {
		RAMSoundRec	rs;

		rs.whatIWant	= kStartRAMSound;
		rs.soundID	= id;

		CallSound ((CallSoundRec *) &rs);
	}
}


void
initsound ()
{
	int32	whatIWant;

	whatIWant = kInitializeSound;
	CallSound ((CallSoundRec *) &whatIWant);
}

void
closesound ()
{
	int32	whatIWant;

	whatIWant = kCleanupSound;
	CallSound ((CallSoundRec *) &whatIWant);
}


void
loadsfx ()
{
	register int	i;
	LoadRAMSoundRec	lrs;
	char		fx[80];		// Whyzzit always 80?

	lrs.whatIWant = kLoadRAMSound;

#ifndef	SOUNDINTERFACE_WORKS
	loaddummies ();
#endif

	for (i = 1;  i < MAX_SFX;  i++) {
		strcpy (fx, sfxdir);
		strcat (fx, ramfx[i-1]);

		lrs.soundID		= i;
		lrs.soundFileName	= fx;
		lrs.amplitude		= MAXAMPLITUDE;
		lrs.balance		= 50;
		lrs.frequency		= 0;

		CallSound ((CallSoundRec *) &lrs);
	}
}

void
freesfx ()
{
	register int	i;
	RAMSoundRec	rs;

	rs.whatIWant = kUnloadRAMSound;

#ifndef	SOUNDINTERFACE_WORKS
	dropdummies ();
#endif
	for (i = 1;  i < MAX_SFX;  i++) {
		rs.soundID = i;

		CallSound ((CallSoundRec *) &rs);
	}
}


int32
loadsound (filename, id)
char	*filename;
int32	id;
{
	LoadRAMSoundRec	lrs;
	char		fx[80];

	strcpy (fx, sfxdir);
	strcat (fx, filename);

	lrs.whatIWant		= kLoadRAMSound;
	lrs.soundID		= id;
	lrs.soundFileName	= fx;
	lrs.amplitude		= MAXAMPLITUDE;
	lrs.balance		= 50;
	lrs.frequency		= 0;

	return (CallSound ((CallSoundRec *) &lrs));
}

void
unloadsound (id)
int32	id;
{
	RAMSoundRec	rs;

	rs.whatIWant	= kUnloadRAMSound;
	rs.soundID	= id;

	CallSound ((CallSoundRec *) &rs);
}


void
spoolsound (filename, nreps)
char	*filename;
int32	nreps;
{
	if (domusic) {
		SpoolSoundRec	ssr;

		/*
		 * Load/start sound FX.
		 */
#ifndef	SOUNDINTERFACE_WORKS
		if (dummiesloaded)
			dropdummies ();
#endif

		ssr.whatIWant	= kSpoolSound;
		ssr.fileToSpool	= filename;
		ssr.numReps	= nreps;
		ssr.amplitude	= 0x3800;
		CallSound ((CallSoundRec *) &ssr);
	}
}

void
stopspoolsound (nsecs)
int32	nsecs;
{
	SpoolFadeSoundRec	sr;

	if (nsecs > 0) {
		sr.whatIWant	= kStopFadeSpoolSound;
		sr.seconds	= nsecs;
	} else
		sr.whatIWant	= kStopSpoolingSound;

	CallSound ((CallSoundRec *) &sr);
}

int
issoundspooling ()
{
	int32	whatIWant;

	whatIWant = kIsSoundSpooling;

	return (CallSound ((CallSoundRec *) &whatIWant));
}



#ifndef	SOUNDINTERFACE_WORKS

static void
loaddummies ()
{
	register int	i;
	LoadRAMSoundRec	lrs;
	char		fx[80];		// Whyzzit always 80?

	if (!dummiesloaded) {
		lrs.whatIWant = kLoadRAMSound;

		for (i = 0;  i < NDUMMIES;  i++) {
			strcpy (fx, sfxdir);
			strcat (fx, dummyfx[i]);

			lrs.soundID		= i + 1000;
			lrs.soundFileName	= fx;
			lrs.amplitude		= MAXAMPLITUDE;
			lrs.balance		= 50;
			lrs.frequency		= 0;

			CallSound ((CallSoundRec *) &lrs);
		}
		dummiesloaded = TRUE;
	}
}

static void
dropdummies ()
{
	register int	i;
	RAMSoundRec	rs;

	if (dummiesloaded) {
		rs.whatIWant = kUnloadRAMSound;

		for (i = NDUMMIES;  --i >= 0; ) {
			rs.soundID = i + 1000;

			CallSound ((CallSoundRec *) &rs);
		}
		dummiesloaded = FALSE;
	}
}

#endif
