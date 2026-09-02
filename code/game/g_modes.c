
#include "g_local.h"
#include "bg_local.h"

void SetClientPhysicsFps(gentity_t* ent, int clientSetting);
void RemoveLaserTraps(gentity_t* ent);
void RemoveDetpacks(gentity_t* ent);
void DeletePlayerProjectiles(gentity_t* ent);
void Cmd_ForceChanged_Real_f(gentity_t* ent, qboolean isCmd);
void ResetPhysicsFpsStuff(gentity_t* ent);

static const char* teamSetStrings[TEAM_NUM_TEAMS] = {
	"f",
	"r",
	"b",
	"s"
};

int settingModeTeam[MAX_CLIENTS] = { 0 };


// if capper, returns amounts of milliseconds since begin of capping
// strict = always return qtrue if player is capper. not strict: check if theres any other non-afk ironman players and only return qtrue if there are. (we are only interested in whether we should restrict the capper's actions like /kill etc)
int G_ClientIsIronmanCapper(gentity_t* ent, qboolean strict) {
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->sess.mode == MODE_IRONMAN && ent->client->sess.modeTeam == MODETEAM_IRONMAN_CAPPER && (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG] || ent->client->ps.powerups[PW_NEUTRALFLAG])) {
		if (!strict) {
			int i;
			gentity_t* other;
			int ironmanchasers = 0;
			for (i = 0; i < level.maxclients; i++) {
				other = g_entities + i;
				if (!other->inuse || !other->client || other->client->sess.sessionTeam == TEAM_SPECTATOR || other == ent) {
					continue;
				}
				if (other->client->sess.mode == MODE_IRONMAN && clampedIntAdd(level.time, -other->client->sess.lastHereTime) < 10000) {
					return level.time - ent->client->pers.lastIronmanFlagGiven;
				}
			}
			return 0;
		}
		return level.time - ent->client->pers.lastIronmanFlagGiven;
	}
	return 0;
}

// check if we need to adjust mode team on respawn
void ModeClientRespawning(gentity_t* ent) {
	modeTeam_t* modeTeamData = &modeTeams[ent->client->sess.modeTeam];
	// settingModeTeam makes sure that we don't recursively mess up our own mode team setting
	// because setteam will do ClientBegin, and as such ClientSpawn, which is calling this.
	// Obviously the first time we set a particular modeteam, we just want to set it, and we don't want the 
	// ClientSpawn to immediately move to the queued modeteam after that
	if (!settingModeTeam[ent-g_entities] && modeTeamData->respawnTeam > MODETEAM_INVALID) {
		ClientSetModeTeam(ent, modeTeamData->respawnTeam);
	}
}

qboolean ModePreventDamage(gentity_t* attacker, gentity_t* target) {
	if (attacker->client->sess.modeTeam == target->client->sess.modeTeam && modeTeams[attacker->client->sess.modeTeam].friendlyTeam == MTH_FRIENDLY) {
		return qtrue;
	}
	return qfalse;
}

team_t ValidateClientModeTeam(gentity_t* ent, team_t wishTeam) {
	modeTeam_t* modeTeamData = &modeTeams[ent->client->sess.modeTeam];
	if (!modeTeamData->applyRealTeam || g_gametype.integer < GT_TEAM || wishTeam == TEAM_SPECTATOR) {
		return wishTeam;
	}
	else {
		return modeTeamData->realTeam;
	}
}

// returns qtrue if ClientUserinfoChanged was called inside
qboolean ClientSetModeTeam(gentity_t* ent, modeTeam_e modeTeam) {
	modeTeam_t* modeTeamData;
	qboolean needUserInfoChange = qtrue; //qfalse; actually, always true because we wanna inform about the client's modeteam via configstring
	team_t targetTeam;
	if (ent->client->sess.modeTeam == modeTeam) {
		return qfalse;
	}
	settingModeTeam[ent - g_entities]++;
	ent->client->sess.modeTeam = modeTeam;
	modeTeamData = &modeTeams[modeTeam];
	targetTeam = ValidateClientModeTeam(ent, ent->client->sess.sessionTeam);
	if (targetTeam != ent->client->sess.sessionTeam) {
		SetTeam(ent, teamSetStrings[targetTeam]);
		needUserInfoChange = qtrue;
	}
	else if (modeTeamData->applyPrefix) {
		needUserInfoChange = qtrue;
	}
	else if (modeTeamData->applyTeamColors && g_gametype.integer < GT_TEAM) {
		needUserInfoChange = qtrue;
	}

	if (needUserInfoChange) {
		ClientUserinfoChanged(ent - g_entities);
		settingModeTeam[ent - g_entities]--;
		CalculateRanks();
		return qtrue;
	}

	settingModeTeam[ent - g_entities]--;
	return qfalse;

}

