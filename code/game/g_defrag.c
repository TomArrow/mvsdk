

#include "g_local.h"
#include "g_dbcmds.h"

// Many parts of defrag code are lifted/adapted from Triforce's JediKnightPlus and loda's japro. Thanks!

void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);
const char* DF_RacePrintAppendage(finishedRunInfo_t* runInfo);
void DF_CheckpointTimer_Touch(gentity_t* trigger, gentity_t* activator, trace_t* trace);
void DF_CarryClientOverToNewRaceStyle(gentity_t* ent, raceStyle_t* newRs);
void UpdateClientRaceVars(gclient_t* client);

#define VALIDATEPTR(type, p) ((void*) (1 ? (p) : (type*)0)) // C/QVM compiler enforces this for us. little sanity check.
#define VALIDATEPTRCMP(j, p) ((void*) (1 ? (p) : (j))) // C/QVM compiler enforces this for us. little sanity check.
#define CLF_INT(a) (size_t)( VALIDATEPTR(int,&((gclient_t*)0)->a))
#define CLF_FLT(a) (size_t)( VALIDATEPTR(float,&((gclient_t*)0)->a))

#define ENTF_INT(a) (size_t)( VALIDATEPTR(int,&((gentity_t*)0)->a))
#define ENTF_FLT(a) (size_t)( VALIDATEPTR(float,&((gentity_t*)0)->a))

// we're using NT mod colors so lots of variety :)
// avoids black and dark blue on normal clients and has unique colors for every clientnum otherwise
// bit randomized too
char clientColors[MAX_CLIENTS] = { 
	'5',
	'3',
	'6',
	'N',
	'R',
	'E',
	'A',
	'V',
	'C',
	'K',
	'f',
	's',
	'g',
	'U',
	'J',
	'c',
	'j',
	'S',
	'o',
	'I',
	'b',
	'u',
	'O',
	'1',
	'B',
	'2',
	'v',
	'i',
	'w',
	'r',
	'7',
	'Y',
};

// TODO investigate timeresidual

#define FIELDSCLIENT()\
		FIELDSFUNC(buttons)\
		FIELDSFUNC(oldbuttons)\
		FIELDSFUNC(latched_buttons)\
		FIELDSFUNC(dangerTime)\
		FIELDSFUNC(fjDidJump)\
		FIELDSFUNC(forcePowerMicroRegenBuffer)\
		FIELDSFUNC(forcePowerSoundDebounce)\
		FIELDSFUNC(invulnerableTimer)\
		FIELDSFUNC(saberCycleQueue)\
		FIELDSFUNC(damage_armor)\
		FIELDSFUNC(damage_blood)\
		FIELDSFUNC(damage_fromWorld)\
		FIELDSFUNC(respawnTime)\
		FIELDSFUNC(rewardTime)\
		FIELDSFUNC(airOutTime)\
		FIELDSFUNC(fireHeld)\
		FIELDSFUNC(timeResidual)\
		FIELDSFUNC(lastSaberStorageTime)\
		FIELDSFUNC(hasCurrentPosition)\
		FIELDSFUNC(sess.saberLevel)\
		FIELDSFUNC(sess.selectedFP)\
		FIELDSFUNC(sess.setForce)\
		FIELDSFUNC(pers.teamState.flagsince)\
		FIELDSFUNC(pers.teamState.lastfraggedcarrier)\
		FIELDSFUNC(pers.teamState.lasthurtcarrier)\
		FIELDSFUNC(pers.teamState.lastreturnedflag)\
		FIELDSFUNC(pers.stats.distanceTraveled)\
		FIELDSFUNC(pers.stats.distanceTraveled2D)\
		FIELDSFUNC(pers.stats.topSpeed)\
		FIELDSFUNC(pers.raceDropped.msecTime)\
		FIELDSFUNC(pers.raceDropped.packetCount)\
		//FIELDSFUNC(damage_knockback)\ // not used anywhere?
		//FIELDSFUNC(sess.updateUITime)\ // not used anywhere?

#define FIELDSCLIENTVEC3()\
		FIELDSFUNC(damage_from)\
		FIELDSFUNC(lastSaberDir_Always)\
		FIELDSFUNC(lastSaberBase_Always)\

#define FIELDSENT()\
		FIELDSFUNC(health)\
		FIELDSFUNC(takedamage)\
		FIELDSFUNC(eventTime)\
		FIELDSFUNC(clipmask)\
		FIELDSFUNC(pain_debounce_time)\
		FIELDSFUNC(fly_sound_debounce_time)\
		FIELDSFUNC(watertype)\
		FIELDSFUNC(waterlevel)\
		FIELDSFUNC(r.contents)

#define FIELDSENTVEC3()\
		FIELDSFUNC(r.mins)\
		FIELDSFUNC(r.maxs)\
		FIELDSFUNC(r.currentOrigin)

#define TIMECOMPENSATEFIELDS()\
		FIELDSFUNC(pain_debounce_time)\
		FIELDSFUNC(fly_sound_debounce_time)\
		FIELDSFUNC(eventTime)\
		FIELDSFUNC(client->airOutTime)\
		FIELDSFUNC(client->dangerTime)\
		FIELDSFUNC(client->forcePowerSoundDebounce)\
		FIELDSFUNC(client->invulnerableTimer)\
		FIELDSFUNC(client->lastSaberStorageTime)\
		FIELDSFUNC(client->ps.duelTime)\
		FIELDSFUNC(client->ps.electrifyTime)\
		FIELDSFUNC(client->ps.externalEventTime)\
		FIELDSFUNC(client->ps.fallingToDeath)\
		FIELDSFUNC(client->ps.fd.forceGripUseTime)\
		FIELDSFUNC(client->ps.forceHandExtendTime)\
		FIELDSFUNC(client->ps.fd.forceGripUseTime)\
		FIELDSFUNC(client->ps.fd.forceHealTime)\
		FIELDSFUNC(client->ps.fd.forceJumpAddTime)\
		FIELDSFUNC(client->ps.fd.forcePowerRegenDebounceTime)\
		FIELDSFUNC(client->ps.fd.forceRageRecoveryTime)\
		FIELDSFUNC(client->ps.footstepTime)\
		FIELDSFUNC(client->ps.forceAllowDeactivateTime)\
		FIELDSFUNC(client->ps.forceGripMoveInterval)\
		FIELDSFUNC(client->ps.forceHandExtendTime)\
		FIELDSFUNC(client->ps.forceRageDrainTime)\
		FIELDSFUNC(client->ps.groundTime)\
		FIELDSFUNC(client->ps.holdMoveTime)\
		FIELDSFUNC(client->ps.lastOnGround)\
		FIELDSFUNC(client->ps.otherKillerDebounceTime)\
		FIELDSFUNC(client->ps.otherKillerTime)\
		FIELDSFUNC(client->ps.otherSoundTime)\
		FIELDSFUNC(client->ps.painTime)\
		FIELDSFUNC(client->ps.saberAttackWound)\
		FIELDSFUNC(client->ps.saberBlockTime)\
		FIELDSFUNC(client->ps.saberDidThrowTime)\
		FIELDSFUNC(client->ps.saberIdleWound)\
		FIELDSFUNC(client->ps.saberLockTime)\
		FIELDSFUNC(client->ps.saberThrowDelay)\
		FIELDSFUNC(client->ps.useDelay)\
		FIELDSFUNC(client->ps.weaponChargeTime)\
		FIELDSFUNC(client->ps.weaponChargeSubtractTime)\
		FIELDSFUNC(client->ps.zoomTime)\
		FIELDSFUNC(client->ps.zoomLockTime)\
		FIELDSFUNC(client->respawnTime)\
		FIELDSFUNC(client->rewardTime)\
		FIELDSFUNC(client->pers.teamState.flagsince)\
		FIELDSFUNC(client->pers.teamState.lastfraggedcarrier)\
		FIELDSFUNC(client->pers.teamState.lasthurtcarrier)\
		FIELDSFUNC(client->pers.teamState.lastreturnedflag)\
		FIELDSFUNC(client->ps.fd.forceDrainTime)\
		FIELDSFUNC(client->ps.fd.forceGripBeingGripped)\
		FIELDSFUNC(client->ps.fd.forceGripSoundTime)\
		FIELDSFUNC(client->ps.fd.forceGripStarted)\
		FIELDSFUNC(client->ps.rocketTargetTime)\
		FIELDSFUNC(client->ps.droneExistTime)\
		FIELDSFUNC(client->ps.droneFireTime)\
		FIELDSFUNC(client->ps.emplacedTime)\


#if SEGMENTEDDEBUG
// using the stringizing operator to save typing...
#define	SEGDEBCLF(x,type) #x,dbgtype_ ## type, (size_t)VALIDATEPTR(type,&((gclient_t*)0)->x),(size_t)VALIDATEPTR(type,&((segDebugVars_t*)0)->x), sizeof(type),#type

debugField_t	segDebugFields[] =
{
	{ SEGDEBCLF(ps.legsAnim, int) },
	{ SEGDEBCLF(ps.torsoAnim, int) },
	{ SEGDEBCLF(ps.saberMove,int ) },
	{ SEGDEBCLF(ps.origin, vec3_t) },
	{ SEGDEBCLF(ps.viewangles, vec3_t) },
	//{ SEGDEBCLF(pers.cmd.angles, veci3_t) }, // it will change for sure, but its not a problem. stop spam.
	{ SEGDEBCLF(pers.cmd.buttons, int) },
	{ SEGDEBCLF(pers.cmd.forwardmove, schar_t) },
	{ SEGDEBCLF(pers.cmd.rightmove, schar_t) },
	{ SEGDEBCLF(pers.cmd.upmove, schar_t) },
};
int segDebugFieldsCount = sizeof(segDebugFields) / sizeof(segDebugFields[0]);
#endif

// NOTE: For start timer, make sure we are not standing in any existing start timer before actually starting, 
// even when leave() is already being called. Only the last left start trigger should actually trigger.

// q3 defrag targets:
// target_starttimer
// target_stoptimer 
// target_checkpoint 

typedef enum q3DefragTargetType_s {
	TARGET_STARTTIMER,
	TARGET_STOPTIMER,
	TARGET_CHECKPOINT,
	TARGET_TYPE_COUNT
} q3DefragTargetType_t;

static const char* q3DefragTargetNames[] = {
	"target_startTimer",
	"target_stopTimer",
	"target_checkpoint"
};

void DF_InvalidateSpawn(gentity_t* ent) {
	if (!ent->client) return;

	ent->client->pers.savedSpawnUsed = qfalse;
}

/*
=====================================================================
Race trigger functions
=====================================================================
Lifted/adapted from JK+ and jaPRO, thanks to loda and TriForce and anyone else who contributed
*/
qboolean DF_PrePmoveValid(gentity_t* ent) {
	int cmdDelta = ent->client->ps.commandTime - ent->client->prePmoveCommandTime;
	// TODO lower limit from 10000? Just basic sanity check anyway
	return cmdDelta > 0 && cmdDelta < 10000 && ent->client->prePmovePositionSet && !((ent->client->ps.eFlags ^ ent->client->prePmoveEFlags) & EF_TELEPORT_BIT);
}
qboolean DF_InTrigger(vec3_t interpOrigin, gentity_t* trigger)
{
	vec3_t	mins, maxs;
	vec3_t	playerMins, playerMaxs;

	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	VectorAdd(interpOrigin, playerMins, mins);
	VectorAdd(interpOrigin, playerMaxs, maxs);

	if (trap_EntityContact(mins, maxs, trigger)) return qtrue;

	return qfalse;
}
qboolean DF_InAnyTrigger(vec3_t interpOrigin, const char* classname, vec3_t playerMins, vec3_t playerMaxs, gentity_t* activator) // TODO make this more efficient
{
	vec3_t	mins, maxs;
	//vec3_t	playerMins, playerMaxs;
	gentity_t* trigger;

	//VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	//VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	VectorAdd(interpOrigin, playerMins, mins);
	VectorAdd(interpOrigin, playerMaxs, maxs);

	trigger = NULL;
	while ((trigger = G_Find(trigger, FOFS(classname), classname)) != NULL) {
		if (trigger->triggerClientSpecific && trigger->parent != activator) continue;
		if (trap_EntityContact(mins, maxs, trigger)) return qtrue;
	}

	return qfalse;
}

