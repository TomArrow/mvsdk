
#include "g_dbcmds.h"
#include "g_local.h"
#include "../qcommon/crypt_blowfish.h"

static void G_CreateUserTable();

static void G_DB_GetChatsResponse(int status) {
	char			text[MAX_STRING_CHARS] = { 0 };
	char 			time[50] = { 0 };
	if (status) {
		Com_Printf("Getting chats failed with status %d.\n", status);
		return;
	}
	Com_Printf("^2Recent chats:\n");
	while (trap_G_COOL_API_DB_NextRow()) {
		
		int id = trap_G_COOL_API_DB_GetInt(0);
		trap_G_COOL_API_DB_GetString(1, text,sizeof(text));
		trap_G_COOL_API_DB_GetString(2, time,sizeof(time));
		Com_Printf("^2%d ^7[%s] %s\n",id, time, text);
	}
}

static void G_RegisterContinue(loginRegisterStruct_t* loginData) {
	static char		cleanUsername[MAX_STRING_CHARS];
	static char		cleanPassword[MAX_STRING_CHARS];
	const char*		request = NULL;

	Q_strncpyz(cleanUsername,loginData->username,sizeof(cleanUsername));
	Q_strncpyz(cleanPassword,loginData->password,sizeof(cleanPassword));
	if (!trap_G_COOL_API_DB_EscapeString(cleanUsername, sizeof(cleanUsername)) || !trap_G_COOL_API_DB_EscapeString(cleanPassword, sizeof(cleanPassword))) {
		Com_Printf("G_RegisterContinue: EscapeString failed.\n");
		return;
	}

	request = va("INSERT INTO users (username,password,created) VALUES ('%s','%s',NOW())", cleanUsername, cleanPassword);

	// check if user already exists
	trap_G_COOL_API_DB_AddRequest((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER, request);

}

static void G_RegisterResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		Com_Printf("register failed due to usertable not existing. Attempting to create. Please try again shortly.\n");
		return;
	}
	else if (status == 1062) {
		Com_Printf("A user with this name already exists.\n");
		return;
	}
	else if (status) {
		Com_Printf("register failed with status %d and error message %s.\n", status, errorMessage);
		return;
	}
	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));
	Com_Printf("Registration successful. You can now log in as '%s'.\n", loginData.username);

}
typedef int ip_t[4];
static gentity_t* DB_VerifyClient(int clientNum, ip_t ip) {
	gentity_t* ent;
	ent = g_entities + clientNum;

	if (!ent->client) {
		Com_Printf("DB_VerifyClient: client no longer valid.\n");
		return NULL;
	}

	if (memcmp(ip, mv_clientSessions[clientNum].clientIP, sizeof(ip))) {
		Com_Printf("DB_VerifyClient: no longer the same client.\n");
		return NULL;
	}
	return ent;
}

