// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

#ifdef JK2_GAME
#include "g_local.h"
#elif JK2_CGAME
#include "../cgame/cg_local.h"
#endif

#define MAX_WEAPON_CHARGE_TIME 5000

extern qboolean PM_GroundSlideOkay(float zNormal);
extern float MovementOverbounceFactor(int moveStyle, playerState_t* ps, usercmd_t* ucmd);
extern void PM_CheckBounceJump(vec3_t normal, vec3_t velocity);
extern vec3_t flatNormal;


pmove_t		*pm;
pml_t		pml;

qboolean gPMDoSlowFall = qfalse;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.50f;
float	pm_swimScale = 0.50f;
float	pm_wadeScale = 0.70f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

//japro/dfmania movement parameters
const float pm_vq3_duckScale = 0.25f;
const float pm_vq3_friction = 8.0f;

const float	pm_cpm_accelerate = 15.0f;
const float	pm_cpm_airaccelerate = 1.0f;
const float	pm_cpm_airstopaccelerate = 2.5f;
const float	pm_cpm_airstrafeaccelerate = 70.0f;
const float	pm_cpm_airstrafewishspeed = 30.0f;


int		c_pmove = 0;

float forceSpeedLevels[4] = 
{
	1, //rank 0?
	1.25,
	1.5,
	1.75
};

int forcePowerNeeded_1_04[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] = 
{
	{ //nothing should be usable at rank 0..
		999,//FP_HEAL,//instant
		999,//FP_LEVITATION,//hold/duration
		999,//FP_SPEED,//duration
		999,//FP_PUSH,//hold/duration
		999,//FP_PULL,//hold/duration
		999,//FP_TELEPATHY,//instant
		999,//FP_GRIP,//hold/duration
		999,//FP_LIGHTNING,//hold/duration
		999,//FP_RAGE,//duration
		999,//FP_PROTECT,//duration
		999,//FP_ABSORB,//duration
		999,//FP_TEAM_HEAL,//instant
		999,//FP_TEAM_FORCE,//instant
		999,//FP_DRAIN,//hold/duration
		999,//FP_SEE,//duration
		999,//FP_SABERATTACK,
		999,//FP_SABERDEFEND,
		999//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		65,//FP_HEAL,//instant //was 25, but that was way too little
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		50,//FP_PROTECT,//duration
		50,//FP_ABSORB,//duration
		50,//FP_TEAM_HEAL,//instant
		50,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		2,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		60,//FP_HEAL,//instant
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		25,//FP_PROTECT,//duration
		25,//FP_ABSORB,//duration
		33,//FP_TEAM_HEAL,//instant
		33,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		1,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		50,//FP_HEAL,//instant //You get 5 points of health.. for 50 force points!
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		60,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		10,//FP_PROTECT,//duration
		10,//FP_ABSORB,//duration
		25,//FP_TEAM_HEAL,//instant
		25,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		0,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

int forcePowerNeeded_1_02[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] = 
{
	{ //nothing should be usable at rank 0..
		999,//FP_HEAL,//instant
		999,//FP_LEVITATION,//hold/duration
		999,//FP_SPEED,//duration
		999,//FP_PUSH,//hold/duration
		999,//FP_PULL,//hold/duration
		999,//FP_TELEPATHY,//instant
		999,//FP_GRIP,//hold/duration
		999,//FP_LIGHTNING,//hold/duration
		999,//FP_RAGE,//duration
		999,//FP_PROTECT,//duration
		999,//FP_ABSORB,//duration
		999,//FP_TEAM_HEAL,//instant
		999,//FP_TEAM_FORCE,//instant
		999,//FP_DRAIN,//hold/duration
		999,//FP_SEE,//duration
		999,//FP_SABERATTACK,
		999,//FP_SABERDEFEND,
		999//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		25,//FP_HEAL,//instant
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		50,//FP_PROTECT,//duration
		50,//FP_ABSORB,//duration
		50,//FP_TEAM_HEAL,//instant
		50,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		2,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		25,//FP_HEAL,//instant
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		25,//FP_PROTECT,//duration
		25,//FP_ABSORB,//duration
		33,//FP_TEAM_HEAL,//instant
		33,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		1,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		25,//FP_HEAL,//instant
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		60,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		10,//FP_PROTECT,//duration
		10,//FP_ABSORB,//duration
		25,//FP_TEAM_HEAL,//instant
		25,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABERATTACK,
		0,//FP_SABERDEFEND,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

int (*forcePowerNeeded)[NUM_FORCE_POWERS] = forcePowerNeeded_1_04;

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] = 
{
	32,//normal jump (+stepheight+crouchdiff = 66)
	96,//(+stepheight+crouchdiff = 130)
	192,//(+stepheight+crouchdiff = 226)
	384//(+stepheight+crouchdiff = 418)
};

float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] =
{
	66,//normal jump (32+stepheight(18)+crouchdiff(24) = 74)
	130,//(96+stepheight(18)+crouchdiff(24) = 138)
	226,//(192+stepheight(18)+crouchdiff(24) = 234)
	418//(384+stepheight(18)+crouchdiff(24) = 426)
};

//rww - Get a pointer to the bgEntity by the index
bgEntity_t* PM_BGEntForNum(int num)
{
	bgEntity_t* ent;

	if (!pm)
	{
		assert(!"You cannot call PM_BGEntForNum outside of pm functions!");
		return NULL;
	}

	if (!pm->baseEnt)
	{
		assert(!"Base entity address not set");
		return NULL;
	}

	if (!pm->entSize)
	{
		assert(!"sizeof(ent) is 0, impossible (not set?)");
		return NULL;
	}

	assert(num >= 0 && num < MAX_GENTITIES);

	ent = (bgEntity_t*)((byte*)pm->baseEnt + pm->entSize * (num));

	return ent;
}

void PM_GrabWallForJump(int anim)
{//NOTE!!! assumes an appropriate anim is being passed in!!!
	PM_SetAnim(SETANIM_BOTH, anim, SETANIM_FLAG_RESTART | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100);
	PM_AddEvent(EV_JUMP);//make sound for grab
	pm->ps->pm_flags |= PMF_STUCK_TO_WALL;
}

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] = 
{
	JUMP_VELOCITY,//normal jump
	420,
	590,
	840
};

Q_INLINE int PM_GetMovePhysics(void)
{
	if (!pm || !pm->ps)
		return MV_JK2;
#if JK2_GAME
	if (pm->ps->stats[STAT_RACEMODE])
		return (pm->ps->stats[STAT_MOVEMENTSTYLE]);
	//else if ((g_movementStyle.integer >= MV_SIEGE && g_movementStyle.integer <= MV_WSW) || g_movementStyle.integer == MV_SP)
	//	return (g_movementStyle.integer);
	//else if (g_movementStyle.integer < MV_SIEGE)
	//	return 0;
	//else if (g_movementStyle.integer >= MV_NUMSTYLES)
	//	return MV_JK2;
#elif JK2_CGAME
	if (cgs.isJK2Pro) {
		return cg.predictedPlayerState.stats[STAT_MOVEMENTSTYLE];
	}
	if (cgs.isTommyTernal && pm->ps->stats[STAT_RACEMODE]) {
		if (!pm) return MV_JK2; // not sure why this is needed. from japro.
		return pm->ps->stats[STAT_MOVEMENTSTYLE];
	}
	//if (cgs.gametype == GT_SIEGE)
	//	return MV_SIEGE;
#endif
	return MV_JK2; // this can happen when we die in racemode too!
}

Q_INLINE int PM_GetRunFlags(void)
{
	if (!pm || !pm->ps)
		return 0;
#if JK2_GAME
	if (pm->ps->stats[STAT_RACEMODE])
		return (pm->ps->stats[STAT_RUNFLAGS]);
	//else if ((g_movementStyle.integer >= MV_SIEGE && g_movementStyle.integer <= MV_WSW) || g_movementStyle.integer == MV_SP)
	//	return (g_movementStyle.integer);
	//else if (g_movementStyle.integer < MV_SIEGE)
	//	return 0;
	//else if (g_movementStyle.integer >= MV_NUMSTYLES)
	//	return MV_JK2;
#elif JK2_CGAME
	if (cgs.isTommyTernal && pm->ps->stats[STAT_RACEMODE]) {
		if (!pm) return defaultRunFlags; // not sure why this is needed. from japro.
		return pm->ps->stats[STAT_RUNFLAGS];
	}
	//if (cgs.gametype == GT_SIEGE)
	//	return MV_SIEGE;
#endif
	return 0; // this can happen when we die in racemode too!
}

Q_INLINE int PM_GetMsecRestrict(void)
{
	if (!pm || !pm->ps)
		return 0;
#if JK2_GAME
	if (pm->ps->stats[STAT_RACEMODE])
		return (pm->ps->stats[STAT_MSECRESTRICT]);
	//else if ((g_movementStyle.integer >= MV_SIEGE && g_movementStyle.integer <= MV_WSW) || g_movementStyle.integer == MV_SP)
	//	return (g_movementStyle.integer);
	//else if (g_movementStyle.integer < MV_SIEGE)
	//	return 0;
	//else if (g_movementStyle.integer >= MV_NUMSTYLES)
	//	return MV_JK2;
#elif JK2_CGAME
	if (cgs.isTommyTernal && pm->ps->stats[STAT_RACEMODE]) {
		if (!pm) return 0; // not sure why this is needed. from japro.
		return pm->ps->stats[STAT_MSECRESTRICT];
	}
	//if (cgs.gametype == GT_SIEGE)
	//	return MV_SIEGE;
#endif
	return 0; // this can happen when we die in racemode too!
}

int PM_GetSaberStance(void)
{
	if ( pm->ps->dualBlade )
	{
		return BOTH_STAND1;
	}
	if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2)
	{ //medium
		return BOTH_STAND2;
	}
	if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3)
	{ //strong
		return BOTH_SABERSLOW_STANCE;
	}

	//fast
	return BOTH_SABERFAST_STANCE;
}

qboolean PM_DoSlowFall(void)
{
	if ( ( (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT || (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_LEFT ) && pm->ps->legsTimer > 500 )
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

void PM_AddEventWithParm( int newEvent, int parm ) 
{
	BG_AddPredictableEventToPlayerstate( newEvent, parm, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;
	const int runFlags = PM_GetRunFlags();

	if ((runFlags & RFL_CLIMBTECH)&& (pm->ps->pm_flags & PMF_STUCK_TO_WALL))
	{//no sliding!
		VectorCopy(in, out);
		return;
	}
	
	backoff = DotProduct (in, normal);
	
	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop, realfriction = pm_friction;
	const int moveStyle = PM_GetMovePhysics();
	
	vel = pm->ps->velocity;
	
	VectorCopy( vel, vec );
	if ( pml.walking ) { 
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		if ((moveStyle == MV_BOUNCE || moveStyle == MV_PINBALL) && vel[2]) {
			vec[2] = vel[2]; // otherwise we stay forever in a bouncy vel[2] state on spawn and cant savespawn
			speed = VectorLength(vec);
		}
		else {
			vel[0] = 0;
			vel[1] = 0;		// allow sinking underwater
			// FIXME: still have z friction underwater?
			return;
		}
	}

	if (MovementIsQuake3Based(moveStyle))
		realfriction = pm_vq3_friction;

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control* realfriction *pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_FLOAT )
	{
		if (pm->ps->pm_type == PM_FLOAT)
		{ //almost no friction while floating
			drop += speed*0.1*pml.frametime;
		}
		else
		{
			drop += speed*pm_spectatorfriction*pml.frametime;
		}
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#if 1
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];	
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_SickoAccelerate( vec3_t wishdir, float wishspeed, float baseAccel, float maxAccel) {
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;
	float		baseInc, accel;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	baseInc = pml.frametime * wishspeed;

	accel = addspeed / baseInc;

	if (accel > maxAccel) {
		accel = maxAccel;
	}
	else if (accel < baseAccel) {
		accel = baseAccel;
	}

	accelspeed = accel* baseInc;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];	
	}
}
/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_QuaJKAccelerate( vec3_t wishdir, float wishspeed, float baseAccel, float maxAccel, float maxAccelWishSpeed) {
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;
	float		accel;
	float		f,finalWishSpeed;
	float		accelAddSlow, accelAddHigh;
	float		neededSpeedSlow, neededSpeedHigh;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);

	if (currentspeed >= wishspeed) return;

	accelAddSlow = baseAccel * pml.frametime * wishspeed;
	accelAddHigh = maxAccel * pml.frametime * maxAccelWishSpeed;

	neededSpeedSlow = wishspeed - accelAddSlow;
	neededSpeedHigh = maxAccelWishSpeed - accelAddHigh;

	f = (currentspeed - neededSpeedHigh) / (neededSpeedSlow - neededSpeedHigh);

	if (f < 0) f = 0;
	else if (f > 1) f = 1;

	accel = (f * baseAccel) + ((1.0f - f) * maxAccel);
	finalWishSpeed = (f * wishspeed) + ((1.0f - f) * maxAccelWishSpeed);

	accelspeed = accel * pml.frametime * finalWishSpeed;

	addspeed = finalWishSpeed - currentspeed; 
	if (addspeed <= 0) {
		return;
	}

	/*
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}

	baseInc = pml.frametime * wishspeed;

	accel = addspeed / baseInc;

	if (accel > maxAccel) {
		accel = maxAccel;
	}
	else if (accel < baseAccel) {
		accel = baseAccel;
	}

	f = (accel - baseAccel) / (maxAccel - baseAccel);

	finalWishSpeed = (f * maxAccelWishSpeed) + ((1.0f - f) * baseAccel);

	accelspeed = accel* pml.frametime*finalWishSpeed;*/
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];	
	}
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;
	int		umove = 0; //cmd->upmove;
			//don't factor upmove into scaling speed

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( umove ) > max ) {
		max = abs( umove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrtf( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + umove * umove );
	scale = (float)pm->ps->speed * max / ( 127.0f * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		} 
	}
}

#define METROID_JUMP 1

qboolean PM_ForceJumpingUp(void)
{
	if ( !(pm->ps->fd.forcePowersActive&(1<<FP_LEVITATION)) && pm->ps->fd.forceJumpCharge )
	{//already jumped and let go
		return qfalse;
	}

	if ( BG_InSpecialJump( pm->ps->legsAnim, PM_GetRunFlags() ) )
	{
		return qfalse;
	}

	if (BG_SaberInSpecial(pm->ps->saberMove))
	{
		return qfalse;
	}

	if (BG_SaberInSpecialAttack(pm->ps->legsAnim))
	{
		return qfalse;
	}

	if (BG_HasYsalamiri(pm->gametype, pm->ps))
	{
		return qfalse;
	}

	if (!BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION))
	{
		return qfalse;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE && //in air
		(pm->ps->pm_flags & PMF_JUMP_HELD) && //jumped
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 && //force-jump capable
		pm->ps->velocity[2] > 0 )//going up
	{
		return qtrue;
	}
	return qfalse;
}

