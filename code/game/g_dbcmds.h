
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

#include "../game/q_shared.h"
#include "../game/g_defrag.h"
#include "../game/bg_defrag_global.h"


typedef enum DBTable_s {
	DBT_USERS,
	DBT_MESSAGES,
	DBT_RUNS,
	DBT_CHECKPOINTS,
	DBT_SUBCONTESTS,
	DBT_MAPRACEDEFAULTS,
	DBT_META,
	DBT_MAPRATINGS,
	DBT_MAPTAGS,
	DBT_MAPMINIMAPS,
	DBT_MAPMETA,
	DBT_COUNT_TABLES
} DBTable_t;
typedef void(QDECL* dbTableCreateFunc_t)();
extern dbTableCreateFunc_t tableCreateFuncs[DBT_COUNT_TABLES];

typedef enum DBRequestTypes_s {
	DBREQUEST_REGISTER,
	DBREQUEST_LOGIN, // actual log in
	DBREQUEST_FORCEDLOGIN, // actual log in
	DBREQUEST_LOGIN_UPDATELASTLOGIN,
	DBREQUEST_BCRYPTPW, // pw bcrypt request (no actual db request)
	DBREQUEST_CREATETABLE,
	DBREQUEST_UPDATECOLUMNS,
	DBREQUEST_INSERTORUPDATERUN,
	DBREQUEST_TOP,
	DBREQUEST_TOPMAPSEARCH,
	DBREQUEST_TIME,
	DBREQUEST_SAVECHECKPOINTS,
	DBREQUEST_LOADCHECKPOINTS,
	DBREQUEST_INSERTORUPDATEMAPRACEDEFAULTS,
	DBREQUEST_LOADMAPRACEDEFAULTS,
	DBREQUEST_INSERTORUPDATESUBCONTEST,
	DBREQUEST_SUBCONTESTLEADERBOARD,
	DBREQUEST_CHANGEPASSWORD,
	DBREQUEST_ARENAGENMAPLIST,
	DBREQUEST_GETLATESTRUNS,
	DBREQUEST_MAPLISTUNPLAYED,
	DBREQUEST_MAPSEARCH,
	DBREQUEST_RANKUPDATE,
	DBREQUEST_RANKUPDATEMAPREQUEST,
	DBREQUEST_RANKUPDATEMAPLATESTSET,
	DBREQUEST_RATEMAP,
	DBREQUEST_RATEMAPSHOWMINE,
	DBREQUEST_RANK,
	DBREQUEST_DEMOCHECK_GETALLRUNS,
	DBREQUEST_SENDUSERMESSAGE,
	DBREQUEST_SENDUSERMESSAGE_ACTUAL,
	DBREQUEST_CHECKUNREADUSERMESSAGES,
	DBREQUEST_LISTUSERMESSAGES,
	DBREQUEST_LISTUSERMESSAGES_UPDATEREAD,
	DBREQUEST_PRUNEUSERMESSAGES,
	DBREQUEST_GENERIC,
	DBREQUEST_INSERTMINIMAP,
	DBREQUEST_TOUCHMAPMETA,
} DBRequestTypes_t;

typedef struct loginRegisterStruct_s {
	DBRequestTypes_t	followUpType;
	int			userId;
	int			ip[4];
	int			clientnum;
	char		username[MAX_STRING_CHARS];
	char		password[MAX_STRING_CHARS];
	char		dbPassword[MAX_STRING_CHARS];
	qboolean	needDoubleBcrypt;
	int			userFlags;
	qboolean	isAdminForcedLogin;
	int			ipAdmin[4];
	int			clientnumAdmin;
} loginRegisterStruct_t;


typedef enum topRequestType_s {
	TOPREQUEST_ALL,
	TOPREQUEST_SPECIFICLB,
} topRequestType_t;


typedef struct topScoresRequestStruct_s {
	int			ip[4];
	int			clientnum;
	topRequestType_t		type;
	mainLeaderboardType_t	lbTypeIfSpecific;
	raceStyle_t	mapDefaultRaceStyle;
	int			page;
	char		course[COURSENAME_MAX_LEN + 1];
	char		subcourse[COURSENAME_MAX_LEN + 1];
	int			style;
}topScoresRequestStruct_t;
typedef struct rankUpdateRequestStruct_s {
	int			ip[4];
	int			clientnum;
	char		course[COURSENAME_MAX_LEN + 1];
	char		subcourse[COURSENAME_MAX_LEN + 1];
	int			style;
	qboolean	flush;
}rankUpdateRequestStruct_t;
typedef struct rankUpdateMapRequestStruct_s {
	int			ip[4];
	int			clientnum;
	int			mapCountLimit;
	qboolean	all;
}rankUpdateMapRequestStruct_t;
typedef struct timeRequestStruct_s {
	int			ip[4];
	int			clientnum;
	raceStyle_t	raceStyle;
	int			lbType;
	char		course[COURSENAME_MAX_LEN + 1];
	char		subcourse[COURSENAME_MAX_LEN + 1];
	int			style;
	qboolean	forUserInfo;
}timeRequestStruct_t;
typedef struct checkPointSaveRequestStruct_s {
	int			ip[4];
	int			clientnum;
}checkPointSaveRequestStruct_t;
typedef struct rateMapStruct_s {
	int			ip[4];
	int			clientnum;
	float		value;
	int			style;
}rateMapStruct_t;

