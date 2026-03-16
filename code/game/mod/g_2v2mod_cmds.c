#include "../g_local.h"

static qboolean G_TvT_Cmd_MemStats(gentity_t *ent) {
    tvt_MemStats_t s;
    int            cn = TVT_ENT_TO_CN(ent);
    table_t       *t;
    tableRow_t    *row;

    TvT_Mem_GetStats(&s);

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Metric", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Value", ALIGN_RIGHT);

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Pool size");
    TvT_Table_SetCell(t, row, 1, va("%d bytes (%d MB)", (int)s.pool_size, (int)(s.pool_size / (1024 * 1024))));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Used");
    TvT_Table_SetCell(t, row, 1, va("%d blocks, %d bytes (%d KB)", s.used_blocks, (int)s.used_bytes, (int)(s.used_bytes / 1024)));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Largest used");
    TvT_Table_SetCell(t, row, 1, va("%d bytes", (int)s.used_largest));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Overhead");
    TvT_Table_SetCell(t, row, 1, va("%d bytes (%d per block)", (int)s.used_overhead, (int)BLOCK_OVERHEAD));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Free");
    TvT_Table_SetCell(t, row, 1, va("%d blocks, %d bytes (%d MB)", s.free_blocks, (int)s.free_bytes, (int)(s.free_bytes / (1024 * 1024))));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Largest free");
    TvT_Table_SetCell(t, row, 1, va("%d bytes", (int)s.free_largest));

    if (s.free_blocks > 1) {
        row = TvT_Table_AddRow(t);
        TvT_Table_SetCell(t, row, 0, "Fragmentation");
        TvT_Table_SetCell(t, row, 1, va("%d free segments", s.free_blocks));
        TvT_Table_SetCellColor(row, 1, S_COLOR_RED);
    }

    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);

    return qtrue;
}

static qboolean G_TvT_Cmd_Shuffle(gentity_t *ent) {
    static int   teamSelection = 0;
    int          players[MAX_CLIENTS];
    unsigned int oldRed = 0, oldBlue = 0;
    unsigned int newRed, newBlue;
    int          count = 0;
    int          redCount, blueCount, maxSize;
    int          i, sel;

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
            players[count++] = i;
        }
        else if (cl->sess.sessionTeam == TEAM_BLUE) {
            oldBlue |= (1u << i);
            players[count++] = i;
        }
        else if (cl->sess.sessionTeam == TEAM_SPECTATOR && cl->tvt.queued) {
            players[count++] = i;
        }
    }

    if (count < 3) {
        G_TvT_Printf(TVT_ENT_TO_CN(ent), "Not enough players to shuffle.\n");
        return qtrue;
    }

    maxSize = tvt_teamSize.integer;

    do {
        G_TvT_FisherYatesShuffle(players, count);

        newRed = newBlue = 0;
        redCount = blueCount = 0;
        sel = teamSelection;
        for (i = 0; i < count; i++) {
            if (sel & 1) {
                if (!maxSize || blueCount < maxSize) {
                    newBlue |= (1u << players[i]);
                    blueCount++;
                }
            }
            else {
                if (!maxSize || redCount < maxSize) {
                    newRed |= (1u << players[i]);
                    redCount++;
                }
            }
            sel ^= 1;
        }
    } while (newRed == oldRed || newRed == oldBlue);

    for (i = 0; i < count; i++) {
        if (newRed & (1u << players[i])) {
            SetTeam(&g_entities[players[i]], "red");
        }
        else if (newBlue & (1u << players[i])) {
            SetTeam(&g_entities[players[i]], "blue");
        }
        else if (level.clients[players[i]].sess.sessionTeam != TEAM_SPECTATOR) {
            SetTeam(&g_entities[players[i]], "spectator");
        }
    }

    teamSelection ^= 1;

    CheckTeamLeader(TEAM_RED);
    CheckTeamLeader(TEAM_BLUE);

    trap_SendServerCommand(-1, "cp \"Teams have been shuffled.\n\"");
    return qtrue;
}

static qboolean G_TvT_Cmd_ModCvars(gentity_t *ent) {
    int         cn = TVT_ENT_TO_CN(ent);
    tvt_Cvar_t *cvars;
    int         count;
    int         i;
    table_t    *t;
    char        search[MAX_TOKEN_CHARS];

    cvars = G_TvT_GetCvarTable(&count);

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Name", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Default", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Current", ALIGN_LEFT);

    for (i = 0; i < count; i++) {
        tableRow_t *row = TvT_Table_AddRow(t);

        TvT_Table_SetCell(t, row, 0, cvars[i].cvarName);
        TvT_Table_SetCell(t, row, 1, cvars[i].description);
        TvT_Table_SetCell(t, row, 2, cvars[i].defaultString);
        TvT_Table_SetCell(t, row, 3, cvars[i].vmCvar->string);

        if (strcmp(cvars[i].vmCvar->string, cvars[i].defaultString)) {
            TvT_Table_SetCellColor(row, 3, S_COLOR_GREEN);
        }
    }

    if (trap_Argc() > 2) {
        tvt_FilterCtx_t filter;
        trap_Argv(2, search, sizeof(search));
        filter.colName = "Name";
        filter.search = search;
        TvT_Table_Filter(t, TvT_Table_FilterSubstring, &filter);
    }

    TvT_Table_Sort(t, "Name", qtrue);
    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);

    return qtrue;
}

