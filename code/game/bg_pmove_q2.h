
#ifndef BG_PMOVE_Q2_H
#define BG_PMOVE_Q2_H

#include "../game/q_shared.h"



typedef struct csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} csurface_t;

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

/*
// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_LAND		16	// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)
*/

#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	//pmove_state_t	s;
	playerState_t* ps;

	// command (in)
	usercmd_t		cmd;
	qboolean		snapinitial;	// if s has been changed outside pmove

	// results (out)
	int			numtouch;
	//struct edict_s* touchents[MAXTOUCH];
	int			touchents[MAXTOUCH];

	//vec3_t		viewangles;			// clamped
	//float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	//struct edict_s* groundentity;
	int			watertype;
	int			waterlevel;

	int			tracemask;

	// callbacks to test the world
	//trace_t(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	void		(*trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
	//int			(*pointcontents) (vec3_t point); 
	int			(*pointcontents)(const vec3_t point, int passEntityNum);
	qboolean	haveQ2StyleTrace;
	int			debugLevel;
	int			cornerSkims; 
	// 0 = no/default q2 behavior. 
	// 1 = restore intended q2 behavior (based on downward speed like q3 but with length variation too). can't jump during pm_time. feels bad.
	// 2 = like 1, but let us jump. all following options let us jump
	// 3 = yes, when downward speed -200. 
	// 4 = yes, when downward speed -200, long pm_time version.
	// 5 = yes, always. 
	// 6 = yes, always, long pm_time version
	float		airAccelerate;
} pmoveq2_t;




#endif

