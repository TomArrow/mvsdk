#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"


const int defaultRunFlags = RFL_NODEADRAMPS;
raceStyle_t defaultRaceStyle;

const int allowedRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_NOROLLSTART | RFL_BOT | RFL_SEGMENTED | RFL_NOROLLS |RFL_CLIMBTECH;
const int allowedMovementStyles = (1 << MV_JK2) | (1 << MV_SICKO) | (1 << MV_QUAJK) | (1 << MV_BOUNCE);// | (1 << MV_PINBALL);

bitInfo_t runFlagsNames[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Disable jumpbug" },//0
	{ "Prevent dead ramps" },//1
	{ "No wall stuck" },//2
	{ "No roll start" },//3
	{ "Strafebot" },//4
	{ "Segmented run" },//5
	{ "No rolls" },//6
	{ "TAS mode" },//7
	{ "Climb tech" },//8
//	{ "Wallspawn" },//9 // was just a test for db column generation
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
//	{ "wlsp" },//9 // was just a test for db column generation
};

bitInfo_t moveStyleNames[MV_NUMSTYLES] = { 
	{ "JK2" },//0
	{ "PJK2" },//1
	{ "JK2SP" },//1
	{ "Speed" },//2
	{ "Sicko" },//3
	{ "QuaJK" },//4
	{ "Bounce" },//5
	{ "Pinball" },//6
};

const int MAX_RUN_FLAGS = ARRAY_LEN(runFlagsNames);

raceStyle_t getDefaultRaceStyle() {
	raceStyle_t df;
	memset(&df, 0, sizeof(df));
	df.movementStyle = MV_JK2;
	df.msec = 7;
	df.jumpLevel = 1;
	df.variant = 0;
	df.runFlags = defaultRunFlags;
	return df;
}

const char* getLeaderboardSQLConditions(mainLeaderboardType_t lbType, raceStyle_t* defaultLevelRaceStyle) {
	static char whereString[LB_TYPES_COUNT][MAX_STRING_CHARS];
	if (lbType == LB_CHEAT) {
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`>0 OR `" QUOTEME(RUNFLAGSDBPREFIX) "%s`>0)", runFlagsShortNames[RFLINDEX_BOT], runFlagsShortNames[RFLINDEX_TAS]);
		return whereString[lbType];
	}
	if (lbType == LB_CUSTOM) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  OR d ## a != 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec != 7 AND msec != 8) "
			"OR jump != %d " 
			RUNFLAGS(RUNFLAGSFUNC)
			"))", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, defaultLevelRaceStyle->jumpLevel
			RUNFLAGS(RUNFLAGSFUNC2)
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	if (lbType == LB_NOJUMPBUG) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  AND d ## a = 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec = 7 OR msec = 8) "
			"AND jump = %d "
			RUNFLAGS(RUNFLAGSFUNC)
			") AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=1)", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, defaultLevelRaceStyle->jumpLevel
			RUNFLAGS(RUNFLAGSFUNC2)
			, runFlagsShortNames[RFLINDEX_JUMPBUGDISABLE].string
		);
		return whereString[lbType];
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef SUBFUNC
	}
	if (lbType == LB_MAIN) { // TODO honestly this sucks, make this readable wtf
#define SUBFUNC(a,d)  AND d ## a = 
#define RUNFLAGSFUNC(a,b,c,d,e,f) e QUOTEME(SUBFUNC(a,d)) "%d " f
#define RUNFLAGSFUNC2(a,b,c,d,e,f) , (int)!!((int)defaultLevelRaceStyle->runFlags & RFL_ ## b)
		Com_sprintf(whereString[lbType], sizeof(whereString[lbType]), "(`" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0 AND ("
			"(msec = 7 OR msec = 8) "
			"AND jump = %d "
			RUNFLAGS(RUNFLAGSFUNC)
			") AND `" QUOTEME(RUNFLAGSDBPREFIX) "%s`=0)", runFlagsShortNames[RFLINDEX_BOT].string, runFlagsShortNames[RFLINDEX_TAS].string, defaultLevelRaceStyle->jumpLevel
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
	//if (raceStyle->movementStyle != MV_JK2) return LB_CUSTOM; // TODO should be its own subcategory altogether?
	if (raceStyle->jumpLevel != defaultLevelRaceStyle->jumpLevel) return LB_CUSTOM;
	//if (raceStyle->variant != defaultLevelRaceStyle->variant) return LB_CUSTOM; // TODO should just be its own course kinda probably
	if (raceStyle->msec != 7 && raceStyle->msec != 8) return LB_CUSTOM;
	if ((raceStyle->runFlags ^ defaultLevelRaceStyle->runFlags) & ~RFL_JUMPBUGDISABLE) return LB_CUSTOM; // runFlags differ in a way beyond jumpbug disable
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
	Q_strlwr(style);
	Q_CleanStr(style,qtrue,qtrue);

	if (!Q_stricmp(style, "jk2"))
		return MV_JK2;
	if (!Q_stricmp(style, "pjk2"))
		return MV_PJK2;
	if (!Q_stricmp(style, "jk2sp"))
		return MV_JK2SP;
	if (!Q_stricmp(style, "speed"))
		return MV_SPEED;
	if (!Q_stricmp(style, "sicko"))
		return MV_SICKO;
	if (!Q_stricmp(style, "quajk"))
		return MV_QUAJK;
	if (!Q_stricmp(style, "bounce"))
		return MV_BOUNCE;
	if (!Q_stricmp(style, "pinball"))
		return MV_PINBALL;
	return -1;
}
qboolean MovementStyleAllowsWeapons(int moveStyle) {
	return qfalse;
}

qboolean MovementStyleHasQuake2Ramps(int moveStyle) {
	if (moveStyle == MV_QUAJK || moveStyle == MV_SICKO || moveStyle == MV_PINBALL) {
		return qtrue;
	}
	return qfalse;
}
qboolean MovementIsQuake3Based(int moveStyle) {
	if (moveStyle == MV_QUAJK || moveStyle == MV_SICKO) {
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
	return 1.001f; // OVERCLIP define
}


const char* DF_MsToString(const int ms)
{
	static char		string[10][15];	// in case va is called by nested functions
	static int		index = 0;
	char* buf;
	int	timeSec, timeMin, timeMsec;

	buf = string[index & (10-1)];
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


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define MAX_MULTIVA_STRING MAX_STRING_CHARS
#define MAX_MULTIVA_BUFFERS 20
char* QDECL multiva(const char* format, ...) {
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
