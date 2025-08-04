// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "mvsdk_setup.h"

#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

#if !defined(CL_LIGHT_H_INC)
	#include "cg_lights.h"
#endif

/*
Ghoul2 Insert Start
*/
void CG_InitItems(void);
/*
Ghoul2 Insert End
*/

vec4_t colorTable[CT_MAX] = 
{
{0, 0, 0, 0},			// CT_NONE
{0, 0, 0, 1},			// CT_BLACK
{1, 0, 0, 1},			// CT_RED
{0, 1, 0, 1},			// CT_GREEN
{0, 0, 1, 1},			// CT_BLUE
{1, 1, 0, 1},			// CT_YELLOW
{1, 0, 1, 1},			// CT_MAGENTA
{0, 1, 1, 1},			// CT_CYAN
{1, 1, 1, 1},			// CT_WHITE
{0.75f, 0.75f, 0.75f, 1},	// CT_LTGREY
{0.50f, 0.50f, 0.50f, 1},	// CT_MDGREY
{0.25f, 0.25f, 0.25f, 1},	// CT_DKGREY
{0.15f, 0.15f, 0.15f, 1},	// CT_DKGREY2

{0.810f, 0.530f, 0.0f,  1},	// CT_VLTORANGE -- needs values
{0.810f, 0.530f, 0.0f,  1},	// CT_LTORANGE
{0.610f, 0.330f, 0.0f,  1},	// CT_DKORANGE
{0.402f, 0.265f, 0.0f,  1},	// CT_VDKORANGE

{0.503f, 0.375f, 0.996f, 1},	// CT_VLTBLUE1
{0.367f, 0.261f, 0.722f, 1},	// CT_LTBLUE1
{0.199f, 0.0f,   0.398f, 1},	// CT_DKBLUE1
{0.160f, 0.117f, 0.324f, 1},	// CT_VDKBLUE1

{0.300f, 0.628f, 0.816f, 1},	// CT_VLTBLUE2 -- needs values
{0.300f, 0.628f, 0.816f, 1},	// CT_LTBLUE2
{0.191f, 0.289f, 0.457f, 1},	// CT_DKBLUE2
{0.125f, 0.250f, 0.324f, 1},	// CT_VDKBLUE2

{0.796f, 0.398f, 0.199f, 1},	// CT_VLTBROWN1 -- needs values
{0.796f, 0.398f, 0.199f, 1},	// CT_LTBROWN1
{0.558f, 0.207f, 0.027f, 1},	// CT_DKBROWN1
{0.328f, 0.125f, 0.035f, 1},	// CT_VDKBROWN1

{0.996f, 0.796f, 0.398f, 1},	// CT_VLTGOLD1 -- needs values
{0.996f, 0.796f, 0.398f, 1},	// CT_LTGOLD1
{0.605f, 0.441f, 0.113f, 1},	// CT_DKGOLD1
{0.386f, 0.308f, 0.148f, 1},	// CT_VDKGOLD1

{0.648f, 0.562f, 0.784f, 1},	// CT_VLTPURPLE1 -- needs values
{0.648f, 0.562f, 0.784f, 1},	// CT_LTPURPLE1
{0.437f, 0.335f, 0.597f, 1},	// CT_DKPURPLE1
{0.308f, 0.269f, 0.375f, 1},	// CT_VDKPURPLE1

{0.816f, 0.531f, 0.710f, 1},	// CT_VLTPURPLE2 -- needs values
{0.816f, 0.531f, 0.710f, 1},	// CT_LTPURPLE2
{0.566f, 0.269f, 0.457f, 1},	// CT_DKPURPLE2
{0.343f, 0.226f, 0.316f, 1},	// CT_VDKPURPLE2

{0.929f, 0.597f, 0.929f, 1},	// CT_VLTPURPLE3
{0.570f, 0.371f, 0.570f, 1},	// CT_LTPURPLE3
{0.355f, 0.199f, 0.355f, 1},	// CT_DKPURPLE3
{0.285f, 0.136f, 0.230f, 1},	// CT_VDKPURPLE3

{0.953f, 0.378f, 0.250f, 1},	// CT_VLTRED1
{0.953f, 0.378f, 0.250f, 1},	// CT_LTRED1
{0.593f, 0.121f, 0.109f, 1},	// CT_DKRED1
{0.429f, 0.171f, 0.113f, 1},	// CT_VDKRED1
{.25f, 0, 0, 1},					// CT_VDKRED
{.70f, 0, 0, 1},					// CT_DKRED
	
{0.717f, 0.902f, 1.0f,   1},		// CT_VLTAQUA
{0.574f, 0.722f, 0.804f, 1},		// CT_LTAQUA
{0.287f, 0.361f, 0.402f, 1},		// CT_DKAQUA
{0.143f, 0.180f, 0.201f, 1},		// CT_VDKAQUA

{0.871f, 0.386f, 0.375f, 1},		// CT_LTPINK
{0.435f, 0.193f, 0.187f, 1},		// CT_DKPINK
{	  0,    .5f,    .5f, 1},		// CT_LTCYAN
{	  0,   .25f,   .25f, 1},		// CT_DKCYAN
{   .179f, .51f,   .92f, 1},		// CT_LTBLUE3
{   .199f, .71f,   .92f, 1},		// CT_LTBLUE3
{   .5f,   .05f,    .4f, 1},		// CT_DKBLUE3

{   0.0f,   .613f,  .097f, 1},		// CT_HUD_GREEN
{   0.835f, .015f,  .015f, 1},		// CT_HUD_RED
{	.567f,	.685f,	1.0f,	.75f},	// CT_ICON_BLUE
{	.515f,	.406f,	.507f,	1},		// CT_NO_AMMO_RED
{   1.0f,   .658f,  .062f, 1},		// CT_HUD_ORANGE

};

char *HolocronIcons[] = {
	"gfx/mp/f_icon_lt_heal",		//FP_HEAL,
	"gfx/mp/f_icon_levitation",		//FP_LEVITATION,
	"gfx/mp/f_icon_speed",			//FP_SPEED,
	"gfx/mp/f_icon_push",			//FP_PUSH,
	"gfx/mp/f_icon_pull",			//FP_PULL,
	"gfx/mp/f_icon_lt_telepathy",	//FP_TELEPATHY,
	"gfx/mp/f_icon_dk_grip",		//FP_GRIP,
	"gfx/mp/f_icon_dk_l1",			//FP_LIGHTNING,
	"gfx/mp/f_icon_dk_rage",		//FP_RAGE,
	"gfx/mp/f_icon_lt_protect",		//FP_PROTECT,
	"gfx/mp/f_icon_lt_absorb",		//FP_ABSORB,
	"gfx/mp/f_icon_lt_healother",	//FP_TEAM_HEAL,
	"gfx/mp/f_icon_dk_forceother",	//FP_TEAM_FORCE,
	"gfx/mp/f_icon_dk_drain",		//FP_DRAIN,
	"gfx/mp/f_icon_sight",			//FP_SEE,
	"gfx/mp/f_icon_saber_attack",	//FP_SABERATTACK,
	"gfx/mp/f_icon_saber_defend",	//FP_SABERDEFEND,
	"gfx/mp/f_icon_saber_throw"		//FP_SABERTHROW
};

int forceModelModificationCount = -1;
int widescreenModificationCount = -1;
int crosshairColorModificationCount = -1;//japro
int strafeHelperActiveColorModificationCount = -1;//japro

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

void CG_CalcEntityLerpPositions( centity_t *cent );
void CG_ROFF_NotetrackCallback( centity_t *cent, const char *notetrack);

static int	C_PointContents(void);
static void C_GetLerpOrigin(void);
static void C_GetLerpAngles(void);
static void C_GetModelScale(void);
static void C_Trace(void);
static void C_GetBoltPos(void);
static void C_ImpactMark(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
qboolean menuInJK2MV = qfalse;
int mvapi = 0;
qboolean submodelBypass = qfalse;
int Init_serverMessageNum;
int Init_serverCommandSequence;
int Init_clientNum;
LIBEXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
	int requestedMvApi = 0;

	switch ( command ) {
	case CG_INIT:
		requestedMvApi = MVAPI_Init(arg11);
		if ( !requestedMvApi )
		{ // Only call CG_Init if we haven't got access to the MVAPI. If we can use the MVAPI we delay the Init until the "MVAPI_AFTER_INIT" command is sent. That allows us use the MVAPI in the actual init.
			CG_Init( arg0, arg1, arg2 );
		}
		else
		{ // Store the values that were meant for CG_Init to use them later, when MVAPIR_AFTER_INIT is called.
			Init_serverMessageNum = arg0;
			Init_serverCommandSequence = arg1;
			Init_clientNum = arg2;
		}
		return requestedMvApi;
	case MVAPI_AFTER_INIT:
		MVAPI_AfterInit();
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(Key_GetProtocolKey15(jk2version, arg0), arg1); // MVSDK: 1.02 uses other keycodes...
		return 0;
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;

	case CG_POINT_CONTENTS:
		return C_PointContents();

	case CG_GET_LERP_ORIGIN:
		C_GetLerpOrigin();
		return 0;

	case CG_GET_LERP_ANGLES:
		C_GetLerpAngles();
		return 0;

	case CG_GET_MODEL_SCALE:
		C_GetModelScale();
		return 0;

	case CG_GET_GHOUL2:
		return (intptr_t)cg_entities[arg0].ghoul2; //NOTE: This is used by the effect bolting which is actually not used at all.
											  //I'm fairly sure if you try to use it with vm's it will just give you total
											  //garbage. In other words, use at your own risk.

	case CG_GET_MODEL_LIST:
		return (intptr_t)cgs.gameModels;

	case CG_CALC_LERP_POSITIONS:
		CG_CalcEntityLerpPositions( &cg_entities[arg0] );
		return 0;

	case CG_TRACE:
		C_Trace();
		return 0;

	case CG_GET_ORIGIN:
		VectorCopy(cg_entities[arg0].currentState.pos.trBase, (float *)arg1);
		return 0;

	case CG_GET_ANGLES:
		VectorCopy(cg_entities[arg0].currentState.apos.trBase, (float *)arg1);
		return 0;

	case CG_GET_BOLT_POS:
		C_GetBoltPos();
		return 0;

	case CG_GET_ORIGIN_TRAJECTORY:
		return (intptr_t)&cg_entities[arg0].nextState.pos;

	case CG_GET_ANGLE_TRAJECTORY:
		return (intptr_t)&cg_entities[arg0].nextState.apos;

	case CG_ROFF_NOTETRACK_CALLBACK:
		CG_ROFF_NotetrackCallback( &cg_entities[arg0], (const char *)arg1 );
		return 0;

	case CG_IMPACT_MARK:
		C_ImpactMark();
		return 0;

	case CG_MAP_CHANGE:
		// this trap map be called more than once for a given map change, as the
		// server is going to attempt to send out multiple broadcasts in hopes that
		// the client will receive one of them
		cg.mMapChange = qtrue;
		return 0;

	default:
		CG_Error( "vmMain: unknown command %i", (int)command );
		break;
	}
	return -1;
}