static qboolean G_TvT_Cmd_Queue(gentity_t *ent) {
    int        cn = TVT_ENT_TO_CN(ent);
    gclient_t *cl = ent->client;

    if (cl->sess.sessionTeam != TEAM_SPECTATOR) {
        G_TvT_Printf(cn, "You must be a spectator to use this command.\n");
        return qtrue;
    }

    cl->tvt.queued = !cl->tvt.queued;
    G_TvT_Printf(cn, "Queue status: %s\n", cl->tvt.queued ? "joined" : "left");
    return qtrue;
}

static qboolean G_TvT_Cmd_Players(gentity_t *ent) {
    int         cn = TVT_ENT_TO_CN(ent);
    table_t    *t;
    tableRow_t *row;
    int         i;

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "ID", ALIGN_RIGHT);
    TvT_Table_AddCol(t, "Name", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Queue", ALIGN_LEFT);

    for (i = 0; i < level.maxclients; i++) {
        gclient_t *cl = &level.clients[i];

        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }

        row = TvT_Table_AddRow(t);
        TvT_Table_SetCell(t, row, 0, va("%d", i));
        TvT_Table_SetCell(t, row, 1, cl->pers.netname);
        TvT_Table_SetCell(t, row, 3, cl->tvt.queued ? "^2X" : "");
    }

    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);
    return qtrue;
}

static qboolean G_TvT_Cmd_ListCommands(gentity_t *ent);

static const tvt_Cmd_t tvt_info_subcmds[] = {
    {"cvars", "Show mod cvar settings", "info cvars [filter]", G_TvT_Cmd_ModCvars, NULL, CMD_CONTEXT_ALL, 0, 1},
    {"cmds", "List available commands", "info cmds [filter]", G_TvT_Cmd_ListCommands, NULL, CMD_CONTEXT_ALL, 0, 1},
    {NULL, NULL, NULL, NULL, NULL, 0, 0, 0}};

static tvt_Cmd_t tvt_commands[] = {
    {"info", "Show mod information", "info <cvars|cmds>", NULL, tvt_info_subcmds, CMD_CONTEXT_ALL, 0, 0},
    {"mem_stats", "Show memory pool statistics", "mem_stats", G_TvT_Cmd_MemStats, NULL, CMD_CONTEXT_SERVER, 0, 0},
    {"shuffle", "Shuffle players between teams", "shuffle", G_TvT_Cmd_Shuffle, NULL, CMD_CONTEXT_SERVER, 0, 0},
    {"pstats", "Show player statistics", "pstats", G_TvT_Cmd_Stats, NULL, CMD_CONTEXT_ALL, 0, 0},
    {"queue", "Toggle queue status", "queue", G_TvT_Cmd_Queue, NULL, CMD_CONTEXT_CLIENT, 0, 0},
    {"players", "Show player list and queue status", "players", G_TvT_Cmd_Players, NULL, CMD_CONTEXT_ALL, 0, 0},
    {NULL, NULL, NULL, NULL, NULL, 0, 0, 0}};

static void G_TvT_Cmd_ListSubCommands(int clientNum, const char *parentName,
                                      const tvt_Cmd_t *subs, cmdContext_t context) {
    const tvt_Cmd_t *s;
    table_t         *t;
    tableRow_t      *row;

    G_TvT_Printf(clientNum, "Available sub-commands for '%s':\n", parentName);

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Sub-command", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Usage", ALIGN_LEFT);

    for (s = subs; s->name; s++) {
        if (s->context & context) {
            row = TvT_Table_AddRow(t);
            TvT_Table_SetCell(t, row, 0, s->name);
            TvT_Table_SetCell(t, row, 1, s->description);
            TvT_Table_SetCell(t, row, 2, s->usage);
        }
    }

    G_TvT_TablePrint(t, clientNum);
    TvT_Table_Destroy(t);
}

static qboolean G_TvT_FilterCmdTable(table_t *t, tableRow_t *row, void *ctx) {
    tvt_CmdFilterCtx_t *filter = (tvt_CmdFilterCtx_t *)ctx;
    int                 col = TvT_Table_FindCol(t, "Context");
    const char         *val;

    if (col >= 0) {
        val = row->cells[col].text;
        if (val && !(atoi(val) & filter->context)) {
            return qfalse;
        }
    }

    if (filter->search) {
        return TvT_Table_FilterSubstring(t, row, filter->search);
    }

    return qtrue;
}

