//
// functionality related to the playersnapshots api feature
// aka: engine tells us that snapshots are about to get sent out, allowing us to prevent it, or to do random other things like change values in entities, adjust visibility etc
//

#include "g_local.h"




/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
static qboolean CG_AdjustPositionForClientTimeMover(const vec3_t in, int moverNum, /*int fromTime, int toTime, */ vec3_t out) {
	gentity_t* gent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles;
	int fromTime, toTime;
	//int backupTrTime;
	// vec3_t	deltaAngles;

	if (moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL) {
		VectorCopy(in, out);
		return qfalse;
	}

	gent = &g_entities[moverNum];
	if (gent->s.eType != ET_MOVER) {
		VectorCopy(in, out);
		return qfalse;
	}

	fromTime = MOVERTIME_ENT(gent);
	toTime = level.time;
	if (fromTime == toTime) {
		VectorCopy(in, out);
		return qfalse;
	}
	//backupTrTime = gent->s.pos.trTime;
	//gent->s.pos.trTime = level.time - (fromTime - gent->s.pos.trTime);

	BG_EvaluateTrajectory(&gent->s.pos, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&gent->s.apos, fromTime, oldAngles);

	BG_EvaluateTrajectory(&gent->s.pos, toTime, origin);
	BG_EvaluateTrajectory(&gent->s.apos, toTime, angles);

	//gent->s.pos.trTime = backupTrTime;

	VectorSubtract(origin, oldOrigin, deltaOrigin);
	// VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd(in, deltaOrigin, out);
	
	return qtrue;

	// FIXME: origin change when on a rotating object
}

int GetPlayerVisibilityForPlayer(gclient_t* cl, qboolean canSeeTASClients, gclient_t* ocl) {
	qboolean ignore = 
		// machine learning tas client hiding (but don't hide clients that could hurt us)
		(!canSeeTASClients && (ocl->pers.tasClient & TASCLIENT_MACHINELEARNING) && (ocl->sess.raceMode || ocl->sess.mode != cl->sess.mode))
		
		// hide others when ignoring (nah don't do it)
		//|| (cl->sess.ignore & (1 << i))

		// hide others when soloing
		|| cl->sess.solo == SOLO_ALL 

		// hide others when soloing in same-style mode and their style is different
		|| cl->sess.solo == SOLO_STYLE && (ocl->sess.mode != cl->sess.mode || cl->sess.mode == MODE_DEFRAG && ocl->sess.raceStyle.movementStyle != cl->sess.raceStyle.movementStyle);
	return ignore ? -1 : 0;
}


