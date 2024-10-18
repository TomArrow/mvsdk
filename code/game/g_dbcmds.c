
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
static void G_CreateMapRaceDefaultsTable();
extern const char* DF_GetCourseName();

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
void PrintRaceTime(finishedRunInfo_t* runInfo, qboolean preliminary, qboolean showRank, gentity_t* ent);

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


	if (coolApi_dbVersion >= 3 && trap_G_COOL_API_DB_GetMoreResults(NULL) && trap_G_COOL_API_DB_NextRow())
	{
		if (!trap_G_COOL_API_DB_GetInt(0)) {// SQL result returns amount of faster runs BY OURSELVES on this LB
			runData.runInfo.pbStatus |= PB_LB;
		}
	}

	if (coolApi_dbVersion >= 3 && trap_G_COOL_API_DB_GetMoreResults(NULL) && trap_G_COOL_API_DB_NextRow())
	{
		runData.runInfo.rankLB = trap_G_COOL_API_DB_GetInt(0) + 1; // SQL result returns amount of faster runs so we add 1 (0 faster runs = #1)
	}

	// SELECT (UNIX_TIMESTAMP(@now)-3000000000) as unixTimeMinus3bill
	// subtracting 3 billion cuz no 64 bit support in vm
	if (coolApi_dbVersion >= 3 && trap_G_COOL_API_DB_GetMoreResults(NULL) && trap_G_COOL_API_DB_NextRow())
	{
		runData.runInfo.unixTimeStampShifted = trap_G_COOL_API_DB_GetInt(0);
	}

	PrintRaceTime(&runData.runInfo, qfalse, qtrue,ent);

}
static void G_InsertMapDefaultsResult(int status, const char* errorMessage, int affectedRows) {
	insertUpdateMapRaceDefaultsStruct_t data;
	gentity_t* ent = NULL;
	//evaluatedRunInfo_t eRunInfo;

	trap_G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

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

	trap_G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

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

	if (!trap_G_COOL_API_DB_NextRow()) {
		trap_SendServerCommand(-1, "print \"^1Map defaults load failed; no defaults found.\n\"");
		level.mapDefaultsLoadFailed = qfalse; // we dont have a defdault so its ok
		level.mapDefaultsConfirmed = qtrue;
		return;
	}
	else {
		raceStyle_t rs;
		rs.movementStyle = MV_JK2;
		rs.msec = trap_G_COOL_API_DB_GetInt(0);
		rs.jumpLevel = trap_G_COOL_API_DB_GetInt(1);
		rs.variant = trap_G_COOL_API_DB_GetInt(2);
		rs.runFlags = trap_G_COOL_API_DB_GetInt(3);
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

	trap_G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d checkpoints saved, user no longer valid.\n", data.clientnum);
		//return;
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
		if (!trap_G_COOL_API_DB_GetMoreResults(&inserted))
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

	trap_G_COOL_API_DB_GetReference((byte*)&data, sizeof(data));

	if (!(ent = DB_VerifyClient(data.clientnum, data.ip))) {
		Com_Printf("^1Client %d checkpoints loaded, user no longer valid.\n", data.clientnum);
		//return;
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

	while (trap_G_COOL_API_DB_NextRow()) {
		trap_G_COOL_API_DB_GetFloat(0,&trEndpos[0]);
		trap_G_COOL_API_DB_GetFloat(1,&trEndpos[1]);
		trap_G_COOL_API_DB_GetFloat(2,&trEndpos[2]);
		trap_G_COOL_API_DB_GetFloat(3,&yaw);
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

typedef struct topLeaderBoardEntry_s {
	qboolean exists;
	char username[USERNAME_MAX_LEN + 1];
	int besttime, userid, runFlags, msec, jump, runFlagsDiff;
	//raceStyle_t raceStyle;
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
	int rank = 1;
	int maxrank = 0;
	int i;
	static topLeaderBoardEntry_t entries[11][LB_TYPES_COUNT];
	//evaluatedRunInfo_t eRunInfo;

	trap_G_COOL_API_DB_GetReference((byte*)&lbRequestData, sizeof(lbRequestData));

	if (!(ent = DB_VerifyClient(lbRequestData.clientnum, lbRequestData.ip))) {
		Com_Printf("^1Client %d run inserted, user no longer valid.\n", lbRequestData.clientnum);
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

	while (trap_G_COOL_API_DB_NextRow()) {
		int type,userid,rankHere;
		topLeaderBoardEntry_t* entry;
		type = trap_G_COOL_API_DB_GetInt(0);
		userid = trap_G_COOL_API_DB_GetInt(3);

		if (type != currentType) {
			currentType = type;
			rank = 1;
			//trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^2Leaderboard type %d.\n\"", currentType));
		}
		if (rank > 9) continue;
		rankHere = userid == -1 ? 10 : rank - 1;
		entry = &entries[rankHere][type]; // unofficial go at the end.
		entry->exists = qtrue;
		if (userid == -1) {
			Q_strncpyz(entry->username, "!unlogged!", sizeof(entry->username));
		}
		else {
			trap_G_COOL_API_DB_GetString(1, entry->username, sizeof(entry->username));
		}
		entry->besttime = trap_G_COOL_API_DB_GetInt(2);
		entry->runFlags = trap_G_COOL_API_DB_GetInt(4);
		entry->runFlagsDiff = (entry->runFlags ^ level.mapDefaultRaceStyle.runFlags) & entry->runFlags; // show all that are active that are different from default
		entry->msec = trap_G_COOL_API_DB_GetInt(5);
		entry->jump = trap_G_COOL_API_DB_GetInt(6);
		if (userid != -1) {
			//trap_SendServerCommand(lbRequestData.clientnum, va("print \"^1#%d %-10s %10s.\n\"", rank, userid == -1 ? "!unlogged!": username, DF_MsToString(besttime)));
			maxrank = MAX(maxrank, rank);
			rank++;
		}
	}

#define MSECSTRING(msec) ((msec) == -1 ? "togl" : ((msec) == -2 ? "flt" : ((msec) == 0 ? "unkn" : multiva("%d", 1000 / (msec)))))
#define LBROW(lbType,coloration,jumpvalue) !entriesHere[lbType].exists ? ' ' :'#', !entriesHere[lbType].exists ? "  " : topNumberStrings[i], coloration(entriesHere[lbType]), entriesHere[lbType].exists ? entriesHere[lbType].username : "", entriesHere[lbType].exists ? MSECSTRING(entriesHere[lbType].msec) : "" jumpvalue(entriesHere[lbType],lbType), !entriesHere[lbType].exists ? "" : DF_MsToString(entriesHere[lbType].besttime)

#define JUMPVALUE(a,b) ,entriesHere[b].exists ? 'j':' ' ,((a).jump)
#define JUMPVALUE_EMPTY(a,b) 
#define TIMECOLOR_DEFAULT(a) '7'
#define TIMECOLOR_CHEAT(a) ((((a).runFlags & RFL_TAS)||((a).runFlags & RFL_BOT)) ? (((a).runFlags & RFL_SEGMENTED) ? 'j':'1') : '7' )
#define TIMECOLOR_CUSTOM(a) (((a).runFlagsDiff & RFL_CLIMBTECH) ? 'E':'7')
	trap_SendServerCommand(lbRequestData.clientnum, va("print \"^2    %-27s^h|     ^2%-27s^h|     ^2%-31s^h|     ^2%-27s^h|     ^2%-29s\n\"", "MAIN","NOJUMPBUG","CUSTOM","SEGMENTED", "CHEAT"));
	for (i = 0; i < 11; i++) {
		topLeaderBoardEntry_t* entriesHere = entries[i];
		if (i >= maxrank && i < 10) continue;
		trap_SendServerCommand(lbRequestData.clientnum, va("print \"%s^7"
			"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
			"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
			"^J%c%02s^%c %-10s ^c%4s %c%-2d ^u%10s ^h| " // so middle (custom) column is 4 wider
			"^J%c%02s^%c %-10s ^c%4s ^u%10s ^h| "
			"^J%c%02s^%c %-10s ^c%4s ^u%10s "
			"\n\"",
			i==10 ? multiva("%31s^h|%32s^h|%36s^h|%32s^h|%32s\n","","","","","") : "",
			LBROW(LB_MAIN, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
			,LBROW(LB_NOJUMPBUG, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
			,LBROW(LB_CUSTOM, TIMECOLOR_CUSTOM, JUMPVALUE)
			,LBROW(LB_SEGMENTED, TIMECOLOR_DEFAULT, JUMPVALUE_EMPTY)
			,LBROW(LB_CHEAT, TIMECOLOR_CHEAT, JUMPVALUE_EMPTY)
			));
	}
	
	//trap_SendServerCommand(lbRequestData.clientnum, va("print \"\n^7color explanation:\n^7    %-27s      ^7%-27s      ^7%-27s      ^7%-27s^      ^7%-29s\n\"", "MAIN", "NOJUMPBUG", "CUSTOM", "SEGMENTED", "CHEAT"));
	trap_SendServerCommand(lbRequestData.clientnum,va( "print \"\n^7username color explanation:  ^E%-12s ^1%-12s ^j%-12s\n^7for more details, request specific leaderboard\n\"","climbtech", "strafebot/TAS", "strafebot/TAS+segmented"));

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
	//trap_SendServerCommand(-1, va("print \"^2%s ^7logged in as '%s'.\n\"",client ? client->pers.netname : "", loginData->username));

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
static void G_UpdateColumnsResult(int status, const char* errorMessage) {
	static referenceSimpleString_t tableName;
	trap_G_COOL_API_DB_GetReference((byte*)&tableName, sizeof(tableName));
	if (status) {
		Com_Printf("updating columns for table %s failed with status %d and error message %s.\n", tableName.s, status, errorMessage);
		return;
	}
	Com_Printf("updating columns for table %s was successful.\n", tableName.s);

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
				case DBREQUEST_UPDATECOLUMNS:
					G_UpdateColumnsResult(status, errorMessage);
					break;
				case DBREQUEST_LOGIN:
					G_LoginFetchDataResult(status, errorMessage);
					break;
				case DBREQUEST_INSERTORUPDATERUN:
					G_InsertRunResult(status, errorMessage, affectedRows);
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
				case DBREQUEST_SAVECHECKPOINTS:
					G_SaveCheckpointsResult(status, errorMessage, affectedRows);
					break;
				case DBREQUEST_LOADCHECKPOINTS:
					G_LoadCheckpointsResult(status, errorMessage, affectedRows);
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
	memcpy(data.ip, mv_clientSessions->clientIP, sizeof(data.ip));

	if (!trap_G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data),DBREQUEST_SAVECHECKPOINTS,request)) {
		G_SendServerCommand(playerent - g_entities, "print \"DB connection not available to save checkpoints.\n\"",qtrue);
		return;
	}
	coursename = DF_GetCourseName();

	// DELETE
	trap_G_COOL_API_DB_PreparedBindString(coursename);
	trap_G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);

	// INSERT
	for (i = 0; i < playerent->client->pers.df_checkpointData.count; i++) {
		gentity_t* check = g_entities + playerent->client->pers.df_checkpointData.checkpointNumbers[i];
		trap_G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);
		trap_G_COOL_API_DB_PreparedBindString(coursename);
		trap_G_COOL_API_DB_PreparedBindInt(i);
		trap_G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[0]);
		trap_G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[1]);
		trap_G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.trEndpos[2]);
		trap_G_COOL_API_DB_PreparedBindFloat(check->checkpointSeed.anglesYaw);
	}

	trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
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
	memcpy(data.ip, mv_clientSessions->clientIP, sizeof(data.ip));

	if (!trap_G_COOL_API_DB_AddPreparedStatement((byte*)&data,sizeof(data), DBREQUEST_LOADCHECKPOINTS, "SELECT x,y,z,yaw FROM checkpoints WHERE course=? AND userid=? ORDER BY number ASC")) {
		G_SendServerCommand(playerent - g_entities, "print \"DB connection not available to load checkpoints.\n\"",qtrue);
		return;
	}

	coursename = DF_GetCourseName();

	trap_G_COOL_API_DB_PreparedBindString(coursename);
	trap_G_COOL_API_DB_PreparedBindInt(playerent->client->sess.login.id);

	trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
}

static void G_CreateUserTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = va("CREATE TABLE IF NOT EXISTS users(id BIGINT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(%d) UNIQUE NOT NULL, password VARCHAR(64)  NOT NULL, lastlogin DATETIME, created DATETIME NOT NULL, lastip  INT UNSIGNED, flags  INT UNSIGNED NOT NULL DEFAULT 0)",USERNAME_MAX_LEN);
	Q_strncpyz(tableName.s, "users", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}

static void G_CreateCheckpointsTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS checkpoints(id BIGINT AUTO_INCREMENT PRIMARY KEY, userid BIGINT SIGNED NOT NULL, course VARCHAR(100) NOT NULL, number TINYINT(2) SIGNED NOT NULL, x DOUBLE NOT NULL, y DOUBLE NOT NULL, z DOUBLE NOT NULL, yaw DOUBLE NOT NULL, UNIQUE KEY checkpoint_unique (userid,course,number), INDEX i_user_map (userid,course), INDEX i_number(number))";
	Q_strncpyz(tableName.s, "checkpoints", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
}
static void G_CreateMapRaceDefaultsTable() {
	referenceSimpleString_t tableName;
	const char* userTableRequest = "CREATE TABLE IF NOT EXISTS mapdefaults(\
			course VARCHAR(100) NOT NULL PRIMARY KEY, \
			msec SMALLINT NOT NULL, \
			jump TINYINT NOT NULL, \
			variant SMALLINT NOT NULL,\
			runFlags INT NOT NULL)";
	Q_strncpyz(tableName.s, "mapdefaults", sizeof(tableName.s));
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
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
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
	trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_UPDATECOLUMNS, columnsUpdateRequest);
}

