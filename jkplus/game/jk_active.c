/*
======================= Jedi Knight Plus Mod ========================
By Tr!Force. Work copyrighted (C) with holder attribution 2005 - 2020
=====================================================================
[Description]: Client active module
=====================================================================
*/

#include "../../code/game/g_local.h" // Original header

/*
=====================================================================
Client timer actions function
=====================================================================
*/

void JKPlus_ClientTimerActions(gentity_t *ent, int msec) {
	gclient_t	*client;
	char		serverMotd[MAX_STRING_CHARS];

	client = ent->client;
	client->JKPlusTimeResidual += msec;

	if (jkcvar_pauseGame.integer) // Tr!Force: [Pause] Don't allow
	{
		return;
	}

	// Launch original client timer actions function
	BaseJK2_ClientTimerActions(ent, msec);

	// Custom time actions
	while (client->JKPlusTimeResidual >= 1000)
	{
		client->JKPlusTimeResidual -= 1000;

		// Drop flag check
		if (client->JKPlusDropFlagTime)
		{
			if (client->JKPlusDropFlagTime > 0)
			{
				client->JKPlusDropFlagTime--;
			}
			else
			{
				client->JKPlusDropFlagTime = 0;
			}
		}

		// Chat protect check
		if (jkcvar_chatProtect.integer && (client->ps.eFlags & EF_TALK))
		{
			if (client->JKPlusChatTime >= jkcvar_chatProtectTime.integer)
			{
				client->JKPlusChatTime = jkcvar_chatProtectTime.integer;
				ent->flags |= FL_GODMODE;
			}
			else
			{
				client->JKPlusChatTime++;
			}
		}
		else
		{
			if (client->JKPlusChatTime != 0) client->JKPlusChatTime = 0;
			if (ent->flags & FL_GODMODE) ent->flags &= ~FL_GODMODE;
		}

		// Show server motd
		if (client->JKPlusMotdTime && *jkcvar_serverMotd.string && jkcvar_serverMotd.string[0] && !Q_stricmp(jkcvar_serverMotd.string, "0") == 0)
		{
			JKPlus_stringEscape(jkcvar_serverMotd.string, serverMotd, MAX_STRING_CHARS);
			trap_SendServerCommand(ent->client->ps.clientNum, va("cp \"%s\nTime limit: %d\"", serverMotd, ent->client->JKPlusMotdTime));
			client->JKPlusMotdTime--;
		}
	}
}

/*
=====================================================================
Client think real function
=====================================================================
*/

void JKPlus_ClientThink_real(gentity_t *ent)
{
	// Already in an emote
	if (JKPlus_emoteIn(ent, -1))
	{
		if (ent->client->ps.pm_type == PM_DEAD)
		{
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			ent->client->ps.forceDodgeAnim = 0;
			ent->client->ps.forceHandExtendTime = 0;
		}
		else if (JKPlus_emoteIn(ent, 0))
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
		else if (JKPlus_emoteIn(ent, 1) && ent->client->pers.cmd.upmove > 0)
		{	
			// In an animation emote
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			ent->client->ps.forceDodgeAnim = 0;
			ent->client->ps.forceHandExtendTime = 0;
		}
		else if (JKPlus_emoteIn(ent, 2))
		{	
			// In a special emote (walkable)
			ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
			ent->client->ps.forceDodgeAnim = (ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT);
			ent->client->ps.forceHandExtendTime = level.time + INFINITE;
		}

		// Keep the client informed about to predict the emote leg timers
		if (!(ent->client->ps.eFlags & EF_EMOTE_IN))
		{
			ent->client->ps.eFlags |= EF_EMOTE_IN;
		}
	}
	else if (ent->client->ps.eFlags & EF_EMOTE_IN)
	{
		ent->client->ps.eFlags &= ~EF_EMOTE_IN;
	}

	// Launch original client think real function
	BaseJK2_ClientThink_real(ent);
}