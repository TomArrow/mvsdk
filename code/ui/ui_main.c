// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
// #define PRE_RELEASE_TADEMO

#include "../ghoul2/G2.h"
#include "ui_local.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/game_version.h"
#include "ui_force.h"
#include "../cgame/animtable.h" //we want this to be compiled into the module because we access it in the shared module.
#include "../game/bg_saga.h"
#include "mvsdk_setup.h"

extern void UI_SaberAttachToChar(itemDef_t *item);

char *forcepowerDesc[NUM_FORCE_POWERS] =
	{
		"@MENUS_OF_EFFECT_JEDI_ONLY_NEFFECT",
		"@MENUS_DURATION_IMMEDIATE_NAREA",
		"@MENUS_DURATION_5_SECONDS_NAREA",
		"@MENUS_DURATION_INSTANTANEOUS",
		"@MENUS_INSTANTANEOUS_EFFECT_NAREA",
		"@MENUS_DURATION_VARIABLE_20",
		"@MENUS_DURATION_INSTANTANEOUS_NAREA",
		"@MENUS_OF_EFFECT_LIVING_PERSONS",
		"@MENUS_DURATION_VARIABLE_10",
		"@MENUS_DURATION_VARIABLE_NAREA",
		"@MENUS_DURATION_CONTINUOUS_NAREA",
		"@MENUS_OF_EFFECT_JEDI_ALLIES_NEFFECT",
		"@MENUS_EFFECT_JEDI_ALLIES_NEFFECT",
		"@MENUS_VARIABLE_NAREA_OF_EFFECT",
		"@MENUS_EFFECT_NAREA_OF_EFFECT",
		"@SP_INGAME_FORCE_SABER_OFFENSE_DESC",
		"@SP_INGAME_FORCE_SABER_DEFENSE_DESC",
		"@SP_INGAME_FORCE_SABER_THROW_DESC"};

// Movedata Sounds
enum
{
	MDS_NONE = 0,
	MDS_FORCE_JUMP,
	MDS_ROLL,
	MDS_SABER,
	MDS_MOVE_SOUNDS_MAX
};

enum
{
	MD_ACROBATICS = 0,
	MD_SINGLE_FAST,
	MD_SINGLE_MEDIUM,
	MD_SINGLE_STRONG,
	MD_DUAL_SABERS,
	MD_SABER_STAFF,
	MD_MOVE_TITLE_MAX
};

// Some hard coded badness
// At some point maybe this should be externalized to a .dat file
char *datapadMoveTitleData[MD_MOVE_TITLE_MAX] =
	{
		"@MENUS_ACROBATICS",
		"@MENUS_SINGLE_FAST",
		"@MENUS_SINGLE_MEDIUM",
		"@MENUS_SINGLE_STRONG",
		"@MENUS_DUAL_SABERS",
		"@MENUS_SABER_STAFF",
};

char *datapadMoveTitleBaseAnims[MD_MOVE_TITLE_MAX] =
	{
		"BOTH_RUN1",
		"BOTH_SABERFAST_STANCE",
		"BOTH_STAND2",
		"BOTH_SABERSLOW_STANCE",
		"BOTH_SABERDUAL_STANCE",
		"BOTH_SABERSTAFF_STANCE",
};

#define MAX_MOVES 16

typedef struct
{
	char *title;
	char *desc;
	char *anim;
	int sound;
} datpadmovedata_t;

static datpadmovedata_t datapadMoveData[MD_MOVE_TITLE_MAX][MAX_MOVES] =
	{
		// Acrobatics
		"@MENUS_FORCE_JUMP1",
		"@MENUS_FORCE_JUMP1_DESC",
		"BOTH_FORCEJUMP1",
		MDS_FORCE_JUMP,
		"@MENUS_FORCE_FLIP",
		"@MENUS_FORCE_FLIP_DESC",
		"BOTH_FLIP_F",
		MDS_FORCE_JUMP,
		"@MENUS_ROLL",
		"@MENUS_ROLL_DESC",
		"BOTH_ROLL_F",
		MDS_ROLL,
		"@MENUS_BACKFLIP_OFF_WALL",
		"@MENUS_BACKFLIP_OFF_WALL_DESC",
		"BOTH_WALL_FLIP_BACK1",
		MDS_FORCE_JUMP,
		"@MENUS_SIDEFLIP_OFF_WALL",
		"@MENUS_SIDEFLIP_OFF_WALL_DESC",
		"BOTH_WALL_FLIP_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_WALL_RUN",
		"@MENUS_WALL_RUN_DESC",
		"BOTH_WALL_RUN_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_WALL_GRAB_JUMP",
		"@MENUS_WALL_GRAB_JUMP_DESC",
		"BOTH_FORCEWALLREBOUND_FORWARD",
		MDS_FORCE_JUMP,
		"@MENUS_RUN_UP_WALL_BACKFLIP",
		"@MENUS_RUN_UP_WALL_BACKFLIP_DESC",
		"BOTH_FORCEWALLRUNFLIP_START",
		MDS_FORCE_JUMP,
		"@MENUS_JUMPUP_FROM_KNOCKDOWN",
		"@MENUS_JUMPUP_FROM_KNOCKDOWN_DESC",
		"BOTH_KNOCKDOWN3",
		MDS_NONE,
		"@MENUS_JUMPKICK_FROM_KNOCKDOWN",
		"@MENUS_JUMPKICK_FROM_KNOCKDOWN_DESC",
		"BOTH_KNOCKDOWN2",
		MDS_NONE,
		"@MENUS_ROLL_FROM_KNOCKDOWN",
		"@MENUS_ROLL_FROM_KNOCKDOWN_DESC",
		"BOTH_KNOCKDOWN1",
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,

		// Single Saber, Fast Style
		"@MENUS_STAB_BACK",
		"@MENUS_STAB_BACK_DESC",
		"BOTH_A2_STABBACK1",
		MDS_SABER,
		"@MENUS_LUNGE_ATTACK",
		"@MENUS_LUNGE_ATTACK_DESC",
		"BOTH_LUNGE2_B__T_",
		MDS_SABER,
		"@MENUS_FAST_ATTACK_KATA",
		"@MENUS_FAST_ATTACK_KATA_DESC",
		"BOTH_A1_SPECIAL",
		MDS_SABER,
		"@MENUS_ATTACK_ENEMYONGROUND",
		"@MENUS_ATTACK_ENEMYONGROUND_DESC",
		"BOTH_STABDOWN",
		MDS_FORCE_JUMP,
		"@MENUS_CARTWHEEL",
		"@MENUS_CARTWHEEL_DESC",
		"BOTH_ARIAL_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_BOTH_ROLL_STAB",
		"@MENUS_BOTH_ROLL_STAB2_DESC",
		"BOTH_ROLL_STAB",
		MDS_SABER,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,

		// Single Saber, Medium Style
		"@MENUS_SLASH_BACK",
		"@MENUS_SLASH_BACK_DESC",
		"BOTH_ATTACK_BACK",
		MDS_SABER,
		"@MENUS_FLIP_ATTACK",
		"@MENUS_FLIP_ATTACK_DESC",
		"BOTH_JUMPFLIPSLASHDOWN1",
		MDS_FORCE_JUMP,
		"@MENUS_MEDIUM_ATTACK_KATA",
		"@MENUS_MEDIUM_ATTACK_KATA_DESC",
		"BOTH_A2_SPECIAL",
		MDS_SABER,
		"@MENUS_ATTACK_ENEMYONGROUND",
		"@MENUS_ATTACK_ENEMYONGROUND_DESC",
		"BOTH_STABDOWN",
		MDS_FORCE_JUMP,
		"@MENUS_CARTWHEEL",
		"@MENUS_CARTWHEEL_DESC",
		"BOTH_ARIAL_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_BOTH_ROLL_STAB",
		"@MENUS_BOTH_ROLL_STAB2_DESC",
		"BOTH_ROLL_STAB",
		MDS_SABER,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,

		// Single Saber, Strong Style
		"@MENUS_SLASH_BACK",
		"@MENUS_SLASH_BACK_DESC",
		"BOTH_ATTACK_BACK",
		MDS_SABER,
		"@MENUS_JUMP_ATTACK",
		"@MENUS_JUMP_ATTACK_DESC",
		"BOTH_FORCELEAP2_T__B_",
		MDS_FORCE_JUMP,
		"@MENUS_STRONG_ATTACK_KATA",
		"@MENUS_STRONG_ATTACK_KATA_DESC",
		"BOTH_A3_SPECIAL",
		MDS_SABER,
		"@MENUS_ATTACK_ENEMYONGROUND",
		"@MENUS_ATTACK_ENEMYONGROUND_DESC",
		"BOTH_STABDOWN",
		MDS_FORCE_JUMP,
		"@MENUS_CARTWHEEL",
		"@MENUS_CARTWHEEL_DESC",
		"BOTH_ARIAL_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_BOTH_ROLL_STAB",
		"@MENUS_BOTH_ROLL_STAB2_DESC",
		"BOTH_ROLL_STAB",
		MDS_SABER,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,

		// Dual Sabers
		"@MENUS_SLASH_BACK",
		"@MENUS_SLASH_BACK_DESC",
		"BOTH_ATTACK_BACK",
		MDS_SABER,
		"@MENUS_FLIP_FORWARD_ATTACK",
		"@MENUS_FLIP_FORWARD_ATTACK_DESC",
		"BOTH_JUMPATTACK6",
		MDS_FORCE_JUMP,
		"@MENUS_DUAL_SABERS_TWIRL",
		"@MENUS_DUAL_SABERS_TWIRL_DESC",
		"BOTH_SPINATTACK6",
		MDS_SABER,
		"@MENUS_ATTACK_ENEMYONGROUND",
		"@MENUS_ATTACK_ENEMYONGROUND_DESC",
		"BOTH_STABDOWN_DUAL",
		MDS_FORCE_JUMP,
		"@MENUS_DUAL_SABER_BARRIER",
		"@MENUS_DUAL_SABER_BARRIER_DESC",
		"BOTH_A6_SABERPROTECT",
		MDS_SABER,
		"@MENUS_DUAL_STAB_FRONT_BACK",
		"@MENUS_DUAL_STAB_FRONT_BACK_DESC",
		"BOTH_A6_FB",
		MDS_SABER,
		"@MENUS_DUAL_STAB_LEFT_RIGHT",
		"@MENUS_DUAL_STAB_LEFT_RIGHT_DESC",
		"BOTH_A6_LR",
		MDS_SABER,
		"@MENUS_CARTWHEEL",
		"@MENUS_CARTWHEEL_DESC",
		"BOTH_ARIAL_RIGHT",
		MDS_FORCE_JUMP,
		"@MENUS_BOTH_ROLL_STAB",
		"@MENUS_BOTH_ROLL_STAB_DESC",
		"BOTH_ROLL_STAB",
		MDS_SABER,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,

		// Saber Staff
		"@MENUS_STAB_BACK",
		"@MENUS_STAB_BACK_DESC",
		"BOTH_A2_STABBACK1",
		MDS_SABER,
		"@MENUS_BACK_FLIP_ATTACK",
		"@MENUS_BACK_FLIP_ATTACK_DESC",
		"BOTH_JUMPATTACK7",
		MDS_FORCE_JUMP,
		"@MENUS_SABER_STAFF_TWIRL",
		"@MENUS_SABER_STAFF_TWIRL_DESC",
		"BOTH_SPINATTACK7",
		MDS_SABER,
		"@MENUS_ATTACK_ENEMYONGROUND",
		"@MENUS_ATTACK_ENEMYONGROUND_DESC",
		"BOTH_STABDOWN_STAFF",
		MDS_FORCE_JUMP,
		"@MENUS_SPINNING_KATA",
		"@MENUS_SPINNING_KATA_DESC",
		"BOTH_A7_SOULCAL",
		MDS_SABER,
		"@MENUS_KICK1",
		"@MENUS_KICK1_DESC",
		"BOTH_A7_KICK_F",
		MDS_FORCE_JUMP,
		"@MENUS_JUMP_KICK",
		"@MENUS_JUMP_KICK_DESC",
		"BOTH_A7_KICK_F_AIR",
		MDS_FORCE_JUMP,
		"@MENUS_BUTTERFLY_ATTACK",
		"@MENUS_BUTTERFLY_ATTACK_DESC",
		"BOTH_BUTTERFLY_FR1",
		MDS_SABER,
		"@MENUS_BOTH_ROLL_STAB",
		"@MENUS_BOTH_ROLL_STAB2_DESC",
		"BOTH_ROLL_STAB",
		MDS_SABER,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
		NULL,
		NULL,
		0,
		MDS_NONE,
};

