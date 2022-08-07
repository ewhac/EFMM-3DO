/*  :ts=8 bk=0
 *
 * option.c Really should be called something like "scoresave.c".
 * 			Does high score and save game and load game including NVRAM file manipulation.
 */
#include <types.h>
#include <operamath.h>
#include <event.h>
#include <mem.h>
#include <graphics.h>
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "castle.h"
#include "objects.h"
#include "imgfile.h"
#include "font.h"
#include "option.h"

#include "app_proto.h"


///////////////////////////////////////
// Externs
///////////////////////////////////////

extern JoyData	jd;
extern CCB		*backwallcel;
extern int32	joytrigger;
extern int32	oldjoybits;
extern int32	playerhealth, gunpower, playerlives, score, nkeys;

extern	int32	wide, high;
extern	Item		vblIO;
extern	RastPort	*rpvis,	*rprend;

extern	uint32		ccbextra;

extern	char	*wallimagefile;


static char	saveInfoFileName[] = "/nvram/MonsterManorData";	// NVRAM file name

static char highScoreScrStr[] = "$progdir/Hi Score.Uncoded16";
static char loadGameScrStr[]  = "$progdir/load game.Uncoded16";
static char saveGameScrStr[]  = "$progdir/Save game.Uncoded16";

#define	firstInstrXLoc	60
#define	firstInstrYLoc 	176
#define	instrLineHeight	14

#define	firstItemXLoc	50
#define	firstItemYLoc	68
#define	itemLineHeight	21
#define	secondItemXLoc	198


SaveGameRec	GameState;


SaveInfoRec	defaultsaveinfo = {
	3,
	{
		"Bols Ewhac",		2000000,
		"Magneto Man",		1500000,
		"Kimmie",		1000000,
	}
};



///////////////////////////////////////
// These are the interface functions you want to call
///////////////////////////////////////


Err DoHighScoreScr( int32 newScore );	// If newScore is -1, show high score screen
										// Otherwise, if newScore makes the high score
										// list, show it and let user enter their name.

Err	DoLoadGameScr( struct SaveGameRec *theGame );	// Fill address of SaveGameRec you pass
											// with a loaded game. Err must be zero for
											// record to be valid.

Err	DoSaveGameScr( struct SaveGameRec *theGame );	// Allow user to save game specified by theGame
											// to one or more slots in NVRAM.

Err ClearSaveStuff( int32 clearScores, int32 clearSaves );	// clear high scores and or
											// saved games (if either or both are non-zero)

// Internal Functions For My Use

static Err LoadSaveInfo( struct SaveInfoRec *theInfo );
static Err WriteSaveInfo( struct SaveInfoRec *theInfo, char *savmsg );
static Err CreateOpenSaveInfoFile( char *fileName, Item *fileItem );
static Err WriteDiskFile( Item fileItem, ubyte *data, int32 numberOfBytes );
static Err ReadDiskFile( Item fileItem, ubyte *data, int32 numberOfBytes );
static Err GetFileBlockSize( Item fileItem, uint32* blockSize );

// You might want to comment out CHECKRESULT

#define CHECKRESULT( name, val ) \
	if ( ((int32) val) < 0) \
	{ \
		printf("Error: Failure in %s: $%lx\n", name, (int32) val); \
		PrintfSysErr(val); \
	}

// Strings I use for instructions (more appear in their local routines; these are
// Used in more than one place so appear here.

	static char editInstr1Str[] = "   Up-Down changes letter.";
	static char editInstr2Str[] = "  Left-Right moves cursor.";
	static char editInstr3Str[] = "Press A when finished entering name.";
	static char editInstr4Str[] = "Press B for upper/lowercase letters.";

// Colors

#define RGB5(r,g,b) ((r<<10)|(g<<5)|b)

	static int32 BrassPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(21,19,1)<<16)|RGB5(20,18,0),
		(RGB5(14,11,10)<<16)|RGB5(10,8,0),
		(RGB5(12,10,0)<<16)|RGB5(8,7,0),
	};

	static int32 BrassHiPLUT[] =
	{
		(RGB5(0,0,0)<<16)|RGB5(0,2,0),
		(RGB5(26,24,5)<<16)|RGB5(27,25,12),
		(RGB5(27,26,17)<<16)|RGB5(28,27,22),
		(RGB5(29,29,26)<<16)|RGB5(28,27,22),
	};

// My Basic FontStruct variable that I use when drawing. Most fields are overwritten
// in the procedures.

extern	FontStruct TestText;

	static FontStruct lineOfText =
	{
		NULL,
		0,
		40,
		140,
		10,
		0,
		BrassPLUT
	};

// my global variables

int32	errlen;

