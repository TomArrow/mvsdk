// V24 Enhanced Features - Auto-Gameplay Systems
// This file contains the implementations for auto-kick, auto-backstab, and auto-aim functionalities

#include "cg_local.h"
#include "cg_public.h"

#define USERCMD_SET_BUTTONS 1 // Define USERCMD_SET_BUTTONS for trap_SetUserCmdValue
#define BUTTON_JUMP 32        // Define BUTTON_JUMP (commonly 32, i.e., 1 << 5)
#define BUTTON_KICK 64        // Define BUTTON_KICK (commonly 64, i.e., 1 << 6)

// Last execution timestamps
static int lastAutoKickTime = 0;
static int lastAutoBackstabTime = 0;
static int lastEnemyDetectTime = 0;

/*
================
AngleBetweenVectors
Returns the angle in degrees between two vectors
================
*/
float AngleBetweenVectors(const vec3_t a, const vec3_t b)
{
    vec3_t aNorm, bNorm;
    float dotProduct;

    // Normalize vectors
    VectorNormalize2(a, aNorm);
    VectorNormalize2(b, bNorm);

    // Calculate dot product
    dotProduct = DotProduct(aNorm, bNorm);

    // Make sure it's in valid range
    if (dotProduct > 1.0f)
        dotProduct = 1.0f;
    else if (dotProduct < -1.0f)
        dotProduct = -1.0f;

    // Return angle in degrees
    return RAD2DEG(acos(dotProduct));
}

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
CG_EntityVisible
Checks if one entity is visible to another using trace
================
*/
qboolean CG_EntityVisible(int sourceClientNum, int targetClientNum)
{
    trace_t tr;
    vec3_t start, end;
    centity_t *source, *target;

    if (sourceClientNum < 0 || sourceClientNum >= MAX_CLIENTS ||
        targetClientNum < 0 || targetClientNum >= MAX_CLIENTS)
    {
        return qfalse;
    }

    source = &cg_entities[sourceClientNum];
    target = &cg_entities[targetClientNum];

    // Use eye position for source
    VectorCopy(source->lerpOrigin, start);
    // Fix: Use predictedPlayerState.viewheight for local client, otherwise use a constant
    if (sourceClientNum == cg.snap->ps.clientNum)
    {
        start[2] += cg.predictedPlayerState.viewheight;
    }
    else
    {
        start[2] += 40; // Use a reasonable default for non-local entities
    }

    // Use center mass for target
    VectorCopy(target->lerpOrigin, end);

    // Perform trace
    CG_Trace(&tr, start, NULL, NULL, end, sourceClientNum, CONTENTS_SOLID);

    // If trace fraction is 1, we have a clear line of sight
    return (tr.fraction == 1.0f || tr.entityNum == targetClientNum);
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
            trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
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

/*
================
CG_FindBestBackstabTarget
Finds the best target for backstabbing based on distance, angle, and prediction
================
*/
int CG_FindBestBackstabTarget(void)
{
    int i;
    int bestTarget = -1;
    float bestScore = 0;
    float maxDistance = cg_autoBackstabDistance.value;
    float maxAngle = cg_autoBackstabAngle.value;
    int currentTime = cg.time;

    // Don't execute too frequently
    if (currentTime - lastAutoBackstabTime < cg_autoBackstabDelay.integer)
    {
        return -1;
    }

    // Check if auto-backstab is enabled
    if (!cg_autoBackstab.integer)
    {
        return -1;
    }

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        centity_t *enemy;
        vec3_t dir;
        float distance, angle, score;
        playerState_t *ps = &cg.snap->ps;

        // Skip invalid clients
        if (!cg.snap || i == cg.snap->ps.clientNum)
        {
            continue;
        }

        enemy = &cg_entities[i];

        // Skip inactive enemies
        if (!enemy->currentValid || enemy->currentState.eFlags & EF_DEAD)
        {
            continue;
        }

        // Skip friends if configured to do so
        if (cg_autoBackstabIgnoreFriends.integer && CG_IsFriend(i))
        {
            continue;
        }

        // Calculate distance and angle
        VectorSubtract(enemy->lerpOrigin, cg.predictedPlayerState.origin, dir);
        distance = VectorLength(dir);

        // Skip if too far
        if (distance > maxDistance)
        {
            continue;
        }

        // Calculate viewing angle
        angle = fabs(AngleBetweenVectors(ps->viewangles, dir));

        // Skip if not in view
        if (angle > maxAngle)
        {
            continue;
        }

        // Calculate score (closer is better)
        score = 1.0f - (distance / maxDistance);

        // Adjust score based on angle (more directly behind is better)
        score *= 1.0f - (angle / maxAngle);

        // Select best target
        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = i;
        }
    }

    if (bestTarget != -1)
    {
        lastAutoBackstabTime = currentTime;
    }

    return bestTarget;
}

