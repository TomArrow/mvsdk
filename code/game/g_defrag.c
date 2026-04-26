

#include "g_local.h"
#include "g_dbcmds.h"

int semiBreakingChangeVersionDefrag = SEMIBREAKINGCHANGEVERSIONDEFRAG;

// Many parts of defrag code are lifted/adapted from Triforce's JediKnightPlus and loda's japro. Thanks!

void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);
const char* DF_RacePrintAppendage(finishedRunInfo_t* runInfo);
void DF_CheckpointTimer_Touch(gentity_t* trigger, gentity_t* activator, trace_t* trace);
void DF_CarryClientOverToNewRaceStyle(gentity_t* ent, raceStyle_t* newRs);
void DF_StartSegmentedReplay(gentity_t* ent, qboolean restart);

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
		FIELDSFUNC(sess.raceStateSoftInvalidated)\
		FIELDSFUNC(pers.teamState.flagsince)\
		FIELDSFUNC(pers.teamState.lastfraggedcarrier)\
		FIELDSFUNC(pers.teamState.lasthurtcarrier)\
		FIELDSFUNC(pers.teamState.lastreturnedflag)\
		FIELDSFUNC(pers.stats.distanceTraveled)\
		FIELDSFUNC(pers.stats.distanceTraveled2D)\
		FIELDSFUNC(pers.stats.topSpeed)\
		FIELDSFUNC(pers.stats.checkpoints)\
		FIELDSFUNC(pers.stats.score)\
		FIELDSFUNC(pers.stats.roll)\
		FIELDSFUNC(pers.stats.q3RallyState)\
		FIELDSFUNC(pers.stats.fpsStats)\
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
		FIELDSFUNC(client->pers.stats.roll.lastRollEndedTime)\
		FIELDSFUNC(client->pers.stats.roll.rollAirStarted)\
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

subContestParams_t subContestParams[SUBCONTESTS_COUNT] = {
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_ROLLYMPICS
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_ROLLYMPICS_FIX
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_DBS_SPEED
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_DBS_KILL
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_DBS_IRONMAN
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_DBS_CTFRETURNS
	{SUBCONTEST_TYPE_MAXVAL}, // SUBCONTESTS_DBS_KILL_SPEEDLOSS
};

const char* nameTagTypeNames[NAMETAG_COUNT] = {
	"none",
	"freedom",
	"oc9"
};

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
	TARGET_SPEED,
	TARGET_TYPE_COUNT
} q3DefragTargetType_t;

typedef enum q3CourseType_s {
	Q3COURSE_UNIVERSAL,
	Q3COURSE_CPMONLY,
	Q3COURSE_VQ3ONLY,
	Q3COURSE_TYPECOUNT,
}q3CourseType_t;

static const char* q3DefragTargetNames[] = {
	"target_startTimer",
	"target_stopTimer",
	"target_checkpoint",
	"target_speed"
};

void DF_InvalidateSpawn(gentity_t* ent) {
	if (!ent->client) return;

	ent->client->pers.savedSpawnUsed = qfalse;
}

void G_SendOrPrint(gentity_t* playerOrNull, const char* text) {
	if (playerOrNull) {
		trap_SendServerCommand(playerOrNull-g_entities,va("print \"%s\"",text));
	}
	else {
		Com_Printf("%s",text);
	}
}
#define BUFFERED_TEXT_MAX_LENGTH (MAX_STRING_CHARS-sizeof("print \"\"")-1)


static bufferedPrint_t broadcastPrint = { 0 };
// to avoid server command overflow when sending a LOT of prints
void G_BufferedSendOrPrint(gentity_t* playerOrNull, qboolean broadcast, qboolean normalPrint, const char* text) {
	if (normalPrint && (playerOrNull || broadcast)) {
		int clNum = broadcast ? -1 : (playerOrNull - g_entities);
		trap_SendServerCommand(clNum, va("print \"%s\"", text));
	}
	else
	{
		bufferedPrint_t* bufferedPrint = broadcast ? &broadcastPrint : (playerOrNull ? &playerOrNull->client->bufferedPrint : NULL);
		if (bufferedPrint) {
			int clNum = broadcast ? -1 : (playerOrNull - g_entities);
			int lenOld = bufferedPrint->curLen;
			int lenNew = strlen(text);
			if ((lenOld + lenNew) > BUFFERED_TEXT_MAX_LENGTH) {
				// overflowing, flush what's already there and buffer the new text
				trap_SendServerCommand(clNum, va("print \"%s\"", bufferedPrint->buffer));
				Q_strncpyz(bufferedPrint->buffer, text, sizeof(bufferedPrint->buffer));
				bufferedPrint->curLen = lenNew;
			}
			else if ((lenOld + lenNew) == BUFFERED_TEXT_MAX_LENGTH) {
				// can't fit any more after this, so just send immediately
				trap_SendServerCommand(clNum, va("print \"%s%s\"", bufferedPrint->buffer, text));
				*bufferedPrint->buffer = '\0';
				bufferedPrint->curLen = 0;
			}
			else {
				// still room. buffer it
				Q_strncpyz(bufferedPrint->buffer+lenOld, text, sizeof(bufferedPrint->buffer)-lenOld);
				bufferedPrint->curLen = lenOld + lenNew;
			}
			bufferedPrint->bufferLastFlushedOrUpdated = level.time;
		}
		else {
			Com_Printf("%s", text);
		}
	}
}
void G_BufferedSendOrPrintFlush(gentity_t* playerOrNull, qboolean broadcast) {
	bufferedPrint_t* bufferedPrint = broadcast ? &broadcastPrint : (playerOrNull ? &playerOrNull->client->bufferedPrint : NULL);
	if (bufferedPrint && *bufferedPrint->buffer) {
		int clNum = broadcast ? -1 : (playerOrNull - g_entities);
		trap_SendServerCommand(clNum,va("print \"%s\"", bufferedPrint->buffer));
		*bufferedPrint->buffer = '\0';
		bufferedPrint->curLen = 0;
		bufferedPrint->bufferLastFlushedOrUpdated = level.time;
	}
}
void G_BufferedSendOrPrintFlushIfNeeded(gentity_t* playerOrNull, qboolean broadcast) {
	bufferedPrint_t* bufferedPrint = broadcast ? &broadcastPrint : (playerOrNull ? &playerOrNull->client->bufferedPrint : NULL);
	if (bufferedPrint && *bufferedPrint->buffer) {
		if (bufferedPrint->bufferLastFlushedOrUpdated + 1000 < level.time || level.time < bufferedPrint->bufferLastFlushedOrUpdated) {
			Com_Printf("^3Flushing client print buffer due to 1000ms passing without flush or update. Code logic problem? (broadcast %d)\n",broadcast);
			G_BufferedSendOrPrintFlush(playerOrNull, broadcast);
		}
	}
}


// We try to find out the current mod running, or something to distinguish multiple instances of the mod running,
// so that we can store temporary demo files without interfering with the other instances
// bit cringe but nicer than having to set an extra g_ cvar
void G_SetupTempDemoSubfolderName() {
	char comb[20];
	char cvar[MAX_QPATH];
	int i;
	char* s;
	comb[0] = '\0';
	trap_Cvar_VariableStringBuffer("net_portReal", cvar, sizeof(cvar));
	if (cvar[0]) {
		Q_strcat(comb, sizeof(comb), cvar);
		Q_strcat(comb, sizeof(comb), "-");
	}
	trap_Cvar_VariableStringBuffer("fs_game", cvar, sizeof(cvar));
	if (cvar[0]) {
		Q_strcat(comb, sizeof(comb), cvar);
		Q_strcat(comb, sizeof(comb), "-");
	}
	trap_Cvar_VariableStringBuffer("fs_cfgLogPath", cvar, sizeof(cvar));
	if (cvar[0]) {
		Q_strcat(comb, sizeof(comb), cvar);
	}
	for (i = 0, s=comb; i < sizeof(comb); i++,s++) {
		if (*s == '\0') break;
		if (*s >= 'a' && *s <= 'z'
			|| *s >= 'A' && *s <= 'Z'
			|| *s >= '0' && *s <= '9'
			|| *s == '_'
			|| *s == '-'
			//|| *s == '.' // could mess with filenames/paths (checkdirtraversal)
			//|| *s == '/' // could mess with filenames (as it is a folder separator)
			|| *s == '['
			|| *s == ']'
			|| *s == '('
			|| *s == ')'
			//|| *s == '<'	// demonames: windows wont allow this in filenames
			//|| *s == '>'	// demonames: windows wont allow this in filenames
			|| *s == '='
			//|| *s == ':'	// demonames: windows wont allow this in filenames
			|| *s == ';'
			|| *s == '+'
			//|| *s == '*'	// demonames: windows wont allow this in filenames
			|| *s == '@'
			) {
			// whitelist. ok.
		}
		else {
			(*s) = '_';
		}
	}
	Q_strncpyz(level.tempDemoNamePrefix,va("%s/",comb), sizeof(level.tempDemoNamePrefix));
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

static int DF_GetNewDemoId() {
	char s[15];
	int num;
	trap_Cvar_VariableStringBuffer("g_defragLastDemoId", s, sizeof(s));
	num = atoi(s);
	num++;
	trap_Cvar_Set("g_defragLastDemoId", va("%d", num));
	return num;
}

void DF_SaveErrorDemo(gentity_t* ent, const char* demoname, const char* errorPrint) {
	gclient_t* cl = ent->client;
	static char sanitizedCourseName[COURSENAME_MAX_LEN+1];
	if (!ent->client) {
		return;
	}
	sanitizeFilename(DF_GetCourseName(qfalse), sanitizedCourseName, qfalse); // take care of possible special cahrs the filesystem may not like
	Com_Printf("^3Error demo requested: %s\n", errorPrint);
	if (cl->pers.keepDemoMaybe) {
		Com_Printf("^1Can't save error demo %s because the game seems to already need the demo elsewhere.\n",demoname);
		return;
	}
	if (!ent->client->pers.recordingDemo) { // thanks to pre-recording we'll get a bit into the past too
		int demoId = DF_GetNewDemoId();

		Com_sprintf(cl->pers.tempDemoName, sizeof(cl->pers.tempDemoName), "%stemp/temp%d_%d",level.tempDemoNamePrefix, cl->ps.clientNum, demoId);
		cl->pers.recordingDemo = qtrue;

		trap_SendConsoleCommand(EXEC_APPEND, va("svdemometa %d dfv %d;svrecord \"%s\" %i\n", cl->ps.clientNum, g_dfv.integer, cl->pers.tempDemoName, cl->ps.clientNum));
		cl->pers.demoStartedTime = level.time;
	}
	if (cl->pers.tempDemoName[0]) {

		//char cvarstr[64];

		qtime_t q;
		trap_RealTime(&q);

		cl->pers.keepDemoMaybe = qtrue;
		cl->pers.stopRecordingTime = level.time + 10000;
		trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"%s\"\n", cl->pers.tempDemoName
			, va("errordemos/%4d-%02d-%02d_%02d-%02d-%02d_%s_client%d_%s",q.tm_year+ 1900,q.tm_mon+1,q.tm_mday,q.tm_hour,q.tm_min,q.tm_sec, sanitizedCourseName,(int)(ent - g_entities),demoname)
		));
	}
		
}

void G_ExecuteClipDemo(int index, qboolean nowait) {
	queuedDemoClip_t* clip = level.queuedDemoClips + index;
	trap_SendConsoleCommand(EXEC_APPEND, clip->cmd);
	if (nowait) {
		clip->state = QDC_INACTIVE;
	}
	else {
		trap_SendConsoleCommand(EXEC_APPEND, va("clipdemodone %d\n", index));
		clip->state = QDC_EXECUTING;
	}
	trap_SendConsoleCommand(EXEC_APPEND, "echo test1\n");
	trap_SendConsoleCommand(EXEC_APPEND, "echo test2\n");
}

void G_SvCmd_ExecuteClipDemoCallback() {

	char arg[20];
	int clipIndex;
	trap_Argv(1, arg, sizeof(arg));
	clipIndex = atoi(arg);
	if (level.queuedDemoClips[clipIndex].state == QDC_EXECUTING) {
		if (g_developer.integer) {
			G_Printf("Demo clip %d successfully confirmed... \n", clipIndex);
		}
		level.queuedDemoClips[clipIndex].state = QDC_INACTIVE;
	}
	else {
		trap_SendServerCommand(-1, "print \"^1Wtf, clip demo execution confirmed, but we don't remember about this demo. This should never happen..\n\"");
		return;
	}
}

void G_CheckEnqueuedClips(qboolean force) {
	int i;
	queuedDemoClip_t* clip = NULL;
	for (i = 0; i < level.maxclients; i++) {
		// reset the queueddemoclips state for each client just to be safe.
		g_entities[i].client->pers.demoClipsPending = qfalse;
	}
	for (i = 0, clip = level.queuedDemoClips; i < MAX_QUEUED_DEMO_CLIPS; i++, clip++) {
		if (clip->state) {
			g_entities[clip->clientNum].client->pers.demoClipsPending = qtrue;
			if (clip->state == QDC_WAITING && (clip->when <= level.time || clip->when > level.time + 99999 || force)) {
				if (g_developer.integer) {
					G_Printf("Executing demo clip %d... \n",i);
				}
				G_ExecuteClipDemo(i, force);
			}
		}
	}
}

void G_EnqueueClipDemo(int clientnum, const char* command, int executionTime) {
	int i;
	int oldestIndex= -1, oldest = INT_MAX;
	queuedDemoClip_t* clip = NULL;
	retry:
	for (i = 0, clip = level.queuedDemoClips; i < MAX_QUEUED_DEMO_CLIPS; i++, clip++) {
		if (clip->state) {
			if (clip->state == QDC_WAITING && (oldestIndex == -1 || clip->when < oldest)) {
				// logically u might think we can discard the executing ones first,
				// but then we'd get an execution confirmation from them and it might confuse our state...
				oldestIndex = i;
				oldest = clip->when;
			}
			continue;
		}

		clip->state = QDC_WAITING;
		clip->clientNum = clientnum;
		clip->when = executionTime;
		Q_strncpyz(clip->cmd,command,sizeof(clip->cmd));
		g_entities[clientnum].client->pers.demoClipsPending = qtrue;
		return;
	}

	if (oldestIndex == -1) {
		trap_SendServerCommand(-1,"print \"^1Failed to enqueue saving clip demo.\n\"");
		return;
	}
	trap_SendServerCommand(-1, "print \"^3Forcing execution of clip demo.\n\"");
	G_ExecuteClipDemo(oldestIndex,qtrue);
	goto retry;

}

void G_FastDBSEffects(gentity_t* ent, float speed, qboolean isReturn) {
	const char* soundFile = "sound/weapons/rocket/lock.wav";
	float shakeIntensity = 2.0f;
	int shakeDuration = 400;
	if (!isReturn) {
		speed *= 0.5f;
	}
	if (speed < 700) {
		return;
	}
	// TODO precache these sounds if they end up causing lag for players similar to connectlag?
	if (speed > 3000) {
		soundFile = "sound/weapons/tie_fighter/tiepass5.wav";
		shakeIntensity = 25.0f;
		shakeDuration = 2000;
	} 
	else if (speed > 2000) {
		soundFile = "sound/weapons/tie_fighter/tieexplode.wav";
		shakeIntensity = 15.0f;
		shakeDuration = 1500;
	}
	else if (speed > 1500) {
		soundFile = "sound/weapons/explosions/explosion_huge3.wav";
		shakeIntensity = 10.0f;
		shakeDuration = 1200;
	}
	else if (speed > 1200) {
		soundFile = "sound/weapons/explosions/explosion_huge2.wav";
		shakeIntensity = 7.5f;
		shakeDuration = 800;
	}
	else if (speed > 1000) {
		soundFile = "sound/weapons/explosions/debrisexplode.wav";
		shakeIntensity = 5.0f;
		shakeDuration = 800;
	}
	else if (speed > 900) {
		soundFile = "sound/weapons/galak/skewerhit.wav";
		shakeIntensity = 4.0f;
		shakeDuration = 800;
	}
	else if (speed > 800) {
		soundFile = "sound/weapons/galak/footstep3.wav";
		shakeIntensity = 3.0f;
		shakeDuration = 600;
	}
	if (!isReturn) {
		shakeIntensity *= 0.5f;
		shakeDuration *= 0.5f;
	}
	if (*soundFile) {
		gentity_t* se = G_Sound(ent, CHAN_AUTO, G_SoundIndex(soundFile));
		se->hideFromActiveRacers = qtrue; // don't bother racers with it
	}
	if (shakeIntensity) {
		G_ScreenShake(ent->client->ps.origin, NULL, shakeIntensity, shakeDuration, qtrue);
	}
}

// only works if sv_demoPreRecord is active and svrecordclip is supported
void G_SaveClipDemo(gentity_t* ent, const char* demoname, const char* clipPrint) {
	gclient_t* cl = ent->client;
	static char sanitizedCourseName[COURSENAME_MAX_LEN + 1];
	static char sanitizedUsername[sizeof(ent->client->sess.login.name)];
	qtime_t q;
	if (!ent->client) {
		return;
	}
	sanitizeFilename(DF_GetCourseName(qfalse), sanitizedCourseName, qfalse); // take care of possible special cahrs the filesystem may not like
	if (ent->client->sess.login.loggedIn) {
		sanitizeFilename(ent->client->sess.login.name, sanitizedUsername, qfalse);
	}
	else {
		Q_strncpyz(sanitizedUsername,"unlogged",sizeof(sanitizedUsername));
	}
	Com_Printf("^2Clip demo requested: %s\n", clipPrint);
	//if (cl->pers.keepDemoMaybe) {
	//	Com_Printf("^1Can't save error demo %s because the game seems to already need the demo elsewhere.\n", demoname);
	//	return;
	//}
	//if (!ent->client->pers.recordingDemo) { // thanks to pre-recording we'll get a bit into the past too
	//	int demoId = DF_GetNewDemoId();

	//	Com_sprintf(cl->pers.tempDemoName, sizeof(cl->pers.tempDemoName), "%stemp/temp%d_%d", level.tempDemoNamePrefix, cl->ps.clientNum, demoId);
	//	cl->pers.recordingDemo = qtrue;

	//	trap_SendConsoleCommand(EXEC_APPEND, va("svdemometa %d dfv %d;svrecord \"%s\" %i\n", cl->ps.clientNum, g_dfv.integer, cl->pers.tempDemoName, cl->ps.clientNum));
	//	cl->pers.demoStartedTime = level.time;
	//}
	//if (cl->pers.tempDemoName[0]) {

	//	//char cvarstr[64];

	//	qtime_t q;
	//	trap_RealTime(&q);

	//	cl->pers.keepDemoMaybe = qtrue;
	//	cl->pers.stopRecordingTime = level.time + 10000;
	//	trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"%s\"\n", cl->pers.tempDemoName
	//		, va("errordemos/%4d-%02d-%02d_%02d-%02d-%02d_%s_client%d_%s", q.tm_year + 1900, q.tm_mon + 1, q.tm_mday, q.tm_hour, q.tm_min, q.tm_sec, sanitizedCourseName, ent - g_entities, demoname)
	//	));
	//}

	trap_RealTime(&q);
	G_EnqueueClipDemo(ent-g_entities, va("svdemometa %d dfv %d;svdemometa %d desc \"%s\";svrecordclip \"%s\" %i 10000;svdemometa %d desc\n", cl->ps.clientNum, g_dfv.integer, cl->ps.clientNum, clipPrint, va("races/clips/%4d-%02d-%02d_%02d-%02d-%02d_%s_client%d_%s_%s", q.tm_year + 1900, q.tm_mon + 1, q.tm_mday, q.tm_hour, q.tm_min, q.tm_sec, sanitizedCourseName, (int)(ent - g_entities), sanitizedUsername, demoname), cl->ps.clientNum, cl->ps.clientNum),level.time+5000);

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
//qboolean DF_InTrigger(vec3_t interpOrigin, gentity_t* trigger)
//{
//	vec3_t	mins, maxs;
//	vec3_t	playerMins, playerMaxs;
//
//	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
//	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);
//
//	VectorAdd(interpOrigin, playerMins, mins);
//	VectorAdd(interpOrigin, playerMaxs, maxs);
//
//	if (trap_EntityContact(mins, maxs, trigger)) return qtrue;
//
//	return qfalse;
//}
qboolean DF_InTrigger(vec3_t interpOrigin, gentity_t* trigger, vec3_t playerMins, vec3_t playerMaxs)
{
	vec3_t	mins, maxs;
	//vec3_t	playerMins, playerMaxs;

	//VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	//VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	VectorAdd(interpOrigin, playerMins, mins);
	VectorAdd(interpOrigin, playerMaxs, maxs);

	if (trap_EntityContact(mins, maxs, trigger)) return qtrue;

	return qfalse;
}
qboolean DF_InAnyTrigger(vec3_t interpOrigin, const char* classname, vec3_t playerMins, vec3_t playerMaxs, gentity_t* activator, int courseId, qboolean ignoreCourseId) // TODO make this more efficient
{
	vec3_t	mins, maxs;
	//vec3_t	playerMins, playerMaxs;
	gentity_t* trigger;

	//VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	//VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	VectorAdd(interpOrigin, playerMins, mins);
	VectorAdd(interpOrigin, playerMaxs, maxs);

	trigger = NULL;
	while ((trigger = G_FindByClassNameFast(trigger, classname)) != NULL) {
		if (/*courseId >=0 &&*/ !ignoreCourseId && trigger->courseID != courseId || trigger->triggerClientSpecific && trigger->parent != activator) continue;
		if (trap_EntityContact(mins, maxs, trigger)) return qtrue;
	}

	return qfalse;
}


#if DEBUG
#define DEBUGTRACETRIGGER 1
#else
#define DEBUGTRACETRIGGER 0
#endif


int DF_InterpolateTouchTimeToOldPosOld(gentity_t* activator, gentity_t* trigger, const char* classname, vec3_t displacementVector, int* warningFlags) // For finish and checkpoint trigger
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
	while ((inTrigger = DF_InAnyTrigger(interpOrigin, classname,activator->client->triggerMins,activator->client->triggerMaxs, activator, trigger->courseID, qfalse)) || !touched)
	{
#if 0
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
#if 0 // this was just debug shit
		else if (!touched) {

			trace_t trace;
			DF_InAnyTrigger(interpOrigin, classname, activator->client->triggerMins, activator->client->triggerMaxs);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->prePmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->postPmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->postPmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->prePmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
		}
#endif
#endif

		lessTime++;
		VectorCopy(interpOrigin, oldInterpOrigin);
		VectorAdd(interpOrigin, delta, interpOrigin);
#if DEBUG
		if (lessTime >= (msecDelta + 100)) break; // just to sanity test a bit
#else
		if (lessTime >= (msecDelta + 1)) break; // if we were forced to go back msecDelta, that would put as at the pre-pmove position. But since race triggers are traced, we are guaranteed to have NOT been in it at the time, so the only way lessTime could be msecDelta or more is if there was some error in the code or floating point imprecision
		// changed this from -1 to +1, so we can produce a warningFlag. we will fix it anyway with a check further down.
#endif
	}
//#if DEBUG
	//assert(lessTime <= msecDelta); // float imprecision could MAYBE, in a freak situation, put as at msecDelta, but definitely no further.
//#endif

	// lessTime should always be lower than msecDelta in a logical world. But... the world isnt always logical.
	// e.g. we can fly a millimeter past a trigger and the intersection area is so small that the millisecond delta steps fail to ever register it here (but trace can)
	// or other stuff?
	// well.. we'll just "punish" the client by giving worst possible lessTime (maximum time addition) since we can't know what really happened here unfortunately.
	// should be pretty rare at least.
	if (lessTime > msecDelta) {
		lessTime = 0;
		VectorClear(displacementVector);
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_OVER;
		trap_SendServerCommand(-1, va("print \"^1client %d, DF_WARNING_INTERPOLATION_FAIL_END_OVER: %d\n\"", (int)(activator - g_entities),lessTime));
	}
	else if (lessTime == msecDelta) {
		lessTime = 0;
		VectorClear(displacementVector);
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_EQUAL;
		trap_SendServerCommand(-1, va("print \"^1client %d, DF_WARNING_INTERPOLATION_FAIL_END_EQUAL: %d\n\"", (int)(activator - g_entities), lessTime));
	}
	else {
		VectorSubtract(oldInterpOrigin, activator->client->postPmovePosition, displacementVector);
	}

	return lessTime;
}


//int DF_InterpolateTouchTimeToOldPos(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean precise) {
int DF_InterpolateTouchTimeToOldPos(gentity_t* activator, gentity_t* trigger, const char* classname, vec3_t displacementVector, int* warningFlags) {
	trace_t trace;
	int clientNum = activator - g_entities;
	int msecDelta = activator->client->ps.commandTime - activator->client->prePmoveCommandTime;
	if (!(coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
		// fallback to old method
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_FALLBACK_NOPRECISE;
		return DF_InterpolateTouchTimeToOldPosOld(activator, trigger, classname, displacementVector, warningFlags);
	}

	memset(&trace, 0, sizeof(trace));
	JP_TracePrecise(&trace, activator->client->prePmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->postPmovePosition, clientNum, CONTENTS_TRIGGER);
	if (trace.entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t* ent = g_entities + trace.entityNum;
		float lessTimePrecise;
		int lessTime;

		if (Q_stricmp(ent->classname,classname) || ent->courseID != trigger->courseID)
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			lessTime = DF_InterpolateTouchTimeToOldPos(activator,trigger,classname,displacementVector,warningFlags);
			ent->r.contents = contents;

			return lessTime;
		}

		if (trace.startsolid || trace.allsolid) {
			*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_STARTSOLID;
			return DF_InterpolateTouchTimeToOldPosOld(activator, trigger, classname, displacementVector, warningFlags);
		}
		else if (trace.fraction >= 1.0) {
			*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_FRACTION1;
			return DF_InterpolateTouchTimeToOldPosOld(activator, trigger, classname, displacementVector, warningFlags);
		}

		lessTimePrecise = (float)msecDelta * (1.0f - trace.fraction);
		lessTime = (int)lessTimePrecise;
#if DEBUGTRACETRIGGER
		{
			int lessTimeCheck = DF_InterpolateTouchTimeToOldPosOld(activator, trigger, classname, displacementVector, warningFlags);
			if (lessTime != lessTimeCheck) {

				trap_SendServerCommand(-1, va("print \"^1DF_InterpolateTouchTimeToOldPos: client %d, lessTime != lessTimeCheck: lessTime %d, lessTimeCheck(legacy) %d, lessTime(float) %f\n\"", (int)(activator - g_entities), lessTime, lessTimeCheck, lessTimePrecise));
			}
		}
#endif
		return lessTime;

	}
	else {
		// uh weird, no hit? try old method?
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_NOHIT;
		return DF_InterpolateTouchTimeToOldPosOld(activator, trigger, classname, displacementVector, warningFlags);
	}

	//if (results->startsolid && start != end)
	//{
	//	trace_t tw;

	//	JP_Trace(&tw, start, mins, maxs, start, passEntityNum, contentmask);
	//	results->startsolid = tw.startsolid;
	//}
}

int DF_InterpolateTouchTimeToOldPosThisTriggerOld(gentity_t* activator, gentity_t* trigger, vec3_t displacementVector) // For finish and checkpoint trigger
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
	while ((inTrigger = DF_InTrigger(interpOrigin, trigger,activator->client->triggerMins,activator->client->triggerMaxs)) || !touched)
	{
#if 0
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
#if 0
		else if (!touched) {

			trace_t trace;
			DF_InAnyTrigger(interpOrigin, classname, activator->client->triggerMins, activator->client->triggerMaxs);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->prePmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->postPmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
			memset(&trace, 0, sizeof(trace));
			JP_TracePrecise(&trace, activator->client->postPmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->prePmovePosition, activator->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
		}
#endif
#endif

		lessTime++;
		VectorCopy(interpOrigin, oldInterpOrigin);
		VectorAdd(interpOrigin, delta, interpOrigin);
#if DEBUG
		if (lessTime >= (msecDelta + 100)) break; // just to sanity test a bit
#else
		if (lessTime >= (msecDelta +1)) break; // if we were forced to go back msecDelta, that would put as at the pre-pmove position. But since race triggers are traced, we are guaranteed to have NOT been in it at the time, so the only way lessTime could be msecDelta or more is if there was some error in the code or floating point imprecision
		// changed this from -1 to +1, so we can produce a warningFlag. we will fix it anyway with a check further down.
#endif
	}
//#if DEBUG
//	assert(lessTime <= msecDelta); // float imprecision could MAYBE, in a freak situation, put as at msecDelta, but definitely no further.
//#endif
// 
	// lessTime should always be lower than msecDelta in a logical world. But... the world isnt always logical.
	// e.g. we can fly a millimeter past a trigger and the intersection area is so small that the millisecond delta steps fail to ever register it here (but trace can)
	// or other stuff?
	// well.. we'll just "punish" the client by giving worst possible lessTime (maximum time addition) since we can't know what really happened here unfortunately.
	// should be pretty rare at least.
	if (lessTime > msecDelta) {
		lessTime = 0;
		VectorClear(displacementVector);

		trap_SendServerCommand(-1, va("print \"^1client %d, checkpoint interpolation over: %d\n\"", (int)(activator - g_entities), lessTime));
	}
	else if (lessTime == msecDelta) {
		lessTime = 0;
		VectorClear(displacementVector);
		trap_SendServerCommand(-1, va("print \"^1client %d, checkpoint interpolation equal\n\"", (int)(activator - g_entities)));
	}
	else {
		VectorSubtract(oldInterpOrigin, activator->client->postPmovePosition, displacementVector);
	}

	return lessTime;
}


