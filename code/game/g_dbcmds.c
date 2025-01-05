
#include "g_local.h"
#include "g_dbcmds.h"
#include "../qcommon/crypt_blowfish.h"


typedef int ip_t[4];

//static int IPToInt() {
//
//}


static void G_CreateUserTable();
static void G_CreateRunsTable();
static void G_CreateCheckpointsTable();
static void G_CreateSubContestsTable();
static void G_CreateMapRaceDefaultsTable();
extern const char* DF_GetCourseName(); 
const char* DF_GetMainSubcourseName();
extern void DF_SetSubContestDefaults(gclient_t* client);

gentity_t* DB_VerifyClient(int clientNum, ip_t ip) {
	gentity_t* ent;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		Com_Printf("DB_VerifyClient: client number %d invalid.\n", clientNum);
		return NULL;
	}
	
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

// we will be creating folders based on usernames so we have to make sure we dont allow any usernames
// that could cause filesystem issues.
// ideally the names also dont cause any issues when printed/sent as commands
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

	if (*s == '-' || *s == '.') {
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify, va("print \"^1Usernames cannot start with - or a dot.\n\"", USERNAME_MAX_LEN));
		}
		return qfalse;
	}
	
	while (*s != '\0') {
		if (*s >= 'a' && *s <= 'z'
			|| *s >= 'A' && *s <= 'Z'
			|| *s >= '0' && *s <= '9'
			|| *s == '_'
			|| *s == '-'
			//|| *s == '.' // could mess with filenames/paths (checkdirtraversal)
			//|| *s == '/' // could mess with filenames (as it is a folder separator)
			|| *s == '['
			|| *s == ']'
			|| *s == '('
			|| *s == ')'
			//|| *s == '<'	// demonames: windows wont allow this in filenames
			//|| *s == '>'	// demonames: windows wont allow this in filenames
			|| *s == '='
			//|| *s == ':'	// demonames: windows wont allow this in filenames
			|| *s == ';'
			|| *s == '+'
			//|| *s == '*'	// demonames: windows wont allow this in filenames
			|| *s == '@'
			) {
			// whitelist. ok.
		}
		else {
			if (clientNumNotify > -2) {
				trap_SendServerCommand(clientNumNotify, "print \"^1Chosen username contains invalid characters. Allowed characters: A-Z a-z 0-9 _-[]()=;+@ and no empty spaces.\n\"");
			}
			return qfalse;
		}
		s++;
	}
	s--;
	if (*s == ' ' || *s == '.') { // well technically we dont allow either of these chars anyway so meh
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify, "print \"^1Username must not end with a space or dot.\n\"");
		}
		return qfalse;
	}

	// these are special reserved windows file/folder names that will cause mayhem if we allow them or best case, we end up losing the demos
	if (len ==3 && 
		(!Q_stricmp(username,"CON")
		|| !Q_stricmp(username, "PRN")
		|| !Q_stricmp(username, "AUX")
		|| !Q_stricmp(username, "NUL"))

		// COM0-COM9, LPT0-LPT9
		|| len == 4 && username[3] >= '0' && username[3] <= '9' &&
		(!Q_stricmpn(username,"COM",3)
			|| !Q_stricmpn(username, "LPT", 3))

		|| !Q_stricmp(username, "CLOCK$")
		) {
		if (clientNumNotify > -2) {
			trap_SendServerCommand(clientNumNotify, "print \"^1Your chosen username is not valid because it is a reserved keyword.\n\"");
		}
		return qfalse;
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
	while (G_COOL_API_DB_NextRow()) {
		
		int id = G_COOL_API_DB_GetInt(0);
		G_COOL_API_DB_GetString(1, text,sizeof(text));
		G_COOL_API_DB_GetString(2, time,sizeof(time));
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
		G_COOL_API_DB_AddPreparedStatement((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER,
			"INSERT INTO users (username,password,created) VALUES (?,?,NOW())");
		G_COOL_API_DB_PreparedBindString(loginData->username);
		G_COOL_API_DB_PreparedBindString(loginData->password);
		G_COOL_API_DB_FinishAndSendPreparedStatement();
	}
	else {
		static char		cleanUsername[MAX_STRING_CHARS];
		static char		cleanPassword[MAX_STRING_CHARS];
		Q_strncpyz(cleanUsername, loginData->username, sizeof(cleanUsername));
		Q_strncpyz(cleanPassword, loginData->password, sizeof(cleanPassword));
		if (!G_COOL_API_DB_EscapeString(cleanUsername, sizeof(cleanUsername)) || !G_COOL_API_DB_EscapeString(cleanPassword, sizeof(cleanPassword))) {
			trap_SendServerCommand(loginData->clientnum, "print \"^1Registration failed (EscapeString failed).\n\"");
			return;
		}

		request = va("INSERT INTO users (username,password,created) VALUES ('%s','%s',NOW())", cleanUsername, cleanPassword);

		// check if user already exists
		G_COOL_API_DB_AddRequest((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_REGISTER, request);
	}

}

static void G_ChangePasswordContinue(loginRegisterStruct_t* loginData) {
	const char*		request = NULL;
	gentity_t* ent = NULL;

	if (!(ent = DB_VerifyClient(loginData->clientnum, loginData->ip))) {
		Com_Printf("^1Change password from client %d failed, user no longer valid.\n", loginData->clientnum);
		return;
	}
	if (ent->client->sess.login.id != loginData->userId) {
		trap_SendServerCommand(loginData->clientnum, "print \"^1Password change failed, no longer logged in as same user.\n\"");
		return;
	}

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)loginData, sizeof(loginRegisterStruct_t), DBREQUEST_CHANGEPASSWORD,
		"UPDATE users SET password=? WHERE id=?")) {
		trap_SendServerCommand(loginData->clientnum, "print \"^1Password change failed for unspecified reason.\n\"");
		return;
	}
	G_COOL_API_DB_PreparedBindString(loginData->password);
	G_COOL_API_DB_PreparedBindInt(loginData->userId);
	G_COOL_API_DB_FinishAndSendPreparedStatement();

}

static void G_RegisterResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData; 
	gentity_t* ent = NULL;

	G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));
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
static void G_ChangePasswordResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData; 
	gentity_t* ent = NULL;

	G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));
	if (!(ent = DB_VerifyClient(loginData.clientnum, loginData.ip))) {
		Com_Printf("^1Change password from client %d failed, user no longer valid.\n", loginData.clientnum);
		return;
	}
	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		trap_SendServerCommand(loginData.clientnum, "print \"^1Change password failed due to usertable not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(loginData.clientnum, va("print \"^1Change password failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}
	trap_SendServerCommand(loginData.clientnum, va("print \"^2Change password. You can now log in with your new password.\n\"", loginData.username));

}

