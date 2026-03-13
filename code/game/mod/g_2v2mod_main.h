#ifndef G_2V2MOD_MAIN_H
#define G_2V2MOD_MAIN_H

#define TVT_SPAWN		0
#define TVT_FIRST_SPAWN	1

// Global mod struct
typedef struct {
	int		spawnArmor[2];		
	int		spawnItems[2];
} tvt_ModState_t;

void G_TvT_Init(void);

#endif
