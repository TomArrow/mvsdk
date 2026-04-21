// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include "g_defrag.h"
#include "g_dbcmds.h"

#include "mvsdk_setup.h"

level_locals_t	level;

gentity_t		g_entities[MAX_ENTITIESTOTAL];
gentity_t*		g_entitiesHashTable[ENTITY_HASH_SIZE];
int				g_entitiesHashTableCount = 0;
gentity_t*		g_logicalents = &g_entities[MAX_GENTITIES]; // Quicker access xD
gclient_t		g_clients[MAX_CLIENTS];

mvsharedEntity_t	mv_entities[MAX_GENTITIES];
mvclientSession_t	mv_clientSessions[MAX_CLIENTS];

qboolean gDuelExit = qfalse;

vmCvar_t	g_trueJedi;

// defrag breaking change version number. just so that a client watching/analyzing a demo can find out.
// version 0 (non existent cvar): up to 2025-02-04
// version 1: 2025-02-04 antiloop upgrade to fix wallstrafe exploit
vmCvar_t	g_dfv;

vmCvar_t	g_gametype;
vmCvar_t	g_MaxHolocronCarry;
vmCvar_t	g_ff_objectives;
vmCvar_t	g_autoMapCycle;
vmCvar_t	g_dmflags;
vmCvar_t	g_maxForceRank;
vmCvar_t	g_forceBasedTeams;
vmCvar_t	g_forceBadSpec; // force ppl to spec if their force settings are bad. default in vanilla, but default 0 here.
vmCvar_t	g_connectSpecAlways; // always connect as spec. if we dont force ppl spec for bad forces, we need this in non >= GT_TEAM gamemodes to stay in spec when connecting. cleaner anyway.
vmCvar_t	g_privateDuel;
vmCvar_t	g_saberLocking;
vmCvar_t	g_saberLockFactor;
vmCvar_t	g_saberTraceSaberFirst;

vmCvar_t	g_modes;
vmCvar_t	g_modesDefault;
vmCvar_t	g_defrag;
vmCvar_t	g_defragLastRunId;
vmCvar_t	g_defragLastDemoId;
vmCvar_t	g_defragAutoDemo;
vmCvar_t	g_defragKillSafetyMinSecs;
vmCvar_t	g_defragSimpleResetSpawn; // /resetspawn does all
vmCvar_t	g_triggersRobust;
vmCvar_t	g_bubbleSpawn;
vmCvar_t	g_reuseCTFSpawns;
vmCvar_t	g_defragForceRegenFps;
vmCvar_t	g_defragArenaAutoGen;
vmCvar_t	g_specAllEnts;
vmCvar_t	g_sv_specAllEnts;
vmCvar_t	g_snapPlayerPosAngles;

vmCvar_t	g_arenaAutoGen;


#ifdef G2_COLLISION_ENABLED
vmCvar_t	g_saberGhoul2Collision;
#endif
vmCvar_t	g_saberAlwaysBoxTrace;
vmCvar_t	g_saberBoxTraceSize;

vmCvar_t	g_logClientInfo;

vmCvar_t	g_slowmoDuelEnd;

vmCvar_t	g_saberDamageScale;

vmCvar_t	g_useWhileThrowing;

vmCvar_t	g_alwaysAllowTeamChat;
vmCvar_t	g_forceRegenTime;
vmCvar_t	g_spawnInvulnerability;
vmCvar_t	g_forcePowerDisable;
vmCvar_t	g_weaponDisable;
vmCvar_t	g_duelWeaponDisable;
vmCvar_t	g_duelTimeout;
vmCvar_t	g_duelQueueTimeout;
vmCvar_t	g_duelQueueAutoRespawn;
vmCvar_t	g_duelSeverDistance; // vanilla is 1024 but its kinda weird. TODO do an afk sever? but the afk guy can still be killed so its whatever
vmCvar_t	g_allowDuelSuicide;
vmCvar_t	g_fraglimitVoteCorrection;
vmCvar_t	g_fraglimit;
vmCvar_t	g_duel_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_saberInterpolate;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_friendlyForce;
vmCvar_t	g_friendlySaber;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_startWeaponAlwaysSaber;
vmCvar_t	g_afkCmdMinSecs;
vmCvar_t	g_inactivity;
vmCvar_t	g_inactivityToSpec;
vmCvar_t	g_inactivityToSpecRacers;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_debugFancy;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_adaptRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_intermissionReadyDuration;
vmCvar_t	g_intermissionReadyCheck;

vmCvar_t	g_mapDefaultMsec;
vmCvar_t	g_mapDefaultJump;
vmCvar_t	g_mapDefaultRunFlags;
vmCvar_t	g_q2trace;
vmCvar_t	g_q2Skims;

vmCvar_t	g_strafebotSlopeHandling;

vmCvar_t	g_autoScoresInterval;
vmCvar_t	g_printTFFAStats;

vmCvar_t	g_printJoins;

vmCvar_t	g_scorePenaltySuicide;
vmCvar_t	g_scorePenaltySuicideDuel;
vmCvar_t	g_scorePenaltyTeamKill;
vmCvar_t	g_tffaAnyDeathIsEnemyScore;
vmCvar_t	g_stackFirstSpawn; // 125/200 + bacta on first spawn, e.g. after map_restart

vmCvar_t	g_antiWallhack;
vmCvar_t	g_antiWallhackFast;					// 1= bad attempt. 2= engine side hulltrace(fast! and ignores glass in a proper map compile)
vmCvar_t	g_antiWallhackEnforceVis;			// if a player is visible, force him to be sent to those who can see him
vmCvar_t	g_antiWallhackBoxSize;				// size of box around player to check if he is visible. 9 points being checked.
vmCvar_t	g_antiWallhackRecalcOffset;			// only recalc the box around a player once he moves further than this (cuz it involves 9 traces)
vmCvar_t	g_antiWallhackVisibleRecalcDelay;	// if a player is visible, amount of milliseconds to wait before recalc (its no harm if a player stays networked for 0.15s after going behind a corner?)
vmCvar_t	g_antiWallhackViewerBoxSize;		// if not 0, we make a "box" around the viewer too instead of looking at the actual camera pos. more traces, but we can debounce it as well and completely eliminate a lot of traces

vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_log;
vmCvar_t	g_logSync;
vmCvar_t	g_statLog;
vmCvar_t	g_statLogFile;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_allowVoteShuffle;
vmCvar_t	g_slowVote;
vmCvar_t	g_slowVoteAFKThreshold;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_developer;
vmCvar_t	g_debugCommandsEnable;
vmCvar_t	g_debugForward;
vmCvar_t	g_debugRight;
vmCvar_t	g_debugUp;
vmCvar_t	g_smoothClients;
vmCvar_t	g_pmove_fixed;
vmCvar_t	g_pmove_msec;
vmCvar_t	g_pmove_float;
vmCvar_t	g_ttFlags;
vmCvar_t	g_ttFlagsGp;
vmCvar_t	g_fixHighFPSAbuse;
vmCvar_t	g_entHUDFields;
vmCvar_t	g_rankings;
vmCvar_t	g_listEntity;
vmCvar_t	g_redteam;
vmCvar_t	g_blueteam;
vmCvar_t	g_singlePlayer;
vmCvar_t	g_enableDust;
vmCvar_t	g_enableBreath;
vmCvar_t	g_dismember;
vmCvar_t	g_forceDodge;
vmCvar_t	g_timeouttospec;
vmCvar_t	g_timescale;
vmCvar_t	g_sv_fps;
vmCvar_t	g_sv_gameFps;
vmCvar_t	g_sv_gameFpsAllowIrregular;
vmCvar_t	g_cm_checksumBsp;
vmCvar_t	g_cm_checksumPak;

vmCvar_t	g_fpsToggleDelay;

vmCvar_t	g_allowNameDupes;

vmCvar_t	g_saberDmgVelocityScale;
vmCvar_t	g_saberDmgDelay_Idle;
vmCvar_t	g_saberDmgDelay_Wound;

vmCvar_t	g_saberDebugPrint;

vmCvar_t	g_austrian;

vmCvar_t	g_pushItems; // thanks to: TriForce's JediKnightPlus jk_itemForcePhysics and the original DS-Online g_dsPushItems (https://github.com/TriForceX/JediKnightPlus)

vmCvar_t	g_debugMelee;

vmCvar_t	g_gamename;
vmCvar_t	g_gamedate;

// Fixes and multiversion cvars
vmCvar_t	g_mv_fixgalaking;
vmCvar_t	g_mv_fixbrokenmodels;
vmCvar_t	g_mv_blockchargejump;
vmCvar_t	g_mv_blockspeedhack;
vmCvar_t	g_mv_fixturretcrash;
vmCvar_t	g_connectionlimit;
vmCvar_t	g_connectinglimit;
vmCvar_t	g_mv_forcePowerDisableMode;

// New cvars
vmCvar_t	g_submodelWorkaround;
vmCvar_t	g_botTeamAutoBalance;

vmCvar_t	g_MVSDK;

vmCvar_t	g_userCmdBuffer;
vmCvar_t	g_userCmdBufferSmoothen;
vmCvar_t	g_blockIdenticalUserSnaps;
vmCvar_t	g_blockIdenticalUserSnapsMinFps;

vmCvar_t	g_randomTipInterval;

vmCvar_t	g_unlockRandom;
vmCvar_t	g_mineSwitchFix;

vmCvar_t	g_kickoffFix;

vmCvar_t	g_crossServerChat;
vmCvar_t	g_crossServerDefragTimes;



// vvv-serverSide features port
vmCvar_t	g_pauseGame;
vmCvar_t	g_minefix;
vmCvar_t	g_pauseTimerFreeze;
vmCvar_t	g_allowChatPause;
vmCvar_t	g_analyzebs;
vmCvar_t	g_logbs;
vmCvar_t	g_voteAsSpec; // this is probably broken. cuz level.numVotingClients won't change for it... so fix at some point
vmCvar_t	g_debugFps; 
vmCvar_t	g_fairFlag; 
vmCvar_t	g_moverfix;
vmCvar_t	g_ctfPersStats;



int gDuelist1 = -1;
int gDuelist2 = -1;

int gRandomUnlockAdd = 0;

