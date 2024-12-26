/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../game/bg_pmove_q2.h"
#include "../game/bg_public.h"

//#define CRINGY_STUCK_DEBUG

#define AUTHENTIC_Q2SNAP

#define	STEPSIZE	18

extern int		c_pmove;

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

typedef struct
{
	vec3_t		origin;			// full float precision
	vec3_t		velocity;		// full float precision


	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	//csurface_t* groundsurface;
	cplane_t	groundplane;
	int			groundcontents;
	int			surfaceFlags;
	trace_t		groundTrace;
	qboolean	groundFound;
	qboolean	clipped;


	int			forwardmove, rightmove, upmove;

	vec3_t		previous_origin;
	vec3_t		previous_velocity; // TA: addition for non-standard (settable) corner skims (pmq2->cornerSkims)
	qboolean	ladder;
} pmlq2_t;

pmoveq2_t* pmq2;
pmlq2_t		pmlq2;


// movement parameters
float	pmq2_stopspeed = 100;
float	pmq2_maxspeed = 300;
float	pmq2_duckspeed = 100;
float	pmq2_accelerate = 10;
float	pmq2_airaccelerate = 0;
float	pmq2_wateraccelerate = 10;
float	pmq2_friction = 6;
float	pmq2_waterfriction = 1;
float	pmq2_waterspeed = 400;

/*

  walking up a step should kill some velocity

*/


/*
==================
PMQ2_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

void PMQ2_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}




/*
==================
PMQ2_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
void PMQ2_StepSlideMove_(void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
#ifdef CRINGY_STUCK_DEBUG
	vec3_t		preBumpVel;
#endif
	int			i, j;
	trace_t	trace;
	vec3_t		end;
	float		time_left;

	numbumps = 4;

	VectorCopy(pmlq2.velocity, primal_velocity);
	numplanes = 0;

	time_left = pmlq2.frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = pmlq2.origin[i] + time_left * pmlq2.velocity[i];

		pmq2->trace(&trace,pmlq2.origin, pmq2->mins, pmq2->maxs, end,pmq2->ps->clientNum,pmq2->tracemask);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			pmlq2.velocity[2] = 0;	// don't build up falling damage
			return;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, pmlq2.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

	   // save entity for contact
		if (pmq2->numtouch < MAXTOUCH && trace.entityNum != ENTITYNUM_NONE)
		{
			pmq2->touchents[pmq2->numtouch] = trace.entityNum;
			pmq2->numtouch++;
		}

		time_left -= time_left * trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, pmlq2.velocity);
#ifdef CRINGY_STUCK_DEBUG
			Com_Printf("numplanes >= MAX_CLIP_PLANES, nulling\n");
#endif
			break;
		}

		if (!pmq2->haveQ2StyleTrace) {
			//
			// if this is the same plane we hit before, nudge velocity
			// out along it, which fixes some epsilon issues with
			// non-axial planes
			//
			// TA: I just couldn't properly get everything to be smooth without this. I tried adjusting trace code to make it more compatible with Q2, but it still wasn't satisfactory. 
			// 
			for (i = 0; i < numplanes; i++) {
				if (DotProduct(trace.plane.normal, planes[i]) > 0.99) {
					VectorAdd(trace.plane.normal, pmlq2.velocity, pmlq2.velocity);
					break;
				}
			}
			//if (i < numplanes) {
			//	continue;
			//}
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

#if 0
		float		rub;

		//
		// modify velocity so it parallels all of the clip planes
		//
		if (numplanes == 1)
		{	// go along this plane
			VectorCopy(pmlq2.velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			PMQ2_ClipVelocity(pmlq2.velocity, planes[0], pmlq2.velocity, 1.01);
			// rub some extra speed off on xy axis
			// not on Z, or you can scrub down walls
			pmlq2.velocity[0] *= rub;
			pmlq2.velocity[1] *= rub;
			pmlq2.velocity[2] *= rub;
		}
		else if (numplanes == 2)
		{	// go along the crease
			VectorCopy(pmlq2.velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pmlq2.velocity);
			VectorScale(dir, d, pmlq2.velocity);

			// rub some extra speed off
			VectorScale(pmlq2.velocity, rub, pmlq2.velocity);
		}
		else
		{
			//			Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
			VectorCopy(vec3_origin, pmlq2.velocity);
			break;
		}

#else

#ifdef CRINGY_STUCK_DEBUG
		VectorCopy(pmlq2.velocity, preBumpVel);
#endif

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i < numplanes; i++)
		{
			PMQ2_ClipVelocity(pmlq2.velocity, planes[i], pmlq2.velocity, 1.01);
#if 0//#ifdef  CRINGY_STUCK_DEBUG
			{
				vec3_t normVel;
				float dot, angle;
				VectorCopy(pmlq2.velocity, normVel);
				VectorNormalize(normVel);
				dot = DotProduct(planes[i], normVel);
				if (dot <= 0) {
					angle = acos(MIN(1.0f, MAX(0.0f, dot))) * (180.0f / M_PI);
					Com_Printf("clip velocity dot <= 0 dot between newVel and normal %f angle %f fraction %f entitynum %d startsolid %d allsolid %d\n", dot, angle, trace.fraction, trace.entityNum, trace.startsolid, trace.allsolid);
				}
			}
#endif
			if (planes[i][2] >= 0.7) {
				pmlq2.clipped = qtrue; // uh am i putting this the right place? idk
			}
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(pmlq2.velocity, planes[j]) < 0)
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
				VectorCopy(vec3_origin, pmlq2.velocity);
#ifdef CRINGY_STUCK_DEBUG
				Com_Printf("i == numplanes && numplanes != 2, nulling\n");
#endif
				break;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pmlq2.velocity);
#ifdef CRINGY_STUCK_DEBUG
			if (!d) {
				vec3_t normVel,traceDir;
				float dot,dot2,dot3,angle;
				VectorCopy(preBumpVel, normVel);
				VectorSubtract(end, pmlq2.origin, traceDir);
				VectorNormalize(normVel);
				dot = DotProduct(planes[0], normVel);
				dot2 = DotProduct(planes[0], preBumpVel);
				dot3 = DotProduct(planes[0], traceDir);
				angle = acos(MIN(1.0f,MAX(0.0f,dot))) * (180.0f / M_PI);
				Com_Printf("go along the crease 0 direction vel (identical planes?) dot between preBumpVel and normal %f, non norm dot %f, dirmove dot nonorm %f, angle %f fraction %f entitynum %d startsolid %d allsolid %d\n",dot,dot2,dot3,angle,trace.fraction,trace.entityNum,trace.startsolid,trace.allsolid);
				// for debug: (so u can step in dev-sama)
				pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, end, pmq2->ps->clientNum, pmq2->tracemask);
			}
#endif
 			VectorScale(dir, d, pmlq2.velocity);
		}
#endif
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(pmlq2.velocity, primal_velocity) <= 0)
		{
#ifdef CRINGY_STUCK_DEBUG
			Com_Printf("DotProduct(pmlq2.velocity, primal_velocity) <= 0, nulling\n");
#endif
			VectorCopy(vec3_origin, pmlq2.velocity);
			break;
		}
	}

	if (pmq2->ps->pm_time)
	{
		VectorCopy(primal_velocity, pmlq2.velocity);
	}
}

/*
==================
PMQ2_StepSlideMove

==================
*/
void PMQ2_StepSlideMove(void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
	//	vec3_t		delta;
	vec3_t		up, down;

	VectorCopy(pmlq2.origin, start_o);
	VectorCopy(pmlq2.velocity, start_v);

	PMQ2_StepSlideMove_();

	VectorCopy(pmlq2.origin, down_o);
	VectorCopy(pmlq2.velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	pmq2->trace(&trace, up, pmq2->mins, pmq2->maxs, up, pmq2->ps->clientNum, pmq2->tracemask);

	if (trace.allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy(up, pmlq2.origin);
	VectorCopy(start_v, pmlq2.velocity);

	PMQ2_StepSlideMove_();

	// push down the final amount
	VectorCopy(pmlq2.origin, down);
	down[2] -= STEPSIZE;
	pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, down, pmq2->ps->clientNum, pmq2->tracemask);
	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, pmlq2.origin);
	}

