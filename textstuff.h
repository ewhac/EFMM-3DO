typedef struct FontDef {
	int32	x;		// initially header (x and y really should be kept track of somewhere else but...)
	int32	y;		// initially version
	struct	DiskShortCCB diskshortccb; // make this a full CCB to be used by this font?
	int32	charmap[128];
	int32	*pdat;	
} FontDef;

