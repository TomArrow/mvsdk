
#ifndef G_DEFRAG_H
#define G_DEFRAG_H

#include "bg_defrag_global.h"
#include "q_shared.h"

#define SEGMENTEDDEBUG 0

#define QUOT2(a) #a
#define QUOTE(a) QUOT2(a)

// Documenting semi-breaking changes that won't affect most situations/maps, but might potentially impact some.
// With this we can tell old runs from new ones in case issues do arise.
// Version 1: Old
// Version 2: 2025-05-21 - Added support for target_fragsfilter
// Version 3: 2025-05-21 - Added support for target_speed and fixed wait being read as int on trigger_multiple (0.5->0) in conversion
// Version 4: 2026-04-07 - Added ability to discard saved positions in segmented runs, to fix a failed savepos
// Version 5: 2026-04-13 - Added more flexible modelScale system, which affects pmove and cgame prediction. It should not affect normal movement styles, but just for safety, i bump the number. Plus, finally making this number be actually used for g_dfv. I must have forgotten earlier and left it at 1 after originally introducing it.
// Version 6: 2026-05-08 - Turns out... distance values were wrong ALL THIS TIME. And such average. Debug builds zero-initialized displacementVector, but i never wrote to it in the beginning with the new interpolation code.
#define SEMIBREAKINGCHANGEVERSIONDEFRAG 6

extern int semiBreakingChangeVersionDefrag;