void PM_JumpForDir( void )
{
	int anim = BOTH_JUMP1;
	if ( pm->cmd.forwardmove > 0 ) 
	{
		anim = BOTH_JUMP1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} 
	else if ( pm->cmd.forwardmove < 0 )
	{
		anim = BOTH_JUMPBACK1;
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
	else if ( pm->cmd.rightmove > 0 ) 
	{
		anim = BOTH_JUMPRIGHT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else if ( pm->cmd.rightmove < 0 ) 
	{
		anim = BOTH_JUMPLEFT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		anim = BOTH_JUMP1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	if(!BG_InDeathAnim(pm->ps->legsAnim))
	{
		PM_SetAnim(SETANIM_LEGS,anim,SETANIM_FLAG_OVERRIDE, 100);
	}
}

void PM_SetPMViewAngle(playerState_t *ps, vec3_t angle, usercmd_t *ucmd)
{
	int			i;

	for (i=0 ; i<3 ; i++)
	{ // set the delta angle
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ps->delta_angles[i] = cmdAngle - ucmd->angles[i];
	}
	VectorCopy (angle, ps->viewangles);
}

qboolean PM_AdjustAngleForWallRun( playerState_t *ps, usercmd_t *ucmd, qboolean doMove )
{
	if (( (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT || (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_LEFT ) && ps->legsTimer > 500 )
	{//wall-running and not at end of anim
		//stick to wall, if there is one
		vec3_t	rt, traceTo, mins, maxs, fwdAngles;
		trace_t	trace;
		float	dist, yawAdjust;

		VectorSet(mins, -15, -15, 0);
		VectorSet(maxs, 15, 15, 24);
		VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

		AngleVectors( fwdAngles, NULL, rt, NULL );
		if ( (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT )
		{
			dist = 128;
			yawAdjust = -90;
		}
		else
		{
			dist = -128;
			yawAdjust = 90;
		}
		VectorMA( ps->origin, dist, rt, traceTo );
		
		pm->trace( &trace, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID );

		if ( trace.fraction < 1.0f )
		{//still a wall there
			if ( (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT )
			{
				ucmd->rightmove = 127;
			}
			else
			{
				ucmd->rightmove = -127;
			}
			if ( ucmd->upmove < 0 )
			{
				ucmd->upmove = 0;
			}
			//make me face perpendicular to the wall
			// NOTE: Something about these 3 lines is perhaps not quite non-deterministic (or is it?) but
			// it makes replays not work properly. Why is that?
			ps->viewangles[YAW] = vectoyaw( trace.plane.normal )+yawAdjust;

			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);

			ucmd->angles[YAW] = ((int)(ANGLE2SHORT( ps->viewangles[YAW] ))) - (int)ps->delta_angles[YAW];
			ucmd->angles[YAW] &= 65535;
			if ( doMove )
			{
				//push me forward
				vec3_t	fwd;
				float	zVel = ps->velocity[2];
				if ( ps->legsTimer > 500 )
				{//not at end of anim yet
					float speed = 175;

					fwdAngles[YAW] = ps->viewangles[YAW];
					AngleVectors( fwdAngles, fwd, NULL, NULL );

					if ( ucmd->forwardmove < 0 )
					{//slower
						speed = 100;
					}
					else if ( ucmd->forwardmove > 0 )
					{
						speed = 250;//running speed
					}
					VectorScale( fwd, speed, ps->velocity );
				}
				ps->velocity[2] = zVel;//preserve z velocity
				//pull me toward the wall, too
				VectorMA( ps->velocity, dist, rt, ps->velocity );
			}
			ucmd->forwardmove = 0;
			return qtrue;
		}
		else if ( doMove )
		{//stop it
			if ( (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT )
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_WALL_RUN_RIGHT_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			}
			else if ( (ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_LEFT )
			{
				PM_SetAnim(SETANIM_BOTH, BOTH_WALL_RUN_LEFT_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			}
		}
	}

	return qfalse;
}

#define	JUMP_OFF_WALL_SPEED	200.0f
//nice...
static float BG_ForceWallJumpStrength(void)
{
	return (forceJumpStrength[FORCE_LEVEL_3] / 2.5f);
}
qboolean PM_AdjustAngleForWallJump(playerState_t* ps, usercmd_t* ucmd, qboolean doMove)
{
	if (((BG_InReboundJump(ps->legsAnim) || BG_InReboundHold(ps->legsAnim))
		&& (BG_InReboundJump(ps->torsoAnim) || BG_InReboundHold(ps->torsoAnim)))
		|| (pm->ps->pm_flags & PMF_STUCK_TO_WALL))
	{//hugging wall, getting ready to jump off
		//stick to wall, if there is one
		vec3_t	checkDir, traceTo, mins, maxs, fwdAngles;
		trace_t	trace;
		float	dist = 128.0f, yawAdjust;

		VectorSet(mins, pm->mins[0], pm->mins[1], 0);
		VectorSet(maxs, pm->maxs[0], pm->maxs[1], 24);
		VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

		switch (ps->legsAnim)
		{
		case BOTH_FORCEWALLREBOUND_RIGHT:
		case BOTH_FORCEWALLHOLD_RIGHT:
			AngleVectors(fwdAngles, NULL, checkDir, NULL);
			yawAdjust = -90;
			break;
		case BOTH_FORCEWALLREBOUND_LEFT:
		case BOTH_FORCEWALLHOLD_LEFT:
			AngleVectors(fwdAngles, NULL, checkDir, NULL);
			VectorScale(checkDir, -1, checkDir);
			yawAdjust = 90;
			break;
		case BOTH_FORCEWALLREBOUND_FORWARD:
		case BOTH_FORCEWALLHOLD_FORWARD:
			AngleVectors(fwdAngles, checkDir, NULL, NULL);
			yawAdjust = 180;
			break;
		case BOTH_FORCEWALLREBOUND_BACK:
		case BOTH_FORCEWALLHOLD_BACK:
			AngleVectors(fwdAngles, checkDir, NULL, NULL);
			VectorScale(checkDir, -1, checkDir);
			yawAdjust = 0;
			break;
		default:
			//WTF???
			pm->ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			return qfalse;
			break;
		}
		//if (pm->debugMelee)
		//[JAPRO - Serverside + Clientside - Physics - Change g_debugmelee 1 so that it has kungfu moves but keeps normal wallgrab.  Create g_debugmelee 2 for kung fu moves and infinite wallgrab - Start]
		if (pm->debugMelee > 1) // we go directly to the JAPLUS/jaPRO behavior in jk2. why not, we're porting what ppl are using
		{//uber-skillz
			if (ucmd->upmove > 0)
			{//hold on until you let go manually
				if (BG_InReboundHold(ps->legsAnim))
				{//keep holding
					if (ps->legsTimer < 150)
					{
						ps->legsTimer = 150;
					}
				}
				else
				{//if got to hold part of anim, play hold anim
					if (ps->legsTimer <= 300)
					{
						ps->saberHolstered = 2;
						PM_SetAnim(SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
						ps->legsTimer = ps->torsoTimer = 150;
					}
				}
			}
		}
		//[JAPRO - Serverside + Clientside - Physics - Change g_debugmelee 1 so that it has kungfu moves but keeps normal wallgrab.  Create g_debugmelee 2 for kung fu moves and infinite wallgrab - End]
		VectorMA(ps->origin, dist, checkDir, traceTo);
		pm->trace(&trace, ps->origin, mins, maxs, traceTo, ps->clientNum, MASK_PLAYERSOLID);
		if ( //ucmd->upmove <= 0 && 
			ps->legsTimer > 100 &&
			trace.fraction < 1.0f &&
			fabs(trace.plane.normal[2]) <= 0.2f/*MAX_WALL_GRAB_SLOPE*/)
		{//still a vertical wall there
			//FIXME: don't pull around 90 turns
			/*
			if ( ent->s.number || !player_locked )
			{
				ucmd->forwardmove = 127;
			}
			*/
			if (ucmd->upmove < 0)
			{
				ucmd->upmove = 0;
			}
			//align me to the wall
			ps->viewangles[YAW] = vectoyaw(trace.plane.normal) + yawAdjust;
			PM_SetPMViewAngle(ps, ps->viewangles, ucmd);
			/*
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			*/
			ucmd->angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - ps->delta_angles[YAW];
			//if ( ent->s.number || !player_locked )
			if (1)
			{
				if (doMove)
				{
					//pull me toward the wall
					VectorScale(trace.plane.normal, -128.0f, ps->velocity);
				}
			}
			ucmd->upmove = 0;
			ps->pm_flags |= PMF_STUCK_TO_WALL;
			return qtrue;
		}
		else if (doMove
			&& (ps->pm_flags & PMF_STUCK_TO_WALL))
		{//jump off
			//push off of it!
			ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			ps->velocity[0] = ps->velocity[1] = 0;
			VectorScale(checkDir, -JUMP_OFF_WALL_SPEED, ps->velocity);
			ps->velocity[2] = BG_ForceWallJumpStrength();
			ps->pm_flags |= PMF_JUMP_HELD;//PMF_JUMPING|PMF_JUMP_HELD;
			//G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
			ps->fd.forceJumpSound = 1; //this is a stupid thing, i should fix it.
			//ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
			if (ps->origin[2] < ps->fd.forceJumpZStart)
			{
				ps->fd.forceJumpZStart = ps->origin[2];
			}
			//FIXME do I need this?

			BG_ForcePowerDrain(ps, FP_LEVITATION, 10);
			//no control for half a second
			ps->pm_flags |= PMF_TIME_KNOCKBACK;
			ps->pm_time = 500;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 127;

			if (BG_InReboundHold(ps->legsAnim))
			{//if was in hold pose, release now
				PM_SetAnim(SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else
			{
				//PM_JumpForDir();
				PM_SetAnim(SETANIM_LEGS, BOTH_FORCEJUMP1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
			}

			//return qtrue;
		}
	}
	ps->pm_flags &= ~PMF_STUCK_TO_WALL;
	return qfalse;
}




//Set the height for when a force jump was started. If it's 0, nuge it up (slight hack to prevent holding jump over slopes)
void PM_SetForceJumpZStart(float value)
{
	pm->ps->fd.forceJumpZStart = value;
	if (!pm->ps->fd.forceJumpZStart && jk2gameplay == VERSION_1_04)
	{
		pm->ps->fd.forceJumpZStart -= 0.1f;
	}
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) 
{
	qboolean onlyWallGrab = qfalse; // in jk 1.02, if we are in air and not wallrunning, we skip out early. but we need to go further for wallgrab. in that case ignore all but wallgrab
	const int runFlags = PM_GetRunFlags();
	const int moveStyle = PM_GetMovePhysics();
	int JUMP_VELOCITY_NEW = JUMP_VELOCITY;

	if (pm->ps->usingATST)
	{
		return qfalse;
	}

	if (pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		return qfalse;
	}

	//Don't allow jump until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		
	}

	if ( PM_InKnockDown( pm->ps ) || BG_InRoll( pm->ps, pm->ps->legsAnim ) ) 
	{//in knockdown
		return qfalse;		
	}

	if (MovementIsQuake3Based(moveStyle)) {
		JUMP_VELOCITY_NEW = 270;
	}

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE || pm->ps->origin[2] < pm->ps->fd.forceJumpZStart)
	{
		pm->ps->fd.forcePowersActive &= ~(1<<FP_LEVITATION);
	}

	if (pm->ps->fd.forcePowersActive & (1 << FP_LEVITATION))
	{ //Force jump is already active.. continue draining power appropriately until we land.
		if (pm->ps->fd.forcePowerDebounce[FP_LEVITATION] < pm->cmd.serverTime)
		{
			BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 5 );
			if (pm->ps->fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_2)
			{
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 300;
			}
			else
			{
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 200;
			}
		}
	}

	if (pm->ps->forceJumpFlip)
	{ //Forced jump anim
		int anim = BOTH_FORCEINAIR1;
		int	parts = SETANIM_BOTH;

		if ( pm->cmd.forwardmove > 0 )
		{
			anim = BOTH_FLIP_F;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{
			anim = BOTH_FLIP_B;
		}
		else if ( pm->cmd.rightmove > 0 )
		{
			anim = BOTH_FLIP_R;
		}
		else if ( pm->cmd.rightmove < 0 )
		{
			anim = BOTH_FLIP_L;
		}
		if ( pm->ps->weaponTime )
		{//FIXME: really only care if we're in a saber attack anim...
			parts = SETANIM_LEGS;
		}

		PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150 );
		pm->ps->forceJumpFlip = qfalse;
		return qtrue;
	}
#if METROID_JUMP
	if ( pm->waterlevel < 2 ) 
	{
		if ( pm->ps->gravity > 0 )
		{//can't do this in zero-G
			if ( PM_ForceJumpingUp() )
			{//holding jump in air
				float curHeight = pm->ps->origin[2] - pm->ps->fd.forceJumpZStart;
				//check for max force jump level and cap off & cut z vel
				int runFlags = PM_GetRunFlags();
				if ( ( curHeight<=forceJumpHeight[0] ||//still below minimum jump height
						(pm->ps->fd.forcePower&&pm->cmd.upmove>=10) ) &&////still have force power available and still trying to jump up 
					curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] &&
					(pm->ps->fd.forceJumpZStart || jk2gameplay != VERSION_1_04 && !(runFlags & RFL_JUMPBUGDISABLE)))//still below maximum jump height
				{//can still go up
					if ( curHeight > forceJumpHeight[0] )
					{//passed normal jump height  *2?
						if ( !(pm->ps->fd.forcePowersActive&(1<<FP_LEVITATION)) )//haven't started forcejump yet
						{
							//start force jump
							pm->ps->fd.forcePowersActive |= (1<<FP_LEVITATION);
							pm->ps->fd.forceJumpSound = 1;
							//play flip
							if ((pm->cmd.forwardmove || pm->cmd.rightmove) && //pushing in a dir
								(pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_FLIP_F &&//not already flipping
								(pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_FLIP_B &&
								(pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_FLIP_R &&
								(pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_FLIP_L )
							{ 
								int anim = BOTH_FORCEINAIR1;
								int	parts = SETANIM_BOTH;

								if ( pm->cmd.forwardmove > 0 )
								{
									anim = BOTH_FLIP_F;
								}
								else if ( pm->cmd.forwardmove < 0 )
								{
									anim = BOTH_FLIP_B;
								}
								else if ( pm->cmd.rightmove > 0 )
								{
									anim = BOTH_FLIP_R;
								}
								else if ( pm->cmd.rightmove < 0 )
								{
									anim = BOTH_FLIP_L;
								}
								if ( pm->ps->weaponTime )
								{
									parts = SETANIM_LEGS;
								}

								PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150 );
							}
							else if ( pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 )
							{
								vec3_t facingFwd, facingRight, facingAngles;
								int	anim = -1;
								float dotR, dotF;
								
								VectorSet(facingAngles, 0, pm->ps->viewangles[YAW], 0);

								AngleVectors( facingAngles, facingFwd, facingRight, NULL );
								dotR = DotProduct( facingRight, pm->ps->velocity );
								dotF = DotProduct( facingFwd, pm->ps->velocity );

								if ( fabs(dotR) > fabs(dotF) * 1.5 )
								{
									if ( dotR > 150 )
									{
										anim = BOTH_FORCEJUMPRIGHT1;
									}
									else if ( dotR < -150 )
									{
										anim = BOTH_FORCEJUMPLEFT1;
									}
								}
								else
								{
									if ( dotF > 150 )
									{
										anim = BOTH_FORCEJUMP1;
									}
									else if ( dotF < -150 )
									{
										anim = BOTH_FORCEJUMPBACK1;
									}
								}
								if ( anim != -1 )
								{
									int parts = SETANIM_BOTH;
									if ( pm->ps->weaponTime )
									{//FIXME: really only care if we're in a saber attack anim...
										parts = SETANIM_LEGS;
									}

									PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150 );
								}
							}
						}
						else
						{ //jump is already active (the anim has started)
							if ( pm->ps->legsTimer < 1 )
							{//not in the middle of a legsAnim
								int anim = (pm->ps->legsAnim&~ANIM_TOGGLEBIT);
								int newAnim = -1;
								switch ( anim )
								{
								case BOTH_FORCEJUMP1:
									newAnim = BOTH_FORCELAND1;//BOTH_FORCEINAIR1;
									break;
								case BOTH_FORCEJUMPBACK1:
									newAnim = BOTH_FORCELANDBACK1;//BOTH_FORCEINAIRBACK1;
									break;
								case BOTH_FORCEJUMPLEFT1:
									newAnim = BOTH_FORCELANDLEFT1;//BOTH_FORCEINAIRLEFT1;
									break;
								case BOTH_FORCEJUMPRIGHT1:
									newAnim = BOTH_FORCELANDRIGHT1;//BOTH_FORCEINAIRRIGHT1;
									break;
								}
								if ( newAnim != -1 )
								{
									int parts = SETANIM_BOTH;
									if ( pm->ps->weaponTime )
									{
										parts = SETANIM_LEGS;
									}

									PM_SetAnim( parts, newAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150 );
								}
							}
						}
					}

					//need to scale this down, start with height velocity (based on max force jump height) and scale down to regular jump vel
					
					if (MovementIsQuake3Based(moveStyle)) {//Forcejump rampjump
						//need to scale this down, start with height velocity (based on max force jump height) and scale down to regular jump vel
						float realForceJumpHeight = forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] * (pm->ps->stats[STAT_LASTJUMPSPEED] / (float)JUMP_VELOCITY_NEW);

						pm->ps->velocity[2] = (realForceJumpHeight-curHeight)/realForceJumpHeight*forceJumpStrength[pm->ps->fd.forcePowerLevel[FP_LEVITATION]];//JUMP_VELOCITY;
						pm->ps->velocity[2] /= 10;//need to scale this down, start with height velocity (based on max force jump height) and scale down to regular jump vel

						pm->ps->velocity[2] += pm->ps->stats[STAT_LASTJUMPSPEED];
					}
					else {
						pm->ps->velocity[2] = (forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]]-curHeight)/forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]]*forceJumpStrength[pm->ps->fd.forcePowerLevel[FP_LEVITATION]];//JUMP_VELOCITY;
						pm->ps->velocity[2] /= 10;
						pm->ps->velocity[2] += JUMP_VELOCITY_NEW;
					}
					pm->ps->pm_flags |= PMF_JUMP_HELD;
				}
				else if ( curHeight > forceJumpHeight[0] && curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - forceJumpHeight[0] )
				{//still have some headroom, don't totally stop it
					if ( pm->ps->velocity[2] > JUMP_VELOCITY_NEW)
					{
						pm->ps->velocity[2] = JUMP_VELOCITY_NEW;
					}
				}
				else
				{
					//pm->ps->velocity[2] = 0;
					//rww - changed for the sake of balance in multiplayer

					if ( pm->ps->velocity[2] > JUMP_VELOCITY_NEW)
					{
						pm->ps->velocity[2] = JUMP_VELOCITY_NEW;
					}
				}
				pm->cmd.upmove = 0;
				return qfalse;
			}
			else if ( jk2gameplay == VERSION_1_02 && pm->ps->groundEntityNum == ENTITYNUM_NONE )
			{
				int legsAnim = (pm->ps->legsAnim&~ANIM_TOGGLEBIT);
				if ( legsAnim != BOTH_WALL_RUN_LEFT && legsAnim != BOTH_WALL_RUN_RIGHT )
				{//special case.. these let you jump off a wall
					if (runFlags & RFL_CLIMBTECH) {
						// gotta allow for wallgrab
						onlyWallGrab = qtrue;
					}
					else {
						return qfalse;
					}
				}
			}

		}
	}

#endif

	//Not jumping
	if ( pm->cmd.upmove < 10 && (pm->ps->groundEntityNum != ENTITYNUM_NONE || jk2gameplay == VERSION_1_02)) {
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) 
	{
		// clear upmove so cmdscale doesn't lower running speed
		if(!onlyWallGrab){
			pm->cmd.upmove = 0;
		}
		return qfalse;
	}

	if ( pm->ps->gravity <= 0 )
	{//in low grav, you push in the dir you're facing as long as there is something behind you to shove off of
		vec3_t	forward, back;
		trace_t	trace;

		if(!onlyWallGrab){
			AngleVectors( pm->ps->viewangles, forward, NULL, NULL );
			VectorMA( pm->ps->origin, -8, forward, back );
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, back, pm->ps->clientNum, pm->tracemask );

			if ( trace.fraction <= 1.0f )
			{
				VectorMA( pm->ps->velocity, JUMP_VELOCITY_NEW*2, forward, pm->ps->velocity );
				PM_SetAnim(SETANIM_LEGS,BOTH_FORCEJUMP1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART, 150);
			}//else no surf close enough to push off of
			pm->cmd.upmove = 0;
		}
	}
	else if ( pm->cmd.upmove > 0 && pm->waterlevel < 2 &&
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 &&
		!(pm->ps->pm_flags&PMF_JUMP_HELD) &&
		pm->ps->weapon == WP_SABER &&
		!BG_HasYsalamiri(pm->gametype, pm->ps) &&
		BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION) )
	{
		if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
		{//on the ground
			//check for left-wall and right-wall special jumps
			int anim = -1;
			float	vertPush = 0;
			if ( pm->cmd.rightmove > 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 )
			{//strafing right
				if ( pm->cmd.forwardmove > 0 )
				{//wall-run
					vertPush = forceJumpStrength[FORCE_LEVEL_2]/2.0f;
					anim = BOTH_WALL_RUN_RIGHT;
				}
				else if ( pm->cmd.forwardmove == 0 )
				{//wall-flip
					vertPush = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
					anim = BOTH_WALL_FLIP_RIGHT;
				}
			}
			else if ( pm->cmd.rightmove < 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 )
			{//strafing left
				if ( pm->cmd.forwardmove > 0 )
				{//wall-run
					vertPush = forceJumpStrength[FORCE_LEVEL_2]/2.0f;
					anim = BOTH_WALL_RUN_LEFT;
				}
				else if ( pm->cmd.forwardmove == 0 )
				{//wall-flip
					vertPush = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
					anim = BOTH_WALL_FLIP_LEFT;
				}
			}
			else if ( jk2gameplay == VERSION_1_02 && pm->cmd.forwardmove > 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 )
			{//run up wall, flip backwards
				vertPush = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
				anim = BOTH_WALL_FLIP_BACK1;
			}
			else if ( pm->cmd.forwardmove < 0 && !(pm->cmd.buttons&BUTTON_ATTACK) )
			{//backflip
				vertPush = JUMP_VELOCITY_NEW;
				anim = BOTH_FLIP_BACK1;//PM_PickAnim( BOTH_FLIP_BACK1, BOTH_FLIP_BACK3 );
			}

			vertPush += 128; //give them an extra shove

			if ( anim != -1 )
			{
				vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
				vec3_t	idealNormal;
				trace_t	trace;
				qboolean doTrace = qfalse;
				int contents = MASK_PLAYERSOLID;

				VectorSet(mins, pm->mins[0],pm->mins[1],0);
				VectorSet(maxs, pm->maxs[0],pm->maxs[1],24);
				VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

				memset(&trace, 0, sizeof(trace)); //to shut the compiler up

				AngleVectors( fwdAngles, fwd, right, NULL );

				//trace-check for a wall, if necc.
				switch ( anim )
				{
				case BOTH_WALL_FLIP_LEFT:
					//NOTE: purposely falls through to next case!
				case BOTH_WALL_RUN_LEFT:
					doTrace = qtrue;
					VectorMA( pm->ps->origin, -16, right, traceto );
					break;

				case BOTH_WALL_FLIP_RIGHT:
					//NOTE: purposely falls through to next case!
				case BOTH_WALL_RUN_RIGHT:
					doTrace = qtrue;
					VectorMA( pm->ps->origin, 16, right, traceto );
					break;

				case BOTH_WALL_FLIP_BACK1:
					doTrace = qtrue;
					VectorMA( pm->ps->origin, 16, fwd, traceto );
					break;
				}

				if ( doTrace )
				{
					pm->trace( &trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, contents );
					VectorSubtract( pm->ps->origin, traceto, idealNormal );
					VectorNormalize( idealNormal );
				}

				if ( !doTrace || (trace.fraction < 1.0f && (trace.entityNum < MAX_CLIENTS || DotProduct(trace.plane.normal,idealNormal) > 0.7f)) )
				{//there is a wall there.. or hit a client
					int parts;
					//move me to side
					if ( anim == BOTH_WALL_FLIP_LEFT )
					{
						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA( pm->ps->velocity, 150, right, pm->ps->velocity );
					}
					else if ( anim == BOTH_WALL_FLIP_RIGHT )
					{
						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA( pm->ps->velocity, -150, right, pm->ps->velocity );
					}
					else if ( anim == BOTH_FLIP_BACK1 
						|| anim == BOTH_FLIP_BACK2 
						|| anim == BOTH_FLIP_BACK3 
						|| anim == BOTH_WALL_FLIP_BACK1 )
					{
						pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
						VectorMA( pm->ps->velocity, -150, fwd, pm->ps->velocity );
					}

					if ( doTrace && anim != BOTH_WALL_RUN_LEFT && anim != BOTH_WALL_RUN_RIGHT )
					{
						if (trace.entityNum < MAX_CLIENTS)
						{
							pm->ps->forceKickFlip = trace.entityNum+1; //let the server know that this person gets kicked by this client
						}
					}

					//up
					if ( vertPush )
					{
						pm->ps->velocity[2] = vertPush;
						pm->ps->fd.forcePowersActive |= (1 << FP_LEVITATION);
					}
					//animate me
					parts = SETANIM_LEGS;
					if ( anim == BOTH_BUTTERFLY_LEFT )
					{
						parts = SETANIM_BOTH;
						pm->cmd.buttons&=~BUTTON_ATTACK;
						pm->ps->saberMove = LS_NONE;
					}
					else if ( !pm->ps->weaponTime )
					{
						parts = SETANIM_BOTH;
					}
					PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
					if ( anim == BOTH_BUTTERFLY_LEFT )
					{
						pm->ps->weaponTime = pm->ps->torsoTimer;
					}
					PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height
					pm->ps->pm_flags |= PMF_JUMP_HELD;
					pm->cmd.upmove = 0;
					pm->ps->fd.forceJumpSound = 1;
				}
			}
		}
		else 
		{//in the air
			int legsAnim = (pm->ps->legsAnim&~ANIM_TOGGLEBIT);
			if ( legsAnim == BOTH_WALL_RUN_LEFT || legsAnim == BOTH_WALL_RUN_RIGHT )
			{//running on a wall
				vec3_t right, traceto, mins, maxs, fwdAngles;
				trace_t	trace;
				int		anim = -1;

				if (onlyWallGrab) return qfalse;

				VectorSet(mins, pm->mins[0], pm->mins[0], 0);
				VectorSet(maxs, pm->maxs[0], pm->maxs[0], 24);
				VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

				AngleVectors( fwdAngles, NULL, right, NULL );

				if ( legsAnim == BOTH_WALL_RUN_LEFT )
				{
					if ( pm->ps->legsTimer > 400 )
					{//not at the end of the anim
						float animLen = PM_AnimLength( 0, (animNumber_t)BOTH_WALL_RUN_LEFT );
						if ( pm->ps->legsTimer < animLen - 400 )
						{//not at start of anim
							VectorMA( pm->ps->origin, -16, right, traceto );
							anim = BOTH_WALL_RUN_LEFT_FLIP;
						}
					}
				}
				else if ( legsAnim == BOTH_WALL_RUN_RIGHT )
				{
					if ( pm->ps->legsTimer > 400 )
					{//not at the end of the anim
						float animLen = PM_AnimLength( 0, (animNumber_t)BOTH_WALL_RUN_RIGHT );
						if ( pm->ps->legsTimer < animLen - 400 )
						{//not at start of anim
							VectorMA( pm->ps->origin, 16, right, traceto );
							anim = BOTH_WALL_RUN_RIGHT_FLIP;
						}
					}
				}
				if ( anim != -1 )
				{
					pm->trace( &trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY );
					if ( trace.fraction < 1.0f )
					{//flip off wall
						int parts = 0;

						if ( anim == BOTH_WALL_RUN_LEFT_FLIP )
						{
							pm->ps->velocity[0] *= 0.5f;
							pm->ps->velocity[1] *= 0.5f;
							VectorMA( pm->ps->velocity, 150, right, pm->ps->velocity );
						}
						else if ( anim == BOTH_WALL_RUN_RIGHT_FLIP )
						{
							pm->ps->velocity[0] *= 0.5f;
							pm->ps->velocity[1] *= 0.5f;
							VectorMA( pm->ps->velocity, -150, right, pm->ps->velocity );
						}
						parts = SETANIM_LEGS;
						if ( !pm->ps->weaponTime )
						{
							parts = SETANIM_BOTH;
						}
						PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
						pm->cmd.upmove = 0;
					}
				}
				if ( pm->cmd.upmove != 0 )
				{//jump failed, so don't try to do normal jump code, just return
					return qfalse;
				}
			}
			else if ( jk2gameplay != VERSION_1_02 &&
				pm->cmd.forwardmove > 0 //pushing forward
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1
				&& pm->ps->velocity[2] > 200
				&& PM_GroundDistance() <= 80 //unfortunately we do not have a happy ground timer like SP (this would use up more bandwidth if we wanted prediction workign right), so we'll just use the actual ground distance.
				&& !BG_InSpecialJump(pm->ps->legsAnim, runFlags) )
			{//run up wall, flip backwards
				vec3_t fwd, traceto, mins, maxs, fwdAngles;
				trace_t	trace;
				vec3_t	idealNormal;

				if (onlyWallGrab) return qfalse;

				VectorSet(mins, pm->mins[0],pm->mins[1],pm->mins[2]);
				VectorSet(maxs, pm->maxs[0],pm->maxs[1],pm->maxs[2]);
				VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

				AngleVectors( fwdAngles, fwd, NULL, NULL );
				VectorMA( pm->ps->origin, 32, fwd, traceto );

				pm->trace( &trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, MASK_PLAYERSOLID );//FIXME: clip brushes too?
				VectorSubtract( pm->ps->origin, traceto, idealNormal );
				VectorNormalize( idealNormal );
				
				if ( trace.fraction < 1.0f )
				{//there is a wall there
					int parts = SETANIM_LEGS;

					pm->ps->velocity[0] = pm->ps->velocity[1] = 0;
					VectorMA( pm->ps->velocity, -150, fwd, pm->ps->velocity );
					pm->ps->velocity[2] += 128;

					if ( !pm->ps->weaponTime )
					{
						parts = SETANIM_BOTH;
					}
					PM_SetAnim( parts, BOTH_WALL_FLIP_BACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );

					pm->ps->legsTimer -= 600; //I force this anim to play to the end to prevent landing on your head and suddenly flipping over.
											  //It is a bit too long at the end though, so I'll just shorten it.

					PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height
					pm->cmd.upmove = 0;
					pm->ps->fd.forceJumpSound = 1;
					BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 5 );

					if (trace.entityNum < MAX_CLIENTS)
					{
						pm->ps->forceKickFlip = trace.entityNum+1; //let the server know that this person gets kicked by this client
					}
				}
			} 
			else if ( (runFlags & RFL_CLIMBTECH) &&
					(!BG_InSpecialJump( legsAnim ,runFlags)//not in a special jump anim
						||BG_InReboundJump( legsAnim )//we're already in a rebound
						||BG_InBackFlip( legsAnim ) )//a backflip (needed so you can jump off a wall behind you)
					//&& pm->ps->velocity[2] <= 0
					&& pm->ps->velocity[2] > -1200 //not falling down very fast
					&& !(pm->ps->pm_flags&PMF_JUMP_HELD)//have to have released jump since last press
					&& (pm->cmd.forwardmove||pm->cmd.rightmove)//pushing in a direction
					//&& pm->ps->forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
					&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2//level 3 jump or better
					//&& WP_ForcePowerAvailable( pm->gent, FP_LEVITATION, 10 )//have enough force power to do another one
					&& BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION)
					&& (pm->ps->origin[2]-pm->ps->fd.forceJumpZStart) < (forceJumpHeightMax[FORCE_LEVEL_3]-(BG_ForceWallJumpStrength()/2.0f)) //can fit at least one more wall jump in (yes, using "magic numbers"... for now)
					//&& (pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_INAIR1 ) )//not in a flip or spin or anything
					)
			{//see if we're pushing at a wall and jump off it if so
				//if ( allowWallGrabs )
				if ( qtrue )
				{
					//FIXME: make sure we have enough force power
					//FIXME: check  to see if we can go any higher
					//FIXME: limit to a certain number of these in a row?
					//FIXME: maybe don't require a ucmd direction, just check all 4?
					//FIXME: should stick to the wall for a second, then push off...
					vec3_t checkDir, traceto, mins, maxs, fwdAngles;
					trace_t	trace;
					vec3_t	idealNormal;
					int		anim = -1;

					VectorSet(mins, pm->mins[0], pm->mins[1], 0.0f);
					VectorSet(maxs, pm->maxs[0], pm->maxs[1], 24.0f);
					VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0.0f);

					if ( pm->cmd.rightmove )
					{
						if ( pm->cmd.rightmove > 0 )
						{
							anim = BOTH_FORCEWALLREBOUND_RIGHT;
							AngleVectors( fwdAngles, NULL, checkDir, NULL );
						}
						else if ( pm->cmd.rightmove < 0 )
						{
							anim = BOTH_FORCEWALLREBOUND_LEFT;
							AngleVectors( fwdAngles, NULL, checkDir, NULL );
							VectorScale( checkDir, -1, checkDir );
						}
					}
					else if ( pm->cmd.forwardmove > 0 )
					{
						anim = BOTH_FORCEWALLREBOUND_FORWARD;
						AngleVectors( fwdAngles, checkDir, NULL, NULL );
					}
					else if ( pm->cmd.forwardmove < 0 )
					{
						anim = BOTH_FORCEWALLREBOUND_BACK;
						AngleVectors( fwdAngles, checkDir, NULL, NULL );
						VectorScale( checkDir, -1, checkDir );
					}
					if ( anim != -1 )
					{//trace in the dir we're pushing in and see if there's a vertical wall there
						bgEntity_t *traceEnt;

						VectorMA( pm->ps->origin, 8, checkDir, traceto );
						pm->trace( &trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, CONTENTS_SOLID );//FIXME: clip brushes too?
						VectorSubtract( pm->ps->origin, traceto, idealNormal );
						VectorNormalize( idealNormal );
						traceEnt = PM_BGEntForNum(trace.entityNum);
						if ( trace.fraction < 1.0f
							&&fabs(trace.plane.normal[2]) <= 0.2f/*MAX_WALL_GRAB_SLOPE*/
							&&((trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid!=SOLID_BMODEL)||DotProduct(trace.plane.normal,idealNormal)>0.7f) )
						{//there is a wall there
							float dot = DotProduct( pm->ps->velocity, trace.plane.normal );
							if ( dot < 1.0f )
							{//can't be heading *away* from the wall!
								//grab it!
								PM_GrabWallForJump( anim );
							}
						}
					}
				}
			}
		}
	}

	if ( !onlyWallGrab
		&& pm->cmd.upmove > 0 
		&& pm->ps->weapon == WP_SABER
		&& (pm->ps->weaponTime > 0||pm->cmd.buttons&BUTTON_ATTACK) )
	{//okay, we just jumped and we're in an attack
		if ( !BG_InRoll( pm->ps, pm->ps->legsAnim )
			&& !PM_InKnockDown( pm->ps )
			&& !BG_InDeathAnim(pm->ps->legsAnim)
			&& !BG_FlippingAnim( pm->ps->legsAnim )
			&& !PM_SpinningAnim( pm->ps->legsAnim )
			&& !BG_SaberInSpecialAttack( pm->ps->torsoAnim )
			&& ( BG_SaberInAttack( pm->ps->saberMove ) ) )
		{//not in an anim we shouldn't interrupt
			//see if it's not too late to start a special jump-attack
			float animLength = PM_AnimLength( 0, (animNumber_t)pm->ps->torsoAnim );
			if ( animLength - pm->ps->torsoTimer < 500 )
			{//just started the saberMove
				//check for special-case jump attacks

				//const int runFlags = PM_GetMovePhysics();

				if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2 )
				{//using medium attacks
					if (PM_GroundDistance() < 32 &&
						!BG_InSpecialJump(pm->ps->legsAnim, runFlags))
					{ //FLIP AND DOWNWARD ATTACK
						trace_t tr;

						if (PM_SomeoneInFront(&tr))
						{
							PM_SetSaberMove(PM_SaberFlipOverAttackMove(&tr));
							pml.groundPlane = qfalse;
							pml.walking = qfalse;
							pm->ps->pm_flags |= PMF_JUMP_HELD;
							pm->ps->groundEntityNum = ENTITYNUM_NONE;
							VectorClear(pml.groundTrace.plane.normal);

							pm->ps->weaponTime = pm->ps->torsoTimer;
						}
					}
				}
				else if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3 )
				{//using strong attacks
					if ( //!(runFlags & RFL_CLIMBTECH) && // using JKA dfa instead then?
						pm->cmd.forwardmove > 0 && //going forward
						((pm->cmd.buttons & BUTTON_ATTACK) || jk2gameplay == VERSION_1_02) && //must be holding attack still
						PM_GroundDistance() < 32 &&
						!BG_InSpecialJump(pm->ps->legsAnim, runFlags))
					{//strong attack: jump-hack
						PM_SetSaberMove( PM_SaberJumpAttackMove() );
						pml.groundPlane = qfalse;
						pml.walking = qfalse;
						pm->ps->pm_flags |= PMF_JUMP_HELD;
						pm->ps->groundEntityNum = ENTITYNUM_NONE;
						VectorClear(pml.groundTrace.plane.normal);

						pm->ps->weaponTime = pm->ps->torsoTimer;
					}
				}
			}
		}
	}
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}
	if ( pm->cmd.upmove > 0 )
	{//no special jumps
		if (MovementIsQuake3Based(moveStyle)) {
			// TODO flood protect jumps? idk
			pm->ps->velocity[2] += JUMP_VELOCITY_NEW;
			if (pm->ps->velocity[2] < 270)
				pm->ps->velocity[2] = 270;
			pm->ps->stats[STAT_LASTJUMPSPEED] = pm->ps->velocity[2];
		}
		else {
			pm->ps->velocity[2] = JUMP_VELOCITY_NEW;
		}
		PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height
		pm->ps->pm_flags |= PMF_JUMP_HELD;
	}

	//Jumping
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	PM_SetForceJumpZStart(pm->ps->origin[2]);

	PM_AddEvent( EV_JUMP );

	//Set the animations
	if ( pm->ps->gravity > 0 && !BG_InSpecialJump( pm->ps->legsAnim, runFlags) )
	{
		PM_JumpForDir();
	}

	return qtrue;
}
/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( cont ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	vel;
	int moveStyle = PM_GetMovePhysics();


	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
	} else {
		for (i=0 ; i<3 ; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, MovementOverbounceFactor(moveStyle, pm->ps, &pm->cmd));

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	
	if ( pm->ps->pm_type == PM_SPECTATOR && pm->cmd.buttons & BUTTON_ALT_ATTACK) { // MVSDK: 1.03+ feature, but let's enable it on 1.02 as well.
		//turbo boost
		scale *= 10;
	}

	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = pm->ps->speed * (pm->cmd.upmove/127.0f); // MVSDK: 1.02 originally put this to 0, but let's use 1.03+ behaviour for this as well.
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	const int	runFlags = PM_GetRunFlags();
	const int	movePhysics = PM_GetMovePhysics();
	float		overbounce = MovementOverbounceFactor(movePhysics, pm->ps, &pm->cmd);

	if (pm->ps->pm_type != PM_SPECTATOR)
	{
#if METROID_JUMP
		PM_CheckJump();
#else
		if (pm->ps->fd.forceJumpZStart &&
			pm->ps->forceJumpFlip)
		{
			PM_CheckJump();
		}
#endif
	}
	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	if ( gPMDoSlowFall )
	{//no air-control
		VectorClear( wishvel );
	}
	else
	{
		for ( i = 0 ; i < 2 ; i++ )
		{
			wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
		}
		wishvel[2] = 0;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	if (movePhysics == MV_SICKO) {
		PM_SickoAccelerate(wishdir, wishspeed, pm_airaccelerate,200.0f);
	}
	else if (movePhysics == MV_QUAJK) {
		float		accel;

		if (DotProduct(pm->ps->velocity, wishdir) < 0)
			accel = pm_cpm_airstopaccelerate;
		else
			accel = pm_airaccelerate;
		PM_QuaJKAccelerate(wishdir, wishspeed, accel,pm_cpm_airstrafeaccelerate,30.0f);
	}
	else {
		PM_Accelerate(wishdir, wishspeed, pm_airaccelerate);
	}

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		if (runFlags & RFL_CLIMBTECH) {
			if (!(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
			{//don't slide when stuck to a wall
				if (PM_GroundSlideOkay(pml.groundTrace.plane.normal[2]))
				{
					PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
						pm->ps->velocity, overbounce);
				}
			}
		}
		else {
			PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
				pm->ps->velocity, overbounce);
		}
	}

	PM_StepSlideMove ( qtrue );

	if (pml.groundBounces) {
		PM_CheckBounceJump(flatNormal, pm->ps->velocity);
	}
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;
	float		totalVel;
	const int	moveStyle = PM_GetMovePhysics();
	float		overbounce = MovementOverbounceFactor(moveStyle, pm->ps, &pm->cmd);

	if (pm->ps->velocity[0] < 0)
	{
		totalVel = -pm->ps->velocity[0];
	}
	else
	{
		totalVel = pm->ps->velocity[0];
	}

	if (pm->ps->velocity[1] < 0)
	{
		totalVel += -pm->ps->velocity[1];
	}
	else
	{
		totalVel += pm->ps->velocity[1];
	}

	if (totalVel < 200)
	{
		pm->ps->fd.forceSpeedSmash = 1;
	}

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if (pm->ps->pm_type != PM_SPECTATOR)
	{
		if ( PM_CheckJump () ) {
			// jumped away
			if ( pm->waterlevel > 1 ) {
				PM_WaterMove();
			} else {
				PM_AirMove();
			}
			return;
		}
	}

	PM_Friction ();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, overbounce);
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, overbounce);
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	// when going up or down slopes the wish velocity should Not be zero

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}
	else if ( (pm->ps->pm_flags & PMF_ROLLING) && !BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		!PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
		if (MovementIsQuake3Based(moveStyle))
			accelerate = pm_cpm_accelerate;
	} else {
		accelerate = pm_accelerate;
		if (MovementIsQuake3Based(moveStyle))
			accelerate = pm_cpm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
		pm->ps->velocity, overbounce);
	PM_CheckBounceJump(pml.groundTrace.plane.normal, pm->ps->velocity);// allow jump out of a bounce

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		pm->ps->fd.forceSpeedSmash = 1;
		return;
	}

	PM_StepSlideMove( qfalse );

	if (pml.groundBounces) {
		PM_CheckBounceJump(flatNormal, pm->ps->velocity);
	}

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );
	if (pm->cmd.buttons & BUTTON_ATTACK) {	//turbo boost
		scale *= 10;
	}
	if (pm->cmd.buttons & BUTTON_ALT_ATTACK) {	//turbo boost
		scale *= 10;
	}

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void )
{
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) 
	{
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) 
	{
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}

