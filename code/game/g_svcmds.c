// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

// extern	vmCvar_t	g_banIPs;
// extern	vmCvar_t	g_filterBan;


typedef struct ipFilter_s
{
	unsigned	mask;
	unsigned	compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, ipFilter_t *f)
{
	char		num[128];
	int			i, j;
	unsigned	compare = 0;
	unsigned	mask = 0;
	byte		*c = (byte *)&compare;
	byte		*m = (byte *)&mask;

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		c[i] = atoi(num);
		if (c[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = mask;
	f->compare = compare;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans (void)
{
	byte	*b;
	int		i;
	char	iplist[MAX_INFO_STRING];

	*iplist = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		b = (byte *)&ipFilters[i].compare;
		Com_sprintf( iplist + strlen(iplist), sizeof(iplist) - strlen(iplist), 
			"%i.%i.%i.%i ", (int)b[0], (int)b[1], (int)b[2], (int)b[3]);
	}

	trap_Cvar_Set( "g_banIPs", iplist );
}



/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket (char *from)
{
	int			i;
	unsigned	mask = 0;
	byte		*m = (byte *)&mask;
	char		*p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	for (i=0 ; i<numIPFilters ; i++)
		if ( (mask & ipFilters[i].mask) == ipFilters[i].compare)
			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
}


static void Svcmd_UnpauseGame_f(void) {
	int delay = UNPAUSE_COUNTDOWN;
	//trap_Cvar_Set( PAUSEGAME_CVARNAME, "0");

	if (!g_pauseGame.integer) {
		return;
	}

	//instead of unpausing immediately, count down so players can prepare
	if (trap_Argc() >= 2) {
		delay = atoi(G_Argv(1)) * 1000;

		if (delay < 0)
			delay = UNPAUSE_COUNTDOWN;
		else if (delay > 10000)
			delay = 10000;
	}

	level.unpauseTime = level.time + delay;	//when level.time hits this, unpause!

	G_CenterPrint(-1, 0, "Game was unpaused by admin.", qtrue, qfalse, qtrue, NULL);
}

static void Svcmd_Pausegame_f(void) {
	if (g_pauseGame.integer) {
		return;
	}
	trap_Cvar_Set(PAUSEGAME_CVARNAME, "1");
	G_Printf("Paused game.\n");
	G_CenterPrint(-1, 0, "Game was paused by admin.",qtrue,qfalse,qtrue,NULL);
}

static void Svcmd_PauseToggle_f(void) {
	if (g_pauseGame.integer)
	{
		Svcmd_UnpauseGame_f();
	}
	else {
		Svcmd_Pausegame_f();
	}
}



/*
=================
AddIP
=================
*/
static void AddIP( char *str )
{
	int		i;

	for (i = 0 ; i < numIPFilters ; i++)
		if (ipFilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
			G_Printf ("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}
	
	if (!StringToFilter (str, &ipFilters[i]))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void) 
{
	char *s, *t;
	char		str[MAX_TOKEN_CHARS];

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for (t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if (!s)
			break;
		while (*s == ' ')
			*s++ = 0;
		if (*t)
			AddIP( t );
		t = s;
	}
}


/*
=================
SvCmd_TestTrace_f
=================
*/
void SvCmd_TestTrace_f() {
	vec3_t		origin, origin2;
	vec3_t		mins, maxs;
	qboolean	precise;
	int			contents;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;
	trace_t		trace;

	if (trap_Argc() != 15) {
		Com_Printf("usage: testtrace x y z x y z mins[0] mins[1] mins[2] maxs[0] maxs[1] maxs[2] contents precise(0 1)\n");
		return;
	}

	for (i = 0; i < 3; i++) {
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}
	for (i = 0; i < 3; i++) {
		trap_Argv(i + 4, buffer, sizeof(buffer));
		origin2[i] = atof(buffer);
	}
	for (i = 0; i < 3; i++) {
		trap_Argv(i + 7, buffer, sizeof(buffer));
		mins[i] = atof(buffer);
	}
	for (i = 0; i < 3; i++) {
		trap_Argv(i + 10, buffer, sizeof(buffer));
		maxs[i] = atof(buffer);
	}

	trap_Argv(13, buffer, sizeof(buffer));
	contents = atoi(buffer);

	trap_Argv(14, buffer, sizeof(buffer));
	precise = atoi(buffer);

	memset(&trace, 0, sizeof(trace_t));
	if (precise) {
		JP_TracePrecise(&trace,origin,mins,maxs,origin2,-1,contents);
	}
	else {
		JP_Trace(&trace, origin, mins, maxs, origin2, -1, contents);
	}

	Com_Printf("startsolid: %d, allsolid: %d, contents: %d, endpos: %f %f %f, entitynum %d, fraction %f, normal: %f %f %f\n",trace.startsolid,trace.allsolid,trace.contents,trace.endpos[0], trace.endpos[1], trace.endpos[2],trace.entityNum,trace.fraction,trace.plane.normal[0],trace.plane.normal[1],trace.plane.normal[2]);

}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			G_Printf ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	G_Printf ( "Didn't find %s.\n", str );
}


/*
===================
Svcmd_EntityInfo_f
===================
*/
void	Svcmd_EntityInfo_f(void) {
	int totalents;
	int inuse;
	int i;
	gentity_t* e;

	inuse = 0;
	for (e = &g_entities[0], i = 0; i < level.num_entities; e++, i++) {
		if (e->inuse) {
			inuse++;
		}
	}
	G_Printf("Normal entity slots in use: %i/%i (%i slots allocated)\n", inuse, MAX_GENTITIES, level.num_entities);
	totalents = inuse;

	inuse = 0;
	for (e = &g_entities[MAX_GENTITIES], i = 0; i < level.num_logicalents; e++, i++) {
		if (e->inuse) {
			inuse++;
		}
	}
	G_Printf("Logical entity slots in use: %i/%i (%i slots allocated)\n", inuse, MAX_LOGICENTITIES, level.num_logicalents);
	totalents += inuse;
	G_Printf("Total entity count: %i/%i\n", totalents, MAX_ENTITIESTOTAL);
}


/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e=0;
	int			i;
	int			max = level.num_entities;
	gentity_t		*check = g_entities;

	for (i = 0; i < 2; i++) {
		if (i) {
			check = &g_entities[MAX_GENTITIES];
			e = MAX_GENTITIES;
			max = MAX_GENTITIES+ level.num_logicalents;

			G_Printf("\nLogical:\n");
		}
		for (; e < max; e++, check++) {
			if (!check->inuse) {
				continue;
			}
			G_Printf("%3i:", e);
			switch (check->s.eType) {
			case ET_GENERAL:
				G_Printf("ET_GENERAL          ");
				break;
			case ET_PLAYER:
				G_Printf("ET_PLAYER           ");
				break;
			case ET_ITEM:
				G_Printf("ET_ITEM             ");
				break;
			case ET_MISSILE:
				G_Printf("ET_MISSILE          ");
				break;
			case ET_MOVER:
				G_Printf("ET_MOVER            ");
				break;
			case ET_BEAM:
				G_Printf("ET_BEAM             ");
				break;
			case ET_PORTAL:
				G_Printf("ET_PORTAL           ");
				break;
			case ET_SPEAKER:
				G_Printf("ET_SPEAKER          ");
				break;
			case ET_PUSH_TRIGGER:
				G_Printf("ET_PUSH_TRIGGER     ");
				break;
			case ET_TELEPORT_TRIGGER:
				G_Printf("ET_TELEPORT_TRIGGER ");
				break;
			case ET_INVISIBLE:
				G_Printf("ET_INVISIBLE        ");
				break;
			case ET_GRAPPLE:
				G_Printf("ET_GRAPPLE          ");
				break;
			default:
				G_Printf("%3i                 ", check->s.eType);
				break;
			}

			if (check->classname) {
				G_Printf("%s", check->classname);
			}
			G_Printf("\n");
		}
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

// credit goes to Bucky: https://github.com/Bucky21659/vVv-serverside/
static void Svcmd_ScrambleTeams_f(void)
{
	gclient_t* client = NULL;
	gentity_t* gent = NULL;
	//int			counts[TEAM_NUM_TEAMS] = {0};
	int			players[MAX_CLIENTS] = { 0 };
	int			i = 0, c = 0, m = 2;
	char* team = NULL;

	if (g_gametype.integer < GT_TEAM) {
		G_Printf("This command can only be used in team games.");
		return;
	}

	/*counts[TEAM_RED] = TeamCount(-1, TEAM_RED);
	counts[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE);
	counts[TEAM_FREE] = counts[TEAM_RED] + counts[TEAM_BLUE];
	Com_Printf("old count %i red %i blue %i\n", counts[TEAM_RED], counts[TEAM_BLUE]);*/

	for (i = 0, client = level.clients; i < MAX_CLIENTS; i++, client++)
	{ //build an array of active client numbers
		if (!client || client->pers.connected == CON_DISCONNECTED || client->sess.sessionTeam == TEAM_SPECTATOR) {
			players[i] = -1;
			continue;
		}

		if (client->sess.sessionTeam == TEAM_RED || client->sess.sessionTeam == TEAM_BLUE) {
			players[i] = i;
		}
		/*else if (level.CTF3ModeActive && client->sess.sessionTeam == TEAM_FREE) {
			players[i] = i;
		}*/
		else {
			players[i] = -1;
		}
	}

	/*for (i = 0;i < MAX_CLIENTS;i++) {
		Com_Printf("players[%i] %i\n", i, players[i]);
	}*/

	Q_shuffle(players, sizeof(players) / sizeof(int));

	/*for (i = 0;i < MAX_CLIENTS;i++) {
		Com_Printf("players[%i] %i\n", i, players[i]);
	}*/

	//c = 0;
	/*if (level.CTF3ModeActive)
		m = 3;*/
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		team = NULL;

		if (players[i] < 0 || players[i] >= MAX_CLIENTS)
			continue;

		gent = &g_entities[players[i]];
		if (!gent || !gent->inuse || !gent->client)
			continue;

		client = gent->client;
		if (!client || client->pers.connected == CON_DISCONNECTED || client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.raceMode || client->sess.mode != MODE_NORMAL)
			continue;

		switch (i % m) {
		default: //case 0: //??
			team = "red";
			break;
		case 1:
			team = "blue";
			break;
		case 2: //should only happen in 3 team ctf?
			team = "yellow";
			//if (!level.CTF3ModeActive)
				Com_Printf("this should only be happening in 3 team ctf %i %i %i %s....\n", i, m, (i % m), team);
			break;
		}

		if (gent->r.svFlags & SVF_BOT)
			SetTeam(gent, team); //SetTeam_Bot(gent, team); // there was probably a reason for this but im lazy rn
		else
			SetTeam(gent, team);
		/*c++;
		if (gent->r.svFlags & SVF_BOT)
			SetTeam_Bot(gent, (c % 2) ? "red" : "blue");
		else
			SetTeam(gent, (c % 2) ? "red" : "blue");*/
	}

#if 1 //causes issues if this command is run too many times...
	trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
	level.restarted = qtrue;
#endif
	trap_SendServerCommand(-1, "print \"Teams were scrambled.\n\"");
}


/*
===================
Svcmd_Shuffle_f

Shuffle teams. Simillar to a card shuffle (riffle). Randomly selected
half of players from team RED go to team BLUE and vice-versa. This
method guarantees that there is a noticeable team change, minimizes
chance of getting previous teams in consecutive calls and balances
team counts.

All credit for this goes to fau and his mod https://github.com/aufau/SaberMod
Why do I have both this and Bucky's ScrambleTeams in here? Idk I thought it's hilarious
===================
*/
static void	Svcmd_Shuffle_f(void)
{
	qboolean	change[MAX_CLIENTS] = { qfalse };
	int			count[TEAM_NUM_TEAMS] = { 0 };
	team_t		first, second, team;
	int			i;

	if (g_gametype.integer < GT_TEAM) {
		return;
	}

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected != CON_DISCONNECTED && !level.clients[i].sess.raceMode && level.clients[i].sess.mode == MODE_NORMAL) {
			count[level.clients[i].sess.sessionTeam]++;
		}
	}

	if (count[TEAM_RED] > count[TEAM_BLUE]) {
		first = TEAM_RED;
	}
	else if (count[TEAM_RED] < count[TEAM_BLUE]) {
		first = TEAM_BLUE;
	}
	else {
		first = (rand() & 1) ? TEAM_RED : TEAM_BLUE;
	}

	second = first == TEAM_RED ? TEAM_BLUE : TEAM_RED;

	team = first;
	while (1) {
		int		changed = 0;
		int		left = count[team];
		int		changeNum = count[team] / 2;

		for (i = 0; i < level.maxclients; i++) {
			gclient_t* client = &level.clients[i];

			if (changed >= changeNum) {
				break;
			}

			if (client->pers.connected != CON_DISCONNECTED &&
				client->sess.sessionTeam == team && !client->sess.raceMode && client->sess.mode == MODE_NORMAL)
			{
				left--;

				if (change[i]) {
					continue;
				}

				if (changed + left < changeNum ||
					irand(1, count[team], qfalse, 1) <= changeNum)
				{
					changed++;
					change[i] = qtrue;
					client->sess.sessionTeam = team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
					client->sess.teamLeader = qfalse;
				}
			}
		}

		if (team == second) {
			break;
		}
		team = second;
	}

	CheckTeamLeader(TEAM_RED);
	CheckTeamLeader(TEAM_BLUE);

	for (i = 0; i < level.maxclients; i++) {
		gclient_t* client = &level.clients[i];

		if (client->pers.connected != CON_DISCONNECTED) {
			if (change[i]) {
				ClientUserinfoChanged(i);
				//ClientUpdateConfigString(i);

				if (client->pers.connected == CON_CONNECTED) {
					ClientBegin(i, qfalse);
				}
			}
		}
	}

	CalculateRanks();

	trap_SendServerCommand(-1, "cp \"Shuffled teams.\"");
}




/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/

const char* TeamName(team_t team);


static void Svcmd_LockUserinfo_f(void) {
	char		buf[128];
	int cl;
	const int argc = trap_Argc();

	if (argc < 2) {
		G_Printf("Usage: lockuser <client> -- prevent a client from changing his name.\n");
		return;
	}

	trap_Argv(1, buf, sizeof(buf));

	cl = G_FindPlayerFromString(buf);

	if (cl == -1) {
		G_Printf("Couldn't find player '%s'\n", buf);
		return;
	}

	if (level.clients[cl].sess.amflags & AMFLAG_LOCKEDNAME) {
		level.clients[cl].sess.amflags &= ~AMFLAG_LOCKEDNAME;

		G_Printf("Name is no longer locked for %s ^7(%d)\n", SHOWNAME(cl), cl);
		trap_SendServerCommand(cl, "print \"Your name is no longer locked.\n\"");

		ClientUserinfoChanged(cl);//, qfalse);
	}
	else {
		level.clients[cl].sess.amflags |= AMFLAG_LOCKEDNAME;

		G_Printf("Name was locked for %s ^7(%d)\n", SHOWNAME(cl), cl);
		trap_SendServerCommand(cl, "print \"Your name has been locked.\n\"");
	}
}

const char* G_TeamNameColoured(const int team) {
	if (team == TEAM_RED)
		return "^1RED";
	else if (team == TEAM_BLUE)
		return "^4BLUE";
	else if (team == TEAM_SPECTATOR)
		return "^3SPECTATOR";
	else if (team == TEAM_PLAYING)
		return "^5PLAYERS";

	return "FREE";
}

int G_TeamForString(const char* str, qboolean allowTeamPlaying) {

	if (!Q_stricmp(str, "b") || !Q_stricmp(str, "blue")) {
		return TEAM_BLUE;
	}
	if (!Q_stricmp(str, "r") || !Q_stricmp(str, "red")) {
		return TEAM_RED;
	}
	if (!Q_stricmp(str, "s") || !Q_stricmpn(str, "spec", 4)) {
		return TEAM_SPECTATOR;
	}
	if (!Q_stricmp(str, "free") || !Q_stricmp(str, "f")) {
		return TEAM_FREE;
	}
	if (allowTeamPlaying && (!Q_stricmp(str, "playing") || !Q_stricmp(str, "p"))) {
		return TEAM_PLAYING;
	}

	return -1;
}

static void Svcmd_LockTeam_f(void) {
	char		buf[64];
	int team = -1;
	const int argc = trap_Argc();
	const char* fail = "usage: lockteam <team>                     prevent players from leaving or joining this team\n"
		"       lockteam <client number> [new team] prevent this client from changing his team\n";

	if (argc < 2) {
		if (level.lockedTeams) {
			int team;

			for (team = 0; team < TEAM_NUM_TEAMS; ++team) {
				if (level.lockedTeams & (1 << team))
					G_Printf("%s ^7team is locked.\n", G_TeamNameColoured(team));
			}
		}
		else
			G_Printf("%s", fail);

		return;
	}

	trap_Argv(1, buf, sizeof(buf));

	if (buf[0] >= '0' && buf[0] <= '9') {
		//we want to lock a client's team
		const int clNum = atoi(buf);
		gclient_t* cl;

		if (clNum < 0 || clNum >= MAX_CLIENTS || level.clients[clNum].pers.connected != CON_CONNECTED) {
			G_Printf("That client slot is not in use.\n");
			return;
		}

		cl = &level.clients[clNum];

		if (argc == 3) {
			//We wanna force this client to be on a specific team.
			trap_Argv(2, buf, sizeof(buf));

			team = G_TeamForString(buf, qfalse);

			if (team == -1) {
				G_Printf("Unrecognised team '%s'.\n", buf);
				return;
			}

			if (cl->sess.sessionTeam != team) {
				SetTeam(&g_entities[clNum], buf);
			}

			cl->sess.amflags |= AMFLAG_LOCKEDTEAM;
			G_Printf("%s^7's team has been locked to %s.\n", SHOWNAME(clNum), G_TeamNameColoured(team));
			trap_SendServerCommand(clNum,va("print \"Your team was locked to %s^7.\n\"", G_TeamNameColoured(team)));
			return;
		}

		if (cl->sess.amflags & AMFLAG_LOCKEDTEAM) {
			//Unlock his team.
			cl->sess.amflags &= ~AMFLAG_LOCKEDTEAM;

			trap_SendServerCommand(clNum,"print \"You can now change your team again.\n\"");
			G_Printf("%s^7 (%d) ^7can now change his team again.\n", SHOWNAME(clNum), clNum);
		}
		else {
			//Lock his team.
			cl->sess.amflags |= AMFLAG_LOCKEDTEAM;

			trap_SendServerCommand(clNum,"print \"Your team has been locked.\n\"");
			G_Printf("%s^7 (%d) ^7can no longer change his team.\n", SHOWNAME(clNum), clNum);
		}

		return;
	}

	//We want to lock a team.
	if (!Q_stricmp(buf, "all") || !Q_stricmp(buf, "both"))
	{
		if (!level.lockedTeams) {
			level.lockedTeams |= (1 << TEAM_RED);
			level.lockedTeams |= (1 << TEAM_BLUE);
			G_Printf("Teams ^1RED ^7and ^4BLUE ^7were locked.\n");
		}
		else {
			level.lockedTeams &= ~(1 << TEAM_RED);
			level.lockedTeams &= ~(1 << TEAM_BLUE);
			G_Printf("Teams ^1RED ^7and ^4BLUE ^7were unlocked.\n");
		}
		return;
	}


	team = G_TeamForString(buf, qfalse);

	if (team == -1) {
		G_Printf("%s", fail);
		return;
	}

	if (team == TEAM_SPECTATOR) {
		G_Printf("Spectator team can't be locked.\n");
		return;
	}


	if (level.lockedTeams & (1 << team)) {
		level.lockedTeams &= ~(1 << team);
		trap_SendServerCommand(-1, va("print \"%s ^7team was unlocked.\n\"", G_TeamNameColoured(team)));
	}
	else {
		level.lockedTeams |= (1 << team);
		trap_SendServerCommand(-1, va("print \"%s ^7team was locked.\n\"", G_TeamNameColoured(team)));
	}
}

// TA: I don't understand what this does, so I'm not doing it for now...
//qboolean SetTeam_Bot(gentity_t* ent, const char* s);

static void Svcmd_ForceTeam_f(void) {
	char		str[MAX_TOKEN_CHARS];
	int num = -1;
	int i;
	const char* botsonly = "allbots";
	gentity_t* ent;

	if (trap_Argc() < 3) {
		G_Printf("Usage: forceteam <client or 'all'> <team>\n");
		return;
	}

	// find the player
	trap_Argv(1, str, sizeof(str));

	//force all to a team if we want
	if (!Q_stricmp(str, "all") || !Q_stricmp(str, botsonly)) {
		int team;
		qboolean botsOnly = (qboolean)(Q_stricmp(str, botsonly) == 0);

		trap_Argv(2, str, sizeof(str));

		team = G_TeamForString(str, qfalse);
		if (team == -1) {
			G_Printf("Unrecognised team '%s'.\n", str);
			return;
		}

		if (g_gametype.integer >= GT_TEAM && team == TEAM_FREE) {
			G_Printf("Can't join TEAM_FREE in teamgames.\n");
			return;
		}

		ent = g_entities;
		for (i = 0; i < MAX_CLIENTS; ++i, ++ent) {

			if (!ent || !ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED)
				continue;

			if (ent->r.svFlags & SVF_BOT)
				SetTeam(ent, str); //SetTeam_Bot(ent, str);
			else if (!botsOnly)
				SetTeam(ent, str);
		}

		return;
	}

	//force a player's team
	num = G_FindPlayerFromString(str);	// str is arg1

	if (num == -1) {
		G_Printf("Couldn't find player '%s'\n", str);
		return;
	}

	ent = &g_entities[num];

	trap_Argv(2, str, sizeof(str));

	i = G_TeamForString(str, qfalse);

	if (i == -1 && Q_stricmp(str, "scoreboard") && Q_stricmp(str, "score")) {
		G_Printf("Unrecognized team '%s'\n", str);
		return;
	}

	if (ent->client->sess.sessionTeam == i) {
		G_Printf("He is already on team %s^7.\n", G_TeamNameColoured(i));
		return;
	}


	// set the team
	if (G_IsBot(num))
		SetTeam(ent, str);//SetTeam_Bot(ent, str);
	else
		SetTeam(ent, str);


	G_Printf("%s^7's (%d) team was forced to %s^7.\n", SHOWNAME(num), num, G_TeamNameColoured(i));
	trap_SendServerCommand(num,va("print \"Your team was forced to %s^7.\n\"", G_TeamNameColoured(i)));
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeamSimple_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}



static void Svcmd_Mute_f(void) {
	gclient_t* cl;
	char		str[64];

	int num;
	int newval;
	const int argc = trap_Argc();
	const char* ALLBOTS = "allbots";

	qboolean pubonly = qfalse;

	if (argc < 2) {
		G_Printf("Usage: mute <client> [pub] - mute a client from chatting. if 'pub' is given as second parameter, this client can still use team chat.\n");
		return;
	}

	//check for pubonly
	if (argc == 3 && g_gametype.integer >= GT_TEAM) {

		trap_Argv(2, str, sizeof(str));

		if (!Q_stricmpn(str, "pub", 3)) {
			pubonly = qtrue;
		}
		else {
			G_Printf("Unknown second parameter '%s' (only 'pub' is a valid second parameter)\n", str);
			return;
		}
	}

	newval = pubonly ? AMFLAG_MUTED_PUBONLY : AMFLAG_MUTED;

	trap_Argv(1, str, sizeof(str));

	if (!Q_stricmp(str, "all") || !Q_stricmp(str, ALLBOTS)) {
		qboolean mute = qfalse;
		qboolean bots = (Q_stricmp(str, ALLBOTS) == 0);
		int i;
		const char* extra;

		cl = level.clients;

		//are we unmuting all, or muting all? can we find a client that is not muted atm?
		for (i = 0; i < MAX_CLIENTS; ++i, ++cl) {
			if (cl->pers.connected != CON_CONNECTED)
				continue;
			if (bots && !G_IsBot(i))
				continue;

			if (!(cl->sess.amflags & newval)) {
				mute = qtrue;
				break;
			}
		}

		cl = level.clients;
		for (i = 0; i < MAX_CLIENTS; ++i, ++cl) {
			if (cl->pers.connected != CON_CONNECTED)
				continue;
			if (bots && !G_IsBot(i))
				continue;

			cl->sess.amflags &= ~(AMFLAG_MUTED_PUBONLY | AMFLAG_MUTED);	//so we dont mix up these two

			if (mute) {
				cl->sess.amflags |= newval;
			}
		}

		extra = bots ? "bots " : "";

		if (mute) {
			if (pubonly)
				trap_SendServerCommand(-1,va("print \"All %swere muted from talking except team chats.\n\"", extra));
			else
				trap_SendServerCommand(-1,va("print \"All %swere muted.\n\"", extra));
		}
		else {
			trap_SendServerCommand(-1,va("print \"All %swere unmuted.\n\"", extra));
		}

		return;
	}


	num = G_FindPlayerFromString(str);

	if (num == -1) {
		G_Printf("Couldn't find client '%s'\n", str);
		return;
	}


	if (level.clients[num].sess.amflags & newval) {
		level.clients[num].sess.amflags &= ~(AMFLAG_MUTED_PUBONLY | AMFLAG_MUTED);
		trap_SendServerCommand(-1, va("print \"%s ^7was unmuted.\n\"", level.clients[num].pers.netname));
	}
	else {
		//MUTE
		//we only want 1 mute "way" applied at a time, so clear both flags here so he doesnt end up getting both standard muted and pub muted (meaningless).
		level.clients[num].sess.amflags &= ~(AMFLAG_MUTED_PUBONLY | AMFLAG_MUTED);
		level.clients[num].sess.amflags |= newval;

		if (pubonly)
			trap_SendServerCommand(-1, va("print \"%s ^7was muted, but can still talk in team chat.\n\"", level.clients[num].pers.netname));
		else
			trap_SendServerCommand(-1, va("print \"%s ^7was muted.\n\"", level.clients[num].pers.netname));
	}
}

const char* G_MsToStringVVV(const int ms) {
	int	   			fsecs = ms / 1000;		//total seconds
	int				wholemins = fsecs / 60;	//whole minutes
	float			fremainsecs;

	if (wholemins < 1)
		return va("%d secs", fsecs);
	else if (wholemins >= 60) {
		const int hrs = wholemins / 60;

		wholemins -= hrs * 60;

		if (wholemins == 0)
			return va("%dh", hrs);

		return va("%dh%dm", hrs, wholemins);
	}

	fremainsecs = (ms - wholemins * 60000) * 0.001f;

	return va("%dm%ds", wholemins, (int)fremainsecs);
}




qboolean G_SkipClient(gclient_t* client, int filter) {
	if (filter != -1) {
		const int team = client->sess.sessionTeam;

		if (filter == TEAM_PLAYING) {
			if (team == TEAM_SPECTATOR)
				return qtrue;
		}
		else if (team != filter)
			return qtrue;
	}

	return qfalse;
}

#define MAX_NAME_SHOWCHAR	24		//Show this amount of chars for each player name on the amstatus.
static void Svcmd_Status_f(void) {
	char		userinfo[MAX_INFO_STRING];
	gclient_t* client = level.clients;
	int 		filter = -1;
	int 		i, c = 0;
	mvclientSession_t* mvSess = mv_clientSessions;

	if (trap_Argc() > 1) {
		const char* arg = G_Argv(1);

		if ((filter = G_TeamForString(arg, qtrue)) == -1) {
			G_Printf("Unknown team '%s'\n", arg);
			return;
		}
	}


	G_Printf("^5%s   %24s    %18s %9s  %s\n", "##", "name", "IP", "rate", "time");

	for (i = 0; i < MAX_CLIENTS; ++i, ++client, ++mvSess) {
		const char* rate;
		char extrainfo[128] = { 0 };
		char cleanIP[32];

		if (!client || client->pers.connected == CON_DISCONNECTED)
			continue;


		if (G_SkipClient(client, filter))
			continue;

		trap_GetUserinfo(i, userinfo, sizeof(userinfo));
		rate = Info_ValueForKey(userinfo, "rate");

		if (client->sess.amflags & AMFLAG_LOCKEDNAME) {
			Q_strcat(extrainfo, sizeof(extrainfo), "locked name   ");
		}
		if (client->sess.amflags & AMFLAG_LOCKEDTEAM) {
			Q_strcat(extrainfo, sizeof(extrainfo), "locked team   ");
		}


		if (client->sess.amflags & AMFLAG_MUTED) {
			Q_strcat(extrainfo, sizeof(extrainfo), "muted");
		}
		else if (client->sess.amflags & AMFLAG_MUTED_PUBONLY) {
			Q_strcat(extrainfo, sizeof(extrainfo), "muted [pub]");
		}


		if (G_IsBot(i)) {
			Q_strncpyz(cleanIP, "BOT", sizeof(cleanIP));
		}
		else {
			char* pch;

			Com_sprintf(cleanIP,sizeof(cleanIP),"%d.%d.%d.%d", mvSess->clientIP[0], mvSess->clientIP[1], mvSess->clientIP[2], mvSess->clientIP[3]);
			//Q_strncpyz(cleanIP, client->sess.ip, sizeof(cleanIP));
			pch = strchr(cleanIP, ':');

			//truncate everything after and including ':'
			if (pch)
				*pch = 0;
		}

		G_Printf("%s ^7   %18s %9s  %8s   %s\n", G_ClientNameWithPrefix(&g_entities[i], 24), cleanIP, rate,
			G_MsToStringVVV(level.time - client->pers.connectTime),
			extrainfo);

		++c;
	}

	if (!c)
		G_Printf("<no clients to list>\n");
}

static void Svcmd_KickClientNum_f(void) {
	char		str[64] = { 0 };
	int num;

	trap_Argv(1, str, sizeof(str));

	if (trap_Argc() < 2 || !str[0] || !(str[0] >= '0' && str[0] <= '9')) {
		G_Printf("Usage: amkick <client number> - kick a client from the server\n");
		return;
	}

	num = atoi(str);

	if (num < 0 || num >= MAX_CLIENTS || level.clients[num].pers.connected == CON_DISCONNECTED) {
		G_Printf("The client number is either out of range or that client is not on the server.\n");
		return;
	}

	trap_DropClient(num, "^7was kicked.");
}

int G_GetNumPlaying(void) {
	int i, c = 0;
	gclient_t* cl = level.clients;

	for (i = 0; i < MAX_CLIENTS; ++i, ++cl) {
		if (cl->pers.connected == CON_DISCONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR)
			continue;

		++c;
	}

	return c;
}




static void Svcmd_CenterPrint_f(void) {

	if (trap_Argc() < 2) {
		G_Printf("Usage: amcp <text>");
		return;
	}

	G_CenterPrint(-1,3, ConcatArgs(1),qfalse,qfalse,qfalse,NULL);
}

static void Svcmd_SwapTeams_f(void) {
	int i;
	gclient_t* cl;

	if (g_gametype.integer < GT_TEAM) {
		G_Printf("This command can only be used in team games.");
		return;
	}

	cl = level.clients;

	for (i = 0; i < MAX_CLIENTS; ++i, ++cl) {
		char* t = NULL;

		if (!cl || cl->pers.connected == CON_DISCONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR || cl->sess.raceMode || cl->sess.mode != MODE_NORMAL)
			continue;

		if (cl->sess.sessionTeam == TEAM_RED)
			t = "b";
		else if (cl->sess.sessionTeam == TEAM_BLUE)
			t = "r";

		if (t) {
			if (g_entities[i].r.svFlags & SVF_BOT)
				SetTeam(&g_entities[i], t); //SetTeam_Bot(&g_entities[i], t);
			else
				SetTeam(&g_entities[i], t);
		}

	}

	trap_SendServerCommand(-1, "print \"Teams were swapped.\n\"");
}

static void Svcmd_Poll_f(void) {
	char buf[256];
	int i;

	if (trap_Argc() < 2) {
		G_Printf("Usage: poll <question>\n");
		return;
	}

	Q_strncpyz(buf, ConcatArgs(1), sizeof(buf));

	level.voteString[0] = '\0';
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "^3%s^7", buf);


	trap_SendServerCommand(-1,va("print \"A vote was started: %s\n\"", level.voteDisplayString));

	level.voteTime = level.time;
	level.voteYes = 0;
	level.voteNo = 0;

	for (i = 0; i < MAX_CLIENTS; ++i) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	trap_SetConfigstring(CS_VOTE_STRING, level.voteDisplayString);
	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}