int DF_InterpolateTouchTimeToOldPos(gentity_t* activator, gentity_t* trigger, const char* classname, vec3_t displacementVector) // For finish and checkpoint trigger
{
	vec3_t	interpOrigin, oldInterpOrigin, delta;
	int lessTime = -1;

	int msecDelta = activator->client->ps.commandTime- activator->client->prePmoveCommandTime;
	qboolean touched = qfalse;
	qboolean inTrigger;
	float msecScale = 1.0f / (float)msecDelta;

	VectorCopy(activator->client->postPmovePosition, interpOrigin);
	VectorSubtract(activator->client->prePmovePosition, activator->client->postPmovePosition,delta);
	VectorScale(delta, msecScale, delta);

	//while ((inTrigger = DF_InTrigger(interpOrigin, trigger)) || !touched)
	while ((inTrigger = DF_InAnyTrigger(interpOrigin, classname,activator->client->triggerMins,activator->client->triggerMaxs, activator)) || !touched)
	{
#if 1
		// with normal trace it can happen that the trace hits a trigger due to epsilom, but entitycontact returns false (because the bounding boxes actually
		// DONT overlap. for a finish/checkpoint trigger, this means that touched=qtrue would never be set with the old algo, so this whole loop is pointless
		// as lessTime will go up until it becomes so big the safety break happens.
		// We now use JP_TracePrecise for trigger tracing, which doesn't use epsilon, but let's be safe anyway, just in case. If that were to happen,
		// we'd want lessTime to end up 0 anyway, as it means we are just hitting the finish trigger with the sweat molecules emanating
		// 1 micrometer from our skin, so this is correct.
		assert(touched || inTrigger);
		touched = qtrue;
#else
		if (inTrigger) touched = qtrue;
		else if (!touched) {

			trace_t trace;
			DF_InAnyTrigger(interpOrigin, classname, activator->client->triggerMins, activator->client->triggerMaxs);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->prePmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->postPmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->postPmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->prePmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
		}
#endif

		lessTime++;
		VectorCopy(interpOrigin, oldInterpOrigin);
		VectorAdd(interpOrigin, delta, interpOrigin);
#if DEBUG
		if (lessTime >= (msecDelta + 100)) break; // just to sanity test a bit
#else
		if (lessTime >= (msecDelta - 1)) break; // if we were forced to go back msecDelta, that would put as at the pre-pmove position. But since race triggers are traced, we are guaranteed to have NOT been in it at the time, so the only way lessTime could be msecDelta or more is if there was some error in the code or floating point imprecision
#endif
	}
#if DEBUG
	assert(lessTime <= msecDelta); // float imprecision could MAYBE, in a freak situation, put as at msecDelta, but definitely no further.
#endif

	VectorSubtract(oldInterpOrigin, activator->client->postPmovePosition, displacementVector);

	return lessTime;
}
int DF_InterpolateTouchTimeForStartTimer(gentity_t* activator, gentity_t* trigger,vec3_t displacementVector) // For start trigger
{
	// TODO: Make this check for ANY start triggers
	vec3_t	interpOrigin, oldInterpOrigin, delta;
	int lessTime = -1;

	int msecDelta = activator->client->ps.commandTime- activator->client->prePmoveCommandTime;
	qboolean left = qfalse;
	qboolean inTrigger;
	float msecScale = 1.0f / (float)msecDelta;

	VectorCopy(activator->client->postPmovePosition, interpOrigin);
	VectorSubtract(activator->client->prePmovePosition, activator->client->postPmovePosition,delta);
	VectorScale(delta, msecScale, delta);

	//while (!(inTrigger = DF_InTrigger(interpOrigin, trigger)) || !left)
	while (!(inTrigger = DF_InAnyTrigger(interpOrigin,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs, activator)) || !left)
	{
#if 1
		// with normal trace it can happen that the trace hits a trigger due to epsilom, but entitycontact returns false (because the bounding boxes actually
		// DONT overlap. for a finish/checkpoint trigger, this means that left=qtrue would never be set with the old algo, so this whole loop is pointless
		// as lessTime will go up until it becomes so big the safety break happens.
		// We now use JP_TracePrecise for trigger tracing, which doesn't use epsilon, but let's be safe anyway, just in case. If that were to happen,
		// we'd want lessTime to end up 0 anyway, as it means we are just hitting the start trigger with the sweat molecules emanating
		// 1 micrometer from our skin, so this is correct.
		assert(left || !inTrigger);
		left = qtrue;
#else
		if (!inTrigger) left = qtrue;
		else if (!left) {
			DF_InAnyTrigger(interpOrigin, "df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs);
		}
#endif

		lessTime++;
		VectorCopy(interpOrigin,oldInterpOrigin);
		VectorAdd(interpOrigin, delta, interpOrigin);

#if DEBUG
		if (lessTime >= (msecDelta + 100)) break; // just to sanity test a bit
#else
		if (lessTime >= (msecDelta - 1)) break; // if we were forced to go back msecDelta, that would put as at the pre-pmove position. But since race triggers are traced, we are guaranteed to have been in it at the time, so the only way lessTime could be msecDelta or more is if there was some error in the code or floating point imprecision
#endif
	}
#if DEBUG
	assert(lessTime <= msecDelta); // float imprecision could MAYBE, in a freak situation, put as at msecDelta, but definitely no further.
#endif

	VectorSubtract(oldInterpOrigin, activator->client->postPmovePosition,displacementVector);

	return lessTime;
}



// Start race timer
void DF_StartTimer_Leave(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	int	lessTime = 0;
	qboolean segmented = qfalse;
	vec3_t interpolationDisplacement;
	gclient_t* cl;
	mainLeaderboardType_t lbType;
	int resposCountSave, savePosCountSave;

	// Check client
	if (!activator->client) return;

	cl = activator->client;

	lbType = classifyLeaderBoard(&cl->sess.raceStyle, &level.mapDefaultRaceStyle);

	if (!cl->sess.raceMode 
		|| cl->ps.pm_type != PM_NORMAL 
		|| cl->ps.stats[STAT_HEALTH] <= 0 
		|| cl->sess.sessionTeam != TEAM_FREE
		//|| cl->ps.duelInProgress && !cl->sess.raceMode // irrelevant, we dont allow non-racemoders to run anyway
		//|| cl->ps.legsAnim == BOTH_JUMPATTACK6 // jka only thing?
		|| cl->pers.lastRaceResetTime == level.time //Dont allow a starttimer to start in the same frame as a resettimer. not like that can happen anyway?
		|| !trap_InPVS(cl->ps.origin, cl->ps.origin) // out of bounds fix? does this need extra checks due to trace/interpolation?
		) return;

	if (cl->sess.raceStateInvalidated) {
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Your race state is invalidated. Please respawn before running.",qfalse,qtrue,qtrue);
		return;
	}
	if (cl->sess.raceStateSoftInvalidated) {
		DF_RaceStateInvalidated(activator,qfalse);
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Your race state is soft-invalidated. Please respawn before running.",qfalse,qtrue,qtrue);
		return;
	}

	segmented = cl->sess.raceStyle.runFlags & RFL_SEGMENTED;

	if (segmented && cl->pers.segmented.state != SEG_RECORDING && cl->pers.segmented.state != SEG_REPLAY) {
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run in a faulty state. Please respawn and try again.",qfalse,qtrue,qtrue);
		DF_RaceStateInvalidated(activator, qfalse);
		return;
	}
	else if (segmented && cl->pers.segmented.state != SEG_REPLAY) {

		if (segmented && cl->pers.segmented.msecProgress > 5000) {
			G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run pre-record is over 5 seconds. Please respawn and try again.",qfalse,qtrue,qfalse);
			DF_RaceStateInvalidated(activator, qfalse);
			return;
		}
		else if (segmented && cl->pers.segmented.msecProgress < 500) {
			G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run pre-record is under 0.5 seconds. Please respawn and try again.",qfalse,qtrue,qfalse);
			DF_RaceStateInvalidated(activator, qfalse);
			return;
		}
	}

	if (DF_InAnyTrigger(cl->postPmovePosition,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs, activator)) return; // we are still in some start trigger.

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Start Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", cl->pers.netname);
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7No valid pre-pmove info. Please restart.",qfalse,qtrue,qfalse);
		return;
	}
	else {
		lessTime = DF_InterpolateTouchTimeForStartTimer(activator, ent, interpolationDisplacement);
	}

	resposCountSave = cl->pers.stats.resposCount;
	savePosCountSave = cl->pers.stats.saveposCount;
	memset(&cl->pers.stats, 0, sizeof(cl->pers.stats)); // reset & initialize run stats
	if (segmented && cl->pers.segmented.state == SEG_REPLAY) { // remember the amount of savepos/respos used during segmented run
		cl->pers.stats.resposCount = resposCountSave;
		cl->pers.stats.saveposCount = savePosCountSave;
	}
	cl->pers.stats.startLevelTime = level.time;
	cl->pers.stats.startLessTime = lessTime;
	cl->pers.stats.distanceTraveled = VectorLength(interpolationDisplacement);
	interpolationDisplacement[2] = 0;
	cl->pers.stats.distanceTraveled2D = VectorLength(interpolationDisplacement);
	cl->pers.stats.topSpeed = XYSPEED(cl->ps.velocity);

	// Set timers
	//activator->client->ps.duelTime = activator->client->ps.commandTime - lessTime;
	cl->pers.raceStartCommandTime = activator->client->ps.commandTime - lessTime;
	//cl->pers.segmented.lastPosUsed = qfalse; // already guaranteed via SEG_RECORDING check above

	memset(&cl->pers.raceDropped,0,sizeof(cl->pers.raceDropped)); // reset info aabout packets dropped due to wrong fps timing

	if (!cl->sess.login.loggedIn) {
		G_CenterPrint(activator - g_entities, 3, va("^%cRace timer started! ^1Warning: Not logged in.",lbType == LB_MAIN ? '7':'O', level.nonDeterministicEntities), qfalse, qtrue, qfalse);
	}
	else if (segmented && level.nonDeterministicEntities) {
		G_CenterPrint(activator - g_entities,3, va("^%cRace timer started! ^1Warning: ^7Map has %i non-deterministic entities. Replay/run may fail.", lbType == LB_MAIN ? '7' : 'O', level.nonDeterministicEntities),qfalse, qtrue,qfalse);
	}
	else {
		G_CenterPrint(activator - g_entities,3, va("^%cRace timer started!", lbType == LB_MAIN ? '7' : 'O'),qfalse,qtrue,qfalse);
	}
}


//qboolean ValidRaceSettings(int restrictions, gentity_t* player)
qboolean ValidRaceSettings(gentity_t* player)
{ //How 2 check if cvars were valid the whole time of run.. and before? since you can get a headstart with higher g_speed before hitting start timer? :S
	//Make most of this hardcoded into racemode..? speed, knockback, debugmelee, stepslidefix, gravity
	int style;
	if (!player->client)
		return qfalse;

	if (!player->client->ps.stats[STAT_RACEMODE])
		return qfalse;

	style = player->client->sess.raceStyle.movementStyle;

	//if (style == MV_OCPM)
	//	return qfalse;//temp

	//if (player->client->sess.accountFlags & JAPRO_ACCOUNTFLAG_NORACE)
	//	return qfalse;
	//if ((style == MV_RJQ3 || style == MV_RJCPM || style == MV_TRIBES) && g_knockback.value != 1000.0f)
	//	return qfalse;

	if (player->client->sess.raceStyle.jumpLevel >= 0) {

		if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != player->client->sess.raceStyle.jumpLevel) {
			return qfalse; // shouldnt happen
		}
	}
	else if (player->client->sess.raceStyle.jumpLevel < -1){
		return qfalse; // shouldnt happen
	}
	else {
		if (player->client->ps.powerups[PW_YSALAMIRI] != INT_MAX) {
			return qfalse; // shouldnt happen
		}
	}

	//if (style != MV_CPM && style != MV_OCPM && style != MV_Q3 && style != MV_WSW && style != MV_RJQ3 && style != MV_RJCPM && style != MV_JETPACK && style != MV_SWOOP && style != MV_JETPACK && style != MV_SLICK && style != MV_BOTCPM && style != MV_COOP_JKA && style != MV_TRIBES) { //Ignore forcejump restrictions if in onlybhop movement modes
	//	if (restrictions & (1 << 0)) {//flags 1 = restrict to jump1
	//		if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 1 || player->client->ps.powerups[PW_YSALAMIRI] > 0) {
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 1!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//	else if (restrictions & (1 << 1)) {//flags 2 = restrict to jump2
	//		if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 2 || player->client->ps.powerups[PW_YSALAMIRI] > 0) {
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//	else if (restrictions & (1 << 2)) {//flags 4 = only jump3
	//		if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 3 || player->client->ps.powerups[PW_YSALAMIRI] > 0) { //Also dont allow ysal in FJ specified courses..?
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//}
	//else if (style == MV_COOP_JKA) {
	//	if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] == 2 && !(restrictions & (1 << 1))) {//using jump2 but its not allowed
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course does not allow force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
	//		return qfalse;
	//	}
	//	if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] == 3 && !(restrictions & (1 << 2))) {//using jump3 but its not allowed
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course does not allow force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
	//		return qfalse;
	//	}
	//}

	//if (player->client->pers.haste && !(restrictions & (1 << 3)))
	//	return qfalse; //IF client has haste, and the course does not allow haste, dont count it.
	//if (((style != MV_JETPACK) && (style != MV_TRIBES)) && (player->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)) && !(restrictions & (1 << 4))) //kinda deprecated.. maybe just never allow jetpack?
	//	return qfalse; //IF client has jetpack, and the course does not allow jetpack, dont count it.
	//if (style == MV_SWOOP && !player->client->ps.m_iVehicleNum)
	//	return qfalse;
	//if (sv_cheats.integer)
//#ifndef DEBUG // always disallow? idk
	if (g_cheats.integer)
		return qfalse;
//#endif
	//if (!g_stepSlideFix.integer)
	//	return qfalse;
	//if (g_jediVmerc.integer) //umm..   ta: ??
	//	return qfalse;
	if (g_debugMelee.integer >= 2 && (player->client->sess.raceStyle.runFlags & RFL_CLIMBTECH))
		return qfalse;
	if (!g_smoothClients.integer)// why?
		return qfalse;
	//if (sv_fps.integer != 20 && sv_fps.integer != 30 && sv_fps.integer != 40)//Dosnt really make a difference.. but eh.... loda fixme
	if (g_sv_fps.integer != 100)// Does this even matter for tommyternal? everything runs on clienttime anyway. well... but demos wouldnt be proper without it, so leave it.
		return qfalse;
	//if (sv_pluginKey.integer) {
	//	if (!player->client->pers.validPlugin && player->client->pers.userName[0]) { //Meh.. only do this if they are logged in to keep the print colors working right i guess..
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: a newer client plugin version\nis required!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
	//		return qfalse;
	//	}
	//}
	//if (player->client->pers.noFollow && (player->client->sess.movementStyle != MV_SIEGE) && (g_allowNoFollow.integer < 3))
	//	return qfalse;
	//if (player->client->pers.practice)
	//	return qfalse;
	//if ((restrictions & (1 << 5)) && (level.gametype == GT_CTF || level.gametype == GT_CTY))//spawnflags 32 is FFA_ONLY
	//	return qfalse;
	//if ((player->client->ps.stats[STAT_RESTRICTIONS] & JAPRO_RESTRICT_ALLOWTELES) && !(restrictions & (1 << 6))) //spawnflags 64 on end trigger is allow_teles
	//	return qfalse;

	return qtrue;
}

// japro thing. weird?
void PlayActualGlobalSound(int soundindex) {
	gentity_t* player;
	int i;

	//G_AddEvent(ent, EV_GLOBAL_SOUND, soundindex); //need to svf_broadcast firsT? and what ent to use ??

	for (i = 0; i < MAX_CLIENTS; i++) {//Build a list of clients
		if (!g_entities[i].inuse)
			continue;
		player = &g_entities[i];
		G_Sound(player, CHAN_AUTO, soundindex);
	}
}

qboolean DF_RemoveCheckPoints(gentity_t* playerent) {
	int i;
	gentity_t* shield;

	for (i = 0; i < playerent->client->pers.df_checkpointData.count; i++) {
		shield = g_entities + playerent->client->pers.df_checkpointData.checkpointNumbers[i];
		if (shield->inuse) {
			G_FreeEntity(shield);
		}
	}

	playerent->client->pers.df_checkpointData.count = 0;
}

// sanity check that the client still exists and knows about this checkpoint andsuch.
void df_checkCheckpointValid(gentity_t* ent) {
	gentity_t* owner;
	int i;
	int shieldNum;

	if (ent->s.owner < 0 || ent->s.owner >= MAX_CLIENTS) {
		goto freeme;
		return;
	}
	owner = g_entities + ent->s.owner;

	if (!owner->inuse || !owner->client || owner->client->pers.connected != CON_CONNECTED) {
		goto freeme;
		return;
	}

	shieldNum = ent - g_entities;
	for (i = 0; i < owner->client->pers.df_checkpointData.count; i++) {
		if (owner->client->pers.df_checkpointData.checkpointNumbers[i] == shieldNum) {

			// ok the client is still active and still knows about this checkpoint. keep it.
			ent->think = df_checkCheckpointValid;
			ent->nextthink = level.time + 1000;
			return;
		}
	}

freeme:
	ent->think = 0;
	ent->nextthink = 0;
	G_FreeEntity(ent);

}


#define SHIELD_HEALTH				250
#define SHIELD_HEALTH_DEC			10		// 25 seconds	
#define MAX_SHIELD_HEIGHT			254
#define MAX_SHIELD_HALFWIDTH		255
#define SHIELD_PLACEDIST			64
void df_createCheckpoint(gentity_t* ent)
{
	trace_t		tr;
	vec3_t		mins, maxs, end, posTraceEnd, negTraceEnd, start;
	int			height, posWidth, negWidth, halfWidth = 0;
	qboolean	xaxis;
	int			paramData = 0;
	static int	shieldID;



	//mh_sendMessage(-1, "Shield Created");
	// trace upward to find height of shield
	VectorCopy(ent->r.currentOrigin, end);
	end[2] += MAX_SHIELD_HEIGHT;
	JP_Trace(&tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT);
	height = (int)(MAX_SHIELD_HEIGHT * tr.fraction);

	// use angles to find the proper axis along which to align the shield
	VectorSet(mins, -SHIELD_HALFTHICKNESS, -SHIELD_HALFTHICKNESS, 0);
	VectorSet(maxs, SHIELD_HALFTHICKNESS, SHIELD_HALFTHICKNESS, height);
	VectorCopy(ent->r.currentOrigin, posTraceEnd);
	VectorCopy(ent->r.currentOrigin, negTraceEnd);

	if ((int)(ent->s.angles[YAW]) == 0) // shield runs along y-axis
	{
		posTraceEnd[1] += MAX_SHIELD_HALFWIDTH;
		negTraceEnd[1] -= MAX_SHIELD_HALFWIDTH;
		xaxis = qfalse;
	}
	else  // shield runs along x-axis
	{
		posTraceEnd[0] += MAX_SHIELD_HALFWIDTH;
		negTraceEnd[0] -= MAX_SHIELD_HALFWIDTH;
		xaxis = qtrue;
	}

	// trace horizontally to find extend of shield
	// positive trace
	VectorCopy(ent->r.currentOrigin, start);
	start[2] += (height >> 1);
	JP_Trace(&tr, start, 0, 0, posTraceEnd, ent->s.number, MASK_SHOT);
	posWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;
	// negative trace
	JP_Trace(&tr, start, 0, 0, negTraceEnd, ent->s.number, MASK_SHOT);
	negWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;

	// kef -- monkey with dimensions and place origin in center
	halfWidth = (posWidth + negWidth) >> 1;
	if (xaxis)
	{
		ent->r.currentOrigin[0] = ent->r.currentOrigin[0] - negWidth + halfWidth;
	}
	else
	{
		ent->r.currentOrigin[1] = ent->r.currentOrigin[1] - negWidth + halfWidth;
	}
	ent->r.currentOrigin[2] += (height >> 1);

	// set entity's mins and maxs to new values, make it solid, and link it
	if (xaxis)
	{
		VectorSet(ent->r.mins, -halfWidth, -SHIELD_HALFTHICKNESS, -(height >> 1));
		VectorSet(ent->r.maxs, halfWidth, SHIELD_HALFTHICKNESS, height >> 1);
	}
	else
	{
		VectorSet(ent->r.mins, -SHIELD_HALFTHICKNESS, -halfWidth, -(height >> 1));
		VectorSet(ent->r.maxs, SHIELD_HALFTHICKNESS, halfWidth, height);
	}
	ent->clipmask = 0;
	ent->r.contents = CONTENTS_TRIGGER;


	paramData = (xaxis << 24) | (height << 16) | (posWidth << 8) | (negWidth);
	ent->s.time2 = paramData;


	//ent->touch = df_touchCustomCheckpoint;




	//ent->nextthink = 0;
	//ent->think = 0;// CheckpointThink;

	ent->think = df_checkCheckpointValid;
	ent->nextthink = level.time + 1000;


	trap_LinkEntity(ent);



	//ShieldGoSolid(ent);

	return;
}


