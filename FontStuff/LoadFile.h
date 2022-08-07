/***************************************************************\
*
*		File loading support routines
*
\***************************************************************/

/*
*	Name:
*		GetFileSize
*	Purpose:
*		Gets file size from disk
*	Entry:
*		Pointer to name of file
*	Returns:
*		File size
*/

int32 GetFileSize (char *filename);

/*
*	Name:
*		LoadFile
*	Purpose:
*		Loads a file from disk
*	Entry:
*		Pointer to name of file
*		Buffer pointer (if 0 does allocmem)
*		Buffer size
*		memtype if buffer pointer is 0
*	Returns:
*		Pointer to loaded data or error if NULL
*/

void *LoadFile (char *filename, void *buffer, uint32 buffersize, uint32 memtype);