#define CGAME_MV_MIN_APILEVEL 1
#define CGAME_MV_MIN_VERSION "1.1"
int MVAPI_Init(int apilevel)
{
	char mv_apiEnabledBuffer[80];
	trap_Cvar_VariableStringBuffer( "mv_apienabled", mv_apiEnabledBuffer, sizeof(mv_apiEnabledBuffer) );

	if (!atoi(mv_apiEnabledBuffer))
	{
		CG_Printf("CGame: MVAPI is not supported at all or has been disabled.\n");
		CG_Printf("CGame: You need at least JK2MV " CGAME_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < CGAME_MV_MIN_APILEVEL)
	{
		CG_Printf("CGame: MVAPI level %i not supported.\n", CGAME_MV_MIN_APILEVEL);
		CG_Printf("CGame: You need at least JK2MV " CGAME_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < MV_APILEVEL)
	{
		CG_Printf("CGame: MVAPI level %i not supported (using level %i instead).\n", MV_APILEVEL, apilevel);
		CG_Printf("CGame: You need at least JK2MV " MV_MIN_VERSION " to enable all API features.\n");
	}

	mvapi = apilevel;
	if ( mvapi > MV_APILEVEL ) mvapi = MV_APILEVEL;

	CG_Printf("CGame: Using MVAPI level %i (%i supported).\n", mvapi, apilevel);
	return mvapi;
}

void MVAPI_AfterInit(void)
{
	if ( mvapi >= 3 )
	{ // If the apilevel supports it tell the engine that we're using 1.04 structs etc. internally
		// Get the inital version
		jk2startversion = trap_MVAPI_GetVersion();
		// Set the version to 1.04
		trap_MVAPI_SetVersion( VERSION_1_04 );
		// Get the current version (should always be 1.04)
		jk2version = trap_MVAPI_GetVersion();

		// Set gameplay and version
		MV_SetGameVersion( jk2version, qfalse );
		MV_SetGamePlay( jk2startversion );
	}

	// Let the engine know we support more than 256 submodels
	if ( mvapi >= 4 ) submodelBypass = trap_MVAPI_EnableSubmodelBypass( qtrue );

	// Call CG_Init now, because we delayed it earilier
	CG_Init( Init_serverMessageNum, Init_serverCommandSequence, Init_clientNum );

	// Disable those JK2MV Engine fixes we can take care of in the VM
	trap_MVAPI_ControlFixes( MVFIX_WPGLOWING );
}

static int C_PointContents(void)
{
	TCGPointContents	*data = (TCGPointContents *)cg.sharedBuffer;

	return CG_PointContents( data->mPoint, data->mPassEntityNum );
}

static void C_GetLerpOrigin(void)
{
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy(cg_entities[data->mEntityNum].lerpOrigin, data->mPoint);
}

static void C_GetLerpAngles(void)
{
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy(cg_entities[data->mEntityNum].lerpAngles, data->mPoint);
}

static void C_GetModelScale(void)
{
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy(cg_entities[data->mEntityNum].modelScale, data->mPoint);
}

static void C_Trace(void)
{
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_Trace(&td->mResult, td->mStart, td->mMins, td->mMaxs, td->mEnd, td->mSkipNumber, td->mMask);
}

static void C_GetBoltPos(void)
{
	TCGBoltPos	*data = (TCGBoltPos *)cg.sharedBuffer;

	if (!cg_entities[data->mEntityNum].ghoul2)
	{
		VectorClear(data->mPoint);
		VectorClear(data->mAngle);
		return;
	}

	VectorCopy(cg_entities[data->mEntityNum].lerpOrigin, data->mPoint);
	if (data->mEntityNum < MAX_CLIENTS)
	{
		VectorCopy(cg_entities[data->mEntityNum].turAngles, data->mAngle);
	}
	else
	{
		VectorCopy(cg_entities[data->mEntityNum].lerpAngles, data->mAngle);
	}
}

static void C_ImpactMark(void)
{
	TCGImpactMark	*data = (TCGImpactMark *)cg.sharedBuffer;

	/*
	CG_ImpactMark((int)arg0, (const float *)arg1, (const float *)arg2, (float)arg3,
		(float)arg4, (float)arg5, (float)arg6, (float)arg7, qtrue, (float)arg8, qfalse);
	*/
	CG_ImpactMark(data->mHandle, data->mPoint, data->mAngle, data->mRotation,
		data->mRed, data->mGreen, data->mBlue, data->mAlphaStart, qtrue, data->mSizeStart, qfalse);
}

/*
Ghoul2 Insert Start
*/
/*
void CG_ResizeG2Bolt(boltInfo_v *bolt, int newCount)
{
	bolt->resize(newCount);
}

void CG_ResizeG2Surface(surfaceInfo_v *surface, int newCount)
{
	surface->resize(newCount);
}

void CG_ResizeG2Bone(boneInfo_v *bone, int newCount)
{
	bone->resize(newCount);
}

void CG_ResizeG2(CGhoul2Info_v *ghoul2, int newCount)
{
	ghoul2->resize(newCount);
}

void CG_ResizeG2TempBone(mdxaBone_v *tempBone, int newCount)
{
	tempBone->resize(newCount);
}
*/
/*
Ghoul2 Insert End
*/
cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];


vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
//vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_dynamicCrosshair;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_drawScores;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugSaber;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;

vmCvar_t	cg_swingAngles;

vmCvar_t	cg_oldPainSounds;

#ifdef G2_COLLISION_ENABLED
vmCvar_t	cg_saberModelTraceEffect;
#endif

vmCvar_t	cg_fpls;

vmCvar_t	cg_saberDynamicMarks;
vmCvar_t	cg_saberDynamicMarkTime;

vmCvar_t	cg_saberContact;
vmCvar_t	cg_saberTrail;

vmCvar_t	cg_duelHeadAngles;

vmCvar_t	cg_speedTrail;
vmCvar_t	cg_auraShell;

vmCvar_t	cg_animBlend;

vmCvar_t	cg_dismember;

//jk2pro Client Cvars - start
vmCvar_t	cjp_client;

vmCvar_t	cg_raceTimer;
vmCvar_t	cg_raceTimerSize;
vmCvar_t	cg_raceTimerX;
vmCvar_t	cg_raceTimerY;
vmCvar_t	cg_speedometer;
vmCvar_t	cg_speedometerX;
vmCvar_t	cg_speedometerY;
vmCvar_t	cg_speedometerSize;
vmCvar_t	cg_showpos;

vmCvar_t	cg_strafeHelperCutoff;
vmCvar_t	cg_strafeHelper;
vmCvar_t	cg_strafeHelperPrecision;
vmCvar_t	cg_strafeHelperLineWidth;
vmCvar_t	cg_strafeHelperActiveColor;
vmCvar_t	cg_strafeHelperInactiveAlpha;

vmCvar_t	cg_strafeHelperOffset;
vmCvar_t	cg_strafeHelper_FPS;

vmCvar_t	cg_crosshairSizeScale;
vmCvar_t	cg_crosshairSaberStyleColor;
vmCvar_t	cg_crosshairColor;
vmCvar_t	cg_crosshairIdentifyTarget;

vmCvar_t	cg_enhancedFlagStatus;
vmCvar_t	cg_drawTimerMsec;
vmCvar_t	cg_movementKeys;
vmCvar_t	cg_movementKeysX;
vmCvar_t	cg_movementKeysY;
vmCvar_t	cg_movementKeysSize;

//only for you, arto
vmCvar_t	cg_hudColors;
vmCvar_t	cg_drawScore;
vmCvar_t	cg_centerHeight;
vmCvar_t	cg_centerSize;

//chatbox
vmCvar_t	cg_chatBox;
vmCvar_t	cg_chatBoxFontSize;
vmCvar_t	cg_chatBoxHeight;
//japro chatbox stuff
vmCvar_t	cg_chatBoxShowHistory;
vmCvar_t	cg_chatBoxX;
vmCvar_t	cg_chatBoxCutOffLength;
vmCvar_t	cg_chatSounds;
vmCvar_t	cg_cleanChatbox;
vmCvar_t	cg_newFont;

vmCvar_t	cg_remaps;
vmCvar_t	cg_autoKillWhenFalling;

vmCvar_t	cg_jumpSounds;
vmCvar_t	cg_rollSounds;
vmCvar_t	cg_hitSounds;
vmCvar_t	cg_newSaberHitSounds;
vmCvar_t	cg_thirdPersonFlagAlpha;
vmCvar_t	cg_drawNonDuelers;
vmCvar_t	cg_brightskins;
vmCvar_t	cg_drawHitBox;
vmCvar_t	cg_playerLOD;
vmCvar_t	cg_privateDuelShell;
vmCvar_t	cg_teamRespawnShield;
vmCvar_t	cg_saberTeamColors;

vmCvar_t	cg_widescreen;
vmCvar_t	cg_fovAspectAdjust;

vmCvar_t	cg_fovViewmodel;
vmCvar_t	cg_fovViewmodelAdjust;

vmCvar_t	cg_fkDuration;
vmCvar_t	cg_fkFirstJumpDuration;
vmCvar_t	cg_fkSecondJumpDelay;

vmCvar_t	cl_commandsize;//Loda - FPS UNLOCK client modcode

vmCvar_t	cg_fixlean; //idk man
vmCvar_t	cg_SPRunAnim;

vmCvar_t	cg_drawInventory;
vmCvar_t	cg_smallScoreboard;
vmCvar_t	cg_colorScoreboard;
vmCvar_t	cg_drawScoreboardIcons;
vmCvar_t	cg_drawPowerUpIcons;
vmCvar_t	cg_drawDemoName;
vmCvar_t	cg_lowhpsound;
vmCvar_t	cg_backSwingCameraRange;
//jk2 pro stuff end

vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPersonPitchOffset;
vmCvar_t	cg_thirdPersonVertOffset;
vmCvar_t	cg_thirdPersonCameraDamp;
vmCvar_t	cg_thirdPersonTargetDamp;

vmCvar_t	cg_thirdPersonAlpha;
vmCvar_t	cg_thirdPersonHorzOffset;

vmCvar_t	cg_stereoSeparation;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawEnemyInfo;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_smoothClients;
vmCvar_t	cg_pmove_fixed;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_pmove_float;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_trueLightning;
/*
Ghoul2 Insert Start
*/
vmCvar_t	cg_debugBB;
/*
Ghoul2 Insert End
*/
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
vmCvar_t	cg_currentSelectedPlayer;
vmCvar_t	cg_currentSelectedPlayerName;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;

vmCvar_t	cg_ui_myteam;
vmCvar_t	cg_com_maxfps;

vmCvar_t	cg_mv_fixbrokenmodelsclient;
vmCvar_t	cg_drawPlayerSprites;
vmCvar_t	cg_developer;
vmCvar_t	cg_smoothCamera;
vmCvar_t	cg_smoothCameraFPS;

vmCvar_t	cg_MVSDK;
vmCvar_t	mvsdk_cgFlags;

vmCvar_t	cg_drawKillMessage;
vmCvar_t	cg_showKills;
vmCvar_t	cg_char_color_red;
vmCvar_t	cg_char_color_green;
vmCvar_t	cg_char_color_blue;
vmCvar_t	cg_char_color_alpha;
vmCvar_t	cg_char_color_red_forced;// when using cg_forcemymodel
vmCvar_t	cg_char_color_green_forced;// when using cg_forcemymodel
vmCvar_t	cg_char_color_blue_forced;// when using cg_forcemymodel
vmCvar_t	cg_char_color_alpha_forced;// when using cg_forcemymodel
vmCvar_t	cg_saber1;
vmCvar_t	cg_saber2;
vmCvar_t	cg_JKA;
vmCvar_t	cg_menuFileParseSpam;
vmCvar_t	cg_randomTaunts;


vmCvar_t	jkcvar_cg_drawClock;

// V24 Enhanced Features - Wallhack CVars
vmCvar_t cg_wallhack;
vmCvar_t cg_wallhackStyle;
vmCvar_t cg_wallhackAlpha;
vmCvar_t cg_wallhackColor;
vmCvar_t cg_wallhackRange;
vmCvar_t cg_wallhackIgnoreFriends;
vmCvar_t cg_wallhackSoundAlert;
vmCvar_t cg_wallhackVisualAlert;
vmCvar_t cg_wallhackPulse;

// V24 Enhanced Features - ESP System CVars
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

// V24/Auto/Friends/Saber/ESP Feature CVars (from feature modules)
vmCvar_t cg_wallhack;
vmCvar_t cg_autoKick;
vmCvar_t cg_autoKick_debug;
vmCvar_t cg_autoKick_sideKickFirst;
vmCvar_t cg_autoKick_distance;
vmCvar_t cg_autoKick_usePrediction;
vmCvar_t cg_autoKick_indicator;
vmCvar_t cg_autoKick_checkRoll;
vmCvar_t cg_autoKick_checkAir;
vmCvar_t cg_autoKick_checkKnockdown;
vmCvar_t cg_autoBackstab;
vmCvar_t cg_autoBackstab_debug;
vmCvar_t cg_autoBackstab_distance;
vmCvar_t cg_autoBackstab_usePrediction;
vmCvar_t cg_debugSaberBox;
vmCvar_t cg_debugSaberBox_usePrediction;
vmCvar_t cg_friendsChatsOnly;


vmCvar_t cg_autoAim;
vmCvar_t cg_autoAim_debug;
vmCvar_t cg_autoAim_usePrediction;
vmCvar_t cg_autoAim_ignoreWalls;
vmCvar_t cg_autoAimDistance;
vmCvar_t cg_autoAimAngle;



typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

=======
static cvarTable_t* systemInfoCvars = NULL;
static cvarTable_t cvarTable[] = {
	// bk001129
	{&cg_ignore, "cg_ignore", "0", 0}, // used for debugging
	{&cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_zoomFov, "cg_zoomfov", "30.0", CVAR_ARCHIVE},
	{&cg_fov, "cg_fov", "90", CVAR_ARCHIVE},
	{&cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE},
	{&cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE},
	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{ &cg_drawCenterAlways, "cg_drawCenterAlways", "0", CVAR_ARCHIVE  },
	{ &cg_drawStrafeHelperSpeedometerAlways, "cg_drawStrafeHelperSpeedometerAlways", "0", CVAR_ARCHIVE  },
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{ &cg_drawAntiLoopIndicator, "cg_drawAntiLoopIndicator", "1", CVAR_ARCHIVE  },
	{ &cg_antiLoopIndicatorX, "cg_antiLoopIndicatorX", "300", CVAR_ARCHIVE  },
	{ &cg_antiLoopIndicatorY, "cg_antiLoopIndicatorY", "400", CVAR_ARCHIVE  },
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{ &cg_drawRamps, "cg_drawRamps", "0", CVAR_ARCHIVE  },
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{ &cg_drawFPSShorterCheckSim, "cg_drawFPSShorterCheckSim", "0", CVAR_TEMP  },
	{ &cg_drawFPSMisses, "cg_drawFPSMisses", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPSSamples, "cg_drawFPSSamples", "16", CVAR_ARCHIVE  },
	{ &cg_drawFPSPhysical, "cg_drawFPSPhysical", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPSLowest, "cg_drawFPSLowest", "1", CVAR_ARCHIVE  },
	{ &cg_drawStrafeBotFactor, "cg_drawStrafeBotFactor", "1", CVAR_ARCHIVE  },
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_draw3dIcons, "cg_draw3dIcons", "0", CVAR_ARCHIVE},
	{&cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE},
	{&cg_drawAmmoWarning, "cg_drawAmmoWarning", "0", CVAR_ARCHIVE},
	{&cg_drawEnemyInfo, "cg_drawEnemyInfo", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE},
	{ &cg_strafebotFactor, "cg_strafebotFactor", "1.0", CVAR_ARCHIVE  },
	{ &cg_leadSounds, "cg_leadSounds", "1", CVAR_ARCHIVE  },
	{ &cg_leadSoundsRace, "cg_leadSoundsRace", "1", CVAR_ARCHIVE  },
	{&cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE},
	{ &cg_drawCrosshairNamesDetails, "cg_drawCrosshairNamesDetails", "1", CVAR_ARCHIVE },
	{&cg_drawScores, "cg_drawScores", "1", CVAR_ARCHIVE},
	{&cg_dynamicCrosshair, "cg_dynamicCrosshair", "1", CVAR_ARCHIVE},
	{&cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE},
	{&cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE},
	{&cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE},
	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE},
	{&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
	{&cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE},
	{&cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE},
	{&cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE},
	{&cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE},
=======
	{ &cg_teleportDisable, "cg_teleportDisable", "0", CVAR_ARCHIVE },
	{ &cg_wallhack, "cg_wallhack", "0", CVAR_TEMP },
	{&cg_centertime, "cg_centertime", "3", CVAR_CHEAT},
	{&cg_runpitch, "cg_runpitch", "0", CVAR_ARCHIVE},
	{&cg_runroll, "cg_runroll", "0", CVAR_ARCHIVE},
	{&cg_bobup, "cg_bobup", "0", CVAR_ARCHIVE},
	{&cg_bobpitch, "cg_bobpitch", "0", CVAR_ARCHIVE},
	{&cg_bobroll, "cg_bobroll", "0", CVAR_ARCHIVE},
	//{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
	{&cg_debugSaber, "cg_debugsaber", "0", CVAR_CHEAT},
	{&cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},
	{&cg_nopredict, "cg_nopredict", "0", 0},
	{&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
	{&cg_showmiss, "cg_showmiss", "0", 0},
	{&cg_footsteps, "cg_footsteps", "1", CVAR_TEMP},
	{&cg_tracerChance, "cg_tracerchance", "0.4", CVAR_TEMP},
	{&cg_tracerWidth, "cg_tracerwidth", "1", CVAR_TEMP},
	{&cg_tracerLength, "cg_tracerlength", "100", CVAR_TEMP},
	// Auto Feature CVars (restored from cg_auto.c)
	{&cg_autoBackstab, "cg_autoBackstab", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstabDistance, "cg_autoBackstabDistance", "128.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstabIgnoreFriends, "cg_autoBackstabIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstabDelay, "cg_autoBackstabDelay", "500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstabSoundAlert, "cg_autoBackstabSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoDefense, "cg_autoDefense", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick, "cg_autoKick", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKickDistance, "cg_autoKickDistance", "16.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKickIgnoreFriends, "cg_autoKickIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKickDelay, "cg_autoKickDelay", "500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKickSoundAlert, "cg_autoKickSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAim, "cg_autoAim", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAimDistance, "cg_autoAimDistance", "128.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAimAngle, "cg_autoAimAngle", "30.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAimIgnoreFriends, "cg_autoAimIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAimDelay, "cg_autoAimDelay", "500", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAimSoundAlert, "cg_autoAimSoundAlert", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espDebug, "cg_espDebug", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugSaber, "cg_debugsaber", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_TEMP },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_TEMP },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_TEMP },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_TEMP },

	// snaphud start
	{&cg_snapHud, "cg_snapHud", "0", CVAR_ARCHIVE},
	{&cg_snapHudRgba1, "cg_snapHudRgba1", "1 0 0 0.5", CVAR_ARCHIVE},
	{&cg_snapHudRgba2, "cg_snapHudRgba2", "0 1 1 0.5", CVAR_ARCHIVE},
	{&cg_snapHudY, "cg_snapHudY", "248", CVAR_ARCHIVE},
	{&cg_snapHudHeight, "cg_snapHudHeight", "8", CVAR_ARCHIVE},
	{&cg_snapHudAuto, "cg_snapHudAuto", "1", CVAR_ARCHIVE},
	{&cg_snapHudDef, "cg_snapHudDef", "45", CVAR_ARCHIVE},
	{&cg_snapHudSpeed, "cg_snapHudSpeed", "0", CVAR_ARCHIVE},
	{&cg_snapHudFps, "cg_snapHudFps", "0", CVAR_ARCHIVE},
	// snaphud end

	{&cg_autoFollow, "cg_autoFollow", "0", CVAR_ARCHIVE},
	{&cg_autoFollowUnfollowAFKDelay, "cg_autoFollowUnfollowAFKDelay", "300", CVAR_ARCHIVE},
	{&cg_autoFollowUnfollowAFKReDelay, "cg_autoFollowUnfollowAFKReDelay", "10", CVAR_ARCHIVE},
	{&cg_autoFollowUnfollowAFKSwitchBackDelay, "cg_autoFollowUnfollowAFKSwitchBackDelay", "600", CVAR_ARCHIVE},
	{&cg_autoFollowManualInterruptDuration, "cg_autoFollowManualInterruptDuration", "30", CVAR_ARCHIVE},

	{&cg_scoreboardDisconnectedPlayersDrawTime, "cg_scoreboardDisconnectedPlayersDrawTime", "30", CVAR_ARCHIVE},
	{&cg_autoScoreboardFetchInterval, "cg_autoScoreboardFetchInterval", "20", CVAR_ARCHIVE},

	// snaphud start
	{&cg_snapHud, "cg_snapHud", "0", CVAR_ARCHIVE},
	{&cg_snapHudRgba1, "cg_snapHudRgba1", "0.5 0.7 0.9 0.7", CVAR_ARCHIVE},
	{&cg_snapHudRgba2, "cg_snapHudRgba2", "0.05 0.05 0.05 0.15", CVAR_ARCHIVE},
	{&cg_snapHudY, "cg_snapHudY", "248", CVAR_ARCHIVE},
	{&cg_snapHudHeight, "cg_snapHudHeight", "8", CVAR_ARCHIVE},
	{&cg_snapHudAuto, "cg_snapHudAuto", "1", CVAR_ARCHIVE},
	{&cg_snapHudDef, "cg_snapHudDef", "45", CVAR_ARCHIVE},
	{&cg_snapHudSpeed, "cg_snapHudSpeed", "0", CVAR_ARCHIVE},
	{&cg_snapHudFps, "cg_snapHudFps", "0", CVAR_ARCHIVE},
	// snaphud end

	// snaphud start
	{&cg_snapHud, "cg_snapHud", "0", CVAR_ARCHIVE},
	{&cg_snapHudRgba1, "cg_snapHudRgba1", "0.5 0.7 0.9 0.7", CVAR_ARCHIVE},
	{&cg_snapHudRgba2, "cg_snapHudRgba2", "0.05 0.05 0.05 0.15", CVAR_ARCHIVE},
	{&cg_snapHudY, "cg_snapHudY", "248", CVAR_ARCHIVE},
	{&cg_snapHudHeight, "cg_snapHudHeight", "8", CVAR_ARCHIVE},
	{&cg_snapHudAuto, "cg_snapHudAuto", "1", CVAR_ARCHIVE},
	{&cg_snapHudDef, "cg_snapHudDef", "45", CVAR_ARCHIVE},
	{&cg_snapHudSpeed, "cg_snapHudSpeed", "0", CVAR_ARCHIVE},
	{&cg_snapHudFps, "cg_snapHudFps", "0", CVAR_ARCHIVE},
	// snaphud end
	{&cg_swingAngles, "cg_swingAngles", "1", 0},

	{&cg_oldPainSounds, "cg_oldPainSounds", "0", 0},
#ifdef G2_COLLISION_ENABLED
	{ &cg_saberModelTraceEffect, "cg_saberModelTraceEffect", "0", CVAR_ARCHIVE  },
	{&cg_saberModelTraceEffect, "cg_saberModelTraceEffect", "0", 0},
#endif

	{&cg_fpls, "cg_fpls", "0", 0},

	{&cg_saberDynamicMarks, "cg_saberDynamicMarks", "0", 0},
	{&cg_saberDynamicMarkTime, "cg_saberDynamicMarkTime", "60000", 0},

	{ &cg_saberContact, "cg_saberContact", "1", 0 },
	{ &cg_saberTrail, "cg_saberTrail", "1", 0 },
	{ &cg_saberEndsGlow, "cg_saberEndsGlow", "0.0", CVAR_ARCHIVE },
	{&cg_saberContact, "cg_saberContact", "1", 0},
	{&cg_saberTrail, "cg_saberTrail", "1", 0},

	{&cg_duelHeadAngles, "cg_duelHeadAngles", "0", 0},

=======
	{ &cg_speedTrail, "cg_speedTrail", "1", 0 },
	{ &cg_speedTrailSP, "cg_speedTrailSP", "0", CVAR_ARCHIVE },
	{ &cg_auraShell, "cg_auraShell", "1", 0 },
	{&cg_speedTrail, "cg_speedTrail", "1", 0},
	{&cg_auraShell, "cg_auraShell", "1", 0},

	{&cg_animBlend, "cg_animBlend", "1", 0},

	{ &cg_dismember, "cg_dismember", "2", CVAR_ARCHIVE },
	{&cg_dismember, "cg_dismember", "0", CVAR_ARCHIVE},

	// jk2pro Client Cvars start
	{&cjp_client, "cjp_client", "1.4JAPRO", CVAR_USERINFO | CVAR_ROM},
	{&cg_raceTimer, "cg_raceTimer", "3", 0},
	{&cg_raceTimerSize, "cg_raceTimerSize", "0.75", 0},
	{&cg_raceTimerX, "cg_raceTimerX", "5", 0},
	{&cg_raceTimerY, "cg_raceTimerY", "280", 0},
	{ &cg_raceTimerNoSpeeds, "cg_raceTimerNoSpeeds", "0", CVAR_ARCHIVE },
	{&cg_raceTimerX, "cg_raceTimerX", "5", 0},
	{&cg_raceTimerY, "cg_raceTimerY", "280", 0},
	{ &cg_customizeRace, "cg_customizeRace", "0", CVAR_ARCHIVE | CVAR_USERINFO }, // hide various things and stuff
	{&cg_speedometer, "cg_speedometer", "0", CVAR_ARCHIVE},
	{&cg_speedometerX, "cg_speedometerX", "98", CVAR_ARCHIVE},
	{&cg_speedometerY, "cg_speedometerY", "460", CVAR_ARCHIVE},
	{&cg_speedometerSize, "cg_speedometerSize", "0.75", CVAR_ARCHIVE},
	{&cg_showpos, "cg_showpos", "0", 0},
	{ &cg_forcemeter, "cg_forcemeter", "0", CVAR_ARCHIVE },
	{ &cg_forcemeterX, "cg_forcemeterX", "300", CVAR_ARCHIVE },
	{ &cg_forcemeterY, "cg_forcemeterY", "240", CVAR_ARCHIVE },
	{ &cg_forceMeterJumpCharge, "cg_forceMeterJumpCharge", "1", CVAR_ARCHIVE },
	{ &cg_forceFieldOpacity, "cg_forceFieldOpacity", "1.0", CVAR_ARCHIVE },
	{ &cg_forceFieldOpacityRace, "cg_forceFieldOpacityRace", "0.5", CVAR_ARCHIVE },

	{&cg_strafeHelperCutoff, "cg_strafeHelperCutoff", "240", CVAR_ARCHIVE},
	{&cg_strafeHelper, "cg_strafeHelper", "992", CVAR_ARCHIVE},
	{&cg_strafeHelperPrecision, "cg_strafeHelperPrecision", "256", 0},
	{&cg_strafeHelperLineWidth, "cg_strafeHelperLineWidth", "1", CVAR_ARCHIVE},
	{&cg_strafeHelperActiveColor, "cg_strafeHelperActiveColor", "0 255 0 200", CVAR_ARCHIVE},
	{&cg_strafeHelperInactiveAlpha, "cg_strafeHelperInactiveAlpha", "200", CVAR_ARCHIVE},

	{&cg_strafeHelperOffset, "cg_strafeHelperOffset", "75", CVAR_ARCHIVE},
	{&cg_strafeHelper_FPS, "cg_strafeHelper_FPS", "0", 0},

	{&cg_crosshairSizeScale, "cg_crosshairSizeScale", "1", CVAR_ARCHIVE},
	{&cg_crosshairSaberStyleColor, "cg_crosshairSaberStyleColor", "0", CVAR_ARCHIVE},
	{&cg_crosshairColor, "cg_crosshairColor", "0 0 0 255", CVAR_ARCHIVE},
	{&cg_crosshairIdentifyTarget, "cg_crosshairIdentifyTarget", "1", CVAR_ARCHIVE},

	{&cg_enhancedFlagStatus, "cg_enhancedFlagStatus", "2", CVAR_ARCHIVE},
	{&cg_drawTimerMsec, "cg_drawTimerMsec", "1", CVAR_ARCHIVE},
	{&cg_movementKeys, "cg_movementKeys", "0", CVAR_ARCHIVE},
	{&cg_movementKeysX, "cg_movementKeysX", "148", CVAR_ARCHIVE},
	{&cg_movementKeysY, "cg_movementKeysY", "428", CVAR_ARCHIVE},
	{&cg_movementKeysSize, "cg_movementKeysSize", "1.0", CVAR_ARCHIVE},

	// only for you, arto
	{&cg_hudColors, "cg_hudColors", "1", CVAR_ARCHIVE},
	{&cg_drawScore, "cg_drawScore", "2", CVAR_ARCHIVE},
	{&cg_centerHeight, "cg_centerHeight", "0", CVAR_ARCHIVE},
	{&cg_centerSize, "cg_centerSize", "1.0", CVAR_ARCHIVE},
	{&cg_crosshairSizeScale, "cg_crosshairSizeScale", "1", CVAR_ARCHIVE},
	{&cg_crosshairSaberStyleColor, "cg_crosshairSaberStyleColor", "0", CVAR_ARCHIVE},
	{&cg_crosshairColor, "cg_crosshairColor", "0 0 0 255", CVAR_ARCHIVE},
	{&cg_crosshairIdentifyTarget, "cg_crosshairIdentifyTarget", "1", CVAR_ARCHIVE},

	// chatbox
	{&cg_chatBox, "cg_chatBox", "10000", CVAR_ARCHIVE},
	{&cg_chatBoxFontSize, "cg_chatBoxFontSize", "1.0", CVAR_ARCHIVE},
	{&cg_chatBoxHeight, "cg_chatBoxHeight", "360", CVAR_ARCHIVE},
	// japro chatbox stuff
	{&cg_chatBoxShowHistory, "cg_chatBoxShowHistory", "1", CVAR_ARCHIVE},
	{&cg_chatBoxX, "cg_chatBoxX", "16", CVAR_ARCHIVE},
	{&cg_chatBoxCutOffLength, "cg_chatBoxCutOffLength", "375", CVAR_ARCHIVE},
	{&cg_chatSounds, "cg_chatSounds", "1", CVAR_ARCHIVE},
	{&cg_cleanChatbox, "cg_cleanChatbox", "0", 0},
	{&cg_newFont, "cg_newFont", "0", CVAR_ARCHIVE},

	{&cg_remaps, "cg_remaps", "1", CVAR_LATCH | CVAR_TEMP},
	{&cg_autoKillWhenFalling, "cg_autoKillWhenFalling", "0", CVAR_ARCHIVE},

	{&cg_jumpSounds, "cg_jumpSounds", "1", CVAR_ARCHIVE},
	{&cg_rollSounds, "cg_rollSounds", "1", CVAR_ARCHIVE},
	{&cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE},
	{&cg_newSaberHitSounds, "cg_newSaberHitSounds", "0", CVAR_ARCHIVE},
	{&cg_thirdPersonFlagAlpha, "cg_thirdPersonFlagAlpha", "1.0", CVAR_ARCHIVE},
	{&cg_drawNonDuelers, "cg_drawNonDuelers", "0", 0},
	{&cg_brightskins, "cg_brightskins", "0", CVAR_ARCHIVE},
	{&cg_drawHitBox, "cg_drawHitBox", "0", CVAR_TEMP},
	{&cg_playerLOD, "cg_playerLOD", "0", CVAR_ARCHIVE},
	{&cg_privateDuelShell, "cg_privateDuelShell", "1", CVAR_ARCHIVE},
	{&cg_teamRespawnShield, "cg_teamRespawnShield", "1", CVAR_ARCHIVE},
	{&cg_saberTeamColors, "cg_saberTeamColors", "1", 0},

	{&cg_widescreen, "cg_widescreen", "1", CVAR_ARCHIVE},
	{&cg_fovAspectAdjust, "cg_fovAspectAdjust", "1", CVAR_ARCHIVE},
	//only for you, arto
	{ &cg_hudColors, "cg_hudColors", "1", CVAR_ARCHIVE },
	{ &cg_drawScore, "cg_drawScore", "2", CVAR_ARCHIVE },
	{ &cg_drawScoreDefrag, "cg_drawScoreDefrag", "0", CVAR_ARCHIVE }, // Interpret score as seconds into a run
	{ &cg_centerHeight, "cg_centerHeight", "0", CVAR_ARCHIVE },
	{ &cg_centerSize, "cg_centerSize", "1.0", CVAR_ARCHIVE },
	// only for you, arto
	{&cg_hudColors, "cg_hudColors", "1", CVAR_ARCHIVE},
	{&cg_drawScore, "cg_drawScore", "2", CVAR_ARCHIVE},
	{&cg_centerHeight, "cg_centerHeight", "0", CVAR_ARCHIVE},
	{&cg_centerSize, "cg_centerSize", "1.0", CVAR_ARCHIVE},

	// chatbox
	{&cg_chatBox, "cg_chatBox", "10000", CVAR_ARCHIVE},
	{&cg_chatBoxFontSize, "cg_chatBoxFontSize", "1.0", CVAR_ARCHIVE},
	{&cg_chatBoxHeight, "cg_chatBoxHeight", "360", CVAR_ARCHIVE},
	// japro chatbox stuff
	{&cg_chatBoxShowHistory, "cg_chatBoxShowHistory", "1", CVAR_ARCHIVE},
	{&cg_chatBoxX, "cg_chatBoxX", "16", CVAR_ARCHIVE},
	{&cg_chatBoxCutOffLength, "cg_chatBoxCutOffLength", "375", CVAR_ARCHIVE},
	{&cg_chatSounds, "cg_chatSounds", "1", CVAR_ARCHIVE},
	{&cg_cleanChatbox, "cg_cleanChatbox", "0", 0},
	{&cg_newFont, "cg_newFont", "0", CVAR_ARCHIVE},

	{&cg_remaps, "cg_remaps", "1", CVAR_LATCH | CVAR_TEMP},
	{&cg_autoKillWhenFalling, "cg_autoKillWhenFalling", "0", CVAR_ARCHIVE},

	{&cg_fovViewmodel, "cg_fovViewmodel", "80", CVAR_ARCHIVE},
	{&cg_fovViewmodelAdjust, "cg_fovViewmodelAdjust", "1", CVAR_ARCHIVE},

	{&cg_fkDuration, "cg_fkDuration", "50", 0},
	{&cg_fkFirstJumpDuration, "cg_fkFirstJumpDuration", "0", 0},
	{&cg_fkSecondJumpDelay, "cg_fkSecondJumpDelay", "0", 0},

	{&cl_commandsize, "cl_commandsize", "64", CVAR_ARCHIVE}, // Loda - FPS UNLOCK client modcode

	{&cg_fixlean, "cg_fixlean", "0", CVAR_LATCH}, // idk man
	{&cg_SPRunAnim, "cg_SPRunAnim", "0", 0},

	{&cg_drawInventory, "cg_drawInventory", "1", CVAR_ARCHIVE},
	{&cg_smallScoreboard, "cg_smallScoreboard", "0", CVAR_ARCHIVE},
	{&cg_colorScoreboard, "cg_colorScoreboard", "0", CVAR_ARCHIVE},
	{&cg_drawScoreboardIcons, "cg_drawScoreboardIcons", "0", CVAR_ARCHIVE},
	{&cg_drawPowerUpIcons, "cg_drawPowerUpIcons", "1", CVAR_ARCHIVE},
	{&cg_drawDemoName, "cg_drawDemoName", "1", 0},
	{&cg_lowhpsound, "cg_lowhpsound", "35", CVAR_ARCHIVE},
	{&cg_backSwingCameraRange, "cg_backSwingCameraRange", "0", CVAR_ARCHIVE},
	// jk2pro stuff end
	{&cg_widescreen, "cg_widescreen", "1", CVAR_ARCHIVE},
	{&cg_fovAspectAdjust, "cg_fovAspectAdjust", "1", CVAR_ARCHIVE},
	{ &cg_cameraFPS, "cg_cameraFPS", "125", CVAR_ARCHIVE },

	{&cg_fovViewmodel, "cg_fovViewmodel", "80", CVAR_ARCHIVE},
	{&cg_fovViewmodelAdjust, "cg_fovViewmodelAdjust", "1", CVAR_ARCHIVE},

	{&cg_fkDuration, "cg_fkDuration", "50", 0},
	{&cg_fkFirstJumpDuration, "cg_fkFirstJumpDuration", "0", 0},
	{&cg_fkSecondJumpDelay, "cg_fkSecondJumpDelay", "0", 0},

	{&cl_commandsize, "cl_commandsize", "0", 0}, // Loda - FPS UNLOCK client modcode

	{&cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPersonPitchOffset, "cg_thirdPersonPitchOffset", "0", CVAR_CHEAT},
	{&cg_thirdPersonVertOffset, "cg_thirdPersonVertOffset", "16", CVAR_ARCHIVE},
	{&cg_thirdPersonCameraDamp, "cg_thirdPersonCameraDamp", "0.3", CVAR_ARCHIVE},
	{&cg_thirdPersonTargetDamp, "cg_thirdPersonTargetDamp", "0.5", CVAR_ARCHIVE},

	{&cg_thirdPersonHorzOffset, "cg_thirdPersonHorzOffset", "0", CVAR_CHEAT},
	{&cg_thirdPersonAlpha, "cg_thirdPersonAlpha", "1.0", CVAR_CHEAT},

	{&cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE},
	{&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
	{&cg_drawInventory, "cg_drawInventory", "1", CVAR_ARCHIVE},
	{&cg_smallScoreboard, "cg_smallScoreboard", "0", CVAR_ARCHIVE},
	{&cg_colorScoreboard, "cg_colorScoreboard", "0", CVAR_ARCHIVE},
	{&cg_drawScoreboardIcons, "cg_drawScoreboardIcons", "0", CVAR_ARCHIVE},
	{&cg_drawPowerUpIcons, "cg_drawPowerUpIcons", "1", CVAR_ARCHIVE},
	{&cg_drawDemoName, "cg_drawDemoName", "1", 0},
	{&cg_lowhpsound, "cg_lowhpsound", "35", CVAR_ARCHIVE},
	{&cg_backSwingCameraRange, "cg_backSwingCameraRange", "0", CVAR_ARCHIVE},
	// jk2pro stuff end

	{&cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPersonPitchOffset, "cg_thirdPersonPitchOffset", "0", CVAR_CHEAT},
	{&cg_thirdPersonVertOffset, "cg_thirdPersonVertOffset", "16", CVAR_ARCHIVE},
	{&cg_thirdPersonCameraDamp, "cg_thirdPersonCameraDamp", "0.3", CVAR_ARCHIVE},
	{&cg_thirdPersonTargetDamp, "cg_thirdPersonTargetDamp", "0.5", CVAR_ARCHIVE},

	{&cg_thirdPersonHorzOffset, "cg_thirdPersonHorzOffset", "0", CVAR_CHEAT},
	{&cg_thirdPersonAlpha, "cg_thirdPersonAlpha", "1.0", CVAR_CHEAT},

	{&cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE},
	{&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
	{ &cg_forceMyModel, "cg_forceMyModel", "", CVAR_ARCHIVE  },
	{ &cg_forceMySaber, "cg_forceMySaber", "", CVAR_ARCHIVE },
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
	//{ &cg_optimizedPredict, "cg_optimizedPredict", "0", CVAR_ARCHIVE },
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
	{ &cg_deferPlayersDebug, "cg_deferPlayersDebug", "0", CVAR_TEMP },
	{&cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE},
	{&cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO},
	{&cg_stats, "cg_stats", "0", 0},
	{&cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE},
	{&cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE},
	{&cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE},
	{&cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE},
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{&cg_buildScript, "com_buildScript", "0", 0}, // force loading of all possible data amd error on failures
	{&cg_paused, "cl_paused", "0", CVAR_ROM},
	{&cg_blood, "com_blood", "1", CVAR_ARCHIVE},
	{&cg_synchronousClients, "g_synchronousClients", "0", 0}, // communicated by systeminfo
	{ &cg_mapDefaultMsec, "g_mapDefaultMsec", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_mapDefaultJump, "g_mapDefaultJump", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_strafebotSlopeHandling, "g_strafebotSlopeHandling", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_mapDefaultRunFlags, "g_mapDefaultRunFlags", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_q2trace, "g_q2trace", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_q2Skims, "g_q2Skims", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{ &cg_g_unlockRandom, "g_unlockRandom", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo
	{&cg_synchronousClients, "g_synchronousClients", "0", 0}, // communicated by systeminfo
	{ &cg_cl_timeNudgeAntiLagHack, "cl_timeNudgeAntiLagHack", "0", 0 },
	{ &cg_cl_timeNudgeSafeServerTime, "cl_timeNudgeSafeServerTime", "0", 0 },

	{&cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{&cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{&cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_enableDust, "g_enableDust", "0", 0},
	{&cg_enableBreath, "g_enableBreath", "0", 0},
	{&cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{&cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},

	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_hudFiles, "cg_hudFiles", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&cg_pmove_fixed, "pmove_fixed", "0", CVAR_SERVERINFO},
	{&cg_pmove_msec, "pmove_msec", "8", CVAR_SERVERINFO},
	{&cg_pmove_float, "pmove_float", "0", CVAR_SERVERINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{&cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE},

	{&cg_ui_myteam, "ui_myteam", "0", CVAR_ROM | CVAR_INTERNAL},
	{&cg_com_maxfps, "com_maxfps", "", 0},

	{&cg_developer, "cg_developer", "0", CVAR_TEMP},
	{&cg_mv_fixbrokenmodelsclient, "mv_fixbrokenmodelsclient", "2", CVAR_ARCHIVE},
	{&cg_drawPlayerSprites, "cg_drawPlayerSprites", "3", CVAR_ARCHIVE},
	{&cg_smoothCamera, "cg_smoothCamera", "1", CVAR_ARCHIVE},
	{&cg_smoothCameraFPS, "cg_smoothCameraFPS", "0", CVAR_ARCHIVE},

	{&cg_MVSDK, "cg_MVSDK", MVSDK_VERSION, CVAR_ROM | CVAR_USERINFO},

	//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
	/*
	Ghoul2 Insert Start
	*/
	{&cg_debugBB, "debugBB", "0", 0},
	/*
	Ghoul2 Insert End
	*/
	{&cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{&cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{&cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_enableDust, "g_enableDust", "0", 0},
	{&cg_enableBreath, "g_enableBreath", "0", 0},
	{&cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{&cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},

	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_hudFiles, "cg_hudFiles", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&cg_pmove_fixed, "pmove_fixed", "0", CVAR_SERVERINFO},
	{&cg_pmove_msec, "pmove_msec", "8", CVAR_SERVERINFO},
	{&cg_pmove_float, "pmove_float", "0", CVAR_SERVERINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{&cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE},

	{&cg_ui_myteam, "ui_myteam", "0", CVAR_ROM | CVAR_INTERNAL},
	{&cg_com_maxfps, "com_maxfps", "", 0},

	{&cg_developer, "cg_developer", "0", CVAR_TEMP},
	{&cg_mv_fixbrokenmodelsclient, "mv_fixbrokenmodelsclient", "2", CVAR_ARCHIVE},
	{&cg_drawPlayerSprites, "cg_drawPlayerSprites", "3", CVAR_ARCHIVE},
	{&cg_smoothCamera, "cg_smoothCamera", "1", CVAR_ARCHIVE},
	{&cg_smoothCameraFPS, "cg_smoothCameraFPS", "0", CVAR_ARCHIVE},

	{&cg_MVSDK, "cg_MVSDK", MVSDK_VERSION, CVAR_ROM | CVAR_USERINFO},
	//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
	/*
	Ghoul2 Insert Start
	*/
	{&cg_debugBB, "debugBB", "0", 0},
	/*
	Ghoul2 Insert End
	*/
	{&cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO},
	{&cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{&cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{&cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_enableDust, "g_enableDust", "0", 0},
	{&cg_enableBreath, "g_enableBreath", "0", 0},
	{&cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{&cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{&cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},

	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_hudFiles, "cg_hudFiles", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&cg_pmove_fixed, "pmove_fixed", "0", CVAR_SERVERINFO},
	{&cg_pmove_msec, "pmove_msec", "8", CVAR_SERVERINFO},
	{&cg_pmove_float, "pmove_float", "0", CVAR_SERVERINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{&cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE},

	{&cg_ui_myteam, "ui_myteam", "0", CVAR_ROM | CVAR_INTERNAL},
	{&cg_com_maxfps, "com_maxfps", "", 0},

	{&cg_developer, "cg_developer", "0", CVAR_TEMP},
	{&cg_mv_fixbrokenmodelsclient, "mv_fixbrokenmodelsclient", "2", CVAR_ARCHIVE},
	{&cg_drawPlayerSprites, "cg_drawPlayerSprites", "3", CVAR_ARCHIVE},
	{&cg_smoothCamera, "cg_smoothCamera", "1", CVAR_ARCHIVE},
	{&cg_smoothCameraFPS, "cg_smoothCameraFPS", "0", CVAR_ARCHIVE},

	{&cg_MVSDK, "cg_MVSDK", MVSDK_VERSION, CVAR_ROM | CVAR_USERINFO},
	{&cg_drawKillMessage, "cg_drawKillMessage", "1", CVAR_ARCHIVE},
	{&cg_showKills, "cg_showKills", "0", CVAR_ARCHIVE},
	{&cg_char_color_red, "char_color_red", "255", CVAR_ARCHIVE},
	{&cg_char_color_green, "char_color_green", "255", CVAR_ARCHIVE},
	{&cg_char_color_blue, "char_color_blue", "255", CVAR_ARCHIVE},

	{&cg_autoKick, "cg_autoKick", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_debug, "cg_autoKick_debug", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_sideKickFirst, "cg_autoKick_sideKickFirst", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_distance, "cg_autoKick_distance", "16.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_usePrediction, "cg_autoKick_usePrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_indicator, "cg_autoKick_indicator", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_checkRoll, "cg_autoKick_checkRoll", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_checkAir, "cg_autoKick_checkAir", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoKick_checkKnockdown, "cg_autoKick_checkKnockdown", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstab, "cg_autoBackstab", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstab_debug, "cg_autoBackstab_debug", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstab_distance, "cg_autoBackstab_distance", "128.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoBackstab_usePrediction, "cg_autoBackstab_usePrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_debugSaberBox, "cg_debugSaberBox", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_debugSaberBox_usePrediction, "cg_debugSaberBox_usePrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_friendsChatsOnly, "cg_friendsChatsOnly", "0", CVAR_ARCHIVE | CVAR_GLOBAL},
	{&cg_autoAim, "cg_autoAim", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAim_debug, "cg_autoAim_debug", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAim_usePrediction, "cg_autoAim_usePrediction", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_autoAim_ignoreWalls, "cg_autoAim_ignoreWalls", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	// V24 Enhanced Features - Wallhack CVars
	{&cg_wallhack, "cg_wallhack", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackStyle, "cg_wallhackStyle", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackAlpha, "cg_wallhackAlpha", "1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackColor, "cg_wallhackColor", "1 1 1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackRange, "cg_wallhackRange", "2048", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackIgnoreFriends, "cg_wallhackIgnoreFriends", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackSoundAlert, "cg_wallhackSoundAlert", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackVisualAlert, "cg_wallhackVisualAlert", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_wallhackPulse, "cg_wallhackPulse", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},

	// V24 Enhanced Features - ESP System CVars
	{&cg_esp, "cg_esp", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espPlayers, "cg_espPlayers", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espItems, "cg_espItems", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espDistance, "cg_espDistance", "2048", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espThroughWalls, "cg_espThroughWalls", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espStyle, "cg_espStyle", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espAlpha, "cg_espAlpha", "1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espSize, "cg_espSize", "1.0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espPlayerNames, "cg_espPlayerNames", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espItemNames, "cg_espItemNames", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espHealthBars, "cg_espHealthBars", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espForceBars, "cg_espForceBars", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espWeaponInfo, "cg_espWeaponInfo", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espBoxes, "cg_espBoxes", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espLines, "cg_espLines", "1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espColorMode, "cg_espColorMode", "0", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
	{&cg_espPlayerColor, "cg_espPlayerColor", "1 1 1", CVAR_ARCHIVE | CVAR_CHEAT | CVAR_GLOBAL},
};

#define cvarTableSize (sizeof( cvarTable ) / sizeof( cvarTable[0] ))

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;

	widescreenModificationCount = cg_widescreen.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	//trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	//trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "forcepowers", DEFAULT_FORCEPOWERS, CVAR_USERINFO | CVAR_ARCHIVE );

	// Cvars uses for transferring data between client and server
	trap_Cvar_Register(NULL, "ui_about_gametype",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_fraglimit",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_capturelimit",	"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_duellimit",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_timelimit",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_maxclients",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_dmflags",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_mapname",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_hostname",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_needpass",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register(NULL, "ui_about_botminplayers",	"0", CVAR_ROM|CVAR_INTERNAL );

	// mvsdk_cgFlags
	MV_UpdateCgFlags();
}

/*																																			
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		CG_UpdateConfigString( CS_PLAYERS + i, qfalse );
	}
}

/*
===================
CG_WideScreenMode
Make 2D drawing functions use widescreen or 640x480 coordinates
===================
*/
void CG_WideScreenMode(qboolean on) {
	if (mvapi >= 3) {
		if (on) {
			trap_MVAPI_SetVirtualScreen(cgs.screenWidth, (float)cgs.screenHeight);
		}
		else {
			trap_MVAPI_SetVirtualScreen((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
		}
	}
}


/*
===================
CG_UpdateWidescreen
===================
*/
static void CG_UpdateWidescreen(void) {
	if (cg_widescreen.integer && mvapi >= 3) {
		if ( cgs.glconfig.vidWidth >= cgs.glconfig.vidHeight ) {
			cgs.screenWidth = (float)SCREEN_HEIGHT * cgs.glconfig.vidWidth / cgs.glconfig.vidHeight;
			cgs.screenHeight = (float)SCREEN_HEIGHT;
		} else {
			cgs.screenWidth = (float)SCREEN_WIDTH;
			cgs.screenHeight = (float)SCREEN_WIDTH * cgs.glconfig.vidHeight / cgs.glconfig.vidWidth;
		}
	} else {
		cgs.screenWidth = (float)SCREEN_WIDTH;
		cgs.screenHeight = (float)SCREEN_HEIGHT;
	}

	cgs.screenXFactor = (float)SCREEN_WIDTH / cgs.screenWidth;
	cgs.screenXFactorInv = cgs.screenWidth / (float)SCREEN_WIDTH;

	cgs.screenYFactor = (float)SCREEN_HEIGHT / cgs.screenHeight;
	cgs.screenYFactorInv = cgs.screenHeight / (float)SCREEN_HEIGHT;

	cgDC.screenWidth = cgs.screenWidth;
	cgDC.screenHeight = cgs.screenHeight;

	if (mvapi >= 3)
		trap_MVAPI_SetVirtualScreen(cgs.screenWidth, cgs.screenHeight);
}

/*
===================
CG_CrosshairColorChange
===================
*/
static void CG_CrosshairColorChange(void) {
	int i;
	sscanf(cg_crosshairColor.string, "%f %f %f %f", &cg.crosshairColor[0], &cg.crosshairColor[1], &cg.crosshairColor[2], &cg.crosshairColor[3]);

	for (i = 0; i < 4; i++) {
		if (cg.crosshairColor[i] < 1)
			cg.crosshairColor[i] = 0;
		else if (cg.crosshairColor[i] > 255)
			cg.crosshairColor[i] = 255;
	}

	cg.crosshairColor[0] /= 255.0f;
	cg.crosshairColor[1] /= 255.0f;
	cg.crosshairColor[2] /= 255.0f;
	cg.crosshairColor[3] /= 255.0f;

	//Com_Printf("New color is %f, %f, %f, %f\n", cg.crosshairColor[0], cg.crosshairColor[1], cg.crosshairColor[2], cg.crosshairColor[3]);
}

/*
===================
CG_StrafeHelperActiveColorChange
===================
*/
static void CG_StrafeHelperActiveColorChange(void) {
	int i;
	if (sscanf(cg_strafeHelperActiveColor.string, "%f %f %f %f", &cg.strafeHelperActiveColor[0], &cg.strafeHelperActiveColor[1], &cg.strafeHelperActiveColor[2], &cg.strafeHelperActiveColor[3]) != 4) {
		cg.strafeHelperActiveColor[0] = 0;
		cg.strafeHelperActiveColor[1] = 255;
		cg.strafeHelperActiveColor[2] = 0;
		cg.strafeHelperActiveColor[3] = 200;
	}

	for (i = 0; i < 4; i++) {
		if (cg.strafeHelperActiveColor[i] < 0)
			cg.strafeHelperActiveColor[i] = 0;
		else if (cg.strafeHelperActiveColor[i] > 255)
			cg.strafeHelperActiveColor[i] = 255;
	}

	trap_Cvar_Set("ui_sha_r", va("%f", cg.strafeHelperActiveColor[0]));
	trap_Cvar_Set("ui_sha_g", va("%f", cg.strafeHelperActiveColor[1]));
	trap_Cvar_Set("ui_sha_b", va("%f", cg.strafeHelperActiveColor[2]));
	trap_Cvar_Set("ui_sha_a", va("%f", cg.strafeHelperActiveColor[3]));

	cg.strafeHelperActiveColor[0] /= 255.0f;
	cg.strafeHelperActiveColor[1] /= 255.0f;
	cg.strafeHelperActiveColor[2] /= 255.0f;
	cg.strafeHelperActiveColor[3] /= 255.0f;

	//Com_Printf("New color is %f, %f, %f, %f\n", cg.strafeHelperActiveColor[0], cg.strafeHelperActiveColor[1], cg.strafeHelperActiveColor[2], cg.strafeHelperActiveColor[3]);
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
		// FIXME E3 HACK
		trap_Cvar_Set( "teamoverlay", "1" );
	}

	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}

	if (widescreenModificationCount != cg_widescreen.modificationCount) {
		widescreenModificationCount = cg_widescreen.modificationCount;
		CG_UpdateWidescreen();
	}

	if (crosshairColorModificationCount != cg_crosshairColor.modificationCount) {
		crosshairColorModificationCount = cg_crosshairColor.modificationCount;
		CG_CrosshairColorChange();
	}

	if (strafeHelperActiveColorModificationCount != cg_strafeHelperActiveColor.modificationCount) {
		strafeHelperActiveColorModificationCount = cg_strafeHelperActiveColor.modificationCount;
		CG_StrafeHelperActiveColorChange();
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		return -1;
	}

	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_DPrintf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	if (cg_developer.integer) {
		va_start (argptr, msg);
		Q_vsnprintf (text, sizeof(text), msg, argptr);
		va_end (argptr);

		trap_Print( text );
	}
}

Q_NORETURN void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

Q_NORETURN void QDECL Com_Error( errorParm_t level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);
	CG_Printf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_SendConsoleCommand
Ensures trailing newline & handles va formatting
================
*/
void QDECL CG_SendConsoleCommand(const char *fmt, ...)
{
	va_list argptr;
	char buf[MAX_STRING_CHARS];
	int len;

	if (!fmt || !fmt[0])
		return;

	va_start(argptr, fmt);
	len = Q_vsnprintf(buf, sizeof(buf), fmt, argptr);
	va_end(argptr);

	if (!buf || !buf[0])
		return;

	if (!len)
		return;
	if (buf[len - 1] != '\n') //check for trailing newline
		Q_strcat(buf, sizeof(buf), "\n"); //append one if we don't have one already

	trap_SendConsoleCommand(buf);
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound );
		}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	name[MAX_QPATH];

	// voice commands
	// rww - no "voice commands" I guess.
	//CG_LoadVoiceChats();

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM004" );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM005" );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM001" );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM002" );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM003");
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM035" );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM036" );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM037" );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM038" );

	cgs.media.redSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/red_glow" );
	cgs.media.redSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/red_line" );
	cgs.media.orangeSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/orange_glow" );
	cgs.media.orangeSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/orange_line" );
	cgs.media.yellowSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/yellow_glow" );
	cgs.media.yellowSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/yellow_line" );
	cgs.media.greenSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/green_glow" );
	cgs.media.greenSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/green_line" );
	cgs.media.blueSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/blue_glow" );
	cgs.media.blueSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/blue_line" );
	cgs.media.purpleSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/purple_glow" );
	cgs.media.purpleSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/purple_line" );
	cgs.media.saberBlurShader			= trap_R_RegisterShader( "gfx/effects/sabers/saberBlur" );

	cgs.media.yellowDroppedSaberShader	= trap_R_RegisterShader("gfx/effects/yellow_glow");

	cgs.media.rivetMarkShader			= trap_R_RegisterShader( "gfx/damage/rivetmark" );

	cgs.media.saberClashFlare			= trap_R_RegisterShader( "gfx/effects/saberFlare" );

	trap_R_RegisterShader( "powerups/ysalimarishell" );
	trap_R_RegisterShader("gfx/effects/saberDamageGlow" );
	
	trap_R_RegisterShader( "gfx/effects/forcePush" );

	cgs.media.forcefieldShader[TEAM_RED] = trap_R_RegisterShader( "gfx/misc/red_portashield" );
	cgs.media.forcefieldDmgShader[TEAM_RED] = 	trap_R_RegisterShader( "gfx/misc/red_dmgshield" );
	cgs.media.forcefieldShader[TEAM_BLUE] = trap_R_RegisterShader( "gfx/misc/blue_portashield" );
	cgs.media.forcefieldDmgShader[TEAM_BLUE] = trap_R_RegisterShader( "gfx/misc/blue_dmgshield" );
	cgs.media.forcefieldShader[TEAM_FREE] = trap_R_RegisterShader( "gfx/misc/yellow_portashield" );
	cgs.media.forcefieldDmgShader[TEAM_FREE] = trap_R_RegisterShader( "gfx/misc/yellow_dmgshield" );

	trap_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_dmg.tga" );

	for (i=1 ; i<9 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", i));
	}

	for (i=1 ; i<10 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/saberblock%i.wav", i));
	}

	for (i=1 ; i<4 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/bounce%i.wav", i));
	}

	for ( i = 0; i < 5; i++) {//JAPRO - Clientside - Use all saber hum sounds
		cgs.media.saberHumSounds[i] = trap_S_RegisterSound(va("sound/weapons/saber/saberhum%i.wav", i + 1));
		if (!cgs.media.saberHumSounds[i])
			Com_Printf("failed to register sound \"sound/weapons/saber/saberhum%i.wav\"\n", i + 1);
	}

	trap_S_RegisterSound( "sound/weapons/saber/saberon.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall1" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall2" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall3" );
	trap_S_RegisterSound("sound/weapons/saber/saberhit.wav");

	cgs.media.teamHealSound = trap_S_RegisterSound("sound/weapons/force/teamheal.wav");
	cgs.media.teamRegenSound = trap_S_RegisterSound("sound/weapons/force/teamforce.wav");

	trap_S_RegisterSound("sound/weapons/force/heal.wav");
	trap_S_RegisterSound("sound/weapons/force/speed.wav");
	trap_S_RegisterSound("sound/weapons/force/see.wav");
	trap_S_RegisterSound("sound/weapons/force/rage.wav");
	trap_S_RegisterSound("sound/weapons/force/lightning.wav");
	trap_S_RegisterSound("sound/weapons/force/lightninghit.wav");
	trap_S_RegisterSound("sound/weapons/force/drain.wav");
	trap_S_RegisterSound("sound/weapons/force/jumpbuild.wav");
	trap_S_RegisterSound("sound/weapons/force/distract.wav");
	trap_S_RegisterSound("sound/weapons/force/distractstop.wav");
	trap_S_RegisterSound("sound/weapons/force/pull.wav");
	trap_S_RegisterSound("sound/weapons/force/push.wav");

	if (cg_buildScript.integer)
	{
		trap_S_RegisterSound("sound/chars/atst/ATSTcrash.wav");
		trap_S_RegisterSound("sound/chars/atst/ATSTstart.wav");
		trap_S_RegisterSound("sound/chars/atst/ATSTstep1.wav");
		trap_S_RegisterSound("sound/chars/atst/ATSTstep2.wav");

		trap_S_RegisterSound("sound/weapons/atst/ATSTfire1.wav");
		trap_S_RegisterSound("sound/weapons/atst/ATSTfire2.wav");
		trap_S_RegisterSound("sound/weapons/atst/ATSTfire3.wav");
		trap_S_RegisterSound("sound/weapons/atst/ATSTfire4.wav");
	}

	for (i=1 ; i<3 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/thermal/bounce%i.wav", i));
	}

	trap_S_RegisterSound("sound/movers/switches/switch2.wav");
	trap_S_RegisterSound("sound/movers/switches/switch3.wav");
	trap_S_RegisterSound("sound/ambience/spark5.wav");
	trap_S_RegisterSound("sound/chars/turret/ping.wav");
	trap_S_RegisterSound("sound/chars/turret/startup.wav");
	trap_S_RegisterSound("sound/chars/turret/shutdown.wav");
	trap_S_RegisterSound("sound/chars/turret/move.wav");
	trap_S_RegisterSound("sound/player/pickuphealth.wav");
	trap_S_RegisterSound("sound/player/pickupshield.wav");

	trap_S_RegisterSound("sound/effects/glassbreak1.wav");

	trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" );
	trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" );

	trap_S_RegisterSound("sound/weapons/force/speedloop.wav");

	trap_S_RegisterSound("sound/weapons/force/protecthit.mp3"); //PDSOUND_PROTECTHIT
	trap_S_RegisterSound("sound/weapons/force/protect.mp3"); //PDSOUND_PROTECT
	trap_S_RegisterSound("sound/weapons/force/absorbhit.mp3"); //PDSOUND_ABSORBHIT
	trap_S_RegisterSound("sound/weapons/force/absorb.mp3"); //PDSOUND_ABSORB
	trap_S_RegisterSound("sound/weapons/force/jump.mp3"); //PDSOUND_FORCEJUMP
	trap_S_RegisterSound("sound/weapons/force/grip.mp3"); //PDSOUND_FORCEGRIP

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {

#ifdef JK2AWARDS
		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/chars/mothma/misc/capture.wav" );
#endif
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM046");
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM045");
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM032" );

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM044");
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM043" );

		if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM042" );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM041" );
			cgs.media.redTookFlagSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM040" );
			cgs.media.blueTookFlagSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM039" );
		}

		if ( cgs.gametype == GT_CTY || cg_buildScript.integer ) {
			cgs.media.redYsalReturnedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM050" );
			cgs.media.blueYsalReturnedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM049" );
			cgs.media.redTookYsalSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM048" );
			cgs.media.blueTookYsalSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM047" );
		}
	}

	cgs.media.drainSound = trap_S_RegisterSound("sound/weapons/force/drained.mp3");

	cgs.media.happyMusic = trap_S_RegisterSound("music/goodsmall.mp3");
	cgs.media.dramaticFailure = trap_S_RegisterSound("music/badsmall.mp3");

	//PRECACHE ALL MUSIC HERE (don't need to precache normally because it's streamed off the disk)
	if (cg_buildScript.integer)
	{
		trap_S_StartBackgroundTrack( "music/mp/duel.mp3", "music/mp/duel.mp3", qfalse );
	}

	cg.loadLCARSStage = 1;

	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav" );

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/player/telein.wav" );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/player/teleout.wav" );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav" );

	trap_S_RegisterSound( "sound/movers/objects/objectHit.wav" );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav" );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav");
	cgs.media.fallSound = trap_S_RegisterSound( "sound/player/fallsplat.wav");

	cgs.media.crackleSound = trap_S_RegisterSound( "sound/effects/energy_crackle.wav" );
#ifdef JK2AWARDS
	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM025" );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM053" );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM017" );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM019" );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM024" );
	cgs.media.assistSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM026" );
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM051");
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM032");
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM052");

	cgs.media.rollSound					= trap_S_RegisterSound( "sound/player/roll1.wav");

	cgs.media.watrInSound				= trap_S_RegisterSound( "sound/player/watr_in.wav");
	cgs.media.watrOutSound				= trap_S_RegisterSound( "sound/player/watr_out.wav");
	cgs.media.watrUnSound				= trap_S_RegisterSound( "sound/player/watr_un.wav");

	cgs.media.explosionModel			= trap_R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	cgs.media.surfaceExplosionShader	= trap_R_RegisterShader( "surfaceExplosion" );

	cgs.media.disruptorShader			= trap_R_RegisterShader( "gfx/effects/burn");

	if (cg_buildScript.integer)
	{
		trap_R_RegisterShader( "gfx/effects/turretflashdie" );
	}

	cgs.media.solidWhite = trap_R_RegisterShader( "gfx/effects/solidWhite_cull" );

	trap_R_RegisterShader("gfx/misc/mp_light_enlight_disable");
	trap_R_RegisterShader("gfx/misc/mp_dark_enlight_disable");

	trap_R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	trap_R_RegisterModel("models/items/remote.md3");

	cgs.media.holocronPickup = trap_S_RegisterSound( "sound/player/holocron.wav" );

	// Zoom
	cgs.media.zoomStart = trap_S_RegisterSound( "sound/interface/zoomstart.wav" );
	cgs.media.zoomLoop	= trap_S_RegisterSound( "sound/interface/zoomloop.wav" );
	cgs.media.zoomEnd	= trap_S_RegisterSound( "sound/interface/zoomend.wav" );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name);

		// should these always be registered??
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		trap_S_RegisterSound (name);
	}

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		CG_UpdateConfigString( CS_SOUNDS + i, qtrue );
	}

	cg.loadLCARSStage = 2;

	// FIXME: only needed with item
	cgs.media.deploySeeker = trap_S_RegisterSound ("sound/chars/seeker/misc/hiss");
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_bacta.wav");
	
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM006" );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/chars/mothma/misc/40MOM010" );

	//jk2pro
	cgs.media.lowHPSound	= trap_S_RegisterSound("sound/common/warning.wav");
	cgs.media.hitSound		= trap_S_RegisterSound("sound/effects/hitsound.wav"); 
	cgs.media.hitSound2		= trap_S_RegisterSound("sound/effects/hitsound2.wav");
	cgs.media.hitSound3		= trap_S_RegisterSound("sound/effects/hitsound3.wav");
	cgs.media.hitSound4		= trap_S_RegisterSound("sound/effects/hitsound4.wav");
	cgs.media.hitTeamSound	= trap_S_RegisterSound("sound/effects/hitsoundteam.wav");

	//new chat sound options
	cgs.media.teamChatSound = trap_S_RegisterSound("sound/movers/switches/button_11.mp3");
	cgs.media.privateChatSound = trap_S_RegisterSound("sound/interface/commlink_off.mp3");
}


//-------------------------------------
// CG_RegisterEffects
// 
// Handles precaching all effect files
//	and any shader, model, or sound
//	files an effect may use.
//-------------------------------------
static void CG_RegisterEffects( void )
{
	int			i;

	for ( i = 1 ; i < MAX_FX ; i++ ) 
	{
		CG_UpdateConfigString( CS_EFFECTS + i, qtrue );
	}

	// Set up the glass effects mini-system.
	CG_InitGlass();
}

//===================================================================================

extern char *forceHolocronModels[];
/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	static char		*sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};

	static char		*sb_c_nums[11] = {
		"gfx/2d/numbers/c_zero",
		"gfx/2d/numbers/c_one",
		"gfx/2d/numbers/c_two",
		"gfx/2d/numbers/c_three",
		"gfx/2d/numbers/c_four",
		"gfx/2d/numbers/c_five",
		"gfx/2d/numbers/c_six",
		"gfx/2d/numbers/c_seven",
		"gfx/2d/numbers/c_eight",
		"gfx/2d/numbers/c_nine",
		"gfx/2d/numbers/t_minus", //?????
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	cg.loadLCARSStage = 3;

	for ( i=0; i < 11; i++ )
	{
		cgs.media.numberShaders[i]			= trap_R_RegisterShaderNoMip( sb_nums[i] );
		cgs.media.smallnumberShaders[i]		= trap_R_RegisterShaderNoMip( sb_t_nums[i] );
		cgs.media.chunkyNumberShaders[i]	= trap_R_RegisterShaderNoMip( sb_c_nums[i] );
	}

	cgs.media.balloonShader = trap_R_RegisterShader( "gfx/mp/chat_icon" );

	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = trap_R_RegisterShaderNoMip("gfx/2d/lag" );
	cgs.media.connectionShader = trap_R_RegisterShaderNoMip( "gfx/2d/net" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );

	Com_Printf( S_COLOR_CYAN "---------- Fx System Initialization ---------\n" );
	trap_FX_InitSystem();
	Com_Printf( S_COLOR_CYAN "----- Fx System Initialization Complete -----\n" );
	CG_RegisterEffects();


	cgs.effects.turretShotEffect = trap_FX_RegisterEffect( "turret/shot" );

	trap_FX_RegisterEffect("effects/blaster/deflect.efx");

	trap_FX_RegisterEffect("emplaced/dead_smoke.efx");
	trap_FX_RegisterEffect("emplaced/explode.efx");

	trap_FX_RegisterEffect("turret/explode.efx");

	trap_FX_RegisterEffect("spark_explosion.efx");

	trap_FX_RegisterEffect("effects/turret/muzzle_flash.efx");
	trap_FX_RegisterEffect("saber/spark.efx");
	trap_FX_RegisterEffect("mp/spawn.efx");
	trap_FX_RegisterEffect("mp/jedispawn.efx");
	trap_FX_RegisterEffect("mp/itemcone.efx");
	trap_FX_RegisterEffect("blaster/deflect.efx");
	trap_FX_RegisterEffect("saber/saber_block.efx");
	trap_FX_RegisterEffect("saber/spark.efx");
	trap_FX_RegisterEffect("saber/blood_sparks.efx");
	trap_FX_RegisterEffect("blaster/smoke_bolton");
	trap_FX_RegisterEffect("force/confusion.efx");

	trap_FX_RegisterEffect("effects/force/lightning.efx");


	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cg.loadLCARSStage = 4;

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	cgs.media.itemRespawningPlaceholder = trap_R_RegisterShader("powerups/placeholder");
	cgs.media.itemRespawningRezOut = trap_R_RegisterShader("powerups/rezout");

	cgs.media.playerShieldDamage = trap_R_RegisterShader("gfx/misc/personalshield");
	cgs.media.forceSightBubble = trap_R_RegisterShader("gfx/misc/sightbubble");
	cgs.media.forceShell = trap_R_RegisterShader("powerups/forceshell");
	cgs.media.sightShell = trap_R_RegisterShader("powerups/sightshell");

	cgs.media.itemHoloModel = trap_R_RegisterModel("models/map_objects/mp/holo.md3");

	if (cgs.gametype == GT_HOLOCRON || cg_buildScript.integer)
	{
		for ( i=0; i < NUM_FORCE_POWERS; i++ )
		{
			if (forceHolocronModels[i] &&
				forceHolocronModels[i][0])
			{
				trap_R_RegisterModel(forceHolocronModels[i]);
			}
		}
	}

	cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY || cg_buildScript.integer ) {
		if (cg_buildScript.integer)
		{
			trap_R_RegisterModel( "models/flags/r_flag.md3" );
			trap_R_RegisterModel( "models/flags/b_flag.md3" );
			trap_R_RegisterModel( "models/flags/r_flag_ysal.md3" );
			trap_R_RegisterModel( "models/flags/b_flag_ysal.md3" );
		}

		if (cgs.gametype == GT_CTF)
		{
			cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
			cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
		}
		else
		{
			cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag_ysal.md3" );
			cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag_ysal.md3" );
		}

		cgs.media.flagShaderYsal[TEAM_RED] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		cgs.media.flagShaderYsal[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
		cgs.media.flagShaderYsal[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" ); //will have to do for now

		cgs.media.flagShader[TEAM_RED] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		cgs.media.flagShader[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );
		cgs.media.flagShader[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );

		cgs.media.flagShaderTaken[TEAM_RED] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
		cgs.media.flagShaderTaken[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
		cgs.media.flagShaderTaken[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1_x" );


		trap_R_RegisterShaderNoMip("gfx/2d/net.tga");

		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );

		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
	}


	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.teamRedShader = trap_R_RegisterShader( "sprites/team_red" );
		cgs.media.teamBlueShader = trap_R_RegisterShader( "sprites/team_blue" );
		cgs.media.teamYellowShader = trap_R_RegisterShader( "sprites/team_yellow" );
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
	}
	else if ( cgs.gametype == GT_JEDIMASTER )
	{
		cgs.media.teamRedShader = trap_R_RegisterShader( "sprites/team_red" );
	}

	cgs.media.armorModel	= 0;//trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = 0;//trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.heartShader			= trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );

	cgs.media.ysaliredShader		= trap_R_RegisterShader( "powerups/ysaliredshell");
	cgs.media.ysaliblueShader		= trap_R_RegisterShader( "powerups/ysaliblueshell");
	cgs.media.ysalimariShader		= trap_R_RegisterShader( "powerups/ysalimarishell");
	cgs.media.boonShader			= trap_R_RegisterShader( "powerups/boonshell");
	cgs.media.endarkenmentShader	= trap_R_RegisterShader( "powerups/endarkenmentshell");
	cgs.media.enlightenmentShader	= trap_R_RegisterShader( "powerups/enlightenmentshell");
	cgs.media.invulnerabilityShader = trap_R_RegisterShader( "powerups/invulnerabilityshell");

//JAPRO - Clientside - Movement Keys - Start
	cgs.media.keyCrouchOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/crouch_off");
	cgs.media.keyCrouchOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/crouch_on");
	cgs.media.keyJumpOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/jump_off");
	cgs.media.keyJumpOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/jump_on");
	cgs.media.keyJumpOffAutoKickShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/jump_off_autokick");
	cgs.media.keyJumpOnAutoKickShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/jump_on_autokick");
	cgs.media.keyBackOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/back_off");
	cgs.media.keyBackOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/back_on");
	cgs.media.keyForwardOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/forward_off");
	cgs.media.keyForwardOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/forward_on");
	cgs.media.keyLeftOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/left_off");
	cgs.media.keyLeftOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/left_on");
	cgs.media.keyRightOffShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/right_off");
	cgs.media.keyRightOnShader = trap_R_RegisterShaderNoMip("gfx/hud/keys/right_on");
//JAPRO - Clientside - Movement Keys - End

#ifdef JK2AWARDS
	cgs.media.medalImpressive		= trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent		= trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet			= trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend			= trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist			= trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture			= trap_R_RegisterShaderNoMip( "medal_capture" );
#endif

	// Binocular interface
	cgs.media.binocularCircle		= trap_R_RegisterShader( "gfx/2d/binCircle" );
	cgs.media.binocularMask			= trap_R_RegisterShader( "gfx/2d/binMask" );
	cgs.media.binocularArrow		= trap_R_RegisterShader( "gfx/2d/binSideArrow" );
	cgs.media.binocularTri			= trap_R_RegisterShader( "gfx/2d/binTopTri" );
	cgs.media.binocularStatic		= trap_R_RegisterShader( "gfx/2d/binocularWindow" );
	cgs.media.binocularOverlay		= trap_R_RegisterShader( "gfx/2d/binocularNumOverlay" );

	cg.loadLCARSStage = 5;

/*
Ghoul2 Insert Start
*/
	CG_InitItems();
/*
Ghoul2 Insert End
*/
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// Register items
	CG_UpdateConfigString( CS_ITEMS, qtrue );

	cg.loadLCARSStage = 6;

	cgs.media.glassShardShader	= trap_R_RegisterShader( "gfx/misc/test_crackle" );

	// doing one shader just makes it look like a shell.  By using two shaders with different bulge offsets and different texture scales, it has a much more chaotic look
	cgs.media.electricBodyShader			= trap_R_RegisterShader( "gfx/misc/electric" );
	cgs.media.electricBody2Shader			= trap_R_RegisterShader( "gfx/misc/fullbodyelectric2" );

	// wall marks
	cgs.media.bulletMarkShader	= trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader	= trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader	= trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader	= trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader	= trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader	= trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader	= trap_R_RegisterShader( "bloodMark" );

	cgs.media.viewPainShader					= trap_R_RegisterShader( "gfx/misc/borgeyeflare" );
	cgs.media.viewPainShader_Shields			= trap_R_RegisterShader( "gfx/mp/dmgshader_shields" );
	cgs.media.viewPainShader_ShieldsAndHealth	= trap_R_RegisterShader( "gfx/mp/dmgshader_shieldsandhealth" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();

	// Considering the cgame module doesn't make use of the ~ 2 mb memory pool in BG we can safely allocate some of it
	// for the inline models instead of having them hardcoded to 256. In a QVM the qhandle_t should be 4 byte and the
	// vec3_t should be 12 byte. For 256 models that's barely 4 kb.
	cgs.inlineDrawModel = (qhandle_t*)BG_Alloc( cgs.numInlineModels * sizeof(qhandle_t) );
	cgs.inlineModelMidpoints = (vec3_t*)BG_Alloc( cgs.numInlineModels * sizeof(vec3_t) );
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[16];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	cg.loadLCARSStage = 7;

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		CG_UpdateConfigString( CS_MODELS + i, qtrue );
	}
	cg.loadLCARSStage = 8;
/*
Ghoul2 Insert Start
*/
	CG_LoadingString("skins");
	// register all the server specified models
	for (i=1 ; i<MAX_CHARSKINS ; i++) {
		CG_UpdateConfigString( CS_CHARSKINS + i, qtrue );
	}

	CG_InitG2Weapons();

/*
Ghoul2 Insert End
*/
	cg.loadLCARSStage = 9;


	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgs.media.halfShieldModel	= trap_R_RegisterModel ( "models/weaphits/testboom.md3" );
	cgs.media.halfShieldShader	= trap_R_RegisterShader( "halfShieldShell" );


	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/

// --- V24 Enhanced Features: Friend System Console Commands ---
void CG_AddFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /addfriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Printf("^1Invalid client number. Use numbers 0-%d.\n", MAX_CLIENTS - 1);
		return;
	}
	if (!cgs.clientinfo[clientNum].infoValid) {
		CG_Printf("^1Client %d is not active.\n", clientNum);
		return;
	}
	CG_AddFriend(clientNum);
}

void CG_RemoveFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /removefriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	CG_RemoveFriend(clientNum);
}

void CG_ClearFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ClearFriends();
}