//int DF_InterpolateTouchTimeToOldPos(gentity_t* activator, gentity_t* trigger, const char* classname, vec3_t displacementVector, int* warningFlags) {
int DF_InterpolateTouchTimeToOldPosThisTrigger(gentity_t* activator, gentity_t* trigger, vec3_t displacementVector) {
	trace_t trace;
	int clientNum = activator - g_entities;
	int msecDelta = activator->client->ps.commandTime - activator->client->prePmoveCommandTime;
	if (!(coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
		// fallback to old method
		//*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_FALLBACK_NOPRECISE;
		return DF_InterpolateTouchTimeToOldPosThisTriggerOld(activator, trigger,  displacementVector);
	}

	memset(&trace, 0, sizeof(trace));
	JP_TracePrecise(&trace, activator->client->prePmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->postPmovePosition, clientNum, CONTENTS_TRIGGER);
	if (trace.entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t* ent = g_entities + trace.entityNum;
		float lessTimePrecise;
		int lessTime;

		if (ent != trigger)
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			lessTime = DF_InterpolateTouchTimeToOldPosThisTrigger(activator, trigger, displacementVector);
			ent->r.contents = contents;

			return lessTime;
		}

		if (trace.startsolid || trace.allsolid) {
			//*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_STARTSOLID;
			return DF_InterpolateTouchTimeToOldPosThisTriggerOld(activator, trigger,  displacementVector);
		}
		else if (trace.fraction >= 1.0) {
			//*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_FRACTION1;
			return DF_InterpolateTouchTimeToOldPosThisTriggerOld(activator, trigger, displacementVector);
		}

		lessTimePrecise = (float)msecDelta * (1.0f - trace.fraction);
		lessTime = (int)lessTimePrecise;
#if DEBUGTRACETRIGGER
		{
			int lessTimeCheck = DF_InterpolateTouchTimeToOldPosThisTriggerOld(activator, trigger, displacementVector);
			if (lessTime != lessTimeCheck) {

				trap_SendServerCommand(-1, va("print \"^1DF_InterpolateTouchTimeToOldPosThisTrigger: client %d, lessTime != lessTimeCheck: lessTime %d, lessTimeCheck(legacy) %d, lessTime(float) %f\n\"", (int)(activator - g_entities), lessTime, lessTimeCheck, lessTimePrecise));
			}
		}
#endif
		return lessTime;

	}
	else {
		// uh weird, no hit? try old method?
		//*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_NOHIT;
		return DF_InterpolateTouchTimeToOldPosThisTriggerOld(activator, trigger, displacementVector);
	}

}



int DF_InterpolateTouchTimeForStartTimerOld(gentity_t* activator, gentity_t* trigger,vec3_t displacementVector,int* warningFlags) // For start trigger
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
	while (!(inTrigger = DF_InAnyTrigger(interpOrigin,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs, activator, trigger->courseID,qfalse)) || !left)
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
		if (lessTime >= (msecDelta + 1)) break; // if we were forced to go back msecDelta, that would put as at the pre-pmove position. But since race triggers are traced, we are guaranteed to have been in it at the time, so the only way lessTime could be msecDelta or more is if there was some error in the code or floating point imprecision

		// changed this from -1 to +1, so we can produce a warningFlag. we will fix it anyway with a check further down.
#endif
	}
//#if DEBUG
//	assert(lessTime <= msecDelta); // float imprecision could MAYBE, in a freak situation, put as at msecDelta, but definitely no further.
//#endif

	// lessTime should always be lower than msecDelta in a logical world. But... the world isnt always logical.
	// e.g. we can fly a millimeter past a trigger and the intersection area is so small that the millisecond delta steps fail to ever register it here (but trace can)
	// or other stuff?
	// well.. we'll just "punish" the client by giving worst possible lessTime (maximum time addition) since we can't know what really happened here unfortunately.
	// should be pretty rare at least.
	if (lessTime > msecDelta) {
		lessTime = msecDelta;
		VectorSubtract(activator->client->prePmovePosition, activator->client->postPmovePosition, displacementVector);
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_START_OVER;
		trap_SendServerCommand(-1, va("print \"^1client %d, DF_WARNING_INTERPOLATION_FAIL_START_OVER: %d\n\"", (int)(activator - g_entities), lessTime));
	}
	else if (lessTime == msecDelta) {
		VectorSubtract(activator->client->prePmovePosition, activator->client->postPmovePosition, displacementVector);
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_START_EQUAL;
		trap_SendServerCommand(-1, va("print \"^1client %d, DF_WARNING_INTERPOLATION_FAIL_START_EQUAL\n\"", (int)(activator - g_entities)));
	}
	else {
		VectorSubtract(oldInterpOrigin, activator->client->postPmovePosition, displacementVector);
	}

	return lessTime;
}


//int DF_InterpolateTouchTimeToOldPos(gentity_t* activator, gentity_t* trigger, const char* classname, vec3_t displacementVector, int* warningFlags) {
int DF_InterpolateTouchTimeForStartTimer(gentity_t* activator, gentity_t* trigger, vec3_t displacementVector, int* warningFlags){
	trace_t trace;
	int clientNum = activator - g_entities;
	int msecDelta = activator->client->ps.commandTime - activator->client->prePmoveCommandTime;
	if (!(coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
		// fallback to old method
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_FALLBACK_NOPRECISE;
		return DF_InterpolateTouchTimeForStartTimerOld(activator, trigger, displacementVector, warningFlags);
	}

	memset(&trace, 0, sizeof(trace));
	JP_TracePrecise(&trace, activator->client->postPmovePosition, activator->client->triggerMins, activator->client->triggerMaxs, activator->client->prePmovePosition, clientNum, CONTENTS_TRIGGER);
	if (trace.entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t* ent = g_entities + trace.entityNum;
		float lessTimePrecise;
		int lessTime;

		if (Q_stricmp(ent->classname, "df_trigger_start") || ent->courseID != trigger->courseID)
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			lessTime = DF_InterpolateTouchTimeForStartTimer(activator, trigger, displacementVector, warningFlags);
			ent->r.contents = contents;

			return lessTime;
		}

		if (trace.startsolid || trace.allsolid) {
			*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_STARTSOLID;
			return DF_InterpolateTouchTimeForStartTimerOld(activator, trigger, displacementVector, warningFlags);
		}
		else if (trace.fraction >= 1.0) {
			*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_FRACTION1;
			return DF_InterpolateTouchTimeForStartTimerOld(activator, trigger, displacementVector, warningFlags);
		}

		lessTimePrecise = (float)msecDelta * trace.fraction;
		lessTime = (int)lessTimePrecise;
#if DEBUGTRACETRIGGER
		{
			int lessTimeCheck = DF_InterpolateTouchTimeForStartTimerOld(activator, trigger, displacementVector, warningFlags);
			if (lessTime != lessTimeCheck) {

				trap_SendServerCommand(-1, va("print \"^1DF_InterpolateTouchTimeForStartTimer: client %d, lessTime != lessTimeCheck: lessTime %d, lessTimeCheck(legacy) %d, lessTime(float) %f\n\"", (int)(activator - g_entities), lessTime, lessTimeCheck, lessTimePrecise));
			}
		}
#endif
		return lessTime;

	}
	else {
		// uh weird, no hit? try old method?
		*warningFlags |= DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_NOHIT;
		return DF_InterpolateTouchTimeForStartTimerOld(activator, trigger, displacementVector, warningFlags);
	}

}


void DF_HandleUnfinishedDemos() {
	int i;
	gentity_t* ent = g_entities;
	for (i = 0; i < level.maxclients; i++,ent++) {
		if (!ent->inuse) continue;
		if (ent->client->pers.recordingDemo) {

			ent->client->pers.recordingDemo = qfalse;
			ent->client->pers.demoStoppedTime = level.time;
			if (!ent->client->pers.keepDemoMaybe) {
				trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i;svrenamedemo \"%s\" \"%strash/trash%d\"\n", i, ent->client->pers.tempDemoName, level.tempDemoNamePrefix, i));
			}
			else {
				trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i\n", i));
			}
		}
	}
}


// Start race timer
void DF_StartTimer_Leave(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	int	lessTime = 0;
	qboolean segmented = qfalse;
	vec3_t interpolationDisplacement;
	gclient_t* cl;
	mainLeaderboardType_t lbType;
	int resposCountSave, savePosCountSave, startLevelTimeSave;
	int discardCountSave, discardResposSave, discardMaxDepthSave;
	rollState_t rollStateSave;
	int warningFlags = 0;

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

	if (cl->pers.raceStartCommandTime && cl->pers.stats.courseId != ent->courseID) {
		// we are already in a run on another course
		if (g_developer.integer) {
			G_Printf("^3DF_StartTimer_Leave: client %d already on a different course.\n",(int)(activator - g_entities));
		}
		return;
	}

	if ((ent->ttFlags & TTFLAGS_STARTTIMER_Q3RALLYSTYLE) && cl->pers.raceStartCommandTime && cl->pers.stats.q3RallyState.active && cl->pers.stats.q3RallyState.directionInited) {
		// don't retrigger runs that already went through checkpoints
		return;
	}
	if (cl->sess.raceStateInvalidated) {
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Your race state is invalidated. Please respawn before running.",qfalse,qtrue,qtrue, NULL);
		cl->pers.lastRaceTimerStartedCP = level.time;
		return;
	}
	if (cl->sess.login.forceLoggedIn) {
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7You were force-logged in by admin and cannot run. Please change your password with /changepassword, logout and log in again.",qfalse,qtrue,qtrue, NULL);
		return;
	}
	if (cl->sess.raceStateSoftInvalidated) {
		//DF_RaceStateInvalidated(activator,qfalse); // dont reset or it becomes impossible to save spawn on maps with reverse course but without extra spawn
		//if ((cl->pers.lastRaceFinishTime + 1000 > level.time || level.time < cl->pers.lastRaceFinishTime) && !(ent->ttFlags & TTFLAGS_STARTTIMER_Q3RALLYSTYLE)) { // q3 rally: dont bother player with message directly after run finished
		if ((cl->pers.lastRaceFinishTime + 1000 < level.time || level.time < cl->pers.lastRaceFinishTime)) { // dont bother player with message directly after run finished (especially annoying on maps with reverse courses)
			G_CenterPrint(activator - g_entities, 3, "^1Warning: ^7Your race state is soft-invalidated. Please respawn before running.", qfalse, qtrue, qtrue, NULL);
		}
		cl->pers.lastRaceTimerStartedCP = level.time;
		return;
	}


	if ((cl->sess.raceStyle.runFlags & RFL_ANTILOOP) /*&& MovementStyleHasAntiLoop(cl->sess.raceStyle.movementStyle)*/ && cl->pers.antiLoop.yawAngleChangeSinceBaseSpeed > ANTILOOP_MAXYAWCHANGE) {
		if (cl->pers.raceStartCommandTime) {
			G_CenterPrint(activator - g_entities, 3, va("^1ANTI-LOOP: ^7Restart blocked by anti-loop. You turned %.2f degrees (%.2f allowed).", cl->pers.antiLoop.yawAngleChangeSinceBaseSpeed, (float)ANTILOOP_MAXYAWCHANGE), qfalse, qtrue, qfalse, "antiloop restart");
		}
		else {
			G_CenterPrint(activator - g_entities, 3, va("^1ANTI-LOOP: ^7Start blocked by anti-loop. You turned %.2f degrees (%.2f allowed).", cl->pers.antiLoop.yawAngleChangeSinceBaseSpeed, (float)ANTILOOP_MAXYAWCHANGE), qfalse, qtrue, qfalse, "antiloop start");
		}
		cl->pers.lastRaceTimerStartedCP = level.time;
		return;
	}

	segmented = cl->sess.raceStyle.runFlags & RFL_SEGMENTED;

	if (segmented && cl->pers.segmented.state != SEG_RECORDING && cl->pers.segmented.state != SEG_REPLAY) {
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run in a faulty state. Please respawn and try again.",qfalse,qtrue,qtrue, NULL);
		DF_RaceStateInvalidated(activator, qfalse);
		cl->pers.lastRaceTimerStartedCP = level.time;
		return;
	}
	else if (segmented && cl->pers.segmented.state != SEG_REPLAY) {

		if (segmented && cl->pers.segmented.msecProgress > 5000) {
			G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run pre-record is over 5 seconds. Please respawn and try again.",qfalse,qtrue,qfalse, NULL);
			DF_RaceStateInvalidated(activator, qfalse);
			cl->pers.lastRaceTimerStartedCP = level.time;
			return;
		}
		else if (segmented && cl->pers.segmented.msecProgress < 500) {
			G_CenterPrint(activator - g_entities,3, "^1Warning: ^7Segmented run pre-record is under 0.5 seconds. Please respawn and try again.",qfalse,qtrue,qfalse, NULL);
			DF_RaceStateInvalidated(activator, qfalse);
			cl->pers.lastRaceTimerStartedCP = level.time;
			return;
		}
	}

	if (DF_InAnyTrigger(cl->postPmovePosition,"df_trigger_start", activator->client->triggerMins, activator->client->triggerMaxs, activator, -1,qtrue)) return; // we are still in some start trigger.

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Start Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", cl->pers.netname);
		G_CenterPrint(activator - g_entities,3, "^1Warning: ^7No valid pre-pmove info. Please restart.",qfalse,qtrue,qfalse, NULL);
		return;
	}
	else {
		lessTime = DF_InterpolateTouchTimeForStartTimer(activator, ent, interpolationDisplacement, &warningFlags);
	}

	resposCountSave = cl->pers.stats.resposCount;
	discardCountSave = cl->pers.stats.discardCount;
	discardResposSave = cl->pers.stats.discardResposCount;
	discardMaxDepthSave = cl->pers.stats.discardMaxDepth;
	discardCountSave = cl->pers.stats.discardCount;
	savePosCountSave = cl->pers.stats.saveposCount;
	rollStateSave = cl->pers.stats.roll;
	startLevelTimeSave = cl->pers.stats.startLevelTime;
	memset(&cl->pers.stats, 0, sizeof(cl->pers.stats)); // reset & initialize run stats
	if (segmented && cl->pers.segmented.state == SEG_REPLAY) { // remember the amount of savepos/respos used during segmented run
		cl->pers.stats.resposCount = resposCountSave;
		cl->pers.stats.saveposCount = savePosCountSave;
		cl->pers.stats.discardCount = discardCountSave;
		cl->pers.stats.discardResposCount = discardResposSave;
		cl->pers.stats.discardMaxDepth = discardMaxDepthSave;
	}
	cl->pers.stats.startLevelTime = startLevelTimeSave;
	cl->pers.stats.startLessTime = lessTime;
	cl->pers.stats.distanceTraveled = VectorLength(interpolationDisplacement);
	interpolationDisplacement[2] = 0;
	cl->pers.stats.distanceTraveled2D = VectorLength(interpolationDisplacement);
	cl->pers.stats.topSpeed = XYSPEED(cl->ps.velocity);
	cl->pers.stats.courseId = ent->courseID;
	cl->pers.stats.startTriggerSpeed = XYSPEED(cl->ps.velocity);
	cl->pers.stats.warningFlags = warningFlags;
	if (ent->overrideMessage && ent->overrideMessage[0]) {
		Q_strncpyz(cl->pers.stats.overrideMessage, ent->overrideMessage,sizeof(cl->pers.stats.overrideMessage));
	}
	if (ent->ttFlags & TTFLAGS_STARTTIMER_Q3RALLYSTYLE) {
		cl->pers.stats.q3RallyState.active = qtrue;
	}


	// Set timers
	//activator->client->ps.duelTime = activator->client->ps.commandTime - lessTime;
	cl->ps.duelTime = cl->pers.raceStartCommandTime = activator->client->ps.commandTime - lessTime;
	//cl->pers.segmented.lastPosUsed = qfalse; // already guaranteed via SEG_RECORDING check above

	cl->pers.lastRaceFinishTime = 0;

	if (segmented) {
		if (cl->pers.segmented.state != SEG_REPLAY) {
			cl->pers.segmented.totalStartCommandTime = cl->pers.raceStartCommandTime;
		} // else keep the old value
	}
	else {
		cl->pers.segmented.totalStartCommandTime = 0;
	}

	if ((cl->pers.raceStartCommandTime - rollStateSave.lastRollEndedTime) < 1000) {
		// roll ended less than 1 second before run start, its probably part of the run. can we do this smarter?
		cl->pers.stats.roll = rollStateSave;
	}

	memset(&cl->pers.raceDropped,0,sizeof(cl->pers.raceDropped)); // reset info aabout packets dropped due to wrong fps timing



	//if (GetTimeMS() - cl->pers.stats.startTime < 500)//Some built in floodprotect per player?
		//return;
	//if (cl->pers.stats.startTime) //Instead of floodprotect, dont let player start a timer if they already have one.  Mapmakers should then put reset timers over the start area.
		//return;

	//trap->Print("Actual trigger touch! time: %i\n", GetTimeMS());

	if (cl->pers.recordingDemo && cl->pers.keepDemoMaybe) {
		//We are still recording a demo that we want to keep? -shouldn't ever happen?
		//Stop and rename it (renaming happens automatically no worries)
		trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i\n", cl->ps.clientNum));
		cl->pers.recordingDemo = qfalse;
		cl->pers.demoStoppedTime = level.time;
	}

	//in rename demo, also make sure demo is stopped before renaming? that way we dont have to have the ;wait 20; here

	//if ((g_defragAutoDemo.integer) && (!cl->pers.noFollow || (cl->sess.movementStyle == MV_SIEGE) || (g_allowNoFollow.integer > 2)) && !(cl->pers.practice) && cl->sess.raceMode && !sv_cheats.integer && cl->pers.userName[0]) {
	if ((g_defragAutoDemo.integer) && cl->sess.raceMode && !g_cheats.integer) {
		if (!cl->pers.recordingDemo) { //Start the new demo
			int demoId = DF_GetNewDemoId();

			Com_sprintf(cl->pers.tempDemoName, sizeof(cl->pers.tempDemoName), "%stemp/temp%d_%d", level.tempDemoNamePrefix, cl->ps.clientNum, demoId);
			cl->pers.recordingDemo = qtrue;
			//trap_SendServerCommand( player-g_entities, "chat \"RECORDING STARTED\"");
			//trap_SendConsoleCommand(EXEC_APPEND, va("svrecord %s/%s %i\n", cl->sess.login.loggedIn ? "temp":"tempanon", cl->sess.login.loggedIn ? cl->sess.login.name : miniva("anon%d",activator-g_entities), cl->ps.clientNum));



			trap_SendConsoleCommand(EXEC_APPEND, va("svdemometa %d dfv %d;svrecord \"%s\" %i\n", cl->ps.clientNum, g_dfv.integer, cl->pers.tempDemoName, cl->ps.clientNum));
			cl->pers.demoStartedTime = level.time;
		}
		else { //Check if we should "restart" the demo
			if (!cl->pers.stats.startLevelTime && (!cl->pers.demoStartedTime || (level.time > (cl->pers.demoStartedTime + 1000) || level.time < cl->pers.demoStartedTime)) || (level.time - cl->pers.stats.startLevelTime > 5000 || level.time < cl->pers.stats.startLevelTime)) { // don't restart demo unless (if already within run) 5 seconds have passed since touching start trigger or (if not already within run) the demo started recording at least 1 second ago. to avoid restarting demo 100 times per second.
				int demoId = DF_GetNewDemoId(); 
				char		tempDemoName[MAX_OSPATH];

				Com_sprintf(tempDemoName, sizeof(tempDemoName), "%stemp/temp%d_%d", level.tempDemoNamePrefix, cl->ps.clientNum, demoId);
				cl->pers.recordingDemo = qtrue;
				cl->pers.demoStoppedTime = level.time;
				//trap_SendServerCommand( player-g_entities, "chat \"RECORDING RESTARTED\"");
				trap_SendConsoleCommand(EXEC_APPEND, va("svdemometa %d dfv %d;svstoprecord %i;svrenamedemo \"%s\" \"%strash/trash%d\";svrecord \"%s\" %i\n", cl->ps.clientNum, g_dfv.integer, cl->ps.clientNum, cl->pers.tempDemoName, level.tempDemoNamePrefix, cl->ps.clientNum, tempDemoName, cl->ps.clientNum));
				Q_strncpyz(cl->pers.tempDemoName, tempDemoName,sizeof(cl->pers.tempDemoName));
				cl->pers.demoStartedTime = level.time;
			}
		}
	}


	cl->pers.stats.startLevelTime = level.time;

	//cl->lastStartTime = level.time;
	cl->pers.keepDemoMaybe = qfalse;


	if (!cl->sess.login.loggedIn) {
		G_CenterPrint(activator - g_entities, 3, va("^%cRace timer started! ^1Warning: Not logged in.",lbType == LB_MAIN ? '7':'O'), qfalse, qtrue, qfalse, "racestarted unlogged");
	}
	else if (segmented && level.nonDeterministicEntities) {
		G_CenterPrint(activator - g_entities,3, va("^%cRace timer started! ^1Warning: ^7Map has %i non-deterministic entities. Replay/run may fail.", lbType == LB_MAIN ? '7' : 'O', level.nonDeterministicEntities),qfalse, qtrue,qfalse, "racestarted nondeterm");
	}
	else {
		G_CenterPrint(activator - g_entities,3, va("^%cRace timer started!", lbType == LB_MAIN ? '7' : 'O'),qfalse,qtrue,qfalse, "racestarted normal");
	}
	cl->pers.lastRaceTimerStartedCP = level.time;
}


//qboolean ValidRaceSettings(int restrictions, gentity_t* player)
qboolean ValidRaceSettings(gentity_t* player)
{ //How 2 check if cvars were valid the whole time of run.. and before? since you can get a headstart with higher g_speed before hitting start timer? :S
	//Make most of this hardcoded into racemode..? speed, knockback, debugmelee, stepslidefix, gravity
	int style;
	gclient_t* cl = player->client;
	if (!cl)
		return qfalse;

	if (!cl->ps.stats[STAT_RACEMODE])
		return qfalse;


	style = cl->sess.raceStyle.movementStyle;

	if (style == MV_CSS)
		return qfalse;//work in progress
	if (style == MV_Q2 && (g_q2trace.integer != 1 || g_q2Skims.integer)) // i decided against the q2 style trace. i just couldnt make it work right. but lets still use the right epsilon. 0 = normal trace. 1 = q2 epsilon. 2 = q2 style
		return qfalse;//work in progress

	//if (cl->sess.accountFlags & JAPRO_ACCOUNTFLAG_NORACE)
	//	return qfalse;
	//if ((style == MV_RJQ3 || style == MV_RJCPM || style == MV_TRIBES) && g_knockback.value != 1000.0f)
	//	return qfalse;

	if (cl->sess.raceStyle.jumpLevel >= 0) {

		if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] != cl->sess.raceStyle.jumpLevel) {
			return qfalse; // shouldnt happen
		}
	}
	else if (cl->sess.raceStyle.jumpLevel < -1){
		return qfalse; // shouldnt happen
	}
	else {
		if (cl->ps.powerups[PW_YSALAMIRI] != INT_MAX) {
			return qfalse; // shouldnt happen
		}
	}

	//if (style != MV_CPM && style != MV_OCPM && style != MV_Q3 && style != MV_WSW && style != MV_RJQ3 && style != MV_RJCPM && style != MV_JETPACK && style != MV_SWOOP && style != MV_JETPACK && style != MV_SLICK && style != MV_BOTCPM && style != MV_COOP_JKA && style != MV_TRIBES) { //Ignore forcejump restrictions if in onlybhop movement modes
	//	if (restrictions & (1 << 0)) {//flags 1 = restrict to jump1
	//		if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] != 1 || cl->ps.powerups[PW_YSALAMIRI] > 0) {
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 1!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//	else if (restrictions & (1 << 1)) {//flags 2 = restrict to jump2
	//		if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] != 2 || cl->ps.powerups[PW_YSALAMIRI] > 0) {
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//	else if (restrictions & (1 << 2)) {//flags 4 = only jump3
	//		if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] != 3 || cl->ps.powerups[PW_YSALAMIRI] > 0) { //Also dont allow ysal in FJ specified courses..?
	//			trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
	//			return qfalse;
	//		}
	//	}
	//}
	//else if (style == MV_COOP_JKA) {
	//	if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] == 2 && !(restrictions & (1 << 1))) {//using jump2 but its not allowed
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course does not allow force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
	//		return qfalse;
	//	}
	//	if (cl->ps.fd.forcePowerLevel[FP_LEVITATION] == 3 && !(restrictions & (1 << 2))) {//using jump3 but its not allowed
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: this course does not allow force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
	//		return qfalse;
	//	}
	//}

	//if (cl->pers.haste && !(restrictions & (1 << 3)))
	//	return qfalse; //IF client has haste, and the course does not allow haste, dont count it.
	//if (((style != MV_JETPACK) && (style != MV_TRIBES)) && (cl->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)) && !(restrictions & (1 << 4))) //kinda deprecated.. maybe just never allow jetpack?
	//	return qfalse; //IF client has jetpack, and the course does not allow jetpack, dont count it.
	//if (style == MV_SWOOP && !cl->ps.m_iVehicleNum)
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
	if (g_debugMelee.integer >= 2 && (cl->sess.raceStyle.runFlags & RFL_CLIMBTECH))
		return qfalse;
	if (!g_smoothClients.integer)// why?
		return qfalse;
	if (g_kickoffFix.integer != 1)
		return qfalse;
	//if (sv_fps.integer != 20 && sv_fps.integer != 30 && sv_fps.integer != 40)//Dosnt really make a difference.. but eh.... loda fixme
	if (g_sv_fps.integer != 100)// Does this even matter for tommyternal? everything runs on clienttime anyway. well... but demos wouldnt be proper without it, so leave it.
		return qfalse;
	if (g_sv_gameFps.integer && g_sv_gameFps.integer != 100)// Does this even matter for tommyternal? everything runs on clienttime anyway. 
		return qfalse;
	if (g_sv_gameFpsAllowIrregular.integer)// Does this even matter for tommyternal? everything runs on clienttime anyway. 
		return qfalse;
	//if (sv_pluginKey.integer) {
	//	if (!cl->pers.validPlugin && cl->pers.userName[0]) { //Meh.. only do this if they are logged in to keep the print colors working right i guess..
	//		trap->SendServerCommand(player - g_entities, "cp \"^3Warning: a newer client plugin version\nis required!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
	//		return qfalse;
	//	}
	//}
	//if (cl->pers.noFollow && (cl->sess.movementStyle != MV_SIEGE) && (g_allowNoFollow.integer < 3))
	//	return qfalse;
	//if (cl->pers.practice)
	//	return qfalse;
	//if ((restrictions & (1 << 5)) && (level.gametype == GT_CTF || level.gametype == GT_CTY))//spawnflags 32 is FFA_ONLY
	//	return qfalse;
	//if ((cl->ps.stats[STAT_RESTRICTIONS] & JAPRO_RESTRICT_ALLOWTELES) && !(restrictions & (1 << 6))) //spawnflags 64 on end trigger is allow_teles
	//	return qfalse;

	return qtrue;
}

// japro thing. weird?
void PlayActualGlobalSound(int soundindex) {
	gentity_t* player;
	int i;

	//G_AddEvent(ent, EV_GLOBAL_SOUND, soundindex); //need to svf_broadcast firsT? and what ent to use ??

	for (i = 0; i < level.maxclients; i++) {//Build a list of clients
		if (!g_entities[i].inuse)
			continue;
		player = &g_entities[i];
		G_Sound(player, CHAN_AUTO, soundindex);
	}
}

qboolean DF_RemoveCheckPoints(gentity_t* playerent) {
	int i;
	gentity_t* shield;
	int removed = 0;

	for (i = 0; i < playerent->client->pers.df_checkpointData.count; i++) {
		shield = g_entities + playerent->client->pers.df_checkpointData.checkpointNumbers[i];
		if (shield->inuse) {
			G_FreeEntity(shield);
			removed++;
		}
	}

	playerent->client->pers.df_checkpointData.count = 0;

	return (qboolean)(removed > 0);
}

void DF_RemoveCheckPoints_Cmd(gentity_t* playerent) {
	DF_RemoveCheckPoints(playerent);
}