qboolean DF_CloneCustomCheckpoint(gentity_t* oldShield, gentity_t* playerent) {
	gentity_t* shield;

	if (playerent->client->pers.df_checkpointData.count >= MAX_CUSTOM_CHECKPOINT_COUNT) return qfalse;

	shield = G_Spawn();
	shield->s.angles[YAW] = oldShield->s.angles[YAW];
	shield->parent = playerent;

	// Set team number.
	shield->s.otherEntityNum2 = TEAM_FREE;

	shield->s.eType = ET_SPECIAL;
	shield->s.modelindex = HI_SHIELD;	// this'll be used in CG_Useable() for rendering.
	shield->classname = "df_trigger_checkpoint";

	shield->r.contents = CONTENTS_TRIGGER;
	shield->triggerOnlyTraced = qtrue;
	shield->triggerClientSpecific = qtrue;

	shield->touch = DF_CheckpointTimer_Touch;
	shield->use = 0; 

	shield->s.pos.trType = TR_STATIONARY;
	shield->s.pos.trTime = 0;
	shield->s.pos.trDuration = 0;
	VectorClear(shield->s.pos.trDelta);

	shield->s.eFlags &= ~EF_NODRAW;
	shield->r.svFlags &= ~SVF_NOCLIENT;

	shield->r.svFlags |= SVF_SINGLECLIENT;
	shield->r.singleClient = playerent->s.number;


	shield->s.owner = playerent->s.number;
	shield->s.shouldtarget = qfalse;

	VectorCopy(oldShield->s.pos.trBase, shield->s.pos.trBase);
	VectorCopy(oldShield->r.currentOrigin, shield->r.currentOrigin);
	VectorCopy(oldShield->r.mins, shield->r.mins);
	VectorCopy(oldShield->r.maxs, shield->r.maxs);

	shield->clipmask = oldShield->clipmask;
	shield->checkpointSeed = oldShield->checkpointSeed;
	shield->r.contents = oldShield->r.contents;
	shield->s.time2 = oldShield->s.time2;

	trap_LinkEntity(shield);

	playerent->client->pers.df_checkpointData.checkpointNumbers[playerent->client->pers.df_checkpointData.count] = shield->s.number;
	playerent->client->pers.df_checkpointData.count++;

	return qtrue;
}

gentity_t* GetClientNumArg() {
	char	arg[MAX_STRING_CHARS];
	int sourcePlayer = -1;
	gentity_t* sourcePlayerEnt;
	if (trap_Argc() > 1)
	{
		trap_Argv(1, arg, sizeof(arg));

		if (arg[0])
		{
			sourcePlayer = atoi(arg);
			if (sourcePlayer >= 0 && sourcePlayer < MAX_CLIENTS) {
				return g_entities + sourcePlayer;
			}
		}
	}

	return NULL;

}

void DF_StealStyle(gentity_t* playerent) {

	gentity_t* sourcePlayerEnt = GetClientNumArg();
	int tmpMsec = playerent->client->sess.raceStyle.msec;

	if (!sourcePlayerEnt || !sourcePlayerEnt->inuse || !sourcePlayerEnt->client) {
		trap_SendServerCommand(playerent - g_entities, "print \"Please specify a valid client number whose style you wish to steal.\n\"");
		return;
	}

	if (!playerent->client->sess.raceMode) {
		trap_SendServerCommand(playerent - g_entities, "print \"Cannot steal style outside of racemode.\n\"");
		return;
	}
	if (!sourcePlayerEnt->client->sess.raceMode) {
		trap_SendServerCommand(playerent - g_entities, "print \"Specified client is not in racemode.\n\"");
		return;
	}

	if (sourcePlayerEnt->client->sess.raceStyle.msec > 0 && sourcePlayerEnt->client->sess.raceStyle.msec != tmpMsec) {
		G_SendServerCommand(playerent - g_entities, va("print \"Style stolen. You must manually set your com_physicsFps to %d.\n\"",(1000/ sourcePlayerEnt->client->sess.raceStyle.msec)),qtrue);
	}
	else {
		G_SendServerCommand(playerent - g_entities, "print \"Style stolen.\n\"",qtrue);
	}
	playerent->client->sess.raceStyle = sourcePlayerEnt->client->sess.raceStyle;
	playerent->client->sess.raceStyle.msec = tmpMsec;
	playerent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
	DF_RaceStateInvalidated(playerent, qtrue);

}
void DF_StealSpawn(gentity_t* playerent) {

	gentity_t* sourcePlayerEnt = GetClientNumArg();

	if (!sourcePlayerEnt || !sourcePlayerEnt->inuse || !sourcePlayerEnt->client) {
		trap_SendServerCommand(playerent - g_entities, "print \"Please specify a valid client number whose spawns you wish to steal.\n\"");
		return;
	}

	if (!sourcePlayerEnt->client->pers.savedSpawnUsed) {
		trap_SendServerCommand(playerent - g_entities, "print \"Specified client does not have a saved spawn.\n\"");
		return;
	}

	playerent->client->pers.savedSpawn = sourcePlayerEnt->client->pers.savedSpawn;
	playerent->client->pers.savedSpawn.ps.clientNum = playerent - g_entities;
	playerent->client->pers.savedSpawnRaceStyle = sourcePlayerEnt->client->pers.savedSpawnRaceStyle;
	playerent->client->pers.savedSpawnUsed = qtrue;

	if (memcmp(&playerent->client->sess.raceStyle, &sourcePlayerEnt->client->sess.raceStyle,sizeof(playerent->client->sess.raceStyle))) {
		G_SendServerCommand(playerent - g_entities, "print \"Spawn stolen. Racestyle differs - trying to steal racestyle too.\n\"",qtrue);
		DF_StealStyle(playerent);
	}
	else {
		G_SendServerCommand(playerent - g_entities, "print \"Spawn stolen.\n\"",qtrue);
	}

}

