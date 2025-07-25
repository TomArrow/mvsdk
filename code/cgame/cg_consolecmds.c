// Minimal stub for missing CG_ConsoleCommand implementation
// Function implemented at end of file after all handler functions are defined
#include "cg_local.h"
// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or

// Minimal struct for commands and memecommands arrays
typedef struct
{
	const char *cmd;
} command_entry_t;

// TODO: Fill with actual commands as needed
static command_entry_t commands[] = {
	{"say"},
	{"say_team"},
	{"tell"},
	{"vsay"},
	{"vsay_team"},
	{"vtell"},
	{"vtaunt"},
	{"vosay"},
	{"vosay_team"},
	{"votell"},
	{"give"},
	{"god"},
	{"notarget"},
	{"noclip"},
	{"team"},
	{"levelshot"},
	{"addbot"},
	{"setviewpos"},
	{"callvote"},
	{"vote"},
	{"callteamvote"},
	{"teamvote"},
	{"stats"},
	{"teamtask"},
	{"loaddefered"},
	{"help"},
	{"ignore"},
	{"aminfo"},
	{"amempower"},
	{"ammerc"},
	{"engage_gunduel"},
	{"engage_fullforceduel"},
	{"afk"},
	{"altf"},
	{"ignoreclear"},
	{"ignorelist"},
	{"specs"},
	{"amkick"},
	{"amstatus"},
	{"cp"},
	{"cvars"},
	{"forceteam"},
	{"lockname"},
	{"lockteam"},
	{"mute"},
	{"pause"},
	{"poll"},
	{"swapteams"},
	{"unpause"},
	{"ammodinfo"},
	{"ammodinfo_twitch"},
	{"amadmin"},
	{"channel"},
	{"channellist"},
	{"engage_ff"},
	{"engage_private"},
	{"invite_private"},
	{"accept_private"},
	{"end_private"},
	{"placemodel"},
	{"drop"},
	{"pick"},
	{"remove"},
	{"ambar"},
	{"ambeg"},
	// V24 Enhanced Features Commands
	{"+autobackstab"},
	{"-autobackstab"},
	{"+autodualbackstab"},
	{"-autodualbackstab"},
	{"+autoadvancedbackstab"},
	{"-autoadvancedbackstab"},
	{"+autokick"},
	{"-autokick"},
	{"+autoaim"},
	{"-autoaim"},
	{"esp_toggle"},
	{"esp_players"},
	{"esp_items"},
	{"esp_health_bars"},
	{"esp_force_bars"},
	{"esp_weapon_info"},
	{"esp_boxes"},
	{"esp_lines"},
	{"esp_names"},
	{"esp_through_walls"},
	{"esp_color_mode"},
	{"esp_debug"},
	{"wallhack_toggle"},
	{"wallhack_style"},
	{"enemy_detection"},
	{"friends_system"},
	{"saber_tip_trace"},
	// Add more as needed
};

static command_entry_t memecommands[] = {
	{"meme1"},
	{"meme2"},
	// Add more as needed
};
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "cg_dbcmds.h"
#include "../qcommon/crypt_blowfish.h"
#include "../qcommon/levenshtein.h"
extern menuDef_t *menuScoreboard;

/*
=================
Auto-Backstab System Command Handlers
=================
*/

static void CG_AutoBackstabDown_f(void)
{
	// Set auto-backstab mode to normal backstab (LS_A_BACK)
	cg.doAutoBackstab = qtrue;
	cg.autoBackstabMode = 1; // Normal backstab mode
}

static void CG_AutoBackstabUp_f(void)
{
	cg.doAutoBackstab = qfalse;
}

static void CG_AutoDualBackstabDown_f(void)
{
	// Set auto-backstab mode to crouched backstab (LS_A_BACK_CR)
	cg.doAutoBackstab = qtrue;
	cg.autoBackstabMode = 2; // Crouched backstab mode
}

static void CG_AutoDualBackstabUp_f(void)
{
	cg.doAutoBackstab = qfalse;
}

static void CG_AutoAdvancedBackstabDown_f(void)
{
	// Set auto-backstab mode to advanced air dual backstab
	cg.doAutoBackstab = qtrue;
	cg.autoBackstabMode = 3; // Advanced air dual backstab mode
}

static void CG_AutoAdvancedBackstabUp_f(void)
{
	cg.doAutoBackstab = qfalse;
}

/*
=================
Auto-Kick System Command Handlers
=================
*/

static void CG_AutoKickDown_f(void)
{
	cg.doAutoKick = qtrue;
}

static void CG_AutoKickUp_f(void)
{
	cg.doAutoKick = qfalse;
}

/*
=================
Auto-Aim System Command Handlers
=================
*/

static void CG_AutoAimDown_f(void)
{
	cg.doAutoAim = qtrue;
}

static void CG_AutoAimUp_f(void)
{
	cg.doAutoAim = qfalse;
}

/*
=================
V24 ESP System Command Handlers
=================
*/

static void CG_ESPToggle_f(void)
{
	if (cg_esp.integer)
	{
		trap_Cvar_Set("cg_esp", "0");
		CG_Printf("^3ESP: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_esp", "1");
		CG_Printf("^3ESP: ^7Enabled\n");
	}
}

static void CG_ESPPlayers_f(void)
{
	if (cg_espPlayers.integer)
	{
		trap_Cvar_Set("cg_espPlayers", "0");
		CG_Printf("^3ESP Players: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espPlayers", "1");
		CG_Printf("^3ESP Players: ^7Enabled\n");
	}
}

static void CG_ESPItems_f(void)
{
	if (cg_espItems.integer)
	{
		trap_Cvar_Set("cg_espItems", "0");
		CG_Printf("^3ESP Items: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espItems", "1");
		CG_Printf("^3ESP Items: ^7Enabled\n");
	}
}

static void CG_SaberTipTrace_f(void)
{
	if (cg_saberModelTraceEffect.integer)
	{
		trap_Cvar_Set("cg_saberModelTraceEffect", "0");
		CG_Printf("^3Saber Tip Tracing: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_saberModelTraceEffect", "1");
		CG_Printf("^3Saber Tip Tracing: ^7Enabled\n");
	}
}

// V24 Enhanced Features - Additional ESP Commands
static void CG_ESPHealthBars_f(void)
{
	if (cg_espHealthBars.integer)
	{
		trap_Cvar_Set("cg_espHealthBars", "0");
		CG_Printf("^3ESP Health Bars: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espHealthBars", "1");
		CG_Printf("^3ESP Health Bars: ^7Enabled\n");
	}
}

static void CG_ESPForceBars_f(void)
{
	if (cg_espForceBars.integer)
	{
		trap_Cvar_Set("cg_espForceBars", "0");
		CG_Printf("^3ESP Force Bars: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espForceBars", "1");
		CG_Printf("^3ESP Force Bars: ^7Enabled\n");
	}
}

static void CG_ESPWeaponInfo_f(void)
{
	if (cg_espWeaponInfo.integer)
	{
		trap_Cvar_Set("cg_espWeaponInfo", "0");
		CG_Printf("^3ESP Weapon Info: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espWeaponInfo", "1");
		CG_Printf("^3ESP Weapon Info: ^7Enabled\n");
	}
}

static void CG_ESPBoxes_f(void)
{
	if (cg_espBoxes.integer)
	{
		trap_Cvar_Set("cg_espBoxes", "0");
		CG_Printf("^3ESP Boxes: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espBoxes", "1");
		CG_Printf("^3ESP Boxes: ^7Enabled\n");
	}
}

static void CG_ESPLines_f(void)
{
	if (cg_espLines.integer)
	{
		trap_Cvar_Set("cg_espLines", "0");
		CG_Printf("^3ESP Lines: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espLines", "1");
		CG_Printf("^3ESP Lines: ^7Enabled\n");
	}
}

static void CG_ESPNames_f(void)
{
	if (cg_espPlayerNames.integer)
	{
		trap_Cvar_Set("cg_espPlayerNames", "0");
		CG_Printf("^3ESP Player Names: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espPlayerNames", "1");
		CG_Printf("^3ESP Player Names: ^7Enabled\n");
	}
}

static void CG_ESPThroughWalls_f(void)
{
	if (cg_espThroughWalls.integer)
	{
		trap_Cvar_Set("cg_espThroughWalls", "0");
		CG_Printf("^3ESP Through Walls: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espThroughWalls", "1");
		CG_Printf("^3ESP Through Walls: ^7Enabled\n");
	}
}

static void CG_ESPColorMode_f(void)
{
	int currentMode = cg_espColorMode.integer;
	int nextMode = (currentMode + 1) % 4; // Cycle through 0-3
	trap_Cvar_Set("cg_espColorMode", va("%i", nextMode));

	const char *modeNames[] = {"Default", "Team-based", "Health-based", "Distance-based"};
	CG_Printf("^3ESP Color Mode: ^7%s\n", modeNames[nextMode]);
}

static void CG_ESPDebug_f(void)
{
	if (cg_espDebug.integer)
	{
		trap_Cvar_Set("cg_espDebug", "0");
		CG_Printf("^3ESP Debug: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espDebug", "1");
		CG_Printf("^3ESP Debug: ^7Enabled\n");
	}
}

// V24 Enhanced Features - Wallhack Commands
static void CG_WallhackToggle_f(void)
{
	if (cg_wallhack.integer)
	{
		trap_Cvar_Set("cg_wallhack", "0");
		CG_Printf("^3Wallhack: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_wallhack", "1");
		CG_Printf("^3Wallhack: ^7Enabled\n");
	}
}

static void CG_WallhackStyle_f(void)
{
	int currentStyle = cg_wallhackStyle.integer;
	int nextStyle = (currentStyle + 1) % 3; // Cycle through 0-2
	trap_Cvar_Set("cg_wallhackStyle", va("%i", nextStyle));

	const char *styleNames[] = {"Wireframe", "Solid", "Transparent"};
	CG_Printf("^3Wallhack Style: ^7%s\n", styleNames[nextStyle]);
}

// V24 Enhanced Features - Enemy Detection Commands
static void CG_EnemyDetectionToggle_f(void)
{
	if (cg_enemyDetection.integer)
	{
		trap_Cvar_Set("cg_enemyDetection", "0");
		CG_Printf("^3Enemy Detection: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_enemyDetection", "1");
		CG_Printf("^3Enemy Detection: ^7Enabled\n");
	}
}

// V24 Enhanced Features - Friends System Commands
static void CG_FriendsSystem_f(void)
{
	if (cg_friendsSystem.integer)
	{
		trap_Cvar_Set("cg_friendsSystem", "0");
		CG_Printf("^3Friends System: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_friendsSystem", "1");
		CG_Printf("^3Friends System: ^7Enabled\n");
	}
}

// V24 Enhanced Features - Additional Auto Command Functions
static void CG_AutoDefenseToggle_f(void)
{
	if (cg_autoDefense.integer)
	{
		trap_Cvar_Set("cg_autoDefense", "0");
		CG_Printf("^3Auto Defense: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoDefense", "1");
		CG_Printf("^3Auto Defense: ^7Enabled\n");
	}
}

// Auto-Kick Configuration Functions
static void CG_AutoKickDistance_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Kick Distance: ^7%.0f units\n", cg_autoKickDistance.value);
		CG_Printf("Usage: autokick_distance <distance>\n");
		return;
	}
	float distance = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoKickDistance", va("%.0f", distance));
	CG_Printf("^3Auto-Kick Distance set to: ^7%.0f units\n", distance);
}

static void CG_AutoKickAngle_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Kick Angle: ^7%.0f degrees\n", cg_autoKickAngle.value);
		CG_Printf("Usage: autokick_angle <angle>\n");
		return;
	}
	float angle = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoKickAngle", va("%.0f", angle));
	CG_Printf("^3Auto-Kick Angle set to: ^7%.0f degrees\n", angle);
}

static void CG_AutoKickDelay_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Kick Delay: ^7%d ms\n", cg_autoKickDelay.integer);
		CG_Printf("Usage: autokick_delay <milliseconds>\n");
		return;
	}
	int delay = atoi(CG_Argv(1));
	trap_Cvar_Set("cg_autoKickDelay", va("%d", delay));
	CG_Printf("^3Auto-Kick Delay set to: ^7%d ms\n", delay);
}

static void CG_AutoKickPrediction_f(void)
{
	if (cg_autoKickPrediction.integer)
	{
		trap_Cvar_Set("cg_autoKickPrediction", "0");
		CG_Printf("^3Auto-Kick Prediction: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoKickPrediction", "1");
		CG_Printf("^3Auto-Kick Prediction: ^7Enabled\n");
	}
}

static void CG_AutoKickIgnoreFriends_f(void)
{
	if (cg_autoKickIgnoreFriends.integer)
	{
		trap_Cvar_Set("cg_autoKickIgnoreFriends", "0");
		CG_Printf("^3Auto-Kick Ignore Friends: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoKickIgnoreFriends", "1");
		CG_Printf("^3Auto-Kick Ignore Friends: ^7Enabled\n");
	}
}