void DF_ClearCheckPointTimes(gentity_t* playerent) {
	gentity_t* checkpoint = NULL;
	int clientNum = playerent - g_entities;
	while (checkpoint = G_FindByClassNameFast(checkpoint, "df_trigger_checkpoint")) {
		memset(&checkpoint->checkpointTimes[clientNum],0,sizeof(checkpoint->checkpointTimes[clientNum]));
		memset(&checkpoint->checkpointTimesSegNonReplay[clientNum],0,sizeof(checkpoint->checkpointTimesSegNonReplay[clientNum]));
	}
}

// sanity check that the client still exists and knows about this checkpoint andsuch.
void df_checkCheckpointValid(gentity_t* ent) {
	gentity_t* owner;
	int i;
	int shieldNum;

	if (ent->s.owner < 0 || ent->s.owner >= level.maxclients) {
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
	G_SetClassName(shield, "df_trigger_checkpoint");

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
	char	arg[10];
	int sourcePlayer = -1;
	//gentity_t* sourcePlayerEnt;
	if (trap_Argc() > 1)
	{
		trap_Argv(1, arg, sizeof(arg));

		if (arg[0] >= '0' && arg[0] <= '9')
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
	G_SetClassName(shield, "df_trigger_checkpoint");

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

void DF_CreateCustomCheckpoint_Cmd(gentity_t* playerent) {
	DF_CreateCustomCheckpoint(playerent);
}

const char* DF_FormatFpsString(char* rawFpsString) {
	static char fpsString[40];
	char* start;
	char* end = rawFpsString;
	int fps;
	int percent;
	int index = 0;
	fpsString[0] = '\0';

	while (*end == ' ') {
		end++;
	}
	start = end;
	while (qtrue) {

		while (*end >= '0' && *end <= '9') {
			end++;
		}
		if (*end != ':') break;
		*end = '\0';
		fps = atoi(start);
		end++;
		start = end;
		while (*end >= '0' && *end <= '9') {
			end++;
		}
		if (*end != '%') break;
		*end = '\0';
		percent = atoi(start);
		end++;
		if (percent >= 5) {
			Q_strcat(fpsString, sizeof(fpsString), miniva("%s%d:%d*/.", index != 0 ? "," : "", fps, percent));
		}
		if (strlen(fpsString) >= 30) {
			break;
		}
		if (*end != ',') break;
		end++;
		if (!*end) break;
		start = end;
	}
	return fpsString;
}

/*
=================
Cmd_Top_f
=================
*/
void DF_TopRequest(gentity_t* ent, const char* coursename, const char* subcoursename, int page, int style, topRequestType_t type, mainLeaderboardType_t lbTypeIfSpecific, raceStyle_t* thisMapDefaultRaceStyle)
{
	topScoresRequestStruct_t data = { 0 };
	int countLBs = LB_TYPES_COUNT;
	const char* mainLBWhere = getLeaderboardSQLConditions(LB_MAIN, thisMapDefaultRaceStyle);
	const char* mainLBNJBWhere = getLeaderboardSQLConditions(LB_NOJUMPBUG, thisMapDefaultRaceStyle);
	const char* customLBWhere = getLeaderboardSQLConditions(LB_CUSTOM, thisMapDefaultRaceStyle);
	const char* segmentedLBWhere = getLeaderboardSQLConditions(LB_SEGMENTED, thisMapDefaultRaceStyle);
	const char* cheatLBWhere = getLeaderboardSQLConditions(LB_CHEAT, thisMapDefaultRaceStyle);

	data.clientnum = ent - g_entities;
	data.type = type;
	data.lbTypeIfSpecific = lbTypeIfSpecific;
	data.style = style;
	Q_strncpyz(data.course, coursename, sizeof(data.course));
	Q_strncpyz(data.subcourse, subcoursename, sizeof(data.subcourse));

	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(data.clientnum, "print \"Top results request failed, database version too low.\n\"");
		return;
	}

	data.mapDefaultRaceStyle = *thisMapDefaultRaceStyle;

	page = MAX(0, page - 1);

	data.page = page;

#define REALRANK "ROW_NUMBER() OVER (PARTITION BY userid=-1 ORDER BY besttime ASC, runwhen ASC) AS realrank" // unlogged ones shouldn't count and this way we can get the proper ones

#define TOPCOLUMNS "users.username,runs_pre.besttime,runs_pre.userid, runs_pre.runFlags, msec, jump, topspeed, average, runwhen, saveposCount, resposCount, duration_ms_segmented_total, fpsString, " REALRANK ",distance,discardCount"
	//#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime,MIN(runwhen) OVER (PARTITION BY userid) AS earliest FROM runs  WHERE course=? AND style=? AND variant=? AND %s ) runs_pre"
#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime FROM runs  WHERE hidden=0 AND course=? AND subcourse=? AND style=? AND variant=? AND %s ) runs_pre"
//#define QUERY2 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id WHERE earliest=runwhen AND besttime=duration_ms GROUP BY userid ORDER BY besttime ASC LIMIT 11"
#define QUERY2 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id WHERE besttime=duration_ms GROUP BY userid ORDER BY besttime ASC, runwhen ASC LIMIT ?,11"

	// TODO what if, for freak reason, someone has two identical times in two different styles? how do i select the earlier one? or should i even care?  earliest=runwhen AND besttime=duration_ms doesnt work cuz not both are neccessarily true

	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	//if (type == TOPREQUEST_SPECIFICLB) {
	//	if (G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_TOP,
	//		va(
	//			"(SELECT 0 AS type," TOPCOLUMNS QUERY2 " )" // limit 11 cuz want unofficial too, even tho we show it separately.
	//			"UNION ALL (SELECT 1 AS type," TOPCOLUMNS QUERY2 " )"
	//			"UNION ALL (SELECT 2 AS type," TOPCOLUMNS QUERY2 " )"
	//			"UNION ALL (SELECT 3 AS type," TOPCOLUMNS QUERY2 " )"
	//			"UNION ALL (SELECT 4 AS type," TOPCOLUMNS QUERY2 " )"
	//			, mainLBWhere, mainLBNJBWhere, customLBWhere, segmentedLBWhere, cheatLBWhere))) {
	//		int i;
	//		for (i = 0; i < countLBs; i++) {
	//			G_COOL_API_DB_PreparedBindString(coursename);
	//			G_COOL_API_DB_PreparedBindString(subcoursename);// subcourse
	//			G_COOL_API_DB_PreparedBindInt(style);
	//			G_COOL_API_DB_PreparedBindInt(0);
	//			G_COOL_API_DB_PreparedBindInt(page * 10);
	//		}
	//		G_COOL_API_DB_FinishAndSendPreparedStatement();
	//	}
	//	else {
	//		trap_SendServerCommand(data.clientnum, "print \"Top results request failed, database connection not available.\n\"");
	//	}
	//}
	//else 
	{
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
				G_COOL_API_DB_PreparedBindString(coursename);
				G_COOL_API_DB_PreparedBindString(subcoursename);// subcourse
				G_COOL_API_DB_PreparedBindInt(style);
				G_COOL_API_DB_PreparedBindInt(0);
				G_COOL_API_DB_PreparedBindInt(page * 10);
			}
			G_COOL_API_DB_FinishAndSendPreparedStatement();
		}
		else {
			trap_SendServerCommand(data.clientnum, "print \"Top results request failed, database connection not available.\n\"");
		}
	}

#undef QUERY2
#undef TOPCOLUMNS

}

void DF_UpdateRanks(gentity_t* ent, const char* coursename, const char* subcoursename, raceStyle_t* thisMapDefaultRaceStyle, qboolean flush)
{
	rankUpdateRequestStruct_t data = { 0 };
	int countLBs = LB_TYPES_COUNT;
	const char* mainLBWhere = getLeaderboardSQLConditions(LB_MAIN, thisMapDefaultRaceStyle);
	const char* mainLBNJBWhere = getLeaderboardSQLConditions(LB_NOJUMPBUG, thisMapDefaultRaceStyle);
	const char* customLBWhere = getLeaderboardSQLConditions(LB_CUSTOM, thisMapDefaultRaceStyle);
	const char* segmentedLBWhere = getLeaderboardSQLConditions(LB_SEGMENTED, thisMapDefaultRaceStyle);
	const char* cheatLBWhere = getLeaderboardSQLConditions(LB_CHEAT, thisMapDefaultRaceStyle);
	int style;

	data.clientnum = ent ? ent - g_entities : -1;

	if (coolApi_dbVersion < 3) {
		G_SendOrPrint(ent,"Rank update request failed, database version too low.\n");
		return;
	}

	Q_strncpyz(data.course,coursename,sizeof(data.course));
	Q_strncpyz(data.subcourse, subcoursename,sizeof(data.subcourse));

#define REALRANKRUN "IF(besttime=duration_ms AND userid!=-1,ROW_NUMBER() OVER (PARTITION BY userid=-1,duration_ms=besttime ORDER BY besttime ASC, runwhen ASC),NULL) AS realrank" // unlogged ones shouldn't count and this way we can get the proper ones
#define TOPCOLUMNS2 "runs_pre.id AS runId, " REALRANKRUN 
#define QUERY3 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id ORDER BY besttime ASC, runwhen ASC"

	if (data.clientnum != -1) {
		memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	}
		
	for (style = 0; style < MV_NUMSTYLES; style++) {
		data.style = style;
		data.flush = (style == MV_NUMSTYLES - 1) && flush;
		if (G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_RANKUPDATE,
			va(
				"UPDATE runs INNER JOIN "
				"("
				"(SELECT 0 AS type," TOPCOLUMNS2 QUERY3 " )" // limit 11 cuz want unofficial too, even tho we show it separately.
				"UNION ALL (SELECT 1 AS type," TOPCOLUMNS2 QUERY3 " )"
				"UNION ALL (SELECT 2 AS type," TOPCOLUMNS2 QUERY3 " )"
				"UNION ALL (SELECT 3 AS type," TOPCOLUMNS2 QUERY3 " )"
				"UNION ALL (SELECT 4 AS type," TOPCOLUMNS2 QUERY3 " )"
				") AS rankdata "
				" ON (rankdata.runId = runs.id) "
				" SET runs.tmpRank = rankdata.realrank, runs.tmpLB=rankdata.type "
				, mainLBWhere, mainLBNJBWhere, customLBWhere, segmentedLBWhere, cheatLBWhere))) {
			int i;
			for (i = 0; i < countLBs; i++) {
				G_COOL_API_DB_PreparedBindString(coursename);
				G_COOL_API_DB_PreparedBindString(subcoursename);// subcourse
				G_COOL_API_DB_PreparedBindInt(style);
				G_COOL_API_DB_PreparedBindInt(0);
			}
			G_COOL_API_DB_FinishAndSendPreparedStatement();
		}
		else {
			G_SendOrPrint(ent, "Rank update request failed, database connection not available.\n\n");
		}
	}


#undef TOPCOLUMNS2
#undef RUNSPRE
#undef QUERY3
#undef REALRANK
}

void DF_UpdateRanksMainRequest(gentity_t* requesterOrNull,const char* courseNameOrNull, qboolean forceAll, int limitCount) {

	rankUpdateMapRequestStruct_t data;

	memset(&data, 0, sizeof(data));

	if (requesterOrNull) {
		data.clientnum = requesterOrNull - g_entities;
		memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	}
	else {
		data.clientnum = -1;
	}

	data.mapCountLimit = limitCount;

#define RANKMAPQUERY_DATELIMITED "SELECT COUNT(runs.id) as runCount, runs.course, runs.subcourse, ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound, mapdefaults.msec, mapdefaults.jump, mapdefaults.variant, mapdefaults.runFlags, MAX(runs.runwhen) AS latestRunWhen, meta.valueWhen FROM runs LEFT JOIN mapdefaults ON (mapdefaults.course = runs.course AND mapdefaults.subcourse = runs.subcourse) LEFT JOIN meta ON (`meta`.`key` = 'rankUpdateLatest') GROUP BY runs.course, runs.subcourse HAVING (meta.valueWhen IS NULL OR meta.valueWhen < latestRunWhen) ORDER BY latestRunWhen ASC"
#define RANKMAPQUERY "SELECT COUNT(runs.id) as runCount, runs.course,runs.subcourse,ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound,mapdefaults.msec,mapdefaults.jump,mapdefaults.variant,mapdefaults.runFlags,MAX(runs.runwhen) AS latestRunWhen FROM runs LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) GROUP BY runs.course,runs.subcourse ORDER BY latestRunWhen ASC"
#define RANKMAPQUERYSEARCH "SELECT COUNT(runs.id) as runCount, runs.course,runs.subcourse,ISNULL(mapdefaults.runFlags) AS mapdefaultsNotFound,mapdefaults.msec,mapdefaults.jump,mapdefaults.variant,mapdefaults.runFlags,MAX(runs.runwhen) AS latestRunWhen FROM runs LEFT JOIN mapdefaults ON (mapdefaults.course=runs.course AND mapdefaults.subcourse=runs.subcourse) WHERE runs.course=?  GROUP BY runs.course,runs.subcourse ORDER BY latestRunWhen ASC"

	if (!courseNameOrNull) {
		level.lastAllRankUpdate = level.time;
		data.all = qtrue;
		if (forceAll) {
			if (!G_COOL_API_DB_AddRequest((byte*)&data, sizeof(data), DBREQUEST_RANKUPDATEMAPREQUEST, RANKMAPQUERY)) {
				G_SendOrPrint(requesterOrNull, "^1Error sending rank update map request query.\n");
			}
		}
		else {
			if (!G_COOL_API_DB_AddRequest((byte*)&data, sizeof(data), DBREQUEST_RANKUPDATEMAPREQUEST, RANKMAPQUERY_DATELIMITED)) {
				if (g_developer.integer) {
					G_SendOrPrint(requesterOrNull, "^1Error sending date-limited rank update map request query.\n");
				}
			}
		}
	}
	else {
		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_RANKUPDATEMAPREQUEST, RANKMAPQUERYSEARCH)) {
			G_SendOrPrint(requesterOrNull, "^1Error sending course-specific rank update map request query.\n");
			return;
		}
		G_COOL_API_DB_PreparedBindString(courseNameOrNull);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}

	// dont do this directly to not get confused by multi-course maps with different styles
	// TODO same for top?
	//DF_UpdateRanks(ent, thisMapName, mainSubCourseName, &level.mapDefaultRaceStyle);
}

/*
=================
DF_TimeRequest
=================
*/
void DF_TimeRequest(gentity_t* ent, const char* coursename, const char* subcoursename, int style, qboolean forUserinfo)
{
	timeRequestStruct_t data={ 0 };
	mainLeaderboardType_t lbType = LB_MAIN;
	raceStyle_t* raceStyle = &ent->client->sess.raceStyle;
	gclient_t* cl = ent->client;
	const char* lbWhere = NULL;

	if (cl->sess.raceMode && !forUserinfo) {
		lbType = classifyLeaderBoard(raceStyle, &level.mapDefaultRaceStyle);
	}
	lbWhere = getLeaderboardSQLConditions(lbType, &level.mapDefaultRaceStyle);

	data.clientnum = ent - g_entities;
	if (!cl->sess.login.loggedIn) {
		trap_SendServerCommand(data.clientnum, "print \"Cannot request time when not logged in.\n\"");
		return;
	}
	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(data.clientnum, "print \"Time request failed, database version too low.\n\"");
		return;
	}

	data.forUserInfo = forUserinfo;
	data.style = style;
	data.raceStyle = cl->sess.raceStyle;
	data.lbType = forUserinfo ? MV_JK2 : lbType;
	Q_strncpyz(data.course, coursename, sizeof(data.course));
	Q_strncpyz(data.subcourse, subcoursename, sizeof(data.subcourse));

#define TOPCOLUMNS "runs_pre.besttime,runs_pre.runFlags, msec, jump"
	//#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime,MIN(runwhen) OVER (PARTITION BY userid) AS earliest FROM runs  WHERE course=? AND style=? AND variant=? AND %s ) runs_pre"
#define RUNSPRE "(SELECT *,MIN(duration_ms) OVER (PARTITION BY userid) AS besttime FROM runs  WHERE course=? AND subcourse=? AND style=? AND variant=? AND userid=? AND %s ) runs_pre"
//#define QUERY2 " FROM " RUNSPRE " LEFT JOIN users ON runs_pre.userid=users.id WHERE earliest=runwhen AND besttime=duration_ms GROUP BY userid ORDER BY besttime ASC LIMIT 11"
#define QUERY2 " FROM " RUNSPRE " WHERE besttime=duration_ms GROUP BY userid ORDER BY besttime ASC, runwhen ASC LIMIT 1"

	// TODO what if, for freak reason, someone has two identical times in two different styles? how do i select the earlier one? or should i even care?  earliest=runwhen AND besttime=duration_ms doesnt work cuz not both are neccessarily true

	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	if (G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_TIME,
		va(
			"SELECT " TOPCOLUMNS QUERY2 
			, lbWhere))) {
		//int i;
		G_COOL_API_DB_PreparedBindString(coursename);
		G_COOL_API_DB_PreparedBindString(subcoursename);// subcourse
		G_COOL_API_DB_PreparedBindInt(style);
		G_COOL_API_DB_PreparedBindInt(raceStyle->variant);
		G_COOL_API_DB_PreparedBindInt(cl->sess.login.id);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		trap_SendServerCommand(data.clientnum, "print \"Time request failed, database connection not available.\n\"");
	}

#undef TOPCOLUMNS
#undef RUNSPRE
#undef QUERY2
}

