#include "g_local.h"
#include "g_dbcmds.h"
#include "../qcommon/levenshtein.h"

typedef enum userMessageType_s {
	MESSAGE_MESSAGE,
	MESSAGE_NOTE,
} userMessageType_T;


void G_SendUserMessageFinished(int status, const char* errorMessage, int affectedRows) {
	gentity_t* ent = NULL;
	userMessageSendStruct_t data;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateMessagesTable();
		trap_SendServerCommand(data.clientnum, "print \"^1User message sending failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d user message sending user search returned, user no longer valid.\n", data.clientnum);
		return;
	}
	else if (status) {
		trap_SendServerCommand(data.clientnum, va("print \"^1User message sending failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}


	trap_SendServerCommand(data.clientnum, va("print \"^2Message successfully sent to %s.\n\"", data.userName));
}

#define TIMEFRAME_ANTISPAM_COUNT 5
void G_SendUserMessageContinue(int status, const char* errorMessage, int affectedRows) {
	gentity_t* ent = NULL;
	userMessageSendStruct_t data;
	static const int pastTimeframes[TIMEFRAME_ANTISPAM_COUNT] = { 1,10,60,360,1440 }; // 1 minute, 10 minutes, 60 minutes, 6 hours, 1 day
	static const int pastTimeframeLimits[TIMEFRAME_ANTISPAM_COUNT] = { 2,5,15,30,50 };
	int pastTimeframeCounts[TIMEFRAME_ANTISPAM_COUNT] = { 0,0,0,0,0 };

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateMessagesTable();
		trap_SendServerCommand(data.clientnum, "print \"^1User message sending failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d user message sending user search returned, user no longer valid.\n", data.clientnum);
		return;
	}
	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(data.clientnum, "print \"^1Message sending failed. Database version too low.\n\"");
		return;
	}
	if (!G_COOL_API_DB_GetMoreResults(NULL) || !G_COOL_API_DB_NextRow())
	{
		trap_SendServerCommand(data.clientnum, va("print \"^1Recipient '%s' not found.\n\"",data.userName));
		return;
	}
	data.recipientId = G_COOL_API_DB_GetInt(0);
	if (!G_COOL_API_DB_GetMoreResults(NULL))
	{
		trap_SendServerCommand(data.clientnum, va("print \"^1Failed to check previous messages to '%s'.\n\"",data.userName));
		return;
	}
	while (G_COOL_API_DB_NextRow()) { // checking old messages to this user to detect spam
		int contains = G_COOL_API_DB_GetInt(0);
		int minutesSince = G_COOL_API_DB_GetInt(2);
		int i;
		for (i = 0; i < TIMEFRAME_ANTISPAM_COUNT; i++) {
			if (minutesSince <= pastTimeframes[i]) {
				pastTimeframeCounts[i]++;
			}
			if (pastTimeframeCounts[i] >= pastTimeframeLimits[i]) {
				trap_SendServerCommand(data.clientnum, va("print \"^1You can only send up to %d messages in a %d minute timeframe. Anti-spam blocked your message.\n\" antispam_lim", pastTimeframeLimits[i], pastTimeframes[i]));
			}
		}
		if (contains) {
			// quick check for simple repetition
			trap_SendServerCommand(data.clientnum, va("print \"^1You have already sent a very similar message to '%s' recently. Anti-spam blocked your message.\n\" antispam_cont", data.userName));
			return;
		}
		else {
			char msg[MAX_STRING_CHARS];
			int len1 = strlen(data.message), len2, diff, maxLen;//,lendiff;
			// do a better check
			G_COOL_API_DB_GetString(1, msg, sizeof(msg)); 
			//lendiff = strlen(msg) - len1;
			//if (lendiff < (len1/2)) {
			//}
			len2 = strlen(msg);
			maxLen = MAX(len1, len2);
			diff = levenshtein(msg, data.message);
			if (diff <= (maxLen/3)) {
				trap_SendServerCommand(data.clientnum, va("print \"^1You have already sent a similar message to '%s' recently. Anti-spam blocked your message.\n\" antispam_lev", data.userName));
				return;
			}
		}
	}

	// do the actual sending

	// find target user.
	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_SENDUSERMESSAGE_ACTUAL, 
		"INSERT INTO messages "
		"(sender,recipient,content,created,sentfrom,type,flags) "
		"VALUES "
		"(?,?,?,NOW(),?,?,?) "
	)) {
		G_SendServerCommand(ent - g_entities, "print \"DB connection not available to send user messages (2).\n\"", qfalse);
		return;
	}

	G_COOL_API_DB_PreparedBindInt(data.senderId);
	G_COOL_API_DB_PreparedBindInt(data.recipientId);
	G_COOL_API_DB_PreparedBindString(data.message);
	G_COOL_API_DB_PreparedBindString(DF_GetCourseName(qfalse));
	G_COOL_API_DB_PreparedBindInt((int)MESSAGE_MESSAGE);
	G_COOL_API_DB_PreparedBindInt(0);
	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