static int PM_TryRoll( void )
{
	trace_t	trace;
	int		anim = -1;
	vec3_t fwd, right, traceto, mins, maxs, fwdAngles;

	if ( BG_SaberInAttack( pm->ps->saberMove ) || BG_SaberInSpecialAttack( pm->ps->torsoAnim ) 
		|| BG_SpinningSaberAnim( pm->ps->legsAnim ) 
		|| (jk2gameplay != VERSION_1_02 && PM_SaberInStart( pm->ps->saberMove )) ) // MVSDK: In 1.02 everyone except client 0 could roll during SaberInStart. In 1.03 and later nobody could roll during SaberInStart. 1.02 people consider client 0 being unable to roll as client 0 bug, so let him roll, too.
	{//attacking or spinning (or, if player, starting an attack)
		return 0;
	}

	// TODO MAYBE jaPRO ysal stuff here?

	if (pm->ps->weapon != WP_SABER || BG_HasYsalamiri(pm->gametype, pm->ps) ||
		!BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION))
	{ //Not using saber, or can't use jump
		return 0;
	}

	VectorSet(mins, pm->mins[0],pm->mins[1],pm->mins[2]+STEPSIZE);
	VectorSet(maxs, pm->maxs[0],pm->maxs[1],CROUCH_MAXS_2);

	VectorSet(fwdAngles, 0, pm->ps->viewangles[YAW], 0);

	AngleVectors( fwdAngles, fwd, right, NULL );

	if ( pm->cmd.forwardmove )
	{ //check forward/backward rolls
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) 
		{
			anim = BOTH_ROLL_B;
			VectorMA( pm->ps->origin, -64, fwd, traceto );
		}
		else
		{
			anim = BOTH_ROLL_F;
			VectorMA( pm->ps->origin, 64, fwd, traceto );
		}
	}
	else if ( pm->cmd.rightmove > 0 )
	{ //right
		anim = BOTH_ROLL_R;
		VectorMA( pm->ps->origin, 64, right, traceto );
	}
	else if ( pm->cmd.rightmove < 0 )
	{ //left
		anim = BOTH_ROLL_L;
		VectorMA( pm->ps->origin, -64, right, traceto );
	}

	if ( anim != -1 )
	{ //We want to roll. Perform a trace to see if we can, and if so, send us into one.
		pm->trace( &trace, pm->ps->origin, mins, maxs, traceto, pm->ps->clientNum, CONTENTS_SOLID );
		if ( trace.fraction >= 1.0f )
		{
			pm->ps->saberMove = LS_NONE;
			return anim;
		}
	}
	return 0;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;
	qboolean	didRoll = qfalse;
	const int	runFlags = PM_GetRunFlags();
	const int	moveStyle = PM_GetMovePhysics();

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		pm->ps->inAirAnim = qfalse;
		return;
	}
	t = (-b - sqrtf( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	// decide which landing animation to use
	if (!BG_InRoll(pm->ps, pm->ps->legsAnim) && pm->ps->inAirAnim)
	{ //only play a land animation if we transitioned into an in-air animation while off the ground
		if (!BG_SaberInSpecial(pm->ps->saberMove))
		{
			if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
				PM_ForceLegsAnim( BOTH_LANDBACK1 );
			} else {
				PM_ForceLegsAnim( BOTH_LAND1 );
			}
		}
	}

	if (pm->ps->weapon != WP_SABER)
	{ //saber handles its own anims
		//This will push us back into our weaponready stance from the land anim.
		if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
		{
			PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
		}
		else
		{
			if (pm->ps->weapon == WP_EMPLACED_GUN)
			{
				PM_StartTorsoAnim( BOTH_GUNSIT1 );
			}
			else
			{
				PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
			}
		}
	}

	if (!BG_InSpecialJump(pm->ps->legsAnim, runFlags) ||
		pm->ps->legsTimer < 1 ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_LEFT ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_WALL_RUN_RIGHT)
	{ //Only set the timer if we're in an anim that can be interrupted (this would not be, say, a flip)
		if (!BG_InRoll(pm->ps, pm->ps->legsAnim) && pm->ps->inAirAnim)
		{
			if (!BG_SaberInSpecial(pm->ps->saberMove) || pm->ps->weapon != WP_SABER)
			{
				pm->ps->legsTimer = TIMER_LAND;
			}
		}
	}

	pm->ps->inAirAnim = qfalse;

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) 
	{
		if( delta >= 2 && !PM_InOnGroundAnim( pm->ps->legsAnim ) && !PM_InKnockDown( pm->ps ) && !BG_InRoll(pm->ps, pm->ps->legsAnim) &&
			(pm->ps->forceHandExtend == HANDEXTEND_NONE || jk2gameplay != VERSION_1_04) )
		{//roll!
			int anim = PM_TryRoll();

			if (PM_InRollComplete(pm->ps, pm->ps->legsAnim))
			{
				anim = 0;
				pm->ps->legsTimer = 0;
				pm->ps->legsAnim = 0;
				PM_SetAnim(SETANIM_BOTH,BOTH_LAND1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150);
				pm->ps->legsTimer = TIMER_LAND;
			}

			if ( anim )
			{//absorb some impact
				pm->ps->legsTimer = 0;
				delta /= 3; // /= 2 just cancels out the above delta *= 2 when landing while crouched, the roll itself should absorb a little damage
				pm->ps->legsAnim = 0;
				PM_SetAnim(SETANIM_BOTH,anim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150);
				didRoll = qtrue;
			}
		}
	}

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if (delta > 7)
		{
			int delta_send = (int)delta;

			if (delta_send > 600)
			{ //will never need to know any value above this
				delta_send = 600;
			}

			if (pm->ps->fd.forceJumpZStart)
			{
				if ((int)pm->ps->origin[2] >= (int)pm->ps->fd.forceJumpZStart)
				{ //was force jumping, landed on higher or same level as when force jump was started
					if (delta_send > 8)
					{
						delta_send = 8;
					}
				}
				else
				{
					if (delta_send > 8)
					{
						int dif = ((int)pm->ps->fd.forceJumpZStart - (int)pm->ps->origin[2]);
						int dmgLess = (forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - dif);

						if (dmgLess < 0)
						{
							dmgLess = 0;
						}

						delta_send -= (dmgLess*0.3);

						if (delta_send < 8)
						{
							delta_send = 8;
						}

						//Com_Printf("Damage sub: %i\n", (int)((dmgLess*0.1)));
					}
				}
			}

			if (didRoll)
			{ //Add the appropriate event..
				PM_AddEventWithParm( EV_ROLL, delta_send );
			}
			else
			{
				PM_AddEventWithParm( EV_FALL, delta_send );
			}
		}
		else
		{
			if (didRoll)
			{
				PM_AddEventWithParm( EV_ROLL, 0 );
			}
			else
			{
				PM_AddEvent( PM_FootstepForSurface() );
			}
		}
	}

	// make sure velocity resets so we don't bounce back up again in case we miss the clear elsewhere
	if (!pml.bounceJumped && moveStyle != MV_PINBALL) {
		pm->ps->velocity[2] = 0;
	}

	// nah lets not do this. this isnt a true q3 overbounce, not even close. more of a meme q3 overbounce version.
	// but q3 overbounce is also kinda lame as way too finnicky. lets do sth more fun. bouncy mode maybe
	//if ((MovementIsQuake3Based(moveStyle)) && ((int)pm->ps->fd.forceJumpZStart > pm->ps->origin[2] + 1)) {
	//	if (1 > (sqrtf(pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1])))//No xyvel
	//		pm->ps->velocity[2] = -vel; //OVERBOUNCE OVER BOUNCE
	////}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	//rww - don't want to do this when handextend_choke, because you can be standing on the ground
	//while still holding your throat.
	if ( pm->ps->pm_type == PM_FLOAT && jk2gameplay != VERSION_1_02 )
	{
		//we're assuming this is because you're being choked
		int parts = SETANIM_LEGS;

		//rww - also don't use SETANIM_FLAG_HOLD, it will cause the legs to float around a bit before going into
		//a proper anim even when on the ground.
		PM_SetAnim(parts, BOTH_CHOKE3, SETANIM_FLAG_OVERRIDE, 100);
	}
	//If the anim is choke3, act like we just went into the air because we aren't in a float
	else if ( pm->ps->groundEntityNum != ENTITYNUM_NONE || ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_CHOKE3 && jk2gameplay != VERSION_1_02) )
	{
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 || pm->ps->pm_type == PM_FLOAT ) {
			if ( pm->ps->velocity[2] <= 0 && !(pm->ps->pm_flags&PMF_JUMP_HELD))
			{
				PM_SetAnim(SETANIM_LEGS,BOTH_INAIR1,SETANIM_FLAG_OVERRIDE, 100);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else if ( pm->cmd.forwardmove >= 0 ) 
			{
				PM_SetAnim(SETANIM_LEGS,BOTH_JUMP1,SETANIM_FLAG_OVERRIDE, 100);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} 
			else 
			{
				PM_SetAnim(SETANIM_LEGS,BOTH_JUMPBACK1,SETANIM_FLAG_OVERRIDE, 100);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}

			pm->ps->inAirAnim = qtrue;
		}
	}
	else if (!pm->ps->inAirAnim)
	{
		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 || pm->ps->pm_type == PM_FLOAT )
		{
			pm->ps->inAirAnim = qtrue;
		}
	}

	if (PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{ //Client won't catch an animation restart because it only checks frame against incoming frame, so if you roll when you land after rolling
	  //off of something it won't replay the roll anim unless we switch it off in the air. This fixes that.
		PM_SetAnim(SETANIM_BOTH,BOTH_INAIR1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150);
		pm->ps->inAirAnim = qtrue;
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


extern void PM_LimitedClipVelocity2(vec3_t in, vec3_t normal, vec3_t out, float overbounce, float maxSpeedNormal);
/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;
	const int	moveStyle = PM_GetMovePhysics();
	float		overbounce = MovementOverbounceFactor(moveStyle, pm->ps, &pm->cmd);

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	if (MovementStyleHasQuake2Ramps(moveStyle) && pm->ps->velocity[2] > 180) {
		if (pm->debugLevel) {
			Com_Printf("%i:q2ramp\n", c_pmove);
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	if (pm->ps->pm_type == PM_FLOAT)
	{
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( BOTH_JUMP1 );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( BOTH_JUMPBACK1 );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	if (!pml.bounceJumped && moveStyle != MV_PINBALL) {
		pml.groundPlane = qtrue;
		pml.walking = qtrue;
	}

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}

		// Thanks to Loda for making this fix and Daggo for pointing me to it.
		if ((trace.plane.normal[0] != 0.0f || trace.plane.normal[1] != 0.0f || trace.plane.normal[2] != 1.0f) && !pm->isSpecialPredict)// don't count them during special predict
		{ // It's a ramp!
			if (!pml.clipped)
			{
				// Don't actually fix this atm to not cause prediction errors.

				const int runFlags = PM_GetRunFlags();
				if (runFlags & RFL_NODEADRAMPS) {
					if (moveStyle == MV_PINBALL) {
						//PM_LimitedClipVelocity(pm->ps->velocity, planes[i], pm->ps->velocity, overbounce,100000.0f);
						overbounce -= trace.plane.normal[2] * 0.6f * (MIN(1600.0f, fabsf(pm->ps->velocity[2])) / 1600.0f); // dont let ground and ceiling bounce as as insanely much.
						PM_LimitedClipVelocity2(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, overbounce, 10000.0f);
					}
					else {
						PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, overbounce);
					}
					PM_CheckBounceJump(trace.plane.normal, pm->ps->velocity); // do we need this here? not sure.
				}
				if (pm->debugLevel) {
					Com_Printf("%i:Dead ramp\n", c_pmove);
				}
#if JK2_CGAME
				if (pm->ps->commandTime > cg_rampCountLastCmdTime) { 
					cg_deadRampsCounted++;
				}
#endif
			}
			else {
				if (pm->debugLevel) {
					Com_Printf("%i:Good ramp\n", c_pmove);
				}
#if JK2_CGAME
				if (pm->ps->commandTime > cg_rampCountLastCmdTime) { 
					cg_goodRampsCounted++;
				}
#endif
			}
#if JK2_CGAME
			cg_rampCountLastCmdTime = pm->ps->commandTime;
#endif
		}
		
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	if (!pml.bounceJumped && moveStyle != MV_PINBALL) {
		pm->ps->groundEntityNum = trace.entityNum;
	}
	pm->ps->lastOnGround = pm->cmd.serverTime;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;	
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->ps->usingATST)
	{
		if (pm->cmd.upmove < 0)
		{
			pm->cmd.upmove = 0;
		}
	}

	if (BG_InRoll(pm->ps, pm->ps->legsAnim))
	{
		pm->maxs[2] = CROUCH_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
		pm->ps->pm_flags &= ~PMF_DUCKED;
		pm->ps->pm_flags |= PMF_ROLLING;
		return;
	}
	else if (pm->ps->pm_flags & PMF_ROLLING)
	{
		// try to stand up
		pm->maxs[2] = DEFAULT_MAXS_2;
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
		if (!trace.allsolid)
			pm->ps->pm_flags &= ~PMF_ROLLING;
	}
	else if (pm->cmd.upmove < 0 ||
		pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible 
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = DEFAULT_MAXS_2;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = CROUCH_MAXS_2;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else if (pm->ps->pm_flags & PMF_ROLLING)
	{
		pm->maxs[2] = CROUCH_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = DEFAULT_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}

	if (pm->ps->usingATST)
	{
		pm->mins[0] = ATST_MINS0;
		pm->mins[1] = ATST_MINS1;
		pm->mins[2] = ATST_MINS2;

		pm->maxs[0] = ATST_MAXS0;
		pm->maxs[1] = ATST_MAXS1;
		pm->maxs[2] = ATST_MAXS2;
	}
}



//===================================================================



/*
==============
PM_Use

Generates a use event
==============
*/
#define USE_DELAY 2000

void PM_Use( void ) 
{
	if ( pm->ps->useTime > 0 )
		pm->ps->useTime -= 100;//pm->cmd.msec;

	if ( pm->ps->useTime > 0 ) {
		return;
	}

	if ( ! (pm->cmd.buttons & BUTTON_USE ) )
	{
		pm->useEvent = 0;
		pm->ps->useTime = 0;
		return;
	}

	pm->useEvent = EV_USE;
	pm->ps->useTime = USE_DELAY;
}

qboolean PM_RunningAnim( int anim )
{
	switch ( (anim&~ANIM_TOGGLEBIT) )
	{
	case BOTH_RUN1:			
	case BOTH_RUN2:			
	case BOTH_RUNBACK1:			
	case BOTH_RUNBACK2:			
	case BOTH_RUNAWAY1:			
		return qtrue;
		break;
	}
	return qfalse;
}

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	// qboolean	footstep;
	int			setAnimFlags = 0;

	if ( ((PM_InSaberAnim( (pm->ps->legsAnim&~ANIM_TOGGLEBIT) ) && !BG_SpinningSaberAnim( (pm->ps->legsAnim&~ANIM_TOGGLEBIT) )) 
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_STAND1 
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_STAND1TO2 
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_STAND2TO1 
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_STAND2 
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_SABERFAST_STANCE
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_SABERSLOW_STANCE
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_BUTTON_HOLD
		|| (pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_BUTTON_RELEASE
		|| PM_LandingAnim( (pm->ps->legsAnim&~ANIM_TOGGLEBIT) ) 
		|| PM_PainAnim( (pm->ps->legsAnim&~ANIM_TOGGLEBIT) ))
		&& jk2gameplay != VERSION_1_02 )
	{//legs are in a saber anim, and not spinning, be sure to override it
		setAnimFlags |= SETANIM_FLAG_OVERRIDE;
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrtf( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 )
		{ // MVSDK: Swimming is broken in 1.02, but let's NOT port the brokeness back in here for 1.02-gameplay. Most 1.02 mods apply the 1.04 behaviour anyway.
			if (pm->xyspeed > 60)
			{
				PM_ContinueLegsAnim( BOTH_SWIMFORWARD );
			}
			else
			{
				PM_ContinueLegsAnim( BOTH_SWIM_IDLE1 );
			}
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( (pm->ps->pm_flags & PMF_DUCKED) || (pm->ps->pm_flags & PMF_ROLLING) ) {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_CROUCH1IDLE && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1IDLE, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_CROUCH1IDLE );
				}
			} else {
				if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
				{
					PM_ContinueLegsAnim( TORSO_WEAPONREADY4 );
				}
				else
				{
					if (pm->ps->weapon == WP_SABER && pm->ps->saberHolstered)
					{
						PM_ContinueLegsAnim( BOTH_STAND1 );
					}
					else if ( pm->ps->weapon == WP_SABER && pm->ps->dualBlade )
					{
						PM_ContinueLegsAnim( BOTH_STAND1 );
					}
					else
					{
						PM_ContinueLegsAnim( WeaponReadyAnim[pm->ps->weapon] );
					}
				}
			}
		}
		return;
	}
	

	// footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		int rolled = 0;

		bobmove = 0.5;	// ducked characters bob much faster

		if ( PM_RunningAnim( pm->ps->legsAnim ) && !BG_InRoll(pm->ps, pm->ps->legsAnim) )
		{//roll!
			rolled = PM_TryRoll();
		}
		if ( !rolled )
		{ //if the roll failed or didn't attempt, do standard crouching anim stuff.
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_CROUCH1WALKBACK && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_CROUCH1WALKBACK );
				}
			}
			else {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_CROUCH1WALK && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_CROUCH1WALK );
				}
			}
		}
		else
		{ //otherwise send us into the roll
			pm->ps->legsTimer = 0;
			pm->ps->legsAnim = 0;
			PM_SetAnim(SETANIM_BOTH,rolled,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150);
			PM_AddEventWithParm( EV_ROLL, 0 );
			pm->maxs[2] = CROUCH_MAXS_2;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
			pm->ps->pm_flags &= ~PMF_DUCKED;
			pm->ps->pm_flags |= PMF_ROLLING;
		}
	}
	else if ((pm->ps->pm_flags & PMF_ROLLING) && !BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		!PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		bobmove = 0.5;	// ducked characters bob much faster

		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
		{
			if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_CROUCH1WALKBACK && jk2gameplay != VERSION_1_02)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags, 100);
			}
			else
			{
				PM_ContinueLegsAnim( BOTH_CROUCH1WALKBACK );
			}
		}
		else
		{
			if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_CROUCH1WALK && jk2gameplay != VERSION_1_02)
			{
				PM_SetAnim(SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags, 100);
			}
			else
			{
				PM_ContinueLegsAnim( BOTH_CROUCH1WALK );
			}
		}
	}
	else
	{
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_RUNBACK1 && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_RUNBACK1, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_RUNBACK1 );
				}
			}
			else {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_RUN1 && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_RUN1, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_RUN1 );
				}
			}
			// footstep = qtrue;
		} else {
			bobmove = 0.2f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_WALKBACK1 && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_WALKBACK1, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_WALKBACK1 );
				}
			}
			else {
				if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) != BOTH_WALK1 && jk2gameplay != VERSION_1_02)
				{
					PM_SetAnim(SETANIM_LEGS, BOTH_WALK1, setAnimFlags, 100);
				}
				else
				{
					PM_ContinueLegsAnim( BOTH_WALK1 );
				}
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
	{
		pm->ps->footstepTime = pm->cmd.serverTime + 300;
		if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}
	
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	// turn of any kind of zooming when weapon switching.
	if (pm->ps->zoomMode)
	{
		pm->ps->zoomMode = 0;
		pm->ps->zoomTime = pm->ps->commandTime;
	}

	// If the player still got the rocket launcher locked on a target remove the lock.
	if ( pm->ps->rocketLockIndex != MAX_CLIENTS )
	{
		pm->ps->rocketLockIndex = MAX_CLIENTS;
		pm->ps->rocketLockTime = 0;
		pm->ps->rocketTargetTime = 0;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROPWEAP1 );
}


