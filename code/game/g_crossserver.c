
#include "g_local.h"
#include "g_defrag.h"
#include "g_dbcmds.h"

static void G_CrossServerChatAll() {
	int i;
	gentity_t* other;
	char	name[MAX_TOKEN_CHARS];
	char	text[MAX_TOKEN_CHARS];
	int thelen;
	Q_strncpyz(name, "(cross-server) ", sizeof(name));
	thelen = strlen(name);
	trap_Argv(3, name + thelen, sizeof(name) - thelen);
	trap_Argv(4, text, sizeof(text));
	// send it to all the apropriate clients
	for (i = 0; i < level.maxclients; i++) {
		other = &g_entities[i];
		G_SayTo(NULL, other, SAY_ALL, COLOR_GREEN, name, text, " crossServer");
	}
}

qboolean G_CrossServerCommand() {

	char	cmd[MAX_TOKEN_CHARS];
	int i, argc;
	char token[BIG_INFO_STRING]; // As the engine uses Cmd_TokenizeString2 a single parameter is theoretically not limited by MAX_TOKEN_CHARS, but by BIG_INFO_STRING

	// Filter '\n' and '\r'
	argc = trap_Argc();
	for (i = 0; i < argc; i++)
	{
		trap_Argv(i, token, sizeof(token));
		if (strchr(token, '\n') || strchr(token, '\r'))
		{
			G_Printf("G_CrossServerCommand: got an invalid command - command blocked.\n");
			return;
		}
	}

	//trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(2, cmd, sizeof(cmd)); // 0 is serverident and 1 is sv_hostname
	G_Printf("G_CrossServerCommand: received %s.\n",cmd);
	if (!Q_stricmp(cmd, "chatAll")) {
		G_CrossServerChatAll();
		return qtrue;
	}
	return qfalse;
}

void G_SendCrossServerCommand(const char* cmd) {
	if (!(coolApi & COOL_APIFEATURE_CROSS_SERVER_COMMANDS)) {
		return;
	}
	trap_G_COOL_API_CrossServerCommand(cmd);
}
