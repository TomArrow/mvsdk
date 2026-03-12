#include "../g_local.h"

static qboolean G_TvT_Cmd_MemStats(gentity_t *ent) {
	tvt_MemStats_t s;
	int cn = TVT_ENT_TO_CN(ent);

	TvT_Mem_GetStats(&s);

	G_TvT_Printf(cn, "--- Memory Pool Stats ---\n");
	G_TvT_Printf(cn, "Pool size:    %d bytes (%d MB)\n",
		(int)s.pool_size, (int)(s.pool_size / (1024 * 1024)));
	G_TvT_Printf(cn, "Used:         %d blocks, %d bytes (%d KB)\n",
		s.used_blocks, (int)s.used_bytes, (int)(s.used_bytes / 1024));
	G_TvT_Printf(cn, "  Largest:    %d bytes\n", (int)s.used_largest);
	G_TvT_Printf(cn, "  Overhead:   %d bytes (%d per block)\n",
		(int)s.used_overhead, (int)BLOCK_OVERHEAD);
	G_TvT_Printf(cn, "Free:         %d blocks, %d bytes (%d MB)\n",
		s.free_blocks, (int)s.free_bytes, (int)(s.free_bytes / (1024 * 1024)));
	G_TvT_Printf(cn, "  Largest:    %d bytes\n", (int)s.free_largest);

	if (s.free_blocks > 1) {
		G_TvT_Printf(cn, "Fragmentation: %d free segments\n", s.free_blocks);
	}

	return qtrue;
}

static qboolean G_TvT_Cmd_Shuffle(gentity_t *ent) {
	static int teamSelection = 0;
	int players[MAX_CLIENTS];
	unsigned int oldRed = 0, oldBlue = 0;
	unsigned int newRed, newBlue;
	int count = 0;
	int i, sel;

	if (g_gametype.integer < GT_TEAM) {
		G_TvT_Printf(TVT_ENT_TO_CN(ent), "This command is only allowed in team based gametypes.\n");
		return qtrue;
	}

	for (i = 0; i < level.maxclients; i++) {
		gclient_t *cl = &level.clients[i];

		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (cl->sess.sessionTeam == TEAM_RED) {
			oldRed |= (1u << i);
		} else if (cl->sess.sessionTeam == TEAM_BLUE) {
			oldBlue |= (1u << i);
		} else {
			continue;
		}
		players[count++] = i;
	}

	if (count < 3) {
		G_TvT_Printf(TVT_ENT_TO_CN(ent), "Not enough players to shuffle.\n");
		return qtrue;
	}

	do {
		G_TvT_FisherYatesShuffle(players, count);

		newRed = newBlue = 0;
		sel = teamSelection;
		for (i = 0; i < count; i++) {
			if (sel & 1) {
				newBlue |= (1u << players[i]);
			} else {
				newRed |= (1u << players[i]);
			}
			sel ^= 1;
		}
	} while (newRed == oldRed || newRed == oldBlue);

	for (i = 0; i < count; i++) {
		SetTeam(&g_entities[players[i]], (newRed & (1u << players[i])) ? "red" : "blue");
	}

	teamSelection ^= 1;

	CheckTeamLeader(TEAM_RED);
	CheckTeamLeader(TEAM_BLUE);

	trap_SendServerCommand(-1, "cp \"Teams have been shuffled.\n\"");
	return qtrue;
}

static tvt_Cmd_t tvt_commands[] = {
	{ "mem_stats", "Show memory pool statistics", "mem_stats",    G_TvT_Cmd_MemStats, CMD_CONTEXT_SERVER,   0,   0 },
	{ "shuffle",   "Shuffle players between teams", "shuffle",    G_TvT_Cmd_Shuffle,  CMD_CONTEXT_SERVER,   0,   0 },
	{ NULL,        NULL,                          NULL,           NULL,             0,                  0,   0 }
};

static qboolean G_TvT_Cmd_Execute(gentity_t *ent, const char *cmd, cmdContext_t context) {
	tvt_Cmd_t *c;
	unsigned int argc;

	for (c = tvt_commands; c->name; c++) {
		if (Q_stricmp(cmd, c->name)) {
			continue;
		}

		if (!(c->context & context)) {
			if (context == CMD_CONTEXT_CLIENT) {
				G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by the server.\n", cmd);
			} else {
				G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by clients.\n", cmd);
			}
			return qtrue;
		}

		argc = (unsigned int)(trap_Argc() - 1);

		if (argc < c->minArgs || (c->maxArgs > 0 && argc > c->maxArgs)) {
			G_TvT_Printf(TVT_ENT_TO_CN(ent), "Usage: %s\n", c->usage);
			return qtrue;
		}

		return c->execute(ent);
	}

	return qfalse;
}

qboolean G_TvT_ClientCommand(gentity_t *ent, const char *cmd) {
	return G_TvT_Cmd_Execute(ent, cmd, CMD_CONTEXT_CLIENT);
}

qboolean G_TvT_ConsoleCommand(const char *cmd) {
	return G_TvT_Cmd_Execute(NULL, cmd, CMD_CONTEXT_SERVER);
}
