/*  :ts=8 bk=0
 *
 * textstuff.c
 *
 * Jon Leupp						9310.01
 */
#include <types.h>
//#include <io.h>
#include <event.h>
#include <mem.h>
#include <operamath.h>
#include <graphics.h>

#include "castle.h"
#include "objects.h"
#include "imgfile.h"

#include "loaf.h"
#include "textstuff.h"

#include "app_proto.h"

int32	mytextx, mytexty; // cursor position

extern RastPort *rprend;
extern int ccbextra;

#define	VERSION		1

// really want higher structure than this FontDef to allow multiple tasks(?) to use same font
// (i.e. have various pen positions and colors and perhaps font sizes, but have the font bitmaps
//  only loaded into memory once)  Need font name in font file header?


CCB	fontccb,*fontcel=&fontccb;		// only one until I get it working (then one per FontDef)	
static int32	FontPLUT[] = {
	0x00000fff, 0x00000000, 0x00000000, 0x00000000,
};

FontDef *
LoadFont(filename)
char *filename;
{
int32	err_len;
char	*buffer;

 kprintf("Attempting to load font\n");
	fontcel->ccb_Flags = CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND | CCB_LAST;

//	fontcel->ccb_XPos = ((wide - REALCELDIM (fontcel->ccb_Width)) >> 1) << 16;
//	fontcel->ccb_YPos = (((high - REALCELDIM (fontcel->ccb_Height)) >> 1)) << 16;
	fontcel->ccb_HDX = ONE_HD;
	fontcel->ccb_VDY = ONE_VD;
	fontcel->ccb_HDY =
	fontcel->ccb_VDX =
	fontcel->ccb_HDDX =
	fontcel->ccb_HDDY = 0;
	fontcel->ccb_PLUTPtr = FontPLUT; 
 kprintf("Got here\n");
	if (!(buffer = allocloadfile(filename,MEMTYPE_CEL,&err_len))) // MEMTYPE_CEL instead of 0?
		 		die ("Couldn't load font file.\n");
	
 kprintf("Font loaded\n");
	return((FontDef*)buffer);
}

void
MyMoveTo(fontdef,x,y)
FontDef *fontdef;
int32 x,y;
{
	fontdef->x = x;
	fontdef->y = y;
 kprintf("x = %d, y = %d\n",x,y);
}

void
MyDrawText(fontdef,str)
FontDef *fontdef;
char *str;
{
int32	i,initialx;
char	c;
int32	*ptr;
static Point	corner[4];

	initialx = fontdef->x;

	for (i=0; c = str[i]; i++) {
 kprintf("i = %d, str[i] = %d, fontdef->charmap[0] = 0x%lx\n",i,c,fontdef->charmap[0]);
		switch(c) {
		case '\n':
			fontdef->x = initialx;
			fontdef->y += 10;
			break;
		default:
			ptr = (int32 *)(&(fontdef->pdat) + ((fontdef->charmap[c])>>2));
 kprintf("ptr = 0x%lx, &(fontdef->pdat) = 0x%lx, fontdef->charmap[c] = 0x%lx\n",ptr,&(fontdef->pdat),fontdef->charmap[c]);
			if (ptr == (int32 *)-1)
				fontdef->x += 8; // treat any ascii value with no legitimate cel like a space
			else {			
				corner[0].pt_Y = corner[1].pt_Y = fontdef->y;
				corner[2].pt_Y = corner[3].pt_Y = fontdef->y + ptr[1]; // ptr[1]=height
	
				corner[0].pt_X = corner[3].pt_X = fontdef->x;
				corner[1].pt_X = corner[2].pt_X = fontdef->x + ptr[0]; // ptr[0] = width

	fontcel->ccb_XPos = (fontdef->x) << 16;
	fontcel->ccb_YPos = (fontdef->y) << 16;

				fontdef->x += ptr[0]+2;
				fontcel->ccb_SourcePtr = (CelData *)(ptr+2); // +2 skips over character width and height

		fontcel->ccb_SourcePtr = (CelData *)(ptr+2); // +2 skips over character width and height

//				FasterMapCel (fontcel, corner);
				DrawCels (rprend->rp_BitmapItem, fontcel);
			}
			break;
		}
	}
}

