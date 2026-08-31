// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

userCmdBuffer_t		userCmdBuffer[MAX_CLIENTS];

static usercmd_t nullUserCmd={ 0 };

void G_UserCmdBuffer_NewFrame() {
	int i;
	for (i = 0; i < level.maxclients; i++) {
		userCmdBuffer[i].msecThisFrame = 0;
	}
}

qboolean PM_SaberInTransition( int move );
qboolean PM_SaberInStart( int move );
qboolean PM_SaberInReturn( int move );

void P_SetTwitchInfo(gclient_t	*client)
{
	client->ps.painTime = LEVELTIME(client);
	client->ps.painDirection ^= 1; // not even sent over the network, kek
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;
	int nowTime = LEVELTIME(player->client);

	// all of this stuff seems only relevant for cgame
	// but pain_debounce_time is co-used by other stuff so lets put it in clientthink_real anyway when racemodeing

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (nowTime > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {

		// don't do more than two pain sounds a second
		if (nowTime - client->ps.painTime < 500 ) {
			return;
		}
		P_SetTwitchInfo(client); // defrag note:unless im mistaken, this is only used gameside in BG_AddPainTwitch, which is called in BG_G2PlayerAngles, which is actually never used
		player->pain_debounce_time = nowTime + 700; // timer also used for sizzle and uh... EV_ROLL?!
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;

		if (client->damage_armor && !client->damage_blood)
		{
			client->ps.damageType = 1; //pure shields
		}
		else if (client->damage_armor)
		{
			client->ps.damageType = 2; //shields and health
		}
		else
		{
			client->ps.damageType = 0; //pure health
		}
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean	envirosuit;
	int			waterlevel;
	int			nowTime = LEVELTIME(ent->client);
	qboolean	isIronman = (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG] || ent->client->ps.powerups[PW_NEUTRALFLAG]) && ent->client->sess.mode == MODE_IRONMAN;
	int			airOutTime = isIronman ? 1000 : 12000;
	int			drownLevel = isIronman ? 1 : 3; // ironman capper starts drowning really fast and in shallow water. prevent annoying stalling tactics.

	if ( ent->client->noclip ) {
		ent->client->airOutTime = nowTime + airOutTime;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > nowTime;

	//
	// check for drowning
	//
	if ( waterlevel >= drownLevel) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = nowTime + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < nowTime) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				int damageFlags = DAMAGE_NO_ARMOR;

				if (ent->client->sess.raceMode) {
					if (ent->client->sess.raceStyle.runFlags & RFL_LAVAPROTECT) {
						damageFlags |= FAKE_DAMAGE_IN_RACEMODE;
					}
					else {
						damageFlags |= DAMAGE_IN_RACEMODE;
					}
				}

				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav")); // "*drown.wav"
				} else if (rand()&1) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = nowTime + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, damageFlags, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = nowTime + airOutTime;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= nowTime) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				int damageFlags = ent->client && ent->client->sess.raceMode && (ent->client->sess.raceStyle.runFlags & RFL_LAVAPROTECT) ? FAKE_DAMAGE_IN_RACEMODE : DAMAGE_IN_RACEMODE;
				//if (!ent->client || !ent->client->sess.raceMode) { //No sizzle dmg in racemode?
					if (ent->watertype & CONTENTS_LAVA) {
						G_Damage(ent, NULL, NULL, NULL, NULL,
							30 * waterlevel, damageFlags, MOD_LAVA);
					}

					if (ent->watertype & CONTENTS_SLIME) {
						G_Damage(ent, NULL, NULL, NULL, NULL,
							10 * waterlevel, damageFlags, MOD_SLIME);
					}
				//}
			}
		}
	}
}





//==============================================================
extern void G_ApplyKnockback( gentity_t *targ, vec3_t newDir, float knockback );
void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf )
{
	float magnitude, my_mass;
	vec3_t	velocity;
	int cont;
	int nowTime = LEVELTIME(self->client);

	if( self->client )
	{
		VectorCopy( self->client->ps.velocity, velocity );
		my_mass = self->mass;
	}
	else 
	{
		VectorCopy( self->s.pos.trDelta, velocity );
		if ( self->s.pos.trType == TR_GRAVITY )
		{
			velocity[2] -= 0.25f * g_gravity.value;
		}
		if( !self->mass )
		{
			my_mass = 1;
		}
		else if ( self->mass <= 10 )
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;///10;
		}
	}

	magnitude = VectorLength( velocity ) * my_mass / 10;

	/*
	if(pointcontents(self.absmax)==CONTENT_WATER)//FIXME: or other watertypes
		magnitude/=3;							//water absorbs 2/3 velocity

	if(self.classname=="barrel"&&self.aflag)//rolling barrels are made for impacts!
		magnitude*=3;

	if(self.frozen>0&&magnitude<300&&self.flags&FL_ONGROUND&&loser==world&&self.velocity_z<-20&&self.last_onground+0.3<time)
		magnitude=300;
	*/

	if ( !self->client || self->client->ps.lastOnGround+300< nowTime || ( self->client->ps.lastOnGround+100 < nowTime && other->material >= MAT_GLASS ) )
	{
		vec3_t dir1, dir2;
		float force = 0, dot;

		if ( other->material >= MAT_GLASS )
			magnitude *= 2;

		//damage them
		if ( magnitude >= 100 && other->s.number < ENTITYNUM_WORLD )
		{
			VectorCopy( velocity, dir1 );
			VectorNormalize( dir1 );
			if( VectorCompare( other->r.currentOrigin, vec3_origin ) )
			{//a brush with no origin
				VectorCopy ( dir1, dir2 );
			}
			else
			{
				VectorSubtract( other->r.currentOrigin, self->r.currentOrigin, dir2 );
				VectorNormalize( dir2 );
			}

			dot = DotProduct( dir1, dir2 );

			if ( dot >= 0.2 )
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= (magnitude/50);

			cont = trap_PointContents( other->r.absmax, other->s.number );
			if( (cont&CONTENTS_WATER) )//|| (self.classname=="barrel"&&self.aflag))//FIXME: or other watertypes
			{
				force /= 3;							//water absorbs 2/3 velocity
			}

			/*
			if(self.frozen>0&&force>10)
				force=10;
			*/

			if( ( force >= 1 && other->s.number != 0 ) || force >= 10)
			{
	/*			
				dprint("Damage other (");
				dprint(loser.classname);
				dprint("): ");
				dprint(ftos(force));
				dprint("\n");
	*/
				if ( other->r.svFlags & SVF_GLASS_BRUSH )
				{
					other->splashRadius = (float)(self->r.maxs[0] - self->r.mins[0])/4.0f;
				}
				if ( other->takedamage )
				{
					G_Damage( other, self, self, velocity, self->r.currentOrigin, force, DAMAGE_NO_ARMOR, MOD_CRUSH);//FIXME: MOD_IMPACT
				}
				else
				{
					G_ApplyKnockback( other, dir2, force );
				}
			}
		}

		if ( damageSelf && self->takedamage )
		{
			//Now damage me
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if ( self->client && self->client->ps.fd.forceJumpZStart )
			{//we were force-jumping
				if ( self->r.currentOrigin[2] >= self->client->ps.fd.forceJumpZStart )
				{//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.fd.forceJumpZStart-self->r.currentOrigin[2])/3;
				}
			}
			//if(self.classname!="monster_mezzoman"&&self.netname!="spider")//Cats always land on their feet
				if( ( magnitude >= 100 + self->health && self->s.number != 0 && self->s.weapon != WP_SABER ) || ( magnitude >= 700 ) )//&& self.safe_time < level.time ))//health here is used to simulate structural integrity
				{
					if ( (self->s.weapon == WP_SABER || self->s.number == 0) && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE && magnitude < 1000 )
					{//players and jedi take less impact damage
						//allow for some lenience on high falls
						magnitude /= 2;
						/*
						if ( self.absorb_time >= time )//crouching on impact absorbs 1/2 the damage
						{
							magnitude/=2;
						}
						*/
					}
					magnitude /= 40;
					magnitude = magnitude - force/2;//If damage other, subtract half of that damage off of own injury
					if ( magnitude >= 1 )
					{
		//FIXME: Put in a thingtype impact sound function
		/*					
						dprint("Damage self (");
						dprint(self.classname);
						dprint("): ");
						dprint(ftos(magnitude));
						dprint("\n");
		*/
						/*
						if ( self.classname=="player_sheep "&& self.flags&FL_ONGROUND && self.velocity_z > -50 )
							return;
						*/
						G_Damage( self, NULL, NULL, NULL, self->r.currentOrigin, magnitude/2, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
					}
				}
		}

		//FIXME: slow my velocity some?

		// NOTENOTE We don't use lastimpact as of yet
//		self->lastImpact = level.time;

		/*
		if(self.flags&FL_ONGROUND)
			self.last_onground=time;
		*/
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

static int int_cmp(const void* a, const void* b) {
	int* aa = (int*)a;
	int* bb = (int*)b;

	if (*aa > *bb) return 1;
	else if (*aa == *bb) return 0;
	else return -1;
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num, numTraced;
	int			touch[MAX_GENTITIES];
	qboolean	touchViaTrace[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	vec3_t		minsPrev, maxsPrev;
	vec3_t		minsTotal, maxsTotal;
	vec3_t		minsPlayer, maxsPlayer;
	static vec3_t	range = { 40, 40, 52 };
	static vec3_t	playerMinsDefault = { -15, -15, DEFAULT_MINS_2 };
	static vec3_t	playerMaxsDefault = { 15, 15, DEFAULT_MAXS_2 };
	qboolean	wantRobustTriggers = g_triggersRobust.integer && ent->client->sess.raceMode || g_triggersRobust.integer > 1;
	qboolean	robustTriggerEvaluation = qfalse;
	qboolean	isTraced;
	int			nowTime = LEVELTIME(ent->client);
	qboolean	needExtraItemsPass = !ent->client->sess.raceMode;

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	robustTriggerEvaluation = wantRobustTriggers && ent->client->prePmovePositionSet && !((ent->client->ps.eFlags ^ ent->client->prePmoveEFlags) & EF_TELEPORT_BIT);

	// if we have a past position, move from that to the current one. 
	// teleport bit check may not be needed since there doesn't appear to be any respawn/teleport
	// between pmove and G_TouchTriggers, but that may change and i may have overlooked sth
	if (robustTriggerEvaluation) {
		qboolean finished = qfalse;
		qboolean reverse = qfalse;
		qboolean needExtraCheck = qfalse;
		qboolean startIsEnd = VectorCompare(ent->client->postPmovePosition, ent->client->prePmovePosition);

		memset(&touchViaTrace, 0, sizeof(touchViaTrace));

		// we want to find the smallest bounding box between last and current frame
		// to minimize some weird crouch hacking to reach triggers with the trace trigger evaluataion
		VectorMax(ent->client->prePmoveMins, ent->client->postPmoveMins, minsPlayer); 
		VectorMin(ent->client->prePmoveMaxs, ent->client->postPmoveMaxs, maxsPlayer);

		VectorCopy(minsPlayer,ent->client->triggerMins);
		VectorCopy(maxsPlayer,ent->client->triggerMaxs);

		VectorAdd(ent->client->postPmovePosition, minsPlayer, mins);
		VectorAdd(ent->client->postPmovePosition, maxsPlayer, maxs);
		VectorAdd(ent->client->prePmovePosition, minsPlayer, minsPrev);
		VectorAdd(ent->client->prePmovePosition, maxsPlayer, maxsPrev);
		num = 0;

		// if start == end, trace will return entitynum even if startsolid apparently, and we don't wanna mark triggers as traced
		// if we are fully in them from start to finish
		// also waste of time to do so many traces then..
		if(!startIsEnd){
			qboolean somethingInTheWay; // either something is in the way (some solid) or we are inside the trigger. either requires an extra check in classical way.
			while (!finished && num < MAX_GENTITIES) {
				memset(&trace, 0, sizeof(trace));
				if (reverse) {
					// use precise non-epsilon trace here or we can end up with a hit if we are technically outside the bounds of 
					// the target brush but the brush side is within 0.125f of the ending. this makes entitycontact return false
					// even though we hit it. it also means that traces may only find something in one direction, but not the other,
					// because this "advantage" goes only in one direction.
					JP_TracePrecise(&trace, ent->client->postPmovePosition, minsPlayer, maxsPlayer, ent->client->prePmovePosition, ent->client->ps.clientNum, CONTENTS_TRIGGER|CONTENTS_SOLID);
				}
				else {
					JP_TracePrecise(&trace, ent->client->prePmovePosition, minsPlayer, maxsPlayer, ent->client->postPmovePosition, ent->client->ps.clientNum, CONTENTS_TRIGGER | CONTENTS_SOLID);
				}
				somethingInTheWay = trace.allsolid || trace.startsolid || (trace.contents & CONTENTS_SOLID);
				if (trace.fraction < 1.0f && !somethingInTheWay) { //startsolid and allsolid don't return a valid entityNum
					hit = &g_entities[trace.entityNum];
					hit->r.contents &= ~CONTENTS_TRIGGER; // exclude it from next trace.
					touch[num++] = trace.entityNum;
					touchViaTrace[trace.entityNum] = qtrue;
				}
				else {
					if (reverse) {
						finished = qtrue;
					}
					else {
						reverse = qtrue;
					}
					if (somethingInTheWay) {
						needExtraCheck = qtrue;
					}
				}
			}
		}
		else
		{
			needExtraCheck = qtrue;
		}
		numTraced = num;
		if (needExtraCheck) {
			int num2;
			int	touch2[MAX_GENTITIES];
			qboolean preContact;
			qboolean postContact;
			VectorMin(minsPrev, mins, minsTotal);
			VectorMax(maxsPrev, maxs, maxsTotal);
			// basically do the oldschool one after all... this is needed for anything we are fully inside of or if any solids were involved
			num2 = trap_EntitiesInBox(minsTotal, maxsTotal, touch2, MAX_GENTITIES); // this is guaranteed to get the remaining stuff because 
			for (i = 0; i < num2; i++) {
				hit = &g_entities[touch2[i]];
				if (!(hit->r.contents & CONTENTS_TRIGGER)) { 
					continue; // no need to worry about dupes since we removed CONTENTS_TRIGGER from the already done ones
				}
				preContact = trap_EntityContact(minsPrev, maxsPrev, hit);
				postContact = trap_EntityContact(mins, maxs, hit);
				if (preContact!= postContact) {
					touchViaTrace[touch2[i]] = qtrue; // sorta. really what touchviatrace is supposed to mean is, we either went in or got out. this serves that.
				}
				else if (!preContact) {
					continue;
				}
				touch[num++] = touch2[i];
				if (num == MAX_GENTITIES) break; // oh well :(
			}
		}

		for (i = 0; i < numTraced; i++) {
			hit = &g_entities[touch[i]];
			hit->r.contents |= CONTENTS_TRIGGER; // give back the content flag.
		}
		
		// put them all in the right order so it respects order of entities in map
		if (!needExtraItemsPass) { // items pass will do it itself
			qsort(touch, num, sizeof(touch[0]), int_cmp);
		}

	}
	else {
		if (wantRobustTriggers) {
			VectorAdd(ent->client->ps.origin, playerMinsDefault, mins);
			VectorAdd(ent->client->ps.origin, playerMaxsDefault, maxs);
		}
		else {
			VectorSubtract(ent->client->ps.origin, range, mins);
			VectorAdd(ent->client->ps.origin, range, maxs);
			needExtraItemsPass = qfalse;
		}

		num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

		// can't use ent->r.absmin, because that has a one unit pad
		VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
		VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs); // TODO uhm how does this relate to g_triggersrobust? think about this...
	}

	if (needExtraItemsPass) {
		// vanilla item touch range is not what you'd expect from their actual r.mins/r.maxs bounding boxes.
		// item bounding boxes are really tiny, around radius 8 or so.
		// but their search detection radius is very big (see 'range' var in this function)
		// and BG_PlayerTouchesItem adds even more confusion into the mix, as it doesn't seem to be coherent with the
		// range var here, and also seemingly confused dimension 0 with dimension 2, such that the horizontal detection box
		// is actually an off-center rectangle. 
		// TLDR: to preserve vanilla item touch behavior, we need this extra pass when using g_triggersrobust
		int num2;
		int	touch2[MAX_GENTITIES];
		vec3_t		minsItems, maxsItems;
		VectorSubtract(ent->client->ps.origin, range, minsItems);
		VectorAdd(ent->client->ps.origin, range, maxsItems);
		num2 = trap_EntitiesInBox(minsItems, maxsItems, touch2, MAX_GENTITIES); // this is guaranteed to get the remaining stuff because 
		for (i = 0; i < num2; i++) {
			hit = &g_entities[touch2[i]];
			if (!(hit->r.contents & CONTENTS_TRIGGER) || hit->s.eType != ET_ITEM) {
				continue; // no need to worry about dupes since we removed CONTENTS_TRIGGER from the already done ones
			}
			touchViaTrace[touch2[i]] = qfalse;
			touch[num++] = touch2[i];
			if (num == MAX_GENTITIES) break; // oh well :(
		}
		qsort(touch, num, sizeof(touch[0]), int_cmp);
	}

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if (hit->triggerClientSpecific && hit->parent != ent) continue; // custom checkpoints

		isTraced = robustTriggerEvaluation && touchViaTrace[touch[i]];

		// special kind of trigger (like for defrag start timer) that starts when we leave it.
		// requires robust trigger evaluation
		if (hit->r.contents & CONTENTS_TRIGGER_EXIT) {
			if (!robustTriggerEvaluation || !hit->leave) {
				continue;
			}
			if (trap_EntityContact(mins, maxs, hit)) {
				// Still in trigger, ignore. 
				continue;
			}
			memset(&trace, 0, sizeof(trace)); // what is this even for?

			if (isTraced || !hit->triggerOnlyTraced) {
				if (hit->leave) {
					hit->leave(hit, ent, &trace);
				}
			}
			continue;
		}

		// rest is mostly normal trigger code
		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER )) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) { // TODO should it be based on client's cmd serverTime for raceMode?
				continue;
			}
		} else {
			if (!robustTriggerEvaluation && !trap_EntityContact( mins, maxs, hit )) { // no need with robust trigger evaluation, we already checked via trace
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if (!hit->triggerOnlyTraced || (isTraced && !trap_EntityContact(minsPrev, maxsPrev, hit))) {
			// if trigger requires only traced, this means we should only hit this trigger when ENTERING it. aka we are in the trigger now, but weren't on last frame.
			// so if the trigger is traced (not allsolid), and we are not in it right now, then we are exiting it, but hit->triggerOnlyTraced should only be hit when entering a trigger.
			// hence, with robust trigger evaluation, we skip when the trigger requires traced touch and we are exiting instead of entering it. 

			if (hit->touch) {
				hit->touch(hit, ent, &trace);
			}

			hit->triggerLastPlayerContact[ent - g_entities] = nowTime;
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}


/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_MoverTouchPushTriggers( gentity_t *ent, vec3_t oldOrg ) 
{
	int			i, num;
	float		step, stepSize, dist;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs, dir, size, checkSpot;
	const vec3_t	range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if ( !VectorLengthSquared( ent->s.pos.trDelta ) ) 
	{
		return;
	}

	VectorSubtract( ent->r.mins, ent->r.maxs, size );
	stepSize = VectorLength( size );
	if ( stepSize < 1 )
	{
		stepSize = 1;
	}

	VectorSubtract( ent->r.currentOrigin, oldOrg, dir );
	dist = VectorNormalize( dir );
	for ( step = 0; step <= dist; step += stepSize )
	{
		VectorMA( ent->r.currentOrigin, step, dir, checkSpot );
		VectorSubtract( checkSpot, range, mins );
		VectorAdd( checkSpot, range, maxs );

		num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

		// can't use ent->r.absmin, because that has a one unit pad
		VectorAdd( checkSpot, ent->r.mins, mins );
		VectorAdd( checkSpot, ent->r.maxs, maxs );

		for ( i=0 ; i<num ; i++ ) 
		{
			hit = &g_entities[touch[i]];

			if ( hit->s.eType != ET_PUSH_TRIGGER )
			{
				continue;
			}

			if ( hit->touch == NULL ) 
			{
				continue;
			}

			if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) 
			{
				continue;
			}


			if ( !trap_EntityContact( mins, maxs, hit ) ) 
			{
				continue;
			}

			memset( &trace, 0, sizeof(trace) );

			if ( hit->touch != NULL ) 
			{
				hit->touch(hit, ent, &trace);
			}
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal
		client->ps.basespeed = 400;

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		if (ent->client->noclip) {
			pm.tracemask = 0;
		}
		pm.trace = JP_Trace;
		pm.rawtrace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		pm.animations = NULL;

		//Set up bg entity data
		pm.baseEnt = (bgEntity_t*)g_entities;
		pm.entSize = sizeof(gentity_t);
		pm.positionChangedOutsidePmove = !VectorCompare(ent->client->ps.origin, client->oldPostPmovePosition);
		pm.oldButtons = ent->client->oldbuttons;
		pm.unlockRandom = g_unlockRandom.integer;

		pm.highFpsFix = g_fixHighFPSAbuse.integer;

		pm.mod = SVMOD_TOMMYTERNAL;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );
		VectorCopy( client->ps.origin, client->oldPostPmovePosition ); // for q2 snapping mode

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}

	// alt attack button cycles backwards
	if ( (client->sess.amflags & AMFLAG_ALTFOLLOW) && client->sess.spectatorState == SPECTATOR_FOLLOW && (client->buttons & BUTTON_ALT_ATTACK) && !(client->oldbuttons & BUTTON_ALT_ATTACK) )
	{
		Cmd_FollowCycle_f( ent, -1 );
	}

	if (client->sess.spectatorState == SPECTATOR_FOLLOW && (ucmd->upmove > 0))
	{ //jump now removes you from follow mode
		StopFollowing(ent);
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			G_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" ,qtrue);
		}
	}
	return qtrue;
}