void DF_StealPos(gentity_t* playerent) {

	gentity_t* sourcePlayerEnt = GetClientNumArg();

	if (!sourcePlayerEnt || !sourcePlayerEnt->inuse || !sourcePlayerEnt->client) {
		trap_SendServerCommand(playerent - g_entities, "print \"Please specify a valid client number whose position you wish to steal.\n\"");
		return;
	}

	if (!sourcePlayerEnt->client->pers.savePosUsed) {
		if ((sourcePlayerEnt->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && sourcePlayerEnt->client->pers.segmented.state >= SEG_RECORDING_HAVELASTPOS) {
			trap_SendServerCommand(playerent - g_entities, "print \"Specified client does not have a saved position. Segmented run saved positions can't be stolen.\n\"");
		}
		else {
			trap_SendServerCommand(playerent - g_entities, "print \"Specified client does not have a saved position.\n\"");
		}
		return;
	}

	if ((sourcePlayerEnt->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && sourcePlayerEnt->client->pers.segmented.state >= SEG_RECORDING_HAVELASTPOS) {
		G_SendServerCommand(playerent - g_entities, "print \"Saved position stolen. Note that the specified client is in segmented race mode and segmented savepos positions cannot be stolen, so you may have gotten a different position than you wanted.\n\"",qtrue);
	}
	else {
		G_SendServerCommand(playerent - g_entities, "print \"Saved position stolen.\n\"",qtrue);
	}

	playerent->client->pers.savedPosition = sourcePlayerEnt->client->pers.savedPosition;
	playerent->client->pers.savedPosition.ps.clientNum = playerent - g_entities;
	playerent->client->pers.savePosUsed = qtrue;

}

void DF_StealCheckpoints(gentity_t* playerent) {
	int i;
	gentity_t* shield;
	gentity_t* sourcePlayerEnt = GetClientNumArg();
	int stolenChecks = 0;


	if (!sourcePlayerEnt || !sourcePlayerEnt->inuse || !sourcePlayerEnt->client) {
		trap_SendServerCommand(playerent - g_entities, "print \"Please specify a valid client number whose checkpoints you wish to steal.\n\"");
		return;
	}

	for (i = 0; i < sourcePlayerEnt->client->pers.df_checkpointData.count; i++) {
		shield = g_entities + sourcePlayerEnt->client->pers.df_checkpointData.checkpointNumbers[i];
		if (shield->inuse) {
			if (DF_CloneCustomCheckpoint(shield, playerent)) {
				stolenChecks++;
			}
			else {
				G_SendServerCommand(playerent - g_entities, "print \"^1Checkpoint limit reached. Can't steal or generate any more checkpoints.\n\"",qtrue);
				break;
			}
		}
	}
	if (stolenChecks) {
		G_SendServerCommand(playerent - g_entities, va("print \"%d checkpoints stolen.\n\"",stolenChecks),qtrue);
	}
	else {
		G_SendServerCommand(playerent - g_entities, "print \"^1No checkpoints stolen.\n\"",qtrue);
	}

}

qboolean DF_CreateCustomCheckpointFromPos(vec3_t trEndpos,float anglesYaw, gentity_t* playerent)
{
	// got enough room so place the portable shield
	gentity_t* shield;

	if (playerent->client->pers.df_checkpointData.count >= MAX_CUSTOM_CHECKPOINT_COUNT) return qfalse;

	shield = G_Spawn();

	VectorCopy(trEndpos, shield->checkpointSeed.trEndpos);
	shield->checkpointSeed.anglesYaw = anglesYaw;

	// Figure out what direction the shield is facing.
	shield->s.angles[YAW] = anglesYaw;
	shield->parent = playerent;

	// Set team number.
	shield->s.otherEntityNum2 = TEAM_FREE;

	shield->s.eType = ET_SPECIAL;
	shield->s.modelindex = HI_SHIELD;	// this'll be used in CG_Useable() for rendering.
	shield->classname = "df_trigger_checkpoint";

	shield->r.contents = CONTENTS_TRIGGER;
	shield->triggerOnlyTraced = qtrue;
	shield->triggerClientSpecific = qtrue;

	shield->touch = DF_CheckpointTimer_Touch;
	// using an item causes it to respawn
	shield->use = 0; //Use_Item;

	G_SetOrigin(shield, trEndpos);

	shield->s.eFlags &= ~EF_NODRAW;
	shield->r.svFlags &= ~SVF_NOCLIENT;

	shield->r.svFlags |= SVF_SINGLECLIENT;
	shield->r.singleClient = playerent->s.number;


	shield->s.owner = playerent->s.number;
	shield->s.shouldtarget = qfalse;

	playerent->client->pers.df_checkpointData.checkpointNumbers[playerent->client->pers.df_checkpointData.count] = shield->s.number;
	playerent->client->pers.df_checkpointData.count++;

	df_createCheckpoint(shield);

	return qtrue;
}

qboolean DF_CreateCustomCheckpoint(gentity_t* playerent)
{
	trace_t		tr;
	vec3_t		fwd, pos, dest, mins = { -4,-4, 0 }, maxs = { 4,4,4 };
	float		anglesYaw;

	if (playerent->client->pers.df_checkpointData.count >= MAX_CUSTOM_CHECKPOINT_COUNT) return qfalse;

	// can we place this in front of us?
	AngleVectors(playerent->client->ps.viewangles, fwd, NULL, NULL);
	fwd[2] = 0;
	VectorMA(playerent->client->ps.origin, SHIELD_PLACEDIST, fwd, dest);
	JP_Trace(&tr, playerent->client->ps.origin, mins, maxs, dest, playerent->s.number, MASK_SHOT);
	if (tr.fraction > 0.9)
	{//room in front
		VectorCopy(tr.endpos, pos);
		// drop to floor
		VectorSet(dest, pos[0], pos[1], pos[2] - 100);
		JP_Trace(&tr, pos, mins, maxs, dest, playerent->s.number, MASK_SOLID);
		if (!tr.startsolid && !tr.allsolid)
		{
			// got enough room so place the portable shield
			//shield = G_Spawn();

			// Figure out what direction the shield is facing.
			if (fabs(fwd[0]) > fabs(fwd[1]))
			{	// shield is north/south, facing east.
				anglesYaw = 0;
			}
			else
			{	// shield is along the east/west axis, facing north
				anglesYaw = 90;
			}

			DF_CreateCustomCheckpointFromPos(tr.endpos,anglesYaw,playerent);

			return qtrue;
		}
	}
	// no room
	return qfalse;
}

void PrintRaceTime(finishedRunInfo_t* runInfo, qboolean preliminary, qboolean showRank, gentity_t* ent) {
	char nameColor, color;
	//static char awardString[MAX_STRING_CHARS - 2] = { 0 };
	static char messageStr[MAX_STRING_CHARS - 2] = { 0 };
	static char fpsStr[10] = { 0 };
	const char* type = preliminary ? "dfsegprelim" : (showRank ? "dffinish_ranked" : "dffinish");
	const char* prefix = "";

	//awardString[0] = 0;
	messageStr[0] = 0;
	fpsStr[0] = 0;

	//Com_Printf("SOldrank %i SNewrank %i GOldrank %i GNewrank %i Addscore %.1f\n", season_oldRank, season_newRank, global_oldRank, global_newRank, addedScore);

	//if (topspeed || average) { //weird hack to not play double sound coop
	//	if (global_newRank == 1) {//WR, Play the sound
	//		if (worldrecordnoise)
	//			PlayActualGlobalSound(worldrecordnoise); //Only for simple PB not WR i guess..
	//		else if (worldrecordnoise != -1) {
	//			if (!level.wrNoise) {
	//				level.wrNoise = G_SoundIndex("sound/chars/rosh_boss/misc/victory3"); //Maybe this should be done when df_trigger_finish is spawned cuz its still gonna hitch maybe on first wr of map? idk
	//			}
	//			PlayActualGlobalSound(level.wrNoise);
	//		}
	//	}
	//	else if (global_newRank > 0) {//PB
	//		if (awesomenoise)
	//			PlayActualGlobalSound(awesomenoise);
	//		else if (awesomenoise != -1) {
	//			if (!level.pbNoise) {
	//				level.pbNoise = G_SoundIndex("sound/chars/rosh/misc/taunt1");
	//			}
	//			PlayActualGlobalSound(level.pbNoise);
	//		}
	//	}
	//}

	nameColor = clientColors[runInfo->clientNum & 31];
	//nameColor = 7 - (runInfo->clientNum % 8);//sad hack
	//if (nameColor < 2)
	//	nameColor = 2;
	//else if (nameColor > 7 || nameColor == 5)
	//	nameColor = 7;

	//if (valid && loggedin)
	if (!preliminary && runInfo->userId != -1) {
		if (runInfo->lbType == LB_MAIN) {
			color = '5';
		}
		else {
			color = 'V';
		}
	}
	else if (!preliminary) {
		if (runInfo->lbType == LB_MAIN) {
			color = '2';
		}
		else {
			color = 'b';
		}
	}
	else {
		if (runInfo->lbType == LB_MAIN) {
			color = '1';
		}
		else {
			color = 'Y';
		}
	}

	if (preliminary) {
		prefix = miniva("^%c[SEGMENTED-PRELIMINARY]^7 ",color);
	}

	if (!showRank) {
		const char* runFlagsString = RunFlagsToString(runInfo->raceStyle.runFlags, level.mapDefaultRaceStyle.runFlags, 1,"^3",NULL);
		const int runFlagsStringLen = strlen(runFlagsString);
		Q_strncpyz(fpsStr, runInfo->raceStyle.msec == -1 ? "togl" : (runInfo->raceStyle.msec == -2 ? "flt" : (runInfo->raceStyle.msec == 0 ? "unkn" : va("%d", 1000 / runInfo->raceStyle.msec))), sizeof(fpsStr));

		Q_strncpyz(messageStr, va("%s^%c%12s ^3%12s^%c  ^3%7.2f^%cmax ^3%7.2f^%cavg ^3%7.1fk^%cdist ^3%2i^%cj ^3%4s^%cfps ^3%6s^%c style %s ^%c",
			prefix,
			color,
			miniva("[%s]",leaderboardNames[runInfo->lbType]),
			DF_MsToString(runInfo->milliseconds),
			color,
			runInfo->topspeed,
			color,
			runInfo->average,
			color,
			runInfo->distance/1000.0f,
			color,
			runInfo->raceStyle.jumpLevel,
			color,
			fpsStr,
			color,
			moveStyleNames[runInfo->raceStyle.movementStyle].string,
			color,
			runFlagsString,
			color), sizeof(messageStr));
		//Q_strncpyz(awardString, va("%s ^%i[^%i%s^%i]", runInfo->netname, color, runInfo->userId == -1 ? 1 : nameColor, runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username, color), sizeof(awardString));
		//if (message)
		//Com_sprintf(messageStr, sizeof(messageStr), "^3%-16s^%i", runInfo->coursename, color);
		//else
		//	Com_sprintf(messageStr, sizeof(messageStr), "^%iCompleted", color);

		if (runInfo->raceStyle.runFlags & RFL_SEGMENTED) { //print number of teles?
			//if (level.clients[clientNum].midRunTeleCount < 1)
			//	Q_strcat(messageStr, sizeof(messageStr), " (PRO)");
			//else
			Q_strcat(messageStr, sizeof(messageStr), va("(^3%i^%c SP, ^3%i^%c RP) ", runInfo->savePosCount,color, runInfo->resposCount,color));
		}
		Q_strcat(messageStr, sizeof(messageStr), va("by^7 %s  ^%c[^%c%s^%c] ", runInfo->netname, color, runInfo->userId == -1 ? '1' : nameColor, runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username, color));
		

		trap_SendServerCommand(-1, va("print \"%s\n\" %s %s", messageStr, type, DF_RacePrintAppendage(runInfo)));

		if(ent && !preliminary)
			G_CenterPrint(ent - g_entities, 3, va("^7%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
	}
	else if (runInfo->rankLB != -1) {

		if (runInfo->rankLB == 1 && (runInfo->pbStatus & PB_LB)) { //was 1 when it shouldnt have been.. ?
			Q_strncpyz(messageStr, va("%s^%c%12s^7 %s ^%c[^%c%s^%c] %sbeat the ^3WORLD RECORD^%c and %s ranked ^3#%i\n",
				prefix,
				color,
				miniva("[%s]", leaderboardNames[runInfo->lbType]),
				runInfo->netname,color, runInfo->userId == -1 ? '1' : nameColor,
				runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username,
				color, 
				runInfo->userId == -1 ? "unofficially " : "",
				color, 
				runInfo->userId == -1 ? "would be " : "is now",
				runInfo->rankLB), 
				sizeof(messageStr));
			if (runInfo->userId != -1) {
				//G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass"));
				if (runInfo->lbType == LB_MAIN) {
					PlayActualGlobalSound(G_SoundIndex("sound/movers/sec_panel_pass"));
					if (ent) {
						G_ScreenShake(vec3_origin, ent, 5.0f, 800, qfalse);
					}
				}
			}

			if(ent && !preliminary)
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
		}
		else if ((runInfo->pbStatus & PB_LB)) {
			Q_strncpyz(messageStr, va("%s^%c%12s^7 %s ^%c[^%c%s^%c] got a new personal best and %s ranked ^3#%i\n",
				prefix,
				color,
				miniva("[%s]", leaderboardNames[runInfo->lbType]), 
				runInfo->netname, 
				color, 
				runInfo->userId == -1 ? '1' : nameColor, 
				runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username,
				color,  
				runInfo->userId == -1 ? "would be " : "is now", 
				runInfo->rankLB), 
				sizeof(messageStr));
			if (runInfo->rankLB <= 10 && runInfo->lbType == LB_MAIN && ent && !preliminary) {
				G_ScreenShake(vec3_origin, ent, 5.0f, 800, qfalse);
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
			}
			if (runInfo->rankLB <= 10 && ent && !preliminary) {
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
			}
			else if(ent && !preliminary){
				G_CenterPrint(ent - g_entities, 3, va("^5%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
			}
		}
		else if(ent && !preliminary){
			G_CenterPrint(ent - g_entities, 3, va("^7%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse);
		}

		/*if (global_newRank > 0) { //Print global rank increased, global score added
			if (global_newRank != global_oldRank) {//Can be from -1 to #.  What do we do in this case..
				if (global_oldRank > 0)
					Q_strcat(awardString, sizeof(awardString), va(" (%i->%i +%.1f)", global_oldRank, global_newRank, addedScore));
				else
					Q_strcat(awardString, sizeof(awardString), va(" (%i +%.1f)", global_newRank, addedScore));
			}
		}
		else if (season_newRank > 0) {//Print season rank increased, global score added
			if (season_newRank != season_oldRank) {
				if (season_oldRank > 0)
					Q_strcat(awardString, sizeof(awardString), va(" (%i->%i +%.1f)", season_oldRank, season_newRank, addedScore));
				else
					Q_strcat(awardString, sizeof(awardString), va(" (%i +%.1f)", season_newRank, addedScore));
			}
		}*/

		trap_SendServerCommand(-1, va("print \"%s\" %s %s", messageStr,type, DF_RacePrintAppendage(runInfo)));
	}
	
}

static int DF_GetNewRunId() {
	char s[15];
	int num;
	trap_Cvar_VariableStringBuffer("g_defragLastRunId", s, sizeof(s));
	num = atoi(s);
	num++;
	trap_Cvar_Set("g_defragLastRunId", va("%d", num));
	return num;
}

const char* DF_GetCourseName() {
	static char serverInfo[BIG_INFO_STRING];
	static char course[COURSENAME_MAX_LEN + 1];
	trap_GetServerinfo(serverInfo, sizeof(serverInfo));
	Q_strncpyz(course, Info_ValueForKey(serverInfo, "mapname"), sizeof(course));
	return course;
}

static void DF_FillClientRunInfo(finishedRunInfo_t* runInfo, gentity_t* ent, int milliseconds) {
	static char serverInfo[BIG_INFO_STRING];
	//static char course[COURSENAME_MAX_LEN + 1];
	gclient_t* client = ent->client;
	if (!client || !client->sess.raceMode) return;
	runInfo->clientNum = ent - g_entities;
	Q_strncpyz(runInfo->netname, client->pers.netname, sizeof(runInfo->netname));
	if (client->sess.login.loggedIn) {
		runInfo->userId = client->sess.login.id;
		Q_strncpyz(runInfo->username, client->sess.login.name, sizeof(runInfo->username));
	}
	else {
		runInfo->userId = -1;
		Q_strncpyz(runInfo->username, "!unlogged!", sizeof(runInfo->username));
	}
	runInfo->raceStyle = client->sess.raceStyle;
	runInfo->lbType = classifyLeaderBoard(&runInfo->raceStyle, &level.mapDefaultRaceStyle);;
	trap_GetServerinfo(serverInfo, sizeof(serverInfo));
	Q_strncpyz(runInfo->coursename, Info_ValueForKey(serverInfo, "mapname"), sizeof(runInfo->coursename));

	runInfo->milliseconds = milliseconds;
	runInfo->startLessTime = client->pers.stats.startLessTime;
	runInfo->levelTimeStart = client->pers.stats.startLevelTime;
	runInfo->lostMsecCount = client->pers.raceDropped.msecTime;
	runInfo->lostPacketCount = client->pers.raceDropped.packetCount;
	runInfo->distance = client->pers.stats.distanceTraveled;
	runInfo->distanceXY = client->pers.stats.distanceTraveled2D;
	runInfo->average = runInfo->distanceXY / ((float)(milliseconds- runInfo->lostMsecCount)*0.001f);
	runInfo->topspeed = client->pers.stats.topSpeed;
	runInfo->savePosCount = client->pers.stats.saveposCount;
	runInfo->resposCount = client->pers.stats.resposCount;
	runInfo->rankLB = -1;
	runInfo->pbStatus = -1;
	runInfo->unixTimeStampShiftedBillionCount = UNIX_TIMESTAMP_SHIFT_BILLIONS; // how much is subtracted from UNIX_TIMESTAMP() in sql before returning the value so we never overflow even a few decades into the future
}

const char* DF_RacePrintAppendage(finishedRunInfo_t* runInfo) {
	return va(
		"%d " // runId
		"%d " // clientNum
		"%d " // userId
		"%d " // milliseconds
		"%d " // levelTimeStart
		"%d " // levelTimeEnd
		"%d " // endCommandTime
		"%d " // startLessTime
		"%d " // endLessTime
		"%d " // warningFlags
		"\"%f\" " // topspeed
		"\"%f\" " // average
		"\"%f\" " // distance
		"\"%f\" " // distanceXY

		"%d " // raceStyle.movementStyle
		"%d " // raceStyle.msec
		"%d " // raceStyle.jumpLevel
		"%d " // raceStyle.variant
		"%d " // raceStyle.runFlags

		"%d " // savePosCount
		"%d " // resposCount
		"%d " // lostMsecCount
		"%d " // lostPacketCount
		"%d " // placeHolder1
		"%d " // placeHolder2
		"%d " // placeHolder3
		"%d " // placeHolder4
		"%d " // placeHolder5
		"%d " // placeHolder6
		"%d " // placeHolder7
		"%d " // placeHolder8
		"%d " // isPB
		"%d " // placeHolder10
		"\"%s\" " // coursename[COURSENAME_MAX_LEN + 1]
		"\"%s\" " // username[USERNAME_MAX_LEN + 1]
		"%d " // unixTimeStampShifted
		"%d " // unixTimeStampShiftedBillionCount
		"%d " // lbType
		,runInfo->runId
		,runInfo->clientNum
		,runInfo->userId
		,runInfo->milliseconds
		,runInfo->levelTimeStart
		,runInfo->levelTimeEnd
		,runInfo->endCommandTime
		,runInfo->startLessTime
		,runInfo->endLessTime
		,runInfo->warningFlags
		,runInfo->topspeed
		,runInfo->average
		,runInfo->distance
		,runInfo->distanceXY

		,(int)runInfo->raceStyle.movementStyle
		,(int)runInfo->raceStyle.msec
		,(int)runInfo->raceStyle.jumpLevel
		,(int)runInfo->raceStyle.variant
		,(int)runInfo->raceStyle.runFlags

		,runInfo->savePosCount
		,runInfo->resposCount
		,runInfo->lostMsecCount
		,runInfo->lostPacketCount
		,runInfo->placeHolder1
		,runInfo->placeHolder2
		,runInfo->placeHolder3
		,runInfo->placeHolder4
		,runInfo->placeHolder5
		,runInfo->placeHolder6
		,runInfo->placeHolder7
		,runInfo->placeHolder8
		,runInfo->pbStatus
		,runInfo->rankLB
		,runInfo->coursename
		,runInfo->username
		,runInfo->unixTimeStampShifted
		,runInfo->unixTimeStampShiftedBillionCount
		,runInfo->lbType
		);
}

// Stop race timer
void DF_FinishTimer_Touch(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	gclient_t* cl;
	int	timeLast, timeBest,newRaceBestTime, lessTime = 0;
	char timeLastStr[32];// , timeBestStr[32];
	int warningFlags = 0;
	qboolean isInserting = qfalse;
	vec3_t interpolationDisplacement;
	static finishedRunInfo_t runInfo;
	
	// Check client
	if (!activator->client) return;

	cl = activator->client;


	if (!cl->sess.raceMode
		|| cl->ps.pm_type != PM_NORMAL
		|| cl->ps.stats[STAT_HEALTH] <= 0
		|| cl->sess.sessionTeam != TEAM_FREE
		//|| cl->ps.duelInProgress && !cl->sess.raceMode // irrelevant, we dont allow non-racemoders to run anyway
		//|| cl->ps.legsAnim == BOTH_JUMPATTACK6 // jka only thing?
		) {
		return;
	}

	if (cl->sess.raceStateInvalidated) {
		//trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

	// Check timer
	if (!cl->pers.raceStartCommandTime) return;

	if (!ValidRaceSettings(activator) || !trap_InPVS(cl->ps.origin, cl->ps.origin)) {// out of bounds fix? does this need extra checks due to trace/interpolation?
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Finish Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", cl->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid pre-pmove info.\n\"", cl->pers.netname));
		warningFlags |= DF_WARNING_INVALID_PREPMOVE;
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, ent, "df_trigger_finish", interpolationDisplacement);
	}

	cl->pers.stats.distanceTraveled -= VectorLength(interpolationDisplacement);
	interpolationDisplacement[2] = 0;
	cl->pers.stats.distanceTraveled2D -= VectorLength(interpolationDisplacement);

	// Set info
	timeLast = cl->ps.commandTime - lessTime - cl->pers.raceStartCommandTime;
	timeBest = !cl->pers.raceBestTime ? timeLast : cl->pers.raceBestTime;

	memset(&runInfo, 0, sizeof(runInfo));
	DF_FillClientRunInfo(&runInfo, activator, timeLast); // fills various stats we collected from start trigger and across run, and some metadata
	runInfo.runId = DF_GetNewRunId();
	runInfo.endLessTime = lessTime;
	runInfo.levelTimeEnd = level.time;
	runInfo.endCommandTime = cl->ps.commandTime - lessTime;
	runInfo.warningFlags = warningFlags;


	Q_strncpyz(timeLastStr, DF_MsToString(timeLast), sizeof(timeLastStr));
	//Q_strncpyz(timeBestStr, DF_MsToString(timeBest), sizeof(timeBestStr));

	if ((cl->sess.raceStyle.runFlags & RFL_SEGMENTED) && cl->pers.segmented.state != SEG_REPLAY) {
		//trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the segmented race in %f units [^2%s^7]: ^1Estimate! Starting rerun now.\n\" dfsegprelim %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, DF_RacePrintAppendage(&runInfo))); // extra params: type runId clientNum milliseconds leveltimeend endcommandtime endInterpolationReduction warningFlags top average distance username

		PrintRaceTime(&runInfo, qtrue, qfalse, activator);
		cl->pers.segmented.state = SEG_REPLAY;
		cl->pers.segmented.playbackStartedTime = level.time;
		cl->pers.segmented.playbackNextCmdIndex = 0;
		return;
	}

	//isInserting = G_InsertRun(activator, timeLast,0,0,0, warningFlags, level.time, runId, cl->ps.commandTime - lessTime);
	isInserting = G_InsertRun(&runInfo);


	PrintRaceTime(&runInfo, qfalse, qfalse, activator);
	// Show info
	/*if (timeLast == timeBest) {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in %f units in [^2%s^7]\n\" dffinish %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, DF_RacePrintAppendage(&runInfo)));
	}
	else if (timeLast < timeBest) {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in %f units in [^5%s^7] which is a new personal record!\n\" dffinish %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, DF_RacePrintAppendage(&runInfo)));
	}
	else {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in %f units in [^2%s^7] and his record was [^5%s^7]\n\" dffinish %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, timeBestStr, DF_RacePrintAppendage(&runInfo)));
	}*/

	// Play sound
	//if (timeLast < timeBest) G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass")); // TODO ok but lets precache it?

	// Show info
	//G_CenterPrint(activator - g_entities,3, "Race timer finished!",qfalse,qtrue,qfalse);

	// Update timers
	//cl->pers.raceLastTime = timeLast;
	if (RaceStyleIsMainLeaderboard(&runInfo.raceStyle,&level.mapDefaultRaceStyle)) {
		// todo load player's best time when logging in
		newRaceBestTime = timeLast > timeBest ? timeBest : timeLast;
		if (cl->pers.raceBestTime != newRaceBestTime) {
			cl->pers.raceBestTime = newRaceBestTime;
			// Update client
			ClientUserinfoChanged(activator - g_entities);
		}
	}

	// Reset timers
	//cl->ps.duelTime = 0;
	cl->pers.raceStartCommandTime = 0;
}

// Checkpoint race timer
void DF_CheckpointTimer_Touch(gentity_t* trigger, gentity_t* activator, trace_t* trace) // TODO Make this only trigger on first contact
{
	gclient_t* cl;
	vec3_t interpolationDisplacement;
	int	timeCheck, lessTime=0;
	int nowTime = LEVELTIME(activator->client);

	// Check client
	if (!activator->client) return;

	cl = activator->client;

	if (!cl->sess.raceMode || cl->ps.pm_type != PM_NORMAL || cl->ps.stats[STAT_HEALTH] <= 0) return;

	if (activator->client->sess.raceStateInvalidated) {
		//trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

	// Check timer
	if (!activator->client->pers.raceStartCommandTime) return;

	if (nowTime - activator->client->pers.raceLastCheckpointTime < 1000) return; // don't spam.

	// we ideally only wanna display checkpoints if the player didn't touch them last frame.
	// doesn't matter for finish triggers as much since they end runs the first time they are touched.
	if (nowTime - trigger->triggerLastPlayerContact[activator-g_entities] < 1000) return;

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Checkpoint Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid checkpoint pre-pmove info.\n\"", activator->client->pers.netname));
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, trigger, "df_trigger_checkpoint", interpolationDisplacement);
	}

	// Set info
	timeCheck = activator->client->ps.commandTime - lessTime - activator->client->pers.raceStartCommandTime;

	// Show info
	G_CenterPrint(activator - g_entities,3, va("Checkpoint!\n^3%s", DF_MsToString(timeCheck)),qfalse,qtrue,qfalse);
	activator->client->pers.raceLastCheckpointTime = nowTime;
}

void DF_target_husk(gentity_t* ent) {
	// do nothing. we just wanna be able to find it and replace it.
}


extern void InitTrigger(gentity_t* self);
void DF_trigger_start_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->r.contents |= CONTENTS_TRIGGER_EXIT;
	ent->leave = DF_StartTimer_Leave;
	ent->triggerOnlyTraced = qtrue; // don't trigger if we are fully inside trigger brush or if robust triggers are deactivated. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_finish_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_FinishTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_checkpoint_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_CheckpointTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_start(gentity_t* ent) {

	InitTrigger(ent);

	ent->r.contents |= CONTENTS_TRIGGER_EXIT;
	ent->leave = DF_StartTimer_Leave;
	ent->triggerOnlyTraced = qtrue; // don't trigger if we are fully inside trigger brush or if robust triggers are deactivated. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_finish(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_FinishTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_checkpoint(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_CheckpointTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	trap_LinkEntity(ent);
}

// q3 defrag targets are dependent on a separate trigger brush, which makes
// tracking accurate times more difficult. so we are going to remove the trigger brushes
// and the targets and replace them with proper defrag spawns.
//
//
// Basically, this function is fucking evil
void G_TurnDefragTargetsIntoTriggers() {
	gentity_t*	target;
	gentity_t*	trigger;
	qboolean	anyTriggerFound = qfalse;
	int i;
	char* oldModel;

	target = NULL;

	for (i = 0; i < TARGET_TYPE_COUNT; i++) {
		// Go through all target types

		// First find all relevant targets
		while ((target = G_Find(target, FOFS(classname), q3DefragTargetNames[i])) != NULL) {
			trigger = NULL;
			if (!target->targetname) {
				G_Printf("DEFRAG: ^1untargeted %s at %s\n", target->classname, vtos(target->s.origin));
				G_FreeEntity(target);
				continue;
			}

			// Then find all triggers that do something with them
			while ((trigger = G_Find(trigger, FOFS(target), target->targetname)) != NULL) {

				if (!trigger->r.bmodel || !(trigger->r.contents & CONTENTS_TRIGGER) || !trigger->model) {
					continue;
				}
				anyTriggerFound = qtrue;
				oldModel = trigger->model;
				G_FreeEntity(trigger);
				G_InitGentity(trigger); // Is this too disgusting and evil? xd. I wanna reuse this slot tho.
				trigger->model = oldModel; 
				switch (i) { // reusing japro classnames for compatibility
					case TARGET_STARTTIMER:
						trigger->classname = "df_trigger_start";
						DF_trigger_start_converted(trigger);
						break;
					case TARGET_STOPTIMER:
						trigger->classname = "df_trigger_finish";
						DF_trigger_finish_converted(trigger);
						break;
					default:
					case TARGET_CHECKPOINT:
						trigger->classname = "df_trigger_checkpoint";
						DF_trigger_checkpoint_converted(trigger);
						break;
				}
			}
			if (!anyTriggerFound) {
				G_Printf("DEFRAG: ^1untargeted %s at %s (targetname %s)\n", target->classname, vtos(target->s.origin),target->targetname);
			}
			G_FreeEntity(target);
		}
	}
}


void SetClientPhysicsFps(gentity_t* ent, int clientSetting);
void RemoveLaserTraps(gentity_t* ent);
void RemoveDetpacks(gentity_t* ent);
void DeletePlayerProjectiles(gentity_t* ent);
void Cmd_ForceChanged_f(gentity_t* ent);

void ResetPhysicsFpsStuff(gentity_t* ent) {

	if (!ent->client) return;
	ent->client->pers.physicsFps.acceptedSetting = 0;
	ent->client->pers.physicsFps.acceptedSettingMsec = 0;
	SetClientPhysicsFps(ent, ent->client->pers.physicsFps.clientSetting); // set it again
}

void DF_SetRaceMode(gentity_t* ent, qboolean value) {
	value = (qboolean)!!value;
	if (ent->client->sess.raceMode == value) return;
	ent->client->sess.raceMode = value;
	ent->s.weapon = WP_SABER; //Dont drop our weapon
	if(!value) Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S//Delete all their projectiles / saved stuff

	// reset physicsfps because racemode has different rules for validating that.
	ResetPhysicsFpsStuff(ent);

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		//Delete all their projectiles / saved stuff
		RemoveLaserTraps(ent);
		RemoveDetpacks(ent);
		DeletePlayerProjectiles(ent);


		G_Kill(ent); //stop abuse
		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->ps.persistant[PERS_KILLED] = 0;
		ent->client->accuracy_shots = 0;
		ent->client->accuracy_hits = 0;
		ent->client->ps.fd.suicides = 0;
		ent->client->pers.enterTime = level.time; //reset scoreboard kills/deaths i guess... and time?
	}
	UpdateClientRaceVars(ent->client);
}

// Adapted from jaPRO
void Cmd_Race_f(gentity_t* ent)
{
	if (!ent->client)
		return;

	if (ent->client->ps.powerups[PW_NEUTRALFLAG] || ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG]) {
		trap_SendServerCommand(ent-g_entities, "print \"^5This command is not allowed when carrying a flag!\n\"");
		return;
	}

	if (!g_defrag.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		DF_SetRaceMode(ent, qfalse);
		//ent->client->sess.raceMode = qfalse;
		//Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S
		return;
	}

	if (g_gametype.integer != GT_FFA) { // TA: What the heck is this?!
		if (g_gametype.integer >= GT_TEAM && g_defrag.integer)
		{//this is ok

			ent->s.weapon = WP_SABER; //Dont drop our weapon
			Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S

			ent->client->sess.raceMode = qfalse;//Set it false here cuz we are flipping it next // TA: (wut? oh.)
			if (ent->client->sess.sessionTeam != TEAM_FREE) {
				SetTeam(ent, "race");// , qfalse);
			}
			else {
				SetTeam(ent, "spec");// , qfalse);
			}
			//return;//duno..
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed in this gametype!\n\"");
			return;
		}
	}

	if (ent->client->sess.raceMode) {//Toggle it
		//ent->client->sess.raceMode = qfalse;
		//ent->s.weapon = WP_SABER; //Dont drop our weapon
		//Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S
		DF_SetRaceMode(ent, qfalse);
		G_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled off.\n\"",qtrue);
	}
	else {
		//ent->client->sess.raceMode = qtrue;
		DF_SetRaceMode(ent, qtrue);
		G_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled on.\n\"",qtrue);
	}


	// reset physicsfps because racemode has different rules for validating that.
	//ResetPhysicsFpsStuff(ent);

	//if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
	//	//Delete all their projectiles / saved stuff
	//	RemoveLaserTraps(ent);
	//	RemoveDetpacks(ent);
	//	DeletePlayerProjectiles(ent);


	//	G_Kill(ent); //stop abuse
	//	ent->client->ps.persistant[PERS_SCORE] = 0;
	//	ent->client->ps.persistant[PERS_KILLED] = 0;
	//	ent->client->accuracy_shots = 0;
	//	ent->client->accuracy_hits = 0;
	//	ent->client->ps.fd.suicides = 0;
	//	ent->client->pers.enterTime = level.time; //reset scoreboard kills/deaths i guess... and time?
	//}
}


