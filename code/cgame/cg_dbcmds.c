
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
	while (CG_COOL_API_DB_NextRow()) {
		
		int id = CG_COOL_API_DB_GetInt(0);
		CG_COOL_API_DB_GetString(1, text,sizeof(text));
		CG_COOL_API_DB_GetString(2, time,sizeof(time));
		Com_Printf("^2%d ^7[%s] %s\n",id, time, text);
	}
}

void CG_DB_CheckResponses() {

	if (coolApi_dbVersion) {
		int requestType;
		int status;
		while (CG_COOL_API_DB_NextResponse(&requestType, NULL, &status, NULL, 0, NULL, 0)) {
			switch (requestType) {
				default:
					if (status) {
						Com_Printf("DB Request of type %d failed with status %d.\n", requestType, status);
					}
					else {
						if (cg_developer.integer) {
							Com_Printf("DB Request of type %d returned with status %d.\n", requestType, status);
						}
					}
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
	if (CG_COOL_API_DB_EscapeString(text, sizeof(text))) {
		request = va("INSERT INTO chats (chat,`time`) VALUES ('%s',NOW())", text);
		CG_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_CHATSAVE, request);
	}
}

void CG_DB_GetChats_f(void) {
	int clientNum = -1;
	int page, first;

	if (!coolApi_dbVersion) {
		CG_Printf("getchats not possible, DB API not available\n");
		return;
	}

	page = atoi(CG_Argv(1))-1;
	page = MAX(page,0);
	first = page*10;

	CG_COOL_API_DB_AddRequest(NULL,0, DBREQUEST_GETCHATS, va("SELECT id, chat, `time` FROM chats ORDER BY time DESC, id DESC LIMIT %d,10",first));
}









qboolean	trap_CG_COOL_API_DB_EscapeString(char* input, int size);
qboolean	trap_CG_COOL_API_DB_AddRequest(byte* reference, int referenceLength, int requestType, const char* request);
qboolean	trap_CG_COOL_API_DB_AddRequestTyped(byte* reference, int referenceLength, int requestType, const char* request, DBRequestType_t dbRequestType);
qboolean	trap_CG_COOL_API_DB_NextResponse(int* requestType, int* affectedRows, int* status, char* errorMessage, int errorMessageSize, byte* reference, int referenceLength);
qboolean	trap_CG_COOL_API_DB_GetReference(byte* reference, int referenceLength);
qboolean	trap_CG_COOL_API_DB_NextRow();
int			trap_CG_COOL_API_DB_GetInt(int place);
void		trap_CG_COOL_API_DB_GetFloat(int place, float* value);
qboolean	trap_CG_COOL_API_DB_GetString(int place, char* out, int outSize);

qboolean	trap_CG_COOL_API_DB_AddPreparedStatement(byte* reference, int referenceLength, int requestType, const char* request);
qboolean	trap_CG_COOL_API_DB_PreparedBindString(const char* string);
qboolean	trap_CG_COOL_API_DB_PreparedBindFloat(float number);
qboolean	trap_CG_COOL_API_DB_PreparedBindInt(int number);
qboolean	trap_CG_COOL_API_DB_PreparedBindBinary(byte* data, int dataLength);
qboolean	trap_CG_COOL_API_DB_FinishAndSendPreparedStatement();
int			trap_CG_COOL_API_DB_GetBinary(int place, byte* out, int outSize);
qboolean	trap_CG_COOL_API_DB_PreparedBindNull();
qboolean	trap_CG_COOL_API_DB_GetMoreResults(int* affectedRows);

qboolean	CG_COOL_API_DB_EscapeString(char* input, int size) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_EscapeString(input, size);
}
qboolean	CG_COOL_API_DB_AddRequest(byte* reference, int referenceLength, int requestType, const char* request) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_AddRequest(reference, referenceLength, requestType, request);
}
qboolean	CG_COOL_API_DB_AddRequestTyped(byte* reference, int referenceLength, int requestType, const char* request, DBRequestType_t dbRequestType) {
	if (coolApi_dbVersion < 2) return qfalse;
	return trap_CG_COOL_API_DB_AddRequestTyped(reference, referenceLength, requestType, request, (int)dbRequestType);
}
qboolean	CG_COOL_API_DB_NextResponse(int* requestType, int* affectedRows, int* status, char* errorMessage, int errorMessageSize, byte* reference, int referenceLength) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_NextResponse(requestType, affectedRows, status, errorMessage, errorMessageSize, reference, referenceLength);
}
qboolean	CG_COOL_API_DB_GetReference(byte* reference, int referenceLength) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_GetReference(reference, referenceLength);
}
qboolean	CG_COOL_API_DB_NextRow() {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_NextRow();
}
int			CG_COOL_API_DB_GetInt(int place) {
	if (!coolApi_dbVersion) return 0;
	return trap_CG_COOL_API_DB_GetInt(place);
}
void		CG_COOL_API_DB_GetFloat(int place, float* value) {
	if (!coolApi_dbVersion) {
		*value = 0;
		return;
	}
	trap_CG_COOL_API_DB_GetFloat(place, value);
}
qboolean	CG_COOL_API_DB_GetString(int place, char* out, int outSize) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_CG_COOL_API_DB_GetString(place, out, outSize);
}

// dbApi v3

qboolean	CG_COOL_API_DB_AddPreparedStatement(byte* reference, int referenceLength, int requestType, const char* request) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_AddPreparedStatement(reference, referenceLength, requestType, request);
}
qboolean	CG_COOL_API_DB_PreparedBindString(const char* string) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_PreparedBindString(string);
}
qboolean	CG_COOL_API_DB_PreparedBindFloat(float number) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_PreparedBindFloat(number);
}
qboolean	CG_COOL_API_DB_PreparedBindInt(int number) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_PreparedBindInt(number);
}
qboolean	CG_COOL_API_DB_PreparedBindBinary(byte* data, int dataLength) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_PreparedBindBinary(data, dataLength);
}
qboolean	CG_COOL_API_DB_FinishAndSendPreparedStatement() {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_FinishAndSendPreparedStatement();
}
int			CG_COOL_API_DB_GetBinary(int place, byte* out, int outSize) {
	if (coolApi_dbVersion < 3) return 0;
	return trap_CG_COOL_API_DB_GetBinary(place, out, outSize);
}
qboolean	CG_COOL_API_DB_PreparedBindNull() {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_PreparedBindNull();
}
qboolean	CG_COOL_API_DB_GetMoreResults(int* affectedRows) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_CG_COOL_API_DB_GetMoreResults(affectedRows);
}
