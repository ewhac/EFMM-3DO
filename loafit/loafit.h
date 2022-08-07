struct ImageEntry {
	struct	ShortCCB *ie_SCCB;
	uint32	*ie_PLUT;
	uint32	ie_PPMP;
};

struct ShortCCB {
	uint32	sccb_Flags;
	uint32	sccb_PRE0;
	uint32	sccb_PRE1;
	int32	sccb_Width;
	int32	sccb_Height;
	uint32	*sccb_PDATSize;
};

struct DiskImageEntry {
	uint32	die_ShortCCBIdx;
	uint32	die_PLUTIdx;
	uint32	die_PPMP;
};

struct DiskShortCCB {
	uint32	dsc_Flags;
	uint32	dsc_PRE0;
	uint32	dsc_PRE1;
	int32	dsc_Width;
	int32	dsc_Height;
	uint32	dsc_PDATSize;
};


typedef struct DiskAnimEntry {
	uint32	dae_Marker;	/*  == ~0			*/
	uint32	dae_ImageIdx;	/*  Index into self.		*/
	uint16	dae_NFrames,	/*  Guess...			*/
		dae_FPS;	/*  Frames per second.		*/
} DiskAnimEntry;

