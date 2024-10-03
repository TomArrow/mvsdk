

#include "g_local.h"
#include "g_dbcmds.h"

// Many parts of defrag code are lifted/adapted from Triforce's JediKnightPlus and loda's japro. Thanks!

void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);
const char* DF_RacePrintAppendage(finishedRunInfo_t* runInfo);

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
qboolean DF_InAnyTrigger(vec3_t interpOrigin, const char* classname, vec3_t playerMins, vec3_t playerMaxs) // TODO make this more efficient
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
	while ((inTrigger = DF_InAnyTrigger(interpOrigin, classname,activator->client->triggerMins,activator->client->triggerMaxs)) || !touched)
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
	while (!(inTrigger = DF_InAnyTrigger(interpOrigin,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs)) || !left)
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
	int resposCountSave, savePosCountSave;

	// Check client
	if (!activator->client) return;

	cl = activator->client;

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
		trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

	segmented = cl->sess.raceStyle.runFlags & RFL_SEGMENTED;

	if (segmented && cl->pers.segmented.state != SEG_RECORDING && cl->pers.segmented.state != SEG_REPLAY) {
		trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Segmented run in a faulty state. Please respawn and try again.\n\"");
		DF_RaceStateInvalidated(activator, qfalse);
		return;
	}
	else if (segmented && cl->pers.segmented.state != SEG_REPLAY) {

		if (segmented && cl->pers.segmented.msecProgress > 5000) {
			trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Segmented run pre-record is over 5 seconds. Please respawn and try again.\n\"");
			DF_RaceStateInvalidated(activator, qfalse);
			return;
		}
		else if (segmented && cl->pers.segmented.msecProgress < 500) {
			trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Segmented run pre-record is under 0.5 seconds. Please respawn and try again.\n\"");
			DF_RaceStateInvalidated(activator, qfalse);
			return;
		}
	}

	if (DF_InAnyTrigger(cl->postPmovePosition,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs)) return; // we are still in some start trigger.

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Start Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", cl->pers.netname);
		trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7No valid pre-pmove info.\nPlease restart.\n\"");
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

	if (segmented && level.nonDeterministicEntities) {
		trap_SendServerCommand(activator - g_entities, va("cp \"Race timer started!\n^1Warning: ^7Map has %i non-deterministic\nentities. Replay/run may fail.\"", level.nonDeterministicEntities));
	}
	else {
		trap_SendServerCommand(activator - g_entities, "cp \"Race timer started!\"");
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

	if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != player->client->sess.raceStyle.jumpLevel) {
		return qfalse; // shouldnt happen
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
	if (sv_fps.integer != 100)// Does this even matter for tommyternal? everything runs on clienttime anyway. well... but demos wouldnt be proper without it, so leave it.
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

void PrintRaceTime(finishedRunInfo_t* runInfo, qboolean preliminary, qboolean showRank) {
	char nameColor, color;
	//static char awardString[MAX_STRING_CHARS - 2] = { 0 };
	static char messageStr[MAX_STRING_CHARS - 2] = { 0 };
	static char fpsStr[10] = { 0 };

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
	if (!preliminary && runInfo->userId != -1)
		color = '5';
	else if (!preliminary)
		color = '2';
	else
		color = '1';


	if (!showRank) {
		const char* runFlagsString = RunFlagsToString(runInfo->raceStyle.runFlags, level.mapDefaultRaceStyle.runFlags, 1,"^3",NULL);
		const int runFlagsStringLen = strlen(runFlagsString);
		Q_strncpyz(fpsStr, runInfo->raceStyle.msec == -1 ? "togl" : (runInfo->raceStyle.msec == -2 ? "flt" : (runInfo->raceStyle.msec == 0 ? "unkn" : va("%d", 1000 / runInfo->raceStyle.msec))), sizeof(fpsStr));

		Q_strncpyz(messageStr, va("^3%12s^%c  ^3%7.2f^%cmax ^3%7.2f^%cavg ^3%7.1fk^%cdist ^3%2i^%cj ^3%4s^%cfps ^3%6s^%c style %s ^%c",
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
		

		trap_SendServerCommand(-1, va("print \"%s\n\" dffinish %s", messageStr, DF_RacePrintAppendage(runInfo)));
	}
	else if (runInfo->rankLB != -1) {

		if (runInfo->rankLB == 1 && (runInfo->pbStatus & PB_LB)) { //was 1 when it shouldnt have been.. ?
			Q_strncpyz(messageStr, va("%s ^%c[^%c%s^%c] %sbeat the ^3WORLD RECORD^%c and %s ranked ^3#%i\n",runInfo->netname,color, runInfo->userId == -1 ? '1' : nameColor,runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username,color, runInfo->userId == -1 ? "unofficially " : "",color, runInfo->userId == -1 ? "would be " : "is now",runInfo->rankLB), sizeof(messageStr));
			if (runInfo->userId != -1) {
				PlayActualGlobalSound(G_SoundIndex("sound/movers/sec_panel_pass"));
				//G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass"));
			}
		}
		else if ((runInfo->pbStatus & PB_LB)) {
			Q_strncpyz(messageStr, va("%s ^%c[^%c%s^%c] got a new personal best and %s ranked ^3#%i\n", runInfo->netname, color, runInfo->userId == -1 ? '1' : nameColor, runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username, color,  runInfo->userId == -1 ? "would be " : "is now", runInfo->rankLB), sizeof(messageStr));
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

		trap_SendServerCommand(-1, va("print \"%s\" dffinish_ranked %s", messageStr, DF_RacePrintAppendage(runInfo)));
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

static void DF_FillClientRunInfo(finishedRunInfo_t* runInfo, gentity_t* ent, int milliseconds) {
	static char serverInfo[BIG_INFO_STRING];
	static char course[COURSENAME_MAX_LEN + 1];
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
		);
}

// Stop race timer
void DF_FinishTimer_Touch(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	gclient_t* cl;
	int	timeLast, timeBest,newRaceBestTime, lessTime = 0;
	char timeLastStr[32], timeBestStr[32];
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
	Q_strncpyz(timeBestStr, DF_MsToString(timeBest), sizeof(timeBestStr));

	if ((cl->sess.raceStyle.runFlags & RFL_SEGMENTED) && cl->pers.segmented.state != SEG_REPLAY) {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the segmented race in %f units [^2%s^7]: ^1Estimate! Starting rerun now.\n\" dfsegprelim %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, DF_RacePrintAppendage(&runInfo))); // extra params: type runId clientNum milliseconds leveltimeend endcommandtime endInterpolationReduction warningFlags top average distance username
		cl->pers.segmented.state = SEG_REPLAY;
		cl->pers.segmented.playbackStartedTime = level.time;
		cl->pers.segmented.playbackNextCmdIndex = 0;
		return;
	}

	//isInserting = G_InsertRun(activator, timeLast,0,0,0, warningFlags, level.time, runId, cl->ps.commandTime - lessTime);
	isInserting = G_InsertRun(&runInfo);


	PrintRaceTime(&runInfo, qfalse, qfalse);
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
	trap_SendServerCommand(activator - g_entities, "cp \"Race timer finished!\"");

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
	trap_SendServerCommand(activator - g_entities, va("cp \"Checkpoint!\n^3%s\"", DF_MsToString(timeCheck)));
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
		ent->client->sess.raceMode = qfalse;
		return;
	}

	if (g_gametype.integer != GT_FFA) {
		if (g_gametype.integer >= GT_TEAM && g_defrag.integer)
		{//this is ok

			ent->s.weapon = WP_SABER; //Dont drop our weapon
			Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S

			ent->client->sess.raceMode = qfalse;//Set it false here cuz we are flipping it next
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
		ent->client->sess.raceMode = qfalse;
		ent->s.weapon = WP_SABER; //Dont drop our weapon
		Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S
		trap_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled off.\n\"");
	}
	else {
		ent->client->sess.raceMode = qtrue;
		trap_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled on.\n\"");
	}

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
		trap_SendServerCommand(ent - g_entities, "cp \"Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n\"");
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
	int level;

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
	level = atoi(jLevel);

	if (level >= 1 && level <= 3) {
		ent->client->sess.raceStyle.jumpLevel = level;
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = ent->client->sess.raceStyle.jumpLevel;
		DF_RaceStateInvalidated(ent, qtrue);
		//DF_InvalidateSpawn(ent);
		if (ent->client->pers.raceStartCommandTime) {
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i): timer reset.\n\"", level));
		}
		else
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i).\n\"", level));
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
	gclient_t* cl;
	int i;
	for (i = 0; i < level.num_entities; i++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		if (saveState) {
			backupValues[i].solidValue = other->s.solid;
			//if (other->s.eType == ET_MOVER) { // hackily "fix" client-timed mover prediction for cgame
				//backupValues[i].trTime = other->s.pos.trTime;
				//other->s.pos.trTime += level.time - ACTIVATORTIME(other->activatorReal);
			//}
		}
		if (ShouldNotCollide(ent,other)) {
			other->s.solid = 0;
		}
		else if (!saveState){
			other->s.solid = backupValues[i].solidValue;
		}

		// avoid issues with custom lightsaber moves on clients.
		// it doesnt USUALLY crash but its an access past the end of the array and other compilers or sanitizers might cause a crash
		// also, cg_debugsabers 1 causes aa crash on cgame due to accessing a broken char* pointer
		// TODO: is sabermove used for anything else?
		// TODO: Don't do this if client has tommyternal client?
		if (saveState) backupValues[i].saberMove = other->s.saberMove;
		if (other->s.saberMove >= LS_MOVE_MAX_DEFAULT) {
			other->s.saberMove = LS_READY;
		}
		if (other->client) {
			cl = other->client;
			if (saveState) { 
				backupValues[i].saberMovePS = cl->ps.saberMove;
				backupValues[i].pmfFollowPS = cl->ps.pm_flags & PMF_FOLLOW;
				VectorCopy(cl->ps.origin, backupValues[i].psMoverOldPos);
				CG_AdjustPositionForClientTimeMover(cl->ps.origin, cl->ps.groundEntityNum, cl->ps.origin); // silly bs (that doesnt work)
			}
			if (cl->sess.raceMode && (cl->sess.raceStyle.runFlags & RFL_SEGMENTED) && cl->pers.segmented.state == SEG_REPLAY) {
				cl->ps.pm_flags |= PMF_FOLLOW;
			}
			if (cl->ps.saberMove >= LS_MOVE_MAX_DEFAULT) {
				cl->ps.saberMove = LS_READY;
			}
		}
	}
}
void PlayerSnapshotRestoreValues() {
	gentity_t* other;
	gclient_t* cl;
	int i;
	for (i = 0; i < level.num_entities; i++) {
		other = g_entities + i;
		if (!other->r.linked || !other->inuse) {
			continue;
		}
		other->s.solid = backupValues[i].solidValue;
		other->s.saberMove = backupValues[i].saberMove; 
		//if (other->s.eType == ET_MOVER) {
		//	other->s.pos.trTime = backupValues[i].trTime;
		//}
		if (other->client) {
			cl = other->client;
			cl->ps.saberMove = backupValues[i].saberMovePS;
			cl->ps.pm_flags = (cl->ps.pm_flags & ~PMF_FOLLOW) | backupValues[i].pmfFollowPS;
			VectorCopy(backupValues[i].psMoverOldPos, cl->ps.origin);
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
			trap_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"");
			return;
		}

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;
		flag = 1 << index;

		//if (index2 < 0 || index2 >= MAX_RUN_FLAGS) {
		if (!(allowedRunFlags & flag)) {
			trap_SendServerCommand(ent - g_entities, va("print \"Run flags: Invalid flag: %i [0, %i]\n\"", index2, MAX_RUN_FLAGS - 1));
			return;
		}

		if (flag & RFL_SEGMENTED) {

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
			DF_RaceStateInvalidated(ent,qtrue);
			//DF_InvalidateSpawn(ent);
		}

		trap_SendServerCommand(ent - g_entities, va("print \"^7%s %s^7\n\"", runFlagsNames[index2].string, ((ent->client->sess.raceStyle.runFlags & flag)
			? "^2Enabled" : "^1Disabled")));
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
		client->ps.fd.forcePowerLevel[FP_LEVITATION] = client->sess.raceStyle.jumpLevel;
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
		trap_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"");
		return;
	}

	if (ent->client->sess.raceStyle.msec != -1) {
		ent->client->sess.raceStyle.msec = -1;
		trap_SendServerCommand(ent - g_entities, "print \"^7Toggle mode activated.\n\"");
	}
	else {
		ent->client->sess.raceStyle.msec = 0;
		trap_SendServerCommand(ent - g_entities, "print \"^7Toggle mode disabled.\n\"");
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
		trap_SendServerCommand(ent - g_entities, "print \"^7Cannot change race settings during a run.\n\"");
		return;
	}

	if (ent->client->sess.raceStyle.msec != -2) {
		ent->client->sess.raceStyle.msec = -2;
		trap_SendServerCommand(ent - g_entities, "print \"^7Float physics mode activated.\n\"");
	}
	else {
		ent->client->sess.raceStyle.msec = 0;
		trap_SendServerCommand(ent - g_entities, "print \"^7Float physics mode disabled.\n\"");
	}

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
			trap_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available during the replay of a run.\n\"");
		}
		return;
	}


	if (!cl->pers.raceStartCommandTime) {

		ucmd = *ucmdPtr;
		// Not currently in a run.
		// Maybe reset recording of packets.
		if (resposRequested || saveposRequested) {
			trap_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available in segmented run mode outside of an active run.\n\"");
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
		trap_SendServerCommand(ent - g_entities, "print \"^1Respos and savepos cannot be used both on the same frame during a segmented run.\n\"");
	}
	else if (cl->pers.raceStartCommandTime >= cl->ps.commandTime && (saveposRequested || resposRequested)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Respos and savepos cannot be used on the first frame of a segmented run.\n\"");
	}
	else if (saveposRequested) {

		if (cl->pers.segmented.state == SEG_RECORDING_INVALIDATED) {
			trap_SendServerCommand(ent - g_entities, "print \"^1Cannot use savepos. Your segmented run was interrupted, e.g. by a death. Please respos.\n\"");
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
			trap_SendServerCommand(ent - g_entities, "print \"^1Cannot use respos. No past segmented run position found.\n\"");
		}
		else if (cl->pers.segmented.state >= SEG_REPLAY) {
			// THIS SHOULD NEVER HAPPEN
			trap_SendServerCommand(ent - g_entities, "print \"^1Cannot use respos during replay. WTF HOW DID WE GET HERE.\n\"");
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

		trap_SendServerCommand(ent - g_entities, "print \"Movement style updated.\n\"");


		ent->client->sess.raceStyle.movementStyle = newStyle;
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
		trap_SendServerCommand(ent - g_entities, "cp \"^1Warning:\n^7You must not have any force powers activated to use this command.\n\"");
		return;
	}

	if (ent->client->sess.raceStateInvalidated) {
		trap_SendServerCommand(ent - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before saving spawn.\n\"");
		return;
	}
	
	if (ent->client->ps.velocity[0] || ent->client->ps.velocity[1] || ent->client->ps.velocity[2] || ent->client->ps.groundEntityNum != ENTITYNUM_WORLD) {
		trap_SendServerCommand(ent - g_entities, va("cp \"^1Warning:\n^7Cannot save spawn.\nPlease stand still. \n(gen %d,\n v0 %f\n, v1 %f\n, v2 %f)\n\"", ent->client->ps.groundEntityNum, ent->client->ps.velocity[0], ent->client->ps.velocity[1], ent->client->ps.velocity[2]));
		return;
	}

	SavePosition(ent,&ent->client->pers.savedSpawn);
	ent->client->pers.savedSpawnUsed = qtrue;
	ent->client->pers.savedSpawnRaceStyle = ent->client->sess.raceStyle;

	trap_SendServerCommand(ent - g_entities, va("print \"Spawnpoint saved at %f %f %f (angles %f %f %f).\n\"",
		ent->client->ps.origin[0],
		ent->client->ps.origin[1],
		ent->client->ps.origin[2],
		ent->client->ps.viewangles[0],
		ent->client->ps.viewangles[1],
		ent->client->ps.viewangles[2]
	));
}

void DF_ResetSpawn(gentity_t* ent) {
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	ent->client->pers.savedSpawnUsed = qfalse;

	trap_SendServerCommand(ent - g_entities, "print \"Spawnpoint has been reset.\n\"");
}
