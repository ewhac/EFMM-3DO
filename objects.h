/*  :ts=8 bk=0
 *
 * objects.h:	General and specific definitions for objects.
 *
 * Leo L. Schwab					9306.30
 */
#ifndef	_OBJECTS_H
#define	_OBJECTS_H

#ifndef	_CASTLE_H
#include "castle.h"
#endif


/***************************************************************************
 * General case Object and Object definition structures.
 */
typedef struct ObDef {
	int32	(*od_Func)();		/*  Function handler		*/
	ubyte	od_Type;
	ubyte	od_State;
	ubyte	od_Flags;
	ubyte	__pad;
	int32	od_ObCount;		/*  # of objects of this type	*/
	void	*od_InstanceBase;	/*  Ptr to base of object array	*/
} ObDef;


typedef struct Object {
	struct Object	*ob_Next;	/*  Next in chain off MapEntry	*/
	ubyte		ob_Type;	/*  Note these are at the same	*/
	ubyte		ob_State;	/*   offset in the structure as	*/
	ubyte		ob_Flags;	/*   in the ObDef.		*/
	ubyte		__pad;
	struct ObDef	*ob_Def;	/*  Ptr to ObDef		*/
	int32		ob_VertIdx;	/*  Index into obverts[] array	*/
} Object;

typedef struct InitData {
	struct MapEntry	*id_MapEntry;	/*  Entry in levelmap[][]	*/
	int32		id_XIdx,
			id_ZIdx;
} InitData;


enum ObjectTypes {
	OTYP_INVALID = 0,
	OTYP_ZOMBIE,
	OTYP_BOSSZOMBIE,
	OTYP_GEORGE,
	OTYP_BOSSGEORGE,
	OTYP_HEAD,
	OTYP_BOSSHEAD,
	OTYP_SPIDER,
	OTYP_NSDOOR,
	OTYP_EWDOOR,
	OTYP_KEY,
	OTYP_POWERUP,
	OTYP_EXIT,
	OTYP_TRIGGER,
	MAX_OTYP
};


enum ObjectStates {
	OBS_INVALID = 0,
	OBS_ASLEEP,
	OBS_AWAKENING,
	OBS_WALKING,
	OBS_CHASING,
	OBS_JUMPING,
	OBS_ATTACKING,
	OBS_DYING,
	OBS_DEAD,
	OBS_OPENING,
	OBS_OPEN,
	OBS_CLOSING,
	OBS_CLOSED,
	MAX_OBS
};


#define	OBF_MOVE	1
#define	OBF_REGISTER	(1<<1)
#define	OBF_RENDER	(1<<2)
#define	OBF_CONTACT	(1<<3)
#define	OBF_SHOOT	(1<<4)
#define	OBF_PROBE	(1<<5)
#define	OBF_SAWME	(1<<6)
#define	OBF_PLAYERONLY	(1<<7)


enum ObjectOperations {
	OP_INVALID = 0,
	OP_FIATLUX,	/*  Initialize object definition		*/
	OP_DESTRUCT,	/*  Free object definition resources		*/
	OP_CREATEOB,	/*  Create object instance			*/
	OP_DELETEOB,	/*  Free object instance			*/
	OP_INITOB,	/*  Initialize next available instance		*/
	OP_MOVE,	/*  Move object					*/
	OP_REGISTER,	/*  Place object and req'd data in render list	*/
	OP_RENDER,	/*  Guess...					*/
	OP_CONTACT,	/*  If object touches player			*/
	OP_SHOT,	/*  If object is shot by player			*/
	OP_PROBE,	/*  Player probes object with 'B' button	*/
	MAX_OP
};


#endif	/*  _OBJECTS_H  */
