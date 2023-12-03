// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

ID_INLINE void CG_LastWeapon(void) { //Called by CG_SetNextSnap, dunno if need to use snap or can use predicted..
	if (!cg.snap)
		return;
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if (cg.snap->ps.pm_flags & PMF_FOLLOW || cg.snap->ps.pm_type == PM_SPECTATOR)
		return;

	if (!cg.lastWeaponSelect[0])
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon;
	if (!cg.lastWeaponSelect[1])
		cg.lastWeaponSelect[1] = cg.predictedPlayerState.weapon;

	if (cg.lastWeaponSelect[0] != cg.predictedPlayerState.weapon) { //Current does not match selected
		cg.lastWeaponSelect[1] = cg.lastWeaponSelect[0]; //Set last to current
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon; //Set current to selected
	}

}


/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity( centity_t *cent ) {
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if ( cent->snapShotTime < cg.time - EVENT_VALID_MSEC ) {
		cent->previousEvent = 0;
	}

	cent->trailTime = cg.snap->serverTime;

	VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	VectorCopy (cent->currentState.angles, cent->lerpAngles);
	if ( cent->currentState.eType == ET_PLAYER ) {
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity( centity_t *cent ) {
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	if ( jk2startversion == VERSION_1_02 )
	{ // MVSDK: Version Magic!
		cent->currentState.torsoAnim = MV_MapAnimation104( cent->currentState.torsoAnim );
		cent->currentState.legsAnim = MV_MapAnimation104( cent->currentState.legsAnim );
	}

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	if (cent->currentState.number >= 0 && cent->currentState.number < MAX_CLIENTS && (cg_debugSaber.integer < -1 || cg_debugSaber.integer >= MAX_CLIENTS || cg_debugSaber.integer == cent->currentState.number) && cent->currentState.saberMove != cent->previousSaberMove) {
		CG_Printf("ent:%3i  saberMove:%3i  saberMoveName:%s \n", cent->currentState.number, cent->currentState.saberMove, saberMoveData[cent->currentState.saberMove].name);
		cent->previousSaberMove = cent->currentState.saberMove;
	}

	// check for events
	CG_CheckEvents( cent );
}


/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use 
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap ) {
	int				i;
	centity_t		*cent;
	entityState_t	*state;

	cg.snap = snap; 

	if ((cg_entities[snap->ps.clientNum].ghoul2 == NULL) && trap_G2_HaveWeGhoul2Models(cgs.clientinfo[snap->ps.clientNum].ghoul2Model))
	{
		trap_G2API_DuplicateGhoul2Instance(cgs.clientinfo[snap->ps.clientNum].ghoul2Model, &cg_entities[snap->ps.clientNum].ghoul2);
		CG_CopyG2WeaponInstance(FIRST_WEAPON, cg_entities[snap->ps.clientNum].ghoul2);
	}
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy(&cent->currentState, state, sizeof(entityState_t));
		//cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		if ( jk2startversion == VERSION_1_02 )
		{ // MVSDK: Version Magic!
			cent->currentState.torsoAnim = MV_MapAnimation104( cent->currentState.torsoAnim );
			cent->currentState.legsAnim = MV_MapAnimation104( cent->currentState.legsAnim );
		}

		CG_ResetEntity( cent );

		if (state->number >= 0 && state->number < MAX_CLIENTS && (cg_debugSaber.integer < -1 || cg_debugSaber.integer >= MAX_CLIENTS || cg_debugSaber.integer == state->number) && state->saberMove != cent->previousSaberMove) {
			CG_Printf("ent:%3i  saberMove:%3i  saberMoveName:%s \n", state->number, state->saberMove, saberMoveData[state->saberMove].name);
			cent->previousSaberMove = state->saberMove;
		}

		// check for events
		CG_CheckEvents( cent );

	}
}