#if 0
	VectorSubtract(pmlq2.origin, up, delta);
	up_dist = DotProduct(delta, start_v);

	VectorSubtract(down_o, start_o, delta);
	down_dist = DotProduct(delta, start_v);
#else
	VectorCopy(pmlq2.origin, up);

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
		+ (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
		+ (up[1] - start_o[1]) * (up[1] - start_o[1]);
#endif

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		VectorCopy(down_o, pmlq2.origin);
		VectorCopy(down_v, pmlq2.velocity);
		return;
	}
	else {
		//if (pm->debugLevel) {
		//	Com_Printf("%i:stepped up\n", c_pmove);
		//}
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pmlq2.velocity[2] = down_v[2];
}


/*
==================
PMQ2_Friction

Handles both ground friction and water friction
==================
*/
void PMQ2_Friction(void)
{
	float* vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	vel = pmlq2.velocity;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

	// apply ground friction
	if ((pmq2->ps->groundEntityNum != ENTITYNUM_NONE && pmlq2.groundFound && !(pmlq2.surfaceFlags & SURF_SLICK)) || (pmlq2.ladder))
	{
		friction = pmq2_friction;
		control = speed < pmq2_stopspeed ? pmq2_stopspeed : speed;
		drop += control * friction * pmlq2.frametime;
	}

	// apply water friction
	if (pmq2->waterlevel && !pmlq2.ladder)
		drop += speed * pmq2_waterfriction * pmq2->waterlevel * pmlq2.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PMQ2_Accelerate

Handles user intended acceleration
==============
*/
void PMQ2_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(pmlq2.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * pmlq2.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlq2.velocity[i] += accelspeed * wishdir[i];
}

void PMQ2_AirAccelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct(pmlq2.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * pmlq2.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlq2.velocity[i] += accelspeed * wishdir[i];
}

/*
=============
PMQ2_AddCurrents
=============
*/
void PMQ2_AddCurrents(vec3_t	wishvel)
{
	vec3_t	v;
	float	s;

	//
	// account for ladders
	//

	if (pmlq2.ladder && fabs(pmlq2.velocity[2]) <= 200)
	{
		if ((pmq2->ps->viewangles[PITCH] <= -15) && (pmlq2.forwardmove > 0))
			wishvel[2] = 200;
		else if ((pmq2->ps->viewangles[PITCH] >= 15) && (pmlq2.forwardmove > 0))
			wishvel[2] = -200;
		else if (pmlq2.upmove > 0)
			wishvel[2] = 200;
		else if (pmlq2.upmove < 0)
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		if (wishvel[0] < -25)
			wishvel[0] = -25;
		else if (wishvel[0] > 25)
			wishvel[0] = 25;

		if (wishvel[1] < -25)
			wishvel[1] = -25;
		else if (wishvel[1] > 25)
			wishvel[1] = 25;
	}


	//
	// add water currents
	//
	/* TA: doesn't exist in q3 engine
	if (pmq2->watertype & MASK_CURRENT)
	{
		VectorClear(v);

		if (pmq2->watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pmq2->watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pmq2->watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pmq2->watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pmq2->watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pmq2->watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pmq2_waterspeed;
		if ((pmq2->waterlevel == 1) && (pmq2->ps->groundEntityNum != ENTITYNUM_NONE))
			s /= 2;

		VectorMA(wishvel, s, v, wishvel);
	}*/

	//
	// add conveyor belt velocities
	//

	//if (pmq2->ps->groundEntityNum != ENTITYNUM_NONE)
	//{
	//	VectorClear(v);

	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_0)
	//		v[0] += 1;
	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_90)
	//		v[1] += 1;
	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_180)
	//		v[0] -= 1;
	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_270)
	//		v[1] -= 1;
	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_UP)
	//		v[2] += 1;
	//	if (pmlq2.groundcontents & CONTENTS_CURRENT_DOWN)
	//		v[2] -= 1;

	//	VectorMA(wishvel, 100 /* pmq2->groundentity->speed */, v, wishvel);
	//}
}


