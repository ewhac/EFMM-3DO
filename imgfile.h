/*  :ts=8 bk=0
 *
 * imgfile.h:	Definitions for 3DO image file operations.
 *
 * Leo L. Schwab					9301.04
 */
#ifndef	_IMGFILE_H
#define	_IMGFILE_H


typedef struct CelArray {
	void	*ca_Buffer;
	int32	ca_BufSiz;
	uint32	ca_Type;	// The chunk type.
	int	ncels;
	CCB	*celptrs[1];
} CelArray;


#endif