static void G_LoginFetchDataResult(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	static char password[MAX_STRING_CHARS];
	static char tmpUsername[sizeof(loginData.username)];
	gentity_t* ent = NULL;

	G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

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

	if (!G_COOL_API_DB_NextRow()) {
		trap_SendServerCommand(loginData.clientnum, "print \"^1Login failed, username not found.\n\"");
		return;
	}
	if (!G_COOL_API_DB_GetString(0, loginData.dbPassword, sizeof(loginData.dbPassword))) {
		trap_SendServerCommand(loginData.clientnum, "print \"^1Login failed, error retrieving password.\n\"");
		return;
	}
	loginData.userFlags = G_COOL_API_DB_GetInt(1);
	loginData.userId = G_COOL_API_DB_GetInt(2);
	if (!G_COOL_API_DB_GetString(3, tmpUsername, sizeof(tmpUsername))) {
		// override username with how its written in DB (cuz can match different case but wanna have demo files named consistently)
		trap_SendServerCommand(loginData.clientnum, "print \"^1WTF COULDN'T GRAB USERNAME, SHOULDN'T HAPPEN!!!.\n\"");
		Com_Printf("^1WTF COULDN'T GRAB USERNAME, SHOULDN'T HAPPEN!!!.\n");
	}
	else {
		Q_strncpyz(loginData.username, tmpUsername,sizeof(loginData.username));
	}

	loginData.followUpType = DBREQUEST_LOGIN;

	if (loginData.needDoubleBcrypt) {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("2|%s|%s|%s", BCRYPT_SETTINGS, loginData.dbPassword, loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}
	else {
		G_COOL_API_DB_AddRequestTyped((byte*)&loginData, sizeof(loginData), DBREQUEST_BCRYPTPW,
			va("1|%s|%s",loginData.dbPassword, loginData.password)
			, DBREQUESTTYPE_BCRYPT);
	}

}

const char* G_GenerateRunDemoName(finishedRunInfo_t* runInfo) {
	static char name[MAX_OSPATH];
	static char sanitizedCourseName[sizeof(runInfo->coursename)];
	static char sanitizedSubCourseName[sizeof(runInfo->subcoursename)];
	static char sanitizedUsername[sizeof(runInfo->subcoursename)];
	sanitizeFilename(runInfo->coursename, sanitizedCourseName, qfalse); // take care of possible special cahrs the filesystem may not like
	sanitizeFilename(runInfo->subcoursename, sanitizedSubCourseName, qfalse); // take care of possible special cahrs the filesystem may not like
	if (runInfo->userId == -1) {

		Com_sprintf(name, sizeof(name), "races/unlogged/%s%s-%s"
			, sanitizedCourseName
			, sanitizedSubCourseName[0] ? miniva("(%s)", sanitizedSubCourseName) : ""
			, DF_DemoRaceStyleNamePart(&runInfo->raceStyle));
	}
	else {
		sanitizeFilename(runInfo->username, sanitizedUsername, qfalse); // take care of possible special cahrs the filesystem may not like
		Com_sprintf(name, sizeof(name), "races/logged/%s/%s-%s%s-%s", sanitizedUsername
			, sanitizedUsername
			, sanitizedCourseName
			, sanitizedSubCourseName[0] ? miniva("(%s)", sanitizedSubCourseName) : ""
			, DF_DemoRaceStyleNamePart(&runInfo->raceStyle));
	}
	return name;
}

void PrintRaceTime(finishedRunInfo_t* runInfo, qboolean preliminary, qboolean showRank, gentity_t* ent);

static void G_InsertRunResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateRunStruct_t runData;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&runData, sizeof(runData));

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
		if (!G_COOL_API_DB_GetMoreResults(&affectedRows))
		{
			trap_SendServerCommand(-1, "print \"^1WTF NO MORE RESULTS\n\"");
		}
	}

	runData.runInfo.pbStatus = 0;
	if (affectedRows == 0) {
		//trap_SendServerCommand(-1, "print \"^1No new PB.\n\"");
		// no new pb
	}
	else if (affectedRows == 1) {
		//trap_SendServerCommand(-1, "print \"^1First run.\n\"");
		runData.runInfo.pbStatus |= PB_FIRSTRUN_SPECIFICSTYLE; // first run
	}
	else if (affectedRows == 2) {
		//trap_SendServerCommand(-1, "print \"^1PB!\n\"");
		runData.runInfo.pbStatus |= PB_NEWPB_SPECIFICSTYLE;
	}
	else {
		trap_SendServerCommand(-1, va("print \"^1WTF %d\n\"", affectedRows));
	}


	if (coolApi_dbVersion >= 3 && G_COOL_API_DB_GetMoreResults(NULL) && G_COOL_API_DB_NextRow())
	{
		if (!G_COOL_API_DB_GetInt(0)) {// SQL result returns amount of faster runs BY OURSELVES on this LB
			runData.runInfo.pbStatus |= PB_LB;
		}
	}

	if (coolApi_dbVersion >= 3 && G_COOL_API_DB_GetMoreResults(NULL) && G_COOL_API_DB_NextRow())
	{
		runData.runInfo.rankLB = G_COOL_API_DB_GetInt(0) + 1; // SQL result returns amount of faster runs so we add 1 (0 faster runs = #1)
	}

	// SELECT (UNIX_TIMESTAMP(@now)-3000000000) as unixTimeMinus3bill
	// subtracting 3 billion cuz no 64 bit support in vm
	if (coolApi_dbVersion >= 3 && G_COOL_API_DB_GetMoreResults(NULL) && G_COOL_API_DB_NextRow())
	{
		runData.runInfo.unixTimeStampShifted = G_COOL_API_DB_GetInt(0);
	}

	if (runData.runInfo.tempDemoName[0]) {
		if ((runData.runInfo.pbStatus & PB_FIRSTRUN_SPECIFICSTYLE) || (runData.runInfo.pbStatus & PB_NEWPB_SPECIFICSTYLE)) {
			//if (runData.runInfo.userId == -1) {
			//	trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"races_unlogged/%s%s-%s\"\n", runData.runInfo.tempDemoName
			//		, runData.runInfo.coursename
			//		, runData.runInfo.subcoursename[0] ? miniva("(%s)", runData.runInfo.subcoursename) : ""
			//		, DF_DemoRaceStyleNamePart(&runData.runInfo.raceStyle)
			//	));
			//}
			//else {
			//	trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"races/%s/%s-%s%s-%s\"\n", runData.runInfo.tempDemoName, runData.runInfo.username
			//		,runData.runInfo.username
			//		,runData.runInfo.coursename
			//		, runData.runInfo.subcoursename[0] ? miniva("(%s)", runData.runInfo.subcoursename) : ""
			//		,DF_DemoRaceStyleNamePart(&runData.runInfo.raceStyle)
			//		));
			//}

			trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"%s\"\n", runData.runInfo.tempDemoName
				, G_GenerateRunDemoName(&runData.runInfo)
			));
		}
		else {
			// "delete" it.
			trap_SendConsoleCommand(EXEC_APPEND, va("svrenamedemo \"%s\" \"trash/trash%d\"\n", runData.runInfo.tempDemoName, runData.runInfo.clientNum));
		}
	}

	PrintRaceTime(&runData.runInfo, qfalse, qtrue,ent);

}
static void G_InsertSubcontestResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateSubContestStruct_t runData;
	gentity_t* ent = NULL;
	int pbStatus = 0;
	int rank = 0;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&runData, sizeof(runData));

	if (!(ent = DB_VerifyClient(runData.clientnum, runData.ip))) {
		Com_Printf("^1Client %d subcontest inserted, user no longer valid.\n", runData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateSubContestsTable();
		trap_SendServerCommand(-1,"print \"^1Subcontest insertion failed due to subcontest table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(-1, va("print \"^1Subcontest insertion failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	if (coolApi_dbVersion >= 3) {
		// first query is SET @now = NOW(). skip it.
		if (!G_COOL_API_DB_GetMoreResults(&affectedRows))
		{
			trap_SendServerCommand(-1, "print \"^1WTF NO MORE RESULTS\n\"");
		}
	}

	pbStatus = 0;
	if (affectedRows == 0) {
		//trap_SendServerCommand(-1, "print \"^1No new PB.\n\"");
		// no new pb
	}
	else if (affectedRows == 1) {
		//trap_SendServerCommand(-1, "print \"^1First run.\n\"");
		pbStatus |= PB_FIRSTRUN_SPECIFICSTYLE; // first run
	}
	else if (affectedRows == 2) {
		//trap_SendServerCommand(-1, "print \"^1PB!\n\"");
		pbStatus |= PB_NEWPB_SPECIFICSTYLE;
	}
	else {
		trap_SendServerCommand(-1, va("print \"^1WTF %d\n\"", affectedRows));
	}


	if (coolApi_dbVersion >= 3 && G_COOL_API_DB_GetMoreResults(NULL) && G_COOL_API_DB_NextRow())
	{
		rank = G_COOL_API_DB_GetInt(0) + 1; // SQL result returns amount of faster runs so we add 1 (0 faster runs = #1)
	}

	if (rank == 1 && pbStatus) {
		if (runData.userid == -1) {
			switch (runData.contest) {
			case SUBCONTESTS_ROLLYMPICS:
				trap_SendServerCommand(-1, va("print \"%s ^7unofficially beat the best logged roll with ^3%.2f^7ups\n\"", ent->client->pers.netname, runData.value));
				break;
			}
		}
		else {
			switch (runData.contest) {
			case SUBCONTESTS_ROLLYMPICS:
				trap_SendServerCommand(-1, va("print \"%s ^7now holds the fastest roll record with ^2%.2f^7ups\n\"", ent->client->pers.netname, runData.value));
				break;
			}
		}
	}

}
static void G_InsertMapDefaultsResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateMapRaceDefaultsStruct_t data;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Map defaults by client %d inserted, user no longer valid.\n", data.clientnum);
		//return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateMapRaceDefaultsTable();
		trap_SendServerCommand(ent - g_entities,"print \"^1Map defaults insertion failed due to map defaults table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(ent - g_entities, va("print \"^1Map defaults insertion failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	trap_SendServerCommand(-1, va("print \"^1Map defaults (%s) for %s were updated\n\"",data.what,data.course));

}

static void G_LoadMapDefaultsResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateMapRaceDefaultsStruct_t data;
	const char* currentCoursename;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateMapRaceDefaultsTable();
		trap_SendServerCommand(-1,"print \"^1Map defaults load failed due to map defaults table not existing. Attempting to create. Please try again shortly.\n\"");
		level.mapDefaultsLoadFailed = qfalse; // we dont have a defdault so its ok
		level.mapDefaultsConfirmed = qtrue;
		return;
	}
	else if (status) {
		trap_SendServerCommand(-1, va("print \"^1Map defaults load failed with status %d and error message %s.\n\"", status, errorMessage));
		level.mapDefaultsLoadFailed = qtrue;
		level.mapDefaultsConfirmed = qfalse;
		return;
	}

	currentCoursename = DF_GetCourseName();
	if (Q_stricmp(currentCoursename, data.course)) {
		if (currentCoursename[0]) {
			trap_SendServerCommand(-1, "print \"^1Map defaults load failed; course name changed (?). Retrying.\n\"");
			DF_LoadMapDefaults();
		}
		else {
			trap_SendServerCommand(-1, "print \"^1Map defaults load failed;  current coursename empty?!?!!?\n\"");
		}
		return;
	}

	if (!G_COOL_API_DB_NextRow()) {
		trap_SendServerCommand(-1, "print \"^1Map defaults load failed; no defaults found.\n\"");
		level.mapDefaultsLoadFailed = qfalse; // we dont have a defdault so its ok
		level.mapDefaultsConfirmed = qtrue;
		return;
	}
	else {
		raceStyle_t rs;
		rs.movementStyle = MV_JK2;
		rs.msec = G_COOL_API_DB_GetInt(0);
		rs.jumpLevel = G_COOL_API_DB_GetInt(1);
		rs.variant = G_COOL_API_DB_GetInt(2);
		rs.runFlags = G_COOL_API_DB_GetInt(3);
		DF_SetMapDefaults(rs);
		level.mapDefaultsLoadFailed = qfalse;
		level.mapDefaultsConfirmed = qtrue;
		trap_SendServerCommand(-1, va("print \"^2Map defaults for %s were loaded.\n\"", data.course));
	}

}

static void G_SaveCheckpointsResult(int status, const char* errorMessage, int affectedRows) {
	checkPointSaveRequestStruct_t data;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;
	int deleted=0, inserted=0;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d checkpoints saved, user no longer valid.\n", data.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateCheckpointsTable();
		G_SendServerCommand(ent-g_entities,"print \"^1Checkpoint saving failed due to checkpoints table not existing. Attempting to create. Please try again shortly.\n\"",qtrue);
		return;
	}
	else if (status) {
		G_SendServerCommand(ent - g_entities, va("print \"^1Checkpoint saving failed with status %d and error message %s.\n\"", status, errorMessage),qtrue);
		return;
	}

	deleted = affectedRows;

	if (coolApi_dbVersion >= 3) {
		// first query is SET @now = NOW(). skip it.
		if (!G_COOL_API_DB_GetMoreResults(&inserted))
		{
			G_SendServerCommand(ent - g_entities, "print \"^1WTF NO MORE RESULTS\n\"",qtrue);
		}
	}

	G_SendServerCommand(ent - g_entities, va("print \"^2%d checkpoints saved to user account, %d old saved checkpoints deleted.\n\"", inserted, deleted),qtrue);

}
qboolean DF_CreateCustomCheckpointFromPos(vec3_t trEndpos, float anglesYaw, gentity_t* playerent);
static void G_LoadCheckpointsResult(int status, const char* errorMessage, int affectedRows) {
	checkPointSaveRequestStruct_t data;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;
	int loaded =0;
	vec3_t trEndpos;
	float yaw;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d checkpoints loaded, user no longer valid.\n", data.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateCheckpointsTable();
		G_SendServerCommand(ent-g_entities,"print \"^1Checkpoint loading failed due to checkpoints table not existing. Attempting to create. Please try again shortly.\n\"",qtrue);
		return;
	}
	else if (status) {
		G_SendServerCommand(ent - g_entities, va("print \"^1Checkpoint loading failed with status %d and error message %s.\n\"", status, errorMessage),qtrue);
		return;
	}

	while (G_COOL_API_DB_NextRow()) {
		G_COOL_API_DB_GetFloat(0,&trEndpos[0]);
		G_COOL_API_DB_GetFloat(1,&trEndpos[1]);
		G_COOL_API_DB_GetFloat(2,&trEndpos[2]);
		G_COOL_API_DB_GetFloat(3,&yaw);
		if (!DF_CreateCustomCheckpointFromPos(trEndpos, yaw, ent)) {
			G_SendServerCommand(ent - g_entities, "print \"^1Checkpoint limit reached. Can't load any more checkpoints.\n\"",qtrue);
			break;
		}
		else {
			loaded++;
		}
	}

	G_SendServerCommand(ent - g_entities, va("print \"^2%d checkpoints loaded from user account.\n\"", loaded),qtrue);

}

