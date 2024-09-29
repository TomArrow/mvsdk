
#ifndef G_DEFRAG_H
#define G_DEFRAG_H

#include "q_shared.h"

#define SEGMENTEDDEBUG 1

#if 0
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? ((assert((client)->pers.cmd.serverTime != 0), (client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#else
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? (((client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#endif


//#define ACTIVATORTIMEHELPERTIMEOLD(client) (((client) && (client)->sess.raceMode) ? (((client)->ps.commandTime > 0) ? (client)->ps.commandTime : level.time) : level.time)

//#define ACTIVATORTIME(a) (((a) && (a)->inuse && g_defrag.integer) ? LEVELTIME((a)->client) : level.time)
//#define ACTIVATORTIMEOLD(a) (((a) && (a)->inuse && g_defrag.integer) ? ACTIVATORTIMEHELPERTIMEOLD((a)->client) : level.time)
//#define ACTIVATORTIME(a) level.time
//#define ACTIVATORTIMEOLD(a) level.previousTime


#define MOVERTIME_ENT(e) ((((e)->activatorReal) && ((e)->activatorReal)->inuse && ((e)->activatorReal)->client && ((e)->activatorReal)->client->sess.raceMode && ((e)->activatorReal)->client->pers.cmd.serverTime > 0 && g_defrag.integer) ? (((e)->activatorReal)->client->pers.cmd.serverTime+(e)->activatorLevelTimeDelta) : level.time)
#define MOVERTIMEOLD_ENT(e) ((((e)->activatorReal) && ((e)->activatorReal)->inuse && ((e)->activatorReal)->client && ((e)->activatorReal)->client->sess.raceMode && ((e)->activatorReal)->client->ps.commandTime > 0 && g_defrag.integer) ? (((e)->activatorReal)->client->ps.commandTime+(e)->activatorLevelTimeDelta) : level.time) // is commandtime really adequate?

typedef enum dfWarningFlags_s {
	DF_WARNING_INVALID_PREPMOVE = (1 << 0),
} dfWarningFlags_t;

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
				float		lasthurtcarrier;
				float		lastreturnedflag;
				float		flagsince;
				float		lastfraggedcarrier;
			} teamState;
		} pers;

		// sess.
		struct {
			int				saberLevel;
			int				selectedFP;
			qboolean		setForce;
			//int				updateUITime; // i dont think this is used anywhere.
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

typedef struct segmented_s {
	segmentedRunState_t	state;

	// requested savepos/respos
	qboolean			savePos;
	qboolean			respos;

	// general recording state
	savedPosition_t		startPos;
	int					msecProgress;

	// last pos related state
	savedPosition_t		lastPos;
	int					lastPosMsecProgress;
	int					lastPosUserCmdIndex;
	veci3_t				anglesDiffAccum; // accumulated change in usercmd angles through any means since last savepos
	veci3_t				anglesDiffAccumActual; // accumulated change in usercmd angles caused by respos, so we can store the usercmd_t as if the resposes had never happened

	// playback
	int					playbackStartedTime;
	int					playbackNextCmdIndex;

#if SEGMENTEDDEBUG
	//vec3_t				debugOrigin[1000]; // every 1/10 of a second we make a backup here and later we compare
	//vec3_t				debugAngles[1000];
	int					debugTime[1000];
	segDebugVars_t		debugVars[1000];
#endif
} segmented_t;




void G_TurnDefragTargetsIntoTriggers();
qboolean MovementStyleAllowsWeapons(int moveStyle);
void PlayerSnapshotHackValues(qboolean saveState, int clientNum);
void PlayerSnapshotRestoreValues();
//void DF_ResetSegmentedRun(gentity_t* ent);
//void DF_SegmentedRunStatusInvalidated(gentity_t* ent); // call when something non-deterministic happens (like death). prevents savepos from being used




#endif
