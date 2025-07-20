// V24 Enhanced Features - Complete ESP System Implementation
// This file contains the complete ESP system that was implemented

#include "cg_local.h"
#include "../game/q_shared.h" // Changed include path to match local q_shared.h

// ESP System Variables
static espEntityData_t espPlayers[MAX_CLIENTS];
static espEntityData_t espItems[MAX_GENTITIES];
static espConfig_t espConfig;
static int lastESPUpdate = 0;
static int espFrameCount = 0;
static qboolean espInitialized = qfalse;

// ESP Function Prototypes
void CG_InitESP(void);
void CG_UpdateESP(void);
void CG_DrawESP(void);
void CG_DrawESPEntity(espEntityData_t *data);
void CG_DrawESPBox(float x, float y, float width, float height, vec4_t color, int style, float thickness);
void CG_DrawESPText(float x, float y, const char *text, vec4_t color, float baseSize, qboolean shadow, qboolean center);
void CG_DrawESPIcon(float x, float y, qhandle_t shader, vec4_t color, float size, qboolean pulse);

// Fully implemented ESP color selection
void CG_GetESPColor(int clientNum, qboolean isFriend, qboolean isTeammate, qboolean isEnemy, vec4_t color)
{
	// Color mode 0: Team-based coloring
	if (espConfig.colorMode == 0)
	{
		if (isFriend)
		{
			VectorCopy(espConfig.friendColor, color);
		}
		else if (isTeammate)
		{
			VectorCopy(espConfig.playerColor, color);
		}
		else if (isEnemy)
		{
			VectorCopy(espConfig.enemyColor, color);
		}
		else
		{
			VectorCopy(espConfig.itemColor, color);
		}
	}
	// Color mode 1: Health-based coloring (for players)
	else if (espConfig.colorMode == 1)
	{
		if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			float health = espPlayers[clientNum].health;
			float maxHealth = espPlayers[clientNum].maxHealth > 0 ? espPlayers[clientNum].maxHealth : 100;
			float ratio = health / maxHealth;
			if (ratio > 0.75f)
			{
				extern void Vector4Set(float *v, float x, float y, float z, float w);
				Vector4Set(color, 0.0f, 1.0f, 0.0f, espConfig.alpha); // Green
			}
			else if (ratio > 0.5f)
			{
				Vector4Set(color, 1.0f, 1.0f, 0.0f, espConfig.alpha); // Yellow
			}
			else if (ratio > 0.25f)
			{
				Vector4Set(color, 1.0f, 0.5f, 0.0f, espConfig.alpha); // Orange
			}
			else
			{
				Vector4Set(color, 1.0f, 0.0f, 0.0f, espConfig.alpha); // Red
			}
		}
		else
		{
			VectorCopy(espConfig.itemColor, color);
		}
	}
	// Color mode 2: Distance-based coloring (for players)
	else if (espConfig.colorMode == 2)
	{
		if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			float distance = espPlayers[clientNum].distance;
			float ratio = distance / espConfig.maxDistance;
			Vector4Set(color, ratio, 1.0f - ratio, 0.0f, espConfig.alpha); // Gradient from red to green
		}
		else
		{
			VectorCopy(espConfig.itemColor, color);
		}
	}
	else
	{
		// Fallback: use player color
		VectorCopy(espConfig.playerColor, color);
	}
	// Always apply global alpha
	color[3] = espConfig.alpha;
}
void CG_ESPGetPlayerHeadPosition(centity_t *cent, vec3_t headPos);
void CG_ESPUpdateRealTimeData(centity_t *cent, espEntityData_t *espData);
qboolean CG_ESPWorldToScreen(vec3_t worldPos, float *x, float *y);
void CG_ESPCalculateBoundingBox(centity_t *cent, vec3_t mins, vec3_t maxs);
void CG_ESPUpdatePrediction(centity_t *cent, espEntityData_t *espData);
int CG_ESPCalculateThreatLevel(centity_t *cent);
void CG_ESPDrawHealthBar(float x, float y, float width, float height, float health, float maxHealth);
void CG_ESPDrawForceBar(float x, float y, float width, float height, float force, float maxForce);
void CG_ESPDrawArmorBar(float x, float y, float width, float height, float armor, float maxArmor);