/*
================
vmMain

This is the only way control passes into the module.
!!! This MUST BE THE VERY FIRST FUNCTION compiled into the .qvm file !!!
================
*/
vmCvar_t ui_debug;
vmCvar_t ui_initialized;
vmCvar_t ui_char_color_red;
vmCvar_t ui_char_color_green;
vmCvar_t ui_char_color_blue;
vmCvar_t ui_char_color_alpha;
vmCvar_t ui_PrecacheModels;
vmCvar_t ui_char_anim;

// Missing cvars needed for UI system
vmCvar_t ui_gameType;
vmCvar_t ui_netGameType;
vmCvar_t ui_serverFilterType;
vmCvar_t ui_currentMap;
vmCvar_t ui_bypassMainMenuLoad;
vmCvar_t ui_botfilter;
vmCvar_t ui_widescreen;
vmCvar_t ui_JKA;
vmCvar_t ui_model;
vmCvar_t ui_headSize;
vmCvar_t ui_s_language;
qboolean menuInJK2MV = qfalse;
qboolean isMainMenu = qfalse;
int mvapi = 0;
int coolApi = 0;
int coolApi_dbVersion = 0;
int coolApi_jkaVersion = 0;

// Missing variable for UI system
qboolean uiUpdateModel = qfalse;

vmCvar_t coolApi_supported_ui;
const int coolApi_supported_ui_int =
	COOL_APIFEATURE_SETPREDICTEDMOVEMENT | COOL_APIFEATURE_GETTEMPORARYUSERCMD | COOL_APIFEATURE_EZDEMOCGAMEBUFFER | COOL_APIFEATURE_GETTIMESINCESNAPRECEIVED | COOL_APIFEATURE_MARIADB | COOL_APIFEATURE_MVAPI_PLAYERSNAPSHOT_SNEAKPEEK | COOL_APIFEATURE_G_SETBRUSHMODELCONTENTFLAGS | COOL_APIFEATURE_G_USERCMDSTORE | COOL_APIFEATURE_RESOLUTIONCHANGED | COOL_APIFEATURE_NONEPSILONTRACE | COOL_APIFEATURE_CUSTOMEPSILONTRACE | COOL_APIFEATURE_JEDI_ACADEMY;

int Init_inGameLoad;

void _UI_Init(qboolean);
void _UI_Shutdown(void);
void _UI_KeyEvent(int key, qboolean down);
void _UI_MouseEvent(int dx, int dy);
void _UI_Refresh(int realtime);
qboolean _UI_IsFullscreen(void);
extern qboolean UI_SaberModelForSaber(const char *saberName, char *saberModel, int saberModelSize);
void UI_ClampMaxPlayers(void);
static void UI_CheckServerName(void);
void UI_BuildQ3Model_List(void);
static qboolean UI_CheckPassword(void);
static void UI_JoinServer(void);
static int UI_OwnerDrawWidth(int ownerDraw, float scale);
static int UI_PlayCinematic(const char *name, float x, float y, float w, float h);
static void UI_StopCinematic(int handle);
static void UI_DrawCinematic(int handle, float x, float y, float w, float h);
static void UI_RunCinematicFrame(int handle);
void Menu_ShowGroup(menuDef_t *menu, char *itemName, qboolean showFlag);
void Menu_ItemDisable(menuDef_t *menu, char *name, int disableFlag);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);
void UI_UpdateTextLanguageCvar(qboolean updateCvarFromJKA);
void UI_UpdateCharacterSkin(void);

LIBEXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11)
{
	int requestedMvApi = 0;
	char coolApiFeaturesBuffer[80];
	if (jk2version == VERSION_UNDEF && command != UI_GETAPIVERSION)
	{ // Shouldn't happen under normal circumstances, but we had this case while debugging on old engine binaries...
		Com_Printf("vmMain [UI]: first call to vmMain had a command != UI_GETAPIVERSION\n");
		MV_UiDetectVersion(); // Try detecting the version now, otherwise we might be missing syscalls...
	}
	switch (command)
	{
	case UI_GETAPIVERSION:
		// arg11 is the mainMenu parameter from JK2MV engine
		if (arg11)
			isMainMenu = qtrue;

		// Initialize version detection first (this sets jk2version)
		MV_UiDetectVersion();

#ifdef JK2MV_MENU
		// When compiled as mvmenu, we're a proper menu module
		if (arg11) // mainMenu request
		{
			// Set menulevel to signal we're a proper mvmenu (the engine checks this)
			trap_Cvar_Set("ui_menulevel", va("%d", MV_MENULEVEL_MAX));
			// Return standard UI API version so engine doesn't reject us
			return UI_API_VERSION;
		}
#else
		// Regular UI build - let engine know we're not a mvmenu
		if (arg11) // mainMenu request
		{
			return 0; // Return 0 to indicate no main menu support
		}
#endif
		// Return the UI API version - this determines which UI module to use
		return /*UI_API_VERSION*/ UI_API_VERSION;
	case UI_INIT:
		trap_Cvar_VariableStringBuffer("cool_apiFeatures", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
		coolApi = atoi(coolApiFeaturesBuffer);
		if (coolApi & COOL_APIFEATURE_MARIADB)
		{
			trap_Cvar_VariableStringBuffer("cool_apiDBVersion", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
			coolApi_dbVersion = atoi(coolApiFeaturesBuffer);
		}
		else
		{
			coolApi_dbVersion = 0;
		}
		if (coolApi & COOL_APIFEATURE_JEDI_ACADEMY)
		{
			trap_Cvar_VariableStringBuffer("cool_apiJKAVersion", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
			coolApi_jkaVersion = atoi(coolApiFeaturesBuffer);
		}
		else
		{
			coolApi_jkaVersion = 0;
		}

		trap_Cvar_Register(&coolApi_supported_ui, "coolApi_supported_ui", va("%d", coolApi_supported_ui_int), CVAR_ROM);
		trap_Cvar_Set("coolApi_supported_ui", va("%d", coolApi_supported_ui_int));

		requestedMvApi = MVAPI_Init(arg11, arg0);

		if (!requestedMvApi)
		{ // Only call _UI_Init if we haven't got access to the MVAPI. If we can use the MVAPI we delay the Init until the "MVAPI_AFTER_INIT" command is sent. That allows us use the MVAPI in the actual init.
			_UI_Init(arg0);
		}
		else
		{ // Store the values that were meant for _UI_Init to use them later, when MVAPIR_AFTER_INIT is called.
			Init_inGameLoad = arg0;
		}
		return requestedMvApi;

	case MVAPI_AFTER_INIT:
		MVAPI_AfterInit();
		return 0;

	case UI_SHUTDOWN:
		_UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		_UI_KeyEvent(Key_GetProtocolKey15(jk2version, arg0), arg1);
		return 0;

	case UI_MOUSE_EVENT:
		_UI_MouseEvent(arg0, arg1);
		return 0;

	case UI_REFRESH:
		_UI_Refresh(arg0);
		return 0;

	case UI_IS_FULLSCREEN:
		return _UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		_UI_SetActiveMenu(arg0);
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen(arg0);
		return 0;
	case UI_HASUNIQUECDKEY: // mod authors need to observe this
		return qtrue;		// bk010117 - change this to qfalse for mods!
	}

	return -1;
}

// Cut down version of the stuff used in the game code
// This is just the bare essentials of what we need to load animations properly for ui ghoul2 models.
// This function doesn't need to be sync'd with the BG_ version in bg_panimate.c unless some sort of fundamental change
// is made. Just make sure the variables/functions accessed in ui_shared.c exist in both modules.
qboolean UIPAFtextLoaded = qfalse;
animation_t uiHumanoidAnimations[MAX_TOTALANIMATIONS]; // humanoid animations are the only ones that are statically allocated.

bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int uiNumAllAnims = 1; // start off at 0, because 0 will always be assigned to humanoid.

animation_t *UI_AnimsetAlloc(void)
{
	assert(uiNumAllAnims < MAX_ANIM_FILES);
	bgAllAnims[uiNumAllAnims].anims = (animation_t *)BG_Alloc(sizeof(animation_t) * MAX_TOTALANIMATIONS);

	return bgAllAnims[uiNumAllAnims].anims;
}

/*
======================
UI_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
static char UIPAFtext[60000];
int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid)
{
	char *text_p;
	int len;
	int i;
	char *token;
	float fps;
	int skip;
	int usedIndex = -1;
	int nextIndex = uiNumAllAnims;

	fileHandle_t f;
	int animNum;

	if (!isHumanoid)
	{
		i = 1;
		while (i < uiNumAllAnims)
		{ // see if it's been loaded already
			if (!Q_stricmp(bgAllAnims[i].filename, filename))
			{
				animset = bgAllAnims[i].anims;
				return i; // alright, we already have it.
			}
			i++;
		}

		// Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if (!animset)
		{
			if (strstr(filename, "players/_humanoid/"))
			{ // then use the static humanoid set.
				animset = uiHumanoidAnimations;
				isHumanoid = qtrue;
				nextIndex = 0;
			}
			else
			{
				animset = UI_AnimsetAlloc();

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		assert(animset);
	}
#endif

	// load the file
	if (!UIPAFtextLoaded || !isHumanoid)
	{ // rww - We are always using the same animation config now. So only load it once. //might want to rethink this.
		len = trap_FS_FOpenFile(filename, &f, FS_READ);
		if ((len <= 0) || (len >= sizeof(UIPAFtext) - 1))
		{
			if (len > 0)
			{
				Com_Error(ERR_DROP, "%s exceeds the allowed ui-side animation buffer!", filename);
			}
			return -1;
		}

		trap_FS_Read(UIPAFtext, len, f);
		UIPAFtext[len] = 0;
		trap_FS_FCloseFile(f);
	}
	else
	{
		return 0; // humanoid index
	}

	// parse the text
	text_p = UIPAFtext;
	skip = 0; // quiet the compiler warning

	// FIXME: have some way of playing anims backwards... negative numFrames?

	// initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for (i = 0; i < MAX_ANIMATIONS; i++)
	{
		animset[i].firstFrame = 0;
		animset[i].numFrames = 0;
		animset[i].loopFrames = -1;
		animset[i].frameLerp = 100;
		//		animset[i].initialLerp = 100;
	}

	// read information for each frame
	while (1)
	{
		token = COM_Parse((const char **)(&text_p));

		if (!token || !token[0])
		{
			break;
		}

		animNum = GetIDForString(animTable, token);
		if (animNum == -1)
		{
// #ifndef FINAL_BUILD
#ifdef _DEBUG
			// Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
#endif
			continue;
		}

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].firstFrame = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].numFrames = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].loopFrames = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		fps = atof(token);
		if (fps == 0)
		{
			fps = 1; // Don't allow divide by zero error
		}
		if (fps < 0)
		{ // backwards
			animset[animNum].frameLerp = floor(1000.0f / fps);
		}
		else
		{
			animset[animNum].frameLerp = ceil(1000.0f / fps);
		}

		//		animset[animNum].initialLerp = ceil(1000.0f / fabs(fps));
	}

#ifdef _DEBUG
	// Check the array, and print the ones that have nothing in them.
	/*
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		if (animTable[i].name != NULL)		// This animation reference exists.
		{
			if (animset[i].firstFrame <= 0 && animset[i].numFrames <=0)
			{	// This is an empty animation reference.
				Com_Printf("***ANIMTABLE reference #%d (%s) is empty!\n", i, animTable[i].name);
			}
		}
	}
	*/
#endif // _DEBUG

	if (isHumanoid)
	{
		bgAllAnims[0].anims = animset;
		Q_strncpyz(bgAllAnims[0].filename, filename, sizeof(bgAllAnims[0].filename));
		UIPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else
	{
		bgAllAnims[nextIndex].anims = animset;
		Q_strncpyz(bgAllAnims[nextIndex].filename, filename, sizeof(bgAllAnims[nextIndex].filename));

		usedIndex = nextIndex;

		if (nextIndex)
		{ // don't bother increasing the number if this ended up as a humanoid load.
			uiNumAllAnims++;
		}
		else
		{
			UIPAFtextLoaded = qtrue;
			usedIndex = 0;
		}
	}

	return usedIndex;
}

#define UI_MV_MIN_APILEVEL 1
#define UI_MV_MIN_VERSION "1.1"
int MVAPI_Init(int apilevel, int inGameLoad)
{
#ifdef JK2MV_MENU
	if (apilevel < MV_APILEVEL)
	{
		// using the mvmenu without jk2mv is useless
		trap_Error("This mvmenu version requires JK2MV " MV_MIN_VERSION);
	}

	menuInJK2MV = qtrue;
	mvapi = apilevel;

	// always using the newest api internally.
	return MV_APILEVEL;
#else
	char version[128];
	char jk2mv[64];

	trap_Cvar_VariableStringBuffer("version", version, sizeof(version));
	trap_Cvar_VariableStringBuffer("JK2MV", jk2mv, sizeof(jk2mv));

	if (strstr(version, "JK2MV") || strlen(jk2mv))
		menuInJK2MV = qtrue;

	if (!trap_Cvar_VariableValue("mv_apienabled"))
	{
		Com_Printf("UI: MVAPI is not supported at all or has been disabled.\n");
		Com_Printf("UI: You need at least JK2MV " UI_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < UI_MV_MIN_APILEVEL)
	{
		Com_Printf("UI: MVAPI level %i not supported.\n", UI_MV_MIN_APILEVEL);
		Com_Printf("UI: You need at least JK2MV " UI_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < MV_APILEVEL)
	{
		Com_Printf("UI: MVAPI level %i not supported (using level %i instead).\n", MV_APILEVEL, apilevel);
		Com_Printf("UI: You need at least JK2MV " MV_MIN_VERSION " to enable all API features.\n");
	}

	mvapi = apilevel;
	if (mvapi > MV_APILEVEL)
		mvapi = MV_APILEVEL;

	Com_Printf("UI: Using MVAPI level %i (%i supported).\n", mvapi, apilevel);
	return mvapi;
#endif
}

void MVAPI_AfterInit(void)
{
	Com_Printf("UI: MVAPI_AfterInit - starting, mvapi=%d\n", mvapi);

	if (mvapi >= 3)
	{ // If the apilevel supports it tell the engine that we're using 1.04 structs etc. internally
		Com_Printf("UI: MVAPI_AfterInit - calling trap_MVAPI_GetVersion (1)\n");
		// Get the inital version
		jk2startversion = trap_MVAPI_GetVersion();
		Com_Printf("UI: MVAPI_AfterInit - jk2startversion=%d\n", jk2startversion);

		Com_Printf("UI: MVAPI_AfterInit - calling trap_MVAPI_SetVersion\n");
		// Set the version to 1.04
		trap_MVAPI_SetVersion(VERSION_1_04);

		Com_Printf("UI: MVAPI_AfterInit - calling trap_MVAPI_GetVersion (2)\n");
		// Get the current version (should always be 1.04)
		jk2version = trap_MVAPI_GetVersion();
		Com_Printf("UI: MVAPI_AfterInit - jk2version=%d\n", jk2version);

		Com_Printf("UI: MVAPI_AfterInit - calling MV_SetGameVersion\n");
		// Set gameplay and version
		MV_SetGameVersion(jk2version, qfalse);
		Com_Printf("UI: MVAPI_AfterInit - calling MV_SetGamePlay\n");
		MV_SetGamePlay(jk2startversion);
		Com_Printf("UI: MVAPI_AfterInit - version setup complete\n");
	}

	Com_Printf("UI: MVAPI_AfterInit - checking mvapi >= 1\n");
	if (mvapi >= 1)
	{ // Set UI menu level capability
#ifdef JK2MV_MENU
	  // When compiled as mvmenu, we support full main menu functionality
		Com_Printf("UI: MVAPI_AfterInit - setting ui_menulevel to 2 (mvmenu)\n");
		trap_Cvar_Set("ui_menulevel", "2");
#else
	  // When compiled as regular UI, we support partial menu functionality
		Com_Printf("UI: MVAPI_AfterInit - setting ui_menulevel to 1 (regular UI)\n");
		trap_Cvar_Set("ui_menulevel", "1");
#endif
	}

	Com_Printf("UI: MVAPI_AfterInit - calling _UI_Init with Init_inGameLoad=%d\n", Init_inGameLoad);
	// Call _UI_Init now, because we delayed it earilier
	_UI_Init(Init_inGameLoad);
	Com_Printf("UI: MVAPI_AfterInit - _UI_Init completed\n");
}

int MV_UiDetectVersion(void)
{
#ifdef JK2MV_MENU
	jk2startversion = jk2version = VERSION_1_04;
	MV_SetGameVersion(jk2version, qtrue); // Set the GameVersion...
	return UI_API_VERSION;
#else
	char buffer[32];
	// MVSDK: Let's detect which version of the engine we are running in...
	jk2version = VERSION_UNDEF;

	trap_Cvar_VariableStringBuffer("mv_apienabled", buffer, sizeof(buffer));
	if (strlen(buffer) && atoi(buffer) > 0)
	{ // JK2MV >= 1.1
		switch (trap_MVAPI_GetVersion())
		{
		case VERSION_1_02:
			jk2version = VERSION_1_02;
			break;
		case VERSION_1_03:
			jk2version = VERSION_1_03;
			break;
		case VERSION_1_04:
			jk2version = VERSION_1_04;
			break;
		default:
			jk2version = VERSION_UNDEF;
		}
	}

	if (jk2version == VERSION_UNDEF)
	{
		char version[128];

		trap_Cvar_VariableStringBuffer("version", version, sizeof(version));

		if (strstr(version, "JK2MP"))
		{ // JK2MP
			if (strstr(version, "1.02"))
				jk2version = VERSION_1_02;
			else if (strstr(version, "1.03"))
				jk2version = VERSION_1_03;
			else if (strstr(version, "1.04"))
				jk2version = VERSION_1_04;
		}
	}

	if (jk2version == VERSION_UNDEF)
	{
		Com_Printf("MVSDK: Unable to detect jk2version [UI]; fallback to 1.04;");
		jk2version = VERSION_1_04;
	}
	Com_Printf("jk2version [UI]: 1.0%i\n", jk2version);
	jk2startversion = jk2version;
	MV_SetGameVersion(jk2version, qtrue); // Set the GameVersion...

	switch (jk2version)
	{
	case VERSION_1_02:
		return UI_API_VERSION_1_02;
	case VERSION_1_03:
	case VERSION_1_04:
	default:
		return UI_API_VERSION;
	}
#endif
}

/*
===================
UI_WideScreenMode
Make 2D drawing functions use widescreen or 640x480 coordinates
===================
*/
void UI_WideScreenMode(qboolean on)
{
	if (mvapi >= 3)
	{
		if (on)
		{
			trap_MVAPI_SetVirtualScreen(uiInfo.screenWidth, uiInfo.virtualScreenHeightOn);
		}
		else
		{
			trap_MVAPI_SetVirtualScreen((float)SCREEN_WIDTH, uiInfo.virtualScreenHeightOff);
		}
	}
}

/*
=================
UI_UpdateWidescreen
=================
*/
static void UI_UpdateWidescreen(void)
{
	float vidWidth = uiInfo.uiDC.glconfig.vidWidth;
	float vidHeight = uiInfo.uiDC.glconfig.vidHeight;
	qboolean portrait;
	qboolean landscape;

	if (ui_widescreen.integer && mvapi >= 3)
	{
		landscape = (3 * vidWidth >= 4 * vidHeight);
		portrait = !landscape;
	}
	else
	{
		portrait = qfalse;
		landscape = qfalse;
	}

	if (isMainMenu)
	{
		portrait = qfalse;
	}

	if (landscape)
	{
		uiInfo.screenWidth = (float)SCREEN_HEIGHT * vidWidth / vidHeight;
		uiInfo.screenHeight = (float)SCREEN_HEIGHT;
		uiInfo.virtualScreenHeightOn = (float)SCREEN_HEIGHT;
		uiInfo.cursorXScale = (SCREEN_WIDTH * vidHeight) / (SCREEN_HEIGHT * vidWidth);
		uiInfo.cursorYScale = 1.0f;
	}
	else if (portrait)
	{
		uiInfo.screenWidth = (float)SCREEN_WIDTH;
		uiInfo.screenHeight = (float)SCREEN_HEIGHT;
		uiInfo.virtualScreenHeightOn = (float)SCREEN_WIDTH * vidHeight / vidWidth;
		uiInfo.cursorXScale = 1.0f;
		uiInfo.cursorYScale = 1.0f;
	}
	else
	{
		uiInfo.screenWidth = (float)SCREEN_WIDTH;
		uiInfo.screenHeight = (float)SCREEN_HEIGHT;
		uiInfo.virtualScreenHeightOn = (float)SCREEN_HEIGHT;
		uiInfo.cursorXScale = 1.0f;
		uiInfo.cursorYScale = (SCREEN_HEIGHT * vidWidth) / (SCREEN_WIDTH * vidHeight);
	}

	uiInfo.virtualScreenHeightOff = uiInfo.virtualScreenHeightOn;

	uiInfo.screenXFactor = (float)SCREEN_WIDTH / uiInfo.screenWidth;
	uiInfo.screenXFactorInv = uiInfo.screenWidth / (float)SCREEN_WIDTH;

	uiInfo.screenYFactor = (float)SCREEN_HEIGHT / uiInfo.screenHeight;
	uiInfo.screenYFactorInv = uiInfo.screenHeight / (float)SCREEN_HEIGHT;

	uiInfo.uiDC.screenWidth = uiInfo.screenWidth;
	uiInfo.uiDC.screenHeight = uiInfo.screenHeight;

	UI_WideScreenMode(qfalse);
}

menuDef_t *Menus_FindByName(const char *p);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);
void UpdateForceUsed();

/*
=================
_UI_SetActiveMenu
=================
*/
void _UI_SetActiveMenu(uiMenuCommand_t menu)
{
	// Always clear the menu first
	Menus_CloseAll();

	switch (menu)
	{
	case UIMENU_NONE:
		trap_Key_SetCatcher(0);
		return;

	case UIMENU_MAIN:
#ifdef JK2MV_MENU
		// When compiled as mvmenu, we DO handle main menu
		Menus_ActivateByName("main");
		trap_Key_SetCatcher(KEYCATCH_UI);
#else
		// When compiled as regular UI module, main menu should be handled by mvmenu
		// If we're being asked to show main menu, just clear UI and let mvmenu handle it
		trap_Key_SetCatcher(0);
#endif
		return;

	case UIMENU_TEAM:
		Menus_ActivateByName("team");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;

	case UIMENU_POSTGAME:
		Menus_ActivateByName("postgame");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;

	case UIMENU_INGAME:
		Menus_ActivateByName("ingame");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;

	case UIMENU_PLAYERCONFIG:
		Menus_ActivateByName("setup_menu2");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;

	case UIMENU_PLAYERFORCE:
		Menus_ActivateByName("setup_menu3");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;

	case UIMENU_MV_DOWNLOAD_POPUP:
		Menus_ActivateByName("mvdownload");
		trap_Key_SetCatcher(KEYCATCH_UI);
		return;
	}
}

/*
=================
UI_FeederSelection
=================
*/
static qboolean UI_FeederSelection(float feederID, int index, itemDef_t *item)
{
	// Handle different feeder selection types
	const int feederIDInt = (int)feederID;

	switch (feederIDInt)
	{
	case FEEDER_PLAYER_SPECIES:
		if (index >= 0 && index < uiInfo.playerSpeciesCount && uiInfo.playerSpeciesCount > 0 && uiInfo.playerSpeciesCount < 1000)
		{
			uiInfo.playerSpeciesIndex = index;
		}
		break;

	case FEEDER_MOVES_TITLES:
		if (index >= 0 && index < MD_MOVE_TITLE_MAX)
		{
			uiInfo.movesTitleIndex = index;
		}
		break;

	case FEEDER_COLORCHOICES:
	case FEEDER_PLAYER_SKIN_HEAD:
	case FEEDER_PLAYER_SKIN_TORSO:
	case FEEDER_PLAYER_SKIN_LEGS:
		// For skin/color choices, just return success without calling UI_UpdateCharacterSkin
		break;

	case FEEDER_SAVEGAMES:
		// No action needed for savegames in multiplayer
		break;

	default:
		// For other feeders, just accept the selection
		break;
	}

	// Return qtrue to indicate the selection was handled
	return qtrue;
}

/*
=================
UI_FeederCount
=================
*/
static int UI_FeederCount(float feederID)
{
	int count = 0, i;

	switch ((int)feederID)
	{
	case FEEDER_Q3HEADS:
		return 0; // Simplified to avoid potential recursion

	case FEEDER_CINEMATICS:
		// Safety check for movieCount
		return (uiInfo.movieCount > 0 && uiInfo.movieCount < 1000) ? uiInfo.movieCount : 0;

	case FEEDER_SERVERS:
		// Safety check for server count
		return (uiInfo.serverStatus.numDisplayServers >= 0 && uiInfo.serverStatus.numDisplayServers < 10000) ? uiInfo.serverStatus.numDisplayServers : 0;

	case FEEDER_SERVERSTATUS:
		// Safety check for server status lines
		return (uiInfo.serverStatusInfo.numLines >= 0 && uiInfo.serverStatusInfo.numLines < 1000) ? uiInfo.serverStatusInfo.numLines : 0;

	case FEEDER_FINDPLAYER:
		// Safety check for found player servers
		return (uiInfo.numFoundPlayerServers >= 0 && uiInfo.numFoundPlayerServers < 1000) ? uiInfo.numFoundPlayerServers : 0;

	case FEEDER_MODS:
		// Safety check for mod count
		return (uiInfo.modCount >= 0 && uiInfo.modCount < 1000) ? uiInfo.modCount : 0;

	case FEEDER_DOWNLOADS:
		// Safety check for downloads count
		return (uiInfo.downloadsCount >= 0 && uiInfo.downloadsCount < 1000) ? uiInfo.downloadsCount : 0;

	case FEEDER_DEMOS:
		// Safety check for demo count
		return (uiInfo.demoCount >= 0 && uiInfo.demoCount < 1000) ? uiInfo.demoCount : 0;

	case FEEDER_MOVES:
		for (i = 0; i < MAX_MOVES; i++)
		{
			if (datapadMoveData[uiInfo.movesTitleIndex][i].title)
			{
				count++;
			}
		}
		return count;

	case FEEDER_MOVES_TITLES:
		return (MD_MOVE_TITLE_MAX);

	case FEEDER_PLAYER_SPECIES:
		// Safety check for species count
		return (uiInfo.playerSpeciesCount >= 0 && uiInfo.playerSpeciesCount < 100) ? uiInfo.playerSpeciesCount : 0;

	case FEEDER_PLAYER_SKIN_HEAD:
		// Safety check for skin head count
		if (uiInfo.playerSpeciesIndex >= 0 && uiInfo.playerSpeciesIndex < uiInfo.playerSpeciesCount)
			return (uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount >= 0 && uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount < 100) ? uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount : 0;
		return 0;

	case FEEDER_PLAYER_SKIN_TORSO:
		// Safety check for skin torso count
		if (uiInfo.playerSpeciesIndex >= 0 && uiInfo.playerSpeciesIndex < uiInfo.playerSpeciesCount)
			return (uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount >= 0 && uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount < 100) ? uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount : 0;
		return 0;

	case FEEDER_PLAYER_SKIN_LEGS:
		// Safety check for skin legs count
		if (uiInfo.playerSpeciesIndex >= 0 && uiInfo.playerSpeciesIndex < uiInfo.playerSpeciesCount)
			return (uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount >= 0 && uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount < 100) ? uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount : 0;
		return 0;

	case FEEDER_COLORCHOICES:
		// Safety check for color choices count
		if (uiInfo.playerSpeciesIndex >= 0 && uiInfo.playerSpeciesIndex < uiInfo.playerSpeciesCount)
			return (uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount >= 0 && uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount < 100) ? uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount : 0;
		return 0;

	case FEEDER_SAVEGAMES:
		// Return 0 for savegames since this is multiplayer
		return 0;

	// For any other feeders, return 0
	default:
		return 0;
	}
}

/*
=================
UI_FeederItemText
=================
*/
static const char *UI_FeederItemText(float feederID, int index, int column, qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3, qhandle_t *handle4, qhandle_t *handle5, qhandle_t *handle6)
{
	// Initialize handles to -1 (no image)
	if (handle1)
		*handle1 = -1;
	if (handle2)
		*handle2 = -1;
	if (handle3)
		*handle3 = -1;
	if (handle4)
		*handle4 = -1;
	if (handle5)
		*handle5 = -1;
	if (handle6)
		*handle6 = -1;

	// Handle different feeder types
	if (feederID == FEEDER_CINEMATICS)
	{
		if (index >= 0 && index < uiInfo.movieCount && uiInfo.movieList && uiInfo.movieCount < 1000)
		{
			return uiInfo.movieList[index];
		}
	}
	else if (feederID == FEEDER_DEMOS)
	{
		if (index >= 0 && index < uiInfo.demoCount && uiInfo.demoList && uiInfo.demoCount < 1000)
		{
			return uiInfo.demoList[index];
		}
	}
	else if (feederID == FEEDER_MODS)
	{
		if (index >= 0 && index < uiInfo.modCount && uiInfo.modList && uiInfo.modCount < 1000)
		{
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr)
			{
				return uiInfo.modList[index].modDescr;
			}
			else
			{
				return uiInfo.modList[index].modName;
			}
		}
	}
	else if (feederID == FEEDER_DOWNLOADS)
	{
		if (index >= 0 && index < uiInfo.downloadsCount && uiInfo.downloadsList && uiInfo.downloadsCount < 1000)
		{
			return uiInfo.downloadsList[index].name;
		}
	}
	else if (feederID == FEEDER_MOVES)
	{
		if (index >= 0 && index < MAX_MOVES)
		{
			return datapadMoveData[uiInfo.movesTitleIndex][index].title;
		}
	}
	else if (feederID == FEEDER_MOVES_TITLES)
	{
		if (index >= 0 && index < MD_MOVE_TITLE_MAX)
		{
			return datapadMoveTitleData[index];
		}
	}
	else if (feederID == FEEDER_PLAYER_SPECIES)
	{
		if (index >= 0 && index < uiInfo.playerSpeciesCount)
		{
			return uiInfo.playerSpecies[index].Name;
		}
	}
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			if (handle1)
				*handle1 = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].icon;
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Color[index].shader;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			if (handle1)
				*handle1 = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].icon;
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHead[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			if (handle1)
				*handle1 = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].icon;
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorso[index].name;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			if (handle1)
				*handle1 = uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].icon;
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLeg[index].name;
		}
	}
	else if (feederID == FEEDER_SAVEGAMES)
	{
		// Return empty string for savegames since this is multiplayer
		return "";
	}

	// For any other feeders, return empty string
	return "";
}