typedef struct userMessageSendStruct_s {
	int			ip[4];
	int			clientnum;
	char		message[MAX_STRING_CHARS];
	int			senderId;
	// gotta query these first:
	char		userName[USERNAME_MAX_LEN + 1];
	int			recipientId;
}userMessageSendStruct_t;

typedef struct userMessagesListStruct_s {
	int			ip[4];
	int			clientnum;
	int			page;
	int			userid;
}userMessagesListStruct_t;


typedef struct userMessagesPruneStruct_s {
	int			ip[4];
	int			clientnum;
}userMessagesPruneStruct_t;

typedef struct latestRunsRequestStruct_s {
	int			ip[4];
	int			clientnum;
	int			userId;
	qboolean	styleSpecified;
	qboolean	pageSpecified;
	char		userSearchTerm[15];
	qboolean	userResults;
}latestRunsRequestStruct_t;



typedef enum mapSearchType_s {
	MAPSEARCH_LONGEST,
	MAPSEARCH_SHORTEST,
	MAPSEARCH_NOTWR,
	MAPSEARCH_WR,
	MAPSEARCH_MOSTPLAYED,
	MAPSEARCH_TOPRATED,
	MAPSEARCH_HARDEST,
	MAPSEARCH_EASIEST,

} mapSearchType_t;

typedef struct mapSearchRequestStruct_s {
	int						ip[4];
	int						clientnum;
	qboolean				styleSpecified;
	qboolean				lbTypeSpecified;
	qboolean				pageSpecified;
	mapSearchType_t			type;
	int						style;
	mainLeaderboardType_t	lbType;
	char					userSearchTerm[15];
} mapSearchRequestStruct_t;
typedef struct rankRequestStruct_s {
	int						ip[4];
	int						clientnum;
	qboolean				styleSpecified;
	qboolean				lbTypeSpecified;
	qboolean				pageSpecified;
	int						style;
	mainLeaderboardType_t	lbType;
} rankRequestStruct_t;


typedef struct maplistUnplayedRequestStruct_s {
	int			ip[4];
	int			clientnum;
}maplistUnplayedRequestStruct_t;
typedef struct demoCheckScriptRequest_s {
	int			ip[4];
	int			clientnum;
	char		outScriptName[100];
}demoCheckScriptRequest_t;

typedef struct insertUpdateRunStruct_s {
	int					ip[4];
	int					clientnum;

	finishedRunInfo_t	runInfo;
	//struct {
	//	int			runId;
	//	int			milliseconds;
	//	float		topspeed;
	//	float		average;
	//	float		distance;
	//	int			warningFlags;
	//	int			levelTimeFinish;
	//} runInfo;

	//raceStyle_t	raceStyle;
} insertUpdateRunStruct_t;

typedef struct insertUpdateSubContestStruct_s {
	int					ip[4];
	int					clientnum;
	float				value;
	int					userid;
	subContests_t		contest;

	// meta
	int					msec;
	int					movementStyle;
	float				extraValue1, extraValue2;
	int					extraValue3, extraValue4;
} insertUpdateSubContestStruct_t;

typedef struct subContestLeaderboardRequestStruct_s {
	int					ip[4];
	int					clientnum;
	subContests_t		contest;
	int					page;
} subContestLeaderboardRequestStruct_t;

typedef struct insertUpdateMapRaceDefaultsStruct_s {
	int					ip[4];
	int					clientnum;
	char				what[10]; // lazy lol
	char				course[COURSENAME_MAX_LEN + 1]; // lazy lol
} insertUpdateMapRaceDefaultsStruct_t;

typedef struct referenceSimpleString_s {
	char	s[MAX_STRING_CHARS];
}referenceSimpleString_t;

typedef struct topRequestStruct_s {
	int						ip[4];
	int						clientnum;
	topRequestType_t		type;
	mainLeaderboardType_t	lbTypeIfSpecific;
	int						page;
	movementStyle_e			style;
} topRequestStruct_t;

void G_DB_CheckResponses();
qboolean G_DB_VerifyUsername(const char* username, int clientNumNotify); 
void G_DB_Init();
void G_CreateUserTable();
void G_CreateMessagesTable();

// bind username and userid
#define USERIDQUERY_USERID "SET @username=?, @userid=?;SELECT @username,@userid;"

// bind username search term
#define USERIDQUERY_USERSEARCH "SET @search = ?; \
(SELECT @username := username, @userid := id, \
instr(username, @search) + instr(REVERSE(username), REVERSE(@search)) - 2 AS diff \
FROM users \
WHERE instr(username, @search) \
ORDER BY diff ASC \
LIMIT 1);"



// 
// generic requests concretes
//
#define MAPTAG_MAX_LEN	40

