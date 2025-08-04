
#ifndef BG_DEFRAG_GLOBAL_H
#define BG_DEFRAG_GLOBAL_H

#include "q_shared.h"

#define COURSENAME_MAX_LEN 64
#define USERNAME_MAX_LEN 32

#define BOUNCEPOWER_MAX 500
#define BOUNCEPOWER_REGEN_MAX 100
#define BOUNCEPOWER_POWERMASK ((1<<9)-1)
#define BOUNCEPOWER_REGENMASK (((1<<7)-1)<<9)

#define CHARGEJUMPFLAG_CHARGING		(1<<0)

#define IRONMAN_NEXTCAPPER_TIMEOUT 3000
#define IRONMAN_FLAGRESPAWNTIME IRONMAN_NEXTCAPPER_TIMEOUT-500
#define IRONMAN_JEDIMASTERSHELL_MINDRAWDISTANCE 2000.0f
#define IRONMAN_MAX_PAST_POSITIONS_COUNT 50 // let's be robust
#define IRONMAN_SAVEPOSITION_MINDISTANCE 1500.0f
#define IRONMAN_SAVEPOSITION_MINDISTANCE_SHORT 100.0f
#define IRONMAN_SAVEPOSITION_MINTIMEFORCE 3000
#define IRONMAN_SAVEPOSITION_MINTIMEFORCESURELY 6000
#define IRONMAN_RESPAWNPOSITION_MINDISTANCE 1500.0f
#define IRONMAN_RESPAWNPOSITION_MINDISTANCE_SHORT 350.0f // for emergencies idk
#define IRONMAN_RESPAWNPOSITION_MAXPOSITIONAGE 10000
#define IRONMAN_NEARBYBROADCASTRANGE 1000.0f

typedef enum playerMode_s { // NEVER change the order in this as it's part of the player configstring. If adding something, add it at the end. If adding something in a fork, add 1000 plus a few hundred (check github for other ppl who extended it?) to distinguish from TommyTernal modes, ty.
	MODE_INVALID,
	MODE_NORMAL,
	MODE_DEFRAG,
	MODE_DUEL,
	MODE_ALLFORCE,
	MODE_IRONMAN,
	MODE_NUM_MODES
} playerMode_e;

typedef struct ironManPos_s { // we periodically save ironman pos so we can spawn near him
	vec3_t		origin;
	vec3_t		velocity;
	vec3_t		angles;
	int			when;
} simplePos_t;

typedef enum //defrag movementstyle enum
{
	//DEFRAG_MV_SIEGE,
	DEFRAG_MV_JK2,
	DEFRAG_MV_PJK2,//DEFRAG_MV_BOTJKA,//DEFRAG_MV_QW, // dont make bot its own. just make bot a runflag
	DEFRAG_MV_JK2SP,//DEFRAG_MV_CPM,
	DEFRAG_MV_SPEED,//DEFRAG_MV_Q3,
	DEFRAG_MV_SICKO,//DEFRAG_MV_CLIMB,//DEFRAG_MV_PJK,
	DEFRAG_MV_QUAJK,//DEFRAG_MV_WSW,
	DEFRAG_MV_BOUNCE,//DEFRAG_MV_RJQ3,
	DEFRAG_MV_PINBALL,//DEFRAG_MV_RJCPM,
	DEFRAG_MV_CSS,//DEFRAG_MV_SWOOP,
	DEFRAG_MV_Q2,//DEFRAG_MV_JETPACK,
	DEFRAG_MV_FORCE,//DEFRAG_MV_SPEED,
	DEFRAG_MV_DREAM,//DEFRAG_MV_SP,
	DEFRAG_MV_CHARGEJUMP,//DEFRAG_MV_SLICK,
	//DEFRAG_MV_BOTCPM,
	DEFRAG_MV_NUMSTYLES
} defragMovementStyle_e;

