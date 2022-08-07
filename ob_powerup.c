/*  :ts=8 bk=0
 *
 * ob_powerup.c:	Powerup objects.
 *
 * Leo L. Schwab					9307.12
 *  Modified for general-ish case			9307.21
 */
#include <types.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "loaf.h"
#include "anim.h"
#include "sound.h"
#include "ob_powerup.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define	POWERVERTS	2


/***************************************************************************
 * $(EXPLETIVE) forward reference to placate $(EXPLETIVE) compiler.
 */
static int32 powerfunc (struct ObPowerup *, int, void *);
static int32 takekey (struct ObPowerup *);
static int32 takeammo (struct ObPowerup *);
static int32 takemoneyandrun (struct ObPowerup *);
static int32 takehealth (struct ObPowerup *);
static int32 takelife (struct ObPowerup *);
static int32 taketalisman (struct ObPowerup *);
static int32 pwr_error (struct ObPowerup *);


/***************************************************************************
 * Object definition instance.
 */
struct DefPowerup	def_Powerup = {
 {	powerfunc,
	OTYP_POWERUP,
	0,
	0,
	0,
	0,
	NULL
 },
	NULL, NULL, NULL,
 {	takekey,
	takeammo,
	takemoneyandrun,
	takemoneyandrun,
	takemoneyandrun,
	takehealth,
	takelife,
	taketalisman,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error,
	pwr_error
 }
};


/***************************************************************************
 * External globals.
 */
extern Vertex	obverts[];
extern int32	nobverts;

extern CCB	*curccb;

extern uint32	linebuf[];

extern int32	nkeys;



/***************************************************************************
 * Code.
 */
static int32
powerfunc (ob, op, dat)
register struct ObPowerup	*ob;
int				op;
void				*dat;
{
	switch (op) {
	case OP_FIATLUX:
		if (!(def_Powerup.od_Env = loadloaf ("powerups.loaf")))
			die ("Couldn't load powerup images.\n");

		if (createanimloafs
		     (def_Powerup.od_Env, &def_Powerup.od_AL) < 0)
			die ("Couldn't create powerup AnimLoafs.\n");

		nkeys = 0;

		break;

	case OP_DESTRUCT:
		if (def_Powerup.od.od_ObCount > 0)
			return (def_Powerup.od.od_ObCount);
		else {
			if (def_Powerup.od_AL) {
				deleteanimloafs (def_Powerup.od_AL);
				def_Powerup.od_AL = NULL;
			}
			if (def_Powerup.od_Env) {
				freeloaf (def_Powerup.od_Env);
				def_Powerup.od_Env = NULL;
			}
		}
		break;

	case OP_CREATEOB:
		return ((int32) createStdObject ((ObDef *) &def_Powerup,
						 OTYP_POWERUP,
						 sizeof (ObPowerup),
						 def_Powerup.od.od_ObCount ?
						  0 : OBF_MOVE,
						 dat));

	case OP_DELETEOB:
		deleteStdObject ((Object *) ob);
		break;

	case OP_INITOB:
	 {
		register InitData	*id;

		id = dat;

		ob->ob.ob_State	= OBS_ASLEEP;
		ob->ob_ME	= id->id_MapEntry;
		ob->ob_XIdx	= id->id_XIdx;
		ob->ob_ZIdx	= id->id_ZIdx;
		ob->ob_PowerType= id->id_MapEntry->me_NSImage;

		ob->ob.ob_Flags	|= OBF_REGISTER | OBF_RENDER;
		if (id->id_MapEntry->me_EWImage)
			ob->ob.ob_Flags |= OBF_CONTACT | OBF_PLAYERONLY;
		break;
	 }
	case OP_MOVE:
		cycleanimloafs (def_Powerup.od_AL,
				def_Powerup.od_Env->iev_NAnimEntries,
				(int32) dat);
		break;

	case OP_REGISTER:
	 {
	 	register Vertex	*zv;
	 	frac16		x, y, z;

		if (nobverts + POWERVERTS > NOBVERTS)
			break;

		ob->ob.ob_VertIdx = nobverts;
		zv = obverts + nobverts;

		x = Convert32_F16 (ob->ob_XIdx) + HALF_F16;
		y = 0;
		z = Convert32_F16 (ob->ob_ZIdx) + HALF_F16;

		zv->X = x;
		zv->Y = y;
		zv->Z = z;
		zv++;

		zv->X = x;
		zv->Y = y - ONE_F16;
		zv->Z = z;

		nobverts += POWERVERTS;

		break;
	 }
	case OP_RENDER:
	 {
		register Vertex		*vp;
		register CCB		*ccb;
		register ImageEntry	*ie;
		register frac16		x, y;
		Point			corner[4];

		vp = obverts + ob->ob.ob_VertIdx;
		if (vp->Z < (ZCLIP >> 8))
			break;

		ie = &def_Powerup.od_Env->iev_ImageEntries[ob->ob_PowerType];

		x = PIXELS2UNITS (REALCELDIM (ie->ie_SCCB->sccb_Width));
		y = PIXELS2UNITS (REALCELDIM (ie->ie_SCCB->sccb_Height));

		x = ConvertF16_32 ((vp[0].Y - vp[1].Y) * x);
		y = ConvertF16_32 ((vp[0].Y - vp[1].Y) * y);

		corner[0].pt_X = vp[0].X - (x >> 1);
		corner[0].pt_Y = vp[0].Y - y;

		corner[1].pt_X = corner[0].pt_X + x;
		corner[1].pt_Y = vp[0].Y - y;

		corner[2].pt_X = corner[1].pt_X;
		corner[2].pt_Y = vp[0].Y;

		corner[3].pt_X = corner[0].pt_X;
		corner[3].pt_Y = vp[0].Y;

		if (!testlinebuf (linebuf, corner[0].pt_X, corner[1].pt_X))
			break;

		ccb = curccb++;
		img2ccb (ie, ccb);

		FasterMapCel (ccb, corner);
		ccb->ccb_Flags &= ~CCB_LAST;
		ob->ob.ob_Flags |= OBF_SAWME;

		break;
	 }
	case OP_CONTACT:
	 {
		register PathBox	*pb;
		frac16			ox, oz;

		pb = dat;

		ox = Convert32_F16 (ob->ob_XIdx) + HALF_F16;
		oz = Convert32_F16 (ob->ob_ZIdx) + HALF_F16;

		if (ox >= pb->End.MinX  &&  ox < pb->End.MaxX  &&
		    oz >= pb->End.MinZ  &&  oz < pb->End.MaxZ)
		{
			register int32	retval;

			retval =
			 (def_Powerup.od_PowerFuncs[ob->ob_PowerType]) (ob);

			if (retval < 0) {
				if (!ob->ob_PrevState) {
					playsound (SFX_CANTGRAB);
					ob->ob_PrevState = TRUE;
				}
			} else {
				removeobfromme ((Object *) ob, ob->ob_ME);
				if (!(ob->ob.ob_Flags & OBF_MOVE))
					ob->ob.ob_State = OBS_INVALID;

				return (retval);
			}
		} else
			ob->ob_PrevState = FALSE;
		break;
	 }
	}
	return (0);
}