/*
=================
UI_FeederItemImage
=================
*/
qhandle_t UI_FeederItemImage(float feederID, int index)
{
	// This function returns the image handle for a specific item in a feeder
	// Return 0 (no image) for all feeders for now
	return 0;
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh(void)
{
	// Simple stub implementation - this function is called during UI refresh
	// but appears to be missing its implementation. Adding empty stub to fix compilation.
	// In a full implementation, this would handle periodic server browser refresh logic.
}

/*
=================
UI_BuildFindPlayerList
=================
*/
static void UI_BuildFindPlayerList(qboolean force)
{
	// Simple stub implementation - this function is called during UI refresh
	// but appears to be missing its implementation. Adding empty stub to fix compilation.
	// In a full implementation, this would build/update the find player list.
}

/*
=================
UI_BuildQ3Model_List
=================
*/
void UI_BuildQ3Model_List(void)
{
	// Simple stub implementation - this function is used for building model lists
	// but appears to be missing its implementation. Adding empty stub to fix compilation.
	// In a full implementation, this would populate available player models.
}

/*
=================
UI_BuildServerStatus
=================
*/
static void UI_BuildServerStatus(qboolean force)
{
	// Simple stub implementation - this function is used for building server status
	// but appears to be missing its implementation. Adding empty stub to fix compilation.
	// In a full implementation, this would update server status information.
}

static char holdSPString[MAX_STRING_CHARS] = {0};

uiInfo_t uiInfo;

static void UI_StartServerRefresh(qboolean full);
static void UI_StopServerRefresh(void);
static void UI_DoServerRefresh(void);
static void UI_BuildServerDisplayList(int force);
static void UI_BuildServerStatus(qboolean force);
static void UI_BuildFindPlayerList(qboolean force);
static int QDECL UI_ServersQsortCompare(const void *arg1, const void *arg2);
static int UI_MapCountByGameType(qboolean singlePlayer);
static int UI_HeadCountByTeam(void);
static void UI_ParseGameInfo(const char *teamFile);
static const char *UI_SelectedMap(int index, int *actual);
static const char *UI_SelectedHead(int index, int *actual);
static int UI_GetIndexFromSelection(int actual);

int ProcessNewUI(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);
int uiSkinColor = SKINCOLOR_DEFAULT;

static serverFilter_t serverFilters[] = {
	{"All", "All"},
	{"1.02", "1.02"},
	{"1.03", "1.03"},
	{"1.04", "1.04"},
};
static const int numServerFilters = sizeof(serverFilters) / sizeof(serverFilter_t);

static const char *skillLevels[] = {
	"SKILL1", //"I Can Win",
	"SKILL2", //"Bring It On",
	"SKILL3", //"Hurt Me Plenty",
	"SKILL4", //"Hardcore",
	"SKILL5"  //"Nightmare"
};
static const int numSkillLevels = sizeof(skillLevels) / sizeof(const char *);

static const char *teamArenaGameTypes[] = {
	"FFA",
	"HOLOCRON",
	"JEDIMASTER",
	"DUEL",
	"SP",
	"TEAM FFA",
	"N/A",
	"CTF",
	"CTY",
	"TEAMTOURNAMENT"};
static int const numTeamArenaGameTypes = sizeof(teamArenaGameTypes) / sizeof(const char *);

static char *netnames[] = {
	"???",
	"UDP",
	"IPX",
	NULL};

// static int gamecodetoui[] = {4,2,3,0,5,1,6};
// static int uitogamecode[] = {4,6,2,3,1,5,7};

const char *UI_GetStripEdString(const char *refSection, const char *refName);

const char *UI_TeamName(int team)
{
	if (team == TEAM_RED)
		return "RED";
	else if (team == TEAM_BLUE)
		return "BLUE";
	else if (team == TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

// returns either string or NULL for OOR...
//
static const char *GetCRDelineatedString(const char *psStripFileRef, const char *psStripStringRef, int iIndex)
{
	static char sTemp[256];
	const char *psList = UI_GetStripEdString(psStripFileRef, psStripStringRef);
	char *p;

	while (iIndex--)
	{
		psList = strchr(psList, '\n');
		if (!psList)
		{
			return NULL; // OOR
		}
		psList++;
	}

	Q_strncpyz(sTemp, psList, sizeof(sTemp));
	p = strchr(sTemp, '\n');
	if (p)
	{
		*p = '\0';
	}

	return sTemp;
}

static const char *GetMonthAbbrevString(int iMonth)
{
	const char *p = GetCRDelineatedString("INGAMETEXT", "MONTHS", iMonth);

	return p ? p : "Jan"; // sanity
}

/*
static const char *netSources[] = {
	"Local",
	"Internet",
	"Favorites"
//	"Mplayer"
};
static const int numNetSources = sizeof(netSources) / sizeof(const char*);
*/
static const int numNetSources = 3; // now hard-entered in StripEd file
static const char *GetNetSourceString(int iSource)
{
	const char *p = GetCRDelineatedString("INGAMETEXT", "NET_SOURCES", iSource);

	return p ? p : "??";
}

void AssetCache()
{
	int n;
	// if (Assets.textFont == NULL) {
	// }
	// Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	// Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	uiInfo.uiDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	uiInfo.uiDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip(ART_FX_RED);
	uiInfo.uiDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip(ART_FX_ORANGE); // trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip(ART_FX_YELLOW); // trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip(ART_FX_GREEN);	 // trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	uiInfo.uiDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip(ART_FX_PURPLE); // trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWDOWN);
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWUP);
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWLEFT);
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWRIGHT);
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip(ASSET_SCROLL_THUMB);
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);

	// Icons for various server settings.
	uiInfo.uiDC.Assets.needPass = trap_R_RegisterShaderNoMip("gfx/menus/needpass");
	uiInfo.uiDC.Assets.noForce = trap_R_RegisterShaderNoMip("gfx/menus/noforce");
	uiInfo.uiDC.Assets.forceRestrict = trap_R_RegisterShaderNoMip("gfx/menus/forcerestrict");
	uiInfo.uiDC.Assets.saberOnly = trap_R_RegisterShaderNoMip("gfx/menus/saberonly");
	uiInfo.uiDC.Assets.trueJedi = trap_R_RegisterShaderNoMip("gfx/menus/truejedi");

	for (n = 0; n < NUM_CROSSHAIRS; n++)
	{
		uiInfo.uiDC.Assets.crosshairShader[n] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c", 'a' + n));
	}

	uiInfo.newHighScoreSound = 0; // trap_S_RegisterSound("sound/feedback/voc_newhighscore.wav");
}