static void ResetSpecificPlayerTimers(gentity_t* ent, qboolean print) {
	qboolean wasReset = qfalse;;

	if (!ent->client)
		return;
	if (ent->client->pers.raceStartCommandTime)// || ent->client->pers.stats.startTimeFlag)
		wasReset = qtrue;

	ent->client->sess.raceStateInvalidated = qtrue;

	if (ent->client->sess.raceMode) {
		//VectorClear(ent->client->ps.velocity);
		ent->client->ps.duelTime = 0;
		ent->client->ps.powerups[PW_YSALAMIRI] = 0; //beh, only in racemode so wont fuck with ppl using amtele as checkpoints midcourse
		ent->client->ps.powerups[PW_FORCE_BOON] = 0;
		if (ent->health > 0) {
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
			ent->client->ps.stats[STAT_ARMOR] = 25;
		}
		if (MovementStyleAllowsWeapons(ent->client->sess.raceStyle.movementStyle)) { //Get rid of their rockets when they tele/noclip..? Do this for every style..
			DeletePlayerProjectiles(ent);
		}

	}

	ent->client->pers.raceStartCommandTime = 0;
	ent->client->ps.fd.forceRageRecoveryTime = 0; 
	
	// not like we really need to do this since it happens in start anyway
	memset(&ent->client->pers.raceDropped, 0, sizeof(ent->client->pers.raceDropped));
	memset(&ent->client->pers.stats, 0, sizeof(ent->client->pers.stats));

	if (wasReset && print)
		G_CenterPrint(ent - g_entities,3, "Timer reset!",qfalse,qtrue,qfalse);
		//G_CenterPrint(ent - g_entities,3, "Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n",qfalse,qtrue,qfalse);
}

void DF_ResetSegmentedRun(gentity_t* ent) {
	ent->client->pers.segmented.state = SEG_DISABLED;
	trap_G_COOL_API_PlayerUserCmdClear(ent - g_entities); 
#ifdef SEGMENTEDDEBUG
	memset(ent->client->pers.segmented.debugTime, 0, sizeof(ent->client->pers.segmented.debugTime));
#endif
}

void DF_SegmentedRunStatusInvalidated(gentity_t* ent) {
	if (!ent->client->sess.raceMode || !(ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED)) {
		return;
	}
	if (ent->client->pers.segmented.state < SEG_RECORDING_HAVELASTPOS || ent->client->pers.segmented.state >= SEG_REPLAY) { // replay can happen even if we dont have lastpos
		DF_RaceStateInvalidated(ent,qtrue);
	}
	else {
		ent->client->pers.segmented.state = SEG_RECORDING_INVALIDATED; // can only respos, not savepos.
	}
}

void DF_RaceStateInvalidated(gentity_t* ent, qboolean print)
{
	ResetSpecificPlayerTimers(ent, print);
	DF_ResetSegmentedRun(ent);
	ent->client->ps.fd.forcePower = 100; //Reset their force back to full i guess!
	ent->client->pers.lastRaceResetTime = level.time;
}

static qboolean MovementStyleAllowsJumpChange(int movementStyle) {
	return qtrue;
}

void Cmd_JumpChange_f(gentity_t* ent)
{
	char jLevel[32];
	int levelint;

	if (!ent->client)
		return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (ent->client->pers.raceStartCommandTime) {
		Com_Printf("^7Cannot change jump settings during a run.");
		return;
	}

	if (trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /jump <level>\n\"");
		return;
	}

	if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE || VectorLength(ent->client->ps.velocity)) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be standing still to use this command!\n\"");
		return;
	}

	if (!MovementStyleAllowsJumpChange(ent->client->sess.raceStyle.movementStyle)) {
		//char styleString[16];
		//IntegerToRaceName(ent->client->sess.movementStyle, styleString, sizeof(styleString));
		//trap_SendServerCommand(ent - g_entities, va("print \"This command is not allowed with your movement style (%s)!\n\"", styleString));
		trap_SendServerCommand(ent - g_entities, va("print \"This command is not allowed with your movement style (%d)!\n\"", ent->client->sess.raceStyle.movementStyle));
		return;
	}

	trap_Argv(1, jLevel, sizeof(jLevel));
	levelint = atoi(jLevel);

	if (levelint >= -1 && levelint <= 3) {
		ent->client->sess.raceStyle.jumpLevel = levelint;
		ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = MAX(0,ent->client->sess.raceStyle.jumpLevel);
		if (ent->client->sess.raceStyle.jumpLevel == -1) {
			ent->client->ps.powerups[PW_YSALAMIRI] = INT_MAX;
		}
		else {
			ent->client->ps.powerups[PW_YSALAMIRI] = 0;
		}
		DF_RaceStateInvalidated(ent, qtrue);
		//DF_InvalidateSpawn(ent);
		if (ent->client->pers.raceStartCommandTime) {
			G_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i): timer reset.\n\"", levelint),qtrue);
		}
		else
			G_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i).\n\"", levelint),qtrue);
	}
	else
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /jump <level>\n\"");
}




/*
==============================
saved - used to hold ownerNums
==============================
*/
static int saved[MAX_GENTITIES];


