// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "../ghoul2/G2.h"

// g_client.c -- client functions that don't happen every frame

static vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
static vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

forcedata_t Client_Force[MAX_CLIENTS];

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int			i;
	char* s;

	ent->specialType = "playerspawn";
	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
	G_SpawnString("spawntype", "", &s);
	if (s && !Q_stricmp(s,"defrag") && ent->spawnDefragPriority < 2) {
		ent->spawnDefragPriority = 2;
	}
	if (level.highestDefragSpawnPriority < ent->spawnDefragPriority) {
		level.highestDefragSpawnPriority = ent->spawnDefragPriority;
	}
	if (ent->notVQ3 || ent->notCPM) {
		G_Printf("^3Q3 style specific spawn found: %s, notvq3 %d, notcpm %d\n",ent->classname,ent->notVQ3,ent->notCPM);
		level.hasQ3StyleSpecificSpawns = qtrue;
	}
	level.deathMatchSpawnCount++;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	G_SetClassName(ent, "info_player_deathmatch");
	ent->spawnDefragPriority = 1;
	SP_info_player_deathmatch( ent );
}

void SP_info_player_race(gentity_t *ent) {
	G_SetClassName(ent, "info_player_deathmatch");
	ent->spawnDefragPriority = 2;
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_imperial (1 0 0) (-16 -16 -24) (16 16 32)
saga start point - imperial
*/
void SP_info_player_imperial(gentity_t *ent) {
	if (g_gametype.integer != GT_SAGA)
	{ //turn into a DM spawn if not in saga game mode
		G_SetClassName(ent, "info_player_deathmatch");
		SP_info_player_deathmatch( ent );
	}
}

/*QUAKED info_player_rebel (1 0 0) (-16 -16 -24) (16 16 32)
saga start point - rebel
*/
void SP_info_player_rebel(gentity_t *ent) {
	if (g_gametype.integer != GT_SAGA)
	{ //turn into a DM spawn if not in saga game mode
		G_SetClassName(ent, "info_player_deathmatch");
		SP_info_player_deathmatch( ent );
	}
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent = &g_entities[self->client->ps.saberIndex];
	vec3_t a;
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{ //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{ //someone killed us and we had the saber thrown, so actually move this saber to the saber location
	  //if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
	  //apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
		return;
	}

	if (!altVelocity)
	{
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0]*256;
		ent->s.pos.trDelta[1] = a[1]*256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap_LinkEntity(ent);
}

void JMSaberThink(gentity_t *ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{
		if (!ent->enemy->client || !ent->enemy->inuse)
		{ //disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2[0] = 1;
			ent->pos2[1] = 0; //respawn next think
			trap_LinkEntity(ent);
		}
		else
		{
			ent->pos2[1] = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->pos2[0] && ent->pos2[1] < level.time)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int i = 0;
//	gentity_t *te;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (other->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master 
	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, va("%i", other->s.number ) );

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = LEVELTIME(other->client) + g_spawnInvulnerability.integer;
	}

	G_CenterPrint( -1, 3, va("%s" S_COLOR_WHITE " %s", other->client->pers.netname, G_GetStripEdString("SVINGAME", "BECOMEJM")), qtrue, qfalse,qtrue, NULL);

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < 200 && other->health > 0)
	{ //full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = 200;
	}

	if (other->client->ps.fd.forcePower < 100)
	{
		other->client->ps.fd.forcePower = 100;
	}

	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

		i++;
	}

	self->pos2[0] = 1;
	self->pos2[1] = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue(self->s.number);

	return;
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t *ent)
{
	if (g_gametype.integer != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}

	ent->enemy = NULL;

	ent->s.eFlags = EF_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( ent->r.maxs, 3, 3, 3 );
	VectorSet( ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = JMSaberTouch;

	trap_LinkEntity(ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/
qboolean ShouldNotCollide(gentity_t* entity, gentity_t* other);
/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( vec3_t origin, gentity_t* spawningEnt) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd(origin, playerMins, mins );
	VectorAdd(origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			if (spawningEnt && !ShouldNotCollide(spawningEnt,hit)) {
				return qtrue;
			}
		}

	}

	return qfalse;
}

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes
#define BUBBLESPAWN_DOWNTRACE 120.0f // we can get up this much with force jump even at 1000fps (121)
qboolean WiggleSpotTelefrag(vec3_t origin, gentity_t* spawningEnt) {
	vec3_t		original;
	vec3_t		test,testdown;
	trace_t		groundTrace;
	int			height;
	int			right;
	int			front;
	qboolean	errorMsgShown = qfalse;

	VectorCopy(origin, original);
	for (height = 0; height < 3; height++) {
		test[2] = original[2]+64.0f*height;
		testdown[2] = original[2] - BUBBLESPAWN_DOWNTRACE;
		for (front = -1; front < 2; front++) {
			testdown[0] = test[0] = original[0]+32.0f*front;
			for (right = -1; right < 2; right++) {
				testdown[1] = test[1] = original[1]+32.0f*right;
				if (!SpotWouldTelefrag(test, spawningEnt)) { // cool, we could spawn here and not kill anyone.
					JP_Trace(&groundTrace,test,playerMins,playerMaxs,testdown,-1, MASK_PLAYERSOLID | MASK_WATER | CONTENTS_NOSPAWN);
					if (groundTrace.startsolid || groundTrace.allsolid || (groundTrace.contents & (MASK_WATER | CONTENTS_NOSPAWN))) {
						continue; // welp, we can't spawn here, in a wall/water/lava/kill trigger or sth
					}
					if (groundTrace.fraction == 1.0f) {
						continue; // nah therer's no ground to stand on
					}
					if (groundTrace.fraction == 0.0f) {
						// this is a weird bug on at least one map i found where you dont get the proper startsolid/allsolid but you get 0 here. its really really odd.
						//if (g_developer.integer) {
						if (!errorMsgShown) {
							Com_Printf("^1WiggleSpotTelefrag: Fraction is 0 but startsolid and allsolid are false. Skipping spawn.\n"); // debug buut it spams :/ screw it
							errorMsgShown = qtrue;
						}
						//}
						continue;
					}
					if (groundTrace.plane.normal[2] < MIN_WALK_NORMAL) {
						continue; // we'd slide down.
					}
					VectorCopy(test, origin);
					//JP_Trace(&groundTrace, test, playerMins, playerMaxs, testdown, -1, MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN); // debug
					return qtrue;
				}
			}
		}
	}
	VectorCopy(original, origin);
	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( gentity_t* spawningEnt ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot->s.origin, spawningEnt) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_FindByClassNameFast( NULL, "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}


gentity_t* SelectDefragSpawnPoint(gentity_t* spawningEnt, vec3_t avoidPoint, vec3_t origin, vec3_t angles)
{
	gentity_t* spot,*startSpot;
	int			 i, j;

	if (spawningEnt->client->pers.chosenDefragSpawnPoint) { // for maps where /savespawn isn't possible
		spot = g_entities + spawningEnt->client->pers.chosenDefragSpawnPoint;
		if (!Q_stricmp(spot->classname, "info_player_deathmatch")) {
			if (!SpotWouldTelefrag(spot->s.origin, spawningEnt)) {

				VectorCopy(spot->s.origin, origin);
				origin[2] += 9;
				VectorCopy(spot->s.angles, angles);
				return spot;
			}
		}
	}

	startSpot = NULL;

	if (spawningEnt->client->pers.lastSpawnPoint) {
		startSpot = g_entities + spawningEnt->client->pers.lastSpawnPoint;
	}

	spot = startSpot;
	for (i = 0; i < 2; i++) {
		// we start out at startSpot and try to find the next in line, so that there's a clean "cycling" through them instead of randomness
		// if there is none after, spot will be NULL and we do a second round as a "wrap around" from the start of g_entities
		while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
			if (SpotWouldTelefrag(spot->s.origin, spawningEnt)) {
				continue;
			}
			if (spot->spawnDefragPriority < level.highestDefragSpawnPriority) {
				continue; // some types of spawns get priority in defrag
			}
			//if (spot == startSpot) { // not rly needed
			//	break;
			//}
			break;
		}
		if (spot || !spot && !startSpot) {
			break;
		}
	}

	if (!spot) {
		spot = G_FindByClassNameFast(NULL, "info_player_deathmatch");
		if (!spot)
		{
			G_Error("Couldn't find a defrag spawn point");
		}
		VectorCopy(spot->s.origin, origin);
		if (g_bubbleSpawn.integer && !(spawningEnt->client && spawningEnt->client->sess.raceMode) && SpotWouldTelefrag(origin, spawningEnt)) {
			WiggleSpotTelefrag(origin, spawningEnt);
		}
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);
		return spot;
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
	return spot;
}
/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint (gentity_t* spawningEnt,vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	if (spawningEnt && spawningEnt->client && spawningEnt->client->sess.raceMode) {
		return SelectDefragSpawnPoint(spawningEnt,avoidPoint,origin,angles);
	}

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot->s.origin, spawningEnt) ) {
			continue;
		}
		if (spawningEnt && spawningEnt->client && spawningEnt->client->sess.raceMode && spot->spawnDefragPriority < level.highestDefragSpawnPriority) {
			continue; // some types of spawns get priority in defrag
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		spot = G_FindByClassNameFast( NULL, "info_player_deathmatch");
		if (!spot)
		{
			G_Error( "Couldn't find a spawn point" );
		}
		VectorCopy (spot->s.origin, origin);
		if (g_bubbleSpawn.integer && !(spawningEnt && spawningEnt->client && spawningEnt->client->sess.raceMode) && SpotWouldTelefrag(origin, spawningEnt)) {
			WiggleSpotTelefrag(origin, spawningEnt);
		}
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}


void G_AnalyzeDuelQueueSpawns() {
	gentity_t* spot = NULL;
	trace_t trace;
	vec3_t from, target;
	float dims[2][2];
	int longerdim,shorterdim,i;
	float offCenter;

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		// trace to floor
		VectorCopy(spot->s.origin, from);
		from[2] += 9.0f; // this is what spawning does :/
		VectorCopy(from, target);
		target[2] -= 50.0f;
		JP_Trace(&trace, from, playerMins, playerMaxs, target, -1, MASK_DEADSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
		if (trace.allsolid || trace.startsolid || trace.fraction == 1.0f || (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN))) {
			// either started in solid or we are over some abyss.
			continue;
		}
		VectorCopy(trace.endpos,from);
		from[2] += 1.0f; // to be safe?
		// trace in 4 directions
		// TODO improve this to trace thicker blocks? or else a map might troll us with some slim X-shaped corridor tricking us into thinking there's a lot of space?
		// but we'd have to do it both directions to find the bigger possible rectangle, then subtract overlap or sth.... annoying
		// Note after observation: the only problem with the current method is that it overestimates the area as many spawns tend to be at junctions between hallways and such. so definitely improve this
		// TODO 2: do floor traces. see ffa_ns_streets.... when centering the long dim especially we need to do regular interval floor traces
		VectorCopy(from, target);
		target[0] += 1000.0f;
		JP_Trace(&trace, from, playerMins, playerMaxs, target, -1, MASK_DEADSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
		if (trace.allsolid || trace.startsolid || trace.fraction == 1.0f || (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN))) {
			dims[0][0] = 0.0f;
		}
		else {
			dims[0][0] = trace.endpos[0] - from[0];
		}

		VectorCopy(from, target);
		target[0] -= 1000.0f;
		JP_Trace(&trace, from, playerMins, playerMaxs, target, -1, MASK_DEADSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
		if (trace.allsolid || trace.startsolid || trace.fraction == 1.0f || (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN))) {
			dims[0][1] = 0.0f;
		}
		else {
			dims[0][1] = trace.endpos[0] - from[0];
		}

		VectorCopy(from, target);
		target[1] += 1000.0f;
		JP_Trace(&trace, from, playerMins, playerMaxs, target, -1, MASK_DEADSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
		if (trace.allsolid || trace.startsolid || trace.fraction == 1.0f || (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN))) {
			dims[1][0] = 0.0f;
		}
		else {
			dims[1][0] = trace.endpos[1] - from[1];
		}

		VectorCopy(from, target);
		target[1] -= 1000.0f;
		JP_Trace(&trace, from, playerMins, playerMaxs, target, -1, MASK_DEADSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
		if (trace.allsolid || trace.startsolid || trace.fraction == 1.0f || (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN))) {
			dims[1][1] = 0.0f;
		}
		else {
			dims[1][1] = trace.endpos[1] - from[1];
		}

		// decide the wider dimension
		longerdim = (dims[0][0] - dims[0][1]) > (dims[1][0] - dims[1][1]) ? 0 : 1;
		shorterdim = longerdim^1;

		VectorCopy(spot->s.origin, from);
		
		//for (i = 0; i < 2; i++) {
			//from[i] = from[i] + (dims[i][0] + dims[i][1]) * 0.5f; // center on both dimensions first
			// actually dont. if we are at the edge of a rectangle, we will blindly center ourselves into a random object inside that triangle
		//}

		from[longerdim] = from[longerdim] + (dims[longerdim][0] + dims[longerdim][1]) * 0.5f; // center ourselves on the longer dim 
				
		VectorCopy(from, spot->duelQueueSpawn.spawn1);
		VectorCopy(from, spot->duelQueueSpawn.spawn2);

		// now we spread out along the longer dim cuz we need two spawns;
		offCenter = MIN((dims[longerdim][0] - dims[longerdim][1]), DUELQUEUE_RESPAWNPOSITION_MINDISTANCE_SHORT) * 0.49f; // 0.5f would be ideal but let's make sure float imprecision won't bite us.

		if (offCenter < 50.0f) {
			// meh.
			continue;
		}

		spot->duelQueueSpawn.spawn1[longerdim] += offCenter;
		spot->duelQueueSpawn.spawn2[longerdim] -= offCenter;
		spot->duelQueueSpawn.floorarea = (dims[0][0] - dims[0][1]) * (dims[1][0] - dims[1][1]);
		spot->duelQueueSpawn.useable = qtrue;

		if (g_developer.integer) {
			G_Printf("Potential duel queue spawn info: #%d, %f area (%fx%f), spawns: %f %f %f, %f %f %f, from origin %f %f %f\n",
				(int)(spot-g_entities), spot->duelQueueSpawn.floorarea, (dims[0][0] - dims[0][1]), (dims[1][0] - dims[1][1]),
				spot->duelQueueSpawn.spawn1[0], spot->duelQueueSpawn.spawn1[1], spot->duelQueueSpawn.spawn1[2],
				spot->duelQueueSpawn.spawn2[0], spot->duelQueueSpawn.spawn2[1], spot->duelQueueSpawn.spawn2[2],
				spot->s.origin[0], spot->s.origin[1], spot->s.origin[2]
				);
		}
	}
}


