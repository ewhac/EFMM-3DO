/*******************************************************************************************
 *	File:			SAudioSubscriber.h
 *
 *	Contains:		definitions for AudioSubscriber.c
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	8/11/93		rdg		Version 2.2
 *						Removed attachRunning from channel structure.  No longer needed.
 *	8/9/93		rdg		Version 2.1
 *						Add kSAudioCtlOpFlushChannel which stops and flushes the currently queued data.
 *	8/6/93		rdg		Version 2.0
 *						Add kSAudioCtlOpCloseChannel so that you can unload that channel's instrument from
 *						the DSP when a logical channel's data is done to free up resources for
 *						newer stuff.
 *	6/25/93		rdg		Version 1.6
 *						Remove references to frequency.  Not useful in this context because
 *						doing sample rate interpolation to change frequency will screw up 
 *						sync with the stream clock.
 *	6/25/93		rdg		Version 1.5
 *						change SAudioOutput member bool stereo to int32 numChannels...
 *						New error codes...
 *						New control opcodes: kSAudioCtlOpGetVol, kSAudioCtlOpGetFreq, kSAudioCtlOpGetPan,	
 *						kSAudioCtlOpGetClockChan.	
 *						Add SA_22K_16B_M_SDX2.
 *						Add Boolean attachRunning flag to the Channel struct
 *	6/22/93		rdg		Version 1.1
 *						Modify for amplitude and pan control.
 *						Change output instrument to mixer2x2.dsp to allow for gain and pan
 *						control.
 *						Add SAudioOutput struct for deal with mixer knobs.  
 *						Remove initialAmplitude field from Channel struct.
 *						Replace channelOutput field with a SAudioOutput struct.
 *						Change SA_DIRECT_OUT_INSTRUMENT_NAME to SA_OUTPUT_INSTRUMENT_NAME
 *	6/19/93		rdg		Version 1.0
 *	6/19/93		rdg		Add clockChannel to SAudioContext.  Add clock struct to 
 *						SAudioControlBlock. clockChannel defaults to 0 and is 
 *						used to determine which logical channel to use for the
 *						"clock".  The time stamp data in all other logical channels is
 *						ignored.  clockChannel can be changed via an SAudioSubscriber
 *						control message.
 *						Added kSAudioCtlOpSetClockChan to the enum for SAudioControlOpcode.
 *						Added stream control block to the subscriber context struct.
 *	6/15/93		jb		Version 0.16
 *						Switch to using CHAR4LITERAL macro to avoid compiler warnings.
 *	5/27/93		rdg		Version 0.15
 *						Enabled #define tags for new instrument/data types:
 *						22K 8bit Stereo, 44K 8bit Stereo, 22K 16bit Stereo Compressed.
 *						Modified SAudioSampleDescriptor field sampleRate to be int32 instead
 *						of a fract16.
 *	5/27/93		rdg		Version 0.14
 *	5/26/93		rdg		Version 0.12
 *						Allow for preloading of instrument templates via control message.
 *						Preload output instrument template at thread instantiation time 
 *						and create output instrument when channel is opened. Make mechanism
 *						table/tag driven to allow for easy addition of new instrument types.
 *						Added 'outputTemplateItem' and 'templateArray' to subscriber context.
 *	5/26/93		rdg		Change to 'SAudioCtlBlock' to a union for different control
 *						request types. Add field for handling template stuff.
 *	5/25/93		rdg		Added TemplateRec to describe templates we know about.
 *						Added an opcode to SAudioCtl for loading templates.
 *	5/25/93		rdg		Version 0.11
 *						added acutalSampleSize to the SSMPHeaderChunk
 *  5/17/93		rdg		Version 0.10
 *						stripped the ctlBlock because the stream will now contain the initialization data
 *						and ctl messages will only be used to change the state of a playing stream.
 *						I also altered the SAudioHeaderChunk so I can pass it to OpenChannel() directly.
 *  5/17/93		rdg		Merged Neil's changes with my updates.  He created the chunk/subchunk structure
 *						which uses SNDS, SHDR, SSMP types.
 *  5/17/93		rdg		I removed the definition of SAUDIO_INITIAL_AMPLITUDE and added a field to
 *						the SAudioChannel and SAudioCtlBlock structures called initialAmplitude. Now this 
 *						value can be passed via a header chunk in the stream or from the main app at 
 *						OpenChannel() time.
 *  5/17/93		rdg		Changed the name of the "width" field in the SAudioSampleDescriptor
 *						to "numbits". 
 *  5/14/93		rdg		Changed the name of kSAudioCtlOpInitChan to kSAudioCtlOpOpenChan.
 *	5/11/93		rdg		Completed first pass at full implementation.
 *	5/8/93		rdg		begin conversion to AudioSubscriber
 *	4/19/93		jb		Add fields to context to support synchronous communications
 *						with a subscriber's creator at the time of creation (so that
 *						a proper completion status can be given to the creator).
 *	4/17/93		jb		Added TEST_CHUNK_TYPE
 *	4/16/93		jb		Switch to New/Dispose model; return context on New to allow for
 *						later cleanup of a subscriber.
 *	4/13/93		jb		Added TestChunkData.
 *	4/12/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __SAUDIOSUBSCRIBER_H__
#define __SAUDIOSUBSCRIBER_H__

#ifndef __DATASTREAM_H__
#include "DataStream.h"
#endif


#ifndef __SUBSCRIBER_H__
#	include "SubscriberUtils.h"
#endif

/********************************************/
/* Error codes returned by SAudioSubscriber */
/********************************************/
typedef enum SAudioErrorCode {
	SAudioErrTemplateNotFound = -7000,		/* required instrument template not found */
	SAudioErrBadChannel,
	SAudioErrVolOutOfRange,
	SAudioErrPanOutOfRange,
	SAudioErrAllocInstFailed,
	SAudioErrUnsupSoundfileType,
	SAudioErrConnectInstFailed,
	SAudioErrCouldntAllocBufferPool,
	SAudioErrNumAudioChannelsOutOfRange,
	SAudioErrOpNotSuppForDataType,
	SAudioErrUnableToFreeInstrument
	};


