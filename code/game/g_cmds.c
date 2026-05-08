// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "g_defrag.h"
#include "g_dbcmds.h"
#include "../qcommon/crypt_blowfish.h"

#include "../ui/menudef.h"			// for the voice chats
#include "../qcommon/levenshtein.h"

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void BG_CycleInven(playerState_t *ps, int direction);
void BG_CycleForce(playerState_t *ps, int direction);

extern void DF_SetSubContestDefaults(gclient_t* client);

static void Cmd_PrintCmdDescriptions_f(gentity_t* ent);

static qboolean DefragDoubleTapSafety(gentity_t* ent, doubleTapType_t type, const char* cmd ) {
	if (!g_defragKillSafetyMinSecs.integer) {
		return qtrue;
	}
	if (!ent->client->sess.raceMode || !ent->client->pers.raceStartCommandTime || (ent->client->ps.commandTime-clampedIntMult(1000, g_defragKillSafetyMinSecs.integer)) < ent->client->pers.raceStartCommandTime || ent->client->ps.commandTime < ent->client->pers.raceStartCommandTime) {
		// nothing to worry about
		memset(&ent->client->pers.doubleTap, 0, sizeof(ent->client->pers.doubleTap));
		return qtrue;
	}
	if (ent->client->pers.doubleTap.lastType == type && (level.time - 500) < ent->client->pers.doubleTap.lastTime && ent->client->pers.doubleTap.lastTime < level.time) {
		memset(&ent->client->pers.doubleTap, 0, sizeof(ent->client->pers.doubleTap));
		return qtrue;
	}
	G_CenterPrint(ent - g_entities, 3, va("^3You are more than ^1%d ^3seconds into a run. Please double-tap your %s bind to confirm intent.", g_defragKillSafetyMinSecs.integer, cmd),qfalse,qtrue,qfalse,NULL);
	ent->client->pers.doubleTap.lastType = type;
	ent->client->pers.doubleTap.lastTime = level.time;
	return qfalse;
}

/*
========================
SendTFFAEndGameStats

Stats screen at the end of a game for team ffa
========================
*/
void TFFAEndGameStatsMessage(int recipient, qboolean final) {
	// scoreboard design mostly taken from Alereon's 2v2 mod, code is my own (TA)
	int team;
	int i, numSorted;
	const char* teamname;
	gclient_t* cl;
	char playerName[MAX_NETNAME * 2];
	qboolean v102colors = (qboolean)(jk2gameplay == VERSION_1_02 || jk2gameplay == VERSION_1_03);
	tffaStats_t*	plStats;
	tffaStats_t		teamStats;
	int				teamScoreSum;
	if (g_gametype.integer != 5 || recipient < -1 || recipient >= level.maxclients) {
		return;
	}
	numSorted = level.numConnectedClients;

	trap_SendServerCommand(recipient, va("print \""
		"%s TFFA game stats\n"
		"\" tffaStats_%s", final ? "^6Final" : "^3Preliminary", final ? "final" : "prelim")
	);
	for (team = TEAM_RED; team <= TEAM_BLUE; team++) {
		memset(&teamStats, 0, sizeof(teamStats));
		teamScoreSum = 0;
		teamname = (team == TEAM_RED) ? "RED" : "BLUE";
		trap_SendServerCommand(recipient, "print \""
			"^6TEAM Player                    Kills Deaths Suicides Team Kills    Dmg Given    Dmg Received   NET Dmg  Team Dmg Score\n"
			"^7----------------------------------------------------------------------------------------------------------------------\n"
			"\""
		);
		for (i = 0; i < numSorted; i++) {
			cl = &level.clients[level.sortedClients[i]];
			if (cl->sess.sessionTeam != team) {
				continue;
			}
			// is jk2gameplay correct here?
			Q_PrintStrCopy(cl->pers.netname, playerName, sizeof(playerName), 23, v102colors, v102colors); // maybe ntcolors shouldn't ever be on by default? meh. gotta decide which client we wanna be more compatible with? let's just allow it if 102 colors are allowed. would be allowed anyway then i guess. also, 23 is the limit here cuz it's actually gonna be printed with up to 24 letters. we wanna keep one empty space so we can cleanly clear the color after.
			plStats = &cl->pers.tffaStats;
			trap_SendServerCommand(recipient, va("print \""
				"^7%-4s %-23s ^7 %6d %6d %8d %10d %12d %15d %9d %9d %5d\n"
				"\" tffaStats_playerv2 %d %d %d %d %d %d %d %d %d %d %d %d %d %d", teamname, playerName, plStats->kills, plStats->deaths, plStats->suicides, plStats->teamkills, plStats->dmgReal.norm.damageDealt, plStats->dmgReal.norm.damageReceived, plStats->dmgReal.norm.damageDealt - plStats->dmgReal.norm.damageReceived, plStats->dmgReal.team.damageDealt, cl->ps.persistant[PERS_SCORE],
				// post-printtext data, clientnum plus all stats duplicated (for easy parsing)
				(int)(cl - g_clients), plStats->kills, plStats->deaths, plStats->suicides, plStats->teamkills, plStats->dmgReal.norm.damageDealt, plStats->dmgReal.norm.damageReceived, plStats->dmgReal.team.damageDealt, plStats->dmgReal.team.damageReceived,plStats->dmg.norm.damageDealt, plStats->dmg.norm.damageReceived, plStats->dmg.team.damageDealt, plStats->dmg.team.damageReceived, cl->ps.persistant[PERS_SCORE]
			));
			// plStats->dmg.norm.damageDealt, plStats->dmg.norm.damageReceived, plStats->dmg.team.damageDealt, plStats->dmg.team.damageReceived
			teamStats.kills += plStats->kills;
			teamStats.deaths += plStats->deaths;
			teamStats.suicides += plStats->suicides;
			teamStats.teamkills += plStats->teamkills;
			teamStats.dmgReal.norm.damageDealt += plStats->dmgReal.norm.damageDealt;
			teamStats.dmgReal.norm.damageReceived += plStats->dmgReal.norm.damageReceived;
			teamStats.dmgReal.team.damageDealt += plStats->dmgReal.team.damageDealt;
			// these are just for the post-print info: team damage taken and also the raw damage values for all things. we can parse it if we want, but it's not printed
			teamStats.dmgReal.team.damageReceived += plStats->dmgReal.team.damageReceived;
			teamStats.dmg.norm.damageDealt += plStats->dmg.norm.damageDealt;
			teamStats.dmg.norm.damageReceived += plStats->dmg.norm.damageReceived;
			teamStats.dmg.team.damageDealt += plStats->dmg.team.damageDealt;
			teamStats.dmg.team.damageReceived += plStats->dmg.team.damageReceived;
			teamScoreSum += cl->ps.persistant[PERS_SCORE];

		}
		trap_SendServerCommand(recipient, va("print \""
			"^7----------------------------------------------------------------------------------------------------------------------\n"
			"^7%-4s Totals                   %6d %6d %8d %10d %12d %15d %9d %9d %5d\n\n\n"
			"\" tffaStats_teamv2 %d %d %d %d %d %d %d %d %d %d %d %d %d %d", teamname, teamStats.kills, teamStats.deaths, teamStats.suicides, teamStats.teamkills, teamStats.dmgReal.norm.damageDealt, teamStats.dmgReal.norm.damageReceived, teamStats.dmgReal.norm.damageDealt - teamStats.dmgReal.norm.damageReceived, teamStats.dmgReal.team.damageDealt, teamScoreSum,
			// post-printtext data, team id plus all stats duplicated (for easy parsing)
				team, teamStats.kills, teamStats.deaths, teamStats.suicides, teamStats.teamkills, teamStats.dmgReal.norm.damageDealt, teamStats.dmgReal.norm.damageReceived, teamStats.dmgReal.team.damageDealt, teamStats.dmgReal.team.damageReceived, teamStats.dmg.norm.damageDealt, teamStats.dmg.norm.damageReceived, teamStats.dmg.team.damageDealt, teamStats.dmg.team.damageReceived, teamScoreSum
			));

	}

	trap_SendServerCommand(recipient, "print \"\" tffaStats_end");
}


#define TEAMSTATS_CMD		"tstats"
static void G_SendTeamStats(gentity_t* ent) {
	/*
		format for 1 team: (6 stat fields)
		%d %d %d %d %d %d
		rets, defense, assist, flaghold, flaggrabs, frags, score
	*/
	char msg[1024] = { 0 };
	int i;
	char* pch = &msg[0];


	for (i = 0; i < 2; ++i) {
		teamStats_t* ts = &level.teamstats[i];
		const char* s;

		s = va("%d %d %d %d %d %d %d ", ts->rets, ts->defense, ts->assist, ts->flaghold, ts->flaggrabs, ts->frags, ts->score);
		pch = mystrcat(pch, sizeof(msg), s);
	}

	pch = va(TEAMSTATS_CMD" %s", msg);

	trap_SendServerCommand(ent - g_entities, pch);
}



/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	Com_sprintf( string, sizeof(string), "scores %i %i %i", level.numConnectedClients, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	stringlength = strlen( string );

	for (i=0 ; i < numSorted ; i++) {
		int		ping, normalFollowerPing;
		int		time, fulltime;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
			normalFollowerPing = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			normalFollowerPing = cl->pers.normalFollowerPing < 999 ? cl->pers.normalFollowerPing : 999;
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		time = (level.time - cl->pers.enterTime) / 60000;
		fulltime = (level.time - cl->sess.firstEnterTime) / 60000;

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i%s %i%s %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], normalFollowerPing, normalFollowerPing == ping ? "" : miniva("/%i",ping), time, time==fulltime ? "" : miniva("/%i", fulltime),
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		Q_strncpyz (string + stringlength, entry,sizeof(string)-stringlength);
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, string );
	if (g_gametype.integer == GT_CTF && ent->client->pers.teamInfo == 2)
		G_SendTeamStats(ent);

	ent->client->lastScoresMessage = level.time;
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}

/*
==================
Cmd_TFFAStats_f

Request current TFFA stats information
==================
*/
void Cmd_TFFAStats_f( gentity_t *ent ) {
	TFFAEndGameStatsMessage( ent - g_entities, qfalse );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
ConcatArgsQuoted
==================
*/
char* ConcatArgsQuoted(int start) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for (i = start; i < c; i++) {
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if (len + tlen + 2 >= MAX_STRING_CHARS - 1) {
			break;
		}
		*(line + len) = '"';
		memcpy(line + len + 1, arg, tlen);
		len += tlen + 1;
		*(line + len) = '"';
		len += 1;
		if (i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		name[MAX_TOKEN_CHARS];
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];

	if ( !CheatsOk( ent ) ) {
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all)
	{
		i = 0;
		while (i < HI_NUM_HOLDABLE)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			i++;
		}
		i = 0;
	}

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (WP_DET_PACK+1))  - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}
	
	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi(arg));
		return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		G_SetClassName(it_ent, it->classname);
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	G_SendServerCommand( ent-g_entities, va("print \"%s\"", msg),qtrue);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	G_SendServerCommand( ent-g_entities, va("print \"%s\"", msg),qtrue);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
extern void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if (!DefragDoubleTapSafety(ent, DOUBLETAP_NOCLIP, "noclip")) {
		return;
	}

	if (g_defrag.integer && ent->client->sess.raceMode) {
		DF_RaceStateInvalidated(ent, qtrue);
		if (ent->health <= 0) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
			return;
		}
	}
	else if ( !CheatsOk( ent ) ) {
		return;
	}
		

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	G_SendServerCommand( ent-g_entities, va("print \"%s\"", msg),qtrue);
}

/*
==================
Cmd_Savepos_f

argv(0) savepos
==================
*/
qboolean SavePosition(gentity_t* client, savedPosition_t* savedPosition);
void Cmd_Savepos_f( gentity_t *ent ) {
	char	*msg;

	if (ent->client->noclip) {
		trap_SendServerCommand(ent - g_entities, "print \"Can't save position during noclip.\n\"");
		return;
	}

	if (g_defrag.integer && ent->client->sess.raceMode) {
		if (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) { // segmented restore/save pos handled elsewhere
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
				ent->client->pers.segmented.savePos = qtrue;
			}
			return;
		}
		//DF_RaceStateInvalidated(ent, qtrue);
		if (ent->health <= 0) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
			return;
		}
	}
	else if ( !CheatsOk( ent ) ) {
		return;
	}


	//VectorCopy(ent->client->ps.origin,ent->client->pers.savePosPosition);
	//VectorCopy(ent->client->ps.velocity,ent->client->pers.savePosVelocity);
	//VectorCopy(ent->client->ps.viewangles,ent->client->pers.savePosAngle);
	//ent->client->pers.savePosPlayerState = ent->client->ps;
	//ent->client->pers.savePosRaceStyle = ent->client->sess.raceStyle;
	if (SavePosition(ent, &ent->client->pers.savedPosition)) {
		ent->client->pers.savedPosition.ps.clientNum = ent-g_entities; // in case we do savepos from spec (yes its allowed because respos invalidates race state anyway)
		ent->client->pers.savedPosition.ps.pm_flags &= ~PMF_FOLLOW; // in case we do savepos from follow (yes its allowed because respos invalidates race state anyway)
		ent->client->pers.savedPosition.raceStartCommandTime = ent->client->pers.savedPosition.ps.duelTime = ent->client->pers.savedPosition.ps.duelInProgress = ent->client->pers.savedPosition.ps.duelIndex = 0;
		ent->client->pers.savePosUsed = qtrue;
	}
	msg = "Position, velocity and angle saved.\n";

	G_SendServerCommand( ent-g_entities, va("print \"%s\"", msg),qtrue);
}

