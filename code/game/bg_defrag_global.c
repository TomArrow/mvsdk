#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"


const int defaultRunFlags = RFL_NODEADRAMPS | RFL_LAVAPROTECT | RFL_ANTILOOP;
raceStyle_t defaultRaceStyle;

const int allowedRollRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_LAVAPROTECT | RFL_ANTILOOP;
const int allowedSafeguardedSubcontestRunFlags = ~(RFL_BOT|RFL_TAS);
const int allowedRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_BOT | RFL_TAS | RFL_SEGMENTED | RFL_CLIMBTECH | RFL_JUMPPADCOMPENSATE | RFL_LAVAPROTECT | RFL_ANTILOOP;// | RFL_NOROLLSTART | RFL_NOROLLS;
const int allowedMapDefaultRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_CLIMBTECH | RFL_JUMPPADCOMPENSATE | RFL_LAVAPROTECT;// | RFL_ANTILOOP;// | RFL_NOROLLSTART | RFL_NOROLLS;
#if WIN32// && DEBUG
const int allowedMovementStyles = (1 << MV_JK2) | (1 << MV_SICKO) | (1 << MV_QUAJK) | (1 << MV_BOUNCE) | (1 << MV_CSS) | (1 << MV_Q2) | (1 << MV_FORCE) | (1 << MV_DREAM)| (1 << MV_CHARGEJUMP)| (1 << MV_RATS);// | (1 << MV_PINBALL);
#else
const int allowedMovementStyles = (1 << MV_JK2) | (1 << MV_SICKO) | (1 << MV_QUAJK) | (1 << MV_BOUNCE) | (1 << MV_Q2) | (1 << MV_FORCE) | (1 << MV_DREAM);// | (1 << MV_PINBALL);
#endif
bitInfo_t runFlagsNames[] = { 
	{ "Disable jumpbug" },//0
	{ "Prevent dead ramps" },//1
	{ "No wall stuck" },//2
	{ "No roll start" },//3
	{ "Strafebot" },//4
	{ "Segmented run" },//5
	{ "No rolls" },//6
	{ "TAS mode (!use if you want to script etc!)" },//7
	{ "Climb tech" },//8
	{ "Jumppad FPS compensation" },//9
	{ "Lava protection" },//10
	{ "Anti-Loop" },//11
//	{ "Wallspawn" },//9 // was just a test for db column generation
};

bitInfo_t leaderboardNames[LB_TYPES_COUNT] = {
	{ "Main" },//0
	{ "NoJumpBug" },//1
	{ "Custom" },//2
	{ "Segmented" },//3
	{ "Cheat" },//4
};

#define RUNFLAGSFUNC(a,b,c,d,e,f) {#a},
bitInfo_t runFlagsShortNames[] = {
	RUNFLAGS(RUNFLAGSFUNC)
};
#undef RUNFLAGSFUNC

bitInfo_t runFlagsVeryShortNames[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "njb" },//0
	{ "ndr" },//1
	{ "nws" },//2
	{ "nrs" },//3
	{ "sb" },//4
	{ "seg" },//5
	{ "nr" },//6
	{ "tas" },//7
	{ "clb" },//8
	{ "jpc" },//9
	{ "lvp" },//10
	{ "al" },//11
//	{ "wlsp" },//9 // was just a test for db column generation
};

bitInfoMVStyle_t moveStyleNames[MV_NUMSTYLES] = {
	{ "JK2"},//0
	{ "PJK2" },//1
	{ "JK2SP" },//1
	{ "Speed" },//2
	{ "Sicko" },//3
	{ "QuaJK" },//4
	{ "Bounce" },//5
	{ "Pinball" },//6
	{ "CSS" },//7
	{ "Q2","quake2"},//8
	{ "Force" },//9
	{ "Dream" },//10
	{ "ChargeJump", "charge"},//11
	{ "Rats" },//11
};

const int MAX_RUN_FLAGS = ARRAY_LEN(runFlagsNames);

raceStyle_t getDefaultMapRaceStyle() {
	raceStyle_t df;
	memset(&df, 0, sizeof(df));
	df.movementStyle = MV_JK2;
	df.msec = 8; // this is relevant for jumppad compensation. most maps are from q3.
	df.jumpLevel = 1;
	df.variant = 0;
	df.runFlags = defaultRunFlags;
	return df;
}

