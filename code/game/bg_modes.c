
#include "bg_modes.h"



bitInfo_t modeNames[MODE_NUM_MODES] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Invalid" },
	{ "Normal" },
	{ "Defrag" },
	{ "Duel" },
	{ "AllForce" },
	{ "Ironman" },
	{ "DuelQueue" },
};

modeTeam_t modeTeams[MODETEAM_NUM_MODETEAMS] = {
	//MODETEAM_INVALID,
	{
		"invalid",	// team name
		"invalid",	// team name
		MTH_INHERIT,
		qfalse,		// dont force real teams
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"0f05",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
	//MODETEAM_NORMAL,
	{
		"normal",	// team name
		"normal",	// team name
		MTH_INHERIT,
		qfalse,		// dont force real teams
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"fff0",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
	//MODETEAM_DEFRAG,
	{
		"defrag",	// team name
		"defrag",	// team name
		MTH_INHERIT,
		qtrue,		// force real team
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"fff1",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
	//MODETEAM_DUEL,
	{
		"duel",		// team name
		"duel",		// team name
		MTH_INHERIT,
		qfalse,		// force real team
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"ff73",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
	//MODETEAM_ALLFORCE,
	{
		"allforce",	// team name
		"allforce",	// team name
		MTH_INHERIT,
		qfalse,		// force real team
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"5f53",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
	//MODETEAM_IRONMAN_CAPPER,
	{
		"ironman_capper",	// team name
		"capper",	// team name
		MTH_FRIENDLY,
		qtrue,		// force real team
		qtrue,		// force team colors
		qtrue,		// don't prefix
		'T',		// team prefix color
		"55f5",		// soreboard bgcolor
		TEAM_BLUE,	// real associated team
		MODETEAM_IRONMAN_CHASER // after dying, become a chaser again
	},
	//MODETEAM_IRONMAN_CHASER,
	{
		"ironman_chaser",	// team name
		"chaser",	// team name
		MTH_FRIENDLY,
		qtrue,		// force real team
		qtrue,		// force team colors
		qtrue,		// don't prefix
		'Y',		// team prefix color
		"f555",		// soreboard bgcolor
		TEAM_RED	// real associated team
	},
	//MODETEAM_DUELQUEUE,
	{
		"duelqueue",// team name
		"queue",	// team name
		MTH_INHERIT,
		qfalse,		// force real team
		qfalse,		// dont force team colors
		qfalse,		// don't prefix
		's',		// team prefix color
		"ff35",		// soreboard bgcolor
		TEAM_FREE	// real associated team
	},
};

modeTeam_e modeDefaultTeams[MODE_NUM_MODES] = {
	MODETEAM_INVALID,
	MODETEAM_NORMAL,
	MODETEAM_DEFRAG,
	MODETEAM_DUEL,
	MODETEAM_ALLFORCE,
	MODETEAM_IRONMAN_CHASER,
	MODETEAM_DUELQUEUE,
};