/*
==================
Cmd_Respos_f

argv(0) respos
==================
*/
void RestorePosition(gentity_t* client, savedPosition_t* savedPosition, veci_t* diffAccum);
void Cmd_Respos_f( gentity_t *ent ) {
	char	*msg;
	if (ent->client->noclip) {
		trap_SendServerCommand(ent - g_entities, "print \"Can't restore position during noclip.\n\"");
		return;
	}

	if (g_defrag.integer && ent->client->sess.raceMode) {
		if (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) { // segmented restore/save pos handled elsewhere
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {

				qboolean segmentedDiscard = qfalse;
				ent->client->pers.segmented.respos = RESPOS_NORMAL;
				// try to discard one savepos position
				if (trap_Argc() > 1) {
					char arg[10];
					trap_Argv(1, arg, sizeof(arg));
					if (!Q_stricmp(arg, "discard")) {
						ent->client->pers.segmented.respos = RESPOS_DISCARD;
					}
				}
			}
			return;
		}
		if (ent->health <= 0) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
			return;
		}
	}
	else if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->pers.savePosUsed ) {
		int tmpScore;
		//VectorCopy(ent->client->pers.savePosPosition, ent->client->ps.origin);
		//VectorCopy(ent->client->pers.savePosVelocity, ent->client->ps.velocity);
		//SetClientViewAngle(ent,ent->client->pers.savePosAngle);
		//ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

		if ((ent->client->sess.sessionTeam == TEAM_SPECTATOR) != (ent->client->pers.savedPosition.ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)) {
			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
				trap_SendServerCommand(ent - g_entities, "print \"Your saved position is not a spectator position and you are in spec.\n\"");
			}
			else {
				trap_SendServerCommand(ent - g_entities, "print \"Your saved position is a spectator position and you are not in spec.\n\"");
			}
			return;
		}

		if (!DefragDoubleTapSafety(ent, DOUBLETAP_RESPOS, "respos")) {
			return;
		}

		RestorePosition(ent,&ent->client->pers.savedPosition,NULL);
		tmpScore = ent->client->pers.stats.score;
		DF_RaceStateInvalidated(ent, qtrue);
		ent->client->pers.stats.score = tmpScore; // so we can continue testing map that kills u with wrong score. hope this is safe but it gets nulled when you start a new run anyway?
	}
	else {
		msg = "Cannot restore position, velocity and angle. None saved.\n";
		G_SendServerCommand(ent - g_entities, va("print \"%s\"", msg),qtrue);
	}

}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}
	if (!DefragDoubleTapSafety(ent,DOUBLETAP_KILL,"kill")) {
		return;
	}

	if (g_pauseGame.integer && !ent->client->sess.raceMode) {
		G_SendServerCommand(ent - g_entities, va("print \"Can't kill yourself while game is paused :----D\n\""), qtrue);
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if ( client->sess.sessionTeam == TEAM_RED ) {
		G_CenterPrintOrPrint( -1, 3, va("%s" S_COLOR_WHITE " %s",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEREDTEAM")), qtrue, qfalse,qtrue, NULL, g_printJoins.integer == 1, g_printJoins.integer == 0);
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		G_CenterPrintOrPrint( -1, 3, va("%s" S_COLOR_WHITE " %s",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBLUETEAM")), qtrue, qfalse,qtrue, NULL, g_printJoins.integer == 1, g_printJoins.integer == 0);
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		G_CenterPrintOrPrint( -1, 3, va("%s" S_COLOR_WHITE " %s",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHESPECTATORS")), qtrue, qfalse,qtrue, NULL, g_printJoins.integer == 1, g_printJoins.integer == 0);
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (g_gametype.integer == GT_TOURNAMENT)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				G_CenterPrint( -1, 3, va("%s" S_COLOR_WHITE " %s %s",
				currentWinner->client->pers.netname, G_GetStripEdString("SVINGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				G_CenterPrintOrPrint( -1, 3, va("%s" S_COLOR_WHITE " %s",
				client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")), g_printJoins.integer == 1, g_printJoins.integer == 0);
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			G_CenterPrintOrPrint( -1, 3, va("%s" S_COLOR_WHITE " %s",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")), qtrue, qfalse,qtrue, NULL, g_printJoins.integer == 1, g_printJoins.integer == 0);
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  (int)(client - &level.clients[0]),
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}

void G_ResetClientVote(gclient_t* client) {
	if ((client->ps.eFlags & EF_VOTED) && level.voteTime) { // reset his vote
		if (client->pers.voteValue) {
			level.voteYes--;
			trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
		}
		else {
			level.voteNo--;
			trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
		}
		client->ps.eFlags &= ~EF_VOTED;
	}
}

/*
=================
SetTeam
=================
*/
int setTeamDepth = 0;
qboolean SetTeam( gentity_t *ent, const char *s ) {
	team_t				team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;
	int					i;

	setTeamDepth++; // kinda for debugging tbh but the whole setteam logic with all all the intertwined calls of ClientSpawn, modes, etc, can be a bit hard to grasp.
	if (setTeamDepth > 1) {
		Com_Printf("^1SetTeam: ^3Recursion level of %d. client %d\n",setTeamDepth-1,(int)(ent-g_entities));
	} else if(setTeamDepth > 10) {
		trap_SendServerCommand(-1,"print \"^1SetTeam: ^3Possible infinite recursion stopped.\n\"");
		return qfalse;
	}

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
				if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					team = TEAM_BLUE;
				}
				else
				{
					team = TEAM_RED;
				}
			}
			else
			{
			*/
			if (g_defrag.integer)
				team = TEAM_FREE;
			else
				team = PickTeam(clientNum);
			//}
		}

		if ( g_teamForceBalance.integer && !g_trueJedi.integer ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYRED")) );
				}

				setTeamDepth--;
				return qfalse; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYBLUE")) );
				}
				setTeamDepth--;
				return qfalse; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
			if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBELIGHT")) );
				setTeamDepth--;
				return;
			}
			if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEDARK")) );
				setTeamDepth--;
				return;
			}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_TOURNAMENT)
		&& level.numNonSpectatorClients >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	// some mode teams enforce specific real teams
	team = ValidateClientModeTeam(ent,team);
	if (team != TEAM_SPECTATOR) {
		specState = SPECTATOR_NOT;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		setTeamDepth--;
		return qfalse;
	}

	//
	// execute the team change
	//

	// unfollow any spectators
	if (team == TEAM_SPECTATOR) {
		StopFollowingClient(ent);
	}

	G_ResetClientVote(client);

	DF_RaceStateInvalidated(ent, qfalse);

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		if ( (g_gametype.integer != GT_TOURNAMENT) || (oldTeam != TEAM_SPECTATOR) )	{//so you don't get dropped to the bottom of the queue for changing skins, etc.
			gclient_t *otherClient;
			int i;
			for ( i = 0; i < level.maxclients; i++ ) {
				otherClient = &g_clients[i];
				if ( otherClient == client )
					otherClient->sess.spectatorOrder = 0;
				else if ( otherClient->pers.connected >= CON_CONNECTING )
					otherClient->sess.spectatorOrder++;
			}
		}
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );
	
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) ); // Ensure following spectators don't take flags or such into ClientBegin and trigger the FlagEatingFix (this allows us to check for powerups in the playerState to prevent flagEating when calling ClientBegin)
	ClientBegin( clientNum, qfalse );

	setTeamDepth--;
	return team != oldTeam;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW && ent->client->sess.spectatorClient >= 0 && ent->client->sess.spectatorClient < MAX_CLIENTS) {
		gentity_t* followed = g_entities + ent->client->sess.spectatorClient;
		if (followed->client && followed != ent && followed->client->sess.raceMode && (followed->client->sess.raceStyle.runFlags & RFL_BOT)) {
			ent->client->ps.viewangles[ROLL] = 0; // in case we were following a strafebotter. so we don't get stuck with a weird angled view
			if (!ent->client->sess.raceMode || !(ent->client->sess.raceStyle.runFlags & RFL_BOT)) {
				ent->client->sess.rollAngleInvalidated = qtrue; // does this make sense? idk. yep. cuz my dumbass sent a bad roll cmd even when just spectating someone who's botting
			}
		}
	}
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;

	// random fixes from TaystJK:
	ent->client->ps.emplacedIndex = 0; // Reset emplacedIndex when stopping following just in case. (ensiform)
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = qfalse; // [MP] Misc fixes and cleanup. (ensiform)
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.saberMove = LS_NONE; // [MP] Fix JACoders#423: spectator bug after special move (xycaleth)
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
	ent->client->ps.isJediMaster = qfalse; // major exploit if you are spectating somebody and they are JM and you reconnect (mrwonko)
	ent->client->ps.bobCycle = 0; // (mrwonko)
	ent->client->ps.pm_type = PM_SPECTATOR;
	ent->client->ps.eFlags &= ~EF_DISINTEGRATION;
	memset(ent->client->ps.powerups,0,sizeof(ent->client->ps.powerups)); // (mrwonko) // TA: made this a memset instead of a loop

	//ent->client->ps.viewangles[ROLL] = 0; // could this fix weirdness with strafebot? idk
	SetClientViewAngle(ent, ent->client->ps.viewangles); //Fix viewangles getting fucked up when we stop spectating someone?

	// don't use dead view angles (from vvv-serverside, which quotes ioq3, dunno if we need this)
	if (ent->client->ps.stats[STAT_HEALTH] <= 0) {
		ent->client->ps.stats[STAT_HEALTH] = 1;
	}

	UpdateClientRaceVars(ent->client); // make sure our race stuff is updated, especially RFL_BOT runflag.
}

// make anyone who's following this client stop doing so
void StopFollowingClient( gentity_t* ent ) {
	int i;
	gentity_t* other = g_entities;
	for (i = 0; i < level.maxclients; i++, other++) {
		if (other->inuse && other->client && other->client->pers.connected == CON_CONNECTED && other->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.spectatorState == SPECTATOR_FOLLOW && other->client->sess.spectatorClient == (ent-g_entities)) {
			StopFollowing(other);
		}
	}
}

qboolean SlowVotingActive(gentity_t* ent) {
	return g_slowVote.integer && ent->client->sess.raceMode || g_slowVote.integer > 1;
}

helpTip_t helpTips[] = {
	{
		"print \"\n^7Various commands:\n\"",
		"print \"Random tip: \n^7Various commands:\n\"",
		qtrue,
		qfalse
	},
	{
		"print \"^2/pickmode^7 - Pick a game mode from: normal, defrag, duel, duelqueue, allforce, ironman (^2/duel^7,^2/allforce^7 and ^2/ironman^7 are their own commands too)\n\"",
		"print \"Random tip: ^2/pickmode^7 - Pick a game mode from: normal, defrag, duel, duelqueue, allforce, ironman (^2/duel^7,^2/allforce^7 and ^2/ironman^7 are their own commands too)\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/players^7 - See info about players including client num and game mode\n\"",
		"print \"Random tip: ^2/players^7 - See info about players including client num and game mode\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/afk^7 - See who's afk and for how long\n\"",
		"print \"Random tip: ^2/afk^7 - See who's afk and for how long\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/stay^7 - Stay on this map. Prevents others from voting for another map while you are ingame and not AFK.\n\"",
		"print \"Random tip: ^2/stay^7 - Stay on this map. Prevents others from voting for another map while you are ingame and not AFK.\n\"",
		qfalse,
		qfalse,
		SlowVotingActive
	},
	{
		"print \"^2/say_cross^7 - Like (^2/say^7) but your chat is broadcasted across all connected servers (bind ^7/messagemode6^7 in new TommyTernal clients for comfortable writing).\n\"",
		"print \"Random tip: ^2/say_cross^7 - Like (^2/say^7) but your chat is broadcasted across all connected servers (bind ^7/messagemode6^7 in new TommyTernal clients for comfortable writing).\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"\n^7Map commands:\n\"",
		"print \"Random tip: \n^7Map commands:\n\"",
		qtrue,
		qfalse
	},
	{
		"print \"^2/maplist^7 - Call to see list of maps you can callvote. Optional: ^2/maplist unplayed\n\"",
		"print \"Random tip: ^2/maplist^7 - Call to see list of maps you can callvote. Optional: ^2/maplist unplayed\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/longest^7,^2/shortest^7,^2/toprated^7,^2/mostplayed^7,^2/hardest^7,^2/easiest^7 - Show longest/shortest/popular(by rating)/popular(by amount of runs)/hardest/easiest maps. Can call with movement style and page.\n\"",
		"print \"Random tip: ^2/longest^7,^2/shortest^7,^2/toprated^7,^2/mostplayed^7,^2/hardest^7,^2/easiest^7 - Show longest/shortest/popular(by rating)/popular(by amount of runs)/hardest/easiest maps. Can call with movement style and page.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/ratemap^7 - Rate the current map from 0 to 10. Call with movement style and number.\n\"",
		"print \"Random tip: ^2/ratemap^7 - Rate the current map from 0 to 10. Call with movement style and number.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/wrs^7,^2/notwr^7 - Show maps you hold/don't hold WR on, sorted by your current rank, highest first. Can call with username.\n\"",
		"print \"Random tip: ^2/wrs^7,^2/notwr^7 - Show maps you hold/don't hold WR on, sorted by your current rank, highest first. Can call with username.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/callvote map^7,^2/callvote mapnum^7,^2/callvote randommap^7 - Call a vote to switch to a map: By name, by map number (from ^2/maplist^7), or by random choice.\n\"",
		"print \"Random tip: ^2/callvote map^7,^2/callvote mapnum^7,^2/callvote randommap^7 - Call a vote to switch to a map: By name, by map number (from ^2/maplist^7), or by random choice.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"\n^7Account commands:\n\"",
		"print \"Random tip: \n^7Account commands:\n\"",
		qtrue,
		qfalse
	},
	{
		"print \"^2/register^7 - Call with username and password to create an account\n\"",
		"print \"Random tip: ^2/register^7 - Call with username and password to create an account\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/login^7 - Call with username and password to log into an existing account\n\"",
		"print \"Random tip: ^2/login^7 - Call with username and password to log into an existing account\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/changepassword^7 - Call with a new password while logged in to change your password.\n\"",
		"print \"Random tip: ^2/changepassword^7 - Call with a new password while logged in to change your password.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/logout^7 - Log out of your account.\n\"",
		"print \"Random tip: ^2/logout^7 - Log out of your account.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"\n^7Visual/personal tweak commands:\n\"",
		"print \"Random tip: \n^7Visual/personal tweak commands:\n\"",
		qtrue,
		qfalse
	},
	{
		"print \"^2/lasers^7 - Turn off or on the display of laserpointers by other players\n\"",
		"print \"Random tip: ^2/lasers^7 - Turn off or on the display of laserpointers by other players\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/solo^7 - Hide or unhide other players. Call with 'style' to hide only other styles/modes. Call with 'showall' to unhide machine learning bots.\n\"",
		"print \"Random tip: ^2/solo^7 - Hide or unhide other players. Call with 'style' to hide only other styles/modes. Call with 'showall' to unhide machine learning bots\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/ignore^7 - Ignore or unignore a player (call with client number from ^2/clientlist^7 or call with 'list' to list your ignores, 'clear' to clear all your ignores or 'all' to ignore all)\n\"",
		"print \"Random tip: ^2/ignore^7 - Ignore or unignore a player (call with client number from ^2/clientlist^7)\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"\n^7Meme commands:\n\"",
		"print \"Random tip: \n^7Meme commands:\n\"",
		qtrue,
		qtrue
	},
	{
		//"print \"^2/freedom^7,^2/oc9^7 - Serverside apply a freedom/oc9 name tag to your name\n\"",
		//"print \"Random tip: ^2/freedom^7,^2/oc9^7 - Serverside apply a freedom/oc9 name tag to your name\n\"",
		"print \"^2/freedom^7 - Serverside apply a freedom name tag to your name\n\"",
		"print \"Random tip: ^2/freedom^7 - Serverside apply a freedom name tag to your name\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/callvote opinion^7,^2/callvote opinionAll^7 - Call a vote on anything, for active players or for everybody.\n\"",
		"print \"Random tip: ^2/callvote opinion^7,^2/callvote opinionAll^7 - Call a vote on anything, for active players or for everybody.\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"\n^7Race style commands:\n\"",
		"print \"Random tip: \n^7Race style commands:\n\"",
		qtrue,
		qtrue
	},
	{
		"print \"^2/move^7 - Set your movement style (call without argument to see options)\n\"",
		"print \"Random tip: ^2/move^7 - Set your movement style (call without argument to see options)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/run^7 - Race style settings (segmented, strafebot, etc.)\n\"",
		"print \"Random tip: ^2/run^7 - Race style settings (segmented, strafebot, etc.)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/jump^7 - Call with -1 to 3 to set jump level (0 = no force, -1 = ysalamir)\n\"",
		"print \"Random tip: ^2/jump^7 - Call with -1 to 3 to set jump level (0 = no force, -1 = ysalamir)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/togglefps^7 - Turn fps toggle mode on or off (also needed for clients without com_physicsFps)\n\"",
		"print \"Random tip: ^2/togglefps^7 - Turn fps toggle mode on or off (also needed for clients without com_physicsFps)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/floatphysics^7 - Turn float physics mode (no velocity snap) on or off \n\"",
		"print \"Random tip: ^2/floatphysics^7 - Turn float physics mode (no velocity snap) on or off \n\"",
		qfalse,
		qtrue
	},
	{
		"print \"\n^7Race commands:\n\"",
		"print \"Random tip: \n^7Race commands:\n\"",
		qtrue,
		qtrue
	},
	{
		"print \"^2/savespawn^7 - Save your spawn point (only valid for your current race style settings). ^3This also saves your currently selected weapon.^7 Use ^2/kill^7 to respawn\n\"",
		"print \"Random tip: ^2/savespawn^7 - Save your spawn point (only valid for your current race style settings). ^3This also saves your currently selected weapon.^7 Use ^2/kill^7 to respawn\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/selectspawn^7 - Alternatively, select one of the real map spawn points for your start, e.g. for maps where ^2/savespawn^7 doesn't work. Call with ^2last^7 to remember the last point you spawned at or ^2closest^7 to select the spawn point closest to you. You can do this even from noclip.\n\"",
		"print \"Random tip: ^2/selectspawn^7 - As an alternative to ^2/savespawn^7, select one of the real map spawn points for your start, e.g. for maps where ^2/savespawn^7 doesn't work. Call with ^2last^7 to remember the last point you spawned at or ^2closest^7 to select the spawn point closest to you. You can do this even from noclip.\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/resetspawn^7 - Deletes/resets your saved spawn point\n\"",
		"print \"Random tip: ^2/resetspawn^7 - Deletes/resets your saved spawn point\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/savepos^7 - Save your current state including position, velocity and angles. Works also from spec.\n\"",
		"print \"Random tip: ^2/savepos^7 - Save your current state including position, velocity and angles. Works also from spec.\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/respos^7 - Restore your saved state. In segmented mode, ^2/respos discard^7 can discard a saved point to fix mistakes.\n\"",
		"print \"Random tip: ^2/respos^7 - Restore your saved state. In segmented mode, ^2/respos discard^7 can discard a saved point to fix mistakes.\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/stealspawn^7 - Steal spawn point from another player. Also steals style, if different. (call with client number from ^2/clientlist^7)\n\"",
		"print \"Random tip: ^2/stealspawn^7 - Steal spawn point from another player. Also steals style, if different. (call with client number from ^2/clientlist^7)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/stealpos^7 - Steal saved position from another player (call with client number from ^2/clientlist^7)\n\"",
		"print \"Random tip: ^2/stealpos^7 - Steal saved position from another player (call with client number from ^2/clientlist^7)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/amtele^7 - Call with a client number or name to teleport to a player\n\"",
		"print \"Random tip: ^2/amtele^7 - Call with a client number or name to teleport to a player\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/launch^7 - Launch yourself with speed. Call without arguments to see available options/parameters\n\"",
		"print \"Random tip: ^2/launch^7 - Launch yourself with speed. Call without arguments to see available options/parameters\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"\n^7Checkpoint commands:\n\"",
		"print \"Random tip: \n^7Checkpoint commands:\n\"",
		qtrue,
		qtrue
	},
	{
		"print \"^2/checkpoint^7 - Add a custom checkpoint at your current position\n\"",
		"print \"Random tip: ^2/checkpoint^7 - Add a custom checkpoint at your current position\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/removecheckpoints^7 - Remove all custom checkpoints\n\"",
		"print \"Random tip: ^2/removecheckpoints^7 - Remove all custom checkpoints\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/stealcheckpoints^7 - Steal custom checkpoints from another player (call with client number from ^2/clientlist^7)\n\"",
		"print \"Random tip: ^2/stealcheckpoints^7 - Steal custom checkpoints from another player (call with client number from ^2/clientlist^7)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/savecheckpoints^7 - Save your custom checkpoints for this map (only if you are logged in. Does not save times.)\n\"",
		"print \"Random tip: ^2/savecheckpoints^7 - Save your custom checkpoints for this map (only if you are logged in. Does not save times.)\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/loadcheckpoints^7 - Load your custom checkpoints for this map\n\"",
		"print \"Random tip: ^2/loadcheckpoints^7 - Load your custom checkpoints for this map\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"\n^7Statistics commands:\n\"",
		"print \"Random tip: \n^7Statistics commands:\n\"",
		qtrue,
		qtrue
	},
	{
		"print \"^2/rank^7 - Show rankings for a given style and leaderboard type. Default JK2/Main\n\"",
		"print \"Random tip: ^2/rank^7 - Show rankings for a given style and leaderboard type. Default JK2/Main\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/top^7 - Show leaderboards. Can call with map and subcourse, otherwise current map data is shown. Call with number to go to next page. Call with movement style to get leaderboards for specific movement style. Defaults to JK2 style\n\"",
		"print \"Random tip: ^2/top^7 - Show leaderboards. Can call with map and subcourse, otherwise current map data is shown. Call with number to go to next page. Call with movement style to get leaderboards for specific movement style. Defaults to JK2 style\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/topmain^7,^2/topnjb^7,^2/topcustom^7,^2/topseg^7,^2/topcheat^7 - Same options as ^2/top^7, shows more detailed specific leaderboards with average/top speed and more\n\"",
		"print \"Random tip: ^2/topmain^7,^2/topnjb^7,^2/topcustom^7,^2/topseg^7,^2/topcheat^7 - Same options as ^2/top^7, shows more detailed specific leaderboards with average/top speed and more\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/time^7 - Check and publicly print your personal best for your current race settings\n\"",
		"print \"Random tip: ^2/time^7 - Check and publicly print your personal best for your current race settings\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/latest^7 - Show latest runs. Can call with movement style and page. Optional: ^2/latest mine^7 or ^2/latest unlogged^7 or call with username\n\"",
		"print \"Random tip: ^2/latest^7 - Show latest runs. Can call with movement style and page. Optional: ^2/latest mine^7 or ^2/latest unlogged^7 or call with username\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/rollympics^7 - Show fastest roll records\n\"",
		"print \"Random tip: ^2/rollympics^7 - Show fastest roll records\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"^2/dbsrecords^7 - Show fastest dbs related records. Options: speed/kill/ironman/ctfret\n\"",
		"print \"Random tip: ^2/dbsrecords^7 - Show fastest dbs related records. Options: speed/kill/ironman/ctfret\n\"",
		qfalse,
		qtrue
	},
	{
		"print \"\n^7Client binds (named binds work in TommyTernal client):\n\"",
		"print \"Random tip: \n^7Client binds (named binds work in TommyTernal client):\n\"",
		qtrue,
		qfalse
	},
	{
		"print \"^2/+laserpointer^7 (^2/+button12^7) - Activates laserpointer in your current view direction to show stuff to others. Works even in spec (use ^2/lasers^7 to hide these)\n\"",
		"print \"Random tip: ^2/+laserpointer^7 (^2/+button12^7) - Activates laserpointer in your current view direction to show stuff to others. Works even in spec (use ^2/lasers^7 to hide these)\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/+bouncepower^7 (^2/+button13^7) - Activates stronger bounce in bounce movement style for up to half a second\n\"",
		"print \"Random tip: ^2/+bouncepower^7 (^2/+button13^7) - Activates stronger bounce in bounce movement style for up to half a second\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/+strafebot^7 (^2/+button14^7) - This button must be pressed in strafebot mode to activate the strafebot. Bind to a key or type in console to keep activated\n\"",
		"print \"Random tip: ^2/+strafebot^7 (^2/+button14^7) - This button must be pressed in strafebot mode to activate the strafebot. Bind to a key or type in console to keep activated\n\"",
		qfalse,
		qfalse
	},
	{
		"print \"^2/messagemode6^7 - Exists in recent TommyTernal versions. Like ^2/messagemode^7, the normal chat bind. Opens a prompt for chat, in this case for cross-server public chats.\n\"",
		"print \"Random tip: ^2/messagemode6^7 - Exists in recent TommyTernal versions. Like ^2/messagemode^7, the normal chat bind. Opens a prompt for chat, in this case for cross-server public chats.\n\"",
		qfalse,
		qfalse
	},
};

const int helpTipCount = sizeof(helpTips) / sizeof(helpTips[0]);


/*
=================
Cmd_Help_f
=================
*/
void Cmd_Help_f(gentity_t* ent) {
	char arg1[20];
	int i;

	if (trap_Argc() > 1) {
		trap_Argv(1,arg1,sizeof(arg1));
		if (!Q_stricmpn(arg1, "seg", 3)) {

			trap_SendServerCommand(ent - g_entities, "print \"^2SEGMENTED RUN HELP\n\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^7In a segmented run, you can save and restore your position at any time after your timer has started. Your timer is restored as well. After you finish your run, a replay of your run is played and your time is saved.\n\"");

			trap_SendServerCommand(ent - g_entities, "print \"\n^7Segmented run commands:\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/run 5^7 - Enables/disables segmented running\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/savepos^7 - Save your current state (only works after your timer starts)\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/respos^7 - Restore your saved state (only works in a run after using savepos)\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/respos discard^7 - Same as ^2/respos^7, but restores to a previous saved state, discarding the most recent one permanently\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/resseg^7 - If your run is failing, this command will allow you to restart the playback.\n\"");

			trap_SendServerCommand(ent - g_entities, "print \"\n^1Important rules:\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^11.^7 If you touch the start timer again during your run, your run ends\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^12.^7 Executing ^2/kill^7 (selfkill) command ends your segmented run. Unbind this for long runs for your own sanity\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^13.^7 Every time you fall into a death trigger on the map, you MUST call ^2/respos^7 again manually (even if it looks like it's working fine), otherwise your segmented state gets corrupted\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^14.^7 Before starting a run, stand still for a few short moments, this resets the usercommand recording for the replay to make things go smooth\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^15.^7 If you get the error that the recording is under 0.5 seconds and thus too short, try to move (e.g. walk/jump) for a second before actually starting your run\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^16.^7 If you get the error that the recording is over 5 seconds and thus too long, make sure you followed rule 4\n\"");


			trap_SendServerCommand(ent - g_entities, "print \"\n^3Please note that due to this feature being a bit experimental, there is a small chance of the replay failing and your run not being added to the leaderboards. This is exaggerated on maps with elevators/doors and complicated trigger logic.\n\"");

			return;
		} else if (!Q_stricmpn(arg1, "tas", 3)) {

			trap_SendServerCommand(ent - g_entities, "print \"^2TAS (tool assisted speedrun) help\n\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^7In TAS mode, you're allowed to do things like use scripts in order to create faster runs. This is not allowed in strafebot mode. If you want to use scripts, be sure to enable TAS mode.\n\"");

			trap_SendServerCommand(ent - g_entities, "print \"\n^7Relevant commands:\n\"");
			trap_SendServerCommand(ent - g_entities, "print \"^2/run 7^7 - Enables/disables TAS mode\n\"");

			trap_SendServerCommand(ent - g_entities, "print \"\n^3Additional features may be added in the future.\n\"");

			return;
		}
	}

	trap_SendServerCommand(ent - g_entities, "print \"^2HELP\n\"");
	trap_SendServerCommand(ent-g_entities,"print \"^7Call ^2/help seg^7 to get help specific to segmented runs.\n\"");
	trap_SendServerCommand(ent-g_entities,"print \"^7Call ^2/help tas^7 to get help specific to scripting runs/TAS mode (^1DO NOT SCRIPT ANYWHERE EXCEPT TAS MODE!^7).\n\n\"");
	trap_SendServerCommand(ent - g_entities, "print \"^7Available commands:\n\n\"");

	if (g_defrag.integer) {
		trap_SendServerCommand(ent - g_entities, va("print \"^2/race^7 - Call to %s racemode.\n\"", ent->client->sess.raceMode ? "exit/enter":"enter/exit")); 
	}

	for (i = 0; i < helpTipCount; i++) {
		if ((!helpTips[i].raceOnly || ent->client->sess.raceMode) && (!helpTips[i].allowfunc || helpTips[i].allowfunc(ent))) {
			trap_SendServerCommand(ent - g_entities, helpTips[i].helpPrint);
		}
	}

	//trap_SendServerCommand(ent - g_entities, "print \"\n^7Various commands:\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/afk^7 - See who's afk and for how long\n\"");

	//trap_SendServerCommand(ent - g_entities, "print \"\n^7Map commands:\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/maplist^7 - Call to see list of maps you can callvote. Optional: ^2/maplist unplayed\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/longest^7,^2/shortest^7,^2/toprated^7,^2/mostplayed^7,^2/hardest^7,^2/easiest^7 - Show longest/shortest/popular(by rating)/popular(by amount of runs)/hardest/easiest maps. Can call with movement style and page.\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/ratemap^7 - Rate the current map from 0 to 10. Call with movement style and number.\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/wrs^7,^2/notwr^7 - Show maps you hold/don't hold WR on, sorted by your current rank, highest first. Can call with username.\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/callvote map^7,^2/callvote mapnum^7,^2/callvote randommap^7 - Call a vote to switch to a map: By name, by map number (from ^2/maplist^7), or by random choice.\n\"");

	//trap_SendServerCommand(ent - g_entities, "print \"\n^7Account commands:\n\"");

	//trap_SendServerCommand(ent - g_entities, "print \"^2/register^7 - Call with username and password to create an account\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/login^7 - Call with username and password to log into an existing account\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/changepassword^7 - Call with a new password while logged in to change your password.\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/logout^7 - Log out of your account.\n\"");


	//trap_SendServerCommand(ent - g_entities, "print \"\n^7Visual/personal tweak commands:\n\"");

	//trap_SendServerCommand(ent - g_entities, "print \"^2/lasers^7 - Turn off or on the display of laserpointers by other players\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/solo^7 - Hide or unhide other players\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/ignore^7 - Ignore or unignore a player (call with client number from ^2/clientlist^7)\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/freedom^7,^2/oc9^7 - Serverside apply a freedom/oc9 name tag to your name\n\"");

	//if (ent->client->sess.raceMode) {

	//	trap_SendServerCommand(ent - g_entities, "print \"\n^7Race style commands:\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/move^7 - Set your movement style (call without argument to see options)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/run^7 - Race style settings (segmented, strafebot, etc.)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/jump^7 - Call with -1 to 3 to set jump level (0 = no force, -1 = ysalamir)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/togglefps^7 - Turn fps toggle mode on or off (also needed for clients without com_physicsFps)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/floatphysics^7 - Turn float physics mode (no velocity snap) on or off \n\"");

	//	trap_SendServerCommand(ent - g_entities, "print \"\n^7Race commands:\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/savespawn^7 - Save your spawn point (only valid for your current race style settings). ^3This also saves your currently selected weapon.^7 Use ^2/kill^7 to respawn\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/resetspawn^7 - Deletes/resets your saved spawn point\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/savepos^7 - Save your current state including position, velocity and angles. Works also from spec.\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/respos^7 - Restore your saved state\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/stealspawn^7 - Steal spawn point from another player. Also steals style, if different. (call with client number from ^2/clientlist^7)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/stealpos^7 - Steal saved position from another player (call with client number from ^2/clientlist^7)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/amtele^7 - Call with a client number or name to teleport to a player\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/launch^7 - Launch yourself with speed. Call without arguments to see available options/parameters\n\"");

	//	trap_SendServerCommand(ent - g_entities, "print \"\n^7Checkpoint commands:\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/checkpoint^7 - Add a custom checkpoint at your current position\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/removecheckpoints^7 - Remove all custom checkpoints\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/stealcheckpoints^7 - Steal custom checkpoints from another player (call with client number from ^2/clientlist^7)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/savecheckpoints^7 - Save your custom checkpoints for this map (only if you are logged in. Does not save times.)\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/loadcheckpoints^7 - Load your custom checkpoints for this map\n\"");

	//	trap_SendServerCommand(ent - g_entities, "print \"\n^7Statistics commands:\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/rank^7 - Show rankings for a given style and leaderboard type. Default JK2/Main\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/top^7 - Show leaderboards. Can call with map and subcourse, otherwise current map data is shown. Call with number to go to next page. Call with movement style to get leaderboards for specific movement style. Defaults to JK2 style\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/topmain^7,^2/topnjb^7,^2/topcustom^7,^2/topseg^7,^2/topcheat^7 - Same options as ^2/top^7, shows more detailed specific leaderboards with average/top speed and more\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/time^7 - Check and publicly print your personal best for your current race settings\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/latest^7 - Show latest runs. Can call with movement style and page. Optional: ^2/latest mine^7 or ^2/latest unlogged or call with username\n\"");
	//	trap_SendServerCommand(ent - g_entities, "print \"^2/rollympics^7 - Show fastest roll records\n\"");
	//}

	//trap_SendServerCommand(ent - g_entities, "print \"\n^7Client binds (named binds work in TommyTernal client):\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/+laserpointer^7 (^2/+button12^7) - Activates laserpointer in your current view direction to show stuff to others. Works even in spec (use ^2/lasers^7 to hide these)\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/+bouncepower^7 (^2/+button13^7) - Activates stronger bounce in bounce movement style for up to half a second\n\"");
	//trap_SendServerCommand(ent - g_entities, "print \"^2/+strafebot^7 (^2/+button14^7) - This button must be pressed in strafebot mode to activate the strafebot. Bind to a key or type in console to keep activated\n\"");

	Cmd_PrintCmdDescriptions_f(ent);

	if (g_allowChatPause.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Pausing/unpausing is currently enabled. Game can be paused by typing ^1!pause^7 in chat, and unpaused by ^1!unpause^7.\n\"");
	}

	if (ent->client->sess.login.loggedIn && ent->client->sess.login.flags) {
		trap_SendServerCommand(ent - g_entities, "print \"\n^7Admin commands:\n\"");
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_CHANGEMAPDEFAULTRACESTYLE) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/mapdefaults^7 - Change map defaults\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_ARENAGEN) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/genArena^7 - call with 'this' or 'allrace' to generate arenas for current map or all maps that have been ran\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_USERSFORCELOGIN) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/forcelogin^7 - call with client number and account name to force login a player so he can change his password\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_ARENALESSMAPS) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/arenaless^7 - List .bsp files without corresponding arena files\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_BLACKLISTMAPS) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/blacklistmap^7 - Blacklists either the current or a named map from being shown in maplist/arenaless list\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_UPDATERANKS) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/updateRanks^7 - Update temporary ranks of this map in the DB\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_DEMOMANAGE) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/demoManage^7 - Call with ^2checkscript^7 to make a script testing the existence of all run demos and generating a copy script from a backup location for missing demos\n\"");
		}
		if (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_CHANGEMAP) {
			trap_SendServerCommand(ent - g_entities, "print \"^2/amMap^7 - Force change map, call with gametype number and mapname\n\"");
		}
	}

}
int G_TeamForString(const char* str, qboolean allowTeamPlaying);

//Called when a user tried to change his team himself
//separated so admins and code can use normal setteam, but setteam_user will check for locked teams, etc.
// TODO check if this needs to be used in other places like setclientmode etc
qboolean SetTeam_User( gentity_t *ent, const char *s ) {
	int oldTeam;
	int newTeam;
	const char *found = NULL;

	if (!ent || !ent->client)
		return qfalse;

	oldTeam = ent->client->sess.sessionTeam;

	//check if hes not allowed to change team
	if (ent->client->sess.amflags & AMFLAG_LOCKEDTEAM) {
		trap_SendServerCommand( ent - g_entities, "print \"You have been denied permission to change team.\n\"" );
		return qfalse;
	}

	#if 0	//i guess this is not standard behaviour..
	if (level.lockedTeams & (1 << oldTeam)) {
		//This client's current team is locked, so dont let him escape it
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", "Your team is locked; you can't leave it.") );
		return qfalse;
	}
	#endif

	if ( (newTeam = G_TeamForString(s, qfalse)) == -1) {
		trap_SendServerCommand( ent - g_entities,va("print \"Unknown team '%s'.\n\"", s));
		return qfalse;
	}



	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities,va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return qfalse;
	}


	if (level.lockedTeams & (1 << newTeam)) {
		trap_SendServerCommand( ent-g_entities, "print \"That team is locked.\n\"" );
		return qfalse;
	}

	if (!DefragDoubleTapSafety(ent, DOUBLETAP_TEAM, "team")) {
		return qfalse;
	}

	if (level.lockedTeams && newTeam == TEAM_FREE && g_gametype.integer >= GT_TEAM) {
		//prevent players from using team free which will assign players to either blue or red in team games,
		//when those teams are locked

		if ( !(level.lockedTeams & (1 << TEAM_RED)) )
			found = "r";
		else if ( !(level.lockedTeams & (1 << TEAM_BLUE)) )
			found = "b";
		else
			found = NULL;

		if (!found) {
			trap_SendServerCommand( ent-g_entities, "print \"Teams ^1RED^7 and ^4BLUE^7 are both locked.\n\"" );
			return qfalse;
		}

		s = found;
	}

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& oldTeam == TEAM_FREE) {//in a tournament game
		//disallow changing teams
		trap_SendServerCommand(ent - g_entities, "print \"Cannot switch teams in Duel\n\"");
		return qfalse;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	//all checks parsed, he can change team
	if (SetTeam(ent, (char*)s)) {
		// team changed
		ent->client->switchTeamTime = level.time + 3000; // MOVED and reduced time from 5000
	}
	else {
		// same team or no change
		// make this less annoying when we accidentally hit spectate at start of game (BUT WE ALREADY ARE ANYWAY!)
		ent->client->switchTeamTime = level.time + 1000;
	}
	return qtrue;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}


	if (gEscaping)
	{
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam_User(ent, s);

}