/*
===========
SelectRandomFurthestDuelQueueSpawnPoint

Chooses a duel queue start away from other duelers.
============
*/
gentity_t* SelectRandomFurthestDuelQueueSpawnPoint(gentity_t* spawningEnt, vec3_t* existingDuelers, int existingDuelersCount, vec3_t origin, vec3_t angles) {
	gentity_t* spot;
	vec3_t		vecto,delta;
	float		dist;
	float		list_dist[64];
	gentity_t* list_spot[64];
	int			numSpots, rnd, i, j;
	float		existingDivider = 1.0f / (float)existingDuelersCount;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		if (SpotWouldTelefrag(spot->s.origin, spawningEnt)) {
			continue;
		}
		VectorClear(delta);
		for (i = 0; i < existingDuelersCount; i++) {
			VectorSubtract(spot->s.origin,existingDuelers[i], vecto);
			VectorAdd(delta, vecto, delta);
		}
		VectorScale(delta, existingDivider, delta); // the average distance to existing duelers is what matters.
		dist = VectorLength(delta);
		for (i = 0; i < numSpots; i++) {
			if (dist > list_dist[i]) {
				if (numSpots >= 64)
					numSpots = 64 - 1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j - 1];
					list_spot[j] = list_spot[j - 1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		spot = G_FindByClassNameFast(NULL, "info_player_deathmatch");
		if (!spot)
		{
			G_Error("Couldn't find a spawn point");
		}
		VectorCopy(spot->s.origin, origin);
		if (g_bubbleSpawn.integer && !(spawningEnt && spawningEnt->client && spawningEnt->client->sess.raceMode) && SpotWouldTelefrag(origin, spawningEnt)) {
			WiggleSpotTelefrag(origin, spawningEnt);
		}
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}
/*
===========
SelectRandomFurthestDuelQueueSpawnPointV2

Chooses a duel queue start away from other duelers.
New version that uses preprocessed duel start positions
============
*/
int sortspotsfloorarea(const void* a, const void* b) {
	gentity_t* spot1 = *(gentity_t**)a;
	gentity_t* spot2 = *(gentity_t**)b;
	return spot2->duelQueueSpawn.floorarea - spot1->duelQueueSpawn.floorarea; // player who won gets priority to keep playing
}
gentity_t* SelectRandomFurthestDuelQueueSpawnPointV2(gentity_t* spawningEnt, gentity_t* spawningEnt2, vec3_t* existingDuelers, int existingDuelersCount, vec3_t origin, vec3_t origin2) {
	gentity_t* spot;
	vec3_t		vecto,delta;
	float		dist;
	float		list_dist[64];
	gentity_t*	list_spot[64];
	int			numSpots, rnd, i, j;
	float		existingDivider = 1.0f / (float)existingDuelersCount;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		if (!spot->duelQueueSpawn.useable || SpotWouldTelefrag(spot->duelQueueSpawn.spawn1, spawningEnt) || SpotWouldTelefrag(spot->duelQueueSpawn.spawn2, spawningEnt2)) {
			continue;
		}
		VectorClear(delta);
		for (i = 0; i < existingDuelersCount; i++) {
			VectorSubtract(spot->s.origin,existingDuelers[i], vecto);
			VectorAdd(delta, vecto, delta);
		}
		VectorScale(delta, existingDivider, delta); // the average distance to existing duelers is what matters.
		dist = VectorLength(delta);
		for (i = 0; i < numSpots; i++) {
			if (dist > list_dist[i]) {
				if (numSpots >= 64)
					numSpots = 64 - 1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j - 1];
					list_spot[j] = list_spot[j - 1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		return NULL;
	}

	if (numSpots > 2) {
		// among those we would choose from, sort by floor area
		qsort(list_spot, numSpots / 2, sizeof(list_spot[0]), sortspotsfloorarea);
	}

	// they're ordered by distance to existing duelers now, but i'd like th


	// select a random spot from the spawn points furthest away
	// pick out of the remaining half of the half.
	// first half: sorted by distance to existing duelers.
	// second half: sorted by floor area.
	rnd = random() * (numSpots / 4);

	VectorCopy(list_spot[rnd]->duelQueueSpawn.spawn1, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->duelQueueSpawn.spawn2, origin2);
	origin2[2] += 9;

	return list_spot[rnd];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint (gentity_t* spawningEnt,vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	return SelectRandomFurthestSpawnPoint(spawningEnt,avoidPoint, origin, angles );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint(gentity_t* spawningEnt, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_FindByClassNameFast(spot, "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 && (!(spawningEnt->client && spawningEnt->client->sess.raceMode) || spot->spawnDefragPriority == level.highestDefragSpawnPriority)) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot->s.origin, spawningEnt) ) {
		return SelectSpawnPoint(spawningEnt,vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_SINK_TIME		45000

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		G_SetClassName(ent, "bodyque");
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
===============
InitPlayerStats
===============
*/
void InitPlayerStats(void) {
	int		i;
	gentity_t* ent;

	for (i = 0; i < MAX_CLIENTS; i++) {
		//if (g_defrag.integer) {
			ent = G_Spawn();
			G_SetClassName(ent, "playerstats");
			ent->neverFree = qtrue;
			ent->s.eType = ET_INVISIBLE;
			ent->s.clientNum = i;
			ent->s.modelGhoul2 = 15; // tell tommyternal cgame that this is a player stats object :)
			ent->r.svFlags |= SVF_BROADCAST;
			level.playerStats[i] = ent;
			trap_LinkEntity(ent);
		//}
		//else {
		//	level.playerStats[i] = 0;
		//}
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > BODY_SINK_TIME + 1500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;	
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;

	if (level.intermissiontime)
	{
		return;
	}

	if ( !ent || !ent->client ) return;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	if (ent->client->ps.eFlags & EF_DISINTEGRATION)
	{ //for now, just don't spawn a body if you got disint'd
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);
	body->s = ent->s;
	if (g_entHUDFields.integer) {
		body->s.generic1 = 0;
		body->s.trickedentindex3 = 0;
		body->s.trickedentindex4 = 0;
	}

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if (ent->client->ps.eFlags & EF_DISINTEGRATION)
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		body->s.weapon = WP_BLASTER; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.clientNum);

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT;

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_TIME;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;
	int			oldRoll;
	qboolean	strafebot = ent->client->sess.raceMode && (ent->client->sess.raceStyle.runFlags & RFL_BOT) || ent->client->sess.rollAngleInvalidated;

	if (strafebot) {
		oldRoll = ent->client->pers.cmd.angles[ROLL];
		ent->client->pers.cmd.angles[ROLL] = 0;
	}

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);

	if (strafebot) {
		ent->client->pers.cmd.angles[ROLL] = oldRoll;
	}
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	gentity_t	*tent;

	if (!ent->client->sess.raceMode) {
		CopyToBodyQue(ent);
	}

	if (gEscaping)
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
	}

	trap_UnlinkEntity (ent);
	ClientSpawn(ent);

	// add a teleportation effect
	if (!ent->client->sess.raceMode) {
		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = ent->s.clientNum;
	}
}

void ClientRespawn(gentity_t* ent) {

	//MaintainBodyQueue(ent);

	// i dont even know what this does :)
	if (gEscaping)// || g_gametype == GT_POWERDUEL) 
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		//AddTournamentQueue(ent->client);
		ClientSpawn(ent);
		//ent->client->iAmALoser = qtrue;
		return;
	}

	trap_UnlinkEntity(ent);


	ClientSpawn(ent);
}


/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, team_t team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( team_t team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int		len, colorlessLen;
	char	ch;
	char	*p;
	int		spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}

		// don't allow leading spaces
		if( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if( !*in ) {
				break;
			}

			// don't allow black in a name, period
			/*if( ColorIndex(*in) == 0 ) {
				in++;
				continue;
			}*/

			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if( ch == ' ' ) {
			spaces++;
			if( spaces > 3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "Padawan", outSize );
	}
}


#define CLANTAG_HASHSIZE 256
static hashEntry_t knownClanTags[] = {
	{"freedom"},{"cos"},{"pi"},{"oc9"},{"eos"},{"fos"},{"bbb"},{"dbd"},{"174"},{"pureness"},{"believers"},
	{"fou"},{"jof"},{"jofa"},{"gog"},{"jip"},{"gog"},{"rrr"},{"ft"},{"bdsm"},{"motf"},{"circus"},
	{"suffix"},{"defiance"},{"el"},{"d2w"},{"coz"},{"fod"},{"ah"},{"jotr"},{"ros"},
	{"ats"},{"lm"},{"sol"},{"wCw"},{"SL"},{"TFJ"},{"ColdThugz"},{"EC"},{"rj"},
	{"KR"},{"93o"},{"930"},{"dA"},{"nWo"},{"ez"},{"GotA"},{"CjS"},{"BOMS"},{"ca"},
	{"eot"},{"bulldozer"},{"WAR"},{"ToD"},{"TB"},{"SPQR"},{"SC"},{"R"},{"NA"},
	{"DARK"},{"NATO"},{"LoD"},{"g"},{"EU"},{"MAD"},{"E621"},{"AFK"},{"Hecc"},{"hvn"},
	{"LSS"},{"vvv"},{"il"},{"so"},{"sf"},{"rodia"},{"tnf"},{"tft"},{"gg"},{"Templar"},{"PlayboyZ"},{"PARADiGM"},
	{"LoC"},{"Lions"},{"lil"},{"LGBTQ"},{"i"},{"dts"},{"dOR"},{"TOR"},{"e"},{"JAWA"},{"Ub"},
};

static const int clanTagCount = sizeof(knownClanTags) / sizeof(knownClanTags[0]);

hashEntry_t* clanTagHashTable[CLANTAG_HASHSIZE] = { 0 };

void InitClanTagHashTable() {
	int i;
	int hash;
	static qboolean inited = qfalse;
	if (!inited) {
		for (i = 0; i < clanTagCount; i++) {
			hash = generateHashValue(knownClanTags[i].text, CLANTAG_HASHSIZE);
			knownClanTags[i].next = clanTagHashTable[hash];
			clanTagHashTable[hash] = &knownClanTags[i];
		}
		inited = qtrue;
	}
}

static qboolean CheckIsClanTag(const char* text) {
	int i;
	int hash;
	hashEntry_t* hashEntry;
	InitClanTagHashTable();
	hash = generateHashValue(text, CLANTAG_HASHSIZE);
	for (hashEntry = clanTagHashTable[hash]; hashEntry; hashEntry = hashEntry->next) {
		if (!Q_stricmp(text,hashEntry->text)) {
			return qtrue;
		}
	}
	return qfalse;
}

void ApplyNameTag(char* name, int bufferSize, nameTagType_t type) {
	char	tmp[MAX_NETNAME];
	char*	s = tmp;
	int		inlen=0;// = strlen(name);
	int		i,j;
	char*	shortest = NULL;
	int		shortestLen = INT_MAX;
	char*	clanTag = NULL;
	qboolean	shortestIsMaybeClanTag;
	qboolean	clanTagFound;
	char*	tmp2;
	int		tmpLen;
	char	lastLetter = '\0';
	int		pieceIndex = 0;
	char	*s2 = name;
	
	//Q_strncpyz(tmp, name, sizeof(tmp)); 
	while (*s2 && inlen<(sizeof(tmp)-1)) {
		if (*s2 == Q_COLOR_ESCAPE && *(s2+1)) { // strip colors so we don't pull apart names
			s2++;
		}
		else {
			*s = *s2;
			inlen++;
			s++;
		}
		s2++;
	}


	*s = '\0';
	s = tmp;

	// NULL out any non-letter or digit char
	while (*s) {
		if (*s == Q_COLOR_ESCAPE) {
			*(s++) = '\0';
			if (*s == '\0') {
				s--; // don't accidentally overflow if this is already the end of the string
			}
			else {
				*s = '\0';
			}
		}
		else if (!(*s >= 'a' && *s <= 'z' || *s >= 'A' && *s <= 'Z' || *s >= '0' && *s <= '9')) {
			*s = '\0';
		}
		else if (*s >= 'A' && *s <= 'Z') {
			*s = tolower(*s);
		}
		s++;
	}
	// find shortest bit
	for (i = 0; i < inlen; i++) {
		if (!lastLetter && tmp[i]) {
			tmp2 = &tmp[i];
			tmpLen = strlen(tmp2);
			clanTagFound = CheckIsClanTag(tmp2);
			if (clanTagFound && !clanTag) {
				clanTag = tmp2;
			}
			else if ((tmpLen < shortestLen || shortestIsMaybeClanTag) && tmpLen > 1 && !clanTagFound) {
				shortest = tmp2;
				shortestLen = tmpLen;
				shortestIsMaybeClanTag = pieceIndex == 0 && tmpLen >= 2 && tmpLen <= 3;
				pieceIndex++;
			}
			i += tmpLen;
		}
		lastLetter = tmp[i];
	}

	if (!shortest) {
		// can't apply nametag, no suitable bit found.
		if (!clanTag) {
			return;
		}
		else {
			shortest = clanTag;
		}
	}

	switch (type) {
	default:
		return;
		break;
	case NAMETAG_FREEDOM:
		Com_sprintf(name, bufferSize,"^7^7^4freedom^4^4^7#^7^7^4%s^4^4^7'",shortest);
		break;
	case NAMETAG_OC9:
		Com_sprintf(name, bufferSize, "^5oc9^7#^5%s", shortest);
		break;
	}

}

#ifdef _DEBUG
void G_DebugWrite(const char *path, const char *text)
{
	fileHandle_t f;

	trap_FS_FOpenFile( path, &f, FS_APPEND );
	trap_FS_Write(text, strlen(text), f);
	trap_FS_FCloseFile(f);
}
#endif

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side 
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally 
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void *g2SaberInstance = NULL;
void SetupGameGhoul2Model(gclient_t *client, char *modelname)
{
	int handle;
	char		afilename[MAX_QPATH];
	char		/**GLAName,*/ *slash;
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};
	int	nowTime = LEVELTIME(client);

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (client->ghoul2 && trap_G2_HaveWeGhoul2Models(client->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&(client->ghoul2));
	}

	/*
	Com_sprintf( afilename, sizeof( afilename ), "models/players/%s/model.glm", modelname );
	handle = trap_G2API_InitGhoul2Model(&client->ghoul2, afilename, 0, 0, -20, 0, 0);
	if (handle<0)
	{
		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&client->ghoul2, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}
	}
	*/

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}
	}

	if (precachedKyle && trap_G2_HaveWeGhoul2Models(precachedKyle))
	{
		trap_G2API_DuplicateGhoul2Instance(precachedKyle, &client->ghoul2);
	}
	else
	{
		return;
	}

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		//get the location of the animation.cfg
		//GLAName = trap_G2API_GetGLAName( client->ghoul2, 0);
		trap_G2API_GetGLAName( client->ghoul2, 0, GLAName);

		if (!GLAName[0])
		{
			if (!BG_ParseAnimationFile("models/players/_humanoid/animation.cfg"))
			{
				Com_Printf( "Failed to load animation file %s\n", afilename );
				return;
			}
			return;
		}
		Q_strncpyz( afilename, GLAName, sizeof( afilename ));
		slash = Q_strrchr( afilename, '/' );
		if ( slash )
		{
			Q_strncpyz(slash, "/animation.cfg",sizeof(afilename) - (slash-afilename));
		}	// Now afilename holds just the path to the animation.cfg
		else 
		{	// Didn't find any slashes, this is a raw filename right in base (whish isn't a good thing)
			return;
		}

		// Try to load the animation.cfg for this model then.
		if ( !BG_ParseAnimationFile( afilename ) )
		{	// The GLA's animations failed
			if (!BG_ParseAnimationFile("models/players/_humanoid/animation.cfg"))
			{
				Com_Printf( "Failed to load animation file %s\n", afilename );
				return;
			}
		}
	}

	trap_G2API_AddBolt(client->ghoul2, 0, "*r_hand");
	trap_G2API_AddBolt(client->ghoul2, 0, "*l_hand");

	// NOTE - ensure this sequence of bolt and bone accessing are always the same because the client expects them in a certain order
	trap_G2API_SetBoneAnim(client->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
	trap_G2API_SetBoneAngles(client->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
	trap_G2API_SetBoneAngles(client->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time); // not using nowTime here because using it on G2 made the server have extreme hitches and idk the cause

	if (!g2SaberInstance)
	{
		trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

		if (g2SaberInstance)
		{
			// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
			trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
			// now set up the gun bolt on it
			trap_G2API_AddBolt(g2SaberInstance, 0, "*flash");
		}
	}

	if (g2SaberInstance)
	{
		trap_G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, client->ghoul2, 1); 
	}
}

extern void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);

static void AcceptClientPhysicsFps(gentity_t* ent, int clientSetting) {
	gclient_t* client = ent->client;
	client->pers.physicsFps.lastChange = level.time;
	client->pers.physicsFps.acceptedSetting = clientSetting;
	client->pers.physicsFps.acceptedSettingMsec = (MAX(1, MIN(1000, 1000 / MAX(1, clientSetting))));
	if (client->sess.raceStyle.msec >= 0) { // -1 = toggle, -2 = float
		client->sess.raceStyle.msec = client->pers.physicsFps.acceptedSettingMsec;
		DF_RaceStateInvalidated(ent, qtrue);
	}
}

static qboolean ValidateClientPhysicsFps(gclient_t* client, int clientSetting) {
	// Do validation of the client com_physicsFps setting here.
	// For example check if the value he set is sensible and allowed by the game settings.
	// Return qfalse if invalid.

	if (client->sess.raceMode) {
		// TODO What if someone uses this to get a setting "validated" but then switches out of racemode? catch that more elegantly?
		return clientSetting > 0 && clientSetting <=1000; // racemode allows all (just doesnt allow toggle outside toggle mode)
	}

	if (g_fixHighFPSAbuse.integer && (clientSetting >= 250 || clientSetting < 40)) {
		return qfalse;
	}

	return qtrue;
}

/*
===========
HandleClientPhysicsFps

Handle com_physicsFps setting of clients.

If g_fpsToggleDelay is enabled, we limit fps toggling by clients by only allowing 
a change in the client's fps setting every X seconds (set by g_fpsToggleDelay)
============
*/
void SetClientPhysicsFps(gentity_t* ent, int clientSetting) {
	gclient_t* client = ent->client;
	
	if (!ent->client) return;
	client->pers.physicsFps.clientSetting = clientSetting;
	client->pers.physicsFps.clientSettingValid = ValidateClientPhysicsFps(client,clientSetting);

	if (!client->pers.physicsFps.clientSettingValid) {
		// Tried to set an invalid setting
		return;
	}

	if (client->pers.physicsFps.acceptedSetting == clientSetting && (!client->sess.raceMode || client->sess.raceStyle.msec < 0 || client->sess.raceStyle.msec == client->pers.physicsFps.acceptedSettingMsec)) {
		// Don't care, nothing changed
		return;
	}

	// quick check for situations that are always ok
	if (client->sess.raceMode) {
		if (client->sess.raceStyle.msec < 0) { // float or toggle mode. doesnt matter then
			// Toggle limiting disabled, or no value accepted yet. Just accept.
			AcceptClientPhysicsFps(ent, clientSetting);
			return;
		}
	}
	else {
		if (!g_fpsToggleDelay.integer || !client->pers.physicsFps.acceptedSetting) {
			// Toggle limiting disabled, or no value accepted yet. Just accept.
			AcceptClientPhysicsFps(ent, clientSetting);
			return;
		}
	}

	if (!clientSetting) {
		// Client has it disabled. Don't do anything.
		return;
	}

	// mode specific checks.
	if (client->sess.raceMode) {
		if (!client->pers.raceStartCommandTime) {
			// Change allowed.
			// Client is not currently in a run.
			AcceptClientPhysicsFps(ent, clientSetting);
		}
	}
	else {
		if ((client->pers.physicsFps.lastChange + g_fpsToggleDelay.integer * 1000) < level.time || client->pers.physicsFps.lastChange > level.time) {
			// Change allowed.
			// Either the minimum time delay has passed or level.time has been reset
			AcceptClientPhysicsFps(ent, clientSetting);
		}
	}
}



void NameDedupe_SanitizeString(char* in, char* out) {
	while (*in) {
		if (*in == 94) {
			in += 2;		// skip color code
			continue;
		}
		if (*in < 32) {
			in++;
			continue;
		}
		*out++ = tolower(*in++);
	}

	*out = 0;
}

void G_SetModelColor(char color[9], const char *userinfo)
{
	byte serverColor[4];
	char clientColor[4][4];

	Q_strncpyz(clientColor[0], Info_ValueForKey(userinfo, "char_color_red"), sizeof(clientColor[0]));
	Q_strncpyz(clientColor[1], Info_ValueForKey(userinfo, "char_color_green"), sizeof(clientColor[1]));
	Q_strncpyz(clientColor[2], Info_ValueForKey(userinfo, "char_color_blue"), sizeof(clientColor[2]));
	Q_strncpyz(clientColor[3], Info_ValueForKey(userinfo, "char_color_alpha"), sizeof(clientColor[3]));

	if (clientColor[0][0] == '\0' || clientColor[1][0] == '\0' || clientColor[2][0] == '\0' || clientColor[3][0] == '\0')
	{
		serverColor[0] = 255;
		serverColor[1] = 255;
		serverColor[2] = 255;
		serverColor[3] = 255;
	}
	else
	{
		serverColor[0] = atoi(clientColor[0]);
		serverColor[1] = atoi(clientColor[1]);
		serverColor[2] = atoi(clientColor[2]);
		serverColor[3] = atoi(clientColor[3]);
	}

	Q_strncpyz(color, colorToHex(serverColor), 9);
}

void G_SetSaberName(char saberName[MAX_QPATH], const char *userinfo)
{
	const char *serverSaberName;
	const char *clientSaberName;

	clientSaberName = Info_ValueForKey(userinfo, "saber1");

	if (clientSaberName[0] == '\0')
	{
		serverSaberName = DEFAULT_SABER1;
	}
	else
	{
		serverSaberName = clientSaberName;
	}

	Q_strncpyz(saberName, serverSaberName, MAX_QPATH);
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	char	*s;
	char	model[MAX_QPATH];
	char	modelColor[9];
	char	saberName[MAX_QPATH];
	//char	headModel[MAX_QPATH];
	char	forcePowers[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	char	oldnameNoModeTeam[MAX_STRING_CHARS];
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	char	redTeam[MAX_INFO_STRING];
	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
	char	modeTeamString[MAX_INFO_STRING];
	int		modeTeamPrefixLength = 0;
	modeTeam_t* modeTeamData;

	// NameCrashFix (whitelisted characters)
	static const char	validChars[]  = " ~QqWwEeRrTtYyUuIiOoPpAaSsDdFfGgHhJjKkLlZzXxCcVvBbNnMm1234567890<>?,./';:][{}`-=!@#$^&*()_+|";
	int					i, j, isValidChar;
	char				*ptr;
	const char*			namekey = "name";

	ent = g_entities + clientNum;
	client = ent->client;

	modeTeamData = &modeTeams[ent->client->sess.modeTeam];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		Q_strncpyz (userinfo, "\\name\\badinfo",sizeof(userinfo));
	}

	if (coolApi & COOL_APIFEATURE_CLIENTREALNAME) {
		if (Info_HasKey(userinfo, "ttrn")) {
			namekey = "ttrn";
		}
		else {
			Com_Printf("^3ClientUserinfoChanged: real name API supported but client %d ttrn key not found. defaulting to normal behavior\n",clientNum);
		}
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->pers.localClient = qtrue;
	}

	// check for TAS client (any client that does any custom hacks/scripting/whatever for defrag)
	s = Info_ValueForKey( userinfo, "tasClient" );
	client->pers.tasClient = atoi(s);

	s = Info_ValueForKey( userinfo, "ttClFl" ); // tommyTernal clientFlags
	client->pers.ttClientFlags = atoi(s);

	// check if it's a demo bot
	s = Info_ValueForKey(userinfo, "engine");
	if (!Q_stricmpn(s, "jkclient", 8)) {
		client->pers.isHeadlessClient = qtrue;
	}
	else {
		client->pers.isHeadlessClient = qfalse;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// check for com_physicsFps setting
	s = Info_ValueForKey( userinfo, "com_physicsFps" );
	client->pers.physicsFps.clientSendsPhysicsFps = *s != '\0';
	if ( atoi( s ) ) {
		SetClientPhysicsFps(ent, atoi(s));
	} else {
		SetClientPhysicsFps(ent, 0);
	}

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	Q_strncpyz (oldnameNoModeTeam, client->pers.netnameNoModeTeam, sizeof(oldnameNoModeTeam) );
	s = Info_ValueForKey (userinfo, namekey);
		
	// NameCrashFix
	for ( i = 0; i < (int)strlen(s); i++ )
	{
		isValidChar = 0;

		for ( j = 0; validChars[j]; j++ )
		{
			if ( s[i] == validChars[j] ) isValidChar = 1; //The char is on the whitelist - it's a valid Char...
		}

		if ( !isValidChar )	s[i] = '.';
	}
	
	// Don't let players use @@@ in their names (multi-language strings)
	ptr = strstr( s, "@@@" );
	while ( ptr )
	{
		memset( ptr, '.', 3 );
		ptr = strstr( s, "@@@" );
	}

	ClientCleanName( s, ent->client->pers.netname, sizeof(ent->client->pers.netname) );

	if (client->sess.amflags & AMFLAG_LOCKEDNAME && oldname[0])	//do these checks for map_restarts where the locked name wont survive
		Q_strncpyz(client->pers.netname, oldname, sizeof(client->pers.netname));

	Info_RemoveKey( userinfo, "name" );
	Info_SetValueForKey( userinfo, "name", ent->client->pers.netname );
	trap_SetUserinfo( clientNum, userinfo );

	if (ent->client->sess.nameTag > 0 && ent->client->sess.nameTag < NAMETAG_COUNT) {
		ApplyNameTag(ent->client->pers.netname, sizeof(ent->client->pers.netname), ent->client->sess.nameTag);
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	// thanks to anonymous donor
	if (!g_allowNameDupes.integer)
	{
		int i;
		char		temp[64];
		char		temp2[64];
		client->pers.nameNumber = 0;

		for (i = strlen(client->pers.netname) - 1; i >= 0; i--)
		{
			if (client->pers.netname[i] == ' ')
				client->pers.netname[i] = '\0';
			else
				break;
		}

		NameDedupe_SanitizeString(client->pers.netname, temp);
		Q_strncpyz(client->pers.wantedNameColor, client->pers.netname, sizeof(client->pers.wantedNameColor));
		Q_strncpyz(client->pers.wantedNameBlank, temp, sizeof(client->pers.wantedNameBlank));

		if (strlen(client->pers.netname) < sizeof(client->pers.netname) - 11)
		{
			for (i = 0; i < level.maxclients; i++)
			{
				if (g_entities[i].client == NULL)
					continue;
				if (!g_entities[i].inuse) {
					continue;
				}
				if (i == clientNum)
					continue;
				if (!Q_stricmp(g_entities[i].client->pers.wantedNameBlank, client->pers.wantedNameBlank))
				{
					if (g_entities[i].client->pers.nameNumber == client->pers.nameNumber)
					{
						client->pers.nameNumber = client->pers.nameNumber + 1;
						i = 0;
					}
				}
				NameDedupe_SanitizeString(client->pers.netname, temp);
				NameDedupe_SanitizeString(g_entities[i].client->pers.netnameNoModeTeam, temp2);
				if (!Q_stricmp(temp2, temp))
				{
					client->pers.nameNumber = client->pers.nameNumber + 1;
					i = 0;
				}
				if (client->pers.nameNumber)
				{
					Q_strncpyz(temp, client->pers.wantedNameColor, sizeof(temp));
					Q_strcat(temp, sizeof(temp), va("^7[^2%i^7]", client->pers.nameNumber));
					Q_strncpyz(client->pers.netname, temp, sizeof(client->pers.netname));
				}
			}
		}


	}

	Q_strncpyz(client->pers.netnameNoModeTeam, client->pers.netname, sizeof(client->pers.netnameNoModeTeam));

	Q_strncpyz(client->pers.netnameClean, client->pers.netname, sizeof(client->pers.netnameClean));
	Q_CleanStr(client->pers.netnameClean, qtrue, qtrue);

	if (modeTeamData->applyPrefix && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		const char* prefix = multiva("^%c[%s]^7 ", modeTeamData->teamPrefixColor, modeTeamData->shortname);
		modeTeamPrefixLength = strlen(prefix);
		Q_strncpyz(ent->client->pers.netname, va("%s%s", prefix, ent->client->pers.netname), sizeof(ent->client->pers.netname));
	}

	// trying to not show the rename on modeteam changes, cuz why spam.
	if (client->pers.connected == CON_CONNECTED && strcmp( oldnameNoModeTeam, client->pers.netnameNoModeTeam)) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname, G_GetStripEdString("SVINGAME", "PLRENAME"),
			client->pers.netname) );
	}

	// set max health
	health = 100; //atoi( Info_ValueForKey( userinfo, "handicap" ) );
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	if( g_gametype.integer >= GT_TEAM ) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "team_model"), sizeof( model ) );
		//Q_strncpyz( headModel, Info_ValueForKey (userinfo, "team_headmodel"), sizeof( headModel ) );
	} else {
		Q_strncpyz(model, Info_ValueForKey(userinfo, "model"), sizeof(model));
		if (modeTeamData->applyTeamColors && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
			if (Q_stricmpn(model, "jedi_", 5) && strchr(model, '|') == NULL) { // don't override multipart models
				char* slash = strchr(model, '/');
				const char* targetSkin = "/default";
				switch (modeTeamData->realTeam) {
					case TEAM_RED:
						targetSkin = "/red";
						break;
					case TEAM_BLUE:
						targetSkin = "/blue";
						break;
				}
				if (slash) {
					*slash = '\0';
				}
				Q_strcat(model, sizeof(model), targetSkin);
			}
		}
		//Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );
	}
	
	// GalakingFix
	if ( g_mv_fixgalaking.integer && (!Q_stricmp(model, "galak_mech") || !Q_stricmpn(model, "galak_mech/", strlen("galak_mech/"))) )
	{
		Q_strncpyz( model, "galak/default", sizeof(model) );
	}
	
	if ( g_mv_fixbrokenmodels.integer && (!Q_stricmpn(model, "kyle/fpls", strlen("kyle/fpls")) || !Q_stricmp(model, "morgan") || (!Q_stricmpn(model, "morgan/", strlen("morgan/")) && (Q_stricmp(model, "morgan/default_mp") && Q_stricmp(model, "morgan/red") && Q_stricmp(model, "morgan/blue")))) )
	{
		Q_strncpyz( model, "kyle/default", sizeof(model) );
	}

	// model color
	if (modeTeamData->applyTeamColors && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		switch (modeTeamData->realTeam) {
			default:
			case TEAM_FREE:
				Q_strncpyz(modelColor, "FFFFFFFF", sizeof(modelColor));
				break;
			case TEAM_BLUE:
				Q_strncpyz(modelColor, "0000FFFF", sizeof(modelColor));
				break;
			case TEAM_RED:
				Q_strncpyz(modelColor, "FF0000FF", sizeof(modelColor));
				break;
		}
	}
	else {
		G_SetModelColor(modelColor, userinfo);
	}

	// saber name
	G_SetSaberName(saberName, userinfo);

	Q_strncpyz( forcePowers, Info_ValueForKey (userinfo, "forcepowers"), sizeof( forcePowers ) );

	team = client->sess.sessionTeam;