/*
================
CG_ExecuteBackstab
Executes a backstab attack with the specified mode
================
*/
void CG_ExecuteBackstab(int mode)
{
    int targetClient;
    usercmd_t cmd = {0};
    int buttons = BUTTON_ATTACK;

    // Find best target
    targetClient = CG_FindBestBackstabTarget();
    if (targetClient == -1)
    {
        return;
    }

    // Execute different backstab modes
    switch (mode)
    {
    case 1: // Normal backstab (LS_A_BACK)
        // Normal backstab uses regular attack button
        trap_SetUserCmdValue(USERCMD_SET_BUTTONS, &cmd, 0, NULL, buttons, 0, 0, 0, 0, 0, 0, 0, 0.0f);
        break;

    case 2: // Crouched backstab (LS_A_BACK_CR)
        // Crouched backstab adds crouch button
        buttons |= BUTTON_WALKING;
        trap_SetUserCmdValue(USERCMD_SET_BUTTONS, &cmd, 0, NULL, buttons, 0, 0, 0, 0, 0, 0, 0, 0.0f);
        break;

    case 3: // Advanced air dual backstab
        // Advanced backstab adds jump + crouch
        buttons |= BUTTON_WALKING;
        buttons |= BUTTON_JUMP;
        trap_SetUserCmdValue(USERCMD_SET_BUTTONS, &cmd, 0, NULL, buttons, 0, 0, 0, 0, 0, 0, 0, 0.0f);
        break;
    }

    // Visual alert if enabled
    if (cg_autoBackstabSoundAlert.integer)
    {
        trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
    }
}

/*
================
CG_FindBestKickTarget
Finds the best target for kicking based on distance, angle, and prediction
================
*/
int CG_FindBestKickTarget(void)
{
    int i;
    int bestTarget = -1;
    float bestScore = 0;
    float maxDistance = cg_autoKickDistance.value;
    float maxAngle = cg_autoKickAngle.value;
    int currentTime = cg.time;

    // Don't execute too frequently
    if (currentTime - lastAutoKickTime < cg_autoKickDelay.integer)
    {
        return -1;
    }

    // Check if auto-kick is enabled
    if (!cg_autoKick.integer)
    {
        return -1;
    }

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        centity_t *enemy;
        vec3_t dir;
        float distance, angle, score;
        playerState_t *ps = &cg.snap->ps;

        // Skip invalid clients
        if (!cg.snap || i == cg.snap->ps.clientNum)
        {
            continue;
        }

        enemy = &cg_entities[i];

        // Skip inactive enemies
        if (!enemy->currentValid || enemy->currentState.eFlags & EF_DEAD)
        {
            continue;
        }

        // Skip friends if configured to do so
        if (cg_autoKickIgnoreFriends.integer && CG_IsFriend(i))
        {
            continue;
        }

        // Skip spectators if configured to do so
        if (cg_autoKickIgnoreSpectators.integer && enemy->currentState.eType == ET_INVISIBLE)
        {
            continue;
        }

        // Calculate distance and angle
        VectorSubtract(enemy->lerpOrigin, cg.predictedPlayerState.origin, dir);
        distance = VectorLength(dir);

        // Skip if too far
        if (distance > maxDistance)
        {
            continue;
        }

        // Calculate viewing angle
        angle = fabs(AngleBetweenVectors(ps->viewangles, dir));

        // Skip if not in view
        if (angle > maxAngle)
        {
            continue;
        }

        // Calculate score (closer is better)
        score = 1.0f - (distance / maxDistance);

        // Adjust score based on angle (more directly in front is better)
        score *= 1.0f - (angle / maxAngle);

        // Select best target
        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = i;
        }
    }

    if (bestTarget != -1)
    {
        lastAutoKickTime = currentTime;
    }

    return bestTarget;
}