// 
// generic requests boilerplate
//
struct gentity_s;
struct genericDbRequestStruct_s;
#define GENERICDBREQUEST_MAX_PARAMS 64
typedef enum genericDbRequestFlags_s {
	GDBRF_RETURNEVENIFENTINVALID = (1<<0), // call the callback even if the ent/user is no longer valid
	GDBRF_NOENT = (1<<1), // request has no caller
}genericDbRequestFlags_t;
#define GENERIC_DB_REQUESTTYPES(a) \
	a(GDBREQUEST_TEST)\
	a(GDBREQUEST_MAPRATINGSFETCH)\
	a(GDBREQUEST_TAG)\
	a(GDBREQUEST_VOTE_MAPSEARCH)\
	a(GDBREQUEST_MAPMETA)\
	a(GDBREQUEST_MAPMINIMAP)

typedef enum genericDbRequestType_s {
#define GDBREQUEST_ENUM(a) a,
	GENERIC_DB_REQUESTTYPES(GDBREQUEST_ENUM)
#undef GDBREQUEST_ENUM
	GDBREQUEST_COUNT_TYPES
} genericDbRequestType_t;
typedef qboolean (QDECL* genericDBRequestCallback_t)(struct gentity_s* ent,struct genericDbRequestStruct_s* data);



typedef enum mapSearchFlags_s {
	MAPSEARCHFLAGS_TAG = (1 << 0), // search for maps that have a positive rank on this tag
	MAPSEARCHFLAGS_NOTAG = (1 << 1), // search for maps that didn't have a particular tag set at all, aka uncategorized
	MAPSEARCHFLAGS_BADPARAM = (1 << 2), // error parsing params
}mapSearchFlags_t;

typedef struct genericDbRequestStruct_s {
	int								ip[4];
	int								clientnum;
	int								page;
	int								userid;
	int								requiredTables; // bitmask
	char							ident[10]; // some short identifier, for debug messages
	genericDbRequestFlags_t			flags;
	genericDbRequestType_t			callbackType;
	struct {
		int status;
		int affectedRows; // for INSERT INTO ON DUPLICATE KEY UPDATE: 0 = no change. 1 = new entry. 2 = updated row
		const char* errorMessage;
	} resultInfo;
	union {
		struct {
			int	requestType;
			char course[COURSENAME_MAX_LEN + 1];
		} minimap;
		struct {
			int	requestType;
			char course[COURSENAME_MAX_LEN + 1];
		} mapmeta;
		struct {
			int	requestType;
			char tag[MAPTAG_MAX_LEN + 1];
			qboolean defrag;
			int value;
		} maptag;
		struct {
			int requestType;
			char fullsearchline[MAPTAG_MAX_LEN + 20 + 1];
			char tag[MAPTAG_MAX_LEN + 1];
			qboolean defrag;
			mapSearchFlags_t searchFlags;
		} callvoteMapsearch;
	} specifics;
} genericDbRequestStruct_t;
typedef enum dbRequestParamType_s {
	PARAM_INT,
	PARAM_FLOAT,
	PARAM_STRING,
} dbRequestParamType_t;
typedef struct genericDbRequestCallbackInfo_s {
	genericDBRequestCallback_t		callback;
} genericDbRequestCallbackInfo_t;
extern genericDBRequestCallback_t GDBREQUEST_TESTBlah;
extern genericDbRequestCallbackInfo_t* genericDBRequestCallbacks[GDBREQUEST_COUNT_TYPES];
#define REGISTER_DBREQUEST_CALLBACK(type,callbackA) genericDbRequestCallbackInfo_t type##_CallbackInfo = {(callbackA)}

int G_DB_GetPageArg(int pageArg);
genericDbRequestStruct_t G_DB_GenericRequest_Prepare(struct gentity_s* ent, genericDbRequestType_t type, int tables, const char* ident, int pageArg);
// convenience wrapper around prepared statement generation. the printf format string is not evaluated into a normal printf string, but rather all placeholders are replaced with ? and the parameter types are applied for the prepared statement and the parameters are bound correctly.
// DO NOT WRAP THE QUERY IN va() OR SIMILAR BEFORE. THAT WILL CREATE A MAJOR SQL INJECTION SECURITY RISK. YOU MUST PASS THE FORMAT STRING DIRECTLY TO THIS FUNCTION, IT WILL CONVERT THE QUERY INTO A PREPARED STATEMENT WITH BOUND PARAMETERS
qboolean QDECL G_DB_GenericRequest_Send(genericDbRequestStruct_t data, PRINTF_FORMAT_STRING char* fmt, ...) __attribute__((format(printf, 2, 3)));

//
// g_messages.c
//

void G_SendUserMessageContinue(int status, const char* errorMessage, int affectedRows);
void G_SendUserMessageFinished(int status, const char* errorMessage, int affectedRows);
void G_ListUserMessagesListContinue(int status, const char* errorMessage, int affectedRows);
void G_CheckUnreadUserMessagesResults(int status, const char* errorMessage, int affectedRows);

#endif