void DF_TopRequest(gentity_t* ent, const char* coursename, const char* subcoursename, int page, int style, topRequestType_t type, mainLeaderboardType_t lbTypeIfSpecific, raceStyle_t* thisMapDefaultRaceStyle);

static void G_TopMapSearchResult(int status, const char* errorMessage, int affectedRows) {
	topRequestStruct_t data;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;
	int loaded =0;
	vec3_t trEndpos;
	static char courseName[COURSENAME_MAX_LEN + 1];
	static char subCourseName[COURSENAME_MAX_LEN + 1];
	int resultsFound = 0;
	int diff, diff2;
	int mapDefaultsFound;
	raceStyle_t mapDefaultRaceStyle;
	qboolean afterRun = qfalse; // TODO send to spectators if following guy who just got PB/WR?

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d top map search results returned, user no longer valid.\n", data.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateRunsTable();
		G_SendServerCommand(ent-g_entities,"print \"^1Searching maps for top results failed due to runs table not existing. Attempting to create. Please try again shortly.\n\"", afterRun);
		return;
	}
	else if (status) {
		G_SendServerCommand(ent - g_entities, va("print \"^1Searching maps for top results failed with status %d and error message %s.\n\"", status, errorMessage), afterRun);
		return;
	}

	G_SendServerCommand(ent - g_entities, "print \"Your top result request matches the following maps/courses:\n\"", afterRun);


	// first query is SET @now = NOW(). skip it.
	if (!G_COOL_API_DB_GetMoreResults(&affectedRows))
	{
		trap_SendServerCommand(-1, "print \"^1WTF NO MORE RESULTS\n\"");
		return;
	}

	while (G_COOL_API_DB_NextRow()) {
		G_COOL_API_DB_GetString(0, courseName,sizeof(courseName));
		G_COOL_API_DB_GetString(1, subCourseName,sizeof(subCourseName));
		diff = G_COOL_API_DB_GetInt(2);
		diff2 = G_COOL_API_DB_GetInt(3);
		if (!resultsFound) {
			mapDefaultsFound = !G_COOL_API_DB_GetInt(4);
			if (mapDefaultsFound) {
				memset(&mapDefaultRaceStyle, 0, sizeof(mapDefaultRaceStyle));
				mapDefaultRaceStyle.msec = G_COOL_API_DB_GetInt(5);
				mapDefaultRaceStyle.jumpLevel = G_COOL_API_DB_GetInt(6);
				mapDefaultRaceStyle.runFlags = G_COOL_API_DB_GetInt(7);
			}
			DF_TopRequest(ent, courseName, subCourseName, data.page, data.style,data.type,data.lbTypeIfSpecific,mapDefaultsFound ? &mapDefaultRaceStyle : &defaultRaceStyle);
		}
		if (!subCourseName[0]) {
			if (g_developer.integer) {
				G_SendServerCommand(ent - g_entities, va("print \"^3%s%s (diff %d %d)\n\"", resultsFound ? "" : "->", courseName, diff, diff2), qtrue);
			}
			else {
				G_SendServerCommand(ent - g_entities, va("print \"^3%s%s\n\"", resultsFound ? "" : "->", courseName), qtrue);
			}
		}
		else {
			if (g_developer.integer) {
				G_SendServerCommand(ent - g_entities, va("print \"^3%s%s/%s  (diff %d %d)\n\"", resultsFound ? "" : "->", courseName, subCourseName, diff, diff2), qtrue);
			}
			else {
				G_SendServerCommand(ent - g_entities, va("print \"^3%s%s/%s\n\"", resultsFound ? "" : "->", courseName, subCourseName), qtrue);
			}
		}
		resultsFound++;

	}
	if (!resultsFound) {
		G_SendServerCommand(ent - g_entities, "print \"^1Nothing.\n\"", qtrue);
	}


}

void G_AutoGenerateArena(const char* thisMapName, qboolean checkBspExists);

static void G_ArenaGenMapListResult(int status, const char* errorMessage, int affectedRows) {
	static char courseName[COURSENAME_MAX_LEN + 1];
	int resultsFound = 0;

	if (level.allRaceGenerationAlreadyCalled) {
		G_SendServerCommand(-1, "print \"^1Allrace arena generation already called once during this map.\n\"", qfalse);
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateRunsTable();
		G_SendServerCommand(-1,"print \"^1Searching maps for arena generation failed due to runs table not existing. Attempting to create. Please try again shortly.\n\"",qfalse);
		return;
	}
	else if (status) {
		G_SendServerCommand(-1, va("print \"^1Searching maps for arena generation failed with status %d and error message %s.\n\"", status, errorMessage),qfalse);
		return;
	}


	while (G_COOL_API_DB_NextRow()) {
		G_COOL_API_DB_GetString(0, courseName,sizeof(courseName));
		resultsFound++;
		G_AutoGenerateArena(courseName, qtrue);
		level.allRaceGenerationAlreadyCalled = qtrue;
	}
	if (!resultsFound) {
		G_SendServerCommand(-1, "print \"^1No maps found for arena generation.\n\"", qtrue);
	}


}

typedef struct topLeaderBoardEntry_s {
	qboolean exists;
	int besttime, userid, runFlags, msec, jump, runFlagsDiff;
	qboolean mainLBCompatible;
	//raceStyle_t raceStyle;
	float topSpeed, average,distance;
	int savePosCount, resposCount, duration_ms_segmented_total;
	char username[USERNAME_MAX_LEN + 1];
	char time[25];
	char fpsString[40];
} topLeaderBoardEntry_t;

// cringe :)
static const char* topNumberStrings[] = {
	"01", // linux doesnt like padding strings with 0 so i do it myself :(
	"02",
	"03",
	"04",
	"05",
	"06",
	"07",
	"08",
	"09",
	"10",
	"UL",
};