/*
================
CG_ExecuteKick
Executes a kick attack
================
*/
void CG_ExecuteKick(void)
{
    int targetClient;
    usercmd_t cmd = {0};
    int buttons = BUTTON_KICK;

    // Find best target
    targetClient = CG_FindBestKickTarget();
    if (targetClient == -1)
    {
        return;
    }

    // Execute kick
    trap_SetUserCmdValue(USERCMD_SET_BUTTONS, &cmd, 0, NULL, buttons, 0, 0, 0, 0, 0, 0, 0, 0.0f);

    // Visual alert if enabled
    if (cg_autoKickSoundAlert.integer)
    {
        trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
    }
}

/*
================
CG_ProcessAutoGameplay
Main handler for all V24 auto-gameplay systems
================
*/
/*
================
CG_FindBestAutoAimTarget
Finds the best target for auto-aim based on distance, angle, and other parameters
================
*/
int CG_FindBestAutoAimTarget(void)
{
    int i;
    int bestTarget = -1;
    float bestScore = 0;
    float maxDistance = cg_autoAimRange.value;
    float maxFOV = cg_autoAimFOV.value;
    int currentTime = cg.time;
    static int lastAutoAimTime = 0;

    // Don't execute too frequently
    if (currentTime - lastAutoAimTime < cg_autoAimDelay.integer)
    {
        return -1;
    }

    // Check if auto-aim is enabled
    if (!cg_autoAim.integer)
    {
        return -1;
    }

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        centity_t *enemy;
        vec3_t dir, predPos;
        float distance, angle, score;
        playerState_t *ps = &cg.snap->ps;

        // Skip invalid clients
        if (!cg.snap || i == cg.snap->ps.clientNum)
        {
            continue;
        }

        enemy = &cg_entities[i];

        // Skip invalid entities
        if (!enemy->currentValid)
        {
            continue;
        }

        // Skip dead players
        if (enemy->currentState.eFlags & EF_DEAD)
        {
            continue;
        }

        // Skip spectators
        if (cg_autoAimIgnoreSpectators.integer && (enemy->currentState.eType == ET_INVISIBLE ||
                                                   enemy->currentState.eFlags & EF_NODRAW))
        {
            continue;
        }

        // Skip friends if friend filtering is enabled
        if (cg_autoAimIgnoreFriends.integer && CG_IsFriend(i))
        {
            continue;
        }

        // Calculate direction to enemy
        VectorCopy(enemy->lerpOrigin, predPos);

        // Apply prediction if enabled
        if (cg_autoAimPrediction.integer)
        {
            // Simple velocity prediction
            VectorMA(predPos, 0.1f, enemy->currentState.pos.trDelta, predPos);
        }

        VectorSubtract(predPos, cg.refdef.vieworg, dir);
        distance = VectorLength(dir);

        // Check if within range
        if (distance > maxDistance)
        {
            continue;
        }

        // Check if within field of view
        angle = AngleBetweenVectors(cg.refdef.viewaxis[0], dir);
        if (angle > maxFOV)
        {
            continue;
        }

        // Check line of sight if wall penetration is disabled
        if (!cg_autoAimWallPenetrate.integer && !CG_EntityVisible(cg.predictedPlayerState.clientNum, enemy->currentState.number))
        {
            continue;
        }

        // Calculate score based on distance and angle from center
        // Lower distances and angles are better
        score = (1.0f - (distance / maxDistance)) * 0.5f + (1.0f - (angle / maxFOV)) * 0.5f;

        // Take the best target
        if (score > bestScore)
        {
            bestScore = score;
            bestTarget = i;
        }
    }

    // If we found a target, update last aim time
    if (bestTarget != -1)
    {
        lastAutoAimTime = currentTime;

        // Debug output
        if (cg_espDebug.integer)
        {
            CG_Printf("^3Auto-Aim: ^7Found target %d with score %.2f\n", bestTarget, bestScore);
        }
    }

    return bestTarget;
}