/*
=================
ClientInactivitySpecTimerReset

Call manually to reset the timer for sending a player to spec. E.g. on /kill
=================
*/
void ClientInactivitySpecTimerReset(gentity_t* ent, int delay) {
	gclient_t* client = ent->client;
	int oldTime = client->pers.inactivityToSpecTime;
	if (delay) {
		client->pers.inactivityToSpecTime = clampedIntAdd(level.time, delay);
	}
	else if (g_inactivityToSpec.integer <= 0) {
		// give everyone some time, so if the operator sets g_inactivityToSpec during
		// gameplay, everyone isn't spectated
		client->pers.inactivityToSpecTime = clampedIntAdd(level.time, 10 * 1000);
	}
	else {
		client->pers.inactivityToSpecTime = clampedIntAdd(level.time, clampedIntMult(g_inactivityToSpec.integer, 1000)); // hmm why this number?
	}
	if (oldTime > client->pers.inactivityToSpecTime) {
		client->pers.inactivityToSpecTime = oldTime;
	}
}

/*
=================
ClientInactivitySpecTimer

Returns qfalse if the client is put to spec
=================
*/
qboolean ClientInactivitySpecTimer( gentity_t* ent ) {
	gclient_t* client = ent->client;
	int afkTime = clampedIntAdd(level.time, -client->sess.lastHereTime);
	int afkSpecTime = clampedIntMult(g_inactivityToSpec.integer, 1000);
	qboolean clientIsAfk = g_inactivityToSpec.integer && afkTime >= afkSpecTime;
	qboolean wasInactive = client->markedAsInactive;
	client->markedAsInactive = qfalse;

	// 2026-04-17 new logic: unifying afk detection. lastHereTime is the authoritative source for deciding if someone's afk. 
	// inactivityToSpecTime is still used to prevent sending people to spec early in some situations. (segmented replay finished etc)

	if (g_inactivityToSpec.integer <= 0 || client->sess.sessionTeam == TEAM_SPECTATOR || level.intermissiontime) {
		// give everyone some time, so if the operator sets g_inactivityToSpec during
		// gameplay, everyone isn't spectated
		client->pers.inactivityToSpecTime = clampedIntAdd(level.time, 10 * 1000); // inactivityToSpecTime
	}
	//else if (client->pers.cmd.forwardmove ||
	//	client->pers.cmd.rightmove ||
	//	client->pers.cmd.upmove ||
	//	(client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))) {
	//	client->inactivityToSpecTime = clampedIntAdd( level.time , clampedIntMult(g_inactivityToSpec.integer, 1000));
	//}
	else if(level.numPlayingClients > 1){ // dont bother sending to spec or marking as inactive if only 1 player is in anyway
		if (ent->client->sess.raceMode && (ent->client->pers.raceStartCommandTime || ent->client->pers.recordingDemo && ent->client->pers.keepDemoMaybe) && !g_inactivityToSpecRacers.integer) {
			// dont spec ppl in the middle of a run, but mark them as inactive
			if (level.time > client->pers.inactivityToSpecTime && clientIsAfk) {
				client->markedAsInactive = qtrue;
			}
		}
		else {
			if (clientIsAfk) {
				G_Printf("^3g_inactivityToSpec: Sending client %d to spec.\n", (int)(ent - g_entities));
				trap_SendServerCommand(-1, va("print \"^3Sending %s ^3to spec for being AFK.\n\"",client->pers.netname));
				SetTeam(ent, "s");
				return qfalse;
			}

			if (afkTime > afkSpecTime - 20000 && (level.time - client->randomLastCenterprint > 1000 || level.time < client->randomLastCenterprint)) {
				client->randomLastCenterprint = level.time;
				G_CenterPrint(client - level.clients, 3, va("^1%d seconds until you are sent to spec for being AFK!", (afkSpecTime - afkTime) / 1000), qfalse, qtrue, qfalse, NULL);
			}
		}
	}
	if (client->markedAsInactive != wasInactive) {

		G_Printf("^3g_inactivityToSpec: Client %d inactivity status changed to %d.\n", (int)(ent - g_entities), client->markedAsInactive);
		G_ResetClientVote(client);
		CalculateRanks(); // need to let the game know this client won't vote :)
	}
	return qtrue;
}

void SetClientPhysicsFps(gentity_t* ent, int clientSetting);

static qboolean ClientCheckNotifyPhysicsFps(gentity_t* ent) {
	gclient_t* client = ent->client;
	const char* notification = NULL;
	int timeToNextChange = 0;

	if (!client) return qfalse;

	if (client->sess.raceMode) {

		if (client->pers.physicsFps.clientSetting && client->pers.physicsFps.clientSetting == client->pers.physicsFps.acceptedSetting && (client->sess.raceStyle.msec < 0 || client->sess.raceStyle.msec == client->pers.physicsFps.acceptedSettingMsec)) {

			// All good.
			return qtrue;
		}
		else if(client->pers.physicsFps.clientSettingValid) {

			// If the new requested value was valid in principle, recheck if we are outside of a run now to set it
			if (client->sess.raceStyle.msec < 0 || !client->pers.raceStartCommandTime) {
				// Give it another try.
				SetClientPhysicsFps(ent, client->pers.physicsFps.clientSetting);
			}
		}

		if (client->sess.raceStyle.msec < 0) {
			return qtrue; // -1 is toggle, -2 is float. Don't care, anything is allowed.
		}

		// anything below this assumes toggle mode is not active

		// Time to notify the client if something isn't right.
		//if ((client->pers.physicsFps.lastNotification + 1000) > level.time && client->pers.physicsFps.lastNotification < level.time) {
		//	return; // Don't spam. Once every 1 second is enough to stay constant on the screen of the client
		//}
		if (client->sess.sessionTeam == TEAM_SPECTATOR) {
			return qtrue; // We don't enforce anything on spectators
		}
		if (!client->pers.physicsFps.clientSetting) {
			//notification = "cp \"^2Toggle disabled.\n^7Please set com_physicsFps to a valid \nFPS setting you wish to play with\nor use ^2/togglefps ^7for toggle mode.\n\"";
			if (client->pers.physicsFps.clientSendsPhysicsFps) {
				notification = "cp \"^2Toggle disabled.\n^1Invalid ^7com_physicsFps value detected.\nYour movement may be restricted.\nPlease set a valid value\nor use ^2/togglefps ^7for toggle mode.\n\"";
			}
			else {
				notification = "cp \"^2Toggle disabled.\n^1No ^7com_physicsFps value detected.\nYour movement may be restricted.\nPlease use a client with com_physicsFps\n^3(dev note: must be CVAR_USERINFO)\n^7or use ^2/togglefps ^7for toggle mode.\n\"";
			}
		}
		else if (client->pers.physicsFps.clientSetting != client->pers.physicsFps.acceptedSetting) {
			if (!client->pers.physicsFps.clientSettingValid) {
				// Seems like the client set an invalid value as it hasn't been accepted
				if (client->pers.physicsFps.clientSendsPhysicsFps) {
					notification = "cp \"^2Toggle disabled.\n^1Invalid ^7com_physicsFps value detected.\nYour movement may be restricted.\nPlease set a valid value\nor use ^2/togglefps ^7for toggle mode.\n\"";
				}
				else {
					notification = "cp \"^2Toggle disabled.\n^1No ^7com_physicsFps value detected.\nYour movement may be restricted.\nPlease use a client with com_physicsFps\n^3(dev note: must be CVAR_USERINFO)\n^7or use ^2/togglefps ^7for toggle mode.\n\"";
				}
			}
			else {
				if (client->pers.physicsFps.acceptedSetting) {
					notification = va("cp \"^2Toggle disabled.\n^7End your run to change com_physicsFps. \nPlease go back to %d fps.\n\"", client->pers.physicsFps.acceptedSetting);
				}
				else {
					// Should never happen?
					// Somehow we have a valid client setting, no accepted setting yet, and yet the value was not formally accepted.
					// Only adding this for debugging in case strange things happen.
					notification = va("cp \"^2Toggle disabled.\n^7Anomaly detected. Please try setting com_physicsFps again\n and respawn.\n\"");
				}
			}
		}

	}
	else {
		
		if (!g_fpsToggleDelay.integer) {
			return qtrue; // We are not limiting anything, don't care.
		}
	
		if (client->pers.physicsFps.clientSetting && client->pers.physicsFps.clientSetting == client->pers.physicsFps.acceptedSetting) {

			// All good.
			return qtrue;
		}
		else if(client->pers.physicsFps.clientSettingValid) {

			// If the new requested value was valid in principle, recheck if enough time has passed now to accept the client's new com_physicsFps setting.
			if ((client->pers.physicsFps.lastChange + g_fpsToggleDelay.integer * 1000) < level.time || client->pers.physicsFps.lastChange > level.time) {
				// Give it another try.
				SetClientPhysicsFps(ent, client->pers.physicsFps.clientSetting);
			}
		}

		// Time to notify the client if something isn't right.
		//if ((client->pers.physicsFps.lastNotification + 1000) > level.time && client->pers.physicsFps.lastNotification < level.time) {
		//	return; // Don't spam. Once every 1 second is enough to stay constant on the screen of the client
		//}
		if (client->sess.sessionTeam == TEAM_SPECTATOR) {
			return qtrue; // We don't enforce anything on spectators
		}
		if (!client->pers.physicsFps.clientSetting) {
			notification = "cp \"^2Anti-Toggle active.\n^7Please set com_physicsFps to a valid \nFPS setting you wish to play with.\n\"";
		}
		else if (client->pers.physicsFps.clientSetting != client->pers.physicsFps.acceptedSetting) {
			if (!client->pers.physicsFps.clientSettingValid) {
				// Seems like the client set an invalid value as it hasn't been accepted
				notification = "cp \"^2Anti-Toggle active.\n^1Invalid ^7com_physicsFps value detected. \nPlease set a valid value.\n\"";
			}
			else {
				timeToNextChange = level.time > client->pers.physicsFps.lastChange ? g_fpsToggleDelay.integer*1000 - (level.time - client->pers.physicsFps.lastChange) : -1;
				if (client->pers.physicsFps.acceptedSetting) {
					notification = va("cp \"^2Anti-Toggle active.\n^7Next com_physicsFps change allowed in %d seconds. \nPlease go back to %d fps.\n\"", timeToNextChange / 1000, client->pers.physicsFps.acceptedSetting);
				}
				else {
					// Should never happen?
					// Somehow we have a valid client setting, no accepted setting yet, and yet the value was not formally accepted.
					// Only adding this for debugging in case strange things happen.
					notification = va("cp \"^2Anti-Toggle active.\n^7Anomaly detected. Please try setting com_physicsFps again \n(time to next allowed change: %d).\n\"", timeToNextChange);
				}
			}
		}
	}

	if (notification) {
		if ((client->pers.physicsFps.lastNotification + 1000) > level.time && client->pers.physicsFps.lastNotification <= level.time) {
			return qfalse; // Don't spam. Once every 1 second is enough to stay constant on the screen of the client
		}
		else {
			G_SendServerCommand(client - level.clients, notification,qtrue);
			client->pers.physicsFps.lastNotification = level.time;
		}
		return qfalse;
	}
	else {
		return qtrue;
	}
}

