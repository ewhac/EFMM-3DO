/*******************************************************************************************
 *	File:			MsgUtils.h
 *
 *	Contains:		definitions for MsgUtils.c
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	4/16/93		jb		Switch to using "Item.h" from "Items.h", and
 *						to using "MsgPort.h" from "MsgPorts.h" in 3B1
 *	4/13/93		jb		Add GetMsgPortSignal(), change CreateMsgPort() to NewMsgPort()
 *						to prepare for upgrade to 3B1
 *	4/5/93		jb		Add 'signalMaskPtr' arg to CreateMsgPort()
 *	4/4/93		jb		Return pointer to message data in WaitForMsg() & PollForMsg()
 *	3/30/93		jb		Add specific 'wait for' arg to WaitForMsg()
 *	3/28/93		jb		Added PollForMsg()
 *	3/28/93		jb		Add WaitForMsg() and RemoveMsgPort()
 *	3/25/93		jb		New today
 *
 *******************************************************************************************/

#ifndef	__MSGUTILS_H__
#define	__MSGUTILS_H__

#ifndef _TYPES_H
#include "types.h"
#endif

#ifndef _ITEM_H
#include "item.h"
#endif

#ifndef _MSGPORT_H
#include "msgport.h"
#endif

/*******************************************/
/* System related message helper functions */
/*******************************************/

Item		NewMsgPort( uint32* signalMaskPtr );
void		RemoveMsgPort( Item msgPortItem );
uint32		GetMsgPortSignal( Item msgPortItem );

Item		CreateMsgItem( Item replyPortItemOrZero );
void		RemoveMsgItem( Item msgItem );

int32		WaitForMsg( Item msgPortItem, Item* msgItemPtr, Message** pMsgPtr,
					void** pMsgDataPtr, Item waitMsgItem );
Boolean		PollForMsg( Item msgPortItem, Item* msgItemPtr, Message** pMsgPtr,
					void** pMsgDataPtr, int32 *status );

#endif	/* __MSGUTILS_H__ */