/**********************/
/* Internal constants */
/**********************/

#define SAUDIO_STREAM_VERSION_0		0		/* stream version supported by subscriber */

#define	SNDS_CHUNK_TYPE		CHAR4LITERAL('S','N','D','S')	/* subscriber data type */
#define	SHDR_CHUNK_TYPE		CHAR4LITERAL('S','H','D','R')	/* header subchunk type */
#define	SSMP_CHUNK_TYPE		CHAR4LITERAL('S','S','M','P')	/* sample subchunk type */

#define	SA_SUBS_MAX_SUBSCRIPTIONS	4		/* max # of streams that can use this subscriber */
#define	SA_SUBS_MAX_CHANNELS		8		/* max # of logical channels per subscription */
#define	SA_SUBS_MAX_BUFFERS			8		/* max # of sample buffers per channel */

#define	SA_OUTPUT_INSTRUMENT_NAME		"mixer2x2.dsp"

/**********************************************/
/* Flags for pre-loading instrument templates */
/**********************************************/                                    

#define	SA_COMPRESSION_SHIFT	24
#define	SA_CHANNELS_SHIFT		16
#define	SA_RATE_SHIFT			8
#define	SA_SIZE_SHIFT			0

/* Sample rate values */
#define	SA_44KHz			1
#define	SA_22KHz			2

/* Sample size values */
#define	SA_16Bit			1
#define	SA_8Bit				2

/* Sample channel values */
#define	SA_STEREO			2
#define	SA_MONO				1

/* Sample compression type values */
#define	SA_COMP_NONE		1
#define	SA_COMP_SDX2		2

/* Macro to create instrument tags from human usable parameters. 
 * The tag values created by this macro are used by the 
 * GetTemplateItem() routine.
 */
#define	MAKE_SA_TAG( rate, bits, chans, compression )	\
	((long) ( (rate << SA_RATE_SHIFT ) 					\
	| (bits << SA_SIZE_SHIFT) 							\
	| (chans << SA_CHANNELS_SHIFT)						\
	| (compression << SA_COMPRESSION_SHIFT) ) )