static qboolean ShouldNotCollide(gentity_t* entity, gentity_t* other)
{
	// since we are in a duel, make everyone else nonsolid
	if (entity->client && entity->client->ps.duelInProgress) {
			if (entity != other && (other-g_entities) != entity->client->ps.duelIndex) {
				if ((other->inuse) &&
					((other->s.eType == ET_PLAYER) ||
						((other->s.eType == ET_GENERAL) &&
							((qtrue) &&
								(!(Q_stricmp(other->classname, "laserTrap")) ||
									(!(Q_stricmp(other->classname, "detpack"))))))))
				{
					return qtrue;
				}
			}
	}
	else if (entity->client && entity->client->sess.raceMode) { //Have to check all entities because swoops can be racemode too :/
			if (other != entity) {
				if ((other->inuse) &&
					((other->s.eType == ET_PLAYER) ||
					// im not sure yet. do i want doors to not be a thing for racers? limits the map choice a little bit.
					//	((other->s.eType == ET_MOVER) &&
					//		(!(Q_stricmp(other->classname, "func_door")) ||
					//			(!(Q_stricmp(other->classname, "func_plat"))))) ||
						((other->s.eType == ET_GENERAL) &&
							(!(Q_stricmp(other->classname, "laserTrap")) || // TODO sth more efficient than string comparison?
								(!(Q_stricmp(other->classname, "detpack")))))))
				{
					return qtrue;
				}
			}
	}
	else { // we are not dueling but make those that are nonsolid
		if (entity->inuse) {//Saber
			const int saberOwner = entity->r.ownerNum;//Saberowner
			if (g_entities[saberOwner].client && g_entities[saberOwner].client->ps.duelInProgress) {
				return qfalse;
			}
		}
		//loda fixme? This should go through all entities... to also account for people lightsabers..? or is that too costly
		if (other != entity) {
			if (other->inuse && other->client &&
				(other->client->ps.duelInProgress || other->client->sess.raceMode)) { //loda fixme? Or the ent is a saber, and its owner is in racemode or duel in progress

				return qtrue; // uh so for example func_bobbing cannot touch us, but we can touch it? is that ok?
			}
		}
	}
	return qfalse;
}

static void JP_TraceReal(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean precise) {
	
	if (precise && (coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
		trap_G_COOL_API_NonEpsilonTrace(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
	else {
		trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
	if (results->entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t* passEnt = g_entities + passEntityNum;
		gentity_t* ent = g_entities + results->entityNum;

		if (ShouldNotCollide(passEnt, ent))
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			if (precise && (coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
				trap_G_COOL_API_NonEpsilonTrace(results, start, mins, maxs, end, passEntityNum, contentmask);
			}
			else {
				JP_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
			}
			ent->r.contents = contents;

			return;
		}
	}

	if (results->startsolid && start != end)
	{
		trace_t tw;

		JP_Trace(&tw, start, mins, maxs, start, passEntityNum, contentmask);
		results->startsolid = tw.startsolid;
	}
}

void JP_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qfalse);
}
// don't use this for movement and normal stuff. 
// normal trace applies an epsilon (0.125f offset) to avoid some fuckery with vector snapping over network and i dont even know,
// but the result is that trace fractions are reduced, to where they hit something that trap_EntityContact does NOT hit at the end position
// so for example with trigger detection, that makes our trigger interpolation less precise. 
// hence, for trigger tracing, use this.
void JP_TracePrecise(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qtrue);
}

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
		return;
	}

	fromTime = MOVERTIME_ENT(gent);
	toTime = level.time;
	if (fromTime == toTime) {
		VectorCopy(in, out);
		return;
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


typedef struct playerSnapshotBackupValues_s {
	int solidValue;
	int saberMove;
	int saberMovePS;
	int pmfFollowPS;
	//int event;
	//int	trTime;
	vec3_t	psMoverOldPos;
} playerSnapshotBackupValues_t;

//static int solidValues[MAX_GENTITIES];
//static int saberMoveValues[MAX_GENTITIES];
//static int saberMoveValuesPS[MAX_GENTITIES];
//static int pmfFollowPS[MAX_GENTITIES];
static playerSnapshotBackupValues_t backupValues[MAX_GENTITIES];
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
	int i;
	for (i = 0; i < level.num_entities; i++, backup++, mvEnt++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		es = &other->s;
		if (saveState) {
			backup->solidValue = es->solid;
			//backup->event = es->event;
			//if (es->eType == ET_MOVER) { // hackily "fix" client-timed mover prediction for cgame
				//backup->trTime = es->pos.trTime;
				//es->pos.trTime += level.time - ACTIVATORTIME(other->activatorReal);
			//}
		}
		if (ShouldNotCollide(ent,other)) {
			es->solid = 0;
		}
		else if (!saveState){
			es->solid = backup->solidValue;
		}

		if (es->eType == ET_BEAM && other->parent != ent && es->generic1 == 3) {
			//mvEnt->snapshotIgnore[clientNum] = cl->sess.solo || cl->sess.hideLasers || (cl->sess.ignore & (1 << es->owner));
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = (cl->sess.solo && other->parent != followedEnt) || cl->sess.hideLasers || (cl->sess.ignore & (1 << es->owner)); // if engine suppoorts it, respect wishes of spectator instead of client that's being followed
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = other->parent != followedEnt && (followedClient->sess.solo || followedClient->sess.hideLasers || (followedClient->sess.ignore & (1 << es->owner))); // snapshot of the follower might happen before the client himself, and snapshotIgnore is based on clientnum in ps
			}
			//if (ent->client->sess.hideLasers) {
			//	es->event = 0;
			//}
			//else if (!saveState) {
			//	es->event = backup->event;
			//}
		}

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

			//mvEnt->snapshotIgnore[clientNum] = /*(cl->sess.ignore & (1 << i)) ||*/ cl->sess.solo;
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = /*(cl->sess.ignore & (1 << i)) ||*/ cl->sess.solo;
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = /*(cl->sess.ignore & (1 << i)) ||*/ followedClient->sess.solo;
			}
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
	if (saveState) {
		// only do this once, its not client-specific
		for (i = 0; i < MAX_CLIENTS; i++) {
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
}
void PlayerSnapshotRestoreValues() {
	gentity_t* other;
	gclient_t* cl;
	entityState_t* es;
	playerSnapshotBackupValues_t* backup = backupValues;
	int i;
	for (i = 0; i < level.num_entities; i++, backup++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		es = &other->s;
		es->solid = backup->solidValue;
		es->saberMove = backup->saberMove; 
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

void DF_SetMapDefaults(raceStyle_t rs) {
	int i;
	gentity_t* client;
	int oldMsec;
	// TODO we wanna update the individual settings of players IF their old defaults were equal to the old map default?
	// but kind of a pita. just apply for now.
	for (i = 0; i < MAX_CLIENTS; i++) {
		client = (g_entities + i);
		if (!client->client) continue;
		DF_CarryClientOverToNewRaceStyle(client,&rs);
		//oldMsec = client->client->sess.raceStyle.msec;
		//client->client->sess.raceStyle = rs;
		//client->client->sess.raceStyle.msec = oldMsec;

		//client->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
	}
	//if (level.mapDefaultRaceStyle.jumpLevel != rs.jumpLevel) {

	//}

	level.mapDefaultRaceStyle = rs;
	trap_Cvar_Set("g_mapDefaultMsec", va("%d", level.mapDefaultRaceStyle.msec));
	trap_Cvar_Set("g_mapDefaultJump", va("%d", level.mapDefaultRaceStyle.jumpLevel));
	trap_Cvar_Set("g_mapDefaultRunFlags", va("%d", level.mapDefaultRaceStyle.runFlags));
	//level.mapDefaultsConfirmed = qtrue;
}

void DF_LoadMapDefaults() {
	insertUpdateMapRaceDefaultsStruct_t	data;
	memset(&data, 0, sizeof(data));
	Q_strncpyz(data.course, DF_GetCourseName(), sizeof(data.course));

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_LOADMAPRACEDEFAULTS,
		"SELECT msec,jump,variant,runFlags FROM mapdefaults WHERE course=?"
	)) {
		trap_SendServerCommand(-1, "print \"^1Map defaults could not be loaded. Leaderboard may not display correctly.\n\"");
		level.mapDefaultsLoadFailed = qtrue;
		return;
	}
	G_COOL_API_DB_PreparedBindString(data.course);

	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

void Cmd_DF_MapDefaults_f(gentity_t* ent)
{
	insertUpdateMapRaceDefaultsStruct_t	data;
	raceStyle_t rs = level.mapDefaultRaceStyle;
	if (!ent->client) return;


	if (trap_Argc() > 2) {
		char arg1[12] = { 0 };


		if (!(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_CHANGEMAPDEFAULTRACESTYLE)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1You don't have permission to change map racestyle defaults.\n\"");
			return;
		}

		trap_Argv(1, arg1, sizeof(arg1));

		memset(&data, 0, sizeof(data));
		data.clientnum = ent - g_entities;
		memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
		Q_strncpyz(data.course, DF_GetCourseName(), sizeof(data.course));
			
		if (!Q_stricmp("run", arg1)) {

			char arg2[8] = { 0 };
			int index, index2, flag;
			const uint32_t mask = allowedMapDefaultRunFlags & ((1 << MAX_RUN_FLAGS) - 1);

			trap_Argv(2, arg2, sizeof(arg2));
			index = atoi(arg2);
			index2 = index;
			flag = 1 << index;

			//if (index2 < 0 || index2 >= MAX_RUN_FLAGS) {
			if (~allowedMapDefaultRunFlags & flag) {
				trap_SendServerCommand(ent - g_entities, va("print \"Run flags: Invalid flag: %i [0, %i]\n\"", index2, MAX_RUN_FLAGS - 1));
				return;
			}

			// segmented is never a map default.
			/*if (flag & RFL_SEGMENTED) {

				if (level.nonDeterministicEntities) {
					trap_SendServerCommand(ent - g_entities, va("print \"Warning: Map contains %i potentially non-deterministic entities. Segmented runs may not replay correctly and thus not count.\n\"", level.nonDeterministicEntities));
				}

				if (!(coolApi & COOL_APIFEATURE_G_USERCMDSTORE)) {
					trap_SendServerCommand(ent - g_entities, va("print \"Error: Segmented runs are only available with the UserCmdStore coolAPI feature. Please use the appropriate server engine.\n\"", index2, MAX_RUN_FLAGS - 1));
					return;
				}
				if (jk2version != VERSION_1_04) {
					// TODO is this still true?
					// We need the JK2MV 1.04 API because we need to send playerstates from game to engine and MV playerstate conversion would mess us up.
					trap_SendServerCommand(ent - g_entities, va("print \"Error: Segmented runs are only available with 1.04 API (this does not mean they don't work in 1.02, it's a code thing).\n\"", index2, MAX_RUN_FLAGS - 1));
					return;
				}
			}*/

			{
				rs.runFlags = flag ^ ((int)rs.runFlags & mask);
			}

			trap_SendServerCommand(ent - g_entities, va("print \"^7Map defaults: %s %s^7\n\"", runFlagsNames[index2].string, ((ent->client->sess.raceStyle.runFlags & flag)
				? "^2Enabled" : "^1Disabled")));

			Q_strncpyz(data.what, "Run flags", sizeof(data.what));

			G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS,
				"INSERT INTO mapdefaults (course,msec,jump,variant,runFlags) VALUES (?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"runFlags=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindInt(rs.msec);
			G_COOL_API_DB_PreparedBindInt(rs.jumpLevel);
			G_COOL_API_DB_PreparedBindInt(rs.variant);
			G_COOL_API_DB_PreparedBindInt(rs.runFlags);

			G_COOL_API_DB_PreparedBindInt(rs.runFlags);

			G_COOL_API_DB_FinishAndSendPreparedStatement();
		} else if (!Q_stricmp("jump", arg1)) {

			char arg2[8] = { 0 };
			int newjump;

			trap_Argv(2, arg2, sizeof(arg2));
			newjump = atoi(arg2);

			if (newjump < -1 || newjump > 3) {
				trap_SendServerCommand(ent - g_entities, va("print \"Jump level %d is not valid. Range is -1(ysal) to 3\n\"", newjump));
				return;
			}

			if(rs.jumpLevel == newjump){
				trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated to %d. No change.\n\"", newjump));
				return;
			}
			rs.jumpLevel = newjump;

			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated to %d.\n\"", newjump));

			Q_strncpyz(data.what, "Jump level", sizeof(data.what));

			G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS,
				"INSERT INTO mapdefaults (course,msec,jump,variant,runFlags) VALUES (?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"jump=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindInt(rs.msec);
			G_COOL_API_DB_PreparedBindInt(rs.jumpLevel);
			G_COOL_API_DB_PreparedBindInt(rs.variant);
			G_COOL_API_DB_PreparedBindInt(rs.runFlags);

			G_COOL_API_DB_PreparedBindInt(rs.jumpLevel);

			G_COOL_API_DB_FinishAndSendPreparedStatement();
		} else if (!Q_stricmp("variant", arg1)) {

			char arg2[8] = { 0 };
			int newvariant;

			trap_Argv(2, arg2, sizeof(arg2));
			newvariant = atoi(arg2);

			if (newvariant != 0) { // topdo check if variant exists?
				trap_SendServerCommand(ent - g_entities, va("print \"Variant %d is not valid. Range is 0 right now.\"", newvariant));
				return;
			}

			if(rs.variant == newvariant){
				trap_SendServerCommand(ent - g_entities, va("print \"Variant updated to %d. No change.\n\"", newvariant));
				return;
			}
			rs.variant = newvariant;

			trap_SendServerCommand(ent - g_entities, va("print \"Variant updated to %d.\n\"", newvariant));

			Q_strncpyz(data.what, "Variant", sizeof(data.what));

			G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS,
				"INSERT INTO mapdefaults (course,msec,jump,variant,runFlags) VALUES (?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"variant=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindInt(rs.msec);
			G_COOL_API_DB_PreparedBindInt(rs.jumpLevel);
			G_COOL_API_DB_PreparedBindInt(rs.variant);
			G_COOL_API_DB_PreparedBindInt(rs.runFlags);

			G_COOL_API_DB_PreparedBindInt(rs.variant);

			G_COOL_API_DB_FinishAndSendPreparedStatement();
		} else if (!Q_stricmp("fps", arg1)) {

			char arg2[8] = { 0 };
			int newfps,newmsec;

			trap_Argv(2, arg2, sizeof(arg2));
			if (!Q_stricmp(arg2, "float")) {
				newfps = -2;
			}
			else {
				newfps = atoi(arg2);
			}

			if (newfps < 1 && newfps != -2 || newfps > 1000) { // topdo check if variant exists?
				trap_SendServerCommand(ent - g_entities, va("print \"Fps %d is not valid. Range is 1 to 1000, or 'float' (you can write -2 instead).\"", newfps));
				return;
			}

			newmsec = newfps == -2 ? -2 : (1000 / newfps);

			if(rs.msec == newmsec){
				trap_SendServerCommand(ent - g_entities, va("print \"Msec updated to %d. No change.\n\"", newmsec));
				return;
			}
			rs.msec = newmsec;

			trap_SendServerCommand(ent - g_entities, va("print \"Msec updated to %d.\n\"", newmsec));

			Q_strncpyz(data.what, "Msec", sizeof(data.what));

			G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS,
				"INSERT INTO mapdefaults (course,msec,jump,variant,runFlags) VALUES (?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"msec=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindInt(rs.msec);
			G_COOL_API_DB_PreparedBindInt(rs.jumpLevel);
			G_COOL_API_DB_PreparedBindInt(rs.variant);
			G_COOL_API_DB_PreparedBindInt(rs.runFlags);

			G_COOL_API_DB_PreparedBindInt(rs.msec);

			G_COOL_API_DB_FinishAndSendPreparedStatement();
		}

		DF_SetMapDefaults(rs);
	}

	{
		qboolean isSet;
		qboolean differentFromDefault;
		int i = 0;
		int differences = defaultRaceStyle.runFlags ^ level.mapDefaultRaceStyle.runFlags;
		for (i = 0; i < MAX_RUN_FLAGS; i++) {
			if (!(allowedMapDefaultRunFlags & (1 << i))) continue;
			isSet = level.mapDefaultRaceStyle.runFlags & (1 << i);
			differentFromDefault = differences & (1 << i);
			trap_SendServerCommand(ent - g_entities, va("print \"%2d ^%d[%s] ^7%s\n\"", i, differentFromDefault ? 1 : 7, isSet ? "X" : " ", runFlagsNames[i].string));
		}
		if (differences) {
			trap_SendServerCommand(ent - g_entities, "print \"Differences from default are marked ^1red^7 for convenience.\n\"");
		}
	}
}