#define IRONMAN_DANGERPOINTS_MAXDISTANCE 350.0f
#define IRONMAN_DANGERPOINTS_MSECS_PER_POINT 10000
void IronmanDangerPoints(gentity_t* self, int msec) {
	static vec3_t	playerMins = { -15, -15, DEFAULT_MINS_2 };
	static vec3_t	playerMaxs = { 15, 15, DEFAULT_MAXS_2 };
	int i;

	if (self->client->sess.mode != MODE_IRONMAN || self->client->sess.modeTeam != MODETEAM_IRONMAN_CAPPER || self->health <= 0 || clampedIntAdd(level.time, -self->client->sess.lastHereTime) > 10000) {
		return;
	}
	
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gentity_t* ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;
		trace_t   trace;

		if (ent == self)
		{
			continue;
		}

		if (ent->client->sess.mode != MODE_IRONMAN || clampedIntAdd(level.time, -ent->client->sess.lastHereTime) > 10000) {
			// ignore non-ironmanners and afk ppl
			continue;
		}

		VectorSubtract(self->client->ps.origin, ent->client->ps.origin, angles);
		dist = VectorLengthSquared(angles);

		if (dist > IRONMAN_DANGERPOINTS_MAXDISTANCE * IRONMAN_DANGERPOINTS_MAXDISTANCE)
		{
			continue;
		}

		// check if this played could hit the ironman
		// we err to the side of being restrictive
		// if theres water, clips or solid stuff inbetween, no points!
		JP_Trace(&trace, ent->client->ps.origin, playerMins, playerMaxs, self->client->ps.origin, ent-g_entities, MASK_DEADSOLID | MASK_WATER | CONTENTS_NOSPAWN);

		if ((trace.fraction < 1.0f || trace.allsolid || trace.startsolid) && trace.entityNum != (self - g_entities)) {
			// we hit sth else...
			continue;
		}

		self->client->ironmanDangerPoints += msec;
	}
	if (self->client->ironmanDangerPoints >= IRONMAN_DANGERPOINTS_MSECS_PER_POINT) {
		// we have accumulated 10000 msecs (10 seconds) with nearby enemies)
		// give points
		int pointsToAward = self->client->ironmanDangerPoints / IRONMAN_DANGERPOINTS_MSECS_PER_POINT;
		self->client->ironmanDangerPoints -= pointsToAward * IRONMAN_DANGERPOINTS_MSECS_PER_POINT;
		AddScore(self, self->client->ps.origin, pointsToAward);
	}
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	if (g_pauseGame.integer && !ent->client->sess.raceMode) {
		return;
	}

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) 
	{
		client->timeResidual -= 1000;

		// count down health when over max
		if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
			if (ent->health != client->ps.stats[STAT_HEALTH] || !(g_teamOverlayDynamicIgnoreDecay.integer & 1)) {
				// this is a bit cringe. idk. normally STAT_HEALTH isnt set here. 
				// i basically wanna check if the ONLY change here is the reduction by 1
				// so we can ignore decay in the teaminfo messages if desired.
				// for this reason i also need to set STAT_HEALTH below (which it normally isnt)
				// so that it doesnt trigger the new teaminfo message on its own when STAT_HEALTH = ent->health at a later point.
				// however ent->health may have changed already in some other place, and that SHOULD trigger the teaminfo message.
				// hence the condition... really dumb
				level.teamLocationChanged |= (1 << client->sess.sessionTeam);
			}
			ent->health--;
			client->ps.stats[STAT_HEALTH] = ent->health; // i hope this is safe. SP does it but we dont normally?
		}

		// count down armor when over max
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
			if (!(g_teamOverlayDynamicIgnoreDecay.integer & 1)) {
				level.teamLocationChanged |= (1 << client->sess.sessionTeam);
			}
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;//, j;
	int		event;
	gclient_t *client;
	int		damage;
//	vec3_t	origin, angles;
//	qboolean	fired;
//	gitem_t *item;
//	gentity_t *drop;
	int nowTime = LEVELTIME(ent->client);

	client = ent->client;

	if ( !client ) return;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL:
		case EV_ROLL:
			{
				int delta = client->ps.eventParms[ i & (MAX_PS_EVENTS-1) ];

				if ( client->ps.fallingToDeath )
				{
					break;
				}

				if (client && client->sess.raceMode)
					break;

				if ( ent->s.eType != ET_PLAYER )
				{
					break;		// not in the player model
				}
				
				if ( g_dmflags.integer & DF_NO_FALLING )
				{
					break;
				}

				if (delta <= 44)
				{
					break;
				}

				damage = delta*0.16; //good enough for now, I guess

				ent->pain_debounce_time = nowTime + 200;	// no normal pain sound
				G_Damage (ent, NULL, NULL, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_FALLING);
			}
			break;
		case EV_FIRE_WEAPON:
			FireWeapon( ent, qfalse );
			ent->client->dangerTime = nowTime;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_ALT_FIRE:
			FireWeapon( ent, qtrue );
			ent->client->dangerTime = nowTime;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_SABER_ATTACK:
			ent->client->dangerTime = nowTime;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		//rww - Note that these must be in the same order (ITEM#-wise) as they are in holdable_t
		case EV_USE_ITEM1: //seeker droid
			ItemUse_Seeker(ent);
			break;
		case EV_USE_ITEM2: //shield
			ItemUse_Shield(ent);
			break;
		case EV_USE_ITEM3: //medpack
			ItemUse_MedPack(ent);
			break;
		case EV_USE_ITEM4: //datapad
			//G_Printf("Used Datapad\n");
			break;
		case EV_USE_ITEM5: //binoculars
			ItemUse_Binoculars(ent);
			break;
		case EV_USE_ITEM6: //sentry gun
			ItemUse_Sentry(ent);
			break;

		default:
			break;
		}
	}

}

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, g_snapPlayerPosAngles.integer);
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

extern int saberOffSound;
extern int saberOnSound;

/*
==================
G_UpdateClientBroadcasts

Determines whether this client should be broadcast to any other clients.  
A client is broadcast when another client is using force sight or is
==================
*/
#define MAX_JEDIMASTER_DISTANCE	2500
#define MAX_JEDIMASTER_FOV		100

#define MAX_SIGHT_DISTANCE		1500
#define MAX_SIGHT_FOV			100


// careful, does not exclude self. need to do that manually
void G_SetSpecAllEntsBroadcasts( int broadcastClients[2] )
{
	int i;

	//broadcastClients[0] = broadcastClients[1] = 0;

	if (!g_specAllEnts.integer || g_sv_specAllEnts.integer && g_sv_specAllEnts.string[0]) {
		// if g_specAllEnts is off, or sv_specAllEnts is on, we don't need this
		return;
	}

	// Any clients with force sight on should see this client
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) { // don't create a wallhack for non-spectators
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	}
}

static void G_UpdateSpecAllEntsBroadcasts ( gentity_t *self )
{
	int i;

	G_SetSpecAllEntsBroadcasts(self->r.broadcastClients);
	self->r.broadcastClients[self->s.number / 32] &= ~(1 << (self->s.number % 32));
}

static void G_UpdateForceSightBroadcasts ( gentity_t *self )
{
	int i;

	// Any clients with force sight on should see this client
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;
	
		if ( ent == self )
		{
			continue;
		}

		// Not using force sight so we shouldnt broadcast to this one
		if ( !(ent->client->ps.fd.forcePowersActive & (1<<FP_SEE) ) )
		{
			continue;
		}

		VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_SIGHT_DISTANCE * MAX_SIGHT_DISTANCE )
		{
			continue;
		}
		
		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_SIGHT_FOV, angles ) )
		{
			//break;
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	
		//break; //TA: WTF why break?
	}
}

static void G_UpdateJediMasterBroadcasts ( gentity_t *self )
{
	int i;

	// Not jedi master mode then nothing to do
	if ( g_gametype.integer != GT_JEDIMASTER )
	{
		return;
	}

	// This client isnt the jedi master so it shouldnt broadcast
	if ( !self->client->ps.isJediMaster )
	{
		return;
	}

	// Broadcast ourself to all clients within range
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_JEDIMASTER_DISTANCE * MAX_JEDIMASTER_DISTANCE )
		{
			continue;
		}
		
		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_JEDIMASTER_FOV, angles ) )
		{
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	}
}

static void G_UpdateIronmanBroadcasts ( gentity_t *self )
{
	int i;

	// Not iron man mode then nothing to do
	if ( self->client->sess.mode != MODE_IRONMAN )
	{
		return;
	}

	// This client isnt the ironman so it shouldnt broadcast
	if ( self->client->sess.modeTeam != MODETEAM_IRONMAN_CAPPER )
	{
		// check if we are at least close to the ironman
		if (level.ironManCurrentPositionSet) {
			vec3_t delta;
			VectorSubtract(level.ironManCurrentPosition.origin,self->client->ps.origin,delta);
			if (VectorLengthSquared(delta) > IRONMAN_NEARBYBROADCASTRANGE* IRONMAN_NEARBYBROADCASTRANGE) {
				// broadcast players near the ironman as well. so demos dont miss out on any cool dbs :)
				// we don't rly care about wallhack or anything in ironman anyway
				return;
			}
		}
		else {
			return;
		}
	}

	// Broadcast ourself to all iron manners
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		if (ent->client->sess.mode != MODE_IRONMAN) {
			continue;
		}

		// for ironman ignore distance. does it matter?
		/*VectorSubtract(self->client->ps.origin, ent->client->ps.origin, angles);
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_JEDIMASTER_DISTANCE * MAX_JEDIMASTER_DISTANCE )
		{
			continue;
		}
		
		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_JEDIMASTER_FOV, angles ) )
		{
			continue;
		}*/

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	}
}

static void G_UpdateDuelQueueBroadcasts ( gentity_t *self )
{
	int i;

	// Not duel queue mode then nothing to do
	if ( self->client->sess.mode != MODE_DUELQUEUE )
	{
		return;
	}

	// This client isnt in a duel so it shouldnt broadcast
	if ( !self->client->ps.duelInProgress )
	{
		// actually... let ppl waiting around see everything, why not
		//return;
	}

	// Broadcast ourself to all iron manners
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		if (ent->client->sess.mode != MODE_DUELQUEUE) {
			continue;
		}

		// client is busy with his own duel, he doesn't need to see anything else
		if (ent->client->ps.duelInProgress) {
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	}
}

void G_UpdateClientBroadcasts ( gentity_t *self )
{
	static int g_antiWallhackModificationCount = -1;
	if (g_antiWallhack.integer) { // experimental wallhack by bucky, based on some jka stuff
		G_UpdateClientBroadcastsAntiWallhack(self);
		return;
	}
	else if (g_antiWallhack.modificationCount != g_antiWallhackModificationCount) {
		g_antiWallhackModificationCount = g_antiWallhack.modificationCount;
		// antiwallhack was switched off, so clear all the snapshotignore/snapshotenforce stuff
		G_ClearAllAntiWallhackSendStates();
	}

	// Clear all the broadcast bits for this client
	memset ( self->r.broadcastClients, 0, sizeof ( self->r.broadcastClients ) );

	// The jedi master is broadcast to everyone in range
	G_UpdateJediMasterBroadcasts ( self );

	// The ironman is broadcast to everyone
	if (self->client->sess.mode == MODE_IRONMAN) {
		G_UpdateIronmanBroadcasts( self );
	}

	// players in duelqueue get broadcast to anyone who is waiting for a duel, so he doesn't get bored.
	if (self->client->sess.mode == MODE_DUELQUEUE) {
		G_UpdateDuelQueueBroadcasts( self );
	}

	// Anyone with force sight on should see this client
	G_UpdateForceSightBroadcasts ( self );

	// If spec all ents is active, let anyone who is a spectator view this client always
	// Not as good as sv_specAllEnts, we can't make this work for follow spectators
	// but decent in case engine access is not available.
	G_UpdateSpecAllEntsBroadcasts( self );
}

qboolean DF_PrePmoveValid(gentity_t* ent);

void UpdateClientPastFpsStats(gentity_t* ent, int msec) {
	entityState_t* stats = &level.playerStats[ent-g_entities]->s;
	fpsMeasure_t* fpsMeasure = &ent->client->pers.fpsMeasure;
	ent->client->lastMsecValue = msec;
	if (stats->pastFpsUnionArray[0] != msec || // if fps is stable and unchanging, dont spam index changes, what for...
		stats->pastFpsUnionArray[1] != msec ||
		stats->pastFpsUnionArray[2] != msec ||
		stats->pastFpsUnionArray[3] != msec) {

		stats->pastFpsUnionArray[stats->fireflag++] = msec;
		stats->fireflag = stats->fireflag & (PLAYERSTATS_PAST_MSEC - 1);
	}

	fpsMeasure->frameTimes[fpsMeasure->index++ % MAX_FPSMEASURE_FRAMECOUNT] = msec;
	if (fpsMeasure->index < MAX_FPSMEASURE_FRAMECOUNT) return;
	if (ent->client->sess.raceMode && ent->client->pers.raceStartCommandTime) {
		int i;
		int total = 0, totalShort = 0;
		int guess, guessShort;
		for (i = 0; i < MAX_FPSMEASURE_FRAMECOUNT; i++) {
			total += fpsMeasure->frameTimes[i];
		}
		for (i = fpsMeasure->index - MAX_FPSMEASURE_SHORT_FRAMECOUNT; i < fpsMeasure->index; i++) {
			totalShort += fpsMeasure->frameTimes[i % MAX_FPSMEASURE_FRAMECOUNT];
		}
		guess = (int)(roundf((float)total / (float)MAX_FPSMEASURE_FRAMECOUNT) + 0.5f);
		guessShort = (int)(roundf((float)totalShort / (float)MAX_FPSMEASURE_SHORT_FRAMECOUNT) + 0.5f);
		if (guess == guessShort) {
			// we compare a short sequence average to a long sequence average and thus combine their strengths and reduce their weaknesses
			// long sequence is reliable (can even be 0% error if client fps is stable) but fps toggles will cause ~2-5% false readings during the transition
			// short sequences are more dynamic but have an error rate above 0%, up to 5% typically (assuming stable-ish framerate in client again)
			// When we compare the long and short average, we reduce the overall error rate to something like 0.1-0.2% per fps toggle.
			// We lose some samples that might have been good but false readings are significantly reduced.
			ent->client->pers.stats.fpsStats.msecCounts[fpsTableMsecToIndex[MAX(0, MIN(FPSTABLE_OVERFLOW_MSECVALUE, guess))]]++;
			ent->client->pers.stats.fpsStats.totalCount++;
		}
	}

}

// Simplified version of PM_UpdateViewAngles. Just to see where we are looking right now without doing pmove first
void GetUpdatedViewAngles(gentity_t* ent, vec3_t out) {
		short		temp;
		int		i;

		//if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		//	return;		// no view changes at all
		//}

		//if (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0) {
		//	return;		// no view changes at all
		//}

		// circularly clamp the angles with deltas
		for (i = 0; i < 3; i++) {
			temp = ent->client->pers.cmd.angles[i] + ent->client->ps.delta_angles[i];
			if (i == PITCH) {
				if (temp > 16000) {
					temp = 16000;
				}
				else if (temp < -16000) {
					temp = -16000;
				}
			}
			out[i] = SHORT2ANGLE(temp);
		}

}

// sanity check that the client still exists and knows about this checkpoint andsuch.
void checkLaserpointerValid(gentity_t* ent) {
	gentity_t* owner;
	int i;
	int lpNum;

	if (!ent->parent) {
		goto freeme;
		return;
	}
	owner = ent->parent;

	if (!owner->inuse || !owner->client || owner->client->pers.connected != CON_CONNECTED) {
		goto freeme;
		return;
	}

	lpNum = ent - g_entities;
	if (owner->client->pers.laserPointerNum == lpNum) {

		// ok the client is still active and still knows about this checkpoint. keep it.
		ent->think = checkLaserpointerValid;
		ent->nextthink = level.time + 1000;
		return;
	}

freeme:
	ent->think = 0;
	ent->nextthink = 0;
	G_FreeEntity(ent);

}

void ClientLaserPointer(gentity_t* ent) {
	vec3_t viewAngles,fwd,end;
	gentity_t* lp = NULL;
	qboolean eventflip = qfalse;
	trace_t tr;
	int flipDelay = 1000/g_sv_fps.integer;
	if (ent->client->pers.laserPointerNum) {
		lp = g_entities + ent->client->pers.laserPointerNum;
		if (!(lp->s.eType == ET_BEAM && lp->parent == ent && lp->s.generic1 == 3)) { // hmm sth went wrong
			lp = NULL;
			ent->client->pers.laserPointerNum = 0;
		}
	}


	GetUpdatedViewAngles(ent, viewAngles); // less delay than using ps.viewangles because pmove hasnt happened yet
	AngleVectors(viewAngles, fwd, NULL, NULL);
	VectorMA(ent->client->ps.origin,3000.0f, fwd, end);
	JP_Trace(&tr, ent->client->ps.origin, NULL, NULL, end, ent->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);

	if (tr.startsolid) {
		if (lp) {
			G_FreeEntity(lp);
			ent->client->pers.laserPointerNum = 0;
		}
		return;
	}

	if (!lp) {
		lp = G_Spawn();
		ent->client->pers.laserPointerNum = lp - g_entities;
		lp->s.eType = ET_BEAM;
		lp->s.generic1 = 3;
		lp->parent = ent;
		lp->s.owner = ent - g_entities;
	}

	VectorCopy(ent->client->ps.origin,lp->r.currentOrigin);
	VectorCopy(ent->client->ps.origin,lp->s.pos.trBase);
	VectorCopy(ent->client->ps.origin,lp->s.origin);
	VectorCopy(tr.endpos,lp->s.origin2); 

	// these dont need to be that precise. snap them to save network bandwidth.
	trap_SnapVector(lp->s.pos.trBase);
	trap_SnapVector(lp->s.origin);
	trap_SnapVector(lp->s.origin2);


	//mv_entities[ent->client->pers.laserPointerNum].snapshotIgnore;

	eventflip = (level.time >= lp->laserPointerLastEventFlip + flipDelay) || level.time < lp->laserPointerLastEventFlip;
	if (eventflip || !lp->s.event) {
		G_AddEvent(lp, EV_TESTLINE,0);
		lp->laserPointerLastEventFlip = level.time;
	}
	lp->s.time2 = flipDelay*4;
	lp->s.weapon = 0x0000ff;
	lp->eventTime = level.time;
	//lp->r.svFlags |= SVF_BROADCAST;
	
	ent->think = checkLaserpointerValid; // Is this really needed? can we do it better? idk
	ent->nextthink = level.time + 1000;

	trap_LinkEntity(lp);
}
void ClientKillLaserPointer(gentity_t* ent) {
	if (ent->client->pers.laserPointerNum) {
		gentity_t* lp = g_entities + ent->client->pers.laserPointerNum;
		if (lp->s.eType == ET_BEAM && lp->parent == ent && lp->s.generic1==3) { // generic1 == 3 just means its a laserpointer *shrug*
			G_FreeEntity(lp);
		}
		ent->client->pers.laserPointerNum = 0;
	}
}

