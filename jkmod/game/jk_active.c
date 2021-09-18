/*
======================= Jedi Knight Plus Mod ========================
By Tr!Force. Work copyrighted (C) with holder attribution 2005 - 2022
=====================================================================
[Description]: Client active module
=====================================================================
*/

#include "../../code/game/g_local.h" // Original header

// Extern stuff
extern int trap_RealTime(qtime_t *qtime);

/*
=====================================================================
Client timer actions function
=====================================================================
*/
void JKMod_ClientTimerActions(gentity_t *ent, int msec) 
{
	gclient_t	*client;
	qtime_t		serverTime;
	char		*serverTimeType;
	char		serverMotd[MAX_STRING_CHARS];

	trap_RealTime(&serverTime);
	serverTimeType = (serverTime.tm_hour > 11 && serverTime.tm_hour < 24) ? "pm" : "am";

	client = ent->client;

	// Drop flag check
	if (client->jkmodClient.dropFlagTime)
	{
		if (client->jkmodClient.dropFlagTime > 0) client->jkmodClient.dropFlagTime--;
		else client->jkmodClient.dropFlagTime = 0;
	}

	// Call vote check
	if (client->jkmodClient.voteWaitTime)
	{
		if (client->jkmodClient.voteWaitTime > 0) client->jkmodClient.voteWaitTime--;
		else client->jkmodClient.voteWaitTime = 0;
	}

	// Dimension time check
	if (client->jkmodClient.dimensionTime)
	{
		if (client->jkmodClient.dimensionTime > 0) client->jkmodClient.dimensionTime--;
		else client->jkmodClient.dimensionTime = 0;
	}

	// Teleport chat time check
	if (client->jkmodClient.teleportChatTime)
	{
		if (client->jkmodClient.teleportChatTime > 0) client->jkmodClient.teleportChatTime--;
		else client->jkmodClient.teleportChatTime = 0;
	}

	// Chat protect check
	if (jkcvar_chatProtect.integer && (client->ps.eFlags & EF_TALK) && !(g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY))
	{
		if (client->jkmodClient.chatTime >= jkcvar_chatProtectTime.integer)
		{
			client->jkmodClient.chatTime = jkcvar_chatProtectTime.integer;

			if (!(client->ps.stats[JK_PLAYER] & JK_CHAT_IN)) client->ps.stats[JK_PLAYER] |= JK_CHAT_IN;
			if (!(client->ps.eFlags & JK_PASS_THROUGH) && jkcvar_chatProtect.integer == 3) client->ps.eFlags |= JK_PASS_THROUGH;
			if (!client->pers.jkmodPers.invulnerability) client->pers.jkmodPers.invulnerability = qtrue;
			if (!client->ps.saberHolstered && !JKMod_EmoteIn(ent, -1)) Cmd_ToggleSaber_f(ent);
		}
		else
		{
			client->jkmodClient.chatTime++;
		}
	}

	// Show server motd
	if (client->jkmodClient.motdTime)
	{
		if (client->jkmodClient.motdTime <= jkcvar_serverMotdTime.integer)
		{
			JKMod_StringEscape(jkcvar_serverMotd.string, serverMotd, MAX_STRING_CHARS);
			G_CenterPrint(client->ps.clientNum, 3, va("%s\nTime: %d\n", serverMotd, client->jkmodClient.motdTime));
		}

		client->jkmodClient.motdTime--;
	}

	// Server news
	if (jkcvar_serverNews.integer && VALIDSTRING(level.jkmodLocals.serverNews[0]) && g_gametype.integer != GT_TOURNAMENT)
	{
		int i;
		int total = level.jkmodLocals.serverNewsCount;
			
		level.jkmodLocals.serverNewsNum++;

		for (i = 1; i < (jkcvar_serverNewsTime.integer * total); i++)
		{
			if (level.jkmodLocals.serverNewsNum == (jkcvar_serverNewsTime.integer * i))
			{
				trap_SendServerCommand(client->ps.clientNum, va("print \"Server News ^5[^7%02i^5:^7%02i%s^5]^7: %s\n\"", serverTime.tm_hour, serverTime.tm_min, serverTimeType, level.jkmodLocals.serverNews[(i-1)]));
					
				// Reset
				if (level.jkmodLocals.serverNewsNum == ((jkcvar_serverNewsTime.integer * total))) {
					level.jkmodLocals.serverNewsNum = 0;
				}
			}
		}
	}

	// Check jetpack fuel
	if (jkcvar_jetPack.integer == 1 && (ent->client->ps.eFlags & JK_JETPACK_ACTIVE))
	{
		int amount = 5;

		if (ent->client->ps.eFlags & JK_JETPACK_FLAMING)
		{
			if (ent->client->ps.stats[JK_FUEL] > 0) {
				// Reduce
				ent->client->ps.stats[JK_FUEL] -= amount;
			} else {
				// Turn off
				ent->client->ps.eFlags &= ~JK_JETPACK_FLAMING;
			}
		}
		else if (ent->client->ps.stats[JK_FUEL] < 100) {
			// Recharge
			ent->client->ps.stats[JK_FUEL] += amount;
		}
	}
}

