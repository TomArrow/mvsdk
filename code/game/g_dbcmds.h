
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

#include "../game/q_shared.h"
#include "../game/g_defrag.h"
#include "../game/bg_defrag_global.h"

typedef enum DBRequestTypes_s {
	DBREQUEST_REGISTER,
	DBREQUEST_LOGIN, // actual log in
	DBREQUEST_LOGIN_UPDATELASTLOGIN,
	DBREQUEST_BCRYPTPW, // pw bcrypt request (no actual db request)
	DBREQUEST_CREATETABLE,
	DBREQUEST_UPDATECOLUMNS,
	DBREQUEST_INSERTORUPDATERUN
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
} loginRegisterStruct_t;

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

typedef struct referenceSimpleString_s {
	char	s[MAX_STRING_CHARS];
}referenceSimpleString_t;

void G_DB_CheckResponses();
qboolean G_DB_VerifyUsername(const char* username, int clientNumNotify); 
void G_DB_Init();

#endif
