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

#include "../game/bg_pmove_css.h"
#include "../game/bg_public.h"


//#define AUTHENTIC_CSSSNAP

#define	STEPSIZE	18

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
	//trace_t		groundTrace;
	qboolean	groundFound;


	float		forwardmove, rightmove, upmove;

	vec3_t		previous_origin;
	qboolean	ladder;
} pmlcss_t;

pmovecss_t* pmcss;
pmlcss_t		pmlcss;


// movement parameters
float	pmcss_stopspeed = 100;
float	pmcss_maxspeed = 300;
float	pmcss_duckspeed = 100;
float	pmcss_accelerate = 10;
float	pmcss_airaccelerate = 0;
float	pmcss_wateraccelerate = 10;
float	pmcss_friction = 6;
float	pmcss_waterfriction = 1;
float	pmcss_waterspeed = 400;

/*

  walking up a step should kill some velocity

*/



/*
===============
PM_AddTouchEnt
===============
*/
static void PMCSS_AddTouchEnt(int entityNum) {
	int		i;

	if (entityNum == ENTITYNUM_WORLD) {
		return;
	}
	if (pmcss->numtouch == MAXTOUCH) {
		return;
	}

	// see if it is already added
	for (i = 0; i < pmcss->numtouch; i++) {
		if (pmcss->touchents[i] == entityNum) {
			return;
		}
	}

	// add it
	pmcss->touchents[pmcss->numtouch] = entityNum;
	pmcss->numtouch++;
}


/*
==================
PMCSS_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

void PMCSS_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
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
PMCSS_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
void PMCSS_StepSlideMove_(void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	int			i, j;
	trace_t	trace;
	vec3_t		end;
	float		time_left;

	numbumps = 4;

	VectorCopy(pmlcss.velocity, primal_velocity);
	numplanes = 0;

	time_left = pmlcss.frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = pmlcss.origin[i] + time_left * pmlcss.velocity[i];

		pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, end, pmcss->ps->clientNum, pmcss->tracemask);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			pmlcss.velocity[2] = 0;	// don't build up falling damage
			return;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, pmlcss.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

	   // save entity for contact
		if (pmcss->numtouch < MAXTOUCH && trace.entityNum != ENTITYNUM_NONE)
		{
			pmcss->touchents[pmcss->numtouch] = trace.entityNum;
			pmcss->numtouch++;
		}

		time_left -= time_left * trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, pmlcss.velocity);
			break;
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
			VectorCopy(pmlcss.velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			PMCSS_ClipVelocity(pmlcss.velocity, planes[0], pmlcss.velocity, 1.01);
			// rub some extra speed off on xy axis
			// not on Z, or you can scrub down walls
			pmlcss.velocity[0] *= rub;
			pmlcss.velocity[1] *= rub;
			pmlcss.velocity[2] *= rub;
		}
		else if (numplanes == 2)
		{	// go along the crease
			VectorCopy(pmlcss.velocity, dir);
			VectorNormalize(dir);
			rub = 1.0 + 0.5 * DotProduct(dir, planes[0]);

			// slide along the plane
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pmlcss.velocity);
			VectorScale(dir, d, pmlcss.velocity);

			// rub some extra speed off
			VectorScale(pmlcss.velocity, rub, pmlcss.velocity);
		}
		else
		{
			//			Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
			VectorCopy(vec3_origin, pmlcss.velocity);
			break;
		}

#else
		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i < numplanes; i++)
		{
			PMCSS_ClipVelocity(pmlcss.velocity, planes[i], pmlcss.velocity, 1.01);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(pmlcss.velocity, planes[j]) < 0)
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
				VectorCopy(vec3_origin, pmlcss.velocity);
				break;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pmlcss.velocity);
			VectorScale(dir, d, pmlcss.velocity);
		}
#endif
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(pmlcss.velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, pmlcss.velocity);
			break;
		}
	}

	if (pmcss->ps->pm_time)
	{
		VectorCopy(primal_velocity, pmlcss.velocity);
	}
}

/*
==================
PMCSS_StepSlideMove

==================
*/
void PMCSS_StepSlideMove(void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		down_dist, up_dist;
	//	vec3_t		delta;
	vec3_t		up, down;

	VectorCopy(pmlcss.origin, start_o);
	VectorCopy(pmlcss.velocity, start_v);

	PMCSS_StepSlideMove_();

	VectorCopy(pmlcss.origin, down_o);
	VectorCopy(pmlcss.velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	pmcss->trace(&trace, up, pmcss->mins, pmcss->maxs, up, pmcss->ps->clientNum, pmcss->tracemask);

	if (trace.allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy(up, pmlcss.origin);
	VectorCopy(start_v, pmlcss.velocity);

	PMCSS_StepSlideMove_();

	// push down the final amount
	VectorCopy(pmlcss.origin, down);
	down[2] -= STEPSIZE;
	pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, down, pmcss->ps->clientNum, pmcss->tracemask);
	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, pmlcss.origin);
	}