void CG_ListFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ListFriends();
}
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgs.media.halfShieldModel	= trap_R_RegisterModel ( "models/weaphits/testboom.md3" );
	cgs.media.halfShieldShader	= trap_R_RegisterShader( "halfShieldShell" );


	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}


// --- V24 Enhanced Features: Friend System Console Commands ---
void CG_AddFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /addfriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Printf("^1Invalid client number. Use numbers 0-%d.\n", MAX_CLIENTS - 1);
		return;
	}
	if (!cgs.clientinfo[clientNum].infoValid) {
		CG_Printf("^1Client %d is not active.\n", clientNum);
		return;
	}
	CG_AddFriend(clientNum);
}

void CG_RemoveFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /removefriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	CG_RemoveFriend(clientNum);
}

void CG_ClearFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ClearFriends();
}

void CG_ListFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ListFriends();
}
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgs.media.halfShieldModel	= trap_R_RegisterModel ( "models/weaphits/testboom.md3" );
	cgs.media.halfShieldShader	= trap_R_RegisterShader( "halfShieldShell" );


	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/



// --- V24 Enhanced Features: Friend System Console Commands ---
void CG_AddFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /addfriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Printf("^1Invalid client number. Use numbers 0-%d.\n", MAX_CLIENTS - 1);
		return;
	}
	if (!cgs.clientinfo[clientNum].infoValid) {
		CG_Printf("^1Client %d is not active.\n", clientNum);
		return;
	}
	CG_AddFriend(clientNum);
}

void CG_RemoveFriend_f(void) {
	int clientNum;
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	if (trap_Argc() < 2) {
		CG_Printf("^3Usage: /removefriend <client number>\n");
		return;
	}
	clientNum = atoi(CG_Argv(1));
	CG_RemoveFriend(clientNum);
}

void CG_ClearFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ClearFriends();
}

void CG_ListFriends_f(void) {
	if (!cg_friendsSystem.integer) {
		CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
		return;
	}
	CG_ListFriends();
}