/*
===================
PMQ2_WaterMove

===================
*/
void PMQ2_WaterMove(void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

	//
	// user intentions
	//
	for (i = 0; i < 3; i++)
		wishvel[i] = pmlq2.forward[i] * pmlq2.forwardmove + pmlq2.right[i] * pmlq2.rightmove;

	if (!pmlq2.forwardmove && !pmlq2.rightmove && !pmlq2.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pmlq2.upmove;

	PMQ2_AddCurrents(wishvel);

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	pmq2->ps->speed = pmq2_maxspeed;

	if (wishspeed > pmq2_maxspeed)
	{
		VectorScale(wishvel, pmq2_maxspeed / wishspeed, wishvel);
		wishspeed = pmq2_maxspeed;
	}
	wishspeed *= 0.5;

	PMQ2_Accelerate(wishdir, wishspeed, pmq2_wateraccelerate);

	PMQ2_StepSlideMove();
}


/*
===================
PMQ2_AirMove

===================
*/
void PMQ2_AirMove(void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		maxspeed;

	fmove = pmlq2.forwardmove;
	smove = pmlq2.rightmove;

	//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	pmlq2.forward[2] = 0;
	pmlq2.right[2] = 0;
	VectorNormalize(pmlq2.forward);
	VectorNormalize(pmlq2.right);
#endif

	for (i = 0; i < 2; i++)
		wishvel[i] = pmlq2.forward[i] * fmove + pmlq2.right[i] * smove;
	wishvel[2] = 0;

	PMQ2_AddCurrents(wishvel);

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	maxspeed = (pmq2->ps->pm_flags & PMF_DUCKED) ? pmq2_duckspeed : pmq2_maxspeed;

	pmq2->ps->speed = maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale(wishvel, maxspeed / wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if (pmlq2.ladder)
	{
		PMQ2_Accelerate(wishdir, wishspeed, pmq2_accelerate);
		if (!wishvel[2])
		{
			if (pmlq2.velocity[2] > 0)
			{
				pmlq2.velocity[2] -= pmq2->ps->gravity * pmlq2.frametime;
				if (pmlq2.velocity[2] < 0)
					pmlq2.velocity[2] = 0;
			}
			else
			{
				pmlq2.velocity[2] += pmq2->ps->gravity * pmlq2.frametime;
				if (pmlq2.velocity[2] > 0)
					pmlq2.velocity[2] = 0;
			}
		}
		PMQ2_StepSlideMove();
	}
	else if (pmq2->ps->groundEntityNum != ENTITYNUM_NONE)
	{	// walking on ground
		pmlq2.velocity[2] = 0; //!!! this is before the accel
		PMQ2_Accelerate(wishdir, wishspeed, pmq2_accelerate);

		// PGM	-- fix for negative trigger_gravity fields
		//		pmlq2.velocity[2] = 0;
		if (pmq2->ps->gravity > 0)
			pmlq2.velocity[2] = 0;
		else
			pmlq2.velocity[2] -= pmq2->ps->gravity * pmlq2.frametime;
		// PGM

		if (!pmlq2.velocity[0] && !pmlq2.velocity[1])
			return;
		PMQ2_StepSlideMove();
	}
	else
	{	// not on ground, so little effect on velocity
		if (pmq2_airaccelerate)
			PMQ2_AirAccelerate(wishdir, wishspeed, pmq2_accelerate);
		else
			PMQ2_Accelerate(wishdir, wishspeed, 1);
		// add gravity
		pmlq2.velocity[2] -= pmq2->ps->gravity * pmlq2.frametime;
		PMQ2_StepSlideMove();
	}
}



/*
=============
PMQ2_CatagorizePosition
=============
*/
void PMQ2_CatagorizePosition(int type)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;
	int			sample1;
	int			sample2;

	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid	
	point[0] = pmlq2.origin[0];
	point[1] = pmlq2.origin[1];
	point[2] = pmlq2.origin[2] - 0.25;
	if (pmlq2.velocity[2] > 180) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		//pmq2->ps->pm_flags &= ~PMF_ON_GROUND;
		pmq2->ps->groundEntityNum = ENTITYNUM_NONE;
	}
	else
	{
		pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, point, pmq2->ps->clientNum, pmq2->tracemask);
		pmlq2.groundplane = trace.plane;
		//pmlq2.groundsurface = trace.surface;
		pmlq2.surfaceFlags = trace.surfaceFlags;
		pmlq2.groundcontents = trace.contents;
		pmlq2.groundFound = trace.entityNum != ENTITYNUM_NONE; // is this right?

#ifdef CRINGY_STUCK_DEBUG
		if (trace.allsolid) {
			// step into it.
			pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, point, pmq2->ps->clientNum, pmq2->tracemask);
		}
