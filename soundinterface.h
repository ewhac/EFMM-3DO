/***************************************************************
**	SoundInterface.h
**
**	Created by Peter Commons for 3DO.
**
**	This is the very first incarnation of the sound interface file.
**	Every function, variable, or constant you use or should know about should be in here.
**
**
** Copyright (c) 1993, 3DO Company.
** This program is proprietary and confidential.
**
Change History:
07/27/93	Initial Version.
08/10/93	Removed instrument file name parameter from load RAM sound; the instrument
			is now determined automagically.
08/11/93	Inherited some changes by Leo.
08/16/93	Removed isStereo flag from LoadRAMSound. Stereo/Mono determination is done
			automatically. Added kStopFadeSpoolSound command and SpoolFadeSoundRec
			data structure.
08/30/93	Add: kIsSoundSpooling function. Returns true if sound currently spooling.
			Modified to work under DragonTail5.
09/01/93	Add instrument cacheing. The first MAX_CACHED_INSTRUMENTS are cached in RAM
			and not reloaded from disk every time they're used. Once the cache is full,
			there is no replacement strategy.
09/08/93	Moved away from a channel-dependent philosophy. You can now load more sounds
			than you have channels for or than you have room for on the dsp. Intruments
			will get loaded into the dsp and unloaded as needed.
09/16/93	Space is now reserved in the dsp for a spooler instrument (currently I use
			halfmono8.dsp to determine the size because it takes up a lot of room and trying
			to use fixedstereosample.dsp didn't save enough). This should prevent the
			loading of too many RAM sounds precluding the use of the dsp for spooling.
			DSP instruments I use (the mixer, the envelope, and the spooler room reserver)
			no longer have pathnames, as they are not needed.
***************************************************************/



/************************************************************/
/* Section1 - Things you might want to change				*/
/************************************************************/

// Set to point to your directory

#define AUDIODIR "$audio"

// Multiply mixer elements times this number. A value of 1 guarantees no clipping
// even if you have 8 sounds playing simultaneously at full volume. Set it higher as needed
// and at your own risk

#define	AUDIO_MULTIPLIER	4

// Each Sound you create must have a unique number that you refer to it by when you
// load it, play it, stop it, or unload it. A value of 0 means a channel has not been
// allocated, so don't use it. For example:

enum {
	kSoundOne = 1,
	kSoundTwo,
	kSoundThree
};

// I will cache the first n unqiue instruments for quick retrieval where n is the
// value defined by MAX_CACHED_INSTRUMENTS. There is no cache replacement; once the
// cache is full, no more instruments are cached.

#define	MAX_CACHED_INSTRUMENTS	8

// This is the maximum number of ram sounds you can "load". Loading a sound just means
// that the sample data is in memory. It is possible to load more than maximum number
// of sounds that can play at one time either because the dsp is out of memory or because
// there are no more mixer channels available.

#define	kMaxRamSounds	22

/************************************************************/
/* Section2 - Things you don't want to change.				*/
/************************************************************/

#define MAXAMPLITUDE 	0x7FFF
#define nil				0
#define noErr			0

#define	kMixerFileName			"mixer8x2.dsp"
#define	kEnvelopeFileName 		"envelope.dsp"
#define	kSpoolerRoomSaveName	"halfmono8.dsp"
#define	kNumChannels	8

#define THREAD_PARENT	((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)

// RAM Resident Sound Info Record

typedef struct SoundDataRec
{
	int32	soundID;				// Sound ID chosen by user for this sound; zero means
									// no sound for this channel
	int32	amplitude;				// Channel Volume Level - if stereo, this is divided
	int32	balance;				// For stereo sounds, how much left and right
	int32	frequency;				// If non-zero, frequency to play sound at
	Item	sample;					// Sample
	char	instrName[100];			// name of instrument being used

	// Items below are only valid if channels have been allocated

	int32	channel[2];				// If >= 0, these are mixer channels than have been
									// assigned for use. The second one is for stereo sounds.
	Item	instrument;				// Instrument
	Item	attachment;				// Attachment of sample to instrument
	Item	freqKnob;				// frequency knob (for variable rate sounds)
} SoundDataRec, *SoundDataPtr, **SoundDataHdl;


/************************************************************/
/* Section3 - How to call my sound interface				*/
/************************************************************/

// Various things you can do (pass in the "whatIWant" field)