/*	NOTE: all client side now

	// team
	switch( team ) {
	case TEAM_RED:
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
		break;
	case TEAM_BLUE:
		ForceClientSkin(client, model, "blue");
//		ForceClientSkin(client, headModel, "blue");
		break;
	}
	// don't ever use a default skin in teamplay, it would just waste memory
	// however bots will always join a team but they spawn in as spectator
	if ( g_gametype.integer >= GT_TEAM && team == TEAM_SPECTATOR) {
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
	}
*/

	if (g_gametype.integer >= GT_TEAM) {
		client->pers.teamInfo = qtrue;
	} else {
		int _atoi;
		s = Info_ValueForKey(userinfo, "teamoverlay");

		_atoi = atoi(s);
		if (_atoi == 2)
			client->pers.teamInfo = 2;
		else if (!*s || _atoi != 0)
			client->pers.teamInfo = 1;
		else
			client->pers.teamInfo = 0;
	}
	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if ( !*s || atoi( s ) == 0 ) {
		client->pers.pmoveFixed = qfalse;
	}
	else {
		client->pers.pmoveFixed = qtrue;
	}
	*/

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	Q_strncpyz(c2, Info_ValueForKey( userinfo, "color2" ),sizeof(c2));

	if (modeTeamData->applyTeamColors && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		switch (modeTeamData->realTeam) {
		default:
		case TEAM_FREE:
			Q_strncpyz(c1, "3", sizeof(c1));
			break;
		case TEAM_RED:
			Q_strncpyz(c1, "0", sizeof(c1));
			break;
		case TEAM_BLUE:
			Q_strncpyz(c1, "4", sizeof(c1));
			break;
		}
	}
	else {
		Q_strncpyz(c1, Info_ValueForKey(userinfo, "color1"), sizeof(c1));
	}

	Q_strncpyz(redTeam, Info_ValueForKey( userinfo, "g_redteam" ),sizeof(redTeam));
	Q_strncpyz(blueTeam, Info_ValueForKey( userinfo, "g_blueteam" ),sizeof(blueTeam));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d\\mvgp\\%i\\bot\\1\\mc\\%s\\st\\%s",
			client->pers.netname, team, model,  c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader, jk2gameplay, modelColor, saberName );
	} else {
		if (client->sess.modeTeam > MODE_NORMAL) {
			Com_sprintf(modeTeamString, sizeof(modeTeamString), "%d/%d/%d/%d/%d/%s/%s", client->sess.modeTeam, modeTeamData->applyTeamColors, modeTeamData->realTeam, modeTeamData->friendlyTeam, modeTeamPrefixLength, modeTeamData->scoreHexColor, modeTeamData->name);
		}
		else {
			modeTeamString[0] = '\0';
		}
		s = va("n\\%s\\un\\%s\\t\\%i\\model\\%s\\g_redteam\\%s\\g_blueteam\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\mvgp\\%i\\jkrace\\%i\\mode\\%i\\mote\\%s\\mc\\%s\\st\\%s\\tas\\%d",
			client->pers.netname, client->sess.login.name, client->sess.sessionTeam, model, redTeam, blueTeam, c1, c2,
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, jk2gameplay, client->pers.raceBestTime, client->sess.mode, modeTeamString, modelColor, saberName, client->pers.tasClient);
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	if (g_logClientInfo.integer)
	{
		G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
	}
}

