
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

#include "../game/q_shared.h"

typedef enum DBRequestTypes_s {
	DBREQUEST_REGISTER,
	DBREQUEST_LOGIN, // actual log in
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


#endif