static void G_TopResult(int status, const char* errorMessage, int affectedRows) {
	topScoresRequestStruct_t lbRequestData;
	gentity_t* ent = NULL;
	int currentType = -1;
	//int rank = 1;
	int maxrank = 0;
	int i;
	int	offsetRank;
	static topLeaderBoardEntry_t entries[11][LB_TYPES_COUNT];
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d top results returned, user no longer valid.\n", lbRequestData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateRunsTable();
		trap_SendServerCommand(lbRequestData.clientnum,"print \"^1Leaderboard display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1Leaderboard display failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	memset(entries, 0, sizeof(entries));

	while (G_COOL_API_DB_NextRow()) {
		int type, userid, rankHere;
		int realRank;
		topLeaderBoardEntry_t* entry;
		type = G_COOL_API_DB_GetInt(0);
		userid = G_COOL_API_DB_GetInt(3);
		realRank = G_COOL_API_DB_GetInt(14);
		if (lbRequestData.type == TOPREQUEST_SPECIFICLB && type != lbRequestData.lbTypeIfSpecific) continue;
		if (type != currentType) {
			currentType = type;
			//rank = 1;
			//trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^2Leaderboard type %d.\n\"", currentType));
		}
		//if (rank > 9) continue;
		rankHere = userid == -1 ? 10 : realRank-1-lbRequestData.page*10;
		if (rankHere > 10 || rankHere < 0) continue;
		entry = &entries[rankHere][type]; // unofficial go at the end.
		entry->exists = qtrue;
		if (userid == -1) {
			Q_strncpyz(entry->username, "!unlogged!", sizeof(entry->username));
		}
		else {
			G_COOL_API_DB_GetString(1, entry->username, sizeof(entry->username));
		}
		entry->besttime = G_COOL_API_DB_GetInt(2);
		entry->runFlags = G_COOL_API_DB_GetInt(4);
		entry->runFlagsDiff = (entry->runFlags ^ lbRequestData.mapDefaultRaceStyle.runFlags) & entry->runFlags; // show all that are active that are different from default
		entry->msec = G_COOL_API_DB_GetInt(5);
		entry->jump = G_COOL_API_DB_GetInt(6);
		G_COOL_API_DB_GetFloat(7,&entry->topSpeed);
		G_COOL_API_DB_GetFloat(8,&entry->average);
		G_COOL_API_DB_GetFloat(15,&entry->distance);
		G_COOL_API_DB_GetString(9,entry->time,sizeof(entry->time));
		entry->savePosCount = G_COOL_API_DB_GetInt(10);
		entry->resposCount = G_COOL_API_DB_GetInt(11);
		entry->duration_ms_segmented_total = G_COOL_API_DB_GetInt(12);
		if (entry->msec == -1) {
			G_COOL_API_DB_GetString(13, entry->fpsString, sizeof(entry->fpsString));
			Q_strncpyz(entry->fpsString, DF_FormatFpsString(entry->fpsString), sizeof(entry->fpsString));
		}
		if (type == LB_SEGMENTED) {
			static raceStyle_t raceStyle;
			memset(&raceStyle,0,sizeof(raceStyle));
			raceStyle.msec = entry->msec;
			raceStyle.runFlags = entry->runFlags;
			raceStyle.jumpLevel = entry->jump;
			raceStyle.runFlags &= ~RFL_SEGMENTED;
			entry->mainLBCompatible = classifyLeaderBoard(&raceStyle,&lbRequestData.mapDefaultRaceStyle) == LB_MAIN;
		}
		else {
			entry->mainLBCompatible = qfalse;
		}
		if (userid != -1) {
			//trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1#%d %-10s %10s.\n\"", rank, userid == -1 ? "!unlogged!": username, DF_MsToString(besttime)));
			//maxrank = MAX(maxrank, rank);
			maxrank = MAX(maxrank, rankHere+1);
			//rank++;
		}
	}

	// TODO how to not make it look bad at page 9 or so? when it goes from 99 to 100?
#define TOPNUMBERSTRING (i == 10 ? "UL" : (offsetRank<10 ? topNumberStrings[i] : miniva("%d",offsetRank+1)))
#define MSECSTRING(msec) ((msec) == -1 ? "togl" : ((msec) == -2 ? "flt" : ((msec) == 0 ? "unkn" : multiva("%d", 1000 / (msec)))))
#define LBROW(lbType,coloration,jumpvalue) !entriesHere[lbType].exists ? ' ' :'#', !entriesHere[lbType].exists ? "  " : TOPNUMBERSTRING, coloration(entriesHere[lbType]), entriesHere[lbType].exists ? entriesHere[lbType].username : "", entriesHere[lbType].exists ? MSECSTRING(entriesHere[lbType].msec) : "" jumpvalue(entriesHere[lbType],lbType), !entriesHere[lbType].exists ? "" : DF_MsToString(entriesHere[lbType].besttime)

#define LBROWFULL_STRING "  ^c%11s  %11s  %11s  %s"

#define LBROWFULL(lbType,coloration,jumpvalue) LBROW(lbType,coloration,jumpvalue),!entriesHere[lbType].exists ? "" :miniva("%.2favg",entriesHere[lbType].average),!entriesHere[lbType].exists ? "" :miniva("%.2ftop",entriesHere[lbType].topSpeed),!entriesHere[lbType].exists ? "" :miniva("%ddist",(int)entriesHere[lbType].distance),!entriesHere[lbType].exists ? "" :entriesHere[lbType].time

#define JUMPVALUE(a,b) ,entriesHere[b].exists ? 'j':' ' ,(entriesHere[b].exists ? miniva("%-2d",(a).jump) : "  ")
#define JUMPVALUE_EMPTY(a,b) 
#define TIMECOLOR_DEFAULT(a) '7'
#define TIMECOLOR_CHEAT(a) ((((a).runFlags & RFL_TAS)||((a).runFlags & RFL_BOT)) ? (((a).runFlags & RFL_SEGMENTED) ? 'j':'1') : '7' )
#define TIMECOLOR_CUSTOM(a) (((a).runFlagsDiff & RFL_CLIMBTECH) ? 'E':'7')
#define TIMECOLOR_SEGMENTED(a) ((a).mainLBCompatible ? '2':'7')
	if (lbRequestData.type == TOPREQUEST_SPECIFICLB) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^2    %-27s\n\"", leaderboardNames[lbRequestData.lbTypeIfSpecific].string));
	} 
	else {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^2    %-27s^h|     ^2%-27s^h|     ^2%-31s^h|     ^2%-27s^h|     ^2%-29s\n\"", "MAIN","NOJUMPBUG","CUSTOM","SEGMENTED", "CHEAT"));
	}
	offsetRank = lbRequestData.page * 10;
	for (i = 0; i < 11; i++, offsetRank++) {
		topLeaderBoardEntry_t* entriesHere = entries[i];
		if (i >= maxrank && i < 10) continue;

		if(lbRequestData.type == TOPREQUEST_SPECIFICLB){
			switch (lbRequestData.lbTypeIfSpecific) {
			case LB_MAIN:
				trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
					"^J%c%02s^%c %-10s ^c%4s ^u%10s" LBROWFULL_STRING
					"\n\"",
					i == 10 ? "\n" : "",
					LBROWFULL(LB_MAIN, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
				));
				break;
			case LB_NOJUMPBUG:
				trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
					"^J%c%02s^%c %-10s ^c%4s ^u%10s" LBROWFULL_STRING
					"\n\"",
					i == 10 ? "\n" : "",
					LBROWFULL(LB_NOJUMPBUG, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
				));
				break;
			case LB_CUSTOM:
				trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
					"^J%c%02s^%c %-10s ^c%4s %c%s ^u%10s" LBROWFULL_STRING "  ^c%s%s"
					"\n\"",
					i == 10 ? "\n" : "",
					LBROWFULL(LB_CUSTOM, TIMECOLOR_CUSTOM, JUMPVALUE),
					!entriesHere[LB_CUSTOM].exists? "" : RunFlagsToString(entriesHere[LB_CUSTOM].runFlags, lbRequestData.mapDefaultRaceStyle.runFlags /*defaultRunFlags level.mapDefaultRaceStyle.runFlags*/, 1, NULL, NULL), // todo make it relative to the relevant map
					(!entriesHere[LB_CUSTOM].exists || entriesHere[LB_CUSTOM].msec != -1) ? "" : multiva(" fps:%s", entriesHere[LB_CUSTOM].fpsString)
				));
				break;
			case LB_SEGMENTED:
				trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
					"^J%c%02s^%c %-10s ^c%4s ^u%10s" LBROWFULL_STRING "  ^c%s%s"
					"\n\"",
					i == 10 ? "\n" : "",
					LBROWFULL(LB_SEGMENTED, TIMECOLOR_SEGMENTED, JUMPVALUE_EMPTY),
					!entriesHere[LB_SEGMENTED].exists ? "" : miniva("(%dSP/%dRP/%s)", entriesHere[LB_SEGMENTED].savePosCount, entriesHere[LB_SEGMENTED].resposCount, DF_MsToString(entriesHere[LB_SEGMENTED].duration_ms_segmented_total)),
					(!entriesHere[LB_SEGMENTED].exists || entriesHere[LB_SEGMENTED].msec != -1) ? "" : multiva(" fps:%s", entriesHere[LB_SEGMENTED].fpsString)
				));
				break;
			case LB_CHEAT:
				trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
					"^J%c%02s^%c %-10s ^c%4s ^u%10s" LBROWFULL_STRING "  ^c%s%s"
					"\n\"",
					i == 10 ? "\n" : "",
					LBROWFULL(LB_CHEAT, TIMECOLOR_CHEAT, JUMPVALUE_EMPTY),
					(!entriesHere[LB_CHEAT].exists || !(entriesHere[LB_CHEAT].runFlags & RFL_SEGMENTED)) ? "" : miniva("(%dSP/%dRP/%s)", entriesHere[LB_CHEAT].savePosCount, entriesHere[LB_CHEAT].resposCount, DF_MsToString(entriesHere[LB_CHEAT].duration_ms_segmented_total)),
					(!entriesHere[LB_CHEAT].exists || entriesHere[LB_CHEAT].msec != -1) ? "" : multiva(" fps:%s", entriesHere[LB_CHEAT].fpsString)
				));
				break;
			}
		}
		else {
			trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
				"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
				"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
				"^J%c%02s^%c %-10s ^c%4s %c%s ^u%10s ^h| " // so middle (custom) column is 4 wider
				"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
				"^J%c%02s^%c %-10s ^c%4s ^u%10s "
				"\n\"",
				i==10 ? multiva("%31s^h|%32s^h|%36s^h|%32s^h|%32s\n","","","","","") : "",
				LBROW(LB_MAIN, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
				,LBROW(LB_NOJUMPBUG, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
				,LBROW(LB_CUSTOM, TIMECOLOR_CUSTOM, JUMPVALUE)
				,LBROW(LB_SEGMENTED, TIMECOLOR_SEGMENTED, JUMPVALUE_EMPTY)
				,LBROW(LB_CHEAT, TIMECOLOR_CHEAT, JUMPVALUE_EMPTY)
				));
		}
	}
	
	//trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^7color explanation:\n^7    %-27s      ^7%-27s      ^7%-27s      ^7%-27s^      ^7%-29s\n\"", "MAIN", "NOJUMPBUG", "CUSTOM", "SEGMENTED", "CHEAT"));
	if (lbRequestData.type != TOPREQUEST_SPECIFICLB) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^7username color explanation: ^2%-12s ^E%-12s ^1%-12s ^j%-12s\n^7for more details, request specific leaderboard\n\"", "main leaderboard compatible settings", "climbtech", "strafebot/TAS", "strafebot/TAS+segmented"));
		trap_SendServerCommand(lbRequestData.clientnum, "print \"^7Specific leaderboard commands: ^c/topmain^7, ^c/topnojumpbug^7, ^c/topcustom^7, ^c/topsegmented^7, ^c/topcheat\n\"");
	}
	else {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^7username color explanation: ^2%-12s ^E%-12s ^1%-12s ^j%-12s\n^7to see an overview of all leaderboards, simply use ^c/top\n\"", "main leaderboard compatible settings", "climbtech", "strafebot/TAS", "strafebot/TAS+segmented"));
	}

}
static void G_LatestRunsResult(int status, const char* errorMessage, int affectedRows) {
	latestRunsRequestStruct_t lbRequestData;
	gentity_t* ent = NULL;
	int resultIndex = 0;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d latest results returned, user no longer valid.\n", lbRequestData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateRunsTable();
		trap_SendServerCommand(lbRequestData.clientnum,"print \"^1Latest results display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1Latest results failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	trap_SendServerCommand(ent - g_entities, "print \"Latest runs:\n\"");

	while (G_COOL_API_DB_NextRow()) {
		int userid,duration_ms;
		qboolean mapDefaultsFound;
		raceStyle_t raceStyle;
		raceStyle_t mapDefaultRaceStyle;
		char username[USERNAME_MAX_LEN+1+10]; // some extra buffer for !unlogged! colored
		char course[COURSENAME_MAX_LEN+1];
		char subcourse[COURSENAME_MAX_LEN +1];
		char runwhen[30];
		char colorChar;
		mainLeaderboardType_t lbType;

		if (resultIndex == 0) {
			trap_SendServerCommand(ent - g_entities, va("print \"^%c%12s %-7s %-10s %-23s %-4s %-4s %-10s %-20s %s\n\""
				, '2'
				, ""
				, "STYLE"
				, "USERNAME"
				, "DATE"
				, "FPS"
				, "JUMP"
				, "TIME"
				, "MAP/COURSE"
				, "RUNFLAGS"
			));
		}

		userid = G_COOL_API_DB_GetInt(0);
		G_COOL_API_DB_GetString(2, course, sizeof(course));
		G_COOL_API_DB_GetString(3, subcourse, sizeof(subcourse));
		raceStyle.movementStyle = G_COOL_API_DB_GetInt(4);
		raceStyle.msec = G_COOL_API_DB_GetInt(5);
		raceStyle.jumpLevel = G_COOL_API_DB_GetInt(6);
		raceStyle.variant = G_COOL_API_DB_GetInt(7);
		raceStyle.runFlags = G_COOL_API_DB_GetInt(8);
		mapDefaultsFound = !G_COOL_API_DB_GetInt(9);
		if (!mapDefaultsFound) {
			memcpy(&mapDefaultRaceStyle, &defaultRaceStyle, sizeof(mapDefaultRaceStyle));
		}
		else {
			mapDefaultRaceStyle.movementStyle = raceStyle.movementStyle;
			mapDefaultRaceStyle.msec = G_COOL_API_DB_GetInt(10);
			mapDefaultRaceStyle.jumpLevel = G_COOL_API_DB_GetInt(11);
			mapDefaultRaceStyle.variant = G_COOL_API_DB_GetInt(12);
			mapDefaultRaceStyle.runFlags = G_COOL_API_DB_GetInt(13);
		}
		duration_ms = G_COOL_API_DB_GetInt(14);
		G_COOL_API_DB_GetString(15, runwhen, sizeof(runwhen));

		lbType = classifyLeaderBoard(&raceStyle, &mapDefaultRaceStyle);

		colorChar = lbType == LB_MAIN ? '7' : 'O';

		if (userid == -1) {
			//Q_strncpyz(username, "!unlogged!", sizeof(username));
			Com_sprintf(username, sizeof(username), "^1!^%cunlogged^1!^%c", colorChar, colorChar);
		}
		else {
			G_COOL_API_DB_GetString(1, username, sizeof(username));
			Com_sprintf(username, sizeof(username), "%-10s", username);
		}

		trap_SendServerCommand(ent - g_entities, va("print \"^%c%12s %-7s %s %-23s %-4s %-4d %-10s %-20s %s\n\""
			, colorChar
			, miniva("[%s]", leaderboardNames[lbType].string)
			, raceStyle.movementStyle < MV_NUMSTYLES ? moveStyleNames[raceStyle.movementStyle].string : "UNKNOWN"
			, username
			, runwhen
			, MSECSTRING(raceStyle.msec)
			, raceStyle.jumpLevel
			, DF_MsToString(duration_ms)
			, subcourse[0] ? multiva("%s/%s", course, subcourse) : course
			, RunFlagsToString(raceStyle.runFlags,mapDefaultRaceStyle.runFlags,0,NULL,NULL)
		));
		resultIndex++;
	}

	trap_SendServerCommand(ent - g_entities, va("print \"\n\""));

	if (!lbRequestData.styleSpecified && !lbRequestData.pageSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can specify movement style and page number for ^2/latest^7.\n\""));
	}
	else if (!lbRequestData.pageSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can also specify page number for ^2/latest^7.\n\""));
	} else if (!lbRequestData.styleSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can also specify movement style for ^2/latest^7.\n\""));
	}

	if (lbRequestData.userId == -2) {

		trap_SendServerCommand(ent - g_entities, va("print \"When logged in, you can call ^2/latest mine^7 to see maps you played recently.\n\""));
		trap_SendServerCommand(ent - g_entities, va("print \"You can also call ^2/latest unlogged^7 to see maps recently played by unlogged players.\n\""));
	}

}