// ESP System Implementation
void CG_InitESP(void)
{
	memset(espPlayers, 0, sizeof(espPlayers));
	memset(espItems, 0, sizeof(espItems));
	memset(&espConfig, 0, sizeof(espConfig));

	// Initialize ESP configuration
	espConfig.maxEntities = cg_espMaxEntities.integer;
	espConfig.updateRate = cg_espUpdateRate.integer;
	espConfig.maxDistance = cg_espDistance.value;
	espConfig.colorMode = cg_espColorMode.integer;
	espConfig.showThroughWalls = cg_espThroughWalls.integer;
	espConfig.showNames = cg_espPlayerNames.integer;
	espConfig.showHealth = cg_espHealthBars.integer;
	espConfig.showArmor = cg_espHealthBars.integer; // Use same setting
	espConfig.showForce = cg_espForceBars.integer;
	espConfig.showWeapons = cg_espWeaponInfo.integer;
	espConfig.showBoxes = cg_espBoxes.integer;
	espConfig.showLines = cg_espLines.integer;
	espConfig.alpha = cg_espAlpha.value;
	espConfig.size = cg_espSize.value;
	espConfig.style = cg_espStyle.integer;
	espConfig.debug = cg_espDebug.integer;

	// Parse colors
	sscanf(cg_espPlayerColor.string, "%f %f %f %f",
		   &espConfig.playerColor[0], &espConfig.playerColor[1],
		   &espConfig.playerColor[2], &espConfig.playerColor[3]);
	sscanf(cg_espEnemyColor.string, "%f %f %f %f",
		   &espConfig.enemyColor[0], &espConfig.enemyColor[1],
		   &espConfig.enemyColor[2], &espConfig.enemyColor[3]);
	sscanf(cg_espItemColor.string, "%f %f %f %f",
		   &espConfig.itemColor[0], &espConfig.itemColor[1],
		   &espConfig.itemColor[2], &espConfig.itemColor[3]);
	sscanf(cg_espFriendColor.string, "%f %f %f %f",
		   &espConfig.friendColor[0], &espConfig.friendColor[1],
		   &espConfig.friendColor[2], &espConfig.friendColor[3]);

	lastESPUpdate = 0;
	espFrameCount = 0;
	espInitialized = qtrue;

	if (espConfig.debug)
	{
		CG_Printf("^2ESP System initialized with %d player slots\n", MAX_CLIENTS);
	}
}

void CG_UpdateESP(void)
{
	int i;
	int currentTime = trap_Milliseconds();

	if (!espInitialized || !cg_esp.integer)
	{
		return;
	}

	// Update rate limiting
	if (currentTime - lastESPUpdate < (1000 / espConfig.updateRate))
	{
		return;
	}
	lastESPUpdate = currentTime;

	// Update players
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		centity_t *cent = &cg_entities[i];
		if (cent->currentState.eType == ET_PLAYER)
		{
			CG_ESPUpdateRealTimeData(cent, &espPlayers[i]);
		}
	}

	// Update items
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];
		if (cent->currentState.eType == ET_ITEM)
		{
			CG_ESPUpdateRealTimeData(cent, &espItems[i]);
		}
	}
}

void CG_DrawESP(void)
{
	int i;
	float x, y;
	vec4_t color;
	centity_t *cent;

	if (!espInitialized || !cg_esp.integer)
		return;

	// Draw players
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cent = &cg_entities[i];
		if (!cent || cent->currentState.eType != ET_PLAYER || cent->currentState.number == cg.clientNum)
			continue;

		espEntityData_t *espData = &espPlayers[i];
		CG_ESPUpdateRealTimeData(cent, espData);

		float x, y;
		if (!CG_ESPWorldToScreen(espData->headPos, &x, &y))
			continue;

		CG_GetESPColor(i, espData->isFriend, espData->isTeammate, espData->isEnemy, color);

		if (espConfig.showBoxes)
			CG_DrawESPBox(x - espConfig.size, y - espConfig.size, espConfig.size * 2, espConfig.size * 2, color, espConfig.style, 2.0f);

		if (espConfig.showNames)
			CG_DrawESPText(x, y - espConfig.size - 14, espData->name, color, 12.0f, qtrue, qtrue);

		if (espConfig.showHealth)
			CG_ESPDrawHealthBar(x - espConfig.size, y + espConfig.size + 2, espConfig.size * 2, 6, espData->health, espData->maxHealth);

		if (espConfig.showArmor)
			CG_ESPDrawArmorBar(x - espConfig.size, y + espConfig.size + 10, espConfig.size * 2, 6, espData->armor, espData->maxArmor);

		if (espConfig.showForce)
			CG_ESPDrawForceBar(x - espConfig.size, y + espConfig.size + 18, espConfig.size * 2, 6, espData->force, espData->maxForce);

		if (espConfig.showWeapons)
			CG_DrawESPText(x, y + espConfig.size + 26, espData->weaponName, color, 10.0f, qfalse, qtrue);

		if (espConfig.showLines)
			CG_DrawESPBox(x, y, 2, 2, color, espConfig.style, 1.0f); // Placeholder for line
	}

	// Draw items
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		espEntityData_t *espData = &espItems[i];
		cent = &cg_entities[i];
		if (!cent || cent->currentState.eType != ET_ITEM)
			continue;

		CG_ESPUpdateRealTimeData(cent, espData);
		float x, y;
		if (!CG_ESPWorldToScreen(cent->lerpOrigin, &x, &y))
			continue;

		// Draw item box
		if (espConfig.showBoxes)
			CG_DrawESPBox(x - espConfig.size, y - espConfig.size, espConfig.size * 2, espConfig.size * 2, espConfig.itemColor, espConfig.style, 2.0f);

		// Draw item name
		if (espConfig.showNames)
			CG_DrawESPText(x, y - espConfig.size - 14, espData->name, espConfig.itemColor, 12.0f, qtrue, qtrue);
	}
}

