#ifndef G_2V2MOD_CVARS_H
#define G_2V2MOD_CVARS_H

#include "../q_shared.h"

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	const char	*description;
	int			cvarFlags;
	int			modificationCount;
	qboolean	trackChange;
	qboolean	(*onChanged)(void);
} tvt_Cvar_t;

qboolean G_TvT_UpdateSpawnArmor(void);
qboolean G_TvT_UpdateSpawnItems(void);

#define TVT_CVAR_LIST \
	TVT_CVAR(tvt_allowBlackNames, "1", "Allow players to use the colour black in their names", CVAR_ARCHIVE, qtrue, NULL) \
	TVT_CVAR(tvt_uniqueNames, "1", "Append a numeric suffix to non unique player names", CVAR_ARCHIVE, qtrue, NULL) \
	TVT_CVAR(tvt_forcePushItems, "1", "Allow force push and pull to move spawned items", CVAR_ARCHIVE, qtrue, NULL) \
	TVT_CVAR(tvt_teamForceRules, "1", "Allow force push, pull and drain to affect teammates", CVAR_ARCHIVE, qtrue, NULL) \
	TVT_CVAR(tvt_teamSuicideScoring, "1", "Teamkill or suicide awards +1 score to the opposing team instead of -1 score to self", CVAR_ARCHIVE, qtrue, NULL) \
	TVT_CVAR(tvt_spawnArmor, "25 25", "Armor value on spawn. Two values for 'respawn firstSpawn' (e.g. '25 200')", CVAR_ARCHIVE, qtrue, G_TvT_UpdateSpawnArmor) \
	TVT_CVAR(tvt_spawnItems, "0 0", "Holdable item bitmask on spawn. Two values for 'respawn firstSpawn' (e.g. '0 8')", CVAR_ARCHIVE, qtrue, G_TvT_UpdateSpawnItems)

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, update) extern vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

void G_TvT_RegisterCvars(void);
void G_TvT_UpdateCvars(void);

#endif