// super simplified versin of clientuserinfochanged, allowing for faster toggle
qboolean ClientPhysicsFpsChanged( int clientNum ) {
	gentity_t *ent;
	gclient_t* client;
	char	*s;
	char	userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for com_physicsFps setting
	s = Info_ValueForKey( userinfo, "com_physicsFps" );
	client->pers.physicsFps.clientSendsPhysicsFps = *s != '\0';
	if ( atoi( s ) ) {
		SetClientPhysicsFps(ent, atoi(s));
	} else {
		SetClientPhysicsFps(ent, 0);
	}
	
	return qtrue;
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
qboolean MV_SetClientIP( int clientNum, char *value )
{
	mvclientSession_t *mvSess = &mv_clientSessions[clientNum];
	int i, j = 0, step = 0;
	char temp[4];

	if ( !value || !strlen(value) ) return qfalse;
	
	memset( temp, 0, sizeof(temp) );

	for ( i = 0; i < (int)strlen(value); i++ )
	{
		if ( value[i] == '.' || (value[i] == ':' && step == 3) )
		{
			mvSess->clientIP[step] = atoi(temp);

			memset( temp, 0, sizeof(temp) );
			step++;
			j = 0;

			if ( step == 4 ) break;
		}
		else if ( value[i] >= '0' && value[i] <= '9' && j < (int)sizeof(temp)-1 )
		{
			temp[j] = value[i];
			j++;
		}
		else return qfalse;
	}
	mvSess->localClient = qfalse;
	return qtrue;
}

extern void DF_ClearCheckPointTimes(gentity_t* playerent);

extern int GetDefaultPlayerMode(qboolean allowDefrag);
void ClientSetDefaultMode(gentity_t* ent, qboolean allowDefrag);

char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;
	gentity_t	*te;
	mvclientSession_t *mvSess = &mv_clientSessions[clientNum];

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if ( G_FilterPacket( value ) ) {
		return "Banned.";
	}

	memset( &userCmdBuffer[clientNum],0,sizeof(userCmdBuffer[clientNum]));
	memset( &mv_clientSessions[clientNum], 0, sizeof(mv_clientSessions[clientNum]) );
	G_ResetUserCmdStore(ent - g_entities);
	if ( (ent->r.svFlags & SVF_BOT) || isBot || !Q_stricmp(value, "localhost") )
	{ // Bots and localhost get 127.0.0.1
		mvSess->clientIP[0] = 127;
		mvSess->clientIP[1] = 0;
		mvSess->clientIP[2] = 0;
		mvSess->clientIP[3] = 1;

		mvSess->localClient = qtrue;
	}
	else if ( firstTime && !MV_SetClientIP( clientNum, value ) ) return "Please wait...";

	if ( !firstTime ) MV_ReadSessionData( clientNum ); // If this isn't a "firstTime" read the stored IPs...
	if ( mvSess->clientIP[0] == 0 && mvSess->clientIP[1] == 0 && mvSess->clientIP[2] == 0 && mvSess->clientIP[3] == 0 ) return "was dropped due to an internal error."; // Should never happen, but just in case...
	
	if ( g_connectionlimit.integer && firstTime && !mvSess->localClient )
	{
		mvclientSession_t	*mvSessOther;
		gentity_t			*other;
		int					sameip;
		int					i;

		sameip = 0;

		for ( i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( i == clientNum ) continue;

			other = &g_entities[i];
			mvSessOther = &mv_clientSessions[i];

			if ( other && other->client && (other->client->pers.connected == CON_CONNECTING || other->client->pers.connected == CON_CONNECTED || other->client->pers.connected != CON_DISCONNECTED)/*&& other->inuse*/ )
			{
				if ( ((mvSessOther->clientIP[0] == mvSess->clientIP[0]) && (mvSessOther->clientIP[1] == mvSess->clientIP[1]) && (mvSessOther->clientIP[2] == mvSess->clientIP[2]) && (mvSessOther->clientIP[3] == mvSess->clientIP[3])))
				{
					sameip++;
				}
			}
		}
		if ( sameip >= g_connectionlimit.integer )
		{
			return "Too many connections from your IP.";
		}
	}

	if ( g_connectinglimit.integer != 0 && firstTime && !mvSess->localClient )
	{
		mvclientSession_t	*mvSessOther;
		gentity_t			*other;
		int					alreadyConnecting;
		int					i;

		alreadyConnecting = 0;

		for ( i = 0; i < MAX_CLIENTS; i++ )
		{
			if ( i == clientNum ) continue;

			other = &g_entities[i];
			mvSessOther = &mv_clientSessions[i];

			if ( other && other->client && other->client->pers.connected == CON_CONNECTING )
			{
				if ( ((mvSessOther->clientIP[0] == mvSess->clientIP[0]) && (mvSessOther->clientIP[1] == mvSess->clientIP[1]) && (mvSessOther->clientIP[2] == mvSess->clientIP[2]) && (mvSessOther->clientIP[3] == mvSess->clientIP[3])) )
				{
					alreadyConnecting++;
				}
			}
		}
		if ( alreadyConnecting >= g_connectinglimit.integer )
		{
			return "Too many players from your IP are trying to connect at the same time.";
		}
	}

	if ( !( ent->r.svFlags & SVF_BOT ) && !isBot && g_needpass.integer ) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			static char sTemp[1024];
			Q_strncpyz(sTemp, G_GetStripEdString("SVINGAME","INVALID_PASSWORD"), sizeof (sTemp) );
			return sTemp;// return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	client->pers.connectTime = level.time;
	client->pers.connected = CON_CONNECTING;
	if (firstTime) { // if map changed and he is auto-"connected" back, don't signal him as not afk.
		client->sess.lastHereTime = level.time;
	}

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo, isBot );
	}
	G_ReadSessionData( client );

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
		if (ent->client->sess.mode == MODE_DEFRAG) {
			// bots cant race
			ClientSetDefaultMode(ent, qfalse);
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStripEdString("SVINGAME", "PLCONNECT")) );
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	te = G_TempEntity( vec3_origin, EV_CLIENTJOIN );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = clientNum;

	memset( &ent->client->ps, 0, sizeof(ent->client->ps) ); // Make sure we always use a fresh playerState for new clients (this allows us to check for powerups in the playerState to prevent flagEating when calling ClientBegin)
	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	DF_ClearCheckPointTimes(ent);

	return NULL;
}

