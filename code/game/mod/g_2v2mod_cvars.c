#include "../g_local.h"

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, update) vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, update) \
    {&name, #name, defaultValue, description, flags, 0, trackChange, update},
static tvt_Cvar_t tvtCvarTable[] = {
    TVT_CVAR_LIST};
#undef TVT_CVAR

static int tvtCvarTableSize = ARRAY_LEN(tvtCvarTable);

void G_TvT_RegisterCvars(void) {
    int         i;
    tvt_Cvar_t *cv;

    for (i = 0, cv = tvtCvarTable; i < tvtCvarTableSize; i++, cv++) {
        trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
        if (cv->vmCvar) {
            cv->modificationCount = cv->vmCvar->modificationCount;
        }
        if (cv->onChanged) {
            cv->onChanged();
        }
    }
}

void G_TvT_UpdateCvars(void) {
    int         i;
    tvt_Cvar_t *cv;

    for (i = 0, cv = tvtCvarTable; i < tvtCvarTableSize; i++, cv++) {
        if (cv->vmCvar) {
            trap_Cvar_Update(cv->vmCvar);

            if (cv->modificationCount != cv->vmCvar->modificationCount) {
                cv->modificationCount = cv->vmCvar->modificationCount;

                if (cv->onChanged && !cv->onChanged()) {
                    // Callback / validation failed, so lets update our cvar reference and reset the modification count to prevent rechecking this cvar until it changes again.
                    trap_Cvar_Update(cv->vmCvar);
                    cv->modificationCount = cv->vmCvar->modificationCount;
                    continue;
                }

                if (cv->trackChange) {
                    trap_SendServerCommand(-1,
                                           va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string));
                }
            }
        }
    }
}

tvt_Cvar_t *G_TvT_GetCvarTable(int *count) {
    *count = tvtCvarTableSize;
    return tvtCvarTable;
}

qboolean G_TvT_UpdateSpawnArmor(void) {
    tvt_ModState_t *tvt = &level.tvt;
    char            buf[MAX_CVAR_VALUE_STRING];
    char           *p;
    int             armor[2];

    Q_strncpyz(buf, tvt_spawnArmor.string, sizeof(buf));
    p = strchr(buf, ' ');
    if (p)
        *p = '\0';

    // Crappy validation but it'll do for now.
    if (!G_TvT_IsNumericString(buf) || (p && !G_TvT_IsNumericString(p + 1))) {
        G_Printf("tvt_spawnArmor: expected numeric value\n");
        trap_Cvar_Set("tvt_spawnArmor", va("%d %d", tvt->spawnArmor[TVT_SPAWN], tvt->spawnArmor[TVT_FIRST_SPAWN]));
        return qfalse;
    }

    armor[TVT_SPAWN] = Com_Clampi(0, 999, atoi(buf));
    armor[TVT_FIRST_SPAWN] = p ? Com_Clampi(0, 999, atoi(p + 1)) : armor[TVT_SPAWN];

    tvt->spawnArmor[TVT_SPAWN] = armor[TVT_SPAWN];
    tvt->spawnArmor[TVT_FIRST_SPAWN] = armor[TVT_FIRST_SPAWN];
    return qtrue;
}

qboolean G_TvT_UpdateSpawnItems(void) {
    tvt_ModState_t *tvt = &level.tvt;
    char            buf[MAX_CVAR_VALUE_STRING];
    char           *p;
    int             items[2];
    int             validItemMask = 0;
    int             i;

    Q_strncpyz(buf, tvt_spawnItems.string, sizeof(buf));
    p = strchr(buf, ' ');
    if (p)
        *p = '\0';

    // Crappy validation but it'll do for now.
    if (!G_TvT_IsNumericString(buf) || (p && !G_TvT_IsNumericString(p + 1))) {
        G_Printf("tvt_spawnItems: expected numeric value\n");
        trap_Cvar_Set("tvt_spawnItems", va("%d %d", tvt->spawnItems[TVT_SPAWN], tvt->spawnItems[TVT_FIRST_SPAWN]));
        return qfalse;
    }

    items[TVT_SPAWN] = atoi(buf);
    items[TVT_FIRST_SPAWN] = p ? atoi(p + 1) : items[TVT_SPAWN];

    for (i = HI_SEEKER; i < HI_NUM_HOLDABLE; i++) {
        validItemMask |= (1 << i);
    }
    if (items[TVT_SPAWN] & ~validItemMask || items[TVT_FIRST_SPAWN] & ~validItemMask) {
        G_Printf("tvt_spawnItems: invalid item bits (valid mask: %d)\n", validItemMask);
        trap_Cvar_Set("tvt_spawnItems", va("%d %d", tvt->spawnItems[TVT_SPAWN], tvt->spawnItems[TVT_FIRST_SPAWN]));
        return qfalse;
    }

    tvt->spawnItems[TVT_SPAWN] = items[TVT_SPAWN];
    tvt->spawnItems[TVT_FIRST_SPAWN] = items[TVT_FIRST_SPAWN];
    return qtrue;
}