// 1=  force visibility. 0 = no change. -1 = hide!
// mvEnt must be UNCHANGED mvSharedEntity of other before any hacking!!
int GetEntVisibilityForPlayerReal(gentity_t* ent, int clientNum, int entNum, gentity_t* other, mvsharedEntity_t* mvEnt, gclient_t* cl, gentity_t* followedEnt, gclient_t* followedClient, qboolean canSeeTASClients) {

	int ignore = mvEnt->snapshotIgnoreRealClient[clientNum];
	int enforce = mvEnt->snapshotEnforceRealClient[clientNum];
	int tmp;
	gclient_t* ocl;
	entityState_t* es = &other->s;

	if (ignore > 0) { 
		// if game already wants ignore (e.g. antiwallhack), skip all this.
		// ignore overrides enforce in engine so any possible enforce doesn't matter anymore.
		return -ignore;
	}
	
#define APPLYVIS(vis) if(vis < 0 ){ ignore++; } else if (vis > 0) { enforce++; }

	// player vents (hop sounds and such)
	if (es->eFlags & EF_PLAYER_EVENT) {
		gclient_t* eventClient = g_entities[es->otherEntityNum].client;
		if (eventClient != followedClient && eventClient != cl) {
			// EF_PLAYER_EVENT is already automatically skipped in engine for whoever we are following and for ourselves.
			// Since you know... playerstate has that info already
			tmp = GetPlayerVisibilityForPlayer(cl, canSeeTASClients, eventClient);
			APPLYVIS(tmp);
		}
	}

	// these are things where we choose purely based on ourselves:
	if (es->eType == (ET_EVENTS + EV_SCREENSHAKE) && !es->modelindex || other->hideFromActiveRacers) { // dont send global screenshakes to active players unless they are not in a run
		if (cl->sess.sessionTeam != TEAM_SPECTATOR && other->parent != ent && cl->pers.raceStartCommandTime) {
			ignore++;
		}
	}

	// these are things where we choose either based on ourselves or whoever we are following:
	if (es->eType == ET_PUSH_TRIGGER || es->eType == ET_TELEPORT_TRIGGER) {
		if (other->notCPM) {
			if (followedClient->sess.raceMode && !MovementStyleHasVQ3OnlyJumppads(followedClient->sess.raceStyle.movementStyle)) {
				ignore++;
			}
		}
		else if (other->notVQ3) {
			if (followedClient->sess.raceMode && !MovementStyleHasCPMOnlyJumppads(followedClient->sess.raceStyle.movementStyle)) {
				ignore++;
			}
		}
	}

	if (es->eType == ET_ITEM) {
		if (((followedClient->entityStates[entNum] || followedClient->triggerTimes[entNum] >= followedClient->pers.cmd.serverTime) && followedClient->sess.raceMode)
			|| ((other->goneForNonRacers || other->availableTimeForNonRacers >= level.time) && !followedClient->sess.raceMode)) {
			ignore++;
		}
	}

	if (other->belongsToParent) { // sniper shots, lightning, etc
		if (other->parent != ent && other->parent != followedEnt) { // our own and our followed's things are allowed by default
			// shouldn't really need this check but let's be safe
			tmp = GetPlayerVisibilityForPlayer(cl, canSeeTASClients, other->parent->client);
			APPLYVIS(tmp);
		}
	}

	// TODO rethink this. see comments below.
	if (es->eType == ET_BEAM &&/* other->parent != ent &&*/ es->generic1 == 3) {
		if (other->parent != ent) {
			if (cl->sess.hideLasers || (cl->sess.ignore & (1 << es->owner))) { // don't wanna see lasers period
				ignore++;
			}
			else { // otherwise treat same as any other player-owned thing 
				tmp = GetPlayerVisibilityForPlayer(cl, canSeeTASClients, other->parent->client);
				APPLYVIS(tmp);
			}
		}
	}
	if (other->client) {
		ocl = other->client;
		if (ocl != followedClient && ocl != cl) {
			// obviously only point to do this if we are not this person or not following this person
			tmp = GetPlayerVisibilityForPlayer(cl, canSeeTASClients, ocl);
			APPLYVIS(tmp);
		}
	}

	if (ignore > 0) { // ignore takes precedence over enforce in engine
		return -ignore;
	}
	else {
		return enforce; // will be 0 if no enforce.
	}
}

#define VISIBILITYONCE 1 // do visibility calc for all recipient players once on the first playersnapshot

#if !VISIBILITYONCE
int GetEntVisibilityForPlayer(int clientNum, int entNum, gentity_t* other, mvsharedEntity_t* mvEnt) {

	gentity_t* ent = g_entities + clientNum;
	gclient_t* cl = ent->client;
	qboolean	canSeeTASClients = (cl->sess.solo == SOLO_SHOWALL || cl->pers.isHeadlessClient || (cl->pers.ttClientFlags & TTFLAGS_CLIENT_SHOWALLPLAYERSINCLUDINGMLBOTS));
	//int followedClientNum = (cl->sess.spectatorState == SPECTATOR_FOLLOW && cl->sess.spectatorClient >= 0 && cl->sess.spectatorClient < MAX_CLIENTS) ? cl->sess.spectatorClient : clientNum;
	int followedClientNum = cl->ps.clientNum; // is this simplification ok? should be, right? we are at snapshot creation stage, this is all that really matters?
	gentity_t* followedEnt = g_entities + followedClientNum;
	gclient_t* followedClient = followedEnt->client;

	return GetEntVisibilityForPlayerReal(ent, clientNum, entNum, other, mvEnt, cl, followedEnt, followedClient, canSeeTASClients);
}
#endif


typedef struct playerSnapshotBackupValues_s {
	int solidValue;
	int saberMove;
	int saberMovePS;
	int pmfFollowPS;
	//int event;
	//int	trTime;
	vec3_t	psMoverOldPos;
	mvsharedEntity_t	mvEntState;
} playerSnapshotBackupValues_t;

static playerSnapshotBackupValues_t backupValues[MAX_GENTITIES];


