
#ifndef BG_DEFRAG_GLOBAL_H
#define BG_DEFRAG_GLOBAL_H

#include "q_shared.h"

#define BOUNCEPOWER_MAX 500
#define BOUNCEPOWER_REGEN_MAX 100
#define BOUNCEPOWER_POWERMASK ((1<<9)-1)
#define BOUNCEPOWER_REGENMASK (((1<<7)-1)<<9)



typedef enum //movementstyle enum
{
	//MV_SIEGE,
	MV_JK2,
	MV_PJK2,//MV_BOTJKA,//MV_QW, // dont make bot its own. just make bot a runflag
	MV_JK2SP,//MV_CPM,
	MV_SPEED,//MV_Q3,
	MV_SICKO,//MV_CLIMB,//MV_PJK,
	MV_QUAJK,//MV_WSW,
	MV_BOUNCE,//MV_RJQ3,
	MV_PINBALL,//MV_RJCPM,
	//MV_SWOOP,
	//MV_JETPACK,
	//MV_SPEED,
	//MV_SP,
	//MV_SLICK,
	//MV_BOTCPM,
	MV_NUMSTYLES
} movementStyle_e;

typedef struct bitInfo_s {
	const char* string;
} bitInfo_t;

typedef enum mainLeaderboardType_s {
	LB_MAIN,
	LB_NOJUMPBUG, // main fps but nojumpbug
	LB_CUSTOM, // other fps, segmented, etc
	LB_CHEAT, // strafebot, tas
	LB_TYPES_COUNT
} mainLeaderboardType_t;


#define SUBQUOTED(a) #a
#define QUOTEME(a) SUBQUOTED(a)

#define RUNFLAGSDBPREFIX runFlag_

//#define a(a,b,c) // not really used, just to avoid compiler getting mad
#define RUNFLAGS(a)\
a(nojumpbug,JUMPBUGDISABLE,0,RUNFLAGSDBPREFIX," /*","*/ ")\
a(nodeadramps,NODEADRAMPS,1,RUNFLAGSDBPREFIX,"","")\
a(nowallstuck,NOWALLSTUCK,2,RUNFLAGSDBPREFIX,"","")\
a(norollstart,NOROLLSTART,3,RUNFLAGSDBPREFIX,"","")\
a(strafebot,BOT,4,RUNFLAGSDBPREFIX,"","")\
a(segmented,SEGMENTED,5,RUNFLAGSDBPREFIX,"","")\
a(norolls,NOROLLS,6,RUNFLAGSDBPREFIX,"","")\
a(tas,TAS,7,RUNFLAGSDBPREFIX,"","")\
a(climb,CLIMBTECH,8,RUNFLAGSDBPREFIX,"","")\
//a(wallspawn,WALLSPAWN,9,RUNFLAGSDBPREFIX,"","")

// the "/*","*/" thing for JUMPBUGDISABLE is so we can disable it for query construction (since it doesn't need to be identical to the level's default, we still query both)

#define RUNFLAGSFUNC(a,b,c,d,e,f) RFL_ ## b=1<<c,

typedef enum runFlags_s {
	RUNFLAGS(RUNFLAGSFUNC)
	/*// 0 is vanilla behavior, 1 is deviation
	RFL_JUMPBUGDISABLE = 1 << 0,
	RFL_NODEADRAMPS = 1 << 1,
	//RFL_NOWALLSTUCK = 1<<2, // just fix by now allowing spawn/respawn/teleport to fuck it
	RFL_NOROLLSTART = 1 << 3,
	RFL_BOT = 1 << 4, // allows strafebot
	RFL_SEGMENTED = 1 << 5, // allows respos
	RFL_NOROLLS = 1 << 6,
	//RFL_TAS = 1 << 7, // absolutely everything is allowed. frametime manipulation etc etc
	RFL_CLIMBTECH = 1 << 8 // jka climb techs*/
} runFlags_t;
#undef RUNFLAGSFUNC

#define RUNFLAGSFUNC(a,b,c,d,e,f) RFLINDEX_ ## b=c,

typedef enum runFlagsIndex_s {
	RUNFLAGS(RUNFLAGSFUNC)
} runFlagsIndex_t;
#undef RUNFLAGSFUNC

extern const int defaultRunFlags;
extern const int allowedRunFlags; // RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_NOROLLSTART | RFL_BOT | RFL_SEGMENTED | RFL_NOROLLS
extern const int allowedMovementStyles;
extern const int MAX_RUN_FLAGS;

extern bitInfo_t runFlagsNames[];
extern bitInfo_t runFlagsShortNames[];
extern bitInfo_t runFlagsVeryShortNames[];
extern bitInfo_t moveStyleNames[MV_NUMSTYLES];

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

extern raceStyle_t defaultRaceStyle;

#define UNIX_TIMESTAMP_SHIFT_BILLIONS 3 // increase this in a few decades when unixTimeStampShifted starts overflowing

#define XYSPEED(a) sqrtf((a)[0]*(a)[0]+(a)[1]*(a)[1])

typedef enum pbFlags_s { // bit flags
	PB_FIRSTRUN_SPECIFICSTYLE = 1,
	PB_NEWPB_SPECIFICSTYLE = 2,
	PB_LB = 4, // Leaderboards sum up various style ranges. e.g. main LB allows 125 and 142 fps runs etc. so pb in ultra specific style is not same as lb pb
} pbFlags_t;

typedef struct finishedRunInfo_s {
	int			runId;
	int			clientNum;
	int			userId;
	int			milliseconds;
	int			levelTimeStart;
	int			levelTimeEnd;
	int			endCommandTime;
	int			startLessTime;
	int			endLessTime;
	int			warningFlags;
	float		topspeed;
	float		average; // excluding dropped time (due
	float		distance;
	float		distanceXY;
	raceStyle_t raceStyle;
	int			savePosCount;
	int			resposCount;
	int			lostMsecCount;
	int			lostPacketCount;
	int			placeHolder1;
	int			placeHolder2;
	int			placeHolder3;
	int			placeHolder4;
	int			placeHolder5;
	int			placeHolder6;
	int			placeHolder7;
	int			placeHolder8;
	int			pbStatus; // see pbFlags_t
	int			rankLB;
	char		coursename[COURSENAME_MAX_LEN + 1];
	char		username[USERNAME_MAX_LEN + 1];
	int			unixTimeStampShifted;
	int			unixTimeStampShiftedBillionCount; 
	char		netname[MAX_NETNAME];
	mainLeaderboardType_t lbType;
} finishedRunInfo_t;

//typedef struct evaluatedRunInfo_s {
	//int fasterCount;
	//qboolean rankAvailable;
	//qboolean wasLoggedIn;
	//int rank;
	//qboolean newPB;
	//qboolean firstRun;
	//int timeStampMinus3Bill;
//} evaluatedRunInfo_t;

int RaceNameToInteger(char* style);
qboolean MovementStyleHasQuake2Ramps(int moveStyle);
qboolean MovementIsQuake3Based(int moveStyle);
const char* DF_MsToString(const int ms);
const char* RunFlagsToString(int runFlags, int defaultRunFlags, int lengthFactor, const char* prefix, const char* suffix);
qboolean RaceStyleIsMainLeaderboard(raceStyle_t* raceStyle, raceStyle_t* defaultRaceStyle);
mainLeaderboardType_t classifyLeaderBoard(raceStyle_t* raceStyle, raceStyle_t* defaultLevelRaceStyle);
const char* getLeaderboardSQLConditions(mainLeaderboardType_t lbType, raceStyle_t* defaultLevelRaceStyle);
raceStyle_t getDefaultRaceStyle();

#endif