void HandleClientLaserPointer(gentity_t* ent) {
	if (!g_defrag.integer) {
		return;
	}
	if ((ent->client->pers.cmd.buttons & BUTTON_LASERPOINTER) && ent->client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		ClientLaserPointer(ent);
	}
	else {
		ClientKillLaserPointer(ent);
	}
}

// save positions of the iron man so we can respawn his chasers at a reasonable distance to him and not make them run half a minute to reach him again
void G_MaybeSaveIronmanPos(gentity_t* ent, qboolean touchedGround) {
	gclient_t* client = ent->client;
	vec3_t diffToOld;
	trace_t trace;
	int allowMidJumpSaves = 0;
	static vec3_t	playerMins = { -15, -15, DEFAULT_MINS_2 };
	static vec3_t	playerMaxs = { 15, 15, DEFAULT_MAXS_2 };
	simplePos_t* targetPos = NULL;
	qboolean usingQuickpos = qfalse;

	VectorCopy(client->ps.origin,level.ironManCurrentPosition.origin);
	VectorCopy(client->ps.velocity,level.ironManCurrentPosition.velocity);
	VectorCopy(client->ps.viewangles,level.ironManCurrentPosition.angles);
	level.ironManCurrentPosition.when = level.time;
	level.ironManCurrentPosition.distanceTraveled = client->pers.distanceTraveled;
	level.ironManCurrentPositionSet = qtrue;

	if (g_debugFancy.integer == DEBUG_IRONMANSPAWNS) {
		int i, j = 0;
		simplePos_t* pos;
		float dist;
		for (i = level.ironManPosCount - 1; i >= MAX(0, level.ironManPosCount - IRONMAN_MAX_PAST_POSITIONS_COUNT + 1) && j < IRONMAN_DEBUG_PASTPOINTS; i--, j++) {
			pos = &level.ironManPos[i % IRONMAN_MAX_PAST_POSITIONS_COUNT];
			dist = client->pers.distanceTraveled - pos->distanceTraveled; // quickly find distance traveled since that pos was saved
			G_SetDebugVar(ironmanDebug.points[j].intDistance, 0, dist);
			dist = Distance(pos->origin, client->ps.origin);
			G_SetDebugVar(ironmanDebug.points[j].realDistance, 0, dist);
		}
		for (; j < IRONMAN_DEBUG_PASTPOINTS; j++) {
			pos = &level.ironManPos[i % IRONMAN_MAX_PAST_POSITIONS_COUNT];
			G_SetDebugVar(ironmanDebug.points[j].intDistance, 0, 0);
			G_SetDebugVar(ironmanDebug.points[j].realDistance, 0, 0);
		}
	}

	if((!level.ironManPosCountQuick || level.lastIronManQuickPosSaved + IRONMAN_SAVEPOSITION_QUICK_MINTIME < level.time) && !(client->ps.pm_flags & PMF_JUMP_HELD) && client->pers.cmd.upmove <= 0 && !client->ps.pm_time){ // quick pos saving
		simplePos_t* targetPosQuick = &level.ironManPosQuick[level.ironManPosCountQuick % IRONMAN_MAX_PAST_POSITIONS_COUNT_QUICK];
		VectorCopy(client->ps.origin, targetPosQuick->origin);
		VectorCopy(client->ps.velocity, targetPosQuick->velocity);
		VectorCopy(client->ps.viewangles, targetPosQuick->angles);
		targetPosQuick->when = level.time;
		targetPosQuick->distanceTraveled = client->pers.distanceTraveled; // this is an integer distance traveled. to keep a permanent distance traveled counter that never has to be reset and isnt affected by floating point imprecision
		level.ironManPosCountQuick++;
		level.lastIronManQuickPosSaved = level.time;
	}

	if (client->sess.modeTeam != MODETEAM_IRONMAN_CAPPER || client->sess.mode != MODE_IRONMAN || !touchedGround && level.lastIronManPosSaved + IRONMAN_SAVEPOSITION_MINTIMEFORCESURELYEVENJUMP > level.time) {
		return;
	}

	if (level.ironManPosCount > 0) {
		VectorSubtract(level.ironManPos[(level.ironManPosCount - 1) % IRONMAN_MAX_PAST_POSITIONS_COUNT].origin, client->ps.origin, diffToOld);

		if (level.lastIronManPosSaved + IRONMAN_SAVEPOSITION_MINTIMEFORCE > level.time) {
			// not been long enough since last, so just check if maybe we traveled a long distance, otherwise skip.
			if (VectorLengthSquared(diffToOld) < IRONMAN_SAVEPOSITION_MINDISTANCE * IRONMAN_SAVEPOSITION_MINDISTANCE) {
				return;
			}
		}
		else if (level.lastIronManPosSaved + IRONMAN_SAVEPOSITION_MINTIMEFORCESURELY > level.time) {
			// even if enough time passed, let's make surer we travel at least a BIT.
			// there MIGHT be a way to abuse this with maybe some specially designed complicated labyrinth or sth idk. gonne be mostly ok tho.
			if (VectorLengthSquared(diffToOld) < IRONMAN_SAVEPOSITION_MINDISTANCE_SHORT * IRONMAN_SAVEPOSITION_MINDISTANCE_SHORT) {
				return;
			}
		}
		else if (level.lastIronManPosSaved + IRONMAN_SAVEPOSITION_MINTIMEFORCESURELYEVENJUMP > level.time) {
			// well now TRULY a lot of time passed. just go.
		}
		else if (level.lastIronManPosSaved + IRONMAN_SAVEPOSITION_MINTIMEFORCESURELYEVENJUMP2 > level.time) {
			// well now TRULY a lot of time passed. just go.
			allowMidJumpSaves = 1;
		}
		else {
			// well now TRULY a lot of time passed and somehow jump was still pressed i guess? cant do much then, just go...
			allowMidJumpSaves = 2;
		}
	}
		
	JP_Trace(&trace, client->ps.origin, playerMins, playerMaxs, client->ps.origin,ent-g_entities, MASK_PLAYERSOLID);

	if (trace.allsolid || trace.startsolid) {
		return; // maybe hes crouching somewhere in a hole or sth. Can't save it because anyone spawning would get stuck.
	}

	targetPos = &level.ironManPos[level.ironManPosCount % IRONMAN_MAX_PAST_POSITIONS_COUNT];

	if (level.ironManPosCountQuick) {
		// go back in time up to a second, through the quickly saved positions.
		// logic: we only save main positions when we are on ground.
		// but especially on maps with tricky jumps at higher speed, this means we could spawn with full speed at a place
		// and the logic forces us to jump off, when in reality we needed to pause/groundframe
		// ergo: let us go back in time before reaching the ground, so the player can see whats coming and can react to it
		int i;
		simplePos_t* pos;
		int age;
		for (i = level.ironManPosCountQuick - 1; i >= MAX(0, level.ironManPosCountQuick - IRONMAN_MAX_PAST_POSITIONS_COUNT_QUICK + 1); i--) {
			pos = &level.ironManPosQuick[i % IRONMAN_MAX_PAST_POSITIONS_COUNT_QUICK];
			age = level.time - pos->when;
			if (age > 400 && age < 1000) {
				// anything in sweet spot?
				usingQuickpos = qtrue;
				*targetPos = *pos;
				break;
			}
		}
	}

	if (level.ironManPosCount > 0 && !usingQuickpos) { // quickpos ALWAYS include midjump saves

		// try to avoid saving mid-jump or at the start of a jump, otherwise people spawning will spawn in the middle of what was supposed
		// to be a force jump and then they might fall to their doom.
		// also avoid saving during a skim (since that wont be properly restored by a spawn atm)
		if (allowMidJumpSaves == 0 && ((client->ps.pm_flags & PMF_JUMP_HELD) || client->pers.cmd.upmove > 0 || client->ps.pm_time)) {
			return;
		}
		else if (allowMidJumpSaves == 1 && (client->ps.pm_flags & PMF_JUMP_HELD)) {
			return;
		}
	}

	if (!usingQuickpos) {
		VectorCopy(client->ps.origin, targetPos->origin);
		VectorCopy(client->ps.velocity, targetPos->velocity);
		VectorCopy(client->ps.viewangles, targetPos->angles);
		targetPos->when = level.time;
		targetPos->distanceTraveled = client->pers.distanceTraveled; // this is an integer distance traveled. to keep a permanent distance traveled counter that never has to be reset and isnt affected by floating point imprecision
	}

	if (g_developer.integer) {
		Com_Printf("Saving ironman position of age %d: %f %f %f", level.time - targetPos->when, targetPos->origin[0], targetPos->origin[1], targetPos->origin[2]);
	}

	level.ironManPosCountQuick = 0; // clear them now to avoid weirdness
	level.ironManPosCount++;
	level.lastIronManPosSaved = level.time;

}

