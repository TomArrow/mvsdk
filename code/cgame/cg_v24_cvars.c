
#include "cg_local.h"

// Wallhack System CVars
vmCvar_t cg_wallhack;
vmCvar_t cg_wallhackStyle;
vmCvar_t cg_wallhackAlpha;
vmCvar_t cg_wallhackColor;
vmCvar_t cg_wallhackRange;
vmCvar_t cg_wallhackIgnoreFriends;
vmCvar_t cg_wallhackSoundAlert;
vmCvar_t cg_wallhackVisualAlert;
vmCvar_t cg_wallhackPulse;

/*
================
V24 Enhanced Features - CVar Definitions and Registration
================
*/

#include "cg_local.h"

// ============================================================================
// V24 ENHANCED FEATURES - CVAR DEFINITIONS
// ============================================================================

/*
================
V24 Enhanced Features - CVar Definitions and Registration
================
*/

#include "cg_local.h"

// ============================================================================
// V24 ENHANCED FEATURES - CVAR DEFINITIONS
// ============================================================================

// ESP System CVars
vmCvar_t cg_esp;
vmCvar_t cg_espPlayers;
vmCvar_t cg_espItems;
vmCvar_t cg_espDistance;
vmCvar_t cg_espThroughWalls;
vmCvar_t cg_espStyle;
vmCvar_t cg_espAlpha;
vmCvar_t cg_espSize;
vmCvar_t cg_espPlayerNames;
vmCvar_t cg_espItemNames;
vmCvar_t cg_espHealthBars;
vmCvar_t cg_espForceBars;
vmCvar_t cg_espWeaponInfo;
vmCvar_t cg_espBoxes;
vmCvar_t cg_espLines;
vmCvar_t cg_espColorMode;
vmCvar_t cg_espPlayerColor;

// ...rest of file unchanged...
vmCvar_t cg_espLines;
vmCvar_t cg_espColorMode;
vmCvar_t cg_espPlayerColor;
vmCvar_t cg_espEnemyColor;
vmCvar_t cg_espItemColor;
vmCvar_t cg_espFriendColor;
vmCvar_t cg_espMostWantedColor;
vmCvar_t cg_espUpdateRate;
vmCvar_t cg_espMaxEntities;
vmCvar_t cg_espDebug;

// Auto Defense System CVars
vmCvar_t cg_autoDefense;
vmCvar_t cg_autoKick;
vmCvar_t cg_autoKickDistance;
vmCvar_t cg_autoKickAngle;
vmCvar_t cg_autoKickMinDamage;
vmCvar_t cg_autoKickDelay;
vmCvar_t cg_autoKickPrediction;
vmCvar_t cg_autoKickIgnoreFriends;
vmCvar_t cg_autoKickIgnoreSpectators;
vmCvar_t cg_autoKickSoundAlert;
vmCvar_t cg_autoKickVisualAlert;

// Auto-Aim System CVars
vmCvar_t cg_autoAim;
vmCvar_t cg_autoAimFOV;
vmCvar_t cg_autoAimRange;
vmCvar_t cg_autoAimDelay;
vmCvar_t cg_autoAimPrediction;
vmCvar_t cg_autoAimIgnoreFriends;
vmCvar_t cg_autoAimIgnoreSpectators;
vmCvar_t cg_autoAimSoundAlert;
vmCvar_t cg_autoAimVisualAlert;
vmCvar_t cg_autoAimWallPenetrate;
vmCvar_t cg_autoAimDamping;
vmCvar_t cg_autoBackstab;
vmCvar_t cg_autoBackstabDistance;
vmCvar_t cg_autoBackstabAngle;
vmCvar_t cg_autoBackstabDelay;
vmCvar_t cg_autoBackstabIgnoreFriends;
vmCvar_t cg_autoBackstabSoundAlert;