// main routine; you shouldn't need this. It tests the different screens.

#if 0

int
example (ac, av)
int	ac;
char	**av;
{
	Err err;
	SaveGameRec	aGame;

	/*
	 * Excuse me a minute while I misuse wallimagefile[]...
	 */
	GetDirectory (wallimagefile, sizeof (wallimagefile));
	CreateAlias ("progdir", wallimagefile);

	openstuff();

	// Test Code

	err = DoHighScoreScr ( -1 );
	printf( "DoHighScoreScr ( -1 ) finished with err %ld\n", err );

	err = DoHighScoreScr ( 111111 );
	printf( "DoHighScoreScr ( 111111 ) finished with err %ld\n", err );

	aGame.sg_score = 11111;
	aGame.sg_XLifeThresh = 20000;
	aGame.sg_Level = 2;
	aGame.sg_NLives = 4;

	err = DoSaveGameScr( &aGame );
	printf( "save game finished with err %ld\n", err );

	err = DoLoadGameScr( &aGame );
	printf( "load game finished with err %ld\n", err );

}

#endif

// My Routines

// More globals

static CelArray	*hssCelArray = NULL;
static CCB		*hssCel = NULL;

/*
**	LoadBkgdScreen()
*/
void
LoadBkgdScreen( char *fileName )
{
	if (!(hssCelArray = parse3DO ( fileName )))
		die ("Couldn't load background screen.\n");

	hssCel = hssCelArray->celptrs[0];
	hssCel->ccb_Flags |= CCB_NPABS | CCB_LDSIZE | CCB_LDPRS |
			     CCB_LDPPMP | CCB_YOXY | CCB_ACW | ccbextra | CCB_BGND;
	hssCel->ccb_Flags &= ~(CCB_ACCW | CCB_TWD);

	hssCel->ccb_Flags |= CCB_LAST;
	hssCel->ccb_XPos = ((wide - REALCELDIM (hssCel->ccb_Width)) >> 1) << 16;
	hssCel->ccb_YPos = (((high - REALCELDIM (hssCel->ccb_Height)) >> 1)) << 16;

//	FontInit (40);	// init font to allow max of 40 chars/line
//	lineOfText.FontPtr = allocloadfile ("mm.font", MEMTYPE_CEL, &errlen);
	lineOfText.FontPtr = TestText.FontPtr;

}

/*
**	CloseBkgdScreen()	- closes high score or load/save screens
*/
void
CloseBkgdScreen()
{
	if (hssCelArray)
		{
		freecelarray (hssCelArray);
		hssCelArray = NULL;
		}

//	FontFree();
//	FreeMem( lineOfText.FontPtr, errlen );
}