/*
===============
PM_FinishWeaponChange
===============
*/
void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	if (weapon == WP_SABER)
	{
		PM_SetSaberMove(LS_DRAW);
	}
	else
	{
		PM_StartTorsoAnim( TORSO_RAISEWEAP1);
	}
	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
}



//---------------------------------------
static qboolean PM_DoChargedWeapons( void )
//---------------------------------------
{
	vec3_t		ang;
	trace_t		tr;
	qboolean	charging = qfalse,
				altFire = qfalse;


	if (pm->ps->stats[STAT_RACEMODE])
		return qfalse;

	// If you want your weapon to be a charging weapon, just set this bit up
	switch( pm->ps->weapon )
	{
	//------------------
	case WP_BRYAR_PISTOL:

		// alt-fire charges the weapon
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
		{
			charging = qtrue;
			altFire = qtrue;
		}
		break;
	
	//------------------
	case WP_BOWCASTER:

		// primary fire charges the weapon
		if ( pm->cmd.buttons & BUTTON_ATTACK )
		{
			charging = qtrue;
		}
		break;
	
	//------------------
	case WP_ROCKET_LAUNCHER:

		// Not really a charge weapon, but we still want to delay fire until the button comes up so that we can
		//	implement our alt-fire locking stuff
		if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] >= weaponData[pm->ps->weapon].altEnergyPerShot )
		{
			vec3_t muzzleOffPoint, muzzlePoint, forward, right, up;

			AngleVectors( pm->ps->viewangles, forward, right, up );

			charging = qtrue;
			altFire = qtrue;

			AngleVectors(pm->ps->viewangles, ang, NULL, NULL);

			VectorCopy( pm->ps->origin, muzzlePoint );
			VectorCopy(WP_MuzzlePoint[WP_ROCKET_LAUNCHER], muzzleOffPoint);

			VectorMA(muzzlePoint, muzzleOffPoint[0], forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
			muzzlePoint[2] += pm->ps->viewheight + muzzleOffPoint[2];

			ang[0] = muzzlePoint[0] + ang[0]*2048;
			ang[1] = muzzlePoint[1] + ang[1]*2048;
			ang[2] = muzzlePoint[2] + ang[2]*2048;

			pm->trace(&tr, muzzlePoint, NULL, NULL, ang, pm->ps->clientNum, MASK_PLAYERSOLID);

			if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS && tr.entityNum != pm->ps->clientNum)
			{
				if (pm->ps->rocketLockIndex == MAX_CLIENTS)
				{
					pm->ps->rocketLockIndex = tr.entityNum;
					pm->ps->rocketLockTime = pm->cmd.serverTime;
				}
				else if (pm->ps->rocketLockIndex != tr.entityNum && pm->ps->rocketTargetTime < pm->cmd.serverTime)
				{
					pm->ps->rocketLockIndex = tr.entityNum;
					pm->ps->rocketLockTime = pm->cmd.serverTime;
				}
				else if (pm->ps->rocketLockIndex == tr.entityNum)
				{
					if (pm->ps->rocketLockTime == -1)
					{
						pm->ps->rocketLockTime = pm->ps->rocketLastValidTime;
					}
				}

				if (pm->ps->rocketLockIndex == tr.entityNum)
				{
					pm->ps->rocketTargetTime = pm->cmd.serverTime + 500;
				}
			}
			else if (pm->ps->rocketTargetTime < pm->cmd.serverTime)
			{
				pm->ps->rocketLockIndex = MAX_CLIENTS;
				pm->ps->rocketLockTime = 0;
			}
			else
			{
				if (pm->ps->rocketLockTime != -1)
				{
					pm->ps->rocketLastValidTime = pm->ps->rocketLockTime;
				}
				pm->ps->rocketLockTime = -1;
			}
		}
		break;

	//------------------
	case WP_THERMAL:

		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
		{
			altFire = qtrue; // override default of not being an alt-fire
			charging = qtrue;
		}
		else if ( pm->cmd.buttons & BUTTON_ATTACK )
		{
			charging = qtrue;
		}
		break;

	case WP_DEMP2:
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
		{
			altFire = qtrue; // override default of not being an alt-fire
			charging = qtrue;
		}
		break;

	case WP_DISRUPTOR:
		if ((pm->cmd.buttons & BUTTON_ATTACK) &&
			pm->ps->zoomMode == 1 &&
			pm->ps->zoomLocked)
		{
			charging = qtrue;
			altFire = qtrue;
		}

		if (pm->ps->zoomMode != 1 &&
			pm->ps->weaponstate == WEAPON_CHARGING_ALT)
		{
			pm->ps->weaponstate = WEAPON_READY;
			charging = qfalse;
			altFire = qfalse;
		}

	} // end switch


	// set up the appropriate weapon state based on the button that's down.  
	//	Note that we ALWAYS return if charging is set ( meaning the buttons are still down )
	if ( charging )
	{
		if ( altFire )
		{
			if ( pm->ps->weaponstate != WEAPON_CHARGING_ALT )
			{
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;

#ifdef _DEBUG
				Com_Printf("Starting charge\n");
#endif
				assert(pm->ps->weapon > WP_NONE);
				BG_AddPredictableEventToPlayerstate(EV_WEAPON_CHARGE_ALT, pm->ps->weapon, pm->ps);
			}

			if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < (weaponData[pm->ps->weapon].altChargeSub+weaponData[pm->ps->weapon].altEnergyPerShot))
			{
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;

				goto rest;
			}
			else if ((pm->cmd.serverTime - pm->ps->weaponChargeTime) < weaponData[pm->ps->weapon].altMaxCharge)
			{
				if (pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime)
				{
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].altChargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;
				}
			}
		}
		else
		{
			if ( pm->ps->weaponstate != WEAPON_CHARGING )
			{
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;

#ifdef _DEBUG
				Com_Printf("Starting charge\n");
#endif
				BG_AddPredictableEventToPlayerstate(EV_WEAPON_CHARGE, pm->ps->weapon, pm->ps);
			}

			if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < (weaponData[pm->ps->weapon].chargeSub+weaponData[pm->ps->weapon].energyPerShot))
			{
				pm->ps->weaponstate = WEAPON_CHARGING;

				goto rest;
			}
			else if ((pm->cmd.serverTime - pm->ps->weaponChargeTime) < weaponData[pm->ps->weapon].maxCharge)
			{
				if (pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime)
				{
					pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].chargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;
				}
			}
		}

		return qtrue; // short-circuit rest of weapon code
	}
