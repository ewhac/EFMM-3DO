/*******************************************************************************************
 *	File:			SubscriberUtils.h
 *
 *	Contains:		Common definitions for data stream subscribers
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/15/93		jb		Include ";" at the end of the SUBS_CHUNK_COMMON macro.
 *						Make queuing routines generalized to not require common channel
 *						header in order to use them. Removed AUDIO_TICKS_PER_SEC since
 *						the audio clock time base can be set by users (hence, this is not
 *						really a constant).
 *	6/2/93		jb		Removed extraneous #include of "graphics.h"
 *	5/15/93		njc		Removed underscores from the _AddDataMsgToTail and _GetNextDataMsg 
 *						routines.
 *	5/5/93		njc		New today.
 *
 *******************************************************************************************/
#ifndef __SUBSCRIBERUTILS_H__
#define __SUBSCRIBERUTILS_H__

#ifndef __DATASTREAMLIB_H__
#include "DataStreamLib.h"
#endif

/**********************/
/* Internal constants */
/**********************/

/* The following macro makes a 32-bit unsigned long scalar out of 4
 * char's as input. This macro is included to avoid compiler warnings from 
 * compilers that object to 4 character literals, for example, 'IMAG'.
 */

#define	CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)


/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define	SUBS_CHUNK_COMMON	\
	int32		chunkType;		/* chunk type */					\
	int32		chunkSize;		/* chunk size including header */	\
	int32		time;			/* position in stream time */		\
	int32		channel;		/* logical channel number */		\
	int32		subChunkType;	/* data sub-type */

typedef	struct SubsChunkData {
	SUBS_CHUNK_COMMON
} SubsChunkData, *SubsChunkDataPtr;


/************************************************************/
/* General subscriber message queue structure with support	*/
/* for fast queuing of entries in a singly linked list.		*/
/************************************************************/
typedef struct SubsQueue {
	SubscriberMsgPtr	head;			/* head of message queue */
	SubscriberMsgPtr	tail;			/* tail of message queue */
	} SubsQueue, *SubsQueuePtr;


/************************************************/
/* Channel context, one per channel, per stream	*/
/************************************************/
typedef struct SubsChannel {
	unsigned long		status;			/* state bits */
	SubsQueue			msgQueue;		/* queue of subscriber messages */
	} SubsChannel, *SubsChannelPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

Boolean				AddDataMsgToTail( SubsQueuePtr subsQueue, SubscriberMsgPtr subMsg );
SubscriberMsgPtr	GetNextDataMsg( SubsQueuePtr subsQueue );

#endif	/* __SUBSCRIBERUTILS_H__ */

