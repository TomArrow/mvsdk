// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

//static int			loadingPlayerIconCount;
//static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];

void CG_LoadBar(void);

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;

	item = &bg_itemlist[itemNum];

	CG_LoadingString( CG_GetStripEdString("INGAME",item->classname) );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			personality[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

/*
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];
	char			*skin;
	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = Q_strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );
		
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}
*/
	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
<<<<<<< HEAD
	//Q_CleanStr( personality, (qboolean)(jk2startversion == VERSION_1_02) );
=======
	Q_CleanStr( personality, (qboolean)(jk2startversion == VERSION_1_02) );
>>>>>>> jediknightplus/master

	/*
	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ));
	}
	*/

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
#define UI_INFOFONT (UI_BIGFONT)
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	const float	x = 0.5f * cgs.screenWidth;
<<<<<<< HEAD
	const float xOffset = 0.5f * (cgs.screenWidth - SCREEN_WIDTH);
	float		y;
=======
	int			y;
>>>>>>> jediknightplus/master
	int			value, valueNOFP;
	qhandle_t	levelshot;
	char		buf[1024];
	int			iPropHeight = 18;	// I know, this is total crap, but as a post release asian-hack....  -Ste

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapname" );
<<<<<<< HEAD

	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s", s));
	trap_R_SetColor(NULL);

	if (levelshot && cgs.screenXFactor < 1.0) {
		CG_DrawPic(0, 0 - (SCREEN_HEIGHT*cgs.screenXFactorInv - SCREEN_HEIGHT) / 2, cgs.screenWidth, SCREEN_HEIGHT*cgs.screenXFactorInv, levelshot);
	}
	else {
		if (!levelshot) {
			levelshot = trap_R_RegisterShaderNoMip("menu/art/unknownmap");
			CG_FillRect(0, 0, xOffset, SCREEN_HEIGHT, colorTable[CT_BLACK]);
			CG_FillRect(xOffset + SCREEN_WIDTH, 0, xOffset, SCREEN_HEIGHT, colorTable[CT_BLACK]);
			CG_DrawPic(xOffset, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot);
		}
		else {
			CG_DrawPic(0, 0, cgs.screenWidth, SCREEN_HEIGHT, levelshot);
		}
	}
=======
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	trap_R_SetColor( NULL );
	CG_DrawPic(0, 0, cgs.screenWidth, cgs.screenHeight, levelshot);
>>>>>>> jediknightplus/master

	CG_LoadBar();
				   
	// draw the icons of things as they are loaded
//	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		const char *psLoading = CG_GetStripEdString("MENUS3", "LOADING_MAPNAME");
		UI_DrawProportionalString( x, (cgs.screenHeight*0.27f)-2-32, va(/*"Loading... %s"*/ psLoading, cg.infoScreenText),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
	} else {
		const char *psAwaitingSnapshot = CG_GetStripEdString("MENUS3", "AWAITING_SNAPSHOT");
		UI_DrawProportionalString( x, (cgs.screenHeight*0.27f)-2-32, /*"Awaiting snapshot..."*/psAwaitingSnapshot,
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
	}

	// draw info string information

	y = (cgs.screenHeight*0.375f)-32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey( info, "sv_hostname" ), 1024);
<<<<<<< HEAD
		Q_CleanAsciiStr(buf);
=======
		Q_CleanStr(buf, (qboolean)(jk2startversion == VERSION_1_02));
>>>>>>> jediknightplus/master
		UI_DrawProportionalString( x, y, buf,
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			const char *psPure = CG_GetStripEdString("INGAMETEXT", "PURE_SERVER");
			UI_DrawProportionalString( x, y, psPure,
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			y += iPropHeight;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( x, y, s,
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			y += iPropHeight;
		}

		{	// display global MOTD at bottom (mirrors ui_main UI_DrawConnectScreen
			char motdString[1024];
			trap_Cvar_VariableStringBuffer( "cl_motdString", motdString, sizeof( motdString ) );

			if (motdString[0])
			{
				UI_DrawProportionalString( x, cgs.screenHeight-55, motdString,
					UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			}
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( x, y, s,
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( x, y, CG_GetStripEdString("INGAMETEXT", "CHEATSAREENABLED"),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}

	// game type
	switch ( cgs.gametype ) {
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_HOLOCRON:
		s = "Holocron FFA";
		break;
	case GT_JEDIMASTER:
		s = "Jedi Master";
		break;
	case GT_SINGLE_PLAYER:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Duel";
		break;
	case GT_TEAM:
		s = "Team FFA";
		break;
	case GT_SAGA:
		s = "N/A";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
	case GT_CTY:
		s = "Capture The Ysalamiri";
		break;
	default:
		s = "Unknown Gametype";
		break;
	}
	UI_DrawProportionalString( x, y, s,
		UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
	y += iPropHeight;
		
	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		UI_DrawProportionalString( x, y,
			va( "%s %i", CG_GetStripEdString("INGAMETEXT", "TIMELIMIT"), value ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}

	if (cgs.gametype < GT_CTF ) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			UI_DrawProportionalString( x, y,
				va( "%s %i", CG_GetStripEdString("INGAMETEXT", "FRAGLIMIT"), value ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			y += iPropHeight;
		}

		if (cgs.gametype == GT_TOURNAMENT)
		{
			value = atoi( Info_ValueForKey( info, "duel_fraglimit" ) );
			if ( value ) {
				UI_DrawProportionalString( x, y,
					va( "%s %i", CG_GetStripEdString("INGAMETEXT", "WINLIMIT"), value ),
					UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
				y += iPropHeight;
			}
		}
	}

	if (cgs.gametype >= GT_CTF) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( x, y, va( "%s %i", CG_GetStripEdString("INGAMETEXT", "CAPTURELIMIT"), value ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			y += iPropHeight;
		}
	}

	if (cgs.gametype >= GT_TEAM)
	{
		value = atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) );
		if ( value ) {
			UI_DrawProportionalString( x, y, CG_GetStripEdString("INGAMETEXT", "FORCEBASEDTEAMS"),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
			y += iPropHeight;
		}
	}

	valueNOFP = atoi( Info_ValueForKey( info, "g_forcePowerDisable" ) );

	value = atoi( Info_ValueForKey( info, "g_maxForceRank" ) );
	if ( value && !valueNOFP ) {
		char fmStr[1024];
		char *text;

		trap_SP_GetStringTextString("INGAMETEXT_MAXFORCERANK",fmStr, sizeof(fmStr));

<<<<<<< HEAD
		if (value > 6)
			value = 6;

=======
>>>>>>> jediknightplus/master
		if ( value >= 0 && value < (int)ARRAY_LEN(forceMasteryLevels) ) {
			text = va( "%s %s", fmStr, CG_GetStripEdString("INGAMETEXT", forceMasteryLevels[value]) );
		} else {
			text = va( "%s %i", fmStr, value );
		}

		UI_DrawProportionalString( x, y, text, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}
	else if (!valueNOFP)
	{
		char fmStr[1024];
		trap_SP_GetStringTextString("INGAMETEXT_MAXFORCERANK",fmStr, sizeof(fmStr));

		UI_DrawProportionalString( x, y, va( "%s %s", fmStr, (char *)CG_GetStripEdString("INGAMETEXT", forceMasteryLevels[7]) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}

	if (cgs.gametype == GT_TOURNAMENT)
	{
		value = atoi( Info_ValueForKey( info, "g_duelWeaponDisable" ) );
	}
	else
	{
		value = atoi( Info_ValueForKey( info, "g_weaponDisable" ) );
	}
	if ( cgs.gametype != GT_JEDIMASTER && value ) {
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "SABERONLYSET") ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}

	if ( valueNOFP ) {
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "NOFPSET") ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
	}


	// Display the rules based on type
		y += iPropHeight;
	switch ( cgs.gametype ) {
	case GT_FFA:					
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_FFA_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_HOLOCRON:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_HOLO_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_HOLO_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_JEDIMASTER:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_JEDI_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_JEDI_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_SINGLE_PLAYER:
		break;
	case GT_TOURNAMENT:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_DUEL_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_DUEL_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_TEAM:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_TEAM_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_TEAM_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_SAGA:
		break;
	case GT_CTF:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_CTF_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_CTF_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	case GT_CTY:
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_CTY_1")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( x, y, va( "%s", (char *)CG_GetStripEdString("INGAMETEXT", "RULES_CTY_2")),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, colorWhite );
		y += iPropHeight;
		break;
	default:
		break;
	}
}

/*
===================
CG_LoadBar
===================
*/
void CG_LoadBar(void)
{
	const int numticks = 9, tickwidth = 40, tickheight = 8;
	const int tickpadx = 20, tickpady = 12;
	const int capwidth = 8;
	const float barwidth = numticks*tickwidth+tickpadx*2+capwidth*2;
	const float barleft = 0.5f * (cgs.screenWidth - barwidth);
	const int barheight = tickheight + tickpady*2, bartop = cgs.screenHeight-barheight;
	const float capleft = barleft+tickpadx;
	const float tickleft = capleft+capwidth;
	const float ticktop = bartop+tickpady;

	trap_R_SetColor( colorWhite );
	// Draw background
	CG_DrawPic(barleft, bartop, barwidth, barheight, cgs.media.loadBarLEDSurround);

	// Draw left cap (backwards)
	CG_DrawPic(tickleft, ticktop, -capwidth, tickheight, cgs.media.loadBarLEDCap);

	// Draw bar
	CG_DrawPic(tickleft, ticktop, tickwidth*cg.loadLCARSStage, tickheight, cgs.media.loadBarLED);

	// Draw right cap
	CG_DrawPic(tickleft+tickwidth*cg.loadLCARSStage, ticktop, capwidth, tickheight, cgs.media.loadBarLEDCap);
}

