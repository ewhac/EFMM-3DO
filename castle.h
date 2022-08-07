/*  :ts=8 bk=0
 *
 * castle.h:	Various definitions.
 *
 * Leo L. Schwab					9305.18
 */
#ifndef	_CASTLE_H
#define	_CASTLE_H

#if ! (defined (__operamath_h) || defined (__OPERAMATH_H))
typedef int32	frac16;
#endif


/***************************************************************************
 * Our old friends...
 */
typedef struct Vertex {
	frac16	X, Y, Z;
} Vertex;

typedef struct Vector {
	frac16	X, Y, Z;
} Vector;

typedef struct Matrix {
	frac16	X0, Y0, Z0,
		X1, Y1, Z1,
		X2, Y2, Z2;
} Matrix;

/*
 * Because of the confusing data structures defined in operamath.h, I've
 * created my own above and cram them through the compiler with the below
 * casts.
 */
#define	VECTCAST	frac16 (*)[3]
#define	MATCAST		frac16 (*)[3]

/*
 * Useful constants.
 */
#define	ONE_F16		(1 << 16)
#define	HALF_F16	(1 << 15)

#define	F_INT(x)	((x) & ~0xFFFF)
#define	F_FRAC(x)	((x) & 0xFFFF)

#define	PIXELS2UNITS(p)	((p) << (16 - 7))	/*  1.0 == 128 pixels	*/


/***************************************************************************
 * Object definitions (monsters, bullets, tables, chairs, amphibious landing
 * crafts, etc.).
 **
 * ewhac 9307.01:  Removed to objects.h.  Go look there.
 */

/***************************************************************************
 * Cell info.
 */
typedef struct	MapEntry {
	struct Object	*me_Obs;
	ubyte		me_NSImage,	// Indicies into ImageEntries.
			me_EWImage;
	ubyte		me_Flags;
	ubyte		me_VisFlags;
} MapEntry;

/*
 * Flags defining which faces of the block have art mapped on them.
 */
#define	VISF_NORTH	1
#define	VISF_WEST	(1<<1)
#define	VISF_SOUTH	(1<<2)
#define	VISF_EAST	(1<<3)

#define	VISF_ALLDIRS	(VISF_NORTH | VISF_WEST | VISF_SOUTH | VISF_EAST)

/*
 * Flags for the automapper; they are the faces we have actually looked at.
 */
#define	MAPF_NORTH	(1<<4)
#define	MAPF_WEST	(1<<5)
#define	MAPF_SOUTH	(1<<6)
#define	MAPF_EAST	(1<<7)

#define	MAPF_ALLDIRS	(MAPF_NORTH | MAPF_WEST | MAPF_SOUTH | MAPF_EAST)


/*
 * Flags defining wall characteristics.
 */
#define	MEF_OPAQUE	1	/*  Can't see through it.		*/
#define	MEF_WALKSOLID	(1<<1)	/*  Can't walk through it.		*/
#define	MEF_SHOTSOLID	(1<<2)	/*  Can't shoot through it.		*/
#define	MEF_ARTWORK	(1<<3)	/*  There's something to draw.		*/


/***************************************************************************
 * Visible cell entry.
 */
typedef struct VisOb {
	int32		vo_LIdx,
			vo_RIdx;
	ubyte		vo_Type;	/*  I don't appear to use this... */
	ubyte		vo_ImgIdx;
	ubyte		vo_VisFlags;
	ubyte		vo_MEFlags;
	struct MapEntry	*vo_ME;
} VisOb;

#define	VOF_THINGISIMG	1		/*  vo_Thing points to ImageEntry. */

#define	VOTYP_WALL	0		/*  These mightn't get used.	*/
#define	VOTYP_OBJECT	1


/***************************************************************************
 * Joypad data.
 */
typedef struct JoyData {
	int32	jd_DX,		/*  Slide left-right.			*/
		jd_DZ,		/*  Move forward-back.			*/
		jd_DAng;	/*  Turn left-right.			*/
	ubyte	jd_ADown,	/*  Button down counts.			*/
		jd_BDown,
		jd_CDown,
		jd_XDown,
		jd_StartDown;
	ubyte	jd_FrameCount;	/*  Number of frames since last sample.	*/
} JoyData;


/***************************************************************************
 * Bounding box structures.
 */
typedef struct BBox {
	frac16	MinX, MinZ,
		MaxX, MaxZ;
} BBox;

typedef struct PathBox {
	struct BBox	Path, Start, End;
	frac16		DX, DZ;
} PathBox;


/***************************************************************************
 * Game-related constants.
 */
#define	WORLDSIZ	64	/*  Temporary; it'll really be 64  */
#define	WORLDSIZ_F16	(WORLDSIZ << 16)

#define	GRIDSIZ		(WORLDSIZ + 1)

#define	MAXNCELS	(WORLDSIZ * WORLDSIZ)
#define	NGRIDPOINTS	(GRIDSIZ * GRIDSIZ)

#define	GRIDCUTOFF	24
#define	MAXWALLVERTS	((GRIDCUTOFF + 1) * (GRIDCUTOFF + 1) * 2)

#define	NOBVERTS	256

#define	MAXVISOBS	900	/*  ### TEMPORARY; make smaller (512)	*/

#define	CX		160
#define	CY		120

#define	MAGIC		320

#define	ZCLIP		(3 * ONE_F16 >> 4)

#define	ONE_HD		(1 << 20)
#define ONE_VD		(1 << 16)

#define	REALCELDIM(x)	((x) <= 0  ?  1 << -(x)  :  (x))


/***************************************************************************
 * Rendering envinronment.
 */
typedef struct RastPort {
	Item		rp_ScreenItem;
	Item		rp_BitmapItem;
	struct Screen	*rp_Screen;
	struct Bitmap	*rp_Bitmap;
} RastPort;


#endif	/* _CASTLE_H */