static table_t *G_TvT_GetCmdTable(void) {
    static table_t  *cmdTable = NULL;
    const tvt_Cmd_t *c;
    const tvt_Cmd_t *s;
    tableRow_t      *row;
    int              ctxCol;

    if (cmdTable) {
        return cmdTable;
    }

    cmdTable = TvT_Table_Create();
    TvT_Table_AddCol(cmdTable, "Command", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Usage", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Context", ALIGN_LEFT);
    TvT_Table_HideCol(cmdTable, "Context", qtrue);

    ctxCol = TvT_Table_FindCol(cmdTable, "Context");

    for (c = tvt_commands; c->name; c++) {
        if (c->subCommands) {
            for (s = c->subCommands; s->name; s++) {
                row = TvT_Table_AddRow(cmdTable);
                TvT_Table_SetCell(cmdTable, row, 0, va("%s %s", c->name, s->name));
                TvT_Table_SetCell(cmdTable, row, 1, s->description);
                TvT_Table_SetCell(cmdTable, row, 2, s->usage);
                TvT_Table_SetCell(cmdTable, row, ctxCol, va("%d", s->context));
            }
        }
        else {
            row = TvT_Table_AddRow(cmdTable);
            TvT_Table_SetCell(cmdTable, row, 0, c->name);
            TvT_Table_SetCell(cmdTable, row, 1, c->description);
            TvT_Table_SetCell(cmdTable, row, 2, c->usage);
            TvT_Table_SetCell(cmdTable, row, ctxCol, va("%d", c->context));
        }
    }

    TvT_Table_Sort(cmdTable, "Command", qtrue);
    return cmdTable;
}

static qboolean G_TvT_Cmd_ListCommands(gentity_t *ent) {
    int                cn = TVT_ENT_TO_CN(ent);
    table_t           *t = G_TvT_GetCmdTable();
    tvt_CmdFilterCtx_t cmdFilter;
    tvt_FilterCtx_t    searchFilter;
    char               search[MAX_TOKEN_CHARS];

    cmdFilter.context = ent ? CMD_CONTEXT_CLIENT : CMD_CONTEXT_SERVER;
    cmdFilter.search = NULL;

    if (trap_Argc() > 2) {
        trap_Argv(2, search, sizeof(search));
        searchFilter.colName = "Command";
        searchFilter.search = search;
        cmdFilter.search = &searchFilter;
    }

    TvT_Table_Filter(t, G_TvT_FilterCmdTable, &cmdFilter);
    G_TvT_TablePrint(t, cn);
    TvT_Table_Filter(t, NULL, NULL);

    return qtrue;
}

static qboolean G_TvT_Cmd_Execute(gentity_t *ent, const char *cmd, cmdContext_t context) {
    tvt_Cmd_t   *c;
    unsigned int argc;

    for (c = tvt_commands; c->name; c++) {
        if (Q_stricmp(cmd, c->name)) {
            continue;
        }

        if (!(c->context & context)) {
            if (context == CMD_CONTEXT_CLIENT) {
                G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by the server.\n", cmd);
            }
            else {
                G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by clients.\n", cmd);
            }
            return qtrue;
        }

        if (c->subCommands) {
            char             subcmd[MAX_TOKEN_CHARS];
            const tvt_Cmd_t *s;
            int              cn = TVT_ENT_TO_CN(ent);

            if (trap_Argc() < 2) {
                G_TvT_Cmd_ListSubCommands(cn, c->name, c->subCommands, context);
                return qtrue;
            }

            trap_Argv(1, subcmd, sizeof(subcmd));

            for (s = c->subCommands; s->name; s++) {
                if (Q_stricmp(subcmd, s->name)) {
                    continue;
                }

                if (!(s->context & context)) {
                    if (context == CMD_CONTEXT_CLIENT) {
                        G_TvT_Printf(cn, "Command '%s %s' can only be used by the server.\n", cmd, subcmd);
                    }
                    else {
                        G_TvT_Printf(cn, "Command '%s %s' can only be used by clients.\n", cmd, subcmd);
                    }
                    return qtrue;
                }

                argc = (unsigned int)(trap_Argc() - 2);

                if (argc < s->minArgs || (s->maxArgs > 0 && argc > s->maxArgs)) {
                    G_TvT_Printf(cn, "Usage: %s\n", s->usage);
                    return qtrue;
                }

                return s->execute(ent);
            }

            G_TvT_Printf(cn, "Unknown sub-command '%s'.\n", subcmd);
            G_TvT_Cmd_ListSubCommands(cn, c->name, c->subCommands, context);
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