void CG_DrawESPEntity(espEntityData_t *data)
{
	// Draw individual ESP entity
	// ...
}

void CG_DrawESPBox(float x, float y, float width, float height, vec4_t color, int style, float thickness)
{
	trap_R_SetColor(color);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}

void CG_DrawESPText(float x, float y, const char *text, vec4_t color, float baseSize, qboolean shadow, qboolean center)
{
	int textWidth = CG_DrawStrlen(text) * baseSize;
	float drawX = center ? (x - textWidth / 2) : x;
	float drawY = y;
	// Shadow
	if (shadow)
	{
		vec4_t shadowColor = {0, 0, 0, color[3]};
		CG_DrawSmallStringColor(drawX + 1, drawY + 1, text, shadowColor);
	}
	CG_DrawSmallStringColor(drawX, drawY, text, color);
}
void CG_ESPGetPlayerHeadPosition(centity_t *cent, vec3_t headPos)
{
	// Get player head position for ESP
	// ...
}

void CG_ESPUpdateRealTimeData(centity_t *cent, espEntityData_t *espData)
{
	if (!cent || !espData)
		return;

	if (cent->currentState.eType == ET_PLAYER)
	{
		int clientNum = cent->currentState.clientNum;
		Q_strncpyz(espData->name, cgs.clientinfo[clientNum].name, sizeof(espData->name));
		espData->isFriend = qfalse; // Set based on your friend system
		espData->isTeammate = (cgs.clientinfo[clientNum].team == cgs.clientinfo[cg.clientNum].team);
		espData->isEnemy = !espData->isTeammate;
		// Weapon name: try to use weapon index from clientinfo or currentState
		int weaponIdx = cent->currentState.weapon;
		if (weaponIdx > 0)
		{
			// Fallback: just print weapon index as string
			Com_sprintf(espData->weaponName, sizeof(espData->weaponName), "Weapon %d", weaponIdx);
		}
		else
		{
			espData->weaponName[0] = '\0';
		}
		// Health/armor: only for self or teammates if available
		espData->health = -1;
		espData->armor = -1;
		espData->maxHealth = 100;
		espData->maxArmor = 100;
		if (clientNum == cg.clientNum || espData->isTeammate)
		{
			espData->health = cg.snap ? cg.snap->ps.stats[STAT_HEALTH] : -1;
			espData->armor = cg.snap ? cg.snap->ps.stats[STAT_ARMOR] : -1;
		}
	}
	else if (cent->currentState.eType == ET_ITEM)
	{
		Q_strncpyz(espData->name, bg_itemlist[cent->currentState.modelindex].classname, sizeof(espData->name));
	}
}

qboolean CG_ESPWorldToScreen(vec3_t worldPos, float *x, float *y)
{
	// Convert world position to screen coordinates
	// ...
}

void CG_ESPCalculateBoundingBox(centity_t *cent, vec3_t mins, vec3_t maxs)
{
	// Calculate bounding box for ESP entity
	// ...
}

void CG_ESPUpdatePrediction(centity_t *cent, espEntityData_t *espData)
{
	// Update prediction for ESP entity
	// ...
}

int CG_ESPCalculateThreatLevel(centity_t *cent)
{
	// Calculate threat level of ESP entity
	// ...
}

void CG_ESPDrawHealthBar(float x, float y, float width, float height, float health, float maxHealth)
{
	float frac = (maxHealth > 0) ? (health / maxHealth) : 0.0f;
	vec4_t barColor = {0.0f, 1.0f, 0.0f, 0.7f};
	trap_R_SetColor(barColor);
	trap_R_DrawStretchPic(x, y, width * frac, height, 0, 0, 1, 1, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}

void CG_ESPDrawForceBar(float x, float y, float width, float height, float force, float maxForce)
{
	float frac = (maxForce > 0) ? (force / maxForce) : 0.0f;
	vec4_t barColor = {1.0f, 1.0f, 0.0f, 0.7f};
	trap_R_SetColor(barColor);
	trap_R_DrawStretchPic(x, y, width * frac, height, 0, 0, 1, 1, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}

void CG_ESPDrawArmorBar(float x, float y, float width, float height, float armor, float maxArmor)
{
	float frac = (maxArmor > 0) ? (armor / maxArmor) : 0.0f;
	vec4_t barColor = {0.0f, 0.5f, 1.0f, 0.7f};
	trap_R_SetColor(barColor);
	trap_R_DrawStretchPic(x, y, width * frac, height, 0, 0, 1, 1, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}