#if 0
	VectorSubtract(pmlcss.origin, up, delta);
	up_dist = DotProduct(delta, start_v);

	VectorSubtract(down_o, start_o, delta);
	down_dist = DotProduct(delta, start_v);
#else
	VectorCopy(pmlcss.origin, up);

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
		+ (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
		+ (up[1] - start_o[1]) * (up[1] - start_o[1]);
#endif

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		VectorCopy(down_o, pmlcss.origin);
		VectorCopy(down_v, pmlcss.velocity);
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pmlcss.velocity[2] = down_v[2];
}


/*
==================
PMCSS_Friction

Handles both ground friction and water friction
==================
*/
void PMCSS_Friction(void)
{
	float* vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	vel = pmlcss.velocity;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

	// apply ground friction
	if ((pmcss->ps->groundEntityNum != ENTITYNUM_NONE && pmlcss.groundFound && !(pmlcss.surfaceFlags & SURF_SLICK)) || (pmlcss.ladder))
	{
		friction = pmcss_friction;
		control = speed < pmcss_stopspeed ? pmcss_stopspeed : speed;
		drop += control * friction * pmlcss.frametime;
	}

	// apply water friction
	if (pmcss->waterlevel && !pmlcss.ladder)
		drop += speed * pmcss_waterfriction * pmcss->waterlevel * pmlcss.frametime;

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
PMCSS_Accelerate

Handles user intended acceleration
==============
*/
void PMCSS_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(pmlcss.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * pmlcss.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlcss.velocity[i] += accelspeed * wishdir[i];
}

void PMCSS_AirAccelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct(pmlcss.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * pmlcss.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlcss.velocity[i] += accelspeed * wishdir[i];
}

/*
=============
PMCSS_AddCurrents
=============
*/
void PMCSS_AddCurrents(vec3_t	wishvel)
{
	vec3_t	v;
	float	s;

	//
	// account for ladders
	//

	if (pmlcss.ladder && fabs(pmlcss.velocity[2]) <= 200)
	{
		if ((pmcss->ps->viewangles[PITCH] <= -15) && (pmlcss.forwardmove > 0))
			wishvel[2] = 200;
		else if ((pmcss->ps->viewangles[PITCH] >= 15) && (pmlcss.forwardmove > 0))
			wishvel[2] = -200;
		else if (pmlcss.upmove > 0)
			wishvel[2] = 200;
		else if (pmlcss.upmove < 0)
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
	if (pmcss->watertype & MASK_CURRENT)
	{
		VectorClear(v);

		if (pmcss->watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pmcss->watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pmcss->watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pmcss->watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pmcss->watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pmcss->watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pmcss_waterspeed;
		if ((pmcss->waterlevel == 1) && (pmcss->ps->groundEntityNum != ENTITYNUM_NONE))
			s /= 2;

		VectorMA(wishvel, s, v, wishvel);
	}*/

	//
	// add conveyor belt velocities
	//

	//if (pmcss->ps->groundEntityNum != ENTITYNUM_NONE)
	//{
	//	VectorClear(v);

	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_0)
	//		v[0] += 1;
	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_90)
	//		v[1] += 1;
	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_180)
	//		v[0] -= 1;
	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_270)
	//		v[1] -= 1;
	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_UP)
	//		v[2] += 1;
	//	if (pmlcss.groundcontents & CONTENTS_CURRENT_DOWN)
	//		v[2] -= 1;

	//	VectorMA(wishvel, 100 /* pmcss->groundentity->speed */, v, wishvel);
	//}
}


