/*  :ts=8 bk=0
 *
 * loaf.h:	Structures for handling LOAF data.
 *
 * Leo L. Schwab					9304.29
 */

/***************************************************************************
 * In-memory structures.
 */
typedef struct ImageEnv {
	struct ImageEntry	*iev_ImageEntries;
	struct AnimEntry	*iev_AnimEntries;
	struct ShortCCB		*iev_SCCBs;
	uint16			*iev_PLUTBuf;
	ubyte			*iev_PDATBuf;
	int32			iev_NImageEntries,
				iev_NAnimEntries,
				iev_NSCCBs,
				iev_PLUTBufSiz,
				iev_PDATBufSiz;
} ImageEnv;

typedef struct ImageEntry {
	struct ShortCCB	*ie_SCCB;
	uint16		*ie_PLUT;
	uint32		ie_PPMPC;
} ImageEntry;

typedef struct AnimEntry {
	struct ImageEntry	*ae_Base;
	struct ImageEntry	*ae_Target;
	int32			ae_NFrames;
	int32			ae_FPS;
} AnimEntry;


typedef struct ShortCCB {
	uint32	sccb_Flags;
	uint32	sccb_PRE0,
		sccb_PRE1;
	int32	sccb_Width,
		sccb_Height;
	void	*sccb_PDAT;
} ShortCCB;


/***************************************************************************
 * On-disk structures.
 */
/*
 * Image Entries.
 */
typedef struct DiskImageEntry {
	uint32	die_SCCBIdx;
	uint32	die_PLUTIdx;
	uint32	die_PPMPC;
} DiskImageEntry;


/*
 * Short CCBs.
 */
typedef struct DiskShortCCB {
	uint32	dsc_Flags;
	uint32	dsc_PRE0,
		dsc_PRE1;
	int32	dsc_Width,
		dsc_Height;
	int32	dsc_PDATSize;
} DiskShortCCB;

typedef struct DiskAnimEntry {
	uint32	dae_Marker;	/*  == ~0			*/
	uint32	dae_ImageIdx;	/*  Index into self.		*/
	uint16	dae_NFrames,	/*  Guess...			*/
		dae_FPS;	/*  Frames per second.		*/
} DiskAnimEntry;