void ClientSetModeReal(gentity_t* ent, playerMode_e mode) {
	qboolean isRace = (qboolean)(mode == MODE_DEFRAG);

	if (ent->client->sess.mode == mode && ent->client->sess.raceMode == isRace) {
		return;
	}
	ent->client->sess.mode = mode;
	ent->client->sess.raceMode = isRace;

	if (!ClientSetModeTeam(ent, modeDefaultTeams[mode])) { // ClientSetModeTeam already calls ClientUserinfoChanged if it changes something. No need to do it twice.
		ClientUserinfoChanged(ent - g_entities);
		CalculateRanks();
	}

	ent->s.weapon = WP_SABER; //Dont drop our weapon
	if (!isRace) Cmd_ForceChanged_Real_f(ent,qfalse);//Make sure their jump level is valid.. if leaving racemode :S//Delete all their projectiles / saved stuff

	// reset physicsfps because racemode has different rules for validating that.
	ResetPhysicsFpsStuff(ent);

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		//Delete all their projectiles / saved stuff
		RemoveLaserTraps(ent);
		RemoveDetpacks(ent);
		DeletePlayerProjectiles(ent);

		if (ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR) { // killing a player links him. catastrophe if not yet inuse :) also not great when in spec.
			G_Kill(ent); //stop abuse
			// ok this is fucking disgusting but hear me out...
			// depending on the situation we MAY change the player's skin
			// which will force cgame to load the model anew
			// and there's some weird bug in cgame where it will then recognize
			// the model change and set the animation again, but it will set it to the 
			// wrong animation due to some prediction glitch (doesn't happen with cg_nopredict 1)
			// which then causes the model to get stuck in a non-moving standing position
			// which is just plain ugly.
			// since we may be able to fix the client bug, but we cannot force everyone to have a fixed client
			// do this to disable prediction for the frame.
			// so much text for such a trivial thing, damn.
			ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
		}
		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->ps.persistant[PERS_KILLED] = 0;
		ent->client->accuracy_shots = 0;
		ent->client->accuracy_hits = 0;
		ent->client->ps.fd.suicides = 0;
		ent->client->pers.enterTime = level.time; //reset scoreboard kills/deaths i guess... and time?

		if (!ent->client->sess.firstEnterTimeSet) {
			ent->client->sess.firstEnterTime = level.time;
			ent->client->sess.firstEnterTimeSet = qtrue;
		}
	}
	UpdateClientRaceVars(ent->client);
	G_SendServerCommand(ent - g_entities, va("print \"^3Mode updated: %s\n\"", modeNames[mode].string), qtrue);
}


int GetDefaultPlayerMode(qboolean allowDefrag) {
	playerMode_e mode = MODE_NORMAL;

	if (g_defrag.integer && allowDefrag) {
		mode = MODE_DEFRAG;
	}
	else if (g_modes.integer && g_modesDefault.integer && g_modesDefault.integer < MODE_NUM_MODES) {
		mode = g_modesDefault.integer;
	}
	if (mode == MODE_DEFRAG && !allowDefrag || mode == MODE_INVALID) {
		mode = MODE_NORMAL;
	}
	return mode;
}

