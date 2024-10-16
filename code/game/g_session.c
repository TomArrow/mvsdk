// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

// Useful when trying to store remaining time across map changes
#define LevelTimeDiff( timeVal )		( timeVal > level.time ? timeVal - level.time : 0 )
#define RestoreLevelTimeDiff( timeVal )	( timeVal = timeVal ? level.time + timeVal : 0 )

extern void DF_CarryClientOverToNewRaceStyle(gentity_t* ent, raceStyle_t* newRs);

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %s", 
		client->sess.sessionTeam,
		client->sess.spectatorOrder,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.setForce,
		client->sess.saberLevel,
		client->sess.selectedFP,
		client->sess.raceMode,
		client->sess.raceStyle.movementStyle,
		client->sess.raceStyle.runFlags,
		client->sess.raceStyle.jumpLevel,
		client->sess.mapStyleBaseline.runFlags,
		client->sess.mapStyleBaseline.jumpLevel,
		client->sess.raceStateInvalidated,
		client->sess.login.loggedIn,
		client->sess.login.id,
		client->sess.login.flags,
		client->sess.login.name
		);

	var = va( "session%i", (int)(client - level.clients) );

	trap_Cvar_Set( var, s );

	s = va("%i %i %i %i %i",
		mv_clientSessions[client-g_clients].clientIP[0],
		mv_clientSessions[client-g_clients].clientIP[1],
		mv_clientSessions[client-g_clients].clientIP[2],
		mv_clientSessions[client-g_clients].clientIP[3],
		mv_clientSessions[client-g_clients].localClient
		);
	var = va( "sessionmv%i", (int)(client-level.clients) );
	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;
	int setForce;
	int tempRaceMode;
	int movementStyle;
	int runFlags;
	int jumpLevel;
	int baseRunFlags;
	int baseJumpLevel;
	int raceStateInvalidated;
	int loggedIn;

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %s",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorOrder,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,                   // bk010221 - format
		&setForce,
		&client->sess.saberLevel,
		&client->sess.selectedFP,
		&tempRaceMode,
		&movementStyle,
		&runFlags,
		&jumpLevel,
		&baseRunFlags,
		&baseJumpLevel,
		&raceStateInvalidated,
		&loggedIn,
		&client->sess.login.id,
		&client->sess.login.flags,
		client->sess.login.name
		);

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
	client->sess.setForce = (qboolean)setForce;
	client->sess.raceMode = (qboolean)tempRaceMode;
	client->sess.raceStyle.movementStyle = (byte)movementStyle;
	client->sess.raceStyle.runFlags = (short)runFlags;
	client->sess.raceStyle.jumpLevel = (signed char)jumpLevel;
	client->sess.mapStyleBaseline.runFlags = (short)baseRunFlags;
	client->sess.mapStyleBaseline.jumpLevel = (signed char)baseJumpLevel;
	client->sess.raceStateInvalidated = qtrue;//likely map change. old stuff wont be valid anymore. // (qboolean)raceStateInvalidated;
	client->sess.login.loggedIn = loggedIn;

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;

	DF_CarryClientOverToNewRaceStyle(g_entities+(client-g_clients),&level.mapDefaultRaceStyle);
}

/*
==================
MV_ReadSessionData

Called on a reconnect
==================
*/
void MV_ReadSessionData( int clientNum )
{
	char	s[MAX_STRING_CHARS];
	const char	*var;
	int localClient;

	var = va( "sessionmv%i", clientNum );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );
	sscanf( s, "%i %i %i %i %i",
		&mv_clientSessions[clientNum].clientIP[0],
		&mv_clientSessions[clientNum].clientIP[1],
		&mv_clientSessions[clientNum].clientIP[2],
		&mv_clientSessions[clientNum].clientIP[3],
		&localClient
		);

	mv_clientSessions[clientNum].localClient = (qboolean)localClient;
	//trap_Cvar_Set( var, "" ); // Causes issues, if people aren't fully ingam, but the server changes maps again.
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
extern void UpdateClientRaceVars(gclient_t* client);
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;
	
	//sess->raceStyle.movementStyle = MV_JK2;
	//sess->raceStyle.jumpLevel = 1;
	//sess->raceStyle.runFlags = defaultRunFlags;
	sess->raceMode = g_defrag.integer; // TODO what about changing g_defrag live, should we take some care? idk
	sess->mapStyleBaseline = level.mapDefaultRaceStyle;
	sess->raceStyle = sess->mapStyleBaseline;
	UpdateClientRaceVars(client);
	//client->ps.fd.forcePowerLevel[FP_LEVITATION] = client->sess.raceStyle.jumpLevel;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			if (!isBot)
			{
				sess->sessionTeam = TEAM_SPECTATOR;	
			}
			else
			{ //Bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if (value[0] == 'r' || value[0] == 'R')
				{
					sess->sessionTeam = TEAM_RED;
				}
				else if (value[0] == 'b' || value[0] == 'B')
				{
					sess->sessionTeam = TEAM_BLUE;
				}
				else
				{
					sess->sessionTeam = PickTeam( -1 );
				}
				BroadcastTeamChange( client, -1 );
			}
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorOrder = 0;

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
