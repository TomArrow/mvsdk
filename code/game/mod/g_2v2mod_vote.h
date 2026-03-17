#ifndef G_2V2MOD_VOTE_H
#define G_2V2MOD_VOTE_H

typedef struct {
    const char       *name;
    const tvt_Cmd_t  *cmd;
    const tvt_Cvar_t *cvar;
} tvt_VoteItem_t;

void     G_TvT_Vote_Init(void);
int      G_TvT_CallVote(gentity_t *ent, const char *arg1, const char *arg2);
qboolean G_TvT_Cmd_VoteList(gentity_t *ent);

#endif