void Cmd_DF_RunSettings_f(gentity_t* ent)
{
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (trap_Argc() > 1) {

		char arg[8] = { 0 };
		int index, index2, flag;
		const uint32_t mask = allowedRunFlags & ((1 << MAX_RUN_FLAGS) - 1);

		if (ent->client->pers.raceStartCommandTime) {
			G_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"",qtrue);
			return;
		}

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;
		flag = 1 << index;

		//if (index2 < 0 || index2 >= MAX_RUN_FLAGS) {
		if (~allowedRunFlags & flag) {
			G_SendServerCommand(ent - g_entities, va("print \"Run flags: Invalid flag: %i [0, %i]\n\"", index2, MAX_RUN_FLAGS - 1),qtrue);
			return;
		}

		if (flag & RFL_SEGMENTED) {

			if (level.nonDeterministicEntities) {
				G_SendServerCommand(ent - g_entities, va("print \"Warning: Map contains %i potentially non-deterministic entities. Segmented runs may not replay correctly and thus not count.\n\"", level.nonDeterministicEntities),qtrue);
			}

			if (!(coolApi & COOL_APIFEATURE_G_USERCMDSTORE)) {
				G_SendServerCommand(ent - g_entities, va("print \"Error: Segmented runs are only available with the UserCmdStore coolAPI feature. Please use the appropriate server engine.\n\"", index2, MAX_RUN_FLAGS - 1),qtrue);
				return;
			}
			if (jk2version != VERSION_1_04) {
				// TODO is this still true?
				// We need the JK2MV 1.04 API because we need to send playerstates from game to engine and MV playerstate conversion would mess us up.
				G_SendServerCommand(ent - g_entities, va("print \"Error: Segmented runs are only available with 1.04 API (this does not mean they don't work in 1.02, it's a code thing).\n\"", index2, MAX_RUN_FLAGS - 1),qtrue);
				return;
			}
		}

		//if (index == 8 || index == 9) { //Radio button these options
		////Toggle index, and make sure everything else in this group (8,9) is turned off
		//	int groupMask = (1 << 8) + (1 << 9);
		//	int value = ent->client->sess.raceStyle.runFlags;

		//	groupMask &= ~(1 << index); //Remove index from groupmask
		//	value &= ~(groupMask); //Turn groupmask off
		//	value ^= (1 << index); //Toggle index item

		//	ent->client->sess.raceStyle.runFlags = value;
		//}
		//else 
		{
			ent->client->sess.raceStyle.runFlags = flag ^ ((int)ent->client->sess.raceStyle.runFlags & mask);
			ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
			DF_RaceStateInvalidated(ent,qtrue);
			//DF_InvalidateSpawn(ent);
		}

		G_SendServerCommand(ent - g_entities, va("print \"^7%s %s^7\n\"", runFlagsNames[index2].string, ((ent->client->sess.raceStyle.runFlags & flag)
			? "^2Enabled" : "^1Disabled")),qtrue);
	}

	{
		qboolean isSet;
		qboolean differentFromDefault;
		int i = 0;
		int differences = ent->client->sess.raceStyle.runFlags ^ level.mapDefaultRaceStyle.runFlags;
		for (i = 0; i < MAX_RUN_FLAGS; i++) {
			if (!(allowedRunFlags & (1 << i))) continue;
			isSet = ent->client->sess.raceStyle.runFlags & (1 << i);
			differentFromDefault = differences & (1 << i);
			trap_SendServerCommand(ent - g_entities, va("print \"%2d ^%d[%s] ^7%s\n\"", i, differentFromDefault ? 1 : 7, isSet ? "X" : " ", runFlagsNames[i].string));
		}
		if (differences) {
			trap_SendServerCommand(ent - g_entities, "print \"Differences from map default are marked ^1red^7. Your runs will not be on the main leaderboard with non-default settings.\n\"");
		}
	}
}

void UpdateClientRaceVars(gclient_t* client) {
	
	if (client->sess.raceMode) { // what happens when switching out of racemode? dont care rn TODO
		client->ps.fd.forcePowerLevel[FP_LEVITATION] = MAX(0,client->sess.raceStyle.jumpLevel);
		if (client->sess.raceStyle.jumpLevel == -1) {
			client->ps.powerups[PW_YSALAMIRI] = INT_MAX;
		}
		else {
			client->ps.powerups[PW_YSALAMIRI] = 0;
		}
	}
	client->ps.stats[STAT_MOVEMENTSTYLE] = client->sess.raceStyle.movementStyle;
	client->ps.stats[STAT_RUNFLAGS] = client->sess.raceStyle.runFlags;
	client->ps.stats[STAT_RACEMODE] = client->sess.raceMode; // can get lost sometimes after death? idk happened once but i had another bug then
	if (client->sess.raceMode) {
		client->ps.stats[STAT_MSECRESTRICT] = client->sess.raceStyle.msec; 
	}
	else if(g_fpsToggleDelay.integer) {
		client->ps.stats[STAT_MSECRESTRICT] = client->pers.physicsFps.acceptedSettingMsec;
	}
	else {
		client->ps.stats[STAT_MSECRESTRICT] = 0;
	}
}

void Cmd_ToggleFPS_f(gentity_t* ent)
{
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (ent->client->pers.raceStartCommandTime) {
		G_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"",qtrue);
		return;
	}

	if (ent->client->sess.raceStyle.msec != -1) {
		ent->client->sess.raceStyle.msec = -1;
		G_SendServerCommand(ent - g_entities, "print \"^7Toggle mode activated.\n\"",qtrue);
	}
	else {
		ent->client->sess.raceStyle.msec = 0;
		G_SendServerCommand(ent - g_entities, "print \"^7Toggle mode disabled.\n\"",qtrue);
	}

	ResetPhysicsFpsStuff(ent);

	DF_RaceStateInvalidated(ent, qtrue);
}

void Cmd_FloatPhysics_f(gentity_t* ent)
{
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (ent->client->pers.raceStartCommandTime) {
		G_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"",qtrue);
		return;
	}

	if (ent->client->sess.raceStyle.msec != -2) {
		ent->client->sess.raceStyle.msec = -2;
		G_SendServerCommand(ent - g_entities, "print \"^7Float physics mode activated.\n\"",qtrue);
	}
	else {
		ent->client->sess.raceStyle.msec = 0;
		G_SendServerCommand(ent - g_entities, "print \"^7Float physics mode disabled.\n\"",qtrue);
	}
	ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;

	ResetPhysicsFpsStuff(ent);

	DF_RaceStateInvalidated(ent, qtrue);
}



qboolean DF_ClientInSegmentedRunMode(gclient_t* client) {
	return (qboolean)(client->sess.raceMode && (client->sess.raceStyle.runFlags & RFL_SEGMENTED));
}

static vec3_t dfOldDelta;
void DF_PreDeltaAngleChange(gclient_t* client) {
	VectorCopy(client->ps.delta_angles, dfOldDelta);
}

void DF_PostDeltaAngleChange(gclient_t* client) {
	//qboolean isinSeg;
	if (client->ps.delta_angles[0] == dfOldDelta[0] && client->ps.delta_angles[1] == dfOldDelta[1] && client->ps.delta_angles[2] == dfOldDelta[2]) {
		return;
	}
	if (!DF_ClientInSegmentedRunMode(client) || client->pers.segmented.state == SEG_DISABLED || client->pers.segmented.state == SEG_REPLAY) {
		return;
	}
	else {
		vec3_t diff2;
		VectorSubtract(client->ps.delta_angles, dfOldDelta, diff2);
		VectorAdd(client->pers.segmented.anglesDiffAccum, diff2, client->pers.segmented.anglesDiffAccum);
		client->pers.segmented.anglesDiffAccum[0] &= 65535;
		client->pers.segmented.anglesDiffAccum[1] &= 65535;
		client->pers.segmented.anglesDiffAccum[2] &= 65535;
	}
}


qboolean SavePosition(gentity_t* client, savedPosition_t* savedPosition) {
	if (!client->client) return qfalse;
	memset(savedPosition, 0, sizeof(savedPosition_t));
	savedPosition->ps = client->client->ps;
	savedPosition->raceStyle = client->client->sess.raceStyle;
	savedPosition->raceStartCommandTime = (client->client->sess.raceStyle.runFlags & RFL_SEGMENTED) ? client->client->pers.raceStartCommandTime : 0;

#define FIELDSFUNC(a) savedPosition->client.a=client->client->a; // lord have mercy
	FIELDSCLIENT()
#undef FIELDSFUNC

#define FIELDSFUNC(a) VectorCopy(client->client->a, savedPosition->client.a); // lord have mercy
	FIELDSCLIENTVEC3()
#undef FIELDSFUNC

#define FIELDSFUNC(a) savedPosition->a=client->a; // lord have mercy
	FIELDSENT()
#undef FIELDSFUNC

#define FIELDSFUNC(a) VectorCopy(client->a, savedPosition->a); // lord have mercy
	FIELDSENTVEC3()
#undef FIELDSFUNC

	// to keep somewhat consistent trigger_multiple and such behavior. kinda disgusting and it wont restore any sort of
	// changed state from trigger_multiple triggering other stuff, making movers move or whichever.
	memcpy(savedPosition->client.triggerTimes,client->client->triggerTimes,sizeof(savedPosition->client.triggerTimes));
	memcpy(savedPosition->client.entityStates,client->client->entityStates,sizeof(savedPosition->client.entityStates));


	return qtrue;

	// its after the return so it will never be reached but its still a nice check for the compiler.
#if 1 // use this when you add new vars to check if we are copying to the right types. qvm will refuse to compile if there are mismatches
		if (1) {
#define FIELDSFUNC(a) VALIDATEPTRCMP(&savedPosition->client.a,&client->client->a); // lord have mercy
			FIELDSCLIENT()
			FIELDSCLIENTVEC3()
#undef FIELDSFUNC
#define FIELDSFUNC(a) VALIDATEPTRCMP(&savedPosition->a,&client->a); // lord have mercy
			FIELDSENT()
			FIELDSENTVEC3()
#undef FIELDSFUNC
				
		}
#endif
}

void RestorePosition(gentity_t* client, savedPosition_t* savedPosition, veci_t* diffAccum) {
	// TODO check clientspawn and clientbegin for any clues on what else to do?
	playerState_t backupPS;
	int delta;
	vec3_t oldDelta, diff2;
	int i;
	int* intPtr;
	float* floatPtr;
	playerState_t* storedPS = &savedPosition->ps;
	if (!client->client) return;

	backupPS = client->client->ps;
	client->client->ps = *storedPS;


#define FIELDSFUNC(a) client->client->a=savedPosition->client.a; // lord have mercy
	FIELDSCLIENT()
#undef FIELDSFUNC

#define FIELDSFUNC(a) VectorCopy( savedPosition->client.a,client->client->a); // lord have mercy
		FIELDSCLIENTVEC3()
#undef FIELDSFUNC

#define FIELDSFUNC(a) client->a=savedPosition->a; // lord have mercy
		FIELDSENT()
#undef FIELDSFUNC

#define FIELDSFUNC(a) VectorCopy(savedPosition->a,client->a); // lord have mercy
		FIELDSENTVEC3()
#undef FIELDSFUNC

	// to keep somewhat consistent trigger_multiple and such behavior. kinda disgusting and it wont restore any sort of
	// changed state from trigger_multiple triggering other stuff, making movers move or whichever.
	memcpy(client->client->triggerTimes, savedPosition->client.triggerTimes,sizeof(client->client->triggerTimes));
	memcpy(client->client->entityStates, savedPosition->client.entityStates,sizeof(client->client->entityStates));

	// make sure there's no weirdness
	client->client->ps.eFlags = (client->client->ps.eFlags & ~EF_TELEPORT_BIT) | ((backupPS.eFlags & EF_TELEPORT_BIT) ^ EF_TELEPORT_BIT); // Make it teleport
	
	// actually, turns out we can't do this because some places in the code actuaally dont check for ANIM_TOGGLEBIT
	// so to stay consistent we must simply keep things as they were
	//client->client->ps.torsoAnim = (client->client->ps.torsoAnim & ~ANIM_TOGGLEBIT) | ((backupPS.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT); // Restart animation if needed
	//client->client->ps.legsAnim = (client->client->ps.legsAnim & ~ANIM_TOGGLEBIT) | ((backupPS.legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT); // Restart animation if needed
	
	client->client->ps.externalEvent = (client->client->ps.externalEvent & ~EV_EVENT_BITS) | ((backupPS.externalEvent & EV_EVENT_BITS)); // Don't execute new events
	client->client->ps.eventSequence = backupPS.eventSequence; // Don't execute new events

	// retime
	delta = backupPS.commandTime - storedPS->commandTime;
	client->client->ps.commandTime = backupPS.commandTime;
	client->client->ps.saberEntityNum = backupPS.saberEntityNum; // yea... better this way:)
	if (storedPS->genericEnemyIndex >= 1024) client->client->ps.genericEnemyIndex += delta;
	if (storedPS->rocketLockTime > 0) client->client->ps.rocketLockTime += delta;

#define FIELDSFUNC(a) if (client->a > 0) { client->a += delta; }
	TIMECOMPENSATEFIELDS()
#undef FIELDSFUNC

	for (i = 0; i < MAX_GENTITIES; i++) {
		// pretty disgusting to have to loop through so many numbers for this, and it not even being a proper fix
		if (client->client->triggerTimes[i]) client->client->triggerTimes[i] += delta;
	}

	for (i = 0; i < MAX_POWERUPS; i++) {
		if (client->client->ps.powerups[i]) client->client->ps.powerups[i] += delta;
	}
	for (i = 0; i < NUM_FORCE_POWERS; i++) {
		if (client->client->ps.fd.forcePowerDebounce[i]) client->client->ps.fd.forcePowerDebounce[i] += delta;
		if (client->client->ps.fd.forcePowerDuration[i]) client->client->ps.fd.forcePowerDuration[i] += delta;
	}


	if (client->client->pers.segmented.state != SEG_REPLAY && (client->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && client->client->sess.raceMode && client->client->pers.raceStartCommandTime && savedPosition->raceStartCommandTime) {
		client->client->pers.raceStartCommandTime = client->client->ps.commandTime - (storedPS->commandTime- savedPosition->raceStartCommandTime);
	}

	client->client->ps.persistant[PERS_SPAWN_COUNT] = backupPS.persistant[PERS_SPAWN_COUNT];

	client->health = storedPS->stats[STAT_HEALTH];
	client->client->buttons = savedPosition->client.buttons;
	client->client->oldbuttons = savedPosition->client.oldbuttons;
	client->client->latched_buttons = savedPosition->client.latched_buttons;

	if (diffAccum) {
		VectorCopy(backupPS.delta_angles, oldDelta);
	}

	SetClientViewAngle(client,storedPS->viewangles);

	if(diffAccum) {
		VectorSubtract(client->client->ps.delta_angles, oldDelta, diff2);
		VectorAdd(diffAccum, diff2, diffAccum);
		diffAccum[0] &= 65535;
		diffAccum[1] &= 65535;
		diffAccum[2] &= 65535;
	}

	VectorCopy(client->client->ps.origin, client->r.currentOrigin);

	VectorCopy(savedPosition->r.mins,client->r.mins);
	VectorCopy(savedPosition->r.maxs, client->r.maxs);
	client->r.contents = savedPosition->r.contents;
	trap_LinkEntity(client);

	// maybe restore oldbuttons and buttons?
	// if ( ( ent->client->buttons & BUTTON_ATTACK ) && ! ( ent->client->oldbuttons & BUTTON_ATTACK ) )
}

void DF_HandleSegmentedRunPre(gentity_t* ent) {
	gclient_t* cl;
	usercmd_t* ucmdPtr;
	usercmd_t ucmd;
	int clientNum;
	qboolean resposRequested, saveposRequested;
	int msec;
	if (!ent->client) return;
	if (!(coolApi & COOL_APIFEATURE_G_USERCMDSTORE)) return;

	clientNum = ent - g_entities;
	cl = ent->client;

	if (!cl->sess.raceMode || !(cl->sess.raceStyle.runFlags & RFL_SEGMENTED)) {
		trap_G_COOL_API_PlayerUserCmdClear(clientNum);
#ifdef SEGMENTEDDEBUG
		memset(ent->client->pers.segmented.debugTime, 0, sizeof(cl->pers.segmented.debugTime));
#endif
		cl->pers.segmented.state = SEG_DISABLED;
		cl->pers.segmented.msecProgress = 0;
		return;
	}

	ucmdPtr = &cl->pers.cmd;

	msec = ucmdPtr->serverTime - cl->ps.commandTime;

	if (msec <= 0)
	{
		return; // idk why this should hapen but whatever (actually might happen after replay?)
	}

	resposRequested = cl->pers.segmented.respos;
	cl->pers.segmented.respos = qfalse;
	saveposRequested = cl->pers.segmented.savePos;
	cl->pers.segmented.savePos = qfalse;


	if (cl->pers.segmented.state == SEG_REPLAY) {
		if (resposRequested || saveposRequested) {
			// TODO we shouldnt even get here. commands from client should be blocked during a replay.
			G_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available during the replay of a run.\n\"",qtrue);
		}
		return;
	}


	if (!cl->pers.raceStartCommandTime) {

		ucmd = *ucmdPtr;
		// Not currently in a run.
		// Maybe reset recording of packets.
		if (resposRequested || saveposRequested) {
			G_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available in segmented run mode outside of an active run.\n\"",qtrue);
		}
		if (!VectorLength(cl->ps.velocity) && !ucmdPtr->forwardmove && !ucmdPtr->rightmove && !ucmdPtr->upmove && cl->ps.groundEntityNum == ENTITYNUM_WORLD || cl->pers.segmented.state < SEG_RECORDING 
			|| cl->pers.segmented.anglesDiffAccum[0] || cl->pers.segmented.anglesDiffAccum[1] || cl->pers.segmented.anglesDiffAccum[2] // just a sanity check
			|| cl->pers.segmented.anglesDiffAccumActual[0] || cl->pers.segmented.anglesDiffAccumActual[1] || cl->pers.segmented.anglesDiffAccumActual[2] // just a sanity check
			) {
			// uuuuh what about mover states etc? oh dear. i guess it wont work for maps with movers. or we do what japro does and disable movers.
			// wait i know! we can disable movers for segmented runs. ez.
			//if (cl->ps.groundEntityNum == ENTITYNUM_WORLD || cl->ps.groundEntityNum == ENTITYNUM_NONE) {
				trap_G_COOL_API_PlayerUserCmdClear(clientNum);
#ifdef SEGMENTEDDEBUG
				memset(cl->pers.segmented.debugTime, 0, sizeof(cl->pers.segmented.debugTime));
#endif
				VectorClear(cl->pers.segmented.anglesDiffAccum);
				VectorClear(cl->pers.segmented.anglesDiffAccumActual);
				SavePosition(ent, &cl->pers.segmented.startPos);
				cl->pers.segmented.state = SEG_RECORDING;
				cl->pers.segmented.msecProgress = 0;
			//}
		}

		ucmd.serverTime = cl->pers.segmented.msecProgress + msec;
		cl->pers.segmented.msecProgress += msec;
		trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &ucmd);
