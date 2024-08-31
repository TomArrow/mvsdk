

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

	if (!activator->client->sess.raceMode) return;

	if (activator->client->sess.raceStateInvalidated) {
		trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

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

	if (!activator->client->sess.raceMode) return;

	if (activator->client->sess.raceStateInvalidated) {
		//trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

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
void DF_CheckpointTimer_Touch(gentity_t* trigger, gentity_t* activator, trace_t* trace) // TODO Make this only trigger on first contact
{
	int	timeCheck, lessTime=0;

	// Check client
	if (!activator->client) return;

	if (!activator->client->sess.raceMode) return;

	if (activator->client->sess.raceStateInvalidated) {
		//trap_SendServerCommand(activator - g_entities, "cp \"^1Warning:\n^7Your race state is invalidated.\nPlease respawn before running.\n\"");
		return;
	}

	// Check timer
	if (!activator->client->pers.raceStartCommandTime) return;

	if (level.time - activator->client->pers.raceLastCheckpointTime < 1000) return; // don't spam.

	// we ideally only wanna display checkpoints if the player didn't touch them last frame.
	// doesn't matter for finish triggers as much since they end runs the first time they are touched.
	if (level.time - trigger->triggerLastPlayerContact[activator-g_entities] < 1000) return; 

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Checkpoint Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid checkpoint pre-pmove info.\n\"", activator->client->pers.netname));
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, trigger, "df_trigger_checkpoint");
	}

	// Set info
	timeCheck = activator->client->ps.commandTime - lessTime - activator->client->pers.raceStartCommandTime;

	// Show info
	trap_SendServerCommand(activator - g_entities, va("cp \"Checkpoint!\n^3%s\"", DF_MsToString(timeCheck)));
	activator->client->pers.raceLastCheckpointTime = level.time;
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

void RemoveLaserTraps(gentity_t* ent);
void RemoveDetpacks(gentity_t* ent);
void DeletePlayerProjectiles(gentity_t* ent);
void Cmd_ForceChanged_f(gentity_t* ent);
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
		VectorClear(ent->client->ps.velocity);
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

	if (wasReset && print)
		trap_SendServerCommand(ent - g_entities, "cp \"Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n\"");
}

static void DF_RaceStateInvalidated(gentity_t* ent, qboolean print)
{
	ResetSpecificPlayerTimers(ent, print);
	ent->client->ps.fd.forcePower = 100; //Reset their force back to full i guess!
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
		if (ent->client->pers.raceStartCommandTime) {
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i): timer reset.\n\"", level));
		}
		else
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i).\n\"", level));
	}
	else
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /jump <level>\n\"");
}


qboolean MovementStyleAllowsWeapons(int moveStyle) {
	return qfalse;
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
						((other->s.eType == ET_MOVER) &&
							(!(Q_stricmp(other->classname, "func_door")) ||
								(!(Q_stricmp(other->classname, "func_plat"))))) ||
						((other->s.eType == ET_GENERAL) &&
							(!(Q_stricmp(other->classname, "laserTrap")) ||
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

				return qtrue;
			}
		}
	}
	return qfalse;
}


void JP_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {

	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
	if (results->entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t* passEnt = g_entities + passEntityNum;
		gentity_t* ent = g_entities + results->entityNum;

		if (ShouldNotCollide(passEnt, ent))
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			JP_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
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

static int solidValues[MAX_GENTITIES];
void PlayerSnapshotSetSolid(qboolean saveState, int clientNum) {
	gentity_t* ent = g_entities + clientNum;
	gentity_t* other;
	int i;
	for (i = 0; i < level.num_entities; i++) {
		other = g_entities + i;
		solidValues[i] = other->s.solid;
		if (ShouldNotCollide(ent,other)) {
			other->s.solid = 0;
		}
	}
}
void PlayerSnapshotRestoreSolid() {
	gentity_t* other;
	int i;
	for (i = 0; i < level.num_entities; i++) {
		other = g_entities + i;
		other->s.solid = solidValues[i];
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
			Com_Printf("^7Cannot change race settings during a run.");
			return;
		}

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;
		flag = 1 << index;

		//if (index2 < 0 || index2 >= MAX_RUN_FLAGS) {
		if (!(allowedRunFlags & flag)) {
			Com_Printf("Run flags: Invalid flag: %i [0, %i]\n", index2, MAX_RUN_FLAGS - 1);
			return;
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
		}

		Com_Printf("%s %s^7\n", runFlagsNames[index2].string, ((ent->client->sess.raceStyle.runFlags & flag)
			? "^2Enabled" : "^1Disabled"));
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
			Com_Printf("%2d ^%d[%s] ^7%s\n", i, differentFromDefault ? 1 : 7, isSet ? "X" : " ", runFlagsNames[i].string);
		}
		if (differences) {
			Com_Printf("Differences from map default are marked ^1red^7. Your runs will not be on the main leaderboard with non-default settings.\n");
		}
	}
}
