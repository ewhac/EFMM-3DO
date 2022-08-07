/*  :ts=8 bk=0
 *
 * ob_trigger.c:	A garage door opener in software.
 *
 * Leo L. Schwab					9308.13
 */
#include <types.h>
#include <operamath.h>

#include "castle.h"
#include "objects.h"
#include "sound.h"

#include "app_proto.h"


/***************************************************************************
 * Structure definitions for this object.
 */
typedef struct DefTrigger {
	struct ObDef	od;
} DefTrigger;

typedef struct ObTrigger {
	struct Object	ob;
	struct Object	*ob_DoorEntries[4];	/*  Doors to toggle.	*/
	ubyte		ob_XIdx,
			ob_ZIdx;
	ubyte		ob_PrevState;
} ObTrigger;


/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 triggerfunc (struct ObTrigger *, int, void *);


/***************************************************************************
 * Object definition instance.
 */
struct DefTrigger	def_Trigger = {
 {	triggerfunc,
	OTYP_TRIGGER,
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
extern MapEntry	levelmap[WORLDSIZ][WORLDSIZ];


/***************************************************************************
 * Code.
 */
static int32
triggerfunc (ob, op, dat)
register struct ObTrigger	*ob;
int				op;
void				*dat;
{
	switch (op) {
	case OP_FIATLUX:
		break;

	case OP_DESTRUCT:
		return (def_Trigger.od.od_ObCount);

	case OP_CREATEOB:
		return ((int32) createStdObject ((ObDef *) &def_Trigger,
						 OTYP_TRIGGER,
						 sizeof (ObTrigger),
						 0,
						 dat));

	case OP_DELETEOB:
		deleteStdObject ((Object *) ob);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;
		register Object		*tstob;
		register MapEntry	*me;
		register int		i;
		int			n, flags;

		id = dat;
		n = 0;

		ob->ob.ob_State	= OBS_ASLEEP;
		ob->ob.ob_Flags	= OBF_MOVE | OBF_CONTACT;
		ob->ob_XIdx	= id->id_XIdx;
		ob->ob_ZIdx	= id->id_ZIdx;

		flags = id->id_MapEntry->me_VisFlags;
		id->id_MapEntry->me_VisFlags = 0;

		if (flags & VISF_WEST)
			for (i = id->id_XIdx + 1;  i < WORLDSIZ;  i++) {
				me = &levelmap[id->id_ZIdx][i];
				if ((tstob = me->me_Obs)  &&
				    (tstob->ob_Type == OTYP_NSDOOR  ||
				     tstob->ob_Type == OTYP_EWDOOR))
				{
					ob->ob_DoorEntries[n++] =
					 (Object *) me;
					break;
				}
			}

		if (flags & VISF_EAST)
			for (i = id->id_XIdx - 1;  i >= 0;  i--) {
				me = &levelmap[id->id_ZIdx][i];
				if ((tstob = me->me_Obs)  &&
				    (tstob->ob_Type == OTYP_NSDOOR  ||
				     tstob->ob_Type == OTYP_EWDOOR))
				{
					ob->ob_DoorEntries[n++] =
					 (Object *) me;
					break;
				}
			}

		if (flags & VISF_NORTH)
			for (i = id->id_ZIdx + 1;  i < WORLDSIZ;  i++) {
				me = &levelmap[i][id->id_XIdx];
				if ((tstob = me->me_Obs)  &&
				    (tstob->ob_Type == OTYP_NSDOOR  ||
				     tstob->ob_Type == OTYP_EWDOOR))
				{
					ob->ob_DoorEntries[n++] =
					 (Object *) me;
					break;
				}
			}

		if (flags & VISF_SOUTH)
			for (i = id->id_ZIdx - 1;  i >= 0;  i--) {
				me = &levelmap[i][id->id_XIdx];
				if ((tstob = me->me_Obs)  &&
				    (tstob->ob_Type == OTYP_NSDOOR  ||
				     tstob->ob_Type == OTYP_EWDOOR))
				{
					ob->ob_DoorEntries[n] =
					 (Object *) me;
					break;
				}
			}

		break;
	 }
	case OP_MOVE:
	 {	/*
		 * The MOVE operation is intended to be called only once.
		 * This will resolve the MapEntry pointers to Object
		 * pointers.
		 */
		register Object	*tstob;
		register int	i;

		for (i = 4;  --i >= 0; ) {
			if (tstob = ob->ob_DoorEntries[i]) {
				tstob = ((MapEntry *) tstob)->me_Obs;
				while (tstob) {
					if (tstob->ob_Type == OTYP_NSDOOR  ||
					    tstob->ob_Type == OTYP_EWDOOR)
					{
						ob->ob_DoorEntries[i] =
						 tstob;
						break;
					}
					tstob = tstob->ob_Next;
				}
			}
		}
		ob->ob.ob_Flags &= ~OBF_MOVE;
		break;
	 }
	case OP_CONTACT:
	 {
		register PathBox	*pb;
		register Object		*door;
		register int		i;
		frac16			kx, kz;

		pb = dat;
		kx = Convert32_F16 (ob->ob_XIdx);
		kz = Convert32_F16 (ob->ob_ZIdx);

		if (kx + ONE_F16 > pb->End.MinX  &&  kx < pb->End.MaxX  &&
		    kz + ONE_F16 > pb->End.MinZ  &&  kz < pb->End.MaxZ)
		{
			if (!ob->ob_PrevState) {
				for (i = 4;  --i >= 0; )
					if (door = ob->ob_DoorEntries[i])
						toggledoor (door);

				ob->ob_PrevState = TRUE;
				playsound (SFX_FLOORTRIGGER);
			}
		} else
			ob->ob_PrevState = FALSE;
		break;
	 }
	}
	return (0);
}
