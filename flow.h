/*  :ts=8 bk=0
 *
 * flow.h:	Simple constants for flow control.
 *
 * Leo L. Schwab					9310.22
 */

enum	FlowControl {
	FC_NOP = 0,
	FC_RESTART,
	FC_NEWGAME,
	FC_RESUMEGAME,
	FC_LOADGAME,
	FC_SAVEGAME,
	FC_PRACTICEGAME,
	FC_DIED,
	FC_COMPLETED,
	FC_USERABORT,
	FC_BUCKY_NEXTLEVEL,
	FC_BUCKY_QUIT,
	MAX_FC
};