#endif

		if (trace.entityNum == ENTITYNUM_NONE || (trace.plane.normal[2] < 0.7 && !trace.startsolid))
		{
			pmq2->ps->groundEntityNum = ENTITYNUM_NONE;
			//pmq2->ps->pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{


			int oldGroundEntityNum = pmq2->ps->groundEntityNum;
			pmq2->ps->groundEntityNum = trace.entityNum;

			// hitting solid ground will end a waterjump
			if (pmq2->ps->pm_flags & PMF_TIME_WATERJUMP)
			{
				pmq2->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND/* | PMF_TIME_TELEPORT*/);
				pmq2->ps->pm_time = 0;
			}

			//if (!(pmq2->ps->pm_flags & PMF_ON_GROUND))
			if (oldGroundEntityNum == ENTITYNUM_NONE)
			{	// just hit the ground

				const int runFlags = PM_GetRunFlags();

				if (pm->debugLevel) {
					Com_Printf("%i:landed, pmlq2.velocity[2] %f (call %d)\n", c_pmove, pmlq2.velocity[2],type);
				}

				// Thanks to Loda for making this fix and Daggo for pointing me to it.
				if ((trace.plane.normal[0] != 0.0f || trace.plane.normal[1] != 0.0f || trace.plane.normal[2] != 1.0f))// don't count them during special predict
				{ // It's a ramp!
					if (!pmlq2.clipped)
					{
						// TODO should we do more checks here to make sure it behaves same as normal clip would? 
						// the trace.plane.normal[2] != 1.0f check in particular seems sus no? since the slidemove stuff
						// works more with various dot products to determine whether to clip etc. oh well. fuck it.

						if (runFlags & RFL_NODEADRAMPS) {

							PMQ2_ClipVelocity(pmlq2.velocity, trace.plane.normal, pmlq2.velocity, 1.01);

							if (pm->debugLevel) {
								Com_Printf("%i:Dead ramp fixed\n", c_pmove);
							}
						}
						else {
							if (pm->debugLevel) {
								Com_Printf("%i:Dead ramp\n", c_pmove);
							}
						}
					}
					else {
						if (pm->debugLevel) {
							Com_Printf("%i:Good ramp\n", c_pmove);
						}
					}
				}


				if (pmq2->cornerSkims >= 5) {
					// this is NONSTANDARD q2 behavior, but in some situations it may happen as you can see below.
					// the idea of the standard heavior is that you get more pm_time the faster you fall, but obviously
					// that doesn't work out by default since slidemove will already take away the Z velocity most of the time
					// We allow pmq2->cornerSkims to set the amount of pm_time added. the high value is supposed to be reserved
					// for higher falls but gives more boost 
					//
					// uhm anyway in normal q3 code this all works via pml.previous_velocity which we could do here too (check -200)
					// and maybe we will, so consider this just a proof of concept
					// the problem with that would be that there may not be a previous_velocity due to only reaching ground as a result of 
					// 1/8 origin snapping of the previous pmove at the start of the new pmove, so that would ahve to be solved as well
					// but if we did that maybe we should use the logic from below (make downspeed dictate amount of pm_time)
					//
					// 
					pmq2->ps->pm_flags |= PMF_TIME_LAND; 
					pmq2->ps->pm_time = pmq2->cornerSkims == 6 ? 25 : 18;
					if (pm->debugLevel) {
						Com_Printf("%i:PMF_TIME_LAND, cornerSkims %d, pmlq2.previous_velocity[2] %f, pmlq2.velocity[2] %f (call %d)\n", c_pmove, pmq2->cornerSkims,pmlq2.previous_velocity[2],pmlq2.velocity[2], type);
					}
				}
				else if (pmq2->cornerSkims >= 1) {
					// this restores originally intended behavior (sorta) of q2 (using previous_velocity instead of velocity 
					// which will usually already have killed downward speed through slidemove
					// 

					if (pmlq2.previous_velocity[2] < -200) {
						pmq2->ps->pm_flags |= PMF_TIME_LAND;
						// don't allow another jump for a little while (TA: it does allow jump if rampfix is active, it just lets us skim corners)
						if (pmq2->cornerSkims <= 2) { 
							// value 1 and 2 of cornerSkims restores "intended" q2 behavior based on previous_velocity
							// value 1 is truly vanilla because it wont let us jump afterwards as q2 devs intended.
							// value 2 will but keep the rest vanilla. 
							// 
							if (pmlq2.previous_velocity[2] < -400)
								pmq2->ps->pm_time = 25;
							else
								pmq2->ps->pm_time = 18;
						}
						else {//if (pmq2->cornerSkims > 2) {
							// values 3 and 4 of cornerSkims behave more like q3 with fixed pm_time no matter the actual previous_velocitty as long as its under negative 200
							pmq2->ps->pm_time = pmq2->cornerSkims == 4 ? 25 : 18;
						}
						if (pm->debugLevel) {
							Com_Printf("%i:PMF_TIME_LAND, pm_time %d, cornerSkims %d, pmlq2.previous_velocity[2] %f (call %d)\n", c_pmove, pmq2->ps->pm_time, pmq2->cornerSkims, pmlq2.previous_velocity[2], type);
						}
					}

				}
				//pmq2->ps->pm_flags |= PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				else if (pmlq2.velocity[2] < -200 && !(runFlags & RFL_NODEADRAMPS)) 
				{
					// rampfix removes rng-feeling vanilla behavior here by not setting pm_time ever.
					// in normal gameplay pm_time is almost never set but when it IS set, it wont let us jump and feel terrible
					// 
					// a good example of this is com_physicsfps 142 on shitstrafe3. we constantly get stopped.
					// 
					// rampfix already also fixes that jump wont trigger wwhen PMF_TIME_LAND is in pm_flags
					// however we still don't want it to get randomly set because it allows corner skims
					// and for competitive defrag we dont want a 1-in-a-million technique to become the way 
					// to win a map
					// 
					// See above code (pmq2->cornerSkims) for a more q3 like behavior for skimming corners

					pmq2->ps->pm_flags |= PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (pmlq2.velocity[2] < -400)
						pmq2->ps->pm_time = 25;
					else
						pmq2->ps->pm_time = 18;

					if (pm->debugLevel) {
						Com_Printf("%i:PMF_TIME_LAND, pmlq2.velocity[2] %f (call %d)\n", c_pmove, pmlq2.velocity[2],type);
					}
				}
			}
		}