int DF_GetSegmentedRunnerCount() {
	int segReplays = 0;
	gentity_t* oEnt;
	int i;

	for (i = 0; i < level.maxclients; i++) {
		oEnt = g_entities + i;

		// extend this to any segmented runner? but how to avoid trolling?
		if (oEnt->client->sess.raceMode && (oEnt->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && oEnt->client->pers.segmented.state == SEG_REPLAY && oEnt->client->pers.connected == CON_CONNECTED) {
			segReplays++;
		}
	}
	return segReplays;
}

// stripColor is only for printing and such. generrally leave it at qfalse unless you know what you're doing
const char* DF_GetCourseName(qboolean stripColor) { 
	static char serverInfo[BIG_INFO_STRING];
	static char course[COURSENAME_MAX_LEN + 1];
	char* s = course;
	trap_GetServerinfo(serverInfo, sizeof(serverInfo));
	Q_strncpyz(course, Info_ValueForKey(serverInfo, "mapname"), sizeof(course));
	while (*s) { // make it lowercase
		*s = tolower(*s);
		s++;
	}
	if (stripColor) { // only for printing and such
		Q_StripColor(course);
	}
	return course;
}


const char* DF_GetMainSubcourseName() {
	int i;
	static char subcourse[COURSENAME_MAX_LEN + 1];
	subcourse[0] = '\0';
	if (level.numCourses >= 1) {
		for (i = 0; i < level.numCourses; i++) {
			if (!level.courseName[i][0]) {
				// if we find an empty name subcourse, we default to that one.
				subcourse[0] = '\0';
				break;
			}
			else if (i == 0) {
				Q_strncpyz(subcourse, level.courseName[i], sizeof(subcourse));
			}
		}
	}
	return subcourse;
}


void DF_RequestPlayerDefaultTime(gentity_t* ent) {

	DF_TimeRequest(ent, DF_GetCourseName(qfalse), DF_GetMainSubcourseName(), MV_JK2, qtrue);
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

		Q_strncpyz(messageStr, va("%s^%c%19s ^3%12s^%c  ^3%7.2f^%cmax ^3%7.2f^%cavg ^3%7.1fk^%cdist ^3%2i^%cj ^3%4s^%cfps %s ^%c%s",
			prefix,
			color,
			miniva("[%s/%s]", moveStyleNames[runInfo->raceStyle.movementStyle].string,leaderboardNames[runInfo->lbType].string),
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
			runFlagsString,
			color,
			runInfo->subcoursename[0] ? miniva("(^3%s^%c) ", runInfo->subcoursename, color) : ""), sizeof(messageStr));
		//Q_strncpyz(awardString, va("%s ^%i[^%i%s^%i]", runInfo->netname, color, runInfo->userId == -1 ? 1 : nameColor, runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username, color), sizeof(awardString));
		//if (message)
		//Com_sprintf(messageStr, sizeof(messageStr), "^3%-16s^%i", runInfo->coursename, color);
		//else
		//	Com_sprintf(messageStr, sizeof(messageStr), "^%iCompleted", color);

		if (runInfo->raceStyle.runFlags & RFL_SEGMENTED) { //print number of teles?
			//if (level.clients[clientNum].midRunTeleCount < 1)
			//	Q_strcat(messageStr, sizeof(messageStr), " (PRO)");
			//else
			if (runInfo->discardCount) {
				Q_strcat(messageStr, sizeof(messageStr), va("(^3%i^%c SP, ^3%i^%c RP, ^3%i^%c D) ", runInfo->savePosCount, color, runInfo->resposCount, color, runInfo->discardCount, color));
			}
			else {
				Q_strcat(messageStr, sizeof(messageStr), va("(^3%i^%c SP, ^3%i^%c RP) ", runInfo->savePosCount, color, runInfo->resposCount, color));
			}
		}
		Q_strcat(messageStr, sizeof(messageStr), va("by^7 %s  ^%c[^%c%s^%c] ", runInfo->netname, color, runInfo->userId == -1 ? '1' : nameColor, runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username, color));
		

		trap_SendServerCommand(-1, va("print \"%s\n\" %s %s", messageStr, type, DF_RacePrintAppendage(runInfo)));

		if (g_crossServerDefragTimes.integer > 1) {
			G_SendCrossServerCommand(va("defragPrint \"^l>^j%s^l>^7:%s\n\" %s_crossServer %s", DF_GetCourseName(qtrue), messageStr, type, DF_RacePrintAppendage(runInfo)));
		}

		if(ent && !preliminary)
			G_CenterPrint(ent - g_entities, 3, va("^7%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
	}
	else if (runInfo->rankLB != -1) {
		// todo what if i DONT get a pb but its still wr compared to other users?
		if (runInfo->rankLB == 1 && (runInfo->pbStatus & PB_LB)) { //was 1 when it shouldnt have been.. ?
			Q_strncpyz(messageStr, va("%s^%c%19s^7 %s ^%c[^%c%s^%c] %sbeat the ^3%s%s^%c%s and %s ranked ^3#%i\n",
				prefix,
				color,
				miniva("[%s/%s]", moveStyleNames[runInfo->raceStyle.movementStyle].string, leaderboardNames[runInfo->lbType].string),
				runInfo->netname,color, runInfo->userId == -1 ? '1' : nameColor,
				runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username,
				color, 
				runInfo->userId == -1 ? "unofficially " : "",
				moveStyleNames[runInfo->raceStyle.movementStyle].string,
				runInfo->lbType == LB_MAIN ? " WORLD RECORD" : miniva("/%s world record", leaderboardNames[runInfo->lbType].string),
				color,
				runInfo->subcoursename[0] ? miniva(" (^3%s^%c)", runInfo->subcoursename, color) : "",
				runInfo->userId == -1 ? "would be " : "is now",
				runInfo->rankLB), 
				sizeof(messageStr));
			if (runInfo->userId != -1) {
				//G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass"));
				if (runInfo->lbType == LB_MAIN) {
					PlayActualGlobalSound(G_SoundIndex("sound/movers/sec_panel_pass"));
					if (ent) {
						if (runInfo->raceStyle.movementStyle == MV_JK2) {
							gentity_t* shakeEnt = G_ScreenShake(ent->client->ps.origin, NULL, 5.0f, 800, qtrue);
							shakeEnt->parent = ent;
						}
						else {
							gentity_t* shakeEnt = G_ScreenShake(ent->client->ps.origin, ent, 5.0f, 800, qfalse);
						}
					}
				}
			}

			if(ent && !preliminary)
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
		}
		else if ((runInfo->pbStatus & PB_LB)) {
			Q_strncpyz(messageStr, va("%s^%c%19s^7 %s ^%c[^%c%s^%c] %s a new %s%s %s best%s and %s ranked ^3#%i\n",
				prefix,
				color,
				miniva("[%s/%s]", moveStyleNames[runInfo->raceStyle.movementStyle].string, leaderboardNames[runInfo->lbType].string),
				runInfo->netname, 
				color, 
				runInfo->userId == -1 ? '1' : nameColor, 
				runInfo->userId == -1 ? "!^7unlogged^1!" : runInfo->username,
				color,
				runInfo->userId == -1 ? "set" : "got",
				moveStyleNames[runInfo->raceStyle.movementStyle].string,
				runInfo->lbType == LB_MAIN ? "" : miniva("/%s", leaderboardNames[runInfo->lbType].string),
				runInfo->userId == -1 ? "unlogged" : "personal",
				runInfo->subcoursename[0] ? miniva(" (^3%s^%c)", runInfo->subcoursename, color) : "",
				runInfo->userId == -1 ? "would be " : "is now", 
				runInfo->rankLB), 
				sizeof(messageStr));
			if (runInfo->rankLB <= 10 && runInfo->lbType == LB_MAIN && ent && !preliminary) {
				G_ScreenShake(ent->client->ps.origin, ent, 2.5f, 800, qfalse); // pb: shake a bit less. in NT, the distinction wasnt in intensity but WR would shake everyone. but lets not confuse other players who might be on track for WR too
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
			}
			if (runInfo->rankLB <= 10 && ent && !preliminary) {
				G_CenterPrint(ent - g_entities, 3, va("^2%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
			}
			else if(ent && !preliminary){
				G_CenterPrint(ent - g_entities, 3, va("^5%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
			}
		}
		else if(ent && !preliminary){
			G_CenterPrint(ent - g_entities, 3, va("^7%s", DF_MsToString(runInfo->milliseconds)), qfalse, qtrue, qfalse, NULL);
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

		if (g_crossServerDefragTimes.integer > 1) {
			if (messageStr[0]) {
				G_SendCrossServerCommand(va("defragPrint \"^l>^j%s^l>^7:%s\" %s_crossServer %s", DF_GetCourseName(qtrue), messageStr, type, DF_RacePrintAppendage(runInfo)));
			}
			else {
				// dont prepend if its empty anyway
				G_SendCrossServerCommand(va("defragPrint \"%s\" %s_crossServer %s", messageStr, type, DF_RacePrintAppendage(runInfo)));
			}
		}

		trap_SendServerCommand(-1, va("print \"%s\" %s %s", messageStr,type, DF_RacePrintAppendage(runInfo)));
	}


	//trap_SendServerCommand(-1, va("print \"^2%s\n\"", runInfo->fpsString));
	
}


typedef struct fpsEntry_s {
	int fps;
	int count;
} fpsEntry_t;

int compareFpsEntry(const void* a, const void* b) {
	return ((fpsEntry_t*)b)->count - ((fpsEntry_t*)a)->count;
}

void DF_MakeUsedFpsString(runFpsStats_t* fps, char* buf, int bufSize) {
	int i,value,index=0;
	int count = 0;
	fpsEntry_t entries[FPSTABLE_SIZE];

	if (!fps->totalCount) {
		buf[0] = '\0';
		return;
	}

	for (i = 0; i < FPSTABLE_SIZE; i++) {
		value = 100 * fps->msecCounts[i] / fps->totalCount;
		if (value < 1) continue; 
		entries[count].fps = 1000 / fpsTableIndexToMsec[i];
		entries[count].count = fps->msecCounts[i];
		count++;
	}

	if (!count) {
		buf[0] = '\0';
		return;
	}

	qsort(entries, count, sizeof(fpsEntry_t), compareFpsEntry);

	buf[0] = '\0';
	for (i = 0; i < count; i++) {
		value = 100 * entries[i].count / fps->totalCount;
		if (value < 1) continue;
		Q_strcat(buf, bufSize, miniva("%c%d:%d%%", index == 0 ? ' ' : ',', entries[i].fps, value));
		index++;
	}
}


static void DF_FillClientRunInfo(finishedRunInfo_t* runInfo, gentity_t* ent, int milliseconds, gentity_t* endtrigger) {
	static char serverInfo[BIG_INFO_STRING];
	//static char course[COURSENAME_MAX_LEN + 1];
	gclient_t* client = ent->client;
	char* s;
	if (!client || !client->sess.raceMode) return;
	runInfo->clientNum = ent - g_entities;
	Q_strncpyz(runInfo->netname, client->pers.netname, sizeof(runInfo->netname));
	if (client->sess.login.loggedIn && !client->sess.login.forceLoggedIn) {
		runInfo->userId = client->sess.login.id;
		Q_strncpyz(runInfo->username, client->sess.login.name, sizeof(runInfo->username));
	}
	else {
		runInfo->userId = -1;
		Q_strncpyz(runInfo->username, "!unlogged!", sizeof(runInfo->username));
	}
	runInfo->raceStyle = client->sess.raceStyle;
	//if (!MovementStyleHasAntiLoop(runInfo->raceStyle.movementStyle)) {
	//	runInfo->raceStyle.runFlags &= ~RFL_ANTILOOP; // remove antiloop from runs that dont even qualify
	//}
	//if (runInfo->raceStyle.movementStyle == MV_CSS || runInfo->raceStyle.movementStyle == MV_Q2) {
	//	runInfo->raceStyle.runFlags &= ~RFL_BOT; // bot doesnt work for these atm so may as well remove that.
	//	runInfo->raceStyle.runFlags &= ~RFL_CLIMBTECH; // climbtech doesnt work for these atm so may as well remove that.
	//	runInfo->raceStyle.runFlags &= ~RFL_JUMPBUGDISABLE; // not meaningful for these atm
	//	runInfo->raceStyle.runFlags &= ~RFL_NOROLLS; // not meaningful
	//	runInfo->raceStyle.runFlags &= ~RFL_NOROLLSTART; // not meaningful
	//	if (runInfo->raceStyle.movementStyle == MV_CSS) {
	//		runInfo->raceStyle.runFlags &= ~RFL_NODEADRAMPS; // not yet implemented
	//	}
	//}

	if (runInfo->raceStyle.movementStyle == MV_CSS || runInfo->raceStyle.movementStyle == MV_Q2) {
		//runInfo->raceStyle.jumpLevel = 1;
	}
	runInfo->raceStyle.runFlags &= ~MovementStyleDisabledRunFlags(runInfo->raceStyle.movementStyle);

	runInfo->lbType = classifyLeaderBoard(&runInfo->raceStyle, &level.mapDefaultRaceStyle);;
	trap_GetServerinfo(serverInfo, sizeof(serverInfo));
	Q_strncpyz(runInfo->coursename, Info_ValueForKey(serverInfo, "mapname"), sizeof(runInfo->coursename));
	s = runInfo->coursename;
	while (*s) {
		*s = tolower(*s);
		s++;
	}

	runInfo->checksumBsp = g_cm_checksumBsp.integer; // needs engine support but it's not essential anyway, so if it's just 0 in the end, not a big loss.
	runInfo->checksumPak = g_cm_checksumPak.integer;

	if (endtrigger->message) {
		Q_strncpyz(runInfo->subcoursename, endtrigger->message, sizeof(runInfo->subcoursename));
	}
	else if (client->pers.stats.overrideMessage[0]) {
		Q_strncpyz(runInfo->subcoursename, client->pers.stats.overrideMessage, sizeof(runInfo->subcoursename));
	}
	else if (client->pers.stats.q3RallyState.active && client->pers.stats.q3RallyState.isReverse) {
		Q_strncpyz(runInfo->subcoursename, "reverse", sizeof(runInfo->subcoursename));
	}
	else {
		runInfo->subcoursename[0] = '\0';
	}
	s = runInfo->subcoursename;
	while (*s) {
		*s = tolower(*s);
		s++;
	}

	if (client->pers.recordingDemo) {
		Q_strncpyz(runInfo->tempDemoName, client->pers.tempDemoName, sizeof(runInfo->tempDemoName));
	}
	else {
		runInfo->tempDemoName[0] = '\0';
	}

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
	runInfo->discardCount = client->pers.stats.discardCount;
	runInfo->discardRespos = client->pers.stats.discardResposCount;
	runInfo->discardMaxDepth = client->pers.stats.discardMaxDepth;
	runInfo->startTriggerSpeed = client->pers.stats.startTriggerSpeed;
	DF_MakeUsedFpsString(&client->pers.stats.fpsStats, runInfo->fpsString, sizeof(runInfo->fpsString));
	if (client->pers.stats.roll.status == ROLL_ENDED) {
		runInfo->rollSpeed = client->pers.stats.roll.rollSpeed;
		runInfo->rollTakeoffClientSpeed = client->pers.stats.roll.finalAirClientSpeed;
	}
	else {
		runInfo->rollTakeoffClientSpeed = runInfo->rollSpeed = 0;
	}
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
		"\"%d-%d-%d\" " // discardCount, discardResposCount, discardMaxDepth // placeHolder1
		"%d " // placeHolder2
		"%d " // placeHolder3
		"%d " // placeHolder4
		"%d " // millisecondsSegmentedTotal
		"\"%f\" " // rollSpeed
		"%d " // rollTakeoffClientSpeed
		"\"%f\" " // startTriggerSpeed
		"%d " // isPB
		"%d " // rankLB
		"\"%s\" " // coursename[COURSENAME_MAX_LEN + 1]
		"\"%s\" " // username[USERNAME_MAX_LEN + 1]
		"%d " // unixTimeStampShifted
		"%d " // unixTimeStampShiftedBillionCount
		"%d " // lbType
		"\"%s\" " // subcoursename[COURSENAME_MAX_LEN + 1]
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
		,runInfo->discardCount, runInfo->discardRespos, runInfo->discardMaxDepth
		//,runInfo->placeHolder1
		,runInfo->placeHolder2
		,runInfo->placeHolder3
		,runInfo->placeHolder4
		,runInfo->millisecondsSegmentedTotal
		,runInfo->rollSpeed
		,runInfo->rollTakeoffClientSpeed
		,runInfo->startTriggerSpeed
		,runInfo->pbStatus
		,runInfo->rankLB
		,runInfo->coursename
		,runInfo->username
		,runInfo->unixTimeStampShifted
		,runInfo->unixTimeStampShiftedBillionCount
		,runInfo->lbType
		,runInfo->subcoursename
		);
}

// Stop race timer
void DF_FinishTimer_Touch(gentity_t* ent, gentity_t* activator, trace_t* trace)
{
	gclient_t* cl;
	int	timeLast, timeBest,newRaceBestTime, lessTime = 0;
	int timeSegmentedTotal = 0;
	//char timeLastStr[32];// , timeBestStr[32];
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


	if (ent->spawnflags & SF_FINISHTIMER_REQUIRE_SPECIFIC_STARTTRIGGER && ent->courseID != cl->pers.stats.courseId) {
		//if (time > 2000 && (cl->randomLastCenterprint < level.time)) {
		//	trap_SendServerCommand(player - g_entities, "cp \"^3Warning: you are on the wrong course!\n\n\n\n\n\n\n\n\n\n\""); //Print the checkpoint(s) its missing?
		//	cl->randomLastCenterprint = level.time + 1000;
		//}
		// just silently ignore this. who needs this error message, its just confusing.
		return;
	}
	//Com_Printf("Flag: %i, Objective %i, player objectives %i\n", restrictions, trigger->objective, player->client->pers.stats.checkpoints);
		//If player has MORe checkpoints than the end trigger requires, that also fails.  Fix this?
	if (ent->spawnflags & SF_FINISHTIMER_REQUIRE_CHECKPOINTS && (ent->objective & cl->pers.stats.checkpoints) != ent->objective) {
		//if (/*time > 2000 &&*/ (cl->randomLastCenterprint < level.time)) {//sad hack to avoid spamming people who just start run(trigger on opposite side of start)
		//	G_CenterPrint(activator - g_entities,3, "^3Warning: you are missing some required checkpoints!",qfalse,qtrue,qfalse); //Print the checkpoint(s) its missing?
		//	cl->randomLastCenterprint = level.time + 1000;
		//}
		// just silently ignore this. who needs this error message, its just confusing/annoying.
		return;
	}

	if (ent->ttFlags & TTFLAGS_FINISHTIMER_Q3RALLYSTYLE && cl->pers.stats.q3RallyState.active) {
		if (!cl->pers.stats.q3RallyState.directionInited) {
			return; // we just went through start trigger
		}
		else if(cl->pers.stats.q3RallyState.isReverse && cl->pers.stats.q3RallyState.lastCheckpoint != 1) {
			if (level.time - cl->randomLastCenterprint > 1000 || level.time < cl->randomLastCenterprint) {
				cl->randomLastCenterprint = level.time;
				G_CenterPrint(activator - g_entities, 3, va("^1You haven't passed all checkpoints yet (reverse direction): %d/%d", level.q3r_numCheckpoints-cl->pers.stats.q3RallyState.lastCheckpoint+1, level.q3r_numCheckpoints), qfalse, qtrue, qfalse, NULL);
				return;
			}
		}
		else if(!cl->pers.stats.q3RallyState.isReverse && cl->pers.stats.q3RallyState.lastCheckpoint != level.q3r_numCheckpoints) {
			if (level.time - cl->randomLastCenterprint > 1000 || level.time < cl->randomLastCenterprint) {
				cl->randomLastCenterprint = level.time;
				G_CenterPrint(activator - g_entities, 3, va("^1You haven't passed all checkpoints yet: %d/%d", cl->pers.stats.q3RallyState.lastCheckpoint, level.q3r_numCheckpoints), qfalse, qtrue, qfalse, NULL);
				return;
			}
		}
	}

	// TODO implement silent flag?
	if (ent->ttFlags & TTFLAGS_FINISHTIMER_SCOREREQUIRE) { // cl->pers.stats.score is specific to runs! not our normal score
		if (cl->pers.stats.score < ent->checkpointScore) {
			//if (cl->pers.stats.score || !(ent->ttFlags & TTFLAGS_FINISHTIMER_Q3RALLYSTYLE)) { // q3 rally: dont bother the player when hes starting/ending. nvm we do separate handling now
				if (level.time - cl->randomLastCenterprint > 1000 || level.time < cl->randomLastCenterprint) {
					cl->randomLastCenterprint = level.time;
					G_CenterPrint(activator - g_entities, 3, va("^1Your checkpoint score is too low: %d/%d", cl->pers.stats.score, ent->checkpointScore), qfalse, qtrue, qfalse, NULL);
				}
			//}
			return;
		}
		else if (ent->ttFlags & TTFLAGS_FINISHTIMER_SCOREREQUIRE_MATCH && cl->pers.stats.score != ent->checkpointScore) {
			if (level.time - cl->randomLastCenterprint > 1000 || level.time < cl->randomLastCenterprint) {
				cl->randomLastCenterprint = level.time;
				G_CenterPrint(activator - g_entities, 3, va("^1Your checkpoint score does not match: %d/%d", cl->pers.stats.score, ent->checkpointScore), qfalse, qtrue, qfalse, NULL);
			}
			return;
		}
	}

	if (!ValidRaceSettings(activator) || !trap_InPVS(cl->ps.origin, cl->ps.origin)) {// out of bounds fix? does this need extra checks due to trace/interpolation?
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}

	warningFlags = cl->pers.stats.warningFlags;

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Finish Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", cl->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid pre-pmove info.\n\"", cl->pers.netname));
		warningFlags |= DF_WARNING_INVALID_PREPMOVE;
	}
	else {
		lessTime = DF_InterpolateTouchTimeToOldPos(activator, ent, "df_trigger_finish", interpolationDisplacement, &warningFlags);
	}

	cl->pers.stats.distanceTraveled -= VectorLength(interpolationDisplacement);
	interpolationDisplacement[2] = 0;
	cl->pers.stats.distanceTraveled2D -= VectorLength(interpolationDisplacement);

	// Set info
	if (cl->sess.raceStyle.runFlags & RFL_SEGMENTED) {
		if (cl->pers.segmented.state == SEG_REPLAY) {
			timeSegmentedTotal = cl->pers.segmented.totalDurationMinusReplay;
		}
		else {
			timeSegmentedTotal = cl->pers.segmented.totalDurationMinusReplay = cl->ps.commandTime - lessTime - cl->pers.segmented.totalStartCommandTime;
		}
	}
	else {
		timeSegmentedTotal = 0;
	}
	timeLast = cl->ps.commandTime - lessTime - cl->pers.raceStartCommandTime;
	timeBest = !cl->pers.raceBestTime ? timeLast : cl->pers.raceBestTime;

	memset(&runInfo, 0, sizeof(runInfo));
	DF_FillClientRunInfo(&runInfo, activator, timeLast, ent); // fills various stats we collected from start trigger and across run, and some metadata
	runInfo.runId = DF_GetNewRunId(); // what was the point of this again? oh yeah to associate results.
	runInfo.endLessTime = lessTime;
	runInfo.levelTimeEnd = level.time;
	runInfo.endCommandTime = cl->ps.commandTime - lessTime;
	runInfo.warningFlags = warningFlags;
	runInfo.millisecondsSegmentedTotal = timeSegmentedTotal;

	Q_strncpyz(cl->pers.lastSubcourseFinishedName, runInfo.subcoursename, sizeof(cl->pers.lastSubcourseFinishedName));

	//Q_strncpyz(timeLastStr, DF_MsToString(timeLast), sizeof(timeLastStr));
	//Q_strncpyz(timeBestStr, DF_MsToString(timeBest), sizeof(timeBestStr));

	if (g_defragArenaAutoGen.integer && !level.hasArenaInfo) {
		level.mustGenerateArena = qtrue;
	}

	if ((cl->sess.raceStyle.runFlags & RFL_SEGMENTED) && cl->pers.segmented.state != SEG_REPLAY) {
		//trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "has finished the segmented race in %f units [^2%s^7]: ^1Estimate! Starting rerun now.\n\" dfsegprelim %s", cl->pers.netname, cl->pers.stats.distanceTraveled, timeLastStr, DF_RacePrintAppendage(&runInfo))); // extra params: type runId clientNum milliseconds leveltimeend endcommandtime endInterpolationReduction warningFlags top average distance username

		PrintRaceTime(&runInfo, qtrue, qfalse, activator);
		DF_StartSegmentedReplay(activator,qfalse);
		//cl->pers.segmented.state = SEG_REPLAY;
		//cl->pers.segmented.playbackStartedTime = level.time;
		//cl->pers.segmented.playbackStartedCommandTimeOffset = cl->ps.commandTime - level.time;
		//cl->pers.segmented.playbackNextCmdIndex = 0;
		//if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
		//	// during replay, we are providing usercmds for server to send to spectators and player for demos
		//	activator->r.svFlags |= SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
		//}
		//activator->s.eFlags |= EF_SEGMENTEDREPLAY;
		//cl->ps.eFlags |= EF_SEGMENTEDREPLAY;
		//cl->ps.duelTime = cl->pers.raceStartCommandTime = 0;
		//cl->pers.stats.startLevelTime = 0;

		ClientInactivitySpecTimerReset(activator, 10000); // just to be safe?
		return;
	}

	if (timeLast <= 0) {
		runInfo.warningFlags |= DF_WARNING_INVALIDRUNTIME;
		PrintRaceTime(&runInfo, qfalse, qfalse, activator);
		trap_SendServerCommand(-1, va("print \"^1Run invalid. Race time under or equal to 0 milliseconds: %d.\n\"",timeLast));
		DF_SaveErrorDemo(activator,multiva("raceMillisecondsInvalid%d",timeLast), multiva("Race millisecond time invalid: %d", timeLast));
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}
	if (runInfo.millisecondsSegmentedTotal < 0) {
		runInfo.warningFlags |= DF_WARNING_INVALIDRUNTIME;
		PrintRaceTime(&runInfo, qfalse, qfalse, activator);
		trap_SendServerCommand(-1, va("print \"^1Run invalid. Segmented total time under 0 milliseconds: %d.\n\"",runInfo.millisecondsSegmentedTotal));
		DF_SaveErrorDemo(activator,multiva("raceSegTotalMillisecondsInvalid%d", runInfo.millisecondsSegmentedTotal), multiva("Race segmented total millisecond time invalid: %d", runInfo.millisecondsSegmentedTotal));
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}
	if (runInfo.distance <= 0) {
		runInfo.warningFlags |= DF_WARNING_INVALIDRUNDISTANCE;
		PrintRaceTime(&runInfo, qfalse, qfalse, activator);
		trap_SendServerCommand(-1, va("print \"^1Run invalid. Distance is under or equal to 0 units: %f.\n\"",runInfo.distance));
		DF_SaveErrorDemo(activator,multiva("raceDistanceInvalid%d", (int)(1000.0f * runInfo.distance)), multiva("Distance invalid: %f", runInfo.distance));
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}
	if (runInfo.distanceXY <= 0) {
		runInfo.warningFlags |= DF_WARNING_INVALIDRUNDISTANCE;
		PrintRaceTime(&runInfo, qfalse, qfalse, activator);
		trap_SendServerCommand(-1, va("print \"^1Run invalid. 2D Distance is under or equal to 0 units: %f.\n\"",runInfo.distanceXY));
		DF_SaveErrorDemo(activator,multiva("race2DDistanceInvalid%d", (int)(1000.0f*runInfo.distanceXY)), multiva("2D Distance invalid: %f", runInfo.distanceXY));
		DF_RaceStateInvalidated(activator, qtrue);
		return;
	}
	
	ClientInactivitySpecTimerReset(activator, 10000); // dont send a client to spec directly after finishing a run if he's afk (mostly related to segmented runs)

	//isInserting = G_InsertRun(activator, timeLast,0,0,0, warningFlags, level.time, runId, cl->ps.commandTime - lessTime);
	isInserting = G_InsertRun(&runInfo);

	cl->pers.keepDemoMaybe = qtrue;
	cl->pers.stopRecordingTime = level.time + 10000;

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
	if (RaceStyleIsMainLeaderboard(&runInfo.raceStyle,&level.mapDefaultRaceStyle) && !Q_stricmp(DF_GetMainSubcourseName(),runInfo.subcoursename)) {
		// todo load player's best time when logging in
		newRaceBestTime = timeLast > timeBest ? timeBest : timeLast;
		if (cl->pers.raceBestTime != newRaceBestTime) {
			cl->pers.raceBestTime = newRaceBestTime;
			// Update client
			CalculateRanks();
			ClientUserinfoChanged(activator - g_entities);
		}
	}

	// Reset timers
	//cl->ps.duelTime = 0;
	cl->ps.duelTime = cl->pers.raceStartCommandTime = 0;
	cl->pers.stats.startLevelTime = 0;
	cl->pers.lastRaceFinishTime = level.time;
	cl->sess.raceStateSoftInvalidated = qtrue; // require respawn before starting a run again. but still allow saving spawns and such
}

// Checkpoint race timer
void DF_CheckpointTimer_Touch(gentity_t* trigger, gentity_t* activator, trace_t* trace) // TODO Make this only trigger on first contact
{
	gclient_t* cl;
	vec3_t interpolationDisplacement;
	int	timeCheck, lessTime=0;
	checkpointTime_t* bestTime;
	int nowTime = LEVELTIME(activator->client);
	char scoreAddExtraText[50];

	if (trigger->parent && trigger->parent != activator) return; // belongs to someone else

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

	if (trigger->spawnflags & SF_CHECKPOINT_RESET_PLAYER_TIMER) { //Instead of a checkpoint, make it reset their time (they went out of bounds or something)
		DF_RaceStateInvalidated(activator, qtrue);
		// make it a soft reset. can still save a spawn after.
		// this can't be used to cheat yourself out of an invalidated state because we already check above that raceStateInvalidated is false.
		activator->client->sess.raceStateInvalidated = qfalse; 
		return;
	}
	if (!(trigger->spawnflags & SF_CHECKPOINT_UNSET_CHECKPOINT) && trigger->objective > 0 && ((cl->pers.stats.checkpoints & trigger->objective) == trigger->objective)) {
		return;
	}
	if (trigger->objective > 0) {  //Bitvalue of the checkpoint Todo, need to print times
		//int i;// , val;

		if (trigger->spawnflags & SF_CHECKPOINT_UNSET_CHECKPOINT) {
			cl->pers.stats.checkpoints &= ~trigger->objective;
			return; //Todo, notify the client or?
		}
		else
			cl->pers.stats.checkpoints |= trigger->objective;
		
		// japro maybe add back in later idk
		//for (i = 0; i < 32; i++) {
		//	val = (1 << i);
		//	if (val == trigger->objective) {
		//		mandatoryCheckpoint = i + 1;
		//		break;
		//	}
		//}
	}

	//if (nowTime - activator->client->pers.raceLastCheckpointTime < 1000) return; // don't spam. // already handled via triggerLastPlayerContact and this way checkpoints can be less than 1s apart if needed.

	// we ideally only wanna display checkpoints if the player didn't touch them last frame.
	// doesn't matter for finish triggers as much since they end runs the first time they are touched.
	if (nowTime - trigger->triggerLastPlayerContact[activator-g_entities] < 1000) return;

	if (!DF_PrePmoveValid(activator)) {
		Com_Printf("^1Defrag Checkpoint Trigger Warning:^7 %s ^7didn't have valid pre-pmove info.", activator->client->pers.netname);
		trap_SendServerCommand(-1, va("print \"^1Warning:^7 %s ^7didn't have valid checkpoint pre-pmove info.\n\"", activator->client->pers.netname));
	}
	else {
		//lessTime = DF_InterpolateTouchTimeToOldPos(activator, trigger, "df_trigger_checkpoint", interpolationDisplacement);
		lessTime = DF_InterpolateTouchTimeToOldPosThisTrigger(activator, trigger, interpolationDisplacement);
	}

	scoreAddExtraText[0] = '\0';
	if (trigger->checkpointScore) {
		if (!(trigger->ttFlags & TTFLAGS_CHECKPOINTTIMER_SCOREONCE) || !cl->entityStates[trigger -g_entities]) {
			cl->pers.stats.score += trigger->checkpointScore;
			Q_strncpyz(scoreAddExtraText, va("\n^7Checkpoint score ^%c%s%d: %d", trigger->checkpointScore > 0 ? '3' : '1', trigger->checkpointScore > 0 ? "+" : "", trigger->checkpointScore, cl->pers.stats.score), sizeof(scoreAddExtraText));
			cl->entityStates[trigger - g_entities] = 1;
		}
	}
	
	if ((trigger->ttFlags & TTFLAGS_CHECKPOINTTIMER_Q3RALLYSTYLE) && cl->pers.stats.q3RallyState.active) {
		if (!cl->pers.stats.q3RallyState.directionInited) {
			if (trigger->number == level.q3r_numCheckpoints) {
				cl->pers.stats.q3RallyState.isReverse = qtrue;
				cl->pers.stats.q3RallyState.directionInited = qtrue;
				cl->pers.stats.q3RallyState.lastCheckpoint = trigger->number;
				cl->pers.stats.courseId = 1;
				Q_strncpyz(scoreAddExtraText, va("\n^7Checkpoint ^%c%d^%c/%d\n^3[reverse]", '3', 1, '7',level.q3r_numCheckpoints), sizeof(scoreAddExtraText));
			}
			else if (trigger->number == 1) {
				cl->pers.stats.q3RallyState.isReverse = qfalse;
				cl->pers.stats.q3RallyState.directionInited = qtrue;
				cl->pers.stats.q3RallyState.lastCheckpoint = trigger->number;
				cl->pers.stats.courseId = 0;
				Q_strncpyz(scoreAddExtraText, va("\n^7Checkpoint ^%c%d^%c/%d", '3', 1, '7', level.q3r_numCheckpoints), sizeof(scoreAddExtraText));
			}
			else {
				Q_strncpyz(scoreAddExtraText, "\n^1You missed a checkpoint!", sizeof(scoreAddExtraText));
			}
		}
		else if (cl->pers.stats.q3RallyState.lastCheckpoint == trigger->number) {
			// just touched it twice, ignore.
		}
		else if(cl->pers.stats.q3RallyState.isReverse && trigger->number == (cl->pers.stats.q3RallyState.lastCheckpoint-1)) {
			Q_strncpyz(scoreAddExtraText, va("\n^7Checkpoint ^%c%d^%c/%d\n^3[reverse]", trigger->number == 1 ? '2':'3', level.q3r_numCheckpoints-trigger->number+1, trigger->number == 1 ? '2' : '7', level.q3r_numCheckpoints), sizeof(scoreAddExtraText));
			cl->pers.stats.q3RallyState.lastCheckpoint = trigger->number;
		}
		else if(!cl->pers.stats.q3RallyState.isReverse && trigger->number == (cl->pers.stats.q3RallyState.lastCheckpoint+1)) {
			Q_strncpyz(scoreAddExtraText, va("\n^7Checkpoint ^%c%d^%c/%d", trigger->number == level.q3r_numCheckpoints ? '2':'3', trigger->number, trigger->number == 1 ? '2' : '7', level.q3r_numCheckpoints), sizeof(scoreAddExtraText));
			cl->pers.stats.q3RallyState.lastCheckpoint = trigger->number;
		}
		else {
			Q_strncpyz(scoreAddExtraText, "\n^1You missed a checkpoint!", sizeof(scoreAddExtraText));
		}
	}

	// Set info
	timeCheck = activator->client->ps.commandTime - lessTime - activator->client->pers.raceStartCommandTime;

	bestTime = ((cl->sess.raceStyle.runFlags & RFL_SEGMENTED) && cl->pers.segmented.state < SEG_REPLAY) ? &trigger->checkpointTimesSegNonReplay[activator - g_entities] : &trigger->checkpointTimes[activator - g_entities]; // in segmented run mode, non-replay checkpoint times 

	if (bestTime->time == 0)
	{
		G_CenterPrint(activator-g_entities,3, va("^2Checkpoint activated\n%s%s", DF_MsToString(timeCheck), scoreAddExtraText),qfalse,qtrue,qfalse, "cptimer activated");
		bestTime->time = timeCheck;
		bestTime->raceStyle = cl->sess.raceStyle;
		bestTime->courseId = cl->pers.stats.courseId;
	}
	else if (bestTime->courseId != cl->pers.stats.courseId) // last time logged on this checkpoint was a different course
	{
		G_CenterPrint(activator-g_entities,3, va("^2Different course, checkpoint reset\n%s%s", DF_MsToString(timeCheck), scoreAddExtraText),qfalse,qtrue,qfalse, "cptimer reset diffcourse");
		bestTime->time = timeCheck;
		bestTime->raceStyle = cl->sess.raceStyle;
		bestTime->courseId = cl->pers.stats.courseId;
	}
	else if (memcmp(&bestTime->raceStyle, &cl->sess.raceStyle,sizeof(bestTime->raceStyle))) // last time logged on this checkpoint was a different style
	{
		G_CenterPrint(activator-g_entities,3, va("^2Style changed, checkpoint reset\n%s%s", DF_MsToString(timeCheck), scoreAddExtraText),qfalse,qtrue,qfalse, "cptimer reset diffstyle");
		bestTime->time = timeCheck;
		bestTime->raceStyle = cl->sess.raceStyle;
		bestTime->courseId = cl->pers.stats.courseId;
	}
	else if (timeCheck <= bestTime->time)
	{
		G_CenterPrint(activator - g_entities, 3, va("%s\n^2-%s%s\n \n \n \n ", DF_MsToString(timeCheck), DF_MsToString(abs(timeCheck - bestTime->time)), scoreAddExtraText),qfalse,qtrue,qfalse, "cptimer pb");
		bestTime->time = timeCheck;
		bestTime->raceStyle = cl->sess.raceStyle;
		bestTime->courseId = cl->pers.stats.courseId;
	}
	else
	{
		G_CenterPrint(activator - g_entities,3, va("%s\n^1+%s%s\n \n \n \n ", DF_MsToString(timeCheck), DF_MsToString(abs(timeCheck - bestTime->time)), scoreAddExtraText),qfalse,qtrue,qfalse, "cptimer slower");

	}

	// Show info
	//G_CenterPrint(activator - g_entities,3, va("Checkpoint!\n^3%s", DF_MsToString(timeCheck)),qfalse,qtrue,qfalse);
	//activator->client->pers.raceLastCheckpointTime = nowTime; // already handled via triggerLastPlayerContact and this way checkpoints can be less than 1s apart if needed.
}

void DF_target_husk(gentity_t* ent) {
	// do nothing. we just wanna be able to find it and replace it.
}

static void DF_RegisterSubCourse(const char* subcourse) {
	static char subCourseName[COURSENAME_MAX_LEN+1];
	int i;

	if (subcourse) {
		Q_strncpyz(subCourseName, subcourse, sizeof(subCourseName));
	}
	else {
		subCourseName[0] = '\0';
		level.emptyNameCourseExists = qtrue;
	}

	for (i = 0; i < level.numCourses; i++) {
		if (!Q_stricmp(subCourseName, level.courseName[i])) {
			return;
		}
	}

	if (level.numCourses >= MAX_COURSE_COUNT) {
		Com_Printf("^1More than %d subcourses found. Skipping '%s' for /top display.\n", MAX_COURSE_COUNT, subCourseName);
		return;
	}

	Q_strncpyz(level.courseName[level.numCourses], subCourseName, sizeof(level.courseName[0]));
	//Q_strlwr(level.courseName[level.numCourses]); // what for. let it be how it is, this isn't dependent on user input like /map cmd
	//Q_CleanStr(level.courseName[level.numCourses]); // we sanitize filenames already. should be fine.
	level.numCourses++;

}


extern void InitTrigger(gentity_t* self);
void DF_trigger_start_converted(gentity_t* ent) {

	InitTrigger(ent);

	ent->r.contents |= CONTENTS_TRIGGER_EXIT;
	ent->leave = DF_StartTimer_Leave;
	ent->triggerOnlyTraced = qtrue; // don't trigger if we are fully inside trigger brush or if robust triggers are deactivated. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_finish_converted(gentity_t* ent,qboolean registerSubCourse) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), deleting. WTF\n", ent->classname);
		G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	ent->touch = DF_FinishTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	DF_RegisterSubCourse(ent->message);

	trap_LinkEntity(ent);
}
void trigger_push_velocity_touch(gentity_t* self, gentity_t* other, trace_t* trace);

void DF_trigger_push_velocity_converted(gentity_t* ent) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), deleting. WTF\n", ent->classname);
		G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	// unlike other triggers, we need to send this one to the client
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/weapons/force/jump.wav");

	ent->s.eType = ET_PUSH_TRIGGER;
	ent->touch = trigger_push_velocity_touch;

	trap_LinkEntity(ent);
}
void DF_trigger_checkpoint_converted(gentity_t* ent) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), deleting. WTF\n", ent->classname);
		G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	ent->touch = DF_CheckpointTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	trap_LinkEntity(ent);
}
void DF_trigger_start(gentity_t* ent) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), maybe used as a target, turning into q3 target_startTimer. WTF\n", ent->classname);
		G_SetClassName(ent, "target_startTimer");
		//G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	ent->r.contents |= CONTENTS_TRIGGER_EXIT;
	ent->leave = DF_StartTimer_Leave;
	ent->triggerOnlyTraced = qtrue; // don't trigger if we are fully inside trigger brush or if robust triggers are deactivated. only when entering/leaving

	level.dfStartTriggerTypes |= (1 << DFTRIG_NT_JAPRO);

	trap_LinkEntity(ent);
}
void DF_trigger_finish(gentity_t* ent) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), maybe used as a target, turning into q3 target_stopTimer. WTF\n", ent->classname);
		G_SetClassName(ent, "target_stopTimer");
		//G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	G_SpawnInt("objective", "0", &ent->objective); // japro checkpoints

	ent->touch = DF_FinishTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	level.dfEndTriggerTypes |= (1 << DFTRIG_NT_JAPRO);

	DF_RegisterSubCourse(ent->message);

	trap_LinkEntity(ent);
}
void DF_trigger_checkpoint(gentity_t* ent) {

	if (!ent->model) {
		// broken dumb trigger (srsly i dont get what some ppl are thinking)
		G_Printf("DEFRAG: ^1Broken %s (no model), maybe used as a target, turning into q3 target_checkpoint. WTF\n", ent->classname);
		G_SetClassName(ent, "target_checkpoint");
		//G_FreeEntity(ent);
		return;
	}

	InitTrigger(ent);

	G_SpawnInt("objective", "0", &ent->objective); // japro checkpoints

	ent->touch = DF_CheckpointTimer_Touch;
	ent->triggerOnlyTraced = qtrue;  // don't trigger if we are fully inside trigger brush. only when entering/leaving

	level.dfCheckPointTriggerTypes |= (1 << DFTRIG_NT_JAPRO);

	trap_LinkEntity(ent);
}