#ifdef ANALYZE_BS
static void Svcmd_ListBS_f(void) {
	int 			i;
	int 			cl = -1;
	gclient_t* c = NULL;
	bsRecord_t* bsr = NULL;
	bsFrameSample_t* frame = NULL;

	if (trap_Argc() < 2)
		cl = level.lastBsClient;
	else
		cl = G_FindPlayerFromString(G_Argv(1));

	if (cl < 0 || cl >= MAX_CLIENTS || level.clients[cl].pers.connected != CON_CONNECTED) {
		G_Printf("No such player (%d).\n", cl);
		return;
	}

	c = level.clients + cl;

	if (!c->pers.numbs) {
		G_Printf("This client has no saved d/bs record.\n");
		return;
	}

	G_Printf("Latest d/bs by %s ^7(%d) (total: %d):\n", SHOWNAME(cl), cl, c->pers.numbs);

	bsr = &c->pers.savedbs;

	for (i = 0; i < NUM_BS_FRAME_SAMPLES; ++i) {
		frame = &bsr->frame[i];

		G_Printf("^%c%s\n", (frame->buttons & BUTTON_DBS) ? '3' : '7',
			BsRecordText(i, frame));
	}
}

#endif	//ANALYZE_BS

#ifdef DEBUGFPS
void FPS_ResetStats(void) {
	level.fpsSamples = 0;
	level.fpsFrameTime = 0;
	level.avgfps = 0;
}

