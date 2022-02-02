// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i\n", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("%s" S_COLOR_WHITE " (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f( void ) {

	CG_BuildSpectatorString();
	if (!cg.demoPlayback && cg.scoresRequestTime + 2000 < cg.time) { //don't clear the scoreboard when watching a demo
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

extern menuDef_t *menuScoreboard;
void Menu_Reset();			// FIXME: add to right include file

static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", cgs.screenHeight * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", cgs.screenHeight * .30, 0);
}


static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5\n");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6\n");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/


/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/

//jk2pro stuff
typedef struct bitInfo_S {
	const char	*string;
} bitInfo_T;

static bitInfo_T strafeTweaks[] = {
	{ "Original style" },//0
	{ "Updated style" },//1
	{ "Cgaz style" },//2
	{ "Warsow style" },//3
	{ "Sound" },//4
	{ "W" },//5
	{ "WA" },//6
	{ "WD" },//7
	{ "A" },//8
	{ "D" },//9
	{ "Rear" },//10
	{ "Center" },//11
	{ "Accel bar" },//12
	{ "Weze style" },//13
	{ "Line Crosshair" }//13
};
static const int MAX_STRAFEHELPER_TWEAKS = ARRAY_LEN(strafeTweaks);

void CG_StrafeHelper_f(void) {
	if (trap_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_STRAFEHELPER_TWEAKS; i++) {
			if ((cg_strafeHelper.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, strafeTweaks[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, strafeTweaks[i].string);
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_STRAFEHELPER_TWEAKS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index < 0 || index >= MAX_STRAFEHELPER_TWEAKS) {
			Com_Printf("strafeHelper: Invalid range: %i [0, %i]\n", index, MAX_STRAFEHELPER_TWEAKS - 1);
			return;
		}

		if ((index == 0 || index == 1 || index == 2 || index == 3 || index == 13)) { //Radio button these options
																					 //Toggle index, and make sure everything else in this group (0,1,2,3,13) is turned off
			int groupMask = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 13);
			int value = cg_strafeHelper.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap_Cvar_Set("cg_strafeHelper", va("%i", value));
		}
		else {
			trap_Cvar_Set("cg_strafeHelper", va("%i", (1 << index) ^ (cg_strafeHelper.integer & mask)));
		}
		trap_Cvar_Update(&cg_strafeHelper);

		Com_Printf("%s %s^7\n", strafeTweaks[index].string, ((cg_strafeHelper.integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T speedometerSettings[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Enable speedometer" },//0
	{ "Pre-speed display" },//1
	{ "Jump height display" },//2
	{ "Jump distance display" },//3
	{ "Vertical speed indicator" },//4
	{ "Yaw speed indicator" },//5
	{ "Accel meter" },//6
	{ "Speed graph" },//7
	{ "Display speed in kilometers instead of units" },//8
	{ "Display speed in imperial miles instead of units" },//9
};
static const int MAX_SPEEDOMETER_SETTINGS = ARRAY_LEN(speedometerSettings);

void cg_speedometer_f(void)
{
	if (trap_Argc() == 1) {
		int i = 0, display = 0;

		for (i = 0; i < MAX_SPEEDOMETER_SETTINGS; i++) {
			if ((cg_speedometer.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", display, speedometerSettings[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", display, speedometerSettings[i].string);
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2;
		const uint32_t mask = (1 << MAX_SPEEDOMETER_SETTINGS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		if (index2 < 0 || index2 >= MAX_SPEEDOMETER_SETTINGS) {
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_SPEEDOMETER_SETTINGS - 1);
			return;
		}

		if (index == 8 || index == 9) { //Radio button these options
		//Toggle index, and make sure everything else in this group (8,9) is turned off
			int groupMask = (1 << 8) + (1 << 9);
			int value = cg_speedometer.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap_Cvar_Set("cg_speedometer", va("%i", value));
		}
		else {
			trap_Cvar_Set("cg_speedometer", va("%i", (1 << index) ^ (cg_speedometer.integer & mask)));
		}
		trap_Cvar_Update(&cg_speedometer);

		Com_Printf("%s %s^7\n", speedometerSettings[index2].string, ((cg_speedometer.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled"));
	}
}

void CG_SanitizeString2(const char *in, char *out)
{
	int i = 0, r = 0;

	while (in[i]) {
		if (i >= MAX_NAME_LENGTH - 1) {//the ui truncates the name here..
			break;
		}
		if (in[i] == '^') {
			if (in[i + 1] >= 48 && in[i + 1] <= 57) { //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else { //just skip the ^
				i++;
				continue;
			}
		}
		if (in[i] < 32) {
			i++;
			continue;
		}
		out[r] = tolower(in[i]);//lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}

int CG_ClientNumberFromString(const char *s)
{
	clientInfo_t *cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	idnum = atoi(s);

	if (s[0] == '-') { //- returns id of target in our crosshair
		idnum = CG_CrosshairPlayer();
		return idnum;
	}

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))  //changed and to or ..
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	CG_SanitizeString2(s, s2);
	for (idnum = 0, cl = cgs.clientinfo; idnum < cgs.maxclients; ++idnum, ++cl) {
		if (!cl->infoValid) {
			continue;
		}
		CG_SanitizeString2(cl->name, n2);

		for (i = 0; i < cgs.maxclients; i++)
		{
			cl = &cgs.clientinfo[i];
			if (!cl->infoValid) {
				continue;
			}
			CG_SanitizeString2(cl->name, n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ //found more than one match
					Com_Printf("More than one user '%s' on the server\n", s);
					return -2;
				}
				match = i;
			}
		}
		if (match != -1)//uhh
			return match;
	}
	if (!atoi(s)) //Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		Com_Printf("User '%s' is not on the server\n", s);
	return -1;
}

void CG_Say_f( void ) {
	char msg[MAX_SAY_TEXT] = {0};
	char word[MAX_SAY_TEXT] = {0};
	char numberStr[MAX_SAY_TEXT] = {0};
	int i, number = 0, numWords = trap_Argc();
	int clientNum = -1, messagetype = 0;

	if (!Q_stricmp(CG_Argv(0), "say")) {
		messagetype = 1;
	}
	else if (!Q_stricmp(CG_Argv(0), "say_team")) {
		messagetype = 2;
	}
	else if (!Q_stricmp(CG_Argv(0), "tell")) {
		clientNum = CG_ClientNumberFromString(CG_Argv(1));
		messagetype = 3;
		if (clientNum < 0) //couldn't find target or multiple matches found
			return;
	}
	else {//shouldn't happen...
		return;
	}

	for (i = 1; i < numWords; i++) {
		if (i == 1 && clientNum > -1) //skip 1st argument in PM since that's the name of the person we're trying to PM
			continue;
		trap_Argv(i, word, sizeof(word));

		if (!Q_stricmp(word, "%H%")) {
			number = cg.predictedPlayerState.stats[STAT_HEALTH];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%S%")) {
			number = cg.predictedPlayerState.stats[STAT_ARMOR];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%F%")) {
			number = cg.predictedPlayerState.fd.forcePower;
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%W%")) {
			number = cg.predictedPlayerState.weapon;
			switch (number) {
				case 1:	Com_sprintf(numberStr, sizeof(numberStr), "Stun baton"); break;
				case 2: Com_sprintf(numberStr, sizeof(numberStr), "Melee"); break;
				case 4:	Com_sprintf(numberStr, sizeof(numberStr), "Bryar"); break;
				case 5:	Com_sprintf(numberStr, sizeof(numberStr), "E11"); break;
				case 6:	Com_sprintf(numberStr, sizeof(numberStr), "Sniper"); break;
				case 7:	Com_sprintf(numberStr, sizeof(numberStr), "Bowcaster");	break;
				case 8:	Com_sprintf(numberStr, sizeof(numberStr), "Repeater"); break;
				case 9:	Com_sprintf(numberStr, sizeof(numberStr), "Demp2");	break;
				case 10: Com_sprintf(numberStr, sizeof(numberStr), "Flechette"); break;
				case 11: Com_sprintf(numberStr, sizeof(numberStr), "Rocket"); break;
				case 12: Com_sprintf(numberStr, sizeof(numberStr), "Thermal"); break;
				case 13: Com_sprintf(numberStr, sizeof(numberStr), "Tripmine"); break;
				case 14: Com_sprintf(numberStr, sizeof(numberStr), "Detpack"); break;
				default: Com_sprintf(numberStr, sizeof(numberStr), "Saber"); break;
			}
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%A%")) {
			number = cg.predictedPlayerState.ammo[weaponData[cg.predictedPlayerState.weapon].ammoIndex];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
#ifndef Q3_VM
		else if (!Q_stricmp(word, "%T%")) { //insert time in 12-hour format
			struct tm *newtime;
			qboolean AM = qtrue;
			time_t rawtime;
			time(&rawtime);
			newtime = localtime(&rawtime);
			if (newtime->tm_hour >= 12) AM = qfalse;
			if (newtime->tm_hour > 12) newtime->tm_hour -= 12;
			if (newtime->tm_hour == 0) newtime->tm_hour = 12;
			Com_sprintf(numberStr, sizeof(numberStr), "%i:%02i %s", newtime->tm_hour, newtime->tm_min, AM ? "AM" : "PM");
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%T2%")) { //insert time in 24-hour format
			struct tm *newtime;
			time_t rawtime;
			time(&rawtime);
			newtime = localtime(&rawtime);
			Com_sprintf(numberStr, sizeof(numberStr), "%02i:%02i", newtime->tm_hour, newtime->tm_min);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
#endif

		Q_strcat(word, MAX_SAY_TEXT, " ");
		Q_strcat(msg, MAX_SAY_TEXT, word);
	}

	switch (messagetype)
	{
		default:
			CG_Printf("%sUnrecognized command %s\n", S_COLOR_YELLOW, CG_Argv(0));
			break;
		case 1:
			trap_SendClientCommand(va("say %s", msg));
			break;
		case 2:
			trap_SendClientCommand(va("say_team %s", msg));
			break;
		case 3:
			if (clientNum > -1) trap_SendClientCommand(va("tell %i %s", clientNum, msg));
			break;
	}
}

void CG_ClientList_f( void )
{
	clientInfo_t *ci;
	int i;
	int count = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) 
	{
		ci = &cgs.clientinfo[ i ];
		if( !ci->infoValid ) 
			continue;

		switch( ci->team ) 
		{
		case TEAM_FREE:
			if (cgs.isCTFMod && cgs.CTF3ModeActive) {
				CG_Printf( "%2d " S_COLOR_YELLOW "Y   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					ci->name, (ci->botSkill != 0) ? " (bot)" : "" );
			}
			else {
				CG_Printf( "%2d " S_COLOR_YELLOW "F   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					ci->name, (ci->botSkill != 0) ? " (bot)" : "" );
			}
 			break;
 
		case TEAM_RED:
			CG_Printf( "%2d " S_COLOR_RED "R   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "" );
			break;

		case TEAM_BLUE:
			CG_Printf( "%2d " S_COLOR_BLUE "B   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "" );
			break;

		default:
		case TEAM_SPECTATOR:
			CG_Printf( "%2d " S_COLOR_YELLOW "S   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "" );
			break;
		}

		count++;
	}

	CG_Printf( "Listed %2d clients\n", count );
}

static void CG_ModVersion_f(void)
{
	CG_Printf("^5Your client version of the mod was compiled on %s at %s\n", __DATE__, __TIME__);
	//trap_SendConsoleCommand("ui_modversion\n");
	if (cgs.isJK2Pro) {
		trap_SendClientCommand( "modversion" );
		trap_Cvar_Set("cjp_client", "1.4JAPRO"); //Do this manually here i guess, just incase it does not do it when game is created due to ja+ or something
	}
}

static void CG_Follow_f(void) {
	int clientNum = -1;

	if (trap_Argc() < 2) {
		CG_Printf("usage /follow <name>\n");
		return;
	}

	clientNum = CG_ClientNumberFromString(CG_Argv(1));

	if (clientNum < 0)
		return;

	CG_SendConsoleCommand("cmd follow %i", clientNum);
}

static void CG_FollowRedFlag_f(void) {
	int i;
	clientInfo_t	*ci;

	if (!cg.snap)
		return;

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[ i ];

		if (ci->powerups & (1 << PW_REDFLAG)) {
			CG_SendConsoleCommand("cmd follow %i", i);
			return;
		}
	}
}

static void CG_FollowBlueFlag_f(void) {
	int i;
	clientInfo_t	*ci;

	if (!cg.snap)
		return;

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[ i ];

		if (ci->powerups & (1 << PW_BLUEFLAG)) {
			CG_SendConsoleCommand("follow %i", i);
			return;
		}
	}
}

static void CG_FollowYellowFlag_f(void) {
	int i;
	clientInfo_t	*ci;

	if (!cg.snap)
		return;

	if (!cgs.isCTFMod || !cgs.CTF3ModeActive)
		return;

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[ i ];

		if (ci->powerups & (1 << PW_NEUTRALFLAG)) {
			CG_SendConsoleCommand("cmd follow %i", i);
			return;
		}
	}
}


static void CG_FollowFastest_f(void) {
	int i, fastestPlayer = -1, currentSpeed, fastestSpeed = 0;
	centity_t *cent;

	if (!cg.snap)
		return;

	for (i=0;i<MAX_CLIENTS;i++) {
		if (i == cg.snap->ps.clientNum)
			continue;

		cent = &cg_entities[i];

		if (!cent)
			continue;
		if (cent->currentState.eType != ET_PLAYER)
			continue;

		currentSpeed = VectorLengthSquared(cent->currentState.pos.trDelta);

		if (currentSpeed > fastestSpeed) {
			fastestSpeed = currentSpeed;
			fastestPlayer = i;
		}

	}
	if (fastestPlayer >= 0 && fastestPlayer < MAX_CLIENTS)
		CG_SendConsoleCommand("cmd follow %i", fastestPlayer);
}

static void CG_RemapShader_f(void) {
	char oldShader[MAX_QPATH], newShader[MAX_QPATH];

	if (trap_Argc() != 3) {
		CG_Printf("Usage: /remapShader <old> <new>\n");
		return;
	}

	trap_Argv( 1, oldShader, sizeof( oldShader ) );
	trap_Argv( 2, newShader, sizeof( newShader ) );

	//validate this ?
	//how to stop from using trans shaders..?

	trap_R_RemapShader(oldShader, newShader, NULL);	 //what is timeoffset for

}

static void CG_ListRemaps_f(void) {
	const char	*info;
	char info2[MAX_CONFIGSTRINGS];
	info = CG_ConfigString( CS_SHADERSTATE );

	Q_strncpyz( info2, info, sizeof(info2));

	Q_strstrip( info2, ":", "\n" );	

	CG_Printf("Remaps: \n %s \n", info2);

	//Replace : with newline
	//replace 0.30@ with null?
	//replace = with " -> "

	//keep track of local remaps somehow
	//either directly or add remap text to array when added, list here

}


void CG_Do_f(void) //loda fixme
{
	char vstr[MAX_QPATH], delay[32];
	int delayMS;

	if (trap_Argc() != 3) {
		Com_Printf("Usage: /do <vstr> <delay>\n");
		return;
	}

	trap_Argv(1, vstr, sizeof(vstr));
	trap_Argv(2, delay, sizeof(delay));

	delayMS = atoi(delay);
	if (delayMS < 0)
		delayMS = 0;
	else if (delayMS > 1000 * 60 * 60)
		delayMS = 1000 * 60 * 60;

	Com_sprintf(cg.doVstr, sizeof(cg.doVstr), "vstr %s\n", vstr);
	cg.doVstrTime = cg.time + delayMS;
}

static void CG_Flipkick_f(void)
{
	//Well we always want to do the first kick, unless we are doing some really advanced predictive shit..

	//Ok, we started out flipkick.  Each frame we want to remove/add jump (+moveup and -moveup).

	cg.numFKFrames = 1;


	//How to make the perfect KS?
	//Get frametime or com_maxfps ?
	//Do however many taps super fast until they are at max jump kick height?
	//trap_SendConsoleCommand("+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup\n");
}

static void CG_Lowjump_f(void)
{
	trap_SendConsoleCommand("+moveup\n");
	Q_strncpyz(cg.doVstr, "-moveup\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollDown_f(void)
{
	if (trap_Key_IsDown(trap_Key_GetKey("+moveup"))) {
		trap_SendConsoleCommand("-moveup\n");
	}

	if (cg.predictedPlayerState.weapon != WP_SABER || cg.predictedPlayerState.powerups[PW_YSALAMIRI] > cg.time)
	{
		trap_SendConsoleCommand("+movedown\n");
		return;
	}

	trap_SendConsoleCommand("+speed\n");
	Q_strncpyz(cg.doVstr, "-moveup;+movedown;-speed\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollUp_f(void)
{
	if (cg.predictedPlayerState.weapon != WP_SABER || cg.predictedPlayerState.powerups[PW_YSALAMIRI] > cg.time)
	{
		trap_SendConsoleCommand("-movedown\n");
		return;
	}

	Q_strncpyz(cg.doVstr, "-movedown;-speed\n", sizeof(cg.doVstr)); //?
	cg.doVstrTime = cg.time;
}

qboolean CG_WeaponSelectable(int i);
void CG_LastWeapon_f(void) //loda fixme. japro
{
	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW)
		return;

	if (!cg.lastWeaponSelect[0])
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon;
	if (!cg.lastWeaponSelect[1])
		cg.lastWeaponSelect[1] = cg.predictedPlayerState.weapon;

	if (cg.lastWeaponSelect[1] == cg.lastWeaponSelect[0]) { //if the weapon we spawned with is still equipped
		int i;
		for (i = LAST_USEABLE_WEAPON; i > 0; i--) {	//cycle to the next available one
			if ((i != cg.weaponSelect) && CG_WeaponSelectable(i)) {
				cg.lastWeaponSelect[1] = i;
				break;
			}
		}
	}

	if (cg.lastWeaponSelect[0] != cg.weaponSelect) { //Current does not match selected
		cg.lastWeaponSelect[1] = cg.lastWeaponSelect[0]; //Set last to current
		cg.lastWeaponSelect[0] = cg.weaponSelect; //Set current to selected
	}

	cg.weaponSelect = cg.lastWeaponSelect[1]; //Set selected to last

	cg.weaponSelectTime = cg.time;
	if (cg.weaponSelect != cg.lastWeaponSelect[1])
		trap_S_MuteSound(cg.predictedPlayerState.clientNum, CHAN_WEAPON);
}


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },

	//jk2pro stuff
	{ "strafeHelper", CG_StrafeHelper_f },
	{ "speedometer", cg_speedometer_f },

	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },

	{ "say", CG_Say_f },
	{ "say_team", CG_Say_f },
	{ "tell", CG_Say_f },

	{ "clientlist", CG_ClientList_f },

	{ "modversion", CG_ModVersion_f },

	{ "follow", CG_Follow_f },
	{ "followRedFlag", CG_FollowRedFlag_f },
	{ "followBlueFlag", CG_FollowBlueFlag_f },
	{ "followYellowFlag", CG_FollowYellowFlag_f },
	{ "followFastest", CG_FollowFastest_f },

	{ "remapShader", CG_RemapShader_f },
	{ "listRemaps", CG_ListRemaps_f },

	{ "do", CG_Do_f },
	{ "flipkick", CG_Flipkick_f },
	{ "lowjump", CG_Lowjump_f },
	{ "+duck", CG_NorollDown_f },
	{ "-duck", CG_NorollUp_f },
	{ "weaplast", CG_LastWeapon_f }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	size_t		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	size_t		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("forcechanged");
	trap_AddCommand ("sv_invnext");
	trap_AddCommand ("sv_invprev");
	trap_AddCommand ("sv_forcenext");
	trap_AddCommand ("sv_forceprev");
	trap_AddCommand ("sv_saberswitch");
	trap_AddCommand ("engage_duel");
	trap_AddCommand ("force_heal");
	trap_AddCommand ("force_speed");
	trap_AddCommand ("force_throw");
	trap_AddCommand ("force_pull");
	trap_AddCommand ("force_distract");
	trap_AddCommand ("force_rage");
	trap_AddCommand ("force_protect");
	trap_AddCommand ("force_absorb");
	trap_AddCommand ("force_healother");
	trap_AddCommand ("force_forcepowerother");
	trap_AddCommand ("force_seeing");
	trap_AddCommand ("use_seeker");
	trap_AddCommand ("use_field");
	trap_AddCommand ("use_bacta");
	trap_AddCommand ("use_electrobinoculars");
	trap_AddCommand ("zoom");
	trap_AddCommand ("use_sentry");
	trap_AddCommand ("bot_order");
	trap_AddCommand ("saberAttackCycle");
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo

	//generic mod server cmds
	trap_AddCommand("help");
	trap_AddCommand("ignore");
	trap_AddCommand("aminfo");
	trap_AddCommand("amempower"); //idk if this is somewhere other than twimod
	trap_AddCommand("ammerc"); //probably exists somewhere if amempower does

	trap_AddCommand("engage_gunduel"); //?
	trap_AddCommand("engage_fullforceduel"); //??

	//ctfmod server cmds
	trap_AddCommand("afk");
	trap_AddCommand("altf");
	trap_AddCommand("ignoreclear");
	trap_AddCommand("ignorelist");
	trap_AddCommand("specs");

	//ctfmod rcon cmds for autocomplete
	trap_AddCommand("amkick");
	trap_AddCommand("amstatus");
	trap_AddCommand("cp");
	trap_AddCommand("cvars");
	trap_AddCommand("forceteam");
	trap_AddCommand("lockname");
	trap_AddCommand("lockteam");
	trap_AddCommand("mute");
	trap_AddCommand("pause");
	trap_AddCommand("poll");
	trap_AddCommand("swapteams");
	trap_AddCommand("unpause");

	//TwiMod cmds
	trap_AddCommand("ammodinfo");
	trap_AddCommand("ammodinfo_twitch");
	trap_AddCommand("amadmin");

	trap_AddCommand("channel");
	trap_AddCommand("channellist");

	trap_AddCommand("mute");

	trap_AddCommand("engage_ff"); //oh ok..

	trap_AddCommand("engage_private");
	trap_AddCommand("invite_private");
	trap_AddCommand("accept_private");
	trap_AddCommand("end_private");

	//TwiMod build cmds
	trap_AddCommand("placemodel");
	trap_AddCommand("drop");
	trap_AddCommand("pick");
	trap_AddCommand("remove");

	//TwiMod emotes
	trap_AddCommand("ambar");
	trap_AddCommand("ambeg");
	trap_AddCommand("amcomeon");
	trap_AddCommand("amflip");
	trap_AddCommand("amlaugh");
	trap_AddCommand("amnod");
	trap_AddCommand("amshake");
	trap_AddCommand("amsuper");
	trap_AddCommand("amsuper2");
	trap_AddCommand("amspin");
	trap_AddCommand("amspin2");
	trap_AddCommand("amspin3");
	trap_AddCommand("amspinr");
	trap_AddCommand("amspin2r");
	trap_AddCommand("amspin3r");
	trap_AddCommand("amthumbsdown");
	trap_AddCommand("amthumbsup");
	trap_AddCommand("amtossover");
	trap_AddCommand("amtossup");
	trap_AddCommand("amvictory");
	trap_AddCommand("amwave2");
	trap_AddCommand("amdontkillme");
	trap_AddCommand("amfakedead");
	trap_AddCommand("amkneel");
	trap_AddCommand("amsit");
	trap_AddCommand("amsit2");
	trap_AddCommand("amsit3");
	trap_AddCommand("amthreaten");
	trap_AddCommand("amtype");
	trap_AddCommand("amtype2");
	trap_AddCommand("amwrite");
	trap_AddCommand("amwrite2");
	trap_AddCommand("amcowboy");
	trap_AddCommand("amhandhips");
	trap_AddCommand("amsurrender");
	trap_AddCommand("amwait");

	trap_AddCommand("amtaunt");
	trap_AddCommand("amtaunt2");
	
	trap_AddCommand("taunt2");

	//these probably exist smehwere..
	trap_AddCommand("amhug");
	trap_AddCommand("amkiss");
}