static void G_ShortestLongestResult(int status, const char* errorMessage, int affectedRows) {
	longestShortestMapsRequestStruct_t lbRequestData;
	gentity_t* ent = NULL;
	int resultIndex = 0;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d shortest/longest map results returned, user no longer valid.\n", lbRequestData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateMapRaceDefaultsTable();
		G_CreateRunsTable();
		trap_SendServerCommand(lbRequestData.clientnum,"print \"^1Shortest/longest map results display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1Shortest/longest map results failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	if (lbRequestData.longest) {
		trap_SendServerCommand(ent - g_entities, va("print \"Longest maps in style %s (based on fastest run including segmented/cheat):\n\"", lbRequestData.style < MV_NUMSTYLES ? moveStyleNames[lbRequestData.style].string : "UNKNOWN"));
	}
	else {
		trap_SendServerCommand(ent - g_entities, va("print \"Shortest maps in style %s (based on fastest run including segmented/cheat):\n\"", lbRequestData.style < MV_NUMSTYLES ? moveStyleNames[lbRequestData.style].string : "UNKNOWN"));
	}

	while (G_COOL_API_DB_NextRow()) {
		char course[COURSENAME_MAX_LEN+1];
		char subcourse[COURSENAME_MAX_LEN +1];
		int time;
		int mapnum;
		infoHashed_t* infoHashed;
		mainLeaderboardType_t lbType;

		if (resultIndex == 0) {
			trap_SendServerCommand(ent - g_entities, va("print \"^%c%10s %-7s %-20s\n\""
				, '2'
				, "TIME"
				, "MAPNUM"
				, "MAP/COURSE"
			));
		}
		time = G_COOL_API_DB_GetInt(0);
		G_COOL_API_DB_GetString(1, course, sizeof(course));
		G_COOL_API_DB_GetString(2, subcourse, sizeof(subcourse));

		infoHashed = G_GetArenaInfoByMap(course);

		trap_SendServerCommand(ent - g_entities, va("print \"^%c%10s %-7s %-20s\n\""
			, '7'
			, DF_MsToString(time)
			, infoHashed ? miniva("%d",infoHashed-g_arenaInfosHashed) : "-"
			, subcourse[0] ? multiva("%s/%s", course, subcourse) : course
		));
		resultIndex++;
	}

	trap_SendServerCommand(ent - g_entities, va("print \"\n\""));

	if (!lbRequestData.styleSpecified && !lbRequestData.pageSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can specify movement style and page number.\n\""));
	}
	else if (!lbRequestData.pageSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can also specify page number.\n\""));
	} else if (!lbRequestData.styleSpecified) {
		trap_SendServerCommand(ent - g_entities, va("print \"Note: You can also specify movement style.\n\""));
	}


}
static void G_MapListUnplayedResult(int status, const char* errorMessage, int affectedRows) {
	maplistUnplayedRequestStruct_t data;
	gentity_t* ent = NULL;
	int resultIndex = 0;
	//evaluatedRunInfo_t eRunInfo;

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d unplayed maplist returned, user no longer valid.\n", data.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateRunsTable();
		trap_SendServerCommand(data.clientnum,"print \"^1Unplayed maplist display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(data.clientnum, va("print \"^1Unplayed maplist failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}
	else {

		int			mapsinmessage = 0;
		const char*	mapName = NULL;
		char		mapListString[1024];
		char		currentMapString[1024];
		char		currentMap[COURSENAME_MAX_LEN + 1];
		qboolean	first = qtrue;
		//int			n = 0;
		//int			milliseconds = 0;
		int			mapsInFrame = 0;
		int			mapNum;
		infoHashed_t* mapInfo;


		Q_strncpyz(mapListString, "", sizeof(mapListString));
		trap_SendServerCommand(ent - g_entities, va("print \"^2----------^7INSTALLED MAPS^2---------\n\""));

		while (G_COOL_API_DB_NextRow()) {
			G_COOL_API_DB_GetString(0, currentMap, sizeof(currentMap));

			mapInfo = G_GetArenaInfoByMap(currentMap);
			if (!mapInfo) {
				continue;
			}

			mapName = mapInfo->name; //Info_ValueForKey(g_arenaInfosHashed[n]., "map");
			mapNum = mapInfo - g_arenaInfosHashed;

			if (strlen(mapName) < 1 || !Q_stricmp(mapName, "<NULL>")) {

				if (mapNum == (g_numArenas - 1)) {
					mapsInFrame += 5;
					trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", mapListString));
					if (mapsInFrame >= 300) {
						mapsInFrame = 0;
						//milliseconds += 100;
					}
					Q_strncpyz(mapListString, "", sizeof(mapListString));
					mapsinmessage = 0;
				}
				continue;
			}

			Q_strncpyz(currentMap, mapName, 24);
			Com_sprintf(currentMapString, sizeof(currentMapString), "^7[^2%03i^7] %-24s", mapNum, currentMap);
			Q_strcat(mapListString, sizeof(mapListString), currentMapString);

			mapsinmessage = mapsinmessage + 1;

			if ((mapsinmessage >= 5) || (mapNum == (g_numArenas - 1))) {
				mapsInFrame += 5;
				trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", mapListString));
				if (mapsInFrame >= 300) {
					mapsInFrame = 0;
					//milliseconds += 100;
				}

				Q_strncpyz(mapListString, "", sizeof(mapListString));
				mapsinmessage = 0;
			}
		}

		if ((mapsinmessage >= 1)) {
			mapsInFrame += 5;
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", mapListString));
			if (mapsInFrame >= 300) {
				mapsInFrame = 0;
				//milliseconds += 100;
			}

			Q_strncpyz(mapListString, "", sizeof(mapListString));
			mapsinmessage = 0;
		}


	}




}
static void G_SubContestLBResult(int status, const char* errorMessage, int affectedRows) {
	subContestLeaderboardRequestStruct_t lbRequestData;
	gentity_t* ent = NULL;
	//int rank = 1;
	int index = 0;

	G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d subcontest results returned, user no longer valid.\n", lbRequestData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateSubContestsTable();
		trap_SendServerCommand(lbRequestData.clientnum,"print \"^1Subcontest display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1Subcontest display failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}

	while (G_COOL_API_DB_NextRow()) {
		int userid,msec,extraValue3,extraValue4;
		float value,extraValue1,extraValue2;
		int realRank;
		static char coursename[COURSENAME_MAX_LEN + 1];
		static char when[20];
		static char username[USERNAME_MAX_LEN + 1];

		userid = G_COOL_API_DB_GetInt(0);
		realRank = G_COOL_API_DB_GetInt(10)-1;

		if (userid != -1 && (realRank < lbRequestData.page * 10 || realRank >= ((lbRequestData.page + 1) * 10))) continue;

		if (!index) {
			trap_SendServerCommand(lbRequestData.clientnum, va("print \"^2ROLLYMPICS\n"));
		}
		G_COOL_API_DB_GetString(1, username, sizeof(username));
		G_COOL_API_DB_GetFloat(2, &value);
		G_COOL_API_DB_GetString(3, when, sizeof(when));
		G_COOL_API_DB_GetString(4, coursename, sizeof(coursename));
		msec = G_COOL_API_DB_GetInt(5);
		G_COOL_API_DB_GetFloat(6, &extraValue1);
		G_COOL_API_DB_GetFloat(7, &extraValue2);
		extraValue3 = G_COOL_API_DB_GetInt(8);
		extraValue4 = G_COOL_API_DB_GetInt(9);


		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^3%-3s ^7%-10s  ^3%4.2f^7ups ^3%6s^7fps ^3%s ^7on ^3%s\n\"",userid==-1 ? "" : miniva("#%d",realRank+1), userid==-1 ?"!unlogged!" : username, value, MSECSTRING(msec), when, coursename));

		if (userid != -1) {
			//rank++;
		}
		index++;
	}

	

}


void DF_RequestPlayerDefaultTime(gentity_t* ent);