/*
**	DoHighScoreScr()
**
**	At game end, call DoHighScoreScr() with the score just achieved by the player.
**	If the score is good enough to beat a currect high score, the player will be asked
**	to enter his/her name. If the score is (-1), the high score list will just be displayed
**	(i.e. no check for high score).
*/
Err
DoHighScoreScr( int32 newScore )
{
	int32 		i, j, curChar;
	int32		curEditLine;	// Which line we're putting our name into or -1 if we're
								// just looking
	RastPort 	*tmp;
	int32		fadeflag = 0;
	char		buff[64];
	int32		frame;
	SaveInfoRec	theInfo;
	Err			err;
	char		achar = 'A';
	char		zchar = 'Z';

	static char noEditInstrStr[] = " Press A, B, or C when done.";
	static char editInstr3Str[] = "     Press A when done.";

	// Load Scores From NVRAM

	resetjoydata();
	err = LoadSaveInfo( &theInfo );
	if ( err )
		{
		return err;
		}

	// Check newScore with high score list

	if ( newScore == -1 )
		{
		curEditLine = -1;
		}
	else
		{
		for ( i = 0; i < theInfo.numScores; i++ )
			{
			if ( newScore > theInfo.score[i].score )
				{
				break;
				}
			}

		printf( "new score fits in slot %ld\n", i );

		if ( i < maxNumHighScores )
			{
			for ( j = maxNumHighScores - 2; j >= i; j-- )
				{
				memcpy( &theInfo.score[j+1], &theInfo.score[j], sizeof( HighScoreRec ) );
				}

			theInfo.score[i].score = newScore;
			theInfo.score[i].name[0] = achar;
			theInfo.score[i].name[1] = 0;
			curEditLine = i;
			curChar = 0;

			if ( i >= theInfo.numScores )
				{
				theInfo.numScores++;
				}
			}
		else
			{
			return 0;				// Don't show screen if we have a low score
			}
		}

	frame = 0;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame

	LoadBkgdScreen( highScoreScrStr );

	SetRast (rprend, 0);
	tmp = rpvis;  rpvis = rprend;  rprend = tmp;
	fadetolevel (rprend, 0);
	resetjoydata();
	WaitVBL (vblIO, 1);

	do
		{

		DrawCels (rprend->rp_BitmapItem, backwallcel);
		DrawCels (rprend->rp_BitmapItem, hssCel);

		lineOfText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		for ( i = 0; i < theInfo.numScores; i++ )
			{
			if ( i == curEditLine )
				{
				lineOfText.PLUTPtr = BrassHiPLUT;
				}
			else
				{
				lineOfText.PLUTPtr = BrassPLUT;
				}

			lineOfText.CoordY = firstItemYLoc + ( itemLineHeight * i );

			// Draw Name

			lineOfText.CoordX = firstItemXLoc;
			lineOfText.TextPtr = theInfo.score[i].name;

			FontPrint(&lineOfText);

			// Draw Number

			sprintf( buff, "%8ld", theInfo.score[i].score );
			for (j=0; buff[j]==' '; buff[j++]='^');
			lineOfText.CoordX = secondItemXLoc;
			lineOfText.TextPtr = buff;

			FontPrint(&lineOfText);
			}

		// Instructions at end

		lineOfText.CoordX = firstInstrXLoc;
		lineOfText.CoordY = firstInstrYLoc;
		lineOfText.PLUTPtr = BrassPLUT;

		if ( curEditLine == -1 )
			{
			lineOfText.CoordY += instrLineHeight;
			lineOfText.TextPtr = noEditInstrStr;
			FontPrint(&lineOfText);
			}
		else
			{
			lineOfText.TextPtr = editInstr1Str;
			FontPrint(&lineOfText);

			lineOfText.CoordY += instrLineHeight;
			lineOfText.TextPtr = editInstr2Str;
			FontPrint(&lineOfText);

			lineOfText.CoordY += instrLineHeight;
			lineOfText.TextPtr = editInstr3Str;
			FontPrint(&lineOfText);
			}

		// Process High Score Entry

		if ( curEditLine != -1 )
			{
			if ( (joytrigger & ControlDown) ||
					((!(frame&7))&&(oldjoybits & ControlDown)))
				{
				frame = 0;

				switch ( theInfo.score[curEditLine].name[curChar] )
					{
					case 'A':
					case 'a':
						theInfo.score[curEditLine].name[curChar] = '9';
						break;
					case '0':
						theInfo.score[curEditLine].name[curChar] = ' ';
						break;
					case ' ':
						theInfo.score[curEditLine].name[curChar] = zchar;
						break;
					default:
						theInfo.score[curEditLine].name[curChar]--;
						break;
					}
				}

			if ( (joytrigger & ControlUp) ||
					((!(frame&7))&&(oldjoybits & ControlUp)))
				{
				frame = 0;

				switch ( theInfo.score[curEditLine].name[curChar] )
					{
					case 'z':
					case 'Z':
						theInfo.score[curEditLine].name[curChar] = ' ';
						break;
					case '9':
						theInfo.score[curEditLine].name[curChar] = achar;
						break;
					case ' ':
						theInfo.score[curEditLine].name[curChar] = '0';
						break;
					default:
						theInfo.score[curEditLine].name[curChar]++;
						break;
					}
				}

			if ( (joytrigger & ControlRight) ||
					((!(frame&7))&&(oldjoybits & ControlRight)))
				{
				if ( curChar < (maxNameLen - 1) )
					{
					curChar++;

					theInfo.score[curEditLine].name[curChar] =
							theInfo.score[curEditLine].name[curChar - 1];
					theInfo.score[curEditLine].name[curChar + 1] = 0;

					frame = 0;
					}
				}

			if ( (joytrigger & ControlLeft) ||
					((!(frame&7))&&(oldjoybits & ControlLeft)))
				{
				if ( curChar > 0 )
					{
					theInfo.score[curEditLine].name[curChar] = 0;
					curChar--;

					frame = 0;
					}
				}

//			if ( (joytrigger & ControlB) || 				// Switch to lower case.
//					((!(frame&15))&&(oldjoybits & ControlB)))
			if (joytrigger & ControlB) 				// Switch to lower case.
				{
				if ( achar == 'A' )
					{
					achar = 'a';
					zchar = 'z';
					if ( theInfo.score[curEditLine].name[curChar] >= 'A' &&
							theInfo.score[curEditLine].name[curChar] <= 'Z' )
						{
						theInfo.score[curEditLine].name[curChar] += ('a' - 'A');
						}
					}
				else
					{
					achar = 'A';
					zchar = 'Z';
					if ( theInfo.score[curEditLine].name[curChar] >= 'a' &&
							theInfo.score[curEditLine].name[curChar] <= 'z' )
						{
						theInfo.score[curEditLine].name[curChar] -= ('a' - 'A');
						}
					}
				}


//			if ( (joytrigger & ControlA) || 				// Done. Save name.
//					((!(frame&15))&&(oldjoybits & ControlA)))
			if (joytrigger & ControlA)	 				// Done. Save name.
				{
				err = WriteSaveInfo( &theInfo, "Score\n Saved." );
				curEditLine = -1;
//??			joytrigger &= ~ControlA;
				}
			}

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		if (!fadeflag)
			{
			fadeup (rpvis, 32);
			fadeflag++;
			}
		else
			{
			DisplayScreen (rpvis->rp_ScreenItem, 0);
			resetjoydata();
			WaitVBL (vblIO, 1);
			}
		frame++;

		} while ( curEditLine != -1 ||
					(( joytrigger & ( ControlA | ControlB | ControlC) ) == 0) );

	oldjoybits = 0xff;
	jd.jd_ADown = jd.jd_BDown = jd.jd_CDown = 0;
	fadetoblank(rpvis, 32);
	CloseBkgdScreen();

	return err;
}