/*
===================
PMCSS_WaterMove

===================
*/
void PMCSS_WaterMove(void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

	//
	// user intentions
	//
	for (i = 0; i < 3; i++)
		wishvel[i] = pmlcss.forward[i] * pmlcss.forwardmove + pmlcss.right[i] * pmlcss.rightmove;

	if (!pmlcss.forwardmove && !pmlcss.rightmove && !pmlcss.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pmlcss.upmove;

	PMCSS_AddCurrents(wishvel);

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pmcss_maxspeed)
	{
		VectorScale(wishvel, pmcss_maxspeed / wishspeed, wishvel);
		wishspeed = pmcss_maxspeed;
	}
	wishspeed *= 0.5;

	PMCSS_Accelerate(wishdir, wishspeed, pmcss_wateraccelerate);

	PMCSS_StepSlideMove();
}


/*
===================
PMCSS_AirMove

===================
*/
void PMCSS_AirMove(void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		maxspeed;

	fmove = pmlcss.forwardmove;
	smove = pmlcss.rightmove;

	//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	pmlcss.forward[2] = 0;
	pmlcss.right[2] = 0;
	VectorNormalize(pmlcss.forward);
	VectorNormalize(pmlcss.right);
#endif

	for (i = 0; i < 2; i++)
		wishvel[i] = pmlcss.forward[i] * fmove + pmlcss.right[i] * smove;
	wishvel[2] = 0;

	PMCSS_AddCurrents(wishvel);

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	maxspeed = (pmcss->ps->pm_flags & PMF_DUCKED) ? pmcss_duckspeed : pmcss_maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale(wishvel, maxspeed / wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if (pmlcss.ladder)
	{
		PMCSS_Accelerate(wishdir, wishspeed, pmcss_accelerate);
		if (!wishvel[2])
		{
			if (pmlcss.velocity[2] > 0)
			{
				pmlcss.velocity[2] -= pmcss->ps->gravity * pmlcss.frametime;
				if (pmlcss.velocity[2] < 0)
					pmlcss.velocity[2] = 0;
			}
			else
			{
				pmlcss.velocity[2] += pmcss->ps->gravity * pmlcss.frametime;
				if (pmlcss.velocity[2] > 0)
					pmlcss.velocity[2] = 0;
			}
		}
		PMCSS_StepSlideMove();
	}
	else if (pmcss->ps->groundEntityNum != ENTITYNUM_NONE)
	{	// walking on ground
		pmlcss.velocity[2] = 0; //!!! this is before the accel
		PMCSS_Accelerate(wishdir, wishspeed, pmcss_accelerate);

		// PGM	-- fix for negative trigger_gravity fields
		//		pmlcss.velocity[2] = 0;
		if (pmcss->ps->gravity > 0)
			pmlcss.velocity[2] = 0;
		else
			pmlcss.velocity[2] -= pmcss->ps->gravity * pmlcss.frametime;
		// PGM

		if (!pmlcss.velocity[0] && !pmlcss.velocity[1])
			return;
		PMCSS_StepSlideMove();
	}
	else
	{	// not on ground, so little effect on velocity
		if (pmcss_airaccelerate)
			PMCSS_AirAccelerate(wishdir, wishspeed, pmcss_accelerate);
		else
			PMCSS_Accelerate(wishdir, wishspeed, 1);
		// add gravity
		pmlcss.velocity[2] -= pmcss->ps->gravity * pmlcss.frametime;
		PMCSS_StepSlideMove();
	}
}


void PMCSS_FancyGroundTrace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask) {
	vec3_t minsArr[4], maxsArr[4];
	vec3_t oldEndPos;
	float oldFraction = results->fraction;
	int i;
	for (i = 0; i < 4; i++) {
		VectorCopy(mins,minsArr[i]);
		VectorCopy(maxs,maxsArr[i]);
	}
	maxsArr[0][0] = MIN(0, maxsArr[0][0]);
	maxsArr[0][1] = MIN(0, maxsArr[0][1]);
	minsArr[1][0] = MAX(0, maxsArr[1][0]);
	minsArr[1][1] = MAX(0, maxsArr[1][1]);
	maxsArr[2][0] = MIN(0, maxsArr[2][0]);
	minsArr[2][1] = MAX(0, minsArr[2][1]);
	minsArr[3][0] = MAX(0, minsArr[3][0]);
	maxsArr[3][1] = MIN(0, maxsArr[3][1]);
	VectorCopy(results->endpos, oldEndPos);

	for (i = 0; i < 4; i++) {
		pm->trace(results,start,minsArr[i],maxsArr[i],end,passEntityNum,contentMask);
		if (results->entityNum != ENTITYNUM_NONE && results->plane.normal[2] >= 0.7) {
			goto done;
		}
	}

done:
	VectorCopy(oldEndPos, results->endpos);
	results->fraction = oldFraction;
	return;
}