static int32
takekey (ob)
struct ObPowerup *ob;
{
	nkeys++;
	playsound (SFX_GRABKEY);
	return (FALSE);
}


static int32
takeammo (ob)
struct ObPowerup *ob;
{
	extern int32	gunpower;

	if (gunpower >= 100)
		return (-1);

	if ((gunpower += 20) > 100)
		gunpower = 100;
	playsound (SFX_GRABAMMO);
	return (FALSE);
}



static int32
takemoneyandrun (ob)
struct ObPowerup *ob;
{
	extern int32	score;

	switch (ob->ob_PowerType) {
	case PWR_MONEY3:
		score += 3000;
	case PWR_MONEY2:
		score += 1000;
	case PWR_MONEY1:
		score += 1000;
	}
	playsound (SFX_GRABMONEY);
	return (FALSE);
}



static int32
takehealth (ob)
struct ObPowerup *ob;
{
	extern int32	playerhealth;

	if (playerhealth >= 100)
		return (-1);

	if ((playerhealth += 20) > 100)
		playerhealth = 100;
	playsound (SFX_GRABHEALTH);
	return (FALSE);
}

static int32
takelife (ob)
struct ObPowerup *ob;
{
	extern int32	playerlives;

	playerlives++;
	playsound (SFX_GRABLIFE);
	return (FALSE);
}

static int32
taketalisman (ob)
struct ObPowerup *ob;
{
	extern int32	score;
	extern int8	gottalisman;

	gottalisman = TRUE;
	score += 10000;
	playsound (SFX_GRABTALISMAN);
	return (FALSE);
}

static int32
pwr_error (ob)
struct ObPowerup *ob;
{
	/*
	 * This routine should never be called.
	 */
	kprintf ("HEY!  How'd you get in here?  SECURITY!!!\n");
	return (FALSE);
}