static void Cmd_Launch_f(gentity_t* ent)
{
	char xySpeedStr[16], xStr[16], yStr[16], zStr[16], yawStr[16], zSpeedStr[16];
	vec3_t fwdAngles, jumpFwd;
	const int clampSpeed = 25000;
	int frameTime;

	if (!ent->client)
		return;


	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in race mode to use this command!\n\""); //Should never happen since cant be in practice w/o racemode? or... w/e
		return;
	}

	if (trap_Argc() != 2 && trap_Argc() != 7) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /launch <speed> or /launch <x y z yaw xyspeed zspeed>\n\"");
		return;
	}

	if (!DefragDoubleTapSafety(ent, DOUBLETAP_LAUNCH, "launch")) {
		return;
	}

	DF_RaceStateInvalidated(ent, qtrue);

	if (trap_Argc() == 2) {
		int xyspeed;

		trap_Argv(1, xySpeedStr, sizeof(xySpeedStr));

		xyspeed = atoi(xySpeedStr);
		if (xyspeed > clampSpeed)
			xyspeed = clampSpeed;
		else if (xyspeed < -clampSpeed)
			xyspeed = -clampSpeed;

		VectorCopy(ent->client->ps.viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, xyspeed, ent->client->ps.velocity);
		ent->client->ps.velocity[2] = 270; //Hmm?
	}
	else {
		int xyspeed, zspeed;
		vec3_t origin, angles;

		trap_Argv(1, xStr, sizeof(xStr));
		trap_Argv(2, yStr, sizeof(yStr));
		trap_Argv(3, zStr, sizeof(zStr));
		trap_Argv(4, yawStr, sizeof(yawStr));
		trap_Argv(5, xySpeedStr, sizeof(xySpeedStr));
		trap_Argv(6, zSpeedStr, sizeof(zSpeedStr));

		xyspeed = atoi(xySpeedStr);
		if (xyspeed > clampSpeed)
			xyspeed = clampSpeed;
		else if (xyspeed < -clampSpeed)
			xyspeed = -clampSpeed;

		zspeed = atoi(zSpeedStr);
		if (zspeed > clampSpeed)
			zspeed = clampSpeed;
		else if (zspeed < -clampSpeed)
			zspeed = -clampSpeed;

		origin[0] = atoi(xStr);
		origin[1] = atoi(yStr);
		origin[2] = atoi(zStr);
		angles[0] = 0;
		angles[1] = atoi(yawStr);
		angles[2] = 0;

		//tele
		//AmTeleportPlayer(ent, origin, angles, qfalse, qtrue, qfalse);
		TeleportPlayer(ent, origin, angles);//, qfalse, qtrue, qfalse);

		fwdAngles[0] = 0;
		fwdAngles[1] = atoi(yawStr);
		fwdAngles[2] = 0;
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);

		VectorScale(jumpFwd, xyspeed, ent->client->ps.velocity);
		ent->client->ps.velocity[2] = zspeed; //Hmm?
	}

	//PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

	//PM_AddEvent( EV_JUMP );
	ent->client->ps.fd.forceJumpSound = 1;
	//ent->client->pers.cmd.upmove = 0;


	//frameTime = ent->client->pmoveMsec;
	//if (frameTime > 16)
	//	frameTime = 16;
	//ent->client->pers.stats.startTime = trap_Milliseconds() + frameTime; //Set their timer as now..
	//ent->client->ps.duelTime = level.time;
	//ent->client->pers.startLag = trap_Milliseconds() - level.frameStartTime + level.time - ent->client->pers.cmd.serverTime; //use level.previousTime?

	//ent->client->pers.stats.displacement = 0;
	//ent->client->pers.stats.displacementSamples = 0;//avg fix for standing in starttimer and /launch
	//ent->client->pers.stats.coopStarted = qtrue;
}


/*
=================
Cmd_Team_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStripEdString("SVINGAME", "FORCEPOWERCHANGED");

	Q_strncpyz(fpChStr, buf,sizeof(fpChStr));

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (g_gametype.integer == GT_TOURNAMENT)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg, sizeof( arg ) );

		if ( !Q_stricmp(arg, "none") || !Q_stricmp(arg, "same") || !Q_stricmp(arg, ";") ) return; // 1.02 clients send those and trigger unwanted team-changes...

		//if there's an arg, assume it's a combo team command from the UI.
		Cmd_Team_f(ent);
	}
}
gentity_t* GetClientNumArg();


static void Cmd_IgnoreClear_f (gentity_t *ent) {
	int i;
	const int clNum = ent - g_entities;

	if (!ent->client->sess.ignore) {
		trap_SendServerCommand(clNum,"print \"Your ignorelist is already empty.\n\"");
	} else {
		trap_SendServerCommand(clNum,"print \"Your ignorelist was cleared.\n\"");
	}

	ent->client->sess.ignore = 0;
}

static void Cmd_IgnoreList_f (gentity_t *ent) {
	int i;
	const int clNum = ent - g_entities;

	if (!ent->client->sess.ignore) {
		trap_SendServerCommand(clNum,"print \"Your ignorelist is empty.\n\"");
		return;
	}

	trap_SendServerCommand(clNum,"print \"^5Ignorelist:\n\"");

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;

		if (ent->client->sess.ignore & (1 << i)) {
			const char *str = va("  %02d^%c)^7 %s \n", i, ColorCodeForClient(&g_entities[i], qfalse), g_entities[i].client->pers.netname);

			trap_SendServerCommand(clNum, va("print \"%s\"", str));
		}
	}
}

void Cmd_Ignore_f(gentity_t* ent) { // TODO make this able to do "all" and finding by name, like vvv-serverside
	gentity_t* client = GetClientNumArg();
	int clientnum;
	if (!client) {
		if (trap_Argc() > 1) {
			char arg[10];
			trap_Argv(1, arg, sizeof(arg));
			if (!Q_stricmp(arg,"list")) {
				Cmd_IgnoreList_f(ent);
				return;
			}
			else if (!Q_stricmp(arg,"clear")) {
				Cmd_IgnoreClear_f(ent);
				return;
			}
			else if (!Q_stricmp(arg,"all")) {
				// from vvv-serverside
				int i;
				qboolean ignore = qfalse;

				//check if all are ignored
				for (i = 0; i < MAX_CLIENTS; ++i) {
					if (level.clients[i].pers.connected == CON_DISCONNECTED)
						continue;

					if (!(ent->client->sess.ignore & (1 << i))) {
						// we must ignore all.
						ignore = qtrue;
						break;
					}
				}

				for (i = 0; i < MAX_CLIENTS; ++i) {
					if (level.clients[i].pers.connected == CON_DISCONNECTED)
						continue;

					if (ignore) {
						ent->client->sess.ignore |= (1 << i);
					}
					else {
						ent->client->sess.ignore &= ~(1 << i);
					}
				}

				if (ignore) {
					trap_SendServerCommand(ent - g_entities, "print \"All are now being ignored.\n\"");
				}
				else {
					trap_SendServerCommand(ent - g_entities, "print \"All have been unignored.\n\"");
				}

				return;
			}
		}
		trap_SendServerCommand(ent - g_entities, "print \"^1Invalid client number specified.\n\"");
		return;
	}
	clientnum = client - g_entities;
	ent->client->sess.ignore = ent->client->sess.ignore ^ (1 << clientnum);
	if (ent->client->sess.ignore & (1 << clientnum)) {
		trap_SendServerCommand(ent - g_entities, va("print \"^1Ignoring client %d now.\n\"",clientnum));
	}
	else {
		trap_SendServerCommand(ent - g_entities, va("print \"^1Not ignoring client %d anymore.\n\"", clientnum));
	}
}
void Cmd_Lasers_f(gentity_t* ent) {
	if (!ent->client->sess.hideLasers) {

		trap_SendServerCommand(ent - g_entities, "print \"^1Hiding laserpointers now.\n\"");
		ent->client->sess.hideLasers = qtrue;
	}
	else {

		trap_SendServerCommand(ent - g_entities, "print \"^1Showing laserpointers now.\n\"");
		ent->client->sess.hideLasers = qfalse;
	}
}
void Cmd_Solo_f(gentity_t* ent) {
	soloState_t onState = SOLO_ALL;
	if (trap_Argc() > 1) {
		static char arg[10];
		trap_Argv(1,arg,sizeof(arg));
		if (!Q_stricmp(arg, "style")) {
			onState = SOLO_STYLE;
		}
		else if (!Q_stricmp(arg, "showall")) {
			onState = SOLO_SHOWALL;
		}
	}
	if (onState == ent->client->sess.solo) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Resetting solo mode, showing all regular players.\n\"");
		ent->client->sess.solo = SOLO_DISABLED;
		return;
	}

	ent->client->sess.solo = onState;
	switch (onState) {
	case SOLO_SHOWALL:
		trap_SendServerCommand(ent - g_entities, "print \"^1Showing all players now including machine learning bots.\n\"");
		break;
	case SOLO_STYLE:
		trap_SendServerCommand(ent - g_entities, "print \"^1Hiding players of other styles/modes now.\n\"");
		break;
	case SOLO_ALL:
		trap_SendServerCommand(ent - g_entities, "print \"^1Hiding other players now.\n\"");
		break;
	default:
		trap_SendServerCommand(ent - g_entities, "print \"^1Something weird happened wooooooot.\n\"");
		break;
	}
}

/*
=================
Cmd_Register_f
=================
*/
void Cmd_Register_f( gentity_t *ent )
{
	static char cmd[MAX_TOKEN_CHARS];
	static char thirdparam[MAX_TOKEN_CHARS];
	static loginRegisterStruct_t loginData;
	qboolean needDoubleBCrypt = qtrue;
	trap_Argv(0, cmd, sizeof(cmd));
	if (coolApi_dbVersion < 2) {
		// DB API cannot do bcrypt.
		trap_SendServerCommand(ent - g_entities, va("print \"^1Server %s not possible. DB version too low.\n\"", cmd));
		return;
	}
	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent - g_entities, va("print \"usage /%s <username> <password>\n\"",cmd));
		return;
	}
	memset(&loginData, 0, sizeof(loginData));
	if (trap_Argc() >= 4) {
		trap_Argv(3, thirdparam, sizeof(thirdparam));
		if (!Q_stricmp(thirdparam, "bcrypt")) {
			needDoubleBCrypt = qfalse; // client already bcrypted once :)
		}
	}
	trap_Argv(1, loginData.username, sizeof(loginData.username));
	trap_Argv(2, loginData.password, sizeof(loginData.password));
	loginData.clientnum = ent - g_entities;

	if (!G_DB_VerifyUsername(loginData.username, loginData.clientnum)) {
		return;
	}
	
	// can only verify the password structure here if it wasn't already hashed. clientside has same check but if someone decides to bypass it, nothing we can do.
	// they just wont be able to log in without their modified client then, oh well.
	if (needDoubleBCrypt && !BG_DB_VerifyPassword(loginData.password, loginData.clientnum)) {
		return;
	}

	memcpy(loginData.ip,mv_clientSessions[loginData.clientnum].clientIP,sizeof(loginData.ip));
	//loginData.followUpType = !Q_stricmp("login", cmd) ? DBREQUEST_LOGIN : DBREQUEST_REGISTER;
	loginData.followUpType = DBREQUEST_REGISTER;
	if (needDoubleBCrypt) {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("2|%s|random|%s", BCRYPT_SETTINGS, loginData.password) 
			, DBREQUESTTYPE_BCRYPT);
	}
	else {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("1|random|%s", loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}
}

/*
=================
Cmd_ChangePassword_f
=================
*/
void Cmd_ChangePassword_f( gentity_t *ent )
{
	static char cmd[MAX_TOKEN_CHARS];
	static char secondparam[MAX_TOKEN_CHARS];
	static loginRegisterStruct_t loginData;
	qboolean needDoubleBCrypt = qtrue;
	trap_Argv(0, cmd, sizeof(cmd));
	if (coolApi_dbVersion < 2) {
		// DB API cannot do bcrypt.
		trap_SendServerCommand(ent - g_entities, va("print \"^1Server %s not possible. DB version too low.\n\"", cmd));
		return;
	}
	if (!ent->client->sess.login.loggedIn) {
		// DB API cannot do bcrypt.
		trap_SendServerCommand(ent - g_entities, va("print \"^1%s not possible. You are not logged in.\n\"", cmd));
		return;
	}
	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent - g_entities, va("print \"usage /%s <password>\n\"",cmd));
		return;
	}
	memset(&loginData, 0, sizeof(loginData));
	if (trap_Argc() >= 3) {
		trap_Argv(2, secondparam, sizeof(secondparam));
		if (!Q_stricmp(secondparam, "bcrypt")) {
			needDoubleBCrypt = qfalse; // client already bcrypted once :)
		}
	}
	trap_Argv(1, loginData.password, sizeof(loginData.password));

	Q_strncpyz(loginData.username, ent->client->sess.login.name, sizeof(loginData.username));
	loginData.userId = ent->client->sess.login.id;
	loginData.clientnum = ent - g_entities;
		
	// can only verify the password structure here if it wasn't already hashed. clientside has same check but if someone decides to bypass it, nothing we can do.
	// they just wont be able to log in without their modified client then, oh well.
	if (needDoubleBCrypt && !BG_DB_VerifyPassword(loginData.password, loginData.clientnum)) {
		return;
	}

	memcpy(loginData.ip,mv_clientSessions[loginData.clientnum].clientIP,sizeof(loginData.ip));
	//loginData.followUpType = !Q_stricmp("login", cmd) ? DBREQUEST_LOGIN : DBREQUEST_REGISTER;
	loginData.followUpType = DBREQUEST_CHANGEPASSWORD;
	if (needDoubleBCrypt) {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("2|%s|random|%s", BCRYPT_SETTINGS, loginData.password) 
			, DBREQUESTTYPE_BCRYPT);
	}
	else {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("1|random|%s", loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}
}

/*
=================
Cmd_Login_f
=================
*/
void Cmd_Login_f( gentity_t *ent )
{
	static char cmd[MAX_TOKEN_CHARS];
	static char thirdparam[MAX_TOKEN_CHARS];
	static loginRegisterStruct_t loginData;

	if (ent->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent - g_entities, va("print \"^1You are already logged in as '%s'.\n\"", ent->client->sess.login.name));
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));
	if (coolApi_dbVersion < 2) {
		// DB API cannot do bcrypt.
		trap_SendServerCommand(ent - g_entities, va("print \"^1Server %s not possible. DB version too low.\n\"", cmd));
		return;
	}
	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent - g_entities, va("print \"usage /%s <username> <password>\n\"",cmd));
		return;
	}
	memset(&loginData, 0, sizeof(loginData));
	loginData.needDoubleBcrypt = qtrue;
	if (trap_Argc() >= 4) {
		trap_Argv(3, thirdparam, sizeof(thirdparam));
		if (!Q_stricmp(thirdparam, "bcrypt")) {
			loginData.needDoubleBcrypt = qfalse; // client already bcrypted once :)
		}
	}
	trap_Argv(1, loginData.username, sizeof(loginData.username));
	trap_Argv(2, loginData.password, sizeof(loginData.password));

	loginData.clientnum = ent - g_entities;
	memcpy(loginData.ip,mv_clientSessions[loginData.clientnum].clientIP,sizeof(loginData.ip));
	if (coolApi_dbVersion >= 3) {
		G_COOL_API_DB_AddPreparedStatement((byte*)&loginData, sizeof(loginData), DBREQUEST_LOGIN,
			"SELECT password,flags,id,username FROM users WHERE username=?");
		G_COOL_API_DB_PreparedBindString(loginData.username);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		static char	cleanUsername[MAX_STRING_CHARS];
		Q_strncpyz(cleanUsername, loginData.username, sizeof(cleanUsername));
		if (!G_COOL_API_DB_EscapeString(cleanUsername, sizeof(cleanUsername))) {
			Com_Printf("Cmd_Login_f: EscapeString failed.\n");
			trap_SendServerCommand(ent - g_entities, va("print \"/%s failed: EscapeString failed\n\"", cmd));
			return;
		}
		G_COOL_API_DB_AddRequest((byte*)&loginData, sizeof(loginData), DBREQUEST_LOGIN,
			va("SELECT password,flags,id FROM users WHERE username='%s'", cleanUsername));
	}
}


void Cmd_NameTag_f(gentity_t* ent) {
	char	cmd[20];
	nameTagType_t	type = NAMETAG_NONE;
	trap_Argv(0, cmd, sizeof(cmd));
	if (!Q_stricmp("freedom", cmd)) {
		type = NAMETAG_FREEDOM;
	}
	else if (!Q_stricmp("oc9", cmd)) {
		type = NAMETAG_OC9;
	}
	else {
		trap_SendServerCommand(ent - g_entities, "print \"^1Weird error. Name tag not recognized.\n\"");
		return; // dunno
	}
	if (ent->client->sess.nameTag == type) {
		trap_SendServerCommand(ent - g_entities, va("print \"^3Name tag disabled: %s\n\"", cmd));
		ent->client->sess.nameTag = NAMETAG_NONE;
	}
	else {
		trap_SendServerCommand(ent - g_entities, va("print \"^2Name tag applied: %s\n\"",cmd));
		ent->client->sess.nameTag = type;
	}
	ClientUserinfoChanged(ent-g_entities);

}

void Cmd_ForceLogin_f(gentity_t* ent) {
	qboolean allRace = qfalse;
	char	arg1[3];
	int		clientnum;
	gentity_t* otherClient = NULL;
	loginRegisterStruct_t data;

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_USERSFORCELOGIN)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You do not have permission to use this command.\n\"");
		return;
	}

	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: forcelogin <clientnum> <username>\n\"");
		return;
	}
	trap_Argv(1, arg1, sizeof(arg1));

	clientnum = atoi(arg1);

	if (clientnum < 0 || clientnum >= level.maxclients) {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid clientnum. Usage: forcelogin <clientnum> <username>\n\"");
		return;
	}

	otherClient = g_entities + clientnum;

	if (!otherClient->inuse || !otherClient->client || otherClient->client->pers.connected != CON_CONNECTED) {
		trap_SendServerCommand(ent - g_entities, "print \"Target client not in a valid state. Must be fully connected and active.\n\"");
		return;
	}

	if (otherClient != ent && otherClient->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent - g_entities, va("print \"Target client is already logged in as %s. He must log out first.\n\"", otherClient->client->sess.login.name));
		return;
	}

	memset(&data, 0, sizeof(data));
	data.clientnum = otherClient - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	data.clientnumAdmin = ent - g_entities;
	memcpy(data.ipAdmin, mv_clientSessions[data.clientnumAdmin].clientIP, sizeof(data.ipAdmin));
	trap_Argv(2, data.username, sizeof(data.username));


	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_FORCEDLOGIN, "SELECT flags,id,username FROM users WHERE username=?")) {
		trap_SendServerCommand(ent - g_entities, "print \"^1DB Error performing force login request.\n\"");
		return;
	}
	G_COOL_API_DB_PreparedBindString(data.username);
	G_COOL_API_DB_FinishAndSendPreparedStatement();

}

/*
=================
Cmd_Logout_f
=================
*/
void Cmd_Logout_f( gentity_t *ent )
{
	if (!ent->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent - g_entities, "print \"You are already logged out.\n\"");
		return;
	}
	memset(&ent->client->sess.login, 0, sizeof(ent->client->sess.login));
	DF_SetSubContestDefaults(ent->client);
	trap_SendServerCommand(ent - g_entities, "print \"^2You were successfully logged out.\n\"");
	if (ent->client->pers.raceBestTime) {
		ent->client->pers.raceBestTime = 0;
		CalculateRanks();
	}
	ClientUserinfoChanged(ent - g_entities);
	memset(ent->client->pers.mapRatings,0,sizeof(ent->client->pers.mapRatings));
	G_SendPlayerMapRatingsUIInfo(ent);
}

extern const char* DF_GetMainSubcourseName();
extern void Cmd_DF_MapDefaults_f(gentity_t* ent);

extern int JP_ClientNumberFromString(gentity_t* to, const char* s);

//[JAPRO - Serverside - All - Amtele Function - Start]
void Cmd_Amtele_f(gentity_t* ent)
{
	gentity_t* teleporter;// = NULL;
	char client1[MAX_NETNAME], client2[MAX_NETNAME];
	char x[32], y[32], z[32], yaw[32];
	int clientid1 = -1, clientid2 = -1;
	vec3_t	angles = { 0, 0, 0 }, origin;
	qboolean droptofloor = qfalse, race = qfalse;
	int allowed;

	if (!ent->client)
		return;
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_FOLLOW)) //lazy
		return;
	if (!ent->client->sess.raceMode)
		return;


	if (ent->client->sess.raceMode) {
		droptofloor = qtrue;
		race = qtrue;
	}

	if (trap_Argc() > 6)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amTele or /amTele <client> or /amTele <client> <client> or /amTele <X> <Y> <Z> <YAW> or /amTele <player> <X> <Y> <Z> <YAW>.\n\"");
		return;
	}

	if (!DefragDoubleTapSafety(ent, DOUBLETAP_AMTELE, "amtele")) {
		return;
	}

	//if (trap_Argc() == 1)//Amtele to telemark
	//{
	//	if (ent->client->pers.telemarkOrigin[0] != 0 || ent->client->pers.telemarkOrigin[1] != 0 || ent->client->pers.telemarkOrigin[2] != 0 || ent->client->pers.telemarkAngle != 0)
	//	{
	//		angles[YAW] = ent->client->pers.telemarkAngle;
	//		angles[PITCH] = ent->client->pers.telemarkPitchAngle;
	//		AmTeleportPlayer(ent, ent->client->pers.telemarkOrigin, angles, droptofloor, race, qfalse);
	//	}
	//	else
	//		trap_SendServerCommand(ent - g_entities, "print \"No telemark set!\n\"");
	//	return;
	//}

	if (trap_Argc() == 2)//Amtele to player
	{
		trap_Argv(1, client1, sizeof(client1));
		clientid1 = JP_ClientNumberFromString(ent, client1);

		if (clientid1 == -1 || clientid1 == -2)
			return;

		if (/*g_entities[clientid1].client->pers.noFollow ||*/ g_entities[clientid1].client->sess.sessionTeam == TEAM_SPECTATOR) {
			//if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_SEEHIDDEN, qfalse, qfalse, NULL))
				return;
		}

		origin[0] = g_entities[clientid1].client->ps.origin[0];
		origin[1] = g_entities[clientid1].client->ps.origin[1];
		origin[2] = g_entities[clientid1].client->ps.origin[2] + 96;
		DF_RaceStateInvalidated(ent, qtrue);
		TeleportPlayer(ent, origin, angles);// , droptofloor, race, qfalse);
		return;
	}

	//if (trap_Argc() == 3)//Amtele player to player
	//{
	//	trap_Argv(1, client1, sizeof(client1));
	//	trap_Argv(2, client2, sizeof(client2));
	//	clientid1 = JP_ClientNumberFromString(ent, client1);
	//	clientid2 = JP_ClientNumberFromString(ent, client2);

	//	if (clientid1 == -1 || clientid1 == -2 || clientid2 == -1 || clientid2 == -2)
	//		return;

	//	if (g_entities[clientid2].client->pers.noFollow || g_entities[clientid2].client->sess.sessionTeam == TEAM_SPECTATOR) {
	//		if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_SEEHIDDEN, qfalse, qfalse, NULL))
	//			return;
	//	}

	//	if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
	//		if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
	//			return;
	//		else
	//			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
	//	}

	//	teleporter = &g_entities[clientid1];

	//	origin[0] = g_entities[clientid2].client->ps.origin[0];
	//	origin[1] = g_entities[clientid2].client->ps.origin[1];
	//	origin[2] = g_entities[clientid2].client->ps.origin[2] + 96;

	//	AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
	//	return;
	//}

	if (trap_Argc() == 4)//|| trap_Argc() == 5)//Amtele to origin (if no angle specified, default 0?)
	{
		trap_Argv(1, x, sizeof(x));
		trap_Argv(2, y, sizeof(y));
		trap_Argv(3, z, sizeof(z));

		origin[0] = atoi(x);
		origin[1] = atoi(y);
		origin[2] = atoi(z);

		/*if (trap_Argc() == 5)
		{
			trap_Argv(4, yaw, sizeof(yaw));
			angles[YAW] = atoi(yaw);
		}*/

		DF_RaceStateInvalidated(ent, qtrue);
		TeleportPlayer(ent, origin, angles);// , droptofloor, race, qfalse);
		return;
	}

	//if (trap_Argc() == 5)//Amtele to angles + origin, OR Amtele player to origin
	//{
	//	trap_Argv(1, client1, sizeof(client1));
	//	clientid1 = JP_ClientNumberFromString(ent, client1);

	//	if (clientid1 == -1 || clientid1 == -2)//Amtele to origin + angles
	//	{
	//		trap_Argv(1, x, sizeof(x));
	//		trap_Argv(2, y, sizeof(y));
	//		trap_Argv(3, z, sizeof(z));

	//		origin[0] = atoi(x);
	//		origin[1] = atoi(y);
	//		origin[2] = atoi(z);

	//		trap_Argv(4, yaw, sizeof(yaw));
	//		angles[YAW] = atoi(yaw);

	//		AmTeleportPlayer(ent, origin, angles, droptofloor, race, qfalse);
	//	}

	//	else//Amtele other player to origin
	//	{
	//		if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
	//			if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
	//				return;
	//			else
	//				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
	//		}

	//		teleporter = &g_entities[clientid1];

	//		trap_Argv(2, x, sizeof(x));
	//		trap_Argv(3, y, sizeof(y));
	//		trap_Argv(4, z, sizeof(z));

	//		origin[0] = atoi(x);
	//		origin[1] = atoi(y);
	//		origin[2] = atoi(z);

	//		AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
	//	}
	//	return;

	//}

	//if (trap_Argc() == 6)//Amtele player to angles + origin
	//{
	//	trap_Argv(1, client1, sizeof(client1));
	//	clientid1 = JP_ClientNumberFromString(ent, client1);

	//	if (clientid1 == -1 || clientid1 == -2)
	//		return;

	//	if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
	//		if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
	//			return;
	//		else
	//			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
	//	}

	//	teleporter = &g_entities[clientid1];

	//	trap_Argv(2, x, sizeof(x));
	//	trap_Argv(3, y, sizeof(y));
	//	trap_Argv(4, z, sizeof(z));

	//	origin[0] = atoi(x);
	//	origin[1] = atoi(y);
	//	origin[2] = atoi(z);

	//	trap_Argv(5, yaw, sizeof(yaw));
	//	angles[YAW] = atoi(yaw);

	//	AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
	//	return;
	//}

	
}


