#include "../g_local.h"

void G_TvT_SyncReadyMask(void) {
    int i;

    for (i = 0; i < g_maxclients.integer; i++) {
        if (level.clients[i].pers.connected == CON_CONNECTED) {
            level.clients[i].ps.stats[STAT_CLIENTS_READY] = level.tvt.match.readyMask;
        }
    }
}

qboolean G_TvT_CheckReadyUp(void) {
    int        i, ready, notReady, redCount, blueCount;
    gclient_t *cl;

    if (!tvt_matchMode.integer) {
        return qfalse;
    }

    if (level.tvt.match.matchInProgress) {
        return qfalse;
    }

    ready = 0;
    notReady = 0;
    redCount = 0;
    blueCount = 0;
    for (i = 0; i < g_maxclients.integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (g_entities[i].r.svFlags & SVF_BOT) {
            continue;
        }
        if (cl->sess.sessionTeam == TEAM_RED) {
            redCount++;
        }
        else if (cl->sess.sessionTeam == TEAM_BLUE) {
            blueCount++;
        }
        else {
            continue;
        }
        if (i < 16 && (level.tvt.match.readyMask & (1 << i))) {
            ready++;
        }
        else {
            notReady++;
        }
    }

    if (level.tvt.match.restartPending) {
        if (notReady > 0) {
            level.tvt.match.restartPending = qfalse;
            trap_SetConfigstring(CS_WARMUP, va("%i", -1));
        }
        else if (level.time >= level.tvt.match.restartTime + 5000) {
            level.tvt.match.readyMask = 0;
            level.tvt.match.restartPending = qfalse;
            G_TvT_SyncReadyMask();
            trap_SetConfigstring(CS_WARMUP, "");
            // Set session flag here rather than in G_WriteSessionData because
            // the session functions can't distinguish a ready-up restart from
            // a manual map_restart.
            // TODO: Kind of a dirty hack, adjust this perhaps in the future
            trap_Cvar_Set("sessiontvt", va("%i %i", level.tvt.match.playedLastRound, 1));
            trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
            level.restarted = qtrue;
        }
        return qtrue;
    }

    if (ready > 0 && notReady == 0 && redCount > 0 && blueCount > 0) {
        level.tvt.match.restartPending = qtrue;
        level.tvt.match.restartTime = level.time;
        trap_SetConfigstring(CS_WARMUP, va("%i", level.time + 5000));
        trap_SendServerCommand(-1, "cp \"All players ready!\n\"");
    }
    else if (redCount + blueCount > 0) {
        trap_SetConfigstring(CS_WARMUP, va("%i", -1));
    }

    return qtrue;
}

void G_TvT_Init(void) {
    G_TvT_RegisterCvars();
    G_TvT_Vote_Init();
}
