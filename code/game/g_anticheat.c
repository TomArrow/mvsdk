#include "g_local.h"

// Antiwallhack ported from Bucky's vvv-serverside
// https://github.com/Bucky21659/vVv-serverside/tree/antiwallhack

static const float maxJediMasterDistance = 2500.0f * 2500.0f; // x^2, optimisation
static const float maxJediMasterFOV = 100.0f;
static const float maxForceSightDistance = Square(1500.0f) * 1500.0f; // x^2, optimisation
static const float maxForceSightFOV = 100.0f;

antiWallhackDebug_t antiWhDebug = {NULL};

static const int	altPlayerOriginIndexes = ANTIWH_ALTORIGIN_POINTMASK;
static const int	dirCheckIndexes = ANTIWH_DIRCHECK_POINTMASK;

static vec3_t	boxDirs[9] = { {0,0,0},
	{1,1,0}, // forward + right
	{1,-1,0}, // forward - right
	{-1,-1,0}, // - forward - right
	{-1,1,0}, // - forward + right
	{1,1,1}, // forward + right + up
	{1,-1,1}, // forward - right + up
	{-1,-1,1}, // - forward - right + up
	{-1,1,1}, // - forward + right + up
};

static vec3_t boxToOriginDirs[9] = { 0 };
static qboolean boxToOriginDirsInited = qfalse;

#define	CM_SURFACE_CLIP_EPSILON	0.125f

const char* awhHiddenReasonStrings[AWHFAIL_COUNT_REASONS] = {
	"no path found",
	"viewer in surface",
	"not in pvs",
	"skipping impossible viewerpoint",
	"wallblocked viewerpoint plane skip",
	"(hard)",
	"wallblocked vieweepoint plane skip",
	"(hard)",
	"viewerpoint direction check skip",
	"trace fail",
};

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

// heavy duty testline reusage logic
static gentity_t* G_GetFreshTestLine(vec3_t origin,gentity_t* parent) {
	gentity_t* ent;
	qboolean eventflip = qfalse;
	vec3_t snapped;
	int i;
	int flipDelay = 1000 / g_sv_fps.integer;
	ent = &g_entities[0];
	for (i = level.maxclients; i < level.num_entities; i++, ent++) {
		if (!ent->client) {
			// clients have their own handling
			memset(ent->r.broadcastClients, 0, sizeof(ent->r.broadcastClients));
		}
		if (!ent->inuse) {
			continue;
		}
		if (!(ent->s.eType == ET_BEAM && ent->parent == parent && ent->freeAfterEvent && ent->s.generic1 == 4)) {
			continue;
		}
		if ((level.time-ent->eventTime) < 100) {
			continue;
		}
		// reuse the laserpointer logic, why not.
		eventflip = (level.time >= ent->laserPointerLastEventFlip + flipDelay) || level.time < ent->laserPointerLastEventFlip;
		if (eventflip || !ent->s.event) {
			G_AddEvent(ent, EV_TESTLINE, 0);
			ent->laserPointerLastEventFlip = level.time;
		}
		VectorCopy(origin, snapped);
		SnapVector(snapped);		// save network bandwidth
		G_SetOrigin(ent, snapped);
		trap_LinkEntity(ent);
		ent->eventTime = level.time;
		ent->s.time2 = flipDelay * 4;
		return ent;
	}
	// didnt find any. make new one.
	ent = G_TempEntity(origin,0);
	ent->s.eType = ET_BEAM;
	G_AddEvent(ent, EV_TESTLINE, 0);
	ent->s.generic1 = 4;
	ent->parent = parent;
	ent->s.time2 = flipDelay * 4;
	ent->eventTime = level.time;
	return ent;
}

// avoids too many testlines leading to them just all disappearing by reusing existing ones and flipping the event instead of using EV_EVENT
gentity_t* G_TestLineBetter(gentity_t* parent, vec3_t start, vec3_t end, int color, int time)
{
	gentity_t* te;

	te = G_GetFreshTestLine(start, parent);
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->r.svFlags |= SVF_BROADCAST;

	return te;
}

static int G_AntiWH_PointContents(vec3_t pos, int passEntityNum) {
	antiWhDebug.pointContentsDone++;
	if (g_antiWallhackFast.integer >= 2 && (coolApi & COOL_APIFEATURE_FASTHULLTRACE)) {
		return trap_G_COOL_API_PointContentsHullFast(pos);
	}
	else {
		return trap_PointContents(pos, passEntityNum);
	}
}
static int G_AntiWH_InPVS(vec3_t pos, gentity_t* ent, vec3_t pos2Fallback) {
	if (!g_antiWallhackFastPVS.integer && (coolApi & COOL_APIFEATURE_INPVSENTITY)) {
		return trap_G_COOL_API_InPVSEntity(pos,ent);
	}
	else {
		return trap_InPVS(pos, pos2Fallback);
	}
}

