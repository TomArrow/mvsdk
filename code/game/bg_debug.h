

#include "q_shared.h"

typedef struct {
	char*		name;
	size_t		offset;
	int			bits;		// 0 = float
} debugField_t;


#define	NETF(x) #x,(size_t)&((entityState_t*)0)->x

extern debugField_t	debugFields[];
extern int debugFieldCount;


#define		MAXDEBUGVARS		20
#define		MAXDEBUGVARNAME		30
typedef struct debugVar_s {
	char			name[MAXDEBUGVARNAME];
	debugField_t*	field;
	int				flags;
} debugVar_t;