static void	G_BitMaskCvarUpdated(cvarTable_t* cvar);
// bk001129 - made static to avoid aliasing
/* static */cvarTable_t		gameCvarTable[] = {

	//must be at the start so that its already registered when other cvars are evaluated that affect it
	{ &g_ttFlags, "ttFlags", "15", CVAR_SERVERINFO | CVAR_ROM, 0, qtrue }, // to communicate special tommyternal server features to the client. value 7 means: (va("%d",TTFLAGSSERVERINFO_HASANTILOOPSTATS|TTFLAGSSERVERINFO_HASFORCESPEEDSMASH|TTFLAGSSERVERINFO_HASFORCEJUMPCHARGE|TTFLAGSSERVERINFO_HASCROSSSERVERCHAT))
	{ &g_ttFlagsGp, "ttFlagsGp", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse }, // gameplay ttflags. 

	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ &g_gamename, "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ &g_gamedate, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_dfv, "dfv", QUOTE(SEMIBREAKINGCHANGEVERSIONDEFRAG), CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  }, // we just wanna let the client know
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_MaxHolocronCarry, "g_MaxHolocronCarry", "3", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_intermissionReadyDuration, "g_intermissionReadyDuration", "10000", CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_intermissionReadyCheck, "g_intermissionReadyCheck", "1", CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_ff_objectives, "g_ff_objectives", "0", /*CVAR_SERVERINFO |*/  CVAR_NORESTART, 0, qtrue },

	{ &g_trueJedi, "g_jediVmerc", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qtrue },

	{ &g_autoMapCycle, "g_autoMapCycle", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	
	{ &g_maxForceRank, "g_maxForceRank", "500", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_forceBasedTeams, "g_forceBasedTeams", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_connectSpecAlways, "g_connectSpecAlways", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_forceBadSpec, "g_forceBadSpec", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_privateDuel, "g_privateDuel", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberLocking, "g_saberLocking", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberLockFactor, "g_saberLockFactor", "6", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberTraceSaberFirst, "g_saberTraceSaberFirst", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_modes, "g_modes", "1", CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_modesDefault, "g_modesDefault", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  }, // default value from playerModes_e enum
	{ &g_defrag, "g_defrag", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_defragAutoDemo, "g_defragAutoDemo", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_defragKillSafetyMinSecs, "g_defragKillSafetyMinSecs", "240", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_defragSimpleResetSpawn, "g_defragSimpleResetSpawn", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_defragLastRunId, "g_defragLastRunId", "0", CVAR_ROM | CVAR_NORESTART, 0, qfalse  },
	{ &g_defragLastDemoId, "g_defragLastDemoId", "0", CVAR_ROM | CVAR_NORESTART, 0, qfalse  },
	{ &g_triggersRobust, "g_triggersRobust", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_bubbleSpawn, "g_bubbleSpawn", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_reuseCTFSpawns, "g_reuseCTFSpawns", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_defragForceRegenFps, "g_defragForceRegenFps", "100", CVAR_ARCHIVE | CVAR_CHEAT, 0, qtrue  },
	{ &g_defragArenaAutoGen, "g_defragArenaAutoGen", "1", CVAR_ARCHIVE, 0, qfalse  }, // auto generate .arena files when a course is finished running, if none exists
	{ &g_arenaAutoGen, "g_arenaAutoGen", "0", CVAR_ARCHIVE, 0, qfalse  }, // auto generate .arena file upon successful spawn in map, if none exists
	{ &g_specAllEnts, "g_specAllEnts", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse  }, // mod-side cheaper version of sv_specallents. will only work for free floating spectators
	{ &g_sv_specAllEnts, "sv_specAllEnts", "", 0, 0, qfalse  }, // inform us of sv_specAllEnts if exists.
	{ &g_snapPlayerPosAngles, "g_snapPlayerPosAngles", "0", CVAR_ARCHIVE, 0, qfalse  }, // serverside pos/angle snapping

#ifdef G2_COLLISION_ENABLED
	{ &g_saberGhoul2Collision, "g_saberGhoul2Collision", "0", 0, 0, qtrue  },
#endif
	{ &g_saberAlwaysBoxTrace, "g_saberAlwaysBoxTrace", "0", 0, 0, qtrue  },
	{ &g_saberBoxTraceSize, "g_saberBoxTraceSize", "2", 0, 0, qtrue  },

	{ &g_logClientInfo, "g_logClientInfo", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_slowmoDuelEnd, "g_slowmoDuelEnd", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_saberDamageScale, "g_saberDamageScale", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_useWhileThrowing, "g_useWhileThrowing", "1", 0, 0, qtrue  },

	{ &g_alwaysAllowTeamChat, "g_alwaysAllowTeamChat", "1", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_forceRegenTime, "g_forceRegenTime", "200", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

	{ &g_spawnInvulnerability, "g_spawnInvulnerability", "3000", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_forcePowerDisable, "g_forcePowerDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_weaponDisable, "g_weaponDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_duelWeaponDisable, "g_duelWeaponDisable", "1", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_duelTimeout, "g_duelTimeout", "10000", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_duelQueueTimeout, "g_duelQueueTimeout", "3000", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_duelQueueAutoRespawn, "g_duelQueueAutoRespawn", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_duelSeverDistance, "g_duelSeverDistance", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_allowDuelSuicide, "g_allowDuelSuicide", "0", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimitVoteCorrection, "g_fraglimitVoteCorrection", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_duel_fraglimit, "duel_fraglimit", "10", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_autoScoresInterval, "g_autoScoresInterval", "10", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_printTFFAStats, "g_printTFFAStats", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_printJoins, "g_printJoins", "2", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_scorePenaltySuicide, "g_scorePenaltySuicide", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_scorePenaltySuicideDuel, "g_scorePenaltySuicideDuel", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_scorePenaltyTeamKill, "g_scorePenaltyTeamKill", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_tffaAnyDeathIsEnemyScore, "g_tffaAnyDeathIsEnemyScore", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_stackFirstSpawn, "g_stackFirstSpawn", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_antiWallhack, "g_antiWallhack", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackFast, "g_antiWallhackFast", "3", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackEnforceVis, "g_antiWallhackEnforceVis", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackBoxSize, "g_antiWallhackBoxSize", "40", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackRecalcOffset, "g_antiWallhackMoveTolerance", "15", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackVisibleRecalcDelay, "g_antiWallhackVisibleRecalcDelay", "150", CVAR_ARCHIVE, 0, qtrue },
	{ &g_antiWallhackViewerBoxSize, "g_antiWallhackViewerBoxSize", "80", CVAR_ARCHIVE, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_pauseGame, PAUSEGAME_CVARNAME, "0", CVAR_VVV|CVAR_SYSTEMINFO, 0, qtrue, qfalse, "Pauses the game, preventing players from moving, items from respawning, etc." },
	{ &g_minefix, "g_minefix", "1", CVAR_VVV | CVAR_ARCHIVE, 0, qtrue, qfalse, "This setting is a fix to the behavior where mines that are dropped by a player always will have an ammo count of 3. There are several values:\n"
		"1 : ammo count will always be the true amount, no matter if the player suicided or was killed\n"
		"2 : ammo count will only be the true amount if the player who dropped them suicided\n"
		"In addition, using a value of -1 or -2 will work the same way as 1 and 2, however, the ammo count will be a maximum of 3."
	},
	{ &g_allowChatPause, "g_allowChatPause", "0", CVAR_VVV | CVAR_ARCHIVE, 0, qfalse, qfalse, "Players not on spectator team can pause/unpause the game by using !pause and !unpause in chat." },
	{ &g_pauseTimerFreeze, "g_pauseTimerFreeze", "0", CVAR_VVV | CVAR_ARCHIVE, 0, qfalse, qfalse, "Restores the game timer during pause on every second, effectively freezing it." }, 
	{ &g_fairFlag, "g_fairflag", "1", CVAR_VVV | CVAR_ARCHIVE, 0, qfalse, qfalse, "If the setting is enabled: in situations where more than one player is standing/touching a ctf flag, checks will be made to ensure that the guy standing closest to it will get/cap it, as an alternative to randomness deciding who should get it." }, 
	{ &g_moverfix, "g_moverfix", "1", CVAR_VVV | CVAR_ARCHIVE/* |CVAR_SERVERINFO */, 0, qfalse, qfalse, "If set, dead bodies will not block side doors in ctf_yavin." },
	{ &g_ctfPersStats, "g_ctfPersStats", "2", CVAR_VVV | CVAR_LATCH | CVAR_ARCHIVE | CVAR_SYSTEMINFO/* |CVAR_SERVERINFO */, 0, qfalse, qfalse, "If set, PERS_IMPRESSIVE_COUNT is used for flag returns in CTF/CTY. If value is 2, PERS_EXCELLENT_COUNT and PERS_GAUNTLET_FRAG_COUNT are used for flag grabs and flag hold time in CTF/CTY." }, // reason for value 2 is that it then overrides some other stats in PERS_EXCELLENT_COUNT and PERS_GAUNTLET_FRAG_COUNT (which are arguably useless but whatever)


	{ &g_mapDefaultMsec, "g_mapDefaultMsec", "8", CVAR_SYSTEMINFO|CVAR_ROM, 0, qfalse  },
	{ &g_mapDefaultJump, "g_mapDefaultJump", "1", CVAR_SYSTEMINFO|CVAR_ROM, 0, qfalse  },
	{ &g_mapDefaultRunFlags, "g_mapDefaultRunFlags", "0", CVAR_SYSTEMINFO | CVAR_ROM, 0, qfalse},
	{ &g_q2trace, "g_q2trace", "1", CVAR_SYSTEMINFO, 0, qtrue},
	{ &g_q2Skims, "g_q2Skims", "0", CVAR_SYSTEMINFO, 0, qtrue},

	{ &g_strafebotSlopeHandling, "g_strafebotSlopeHandling", "1", CVAR_SYSTEMINFO | CVAR_CHEAT, 0, qfalse},

#ifdef ANALYZE_BS
	{ &g_analyzebs, "g_analyzebs", "0", CVAR_ARCHIVE | CVAR_VVV, 0, qfalse, qfalse, "Analyze d/bs events of players" },
	{ &g_logbs, "g_logbs", "0", CVAR_ARCHIVE | CVAR_VVV, 0, qfalse, qfalse, "Log all d/bs events to disk (needs g_analyzebs 1)" },
#endif
#ifdef DEBUGFPS
	{ &g_debugFps, "g_debugFps", "0", CVAR_TEMP | CVAR_VVV, 0, qfalse, qfalse, "Collect server FPS stats for debugging (temporary cvar)" },
#endif

	{ &g_voteAsSpec, "g_voteAsSpec", "0", CVAR_ARCHIVE | CVAR_VVV, 0, qfalse, qfalse, "Allow voting for spectators (Buggy, don't use)" },

	{ &g_saberInterpolate, "g_saberInterpolate", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlyForce, "g_friendlyForce", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlySaber, "g_friendlySaber", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", 0, 0, qtrue  },
	{ &g_log, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_statLog, "g_statLog", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_statLogFile, "g_statLogFile", "statlog.log", CVAR_ARCHIVE, 0, qfalse },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_randomTipInterval, "g_randomTipInterval", "600", CVAR_ARCHIVE, 0, qfalse  },


	{ &g_speed, "g_speed", "250", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "5", 0, 0, qtrue },
	{ &g_adaptRespawn, "g_adaptrespawn", "1", CVAR_ARCHIVE, 0, qtrue  },		// Make weapons respawn faster with a lot of players.
	{ &g_forcerespawn, "g_forcerespawn", "60", 0, 0, qtrue },		// One minute force respawn.  Give a player enough time to reallocate force.
	{ &g_startWeaponAlwaysSaber, "g_startWeaponAlwaysSaber", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_afkCmdMinSecs, "g_afkCmdMinSecs", "30", CVAR_ARCHIVE, 0, qfalse },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_inactivityToSpec, "g_inactivityToSpec", "300", CVAR_ARCHIVE, 0, qtrue },
	{ &g_inactivityToSpecRacers, "g_inactivityToSpecRacers", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_debugFancy, "g_debugFancy", "0", CVAR_TEMP, 0, qtrue },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_allowVoteShuffle, "g_allowVoteShuffle", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_slowVote, "g_slowVote", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_slowVoteAFKThreshold, "g_slowVoteAFKThreshold", "300", CVAR_ARCHIVE, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

	{ &g_developer, "developer", "0", 0, 0, qfalse },
	{ &g_debugCommandsEnable, "g_debugCommandsEnable", "0", CVAR_CHEAT, 0, qtrue },

#if 0
	{ &g_debugForward, "g_debugForward", "0", 0, 0, qfalse },
	{ &g_debugRight, "g_debugRight", "0", 0, 0, qfalse },
	{ &g_debugUp, "g_debugUp", "0", 0, 0, qfalse },
#endif

	{ &g_redteam, "g_redteam", "Empire", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue },
	{ &g_blueteam, "g_blueteam", "Rebellion", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue  },
	{ &g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse, qfalse  },

	{ &g_enableDust, "g_enableDust", "0", 0, 0, qtrue, qfalse },
	{ &g_enableBreath, "g_enableBreath", "0", 0, 0, qtrue, qfalse },
	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &g_pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qtrue},
	{ &g_pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qtrue},
	{ &g_pmove_float, "pmove_float", "0", CVAR_SYSTEMINFO, 0, qtrue},
	{ &g_fixHighFPSAbuse, "g_fixHighFPSAbuse", "0", CVAR_SYSTEMINFO, 0, qtrue},
	{ &g_entHUDFields, "g_entHUDFields", "1", CVAR_SYSTEMINFO|CVAR_ARCHIVE, 0, qtrue},

	{ &g_unlockRandom, "g_unlockRandom", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qtrue },
	{ &g_mineSwitchFix, "g_mineSwitchFix", "0", CVAR_ARCHIVE, 0, qtrue, qfalse, NULL, { G_BitMaskCvarUpdated, (void*)&g_ttFlagsGp, "ttFlagsGp", TTFLAGS_GAMEPLAY_SERVERINFO_MINESWITCHFIX} },
	{ &g_kickoffFix, "g_kickoffFix", "1", CVAR_ARCHIVE, 0, qtrue, qfalse, NULL, { G_BitMaskCvarUpdated, (void*)&g_ttFlagsGp, "ttFlagsGp", TTFLAGS_GAMEPLAY_SERVERINFO_MINESWITCHFIX} },

	{ &g_crossServerChat, "g_crossServerChat", "2", CVAR_ARCHIVE, 0, qtrue}, // 1 = receive. 2 = need special say_cross cmd to allow sharing. 3 = share all
	{ &g_crossServerDefragTimes, "g_crossServerDefragTimes", "2", CVAR_ARCHIVE, 0, qtrue}, // Share achieved defrag time prints across servers. 1 = receive. 2 = send

	{ &g_rankings, "g_rankings", "0", 0, 0, qfalse},

	{ &g_dismember, "g_dismember", "100", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_forceDodge, "g_forceDodge", "1", 0, 0, qtrue  },

	{ &g_timeouttospec, "g_timeouttospec", "70", CVAR_ARCHIVE, 0, qfalse },
	{ &g_timescale, "timescale", "", 0, 0, qtrue },
	{ &g_sv_fps, "sv_fps", "100", 0, 0, qtrue },
	{ &g_sv_gameFps, "sv_gameFps", "100", 0, 0, qfalse },
	{ &g_sv_gameFpsAllowIrregular, "sv_gameFpsAllowIrregular", "0", 0, 0, qfalse },
	{ &g_cm_checksumBsp, "cm_checksumBsp", "0", 0, 0, qfalse },
	{ &g_cm_checksumPak, "cm_checksumPak", "0", 0, 0, qfalse },

	{ &g_fpsToggleDelay, "g_fpsToggleDelay", "0", CVAR_ARCHIVE, 0, qfalse }, // e.g. set to 300 for 300 second (5 minute) delay between allowed com_physicsFps changes by the client

	{ &g_allowNameDupes, "g_allowNameDupes", "0", CVAR_ARCHIVE, 0, qfalse },

	{ &g_saberDmgVelocityScale, "g_saberDmgVelocityScale", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Idle, "g_saberDmgDelay_Idle", "350", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Wound, "g_saberDmgDelay_Wound", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_saberDebugPrint, "g_saberDebugPrint", "0", CVAR_CHEAT, 0, qfalse  },

	{ &g_austrian, "g_austrian", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_pushItems, "g_pushItems", "0", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_debugMelee, "g_debugMelee", "1", CVAR_SERVERINFO, 0, qtrue  }, // jka wallgrab related

	{ &g_mv_fixgalaking, "mv_fixgalaking", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_mv_fixbrokenmodels, "mv_fixbrokenmodels", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_mv_blockchargejump, "mv_blockchargejump", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_mv_blockspeedhack, "mv_blockspeedhack", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_mv_fixturretcrash, "mv_fixturretcrash", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_connectinglimit, "g_connectinglimit", "3", CVAR_ARCHIVE, 0, qfalse },
	{ &g_connectionlimit, "g_connectionlimit", "0", CVAR_ARCHIVE, 0, qfalse },

	// The 1.02 client doesn't show the force selection menu if ANY force power is disabled. And the basejk 1.02 server sets saber attack to level 3,
	// saber defense to level 3 and force jump to level 1, if ANY force power is disabled. This leads to some differences in g_forcePowerDisable
	// behaviour between 1.02 and 1.03+. For instance disabling all powers except jump, saber attack and saber defense on 1.02 behaves the same as
	// disabling all force powers on 1.03+. To stay compatible with existing configs mvsdk is going to handle g_forcePowerDisable like 1.02, when
	// loaded in 1.02 mode and mv_forcePowerDisableMode is set. By disabling this cvar 1.03+ behaviour is going to be enabled in 1.02 mode, but
	// base clients might have disadvantages, cause their ui doesn't allow them to assign force jump points, even when force jump is enabled.
	// This cvar only has an effect when the startversion is 1.02.
	{ &g_mv_forcePowerDisableMode, "mv_forcePowerDisableMode", "1", CVAR_ARCHIVE, 0, qfalse },

	// g_submodelWorkaround was technically just setting a flag for mvsdk clients to apply the clientside workaround
	// The cvar might seem more appropriate on cgame, but defaulting it to "0" on the client would make the workaround hardly usable
	// and defaulting it to "1" on the the client might lead to mappers not realising they exceeded basejk limits when testing their maps.
	// So we have a cvar in the game module to let servers enable the clientside workaround for bigger maps, defaulting to "0".
	// Clients supporting the workaround are going to inform the server about it in their userinfo, no matter what this cvar is set to.
	// By now g_submodelWorkaround also supports the value "2", which leads to the modelindex of submodel entities being copied to their
	// time2 fields. This way we can use more than 8 bit for the modelindex. The time2 modelindex is only useful in combination with an
	// engine that supports more than the default 256 submodels.
	{ &g_submodelWorkaround, "g_submodelWorkaround", "0", CVAR_ARCHIVE, 0, qtrue },

	// Bots reset their teams on map_restart and map change on basejk. This is often undesired, so let the host decide.
	{ &g_botTeamAutoBalance, "g_botTeamAutoBalance", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_userCmdBuffer, "g_userCmdBuffer", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_userCmdBufferSmoothen, "g_userCmdBufferSmoothen", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_blockIdenticalUserSnaps, "g_blockIdenticalUserSnaps", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_blockIdenticalUserSnapsMinFps, "g_blockIdenticalUserSnapsMinFps", "30", CVAR_ARCHIVE, 0, qtrue },

	{ &g_MVSDK, "g_MVSDK", MVSDK_VERSION, CVAR_ROM | CVAR_SERVERINFO, 0, qfalse },

};