static void G_TimeResult(int status, const char* errorMessage, int affectedRows) {
	timeRequestStruct_t lbRequestData;
	gentity_t* ent = NULL;

	G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d time returned, user no longer valid.\n", lbRequestData.clientnum);
		return;
	}

	if (status == 1146) {
		// table doesn't exist. create it.
		G_CreateUserTable();
		G_CreateRunsTable();
		trap_SendServerCommand(lbRequestData.clientnum,"print \"^1Time display failed due to table not existing. Attempting to create. Please try again shortly.\n\"");
		return;
	}
	else if (status) {
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1Time display failed with status %d and error message %s.\n\"", status, errorMessage));
		return;
	}
	
	if (Q_stricmp(lbRequestData.course, DF_GetCourseName()) && lbRequestData.forUserInfo) {
		// this isn't the correct course.
		Com_Printf("^1Coursename changed, requested time not useful. Requesting new pb for client %d",ent-g_entities);
		DF_RequestPlayerDefaultTime(ent);
		return;
	}

	if (G_COOL_API_DB_NextRow()) {
		int time;
		time = G_COOL_API_DB_GetInt(0);

		if (lbRequestData.forUserInfo) {
			//if (time != ent->client->pers.raceBestTime) { // dont check, this is just called from login, which doesnt do the calc, so we always do it.
				ent->client->pers.raceBestTime = time;
				CalculateRanks();
				ClientUserinfoChanged(ent - g_entities);
			//}
		}
		else {

			if (!Q_stricmp(DF_GetCourseName(), lbRequestData.course)) {
				if (lbRequestData.subcourse[0]) {
					trap_SendServerCommand(-1, va("print \"%s's ^7best time on %s leaderboard in style %s on subcourse %s is %s\n\"", ent->client->pers.netname, leaderboardNames[lbRequestData.lbType].string, moveStyleNames[lbRequestData.style].string, lbRequestData.subcourse, DF_MsToString(time)));
				}
				else
				{
					trap_SendServerCommand(-1, va("print \"%s's ^7best time on %s leaderboard in style %s is %s\n\"", ent->client->pers.netname, leaderboardNames[lbRequestData.lbType].string, moveStyleNames[lbRequestData.style].string, DF_MsToString(time)));
				}
			}
			else {
				if (lbRequestData.subcourse[0]) {
					trap_SendServerCommand(-1, va("print \"%s's ^7best time on %s leaderboard in style %s on %s/%s is %s\n\"", ent->client->pers.netname, leaderboardNames[lbRequestData.lbType].string, moveStyleNames[lbRequestData.style].string, lbRequestData.course, lbRequestData.subcourse, DF_MsToString(time)));
				}
				else
				{
					trap_SendServerCommand(-1, va("print \"%s's ^7best time on %s leaderboard in style %s on %s is %s\n\"", ent->client->pers.netname, leaderboardNames[lbRequestData.lbType].string, moveStyleNames[lbRequestData.style].string, lbRequestData.course, DF_MsToString(time)));
				}
			}
		}
	}
	else if (lbRequestData.forUserInfo) {

		ent->client->pers.raceBestTime = 0;
		CalculateRanks();
	} 

	

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
	client->sess.login.forceLoggedIn = qfalse;
	//if (client->pers.raceBestTime) {
		client->pers.raceBestTime = 0;
		//CalculateRanks(); // we do this in the response handler for DF_RequestPlayerDefaultTime, to avoid audio spam of rank changes
	//}
	DF_SetSubContestDefaults(client);

	DF_RequestPlayerDefaultTime(ent);

	trap_SendServerCommand(loginData->clientnum, va("print \"^2Successfully logged in as '%s'.\n\"",loginData->username));
	//trap_SendServerCommand(-1, va("print \"^2%s ^7logged in as '%s'.\n\"",client ? client->pers.netname : "", loginData->username));

	ClientUserinfoChanged(ent - g_entities);

	// fire and forget, not that important
	G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_LOGIN_UPDATELASTLOGIN,
		va("UPDATE users SET lastlogin=NOW() WHERE id=%d", loginData->userId));
}

static void G_ForceLoginContinue(int status, const char* errorMessage, int affectedRows) {
	static char		cryptedPw[MAX_STRING_CHARS];
	const char* request = NULL;
	gentity_t* ent = NULL;
	gclient_t* client = NULL;
	gentity_t* adminEnt = NULL;
	loginRegisterStruct_t data;
	char usernameDb[USERNAME_MAX_LEN + 1];

	G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(adminEnt = DB_VerifyClient(data.clientnumAdmin, data.ipAdmin))) {
		Com_Printf("^1Client %d force login as %s returned, admin no longer valid.\n", data.clientnum, data.username);
	}
	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		if (adminEnt) {
			trap_SendServerCommand(data.clientnumAdmin, va("print \"^1Client %d force login as %s returned, user no longer valid.\n\"", data.clientnum, data.username));
		}
		else {
			Com_Printf("^1Client %d force login as %s returned, user no longer valid.\n", data.clientnum, data.username);
		}
		return;
	}

	if (!G_COOL_API_DB_NextRow()) {
		if (adminEnt) {
			trap_SendServerCommand(data.clientnumAdmin, va("print \"^1Client %d force login as %s returned, username not found.\n\"", data.clientnum, data.username));
		}
		else {
			Com_Printf("^1Client %d force login as %s returned, username not found.\n", data.clientnum, data.username);
		}
		return;
	} 
	//flags, id, username
	data.userFlags = G_COOL_API_DB_GetInt(0);
	data.userId = G_COOL_API_DB_GetInt(1);
	G_COOL_API_DB_GetString(2, usernameDb,sizeof(usernameDb));

	if (Q_stricmp(usernameDb, data.username)) {
		if (adminEnt) {
			trap_SendServerCommand(data.clientnumAdmin, va("print \"^1Client %d force login as %s returned, DB username %s does not match WTF.\n\"", data.clientnum, data.username, usernameDb));
		}
		else {
			Com_Printf("^1Client %d force login as %s returned, DB username %s does not match WTF.\n", data.clientnum, data.username, usernameDb);
		}
		return;
	}

	client = ent->client;

	Q_strncpyz(client->sess.login.name, usernameDb,sizeof(client->sess.login.name));
	client->sess.login.id = data.userId;
	client->sess.login.flags = data.userFlags;
	client->sess.login.loggedIn = qtrue;
	client->sess.login.forceLoggedIn = qtrue;
	//if (client->pers.raceBestTime) {
		client->pers.raceBestTime = 0;
		//CalculateRanks(); // we do this in the response handler for DF_RequestPlayerDefaultTime, to avoid audio spam of rank changes
	//}
	DF_SetSubContestDefaults(client);

	DF_RequestPlayerDefaultTime(ent);

	trap_SendServerCommand(data.clientnum, va("print \"^3You were force-logged in by an admin as '%s'. Change your password with /changepassword, then log out and log in again.\n\"", usernameDb));
	//trap_SendServerCommand(-1, va("print \"^2%s ^7logged in as '%s'.\n\"",client ? client->pers.netname : "", loginData->username));

	if (adminEnt) {
		trap_SendServerCommand(data.clientnumAdmin, va("print \"^3Client %d was force-logged in as %s.\n\"", data.clientnum,  usernameDb));
	}

	ClientUserinfoChanged(ent - g_entities);

	// fire and forget, not that important
	//G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_LOGIN_UPDATELASTLOGIN,
	//	va("UPDATE users SET lastlogin=NOW() WHERE id=%d", data.userId));
}

static void G_CreateTableResult(int status, const char* errorMessage) {
	static referenceSimpleString_t tableName;
	G_COOL_API_DB_GetReference((byte*)&tableName, sizeof(tableName));
	if (status) {
		Com_Printf("creating table %s failed with status %d and error message %s.\n", tableName.s, status, errorMessage);
		return;
	}
	Com_Printf("creating table %s was successful.\n", tableName.s);

}
static void G_UpdateColumnsResult(int status, const char* errorMessage) {
	static referenceSimpleString_t tableName;
	G_COOL_API_DB_GetReference((byte*)&tableName, sizeof(tableName));
	if (status) {
		Com_Printf("updating columns for table %s failed with status %d and error message %s.\n", tableName.s, status, errorMessage);
		return;
	}
	Com_Printf("updating columns for table %s was successful.\n", tableName.s);

}

static void G_PWBCryptReturned(int status, const char* errorMessage) {
	static loginRegisterStruct_t loginData;
	gentity_t* ent;

	G_COOL_API_DB_GetReference((byte*)&loginData, sizeof(loginData));

	if (!(ent = DB_VerifyClient(loginData.clientnum, loginData.ip))) {
		Com_Printf("^1bcrypt succeeded, but user no longer valid (#2).\n");
		return;
	}

	if (status) {
		trap_SendServerCommand(loginData.clientnum,va("print \"^1Password bcrypting failed with status %d and error %s.\n\"", status, errorMessage));
		return;
	}
	if (G_COOL_API_DB_NextRow()) {
		if (!G_COOL_API_DB_GetString(0, loginData.password, sizeof(loginData.password))) {
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
			case DBREQUEST_CHANGEPASSWORD:
				G_ChangePasswordContinue(&loginData);
				break;
		}

#ifdef BCRYPTDEBUG
		if (g_developer.integer) {
			trap_SendServerCommand(loginData.clientnum, va("print \"G_Login_PWBCryptReturned: Client %d (user %s), Crypted pw: %s\n\"", loginData.clientnum, loginData.username, loginData.password));
		}
#endif
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
		while (G_COOL_API_DB_NextResponse(&requestType, &affectedRows, &status, errorMessage, sizeof(errorMessage), NULL, 0)) {
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
				case DBREQUEST_CHANGEPASSWORD:
					G_ChangePasswordResult(status, errorMessage);
					break;
				case DBREQUEST_CREATETABLE:
					G_CreateTableResult(status, errorMessage);
					break;
				case DBREQUEST_UPDATECOLUMNS:
					G_UpdateColumnsResult(status, errorMessage);
					break;
				case DBREQUEST_LOGIN:
					G_LoginFetchDataResult(status, errorMessage);
					break;
				case DBREQUEST_INSERTORUPDATERUN:
					G_InsertRunResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_INSERTORUPDATESUBCONTEST:
					G_InsertSubcontestResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS:
					G_InsertMapDefaultsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_LOADMAPRACEDEFAULTS:
					G_LoadMapDefaultsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_TOP:
					G_TopResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_GETLATESTRUNS:
					G_LatestRunsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_SHORTESTLONGESTMAPS:
					G_ShortestLongestResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_MAPLISTUNPLAYED:
					G_MapListUnplayedResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_SUBCONTESTLEADERBOARD:
					G_SubContestLBResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_TIME:
					G_TimeResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_SAVECHECKPOINTS:
					G_SaveCheckpointsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_LOADCHECKPOINTS:
					G_LoadCheckpointsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_TOPMAPSEARCH:
					G_TopMapSearchResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_ARENAGENMAPLIST:
					G_ArenaGenMapListResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_FORCEDLOGIN:
					G_ForceLoginContinue(status, errorMessage, affectedRows);
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
	if (G_COOL_API_DB_EscapeString(text, sizeof(text))) {
		request = va("INSERT INTO chats (chat,`time`) VALUES ('%s',NOW())", text);
		G_COOL_API_DB_AddRequest(NULL, 0, DBREQUEST_CHATSAVE, request);
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

	G_COOL_API_DB_AddRequest(NULL,0, DBREQUEST_GETCHATS, va("SELECT id, chat, `time` FROM chats ORDER BY time DESC, id DESC LIMIT %d,10",first));
}
*/
void G_DB_SaveUserCheckpoints(gentity_t* playerent) {
	static const char requestBase[] = "DELETE FROM checkpoints WHERE course=? AND userid=?;INSERT INTO checkpoints (userid,course,number,x,y,z,yaw) VALUES ";
	static const char checkPointValues[] = "(?,?,?,?,?,?,?)";
	static char request[sizeof(requestBase) + (sizeof(checkPointValues)+1)*MAX_CUSTOM_CHECKPOINT_COUNT+1];
	const char* coursename = NULL;
	static checkPointSaveRequestStruct_t data;
	int i;
	if (coolApi_dbVersion < 3) {
		G_SendServerCommand(playerent-g_entities,"print \"DB version too low to save checkpoints.\n\"",qtrue);
		return;
	}
	if (!playerent->client->pers.df_checkpointData.count) {
		G_SendServerCommand(playerent-g_entities,"print \"No checkpoints found for saving.\n\"",qtrue);
		return;
	}
	if (!playerent->client->sess.login.loggedIn) {
		G_SendServerCommand(playerent-g_entities,"print \"Can't save checkpoints unless logged in.\n\"",qtrue);
		return;
	}
	request[0] = 0;
	Q_strcat(request, sizeof(request), requestBase);
	Q_strcat(request, sizeof(request), checkPointValues);
	for (i = 1; i < playerent->client->pers.df_checkpointData.count; i++) {
		Q_strcat(request, sizeof(request), va(",%s",checkPointValues));
	}
	memset(&data, 0, sizeof(data));
	data.clientnum = playerent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_SAVECHECKPOINTS,request)) {
		G_SendServerCommand(playerent - g_entities, "print \"DB connection not available to save checkpoints.\n\"",qtrue);
		return;
	}
	coursename = DF_GetCourseName();

	// DELETE
	G_COOL_API_DB_PreparedBindString(coursename);
	G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);

	// INSERT
	for (i = 0; i < playerent->client->pers.df_checkpointData.count; i++) {
		gentity_t* check = g_entities + playerent->client->pers.df_checkpointData.checkpointNumbers[i];
		G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);
		G_COOL_API_DB_PreparedBindString(coursename);
		G_COOL_API_DB_PreparedBindInt(i);
		G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[0]);
		G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[1]);
		G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[2]);
		G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.anglesYaw);
	}

	G_COOL_API_DB_FinishAndSendPreparedStatement();
}
void G_DB_LoadUserCheckpoints(gentity_t* playerent) {
	static checkPointSaveRequestStruct_t data;
	int i;
	const char* coursename = NULL;
	if (coolApi_dbVersion < 3) {
		G_SendServerCommand(playerent-g_entities,"print \"DB version too low to load checkpoints.\n\"",qtrue);
		return;
	}
	if (!playerent->client->sess.login.loggedIn) {
		G_SendServerCommand(playerent-g_entities,"print \"Can't load checkpoints unless logged in.\n\"",qtrue);
		return;
	}
	memset(&data, 0, sizeof(data));
	data.clientnum = playerent - g_entities;
	memcpy(data.ip, mv_clientSessions[data.clientnum].clientIP, sizeof(data.ip));

	if (!G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data), DBREQUEST_LOADCHECKPOINTS, "SELECT x,y,z,yaw FROM checkpoints WHERE course=? AND userid=? ORDER BY number ASC")) {
		G_SendServerCommand(playerent - g_entities, "print \"DB connection not available to load checkpoints.\n\"",qtrue);
		return;
	}

	coursename = DF_GetCourseName();

	G_COOL_API_DB_PreparedBindString(coursename);
	G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);

	G_COOL_API_DB_FinishAndSendPreparedStatement();
}

