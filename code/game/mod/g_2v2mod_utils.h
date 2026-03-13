#ifndef G_2V2MOD_UTILS_H
#define G_2V2MOD_UTILS_H

#define TVT_PRINT_ALL      -1
#define TVT_PRINT_CONSOLE  -2

#define TVT_ENT_TO_CN(ent) ((ent) ? (int)((ent) - g_entities) : TVT_PRINT_CONSOLE)

// Generic print function
void G_TvT_Printf(int clientNum, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void G_TvT_MakeNameUnique(int clientNum, char *name);
void G_TvT_FisherYatesShuffle(int *array, int n);

// Force push/pull items
void G_TvT_ForcePushItem(gentity_t *item, qboolean pull, vec3_t forward);

// Spawn loadout
void G_TvT_SetLoadout(gclient_t *client, qboolean firstSpawn);

// Utilities
void G_TvT_FisherYatesShuffle(int *array, int n);
qboolean G_TvT_IsNumericString(const char *s) ;

#endif