static void Svcmd_FPS_f(void) {

	if (!g_debugFps.integer) {
		G_Printf("g_debugFps is 0. FPS stats will probably be wrong or not updated anymore.\n");
	}

	if (!Q_stricmp(G_Argv(1), "reset")) {
		G_Printf("FPS stats were reset.\n");
		FPS_ResetStats();
		return;
	}


	G_Printf("Current server fps: ^5%.3f ^7- overall avg = ^5%.4f\n", level.avgfps,
		level.fpsSamples ? 1000.f / ((float)level.fpsFrameTime / (float)level.fpsSamples) : -1);
}
#endif

void Info_Print(const char* s);

static void Svcmd_DumpUser_f(void) {
	int cl;
	char info[1024] = { 0 };

	if (trap_Argc() < 2) {
		G_Printf("Usage: %s <client> : print a client's userinfo\n", G_Argv(0));
		return;
	}

	cl = G_FindPlayerFromString(ConcatArgs(1));
	if (cl == -1) {
		G_Printf("Couldn't find that player.\n");
		return;
	}

	G_Printf("Userinfo for %s ^7(%d):\n", SHOWNAME(cl), cl);
	trap_GetUserinfo(cl, info, sizeof(info));

	G_Printf("%s\n", info);
}

int clientafkcmp(const void* a, const void* b) {
	const gentity_t** c1, ** c2;

	c1 = (const gentity_t**)a;
	c2 = (const gentity_t**)b;
	return (*c2)->client->sess.lastHereTime - (*c1)->client->sess.lastHereTime;
}