/*
=============
PMCSS_CatagorizePosition
=============
*/
void PMCSS_CatagorizePosition(void)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;
	int			sample1;
	int			sample2;
	vec3_t		flatforward,spot;

	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid	
	point[0] = pmlcss.origin[0];
	point[1] = pmlcss.origin[1];

	//
	// get waterlevel, accounting for ducking
	//
	pmcss->waterlevel = 0;
	pmcss->watertype = 0;

	sample2 = pmcss->ps->viewheight - pmcss->mins[2];
	sample1 = sample2 / 2;

	point[2] = pmlcss.origin[2] + pmcss->mins[2] + 1;
	cont = pmcss->pointcontents(point, pmcss->ps->clientNum);

	if (cont & MASK_WATER)
	{
		pmcss->watertype = cont;
		pmcss->waterlevel = 1;
		point[2] = pmlcss.origin[2] + pmcss->mins[2] + sample1;
		cont = pmcss->pointcontents(point, pmcss->ps->clientNum);
		if (cont & MASK_WATER)
		{
			pmcss->waterlevel = 2;
			point[2] = pmlcss.origin[2] + pmcss->mins[2] + sample2;
			cont = pmcss->pointcontents(point, pmcss->ps->clientNum);
			if (cont & MASK_WATER)
				pmcss->waterlevel = 3;
		}
	}

	if (pm->ps->pm_type == PM_SPECTATOR) {
		return;
	}

	pmlcss.ladder = qfalse;

	// check for ladder
	flatforward[0] = pmlcss.forward[0];
	flatforward[1] = pmlcss.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pmlcss.origin, 1, flatforward, spot);
	pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, spot, pmcss->ps->clientNum, pmcss->tracemask);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		pmlcss.ladder = qtrue;



	point[2] = pmlcss.origin[2] - 2.0;

	if (pmlcss.velocity[2] > 140 || pmlcss.velocity[2]>0 && pmlcss.ladder) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		//pmcss->ps->pm_flags &= ~PMF_ON_GROUND;
		pmcss->ps->groundEntityNum = ENTITYNUM_NONE;
	}
	else
	{
		qboolean ground = qtrue;
		pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, point, pmcss->ps->clientNum, pmcss->tracemask);
		pmlcss.groundplane = trace.plane;
		//pmlcss.groundsurface = trace.surface;
		pmlcss.surfaceFlags = trace.surfaceFlags;
		pmlcss.groundcontents = trace.contents;
		pmlcss.groundFound = trace.entityNum != ENTITYNUM_NONE; // is this right?

		if (trace.entityNum == ENTITYNUM_NONE || trace.plane.normal[2] < 0.7)
		{
			PMCSS_FancyGroundTrace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, point, pmcss->ps->clientNum, pmcss->tracemask);
			if (trace.entityNum == ENTITYNUM_NONE || trace.plane.normal[2] < 0.7)
			{
				pmcss->ps->groundEntityNum = ENTITYNUM_NONE;
				ground = qfalse;
			}
		}
		if(ground)
		{
			int oldGroundEntityNum = pmcss->ps->groundEntityNum;
			pmcss->ps->groundEntityNum = trace.entityNum;

			// hitting solid ground will end a waterjump
			pmcss->ps->rocketLockTime = 0;

			if (pmcss->ps->groundEntityNum != ENTITYNUM_WORLD)
			{
				PMCSS_AddTouchEnt(pmcss->ps->groundEntityNum);
			}
		}

	}

	

}


