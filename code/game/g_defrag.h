
#ifndef G_DEFRAG_H
#define G_DEFRAG_H

#include "q_shared.h"

#define SEGMENTEDDEBUG 1

#if 0
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? ((assert((client)->pers.cmd.serverTime != 0), (client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#else
#define LEVELTIME(client) (((client) && (client)->sess.raceMode) ? (((client)->pers.cmd.serverTime > 0) ? (client)->pers.cmd.serverTime : level.time) : level.time)
#endif

typedef struct savedPosition_s {
	playerState_t	ps;
	int				buttons;
	int				oldbuttons;
	int				latched_buttons;
	raceStyle_t		raceStyle;
	int				raceStartCommandTime;
	vec3_t			mins;
	vec3_t			maxs;
	int			contents;
} savedPosition_t;

// TODO What if someone touches start trigger, then just stands around forever with start active?
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