const char* getLeaderboardSQLConditions(mainLeaderboardType_t lbType, raceStyle_t* defaultLevelRaceStyle) {
	static char whereString[LB_TYPES_COUNT][MAX_STRING_CHARS];
	if (lbType == LB_CHEAT) {
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`>0 OR `" QUOTEME(RUNFLAGSDBPREFIX) "%s`>0)", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string);
		return whereString[lbType];
	}
	if (lbType == LB_SEGMENTED) { // TODO honestly this sucks, make this readable wtf
		// WHY am i putting the "OR " and "AND " in its own quotes instead of just intoo SUBFUNC? Because QVM preprocessor thinks there shouldn't be an empty space between AND and d then. Wtf? oh well
#define SUBFUNC(a,d)  d ## a != 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e "OR " QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND  `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=1 )", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, runFlagsShortNames[RFLINDEX_SEGMENTED].string
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	if (lbType == LB_CUSTOM) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  d ## a != 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e "OR " QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec != 7 AND msec != 8) "
			"OR jump != %d " 
			RUNFLAGS(RUNFLAGSFUNC)
			"))", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, runFlagsShortNames[RFLINDEX_SEGMENTED].string, defaultLevelRaceStyle->jumpLevel
			RUNFLAGS(RUNFLAGSFUNC2)
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	if (lbType == LB_NOJUMPBUG) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  d ## a = 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e "AND " QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec = 7 OR msec = 8) "
			"AND jump = %d "
			RUNFLAGS(RUNFLAGSFUNC)
			") AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=1)", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, runFlagsShortNames[RFLINDEX_SEGMENTED].string, defaultLevelRaceStyle->jumpLevel
			RUNFLAGS(RUNFLAGSFUNC2)
			, runFlagsShortNames[RFLINDEX_JUMPBUGDISABLE].string
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	if (lbType == LB_MAIN) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  d ## a = 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e "AND " QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec = 7 OR msec = 8) "
			"AND jump = %d "
			RUNFLAGS(RUNFLAGSFUNC)
			") AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0)", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, runFlagsShortNames[RFLINDEX_SEGMENTED].string, defaultLevelRaceStyle->jumpLevel
			RUNFLAGS(RUNFLAGSFUNC2)
			, runFlagsShortNames[RFLINDEX_JUMPBUGDISABLE].string
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	return "";
}

mainLeaderboardType_t classifyLeaderBoard(raceStyle_t* raceStyle, raceStyle_t* defaultLevelRaceStyle) {
	if ((raceStyle->runFlags & RFL_BOT) || (raceStyle->runFlags & RFL_TAS)) {
		return LB_CHEAT;
	}
	if (raceStyle->runFlags & RFL_SEGMENTED) return LB_SEGMENTED;
	//if (raceStyle->movementStyle != MV_JK2) return LB_CUSTOM; // TODO should be its own subcategory altogether?
	if (raceStyle->jumpLevel != defaultLevelRaceStyle->jumpLevel) return LB_CUSTOM;
	//if (raceStyle->variant != defaultLevelRaceStyle->variant) return LB_CUSTOM; // TODO should just be its own course kinda probably
	if (raceStyle->msec != 7 && raceStyle->msec != 8) return LB_CUSTOM;
	if ((raceStyle->runFlags ^ defaultLevelRaceStyle->runFlags) & ~(RFL_JUMPBUGDISABLE)) return LB_CUSTOM; // runFlags differ in a way beyond jumpbug disable
	if (raceStyle->runFlags & RFL_JUMPBUGDISABLE) return LB_NOJUMPBUG;
	return LB_MAIN;
}

