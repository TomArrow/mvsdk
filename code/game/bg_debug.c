
#include "bg_debug.h"

// these are fields i'm reasonably confident can be safely abused for sending stuff in an otherwise useless ET_INVISIBLE entity
// excluding any that would cause problems like mv version remapping (animations and such)
debugField_t	debugFields[] =
{
	{ NETF(pos.trTime), 32 },
	{ NETF(pos.trBase[0]), 0 },
	{ NETF(pos.trBase[1]), 0 },
	{ NETF(pos.trDelta[0]), 0 },
	{ NETF(pos.trDelta[1]), 0 },
	{ NETF(pos.trBase[2]), 0 },
	{ NETF(apos.trBase[1]), 0 },
	{ NETF(pos.trDelta[2]), 0 },
	{ NETF(apos.trBase[0]), 0 },
	//{ NETF(event), 10 },			// events still get evaluated	
	{ NETF(angles2[1]), 0 },
	//{ NETF(eType), 8 },			// ofc used for identification
	//{ NETF(torsoAnim), 16 },		// cant use anims, they remap using tables and could cause memory access violations
	{ NETF(forceFrame), 16 },
	{ NETF(eventParm), 8 },
	//{ NETF(legsAnim), 16 },		// cant use anims, they remap using tables and could cause memory access violations	
	{ NETF(groundEntityNum), GENTITYNUM_BITS },
	//{ NETF(pos.trType), 8 },		// dont cause interpolation to create issues for us
	{ NETF(eFlags), 32 },
	{ NETF(bolt1), 8 },
	{ NETF(bolt2), GENTITYNUM_BITS },
	{ NETF(trickedentindex), 16 },
	{ NETF(trickedentindex2), 16 },
	{ NETF(trickedentindex3), 16 },
	{ NETF(trickedentindex4), 16 },
	{ NETF(speed), 0 },
	{ NETF(fireflag), 2 },
	{ NETF(genericenemyindex), 32 },
	{ NETF(activeForcePass), 6 },
	{ NETF(emplacedOwner), 32 },
	{ NETF(otherEntityNum), GENTITYNUM_BITS },
	{ NETF(weapon), 8 },
	{ NETF(clientNum), 8 },
	{ NETF(angles[1]), 0 },
	{ NETF(pos.trDuration), 32 },
	//{ NETF(apos.trType), 8 },		// dont cause interpolation to cause issues for us
	{ NETF(origin[0]), 0 },
	{ NETF(origin[1]), 0 },
	{ NETF(origin[2]), 0 },
	//{ NETF(solid), 24 },		// avoid interpreting as bmodel by accident
	{ NETF(owner), GENTITYNUM_BITS },
	{ NETF(teamowner), 8 },
	{ NETF(shouldtarget), 1 },
	{ NETF(powerups), 16 },
	//{ NETF(modelGhoul2), 4 }, // used for identifying as a debug entity
	{ NETF(g2radius), 8 },
	{ NETF(modelindex), -8 },
	{ NETF(otherEntityNum2), GENTITYNUM_BITS },
	//{ NETF(loopSound), 8 }, // 
	{ NETF(generic1), 8 },
	{ NETF(origin2[2]), 0 },
	{ NETF(origin2[0]), 0 },
	{ NETF(origin2[1]), 0 },
	{ NETF(modelindex2), 8 },
	{ NETF(angles[0]), 0 },
	{ NETF(time), 32 },
	{ NETF(apos.trTime), 32 },
	{ NETF(apos.trDuration), 32 },
	{ NETF(apos.trBase[2]), 0 },
	{ NETF(apos.trDelta[0]), 0 },
	{ NETF(apos.trDelta[1]), 0 },
	{ NETF(apos.trDelta[2]), 0 },
	{ NETF(time2), 32 },
	{ NETF(angles[2]), 0 },
	{ NETF(angles2[0]), 0 },
	{ NETF(angles2[2]), 0 },
	//{ NETF(constantLight), 32 },	// dont create glowing lights
	{ NETF(frame), 16 },
	{ NETF(saberInFlight), 1 },
	{ NETF(saberEntityNum), GENTITYNUM_BITS },
	{ NETF(saberMove), 8 },
	{ NETF(forcePowersActive), 32 },
	{ NETF(isJediMaster), 1 }
};

int debugFieldCount = sizeof(debugFields) / sizeof(debugFields[0]);