qboolean G_Q3DefragTriggerConvert(gentity_t* trigger, gentity_t* target, q3DefragTargetType_t targetType, qboolean* anyTriggerFound, int depth, triggerConversionProperties_t* props) {
	triggerConversionProperties_t propsLocal;
	char* oldModel;
	q3CourseType_t q3CourseType = Q3COURSE_UNIVERSAL;
	qboolean specificQ3SpawnTypeOverride = qfalse;
	const char* typeString = NULL;
	gentity_t* otherTarget = NULL;
	const char* oldClass;
	qboolean oldNotCPM, oldNotVQ3;
	float oldWait;

	if (!props) {
		memset(&propsLocal, 0, sizeof(propsLocal));
		props = &propsLocal;
	}

	if (!trigger->r.bmodel || !(trigger->r.contents & CONTENTS_TRIGGER) || !trigger->model) {
		triggerConversionProperties_t propsHere = *props;
		// check if this is a target_fragsfilter or such
		gentity_t* trueTrigger = NULL;

		// upward inherit this stuff. if any part of the chain has one of these values set, the others must too
		propsHere.notCPM = (qboolean)(propsHere.notCPM || trigger->notCPM);
		propsHere.notVQ3 = (qboolean)(propsHere.notVQ3 || trigger->notVQ3);

		if (depth > 10) {
			// we might be stuck in a loop
			G_Printf("DEFRAG: ^1%s referenced by %s, but we have reached maximum depth. Possible recursion? Quitting.\n", target->classname, trigger->classname);
			return qfalse;
		}

		if (!Q_stricmp(trigger->classname, "target_fragsFilter") && targetType == TARGET_STOPTIMER) {
			propsHere.checkpointScore = trigger->count;
			propsHere.triggerPropsToSet |= TRIGPROP_CHECKPOINTSCORE;
			propsHere.ttFlags |= TTFLAGS_FINISHTIMER_SCOREREQUIRE;
			if (trigger->spawnflags & Q3SPAWNFLAG_TARGET_FRAGSFILTER_SILENT) {
				propsHere.ttFlags |= TTFLAGS_FINISHTIMER_SCOREREQUIRE_SILENT;
			}
			if (trigger->spawnflags & Q3SPAWNFLAG_TARGET_FRAGSFILTER_MATCH) {
				propsHere.ttFlags |= TTFLAGS_FINISHTIMER_SCOREREQUIRE_MATCH;
			}
			if (trigger->targetname && trigger->targetname[0]) {
				qboolean anyMatch = qfalse;
				// Then find all triggers that do something with them
				while ((trueTrigger = G_Find(trueTrigger, FOFS(target), trigger->targetname)) != NULL) {

					anyMatch = qtrue;
					if (G_Q3DefragTriggerConvert(trueTrigger, target, targetType, anyTriggerFound, depth+1, &propsHere)) {
						G_Printf("DEFRAG: ^3%s referenced by %s successfully converted at depth %d.\n", target->classname, trigger->classname, depth+1);
					}
					else {
						G_Printf("DEFRAG: ^3%s referenced by %s, which was not converted to a trigger at depth %d.\n", target->classname, trigger->classname, depth + 1);
					}
				}
				if (!anyMatch) {
					G_Printf("DEFRAG: ^1%s referenced by %s, but latter is not referenced by anything.\n", target->classname, trigger->classname);
				}
			}
			else {
				G_Printf("DEFRAG: ^1%s referenced by %s, but latter has no targetname.\n", target->classname, trigger->classname);
			}
		}
		else if (!Q_stricmp(trigger->classname, "target_relay")) {
			if (trigger->spawnflags & 4) {
				G_Printf("DEFRAG: ^1%s referenced by %s, but latter has spawnflag 4 (random). Weird, not supported. But converting anyway, ignoring the flag.\n", target->classname, trigger->classname);
			}
			if (trigger->targetname && trigger->targetname[0]) {
				qboolean anyMatch = qfalse;
				// Then find all triggers that do something with them
				while ((trueTrigger = G_Find(trueTrigger, FOFS(target), trigger->targetname)) != NULL) {
					anyMatch = qtrue;
					if (G_Q3DefragTriggerConvert(trueTrigger, target, targetType, anyTriggerFound, depth+1, &propsHere)) {
						G_Printf("DEFRAG: ^3%s referenced by %s successfully converted at depth %d.\n", target->classname, trigger->classname, depth+1);
					}
					else {
						G_Printf("DEFRAG: ^3%s referenced by %s, which was not converted to a trigger at depth %d.\n", target->classname, trigger->classname, depth + 1);
					}
				}
				if (!anyMatch) {
					G_Printf("DEFRAG: ^1%s referenced by %s, but latter is not referenced by anything.\n", target->classname, trigger->classname);
				}
			}
			else {
				G_Printf("DEFRAG: ^1%s referenced by %s, but latter has no targetname.\n", target->classname, trigger->classname);
			}
		}
		else {
			G_Printf("DEFRAG: ^1%s referenced by %s, not implemented.\n",target->classname,trigger->classname);
		}

		G_FreeEntity(trigger);

		return qfalse;
	}
	*anyTriggerFound = qtrue;
	oldModel = trigger->model;

	q3CourseType = Q3COURSE_UNIVERSAL;
	if (target->notVQ3 || trigger->notVQ3 || props->notVQ3)
	{
		q3CourseType = Q3COURSE_CPMONLY;
	}
	else if (target->notCPM || trigger->notCPM || props->notCPM)
	{
		q3CourseType = Q3COURSE_VQ3ONLY;
	}

	if (targetType == TARGET_STARTTIMER && level.hasQ3StyleSpecificSpawns && !q3CourseType) {
		// this is REALLY disgusting. 
		gentity_t* spawn = NULL;
		float spawnDistances[Q3COURSE_TYPECOUNT] = { HUGE_VALF,HUGE_VALF, HUGE_VALF };
		float distance;
		vec3_t triggerCenter, distanceVec;
		int type, type2, clo;
		q3CourseType_t q3SpawnType = Q3COURSE_UNIVERSAL;

		VectorAdd(trigger->r.absmin, trigger->r.absmax, triggerCenter);
		VectorScale(triggerCenter, 0.5f, triggerCenter);

		// Check distances to various types of spawns. YIKES
		while ((spawn = G_Find(spawn, FOFS(specialType), "playerspawn")) != NULL) {
			if (trap_InPVSIgnorePortals(triggerCenter, spawn->s.origin)) {
				q3SpawnType = spawn->notCPM ? Q3COURSE_VQ3ONLY : (spawn->notVQ3 ? Q3COURSE_CPMONLY : Q3COURSE_UNIVERSAL);
				VectorSubtract(triggerCenter, spawn->s.origin, distanceVec);
				distance = VectorLength(distanceVec);
				if (spawnDistances[q3SpawnType] > distance) {
					spawnDistances[q3SpawnType] = distance;
				}
			}
		}

		// rofl.. so bad.
		q3SpawnType = Q3COURSE_UNIVERSAL; // reuse this var.
		if (spawnDistances[Q3COURSE_VQ3ONLY] < spawnDistances[Q3COURSE_CPMONLY] && spawnDistances[Q3COURSE_VQ3ONLY] < spawnDistances[Q3COURSE_UNIVERSAL]) {
			q3SpawnType = Q3COURSE_VQ3ONLY;
			if (spawnDistances[Q3COURSE_CPMONLY] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_VQ3ONLY] * 2 > spawnDistances[Q3COURSE_CPMONLY]) {
					G_Printf("DEFRAG: ^1VQ3-only spawn closest to startTimer but distance is more not at least 50%% of the distance to a cpm-only spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3VQ3-only spawn closest to startTimer but cpm-only spawn found in PVS.\n");
				}
			}
			if (spawnDistances[Q3COURSE_UNIVERSAL] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_VQ3ONLY] * 2 > spawnDistances[Q3COURSE_UNIVERSAL]) {
					G_Printf("DEFRAG: ^1VQ3-only spawn closest to startTimer but distance is more not at least 50%% of the distance to a universal spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3VQ3-only spawn closest to startTimer but universal spawn found in PVS.\n");
				}
			}
		}
		else if (spawnDistances[Q3COURSE_CPMONLY] < spawnDistances[Q3COURSE_VQ3ONLY] && spawnDistances[Q3COURSE_CPMONLY] < spawnDistances[Q3COURSE_UNIVERSAL]) {
			q3SpawnType = Q3COURSE_CPMONLY;
			if (spawnDistances[Q3COURSE_VQ3ONLY] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_CPMONLY] * 2 > spawnDistances[Q3COURSE_VQ3ONLY]) {
					G_Printf("DEFRAG: ^1CPM-only spawn closest to startTimer but distance is more not at least 50%% of the distance to a vq3-only spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3CPM-only spawn closest to startTimer but vq3-only spawn found in PVS.\n");
				}
			}
			if (spawnDistances[Q3COURSE_UNIVERSAL] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_CPMONLY] * 2 > spawnDistances[Q3COURSE_UNIVERSAL]) {
					G_Printf("DEFRAG: ^1CPM-only spawn closest to startTimer but distance is more not at least 50%% of the distance to a universal spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3CPM-only spawn closest to startTimer but universal spawn found in PVS.\n");
				}
			}
		}
		else if (spawnDistances[Q3COURSE_UNIVERSAL] < spawnDistances[Q3COURSE_VQ3ONLY] && spawnDistances[Q3COURSE_UNIVERSAL] < spawnDistances[Q3COURSE_CPMONLY]) {
			q3SpawnType = Q3COURSE_UNIVERSAL;
			if (spawnDistances[Q3COURSE_VQ3ONLY] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_UNIVERSAL] * 2 > spawnDistances[Q3COURSE_VQ3ONLY]) {
					G_Printf("DEFRAG: ^1Universal spawn closest to startTimer but distance is more not at least 50%% of the distance to a vq3-only spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3Universal spawn closest to startTimer but vq3-only spawn found in PVS.\n");
				}
			}
			if (spawnDistances[Q3COURSE_CPMONLY] != HUGE_VALF) {
				if (spawnDistances[Q3COURSE_UNIVERSAL] * 2 > spawnDistances[Q3COURSE_CPMONLY]) {
					G_Printf("DEFRAG: ^1Universal spawn closest to startTimer but distance is more not at least 50%% of the distance to a cpm-only spawn.\n");
				}
				else {
					G_Printf("DEFRAG: ^3Universal spawn closest to startTimer but cpm-only spawn found in PVS.\n");
				}
			}
		}

		q3CourseType = q3SpawnType;
		if (q3CourseType) {
			specificQ3SpawnTypeOverride = qtrue;
		}
	}

	typeString = NULL;
	if (q3CourseType == Q3COURSE_CPMONLY) {
		typeString = "cpmcourse";
	}
	else if (q3CourseType == Q3COURSE_VQ3ONLY) {
		typeString = "vq3course";
	}

	oldClass = trigger->classname;
	oldWait = trigger->wait;
	oldNotCPM = trigger->notCPM;
	oldNotVQ3 = trigger->notVQ3;

	G_FreeEntity(trigger);
	G_InitGentity(trigger); // Is this too disgusting and evil? xd. I wanna reuse this slot tho.
	trigger->model = oldModel;
	trigger->notCPM = oldNotCPM;
	trigger->notVQ3 = oldNotVQ3;


	otherTarget = NULL;
	// check for other targets that might be relevant.
	// e.g. target_score for checkpoints
	while ((otherTarget = G_Find(otherTarget, FOFS(targetname), target->targetname)) != NULL) {

		if (otherTarget == target) {
			continue;
		}
		if (!Q_stricmp(otherTarget->classname, "target_score") && targetType == TARGET_CHECKPOINT) {
			//checkpointScore
			trigger->checkpointScore = otherTarget->count ? otherTarget->count : 1;
			if (oldWait < 0) {
				trigger->ttFlags |= TTFLAGS_CHECKPOINTTIMER_SCOREONCE;
			}
			G_Printf("DEFRAG: ^3%s which references %s also references %s. Applying.\n", oldClass, target->classname, otherTarget->classname);
		}
		else if (!Q_stricmp(otherTarget->classname, "target_speed") && targetType == TARGET_SPEED) {
			// eeeew. trigger references multiple target_speeds
			gentity_t* triggerCopy = G_SpawnAfter(trigger); // disgusting but we need to preserve the order of execution.
			triggerCopy->model = trigger->model;
			G_SetClassName(triggerCopy, "trigger_push_velocity");
			DF_trigger_push_velocity_converted(triggerCopy);
			triggerCopy->s.saberInFlight = 1;
			triggerCopy->s.forceFrame = otherTarget->spawnflags;
			triggerCopy->s.origin2[0] = otherTarget->speed;
			triggerCopy->s.origin2[1] = oldWait;
			triggerCopy->wait = oldWait;
			triggerCopy->notCPM = triggerCopy->s.generic1 = otherTarget->notCPM || oldNotCPM;
			triggerCopy->notVQ3 = triggerCopy->s.genericenemyindex = otherTarget->notVQ3 || oldNotVQ3;
			G_Printf("DEFRAG: ^3%s which references %s also references %s. Making extra trigger.\n", oldClass, target->classname, otherTarget->classname);
			G_Printf("DEFRAG: ^2%s converted (via copy).\n", triggerCopy->classname);
		}
		else {
			G_Printf("DEFRAG: ^1%s which references %s also references %s. Not implemented.\n", oldClass, target->classname, otherTarget->classname);
		}

	}

	trigger->courseID = q3CourseType;
	if (q3CourseType || !level.hasQ3StyleSpecificSpawns) {
		trigger->spawnflags |= SF_FINISHTIMER_REQUIRE_SPECIFIC_STARTTRIGGER;
	}

	switch (targetType) { // reusing japro classnames for compatibility
	case TARGET_STARTTIMER:
		G_SetClassName(trigger, "df_trigger_start");
		if (specificQ3SpawnTypeOverride && typeString) {
			trigger->overrideMessage = typeString; // ugh, is that cast safe. not rly?
			DF_RegisterSubCourse(trigger->overrideMessage);
		}
		DF_trigger_start_converted(trigger);
		level.dfStartTriggerTypes |= (1 << DFTRIG_Q3);
		G_Printf("DEFRAG: ^2Q3 %s (%s%s) at %s converted.\n", target->classname, q3CourseType ? typeString : "", specificQ3SpawnTypeOverride ? "-SPAWNOVERRIDE" : "", vtos(target->s.origin));
		break;
	case TARGET_STOPTIMER:
		G_SetClassName(trigger, "df_trigger_finish");
		if (q3CourseType && typeString) {
			trigger->message = typeString;
		}
		else {
			trigger->message = NULL;
		}
		DF_trigger_finish_converted(trigger, q3CourseType && typeString || !level.hasQ3StyleSpecificSpawns);
		level.dfEndTriggerTypes |= (1 << DFTRIG_Q3);
		G_Printf("DEFRAG: ^2Q3 %s (%s) at %s converted.\n", target->classname, q3CourseType ? typeString : "", vtos(target->s.origin));
		break;
	case TARGET_SPEED:
		// TODO Ability to activate/deactivate?
		G_SetClassName(trigger, "trigger_push_velocity");
		DF_trigger_push_velocity_converted(trigger);
		trigger->s.saberInFlight = 1;
		trigger->s.forceFrame = target->spawnflags;
		trigger->s.origin2[0] = target->speed;
		trigger->s.origin2[1] = oldWait; // maybe we predict it someday?
		trigger->wait = oldWait;
		trigger->s.generic1 = (qboolean)(target->notCPM || props->notCPM || oldNotCPM);
		trigger->s.genericenemyindex = (qboolean)(target->notVQ3 || props->notVQ3 || oldNotVQ3);
		G_Printf("DEFRAG: ^2Q3 %s (%s) at %s converted.\n", target->classname, q3CourseType ? typeString : "", vtos(target->s.origin));
		break;
	default:
	case TARGET_CHECKPOINT:
		G_SetClassName(trigger, "df_trigger_checkpoint");
		DF_trigger_checkpoint_converted(trigger);
		level.dfCheckPointTriggerTypes |= (1 << DFTRIG_Q3);
		G_Printf("DEFRAG: ^2Q3 %s (%s) at %s converted.\n", target->classname, q3CourseType ? typeString : "", vtos(target->s.origin));
		break;
	}

	trigger->ttFlags |= props->ttFlags;
	if (props->triggerPropsToSet & TRIGPROP_CHECKPOINTSCORE) {
		trigger->checkpointScore |= props->checkpointScore;
	}

	return qtrue;
}

// q3 defrag targets are dependent on a separate trigger brush, which makes
// tracking accurate times more difficult. so we are going to remove the trigger brushes
// and the targets and replace them with proper defrag spawns.
//
//
// Basically, this function is fucking evil
void G_ConvertDefragTriggerTypes() {
	gentity_t*	target;
	gentity_t*	trigger;
	qboolean	anyTriggerFound = qfalse;
	int i,index;
	char* oldModel;
	char* oldType;

	//if ((level.dfStartTriggerTypes & (1<<DFTRIG_NT_JAPRO)) && (level.dfEndTriggerTypes & (1<<DFTRIG_NT_JAPRO)) && (level.dfCheckPointTriggerTypes & (1<<DFTRIG_NT_JAPRO)) ) {
	if (level.dfStartTriggerTypes && level.dfEndTriggerTypes && level.dfCheckPointTriggerTypes ) {
		//return; // got all we need, highest rank type triggers. the checkpoint one maybe doesnt need to be checked? idk.
		return; // got all we need, triggers of every type
	}

	target = NULL;
	for (i = 0; i < TARGET_TYPE_COUNT; i++) {
		// Go through all target types

		if (i == TARGET_STARTTIMER && level.dfStartTriggerTypes || i == TARGET_STOPTIMER && level.dfEndTriggerTypes || i == TARGET_CHECKPOINT && level.dfCheckPointTriggerTypes) {
			continue; // already got this type covered.
		}

		target = NULL;
		// Q3 style triggers
		while ((target = G_FindByClassName(target, q3DefragTargetNames[i])) != NULL) {
			trigger = NULL;
			if (!target->targetname) {
				G_Printf("DEFRAG: ^1untargeted %s at %s\n", target->classname, vtos(target->s.origin));
				G_FreeEntity(target);
				continue;
			}

			// Then find all triggers that do something with them
			while ((trigger = G_Find(trigger, FOFS(target), target->targetname)) != NULL) {

				G_Q3DefragTriggerConvert(trigger,target,i,&anyTriggerFound,0,NULL);
			}
			if (!anyTriggerFound) {
				G_Printf("DEFRAG: ^1untargeted %s at %s (targetname %s)\n", target->classname, vtos(target->s.origin),target->targetname);
			}
			G_FreeEntity(target);
		}

		if (i == TARGET_STARTTIMER && level.dfStartTriggerTypes || i == TARGET_STOPTIMER && level.dfEndTriggerTypes || i == TARGET_CHECKPOINT && level.dfCheckPointTriggerTypes || i == TARGET_SPEED) {
			continue; // already got this type covered.
		}

		// twi_timer
		if (i != TARGET_CHECKPOINT) { // dunno how twi mod handles checkpoints
			trigger = NULL;
			while ((trigger = G_FindByClassName(trigger, "Twi_timer")) != NULL) {
				if (!trigger->model) {
					continue;
				}
				if ((trigger->spawnflags & 1) && i != TARGET_STOPTIMER || !(trigger->spawnflags & 1) && i != TARGET_STARTTIMER) {
					continue; // spawnflag 1 means endtimer
				}
				// TODO Is the value of Twi_timer supposed to be the subcourse name?
				oldModel = trigger->model;
				oldType = trigger->classname;
				G_FreeEntity(trigger);
				G_InitGentity(trigger); // Is this too disgusting and evil? xd. I wanna reuse this slot tho.
				trigger->model = oldModel; 
				switch (i) { // reusing japro classnames for compatibility
					case TARGET_STARTTIMER:
						G_SetClassName(trigger, "df_trigger_start");
						DF_trigger_start_converted(trigger);
						level.dfStartTriggerTypes |= (1 << DFTRIG_TWITIMER);
						G_Printf("DEFRAG: ^2Twi %s converted to df_trigger_start.\n", oldType);
						break;
					case TARGET_STOPTIMER:
						G_SetClassName(trigger, "df_trigger_finish");
						DF_trigger_finish_converted(trigger,qtrue);
						level.dfEndTriggerTypes |= (1 << DFTRIG_TWITIMER);
						G_Printf("DEFRAG: ^2Twi %s converted to df_trigger_finish.\n", oldType);
						break;
					default:
					case TARGET_CHECKPOINT: // wont ever be hit atm, dunno how twi does checkpoints
						G_SetClassName(trigger, "df_trigger_checkpoint");
						DF_trigger_checkpoint_converted(trigger);
						level.dfCheckPointTriggerTypes |= (1 << DFTRIG_TWITIMER);
						G_Printf("DEFRAG: ^2Twi %s converted to df_trigger_checkpoint.\n", oldType);
						break;
				}
			}
			
		}

		if (i == TARGET_STARTTIMER && level.dfStartTriggerTypes || i == TARGET_STOPTIMER && level.dfEndTriggerTypes || i == TARGET_CHECKPOINT && level.dfCheckPointTriggerTypes) {
			continue; // already got this type covered.
		}

		// q3 rally timers
		// twi_timer
		if (level.q3r_numCheckpoints && level.q3r_hasStartFinish) { // dunno how twi mod handles checkpoints
			const char* typeToFind = i == TARGET_CHECKPOINT ? "rally_checkpoint" : "rally_startfinish";
			int oldNumber;
			trigger = NULL;
			while ((trigger = G_FindByClassName(trigger, typeToFind)) != NULL) {
				if (!trigger->model) {
					continue;
				}
				// 
				if (i == TARGET_STOPTIMER && level.q3r_numCheckpoints > 1) {
					DF_RegisterSubCourse("reverse");
				}

				if (i == TARGET_STARTTIMER) {
					gentity_t* triggerCopy = G_Spawn();
					// since we have to reuse rally_startfinish, we must just initialize a fresh entity for either start or finish.
					// since we still need to find and convert this trigger again later... copy for the start trigger

					triggerCopy->model = trigger->model;
					triggerCopy->number = trigger->number;
					G_SetClassName(triggerCopy, "df_trigger_start");
					DF_trigger_start_converted(triggerCopy);
					level.dfStartTriggerTypes |= (1 << DFTRIG_Q3RALLY);
					G_Printf("DEFRAG: ^2Q3R %s converted (via copy) to df_trigger_start.\n", trigger->classname);
					triggerCopy->ttFlags |= TTFLAGS_STARTTIMER_Q3RALLYSTYLE;
				}
				else {
					// TODO Is the value of Twi_timer supposed to be the subcourse name?
					oldModel = trigger->model;
					oldType = trigger->classname;
					oldNumber = trigger->number;
					G_FreeEntity(trigger);
					G_InitGentity(trigger); // Is this too disgusting and evil? xd. I wanna reuse this slot tho.
					trigger->model = oldModel;
					trigger->number = oldNumber;
					switch (i) { // reusing japro classnames for compatibility
					/*case TARGET_STARTTIMER:
						G_SetClassName(trigger, "df_trigger_start");
						DF_trigger_start_converted(trigger);
						level.dfStartTriggerTypes |= (1 << DFTRIG_Q3RALLY);
						G_Printf("DEFRAG: ^2Q3R %s converted to df_trigger_start.\n", oldType);
						break;*/
					case TARGET_STOPTIMER:
						G_SetClassName(trigger, "df_trigger_finish");
						DF_trigger_finish_converted(trigger, qtrue);
						level.dfEndTriggerTypes |= (1 << DFTRIG_Q3RALLY);
						//trigger->ttFlags |= TTFLAGS_FINISHTIMER_SCOREREQUIRE; 
						//trigger->checkpointScore = level.q3r_numCheckpoints;
						trigger->ttFlags |= TTFLAGS_FINISHTIMER_Q3RALLYSTYLE; // dont throw error if no checkpoints hit at all
						G_Printf("DEFRAG: ^2Q3R %s converted to df_trigger_finish.\n", oldType);
						break;
					default:
					case TARGET_CHECKPOINT: // wont ever be hit atm, dunno how twi does checkpoints
						G_SetClassName(trigger, "df_trigger_checkpoint");
						DF_trigger_checkpoint_converted(trigger);
						level.dfCheckPointTriggerTypes |= (1 << DFTRIG_Q3RALLY);
						//trigger->checkpointScore = 1;
						trigger->ttFlags |= TTFLAGS_CHECKPOINTTIMER_Q3RALLYSTYLE;
						//trigger->ttFlags |= TTFLAGS_CHECKPOINTTIMER_SCOREONCE;
						G_Printf("DEFRAG: ^2Q3R %s converted to df_trigger_checkpoint.\n", oldType);
						break;
					}
				}

			}

		}


		if (i == TARGET_STARTTIMER && level.dfStartTriggerTypes || i == TARGET_STOPTIMER && level.dfEndTriggerTypes || i == TARGET_CHECKPOINT && level.dfCheckPointTriggerTypes) {
			continue; // already got this type covered.
		}

		// trigger_multiple
		if (i != TARGET_CHECKPOINT) { // dunno how twi mod handles checkpoints
			trigger = NULL;
			index = (level.dfStartTriggerTypes & (1 << DFTRIG_TRIGMULT)) ? 0 : -1; // start already found so now the index is automatically 1 higher already.
			while ((trigger = G_FindByClassName(trigger, "trigger_multiple")) != NULL) {
				qboolean disgustingType = qfalse;
				if (!trigger->model) {
					continue;
				}
				if (trigger->roffname || trigger->target) {
					qboolean targetFound = qfalse;
					if (trigger->target) {
						// disgusting, but some mappers fucking just did trigger_multiple as start/end triggers with meaningless (non-existing) targets. im so mad.
						target = NULL;
						if ((target = G_Find(target, FOFS(targetname), trigger->target)) != NULL) {
							targetFound = qtrue;
						}
					}
					if (targetFound) {
						continue; // this is most likely a normal trigger, not an abused timer type trigger_multiple.
					}
					else {
						disgustingType = qtrue;
					}
				}
				index++;
				if (index == 1 && i != TARGET_STOPTIMER || index == 0 && i != TARGET_STARTTIMER) {
					continue; // index 0 = first trigger_multiple. means start timer. 
				}
				if ((level.dfEndTriggerTypes & (1 << DFTRIG_TRIGMULT)) && i == TARGET_STOPTIMER) {
					G_Printf("DEFRAG: ^1refusing to convert %s to df_trigger_finish%s. Already did it once. This map is really badly made (or not a defrag map at all and still very badly made).\n", trigger->classname, disgustingType ? " (disgusting type)" : "");
					continue; // already have one trigger_multiple finish. avoid creating additional ones in case a map is SUPER badly fucked
				}
				else if ((level.dfStartTriggerTypes & (1 << DFTRIG_TRIGMULT)) && i == TARGET_STARTTIMER) {
					G_Printf("DEFRAG: ^1refusing to convert %s to df_trigger_start%s. Already did it once. This map is really badly made (or not a defrag map at all AND badly made).\n", trigger->classname, disgustingType ? " (disgusting type)" : "");
					continue; // already have one trigger_multiple finish. avoid creating additional ones in case a map is SUPER badly fucked
				}

				oldModel = trigger->model;
				oldType = trigger->classname;
				G_FreeEntity(trigger);
				G_InitGentity(trigger); // Is this too disgusting and evil? xd. I wanna reuse this slot tho.
				trigger->model = oldModel;
				switch (i) { // reusing japro classnames for compatibility
				case TARGET_STARTTIMER:
					G_SetClassName(trigger, "df_trigger_start");
					DF_trigger_start_converted(trigger);
					level.dfStartTriggerTypes |= (1 << DFTRIG_TRIGMULT);
					G_Printf("DEFRAG: ^2%s converted to df_trigger_start%s.\n", oldType, disgustingType ? " (disgusting type)" : "");
					break;
				case TARGET_STOPTIMER:
					G_SetClassName(trigger, "df_trigger_finish");
					DF_trigger_finish_converted(trigger, qtrue);
					level.dfEndTriggerTypes |= (1 << DFTRIG_TRIGMULT);
					G_Printf("DEFRAG: ^2%s converted to df_trigger_finish%s.\n", oldType, disgustingType ? " (disgusting type)" : "");
					break;
				default:
				case TARGET_CHECKPOINT: // wont ever be hit atm, dunno how that trigger_multiple "system" does checkpoints
					G_SetClassName(trigger, "df_trigger_checkpoint");
					DF_trigger_checkpoint_converted(trigger);
					level.dfCheckPointTriggerTypes |= (1 << DFTRIG_TRIGMULT);
					G_Printf("DEFRAG: ^2%s converted to df_trigger_checkpoint%s.\n", oldType, disgustingType ? " (disgusting type)" : "");
					break;
				}
			}

		}

	}

	if (!level.dfStartTriggerTypes) {
		G_Printf("DEFRAG: ^1No start timers found.\n");
	}
	if (!level.dfEndTriggerTypes) {
		G_Printf("DEFRAG: ^1No end timers found.\n");
	}
	if (!level.dfCheckPointTriggerTypes) {
		G_Printf("DEFRAG: ^3No map checkpoints found.\n");
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

void ClientSetModeReal(gentity_t* ent, playerMode_e mode) {
	qboolean isRace = (qboolean)( mode == MODE_DEFRAG);

	if (ent->client->sess.mode == mode && ent->client->sess.raceMode == isRace) {
		return;
	}
	ent->client->sess.mode = mode;
	ent->client->sess.raceMode = isRace;

	ClientUserinfoChanged(ent-g_entities);
	
	ent->s.weapon = WP_SABER; //Dont drop our weapon
	if (!isRace) Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S//Delete all their projectiles / saved stuff

	// reset physicsfps because racemode has different rules for validating that.
	ResetPhysicsFpsStuff(ent);

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		//Delete all their projectiles / saved stuff
		RemoveLaserTraps(ent);
		RemoveDetpacks(ent);
		DeletePlayerProjectiles(ent);

		if (ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR) { // killing a player links him. catastrophe if not yet inuse :) also not great when in spec.
			G_Kill(ent); //stop abuse
		}
		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->ps.persistant[PERS_KILLED] = 0;
		ent->client->accuracy_shots = 0;
		ent->client->accuracy_hits = 0;
		ent->client->ps.fd.suicides = 0;
		ent->client->pers.enterTime = level.time; //reset scoreboard kills/deaths i guess... and time?

		if (!ent->client->sess.firstEnterTimeSet) {
			ent->client->sess.firstEnterTime = level.time;
			ent->client->sess.firstEnterTimeSet = qtrue;
		}
	}
	UpdateClientRaceVars(ent->client);
	G_SendServerCommand(ent - g_entities, va("print \"^3Mode updated: %s\n\"",modeNames[mode].string), qtrue);
}


int GetDefaultPlayerMode(qboolean allowDefrag) {
	playerMode_e mode = MODE_NORMAL;

	if (g_defrag.integer && allowDefrag) {
		mode = MODE_DEFRAG;
	}
	else if (g_modes.integer && g_modesDefault.integer && g_modesDefault.integer < MODE_NUM_MODES) {
		mode = g_modesDefault.integer;
	}
	if (mode == MODE_DEFRAG && !allowDefrag || mode == MODE_INVALID) {
		mode = MODE_NORMAL;
	}
	return mode;
}

void SetClientMode(gentity_t* ent, playerMode_e mode) {
	if (!ent->client)
		return;

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && (ent->client->ps.powerups[PW_NEUTRALFLAG] || ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG]) && ent->client->sess.mode != MODE_IRONMAN) { // maybe not let capper switch unless theres no other players left in ironman or theyre afk or whatever? meh.
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed when carrying a flag!\n\"");
		return;
	}

	if (!g_defrag.integer && mode == MODE_DEFRAG) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		//DF_SetRaceMode(ent, qfalse);
		mode = MODE_NORMAL;
		//return;
	}

	if (mode > MODE_DEFRAG && !g_modes.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		mode = MODE_NORMAL;
	}

	if (mode >= MODE_NUM_MODES || mode < 0) {
		trap_SendServerCommand(ent - g_entities, "print \"^5Invalid mode specified!\n\"");
		mode = MODE_NORMAL;
	}

	if (g_gametype.integer != GT_FFA) { // TA: What the heck is this?!
		if (g_gametype.integer >= GT_TEAM && g_defrag.integer && mode == MODE_DEFRAG)
		{//this is ok

			ent->s.weapon = WP_SABER; //Dont drop our weapon
			Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S

			ent->client->sess.mode = MODE_INVALID; // we will reset this
			ent->client->sess.raceMode = qfalse;//Set it false here cuz we are flipping it next // TA: (wut? oh.)
			if (ent->client->sess.sessionTeam != TEAM_FREE) {
				SetTeam(ent, "race");
			}
			else {
				SetTeam(ent, "spec");
			}
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed in this gametype!\n\"");
			return;
		}
	}

	// toggle
	if (mode == ent->client->sess.mode && mode != MODE_NORMAL) {
		int defaultMode = GetDefaultPlayerMode(qtrue);
		if (g_modes.integer && mode == defaultMode) {
			mode = MODE_NORMAL;
		}
		else {
			mode = defaultMode;
		}
	}

	ClientSetModeReal(ent, mode);
}