static void CG_AutoKickIgnoreSpectators_f(void)
{
	if (cg_autoKickIgnoreSpectators.integer)
	{
		trap_Cvar_Set("cg_autoKickIgnoreSpectators", "0");
		CG_Printf("^3Auto-Kick Ignore Spectators: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoKickIgnoreSpectators", "1");
		CG_Printf("^3Auto-Kick Ignore Spectators: ^7Enabled\n");
	}
}

static void CG_AutoKickSoundAlert_f(void)
{
	if (cg_autoKickSoundAlert.integer)
	{
		trap_Cvar_Set("cg_autoKickSoundAlert", "0");
		CG_Printf("^3Auto-Kick Sound Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoKickSoundAlert", "1");
		CG_Printf("^3Auto-Kick Sound Alert: ^7Enabled\n");
	}
}

static void CG_AutoKickVisualAlert_f(void)
{
	if (cg_autoKickVisualAlert.integer)
	{
		trap_Cvar_Set("cg_autoKickVisualAlert", "0");
		CG_Printf("^3Auto-Kick Visual Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoKickVisualAlert", "1");
		CG_Printf("^3Auto-Kick Visual Alert: ^7Enabled\n");
	}
}

// Auto-Aim Configuration Functions
static void CG_AutoAimFOV_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Aim FOV: ^7%.0f degrees\n", cg_autoAimFOV.value);
		CG_Printf("Usage: autoaim_fov <degrees>\n");
		return;
	}
	float fov = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoAimFOV", va("%.0f", fov));
	CG_Printf("^3Auto-Aim FOV set to: ^7%.0f degrees\n", fov);
}

static void CG_AutoAimRange_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Aim Range: ^7%.0f units\n", cg_autoAimRange.value);
		CG_Printf("Usage: autoaim_range <distance>\n");
		return;
	}
	float range = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoAimRange", va("%.0f", range));
	CG_Printf("^3Auto-Aim Range set to: ^7%.0f units\n", range);
}

static void CG_AutoAimDelay_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Aim Delay: ^7%d ms\n", cg_autoAimDelay.integer);
		CG_Printf("Usage: autoaim_delay <milliseconds>\n");
		return;
	}
	int delay = atoi(CG_Argv(1));
	trap_Cvar_Set("cg_autoAimDelay", va("%d", delay));
	CG_Printf("^3Auto-Aim Delay set to: ^7%d ms\n", delay);
}

static void CG_AutoAimPrediction_f(void)
{
	if (cg_autoAimPrediction.integer)
	{
		trap_Cvar_Set("cg_autoAimPrediction", "0");
		CG_Printf("^3Auto-Aim Prediction: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimPrediction", "1");
		CG_Printf("^3Auto-Aim Prediction: ^7Enabled\n");
	}
}

static void CG_AutoAimDamping_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Aim Damping: ^7%.2f\n", cg_autoAimDamping.value);
		CG_Printf("Usage: autoaim_damping <0.0-1.0>\n");
		return;
	}
	float damping = atof(CG_Argv(1));
	if (damping < 0.0f)
		damping = 0.0f;
	if (damping > 1.0f)
		damping = 1.0f;
	trap_Cvar_Set("cg_autoAimDamping", va("%.2f", damping));
	CG_Printf("^3Auto-Aim Damping set to: ^7%.2f\n", damping);
}

static void CG_AutoAimIgnoreFriends_f(void)
{
	if (cg_autoAimIgnoreFriends.integer)
	{
		trap_Cvar_Set("cg_autoAimIgnoreFriends", "0");
		CG_Printf("^3Auto-Aim Ignore Friends: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimIgnoreFriends", "1");
		CG_Printf("^3Auto-Aim Ignore Friends: ^7Enabled\n");
	}
}

static void CG_AutoAimIgnoreSpectators_f(void)
{
	if (cg_autoAimIgnoreSpectators.integer)
	{
		trap_Cvar_Set("cg_autoAimIgnoreSpectators", "0");
		CG_Printf("^3Auto-Aim Ignore Spectators: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimIgnoreSpectators", "1");
		CG_Printf("^3Auto-Aim Ignore Spectators: ^7Enabled\n");
	}
}

static void CG_AutoAimSoundAlert_f(void)
{
	if (cg_autoAimSoundAlert.integer)
	{
		trap_Cvar_Set("cg_autoAimSoundAlert", "0");
		CG_Printf("^3Auto-Aim Sound Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimSoundAlert", "1");
		CG_Printf("^3Auto-Aim Sound Alert: ^7Enabled\n");
	}
}

static void CG_AutoAimVisualAlert_f(void)
{
	if (cg_autoAimVisualAlert.integer)
	{
		trap_Cvar_Set("cg_autoAimVisualAlert", "0");
		CG_Printf("^3Auto-Aim Visual Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimVisualAlert", "1");
		CG_Printf("^3Auto-Aim Visual Alert: ^7Enabled\n");
	}
}

static void CG_AutoAimWallPenetrate_f(void)
{
	if (cg_autoAimWallPenetrate.integer)
	{
		trap_Cvar_Set("cg_autoAimWallPenetrate", "0");
		CG_Printf("^3Auto-Aim Wall Penetrate: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoAimWallPenetrate", "1");
		CG_Printf("^3Auto-Aim Wall Penetrate: ^7Enabled\n");
	}
}

// Auto-Backstab Configuration Functions
static void CG_AutoBackstabDistance_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Backstab Distance: ^7%.0f units\n", cg_autoBackstabDistance.value);
		CG_Printf("Usage: autobackstab_distance <distance>\n");
		return;
	}
	float distance = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoBackstabDistance", va("%.0f", distance));
	CG_Printf("^3Auto-Backstab Distance set to: ^7%.0f units\n", distance);
}

static void CG_AutoBackstabAngle_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Backstab Angle: ^7%.0f degrees\n", cg_autoBackstabAngle.value);
		CG_Printf("Usage: autobackstab_angle <angle>\n");
		return;
	}
	float angle = atof(CG_Argv(1));
	trap_Cvar_Set("cg_autoBackstabAngle", va("%.0f", angle));
	CG_Printf("^3Auto-Backstab Angle set to: ^7%.0f degrees\n", angle);
}

static void CG_AutoBackstabDelay_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Auto-Backstab Delay: ^7%d ms\n", cg_autoBackstabDelay.integer);
		CG_Printf("Usage: autobackstab_delay <milliseconds>\n");
		return;
	}
	int delay = atoi(CG_Argv(1));
	trap_Cvar_Set("cg_autoBackstabDelay", va("%d", delay));
	CG_Printf("^3Auto-Backstab Delay set to: ^7%d ms\n", delay);
}

static void CG_AutoBackstabIgnoreFriends_f(void)
{
	if (cg_autoBackstabIgnoreFriends.integer)
	{
		trap_Cvar_Set("cg_autoBackstabIgnoreFriends", "0");
		CG_Printf("^3Auto-Backstab Ignore Friends: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoBackstabIgnoreFriends", "1");
		CG_Printf("^3Auto-Backstab Ignore Friends: ^7Enabled\n");
	}
}

static void CG_AutoBackstabSoundAlert_f(void)
{
	if (cg_autoBackstabSoundAlert.integer)
	{
		trap_Cvar_Set("cg_autoBackstabSoundAlert", "0");
		CG_Printf("^3Auto-Backstab Sound Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_autoBackstabSoundAlert", "1");
		CG_Printf("^3Auto-Backstab Sound Alert: ^7Enabled\n");
	}
}

// Additional ESP Configuration Functions
static void CG_ESPDistance_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Distance: ^7%.0f units\n", cg_espDistance.value);
		CG_Printf("Usage: esp_distance <distance>\n");
		return;
	}
	float distance = atof(CG_Argv(1));
	trap_Cvar_Set("cg_espDistance", va("%.0f", distance));
	CG_Printf("^3ESP Distance set to: ^7%.0f units\n", distance);
}

static void CG_ESPStyle_f(void)
{
	int currentStyle = cg_espStyle.integer;
	int nextStyle = (currentStyle + 1) % 3; // Cycle through 0-2
	trap_Cvar_Set("cg_espStyle", va("%i", nextStyle));

	const char *styleNames[] = {"Basic", "Advanced", "Minimal"};
	CG_Printf("^3ESP Style: ^7%s\n", styleNames[nextStyle]);
}

static void CG_ESPAlpha_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Alpha: ^7%.2f\n", cg_espAlpha.value);
		CG_Printf("Usage: esp_alpha <0.0-1.0>\n");
		return;
	}
	float alpha = atof(CG_Argv(1));
	if (alpha < 0.0f)
		alpha = 0.0f;
	if (alpha > 1.0f)
		alpha = 1.0f;
	trap_Cvar_Set("cg_espAlpha", va("%.2f", alpha));
	CG_Printf("^3ESP Alpha set to: ^7%.2f\n", alpha);
}

static void CG_ESPSize_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Size: ^7%.1f\n", cg_espSize.value);
		CG_Printf("Usage: esp_size <size>\n");
		return;
	}
	float size = atof(CG_Argv(1));
	trap_Cvar_Set("cg_espSize", va("%.1f", size));
	CG_Printf("^3ESP Size set to: ^7%.1f\n", size);
}

static void CG_ESPPlayerNames_f(void)
{
	if (cg_espPlayerNames.integer)
	{
		trap_Cvar_Set("cg_espPlayerNames", "0");
		CG_Printf("^3ESP Player Names: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espPlayerNames", "1");
		CG_Printf("^3ESP Player Names: ^7Enabled\n");
	}
}

static void CG_ESPItemNames_f(void)
{
	if (cg_espItemNames.integer)
	{
		trap_Cvar_Set("cg_espItemNames", "0");
		CG_Printf("^3ESP Item Names: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_espItemNames", "1");
		CG_Printf("^3ESP Item Names: ^7Enabled\n");
	}
}

static void CG_ESPPlayerColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Player Color: ^7%s\n", cg_espPlayerColor.string);
		CG_Printf("Usage: esp_player_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_espPlayerColor", CG_Argv(1));
	CG_Printf("^3ESP Player Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_ESPEnemyColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Enemy Color: ^7%s\n", cg_espEnemyColor.string);
		CG_Printf("Usage: esp_enemy_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_espEnemyColor", CG_Argv(1));
	CG_Printf("^3ESP Enemy Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_ESPItemColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Item Color: ^7%s\n", cg_espItemColor.string);
		CG_Printf("Usage: esp_item_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_espItemColor", CG_Argv(1));
	CG_Printf("^3ESP Item Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_ESPFriendColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Friend Color: ^7%s\n", cg_espFriendColor.string);
		CG_Printf("Usage: esp_friend_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_espFriendColor", CG_Argv(1));
	CG_Printf("^3ESP Friend Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_ESPMostWantedColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Most Wanted Color: ^7%s\n", cg_espMostWantedColor.string);
		CG_Printf("Usage: esp_most_wanted_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_espMostWantedColor", CG_Argv(1));
	CG_Printf("^3ESP Most Wanted Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_ESPUpdateRate_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Update Rate: ^7%d ms\n", cg_espUpdateRate.integer);
		CG_Printf("Usage: esp_update_rate <milliseconds>\n");
		return;
	}
	int rate = atoi(CG_Argv(1));
	trap_Cvar_Set("cg_espUpdateRate", va("%d", rate));
	CG_Printf("^3ESP Update Rate set to: ^7%d ms\n", rate);
}

static void CG_ESPMaxEntities_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3ESP Max Entities: ^7%d\n", cg_espMaxEntities.integer);
		CG_Printf("Usage: esp_max_entities <count>\n");
		return;
	}
	int max = atoi(CG_Argv(1));
	trap_Cvar_Set("cg_espMaxEntities", va("%d", max));
	CG_Printf("^3ESP Max Entities set to: ^7%d\n", max);
}

// Additional Wallhack Functions
static void CG_WallhackAlpha_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Wallhack Alpha: ^7%.2f\n", cg_wallhackAlpha.value);
		CG_Printf("Usage: wallhack_alpha <0.0-1.0>\n");
		return;
	}
	float alpha = atof(CG_Argv(1));
	if (alpha < 0.0f)
		alpha = 0.0f;
	if (alpha > 1.0f)
		alpha = 1.0f;
	trap_Cvar_Set("cg_wallhackAlpha", va("%.2f", alpha));
	CG_Printf("^3Wallhack Alpha set to: ^7%.2f\n", alpha);
}

static void CG_WallhackColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Wallhack Color: ^7%s\n", cg_wallhackColor.string);
		CG_Printf("Usage: wallhack_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_wallhackColor", CG_Argv(1));
	CG_Printf("^3Wallhack Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_WallhackRange_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Wallhack Range: ^7%.0f units\n", cg_wallhackRange.value);
		CG_Printf("Usage: wallhack_range <distance>\n");
		return;
	}
	float range = atof(CG_Argv(1));
	trap_Cvar_Set("cg_wallhackRange", va("%.0f", range));
	CG_Printf("^3Wallhack Range set to: ^7%.0f units\n", range);
}

