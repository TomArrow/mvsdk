
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

	G_LogPrintf("say(cross-server): %s: %s\n", name, text);
	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (i = 0; i < level.maxclients; i++) {
		other = &g_entities[i];
		G_SayTo(NULL, other, SAY_ALL, COLOR_GREEN, name, text, " crossServer");
	}
}
static void G_CrossServerPrint() {
	char	firstArg[MAX_TOKEN_CHARS];
	const char* reprint;
	trap_Argv(3,firstArg,sizeof(firstArg));
	reprint = va("print \"cross-server: %s\" %s", firstArg,ConcatArgsQuoted(4));
	trap_SendServerCommand(-1, reprint);
}

qboolean G_CrossServerCommand() {

	char	cmd[MAX_TOKEN_CHARS];
	int i, argc;
	//char token[BIG_INFO_STRING]; // As the engine uses Cmd_TokenizeString2 a single parameter is theoretically not limited by MAX_TOKEN_CHARS, but by BIG_INFO_STRING

	// Filter '\n' and '\r'
	//argc = trap_Argc();
	//for (i = 0; i < argc; i++)
	//{
	//	trap_Argv(i, token, sizeof(token));
	//	if (strchr(token, '\n') || strchr(token, '\r'))
	//	{
	//		G_Printf("G_CrossServerCommand: got an invalid command - command blocked.\n");
	//		return;
	//	}
	//}

	//trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(2, cmd, sizeof(cmd)); // 0 is serverident and 1 is sv_hostname
	G_Printf("G_CrossServerCommand: received %s.\n",cmd);
	if (!Q_stricmp(cmd, "chatAll") && g_crossServerChat.integer) {
		G_CrossServerChatAll();
		return qtrue;
	} else if (!Q_stricmp(cmd, "defragPrint") && g_crossServerDefragTimes.integer) {
		G_CrossServerPrint();
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
