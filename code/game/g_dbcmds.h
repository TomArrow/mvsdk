
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

#include "../game/q_shared.h"

#define USERNAME_MAX_LEN 10
#define PASSWORD_MAX_LEN 50 // bcrypt has a limit which seems to be 72 but some sources say its only 50 and im too lazy to read the bcrypt code to decide whos right. 50 is enough anyway.

typedef enum DBRequestTypes_s {
	DBREQUEST_REGISTER,
	DBREQUEST_LOGIN, // actual log in
	DBREQUEST_LOGIN_UPDATELASTLOGIN,
	DBREQUEST_BCRYPTPW, // pw bcrypt request (no actual db request)
	DBREQUEST_CREATETABLE,
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

typedef struct referenceSimpleString_s {
	char	s[MAX_STRING_CHARS];
}referenceSimpleString_t;

void G_DB_CheckResponses();
qboolean G_DB_VerifyUsername(const char* username, int clientNumNotify);
qboolean G_DB_VerifyPassword(const char* password, int clientNumNotify);

#endif