#if 0
		if (trace.fraction < 1.0 && trace.ent && pmlq2.velocity[2] < 0)
			pmlq2.velocity[2] = 0;
#endif

		if (pmq2->numtouch < MAXTOUCH && trace.entityNum != ENTITYNUM_NONE)
		{
			pmq2->touchents[pmq2->numtouch] = trace.entityNum;
			pmq2->numtouch++;
		}
	}

	//
	// get waterlevel, accounting for ducking
	//
	pmq2->waterlevel = 0;
	pmq2->watertype = 0;

	sample2 = pmq2->ps->viewheight - pmq2->mins[2];
	sample1 = sample2 / 2;

	point[2] = pmlq2.origin[2] + pmq2->mins[2] + 1;
	cont = pmq2->pointcontents(point,pmq2->ps->clientNum);

	if (cont & MASK_WATER)
	{
		pmq2->watertype = cont;
		pmq2->waterlevel = 1;
		point[2] = pmlq2.origin[2] + pmq2->mins[2] + sample1;
		cont = pmq2->pointcontents(point,pmq2->ps->clientNum);
		if (cont & MASK_WATER)
		{
			pmq2->waterlevel = 2;
			point[2] = pmlq2.origin[2] + pmq2->mins[2] + sample2;
			cont = pmq2->pointcontents(point, pmq2->ps->clientNum);
			if (cont & MASK_WATER)
				pmq2->waterlevel = 3;
		}
	}

}


/*
=============
PMQ2_CheckJump
=============
*/
void PMQ2_CheckJump(void)
{
	if (pmq2->ps->pm_flags & PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		const int runFlags = PM_GetRunFlags();

		if (pmq2->cornerSkims) {
			if (pmq2->cornerSkims == 1) {
				return; // cornerSkims 1 == originally intended vanilla q2 behavior (feels bad, wont be able to bunnyhop)
			}
			else {
				// above 1 cornerSkims we can actually skim corners regularly
			}
		}
		else if (runFlags & RFL_NODEADRAMPS) {
			// kinda in the same spirit tbh. this behaves like an RNG that sometimes wont let us hop and keep speed.
			// different fps makes it behave differently as well. it feels really bad.
			// depending on whether the even floor was clipped during the slidemove,
			// PMF_TIME_LAND will be set or not, as its dependent on downward speed,
			// but the slidemove clipping will eliminate that downward speed (or not),
			// so this is essentially completely pointless RNG
		}
		else {
			return;
		}
	}

	if (pmlq2.upmove < 10)
	{	// not holding jump
		pmq2->ps->pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pmq2->ps->pm_flags & PMF_JUMP_HELD)
		return;

	if (pmq2->ps->pm_type == PM_DEAD)
		return;

	if (pmq2->waterlevel >= 2)
	{	// swimming, not jumping
		pmq2->ps->groundEntityNum = ENTITYNUM_NONE;

		if (pmlq2.velocity[2] <= -300)
			return;

		if (pmq2->watertype == CONTENTS_WATER)
			pmlq2.velocity[2] = 100;
		else if (pmq2->watertype == CONTENTS_SLIME)
			pmlq2.velocity[2] = 80;
		else
			pmlq2.velocity[2] = 50;
		return;
	}

	if (pmq2->ps->groundEntityNum == ENTITYNUM_NONE)
		return;		// in air, so no effect

	pmq2->ps->pm_flags |= PMF_JUMP_HELD;

	pmq2->ps->groundEntityNum = ENTITYNUM_NONE;
	pmlq2.velocity[2] += 270;
	if (pmlq2.velocity[2] < 270)
		pmlq2.velocity[2] = 270;

	if (pm->debugLevel) {
		Com_Printf("%i:jump\n", c_pmove);
	}
}


/*
=============
PMQ2_CheckSpecialMovement
=============
*/
void PMQ2_CheckSpecialMovement(void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	trace_t	trace;

	if (pmq2->ps->pm_time)
		return;

	pmlq2.ladder = qfalse;

	// check for ladder
	flatforward[0] = pmlq2.forward[0];
	flatforward[1] = pmlq2.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pmlq2.origin, 1, flatforward, spot);
	pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, spot, pmq2->ps->clientNum, pmq2->tracemask);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		pmlq2.ladder = qtrue;

	// check for water jump
	if (pmq2->waterlevel != 2)
		return;

	VectorMA(pmlq2.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pmq2->pointcontents(spot, pmq2->ps->clientNum);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = pmq2->pointcontents(spot, pmq2->ps->clientNum);
	if (cont)
		return;
	// jump out of water
	VectorScale(flatforward, 50, pmlq2.velocity);
	pmlq2.velocity[2] = 350;

	pmq2->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pmq2->ps->pm_time = 255;
}