qboolean ClientModeValid(gentity_t* ent, qboolean allowDefrag) {
	return ent->client->sess.mode == MODE_NORMAL || ent->client->sess.mode == MODE_DEFRAG && g_defrag.integer && ent->client->sess.raceMode && allowDefrag || ent->client->sess.mode > MODE_DEFRAG && ent->client->sess.mode < MODE_NUM_MODES&& g_modes.integer;
}


void ClientSetDefaultMode(gentity_t* ent, qboolean allowDefrag) {
	ent->client->sess.mode = MODE_INVALID; // force it 
	ClientSetModeReal(ent, GetDefaultPlayerMode(allowDefrag));
}

void ResetClientModeIfInvalid(gentity_t* ent, qboolean allowDefrag) {
	if (!ClientModeValid(ent, allowDefrag)) {
		Com_Printf("^3Client %d mode invalid, resetting: %d (racemode %d)\n",(int)(ent - g_entities),ent->client->sess.mode, ent->client->sess.raceMode);
		G_SendServerCommand(ent-g_entities,va("print \"^3Mode invalid, resetting: %d (racemode %d)\n\"",ent->client->sess.mode, ent->client->sess.raceMode),qtrue);
		ClientSetDefaultMode(ent, allowDefrag);
	}
}

// Adapted from jaPRO
void Cmd_Race_f(gentity_t* ent)
{
	if (!ent->client)
		return;

	SetClientMode(ent, MODE_DEFRAG);

}

void Cmd_Mode_f(gentity_t* ent)
{
	char mode[20];
	int modeNum;
	if (!ent->client)
		return;

	trap_Argv(1, mode, sizeof(mode));
	if (!Q_stricmp(mode, "reset"))
	{
		modeNum = GetDefaultPlayerMode(qtrue);
		if (modeNum == ent->client->sess.mode) {
			ent->client->sess.mode = MODE_INVALID; // force a reset
		}
	}
	else {
		modeNum = PlayerModeNameToInteger(mode);
	}
	if (modeNum == -1) {
		trap_SendServerCommand(ent-g_entities,"print \"Invalid mode specified. Valid modes: reset, normal, defrag, duel, duelqueue, allforce, ironman\n\"");
		return;
	}
	
	SetClientMode(ent, modeNum);

}
void Cmd_ModeCmd_f(gentity_t* ent)
{
	char mode[20];
	int modeNum;
	if (!ent->client)
		return;

	trap_Argv(0, mode, sizeof(mode));
	if (!Q_stricmp(mode, "reset"))
	{
		modeNum = GetDefaultPlayerMode(qtrue);
		if (modeNum == ent->client->sess.mode) {
			ent->client->sess.mode = MODE_INVALID; // force a reset
		}
	}
	else {
		modeNum = PlayerModeNameToInteger(mode);
	}
	if (modeNum == -1) {
		trap_SendServerCommand(ent-g_entities,"print \"Invalid mode specified. Valid modes: reset, normal, defrag, duel, allforce, ironman\n\"");
		return;
	}
	
	SetClientMode(ent, modeNum);

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

	ent->client->pers.roll.segmentDisqualified |= ROLLDIS_RESETTIMERS;

	ent->client->ps.duelTime = ent->client->pers.raceStartCommandTime = 0;
	ent->client->pers.stats.startLevelTime = 0; 
	ent->client->ps.fd.forceRageRecoveryTime = 0; 
	
	// not like we really need to do this since it happens in start anyway
	memset(&ent->client->pers.raceDropped, 0, sizeof(ent->client->pers.raceDropped));
	memset(&ent->client->pers.stats, 0, sizeof(ent->client->pers.stats));

	if (wasReset && print)
		G_CenterPrint(ent - g_entities,3, "Timer reset!",qfalse,qtrue,qfalse, NULL);
		//G_CenterPrint(ent - g_entities,3, "Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n",qfalse,qtrue,qfalse);
}

void DF_ResetSegmentedRun(gentity_t* ent) {
	ent->client->pers.segmented.state = SEG_DISABLED;
	trap_G_COOL_API_PlayerUserCmdClear(ent - g_entities); 

	ent->client->pers.segmented.lastPosCount = 0;
	ent->client->pers.segmented.lastPos[0].resposCount = 0;
	memset(&ent->client->pers.segmented.lastPos[0].discards, 0, sizeof(ent->client->pers.segmented.lastPos[0].discards));
#if SEGMENTEDDEBUG
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
	ClientInactivitySpecTimerReset(ent, 500); // the client did something to invalidate his racetime, e.g. /kill. Reset his afk timer so he doesn't get sent to spec immediately
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
		trap_SendServerCommand(ent - g_entities, va("print \"This command is not allowed with your movement style (%d)!\n\"", (int)ent->client->sess.raceStyle.movementStyle));
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


qboolean ShouldNotCollide(gentity_t* entity, gentity_t* other)
{
	// we are in the duel queue but not in a duel, make everyone else nonsolid
	if (entity->client && entity->client->sess.mode == MODE_DUELQUEUE && !entity->client->ps.duelInProgress) {
		if (entity != other) {
			if (other->inuse &&
				(other->s.eType == ET_PLAYER ||
					(other->collisionFlags & ECF_NODUELERS)))
			{
				return qtrue;
			}
		}
	}
	// since we are in a duel, make everyone else nonsolid
	else if (entity->client && entity->client->ps.duelInProgress) {
			if (entity != other && (other-g_entities) != entity->client->ps.duelIndex) {
				if (other->inuse &&
					((other->collisionFlags & ECF_NODUELERS) || other->s.eType == ET_PLAYER))
				{
					return qtrue;
				}
			}
	}
	else if (entity->client && (entity->client->sess.raceMode || other->client && other->client->sess.mode != entity->client->sess.mode)) { //Have to check all entities because swoops can be racemode too :/
			if (other != entity) {
				if (other->inuse &&
					((other->collisionFlags & ECF_NORACERS) && entity->client->sess.raceMode || other->s.eType == ET_PLAYER))
					// im not sure yet. do i want doors to not be a thing for racers? limits the map choice a little bit.
					//	((other->s.eType == ET_MOVER) &&
					//		(!(Q_stricmp(other->classname, "func_door")) ||
					//			(!(Q_stricmp(other->classname, "func_plat"))))) ||
						
				{
					return qtrue;
				}
			}
	}
	else { // we are not dueling but make those that are nonsolid
		int entityOrOwnerMode = entity->client ? entity->client->sess.mode : MODE_NORMAL;
		const int saberOwner = entity->r.ownerNum;//Saberowner
		if (entity->inuse) {//Saber
			if (g_entities[saberOwner].client) {
				entityOrOwnerMode = g_entities[saberOwner].client->sess.mode;
				if (g_entities[saberOwner].client->ps.duelInProgress) {
					return qfalse; // wait so... if we are in a duel we can touch EVERYONE? or. i guess the game will already filter it elsewhere?
				}
			}
		}
		//loda fixme? This should go through all entities... to also account for people lightsabers..? or is that too costly
		if (other != entity) {
			if (other->inuse && other->client &&
				(other->client->ps.duelInProgress || other->client->sess.raceMode || !entity->client && saberOwner != ENTITYNUM_NONE && entityOrOwnerMode != other->client->sess.mode)) { //loda fixme? Or the ent is a saber, and its owner is in racemode or duel in progress

				return qtrue; // uh so for example func_bobbing cannot touch us, but we can touch it? is that ok?
			}
			// yep just go and dont touch sabers of racers or othermoders either :) or we will make them trollable in segmented runs
			if (!other->client && other->r.ownerNum != ENTITYNUM_NONE) {
				gentity_t* owner = &g_entities[other->r.ownerNum];
				if (owner->client) {
					if (owner->client->sess.raceMode || entityOrOwnerMode != owner->client->sess.mode) {
						return qtrue;
					}
				}
				// bit of "recursion" here .. eh we should rewrite this entire function someday its kinda chaotic and disgusting.
				// one crash ive had from not checking for if client is an event that belongs to a detpack that belongs to a player. meh. this is all disgusting.
				// we should prolly just do a better check based on clients, and then for items we just take the owner and recurse the entire function
				else if (owner->r.ownerNum != ENTITYNUM_NONE){ 
					owner = &g_entities[owner->r.ownerNum];
					if (owner->client) {
						if (owner->client->sess.raceMode || entityOrOwnerMode != owner->client->sess.mode) {
							return qtrue;
						}
					}
					else if (owner->r.ownerNum != ENTITYNUM_NONE) {
						owner = &g_entities[owner->r.ownerNum];
						if (owner->client) {
							if (owner->client->sess.raceMode || entityOrOwnerMode != owner->client->sess.mode) {
								return qtrue;
							}
						}
						else if (owner->r.ownerNum != ENTITYNUM_NONE) {
							owner = &g_entities[owner->r.ownerNum];
							if (owner->client) {
								if (owner->client->sess.raceMode || entityOrOwnerMode != owner->client->sess.mode) {
									return qtrue;
								}
							}
							else {
								Com_Printf("^1ShouldNotCollide: Ownernum max recursion level reached for item %d of class '%s'\n", (int)(other - g_entities),other->classname ? other->classname: "");
							}
						}
						else {
							Com_Printf("^1ShouldNotCollide: Owner of item %d of class '%s' ends as %d of class '%s'\n", (int)(other - g_entities), other->classname ? other->classname : "", (int)(owner - g_entities), owner->classname ? owner->classname : "");
						}
					}
					else {
						Com_Printf("^1ShouldNotCollide: Owner of item %d of class '%s' ends as %d of class '%s'\n", (int)(other - g_entities), other->classname ? other->classname : "", (int)(owner - g_entities), owner->classname ? owner->classname : "");
					}
				}
				else {
					Com_Printf("^1ShouldNotCollide: Owner of item %d of class '%s' ends as %d of class '%s'\n", (int)(other - g_entities), other->classname ? other->classname : "", (int)(owner - g_entities), owner->classname ? owner->classname : "");
				}
			}
		}
	}
	return qfalse;
}

static void JP_TraceReal(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean precise,qboolean traceCustomEpsilon, float customEpsilon, int traceCustomFlags) {
	
	if (precise && (coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
		trap_G_COOL_API_NonEpsilonTrace(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
	else if ((traceCustomEpsilon || traceCustomFlags) && (coolApi & COOL_APIFEATURE_CUSTOMEPSILONTRACE)) {
		trap_G_COOL_API_CustomEpsilonTrace(results, start, mins, maxs, end, passEntityNum, contentmask, traceCustomEpsilon, customEpsilon, traceCustomFlags);
	}
	else {
		trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
	}
	if (results->entityNum < ENTITYNUM_MAX_NORMAL && passEntityNum >= 0)
	{
		gentity_t* passEnt = g_entities + passEntityNum;
		gentity_t* ent = g_entities + results->entityNum;

		if (ShouldNotCollide(passEnt, ent))
		{
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;
			//if (precise && (coolApi & COOL_APIFEATURE_NONEPSILONTRACE)) {
			//	trap_G_COOL_API_NonEpsilonTrace(results, start, mins, maxs, end, passEntityNum, contentmask);
			//}
			//else {
			//	JP_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);
			//}
			JP_TraceReal(results,start,mins,maxs,end,passEntityNum,contentmask,precise, traceCustomEpsilon,customEpsilon, traceCustomFlags);
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
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qfalse,qfalse,0,0);
}
// don't use this for movement and normal stuff. 
// normal trace applies an epsilon (0.125f offset) to avoid some fuckery with vector snapping over network and i dont even know,
// but the result is that trace fractions are reduced, to where they hit something that trap_EntityContact does NOT hit at the end position
// so for example with trigger detection, that makes our trigger interpolation less precise. 
// hence, for trigger tracing, use this.
void JP_TracePrecise(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qtrue,qfalse,0,0);
}
void JP_TraceCustomEpsilon(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean traceCustomEpsilon,float customEpsilon,int traceCustomFlags)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qfalse, traceCustomEpsilon,customEpsilon, traceCustomFlags);
}
void JP_TraceCustomEpsilonQ2(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qfalse,qtrue, 0.03125f,TRACECUSTOMFLAG_Q2STYLE);
}
void JP_TraceCustomEpsilonQ2Lite(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask,qfalse,qtrue, 0.03125f,0);
}
void JP_TraceBenchmarked(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int customizationFlags)
{
	JP_TraceReal(results, start, mins, maxs, end, passEntityNum, contentmask, qfalse, qfalse, 0, TRACECUSTOMFLAG_BENCHMARK| customizationFlags);
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
	gclient_t* soloRelevantClient = (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) ? cl : followedClient;
	qboolean	canSeeTASClients = (soloRelevantClient->sess.solo == SOLO_SHOWALL || soloRelevantClient->pers.isHeadlessClient || (soloRelevantClient->pers.ttClientFlags & TTFLAGS_CLIENT_SHOWALLPLAYERSINCLUDINGMLBOTS));
	int i, originalValueReusable;
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

		if (es->eFlags & EF_PLAYER_EVENT) {
			gclient_t* eventClient = g_entities[es->otherEntityNum].client;
			#define IGNORETAS (!canSeeTASClients && (eventClient->pers.tasClient & TASCLIENT_MACHINELEARNING) && (eventClient->sess.raceMode || eventClient->sess.mode != cl->sess.mode)) // don't hide clients that could hurt us
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = IGNORETAS || backup->mvEntState.snapshotIgnoreRealClient[clientNum] || /*(cl->sess.ignore & (1 << i)) ||*/ cl->sess.solo == SOLO_ALL || cl->sess.solo == SOLO_STYLE && eventClient && (eventClient->sess.mode != cl->sess.mode || cl->sess.mode == MODE_DEFRAG && eventClient->sess.raceStyle.movementStyle != cl->sess.raceStyle.movementStyle);
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = IGNORETAS || backup->mvEntState.snapshotIgnore[clientNum] || /*(cl->sess.ignore & (1 << i)) ||*/ followedClient->sess.solo == SOLO_ALL || followedClient->sess.solo == SOLO_STYLE && eventClient && (eventClient->sess.mode != followedClient->sess.mode || followedClient->sess.mode == MODE_DEFRAG && eventClient->sess.raceStyle.movementStyle != followedClient->sess.raceStyle.movementStyle);
			}
			#undef IGNORETAS
		}

		if (es->eType == ET_PUSH_TRIGGER) {
			if (other->notCPM) {
				mvEnt->snapshotIgnore[followedClientNum] = backup->mvEntState.snapshotIgnore[followedClientNum] || followedClient->sess.raceMode && !MovementStyleHasVQ3OnlyJumppads(followedClient->sess.raceStyle.movementStyle);
			}
			else if (other->notVQ3) {
				mvEnt->snapshotIgnore[followedClientNum] = backup->mvEntState.snapshotIgnore[followedClientNum] || followedClient->sess.raceMode && !MovementStyleHasCPMOnlyJumppads(followedClient->sess.raceStyle.movementStyle);
			}
		}

		if (es->eType == ET_ITEM) {
			mvEnt->snapshotIgnore[followedClientNum] = backup->mvEntState.snapshotIgnore[followedClientNum] ||
				((followedClient->entityStates[i] || followedClient->triggerTimes[i] >= followedClient->pers.cmd.serverTime) && followedClient->sess.raceMode)
				|| ((other->goneForNonRacers || other->availableTimeForNonRacers >= level.time) && !followedClient->sess.raceMode);
		}

		if (es->eType == (ET_EVENTS + EV_SCREENSHAKE) && !es->modelindex || other->hideFromActiveRacers) { // dont send global screenshakes to active players unless they are not in a run
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = backup->mvEntState.snapshotIgnoreRealClient[clientNum] ||  cl->sess.sessionTeam != TEAM_SPECTATOR && other->parent != ent && cl->pers.raceStartCommandTime;
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = backup->mvEntState.snapshotIgnore[clientNum] || followedClient->sess.sessionTeam != TEAM_SPECTATOR && other->parent != followedEnt && followedClient->pers.raceStartCommandTime;
			}
		}

		if (other->belongsToParent) { // sniper shots, lightning, etc
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = backup->mvEntState.snapshotIgnoreRealClient[clientNum] || other->parent != ent && cl->sess.solo > 0 && other->parent != followedEnt; // if engine suppoorts it, respect wishes of spectator instead of client that's being followed
			}
			else { // wait wtf. why so complicated? we can respect wishes of this player no? since it gets updated on each target client anyway
				mvEnt->snapshotIgnore[followedClientNum] = backup->mvEntState.snapshotIgnore[clientNum] = mvEnt->snapshotIgnore[clientNum] || other->parent != followedEnt && followedClient->sess.solo > 0; // snapshot of the follower might happen before the client himself, and snapshotIgnore is based on clientnum in ps. Uhm does that make sense?
			}
		}

		// TODO rethink this. see comments below.
		if (es->eType == ET_BEAM &&/* other->parent != ent &&*/ es->generic1 == 3) {
			//mvEnt->snapshotIgnore[clientNum] = cl->sess.solo || cl->sess.hideLasers || (cl->sess.ignore & (1 << es->owner));
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = backup->mvEntState.snapshotIgnoreRealClient[clientNum] || other->parent != ent && ((cl->sess.solo > 0 && other->parent != followedEnt) || cl->sess.hideLasers || (cl->sess.ignore & (1 << es->owner))); // if engine suppoorts it, respect wishes of spectator instead of client that's being followed
			}
			else { // wait wtf. why so complicated? we can respect wishes of this player no? since it gets updated on each target client anyway
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = backup->mvEntState.snapshotIgnore[clientNum] || other->parent != followedEnt && (followedClient->sess.solo > 0 || followedClient->sess.hideLasers || (followedClient->sess.ignore & (1 << es->owner))); // snapshot of the follower might happen before the client himself, and snapshotIgnore is based on clientnum in ps. Uhm does that make sense?
			}
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
			#define IGNORETAS (!canSeeTASClients && (ocl->pers.tasClient & TASCLIENT_MACHINELEARNING) && (ocl->sess.raceMode || ocl->sess.mode != cl->sess.mode)) // don't hide clients that could hurt us
			//mvEnt->snapshotIgnore[clientNum] = /*(cl->sess.ignore & (1 << i)) ||*/ cl->sess.solo;
			if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
				mvEnt->snapshotIgnoreRealClient[clientNum] = IGNORETAS || backup->mvEntState.snapshotIgnoreRealClient[clientNum] || /*(cl->sess.ignore & (1 << i)) ||*/ cl->sess.solo == SOLO_ALL || cl->sess.solo == SOLO_STYLE && (ocl->sess.mode != cl->sess.mode || cl->sess.mode == MODE_DEFRAG && ocl->sess.raceStyle.movementStyle != cl->sess.raceStyle.movementStyle);
			}
			else {
				mvEnt->snapshotIgnore[followedClientNum] = mvEnt->snapshotIgnore[clientNum] = IGNORETAS || backup->mvEntState.snapshotIgnore[clientNum] || /*(cl->sess.ignore & (1 << i)) ||*/ followedClient->sess.solo == SOLO_ALL || followedClient->sess.solo == SOLO_STYLE && (ocl->sess.mode != followedClient->sess.mode || followedClient->sess.mode == MODE_DEFRAG && ocl->sess.raceStyle.movementStyle != followedClient->sess.raceStyle.movementStyle);
			}
			#undef IGNORETAS
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

void DF_SetMapDefaults(raceStyle_t rs) {
	int i;
	gentity_t* client;
	//int oldMsec;
	// TODO we wanna update the individual settings of players IF their old defaults were equal to the old map default?
	// but kind of a pita. just apply for now.
	for (i = 0; i < MAX_CLIENTS; i++) {
		client = (g_entities + i);
		if (!client->client) continue;
		DF_CarryClientOverToNewRaceStyle(client,&rs); 
		// TODO is this right? hm
		if (client->client->sess.login.loggedIn) {
			DF_RequestPlayerDefaultTime(client);
		}
		else {
			client->client->pers.raceBestTime = 0;
		}
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
	Q_strncpyz(data.course, DF_GetCourseName(qfalse), sizeof(data.course));

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_LOADMAPRACEDEFAULTS,
		"SELECT msec,jump,variant,runFlags FROM mapdefaults WHERE course=? AND subcourse=?"
	)) {
		trap_SendServerCommand(-1, "print \"^1Map defaults could not be loaded. Leaderboard may not display correctly.\n\"");
		level.mapDefaultsLoadFailed = qtrue;
		return;
	}
	G_COOL_API_DB_PreparedBindString(data.course);
	G_COOL_API_DB_PreparedBindString(DF_GetMainSubcourseName()); // subcourse

	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

void Cmd_DF_MapDefaults_f(gentity_t* ent)
{
	insertUpdateMapRaceDefaultsStruct_t	data;
	raceStyle_t rs = level.mapDefaultRaceStyle;
	const char* subcourse = DF_GetMainSubcourseName(); // TODO let us set this somehow
	int alwaysRunFlags = defaultRunFlags & ~allowedMapDefaultRunFlags; // they are default but we are not allowed to change their defaultness (?!)
	if (!ent->client) return;

	rs.runFlags |= alwaysRunFlags;

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
		Q_strncpyz(data.course, DF_GetCourseName(qfalse), sizeof(data.course));
			
		if (!Q_stricmp("run", arg1)) {

			char arg2[8] = { 0 };
			int index, index2, flag;
			const uint32_t mask = (allowedMapDefaultRunFlags & ((1 << MAX_RUN_FLAGS) - 1)) | alwaysRunFlags;

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
				"INSERT INTO mapdefaults (course,subcourse,msec,jump,variant,runFlags) VALUES (?,?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"runFlags=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindString(subcourse); // subcourse
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
				"INSERT INTO mapdefaults (course,subcourse,msec,jump,variant,runFlags) VALUES (?,?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"jump=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindString(subcourse); // subcourse
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
				"INSERT INTO mapdefaults (course,subcourse,msec,jump,variant,runFlags) VALUES (?,?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"variant=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindString(subcourse); // subcourse
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
				"INSERT INTO mapdefaults (course,subcourse,msec,jump,variant,runFlags) VALUES (?,?,?,?,?,?)"
				"ON DUPLICATE KEY UPDATE "
				"msec=?"
			);
			G_COOL_API_DB_PreparedBindString(data.course);
			G_COOL_API_DB_PreparedBindString(subcourse); // subcourse
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

void Cmd_DF_RestartSegmentedRun_f(gentity_t* ent) {
	if (!DF_ClientInSegmentedRunMode(ent->client) || ent->client->pers.segmented.state < SEG_REPLAY) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Cannot restart segmented replay. No replay active.\n\"");
		return;
	}
	else if (!ent->client->pers.segmented.playbackErrored) {
		trap_SendServerCommand(ent - g_entities, "print \"^1Cannot restart segmented replay. Replay has not failed so far.\n\"");
		return;
	}
	DF_StartSegmentedReplay(ent, qtrue);
}