/* Uncompressed instrument tags */
#define	SA_44K_16B_S		(MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_44K_16B_M		(MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_NONE ))
#define	SA_44K_8B_S			(MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_44K_8B_M			(MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_MONO, SA_COMP_NONE ))
#define	SA_22K_8B_S			(MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE ))
#define	SA_22K_8B_M			(MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_MONO, SA_COMP_NONE ))

/* Compressed instrument tags */
#define	SA_44K_16B_S_SDX2	(MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 ))
#define	SA_44K_16B_M_SDX2	(MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 ))
#define	SA_22K_16B_S_SDX2	(MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 ))		
#define	SA_22K_16B_M_SDX2	(MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 ))

/* Instruments to play these types do not yet exist */
#if 0
#define	SA_22K_16B_S		(MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE ))		
#define	SA_22K_16B_M		(MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_NONE ))
#endif

/****************************************************************************/
/* Structure to describe instrument template items that may be dynamically	*/
/* loaded. An array of these describes all the possible templates we know	*/
/* about. We actually create an instrument when explicitly asked to do	    */
/* so, or when we encounter a header in a stream that requires a template	*/
/* that we do not currently have loaded. 									*/
/* Each instantiation of an audio subscriber gets a clean copy of the 		*/
/* initial array of structures.												*/
/****************************************************************************/
typedef struct TemplateRec {
		int32		templateTag;		/* used to match caller input value */ 
		Item		templateItem;		/* item for the template or zero */
		char*		instrumentName;		/* ptr to string of filename */
	} TemplateRec, *TemplateRecPtr;

/***********************************************/
/* Opcode values as passed in control messages */
/***********************************************/
typedef enum SAudioControlOpcode {
		kSAudioCtlOpLoadTemplates = 0,
		kSAudioCtlOpSetVol,	
		kSAudioCtlOpSetPan,	
		kSAudioCtlOpSetClockChan,	
		kSAudioCtlOpGetVol,	
		kSAudioCtlOpGetFreq,	
		kSAudioCtlOpGetPan,	
		kSAudioCtlOpGetClockChan,
		kSAudioCtlOpCloseChannel,
		kSAudioCtlOpFlushChannel
		};

/**************************************/
/* Control block 					  */
/**************************************/
typedef union SAudioCtlBlock {

		struct {						/* kSAudioCtlOpLoadTemplates */
			long*		tagListPtr;		/* NULL terminated tag list pointer */
			} loadTemplates;

		struct {						/* kSAudioCtlOpSetVol */
			int32		channelNumber;	/* which channel to control */
			int32		value;
			} amplitude;

		struct {						/* kSAudioCtlOpSetPan */
			int32		channelNumber;	/* which channel to control */
			int32		value;
			} pan;

		struct {						/* kSAudioCtlOpSetClockChannel */
			int32		channelNumber;	/* channel to make clock channel */
			} clock;

		struct {						/* kSAudioCtlOpCloseChannel */
			int32		channelNumber;	/* channel to close */
			} closeChannel;

		struct {						/* kSAudioCtlOpFlushChannel */
			int32		channelNumber;	/* channel to flush */
			} flushChannel;
			
	} SAudioCtlBlock, *SAudioCtlBlockPtr;

/*****************************************************************
 * Items necessary to play a data chunk via the Audio Folio, one 
 * set per channel, per stream.
 *****************************************************************/

typedef struct SAudioBuffer {
	Item				sample;					/* sample to use for playing a data chunk */
	Item 				attachment;				/* attachment for connecting data to instrument */
	Item				cue;					/* cue so we know when sample has finished */
	uint32				signal;					/* signal so we can figure out which sample it was */
	SubscriberMsgPtr	pendingMsg;				/* ptr to msg so we can reply when this chunk is done */
	void*				link;					/* for linking into lists */
	} SAudioBuffer, *SAudioBufferPtr;

/*****************************************************************
 * The output instrument Item and fields necessary to control
 * volume and panning.
 *****************************************************************/

