#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

const int defaultRunFlags = RFL_NODEADRAMPS;

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
	int	timeSec, timeMin, timeMsec;

	timeMsec = ms;
	timeSec = timeMsec / 1000;
	timeMsec -= timeSec * 1000;
	timeMin = timeSec / 60;
	timeSec -= timeMin * 60;

	return !ms ? "00:00.000" : va("%02i:%02i.%03i", timeMin, timeSec, timeMsec);
}