static void G_LoginFetchDataResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	static char password[MAX_STRING_CHARS];
	gentity_t* ent = NULL;
	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		Com_Printf("login failed due to usertable not existing. Attempting to create. Please try again shortly.\n");
		return;
	}
	else if (status) {
		Com_Printf("login failed with status %d and error message %s.\n", status, errorMessage);
		return;
	}
	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

	if (!(ent=DB_VerifyClient(loginData.clientnum,loginData.ip))) {
		Com_Printf("login failed, user no longer valid.\n");
		return;
	}

	if (!trap_G_COOL_API_DB_NextRow()) {
		Com_Printf("login failed, user not found.\n");
		return;
	}
	if (!trap_G_COOL_API_DB_GetString(0, loginData.dbPassword, sizeof(loginData.dbPassword))) {
		Com_Printf("login failed, error retrieving password.\n");
		return;
	}
	loginData.userFlags = trap_G_COOL_API_DB_GetInt(1);

	loginData.followUpType = DBREQUEST_LOGIN;

	if (loginData.needDoubleBcrypt) {
		trap_G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("2|%s|%s|%s", BCRYPT_SETTINGS, loginData.dbPassword, loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}
	else {
		trap_G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("1|%s|%s",loginData.dbPassword, loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}

}


static void G_LoginContinue(loginRegisterStruct_t* loginData) {
	static char		cryptedPw[MAX_STRING_CHARS];
	const char* request = NULL;
	gentity_t* ent = NULL;

	if (!(ent = DB_VerifyClient(loginData->clientnum, loginData->ip))) {
		Com_Printf("login failed, user no longer valid (#2).\n");
		return;
	}
	if (strcmp(loginData->password, loginData->dbPassword)) {
		Com_Printf("login failed, password doesn't match.\n");
		return;
	}
	Com_Printf("login successful.\n");
}

static void G_CreateTableResult(int status, const char* errorMessage) {
	static referenceSimpleString_t tableName;
	trap_G_COOL_API_DB_GetReference((byte*)&tableName, sizeof(tableName));
	if (status) {
		Com_Printf("creating table %s failed with status %d and error message %s.\n", tableName.s, status, errorMessage);
		return;
	}
	Com_Printf("creating table %s was successful.\n", tableName.s);

}

static void G_PWBCryptReturned(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	gentity_t* ent;
	if (status) {
		Com_Printf("Password bcrypting failed with status %d and error %s.\n", status, errorMessage);
		return;
	}
	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

	ent = g_entities + loginData.clientnum;

	if (!ent->client) {
		Com_Printf("Password bcrypting finished but client no longer valid?\n", status);
		return;
	}

	if (memcmp(loginData.ip, mv_clientSessions[loginData.clientnum].clientIP, sizeof(loginData.ip))) {
		Com_Printf("Password bcrypting finished but it's no longer the same client?\n", status);
		return;
	}

	if (trap_G_COOL_API_DB_NextRow()) {
		if (!trap_G_COOL_API_DB_GetString(0, loginData.password, sizeof(loginData.password))) {
			Com_Printf("Failed to get bcrypted password from DB API.\n");
			return;
		}

		switch (loginData.followUpType) {
			case DBREQUEST_REGISTER:
				G_RegisterContinue(&loginData);
				break;
			case DBREQUEST_LOGIN:
				G_LoginContinue(&loginData);
				break;
		}

		Com_Printf("G_Login_PWBCryptReturned: Client %d (user %s), Crypted pw: %s\n", loginData.clientnum, loginData.username, loginData.password);

	}
	else {
		Com_Printf("Failed to get bcrypted password from DB API (no response row).\n");
	}
}

void G_DB_CheckResponses() {
	char errorMessage[MAX_STRING_CHARS];

	if (coolApi_dbVersion) {
		int requestType;
		int status;
		while (trap_G_COOL_API_DB_NextResponse(&requestType, NULL, &status, errorMessage, sizeof(errorMessage), NULL, 0)) {
			switch (requestType) {
				default:
					if (status) {
						Com_Printf("DB Request of type %d failed with status %d.\n", requestType, status);
					}
					else {
						if (g_developer.integer) {
							Com_Printf("DB Request of type %d returned with status %d.\n", requestType, status);
						}
					}
					break;
				case DBREQUEST_BCRYPTPW:
					G_PWBCryptReturned(status, errorMessage);
					break;
				case DBREQUEST_REGISTER:
					G_RegisterResult(status, errorMessage);
					break;
				case DBREQUEST_CREATETABLE:
					G_CreateTableResult(status, errorMessage);
					break;
				case DBREQUEST_LOGIN:
					G_LoginFetchDataResult(status, errorMessage);
					break;
				//case DBREQUEST_GETCHATS:
				//	G_DB_GetChatsResponse(status);
				//	break;
			}
		}
	}
}
/*
void G_DB_InsertChat(const char* chatText) {
	char		text[MAX_STRING_CHARS] = { 0 };
	const char* request;

	if (!coolApi_dbVersion || cg.demoPlayback) return;

	// save it to db
	Q_strncpyz(text, chatText, sizeof(text));
	if (trap_G_COOL_API_DB_EscapeString(text, sizeof(text))) {
		request = va("INSERT INTO chats (chat,`time`) VALUES ('%s',NOW())", text);
		trap_G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_CHATSAVE, request);
	}
}

void G_DB_GetChats_f(void) {
	int clientNum = -1;
	int page, first;

	if (!coolApi_dbVersion) {
		G_Printf("getchats not possible, DB API not available\n");
		return;
	}

	page = atoi(G_Argv(1))-1;
	page = MAX(page,0);
	first = page*10;

	trap_G_COOL_API_DB_AddRequest(NULL,0, DBREQUEST_GETCHATS, va("SELECT id, chat, `time` FROM chats ORDER BY time DESC, id DESC LIMIT %d,10",first));
}
*/



static void G_CreateUserTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS users(id BIGINT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(10) UNIQUE, password VARCHAR(64), lastlogin DATETIME, created DATETIME NOT NULL, lastip  INT UNSIGNED, flags  INT UNSIGNED)";
	Q_strncpyz(tableName.s, "users", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