void Cmd_DF_RunSettings_f(gentity_t* ent)
{
	qboolean strafebotButtonMessage = qfalse;
	qboolean segmentedRunMessage = qfalse;
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (trap_Argc() > 1) {

		char arg[8] = { 0 };
		int index, index2, flag;
		const uint32_t styleAllowed = allowedRunFlags & ~MovementStyleDisabledRunFlags(ent->client->sess.raceStyle.movementStyle);
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
		if (~styleAllowed & flag) {
			G_SendServerCommand(ent - g_entities, va("print \"Run flags: Invalid flag for this movement style: %i [0, %i]\n\"", index2, MAX_RUN_FLAGS - 1),qtrue);
			return;
		}

		if ( ent->client->pers.tasClient && ((ent->client->sess.raceStyle.runFlags & flag) & RFL_TAS) ) {
			G_SendServerCommand(ent - g_entities, "print \"Run flags: TAS clients are not allowed to switch TAS mode off.\n\"",qtrue);
			return;
		}

		if (flag & RFL_SEGMENTED) {

			if (!(ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED)) {
				segmentedRunMessage = qtrue;
			}

			if (level.nonDeterministicEntities) {
				G_SendServerCommand(ent - g_entities, va("print \"Warning: Map contains %i potentially non-deterministic entities. Segmented runs may not replay correctly and thus not count.\n\"", level.nonDeterministicEntities),qtrue);
			}

			if (!(coolApi & COOL_APIFEATURE_G_USERCMDSTORE)) {
				G_SendServerCommand(ent - g_entities,"print \"Error: Segmented runs are only available with the UserCmdStore coolAPI feature. Please use the appropriate server engine.\n\"",qtrue);
				return;
			}
			if (jk2version != VERSION_1_04) {
				// TODO is this still true?
				// We need the JK2MV 1.04 API because we need to send playerstates from game to engine and MV playerstate conversion would mess us up.
				G_SendServerCommand(ent - g_entities, "print \"Error: Segmented runs are only available with 1.04 API (this does not mean they don't work in 1.02, it's a code thing).\n\"",qtrue);
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
			ent->client->sess.raceStyle.runFlags &= styleAllowed; // just sanity/safety check
			ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
			DF_RaceStateInvalidated(ent,qtrue);
			if (flag & RFL_BOT ) {
				if (!(ent->client->sess.raceStyle.runFlags & RFL_BOT)) {
					// strafebot was turned off.
					ent->client->sess.rollAngleInvalidated = qtrue;
				}
				else {
					strafebotButtonMessage = qtrue;
				}
			} 
			//DF_InvalidateSpawn(ent);
		}

		G_SendServerCommand(ent - g_entities, va("print \"^7%s %s^7\n\"", runFlagsNames[index2].string, ((ent->client->sess.raceStyle.runFlags & flag)
			? "^2Enabled" : "^1Disabled")),qtrue);

		if (segmentedRunMessage) {
			trap_SendServerCommand(ent - g_entities, "print \"You have activated segmented run mode. Please consult ^2/help seg^7 if you encounter any problems or have any questions about how it works.\n\"");
		}
		if (strafebotButtonMessage) {
			trap_SendServerCommand(ent - g_entities, "print \"You have activated strafebot mode. Please bind ^2/+strafebot^7 (^2/+button14^7 if you don't use TommyTernal client) to a key to activate the strafebot itself, or enter it in console (same as keeping the button pressed)\n\"");
		}
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
		int oldRunFlags = client->sess.raceStyle.runFlags;

		/*if ((client->sess.raceStyle.movementStyle == MV_Q2 || client->sess.raceStyle.movementStyle == MV_CSS) && client->sess.raceStyle.jumpLevel != 1) {
			client->sess.raceStyle.jumpLevel = 1;
			trap_SendServerCommand(client - g_clients, "print \"Invalid jump height for style detected.\n\"");
			DF_RaceStateInvalidated(g_entities + (client - g_clients), qtrue);
		}*/

		client->sess.raceStyle.runFlags &= ~MovementStyleDisabledRunFlags(client->sess.raceStyle.movementStyle);
		if (client->pers.tasClient) {
			client->sess.raceStyle.runFlags |= RFL_TAS;
		}

		if (client->sess.raceStyle.runFlags != oldRunFlags) { // sanity checks
			trap_SendServerCommand(client - g_clients, "print \"Invalid run flags detected & fixed.\n\"");
			DF_RaceStateInvalidated(g_entities + (client - g_clients), qtrue);
		}

		client->ps.fd.forcePowersKnown = 0;
		client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_2; // allow to "shoot open" doors
		client->ps.fd.forcePowersKnown |= (1 << FP_LIGHTNING);
		client->ps.fd.forcePowerLevel[FP_LEVITATION] = MAX(0,client->sess.raceStyle.jumpLevel);
		if (client->sess.raceStyle.jumpLevel > 0) {
			client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		}
		client->ps.fd.forcePowerLevel[FP_SABERATTACK] = 3; //make sure its allowed on server? or?
		client->ps.fd.forcePowersKnown |= (1 << FP_SABERATTACK);
		client->ps.fd.forcePowerLevel[FP_SABERDEFEND] = 3; // should we set this at all? idk maybe some maps have guns firing at us or sth? make sure its consistent then i guess
		client->ps.fd.forcePowersKnown |= (1 << FP_SABERDEFEND);
		if (client->sess.raceStyle.movementStyle == MV_FORCE) {
			client->ps.fd.forcePowerLevel[FP_RAGE] = client->ps.fd.forcePowerLevel[FP_SPEED] = 3; // TODO will this work ok when ppl go out of racemode? idk
			client->ps.fd.forcePowersKnown |= (1 << FP_RAGE) | (1 << FP_SPEED);
		}
		else {
			client->ps.fd.forcePowerLevel[FP_RAGE] = client->ps.fd.forcePowerLevel[FP_SPEED] = 0;
			//client->ps.fd.forcePowersKnown &= ~((1 << FP_RAGE) | (1 << FP_SPEED));
		}
		if (client->sess.raceStyle.jumpLevel == -1) {
			client->ps.powerups[PW_YSALAMIRI] = INT_MAX;
		}
		else {
			client->ps.powerups[PW_YSALAMIRI] = 0;
		}
		if (client->pers.raceStartCommandTime) {
			client->ps.persistant[PERS_SCORE] = (client->ps.commandTime - client->pers.raceStartCommandTime)/ 1000;
		}
		else {
			client->ps.persistant[PERS_SCORE] = 0;
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
	VectorCopySafe(client->ps.delta_angles, dfOldDelta);
}

void DF_PostDeltaAngleChange(gclient_t* client, qboolean setResettable) {
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
		if (setResettable) {
			client->pers.segmented.anglesDiffResettable = qtrue; // for segmented strafebot runs
		}
	}
}


qboolean SavePosition(gentity_t* client, savedPosition_t* savedPosition) {
	if (!client->client) return qfalse;
	memset(savedPosition, 0, sizeof(savedPosition_t));
	savedPosition->ps = client->client->ps;
	savedPosition->raceStyle = client->client->sess.raceStyle;
	savedPosition->raceStartCommandTime = (client->client->sess.raceStyle.runFlags & RFL_SEGMENTED) ? client->client->pers.raceStartCommandTime : 0;
	savedPosition->ps.duelTime = savedPosition->raceStartCommandTime;

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
	//int* intPtr;
	//float* floatPtr;
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

	//restore some carefully selected persistant values that don't affect gameplay to make things feel better/more consistent clientside
	// i can probably do way more if not all of the peristant values and others but better safe than sorry for now. can always fix that later.
	client->client->ps.persistant[PERS_RANK] = backupPS.persistant[PERS_RANK]; // avoid "the force is with you" spam.
	client->client->ps.persistant[PERS_PLAYEREVENTS] = backupPS.persistant[PERS_PLAYEREVENTS]; // its only for the rewards stuff
	client->client->ps.persistant[PERS_SPAWN_COUNT] = backupPS.persistant[PERS_SPAWN_COUNT]+1; // makes it a teleport frame for cgame and makes it use the rright weapon

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
		client->client->ps.duelTime = client->client->pers.raceStartCommandTime = client->client->ps.commandTime - (storedPS->commandTime- savedPosition->raceStartCommandTime);
	}
	else {
		client->client->ps.duelTime = client->client->pers.raceStartCommandTime = 0;
	}

	client->client->ps.persistant[PERS_SPAWN_COUNT] = backupPS.persistant[PERS_SPAWN_COUNT];

	client->health = storedPS->stats[STAT_HEALTH];
	client->client->buttons = savedPosition->client.buttons;
	client->client->oldbuttons = savedPosition->client.oldbuttons;
	client->client->latched_buttons = savedPosition->client.latched_buttons;

	if (diffAccum) {
		VectorCopySafe(backupPS.delta_angles, oldDelta);
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

void DF_StartSegmentedReplay(gentity_t* ent, qboolean restart) {
	gclient_t* cl = ent->client;
	cl->pers.segmented.state = SEG_REPLAY;
	cl->pers.segmented.playbackStartedTime = level.time;
	if (!restart) {
		cl->pers.segmented.playbackStartedCommandTimeOffset = cl->ps.commandTime - level.time;
	}
	cl->pers.segmented.playbackNextCmdIndex = 0;
	cl->pers.segmented.playbackErrored = qfalse;
	if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
		// during replay, we are providing usercmds for server to send to spectators and player for demos
		ent->r.svFlags |= SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
	}
	ent->s.eFlags |= EF_SEGMENTEDREPLAY;
	cl->ps.eFlags |= EF_SEGMENTEDREPLAY;
	cl->ps.duelTime = cl->pers.raceStartCommandTime = 0;
	cl->pers.stats.startLevelTime = 0;
}

posHashType_t DF_GetPositionHash(playerState_t* ps) {
	floatint_t f;
	unsigned int hash = 0;
	posHashType_t realhash = 0;
#define ADDTOHASH(a) f.f = (a);hash ^= f.i
	ADDTOHASH(ps->origin[0]);
	ADDTOHASH(ps->origin[1]);
	ADDTOHASH(ps->origin[2]);
	ADDTOHASH(ps->velocity[0]);
	ADDTOHASH(ps->velocity[1]);
	ADDTOHASH(ps->velocity[2]);
#undef ADDTOHASH
	realhash = (hash & 0xFF) | ((hash >> 8) & 0xFF) | ((hash >> 16) & 0xFF) | ((hash >> 24) & 0xFF);
	//return hash;
	return realhash;
}

void DF_HandleSegmentedRunPre(gentity_t* ent) {
	gclient_t* cl;
	usercmd_t* ucmdPtr;
	usercmd_t ucmd;
	int clientNum;
	resposType_t resposRequested;
	qboolean saveposRequested;
	qboolean strafeBotActive;
	int msec;
	posHashType_t posHash;
	if (!ent->client) return;
	if (!(coolApi & COOL_APIFEATURE_G_USERCMDSTORE)) return;

	clientNum = ent - g_entities;
	cl = ent->client;

	if (!cl->sess.raceMode || !(cl->sess.raceStyle.runFlags & RFL_SEGMENTED)) {
		trap_G_COOL_API_PlayerUserCmdClear(clientNum);
		cl->pers.segmented.lastPosCount = 0;
		cl->pers.segmented.lastPos[0].resposCount = 0;
		memset(&cl->pers.segmented.lastPos[0].discards, 0, sizeof(cl->pers.segmented.lastPos[0].discards));
		if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
			// during replay, we are providing usercmds for server to send to spectators and player for demos
			ent->r.svFlags &= ~SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
		}
		ent->s.eFlags &= ~EF_SEGMENTEDREPLAY;
		cl->ps.eFlags &= ~EF_SEGMENTEDREPLAY;

#if SEGMENTEDDEBUG
		memset(ent->client->pers.segmented.debugTime, 0, sizeof(cl->pers.segmented.debugTime));
#endif
		cl->pers.segmented.state = SEG_DISABLED;
		cl->pers.segmented.msecProgress = 0;
		cl->pers.segmented.anglesDiffResettable = qfalse;
		return;
	}

	strafeBotActive = !!(cl->sess.raceStyle.runFlags & RFL_BOT);

	ucmdPtr = &cl->pers.cmd;

	msec = ucmdPtr->serverTime - cl->ps.commandTime;

	if (msec <= 0)
	{
		return; // idk why this should hapen but whatever (actually might happen after replay?)
	}

	resposRequested = cl->pers.segmented.respos;
	cl->pers.segmented.respos = RESPOS_NONE;
	saveposRequested = cl->pers.segmented.savePos;
	cl->pers.segmented.savePos = qfalse;


	if (cl->pers.segmented.state == SEG_REPLAY) {
		if (resposRequested || saveposRequested) {
			// TODO we shouldnt even get here. commands from client should be blocked during a replay.
			G_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available during the replay of a run.\n\"",qtrue);
		}
		if (cl->pers.segmented.playbackErrored && (!cl->pers.segmented.lastPlaybackErroredCenterprint || level.time-2500 > cl->pers.segmented.lastPlaybackErroredCenterprint || cl->pers.segmented.lastPlaybackErroredCenterprint > level.time)) {
			G_CenterPrint(ent-g_entities,3,"^3Segmented replay failing. You can try ^2/resseg^3 to restart the replay.\n",qfalse,qtrue,qfalse,NULL);
			cl->pers.segmented.lastPlaybackErroredCenterprint = level.time;
		}
		if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
			// during replay, we are providing usercmds for server to send to spectators and player for demos
			ent->r.svFlags |= SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
		}
		ent->s.eFlags |= EF_SEGMENTEDREPLAY;
		cl->ps.eFlags |= EF_SEGMENTEDREPLAY;
		return;
	}

	if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
		// during replay, we are providing usercmds for server to send to spectators and player for demos
		ent->r.svFlags &= ~SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
	}
	ent->s.eFlags &= ~EF_SEGMENTEDREPLAY;
	cl->ps.eFlags &= ~EF_SEGMENTEDREPLAY;


	if (!cl->pers.raceStartCommandTime) {

		ucmd = *ucmdPtr;
		// Not currently in a run.
		// Maybe reset recording of packets.
		if (resposRequested || saveposRequested) {
			G_SendServerCommand(ent - g_entities, "print \"Respos/savepos are not available in segmented run mode outside of an active run.\n\"",qtrue);
		}
		if (!VectorLength(cl->ps.velocity) && !ucmdPtr->forwardmove && !ucmdPtr->rightmove && !ucmdPtr->upmove && cl->ps.groundEntityNum == ENTITYNUM_WORLD || cl->pers.segmented.state < SEG_RECORDING 
			|| (cl->sess.raceStyle.runFlags & RFL_BOT) && cl->pers.segmented.anglesDiffResettable // i think this captures the shit below?
			|| (( cl->pers.segmented.anglesDiffAccum[0] || cl->pers.segmented.anglesDiffAccum[1] || cl->pers.segmented.anglesDiffAccum[2] // just a sanity check
			|| cl->pers.segmented.anglesDiffAccumActual[0] || cl->pers.segmented.anglesDiffAccumActual[1] || cl->pers.segmented.anglesDiffAccumActual[2]) && !(cl->sess.raceStyle.runFlags & RFL_BOT)) // just a sanity check
			|| cl->sess.rollAngleInvalidated || cl->sess.raceStateInvalidated // just to be safe(r)
			) {
			// uuuuh what about mover states etc? oh dear. i guess it wont work for maps with movers. or we do what japro does and disable movers.
			// wait i know! we can disable movers for segmented runs. ez.
			//if (cl->ps.groundEntityNum == ENTITYNUM_WORLD || cl->ps.groundEntityNum == ENTITYNUM_NONE) {
				trap_G_COOL_API_PlayerUserCmdClear(clientNum);
#if SEGMENTEDDEBUG
				memset(cl->pers.segmented.debugTime, 0, sizeof(cl->pers.segmented.debugTime));
#endif
				VectorClear(cl->pers.segmented.anglesDiffAccum);
				VectorClear(cl->pers.segmented.anglesDiffAccumActual);
				SavePosition(ent, &cl->pers.segmented.startPos);
				cl->pers.segmented.state = SEG_RECORDING;
				cl->pers.segmented.msecProgress = 0;
				cl->pers.segmented.anglesDiffResettable = qfalse;
				cl->pers.segmented.lastPosCount = 0;
				cl->pers.segmented.lastPos[0].resposCount = 0;
				memset(&cl->pers.segmented.lastPos[0].discards, 0, sizeof(cl->pers.segmented.lastPos[0].discards));
			//}
		}

		ucmd.serverTime = cl->pers.segmented.msecProgress + msec;
		cl->pers.segmented.msecProgress += msec;

		trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &ucmd, DF_GetPositionHash(&cl->ps));
#if SEGMENTEDDEBUG
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
			segmentedPos_t* savePos = &cl->pers.segmented.lastPos[RESPOSINDEX(cl->pers.segmented.lastPosCount)];
			SavePosition(ent, &savePos->pos);
			{
				// mark this as a cut
				usercmd_t cutmarker;
				memset(&cutmarker, 0, sizeof(cutmarker));
				cutmarker.serverTime = -1;
				trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &cutmarker,0);
			}
			cl->pers.stats.saveposCount++;
			savePos->posIndex = cl->pers.segmented.lastPosCount;
			savePos->msecProgress = cl->pers.segmented.msecProgress;
			savePos->resposCount = 0;
			memset(&savePos->discards, 0, sizeof(savePos->discards));
			savePos->userCmdIndex = trap_G_COOL_API_PlayerUserCmdGetCount(clientNum) - 1;
			cl->pers.segmented.state = SEG_RECORDING_HAVELASTPOS;
			VectorCopy(cl->pers.segmented.anglesDiffAccum,savePos->anglesDiffAccum);
			VectorClear(cl->pers.segmented.anglesDiffAccum);
			cl->pers.segmented.lastPosCount++;
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
			int resposIndex = cl->pers.segmented.lastPosCount - 1;
			segmentedPos_t* resPos = &cl->pers.segmented.lastPos[RESPOSINDEX(resposIndex)];
			qboolean discardError = qfalse;

			if (resposRequested == RESPOS_DISCARD) {
				// we want to discard the latest savepos and go to the previous one
				// first check if the current respos point is valid to begin with
				if (resposIndex >= 1) {
					if (resPos->posIndex == resposIndex) {
						// now check if the one before that is valid
						int resposIndexOld = cl->pers.segmented.lastPosCount - 2;
						segmentedPos_t* resPosOld = &cl->pers.segmented.lastPos[RESPOSINDEX(resposIndexOld)];
						if (resPosOld->posIndex == resposIndexOld) {
							int left;
							// ok all good. let's roll back.
							// add the stored accum of this point to the current accum, since we're rolling it back.
							VectorAdd(cl->pers.segmented.anglesDiffAccum, resPos->anglesDiffAccum, cl->pers.segmented.anglesDiffAccum);
							cl->pers.segmented.anglesDiffAccum[0] &= 65535;
							cl->pers.segmented.anglesDiffAccum[1] &= 65535;
							cl->pers.segmented.anglesDiffAccum[2] &= 65535;
							// add the discarded point's resposCount to the previous point and log some stats about the discards
							resPosOld->resposCount += resPos->resposCount; // resposCount includes everything.
							resPosOld->discards.resposCount += resPos->resposCount; // discards.resposCount includes everything except this point's own respos count (so we can simply subtract)
							resPosOld->discards.discardCount += resPos->discards.discardCount + 1; // discardCount is the sum of all discards including more deeply nested discards
							resPosOld->discards.maxDiscardDepth = MAX(resPos->discards.maxDiscardDepth + 1, resPosOld->discards.maxDiscardDepth);

							cl->pers.stats.discardCount++;
							cl->pers.stats.discardResposCount += resPos->resposCount;
							cl->pers.stats.discardMaxDepth = MAX(resPosOld->discards.maxDiscardDepth, cl->pers.stats.discardMaxDepth);

							cl->pers.segmented.lastPosCount--;
							resposIndex = cl->pers.segmented.lastPosCount - 1;
							resPos = &cl->pers.segmented.lastPos[RESPOSINDEX(resposIndex)];

							for (left = 0; left < SEGMENTED_MAX_RESPOS; left++) {
								// find out how many more times we can do this.
								resposIndexOld = cl->pers.segmented.lastPosCount - 2 - left;
								resPosOld = &cl->pers.segmented.lastPos[RESPOSINDEX(resposIndexOld)];
								if (resposIndexOld < 0 || resposIndexOld != resPosOld->posIndex) {
									break;
								}
							}

							G_SendServerCommand(ent - g_entities, va("print \"One saved position was discarded. %d more discards are possible.\n\"", left), qtrue);
						}
						else {
							G_SendServerCommand(ent - g_entities, va("print \"^1Cannot discard saved position, previous position invalid/too old. Requested position %d, reference position %d.\n\"", resposIndexOld, resPosOld->posIndex), qtrue);
							discardError = qtrue;
						}
					}
					else {
						G_SendServerCommand(ent - g_entities, va("print \"^1Cannot discard saved position, current position invalid/too old. Requested position %d, reference position %d. ^1THIS SHOULD NEVER HAPPEN.\n\"", resposIndex, resPos->posIndex), qtrue);
						discardError = qtrue;
					}
				}
				else {
					G_SendServerCommand(ent - g_entities,"print \"^1Cannot discard saved position, there is nothing to go back to.\n\"", qtrue);
					discardError = qtrue;
				}
			}

			if (resposIndex >= 0 && resPos->posIndex != resposIndex) { // this should never happen! since we should refuse to discard a point if it meant ending up on an invalid one
				G_SendServerCommand(ent - g_entities, va("print \"^1Cannot restore position, position invalid/too old. Requested position %d, reference position %d. ^1THIS SHOULD NEVER HAPPEN.\n\"", resposIndex, resPos->posIndex), qtrue);
			}
			else {
				VectorAdd(cl->pers.segmented.anglesDiffAccumActual, cl->pers.segmented.anglesDiffAccum, cl->pers.segmented.anglesDiffAccumActual);
				cl->pers.segmented.anglesDiffAccumActual[0] &= 65535;
				cl->pers.segmented.anglesDiffAccumActual[1] &= 65535;
				cl->pers.segmented.anglesDiffAccumActual[2] &= 65535;
				VectorClear(cl->pers.segmented.anglesDiffAccum);
				RestorePosition(ent, &resPos->pos, cl->pers.segmented.anglesDiffAccumActual);
				cl->pers.stats.resposCount++;
				resPos->resposCount++;
				cl->pers.segmented.state = SEG_RECORDING_HAVELASTPOS; // un-invalidate.
				cl->pers.segmented.msecProgress = resPos->msecProgress;
				trap_G_COOL_API_PlayerUserCmdRemove(clientNum, resPos->userCmdIndex + 1, trap_G_COOL_API_PlayerUserCmdGetCount(clientNum) - 1);
				{
					// save how many times we respos'd this cut, and discards info
					usercmd_t cutmarker2;
					memset(&cutmarker2, 0, sizeof(cutmarker2));
					cutmarker2.serverTime = -2;
					cutmarker2.buttons = resPos->resposCount;
					cutmarker2.angles[0] = resPos->discards.discardCount;
					cutmarker2.angles[1] = resPos->discards.resposCount;
					cutmarker2.angles[2] = resPos->discards.maxDiscardDepth;
					trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &cutmarker2, 0);
				}
			}

		}
	}

	
	ucmd = *ucmdPtr;
	ucmd.angles[0] += cl->pers.segmented.anglesDiffAccumActual[0];
	ucmd.angles[1] += cl->pers.segmented.anglesDiffAccumActual[1];
	ucmd.angles[0] &= 65535;
	ucmd.angles[1] &= 65535;
	if (!strafeBotActive) {
		// in strafebot mode, roll is used for strafefactor packed as fp16, so dont touch this.
		ucmd.angles[2] += cl->pers.segmented.anglesDiffAccumActual[2];
		ucmd.angles[2] &= 65535; 
	}
	ucmd.serverTime = cl->pers.segmented.msecProgress + msec;
	cl->pers.segmented.msecProgress += msec;
	trap_G_COOL_API_PlayerUserCmdAdd(clientNum, &ucmd, DF_GetPositionHash(&cl->ps));
#if SEGMENTEDDEBUG
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
	if (!ent->client || ent->client->pers.connected == CON_DISCONNECTED || !ent->client->sess.raceMode) return;

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

		if (ent->client->pers.tasClient) { // just to make sure. 
			newRunFlags |= RFL_TAS; 
		}

		if (sess->raceStyle.jumpLevel != newJumpLevel ||
			sess->raceStyle.runFlags != newRunFlags ||
			sess->raceStyle.variant != newRs->variant ) {

			DF_RaceStateInvalidated(ent, qtrue);

			sess->raceStyle.jumpLevel = newJumpLevel;
			sess->raceStyle.runFlags = newRunFlags;
			sess->raceStyle.variant = newRs->variant;

			G_CenterPrint(ent - g_entities,3, "^2Map defaults loaded/changed. Run reset.",qfalse,qtrue,qtrue, NULL);
			G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed. Run reset.\n\"",qtrue);
		}
		else if(sess->spectatorState != SPECTATOR_FOLLOW || sess->spectatorClient < 0) {

			G_CenterPrint(ent - g_entities,3, "^2Map defaults loaded/changed.", qfalse,qtrue,qtrue, NULL);
			G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed.\n\"", qtrue);
		}


	}
	else if (sess->spectatorState != SPECTATOR_FOLLOW || sess->spectatorClient < 0) {
		G_CenterPrint(ent - g_entities, 3,"^2Map defaults loaded/changed.", qfalse,qtrue,qtrue, NULL);
		G_SendServerCommand(ent - g_entities, "print \"^2Map defaults loaded/changed.\n\"", qtrue);
	}

	sess->mapStyleBaseline = *newRs;
}