/*
**	DoLoadGameScr()
**
**	Select from the games stored in NVRAM for loading.
**	The parameter "theGame" is a pointer to a record allocated by the calling routine
**	which I will fill in with the desired game.
**
**	If Err (the result code) is -1, the user cancelled.
**	If Err is something else, it is a load NVRAM error.
*/
Err
DoLoadGameScr( struct SaveGameRec *theGame )
{
	int32 		i;
	RastPort 	*tmp;
	int32		fadeflag = 0;
	char		buff[64];
	int32		frame;
	SaveInfoRec	theInfo;
	Err			err = 0;
	int32		curSelLine = -1;

	static char loadInstr1Str[] = "Use Up-Down to select game.";
	static char loadInstr2Str[] = "     Press A to load game.";
	static char loadInstr3Str[] = "       Press C to cancel.";
	static char loadInstr4Str[] = "   Sorry, no games to load.";

	// Load Saved Games From Disk

	LoadSaveInfo( &theInfo );

	for ( i = 0; i < maxNumSaves; i++ )
		{
		if ( theInfo.save[i].valid )
			{
			curSelLine = i;
			break;
			}
		}

	frame = 0;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame
	joytrigger &= ~ControlA; // untrigger A so won't end on 1st frame

	LoadBkgdScreen( loadGameScrStr );

	SetRast (rprend, 0);
	tmp = rpvis;  rpvis = rprend;  rprend = tmp;
	fadetolevel (rprend, 0);
	WaitVBL (vblIO, 1);

	do
		{
		resetjoydata();

		DrawCels (rprend->rp_BitmapItem, backwallcel);
		DrawCels (rprend->rp_BitmapItem, hssCel);

		lineOfText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		for ( i = 0; i < maxNumSaves; i++ )
			{
			if ( i == curSelLine )
				{
				lineOfText.PLUTPtr = BrassHiPLUT;
				}
			else
				{
				lineOfText.PLUTPtr = BrassPLUT;
				}

			lineOfText.CoordY = firstItemYLoc + ( itemLineHeight * i );

			// Draw Name

			lineOfText.CoordX = firstItemXLoc;

			if ( !theInfo.save[i].valid )
				{
				theInfo.save[i].name[0] = 0;
				}

			sprintf (buff, "%ld) %s",
				 i + 1, theInfo.save[i].name);
			lineOfText.TextPtr = buff;

			FontPrint(&lineOfText);
			}

		// Instructions at end

		lineOfText.CoordX = firstInstrXLoc;
		lineOfText.PLUTPtr = BrassPLUT;

		if ( curSelLine != -1 )
			{
			lineOfText.CoordY = firstInstrYLoc;
			lineOfText.TextPtr = loadInstr1Str;
			FontPrint(&lineOfText);

			lineOfText.CoordY += instrLineHeight;
			lineOfText.TextPtr = loadInstr2Str;
			FontPrint(&lineOfText);
			}
		else
			{
			lineOfText.CoordY = firstInstrYLoc;
			lineOfText.TextPtr = loadInstr4Str;
			FontPrint(&lineOfText);
			}

		lineOfText.CoordY += instrLineHeight;
		lineOfText.TextPtr = loadInstr3Str;
		FontPrint(&lineOfText);

		if ( curSelLine != -1 )
			{
			if ( (joytrigger & ControlDown) ||
					((!(frame&7))&&(oldjoybits & ControlDown)))
				{
				frame = 0;

				do	{
					curSelLine++;

					if ( curSelLine >= maxNumSaves )
						{
						curSelLine = 0;
						}
					} while ( !theInfo.save[curSelLine].valid );
				}

			if ( (joytrigger & ControlUp) ||
					((!(frame&7))&&(oldjoybits & ControlUp)))
				{
				frame = 0;

				do	{
					curSelLine--;

					if ( curSelLine < 0 )
						{
						curSelLine = maxNumSaves - 1;
						}
					} while ( !theInfo.save[curSelLine].valid );
				}
			}

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		if (!fadeflag)
			{
			fadeup (rpvis, 32);
			fadeflag++;
			}
		else
			{
			DisplayScreen (rpvis->rp_ScreenItem, 0);
			WaitVBL (vblIO, 1);
			}
		frame++;

		} while ( (joytrigger & ( ControlA | ControlC)) == 0  );

	if ( err == 0 )	// if we have no error & the player hit A, load game!
		{
		if ( curSelLine >= 0   &&  (joytrigger & ControlA ) )
			{
			memcpy( theGame, &theInfo.save[curSelLine], sizeof( SaveGameRec ) );
			}
		else
			{
			err = -1; // User Cancelled
			}
		}

	oldjoybits = 0xff;
	jd.jd_ADown = jd.jd_BDown = jd.jd_CDown = 0;
	fadetoblank(rpvis, 32);
	CloseBkgdScreen();

	return err;
}