static void G_CreateUserTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = va("CREATE TABLE IF NOT EXISTS users(id BIGINT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(%d) UNIQUE NOT NULL, password VARCHAR(64)  NOT NULL, lastlogin DATETIME, created DATETIME NOT NULL, lastip  INT UNSIGNED, flags  INT UNSIGNED NOT NULL DEFAULT 0)",USERNAME_MAX_LEN);
	Q_strncpyz(tableName.s, "users", sizeof(tableName.s));
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}

static void G_CreateCheckpointsTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS checkpoints(id BIGINT AUTO_INCREMENT PRIMARY KEY, userid BIGINT SIGNED NOT NULL, course VARCHAR(100) NOT NULL, number TINYINT(2) SIGNED NOT NULL, x DOUBLE NOT NULL, y DOUBLE NOT NULL, z DOUBLE NOT NULL, yaw DOUBLE NOT NULL, UNIQUE KEY checkpoint_unique (userid,course,number), INDEX i_user_map (userid,course), INDEX i_number(number))";
	Q_strncpyz(tableName.s, "checkpoints", sizeof(tableName.s));
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
static void G_CreateSubContestsTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS subcontests(id BIGINT AUTO_INCREMENT PRIMARY KEY, userid BIGINT SIGNED NOT NULL, course VARCHAR(100) NOT NULL, type SMALLINT NOT NULL, value DOUBLE NOT NULL, recordwhen DATETIME NOT NULL, msec SMALLINT NOT NULL, extraValue1 DOUBLE,extraValue2 DOUBLE,extraValue3 INTEGER,extraValue4 INTEGER, UNIQUE KEY user_type (userid,type),INDEX i_value(value))";
	Q_strncpyz(tableName.s, "subcontests", sizeof(tableName.s));
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
static void G_CreateMapRaceDefaultsTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS mapdefaults(\
			course VARCHAR(100) NOT NULL, \
			subcourse VARCHAR(100) NOT NULL, \
			msec SMALLINT NOT NULL, \
			jump TINYINT NOT NULL, \
			variant SMALLINT NOT NULL,\
			runFlags INT NOT NULL,\
			PRIMARY KEY(course,subcourse))";
	Q_strncpyz(tableName.s, "mapdefaults", sizeof(tableName.s));
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
static void G_CreateRunsTable() {
	referenceSimpleString_t tableName;
#define SUBFUNC(a) `runFlag_ ## a` TINYINT(1)
#define SUBFUNC2(a) `runFlag_ ## a`
#define SUBFUNC3(a)  INDEX `i_ ## runFlag_ ## a` (`runFlag_ ## a`)
#define SUBFUNC4(a)  ALTER TABLE runs ADD COLUMN IF NOT EXISTS `runFlag_ ## a` TINYINT(1)
#define SUBFUNC5(a)  ALTER TABLE runs ADD INDEX IF NOT EXISTS `i_ ## runFlag_ ## a` (`runFlag_ ## a`)
#define SUBFUNC6(a)  ALTER TABLE runs ALTER COLUMN `runFlag_ ## a` DROP DEFAULT
#define RUNFLAGSFUNC(a,b,c,d,e,f) QUOTEME(SUBFUNC(a)) " NOT NULL,"
#define RUNFLAGSFUNC2(a,b,c,d,e,f) "," QUOTEME(SUBFUNC2(a))
#define RUNFLAGSFUNC3(a,b,c,d,e,f) QUOTEME(SUBFUNC3(a)) ","
#define RUNFLAGSFUNC4(a,b,c,d,e,f) QUOTEME(SUBFUNC4(a)) " NOT NULL DEFAULT 0;"
#define RUNFLAGSFUNC5(a,b,c,d,e,f) QUOTEME(SUBFUNC5(a)) ";"
#define RUNFLAGSFUNC6(a,b,c,d,e,f) QUOTEME(SUBFUNC6(a)) ";"
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS runs(\
			id BIGINT AUTO_INCREMENT PRIMARY KEY, \
			userid BIGINT SIGNED NOT NULL, \
			course VARCHAR(100) NOT NULL, \
			subcourse VARCHAR(100) NOT NULL, \
			duration_ms INT UNSIGNED NOT NULL, \
			duration_ms_segmented_total INT UNSIGNED NOT NULL, \
			startLessTime INT UNSIGNED NOT NULL, \
			endLessTime INT NOT NULL, \
			saveposCount INT NOT NULL, \
			resposCount INT NOT NULL, \
			lostMsecCount INT NOT NULL, \
			lostCmdsCount INT NOT NULL, \
			topspeed DOUBLE NOT NULL, \
			rollSpeed DOUBLE NOT NULL, \
			rollTakeoffClientSpeed INT NOT NULL, \
			startTriggerSpeed DOUBLE NOT NULL, \
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
			fpsString VARCHAR(255) NOT NULL, \
			server VARCHAR(255) NOT NULL, \
			hidden TINYINT(1) NOT NULL DEFAULT 0, \
			UNIQUE KEY user_runtype (userid,course,subcourse,style,msec,jump,variant,runFlags"
			//QUOTEME(RUNFLAGS(RUNFLAGSFUNC2))
			"), \
			INDEX i_userid (userid), INDEX i_course_subcourse (course,subcourse), INDEX i_course (course), INDEX i_subcourse (subcourse), \
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
			INDEX i_hidden (hidden), \
			INDEX i_runtype (style,msec,jump,variant,runFlags) );"
			RUNFLAGS(RUNFLAGSFUNC4)
			//RUNFLAGS(RUNFLAGSFUNC5)
			//RUNFLAGS(RUNFLAGSFUNC6)
			"";
	const char* columnsUpdateRequest = ""
			RUNFLAGS(RUNFLAGSFUNC4)
			RUNFLAGS(RUNFLAGSFUNC5)
			RUNFLAGS(RUNFLAGSFUNC6)
			"";
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef RUNFLAGSFUNC3
#undef RUNFLAGSFUNC4
#undef RUNFLAGSFUNC5
#undef SUBFUNC
#undef SUBFUNC2
#undef SUBFUNC3
#undef SUBFUNC4
#undef SUBFUNC5
	
	//if (g_developer.integer) {
	//	G_Printf("TABLE QUERY DEBUG: %s", userTableRequest);
	//}
	// fields without index (cuz just info/debug, dont need to search/filter by it:
	// - distanceXY
	// - startLessTime
	// - endLessTime
	// - saveposCount
	// - resposCount
	// - lostMsecCount
	// - lostCmdsCount
	Q_strncpyz(tableName.s, "runs", sizeof(tableName.s));
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
	G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_UPDATECOLUMNS, columnsUpdateRequest);
}

