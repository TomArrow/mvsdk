
#include "bg_defrag_global.h"

const int defaultRunFlags = RFL_NODEADRAMPS;

const int allowedRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_NOROLLSTART | RFL_BOT | RFL_SEGMENTED | RFL_NOROLLS |RFL_CLIMBTECH;
const int allowedMovementStyles = (1 << MV_JK2)| (1 << MV_SICKO)| (1 << MV_QUAJK);

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
};

bitInfo_t moveStyleNames[MV_NUMSTYLES] = { 
	{ "JK2" },//0
	{ "PJK2" },//1
	{ "JK2SP" },//1
	{ "Speed" },//2
	{ "Sicko" },//3
	{ "QuaJK" },//4
};

const int MAX_RUN_FLAGS = ARRAY_LEN(runFlagsNames);


int RaceNameToInteger(char* style) {
	Q_strlwr(style);
	Q_CleanStr(style,qtrue);

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
	return -1;
}