// bk001129 - made static to avoid aliasing
/* static */int gameCvarTableSize = sizeof(gameCvarTable) / sizeof(gameCvarTable[0]);


void G_InitGame					( int levelTime, int randomSeed, int restart );
void G_RunFrame					( int levelTime );
void G_RunFrameSpectators		( int levelTime );
void G_ShutdownGame				( int restart );
void CheckExitRules				( void );
void G_ROFF_NotetrackCallback	( gentity_t *cent, const char *notetrack);


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
int mvapi = 0;
qboolean mvStructConversionDisabled = qfalse;

int coolApi = 0;
int coolApi_dbVersion = 0;
int coolApi_jkaVersion = 0;
int coolApi_userCmdVersion = 0;
int coolApi_supportedVMFeatures = 0;
vmCvar_t coolApi_supported_game_userCmdStoreVersion;
const int coolApi_supported_game_userCmdStoreVersion_int = 1;
vmCvar_t coolApi_supported_game;
const int coolApi_supported_game_int =
  COOL_APIFEATURE_SETPREDICTEDMOVEMENT
| COOL_APIFEATURE_GETTEMPORARYUSERCMD
| COOL_APIFEATURE_EZDEMOCGAMEBUFFER
| COOL_APIFEATURE_GETTIMESINCESNAPRECEIVED
| COOL_APIFEATURE_MARIADB
| COOL_APIFEATURE_MVAPI_PLAYERSNAPSHOT_SNEAKPEEK
| COOL_APIFEATURE_G_SETBRUSHMODELCONTENTFLAGS
| COOL_APIFEATURE_G_USERCMDSTORE
| COOL_APIFEATURE_RESOLUTIONCHANGED
| COOL_APIFEATURE_NONEPSILONTRACE
| COOL_APIFEATURE_GAME_VMCALL_PHYSICSFPSUPDATE
| COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS
| COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED
| COOL_APIFEATURE_VMCUSTOMFLAGS
| COOL_APIFEATURE_KEEPZOMBIE
| COOL_APIFEATURE_CUSTOMEPSILONTRACE
| COOL_APIFEATURE_JEDI_ACADEMY
| COOL_APIFEATURE_CROSS_SERVER_COMMANDS
| COOL_APIFEATURE_G_UPDATESPECTATORS
| COOL_APIFEATURE_BENCHMARKING
| COOL_APIFEATURE_PRETRACE_TRACE
| COOL_APIFEATURE_MVAPI_SUBMODELBYPASS_SNEAKPEEK
| COOL_APIFEATURE_FASTHULLTRACE
;
const int coolApi_supported_game_vmflags_int = COOL_APIFEATURE_VMGAME_FLAG_SEGMENTEDREPLAY | COOL_APIFEATURE_VMGAME_GAME_FIX_TRACECALLS;

int Init_levelTime;
int Init_randomSeed;
int Init_restart;

intptr_t JK2_vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  );
LIBEXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  )
{ // Wrapper for vmMain, to apply version-specifc adjustments at the beginning and the end of every VM_Call without compleltly changing the vmMain function.
	intptr_t retValue;
	static int activeVMCalls = 0;

	if ( !activeVMCalls ) // If we're not using any wrapper functions it can happen that a syscall triggers a VM_Call and we would try to convert data that has been converted already. So we need to keep track of this...
		MV_VersionMagic( qfalse );
	activeVMCalls++;

	retValue = JK2_vmMain( command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 );

	activeVMCalls--;
	if ( !activeVMCalls ) 
		MV_VersionMagic( qtrue );

	return retValue;
}
intptr_t JK2_vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
	int requestedMvApi = 0;
	char coolApiFeaturesBuffer[80];
	switch ( command ) {
	case GAME_INIT:
		trap_Cvar_VariableStringBuffer("cool_apiFeatures", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
		coolApi = atoi(coolApiFeaturesBuffer);
		trap_Cvar_VariableStringBuffer("cool_apiFeatures", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
		coolApi = atoi(coolApiFeaturesBuffer);
		if (coolApi & COOL_APIFEATURE_G_USERCMDSTORE) {
			trap_Cvar_VariableStringBuffer("cool_apiUserCmdStoreVersion", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
			coolApi_userCmdVersion = atoi(coolApiFeaturesBuffer);
		}
		else {
			coolApi_userCmdVersion = 0;
		}
		if (coolApi & COOL_APIFEATURE_MARIADB) {
			trap_Cvar_VariableStringBuffer("cool_apiDBVersion", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
			coolApi_dbVersion = atoi(coolApiFeaturesBuffer);
		}
		else {
			coolApi_dbVersion = 0;
		}
		if (coolApi & COOL_APIFEATURE_JEDI_ACADEMY) {
			trap_Cvar_VariableStringBuffer("cool_apiJKAVersion", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
			coolApi_jkaVersion = atoi(coolApiFeaturesBuffer);
		}
		else {
			coolApi_jkaVersion = 0;
		}

		trap_Cvar_VariableStringBuffer("com_cool_supportedCoolApiVMFeatures", coolApiFeaturesBuffer, sizeof(coolApiFeaturesBuffer));
		coolApi_supportedVMFeatures = atoi(coolApiFeaturesBuffer);

		trap_Cvar_Register(&coolApi_supported_game, "coolApi_supported_game", va("%d", coolApi_supported_game_int), CVAR_ROM);
		trap_Cvar_Register(&coolApi_supported_game_userCmdStoreVersion, "coolApi_supported_game_userCmdStoreVersion", va("%d", coolApi_supported_game_userCmdStoreVersion_int), CVAR_ROM);
		trap_Cvar_Set("coolApi_supported_game", va("%d", coolApi_supported_game_int));
		trap_Cvar_Set("coolApi_supported_game_userCmdStoreVersion", va("%d", coolApi_supported_game_userCmdStoreVersion_int));
		trap_Cvar_Set("coolApi_supported_game_vmflags", va("%d", coolApi_supported_game_vmflags_int));

		requestedMvApi = MVAPI_Init(arg11);
		if ( !requestedMvApi )
		{ // Only call G_InitGame if we haven't got access to the MVAPI. If we can use the MVAPI we delay the Init until the "MVAPI_AFTER_INIT" command is sent. That allows us use the MVAPI in the actual init.
			G_InitGame( arg0, arg1, arg2 );
		}
		else
		{ // Store the values that were meant for G_InitGame to use them later, when MVAPIR_AFTER_INIT is called.
			Init_levelTime = arg0;
			Init_randomSeed = arg1;
			Init_restart = arg2;
		}
		if (mvapi >= 4 || (coolApi & COOL_APIFEATURE_MVAPI_PLAYERSNAPSHOT_SNEAKPEEK) ) {
			trap_MVAPI_EnablePlayerSnapshots(qtrue);
		}
		return requestedMvApi;
	case MVAPI_AFTER_INIT:
		MVAPI_AfterInit();
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_COOL_API_PHYSICSFPSUPDATE:
		if (coolApi & COOL_APIFEATURE_GAME_VMCALL_PHYSICSFPSUPDATE) {
			return (int)ClientPhysicsFpsChanged(arg0);
		}
	case GAME_COOL_API_CROSS_SERVER_COMMAND_RECEIVED:
		if (coolApi & COOL_APIFEATURE_CROSS_SERVER_COMMANDS) {
			return G_CrossServerCommand();
		}
	case GAME_COOL_API_KEEPZOMBIE:
		if (coolApi & COOL_APIFEATURE_KEEPZOMBIE) {
			gentity_t* ent = g_entities + arg0;
			return DF_KeepClientZombie(ent);
		}
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		// TA: Ported anti-flageat from vvv-serverside. This is probably not needed anymore in the jk2mv/mvsdk age because other places in the code have been implemented to fix this. But do it anyway?
		//This one is called after bad people have tried to eat flags and on transition between teams and stuff.
		//However, we are sure this one is called by the engine after the flag eating cmd, so we only need to check for some stuff here and not elsewhere.
		//Basically, if this guy appears to have any flag, return it.
#define FLAGEAT
#ifdef FLAGEAT
		if (arg0 >= 0 && arg0 < MAX_CLIENTS) {
			gentity_t* ent = &g_entities[arg0];
			qboolean naughty = qfalse;

			if (ent && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
				if (ent->client->ps.powerups[PW_REDFLAG]) {
					Team_ReturnFlag(TEAM_RED);
					naughty = qtrue;
				}
				if (ent->client->ps.powerups[PW_BLUEFLAG]) {
					Team_ReturnFlag(TEAM_BLUE);
					naughty = qtrue;
				}
			}

			if (naughty) {
				//we just returned flag instantly in above code, so make sure the flag doesnt drop to the ground in ClientDisconnect when hes kicked.
				ent->client->ps.powerups[PW_BLUEFLAG] = ent->client->ps.powerups[PW_REDFLAG] = 0;

				//G_SecurityLogPrint("Flag eating attempt", ent);
				trap_DropClient(arg0, "was kicked for trying to do very bad thing!");
				return 0;
			}
		}
#endif
		ClientBegin( arg0, qtrue );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		if ((coolApi & COOL_APIFEATURE_G_UPDATESPECTATORS) && arg1 == 1) {
			G_RunFrameSpectators(arg0);
		}
		else {
			G_RunFrame(arg0);
		}
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	case GAME_ROFF_NOTETRACK_CALLBACK:
		G_ROFF_NotetrackCallback( &g_entities[arg0], (const char *)arg1 );
	case GAME_MVAPI_PLAYERSNAPSHOT:
		if (mvapi >= 4 || coolApi & COOL_APIFEATURE_MVAPI_PLAYERSNAPSHOT_SNEAKPEEK) {
			static int lastPlayerSnapshotNum = -1;
			int playerNum = arg0;
			if (playerNum == -1) {
				if (lastPlayerSnapshotNum != -1) {
					PlayerSnapshotRestoreValues();
				}
			}
			else
			{
				gclient_t* client = (g_entities + playerNum)->client;
				int maxMsecDelay = MIN(200,1000/MAX(1,g_blockIdenticalUserSnapsMinFps.integer));
				if (g_blockIdenticalUserSnaps.integer && client && (client->sess.sessionTeam != TEAM_SPECTATOR || !client->anyClientMovedSinceSnapshot) && client->lastSnapshotSentCommandTime == client->ps.commandTime && level.time > client->lastSnapshotSent && (level.time-client->lastSnapshotSent) < maxMsecDelay) {
					// smooth demos a bit by not sending useless repeated packets, unless it would be come TOO stuttery
					// TODO make it able to go even lower fps but then do some more advanced checks to make sure any spawned/despawned item and such is still visible? hook into G_Spawn and G_Free?
					return qfalse;
				}
				PlayerSnapshotHackValues(lastPlayerSnapshotNum == -1, playerNum);
				client->lastSnapshotSentCommandTime = client->ps.commandTime;
				client->lastSnapshotSent = level.time;
				client->anyClientMovedSinceSnapshot = qfalse;
			}
			lastPlayerSnapshotNum = playerNum;
			return qtrue;
		}
		break;
	}


	return -1;
}


static void	G_BitMaskCvarUpdated(cvarTable_t* cvar) {
	vmCvar_t* cvarBase = (vmCvar_t*)cvar->update.pparam1;

	if (!cvarBase) {
		Com_Error(ERR_FATAL, "G_BitMaskCvarUpdated: pparam1 must be a vmCvar_t* pointer");
	}

	if (cvar->vmCvar->integer) {
		cvarBase->integer |= cvar->update.iparam1;
	}
	else {
		cvarBase->integer &= ~cvar->update.iparam1;
	}

	trap_Cvar_Set(cvar->update.cparam1, va("%d", cvarBase->integer));
	trap_Cvar_Update(cvarBase);
}



void QDECL G_Printf(PRINTF_FORMAT_STRING const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

Q_NORETURN void QDECL G_Error(PRINTF_FORMAT_STRING const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

#define GAME_MV_MIN_APILEVEL 1
#define GAME_MV_MIN_VERSION "1.1"
int MVAPI_Init(int apilevel)
{
	if (!trap_Cvar_VariableIntegerValue("mv_apienabled"))
	{
		G_Printf("Game: MVAPI is not supported at all or has been disabled.\n");
		G_Printf("Game: You need at least JK2MV " GAME_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < GAME_MV_MIN_APILEVEL)
	{
		G_Printf("Game: MVAPI level %i not supported.\n", GAME_MV_MIN_APILEVEL);
		G_Printf("Game: You need at least JK2MV " GAME_MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < MV_APILEVEL)
	{
		G_Printf("Game: MVAPI level %i not supported (using level %i instead).\n", MV_APILEVEL, apilevel);
		G_Printf("Game: You need at least JK2MV " MV_MIN_VERSION " to enable all API features.\n");
	}

	mvapi = apilevel;
	if ( mvapi > MV_APILEVEL ) mvapi = MV_APILEVEL;

	G_Printf("Game: Using MVAPI level %i (%i supported).\n", mvapi, apilevel);
	return mvapi;
}

void MVAPI_AfterInit(void)
{
	if ( mvapi >= 3 )
	{ // If the apilevel supports it tell the engine that we're using 1.04 structs etc. internally
		// Get the inital version
		jk2startversion = trap_MVAPI_GetVersion();
		// Set the version to 1.04
		trap_MVAPI_SetVersion( VERSION_1_04 );
		// Get the current version (should always be 1.04)
		jk2version = trap_MVAPI_GetVersion();

		// Set gameplay and version
		MV_SetGameVersion( jk2version, qfalse );
		MV_SetGamePlay( jk2startversion );
	}
	else if ( mvapi >= 2 )
	{ // If the mvapi supports it tell the engine that we are using the post 1.02 structs internally and don't waste any time converting structs
		mvStructConversionDisabled = qtrue;
		trap_MVAPI_DisableStructConversion( mvStructConversionDisabled );
	}

	// Let the engine know we support more than 256 submodels
	if ( mvapi >= 4 || coolApi & COOL_APIFEATURE_MVAPI_SUBMODELBYPASS_SNEAKPEEK ) trap_MVAPI_EnableSubmodelBypass( qtrue );

	// Call G_InitGame now, because we delayed it earilier
	G_InitGame( Init_levelTime, Init_randomSeed, Init_restart );

	// Disable those JK2MV Engine fixes we can take care of in the VM
	trap_MVAPI_ControlFixes( MVFIX_NAMECRASH | MVFIX_FORCECRASH | MVFIX_GALAKING | MVFIX_BROKENMODEL | MVFIX_TURRETCRASH | MVFIX_CHARGEJUMP | MVFIX_SPEEDHACK | MVFIX_SABERSTEALING | MVFIX_PLAYERGHOSTING );

	// Inform JK2MV that we can handle level.time resetting on mapchanges
	if ( mvapi >= 4 ) trap_MVAPI_ResetServerTime( qtrue );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=MAX_CLIENTS, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders( void ) {
#if 0
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f); 
	AddRemap("textures/ctf2/redteam02", string, f); 
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f); 
	AddRemap("textures/ctf2/blueteam02", string, f); 
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
#endif
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags & ~CVAR_CUSTOMMODMASK );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;
		if (cv->update.func)
			cv->update.func(cv);

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if ( strcmp(g_gamename.string, GAMEVERSION) || strcmp(g_gamedate.string, __DATE__) ) {
		// Inform the host about the unexpected change
		G_Printf( S_COLOR_YELLOW "WARNING: The gamename or gamedate changed after mapchange.\n"
		          S_COLOR_YELLOW "         This could indiciate unexpected side-effects due to module updates at runtime.\n"
		          S_COLOR_YELLOW "         You might want to restart the server.\n" );

		trap_Cvar_Set( "gamename", GAMEVERSION );
		trap_Cvar_Set( "gamedate", __DATE__ );
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update(&g_gametype);
	}

	level.warmupModificationCount = g_warmup.modificationCount;

	MV_UpdateSvFlags();
}


void G_CheckCvarChanges() {
	static int lastModCountRandomUnlock = -1;
	if (lastModCountRandomUnlock != g_unlockRandom.modificationCount) {
		gRandomUnlockAdd = g_unlockRandom.integer ? 1 : 0;
		lastModCountRandomUnlock = g_unlockRandom.modificationCount;
	}
}

// A whole lot of stuff must be done when the pauseGame cvar changed value. We check that here.
void G_HandlePauseStateChange(cvarTable_t* cv) {
	int val;
	int k;

	if (cv->vmCvar->integer)
		val = 1;
	else
		val = 0;

	//trap_SendConsoleCommand(EXEC_APPEND, va("g_synchronousClients %d\n", val));

	if (val) {
		gentity_t* ent;
		pauseGameStartTime = level.time;
		trap_SendServerCommand(-1, "print \"Game was paused.\n\"");

		// save clients' viewangles..
		// This is so we can restore them upon unpause. Because a
		// client may move his mouse which will cause sudden
		// viewangle change upon unpause. We prevent that by
		// forcing the viewangle back to the viewangle he had at
		// the moment of pause. 
		for (k = 0, ent = g_entities; k < MAX_CLIENTS; ++k, ++ent) {
			if (ent && ent->client && ent->client->pers.connected != CON_DISCONNECTED) {
				VectorCopy(ent->client->ps.viewangles, ent->client->pauseSavedViewangles);
			}
		}
	}
	else {
		// PAUSE STOPPED
		level.unpauseClient = -1;

		//SAFETY CHECK
		if (pauseGameStartTime > 0 && pauseGameStartTime < level.time) {
			//postpone all think functions
			gentity_t* ent;
			const int pauseDuration = level.time - pauseGameStartTime;

			//postpone think functions that were blocked during pause.
			for (k = MAX_CLIENTS; k < MAX_GENTITIES; ++k) {
				ent = &g_entities[k];
				if (ent && ent->inuse && ent->think && ent->nextthink > 0) {
					ent->nextthink += pauseDuration;
				}
			}

			// G_LogPrintf("Pause stopped after %s.\n", pauseGameStartTime, G_MsToString(pauseDuration));
			trap_SendServerCommand(-1, va("print \"Pause ended after %s.\n\"", G_MsToStringVVV(pauseDuration)));

			if (!g_pauseTimerFreeze.integer) {
				//Roll back the time that cg_drawTimer shows
				level.startTime += pauseDuration;
				trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));
				pauseGameStartTime = 0;
			}

			//fix so times are correct on scoreboard. we dont wanna count time while game is paused
			for (k = 0, ent = g_entities; k < MAX_CLIENTS; ++k, ++ent) {
				if (ent && ent->client && ent->client->pers.connected != CON_DISCONNECTED) {
					ent->client->pers.enterTime += pauseDuration;

					//ok, in case someone joined during pause, ensure they dont get negative time..
					if (ent->client->pers.enterTime > level.time)
						ent->client->pers.enterTime = level.time;


					if (ent->client->pers.teamState.flagsince) {
						//This guy is holding a flag. Update the timer so its not counting pause time.
						ent->client->pers.teamState.flagsince += pauseDuration;

						if (ent->client->pers.teamState.flagsince > level.time)
							ent->client->pers.teamState.flagsince = level.time;
					}
					if (ent->client->pers.teamState.lastreturnedflag) {
						ent->client->pers.teamState.lastreturnedflag += pauseDuration;
					}
					if (ent->client->pers.teamState.lastfraggedcarrier) {
						ent->client->pers.teamState.lastfraggedcarrier += pauseDuration;
					}
					if (ent->client->pers.teamState.lasthurtcarrier) {
						ent->client->pers.teamState.lasthurtcarrier += pauseDuration;
					}

					if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && !ent->client->sess.raceMode) {
						//restore this clients viewangles as the same as before pause.
						DF_PreDeltaAngleChange(ent->client);
						SetClientViewAngle(ent, ent->client->pauseSavedViewangles);
						DF_PostDeltaAngleChange(ent->client, qtrue);
					}

					//Somehow, this causes bugging if someone entered the game during pause (they get infinite invulnerability)
					// if (object->client->invulnerableTimer)
						// object->client->invulnerableTimer += pauseDuration;
				}
			}
		}
	}
}

/*
=================
G_UpdateCvars
=================
*/
int pauseGameStartTime = 0;
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->update.func ) {
					cv->update.func(cv);
				}

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", 
						cv->cvarName, cv->vmCvar->string ) );
				}

				// hack to make smooth pauses
				// Thanks to Daggolin for this tip!
				if (cv->vmCvar == &g_pauseGame) {
					G_HandlePauseStateChange(cv);
				}