rest:
	// Only charging weapons should be able to set these states...so....
	//	let's see which fire mode we need to set up now that the buttons are up
	if ( pm->ps->weaponstate == WEAPON_CHARGING )
	{
		// weapon has a charge, so let us do an attack
#ifdef _DEBUG
		Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif

		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ATTACK;
		pm->ps->eFlags |= EF_FIRING;
	}
	else if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT )
	{
		// weapon has a charge, so let us do an alt-attack
#ifdef _DEBUG
		Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif

		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ALT_ATTACK;
		pm->ps->eFlags |= (EF_FIRING|EF_ALT_FIRING);
	}

	return qfalse; // continue with the rest of the weapon code
}


#define BOWCASTER_CHARGE_UNIT	200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in g_weapon
#define BRYAR_CHARGE_UNIT		200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in g_weapon

int PM_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->usingATST)
	{
		return 0;
	}

	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (ps->duelInProgress && jk2gameplay == VERSION_1_04)
	{ //not allowed to use holdables while in a private duel.
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}
		if (ps->stats[STAT_HEALTH] <= 0 ||
			(ps->eFlags & EF_DEAD))
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		pm->trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			PM_AddEventWithParm(EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		pm->trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			pm->trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		PM_AddEventWithParm(EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	default:
		return 1;
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void )
{
	int		addTime;
	int amount;
	int		killAfterItem = 0;
	const int	runFlags = PM_GetRunFlags();

	if (pm->ps->usingATST)
	{
		if ( pm->ps->weaponTime > 0 )
		{
			pm->ps->weaponTime -= pml.msec;
		}

		if (pm->ps->weaponTime < 1 && (pm->cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)))
		{
			pm->ps->weaponTime += 500;

			if (pm->ps->atstAltFire)
			{
				PM_AddEvent( EV_ALT_FIRE );
				pm->ps->atstAltFire = qfalse;
			}
			else
			{
				PM_AddEvent( EV_FIRE_WEAPON );
				pm->ps->atstAltFire = qtrue;
			}
		}

		return;
	}

	if (pm->ps->weapon != WP_DISRUPTOR && pm->ps->weapon != WP_ROCKET_LAUNCHER)
	{ //check for exceeding max charge time if not using disruptor or rocket launcher
		if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT )
		{
			int timeDif = (pm->cmd.serverTime - pm->ps->weaponChargeTime);

			if (timeDif > MAX_WEAPON_CHARGE_TIME)
			{
				pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			}
		}

		if ( pm->ps->weaponstate == WEAPON_CHARGING )
		{
			int timeDif = (pm->cmd.serverTime - pm->ps->weaponChargeTime);

			if (timeDif > MAX_WEAPON_CHARGE_TIME)
			{
				pm->cmd.buttons &= ~BUTTON_ATTACK;
			}
		}
	}

	if (pm->ps->forceHandExtend == HANDEXTEND_WEAPONREADY)
	{ //reset into weapon stance
		if (pm->ps->weapon != WP_SABER)
		{ //saber handles its own anims
			if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
			{
				//PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
				PM_StartTorsoAnim( TORSO_RAISEWEAP1);
			}
			else
			{
				if (pm->ps->weapon == WP_EMPLACED_GUN)
				{
					PM_StartTorsoAnim( BOTH_GUNSIT1 );
				}
				else
				{
					//PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
					PM_StartTorsoAnim( TORSO_RAISEWEAP1);
				}
			}
		}

		//we now go into a weapon raise anim after every force hand extend.
		//this is so that my holster-view-weapon-when-hand-extend stuff works.
		pm->ps->weaponstate = WEAPON_RAISING;
		pm->ps->weaponTime += 250;

		pm->ps->forceHandExtend = HANDEXTEND_NONE;
	}
	else if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
	{ //nothing else should be allowed to happen during this time, including weapon fire
		int desiredAnim = 0;
		qboolean seperateOnTorso = qfalse;
		int desiredOnTorso = 0;

		switch(pm->ps->forceHandExtend)
		{
		case HANDEXTEND_FORCEPUSH:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		case HANDEXTEND_FORCEPULL:
			desiredAnim = BOTH_FORCEPULL;
			break;
		case HANDEXTEND_FORCEGRIP:
			desiredAnim = BOTH_FORCEGRIP_HOLD;
			break;
		case HANDEXTEND_SABERPULL:
			desiredAnim = BOTH_SABERPULL;
			break;
		case HANDEXTEND_CHOKE:
			desiredAnim = BOTH_CHOKE3; //left-handed choke
			break;
		case HANDEXTEND_DODGE:
			desiredAnim = pm->ps->forceDodgeAnim;
			break;
		case HANDEXTEND_KNOCKDOWN:
			if (pm->ps->forceDodgeAnim)
			{
				if (pm->ps->forceDodgeAnim > 4 && jk2gameplay == VERSION_1_04) // MVSDK: This should be enough to have the "seperateOnTorso" behaviour only with 1.04 gameplay
				{ //this means that we want to play a sepereate anim on the torso
					int originalDAnim = pm->ps->forceDodgeAnim-8; //-8 is the original legs anim
					if (originalDAnim == 2)
					{
						desiredAnim = BOTH_FORCE_GETUP_B1;
					}
					else if (originalDAnim == 3)
					{
						desiredAnim = BOTH_FORCE_GETUP_B3;
					}
					else
					{
						desiredAnim = BOTH_GETUP1;
					}

					//now specify the torso anim
					seperateOnTorso = qtrue;
					desiredOnTorso = BOTH_FORCEPUSH;
				}
				else if (pm->ps->forceDodgeAnim == 2)
				{
					desiredAnim = BOTH_FORCE_GETUP_B1;
				}
				else if (pm->ps->forceDodgeAnim == 3)
				{
					desiredAnim = BOTH_FORCE_GETUP_B3;
				}
				else
				{
					desiredAnim = BOTH_GETUP1;
				}
			}
			else
			{
				desiredAnim = BOTH_KNOCKDOWN1;
			}
			break;
		case HANDEXTEND_DUELCHALLENGE:
			desiredAnim = BOTH_ENGAGETAUNT;
			break;
		case HANDEXTEND_TAUNT:
			desiredAnim = pm->ps->forceDodgeAnim;
			break;
			//Hmm... maybe use these, too?
			//BOTH_FORCEHEAL_QUICK //quick heal (SP level 2 & 3)
			//BOTH_MINDTRICK1 // wave (maybe for mind trick 2 & 3 - whole area, and for force seeing)
			//BOTH_MINDTRICK2 // tap (maybe for mind trick 1 - one person)
			//BOTH_FORCEGRIP_START //start grip
			//BOTH_FORCEGRIP_HOLD //hold grip
			//BOTH_FORCEGRIP_RELEASE //release grip
			//BOTH_FORCELIGHTNING //quick lightning burst (level 1)
			//BOTH_FORCELIGHTNING_START //start lightning
			//BOTH_FORCELIGHTNING_HOLD //hold lightning
			//BOTH_FORCELIGHTNING_RELEASE //release lightning
		default:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		}

		if (!seperateOnTorso)
		{ //of seperateOnTorso, handle it after setting the legs
			PM_SetAnim(SETANIM_TORSO, desiredAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
			pm->ps->torsoTimer = 1;
		}

		if (pm->ps->forceHandExtend == HANDEXTEND_DODGE || pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			((pm->ps->forceHandExtend == HANDEXTEND_CHOKE && pm->ps->groundEntityNum == ENTITYNUM_NONE) && jk2gameplay != VERSION_1_02) )
		{ //special case, play dodge anim on whole body, choke anim too if off ground
			if (seperateOnTorso)
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
				pm->ps->legsTimer = 1;

				PM_SetAnim(SETANIM_TORSO, desiredOnTorso, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
				pm->ps->torsoTimer = 1;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
				pm->ps->legsTimer = 1;
			}
		}

		return;
	}

	if (BG_InSpecialJump(pm->ps->legsAnim, runFlags) ||
		BG_InRoll(pm->ps, pm->ps->legsAnim) ||
		PM_InRollComplete(pm->ps, pm->ps->legsAnim))
	{
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;
	}

	if (pm->ps->duelInProgress)
	{
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;

		if (pm->ps->duelTime >= pm->cmd.serverTime)
		{
			pm->cmd.upmove = 0;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
	}

	if (pm->ps->weapon == WP_SABER && pm->ps->saberMove != LS_READY && pm->ps->saberMove != LS_NONE)
	{
		pm->cmd.weapon = WP_SABER; //don't allow switching out mid-attack
	}

	if (pm->ps->weapon == WP_SABER)
	{
		//rww - we still need the item stuff, so we won't return immediately
		PM_WeaponLightsaber();
		killAfterItem = 1;
	}
	else
	{
		pm->ps->saberHolstered = qfalse;
	}

	if (pm->ps->weapon == WP_THERMAL ||
		pm->ps->weapon == WP_TRIP_MINE ||
		pm->ps->weapon == WP_DET_PACK)
	{
		if (pm->ps->weapon == WP_THERMAL)
		{
			if ((pm->ps->torsoAnim&~ANIM_TOGGLEBIT) == WeaponAttackAnim[pm->ps->weapon] &&
				(pm->ps->weaponTime-200) <= 0)
			{
				PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
			}
		}
		else
		{
			if ((pm->ps->torsoAnim&~ANIM_TOGGLEBIT) == WeaponAttackAnim[pm->ps->weapon] &&
				(pm->ps->weaponTime-700) <= 0)
			{
				PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
			}
		}
	}

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {

			if (!pm->ps->stats[STAT_HOLDABLE_ITEM])
			{
				return;
			}

			if (!PM_ItemUsable(pm->ps, 0))
			{
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				return;
			}
			else
			{
				if (pm->ps->stats[STAT_HOLDABLE_ITEMS] & (1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag))
				{
					if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS)
					{ //never use up the binoculars
						pm->ps->stats[STAT_HOLDABLE_ITEMS] -= (1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag);
					}
				}
				else
				{
					return; //this should not happen...
				}

				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );

				if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS)
				{
					pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
					BG_CycleInven(pm->ps, 1);
				}
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}

	if (pm->ps->weapon == WP_SABER)
	{ //we can't toggle zoom while using saber (for obvious reasons) so make sure it's always off
		pm->ps->zoomMode = 0;
		pm->ps->zoomFov = 0;
		pm->ps->zoomLocked = qfalse;
		pm->ps->zoomLockTime = 0;
	}

	if (killAfterItem)
	{
		return;
	}

	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

	if (pm->ps->isJediMaster && pm->ps->emplacedIndex)
	{
		pm->ps->emplacedIndex = 0;
	}

	if (pm->ps->duelInProgress && pm->ps->emplacedIndex)
	{
		pm->ps->emplacedIndex = 0;
	}

	if (pm->ps->weapon == WP_EMPLACED_GUN && pm->ps->emplacedIndex)
	{
		pm->cmd.weapon = WP_EMPLACED_GUN; //No switch for you!
		PM_StartTorsoAnim( BOTH_GUNSIT1 );
	}

	if (pm->ps->isJediMaster || pm->ps->duelInProgress || pm->ps->trueJedi)
	{
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;

		if (pm->ps->isJediMaster || pm->ps->trueJedi)
		{
			pm->ps->stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
	}

	amount = weaponData[pm->ps->weapon].energyPerShot;

	// take an ammo away if not infinite
	if ( pm->ps->weapon != WP_NONE &&
		pm->ps->weapon == pm->cmd.weapon &&
		(pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING) )
	{
		if ( pm->ps->ammo[ weaponData[pm->ps->weapon].ammoIndex ] != -1 )
		{
			// enough energy to fire this weapon?
			if (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].energyPerShot &&
				pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].altEnergyPerShot) 
			{ //the weapon is out of ammo essentially because it cannot fire primary or secondary, so do the switch
			  //regardless of if the player is attacking or not
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS+pm->ps->weapon );

				if (pm->ps->weaponTime < 500)
				{
					pm->ps->weaponTime += 500;
				}
				return;
			}

			if (pm->ps->weapon == WP_DET_PACK && !pm->ps->hasDetPackPlanted && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < 1)
			{
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS+pm->ps->weapon );

				if (pm->ps->weaponTime < 500)
				{
					pm->ps->weaponTime += 500;
				}
				return;
			}
		}
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_SABER ) {
			PM_StartTorsoAnim( PM_GetSaberStance() );
		} else {
			if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
			{
				PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
			}
			else
			{
				if (pm->ps->weapon == WP_EMPLACED_GUN)
				{
					PM_StartTorsoAnim( BOTH_GUNSIT1 );
				}
				else
				{
					PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
				}
			}
		}
		return;
	}

	if (((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == TORSO_WEAPONREADY4 ||
		(pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == BOTH_ATTACK4) &&
		(pm->ps->weapon != WP_DISRUPTOR || pm->ps->zoomMode != 1))
	{
		if (pm->ps->weapon == WP_EMPLACED_GUN)
		{
			PM_StartTorsoAnim( BOTH_GUNSIT1 );
		}
		else
		{
			PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
		}
	}
	else if (((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_WEAPONREADY4 &&
		(pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) != BOTH_ATTACK4) &&
		(pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1))
	{
		PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
	}


	if (pm->ps->weapon != WP_ROCKET_LAUNCHER)
	{
		pm->ps->rocketLockIndex = MAX_CLIENTS;
		pm->ps->rocketLockTime = 0;
		pm->ps->rocketTargetTime = 0;
	}

	if ( PM_DoChargedWeapons())
	{
		// In some cases the charged weapon code may want us to short circuit the rest of the firing code
		return;
	}

	// check for fire
	if ( ! (pm->cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK))) 
	{
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if (pm->ps->weapon == WP_EMPLACED_GUN)
	{
		addTime = weaponData[pm->ps->weapon].fireTime;
		pm->ps->weaponTime += addTime;
		PM_AddEvent( EV_FIRE_WEAPON );
		return;
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		(pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
		!pm->ps->zoomLocked)
	{
		return;
	}

	if (pm->ps->weapon == WP_DISRUPTOR &&
		(pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
		pm->ps->zoomMode == 2)
	{ //can't use disruptor secondary while zoomed binoculars
		return;
	}

	if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1)
	{
		PM_StartTorsoAnim( BOTH_ATTACK4 );
	}
	/*else if ((runFlags & RFL_CLIMBTECH) && pm->ps->weapon == WP_MELEE)// uh is this even actually climbtech?
	{ //special anims for standard melee attacks
		//Alternate between punches and use the anim length as weapon time.
		if (!pm->ps->m_iVehicleNum)
		{ //if riding a vehicle don't do this stuff at all
			if (pm->debugMelee &&
				(pm->cmd.buttons & BUTTON_ATTACK) &&
				(pm->cmd.buttons & BUTTON_ALT_ATTACK))
			{ //ok, grapple time
#if 0 //eh, I want to try turning the saber off, but can't do that reliably for prediction..
				qboolean icandoit = qtrue;
				if (pm->ps->weaponTime > 0)
				{ //weapon busy
					icandoit = qfalse;
				}
				if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
				{ //force power or knockdown or something
					icandoit = qfalse;
				}
				if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE)
				{
					icandoit = qfalse;
				}

				if (icandoit)
				{
					//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
					PM_SetAnim(SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (pm->ps->torsoAnim == BOTH_KYLE_GRAB)
					{ //providing the anim set succeeded..
						pm->ps->torsoTimer += 500; //make the hand stick out a little longer than it normally would
						if (pm->ps->legsAnim == pm->ps->torsoAnim)
						{
							pm->ps->legsTimer = pm->ps->torsoTimer;
						}
						pm->ps->weaponTime = pm->ps->torsoTimer;
						return;
					}
				}
#else
#ifdef _GAME
				if (pm_entSelf)
				{
					if (TryGrapple((gentity_t*)pm_entSelf))
					{
						return;
					}
				}
#else
				return;
#endif
#endif
			}
			else if (pm->debugMelee && 
				(pm->cmd.buttons & BUTTON_ALT_ATTACK))
			{ //kicks
				if (!BG_KickingAnim(pm->ps->torsoAnim) &&
					!BG_KickingAnim(pm->ps->legsAnim))
				{
					int kickMove = PM_KickMoveForConditions();
					if (kickMove == LS_HILT_BASH)
					{ //yeah.. no hilt to bash with!
						kickMove = LS_KICK_F;
					}

					if (kickMove != -1)
					{
						if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
						{//if in air, convert kick to an in-air kick
							float gDist = PM_GroundDistance();
							//let's only allow air kicks if a certain distance from the ground
							//it's silly to be able to do them right as you land.
							//also looks wrong to transition from a non-complete flip anim...
							if ((!BG_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
								gDist > 64.0f && //strict minimum
								gDist > (-pm->ps->velocity[2]) - 64.0f //make sure we are high to ground relative to downward velocity as well
								)
							{
								switch (kickMove)
								{
								case LS_KICK_F:
									kickMove = LS_KICK_F_AIR;
									break;
								case LS_KICK_B:
									kickMove = LS_KICK_B_AIR;
									break;
								case LS_KICK_R:
									kickMove = LS_KICK_R_AIR;
									break;
								case LS_KICK_L:
									kickMove = LS_KICK_L_AIR;
									break;
								default: //oh well, can't do any other kick move while in-air
									kickMove = -1;
									break;
								}
							}
							else
							{ //off ground, but too close to ground
								kickMove = -1;
							}
						}
					}

					if (kickMove != -1)
					{
						int kickAnim = saberMoveData[kickMove].animToUse;

						if (kickAnim != -1)
						{
							PM_SetAnim(SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							if (pm->ps->legsAnim == kickAnim)
							{
								pm->ps->weaponTime = pm->ps->legsTimer;
								return;
							}
						}
					}
				}

				//if got here then no move to do so put torso into leg idle or whatever
				if (pm->ps->torsoAnim != pm->ps->legsAnim)
				{
					PM_SetAnim(SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				pm->ps->weaponTime = 0;
				return;
			}
			else
			{ //just punch
				int desTAnim = BOTH_MELEE1;
				if (pm->ps->torsoAnim == BOTH_MELEE1)
				{
					desTAnim = BOTH_MELEE2;
				}
				PM_StartTorsoAnim(desTAnim);

				if (pm->ps->torsoAnim == desTAnim)
				{
					pm->ps->weaponTime = pm->ps->torsoTimer;
				}
			}
		}
	}*/
	else
	{
		PM_StartTorsoAnim( WeaponAttackAnim[pm->ps->weapon] );
	}

	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
	{
		amount = weaponData[pm->ps->weapon].altEnergyPerShot;
	}
	else
	{
		amount = weaponData[pm->ps->weapon].energyPerShot;
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ weaponData[pm->ps->weapon].ammoIndex ] != -1 )
	{
		// enough energy to fire this weapon?
		if ((pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] - amount) >= 0) 
		{
			pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= amount;
		}
		else	// Not enough energy
		{
			// Switch weapons
			if (pm->ps->weapon != WP_DET_PACK || !pm->ps->hasDetPackPlanted)
			{
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS+pm->ps->weapon );
				if (pm->ps->weaponTime < 500)
				{
					pm->ps->weaponTime += 500;
				}
			}
			return;
		}
	}

	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) 	{
		if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode != 1)
		{
			PM_AddEvent( EV_FIRE_WEAPON );
			addTime = weaponData[pm->ps->weapon].fireTime;
		}
		else
		{
			PM_AddEvent( EV_ALT_FIRE );
			addTime = weaponData[pm->ps->weapon].altFireTime;
		}
	}
	else {
		PM_AddEvent( EV_FIRE_WEAPON );
		addTime = weaponData[pm->ps->weapon].fireTime;
	}

	if ( pm->ps->powerups[PW_HASTE] ) {
		addTime /= 1.3;
	}

	if (pm->ps->fd.forcePowersActive & (1 << FP_RAGE))
	{
		addTime *= 0.75;
	}
	else if (pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime)
	{
		addTime *= 1.5;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer < 1 && pm->ps->forceHandExtend == HANDEXTEND_NONE &&
			pm->ps->legsTimer < 1 && pm->ps->weaponTime < 1 && (pm->ps->saberLockTime < pm->cmd.serverTime || jk2gameplay == VERSION_1_02)) {

			pm->ps->forceHandExtend = HANDEXTEND_TAUNT;

			//FIXME: random taunt anims?
			pm->ps->forceDodgeAnim = BOTH_ENGAGETAUNT;

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;
			
			pm->ps->weaponTime = 100;

			PM_AddEvent( EV_TAUNT );
		}