#ifdef SEGMENTEDDEBUG
		{
			int timeIndex = cl->pers.segmented.msecProgress / 100;
			if (timeIndex >= 0 && timeIndex < 1000) {
				int i;
				cl->pers.segmented.debugTime[timeIndex] = cl->pers.segmented.msecProgress;
				for (i = 0; i < segDebugFieldsCount; i++) {
					void* ptrSrc = ((byte*)cl)+ segDebugFields[i].offset;
					void* ptrDst = ((byte*)&cl->pers.segmented.debugVars[timeIndex])+ segDebugFields[i].offsetDebugVars;
					memcpy(ptrDst,ptrSrc, segDebugFields[i].typeSize);
				}
				//VectorCopy(cl->ps.origin,cl->pers.segmented.debugOrigin[timeIndex]);
				//VectorCopy(cl->ps.viewangles,cl->pers.segmented.debugAngles[timeIndex]);
			}
		}
#endif

		// No last pos can be stored outside a run.
		cl->pers.segmented.state = SEG_RECORDING;
		return;
	}

	if (resposRequested && saveposRequested) {
		G_SendServerCommand(ent - g_entities, "print \"^1Respos and savepos cannot be used both on the same frame during a segmented run.\n\"",qtrue);
	}
	else if (cl->pers.raceStartCommandTime >= cl->ps.commandTime && (saveposRequested || resposRequested)) {
		G_SendServerCommand(ent - g_entities, "print \"^1Respos and savepos cannot be used on the first frame of a segmented run.\n\"",qtrue);
	}
	else if (saveposRequested) {

		if (cl->pers.segmented.state == SEG_RECORDING_INVALIDATED) {
			G_SendServerCommand(ent - g_entities, "print \"^1Cannot use savepos. Your segmented run was interrupted, e.g. by a death. Please respos.\n\"",qtrue);
		}
		else {
			SavePosition(ent, &cl->pers.segmented.lastPos);
			cl->pers.stats.saveposCount++;
			cl->pers.segmented.lastPosMsecProgress = cl->pers.segmented.msecProgress;
			cl->pers.segmented.state = SEG_RECORDING_HAVELASTPOS;
			cl->pers.segmented.lastPosUserCmdIndex = trap_G_COOL_API_PlayerUserCmdGetCount(clientNum) - 1;
			VectorClear(cl->pers.segmented.anglesDiffAccum);
		}
	}
	else if(resposRequested) {
		if (cl->pers.segmented.state < SEG_RECORDING_HAVELASTPOS) {
			G_SendServerCommand(ent - g_entities, "print \"^1Cannot use respos. No past segmented run position found.\n\"",qtrue);
		}
		else if (cl->pers.segmented.state >= SEG_REPLAY) {
			// THIS SHOULD NEVER HAPPEN
			G_SendServerCommand(ent - g_entities, "print \"^1Cannot use respos during replay. WTF HOW DID WE GET HERE.\n\"",qtrue);
		}
		else {

			VectorAdd(cl->pers.segmented.anglesDiffAccumActual, cl->pers.segmented.anglesDiffAccum, cl->pers.segmented.anglesDiffAccumActual);
			cl->pers.segmented.anglesDiffAccumActual[0] &= 65535;
			cl->pers.segmented.anglesDiffAccumActual[1] &= 65535;
			cl->pers.segmented.anglesDiffAccumActual[2] &= 65535;
			VectorClear(cl->pers.segmented.anglesDiffAccum);
			RestorePosition(ent, &cl->pers.segmented.lastPos,cl->pers.segmented.anglesDiffAccumActual);
			cl->pers.stats.resposCount++;
			cl->pers.segmented.state = SEG_RECORDING_HAVELASTPOS; // un-invalidate.
			cl->pers.segmented.msecProgress = cl->pers.segmented.lastPosMsecProgress;
			trap_G_COOL_API_PlayerUserCmdRemove(clientNum, cl->pers.segmented.lastPosUserCmdIndex + 1, trap_G_COOL_API_PlayerUserCmdGetCount(clientNum) - 1);
		}
	}

	
	ucmd = *ucmdPtr;
	ucmd.angles[0] += cl->pers.segmented.anglesDiffAccumActual[0];
	ucmd.angles[1] += cl->pers.segmented.anglesDiffAccumActual[1];
	ucmd.angles[2] += cl->pers.segmented.anglesDiffAccumActual[2];
	ucmd.angles[0] &= 65535;
	ucmd.angles[1] &= 65535;
	ucmd.angles[2] &= 65535;
	ucmd.serverTime = cl->pers.segmented.msecProgress + msec;
	cl->pers.segmented.msecProgress += msec;
	trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &ucmd);
#ifdef SEGMENTEDDEBUG
	{
		int timeIndex = cl->pers.segmented.msecProgress / 100;
		if (timeIndex >= 0 && timeIndex < 1000) {
			int i;
			cl->pers.segmented.debugTime[timeIndex] = cl->pers.segmented.msecProgress;
			for (i = 0; i < segDebugFieldsCount; i++) {
				void* ptrSrc = ((byte*)cl) + segDebugFields[i].offset;
				void* ptrDst = ((byte*)&cl->pers.segmented.debugVars[timeIndex]) + segDebugFields[i].offsetDebugVars;
				memcpy(ptrDst, ptrSrc, segDebugFields[i].typeSize);
			}
			//VectorCopy(cl->ps.origin,cl->pers.segmented.debugOrigin[timeIndex]);
			//VectorCopy(cl->ps.viewangles,cl->pers.segmented.debugAngles[timeIndex]);
		}
	}
#endif


	return;

}


int DF_GetRunFlags(gentity_t* ent) {
	if (!ent->client) {
		return 0;
	}
	return ent->client->sess.raceMode ? ent->client->sess.raceStyle.runFlags : 0;
}

void DF_CarryClientOverToNewRaceStyle(gentity_t* ent, raceStyle_t* newRs) {
	clientSession_t* sess;
	if (!ent->client || ent->client->pers.connected != CON_CONNECTED || !ent->client->sess.raceMode) return;

	sess = &ent->client->sess;

	if (!memcmp(newRs, &sess->mapStyleBaseline, sizeof(raceStyle_t))) return; // no change

	if (memcmp(newRs, &sess->raceStyle,sizeof(raceStyle_t))) {
		int playerCustomRunFlagBits = sess->raceStyle.runFlags ^ sess->mapStyleBaseline.runFlags;
		int newRunFlags = (sess->raceStyle.runFlags & playerCustomRunFlagBits) | (newRs->runFlags & ~playerCustomRunFlagBits);
		int newJumpLevel = sess->raceStyle.jumpLevel;

		// the value changed from the previous map settings this client saw, and the client didn't have any custom setting
		if (newRs->jumpLevel != sess->mapStyleBaseline.jumpLevel && sess->raceStyle.jumpLevel == sess->mapStyleBaseline.jumpLevel) {
			newJumpLevel = newRs->jumpLevel;
		}

		if (sess->raceStyle.jumpLevel != newJumpLevel ||
			sess->raceStyle.runFlags != newRunFlags ||
			sess->raceStyle.variant != newRs->variant ) {

			DF_RaceStateInvalidated(ent, qtrue);

			sess->raceStyle.jumpLevel = newJumpLevel;
			sess->raceStyle.runFlags = newRunFlags;
			sess->raceStyle.variant = newRs->variant;

			G_CenterPrint(ent - g_entities,3, "^2Map defaults loaded/changed. Run reset.",qfalse,qtrue,qtrue);
			G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed. Run reset.\n\"",qtrue);
		}
		else if(sess->spectatorState != SPECTATOR_FOLLOW || sess->spectatorClient < 0) {

			G_CenterPrint(ent - g_entities,3, "^2Map defaults loaded/changed.", qfalse,qtrue,qtrue);
			G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed.\n\"", qtrue);
		}


	}
	else if (sess->spectatorState != SPECTATOR_FOLLOW || sess->spectatorClient < 0) {
		G_CenterPrint(ent - g_entities, 3,"^2Map defaults loaded/changed.", qfalse,qtrue,qtrue);
		G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed.\n\"", qtrue);
	}

	sess->mapStyleBaseline = *newRs;

}

void Cmd_MovementStyle_f(gentity_t* ent)
{
	char mStyle[32];
	int newStyle;

	if (!ent->client)
		return;

	if (trap_Argc() != 2) {
		goto showinfo;
		return;
	}

	//Do alive check here so they can see style list?
	if ((ent->health <= 0
		//|| ent->client->tempSpectate >= level.time
		|| ent->client->sess.sessionTeam == TEAM_SPECTATOR))
	{
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
		return;
	}

	if (!g_defrag.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"This command is not allowed in this gamemode!\n\"");
		return;
	}

	/*
	if (level.gametype != GT_FFA) {
		trap_SendServerCommand(ent-g_entities, "print \"This command is not allowed in this gametype!\n\"");
		return;
	}
	*/

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (VectorLength(ent->client->ps.velocity)) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be standing still to use this command!\n\"");
		return;
	}

	trap_Argv(1, mStyle, sizeof(mStyle));

	newStyle = RaceNameToInteger(mStyle);
	//Just return if newstyle = old style?

	if (!(allowedMovementStyles & (1 << newStyle))) {
		trap_SendServerCommand(ent - g_entities, "print \"Movement style not allowed!\n\"");
		return;
	}

	if (newStyle >= 0) {

		G_SendServerCommand(ent - g_entities, "print \"Movement style updated.\n\"",qtrue);


		ent->client->sess.raceStyle.movementStyle = newStyle;
		ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
		DF_RaceStateInvalidated(ent,qtrue);
		//DF_InvalidateSpawn(ent);

		if (newStyle == MV_SPEED) {
			ent->client->ps.fd.forcePower = 50;
		}
		return;
	}
showinfo:
	{
		int i,index;
		char printString[256];
		printString[0] = '\0';
		Q_strcat(printString, sizeof(printString), "print \"Usage: /move <");
		index = 0;
		for (i = 0; i < MV_NUMSTYLES; i++) {
			if ((allowedMovementStyles & (1 << i))) {
				if (index) {
					Q_strcat(printString, sizeof(printString), ",");
				}
				Q_strcat(printString, sizeof(printString), moveStyleNames[i].string);
				index++;
			}
		}
		Q_strcat(printString, sizeof(printString), ">.\n\"");
		trap_SendServerCommand(ent - g_entities, printString);
	}
}


// TODO need more checks?
void DF_SaveSpawn(gentity_t* ent) {
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (ent->client->ps.pm_type != PM_NORMAL || ent->client->ps.stats[STAT_HEALTH] <= 0) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be alive and in a normal state to use this command!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.pm_flags & PMF_FOLLOW)) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in a team to use this command!\n\"");
		return;
	}

	if (ent->client->ps.fd.forcePowersActive) {
		G_CenterPrint(ent - g_entities,3, "^1Warning: ^7You must not have any force powers activated to save spawn.",qfalse,qtrue,qtrue);
		return;
	}

	if (ent->client->sess.raceStateInvalidated) {
		G_CenterPrint(ent - g_entities,3, "^1Warning: ^7Your race state is invalidated. Please respawn before saving spawn.",qfalse,qtrue,qtrue);
		return;
	}
	
	if (ent->client->ps.velocity[0] || ent->client->ps.velocity[1] || ent->client->ps.velocity[2] || ent->client->ps.groundEntityNum != ENTITYNUM_WORLD) {
		G_CenterPrint(ent - g_entities,3, va("^1Warning: ^7Cannot save spawn. Please stand still. (gen %d, v0 %f, v1 %f, v2 %f)", ent->client->ps.groundEntityNum, ent->client->ps.velocity[0], ent->client->ps.velocity[1], ent->client->ps.velocity[2]),qfalse,qtrue,qtrue);
		return;
	}

	SavePosition(ent,&ent->client->pers.savedSpawn);
	ent->client->pers.savedSpawnUsed = qtrue;
	ent->client->pers.savedSpawnRaceStyle = ent->client->sess.raceStyle;

	G_SendServerCommand(ent - g_entities, va("print \"Spawnpoint saved at %f %f %f (angles %f %f %f).\n\"",
		ent->client->ps.origin[0],
		ent->client->ps.origin[1],
		ent->client->ps.origin[2],
		ent->client->ps.viewangles[0],
		ent->client->ps.viewangles[1],
		ent->client->ps.viewangles[2]
	),qtrue);
}

void DF_ResetSpawn(gentity_t* ent) {
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	ent->client->pers.savedSpawnUsed = qfalse;

	G_SendServerCommand(ent - g_entities, "print \"Spawnpoint has been reset.\n\"",qtrue);
}