/*
================
CG_ExecuteAutoAim
Executes an auto-aim operation by adjusting view angles toward the target
================
*/
void CG_ExecuteAutoAim(void)
{
    int targetClient;
    vec3_t targetPos, targetDir, currentAngles, targetAngles;
    centity_t *target;
    float damping;

    // Find best target
    targetClient = CG_FindBestAutoAimTarget();
    if (targetClient == -1)
    {
        return;
    }

    // Get target entity
    target = &cg_entities[targetClient];

    // Calculate target position with prediction if enabled
    VectorCopy(target->lerpOrigin, targetPos);
    if (cg_autoAimPrediction.integer)
    {
        // Simple velocity prediction
        VectorMA(targetPos, 0.1f, target->currentState.pos.trDelta, targetPos);
    }

    // Calculate direction to target
    VectorSubtract(targetPos, cg.refdef.vieworg, targetDir);
    vectoangles(targetDir, targetAngles);

    // Apply auto-aim adjustment to view angles
    CG_ApplyAutoAim(targetAngles);

    // Visual and sound alerts if enabled
    if (cg_autoAimVisualAlert.integer)
    {
        // Visual effect could be added here
    }

    if (cg_autoAimSoundAlert.integer)
    {
        trap_S_StartLocalSound(cgs.media.selectSound, CHAN_LOCAL_SOUND);
    }
}

/*
================
CG_ApplyAutoAim
Applies auto-aim angle adjustments to the current view
================
*/
void CG_ApplyAutoAim(vec3_t targetAngles)
{
    vec3_t currentAngles;
    float damping = cg_autoAimDamping.value;

    // Use default damping if value is invalid
    if (damping <= 0.0f)
        damping = 0.5f;

    // Get current view angles
    VectorCopy(cg.refdefViewAngles, currentAngles);

    // Normalize angles
    if (targetAngles[YAW] - currentAngles[YAW] > 180)
        targetAngles[YAW] -= 360;
    if (targetAngles[YAW] - currentAngles[YAW] < -180)
        targetAngles[YAW] += 360;

    // Apply damping
    currentAngles[PITCH] += (targetAngles[PITCH] - currentAngles[PITCH]) * damping;
    currentAngles[YAW] += (targetAngles[YAW] - currentAngles[YAW]) * damping;

    // Store the suggested view angles for automation
    VectorCopy(currentAngles, cg.autoSuggestedViewAngles);
}

/*
================
CG_ProcessAutoAim
Main processing function for the auto-aim system
================
*/
void CG_ProcessAutoAim(void)
{
    int i;

    if (!cg_autoAim.integer)
    {
        return;
    }

    // Check our own state first
    if (!cg.snap || !cg.predictedPlayerState.stats[STAT_HEALTH] > 0)
    {
        return;
    }

    // Execute auto-aim if enabled
    if (cg_autoAim.integer >= 1)
    {
        CG_ExecuteAutoAim();
    }
}

/*
================
CG_ProcessAutoGameplay
Main handler for all V24 auto-gameplay systems
================
*/
void CG_ProcessAutoGameplay(void)
{
    // Don't process anything if not in a valid game state
    if (!cg.snap || !cg_autoDefense.integer)
    {
        return;
    }

    // Process auto-backstab
    if (cg.doAutoBackstab && cg_autoBackstab.integer)
    {
        CG_ExecuteBackstab(cg.autoBackstabMode);
    }

    // Process auto-kick
    if (cg.doAutoKick && cg_autoKick.integer)
    {
        CG_ExecuteKick();
    }

    // Process auto-aim
    if (cg_autoAim.integer)
    {
        CG_ProcessAutoAim();
    }

    // Handle continuous auto-backstab mode (mode 3: fully automatic)
    if (cg_autoBackstab.integer == 3)
    {
        cg.doAutoBackstab = qtrue;
        cg.autoBackstabMode = 1;
        CG_ExecuteBackstab(1);
    }

    // Handle continuous auto-kick mode (mode 3: fully automatic)
    if (cg_autoKick.integer == 3)
    {
        cg.doAutoKick = qtrue;
        CG_ExecuteKick();
    }
}
