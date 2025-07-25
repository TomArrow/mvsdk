#include "ui_local.h"
#include "ui_skin.h"
static speciePart_t specieParts = PART_NONE;
static qboolean modelChanged = qfalse;
void UI_LoadSkins(void)
{
	char path[MAX_QPATH] = "";
	char *skin = NULL;
	char *p = NULL;
	int i = 0;
	char *model = path;
	char modelPrevious[MAX_QPATH] = "";
	char modelAbsolutePath[MAX_QPATH] = "";
	void *ghoul2 = NULL;
	qboolean precacheSkin = qfalse;
	int fileCount = trap_FS_CreateFileList("models/players/", ".skin");
	UI_LoadSkinAlloc();
	for (i = 0; i < fileCount; i++)
	{
		trap_FS_GetNextFile(path, sizeof(path));
		//Com_Printf("%04d: %s\n", i, path);
		p = strchr(path, '/');
		if (p == NULL)
		{
			continue;
		}
		*p = '\0';
		p++;
		if (strchr(p, '/') != NULL)
		{
			continue;
		}
		skin = p;
		p = Q_strrchr(skin, '.');
		if (p == NULL)
		{
			continue;
		}
		*p = '\0';
		if (Q_stricmp(model, modelPrevious) != 0)
		{
			Q_strncpyz(modelPrevious, model, sizeof(modelPrevious));
			if (ui_PrecacheModels.integer)
			{
				ghoul2 = NULL;
				Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/model.glm", model);
				trap_G2API_InitGhoul2Model(&ghoul2, modelAbsolutePath, 0, 0, 0, 0, 0);
			}
			modelChanged = qtrue;
		}
		if (Q_stricmpn(skin, "model_", 6) == 0)
		{
			UI_LoadSkinDefault(model, &skin[6]);
			precacheSkin = qtrue;
		}
		else if (Q_stricmpn(skin, "head_", 5) == 0)
		{
			UI_LoadSkinHead(model, skin);
			precacheSkin = qtrue;
		}
		else if (Q_stricmpn(skin, "torso_", 6) == 0)
		{
			UI_LoadSkinTorso(model, skin);
			precacheSkin = qtrue;
		}
		else if (Q_stricmpn(skin, "lower_", 6) == 0)
		{
			UI_LoadSkinLower(model, skin);
			precacheSkin = qtrue;
		}
		if (ui_PrecacheModels.integer && precacheSkin)
		{
			precacheSkin = qfalse;
			Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/%s.skin", model, skin);
			trap_R_RegisterSkin(modelAbsolutePath);
		}
	}
	modelChanged = qtrue;
	UI_CheckSpecie(NULL, NULL, qfalse);
	trap_FS_CloseFileList();
}
void UI_LoadSkinAlloc(void)
{
	uiInfo.q3HeadCount = 0;
	uiInfo.playerSpeciesCount = -1;
	uiInfo.playerSpeciesIndex = 0;
	specieParts = PART_NONE;
#ifdef DYNAMIC_PLAYER_SPECIES
	uiInfo.q3HeadsMax = 128;
	uiInfo.playerSpeciesMax = 8;
	uiInfo.q3Heads = (q3Head_t *)trap_Z_Malloc(sizeof(q3Head_t) * uiInfo.q3HeadsMax, TAG_UI, qfalse);
	uiInfo.playerSpecies = (playerSpeciesInfo_t *)trap_Z_Malloc(uiInfo.playerSpeciesMax * sizeof(playerSpeciesInfo_t), TAG_UI, qfalse);
#endif
}
qboolean UI_LoadSpecie(const char *model, const char *skin)
{
	playerSpeciesInfo_t *species;
#ifdef DYNAMIC_PLAYER_SPECIES
	if (uiInfo.playerSpeciesCount >= uiInfo.playerSpeciesMax)
	{
		uiInfo.playerSpeciesMax *= 2;
		uiInfo.playerSpecies = (playerSpeciesInfo_t *)trap_Z_Realloc(uiInfo.playerSpecies, uiInfo.playerSpeciesMax * sizeof(playerSpeciesInfo_t), qfalse);
	}
#else
	if (uiInfo.playerSpeciesCount >= MAX_PLAYERMODELS)
	{
		uiInfo.playerSpeciesCount = MAX_PLAYERMODELS;
		return qfalse;
	}
#endif
	species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
	Q_strncpyz(species->Name, model, MAX_QPATH);
	species->SkinHeadCount = 0;
	species->SkinTorsoCount = 0;
	species->SkinLegCount = 0;
	species->ColorCount = 0;
#ifdef DYNAMIC_PLAYER_SPECIES
	species->SkinHeadMax = 8;
	species->SkinTorsoMax = 8;
	species->SkinLegMax = 8;
	species->ColorMax = 16;
	species->SkinHead = (skinName_t *)trap_Z_Malloc(species->SkinHeadMax * sizeof(skinName_t), TAG_UI, qfalse);
	species->SkinTorso = (skinName_t *)trap_Z_Malloc(species->SkinTorsoMax * sizeof(skinName_t), TAG_UI, qfalse);
	species->SkinLeg = (skinName_t *)trap_Z_Malloc(species->SkinLegMax * sizeof(skinName_t), TAG_UI, qfalse);
	species->Color = (playerColor_t *)trap_Z_Malloc(species->ColorMax * sizeof(playerColor_t), TAG_UI, qfalse);
#endif
	UI_ParseColorData(model, species);
	specieParts = PART_NONE;
	return qtrue;
}
qboolean UI_CheckSpecie(const char *model, const char *skin, qboolean load)
{
	if (modelChanged)
	{
		modelChanged = qfalse;
		if (specieParts != PART_ALL && uiInfo.playerSpeciesCount != -1)
		{
			playerSpeciesInfo_t *species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
			UI_FreeSpecies(species);
		}
		else
		{
			uiInfo.playerSpeciesCount++;
		}
		if (load)
		{
			return UI_LoadSpecie(model, skin);
		}
	}
#ifndef DYNAMIC_PLAYER_SPECIES
	if (uiInfo.playerSpeciesCount >= MAX_PLAYERMODELS)
	{
		uiInfo.playerSpeciesCount = MAX_PLAYERMODELS;
		return qfalse;
	}
#endif
	return qtrue;
}
void UI_LoadSkinHead(const char *model, const char *skin)
{
	playerSpeciesInfo_t *species;
	char modelAbsolutePath[MAX_QPATH];
	qhandle_t icon;
	if (!UI_CheckSpecie(model, skin, qtrue))
	{
		return;
	}
	species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
#ifdef DYNAMIC_PLAYER_SPECIES
	if (species->SkinHeadCount >= species->SkinHeadMax)
	{
		species->SkinHeadMax *= 2;
		species->SkinHead = (skinName_t *)trap_Z_Realloc(species->SkinHead, species->SkinHeadMax * sizeof(skinName_t), qfalse);
	}
#else
	if (species->SkinHeadCount >= MAX_PLAYERMODELS)
	{
		species->SkinHeadCount = MAX_PLAYERMODELS;
		return;
	}
#endif
	Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/icon_%s", model, skin);
	icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	if (icon == 0)
	{
		Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/mb2_icon_%s", model, skin);
		icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	}
	if (icon == 0)
	{
		if (ui_loadSkinsWithoutIcons.integer == 0)
		{
			return;
		}
		icon = uiInfo.uiDC.Assets.unknownModelIcon;
	}
	Q_strncpyz(species->SkinHead[species->SkinHeadCount].name, skin, SKIN_LENGTH);
	species->SkinHead[species->SkinHeadCount].icon = icon;
	species->SkinHeadCount++;
	specieParts |= PART_HEAD;
}
void UI_LoadSkinTorso(const char *model, const char *skin)
{
	playerSpeciesInfo_t *species;
	char modelAbsolutePath[MAX_QPATH];
	qhandle_t icon;
	if (!UI_CheckSpecie(model, skin, qtrue))
	{
		return;
	}
	species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
#ifdef DYNAMIC_PLAYER_SPECIES
	if (species->SkinTorsoCount >= species->SkinTorsoMax)
	{
		species->SkinTorsoMax *= 2;
		species->SkinTorso = (skinName_t *)trap_Z_Realloc(species->SkinTorso, species->SkinTorsoMax * sizeof(skinName_t), qfalse);
	}
#else
	if (species->SkinTorsoCount >= MAX_PLAYERMODELS)
	{
		species->SkinTorsoCount = MAX_PLAYERMODELS;
		return;
	}
#endif
	Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/icon_%s", model, skin);
	icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	if (icon == 0)
	{
		Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/mb2_icon_%s", model, skin);
		icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	}
	if (icon == 0)
	{
		if (ui_loadSkinsWithoutIcons.integer == 0)
		{
			return;
		}
		icon = uiInfo.uiDC.Assets.unknownModelIcon;
	}
	Q_strncpyz(species->SkinTorso[species->SkinTorsoCount].name, skin, SKIN_LENGTH);
	species->SkinTorso[species->SkinTorsoCount].icon = icon;
	species->SkinTorsoCount++;
	specieParts |= PART_TORSO;
}
void UI_LoadSkinLower(const char *model, const char *skin)
{
	playerSpeciesInfo_t *species;
	char modelAbsolutePath[MAX_QPATH];
	qhandle_t icon;
	if (!UI_CheckSpecie(model, skin, qtrue))
	{
		return;
	}
	species = &uiInfo.playerSpecies[uiInfo.playerSpeciesCount];
#ifdef DYNAMIC_PLAYER_SPECIES
	if (species->SkinLegCount >= species->SkinLegMax)
	{
		species->SkinLegMax *= 2;
		species->SkinLeg = (skinName_t *)trap_Z_Realloc(species->SkinLeg, species->SkinLegMax * sizeof(skinName_t), qfalse);
	}
#else
	if (species->SkinLegCount >= MAX_PLAYERMODELS)
	{
		species->SkinLegCount = MAX_PLAYERMODELS;
		return;
	}
#endif
	Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/icon_%s", model, skin);
	icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	if (icon == 0)
	{
		Com_sprintf(modelAbsolutePath, sizeof(modelAbsolutePath), "models/players/%s/mb2_icon_%s", model, skin);
		icon = trap_R_RegisterShaderNoMip(modelAbsolutePath);
	}
	if (icon == 0)
	{
		if (ui_loadSkinsWithoutIcons.integer == 0)
		{
			return;
		}
		icon = uiInfo.uiDC.Assets.unknownModelIcon;
	}
	Q_strncpyz(species->SkinLeg[species->SkinLegCount].name, skin, SKIN_LENGTH);
	species->SkinLeg[species->SkinLegCount].icon = icon;
	species->SkinLegCount++;
	specieParts |= PART_LOWER;
}
void UI_LoadSkinDefault(const char *model, const char *skin)
{
	q3Head_t *newHead;
	char name[MAX_QPATH * 2];
	const char *p;
	char *p2;
#ifdef DYNAMIC_PLAYER_SPECIES
	if (uiInfo.q3HeadCount >= uiInfo.q3HeadsMax)
	{
		uiInfo.q3HeadsMax *= 2;
		uiInfo.q3Heads = (q3Head_t *)trap_Z_Realloc(uiInfo.q3Heads, sizeof(q3Head_t) * uiInfo.q3HeadsMax, qfalse);
	}
#else
	if (uiInfo.q3HeadCount >= MAX_Q3PLAYERMODELS)
	{
		uiInfo.q3HeadCount = MAX_Q3PLAYERMODELS;
		return;
	}
#endif
	Com_sprintf(name, sizeof(name), "%s/%s", model, skin);
	newHead = &uiInfo.q3Heads[uiInfo.q3HeadCount];
	Q_strncpyz(newHead->name, name, MAX_QPATH);
	p = skin;
	if (*p == '|')
	{
		p++;
	}
	Com_sprintf(name, sizeof(name), "models/players/%s/icon_%s", model, p);
	p2 = strchr(name, '|');
	if (p2 != NULL)
	{
		*p2 = '\0';
	}
	newHead->icon = trap_R_RegisterShaderNoMip(name);
	if (newHead->icon == 0)
	{
		Com_sprintf(name, sizeof(name), "models/players/%s/mb2_icon_%s", model, p);
		p2 = strchr(name, '|');
		if (p2 != NULL)
		{
			*p2 = '\0';
		}
		newHead->icon = trap_R_RegisterShaderNoMip(name);
	}
	if (newHead->icon == 0)
	{
		if (ui_loadSkinsWithoutIcons.integer == 0)
		{
			return;
		}
		newHead->icon = uiInfo.uiDC.Assets.unknownModelIcon;
	}
	uiInfo.q3HeadCount++;
}
void UI_FreeSpecies(playerSpeciesInfo_t *species)
{
#ifdef DYNAMIC_PLAYER_SPECIES
	trap_Z_Free(species->SkinHead);
	trap_Z_Free(species->SkinTorso);
	trap_Z_Free(species->SkinLeg);
	trap_Z_Free(species->Color);
#endif
}
void UI_FreeAllSpecies(void)
{
	int i;
	for (i = 0; i < uiInfo.playerSpeciesCount; i++)
	{
		UI_FreeSpecies(&uiInfo.playerSpecies[i]);
	}
#ifdef DYNAMIC_PLAYER_SPECIES
	trap_Z_Free(uiInfo.playerSpecies);
#endif
}
void UI_ParseColorData(const char *model, playerSpeciesInfo_t *species)
{
	char filePath[MAX_QPATH];
	int fileLength;
	fileHandle_t fileHandle;
	const char *token;
	const char *p;
#ifdef DYNAMIC_FILE_LIST
	char *fileBuffer;
#else
	char fileBuffer[4096];
#endif
	Com_sprintf(filePath, sizeof(filePath), "models/players/%s/PlayerChoice.txt", model);
	COM_BeginParseSession(filePath);
	fileLength = trap_FS_FOpenFile(filePath, &fileHandle, FS_READ);
	if (fileLength <= 0 || fileHandle <= 0)
	{
		return;
	}
#ifdef DYNAMIC_FILE_LIST
	fileBuffer = (char *) trap_Z_Malloc((fileLength + 1) * sizeof(char), TAG_UI, qfalse);
#else
	if (fileLength >= sizeof(fileBuffer))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: UI_ParseColorData: file \"%s\" is too large (max is %d bytes)\n", filePath, sizeof(fileBuffer) - 1);
		trap_FS_FCloseFile(fileHandle);
		return;
	}