void CG_NewSnapshotArrived(void);

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
static void CG_TransitionSnapshot( void ) {
	centity_t			*cent;
	snapshot_t			*oldFrame;
	int					i;

	if ( !cg.snap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// if we had a map_restart, set everthing with initial
	if ( !cg.snap ) {
	}

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qfalse;

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		CG_TransitionEntity( cent );

		// remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	cg.nextSnap = NULL;

	// check for playerstate transition events
	if ( oldFrame ) {
		playerState_t	*ops, *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;	// will be cleared by prediction code
		}
		else if (cg_cameraFPS.integer >= CAMERA_MIN_FPS) {
			cg.thisFrameTeleport = qfalse; // clear for interpolated player with new camera damping
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg_nopredict.integer || cg_synchronousClients.integer ) {
			CG_TransitionPlayerState( ps, ops );
		}

		CG_NewSnapshotArrived();
	}

}


/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void CG_SetNextSnap( snapshot_t *snap ) {
	int					num;
	entityState_t		*es;
	centity_t			*cent;

	cg.nextSnap = snap;

	CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ ) 
	{
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy(&cent->nextState, es, sizeof(entityState_t));
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		cg.nextFrameTeleport = qtrue;
	} else {
		cg.nextFrameTeleport = qfalse;
	}

	// if changing follow mode, don't interpolate
	if ( cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum ) {
		cg.nextFrameTeleport = qtrue;
	}

	// if changing server restarts, don't interpolate
	if ( ( cg.nextSnap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
		cg.nextFrameTeleport = qtrue;
	}

	// sort out solid entities
	CG_BuildSolidList();

	CG_LastWeapon();
}