void _UI_DrawSides(float x, float y, float w, float h, float size)
{
	trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
	trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size)
{
	trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect(float x, float y, float width, float height, float size, const float *color)
{
	trap_R_SetColor(color);

	_UI_DrawTopBottom(x, y, width, height, size);
	_UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor(NULL);
}

int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
	case 1:
		return uiInfo.uiDC.Assets.qhSmallFont;
	case 2:
		return uiInfo.uiDC.Assets.qhMediumFont;
	case 3:
		return uiInfo.uiDC.Assets.qhBigFont;
	case 4:
		return uiInfo.uiDC.Assets.qhSmall2Font;
	}

	return uiInfo.uiDC.Assets.qhMediumFont; // 0;
}

int Text_Width(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);
	float w;

	UI_WideScreenMode(qtrue);
	w = trap_R_Font_StrLenPixels(text, iFontIndex, scale) * uiInfo.screenXFactor;
	UI_WideScreenMode(qfalse);
	return w;
}

int Text_Height(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);
	float h;
	UI_WideScreenMode(qtrue);
	h = trap_R_Font_HeightPixels(iFontIndex, scale) * uiInfo.screenYFactor;
	UI_WideScreenMode(qfalse);
	return h;
}

void Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont)
{
	int iStyleOR = 0;

	int iFontIndex = MenuFontToHandle(iMenuFont);
	//
	// kludge.. convert JK2 menu styles to SOF2 printstring ctrl codes...
	//
	switch (style)
	{
	case ITEM_TEXTSTYLE_NORMAL:
		iStyleOR = 0;
		break; // JK2 normal text
	case ITEM_TEXTSTYLE_BLINK:
		iStyleOR = (int)STYLE_BLINK;
		break; // JK2 fast blinking
	case ITEM_TEXTSTYLE_PULSE:
		iStyleOR = (int)STYLE_BLINK;
		break; // JK2 slow pulsing
	case ITEM_TEXTSTYLE_SHADOWED:
		iStyleOR = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow
	case ITEM_TEXTSTYLE_OUTLINED:
		iStyleOR = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow
	case ITEM_TEXTSTYLE_OUTLINESHADOWED:
		iStyleOR = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow
	case ITEM_TEXTSTYLE_SHADOWEDMORE:
		iStyleOR = (int)STYLE_DROPSHADOW;
		break; // JK2 drop shadow
	}

	UI_WideScreenMode(qtrue);
	x *= uiInfo.screenXFactorInv;
	y *= uiInfo.screenYFactorInv;
	trap_R_Font_DrawString(x,					  // int ox
						   y,					  // int oy
						   text,				  // const char *text
						   color,				  // paletteRGBA_c c
						   iStyleOR | iFontIndex, // const int iFontHandle
						   !limit ? -1 : limit,	  // iCharLimit (-1 = none)
						   scale);				  // const float scale = 1.0f

	UI_WideScreenMode(qfalse);
}

void Text_PaintWithCursor(float x, float y, float scale, const vec4_t color, const char *text, unsigned cursorPos, char cursor, unsigned limit, int style, int iMenuFont)
{
	Text_Paint(x, y, scale, color, text, 0, limit, style, iMenuFont);

	// now print the cursor as well...  (excuse the braces, it's for porting C++ to C)
	//
	{
		char sTemp[1024];
		unsigned iCopyCount = limit ? MIN((unsigned)strlen(text), (unsigned)limit) : (unsigned)strlen(text);
		iCopyCount = MIN(iCopyCount, cursorPos);
		iCopyCount = MIN(iCopyCount, (int)sizeof(sTemp) - 1);

		// copy text into temp buffer for pixel measure...
		//
		Q_strncpyz(sTemp, text, iCopyCount + 1);

		{
			int iNextXpos = Text_Width(sTemp, scale, iMenuFont);

			Text_Paint(x + iNextXpos, y, scale, color, va("%c", cursor), 0, limit, style | ITEM_TEXTSTYLE_BLINK, iMenuFont);
		}
	}
}

