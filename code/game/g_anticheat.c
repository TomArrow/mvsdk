#include "g_local.h"

// Antiwallhack ported from Bucky's vvv-serverside
// https://github.com/Bucky21659/vVv-serverside/tree/antiwallhack

static const float maxJediMasterDistance = 2500.0f * 2500.0f; // x^2, optimisation
static const float maxJediMasterFOV = 100.0f;
static const float maxForceSightDistance = Square(1500.0f) * 1500.0f; // x^2, optimisation
static const float maxForceSightFOV = 100.0f;

antiWallhackDebug_t antiWhDebug = {NULL};

/*
static float VectorAngle( const vec3_t a, const vec3_t b ) {
	const float lA = VectorLength( a );
	const float lB = VectorLength( b );
	const float lAB = lA * lB;

	if ( lAB == 0.0f ) {
		return 0.0f;
	}
	else {
		return (float)(acosf( DotProduct( a, b ) / lAB ) * (180.f / M_PI));
	}
}

static void MakeVector( const vec3_t ain, vec3_t vout ) {
	float pitch, yaw, tmp;

	pitch = (float)(ain[PITCH] * M_PI / 180.0f);
	yaw = (float)(ain[YAW] * M_PI / 180.0f);
	tmp = (float)cosf( pitch );

	vout[1] = (float)(-tmp * -cosf( yaw ));
	vout[2] = (float)(sinf( yaw )*tmp);
	vout[3] = (float)-sinf( pitch );
}
*/
static int G_AntiWH_PointContents(vec3_t pos, int passEntityNum) {
	antiWhDebug.pointContentsDone++;
	if (g_antiWallhackFast.integer >= 2 && (coolApi & COOL_APIFEATURE_FASTHULLTRACE)) {
		return trap_G_COOL_API_PointContentsHullFast(pos);
	}
	else {
		return trap_PointContents(pos, passEntityNum);
	}
}

static qboolean SE_RenderIsVisible( const gentity_t *self, const vec3_t startPos, const vec3_t testOrigin,
	qboolean reversedCheck, int traceCustomFlags )
{
	trace_t results;

	JP_TraceBenchmarked( &results, startPos, NULL, NULL, testOrigin, self - g_entities, MASK_SOLID, traceCustomFlags);

	antiWhDebug.tracesDone++;

	if ( results.fraction < 1.0f ) {
		if ( (results.surfaceFlags & SURF_FORCEFIELD)
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_GLASS
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_SHATTERGLASS )
		{
			//FIXME: This is a quick hack to render people and things through glass and force fields, but will also take
			//	effect even if there is another wall between them (and double glass) - which is bad, of course, but
			//	nothing i can prevent right now.
			if ( reversedCheck || SE_RenderIsVisible( self, testOrigin, startPos, qtrue, traceCustomFlags ) ) {
				return qtrue;
			}
		}

		return qfalse;
	}

	return qtrue;
}

static qboolean SE_RenderPlayerChecks( const gentity_t *self, const vec3_t playerOrigin, vec3_t playerPoints[9], int traceCustomFlags) {
	trace_t results;
	int i;

	for ( i = 0; i < 9; i++ ) {
		if (G_AntiWH_PointContents( playerPoints[i], self - g_entities ) & CONTENTS_SOLID ) {
			JP_TraceBenchmarked( &results, playerOrigin, NULL, NULL, playerPoints[i], self - g_entities, MASK_SOLID, traceCustomFlags);
			antiWhDebug.tracesDone++;
			VectorCopy( results.endpos, playerPoints[i] );
		}
	}

	return qtrue;
}