typedef struct SAudioOutput {
	Item				instrument;				/* instrument item */
	int32				numChannels;			/* is instrument mono or stereo or ? */
	Item 				leftGain0;				/* left gain knob for channel 0 */
	Item 				rightGain0;				/* right gain knob for channel 0 */
	Item 				leftGain1;				/* left gain knob for channel 1 */
	Item 				rightGain1;				/* right gain knob for channel 1 */
	int32 				currentAmp;				
	int32 				currentPan;				
	} SAudioOutput, *SAudioOutputPtr;

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct SAudioChannel {
	unsigned long			status;				/* state bits (see below) */
	int32					numBuffers;			/* how many buffers to use for this channel */
	MemPoolPtr 				bufferPool;			/* pool of pre initalized attachments, cues, signals, and samples */
	Item					channelInstrument;	/* DSP instrument to play channel's data chunks */
	Boolean					instStarted;		/* flag to know if instrument is started */
	SAudioOutput			channelOutput;		/* contains output instrument and control knobs */
	uint32   				signalMask;			/* the ORd signals for all the current cues on this channel */
	SubsQueue				dataQueue;			/* waiting data chunks */
	int32					inuseCount;			/* number of buffers currently in the in use queue */
	SAudioBufferPtr			inuseQueueHead;		/* pointer to head of buffers queued to the audio folio */
	SAudioBufferPtr			inuseQueueTail;		/* pointer to tail of buffers queued to the audio folio */
	} SAudioChannel, *SAudioChannelPtr;

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct SAudioContext {
	Item			creatorTask;				/* who to signal when we're done initializing */
	uint32			creatorSignal;				/* signal to send for synchronous completion */
	int32			creatorStatus;				/* result code for creator */

	Item			threadItem;					/* subscriber thread item */
	void*			threadStackBlock;			/* pointer to thread's stack memory block */

	Item			requestPort;				/* message port item for subscriber requests */
	uint32			requestPortSignal;			/* signal to detect request port messages */

	DSStreamCBPtr	streamCBPtr;				/* stream this subscriber belongs to */
	
	TemplateRecPtr	templateArray;				/* ptr to array of template records */
	
	uint32			allBufferSignals;			/* or's signals for all instruments on all channels */

	Item			outputTemplateItem;			/* template item for making channel's output instrument */

	int32			clockChannel;				/* which logical channel to use for clock */
		 
	SAudioChannel	channel[SA_SUBS_MAX_CHANNELS];	/* an array of channels */

	} SAudioContext, *SAudioContextPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct SAudioSampleDescriptor
{
	uint32		dataFormat;					/* 4 char ident (e.g. 'raw ')	*/
	int32		sampleSize;					/* bits per sample, typically 16 */
	int32 		sampleRate;    				/* 44100 or 22050 */
	int32		numChannels;				/* channels per frame, 1 = mono, 2=stereo */
	uint32  	compressionType;  			/* eg. SDX2 , 930210 */
	int32		compressionRatio;
	int32		sampleCount;				/*	Number of samples in sound	*/
} SAudioSampleDescriptor, *SAudioSampleDescriptorPtr;


typedef	struct SAudioHeaderChunk {
	SUBS_CHUNK_COMMON
	int32		version;					/*	0 for this version */
	int32		numBuffers;					/*  max # of buffers that can be queued to the AudioFolio 	*/
	int32		initialAmplitude;			/*  initial volume of the stream */
	int32		initialPan;					/*  initial pan of the stream */
	SAudioSampleDescriptor sampleDesc;		/*  info about sound format */
} SAudioHeadeChunk, *SAudioHeaderChunkPtr;

typedef struct SAudioDataChunk {
	SUBS_CHUNK_COMMON
	int32		actualSampleSize;			/* actual number of bytes in the sample data */
	char		samples[4];					/* start of audio samples */
	} SAudioDataChunk, *SAudioDataChunkPtr;



/*****************************/
/* Public routine prototypes */
/*****************************/

int32	InitSAudioSubscriber( void );
int32	NewSAudioSubscriber( SAudioContextPtr *pCtx, DSStreamCBPtr streamCBPtr, long deltaPriority );
int32	DisposeSAudioSubscriber( SAudioContextPtr ctx );

#endif	/* __SAUDIOSUBSCRIBER_H__ */