enum {
	kInitializeSound = 1,	// Must call this once before making any other sound calls
	kCleanupSound,			// Call this when you're done with sound for good
	kSpoolSound,			// Spools the file (look above) specified from disk. Loops 100 times.
	kStopSpoolingSound,		// Stops the currently spooling sound immediately.
	kBeQuiet,				// Set Global Mixer Levels To 0
	kBeNoisy,				// Set Global Mixer Levels To Non-Zero
	kLoadRAMSound,			// Load a ram resident sound
	kUnloadRAMSound,		// Unload a RAM resident sound ( stop it if it's playing)
	kStartRAMSound,			// Start a RAM resident sound ( will stop automatically if no loops)
	kStopRAMSound,			// Stop a RAM resident sound ( if it's still playing )
	kSetRAMSoundFreq,		// Set frequency of a variable-rate RAM sound.
	kSetRAMSoundAmpl,		// Set the amplitude of a variable-rate RAM sound.
	kStopFadeSpoolSound,	// Fade spooled sound out over number of seconds specified
	kIsSoundSpooling		// Returns a non-zero value if a sound is currently being spooled
};

//	Data Structures (Parameter Blocks) you pass to CallSound()

//	Load RAM Sound Parameter Block

typedef	struct LoadRAMSoundRec
{
	int32	whatIWant;				// Union Structure Identifier; must be the first
									// field for all parameter blocks
	int32	soundID;				// unique sound identifier used to refer to sound
									// from now on when playing it, stopping it, etc.
	char *	soundFileName;			// ptr to name of sound to play
	int32	amplitude;				// amplitude to play sound at (0-0x7FFF)
	int32	balance;				// left right speaker balance - 0 means all left
									// 50 means even, 100 means all right speaker
	int32	frequency;				// If non-zero, sound is variable-rate and value
									// specifies rate to play sound at. 0x8000 is sampled
									// rate, 0x4000 is 1/2 rate, etc.
} LoadRAMSoundRec, *LoadRAMSoundPtr, **LoadRAMSoundHdl;

//	Start, Stop, and Unload RAM Sound Parameter Block

typedef	struct RAMSoundRec
{
	int32	whatIWant;				// Union Structure Identifier; must be the first
									// field for all parameter blocks
	int32	soundID;				// unique sound identifier used to refer to sound
									// from now on when playing it, stopping it, etc.
} RAMSoundRec, *RAMSoundPtr, **RAMSoundHdl;

// Set amplitude and frequency for RAM Sound Parameter Block

typedef	struct SetRAMSoundRec
{
	int32	whatIWant;				// Union Structure Identifier; must be the first
									// field for all parameter blocks
	int32	soundID;				// unique sound identifier used to refer to sound
									// from now on when playing it, stopping it, etc.
	int32	level;					// New Frequency or Amplitude
} SetRAMSoundRec, *SetRAMSoundPtr, **SetRAMSoundHdl;


// Spool Sound Parameter Block

typedef	struct SpoolSoundRec
{
	int32	whatIWant;				// Union Structure Identifier; must be the first
									// field for all parameter blocks
	char *	fileToSpool;			// Ptr to name of file to spool
	int32	numReps;				// Number of repetitions to play of the sound
	int32	amplitude;				// Amplitude of sound when played (0-0x7FFF)
}	SpoolSoundRec, *SpoolSoundPtr, **SpoolSoundHdl;

// Spool Sound Parameter Block

typedef	struct SpoolFadeSoundRec
{
	int32	whatIWant;				// Union Structure Identifier; must be the first
									// field for all parameter blocks
	int32	seconds;				// number of seconds to take to fade sound
}	SpoolFadeSoundRec, *SpoolFadeSoundPtr, **SpoolFadeSoundHdl;

// Union of all types of parameter blocks. If there are no extra parameters, just set
// the whatIWant field below to the desired function (e.g. kInitializeSound).

typedef union CallSoundRec
{
	int32				whatIWant;		// Union Structure Identifier; must be the first
										// field for all parameter blocks
	LoadRAMSoundRec		loadSound;
	SpoolSoundRec		spoolSound;
	SpoolFadeSoundRec 	fadeSound;
	RAMSoundRec			ramSound;
	SetRAMSoundRec		setSound;
} CallSoundRec, *CallSoundPtr, **CallSoundHdl;

// 	This is the function you always call.
//	This function definition shouldn't ever change, although the
//	parameter blocks might (but hopefully not).

int32	CallSound( union CallSoundRec *soundPtr );