// Enemy Detection System CVars
vmCvar_t cg_enemyDetection;
vmCvar_t cg_enemyDetectionRange;
vmCvar_t cg_enemyDetectionFOV;
vmCvar_t cg_enemyDetectionStyle;
vmCvar_t cg_enemyDetectionColor;
vmCvar_t cg_enemyDetectionSound;
vmCvar_t cg_enemyDetectionPulse;
vmCvar_t cg_enemyDetectionPulseRate;
vmCvar_t cg_enemyDetectionIgnoreFriends;
vmCvar_t cg_enemyDetectionThroughWalls; // Wallhack System CVars

// Auto-Aim System CVars
vmCvar_t cg_autoAim;
vmCvar_t cg_autoAimFOV;
vmCvar_t cg_autoAimRange;
vmCvar_t cg_autoAimDelay;
vmCvar_t cg_autoAimPrediction;
vmCvar_t cg_autoAimIgnoreFriends;
vmCvar_t cg_autoAimIgnoreSpectators;
vmCvar_t cg_autoAimSoundAlert;
vmCvar_t cg_autoAimVisualAlert;
vmCvar_t cg_autoAimWallPenetrate;
vmCvar_t cg_autoAimDamping;

// Friends System CVars
vmCvar_t cg_friendsSystem;
vmCvar_t cg_friendsMaxCount;
vmCvar_t cg_friendsAutoSave;
vmCvar_t cg_friendsVisualMarkers;
vmCvar_t cg_friendsSoundNotifications;

// Cheat Override System CVars
vmCvar_t cg_overrideCheats;
vmCvar_t cg_serverCheatBypass;

// ============================================================================
// V24 ENHANCED FEATURES - CVAR REGISTRATION
// ============================================================================