#ifdef DEBUGFPS
				else if (cv->vmCvar == &g_sv_fps || cv->vmCvar == &g_sv_gameFps || cv->vmCvar == &g_timescale) {
					FPS_ResetStats();
				}
#endif

				if (cv->teamShader) {
					remapped = qtrue;
				}

				// mvsdk_svFlags
				if ( cv->vmCvar == &g_submodelWorkaround )
					MV_UpdateSvFlags();
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}
}

/*
=================
MV_UpdateMvsdkConfigstring

=================
*/
void MV_UpdateMvsdkConfigstring( char *key, char *value )
{
	char csString[MAX_INFO_STRING];

	trap_GetConfigstring( CS_MVSDK, csString, sizeof(csString) );
	Info_SetValueForKey( csString, key, value );
	trap_SetConfigstring( CS_MVSDK, csString );
}

/*
=================
MV_UpdateSvFlags

Called when registering cvars and updating specific cvars and updates the mvsdk_svFlags according to the current settings
=================
*/
void MV_UpdateSvFlags( void )
{
	// mvsdk_svFlags - Used to inform clients about additional mvsdk serverside-features or the compatibility to clientside-features
	static int lastValue = 0;
	int intValue = 0;

	// Check for the features and determine the flags
	if ( level.bboxEncoding )               intValue |= MVSDK_SVFLAG_BBOX;
	if ( g_submodelWorkaround.integer & 1 ) intValue |= MVSDK_SVFLAG_SUBMODEL_WORKAROUND;
	if ( level.modelindexTime2 )            intValue |= MVSDK_SVFLAG_SUBMODEL_TIME2;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! Forks of MVSDK should NOT modify the mvsdk_svFlags                              !!!
	// !!! Removal, replacement or adding of new flags might lead to incompatibilities     !!!
	// !!! Forks should define their own infostring, but they can send it through CS_MVSDK !!!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Check if we have to update anything or if we can return already
	if ( intValue == lastValue ) return;

	// Update the configstring
	MV_UpdateMvsdkConfigstring( "mvsdk_svFlags", va("%i", intValue) );

	// Remember the old value
	lastValue = intValue;
}

void G_SetupTempDemoSubfolderName();


void InitClanTagHashTable();

void G_InitClientCommands(void);
/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;

	B_InitAlloc(); //make sure everything is clean

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);

	InitFpsTable();
	InitClanTagHashTable();
	
	if ( jk2version == VERSION_UNDEF )
	{ // We don't know the version of the server, yet...
		// JK2MV with api?
		if ( mvapi ) jk2version = trap_MVAPI_GetVersion();

		if ( jk2version == VERSION_UNDEF )
		{
			char version[128];

			trap_Cvar_VariableStringBuffer("version", version, sizeof(version));

			// Not checking for exact strings, as those are different on every build. Instead we check if the version is in the string.
			if ( strstr(version, "JK2MP") )
			{ // Seems to be JK2MP or JK2MV > 1.1
					 if ( strstr(version, "1.02") ) jk2version = VERSION_1_02;
				else if ( strstr(version, "1.03") ) jk2version = VERSION_1_03;
				else if ( strstr(version, "1.04") ) jk2version = VERSION_1_04;
				else
				{
					jk2version = VERSION_1_04;
					G_Printf("MVSDK: Unable to detect jk2mp version, setting to 1.04 compatibility.\n");
				}
			}
			else if ( strstr(version, "JK2MV") )
			{ // Seems to be jk2mv, but an old version, try to find the version by reading the mv_serverversion cvar
				trap_Cvar_VariableStringBuffer("mv_serverversion", version, sizeof(version));
					 if ( !Q_stricmp(version, "1.02") ) jk2version = VERSION_1_02;
				else if ( !Q_stricmp(version, "1.03") ) jk2version = VERSION_1_03;
				else if ( !Q_stricmp(version, "1.04") ) jk2version = VERSION_1_04;
			}
		}

		if ( jk2version == VERSION_UNDEF ) G_Error("MVSDK: Unable to detect jk2version [Game].");
		jk2startversion = jk2version;
		MV_SetGameVersion(jk2version, qtrue);
	}
	G_Printf("jk2version [Game]: 1.0%i\n", jk2startversion);

	srand( randomSeed );
	mysrand( randomSeed ); // On linux rand() behaves different than on Winodws or in a qvm, ...

	G_RegisterCvars();

	DF_CheckRaceCvarChanges(qtrue);

	G_DB_Init();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;
	level.frameTimeMsec = 0;

	G_SetupTempDemoSubfolderName();

	if (g_defrag.integer && g_defragAutoDemo.integer) {
		trap_Cvar_Set("sv_autoDemo", "0"); // disable autodemo if we are doing defrag auto demo. it will interfere when ppl first connect
	}

	memset( &userCmdBuffer, 0, sizeof(userCmdBuffer));

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	//trap_SP_RegisterServer("mp_svgame");

	//for logging d/bs events
#ifdef ANALYZE_BS
	trap_FS_FOpenFile("bsevents.dat", &level.bsLogFile, FS_APPEND_SYNC);
#endif

	if ( g_log.string[0] ) {
		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	G_LogWeaponInit();

	G_InitWorldSession();

	if (g_defrag.integer) {
		AddRemap("gfx/misc/blue_portashield", "gfx/2d/bracket", level.time);
		trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
	}

	AddRemap("gfx/mp/chat_icon", "gfx/mp/dmgshader_shields", level.time); // looks nice
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());

	// initialize all entities for this game
	memset( g_entities, 0, MAX_ENTITIESTOTAL * sizeof(g_entities[0]) );
	level.gentities = g_entities;
	
	// We can initialise this even without the JK2MV API and use it in the VM, but we can only share it with the engine, if the API is available
	memset( &mv_entities, 0, sizeof(mv_entities) );

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	if ( jk2version == VERSION_1_02 && !mvStructConversionDisabled )
	{ // 1.02
		// initialize all clients for this game
		memset( g_ps, 0, MAX_CLIENTS * sizeof(g_ps[0]) );

		trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
			(playerState_t*)&g_ps[0], sizeof( g_ps[0] ) );
	}
	else
	{
		trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
			&level.clients[0].ps, sizeof( level.clients[0] ) );
	}
	
	// Inform the engine about our mv_entities
	if ( mvapi ) trap_MVAPI_LocateGameData( mv_entities, level.num_entities, sizeof( mvsharedEntity_t ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	InitPlayerStats();

	ClearRegisteredItems();

	// initialize saga mode before spawning entities so we know
	// if we should remove any saga-related entities on spawn
	InitSagaMode();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	defaultRaceStyle = getDefaultMapRaceStyle(); // it has 16 bit values so we can't just do the initializer values in a const global :/
	//level.mapDefaultRaceStyle = defaultRaceStyle;
	DF_SetMapDefaults(defaultRaceStyle);
	level.mapDefaultsConfirmed = qfalse;
	level.mapDefaultsLoadFailed = qfalse;
	level.hasArenaInfo = qfalse;
	level.mustGenerateArena = qfalse;
	level.allRaceGenerationAlreadyCalled = qfalse;
	level.arenasLoaded = qfalse;
	if (g_defrag.integer) {
		DF_LoadMapDefaults();
	}
	G_UserMessagesPrune();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( g_gametype.integer >= GT_TEAM ) {
		G_CheckTeamItems();
	}
	else if ( g_gametype.integer == GT_JEDIMASTER )
	{
		trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );
	}

	trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1") );
	trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("-1") );
	gDuelist1 = -1;
	gDuelist2 = -1;

	if (g_modes.integer) {
		// for ironman we want client to be able to display the flag even if the map doesnt have it
		gitem_t* flag = BG_FindItem("team_CTF_blueflag");
		RegisterItem(flag);
		flag = BG_FindItem("team_CTF_redflag");
		RegisterItem(flag);
	}

	SaveRegisteredItems();

	G_Printf ("-----------------------------------\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	G_InitClientCommands();

	if ( g_gametype.integer == GT_TOURNAMENT )
	{
		G_LogPrintf("Duel Tournament Begun: kill limit %d, win limit: %d\n", g_fraglimit.integer, g_duel_fraglimit.integer );
	}
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	G_Printf ("==== ShutdownGame ====\n");

	G_UserMessagesPrune();

	G_LogWeaponOutput();

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	DF_HandleUnfinishedDemos();

	G_CheckEnqueuedClips(qtrue);

#ifdef ANALYZE_BS
	if (level.bsLogFile) {
		trap_FS_FCloseFile(level.bsLogFile);
	}
#endif

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap_ROFF_Clean();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc

	// Some builds of jk2mv don't reset the vm->gameversion on map_restart, so reset the gameversion now
	if ( mvapi >= 3 && jk2version != jk2startversion )
	{
		// Reset the gameversion in the engine
		trap_MVAPI_SetVersion( jk2startversion );

		// If we are not called again it shouldn't matter, but in case some modified engine version decides to do
		// additional vmCalls after the shutdown we want to know what version it expects
		jk2version = trap_MVAPI_GetVersion();
		MV_SetGameVersion( jk2version, qfalse );

		// Replace the gamedata (the g_ps array gets valid data at the end of the vmCall if we returned into 1.02 mode)
		if ( jk2version == VERSION_1_02 && !mvStructConversionDisabled )
		{
			memset( g_ps, 0, MAX_CLIENTS * sizeof(g_ps[0]) );
			trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), (playerState_t*)&g_ps[0], sizeof( g_ps[0] ) );
		}
		else
		{
			trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), &level.clients[0].ps, sizeof( level.clients[0] ) );
		}
	}
}