typedef struct bitInfo_s {
	const char* string;
} bitInfo_t;

typedef enum mainLeaderboardType_s {
	LB_MAIN,
	LB_NOJUMPBUG, // main fps but nojumpbug
	LB_CUSTOM, // other fps, segmented, etc
	LB_SEGMENTED, // main fps with or without jumpbug, segmented
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
a(jpadcomp,JUMPPADCOMPENSATE,9,RUNFLAGSDBPREFIX,"","")\
a(lavaProtect,LAVAPROTECT,10,RUNFLAGSDBPREFIX,"","")\
a(antiLoop,ANTILOOP,11,RUNFLAGSDBPREFIX,"","")\
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
extern const int allowedRollRunFlags; // RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS;
extern const int allowedRunFlags; // RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_BOT | RFL_SEGMENTED | RFL_CLIMBTECH | RFL_JUMPPADCOMPENSATE;// | RFL_NOROLLSTART | RFL_NOROLLS;
extern const int allowedMapDefaultRunFlags; //  RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_CLIMBTECH | RFL_JUMPPADCOMPENSATE;// | RFL_NOROLLSTART | RFL_NOROLLS;
extern const int allowedMovementStyles;
extern const int MAX_RUN_FLAGS;

extern bitInfo_t runFlagsNames[];
extern bitInfo_t runFlagsShortNames[];
extern bitInfo_t runFlagsVeryShortNames[];
extern bitInfo_t modeNames[MODE_NUM_MODES];
extern bitInfo_t moveStyleNames[DEFRAG_MV_NUMSTYLES];
extern bitInfo_t leaderboardNames[LB_TYPES_COUNT];




#define Q3SPAWNFLAG_TARGET_SPEED_PERCENTAGE			(1<<0)
#define Q3SPAWNFLAG_TARGET_SPEED_ADD				(1<<1)
#define Q3SPAWNFLAG_TARGET_SPEED_POSX				(1<<2)
#define Q3SPAWNFLAG_TARGET_SPEED_NEGX				(1<<3)
#define Q3SPAWNFLAG_TARGET_SPEED_POSY				(1<<4)
#define Q3SPAWNFLAG_TARGET_SPEED_NEGY				(1<<5)
#define Q3SPAWNFLAG_TARGET_SPEED_POSZ				(1<<6)
#define Q3SPAWNFLAG_TARGET_SPEED_NEGZ				(1<<7)
#define Q3SPAWNFLAG_TARGET_SPEED_LAUNCHER			(1<<8)


#define SF_FINISHTIMER_JUMP1_ONLY							(1<<0)	// japro, irrelevant for us
#define SF_FINISHTIMER_JUMP2_ONLY							(1<<1)	// japro, irrelevant for us
#define SF_FINISHTIMER_JUMP3_ONLY							(1<<2)	// japro, irrelevant for us
#define SF_FINISHTIMER_ALLOW_HASTE							(1<<3)	// japro, irrelevant for us
#define SF_FINISHTIMER_ALLOW_JETPACK						(1<<4)	// japro, irrelevant for us
#define SF_FINISHTIMER_NOT_IN_CTF							(1<<5)	// japro, irrelevant for us
#define SF_FINISHTIMER_ALLOW_MIDMAP_TELES					(1<<6)	// japro, irrelevant for us
#define SF_FINISHTIMER_REQUIRE_CHECKPOINTS					(1<<7)
#define SF_FINISHTIMER_REQUIRE_SPECIFIC_STARTTRIGGER		(1<<8)

#define TTFLAGS_STARTTIMER_Q3RALLYSTYLE						(1<<0)	// don't retrigger if any checkpointscore to avoid killing the run when done

#define TTFLAGS_CHECKPOINTTIMER_SCOREONCE					(1<<0)	// add checkpointScore only once per respawn
#define TTFLAGS_CHECKPOINTTIMER_Q3RALLYSTYLE				(1<<1)	// q3 rally style checkpoint

#define TTFLAGS_FINISHTIMER_SCOREREQUIRE					(1<<0)	// to make q3_fragsFilter work. must have minimum score
#define TTFLAGS_FINISHTIMER_SCOREREQUIRE_SILENT				(1<<1)  // no messages if fail
#define TTFLAGS_FINISHTIMER_SCOREREQUIRE_MATCH				(1<<2)  // must match exactly (can't be over)
#define TTFLAGS_FINISHTIMER_Q3RALLYSTYLE					(1<<3)	// don't retrigger.

#define SF_CHECKPOINT_UNUSED								(1<<0)
#define SF_CHECKPOINT_RESET_PLAYER_TIMER					(1<<1)
#define SF_CHECKPOINT_UNSET_CHECKPOINT						(1<<2)

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
	signed char jumpLevel; // 0=no force, -1 = ysal, -2 = ?, 4=jumpcharge?
	short variant; // when we have map variants (invis walls and such). 0 =default (ignore for now)
	short runFlags; // flags from runFlags_t => STAT_RUNFLAGS
} raceStyle_t;