/*
=============
PMCSS_CheckJump
=============
*/
void PMCSS_CheckJump(void)
{
	if (pmcss->ps->pm_flags & PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		return;
	}

	if (pmlcss.upmove < 10)
	{	// not holding jump
		pmcss->ps->pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pmcss->ps->pm_flags & PMF_JUMP_HELD)
		return;

	if (pmcss->ps->pm_type == PM_DEAD)
		return;

	if (pmcss->waterlevel >= 2)
	{	// swimming, not jumping
		pmcss->ps->groundEntityNum = ENTITYNUM_NONE;

		if (pmlcss.velocity[2] <= -300)
			return;

		if (pmcss->watertype == CONTENTS_WATER)
			pmlcss.velocity[2] = 100;
		else if (pmcss->watertype == CONTENTS_SLIME)
			pmlcss.velocity[2] = 80;
		else
			pmlcss.velocity[2] = 50;
		return;
	}

	if (pmcss->ps->groundEntityNum == ENTITYNUM_NONE)
		return;		// in air, so no effect

	pmcss->ps->pm_flags |= PMF_JUMP_HELD;

	pmcss->ps->groundEntityNum = ENTITYNUM_NONE;
	pmlcss.velocity[2] += 270;
	if (pmlcss.velocity[2] < 270)
		pmlcss.velocity[2] = 270;
}


/*
=============
PMCSS_CheckSpecialMovement
=============
*/
void PMCSS_CheckSpecialMovement(void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	trace_t	trace;

	if (pmcss->ps->pm_time)
		return;

	flatforward[0] = pmlcss.forward[0];
	flatforward[1] = pmlcss.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);


	// check for water jump
	if (pmcss->waterlevel != 2)
		return;

	VectorMA(pmlcss.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pmcss->pointcontents(spot, pmcss->ps->clientNum);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = pmcss->pointcontents(spot, pmcss->ps->clientNum);
	if (cont)
		return;
	// jump out of water
	VectorScale(flatforward, 50, pmlcss.velocity);
	pmlcss.velocity[2] = 350;

	pmcss->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pmcss->ps->pm_time = 255;
}


/*
===============
PMCSS_FlyMove
===============
*/
void PMCSS_FlyMove(qboolean doclip)
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

	pmcss->ps->viewheight = 22;

	// friction

	speed = VectorLength(pmlcss.velocity);
	if (speed < 1)
	{
		VectorCopy(vec3_origin, pmlcss.velocity);
	}
	else
	{
		drop = 0;

		friction = pmcss_friction * 1.5;	// extra friction
		control = speed < pmcss_stopspeed ? pmcss_stopspeed : speed;
		drop += control * friction * pmlcss.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale(pmlcss.velocity, newspeed, pmlcss.velocity);
	}

	// accelerate
	fmove = pmlcss.forwardmove;
	smove = pmlcss.rightmove;

	VectorNormalize(pmlcss.forward);
	VectorNormalize(pmlcss.right);

	for (i = 0; i < 3; i++)
		wishvel[i] = pmlcss.forward[i] * fmove + pmlcss.right[i] * smove;
	wishvel[2] += pmlcss.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pmcss_maxspeed)
	{
		VectorScale(wishvel, pmcss_maxspeed / wishspeed, wishvel);
		wishspeed = pmcss_maxspeed;
	}


	currentspeed = DotProduct(pmlcss.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = pmcss_accelerate * pmlcss.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmlcss.velocity[i] += accelspeed * wishdir[i];

	if (doclip) {
		for (i = 0; i < 3; i++)
			end[i] = pmlcss.origin[i] + pmlcss.frametime * pmlcss.velocity[i];

		pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, end, pmcss->ps->clientNum, pmcss->tracemask);

		VectorCopy(trace.endpos, pmlcss.origin);
	}
	else {
		// move
		VectorMA(pmlcss.origin, pmlcss.frametime, pmlcss.velocity, pmlcss.origin);
	}
}

float smooth0To1(float input) {
	return 3.0f * input * input - 2.0f * input * input * input;
}