/*
=====================================================================
Client think real function
=====================================================================
*/
void JKMod_ClientThink_real(gentity_t *ent)
{
	usercmd_t	*cmd;

	// Checks
	if (!ent || !ent->client) return;
	if (ent->client->pers.connected != CON_CONNECTED) return;
	
	cmd = &ent->client->pers.cmd;

	// Already in an emote
	if (JKMod_EmoteIn(ent, -1))
	{
		if (ent->client->ps.pm_type == PM_DEAD)
		{
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			ent->client->ps.forceDodgeAnim = 0;
			ent->client->ps.forceHandExtendTime = 0;
		}
		else if (JKMod_EmoteIn(ent, 0))
		{	
			// In a frozen emote
			if (ent->client->pers.cmd.upmove > 0)
			{
				ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
				ent->client->ps.forceDodgeAnim = 0;
				ent->client->ps.forceHandExtendTime = 0;
			}
			else
			{
				ent->client->ps.forceHandExtend = HANDEXTEND_DODGE;
				ent->client->ps.forceDodgeAnim = (ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT);
				ent->client->ps.forceHandExtendTime = level.time + INFINITE;
			}
		}
		else if (JKMod_EmoteIn(ent, 1) && ent->client->pers.cmd.upmove > 0)
		{	
			// In an animation emote
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			ent->client->ps.forceDodgeAnim = 0;
			ent->client->ps.forceHandExtendTime = 0;
		}
		else if (JKMod_EmoteIn(ent, 2))
		{	
			// In a special emote (walkable)
			ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
			ent->client->ps.forceDodgeAnim = (ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT);
			ent->client->ps.forceHandExtendTime = level.time + INFINITE;
		}

		// Keep the client informed about to predict the emote leg timers
		if (!(ent->client->ps.stats[JK_PLAYER] & JK_EMOTE_IN)) ent->client->ps.stats[JK_PLAYER] |= JK_EMOTE_IN;
	}
	else if (ent->client->ps.stats[JK_PLAYER] & JK_EMOTE_IN)
	{
		if (JKMod_OthersInBox(ent)) JKMod_AntiStuckBox(ent);
		ent->client->ps.stats[JK_PLAYER] &= ~JK_EMOTE_IN;
	}

	// Check player pass-through
	if (ent->client->ps.eFlags & JK_PASS_THROUGH)
	{
		if (ent->r.contents & CONTENTS_BODY) ent->r.contents &= ~CONTENTS_BODY;
		if (!ent->client->pers.jkmodPers.passThrough) ent->client->pers.jkmodPers.passThrough = qtrue;
	}
	else if (ent->client->pers.jkmodPers.passThrough) {
		if (!(ent->r.contents & CONTENTS_BODY)) ent->r.contents = CONTENTS_BODY;
		ent->client->pers.jkmodPers.passThrough = qfalse;
	}

	// Check player anti-stuck
	if ((ent->client->ps.stats[JK_PLAYER] & JK_ANTI_STUCK) && !(ent->client->ps.eFlags & JK_PASS_THROUGH))
	{
		if (JKMod_OthersInBox(ent)) {
			if (ent->r.contents & CONTENTS_BODY) {
				ent->r.contents &= ~CONTENTS_BODY;
				JKMod_AntiStuckBox(ent);
			}
		} else {
			if (!(ent->r.contents & CONTENTS_BODY)) ent->r.contents = CONTENTS_BODY;
			ent->client->ps.stats[JK_PLAYER] &= ~JK_ANTI_STUCK;
		}
	}

	// Check chat off
	if (ent->client->ps.stats[JK_PLAYER] & JK_CHAT_IN)
	{
		if (!(ent->client->ps.eFlags & EF_TALK))
		{
			if (!ent->client->pers.jkmodPers.passThroughPerm) 
			{
				if (ent->client->ps.groundEntityNum != ENTITYNUM_WORLD) { ent->client->ps.origin[2] += 2; ent->client->ps.eFlags ^= EF_TELEPORT_BIT; } // Workaround
				if (ent->client->ps.eFlags & JK_PASS_THROUGH) ent->client->ps.eFlags &= ~JK_PASS_THROUGH;
				if (JKMod_OthersInBox(ent)) JKMod_AntiStuckBox(ent);
			}
			if (ent->client->jkmodClient.chatTime != 0) ent->client->jkmodClient.chatTime = 0;
			if (ent->client->pers.jkmodPers.invulnerability) ent->client->pers.jkmodPers.invulnerability = qfalse;

			ent->client->ps.stats[JK_PLAYER] &= ~JK_CHAT_IN;
		}
	}

	// Check race dimension saber toogle
	if (ent->client->ps.stats[JK_DIMENSION] == DIMENSION_RACE && !ent->client->ps.saberHolstered)
	{
		ent->client->ps.saberHolstered = qtrue;
		ent->client->ps.weaponTime = 400;
	}

	// Check jetpack flaming
	if (!(ent->client->ps.eFlags & JK_JETPACK_ACTIVE) || ent->client->ps.pm_type == PM_DEAD)
	{
		ent->client->ps.eFlags &= ~JK_JETPACK_FLAMING;
	}

	// Check invulnerability
	if (ent->client->pers.jkmodPers.invulnerability)
	{
		if (ent->takedamage) ent->takedamage = qfalse;
	}
	else
	{
		if (!ent->takedamage && ent->health > 0) ent->takedamage = qtrue;
	}

	// Tr!Force: [ButtonUse] Trigger animation
	if ((cmd->buttons & BUTTON_USE) 
		&& (ent->client->ps.stats[JK_MOVEMENT] & JK_USE_STAND) 
		&& (ent->client->pers.jkmodPers.buttonUseAnimValid)
		&& !(ent->r.svFlags & SVF_BOT) 
		&& !(ent->client->ps.eFlags & JK_JETPACK_FLAMING) 
		&& !((ent->client->ps.eFlags & JK_JETPACK_ACTIVE) && ent->client->ps.groundEntityNum == ENTITYNUM_NONE) 
		&& !JKMod_PlayerMoving(ent, qfalse, qtrue))
	{
		ent->client->pers.jkmodPers.buttonUseAnim = qtrue;
		ent->client->ps.saberMove = LS_NONE;
		ent->client->ps.saberBlocked = 0;
		ent->client->ps.saberBlocking = 0;
		ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
		ent->client->ps.forceDodgeAnim = BOTH_BUTTON_HOLD;
		ent->client->ps.forceHandExtendTime = level.time + INFINITE;
	}
	else if (ent->client->pers.jkmodPers.buttonUseAnim)
	{
		ent->client->pers.jkmodPers.buttonUseAnim = qfalse;
		ent->client->pers.jkmodPers.buttonUseAnimValid = qfalse;
		ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
		ent->client->ps.forceDodgeAnim = 0;
		ent->client->ps.forceHandExtendTime = 0;
	}

	// Launch original client think real function
	BaseJK2_ClientThink_real(ent);
}

