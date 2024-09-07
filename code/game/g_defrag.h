
#ifndef G_DEFRAG_H
#define G_DEFRAG_H

#include "q_shared.h"



typedef struct savedPosition_s {
	playerState_t	ps;
	int				buttons;
	int				oldbuttons;
	int				latched_buttons;
	raceStyle_t		raceStyle;
	int				raceStartCommandTime;
	vec3_t			mins;
	vec3_t			maxs;
} savedPosition_t;

// TODO What if someone touches start trigger, then just stands around forever with start active?
typedef enum segmentedRunState_s {
	SEG_DISABLED,
	SEG_RECORDING, // start position is set and we are recording
	SEG_RECORDING_HAVELASTPOS, // start position is set and last position is set and we are recording
	SEG_RECORDING_INVALIDATED, // means we are in a run, have last position set and cannot savepos, only respos (e.g. after death)
	SEG_REPLAY // we are replaying. do not accept any commaands or whatever.
} segmentedRunState_t;

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
	veci3_t				anglesDiffAccum; // accumulated change in usercmd angles through respos, so we can store the usercmd_t as if the resposes had never happened

	// playback
	int					playbackStartedTime;
	int					playbackNextCmdIndex;
} segmented_t;


void G_TurnDefragTargetsIntoTriggers();
qboolean MovementStyleAllowsWeapons(int moveStyle);
void PlayerSnapshotHackValues(qboolean saveState, int clientNum);
void PlayerSnapshotRestoreValues();
//void DF_ResetSegmentedRun(gentity_t* ent);
//void DF_SegmentedRunStatusInvalidated(gentity_t* ent); // call when something non-deterministic happens (like death). prevents savepos from being used

#endif