void DF_CheckRollSpeed(gentity_t* ent);

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void DF_HandleSegmentedRunPre(gentity_t* ent);
posHashType_t DF_GetPositionHash(playerState_t* ps);
//void DF_SetRaceMode(gentity_t* ent, qboolean value); 
void ResetClientModeIfInvalid(gentity_t* ent, qboolean allowDefrag);
void DF_RaceStateInvalidated(gentity_t* ent, qboolean print);
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	gentity_t	*otherClientEnt;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	int			i;
	usercmd_t	*ucmd;
	//int			oldTeleportBit;
	int			nowTime = LEVELTIME(ent->client); // when racing, make everything relative to commandtime
	int	moveStyle;
	qboolean	clientFpsOk;
	qboolean	inactivityToSpec = qfalse;
	vec3_t		prePmoveVelocity;
	float		forcedModelScale = 0.0f;
	int			oldSaberMove;
	qboolean	pauseItemUse;

	client = ent->client;

	if ( !ent || !ent->client ) return;

	G_BufferedSendOrPrintFlushIfNeeded(ent,qfalse, qfalse);

	for (i = 0; i < level.maxclients; i++) {
		otherClientEnt = g_entities + i;
		// this is for snapshot limiting (avoiding dupe snapshots when no commandtime update has happened for smoother demos)
		// if we are in follow mode, we wont set it for ourselves otherwise we might override the snapshot limiting. it will think something moved,
		// but it was just us as spectator sending usercmds that did nothing relevant. dont set it for others either then i guess?
		// also if we are a free floating spectator, don't count our movement for others
		if (!otherClientEnt->client || !otherClientEnt->inuse || (/*otherClientEnt == ent &&*/ ent->client->sess.spectatorState == SPECTATOR_FOLLOW) || (ent->client->sess.sessionTeam == TEAM_SPECTATOR && otherClientEnt != ent)) continue;
		otherClientEnt->client->anyClientMovedSinceSnapshot = qtrue;
	}

	if (ent->client->sess.rollAngleInvalidated) {
		if (!ent->client->pers.cmd.angles[ROLL]) {
			ent->client->sess.rollAngleInvalidated = qfalse;
			DF_RaceStateInvalidated(ent, qtrue);
		}
		ent->client->pers.cmd.angles[ROLL] = 0;
	}


	moveStyle = client->sess.raceMode ? client->sess.raceStyle.movementStyle : MV_JK2;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	DF_HandleSegmentedRunPre(ent);

	// moved this under DF_HandleSegmentedRunPre. makes more sense?
	if (ent->client->sess.raceMode) { // in racemode we want all things to be consistent and deterministic, so we do this on every CLIENT frame and change level.time references to cmd.servertime where possible
		// uh should we do this after handlesegmentedrunpre?
		if ((!level.intermissiontime) && !(ent->client->ps.pm_flags & PMF_FOLLOW) && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
		}
	}

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - (ent->client->sess.raceMode ? 1500:1000) ) { // bit higher limit for racemoders to avoid extreme lags killing runs
		if (ent->client->sess.raceMode && (ent->client->sess.raceStyle.msec == 1000 || ent->client->sess.raceStyle.msec == -1) && (ucmd->serverTime - client->ps.commandTime) == 1000 && ucmd->serverTime >= level.time - 2000) {
			// special exception for 1 fps defragging (for very funny people)
			// increase the limit even more for them
		}
		else {
			ucmd->serverTime = level.time - (ent->client->sess.raceMode ? 1500 : 1000);
		}
//		G_Printf("serverTime >>>>>\n" );
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200) {
		if (ent->client->sess.raceMode && (ent->client->sess.raceStyle.msec > 200 || ent->client->sess.raceStyle.msec == -1) ) {
			// racemode is more lenient, we wanna allow 1 fps runs and nonsense like that
			if (msec > 1000) {
				msec = 1000;
			}
		}
		else {
			msec = 200;
		}
	}

	level.playerStats[ent - g_entities]->s.eFlags &= ~PF_RENDER_ZCOMP;
	if (moveStyle == MV_RATS) {
		forcedModelScale = 0.05f;
	}
	else if (moveStyle == MV_Q2) {
		forcedModelScale = 0.875f;
		level.playerStats[ent - g_entities]->s.eFlags |= PF_RENDER_ZCOMP; // q2 keeps the -24 MINS_Z so we gotta compensate when rendering
	}
	level.playerStats[ent - g_entities]->s.pos.trBase[2] = forcedModelScale;
	
	if (g_defrag.integer && client->sess.raceMode && ent->activatedEntities) {
		gentity_t* actEnt = ent->activatedEntities;
		while (actEnt) {
			if (actEnt->s.eType == ET_MOVER && actEnt->inuse && !ent->freeAfterEvent && (ent->r.linked || !ent->neverFree) ) {
				// run movers on client time
				G_RunMover(actEnt);
			}
			actEnt = actEnt->nextActivatedEntity;
		}

	}

	if ( g_pmove_msec.integer < 1 ) {
		trap_Cvar_Set("pmove_msec", "1");
	}
	else if (g_pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if ( g_pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + g_pmove_msec.integer-1) / g_pmove_msec.integer) * g_pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//if (client->sess.sessionTeam == TEAM_FREE && !g_defrag.integer) {
	//	if (client->sess.raceMode || g_gametype.integer >= GT_TEAM) {
	//		SetTeam(ent, "spectator");// , qtrue);
	//		DF_SetRaceMode(ent,qfalse);
	//	}
	//}
	if (client->sess.sessionTeam == TEAM_FREE) {
		ResetClientModeIfInvalid(ent, (qboolean)!(ent->r.svFlags& SVF_BOT));
	}

	//if (client->ps.stats[STAT_RACEMODE]) {//Is this really needed..
	//	if (client->ps.stats[STAT_MOVEMENTSTYLE] == MV_OCPM) {
	//		ucmd->serverTime = ((ucmd->serverTime + 7) / 8) * 8;
	//	}
	//	else {
	//		if (msec < 3)
	//			ucmd->serverTime = ((ucmd->serverTime + 2) / 3) * 3;//Integer math was making this bad, but is this even really needed? I guess for 125fps bhop height it is?
	//		else if (msec > 16 && client->pers.practice)
	//			ucmd->serverTime = ((ucmd->serverTime + 15) / 16) * 16;
	//	}
	//}
	//else if (pmove_fixed.integer || client->pers.pmoveFixed)
	//	ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer - 1) / pmove_msec.integer) * pmove_msec.integer;

	//if ((client->sess.sessionTeam != TEAM_SPECTATOR) && !client->ps.stats[STAT_RACEMODE] && ((g_movementStyle.integer >= MV_SIEGE && g_movementStyle.integer <= MV_WSW) || g_movementStyle.integer == MV_SP || g_movementStyle.integer == MV_SLICK || g_movementStyle.integer == MV_TRIBES)) { //Ok,, this should be like every frame, right??
	//	client->sess.movementStyle = g_movementStyle.integer;
	//}
	UpdateClientRaceVars(client);
	//client->ps.stats[STAT_MOVEMENTSTYLE] = client->sess.raceStyle.movementStyle;
	//client->ps.stats[STAT_RUNFLAGS] = client->sess.raceStyle.runFlags;
	//client->ps.stats[STAT_RACEMODE] = client->sess.raceMode; // can get lost sometimes after death? idk happened once but i had another bug then

	//if ((g_neutralFlag.integer < 4) && client->ps.powerups[PW_NEUTRALFLAG]) {
	//	if (client->ps.fd.forcePowerLevel[FP_LEVITATION] > 1) {
	//		client->savedJumpLevel = client->ps.fd.forcePowerLevel[FP_LEVITATION];
	//		client->ps.fd.forcePowerLevel[FP_LEVITATION] = 1;
	//	}
	//}
	//else if (client->savedJumpLevel) {
	//	client->ps.fd.forcePowerLevel[FP_LEVITATION] = client->savedJumpLevel;
	//}
	//if (client->ps.stats[STAT_RACEMODE]) {

	HandleClientLaserPointer(ent); // logically should be after intermission but eh, can use some memes

	inactivityToSpec = !ClientInactivitySpecTimer(ent);

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	if (ent && ent->client && (ent->client->ps.eFlags & EF_INVULNERABLE))
	{
		if (ent->client->invulnerableTimer <= nowTime && (!g_pauseGame.integer || ent->client->sess.raceMode))
		{
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		}
	}

	if (ent && ent->client && ent->client->sess.raceMode) {
		const int movementStyle = ent->client->sess.raceStyle.movementStyle;
		//if (movementStyle == MV_RJCPM || movementStyle == MV_RJQ3) {
		//	ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_SABER) + (1 << WP_ROCKET_LAUNCHER);
		//	ent->client->ps.ammo[AMMO_ROCKETS] = 2;
		//	if (ent->health > 0)
		//		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
		//}
		//else if (movementStyle == MV_JETPACK) {
		//	ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_SABER) + (1 << WP_DET_PACK);
		//	if (!ent->client->pers.stats.startTime)
		//		ent->client->ps.ammo[AMMO_DETPACK] = 4; //Dont drop their ammo before the course starts? qol
		//	if (ent->health > 0)
		//		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
		//}
		//else if (movementStyle == MV_TRIBES) {
		//	ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_MELEE) + (1 << WP_CONCUSSION);
		//	if (!ent->client->pers.stats.startTime)
		//		ent->client->ps.ammo[AMMO_METAL_BOLTS] = 120; //Dont drop their ammo before the course starts? qol
		//	if (ent->health > 0)
		//		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
		//}
		//else if (movementStyle == MV_COOP_JKA) {
		//	//ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 1;
		//	ent->client->ps.fd.forcePowerLevel[FP_LIGHTNING] = ent->client->ps.fd.forcePowerLevel[FP_DRAIN] = 2;
		//	ent->client->ps.fd.forcePowerLevel[FP_SPEED] = ent->client->ps.fd.forcePowerLevel[FP_GRIP] = ent->client->ps.fd.forcePowerLevel[FP_PUSH] = ent->client->ps.fd.forcePowerLevel[FP_PULL] = 3;
		//	ent->client->ps.fd.forcePowersKnown = (1 << FP_PULL) + (1 << FP_PUSH) + (1 << FP_SPEED) + (1 << FP_GRIP) + (1 << FP_DRAIN) + (1 << FP_LIGHTNING);
		//	ent->client->ps.stats[STAT_WEAPONS] = (1 << 16) - 1 - (1 << WP_DET_PACK) - (1 << WP_TRIP_MINE) - (1 << WP_THERMAL); //all weapons? w/o tripmine detpack.
		//	if (ent->health > 0)
		//		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_HEALTH] = ent->health = 999;
		//}
		//else 
		{
			client->ps.ammo[AMMO_POWERCELL] = 300;

			if (movementStyle == MV_JK2){// || movementStyle == MV_SIEGE || movementStyle == MV_QW || movementStyle == MV_PJK || movementStyle == MV_SP || movementStyle == MV_SPEED || movementStyle == MV_JETPACK) {
				ent->client->ps.stats[STAT_WEAPONS] = /*(1 << WP_MELEE) +*/ (1 << WP_SABER) + (1 << WP_DISRUPTOR) + (1 << WP_STUN_BATON);
			}
			else {
				ent->client->ps.stats[STAT_WEAPONS] = /*(1 << WP_MELEE) +*/ (1 << WP_SABER) + (1 << WP_DISRUPTOR) + (1 << WP_STUN_BATON);
			}
		}

		//if (movementStyle == MV_JETPACK || movementStyle == MV_TRIBES) //always give jetpack style a jetpack, and non jetpack styles no jetpack, maybe this should just be in clientspawn ?
		//	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
		//else
		//	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_JETPACK);
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}
	if (inactivityToSpec) {
		return;
	}

	if (client->sess.login.forceLoggedIn && (level.time - client->randomLastCenterprint > 1000 || level.time < client->randomLastCenterprint)) {
		client->randomLastCenterprint = level.time;
		G_CenterPrint(client - level.clients, 3, "^3You were force-logged in by an admin. Please change your password with /changepassword, log out and log in again.", qfalse, qtrue, qfalse, NULL);
	}

	if (client->sess.sessionTeam != TEAM_SPECTATOR) {
		BG_UserCmdToUserStats(&client->pers.cmd, &level.playerStats[ent - g_entities]->s);
	}
	else { // dont waste demo space on commands of spectators, what for...
		BG_UserCmdToUserStats(&nullUserCmd, &level.playerStats[ent - g_entities]->s);
	}

	BG_RaceStyleToUserStats(&client->sess.raceStyle,&level.playerStats[ent-g_entities]->s);
	level.playerStats[ent - g_entities]->s.activeForcePass = classifyLeaderBoard(&client->sess.raceStyle,&level.mapDefaultRaceStyle );

	clientFpsOk = ClientCheckNotifyPhysicsFps(ent); // Let the client know about his need to set a different com_physicsFps value if needed

	// race mode toggle restrictions
	if (client->sess.raceMode && (client->sess.raceStyle.msec == 0 || client->sess.raceStyle.msec > 0 && client->sess.raceStyle.msec != msec)) {
		// something weird/disallowed is going on with the client fps. best case scenario, it's dropped packets. 
		if (client->pers.raceStartCommandTime) {
			client->pers.raceDropped.msecTime += client->pers.cmd.serverTime - client->ps.commandTime;
			client->pers.raceDropped.packetCount++;
		}
		client->ps.commandTime = client->pers.cmd.serverTime;
		//client->lastMsecValue = msec;
		UpdateClientPastFpsStats(ent, msec);
		return;
	}

	if (clientFpsOk // if we are already notifying about physicsfps settings issues, ignore these errors.
		&& (client->pers.raceDropped.msecTime != client->pers.raceDropped.lastNotificationMsecTime || client->pers.raceDropped.packetCount != client->pers.raceDropped.lastNotificationPacketCount)
		&& ((client->pers.raceDropped.lastNotification + 1000) < level.time || client->pers.raceDropped.lastNotification > level.time)
		) {
		G_SendServerCommand(ent - g_entities, va("print \"^1%d ^7msec from ^1%d ^7packets soft-dropped due to wrong packet timing. Packet loss? Try a higher cl_packetdup value.\n\"", (client->pers.raceDropped.msecTime - client->pers.raceDropped.lastNotificationMsecTime), (client->pers.raceDropped.packetCount - client->pers.raceDropped.lastNotificationPacketCount)),qtrue);
		client->pers.raceDropped.lastNotification = level.time;
		client->pers.raceDropped.lastNotificationMsecTime = client->pers.raceDropped.msecTime;
		client->pers.raceDropped.lastNotificationPacketCount = client->pers.raceDropped.packetCount;
	}

	// clear the rewards if time
	if (nowTime > client->rewardTime ) {
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.eFlags & EF_DISINTEGRATION ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		if (client->ps.forceGripChangeMovetype)
		{
			client->ps.pm_type = client->ps.forceGripChangeMovetype;
		}
		else
		{
			client->ps.pm_type = PM_NORMAL;
		}
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	//client->ps.speed = g_speed.value;
	client->ps.basespeed = g_speed.value; 
	
	if (g_pauseGame.integer && !client->sess.raceMode) {
		//client->ps.pm_type = PM_FREEZE;
		client->ps.pm_type = PM_SPINTERMISSION;	//no movement NOR viewangles-change allowed
	}

	if (client->sess.raceMode /* || client->ps.stats[STAT_RACEMODE]*/) {
		//client->ps.speed = 250.0f;
		client->ps.basespeed = 250.0f;
		client->ps.gravity = 800.0f;
		//if (client->sess.raceStyle.movementStyle == MV_PINBALL) {
		//	client->ps.gravity = 200.0f;
		//}
	}

	if (MovementIsQuake3Based(moveStyle)) {
		//client->ps.speed = 320.0f;
		client->ps.basespeed = 320.0f;
	}

	if (ent->client->ps.duelInProgress)
	{
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];

		//Keep the time updated, so once this duel ends this player can't engage in a duel for another
		//10 seconds. This will give other people a chance to engage in duels in case this player wants
		//to engage again right after he's done fighting and someone else is waiting.
		if (!ent->client->sess.raceMode) {

			ent->client->ps.fd.privateDuelTime = level.time + (ent->client->sess.mode == MODE_DUELQUEUE ? g_duelQueueTimeout.integer : g_duelTimeout.integer);

			if (ent->client->ps.duelTime < level.time)
			{
				//Bring out the sabers
				if (ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered &&
					ent->client->ps.duelTime)
				{
					if (!saberOffSound || !saberOnSound)
					{
						saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
						saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
					}

					ent->client->ps.saberHolstered = qfalse;
					G_Sound(ent, CHAN_AUTO, saberOnSound);

					G_AddEvent(ent, EV_PRIVATE_DUEL, 2);

					ent->client->ps.duelTime = 0;
				}

				if (duelAgainst && duelAgainst->client && duelAgainst->inuse &&
					duelAgainst->client->ps.weapon == WP_SABER && duelAgainst->client->ps.saberHolstered &&
					duelAgainst->client->ps.duelTime)
				{
					if (!saberOffSound || !saberOnSound)
					{
						saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
						saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
					}

					duelAgainst->client->ps.saberHolstered = qfalse;
					G_Sound(duelAgainst, CHAN_AUTO, saberOnSound);

					G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 2);

					duelAgainst->client->ps.duelTime = 0;
				}
			}
			else
			{
				client->ps.speed = 0;
				client->ps.basespeed = 0;
				ucmd->forwardmove = 0;
				ucmd->rightmove = 0;
				ucmd->upmove = 0;
			}
		}

		if (!duelAgainst || !duelAgainst->client || !duelAgainst->inuse ||
			duelAgainst->client->ps.duelIndex != ent->s.number)
		{
			ent->client->ps.duelInProgress = 0;
			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
		}
		else if (duelAgainst->health < 1 || duelAgainst->client->ps.stats[STAT_HEALTH] < 1)
		{
			char* s;
			ent->client->ps.duelInProgress = 0;
			duelAgainst->client->ps.duelInProgress = 0;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
			G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

			//Winner gets full health.. providing he's still alive
			if (!ent->client->sess.raceMode) { // did i port this right?
				if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
				{
					if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
					{
						ClientSetStatHealth(ent->client, ent->health = ent->client->ps.stats[STAT_MAX_HEALTH]);
					}

					if (g_spawnInvulnerability.integer)
					{
						ent->client->ps.eFlags |= EF_INVULNERABLE;
						ent->client->invulnerableTimer = nowTime + g_spawnInvulnerability.integer;
					}
				}

				/*
				trap_SendServerCommand( ent-g_entities, va("print \"%s" S_COLOR_WHITE " %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELWINNER")) );
				trap_SendServerCommand( duelAgainst-g_entities, va("print \"%s" S_COLOR_WHITE " %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELWINNER")) );
				*/
				//Private duel announcements are now made globally because we only want one duel at a time.
				if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
				{
					int duelTime = (level.time - ent->client->pers.lastDuelStart) / 1000; // + 2000 cuz we add 2000 when the duel starts, idk why.
					int minutes = duelTime / 60;
					int seconds = duelTime % 60;

					ent->client->pers.lastDuelStatus = 1;
					duelAgainst->client->pers.lastDuelStatus = -1;

					G_LogPrintf(
						"DuelWin: %i %i %i %i: %s has defeated %s in %02i:%02i with %i/%i left\n",
						ent->s.number, duelAgainst->s.number,
						ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR],
						ent->client->pers.netname, duelAgainst->client->pers.netname,
						minutes, seconds,
						ent->client->ps.stats[STAT_HEALTH], ent->client->ps.stats[STAT_ARMOR]);

					// nicer prints from sabermod
					s = va("print \"%s" S_COLOR_WHITE " %s %s" S_COLOR_WHITE
						" in " S_COLOR_CYAN "%02i" S_COLOR_WHITE ":" S_COLOR_CYAN "%02i" S_COLOR_WHITE
						" with " S_COLOR_RED "%i" S_COLOR_WHITE "/" S_COLOR_GREEN "%i" S_COLOR_WHITE " left!\n\"",
						ent->client->pers.netname,
						G_GetStripEdString("SVINGAME", "PLDUELWINNER"),
						duelAgainst->client->pers.netname,
						minutes,
						seconds,
						ent->client->ps.stats[STAT_HEALTH],
						ent->client->ps.stats[STAT_ARMOR]);
					//G_CenterPrint( -1, 3, va("%s" S_COLOR_WHITE " %s %s" S_COLOR_WHITE "!", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELWINNER"), duelAgainst->client->pers.netname) , qtrue,qfalse,qtrue, NULL);
				}
				else
				{ //it was a draw, because we both managed to die in the same frame
					//G_CenterPrint( -1, 3, va("%s", G_GetStripEdString("SVINGAME", "PLDUELTIE")), qtrue, qfalse,qtrue, NULL);
					G_LogPrintf("DuelTie: %i %i: The duel between %s and %s is ended in a draw, both fighters have died\n",
						ent->s.number, duelAgainst->s.number,
						ent->client->pers.netname, duelAgainst->client->pers.netname);
					// nicer prints from sabermod
					s = va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELTIE"));


					ent->client->pers.lastDuelStatus = 0;
					duelAgainst->client->pers.lastDuelStatus = -0;
				}
				trap_SendServerCommand(-1, s);
			}
		}
		else if(!ent->client->sess.raceMode && g_duelSeverDistance.value != 0.0f)
		{
			vec3_t vSub;
			float subLen = 0;

			VectorSubtract(ent->client->ps.origin, duelAgainst->client->ps.origin, vSub);
			subLen = VectorLength(vSub);

			if (subLen >= g_duelSeverDistance.value)
			{
				ent->client->ps.duelInProgress = 0;
				duelAgainst->client->ps.duelInProgress = 0;

				G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
				G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 0);

				trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELSTOP")) );
			}
		}
	}

	/*
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}
	*/

	if (client->ps.usingATST && ent->health > 0)
	{ //we have special shot clip boxes as an ATST
		ent->r.contents |= CONTENTS_NOSHOT;
		ATST_ManageDamageBoxes(ent);
	}
	else
	{
		ent->r.contents &= ~CONTENTS_NOSHOT;
		client->damageBoxHandle_Head = 0;
		client->damageBoxHandle_RLeg = 0;
		client->damageBoxHandle_LLeg = 0;
	}

	//rww - moved this stuff into the pmove code so that it's predicted properly
	//BG_AdjustClientSpeed(&client->ps, &client->pers.cmd, nowTime);

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	if (ent->client && ent->client->ps.fallingToDeath &&
		(nowTime - FALL_FADE_TIME) > ent->client->ps.fallingToDeath)
	{ //die!
		player_die(ent, ent, ent, 100000, MOD_FALLING);
		respawn(ent);
		ent->client->ps.fallingToDeath = 0;

		G_MuteSound(ent->s.number, CHAN_VOICE); //stop screaming, because you are dead!
	}

	if (ent->client->ps.otherKillerTime > nowTime &&
		ent->client->ps.groundEntityNum != ENTITYNUM_NONE &&
		ent->client->ps.otherKillerDebounceTime < nowTime &&
		(!ent->waterlevel || !(ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))) &&
		!(ent->watertype & (CONTENTS_NOSPAWN)))
	{
		ent->client->ps.otherKillerTime = 0;
		ent->client->ps.otherKiller = ENTITYNUM_NONE;
	}
	else if (ent->client->ps.otherKillerTime > nowTime &&
		ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		if (ent->client->ps.otherKillerDebounceTime < (nowTime + 100))
		{
			ent->client->ps.otherKillerDebounceTime = nowTime + 100;
		}
	}