/*
===============
PMQ2_FlyMove
===============
*/
void PMQ2_FlyMove(qboolean doclip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	vec3_t		end;
	trace_t	trace;

	pmq2->ps->viewheight = 22;

	// friction

	speed = VectorLength(pmlq2.velocity);
	if (speed < 1)
	{
		VectorCopy(vec3_origin, pmlq2.velocity);
	}
	else
	{
		drop = 0;

		friction = pmq2_friction * 1.5;	// extra friction
		control = speed < pmq2_stopspeed ? pmq2_stopspeed : speed;
		drop += control * friction * pmlq2.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale(pmlq2.velocity, newspeed, pmlq2.velocity);
	}

	// accelerate
	fmove = pmlq2.forwardmove;
	smove = pmlq2.rightmove;

	VectorNormalize(pmlq2.forward);
	VectorNormalize(pmlq2.right);

	for (i = 0; i < 3; i++)
		wishvel[i] = pmlq2.forward[i] * fmove + pmlq2.right[i] * smove;
	wishvel[2] += pmlq2.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pmq2_maxspeed)
	{
		VectorScale(wishvel, pmq2_maxspeed / wishspeed, wishvel);
		wishspeed = pmq2_maxspeed;
	}

	pmq2->ps->speed = pmq2_maxspeed;

	currentspeed = DotProduct(pmlq2.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = pmq2_accelerate * pmlq2.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlq2.velocity[i] += accelspeed * wishdir[i];

	if (doclip) {
		for (i = 0; i < 3; i++)
			end[i] = pmlq2.origin[i] + pmlq2.frametime * pmlq2.velocity[i];

		pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, end, pmq2->ps->clientNum, pmq2->tracemask);

		VectorCopy(trace.endpos, pmlq2.origin);
	}
	else {
		// move
		VectorMA(pmlq2.origin, pmlq2.frametime, pmlq2.velocity, pmlq2.origin);
	}
}


/*
==============
PMQ2_CheckDuck

Sets mins, maxs, and pmq2->ps->viewheight
==============
*/
void PMQ2_CheckDuck(void)
{
	trace_t	trace;

	pmq2->mins[0] = -16;
	pmq2->mins[1] = -16;

	pmq2->maxs[0] = 16;
	pmq2->maxs[1] = 16;

	/* TA: doesnt exist in jk
	if (pmq2->ps->pm_type == PMQ2_GIB)
	{
		pmq2->mins[2] = 0;
		pmq2->maxs[2] = 16;
		pmq2->ps->viewheight = 8;
		return;
	}*/

	pmq2->mins[2] = -24;

	if (pmq2->ps->pm_type == PM_DEAD)
	{
		pmq2->ps->pm_flags |= PMF_DUCKED;
	}
	//else if (pmlq2.upmove < 0 && (pmq2->ps->pm_flags & PMF_ON_GROUND))
	else if (pmlq2.upmove < 0 && (pmq2->ps->groundEntityNum != ENTITYNUM_NONE))
	{	// duck
		pmq2->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pmq2->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pmq2->maxs[2] = 32;
			pmq2->trace(&trace, pmlq2.origin, pmq2->mins, pmq2->maxs, pmlq2.origin, pmq2->ps->clientNum, pmq2->tracemask);
			if (!trace.allsolid)
				pmq2->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pmq2->ps->pm_flags & PMF_DUCKED)
	{
		pmq2->maxs[2] = 4;
		pmq2->ps->viewheight = -2;
	}
	else
	{
		pmq2->maxs[2] = 32;
		pmq2->ps->viewheight = 22;
	}
}


/*
==============
PMQ2_DeadMove
==============
*/
void PMQ2_DeadMove(void)
{
	float	forward;

	if (pmq2->ps->groundEntityNum == ENTITYNUM_NONE)
		return;

	// extra friction

	forward = VectorLength(pmlq2.velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear(pmlq2.velocity);
	}
	else
	{
		VectorNormalize(pmlq2.velocity);
		VectorScale(pmlq2.velocity, forward, pmlq2.velocity);
	}
}


qboolean	PMQ2_GoodPosition(void)
{
	trace_t	trace;
	vec3_t	origin, end;
	int		i;

	if (pmq2->ps->pm_type == PM_SPECTATOR)
		return qtrue;

	for (i = 0; i < 3; i++)
#ifdef AUTHENTIC_Q2SNAP
		origin[i] = end[i] = pmq2->ps->origin[i] *0.125;
#else
		origin[i] = end[i] = pmq2->ps->origin[i];
#endif
	pmq2->trace(&trace, origin, pmq2->mins, pmq2->maxs, end, pmq2->ps->clientNum, pmq2->tracemask);

	return !trace.allsolid;
}