#if 0
// Here's an interesting bit.  The bots in TA used buttons to do additional gestures.
// I ripped them out because I didn't want too many buttons given the fact that I was already adding some for JK2.
// We can always add some back in if we want though.
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
#endif //
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	const int moveStyle = PM_GetMovePhysics();

	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}

	// handle bounce power
	if (moveStyle == MV_BOUNCE) {
		int bouncePower = pm->ps->stats[STAT_BOUNCEPOWER] & BOUNCEPOWER_POWERMASK;
		int bounceRegenTimer = (pm->ps->stats[STAT_BOUNCEPOWER] & BOUNCEPOWER_REGENMASK) >> 9;
		if (pm->cmd.buttons & BUTTON_BOUNCEPOWER) {
			// using bounce power. decrease it.
			bouncePower -= pml.msec;
			bounceRegenTimer = BOUNCEPOWER_REGEN_MAX;
		}
		else {
			bounceRegenTimer -= pml.msec;
			if (bounceRegenTimer <= 0) {
				bouncePower += 10;
				bounceRegenTimer = BOUNCEPOWER_REGEN_MAX;
			}
		}
		bouncePower = MAX(0,MIN(BOUNCEPOWER_MAX,bouncePower));
		bounceRegenTimer = MAX(0,MIN(BOUNCEPOWER_REGEN_MAX, bounceRegenTimer));
		pm->ps->stats[STAT_BOUNCEPOWER] = (bouncePower & BOUNCEPOWER_POWERMASK) | ((bounceRegenTimer << 9) & BOUNCEPOWER_REGENMASK);
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

}