void Cmd_MovementStyle_f(gentity_t* ent)
{
	char mStyle[32];
	int newStyle;
	qboolean bounceButtonMessage = qfalse;

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

	// what for... we invalidate anyway
	//if (VectorLength(ent->client->ps.velocity)) {
	//	trap_SendServerCommand(ent - g_entities, "print \"You must be standing still to use this command!\n\"");
	//	return;
	//}

	trap_Argv(1, mStyle, sizeof(mStyle));

	newStyle = RaceNameToInteger(mStyle);
	//Just return if newstyle = old style?

	if (!(allowedMovementStyles & (1 << newStyle))) {
		trap_SendServerCommand(ent - g_entities, "print \"Movement style not allowed!\n\"");
		return;
	}

	if (newStyle >= 0) {
		int oldFlags = ent->client->sess.raceStyle.runFlags;
		int oldJump = ent->client->sess.raceStyle.jumpLevel;

		if (newStyle == MV_BOUNCE) {
			bounceButtonMessage = qtrue;
		}
		if (newStyle == MV_Q2 || newStyle == MV_CSS) {
			//ent->client->sess.raceStyle.jumpLevel = 1;
		}
		ent->client->sess.raceStyle.runFlags |= (MovementStyleDisabledRunFlags(ent->client->sess.raceStyle.movementStyle) & level.mapDefaultRaceStyle.runFlags); // if some flags were disabled by the style but are default in this level race style, re-enable them.
		ent->client->sess.raceStyle.movementStyle = newStyle;
		ent->client->sess.raceStyle.runFlags &= ~MovementStyleDisabledRunFlags(newStyle);
		G_SendServerCommand(ent - g_entities, va("print \"Movement style updated%s.%s\n\"", ent->client->sess.raceStyle.runFlags != oldFlags ? ", run flags updated for new style" : "", ent->client->sess.raceStyle.jumpLevel != oldJump ? " Jumplevel reset for new style." : ""), qtrue);
		ent->client->sess.mapStyleBaseline = level.mapDefaultRaceStyle;
		DF_RaceStateInvalidated(ent,qtrue);
		//DF_InvalidateSpawn(ent);

		if (newStyle == MV_SPEED) {
			ent->client->ps.fd.forcePower = 50;
		}

		if (bounceButtonMessage) {

			trap_SendServerCommand(ent - g_entities, "print \"You are playing in bounce movement style now. Please bind ^2/+bouncepower^7 (^2/+button13^7 if you don't use TommyTernal client) to a key. While pressing the key, you have extra bounce intensity to reflect off walls, floors, etc. This lasts for up to half a second at a time. If you use TommyTernal client, you will see a bar showing how much of this extra bounce power you have left.\n\"");
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
		G_CenterPrint(ent - g_entities,3, "^1Warning: ^7You must not have any force powers activated to save spawn.",qfalse,qtrue,qtrue, NULL);
		return;
	}

	if (ent->client->sess.raceStateInvalidated) {
		G_CenterPrint(ent - g_entities,3, "^1Warning: ^7Your race state is invalidated. Please respawn before saving spawn.",qfalse,qtrue,qtrue, NULL);
		return;
	}
	
	if (ent->client->ps.velocity[0] || ent->client->ps.velocity[1] || ent->client->ps.velocity[2] || ent->client->ps.groundEntityNum != ENTITYNUM_WORLD) {
		G_CenterPrint(ent - g_entities,3, va("^1Warning: ^7Cannot save spawn. Please stand still. (gen %d, v0 %f, v1 %f, v2 %f)", ent->client->ps.groundEntityNum, ent->client->ps.velocity[0], ent->client->ps.velocity[1], ent->client->ps.velocity[2]),qfalse,qtrue,qtrue, NULL);
		return;
	}

	SavePosition(ent,&ent->client->pers.savedSpawn);
	ent->client->pers.savedSpawn.client.sess.raceStateSoftInvalidated = qfalse; // for a spawn we will reset this.
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


void DF_SelectSpawn(gentity_t* ent) {

	qboolean allRace = qfalse;
	char	arg1[10];

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (trap_Argc() < 2) {
		if (g_defragSimpleResetSpawn.integer) {
			trap_SendServerCommand(ent - g_entities, "print \"Usage: selectSpawn [closest|last].\n\"");
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"Usage: selectSpawn [closest|last|reset].\n\"");
		}
		return;
	}
	trap_Argv(1, arg1, sizeof(arg1));

	if (!Q_stricmp(arg1, "closest")) {
		gentity_t* spawnPoint = SelectNearestDeathmatchSpawnPoint(ent->client->ps.origin);

		if (!spawnPoint) {
			G_SendServerCommand(ent - g_entities, "print \"^1No near spawnpoint found, WTF.\n\"", qtrue);
		}

		// TA: does this rly make sense here tho? idk
		// Tim needs to prevent bots from spawning at the initial point
		// on q3dm0...
		if ((spawnPoint->flags & FL_NO_BOTS) && (ent->r.svFlags & SVF_BOT)) {
			G_SendServerCommand(ent - g_entities, "print \"^1Closest spawn point is only for humans and you are a bot.\n\"", qtrue);
			return;	// try again
		}
		// just to be symetric, we have a nohumans option...
		if ((spawnPoint->flags & FL_NO_HUMANS) && !(ent->r.svFlags & SVF_BOT)) {
			G_SendServerCommand(ent - g_entities, "print \"^1Closest spawn point is only for bots and you are a human.\n\"", qtrue);
			return;	// try again
		}

		ent->client->pers.chosenDefragSpawnPoint = spawnPoint - g_entities;
		G_SendServerCommand(ent - g_entities, va("print \"^2Nearest point with entity number %d has been selected as your defrag spawn point.\n\"", ent->client->pers.chosenDefragSpawnPoint), qtrue);

	}
	else if (!Q_stricmp(arg1, "last")) {
		if (!ent->client->pers.lastSpawnPoint) {
			G_SendServerCommand(ent - g_entities, "print \"^1Did not find a previous spawn point, sorry about that.\n\"", qtrue);
			return;
		}
		ent->client->pers.chosenDefragSpawnPoint = ent->client->pers.lastSpawnPoint;
		G_SendServerCommand(ent - g_entities, va("print \"^2Previous spawn point with entity number %d has been selected as your defrag spawn point.\n\"", ent->client->pers.chosenDefragSpawnPoint), qtrue);
	}
	else if (!g_defragSimpleResetSpawn.integer && !Q_stricmp(arg1, "reset")) {
		if (!ent->client->pers.chosenDefragSpawnPoint) {
			G_SendServerCommand(ent - g_entities, "print \"^1Can not reset. No spawn point saved.\n\"", qtrue);
			return;
		}
		ent->client->pers.chosenDefragSpawnPoint = 0;
		G_SendServerCommand(ent - g_entities, "print \"^2Your defrag spawn point has been reset (this does not affect spawns saved with ^7/savespawn^2).\n\"", qtrue);
	}
	else {
		if (g_defragSimpleResetSpawn.integer) {
			trap_SendServerCommand(ent - g_entities, "print \"Usage: selectSpawn [closest|last].\n\"");
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"Usage: selectSpawn [closest|last|reset].\n\"");
		}
		return;
	}

}

void DF_ResetSpawn(gentity_t* ent) {
	if (!ent->client) return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	ent->client->pers.savedSpawnUsed = qfalse;

	if (ent->client->pers.chosenDefragSpawnPoint && !g_defragSimpleResetSpawn.integer) {
		G_SendServerCommand(ent - g_entities, "print \"Spawnpoint has been reset (this does not affect spawns saved with ^2/selectspawn^7. Use ^2/selectspawn reset^7 for those.).\n\"", qtrue);
	}
	else {
		ent->client->pers.chosenDefragSpawnPoint = 0;
		G_SendServerCommand(ent - g_entities, "print \"Spawnpoint has been reset.\n\"", qtrue);
	}
}


void lowerString(char* s) {
	if (!s) return;
	while (*s) {
		*s = tolower(*s);
		s++;
	}
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int JP_ClientNumberFromString(gentity_t* to, const char* s)
{
	gclient_t* cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	idnum = atoi(s);


	//redo
	/*
	if (!Q_stricmp(s, "0")) {
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return 0;
	}
	if (idnum && idnum < 32) {
		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}
	*/
	//end redo

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))  //changed and to or ..
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}




	// check for a name match
	//SanitizeString2(s, s2); // TODO adapt the jka code a bit for more robustness?
	Q_strncpyz(s2, s, sizeof(s2));
    Q_CleanStr(s2, (qboolean)(jk2startversion == VERSION_1_02), qtrue);
	lowerString(s2);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		//SanitizeString2(cl->pers.netname, n2);
		Q_strncpyz(n2, cl->pers.netname, sizeof(n2));
		Q_CleanStr(n2, (qboolean)(jk2startversion == VERSION_1_02), qtrue);
		lowerString(n2);

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];
			//SanitizeString2(cl->pers.netname, n2);
			Q_strncpyz(n2, cl->pers.netname, sizeof(n2));
			Q_CleanStr(n2, (qboolean)(jk2startversion == VERSION_1_02), qtrue);
			lowerString(n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ //found more than one match
					trap_SendServerCommand(to - g_entities, va("print \"More than one user '%s' on the server\n\"", s));
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	if (!atoi(s)) //Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		trap_SendServerCommand(to - g_entities, va("print \"User '%s' is not on the server\n\"", s));
	return -1;
}
void DF_SetSubContestDefaults(gclient_t* client) {
	int i;
	for (i = 0; i < SUBCONTESTS_COUNT; i++) {
		if (subContestParams[i].type == SUBCONTEST_TYPE_MINVAL) {
			client->sess.subcontestVals[i].value = HUGE_VALF;
		} else if (subContestParams[i].type == SUBCONTEST_TYPE_MAXVAL) {
			client->sess.subcontestVals[i].value = 0;
		}
	}
}

#define SUBCONTESTINSERT_1 "INSERT INTO subcontests (userid, course, type, value, recordwhen, msec, style, extraValue1, extraValue2, extraValue3, extraValue4) VALUES (?, ?, ?, ?, @now, ?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE "

#define MAXVALCONDITION "?>value"
#define MINVALCONDITION "?<value"

#define SUBCONTESTINSERT_2(a) "course = IF(" a ",?,course),"\
"recordwhen = IF(" a ",@now,recordwhen),"\
"msec = IF(" a ",?,msec),"\
"style = IF(" a ",?,style),"\
"extraValue1 = IF(" a ",?,extraValue1),"\
"extraValue2 = IF(" a ",?,extraValue2),"\
"extraValue3 = IF(" a ",?,extraValue3),"\
"extraValue4 = IF(" a ",?,extraValue4),"\
"value = IF(" a ",?,value);"

#define SUBCONTESTGETRANK(a) "SELECT COUNT(DISTINCT userid) AS countFaster FROM subcontests WHERE userid !=? AND userid != -1 AND type=? AND (" a " OR (value=? AND recordwhen<@now));" // if someone got the same time as you, but earlier, hes in front of u

#define MAXVALSORT "value DESC"
#define MINVALSORT "value ASC"

#define REALRANKSUBCONTEST(b) "ROW_NUMBER() OVER (PARTITION BY userid=-1 ORDER BY " b ") AS realRank"
#define SUBCONTESTTOPLIST(a) "SELECT userid,users.username,value,recordwhen,course,msec,extraValue1,extraValue2,extraValue3,extraValue4," REALRANKSUBCONTEST(a) " FROM subcontests LEFT JOIN users ON users.id=subcontests.userid WHERE type=? ORDER BY " a " LIMIT ?,11;" 



void DF_RequestSubContestLeaderboard(gentity_t* ent, subContests_t contest, int page) {
	subContestLeaderboardRequestStruct_t data;
	subContestParams_t* params = &subContestParams[contest];
	const char* query = NULL;
	if (params->type == SUBCONTEST_TYPE_MAXVAL) {
		query = SUBCONTESTTOPLIST(MAXVALSORT);
	}
	else if (params->type == SUBCONTEST_TYPE_MINVAL) {
		query = SUBCONTESTTOPLIST(MINVALSORT);
	}

	page = MAX(0, page - 1);

	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	data.contest = contest;
	data.page = page;

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_SUBCONTESTLEADERBOARD, query)) {
		return;
	}

	G_COOL_API_DB_PreparedBindInt(contest);
	G_COOL_API_DB_PreparedBindInt(page*10);

	G_COOL_API_DB_FinishAndSendPreparedStatement();

}


void DF_SetPlayerSubContestValue(gentity_t* ent, subContests_t subcontest, float value, float extraParam1, float extraParam2, int extraParam3, int extraParam4) {
	subContestParams_t* params = &subContestParams[subcontest];
	if (params->type == SUBCONTEST_TYPE_MAXVAL && value > ent->client->sess.subcontestVals[subcontest].value || params->type == SUBCONTEST_TYPE_MINVAL && value < ent->client->sess.subcontestVals[subcontest].value) {
		const char* query = NULL;
		insertUpdateSubContestStruct_t data;
		ent->client->sess.subcontestVals[subcontest].value = value;
		if (params->type == SUBCONTEST_TYPE_MAXVAL) {
			query = "SET @now=NOW();" SUBCONTESTINSERT_1 SUBCONTESTINSERT_2(MAXVALCONDITION) SUBCONTESTGETRANK(MINVALCONDITION);
		}
		else if (params->type == SUBCONTEST_TYPE_MINVAL) {
			query = "SET @now=NOW();" SUBCONTESTINSERT_1 SUBCONTESTINSERT_2(MINVALCONDITION) SUBCONTESTGETRANK(MAXVALCONDITION);
		}

		data.clientnum = ent - g_entities;
		memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
		data.value = value;
		data.userid = ent->client->sess.login.loggedIn ? ent->client->sess.login.id : -1;
		data.contest = subcontest;
		data.msec = ent->client->sess.raceMode ? ent->client->sess.raceStyle.msec : ent->client->pers.physicsFps.acceptedSettingMsec;

		// meta
		data.movementStyle = ent->client->sess.raceMode ? ent->client->sess.raceStyle.movementStyle : MV_JK2;
		data.extraValue1 = extraParam1;
		data.extraValue2 = extraParam2;
		data.extraValue3 = extraParam3;
		data.extraValue4 = extraParam4;

		if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_INSERTORUPDATESUBCONTEST,query)) {
			return;
		}

		// insert
		G_COOL_API_DB_PreparedBindInt(data.userid);
		G_COOL_API_DB_PreparedBindString(DF_GetCourseName(qfalse));
		G_COOL_API_DB_PreparedBindInt(subcontest);
		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindInt(data.msec);
		G_COOL_API_DB_PreparedBindInt(data.movementStyle);
		G_COOL_API_DB_PreparedBindFloat(extraParam1);
		G_COOL_API_DB_PreparedBindFloat(extraParam2);
		G_COOL_API_DB_PreparedBindInt(extraParam3);
		G_COOL_API_DB_PreparedBindInt(extraParam4);

		// or update
		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindString(DF_GetCourseName(qfalse));

		G_COOL_API_DB_PreparedBindFloat(value); //date

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindInt(data.msec);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindInt(data.movementStyle);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindFloat(extraParam1);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindFloat(extraParam2);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindInt(extraParam3);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindInt(extraParam4);

		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindFloat(value);

		// get rank
		G_COOL_API_DB_PreparedBindInt(data.userid);
		G_COOL_API_DB_PreparedBindInt(subcontest);
		G_COOL_API_DB_PreparedBindFloat(value);
		G_COOL_API_DB_PreparedBindFloat(value);

		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
}

// just some basic common checks. wanna be in jk2 movement. and not bot movement or some other.
qboolean DF_SetPlayerSubContestValueSafeguarded(gentity_t* ent, subContests_t subcontest, float value, float extraParam1, float extraParam2, int extraParam3, int extraParam4) {
	if (g_cheats.integer || ent->client->pers.tasClient || ent->client->pers.isHeadlessClient) {
		return qfalse;
	}
	if (ent->client->sess.raceMode) { // if in racemode, let's check weird stuff isn't going on.
		raceStyle_t clientRs = ent->client->sess.raceStyle;
		if ((clientRs.runFlags & (~allowedSafeguardedSubcontestRunFlags)) || clientRs.movementStyle != MV_JK2 && clientRs.movementStyle != MV_CHARGEJUMP && clientRs.movementStyle != MV_SPEED && clientRs.movementStyle != MV_FORCE && clientRs.movementStyle != MV_JK2SP || ent->client->sess.raceStateInvalidated) {
			// has runflags that aren't accepted (just cheat stuff generally)
			// or is not in jk2 movement mode (let's be a bit gatekeepy here!)
			return qfalse;
		}
	}
	DF_SetPlayerSubContestValue(ent,subcontest,value,extraParam1,extraParam2,extraParam3,extraParam4);
	return qtrue;
}




void DF_CheckRollSpeed(gentity_t* ent) {
	rollState_t* roll = &ent->client->pers.roll;
	rollState_t* statsRoll = &ent->client->pers.stats.roll;

	if (!ent->client->sess.raceMode) return; // dont bother outside defrag

	if (roll->status == ROLL_ENDED) {
		G_CenterPrint(ent - g_entities, 3, va("Roll Speed: ^%c%.2f^7ups, flyoff speedmult: %d, time: %d%s", roll->rollDisqualified ? '1' : '3', roll->rollSpeed, roll->finalAirClientSpeed, roll->rollAirTime,roll->rollDisqualified ? multiva(", dis: %d", roll->rollDisqualified) : ""), qfalse, qtrue, qfalse, "rollspeed");
		if (!roll->rollDisqualified && !ent->client->sess.raceStateInvalidated) {
			raceStyle_t defaults = defaultRaceStyle;
			raceStyle_t clientRs = ent->client->sess.raceStyle;
			defaults.runFlags = (defaults.runFlags & ~allowedRollRunFlags) | (clientRs.runFlags & allowedRollRunFlags); // allowedRollRunFlags are ones we don't care about, so we just take whatever the client has
			if (classifyLeaderBoard(&clientRs,&defaults) == LB_MAIN && clientRs.movementStyle == MV_JK2) {
				defaults.msec = clientRs.msec;
				DF_SetPlayerSubContestValue(ent, SUBCONTESTS_ROLLYMPICS_FIX, roll->rollSpeed, 0, 0, roll->finalAirClientSpeed, roll->rollAirTime);
			}
		}
		if (ent->client->pers.raceStartCommandTime) {
			if (statsRoll->status == ROLL_NONE) {
				*statsRoll = *roll;
			}
			else if (statsRoll->lastRollEndedTime < ent->client->pers.raceStartCommandTime && (roll->lastRollEndedTime - ent->client->pers.raceStartCommandTime) < 2000) {
				// last logged roll was before run and this one is within 2 seconds inside run. use this one then.
				*statsRoll = *roll;
			}
		}
		else {
			*statsRoll = *roll;
		}
	}
}

void ClientDisconnectFinish(int clientNum, gentity_t* ent);

qboolean DF_KeepClientZombie(gentity_t* ent) {
	qboolean isReplaying; 

	if (!ent->client) return qfalse;

	isReplaying = ent->client->sess.raceMode && (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && ent->client->pers.segmented.state == SEG_REPLAY;

	if (ent->client && (ent->client->pers.demoClipsPending || isReplaying || ent->client->pers.recordingDemo && ent->client->pers.keepDemoMaybe && !ent->client->pers.raceStartCommandTime)) { // we are either in a replay or at the end of a run still recording. or we have queued clip demos
		ent->client->clientIsZombified = qtrue;
		return qtrue;
	}
	else {
		if (ent->client->clientIsZombified) {
			ent->client->clientIsZombified = qfalse;
			ClientDisconnectFinish(ent-g_entities,ent);
		}
		return qfalse;
	}
}




// q3 rally map support
// code copied/adapted from q3rally

#define CHECKPOINT_SOUNDS		1
#define CHECKPOINT_MESSAGES		2

void Q3R_Touch_StartFinish(gentity_t* self, gentity_t* other, trace_t* trace) {
	char* place;

	if (!other->client) {
		return;
	}

	// this isnt actually used in this way. im just keeping the code here so i can remember the logic

	//if (g_developer.integer)
	//	G_Printf("Client %i touched the startfinish line.  Checkpoint number %i\n", other->s.clientNum, self->number);

	//if (other->currentLap > level.numberOfLaps && level.numberOfLaps) {
	//	return;
	//}

	//if (self->number == other->number) {
	//	other->currentLap++;
	//	// increment lap
	//	if (other->currentLap > level.numberOfLaps && level.numberOfLaps) {
	//		other->client->finishRaceTime = level.time;
	//		other->s.weapon = WP_NONE;
	//		other->takedamage = qfalse;

	//		trap_SendServerCommand(-1, va("raceFinishTime %i %i", other->s.clientNum, other->client->finishRaceTime));

	//		if (!level.finishRaceTime) {
	//			other->client->ps.stats[STAT_POSITION] = 1; // make sure the player is first

	//			level.winnerNumber = other->s.clientNum;
	//			level.finishRaceTime = level.time;
	//			trap_SendServerCommand(-1, va("print \"%s won the race!\n\"", other->client->pers.netname));
	//			trap_SendServerCommand(level.winnerNumber, "cp \"You won the race!\n\"");
	//		}
	//		else {
	//			switch (other->client->ps.stats[STAT_POSITION]) {
	//			case 1:
	//				place = "first";
	//				break;
	//			case 2:
	//				place = "second";
	//				break;
	//			case 3:
	//				place = "third";
	//				break;
	//			case 4:
	//				place = "forth";
	//				break;
	//			case 5:
	//				place = "fifth";
	//				break;
	//			case 6:
	//				place = "sixth";
	//				break;
	//			case 7:
	//				place = "seventh";
	//				break;
	//			case 8:
	//				place = "eighth";
	//				break;
	//			default:
	//				place = NULL;
	//				Com_Printf("Unknown placing: %i\n", other->client->ps.stats[STAT_POSITION]);
	//				break;
	//			}

	//			if (other->client->ps.stats[STAT_POSITION] <= 8) {
	//				trap_SendServerCommand(-1, va("print \"%s finished the race in %s place!\n\"", other->client->pers.netname, place));
	//			}
	//			else {
	//				trap_SendServerCommand(-1, va("print \"%s finished the race!\n\"", other->client->pers.netname));
	//			}
	//		}
	//	}
	//	else {
	//		other->number = 1;
	//		other->client->ps.stats[STAT_NEXT_CHECKPOINT] = other->number;
	//		other->client->ps.stats[STAT_FRAC_TO_NEXT_CHECKPOINT] = FLOAT2SHORT(0.1f);
	//		//			Com_Printf( "resetting frac, sf\n" );
	//		trap_SendServerCommand(other->client->ps.clientNum, va("newLapTime %i %i\n", other->currentLap, level.time));
	//	}


	//	if (other->currentLap == level.numberOfLaps) {
	//		trap_SendServerCommand(other->s.number, "cp \"Final lap\n\"");
	//		Rally_Sound(self, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/finallap.wav"));
	//	}
	//	else {
	//		Rally_Sound(self, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/checkpoint.wav"));
	//	}
	//}
}

void Q3R_Think_StartFinish(gentity_t* self) {
	gentity_t* ent;
	int		checkpoints;

	// FIXME: only do this a couple times after a client joins
	// send checkpoint to clients
/*
	if ((level.time / 2000) % 2)
		self->r.svFlags |= SVF_BROADCAST;
	else
		self->r.svFlags |= SVF_NOCLIENT;

	self->nextthink = level.time + 2000;
*/
// if there is a target use its origin and angles instead


	//if (self->target) {
	//	ent = G_PickTarget(self->target,!g_defrag.integer,NULL);
	//	if (ent) {
	//		VectorCopy(ent->s.origin, self->s.origin);
	//		VectorCopy(ent->s.angles, self->s.angles);
	//		self->s.frame = 1;

	//		G_FreeEntity(ent);
	//	}
	//	self->target = 0;
	//}

	//if (self->s.origin2[0] == 0.0f &&
	//	self->s.origin2[1] == 0.0f &&
	//	self->s.origin2[2] == 0.0f &&
	//	(self->s.origin[0] != 0.0f ||
	//		self->s.origin[1] != 0.0f ||
	//		self->s.origin[2] != 0.0f))
	//	VectorCopy(self->s.origin, self->s.origin2);

	// TA: do this count directly in rally_checkpoint spawn.

	//checkpoints = 0;

	//ent = NULL;
	//while ((ent = G_Find(ent, FOFS(classname), "rally_checkpoint")) != NULL) checkpoints++;
	//level.numCheckpoints = checkpoints;
	//if (g_trackReversed.integer && level.trackIsReversable) {
	//	ent = NULL;
	//	while ((ent = G_Find(ent, FOFS(classname), "rally_checkpoint")) != NULL) {
	//		ent->number = level.numCheckpoints - ent->number;
	//	}
	//}

	//self->number = level.numCheckpoints;
	//self->s.weapon = self->number;
}


void Q3R_SP_rally_startfinish(gentity_t* ent) {

	InitTrigger(ent);
	trap_LinkEntity(ent);
	// this is just a placeholder, we will convert it to normal defrag triggers. just keeping commented code so i know what its supposed to be doing
	level.q3r_numberOfLaps = ent->laps;
	level.q3r_hasStartFinish = qtrue;

	//G_SetClassName(ent, "rally_checkpoint");

	//trap_SetBrushModel(ent, ent->model);

	//if (!g_laplimit.integer) {
	//	level.numberOfLaps = ent->laps;
	//	trap_Cvar_Set("laplimit", va("%d", level.numberOfLaps));
	//}
	//else
	//	level.numberOfLaps = g_laplimit.integer;

	//// STONELANCE - April 23, 2002 temp for testing bezier curve stuff
	//ent->r.svFlags |= SVF_BROADCAST;
	////
	//ent->s.eType = ET_CHECKPOINT;

	//ent->touch = Touch_StartFinish;
	//ent->think = Think_StartFinish;
	//ent->nextthink = level.time + 100;
	//ent->s.frame = 0;

	//trap_LinkEntity(ent);
}

//
// rally_checkpoint
//

void Q3R_Touch_Checkpoint(gentity_t* self, gentity_t* other, trace_t* trace) {
	if (!other->client) {
		return;
	}


	// this is just a placeholder, we will convert it to normal defrag triggers. just keeping commented code so i know what its supposed to be doing
	// 
	//if (g_developer.integer)
	//	G_Printf("Client %i touched checkpoint number %i\n", other->s.clientNum, self->number);

	//if (self->number == other->number) {
	//	other->number++;	// FIXME: get rid of number? use s.weapon instead?
	//	other->client->ps.stats[STAT_NEXT_CHECKPOINT] = other->number;
	//	other->client->ps.stats[STAT_FRAC_TO_NEXT_CHECKPOINT] = FLOAT2SHORT(0.1f);
	//	//		Com_Printf( "resetting frac, cp\n" );

	//	if (self->spawnflags & CHECKPOINT_SOUNDS)
	//		Rally_Sound(self, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/checkpoint.wav"));

	//	if (self->spawnflags & CHECKPOINT_MESSAGES && self->s.otherEntityNum != -1 &&
	//		self->s.otherEntityNum != other->s.number)
	//	{
	//		if (g_entities[self->s.otherEntityNum].client->ps.stats[STAT_POSITION] < other->client->ps.stats[STAT_POSITION])
	//		{
	//			trap_SendServerCommand(other->s.number,
	//				va("print \"%s is ahead by %i seconds\n\"",
	//					g_entities[self->s.otherEntityNum].client->pers.netname,
	//					(level.time - self->updateTime) / 1000));
	//		}
	//	}

	//	self->s.otherEntityNum = other->s.number;
	//	self->updateTime = level.time;
	//}
}


void Q3R_Think_Checkpoint(gentity_t* self) {
	gentity_t* ent;

	// this is just a placeholder, we will convert it to normal defrag triggers. just keeping commented code so i know what its supposed to be doing
	/*
		// FIXME: only do this a couple times after a client joins
		// send checkpoint to clients
		if ((level.time / 2000) % 2){
			Com_Printf("Broadcast %d\n", self->s.number);
			self->r.svFlags |= SVF_BROADCAST;
			trap_LinkEntity (ent);
		}
		else{
			Com_Printf("Noclient\n");
			self->r.svFlags |= SVF_NOCLIENT;
			trap_LinkEntity (ent);
		}

		self->nextthink = level.time + 2000;
	*/

	// the following is not commented in q3r, the above is tho

	// if there is a target use its origin and angles instead
	//if (self->target) {
	//	ent = G_PickTarget(self->target,!g_defrag.integer,NULL);
	//	if (ent) {
	//		VectorCopy(ent->s.origin, self->s.origin);
	//		VectorCopy(ent->s.angles, self->s.angles);
	//		self->s.frame = 1;

	//		G_FreeEntity(ent);
	//	}
	//	self->target = 0;
	//}

	//if (self->s.origin2[0] == 0.0f &&
	//	self->s.origin2[1] == 0.0f &&
	//	self->s.origin2[2] == 0.0f &&
	//	(self->s.origin[0] != 0.0f ||
	//		self->s.origin[1] != 0.0f ||
	//		self->s.origin[2] != 0.0f))
	//	VectorCopy(self->s.origin, self->s.origin2);

	//self->s.weapon = self->number;
}

//	spawnflag 1 enable messages, spawn flag 2 enable sound, 3 is enable both
void Q3R_SP_rally_checkpoint(gentity_t* ent) {

	// this is just a placeholder, we will convert it to normal defrag triggers. just keeping commented code so i know what its supposed to be doing
	InitTrigger(ent);
	trap_LinkEntity(ent);

	level.q3r_numCheckpoints++;

	//trap_SetBrushModel(ent, ent->model);

	//// STONELANCE - April 23, 2002 temp for testing bezier curve stuff
	//ent->r.svFlags |= SVF_BROADCAST;
	////
	//ent->s.eType = ET_CHECKPOINT;

	//ent->think = Think_Checkpoint;
	//ent->nextthink = level.time + 200;

	//ent->touch = Touch_Checkpoint;
	//ent->s.frame = 0;

	//trap_LinkEntity(ent);
}

void SP_HoldableMedkit(gentity_t* ent) {

	gitem_t* item = BG_FindItemForHoldable(HI_MEDPAC);
	G_SpawnItem(ent, item);
	ent->bactaExtra = 25;
}


void DF_InvalidateRunsByStyle(movementStyle_e style) {
	int i;
	gentity_t* ent = g_entities;
	for (i = 0; i < level.maxclients; i++, ent++) {
		
		if (!ent->inuse || !ent->client) {
			continue;
		}
		if (ent->client->sess.raceMode && ent->client->sess.raceStyle.movementStyle == style) {
			DF_RaceStateInvalidated(ent, qtrue);
		}
	}
}

int q2TraceModificationCount = 0;
int q2SkimsModificationCount = 0;
void DF_CheckRaceCvarChanges(qboolean init) {
	if (init) {
		q2TraceModificationCount = g_q2trace.modificationCount;
		q2SkimsModificationCount = g_q2Skims.modificationCount;
		return;
	}
	if (g_q2trace.modificationCount != q2TraceModificationCount) {
		q2TraceModificationCount = g_q2trace.modificationCount;
		DF_InvalidateRunsByStyle(MV_Q2);
	} else if (g_q2Skims.modificationCount != q2SkimsModificationCount) {
		q2SkimsModificationCount = g_q2Skims.modificationCount;
		DF_InvalidateRunsByStyle(MV_Q2);
	}
}




