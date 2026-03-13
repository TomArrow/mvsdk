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
} tvt_Cvar_t;

#define TVT_CVAR_LIST \
	TVT_CVAR(tvt_allowBlackNames, "1", "Allow players to use the colour black in their names", 0, qtrue) \
	TVT_CVAR(tvt_uniqueNames, "1", "Append a numeric suffix to non unique player names", 0, qtrue) \
	TVT_CVAR(tvt_forcePushItems, "1", "Allow force push and pull to move spawned items", 0, qtrue) \
	TVT_CVAR(tvt_teamForceRules, "0", "Allow force push, pull and drain to affect teammates", 0, qtrue)

#define TVT_CVAR(name, defaultValue, description, flags, trackChange) extern vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

void G_TvT_RegisterCvars(void);
void G_TvT_UpdateCvars(void);

#endif
