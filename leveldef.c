/*  :ts=8 bk=0
 *
 * leveldef.c:	What the characters in a level file mean.
 *
 * Leo L. Schwab					9306.22
 */
#include <types.h>

#include "castle.h"
#include "objects.h"


#define	STDWALLFLAGS	(MEF_ARTWORK | MEF_OPAQUE | MEF_WALKSOLID | MEF_SHOTSOLID)
#define	STDWALL(ns,ew)	{NULL, ns, ew, STDWALLFLAGS, VISF_ALLDIRS }

#define	TRANSWALLFLAGS	(MEF_ARTWORK | MEF_WALKSOLID | MEF_SHOTSOLID)
#define	TRANSWALL(ns,ew) {NULL, ns, ew, TRANSWALLFLAGS, VISF_ALLDIRS }

#define	NOTWALLFLAGS	MEF_ARTWORK
#define	NOTWALL(ns,ew)	{NULL, ns, ew, NOTWALLFLAGS, VISF_ALLDIRS }

#define	ZOMBIE		{(Object *) &def_Zombie, 0, 0, 0, 0 }
#define	BOSSZOMBIE	{(Object *) &def_BossZombie, 0, 0, 0, 0 }
#define	GEORGE		{(Object *) &def_George, 0, 0, 0, 0 }
#define	BOSSGEORGE	{(Object *) &def_BossGeorge, 0, 0, 0, 0 }
#define	HEAD		{(Object *) &def_Head,   0, 0, 0, 0 }
#define	BOSSHEAD	{(Object *) &def_BossHead, 0, 0, 0, 0 }
#define	SPIDER		{(Object *) &def_Spider, 0, 0, 0, 0 }
#define	NSDOOR		{(Object *) &def_NSDoor, 0, 0, MEF_WALKSOLID | MEF_SHOTSOLID, 0 }
#define	EWDOOR		{(Object *) &def_EWDoor, 0, 0, MEF_WALKSOLID | MEF_SHOTSOLID, 0 }
#define	EXIT		{(Object *) &def_Exit,	 52, 53, MEF_OPAQUE | MEF_SHOTSOLID, VISF_ALLDIRS }
#define	DOORTRIGGER(x)	{(Object *) &def_Trigger,0, 0, 0, (x) }


#define	KEY		{(Object *) &def_Powerup,0, 1, 0, 0 }
#define	AMMO		{(Object *) &def_Powerup,1, 1, 0, 0 }
#define	MONEY1		{(Object *) &def_Powerup,2, 1, 0, 0 }
#define	MONEY2		{(Object *) &def_Powerup,3, 1, 0, 0 }
#define	MONEY3		{(Object *) &def_Powerup,4, 1, 0, 0 }
#define	HEALTH		{(Object *) &def_Powerup,5, 1, 0, 0 }
#define	LIFE		{(Object *) &def_Powerup,6, 1, 0, 0 }
#define	TALISMAN	{(Object *) &def_Powerup,7, 1, 0, 0 }

#define	DECOR1		{(Object *) &def_Powerup,10, 0, 0, 0 }
#define	DECOR2		{(Object *) &def_Powerup,11, 0, 0, 0 }
#define	DECOR3		{(Object *) &def_Powerup,12, 0, 0, 0 }

#define	OBJECT1		{(Object *) &def_Powerup,13, 0, MEF_WALKSOLID, 0 }
#define	OBJECT2		{(Object *) &def_Powerup,14, 0, MEF_WALKSOLID, 0 }
#define	OBJECT3		{(Object *) &def_Powerup,15, 0, MEF_WALKSOLID, 0 }

#define	DECOR4		{(Object *) &def_Powerup,16, 0, 0, 0 }
#define	DECOR5		{(Object *) &def_Powerup,17, 0, 0, 0 }
#define	DECOR6		{(Object *) &def_Powerup,18, 0, 0, 0 }
#define	DECOR7		{(Object *) &def_Powerup,19, 0, 0, 0 }
#define	DECOR8		{(Object *) &def_Powerup,20, 0, 0, 0 }
#define	DECOR9		{(Object *) &def_Powerup,21, 0, 0, 0 }
#define	DECOR10		{(Object *) &def_Powerup,22, 0, 0, 0 }
#define	DECOR11		{(Object *) &def_Powerup,23, 0, 0, 0 }

#define	OBJECT4		{(Object *) &def_Powerup,24, 0, MEF_WALKSOLID, 0 }
#define	OBJECT5		{(Object *) &def_Powerup,25, 0, MEF_WALKSOLID, 0 }
#define	OBJECT6		{(Object *) &def_Powerup,26, 0, MEF_WALKSOLID, 0 }
#define	OBJECT7		{(Object *) &def_Powerup,27, 0, MEF_WALKSOLID, 0 }
#define	OBJECT8		{(Object *) &def_Powerup,28, 0, MEF_WALKSOLID, 0 }
#define	OBJECT9		{(Object *) &def_Powerup,29, 0, MEF_WALKSOLID, 0 }
#define	OBJECT10	{(Object *) &def_Powerup,30, 0, MEF_WALKSOLID, 0 }
#define	OBJECT11	{(Object *) &def_Powerup,31, 0, MEF_WALKSOLID, 0 }