#define AFK_SECONDS		20
static void Svcmd_AfkList_f(void) {
	int i, count = 0;
	gentity_t* c;
	int secs = g_afkCmdMinSecs.integer; //AFK_SECONDS;
	const int argc = trap_Argc();
	int filter = TEAM_PLAYING;
	gentity_t* sortedClients[MAX_CLIENTS];


	G_Printf("Non-spectating clients who are inactive for more than ^5%d^7 seconds:\n", secs);

	for (i = 0, c = g_entities; i < MAX_CLIENTS; ++i, ++c) {
		int diff;

		if (!c || !c->inuse || !c->client || c->client->pers.connected != CON_CONNECTED)
			continue;

		if (c->r.svFlags & SVF_BOT)
			continue;

		if (G_SkipClient(c->client, filter))
			continue;

		diff = level.time - c->client->sess.lastHereTime;

		diff /= 1000;	//diff is now seconds since last action

		if (diff > secs)
			sortedClients[count++] = c;
	}

	if (!count)
		G_Printf("<no clients to list>\n");
	else {
		gentity_t* ent;

		//clients who have been afk longest will be shown first.
		qsort(sortedClients, count, sizeof(gentity_t**), clientafkcmp);

		for (i = 0; i < count; ++i) {
			ent = sortedClients[i];
			G_Printf("%s ^7: %s\n", G_ClientNameWithPrefix(ent, 20), G_MsToStringVVV(level.time - ent->client->sess.lastHereTime));
		}
	}
}

