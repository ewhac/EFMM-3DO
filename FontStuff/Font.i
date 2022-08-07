	BEGINSTRUCT	FontStruct
		PTR	TextPtr
		PTR	FontPtr
		LONG	CoordX
		LONG	CoordY
		LONG	LineFeedOffset
		LONG	BItem
		PTR	PLUTPtr
	ENDSTRUCT

		IMPORT	MyAllocMem
		IMPORT	MyFreeMem


		END
