
#include "g_local.h"
#include "g_dbcmds.h"
#include "../qcommon/crypt_blowfish.h"


typedef int ip_t[4];

//static int IPToInt() {
//
//}


static void G_CreateUserTable();
static void G_CreateRunsTable();

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
/* moved to bg_misc.c
qboolean G_DB_VerifyPassword(const char* password, int clientNumNotify) {
	const char* s = password;
	int len = strlen(password);
	if (len > PASSWORD_MAX_LEN) {
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify,va("print \"^1Chosen password is too long. Maximum %d characters.\n\"", PASSWORD_MAX_LEN));
		}
		return qfalse;
	}
	
	while (*s != '\0') {
		if (*s >= 'a' && *s <= 'z'
			|| *s >= 'A' && *s <= 'Z'
			|| *s >= '0' && *s <= '9'
			|| *s == '_'
			|| *s == '-'
			|| *s == '.'
			|| *s == '/' // pws allow aa bit more leeway than usernames, as they will never be used plaintext, and more possible chars means more security
			|| *s == '[' // cant allow % because netcode wont send it properly, nor ascii codes above 127
			|| *s == ']' // cant allow " because it would break the command
			|| *s == '(' // cant allow ^ because it would be annoying to type colored passwords
			|| *s == ')' // cant allow ` or ~ because console may not allow to type them
			|| *s == '<' // someone COULD of course try it with a .cfg file but let's keep things such that they can be typed ingame
			|| *s == '>'
			|| *s == '='
			|| *s == ':'
			|| *s == ';'
			|| *s == '+'
			|| *s == '*'
			|| *s == '!'
			|| *s == '#'
			|| *s == '$'
			|| *s == '&'
			|| *s == '@'
			|| *s == ','
			|| *s == '?'
			|| *s == '|'
			|| *s == '\''
			) {
			// whitelist. ok.
		}
		else {
			if (clientNumNotify > -2) {
				trap_SendServerCommand(clientNumNotify, "print \"^1Chosen password contains invalid characters. Allowed characters: A-Z a-z 0-9 _-.,/[]()<>=:;+*!#$&@'?| and no empty spaces.\n\"");
			}
			return qfalse;
		}
		s++;
	}
	return qtrue;
}
*/
qboolean G_DB_VerifyUsername(const char* username, int clientNumNotify) {
	const char* s = username;
	int len = strlen(username);
	if (len < USERNAME_MIN_LEN) {
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify,va("print \"^1Chosen username is too short. Minimum %d characters.\n\"", USERNAME_MIN_LEN));
		}
		return qfalse;
	}
	if (len > USERNAME_MAX_LEN) {
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify,va("print \"^1Chosen username is too long. Maximum %d characters.\n\"", USERNAME_MAX_LEN));
		}
		return qfalse;
	}
	
	while (*s != '\0') {
		if (*s >= 'a' && *s <= 'z'
			|| *s >= 'A' && *s <= 'Z'
			|| *s >= '0' && *s <= '9'
			|| *s == '_'
			|| *s == '-'
			|| *s == '.'
			|| *s == '/' // thought about allowing more creativity but its not unlikely ppl will just troll and use random chars?
			|| *s == '['
			|| *s == ']'
			|| *s == '('
			|| *s == ')'
			|| *s == '<'
			|| *s == '>'
			|| *s == '='
			|| *s == ':'
			|| *s == ';'
			|| *s == '+'
			|| *s == '*'
			|| *s == '@'
			) {
			// whitelist. ok.
		}
		else {
			if (clientNumNotify > -2) {
				trap_SendServerCommand(clientNumNotify, "print \"^1Chosen username contains invalid characters. Allowed characters: A-Z a-z 0-9 _-./[]()<>=:;+*@ and no empty spaces.\n\"");
			}
			return qfalse;
		}
		s++;
	}
	return qtrue;
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
	const char*		request = NULL;
	gentity_t* ent = NULL;

	if (!(ent = DB_VerifyClient(loginData->clientnum, loginData->ip))) {
		Com_Printf("^1Register from client %d failed, user no longer valid.\n", loginData->clientnum);
		return;
	}

	if (coolApi_dbVersion >= 3) {
		trap_G_COOL_API_DB_AddPreparedStatement((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER,
			"INSERT INTO users (username,password,created) VALUES (?,?,NOW())");
		trap_G_COOL_API_DB_PreparedBindString(loginData->username);
		trap_G_COOL_API_DB_PreparedBindString(loginData->password);
		trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		static char		cleanUsername[MAX_STRING_CHARS];
		static char		cleanPassword[MAX_STRING_CHARS];
		Q_strncpyz(cleanUsername, loginData->username, sizeof(cleanUsername));
		Q_strncpyz(cleanPassword, loginData->password, sizeof(cleanPassword));
		if (!trap_G_COOL_API_DB_EscapeString(cleanUsername, sizeof(cleanUsername)) || !trap_G_COOL_API_DB_EscapeString(cleanPassword, sizeof(cleanPassword))) {
			trap_SendServerCommand(loginData->clientnum, "print \"^1Registration failed (EscapeString failed).\n\"");
			return;
		}

		request = va("INSERT INTO users (username,password,created) VALUES ('%s','%s',NOW())", cleanUsername, cleanPassword);

		// check if user already exists
		trap_G_COOL_API_DB_AddRequest((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER, request);
	}

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
void PrintRaceTime(finishedRunInfo_t* runInfo, qboolean preliminary, qboolean showRank);

static void G_InsertRunResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateRunStruct_t runData;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;

	trap_G_COOL_API_DB_GetReference((byte*)&runData, sizeof(runData));

	if (!(ent = DB_VerifyClient(runData.clientnum, runData.ip))) {
		Com_Printf("^1Client %d run inserted, user no longer valid.\n", runData.clientnum);
		//return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateRunsTable();
		trap_SendServerCommand(-1,"print \"^1Run insertion failed due to runtable not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(-1, va("print \"^1Run insertion failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	if (coolApi_dbVersion >= 3) {
		// first query is SET @now = NOW(). skip it.
		if (!trap_G_COOL_API_DB_GetMoreResults(&affectedRows))
		{
			trap_SendServerCommand(-1, "print \"^1WTF NO MORE RESULTS\n\"");
		}
	}

	if (affectedRows == 0) {
		//trap_SendServerCommand(-1, "print \"^1No new PB.\n\"");
		runData.runInfo.isPB = 0; // no new pb
	}
	else if (affectedRows == 1) {
		//trap_SendServerCommand(-1, "print \"^1First run.\n\"");
		runData.runInfo.isPB = 1; // first run
	}
	else if (affectedRows == 2) {
		//trap_SendServerCommand(-1, "print \"^1PB!\n\"");
		runData.runInfo.isPB = 2;
	}
	else {
		trap_SendServerCommand(-1, va("print \"^1WTF %d\n\"", affectedRows));
	}

	if (coolApi_dbVersion >= 3 && trap_G_COOL_API_DB_GetMoreResults(NULL) && trap_G_COOL_API_DB_NextRow())
	{
		runData.runInfo.rank = trap_G_COOL_API_DB_GetInt(0) + 1; // SQL result returns amount of faster runs so we add 1 (0 faster runs = #1)
	}

	// SELECT (UNIX_TIMESTAMP(@now)-3000000000) as unixTimeMinus3bill
	// subtracting 3 billion cuz no 64 bit support in vm
	if (coolApi_dbVersion >= 3 && trap_G_COOL_API_DB_GetMoreResults(NULL) && trap_G_COOL_API_DB_NextRow())
	{
		runData.runInfo.unixTimeStampShifted = trap_G_COOL_API_DB_GetInt(0);
	}

	PrintRaceTime(&runData.runInfo, qfalse, qtrue);

}


static void G_LoginContinue(loginRegisterStruct_t* loginData) {
	static char		cryptedPw[MAX_STRING_CHARS];
	const char* request = NULL;
	gentity_t* ent = NULL;
	gclient_t* client = NULL;

	if (!(ent = DB_VerifyClient(loginData->clientnum, loginData->ip))) {
		Com_Printf("^1Login failed, user no longer valid (#2).\n");
		return;
	}
	if (strcmp(loginData->password, loginData->dbPassword)) {
		trap_SendServerCommand(loginData->clientnum, "print \"^1Login failed, password doesn't match.\n\"");
		return;
	}

	client = ent->client;

	Q_strncpyz(client->sess.login.name, loginData->username,sizeof(client->sess.login.name));
	client->sess.login.id = loginData->userId;
	client->sess.login.flags = loginData->userFlags;
	client->sess.login.loggedIn = qtrue;

	trap_SendServerCommand(loginData->clientnum, va("print \"^2Successfully logged in as '%s'.\n\"",loginData->username));

	ClientUserinfoChanged(ent - g_entities);

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
		int affectedRows;
		while (trap_G_COOL_API_DB_NextResponse(&requestType, &affectedRows, &status, errorMessage, sizeof(errorMessage), NULL, 0)) {
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
				case DBREQUEST_INSERTORUPDATERUN:
					G_InsertRunResult(status, errorMessage, affectedRows);
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
	const char* userTableRequest = va("CREATE TABLE IF NOT EXISTS users(id BIGINT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(%d) UNIQUE NOT NULL, password VARCHAR(64)  NOT NULL, lastlogin DATETIME, created DATETIME NOT NULL, lastip  INT UNSIGNED, flags  INT UNSIGNED NOT NULL DEFAULT 0)",USERNAME_MAX_LEN);
	Q_strncpyz(tableName.s, "users", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
static void G_CreateRunsTable() {
	referenceSimpleString_t tableName;
#define SUBFUNC(a) `runFlag_ ## a` TINYINT(1)
#define SUBFUNC2(a) `runFlag_ ## a`
#define SUBFUNC3(a)  INDEX `i_ ## runFlag_ ## a` (`runFlag_ ## a`)
#define RUNFLAGSFUNC(a,b,c) QUOTEME(SUBFUNC(a)) " NOT NULL,"
#define RUNFLAGSFUNC2(a,b,c) "," QUOTEME(SUBFUNC2(a))
#define RUNFLAGSFUNC3(a,b,c) QUOTEME(SUBFUNC3(a)) ","
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS runs(\
			id BIGINT AUTO_INCREMENT PRIMARY KEY, \
			userid BIGINT SIGNED NOT NULL, \
			course VARCHAR(100) NOT NULL, \
			duration_ms INT UNSIGNED NOT NULL, \
			startLessTime INT NOT NULL, \
			endLessTime INT NOT NULL, \
			saveposCount INT NOT NULL, \
			resposCount INT NOT NULL, \
			lostMsecCount INT NOT NULL, \
			lostCmdsCount INT NOT NULL, \
			topspeed DOUBLE NOT NULL, \
			average DOUBLE NOT NULL, \
			distance DOUBLE NOT NULL, \
			distanceXY DOUBLE NOT NULL, \
			style SMALLINT UNSIGNED NOT NULL, \
			msec SMALLINT NOT NULL, \
			jump TINYINT NOT NULL, \
			variant SMALLINT NOT NULL,"
			RUNFLAGS(RUNFLAGSFUNC)
			"runFlags INT NOT NULL, \
			runwhen DATETIME NOT NULL, \
			runfirst DATETIME NOT NULL, \
			warningFlags INT NOT NULL, \
			UNIQUE KEY user_runtype (userid,course,style,msec,jump,variant,runFlags"
			//QUOTEME(RUNFLAGS(RUNFLAGSFUNC2))
			"), \
			INDEX i_userid (userid), INDEX i_course (course), \
			INDEX i_duration_ms (duration_ms), \
			INDEX i_distance (distance), \
			INDEX i_style (style), \
			INDEX i_msec (msec), \
			INDEX i_jump (jump), \
			INDEX i_variant (variant),"
			RUNFLAGS(RUNFLAGSFUNC3)
			"INDEX i_runflags (runFlags), \
			INDEX i_runwhen(runwhen), \
			INDEX i_runfirst (runfirst),\
			INDEX i_warningFlags (warningFlags), \
			INDEX i_runtype (style,msec,jump,variant,runFlags) )";
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef RUNFLAGSFUNC3
#undef SUBFUNC
#undef SUBFUNC2
#undef SUBFUNC3

	if (g_developer.integer) {
		G_Printf("TABLE QUERY DEBUG: %s", userTableRequest);
	}
	// fields without index (cuz just info/debug, dont need to search/filter by it:
	// - distanceXY
	// - startLessTime
	// - endLessTime
	// - saveposCount
	// - resposCount
	// - lostMsecCount
	// - lostCmdsCount
	Q_strncpyz(tableName.s, "runs", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}

static void G_DB_CreateTables() {
	G_CreateUserTable();
	G_CreateRunsTable();
}

void G_DB_Init() {
	if (coolApi_dbVersion) {
		G_Printf("------- DB Initialization -------\n");
		G_DB_CreateTables();
		G_Printf("------- DB Initialization End -------\n");
	}
}
extern const char* DF_RacePrintAppendage(finishedRunInfo_t* runInfo);
//qboolean G_InsertRun(gentity_t* ent, int milliseconds, float topspeed, float average, float distance, int warningFlags, int levelTimeFinish, int commandTimeFinish, int runId) {
qboolean G_InsertRun(finishedRunInfo_t* runInfo) {
	//gclient_t* cl = ent->client;
	insertUpdateRunStruct_t runData;
	//static char serverInfo[BIG_INFO_STRING];
	//static char course[COURSENAME_MAX_LEN+1];
	const char* insertOrUpdateRequest = NULL;
	//if (!cl || !cl->sess.raceMode) return qfalse;
	memset(&runData, 0, sizeof(runData));

	//runData.runInfo.runId = runId;
	//runData.runInfo.milliseconds = milliseconds;
	//runData.runInfo.topspeed = topspeed;
	//runData.runInfo.average = average;
	//runData.runInfo.distance = distance;
	//runData.runInfo.warningFlags = warningFlags;
	//runData.runInfo.levelTimeEnd = levelTimeFinish;

	runData.runInfo = *runInfo;

	//runData.userId = cl->sess.login.loggedIn ? cl->sess.login.id : -1;
	runData.clientnum = runInfo->clientNum;
	memcpy(runData.ip, mv_clientSessions[runData.clientnum].clientIP, sizeof(runData.ip));

	//trap_GetServerinfo(serverInfo, sizeof(serverInfo));
	//Q_strncpyz(course, Info_ValueForKey(serverInfo, "mapname"), sizeof(course));

	// TODO add used fps settings (in case of toggle)
	// TODO add count of savepos/respos

#define SUBFUNC(a) `runFlag_ ## a`
#define RUNFLAGSFUNC(a,b,c) QUOTEME(SUBFUNC(a)) "," // gotta do this cuz qvm gets confused by the comma otherwise
#define RUNFLAGSFUNC2(a,b,c) "?,"
#define RUNFLAGSFUNC3(a,b,c) `runFlag_ ## a`=? AND
	if (coolApi_dbVersion >= 3) {
		insertOrUpdateRequest =
			"SET @now=NOW();"
			"INSERT INTO runs (userid,course,duration_ms,topspeed,average,distance,style,msec,jump,variant,runFlags,"
			RUNFLAGS(RUNFLAGSFUNC)
			"runwhen,runfirst,warningFlags, distanceXY,startLessTime,endLessTime,saveposCount,resposCount,lostMsecCount,lostCmdsCount)"
			"VALUES (?,?,?,?,?,?,?,?,?,?,?,"
			RUNFLAGS(RUNFLAGSFUNC2)
			"@now,@now,?,?,?,?,?,?,?,?)"
			"ON DUPLICATE KEY UPDATE "
			"duration_ms = IF(?<duration_ms,?,duration_ms),"
			"topspeed = IF(?<duration_ms,?,topspeed),"
			"average = IF(?<duration_ms,?,average),"
			"distance = IF(?<duration_ms,?,distance),"
			"runwhen = IF(?<duration_ms,@now,runwhen),"
			"warningFlags = IF(?<duration_ms,?,warningFlags),"
			"distanceXY = IF(?<duration_ms,?,distanceXY),"
			"startLessTime = IF(?<duration_ms,?,startLessTime),"
			"endLessTime = IF(?<duration_ms,?,endLessTime),"
			"saveposCount = IF(?<duration_ms,?,saveposCount),"
			"resposCount = IF(?<duration_ms,?,resposCount),"
			"lostMsecCount = IF(?<duration_ms,?,lostMsecCount),"
			"lostCmdsCount = IF(?<duration_ms,?,lostCmdsCount);"
			// No second statement. check our rank.
			"SELECT COUNT(userid) AS countFaster FROM runs WHERE userid !=? AND userid!=-1 AND course=? AND style=? AND msec=? AND jump=? AND variant=? AND runFlags=? AND "
			//QUOTEME(RUNFLAGS(RUNFLAGSFUNC3))
			" (duration_ms<? OR (duration_ms=? AND runwhen<@now));" // if someone got the same time as you, but earlier, hes in front of u
			"SELECT (UNIX_TIMESTAMP(@now)-(?*1000000000)) as unixTimeMinus3bill";
	}
	else {
		insertOrUpdateRequest =
			"INSERT INTO runs (userid,course,duration_ms,topspeed,average,distance,style,msec,jump,variant,runFlags,"
			QUOTEME(RUNFLAGS(RUNFLAGSFUNC))
			"runwhen,runfirst,warningFlags, distanceXY,startLessTime,endLessTime,saveposCount,resposCount,lostMsecCount,lostCmdsCount)"
			"VALUES (?,?,?,?,?,?,?,?,?,?,?,"
			QUOTEME(RUNFLAGS(RUNFLAGSFUNC2))
			"NOW(),NOW(),?,?,?,?,?,?,?,?)"
			"ON DUPLICATE KEY UPDATE "
			"duration_ms = IF(?<duration_ms,?,duration_ms),"
			"topspeed = IF(?<duration_ms,?,topspeed),"
			"average = IF(?<duration_ms,?,average),"
			"distance = IF(?<duration_ms,?,distance),"
			"runwhen = IF(?<duration_ms,NOW(),runwhen),"
			"warningFlags = IF(?<duration_ms,?,warningFlags),"
			"distanceXY = IF(?<duration_ms,?,distanceXY),"
			"startLessTime = IF(?<duration_ms,?,startLessTime),"
			"endLessTime = IF(?<duration_ms,?,endLessTime),"
			"saveposCount = IF(?<duration_ms,?,saveposCount),"
			"resposCount = IF(?<duration_ms,?,resposCount),"
			"lostMsecCount = IF(?<duration_ms,?,lostMsecCount),"
			"lostCmdsCount = IF(?<duration_ms,?,lostCmdsCount);"
			// No second statement. check our rank.
			"SELECT COUNT(userid) AS countFaster FROM runs WHERE userid !=? AND userid!=-1 AND course=? AND style=? AND msec=? AND jump=? AND variant=? AND runFlags=? AND "
			//QUOTEME(RUNFLAGS(RUNFLAGSFUNC3))
			" (duration_ms<? OR (duration_ms=? AND runwhen<NOW()))"; // if someone got the same time as you, but earlier, hes in front of u
	}
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef RUNFLAGSFUNC3
		


	if(!trap_G_COOL_API_DB_AddPreparedStatement((byte*)&runData, sizeof(insertUpdateRunStruct_t), DBREQUEST_INSERTORUPDATERUN,
		insertOrUpdateRequest)) {
		trap_SendServerCommand(-1, va("print \"Database connection not available. Run cannot be saved.\n\" dfrunsavefailed %s", DF_RacePrintAppendage(runInfo)));
		return qfalse;
	}

	// INSERT PART
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	trap_G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->topspeed);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->average);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->distance);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
#define RUNFLAGSFUNC(a,b,c) trap_G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
	RUNFLAGS(RUNFLAGSFUNC)
#undef RUNFLAGSFUNC

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->warningFlags);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->distanceXY);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->startLessTime);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->endLessTime);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->savePosCount);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->resposCount);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->lostMsecCount);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->lostPacketCount);

	// UPDATE PART
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->topspeed);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->average);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->distance);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds); // runwhen

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->warningFlags);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindFloat(runInfo->distanceXY);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->startLessTime);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->endLessTime);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->savePosCount);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->resposCount);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->lostMsecCount);

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->lostPacketCount);

	// SECONED QUERY - SELECT
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	trap_G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

//#define RUNFLAGSFUNC(a,b,c) trap_G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
	//RUNFLAGS(RUNFLAGSFUNC)
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
//#undef RUNFLAGSFUNC

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	if (coolApi_dbVersion >= 3) {
		trap_G_COOL_API_DB_PreparedBindInt(runInfo->unixTimeStampShiftedBillionCount);
	}

	trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
	//Q_strncpyz(tableName.s, "runs", sizeof(tableName.s));
	//trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
	return qtrue;
}

