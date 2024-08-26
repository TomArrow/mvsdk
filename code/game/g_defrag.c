

#include "g_defrag.h"
#include "g_local.h"

// NOTE: For start timer, make sure we are not standing in any existing start timer before actually starting, 
// even when leave() is already being called. Only the last left start trigger should actually trigger.

// q3 defrag targets:
// target_starttimer
// target_stoptimer 
// target_checkpoint 

typedef enum q3DefragTargetType {
	TARGET_STARTTIMER,
	TARGET_STOPTIMER,
	TARGET_CHECKPOINT,
	TARGET_TYPE_COUNT
};

static const char* q3DefragTargetNames[] = {
	"target_startTimer",
	"target_stopTimer",
	"target_checkpoint"
};

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
qboolean DF_InAnyTrigger(vec3_t interpOrigin, const char* classname)
{
	vec3_t	mins, maxs;
	vec3_t	playerMins, playerMaxs;
	gentity_t* trigger;

	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	VectorAdd(interpOrigin, playerMins, mins);
	VectorAdd(interpOrigin, playerMaxs, maxs);

	trigger = NULL;
	while ((trigger = G_Find(trigger, FOFS(classname), classname)) != NULL) {
		if (trap_EntityContact(mins, maxs, trigger)) return qtrue;
	}

	return qfalse;
}
/* Outdated and boring!
int DF_InterpolateTouchTime(gentity_t* activator, gentity_t* trigger)
{
	vec3_t	interpOrigin, delta;
	int lessTime = -1;

	qboolean touched = qfalse;
	qboolean inTrigger;

	VectorCopy(activator->client->ps.origin, interpOrigin);
	VectorScale(activator->s.pos.trDelta, 0.001f, delta);

	while ((inTrigger = DF_InTrigger(interpOrigin, trigger)) || !touched)
	{
		if (inTrigger) touched = qtrue;

		lessTime++;
		VectorSubtract(interpOrigin, delta, interpOrigin);

		if (lessTime >= 250) break;
	}

	return lessTime;
}*/
int DF_InterpolateTouchTimeToOldPos(gentity_t* activator, gentity_t* trigger, const char* classname) // For finish and checkpoint trigger
{
	vec3_t	interpOrigin, delta;
	int lessTime = -1;

	int msecDelta = activator->client->ps.commandTime- activator->client->prePmoveCommandTime;
	qboolean touched = qfalse;
	qboolean inTrigger;
	float msecScale = 1.0f / (float)msecDelta;

	VectorCopy(activator->client->ps.origin, interpOrigin);
	VectorSubtract(activator->client->prePmovePosition, activator->client->ps.origin,delta);
	VectorScale(activator->s.pos.trDelta, msecScale, delta);

	//while ((inTrigger = DF_InTrigger(interpOrigin, trigger)) || !touched)
	while ((inTrigger = DF_InAnyTrigger(interpOrigin, classname)) || !touched)
	{
		if (inTrigger) touched = qtrue;

		lessTime++;
		VectorAdd(interpOrigin, delta, interpOrigin);

		if (lessTime >= msecDelta) break;
	}

	return MAX(0,lessTime-1); // -1 because the final lessTime value we were already out of the trigger, but we want the first frame IN the trigger to count
}
int DF_InterpolateTouchTimeForStartTimer(gentity_t* activator, gentity_t* trigger) // For start trigger
{
	// TODO: Make this check for ANY start triggers
	vec3_t	interpOrigin, delta;
	int lessTime = -1;

	int msecDelta = activator->client->ps.commandTime- activator->client->prePmoveCommandTime;
	qboolean left = qfalse;
	qboolean inTrigger;
	float msecScale = 1.0f / (float)msecDelta;

	VectorCopy(activator->client->ps.origin, interpOrigin);
	VectorSubtract(activator->client->prePmovePosition, activator->client->ps.origin,delta);
	VectorScale(activator->s.pos.trDelta, msecScale, delta);

	//while (!(inTrigger = DF_InTrigger(interpOrigin, trigger)) || !left)
	while (!(inTrigger = DF_InAnyTrigger(interpOrigin,"df_trigger_start")) || !left)
	{
		if (!inTrigger) left = qtrue;

		lessTime++;
		VectorAdd(interpOrigin, delta, interpOrigin);

		if (lessTime >= msecDelta) break;
	}

	return MAX(0,lessTime-1); // -1 because the final lessTime value we were already out of the trigger, but we want the first frame IN the trigger to count
}