static void Svcmd_ListIP_f(void) {
	// G_Printf("use /rcon g_banIPs\n");
	trap_SendConsoleCommand(EXEC_NOW, "g_banIPs\n");
}

static void Svcmd_Say_f(void) {
	if (g_dedicated.integer) {
		char txt[256] = { 0 };

		Q_strncpyz(txt, ConcatArgs(1), sizeof(txt));

		if (!txt || !txt[0])
			G_Printf("Usage: say <message>\n");
		else
			trap_SendServerCommand(-1,va("print \"server: %s\n\"", txt));
	}
}







/*
===================
Svcmd_NumBehavior_f

writes a debug file about number behavior things to compare qvm and libs
===================
*/
void	Svcmd_NumBehavior_f( void ) {
	gclient_t		*cl;
	char			str[MAX_TOKEN_CHARS];
	fileHandle_t	f;
	int				i;
	signed char		sb;
	byte			b;
	int				intn;
	unsigned int	uintn;
	float			fValue;

	if (trap_Argc() < 2) {
		Com_Printf("specify a filename.");
		return;
	}

	trap_Argv(1, str, sizeof(str));

	trap_FS_FOpenFile(str, &f, FS_WRITE);

	if (!f) {
		Com_Printf("unable to open file for writing: %s.",str);
		return;
	}

#ifdef Q3_VM
	Com_sprintf(str, sizeof(str), "\nSvcmd_NumBehavior_f (VM)\n");
	trap_FS_Write(str, strlen(str), f);
#else
	Com_sprintf(str, sizeof(str), "\nSvcmd_NumBehavior_f\n");
	trap_FS_Write(str, strlen(str), f);
#endif

	Com_sprintf(str, sizeof(str), "\nangle2short\n");
	trap_FS_Write(str, strlen(str), f);

	for (i = -100000; i < 100000; i++) {
		fValue = SHORT2ANGLE(i);
		Com_sprintf(str, sizeof(str), "%d angle2short %d, float angle %f, (from float) %d\n",i,i & 65535, fValue, ANGLE2SHORT(fValue));
		trap_FS_Write(str,strlen(str),f);
	}

	Com_sprintf(str, sizeof(str), "\nsbyte2byte\n");
	trap_FS_Write(str, strlen(str), f);


	for (i = -128; i <= 127; i++) {
		sb = i;
		intn = (int)((byte)sb << 24);
		uintn = (unsigned int)intn;
		Com_sprintf(str, sizeof(str), "%d (%d) sbyte 2 byte %d, bytecast and << 24 %d to int, bytecast and << 24 %u to uint, bytecast and << 24 %u to int and then to uint, same and >> 24 again %u, same and back to sbyte %d, int >> 24 %d, and cast to sbyte %d\n", i,(int)sb,(int)(byte)sb, intn,(unsigned int)( (byte)sb << 24), uintn, (uintn >> 24), (int)(signed char)(uintn >> 24), intn>>24, (int)(signed char)(intn>>24));
		trap_FS_Write(str, strlen(str), f);
	}

	Com_sprintf(str, sizeof(str), "\nsbyte2byte (using unsigned char)\n");
	trap_FS_Write(str, strlen(str), f);


	for (i = -128; i <= 127; i++) {
		sb = i;
		intn = (int)((unsigned char)sb << 24);
		uintn = (unsigned int)intn;
		Com_sprintf(str, sizeof(str), "%d (%d) sbyte 2 byte %d, bytecast and << 24 %d to int, bytecast and << 24 %u to uint, bytecast and << 24 %u to int and then to uint, same and >> 24 again %u, same and back to sbyte %d, int >> 24 %d, and cast to sbyte %d\n", i,(int)sb,(int)(unsigned char)sb, intn,(unsigned int)( (unsigned char)sb << 24), uintn, (uintn >> 24), (int)(signed char)(uintn >> 24), intn>>24, (int)(signed char)(intn>>24));
		trap_FS_Write(str, strlen(str), f);
	}

	Com_sprintf(str, sizeof(str), "\nsbyte2byte (realvar)\n");
	trap_FS_Write(str, strlen(str), f);


	for (i = -128; i <= 127; i++) {
		sb = i;
		b = (byte)sb;
		intn = (int)(b << 24);
		uintn = (unsigned int)intn;
		Com_sprintf(str, sizeof(str), "%d (%d) sbyte 2 byte %d, bytecast and << 24 %d to int, bytecast and << 24 %u to uint, bytecast and << 24 %u to int and then to uint, same and >> 24 again %u, same and back to sbyte %d, int >> 24 %d, and cast to sbyte %d\n", i,(int)sb,(int)b, intn,(unsigned int)( b << 24), uintn, (uintn >> 24), (int)(signed char)(uintn >> 24), intn>>24, (int)(signed char)(intn>>24));
		trap_FS_Write(str, strlen(str), f);
	}

	Com_Printf("done.");

	trap_FS_FCloseFile(f);
}