static qboolean SE_IsPlayerCrouching( const gentity_t *ent ) {
	const playerState_t *ps = &ent->client->ps;

	// FIXME: This is no proper way to determine if a client is actually in a crouch position, we want to do this in
	//	order to properly hide a client from the enemy when he is crouching behind an obstacle and could not possibly
	//	be seen.

	if ( !ent->inuse || !ps ) {
		return qfalse;
	}

	if ( ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ) {
		return qtrue;
	}

	if ( ps->pm_flags & PMF_DUCKED ) {
		return qtrue;
	}

	switch ( ps->legsAnim ) {
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	/*case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:*/
		return qtrue;
	default:
		break;
	}

	switch ( ps->torsoAnim ) {
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	/*case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:*/
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

static qboolean SE_RenderPlayerPoints( qboolean isCrouching, const vec3_t playerAngles, const vec3_t playerOrigin,
	vec3_t playerPoints[9], vec3_t playerPointsCenter, vec3_t playerPointsMins, vec3_t playerPointsMaxs )
{
	int isHeight = isCrouching ? 32 : 56;
	vec3_t	forward = { 1,0,0 }, right = { 0,1,0 }, up = { 0,0,1 }; // TA: why do we care about his orientation? his orientation doesnt matter as to whether he's visible. this just causes ppl to randomly phase in and out of visibility based on their own rotation

	//AngleVectors( playerAngles, forward, right, up ); // see comment about forward, right, up

	VectorMA( playerOrigin, 32.0f, up, playerPoints[0] );
	VectorMA( playerOrigin, g_antiWallhackBoxSize.value, forward, playerPoints[1] );
	VectorMA( playerPoints[1], g_antiWallhackBoxSize.value, right, playerPoints[1] );
	VectorMA( playerOrigin, g_antiWallhackBoxSize.value, forward, playerPoints[2] );
	VectorMA( playerPoints[2], -g_antiWallhackBoxSize.value, right, playerPoints[2] );
	VectorMA( playerOrigin, -g_antiWallhackBoxSize.value, forward, playerPoints[3] );
	VectorMA( playerPoints[3], g_antiWallhackBoxSize.value, right, playerPoints[3] );
	VectorMA( playerOrigin, -g_antiWallhackBoxSize.value, forward, playerPoints[4] );
	VectorMA( playerPoints[4], -g_antiWallhackBoxSize.value, right, playerPoints[4] );

	VectorMA( playerPoints[1], isHeight, up, playerPoints[5] );
	VectorMA( playerPoints[2], isHeight, up, playerPoints[6] );
	VectorMA( playerPoints[3], isHeight, up, playerPoints[7] );
	VectorMA( playerPoints[4], isHeight, up, playerPoints[8] );

	VectorMA( playerOrigin, isHeight * 0.5f, up, playerPointsCenter);
	VectorSet(playerPointsMins, -g_antiWallhackBoxSize.value, -g_antiWallhackBoxSize.value, -0.5f*isHeight);
	VectorSet(playerPointsMaxs, g_antiWallhackBoxSize.value, g_antiWallhackBoxSize.value, 0.5f*isHeight);

	return qtrue;
}

static void GetCameraPosition(const gentity_t *self, vec3_t cameraOrigin) {
	vec3_t forward;
	int thirdPersonRange = 80, thirdPersonVertOffset = 16;
	// int thirdPerson = 1;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	//Lets see if they have japro, then get the thirdpersonvertoffset and thirdpersonrange.  otherwise just use defaults of 16 and 80.
	/*if (self->client->pers.isJAPRO) {
		// thirdPerson = self->client->pers.thirdPerson;
		thirdPersonRange = self->client->pers.thirdPersonRange;
		thirdPersonVertOffset = self->client->pers.thirdPersonVertOffset;
	}*/

	//Get third person position.  
	VectorCopy( self->client->ps.origin, cameraOrigin );
	VectorMA( cameraOrigin, -thirdPersonRange, forward, cameraOrigin );
	cameraOrigin[2] += 24 + thirdPersonVertOffset;

	if (SE_IsPlayerCrouching(self))
		cameraOrigin[2] -= 32;
}

static qboolean SE_NetworkPlayer( const gentity_t *self, const gentity_t *other ) {
	int i,  contents;
	vec3_t firstPersonPos, thirdPersonPos;// , targPos[9], targetCenter, targetMins, targetMaxs;
	int whVal = g_antiWallhack.integer < 0 ? -g_antiWallhack.integer : g_antiWallhack.integer;
	int preTraceFlags = 0, traceFlags = 0;
	qboolean recalcTargetBox= qtrue;
	qboolean isCrouching;

	if (g_antiWallhackFast.integer == 1 && (coolApi & COOL_APIFEATURE_PRETRACE_TRACE)) {
		preTraceFlags = TRACECUSTOMFLAG_MARKBRUSHES, traceFlags = TRACECUSTOMFLAG_WALKBRUSHES;
	}
	if (g_antiWallhackFast.integer >= 2 && (coolApi & COOL_APIFEATURE_FASTHULLTRACE)) {
		preTraceFlags = 0;
		traceFlags = TRACECUSTOMFLAG_FASTHULLTRACE;
		if (g_antiWallhackFast.integer == 3) {
			traceFlags |= TRACECUSTOMFLAG_SKIPENTITYTRACE;
		}
	}

	if (g_antiWallhackVisibleRecalcDelay.integer && other->client->antiwh.visibleTo[self - g_entities] && (level.time - g_antiWallhackVisibleRecalcDelay.integer) < other->client->antiwh.visibleToLastCheck[self - g_entities]) {
		// player was visible less than 0.15s ago. just don't bother rechecking. who cares.
		return qtrue;
	}

	if (!trap_InPVS(self->r.currentOrigin,other->r.currentOrigin)) { // not in PVS. ignore.
		return qfalse;
	}

	GetCameraPosition(self, thirdPersonPos);

	VectorCopy( self->client->ps.origin, firstPersonPos );
	firstPersonPos[2] += 24;
	if (SE_IsPlayerCrouching(self))
		firstPersonPos[2] -= 32;

	contents = G_AntiWH_PointContents( firstPersonPos, self - g_entities );

	// translucent, we should probably just network them anyways
	if ( contents & (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME) ) {
		return qtrue;
	}

	// entirely in an opaque surface, no point networking them.
	if ( contents & (CONTENTS_SOLID | CONTENTS_TERRAIN | CONTENTS_OPAQUE) ) {
#ifdef _DEBUG
		if ( self->s.number == 0 && g_antiWallhack.integer < 0) {
			Com_Printf( "WALLHACK[%i]: inside opaque surface\n", level.time );
		}
#endif // _DEBUG
		return qfalse;
	}


	isCrouching = SE_IsPlayerCrouching(other);
	if (g_antiWallhackRecalcOffset.value > 0.0f && other->client->antiwh.boxCreated && isCrouching == other->client->antiwh.crouching) {
		// already have a box, check if we can reuse
		vec3_t moveOffset;
		VectorSubtract(other->client->ps.origin, other->client->antiwh.origin, moveOffset);
		if (VectorLengthSquared(moveOffset) < g_antiWallhackRecalcOffset.value * g_antiWallhackRecalcOffset.value) {
			recalcTargetBox = qfalse;
		}
	}

	if (recalcTargetBox) {
		// plot their bbox pointer into targPos[]
		other->client->antiwh.crouching = isCrouching;
		VectorCopy(other->client->ps.origin, other->client->antiwh.origin);
		SE_RenderPlayerPoints(other->client->antiwh.crouching, other->client->ps.viewangles, other->client->antiwh.origin,
			other->client->antiwh.box, other->client->antiwh.boxCenter, other->client->antiwh.boxMins, other->client->antiwh.boxMaxs);
		other->client->antiwh.boxCreated = qtrue;
	}

	if (preTraceFlags) {
		trace_t pretrace;
		//JP_TraceBenchmarked(&pretrace, targetCenter, targetMins, targetMaxs, targetCenter, self - g_entities, MASK_SOLID, preTraceFlags);
		//antiWhDebug.tracesDone++;
		JP_TraceBenchmarked(&pretrace, thirdPersonPos, other->client->antiwh.boxMins, other->client->antiwh.boxMaxs, other->client->antiwh.boxCenter, self - g_entities, MASK_SOLID, preTraceFlags);
		antiWhDebug.tracesDone++;
	}

	if (recalcTargetBox) {
		SE_RenderPlayerChecks( self, other->client->ps.origin, other->client->antiwh.box, traceFlags );
	}

	//if (preTraceFlags) {
	//	trace_t pretrace;
	//	JP_TraceBenchmarked(&pretrace, thirdPersonPos, targetMins, targetMaxs, targetCenter, self - g_entities, MASK_SOLID, //preTraceFlags);
	//	antiWhDebug.tracesDone++;
	//}

	other->client->antiwh.visibleTo[self - g_entities] = qfalse;
	other->client->antiwh.visibleToLastCheck[self - g_entities] = level.time;
	for ( i = 0; i < 9; i++ ) {

		if (whVal > 1) {
			int offset = whVal - 2;

			if (offset < 0)
				offset = 0;
			if (offset > 8)
				offset = 8;

			G_TestLine(thirdPersonPos, other->client->antiwh.box[offset], 0x0000ff, 200); //check trace.fraction? ehh trace.startsolid or whatever?
		}

		if ( SE_RenderIsVisible( self, thirdPersonPos, other->client->antiwh.box[i], qfalse, traceFlags) ) {
			other->client->antiwh.visibleTo[self - g_entities] = qtrue;
			return qtrue;
		}
		if ( SE_RenderIsVisible( self, firstPersonPos, other->client->antiwh.box[i], qfalse, traceFlags) ) {
			other->client->antiwh.visibleTo[self - g_entities] = qtrue;
			return qtrue;
		}
	}

#ifdef _DEBUG
	if ( self->s.number == 0 && g_antiWallhack.integer < 0) {
		Com_Printf( "WALLHACK[%i]: not visible\n", level.time );
	}
#endif // _DEBUG

	return qfalse;
}

/*
static qboolean SE_RenderInFOV( const gentity_t *self, const vec3_t testOrigin ) {
	const float fov = 110.0f;
	vec3_t	tmp, aim, view;

	VectorCopy( self->client->ps.origin, tmp );
	VectorSubtract( testOrigin, tmp, aim );
	MakeVector( self->client->ps.viewangles, view );

	// don't network if they're not in our field of view
	//TODO: only skip if they haven't been in our field of view for ~500ms to avoid flickering
	//TODO: also check distance, factoring in delta angle
	if ( VectorAngle( view, aim ) > (fov / 1.2f) ) {
#ifdef _DEBUG
		if ( self->s.number == 0 ) {
			trap_Print( "WALLHACK[%i]: not in field of view\n", level.time );
		}
#endif // _DEBUG
		return qfalse;
	}

	return qtrue;
}
*/

// Tracing non-players seems to have a bad effect, we know players are limited to 32 per frame, however other gentities
//	that are being added are not! It's stupid to actually add traces for it, even with a limited form i used before of 2
//	traces per object. There are to many too track and simply networking them takes less FPS either way
qboolean G_EntityOccluded( const gentity_t *self, const gentity_t *other ) {
	// This is a non-player object, just send it (see above).
	if ( !other->inuse || other->s.number >= level.maxclients ) {
		return qtrue;
	}

	// If this player is me, or my spectee, we will always draw and don't trace.
	if ( self == other ) {
		return qtrue;
	}

	if ( self->client->ps.zoomMode ) { // 0.0
		return qtrue;
	}

	/*
	// Not rendering; this player is not in our FOV.
	if ( !SE_RenderInFOV( self, other->client->ps.origin ) ) {
		Com_Printf("NOT FOV");
		return qtrue;
	}
	*/

	// Not rendering; this player's traces did not appear in my screen.
	if ( !SE_NetworkPlayer( self, other ) ) {
		return qtrue;
	}

	return qfalse;
}

void G_UpdateClientBroadcastsAntiWallhack( gentity_t *self ) {
	int i;
	gentity_t *other;
	if (level.debugState.debug == DEBUG_ANTIWALLHACK) {
		if (level.time != antiWhDebug.lastServerTime) {
			int delta = level.time - antiWhDebug.lastServerTime;
			float tracesPerSecond = 1000.0f*(float)antiWhDebug.tracesDone/(float)delta;
			float pointContentsPerSecond = 1000.0f*(float)antiWhDebug.pointContentsDone /(float)delta;
			float timeSpentTraces = G_COOL_API_Benchmark(BENCHMARK_GETCLEARMEASUREMENT | BENCHMARK_MEASURE_VMTARGET_GAME | BENCHMARK_MEASURE_TRACES_MARKED, 0,0,0, NULL, 0);
			float tracesPerSecondSpeed = 1000.0f * (float)antiWhDebug.tracesDone / timeSpentTraces;
			G_SetDebugVar(antiWhDebug.tracesPerSecondCountFloat,0,tracesPerSecond);
			G_SetDebugVar(antiWhDebug.tracesPerSecondSpeedFloat,0, tracesPerSecondSpeed);
			G_SetDebugVar(antiWhDebug.pointContentsPerSecondCountFloat,0, pointContentsPerSecond);
			antiWhDebug.tracesDone = 0;
			antiWhDebug.pointContentsDone = 0;
			antiWhDebug.lastServerTime = level.time;
		}
	}

	// we are always sent to ourselves
	// we are always sent to other clients if we are in their PVS
	// if we are not in their PVS, we must set the broadcastClients bit field
	// if we do not wish to be sent to any particular entity, we must set the ignoreClient array in the mv entity

	for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
		int send = 0; // 0 = let server handle vis. 1 = force send. 2 = 
		float dist;
		vec3_t angles;

		if (!other->inuse || other->client->pers.connected != CON_CONNECTED) {
			// no need to compute visibility for non-connected clients
			continue;
		}

		if ( other == self ) {
			// we are always sent to ourselves anyway, this is purely an optimisation
			continue;
		}

		if (other->client->sess.sessionTeam == TEAM_SPECTATOR) {
			send = g_specAllEnts.integer ? 1 : 0;
		}
		else {
			if (G_EntityOccluded(other, self)) { // TA: Needed to flip the 2 arguments. We're checking if he can see us, not if we can see him.
				send = -1;
			}
			else {
				send = g_antiWallhackEnforceVis.integer ? 1 : 0;
			}
		}

		// TODO Get some global concept having an overview where this is all used including snapshothacking, so we don't get confused or create conflicts
		if (coolApi & COOL_APIFEATURE_MVSHAREDENTITY_REALCLIENTS) {
			mv_entities[self->s.number].snapshotIgnoreRealClient[other->s.number] = send < 0;
			mv_entities[self->s.number].snapshotEnforceRealClient[other->s.number] = send > 0;
		}
		else {
			mv_entities[self->s.number].snapshotIgnore[other->s.number] = send < 0;
			mv_entities[self->s.number].snapshotEnforce[other->s.number] = send > 0;
		}
	}
}

void G_ClearAllAntiWallhackSendStates() {
	int i;
	for (i = 0; i < level.maxclients; i++) {
		memset(mv_entities[i].snapshotIgnoreRealClient,0,sizeof(mv_entities[i].snapshotIgnore));
		memset(mv_entities[i].snapshotEnforceRealClient,0,sizeof(mv_entities[i].snapshotEnforce));
		memset(mv_entities[i].snapshotIgnore,0,sizeof(mv_entities[i].snapshotIgnore));
		memset(mv_entities[i].snapshotEnforce,0,sizeof(mv_entities[i].snapshotEnforce));
	}
}