void G_WriteClientSessionData( gclient_t *client );
//void DF_SetRaceMode(gentity_t* ent, qboolean value);
void ResetClientModeIfInvalid(gentity_t* ent, qboolean allowDefrag);
/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags, i;
	char		userinfo[MAX_INFO_VALUE], *modelname;
	int			nowTime;

	ent = g_entities + clientNum;
	nowTime = LEVELTIME(ent->client); // TODO does that even make sense here? idk.
	
	// FlagEatingFix - We must ensure that powerups are cleared on ClientConnect and before team changes. Otherwise we might accidently trigger a flag duplication here.
	for ( i = PW_REDFLAG; i <= PW_NEUTRALFLAG; i++ )
	{
		if ( ent->client->ps.powerups[i] )
		{
			gitem_t		*item;
			gentity_t	*drop;

			item = BG_FindItemForPowerup( i );
			if ( item )
			{
				drop = Drop_Item( ent, item, 45, 0 );
				// decide how many seconds it has left
				drop->count = ( ent->client->ps.powerups[ i ] - nowTime ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				//if ( drop->count > 1000 ) { // in case of any weird confusion with nowTime?
				//	drop->count = 1000;
				//}
			}

			ent->client->ps.powerups[i] = 0;
		}
	}

	if ((ent->r.svFlags & SVF_BOT) && g_gametype.integer >= GT_TEAM)
	{
		if (allowTeamReset && g_botTeamAutoBalance.integer)
		{
			const char *team = "Red";
			int preSess;

			if ( !ent->client->pers.botDelayed )
			{ // Delay bots until all clients are connected
				ent->client->pers.botDelayed = qtrue;
				return;
			}

			//SetTeam(ent, "");
			ent->client->sess.sessionTeam = PickTeam(clientNum);
			trap_GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				ent->client->sess.sessionTeam = TEAM_RED;
			}

			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				team = "Red";
			}
			else
			{
				team = "Blue";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap_SetUserinfo( clientNum, userinfo );

			ent->client->ps.persistant[ PERS_TEAM ] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData( ent->client );
			ent->client->sess.sessionTeam = preSess;
			G_WriteClientSessionData(ent->client);
			ClientUserinfoChanged( clientNum );
			ClientBegin(clientNum, qfalse);
			return;
		}
	}

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	if (!client->sess.firstEnterTimeSet) {
		client->sess.firstEnterTime = level.time;
		client->sess.firstEnterTimeSet = qtrue;
	}
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;

	DeathmatchScoreboardMessage(ent);		//add so you get fresh scores once joining/changing teams.

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	//init saber ent
	WP_SaberInitBladeData( ent );

	// First time model setup for that player.
	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	modelname = Info_ValueForKey (userinfo, "model");
	SetupGameGhoul2Model(client, modelname);

	if (ent->client->ghoul2)
	{
		ent->bolt_Head = trap_G2API_AddBolt(ent->client->ghoul2, 0, "cranium");
		ent->bolt_Waist = trap_G2API_AddBolt(ent->client->ghoul2, 0, "thoracic");
		ent->bolt_LArm = trap_G2API_AddBolt(ent->client->ghoul2, 0, "lradius");
		ent->bolt_RArm = trap_G2API_AddBolt(ent->client->ghoul2, 0, "rradius");
		ent->bolt_LLeg = trap_G2API_AddBolt(ent->client->ghoul2, 0, "ltibia");
		ent->bolt_RLeg = trap_G2API_AddBolt(ent->client->ghoul2, 0, "rtibia");
		ent->bolt_Motion = trap_G2API_AddBolt(ent->client->ghoul2, 0, "Motion");
	}

	//if (client->sess.raceMode && !g_defrag.integer) {
	//	client->sess.raceMode = g_defrag.integer;
	//	Cmd_ForceChanged_f(ent);
	//} else {
	//	client->sess.raceMode = g_defrag.integer;
	//}
	//DF_SetRaceMode(ent,g_defrag.integer);
	ResetClientModeIfInvalid(ent,(qboolean)!(ent->r.svFlags& SVF_BOT));

	//if (client->sess.raceMode)
	//	client->ps.stats[STAT_RACEMODE] = 1;
	//else
	//	client->ps.stats[STAT_RACEMODE] = 0;

	//client->ps.stats[STAT_MOVEMENTSTYLE] = client->sess.raceStyle.movementStyle;
	//client->ps.stats[STAT_RUNFLAGS] = client->sess.raceStyle.runFlags;
	UpdateClientRaceVars(client);

	// locate ent at a spawn point
	G_GetUserCmd(client - level.clients, &ent->client->pers.cmd, GETUSERCMD_NOADVANCE); // make sure LEVELTIME() inside ClientSpawn gets a valid serverTime value
	ClientSpawn( ent );

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// send event
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;

		if ( g_gametype.integer != GT_TOURNAMENT  ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStripEdString("SVINGAME", "PLENTER")) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	G_ClearClientLog(clientNum);

	trap_SendServerCommand(clientNum, "print \"Welcome. Type ^2/help^7 for info about commands and how to play.\n\"");

	G_SendPlayerMapRatingsUIInfo(ent);
}

static qboolean AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