void Svcmd_ResetScores_f(void) {
	int i;
	//gclient_t	*cl;
	gentity_t* ent;

	//Respawn each player for forcepower updates?
	//bg_legalizeforcepowers

	for (i = 0; i < level.numConnectedClients; i++) {
		//cl=&level.clients[level.sortedClients[i]];
		ent = &g_entities[level.sortedClients[i]];

		if (ent->inuse && ent->client) {
			//ent->client->ps.fd.forceDoInit = 1;

			//if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && !ent->client->sess.raceMode) {
				//G_Kill( ent ); //respawn them
			//}

			ent->client->ps.persistant[PERS_SCORE] = 0;
			ent->client->ps.persistant[PERS_HITS] = 0;
			ent->client->ps.persistant[PERS_KILLED] = 0;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT] = 0;
			ent->client->ps.persistant[PERS_EXCELLENT_COUNT] = 0;
			ent->client->ps.persistant[PERS_DEFEND_COUNT] = 0;
			ent->client->ps.persistant[PERS_ASSIST_COUNT] = 0;
			ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT] = 0;
			ent->client->ps.persistant[PERS_CAPTURES] = 0;

			//ent->client->pers.stats.damageGiven = 0;
			//ent->client->pers.stats.damageTaken = 0;
			//ent->client->pers.stats.teamKills = 0;
			//ent->client->pers.stats.kills = 0;
			//ent->client->pers.stats.teamHealGiven = 0;
			//ent->client->pers.stats.teamEnergizeGiven = 0;
			//ent->client->pers.stats.enemyDrainDamage = 0;
			//ent->client->pers.stats.teamDrainDamage = 0;
			ent->client->accuracy_shots = 0;
			ent->client->accuracy_hits = 0;

			ent->client->ps.fd.suicides = 0;
			//Cmd_ForceChange_f(ent);
			//WP_InitForcePowers( ent );
		}
	}

	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	CalculateRanks();
	trap_SendServerCommand(-1, "print \"Scores have been reset.\n\"");
}


