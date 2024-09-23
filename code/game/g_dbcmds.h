
#ifndef G_DBCMDS_H
#define G_DBCMDS_H

typedef enum DBRequestTypes_s {
	DBREQUEST_REGISTER,
	DBREQUEST_LOGIN,
} DBRequestTypes_t;

void G_DB_CheckResponses();


#endif
