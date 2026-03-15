#ifndef G_2V2MOD_MAIN_H
#define G_2V2MOD_MAIN_H

#define TVT_SPAWN 0
#define TVT_FIRST_SPAWN 1

#define TVT_PRINT_MAX_JOBS 8

typedef struct {
    int   clientNum;
    char *text;
    int   offset;
    char  lastColor;
} tvt_printJob_t;

typedef struct {
    int kills;
    int deaths;
    int suicides;
    int teamKills;
    int dmgGiven;
    int dmgReceived;
    int teamDmg;
} tvt_ClientStats_t;

// Client mod struct
typedef struct {
    tvt_ClientStats_t stats;
} tvt_ClientState_t;

// Global mod struct
typedef struct {
    int            spawnArmor[2];
    int            spawnItems[2];
    tvt_printJob_t printJobs[TVT_PRINT_MAX_JOBS];
    int            numPrintJobs;
} tvt_ModState_t;

void G_TvT_Init(void);

#endif
