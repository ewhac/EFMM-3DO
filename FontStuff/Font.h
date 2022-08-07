/***************************************************************\
*
* Header file for Greg Omi's Font routines
*
* By:  Greg Omi
*
* Last update:  22-Sep-93
*
* Copyright (c) 1993, The 3DO Company
*
* This program is proprietary and confidential
*
\***************************************************************/


/***************************************************************\
*
*		Font support routines
*
\***************************************************************/

typedef	struct FontStruct 
{
		char	*TextPtr;
		void	*FontPtr;
		int32	CoordX;
		int32	CoordY;
		int32	LineFeedOffset;
		Item	BItem;
		void	*PLUTPtr;
} FontStruct;

/*
*	Name:
*		FontInit
*	Purpose:
*		Allocates memory and initializes Font
*	Entry:
*		Maximum number of characters per line
*/

void FontInit (int32 num);

/*
*	Name:
*		FontPrint
*	Purpose:
*		Prints a text message using a font.
*		Expects the character set to be packed,
*		rotated 90 degrees to be facing down,
*		and horizontally flipped.
*		The control point is in the top left corner.
*		Accepts strings with CR/LF.
*	Entry:
*		FontStruct pointer
*/

void FontPrint (FontStruct*);

/*
*	Name:
*		FontFree
*	Purpose:
*		Frees memory allocation for characters
*/

void FontFree (void);


