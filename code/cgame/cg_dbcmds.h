
#ifndef CG_DBCMDS_H
#define CG_DBCMDS_H

typedef enum DBRequestTypes {
	DBREQUEST_CHATSAVE,
	DBREQUEST_GETCHATS,
};

void CG_DB_CheckResponses();

void CG_DB_InsertChat(const char* chatText);
void CG_DB_GetChats_f(void);


#endif