// Save Game

/*
**	DoSaveGameScr()
**
**	Save the game pointed to by theGame.
**
**	If Err (the result code) is -1, the user cancelled.
**	If Err is something else, it is a load NVRAM error.
*/
Err
DoSaveGameScr( struct SaveGameRec *theGame )
{
	int32 		i, curChar;
	RastPort 	*tmp;
	int32		fadeflag = 0;
	char		buff[64];
	SaveGameRec	oldSave;
	int32		frame;
	SaveInfoRec	theInfo;
	Err			err = 0;
	char		achar = 'A';
	char		zchar = 'Z';
	int32		curSelLine = -1;
	int32		curEditLine = -1;

	static char saveInstr1Str[] = "   A saves to selected slot.";
	static char saveInstr2Str[] = " B changes name then saves.";
	static char saveInstr3Str[] = "     Press C when done.";
	static char editInstr3Str[] = "Press A when done, C cancels.";
	static char	gamesaved[] = "Your game has\nbeen saved.";

	// Load Saved Games From Disk

	LoadSaveInfo( &theInfo );

	curSelLine = 0;

	frame = 0;
	joytrigger &= ~ControlC; // untrigger C so won't end on 1st frame
	joytrigger &= ~ControlB; // untrigger B so won't end on 1st frame
	joytrigger &= ~ControlA; // untrigger A so won't end on 1st frame
	//stattext[GOTOGAME].str = start_game[0];
	//stattext[GOTOGAME].x = OPTX+108 - (4*0);

	LoadBkgdScreen( saveGameScrStr );

	SetRast (rprend, 0);
	tmp = rpvis;  rpvis = rprend;  rprend = tmp;

	do
		{
		resetjoydata();

		DrawCels (rprend->rp_BitmapItem, backwallcel);
		DrawCels (rprend->rp_BitmapItem, hssCel);

		lineOfText.BItem = rprend->rp_BitmapItem;	// Set Bitmap to write font to once per VBLANK

		for ( i = 0; i < maxNumSaves; i++ )
			{
			if ( i == curSelLine )
				{
				lineOfText.PLUTPtr = BrassHiPLUT;
				}
			else
				{
				lineOfText.PLUTPtr = BrassPLUT;
				}

			lineOfText.CoordY = firstItemYLoc + ( itemLineHeight * i );

			// Draw Name

			lineOfText.CoordX = firstItemXLoc;

			if ( !theInfo.save[i].valid )
				{
				theInfo.save[i].name[0] = 0;
				}

			sprintf (buff, "%ld) %s",
				 i + 1, theInfo.save[i].name);
			lineOfText.TextPtr = buff;

			FontPrint(&lineOfText);
			}

		// Instructions at end

		lineOfText.CoordX = firstInstrXLoc;
		lineOfText.PLUTPtr = BrassPLUT;

		lineOfText.CoordY = firstInstrYLoc;
		lineOfText.TextPtr = ((curEditLine == -1) ? saveInstr1Str : editInstr1Str);
		FontPrint(&lineOfText);

		lineOfText.CoordY += instrLineHeight;
		lineOfText.TextPtr = ((curEditLine == -1) ? saveInstr2Str : editInstr2Str);
		FontPrint(&lineOfText);

		lineOfText.CoordY += instrLineHeight;
		lineOfText.TextPtr = ((curEditLine == -1) ? saveInstr3Str : editInstr3Str);
		FontPrint(&lineOfText);

		// Process High Score Entry

		if ( curEditLine != -1 )	// in edit mode
			{
			if ( (joytrigger & ControlDown) ||
					((!(frame&7))&&(oldjoybits & ControlDown)))
				{
				frame = 0;

				switch ( theInfo.save[curEditLine].name[curChar] )
					{
					case 'A':
					case 'a':
						theInfo.save[curEditLine].name[curChar] = '9';
						break;
					case '0':
						theInfo.save[curEditLine].name[curChar] = ' ';
						break;
					case ' ':
						theInfo.save[curEditLine].name[curChar] = zchar;
						break;
					default:
						theInfo.save[curEditLine].name[curChar]--;
						break;
					}
				}

			if ( (joytrigger & ControlUp) ||
					((!(frame&7))&&(oldjoybits & ControlUp)))
				{
				frame = 0;

				switch ( theInfo.save[curEditLine].name[curChar] )
					{
					case 'z':
					case 'Z':
						theInfo.save[curEditLine].name[curChar] = ' ';
						break;
					case '9':
						theInfo.save[curEditLine].name[curChar] = achar;
						break;
					case ' ':
						theInfo.save[curEditLine].name[curChar] = '0';
						break;
					default:
						theInfo.save[curEditLine].name[curChar]++;
						break;
					}
				}

			if ( (joytrigger & ControlRight) ||
					((!(frame&7))&&(oldjoybits & ControlRight)))
				{
				if ( curChar < (maxNameLen - 1) )
					{
					curChar++;

					theInfo.save[curEditLine].name[curChar] =
							theInfo.save[curEditLine].name[curChar - 1];
					theInfo.save[curEditLine].name[curChar + 1] = 0;

					frame = 0;
					}
				}

			if ( (joytrigger & ControlLeft) ||
					((!(frame&7))&&(oldjoybits & ControlLeft)))
				{
				if ( curChar > 0 )
					{
					theInfo.save[curEditLine].name[curChar] = 0;
					curChar--;

					frame = 0;
					}
				}

			if ( (joytrigger & ControlB) || 				// Switch to lower case.
					((!(frame&15))&&(oldjoybits & ControlB)))
				{
				if ( achar == 'A' )
					{
					achar = 'a';
					zchar = 'z';
					if ( theInfo.save[curEditLine].name[curChar] >= 'A' &&
							theInfo.save[curEditLine].name[curChar] <= 'Z' )
						{
						theInfo.save[curEditLine].name[curChar] += ('a' - 'A');
						}
					}
				else
					{
					achar = 'A';
					zchar = 'Z';
					if ( theInfo.save[curEditLine].name[curChar] >= 'a' &&
							theInfo.save[curEditLine].name[curChar] <= 'z' )
						{
						theInfo.save[curEditLine].name[curChar] -= ('a' - 'A');
						}
					}
				}


			if ( (joytrigger & ControlA) ||
					((!(frame&15))&&(oldjoybits & ControlA)))
				{
				err = WriteSaveInfo( &theInfo, gamesaved);
				curEditLine = -1;
				joytrigger -= ControlA;
				}

			if ( (joytrigger & ControlC) ||
					((!(frame&15))&&(oldjoybits & ControlC)))
				{
				// cancel

				theInfo.save[curSelLine] = oldSave;
				curEditLine = -1;
				joytrigger -= ControlC;
				}

			}
		else						// not in edit mode
			{
			if ( (joytrigger & ControlDown) ||
					((!(frame&7))&&(oldjoybits & ControlDown)))
				{
				frame = 0;

				curSelLine++;

				if ( curSelLine >= maxNumSaves )
					{
					curSelLine = 0;
					}
				}

			if ( (joytrigger & ControlUp) ||
					((!(frame&7))&&(oldjoybits & ControlUp)))
				{
				frame = 0;

				curSelLine--;

				if ( curSelLine < 0 )
					{
					curSelLine = maxNumSaves - 1;
					}
				}

			if ( (joytrigger & ControlA) ||
					((!(frame&15))&&(oldjoybits & ControlA)))
				{
				theInfo.save[curSelLine].valid = true;
				theInfo.save[curSelLine].sg_score = theGame->sg_score;
				theInfo.save[curSelLine].sg_XLifeThresh = theGame->sg_XLifeThresh;
				theInfo.save[curSelLine].sg_Level = theGame->sg_Level;
				theInfo.save[curSelLine].sg_NLives = theGame->sg_NLives;

				err = WriteSaveInfo( &theInfo, gamesaved);	// just write info
				}


			if ( (joytrigger & ControlB) || 			// go into edit mode, write at end
					((!(frame&15))&&(oldjoybits & ControlB)))
				{
				oldSave = theInfo.save[curSelLine];

				theInfo.save[curSelLine].valid = true;
				theInfo.save[curSelLine].name[0] = 'A';
				theInfo.save[curSelLine].name[1] = 0;
				theInfo.save[curSelLine].sg_score = theGame->sg_score;
				theInfo.save[curSelLine].sg_XLifeThresh = theGame->sg_XLifeThresh;
				theInfo.save[curSelLine].sg_Level = theGame->sg_Level;
				theInfo.save[curSelLine].sg_NLives = theGame->sg_NLives;

				curEditLine = curSelLine;
				curChar = 0;
				achar = 'A';
				zchar = 'Z';
				frame = 0;
				}

			}


		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		if (!fadeflag)
			{
			fadeup (rpvis, 32);
			fadeflag++;
			}
		else
			{
			DisplayScreen (rpvis->rp_ScreenItem, 0);
			WaitVBL (vblIO, 1);
			}
		frame++;

		} while ( curEditLine != -1 || (joytrigger & (ControlC)) == 0  );

	oldjoybits = 0xff;
	jd.jd_ADown = jd.jd_BDown = jd.jd_CDown = 0;
	fadetoblank(rpvis, 32);
	CloseBkgdScreen();

	return err;
}