extern raceStyle_t defaultRaceStyle;

#define UNIX_TIMESTAMP_SHIFT_BILLIONS 3 // increase this in a few decades when unixTimeStampShifted starts overflowing

#define XYSPEED(a) sqrtf((a)[0]*(a)[0]+(a)[1]*(a)[1])

typedef struct checkpointTime_s {
	int time;
	raceStyle_t raceStyle;
	int courseId;
} checkpointTime_t;


typedef enum rollStatus_s {
	ROLL_NONE,
	ROLL_STARTED,
	ROLL_AIR,
	ROLL_TOUCH, // temporary ended (touched ground)
	ROLL_ENDED // finalized value
} rollStatus_t;

typedef enum rollType_s {
	ROLLTYPE_FRONT,
	ROLLTYPE_BACK,
	ROLLTYPE_LEFT,
	ROLLTYPE_RIGHT,
} rollType_t;

typedef struct rollState_s {
	rollStatus_t	status;
	qboolean		rollDisqualified;	// if roll is disqualified for the rollympics (touching a teleport, mover or jumppad, or isn't main LB with jk2 movement style, or has more than 1 segment, or has slide)
	qboolean		segmentDisqualified;	// this is if an air segment has a slide. that and following air segments will disqualify the roll from rollympics
	qboolean		rollStartedInAir;
	qboolean		lastFrameWasRoll;	// wwhether we are rolling before pmove
	rollType_t		rollType;
	float			lastSpeed;			// XY velocity before pmove
	int				lastClientSpeed;	// ps->speed before pmove
	int				lastClientTime;		// commandtime before pmove
	int				airClientSpeed;		// last ps->speed before roll ended
	int				finalAirClientSpeed;// last ps->speed before roll ended (of the roll segment that determined speed)
	float			rollSpeed;			// last XY velocity before roll ended
	int				rollAirTime;
	int				rollAirStarted;
	int				lastRollEndedTime;	// last commandtime before roll ended
} rollState_t;

// for q3 rally maps
typedef struct q3TrackStatus_s {
	qboolean	active;				// is q3rally mode
	qboolean	directionInited;	// set when passing first checkpoint
	qboolean	isReverse;
	int			lastCheckpoint;
} q3TrackStatus_t;

#define ANTILOOP_MAXYAWCHANGE 270
typedef struct antiLoopState_s {
	float						yawAngleChangeSinceBaseSpeed;
} antiLoopState_t;

#define MAX_FPSMEASURE_FRAMECOUNT 32
#define MAX_FPSMEASURE_SHORT_FRAMECOUNT 8

typedef struct fpsMeasure_s {
	short frameTimes[MAX_FPSMEASURE_FRAMECOUNT];
	int index;
} fpsMeasure_t;