/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
snapshot_1_02_t	activeSnapshot_1_02; // MVSDK: Only used to receive the new snapshot. We're copying the content over as soon as we have the new snapshot... // Global variable for the qvm compiler...
static snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean	r;
	snapshot_t	*dest;

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 ) {
		CG_Printf( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
			cg.latestSnapshotNum, cgs.processedSnapshotNum );
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else {
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;

		if ( jk2version == VERSION_1_02 )
		{ // MVSDK: Multiversion magic!
			r = trap_GetSnapshot( cgs.processedSnapshotNum, (snapshot_t*)&activeSnapshot_1_02 );
		}
		else
		{
			r = trap_GetSnapshot( cgs.processedSnapshotNum, dest );
		}

		// FIXME: why would trap_GetSnapshot return a snapshot with the same server time
		if ( cg.snap && r && dest->serverTime == cg.snap->serverTime ) {
			//continue;
		}

		// if it succeeded, return
		if ( r ) {
			if ( jk2version == VERSION_1_02 )
			{ // MVSDK: Multiversion Magic
				static const size_t section1 = (size_t)((char *)&((snapshot_t*)NULL)->ps);
				static const size_t section2 = (size_t)((char *)&((playerState_t*)NULL)->forceRestricted);
				static const size_t section3 = (size_t)((char *)&((playerState_t*)NULL)->saberIndex - (char *)&((playerState_t*)NULL)->forceRestricted);
				static const size_t section4 = (size_t)((char *)(&((snapshot_t*)NULL)->ps) + sizeof(playerState_t) - (char *)&((snapshot_t*)NULL)->ps.saberIndex);
				static const size_t section5 = (size_t)((char *)(&(((snapshot_t*)NULL)[1])) - (char *)&((snapshot_t*)NULL)->numEntities);

				/* Convert the snapshot (mainly because of the playerState) */
				memcpy( dest, &(activeSnapshot_1_02), section1 ); // Copy everything till ps
				memcpy( &(dest->ps), &(activeSnapshot_1_02.ps), section2 ); // Copy everything till ps.forceRestricted
				memset( &(dest->ps.forceRestricted), 0, section3 ); // 0 everything from ps.forceRestricted till ps.saberIndex
				memcpy( &(dest->ps.saberIndex), &(activeSnapshot_1_02.ps.saberIndex), section4 ); // Copy everything starting with ps.saberIndex
				memcpy( &dest->numEntities, &(activeSnapshot_1_02.numEntities), section5 ); // Copy everything after ps
			}
			if ( jk2startversion == VERSION_1_02 )
			{
				/* Convert the animations */
				dest->ps.legsAnim = MV_MapAnimation104(dest->ps.legsAnim);
				dest->ps.legsAnimExecute = MV_MapAnimation104(dest->ps.legsAnimExecute);
				dest->ps.torsoAnim = MV_MapAnimation104(dest->ps.torsoAnim);
				dest->ps.torsoAnimExecute = MV_MapAnimation104(dest->ps.torsoAnimExecute);

				/* Only convert forceDodgeAnim if it really is an animation (forceHandExtend being either HANDEXTEND_TAUNT or HANDEXTEND_DODGE) */
				if ( dest->ps.forceHandExtend == HANDEXTEND_TAUNT || dest->ps.forceHandExtend == HANDEXTEND_DODGE ) dest->ps.forceDodgeAnim = MV_MapAnimation104(dest->ps.forceDodgeAnim);

				/* The following two seem to be unused, but maybe custom cgames make use of them (well, fullAnimExecute seems to not even be set at least once - could probably just leave that one out) */
				dest->ps.fullAnimExecute = MV_MapAnimation104(dest->ps.fullAnimExecute);
				dest->ps.saberAttackSequence = MV_MapAnimation104(dest->ps.saberAttackSequence);

				/* Convert the saberblocks */
				if (dest->ps.saberBlocked > BLOCKED_NONE) {
					dest->ps.saberBlocked++;
				}
			}
			CG_AddLagometerSnapshotInfo( dest );
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}


//so we dont need to check if it's predicted client every time
qboolean CG_EntityIsValid(const int clientNum) {
	return (qboolean)(clientNum == cg.snap->ps.clientNum || (clientNum >= 0 && cg_entities[clientNum].currentValid));
}


// #define MAX_PREDEMO_FRAGS 100

extern const char* timescaleString;

ezDemoBuffer_t ezDemoBuffer;

qboolean ezdemoSeeking = qfalse;

#ifdef CG_EZDEMO
static void CG_EzdemoSeek(const int pdCount) {
	static int i = 1;	//iterates through events up till pdCount

	// const int curtime = cg.snap->serverTime;
	const int curtime = cg.time;
	const int pretime = x3_ezdemoPreTime.integer;	//we want to skip up to the particular event X ms before it happens
	const int protime = x3_ezdemoPostTime.integer;	// after the event happened, wait X ms before skipping forward...

	static int 		client;
	static int 		eventtime;
	static qboolean awaitingEvent = qfalse;
	static qboolean eventStarted = qfalse;		//so we can allow the user to alter timescale as he sees fit during the event.

	if (i > pdCount) {
		static qboolean printedMsg = qfalse;		//ARF

		if (!printedMsg) {
			Com_Printf("No more events in the demo...\n");
			trap_SendConsoleCommand("disconnect\n");
			printedMsg = qtrue;
		}

		return;	//no more events to seek forward to
	}


	if (!awaitingEvent) {
		//i as a static var .. nice!
		for (; i <= pdCount; ++i) {
			char buf[32];
			char cl[8], tm[32];
			char* pch = NULL;

			if (ezDemoBuffer.eventCount) {
				eventtime = ezDemoBuffer.events[i - 1].serverTime;
				client = ezDemoBuffer.events[i - 1].clientNum;
			}
			else {

				trap_Cvar_VariableStringBuffer(va("pd%i", i), buf, sizeof(buf));

				pch = strchr(buf, '\\');

				if (!pch) {
					continue;	//was "break;"
				}

				// parse target servertime
				Q_strncpyz(tm, pch + 1, sizeof(tm));
				eventtime = atoi(tm);


				//parse target client num
				Q_strncpyz(cl, buf, strlen(buf) - strlen(pch) + 1);
				client = atoi(cl);
			}

			//check if we're psat this time, in which case skip this element.
			// if (time > eventtime + protime){
			// if ( !(curtime <= eventtime - pretime) ) {
			// if (curtime >= eventtime + protime) {
#define OK_EXTRA_MS	30
			if (curtime > eventtime + protime - OK_EXTRA_MS) {
				// CG_Printf("Skipping this element because servertime (%i) > %i. (it already happened)\n", curtime, eventtime-OK_EXTRA_MS);
				continue;
			}


			// Com_Printf("^3took event at time %d, client %d\n", eventtime, client);

			//By here, we have a valid element that has not happened yet. Break out and fastforward to it!
			eventStarted = qfalse;
			break;
		}
	}


	if (curtime < eventtime - pretime) {
		// if (eventtime - curtime >= pretime) {
			//we've not yet reached this event's time. fastforward
		const int diff = eventtime - pretime - cg.time;	//how many ms are we from this event beginning?
		const int secs = diff / 1000;
		int ts = 2;	//timescale value

		// This is a mess, but was made so that fastforwarding doesnt take too long and is not too fast so that the event is skipped!
		if (secs > 420) {
			ts = 2900;
		}
		else if (secs > 360) {
			ts = 1900;
		}
		else if (secs > 180) {
			ts = 1600;
		}
		else if (secs > 160) {
			ts = 1050;
		}
		else if (secs > 120) {
			ts = 920;
		}
		else if (secs > 100) {
			ts = 840;
		}
		else if (secs > 60) {
			ts = 760;
		}
		else if (secs > 45) {
			ts = 420;
		}
		else if (secs > 30) {
			ts = 390;
		}
		else if (secs > 15) {
			ts = 290;
		}
		else if (secs > 6) {
			ts = 175;
		}
		else if (secs > 3) {
			ts = 115;
		}
		else if (secs > 2) {
			ts = 45;
		}
		else if (secs > 1) {
			ts = 15;
		}
		else {
			ts = 8;
		}

		//just force the timescale when skipping forward (dont allow user to set lower timescale when seeking forward to event)
		trap_Cvar_Set(timescaleString, va("%i", ts));

		// Com_Printf("Seeking forward to this event i%i, happening at time %i with client %i. (in %.2f secs)\n", i, eventtime, client, (eventtime-pretime - time) / 1000.f);

		awaitingEvent = qtrue;
		trap_Cvar_Set("s_forcevol", "0.01");
	}
	else if (curtime >= eventtime - pretime && curtime <= eventtime + protime) {
		//this event is currently happening! dont fastforward, lets see it in normal motion.
		// Com_Printf("event is currently happening.. timescale forced to 1. setting viewpos to client %i '%s'\n", client, cgs.clientinfo[client].name);

		if (CG_EntityIsValid(client)) {

			if (!eventStarted) {
				if (cg.refclient != client)
					trap_SendConsoleCommand(va("follow %i\n", client));

				trap_Cvar_Set(timescaleString, va("%i", 1));
				trap_Cvar_Set("s_forcevol", "0");
				eventStarted = qtrue;	//allow the user to modify timescale
			}

		}
		else if (curtime > eventtime) {
			// Com_Printf("^5can no longer see who did this frag, fastforwarding!\n");
			//if an event happened and we're not past posttime, and we can no longer see who did it, just skip to next event.
			awaitingEvent = qfalse;
			eventStarted = qfalse;
			//the loop will now continue to find next event
			trap_Cvar_Set(timescaleString, va("%i", 11));
		}
		else {
			//cant see the guy who gets the frag yet, so fastforward A LITTLE
			trap_Cvar_Set(timescaleString, "1.8");
		}
	}
	else if (curtime > eventtime + protime) {
		//the event happened already
		awaitingEvent = qfalse;
		eventStarted = qfalse;
		//the loop will now continue to find next event
	}
}
#endif

qboolean skippingPause = qfalse;
static void CG_ControlDemoSeek(void) {
	static qboolean 	fastforwarding = qfalse;
	int 				timescaleX;// = ClampInt(x3_demoSeekTimescale.integer, 2, 200);
	qboolean 			dofastforward = qfalse;
	static int			lastValidCap = 0;

#ifdef CG_EZDEMO
	static int doFragSeek = -1;

	if (doFragSeek == -1) {
		if (ezDemoBuffer.eventCount) {
			doFragSeek = ezDemoBuffer.eventCount;
		}
		else {
			doFragSeek = CG_Cvar_Get_int("pdCount");	//for the love of all, only do cvar_get 1 time...
		}
	}

	if (doFragSeek > 0) {
		// Com_Printf("Doing DemoFragSeek...\n");
		ezdemoSeeking = qtrue;
		CG_EzdemoSeek(doFragSeek);
		return;
	}
#endif

	if (!x3_demoSkipPauses.integer && !cg.demoseek) {
		if (fastforwarding) { // Fix: When maprestart seekmode is used exclusively, it wont stop fast forwarding otherwise
			trap_Cvar_Set(timescaleString, "1");
			fastforwarding = qfalse;

			//if (cg_x3clmodule)
			//	trap_Cvar_Set("s_forcevol", "0");
		}
		return;	//no seeking
	}

	timescaleX = MIN(MAX(x3_demoSeekTimescale.integer, 2), 200);

	if (cg.demoseek & DEMOSEEK_SPECIFIC_CLIENT_ONLY) {
		if (cg.refclient != cg.demofollowclient)
		{	//not the clientnum we wish, speed up.
			dofastforward = qtrue;
		}
	}

	if ((cg.demoseek & DEMOSEEK_RETMODE) && cg.refclient < MAX_CLIENTS) {
		//Fastforward until:
		// 1. our flag is taken
		// 2. the flag carrier is currentValid and within certain distance?
		int capper;
		int team = cg.refteam;
		vec3_t origin;

		//if (cg.demofollowvis) {
			VectorCopy(cg_entities[cg.demofollowclient].lerpOrigin, origin);
		//}
		//else 
		{
			VectorCopy(cg.predictedPlayerState.origin, origin);
		}


		capper = team == TEAM_RED ? (cgs.redFlagCarrier ? cgs.redFlagCarrier-cgs.clientinfo : -1) : (cgs.redFlagCarrier ? cgs.blueFlagCarrier - cgs.clientinfo : -1);

		if (!CG_OtherTeamHasFlag() ||
			(capper >= 0 && capper < MAX_CLIENTS && cgs.clientinfo[capper].infoValid &&
				(!CG_EntityIsValid(capper) || Distance(cg_entities[capper].lerpOrigin, origin) > 1000))
			) {
			//fastforward if other team doesnt have flag or their capper isnt visible for us
			if(fastforwarding || cg.lastRefClientKill < cg.time && (cg.time- cg.lastRefClientKill) > 2000) dofastforward = qtrue; // 2 seconds cooldown where we don't fast-forward after killing someone.
		}
	}
	else if (cg.demoseek & DEMOSEEK_CAPPING_ONLY) {
		//Speeed up the demo until we have the flag
		//if (!CG_IsACapper(cg.refclient))
		if (cg.refclient != (cgs.redFlagCarrier - cgs.clientinfo) && cg.refclient != (cgs.blueFlagCarrier - cgs.clientinfo)) {

			if (lastValidCap < cg.time && (cg.time - lastValidCap) > 2000) dofastforward = qtrue;
		}
		else {
			lastValidCap = cg.time;
		}
	}

	if ((x3_demoSkipPauses.integer && cg.pausedGame) || (cg.demoseek & DEMOSEEK_MAPRESTART && !cg.mapRestart)) {
		dofastforward = qtrue;
		timescaleX = 200;	//speed up very fast until its unpaused

		if (x3_demoSkipPauses.integer && cg.pausedGame)
			skippingPause = qtrue;
	}
	else {
		skippingPause = qfalse;
	}

	if (dofastforward && !fastforwarding) {
		trap_Cvar_Set(timescaleString, va("%i", timescaleX));
		fastforwarding = qtrue;

		//if (cg_x3clmodule)
		//	trap_Cvar_Set("s_forcevol", "0.015");

	}
	else if (!dofastforward && fastforwarding) {
		trap_Cvar_Set(timescaleString, "1");
		fastforwarding = qfalse;

		//if (cg_x3clmodule)
		//	trap_Cvar_Set("s_forcevol", "0");
	}
}

void CG_NewSnapshotArrived(void) {

	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		cg.speccing = qtrue;
	else
		cg.speccing = qfalse;

	ezdemoSeeking = qfalse;
	if (cg.demoPlayback)
		CG_ControlDemoSeek();
}




/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots( void ) {
	snapshot_t		*snap;
	int				n;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			// this should never happen
			CG_Error( "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
		}
		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
			CG_SetInitialSnapshot( snap );
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				break;
			}

			CG_SetNextSnap( snap );


			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
	}
	if ( cg.time < cg.snap->serverTime ) {
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}

	// set cg.frameInterpolation
	if (cg.nextSnap) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if (delta == 0) {
			cg.frameInterpolation = 0;
		}
		else {
			cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
		}
	}
	else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because
									// no entities should be marked as interpolating
		cg.predictedTimeFrac = 0.0f;
	}

}