#define	EMPTY		{ NULL }	/*  Empty (duh)  */


/***************************************************************************
 * WARNING:  These external references are BALD FACED LIES!  It is done
 * because the true definition of the structure is not in scope.
 */
extern ObDef	def_Zombie,
		def_BossZombie,
		def_George,
		def_BossGeorge,
		def_Head,
		def_BossHead,
		def_Spider,
		def_NSDoor,
		def_EWDoor,
		def_Powerup,
		def_Exit,
		def_Trigger;



MapEntry	chardef[] = {
/*' '*/ EMPTY,
/*'!'*/	EMPTY,
/*'"'*/	EMPTY,
/*'#'*/		STDWALL (0, 0),
/*'$'*/	EMPTY,
/*'%'*/		OBJECT4,
/*'&'*/		OBJECT5,
/*'''*/		OBJECT6,
/*'('*/		DECOR4,
/*')'*/		DECOR5,
/*'*'*/		TALISMAN,
/*'+'*/		OBJECT7,
/*','*/	EMPTY,
/*'-'*/		NSDOOR,
/*'.'*/	EMPTY,
/*'/'*/		DECOR6,
/*'0'*/	EMPTY,
/*'1'*/		MONEY1,
/*'2'*/		MONEY2,
/*'3'*/		MONEY3,
/*'4'*/	EMPTY,
/*'5'*/	EMPTY,
/*'6'*/	EMPTY,
/*'7'*/	EMPTY,
/*'8'*/	EMPTY,
/*'9'*/	EMPTY,
/*':'*/		OBJECT8,
/*';'*/		OBJECT9,
/*'<'*/ EMPTY,	/*  RESERVED  */
/*'='*/		OBJECT10,
/*'>'*/	EMPTY,	/*  RESERVED  */
/*'?'*/	EMPTY,
/*'@'*/	EMPTY,
/*'A'*/		STDWALL (0, 1),
/*'B'*/		STDWALL (2, 3),
/*'C'*/		STDWALL (4, 5),
/*'D'*/		STDWALL (6, 7),
/*'E'*/		STDWALL (8, 9),
/*'F'*/		STDWALL (10, 11),
/*'G'*/		STDWALL (12, 13),
/*'H'*/		STDWALL (14, 15),
/*'I'*/		STDWALL (16, 17),
/*'J'*/		STDWALL (18, 19),
/*'K'*/		STDWALL (20, 21),
/*'L'*/		STDWALL (22, 23),
/*'M'*/		STDWALL (24, 25),
/*'N'*/		STDWALL (26, 27),
/*'O'*/		STDWALL (28, 29),
/*'P'*/		TRANSWALL (30, 31),
/*'Q'*/		TRANSWALL (32, 33),
/*'R'*/		TRANSWALL (34, 35),
/*'S'*/		TRANSWALL (36, 37),
/*'T'*/		TRANSWALL (38, 39),
/*'U'*/		TRANSWALL (40, 41),
/*'V'*/		TRANSWALL (42, 43),
/*'W'*/		NOTWALL (44, 45),
/*'X'*/		NOTWALL (46, 47),
/*'Y'*/		NOTWALL (48, 49),
/*'Z'*/		NOTWALL (50, 51),
/*'['*/		DECOR8,
/*'\'*/		DECOR7,
/*']'*/		DECOR9,
/*'^'*/	EMPTY,	/*  RESERVED  */
/*'_'*/	EMPTY,
/*'`'*/	EMPTY,
/*'a'*/		AMMO,
/*'b'*/		DECOR1,
/*'c'*/		DECOR2,
/*'d'*/		DECOR3,
/*'e'*/		DOORTRIGGER (VISF_EAST | VISF_WEST),
/*'f'*/		HEAD,	/*  Head - f for face (good as anything else)  */
/*'g'*/		GEORGE,
/*'h'*/		HEALTH,
/*'i'*/	EMPTY,
/*'j'*/	EMPTY,
/*'k'*/		KEY,
/*'l'*/		LIFE,
/*'m'*/	EMPTY,
/*'n'*/		DOORTRIGGER (VISF_NORTH | VISF_SOUTH),
/*'o'*/	EMPTY,
/*'p'*/		OBJECT1,
/*'q'*/		OBJECT2,
/*'r'*/		OBJECT3,
/*'s'*/		SPIDER,
/*'t'*/		BOSSHEAD,
/*'u'*/		DOORTRIGGER (VISF_NORTH | VISF_SOUTH | VISF_EAST | VISF_WEST),
/*'v'*/	EMPTY,	/*  RESERVED  */
/*'w'*/		BOSSZOMBIE,
/*'x'*/		EXIT,
/*'y'*/		BOSSGEORGE,
/*'z'*/		ZOMBIE,
/*'{'*/		DECOR10,
/*'|'*/		EWDOOR,
/*'}'*/		DECOR11,
/*'~'*/		OBJECT11,
/*DEL*/	EMPTY
};