static void CG_WallhackIgnoreFriends_f(void)
{
	if (cg_wallhackIgnoreFriends.integer)
	{
		trap_Cvar_Set("cg_wallhackIgnoreFriends", "0");
		CG_Printf("^3Wallhack Ignore Friends: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_wallhackIgnoreFriends", "1");
		CG_Printf("^3Wallhack Ignore Friends: ^7Enabled\n");
	}
}

static void CG_WallhackSoundAlert_f(void)
{
	if (cg_wallhackSoundAlert.integer)
	{
		trap_Cvar_Set("cg_wallhackSoundAlert", "0");
		CG_Printf("^3Wallhack Sound Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_wallhackSoundAlert", "1");
		CG_Printf("^3Wallhack Sound Alert: ^7Enabled\n");
	}
}

static void CG_WallhackVisualAlert_f(void)
{
	if (cg_wallhackVisualAlert.integer)
	{
		trap_Cvar_Set("cg_wallhackVisualAlert", "0");
		CG_Printf("^3Wallhack Visual Alert: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_wallhackVisualAlert", "1");
		CG_Printf("^3Wallhack Visual Alert: ^7Enabled\n");
	}
}

static void CG_WallhackPulse_f(void)
{
	if (cg_wallhackPulse.integer)
	{
		trap_Cvar_Set("cg_wallhackPulse", "0");
		CG_Printf("^3Wallhack Pulse: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_wallhackPulse", "1");
		CG_Printf("^3Wallhack Pulse: ^7Enabled\n");
	}
}

// Additional Enemy Detection Functions
static void CG_EnemyDetectionRange_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Enemy Detection Range: ^7%.0f units\n", cg_enemyDetectionRange.value);
		CG_Printf("Usage: enemy_detection_range <distance>\n");
		return;
	}
	float range = atof(CG_Argv(1));
	trap_Cvar_Set("cg_enemyDetectionRange", va("%.0f", range));
	CG_Printf("^3Enemy Detection Range set to: ^7%.0f units\n", range);
}

static void CG_EnemyDetectionFOV_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Enemy Detection FOV: ^7%.0f degrees\n", cg_enemyDetectionFOV.value);
		CG_Printf("Usage: enemy_detection_fov <degrees>\n");
		return;
	}
	float fov = atof(CG_Argv(1));
	trap_Cvar_Set("cg_enemyDetectionFOV", va("%.0f", fov));
	CG_Printf("^3Enemy Detection FOV set to: ^7%.0f degrees\n", fov);
}

static void CG_EnemyDetectionStyle_f(void)
{
	int currentStyle = cg_enemyDetectionStyle.integer;
	int nextStyle = (currentStyle + 1) % 3; // Cycle through 0-2
	trap_Cvar_Set("cg_enemyDetectionStyle", va("%i", nextStyle));

	const char *styleNames[] = {"Highlight", "Box", "Arrow"};
	CG_Printf("^3Enemy Detection Style: ^7%s\n", styleNames[nextStyle]);
}

static void CG_EnemyDetectionColor_f(void)
{
	if (trap_Argc() != 2)
	{
		CG_Printf("^3Enemy Detection Color: ^7%s\n", cg_enemyDetectionColor.string);
		CG_Printf("Usage: enemy_detection_color <color>\n");
		return;
	}
	trap_Cvar_Set("cg_enemyDetectionColor", CG_Argv(1));
	CG_Printf("^3Enemy Detection Color set to: ^7%s\n", CG_Argv(1));
}

static void CG_EnemyDetectionSound_f(void)
{
	if (cg_enemyDetectionSound.integer)
	{
		trap_Cvar_Set("cg_enemyDetectionSound", "0");
		CG_Printf("^3Enemy Detection Sound: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_enemyDetectionSound", "1");
		CG_Printf("^3Enemy Detection Sound: ^7Enabled\n");
	}
}

static void CG_EnemyDetectionPulse_f(void)
{
	if (cg_enemyDetectionPulse.integer)
	{
		trap_Cvar_Set("cg_enemyDetectionPulse", "0");
		CG_Printf("^3Enemy Detection Pulse: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_enemyDetectionPulse", "1");
		CG_Printf("^3Enemy Detection Pulse: ^7Enabled\n");
	}
}

// Friends System Command Functions
static void CG_FriendAdd_f(void)
{
	int clientNum;
	if (trap_Argc() != 2)
	{
		clientNum = CG_CrosshairPlayer();
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			CG_Printf("^3Usage: friend_add <player_number> or aim at a player\n");
			return;
		}
	}
	else
	{
		clientNum = atoi(CG_Argv(1));
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			CG_Printf("^1Invalid player number\n");
			return;
		}
	}
	CG_AddFriend(clientNum);
}

static void CG_FriendRemove_f(void)
{
	int clientNum;
	if (trap_Argc() != 2)
	{
		clientNum = CG_CrosshairPlayer();
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			CG_Printf("^3Usage: friend_remove <player_number> or aim at a player\n");
			return;
		}
	}
	else
	{
		clientNum = atoi(CG_Argv(1));
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			CG_Printf("^1Invalid player number\n");
			return;
		}
	}
	CG_RemoveFriend(clientNum);
}

static void CG_FriendList_f(void)
{
	CG_ListFriends();
}

static void CG_FriendClear_f(void)
{
	CG_ClearFriends();
}

static void CG_FriendsVisualMarkers_f(void)
{
	if (cg_friendsVisualMarkers.integer)
	{
		trap_Cvar_Set("cg_friendsVisualMarkers", "0");
		CG_Printf("^3Friends Visual Markers: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_friendsVisualMarkers", "1");
		CG_Printf("^3Friends Visual Markers: ^7Enabled\n");
	}
}

static void CG_FriendsSoundNotifications_f(void)
{
	if (cg_friendsSoundNotifications.integer)
	{
		trap_Cvar_Set("cg_friendsSoundNotifications", "0");
		CG_Printf("^3Friends Sound Notifications: ^7Disabled\n");
	}
	else
	{
		trap_Cvar_Set("cg_friendsSoundNotifications", "1");
		CG_Printf("^3Friends Sound Notifications: ^7Enabled\n");
	}
}

void CG_TargetCommand_f(void)
{
	int targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum)
	{
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendConsoleCommand(va("gc %i %i\n", targetNum, atoi(test)));
}

/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer + 10)));
}