char	*ConcatArgs( int start );

void G_SvCmd_ExecuteClipDemoCallback();



static void Svcmd_Servers_f(void) {
	int i;

	if (trap_Argc() >= 2)
	{
		trap_Cvar_Set("sv_master1", "masterjk2.ravensoft.com");
		trap_Cvar_Set("sv_master2", "master.jk2mv.org");
		trap_Cvar_Set("sv_master3", "master.jkhub.org");
		trap_Cvar_Set("sv_master4", "master.ouned.de");
		G_Printf("Masters restored.\n");
		return;
	}

	for (i = 1; i < 10; ++i) {
		trap_Cvar_Set(va("sv_master%d", i), "bad");
	}
	G_Printf("Masterservers were nullset.\n");
}

static void Svcmd_Help_f (void);

#define SVCMD_ALIASONLYDEDICATED		(1<<0) // dont override normal cmds when running in a non-dedi client

typedef struct {
	const char *cmdName;		//the main name of the command
	const char *cmdAlias;		//if the cmd is prepended with "am", we will skip that, so all have am-cmd as alias as  well in addition to this
	const char *cmdArgs;		//arguments mask
	const char *cmdDesc;		//description of the command

	void		(*function)(void);
	int			flags;
} gameAdminCommand_t;


static void Svcmd_Cvars_f (void);

static const gameAdminCommand_t G_AdminGameCommands[] = {
    {"amkick", "kick", "<client number>", "Kick a client from the server", Svcmd_KickClientNum_f }, // "kick" .. should we just NULL that? server overrides it anyway
    {"amstatus", "status", "", "Print a list of clients on the server, their IP and time. (shortcut: /rcon s)", Svcmd_Status_f }, // "status" .. should we just NULL that? server overrides it anyway

    {"mute", NULL, "<client number> [pub]",	"Mute or unmute a client from chatting on the server. If 'pub' is given as additional parameter, the client may still use team chats", Svcmd_Mute_f },

    {"forceteam", "team", "<client number> <team>", "Force a client (or 'all') to be on a specific team", Svcmd_ForceTeam_f, SVCMD_ALIASONLYDEDICATED },
    {"lockteam", "lock", "<team> OR <client number>", "Lock a team from players joining or leaving it OR prevent a client from changing his team", Svcmd_LockTeam_f },

	 {"lockname", "lockui", "<client number>", "Prevent a client from changing his name", Svcmd_LockUserinfo_f },
	 {"swapteams", "swapteam", "", "Teams RED and BLUE will be swapped", Svcmd_SwapTeams_f },
	{ "scrambleteams", "scrambleteams", "", "Scrambles teams RED and BLUE", Svcmd_ScrambleTeams_f },

    {"cp", NULL, "<text>", "Send a message to all clients that will be displayed in center of the screen", Svcmd_CenterPrint_f },
    {"poll", "vote", "<question>", "Start a poll", Svcmd_Poll_f, SVCMD_ALIASONLYDEDICATED },
    {"afklist", "afk", "", "See a list of clients who haven't touched any buttons for a while", Svcmd_AfkList_f, SVCMD_ALIASONLYDEDICATED },

	#ifdef DEBUGFPS
	{"fps", NULL, "[reset]", "See what fps the server is currently running at to see if it can hold up to the sv_fps value.", Svcmd_FPS_f},
	#endif

	 {"dump", NULL, "<client>", NULL, Svcmd_DumpUser_f },

    {"svhelp", "help", "", "Display this list of admin commands", Svcmd_Help_f, SVCMD_ALIASONLYDEDICATED},
	#ifdef ANALYZE_BS
	{"bsr", NULL, "[client]", "examine a client's latest d/bs move", Svcmd_ListBS_f },
	#endif

    {"masterz", NULL, "", NULL, Svcmd_Servers_f },	//hack for setting sv_master cvars, which are readonly on jk2mv
    {"cvars", NULL, "", "Display a list of new command variables in the mod", Svcmd_Cvars_f },
};

