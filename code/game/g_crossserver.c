
#include "g_local.h"
#include "g_defrag.h"
#include "g_dbcmds.h"

static void G_CrossServerChatAll() {
	int i;
	gentity_t* other;
	char	ourIdent[MAX_TOKEN_CHARS];
	char	ourHostname[MAX_TOKEN_CHARS];
	char	source[MAX_TOKEN_CHARS];
	char	name[MAX_TOKEN_CHARS];
	char	text[MAX_TOKEN_CHARS];
	int thelen;
	trap_Cvar_VariableStringBuffer("sv_crossServerCommandIdent", ourIdent, sizeof(ourIdent));
	trap_Cvar_VariableStringBuffer("sv_hostname", ourHostname, sizeof(ourHostname));
	trap_Argv(0, source, sizeof(source)); // 0 is ident
	if (!source[0] || !Q_stricmp(ourIdent,source)) { // use hostname if ident is identical to ours
		trap_Argv(1, source, sizeof(source)); // 1 is sv_hostname
	}
	if (!source[0] || !Q_stricmp(source,"noname") || !Q_stricmp(source, ourHostname)) { // use mapname if server has no name or is identical to ours
		trap_Argv(5, source, sizeof(source)); // 5 is mapname
	}

	if (!source[0]) {
		// huh..
		Q_strncpyz(name, "^l>^jCROSS-SERVER^l>^7: ", sizeof(name));
	}
	else {
		Q_StripColor(source);
		Com_sprintf(name, sizeof(name), "^l>^j%s^l>^7: ", source);
	}
	thelen = strlen(name);
	trap_Argv(3, name + thelen, sizeof(name) - thelen);
	trap_Argv(4, text, sizeof(text));

	G_LogPrintf("say(cross-server from %s): %s: %s\n",source, name, text);
	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("(cross-server from %s) %s%s\n", source, name, text);
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
	//reprint = va("print \"cross-server: %s\" %s", firstArg,ConcatArgsQuoted(4));
	reprint = va("print \"%s\" %s", firstArg,ConcatArgsQuoted(4));
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