#endif
	fileBuffer[0] = '\0';
	trap_FS_Read(fileBuffer, fileLength, fileHandle);
	fileBuffer[fileLength] = '\0';
	trap_FS_FCloseFile(fileHandle);
	p = fileBuffer;
	while (p)
	{
		token = COM_ParseExt(&p, qtrue);	//looking for the shader
		if (token[0] == 0)
		{
			goto freeBuffer;
		}
#ifdef DYNAMIC_PLAYER_SPECIES
		if (species->ColorCount >= species->ColorMax)
		{
			species->ColorMax *= 2;
			species->Color = (playerColor_t*)trap_Z_Realloc(species->Color, species->ColorMax * sizeof(playerColor_t), qfalse);
		}
#else
		if (species->ColorCount >= MAX_PLAYERMODELS)
		{
			species->ColorCount = MAX_PLAYERMODELS;
			goto parseError;
		}
#endif
		Q_strncpyz(species->Color[species->ColorCount].shader, token, MAX_QPATH);
		species->Color[species->ColorCount].actionText[0] = '\0';
		token = COM_ParseExt(&p, qtrue);	//looking for action block {
		if (token[0] != '{')
		{
			goto parseError;
		}
		token = COM_ParseExt(&p, qtrue);	//looking for action commands
		while (token[0] != '}')
		{
			if (token[0] == 0)
			{	//EOF
				goto parseError;
			}
			Q_strcat(species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, token);
			Q_strcat(species->Color[species->ColorCount].actionText, ACTION_BUFFER_SIZE, " ");
			token = COM_ParseExt(&p, qtrue);	//looking for action commands or final }
		}
		species->ColorCount++;	//next color please
	}
parseError:
	Com_Printf(S_COLOR_YELLOW "WARNING: UI_ParseColorData: Errors parsing \"%s\"\n", filePath);
freeBuffer:
#ifdef DYNAMIC_FILE_LIST
	trap_Z_Free(fileBuffer);
#endif
	return;
}