/*
**	ClearSaveStuff()
**
**	Clear the NVRAM save file. Choose to clear high scores or saved games or both.
**
*/
Err
ClearSaveStuff( int32 clearScores, int32 clearSaves )
{
	SaveInfoRec	theInfo;
	Err			err = 0;
	int32		i;

	err = LoadSaveInfo( &theInfo );

	if ( err == 0 && clearScores )
		{
		theInfo.numScores = 0;
		}

	if ( err == 0 && clearSaves )
		{
		for ( i = 0; i < maxNumSaves; i++ )
			{
			theInfo.save[i].valid = 0;
			}
		}

	if ( err == 0 )
		{
		err = WriteSaveInfo( &theInfo, "Data Erased." );
		}

	return err;
}

// File Reading/Writing Stuff \\

/*
**	LoadSaveInfo()
**
**	Load the save info from NVRAM if it exists. Otherwise create it & write it.
*/
static Err
LoadSaveInfo( struct SaveInfoRec *theInfo )
{
	Item 	fileItem;
	Err		err = 0;

	fileItem = OpenDiskFile( saveInfoFileName );
	if ( fileItem < 0 )
		{
		err = CreateOpenSaveInfoFile( saveInfoFileName, &fileItem );
		}

	if ( fileItem < 0 || err )
		{
		printf( "failed to open and/or create nvram file");
		memset( theInfo, 0, sizeof( SaveInfoRec ));	// all zeroes is good starting state
		return fileItem;
		}

	// If we make it here, we either just opened our file or we called createopen...
	// which created the file and wrote our default data into it.

	err = ReadDiskFile( fileItem, (ubyte *) theInfo, sizeof( SaveInfoRec ) );

	err = CloseDiskFile( fileItem );

	return err;
}

