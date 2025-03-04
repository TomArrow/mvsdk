// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int				serverTime;
	int				angles[3];
	int 			buttons;
	byte			weapon;           // weapon 
	byte			forcesel;
	byte			invensel;
	byte			generic_cmd;
	signed char	forwardmove, rightmove, upmove;
} usercmd_t;


forward/right/upmove fits into one integer (24 bits is enough even)
weapon/forcesel/invensel/generic_cmd fits into another int
angles needs 16 bits each.
buttons needs 16 bits
servertime needs full int ... wait i dont need that. just use an int to save current and previous msec? or just current msec? idk maybe trickedentindex for last 4 msec values?


// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int				serverTime;
	int				angles[3];
	int 			buttons;
	byte			weapon;           // weapon 
	byte			forcesel;
	byte			invensel;
	byte			generic_cmd;
	signed char	forwardmove, rightmove, upmove;
} usercmd_t;

typedef struct raceStyle_s {
	byte movementStyle; // jk2. maybe some day pjk2 => STAT_MOVEMENTSTYLE
	short msec; // -1 if toggle, -2 if float (ignore float for now, its cringe anyway)
	signed char jumpLevel; // 0=no force, -1 = ysal, -2 = ?, 4=jumpcharge?
	short variant; // when we have map variants (invis walls and such). 0 =default (ignore for now)
	short runFlags; // flags from runFlags_t => STAT_RUNFLAGS
} raceStyle_t;


netField_t	entityStateFields15[] =
{
	{ NETF(pos.trTime), 32 }, // respos count of last cut mark in segmented
	{ NETF(pos.trBase[0]), 0 }, // antiloop angle change
	{ NETF(pos.trBase[1]), 0 }, // forceSpeedSmash
	{ NETF(pos.trDelta[0]), 0 }, // fd.forceJumpCharge
	{ NETF(pos.trDelta[1]), 0 }, // ACTUALLY NVM NOT DOING THIS: ps.groundtime (int->float)
	{ NETF(pos.trBase[2]), 0 },
	{ NETF(apos.trBase[1]), 0 }, // ucmd angles
	{ NETF(pos.trDelta[2]), 0 },
	{ NETF(apos.trBase[0]), 0 }, // ucmd angles
	{ NETF(event), 10 },			// There is a maximum of 256 events (8 bits transmission, 2 high bits for uniqueness)
	{ NETF(angles2[1]), 0 },
	{ NETF(eType), 8 }, // ET_INVISIBLE
	{ NETF(torsoAnim), 16 }, 		// Maximum number of animation sequences is 2048.  Top bit is reserved for the togglebit
	{ NETF(forceFrame), 16 }, // ucmd: buttons   //if you have over 65536 frames, then this will explode. Of course if you have that many things then lots of things will probably explode.
	{ NETF(eventParm), 8 },
	{ NETF(legsAnim), 16 },		// Maximum number of animation sequences is 2048.  Top bit is reserved for the togglebit
	{ NETF(groundEntityNum), GENTITYNUM_BITS },
	{ NETF(pos.trType), 8 },
	{ NETF(eFlags), 32 },
	{ NETF(bolt1), 8 }, // movementstyle
	{ NETF(bolt2), GENTITYNUM_BITS },
	{ NETF(trickedentindex), 16 }, // array of 4 past fps values //See note in PSF
	{ NETF(trickedentindex2), 16 }, // array of 4 past fps values
	{ NETF(trickedentindex3), 16 }, // array of 4 past fps values
	{ NETF(trickedentindex4), 16 }, // array of 4 past fps values
	{ NETF(speed), 0 },
	{ NETF(fireflag), 2 }, // past fps index
	{ NETF(genericenemyindex), 32 }, //Do not change to GENTITYNUM_BITS, used as a time offset for seeker
	{ NETF(activeForcePass), 6 }, // leaderboard type
	{ NETF(emplacedOwner), 32 }, //As above, also used as a time value (for electricity render time)
	{ NETF(otherEntityNum), GENTITYNUM_BITS },
	{ NETF(weapon), 8 },
	{ NETF(clientNum), 8 }, // clientnum
	{ NETF(angles[1]), 0 },
	{ NETF(pos.trDuration), 32 },
	{ NETF(apos.trType), 8 },
	{ NETF(origin[0]), 0 },
	{ NETF(origin[1]), 0 },
	{ NETF(origin[2]), 0 },
	{ NETF(solid), 24 },
	{ NETF(owner), GENTITYNUM_BITS },
	{ NETF(teamowner), 8 },
	{ NETF(shouldtarget), 1 },
	{ NETF(powerups), 16 }, // rs: variant
	{ NETF(modelGhoul2), 4 }, // 15 (to show its stats)
	{ NETF(g2radius), 8 },
	{ NETF(modelindex), -8 }, // jumplevel
	{ NETF(otherEntityNum2), GENTITYNUM_BITS },
	{ NETF(loopSound), 8 },
	{ NETF(generic1), 8 },
	{ NETF(origin2[2]), 0 },
	{ NETF(origin2[0]), 0 },
	{ NETF(origin2[1]), 0 },
	{ NETF(modelindex2), 8 },
	{ NETF(angles[0]), 0 },
	{ NETF(time), 32 }, 
	{ NETF(apos.trTime), 32 }, // last segmented run reset point in replay (commandtime)
	{ NETF(apos.trDuration), 32 }, // racestyle: msec and runflags
	{ NETF(apos.trBase[2]), 0 }, // ucmd angles
	{ NETF(apos.trDelta[0]), 0 },
	{ NETF(apos.trDelta[1]), 0 },
	{ NETF(apos.trDelta[2]), 0 },
	{ NETF(time2), 32 },
	{ NETF(angles[2]), 0 },
	{ NETF(angles2[0]), 0 },
	{ NETF(angles2[2]), 0 },
	{ NETF(constantLight), 32 }, // usercmd: weapon, forcesel, invensel, generric_cmd
	{ NETF(frame), 16 }, // amount of saveposes in current segmented replay
	{ NETF(saberInFlight), 1 },
	{ NETF(saberEntityNum), GENTITYNUM_BITS },
	{ NETF(saberMove), 8 },
	{ NETF(forcePowersActive), 32 }, // usercmd: forwardmove, rightmove, upmove
	{ NETF(isJediMaster), 1 }
};