static void G_DB_CreateTables() {
	G_CreateUserTable();
	G_CreateRunsTable();
	G_CreateCheckpointsTable();
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



#define SUBFUNC(a,b) `b ## a`
#define RUNFLAGSFUNC(a,b,c,d,e,f) QUOTEME(SUBFUNC(a,d)) "," // gotta do this cuz qvm gets confused by the comma otherwise
#define RUNFLAGSFUNC2(a,b,c,d,e,f) "?,"
#define RUNFLAGSFUNC3(a,b,c,d,e,f) `d ## a`=? AND
	
	lbSQLCondition = getLeaderboardSQLConditions(runInfo->lbType, &level.mapDefaultRaceStyle);
	insertOrUpdateRequest =
		va("SET @now=NOW();"
			"INSERT INTO runs (userid,course,duration_ms,topspeed,average,distance,style,msec,jump,variant,runFlags,"
			RUNFLAGS(RUNFLAGSFUNC)
			"runwhen,runfirst,warningFlags, distanceXY,startLessTime,endLessTime,saveposCount,resposCount,lostMsecCount,lostCmdsCount)"
			"VALUES (?,?,?,?,?,?,?,?,?,?,?,"
			RUNFLAGS(RUNFLAGSFUNC2)
			"@now,@now,?,?,?,?,?,?,?,?)"
			"ON DUPLICATE KEY UPDATE "
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
			"lostCmdsCount = IF(?<duration_ms,?,lostCmdsCount),"
			"duration_ms = IF(?<duration_ms,?,duration_ms);" // duration_ms has to be set last or else all other columns arent updated
			// check if we had a better time on this leaderboard before. (return value of INSERT OR UPDATE only tells us if it was the best with the unique key, but leaderboards accumulate ranges of race settings, especially "custom" leaderboard and such)
			"SELECT COUNT(id) AS countOwnFaster FROM runs WHERE userid=? AND course=? AND style=? AND variant=? AND %s AND (duration_ms<? OR (duration_ms=? AND runwhen<@now));"
			// check our new rank.
			"SELECT COUNT(DISTINCT userid) AS countFaster FROM runs WHERE userid !=? AND userid!=-1 AND course=? AND style=? AND variant=? AND %s AND (duration_ms<? OR (duration_ms=? AND runwhen<@now));" // if someone got the same time as you, but earlier, hes in front of u
			"SELECT (UNIX_TIMESTAMP(@now)-(?*1000000000)) as unixTimeMinus3bill", lbSQLCondition, lbSQLCondition);
	
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
#define RUNFLAGSFUNC(a,b,c,d,e,f) trap_G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
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

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	// SECOND QUERY - SELECT OUR BEST TIME
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	trap_G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

	//#define RUNFLAGSFUNC(a,b,c) trap_G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
		//RUNFLAGS(RUNFLAGSFUNC)
		//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
	//#undef RUNFLAGSFUNC

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	// THIRD QUERY - SELECT RANK
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->userId);
	trap_G_COOL_API_DB_PreparedBindString(runInfo->coursename);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.movementStyle);
	//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.msec);
	//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.jumpLevel);
	trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.variant);

//#define RUNFLAGSFUNC(a,b,c) trap_G_COOL_API_DB_PreparedBindInt((int)!!((int)runInfo->raceStyle.runFlags & RFL_ ## b));
	//RUNFLAGS(RUNFLAGSFUNC)
	//trap_G_COOL_API_DB_PreparedBindInt((int)runInfo->raceStyle.runFlags);
//#undef RUNFLAGSFUNC

	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);
	trap_G_COOL_API_DB_PreparedBindInt(runInfo->milliseconds);

	//if (coolApi_dbVersion >= 3) {
		trap_G_COOL_API_DB_PreparedBindInt(runInfo->unixTimeStampShiftedBillionCount);
	//}

	trap_G_COOL_API_DB_FinishAndSendPreparedStatement();
	//Q_strncpyz(tableName.s, "runs", sizeof(tableName.s));
	//trap_G_COOL_API_DB_AddRequest((byte*)&tableName,sizeof(referenceSimpleString_t), DBREQUEST_CREATETABLE, userTableRequest);
	return qtrue;
}

