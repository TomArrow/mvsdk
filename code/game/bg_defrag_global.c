
#include "bg_defrag_global.h"

const int defaultRunFlags = RFL_NODEADRAMPS;

const int allowedRunFlags = RFL_JUMPBUGDISABLE | RFL_NODEADRAMPS | RFL_NOROLLSTART | RFL_BOT | RFL_SEGMENTED | RFL_NOROLLS |RFL_CLIMBTECH;

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

const int MAX_RUN_FLAGS = ARRAY_LEN(runFlagsNames);
