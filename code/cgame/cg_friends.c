// V24 Enhanced Features - Friend System Command Implementations

#include "cg_local.h"

// Friend system command functions - these are called from the console commands
void CG_AddFriend_f(void)
{
    int clientNum;

    if (!cg_friendsSystem.integer)
    {
        CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
        return;
    }

    if (trap_Argc() < 2)
    {
        CG_Printf("^3Usage: /addfriend <client number>\n");
        return;
    }

    clientNum = atoi(CG_Argv(1));
    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        CG_Printf("^1Invalid client number. Use numbers 0-%d.\n", MAX_CLIENTS - 1);
        return;
    }

    // Check if client exists
    if (!cgs.clientinfo[clientNum].infoValid)
    {
        CG_Printf("^1Client %d is not active.\n", clientNum);
        return;
    }

    CG_AddFriend(clientNum);
}

void CG_RemoveFriend_f(void)
{
    int clientNum;

    if (!cg_friendsSystem.integer)
    {
        CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
        return;
    }

    if (trap_Argc() < 2)
    {
        CG_Printf("^3Usage: /removefriend <client number>\n");
        return;
    }

    clientNum = atoi(CG_Argv(1));
    CG_RemoveFriend(clientNum);
}

void CG_ClearFriends_f(void)
{
    if (!cg_friendsSystem.integer)
    {
        CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
        return;
    }

    CG_ClearFriends();
}

void CG_ListFriends_f(void)
{
    if (!cg_friendsSystem.integer)
    {
        CG_Printf("^3Friend system is disabled. Enable with /cg_friendsSystem 1\n");
        return;
    }

    CG_ListFriends();
}