void PMCSS_SetViewHeight(float ratio) {

	pm->ps->viewheight = 47.0f * ratio + 64.0f * (1.0f - ratio);
}


/*
==============
PMCSS_CheckDuck

Sets mins, maxs, and pmcss->ps->viewheight
==============
*/
void PMCSS_CheckDuck(void)
{
	trace_t	trace;
	int buttons = pmcss->oldbuttons ^ pmcss->cmd.buttons;
	int buttonsNew = buttons & pmcss->cmd.buttons;
	int buttonsOld = buttons & pmcss->oldbuttons;


	if (pm->ps->fd.forceRageRecoveryTime) {
		int crouchTime = 1000 - pm->ps->fd.forceRageRecoveryTime;
		if (crouchTime < 0) {
			crouchTime = 0;
		}
		if (crouchTime > 200) {
			pm->ps->fd.forceRageRecoveryTime = 0;
			PMCSS_SetViewHeight(0);
		}
		else {
			PMCSS_SetViewHeight(smooth0To1(1.0f-((float)crouchTime/200.0f)));
		}
	}

	if (pm->ps->pm_type == PM_DEAD) {
		return;
	}

	if (!pmcss->crouchSpeedReduced && (pm->ps->pm_flags & PMF_DUCKED) && pm->ps->groundEntityNum != ENTITYNUM_NONE) { // will this break with pmove_fixed?
		pmcss->cmd.forwardmove /= 3;
		pmcss->cmd.rightmove /= 3;
		pmcss->cmd.upmove /= 3;
		pmcss->crouchSpeedReduced = qtrue;
	}

	/*


	pmcss->mins[0] = -16;
	pmcss->mins[1] = -16;

	pmcss->maxs[0] = 16;
	pmcss->maxs[1] = 16;


	pmcss->mins[2] = -24;

	if (pmcss->ps->pm_type == PM_DEAD)
	{
		pmcss->ps->pm_flags |= PMF_DUCKED;
	}
	//else if (pmlcss.upmove < 0 && (pmcss->ps->pm_flags & PMF_ON_GROUND))
	else if (pmlcss.upmove < 0 && (pmcss->ps->groundEntityNum != ENTITYNUM_NONE))
	{	// duck
		pmcss->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pmcss->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pmcss->maxs[2] = 32;
			pmcss->trace(&trace, pmlcss.origin, pmcss->mins, pmcss->maxs, pmlcss.origin, pmcss->ps->clientNum, pmcss->tracemask);
			if (!trace.allsolid)
				pmcss->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pmcss->ps->pm_flags & PMF_DUCKED)
	{
		pmcss->maxs[2] = 4;
		pmcss->ps->viewheight = -2;
	}
	else
	{
		pmcss->maxs[2] = 32;
		pmcss->ps->viewheight = 22;
	}*/
}


/*
==============
PMCSS_DeadMove
==============
*/
void PMCSS_DeadMove(void)
{
	float	forward;

	if (pmcss->ps->groundEntityNum == ENTITYNUM_NONE)
		return;

	// extra friction

	forward = VectorLength(pmlcss.velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear(pmlcss.velocity);
	}
	else
	{
		VectorNormalize(pmlcss.velocity);
		VectorScale(pmlcss.velocity, forward, pmlcss.velocity);
	}
}


qboolean	PMCSS_GoodPosition(void)
{
	trace_t	trace;
	vec3_t	origin, end;
	int		i;

	if (pmcss->ps->pm_type == PM_SPECTATOR)
		return qtrue;

	for (i = 0; i < 3; i++)
		origin[i] = end[i] = pmcss->ps->origin[i];
	pmcss->trace(&trace, origin, pmcss->mins, pmcss->maxs, end, pmcss->ps->clientNum, pmcss->tracemask);

	return !trace.allsolid;
}

