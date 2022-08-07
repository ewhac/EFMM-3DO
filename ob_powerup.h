/*  :ts=8 bk=0
 *
 * ob_powerup.h:	Public definitions for powerups.
 *
 * Leo L. Schwab					9310.21
 */
#ifndef	_OBJECTS_H
#include "objects.h"
#endif


/***************************************************************************
 * The various kinds of powerups.
 */
enum PowerUps {
	PWR_KEY,
	PWR_AMMO,
	PWR_MONEY1,
	PWR_MONEY2,
	PWR_MONEY3,
	PWR_HEALTH,
	PWR_LIFE,
	PWR_TALISMAN,
	PWR_RESERVED1,
	PWR_RESERVED2,
	PWR_DECOR1,
	PWR_DECOR2,
	PWR_DECOR3,
	PWR_OBJECT1,
	PWR_OBJECT2,
	PWR_OBJECT3,
	PWR_DECOR4,
	PWR_DECOR5,
	PWR_DECOR6,
	PWR_DECOR7,
	PWR_DECOR8,
	PWR_DECOR9,
	PWR_DECOR10,
	PWR_DECOR11,
	PWR_OBJECT4,
	PWR_OBJECT5,
	PWR_OBJECT6,
	PWR_OBJECT7,
	PWR_OBJECT8,
	PWR_OBJECT9,
	PWR_OBJECT10,
	PWR_OBJECT11,
	MAX_PWR
};


/***************************************************************************
 * Powerup structure definitions.
 */
typedef struct DefPowerup {
	struct ObDef		od;
	struct ImageEnv		*od_Env;
	struct ImageEntry	*od_Images;
	struct AnimLoaf		*od_AL;
	int32			(*od_PowerFuncs[MAX_PWR])();
} DefPowerup;

typedef struct ObPowerup {
	struct Object	ob;
	struct MapEntry	*ob_ME;
	ubyte		ob_XIdx,
			ob_ZIdx;
	ubyte		ob_PowerType;
	ubyte		ob_PrevState;
} ObPowerup;
