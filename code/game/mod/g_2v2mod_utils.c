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