static qboolean SE_RenderIsVisible( gentity_t *self, const vec3_t startPos, const vec3_t testOrigin,
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


// this updates the actual index for a box index for both viewer and viewee box at the same time.
// why? because the first ANTIWH_BOX_BASESIZE entries 
static qboolean SE_CheckBoxIndex( antiWallhackPlayerData_t* awh, int index, int passEntityNum, int traceCustomFlags) {
	trace_t results;
	int indexBit = 1 << index;
	awhBoxPt_t* end;
	awhBoxPt_t* otherEnd = NULL;
	qboolean inSolid;
	qboolean boxChanged = qfalse;

	if (index == ANTIWH_WIDEBOX_FIRSTPERSONPOS) {
		return boxChanged;
	} else if (awh->boxCreatedBitmask & indexBit) {
		return boxChanged;
	}

	end = (awh->viewerBoxSize || index >= ANTIWH_BOX_BASESIZE) ? &awh->viewerBox[index] : &awh->box[index];

	// TA: honestly the passentitynum here is a bit dumb... players cannot be CONTENTS_SOLID, only CONTENTS_BODY anyway but OH WELL
	inSolid = G_AntiWH_PointContents(end->pos, passEntityNum) & CONTENTS_SOLID;

	otherEnd = index < ANTIWH_BOX_BASESIZE && awh->viewerBoxSize ? &awh->box[index] : NULL;
	if (!inSolid && otherEnd) {
		// since the point contents only check one point and the viewer box (when active) is far wider, we still need to do the closer check
		inSolid = G_AntiWH_PointContents(otherEnd->pos, passEntityNum) & CONTENTS_SOLID;
	}

	end->wallblocked = 0;
	if (otherEnd) {
		otherEnd->wallblocked = 0;
	}

	if (inSolid) {
		trace_t results;
		vec_t* start = (altPlayerOriginIndexes & indexBit) ? awh->altOrigin : awh->origin;
		JP_TraceBenchmarked(&results, start, NULL, NULL, end->pos, passEntityNum, MASK_SOLID, traceCustomFlags);
		antiWhDebug.tracesDone++;
		end->wallblocked = 0;
		if (results.allsolid || results.startsolid) {
			VectorCopy(start,end->pos);
			end->wallblocked = 2;
			if (otherEnd) {
				otherEnd->wallblocked = 2;
				VectorCopy(start, otherEnd->pos);
			}
			boxChanged = qtrue;
		}
		else if (results.fraction != 1.0f) {
			end->wallblocked = 1;
			VectorCopy(results.plane.normal, end->wallblocknormal);
			end->wallblockdist = results.plane.dist;
			VectorCopy(results.endpos, end->pos);
			if (otherEnd) {
				// we checked the far box point. we now also do a cheap check if the closer box needs to be adjusted as well
				vec3_t lineGot, dir;
				float dot, len;
				VectorSubtract(results.endpos, start, lineGot);
				VectorSubtract(otherEnd->pos, start, dir);
				len = VectorNormalize(dir);
				dot = DotProduct(lineGot, dir);
				if (dot < len) {
					// yep. we didn't even get as far as the close point.
					VectorCopy(results.endpos, otherEnd->pos);
					VectorCopy(results.plane.normal, otherEnd->wallblocknormal);
					otherEnd->wallblockdist = results.plane.dist;
					otherEnd->wallblocked = 1;
				}
			}
			boxChanged = qtrue;
		}
	}
	awh->boxCreatedBitmask |= indexBit;

	return boxChanged;
}

#if 0
static qboolean SE_RenderPlayerChecks( gentity_t *self, const vec3_t playerOrigin, vec3_t playerPoints[9], int traceCustomFlags) {
	trace_t results;
	int i;

	for ( i = 0; i < 9; i++ ) {
		// TA: honestly the passentitynum here is a bit dumb... players cannot be CONTENTS_SOLID, only CONTENTS_BODY anyway but OH WELL
		if (G_AntiWH_PointContents( playerPoints[i], self - g_entities ) & CONTENTS_SOLID ) {
			JP_TraceBenchmarked( &results, playerOrigin, NULL, NULL, playerPoints[i], self - g_entities, MASK_SOLID, traceCustomFlags);
			antiWhDebug.tracesDone++;
			VectorCopy( results.endpos, playerPoints[i] );
		}
	}

	return qtrue;
}


static qboolean SE_IsPlayerCrouching( gentity_t *ent ) {
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
#endif

// basic thought: the normal player box is 30*30*(variable height)
// we want our top box points to be extensions of the player box that have equal angles in relation to each of the box's adjacent sides
// this way we have nice elegant and logical geometry AND we can do a single trace for the basic viewer and viewee points, since they lie on the same line
// 
// 
const float topBoxOffset = 15.0f;  // if we subdivide the real player box top side into 4 equal parts on top, we get 15x15 slices. then we also move 15 down. if we trace the top points from there, we get a nice 45/45/45 line out

// boxSize and wideBoxSize are half the side lengths of the box desired.
static void SE_RenderPlayerPoints( antiWallhackPlayerData_t* awh, float boxSize, float wideBoxSize
#if ANTIWH_PRETRACE
, vec3_t playerPointsCenter, vec3_t playerPointsMins, vec3_t playerPointsMaxs 
#endif
)
{
	int box,i;
	//AngleVectors( playerAngles, forward, right, up ); // see comment about forward, right, up
	

	const vec3_t	forward = { 1,0,0 }, right = { 0,1,0 }, up = { 0,0,1 }; // TA: why do we care about his orientation? his orientation doesnt matter as to whether he's visible. this just causes ppl to randomly phase in and out of visibility based on their own rotation

	if (!boxToOriginDirsInited) {
		for (i = 1; i < 9; i++) {
			VectorScale(boxDirs[i], -1.0f, boxToOriginDirs[i]);
			VectorNormalize(boxToOriginDirs[i]);
		}
		boxToOriginDirsInited = qtrue;
	}


	VectorSet(awh->altOrigin,awh->origin[0],awh->origin[1],awh->origin[2]+awh->maxsZ-topBoxOffset);



	for (box = 0; box < 2; box++) {
		float scale;
		awhBoxPt_t* outBox;
		float upOffset;
		float* diagonalMaxSize;
		if (box == 0) {
			scale = boxSize;
			outBox = awh->box;
			diagonalMaxSize = &awh->boxMaxDiagonal;
			upOffset = (awh->maxsZ - topBoxOffset) + scale; // we are adding this z to player origin for the top points.so move us to the nice even-angled source point (altOrigin) first, and from there we go up the boxSize again
		}
		else if (box == 1) {
			if (!wideBoxSize) {
				return;
			}
			scale = wideBoxSize;
			outBox = awh->viewerBox;
			diagonalMaxSize = &awh->viewerBoxMaxDiagonal;
			upOffset = (awh->maxsZ - topBoxOffset) + scale; // we are adding this z to player origin for the top points.so move us to the nice even-angled source point (altOrigin) first, and from there we go up the boxSize again
		}
		VectorMA(awh->origin, 32.0f, up, outBox[0].pos);
		VectorMA(awh->origin, scale, boxDirs[1], outBox[1].pos);
		VectorMA(awh->origin, scale, boxDirs[2], outBox[2].pos);
		VectorMA(awh->origin, scale, boxDirs[3], outBox[3].pos);
		VectorMA(awh->origin, scale, boxDirs[4], outBox[4].pos);
		//VectorMA(awh->origin, scale, forward, outBox[1].pos);
		//VectorMA(outBox[1].pos, scale, right, outBox[1].pos);
		//VectorMA(awh->origin, scale, forward, outBox[2].pos);
		//VectorMA(outBox[2].pos, -scale, right, outBox[2].pos);
		//VectorMA(awh->origin, -scale, forward, outBox[3].pos);
		//VectorMA(outBox[3].pos, -scale, right, outBox[3].pos);
		//VectorMA(awh->origin, -scale, forward, outBox[4].pos);
		//VectorMA(outBox[4].pos, scale, right, outBox[4].pos);

		VectorMA(outBox[1].pos, upOffset, up, outBox[5].pos);
		VectorMA(outBox[2].pos, upOffset, up, outBox[6].pos);
		VectorMA(outBox[3].pos, upOffset, up, outBox[7].pos);
		VectorMA(outBox[4].pos, upOffset, up, outBox[8].pos);

		*diagonalMaxSize = Distance(outBox[1].pos,outBox[7].pos);

#if ANTIWH_PRETRACE
		VectorMA(awh->origin, upOffset * 0.5f, up, awh->boxCenter);
		VectorSet(awh->boxMins, -scale, -scale, -0.5f * upOffset);
		VectorSet(awh->boxMaxs, scale, scale, 0.5f * upOffset);
#endif
	}
	return;
}

static void GetCameraPosition(gentity_t *self, vec3_t cameraOrigin) {
	vec3_t forward;
	const float thirdPersonRange = 80, thirdPersonVertOffset = 16;
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
	//cameraOrigin[2] += 24 + thirdPersonVertOffset;
	cameraOrigin[2] += self->client->ps.viewheight + thirdPersonVertOffset;

	//if (SE_IsPlayerCrouching(self))
	//	cameraOrigin[2] -= 32;
}

// precalculate target positions for the viewer and viewee boxes, but dont do actual traces to them yet. 
// returns whether there was any change that might influence visibility from invisible->visible
static qboolean SE_CheckUpdatePlayerBoxes(gentity_t* other, int traceFlags) {
	vec3_t firstPersonPos, thirdPersonPos;// , targPos[9], targetCenter, targetMins, targetMaxs;
	antiWallhackPlayerData_t* awh = &other->client->antiwh;
	
	if (g_antiWallhackRecalcOffset.value > 0.0f && awh->boxSize == g_antiWallhackBoxSize.value && awh->viewerBoxSize == g_antiWallhackViewerBoxSize.value && other->r.maxs[2] == awh->maxsZ) {
		// already have a box, check if we can reuse
		vec3_t moveOffset;
		VectorSubtract(other->client->ps.origin, awh->origin, moveOffset);
		if (VectorLengthSquared(moveOffset) < g_antiWallhackRecalcOffset.value * g_antiWallhackRecalcOffset.value) {
			// we have determined nothing needs to be done. our old base positions are still fine.
			if (!g_antiWallhackViewerBoxSize.value) {
				// viewer box is not active, so we still need to recalc thirdpersonpos quick.
				GetCameraPosition(other, awh->viewerBox[ANTIWH_WIDEBOX_THIRDPERSONPOS].pos);
				awh->boxCreatedBitmask &= ~(1 << ANTIWH_WIDEBOX_THIRDPERSONPOS); // thirdperson pos is always recalced
				awh->boxIndex = ++other->client->pers.antiWallhackBoxIndex;
				return qtrue;
			}
			return qfalse;
		}
	}

	VectorCopy(other->client->ps.origin, awh->viewerBox[ANTIWH_WIDEBOX_FIRSTPERSONPOS].pos);
	// technically viewheight would be the intended value, BUT there's a glitch when you roll into a narrow space. 
	// your viewheight will reset, but your hitbox wont since there is no space to uncrouch
	awh->viewerBox[ANTIWH_WIDEBOX_FIRSTPERSONPOS].pos[2] += other->r.maxs[2];

	if (!g_antiWallhackViewerBoxSize.value) {
		// viewer box is not active, go for traditional thing with thirdpersonpos
		GetCameraPosition(other, awh->viewerBox[ANTIWH_WIDEBOX_THIRDPERSONPOS].pos);
		// we will check firstpersonpos and thirdpersonpos
		awh->wideBoxCheckMask = (1 << ANTIWH_WIDEBOX_THIRDPERSONPOS) | (1 << ANTIWH_WIDEBOX_FIRSTPERSONPOS);
	}
	else {
		// we will check all except thirdpersonpos
		awh->wideBoxCheckMask = -1 & ~(1 << ANTIWH_WIDEBOX_THIRDPERSONPOS);
	}

	// plot their bbox pointer into targPos[]
	awh->maxsZ = other->r.maxs[2];
	VectorCopy(other->client->ps.origin, awh->origin);
	VectorCopy(other->r.currentOrigin, awh->rOrigin);

	// this is for the pvs check. using r.currentOrigin makes it die a horrible death on ffa_bespin stairs :)
	VectorCopy(other->client->ps.origin, awh->realViewHeightOrigin); 
	awh->realViewHeightOrigin[2] += other->client->ps.viewheight;

	SE_RenderPlayerPoints(&other->client->antiwh, g_antiWallhackBoxSize.value, g_antiWallhackViewerBoxSize.value
#if ANTIWH_PRETRACE
		, awh->boxCenter, awh->boxMins, awh->boxMaxs
#endif
	);
	awh->boxSize = g_antiWallhackBoxSize.value;
	awh->viewerBoxSize = g_antiWallhackViewerBoxSize.value;
	awh->boxCreatedBitmask = 0; // which indexes are done.
	awh->lastBoxUpdate = level.time; // for debugging/ drawing the box.
	awh->boxIndex = ++other->client->pers.antiWallhackBoxIndex;

	return qtrue;
}

static void SE_DebugBox( gentity_t* self ) {
	int i,b;
	awhBoxPt_t* box;
	if (!g_antiWallhackDebugBox.integer) {
		return;
	}
	if (/*self->client->antiwh.lastBoxUpdate == level.time || */ level.time >= self->client->antiwh.nextTestLineBox) {
		// draw player box
		const int indexes[12][2] = {
			// bottom
			{1,2},
			{2,3},
			{3,4},
			{4,1},

			// top
			{5,6},
			{6,7},
			{7,8},
			{8,5},

			// connecting sides
			{1,5},
			{2,6},
			{3,7},
			{4,8},
		};

		for (b = 0; b < 2; b++) {
			box = (b == 0) ? self->client->antiwh.box : self->client->antiwh.viewerBox;
			for (i = 0; i < 12; i++) {
				G_TestLineBetter(self,box[indexes[i][0]].pos, box[indexes[i][1]].pos, b ? 0xff0000 : 0x00ff00, 200);
			}
		}
		self->client->antiwh.nextTestLineBox = level.time + 100;
	}
}

static void SE_DebugWinLine( gentity_t* self, gentity_t* other) {
	int i,b;
	awhVis_t* visMemory = &other->client->antiwh.visibleTo[self - g_entities];
	if (!g_antiWallhackDebugWinLine.integer || level.time < other->client->antiwh.nextTestLineWin || visMemory->hidden) {
		return;
	}
	G_TestLineBetter(self, self->client->antiwh.viewerBox[visMemory->winLineViewer].pos, other->client->antiwh.box[visMemory->winLineViewee].pos, 0x0000ff, 200);
		other->client->antiwh.nextTestLine = level.time + 100;
}

static int SE_MustHidePlayer( gentity_t *viewer, gentity_t *viewee ) {
	int i,j,viewerIndex,vieweeIndex,  contents;
	int whVal = g_antiWallhack.integer < 0 ? -g_antiWallhack.integer : g_antiWallhack.integer;
#if ANTIWH_PRETRACE
	int preTraceFlags = 0
#endif
	int traceFlags = 0;
	antiWallhackPlayerData_t* awhViewer = &viewer->client->antiwh;
	antiWallhackPlayerData_t* awhViewee = &viewee->client->antiwh;
	awhVis_t* visMemory = &awhViewee->visibleTo[viewer - g_entities];
	vec3_t enemyPointToViewerPoint;
	awhBoxPt_t* viewerPt, * vieweePt;
	float dot;
	int inpvs;
	int impossibleVieweePoints = 0;
	int hideReasons = 0;
	
#if ANTIWH_PRETRACE
	if (g_antiWallhackFast.integer == 1 && (coolApi & COOL_APIFEATURE_PRETRACE_TRACE)) {
		preTraceFlags = TRACECUSTOMFLAG_MARKBRUSHES, traceFlags = TRACECUSTOMFLAG_WALKBRUSHES;
	}
#endif
	if (g_antiWallhackFast.integer >= 2 && (coolApi & COOL_APIFEATURE_FASTHULLTRACE)) {
#if ANTIWH_PRETRACE
		preTraceFlags = 0;
#endif
		traceFlags = TRACECUSTOMFLAG_FASTHULLTRACE;
		if (g_antiWallhackFast.integer == 3) {
			traceFlags |= TRACECUSTOMFLAG_SKIPENTITYTRACE;
		}
	}

	// who cares, let him see.
	if (viewee->health <= 0 || viewee->client->ps.stats[STAT_HEALTH] <= 0 || !viewee->r.linked || !viewer->r.linked) {
		return 0;
	}

	if (g_antiWallhackVisibleRecalcDelay.integer && !visMemory->hidden && (level.time - g_antiWallhackVisibleRecalcDelay.integer) < visMemory->lastCheck) {
		// player was visible less than 0.15s ago. just don't bother rechecking. who cares.
		return 0;
	}

	SE_CheckUpdatePlayerBoxes(viewee, traceFlags & ~(TRACECUSTOMFLAG_WALKBRUSHES));
	SE_CheckUpdatePlayerBoxes(viewer, traceFlags & ~(TRACECUSTOMFLAG_WALKBRUSHES));

#if ANTIWH_PRETRACE // this used to be before SE_RenderPlayerChecks. doesn't make much sense now :) part of the reason i got rid of this, to make refactoring not a pita
	if (preTraceFlags) {
		trace_t pretrace;
		JP_TraceBenchmarked(&pretrace, thirdPersonPos, awhOther->boxMins, awhOther->boxMaxs, awhOther->boxCenter, self - g_entities, MASK_SOLID, preTraceFlags);
		antiWhDebug.tracesDone++;
	}
#endif



	/*if (whVal > 1 && (level.time >= awhOther->nextTestLine)) {
		int offset = whVal - 2;

		if (offset < 0)
			offset = 0;
		if (offset > 8)
			offset = 8;

		G_TestLineBetter(thirdPersonPos, awhOther->box[offset], 0x0000ff, 200); //check trace.fraction? ehh trace.startsolid or whatever?
		awhOther->nextTestLine = level.time + 200;
	}*/

	if (visMemory->viewerBoxIndex == awhViewer->boxIndex && visMemory->vieweeBoxIndex == awhViewee->boxIndex) {
		// nothing changed enough to warrant a recalc.
		return visMemory->hidden;
	}

	// close enough that no wall can be in-between
	// eh, that's gonna happen practically never, why even waste processing power on it.
	//if (DistanceSquared(awhViewer->rOrigin, awhViewee->rOrigin) < 30.0f) {
	//	// same leaf. consider visible.
	//	visMemory->visible = qtrue;
	//	visMemory->lastCheck = level.time;
	//	visMemory->viewerBoxIndex = awhViewer->boxIndex;
	//	visMemory->vieweeBoxIndex = awhViewee->boxIndex;
	//}
	
	// using r.currentOrigin for the viewer makes it die a horrible death on ffa_bespin stairs near big shield
	//	 if one person is on the stairs near the shield and another above him beind the railing (what's a better english word?) :)
	// why? who the hell knows. it's actually WEIRD AS FUCK
	// weirdest part is: it fixes it in both directions, but without the fix, jumping up from the stairs doesn't even fix it.
	// even though r.currentOrigin is basically the same, just lower.
	// BONUS: we now use a InPVSEntity trap that simulates the exact server snapshot PVS check, including multiple clusters for the viewed entity.
	inpvs = G_AntiWH_InPVS(awhViewer->realViewHeightOrigin, viewee, awhViewee->rOrigin);

	if (!inpvs) { // not in PVS. ignore.
		visMemory->hidden = (1<<AWHFAIL_INPVS_SKIP);
		visMemory->lastCheck = level.time;
		visMemory->viewerBoxIndex = awhViewer->boxIndex;
		visMemory->vieweeBoxIndex = awhViewee->boxIndex;
		return visMemory->hidden;
	}
	else if (g_antiWallhackFast.integer >= 2 && coolApi && inpvs > 1) {
		// same leaf. consider visible.
		visMemory->hidden = 0;
		visMemory->lastCheck = level.time;
		visMemory->viewerBoxIndex = awhViewer->boxIndex;
		visMemory->vieweeBoxIndex = awhViewee->boxIndex;
		return visMemory->hidden;
	}

	contents = G_AntiWH_PointContents(awhViewer->viewerBox[ANTIWH_WIDEBOX_FIRSTPERSONPOS].pos, viewer - g_entities);

	// translucent, we should probably just network them anyways
	// TA: isnt this a bit stoopid? :/
	if (contents & (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)) {
		visMemory->hidden = 0;
		visMemory->lastCheck = level.time;
		visMemory->viewerBoxIndex = awhViewer->boxIndex;
		visMemory->vieweeBoxIndex = awhViewee->boxIndex;
		return visMemory->hidden;
	}

	// entirely in an opaque surface, no point networking them.
	if (contents & (CONTENTS_SOLID | CONTENTS_TERRAIN | CONTENTS_OPAQUE)) {
		visMemory->hidden = (1<<AWHFAIL_INSURFACE);
		visMemory->lastCheck = level.time;
		visMemory->viewerBoxIndex = awhViewer->boxIndex;
		visMemory->vieweeBoxIndex = awhViewee->boxIndex;
		return visMemory->hidden;
	}

	visMemory->hidden = (1<<AWHFAIL_NOPATHFOUND);
	visMemory->lastCheck = level.time;
	visMemory->viewerBoxIndex = awhViewer->boxIndex;
	visMemory->vieweeBoxIndex = awhViewee->boxIndex;
	visMemory->traces = 0;
	visMemory->tracesTotal = 0;
	for( j = 0; j < ANTIWH_WIDEBOX_SIZE; j++ ) {

		viewerIndex = ( j + visMemory->winLineViewer ) % ANTIWH_WIDEBOX_SIZE;

		if (!(awhViewer->wideBoxCheckMask & (1 << viewerIndex))) {
			continue;
		}

		if (!(awhViewer->boxCreatedBitmask & (1 << viewerIndex))) {
			if (SE_CheckBoxIndex(&viewer->client->antiwh, viewerIndex, (viewee - g_entities), traceFlags)) {
				awhViewer->lastBoxUpdate = level.time;
			}
		}

		viewerPt = &awhViewer->viewerBox[viewerIndex];

		for ( i = 0; i < ANTIWH_BOX_BASESIZE; i++ ) {

			visMemory->tracesTotal++;

			// optimization: start at the indexes that were successful last time (aka skip the indexes that failed previously and do them later)
			// ideally we only need to recheck a single line again.
			vieweeIndex = ( i + visMemory->winLineViewee ) % ANTIWH_BOX_BASESIZE;

			if (impossibleVieweePoints & (1 << vieweeIndex)) {
				visMemory->hidden |= (1<<AWHFAIL_IMPOSS_VIEWERPOINT_SKIP);
				continue;
			}

			if (!(awhViewee->boxCreatedBitmask & (1 << vieweeIndex))) {
				if (SE_CheckBoxIndex(&viewee->client->antiwh, vieweeIndex, (viewer - g_entities), traceFlags)) {
					awhViewee->lastBoxUpdate = level.time;
				}
			}

			vieweePt = &awhViewee->viewerBox[vieweeIndex];
						
			if (viewerPt->wallblocked) {
				dot = DotProduct(vieweePt->pos, viewerPt->wallblocknormal) - viewerPt->wallblockdist;
				if (dot < -1.0f) {
					// well, there's a wall there, and the viewee point lies behind the plane of the wall so... no chance bucko.
					if (dot < -awhViewee->boxMaxDiagonal) {
						visMemory->hidden |= (1<<AWHFAIL_VIEWERPOINT_WALLBLOCK_PLANE_SKIP);
						break; // yea this point lies behind the wall normal farther than the entire viewee max box size, so no chance ANY of the viewee points will be visible. may as well stop trying.
					}
					visMemory->hidden |= (1<<AWHFAIL_VIEWERPOINT_WALLBLOCK_PLANE_SKIP_HARD);
					continue;
				}
			}

			if (vieweePt->wallblocked) {
				dot = DotProduct(viewerPt->pos, vieweePt->wallblocknormal) - vieweePt->wallblockdist;
				if (dot < -1.0f) {
					// well, there's a wall there, and the viewer point lies behind the plane of the wall so... no chance bucko.
					if (dot < -awhViewer->boxMaxDiagonal ) {
						visMemory->hidden |= (1<<AWHFAIL_VIEWEEPOINT_WALLBLOCK_PLANE_SKIP_HARD);
						impossibleVieweePoints |= (1 << vieweeIndex); // yea this viewer point lies behind the wall normal farther than the entire viewer max box size, so no chance ANY of the viewer points will see this viewee point. may as well stop trying.
					}
					visMemory->hidden |= (1<<AWHFAIL_VIEWEEPOINT_WALLBLOCK_PLANE_SKIP);
					continue;
				}
			}

			if (dirCheckIndexes & (1 << viewerIndex)) {
				VectorSubtract(viewerPt->pos, vieweePt->pos, enemyPointToViewerPoint);
				VectorNormalize(enemyPointToViewerPoint);
				if (DotProduct(enemyPointToViewerPoint, boxToOriginDirs[viewerIndex]) > 0.707f) {
					// if the viewer camera was at this place, there's no way we could see that enemy
					visMemory->hidden |= (1<<AWHFAIL_VIEWERPOINT_DIRCHECK_SKIP);
					continue;
				}
			}

			visMemory->traces++;
			if ( SE_RenderIsVisible( viewer, awhViewer->viewerBox[viewerIndex].pos, vieweePt->pos, qfalse, traceFlags) ) {
				visMemory->hidden = 0;
				visMemory->winLineViewee = vieweeIndex;
				visMemory->winLineViewer = viewerIndex;
				return visMemory->hidden;
			}
		}
	}


	return visMemory->hidden;
}

/*
static qboolean SE_RenderInFOV( gentity_t *self, const vec3_t testOrigin ) {
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
int G_EntityOccluded( gentity_t *self, gentity_t *other ) {
	int nonetwork;
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
	nonetwork = SE_MustHidePlayer(self, other);

#ifdef _DEBUG
	if (nonetwork && self->s.number == 0 && g_antiWallhack.integer < 0) {
		char failReasons[MAX_STRING_CHARS];
		int i,added = 0;
		awhVis_t* visMemory = &other->client->antiwh.visibleTo[self - g_entities];
		*failReasons = '\0';
		for (i = 0; i < AWHFAIL_COUNT_REASONS; i++) {
			if (!(nonetwork & (1 << i))) {
				continue;
			}
			if (added) {
				Q_strcat(failReasons, sizeof(failReasons), ",");
			}
			Q_strcat(failReasons, sizeof(failReasons), awhHiddenReasonStrings[i]);
			added++;
		}
		Com_Printf("WALLHACK[%i]: not visible;%s reasons: %s\n", level.time, (nonetwork & (1 << AWHFAIL_NOPATHFOUND)) ? multiva(" traces done: %d/%d;", visMemory->traces,visMemory->tracesTotal) : "", failReasons);
	}
#endif // _DEBUG

	if (g_antiWallhackDebugBox.integer) {
		SE_DebugBox(self);
		SE_DebugBox(other);
	}
	if (g_antiWallhackDebugWinLine.integer) {
		SE_DebugWinLine(self,other);
	}
	return nonetwork;
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
			float tracesPerSecondSpeed = timeSpentTraces == 0 ? 0 :1000.0f * (float)antiWhDebug.tracesDone / timeSpentTraces;
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





























//
// 
//
// IP SANCTION SYSTEM
// for now, these are temporary ip restrictions that don't persist across level restarts/map changes
//
//
//
ipSanction_t ipSanctions[MAX_SANCTIONS_GLOBAL] = { 0 };
ipSanction_t* ipSanctionsFree = ipSanctions;
ipSanction_t* ipSanctionsActive = NULL;
void G_InitIPSanctions(void) {
	int i;
	memset(ipSanctions, 0, sizeof(ipSanctions));
	for (i = 0; i < MAX_SANCTIONS_GLOBAL-1; i++) {
		ipSanctions[i].next = &ipSanctions[i+1];
	}
	ipSanctionsFree = ipSanctions;
	ipSanctionsActive = NULL;
}
void G_RemoveIPSanction(ipSanction_t* ipSanction) {
	if (ipSanction == ipSanctionsActive) {
		ipSanctionsActive = ipSanction->next;
		ipSanction->next = ipSanctionsFree;
		ipSanctionsFree = ipSanction;
		if (g_developer.integer) {
			Com_Printf("^3G_RemoveIPSanction: IP sanction removed (0).\n");
		}
		return;
	}
	else {
		ipSanction_t* sanction;
		ipSanction_t* previous;
		if (!ipSanctionsActive) {
			Com_Error(ERR_FATAL, "G_RemoveIPSanction: Trying to remove sanction when no sanctions exist.");
			return;
		}
		previous = ipSanctionsActive;
		sanction = ipSanctionsActive->next;
		while (sanction) {
			if (sanction == ipSanction) {
				previous->next = sanction->next;
				sanction->next = ipSanctionsFree;
				ipSanctionsFree = sanction;
				if (g_developer.integer) {
					Com_Printf("^3G_RemoveIPSanction: IP sanction removed (n).\n");
				}
				return;
			}
			previous = sanction;
			sanction = sanction->next;
		}
		Com_Error(ERR_FATAL, "G_RemoveIPSanction: Trying to remove sanction that doesn't exist.");
		return;
	}
}
void G_CleanIPSanctions(qboolean forceOneFree) {
	ipSanction_t*	sanction = ipSanctionsActive;
	ipSanction_t*	sanctionNext;
	ipSanction_t*	oldest = NULL;
	int				oldestExpire = INT_MAX;
	int				time = trap_RealTime(NULL);

	sanction = ipSanctionsActive;
	while (sanction) {
		sanctionNext = sanction->next;
		if (sanction->sanction.expires <= time) {
			G_RemoveIPSanction(sanction);
		}
		else if(!oldest || sanction->sanction.expires < oldestExpire) {
			oldestExpire = sanction->sanction.expires;
			oldest = sanction;
		}
		sanction = sanctionNext;
	}

	if (forceOneFree && !ipSanctionsFree) {
		if (!oldest) {
			Com_Error(ERR_FATAL, "G_CleanIPSanctions: Trying to clear one sanction slot, but no suitable slot was found.");
			return;
		}
		if (g_developer.integer) {
			Com_Printf("^3G_CleanIPSanctions: Oldest sanction removed to make space.\n");
		}
		G_RemoveIPSanction(oldest);
	}
}
void G_AddIPSanction(gentity_t* ent, int seconds, sanctionType_t type, int param1, unsigned int param2, const char* reason) {
	ipSanction_t*	sanction;
	int				time = trap_RealTime(NULL);
	G_CleanIPSanctions(qtrue);
	if (!ipSanctionsFree) {
		Com_Error(ERR_FATAL, "G_AddIPSanction: Error allocating ip sanction.");
		return;
	}
	sanction = ipSanctionsFree;
	ipSanctionsFree = sanction->next;
	sanction->next = ipSanctionsActive;
	ipSanctionsActive = sanction;

	sanction->sanction.type = type;
	sanction->sanction.param1 = param1;
	sanction->sanction.param2 = param2;
	sanction->sanction.expires = time + seconds;
	Q_strncpyz(sanction->sanction.reason,reason,sizeof(sanction->sanction.reason));
	memcpy(sanction->ip, mv_clientSessions[ent - g_entities].clientIP, sizeof(sanction->ip));
}

sanction_t* G_CheckIPSanctionMatchParam1(gentity_t* ent, sanctionType_t type, int param1) {
	ipSanction_t*	sanction;
	ipSanction_t*	sanctionNext;
	//ipSanction_t*	highestExpireSanction = NULL;
	//int				highestExpire = INT_MIN;
	int				time = trap_RealTime(NULL);
	sanction = ipSanctionsActive;
	while (sanction) {
		sanctionNext = sanction->next;
		if (sanction->sanction.expires <= time) { // clean up on the fly
			G_RemoveIPSanction(sanction);
		}
		else if (sanction->sanction.type == type && sanction->sanction.param1 == param1 && !memcmp(sanction->ip,mv_clientSessions[ent - g_entities].clientIP,sizeof(sanction->ip))) {
			//if (!highestExpireSanction || highestExpire < sanction->sanction.expires) {
			//	highestExpireSanction = sanction;
			//	highestExpire = sanction->sanction.expires;
			//}
			return &sanction->sanction;
		}
		sanction = sanctionNext;
	}
	//return highestExpireSanction; // meh keep it simple
	return NULL;
}


sanction_t* G_CheckIPSanctionMatchParam2(gentity_t* ent, sanctionType_t type, unsigned int param2) {
	ipSanction_t* sanction;
	ipSanction_t* sanctionNext;
	int				time = trap_RealTime(NULL);
	sanction = ipSanctionsActive;
	while (sanction) {
		sanctionNext = sanction->next;
		if (sanction->sanction.expires <= time) { // clean up on the fly
			G_RemoveIPSanction(sanction);
		}
		else if (sanction->sanction.type == type && sanction->sanction.param2 == param2 && !memcmp(sanction->ip, mv_clientSessions[ent - g_entities].clientIP, sizeof(sanction->ip))) {
			return &sanction->sanction;
		}
		sanction = sanctionNext;
	}
	return NULL;
}
sanction_t* G_CheckIPSanctionMatchParams(gentity_t* ent, sanctionType_t type, int param1, unsigned int param2) {
	ipSanction_t* sanction;
	ipSanction_t* sanctionNext;
	int				time = trap_RealTime(NULL);
	sanction = ipSanctionsActive;
	while (sanction) {
		sanctionNext = sanction->next;
		if (sanction->sanction.expires <= time) { // clean up on the fly
			G_RemoveIPSanction(sanction);
		}
		else if (sanction->sanction.type == type && sanction->sanction.param1 == param1 && sanction->sanction.param2 == param2 && !memcmp(sanction->ip, mv_clientSessions[ent - g_entities].clientIP, sizeof(sanction->ip))) {
			return &sanction->sanction;
		}
		sanction = sanctionNext;
	}
	return NULL;
}


void Cmd_Sanction_f(gentity_t* ent) {
	qboolean allRace = qfalse;
	int clientnum, seconds, type, param1, param2;
	const char* reason;
	gentity_t* sourcePlayerEnt = GetClientNumArg();

	if (!ent->client->sess.login.loggedIn || !(ent->client->sess.login.flags & TT_ACCOUNTFLAG_A_SANCTION)) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You do not have permission to use this command.\n\"");
		return;
	}

	if (trap_Argc() < 5) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: sanction [clientnum] [seconds] [type] [param1] [param2] ([reason]).\n\"");
		return;
	}

	if (!sourcePlayerEnt || !sourcePlayerEnt->inuse || !sourcePlayerEnt->client) {
		trap_SendServerCommand(ent - g_entities, "print \"Please specify a valid client number who you wish to sanction.\n\"");
		return;
	}

	clientnum = atoi(G_Argv(1));
	seconds = atoi(G_Argv(2));
	type = atoi(G_Argv(3));
	param1 = atoi(G_Argv(4));
	param2 = atoi(G_Argv(5));
	reason = ConcatArgs(6);

	G_AddIPSanction(sourcePlayerEnt, seconds, type, param1, param2, reason);

	trap_SendServerCommand(ent - g_entities, "print \"Sanction added.\n\"");

}