//-------------------------------------------
void PM_AdjustAttackStates( pmove_t *pm )
//-------------------------------------------
{
	int amount;

	// get ammo usage
	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
	{
		amount = pm->ps->ammo[weaponData[ pm->ps->weapon ].ammoIndex] - weaponData[pm->ps->weapon].altEnergyPerShot;
	}
	else
	{
		amount = pm->ps->ammo[weaponData[ pm->ps->weapon ].ammoIndex] - weaponData[pm->ps->weapon].energyPerShot;
	}

	// disruptor alt-fire should toggle the zoom mode, but only bother doing this for the player?
	if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->weaponstate == WEAPON_READY )
	{
		if ( !(pm->ps->eFlags & EF_ALT_FIRING) && (pm->cmd.buttons & BUTTON_ALT_ATTACK) /*&&
			pm->cmd.upmove <= 0 && !pm->cmd.forwardmove && !pm->cmd.rightmove*/)
		{
			// We just pressed the alt-fire key
			if ( !pm->ps->zoomMode && !pm->ps->stats[STAT_RACEMODE])
			{
				// not already zooming, so do it now
				pm->ps->zoomMode = 1;
				pm->ps->zoomLocked = qfalse;
				pm->ps->zoomFov = 80.0f;//cg_fov.value;
				pm->ps->zoomLockTime = pm->cmd.serverTime + 50;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
			else if (pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
			{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
				// already zooming, so must be wanting to turn it off
				pm->ps->zoomMode = 0;
				pm->ps->zoomTime = pm->ps->commandTime;
				pm->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
		}
		else if ( !(pm->cmd.buttons & BUTTON_ALT_ATTACK ) && pm->ps->zoomLockTime < pm->cmd.serverTime)
		{
			// Not pressing zoom any more
			if ( pm->ps->zoomMode )
			{
				if (pm->ps->zoomMode == 1 && !pm->ps->zoomLocked)
				{ //approximate what level the client should be zoomed at based on how long zoom was held
					pm->ps->zoomFov = ((pm->cmd.serverTime+50) - pm->ps->zoomLockTime) * 0.035f;
					if (pm->ps->zoomFov > 50)
					{
						pm->ps->zoomFov = 50;
					}
					if (pm->ps->zoomFov < 1)
					{
						pm->ps->zoomFov = 1;
					}
				}
				// were zooming in, so now lock the zoom
				pm->ps->zoomLocked = qtrue;
			}
		}
		//This seemed like a good idea, but apparently it confuses people. So disabled for now.
		/*
		else if (!(pm->ps->eFlags & EF_ALT_FIRING) && (pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
			(pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove))
		{ //if you try to zoom while moving, just convert it into a primary attack
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
		*/

		if (pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove)
		{
			if (pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
			{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
				pm->ps->zoomMode = 0;
				pm->ps->zoomTime = pm->ps->commandTime;
				pm->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
		}

		if ( pm->cmd.buttons & BUTTON_ATTACK )
		{
			// If we are zoomed, we should switch the ammo usage to the alt-fire, otherwise, we'll
			//	just use whatever ammo was selected from above
			if ( pm->ps->zoomMode )
			{
				amount = pm->ps->ammo[weaponData[ pm->ps->weapon ].ammoIndex] - 
							weaponData[pm->ps->weapon].altEnergyPerShot;
			}
		}
		else
		{
			// alt-fire button pressing doesn't use any ammo
			amount = 0;
		}
	}
	else if (pm->ps->weapon == WP_DISRUPTOR) //still perform certain checks, even if the weapon is not ready
	{
		if (pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove)
		{
			if (pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
			{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
				pm->ps->zoomMode = 0;
				pm->ps->zoomTime = pm->ps->commandTime;
				pm->ps->zoomLocked = qfalse;
				PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
			}
		}
	}

	// set the firing flag for continuous beam weapons, saber will fire even if out of ammo
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && 
			pm->ps->pm_type != PM_INTERMISSION && 
			( pm->cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)) && 
			( amount >= 0 || pm->ps->weapon == WP_SABER ))
	{
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
		{
			pm->ps->eFlags |= EF_ALT_FIRING;
		}
		else
		{
			pm->ps->eFlags &= ~EF_ALT_FIRING;
		}

		// This flag should always get set, even when alt-firing
		pm->ps->eFlags |= EF_FIRING;
	} 
	else 
	{
		// Clear 'em out
		pm->ps->eFlags &= ~(EF_FIRING|EF_ALT_FIRING);
	}

	// disruptor should convert a main fire to an alt-fire if the gun is currently zoomed
	if ( pm->ps->weapon == WP_DISRUPTOR)
	{
		if ( pm->cmd.buttons & BUTTON_ATTACK && pm->ps->zoomMode == 1 && pm->ps->zoomLocked)
		{
			// converting the main fire to an alt-fire
			pm->cmd.buttons |= BUTTON_ALT_ATTACK;
			pm->ps->eFlags |= EF_ALT_FIRING;
		}
		else if ( pm->cmd.buttons & BUTTON_ALT_ATTACK && pm->ps->zoomMode == 1 && pm->ps->zoomLocked)
		{
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->ps->eFlags &= ~EF_ALT_FIRING;
		}
	}
}

void BG_CmdForRoll( int anim, usercmd_t *pCmd )
{
	switch ( (anim&~ANIM_TOGGLEBIT) )
	{
	case BOTH_ROLL_F:
		pCmd->forwardmove = 127;
		pCmd->rightmove = 0;
		break;
	case BOTH_ROLL_B:
		pCmd->forwardmove = -127;
		pCmd->rightmove = 0;
		break;
	case BOTH_ROLL_R:
		pCmd->forwardmove = 0;
		pCmd->rightmove = 127;
		break;
	case BOTH_ROLL_L:
		pCmd->forwardmove = 0;
		pCmd->rightmove = -127;
		break;
	}
	pCmd->upmove = 0;
}

qboolean PM_SaberInTransition( int move );

void BG_AdjustClientSpeed(playerState_t *ps, usercmd_t *cmd, int svTime)
{
	//For prediction, always reset speed back to the last known server base speed
	//If we didn't do this, under lag we'd eventually dwindle speed down to 0 even though
	//that would not be the correct predicted value.
	ps->speed = ps->basespeed;

	if (ps->forceHandExtend == HANDEXTEND_DODGE)
	{
		ps->speed = 0;
	}

	if (ps->forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		ps->speed = 0;
	}

	if (ps->usingATST && (cmd->rightmove ||
		cmd->forwardmove))
	{
		if (!ps->holdMoveTime)
		{
			ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
				| BOTH_RUN1START;
			ps->holdMoveTime = svTime;
		}
	}
	else
	{
		ps->holdMoveTime = 0;

		if (ps->usingATST)
		{
			ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
				| BOTH_STAND1;
		}
	}

	if (ps->usingATST &&
		((svTime - ps->holdMoveTime) < 500 ||
		!ps->holdMoveTime))
	{
		ps->speed = 0;
	}
	else if (ps->usingATST)
	{
		if ((svTime - ps->holdMoveTime) < 600)
		{
			ps->speed *= 0.4;
		}
		else if ((svTime - ps->holdMoveTime) < 1000)
		{
			ps->speed *= 0.5;
		}
		else if ((svTime - ps->holdMoveTime) < 1400)
		{
			ps->speed *= 0.6;
		}
		else if ((svTime - ps->holdMoveTime) < 1700)
		{
			ps->speed *= 0.7;
		}
		else if ((svTime - ps->holdMoveTime) < 1900)
		{
			ps->speed *= 0.8;
		}

		if (cmd->forwardmove < 0)
		{
			ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
				| BOTH_WALKBACK1;
			ps->speed *= 0.6;
		}
		else
		{
			ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
				| BOTH_RUN1;
		}
	}
	else if ( cmd->forwardmove < 0 && !(cmd->buttons&BUTTON_WALKING) && pm->ps->groundEntityNum != ENTITYNUM_NONE && jk2gameplay == VERSION_1_04 )
	{//running backwards is slower than running forwards (like SP)
		ps->speed *= 0.75;
	}

	if (ps->fd.forcePowersActive & (1 << FP_GRIP) && jk2gameplay != VERSION_1_02)
	{
		ps->speed *= 0.4;
	}

	if (ps->fd.forcePowersActive & (1 << FP_SPEED))
	{
		if (ps->fd.forceSpeedSmash < 1.2)
		{
			ps->fd.forceSpeedSmash = 1.2f;
		}
		if (ps->fd.forceSpeedSmash > forceSpeedLevels[ps->fd.forcePowerLevel[FP_SPEED]]) //2.8
		{
			ps->fd.forceSpeedSmash = forceSpeedLevels[ps->fd.forcePowerLevel[FP_SPEED]];
		}
		ps->speed *= ps->fd.forceSpeedSmash;
		ps->fd.forceSpeedSmash += 0.005f;
	}

	if (ps->fd.forcePowersActive & (1 << FP_RAGE))
	{
		ps->speed *= 1.3;
	}
	else if (ps->fd.forceRageRecoveryTime > svTime)
	{
		ps->speed *= 0.75;
	}

	if (ps->fd.forceGripCripple)
	{
		if (ps->fd.forcePowersActive & (1 << FP_RAGE))
		{
			ps->speed *= 0.9;
		}
		else if (ps->fd.forcePowersActive & (1 << FP_SPEED))
		{ //force speed will help us escape
			ps->speed *= 0.8;
		}
		else
		{
			ps->speed *= 0.2;
		}
	}

	if ( BG_SaberInAttack( ps->saberMove ) && cmd->forwardmove < 0 )
	{//if running backwards while attacking, don't run as fast.
		switch( ps->fd.saberAnimLevel )
		{
		case FORCE_LEVEL_1:
			ps->speed *= 0.75f;
			break;
		case FORCE_LEVEL_2:
			ps->speed *= 0.60f;
			break;
		case FORCE_LEVEL_3:
			ps->speed *= 0.45f;
			break;
		default:
			break;
		}
	}
	else if ( BG_SpinningSaberAnim( ps->legsAnim ) )
	{
		if (ps->fd.saberAnimLevel == FORCE_LEVEL_3 && jk2gameplay != VERSION_1_02)
		{
			ps->speed *= 0.3f;
		}
		else
		{
			ps->speed *= 0.5f;
		}
	}
	else if ( ps->weapon == WP_SABER && BG_SaberInAttack( ps->saberMove ) )
	{//if attacking with saber while running, drop your speed
		switch( ps->fd.saberAnimLevel )
		{
		case FORCE_LEVEL_2:
			ps->speed *= 0.85f;
			break;
		case FORCE_LEVEL_3:
			ps->speed *= (jk2gameplay == VERSION_1_02 ? 0.70f : 0.55f);
			break;
		default:
			break;
		}
	}
	else if (ps->weapon == WP_SABER && ps->fd.saberAnimLevel == FORCE_LEVEL_3 &&
		PM_SaberInTransition(ps->saberMove) && jk2gameplay != VERSION_1_02)
	{ //Now, we want to even slow down in transitions for level 3 (since it has chains and stuff now)
		if (cmd->forwardmove < 0)
		{
			ps->speed *= 0.4f;
		}
		else
		{
			ps->speed *= 0.6f;
		}
	}


	if ( BG_InRoll( ps, ps->legsAnim ) && ps->speed > 200 )
	{ //can't roll unless you're able to move normally
		BG_CmdForRoll( ps->legsAnim, cmd );
		if ((ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_ROLL_B)
		{ //backwards roll is pretty fast, should also be slower
			ps->speed = ps->legsTimer/2.5;
		}
		else
		{
			ps->speed = ps->legsTimer/1.5;//450;
		}
		if (ps->speed > 600)
		{
			ps->speed = 600;
		}
		//Automatically slow down as the roll ends.
	}
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector( float *v );

void PmoveSingle (pmove_t *pmove) {
	int runFlags;
	int msecRestrict;
	pm = pmove;
	runFlags = PM_GetRunFlags();
	msecRestrict = PM_GetMsecRestrict();

	gPMDoSlowFall = PM_DoSlowFall();

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if (pm->ps->pm_type == PM_FLOAT)
	{ //You get no control over where you go in grip movement
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->eFlags & EF_DISINTEGRATION)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->saberMove == LS_A_LUNGE )
	{//can't move during lunge
		pm->cmd.rightmove = pm->cmd.upmove = 0;
		if ( pm->ps->legsTimer > 500 )
		{
			pm->cmd.forwardmove = 127;
		}
		else
		{
			pm->cmd.forwardmove = 0;
		}
	}

	if ( pm->ps->saberMove == LS_A_JUMP_T__B_ )
	{//can't move during leap
		if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
		{//hit the ground
			pm->cmd.forwardmove = 0;
		}
		pm->cmd.rightmove = pm->cmd.upmove = 0;
	}

	if ( pm->ps->saberMove == LS_A_BACK || pm->ps->saberMove == LS_A_BACK_CR 
		|| pm->ps->saberMove == LS_A_BACKSTAB || pm->ps->saberMove == LS_A_FLIP_STAB ||
		pm->ps->saberMove == LS_A_FLIP_SLASH || pm->ps->saberMove == LS_A_JUMP_T__B_ )
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ((pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_A2_STABBACK1) ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_ATTACK_BACK) ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_CROUCHATTACKBACK1) ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_FORCELEAP2_T__B_) ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_JUMPFLIPSTABDOWN) ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == (BOTH_JUMPFLIPSLASHDOWN1))
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (((pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_KISSER1LOOP ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_KISSEE1LOOP) && jk2gameplay == VERSION_1_04)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->emplacedIndex)
	{
		if (pm->cmd.forwardmove < 0)
		{
			pm->ps->emplacedIndex = 0;
		}
		else
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
			pm->cmd.upmove = 0;
		}
	}

	if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->weaponstate == WEAPON_CHARGING_ALT)
	{ //not allowed to move while charging the disruptor
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		if (pm->cmd.upmove > 0)
		{
			pm->cmd.upmove = 0;
		}
	}

	BG_AdjustClientSpeed(pm->ps, &pm->cmd, pm->cmd.serverTime);

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// In certain situations, we may want to control which attack buttons are pressed and what kind of functionality
	//	is attached to them
	PM_AdjustAttackStates( pm );

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK && jk2startversion != VERSION_1_02 ) { // MVSDK: 1.02 people are used to walk around with open console, 1.03 and 1.04 can't do that. Let's make this depending on the actual version we're running, not the gameplay...
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.seed = pmove->cmd.serverTime;
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	if (runFlags & RFL_CLIMBTECH) {
		PM_AdjustAngleForWallJump(pm->ps, &pm->cmd, qtrue);
	}
	PM_AdjustAngleForWallRun(pm->ps, &pm->cmd, qtrue);

	if ((pm->ps->saberMove == LS_A_JUMP_T__B_ || pm->ps->saberMove == LS_A_LUNGE ||
		((pm->ps->saberMove == LS_A_BACK_CR || pm->ps->saberMove == LS_A_BACK ||
		pm->ps->saberMove == LS_A_BACKSTAB) && jk2gameplay == VERSION_1_04)) && jk2gameplay != VERSION_1_02) // MVSDK: One of the place where 1.02, 1.03 and 1.04 are all different!
	{
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	if (((pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_KISSER1LOOP ||
		(pm->ps->legsAnim&~ANIM_TOGGLEBIT) == BOTH_KISSEE1LOOP) && jk2gameplay == VERSION_1_04)
	{
		pm->ps->viewangles[PITCH] = 0;
		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	}

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

#ifdef JK2_GAME
	if ( g_mv_blockspeedhack.integer )
	{
		float oldRoll = pm->ps->viewangles[ROLL];
		pm->ps->viewangles[ROLL] = 0;
		AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);
		pm->ps->viewangles[ROLL] = oldRoll;
	}
	else
	{
#endif
	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);
#ifdef JK2_GAME
	}
#endif

	if ( pm->cmd.upmove < 10 && (!(runFlags & RFL_CLIMBTECH) || !(pm->ps->pm_flags & PMF_STUCK_TO_WALL))) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->saberLockTime >= pm->cmd.serverTime)
	{
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 50;
		pm->cmd.rightmove = 0;//*= 0.1;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return;		// no movement at all
	}

	if (gPMDoSlowFall)
	{
		pm->ps->gravity *= 0.5;
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{//on ground
		pm->ps->fd.forceJumpZStart = 0;
	}

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();


	// TODO MAYBE jaPRO fix strafebot up.
#if JK2_CGAME
	if (pm->ps->stats[STAT_RACEMODE] && pm->ps->pm_type == PM_NORMAL && pm->cmd.buttons & BUTTON_STRAFEBOT){// && !(cgs.restricts & RESTRICT_SB)) {
#else
	if (pm->ps->stats[STAT_RACEMODE] && pm->ps->pm_type == PM_NORMAL && pm->cmd.buttons & BUTTON_STRAFEBOT) {
#endif
		//const int moveStyle = PM_GetMovePhysics();
		const int runFlags = PM_GetRunFlags();
#if JK2_CGAME
		if (pm->ps->clientNum >= 0 && pm->ps->clientNum < MAX_CLIENTS && (runFlags & RFL_BOT))// (moveStyle == MV_BOTJKA /*|| (g_entities[pm->ps->clientNum].client && g_entities[pm->ps->clientNum].client->pers.practice)*/))
#else
		if (pm->ps->clientNum >= 0 && pm->ps->clientNum < MAX_CLIENTS && (runFlags & RFL_BOT))// (moveStyle == MV_BOTJKA /* || (g_entities[pm->ps->clientNum].client && g_entities[pm->ps->clientNum].client->pers.practice)*/))
#endif
		{
			const float realCurrentSpeed = sqrtf((pm->ps->velocity[0] * pm->ps->velocity[0]) + (pm->ps->velocity[1] * pm->ps->velocity[1]));
			if (realCurrentSpeed > 0) {
				vec3_t vel = { 0 }, velangle;
				float optimalDeltaAngle = 0;
				qboolean CJ = qtrue;
				if (pm->ps->groundEntityNum != ENTITYNUM_WORLD || pm->cmd.upmove > 0)
					CJ = qfalse;
				//else if (moveStyle == MV_SLICK)
				//	CJ = qfalse;
				else if (pml.walking && pml.groundTrace.surfaceFlags & SURF_SLICK) { //Lmao fuck this bullshit. no way to tell if we are on slick i guess.
					CJ = qfalse;
				}
				else if (realCurrentSpeed > pm->ps->basespeed * 1.5f) //idk this is retarded, but lets us groundframe
					CJ = qfalse;

				if (realCurrentSpeed > pm->ps->basespeed || (CJ && (realCurrentSpeed > (pm->ps->basespeed * 0.5f)))) {
					float middleOffset = 0; //Idk
#if JK2_GAME
					//middleOffset = bot_strafeOffset.integer;
					middleOffset = 0;
#endif
					if (CJ)
						//if (moveStyle == MV_CPM || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM)
						//	optimalDeltaAngle = -1; //CJ //Take into account ground accel/friction.. only cpm styles turn faster?
						//else
							optimalDeltaAngle = -6;
					else {
						float realAccel = pm_airaccelerate;
						//if (moveStyle == MV_SP)
						//	realAccel = pm_sp_airaccelerate;
						//else if (moveStyle == MV_SLICK)
						//	realAccel = pm_slick_accelerate;
						//jetpack. 1.4f ?
						optimalDeltaAngle = (acos((double)((pm->ps->basespeed - (realAccel * pm->ps->basespeed * pml.frametime)) / realCurrentSpeed)) * (180.0f / M_PI) - 45.0f);
						if (optimalDeltaAngle < 0 || optimalDeltaAngle > 360)
							optimalDeltaAngle = 0;
					}

					vel[0] = pm->ps->velocity[0];
					vel[1] = pm->ps->velocity[1];
					vectoangles(vel, velangle);

					if (pm->cmd.forwardmove > 0 && pm->cmd.rightmove > 0) {//WD
						optimalDeltaAngle = 0 - optimalDeltaAngle;
					}
					else if (pm->cmd.forwardmove > 0 && pm->cmd.rightmove < 0) {//WA
						optimalDeltaAngle = 0 + optimalDeltaAngle;
					}
					else if (!pm->cmd.forwardmove && pm->cmd.rightmove > 0) {//D
						//if (moveStyle == MV_QW || moveStyle == MV_CPM || moveStyle == MV_PJK || moveStyle == MV_WSW || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM)
						//	optimalDeltaAngle = 0 - middleOffset; //Take into account speed.
						//else
							optimalDeltaAngle = 45 - optimalDeltaAngle;
					}
					else if (!pm->cmd.forwardmove && pm->cmd.rightmove < 0) {//A
						//if (moveStyle == MV_QW || moveStyle == MV_CPM || moveStyle == MV_PJK || moveStyle == MV_WSW || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM)
						//	optimalDeltaAngle = 0 + middleOffset;
						//else
							optimalDeltaAngle = -45 + optimalDeltaAngle;
					}
					else if (pm->cmd.forwardmove > 0 && !pm->cmd.rightmove) {//W
						if (AngleSubtract(velangle[YAW], pm->ps->viewangles[YAW]) > 0) { //Decide which W we want.  (Whatever is closest)
							//if (moveStyle == MV_QW || moveStyle == MV_CPM || moveStyle == MV_PJK || moveStyle == MV_WSW || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM) //Why the f does it switch
							//	optimalDeltaAngle = -45; //Needs good offset
							//else
								optimalDeltaAngle = -45 - optimalDeltaAngle;
						}
						else { //Right side
							//if (moveStyle == MV_QW || moveStyle == MV_CPM || moveStyle == MV_PJK || moveStyle == MV_WSW || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM)
							//	optimalDeltaAngle = 45; //Needs good offset
							//else
								optimalDeltaAngle = 45 + optimalDeltaAngle;
						}
					}

					velangle[YAW] += optimalDeltaAngle;
					velangle[PITCH] = pm->ps->viewangles[PITCH];

					PM_SetPMViewAngle(pm->ps, velangle, &pm->cmd);
					AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up); //Have to re set this here
				}
			}
		}
	}

	if (pm->ps->pm_type == PM_FLOAT)
	{
		PM_FlyMove ();
	}
	else if((!pm->requiredCmdMsec || pml.msec == pm->requiredCmdMsec) && (pm->isSpecialPredict || msecRestrict <= 0 || msecRestrict == pml.msec))
	{
		if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
			PM_WaterJumpMove();
		} else if ( pm->waterlevel > 1 ) {
			// swimming
			PM_WaterMove();
		} else if ( pml.walking ) {
			// walking on ground
			PM_WalkMove();
		} else {
			// airborne
			PM_AirMove();
		}
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	if (pm->cmd.forcesel != (byte)-1 && (pm->ps->fd.forcePowersKnown & (1 << pm->cmd.forcesel)))
	{
		pm->ps->fd.forcePowerSelected = pm->cmd.forcesel;
	}
	if (pm->cmd.invensel != (byte)-1 && (pm->ps->stats[STAT_HOLDABLE_ITEMS] & (1 << pm->cmd.invensel)))
	{
		pm->ps->stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(pm->cmd.invensel, IT_HOLDABLE);
	}

	// weapons
	PM_Weapon();

	PM_Use();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	//Walbug fix start, if getting stuck w/o noclip is even possible.  This should maybe be after round float? im not sure..
	// TODO MAYBE jaPRO this actually kills strafing on yavin. find better solution.
	//if ((pm->ps->persistant[PERS_TEAM] != TEAM_SPECTATOR) && pm->ps->stats[STAT_RACEMODE] && VectorCompare(pm->ps->origin, pml.previous_origin) /*&& (VectorLengthSquared(pm->ps->velocity) > VectorLengthSquared(pml.previous_velocity))*/)
	//	VectorClear(pm->ps->velocity); //Their velocity is increasing while their origin is not moving (wallbug), so prevent this..
		//VectorCopy(pml.previous_velocity, pm->ps->velocity);
	//To fix rocket wallbug, since that gets applied elsewhere, just always reset vel if origins dont match?
	//Wallbug fix end

	// snap some parts of playerstate to save network bandwidth
	if (pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		trap_SnapVector( pm->ps->velocity );
	}
	else {
		if (/*pm->ps->stats[STAT_RACEMODE] || */pm->pmove_float > 2  && !pm->ps->stats[STAT_RACEMODE] || msecRestrict == -2) {
		}
#if JK2_GAME
		else if (g_fixHighFPSAbuse.integer
#elif JK2_CGAME
		else if ((cgs.jcinfo & JK2PRO_CINFO_HIGHFPSFIX) //could move these checks to cg_predict, and just set pm->pmove_float accordingly?
#endif
			&& !pm->ps->stats[STAT_RACEMODE]
			&& (pml.msec <= 4 || pml.msec > 25)
			|| pm->requiredCmdMsec && pm->requiredCmdMsec != pml.msec
			|| !pm->isSpecialPredict && msecRestrict > 0 && msecRestrict != pml.msec
			) { //do nothing above 250FPS or below 40FPS, or if a certain msec timing is demanded by the game via requiredCmdMsec (used for toggle limiting via g_fpsToggleDelay)
		}
		else if (pm->pmove_float == 2 && !pm->ps->stats[STAT_RACEMODE]) { //pmove_float 2: snaps vertical velocity only, so 125/142fps jumps are still the same height?
			// TODO allow this option in racemode somehow too?
			vec3_t oldVelocity = { 0 };
			VectorCopy( pm->ps->velocity, oldVelocity );
			trap_SnapVector( pm->ps->velocity );
			pm->ps->velocity[2] = oldVelocity[2];
		}
		else if (!pm->pmove_float || pm->ps->stats[STAT_RACEMODE] && msecRestrict > -2) {
			trap_SnapVector( pm->ps->velocity );
		}
	}

	if (gPMDoSlowFall)
	{
		pm->ps->gravity *= 2;
	}
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	if (pmove->ps->fallingToDeath)
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
		pmove->cmd.buttons = 0;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		}
		else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}
}

