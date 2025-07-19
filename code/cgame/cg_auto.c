// V24 Enhanced Features - Auto-Gameplay Systems
// This file contains the implementations for auto-kick, auto-backstab, and auto-aim functionalities

#include "cg_local.h"

// Last execution timestamps
static int lastAutoKickTime = 0;
static int lastAutoBackstabTime = 0;
static int lastEnemyDetectTime = 0;

// Friend system array
static int friendList[MAX_FRIENDS];
static int friendCount = 0;

/*
================
CG_IsFriend
Returns whether the given client is in our friends list
================
*/
qboolean CG_IsFriend(int clientNum)
{
    int i;

    if (!cg_friendsSystem.integer || clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return qfalse;
    }

    // Check friend list
    for (i = 0; i < friendCount; i++)
    {
        if (friendList[i] == clientNum)
        {
            return qtrue;
        }
    }

    return qfalse;
}

/*
================
CG_IsEnemyInRange
Checks if an enemy is within the specified distance and view angle
================
*/
qboolean CG_IsEnemyInRange(float maxDistance, float maxAngle, vec3_t enemyPos)
{
    vec3_t dir, angles, viewAngles;
    float distance, angle;

    // Calculate distance
    VectorSubtract(enemyPos, cg.refdef.vieworg, dir);
    distance = VectorLength(dir);

    // Check distance
    if (distance > maxDistance)
    {
        return qfalse;
    }

    // Check angle
    vectoangles(dir, angles);
    VectorCopy(cg.refdefViewAngles, viewAngles);

    // Normalize angles
    if (angles[YAW] > 180)
    {
        angles[YAW] -= 360;
    }
    if (viewAngles[YAW] > 180)
    {
        viewAngles[YAW] -= 360;
    }

    // Calculate angle difference
    angle = fabs(angles[YAW] - viewAngles[YAW]);
    if (angle > 180)
    {
        angle = 360 - angle;
    }

    // Check if within the view cone
    if (angle > maxAngle)
    {
        return qfalse;
    }

    return qtrue;
}

/*
================
CG_GetEnemyTargetPos
Gets the target position for an enemy, accounting for prediction
================
*/
void CG_GetEnemyTargetPos(vec3_t targetPos, centity_t *enemy)
{
    float predictionTime;
    vec3_t predictedMove;

    // Base position is the head/torso
    VectorCopy(enemy->lerpOrigin, targetPos);
    targetPos[2] += 40; // Aim at upper body

    // Apply prediction if enabled
    if (cg_autoKickPrediction.integer)
    {
        predictionTime = 0.1f; // 100ms prediction

        // Predict movement based on velocity
        VectorScale(enemy->currentState.pos.trDelta, predictionTime, predictedMove);
        VectorAdd(targetPos, predictedMove, targetPos);
    }
}

/*
================
CG_ExecuteAutoKick
Performs the actual kick command
================
*/
void CG_ExecuteAutoKick(void)
{
    // Don't execute too often
    if (cg.time - lastAutoKickTime < cg_autoKickDelay.integer)
    {
        return;
    }

    // Send kick command
    trap_SendConsoleCommand("+kick;wait 1;-kick\n");

    // Play sound alert if enabled
    if (cg_autoKickSoundAlert.integer)
    {
        trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
    }

    lastAutoKickTime = cg.time;

    if (cg_espDebug.integer)
    {
        CG_Printf("^3Auto-Kick: ^7Executed at time %d\n", cg.time);
    }
}

/*
================
CG_ExecuteAutoBackstab
Performs the actual backstab maneuver
================
*/
void CG_ExecuteAutoBackstab(void)
{
    // Don't execute too often
    if (cg.time - lastAutoBackstabTime < cg_autoBackstabDelay.integer)
    {
        return;
    }

    // Send attack command sequence
    trap_SendConsoleCommand("+attack;wait 1;-attack\n");

    // Play sound alert if enabled
    if (cg_autoBackstabSoundAlert.integer)
    {
        trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
    }

    lastAutoBackstabTime = cg.time;

    if (cg_espDebug.integer)
    {
        CG_Printf("^3Auto-Backstab: ^7Executed at time %d\n", cg.time);
    }
}

/*
================
CG_ProcessAutoKick
Main processing function for the auto-kick system
================
*/
void CG_ProcessAutoKick(void)
{
    int i;
    centity_t *enemy;
    vec3_t targetPos;
    qboolean found = qfalse;
    float bestDistance = 999999;
    int bestTarget = -1;

    if (!cg_autoKick.integer || !cg_autoDefense.integer)
    {
        return;
    }

    // Check our own state first
    if (!cg.snap || !cg.predictedPlayerState.stats[STAT_HEALTH] > 0)
    {
        return;
    }

    // Find closest enemy in range
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        enemy = &cg_entities[i];

        // Skip invalid entities
        if (!enemy->currentValid || i == cg.snap->ps.clientNum)
        {
            continue;
        }

        // Skip dead players
        if (enemy->currentState.eFlags & EF_DEAD)
        {
            continue;
        }

        // Skip spectators if configured to
        if (cg_autoKickIgnoreSpectators.integer &&
            (enemy->currentState.eType == ET_INVISIBLE ||
             enemy->currentState.eFlags & EF_NODRAW))
        {
            continue;
        }

        // Skip friends if configured to
        if (cg_autoKickIgnoreFriends.integer && CG_IsFriend(i))
        {
            continue;
        }

        // Get target position
        CG_GetEnemyTargetPos(targetPos, enemy);

        // Check if in range and angle
        if (CG_IsEnemyInRange(cg_autoKickDistance.value, cg_autoKickAngle.value, targetPos))
        {
            float distance = Distance(cg.refdef.vieworg, targetPos);

            // Track closest enemy
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestTarget = i;
                found = qtrue;
            }
        }
    }

    // Execute kick if target found
    if (found && bestTarget != -1)
    {
        CG_ExecuteAutoKick();
    }
}

