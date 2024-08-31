
#ifndef BG_DEFRAG_GLOBAL_H
#define BG_DEFRAG_GLOBAL_H

#include "q_shared.h"

typedef struct bitInfo_s {
	const char* string;
} bitInfo_t;


typedef enum runFlags_s { // 0 is vanilla behavior, 1 is deviation
	RFL_JUMPBUGDISABLE = 1 << 0,
	RFL_NODEADRAMPS = 1 << 1,
	//RFL_NOWALLSTUCK = 1<<2, // just fix by now allowing spawn/respawn/teleport to fuck it
	RFL_NOROLLSTART = 1 << 3,
	RFL_BOT = 1 << 4, // allows strafebot
	RFL_SEGMENTED = 1 << 5, // allows respos
	RFL_NOROLLS = 1 << 6,
	//RFL_TAS = 1 << 7, // absolutely everything is allowed. frametime manipulation etc etc
	RFL_CLIMBTECH = 1 << 8 // jka climb techs
} runFlags_t;

extern const int defaultRunFlags;
extern const int allowedRunFlags; // RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_NOROLLSTART | RFL_BOT | RFL_SEGMENTED | RFL_NOROLLS
extern const int MAX_RUN_FLAGS;

extern bitInfo_t runFlagsNames[];

// can't do this: because qvm has issues compiling shorts :/
//typedef struct raceStyle_s {
//	byte movementStyle; // jk2. maybe some day pjk2 => STAT_MOVEMENTSTYLE
//	short msec; // -1 if toggle, -2 if float (ignore float for now, its cringe anyway)
//	signed char jumpLevel; // 0=no force, -1 = ysal, -2 = ?
//	unsigned short variant; // when we have map variants (invis walls and such). 0 =default (ignore for now)
//	unsigned short runFlags; // flags from runFlags_t => STAT_RUNFLAGS
//} raceStyle_t;
typedef struct raceStyle_s {
	byte movementStyle; // jk2. maybe some day pjk2 => STAT_MOVEMENTSTYLE
	short msec; // -1 if toggle, -2 if float (ignore float for now, its cringe anyway)
	signed char jumpLevel; // 0=no force, -1 = ysal, -2 = ?
	short variant; // when we have map variants (invis walls and such). 0 =default (ignore for now)
	short runFlags; // flags from runFlags_t => STAT_RUNFLAGS
} raceStyle_t;

#endif