/*
================
PMQ2_SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
void PMQ2_SnapPosition(void)
{
	int		sign[3];
	int		i, j, bits;
	//short	base[3];
	float	base[3]; // this isn't 100% authentic but it should be authentic in the range of short in any case and way beyond it too for a while
	// try all single bits first
	static int jitterbits[8] = { 0,4,1,2,3,5,6,7 };

#ifndef AUTHENTIC_Q2SNAP
	return; // no need for snapping in jk2
#else

	// we don't need this in jk but we keep it for authentic feel just in case.

	// snap velocity to eigths
	for (i = 0; i < 3; i++)
		pmq2->ps->velocity[i] = (int)(pmlq2.velocity[i] * 8);

	for (i = 0; i < 3; i++)
	{
		if (pmlq2.origin[i] >= 0)
			sign[i] = 1;
		else
			sign[i] = -1;
		pmq2->ps->origin[i] = (int)(pmlq2.origin[i] * 8);
		if (pmq2->ps->origin[i] * 0.125 == pmlq2.origin[i])
			sign[i] = 0;
	}
	VectorCopy(pmq2->ps->origin, base);

	// try all combinations
	for (j = 0; j < 8; j++)
	{
		bits = jitterbits[j];
		VectorCopy(base, pmq2->ps->origin);
		for (i = 0; i < 3; i++)
			if (bits & (1 << i))
				pmq2->ps->origin[i] += sign[i];

		if (PMQ2_GoodPosition())
			return;
	}

	// go back to the last position
	VectorCopy(pmlq2.previous_origin, pmq2->ps->origin);
#ifdef CRINGY_STUCK_DEBUG
	Com_Printf ("using previous_origin\n");
#endif
#endif

	
}

#if 0
//NO LONGER USED
/*
================
PMQ2_InitialSnapPosition

================
*/
void PMQ2_InitialSnapPosition(void)
{
	int		x, y, z;
	short	base[3];

	VectorCopy(pmq2->ps->origin, base);

	for (z = 1; z >= -1; z--)
	{
		pmq2->ps->origin[2] = base[2] + z;
		for (y = 1; y >= -1; y--)
		{
			pmq2->ps->origin[1] = base[1] + y;
			for (x = 1; x >= -1; x--)
			{
				pmq2->ps->origin[0] = base[0] + x;
				if (PMQ2_GoodPosition())
				{
					pmlq2.origin[0] = pmq2->ps->origin[0] * 0.125;
					pmlq2.origin[1] = pmq2->ps->origin[1] * 0.125;
					pmlq2.origin[2] = pmq2->ps->origin[2] * 0.125;
					VectorCopy(pmq2->ps->origin, pmlq2.previous_origin);
					return;
				}
			}
		}
	}
#ifdef
	Com_DPrintf("Bad InitialSnapPosition\n");
#endif
}
#else
/*
================
PMQ2_InitialSnapPosition

================
*/
void PMQ2_InitialSnapPosition(void)
{
	int        x, y, z;
#ifdef AUTHENTIC_Q2SNAP
	float      base[3]; // we still change them to float because our origin number is a float. should still behave the same at least within the range of a short, but allows larger maps to work properly
	static float offset[3] = { 0, -1, 1 }; // should behave like integer math in smaller ranges
#else
	float      base[3];
	static float offset[3] = { 0, -0.125, 0.125 };
#endif

	VectorCopy(pmq2->ps->origin, base);

	for (z = 0; z < 3; z++) {
		pmq2->ps->origin[2] = base[2] + offset[z];
		for (y = 0; y < 3; y++) {
			pmq2->ps->origin[1] = base[1] + offset[y];
			for (x = 0; x < 3; x++) {
				pmq2->ps->origin[0] = base[0] + offset[x];
				if (PMQ2_GoodPosition()) {
#ifdef AUTHENTIC_Q2SNAP
					pmlq2.origin[0] = pmq2->ps->origin[0] * 0.125;
					pmlq2.origin[1] = pmq2->ps->origin[1] * 0.125;
					pmlq2.origin[2] = pmq2->ps->origin[2] * 0.125;
#else
					pmlq2.origin[0] = pmq2->ps->origin[0];
					pmlq2.origin[1] = pmq2->ps->origin[1];
					pmlq2.origin[2] = pmq2->ps->origin[2];
#endif
					VectorCopy(pmq2->ps->origin, pmlq2.previous_origin);
					return;
				}
			}
		}
	}
#ifdef CRINGY_STUCK_DEBUG
	Com_Printf("Bad InitialSnapPosition\n");
#endif
}

#endif