/*
================
PMCSS_InitialSnapPosition

================
*/
void PMCSS_InitialSnapPosition(void)
{
	int        x, y, z;
	float      base[3];
	static float offset[3] = { 0, -0.125, 0.125 };

	VectorCopy(pmcss->ps->origin, base);

	for (z = 0; z < 3; z++) {
		pmcss->ps->origin[2] = base[2] + offset[z];
		for (y = 0; y < 3; y++) {
			pmcss->ps->origin[1] = base[1] + offset[y];
			for (x = 0; x < 3; x++) {
				pmcss->ps->origin[0] = base[0] + offset[x];
				if (PMCSS_GoodPosition()) {
					pmlcss.origin[0] = pmcss->ps->origin[0];
					pmlcss.origin[1] = pmcss->ps->origin[1];
					pmlcss.origin[2] = pmcss->ps->origin[2];
					VectorCopy(pmcss->ps->origin, pmlcss.previous_origin);
					return;
				}
			}
		}
	}

	//Com_DPrintf("Bad InitialSnapPosition\n");
}


/*
================
PMCSS_ClampAngles

================
*/
void PMCSS_ClampAngles(void)
{
	short	temp;
	int		i;

	//if (pmcss->ps->pm_flags & PMF_TIME_TELEPORT)
	//{
	//	pmcss->ps->viewangles[YAW] = SHORT2ANGLE(pmcss->cmd.angles[YAW] + pmcss->ps->delta_angles[YAW]);
	//	pmcss->ps->viewangles[PITCH] = 0;
	//	pmcss->ps->viewangles[ROLL] = 0;
	//}
	//else
	{
		// circularly clamp the angles with deltas
		for (i = 0; i < 3; i++)
		{
			temp = pmcss->cmd.angles[i] + pmcss->ps->delta_angles[i];
			pmcss->ps->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pmcss->ps->viewangles[PITCH] > 89 && pmcss->ps->viewangles[PITCH] < 180)
			pmcss->ps->viewangles[PITCH] = 89;
		else if (pmcss->ps->viewangles[PITCH] < 271 && pmcss->ps->viewangles[PITCH] >= 180)
			pmcss->ps->viewangles[PITCH] = 271;
	}
	AngleVectors(pmcss->ps->viewangles, pmlcss.forward, pmlcss.right, pmlcss.up);
}


static void PMCSS_ScaleWishVel() {
	if (pmcss->ps->pm_type != PM_NOCLIP && pmcss->ps->pm_type != PM_SPECTATOR) {
		float maxSpeed = (pmcss->cmd.buttons & BUTTON_WALKING) ? 100 : 320;
		float wishSpeed = sqrtf(pmlcss.forwardmove* pmlcss.forwardmove+ pmlcss.rightmove * pmlcss.rightmove + pmlcss.upmove * pmlcss.upmove);
		if (wishSpeed > maxSpeed) {
			float ratio = maxSpeed / wishSpeed;
			pmlcss.forwardmove *= ratio;
			pmlcss.rightmove *= ratio;
			pmlcss.upmove *= ratio;
		}
	}
	if (pmcss->ps->pm_type == PM_FREEZE || pmcss->ps->pm_type == PM_DEAD) {
		pmlcss.forwardmove = 0;
		pmlcss.rightmove = 0;
		pmlcss.upmove = 0;
	}

}