// means main main, used for checking if time should appear in tab scoreboard. since there is only one main scoreboard, no flexibility thus.
qboolean RaceStyleIsMainLeaderboard(raceStyle_t* raceStyle, raceStyle_t* defaultRaceStyle) {
	if (raceStyle->movementStyle != MV_JK2) return qfalse;
	if (raceStyle->msec != 7 && raceStyle->msec != 8) return qfalse;
	if (raceStyle->jumpLevel != defaultRaceStyle->jumpLevel) return qfalse;
	if (raceStyle->runFlags != defaultRaceStyle->runFlags) return qfalse;
	if (raceStyle->variant != defaultRaceStyle->variant) return qfalse;
	return qtrue;
}

const char* RunFlagsToString(int runFlags, int defaultRunFlags, int lengthFactor, const char* prefix, const char* suffix) {
	static char s[MAX_STRING_CHARS];
	bitInfo_t* names = runFlagsNames;
	int i;
	qboolean differentFromDefault;
	qboolean isSet;
	qboolean anyContents = qfalse;
	int differences = runFlags ^ defaultRunFlags;
	if (lengthFactor == 0) {
		names = runFlagsVeryShortNames;
	}
	else if (lengthFactor == 1) {
		names = runFlagsShortNames;
	}
	s[0] = 0;
	for (i = 0; i < MAX_RUN_FLAGS; i++) {
		if (!(allowedRunFlags & (1 << i))) continue;
		isSet = runFlags & (1 << i);
		differentFromDefault = differences & (1 << i);
		if (!differentFromDefault) continue;
		if (!anyContents && prefix) {
			Q_strcat(s, sizeof(s), prefix);
		}
		anyContents = qtrue;
		Q_strcat(s, sizeof(s), va("%c%s",isSet ? '+' : '-', names[i].string));
	}
	if (anyContents && suffix) {
		Q_strcat(s, sizeof(s), suffix);
	}
	return s;
}

int RaceNameToInteger(char* style) {
	int i = 0;
	Q_strlwr(style);
	Q_CleanStr(style,qtrue,qtrue);

	for (i = 0; i < MV_NUMSTYLES; i++) {
		if (!Q_stricmp(moveStyleNames[i].string, style) || moveStyleNames[i].alias && !Q_stricmp(moveStyleNames[i].alias, style)) {
			return i;
		}
	}
	return -1;
}
int LeaderboardNameToInteger(char* lbType) {
	Q_strlwr(lbType);
	Q_CleanStr(lbType,qtrue,qtrue);

	if (!Q_stricmp(lbType, "main"))
		return LB_MAIN;
	if (!Q_stricmp(lbType, "njb"))
		return LB_NOJUMPBUG;
	if (!Q_stricmp(lbType, "nojumpbug"))
		return LB_NOJUMPBUG;
	if (!Q_stricmp(lbType, "custom"))
		return LB_CUSTOM;
	if (!Q_stricmp(lbType, "seg"))
		return LB_SEGMENTED;
	if (!Q_stricmp(lbType, "segmented"))
		return LB_SEGMENTED;
	if (!Q_stricmp(lbType, "cheat"))
		return LB_CHEAT;
	return -1;
}
int PlayerModeNameToInteger(char* modeName) {
	Q_strlwr(modeName);
	Q_CleanStr(modeName,qtrue,qtrue);

	if (!Q_stricmp(modeName, "normal"))
		return MODE_NORMAL;
	if (!Q_stricmp(modeName, "defrag") || !Q_stricmp(modeName, "race"))
		return MODE_DEFRAG;
	if (!Q_stricmp(modeName, "duel"))
		return MODE_DUEL;
	if (!Q_stricmp(modeName, "duelq") || !Q_stricmp(modeName, "duelqueue"))
		return MODE_DUELQUEUE;
	if (!Q_stricmp(modeName, "allforce"))
		return MODE_ALLFORCE;
	if (!Q_stricmp(modeName, "ironman"))
		return MODE_IRONMAN;
	return -1;
}
qboolean MovementStyleAllowsWeapons(int moveStyle) {
	return qfalse;
}