//===================================================================

Q_NORETURN void QDECL Com_Error ( errorParm_t level, PRINTF_FORMAT_STRING const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( PRINTF_FORMAT_STRING const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	G_Printf ("%s", text);
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
//	if ( level.intermissiontime ) {
//		return;
//	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorOrder > nextInLine->sess.spectatorOrder ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

void RemoveDuelDrawLoser(void)
{
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[ level.sortedClients[0] ].pers.connected != CON_CONNECTED )
	{
		return;
	}
	if ( level.clients[ level.sortedClients[1] ].pers.connected != CON_CONNECTED )
	{
		return;
	}

	clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
	clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		clFailure = 1;
	}
	else if (clSec > clFirst)
	{
		clFailure = 0;
	}
	else
	{
		clFailure = 2;
	}

	if (clFailure != 2)
	{
		SetTeam( &g_entities[ level.sortedClients[clFailure] ], "s" );
	}
	else
	{ //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[ level.sortedClients[1] ], "s" );
	}
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		int clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
		int clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if (clFirst > clSec)
		{
			clFailure = 1;
			clSuccess = 0;
		}
		else if (clSec > clFirst)
		{
			clFailure = 0;
			clSuccess = 1;
		}
		else
		{
			clFailure = 2;
			clSuccess = 2;
		}

		if (clFailure != 2)
		{
			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
		else
		{
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );

			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;
	int raceA, raceB;
	int scoreA, scoreB;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	raceA = ca->pers.raceBestTime ? ca->pers.raceBestTime : INT_MAX;
	raceB = cb->pers.raceBestTime ? cb->pers.raceBestTime : INT_MAX;

	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if (g_gametype.integer != GT_TOURNAMENT) { // spectator order matters in tournaments i guess. racing doesn't rly make sense in duel mode anyway
			if (raceA
				< raceB) {
				return -1;
			}
			if (raceA
				> raceB) {
				return 1;
			}
		}
		if ( ca->sess.spectatorOrder > cb->sess.spectatorOrder ) {
			return -1;
		}
		if ( ca->sess.spectatorOrder < cb->sess.spectatorOrder ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	if (raceA
		< raceB) {
		return -1;
	}
	if (raceA
		> raceB) {
		return 1;
	}

	scoreA = ca->sess.raceMode ? 32767 : ca->ps.persistant[PERS_SCORE];
	scoreB = cb->sess.raceMode ? 32767 : cb->ps.persistant[PERS_SCORE];

	// then sort by score
	if (scoreA
		> scoreB) {
		return -1;
	}
	if (scoreA
		< scoreB) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		raceBestTime;
	int		newScore;
	// int		preNumSpec = 0;
	// int		nonSpecIndex = -1;
	gclient_t	*cl;

	// preNumSpec = level.numNonSpectatorClients;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots
	for ( i = 0; i < /*TEAM_NUM_TEAMS*/2; i++ ) { // TEAM_NUM_TEAMS is 4, numteamVotingClients has a size of [2]
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;
			if (level.clients[i].pers.connected == CON_CONNECTED) {
				level.numFullyConnectedClients++;
			}

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;
				// nonSpecIndex = i;
			
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) && !level.clients[i].markedAsInactive) {
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	if (!g_warmup.integer)
	{
		level.warmupTime = 0;
	}

	/*
	if (level.numNonSpectatorClients == 2 && preNumSpec < 2 && nonSpecIndex != -1 && g_gametype.integer == GT_TOURNAMENT && !level.warmupTime)
	{
		gentity_t *currentWinner = G_GetDuelWinner(&level.clients[nonSpecIndex]);

		if (currentWinner && currentWinner->client)
		{
			G_CenterPrint( -1, 3, va("%s" S_COLOR_WHITE " %s %s\n",
			currentWinner->client->pers.netname, G_GetStripEdString("SVINGAME", "VERSUS"), level.clients[nonSpecIndex].pers.netname));
		}
	}
	*/
	//NOTE: for now not doing this either. May use later if appropriate.

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {	
		int		oldRanks[MAX_CLIENTS];
		rank = -1;
		score = 0;
		raceBestTime = 0;
		if (g_developer.integer > 1) {
			for (i = 0; i < level.numPlayingClients; i++) {
				oldRanks[level.sortedClients[i]] = level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK];
			}
		}
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->sess.raceMode ? 32767 : cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score || cl->pers.raceBestTime != raceBestTime ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			raceBestTime = cl->pers.raceBestTime;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
		if (g_developer.integer > 1) {
			for (i = 0; i < level.numPlayingClients; i++) {
				if (oldRanks[level.sortedClients[i]] != level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK]) {
					G_Printf("^3Rank changed for client %d (score %d, racebesttime %d) from %d (tied %d) to %d (tied %d)\n",
						level.sortedClients[i],
						level.clients[level.sortedClients[i]].ps.persistant[PERS_SCORE],
						level.clients[level.sortedClients[i]].pers.raceBestTime,
						oldRanks[level.sortedClients[i]] &~ RANK_TIED_FLAG,
						!!(oldRanks[level.sortedClients[i]] & RANK_TIED_FLAG),
						level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] &~ RANK_TIED_FLAG,
						!!(level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] & RANK_TIED_FLAG)
						);
				}
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( g_gametype.integer >= GT_TEAM ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}

		if (g_gametype.integer != GT_TOURNAMENT)
		{ //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 )
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0] ) );
			}
			else
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || g_gametype.integer == GT_TOURNAMENT ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendTFFAEndGameStats

Stats screen at the end of a game for team ffa
========================
*/
void SendTFFAEndGameStats() {
	if (g_printTFFAStats.integer) {
		TFFAEndGameStatsMessage(-1, qtrue);
	}
}

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}


	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	if (g_entHUDFields.integer) {
		ent->client->ps.generic1 = ent->s.generic1 = 0;
		ent->client->ps.fd.forceMindtrickTargetIndex3 = ent->s.trickedentindex3 = 0;
		ent->client->ps.fd.forceMindtrickTargetIndex4 = ent->s.trickedentindex4 = 0;
	}
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_FindByClassNameFast(NULL, "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( NULL, vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target, qtrue, NULL );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

		AdjustTournamentScores();
		if (DuelLimitHit())
		{
			gDuelExit = qtrue;
		}
		else
		{
			gDuelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	if (g_singlePlayer.integer) {
		trap_Cvar_Set("ui_singlePlayerActive", "0");
		UpdateTournamentInfo();
	}

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			respawn(client);
		}
		MoveClientToIntermission( client );
	}

	SendTFFAEndGameStats();

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}

/*
==================
Parity

Calculates parity of 1s in binary representation of i
==================
*/
static qboolean Parity(int i)
{
	qboolean parity = qtrue;

	while (i) {
		parity = (qboolean)!parity;
		i &= i - 1;
	}

	return parity;
}

/*
==================
Shuffle

Shuffle players according to score
==================
*/
static void Shuffle(void)
{
	gentity_t* ent;
	int			clientNum;
	team_t		newTeam;
	int			i;

	for (i = 0; i < level.numNonSpectatorClients; i++) {
		clientNum = level.sortedClients[i];
		ent = g_entities + clientNum;
		newTeam = Parity(i) ? TEAM_RED : TEAM_BLUE;

		ent->client->sess.sessionTeam = newTeam;
		ent->client->sess.teamLeader = qfalse;
	}

	CheckTeamLeader(TEAM_RED);
	CheckTeamLeader(TEAM_BLUE);

	for (i = 0; i < level.numNonSpectatorClients; i++) {
		clientNum = level.sortedClients[i];
		ent = g_entities + clientNum;

		respawn(ent);
		ClientUserinfoChanged(clientNum);
		//ClientUpdateConfigString(clientNum);
	}

	CalculateRanks();
}

qboolean DuelLimitHit(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( g_duel_fraglimit.integer && cl->sess.wins >= g_duel_fraglimit.integer )
		{
			return qtrue;
		}
	}

	return qfalse;
}

void DuelResetWinsLosses(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_TOURNAMENT  ) {
		if (!DuelLimitHit())
		{
			if ( !level.restarted ) {
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;	
		}

		DuelResetWinsLosses();
	}


	trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}



typedef struct {
	//8 bytes. can we make it 4?
	byte tm_sec;     /* seconds after the minute - [0,59] */
	byte tm_min;     /* minutes after the hour - [0,59] */
	byte tm_hour;    /* hours since midnight - [0,23] */
	byte tm_mday;    /* day of the month - [1,31] */
	byte tm_mon;     /* months since January - [0,11] */
	byte tm_year;    /* years since 1900 */		//as of 2015, value is 115, so we still have a few years before we hit 127
	byte tm_wday;	 /* days since Sunday - [0,6] */
	byte tm_isdst;	 /* daylight savings time flag */
} smallTime_t;

void QtimeToSmallTime(qtime_t* qt, smallTime_t* st) {
	st->tm_sec = (byte)qt->tm_sec;
	st->tm_min = (byte)qt->tm_min;
	st->tm_hour = (byte)qt->tm_hour;
	st->tm_mday = (byte)qt->tm_mday;
	st->tm_mon = (byte)qt->tm_mon;
	st->tm_year = (byte)qt->tm_year;
	st->tm_wday = (byte)qt->tm_wday;
	st->tm_isdst = (byte)qt->tm_isdst;
}
#ifdef ANALYZE_BS
void QDECL G_LogBsEvent(bsRecord_t* bsr, gentity_t* ent) {
	int 			len;
	qtime_t			time;
	smallTime_t 	smalltime;
	unsigned char	ipb[4];		//bytes
	mvclientSession_t* mvSess = &mv_clientSessions[ent - g_entities];

	if (!level.bsLogFile) {
		return;
	}

	trap_RealTime(&time);
	QtimeToSmallTime(&time, &smalltime);

	ipb[0] = mvSess->clientIP[0];
	ipb[1] = mvSess->clientIP[1];
	ipb[2] = mvSess->clientIP[2];
	ipb[3] = mvSess->clientIP[3];

	//write player IP in 4 bytes
	trap_FS_Write(&ipb, sizeof(ipb), level.bsLogFile);

	//write player name; use len+1 so we write the 0 byte also
	len = strlen(ent->client->pers.netnameClean);
	trap_FS_Write(ent->client->pers.netnameClean, len + 1, level.bsLogFile);

	//write timestamp
	trap_FS_Write(&smalltime, sizeof(smallTime_t), level.bsLogFile);

	//write the BS record data
	trap_FS_Write(bsr, sizeof(bsRecord_t), level.bsLogFile);
}
#endif

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf(PRINTF_FORMAT_STRING const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof(string), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	Q_vsnprintf( string +7, sizeof(string)-7 , fmt,argptr );
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
	qboolean		won = qtrue;

	if (g_defrag.integer) { // no game ending in defrag.
		G_LogPrintf("Exit (stopped due to defrag): %s\n", string);
		return;
	}

	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],	cl->pers.netname );
		if (g_singlePlayer.integer && g_gametype.integer == GT_TOURNAMENT) {
			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
				won = qfalse;
			}
		}
	}

	if (g_singlePlayer.integer) {
		if (g_gametype.integer >= GT_CTF) {
			won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
		}
		trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) { // used cl->ps.clientNum as index here previously. change from vvv-serverside
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	if ( g_gametype.integer == GT_TOURNAMENT && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) )
	{
		gDidDuelStuff = qtrue;

		if ( g_austrian.integer )
		{
			G_LogPrintf("Duel Results:\n");
			//G_LogPrintf("Duel Time: %d\n", level.time );
			G_LogPrintf("winner: %s, score: %d, wins/losses: %d/%d\n", 
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
			G_LogPrintf("loser: %s, score: %d, wins/losses: %d/%d\n", 
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses );
		}
		// if we are running a tournement map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if ( g_austrian.integer )
			{
				G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n", 
					level.clients[level.sortedClients[0]].pers.netname,
					level.clients[level.sortedClients[0]].sess.wins,
					level.clients[level.sortedClients[0]].sess.losses,
					level.clients[level.sortedClients[1]].pers.netname,
					level.clients[level.sortedClients[1]].sess.wins,
					level.clients[level.sortedClients[1]].sess.losses,
					g_fraglimit.integer );
			}
			
			if (level.numPlayingClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

				gDuelist1 = level.sortedClients[0];
				gDuelist2 = level.sortedClients[1];
			}

			return;	
		}

		if ( g_austrian.integer )
		{
			G_LogPrintf("Duel Tournament Winner: %s wins/losses: %d/%d\n", 
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
		}
		//this means we hit the duel limit so reset the wins/losses
		//but still push the loser to the back of the line, and retain the order for
		//the map change
		if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
			level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
			level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
			level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
		{
			RemoveDuelDrawLoser();
		}
		else
		{
			RemoveTournamentLoser();
		}

		AddTournamentPlayer();

		if (level.numPlayingClients >= 2)
		{
			trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

			gDuelist1 = level.sortedClients[0];
			gDuelist2 = level.sortedClients[1];
		}
	}

	if (g_gametype.integer == GT_TOURNAMENT && !gDuelExit)
	{ //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 )
		{ //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for (i=0 ; i< g_maxclients.integer ; i++)
		{ //being in a "ready" state is not necessary here, so clear it for everyone
		  //yes, I also thinking holding this in a ps value uniquely for each player
		  //is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + MIN(g_intermissionReadyDuration.integer,5000) ) {
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready && g_intermissionReadyCheck.integer) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + g_intermissionReadyDuration.integer ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}
	
	if ( g_gametype.integer >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
extern void Svcmd_ResetScores_f(void);
extern void ClientRespawn(gentity_t* ent);
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if (gDoSlowMoDuel)
	{ //don't go to intermission while in slow motion
		return;
	}

	if (gEscaping)
	{
		int i = 0;
		int numLiveClients = 0;

		while (i < MAX_CLIENTS)
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
				{
					numLiveClients++;
				}
			}

			i++;
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			qboolean racer = qfalse;

			level.intermissionQueued = 0;

			if (g_defrag.integer) {
				for (i = 0; i < level.maxclients; i++) {
					if (g_entities[i].inuse && g_entities[i].client && g_entities[i].client->sess.raceMode && (g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR)) {
						racer = qtrue;
						break;
					}
				}
			}

			// from japro. not sure if i even need this or it makes any sense
			if (racer) { //only do this if someoen is in racemode?
				//PrintStats(-1);//JAPRO STATS
				for (i = 0; i < level.maxclients; i++) {
					if (!g_entities[i].inuse || !g_entities[i].client || g_entities[i].client->sess.raceMode || (g_entities[i].client->sess.sessionTeam == TEAM_SPECTATOR))
						continue;
					ClientRespawn(&g_entities[i]);	// respawn if dead... respawn if alive too?
				}
				Svcmd_ResetScores_f();
			}
			else {
				BeginIntermission();
			}
			// uh? should this not NOT be called then?
			BeginIntermission();
		}
		return;
	}

	// check for sudden death
	if ( ScoreIsTied() ) {
		// always wait for sudden death
		if (g_gametype.integer != GT_TOURNAMENT || !g_timelimit.integer)
		{
			return;
		}
	}

	if ( g_timelimit.integer && !level.warmupTime && !g_defrag.integer && !g_pauseGame.integer) {
		if ( level.time - level.startTime >= g_timelimit.integer*60000 ) {
//			trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"");
			trap_SendServerCommand( -1, va("print \"%s.\n\"",G_GetStripEdString("SVINGAME", "TIMELIMIT_HIT")));
			LogExit( "Timelimit hit." );
			return;
		}
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( g_gametype.integer < GT_CTF && g_fraglimit.integer && !g_defrag.integer) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, va("print \"Red %s\n\"", G_GetStripEdString("SVINGAME", "HIT_THE_KILL_LIMIT")) );
			LogExit( "Kill limit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, va("print \"Blue %s\n\"", G_GetStripEdString("SVINGAME", "HIT_THE_KILL_LIMIT")) );
			LogExit( "Kill limit hit." );
			return;
		}

		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( g_gametype.integer == GT_TOURNAMENT && g_duel_fraglimit.integer && cl->sess.wins >= g_duel_fraglimit.integer )
			{
				LogExit( "Duel limit hit." );
				gDuelExit = qtrue;
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname ) );
				return;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Kill limit hit." );
				gDuelExit = qfalse;
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
												cl->pers.netname,
												G_GetStripEdString("SVINGAME", "HIT_THE_KILL_LIMIT")
												) 
										);
				return;
			}
		}
	}

	if ( g_gametype.integer >= GT_CTF && g_capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT ) {

		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 ) {
			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				gDuelist1 = level.sortedClients[0];
				gDuelist2 = level.sortedClients[1];
			}
		}

		if (level.numPlayingClients >= 2)
		{
			if (gDuelist1 == -1 ||
				gDuelist2 == -1)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				gDuelist1 = level.sortedClients[0];
				gDuelist2 = level.sortedClients[1];
				if ( g_austrian.integer )
				{
					G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n", 
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						g_fraglimit.integer );
				}
				//trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{ //don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
