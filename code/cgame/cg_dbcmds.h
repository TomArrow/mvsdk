
#ifndef CG_DBCMDS_H
#define CG_DBCMDS_H

#include "../game/q_shared.h"

typedef enum DBRequestTypes_s {
	DBREQUEST_CHATSAVE,
	DBREQUEST_GETCHATS,
} DBRequestTypes_t;

void CG_DB_CheckResponses();

void CG_DB_InsertChat(const char* chatText);
void CG_DB_GetChats_f(void);

#endif