int	MovementStyleDisabledRunFlags(int moveStyle) {
	int disallowed = 0;
	if (moveStyle == MV_Q2 || moveStyle == MV_CSS) {
		disallowed |= RFL_BOT; // bot doesnt work for these atm so may as well remove that.
		disallowed |= RFL_CLIMBTECH; // climbtech doesnt work for these atm so may as well remove that.
		disallowed |= RFL_JUMPBUGDISABLE;// not meaningful for these atm
		disallowed |= RFL_NOROLLS; // not meaningful
		disallowed |= RFL_NOROLLSTART;// not meaningful
		disallowed |= RFL_JUMPPADCOMPENSATE; // kinda doesnt work right (yet?) idk. 
		if (moveStyle == MV_CSS) {
			disallowed |= RFL_NODEADRAMPS; // not implemented
		}
	}
	return disallowed;
}

//qboolean MovementStyleHasAntiLoop(int moveStyle) {
//	if (moveStyle == MV_JK2 || moveStyle == MV_SPEED /*|| moveStyle == MV_JK2SP*/) { // is this correct? does sp need antiloop?
//		return qfalse;
//	}
//	return qtrue;
//}
qboolean MovementStyleHasQuake2Ramps(int moveStyle) {
	if (moveStyle == MV_DREAM || moveStyle == MV_QUAJK || moveStyle == MV_SICKO || moveStyle == MV_PINBALL) {
		return qtrue;
	}
	return qfalse;
}
qboolean MovementStyleHasVQ3OnlyJumppads(int moveStyle) {
	if (moveStyle == MV_DREAM || moveStyle == MV_QUAJK || moveStyle == MV_SICKO || moveStyle == MV_PJK2) {
		return qfalse;
	}
	return qtrue;
}
qboolean MovementStyleHasCPMOnlyJumppads(int moveStyle) {
	if (moveStyle == MV_DREAM || moveStyle == MV_QUAJK || moveStyle == MV_SICKO || moveStyle == MV_PJK2) {
		return qtrue;
	}
	return qfalse;
}
qboolean MovementIsQuake3Based(int moveStyle) {
	if (moveStyle == MV_DREAM || moveStyle == MV_QUAJK || moveStyle == MV_SICKO) {
		return qtrue;
	}
	return qfalse;
}
float MovementOverbounceFactor(int moveStyle, playerState_t* ps, usercmd_t* ucmd) {
	if (moveStyle == MV_BOUNCE) {
		if ((ps->stats[STAT_BOUNCEPOWER] & BOUNCEPOWER_POWERMASK) && (ucmd->buttons & BUTTON_BOUNCEPOWER)) {
			return 2.0f;
		}
		return 1.3f;
	}
	else if (moveStyle == MV_PINBALL) {
		return 2.1f;
	}
	//else if (moveStyle == MV_DREAM) {
	//	return 1.1f;
	//}
	return 1.001f; // OVERCLIP define
}

#define MAX_MSTOSTRING_BUFFERS 64
const char* DF_MsToString(const int ms)
{
	static char		string[MAX_MSTOSTRING_BUFFERS][15];	// in case va is called by nested functions
	static int		index = 0;
	char* buf;
	int	timeSec, timeMin, timeMsec;

	buf = string[index & (MAX_MSTOSTRING_BUFFERS -1)];
	index++;

	timeMsec = ms;
	timeSec = timeMsec / 1000;
	timeMsec -= timeSec * 1000;
	timeMin = timeSec / 60;
	timeSec -= timeMin * 60;

	if (!ms) {
		Q_strncpyz(buf,"00:00.000", sizeof(string[0]));
	}
	else {
		Com_sprintf(buf, sizeof(string[0]), "%02i:%02i.%03i", timeMin, timeSec, timeMsec);
	}
	return buf;
	//return !ms ? "00:00.000" : va("%02i:%02i.%03i", timeMin, timeSec, timeMsec);
}

