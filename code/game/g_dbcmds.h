
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

#include "../game/q_shared.h"
#include "../game/g_defrag.h"
#include "../game/bg_defrag_global.h"

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

typedef struct latestRunsRequestStruct_s {
	int			ip[4];
	int			clientnum;
	int			userId;
	qboolean	styleSpecified;
	qboolean	pageSpecified;
	char		userSearchTerm[15];
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


typedef struct maplistUnplayedRequestStruct_s {
	int			ip[4];
	int			clientnum;
}maplistUnplayedRequestStruct_t;

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

#endif
