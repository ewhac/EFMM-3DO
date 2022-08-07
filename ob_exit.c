/*  :ts=8 bk=0
 *
 * ob_exit.c:	Level exit.
 *
 * Leo L. Schwab					9307.21
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"

#include "app_proto.h"


/***************************************************************************
 * Structure definitions for this object.
 */
typedef struct DefExit {
	struct ObDef	od;
} DefExit;

typedef struct ObExit {
	struct Object	ob;
	struct MapEntry	*ob_ME;
	ubyte		ob_XIdx,
			ob_ZIdx;
} ObExit;


/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 exitfunc (struct ObExit *, int, void *);


/***************************************************************************
 * Object definition instance.
 */
struct DefExit	def_Exit = {
 {	exitfunc,
	OTYP_EXIT,
	0,
	0,
	0,
	0,
	NULL
 }
};


/***************************************************************************
 * External globals.
 */
extern int8	exitedlevel;
extern int8	gottalisman;


/***************************************************************************
 * Code.
 */
static int32
exitfunc (ob, op, dat)
register struct ObExit	*ob;
int			op;
void			*dat;
{
	switch (op) {
	case OP_FIATLUX:
		break;

	case OP_DESTRUCT:
		return (def_Exit.od.od_ObCount);

	case OP_CREATEOB:
		return ((int32) createStdObject ((ObDef *) &def_Exit,
						 OTYP_EXIT,
						 sizeof (ObExit),
						 0,
						 dat));

	case OP_DELETEOB:
		deleteStdObject ((Object *) ob);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;

		id = dat;

		ob->ob.ob_State	= OBS_ASLEEP;
		ob->ob.ob_Flags	= OBF_CONTACT | OBF_PLAYERONLY;
		ob->ob_ME	= id->id_MapEntry;
		ob->ob_XIdx	= id->id_XIdx;
		ob->ob_ZIdx	= id->id_ZIdx;
		break;
	 }
	case OP_CONTACT:
	 {
		register PathBox	*pb;
		BBox			eb;

		pb = dat;

		eb.MinX = Convert32_F16 (ob->ob_XIdx);
		eb.MinZ = Convert32_F16 (ob->ob_ZIdx);
		eb.MaxX = eb.MinX + ONE_F16;
		eb.MaxZ = eb.MinZ + ONE_F16;

		if (checkcontact (pb, &eb, TRUE)) {
			if (gottalisman) {
				exitedlevel = TRUE;
				rendermessage (120, "Exit!");
			} else
				rendermessage (120, "You need the\nTalisman.");
			fullstop ();
			return (TRUE);
		}
		break;
	 }
	}
	return (0);
}
