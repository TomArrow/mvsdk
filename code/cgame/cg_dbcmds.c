
#include "cg_dbcmds.h"
#include "cg_local.h"

static void CG_DB_GetChatsResponse(int status) {
	char			text[MAX_STRING_CHARS] = { 0 };
	char 			time[50] = { 0 };
	if (status) {
		Com_Printf("Getting chats failed with status %d.\n", status);
		return;
	}
	Com_Printf("^2Recent chats:\n");
	while (trap_CG_COOL_API_DB_NextRow()) {
		
		int id = trap_CG_COOL_API_DB_GetInt(0);
		trap_CG_COOL_API_DB_GetString(1, text,sizeof(text));
		trap_CG_COOL_API_DB_GetString(2, time,sizeof(time));
		Com_Printf("^2%d ^7[%s] %s\n",id, time, text);
	}
}

void CG_DB_CheckResponses() {

	if (coolApi_dbVersion) {
		int requestType;
		int status;
		while (trap_CG_COOL_API_DB_NextResponse(&requestType, NULL, &status, NULL, 0, NULL, 0)) {
			switch (requestType) {
				default:
					Com_Printf("DB Request of type %d returned with status %d.\n", requestType, status);
					break;
				case DBREQUEST_GETCHATS:
					CG_DB_GetChatsResponse(status);
					break;
			}
		}
	}
}

void CG_DB_InsertChat(const char* chatText) {
	char		text[MAX_STRING_CHARS] = { 0 };
	const char* request;

	if (!coolApi_dbVersion || cg.demoPlayback) return;

	// save it to db
	Q_strncpyz(text, chatText, sizeof(text));
	if (trap_CG_COOL_API_DB_EscapeString(text, sizeof(text))) {
		request = va("INSERT INTO chats (chat,`time`) VALUES ('%s',NOW())", text);
		trap_CG_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_CHATSAVE, request);
	}
}

void CG_DB_GetChats_f(void) {
	int clientNum = -1;

	if (!coolApi_dbVersion) {
		CG_Printf("getchats not possible, DB API not available\n");
		return;
	}

	trap_CG_COOL_API_DB_AddRequest(NULL,0, DBREQUEST_GETCHATS, "SELECT id, chat, `time` FROM chats ORDER BY time DESC LIMIT 0,10");
}