qboolean atoi_real(const char* string) {
	size_t i;
	if (!*string) return qfalse;
	for (i = 0; string[i] != '\0'; ++i) {
		if (string[i] < '0' || string[i] > '9') {
			return qfalse;
		}
	}
	return qtrue;
}

void DF_TopRequest(gentity_t* ent, const char* coursename, const char* subcoursename, int page, int style, topRequestType_t type, mainLeaderboardType_t lbTypeIfSpecific, raceStyle_t* thisMapDefaultRaceStyle);

void DF_PrintSubCoursesToPlayer(gentity_t* ent) {
	int i;
	for (i = 0; i < level.numCourses; i++) { //32 max
		if (level.courseName[i] && level.courseName[i][0])
			trap_SendServerCommand(ent - g_entities, va("print \"  ^5%i ^7- ^3%s\n\"", i + 1, level.courseName[i]));
	}
}

void DF_PrintUnspecifiedCourseErrorToPlayer(gentity_t* ent) {

	if (level.numCourses != 1) {
		if (level.numCourses) {
			trap_SendServerCommand(ent - g_entities, "print \"This map has multiple courses, you might have to specify one of the following with /top <mapname> <subcoursename> <style (optional)> <page (optional)>.\n\"");
			DF_PrintSubCoursesToPlayer(ent);
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"This map appears to have no courses, you might have to specify one of the following with /top <mapname> <subcoursename> <style (optional)> <page (optional)>.\n\"");
		}
	}
}


void DF_TimeRequest(gentity_t* ent, const char* coursename, const char* subcoursename, int style, qboolean forUserinfo);
/*
=================
Cmd_Time_f
=================
*/
void Cmd_Time_f(gentity_t* ent) {
	DF_TimeRequest(ent,DF_GetCourseName(qfalse),ent->client->pers.lastSubcourseFinishedName,ent->client->sess.raceStyle.movementStyle,qfalse);
}

/*
=================
Cmd_Top_f
=================
*/
void Cmd_Top_f( gentity_t *ent )
{
	topRequestStruct_t data;
	qboolean mainCourseNameFound = qfalse;
	qboolean subCourseNameFound = qfalse;
	const int args = trap_Argc();
	int i,t;
	//int style = MV_JK2;
	//int page = 1;
	//int style = -1, page = -1, start = 0, input, i;
	char inputString[COURSENAME_MAX_LEN+1];
	char courseName[COURSENAME_MAX_LEN + 1] = { 0 };
	char subcourseName[COURSENAME_MAX_LEN + 1] = { 0 };
	char cmd[MAX_TOKEN_CHARS];
	const char* thisMapName = DF_GetCourseName(qfalse);
	const char* mainSubCourseName = DF_GetMainSubcourseName();

	data.page = 1;
	data.style = MV_JK2;

	data.type = TOPREQUEST_ALL;
	data.lbTypeIfSpecific = LB_MAIN; // just to shut the compiler up

	trap_Argv(0, cmd, sizeof(cmd));

	if (!Q_stricmp(cmd,"topmain")) {
		data.type = TOPREQUEST_SPECIFICLB;
		data.lbTypeIfSpecific = LB_MAIN;
	} else if (!Q_stricmp(cmd,"topnjb") || !Q_stricmp(cmd, "topnojumpbug")) {
		data.type = TOPREQUEST_SPECIFICLB;
		data.lbTypeIfSpecific = LB_NOJUMPBUG;
	} else if (!Q_stricmp(cmd,"topcustom")) {
		data.type = TOPREQUEST_SPECIFICLB;
		data.lbTypeIfSpecific = LB_CUSTOM;
	} else if (!Q_stricmp(cmd,"topsegmented") || !Q_stricmp(cmd, "topseg")) {
		data.type = TOPREQUEST_SPECIFICLB;
		data.lbTypeIfSpecific = LB_SEGMENTED;
	} else if (!Q_stricmp(cmd,"topcheat")) {
		data.type = TOPREQUEST_SPECIFICLB;
		data.lbTypeIfSpecific = LB_CHEAT;
	}

	if (args <= 1) {
		DF_PrintUnspecifiedCourseErrorToPlayer(ent);
		DF_TopRequest(ent, thisMapName, mainSubCourseName, data.page, data.style, data.type, data.lbTypeIfSpecific,&level.mapDefaultRaceStyle);
		return;
	}

	for (i = 1; i < args; i++) {
		trap_Argv(i, inputString, sizeof(inputString));
		if (atoi_real(inputString)) {
			//BUG - atoi(inputstring) returns true for values like "18percent" where it should return false..
			data.page = atoi(inputString);
		} else if ((t = RaceNameToInteger(inputString)) != -1) {
			data.style = t;
		}
		else {
			if (!mainCourseNameFound) {
				Q_strncpyz(courseName, inputString, sizeof(courseName));
				mainCourseNameFound = qtrue;
			}
			else {
				Q_strncpyz(subcourseName, inputString, sizeof(subcourseName));
				subCourseNameFound = qtrue;
			}
		}
	}

	if (!mainCourseNameFound) {
		DF_PrintUnspecifiedCourseErrorToPlayer(ent);
		DF_TopRequest(ent, thisMapName, mainSubCourseName, data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
		return;
	}
	else if (!subCourseNameFound){
		if (!Q_stricmp(courseName, thisMapName) && level.emptyNameCourseExists) {
			//if (level.emptyNameCourseExists) {
			//	DF_TopRequest(ent, thisMapName, "", data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
			//}
			//else if (level.numCourses == 1) {
			//	DF_TopRequest(ent, thisMapName, mainSubCourseName, data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
			//	DF_TopRequest(ent, thisMapName, mainSubCourseName, data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
			//} // DF_GetMainSubcourseName does the same thing anway
			DF_TopRequest(ent, thisMapName, mainSubCourseName, data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
			return;
		}

		// check if its a subcourse of the current map
		// if someone specifies exactly, we can avoid one DB call to find fitting maps
		for (i = 0; i < level.numCourses; i++) { //32 max
			if (!Q_stricmp(level.courseName[i], courseName)) {
				DF_TopRequest(ent, thisMapName, level.courseName[i], data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
				return;
			}
		}
	}
	else if(!Q_stricmp(courseName, thisMapName)){
		// check if its a subcourse of the current map
		// if someone specifies exactly, we can avoid one DB call to find fitting maps
		for (i = 0; i < level.numCourses; i++) { //32 max
			if (!Q_stricmp(level.courseName[i], subcourseName)) {
				DF_TopRequest(ent, thisMapName, level.courseName[i], data.page, data.style, data.type, data.lbTypeIfSpecific, &level.mapDefaultRaceStyle);
				return;
			}
		}
	}

	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));

	if (mainCourseNameFound && subCourseNameFound) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_TOPMAPSEARCH,
			"SET @search = ?,@subsearch=?;"
			"SELECT runs.course,runs.subcourse " 
			",instr(runs.course,@search) +instr(REVERSE(runs.course),REVERSE(@search))-2 AS diff "
			",instr(runs.subcourse,@subsearch) +instr(REVERSE(runs.subcourse),REVERSE(@subsearch))-2 AS diff2 "
			",ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound,mapdefaults.msec,mapdefaults.jump,mapdefaults.runFlags "
			"FROM runs "
			"LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) "
			"GROUP BY runs.course,runs.subcourse HAVING instr(runs.course, @search) AND instr(runs.subcourse, @subsearch) "
			"ORDER BY diff+diff2" // order stuff nicely and logically. best match comes first
		)) {
			return;
		}
		G_COOL_API_DB_PreparedBindString(courseName);
		G_COOL_API_DB_PreparedBindString(subcourseName);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_TOPMAPSEARCH, 
			"SET @search = ?;"
			"SELECT runs.course,runs.subcourse " 
			",instr(runs.course,@search) +instr(REVERSE(runs.course),REVERSE(@search))-2 AS diff "
			",instr(runs.subcourse,@search) +instr(REVERSE(runs.subcourse),REVERSE(@search))-2 AS diff2 "
			",ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound,mapdefaults.msec,mapdefaults.jump,mapdefaults.runFlags "
			"FROM runs "
			"LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) "
			"GROUP BY runs.course,runs.subcourse HAVING instr(runs.course, @search) OR instr(runs.subcourse, @search) "
			"ORDER BY CASE " // order stuff nicely and logically. best match comes first
			"WHEN diff = -2 AND diff2 != -2 THEN diff2 "
			"WHEN diff2 = -2 AND diff != -2 THEN diff "
			"ELSE IF(diff<diff2,diff,diff2) "
			"END"
			", LENGTH(runs.course), LENGTH(runs.subcourse)"
		)) {
			return;
		}
		G_COOL_API_DB_PreparedBindString(courseName);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}



	//DF_TopRequest(ent, thisMapName,"");


	/*
	topScoresRequestStruct_t data;
	int countLBs = LB_TYPES_COUNT;
	const char* mainLBWhere = getLeaderboardSQLConditions(LB_MAIN, &level.mapDefaultRaceStyle);
	const char* mainLBNJBWhere = getLeaderboardSQLConditions(LB_NOJUMPBUG, &level.mapDefaultRaceStyle);
	const char* customLBWhere = getLeaderboardSQLConditions(LB_CUSTOM, &level.mapDefaultRaceStyle);
	const char* segmentedLBWhere = getLeaderboardSQLConditions(LB_SEGMENTED, &level.mapDefaultRaceStyle);
	const char* cheatLBWhere = getLeaderboardSQLConditions(LB_CHEAT, &level.mapDefaultRaceStyle);
	const char* courseName = DF_GetCourseName();
	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(data.clientnum, "print \"Top results request failed, database version too low.\n\"");
		return;
	}

#define TOPCOLUMNS "users.username,runs_pre.besttime,runs_pre.userid, runs_pre.runFlags, msec, jump"
//#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime,MIN(runwhen) OVER (PARTITION BY userid) AS earliest FROM runs  WHERE course=? AND style=? AND variant=? AND %s ) runs_pre"
#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime FROM runs  WHERE course=? AND subcourse=? AND style=? AND variant=? AND %s ) runs_pre"
//#define QUERY2 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id WHERE earliest=runwhen AND besttime=duration_ms GROUP BY userid ORDER BY besttime ASC LIMIT 11"
#define QUERY2 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id WHERE besttime=duration_ms GROUP BY userid ORDER BY besttime ASC LIMIT 11"

	// TODO what if, for freak reason, someone has two identical times in two different styles? how do i select the earlier one? or should i even care?  earliest=runwhen AND besttime=duration_ms doesnt work cuz not both are neccessarily true
	
	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	if (G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_TOP,
		va(
			"(SELECT 0 AS type," TOPCOLUMNS QUERY2 " )" // limit 11 cuz want unofficial too, even tho we show it separately.
			"UNION ALL (SELECT 1 AS type," TOPCOLUMNS QUERY2 " )"
			"UNION ALL (SELECT 2 AS type," TOPCOLUMNS QUERY2 " )"
			"UNION ALL (SELECT 3 AS type," TOPCOLUMNS QUERY2 " )"
			"UNION ALL (SELECT 4 AS type," TOPCOLUMNS QUERY2 " )"
			, mainLBWhere, mainLBNJBWhere, customLBWhere, segmentedLBWhere, cheatLBWhere))) {
		int i;
		for (i = 0; i < countLBs; i++) {
			G_COOL_API_DB_PreparedBindString(courseName);
			G_COOL_API_DB_PreparedBindString("");// subcourse
			G_COOL_API_DB_PreparedBindInt((int)MV_JK2);
			G_COOL_API_DB_PreparedBindInt(0);
		}
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		trap_SendServerCommand(data.clientnum, "print \"Top results request failed, database connection not available.\n\"");
	}
	*/

}

/*
=================
Cmd_Top_f
=================
*/
void Cmd_UpdateRanks_f( gentity_t *ent )
{
	const char* thisMapName = DF_GetCourseName(qfalse);
	const char* mainSubCourseName = DF_GetMainSubcourseName();
	char arg[10];
	qboolean all = qfalse;
	qboolean forceAll = qfalse;

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_UPDATERANKS)) {
		trap_SendServerCommand(ent-g_entities,"print \"You don't have permissions to execute this command.\n\"");
		return;
	}

	if (!level.mapDefaultsConfirmed) {
		trap_SendServerCommand(ent - g_entities, "print \"Cannot run rank update. Level map defaults are not confirmed.\n\"");
		return;
	}

	if (trap_Argc() > 1) {
		trap_Argv(1,arg,sizeof(arg));
		if (!Q_stricmp(arg,"all")) {
			all = qtrue;
		}
		else if (!Q_stricmp(arg,"forceall")) {
			all = qtrue;
			forceAll = qtrue;
		}
	}
	DF_UpdateRanksMainRequest(ent, all ? NULL : DF_GetCourseName(qfalse), forceAll, 0);

}
void Cmd_DemoManage_f( gentity_t *ent )
{
	char arg[100];
	qboolean checkScript = qfalse;

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_DEMOMANAGE)) {
		trap_SendServerCommand(ent-g_entities,"print \"You don't have permissions to execute this command.\n\"");
		return;
	}

	if (trap_Argc() > 1) {
		trap_Argv(1,arg,sizeof(arg));
		if (!Q_stricmp(arg,"checkScript")) {
			checkScript = qtrue;
		}
	}

	if (checkScript) {

		demoCheckScriptRequest_t data;
		data.clientnum = ent - g_entities;
		memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
		if (trap_Argc() > 2) {
			trap_Argv(2, arg, sizeof(arg));
			if (arg[0]) {
				Com_sprintf(data.outScriptName, sizeof(data.outScriptName), "%s", arg);
			}
			else {
				Q_strncpyz(data.outScriptName, "checkDemos", sizeof(data.outScriptName));
			}
		}
		else {
			Q_strncpyz(data.outScriptName, "checkDemos", sizeof(data.outScriptName));
		}
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_DEMOCHECK_GETALLRUNS,
			"SELECT runs.userid, users.username,runs.course, runs.subcourse, runs.`style`, runs.variant,runs.msec,runs.jump,runs.runFlags  FROM runs LEFT JOIN users ON (runs.userid =users.id) "
		)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Cannot request runs for democheck script generation.\n\"");
			return;
		}
		G_COOL_API_DB_FinishAndSendPreparedStatement();
		return;
	}


}

//[JAPRO - Serverside - All - Ampsay Function - Start]
/*
=================
Cmd_Ammap_f
=================
*/
void Cmd_AmMap_f(gentity_t* ent)
{
	char    gametype[2];
	int		gtype;
	char    mapname[256];


	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_CHANGEMAP)) {
		trap_SendServerCommand(ent - g_entities, "print \"You don't have permissions to execute this command.\n\"");
		return;
	}


	if (trap_Argc() != 3)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amMap <gametype #> <map>.\n\"");
		return;
	}

	trap_Argv(1, gametype, sizeof(gametype));
	trap_Argv(2, mapname, sizeof(mapname));

	if (strchr(mapname, ';') || strchr(mapname, '\r') || strchr(mapname, '\n'))
	{
		trap_SendServerCommand(ent - g_entities, "print \"Invalid map string.\n\"");
		return;
	}

	if (gametype[0] < '0' && gametype[0] > '8')
	{
		trap_SendServerCommand(ent - g_entities, "print \"Invalid gametype.\n\"");
		return;
	}

	gtype = atoi(gametype);

	{
		static char				unsortedMaps[32768];
		char* possibleMapName;
		int					numMaps;
		//const unsigned int  MAX_MAPS = 4096;
		qboolean found = qfalse;

		numMaps = trap_FS_GetFileList("maps", ".bsp", unsortedMaps, sizeof(unsortedMaps));
		if (numMaps) {
			int len, i;
			//if (numMaps > MAX_MAPS)
			//	numMaps = MAX_MAPS;
			possibleMapName = unsortedMaps;
			for (i = 0; i < numMaps; i++) {
				len = strlen(possibleMapName);
				if (!Q_stricmp(possibleMapName + len - 4, ".bsp"))
					possibleMapName[len - 4] = '\0';
				if (!Q_stricmp(mapname, possibleMapName)) {
					found = qtrue;
				}
				possibleMapName += len + 1;
			}
		}
		if (!found)
			return;
	}

	//if (ent->client->sess.juniorAdmin)//Logged in as junior admin
	trap_SendServerCommand(-1, va("print \"^3Map change triggered by ^7%s\n\"", ent->client->pers.netname));
	G_LogPrintf("Map change triggered by ^7%s\n", ent->client->pers.netname);

	trap_SendConsoleCommand(EXEC_APPEND, va("g_gametype %i\n", gtype));
	trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", mapname));

}
//[JAPRO - Serverside - All - Ammap Function - End]




void Cmd_MapRatings_f(gentity_t* ent) {
	char arg[10];

	// TODO...

	if (trap_Argc() > 1) {
		trap_Argv(1, arg, sizeof(arg));
		if (!Q_stricmp(arg, "top")) {
			/// top maps
			return;
		} 
	}


}

void Cmd_Maplist_f(gentity_t* ent) {

	int			mapsinmessage = 0;
	const char*	type = NULL;
	char		currentMap[COURSENAME_MAX_LEN+1];
	qboolean	first = qtrue;
	int			n = 0;

	if (trap_Argc() > 1) {
		char arg[10];
		trap_Argv(1, arg, sizeof(arg));
		if (!Q_stricmp(arg,"unplayed")) {
			if (!ent->client->sess.login.loggedIn) {
				trap_SendServerCommand(ent - g_entities, "print \"Cannot display unplayed maps unless you are logged in.\n\"");
				return;
			}
			else {
				maplistUnplayedRequestStruct_t data;
				data.clientnum = ent - g_entities;
				memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
				if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_MAPLISTUNPLAYED,
					"SELECT runs.course,runs2.userid FROM runs LEFT JOIN runs AS runs2 ON (runs.course=runs2.course AND runs2.userid=?) GROUP BY runs.course HAVING runs2.userid IS NULL ORDER BY course ASC"
				)) {
					trap_SendServerCommand(ent - g_entities, "print \"^1Cannot display unplayed maps - database error.\n\"");
					return;
				}
				G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
				G_COOL_API_DB_FinishAndSendPreparedStatement();
				return;
			}
		}
	}

	G_BufferedSendOrPrint(ent, qfalse, qfalse, "^2----------^7INSTALLED MAPS^2---------\n",qfalse);

	for (n = 0; n < g_numArenas; n++) {

		type = g_arenaInfosHashed[n].name; 

		if (strlen(type) < 1 || !Q_stricmp(type, "<NULL>")) {

			if (n == (g_numArenas - 1)) {
				G_BufferedSendOrPrint(ent, qfalse, qfalse, "\n", qfalse);
				mapsinmessage = 0;
			}
			continue;
		}

		Q_strncpyz(currentMap, type, 24);
		G_BufferedSendOrPrint(ent, qfalse, qfalse, va("^7[^2%03i^7] %-24s", n, currentMap), qfalse);

		mapsinmessage++;

		if ((mapsinmessage >= 5) || (n == (g_numArenas - 1))) {
			G_BufferedSendOrPrint(ent, qfalse, qfalse, "\n", qfalse);
			mapsinmessage = 0;
		}
	}
	G_BufferedSendOrPrint(ent, qfalse, qfalse, "\nWhen logged in, you can call ^2/maplist unplayed^7 to see maps that were finished by other people that you haven't played yet.\n", qfalse);
	G_BufferedSendOrPrintFlush(ent, qfalse, qfalse);

}

void Cmd_Latest_f(gentity_t* ent) {
	int clientNum = -1;
	int page, first;
	int i,t;
	int style = -1;
	latestRunsRequestStruct_t data;
	//char pageNum[10];
	const int args = trap_Argc();
	char inputString[15];

	memset(&data, 0, sizeof(data));
	
	data.userId = -2;

	if (!coolApi_dbVersion) {
		trap_SendServerCommand(ent-g_entities,"print \"^1latest not possible, DB API not available\n\"");
		return;
	}

	data.clientnum = ent - g_entities;
	memcpy(&data.ip, &mv_clientSessions[data.clientnum], sizeof(data.ip));

	page = 0;
	if (trap_Argc() > 1) {
		for (i = 1; i < args; i++) {
			trap_Argv(i, inputString, sizeof(inputString));
			if (atoi_real(inputString)) {
				//BUG - atoi(inputstring) returns true for values like "18percent" where it should return false..
				page = atoi(inputString);
				data.pageSpecified = qtrue;
			}
			else if ((t = RaceNameToInteger(inputString)) != -1) {
				style = t;
				data.styleSpecified = qtrue;
			}
			else if (!Q_stricmp(inputString,"mine")) {
				if (!ent->client->sess.login.loggedIn) {
					trap_SendServerCommand(ent - g_entities, "print \"Cannot show your latest runs because you are not logged in.\n\"");
					return;
				}
				data.userId = ent->client->sess.login.id;
			}
			else if(!Q_stricmp(inputString, "unlogged")) {
				data.userId = -1;
			}
			else if(*inputString){
				Q_strncpyz(data.userSearchTerm, inputString, sizeof(data.userSearchTerm));
				data.userId = -3;
			}
		}
	}
	else {
		page = 0;
	}
	page = MAX(page-1, 0);
	first = page * 10;

#define LATESTQUERY "SELECT runs.userid,users.username,runs.course,runs.subcourse,runs.style,runs.msec,runs.jump,runs.variant,runs.runflags,ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound,mapdefaults.msec,mapdefaults.jump,mapdefaults.variant,mapdefaults.runFlags,runs.duration_ms,runs.runwhen,runs.tmpRank FROM runs LEFT JOIN users ON (users.id = runs.userid) LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) "
#define LATESTQUERY_STYLEWUERE " runs.style=? "
#define LATESTQUERY_USERWUERE " runs.userid=@userid "
#define LATESTQUERY_END " ORDER BY runs.runwhen DESC  LIMIT ?,10"

	if (style == -1) {

		if (data.userId != -2) {
			data.userResults = qtrue;
			if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_GETLATESTRUNS,
				data.userId == -3
				? USERIDQUERY_USERSEARCH LATESTQUERY " WHERE " LATESTQUERY_USERWUERE LATESTQUERY_END
				: USERIDQUERY_USERID LATESTQUERY " WHERE " LATESTQUERY_USERWUERE LATESTQUERY_END
			)) {
				trap_SendServerCommand(ent - g_entities, "print \"^1Latest runs cannot be displayed. Database request failed.\n\"");
				return;
			}
			if (data.userId == -3) {
				G_COOL_API_DB_PreparedBindString(data.userSearchTerm);
			}
			else {
				G_COOL_API_DB_PreparedBindString(data.userId == -1 ? "" : ent->client->sess.login.name);
				G_COOL_API_DB_PreparedBindInt(data.userId);
			}
		}
		else {
			data.userResults = qfalse;
			if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_GETLATESTRUNS, LATESTQUERY LATESTQUERY_END)) {
				trap_SendServerCommand(ent - g_entities, "print \"^1Latest runs cannot be displayed. Database request failed.\n\"");
				return;
			}
		}
	}
	else {
		if (data.userId != -2) {
			data.userResults = qtrue;
			if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_GETLATESTRUNS,
				data.userId == -3
				? USERIDQUERY_USERSEARCH LATESTQUERY  " WHERE " LATESTQUERY_USERWUERE  " AND " LATESTQUERY_STYLEWUERE LATESTQUERY_END
				: USERIDQUERY_USERID LATESTQUERY  " WHERE " LATESTQUERY_USERWUERE  " AND " LATESTQUERY_STYLEWUERE LATESTQUERY_END
			)) {
				trap_SendServerCommand(ent - g_entities, "print \"^1Latest runs cannot be displayed. Database request failed.\n\"");
				return;
			}
			if (data.userId == -3) {
				G_COOL_API_DB_PreparedBindString(data.userSearchTerm);
			}
			else {
				G_COOL_API_DB_PreparedBindString(data.userId == -1 ? "" : ent->client->sess.login.name);
				G_COOL_API_DB_PreparedBindInt(data.userId);
			}
			G_COOL_API_DB_PreparedBindInt(style);
		}
		else {
			data.userResults = qfalse;
			if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_GETLATESTRUNS, LATESTQUERY  " WHERE "  LATESTQUERY_STYLEWUERE LATESTQUERY_END)) {
				trap_SendServerCommand(ent - g_entities, "print \"^1Latest runs cannot be displayed. Database request failed.\n\"");
				return;
			}
			G_COOL_API_DB_PreparedBindInt(style);
		}
	}

	G_COOL_API_DB_PreparedBindInt(first);
	G_COOL_API_DB_FinishAndSendPreparedStatement();

}

