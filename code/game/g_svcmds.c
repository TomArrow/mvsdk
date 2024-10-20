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
			"%i.%i.%i.%i ", b[0], b[1], b[2], b[3]);
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

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
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

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "entityinfo") == 0 ) {
		Svcmd_EntityInfo_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
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

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
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
		trap_SendServerCommand( -1, va("print \"server: %s\n\"", ConcatArgs(0) ) );
		return qtrue;
	}

	return qfalse;
}

