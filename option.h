/*  :ts=8 bk=0
 *
 * option.h:	Common definitions you need to know.
 *
 * Leo L. Schwab					9310.22
 */

///////////////////////////////////////
// Data Structures and defines important to me and possibly you (at least the
// SaveGameRec).
///////////////////////////////////////

#define	maxNameLen	15

typedef struct HighScoreRec {
	char	name[maxNameLen + 1];
	int32	score;
} HighScoreRec;

typedef struct SaveGameRec {
	char	name[maxNameLen + 1];	/* These first three items are part of the SaveGameRec	*/
	ubyte	valid;			/* that is passed between my code and you but what	*/
	ubyte	unused;			/* values they contain when you call SaveGame() I ignore. */

	int32	sg_score;		/* Add more fields here as you want.			*/
	int32	sg_XLifeThresh;		/* Please note that I do individual assigns of these	*/
	ubyte	sg_Level;		/* fields later on so if you add a field, add it below,	*/
	ubyte	sg_NLives;		/* too.							*/
} SaveGameRec;


#define	maxNumHighScores	5	/*  # of saved high scores	*/
#define	maxNumSaves		5	/*  # of saved game slots.	*/

typedef struct SaveInfoRec {		/* this is what's written to NVRAM */
	int32 		numScores;
	HighScoreRec	score[maxNumHighScores];
	SaveGameRec	save[maxNumSaves];
} SaveInfoRec;