/*
================
PM_DropTimers
================
*/
static void PMCSS_DropTimers(void) {

	// drop misc timing counter

	if (pmcss->ps->forceDodgeAnim) {
		if (pmlcss.msec >= pmcss->ps->forceDodgeAnim) {
			pmcss->ps->pm_time = 0;
		}
		else {
			pmcss->ps->forceDodgeAnim -= pmlcss.msec;
		}
	}
	if (pmcss->ps->fd.forceRageRecoveryTime) {
		if (pmlcss.msec >= pmcss->ps->fd.forceRageRecoveryTime) {
			pmcss->ps->pm_time = 0;
		}
		else {
			pmcss->ps->fd.forceRageRecoveryTime -= pmlcss.msec;
		}
	}
	if (pmcss->ps->fd.forcePowerDebounce[FP_LEVITATION]) {
		if (pmlcss.msec >= pmcss->ps->fd.forcePowerDebounce[FP_LEVITATION]) {
			pmcss->ps->pm_time = 0;
		}
		else {
			pmcss->ps->fd.forcePowerDebounce[FP_LEVITATION] -= pmlcss.msec;
		}
	}
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void PmoveCSS(pmovecss_t* pmove)
{
	pmcss = pmove;

	// clear results
	pmcss->numtouch = 0;
	VectorClear(pmcss->ps->viewangles);
	pmcss->ps->viewheight = 0;
	pmcss->ps->groundEntityNum = ENTITYNUM_NONE;
	pmcss->watertype = 0;
	pmcss->waterlevel = 0;

	// clear all pmove local vars
	memset(&pmlcss, 0, sizeof(pmlcss));

	pmlcss.origin[0] = pmcss->ps->origin[0];
	pmlcss.origin[1] = pmcss->ps->origin[1];
	pmlcss.origin[2] = pmcss->ps->origin[2];

	pmlcss.velocity[0] = pmcss->ps->velocity[0];
	pmlcss.velocity[1] = pmcss->ps->velocity[1];
	pmlcss.velocity[2] = pmcss->ps->velocity[2];

	pmlcss.msec = pmcss->cmd.serverTime - pmcss->ps->commandTime;
	pmlcss.forwardmove = (int)pmcss->cmd.forwardmove * 500 / 127;//adapt from q3 range
	pmlcss.rightmove = (int)pmcss->cmd.rightmove * 500 / 127;//adapt from q3 range
	pmlcss.upmove = (int)pmcss->cmd.upmove * 500 / 127;//adapt from q3 range

	PMCSS_ScaleWishVel();

	// save old org in case we get stuck
	VectorCopy(pmcss->ps->origin, pmlcss.previous_origin);

	pmlcss.frametime = pmlcss.msec * 0.001;

	PMCSS_DropTimers();

	PMCSS_ClampAngles();

	if (pmcss->ps->pm_type == PM_SPECTATOR)
	{
		PMCSS_FlyMove(qfalse);
		VectorCopy(pmlcss.origin, pmcss->ps->origin);
		VectorCopy(pmlcss.velocity, pmcss->ps->velocity);
		return;
	}

	if (pmcss->ps->pm_type >= PM_DEAD)
	{
		pmlcss.forwardmove = 0;
		pmlcss.rightmove = 0;
		pmlcss.upmove = 0;
	}

	if (pmcss->ps->pm_type == PM_FREEZE)
		return;		// no movement at all


	if (pmcss->snapinitial)
		PMCSS_InitialSnapPosition();

	// set groundentity, watertype, and waterlevel
	PMCSS_CatagorizePosition();


	// set mins, maxs, and viewheight
	PMCSS_CheckDuck();

	if (pmcss->ps->pm_type == PM_DEAD)
		PMCSS_DeadMove();

	PMCSS_CheckSpecialMovement();

	// drop timing counter
	if (pmcss->ps->pm_time)
	{
		int		msec;

		//msec = pmcss->cmd.msec >> 3;
		msec = pmlcss.msec >> 3; // why is this >> 3?
		if (!msec)
			msec = 1;
		if (msec >= pmcss->ps->pm_time)
		{
			pmcss->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND/* | PMF_TIME_TELEPORT*/);
			pmcss->ps->pm_time = 0;
		}
		else
			pmcss->ps->pm_time -= msec;
	}

	//if (pmcss->ps->pm_flags & PMF_TIME_TELEPORT)
	//{	// teleport pause stays exactly in place
	//}
	//else 
	if (pmcss->ps->pm_flags & PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		pmlcss.velocity[2] -= pmcss->ps->gravity * pmlcss.frametime;
		if (pmlcss.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			pmcss->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND /*| PMF_TIME_TELEPORT*/);
			pmcss->ps->pm_time = 0;
		}

		PMCSS_StepSlideMove();
	}
	else
	{
		PMCSS_CheckJump();

		PMCSS_Friction();

		if (pmcss->waterlevel >= 2)
			PMCSS_WaterMove();
		else {
			vec3_t	angles;

			VectorCopy(pmcss->ps->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors(angles, pmlcss.forward, pmlcss.right, pmlcss.up);

			PMCSS_AirMove();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PMCSS_CatagorizePosition();

	VectorCopy(pmlcss.origin, pmcss->ps->origin);
	VectorCopy(pmlcss.velocity, pmcss->ps->velocity);
}