qboolean G_CheckForNearbyDuelSpawn(gentity_t* ent, vec3_t opponentOrigin, vec3_t spawn_origin, vec3_t spawn_angles) {
	int				i;
	vec3_t			delta;
	//float normalSpawnDist; // wanted to check if normal spawn dist is closer but that might be too simplistic for complex level architectures
	float			currentDist;
	trace_t			trace;
	qboolean		good;
	vec3_t			goodOrigin;

	int side, front, up, dist, skipvis;
	float traceDist = DUELQUEUE_RESPAWNPOSITION_MINDISTANCE_SHORT * 2.0f;
	float fracRequired = 0.4;
	good = qfalse;
	VectorCopy(opponentOrigin, goodOrigin);
	// we might spawn right on the opponents's ass
	// try to move us a bit away if we can?
	for (skipvis = 0; skipvis < 2 && !good; skipvis++) { // in emergency, dont require visual contact to opponent
		for (dist = 0; dist < 2 && !good; dist++) { // try shorter distance if nothing fouund
			if (dist == 1) {
				traceDist = DUELQUEUE_RESPAWNPOSITION_MINDISTANCE_SHORT;
				fracRequired = 0.8f;
			}
			for (up = 0; up < 2 && !good; up++) {
				for (side = -1; side < 2 && !good; side++) {
					for (front = -1; front < 2 && !good; front++) {
						if (side == 0 && front == 0) {
							continue;
						}
						goodOrigin[0] = opponentOrigin[0] + (float)front * traceDist;
						goodOrigin[1] = opponentOrigin[1] + (float)side * traceDist;
						goodOrigin[2] = opponentOrigin[2] + (float)up * 64.0f;
						//if (WiggleSpotTelefrag(goodOrigin, ent)) {

						if (skipvis) {
							if (WiggleSpotTelefrag(goodOrigin, ent)) {
								good = qtrue;
								break;
							}
						}
						else {
							JP_Trace(&trace, level.ironManCurrentPosition, playerMins, playerMaxs, goodOrigin, level.ironManClientNum, MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
							// make sure we could actually reach the capper from that place
							if (!trace.allsolid && !trace.startsolid && !(trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN)) && trace.fraction > fracRequired) { // let's be at least 0.6*min distance away
								// trace back in other direction (due to patches/1-way clips only being recognized in one direction)
								VectorCopy(trace.endpos, goodOrigin);
								JP_Trace(&trace, goodOrigin, playerMins, playerMaxs, level.ironManCurrentPosition, level.ironManClientNum, MASK_PLAYERSOLID);
								if (trace.fraction == 1.0f) {
									if (WiggleSpotTelefrag(goodOrigin, ent)) {
										good = qtrue;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (!good) {
		VectorCopy(opponentOrigin, goodOrigin);
		good = WiggleSpotTelefrag(goodOrigin, ent);
	}



	if (good) {
		// ok found a good pos
		VectorSubtract(level.ironManCurrentPosition, goodOrigin, delta);
		VectorNormalize(delta);
		vectoangles(delta, spawn_angles); // look at the iron man
		spawn_angles[ROLL] = spawn_angles[PITCH] = 0;

		VectorCopy(goodOrigin, spawn_origin);
		return qtrue;
	}

	

	return qfalse;
}


qboolean G_CheckForCloserIronmanSpawn(gentity_t* ent, vec3_t spawn_origin, vec3_t spawn_angles, vec3_t spawn_velocity) {
	int				i;
	int				allowShortPos = 0;
	vec3_t			delta;
	//float normalSpawnDist; // wanted to check if normal spawn dist is closer but that might be too simplistic for complex level architectures
	float			currentDist;
	qboolean		good = qfalse;
	simplePos_t*	pos;
	trace_t			trace;
	//vec3_t			velNorm;
	if (!level.ironManPosCount || !level.ironManCurrentPositionSet || level.ironManClientNum == -1) {
		return qfalse;
	}

	//VectorSubtract(level.ironManCurrentPosition, spawn_origin, delta);
	//normalSpawnDist = VectorLengthSquared(delta);

retry:
	for (i = level.ironManPosCount - 1; i >= MAX(0, level.ironManPosCount - IRONMAN_MAX_PAST_POSITIONS_COUNT + 1); i--) {
		pos = &level.ironManPos[i % IRONMAN_MAX_PAST_POSITIONS_COUNT];
		if (pos->when + IRONMAN_RESPAWNPOSITION_MAXPOSITIONAGE < level.time) {
			// position is too old
			if (allowShortPos < 2) {
				// let's try with allowing shorter distances
				allowShortPos++;
				goto retry;
			}
			else {
				// fuck it
				return qfalse;
			}
		}

		VectorSubtract(pos->origin, level.ironManCurrentPosition, delta);
		currentDist = VectorLengthSquared(delta);
		
		if (allowShortPos == 2) {
			// we are desperate. spawn right on top of his head if needed! maybe hes camping or sth xd
			good = qtrue;
		}
		else if (currentDist > IRONMAN_RESPAWNPOSITION_MINDISTANCE* IRONMAN_RESPAWNPOSITION_MINDISTANCE) {
			good = qtrue;
		}
		else if (allowShortPos && currentDist > IRONMAN_RESPAWNPOSITION_MINDISTANCE_SHORT* IRONMAN_RESPAWNPOSITION_MINDISTANCE_SHORT) {
			good = qtrue;
		}

		if (good) {
			vec3_t goodOrigin;
			float speed;

			if (allowShortPos == 2) {
				int side, front, up, dist, skipvis;
				float traceDist = IRONMAN_RESPAWNPOSITION_MINDISTANCE_SHORT * 2.0f;
				float fracRequired = 0.4;
				good = qfalse;
				VectorCopy(pos->origin, goodOrigin);
				// we might spawn right on the capper's ass
				// try to move us a bit away if we can?
				for (skipvis = 0; skipvis < 2 && !good; skipvis++) { // in emergency, dont require visual contact to capper
					for (dist = 0; dist < 2 && !good; dist++) { // try shorter distance if nothing fouund
						if (dist == 1) {
							traceDist = IRONMAN_RESPAWNPOSITION_MINDISTANCE_SHORT;
							fracRequired = 0.8f;
						}
						for (up = 0; up < 2 && !good; up++) {
							for (side = -1; side < 2 && !good; side++) {
								for (front = -1; front < 2 && !good; front++) {
									if (side == 0 && front == 0) {
										continue;
									}
									goodOrigin[0] = pos->origin[0] + (float)front * traceDist;
									goodOrigin[1] = pos->origin[1] + (float)side * traceDist;
									goodOrigin[2] = pos->origin[2] + (float)up * 64.0f;
									//if (WiggleSpotTelefrag(goodOrigin, ent)) {

									if (skipvis) {
										if (WiggleSpotTelefrag(goodOrigin, ent)) {
											good = qtrue;
											break;
										}
									}
									else {
										JP_Trace(&trace, level.ironManCurrentPosition, playerMins, playerMaxs, goodOrigin, level.ironManClientNum, MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN);
										// make sure we could actually reach the capper from that place
										if (!trace.allsolid && !trace.startsolid && !(trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NOSPAWN)) && trace.fraction > fracRequired) { // let's be at least 0.6*min distance away
											// trace back in other direction (due to patches/1-way clips only being recognized in one direction)
											VectorCopy(trace.endpos, goodOrigin);
											JP_Trace(&trace, goodOrigin, playerMins, playerMaxs, level.ironManCurrentPosition, level.ironManClientNum, MASK_PLAYERSOLID);
											if (trace.fraction == 1.0f) {
												if (WiggleSpotTelefrag(goodOrigin, ent)) {
													good = qtrue;
													break;
												}
											}
										}
									}
								}
							}
						}
					}
				}
				if (!good) {
					VectorCopy(pos->origin, goodOrigin);
					good = WiggleSpotTelefrag(goodOrigin, ent);
				}
			}
			else {
				VectorCopy(pos->origin, goodOrigin);
				good = WiggleSpotTelefrag(goodOrigin, ent);
			}



			if (good) {
				// ok found a good pos
				VectorCopy(pos->velocity, spawn_velocity);
				VectorSubtract(level.ironManCurrentPosition,goodOrigin,delta);
				VectorNormalize(delta);
				vectoangles(delta, spawn_angles); // look at the iron man
				spawn_angles[ROLL] = spawn_angles[PITCH] = 0;

				//VectorCopy(pos->velocity, velNorm);
				//speed = VectorNormalize(velNorm);
				//if (speed > 10) {
				//	vectoangles(velNorm, spawn_angles);
				//	spawn_angles[ROLL] = spawn_angles[PITCH] = 0;
				//}
				//else {
				//	VectorCopy(pos->angles,spawn_angles);
				//}
				VectorCopy(goodOrigin, spawn_origin);
				return qtrue;
			}

		}

	}

	if (allowShortPos < 2) {
		// let's try with allowing shorter distances
		allowShortPos++;
		goto retry;
	}

	return qfalse;

	
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
extern qboolean WP_HasForcePowers( const playerState_t *ps );
extern void RestorePosition(gentity_t* client, savedPosition_t* savedPosition, veci_t* diffAccum);
void ClientSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
#if SEGMENTEDDEBUG
	static clientPersistant_t	saved;
#else
	static clientPersistant_t	saved;
#endif
	clientSession_t		savedSess;
	vec3_t				savedDeltaAngles;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	qboolean	lastSpawnPointRaceValid = qfalse;
	int		flags;
	int		savedPing;
	int		savedCommandTime;
//	char	*savedAreaBits;
	int		accuracy_hits, accuracy_shots;
	int		eventSequence;
//	char	userinfo[MAX_INFO_STRING];
	forcedata_t			savedForce;
	void		*ghoul2save;
	int		saveSaberNum = ENTITYNUM_NONE;
	int		wDisable = 0;
	qboolean	inSegmentedRun = qfalse;
	qboolean	raceSpawnPossible = qfalse;
	qboolean	useSavedSpawn = qfalse;
	int			nowTime = LEVELTIME(ent->client); // at the start of a client (ClientBegin) pers.cmd.serverTime is empty
	vec3_t		spawn_velocity;
	qboolean	spawn_velocity_set = qfalse;

	index = ent - g_entities;
	client = ent->client;

	if ( ent->client->ps.saberInFlight && ent->client->ps.saberEntityNum >= MAX_CLIENTS && ent->client->ps.saberEntityNum < MAX_GENTITIES )
	{
		gentity_t *saberent = &g_entities[ent->client->ps.saberEntityNum];

		if ( saberent && saberent->inuse && saberent->r.ownerNum == ent-g_entities && saberent->touch == thrownSaberTouch )
		{
			saberent->touch = SaberGotHit;
			saberent->think = SaberUpdateSelf;
			/*if ( jk2gameplay == VERSION_1_04 )*/ saberent->bolt_Head = 0; // MVSDK: This shouldn't affect gameplay.
			saberent->nextthink = level.time;

			MakeDeadSaber(saberent);

			saberent->r.svFlags |= (SVF_NOCLIENT);
			saberent->r.contents = CONTENTS_LIGHTSABER;
			VectorSet( saberent->r.mins, -SABER_BOX_SIZE, -SABER_BOX_SIZE, -SABER_BOX_SIZE );
			VectorSet( saberent->r.maxs, SABER_BOX_SIZE, SABER_BOX_SIZE, SABER_BOX_SIZE );
			saberent->s.loopSound = 0;

			ent->client->ps.saberInFlight = qfalse;
			ent->client->ps.saberThrowDelay = nowTime + 500;
			ent->client->ps.saberCanThrow = qfalse;
		}
	}

	if (client->ps.fd.forceDoInit)
	{ //force a reread of force powers
		WP_InitForcePowers( ent );
		client->ps.fd.forceDoInit = 0;
	}

	inSegmentedRun = client->sess.sessionTeam != TEAM_SPECTATOR && DF_ClientInSegmentedRunMode(client) && client->pers.segmented.state >= SEG_RECORDING_HAVELASTPOS && client->pers.segmented.state < SEG_REPLAY && client->pers.segmented.lastPos[RESPOSINDEX(client->pers.segmented.lastPosCount-1)].posIndex == (client->pers.segmented.lastPosCount - 1);

	raceSpawnPossible = client->sess.sessionTeam != TEAM_SPECTATOR && client->sess.raceMode && client->pers.savedSpawnUsed;
	useSavedSpawn = raceSpawnPossible && !inSegmentedRun && !memcmp(&client->sess.raceStyle, &client->pers.savedSpawnRaceStyle, sizeof(client->sess.raceStyle));

	if (raceSpawnPossible && !useSavedSpawn && !inSegmentedRun) {
		G_CenterPrint(ent - g_entities,3, "^1Warning: ^7Your spawn point is not valid for your changed race settings.",qfalse,qtrue,qfalse,NULL);
	}

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						spawn_origin, spawn_angles);
		lastSpawnPointRaceValid = qfalse;
	} else if (inSegmentedRun) {
		spawnPoint = NULL;
		VectorCopy(client->pers.segmented.lastPos[RESPOSINDEX(client->pers.segmented.lastPosCount - 1)].pos.ps.origin, spawn_origin);
		VectorCopy(client->pers.segmented.lastPos[RESPOSINDEX(client->pers.segmented.lastPosCount - 1)].pos.ps.viewangles, spawn_angles);
		client->pers.segmented.respos = qtrue;
	} else if (useSavedSpawn) {
		spawnPoint = NULL;
		VectorCopy(client->pers.savedSpawn.ps.origin, spawn_origin);
		VectorCopy(client->pers.savedSpawn.ps.viewangles, spawn_angles);
	} else if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( ent,
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						spawn_origin, spawn_angles);
		lastSpawnPointRaceValid = qfalse;
	}
	else if (g_gametype.integer == GT_SAGA)
	{
		spawnPoint = SelectSagaSpawnPoint ( ent,
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						spawn_origin, spawn_angles);
		lastSpawnPointRaceValid = qfalse;
	}
	else {
		int iters = 1;
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( ent,spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint ( ent,
					client->ps.origin, 
					spawn_origin, spawn_angles);
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			lastSpawnPointRaceValid = qtrue;

			break;

		} while (iters-- > 0); // TA: this looks like it could potentially cause an infinite loop. limit it to 2.
		if (!spawnPoint) {

			G_Error("Couldn't find a spawn point (#3)");
		}
	}

	if (client->sess.mode == MODE_IRONMAN && client->sess.modeTeam != MODETEAM_IRONMAN_CAPPER) {
		if (G_CheckForCloserIronmanSpawn(ent,spawn_origin,spawn_angles,spawn_velocity)) {
			spawnPoint = NULL;
			spawn_velocity_set = qtrue;
		}
	}

	client->pers.teamState.state = TEAM_ACTIVE;

	if (g_arenaAutoGen.integer && !level.hasArenaInfo) {
		level.mustGenerateArena = qtrue;
	}

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	VectorCopySafe(client->ps.delta_angles, savedDeltaAngles);
	savedPing = client->ps.ping;
	savedCommandTime = client->ps.commandTime;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	ghoul2save = client->ghoul2;

	saveSaberNum = client->ps.saberEntityNum;

	G_BufferedSendOrPrintFlush(ent, qfalse, qfalse);
	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	VectorCopySafe(savedDeltaAngles, client->ps.delta_angles); // to make sure my segmented runs work

	//rww - Don't wipe the ghoul2 instance or the animation data
	client->ghoul2 = ghoul2save;

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = nowTime + 12000;

//	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	client->pers.maxHealth = 100;//atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	G_SetClassName(ent, "player");
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->contenttype = 0;
	ent->flags = 0;

	ResetClientModeIfInvalid(ent, (qboolean)!(ent->r.svFlags& SVF_BOT));
	//if (!g_defrag.integer) {
	//	DF_SetRaceMode(ent,qfalse);
	//	//if (client->sess.raceMode) {
	//	//	client->sess.raceMode = qfalse;
	//	//	Cmd_ForceChanged_f(ent);
	//	//}
	//	//else {
	//	//	client->sess.raceMode = qfalse;
	//	//}
	//}

	ModeClientRespawning(ent);

	//if (client->sess.raceMode)
	//	client->ps.stats[STAT_RACEMODE] = 1;
	//else
	//	client->ps.stats[STAT_RACEMODE] = 0;

	//client->ps.stats[STAT_MOVEMENTSTYLE] = client->sess.raceStyle.movementStyle;
	//client->ps.stats[STAT_RUNFLAGS] = client->sess.raceStyle.runFlags;
	UpdateClientRaceVars(client);

	if (client->sess.raceMode && client->sess.raceStyle.movementStyle == MV_BOUNCE) {
		client->ps.stats[STAT_BOUNCEPOWER] = BOUNCEPOWER_MAX;
	}
	else if (client->sess.raceMode && client->sess.raceStyle.movementStyle == MV_CHARGEJUMP) {
		client->ps.stats[STAT_CHARGEJUMPDATA] = 0;
	}
	else {
		client->ps.stats[STAT_BOUNCEPOWER] = 0;
	}

	
	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;
	//give default weapons
	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );

	if (g_gametype.integer == GT_TOURNAMENT)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	if ( jk2gameplay == VERSION_1_02 )
	{
		if (g_gametype.integer == GT_HOLOCRON)
		{
			//always get free saber level 1 in holocron
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
		}
		else
		{
			if (client->ps.fd.forcePowerLevel[FP_SABERATTACK])
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
			}
			else
			{ //if you don't have saber attack rank then you don't get a saber
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
			}
		}
		
		if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
		{
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
		}
		else if (g_gametype.integer == GT_JEDIMASTER)
		{
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
		}

		if (g_gametype.integer == GT_JEDIMASTER)
		{
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
		}

		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL) && (!g_defrag.integer || !(client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))))
		{
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			client->ps.weapon = WP_SABER;
		}
		else
		{
			client->ps.weapon = WP_STUN_BATON;
		}
	}
	else
	{
		if ( g_gametype.integer != GT_HOLOCRON 
			&& g_gametype.integer != GT_JEDIMASTER 
			&& !HasSetSaberOnly()
			&& !AllForceDisabled( g_forcePowerDisable.integer )
			&& g_trueJedi.integer )
		{
			if ( jk2gameplay == VERSION_1_04 && g_gametype.integer >= GT_TEAM && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam == TEAM_RED) )
			{//In Team games, force one side to be merc and other to be jedi
				if ( level.numPlayingClients > 0 )
				{//already someone in the game
					int		i;
					team_t	forceTeam = TEAM_SPECTATOR;
					for ( i = 0 ; i < level.maxclients ; i++ ) 
					{
						if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
							continue;
						}
						if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED ) 
						{//in-game
							if ( WP_HasForcePowers( &level.clients[i].ps ) )
							{//this side is using force
								forceTeam = level.clients[i].sess.sessionTeam;
							}
							else
							{//other team is using force
								if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
								{
									forceTeam = TEAM_RED;
								}
								else
								{
									forceTeam = TEAM_BLUE;
								}
							}
							break;
						}
					}
					if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam )
					{//using force but not on right team, switch him over
						const char *teamName = TeamName( forceTeam );
						//client->sess.sessionTeam = forceTeam;
						SetTeam( ent, (char *)teamName );
						return;
					}
				}
			}

			if ( WP_HasForcePowers( &client->ps ) )
			{
				client->ps.trueNonJedi = qfalse;
				client->ps.trueJedi = qtrue;
				//make sure they only use the saber
				client->ps.weapon = WP_SABER;
				client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
			}
			else
			{//no force powers set
				client->ps.trueNonJedi = qtrue;
				client->ps.trueJedi = qfalse;
				if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
				}
				if ( jk2gameplay == VERSION_1_04 )
				{
					if (!wDisable || !(wDisable & (1 << WP_BLASTER)))
					{
						client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BLASTER );
					}
					if (!wDisable || !(wDisable & (1 << WP_BOWCASTER)))
					{
						client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BOWCASTER );
					}
				}
				client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
				client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
				if (g_defrag.integer && (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))) {
					client->ps.weapon = WP_SABER;
				}
				else {
					client->ps.weapon = WP_BRYAR_PISTOL;
				}
			}
		}
		else
		{//jediVmerc is incompatible with this gametype, turn it off!
			if ( jk2gameplay == VERSION_1_04 ) trap_Cvar_Set( "g_jediVmerc", "0" ); // MVSDK: I don't know what happens if you try jediVmerc with one of the incompatible gametypes, but maybe you end up with some special kind of jedi-master gametype... // FIXME: Check if jediVmerc with incompatible gametypes has some bad side-effects.
			if (g_gametype.integer == GT_HOLOCRON)
			{
				//always get free saber level 1 in holocron
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
			}
			else
			{
				if (client->ps.fd.forcePowerLevel[FP_SABERATTACK])
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
				}
				else
				{ //if you don't have saber attack rank then you don't get a saber
					client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
				}
			}

			if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
			}
			else if (g_gametype.integer == GT_JEDIMASTER)
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
			}

			if (g_gametype.integer == GT_JEDIMASTER)
			{
				client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
			}

			if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL) && (!g_defrag.integer || !(client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))))
			{
				client->ps.weapon = WP_BRYAR_PISTOL;
			}
			else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
			{
				client->ps.weapon = WP_SABER;
			}
			else
			{
				client->ps.weapon = WP_STUN_BATON;
			}
		}
	}


	switch (client->sess.mode) {
		case MODE_NORMAL:
		default:
			break;
		case MODE_DEFRAG:
			client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER) + (1 << WP_DISRUPTOR) + (1 << WP_STUN_BATON);
			client->ps.weapon = WP_SABER;
			break;
		case MODE_DUEL:
		case MODE_DUELQUEUE:
		case MODE_ALLFORCE:
		case MODE_IRONMAN:
			client->ps.stats[STAT_WEAPONS] = 1 << WP_SABER;
			client->ps.weapon = WP_SABER;
			break;
	}

	/*
	client->ps.stats[STAT_HOLDABLE_ITEMS] |= ( 1 << HI_BINOCULARS );
	client->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_BINOCULARS, IT_HOLDABLE);
	*/

	client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
	client->ps.stats[STAT_HOLDABLE_ITEM] = 0;

	if ( client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		client->ps.stats[STAT_WEAPONS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

	client->ps.ammo[AMMO_BLASTER] = 100; //ammoData[AMMO_BLASTER].max; //100 seems fair.
//	client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
//	client->ps.ammo[AMMO_FORCE] = ammoData[AMMO_FORCE].max;
//	client->ps.ammo[AMMO_METAL_BOLTS] = ammoData[AMMO_METAL_BOLTS].max;
//	client->ps.ammo[AMMO_ROCKETS] = ammoData[AMMO_ROCKETS].max;
/*
	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_BRYAR_PISTOL);
	if ( g_gametype.integer == GT_TEAM ) {
		client->ps.ammo[WP_BRYAR_PISTOL] = 50;
	} else {
		client->ps.ammo[WP_BRYAR_PISTOL] = 100;
	}
*/
	client->ps.rocketLockIndex = MAX_CLIENTS;
	client->ps.rocketLockTime = 0;

	//rww - Set here to initialize the circling seeker drone to off.
	//A quick note about this so I don't forget how it works again:
	//ps.genericEnemyIndex is kept in sync between the server and client.
	//When it gets set then an entitystate value of the same name gets
	//set along with an entitystate flag in the shared bg code. Which
	//is why a value needs to be both on the player state and entity state.
	//(it doesn't seem to just carry over the entitystate value automatically
	//because entity state value is derived from player state data or some
	//such)
	client->ps.genericEnemyIndex = -1;

	client->ps.isJediMaster = qfalse;

	client->ps.fallingToDeath = 0;

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers( ent );

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25;

	// Start with a small amount of armor as well.
	client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25;
	if (client->ps.persistant[PERS_SPAWN_COUNT] == 1 && g_stackFirstSpawn.integer) {
		// first spawn. if g_stackFirstSpawn is 1, give us 200 armor and a medpack
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 2;
		client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC);
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC, IT_HOLDABLE);
	}

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	G_GetUserCmd(client - level.clients, &ent->client->pers.cmd, GETUSERCMD_NOADVANCE);
	if(!useSavedSpawn){
		DF_PreDeltaAngleChange(ent->client);
		SetClientViewAngle(ent, spawn_angles);
		DF_PostDeltaAngleChange(ent->client,qtrue);
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {

	} else {
		G_KillBox( ent );
		trap_LinkEntity (ent);

		// force the base weapon up
		client->ps.weapon = WP_BRYAR_PISTOL;
		client->ps.weaponstate = FIRST_WEAPON;

	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = nowTime; // i think for invulnerability shell and such?
	client->pers.lastLevelSpawnTime = level.time; // always level.time, never commandtime, never use for gameplay sensitive things (that could mess up a segmented replay)
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	//client->inactivityToSpecTime = level.time + g_inactivityToSpec.integer * 1000; someone could be stuck falling into a death trigger on a weird map and never go to spec.
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = WeaponReadyAnim[client->ps.weapon];
	client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		if (spawnPoint) {
			G_UseTargets(spawnPoint, ent);
		}

		client->ps.weapon = 1;
		if ((client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER)) && (client->sess.raceMode || g_startWeaponAlwaysSaber.integer)) { // TA: Always prefer saber
			client->ps.weapon = WP_SABER;
		}
		else {
			// select the highest weapon number available, after any
			// spawn given items have fired
			// TA: this is chaotic. ps.weapon is set like 3 times in this whole function or more wtf. first with logic, then hard to bryar, and then here
			for (i = WP_NUM_WEAPONS - 1; i > 0; i--) {
				if (client->ps.stats[STAT_WEAPONS] & (1 << i)) {
					client->ps.weapon = i;
					break;
				}
			}
		}
	}

	if (spawn_velocity_set) {
		VectorCopy(spawn_velocity, client->ps.velocity);
	}

	if (spawnPoint && lastSpawnPointRaceValid) {
		client->pers.lastSpawnPoint = spawnPoint - g_entities;
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	if (!inSegmentedRun) {
		// dont do in racemode in segmented runs and with start spawn
		client->ps.commandTime = nowTime - 100;
		ent->client->pers.cmd.serverTime = nowTime;
		if (spawn_velocity_set) {
			ent->client->pers.cmd.upmove = 127; // jump to preserve the velocity if needed? might need more tweaking
		}
		ClientThink(ent - g_entities);
	}
	else {
		client->ps.commandTime = (savedCommandTime >0) ? savedCommandTime : nowTime; // how will things work out when fps anti toggle is active?
	}
	

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, g_snapPlayerPosAngles.integer);
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	if (g_spawnInvulnerability.integer && !ent->client->sess.raceMode)
	{
		ent->client->ps.eFlags |= EF_INVULNERABLE;
		ent->client->invulnerableTimer = nowTime + g_spawnInvulnerability.integer;
	}

	// run the presend to set anything else
	if ( ent->client->sess.spectatorState != SPECTATOR_FOLLOW )
	{ // Only do this if we're not dealing with follow spectators to prevent two bugs:
	  // 1) follow spectators turning into free spectators at map_restart, because the client they were following has a higher client number and isn't ingame, yet
	  // 2) follow spectators corrupting their s.number in BG_PlayerStateToEntityState, cause they get the other client's playerState in ClientEndFrame
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
			SpectatorClientEndFrame(ent);
		}
		else {
			ClientEndFrame(ent,qtrue); // is the qtrue logical? just trying to keep things consistent with how they were while making defrag more deterministic
		}
	}

	if (useSavedSpawn) {
		int oldTeleBit = client->ps.eFlags & EF_TELEPORT_BIT;
		RestorePosition(ent, &client->pers.savedSpawn, client->pers.segmented.anglesDiffAccum);

		// lets make sure tele bit doesn't get messed up.
		// don't do this in other places where we use RestorePosition, because EF_TELEPORT_BIT
		// can have an effect on some things like robust trigger logic, and segmented runs need to 
		// remain 100% stable at all times.
		// but on spawn (which inevitably isn't during a race) it's fair game)
		client->ps.eFlags &= ~EF_TELEPORT_BIT;
		client->ps.eFlags |= oldTeleBit;
	}

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, g_snapPlayerPosAngles.integer);

	if (!inSegmentedRun) {
		DF_RaceStateInvalidated(ent,qfalse);
		ent->client->sess.raceStateInvalidated = qfalse;
		ent->client->pers.antiLoop.yawAngleChangeSinceBaseSpeed = 0;
		if (ent->client->pers.lastRaceTimerStartedCP > level.time-3000) {
			G_CenterPrint(ent - g_entities, 3, "", qfalse, qtrue, qfalse, NULL); // just send an empty cp to clear the screen of the old "Timer started!"
			ent->client->pers.lastRaceTimerStartedCP = 0;
		}
	}
}

