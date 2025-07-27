#ifndef UI_SKIN_H
#define UI_SKIN_H
enum speciePart_e
{
	PART_NONE = 0,
	PART_HEAD = 1 << 0,
	PART_TORSO = 1 << 1,
	PART_LOWER = 1 << 2,
	PART_ALL = PART_HEAD | PART_TORSO | PART_LOWER
};
typedef enum speciePart_e speciePart_t;
void UI_LoadSkins(void);
void UI_LoadSkinAlloc(void);
qboolean UI_LoadSpecie(const char *model, const char *skin);
qboolean UI_CheckSpecie(const char *model, const char *skin, qboolean load);
void UI_LoadSkinHead(const char *model, const char *skin);
void UI_LoadSkinTorso(const char *model, const char *skin);
void UI_LoadSkinLower(const char *model, const char *skin);
void UI_LoadSkinDefault(const char *model, const char *skin);
void UI_FreeSpecies(playerSpeciesInfo_t *species);
void UI_FreeAllSpecies(void);
void UI_ParseColorData(const char *model, playerSpeciesInfo_t *species);
#endif