void G_Cmd_SendUserMessage(gentity_t* ent) {
	static userMessageSendStruct_t data;
	int i,argc = trap_Argc();
	const char* coursename = NULL;
	if (coolApi_dbVersion < 3) {
		G_SendServerCommand(ent - g_entities, "print \"DB version too low to send user messages.\n\"", qfalse);
		return;
	}
	if (!ent->client->sess.login.loggedIn) {
		G_SendServerCommand(ent - g_entities, "print \"Can't send user messages unless logged in.\n\"", qfalse);
		return;
	}
	if (argc < 4) {
		G_SendServerCommand(ent - g_entities, "print \"Usage: messages send <recipient> <message>.\n\"", qfalse);
		return;
	}

	memset(&data, 0, sizeof(data));
	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));
	data.senderId = ent->client->sess.login.id;

	trap_Argv(2, data.userName, sizeof(data.userName));
	trap_Argv(3, data.message, sizeof(data.message));
	for (i = 4; i < argc; i++) {
		static char extra[MAX_STRING_CHARS];
		// concat extra stuff
		trap_Argv(i, extra, sizeof(extra));
		Q_strcat(data.message, sizeof(data.message), va(" %s",extra));
	}

	if (!strlen(data.userName) || !strlen(data.message)) {
		G_SendServerCommand(ent - g_entities, "print \"Recipient and message cannot be empty.\n\"", qfalse);
		return;
	}

	// find target user.
	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_SENDUSERMESSAGE, 
		"SET @userid=NULL,@message=?;"
		"SELECT @userid := id FROM users WHERE username=?;"
		"SELECT "
		"instr(messages.content,@message) AS contains " // +instr(@message,messages.content) // let's not do that or else we'd be checking if an old message is contained in a new one. so if an old message was a single common word, it would block almost anything coming after
		",content "
		", TIMESTAMPDIFF(MINUTE,messages.created,NOW()) AS minutesSince "
		"FROM messages WHERE recipient=@userid AND sender=? AND created > (DATE(NOW()) - INTERVAL 7 DAY) LIMIT 20;"
	)) {
		G_SendServerCommand(ent - g_entities, "print \"DB connection not available to send user messages.\n\"", qfalse);
		return;
	}

	G_COOL_API_DB_PreparedBindString(data.message);
	G_COOL_API_DB_PreparedBindString(data.userName);
	G_COOL_API_DB_PreparedBindInt(data.senderId);
	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

void G_ListUserMessagesListContinue(int status, const char* errorMessage, int affectedRows) {
	gentity_t* ent = NULL;
	int i;
	userMessagesListStruct_t data;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateMessagesTable();
		trap_SendServerCommand(data.clientnum, "print \"^1User message sending failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d user message sending user search returned, user no longer valid.\n", data.clientnum);
		return;
	}
	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(data.clientnum, "print \"^1Message sending failed. Database version too low.\n\"");
		return;
	}
	trap_SendServerCommand(data.clientnum, va("print \"^7Messages involving you, page %d:\n\"",data.page));
	trap_SendServerCommand(data.clientnum, "print \"^2-------------------------------------------------------------\n\"");
	i = 0;
	while (G_COOL_API_DB_NextRow())
	{
		//  SELECT users1.username AS sendername, users2.username as recipientname, created, sentfrom, content
		char	sender[USERNAME_MAX_LEN+1];
		char	recipient[USERNAME_MAX_LEN+1];
		char	msg[MAX_STRING_CHARS];
		char	when[25];
		char	sentfrom[COURSENAME_MAX_LEN+1];
		qboolean	unread = G_COOL_API_DB_GetInt(5);
		int		recipientId = G_COOL_API_DB_GetInt(7);

		if (i > 0) {
			trap_SendServerCommand(data.clientnum, "print \"^3---------------------------------------\n\"");
		}

		// do the actual sending
		G_COOL_API_DB_GetString(0, sender, sizeof(sender));
		G_COOL_API_DB_GetString(1, recipient, sizeof(recipient));
		G_COOL_API_DB_GetString(2, when, sizeof(when));
		G_COOL_API_DB_GetString(3, sentfrom, sizeof(sentfrom));
		G_COOL_API_DB_GetString(4, msg, sizeof(msg));

		trap_SendServerCommand(data.clientnum, va("print \"^7From: %10s, To: %10s, Time: %s, Sent from: %s\n\"", sender, recipient, when, sentfrom));
		trap_SendServerCommand(data.clientnum, va("print \"^7%s\n\"", msg));

		if (unread && recipientId == ent->client->sess.login.id) { // we want to auto-prune old messages so remember when they were read
			int	id = G_COOL_API_DB_GetInt(6);
			if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_LISTUSERMESSAGES_UPDATEREAD,
				"UPDATE messages SET readtime=NOW() WHERE id=? AND readtime IS NULL"
			)) {
				if (g_developer.integer) {
					G_SendServerCommand(ent - g_entities, "print \"DB connection not available to update user messages read time.\n\"", qfalse);
				}
				return;
			}
			else {
				G_COOL_API_DB_PreparedBindInt(id);
				G_COOL_API_DB_FinishAndSendPreparedStatement();
			}
		}
		i++;
	}
	trap_SendServerCommand(data.clientnum, "print \"^2-------------------------------------------------------------\n\"");

}