/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer - 10)));
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f(void)
{
	CG_Printf("%s" S_COLOR_WHITE " (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
			  (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
			  (int)cg.refdefViewAngles[YAW]);
}

/*
=============
CG_Viewaxis_f

Debugging command to print the current view axis
=============
*/
static void CG_Viewaxis_f(void)
{
	vec3_t viewAxis[3];
	AnglesToAxis(cg.refdefViewAngles, viewAxis);
	CG_Printf("%s" S_COLOR_WHITE " (%f %f %f),(%f %f %f),(%f %f %f)\n", cgs.mapname,
			  viewAxis[0][0], viewAxis[0][1], viewAxis[0][2],
			  viewAxis[1][0], viewAxis[1][1], viewAxis[1][2],
			  viewAxis[2][0], viewAxis[2][1], viewAxis[2][2]);
}

static void CG_ScoresDown_f(void)
{

	CG_BuildSpectatorString();
	if (!cg.demoPlayback && cg.scoresRequestTime + 2000 < cg.time)
	{ // don't clear the scoreboard when watching a demo
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if (!cg.showScores)
		{
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	}
	else
	{
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f(void)
{
	if (cg.showScores)
	{
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

extern menuDef_t *menuScoreboard;
void Menu_Reset(); // FIXME: add to right include file

static void CG_scrollScoresDown_f(void)
{
	if (menuScoreboard && cg.scoreBoardShowing)
	{
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}

static void CG_scrollScoresUp_f(void)
{
	if (menuScoreboard && cg.scoreBoardShowing)
	{
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}

static void CG_spWin_f(void)
{
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", cgs.screenHeight * .30, 0);
}

static void CG_spLose_f(void)
{
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", cgs.screenHeight * .30, 0);
}

static void CG_TellTarget_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_VoiceTellTarget_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_VoiceTellAttacker_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

qboolean CG_StringIsDigitsOnly(const char *buf)
{
	int i;
	const int len = strlen(buf);

	for (i = 0; i < len; ++i)
	{
		if (buf[i] >= '0' && buf[i] <= '9')
		{
			// its a digit 0-9, cool!
		}
		else
		{
			return qfalse;
		}
	}

	return qtrue;
}

char *stristr(const char *str, const char *charset)
{
	int i;

	while (*str)
	{
		for (i = 0; charset[i] && str[i]; ++i)
		{
			if (toupper(charset[i]) != toupper(str[i]))
			{
				break;
			}
		}
		if (!charset[i])
		{
			return (char *)str;
		}
		str++;
	}

	return NULL;
}

int CG_FindPlayerFromString(const char *buf, int trySkip, int skipClient)
{
	int client = -1;
	int k;

#ifdef XDEVELOPER
	if (x3_debug.integer)
	{
		CG_Printf("CG_FindPlayerFromString '%s' %d, %d,\n", buf, trySkip, skipClient);
	}
#endif

	if (CG_StringIsDigitsOnly(buf))
	{

		client = atoi(buf);

		if (client >= 0 && client < MAX_CLIENTS && cgs.clientinfo[client].infoValid)
		{ //
			return client;
		}
	}

	else if (!Q_stricmp(buf, "self") || !strcmp(buf, "me"))
	{
		return cg.clientNum;
	}

	// If we got here, it means our argument wasnt a (valid) clientnumber, so do substring search.
	// NEW feature: start by doing case sensitive string compare, then case insensitive string compare, then case sensitive substring search. if this fails, do case insensitive! GENIUS!

	for (k = 0; k < 8; ++k)
	{
		clientInfo_t *ci;
		int i;

		for (i = 0, ci = cgs.clientinfo; i < MAX_CLIENTS; ++i, ++ci)
		{
			char nameS[256] = {0};
			const char *name;

			if (!ci || !ci->infoValid)
				continue;

			if (skipClient != -1)
			{

				if (i == skipClient)
					continue;
			}

			if ((trySkip & TRYSKIP_SPECTATORS) && ci->team == TEAM_SPECTATOR)
			{
				continue;
			}
			if ((trySkip & TRYSKIP_SELF) && i == cg.clientNum)
			{
				continue;
			}

			if (k % 2)
			{
				// for k=1,3,5,7.. we will clean the name to see if we can match then
				Q_strncpyz(nameS, ci->name, sizeof(nameS));
				Q_CleanStr(nameS, qtrue, cgs.isTommyTernal);
				name = nameS;
			}
			else
			{
				name = ci->name;
			}

			switch (k)
			{
			case 0:
			case 1:
				if (!strcmp(name, buf))
					return i; // Case sensitive string compare.
				break;
			case 2:
			case 3:
				if (!Q_stricmp(name, buf))
					return i; // Case insensitive string compare.
				break;
			case 4:
			case 5:
				if (strstr(name, buf))
					return i; // Do case sensitive substring search now.
				break;
			case 6:
			case 7:
				if (stristr(name, buf))
					return i; // case insensitive substring search now.
				break;
			}
		}
	}

	if (trySkip > 0)
	{
		return CG_FindPlayerFromString(buf, 0, skipClient);
	}
	else if (skipClient != -1)
	{
		return CG_FindPlayerFromString(buf, trySkip, -1);
	}

	Com_Printf("Couldn't find player \"%s^7\"!\n", buf);

	return -1; // Client isnt valid
}

qboolean CG_FindClientFromConsoleArgs(int *clientvar, int tryskip, int skipclient)
{
	// Return qtrue if found match (no error); return qfalse if error! (couldn't find player)
	char buf[64] = {0};
	int client = -1;
	int i = 0;

	if (trap_Argc() < 2)
	{
		Com_Printf("No player name/client number specified.\n", buf);
		return qfalse; // Error: no argument!
	}

	trap_Args(buf, sizeof(buf)); // set buf = the argument input by the user. trap_args just copies entire argument string, i.e. all arguments, not a specific one. which means it allows spaces.

	client = CG_FindPlayerFromString(buf, tryskip, skipclient);

	if (client == -1)
		return qfalse;

	*clientvar = client;
	return qtrue;
}

static const char *NOT_PLAYING_DEMO = "Not playing a demo.\n";

static void CG_DemoSeekRetMode_f(void)
{

	if (!cg.demoPlayback)
	{
		Com_Printf(NOT_PLAYING_DEMO);
		return;
	}

	if (!(cg.demoseek & DEMOSEEK_RETMODE))
	{
		cg.demoseek |= DEMOSEEK_RETMODE;
		Com_Printf("Demoseek Retmode activated.\n");
	}
	else
	{
		cg.demoseek &= ~DEMOSEEK_RETMODE;
		Com_Printf("Demoseek Retmode deactivated.\n");
	}
}

void CG_DemoSeekToCappingOnly_f(void)
{

	if (!cg.demoPlayback)
	{
		Com_Printf(NOT_PLAYING_DEMO);
		return;
	}

	// Set the timescale value very high until our playerstate in a demo has the flag., used for analyzing capping demos
	if (!(cg.demoseek & DEMOSEEK_CAPPING_ONLY))
	{
		cg.demoseek |= DEMOSEEK_CAPPING_ONLY;
		Com_Printf("Demoseek: seeking until a capper is being followed...\n");

		if (cg.demoseek & DEMOSEEK_RETMODE)
			cg.demoseek &= ~DEMOSEEK_RETMODE;
	}
	else
	{
		cg.demoseek &= ~DEMOSEEK_CAPPING_ONLY;
		Com_Printf("Demoseek: No longer seeking until a capper is being followed...\n");
	}
}

void CG_DemoSeekClientNum_f(void)
{
	char buf[96] = {0};
	int client;

	if (!cg.demoPlayback)
	{
		Com_Printf(NOT_PLAYING_DEMO);
		return;
	}

	trap_Args(buf, sizeof(buf));

	if (trap_Argc() < 2 || !Q_stricmp("clear", buf) || !Q_stricmp("none", buf) || !Q_stricmp("stop", buf) || !Q_stricmp("-1", buf))
	{
		cg.demoseek &= ~DEMOSEEK_SPECIFIC_CLIENT_ONLY;
		Com_Printf("Demoseek: No longer seeking until a certain client is being followed...\n");
		return;
	}

	if (!CG_FindClientFromConsoleArgs(&client, TRYSKIP_SPECTATORS, -1))
		return;

	if (cg.demoseek & DEMOSEEK_SPECIFIC_CLIENT_ONLY && cg.demoseekClientNum == client)
	{
		cg.demoseek &= ~DEMOSEEK_SPECIFIC_CLIENT_ONLY;
		Com_Printf("Demoseek: No longer seeking until a certain client is being followed...\n");
		return;
	}

	if (cgs.clientinfo[client].infoValid /*&& cgs.clientinfo[client].team != TEAM_SPECTATOR*/)
	{
		cg.demoseek |= DEMOSEEK_SPECIFIC_CLIENT_ONLY;
		cg.demoseekClientNum = client;

		// cg.dodemofollow = qtrue;
		// cg.demofollowvis = qfalse;
		cg.demofollowclient = client;

		Com_Printf("Now seeking demo unless (%i) %s ^7is being followed\n", client, cgs.clientinfo[client].name);
	}
	else
	{
		Com_Printf("Client %s ^7(%i) isn't valid or is on spectator team.\n", cgs.clientinfo[client].name, client);
	}
}

const char *timescaleString = "timescale";

static void CG_DemoSeekToMapRestart_f(void)
{

	if (!cg.demoPlayback)
	{
		Com_Printf(NOT_PLAYING_DEMO);
		return;
	}

	if (cg.demoseek & DEMOSEEK_MAPRESTART)
	{
		Com_Printf("No longer seeking until a map restart. \n");
		cg.demoseek &= ~DEMOSEEK_MAPRESTART;
		return;
	}

	cg.demoseek |= DEMOSEEK_MAPRESTART;
	Com_Printf("Now seeking in demo until a map restart. \n");
}

static void CG_DemoSeekStop_f(void)
{
	cg.demoseekClientNum = 0;
	cg.demoseek = 0;
	Com_Printf("Demoseek: no longer seeking at all.\n");
	trap_Cvar_Set(timescaleString, "1");
}

static void CG_NextTeamMember_f(void)
{
	CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f(void)
{
	CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f(void)
{
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci)
	{
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum)
		{
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP)
	{
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE)
		{
			if (!CG_OtherTeamHasFlag())
			{
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT)
		{
			if (!CG_YourTeamHasFlag())
			{
				cgs.currentOrder++;
			}
		}
	}
	else
	{
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}

static void CG_ConfirmOrder_f(void)
{
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5\n");
	if (cg.time < cgs.acceptOrderTime)
	{
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f(void)
{
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6\n");
	if (cg.time < cgs.acceptOrderTime)
	{
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f(void)
{
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY)
	{
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	}
	else
	{
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f(void)
{
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f(void)
{
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f(void)
{
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f(void)
{
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f(void)
{
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f(void)
{
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f(void)
{
	int clientNum;
	char command[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1)
	{
		return;
	}

	Com_sprintf(command, 128, "tell %i suicide", clientNum);
	trap_SendClientCommand(command);
}

/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
	CG_EventHandling(CGAME_EVENT_NONE);
	trap_Key_SetCatcher(0);
  } else {
	CG_EventHandling(CGAME_EVENT_TEAMMENU);
	//trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f(void)
{
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if (!atoi(var))
	{
		return;
	}
	if (cg_cameraOrbit.value != 0)
	{
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	}
	else
	{
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/

// jk2pro stuff
typedef struct bitInfo_S
{
	const char *string;
} bitInfo_T;

static bitInfo_T strafeTweaks[] = {
	{"Original style"}, // 0
	{"Updated style"},	// 1
	{"Cgaz style"},		// 2
	{"Warsow style"},	// 3
	{"Sound"},			// 4
	{"W"},				// 5
	{"WA"},				// 6
	{"WD"},				// 7
	{"A"},				// 8
	{"D"},				// 9
	{"Rear"},			// 10
	{"Center"},			// 11
	{"Accel bar"},		// 12
	{"Weze style"},		// 13
	{"Line Crosshair"}, // 14
	{"RealAccel"}		// 15
};
static const int MAX_STRAFEHELPER_TWEAKS = ARRAY_LEN(strafeTweaks);

void CG_StrafeHelper_f(void)
{
	if (trap_Argc() == 1)
	{
		int i = 0;
		for (i = 0; i < MAX_STRAFEHELPER_TWEAKS; i++)
		{
			if ((cg_strafeHelper.integer & (1 << i)))
			{
				Com_Printf("%2d [X] %s\n", i, strafeTweaks[i].string);
			}
			else
			{
				Com_Printf("%2d [ ] %s\n", i, strafeTweaks[i].string);
			}
		}
		return;
	}
	else
	{
		char arg[8] = {0};
		int index;
		const uint32_t mask = (1 << MAX_STRAFEHELPER_TWEAKS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index < 0 || index >= MAX_STRAFEHELPER_TWEAKS)
		{
			Com_Printf("strafeHelper: Invalid range: %i [0, %i]\n", index, MAX_STRAFEHELPER_TWEAKS - 1);
			return;
		}

		if ((index == 0 || index == 1 || index == 2 || index == 3 || index == 13))
		{ // Radio button these options
		  // Toggle index, and make sure everything else in this group (0,1,2,3,13) is turned off
			int groupMask = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 13);
			int value = cg_strafeHelper.integer;

			groupMask &= ~(1 << index); // Remove index from groupmask
			value &= ~(groupMask);		// Turn groupmask off
			value ^= (1 << index);		// Toggle index item

			trap_Cvar_Set("cg_strafeHelper", va("%i", value));
		}
		else
		{
			trap_Cvar_Set("cg_strafeHelper", va("%i", (1 << index) ^ (cg_strafeHelper.integer & mask)));
		}
		trap_Cvar_Update(&cg_strafeHelper);

		Com_Printf("%s %s^7\n", strafeTweaks[index].string, ((cg_strafeHelper.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T speedometerSettings[] = {
	// MAX_WEAPON_TWEAKS tweaks (24)
	{"Enable speedometer"},								  // 0
	{"Pre-speed display"},								  // 1
	{"Jump height display"},							  // 2
	{"Jump distance display"},							  // 3
	{"Vertical speed indicator"},						  // 4
	{"Yaw speed indicator"},							  // 5
	{"Accel meter"},									  // 6
	{"Speed graph"},									  // 7
	{"Display speed in kilometers instead of units"},	  // 8
	{"Display speed in imperial miles instead of units"}, // 9
	{"Disable speed display"},							  // 10
	{"Accel miss"},										  // 11
	{"Z position display"},								  // 12
};
static const int MAX_SPEEDOMETER_SETTINGS = ARRAY_LEN(speedometerSettings);

void cg_speedometer_f(void)
{
	if (trap_Argc() == 1)
	{
		int i = 0, display = 0;

		for (i = 0; i < MAX_SPEEDOMETER_SETTINGS; i++)
		{
			if ((cg_speedometer.integer & (1 << i)))
			{
				Com_Printf("%2d [X] %s\n", display, speedometerSettings[i].string);
			}
			else
			{
				Com_Printf("%2d [ ] %s\n", display, speedometerSettings[i].string);
			}
			display++;
		}
		return;
	}
	else
	{
		char arg[8] = {0};
		int index, index2;
		const uint32_t mask = (1 << MAX_SPEEDOMETER_SETTINGS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		if (index2 < 0 || index2 >= MAX_SPEEDOMETER_SETTINGS)
		{
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_SPEEDOMETER_SETTINGS - 1);
			return;
		}

		if (index == 8 || index == 9)
		{ // Radio button these options
			// Toggle index, and make sure everything else in this group (8,9) is turned off
			int groupMask = (1 << 8) + (1 << 9);
			int value = cg_speedometer.integer;

			groupMask &= ~(1 << index); // Remove index from groupmask
			value &= ~(groupMask);		// Turn groupmask off
			value ^= (1 << index);		// Toggle index item

			trap_Cvar_Set("cg_speedometer", va("%i", value));
		}
		else
		{
			trap_Cvar_Set("cg_speedometer", va("%i", (1 << index) ^ (cg_speedometer.integer & mask)));
		}
		trap_Cvar_Update(&cg_speedometer);

		Com_Printf("%s %s^7\n", speedometerSettings[index2].string, ((cg_speedometer.integer & (1 << index2)) ? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T customizeRaceSettings[] = {
	// MAX_WEAPON_TWEAKS tweaks (24)
	{"Hide roll speed centerprints"},				 // 0
	{"Hide checkpoint centerprints"},				 // 1
	{"Hide anti-loop restart blocked centerprints"}, // 2
	{"Hide non-warning race start centerprints"},	 // 3
	{"Hide CTF messages in racemode"},				 // 4
													 //{ "Auto-respawn when antiloop is triggered" },//5
};
static const int MAX_CUSTOMIZERACE_SETTINGS = ARRAY_LEN(customizeRaceSettings);

void cg_customizeRace_f(void)
{
	if (trap_Argc() == 1)
	{
		int i = 0, display = 0;

		for (i = 0; i < MAX_CUSTOMIZERACE_SETTINGS; i++)
		{
			if ((cg_customizeRace.integer & (1 << i)))
			{
				Com_Printf("%2d [X] %s\n", display, customizeRaceSettings[i].string);
			}
			else
			{
				Com_Printf("%2d [ ] %s\n", display, customizeRaceSettings[i].string);
			}
			display++;
		}
		return;
	}
	else
	{
		char arg[8] = {0};
		int index, index2;
		const uint32_t mask = (1 << MAX_CUSTOMIZERACE_SETTINGS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		if (index2 < 0 || index2 >= MAX_CUSTOMIZERACE_SETTINGS)
		{
			Com_Printf("customize race: Invalid range: %i [0, %i]\n", index2, MAX_CUSTOMIZERACE_SETTINGS - 1);
			return;
		}

		trap_Cvar_Set("cg_customizeRace", va("%i", (1 << index) ^ (cg_customizeRace.integer & mask)));
		trap_Cvar_Update(&cg_customizeRace);

		Com_Printf("%s %s^7\n", customizeRaceSettings[index2].string, ((cg_customizeRace.integer & (1 << index2)) ? "^2Enabled" : "^1Disabled"));
	}
}

void CG_SanitizeString2(const char *in, char *out)
{
	int i = 0, r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH - 1)
		{ // the ui truncates the name here..
			break;
		}
		if (in[i] == '^')
		{
			if (in[i + 1] >= 48 && in[i + 1] <= 57)
			{ // only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ // just skip the ^
				i++;
				continue;
			}
		}
		if (in[i] < 32)
		{
			i++;
			continue;
		}
		out[r] = tolower(in[i]); // lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}

int CG_ClientNumberFromString(const char *s)
{
	clientInfo_t *cl;
	int idnum, i, match = -1;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];
	idnum = atoi(s);

	if (s[0] == '-')
	{ //- returns id of target in our crosshair
		idnum = CG_CrosshairPlayer();
		return idnum;
	}

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) // changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid)
		{
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2)) // changed and to or ..
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid)
		{
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &cgs.clientinfo[idnum];
		if (!cl->infoValid)
		{
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	CG_SanitizeString2(s, s2);
	for (idnum = 0, cl = cgs.clientinfo; idnum < cgs.maxclients; ++idnum, ++cl)
	{
		if (!cl->infoValid)
		{
			continue;
		}
		CG_SanitizeString2(cl->name, n2);

		for (i = 0; i < cgs.maxclients; i++)
		{
			cl = &cgs.clientinfo[i];
			if (!cl->infoValid)
			{
				continue;
			}
			CG_SanitizeString2(cl->name, n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ // found more than one match
					Com_Printf("More than one user '%s' on the server\n", s);
					return -2;
				}
				match = i;
			}
		}
		if (match != -1) // uhh
			return match;
	}
	if (!atoi(s)) // Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		Com_Printf("User '%s' is not on the server\n", s);
	return -1;
}

void CG_Say_f(void)
{
	char msg[MAX_SAY_TEXT] = {0};
	char word[MAX_SAY_TEXT] = {0};
	char numberStr[MAX_SAY_TEXT] = {0};
	int i, number = 0, numWords = trap_Argc();
	int clientNum = -1, messagetype = 0;

	if (!Q_stricmp(CG_Argv(0), "say"))
	{
		messagetype = 1;
	}
	else if (!Q_stricmp(CG_Argv(0), "say_team"))
	{
		messagetype = 2;
	}
	else if (!Q_stricmp(CG_Argv(0), "tell"))
	{
		clientNum = CG_ClientNumberFromString(CG_Argv(1));
		messagetype = 3;
		if (clientNum < 0) // couldn't find target or multiple matches found
			return;
	}
	else
	{ // shouldn't happen...
		return;
	}

	for (i = 1; i < numWords; i++)
	{
		if (i == 1 && clientNum > -1) // skip 1st argument in PM since that's the name of the person we're trying to PM
			continue;
		trap_Argv(i, word, sizeof(word));

		if (!Q_stricmp(word, "%H%"))
		{
			number = cg.predictedPlayerState.stats[STAT_HEALTH];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%S%"))
		{
			number = cg.predictedPlayerState.stats[STAT_ARMOR];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%F%"))
		{
			number = cg.predictedPlayerState.fd.forcePower;
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%W%"))
		{
			number = cg.predictedPlayerState.weapon;
			switch (number)
			{
			case 1:
				Com_sprintf(numberStr, sizeof(numberStr), "Stun baton");
				break;
			case 2:
				Com_sprintf(numberStr, sizeof(numberStr), "Melee");
				break;
			case 4:
				Com_sprintf(numberStr, sizeof(numberStr), "Bryar");
				break;
			case 5:
				Com_sprintf(numberStr, sizeof(numberStr), "E11");
				break;
			case 6:
				Com_sprintf(numberStr, sizeof(numberStr), "Sniper");
				break;
			case 7:
				Com_sprintf(numberStr, sizeof(numberStr), "Bowcaster");
				break;
			case 8:
				Com_sprintf(numberStr, sizeof(numberStr), "Repeater");
				break;
			case 9:
				Com_sprintf(numberStr, sizeof(numberStr), "Demp2");
				break;
			case 10:
				Com_sprintf(numberStr, sizeof(numberStr), "Flechette");
				break;
			case 11:
				Com_sprintf(numberStr, sizeof(numberStr), "Rocket");
				break;
			case 12:
				Com_sprintf(numberStr, sizeof(numberStr), "Thermal");
				break;
			case 13:
				Com_sprintf(numberStr, sizeof(numberStr), "Tripmine");
				break;
			case 14:
				Com_sprintf(numberStr, sizeof(numberStr), "Detpack");
				break;
			default:
				Com_sprintf(numberStr, sizeof(numberStr), "Saber");
				break;
			}
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%A%"))
		{
			number = cg.predictedPlayerState.ammo[weaponData[cg.predictedPlayerState.weapon].ammoIndex];
			Com_sprintf(numberStr, sizeof(numberStr), "%i", number);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
#ifndef Q3_VM
		else if (!Q_stricmp(word, "%T%"))
		{ // insert time in 12-hour format
			struct tm *newtime;
			qboolean AM = qtrue;
			time_t rawtime;
			time(&rawtime);
			newtime = localtime(&rawtime);
			if (newtime->tm_hour >= 12)
				AM = qfalse;
			if (newtime->tm_hour > 12)
				newtime->tm_hour -= 12;
			if (newtime->tm_hour == 0)
				newtime->tm_hour = 12;
			Com_sprintf(numberStr, sizeof(numberStr), "%i:%02i %s", newtime->tm_hour, newtime->tm_min, AM ? "AM" : "PM");
			Q_strncpyz(word, numberStr, sizeof(word));
		}
		else if (!Q_stricmp(word, "%T2%"))
		{ // insert time in 24-hour format
			struct tm *newtime;
			time_t rawtime;
			time(&rawtime);
			newtime = localtime(&rawtime);
			Com_sprintf(numberStr, sizeof(numberStr), "%02i:%02i", newtime->tm_hour, newtime->tm_min);
			Q_strncpyz(word, numberStr, sizeof(word));
		}
#endif

		Q_strcat(word, MAX_SAY_TEXT, " ");
		Q_strcat(msg, MAX_SAY_TEXT, word);
	}

	switch (messagetype)
	{
	default:
		CG_Printf("%sUnrecognized command %s\n", S_COLOR_YELLOW, CG_Argv(0));
		break;
	case 1:
		trap_SendClientCommand(va("say %s", msg));
		break;
	case 2:
		trap_SendClientCommand(va("say_team %s", msg));
		break;
	case 3:
		if (clientNum > -1)
			trap_SendClientCommand(va("tell %i %s", clientNum, msg));
		break;
	}
}

void CG_ClientList_f(void)
{
	clientInfo_t *ci;
	int i;
	int count = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
			continue;

		switch (ci->team)
		{
		case TEAM_FREE:
			if (cgs.isCTFMod && cgs.CTF3ModeActive)
			{
				CG_Printf("%2d " S_COLOR_YELLOW "Y   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
						  ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			}
			else
			{
				CG_Printf("%2d " S_COLOR_YELLOW "F   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
						  ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			}
			break;

		case TEAM_RED:
			CG_Printf("%2d " S_COLOR_RED "R   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					  ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;

		case TEAM_BLUE:
			CG_Printf("%2d " S_COLOR_BLUE "B   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					  ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;

		default:
		case TEAM_SPECTATOR:
			CG_Printf("%2d " S_COLOR_YELLOW "S   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					  ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;
		}

		count++;
	}

	CG_Printf("Listed %2d clients\n", count);
}

static void CG_ModVersion_f(void)
{
	CG_Printf("^5Your client version of the mod was compiled on %s at %s\n", __DATE__, __TIME__);
	// trap_SendConsoleCommand("ui_modversion\n");
	if (cgs.isJK2Pro)
	{
		trap_SendClientCommand("modversion");
	}
}

static void CG_Follow_f(void)
{
	int clientNum = -1;

	if (trap_Argc() < 2)
	{
		CG_Printf("usage /follow <name>\n");
		return;
	}

	clientNum = CG_ClientNumberFromString(CG_Argv(1));

	if (clientNum < 0)
		return;

	CG_SendConsoleCommand("cmd follow %i", clientNum);
	cg.lastManualCommandInterruptingAutoFollow = cg.time;
}

static void CG_Login_f(void)
{
	int clientNum = -1;
	char cmd[64];
	char username[64];
	char pw[64];
	char thirdarg[64];
	const static char settings[64] = BCRYPT_SETTINGS;
	char output[64];

	if (trap_Argc() < 3)
	{
		if (cgs.isTommyTernal)
		{
			CG_Printf("usage /login <username> <password> [raw] (password is clientside-hashed before sending to server by default for TommyTernal servers. Pass 'raw' to override)\n");
		}
		else
		{
			CG_Printf("usage /login <username> <password> [bcrypt] (use bcrypt to hash pw clientside before sending to server)\n");
		}
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, pw, sizeof(pw));
	trap_Argv(3, thirdarg, sizeof(thirdarg));

	if (!Q_stricmp(thirdarg, "raw"))
	{
		CG_SendConsoleCommand("cmd %s \"%s\" \"%s\" raw", cmd, username, pw);
	}
	else if (cgs.isTommyTernal)
	{

		bcrypt_errno = 0;
		_crypt_blowfish_rn(pw, settings, output, 64);

		CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);

		if (!bcrypt_errno)
		{
			CG_SendConsoleCommand("cmd %s \"%s\" \"%s\" bcrypt", cmd, username, output);
		}
		else
		{
			CG_Printf("Clientside bcrypt hashing of password failed. Use '/login <username> <password> raw' to send password to the server in plaintext.\n");
		}
	}
	else
	{
		// Allow
		trap_Argv(3, thirdarg, sizeof(thirdarg));
		if (!Q_stricmp(thirdarg, "bcrypt"))
		{

			bcrypt_errno = 0;
			_crypt_blowfish_rn(pw, settings, output, 64);

			CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);
			if (!bcrypt_errno)
			{
				CG_SendConsoleCommand("cmd %s \"%s\" \"%s\"", cmd, username, output);
			}
			else
			{
				CG_Printf("Requested clientside bcrypt hashing of password failed. Please report this issue.\n");
			}
		}
		else
		{
			CG_SendConsoleCommand("cmd %s \"%s\" \"%s\"", cmd, username, pw);
		}
	}

	// clientNum = CG_ClientNumberFromString(CG_Argv(1));

	// if (clientNum < 0)
	//	return;

	// CG_SendConsoleCommand("cmd follow %i", clientNum);
}

static void CG_Register_f(void)
{
	int clientNum = -1;
	char cmd[64];
	char username[64];
	char pw[64];
	char thirdarg[64];
	const static char settings[64] = BCRYPT_SETTINGS;
	char output[64];

	if (trap_Argc() < 3)
	{
		if (cgs.isTommyTernal)
		{
			CG_Printf("usage /register <username> <password> [raw] (password is clientside-hashed before sending to server by default for TommyTernal servers. Pass 'raw' to override)\n");
		}
		else
		{
			CG_Printf("usage /register <username> <password> [bcrypt] (use bcrypt to hash pw clientside before sending to server)\n");
		}
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, pw, sizeof(pw));

	trap_Argv(3, thirdarg, sizeof(thirdarg));
	if (!Q_stricmp(thirdarg, "raw"))
	{
		CG_SendConsoleCommand("cmd %s \"%s\" \"%s\" raw", cmd, username, pw);
	}
	else if (cgs.isTommyTernal)
	{

		if (!BG_DB_VerifyPassword(pw, -1))
		{
			return;
		}

		bcrypt_errno = 0;
		_crypt_blowfish_rn(pw, settings, output, 64);

		CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);

		if (!bcrypt_errno)
		{
			CG_SendConsoleCommand("cmd %s \"%s\" \"%s\" bcrypt", cmd, username, output);
		}
		else
		{
			CG_Printf("Clientside bcrypt hashing of password failed. Use '/register <username> <password> raw' to send password to the server in plaintext.\n");
		}
	}
	else
	{
		// Allow
		trap_Argv(3, thirdarg, sizeof(thirdarg));
		if (!Q_stricmp(thirdarg, "bcrypt"))
		{

			bcrypt_errno = 0;
			_crypt_blowfish_rn(pw, settings, output, 64);

			CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);
			if (!bcrypt_errno)
			{
				CG_SendConsoleCommand("cmd %s \"%s\" \"%s\"", cmd, username, output);
			}
			else
			{
				CG_Printf("Requested clientside bcrypt hashing of password failed. Please report this issue.\n");
			}
		}
		else
		{
			CG_SendConsoleCommand("cmd %s \"%s\" \"%s\"", cmd, username, pw);
		}
	}

	// clientNum = CG_ClientNumberFromString(CG_Argv(1));

	// if (clientNum < 0)
	//	return;

	// CG_SendConsoleCommand("cmd follow %i", clientNum);
}

static void CG_ChangePassword_f(void)
{
	int clientNum = -1;
	char cmd[64];
	char pw[64];
	char secondarg[64];
	const static char settings[64] = BCRYPT_SETTINGS;
	char output[64];

	if (trap_Argc() < 2)
	{
		if (cgs.isTommyTernal)
		{
			CG_Printf("usage /changepassword <password> [raw] (password is clientside-hashed before sending to server by default for TommyTernal servers. Pass 'raw' to override)\n");
		}
		else
		{
			CG_Printf("usage /changepassword <password> [bcrypt] (use bcrypt to hash pw clientside before sending to server)\n");
		}
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(1, pw, sizeof(pw));

	trap_Argv(2, secondarg, sizeof(secondarg));
	if (!Q_stricmp(secondarg, "raw"))
	{
		CG_SendConsoleCommand("cmd %s \"%s\" raw", cmd, pw);
	}
	else if (cgs.isTommyTernal)
	{

		if (!BG_DB_VerifyPassword(pw, -1))
		{
			return;
		}

		bcrypt_errno = 0;
		_crypt_blowfish_rn(pw, settings, output, 64);

		CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);

		if (!bcrypt_errno)
		{
			CG_SendConsoleCommand("cmd %s \"%s\" bcrypt", cmd, output);
		}
		else
		{
			CG_Printf("Clientside bcrypt hashing of password failed. Use '/changepassword <password> raw' to send password to the server in plaintext.\n");
		}
	}
	else
	{
		// Allow
		trap_Argv(2, secondarg, sizeof(secondarg));
		if (!Q_stricmp(secondarg, "bcrypt"))
		{

			bcrypt_errno = 0;
			_crypt_blowfish_rn(pw, settings, output, 64);

			CG_DPrintf("cg bcrypt; settings: %s\nRaw pw: %s, bcrypt: %s, bcrypt_errno: %d\n", settings, pw, output, bcrypt_errno);
			if (!bcrypt_errno)
			{
				CG_SendConsoleCommand("cmd %s \"%s\"", cmd, output);
			}
			else
			{
				CG_Printf("Requested clientside bcrypt hashing of password failed. Please report this issue.\n");
			}
		}
		else
		{
			CG_SendConsoleCommand("cmd %s \"%s\"", cmd, pw);
		}
	}

	// clientNum = CG_ClientNumberFromString(CG_Argv(1));

	// if (clientNum < 0)
	//	return;

	// CG_SendConsoleCommand("cmd follow %i", clientNum);
}

static void CG_FollowRedFlag_f(void)
{
	int i;
	clientInfo_t *ci;

	if (!cg.snap)
		return;

	for (i = 0; i < cgs.maxclients; i++)
	{
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[i];

		if (ci->powerups & (1 << PW_REDFLAG))
		{
			CG_SendConsoleCommand("cmd follow %i", i);
			cg.lastManualCommandInterruptingAutoFollow = cg.time;
			return;
		}
	}
}

static void CG_FollowBlueFlag_f(void)
{
	int i;
	clientInfo_t *ci;

	if (!cg.snap)
		return;

	for (i = 0; i < cgs.maxclients; i++)
	{
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[i];

		if (ci->powerups & (1 << PW_BLUEFLAG))
		{
			CG_SendConsoleCommand("follow %i", i);
			cg.lastManualCommandInterruptingAutoFollow = cg.time;
			return;
		}
	}
}

static void CG_FollowYellowFlag_f(void)
{
	int i;
	clientInfo_t *ci;

	if (!cg.snap)
		return;

	if (!cgs.isCTFMod || !cgs.CTF3ModeActive)
		return;

	for (i = 0; i < cgs.maxclients; i++)
	{
		if (i == cg.snap->ps.clientNum)
			continue;

		ci = &cgs.clientinfo[i];

		if (ci->powerups & (1 << PW_NEUTRALFLAG))
		{
			CG_SendConsoleCommand("cmd follow %i", i);
			cg.lastManualCommandInterruptingAutoFollow = cg.time;
			return;
		}
	}
}

static void CG_FollowFastest_f(void)
{
	int i, fastestPlayer = -1, currentSpeed, fastestSpeed = 0;
	centity_t *cent;

	if (!cg.snap)
		return;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i == cg.snap->ps.clientNum)
			continue;

		cent = &cg_entities[i];

		if (!cent)
			continue;
		if (cent->currentState.eType != ET_PLAYER)
			continue;

		currentSpeed = VectorLengthSquared(cent->currentState.pos.trDelta);

		if (currentSpeed > fastestSpeed)
		{
			fastestSpeed = currentSpeed;
			fastestPlayer = i;
		}
	}
	if (fastestPlayer >= 0 && fastestPlayer < MAX_CLIENTS)
	{
		CG_SendConsoleCommand("cmd follow %i", fastestPlayer);
		cg.lastManualCommandInterruptingAutoFollow = cg.time;
	}
}

static void CG_RemapShader_f(void)
{
	char oldShader[MAX_QPATH], newShader[MAX_QPATH];

	if (trap_Argc() != 3)
	{
		CG_Printf("Usage: /remapShader <old> <new>\n");
		return;
	}

	trap_Argv(1, oldShader, sizeof(oldShader));
	trap_Argv(2, newShader, sizeof(newShader));

	// validate this ?
	// how to stop from using trans shaders..?

	trap_R_RemapShader(oldShader, newShader, NULL); // what is timeoffset for
}

static void CG_ListRemaps_f(void)
{
	const char *info;
	char info2[MAX_CONFIGSTRINGS];
	info = CG_ConfigString(CS_SHADERSTATE);

	Q_strncpyz(info2, info, sizeof(info2));

	Q_strstrip(info2, ":", "\n");

	CG_Printf("Remaps: \n %s \n", info2);

	// Replace : with newline
	// replace 0.30@ with null?
	// replace = with " -> "

	// keep track of local remaps somehow
	// either directly or add remap text to array when added, list here
}

void CG_Do_f(void) // loda fixme
{
	char vstr[MAX_QPATH], delay[32];
	int delayMS;

	if (trap_Argc() != 3)
	{
		Com_Printf("Usage: /do <vstr> <delay>\n");
		return;
	}

	if (cgs.restricts & RESTRICT_DO)
	{
		return;
	}

	if ((cg.clientNum == cg.predictedPlayerState.clientNum) || !cg.snap)
	{
		if (cg.predictedPlayerState.stats[STAT_RACEMODE])
			return;
	}
	else
	{
		if (cg.snap->ps.stats[STAT_RACEMODE])
			return;
	}

	trap_Argv(1, vstr, sizeof(vstr));
	trap_Argv(2, delay, sizeof(delay));

	delayMS = atoi(delay);
	if (delayMS < 0)
		delayMS = 0;
	else if (delayMS > 1000 * 60 * 60)
		delayMS = 1000 * 60 * 60;

	Com_sprintf(cg.doVstr, sizeof(cg.doVstr), "vstr %s\n", vstr);
	cg.doVstrTime = cg.time + delayMS;
}

static void CG_Flipkick_f(void)
{
	// Well we always want to do the first kick, unless we are doing some really advanced predictive shit..

	// Ok, we started out flipkick.  Each frame we want to remove/add jump (+moveup and -moveup).

	cg.numFKFrames = 1;

	// How to make the perfect KS?
	// Get frametime or com_maxfps ?
	// Do however many taps super fast until they are at max jump kick height?
	// trap_SendConsoleCommand("+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup;wait 2;+moveup;wait 2;-moveup\n");
}

static void CG_DiscoLights_f(void)
{
	if (cg_acidtrip.integer)
	{
		trap_Cvar_Set("cg_acidtrip", "0");
		CG_Printf("Turning off disco lights.\n");
	}
	else
	{
		trap_Cvar_Set("cg_acidtrip", "20");
		CG_Printf("^3ACTIVATING ^2DISCO ^1LIGHTS^7!\n");
	}
}
extern qboolean secretQuiGonAllowed;
extern qboolean cgQuigonUnlocked;
static void CG_QuiGonJinn_f(void)
{
	char arg1[10];
	char arg2[10];
	qboolean nope = qfalse;
	if (trap_Argc() < 3)
	{
		nope = qtrue;
	}
	else
	{
		trap_Argv(1, arg1, sizeof(arg1));
		trap_Argv(2, arg2, sizeof(arg2));
		if (Q_stricmp(arg1, "gon") && Q_stricmp(arg1, "gonn") || Q_stricmp(arg2, "jin") && Q_stricmp(arg2, "jinn"))
		{
			nope = qtrue;
		}
	}

	if (nope)
	{
		trap_SendConsoleCommand("quit\n");
		return;
	}

	if (!secretQuiGonAllowed)
	{
		secretQuiGonAllowed = qtrue;
		CG_Printf("^2SECRET QUI-GON JINN SKIN UNLOCKED!\n");
		CG_Printf("Credit for original model/skin by yasuakiNk goes to yasuakiNk, Seven, Elek Andor and Toshi.\n");
		trap_Cvar_Set("model", "secret_quigon/default");
		trap_Cvar_Set("team_model", "secret_quigon/default");
		cgQuigonUnlocked = qtrue;
	}
}

static void CG_Lowjump_f(void)
{
	trap_SendConsoleCommand("+moveup\n");
	Q_strncpyz(cg.doVstr, "-moveup\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollDown_f(void)
{
	if (trap_Key_IsDown(trap_Key_GetKey("+moveup")))
	{
		trap_SendConsoleCommand("-moveup\n");
	}

	if (cg.predictedPlayerState.weapon != WP_SABER || cg.predictedPlayerState.powerups[PW_YSALAMIRI] > cg.time)
	{
		trap_SendConsoleCommand("+movedown\n");
		return;
	}

	trap_SendConsoleCommand("+speed\n");
	Q_strncpyz(cg.doVstr, "-moveup;+movedown;-speed\n", sizeof(cg.doVstr));
	cg.doVstrTime = cg.time;
}

static void CG_NorollUp_f(void)
{
	if (cg.predictedPlayerState.weapon != WP_SABER || cg.predictedPlayerState.powerups[PW_YSALAMIRI] > cg.time)
	{
		trap_SendConsoleCommand("-movedown\n");
		return;
	}

	Q_strncpyz(cg.doVstr, "-movedown;-speed\n", sizeof(cg.doVstr)); //?
	cg.doVstrTime = cg.time;
}

qboolean CG_WeaponSelectable(int i);
void CG_LastWeapon_f(void) // loda fixme. japro
{
	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if (cg.predictedPlayerState.pm_flags & PMF_FOLLOW)
		return;

	if (!cg.lastWeaponSelect[0])
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon;
	if (!cg.lastWeaponSelect[1])
		cg.lastWeaponSelect[1] = cg.predictedPlayerState.weapon;

	if (cg.lastWeaponSelect[1] == cg.lastWeaponSelect[0])
	{ // if the weapon we spawned with is still equipped
		int i;
		for (i = LAST_USEABLE_WEAPON; i > 0; i--)
		{ // cycle to the next available one
			if ((i != cg.weaponSelect) && CG_WeaponSelectable(i))
			{
				cg.lastWeaponSelect[1] = i;
				break;
			}
		}
	}

	if (cg.lastWeaponSelect[0] != cg.weaponSelect)
	{													 // Current does not match selected
		cg.lastWeaponSelect[1] = cg.lastWeaponSelect[0]; // Set last to current
		cg.lastWeaponSelect[0] = cg.weaponSelect;		 // Set current to selected
	}

	cg.weaponSelect = cg.lastWeaponSelect[1]; // Set selected to last

	cg.weaponSelectTime = cg.time;
	if (cg.weaponSelect != cg.lastWeaponSelect[1])
		trap_S_MuteSound(cg.predictedPlayerState.clientNum, CHAN_WEAPON);
}

static void CG_PrintKillsForClient(int client)
{
	int i;
	float ratio;
	clientInfo_t *ci;

	ci = &cgs.clientinfo[client];

	if (cg.totalDeaths[client] != 0)
	{
		ratio = (float)cg.totalKills[client] / (float)cg.totalDeaths[client];
	}
	else
	{
		ratio = 0.0f;
	}

	CG_Printf(
		"Total kills for %s\n" S_COLOR_WHITE
		"Kills: " S_COLOR_GREEN "%d\n" S_COLOR_WHITE
		"Deaths: " S_COLOR_RED "%d\n" S_COLOR_WHITE
		"Ratio: " S_COLOR_YELLOW "%5.2f\n" S_COLOR_WHITE
		"ID Kills Deaths Ratio Name\n",
		ci->name,
		cg.totalKills[client],
		cg.totalDeaths[client],
		ratio);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];

		if (ci->infoValid == qfalse)
		{
			continue;
		}

		if (i == client)
		{
			continue;
		}

		if (cg.directKills[i][client] != 0)
		{
			ratio = (float)cg.directKills[client][i] / (float)cg.directKills[i][client];
		}
		else
		{
			ratio = 0.0f;
		}

		CG_Printf(
			"%2d " S_COLOR_GREEN "%5d " S_COLOR_RED "%6d " S_COLOR_YELLOW "%5.2f " S_COLOR_WHITE "%s\n",
			i,
			cg.directKills[client][i],
			cg.directKills[i][client],
			ratio,
			ci->name);
	}
}

static void CG_PrintKillsForAllClients(void)
{
	int i;
	float ratio;
	clientInfo_t *ci;

	CG_Printf(
		"Total kills\n"
		"ID Kills Deaths Ratio Name\n");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];

		if (ci->infoValid == qfalse)
		{
			continue;
		}

		if (cg.totalDeaths[i] != 0)
		{
			ratio = (float)cg.totalKills[i] / (float)cg.totalDeaths[i];
		}
		else
		{
			ratio = 0.0f;
		}

		CG_Printf(
			"%2d " S_COLOR_GREEN "%5d " S_COLOR_RED "%6d " S_COLOR_YELLOW "%5.2f " S_COLOR_WHITE "%s\n",
			i,
			cg.totalKills[i],
			cg.totalDeaths[i],
			ratio,
			ci->name);
	}
}

static void CG_KillTracker_f(void)
{
	if (trap_Argc() > 1)
	{
		int client = CG_ClientNumberFromString(CG_Argv(1));
		if (client >= 0)
		{
			CG_PrintKillsForClient(client);
		}
	}
	else
	{
		CG_PrintKillsForAllClients();
	}
}

typedef struct
{
	char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {
	{"testgun", CG_TestGun_f},
	{"testmodel", CG_TestModel_f},
	{"nextframe", CG_TestModelNextFrame_f},
	{"prevframe", CG_TestModelPrevFrame_f},
	{"nextskin", CG_TestModelNextSkin_f},
	{"prevskin", CG_TestModelPrevSkin_f},
	{"viewpos", CG_Viewpos_f},
	{"+scores", CG_ScoresDown_f},
	{"-scores", CG_ScoresUp_f},
	{"sizeup", CG_SizeUp_f},
	{"sizedown", CG_SizeDown_f},
	{"weapnext", CG_NextWeapon_f},
	{"weapprev", CG_PrevWeapon_f},
	{"weapon", CG_Weapon_f},
	{"tell_target", CG_TellTarget_f},
	{"tell_attacker", CG_TellAttacker_f},
	{"vtell_target", CG_VoiceTellTarget_f},
	{"vtell_attacker", CG_VoiceTellAttacker_f},
	{"tcmd", CG_TargetCommand_f},
	{"nextTeamMember", CG_NextTeamMember_f},
	{"prevTeamMember", CG_PrevTeamMember_f},
	{"nextOrder", CG_NextOrder_f},
	{"confirmOrder", CG_ConfirmOrder_f},
	{"denyOrder", CG_DenyOrder_f},
	{"taskOffense", CG_TaskOffense_f},
	{"taskDefense", CG_TaskDefense_f},
	{"taskPatrol", CG_TaskPatrol_f},
	{"taskCamp", CG_TaskCamp_f},
	{"taskFollow", CG_TaskFollow_f},
	{"taskRetrieve", CG_TaskRetrieve_f},
	{"taskEscort", CG_TaskEscort_f},
	{"taskSuicide", CG_TaskSuicide_f},
	{"taskOwnFlag", CG_TaskOwnFlag_f},
	{"tauntKillInsult", CG_TauntKillInsult_f},
	{"tauntPraise", CG_TauntPraise_f},
	{"tauntTaunt", CG_TauntTaunt_f},
	{"tauntDeathInsult", CG_TauntDeathInsult_f},
	{"tauntGauntlet", CG_TauntGauntlet_f},
	{"spWin", CG_spWin_f},
	{"spLose", CG_spLose_f},
	{"scoresDown", CG_scrollScoresDown_f},
	{"scoresUp", CG_scrollScoresUp_f},
	{"startOrbit", CG_StartOrbit_f},
	//{ "camera", CG_Camera_f },
	{"loaddeferred", CG_LoadDeferredPlayers},
	{"invnext", CG_NextInventory_f},
	{"invprev", CG_PrevInventory_f},
	{"forcenext", CG_NextForcePower_f},
	{"forceprev", CG_PrevForcePower_f},

	// jk2pro stuff
	{"strafeHelper", CG_StrafeHelper_f},
	{"speedometer", cg_speedometer_f},

	{"+zoom", CG_ZoomDown_f},
	{"-zoom", CG_ZoomUp_f},

	{"say", CG_Say_f},
	{"say_team", CG_Say_f},
	{"tell", CG_Say_f},

	{"clientlist", CG_ClientList_f},

	{"modversion", CG_ModVersion_f},

	{"follow", CG_Follow_f},
	{"followRedFlag", CG_FollowRedFlag_f},
	{"followBlueFlag", CG_FollowBlueFlag_f},
	{"followYellowFlag", CG_FollowYellowFlag_f},
	{"followFastest", CG_FollowFastest_f},

	{"remapShader", CG_RemapShader_f},
	{"listRemaps", CG_ListRemaps_f},

	{"do", CG_Do_f},
	{"flipkick", CG_Flipkick_f},
	{"lowjump", CG_Lowjump_f},
	{"+duck", CG_NorollDown_f},
	{"-duck", CG_NorollUp_f},
	{"weaplast", CG_LastWeapon_f},

	{"killTracker", CG_KillTracker_f},
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand(void)
{
	const char *cmd;
	size_t i;

	cmd = CG_Argv(0);

	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		trap_AddCommand(commands[i].cmd);
	}

	if (coolApi & COOL_APIFEATURE_ADDMEMECOMMAND)
	{
		for (i = 0; i < sizeof(memecommands) / sizeof(memecommands[0]); i++)
		{
			trap_CG_COOL_API_AddMemeCommand(memecommands[i].cmd);
		}
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand("forcechanged");
	trap_AddCommand("sv_invnext");
	trap_AddCommand("sv_invprev");
	trap_AddCommand("sv_forcenext");
	trap_AddCommand("sv_forceprev");
	trap_AddCommand("sv_saberswitch");
	trap_AddCommand("engage_duel");
	trap_AddCommand("force_heal");
	trap_AddCommand("force_speed");
	trap_AddCommand("force_throw");
	trap_AddCommand("force_pull");
	trap_AddCommand("force_distract");
	trap_AddCommand("force_rage");
	trap_AddCommand("force_protect");
	trap_AddCommand("force_absorb");
	trap_AddCommand("force_healother");
	trap_AddCommand("force_forcepowerother");
	trap_AddCommand("force_seeing");
	trap_AddCommand("use_seeker");
	trap_AddCommand("use_field");
	trap_AddCommand("use_bacta");
	trap_AddCommand("use_electrobinoculars");
	trap_AddCommand("zoom");
	trap_AddCommand("use_sentry");
	trap_AddCommand("bot_order");
	trap_AddCommand("saberAttackCycle");
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("tell");
	trap_AddCommand("vsay");
	trap_AddCommand("vsay_team");
	trap_AddCommand("vtell");
	trap_AddCommand("vtaunt");
	trap_AddCommand("vosay");
	trap_AddCommand("vosay_team");
	trap_AddCommand("votell");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("team");
	trap_AddCommand("levelshot");
	trap_AddCommand("addbot");
	trap_AddCommand("setviewpos");
	trap_AddCommand("callvote");
	trap_AddCommand("vote");
	trap_AddCommand("callteamvote");
	trap_AddCommand("teamvote");
	trap_AddCommand("stats");
	trap_AddCommand("teamtask");
	trap_AddCommand("loaddefered"); // spelled wrong, but not changing for demo

	// generic mod server cmds
	trap_AddCommand("help");
	trap_AddCommand("ignore");
	trap_AddCommand("aminfo");
	trap_AddCommand("amempower"); // idk if this is somewhere other than twimod
	trap_AddCommand("ammerc");	  // probably exists somewhere if amempower does

	trap_AddCommand("engage_gunduel");		 //?
	trap_AddCommand("engage_fullforceduel"); //??

	// ctfmod server cmds
	trap_AddCommand("afk");
	trap_AddCommand("altf");
	trap_AddCommand("ignoreclear");
	trap_AddCommand("ignorelist");
	trap_AddCommand("specs");

	// ctfmod rcon cmds for autocomplete
	trap_AddCommand("amkick");
	trap_AddCommand("amstatus");
	trap_AddCommand("cp");
	trap_AddCommand("cvars");
	trap_AddCommand("forceteam");
	trap_AddCommand("lockname");
	trap_AddCommand("lockteam");
	trap_AddCommand("mute");
	trap_AddCommand("pause");
	trap_AddCommand("poll");
	trap_AddCommand("swapteams");
	trap_AddCommand("unpause");

	// TwiMod cmds
	trap_AddCommand("ammodinfo");
	trap_AddCommand("ammodinfo_twitch");
	trap_AddCommand("amadmin");

	trap_AddCommand("channel");
	trap_AddCommand("channellist");

	trap_AddCommand("mute");

	trap_AddCommand("engage_ff"); // oh ok..

	trap_AddCommand("engage_private");
	trap_AddCommand("invite_private");
	trap_AddCommand("accept_private");
	trap_AddCommand("end_private");

	// TwiMod build cmds
	trap_AddCommand("placemodel");
	trap_AddCommand("drop");
	trap_AddCommand("pick");
	trap_AddCommand("remove");

	// TwiMod emotes
	trap_AddCommand("ambar");
	trap_AddCommand("ambeg");
	trap_AddCommand("amcomeon");
	trap_AddCommand("amflip");
	trap_AddCommand("amlaugh");
	trap_AddCommand("amnod");
	trap_AddCommand("amshake");
	trap_AddCommand("amsuper");
	trap_AddCommand("amsuper2");
	trap_AddCommand("amspin");
	trap_AddCommand("amspin2");
	trap_AddCommand("amspin3");
	trap_AddCommand("amspinr");
	trap_AddCommand("amspin2r");
	trap_AddCommand("amspin3r");
	trap_AddCommand("amthumbsdown");
	trap_AddCommand("amthumbsup");
	trap_AddCommand("amtossover");
	trap_AddCommand("amtossup");
	trap_AddCommand("amvictory");
	trap_AddCommand("amwave2");
	trap_AddCommand("amdontkillme");
	trap_AddCommand("amfakedead");
	trap_AddCommand("amkneel");
	trap_AddCommand("amsit");
	trap_AddCommand("amsit2");
	trap_AddCommand("amsit3");
	trap_AddCommand("amthreaten");
	trap_AddCommand("amtype");
	trap_AddCommand("amtype2");
	trap_AddCommand("amwrite");
	trap_AddCommand("amwrite2");
	trap_AddCommand("amcowboy");
	trap_AddCommand("amhandhips");
	trap_AddCommand("amsurrender");
	trap_AddCommand("amwait");

	trap_AddCommand("amtaunt");
	trap_AddCommand("amtaunt2");

	trap_AddCommand("taunt2");

	// these probably exist smehwere..
	trap_AddCommand("amhug");
	trap_AddCommand("amkiss");

	// tommyternal
	trap_AddCommand("run");
	trap_AddCommand("race");
	trap_AddCommand("jump");
	trap_AddCommand("savespawn");
	trap_AddCommand("resetspawn");
	trap_AddCommand("savepos");
	trap_AddCommand("respos");
	trap_AddCommand("move");
	trap_AddCommand("togglefps");
	trap_AddCommand("floatphysics");
	trap_AddCommand("checkpoint");
	trap_AddCommand("removecheckpoints");
	trap_AddCommand("stealcheckpoints");
	trap_AddCommand("stealspawn");
	trap_AddCommand("stealpos");
	trap_AddCommand("top");
	trap_AddCommand("topmain");
	trap_AddCommand("topnjb");
	trap_AddCommand("topnojumpbug");
	trap_AddCommand("topcustom");
	trap_AddCommand("topsegmented");
	trap_AddCommand("topseg");
	trap_AddCommand("topcheat");
	trap_AddCommand("time");
	trap_AddCommand("logout");
	trap_AddCommand("savecheckpoints");
	trap_AddCommand("loadcheckpoints");
	trap_AddCommand("lasers");
	trap_AddCommand("solo");
	trap_AddCommand("mapdefaults");
	trap_AddCommand("amtele");
	trap_AddCommand("rollympics");
	trap_AddCommand("latest");
	trap_AddCommand("help");
	trap_AddCommand("ignore");
	trap_AddCommand("maplist");
	trap_AddCommand("launch");
	trap_AddCommand("longest");
	trap_AddCommand("shortest");
	trap_AddCommand("hardest");
	trap_AddCommand("easiest");
	trap_AddCommand("notwr");
	trap_AddCommand("wrs");
	trap_AddCommand("rank");
	trap_AddCommand("pickmode");
	trap_AddCommand("players");
	trap_AddCommand("duel");
	trap_AddCommand("allforce");
	trap_AddCommand("ironman");
	trap_AddCommand("stay");
	trap_AddCommand("say_cross");
	trap_AddCommand("resseg");

	// V24 Enhanced Features - Complete Command Registration
	// Auto Systems - Basic Commands
	trap_AddCommand("+autobackstab");
	trap_AddCommand("-autobackstab");
	trap_AddCommand("+autodualbackstab");
	trap_AddCommand("-autodualbackstab");
	trap_AddCommand("+autoadvancedbackstab");
	trap_AddCommand("-autoadvancedbackstab");

	// V24 Enhanced Features - Short Aliases for Backstabs
	trap_AddCommand("+bs");
	trap_AddCommand("-bs");
	trap_AddCommand("+dbs");
	trap_AddCommand("-dbs");
	trap_AddCommand("+adbs");
	trap_AddCommand("-adbs");

	trap_AddCommand("+autokick");
	trap_AddCommand("-autokick");
	trap_AddCommand("+autoaim");
	trap_AddCommand("-autoaim");
	trap_AddCommand("auto_defense_toggle");

	// Auto-Kick Configuration Commands
	trap_AddCommand("autokick_distance");
	trap_AddCommand("autokick_angle");
	trap_AddCommand("autokick_delay");
	trap_AddCommand("autokick_prediction");
	trap_AddCommand("autokick_ignore_friends");
	trap_AddCommand("autokick_ignore_spectators");
	trap_AddCommand("autokick_sound_alert");
	trap_AddCommand("autokick_visual_alert");

	// Auto-Aim Configuration Commands
	trap_AddCommand("autoaim_fov");
	trap_AddCommand("autoaim_range");
	trap_AddCommand("autoaim_delay");
	trap_AddCommand("autoaim_prediction");
	trap_AddCommand("autoaim_damping");
	trap_AddCommand("autoaim_ignore_friends");
	trap_AddCommand("autoaim_ignore_spectators");
	trap_AddCommand("autoaim_sound_alert");
	trap_AddCommand("autoaim_visual_alert");
	trap_AddCommand("autoaim_wall_penetrate");

	// Auto-Backstab Configuration Commands
	trap_AddCommand("autobackstab_distance");
	trap_AddCommand("autobackstab_angle");
	trap_AddCommand("autobackstab_delay");
	trap_AddCommand("autobackstab_ignore_friends");
	trap_AddCommand("autobackstab_sound_alert");

	// ESP Complete Commands
	trap_AddCommand("esp_toggle");
	trap_AddCommand("esp_players");
	trap_AddCommand("esp_items");
	trap_AddCommand("esp_distance");
	trap_AddCommand("esp_through_walls");
	trap_AddCommand("esp_style");
	trap_AddCommand("esp_alpha");
	trap_AddCommand("esp_size");
	trap_AddCommand("esp_player_names");
	trap_AddCommand("esp_item_names");
	trap_AddCommand("esp_health_bars");
	trap_AddCommand("esp_force_bars");
	trap_AddCommand("esp_weapon_info");
	trap_AddCommand("esp_boxes");
	trap_AddCommand("esp_lines");
	trap_AddCommand("esp_names");
	trap_AddCommand("esp_color_mode");
	trap_AddCommand("esp_player_color");
	trap_AddCommand("esp_enemy_color");
	trap_AddCommand("esp_item_color");
	trap_AddCommand("esp_friend_color");
	trap_AddCommand("esp_most_wanted_color");
	trap_AddCommand("esp_update_rate");
	trap_AddCommand("esp_max_entities");
	trap_AddCommand("esp_debug");

	// Wallhack Complete Commands
	trap_AddCommand("wallhack_toggle");
	trap_AddCommand("wallhack_style");
	trap_AddCommand("wallhack_alpha");
	trap_AddCommand("wallhack_color");
	trap_AddCommand("wallhack_range");
	trap_AddCommand("wallhack_ignore_friends");
	trap_AddCommand("wallhack_sound_alert");
	trap_AddCommand("wallhack_visual_alert");
	trap_AddCommand("wallhack_pulse");

	// Enemy Detection Complete Commands
	trap_AddCommand("enemy_detection");
	trap_AddCommand("enemy_detection_range");
	trap_AddCommand("enemy_detection_fov");
	trap_AddCommand("enemy_detection_style");
	trap_AddCommand("enemy_detection_color");
	trap_AddCommand("enemy_detection_sound");
	trap_AddCommand("enemy_detection_pulse");

	// Friends System Complete Commands
	trap_AddCommand("friend_add");
	trap_AddCommand("friend_remove");
	trap_AddCommand("friend_list");
	trap_AddCommand("friend_clear");
	trap_AddCommand("friends_system");
	trap_AddCommand("friends_visual_markers");
	trap_AddCommand("friends_sound_notifications");

	// Saber Commands
	trap_AddCommand("saber_tip_trace");
}

// V24 Enhanced Features - Complete Console Command Handler
qboolean CG_ConsoleCommand(void)
{
	const char *cmd;

	cmd = CG_Argv(0);

	// V24 Enhanced Features - Auto-Backstab Commands
	if (!Q_stricmp(cmd, "+autobackstab"))
	{
		CG_AutoBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-autobackstab"))
	{
		CG_AutoBackstabUp_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "+autodualbackstab"))
	{
		CG_AutoDualBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-autodualbackstab"))
	{
		CG_AutoDualBackstabUp_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "+autoadvancedbackstab"))
	{
		CG_AutoAdvancedBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-autoadvancedbackstab"))
	{
		CG_AutoAdvancedBackstabUp_f();
		return qtrue;
	}

	// V24 Enhanced Features - Short Backstab Aliases
	if (!Q_stricmp(cmd, "+bs"))
	{
		CG_AutoBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-bs"))
	{
		CG_AutoBackstabUp_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "+dbs"))
	{
		CG_AutoDualBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-dbs"))
	{
		CG_AutoDualBackstabUp_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "+adbs"))
	{
		CG_AutoAdvancedBackstabDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-adbs"))
	{
		CG_AutoAdvancedBackstabUp_f();
		return qtrue;
	}

	// V24 Enhanced Features - Auto-Kick Commands
	if (!Q_stricmp(cmd, "+autokick"))
	{
		CG_AutoKickDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-autokick"))
	{
		CG_AutoKickUp_f();
		return qtrue;
	}

	// V24 Enhanced Features - Auto-Aim Commands
	if (!Q_stricmp(cmd, "+autoaim"))
	{
		CG_AutoAimDown_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "-autoaim"))
	{
		CG_AutoAimUp_f();
		return qtrue;
	}

	// Auto Defense System Toggle
	if (!Q_stricmp(cmd, "auto_defense_toggle"))
	{
		CG_AutoDefenseToggle_f();
		return qtrue;
	}

	// Auto-Kick Configuration Commands
	if (!Q_stricmp(cmd, "autokick_distance"))
	{
		CG_AutoKickDistance_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_angle"))
	{
		CG_AutoKickAngle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_delay"))
	{
		CG_AutoKickDelay_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_prediction"))
	{
		CG_AutoKickPrediction_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_ignore_friends"))
	{
		CG_AutoKickIgnoreFriends_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_ignore_spectators"))
	{
		CG_AutoKickIgnoreSpectators_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_sound_alert"))
	{
		CG_AutoKickSoundAlert_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autokick_visual_alert"))
	{
		CG_AutoKickVisualAlert_f();
		return qtrue;
	}

	// Auto-Aim Configuration Commands
	if (!Q_stricmp(cmd, "autoaim_fov"))
	{
		CG_AutoAimFOV_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_range"))
	{
		CG_AutoAimRange_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_delay"))
	{
		CG_AutoAimDelay_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_prediction"))
	{
		CG_AutoAimPrediction_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_damping"))
	{
		CG_AutoAimDamping_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_ignore_friends"))
	{
		CG_AutoAimIgnoreFriends_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_ignore_spectators"))
	{
		CG_AutoAimIgnoreSpectators_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_sound_alert"))
	{
		CG_AutoAimSoundAlert_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_visual_alert"))
	{
		CG_AutoAimVisualAlert_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autoaim_wall_penetrate"))
	{
		CG_AutoAimWallPenetrate_f();
		return qtrue;
	}

	// Auto-Backstab Configuration Commands
	if (!Q_stricmp(cmd, "autobackstab_distance"))
	{
		CG_AutoBackstabDistance_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autobackstab_angle"))
	{
		CG_AutoBackstabAngle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autobackstab_delay"))
	{
		CG_AutoBackstabDelay_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autobackstab_ignore_friends"))
	{
		CG_AutoBackstabIgnoreFriends_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "autobackstab_sound_alert"))
	{
		CG_AutoBackstabSoundAlert_f();
		return qtrue;
	}

	// V24 Enhanced Features - ESP Commands
	if (!Q_stricmp(cmd, "esp_toggle"))
	{
		CG_ESPToggle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_players"))
	{
		CG_ESPPlayers_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_items"))
	{
		CG_ESPItems_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_health_bars"))
	{
		CG_ESPHealthBars_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_force_bars"))
	{
		CG_ESPForceBars_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_weapon_info"))
	{
		CG_ESPWeaponInfo_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_boxes"))
	{
		CG_ESPBoxes_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_lines"))
	{
		CG_ESPLines_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_names"))
	{
		CG_ESPNames_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_through_walls"))
	{
		CG_ESPThroughWalls_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_color_mode"))
	{
		CG_ESPColorMode_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_debug"))
	{
		CG_ESPDebug_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_distance"))
	{
		CG_ESPDistance_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_style"))
	{
		CG_ESPStyle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_alpha"))
	{
		CG_ESPAlpha_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_size"))
	{
		CG_ESPSize_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_player_names"))
	{
		CG_ESPPlayerNames_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_item_names"))
	{
		CG_ESPItemNames_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_health_bars"))
	{
		CG_ESPHealthBars_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_force_bars"))
	{
		CG_ESPForceBars_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_player_color"))
	{
		CG_ESPPlayerColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_enemy_color"))
	{
		CG_ESPEnemyColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_item_color"))
	{
		CG_ESPItemColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_friend_color"))
	{
		CG_ESPFriendColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_most_wanted_color"))
	{
		CG_ESPMostWantedColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_update_rate"))
	{
		CG_ESPUpdateRate_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "esp_max_entities"))
	{
		CG_ESPMaxEntities_f();
		return qtrue;
	}

	// V24 Enhanced Features - Wallhack Commands
	if (!Q_stricmp(cmd, "wallhack_toggle"))
	{
		CG_WallhackToggle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_style"))
	{
		CG_WallhackStyle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_alpha"))
	{
		CG_WallhackAlpha_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_color"))
	{
		CG_WallhackColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_range"))
	{
		CG_WallhackRange_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_ignore_friends"))
	{
		CG_WallhackIgnoreFriends_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_sound_alert"))
	{
		CG_WallhackSoundAlert_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_visual_alert"))
	{
		CG_WallhackVisualAlert_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "wallhack_pulse"))
	{
		CG_WallhackPulse_f();
		return qtrue;
	}

	// V24 Enhanced Features - Enemy Detection Commands
	if (!Q_stricmp(cmd, "enemy_detection"))
	{
		CG_EnemyDetectionToggle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_range"))
	{
		CG_EnemyDetectionRange_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_fov"))
	{
		CG_EnemyDetectionFOV_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_style"))
	{
		CG_EnemyDetectionStyle_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_color"))
	{
		CG_EnemyDetectionColor_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_sound"))
	{
		CG_EnemyDetectionSound_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "enemy_detection_pulse"))
	{
		CG_EnemyDetectionPulse_f();
		return qtrue;
	}

	// V24 Enhanced Features - Friends System Commands
	if (!Q_stricmp(cmd, "friend_add"))
	{
		CG_FriendAdd_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friend_remove"))
	{
		CG_FriendRemove_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friend_list"))
	{
		CG_FriendList_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friend_clear"))
	{
		CG_FriendClear_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friends_system"))
	{
		CG_FriendsSystem_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friends_visual_markers"))
	{
		CG_FriendsVisualMarkers_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "friends_sound_notifications"))
	{
		CG_FriendsSoundNotifications_f();
		return qtrue;
	}

	// V24 Enhanced Features - Saber Commands
	if (!Q_stricmp(cmd, "saber_tip_trace"))
	{
		CG_SaberTipTrace_f();
		return qtrue;
	}

	// Debug command for mouse/view issues
	if (!Q_stricmp(cmd, "debug_mouse"))
	{
		CG_Printf("=== Mouse/View Debug Info ===\n");
		if (cg.snap)
		{
			CG_Printf("Saber Lock Time: %d (current: %d) - %s\n",
					  cg.snap->ps.saberLockTime, cg.time,
					  (cg.snap->ps.saberLockTime > cg.time) ? "LOCKED" : "FREE");
			CG_Printf("Using ATST: %s\n", cg.snap->ps.usingATST ? "YES" : "NO");
			CG_Printf("Player State: PM_TYPE %d\n", cg.snap->ps.pm_type);
		}
		CG_Printf("Auto-Aim Enabled: %d\n", cg_autoAim.integer);
		CG_Printf("Auto-Defense Enabled: %d\n", cg_autoDefense.integer);
		CG_Printf("Auto Suggested Angles: %.2f %.2f %.2f\n",
				  cg.autoSuggestedViewAngles[0], cg.autoSuggestedViewAngles[1], cg.autoSuggestedViewAngles[2]);
		CG_Printf("Mouse Captured: %s\n", cg.mouseCaptured ? "YES" : "NO");
		CG_Printf("Last Manual Command: %d (current: %d)\n", cg.lastManualCommandInterruptingAutoFollow, cg.time);
		CG_Printf("Zoom Sensitivity: %.3f\n", cg.zoomSensitivity);
		return qtrue;
	}

	// DLL diagnostic command
	if (!Q_stricmp(cmd, "debug_dlls"))
	{
		char buffer[64];
		CG_Printf("=== DLL Status Debug Info ===\n");
		CG_Printf("CGame Module: LOADED (this command is running from cgame_x64.dll)\n");
		CG_Printf("VM Modes:\n");

		trap_Cvar_VariableStringBuffer("vm_cgame", buffer, sizeof(buffer));
		CG_Printf("  vm_cgame: %s\n", buffer);

		trap_Cvar_VariableStringBuffer("vm_game", buffer, sizeof(buffer));
		CG_Printf("  vm_game: %s\n", buffer);

		trap_Cvar_VariableStringBuffer("vm_ui", buffer, sizeof(buffer));
		CG_Printf("  vm_ui: %s\n", buffer);

		CG_Printf("Game State:\n");
		if (cg.snap)
		{
			CG_Printf("  Connected to server: YES\n");
			CG_Printf("  Client Number: %d\n", cg.clientNum);
			CG_Printf("  Server Time: %d\n", cg.snap->serverTime);
		}
		else
		{
			CG_Printf("  Connected to server: NO\n");
		}
		CG_Printf("To test UI module, open a menu (ESC)\n");
		return qtrue;
	}

	// Command not handled
	return qfalse;
}