void SetClientMode(gentity_t* ent, playerMode_e mode) {
	if (!ent->client)
		return;

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && (ent->client->ps.powerups[PW_NEUTRALFLAG] || ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG]) && ent->client->sess.mode != MODE_IRONMAN) { // maybe not let capper switch unless theres no other players left in ironman or theyre afk or whatever? meh.
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed when carrying a flag!\n\"");
		return;
	}

	if (!g_defrag.integer && mode == MODE_DEFRAG) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		//DF_SetRaceMode(ent, qfalse);
		mode = MODE_NORMAL;
		//return;
	}

	if (mode > MODE_DEFRAG && !g_modes.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		mode = MODE_NORMAL;
	}

	if (mode >= MODE_NUM_MODES || mode < 0) {
		trap_SendServerCommand(ent - g_entities, "print \"^5Invalid mode specified!\n\"");
		mode = MODE_NORMAL;
	}

	if (g_gametype.integer != GT_FFA) { // TA: What the heck is this?!
		if (g_gametype.integer >= GT_TEAM && g_defrag.integer && mode == MODE_DEFRAG)
		{//this is ok

			ent->s.weapon = WP_SABER; //Dont drop our weapon
			Cmd_ForceChanged_Real_f(ent,qfalse);//Make sure their jump level is valid.. if leaving racemode :S

			ent->client->sess.mode = MODE_INVALID; // we will reset this
			ent->client->sess.raceMode = qfalse;//Set it false here cuz we are flipping it next // TA: (wut? oh.)
			if (ent->client->sess.sessionTeam != TEAM_FREE) {
				SetTeam(ent, "race");
			}
			else {
				SetTeam(ent, "spec");
			}
		}
		else {

			//trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed in this gametype!\n\"");
			//return;
		}
	}

	// toggle
	if (mode == ent->client->sess.mode && mode != MODE_NORMAL) {
		int defaultMode = GetDefaultPlayerMode(qtrue);
		if (g_modes.integer && mode == defaultMode) {
			mode = MODE_NORMAL;
		}
		else {
			mode = defaultMode;
		}
	}

	ClientSetModeReal(ent, mode);
}

qboolean ClientModeValid(gentity_t* ent, qboolean allowDefrag) {
	return ent->client->sess.mode == MODE_NORMAL || ent->client->sess.mode == MODE_DEFRAG && g_defrag.integer && ent->client->sess.raceMode && allowDefrag || ent->client->sess.mode > MODE_DEFRAG && ent->client->sess.mode < MODE_NUM_MODES&& g_modes.integer;
}


void ClientSetDefaultMode(gentity_t* ent, qboolean allowDefrag) {
	ent->client->sess.mode = MODE_INVALID; // force it 
	ClientSetModeReal(ent, GetDefaultPlayerMode(allowDefrag));
}

void ResetClientModeIfInvalid(gentity_t* ent, qboolean allowDefrag) {
	if (!ClientModeValid(ent, allowDefrag)) {
		Com_Printf("^3Client %d mode invalid, resetting: %d (racemode %d)\n", (int)(ent - g_entities), ent->client->sess.mode, ent->client->sess.raceMode);
		G_SendServerCommand(ent - g_entities, va("print \"^3Mode invalid, resetting: %d (racemode %d)\n\"", ent->client->sess.mode, ent->client->sess.raceMode), qtrue);
		ClientSetDefaultMode(ent, allowDefrag);
	}
}

void Cmd_Mode_f(gentity_t* ent)
{
	char mode[20];
	int modeNum;
	if (!ent->client)
		return;

	trap_Argv(1, mode, sizeof(mode));
	if (!Q_stricmp(mode, "reset"))
	{
		modeNum = GetDefaultPlayerMode(qtrue);
		if (modeNum == ent->client->sess.mode) {
			ent->client->sess.mode = MODE_INVALID; // force a reset
		}
	}
	else {
		modeNum = PlayerModeNameToInteger(mode);
	}
	if (modeNum == -1) {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid mode specified. Valid modes: reset, normal, defrag, duel, duelqueue, allforce, ironman\n\"");
		return;
	}

	SetClientMode(ent, modeNum);

}



void Cmd_ModeCmd_f(gentity_t* ent)
{
	char mode[20];
	int modeNum;
	sanction_t* sanction;
	if (!ent->client)
		return;

	trap_Argv(0, mode, sizeof(mode));
	if (!Q_stricmp(mode, "reset"))
	{
		modeNum = GetDefaultPlayerMode(qtrue);
		if (modeNum == ent->client->sess.mode) {
			ent->client->sess.mode = MODE_INVALID; // force a reset
		}
	}
	else {
		modeNum = PlayerModeNameToInteger(mode);
		if (sanction = G_CheckIPSanctionMatchParam1(ent, SANCTION_MODERESTRICTION, modeNum)) {
			int				time = trap_RealTime(NULL);
			trap_SendServerCommand(ent - g_entities, va("print \"You are currently not allowed to enter this mode. Reason: %s. Restriction expires in %d seconds\n\"",sanction->reason,sanction->expires-time));
			return;
		}
	}
	if (modeNum == -1) {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid mode specified. Valid modes: reset, normal, defrag, duel, duelqueue, allforce, ironman\n\"");
		return;
	}

	SetClientMode(ent, modeNum);

}