void G_SendPlayerMapRatingsUIInfo(gentity_t* ent) {
	char cmd[MAX_STRING_CHARS];
	char addbit[20];
	int i,j;
	int len = 0;
	if (!ent->client) {
		return;
	}
	if (!(ent->client->pers.ttClientFlags & TTFLAGS_CLIENT_SUPPORTS_TTCMD)) {
		return;
	}
	if (!ent->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent-g_entities,"ttCmd rateMapState 0");
		return;
	}
	Q_strncpyz(cmd, "ttCmd rateMapState 1 ", sizeof(cmd));
	len = strlen(cmd);
	for (i = 0; i < MV_NUMSTYLES; i++) {
		cmd[len++] = ent->client->pers.mapRatings[i].rated ? '1' : '0';
	}
	for (i = 0; i < MV_NUMSTYLES; i++) {
		if (!ent->client->pers.mapRatings[i].rated) {
			continue;
		}
		Com_sprintf(addbit,sizeof(addbit)," %.4f", ent->client->pers.mapRatings[i].rating);

		j = strlen(addbit);
		if (len + j > 1022)
			break;

		Q_strncpyz(cmd + len, addbit, sizeof(cmd) - len);
		len += j;
	}

	cmd[len] = '\0';
	trap_SendServerCommand(ent - g_entities, cmd);

}

static qboolean QDECL PlayerMapRatingsFetchResult(gentity_t* ent, genericDbRequestStruct_t* data) {
	memset(&ent->client->pers.mapRatings, 0, sizeof(ent->client->pers.mapRatings));
	if (!ent->client || !ent->client->sess.login.loggedIn) {
		return qfalse;
	}
	while (G_COOL_API_DB_NextRow()) {
		int style = G_COOL_API_DB_GetInt(0);
		float rating;
		G_COOL_API_DB_GetFloat(1,&rating);
		if (style >= 0 && style < MV_NUMSTYLES) {
			ent->client->pers.mapRatings[style].rated = qtrue;
			ent->client->pers.mapRatings[style].rating = rating;
		}
	}
	G_SendPlayerMapRatingsUIInfo(ent);
	return qtrue;
}
REGISTER_DBREQUEST_CALLBACK(GDBREQUEST_MAPRATINGSFETCH, PlayerMapRatingsFetchResult);
void G_CheckPlayerMapRatings(gentity_t* ent) {
	genericDbRequestStruct_t data = G_DB_GenericRequest_Prepare(ent, GDBREQUEST_MAPRATINGSFETCH, (1 << DBT_MAPRATINGS), "mapratingsfetch", 0);
	if (!ent->client || !ent->client->sess.login.loggedIn) {
		return;
	}
	if (!G_DB_GenericRequest_Send(data, "SELECT style, rating FROM mapratings WHERE userid=%d AND course=%s ORDER BY style ASC", ent->client->sess.login.id, DF_GetCourseName(qfalse))) {
		trap_SendServerCommand(ent - g_entities, "print \"Error sending user map ratings request.\n\"");
	}
}

void Cmd_RateMap_f(gentity_t* ent) {
	char arg[30];
	char* str;
	rateMapStruct_t data;

	if (!ent->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent - g_entities, "print \"You can't rate maps without being logged in.\n\"");
		return;
	}

	memset(&data, 0, sizeof(data));

	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));

	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent-g_entities,"print \"Usage: ratemap <style> <rating>; rating must be a number from 0 to 10, and can be a fractional number\n\"");
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_RATEMAPSHOWMINE,
			//"REPLACE INTO mapratings (course,userid,style,rating) VALUES (?,?,?,?)"
			"SELECT style,rating FROM mapratings WHERE course=? AND userid=? ORDER BY style ASC"
		)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error fetching own ratings of current map: Database error, query could not be sent.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindString(DF_GetCourseName(qfalse));
		G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);

		G_COOL_API_DB_FinishAndSendPreparedStatement();

		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	data.style = RaceNameToInteger(arg);
	if (data.style == -1) {
		trap_SendServerCommand(ent - g_entities, va("print \"Style '%s' not recognized.\n\"",arg));
		trap_SendServerCommand(ent - g_entities, "print \"Usage: ratemap <style> <rating>; rating must be a number from 0 to 10, and can be a fractional number\n\"");
		return;
	}
	trap_Argv(2, arg, sizeof(arg));
	str = arg;
	while (*str) {
		if (*str == ',') {
			*str = '.'; // normalize fractional numbers
		}
		if (*str != '.' && !(*str >= '0' && *str <= '9')) {
			trap_SendServerCommand(ent - g_entities, va("print \"Character '%c' is not supported for rating value.\n\"", *str));
			trap_SendServerCommand(ent - g_entities, "print \"Usage: ratemap <style> <rating>; rating must be a number from 0 to 10, and can be a fractional number\n\"");
			return;
		}
		str++;
	}
	data.value = atof(arg);
	if (data.value < 0 || data.value > 10) {
		trap_SendServerCommand(ent - g_entities, va("print \"Rating value %f is out of range. Must be 0-10.\n\"", data.value));
		trap_SendServerCommand(ent - g_entities, "print \"Usage: ratemap <style> <rating>; rating must be a number from 0 to 10, and can be a fractional number\n\"");
		return;
	}

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_RATEMAP,
		//"REPLACE INTO mapratings (course,userid,style,rating) VALUES (?,?,?,?)"
		"INSERT INTO mapratings (course,userid,style,rating) VALUES (?,?,?,?) ON DUPLICATE KEY UPDATE rating=?"
	)) {
		trap_SendServerCommand(ent - g_entities, "print \"Error rating map: Database error, query could not be sent.\n\"");
		return;
	}
	G_COOL_API_DB_PreparedBindString(DF_GetCourseName(qfalse));
	G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
	G_COOL_API_DB_PreparedBindInt(data.style);
	G_COOL_API_DB_PreparedBindFloat(data.value);
	G_COOL_API_DB_PreparedBindFloat(data.value);

	G_COOL_API_DB_FinishAndSendPreparedStatement();

}

void Cmd_MapSearch_f(gentity_t* ent) {
	int clientNum = -1;
	int page, first;
	int i,t;
	mapSearchRequestStruct_t data;
	//char pageNum[10];
	const int args = trap_Argc();
	char inputString[15];
	char cmd[15];
	trap_Argv(0,cmd,sizeof(cmd));
		
	memset(&data, 0, sizeof(data));
	data.userSearchTerm[0] = '\0';
	
	data.type = MAPSEARCH_SHORTEST;
	if (!Q_stricmp(cmd, "longest")) {
		data.type = MAPSEARCH_LONGEST;
	}
	else if (!Q_stricmp(cmd, "mostplayed")) {
		data.type = MAPSEARCH_MOSTPLAYED;
	}
	else if (!Q_stricmp(cmd, "toprated")) {
		data.type = MAPSEARCH_TOPRATED;
	}
	else if (!Q_stricmp(cmd, "notwr")) {
		data.type = MAPSEARCH_NOTWR;
	}
	else if (!Q_stricmp(cmd, "wrs")) {
		data.type = MAPSEARCH_WR;
	}
	else if (!Q_stricmp(cmd, "hardest")) {
		data.type = MAPSEARCH_HARDEST;
	}
	else if (!Q_stricmp(cmd, "easiest")) {
		data.type = MAPSEARCH_EASIEST;
	}

	if (!coolApi_dbVersion) {
		trap_SendServerCommand(ent-g_entities,"print \"^1Longest/shortest request not possible, DB API not available\n\"");
		return;
	}

	data.clientnum = ent - g_entities;
	memcpy(&data.ip, &mv_clientSessions[data.clientnum], sizeof(data.ip));

	page = 0;
	if (trap_Argc() > 1) {
		for (i = 1; i < args; i++) {
			trap_Argv(i, inputString, sizeof(inputString));
			if (atoi_real(inputString)) {
				//BUG - atoi(inputstring) returns true for values like "18percent" where it should return false..
				page = atoi(inputString);
				data.pageSpecified = qtrue;
			}
			else if ((t = RaceNameToInteger(inputString)) != -1) {
				data.style = t;
				data.styleSpecified = qtrue;
			}
			else if ((t = LeaderboardNameToInteger(inputString)) != -1) {
				data.lbType = t;
				data.lbTypeSpecified = qtrue;
			}
			else {
				Q_strncpyz(data.userSearchTerm, inputString,sizeof(data.userSearchTerm));
			}
		}
	}
	else {
		page = 0;
	}
	page = MAX(page-1, 0);
	first = page * 10;


	if (!ent->client->sess.login.loggedIn && !*data.userSearchTerm && (data.type == MAPSEARCH_NOTWR || data.type == MAPSEARCH_WR)) {
		trap_SendServerCommand(ent - g_entities, "print \"Gotta be logged in to use ^2/notwr^7,^2/wrs^7; or specify a user search term.\n\"");
		return;
	}

	// TODO more distinction? avoid segmented times? idk

	if (data.style == -1) {
		data.style = MV_JK2;
	}

#define LONGESTSHORTESTQUERY "SELECT MIN(duration_ms) as fastest,runs.course,runs.subcourse FROM runs LEFT JOIN mapdefaults ON (mapdefaults.course = runs.course AND mapdefaults.subcourse = runs.subcourse) WHERE hidden=0 AND style = ? AND (runs.jump = mapdefaults.jump OR (mapdefaults.jump IS NULL AND runs.jump = 1)) GROUP BY runs.course,runs.subcourse ORDER BY fastest "
#define MOSTPLAYEDQUERY "SELECT COUNT(DISTINCT userid) as playerCount, runs.course, runs.subcourse FROM runs WHERE hidden=0 AND style = ? GROUP BY runs.course, runs.subcourse ORDER BY playerCount DESC "
#define TOPRATEDQUERY "SELECT AVG(rating) AS avgRating,COUNT(DISTINCT userid) ratingCount, course FROM mapratings WHERE style=? GROUP BY course,style ORDER BY avgRating DESC "
#define WRORNOTWRQUERY "SELECT runs.course,runs.subcourse,COUNT(subruns.userid) >0 AS anyruns,MIN(subruns.tmpRank) AS bestrank, COUNT(DISTINCT subruns2.userid) AS playerCount, AVG(mapratings.rating) AS rating, COUNT(DISTINCT mapratings.userid) AS ratingCount, mapratings2.rating as myRating, mapratings2.rating IS NOT NULL AS haveMyRating, MIN(subruns2.duration_ms) AS fastestTime  FROM runs \
	LEFT JOIN runs AS subruns ON(subruns.hidden=0 AND subruns.userid = @userid AND subruns.course = runs.course AND subruns.subcourse = runs.subcourse AND subruns.style = ? AND subruns.tmpLB = ?) \
	LEFT JOIN runs AS subruns2 ON(subruns2.hidden=0 AND subruns2.course = runs.course AND subruns2.subcourse = runs.subcourse AND subruns2.style = ? AND subruns2.tmpLB = ?) \
	LEFT JOIN mapratings ON (mapratings.course=runs.course AND mapratings.style=?)\
	LEFT JOIN mapratings AS mapratings2 ON (mapratings2.course=runs.course AND mapratings2.style=? AND mapratings2.userid=@userid)\
	WHERE runs.hidden=0 \
		GROUP BY course, subcourse  "
#define WRORNOTWRQUERY_END " ORDER BY anyruns DESC, bestrank ASC, playerCount DESC,rating DESC, runs.course ASC "
#define NOTWRQUERY WRORNOTWRQUERY " HAVING bestrank > 1 OR anyruns = 0 " WRORNOTWRQUERY_END
#define WRQUERY WRORNOTWRQUERY " HAVING bestrank = 1 " WRORNOTWRQUERY_END

	// TODO can we simplify this? LOL
	// TODO dont use actual best time as basis for deviation but rather the best time's holder's average general deviation multiplied with his best time?
#define HARDESTEASIESTQUERY "SET @style=?; \
 \
SELECT course,subcourse,AVG(playerDevDev)*100 AS avgdevdev, best, COUNT(*) as samples FROM \
( \
	SELECT users.id,users.username,runs.course,runs.subcourse,MIN(runs.duration_ms) AS pb,MIN(subruns.duration_ms) AS best, MIN(runs.duration_ms) /MIN(subruns.duration_ms) AS dev,avgDev as playerAvgDev,(MIN(runs.duration_ms) /MIN(subruns.duration_ms))/avgDev as playerDevDev, COUNT(DISTINCT subruns.userid) AS players, COUNT(userDevs.id) as samples  \
	FROM users  \
	CROSS JOIN runs ON (runs.hidden=0 AND users.id = runs.userid AND runs.style=@style AND runs.tmpLB=0) \
	LEFT JOIN runs as subruns ON (subruns.hidden= 0 AND subruns.course=runs.course AND subruns.subcourse=runs.subcourse AND subruns.style=runs.style AND subruns.tmpLB=0) \
	LEFT JOIN ( \
		SELECT username,id,AVG(dev) AS avgDev,COUNT(*) as samples \
		FROM ( \
			SELECT users.id,users.username,runs.course,runs.subcourse,MIN(runs.duration_ms) AS pb,MIN(subruns.duration_ms) AS best, MIN(runs.duration_ms) /MIN(subruns.duration_ms) AS dev, COUNT(DISTINCT subruns.userid) AS players FROM users  \
			CROSS JOIN runs ON (runs.hidden= 0 AND users.id = runs.userid AND runs.style=@style AND runs.tmpLB=0) \
			LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) \
			LEFT JOIN runs as subruns ON (subruns.hidden=0 AND subruns.course=runs.course AND subruns.subcourse=runs.subcourse AND (subruns.jump=mapdefaults.jump OR (subruns.jump=1 AND mapdefaults.jump IS NULL)) AND subruns.style=@style AND subruns.tmpLB=0) \
			WHERE users.id != -1 \
			GROUP BY users.id,runs.course,runs.subcourse \
			HAVING players>=4 \
			ORDER BY runs.course,runs.subcourse \
		) usermapPerformance \
		GROUP BY usermapPerformance.id \
		HAVING samples >= 3 \
		ORDER BY avgDev ASC \
	) AS userDevs ON (userDevs.id=users.id) \
	WHERE users.id != -1 \
	GROUP BY users.id,runs.course,runs.subcourse \
	HAVING players>=4 AND playerDevDev IS NOT NULL \
	ORDER BY runs.course,runs.subcourse \
) mapUserDeviations \
GROUP BY course,subcourse \
HAVING samples>=3 \
ORDER BY avgdevdev "

#define PAGINGLIMIT " LIMIT ?,10"

	if (data.type == MAPSEARCH_MOSTPLAYED) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, MOSTPLAYEDQUERY PAGINGLIMIT)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Most played maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else if (data.type == MAPSEARCH_TOPRATED) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, TOPRATEDQUERY PAGINGLIMIT)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Top rated maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else if (data.type == MAPSEARCH_LONGEST) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, LONGESTSHORTESTQUERY "DESC" PAGINGLIMIT)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Longest maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else if (data.type == MAPSEARCH_HARDEST || data.type == MAPSEARCH_EASIEST) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, 
			data.type == MAPSEARCH_HARDEST 
			? HARDESTEASIESTQUERY " DESC " PAGINGLIMIT
			: HARDESTEASIESTQUERY " ASC " PAGINGLIMIT
		)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Hardest/easiest maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else if (data.type == MAPSEARCH_SHORTEST) {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, LONGESTSHORTESTQUERY "ASC" PAGINGLIMIT)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Shortest maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else if (data.type == MAPSEARCH_NOTWR || data.type == MAPSEARCH_WR) {
		qboolean requestSuccess;
		if (*data.userSearchTerm) {
			if (requestSuccess = G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH,
				data.type == MAPSEARCH_NOTWR
				? USERIDQUERY_USERSEARCH NOTWRQUERY PAGINGLIMIT
				: USERIDQUERY_USERSEARCH WRQUERY PAGINGLIMIT
			)) {
				G_COOL_API_DB_PreparedBindString(data.userSearchTerm);
			}
		}
		else {
			if (requestSuccess = G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_MAPSEARCH, 
				data.type == MAPSEARCH_NOTWR 
				? USERIDQUERY_USERID NOTWRQUERY PAGINGLIMIT
				: USERIDQUERY_USERID WRQUERY PAGINGLIMIT

			)) {
				G_COOL_API_DB_PreparedBindString(ent->client->sess.login.name);
				G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
			}
		}
		if (!requestSuccess) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Non-WR maps cannot be displayed. Database request failed.\n\"");
			return;
		}
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(data.lbType);
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(data.lbType);
		G_COOL_API_DB_PreparedBindInt(data.style);
		G_COOL_API_DB_PreparedBindInt(data.style);
		//G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
		G_COOL_API_DB_PreparedBindInt(first);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}

}
void Cmd_Rank_f(gentity_t* ent) {
	//int clientNum = -1;
	int page, first;
	int i,t;
	rankRequestStruct_t data;
	const int args = trap_Argc();
	char inputString[15];
	
	memset(&data, 0, sizeof(data));
	
	if (!coolApi_dbVersion) {
		trap_SendServerCommand(ent-g_entities,"print \"^1Rank request not possible, DB API not available\n\"");
		return;
	}

	data.clientnum = ent - g_entities;
	memcpy(&data.ip, &mv_clientSessions[data.clientnum], sizeof(data.ip));

	page = 0;
	if (trap_Argc() > 1) {
		for (i = 1; i < args; i++) {
			trap_Argv(i, inputString, sizeof(inputString));
			if (atoi_real(inputString)) {
				//BUG - atoi(inputstring) returns true for values like "18percent" where it should return false..
				page = atoi(inputString);
				data.pageSpecified = qtrue;
			}
			else if ((t = RaceNameToInteger(inputString)) != -1) {
				data.style = t;
				data.styleSpecified = qtrue;
			}
			else if ((t = LeaderboardNameToInteger(inputString)) != -1) {
				data.lbType = t;
				data.lbTypeSpecified = qtrue;
			}
		}
	}
	else {
		page = 0;
	}
	page = MAX(page-1, 0);
	first = page * 10;


	// TODO more distinction? avoid segmented times? idk

	if (data.style == -1) {
		data.style = MV_JK2;
	}

#define RANKREQUEST "SELECT username,SUM(tmpRank=1) as golds,SUM(tmpRank=2) as silvers,SUM(tmpRank=3) as bronzes, ROW_NUMBER() OVER (ORDER BY golds DESC) AS realrank, MAX(goldtime) as mostRecentGold \
FROM ( \
SELECT username,users.id,runs.style,runs.tmpLB,runs.tmpRank,SUM(tmpRank) OVER (PARTITION BY runs.userid,runs.style,runs.tmpRank,runs.tmpLB) AS rankSum, IF(runs.tmpRank=1,runs.runwhen,NULL) AS goldtime \
FROM users \
LEFT JOIN runs ON (runs.hidden=0 AND runs.userid=users.id  ) \
WHERE style=? AND tmpLB=? AND tmpRank < 4 \
) rankstuff \
GROUP BY id,style,tmpLB \
ORDER BY golds DESC, mostRecentGold ASC"


	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_RANK, RANKREQUEST PAGINGLIMIT)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Ranks cannot be displayed. Database request failed.\n\"");
		return;
	}
	G_COOL_API_DB_PreparedBindInt(data.style);
	G_COOL_API_DB_PreparedBindInt(data.lbType);
	G_COOL_API_DB_PreparedBindInt(first);
	G_COOL_API_DB_FinishAndSendPreparedStatement();

}

/*
=================
Cmd_Rollympics_f
=================
*/
void Cmd_Rollympics_f( gentity_t *ent )
{
	int page = 1;
	if (trap_Argc() > 1) {
		char number[15];
		trap_Argv(1, number, sizeof(number));
		page = atoi(number);
		if (page < 1) {
			page = 1;
		}
	}

	DF_RequestSubContestLeaderboard(ent, SUBCONTESTS_ROLLYMPICS_FIX,page);
}
/*
=================
Cmd_DBSRecords_f
=================
*/
void Cmd_DBSRecords_f( gentity_t *ent )
{
	int page = 1;
	char arg[15];
	subContests_t category = SUBCONTESTS_DBS_SPEED;
	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: dbsrecords <speed|kill|ironman|ctfret|killslowed|stupidspeed> [page]\n\"");
		return;
	}
	
	trap_Argv(1,arg,sizeof(arg));
	if (!Q_stricmp(arg,"speed")) {
		category = SUBCONTESTS_DBS_SPEED;
	} else if (!Q_stricmp(arg,"stupidspeed")) {
		category = SUBCONTESTS_DBS_STUPIDSPEED;
	} else if (!Q_stricmp(arg, "kill")) {
		category = SUBCONTESTS_DBS_KILL;
	} else if (!Q_stricmp(arg, "killslowed") || !Q_stricmp(arg, "speedloss")) {
		category = SUBCONTESTS_DBS_KILL_SPEEDLOSS;
	} else if (!Q_stricmp(arg, "ironman")) {
		category = SUBCONTESTS_DBS_IRONMAN;
	} else if (!Q_stricmp(arg, "ctfreturns")) {
		category = SUBCONTESTS_DBS_CTFRETURN;
	}
	else {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: dbsrecords <speed|kill|ironman|ctfret|killslowed> [page]\n\"");
		return;
	}

	if (trap_Argc() > 2) {
		trap_Argv(1, arg, sizeof(arg));
		page = atoi(arg);
		if (page < 1) {
			page = 1;
		}
	}

	DF_RequestSubContestLeaderboard(ent, category, page);
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];
	
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->switchTeamTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam_User( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->switchTeamTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam_User( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;

	if ( original >= MAX_CLIENTS || original < 0 ) original = 0; // SpectatorCrashFix (infinite loop)

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, const char* append ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM && ent && ent->client && !(OnSameTeam(ent, other) || g_alwaysAllowTeamChat.integer && (ent->client->sess.sessionTeam == TEAM_SPECTATOR || g_alwaysAllowTeamChat.integer >=2) && ent->client->sess.sessionTeam == other->client->sess.sessionTeam) ) {
		return;
	}
	if ( ent && other->client->sess.ignore & (1 << (ent-g_entities))) {
		return;
	}
	// no chatting to players in tournements
	if ( ent && ent->client && (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}

	trap_SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\"%s%s",
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message, ent ? miniva(" %i",(int)(ent - g_entities)) : "", append ? append : "")); // lets have some privacy for private chatters
}


const char* SkipWhitespace2(const char* data) {
	while (*data == ' ')
		++data;

	return data;
}
static char* G_GrabToken(const char* p, qboolean skipLeadingWhiteSpace) {
	static char token[256] = { 0 };
	int i = 0;
	int c;

	if (!p)
		return NULL;

	memset(&token, 0, sizeof(token));

	if (skipLeadingWhiteSpace)
		while (*p == ' ') ++p;


	while ((c = *(p++)) != 0 && i < 255) {
		if (c == ' ')
			break;
		token[i++] = c;
	}
	token[i] = 0;

	return token;
}
#define		CCMD_SAYALLONLY			(1<<0)
typedef struct {
	const char* name;
	const char* argsMask;
	const char* desc;
	void 		(*fun)(gentity_t* ent, const char* args);
	int			flags;
} chatCommand_t;

static void Cmd_PauseGame_C(gentity_t* ent, const char* args);
static void Cmd_UnpauseGame_C(gentity_t* ent, const char* args);

static const chatCommand_t chatCmds[] = {
	{ "pause", NULL, "Pause the game", Cmd_PauseGame_C, CCMD_SAYALLONLY },
	{ "unpause", NULL, "Unpause the game", Cmd_UnpauseGame_C, CCMD_SAYALLONLY },
};
static const size_t numChatCommands = ARRAY_LEN(chatCmds);

static void Cmd_PauseGame_C(gentity_t* ent, const char* args) {
	int c;

	if (args && args[0]) {
		return;
	}

	if (!g_allowChatPause.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"This command is currently disabled.\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Only playing players can use this command.\n\"");
		return;
	}



	if (level.unpauseTime) {
		//game is about to be unpaused, so we dont care if the pause cvar is on. make sure it stays paused!
		level.unpauseTime = 0;
	}
	else if (g_pauseGame.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Game is already paused.\n\"");
		return;
	}

	level.unpauseClient = ent - g_entities + 1;

	G_CenterPrint(-1,3,va("Game was paused by %s^7.", ent->client->pers.netname),qtrue,qfalse,qtrue,NULL);
	G_LogPrintf("Game was paused by client %d\n", (int)(ent - g_entities));

	trap_SendConsoleCommand(EXEC_APPEND, PAUSEGAME_CVARNAME" 1\n");
}

static void Cmd_UnpauseGame_C(gentity_t* ent, const char* args) {
	int real;

	if (args && args[0]) {
		return;
	}

	if (!g_allowChatPause.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"This command is currently disabled.\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Only playing players can use this command.\n\"");
		return;
	}

	if (!g_pauseGame.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Game is not paused.\n\"");
		return;
	}

	if (g_allowChatPause.integer == 2) { // g_allowChatPause 2 = only the pausing client can unpause
		real = level.unpauseClient - 1;

		if (real >= 0 && real < MAX_CLIENTS) {

			if (!g_entities[real].inuse || !g_entities[real].client) {
				//the guy who paused left. = anyone can unpause
			}
			else if (ent - g_entities == real) {
				//hes the one who paused, so he can unpause.
			}
			else if (level.clients[real].sess.sessionTeam != TEAM_SPECTATOR) {
				trap_SendServerCommand(ent - g_entities, va("print \"Game was paused by %s ^7(%d); only he can unpause.\n\"", SHOWNAME(real), real));
				return;
			}
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"no.\n\"");
			return;
		}
	}

	if (level.unpauseTime) {
		trap_SendServerCommand(ent - g_entities, "print \"Unpause is already pending.\n\"");
		return;
	}

	G_LogPrintf("Game was unpaused by client %d\n", (int)(ent - g_entities));

	//instart of unpausing immediately, count down so players can prepare
	level.unpauseTime = level.time + UNPAUSE_COUNTDOWN;		//when level.time hits this, unpause!
}



static qboolean QDECL TestCallback(gentity_t* ent, genericDbRequestStruct_t* data) {
	if (G_COOL_API_DB_NextRow()) {
		char result[20]; 
		G_COOL_API_DB_GetString(0, result, sizeof(result));
		trap_SendServerCommand(ent - g_entities, va("print \"test: %s\n\"", result));
	}
	return qtrue;
}
REGISTER_DBREQUEST_CALLBACK(GDBREQUEST_TEST,TestCallback);