//	WP_ForcePowersUpdate( ent, msec, ucmd); //update any active force powers
//	WP_SaberPositionUpdate(ent, ucmd); //check the server-side saber point, do apprioriate server-side actions (effects are cs-only)

	if ((ent->client->pers.cmd.buttons & BUTTON_USE) && ent->client->ps.useDelay < nowTime)
	{
		TryUse(ent);
		ent->client->ps.useDelay = nowTime + 100;
	}

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = JP_Trace;
	pm.q2trace = g_q2trace.integer == 2 ? JP_TraceCustomEpsilonQ2 : (g_q2trace.integer ? JP_TraceCustomEpsilonQ2Lite : 0);
	pm.q2TraceStyle = g_q2trace.integer;
	pm.q2Skims = g_q2Skims.integer;
	pm.rawtrace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;
	pm.unlockRandom = g_unlockRandom.integer;
	pm.mineSwitchFix = g_mineSwitchFix.integer;
	pm.kickoffFix = g_kickoffFix.integer;

	pm.pmove_fixed = g_pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = g_pmove_msec.integer;
	pm.pmove_float = g_pmove_float.integer;

	pm.mod = SVMOD_TOMMYTERNAL;

	pm.highFpsFix = g_fixHighFPSAbuse.integer;

	pm.animations = bgGlobalAnimations;//NULL;

	pm.gametype = g_gametype.integer;

	pm.debugMelee = g_debugMelee.integer;

	if (g_fpsToggleDelay.integer && !client->sess.raceMode) { // TODO unify requiredCmdMsec and STAT_MSECRESTRICT and maybe all the other stuff related to this 
		pm.requiredCmdMsec = client->pers.physicsFps.acceptedSettingMsec ? client->pers.physicsFps.acceptedSettingMsec : -1;
	}

	//Set up bg entity data
	pm.baseEnt = (bgEntity_t*)g_entities;
	pm.entSize = sizeof(gentity_t);

	//VectorCopy( client->ps.origin, client->oldOrigin );

	if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
		if ( level.time - level.intermissionQueued >= 1000  ) {
			pm.cmd.buttons = 0;
			pm.cmd.forwardmove = 0;
			pm.cmd.rightmove = 0;
			pm.cmd.upmove = 0;
			if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
				trap_SendConsoleCommand( EXEC_APPEND, "centerview\n");
			}
			ent->client->ps.pm_type = PM_SPINTERMISSION;
		}
	}

	for ( i = 0 ; i < MAX_CLIENTS ; i++ )
	{
		if (g_entities[i].inuse && g_entities[i].client)
		{
			pm.bgClients[i] = &g_entities[i].client->ps;
		}
	}

	if (ent->client->ps.saberLockTime > nowTime)
	{
		gentity_t *blockOpp = &g_entities[ent->client->ps.saberLockEnemy];

		if (blockOpp && blockOpp->inuse && blockOpp->client)
		{
			vec3_t lockDir, lockAng;

			//VectorClear( ent->client->ps.velocity );
			VectorSubtract( blockOpp->r.currentOrigin, ent->r.currentOrigin, lockDir );
			//lockAng[YAW] = vectoyaw( defDir );
			vectoangles(lockDir, lockAng);
			DF_PreDeltaAngleChange(ent->client);
			SetClientViewAngle( ent, lockAng );
			DF_PostDeltaAngleChange(ent->client, qtrue);
		}

		if ( ( ent->client->buttons & BUTTON_ATTACK ) && ! ( ent->client->oldbuttons & BUTTON_ATTACK ) )
		{
			ent->client->ps.saberLockHits++;
		}
		if (ent->client->ps.saberLockHits > 2)
		{
			if (!ent->client->ps.saberLockAdvance)
			{
				ent->client->ps.saberLockHits -= 3;
			}
			ent->client->ps.saberLockAdvance = qtrue;
		}
	}
	else
	{
		ent->client->ps.saberLockFrame = 0;
		//check for taunt
		if ( (pm.cmd.generic_cmd == GENCMD_ENGAGE_DUEL) && (g_gametype.integer == GT_TOURNAMENT) )
		{//already in a duel, make it a taunt command
			pm.cmd.buttons |= BUTTON_GESTURE;
		}
	}

	// Save some value for robust trigger application
	VectorCopy(ent->client->ps.origin,ent->client->prePmovePosition);
	VectorCopy(ent->r.mins,ent->client->prePmoveMins); // this is -8 -8 -8 8 8 8 sometimes?!?!?! when rolling?
	VectorCopy(ent->r.maxs,ent->client->prePmoveMaxs);
	VectorCopy(ent->client->ps.velocity,prePmoveVelocity);
	ent->client->prePmoveEFlags = ent->client->ps.eFlags;
	ent->client->prePmovePositionSet = qtrue;
	ent->client->prePmoveCommandTime = ent->client->ps.commandTime;

	pm.handleStrafebotSlopes = g_strafebotSlopeHandling.integer;
	pm.roll = ent->client->pers.roll;
	pm.antiLoop = ent->client->pers.antiLoop;
	pm.oldButtons = ent->client->oldbuttons;
	pm.forcedModelScale = forcedModelScale;
	pm.pauseGame = ent->client->sess.raceMode ? 0 : g_pauseGame.integer;
	pm.playerBump = ent->client->sess.raceMode ? 0 : g_playerBump.integer;
	DF_PreDeltaAngleChange(ent->client);
	pm.positionChangedOutsidePmove = !VectorCompare(ent->client->ps.origin, client->oldPostPmovePosition);
	oldSaberMove = pm.ps->saberMove;
	Pmove (&pm);
	DF_PostDeltaAngleChange(ent->client,!(ent->client->sess.raceStyle.runFlags & RFL_BOT)); // qfalse if strafebot
	ent->client->pers.roll = pm.roll;
	ent->client->pers.antiLoop = pm.antiLoop;

	if (oldSaberMove != LS_A_BACK_CR && pm.ps->saberMove == LS_A_BACK_CR) {
		ent->client->pers.lastDbsSpeed = XYSPEED(pm.ps->velocity);
		if (!DF_SetPlayerSubContestValueSafeguarded(ent, SUBCONTESTS_DBS_SPEED, XYSPEED(pm.ps->velocity), 0, 0, 0, 0)) {
			// let's allow a stupidcontest. where u can just use /launch to launch urself at 10000 ups for a stupidly fast dbs
			DF_SetPlayerSubContestValue(ent, SUBCONTESTS_DBS_STUPIDSPEED, XYSPEED(pm.ps->velocity), 0, 0, 0, 0);
		}
	}

	if (client->sess.modeTeam == MODETEAM_IRONMAN_CAPPER) {
		G_MaybeSaveIronmanPos(ent, client->ps.groundEntityNum == ENTITYNUM_WORLD || pm.touchedGroundOrSlide);
	}

	if (ent->client->sess.raceStateInvalidated) {
		ent->client->pers.roll.rollDisqualified |= ROLLDIS_RACESTATEINVALID;
	}

	level.playerStats[ent - g_entities]->s.pos.trBase[1] = ent->client->ps.fd.forceSpeedSmash; // for nicer force speed client prediction
	level.playerStats[ent - g_entities]->s.pos.trDelta[0] = ent->client->ps.fd.forceJumpCharge; // for chargejump movement prediction
	//level.playerStats[ent - g_entities]->s.pos.trDelta[1] = intfloat(ent->client->ps.groundTime); // for chargejump movement prediction. nvm got rid of the groundtime thingie kinda

	//DF_AntiLoop_NewAngle(&ent->client->pers.antiLoop,prePmoveVelocity,ent->client->ps.velocity,ent->client->ps.basespeed,ent->client->sess.raceMode && ent->client->pers.raceStartCommandTime);
	DF_AntiLoop_NewAngle(&ent->client->pers.antiLoop,pm.lastAntiLoopVelocity,ent->client->ps.velocity,ent->client->ps.basespeed,ent->client->sess.raceMode && ent->client->pers.raceStartCommandTime);
	level.playerStats[ent - g_entities]->s.pos.trBase[0] = ent->client->pers.antiLoop.yawAngleChangeSinceBaseSpeed;
	DF_CheckRollSpeed(ent);

	VectorCopy(ent->client->ps.origin,ent->client->postPmovePosition);
	VectorCopy(pm.mins, ent->client->postPmoveMins);
	VectorCopy(pm.maxs, ent->client->postPmoveMaxs);
	VectorCopy(client->ps.origin, client->oldPostPmovePosition); // for q2 snapping mode

	UpdateClientPastFpsStats(ent,msec);
	//client->lastMsecValue = msec;

	if (DF_PrePmoveValid(ent)) { // is this accurate? can there be any movement outside of pmove? other than teleport, that is.
		vec3_t displacementAdd;
		float currentXYSpeed = XYSPEED(client->ps.velocity);
		float distanceTraveled;
		int fractionIntegerized;
		VectorSubtract(client->postPmovePosition, client->prePmovePosition, displacementAdd);
		distanceTraveled = VectorLength(displacementAdd);
		client->pers.distanceTraveledFraction += distanceTraveled;
		fractionIntegerized = client->pers.distanceTraveledFraction;
		client->pers.distanceTraveled += fractionIntegerized;
		client->pers.distanceTraveledFraction -= (float)fractionIntegerized;
		if (client->pers.raceStartCommandTime) {
			client->pers.stats.distanceTraveled += distanceTraveled;
			displacementAdd[2] = 0;
			client->pers.stats.distanceTraveled2D += VectorLength(displacementAdd);
			if (currentXYSpeed > client->pers.stats.topSpeed) {
				client->pers.stats.topSpeed = currentXYSpeed;
			}
		}
	}

	if (pm.checkDuelLoss)
	{
		if (pm.checkDuelLoss > 0 && pm.checkDuelLoss <= MAX_CLIENTS)
		{
			gentity_t *clientLost = &g_entities[pm.checkDuelLoss-1];

			if (clientLost && clientLost->inuse && clientLost->client && Q_irand(0, 40 + gRandomUnlockAdd, clientLost->client->sess.raceMode, 20) > clientLost->health)
			{
				vec3_t attDir;
				VectorSubtract(ent->client->ps.origin, clientLost->client->ps.origin, attDir);
				VectorNormalize(attDir);

				VectorClear(clientLost->client->ps.velocity);
				clientLost->client->ps.forceHandExtend = HANDEXTEND_NONE;
				clientLost->client->ps.forceHandExtendTime = 0;

				gGAvoidDismember = 1;
				G_Damage(clientLost, ent, ent, attDir, clientLost->client->ps.origin, 9999, DAMAGE_NO_PROTECTION, MOD_SABER);

				if (clientLost->health < 1)
				{
					gGAvoidDismember = 2;
					G_CheckForDismemberment(clientLost, clientLost->client->ps.origin, 999, (clientLost->client->ps.legsAnim&~ANIM_TOGGLEBIT));
				}

				gGAvoidDismember = 0;
			}
		}

		pm.checkDuelLoss = 0;
	}

	pauseItemUse = (qboolean)(g_pauseGame.integer && !ent->client->sess.raceMode);
	switch(pm.cmd.generic_cmd)
	{
	case 0:
		break;
	case GENCMD_SABERSWITCH:
		Cmd_ToggleSaber_f(ent);
		break;
	case GENCMD_ENGAGE_DUEL:
		//if ( g_gametype.integer == GT_TOURNAMENT )
		//{//already in a duel, made it a taunt command
		//}
		//else
		//{
			Cmd_EngageDuel_f(ent);
		//}
		break;
	case GENCMD_FORCE_HEAL:
		ForceHeal(ent);
		break;
	case GENCMD_FORCE_SPEED:
		ForceSpeed(ent, 0);
		break;
	case GENCMD_FORCE_THROW:
		ForceThrow(ent, qfalse);
		break;
	case GENCMD_FORCE_PULL:
		ForceThrow(ent, qtrue);
		break;
	case GENCMD_FORCE_DISTRACT:
		ForceTelepathy(ent);
		break;
	case GENCMD_FORCE_RAGE:
		ForceRage(ent);
		break;
	case GENCMD_FORCE_PROTECT:
		ForceProtect(ent);
		break;
	case GENCMD_FORCE_ABSORB:
		ForceAbsorb(ent);
		break;
	case GENCMD_FORCE_HEALOTHER:
		ForceTeamHeal(ent);
		break;
	case GENCMD_FORCE_FORCEPOWEROTHER:
		ForceTeamForceReplenish(ent);
		break;
	case GENCMD_FORCE_SEEING:
		ForceSeeing(ent);
		break;
	case GENCMD_USE_SEEKER:
		if ( !pauseItemUse && (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)) &&
			G_ItemUsable(&ent->client->ps, HI_SEEKER,ent) )
		{
			ItemUse_Seeker(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SEEKER, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
		}
		break;
	case GENCMD_USE_FIELD:
		if (!pauseItemUse && (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD)) &&
			G_ItemUsable(&ent->client->ps, HI_SHIELD, ent) )
		{
			ItemUse_Shield(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SHIELD, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SHIELD);
		}
		break;
	case GENCMD_USE_BACTA:
		if (!pauseItemUse && (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC)) &&
			G_ItemUsable(&ent->client->ps, HI_MEDPAC, ent) )
		{
			ItemUse_MedPack(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_MEDPAC, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC);
		}
		break;
	case GENCMD_USE_ELECTROBINOCULARS:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
			G_ItemUsable(&ent->client->ps, HI_BINOCULARS, ent) )
		{
			ItemUse_Binoculars(ent);
			if (ent->client->ps.zoomMode == 0)
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
			}
			else
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
			}
		}
		break;
	case GENCMD_ZOOM:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
			G_ItemUsable(&ent->client->ps, HI_BINOCULARS, ent) )
		{
			ItemUse_Binoculars(ent);
			if (ent->client->ps.zoomMode == 0)
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
			}
			else
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
			}
		}
		break;
	case GENCMD_USE_SENTRY:
		if (!pauseItemUse && (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN)) &&
			G_ItemUsable(&ent->client->ps, HI_SENTRY_GUN, ent) )
		{
			ItemUse_Sentry(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SENTRY_GUN, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SENTRY_GUN);
		}
		break;
	case GENCMD_SABERATTACKCYCLE:
		Cmd_SaberAttackCycle_f(ent);
		break;
	default:
		break;
	}

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = nowTime;
	}
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, g_snapPlayerPosAngles.integer);
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, g_snapPlayerPosAngles.integer);
	}
	SendPendingPredictableEvents( &ent->client->ps );

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;
	ent->contenttype = pm.contenttype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	if ( pm.useEvent )
	{
		//TODO: Use
//		TryUse( ent );
	}

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
//	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = nowTime;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// Did we kick someone in our pmove sequence?
	if (client->ps.forceKickFlip && !client->sess.raceMode)
	{
		gentity_t *faceKicked = &g_entities[client->ps.forceKickFlip-1];

		if (faceKicked && faceKicked->client && (!OnSameTeam(ent, faceKicked) || g_friendlyFire.integer) &&
			(!faceKicked->client->ps.duelInProgress || faceKicked->client->ps.duelIndex == ent->s.number) &&
			(!ent->client->ps.duelInProgress || ent->client->ps.duelIndex == faceKicked->s.number))
		{
			if ( faceKicked && faceKicked->client && faceKicked->health && faceKicked->takedamage && !faceKicked->client->sess.raceMode && !faceKicked->client->noclip)
			{//push them away and do pain
				vec3_t oppDir;
				int strength = (int)VectorNormalize2( client->ps.velocity, oppDir );

				strength *= 0.05;

				VectorScale( oppDir, -1, oppDir );

				G_Damage( faceKicked, ent, ent, oppDir, client->ps.origin, strength, DAMAGE_NO_ARMOR, MOD_MELEE );

				if ( (faceKicked->client->ps.weapon != WP_SABER ||
					 faceKicked->client->ps.fd.saberAnimLevel < FORCE_LEVEL_3 ||
					 (!BG_SaberInAttack(faceKicked->client->ps.saberMove) && !PM_SaberInStart(faceKicked->client->ps.saberMove) && !PM_SaberInReturn(faceKicked->client->ps.saberMove) && !PM_SaberInTransition(faceKicked->client->ps.saberMove)))
					 || jk2gameplay == VERSION_1_02 )
				{
					if (faceKicked->health > 0 &&
						faceKicked->client->ps.stats[STAT_HEALTH] > 0 &&
						faceKicked->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{
						if (Q_irand(1, 10, faceKicked->client->sess.raceMode,5) <= 3)
						{ //only actually knock over sometimes, but always do velocity hit
							faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							faceKicked->client->ps.forceHandExtendTime = LEVELTIME(faceKicked->client) + 1100;
							faceKicked->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
						}

						faceKicked->client->ps.otherKiller = ent->s.number;
						faceKicked->client->ps.otherKillerTime = LEVELTIME(faceKicked->client) + 5000;
						faceKicked->client->ps.otherKillerDebounceTime = LEVELTIME(faceKicked->client) + 100;

						faceKicked->client->ps.velocity[0] = oppDir[0]*(strength*40);
						faceKicked->client->ps.velocity[1] = oppDir[1]*(strength*40);
						faceKicked->client->ps.velocity[2] = 200;
					}
				}

				G_Sound( faceKicked, CHAN_AUTO, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4,qfalse,2)) ) );
			}
		}

		client->ps.forceKickFlip = 0;
	}

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		if (client->sess.raceMode) { // respawn immediately in racemode
			respawn(ent);
			return;
		}

		// wait for the attack button to be pressed
		if (nowTime > client->respawnTime && !gDoSlowMoDuel ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && 
				(nowTime - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				respawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
				respawn( ent );
			}
		}
		else if (gDoSlowMoDuel)
		{
			client->respawnTime = nowTime + 1000;
		}
		return;
	}

	// give ironman points for being near danger
	IronmanDangerPoints( ent, msec );

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

	G_UpdateClientBroadcasts ( ent );



	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		ClientEndFrameInClientThink(ent);
	}
}

/*
==================
G_CheckClientTimeouts

Checks whether a client has exceded any timeouts and act accordingly
==================
*/
void G_CheckClientTimeouts ( gentity_t *ent )
{
	qboolean isReplaying;
	// Only timeout supported right now is the timeout to spectator mode
	if ( g_timeouttospec.integer <= 0 ) // one may accidentally set 9999999999999 and cause overflow and that would lead to unintended consequences
	{
		return;
	}

	if (g_pauseGame.integer && !ent->client->sess.raceMode) {
		return;
	}

	// Already a spectator, no need to boot them to spectator
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		return;
	}

	isReplaying = ent->client->sess.raceMode && (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && ent->client->pers.segmented.state == SEG_REPLAY;

	if ((isReplaying || ent->client->pers.recordingDemo && ent->client->pers.keepDemoMaybe && !ent->client->pers.raceStartCommandTime)) {
		return; // dont send zombies to spec until dmeo is finished
	}

	// See how long its been since a command was received by the client and if its 
	// longer than the timeout to spectator then force this client into spectator mode
	if ( level.time - ent->client->pers.cmd.serverTime > clampedIntMult(g_timeouttospec.integer ,1000) )
	{
		G_Printf("^3g_timeouttospec: Sending client %d to spec. %d time delta.\n",(int)(ent - g_entities), level.time - ent->client->pers.cmd.serverTime);
		SetTeam ( ent, "spectator" );
	}
}