extern qboolean DF_RemoveCheckPoints(gentity_t* playerent);

void ClientDisconnectFinish(int clientNum, gentity_t* ent) {
	gentity_t* tent;
	int			i;

	// remove this player as the activator from any activated ents
	// is this actually safe? what if some ent just expects the activator to be a valid ent?
	// yea better just check activator->inuse...
	G_ClearEntityActivator(ent); // this one not needed prolly cuz client has no activator, but lets be safe.
	G_ClearActivatedEntities(ent);
	DF_ClearCheckPointTimes(ent);
	G_ResetUserCmdStore(ent - g_entities);

	DF_RemoveCheckPoints(ent);

	if (ent->client->pers.recordingDemo) {

		ent->client->pers.recordingDemo = qfalse;
		ent->client->pers.demoStoppedTime = level.time;
		if (!ent->client->pers.keepDemoMaybe) {
			trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i;svrenamedemo \"%s\" \"%strash/trash%d\"\n", (int)(ent - g_entities), ent->client->pers.tempDemoName, level.tempDemoNamePrefix, (int)(ent - g_entities)));
		}
		else {
			trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i\n", (int)(ent - g_entities)));
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i - 50] && ent->client->ps.fd.killSoundEntIndex[i - 50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i - 50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i - 50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	// stop any following clients
	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum) {
			StopFollowing(&g_entities[i]);
		}
		level.clients[i].sess.ignore &= ~(1 << clientNum);
	}

	// send effect if they were completely connected
	if (ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems(ent, -1);
	}

	G_LogPrintf("ClientDisconnect: %i\n", clientNum);

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& !level.intermissiontime
		&& !level.warmupTime) {
		if (level.sortedClients[1] == clientNum) {
			level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[0]].sess.wins++;
			ClientUserinfoChanged(level.sortedClients[0]);
		}
		else if (level.sortedClients[0] == clientNum) {
			level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[1]].sess.wins++;
			ClientUserinfoChanged(level.sortedClients[1]);
		}
	}

	trap_UnlinkEntity(ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	G_SetClassName(ent, "disconnected");
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if (ent->r.svFlags & SVF_BOT) {
		BotAIShutdownClient(clientNum, qfalse);
	}

	G_ClearClientLog(clientNum);
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	G_ResetClientVote(ent->client);

	if (DF_KeepClientZombie(ent)) {
		return;
	}

	ClientDisconnectFinish(clientNum, ent);
}