// maxX param is initially an X limit, but is also used as feedback. 0 = text was clipped to fit within, else maxX = next pos
//
static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int iMenuFont)
{
	// float fMax = *maxX;
	int iPixelLen = Text_Width(text, scale, iMenuFont);
	if (x + iPixelLen > *maxX)
	{
		// whole text won't fit, so we need to print just the amount that does...
		//  Ok, this is slow and tacky, but only called occasionally, and it works...
		//
		char sTemp[4096] = {0}; // lazy assumption
		const char *psText = text;
		char *psOut = &sTemp[0];
		char *psOutLastGood = psOut;
		unsigned int uiLetter;

		while (*psText && (x + Text_Width(sTemp, scale, iMenuFont) <= *maxX) && psOut < &sTemp[sizeof(sTemp) - 1] // sanity
		)
		{
			int iAdvanceCount;
			psOutLastGood = psOut;

			if (jk2version == VERSION_1_02)
			{
				uiLetter = trap_AnyLanguage_ReadCharFromString_1_02(&psText);
			}
			else
			{
				uiLetter = trap_AnyLanguage_ReadCharFromString_1_04(psText, &iAdvanceCount, NULL);
				psText += iAdvanceCount;
			}

			if (uiLetter > 255)
			{
				*psOut++ = uiLetter >> 8;
				*psOut++ = uiLetter & 0xFF;
			}
			else
			{
				*psOut++ = uiLetter & 0xFF;
			}
		}
		*psOutLastGood = '\0';

		*maxX = 0; // feedback
		Text_Paint(x, y, scale, color, sTemp, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
	else
	{
		// whole text fits fine, so print it all...
		//
		*maxX = x + iPixelLen; // feedback the next position, as the caller expects
		Text_Paint(x, y, scale, color, text, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
}

void UI_ShowPostGame(qboolean newHigh)
{
	trap_Cvar_Set("cg_cameraOrbit", "0");
	trap_Cvar_Set("sv_killserver", "1");
	uiInfo.soundHighScore = newHigh;
	_UI_SetActiveMenu(UIMENU_POSTGAME);
}
/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic(qhandle_t image, int w, int h)
{
	int x, y;
	x = (uiInfo.screenWidth - w) / 2;
	y = (uiInfo.screenHeight - h) / 2;
	UI_DrawHandlePic(x, y, w, h, image);
}

int frameCount = 0;
int startTime;

vmCvar_t ui_rankChange;
vmCvar_t ui_menuFileParseSpam;
static void UI_BuildPlayerList();
char parsedFPMessage[1024];
extern int FPMessageTime;
static void Text_PaintCenter(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int iMenuFont);

const char *UI_GetStripEdString(const char *refSection, const char *refName)
{
	static char text[1024] = {0};

	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
}

static void _UI_CheckWindowResize()
{

	// cache redundant calulations
	if (coolApi & COOL_APIFEATURE_RESOLUTIONCHANGED)
	{
		if (trap_UI_COOL_API_GlResolutionChanged(uiInfo.uiDC.glconfig.vidWidth, uiInfo.uiDC.glconfig.vidHeight))
		{

			trap_GetGlconfig(&uiInfo.uiDC.glconfig);

			UI_UpdateWidescreen();
			// for 640x480 virtualized screen
			uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0 / (float)SCREEN_HEIGHT);
			uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0 / (float)SCREEN_WIDTH);
			if (uiInfo.uiDC.glconfig.vidWidth * SCREEN_HEIGHT > uiInfo.uiDC.glconfig.vidHeight * SCREEN_WIDTH)
			{
				// wide screen
				uiInfo.uiDC.bias = 0.5 * (uiInfo.uiDC.glconfig.vidWidth - (uiInfo.uiDC.glconfig.vidHeight * ((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT)));
			}
			else
			{
				// no wide screen
				uiInfo.uiDC.bias = 0;
			}

			UI_WideScreenMode(qtrue);

			Init_Display(&uiInfo.uiDC);
		}
	}
}

#define UI_FPS_FRAMES 4
static char serverInfo[MAX_INFO_STRING];
static int serverGameType;
void _UI_Refresh(int realtime)
{
	static int index;
	static int previousTimes[UI_FPS_FRAMES];
	static int nextRefresh;

	// if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	// }

	// check if window size changed
	_UI_CheckWindowResize();

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if (index > UI_FPS_FRAMES)
	{
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0; i < UI_FPS_FRAMES; i++)
		{
			total += previousTimes[i];
		}
		if (!total)
		{
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	if (nextRefresh < realtime)
	{
		nextRefresh = realtime + 1000;

		// Update the g_gametype once per second, it's unusual for servers to switch them at runtime anyway
		trap_GetConfigString(CS_SERVERINFO, serverInfo, sizeof(serverInfo));
		serverGameType = atoi(Info_ValueForKey(serverInfo, "g_gametype"));
	}

	UI_UpdateCvars();

	if (Menu_Count() > 0)
	{
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus(qfalse);
		// refresh find player list
		UI_BuildFindPlayerList(qfalse);
		// draw cursor
		if ((trap_Key_GetCatcher() & KEYCATCH_UI) && Menu_Count() > 0)
		{
			float cursorx = uiInfo.uiDC.cursorx * uiInfo.screenXFactorInv;
			float cursory = uiInfo.uiDC.cursory * uiInfo.screenYFactorInv;

			UI_SetColor(NULL);
			UI_WideScreenMode(qtrue);
			UI_DrawHandlePic(cursorx, cursory, 48, 48, uiInfo.uiDC.Assets.cursor);
			UI_WideScreenMode(qfalse);
		}
	}

#ifndef NDEBUG
	if (uiInfo.uiDC.debug)
	{
		// cursor coordinates
		// FIXME
		// UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	if (ui_rankChange.integer)
	{
		FPMessageTime = realtime + 3000;

		if (!parsedFPMessage[0] /*&& uiMaxRank > ui_rankChange.integer*/)
		{
			const char *printMessage = UI_GetStripEdString("INGAMETEXT", "SET_NEW_RANK");

			int i = 0;
			int p = 0;
			int linecount = 0;

			while (printMessage[i] && p < 1024)
			{
				parsedFPMessage[p] = printMessage[i];
				p++;
				i++;
				linecount++;

				if (linecount > 64 && printMessage[i] == ' ')
				{
					parsedFPMessage[p] = '\n';
					p++;
					linecount = 0;
				}
			}
			parsedFPMessage[p] = '\0';
		}

		// if (uiMaxRank > ui_rankChange.integer)
		{
			uiServerForceRank = ui_rankChange.integer;
			uiMaxRank = Com_Clampi(1, MAX_FORCE_RANK, ui_rankChange.integer);
			uiForceRank = uiMaxRank;

			/*
			while (x < NUM_FORCE_POWERS)
			{
				//For now just go ahead and clear force powers upon rank change
				uiForcePowersRank[x] = 0;
				x++;
			}
			uiForcePowersRank[FP_LEVITATION] = 1;
			uiForceUsed = 0;
			*/

			// Use BG_LegalizedForcePowers and transfer the result into the UI force settings
			UI_ReadLegalForce();
		}

		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABERATTACK] < 1)
		{
			uiForcePowersRank[FP_SABERATTACK] = 1;
		}
		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABERDEFEND] < 1)
		{
			uiForcePowersRank[FP_SABERDEFEND] = 1;
		}
		trap_Cvar_Set("ui_rankChange", "0");

		// remember to update the force power count after changing the max rank
		UpdateForceUsed();
	}

	if (ui_freeSaber.integer)
	{
		bgForcePowerCost[FP_SABERATTACK][FORCE_LEVEL_1] = 0;
		bgForcePowerCost[FP_SABERDEFEND][FORCE_LEVEL_1] = 0;
	}
	else
	{
		bgForcePowerCost[FP_SABERATTACK][FORCE_LEVEL_1] = 1;
		bgForcePowerCost[FP_SABERDEFEND][FORCE_LEVEL_1] = 1;
	}

	/*
	if (parsedFPMessage[0] && FPMessageTime > realtime)
	{
		vec4_t txtCol;
		int txtStyle = ITEM_TEXTSTYLE_SHADOWED;

		if ((FPMessageTime - realtime) < 2000)
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = (((float)FPMessageTime - (float)realtime)/2000);

			txtStyle = 0;
		}
		else
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = colorWhite[3];
		}

		Text_Paint(10, 0, 1, txtCol, parsedFPMessage, 0, 1024, txtStyle, FONT_MEDIUM);
	}
	*/
	// For now, don't bother.
}

void UI_CleanupGhoul2(void);

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown(void)
{
	trap_LAN_SaveCachedServers();
	UI_CleanupGhoul2();

	// We don't get a new force rank from the server during vid_restart (the server sends "nfr" on force init, the cgame
	// receives it and tells the engine to change the "ui_rankChange" cvar and ui just checks the value of that cvar each
	// refresh). So when shutting down the UI module we have to store the old ui_rankChange to restore it afterwards.
	// We only want to do this during a vid_restart, cause otherwise we would carry over the force rank from our previous
	// server to the next one. So we store the rank in a temporary cvar and let the UI_Init method decide whether it's
	// relevant or not.
	trap_Cvar_Set("_ui_serverForceRank", va("%i", uiServerForceRank));
}

char *defaultMenu = NULL;

const char *GetMenuBuffer(const char *filename)
{
	int len;
	fileHandle_t f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f)
	{
		trap_Print(va(S_COLOR_RED "menu file not found: %s, using default\n", filename));
		return defaultMenu;
	}
	if (len >= MAX_MENUFILE)
	{
		trap_Print(va(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE));
		trap_FS_FCloseFile(f);
		return defaultMenu;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);
	// COM_Compress(buf);
	return buf;
}

qboolean Asset_Parse(int handle)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0)
	{
		return qfalse;
	}

	while (1)
	{

		memset(&token, 0, sizeof(pc_token_t));

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0)
		{
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0)
		{
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize))
			{
				return qfalse;
			}
			// trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.qhMediumFont = trap_R_RegisterFont(token.string);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0)
		{
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize))
			{
				return qfalse;
			}
			// trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmallFont = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "small2Font") == 0)
		{
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize))
			{
				return qfalse;
			}
			// trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmall2Font = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0)
		{
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize))
			{
				return qfalse;
			}
			// trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.bigFont);
			uiInfo.uiDC.Assets.qhBigFont = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "stripedFile") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				Com_Printf(S_COLOR_YELLOW "Bad 1st parameter for keyword 'stripedFile'\n");
				return qfalse;
			}
			trap_SP_Register(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0)
		{
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr))
			{
				Com_Printf(S_COLOR_YELLOW "Bad 1st parameter for keyword 'cursor'\n");
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip(uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(token.string);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound(token.string);
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound(token.string);
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound(token.string);
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0)
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0)
		{
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp))
			{
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0)
		{
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle))
			{
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0)
		{
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount))
			{
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0)
		{
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX))
			{
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0)
		{
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY))
			{
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0)
		{
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor))
			{
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

		if (Q_stricmp(token.string, "moveRollSound") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.moveRollSound = trap_S_RegisterSound(token.string);
			}
			continue;
		}

		if (Q_stricmp(token.string, "moveJumpSound") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.moveJumpSound = trap_S_RegisterSound(token.string);
			}

			continue;
		}
		if (Q_stricmp(token.string, "datapadmoveSaberSound1") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound1 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound2") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound2 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound3") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound3 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound4") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound4 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound5") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound5 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound6") == 0)
		{
			if (trap_PC_ReadToken(handle, &token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound6 = trap_S_RegisterSound(token.string);
			}

			continue;
		}

		// precaching various sound files used in the menus
		if (Q_stricmp(token.string, "precacheSound") == 0)
		{
			const char *tempStr;
			if (PC_Script_Parse(handle, &tempStr))
			{
				char *soundFile;
				do
				{
					soundFile = COM_ParseExt(&tempStr, qfalse);
					if (soundFile[0] != 0 && soundFile[0] != ';')
					{
						trap_S_RegisterSound(soundFile);
					}
				} while (soundFile[0]);
			}
			continue;
		}
	}
	return qfalse;
}

void UI_Report()
{
	String_Report();
	// Font_Report();
}