static void Cmd_Test_f(gentity_t* ent) {
	int userid;
	genericDbRequestStruct_t data = G_DB_GenericRequest_Prepare(ent, GDBREQUEST_TEST,(1<<DBT_USERS),"testcmd", 0);
	if (trap_Argc() < 2) {
		return;
	}
	userid = atoi(G_Argv(1));
	if (!G_DB_GenericRequest_Send(data,"SELECT course FROM runs GROUP BY course HAVING COUNT(*)=%d",userid)) {
		trap_SendServerCommand(ent-g_entities,"print \"Error sending test request.\n\"");
	}
}

typedef enum tagMapSubCmd_s {
	TAGMAP_ADD,
	TAGMAP_REMOVE,
	TAGMAP_CLEAR,
	TAGMAP_LIST,
	TAGMAP_LISTALL,
	TAGMAP_SEARCH,
} tagMapSubCmd_t;
static qboolean QDECL TagCallback(gentity_t* ent, genericDbRequestStruct_t* data) {
	int rows = 0;
	switch (data->specifics.maptag.requestType) {
		case TAGMAP_ADD:
			if (data->resultInfo.affectedRows == 0) {
				trap_SendServerCommand(ent - g_entities, va("print \"^1Unknown Error setting tag '%s'.\n\"", data->specifics.maptag.tag));
			}
			else if (data->resultInfo.affectedRows == 1) {
				trap_SendServerCommand(ent - g_entities, va("print \"^2Tag '%s' successfully added.\n\"", data->specifics.maptag.tag));
			}
			else if (data->resultInfo.affectedRows == 2) {
				trap_SendServerCommand(ent - g_entities, va("print \"^3Tag '%s' has already been set by you.\n\"", data->specifics.maptag.tag));
			}
			break;
		case TAGMAP_REMOVE:
			if (!data->resultInfo.affectedRows) {
				trap_SendServerCommand(ent - g_entities, va("print \"^1Tag '%s' could not be removed. It didn't exist?\n\"", data->specifics.maptag.tag));
			}
			else {
				trap_SendServerCommand(ent - g_entities, va("print \"^2Tag '%s' successfully removed.\n\"", data->specifics.maptag.tag));
			}
			break;
		case TAGMAP_CLEAR:
			if (!data->resultInfo.affectedRows) {
				trap_SendServerCommand(ent - g_entities, "print \"^1Tags were not cleared. No tags found?\n\"");
			}
			else {
				trap_SendServerCommand(ent - g_entities, "print \"^2Your tags for this map were successfully cleared.\n\"");
			}
			break;
		case TAGMAP_LIST:
			trap_SendServerCommand(ent - g_entities, va("print \"^2Active tags on this map, page %d:\n\"", data->page+1));
			while (G_COOL_API_DB_NextRow()) {
				char tag[MAPTAG_MAX_LEN + 1];
				int taggers = G_COOL_API_DB_GetInt(1);
				int byme = G_COOL_API_DB_GetInt(2);
				G_COOL_API_DB_GetString(0, tag, sizeof(tag));
				trap_SendServerCommand(ent - g_entities, va("print \"^%c%s (%d taggers)\n\"", byme? '2':'7',tag,taggers));
				rows++;
			}
			if (rows) {
				trap_SendServerCommand(ent - g_entities, "print \"^2Green ^7tags are tags that you yourself have added to this map.\n\"");
			}
			break;
		case TAGMAP_LISTALL:
			trap_SendServerCommand(ent - g_entities, va("print \"^2List of all tags, page %d:\n\"", data->page + 1));
			while (G_COOL_API_DB_NextRow()) {
				char tag[MAPTAG_MAX_LEN + 1];
				int taggers = G_COOL_API_DB_GetInt(1);
				int byme = G_COOL_API_DB_GetInt(2);
				int maps = G_COOL_API_DB_GetInt(3);
				G_COOL_API_DB_GetString(0, tag, sizeof(tag));
				trap_SendServerCommand(ent - g_entities, va("print \"^%c%s (%d taggers, %d maps)\n\"", byme? '2':'7',tag,taggers,maps));
				rows++;
			}
			if (rows) {
				trap_SendServerCommand(ent - g_entities, "print \"^2Green ^7tags are tags that you yourself have used.\n\"");
			}
			break;
		case TAGMAP_SEARCH:
			trap_SendServerCommand(ent - g_entities, va("print \"^2Maps matching tag '%s', page %d:\n\"", data->specifics.maptag.tag, data->page + 1));
			while (G_COOL_API_DB_NextRow()) {
				char map[COURSENAME_MAX_LEN + 1];
				int taggers = G_COOL_API_DB_GetInt(1);
				int byme = G_COOL_API_DB_GetInt(2);
				int runners = data->specifics.maptag.defrag ? G_COOL_API_DB_GetInt(3) : 0;
				G_COOL_API_DB_GetString(0, map, sizeof(map));
				trap_SendServerCommand(ent - g_entities, va("print \"^%c%s (%d taggers%s)\n\"", byme ? '2' : '7', map, taggers, data->specifics.maptag.defrag ? multiva(", %d players",runners) : ""));
				rows++;
			}
			if (rows) {
				trap_SendServerCommand(ent - g_entities, "print \"^2Green ^7maps are maps that you yourself have tagged with this tag.\n\"");
			}
			break;
		default:
			trap_SendServerCommand(ent - g_entities, "print \"^3Unknown map tag request finished.\n\"");
			break;
	}
	//if (G_COOL_API_DB_NextRow()) {
	//	char result[20]; 
	//	G_COOL_API_DB_GetString(0, result, sizeof(result));
	//	trap_SendServerCommand(ent - g_entities, va("print \"test: %s\n\"", result));
	//}
	return qtrue;
}
REGISTER_DBREQUEST_CALLBACK(GDBREQUEST_TAG,TagCallback);
static const char* ValidateMapTag(const char* tagName) {
	int len = strlen(tagName);
	if (len > MAPTAG_MAX_LEN) {
		return miniva("Map tag too long (up to %d characters).", MAPTAG_MAX_LEN);
	} else if (len < 3) {
		return "Map tag is too short (minimum 3 characters).";
	}
	while (*tagName) {
		if (*tagName >= '0' && *tagName <= '9' || *tagName >= 'a' && *tagName <= 'z' || *tagName >= 'A' && *tagName <= 'Z' || *tagName == '_') {
			tagName++;
			continue;
		}
		else {
			return "Invalid character in map tag."; //  miniva("Invalid character in map tag.");
		}
	}
	return NULL;
}
static void Cmd_TagMap_f(gentity_t* ent) {
	int userid;
	const char* arg;
	int arglen = trap_Argc();
	const char* courseName;
	genericDbRequestStruct_t data = G_DB_GenericRequest_Prepare(ent, GDBREQUEST_TAG,(1<<DBT_USERS) | (1 << DBT_MAPTAGS),"tagmap", 0);
	if (arglen < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /tag <tagname> - Add a tag\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /tag <remove> <tagname> - Remove a tag you added\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /tag <list|listall|clear> - List the map's tags, all known tags, or clear your own tags of this map\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /tag <search> <tagname> - Find maps that have a tag\n\"");
		return;
	}
	arg = G_Argv(1);

	courseName = DF_GetCourseName(qfalse);

	if (!Q_stricmp(arg, "search") && arglen > 2) {
		const char* tag = G_Argv(2);
		const char* validateError = ValidateMapTag(tag);
		data.specifics.maptag.requestType = TAGMAP_SEARCH;
		if (validateError) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot search for tag: %s.\n\"", validateError));
			return;
		}
		Q_strncpyz(data.specifics.maptag.tag, tag, sizeof(data.specifics.maptag.tag));
		data.page = G_DB_GetPageArg(3);
		if (g_defrag.integer) {
			data.specifics.maptag.defrag = qtrue;
			data.requiredTables |= (1<<DBT_RUNS);
			if (!G_DB_GenericRequest_Send(data, "SELECT maptags.course,COUNT(DISTINCT maptags.userid) AS taggers,MAX(%d=maptags.userid) AS ismine,COUNT(DISTINCT runs.userid) AS runners FROM maptags \
				LEFT JOIN runs ON (runs.course=maptags.course) \
				WHERE tag=%s GROUP BY course HAVING runners > 0 ORDER BY ismine DESC, taggers DESC, runners DESC LIMIT %d,10", ent->client->sess.login.id, tag, data.page * 10)) {
				trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
			}
		}
		else {
			if (!G_DB_GenericRequest_Send(data, "SELECT course,COUNT(DISTINCT userid) AS taggers,MAX(%d=userid) AS ismine FROM maptags WHERE tag=%s GROUP BY course ORDER BY ismine DESC, taggers DESC LIMIT %d,10", ent->client->sess.login.id, tag, data.page * 10)) {
				trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
			}
		}
	}
	else if (!Q_stricmp(arg, "list")) {
		data.specifics.maptag.requestType = TAGMAP_LIST;
		data.page = G_DB_GetPageArg(2);
		if (!G_DB_GenericRequest_Send(data, "SELECT tag,COUNT(DISTINCT userid) AS taggers,MAX(%d=userid) AS ismine FROM maptags WHERE course=%s GROUP BY tag ORDER BY ismine DESC, taggers DESC LIMIT %d,10", ent->client->sess.login.id, courseName, data.page * 10)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
		}
	}
	else if (!Q_stricmp(arg, "listall")) {
		data.specifics.maptag.requestType = TAGMAP_LISTALL;
		data.page = G_DB_GetPageArg(2);
		if (!G_DB_GenericRequest_Send(data, "SELECT tag,COUNT(DISTINCT userid) AS taggers,MAX(%d=userid) AS ismine,COUNT(DISTINCT maptags.course) AS maps FROM maptags GROUP BY tag ORDER BY maps DESC, taggers DESC,ismine DESC LIMIT %d,10", ent->client->sess.login.id,data.page*10)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
		}
	}
	else if (!ent->client->sess.login.loggedIn) {
		trap_SendServerCommand(ent - g_entities, "print \"Login is required for adding, removing or clearing tags.\n\"");
		return;
	}
	else if (!Q_stricmp(arg,"remove") && arglen > 2) {
		const char* tag = G_Argv(2);
		const char* validateError = ValidateMapTag(tag);
		data.specifics.maptag.requestType = TAGMAP_REMOVE;
		if (validateError) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot remove tag: %s.\n\"", validateError));
			return;
		}
		Q_strncpyz(data.specifics.maptag.tag, tag, sizeof(data.specifics.maptag.tag));
		if (!G_DB_GenericRequest_Send(data, "DELETE FROM maptags WHERE course=%s AND userid=%d AND tag=%s", courseName, ent->client->sess.login.id, tag)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
		}

	} else if (!Q_stricmp(arg,"clear")) {
		data.specifics.maptag.requestType = TAGMAP_CLEAR;
		if (!G_DB_GenericRequest_Send(data, "DELETE FROM maptags WHERE course=%s AND userid=%d", courseName, ent->client->sess.login.id)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
		}
	} else  {
		const char* tag = arg;
		const char* validateError = ValidateMapTag(tag);
		data.specifics.maptag.requestType = TAGMAP_ADD;
		if (validateError) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot set tag: %s.\n\"", validateError));
			return;
		} 
		Q_strncpyz(data.specifics.maptag.tag,tag,sizeof(data.specifics.maptag.tag));
		if (!G_DB_GenericRequest_Send(data, "INSERT INTO maptags (course,userid,tag,setwhen,updatedwhen) VALUES (%s,%d,%s,NOW(),NOW()) ON DUPLICATE KEY UPDATE updatedwhen=NOW()", courseName, ent->client->sess.login.id, tag)) {
			trap_SendServerCommand(ent - g_entities, "print \"Error sending maptag request.\n\"");
		}
	}
}



//for limiting user args length so we're not wasting bandwidth echoing bad user requests.
const char* ShortString(const char* str) {
	static char buf[32] = { 0 };

	if (str)
		Q_strncpyz(buf, str, sizeof(buf));
	else
		buf[0] = 0;

	return &buf[0];
}



#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	char		nameToAll[93]; // 64+29
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	int			pseudoArgC;
	const char* pseudoCmd;
	const char* pseudoArg1;
	char		lowercaseCmd[20]; // for levenshtein check. doesnt need to be longer. that long of a cmd wouldn't trigger it anyway
	int			cmdLen,i;
	int			originalMode = mode;
	qboolean	allowCrossServer = mode == SAY_CROSSSERVER && g_crossServerChat.integer >1 || g_crossServerChat.integer > 2;
	qboolean	toAllServers = qfalse;

	if (mode == SAY_CROSSSERVER) {
		mode = SAY_ALL;
	}

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM && !(g_alwaysAllowTeamChat.integer && ent->client->sess.sessionTeam == TEAM_SPECTATOR || g_alwaysAllowTeamChat.integer >= 2) ) {
		mode = SAY_ALL;
	}

	BG_Cmd_TokenizeString(chatText); // vm-side cmd parsing :/
	pseudoArgC = BG_Cmd_Argc();
	pseudoCmd = BG_Cmd_Argv(0);
	pseudoArg1 = BG_Cmd_Argv(1);

	if (pseudoArgC >= 2) {
		Q_strncpyz(lowercaseCmd, pseudoCmd, sizeof(lowercaseCmd));
		cmdLen = strlen(lowercaseCmd);
		for (i = 0; i < cmdLen; i++) {
			lowercaseCmd[i] = tolower(lowercaseCmd[i]);
		}

		if ((
			(levenshtein("login", lowercaseCmd) <= 2 && Q_stricmp("logout", pseudoCmd) && Q_stricmp("admin", pseudoCmd) && Q_stricmp("losing", pseudoCmd))  // allow "admin"/"logout" tho. "losing" (the word" is also ok :)
			|| levenshtein("register", lowercaseCmd) <= 2
			) && pseudoArgC >=3 && pseudoArgC <= 5) {
			G_LogPrintf("clientSay accidental credentials? (mode %d): %s : %s %s ****** %s\n", mode, ent->client->pers.netname, pseudoCmd, pseudoArg1, BG_Cmd_ArgsFrom(3));
			G_CenterPrint(ent - g_entities, 3, "^1You may have accidentally typed your account credentials in chat, so your message was blocked.", qfalse, qfalse, qtrue, NULL);
			return;
		}
		else if (levenshtein("changepassword", lowercaseCmd) <= 3 && pseudoArgC >= 2 && pseudoArgC <= 4) {
			G_LogPrintf("clientSay accidental credentials? (mode %d): %s : %s ****** %s\n", mode, ent->client->pers.netname, pseudoCmd, BG_Cmd_ArgsFrom(2));
			G_CenterPrint(ent - g_entities, 3, "^1You may have accidentally typed a password in chat, so your message was blocked.", qfalse, qfalse, qtrue, NULL);
			return;
		}
	}

	nameToAll[0] = '\0';

	// CHECK MUTE
	if (ent->client->sess.amflags & AMFLAG_MUTED) {
		trap_SendServerCommand(ent - g_entities, "cp \"You are muted.\"");
		return;
	}
	else if ((ent->client->sess.amflags & AMFLAG_MUTED_PUBONLY) && mode != SAY_TEAM) {
		trap_SendServerCommand(ent - g_entities, "cp \"You can only talk in team chat.\"");
		return;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say%s: %s: %s\n", originalMode == SAY_CROSSSERVER ? "(to all servers)" : "", ent->client->pers.netname, chatText);
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, NULL);
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf("%s%s\n", name, text);
	}

	if (mode == SAY_ALL && (coolApi & COOL_APIFEATURE_CROSS_SERVER_COMMANDS) && allowCrossServer) {
		G_SendCrossServerCommand(va("chatAll \"%s\" \"%s\" \"%s\"", name, text, DF_GetCourseName(qfalse)));
		//Com_sprintf(nameToAll, sizeof(nameToAll), "(to all servers) %s", name);
		Com_sprintf(nameToAll, sizeof(nameToAll), "^d<^fto all servers^d<^7: %s", name);
		toAllServers = qtrue;
	}

	//Check for chat cmds.
	if (chatText[0] == '!') {
		const char* token = G_GrabToken(++chatText, qfalse);
		const chatCommand_t* cmd;

		for (j = 0, cmd = chatCmds; j < numChatCommands; ++j, ++cmd) {
			if (cmd && cmd->fun && !Q_stricmp(token, cmd->name)) {
				//only allow pause cmds in say_all
				if ((cmd->flags & CCMD_SAYALLONLY) && mode != SAY_ALL) {
					break;
				}



				chatText += strlen(token);
				cmd->fun(ent, SkipWhitespace2(chatText));
				break;
			}
		}
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, toAllServers ? nameToAll : name, text, toAllServers ? " crossServerBroadcast" : "");
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

static void Cmd_SayNormal_f(gentity_t* ent) {
	Cmd_Say_f(ent, SAY_ALL, qfalse);
}
static void Cmd_SayCross_f(gentity_t* ent) {
	Cmd_Say_f(ent, SAY_CROSSSERVER, qfalse);
}
static void Cmd_SayTeam_f(gentity_t* ent) {
	Cmd_Say_f(ent, SAY_TEAM, qfalse);
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, "[private message]" /*p*/); // lets have some privacy
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}


static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if (g_gametype.integer == GT_TOURNAMENT) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

#if 0
/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_STUN_BATON) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > LEVELTIME(who->client)) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
#endif


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order >= (int)ARRAY_LEN(gc_orders) ) { // should be >= lol
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	G_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) , qtrue);
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Single Player",
	"Team FFA",
	"N/A",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
G_ClientNumberFromName

Finds the client number of the client with the given name
==================
*/
int G_ClientNumberFromName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

/*
==================
G_ClientNumberFromStrippedName

Same as above, but strips special characters out of the names before comparing.
==================
*/
int G_ClientNumberFromStrippedName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString2( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString2( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

extern void G_BlacklistMap(const char* thisMapName);

void Cmd_GenArena_f(gentity_t* ent) {
	qboolean allRace = qfalse;
	char	arg1[10];

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_ARENAGEN)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You do not have permission to use this command.\n\"");
		return;
	}

	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: genArena [allrace|this].\n\"");
		return;
	}
	trap_Argv(1, arg1, sizeof(arg1));

	if (!Q_stricmp(arg1, "this")) {
		if (!level.hasArenaInfo) {
			level.mustGenerateArena = qtrue;
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"This map already has an arena info.\n\"");
			return;
		}
	}
	else if (!Q_stricmp(arg1, "allrace")) {
		G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_ARENAGENMAPLIST, "SELECT DISTINCT course FROM runs");
	}
	else {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: genArena [allrace|this].\n\"");
		return;
	}

}

#define ARENALESS_LINE_MAX_LENGTH 150
void Cmd_Arenaless_f(gentity_t* ent) {
	qboolean		allRace = qfalse;
	char			arg1[10];
	static char		dirlistBsp[524288];
	char*			bspptr,*strptr;
	int				numBsps;
	int				i,bspLen;
	infoHashed_t*	ai;
	char			currentMessage[MAX_STRING_CHARS];
	char*			curMsgPtr = currentMessage;
	int				curMsgIndex = 0;
	qboolean		overflowing = qfalse;
	int				count = 0;

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_ARENALESSMAPS)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You do not have permission to use this command.\n\"");
		return;
	}
	trap_SendServerCommand(ent - g_entities, "print \"Maps without arena:\n\"");

	numBsps = trap_FS_GetFileList("maps", ".bsp", dirlistBsp, sizeof(dirlistBsp));
	bspptr = dirlistBsp;
	curMsgIndex = 0;
	for (i = 0; i < numBsps; i++, bspptr += bspLen + 1) {
		bspLen = strlen(bspptr); 
		// a.bsp
		*(bspptr + bspLen - 4) = '\0'; // cut off .bsp
		ai = G_GetArenaInfoByMap(bspptr);
		if (ai || G_IsMapBlacklisted(bspptr)) {
			continue; // already have this as arena
		}

		if ((curMsgIndex+ bspLen+25)>=sizeof(currentMessage)) { // check if we are overflowing (shouldnt happen as we are pretty generous overall)
			*curMsgPtr++ = '\0';
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", currentMessage));
			curMsgPtr = currentMessage;
			curMsgIndex = 0;
		}

		if ((curMsgIndex + bspLen + 25) >= sizeof(currentMessage)) {
			continue; // we are STILL overflowing. troll map that has a name thats 1024 chars long? skip.
		}
		count++;
		strptr = bspptr;
		while (*strptr) { // 25 cuz im too lazy to think this through.
			*curMsgPtr++ = *strptr++;
			curMsgIndex++;
		}
		*curMsgPtr++ = ' ';
		curMsgIndex++;
		while (curMsgIndex % 20 && curMsgIndex < ARENALESS_LINE_MAX_LENGTH) { // align the mapnames at 20 char distances
			*curMsgPtr++ = ' ';
			curMsgIndex++;
		}
		if (curMsgIndex >= ARENALESS_LINE_MAX_LENGTH) {
			*curMsgPtr++ = '\0';
			trap_SendServerCommand(ent-g_entities,va("print \"%s\n\"",currentMessage));
			curMsgPtr = currentMessage;
			curMsgIndex = 0;
		}
	}

	if (curMsgIndex) {
		*curMsgPtr++ = '\0';
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", currentMessage));
		//curMsgPtr = currentMessage; // no need, we're done anyway
		//curMsgIndex = 0;
	}
	trap_SendServerCommand(ent - g_entities, va("print \"\nTotal count: %d\n\"", count));



}

void Cmd_BlacklistMap_f(gentity_t* ent) {
	char arg[128];

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_BLACKLISTMAPS)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You do not have permission to use this command.\n\"");
		return;
	}

	if (trap_Argc() > 1) {
		trap_Argv(1, arg, sizeof(arg));
	}
	else {
		Q_strncpyz(arg, DF_GetCourseName(qfalse), sizeof(arg));
	}
	if (G_IsMapBlacklisted(arg)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Cannot blacklist map. Already blacklisted.\n\"");
		return;
	}
	G_BlacklistMap(arg);
	

}

typedef struct afkClient_s {
	int afkTime;
	int	specSendIn;
	int clientNum;
} afkClient_t;



int compareAfkCliientEntry(const void* a, const void* b) {
	return ((afkClient_t*)b)->afkTime - ((afkClient_t*)a)->afkTime;
}

void Cmd_Afk_f(gentity_t* ent) {
	gentity_t* other;
	afkClient_t afkTimes[MAX_CLIENTS];
	int afkCount = 0;
	int i;
	int millisecs,minMillisecs = clampedIntMult(g_afkCmdMinSecs.integer, 1000);
	for (i = 0; i < level.maxclients; i++) {
		other = g_entities + i;
		if (!other->inuse || !other->client) {
			continue;
		}
		millisecs = clampedIntAdd(level.time, -other->client->sess.lastHereTime);
		if (millisecs < minMillisecs) {
			continue;
		}
		afkTimes[afkCount].afkTime = millisecs;
		afkTimes[afkCount].clientNum = i;
		if (g_inactivityToSpec.integer && other->client->sess.sessionTeam != TEAM_SPECTATOR && millisecs >= clampedIntMult(g_inactivityToSpec.integer,1000)) {
			millisecs = clampedIntAdd(other->client->pers.inactivityToSpecTime, -level.time);
			afkTimes[afkCount].specSendIn = millisecs;
		}
		else {
			afkTimes[afkCount].specSendIn = 0;
		}
		afkCount++;
	}
	if (!afkCount) {

		trap_SendServerCommand(ent - g_entities, "print \"Nobody is afk.\n\"");
		return;
	}
	qsort(afkTimes,afkCount,sizeof(afkTimes[0]), compareAfkCliientEntry);
	trap_SendServerCommand(ent - g_entities, "print \"Players AFK status:\n\"");
	for (i = 0; i < afkCount; i++) {
		other = g_entities + afkTimes[i].clientNum;
		trap_SendServerCommand(ent - g_entities, va("print \"%15s%s %2d %s\n\"",DF_MsToString(afkTimes[i].afkTime),afkTimes[i].specSendIn > 0 ? miniva(" (to spec in %s)", DF_MsToString(afkTimes[i].specSendIn)) : "", afkTimes[i].clientNum, other->client->pers.netname));
	}
}

void Cmd_Players_f(gentity_t* ent) {
	gentity_t* other;
	gclient_t* cl;
	int i;
	int millisecs,minMillisecs = clampedIntMult(g_afkCmdMinSecs.integer, 1000), minMillisecsStayOnMap = clampedIntMult(g_slowVoteAFKThreshold.integer, 1000);
	qboolean slowVoteActive = G_SlowVoteActive();
	trap_SendServerCommand(ent - g_entities, "print \"Players:\n\"");
	trap_SendServerCommand(ent - g_entities, "print \"^2#  User       Mode                      AFK        FPS  Jump  Name\n\"");
	for (i = 0; i < level.maxclients; i++) {
		other = g_entities + i;
		if (!other->inuse || !other->client) {
			continue;
		}
		cl = other->client;
		millisecs = clampedIntAdd(level.time, -other->client->sess.lastHereTime);
		trap_SendServerCommand(ent - g_entities, va("print \"%-2d %-10s %-25s %-10s %-4s %-5d %s%s%s\n\"", 
			i,
			cl->sess.login.loggedIn ? cl->sess.login.name : "",
			cl->sess.raceMode ? multiva("Race:%s/%s", moveStyleNames[cl->sess.raceStyle.movementStyle].string, leaderboardNames[classifyLeaderBoard(&cl->sess.raceStyle,&level.mapDefaultRaceStyle)].string) : modeNames[cl->sess.mode].string,
			millisecs >= minMillisecs ? DF_MsToString(millisecs) : "",
			cl->sess.raceStyle.msec == -1 ? "togl" : (cl->sess.raceStyle.msec == -2 ? "flt" : (cl->sess.raceStyle.msec == 0 ? "unkn" : miniva("%d", 1000 / cl->sess.raceStyle.msec))),
			cl->sess.raceStyle.jumpLevel,
			other->client->pers.netname,
			other->client->pers.stayOnMap && slowVoteActive ?(clampedIntAdd(level.time,-cl->sess.lastHereTime) < minMillisecsStayOnMap ? " ^7(wants to stay on map)" : " ^7(wants to stay on map but ^1AFK^7)") : "",
			(other->client->pers.tasClient & TASCLIENT_MACHINELEARNING) ? " ^7(TAS Machine Learning Bot)" : (other->client->pers.tasClient ? " ^7(TAS Client)" : "")
		));
	}
}