#endif
	} else if ( level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( g_gametype.integer > GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}


int G_CalculateVoteExecuteTime() {
	int i;
	gentity_t* ent = g_entities;
	// check if anyone is racing who didn't vote yes
	for (i = 0; i < level.maxclients; i++,ent++) {
		if (ent->inuse && ent->client && ent->client->sess.raceMode && ent->client->pers.raceStartCommandTime && !ent->client->pers.voteValue) {
			trap_SendServerCommand(-1,va("print \"Giving %s (and others?) 60 seconds to finish his race.\n\"",ent->client->pers.netname));
			return 60000;
		}
	}
	return 3000;
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	int requiredClients = g_voteAsSpec.integer ? level.numFullyConnectedClients : level.numVotingClients;
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );

		if (level.votingGametype)
		{
			if (trap_Cvar_VariableIntegerValue("g_gametype") != level.votingGametypeTo)
			{ //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap(level.votingGametypeTo, qtrue);

				if (nextMap && nextMap[0])
				{
					trap_SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
				}

			}
			else
			{ //otherwise, just leave the map until a restart
				G_RefreshNextMap(level.votingGametypeTo, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{ //This means to auto-correct fraglimit when voting to and from duel.
				int currentGT = trap_Cvar_VariableIntegerValue("g_gametype");
				int currentFL = trap_Cvar_VariableIntegerValue("fraglimit");

				if (level.votingGametypeTo == GT_TOURNAMENT && currentGT != GT_TOURNAMENT)
				{
					if (currentFL > 3 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
				}
				else if (level.votingGametypeTo != GT_TOURNAMENT && currentGT == GT_TOURNAMENT)
				{
					if (currentFL && currentFL < 20)
					{ //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if ( !level.voteTime ) {
		return;
	}
	if (level.votingOpinionAll) {
		if (level.time - level.voteTime >= VOTE_TIME || (level.voteYes+ level.voteNo) >= level.numFullyConnectedClients) {
			trap_SendServerCommand(-1, va("print \"^3%s^7results: %d Yes vs %d No\n\"", level.voteDisplayString, level.voteYes, level.voteNo));
			level.votingOpinion = qfalse;
			level.votingOpinionAll = qfalse;
		}
		else {
			// still waiting for a majority
			return;
		}
	}
	else if (level.votingOpinion) {
		if (level.time - level.voteTime >= VOTE_TIME || (level.voteYes+ level.voteNo) >= requiredClients) {
			trap_SendServerCommand(-1, va("print \"^3%s^7results: %d Yes vs %d No\n\"", level.voteDisplayString, level.voteYes, level.voteNo));
			level.votingOpinion = qfalse;
			level.votingOpinionAll = qfalse;
		}
		else {
			// still waiting for a majority
			return;
		}
	}
	else {
		if (level.time - level.voteTime >= VOTE_TIME) {
			trap_SendServerCommand(-1, va("print \"%s: %d Yes vs %d No\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED"), level.voteYes, level.voteNo));
		}
		else {
			if (level.voteYes > requiredClients / 2 && (!g_slowVote.integer || level.voteYes == requiredClients)) {
				// execute the command, then remove the vote
				trap_SendServerCommand(-1, va("print \"%s: %d vs %d\n\"", G_GetStripEdString("SVINGAME", "VOTEPASSED"), level.voteYes, level.voteNo));
				level.voteExecuteTime = level.time + G_CalculateVoteExecuteTime();
			}
			else if (level.voteNo > 0 && g_slowVote.integer) {
				trap_SendServerCommand(-1, va("print \"%s (slow voting enabled): %d vs %d\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED"), level.voteYes, level.voteNo));
			}
			else if (level.voteNo >= requiredClients / 2) {
				// same behavior as a timeout
				trap_SendServerCommand(-1, va("print \"%s: %d vs %d\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED"), level.voteYes, level.voteNo));
			}
			else {
				// still waiting for a majority
				return;
			}
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}

/*
==================
PrintTeam
==================
*/
void PrintTeam(team_t team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(team_t team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s" S_COLOR_WHITE " is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s" S_COLOR_WHITE " is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s" S_COLOR_WHITE " %s\n\"", level.clients[client].pers.netname, G_GetStripEdString("SVINGAME", "NEWTEAMLEADER")) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( team_t team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( team_t team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEFAILED")) );
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEPASSED")) );
			//
			if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				//set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEFAILED")) );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );

}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

static void G_CheckUnpause(void)
{
	static int lastPrint = -1;
	int secs;

	//if game is not paused, an admin unpaused, so just cancel.
	if (!g_pauseGame.integer) {
		level.unpauseTime = 0;
		return;
	}

	secs = level.unpauseTime - level.time;		// how many ms until pause?
	secs /= 1000;	//how many seconds left until game will unpause?
	secs += 1;		//so it prints 3,2,1 instead of 2,1,0

	if (secs != lastPrint) {
		G_CenterPrint(-1,0, va("Unpause in ^5%d ^7secs", secs),qtrue,qfalse,qfalse,NULL);
		lastPrint = secs;
	}

	if (level.time >= level.unpauseTime) {
		//time to unpause.
		//trap_SendConsoleCommand(EXEC_APPEND, PAUSEGAME_CVARNAME" 0\n");
		trap_Cvar_Set(PAUSEGAME_CVARNAME, "0"); // change by bucky, but maybe it's better to append to keep the cvar state consistent with the value stored in the vmCvar_t? meh prolly irrelevant

		//clear the "unpause in 1 sec" message.
		G_CenterPrint(-1, 0, " ", qtrue, qfalse, qfalse, NULL);

		level.unpauseTime = 0;
		level.unpauseClient = -2;
		lastPrint = -1;
	}


}


#define AUTOGEN_ARENA_NAME "_autoGenArenas" // changed back to _ from 0. doesnt affect ordering anyway
/*
=============
G_AutoGenerateArena
=============
*/
void G_AutoGenerateArena(const char* thisMapName, qboolean checkBspExists, qboolean immediatePrint, qboolean silentSkip)
{
	vmCvar_t		mapname;
	int				len = 0;
	fileHandle_t	f;
	static char		arenaText[MAX_ARENAS_TEXT];
	int				arenaTextLength;
	int				arenaFileIndex = 0;
	const char*		tmp;

	if (!level.arenasLoaded) {
		G_BufferedSendOrPrint(NULL,qtrue,immediatePrint,va("^1Can't generate arena, arenas weren't loaded (can't avoid dupes).\n"));
		return;
	}


	if (checkBspExists) {
		int i;

		if (!Q_stricmp(thisMapName, DF_GetCourseName(qfalse)) && level.hasArenaInfo) {
			G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^3Arena auto generation skipped, %s already has arena info.\n", thisMapName));
		}

		if (G_DoesMapHaveArena(thisMapName)) {
			if (!silentSkip) {
				G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^3Arena auto generation skipped, %s already has arena info.\n", thisMapName));
			}
			return;
		}

		if (G_IsMapBlacklisted(thisMapName)) {
			G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^3Arena auto generation skipped, %s is blacklisted.\n", thisMapName));
			return;
		}

		tmp = va("maps/%s.bsp", thisMapName);
		trap_FS_FOpenFile(tmp, &f, FS_READ);
		if (!f) {
			G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^1Arena auto generation skipped, cannot find/open %s.\n",tmp));
			return;
		}
		trap_FS_FCloseFile(f);

	}

	Q_strncpyz(arenaText,va("{\nmap \"%s\"\nlongname \"%s\"\ntype \"ffa\"\n}\n", thisMapName,level.message[0] ? level.message : thisMapName),sizeof(arenaText));

	arenaTextLength = strlen(arenaText);
	while (((len=trap_FS_FOpenFile(va("scripts/" AUTOGEN_ARENA_NAME "%d.arena",arenaFileIndex), &f, FS_READ)) + arenaTextLength + 2) > MAX_ARENAS_TEXT){
		if (!f) {

			// file doesnt exist yet. good. wait, we would prolly never get here then. oh well
			break;
		}
		trap_FS_FCloseFile(f);
		f = 0;
		arenaFileIndex++;
	}
	if (f) {
		trap_FS_FCloseFile(f); // we need to close and reopen it. the first open was in FS_READ mode to get the filesize. second open is in FS_APPEND mode. if the file doesnt yet exist thats fine, we will create it.
		f = 0;
	}

	trap_FS_FOpenFile(va("scripts/" AUTOGEN_ARENA_NAME "%d.arena", arenaFileIndex), &f, FS_APPEND);

	if (!f) {
		G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^1Arena auto generation failed, cannot open scripts/" AUTOGEN_ARENA_NAME "%d.arena for writing.\n", arenaFileIndex));
		return;
	}
	else {
		G_BufferedSendOrPrint(NULL, qtrue, immediatePrint, va("^2Generating arena for %s (length %d) in scripts/" AUTOGEN_ARENA_NAME "%d.arena (length %d).\n", thisMapName, arenaTextLength, arenaFileIndex, len));
	}

	trap_FS_Write(arenaText,arenaTextLength,f);

	trap_FS_FCloseFile(f);
}

int sortqueuedduelers(const void* a, const void* b) {
	gentity_t* player1 = &g_entities[*(int*)a];
	gentity_t* player2 = &g_entities[*(int*)b];
	// whoever didnt get flag for longest time is most eligible to get it
	if (player1->client->pers.lastDuel == player2->client->pers.lastDuel) {
		return player2->client->pers.lastDuelStatus - player1->client->pers.lastDuelStatus; // player who won gets priority to keep playing
	}
	return player1->client->pers.lastDuel - player2->client->pers.lastDuel;
}
typedef struct playerDuelPairup_s {
	int	player1;
	int player2;
	int lastTime;
}playerDuelPairup_t;
int sortqueuedduelcombos(const void* a, const void* b) {
	playerDuelPairup_t* combo1 = (playerDuelPairup_t*)a;
	playerDuelPairup_t* combo2 = (playerDuelPairup_t*)b;
	// whichever combo didn't happen the longest gets priority
	return combo1->lastTime - combo2->lastTime;
}
void G_CheckDuelQueueStatus() {
	int i;
	gentity_t* ent = g_entities;
	gentity_t* ent2;
	int queuedDuelers[MAX_CLIENTS];
	int queuedDuelerCount = 0;
	vec3_t existingDuelersPos[MAX_CLIENTS];
	int existingDuelerCount = 0;
	vec3_t spawnpoint;
	vec3_t spawnpointAngles;
	vec3_t spawnpointWiggled;
	qboolean anyPastTimeout = qfalse;
	int randomteam;

	if (level.intermissiontime || level.intermissionQueued) {
		return;
	}

	// find players in duel queue mode that aren't dueling, but could be.
	// TODO ignore afk players if g_duelQueueAutoRespawn is 1?
	for (i = 0; i < level.maxclients; i++, ent++) {
		if (!ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED || ent->client->sess.mode != MODE_DUELQUEUE || ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if (clampedIntAdd(level.time,-ent->client->sess.lastHereTime) > 30000) { // ignore afk player
			continue;
		}
		if (ent->client->pers.lastDuel > level.time) {
			ent->client->pers.lastDuel = 0; // shouldn't happen but who knows
		}
		if (ent->client->ps.duelInProgress) {
			ent->client->pers.lastDuel = level.time; // doing this here so players are in sync with the lastDuel thing. unlike privateDuelTime which happens in clientthink_real
			VectorCopy(ent->client->ps.origin,existingDuelersPos[existingDuelerCount]);
			existingDuelerCount++;
			continue;
		}
		if (!G_PlayerCanDuel(ent, qfalse,qfalse)) {
			// in spec or already in a duel
			continue;
		}
		if (ent->client->pers.lastDuel + g_duelQueueTimeout.integer <= level.time) {
			anyPastTimeout = qtrue;
		}
		queuedDuelers[queuedDuelerCount++] = i;
	}
	if (queuedDuelerCount < 2 || !anyPastTimeout) {
		// need at least 2 and at least one person should be past the timeout.
		// idea is: the timeout allows new player combinations. otherwise the timing will guarantee that it's always the same pairs of ppl playing together
		return;
	}

	if (queuedDuelerCount % 2) {
		// if we can't pair them up, sort them to see who's waited the longest
		qsort(queuedDuelers, queuedDuelerCount, sizeof(queuedDuelers[0]), sortqueuedduelers);
	}

	if (g_developer.integer) {
		G_Printf("Sorted duel queue candidates: \n");
		for (i = 0; i < queuedDuelerCount; i++) {
			ent = g_entities + queuedDuelers[i];
			G_Printf("client %d: lastduel %d, lastduelstatus %d\n", queuedDuelers[i],ent->client->pers.lastDuel, ent->client->pers.lastDuelStatus);
		}
	}

	queuedDuelerCount = queuedDuelerCount / 2 * 2; // round to pairs

	// ok now we need to do some magic to try to get new player combinations
	// if there's 4 players and their matches ended recently, they are going to be sorted such that they will most likely end up paired
	// with the same person again
	// so let us make a list of all possible player combos with the current player pool and prioritize the ones that haven't happened for a while
	if(queuedDuelerCount > 2){
		playerDuelPairup_t pairs[(MAX_CLIENTS * (MAX_CLIENTS-1)) / 2] = { 0 }; 
		int pairsSet = 0;
		int clientMask = 0;
		int j;
		for (i = 0; i < queuedDuelerCount; i ++) {
			for (j = i+1; j < queuedDuelerCount; j++) {
				ent = g_entities + queuedDuelers[i];
				ent2 = g_entities + queuedDuelers[j];
				pairs[pairsSet].player1 = queuedDuelers[i];
				pairs[pairsSet].player2 = queuedDuelers[j];
				pairs[pairsSet].lastTime = MIN(ent->client->pers.lastDueled[queuedDuelers[j]], ent2->client->pers.lastDueled[queuedDuelers[i]]); // double cuz a client can disconnect and i cba cleaning it up in all other clients, pointless waste of time.
				pairsSet++;
			}
		}
		qsort(pairs, pairsSet, sizeof(pairs[0]), sortqueuedduelcombos);
		queuedDuelerCount = 0;
		for (i = 0; i < pairsSet; i++) {
			if (!(clientMask & (1 << pairs[i].player1)) && !(clientMask & (1 << pairs[i].player2))) {
				// this combo is still possible (both players not used yet)
				clientMask |= (1 << pairs[i].player1);
				clientMask |= (1 << pairs[i].player2);
				queuedDuelers[queuedDuelerCount++] = pairs[i].player1;
				queuedDuelers[queuedDuelerCount++] = pairs[i].player2;
			}
		}
	}

	for (i = 0; i < queuedDuelerCount; i += 2) {
		vec3_t vecto;
		vec3_t ang;
		float distsq;
		qboolean needRespawn = qtrue;

		ent = g_entities + queuedDuelers[i];
		ent2 = g_entities + queuedDuelers[i + 1];

		if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, ent2)) // this player combo is not allowed
		{
			continue;
		}

		//if (g_duelQueueAutoRespawn.integer) { // no need to double check, already integrated in G_PlayerCanDuel
			if (ent->health <= 0 || ent->client->ps.stats[STAT_HEALTH] < 1) {
				respawn(ent);
			}
			if (ent2->health <= 0 || ent2->client->ps.stats[STAT_HEALTH] < 1) {
				respawn(ent2);
			}
		//}

		VectorSubtract(ent->r.currentOrigin, ent2->r.currentOrigin,vecto);
		distsq = VectorLengthSquared(vecto);
		if (distsq < 256.0f* 256.0f && distsq > 80.0f*80.0f) { // is within a reasonable range. not too far away, not too close
			trace_t trace;
			// they are close enough to each other but can they see each other?
			JP_Trace(&trace, ent->client->ps.origin, NULL, NULL, ent2->client->ps.origin, -1, MASK_SOLID);
			if (trace.fraction == 1.0f) {
				// ALL GOOD!
				needRespawn = qfalse;
			}
		}

		if (needRespawn) {
			vec3_t pos2;
			// unlink them so we can teleport them without their old positions having any influence on anything.
			trap_UnlinkEntity(ent);
			trap_UnlinkEntity(ent2);

			// try nice method with precalculated spawn points first. 
			if (SelectRandomFurthestDuelQueueSpawnPointV2(ent, ent2, existingDuelersPos, existingDuelerCount, spawnpoint, pos2)) {
				VectorClear(spawnpointAngles);

				VectorCopy(spawnpoint, spawnpointWiggled);
				WiggleSpotTelefrag(spawnpointWiggled, ent);
				spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
				TeleportPlayer(ent, spawnpointWiggled, spawnpointAngles);
				VectorClear(ent->client->ps.velocity);

				VectorCopy(pos2, spawnpointWiggled);
				WiggleSpotTelefrag(spawnpointWiggled, ent2);
				spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
				TeleportPlayer(ent2, spawnpointWiggled, spawnpointAngles);
				VectorClear(ent2->client->ps.velocity);

				if (g_developer.integer) {
					G_Printf("Duel queue locations chosen via cool method.\n");
				}
			}
			else {
				// fallback to shittyer method
				// Find a spawn 
				//SelectSpawnPoint(ent, vec3_origin, spawnpoint, spawnpointAngles);
				SelectRandomFurthestDuelQueueSpawnPoint(ent, existingDuelersPos, existingDuelerCount, spawnpoint, spawnpointAngles);

				VectorCopy(spawnpoint, spawnpointWiggled);
				WiggleSpotTelefrag(spawnpointWiggled, ent);
				spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
				TeleportPlayer(ent, spawnpointWiggled, spawnpointAngles);
				VectorClear(ent->client->ps.velocity);

				// move this one away a bit
				if (!G_CheckForNearbyDuelSpawn(ent2, ent->client->ps.origin, spawnpointWiggled, spawnpointAngles)) {
					// uh oh
					// gimme random spawn?
					SelectSpawnPoint(ent2, vec3_origin, spawnpoint, spawnpointAngles);
					VectorCopy(spawnpoint, spawnpointWiggled);
					WiggleSpotTelefrag(spawnpointWiggled, ent2);
				}
				spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
				TeleportPlayer(ent2, spawnpointWiggled, spawnpointAngles);
				VectorClear(ent2->client->ps.velocity);

				if (g_developer.integer) {
					G_Printf("Duel queue locations chosen via dumb method.\n");
				}
			}

			VectorSubtract(ent->r.currentOrigin, ent2->r.currentOrigin, vecto);
		}


		// make them look at each other.
		VectorNormalize(vecto);
		vectoangles(vecto,ang);
		DF_PreDeltaAngleChange(ent2->client);
		SetClientViewAngle(ent2,ang);
		DF_PostDeltaAngleChange(ent2->client,qtrue);
		VectorScale(vecto, -1, vecto); // invert the vector
		vectoangles(vecto, ang);
		DF_PreDeltaAngleChange(ent->client);
		SetClientViewAngle(ent, ang);
		DF_PostDeltaAngleChange(ent->client, qtrue);

		G_StartDuel(ent,ent2,qtrue);

		ent->client->ps.fd.privateDuelTime = 0;
		ent2->client->ps.fd.privateDuelTime = 0;
		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent2->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;
		ent2->client->ps.forceHandExtendTime = level.time + 1000;
	}
}
int sortironmanners(const void* a, const void* b) {
	gentity_t* player1 = &g_entities[*(int*)a];
	gentity_t* player2 = &g_entities[*(int*)b];
	qboolean afk1 = clampedIntAdd(level.time, -player1->client->sess.lastHereTime) > 30000;
	qboolean afk2 = clampedIntAdd(level.time, -player2->client->sess.lastHereTime) > 30000;

	if (afk1 != afk2) {
		return afk1 - afk2; // people who are afk shouldn't get the flag
	}

	// whoever didnt get flag for longest time is most eligible to get it
	return player1->client->pers.lastIronmanFlagGiven - player2->client->pers.lastIronmanFlagGiven;
}
gentity_t* PrintCTFMessage(int plIndex, int teamIndex, int ctfMessage);
void G_CheckIronManStatus() {
	int i;
	gentity_t* ent = g_entities;
	int ironManners[MAX_CLIENTS];
	int ironmannerCount = 0;
	int flagCount = 0;
	vec3_t spawnpoint;
	vec3_t spawnpointAngles;
	vec3_t spawnpointWiggled;
	int randomteam;

	if (level.intermissiontime || level.intermissionQueued || level.lastIronManKilled + IRONMAN_NEXTCAPPER_TIMEOUT > level.time) { // leave us 3 seconds to breathe after ironman is killed
		return; 
	}

	level.ironManClientNum = -1;

	for (i = 0; i < level.maxclients; i++, ent++) {
		if (!ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED || ent->client->sess.mode != MODE_IRONMAN || ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if (ent->client->ps.powerups[PW_BLUEFLAG] || ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_NEUTRALFLAG]) {
			flagCount++;
			level.ironManClientNum = i;
		}
		ironManners[ironmannerCount++] = i;
	}
	if (flagCount >= 1 || ironmannerCount <= 1) {
		// > 1 would be very weird tho!
		return;
	}
	// ok nobody has flag. take care of things.
	qsort(ironManners,ironmannerCount,sizeof(ironManners[0]),sortironmanners);

	level.ironManPosCount = 0;
	level.lastIronManPosSaved = 0;

	// unlink all ironmanners so we can teleport them without their old positions having any influence on anything.
	for (i = 0; i < ironmannerCount; i++) { 
		ent = &g_entities[ironManners[i]];

		trap_UnlinkEntity(ent);
	}

	// Spawn somewhere
	SelectSpawnPoint(ent, vec3_origin, spawnpoint, spawnpointAngles);

	for (i = ironmannerCount-1; i >= 0; i--) { // spawn the ironman last so if theres anything dimension crushing, he still survives
		ent = &g_entities[ironManners[i]];

		// reset health/revive
		if (ent->health <= 0) {
			respawn(ent);
			// unlink us so we can wiggle without our own position affecting us. teleportplayer will relink us anyway
			trap_UnlinkEntity(ent);
		}
		ent->client->isIronMan = qfalse;
		ent->health = 100;
		ent->client->ps.stats[STAT_HEALTH] = 100;
		ent->client->ps.stats[STAT_ARMOR] = 25;
		ent->client->isIronMan = qfalse;

		VectorCopy(spawnpoint, spawnpointWiggled);
		WiggleSpotTelefrag(spawnpointWiggled,ent);
		spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
		TeleportPlayer(ent, spawnpointWiggled, spawnpointAngles);
		spawnpointWiggled[2] -= 1.0f; // since teleportplayer adds that
	}

	// set data for the iron man (ent will be the ironman as we counted backwards in the looop)
	ent->client->isIronMan = qtrue;
	ent->client->ps.stats[STAT_ARMOR] = 100; // ironman gets full armor
	ent->client->ps.eFlags |= EF_INVULNERABLE;
	ent->client->invulnerableTimer = level.time + 3000; // give ironman 2 seconds of invulnerability to not get insta killed
	ent->client->pers.lastIronmanFlagGiven = level.time;
	randomteam = Q_irand(PW_REDFLAG, 2, qfalse, PW_REDFLAG);
	ent->client->ps.powerups[randomteam] = INT_MAX; // lets not do neutral flag cuz its not actually visible :/
	PrintCTFMessage(ent - g_entities, randomteam == PW_REDFLAG ? TEAM_RED : TEAM_BLUE, CTFMESSAGE_PLAYER_GOT_FLAG);

	if (g_ctfPersStats.integer > 1) {
		ent->client->pers.teamState.flagsince = level.time;
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
	}

	VectorCopy(ent->client->ps.origin, level.ironManCurrentPosition);
	level.ironManCurrentPositionSet = qtrue;
}



/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime; 
	int			nowTime = ent->s.eType == ET_MOVER ? MOVERTIME_ENT(ent) : level.time;

	if (g_pauseGame.integer && !g_defrag.integer ) { // && !MOVERUSESCLIENTTIME(ent)
		// stop thinks except if defrag is active. would mess with racers
		return;
	}

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > nowTime) {
		return;
	}
	
	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

void G_UserCmdBuffer_NewFrame();

/*
================
G_RunFrameSpectators

Lighter version of G_RunFrame that only updates stuff so spectators get up-to-date info when using sv_gamefps
================
*/
void G_RunFrameSpectators(int levelTime) {
	int			i;
	gentity_t* ent;

	for (i = 0; i < level.maxclients; i++) {
		ent = g_entities + i;
		if (!ent->inuse || !ent->client) continue;
		ent->client->pers.normalFollowerPing = ent->client->ps.ping; // server (except on map restarts and such) calcs proper pings and saves them to the ps before running game frame. intercept it here so we know it and can send the real ping even for clients that are spectating.
	}

	if (g_entHUDFields.integer) {
		ent = &g_entities[0];
		//for (i = 0; i < level.num_entities; i++, ent++)
		for (i = 0; i < MAX_CLIENTS; i++, ent++)
		{
			if (!ent->inuse) {
				continue;
			}
			else if (ent->client) {

				if (ent->s.eType == ET_GENERAL || ent->client->ps.pm_type == PM_INTERMISSION) {
					// We are in an intermission and players have been turned into ET_GENERAL. 
					// Set all this stuff to 0 so we don't get glowing bodies
					ent->client->ps.generic1 = ent->s.generic1 = 0;
					ent->client->ps.fd.forceMindtrickTargetIndex3 = ent->s.trickedentindex3 = 0;
					ent->client->ps.fd.forceMindtrickTargetIndex4 = ent->s.trickedentindex4 = 0;
				}
				else {
					//ent->client->ps.fd.forcePowersActive &= ~(31 << 20);
					//ent->client->ps.fd.forcePowersActive |= ((dimensionNum & 31) << 20);
					// trickedentindex3: armor (8 bits), health (8 bits)
					ent->client->ps.fd.forceMindtrickTargetIndex3 = ent->s.trickedentindex3 = ((MIN(127, MAX(-128, ent->client->ps.stats[STAT_HEALTH])) & 0xff) << 8) | (MIN(255, MAX(0, ent->client->ps.stats[STAT_ARMOR])) & 0xff);
					// trickedentindex4: force power (7 bits), current weapon ammo (7 bits), saberdrawanimlevel (2 bits)
					ent->client->ps.fd.forceMindtrickTargetIndex4 = ent->s.trickedentindex4 = (ent->client->ps.fd.saberDrawAnimLevel & 3) << 14 | ((MAX(0, MIN(127, ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex])) & 127) << 7) | (MAX(0, MIN(127, ent->client->ps.fd.forcePower)) & 127);
					// generic1: seeker, forcefield, bacta, sentry in inventory (1 bit each), mine count (4 bits)
					ent->client->ps.generic1 = ent->s.generic1 = ((MAX(0, MIN(15, ent->client->ps.ammo[weaponData[WP_TRIP_MINE].ammoIndex])) & 15) << 4) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN))) << 3) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC))) << 2) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD))) << 1) | (!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)));
				}
			}

			//ent->s.forcePowersActive &= ~(31 << 20);
			//ent->s.forcePowersActive |= ((dimensionNum & 31) << 20);
		}
	}

	ent = &g_entities[0];
	for (i = 0; i < level.maxclients; i++, ent++) {
		if (ent->inuse) {
			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
				SpectatorClientEndFrame(ent); // put spectators in their own loop so they get the truly most updated version
			}
		}
	}

	// do i do this here? idk
	//G_UserCmdBuffer_NewFrame();
}
/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/

