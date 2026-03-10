#include "../g_local.h"

static qboolean TvT_Cmd_MemStats(gentity_t *ent) {
	TvT_MemStats_t s;
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

static tvt_cmd_t tvt_commands[] = {
	{ "mem_stats", "Show memory pool statistics", "mem_stats",    TvT_Cmd_MemStats, CMD_CONTEXT_SERVER,   0,   0 },
	{ NULL,        NULL,                          NULL,           NULL,             0,                  0,   0 }
};

static qboolean G_TvT_Cmd_Execute(gentity_t *ent, const char *cmd, cmdContext_t context) {
	tvt_cmd_t *c;
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
