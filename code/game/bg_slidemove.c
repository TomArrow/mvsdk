// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_slidemove.c -- part of bg_pmove functionality

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/


qboolean PM_GroundSlideOkay(float zNormal)
{
	// nvm, already guarded in all calls
	//const int runFlags = PM_GetRunFlags();
	//if (!(runFlags & RFL_CLIMBTECH)) return qtrue;

	if (zNormal > 0)
	{
		if (pm->ps->velocity[2] > 0)
		{
			if (pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT
				|| pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT_STOP
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT_STOP
				|| pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START
				//|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
				//|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
				//|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
				|| BG_InReboundJump(pm->ps->legsAnim))
			{
				return qfalse;
			}
		}
	}
	return qtrue;
}




/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
void PM_Q2StepSlideMove_(void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		normal,planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	int			i, j;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	const int	runFlags = PM_GetRunFlags();

	numbumps = 4;

	VectorCopy(pm->ps->velocity, primal_velocity);
	numplanes = 0;

	time_left = pml.frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = pm->ps->origin[i] + time_left * pm->ps->velocity[i];

		pm->trace(&trace,pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage
			return;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, pm->ps->origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

	   // save entity for contact
		if (pm->numtouch < MAXTOUCH && trace.entityNum != ENTITYNUM_WORLD)
		{
			pm->touchents[pm->numtouch] = trace.entityNum;
			pm->numtouch++;
		}

		time_left -= time_left * trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, pm->ps->velocity);
			break;
		}

		VectorCopy(trace.plane.normal,normal);

		if ((runFlags & RFL_CLIMBTECH) && !PM_GroundSlideOkay(normal[2]))
		{//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize(normal);
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		// 
		// TA: Copied this over from the normal jk function and it makes the movement smoother while keeping it overall nice. Nice!
		//
		if (!(runFlags & RFL_CLIMBTECH) || !(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
		{//no sliding if stuck to wall!
			for (i = 0; i < numplanes; i++) {
				if (DotProduct(normal, planes[i]) > 0.99) {
					VectorAdd(normal, pm->ps->velocity, pm->ps->velocity);
					break;
				}
			}
			if (i < numplanes) {
				continue;
			}
		}

		VectorCopy(normal, planes[numplanes]);
		numplanes++;

#if 0
		float		rub;

		//
		// modify velocity so it parallels all of the clip planes
		//
		if (numplanes == 1)
		{	// go along this plane
			VectorCopy(pm->ps->velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			PM_ClipVelocity(pm->ps->velocity, planes[0], pm->ps->velocity, 1.01);
			// rub some extra speed off on xy axis
			// not on Z, or you can scrub down walls
			pm->ps->velocity[0] *= rub;
			pm->ps->velocity[1] *= rub;
			pm->ps->velocity[2] *= rub;
		}
		else if (numplanes == 2)
		{	// go along the crease
			VectorCopy(pm->ps->velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pm->ps->velocity);
			VectorScale(dir, d, pm->ps->velocity);

			// rub some extra speed off
			VectorScale(pm->ps->velocity, rub, pm->ps->velocity);
		}
		else
		{
			//			Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
			VectorCopy(vec3_origin, pm->ps->velocity);
			break;
		}

#else
		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(pm->ps->velocity, planes[i], pm->ps->velocity, 1.01);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(pm->ps->velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy(vec3_origin, pm->ps->velocity);
				break;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pm->ps->velocity);
			VectorScale(dir, d, pm->ps->velocity);
		}
#endif
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(pm->ps->velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, pm->ps->velocity);
			break;
		}
	}

	if (pm->ps->pm_time)
	{
		VectorCopy(primal_velocity, pm->ps->velocity);
	}
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_Q2StepSlideMove(qboolean gravity)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
	//	vec3_t		delta;
	vec3_t		up, down;

	if (gravity) {
		//if (pm->ps->gravity > 0)
		//	pml.velocity[2] = 0;
		//else
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	PM_Q2StepSlideMove_();

	VectorCopy(pm->ps->origin, down_o);
	VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	pm->trace(&trace,up, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy(up, pm->ps->origin);
	VectorCopy(start_v, pm->ps->velocity);

	PM_Q2StepSlideMove_();

	// push down the final amount
	VectorCopy(pm->ps->origin, down);
	down[2] -= STEPSIZE;
	pm->trace(&trace,pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, pm->ps->origin);
	}

#if 0
	VectorSubtract(pm->ps->origin, up, delta);
	up_dist = DotProduct(delta, start_v);

	VectorSubtract(down_o, start_o, delta);
	down_dist = DotProduct(delta, start_v);
#else
	VectorCopy(pm->ps->origin, up);

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
		+ (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
		+ (up[1] - start_o[1]) * (up[1] - start_o[1]);
#endif

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		VectorCopy(down_o, pm->ps->origin);
		VectorCopy(down_v, pm->ps->velocity);
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pm->ps->velocity[2] = down_v[2];
}



/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean	PM_SlideMove( qboolean gravity ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		normal, planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	const int	runFlags = PM_GetRunFlags();
	const int	moveStyle = PM_GetMovePhysics();
	
	VectorClear( endVelocity );
	VectorClear( endClipVelocity );

	numbumps = 4;

	VectorCopy (pm->ps->velocity, primal_velocity);

	if ( gravity ) {
		VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			if(!(runFlags & RFL_CLIMBTECH) || PM_GroundSlideOkay(pml.groundTrace.plane.normal[2])){
				// slide along the ground plane
				PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
					pm->ps->velocity, OVERCLIP );
			}
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
		if ((runFlags & RFL_CLIMBTECH) && !PM_GroundSlideOkay(planes[0][2]))
		{
			planes[0][2] = 0;
			VectorNormalize(planes[0]);
		}
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy (trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1) {
			 break;		// moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		VectorCopy(trace.plane.normal, normal);

		if ((runFlags & RFL_CLIMBTECH) && !PM_GroundSlideOkay(normal[2]))
		{//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize(normal);
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if (!(runFlags & RFL_CLIMBTECH) || !(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
		{//no sliding if stuck to wall!
			for (i = 0; i < numplanes; i++) {
				if (DotProduct(normal, planes[i]) > 0.99) {
					VectorAdd(normal, pm->ps->velocity, pm->ps->velocity);
					break;
				}
			}
			if (i < numplanes) {
				continue;
			}
		}
		VectorCopy (normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = DotProduct( pm->ps->velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity (pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// TODO MAYBE jaPRO player collision physics fix?

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( pm->ps->velocity ); // TODO can we make this nicer? is this why we can get stuck in sloped walls stuff?
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

// A dumbed down version of PM_SlideMove
// We only check if we freely get where we need to get and then do a check for ground under us.
// The apparent cause of the bug in a nutshell:
// We can freely (no clip/objects/floor in the way) travel/fall to the place where we will logically be in pml.frameTime time.
// This means no ramp boost through PM_ClipVelocity happens.
// Let's say we are falling straight down at effective velocity[2] == -1000 and 142 fps (pml.frametime == 0.007 which is 7msec).
// That puts us traveling down at about 7 units per frame. 
// If the ground is less than 7 units away from us, we will catch it in the initial PM_StepSlideMove and it will be clipped (ramp boost).
// If the ground is 8 units away from us, we will catch it the same way in the next frame.
// HOWEVER, if the ground is more than 7 and less than 7.25 units away from us, we will first fall the 7 units, and then the following ground check (which checks 0.25 units under new position)
// will determine that we are standing on ground without giving ramp boost. Which is also the place where Loda's old ramp boost fix is applied in case the ramp boost didn't happen which he checked via
// pml.clipped
qboolean PM_PredictDeadRamp(qboolean gravity) {
	trace_t	trace;
	vec3_t		end;
	vec3_t		testVelocity;
	vec3_t		point;
	vec3_t		newPos;
	int			i, j, k;

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE) {
		return qfalse;
	}

	VectorCopy(pm->ps->velocity, testVelocity);
	if (gravity) {
		testVelocity[2] -= pm->ps->gravity * pml.frametime;
		testVelocity[2] = (pm->ps->velocity[2] + testVelocity[2]) * 0.5;
	}

	// calculate position we are trying to move to
	VectorMA(pm->ps->origin, pml.frametime, testVelocity, end);

	// see if we can make it there
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);


	if (trace.fraction == 1) {
		VectorCopy(trace.endpos, newPos);

		point[0] = newPos[0];
		point[1] = newPos[1];
		point[2] = newPos[2] - 0.25;

		pm->trace(&trace, newPos, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);

		// do something corrective if the trace starts in a solid...
		if (trace.allsolid) {
			// jitter around
			for (i = -1; i <= 1; i++) {
				for (j = -1; j <= 1; j++) {
					for (k = -1; k <= 1; k++) {
						VectorCopy(newPos, point);
						point[0] += (float)i;
						point[1] += (float)j;
						point[2] += (float)k;
						pm->trace(&trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
						if (!trace.allsolid) {
							point[0] = newPos[0];
							point[1] = newPos[1];
							point[2] = newPos[2] - 0.25;

							pm->trace(&trace, newPos, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
							i = j = k = 2; // Stupid way to end the loop lol.
						}
					}
				}
			}
		}

		if (trace.fraction != 1.0 && (trace.plane.normal[0] != 0.0f || trace.plane.normal[1] != 0.0f || trace.plane.normal[2] != 1.0f)) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( qboolean gravity ) {
	vec3_t		start_o, start_v;
	// vec3_t		down_o, down_v;
	trace_t		trace;
//	float		down_dist, up_dist;
//	vec3_t		delta, delta2;
	vec3_t		up, down;
	vec3_t		nvel, prevel;
	float		stepSize;
	float		totalVel;
	float		pre_z;
	int			usingspeed;
	int			i;
	const int moveStyle = PM_GetMovePhysics();
	int NEW_STEPSIZE = STEPSIZE;

	if (moveStyle == MV_QUAJK) {
		PM_Q2StepSlideMove(gravity);
		return;
		if (pm->ps->velocity[2] > 0 && pm->cmd.upmove > 0) { // do we really need this?
			int jumpHeight = pm->ps->origin[2] - pm->ps->fd.forceJumpZStart;

			if (jumpHeight > 48)
				jumpHeight = 48;
			else if (jumpHeight < 22)
				jumpHeight = 22;

			NEW_STEPSIZE = 48 - jumpHeight + 22;

			//trap->SendServerCommand(-1, va("print \"new stepsize: %i, expected max end height: %i\n\"", NEW_STEPSIZE, NEW_STEPSIZE + (int)(pm->ps->origin[2] - pm->ps->fd.forceJumpZStart)));

			//This means that we can always clip things up to 48 units tall, if we are moving up when we hit it and from a bhop..
			//It means we can sometimes clip things up to 70 units tall, if we hit it in right part of jump
			//Should it be higher..? some of the things in q3 are 56 units tall..

			//NEW_STEPSIZE = 46;
			//Make stepsize equal to.. our current 48 - our current jumpheight ?
		}
		else
			NEW_STEPSIZE = 22;
	}

	i = 0;

	usingspeed = 0;

	VectorCopy (pm->ps->origin, start_o);
	VectorCopy (pm->ps->velocity, start_v);

	if (pm->debugLevel) {
		if (PM_PredictDeadRamp(gravity)) {
			Com_Printf("%i:predicting dead ramp\n", c_pmove);
		}
	}
	
	if (BG_InReboundHold(pm->ps->legsAnim))
	{
		gravity = qfalse;
	}

	if ( PM_SlideMove( gravity ) == 0 ) {
		return;		// we got exactly where we wanted to go first try	
	}

	if (pm->ps->fd.forcePowersActive & (1 << FP_SPEED))
	{
		usingspeed = 1;
	}

	VectorCopy(start_o, down);
	down[2] -= NEW_STEPSIZE;
	pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if ( pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
										DotProduct(trace.plane.normal, up) < 0.7)) {

		if (!usingspeed)
		{
			return;
		}
	}

	// VectorCopy (pm->ps->origin, down_o);
	// VectorCopy (pm->ps->velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += NEW_STEPSIZE;

	// test the player position if they were a stepheight higher
	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if ( trace.allsolid ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		if (!usingspeed)
		{
			return;		// can't step up
		}
	}

	pml.clippedPre = qtrue;

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy (trace.endpos, pm->ps->origin);
	VectorCopy (start_v, pm->ps->velocity);

	VectorCopy(pm->ps->velocity, prevel);

	pre_z = prevel[2];

	PM_SlideMove( gravity );

	pml.clipped = qtrue;

	VectorSubtract(pm->ps->velocity, prevel, prevel);
	if (prevel[0] < 0)
	{
		prevel[0] = -prevel[0];
	}
	if (prevel[1] < 0)
	{
		prevel[1] = -prevel[1];
	}

	totalVel = prevel[0]+prevel[1];

	if (pre_z > 480 && (pre_z - pm->ps->velocity[2]) >= 480 && pm->ps->fd.forceJumpZStart)
	{ //smashed head on the ceiling during a force jump
		pm->ps->fd.forceSpeedDoDamage = (pre_z - pm->ps->velocity[2])*0.04;
		if (pm->numtouch)
		{ //do damage to the other player if we hit one
			while (i < pm->numtouch)
			{
				if (pm->touchents[i] < MAX_CLIENTS && pm->touchents[i] != pm->ps->clientNum)
				{
					pm->ps->fd.forceSpeedHitIndex = pm->touchents[i];
					break;
				}

				i++;
			}
		}

		i = 0;
	}

	if (usingspeed)
	{
		if (pm->ps->fd.forceSpeedSmash > 1.3 && totalVel > 500)
		{ //if we were going fast enough and hadn't hit a while in a while then smash into it hard
		  //the difference between our velocity pre and post colide must also be greater than 600 to do damage
			//Com_Printf("SMASH %f\n", pm->ps->fd.forceSpeedSmash);
			VectorCopy(start_v, nvel); //then bounce the player back a bit in the opposite of the direction he was going
			nvel[0] += start_o[0];
			nvel[1] += start_o[1];
			nvel[2] += start_o[2];
			VectorSubtract(start_o, nvel, nvel);
			pm->ps->velocity[0] = nvel[0]*0.1;
			pm->ps->velocity[1] = nvel[1]*0.1;
			pm->ps->velocity[2] = 64;
			pm->ps->fd.forceSpeedDoDamage = pm->ps->fd.forceSpeedSmash*10; //do somewhere in the range of 15-25 damage, depending on speed
			pm->ps->fd.forceSpeedSmash = 0;

			if (pm->numtouch)
			{
				while (i < pm->numtouch)
				{
					if (pm->touchents[i] < MAX_CLIENTS && pm->touchents[i] != pm->ps->clientNum)
					{
						pm->ps->fd.forceSpeedHitIndex = pm->touchents[i];
						break;
					}

					i++;
				}
			}
			return;
		}

		pm->ps->fd.forceSpeedSmash -= 0.1;
		//we hit a wall so decrease speed

		if (pm->ps->fd.forceSpeedSmash < 1)
		{
			pm->ps->fd.forceSpeedSmash = 1;
		}
	}

	// push down the final amount
	VectorCopy (pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if ( !trace.allsolid ) {
		VectorCopy (trace.endpos, pm->ps->origin);
	}
	if ( trace.fraction < 1.0 ) {
		PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
	}

#if 0
	// if the down trace can trace back to the original position directly, don't step
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, start_o, pm->ps->clientNum, pm->tracemask);
	if ( trace.fraction == 1.0 ) {
		// use the original move
		VectorCopy (down_o, pm->ps->origin);
		VectorCopy (down_v, pm->ps->velocity);
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend\n", c_pmove);
		}
	} else 
#endif
	{
		// use the step move
		float	delta;

		delta = pm->ps->origin[2] - start_o[2];
		if ( delta > 2 ) {
			if ( delta < 7 ) {
				PM_AddEvent( EV_STEP_4 );
			} else if ( delta < 11 ) {
				PM_AddEvent( EV_STEP_8 );
			} else if ( delta < 15 ) {
				PM_AddEvent( EV_STEP_12 );
			} else {
				PM_AddEvent( EV_STEP_16 );
			}
		}
		if ( pm->debugLevel ) {
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}