void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;
	int start, end;
	int			activeRunnerCount = 0;
	int			specAllEntsBroadcastClients[2] = { 0,0 };

	for (i = 0; i < level.maxclients; i++) {
		ent = g_entities + i;
		if (!ent->inuse || !ent->client) continue;
		ent->client->pers.normalFollowerPing = ent->client->ps.ping; // server (except on map restarts and such) calcs proper pings and saves them to the ps before running game frame. intercept it here so we know it and can send the real ping even for clients that are spectating.
	}

	G_BufferedSendOrPrintFlushIfNeeded(NULL, qtrue);

	if (gDoSlowMoDuel)
	{
		if (level.restarted)
		{
			char buf[128];
			float tFVal = 0;

			trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

			tFVal = atof(buf);

			trap_Cvar_Set("timescale", "1");
			if (tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
		else
		{
			float timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
			float useDif = 0; //the difference to use when actually setting the timescale

			if (timeDif < 150)
			{
				trap_Cvar_Set("timescale", "0.1f");
			}
			else if (timeDif < 1150)
			{
				useDif = (timeDif/1000); //scale from 0.1 up to 1
				if (useDif < 0.1)
				{
					useDif = 0.1f;
				}
				if (useDif > 1.0)
				{
					useDif = 1.0;
				}
				trap_Cvar_Set("timescale", va("%f", useDif));
			}
			else
			{
				char buf[128];
				float tFVal = 0;

				trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

				tFVal = atof(buf);

				trap_Cvar_Set("timescale", "1");
				if (timeDif > 1500 && tFVal == 1.0f)
				{
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

#ifdef DEBUGFPS
#define	FPS_FRAMES	16	//orig
	if(g_debugFps.integer){
		static int	previousTimes[FPS_FRAMES];
		static int	index = 0, previous = 0;
		int			i, total, fps;
		int			t, frameTime;

		t = trap_Milliseconds();
		frameTime = t - previous;
		previous = t;

		// frameTime = cg.realframetime;	 HAX!
		previousTimes[index % FPS_FRAMES] = frameTime;
		if (++index > FPS_FRAMES) {
			// average multiple frames together to smooth changes out a bit
			total = 0;
			for (i = 0; i < FPS_FRAMES; i++) {
				total += previousTimes[i];
			}
			if (!total) {
				total = 1;
			}
			fps = 1000 * FPS_FRAMES / total;

			level.avgfps = fps;

			level.fpsSamples++;
			level.fpsFrameTime += frameTime;
		}
	}
#endif


	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	level.frameTimeMsec = msec = level.time - level.previousTime;

	//if ( level.pause.state != PAUSE_NONE ) {
	if (g_pauseGame.integer && g_pauseTimerFreeze.integer) {
		static int lastCSTime = 0;
		int dt = level.time - level.previousTime;

		// compensate for timelimit and warmup time
		if (level.warmupTime > 0)
			level.warmupTime += dt;
		level.startTime += dt;

		// floor start time to avoid time flipering
		if ((level.time - level.startTime) % 1000 >= 500)
			level.startTime += (level.time - level.startTime) % 1000;

		// initial CS update time, needed!
		if (!lastCSTime)
			lastCSTime = level.time;

		// client needs to do the same, just adjust the configstrings periodically
		// i can't see a way around this mess without requiring a client mod.
		if (lastCSTime < level.time - 1000) {
			lastCSTime += 1000;
			trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));
			if (level.warmupTime > 0)
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
		}
	}

	g_TimeSinceLastFrame = (level.time - g_LastFrameTime);

	// get any cvar changes
	G_UpdateCvars();

	G_CheckCvarChanges();

	DF_CheckRaceCvarChanges(qfalse);

	G_DB_CheckResponses();

	if (g_defrag.integer && !level.mapDefaultsConfirmed && !level.mapDefaultsLoadFailed && (level.time > (level.mapDefaultsProblemLastAnnounced+1000)|| level.time < level.mapDefaultsProblemLastAnnounced)) {
		G_CenterPrint(-1,3,"^1Loading map defaults...",qfalse,qfalse,qtrue, NULL);
		//trap_SendServerCommand(-1,"print \"^1Loading map defaults...\n\"");
		level.mapDefaultsProblemLastAnnounced = level.time;
	}

	G_SetSpecAllEntsBroadcasts(specAllEntsBroadcastClients);

	G_DebugHandleState();

	//
	// go through all allocated objects
	//
	start = trap_Milliseconds();
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->client ) {
			// clients have their own handling
			memset(ent->r.broadcastClients, 0, sizeof(ent->r.broadcastClients));
		}
		if ( !ent->inuse ) {
			continue;
		}

		if (!ent->client) {
			// clients have their own handling
			ent->r.broadcastClients[0] |= specAllEntsBroadcastClients[0];
			ent->r.broadcastClients[1] |= specAllEntsBroadcastClients[1];
		}

		// clear events that are too old
		if ( LEVELTIME(ent->client) - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{ //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					G_FreeEntity( ent );
					continue;
				}
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			if (g_defrag.integer && ent->activatorReal && ent->activatorReal->inuse && ent->activatorReal->client && ent->activatorReal->client->sess.raceMode) {
				// we run this mover on client time.
			}
			else 
			{
				G_RunMover(ent);
			}
			continue;
		}

		if ( i < MAX_CLIENTS ) 
		{
			G_CheckClientTimeouts ( ent );
			
			if (!ent->client->sess.raceMode) {
				if((!level.intermissiontime)&&!(ent->client->ps.pm_flags&PMF_FOLLOW) && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
				{
					WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
					WP_SaberPositionUpdate(ent, &ent->client->pers.cmd );
				}
			}
			G_RunClient( ent );

			if (ent->client->sess.raceMode && 
				( ent->client->pers.raceStartCommandTime // actively running
				|| (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && ent->client->pers.segmented.state > SEG_RECORDING // replaying
				|| ent->client->pers.recordingDemo && ent->client->pers.keepDemoMaybe // run finished recently. lets not lag demo
				)
				) {
				activeRunnerCount++;
			}

			continue;
		}

		G_RunThink( ent );
	}

	if (!activeRunnerCount && !level.hasArenaInfo && level.mustGenerateArena) {
		G_AutoGenerateArena(DF_GetCourseName(qfalse),qfalse,qtrue,qfalse);
		level.mustGenerateArena = qfalse;
		level.hasArenaInfo = qtrue;
	}

	if (!level.nextRandomTip) {
		level.nextRandomTip = level.time + 30000;
	}

	if (level.time > level.nextRandomTip && g_randomTipInterval.integer) {
		int tries = 0;
		qboolean failed = qfalse;
		helpTip_t* tip = NULL;
		tip = &helpTips[Q_irand(0,helpTipCount,qfalse,0)];
		while (tip->header || !tip->randomTipPrint || tip->raceOnly && !g_defrag.integer || tip->allowfunc && !tip->allowfunc(NULL)) {
			tip = &helpTips[Q_irand(0, helpTipCount, qfalse, 0)];
			tries++;
			if (tries >= 10) {
				// why am i doing this?
				// because qvm might (?) behave very strange with q_irand and in some freak situation never give us what we want.
				// let's be safe, shrug
				failed = qtrue;
				break;
			}
		}
		if (!failed) {
			trap_SendServerCommand(-1, tip->randomTipPrint);
			level.nextRandomTip = clampedIntAdd(level.time, MAX(clampedIntMult(clampedIntMult(MAX(1, g_randomTipInterval.integer), 1000), Q_irand(100, 400, qfalse, 200)) / 200, 1000));
		}
	}

	if (!level.numPlayingClients && (clampedIntAdd(level.lastAllRankUpdate, 60000) < level.time || level.time < level.lastAllRankUpdate || !level.lastAllRankUpdate && level.time)) {
		//Com_Printf("^3Executing auto rank update.");
		DF_UpdateRanksMainRequest(NULL,NULL,qfalse,5); // up to 5 maps at a time
	}
	else if (level.shouldUpdateMapRanks && !activeRunnerCount) {
		DF_UpdateRanksMainRequest(NULL, DF_GetCourseName(qfalse), qfalse, 0); // up to 5 maps at a time
		level.shouldUpdateMapRanks = qfalse;
	}

	if (g_modes.integer) {
		G_CheckIronManStatus();
		G_CheckDuelQueueStatus();
	}

	G_CheckEnqueuedClips(qfalse);

	// Process logical entities
	ent = &g_entities[MAX_GENTITIES];
	for (i = 0; i < level.num_logicalents; i++, ent++) {
		if (!ent->inuse) {
			continue;
		}
		// Logical entities only think, nothing else
		G_RunThink(ent);
	}

end = trap_Milliseconds();

	trap_ROFF_UpdateEntities();

	if (g_entHUDFields.integer) {
		ent = &g_entities[0];
		//for (i = 0; i < level.num_entities; i++, ent++)
		for (i = 0; i < MAX_CLIENTS; i++, ent++)
		{
			if (!ent->inuse) {
				continue;
			}
			else if (ent->client) {

				if (ent->s.eType == ET_GENERAL || ent->client->ps.pm_type == PM_INTERMISSION) {
					// We are in an intermission and players have been turned into ET_GENERAL. 
					// Set all this stuff to 0 so we don't get glowing bodies
					ent->client->ps.generic1 = ent->s.generic1 = 0;
					ent->client->ps.fd.forceMindtrickTargetIndex3 = ent->s.trickedentindex3 = 0;
					ent->client->ps.fd.forceMindtrickTargetIndex4 = ent->s.trickedentindex4 = 0;
				}
				else {
					//ent->client->ps.fd.forcePowersActive &= ~(31 << 20);
					//ent->client->ps.fd.forcePowersActive |= ((dimensionNum & 31) << 20);
					// trickedentindex3: armor (8 bits), health (8 bits)
					ent->client->ps.fd.forceMindtrickTargetIndex3 = ent->s.trickedentindex3 = ((MIN(127,MAX(-128,ent->client->ps.stats[STAT_HEALTH])) & 0xff) << 8) | (MIN(255, MAX(0, ent->client->ps.stats[STAT_ARMOR])) & 0xff);
					// trickedentindex4: force power (7 bits), current weapon ammo (7 bits), saberdrawanimlevel (2 bits)
					ent->client->ps.fd.forceMindtrickTargetIndex4 = ent->s.trickedentindex4 = (ent->client->ps.fd.saberDrawAnimLevel & 3) << 14 | ((MAX(0, MIN(127, ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex])) & 127) << 7) | (MAX(0, MIN(127, ent->client->ps.fd.forcePower)) & 127);
					// generic1: seeker, forcefield, bacta, sentry in inventory (1 bit each), mine count (4 bits)
					ent->client->ps.generic1 = ent->s.generic1 = ((MAX(0, MIN(15, ent->client->ps.ammo[weaponData[WP_TRIP_MINE].ammoIndex])) & 15) << 4) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN))) << 3) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC))) << 2) | ((!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD))) << 1) | (!!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)));
				}
			}

			//ent->s.forcePowersActive &= ~(31 << 20);
			//ent->s.forcePowersActive |= ((dimensionNum & 31) << 20);
		}
	}

