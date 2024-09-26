
#include "g_dbcmds.h"
#include "g_local.h"
#include "../qcommon/crypt_blowfish.h"


typedef int ip_t[4];

//static int IPToInt() {
//
//}


static void G_CreateUserTable();

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
	gentity_t* ent = NULL;

	if (!(ent = DB_VerifyClient(loginData->clientnum, loginData->ip))) {
		Com_Printf("^1Register from client %d failed, user no longer valid.\n", loginData->clientnum);
		return;
	}
	Q_strncpyz(cleanUsername,loginData->username,sizeof(cleanUsername));
	Q_strncpyz(cleanPassword,loginData->password,sizeof(cleanPassword));
	if (!trap_G_COOL_API_DB_EscapeString(cleanUsername, sizeof(cleanUsername)) || !trap_G_COOL_API_DB_EscapeString(cleanPassword, sizeof(cleanPassword))) {
		trap_SendServerCommand(loginData->clientnum,"print \"^1Registration failed (EscapeString failed).\n\"");
		return;
	}

	request = va("INSERT INTO users (username,password,created) VALUES ('%s','%s',NOW())", cleanUsername, cleanPassword);

	// check if user already exists
	trap_G_COOL_API_DB_AddRequest((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER, request);

}

static void G_RegisterResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData; 
	gentity_t* ent = NULL;

	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));
	if (!(ent = DB_VerifyClient(loginData.clientnum, loginData.ip))) {
		Com_Printf("^1Register from client %d failed, user no longer valid.\n", loginData.clientnum);
		return;
	}
	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		trap_SendServerCommand(loginData.clientnum, "print \"^1Registration failed due to usertable not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status == 1062) {
		trap_SendServerCommand(loginData.clientnum, "print \"^1A user with this name already exists.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(loginData.clientnum, va("print \"^1Registration failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}
	trap_SendServerCommand(loginData.clientnum, va("print \"^2Registration successful. You can now log in as '%s'.\n\"", loginData.username));

}

static void G_LoginFetchDataResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	static char password[MAX_STRING_CHARS];
	gentity_t* ent = NULL;

	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

	if (!(ent = DB_VerifyClient(loginData.clientnum, loginData.ip))) {
		Com_Printf("^1Login from client %d failed, user no longer valid.\n", loginData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		trap_SendServerCommand(loginData.clientnum,"print \"^1Login failed due to usertable not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(loginData.clientnum, va("print \"^1Login failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	if (!trap_G_COOL_API_DB_NextRow()) {
		trap_SendServerCommand(loginData.clientnum, "print \"^1Login failed, username not found.\n\"");
		return;
	}
	if (!trap_G_COOL_API_DB_GetString(0, loginData.dbPassword, sizeof(loginData.dbPassword))) {
		trap_SendServerCommand(loginData.clientnum, "print \"^1Login failed, error retrieving password.\n\"");
		return;
	}
	loginData.userFlags = trap_G_COOL_API_DB_GetInt(1);
	loginData.userId = trap_G_COOL_API_DB_GetInt(2);

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
		Com_Printf("^1Login failed, user no longer valid (#2).\n");
		return;
	}
	if (strcmp(loginData->password, loginData->dbPassword)) {
		trap_SendServerCommand(loginData->clientnum, "print \"^1Login failed, password doesn't match.\n\"");
		return;
	}
	trap_SendServerCommand(loginData->clientnum, va("print \"^2Successfully logged in as '%s'.\n\"",loginData->username));



	// fire and forget, not that important
	trap_G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_LOGIN_UPDATELASTLOGIN,
		va("UPDATE users SET lastlogin=NOW() WHERE id=%d", loginData->userId));
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

	trap_G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

	if (!(ent = DB_VerifyClient(loginData.clientnum, loginData.ip))) {
		Com_Printf("^1bcrypt succeeded, but user no longer valid (#2).\n");
		return;
	}

	if (status) {
		trap_SendServerCommand(loginData.clientnum,va("print \"^1Password bcrypting failed with status %d and error %s.\n\"", status, errorMessage));
		return;
	}
	if (trap_G_COOL_API_DB_NextRow()) {
		if (!trap_G_COOL_API_DB_GetString(0, loginData.password, sizeof(loginData.password))) {
			trap_SendServerCommand(loginData.clientnum, "print \"^1Failed to get bcrypted password from DB API.\n\"");
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

		if (g_developer.integer) {
			trap_SendServerCommand(loginData.clientnum, va("print \"G_Login_PWBCryptReturned: Client %d (user %s), Crypted pw: %s\n\"", loginData.clientnum, loginData.username, loginData.password));
		}
	}
	else {
		trap_SendServerCommand(loginData.clientnum, "print \"^1Failed to get bcrypted password from DB API (no response row).\n\"");
	}
}

void G_DB_CheckResponses() {
	char errorMessage[MAX_STRING_CHARS];

	if (coolApi_dbVersion) {
		int requestType;
		int status;
		while (trap_G_COOL_API_DB_NextResponse(&requestType, NULL, &status, errorMessage, sizeof(errorMessage), NULL, 0)) {
			switch (requestType) {
				case DBREQUEST_LOGIN_UPDATELASTLOGIN:
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