void G_ResetUserCmdStore(int clientNum) {
	userCmdBuffer[clientNum].nextBufferIndex = userCmdBuffer[clientNum].nextToExecute = userCmdBuffer[clientNum].msecThisFrame = 0;
}

// we implement a buffering here to smooth out demos if ppl have extreme lag (causing a LOT of packets to get executed at once)
qboolean G_GetUserCmd(int clientNum, usercmd_t* ucmd, getUserCmdType_t advance) {
	usercmd_t* newCmd = &userCmdBuffer[clientNum].buf[userCmdBuffer[clientNum].nextBufferIndex % USERCMD_BUFFER_MAX];
	usercmd_t* oldCmd = NULL;
	gentity_t* userEnt = clientNum >= 0 && clientNum < MAX_CLIENTS?  g_entities + clientNum : NULL;
	qboolean superSmooth = g_userCmdBufferSmoothen.integer > 1 || g_userCmdBufferSmoothen.integer == 1 && userEnt && userEnt->client && userEnt->client->sess.raceMode; // reduces how much client can advance per frame and also keeps a 3-4 frame buffer to smooth out clients with low maxpackets. think of it a bit similar to double/triple buffering on GPUS. Bit of delay but smoother. TODO make it buffer only up to 50ms in this case? TODO dynamically adjust the amount of buffering up if someone is lagging a LOT?
	int baseFrameAdvanceMultiplier = superSmooth ? 2 : 5;
	int maxFrameAdvance = level.frameTimeMsec ? ((level.frameTimeMsec* baseFrameAdvanceMultiplier) > USERCMD_BUFFER_MAX_FRAMEADVANCE_MAX ? MAX(level.frameTimeMsec*2, USERCMD_BUFFER_MAX_FRAMEADVANCE_MAX) : (level.frameTimeMsec * baseFrameAdvanceMultiplier)) : INT_MAX;
	qboolean didAdvance = qfalse;
	int currentServerTime;

	if (!g_userCmdBuffer.integer && userCmdBuffer[clientNum].nextBufferIndex <= 1) {
		trap_GetUsercmd(clientNum, ucmd);
		return qfalse;
	}

	if (userCmdBuffer[clientNum].nextBufferIndex > 0) {
		oldCmd = &userCmdBuffer[clientNum].buf[(userCmdBuffer[clientNum].nextBufferIndex - 1) % USERCMD_BUFFER_MAX];
	}
	else {
		didAdvance = qtrue;
	}

	// first check if there's a new cmd available so we don't lose it
	trap_GetUsercmd(clientNum, newCmd);
	if (!oldCmd || newCmd->serverTime != oldCmd->serverTime) {
		// they are different -> it's a new one. save it.
		userCmdBuffer[clientNum].nextBufferIndex++;
	}

	currentServerTime = newCmd->serverTime;


	if ((userCmdBuffer[clientNum].nextBufferIndex - userCmdBuffer[clientNum].nextToExecute) > USERCMD_BUFFER_MAX) {
		// overflowed
		G_SendServerCommand(clientNum, "print \"^1Server usercmd buffer overflowed. Very bad internet?\n\"", qtrue);
		G_Printf("^1Usercmd buffer overflowed for client %d.\n", clientNum);
		userCmdBuffer[clientNum].nextToExecute = userCmdBuffer[clientNum].nextBufferIndex - USERCMD_BUFFER_MAX;
		didAdvance = qtrue;
	}


	// check if we want to & can advance
	if (!didAdvance && advance && userCmdBuffer[clientNum].nextToExecute < (userCmdBuffer[clientNum].nextBufferIndex-1)) {
		int nextMsec;
		qboolean superSmoothAllows;
		oldCmd = &userCmdBuffer[clientNum].buf[(userCmdBuffer[clientNum].nextToExecute) % USERCMD_BUFFER_MAX]; // we just keep reusing these pointers like dirty animals :)
		newCmd = &userCmdBuffer[clientNum].buf[(userCmdBuffer[clientNum].nextToExecute+1) % USERCMD_BUFFER_MAX];
		nextMsec = newCmd->serverTime - oldCmd->serverTime;
		superSmoothAllows = !superSmooth || (userCmdBuffer[clientNum].nextBufferIndex - userCmdBuffer[clientNum].nextToExecute) > 4; // in super smooth mode, always keep 4 packets on the back burner.
		if (nextMsec <= 0 // something weird is happening. best not to interfere, just go
			|| !userCmdBuffer[clientNum].msecThisFrame // no cmds have been executed this frame yet, just go
			|| (userCmdBuffer[clientNum].msecThisFrame + nextMsec) < maxFrameAdvance && superSmoothAllows // we still have some room to squeeze it in this frame, go.
			|| (currentServerTime- oldCmd->serverTime) > USERCMD_BUFFER_MAX_DELAY // delay is too big, just go
			|| (userCmdBuffer[clientNum].nextBufferIndex - userCmdBuffer[clientNum].nextToExecute) > USERCMD_BUFFER_MAX_BLOCKING // let's keep ~10% as buffer so we never overflow. just go.
			|| !g_userCmdBuffer.integer
			) {
			userCmdBuffer[clientNum].nextToExecute++;
			userCmdBuffer[clientNum].msecThisFrame += nextMsec;
			didAdvance = qtrue;
		}
		else {
			if (g_developer.integer > 10) {
				G_Printf("^1frame advance delayed (%s) for client %d; level.time %d, msecThisFrame %d, nextMsec %d, delay %d, buffer size %d, nextToExecute %d\n", advance == GETUSERCMD_ADVANCERUNCLIENT ? "RunClient" : "ClientThink", clientNum, level.time, userCmdBuffer[clientNum].msecThisFrame, nextMsec, (currentServerTime - oldCmd->serverTime), userCmdBuffer[clientNum].nextBufferIndex - userCmdBuffer[clientNum].nextToExecute, userCmdBuffer[clientNum].nextToExecute);
			}
		}
	}


	newCmd = &userCmdBuffer[clientNum].buf[userCmdBuffer[clientNum].nextToExecute % USERCMD_BUFFER_MAX]; // whatever, just reuse the pointer for this;

	if (didAdvance || !advance) { // if we wanted to advance, but didn't, act like nothing happened.
		*ucmd = *newCmd;
	}

	if (userCmdBuffer[clientNum].nextToExecute && userCmdBuffer[clientNum].nextToExecute == (userCmdBuffer[clientNum].nextBufferIndex - 1)) {
		// ok we got nothing buffered rn, reset everything a bit.
		//if (g_developer.integer > 2) {
		//	G_Printf("^2resetting command buffer for client %d\n", clientNum);
		//}
		// is this needed?
		if (userCmdBuffer[clientNum].nextToExecute % USERCMD_BUFFER_MAX) {
			userCmdBuffer[clientNum].buf[0] = *newCmd;
		}
		userCmdBuffer[clientNum].nextToExecute = 0;
		userCmdBuffer[clientNum].nextBufferIndex = 1;
	}

	return didAdvance;
}



#ifdef ANALYZE_BS
const char* BsRecordText(int i, bsFrameSample_t* frame) {

	int buttons = frame->buttons;
	int frametime = frame->frametime;
	int moves = frame->moves;

	return va("[%02d]  %4.3f   ms  %2d    %c%c%c%c  %c  %s",
		i, frame->yawdelta, frame->frametime,
		(buttons & BUTTON_ATTACK) ? 'M' : ' ',
#if 0
		(frame->forwardmove > 0) ? 'W' : ((frame->forwardmove < 0) ? 'S' : ' '),
		(frame->rightmove > 0) ? 'D' : ((frame->rightmove < 0) ? 'A' : ' '),
		(frame->upmove > 0) ? 'J' : ((frame->upmove < 0) ? 'C' : ' '),
#else
		(moves & MOVE_FORWARD) ? 'W' : (moves & MOVE_BACK ? 'S' : ' '),
		(moves & MOVE_RIGHT) ? 'D' : (moves & MOVE_LEFT ? 'A' : ' '),
		(moves & MOVE_UP) ? 'J' : (moves & MOVE_DOWN ? 'C' : ' '),
#endif
		(buttons & BUTTON_ANY) ? '_' : ' ',
		(buttons & BUTTON_DBS) ? "d/bs" : "");

}


void CheckBackStab(int clientNum) {
	//This guy just got permission to do a backstab.
	//Let's see how his angles evolved up until this attack.
	clientPersistant_t* pers;
	int last;

	if (clientNum >= MAX_CLIENTS || clientNum < 0 || !g_entities[clientNum].client)
		return;

	pers = &g_entities[clientNum].client->pers;

	//mark the current yaw angle (it was the yaw angle at which he performed the bs)
	last = (pers->cmdstack - 1) & ANGLES_MASK;		// the LATEST YAW (index)

	pers->backupcmd[last].buttons |= BUTTON_DBS;
	pers->analyzestart = pers->cmdstack + BS_ANALYZE_SAMPLES;	//when cmdstack hits this, we will analyze the move.
}

void BuildBsRecord(bsRecord_t* bsr, gclient_t* client)
{
	clientPersistant_t* pers = &client->pers;
	const int 		    first = pers->cmdstack & ANGLES_MASK;		// the earliest cmd recorded (index)
	int 				i, ind;

	usercmd_t* prev = &pers->backupcmd[first & ANGLES_MASK];

	//build a bsRecord_t
	ind = first + 1;
	for (i = 0; i < NUM_BS_FRAME_SAMPLES; ++i, ++ind) {
		bsFrameSample_t* frame = &bsr->frame[i];
		usercmd_t* cmd = &pers->backupcmd[ind & ANGLES_MASK];

		frame->yawdelta = AngleDelta(SHORT2ANGLE(cmd->angles[YAW]), SHORT2ANGLE(prev->angles[YAW]));
		frame->frametime = cmd->serverTime - prev->serverTime;
		frame->buttons = cmd->buttons;

#if 0
		frame->forwardmove = cmd->forwardmove;
		frame->rightmove = cmd->rightmove;
		frame->upmove = cmd->upmove;
#else
		frame->moves = 0;
		if (cmd->forwardmove > 0)
			frame->moves |= MOVE_FORWARD;
		else if (cmd->forwardmove < 0)
			frame->moves |= MOVE_BACK;

		if (cmd->rightmove > 0)
			frame->moves |= MOVE_RIGHT;
		else if (cmd->rightmove < 0)
			frame->moves |= MOVE_LEFT;

		if (cmd->upmove > 0)
			frame->moves |= MOVE_UP;
		else if (cmd->upmove < 0)
			frame->moves |= MOVE_DOWN;
#endif

		prev = cmd;
	}
}

void AnalyzeBS(gentity_t* ent) {
	bsRecord_t* bsr = &ent->client->pers.savedbs;

	BuildBsRecord(bsr, ent->client);

	if (g_logbs.integer)
	{
		//save all bs events to disk
		G_LogBsEvent(bsr, ent);
	}

	++ent->client->pers.numbs;
	level.lastBsClient = ent - g_entities;
}

#else
void CheckBackStab(int clientNum) {
}
#endif


/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent = g_entities + clientNum;
	usercmd_t tmpCmdForAfkCheck; // only used for checking if player is active or afk, rest is handled via G_GetUserCmd
	qboolean segmentedReplay = DF_ClientInSegmentedRunMode(ent->client) && ent->client->pers.segmented.state == SEG_REPLAY;
	int newbuttons;

	trap_GetUsercmd(clientNum, &tmpCmdForAfkCheck);
	newbuttons = tmpCmdForAfkCheck.buttons;
	//if (tmpCmdForAfkCheck.forwardmove || tmpCmdForAfkCheck.rightmove || tmpCmdForAfkCheck.upmove || (newbuttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) || ((newbuttons^ ent->client->sess.oldbuttons_immediate) & BUTTON_TALK)) {
	newbuttons &= ~(65536|BUTTON_TALK); // netcode for usercmds is a bit weird. encoding does 1 bit less than decoding. buttons is a 16 bit val so 16th bit ends up "random", so we can't use it for afk detection
	// BUTTON_TALK is also unreliable. map changes will for example change console status etc. 
	if (tmpCmdForAfkCheck.forwardmove || tmpCmdForAfkCheck.rightmove || tmpCmdForAfkCheck.upmove || ent->client->sess.sessionInitialized && (newbuttons ^ ent->client->sess.oldbuttons_immediate)) {
		if (g_developer.integer) {
			if (clampedIntAdd(level.time, -ent->client->sess.lastHereTime) > 30000) {
				Com_Printf("^3Client %d came back from AFK after %d milliseconds, fm %d, rm %d, um %d, btnchange %d, oldbuttons %d, buttons %d.\n",(int)(ent - g_entities), clampedIntAdd(level.time, -ent->client->sess.lastHereTime),tmpCmdForAfkCheck.forwardmove,tmpCmdForAfkCheck.rightmove,tmpCmdForAfkCheck.upmove, newbuttons ^ ent->client->sess.oldbuttons_immediate, ent->client->sess.oldbuttons_immediate, newbuttons);
			}
			else if (level.time < ent->client->sess.lastHereTime) {
				Com_Printf("^3Client %d came back from AFK (glitch %d<%d), fm %d, rm %d, um %d, btnchange %d, oldbuttons %d, buttons %d.\n", (int)(ent - g_entities), level.time, ent->client->sess.lastHereTime, tmpCmdForAfkCheck.forwardmove, tmpCmdForAfkCheck.rightmove, tmpCmdForAfkCheck.upmove, newbuttons ^ ent->client->sess.oldbuttons_immediate, ent->client->sess.oldbuttons_immediate, newbuttons);
			}
		}
		ent->client->sess.lastHereTime = level.time; // for afk tracking for players
		ent->client->pers.lastMovementTime = level.time; // for /kill allowance as ironman capper and such
	}
	ent->client->sess.oldbuttons_immediate = newbuttons;

	//if (!segmentedReplay) {
	//	canRun = G_GetUserCmd(clientNum, &ent->client->pers.cmd,qtrue);
	//}

#ifdef ANALYZE_BS
	//Copy the newly arrived usercmd into circular buffer
	if (!(ent->r.svFlags & SVF_BOT) && g_analyzebs.integer) {
		usercmd_t* sm = &ent->client->pers.backupcmd[ent->client->pers.cmdstack & ANGLES_MASK];

		memcpy(sm, &tmpCmdForAfkCheck, sizeof(usercmd_t));

		sm->buttons &= ~(BUTTON_DBS); // same value as BUTTON_LASERPOINTER on this mod. lets ignore the laserpointer for this i guess.

		if (++ent->client->pers.cmdstack == ent->client->pers.analyzestart) {
			AnalyzeBS(ent);
			ent->client->pers.analyzestart = -1;
		}
	}
#endif


	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && !segmentedReplay) {
		int index = 0;
		while (G_GetUserCmd(clientNum, &ent->client->pers.cmd, GETUSERCMD_ADVANCECLIENTTHINK) || !g_userCmdBuffer.integer && !index) {

			ClientThink_real(ent);
			index++;
		}
	}
	else if (!segmentedReplay){
		ent->client->pers.cmd = tmpCmdForAfkCheck; // make sure bots work.
	}
}


static void ForceClientUpdate(gentity_t* ent) {
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		return; // or we get stuck in endless loop :)
	}

	ent->client->lastCmdTime = level.time;

	// fill with seemingly valid data
	//ent->client->pers.cmd.serverTime = level.time;

	//ent->client->pers.cmd.buttons = 0;
	//ent->client->pers.cmd.forwardmove = ent->client->pers.cmd.rightmove = ent->client->pers.cmd.upmove = 0;

	//ent->client->pers.cmd.buttons = ent->client->pers.lastCmd.buttons;
	//ent->client->pers.cmd.forwardmove = ent->client->pers.lastCmd.forwardmove;
	//ent->client->pers.cmd.rightmove = ent->client->pers.lastCmd.rightmove;
	//ent->client->pers.cmd.upmove = ent->client->pers.lastCmd.upmove;

	if (ent->client->sess.raceMode && ent->client->sess.raceStyle.msec > 0) {
		while (ent->client->ps.commandTime < (level.time+ent->client->pers.segmented.playbackStartedCommandTimeOffset)) {
			ent->client->pers.cmd.serverTime = ent->client->ps.commandTime + ent->client->sess.raceStyle.msec;
			ClientThink_real(ent);
			if (ent->client->ps.commandTime != ent->client->pers.cmd.serverTime) {
				trap_SendServerCommand(-1,va("print \"^1ForceClientUpdate: WTF. ClientThink_real returned with commandTime %d but cmd.serverTime was %d\n\"", ent->client->ps.commandTime, ent->client->pers.cmd.serverTime));
				ent->client->ps.commandTime = ent->client->pers.cmd.serverTime;
			}
		}
	}
	else {
		ent->client->pers.cmd.serverTime = level.time;
		ClientThink_real(ent);
	}

}