start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
				ClientEndFrame(ent,qfalse);
			}
		}
	}
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
				SpectatorClientEndFrame(ent); // put spectators in their own loop so they get the truly most updated version
			}
		}
	}
end = trap_Milliseconds();

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	if (level.unpauseTime) {
		// an unpause is pending.
		G_CheckUnpause();
	}


	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();


	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = 0;
		}
	}

	G_UserCmdBuffer_NewFrame();

	g_LastFrameTime = level.time;

	// suppress unused-but-set warnings
	(void)start;
	(void)end;
	(void)msec;
}

const char *G_GetStripEdString(char *refSection, char *refName)
{
	/*
	static char text[1024]={0};
	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a striped reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the striped reference indication and dealt with
	//properly.
	static char text[1024]={0};
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

// On linux rand() behaves different than on Winodws or in a qvm, ...
static int myRandSeed = 0;
void	mysrand( unsigned seed ) {
	myRandSeed = seed;
}

int		myrand( void ) {
	myRandSeed = (69069 * myRandSeed + 1);
	return myRandSeed & 0x7fff;
}

void G_StringAppendSubstring( char *dst, size_t dstSize, const char *src, size_t srcLen )
{
	Q_strcat( dst, strlen(dst)+srcLen+1 >= dstSize ? dstSize : strlen(dst)+srcLen+1, src );
}

/*
================
MV_BBoxToTime2

This function uses an entity state's time2 value to encode the missing 3 bbox values that are not encoded by trap_LinkEntity already.
This function is supposed to be called for solid entities with a non-symmetric bbox after the entity was linked.
================
*/
void MV_BBoxToTime2( gentity_t *ent )
{
	int maxs1, mins0, mins1;

	// Only do this if the entity is solid (same condition as in the engine when calling trap_LinkEntity)
	if ( !(ent->r.contents & (CONTENTS_SOLID|CONTENTS_BODY)) )
		return;

	if ( !level.bboxEncoding )
	{ // Okay, from now on we're encoding maxs[1], mins[0] and mins[1] in s.time2; let the clients know about it
		level.bboxEncoding = qtrue;
		MV_UpdateSvFlags();
	}

	// When we call trap_LinkEntity the engine encodes maxs[0], mins[2] and maxs[2] as r.solid
	// However as that's not enough to properly predict bboxes on the client we additionally encode
	// the missing three values (maxs[1], mins[0] and mins[1]) as s.time2 for mvsdk clients to use in
	// prediction. The values are encoded similar to the way the engine encodes them (>= 1).

	maxs1 = ent->r.maxs[1];
	if ( maxs1 < 1 ) maxs1 = 1;
	if ( maxs1 > 255 ) maxs1 = 255;

	mins0 = -(ent->r.mins[0]);
	if ( mins0 < 1 ) mins0 = 1;
	if ( mins0 > 255 ) mins0 = 255;

	mins1 = -(ent->r.mins[1]);
	if ( mins1 < 1 ) mins1 = 1;
	if ( mins1 > 255 ) mins1 = 255;

	// Encode the values for prediction
	ent->s.time2 = (mins1 << 16) | (mins0 << 8) | maxs1;
}

/*
================
MV_ModelindexToTime2

This function uses an entity state's time2 value to transmit the modelindex to work around the 8 bit modelindex limit.
This function is supposed to be called for all entities after assigning the brushmodel.
================
*/
void MV_ModelindexToTime2( gentity_t *ent )
{
	// Don't do this if the server hasn't enabled it.
	if ( !(g_submodelWorkaround.integer & 2) )
		return;

	if ( !level.modelindexTime2 )
	{ // Let clients know that a modelindex can be found in the time2 value now.
		level.modelindexTime2 = qtrue;
		MV_UpdateSvFlags();
	}

	// The original idea was to only store modelindex >= 255 in time2 and signal this by setting modelindex to 255, however
	// as the serverside engine still needs to know the correct modelindex we just copy the modelindex over to time2 and
	// work with that.
	ent->s.time2 = ent->s.modelindex;
}

