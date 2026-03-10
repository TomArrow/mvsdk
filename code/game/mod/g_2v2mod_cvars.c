#include "../g_local.h"

#define TVT_CVAR(name, defaultValue, description, flags, trackChange) vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

#define TVT_CVAR(name, defaultValue, description, flags, trackChange) \
	{&name, #name, defaultValue, description, flags, 0, trackChange},
static tvt_Cvar_t tvtCvarTable[] = {
	TVT_CVAR_LIST
};
#undef TVT_CVAR

static int tvtCvarTableSize = ARRAY_LEN(tvtCvarTable);

void G_TvT_RegisterCvars(void) {
	int i;
	tvt_Cvar_t *cv;

	for (i = 0, cv = tvtCvarTable; i < tvtCvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar) {
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}
}

void G_TvT_UpdateCvars(void) {
	int i;
	tvt_Cvar_t *cv;

	for (i = 0, cv = tvtCvarTable; i < tvtCvarTableSize; i++, cv++) {
		if (cv->vmCvar) {
			trap_Cvar_Update(cv->vmCvar);

			if (cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->trackChange) {
					trap_SendServerCommand(-1,
						va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string));
				}
			}
		}
	}
}