void G_Cmd_ListUserMessages(gentity_t* ent) {
	static userMessagesListStruct_t data;
	int start = 0;
	const char* coursename = NULL;
	if (coolApi_dbVersion < 3) {
		G_SendServerCommand(ent - g_entities, "print \"DB version too low to send user messages.\n\"", qfalse);
		return;
	}
	if (!ent->client->sess.login.loggedIn) {
		G_SendServerCommand(ent - g_entities, "print \"Can't list user messages unless logged in.\n\"", qfalse);
		return;
	}
	//if (trap_Argc() < 4) {
	//	G_SendServerCommand(ent - g_entities, "print \"Usage: messages list\n\"", qfalse);
	//	return;
	//}

	memset(&data, 0, sizeof(data));
	data.clientnum = ent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));

	// find target user.
	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_LISTUSERMESSAGES, 
		"SELECT users1.username AS sendername, users2.username as recipientname, "
		"messages.created, sentfrom, content,(messages.readtime IS NULL) AS unread,messages.id,recipient "
		"FROM messages "
		"LEFT JOIN users AS users1 ON users1.id=sender "
		"LEFT JOIN users AS users2 ON users2.id=recipient "
		"WHERE (sender=? OR recipient=?) "
		"ORDER BY created DESC "
		"LIMIT ?,10;"
		"UPDATE messages SET readtime=IF(readtime IS NULL,NOW(),readtime)"
	)) {
		G_SendServerCommand(ent - g_entities, "print \"DB connection not available to list user messages.\n\"", qfalse);
		return;
	}

	if (trap_Argc() > 2) {
		char arg[10];
		trap_Argv(2, arg, sizeof(arg));
		data.page = atoi(arg);
		start = data.page * 10;
	}

	G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
	G_COOL_API_DB_PreparedBindInt(ent->client->sess.login.id);
	G_COOL_API_DB_PreparedBindInt(start);
	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

void G_Cmd_UserMessages(gentity_t* ent) {
	char arg[10];
	trap_SendServerCommand(ent - g_entities,"print \"^1MESSAGING SYSTEM IMPORTANT WARNING: ^3Do not use this messaging system for anything private. Messages are stored in plaintext. After reading, messages are ^3permanently deleted ^3after 30 days, so don't rely on them remaining available forever.\n\"");
	if (trap_Argc() < 2) {
		G_SendServerCommand(ent - g_entities, "print \"Usage: messages <send|list> [recipient or page number] [message if sending].\n\"", qfalse);
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if (!Q_stricmp(arg,"send")) {
		G_Cmd_SendUserMessage(ent);
	} else if (!Q_stricmp(arg,"list")) {
		G_Cmd_ListUserMessages(ent);
	}
}

// prune old usermessages that are already read.
// keep up to 1 month
void G_UserMessagesPrune() {
	userMessagesPruneStruct_t data;
	// find target user.
	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data, sizeof(data), DBREQUEST_PRUNEUSERMESSAGES,
		"DELETE FROM messages WHERE readtime IS NOT NULL AND readtime < (DATE(NOW()) - INTERVAL 30 DAY) AND created < (DATE(NOW()) - INTERVAL 30 DAY)"
	)) {
		if (g_developer.integer) {
			Com_Printf("print \"DB connection not available to prune user messages.\n\"", qfalse);
		}
		return;
	}
	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

