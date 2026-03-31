#include "g_local.h"

// Antiwallhack ported from Bucky's vvv-serverside
// https://github.com/Bucky21659/vVv-serverside/tree/antiwallhack

#if _ANTIWALLHACK
static const float maxJediMasterDistance = 2500.0f * 2500.0f; // x^2, optimisation
static const float maxJediMasterFOV = 100.0f;
static const float maxForceSightDistance = Square(1500.0f) * 1500.0f; // x^2, optimisation
static const float maxForceSightFOV = 100.0f;
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

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


static qboolean SE_RenderIsVisible( const gentity_t *self, const vec3_t startPos, const vec3_t testOrigin,
	qboolean reversedCheck )
{
	trace_t results;

	trap_Trace( &results, startPos, NULL, NULL, testOrigin, self - g_entities, MASK_SOLID );

	if ( results.fraction < 1.0f ) {
		if ( (results.surfaceFlags & SURF_FORCEFIELD)
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_GLASS
			|| (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_SHATTERGLASS )
		{
			//FIXME: This is a quick hack to render people and things through glass and force fields, but will also take
			//	effect even if there is another wall between them (and double glass) - which is bad, of course, but
			//	nothing i can prevent right now.
			if ( reversedCheck || SE_RenderIsVisible( self, testOrigin, startPos, qtrue ) ) {
				return qtrue;
			}
		}

		return qfalse;
	}

	return qtrue;
}

static qboolean SE_RenderPlayerChecks( const gentity_t *self, const vec3_t playerOrigin, vec3_t playerPoints[9] ) {
	trace_t results;
	int i;

	for ( i = 0; i < 9; i++ ) {
		if ( trap_PointContents( playerPoints[i], self - g_entities ) == CONTENTS_SOLID ) {
			trap_Trace( &results, playerOrigin, NULL, NULL, playerPoints[i], self - g_entities, MASK_SOLID );
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
	vec3_t playerPoints[9] )
{
	int isHeight = isCrouching ? 32 : 56;
	vec3_t	forward, right, up;

	AngleVectors( playerAngles, forward, right, up );

	VectorMA( playerOrigin, 32.0f, up, playerPoints[0] );
	VectorMA( playerOrigin, 64.0f, forward, playerPoints[1] );
	VectorMA( playerPoints[1], 64.0f, right, playerPoints[1] );
	VectorMA( playerOrigin, 64.0f, forward, playerPoints[2] );
	VectorMA( playerPoints[2], -64.0f, right, playerPoints[2] );
	VectorMA( playerOrigin, -64.0f, forward, playerPoints[3] );
	VectorMA( playerPoints[3], 64.0f, right, playerPoints[3] );
	VectorMA( playerOrigin, -64.0f, forward, playerPoints[4] );
	VectorMA( playerPoints[4], -64.0f, right, playerPoints[4] );

	VectorMA( playerPoints[1], isHeight, up, playerPoints[5] );
	VectorMA( playerPoints[2], isHeight, up, playerPoints[6] );
	VectorMA( playerPoints[3], isHeight, up, playerPoints[7] );
	VectorMA( playerPoints[4], isHeight, up, playerPoints[8] );

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

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
static qboolean SE_NetworkPlayer( const gentity_t *self, const gentity_t *other ) {
	int i,  contents;
	vec3_t firstPersonPos, thirdPersonPos, targPos[9];

	GetCameraPosition(self, thirdPersonPos);

	VectorCopy( self->client->ps.origin, firstPersonPos );
	firstPersonPos[2] += 24;
	if (SE_IsPlayerCrouching(self))
		firstPersonPos[2] -= 32;

	contents = trap_PointContents( firstPersonPos, self - g_entities );

	// translucent, we should probably just network them anyways
	if ( contents & (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME) ) {
		return qtrue;
	}

	// entirely in an opaque surface, no point networking them.
	if ( contents & (CONTENTS_SOLID | CONTENTS_TERRAIN | CONTENTS_OPAQUE) ) {
#ifdef _DEBUG
		if ( self->s.number == 0 ) {
			Com_Printf( "WALLHACK[%i]: inside opaque surface\n", level.time );
		}
#endif // _DEBUG
		return qfalse;
	}

	// plot their bbox pointer into targPos[]
	SE_RenderPlayerPoints( SE_IsPlayerCrouching( other ), other->client->ps.viewangles, other->client->ps.origin,
		targPos );
	SE_RenderPlayerChecks( self, other->client->ps.origin, targPos );

	for ( i = 0; i < 9; i++ ) {

		if (g_antiWallhack.integer > 1) {
			int offset = g_antiWallhack.integer - 2;

			if (offset < 0)
				offset = 0;
			if (offset > 8)
				offset = 8;

			G_TestLine(thirdPersonPos, targPos[offset], 0x0000ff, 200); //check trace.fraction? ehh trace.startsolid or whatever?
		}

		if ( SE_RenderIsVisible( self, thirdPersonPos, targPos[i], qfalse ) ) {
			return qtrue;
		}
		if ( SE_RenderIsVisible( self, firstPersonPos, targPos[i], qfalse ) ) {
			return qtrue;
		}
	}

#ifdef _DEBUG
	if ( self->s.number == 0 ) {
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

	//if (!g_antiWallhack.integer) {
	//	// Clear all the broadcast bits for this client
	//	memset ( self->r.broadcastClients, 0, sizeof ( self->r.broadcastClients ) );

	//	// The jedi master is broadcast to everyone in range
	//	G_UpdateJediMasterBroadcasts ( self );

	//	// Anyone with force sight on should see this client
	//	G_UpdateForceSightBroadcasts ( self );
	//	return;
	//}

	// we are always sent to ourselves
	// we are always sent to other clients if we are in their PVS
	// if we are not in their PVS, we must set the broadcastClients bit field
	// if we do not wish to be sent to any particular entity, we must set the broadcastClients bit field and the
	//	SVF_BROADCASTCLIENTS bit flag
	//self->r.broadcastClients[0] = 0u;
	//self->r.broadcastClients[1] = 0u;

	for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
		qboolean send = qfalse;
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

		if (g_removeSpectatorPortals.integer && other->client->sess.sessionTeam == TEAM_SPECTATOR) {
			send = qtrue;
		}
		else if (g_antiWallhack.integer) {//other->client->ps.duelInProgress && self->client->ps.duelInProgress && other->client->ps.duelIndex == self->client->ps.clientNum && self->client->ps.duelIndex == other->client->ps.clientNum) {
			//Com_Printf("g_antiWallhack %i\n", g_antiWallhack.integer);
			if (G_EntityOccluded(self, other)) {
				other->r.svFlags |= SVF_NOTSINGLECLIENT;
				other->r.singleClient = self->client->ps.clientNum;
				continue;
			}
			else {
				other->r.svFlags &= ~SVF_NOTSINGLECLIENT;
				send = qtrue;
			}
		}
		else if (!g_antiWallhack.integer) {
			send = qtrue;
		}
		else
		{
			VectorSubtract(self->client->ps.origin, other->client->ps.origin, angles);
			dist = VectorLengthSquared(angles);
			vectoangles(angles, angles);

			// broadcast jedi master to everyone if we are in distance/field of view
			if (g_gametype.integer == GT_JEDIMASTER && self->client->ps.isJediMaster) {
				if (dist < maxJediMasterDistance
					&& InFieldOfVision(other->client->ps.viewangles, maxJediMasterFOV, angles))
				{
					send = qtrue;
				}
			}

			// broadcast this client to everyone using force sight if we are in distance/field of view
			if ((other->client->ps.fd.forcePowersActive & (1 << FP_SEE))) {
				if (dist < maxForceSightDistance
					&& InFieldOfVision(other->client->ps.viewangles, maxForceSightFOV, angles))
				{
					send = qtrue;
				}
			}
		}

		/*if ( send ) {
			self->r.broadcastClients[i / 32] |= (1 << (i % 32));
			//Q_AddToBitflags( self->r.broadcastClients, i, 32 );
		}*/
		mv_entities[self->s.number].snapshotIgnore[other->s.number] = !send; //?
	}

	trap_LinkEntity( self );
}
#endif