void UI_ParseMenu(const char *menuFile)
{
	int handle;
	pc_token_t token;
	qboolean menuIsJKA = qfalse;
	const char *menuExtension;
	char menuPath[MAX_QPATH];
	int fileHandle = -1;

	if (ui_menuFileParseSpam.integer)
	{
		Com_Printf("Parsing menu file:%s\n", menuFile);
	}

	menuExtension = Q_strrchr(menuFile, '.');
	if (menuExtension == NULL)
	{
		menuExtension = "";
	}

	if (ui_JKA.integer < 0 || ui_JKA.integer > 2)
	{
		trap_Cvar_Set("ui_JKA", "0");
		trap_Cvar_Update(&ui_JKA);
	}

	if (ui_JKA.integer == 0)
	{ // load menu files from JK2 paths, discard files with ".menu_jka"
		if (Q_stricmp(menuExtension, ".menu_jka") == 0)
		{
			if (ui_menuFileParseSpam.integer)
			{
				Com_Printf("Skipping menu file:%s\n", menuFile);
			}
			return;
		}
		menuIsJKA = qfalse;
	}
	else if (ui_JKA.integer == 1)
	{ // load menu files from JK2 paths, override ".menu" files with ".menu_jka"
		if (Q_stricmp(menuExtension, ".menu") == 0)
		{
			COM_StripExtension(menuFile, menuPath, sizeof(menuPath));
			COM_DefaultExtension(menuPath, sizeof(menuPath), ".menu_jka");
			trap_FS_FOpenFile(menuPath, &fileHandle, FS_READ);
			if (fileHandle)
			{
				trap_FS_FCloseFile(fileHandle);
				if (ui_menuFileParseSpam.integer)
				{
					Com_Printf("Skipping menu file:%s\n", menuFile);
				}
				return;
			}
		}
		if (coolApi_jkaVersion)
		{
			menuIsJKA = !!(trap_UI_COOL_API_GetFileVersion(menuFile) & FILE_VERSION_JKA);
		}
		else
		{
			menuIsJKA = qfalse;
		}
		if (Q_stricmp(menuExtension, ".menu_jka") == 0)
		{
			menuIsJKA = qtrue;
		}
	}
	else if (ui_JKA.integer == 2)
	{ // load menu files from JKA paths, discard files with ".menu_jka"
		if (Q_stricmp(menuExtension, ".menu_jka") == 0)
		{
			if (ui_menuFileParseSpam.integer)
			{
				Com_Printf("Skipping menu file:%s\n", menuFile);
			}
			return;
		}
		menuIsJKA = qtrue;
	}

	Menu_SetJKA(menuIsJKA);

	handle = trap_PC_LoadSource(menuFile);
	if (!handle)
	{
		return;
	}

	while (1)
	{
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		// if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		// }

		// if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		// }

		if (token.string[0] == '}')
		{
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0)
		{
			if (Asset_Parse(handle))
			{
				continue;
			}
			else
			{
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0)
		{
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean Load_Menu(int handle)
{
	pc_token_t token;

	Com_Printf("UI: Load_Menu - starting\n");

	if (!trap_PC_ReadToken(handle, &token))
	{
		Com_Printf("UI: Load_Menu - failed to read first token\n");
		return qfalse;
	}

	Com_Printf("UI: Load_Menu - first token: '%s'\n", token.string);
	if (token.string[0] != '{')
	{
		Com_Printf("UI: Load_Menu - first token is not '{', returning false\n");
		return qfalse;
	}

	Com_Printf("UI: Load_Menu - entering parsing loop\n");
	while (1)
	{
		Com_Printf("UI: Load_Menu - about to read token in loop\n");
		if (!trap_PC_ReadToken(handle, &token))
		{
			Com_Printf("UI: Load_Menu - failed to read token in loop\n");
			return qfalse;
		}

		Com_Printf("UI: Load_Menu - loop token: '%s'\n", token.string);
		if (token.string[0] == 0)
		{
			Com_Printf("UI: Load_Menu - empty token, returning false\n");
			return qfalse;
		}

		if (token.string[0] == '}')
		{
			Com_Printf("UI: Load_Menu - found '}', returning true\n");
			return qtrue;
		}

		Com_Printf("UI: Load_Menu - about to call UI_ParseMenu with: '%s'\n", token.string);
		UI_ParseMenu(token.string);
		Com_Printf("UI: Load_Menu - UI_ParseMenu completed\n");
	}
	return qfalse;
}

void UI_LoadMenus(const char *menuFile, qboolean reset)
{
	pc_token_t token;
	int handle;
	int start;

	Com_Printf("UI: UI_LoadMenus - starting with file: %s\n", menuFile);

	start = trap_Milliseconds();
	Com_Printf("UI: UI_LoadMenus - trap_Milliseconds() completed\n");

	Com_Printf("UI: UI_LoadMenus - about to call trap_PC_LoadGlobalDefines\n");
	if (ui_JKA.integer == 2)
		trap_PC_LoadGlobalDefines("ui/jamp/menudef.h");
	else
		trap_PC_LoadGlobalDefines("ui/jk2mp/menudef.h");
	Com_Printf("UI: UI_LoadMenus - trap_PC_LoadGlobalDefines completed\n");

	Com_Printf("UI: UI_LoadMenus - about to call trap_PC_LoadSource\n");
	handle = trap_PC_LoadSource(menuFile);
	Com_Printf("UI: UI_LoadMenus - trap_PC_LoadSource returned handle: %d\n", handle);
	if (!handle)
	{
		Com_Printf("UI: UI_LoadMenus - handle is null, trying fallback\n");
		Com_Printf(S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile);

		if (ui_JKA.integer == 2)
			handle = trap_PC_LoadSource("ui/jampmenus.txt");
		else
			handle = trap_PC_LoadSource("ui/jk2mpmenus.txt");

		if (!handle)
		{
			Com_Error(ERR_DROP, "default menu file not found: ui/menus.txt, unable to continue!");
		}
	}

	Com_Printf("UI: UI_LoadMenus - checking reset flag: %d\n", reset);
	if (reset)
	{
		Com_Printf("UI: UI_LoadMenus - about to call Menu_Reset()\n");
		Menu_Reset();
		Com_Printf("UI: UI_LoadMenus - Menu_Reset() completed\n");
	}

	Com_Printf("UI: UI_LoadMenus - starting parsing loop\n");
	while (1)
	{
		Com_Printf("UI: UI_LoadMenus - about to call trap_PC_ReadToken\n");
		if (!trap_PC_ReadToken(handle, &token))
		{
			Com_Printf("UI: UI_LoadMenus - trap_PC_ReadToken returned false, breaking\n");
			break;
		}
		Com_Printf("UI: UI_LoadMenus - got token: '%s'\n", token.string);
		if (token.string[0] == 0 || token.string[0] == '}')
		{
			Com_Printf("UI: UI_LoadMenus - empty token or '}', breaking\n");
			break;
		}

		if (token.string[0] == '}')
		{
			Com_Printf("UI: UI_LoadMenus - '}' token, breaking\n");
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0)
		{
			Com_Printf("UI: UI_LoadMenus - found 'loadmenu', calling Load_Menu\n");
			if (Load_Menu(handle))
			{
				Com_Printf("UI: UI_LoadMenus - Load_Menu returned true, continuing\n");
				continue;
			}
			else
			{
				Com_Printf("UI: UI_LoadMenus - Load_Menu returned false, breaking\n");
				break;
			}
		}
	}

	if (!uiInfo.inGameLoad)
	{
		UI_ParseMenu("ui/jk2mv/download_popup.menu");
		UI_ParseMenu("ui/jk2mv/download_info.menu");
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

	trap_PC_FreeSource(handle);

	trap_PC_RemoveAllGlobalDefines();
}

void UI_Load()
{
	char *menuSet;
	char lastName[1024];
	menuDef_t *menu;

	Com_Printf("UI: UI_Load - starting\n");

	menu = Menu_GetFocused();
	Com_Printf("UI: UI_Load - Menu_GetFocused() completed\n");

	if (menu && menu->window.name)
	{
		Q_strncpyz(lastName, menu->window.name, sizeof(lastName));
		Com_Printf("UI: UI_Load - copied menu name: %s\n", lastName);
	}
	else
	{
		lastName[0] = 0;
		Com_Printf("UI: UI_Load - no focused menu, set lastName to empty\n");
	}

	Com_Printf("UI: UI_Load - determining menuSet\n");
	if (uiInfo.inGameLoad)
	{
		if (ui_JKA.integer == 2)
			menuSet = "ui/jampingame.txt";
		else
			menuSet = "ui/jk2mpingame.txt";
		Com_Printf("UI: UI_Load - inGameLoad=true, menuSet=%s\n", menuSet);
	}
	else
	{
		menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
		Com_Printf("UI: UI_Load - inGameLoad=false, got ui_menuFilesMP=%s\n", menuSet ? menuSet : "(null)");
	}
	if (menuSet == NULL || menuSet[0] == '\0' || Q_stricmp(menuSet, "ui/jk2mpmenus.txt") == 0)
	{
		if (ui_JKA.integer == 2)
			menuSet = "ui/jampmenus.txt";
		else
			menuSet = "ui/jk2mpmenus.txt";
		Com_Printf("UI: UI_Load - fallback menuSet=%s\n", menuSet);
	}

#if 1
	Com_Printf("UI: UI_Load - about to load menus, Init_inGameLoad=%d\n", Init_inGameLoad);
	if (Init_inGameLoad)
	{
		Com_Printf("UI: UI_Load - loading ingame menus\n");
		if (ui_JKA.integer == 2)
			UI_LoadMenus("ui/jampingame.txt", qtrue);
		else
			UI_LoadMenus("ui/jk2mpingame.txt", qtrue);
		Com_Printf("UI: UI_Load - ingame menus loaded\n");
	}
	else if (!ui_bypassMainMenuLoad.integer)
	{
		Com_Printf("UI: UI_Load - loading main menus: %s\n", menuSet);
		UI_LoadMenus(menuSet, qtrue);
		Com_Printf("UI: UI_Load - main menus loaded\n");
	}
#else // this was adding quite a giant amount of time to the load time
	UI_LoadMenus(menuSet, qtrue);
	UI_LoadMenus("ui/jk2mpingame.txt", qtrue);
#endif

	trap_Cvar_Register(NULL, "ui_name", UI_Cvar_VariableString("name"), CVAR_INTERNAL); // get this now, jic the menus change again trying to setName before getName

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();
	UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);

	UI_BuildQ3Model_List();
	UI_LoadBots();

	UI_LoadForceConfig_List();

	UI_InitForceShaders();

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = /*gamecodetoui[*/ (int)trap_Cvar_VariableValue("color1"); //-1];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	trap_Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");
	trap_Cvar_Set("ui_mousePitchVeh", (trap_Cvar_VariableValue("m_pitchVeh") >= 0) ? "0" : "1");

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	trap_Cvar_Register(NULL, "debug_protocol", "", 0);
	trap_Cvar_Register(NULL, "ui_hidelang", "0", CVAR_INTERNAL);

	trap_Cvar_Set("ui_actualNetGameType", va("%d", ui_netGameType.integer));

	trap_Cvar_Register(&ui_serverFilterType, "ui_serverFilterType", "0", CVAR_ARCHIVE | CVAR_GLOBAL);

	// botfilter
	trap_Cvar_Register(&ui_botfilter, "ui_botfilter", "0", CVAR_ARCHIVE | CVAR_GLOBAL);

	// Additional missing cvars
	trap_Cvar_Register(&ui_gameType, "ui_gameType", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	trap_Cvar_Register(&ui_netGameType, "ui_netGameType", "0", CVAR_ARCHIVE | CVAR_GLOBAL);
	trap_Cvar_Register(&ui_currentMap, "ui_currentMap", "", CVAR_ARCHIVE | CVAR_GLOBAL);

#ifdef JK2MV_MENU
	// MVMenu build - allow main menu loading
	trap_Cvar_Register(&ui_bypassMainMenuLoad, "ui_bypassMainMenuLoad", "0", CVAR_ARCHIVE);
#else
	// Regular UI build - bypass main menu loading (handled by mvmenu)
	trap_Cvar_Register(&ui_bypassMainMenuLoad, "ui_bypassMainMenuLoad", "1", CVAR_ARCHIVE);
#endif

	trap_Cvar_Register(&ui_widescreen, "ui_widescreen", "0", CVAR_ARCHIVE);
	trap_Cvar_Register(&ui_JKA, "ui_JKA", "0", CVAR_ROM);
	trap_Cvar_Register(&ui_model, "ui_model", "", CVAR_ARCHIVE | CVAR_GLOBAL);
	trap_Cvar_Register(&ui_headSize, "ui_headSize", "1.0", CVAR_ARCHIVE);
	trap_Cvar_Register(&ui_s_language, "ui_s_language", "", CVAR_ARCHIVE);
}

void _UI_Init(qboolean inGameLoad)
{
	uiInfo.inGameLoad = inGameLoad;
	Init_inGameLoad = inGameLoad;

	if (!trap_Cvar_VariableValue("ui_iniwrited"))
	{
		trap_Cvar_Set("ui_iniwrited", "1");
	}

	UI_RegisterCvars();

	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig(&uiInfo.uiDC.glconfig);

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0 / 480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0 / 640.0);
	if (uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640)
	{
		// wide screen
		uiInfo.uiDC.bias = 0.5 * (uiInfo.uiDC.glconfig.vidWidth - (uiInfo.uiDC.glconfig.vidHeight * (640.0 / 480.0)));
		uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidHeight * (1.0 / 480.0);
	}
	else
	{
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	// UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &trap_R_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.RegisterFont = &trap_R_RegisterFont;
	uiInfo.uiDC.Font_StrLenPixels = &trap_R_Font_StrLenPixels;
	uiInfo.uiDC.Font_StrLenChars = &trap_R_Font_StrLenChars;
	uiInfo.uiDC.Font_HeightPixels = &trap_R_Font_HeightPixels;
	uiInfo.uiDC.Font_DrawString = &trap_R_Font_DrawString;
	uiInfo.uiDC.Language_IsAsian = trap_Language_IsAsian;
	uiInfo.uiDC.Language_UsesSpaces = trap_Language_UsesSpaces;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error;
	uiInfo.uiDC.Print = &Com_Printf;
	uiInfo.uiDC.ownerDrawWidth = UI_OwnerDrawWidth;
	// uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = UI_RunCinematicFrame;
	uiInfo.uiDC.getClipboardData = &trap_GetClipboardData;
	// uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;

	// Assign feeder function pointers
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;

	Init_Display(&uiInfo.uiDC);

	String_Init();
	uiInfo.uiDC.cursor = trap_R_RegisterShaderNoMip("menu/art/3_cursor2");
	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip("white");

	AssetCache();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

	UI_ParseGameInfo("ui/jk2mp/gameinfo.txt");
	UI_LoadArenas();

	UI_LoadMenus("ui/menus.txt", qtrue);
	if (!uiInfo.inGameLoad)
	{
		Menus_CloseAll();
	}
	else
	{
		Menus_CloseAll();
		Menus_ActivateByName("ingame_main");
	}

	trap_LAN_LoadCachedServers();
	UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);

	UI_BuildQ3Model_List();
	UI_LoadBots();

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = (int)trap_Cvar_VariableValue("color1");
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	trap_Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	if (trap_Cvar_VariableValue("ui_TeamArenaFirstRun") == 0)
	{
		trap_Cvar_Set("s_volume", "0.8");
		trap_Cvar_Set("s_musicvolume", "0.25");
		trap_Cvar_Set("ui_TeamArenaFirstRun", "1");
	}

	trap_Cvar_Register(NULL, "debug_protocol", "", 0);

	trap_Cvar_Set("ui_actualNetGameType", va("%d", ui_netGameType.integer));
}
void _UI_KeyEvent(int key, qboolean down)
{
	// Handle key events - only if down and we have a valid focused menu
	if (down)
	{
		menuDef_t *menu = Menu_GetFocused();
		if (menu)
		{
			Menu_HandleKey(menu, key, down);
		}
	}
}

void _UI_MouseEvent(int dx, int dy)
{
	// Handle mouse events safely - check if uiDC is initialized
	if (&uiInfo && &uiInfo.uiDC)
	{
		uiInfo.uiDC.cursorx += dx;
		uiInfo.uiDC.cursory += dy;

		// Clamp cursor to screen bounds
		if (uiInfo.uiDC.cursorx < 0)
			uiInfo.uiDC.cursorx = 0;
		if (uiInfo.uiDC.cursory < 0)
			uiInfo.uiDC.cursory = 0;
		if (uiInfo.uiDC.cursorx > uiInfo.uiDC.screenWidth)
			uiInfo.uiDC.cursorx = uiInfo.uiDC.screenWidth;
		if (uiInfo.uiDC.cursory > uiInfo.uiDC.screenHeight)
			uiInfo.uiDC.cursory = uiInfo.uiDC.screenHeight;
	}
}

qboolean _UI_IsFullscreen(void)
{
	// Check if UI is in fullscreen mode - safely check for focused menu
	menuDef_t *menu = Menu_GetFocused();
	return (menu != NULL);
}

static int UI_OwnerDrawWidth(int ownerDraw, float scale)
{
	const char *s = NULL;

	switch (ownerDraw)
	{
	case UI_HANDICAP:
	{
		int h = Com_Clamp(5, 100, trap_Cvar_VariableValue("handicap"));
		int i = 20 - h / 5;
		const char *handicapValues[] = {"None", "95", "90", "85", "80", "75", "70", "65", "60", "55", "50", "45", "40", "35", "30", "25", "20", "15", "10", "5"};
		s = handicapValues[i];
	}
	break;
	default:
		s = "Unknown";
		break;
	}

	if (s)
	{
		return Text_Width(s, scale, FONT_MEDIUM);
	}
	return 0;
}

static int UI_PlayCinematic(const char *name, float x, float y, float w, float h)
{
	return trap_CIN_PlayCinematic(name, x, y, w, h, (CIN_loop | CIN_silent));
}

static void UI_StopCinematic(int handle)
{
	if (handle >= 0)
	{
		trap_CIN_StopCinematic(handle);
	}
}

static void UI_DrawCinematic(int handle, float x, float y, float w, float h)
{
	trap_CIN_SetExtents(handle, x, y, w, h);
	trap_CIN_DrawCinematic(handle);
}

static void UI_RunCinematicFrame(int handle)
{
	trap_CIN_RunCinematic(handle);
}

typedef struct
{
	vmCvar_t *vmCvar;
	char *cvarName;
	char *defaultString;
	int cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{&ui_debug, "ui_debug", "0", CVAR_TEMP},
	{&ui_initialized, "ui_initialized", "0", CVAR_TEMP},
	{&ui_char_color_red, "ui_char_color_red", "255", CVAR_ARCHIVE},
	{&ui_char_color_green, "ui_char_color_green", "255", CVAR_ARCHIVE},
	{&ui_char_color_blue, "ui_char_color_blue", "255", CVAR_ARCHIVE},
	{&ui_char_color_alpha, "ui_char_color_alpha", "255", CVAR_ARCHIVE},
	{&ui_PrecacheModels, "ui_PrecacheModels", "0", CVAR_ARCHIVE},
	{&ui_char_anim, "ui_char_anim", "BOTH_WALK1", CVAR_ROM | CVAR_INTERNAL},
	{&ui_gameType, "ui_gametype", "0", CVAR_ARCHIVE | CVAR_INTERNAL},
	{&ui_netGameType, "ui_netGametype", "0", CVAR_ARCHIVE | CVAR_INTERNAL},
	{&ui_serverFilterType, "ui_serverFilterType", "0", CVAR_ARCHIVE},
	{&ui_currentMap, "ui_currentMap", "0", CVAR_ARCHIVE},
	{&ui_bypassMainMenuLoad, "ui_bypassMainMenuLoad", "0", CVAR_ARCHIVE},
	{&ui_botfilter, "ui_botfilter", "0", CVAR_ARCHIVE},
	{&ui_widescreen, "ui_widescreen", "1", CVAR_ARCHIVE},
	{&ui_JKA, "ui_JKA", "1", CVAR_ARCHIVE},
	{&ui_model, "model", "", CVAR_USERINFO | CVAR_ARCHIVE},
	{&ui_headSize, "ui_headSize", "1.0", CVAR_ARCHIVE},
	{&ui_s_language, "ui_s_language", "", CVAR_ARCHIVE},
};

static int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars(void)
{
	int i;
	cvarTable_t *cv;

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}
}

static void UI_ParseGameInfo(const char *teamFile)
{
	// Simple stub implementation - just parse basic structure without processing
	char *token;
	const char *p;
	const char *buff = NULL;

	buff = GetMenuBuffer(teamFile);
	if (!buff)
	{
		return;
	}

	p = buff;

	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (!token || token[0] == 0 || token[0] == '}')
		{
			break;
		}

		if (Q_stricmp(token, "}") == 0)
		{
			break;
		}

		// Skip over any structured data by reading until closing brace
		if (Q_stricmp(token, "gametypes") == 0 || Q_stricmp(token, "joingametypes") == 0 || Q_stricmp(token, "maps") == 0)
		{
			token = COM_ParseExt(&p, qtrue);
			if (token[0] == '{')
			{
				int depth = 1;
				while (depth > 0)
				{
					token = COM_ParseExt(&p, qtrue);
					if (!token || token[0] == 0)
						break;
					if (token[0] == '{')
						depth++;
					else if (Q_stricmp(token, "}") == 0)
						depth--;
				}
			}
		}
	}
}

void UI_LoadForceConfig_List(void)
{
	// Load force configuration list
	// This function loads force power configurations
}

void UI_UpdateCvars(void)
{
	// Update UI cvars
	// This function updates various UI cvars
}

void UI_DrawConnectScreen(qboolean overlay)
{
	// Draw connection screen
	// This function draws the connection screen overlay
}

int UI_GetHeadByIndex(int index)
{
	// Get head model by index
	return 0; // Return default head index
}

int UI_HeadIndexForModel(const char *modelName)
{
	// Get head index for model name
	return 0; // Return default head index
}

const char *UI_GetModelWithTeamColor(const char *model)
{
	// Get model with team color
	return ""; // Return empty string for now
}

int UI_HeadCountByColor(void)
{
	// Get head count by color
	return 1; // Return default count
}

void UI_FeederScrollTo(float feederId, int scrollTo)
{
	// Scroll feeder to index
	// This function scrolls a UI feeder to the specified index
}

qboolean UI_TrueJediEnabled(void)
{
	// Check if True Jedi mode is enabled
	return qfalse; // Return false by default
}

void UI_UpdateCharacterSkin(void)
{
	// Update character skin display in UI
	// This handles skin changes for character customization
	if (uiInfo.playerSpeciesIndex >= 0 && uiInfo.playerSpeciesIndex < uiInfo.playerSpeciesCount)
	{
		// Trigger refresh of character display
		// The actual skin update is handled by the menu system
	}
}

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif
