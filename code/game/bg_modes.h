#ifndef BG_MODES_H
#define BG_MODES_H

#include "q_shared.h"
#include "bg_public.h"



typedef enum playerMode_s { // NEVER change the order in this as it's part of the player configstring. If adding something, add it at the end. If adding something in a fork, add 1000 plus a few hundred (check github for other ppl who extended it?) to distinguish from TommyTernal modes, ty.
	MODE_INVALID,
	MODE_NORMAL,
	MODE_DEFRAG,
	MODE_DUEL,
	MODE_ALLFORCE,
	MODE_IRONMAN,
	MODE_DUELQUEUE,
	MODE_NUM_MODES
} playerMode_e;

typedef enum modeTeams_ee{ // NEVER change the order in this as it's part of the player configstring. If adding something, add it at the end. If adding something in a fork, add 1000 plus a few hundred (check github for other ppl who extended it?) to distinguish from TommyTernal modes, ty.
	MODETEAM_INVALID,
	MODETEAM_NORMAL,
	MODETEAM_DEFRAG,
	MODETEAM_DUEL,
	MODETEAM_ALLFORCE,
	MODETEAM_IRONMAN_CAPPER,
	MODETEAM_IRONMAN_CHASER,
	MODETEAM_DUELQUEUE,
	MODETEAM_NUM_MODETEAMS
} modeTeam_e;

typedef enum modeTeamHurtMode_s {
	MTH_INHERIT,	// inherit from ingame teams. team_Free can hurt each other, others cant
	MTH_HURT,		// hurt.
	MTH_FRIENDLY,	// don't hurt.
	MTH_COUNT_TYPES
} modeTeamHurtMode_e;

// mode-specific teams.
// each mode can define new teams.
// 
typedef struct modeTeam_s {
	const char*			name;			// the name of the team, lowercase, and make spaces _ (uppercase and spaces is done automatically where needed)
	const char*			shortname;		// shorter name for ppl in the same mode?
	modeTeamHurtMode_e	friendlyTeam;	// how does hurting others behave
	qboolean			applyRealTeam;	// when gametype >= GT_TEAM, should we force a real team for this modeteam? (spectator is unaffected)
	qboolean			applyTeamColors;// when gametype < GT_TEAM, should we force team colors?
	qboolean			applyPrefix;	// should we prefix the team name in front of playernames?
	const char			teamPrefixColor;
	const char*			scoreHexColor;
	int					realTeam;		// corresponding team if gametype >= GT_TEAM
	modeTeam_e			respawnTeam;	// which modeTeam do we respawn in after death? default MODETEAM_INVALID, which just does nothing.
} modeTeam_t;


extern bitInfo_t modeNames[MODE_NUM_MODES];
extern modeTeam_t modeTeams[MODETEAM_NUM_MODETEAMS];
extern modeTeam_e modeDefaultTeams[MODE_NUM_MODES];
#endif