static void G_DB_CreateTables() {
	G_CreateUserTable();
	G_CreateRunsTable();
	G_CreateCheckpointsTable();
	G_CreateSubContestsTable();
	G_CreateMapRaceDefaultsTable();
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
	const char* lbSQLCondition = NULL;
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


	if (coolApi_dbVersion < 3) {
		trap_SendServerCommand(-1, va("print \"Database API version below < 3. Run cannot be saved.\n\" dfrunsavefailed %s", DF_RacePrintAppendage(runInfo)));
		return qfalse;
	}



#define GETCONNECTIONIP "(select host from information_schema.processlist WHERE ID=connection_id())"
#define SUBFUNC(a,b) `b ## a`
#define RUNFLAGSFUNC(a,b,c,d,e,f) QUOTEME(SUBFUNC(a,d)) "," // gotta do this cuz qvm gets confused by the comma otherwise
#define RUNFLAGSFUNC2(a,b,c,d,e,f) "?,"
#define RUNFLAGSFUNC3(a,b,c,d,e,f) `d ## a`=? AND
	
	lbSQLCondition = getLeaderboardSQLConditions(runInfo->lbType, &level.mapDefaultRaceStyle);
	insertOrUpdateRequest =
		va("SET @now=NOW();"
			"INSERT INTO runs (userid,course,subcourse,duration_ms,duration_ms_segmented_total,topspeed,startTriggerSpeed,rollSpeed,rollTakeoffClientSpeed,average,distance,style,msec,jump,variant,runFlags,"
			RUNFLAGS(RUNFLAGSFUNC)
			"runwhen,runfirst,warningFlags,fpsString, distanceXY,startLessTime,endLessTime,saveposCount,resposCount,lostMsecCount,lostCmdsCount,server)"
			"VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,"
			RUNFLAGS(RUNFLAGSFUNC2)
			"@now,@now,?,?,?,?,?,?,?,?,?," GETCONNECTIONIP ")"
			"ON DUPLICATE KEY UPDATE "
			"duration_ms_segmented_total = IF(?<duration_ms,?,duration_ms_segmented_total),"
			"topspeed = IF(?<duration_ms,?,topspeed),"
			"startTriggerSpeed = IF(?<duration_ms,?,startTriggerSpeed),"
			"rollSpeed = IF(?<duration_ms,?,rollSpeed),"
			"rollTakeoffClientSpeed = IF(?<duration_ms,?,rollTakeoffClientSpeed),"
			"average = IF(?<duration_ms,?,average),"
			"distance = IF(?<duration_ms,?,distance),"
			"runwhen = IF(?<duration_ms,@now,runwhen),"
			"warningFlags = IF(?<duration_ms,?,warningFlags),"
			"fpsString = IF(?<duration_ms,?,fpsString),"
			"distanceXY = IF(?<duration_ms,?,distanceXY),"
			"startLessTime = IF(?<duration_ms,?,startLessTime),"
			"endLessTime = IF(?<duration_ms,?,endLessTime),"
			"saveposCount = IF(?<duration_ms,?,saveposCount),"
			"resposCount = IF(?<duration_ms,?,resposCount),"
			"lostMsecCount = IF(?<duration_ms,?,lostMsecCount),"
			"lostCmdsCount = IF(?<duration_ms,?,lostCmdsCount),"
			"server = IF(?<duration_ms," GETCONNECTIONIP ",server),"
			"duration_ms = IF(?<duration_ms,?,duration_ms);" // duration_ms has to be set last or else all other columns arent updated
			// check if we had a better time on this leaderboard before. (return value of INSERT OR UPDATE only tells us if it was the best with the unique key, but leaderboards accumulate ranges of race settings, especially "custom" leaderboard and such)
			"SELECT COUNT(id) AS countOwnFaster FROM runs WHERE userid=? AND course=? AND subcourse=? AND style=? AND variant=? AND %s AND (duration_ms<? OR (duration_ms=? AND runwhen<@now));"
			// check our new rank.
			"SELECT COUNT(DISTINCT userid) AS countFaster FROM runs WHERE hidden=0 AND userid !=? AND userid!=-1 AND course=? AND subcourse=? AND style=? AND variant=? AND %s AND (duration_ms<? OR (duration_ms=? AND runwhen<@now));" // if someone got the same time as you, but earlier, hes in front of u
			"SELECT (UNIX_TIMESTAMP(@now)-(?*1000000000)) as unixTimeMinus3bill", lbSQLCondition, lbSQLCondition);
	
#undef RUNFLAGSFUNC
#undef RUNFLAGSFUNC2
#undef RUNFLAGSFUNC3
		


	if(!G_COOL_API_DB_AddPreparedStatement((byte*)&runData, sizeof(insertUpdateRunStruct_t), DBREQUEST_INSERTORUPDATERUN,
		insertOrUpdateRequest)) {
		trap_SendServerCommand(-1, va("print \"Database connection not available. Run cannot be saved.\n\" dfrunsavefailed %s", DF_RacePrintAppendage(runInfo)));
		return qfalse;
	}

	// INSERT PART
	G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	G_COOL_API_DB_PreparedBindString(runInfo->subcoursename);
	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->millisecondsSegmentedTotal);
	G_COOL_API_DB_PreparedBindFloat(runInfo->topspeed);
	G_COOL_API_DB_PreparedBindFloat(runInfo->startTriggerSpeed);
	G_COOL_API_DB_PreparedBindFloat(runInfo->rollSpeed);
	G_COOL_API_DB_PreparedBindInt(runInfo->rollTakeoffClientSpeed);
	G_COOL_API_DB_PreparedBindFloat(runInfo->average);
	G_COOL_API_DB_PreparedBindFloat(runInfo->distance);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
#define RUNFLAGSFUNC(a,b,c,d,e,f) G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
	RUNFLAGS(RUNFLAGSFUNC)
#undef RUNFLAGSFUNC

	G_COOL_API_DB_PreparedBindInt(runInfo->warningFlags);
	G_COOL_API_DB_PreparedBindString(runInfo->fpsString);
	G_COOL_API_DB_PreparedBindFloat(runInfo->distanceXY);
	G_COOL_API_DB_PreparedBindInt(runInfo->startLessTime);
	G_COOL_API_DB_PreparedBindInt(runInfo->endLessTime);
	G_COOL_API_DB_PreparedBindInt(runInfo->savePosCount);
	G_COOL_API_DB_PreparedBindInt(runInfo->resposCount);
	G_COOL_API_DB_PreparedBindInt(runInfo->lostMsecCount);
	G_COOL_API_DB_PreparedBindInt(runInfo->lostPacketCount);

	// UPDATE PART
	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->millisecondsSegmentedTotal);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->topspeed);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->startTriggerSpeed);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->rollSpeed);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->rollTakeoffClientSpeed);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->average);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->distance);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds); // runwhen

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->warningFlags);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindString(runInfo->fpsString);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindFloat(runInfo->distanceXY);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->startLessTime);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->endLessTime);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->savePosCount);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->resposCount);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->lostMsecCount);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->lostPacketCount);

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds); // server (value is hardcoded)

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	// SECOND QUERY - SELECT OUR BEST TIME
	G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	G_COOL_API_DB_PreparedBindString(runInfo->subcoursename);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

	//#define RUNFLAGSFUNC(a,b,c) G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
		//RUNFLAGS(RUNFLAGSFUNC)
		//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
	//#undef RUNFLAGSFUNC

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	// THIRD QUERY - SELECT RANK
	G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	G_COOL_API_DB_PreparedBindString(runInfo->subcoursename);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

//#define RUNFLAGSFUNC(a,b,c) G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
	//RUNFLAGS(RUNFLAGSFUNC)
	//G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
//#undef RUNFLAGSFUNC

	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	//if (coolApi_dbVersion >= 3) {
		G_COOL_API_DB_PreparedBindInt(runInfo->unixTimeStampShiftedBillionCount);
	//}

	G_COOL_API_DB_FinishAndSendPreparedStatement();
	//Q_strncpyz(tableName.s, "runs", sizeof(tableName.s));
	//G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
	return qtrue;
}













qboolean	trap_G_COOL_API_DB_EscapeString(char* input, int size);
qboolean	trap_G_COOL_API_DB_AddRequest(byte* reference, int referenceLength, int requestType, const char* request);
qboolean	trap_G_COOL_API_DB_AddRequestTyped(byte* reference, int referenceLength, int requestType, const char* request, DBRequestType_t dbRequestType);
qboolean	trap_G_COOL_API_DB_NextResponse(int* requestType, int* affectedRows, int* status, char* errorMessage, int errorMessageSize, byte* reference, int referenceLength);
qboolean	trap_G_COOL_API_DB_GetReference(byte* reference, int referenceLength);
qboolean	trap_G_COOL_API_DB_NextRow();
int			trap_G_COOL_API_DB_GetInt(int place);
void		trap_G_COOL_API_DB_GetFloat(int place, float* value);
qboolean	trap_G_COOL_API_DB_GetString(int place, char* out, int outSize);

qboolean	trap_G_COOL_API_DB_AddPreparedStatement(byte* reference, int referenceLength, int requestType, const char* request);
qboolean	trap_G_COOL_API_DB_PreparedBindString(const char* string);
qboolean	trap_G_COOL_API_DB_PreparedBindFloat(float number);
qboolean	trap_G_COOL_API_DB_PreparedBindInt(int number);
qboolean	trap_G_COOL_API_DB_PreparedBindBinary(byte* data, int dataLength);
qboolean	trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
int			trap_G_COOL_API_DB_GetBinary(int place, byte* out, int outSize);
qboolean	trap_G_COOL_API_DB_PreparedBindNull();
qboolean	trap_G_COOL_API_DB_GetMoreResults(int* affectedRows);

qboolean	G_COOL_API_DB_EscapeString(char* input, int size) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_EscapeString(input, size);
}
qboolean	G_COOL_API_DB_AddRequest(byte* reference, int referenceLength, int requestType, const char* request) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_AddRequest( reference, referenceLength, requestType, request);
}
qboolean	G_COOL_API_DB_AddRequestTyped(byte* reference, int referenceLength, int requestType, const char* request, DBRequestType_t dbRequestType){
	if (coolApi_dbVersion < 2) return qfalse;
	return trap_G_COOL_API_DB_AddRequestTyped( reference, referenceLength, requestType, request, (int)dbRequestType);
}
qboolean	G_COOL_API_DB_NextResponse(int* requestType, int* affectedRows, int* status, char* errorMessage, int errorMessageSize, byte* reference, int referenceLength) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_NextResponse( requestType, affectedRows, status, errorMessage, errorMessageSize, reference, referenceLength);
}
qboolean	G_COOL_API_DB_GetReference(byte* reference, int referenceLength) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_GetReference(reference, referenceLength);
}
qboolean	G_COOL_API_DB_NextRow() {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_NextRow();
}
int			G_COOL_API_DB_GetInt(int place) {
	if (!coolApi_dbVersion) return 0;
	return trap_G_COOL_API_DB_GetInt( place);
}
void		G_COOL_API_DB_GetFloat(int place, float* value) {
	if (!coolApi_dbVersion) {
		*value = 0;
		return;
	}
	trap_G_COOL_API_DB_GetFloat( place, value);
}
qboolean	G_COOL_API_DB_GetString(int place, char* out, int outSize) {
	if (!coolApi_dbVersion) return qfalse;
	return trap_G_COOL_API_DB_GetString( place, out, outSize);
}

// dbApi v3

qboolean	G_COOL_API_DB_AddPreparedStatement(byte* reference, int referenceLength, int requestType, const char* request) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_AddPreparedStatement( reference, referenceLength, requestType, request);
}
qboolean	G_COOL_API_DB_PreparedBindString(const char* string) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_PreparedBindString( string);
}
qboolean	G_COOL_API_DB_PreparedBindFloat(float number) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_PreparedBindFloat( number);
}
qboolean	G_COOL_API_DB_PreparedBindInt(int number) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_PreparedBindInt( number);
}
qboolean	G_COOL_API_DB_PreparedBindBinary(byte* data, int dataLength) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_PreparedBindBinary( data, dataLength);
}
qboolean	G_COOL_API_DB_FinishAndSendPreparedStatement() {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
}
int			G_COOL_API_DB_GetBinary(int place, byte* out, int outSize) {
	if (coolApi_dbVersion < 3) return 0;
	return trap_G_COOL_API_DB_GetBinary( place, out, outSize);
}
qboolean	G_COOL_API_DB_PreparedBindNull() {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_PreparedBindNull();
}
qboolean	G_COOL_API_DB_GetMoreResults(int* affectedRows) {
	if (coolApi_dbVersion < 3) return qfalse;
	return trap_G_COOL_API_DB_GetMoreResults( affectedRows);
}