static void PlayerSnapshotUpdateEntityVis() {
	int entitites[MAX_GENTITIES];
	int entityCount = 0;
	int i;
	int clientNum, entityNum;
	gentity_t *ent, *other, *followedEnt;
	gclient_t *cl, *followedClient;
	qboolean	canSeeTASClients;
	int followedClientNum;
	playerSnapshotBackupValues_t* backup;
	mvsharedEntity_t* mvEnt;
	int visibility;
	int innerLoopCount = 0;

	// first make a quick list of entities. so we don't loop over hundreds of unused entities for each client
	other = g_entities;
	for (i = 0; i < level.num_entities; i++, other++) {
		if (!other->inuse) {
			continue;
		}
		entitites[entityCount++] = i;
	}

	// only do this once, its not client-specific
	ent = g_entities;
	for (clientNum = 0; clientNum < MAX_CLIENTS; clientNum++, ent++) {
		if (!ent->inuse) {
			continue;
		}
		
		// set up some vars for the check
		cl = ent->client;
		canSeeTASClients = (cl->sess.solo == SOLO_SHOWALL || cl->pers.isHeadlessClient || (cl->pers.ttClientFlags & TTFLAGS_CLIENT_SHOWALLPLAYERSINCLUDINGMLBOTS));
		//int followedClientNum = (cl->sess.spectatorState == SPECTATOR_FOLLOW && cl->sess.spectatorClient >= 0 && cl->sess.spectatorClient < MAX_CLIENTS) ? cl->sess.spectatorClient : clientNum;
		followedClientNum = cl->ps.clientNum; // is this simplification ok? should be, right? we are at snapshot creation stage, this is all that really matters?
		followedEnt = g_entities + followedClientNum;
		followedClient = followedEnt->client;

		// do visibility now
		other = g_entities;
		backup = backupValues;
		for (i = 0; i < entityCount; i++, other++, backup++) {
			entityNum = entitites[i];
			other = g_entities + entityNum;
			backup = backupValues + entityNum;
			mvEnt = mv_entities + entityNum;
			visibility = GetEntVisibilityForPlayerReal(ent, clientNum, entityNum, other, &backup->mvEntState,cl, followedEnt, followedClient, canSeeTASClients );
			if (visibility < 0) {
				if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
					mvEnt->snapshotIgnoreRealClient[clientNum] = MIN(-visibility, 255);
				}
				else {
					mvEnt->snapshotIgnore[followedClientNum] = MIN(-visibility, 255);
				}
			}
			else if (visibility > 0) {
				if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
					mvEnt->snapshotEnforceRealClient[clientNum] = MIN(visibility, 255);
				}
				else {
					mvEnt->snapshotEnforce[followedClientNum] = MIN(visibility, 255);
				}
			}
			innerLoopCount++;
		}
	}

	return;
}

void PlayerSnapshotUpdateBroadcasts() {
	gentity_t* ent;
	int i;
	// only do this once, its not client-specific
	ent = g_entities;
	for (i = 0; i < MAX_CLIENTS; i++, ent++) {
		if (!ent->inuse) {
			continue;
		}

		if (ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
			// nicer place to do this :) this includes antiwh... so should reduce load a bit, especially with lower snaps, allowing us to reduce snaps if antiwh starts eating too much resoruces
			G_UpdateClientBroadcasts(ent);
		}

		if (level.playerStats[i]) { // only send player stats of active clients, dont be wasteful
			if ((g_entities+i)->inuse) {
				// client active
				level.playerStats[i]->r.svFlags |= SVF_BROADCAST;
			}
			else {
				level.playerStats[i]->r.svFlags &= ~SVF_BROADCAST;
			}
		}
	}
	
}