/*
================
PMQ2_ClampAngles

================
*/
void PMQ2_ClampAngles(void)
{
	short	temp;
	int		i;

	//if (pmq2->ps->pm_flags & PMF_TIME_TELEPORT)
	//{
	//	pmq2->ps->viewangles[YAW] = SHORT2ANGLE(pmq2->cmd.angles[YAW] + pmq2->ps->delta_angles[YAW]);
	//	pmq2->ps->viewangles[PITCH] = 0;
	//	pmq2->ps->viewangles[ROLL] = 0;
	//}
	//else
	{
		// circularly clamp the angles with deltas
		for (i = 0; i < 3; i++)
		{
			temp = pmq2->cmd.angles[i] + pmq2->ps->delta_angles[i];
			pmq2->ps->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pmq2->ps->viewangles[PITCH] > 89 && pmq2->ps->viewangles[PITCH] < 180)
			pmq2->ps->viewangles[PITCH] = 89;
		else if (pmq2->ps->viewangles[PITCH] < 271 && pmq2->ps->viewangles[PITCH] >= 180)
			pmq2->ps->viewangles[PITCH] = 271;
	}
	AngleVectors(pmq2->ps->viewangles, pmlq2.forward, pmlq2.right, pmlq2.up);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void PmoveQ2(pmoveq2_t* pmove)
{
	pmq2 = pmove;

	// clear results
	pmq2->numtouch = 0;
	VectorClear(pmq2->ps->viewangles);
	pmq2->ps->viewheight = 0;
	//pmq2->ps->groundEntityNum = ENTITYNUM_NONE;
	pmq2->watertype = 0;
	pmq2->waterlevel = 0;

	// clear all pmove local vars
	memset(&pmlq2, 0, sizeof(pmlq2));


#ifdef AUTHENTIC_Q2SNAP
	pmq2->ps->origin[0] = (int)(pmq2->ps->origin[0] * 8.0f);
	pmq2->ps->origin[1] = (int)(pmq2->ps->origin[1] * 8.0f);
	pmq2->ps->origin[2] = (int)(pmq2->ps->origin[2] * 8.0f);
	
	pmq2->ps->velocity[0] = (int)(pmq2->ps->velocity[0] * 8.0f);
	pmq2->ps->velocity[1] = (int)(pmq2->ps->velocity[1] * 8.0f);
	pmq2->ps->velocity[2] = (int)(pmq2->ps->velocity[2] * 8.0f);

	// convert origin and velocity to float values
	pmlq2.origin[0] = pmq2->ps->origin[0] * 0.125;
	pmlq2.origin[1] = pmq2->ps->origin[1] * 0.125;
	pmlq2.origin[2] = pmq2->ps->origin[2] * 0.125;

	pmlq2.velocity[0] = pmq2->ps->velocity[0] * 0.125;
	pmlq2.velocity[1] = pmq2->ps->velocity[1] * 0.125;
	pmlq2.velocity[2] = pmq2->ps->velocity[2] * 0.125;
#else
	// convert origin and velocity to float values
	pmlq2.origin[0] = pmq2->ps->origin[0];
	pmlq2.origin[1] = pmq2->ps->origin[1];
	pmlq2.origin[2] = pmq2->ps->origin[2];

	pmlq2.velocity[0] = pmq2->ps->velocity[0];
	pmlq2.velocity[1] = pmq2->ps->velocity[1];
	pmlq2.velocity[2] = pmq2->ps->velocity[2];
#endif

	pmlq2.msec = pmq2->cmd.serverTime - pmq2->ps->commandTime;
	pmlq2.forwardmove = (int)pmq2->cmd.forwardmove * 500 / 127;//adapt from q3 range
	pmlq2.rightmove = (int)pmq2->cmd.rightmove * 500 / 127;//adapt from q3 range
	pmlq2.upmove = (int)pmq2->cmd.upmove * 500 / 127;//adapt from q3 range

	// save old vel for corner skims (non-standard Q2 to restore originally intended behavior, controllable via pmq2->cornerSkims, default 0 = deactivated)
	VectorCopy(pmlq2.velocity, pmlq2.previous_velocity);

	// save old org in case we get stuck
	VectorCopy(pmq2->ps->origin, pmlq2.previous_origin);

	pmlq2.frametime = pmlq2.msec * 0.001;

	pmq2->ps->basespeed = pmq2->ps->speed = pmq2_maxspeed;

	PMQ2_ClampAngles();

	if (pmq2->ps->pm_type == PM_SPECTATOR)
	{
		PMQ2_FlyMove(qfalse);
#ifdef AUTHENTIC_Q2SNAP
		PMQ2_SnapPosition();
		pmq2->ps->origin[0] *= 0.125f;
		pmq2->ps->origin[1] *= 0.125f;
		pmq2->ps->origin[2] *= 0.125f;
		pmq2->ps->velocity[0] *= 0.125f;
		pmq2->ps->velocity[1] *= 0.125f;
		pmq2->ps->velocity[2] *= 0.125f;
#else 
		VectorCopy(pmlq2.origin, pmq2->ps->origin);
		VectorCopy(pmlq2.velocity, pmq2->ps->velocity);
#endif
		return;
	}

	if (pmq2->ps->pm_type >= PM_DEAD)
	{
		pmlq2.forwardmove = 0;
		pmlq2.rightmove = 0;
		pmlq2.upmove = 0;
	}

	if (pmq2->ps->pm_type == PM_FREEZE)
		return;		// no movement at all

	// set mins, maxs, and viewheight
	PMQ2_CheckDuck();

	if (pmq2->snapinitial)
		PMQ2_InitialSnapPosition();

	//pmlq2.clipped = qtrue;
	// set groundentity, watertype, and waterlevel
	PMQ2_CatagorizePosition(0);
	//pmlq2.clipped = qfalse; // dead ramp detection only after slidemove. actually nvm, this might also fix an issue where land gets detected before jump (i guess from quantizing position) and blocks rejump

	if (pmq2->ps->pm_type == PM_DEAD)
		PMQ2_DeadMove();

	PMQ2_CheckSpecialMovement();

	// drop timing counter
	if (pmq2->ps->pm_time)
	{
		int		msec;

		//msec = pmq2->cmd.msec >> 3;
		msec = pmlq2.msec >> 3; // why is this >> 3?
		if (!msec)
			msec = 1;
		if (msec >= pmq2->ps->pm_time)
		{
			pmq2->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND/* | PMF_TIME_TELEPORT*/);
			pmq2->ps->pm_time = 0;
		}
		else
			pmq2->ps->pm_time -= msec;
	}

	//if (pmq2->ps->pm_flags & PMF_TIME_TELEPORT)
	//{	// teleport pause stays exactly in place
	//}
	//else 
	if (pmq2->ps->pm_flags & PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		pmlq2.velocity[2] -= pmq2->ps->gravity * pmlq2.frametime;
		if (pmlq2.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			pmq2->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND /*| PMF_TIME_TELEPORT*/);
			pmq2->ps->pm_time = 0;
		}

		PMQ2_StepSlideMove();
	}
	else
	{
		PMQ2_CheckJump();

		PMQ2_Friction();

		if (pmq2->waterlevel >= 2)
			PMQ2_WaterMove();
		else {
			vec3_t	angles;

			VectorCopy(pmq2->ps->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors(angles, pmlq2.forward, pmlq2.right, pmlq2.up);

			PMQ2_AirMove();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PMQ2_CatagorizePosition(1);


#ifdef AUTHENTIC_Q2SNAP
	PMQ2_SnapPosition();
	pmq2->ps->origin[0] *= 0.125f;
	pmq2->ps->origin[1] *= 0.125f;
	pmq2->ps->origin[2] *= 0.125f;
	pmq2->ps->velocity[0] *= 0.125f;
	pmq2->ps->velocity[1] *= 0.125f;
	pmq2->ps->velocity[2] *= 0.125f;
#else 
	// TODO if not using authentic snap and if using q2 style trace (even if not?) we need to still nudge the final position in case its in a solid
	VectorCopy(pmlq2.origin,pmq2->ps->origin);
	VectorCopy(pmlq2.velocity,pmq2->ps->velocity);
#endif
}