/*
=====================================================================
Run client function
=====================================================================
*/
void JKMod_RunClient(gentity_t *ent) 
{
	qboolean GTconfigLoaded = jkcvar_gameTypeConfig.integer && level.newSession ? level.jkmodLocals.cvarTempUnlock == 2 : qtrue;

	if (jkcvar_antiWarp.integer)
	{
		gclient_t	*client = ent->client;
		usercmd_t	*cmd = &client->pers.cmd;

		if ((ent->r.svFlags & SVF_BOT) || g_synchronousClients.integer)
		{
			// Call ClientBegin for delayed bots now
			if (ent->client->pers.botDelayed && GTconfigLoaded)
			{ 
				ClientBegin(ent - g_entities, qtrue);
				ent->client->pers.botDelayed = qfalse;
			}

			cmd->serverTime = level.time;
			ClientThink_real(ent);
		}
		else if (client->lastCmdTime > 0 &&
			client->lastCmdTime < level.time - (jkcvar_antiWarpTime.value*1000) &&
			client->pers.connected == CON_CONNECTED &&
			client->sess.spectatorState == SPECTATOR_NOT &&
			client->ps.pm_type != PM_DEAD)
		{
			client->ps.eFlags |= EF_CONNECTION;

			if (jkcvar_antiWarp.integer == 2)
			{
				// Create a fake user command to make him move, causing client prediction error for a warping player
				cmd->serverTime = level.time + (cmd->serverTime - client->lastCmdTime);
				cmd->buttons = 0;
				cmd->generic_cmd = 0; // Let go any force power
				cmd->forwardmove = 0;
				cmd->rightmove = 0;
				cmd->upmove = 0;

				ClientThink_real(ent);
			}
		}
		else
		{
			client->ps.eFlags &= ~EF_CONNECTION;
		}
	}
	else
	{
		if (!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer) 
		{
			return;
		}

		if (ent->client->pers.botDelayed && GTconfigLoaded) // Call ClientBegin for delayed bots now
		{
			ClientBegin(ent-g_entities, qtrue);
			ent->client->pers.botDelayed = qfalse;
		}

		ent->client->pers.cmd.serverTime = level.time;
		ClientThink_real(ent);
	}
}

/*
=====================================================================
Pause client think function
=====================================================================
*/
void JKMod_PauseClientThink(gentity_t *ent) 
{
	gclient_t	*client = ent->client;
	usercmd_t	*cmd = &client->pers.cmd;

	// Stop command time
	client->ps.commandTime = cmd->serverTime;

	// Check chat flag
	if ( cmd->buttons & BUTTON_TALK ) {
		ent->s.eFlags |= EF_TALK;
		client->ps.eFlags |= EF_TALK;
	} else {
		ent->s.eFlags &= ~EF_TALK;
		client->ps.eFlags &= ~EF_TALK;
	}

	// Stop movement and prediction
	client->ps.pm_type = PM_SPINTERMISSION;

	// Force view angles
	SetClientViewAngle(ent, client->ps.viewangles);
}