void PlayerSnapshotHackValues(qboolean saveState, int clientNum) {
	gentity_t* ent = g_entities + clientNum;
	gentity_t* other;
	gclient_t* cl = ent->client;
	gclient_t* ocl;
	entityState_t* es;
	playerSnapshotBackupValues_t* backup = backupValues;
	mvsharedEntity_t* mvEnt = mv_entities;
	int followedClientNum = (cl->sess.spectatorState == SPECTATOR_FOLLOW && cl->sess.spectatorClient >= 0 && cl->sess.spectatorClient < MAX_CLIENTS) ? cl->sess.spectatorClient : clientNum;
	gentity_t* followedEnt = g_entities + followedClientNum;
	gclient_t* followedClient = followedEnt->client;
	gclient_t* soloRelevantClient = (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) ? cl : followedClient;
	qboolean	canSeeTASClients = (soloRelevantClient->sess.solo == SOLO_SHOWALL || soloRelevantClient->pers.isHeadlessClient || (soloRelevantClient->pers.ttClientFlags & TTFLAGS_CLIENT_SHOWALLPLAYERSINCLUDINGMLBOTS));
	int i, originalValueReusable;
	int visibility;

	if (saveState) {
		PlayerSnapshotUpdateBroadcasts(); // doing this before we do mvent backups
	}

	for (i = 0; i < level.num_entities; i++, backup++, mvEnt++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		es = &other->s;
		if (saveState) {
			originalValueReusable = es->solid;
			backup->solidValue = es->solid;
			//backup->event = es->event;
			//if (es->eType == ET_MOVER) { // hackily "fix" client-timed mover prediction for cgame
				//backup->trTime = es->pos.trTime;
				//es->pos.trTime += level.time - ACTIVATORTIME(other->activatorReal);
			//}
			backup->mvEntState = *mvEnt; // cringe but eh.
		}
		else {
			originalValueReusable = backup->solidValue;
		}
		if (originalValueReusable && ShouldNotCollide(ent,other)) { // no need to check if it never was solid to begin with, and it caused console spam from misc_portal_surface owner shit
			es->solid = 0;
		}
		else if (!saveState){
			es->solid = originalValueReusable;
		}

#if !VISIBILITYONCE
		visibility = GetEntVisibilityForPlayer(clientNum, i, other, &backup->mvEntState);
		if (visibility < 0) {
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = MIN(-visibility,255);
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = MIN(-visibility, 255);
			}
		}
		else if (visibility > 0) {
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotEnforceRealClient[clientNum] = MIN(-visibility, 255);
			}
			else {
				mvEnt->snapshotEnforce[followedClientNum] = MIN(-visibility, 255);
			}
		}
#endif

		// avoid issues with custom lightsaber moves on clients.
		// it doesnt USUALLY crash but its an access past the end of the array and other compilers or sanitizers might cause a crash
		// also, cg_debugsabers 1 causes aa crash on cgame due to accessing a broken char* pointer
		// TODO: is sabermove used for anything else?
		// TODO: Don't do this if client has tommyternal client?
		if (saveState) backup->saberMove = es->saberMove;
		if (es->saberMove >= LS_MOVE_MAX_DEFAULT) {
			es->saberMove = LS_READY;
		}
		if (other->client) {
			ocl = other->client;
			
			if (saveState) { 
				backup->saberMovePS = ocl->ps.saberMove;
				backup->pmfFollowPS = ocl->ps.pm_flags & PMF_FOLLOW;
				VectorCopy(ocl->ps.origin, backup->psMoverOldPos);
				CG_AdjustPositionForClientTimeMover(ocl->ps.origin, ocl->ps.groundEntityNum, ocl->ps.origin); // silly bs (that doesnt work)
			}
			if (ocl->sess.raceMode && (ocl->sess.raceStyle.runFlags & RFL_SEGMENTED) && ocl->pers.segmented.state == SEG_REPLAY) {
				ocl->ps.pm_flags |= PMF_FOLLOW;
			}
			if (ocl->ps.saberMove >= LS_MOVE_MAX_DEFAULT) {
				ocl->ps.saberMove = LS_READY;
			}
		}
	}
#if VISIBILITYONCE
	if (saveState) {
		// gotta call this down here because it relies on backup mvEnt values already being set
		PlayerSnapshotUpdateEntityVis();
	}
#endif
}
void PlayerSnapshotRestoreValues() {
	gentity_t* other;
	gclient_t* cl;
	entityState_t* es;
	playerSnapshotBackupValues_t* backup = backupValues;
	mvsharedEntity_t* mvEnt = mv_entities;
	int i;
	for (i = 0; i < level.num_entities; i++, backup++, mvEnt++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		es = &other->s;
		es->solid = backup->solidValue;
		es->saberMove = backup->saberMove; 
		*mvEnt = backup->mvEntState;
		//es->event = backup->event; 
		//if (es->eType == ET_MOVER) {
		//	es->pos.trTime = backup->trTime;
		//}
		if (other->client) {
			cl = other->client;
			cl->ps.saberMove = backup->saberMovePS;
			cl->ps.pm_flags = (cl->ps.pm_flags & ~PMF_FOLLOW) | backup->pmfFollowPS;
			VectorCopy(backup->psMoverOldPos, cl->ps.origin);
		}
	}
}