/*
**	LoadSaveInfo()
**
**	Load the save info from NVRAM if it exists. Otherwise create it & write it.
*/
static Err
WriteSaveInfo( struct SaveInfoRec *theInfo, char *savmsg )
{
	Item 	fileItem;
	Err		err = 0;

	fileItem = OpenDiskFile( saveInfoFileName );
	if ( fileItem < 0 )
		{
		printf( "failed to open");
		return fileItem;
		}

	err = WriteDiskFile( fileItem, (ubyte *) theInfo, sizeof( SaveInfoRec ) );

	err = CloseDiskFile( fileItem );

	if (err < 0)
		rendermessage (240, "There was\nan error.\nData not\nsaved.");
	else
		rendermessage (120, savmsg);

	return err;
}

/*
**	CreateOpenSaveInfoFile()
**
**	Create a new NVRAM file if we can. If we succeed, open it, alloc room for it,
**	and fill it with our default data.
*/

static Err
CreateOpenSaveInfoFile( char *fileName, Item *fileItem )
{
	Err err;
	Item ioReqItem;
	uint32 numberOfBlocks, blockSize;

	*fileItem = CreateFile( fileName );
	CHECKRESULT( "CreateNVRAMFILE:Create File", *fileItem );
	err = ( *fileItem < 0 ) ? *fileItem : 0;

	if ( err == 0 )
		{
		*fileItem = OpenDiskFile( fileName );
		CHECKRESULT( "CreateBVRANFUKELOpenDiskFile", *fileItem );
		err = ( *fileItem < 0 ) ? *fileItem : 0;

		if ( err == 0 )
			{
			err = GetFileBlockSize( *fileItem, &blockSize );
			CHECKRESULT( "createNVRAMFile:GetFileBlockSize", err );

			if ( err == 0 )
				{
				numberOfBlocks = (sizeof (SaveInfoRec) + blockSize - 1) / blockSize;

				ioReqItem = CreateIOReq( NULL, 0, *fileItem, 0 );
				CHECKRESULT( "CreateNVRAMFile:CreateIOReq", ioReqItem );

				if ( ioReqItem >= 0 )
					{
					IOInfo fileInfo;

					memset( &fileInfo, 0, sizeof(IOInfo) );

					fileInfo.ioi_Command = FILECMD_ALLOCBLOCKS;
					fileInfo.ioi_Offset = numberOfBlocks;

					err = DoIO( ioReqItem, &fileInfo );
					CHECKRESULT( "CreateNVRAMFile:DoIO", err );

					err = DeleteIOReq( ioReqItem );
					CHECKRESULT( "CreateNVRAMFile:DeleteIOReq", err );
					}
				else
					{
					err = ioReqItem;
					}
				}

			if ( err == 0 )
				{
				err = WriteDiskFile( *fileItem, (ubyte *) &defaultsaveinfo, sizeof( SaveInfoRec ) );
				CHECKRESULT( "CreateNVRAMFile:WriteDiskFile", err );
				}
			}
		}

	return err;
}