// g_slow
qboolean G_SlowVoteActive() {
	int minMillisecs = clampedIntMult(g_slowVoteAFKThreshold.integer, 1000);
	if (g_slowVote.integer > 1) { // 2 = always active
		return qtrue;
	}
	else if(g_slowVote.integer){ // 1 = only active if ppl are racing
		gentity_t* oEnt;
		int i;
		for (i = 0; i < level.maxclients; i++) {
			oEnt = g_entities + i;

			if (oEnt->client->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (oEnt->client->sess.raceMode && oEnt->client->sess.sessionTeam != TEAM_SPECTATOR && clampedIntAdd(level.time, -oEnt->client->sess.lastHereTime) < minMillisecs) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

extern int DF_GetSegmentedRunnerCount();

int G_SlowVoteProhibits(int ownclientNum) {
	gentity_t* oEnt;
	int i;
	int stayers = 0;
	int minMillisecs = clampedIntMult(g_slowVoteAFKThreshold.integer, 1000);
	qboolean slowVoteActive = G_SlowVoteActive();

	if (!slowVoteActive) return 0;

	for (i = 0; i < level.maxclients; i++) {
		oEnt = g_entities + i;

		if (i == ownclientNum || oEnt->client->pers.connected != CON_CONNECTED) {
			continue;
		}
		// extend this to any segmented runner? but how to avoid trolling?
		if (oEnt->client->pers.stayOnMap && oEnt->client->sess.sessionTeam != TEAM_SPECTATOR && clampedIntAdd(level.time,-oEnt->client->sess.lastHereTime) < minMillisecs) {
			stayers++;
		}
	}
	return stayers;
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int		i;
	int		tmp;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];
	//int		clientPermissions;
	qboolean	canVoteBesideMap = qfalse;
	qboolean	canVoteTeamShuffle = qfalse;
	qboolean	votingOpinion = qfalse;
	qboolean	votingOpinionAll = qfalse;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.voteExecuteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", "Cannot call a vote, old vote not yet exceuted, please wait.") );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEINPROGRESS")) );
		return;
	}
	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXVOTES")) );
		return;
	}
	trap_Argv(1, arg1, sizeof(arg1));
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && Q_stricmp(arg1, "opinion") && Q_stricmp(arg1, "opinionAll")) { // opinions can be initiated from spec
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	canVoteBesideMap = g_allowVote.integer > 1 || ent->client->sess.login.loggedIn && (ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_VOTEBESIDESMAP);
	canVoteTeamShuffle = g_allowVoteShuffle.integer > 0 || canVoteBesideMap;
	//clientPermissions = ent->client->sess.login.loggedIn ? ent->client->sess.login.flags : 0;

	// make sure it is a valid command to vote on
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "randommap" ) ) {
	} else if ( !Q_stricmp( arg1, "opinion" ) ) {
	} else if ( !Q_stricmp( arg1, "opinionAll" ) ) {
	} else if ( !Q_stricmp( arg1, "mapnum" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
	} else if ( !Q_stricmp( arg1, "timelimit" ) ) {
	} else if ( !Q_stricmp( arg1, "fraglimit" ) ) {
	} else if ( !Q_stricmp( arg1, "shuffle" ) || !Q_stricmp( arg1, "scrambleteams" ) ) {
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, mapnum <mapnum>, randommap, opinion <anything>, opinionAll <anything>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>, shuffle.\n\"" );
		return;
	}


	// if there is still a vote to be executed - wait HUH?
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( arg1, "g_gametype" ) && canVoteBesideMap)
	{
		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand(ent - g_entities, "print \"Cannot vote for a new gametype while segmented runs are being replayed.\n\"");
			return;
		}

		if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot vote for a new gametype, slow voting is active and %d other active players with to stay.\n\"", tmp));
			return;
		}

		i = atoi( arg2 );
		if( i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			trap_SendServerCommand( ent-g_entities, "print \"Invalid gametype.\n\"" );
			return;
		}

		level.votingGametype = qtrue;
		level.votingGametypeTo = i;

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[i] );
	}
	else if ( !Q_stricmp( arg1, "opinion" ) ) 
	{
		if (!level.numVotingClients) {
			trap_SendServerCommand(ent - g_entities, "print \"There's nobody here who could vote on that. Try opinionAll?\n\"");
			return;
		}
		if (trap_Argc() < 2) {
			trap_SendServerCommand(ent - g_entities, "print \"What opinion do you want to vote on?\n\"");
			return;
		}
		else {
			const char* args = ConcatArgs(2);
			if (strlen(args) > 150) {
				trap_SendServerCommand(ent - g_entities, "print \"That's a bit long. Try to be concise.\n\"");
				return;
			}
			votingOpinion = qtrue;
			level.voteString[0] = '\0';
			Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "^3Opinion (players): %s ", args);
		}
	}
	else if ( !Q_stricmp( arg1, "opinionAll" ) ) 
	{
		if (trap_Argc() < 2) {
			trap_SendServerCommand(ent - g_entities, "print \"What opinion do you want to vote on?\n\"");
			return;
		}
		else {
			const char* args = ConcatArgs(2);
			if (strlen(args) > 150) {
				trap_SendServerCommand(ent - g_entities, "print \"That's a bit long. Try to be concise.\n\"");
				return;
			}
			votingOpinion = qtrue;
			votingOpinionAll = qtrue;
			level.voteString[0] = '\0';
			Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "^3Opinion (all): %s ", args);
		}
	}
	else if ( !Q_stricmp( arg1, "map" ) ) 
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];
		

		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand( ent-g_entities, "print \"Cannot vote for a new map while segmented runs are being replayed.\n\"" );
			return;
		}

		if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot vote for a new map, slow voting is active and %d other active players with to stay.\n\"", tmp));
			return;
		}

		if (!G_DoesMapSupportGametype(arg2, trap_Cvar_VariableIntegerValue("g_gametype")))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s" S_COLOR_WHITE "; set nextmap %s", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		}
	}
	else if (canVoteTeamShuffle && ( !Q_stricmp( arg1, "shuffle" ) || !Q_stricmp( arg1, "scrambleteams" )) )
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];
		

		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand( ent-g_entities, "print \"Cannot vote for a team shuffle while segmented runs are being replayed.\n\"" );
			return;
		}

		//if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
		//	trap_SendServerCommand(ent - g_entities, va("print \"Cannot shuffle teams, slow voting is active and %d other active players with to stay.\n\"", tmp));
		//	return;
		//}

		Com_sprintf(level.voteString, sizeof(level.voteString), "%s", arg1);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	}
	else if ( !Q_stricmp( arg1, "mapnum" ) ) 
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char			s[MAX_STRING_CHARS];
		char			mapname[MAX_STRING_CHARS];
		int				mapnum = atoi(arg2);

		if (mapnum < 0 || mapnum >= g_numArenas) {
			trap_SendServerCommand(ent - g_entities, "print \"Map could not be found from mapnum.\n\"");
			return;
		}

		//Q_strncpyz(mapname,Info_ValueForKey(g_arenaInfos[mapnum], "map"),sizeof(mapname));
		Q_strncpyz(mapname,g_arenaInfosHashed[mapnum].name,sizeof(mapname));

		if (!mapname || !mapname[0]) {
			trap_SendServerCommand(ent - g_entities, "print \"Map could not be found from mapnum (wtf?!).\n\"");
			return;
		}

		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand( ent-g_entities, "print \"Cannot vote for a new map while segmented runs are being replayed.\n\"" );
			return;
		}

		if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot vote for a new map, slow voting is active and %d other active players with to stay.\n\"", tmp));
			return;
		}

		if (!G_DoesMapSupportGametype(mapname, trap_Cvar_VariableIntegerValue("g_gametype")))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", "map", mapname, s );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s" S_COLOR_WHITE "; set nextmap %s", "map", mapname, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", "map", mapname);
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		}
	} else if ( !Q_stricmp( arg1, "randommap" ) ) 
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char			s[MAX_STRING_CHARS];
		char			mapname[MAX_STRING_CHARS];
		int				mapnum = -1; //atoi(arg2);
		int				tries = 0;

		if (g_numArenas < 1) {
			trap_SendServerCommand(ent - g_entities, "print \"No maps found.\n\"");
			return;
		}

		while (mapnum == -1 && tries< 10) {
			mapnum = Q_irand(0, g_numArenas, qfalse, 0); //atoi(arg2);
			if (mapnum < 0 || mapnum >= g_numArenas) {
				trap_SendServerCommand(ent - g_entities, "print \"WEIRD! Map could not be found from mapnum.\n\"");
				Com_Printf("WEIRD! randommap Map could not be found from mapnum %d, g_numArenas %d.\n", mapnum, g_numArenas);
				return;
			}
			if (!Q_stricmp(g_arenaInfosHashed[mapnum].name,DF_GetCourseName(qfalse))) { // dont go on the same map we are on now
				mapnum = -1;
			}
			tries++;
		}

		if (mapnum == -1) {
			trap_SendServerCommand(ent - g_entities, "print \"Was unable to choose random map after 10 tries.\n\"");
			Com_Printf("WEIRD! randommap Was unable to choose random map after 10 tries. g_numArenas %d.\n", g_numArenas);
			return;
		}

		//Q_strncpyz(mapname,Info_ValueForKey(g_arenaInfos[mapnum], "map"),sizeof(mapname));
		Q_strncpyz(mapname,g_arenaInfosHashed[mapnum].name,sizeof(mapname));

		if (!mapname || !mapname[0]) {
			trap_SendServerCommand(ent - g_entities, "print \"Map could not be found from mapnum (wtf?!).\n\"");
			return;
		}

		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand( ent-g_entities, "print \"Cannot vote for a new map while segmented runs are being replayed.\n\"" );
			return;
		}

		if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot vote for a new map, slow voting is active and %d other active players with to stay.\n\"", tmp));
			return;
		}

		if (!G_DoesMapSupportGametype(mapname, trap_Cvar_VariableIntegerValue("g_gametype")))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", "map", mapname, s );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "(randommap) %s %s" S_COLOR_WHITE "; set nextmap %s", "map", mapname, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", "map", mapname);
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "(randommap) %s", level.voteString );
		}
	}
	else if ( !Q_stricmp ( arg1, "clientkick" ) && canVoteBesideMap)
	{
		int n = atoi ( arg2 );

		if ( n < 0 || n >= MAX_CLIENTS )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", n ) );
			return;
		}

		if ( g_entities[n].client->pers.connected == CON_DISCONNECTED )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", n ) );
			return;
		}
			
		Com_sprintf ( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[n].client->pers.netname );
	}
	else if ( !Q_stricmp ( arg1, "kick" ) && canVoteBesideMap)
	{
		int clientid = G_ClientNumberFromName ( arg2 );

		if ( clientid == -1 )
		{
			clientid = G_ClientNumberFromStrippedName(arg2);

			if (clientid == -1)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"there is no client named '%s' currently on the server.\n\"", arg2 ) );
				return;
			}
		}

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", clientid );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[clientid].client->pers.netname );
	}
	else if ( !Q_stricmp( arg1, "nextmap" ) && canVoteBesideMap)
	{
		char	s[MAX_STRING_CHARS];

		if (DF_GetSegmentedRunnerCount()) {
			trap_SendServerCommand(ent - g_entities, "print \"Cannot vote for a new map while segmented runs are being replayed.\n\"");
			return;
		}
		
		if (tmp = G_SlowVoteProhibits(ent - g_entities)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Cannot vote for a new map, slow voting is active and %d other active players with to stay.\n\"",tmp));
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} 
	else if(canVoteBesideMap)
	{
		if ( !Q_stricmp( arg1, "g_doWarmup" ) || !Q_stricmp( arg1, "timelimit" ) || !Q_stricmp( arg1, "fraglimit" ) )
		{
			if ( strlen(arg2) >= MAX_CVAR_VALUE_STRING )
			{
				trap_SendServerCommand( ent-g_entities, "print \"The specified value is too long.\n" );
				return;
			}
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}
	else {
		trap_SendServerCommand(ent - g_entities, "print \"Can't vote for that.\n\"");
		return;
	}


	level.votingOpinion = votingOpinion;
	level.votingOpinionAll = votingOpinionAll;

	trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s: %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLCALLEDVOTE"), level.voteDisplayString ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = level.votingOpinion ? 0 : 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	if (!level.votingOpinion) {
		ent->client->ps.eFlags |= EF_VOTED;
		ent->client->pers.voteValue = qtrue;
	}

	// Append white colorcode at the end of the display string as workaround for cgame leaking colors
	Q_strcat( level.voteDisplayString, sizeof(level.voteDisplayString), S_COLOR_WHITE );

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );	
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEALREADY")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && !level.votingOpinionAll && !g_voteAsSpec.integer) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}
	if ( ent->client->markedAsInactive ) {
		trap_SendServerCommand( ent-g_entities, "print \"You cannot vote as you are afk.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLVOTECAST")) );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		ent->client->pers.voteValue = qtrue;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		ent->client->pers.voteValue = qfalse;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, cs_offset;
	team_t	team;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADY")) );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXTEAMVOTES")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			Q_strcat(arg2,sizeof(arg2), " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader, (qboolean)(jk2startversion == VERSION_1_02), qtrue);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname, (qboolean)(jk2startversion == VERSION_1_02), qtrue);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s" S_COLOR_WHITE " called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLTEAMVOTECAST")) );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}


	if (!DefragDoubleTapSafety(ent, DOUBLETAP_SETVIEWPOS, "setviewpos")) {
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	DF_RaceStateInvalidated(ent, qtrue);

	TeleportPlayer( ent, origin, angles );
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}

void Cmd_Stay_f(gentity_t* ent) {
	qboolean slowVoteActive = G_SlowVoteActive();
	if (!slowVoteActive && !ent->client->pers.stayOnMap) {
		if (g_slowVote.integer == 1) {
			trap_SendServerCommand(ent - g_entities, "print \"^3Slow voting is not currently active on this server because nobody is in racemode.\n\"");
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"^3Slow voting is not currently active on this server.\n\"");
		}
		return;
	}
	if (!ent->client->pers.stayOnMap) {

		trap_SendServerCommand(ent - g_entities, "print \"^3Locking in this map. Others can not vote for other maps while you are not in spec and not AFK.\n\"");
		ent->client->pers.stayOnMap = qtrue;
	}
	else {

		trap_SendServerCommand(ent - g_entities, "print \"^1You are no longer locking this map. People can vote for another map.\n\"");
		ent->client->pers.stayOnMap = qfalse;
	}
}

int G_ItemUsable(playerState_t *ps, int forcedUse, gentity_t* ent)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->usingATST)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] + ent->client->bactaExtra)
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		JP_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		JP_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			JP_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	default:
		return 1;
	}
}

extern int saberOffSound;
extern int saberOnSound;

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	int		nowTime = LEVELTIME(ent->client);
	if (!saberOffSound || !saberOnSound)
	{
		saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
		saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
	}

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (g_pauseGame.integer && !ent->client->sess.raceMode) {
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= nowTime)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered)
		{
			ent->client->ps.saberHolstered = qfalse;
			G_Sound(ent, CHAN_AUTO, saberOnSound);
		}
		else
		{
			ent->client->ps.saberHolstered = qtrue;
			G_Sound(ent, CHAN_AUTO, saberOffSound);

			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;

	if ( !ent || !ent->client )
	{
		return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	selectLevel++;
	if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABERATTACK] )
	{
		selectLevel = FORCE_LEVEL_1;
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}

qboolean G_PlayerCanDuel(gentity_t* ent, qboolean message, qboolean challenged) {

	if (g_gametype.integer == GT_TOURNAMENT)
	{ //rather pointless in this mode..
		if (message) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")));
		}
		return qfalse;
	}

	if (g_gametype.integer >= GT_TEAM && ent->client->sess.mode == MODE_NORMAL)
	{ //no private dueling in team modes
		if (message) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")));
		}
		return qfalse;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (ent->health < 1 || ent->client->ps.stats[STAT_HEALTH] < 1) {
		if (ent->client->sess.mode != MODE_DUELQUEUE || !g_duelQueueAutoRespawn.integer) { // if in duel queue with autorespawn, we don't care.
			return qfalse;
		}
	}

	/*
	if (!ent->client->ps.saberHolstered)
	{ //must have saber holstered at the start of the duel
		return;
	}
	*/
	//NOTE: No longer doing this..

	if (ent->client->ps.saberInFlight)
	{
		return qfalse;
	}

	if (ent->client->ps.duelInProgress)
	{
		return qfalse;
	}

	if (ent->client->sess.raceMode) {
		return qfalse;
	}

	//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	//if (ent->client->sess.mode == MODE_DUELQUEUE) {
	//	if (ent->client->pers.lastDuel + g_duelQueueTimeout.integer > level.time) { // privateDuelTime desyncs cuz it runs on clientthink_Real...
	//		return qfalse;
	//	}
	//}
	//else 
	if (ent->client->sess.mode != MODE_DUELQUEUE && ent->client->ps.fd.privateDuelTime > level.time && !challenged)
	{
		if (message) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_JUSTDID")));
		}
		return qfalse;
	}
	return qtrue;
}