// Start race timer
void DF_StartTimer_Leave(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	int	lessTime = 0;
	
	// Check client
	if (!activator->client) return;

	if (DF_InAnyTrigger(activator->client->ps.origin,"df_trigger_start")) return; // we are still in some start trigger.

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Start Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7No valid pre-pmove info.\nPlease restart.\n\"");
		return;
	}
	else {
		lessTime = DF_InterpolateTouchTimeForStartTimer(activator, ent);
	}

	// Set timers
	//activator->client->ps.duelTime = activator->client->ps.commandTime - lessTime;
	activator->client->pers.raceStartCommandTime = activator->client->ps.commandTime - lessTime;

	trap_SendServerCommand(activator - g_entities, "cp \"Race timer started!\"");
}
const char* DF_MsToString(const int ms)
{
	int	timeSec, timeMin, timeMsec;

	timeMsec = ms;
	timeSec = timeMsec / 1000;
	timeMsec -= timeSec * 1000;
	timeMin = timeSec / 60;
	timeSec -= timeMin * 60;

	return !ms ? "00:00.000" : va("%02i:%02i.%03i", timeMin, timeSec, timeMsec);
}

// Stop race timer
void DF_FinishTimer_Touch(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	int	timeLast, timeBest, lessTime = 0;
	char timeLastStr[32], timeBestStr[32];
	
	// Check client
	if (!activator->client) return;

	// Check timer
	if (!activator->client->pers.raceStartCommandTime) return;

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Finish Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid pre-pmove info.\n\"", activator->client->pers.netname));
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, ent, "df_trigger_finish");
	}

	// Set info
	timeLast = activator->client->ps.commandTime - lessTime - activator->client->pers.raceStartCommandTime;
	timeBest = !activator->client->pers.raceBestTime ? timeLast : activator->client->pers.raceBestTime;

	Q_strncpyz(timeLastStr, DF_MsToString(timeLast), sizeof(timeLastStr));
	Q_strncpyz(timeBestStr, DF_MsToString(timeBest), sizeof(timeBestStr));

	// Show info
	if (timeLast == timeBest) {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in [^2%s^7]\n\"", activator->client->pers.netname, timeLastStr));
	}
	else if (timeLast < timeBest) {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in [^5%s^7] which is a new personal record!\n\"", activator->client->pers.netname, timeLastStr));
	}
	else {
		trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the race in [^2%s^7] and his record was [^5%s^7]\n\"", activator->client->pers.netname, timeLastStr, timeBestStr));
	}

	// Play sound
	if (timeLast < timeBest) G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass")); // TODO ok but lets precache it?

	// Show info
	trap_SendServerCommand(activator - g_entities, "cp \"Race timer finished!\"");

	// Update timers
	//activator->client->pers.raceLastTime = timeLast;
	activator->client->pers.raceBestTime = timeLast > timeBest ? timeBest : timeLast;

	// Update client
	//ClientUserinfoChanged(activator - g_entities); // TODO why?

	// Reset timers
	//activator->client->ps.duelTime = 0;
	activator->client->pers.raceStartCommandTime = 0;
}

// Checkpoint race timer
void DF_CheckpointTimer_Touch(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	int	timeCheck, lessTime=0;

	// Check client
	if (!activator->client) return;

	// Check timer
	if (!activator->client->pers.raceStartCommandTime) return;

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Checkpoint Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid checkpoint pre-pmove info.\n\"", activator->client->pers.netname));
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, ent, "df_trigger_checkpoint");
	}

	// Set info
	timeCheck = activator->client->ps.commandTime - lessTime - activator->client->pers.raceStartCommandTime;

	// Show info
	trap_SendServerCommand(activator - g_entities, va("cp \"Checkpoint!\n^3%s\"", DF_MsToString(timeCheck)));
}

void DF_target_husk(gentity_t* ent) {
	// do nothing. we just wanna be able to find it and replace it.
}


extern void InitTrigger(gentity_t* self);
void DF_trigger_start_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->r.contents |= CONTENTS_TRIGGER_EXIT;
	ent->leave = DF_StartTimer_Leave;

	trap_LinkEntity(ent);
}
void DF_trigger_finish_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_FinishTimer_Touch;

	trap_LinkEntity(ent);
}
void DF_trigger_checkpoint_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->touch = DF_CheckpointTimer_Touch;

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