void G_SendServerCommand(int targetnum, const char* cmd, qboolean alsoFollowers) {
	int i;
	gentity_t* other;
	trap_SendServerCommand(targetnum, cmd);
	if (targetnum == -1 || !alsoFollowers) {
		return;
	}
	
	for (i = 0; i < level.maxclients; i++) {
		other = g_entities + i;
		if (!other->client || !other->inuse || other->client->sess.spectatorState != SPECTATOR_FOLLOW || other->client->sess.spectatorClient != targetnum) continue; // can !other->client happen? no idea lazy to think about it.
		trap_SendServerCommand(i, cmd);
	}
}

void G_CenterPrintOrPrint(int targetNum, int autoLineWraps, const char* message, qboolean printInDefrag, qboolean alsoFollowers, qboolean alwaysPrint, const char* extra, qboolean onlyPrint, qboolean noop) {
	if (noop) {
		return;
	}
	else if (onlyPrint) {
		if (extra) {
			G_SendServerCommand(targetNum, va("print \"%s\n\" %s", message, extra), alsoFollowers);
		}
		else {
			G_SendServerCommand(targetNum, va("print \"%s\n\"", message), alsoFollowers);
		}
		return;
	}
	else {
		G_CenterPrint(targetNum, autoLineWraps, message, printInDefrag, alsoFollowers, alwaysPrint, extra);
	}
}

#define MAX_CLIENT_CENTERPRINT_LINELENGTH 50
#define MAX_CLIENT_CENTERPRINT_LENGTH 1024
void G_CenterPrint( int targetNum, int autoLineWraps, const char *message, qboolean printInDefrag, qboolean alsoFollowers, qboolean alwaysPrint, const char* extra)
{
	int len = strlen(message);
	if (printInDefrag && g_defrag.integer) {
		if (extra) {
			G_SendServerCommand(targetNum, va("print \"%s\n\" %s", message, extra), alsoFollowers);
		}
		else {
			G_SendServerCommand(targetNum, va("print \"%s\n\"", message), alsoFollowers);
		}
	}
	else if (!autoLineWraps || len <= MAX_CLIENT_CENTERPRINT_LINELENGTH) {
		if (alwaysPrint) {
			if (extra) {
				G_SendServerCommand(targetNum, va("print \"%s\n\" %s", message,extra), alsoFollowers);
			}
			else {
				G_SendServerCommand(targetNum, va("print \"%s\n\"", message), alsoFollowers);
			}
		}
		if (extra) {
			G_SendServerCommand(targetNum, va("cp \"%s\" %s", message, extra), alsoFollowers);
		}
		else {
			G_SendServerCommand(targetNum, va("cp \"%s\"", message), alsoFollowers);
		}
	}
	else
	{
		char newMessage[MAX_CLIENT_CENTERPRINT_LENGTH];

		const char *lineStart = message;
		const char *lineEnd = lineStart;
		const char *wordStart, *wordEnd;
		const char *ptr;

		int lineLength;
		int wordLength;
		int curLength;
		int reset;
		int isMultiLang;

		if (alwaysPrint) {
			if (extra) {
				G_SendServerCommand(targetNum, va("print \"%s\n\" %s", message,extra), alsoFollowers);
			}
			else {
				G_SendServerCommand(targetNum, va("print \"%s\n\"", message), alsoFollowers);
			}
		}

		*newMessage = 0;

		while ( *lineStart && (size_t)(lineStart-message) < len)
		{
			if ( *newMessage ) Q_strcat( newMessage, sizeof(newMessage), "\n" );

			while ( *lineEnd && *lineEnd != '\n' ) lineEnd++;
			lineLength = lineEnd - lineStart;

			isMultiLang = 0;
			ptr = lineStart;
			while ( ptr < lineEnd-2 )
			{
				if ( *ptr == '@' && *(ptr+1) == '@' && *(ptr+2) == '@' )
				{
					isMultiLang = 1;
					break;
				}
				ptr++;
			}

			if ( lineLength > MAX_CLIENT_CENTERPRINT_LINELENGTH || isMultiLang )
			{ // Now we have to cut the line.
				wordStart = wordEnd = lineStart;
				curLength = 0;
				reset = 0;

				while ( lineStart < lineEnd )
				{
					isMultiLang = 0;
					if ( reset )
					{
						curLength = 0;
						Q_strcat( newMessage, sizeof(newMessage), "\n" );

						ptr = wordStart;
						while ( *ptr && (*ptr == ' ' || *ptr == '\t') ) ptr++;

						if ( !Q_IsColorString_1_02(ptr) ) // CenterPrint interprets colors like the 1.02 console
						{
							// Find the old color if we don't start with a new color on the next line
							ptr = wordStart - 1;
							while ( *ptr && ptr >= lineStart )
							{
								if ( Q_IsColorString_1_02(ptr) )
								{
									if ( *(ptr+1) == '7' ) break; // Don't redo white

									Q_strcat( newMessage, sizeof(newMessage), va("^%c", *(ptr+1)) );
									curLength += 2;
									reset = 2;
									break;
								}
								ptr--;
							}
						}
					}
					wordEnd = wordStart;
					while ( *wordEnd != ' ' && *wordEnd != '\t' && *wordEnd != '\n' && wordEnd < lineEnd ) wordEnd++;
					wordLength = wordEnd - wordStart;

					if ( wordEnd > lineEnd ) // Make sure the word is still in our line and doesn't exceed it
						break;

					if ( wordLength >= 3 && *wordStart == '@' && *(wordStart+1) == '@' && *(wordStart+2) == '@' )
						isMultiLang = 1;

					// & 1: move whole words to the next line (prefered if combined with 2)
					// & 2: cut words into pieces
					if ( curLength + wordLength > MAX_CLIENT_CENTERPRINT_LINELENGTH || isMultiLang )
					{ // The next word would make the line too long
						if ( (autoLineWraps & 2) && (!curLength || !(autoLineWraps & 1)) && !isMultiLang )
						{ // We want to cut the word into pieces
							if ( curLength && !reset ) Q_strcat( newMessage, sizeof(newMessage), " " );

							wordEnd = ptr = wordStart + (MAX_CLIENT_CENTERPRINT_LINELENGTH - curLength);
							ptr = wordEnd - 1;

							// Make sure we don't accidently split a colorcode
							while ( *ptr && ptr > wordStart && *ptr == '^' ) ptr--;
							if ( ptr == wordStart ) ptr = wordEnd - 1;

							wordEnd = ptr + 1;

							G_StringAppendSubstring( newMessage, sizeof(newMessage), wordStart, wordEnd-wordStart );
							wordStart = wordEnd;
						}
						else if ( !curLength || reset == 2 )
						{ // We don't to split it, so just append the whole thing and let the client cut it off
							G_StringAppendSubstring( newMessage, sizeof(newMessage), wordStart, wordLength );
							wordStart = wordEnd + 1;
						}

						// Reset the counters
						reset = 1;
					}
					else
					{ // Append the word
						// If this isn't the first word add a space
						if ( curLength && !reset ) Q_strcat( newMessage, sizeof(newMessage), " " );
						G_StringAppendSubstring( newMessage, sizeof(newMessage), wordStart, wordLength );
						curLength += wordLength + 1;
						wordStart = wordEnd + 1;

						reset = 0;
					}

					// Shouldn't get here
					if ( !*wordEnd ) break;
				}
			}
			else G_StringAppendSubstring( newMessage, sizeof(newMessage), lineStart, lineLength );

			// Next line
			lineStart = lineEnd + 1;
			lineEnd = lineStart;
		}
		if (extra) {
			G_SendServerCommand(targetNum, va("cp \"%s\" %s", newMessage, extra), alsoFollowers);
		}
		else {
			G_SendServerCommand(targetNum, va("cp \"%s\"", newMessage), alsoFollowers);
		}
	}
}