const char* DF_DemoRaceStyleNamePart(raceStyle_t* rs) {
	return va("v%d_%s_%s_j%d%s", rs->variant, rs->movementStyle < MV_NUMSTYLES ? moveStyleNames[rs->movementStyle].string : miniva("UNKNOWN%d",(int)rs->movementStyle),
		rs->msec == -1 ? "togglefps" : (rs->msec == -2 ? "floatphysics" : miniva("%dfps", 1000 / rs->msec))
		, rs->jumpLevel, RunFlagsToString(rs->runFlags, 0, 0, "_", NULL));
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define MAX_MULTIVA_STRING MAX_STRING_CHARS
#define MAX_MULTIVA_BUFFERS 64
char* QDECL multiva(PRINTF_FORMAT_STRING const char* format, ...) {
	va_list		argptr;
	static char		string[MAX_MULTIVA_BUFFERS][MAX_MULTIVA_STRING];	// in case va is called by nested functions
	static int		index = 0;
	char* buf;

	buf = string[index & (MAX_MULTIVA_BUFFERS-1)];
	index++;

	va_start(argptr, format);
	Q_vsnprintf(buf, MAX_MULTIVA_STRING, format, argptr);
	va_end(argptr);

	return buf;
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define MAX_MINIVA_STRING 32
#define MAX_MINIVA_BUFFERS 2048
char* QDECL miniva(PRINTF_FORMAT_STRING const char* format, ...) {
	va_list		argptr;
	static char		string[MAX_MINIVA_BUFFERS][MAX_MINIVA_STRING];	// in case va is called by nested functions
	static int		index = 0;
	char* buf;

	buf = string[index & (MAX_MINIVA_BUFFERS -1)];
	index++;

	va_start(argptr, format);
	Q_vsnprintf(buf, MAX_MINIVA_STRING, format, argptr);
	va_end(argptr);

	return buf;
}





int		fpsTableMsecToIndex[FPSTABLE_OVERFLOW_MSECVALUE + 1];
int		fpsTableIndexToMsec[FPSTABLE_SIZE];

// we wanna be able to store used fps (msec) settings, but there's only a number of values (62 or 63 I think) that can actually be set,
// because for example there are no values between 142 and 125 fps since msec values are integers and 142 is 7 and 125 is 8.
// so in cases where we wanna track how often each fps value was used, we can have a small 64 value array instead of a 1000 value array. less memory, faster to null and copy (e.g. in segmented run)
void	InitFpsTable() {
	int i;
	int index = 0;
	int lastMsec = -1;
	int msec;
	fpsTableMsecToIndex[0] = fpsTableIndexToMsec[0] = 0; // well, 0 msec should never happen, shrug. but just fill it with something to not have freak errors
	for (i = 1; i <= FPSTABLE_MAX_MEASURED_MSECVALUE; i++) {
		msec = 1000 /(1000 / i);
		if (msec != lastMsec) index++;
		lastMsec = msec;
		fpsTableMsecToIndex[i] = index;
		fpsTableIndexToMsec[index] = msec;
	}
	index++;
	fpsTableMsecToIndex[FPSTABLE_OVERFLOW_MSECVALUE] = index;
	for (; index < FPSTABLE_SIZE; index++) {
		fpsTableIndexToMsec[index] = FPSTABLE_OVERFLOW_MSECVALUE; // this "loop" actually just runs once, since i set the array to the needed size. bit random that i can't explain why it has to be exactly that size but it just is that way. wish i could come up with a math formula to do this mapping instead of LUTs
	}
}


void DF_AntiLoop_NewAngle(antiLoopState_t* antiLoopState, vec3_t oldVelocity, vec3_t velocity, float baseSpeed, qboolean inRace) {
	//float xyVel = VectorLength(velocity); //XYSPEED(velocity);
	float xyVel = XYSPEED(velocity); // should prolly be vectorlength but now ppl already did runs like this so i dont wanna mess wiht it (cuz ppl in quajk can crawl up big steep slopes and move dowm with essentially < baseSpeed but high vertical speed but oh well
	if (xyVel < baseSpeed/* && !inRace*/) {
		antiLoopState->yawAngleChangeSinceBaseSpeed = 0;
	}
	else {
		vec3_t velNorm, oldVelNorm;
		vec3_t angles,anglesOld;
		float diff;
		VectorCopy(oldVelocity, oldVelNorm);
		VectorCopy(velocity, velNorm);
		VectorNormalize(oldVelNorm);
		VectorNormalize(velNorm);
		vectoangles(velNorm, angles);
		vectoangles(oldVelNorm, anglesOld);
		diff = AngleSubtract(angles[YAW], anglesOld[YAW]);
		antiLoopState->yawAngleChangeSinceBaseSpeed += fabsf(diff);
	}
}