extern void RestorePosition(gentity_t* client, savedPosition_t* savedPosition, veci_t* diffAccum);
void G_RunClient( gentity_t *ent ) {
	qboolean areSegReplaying = DF_ClientInSegmentedRunMode(ent->client) && ent->client->pers.segmented.state == SEG_REPLAY;

	// check if we should execute a few client frames that got buffered
	if (!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && !areSegReplaying && g_userCmdBuffer.integer && ent->client->pers.connected == CON_CONNECTED) {
		while (G_GetUserCmd(ent - g_entities, &ent->client->pers.cmd, GETUSERCMD_ADVANCERUNCLIENT)) {
			if (g_developer.integer > 10) {
				G_Printf("^3executing buffered cmd for client %d\n", (int)(ent - g_entities));
			}
			ClientThink_real(ent);
		}
	}

	//If racemode , do forceclientupdaterate hardcoded at like 4/5 hz ?

	// force client updates if they're not sending packets at roughly 4hz

	if (ent->client->pers.recordingDemo) { //(ent->client->ps.pm_flags & PMF_FOLLOW) ?
		if (g_cheats.integer || !ent->client->sess.raceMode || (ent->client->sess.sessionTeam == TEAM_SPECTATOR)) {
			//Their demo is bad, dont keep telling game to keep it
		}
		else if (!areSegReplaying && (!g_pauseGame.integer || ent->client->sess.raceMode) && (!ent->client->pers.stats.startLevelTime ||
			((ent->client->lastHereTime < level.time - 60 * 60 * 1000) && (level.time - ent->client->pers.demoStoppedTime > 10000)))
			//  || (trap_Milliseconds() - ent->client->pers.stats.startTime > 240 * 60 * 1000)) // just give up on races longer than 4 hours lmao
			)
		{
			//Their demo is bad, dont keep telling game to keep it
		}
		else
			ent->client->pers.stopRecordingTime = level.time + 10000; //Their demo is good! tell game not to delete it yet
	}

	if (ent->client->pers.recordingDemo && (ent->client->pers.stopRecordingTime < level.time)) {
		ent->client->pers.recordingDemo = qfalse;
		ent->client->pers.demoStoppedTime = level.time;

		if (ent->client->pers.keepDemoMaybe) {
			// rename happens automatically
			trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i\n", ent->s.number));
		}
		else {
			trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i;svrenamedemo \"%s\" \"%strash/trash%d\"\n", ent->s.number, ent->client->pers.tempDemoName, level.tempDemoNamePrefix, ent->s.number));
		}
	}
	
	if (ent->client->pers.cmd.forwardmove || ent->client->pers.cmd.rightmove || ent->client->pers.cmd.upmove || (ent->client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))) {
		ent->client->lastHereTime = level.time; // for demo stuff
	}

	if (g_autoScoresInterval.integer && (ent->client->lastScoresMessage < (level.time-clampedIntMult(g_autoScoresInterval.integer,1000)) || level.time < ent->client->lastScoresMessage)) {
		DeathmatchScoreboardMessage(ent);
	}

	//if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && (!DF_ClientInSegmentedRunMode(ent->client) || ent->client->pers.segmented.state != SEG_REPLAY)) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && !areSegReplaying) {
		entityState_t* stats = &level.playerStats[ent - g_entities]->s;
		stats->apos.trTime = 0;
		stats->pos.trTime = 0;
		stats->frame = 0;
		if (ent->client->clientIsZombified) {
			ForceClientUpdate(ent); // client is officially disconnected. we are likely just after a segmented run replay. just move him a lil bit.
		}
		return;
	}

	if ( ent->client->pers.botDelayed )
	{ // Call ClientBegin for delayed bots now
		ClientBegin( ent-g_entities, qtrue );
		ent->client->pers.botDelayed = qfalse;
	}

	//if (DF_ClientInSegmentedRunMode(ent->client) && ent->client->pers.segmented.state == SEG_REPLAY) {
	if (areSegReplaying) {
		usercmd_t ucmd;
		gclient_t* cl = ent->client;

		ClientInactivitySpecTimerReset(ent, 10000); // if we are in a segmented replay, don't let a player be sent to spec immediately after the replay ends. cuz ugly.

		if (!cl->pers.segmented.playbackNextCmdIndex) {
			RestorePosition(ent, &cl->pers.segmented.startPos, NULL);
			VectorCopy(cl->pers.segmented.startPos.ps.delta_angles, cl->ps.delta_angles); //keep this so we can replay properly. we won't let the person move anyway.
		}
		while (qtrue) {
			qboolean success=qtrue;
			posHashType_t posHash =0,currentPosHash=0;
			int targetServerTime;
			ucmd.serverTime = -1;
			while (success && (ucmd.serverTime == -1 || ucmd.serverTime == -2)) { // -1 is just a marker for cuts
				success = trap_G_COOL_API_PlayerUserCmdGet(ent - g_entities, cl->pers.segmented.playbackNextCmdIndex, &ucmd, &posHash);
				if (success && (ucmd.serverTime == -1 || ucmd.serverTime == -2)) {
					entityState_t* stats = &level.playerStats[ent - g_entities]->s;
					switch (ucmd.serverTime) {
					case -1:
						stats->apos.trTime = cl->pers.segmented.playbackNextCmdIndex == 0 ? cl->pers.segmented.playbackStartedTime + cl->pers.segmented.playbackStartedCommandTimeOffset : cl->ps.commandTime;
						stats->frame = cl->pers.segmented.playbackNextCmdIndex == 0 ? 1 : stats->frame + 1;
						stats->pos.trTime = 0;
						stats->pos.trDuration = 0;
						break;
					case -2:
						stats->pos.trTime = ucmd.buttons; // this isnt time. this is count of respos of this point. will usually follow right after the -1 one.
						// low 14 bits: discard-resposcount. then 10 bits discard-count. then 8 bit max discard depth
						// we risk a bit less bits here since the info isnt THAT vital, and discards are gonna be less frequent than respos
						// this allows for something like 16384 discarded respos, 1023 discards, and 255 max discard depth
						// if we reach highest value we just show something like 255+
						stats->pos.trDuration = CLAMP(ucmd.angles[1],0,(1<<14)-1) | (CLAMP(ucmd.angles[0], 0, (1 << 10) - 1)) << 14 | (CLAMP(ucmd.angles[2], 0, (1 << 8) - 1)) << 24;
						break;
					}
					cl->pers.segmented.playbackNextCmdIndex++;
				} else if (success && ucmd.serverTime == -2) {
					entityState_t* stats = &level.playerStats[ent - g_entities]->s;
					stats->apos.trTime = cl->pers.segmented.playbackNextCmdIndex == 0 ? cl->pers.segmented.playbackStartedTime + cl->pers.segmented.playbackStartedCommandTimeOffset : cl->ps.commandTime;
					stats->frame = cl->pers.segmented.playbackNextCmdIndex == 0 ? 1 : stats->frame + 1;
					cl->pers.segmented.playbackNextCmdIndex++;
				}
			}
			if (!success) {
				trap_G_COOL_API_PlayerUserCmdClear(ent-g_entities);
				cl->pers.segmented.lastPosCount = 0;
				cl->pers.segmented.lastPos[0].resposCount = 0;
				memset(&cl->pers.segmented.lastPos[0].discards,0,sizeof(cl->pers.segmented.lastPos[0].discards));
#if SEGMENTEDDEBUG
				memset(cl->pers.segmented.debugTime, 0, sizeof(cl->pers.segmented.debugTime));
#endif
				G_ResetUserCmdStore(ent - g_entities); // clear this so it doesn't execute very old ones now.
				G_GetUserCmd(ent - g_entities, &ent->client->pers.cmd, GETUSERCMD_NOADVANCE);
				SetClientViewAngle(ent,ent->client->ps.viewangles); // make a smooth transition back to player-controlled gameplay
				//ent->client->ps.commandTime = ent->client->pers.cmd.serverTime; // fuck it, we apply the offset at the start now so... whatever.
				ent->client->pers.segmented.state = SEG_DISABLED; // done
				if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
					// during replay, we are providing usercmds for server to send to spectators and player for demos
					ent->r.svFlags &= ~SVF_COOLAPI_GAMEGENERATEDSENDBACKUSERCMD;
				}
				ent->s.eFlags &= ~EF_SEGMENTEDREPLAY;
				cl->ps.eFlags &= ~EF_SEGMENTEDREPLAY;
				break;
			}
			targetServerTime = cl->pers.segmented.playbackStartedTime + cl->pers.segmented.playbackStartedCommandTimeOffset + ucmd.serverTime;

			currentPosHash = DF_GetPositionHash(&cl->ps);
			if (coolApi_userCmdVersion >= 1 && currentPosHash != posHash) {
				if (!cl->pers.segmented.playbackErrored) {
					G_SendServerCommand(ent - g_entities, va("print \"^3Segmented replay failing. You can try ^2/resseg^3 to restart the replay. (position hash does not match: ^1%d -> %d^3, further failed frame messages will be suppressed)\n\"", (int)posHash, (int)currentPosHash), qtrue);
				}
				cl->pers.segmented.playbackErrored = qtrue;
			}

			if (targetServerTime <= (level.time + cl->pers.segmented.playbackStartedCommandTimeOffset)) {
				cl->pers.cmd = ucmd;
#if SEGMENTEDDEBUG
				{
					int timeIndex = ucmd.serverTime / 100;
					if (timeIndex >= 0 && timeIndex < 1000) {
						if (cl->pers.segmented.debugTime[timeIndex] == ucmd.serverTime) {
							int i;
							for (i = 0; i < segDebugFieldsCount; i++) {
								void* ptrSrc = ((byte*)cl) + segDebugFields[i].offset;
								void* ptrDst = ((byte*)&cl->pers.segmented.debugVars[timeIndex]) + segDebugFields[i].offsetDebugVars;
								if (memcmp(ptrDst, ptrSrc, segDebugFields[i].typeSize)) {
									switch (segDebugFields[i].type) {
									case dbgtype_float:
									{
										float* current = ptrSrc;
										float* compare = ptrDst;
										float diff;
										diff = *current - *compare;
										G_SendServerCommand(ent - g_entities, va("print \"^1SEGDEBUG: ^%d%s^7(float) CHANGED: %f diff, %f -> %f \n\"", i, segDebugFields[i].name,
											fabsf(diff),
											(*current),
											(*compare)
										),qtrue);
									}
									break;
									case dbgtype_int:
									{
										int* current = ptrSrc;
										int* compare = ptrDst;
										int diff;
										diff = *current - *compare;
										G_SendServerCommand(ent - g_entities, va("print \"^1SEGDEBUG: ^%d%s^7(%s) CHANGED: %i diff, %i -> %i \n\"", i, segDebugFields[i].name, segDebugFields[i].typeName,
											abs(diff),
											(*current),
											(*compare)
										),qtrue);
									}
									case dbgtype_schar_t:
									{
										schar_t* current = ptrSrc;
										schar_t* compare = ptrDst;
										int diff;
										diff = *current - *compare;
										G_SendServerCommand(ent - g_entities, va("print \"^1SEGDEBUG: ^%d%s^7(%s) CHANGED: %i diff, %i -> %i \n\"", i, segDebugFields[i].name, segDebugFields[i].typeName,
											abs(diff),
											(*current),
											(*compare)
										),qtrue);
									}
									break;
									case dbgtype_vec3_t:
									{
										vec3_t* current = ptrSrc;
										vec3_t* compare = ptrDst;
										vec3_t diff;
										VectorSubtract(*current, *compare, diff);
										G_SendServerCommand(ent - g_entities, va("print \"^1SEGDEBUG: ^%d%s^7(vec3_t) CHANGED: %f diff, %f %f %f -> %f %f %f \n\"", i, segDebugFields[i].name,
											VectorLength(diff),
											(*current)[0],
											(*current)[1],
											(*current)[2],
											(*compare)[0],
											(*compare)[1],
											(*compare)[2]
										), qtrue);
									}
									break;
									case dbgtype_veci3_t:
									{
										veci3_t* current = ptrSrc;
										veci3_t* compare = ptrDst;
										vec3_t diff;
										VectorSubtract(*current, *compare, diff);
										G_SendServerCommand(ent - g_entities, va("print \"^1SEGDEBUG: ^%d%s^7(veci3_t) CHANGED: %f diff, %i %i %i -> %i %i %i \n\"", i, segDebugFields[i].name,
											VectorLength(diff),
											(*current)[0],
											(*current)[1],
											(*current)[2],
											(*compare)[0],
											(*compare)[1],
											(*compare)[2]
										),qtrue);
									}
									break;
									}
								}
							}

						}
					}
				}
#endif
				cl->pers.cmd.serverTime = targetServerTime;
				if (cl->pers.segmented.playbackNextCmdIndex == 0) {
					cl->ps.commandTime = cl->pers.segmented.playbackStartedTime + cl->pers.segmented.playbackStartedCommandTimeOffset; // shouldn't change anything? but be safe.
				}
				if (coolApi & COOL_APIFEATURE_SENDBACKUCMD_GAMEGENERATED) {
					// during replay, we are providing usercmds for server to send to spectators and player for demos
					trap_G_COOL_API_SendBackUCMD_GameGenerated(ent-g_entities,&cl->pers.cmd);
				}
				ClientThink_real(ent);
				cl->pers.segmented.playbackNextCmdIndex++;
			}
			else {
				break;
			}
		}

	}
	else {
		entityState_t* stats = &level.playerStats[ent - g_entities]->s;
		stats->apos.trTime = 0;
		stats->pos.trTime = 0;
		stats->frame = 0;

		ent->client->pers.cmd.serverTime = level.time;
		ClientThink_real(ent);
	}
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags;
		int		savedPing;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				savedPing = ent->client->ps.ping;
				ent->client->ps = cl->ps;
				ent->client->ps.ping = savedPing;
				// let's not overwrite ps ping, let that be the real one. but still overwrite this one to replicate old behavior
				// this is ok because ps->ping isn't networked anyway so we can just handle it differently internally
				ent->client->pers.normalFollowerPing = cl->ps.ping;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ent->client->pers.lastTeamInfoMessageSent = 0; // reset this
					memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) ); // Ensure following spectators don't take flags or such into ClientBegin and trigger the FlagEatingFix
					ClientBegin( ent->client - level.clients, qtrue );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

void ClientEndFrameRaceCritical(gentity_t* ent) {
	int i;
	int			nowTime = LEVELTIME(ent->client);

	// turn off any expired powerups
	for (i = 0; i < MAX_POWERUPS; i++) {
		if (ent->client->ps.powerups[i] < nowTime) {
			ent->client->ps.powerups[i] = 0;
		}
	}

	// save network bandwidth
#if 0
	if (!g_synchronousClients->integer && (ent->client->ps.pm_type == PM_NORMAL || ent->client->ps.pm_type == PM_FLOAT)) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear(ent->client->ps.viewangles);
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	G_SetClientSound(ent);
}

void ClientEndFrameServerFrame(gentity_t* ent) {
	// defrag: keep stuff below in a loop that is actually at the end to have up to date values.

	ClientSetStatHealth(ent->client, ent->health); // FIXME: get rid of ent->health...	

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if (level.time - ent->client->lastCmdTime > 1000 && !(ent->client->sess.raceMode && (ent->client->sess.raceStyle.runFlags & RFL_SEGMENTED) && ent->client->pers.segmented.state == SEG_REPLAY)) { // let it be ok during replays (if ppl time out/ disconnect)
		//fix so all clients get this information and can draw lag sprites if their client mod supports that // TA: but why would u wanna see it on urself?
		ent->client->ps.eFlags |= EF_CONNECTION;
		ent->s.eFlags |= EF_CONNECTION;
		if (level.time - ent->client->lastCmdTime > 3000) {
			G_ClearActivatedEntities(ent); // dont let this client bug out all the movers he touched while he's having connection issues
		}
	}
	else {
		ent->client->ps.eFlags &= ~EF_CONNECTION;
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	// set the latest infor
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate(&ent->client->ps, &ent->s, ent->client->ps.commandTime, g_snapPlayerPosAngles.integer);
	}
	else {
		BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, g_snapPlayerPosAngles.integer);
	}
	SendPendingPredictableEvents(&ent->client->ps);

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}
/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent, qboolean forceFull) {
	//int			i;
	//int			nowTime = LEVELTIME(ent->client);

	// this gets its own loop now.
	//if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
	//	SpectatorClientEndFrame( ent );
	//	return;
	//}

	if (!ent->client->sess.raceMode || forceFull) {
		ClientEndFrameRaceCritical(ent);
	}
	ClientEndFrameServerFrame(ent);
}

void ClientEndFrameInClientThink( gentity_t *ent ) {
	//int			i;
	//int			nowTime = LEVELTIME(ent->client);

	// this gets its own loop now.
	//if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
	//	SpectatorClientEndFrame( ent );
	//	return;
	//}

	if (ent->client->sess.raceMode) {
		ClientEndFrameRaceCritical(ent);
	}
}