static Err
WriteDiskFile( Item fileItem, ubyte *data, int32 numberOfBytes )
{
	int32 err;
	IOInfo fileInfo;
	Item ioReqItem;

	ioReqItem = CreateIOReq( NULL, 0, fileItem, 0 );
	CHECKRESULT( "WriteNVRAMFile:CreateIOReq", ioReqItem );

	if ( ioReqItem >= 0 )
		{
		memset( &fileInfo, 0, sizeof(IOInfo) );

		fileInfo.ioi_Command = CMD_WRITE;
		fileInfo.ioi_Send.iob_Buffer = data;
		fileInfo.ioi_Send.iob_Len = (int) (numberOfBytes & ~3);

		fileInfo.ioi_Offset = 0;

		err = DoIO( ioReqItem, &fileInfo );
		CHECKRESULT( "WriteNVRAMFIle:DoIO", err );

		err = DeleteIOReq( ioReqItem );
		CHECKRESULT( "WriteNVRamFile:DeleteIOReq", err );
		}
	else
		{
		err = ioReqItem;
		}

	return err;
}

static Err
ReadDiskFile( Item fileItem, ubyte *data, int32 numberOfBytes )
{
	int32 err;
	IOInfo fileInfo;
	Item ioReqItem;

	ioReqItem = CreateIOReq( NULL, 0, fileItem, 0 );
	CHECKRESULT( "ReadNVRAMFile:CreateIOReq", ioReqItem );

	if ( ioReqItem >= 0 )
		{
		memset( &fileInfo, 0, sizeof(IOInfo) );

		fileInfo.ioi_Command = CMD_READ;

		fileInfo.ioi_Recv.iob_Buffer = data;
		fileInfo.ioi_Recv.iob_Len = numberOfBytes;

		fileInfo.ioi_Offset = 0;

		err = DoIO( ioReqItem, &fileInfo );
		CHECKRESULT( "ReadNVRAMFIle:DoIO", err );

		err = DeleteIOReq( ioReqItem );
		CHECKRESULT( "ReadNVRamFile:DeleteIOReq", err );
		}
	else
		{
		err = ioReqItem;
		}

	return err;
}

static Err
GetFileBlockSize( Item fileItem, uint32* blockSize )
{
	int32 		err;
	IOInfo 		fileInfo;
	FileStatus	status;
	Item 		ioReqItem;

	ioReqItem = CreateIOReq( NULL, 0, fileItem, 0 );
	CHECKRESULT( "GetFileBlockSize:CreateIOReq", ioReqItem );

	if ( ioReqItem >= 0 )
		{
		memset( &fileInfo, 0, sizeof(IOInfo) );

		fileInfo.ioi_Command = CMD_STATUS;

		fileInfo.ioi_Recv.iob_Buffer = &status;
		fileInfo.ioi_Recv.iob_Len = sizeof( FileStatus );

		err = DoIO( ioReqItem, &fileInfo );
		CHECKRESULT( "GetFileBlockSize:DoIO", err );

		err = DeleteIOReq( ioReqItem );
		CHECKRESULT( "GetFileBlockSize:DeleteIOReq", err );

		if ( err == 0 )
			{
			*blockSize = status.fs.ds_DeviceBlockSize;
			}
		}
	else
		{
		err = ioReqItem;
		}

	return err;
}