/*
================
CG_ProcessAutoBackstab
Main processing function for the auto-backstab system
================
*/
void CG_ProcessAutoBackstab(void)
{
    int i;
    centity_t *enemy;
    vec3_t targetPos, enemyAngles, relativePos, forward;
    qboolean found = qfalse;
    float bestDistance = 999999;
    int bestTarget = -1;

    if (!cg_autoBackstab.integer || !cg_autoDefense.integer)
    {
        return;
    }

    // Check our own state first
    if (!cg.snap || !cg.predictedPlayerState.stats[STAT_HEALTH] > 0)
    {
        return;
    }

    // Find closest enemy for backstab
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        enemy = &cg_entities[i];

        // Skip invalid entities
        if (!enemy->currentValid || i == cg.snap->ps.clientNum)
        {
            continue;
        }

        // Skip dead players
        if (enemy->currentState.eFlags & EF_DEAD)
        {
            continue;
        }

        // Skip friends if configured to
        if (cg_autoBackstabIgnoreFriends.integer && CG_IsFriend(i))
        {
            continue;
        }

        // Get target position and angles
        VectorCopy(enemy->lerpOrigin, targetPos);
        VectorCopy(enemy->lerpAngles, enemyAngles);

        // Get enemy's forward vector
        AngleVectors(enemyAngles, forward, NULL, NULL);

        // Calculate relative position
        VectorSubtract(cg.refdef.vieworg, targetPos, relativePos);
        VectorNormalize(relativePos);

        // Check if we're behind them (dot product < 0 means we're behind)
        if (DotProduct(forward, relativePos) < -0.5f)
        {
            float distance = Distance(cg.refdef.vieworg, targetPos);

            // Check distance
            if (distance <= cg_autoBackstabDistance.value)
            {
                // Track closest enemy
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestTarget = i;
                    found = qtrue;
                }
            }
        }
    }

    // Execute backstab if target found
    if (found && bestTarget != -1)
    {
        CG_ExecuteAutoBackstab();
    }
}

/*
================
CG_ProcessAutoGameplay
Main processing function for all auto-gameplay features
================
*/
void CG_ProcessAutoGameplay(void)
{
    if (!cg.snap || !cg_autoDefense.integer)
    {
        return;
    }

    // Process auto-kick
    if (cg_autoKick.integer)
    {
        CG_ProcessAutoKick();
    }

    // Process auto-backstab
    if (cg_autoBackstab.integer)
    {
        CG_ProcessAutoBackstab();
    }
}

/*
================
CG_AddFriend
Adds a client to the friends list
================
*/
void CG_AddFriend(int clientNum)
{
    int i;

    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return;
    }

    // Check if already in list
    for (i = 0; i < friendCount; i++)
    {
        if (friendList[i] == clientNum)
        {
            return; // Already a friend
        }
    }

    // Add to list if not full
    if (friendCount < MAX_FRIENDS)
    {
        friendList[friendCount] = clientNum;
        friendCount++;

        // Notification
        if (cg_friendsSoundNotifications.integer)
        {
            trap_S_StartLocalSound(cgs.media.friendAddedSound, CHAN_LOCAL_SOUND);
        }

        CG_Printf("^5Friend added: ^7%s\n", cgs.clientinfo[clientNum].name);
    }
    else
    {
        CG_Printf("^1Friend list full (max: %d)\n", MAX_FRIENDS);
    }
}

/*
================
CG_RemoveFriend
Removes a client from the friends list
================
*/
void CG_RemoveFriend(int clientNum)
{
    int i, j;

    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return;
    }

    // Find in list
    for (i = 0; i < friendCount; i++)
    {
        if (friendList[i] == clientNum)
        {
            // Remove by shifting remaining entries
            for (j = i; j < friendCount - 1; j++)
            {
                friendList[j] = friendList[j + 1];
            }
            friendCount--;

            CG_Printf("^5Friend removed: ^7%s\n", cgs.clientinfo[clientNum].name);
            return;
        }
    }

    CG_Printf("^3Player not in friend list\n");
}

/*
================
CG_ClearFriends
Clears the entire friends list
================
*/
void CG_ClearFriends(void)
{
    friendCount = 0;
    CG_Printf("^5Friend list cleared\n");
}

/*
================
CG_ListFriends
Lists all friends in the console
================
*/
void CG_ListFriends(void)
{
    int i;

    CG_Printf("^5Friend List (%d/%d):\n", friendCount, MAX_FRIENDS);

    if (friendCount == 0)
    {
        CG_Printf("  No friends added\n");
        return;
    }

    for (i = 0; i < friendCount; i++)
    {
        if (friendList[i] >= 0 && friendList[i] < MAX_CLIENTS)
        {
            CG_Printf("  %2d: %s\n", i + 1, cgs.clientinfo[friendList[i]].name);
        }
    }
}