typedef enum pbFlags_s { // bit flags
	PB_FIRSTRUN_SPECIFICSTYLE = 1,
	PB_NEWPB_SPECIFICSTYLE = 2,
	PB_LB = 4, // Leaderboards sum up various style ranges. e.g. main LB allows 125 and 142 fps runs etc. so pb in ultra specific style is not same as lb pb
} pbFlags_t;

typedef enum dfTriggerTypes_s {
	DFTRIG_TRIGMULT,	// acrobat maps. first trigger_multiple is start, second is end
	DFTRIG_Q3RALLY,		// q3 rally maps :)
	DFTRIG_TWITIMER,	// one version of acrobat_metal uses this
	DFTRIG_Q3,			// Quake 3 style timers
	DFTRIG_NT_JAPRO,	// NT mod/ japro triggrs
	DFTRIG_TYPES_COUNT
} dfTriggerTypes_t;

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
	int			millisecondsSegmentedTotal;
	float		rollSpeed;
	int			rollTakeoffClientSpeed;
	float		startTriggerSpeed;
	int			pbStatus; // see pbFlags_t
	int			rankLB;
	char		coursename[COURSENAME_MAX_LEN + 1];
	char		subcoursename[COURSENAME_MAX_LEN + 1];
	char		username[USERNAME_MAX_LEN + 1];
	int			unixTimeStampShifted;
	int			unixTimeStampShiftedBillionCount; 
	char		netname[MAX_NETNAME];
	mainLeaderboardType_t lbType;
	char		tempDemoName[MAX_QPATH];
	char		fpsString[255+1];
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

int PlayerModeNameToInteger(char* modeName);
int RaceNameToInteger(char* style);
int LeaderboardNameToInteger(char* lbType);
//qboolean MovementStyleHasAntiLoop(int moveStyle);
qboolean MovementStyleHasQuake2Ramps(int moveStyle);
qboolean MovementStyleHasVQ3OnlyJumppads(int moveStyle);
qboolean MovementStyleHasCPMOnlyJumppads(int moveStyle);
qboolean MovementIsQuake3Based(int moveStyle);
const char* DF_MsToString(const int ms);
const char* RunFlagsToString(int runFlags, int defaultRunFlags, int lengthFactor, const char* prefix, const char* suffix);
qboolean RaceStyleIsMainLeaderboard(raceStyle_t* raceStyle, raceStyle_t* defaultRaceStyle);
mainLeaderboardType_t classifyLeaderBoard(raceStyle_t* raceStyle, raceStyle_t* defaultLevelRaceStyle);
const char* getLeaderboardSQLConditions(mainLeaderboardType_t lbType, raceStyle_t* defaultLevelRaceStyle);
raceStyle_t getDefaultMapRaceStyle();
char* QDECL multiva(const char* format, ...) __attribute__((format(printf, 1, 2)));
char* QDECL miniva(const char* format, ...) __attribute__((format(printf, 1, 2)));
const char* DF_DemoRaceStyleNamePart(raceStyle_t* rs);
void DF_AntiLoop_NewAngle(antiLoopState_t* antiLoopState, vec3_t oldVelocity, vec3_t velocity, float baseSpeed, qboolean inRace);


#define FPSTABLE_SIZE 64 // i already checked, we need roughly 62-63 entries for unqiuely settable fps. just do a bit higher for overflow values. this isnt really depending on any factors, its just how it is, but i can't really give a math formula for WHY it is like that. it just is.

#define FPSTABLE_MAX_MEASURED_MSECVALUE 1000
#define FPSTABLE_OVERFLOW_MSECVALUE FPSTABLE_MAX_MEASURED_MSECVALUE+1

extern int	fpsTableMsecToIndex[FPSTABLE_OVERFLOW_MSECVALUE + 1];
extern int	fpsTableIndexToMsec[FPSTABLE_SIZE];

void		InitFpsTable();

#endif