static const size_t numAdminCommands = ARRAY_LEN( G_AdminGameCommands );

extern cvarTable_t		gameCvarTable[];
extern int gameCvarTableSize;

int cvarcmp( const void *a, const void *b ) {
	const cvarTable_t **c1, **c2;

	c1 = (const cvarTable_t**)a;
	c2 = (const cvarTable_t**)b;
	return strcmp((*c1)->cvarName, (*c2)->cvarName);
}


static void Svcmd_Cvars_f (void) {
	int			i, c = 0;
	cvarTable_t	*cv;
	cvarTable_t	*sorted[512];

	G_Printf("New serverside command variables and their current settings:\n");

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; ++i, ++cv ) {
		if (!cv || !cv->vmCvar || !cv->cvarName)
			continue;

		if ( !(cv->cvarFlags & CVAR_VVV) )
			continue;

		sorted[ c++ ] = cv;
	}

	if (c)
		qsort(sorted, c, sizeof(sorted[0]), cvarcmp);

	for (i = 0; i < c; ++i) {
		
		char buf[512] = {0};
		char *pch = &buf[0];
		cv = sorted[i];

		pch = mystrcat(pch, sizeof(buf), va(" ^5%s ^7%s", cv->cvarName, cv->vmCvar->string));
		pch = mystrcat(pch, sizeof(buf),
			!strcmp(cv->vmCvar->string, cv->defaultString) ? "  (default)\n" : "\n");

		if (cv->desc && cv->desc[0])
			pch = mystrcat(buf, sizeof(buf), va("    %s\n\n", cv->desc));

		G_Printf( "%s", buf);
	}
}

int cmdcmphelp( const void *a, const void *b ) {
	const gameAdminCommand_t **c1, **c2;

	c1 = (const gameAdminCommand_t**)a;
	c2 = (const gameAdminCommand_t**)b;
	return strcmp((*c1)->cmdName, (*c2)->cmdName);
}

static void Svcmd_Help_f (void) {
	const gameAdminCommand_t *cmd;
	gameAdminCommand_t *sorted[64] = {0};
	int i, c = 0;

	G_Printf("^7--- ^5ADMIN COMMANDS ^7---\n\n");

	for ( i = 0, cmd = G_AdminGameCommands ; i < numAdminCommands ; ++cmd, ++i) {

		if ( !cmd || !cmd->cmdName || !cmd->cmdDesc  )
			continue;

		sorted[ c++ ] = (gameAdminCommand_t *)cmd;
	}

	if (c)
		qsort(sorted, c, sizeof(sorted[0]), cmdcmphelp);

	for (i = 0; i < c; ++i) {
		cmd = sorted[i];

		G_Printf(" ^5%s ^7%s\n"
				 "    %s\n\n", cmd->cmdName, cmd->cmdArgs, cmd->cmdDesc );
	}
}

/*
=================
ConsoleCommand

=================
*/

qboolean G_IsBot(int client) {
	if (client >= 0 && client < MAX_CLIENTS && g_entities[client].inuse && g_entities[client].client && g_entities[client].r.svFlags & SVF_BOT)
		return qtrue;

	return qfalse;
}
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText );

const char *ShortString (const char *str);

qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];
	int i;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	// check new servercmds
	{
		const char* sp = cmd;
		const gameAdminCommand_t* cvCmd;

		//skip "am" prefix
		if (!Q_stricmpn(cmd, "am", 2) && cmd[2]) {
			sp += 2;
		}

		//special shortcut for amstatus
		if (!Q_stricmp(sp, "st") || !Q_stricmp(sp, "s")) {
			Svcmd_Status_f();
			return qtrue;
		}

		for (i = 0, cvCmd = G_AdminGameCommands; i < numAdminCommands; ++cvCmd, ++i) {

			if (!cvCmd || !cvCmd->cmdName || !cvCmd->function)
				continue;

			if (!Q_stricmp(sp, cvCmd->cmdName) 
				|| (cvCmd->cmdAlias && !Q_stricmp(sp, cvCmd->cmdAlias) && (!(cvCmd->flags & SVCMD_ALIASONLYDEDICATED) || g_dedicated.integer))
				) {
				cvCmd->function();
				return qtrue;
			}
		}
	}


	if ( Q_stricmp (cmd, "testtrace") == 0 ) {
		SvCmd_TestTrace_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "entityinfo") == 0 ) {
		Svcmd_EntityInfo_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "numbehavior") == 0) {
		Svcmd_NumBehavior_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "botlist") == 0) {
		Svcmd_BotList_f();
		return qtrue;
	}

/*	if (Q_stricmp (cmd, "abort_podium") == 0) {
		Svcmd_AbortPodium_f();
		return qtrue;
	}
*/
	if (Q_stricmp (cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	// fau version
	if (Q_stricmp(cmd, "shuffle") == 0) {
		Svcmd_Shuffle_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "pause") == 0) {
		Svcmd_Pausegame_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "unpause") == 0) {
		Svcmd_UnpauseGame_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "togglepause") == 0) {
		Svcmd_PauseToggle_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
		return qtrue;
	}

	if (Q_stricmp (cmd, "clipdemodone") == 0) {
		G_SvCmd_ExecuteClipDemoCallback();
		return qtrue;
	}

#if _DEBUG // Only in debug builds
	if ( !Q_stricmp(cmd, "jk2gameplay") )
	{
		char arg1[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg1, sizeof(arg1) );

		switch ( atoi(arg1) )
		{
			case VERSION_1_02:
				MV_SetGamePlay(VERSION_1_02);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.02\n\"" );
				break;
			case VERSION_1_03:
				MV_SetGamePlay(VERSION_1_03);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.03\n\"" );
				break;
			default:
			case VERSION_1_04:
				MV_SetGamePlay(VERSION_1_04);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.04\n\"" );
				break;
		}
		return qtrue;
	}
#endif

	if (g_dedicated.integer) {
		if (Q_stricmp (cmd, "say") == 0) {
			trap_SendServerCommand( -1, va("print \"server: %s\n\"", ConcatArgs(1) ) );
			return qtrue;
		}
		// everything else will also be printed as a say command
		//trap_SendServerCommand( -1, va("print \"server: %s\n\"", ConcatArgs(0) ) );

		// doesnt make sense in non dedi operation:
		G_Printf("Unknown command ~%s~. Use ~help~ to see admin commands.\n", ShortString(cmd));

		//return qtrue;
	}

	return qfalse;
}