void G_StartDuel(gentity_t* ent, gentity_t* challenged, qboolean message) {
	if (message) {
		trap_SendServerCommand( /*challenged-g_entities*/-1, va("print \"%s" S_COLOR_WHITE " %s %s" S_COLOR_WHITE "!\n\"", challenged->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELACCEPT"), ent->client->pers.netname));
	}

	ent->client->ps.duelInProgress = qtrue;
	challenged->client->ps.duelInProgress = qtrue;

	ent->client->ps.duelTime = level.time + 2000;
	challenged->client->ps.duelTime = level.time + 2000;

	G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
	G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

	//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)

	if (!ent->client->ps.saberHolstered)
	{
		G_Sound(ent, CHAN_AUTO, saberOffSound);
		ent->client->ps.weaponTime = 400;
		ent->client->ps.saberHolstered = qtrue;
	}
	if (!challenged->client->ps.saberHolstered)
	{
		G_Sound(challenged, CHAN_AUTO, saberOffSound);
		challenged->client->ps.weaponTime = 400;
		challenged->client->ps.saberHolstered = qtrue;
	}
	
	ent->client->ps.duelIndex = challenged - g_entities;
	challenged->client->ps.duelIndex = ent - g_entities;

	ent->client->pers.lastDueled[challenged - g_entities] = level.time;
	challenged->client->pers.lastDueled[ent - g_entities] = level.time;

	ent->client->pers.lastDuelStart = ent->client->pers.lastDuel = challenged->client->pers.lastDuelStart = challenged->client->pers.lastDuel = level.time;

	ent->client->ps.stats[STAT_ARMOR] =
		ent->client->ps.stats[STAT_HEALTH] =
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
	challenged->client->ps.stats[STAT_ARMOR] =
		challenged->client->ps.stats[STAT_HEALTH] =
		challenged->health = challenged->client->ps.stats[STAT_MAX_HEALTH];
}

void Cmd_EngageDuel_f(gentity_t *ent)
{
	trace_t tr;
	vec3_t forward, fwdOrg;
	int		nowTime = LEVELTIME(ent->client);

	if (!g_privateDuel.integer)
	{
		return;
	}
	if (ent->client->sess.mode == MODE_DUELQUEUE) // managed automatically
	{
		return;
	}

	if (!G_PlayerCanDuel(ent,qtrue,qfalse)) {
		return;
	}

	if (G_OtherPlayersDueling())
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_BUSY")) );
		return;
	}

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	JP_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t *challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			!G_PlayerCanDuel(challenged,qfalse,qtrue))
		{
			return;
		}

		if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
		{
			return;
		}

		ent->client->ps.duelTime = level.time + 5000; // put this higher up so G_StartDuel can overwrite it so that our duel duration calculation is correct. all pretty dumb probably idk.

		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
		{
			G_StartDuel(ent, challenged, qtrue);
		}
		else
		{
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			G_CenterPrint( challenged-g_entities, 3, va("%s" S_COLOR_WHITE " %s", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELCHALLENGE")),qfalse,qtrue,qfalse, NULL);
			G_CenterPrint( ent-g_entities, 3, va("%s %s", G_GetStripEdString("SVINGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname),qfalse,qtrue ,qfalse, NULL);
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = nowTime + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
	}
}



const char* G_ClientNameWithPrefix(gentity_t* ent, const int maxNameChars) {

	const char col = ColorCodeForClient(ent, qfalse);
	static char buf[256] = { 0 };

	memset(&buf, 0, 256);

	if (ent && ent->client)
		Com_sprintf(buf, sizeof(buf), "%02d^%c) ^7 %s", (int)(ent - g_entities), col, maxNameChars > 0 ?
			G_ClientNameFixedLength(ent, maxNameChars) :
			ent->client->pers.netname);
	else
		Q_strncpyz(buf, "?", sizeof(buf));

	return &buf[0];
}

#define G_MAXBUFNAMELEN	256		//...
//if clampMax is true, dont let string be longer than numChars visible chars
const char* G_StringFixedLength(const char* str, int numChars, char fillChar, qboolean clampMax) {
	int nc = 0, m;

	const char* ret;
	const int _strlen = strlen(str);
	static char outName[2][G_MAXBUFNAMELEN];
	static int index = 0;

	if (numChars <= 0)
		numChars = 1024;

	for (m = 0; nc < numChars && m < G_MAXBUFNAMELEN - 1; ++m, ++nc) {
		int c;

		if (m < _strlen) {
			c = str[m];

			if (Q_IsColorString(str + m)) {
				nc -= 2;	//this char and the next will not count
			}
		}
		else if (!clampMax) {
			break;
		}
		else {
			c = fillChar;
		}

		outName[index][m] = c;
	}
	outName[index][m] = 0;

	ret = outName[index];
	index ^= 1;
	return ret;
}


const char* G_ClientNameFixedLength(gentity_t* ent, const int numChars) {
	return G_StringFixedLength(ent->client->pers.netname, numChars, ' ', qtrue);
}

//"cheap" findplayer version for use by clients. Only checks clean names, and only checks Q_stricmp and q_stristr (both case insensitive)
//ent = guy who wants to find someone.
int G_FindPlayerFromStringCheap(gentity_t* ent, const char* find, qboolean trySkipSpecs) {
	int			i, k;
	int ret = -1;
	gclient_t* cl;

	if (!find || !find[0])
		return -1;

	if (ent && ent->client && ent->inuse && !Q_stricmp(find, "self")) {
		return ent - g_entities;
	}

	if (find[0] >= '0' && find[0] <= '9') {
		i = atoi(find);

		if (i >= 0 && i < MAX_CLIENTS && level.clients[i].pers.connected != CON_DISCONNECTED) {
			return i;
		}

		return -1;
	}

	for (k = 0; k < 2; ++k) {
		for (i = 0, cl = level.clients; i < MAX_CLIENTS; ++i, ++cl) {
			const char* name = NULL;

			if (!cl || cl->pers.connected == CON_DISCONNECTED)
				continue;

			if (trySkipSpecs && cl->sess.sessionTeam == TEAM_SPECTATOR)
				continue;

			name = cl->pers.netnameClean;

			switch (k)
			{
			case 0:
				if (!Q_stricmp(name, find)) ret = i;		//case insensitive string compare.
				break;
			case 1:
				if (Q_stristr(name, find)) ret = i;		//case insensitive substring search.
				break;
			}
		}
	}

	if (trySkipSpecs && ret == -1) {
		return G_FindPlayerFromStringCheap(ent, find, qfalse);
	}

	return ret;
}

//this thing is rather ineffective/slow, but that should not be a problem since its only used for server / rcon commands.
int G_FindPlayerFromString(const char* buf) {
	int			i, k;
	gclient_t* cl;

	if (!buf || !buf[0])
		return -1;

	if (buf[0] >= '0' && buf[0] <= '9') {
		i = atoi(buf);

		if (i >= 0 && i < MAX_CLIENTS && level.clients[i].pers.connected != CON_DISCONNECTED) {
			return i;
		}

		return -1;
	}

	for (k = 0; k < 8; ++k) {
		for (i = 0, cl = level.clients; i < MAX_CLIENTS; ++i, ++cl) {
			char		nameStr[256] = { 0 };
			const char* name;

			if (!cl || cl->pers.connected == CON_DISCONNECTED)
				continue;


			if (k % 2) {
				name = cl->pers.netnameClean;
			}
			else {
				name = cl->pers.netname;
			}

			switch (k)
			{
			case 0:
			case 1:
				if (!strcmp(name, buf)) return i;			//case sensitive string compare.
				break;
			case 2:
			case 3:
				if (!Q_stricmp(name, buf)) return i;		//case insensitive string compare.
				break;
			case 4:
			case 5:
				if (strstr(name, buf)) return i;		//case sensitive substring search.
				break;
			case 6:
			case 7:
				if (Q_stristr(name, buf)) return i;		//case insensitive substring search.
				break;
			}
		}
	}

	return -1;	// Client isnt valid
}

char ColorCodeForClient(gentity_t* target, qboolean botSpecialColor) {
	char col = '7';

	//set a special color according to team or connect status
	if (botSpecialColor && target->r.svFlags & SVF_BOT) {
		col = '5';	//Cyan for bots!
	}
	else if (target->client->pers.connected == CON_CONNECTING) {
		col = '2';
	}
	else if (target->client->sess.sessionTeam == TEAM_BLUE) {
		col = '4';
	}
	else if (target->client->sess.sessionTeam == TEAM_RED) {
		col = '1';
	}
	else if (target->client->sess.sessionTeam == TEAM_SPECTATOR) {
		col = '3';
	}

	return col;

}



void PM_SetAnim(int setAnimParts,int anim,int setAnimFlags, int blendTime);

#if 1//defined(_DEBUG)
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;
	pmove_t pmv;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = JP_Trace;
	pmv.rawtrace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, i, flags, 0);
	if (pmv.ps->pm_type >= PM_DEAD) {
		pmv.ps->legsAnim =
			((pmv.ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | i;
		pmv.ps->torsoAnim =
			((pmv.ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | i;
	}

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	pmove_t pmv;

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = JP_Trace;
	pmv.rawtrace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t *self);

#if 1 // defined(_DEBUG)
void DismembermentByNum(gentity_t *self, int num);
#endif
extern void Cmd_Race_f(gentity_t* ent);
extern void Cmd_Mode_f(gentity_t* ent);
extern void Cmd_ModeCmd_f(gentity_t* ent);
extern void Cmd_JumpChange_f(gentity_t* ent);
extern void Cmd_DF_RunSettings_f(gentity_t* ent);
extern void Cmd_DF_RestartSegmentedRun_f(gentity_t* ent);
extern void Cmd_MovementStyle_f(gentity_t* ent);
extern void DF_SaveSpawn(gentity_t* ent);
extern void DF_SelectSpawn(gentity_t* ent);
extern void DF_ResetSpawn(gentity_t* ent);
extern void Cmd_ToggleFPS_f(gentity_t* ent);
extern void Cmd_FloatPhysics_f(gentity_t* ent);
extern void DF_CreateCustomCheckpoint_Cmd(gentity_t* playerent);
extern void DF_RemoveCheckPoints_Cmd(gentity_t* playerent);
extern void DF_StealCheckpoints(gentity_t* playerent);
extern void DF_StealSpawn(gentity_t* playerent);
extern void DF_StealPos(gentity_t* playerent);
extern void G_DB_SaveUserCheckpoints(gentity_t* playerent);
extern void G_DB_LoadUserCheckpoints(gentity_t* playerent);


static void Cmd_SpectatorList_f (gentity_t *sendToMe) {
	int i;
	gentity_t *ent = g_entities;
	const int clNum = sendToMe - g_entities;
	int c = 0;
	qboolean matchme = qfalse;

	if (!Q_stricmp(G_Argv(1), "me"))
		matchme = qtrue;

	trap_SendServerCommand(clNum,va("print \"%s%24s     %s\n\"", "     ", "^5spectators", "speccing"));

	for (i = 0; i < MAX_CLIENTS; ++i, ++ent) {

		if (!ent || !ent->client || ent->client->pers.connected != CON_CONNECTED || ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			continue;

		if (matchme && ent->client->ps.clientNum != clNum)
			continue;

		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW &&
			ent->client->ps.clientNum >= 0 && ent->client->ps.clientNum < MAX_CLIENTS) {

			char extra[8] = {0};

			if (ent->client->ps.clientNum == clNum) {
				//he is speccing me
				Q_strncpyz(extra, " (YOU)", sizeof(extra));
			}

			trap_SendServerCommand( clNum,va("print \"%s^7   %s^7 (%d)%s\n\"", G_ClientNameWithPrefix(ent, 24),
				SHOWNAME(ent->client->ps.clientNum), ent->client->ps.clientNum, extra ));
		} else {
			trap_SendServerCommand( clNum,va("print \"%24s^7   (nobody)\n\"", G_ClientNameWithPrefix(ent, 24)) );
		}
		++c;
	}

	if (!c)
		trap_SendServerCommand( clNum, "print \"<no spectators to list>\n\"" );
}

const char* G_Argv(int arg) {
	static char buf[256];

	trap_Argv(arg, buf, 256);

	return &buf[0];
}

static void Cmd_FollowNext_f(gentity_t* ent) {
	Cmd_FollowCycle_f(ent, 1);
}

static void Cmd_FollowPrev_f(gentity_t* ent) {
	Cmd_FollowCycle_f(ent, -1);
}

static void Cmd_AddBot_f(gentity_t* ent) {
	//because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
//			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
	trap_SendServerCommand(ent-g_entities, va("print \"%s.\n\"", G_GetStripEdString("SVINGAME", "ONLY_ADD_BOTS_AS_SERVER")));
}

static void Cmd_TheDestroyer_f(gentity_t* ent) {
	if (CheatsOk(ent) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
		Cmd_ToggleSaber_f(ent);

		if (!ent->client->ps.saberHolstered)
		{
			if (ent->client->ps.dualBlade)
			{
				ent->client->ps.dualBlade = qfalse;
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
			}
			else
			{
				ent->client->ps.dualBlade = qtrue;

				trap_SendServerCommand(-1, va("print \"%sTHE DESTROYER COMETH\n\"", S_COLOR_RED));
				G_ScreenShake(vec3_origin, NULL, 10.0f, 800, qtrue);
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
			}
		}
	}
}

static void Cmd_Debug_HeadExplodeY_f(gentity_t* ent) {
	if ( CheatsOk(ent))
	{
		Cmd_Kill_f(ent);
		if (ent->health < 1)
		{
			float presaveVel = ent->client->ps.velocity[2];
			ent->client->ps.velocity[2] = 500;
			DismembermentTest(ent);
			ent->client->ps.velocity[2] = presaveVel;
		}
	}
}

static void Cmd_Debug_G2AnimEnt_f(gentity_t* ent) {
	if (CheatsOk(ent))
	{
		G_CreateExampleAnimEnt(ent);
	}
}

static void Cmd_Debug_LoveAndPeace_f(gentity_t* ent) {
	if (CheatsOk(ent))
	{
		trace_t tr;
		vec3_t fPos;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

		fPos[0] = ent->client->ps.origin[0] + fPos[0] * 40;
		fPos[1] = ent->client->ps.origin[1] + fPos[1] * 40;
		fPos[2] = ent->client->ps.origin[2] + fPos[2] * 40;

		JP_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

		if (tr.entityNum < MAX_CLIENTS && tr.entityNum != ent->s.number)
		{
			gentity_t* other = &g_entities[tr.entityNum];

			if (other && other->inuse && other->client)
			{
				vec3_t entDir;
				vec3_t otherDir;
				vec3_t entAngles;
				vec3_t otherAngles;

				if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(ent);
				}

				if (other->client->ps.weapon == WP_SABER && !other->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(other);
				}

				if ((ent->client->ps.weapon != WP_SABER || ent->client->ps.saberHolstered) &&
					(other->client->ps.weapon != WP_SABER || other->client->ps.saberHolstered))
				{
					VectorSubtract(other->client->ps.origin, ent->client->ps.origin, otherDir);
					VectorCopy(ent->client->ps.viewangles, entAngles);
					entAngles[YAW] = vectoyaw(otherDir);
					DF_PreDeltaAngleChange(ent->client);
					SetClientViewAngle(ent, entAngles);
					DF_PostDeltaAngleChange(ent->client, qtrue);

					StandardSetBodyAnim(ent, BOTH_KISSER1LOOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
					ent->client->ps.saberMove = LS_NONE;
					ent->client->ps.saberBlocked = 0;
					ent->client->ps.saberBlocking = 0;

					VectorSubtract(ent->client->ps.origin, other->client->ps.origin, entDir);
					VectorCopy(other->client->ps.viewangles, otherAngles);
					otherAngles[YAW] = vectoyaw(entDir);
					DF_PreDeltaAngleChange(other->client);
					SetClientViewAngle(other, otherAngles);
					DF_PostDeltaAngleChange(other->client, qtrue);

					StandardSetBodyAnim(other, BOTH_KISSEE1LOOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS);
					other->client->ps.saberMove = LS_NONE;
					other->client->ps.saberBlocked = 0;
					other->client->ps.saberBlocking = 0;
				}
			}
		}
	}
}

static void Cmd_Debug_DebugSetBodyAnim_f(gentity_t* ent) {
	Cmd_DebugSetBodyAnim_f(ent, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
}
static void Cmd_Debug_DebugKnockMeDown_f(gentity_t* ent) {
	int		nowTime = LEVELTIME(ent->client);
	ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
	ent->client->ps.forceDodgeAnim = 0;
	if (trap_Argc() > 1)
	{
		ent->client->ps.forceHandExtendTime = nowTime + 1100;
		ent->client->ps.quickerGetup = qfalse;
	}
	else
	{
		ent->client->ps.forceHandExtendTime = nowTime + 700;
		ent->client->ps.quickerGetup = qtrue;
	}
}
static void Cmd_Debug_DebugDismemberment_f(gentity_t* ent) {

	Cmd_Kill_f(ent);
	if (ent->health < 1)
	{
		char	arg[MAX_STRING_CHARS];
		int		iArg = 0;

		if (trap_Argc() > 1)
		{
			trap_Argv(1, arg, sizeof(arg));

			if (arg[0])
			{
				iArg = atoi(arg);
			}
		}

		DismembermentByNum(ent, iArg);
	}
}


static void Cmd_AltFolow_f( gentity_t *ent ) {
	if (!ent || !ent->client)
		return;

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities,"print \"You need to be on spectator team to use this feature.\n\"");
		return;
	}


	//setting persists through maprestarts
	ent->client->sess.amflags ^= AMFLAG_ALTFOLLOW;
	if (ent->client->sess.amflags & AMFLAG_ALTFOLLOW)
		trap_SendServerCommand(ent - g_entities,"print \"Alt-follow is now ^2enabled^7 (use alt-attack to spectate the previous player.)\n\"");
	else
		trap_SendServerCommand(ent - g_entities,"print \"Alt-follow is now ^1disabled^7.\n\"");
}

//Stuff from openJK
#define CMD_NOINTERMISSION		(1<<0)
#define CMD_INTERMISSIONUNKNOWN	(1<<1) // not explicitly blocked by intermission but not known either. will be printed as say. dumb, but keeping it to maintain behavior
#define CMD_CHEAT				(1<<2)
#define CMD_ALIVE				(1<<3)
#define CMD_ALLOWINREPLAY		(1<<4) // allow during a segmented run replay
#define CMD_ALLOWWHENFORCELOGIN (1<<5) // allow when force logged in by admin (we wanna encourage changing password so we disable most cmds)
#define CMD_DEBUG				(1<<6) // cmds hidden behind _DEBUG
#define CMD_SIGNALSPRESENCE		(1<<7) // usage of this cmd resets the afk timer for a player

typedef struct command_s {
	const char* name;	//string that invokes the command
	const char* args;	//Args mask
	void		(*func)(gentity_t* ent);	//function to execute
	int			flags;
	const char* desc;	//description
} clientCommand_t;

//for bsearch
int cmdcmp(const void* a, const void* b) {
	return Q_stricmp((const char*)a, ((clientCommand_t*)b)->name);
}

clientCommand_t clientCommands[] = {
	{"addbot",				NULL, Cmd_AddBot_f,						CMD_INTERMISSIONUNKNOWN},
#ifdef _DEBUG
	{"afk",					NULL, Cmd_Afk_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY},
#else
	{"afk",					NULL, Cmd_Afk_f,						CMD_NOINTERMISSION},
#endif
	{"allforce",			NULL, Cmd_ModeCmd_f,					CMD_NOINTERMISSION},
	{"altf",				NULL, Cmd_AltFolow_f,					CMD_NOINTERMISSION, "Toggle being able to use alt-attack to spec the previous player." },
	{"amMap",				NULL, Cmd_AmMap_f,						CMD_NOINTERMISSION},
	{"amtele",				NULL, Cmd_Amtele_f,						CMD_NOINTERMISSION},
	{"arenaless",			NULL, Cmd_Arenaless_f,					CMD_NOINTERMISSION},
	{"blacklistmap",		NULL, Cmd_BlacklistMap_f,				CMD_NOINTERMISSION},
	{"callteamvote",		NULL, Cmd_CallTeamVote_f,				CMD_NOINTERMISSION},
	{"callvote",			NULL, Cmd_CallVote_f,					CMD_NOINTERMISSION},
	{"changepassword",		NULL, Cmd_ChangePassword_f,				CMD_NOINTERMISSION | CMD_ALLOWWHENFORCELOGIN},
	{"checkpoint",			NULL, DF_CreateCustomCheckpoint_Cmd,	CMD_NOINTERMISSION},
	{"dbsrecords",			NULL, Cmd_DBSRecords_f,					CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"debugDismemberment",	NULL, Cmd_Debug_DebugDismemberment_f,	CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"debugKnockMeDown",	NULL, Cmd_Debug_DebugKnockMeDown_f,		CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"debugSetBodyAnim",	NULL, Cmd_Debug_DebugSetBodyAnim_f,		CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"debugSetSaberMove",	NULL, Cmd_DebugSetSaberMove_f,			CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"demoManage",			NULL, Cmd_DemoManage_f,					CMD_NOINTERMISSION},
	{"duel",				NULL, Cmd_ModeCmd_f,					CMD_NOINTERMISSION},
	{"duelq",				NULL, Cmd_ModeCmd_f,					CMD_NOINTERMISSION},
	{"duelqueue",			NULL, Cmd_ModeCmd_f,					CMD_NOINTERMISSION},
	{"easiest",				NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"floatphysics",		NULL, Cmd_FloatPhysics_f,				CMD_NOINTERMISSION},
	{"follow",				NULL, Cmd_Follow_f,						CMD_NOINTERMISSION},
	{"follownext",			NULL, Cmd_FollowNext_f,					CMD_NOINTERMISSION},
	{"followprev",			NULL, Cmd_FollowPrev_f,					CMD_NOINTERMISSION},
	{"forcechanged",		NULL, Cmd_ForceChanged_f,				0},
	{"forcelogin",			NULL, Cmd_ForceLogin_f,					CMD_NOINTERMISSION},
	{"freedom",				NULL, Cmd_NameTag_f,					CMD_NOINTERMISSION},
	{"g2animent",			NULL, Cmd_Debug_G2AnimEnt_f,			CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"gc",					NULL, Cmd_GameCommand_f,				CMD_NOINTERMISSION},
	{"genArena",			NULL, Cmd_GenArena_f,					CMD_NOINTERMISSION},
	{"give",				NULL, Cmd_Give_f,						CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"god",					NULL, Cmd_God_f,						CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"hardest",				NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"headexplodey",		NULL, Cmd_Debug_HeadExplodeY_f,			CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"help",				NULL, Cmd_Help_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"ignore",				NULL, Cmd_Ignore_f,						CMD_NOINTERMISSION},
	{"ironman",				NULL, Cmd_ModeCmd_f,					CMD_NOINTERMISSION},
	{"jump",				NULL, Cmd_JumpChange_f,					CMD_NOINTERMISSION},
	{"kill",				NULL, Cmd_Kill_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"lasers",				NULL, Cmd_Lasers_f,						CMD_NOINTERMISSION},
	{"latest",				NULL, Cmd_Latest_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"launch",				NULL, Cmd_Launch_f,						CMD_NOINTERMISSION},
	{"levelshot",			NULL, Cmd_LevelShot_f,					CMD_NOINTERMISSION},
	{"loadcheckpoints",		NULL, G_DB_LoadUserCheckpoints,			CMD_NOINTERMISSION},
	{"login",				NULL, Cmd_Login_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"logout",				NULL, Cmd_Logout_f,						CMD_NOINTERMISSION | CMD_ALLOWWHENFORCELOGIN | CMD_SIGNALSPRESENCE},
	{"longest",				NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"loveandpeace",		NULL, Cmd_Debug_LoveAndPeace_f,			CMD_INTERMISSIONUNKNOWN | CMD_DEBUG},
	{"mapdefaults",			NULL, Cmd_DF_MapDefaults_f,				CMD_NOINTERMISSION},
	{"maplist",				NULL, Cmd_Maplist_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"messages",			NULL, G_Cmd_UserMessages,				CMD_NOINTERMISSION,	"User account message sending/receiving (don't use for anything private)"},
	{"mostplayed",			NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"move",				NULL, Cmd_MovementStyle_f,				CMD_NOINTERMISSION},
	{"noclip",				NULL, Cmd_Noclip_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"notarget",			NULL, Cmd_Notarget_f,					CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION},
	{"notwr",				NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"pickmode",			NULL, Cmd_Mode_f,						CMD_NOINTERMISSION},
	{"players",				NULL, Cmd_Players_f,					CMD_NOINTERMISSION},
	{"race",				NULL, Cmd_Race_f,						CMD_NOINTERMISSION},
	{"rank",				NULL, Cmd_Rank_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"ratemap",				NULL, Cmd_RateMap_f,					CMD_NOINTERMISSION},
	{"register",			NULL, Cmd_Register_f,					CMD_NOINTERMISSION},
	{"removecheckpoints",	NULL, DF_RemoveCheckPoints_Cmd,			CMD_NOINTERMISSION},
	{"resetspawn",			NULL, DF_ResetSpawn,					CMD_NOINTERMISSION},
	{"respos",				NULL, Cmd_Respos_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"resseg",				NULL, Cmd_DF_RestartSegmentedRun_f,		CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"rollympics",			NULL, Cmd_Rollympics_f,					CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"run",					NULL, Cmd_DF_RunSettings_f,				CMD_NOINTERMISSION},
	{"savecheckpoints",		NULL, G_DB_SaveUserCheckpoints,			CMD_NOINTERMISSION},
	{"savepos",				NULL, Cmd_Savepos_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"savespawn",			NULL, DF_SaveSpawn,						CMD_NOINTERMISSION},
	{"say",					NULL, Cmd_SayNormal_f,					CMD_ALLOWINREPLAY | CMD_ALLOWWHENFORCELOGIN | CMD_SIGNALSPRESENCE},
	{"say_cross",			NULL, Cmd_SayCross_f,					CMD_ALLOWINREPLAY | CMD_ALLOWWHENFORCELOGIN | CMD_SIGNALSPRESENCE},
	{"say_team",			NULL, Cmd_SayTeam_f,					CMD_ALLOWINREPLAY | CMD_ALLOWWHENFORCELOGIN | CMD_SIGNALSPRESENCE},
	{"score",				NULL, Cmd_Score_f,						CMD_ALLOWINREPLAY | CMD_ALLOWWHENFORCELOGIN},
	{"selectspawn",			NULL, DF_SelectSpawn,					CMD_NOINTERMISSION},
	{"setviewpos",			NULL, Cmd_SetViewpos_f,					CMD_CHEAT | CMD_NOINTERMISSION},
	{"shortest",			NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"solo",				NULL, Cmd_Solo_f,						CMD_NOINTERMISSION},
	{"specs",				NULL, Cmd_SpectatorList_f,				CMD_NOINTERMISSION, "Print a list of spectators and whom they are spectating." },
	{"stats",				NULL, Cmd_Stats_f,						CMD_NOINTERMISSION},
	{"stay",				NULL, Cmd_Stay_f,						CMD_NOINTERMISSION},
	{"stealcheckpoints",	NULL, DF_StealCheckpoints,				CMD_NOINTERMISSION},
	{"stealpos",			NULL, DF_StealPos,						CMD_NOINTERMISSION},
	{"stealspawn",			NULL, DF_StealSpawn,					CMD_NOINTERMISSION},
	{"tag",					NULL, Cmd_TagMap_f,						CMD_NOINTERMISSION, "Tag a map, search for tagged maps or list a map's tags."},
	{"team",				NULL, Cmd_Team_f,						CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"teamtask",			NULL, Cmd_TeamTask_f,					CMD_NOINTERMISSION},
	{"teamvote",			NULL, Cmd_TeamVote_f,					CMD_NOINTERMISSION},
	{"tell",				NULL, Cmd_Tell_f,						CMD_ALLOWINREPLAY | CMD_ALLOWWHENFORCELOGIN | CMD_SIGNALSPRESENCE},
	{"test",				NULL, Cmd_Test_f,						CMD_NOINTERMISSION},
	{"tffaStats",			NULL, Cmd_TFFAStats_f,					CMD_NOINTERMISSION},
	{"thedestroyer",		NULL, Cmd_TheDestroyer_f,				CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION}, // technically there should be a fallthrough to "unknown cmd" here but meh.
	{"time",				NULL, Cmd_Time_f,						CMD_NOINTERMISSION},
	{"togglefps",			NULL, Cmd_ToggleFPS_f,					CMD_NOINTERMISSION},
	{"top",					NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topcheat",			NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topcustom",			NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topmain",				NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topnjb",				NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topnojumpbug",		NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"toprated",			NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE},
	{"topseg",				NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"topsegmented",		NULL, Cmd_Top_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY | CMD_SIGNALSPRESENCE},
	{"updateRanks",			NULL, Cmd_UpdateRanks_f,				CMD_NOINTERMISSION},
	{"vote",				NULL, Cmd_Vote_f,						CMD_NOINTERMISSION},
	{"where",				NULL, Cmd_Where_f,						CMD_NOINTERMISSION | CMD_ALLOWINREPLAY },
	{"wrs",					NULL, Cmd_MapSearch_f,					CMD_NOINTERMISSION | CMD_SIGNALSPRESENCE },
};

static const size_t numCommands = ARRAY_LEN(clientCommands);


static void Cmd_PrintCmdDescriptions_f(gentity_t* ent)
{
	const clientCommand_t* cmd;
	const int 				entNum = ent - g_entities;
	int 					i;
	
	trap_SendServerCommand(entNum, "print \"\n^7Other client commands:\n\"");

	for (i = 0, cmd = clientCommands; i < numCommands; ++cmd, ++i)
	{
		if (cmd && cmd->desc) {
			trap_SendServerCommand(ent - g_entities, va("print \"^2/%s^7 - %s\n\"",cmd->name,cmd->desc));
			//trap_SendServerCommand(entNum, va("print \" ^1/%s ^7%s\n"
			//	"    %s\n\"", cmd->name, cmd->args ? cmd->args : "", cmd->desc));
		}
	}

}


//For qsort
int cmdcmp2(const void* a, const void* b) {
	return strcmp(((clientCommand_t*)a)->name, ((clientCommand_t*)b)->name);
}

void G_InitClientCommands(void) {
	//Sort the commands array alphabetically to allow for binary search.
	qsort(clientCommands, numCommands, sizeof(clientCommands[0]), cmdcmp2);
}



/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];
	char token[BIG_INFO_STRING]; // As the engine uses Cmd_TokenizeString2 a single parameter is theoretically not limited by MAX_TOKEN_CHARS, but by BIG_INFO_STRING
	int i, argc;
	int		nowTime = LEVELTIME((g_entities+clientNum)->client);
	clientCommand_t* command;


	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected < CON_CONNECTED ) {
		return;		// not fully in game yet
	}

	// Filter '\n' and '\r'
	argc = trap_Argc();
	for ( i = 0; i < argc; i++ )
	{
		trap_Argv( i, token, sizeof(token) );
		if ( strchr(token, '\n') || strchr(token, '\r') )
		{
			trap_SendServerCommand( clientNum, "print \"Invalid input - command blocked.\n\"" );
			G_Printf("ClientCommand: client '%i' (%s) tried to use an invalid command - command blocked.\n", clientNum, ent->client->pers.netname);
			return;
		}
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );


	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	command = (clientCommand_t*)bsearch(cmd, clientCommands, numCommands, sizeof(clientCommands[0]), cmdcmp);

	if (command && (command->flags & CMD_SIGNALSPRESENCE)) {
		ent->client->sess.lastHereTime = level.time; // for afk tracking for players
	}

	if (!command) {
		//
		if (level.intermissiontime) {
			Cmd_Say_f(ent, qfalse, qtrue);
		}
		else {
			trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
		}
	}
	else if ((command->flags & CMD_NOINTERMISSION) && (/*level.intermissionQueued ||*/ level.intermissiontime))
	{
		trap_SendServerCommand(clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd));
	}
	else if ((command->flags & CMD_INTERMISSIONUNKNOWN) && (/*level.intermissionQueued ||*/ level.intermissiontime))
	{
		// replicate pointless vanilla behavior :)
		Cmd_Say_f(ent, qfalse, qtrue);
	}
	else if ((command->flags & CMD_CHEAT) && !g_cheats.integer)
	{
		trap_SendServerCommand(clientNum, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
	}
	else if ((command->flags & CMD_ALIVE) && (ent->health <= 0 || ent->client->sess.sessionTeam == TEAM_SPECTATOR))
	{
		trap_SendServerCommand(clientNum, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
	}
	else if (!(command->flags & CMD_ALLOWINREPLAY) && DF_ClientInSegmentedRunMode(ent->client) && ent->client->pers.segmented.state >= SEG_REPLAY)
	{
		trap_SendServerCommand(clientNum, "print \"Cannot send commands during segmented run replay.\n\"");
	}
	else if (!(command->flags & CMD_ALLOWWHENFORCELOGIN) && ent->client->sess.login.forceLoggedIn)
	{
		trap_SendServerCommand(clientNum, "print \"^3You cannot send most commands because you were force-logged in by an admin. Please change your password with /changepassword, logout and log in again.\n\"");
	}
#ifndef _DEBUG
	else if ((command->flags & CMD_DEBUG) && !g_debugCommandsEnable.integer) {
		// only available with _DEBUG or g_debugCommandsEnable
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
	}
#endif
	else {
		command->func(ent);
	}



	/*
	if (Q_stricmp (cmd, "vsay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		Cmd_VoiceTaunt_f ( ent );
		return;
	}
	*/


	
	/*
	else if (Q_stricmp(cmd, "#mm") == 0 && CheatsOk( ent ))
	{
		G_PlayerBecomeATST(ent);
	}
	*/
	//I broke the ATST when I restructured it to use a single global anim set for all client animation.
	//You can fix it, but you'll have to implement unique animations (per character) again.

	//trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}