#if 0
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? ((assert((client)->pers.cmd.serverTime != 0), (client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#else
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? (((client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#endif
#define MOVESTYLE(client) (((client) && (client)->sess.raceMode) ? (client->sess.raceStyle.movementStyle) : MV_JK2)

//#define ACTIVATORTIMEHELPERTIMEOLD(client) (((client) && (client)->sess.raceMode) ? (((client)->ps.commandTime > 0) ? (client)->ps.commandTime : level.time) : level.time)

//#define ACTIVATORTIME(a) (((a) && (a)->inuse && g_defrag.integer) ? LEVELTIME((a)->client) : level.time)
//#define ACTIVATORTIMEOLD(a) (((a) && (a)->inuse && g_defrag.integer) ? ACTIVATORTIMEHELPERTIMEOLD((a)->client) : level.time)
//#define ACTIVATORTIME(a) level.time
//#define ACTIVATORTIMEOLD(a) level.previousTime


#define MOVERUSESCLIENTTIME(e) (((e)->activatorReal) && ((e)->activatorReal)->inuse && ((e)->activatorReal)->client && ((e)->activatorReal)->client->sess.raceMode && ((e)->activatorReal)->client->pers.cmd.serverTime > 0 && g_defrag.integer)
#define MOVERTIME_ENT(e) (MOVERUSESCLIENTTIME(e) ? (((e)->activatorReal)->client->pers.cmd.serverTime+(e)->activatorLevelTimeDelta) : level.time)
#define MOVERTIMEOLD_ENT(e) ((((e)->activatorReal) && ((e)->activatorReal)->inuse && ((e)->activatorReal)->client && ((e)->activatorReal)->client->sess.raceMode && ((e)->activatorReal)->client->ps.commandTime > 0 && g_defrag.integer) ? (((e)->activatorReal)->client->ps.commandTime+(e)->activatorLevelTimeDelta) : level.time) // is commandtime really adequate?

typedef unsigned char posHashType_t;

typedef struct checkpointSeed_s {
	vec3_t	trEndpos;
	float	anglesYaw;
} checkpointSeed_t;

typedef enum dfWarningFlags_s {
	DF_WARNING_INVALID_PREPMOVE = (1 << 0),
	DF_WARNING_INTERPOLATION_FAIL_START_OVER = (1 << 1),
	DF_WARNING_INTERPOLATION_FAIL_START_EQUAL = (1 << 2),
	DF_WARNING_INTERPOLATION_FAIL_END_OVER = (1 << 3),
	DF_WARNING_INTERPOLATION_FAIL_END_EQUAL = (1 << 4),
	DF_WARNING_INTERPOLATION_FAIL_FALLBACK_NOPRECISE = (1 << 5),
	DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_NOHIT = (1 << 6),
	DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_STARTSOLID = (1 << 7),
	DF_WARNING_INTERPOLATION_FAIL_START_FALLBACK_FRACTION1 = (1 << 8),
	DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_NOHIT = (1 << 9),
	DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_STARTSOLID = (1 << 10),
	DF_WARNING_INTERPOLATION_FAIL_END_FALLBACK_FRACTION1 = (1 << 11),
	DF_WARNING_INVALIDRUNTIME = (1 << 12),
	DF_WARNING_INVALIDRUNDISTANCE = (1 << 13),
} dfWarningFlags_t;

typedef struct runFpsStats_s {
	int		msecCounts[FPSTABLE_SIZE];
	int		totalCount;
} runFpsStats_t;

typedef struct savedPosition_s {
	playerState_t	ps;
	raceStyle_t		raceStyle;
	int				raceStartCommandTime;

	// entity
	int				health;
	qboolean		takedamage;
	int				eventTime;
	int				clipmask; // ?
	int				pain_debounce_time;
	int				fly_sound_debounce_time;
	//int				last_move_time;			// just movers
	//int			count; // idk
	int				watertype;
	int				waterlevel;
	//int				hurt_timestamp;

	//r.
	struct {
		vec3_t			mins;
		vec3_t			maxs;
		vec3_t			currentOrigin;
		//vec3_t			currentAngles; // not used for players?
		//vec3_t			absmax; // done anyway by linkentity?
		//vec3_t			absmin;
		int				contents;
	} r;

	// cl->
	struct {
		int				buttons;
		int				oldbuttons;
		int				latched_buttons;
		int				dangerTime;
		qboolean		fjDidJump;
		int				forcePowerMicroRegenBuffer;
		int				forcePowerSoundDebounce; //if > level.time, don't do certain sound events again (drain sound, absorb sound, etc)
		int				invulnerableTimer;
		int				saberCycleQueue;
		int				damage_armor;		// damage absorbed by armor
		int				damage_blood;		// damage taken out of health
		//int				damage_knockback;	// impact damage (dont see this actually used anywhere atm)
		vec3_t			damage_from;		// origin for vector calculation
		qboolean		damage_fromWorld;	// if true, don't use the damage_from vector
		int				respawnTime;		// can respawn when time > this, force after g_forcerespwan
		int				rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this
		int				airOutTime; 
		qboolean		fireHeld;			// used for hook
		int				timeResidual; 
		vec3_t			lastSaberDir_Always; //every getboltmatrix, set to saber dir
		vec3_t			lastSaberBase_Always; //every getboltmatrix, set to saber base
		int				lastSaberStorageTime; //server time that the above two values were updated (for making sure they aren't out of date)
		qboolean		hasCurrentPosition;	//are lastSaberTip and lastSaberBase valid?
		int				triggerTimes[MAX_GENTITIES]; // to have SLIGHTLY more deterministic behavior with trigger_multiple etc.
		int				entityStates[MAX_GENTITIES]; // allow us to store some simplistic states about other entities, like func_usable. letting us know if the func_usable is turned on/off for this player

		// pers.
		struct {
			struct {
				int			lasthurtcarrier;
				int			lastreturnedflag;
				int			flagsince;
				int			lastfraggedcarrier;
			} teamState;
			struct {
				//int	startLevelTime; // this stuff is more for demo cutting so no need to compensate.
				//int	startLessTime;
				float	distanceTraveled;
				float	distanceTraveled2D;
				float	topSpeed;
				int		checkpoints;
				int score; // target_score uses this in defrag mode
				rollState_t roll;
				q3TrackStatus_t q3RallyState;
				runFpsStats_t	fpsStats;
			} stats;
			struct {
				int			msecTime; 
				int			packetCount;
			} raceDropped;

		} pers;

		// sess.
		struct {
			int				saberLevel;
			int				selectedFP;
			qboolean		setForce;
			//int				updateUITime; // i dont think this is used anywhere.
			qboolean		raceStateSoftInvalidated;	// can still set spawn but not start run. used to prevent teleport starts.
		} sess;
	} client;
} savedPosition_t;


//typedef struct {
//	//char* name;
//	//debugFieldType_t type;
//	size_t	offset;
//	size_t	offsetSavepos;
//	size_t	typeSize;
//	//char* typeName;
//} saveposField_t;

// TODO What if someone touches start trigger, then just stands around forever with start active?
// TODO I meant for this to make the state easier to manage but it actually caused some weird bugs, like
//		SEG_REPLAY > SEG_RECORDING_HAVELASTPOS and thus respos from invalid stored pos. 
//		I added extra checks now but maybe come up with sth better?
typedef enum segmentedRunState_s {
	SEG_DISABLED,
	SEG_RECORDING, // start position is set and we are recording
	SEG_RECORDING_HAVELASTPOS, // start position is set and last position is set and we are recording
	SEG_RECORDING_INVALIDATED, // means we are in a run, have last position set and cannot savepos, only respos (e.g. after death)
	SEG_REPLAY // we are replaying. do not accept any commaands or whatever.
} segmentedRunState_t;

typedef enum soloState_s {
	SOLO_SHOWALL = -1, // show all including machine learning TAS clients
	SOLO_DISABLED,
	SOLO_STYLE,
	SOLO_ALL,
} soloState_t;

typedef enum {
	TRIGPROP_CHECKPOINTSCORE = (1 << 0)
} triggerPropsToSet_t;

typedef struct { // while traversing backwards to find root trigger, all stuff that has to be set on the trigger itself is added here. At the end it is set.
	int	ttFlags;
	int	checkpointScore;
	triggerPropsToSet_t triggerPropsToSet;
	qboolean notVQ3, notCPM;
} triggerConversionProperties_t;

#if SEGMENTEDDEBUG
typedef enum {
	dbgtype_float,
	dbgtype_int,
	dbgtype_vec3_t,
	dbgtype_veci3_t,
	dbgtype_schar_t,
} debugFieldType_t;

typedef struct {
	char* name;
	debugFieldType_t type;
	size_t	offset;
	size_t	offsetDebugVars;
	size_t	typeSize;
	char* typeName;
} debugField_t;


typedef signed char schar_t;

typedef struct {
	struct {
		vec3_t	origin;
		vec3_t	viewangles;
		int		legsAnim;
		int		torsoAnim;
		int		saberMove;
	} ps;
	struct {

		struct {
			veci3_t	angles;
			int buttons;
			schar_t	forwardmove, rightmove, upmove;
		} cmd;
	} pers;
} segDebugVars_t;

extern debugField_t	segDebugFields[];
extern int segDebugFieldsCount;

#endif

#define SEGMENTED_MAX_RESPOS 10
typedef struct segmentedPos_s {
	savedPosition_t		pos;
	int					posIndex;
	int					msecProgress;
	int					userCmdIndex;
	int					resposCount;
	struct {
		int		resposCount;
		int		discardCount;
		int		maxDiscardDepth;
	} discards;
	veci3_t				anglesDiffAccum; // specific to this point. if we decide to remove the saved pos, we need to also add its would-be accum back
} segmentedPos_t;

typedef enum resposType_s {
	RESPOS_NONE,
	RESPOS_NORMAL,
	RESPOS_DISCARD
} resposType_t;

#define RESPOSINDEX(a) (MAX((a),0) % SEGMENTED_MAX_RESPOS)

typedef struct segmented_s {
	segmentedRunState_t	state;

	// requested savepos/respos
	qboolean			savePos;
	resposType_t		respos;

	// general recording state
	savedPosition_t		startPos;
	int					msecProgress;

	// last pos related state
	segmentedPos_t		lastPos[SEGMENTED_MAX_RESPOS];
	int					lastPosCount;
	//int				lowestPosIndex; // it's a circular buffer so if we're e.g. saving index 110, don't allow to go back more than SEGMENTED_MAX_RESPOS-1 as those will be overwritten. actually nvm we'll just save the index inside the array element and compare, similar to snapshots
	veci3_t				anglesDiffAccum; // accumulated change in usercmd angles through any means since last savepos
	veci3_t				anglesDiffAccumActual; // accumulated change in usercmd angles caused by respos, so we can store the usercmd_t as if the resposes had never happened
	qboolean			anglesDiffResettable; // if anglesDiff changed through respawn or such, allow us to restart the segmented recording. but dont do it on pmove angle changes maybe... or? i mean it works fine with just checking accum. but breaks strafebot. make exception for strafebot.

	// playback
	int					playbackStartedTime;
	int					playbackStartedCommandTimeOffset;
	int					playbackNextCmdIndex;
	
	int					totalStartCommandTime;
	int					totalDurationMinusReplay;

	qboolean			playbackErrored;
	int					lastPlaybackErroredCenterprint;

#if SEGMENTEDDEBUG
	//vec3_t				debugOrigin[1000]; // every 1/10 of a second we make a backup here and later we compare
	//vec3_t				debugAngles[1000];
	int					debugTime[1000];
	segDebugVars_t		debugVars[1000];
#endif
} segmented_t;


// User CMD Buffer
// to smooth out ppl who lag and send a lot of cmds at the same time
// guarantee a certain fps of granularity, kinda
#define	USERCMD_BUFFER_MAX 1024
#define USERCMD_BUFFER_CRITICAL_ZONE 128
#define USERCMD_BUFFER_MAX_DELAY 800
#define USERCMD_BUFFER_MAX_FRAMEADVANCE_MAX 100
#define USERCMD_BUFFER_MAX_BLOCKING (USERCMD_BUFFER_MAX-USERCMD_BUFFER_CRITICAL_ZONE)
typedef struct userCmdBuffer_s {
	usercmd_t	buf[USERCMD_BUFFER_MAX];
	int			nextBufferIndex;
	int			nextToExecute;
	int			msecThisFrame;
} userCmdBuffer_t;


#define Q3SPAWNFLAG_TARGET_FRAGSFILTER_REMOVER		(1<<0)
#define Q3SPAWNFLAG_TARGET_FRAGSFILTER_SILENT		(1<<2)
#define Q3SPAWNFLAG_TARGET_FRAGSFILTER_RESET		(1<<3)
#define Q3SPAWNFLAG_TARGET_FRAGSFILTER_MATCH		(1<<4)

extern userCmdBuffer_t		userCmdBuffer[MAX_CLIENTS]; 

typedef enum subContests_s { // always add at the end here cuz entries in the db rely on this staying consistent
	SUBCONTESTS_ROLLYMPICS, // old bugged version. let's make sure it doesnt accidentally conflict
	SUBCONTESTS_ROLLYMPICS_FIX, 
	SUBCONTESTS_DBS_SPEED, 
	SUBCONTESTS_DBS_KILL, 
	SUBCONTESTS_DBS_IRONMAN, 
	SUBCONTESTS_DBS_CTFRETURN, 
	SUBCONTESTS_DBS_KILL_SPEEDLOSS, 
	SUBCONTESTS_DBS_STUPIDSPEED, // dbs speed without any restrictions
	SUBCONTESTS_COUNT
} subContests_t;

typedef enum subContestType_s {
	SUBCONTEST_TYPE_MAXVAL,
	SUBCONTEST_TYPE_MINVAL
} subContestType_t;

typedef enum nameTagType_s {
	NAMETAG_NONE,
	NAMETAG_FREEDOM,
	NAMETAG_OC9,
	NAMETAG_COUNT
} nameTagType_t;

extern const char* nameTagTypeNames[NAMETAG_COUNT];


typedef struct subContestParams_s {
	subContestType_t type;
} subContestParams_t;

typedef struct subContestState_s {
	float			value;
} subContestState_t;

extern subContestParams_t subContestParams[SUBCONTESTS_COUNT];

void G_ConvertDefragTriggerTypes();
qboolean MovementStyleAllowsWeapons(int moveStyle);
void PlayerSnapshotHackValues(qboolean saveState, int clientNum);
void PlayerSnapshotRestoreValues();
int	MovementStyleDisabledRunFlags(int moveStyle);
//void DF_ResetSegmentedRun(gentity_t* ent);
//void DF_SegmentedRunStatusInvalidated(gentity_t* ent); // call when something non-deterministic happens (like death). prevents savepos from being used

void DF_LoadMapDefaults();
void DF_SetMapDefaults(raceStyle_t rs);

void DF_HandleUnfinishedDemos();
const char* DF_FormatFpsString(char* rawFpsString);
void DF_CheckRaceCvarChanges(qboolean init);

const char* DF_GetCourseName(qboolean stripColor);

#endif
