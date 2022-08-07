/*  :ts=8 bk=0
 *
 * anim.h:	Little helpers for animation sequencing.
 *
 * Leo L. Schwab					9307.18
 */
#ifndef	_ANIM_H
#define	_ANIM_H


/***************************************************************************
 * Relevant definitions.
 * (Why isn't there a system datum that will tell me this?)
 */
#define	VBLANKFREQ	60


/***************************************************************************
 * Animation sequencing.
 */
typedef struct AnimDef {		/*  This is mis-named.  */
	int32	ad_FPS;
	int32	ad_Counter;
} AnimDef;



typedef struct AnimLoaf {
	struct AnimDef		al_Def;
	struct AnimEntry	*al_AE;
	int32			al_CurFrame;
} AnimLoaf;


#endif	/* _ANIM_H */