/*
===================
CG_RegisterV24Cvars
===================
*/
void CG_RegisterV24Cvars(void)
{
    // ESP System CVars
    trap_Cvar_Register(&cg_esp, "cg_esp", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espPlayers, "cg_espPlayers", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espItems, "cg_espItems", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espDistance, "cg_espDistance", "1500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espThroughWalls, "cg_espThroughWalls", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espStyle, "cg_espStyle", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espAlpha, "cg_espAlpha", "1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espSize, "cg_espSize", "1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espPlayerNames, "cg_espPlayerNames", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espItemNames, "cg_espItemNames", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espHealthBars, "cg_espHealthBars", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espForceBars, "cg_espForceBars", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espWeaponInfo, "cg_espWeaponInfo", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espBoxes, "cg_espBoxes", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espLines, "cg_espLines", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espColorMode, "cg_espColorMode", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espPlayerColor, "cg_espPlayerColor", "0.0 1.0 0.0 1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espEnemyColor, "cg_espEnemyColor", "1.0 0.0 0.0 1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espItemColor, "cg_espItemColor", "1.0 1.0 0.0 1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espFriendColor, "cg_espFriendColor", "0.0 0.5 1.0 1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espMostWantedColor, "cg_espMostWantedColor", "1.0 0.5 0.0 1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espUpdateRate, "cg_espUpdateRate", "20", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espMaxEntities, "cg_espMaxEntities", "64", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_espDebug, "cg_espDebug", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Auto Defense System CVars
    trap_Cvar_Register(&cg_autoDefense, "cg_autoDefense", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKick, "cg_autoKick", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickDistance, "cg_autoKickDistance", "100", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickAngle, "cg_autoKickAngle", "120", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickMinDamage, "cg_autoKickMinDamage", "10", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickDelay, "cg_autoKickDelay", "500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickPrediction, "cg_autoKickPrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickIgnoreFriends, "cg_autoKickIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickIgnoreSpectators, "cg_autoKickIgnoreSpectators", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickSoundAlert, "cg_autoKickSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoKickVisualAlert, "cg_autoKickVisualAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Auto-Aim System CVars
    trap_Cvar_Register(&cg_autoAim, "cg_autoAim", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimFOV, "cg_autoAimFOV", "30", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimRange, "cg_autoAimRange", "1500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimDelay, "cg_autoAimDelay", "100", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimPrediction, "cg_autoAimPrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimIgnoreFriends, "cg_autoAimIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimIgnoreSpectators, "cg_autoAimIgnoreSpectators", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimSoundAlert, "cg_autoAimSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimVisualAlert, "cg_autoAimVisualAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimWallPenetrate, "cg_autoAimWallPenetrate", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimDamping, "cg_autoAimDamping", "0.5", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    trap_Cvar_Register(&cg_autoBackstab, "cg_autoBackstab", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoBackstabDistance, "cg_autoBackstabDistance", "200", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoBackstabAngle, "cg_autoBackstabAngle", "60", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoBackstabDelay, "cg_autoBackstabDelay", "250", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoBackstabIgnoreFriends, "cg_autoBackstabIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoBackstabSoundAlert, "cg_autoBackstabSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Enemy Detection System CVars
    trap_Cvar_Register(&cg_enemyDetection, "cg_enemyDetection", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionRange, "cg_enemyDetectionRange", "1000", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionFOV, "cg_enemyDetectionFOV", "90", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionStyle, "cg_enemyDetectionStyle", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionColor, "cg_enemyDetectionColor", "1.0 0.0 0.0 0.5", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionSound, "cg_enemyDetectionSound", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionPulse, "cg_enemyDetectionPulse", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionPulseRate, "cg_enemyDetectionPulseRate", "500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionIgnoreFriends, "cg_enemyDetectionIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_enemyDetectionThroughWalls, "cg_enemyDetectionThroughWalls", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Auto-Aim System CVars
    trap_Cvar_Register(&cg_autoAim, "cg_autoAim", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimFOV, "cg_autoAimFOV", "30.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimRange, "cg_autoAimRange", "2000.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimDelay, "cg_autoAimDelay", "100", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimPrediction, "cg_autoAimPrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimIgnoreFriends, "cg_autoAimIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimIgnoreSpectators, "cg_autoAimIgnoreSpectators", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimSoundAlert, "cg_autoAimSoundAlert", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimVisualAlert, "cg_autoAimVisualAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimWallPenetrate, "cg_autoAimWallPenetrate", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_autoAimDamping, "cg_autoAimDamping", "0.5", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Wallhack System CVars
    trap_Cvar_Register(&cg_wallhack, "cg_wallhack", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackStyle, "cg_wallhackStyle", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackAlpha, "cg_wallhackAlpha", "0.25", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackColor, "cg_wallhackColor", "1.0 0.0 0.0 0.25", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackRange, "cg_wallhackRange", "1500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackIgnoreFriends, "cg_wallhackIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackSoundAlert, "cg_wallhackSoundAlert", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackVisualAlert, "cg_wallhackVisualAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_wallhackPulse, "cg_wallhackPulse", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL);

    // Friends System CVars
    trap_Cvar_Register(&cg_friendsSystem, "cg_friendsSystem", "0", CVAR_ARCHIVE);
    trap_Cvar_Register(&cg_friendsMaxCount, "cg_friendsMaxCount", "64", CVAR_ARCHIVE);
    trap_Cvar_Register(&cg_friendsAutoSave, "cg_friendsAutoSave", "1", CVAR_ARCHIVE);
    trap_Cvar_Register(&cg_friendsVisualMarkers, "cg_friendsVisualMarkers", "1", CVAR_ARCHIVE);
    trap_Cvar_Register(&cg_friendsSoundNotifications, "cg_friendsSoundNotifications", "1", CVAR_ARCHIVE);

    // Cheat Override System CVars
    trap_Cvar_Register(&cg_overrideCheats, "cg_overrideCheats", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
    trap_Cvar_Register(&cg_serverCheatBypass, "cg_serverCheatBypass", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
}
