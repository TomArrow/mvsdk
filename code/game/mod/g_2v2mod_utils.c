#include "../g_local.h"

void G_TvT_MakeNameUnique(int clientNum, char *name) {
	char	stripped[MAX_NETNAME];
	char	otherStripped[MAX_NETNAME];
	char	baseName[MAX_NETNAME];
	char	suffix[16];
	int		i, suffixNum, baseMaxLen;
	qboolean duplicate;

	if (!tvt_uniqueNames.integer) {
		return;
	}

	Q_strncpyz(baseName, name, sizeof(baseName));

	suffixNum = 1;
	while ( 1 ) {
		Q_strncpyz(stripped, name, sizeof(stripped));
		Q_CleanStr(stripped, (qboolean)(jk2startversion == VERSION_1_02));

		duplicate = qfalse;
		for (i = 0; i < level.maxclients; i++) {
			if (i == clientNum) {
				continue;
			}
			if (level.clients[i].pers.connected == CON_DISCONNECTED) {
				continue;
			}

			Q_strncpyz(otherStripped, level.clients[i].pers.netname, sizeof(otherStripped));
			Q_CleanStr(otherStripped, (qboolean)(jk2startversion == VERSION_1_02));

			if (!Q_stricmp(stripped, otherStripped)) {
				duplicate = qtrue;
				break;
			}
		}

		if (!duplicate) {
			break;
		}

		Com_sprintf(suffix, sizeof(suffix), "^7(%d)", suffixNum);

		baseMaxLen = MAX_NETNAME - 1 - strlen(suffix);

		Q_strncpyz(name, baseName, baseMaxLen + 1);
		Q_strcat(name, MAX_NETNAME, suffix);

		suffixNum++;
	}
}

void G_TvT_Printf(int clientNum, const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (clientNum == TVT_PRINT_CONSOLE) {
		G_Printf("%s", buf);
	} else {
		trap_SendServerCommand(clientNum, va("print \"%s\"", buf));
	}
}

static void G_TvT_ResetItem(gentity_t *ent) {
	G_SetOrigin(ent, ent->pos1);
	RespawnItem(ent);
}

static void G_TvT_TouchItem(gentity_t *ent, gentity_t *other, trace_t *trace) {
	Touch_Item(ent, other, trace);

	// Use a wrapper for respawn item to make sure it spawns at its original position
	if (ent->think == RespawnItem) {
		ent->think = G_TvT_ResetItem;
	}
}

void G_TvT_ForcePushItem(gentity_t *item, qboolean pull, vec3_t forward) {
	float speed = pull ? -650.0f : 650.0f;

	if (!(item->flags & FL_DROPPED_ITEM)) {
		if (item->touch == Touch_Item) {
			// pos1 is unused for items, so we can store the original spawn position in this field
			VectorCopy(item->r.currentOrigin, item->pos1);
			item->touch = G_TvT_TouchItem;
		}

		item->nextthink = level.time + 30000;
		item->think = G_TvT_ResetItem;
	}

	item->s.pos.trType = TR_GRAVITY;
	item->s.pos.trTime = level.time;
	VectorCopy(item->r.currentOrigin, item->s.pos.trBase);
	VectorScale(forward, speed, item->s.pos.trDelta);

	item->s.groundEntityNum = ENTITYNUM_NONE;
}

void G_TvT_SetLoadout(gclient_t *client, qboolean firstSpawn) {
	tvt_ModState_t *tvt = &level.tvt;
	int spawnType = firstSpawn ? TVT_FIRST_SPAWN : TVT_SPAWN;

	// Set the first spawn & respawn, shield & holdable value for players
	client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * tvt->spawnArmor[spawnType] / 100;
	client->ps.stats[STAT_HOLDABLE_ITEMS] |= tvt->spawnItems[spawnType];
}

void G_TvT_FisherYatesShuffle(int *array, int n) {
	// From https://stackoverflow.com/questions/42321370/fisher-yates-shuffling-algorithm-in-c/42322025#42322025
	int i, j, tmp;

	for (i = n - 1; i > 0; i--) {
		j = rand() % (i + 1);
		tmp = array[j];
		array[j] = array[i];
		array[i] = tmp;
	}
}

qboolean G_TvT_IsNumericString(const char *s) {
	if (!s || !*s) return qfalse;
	if (*s == '-') s++;
	if (!*s) return qfalse;
	while (*s) {
		if (*s < '0' || *s > '9') return qfalse;
		s++;
	}
	return qtrue;
}

