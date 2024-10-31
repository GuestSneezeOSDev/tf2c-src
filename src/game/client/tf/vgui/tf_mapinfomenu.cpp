//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "IGameUIFuncs.h" // for key bindings

#ifdef _WIN32
#include "winerror.h"
#endif
#include "ixboxsystem.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_mapinfomenu.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );
const char *GetMapType( const char *mapName, bool bFourTeam = false );
const char *GetMapAuthor( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::CTFMapInfoMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_MAPINFO)
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( ClientSchemesArray[SCHEME_CLIENT_STRING] );

	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pTitle = new CExLabel(this, "MapInfoTitle", " ");

	m_pContinue = new CExButton( this, "MapInfoContinue", "#TF_Continue" );
	m_pBack = new CExButton( this, "MapInfoBack", "#TF_Back" );
	m_pIntro = new CExButton( this, "MapInfoWatchIntro", "#TF_WatchIntro" );

	// info window about this map
	m_pMapInfo = new CExRichText( this, "MapInfoText" );
	m_pMapImage = new ImagePanel( this, "MapImage" );

	m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::~CTFMapInfoMenu()
{
}


void CTFMapInfoMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/MapInfoMenu.res");

	CheckIntroState();
	CheckBackContinueButtons();

	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

	// Save off the map name so we can re-load the page in ApplySchemeSettings().
	V_strcpy_safe( m_szMapName, mapname );

	const char *szGameType = GetMapType( m_szMapName );

	// fallback in case the map does not have a matching prefix.
	if( !Q_strcmp( szGameType, "" ) && GameRules() )
	{
		szGameType = GameRules()->GetGameTypeName();
	}
	SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( szGameType ) );

	Q_strupr( m_szMapName );

	LoadMapPage( m_szMapName );
	SetMapTitle();

	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
}


void CTFMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	m_KeyRepeat.Reset();

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		CheckIntroState();
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}


bool CTFMapInfoMenu::CheckForIntroMovie()
{
	if ( TFGameRules() && g_pFullFileSystem->FileExists( TFGameRules()->GetVideoFileForMap() ) )
		return true;

	return false;
}

const char *COM_GetModDirectory();


bool CTFMapInfoMenu::HasViewedMovieForMap()
{
	return ( UTIL_GetMapKeyCount( "viewed" ) > 0 );
}


void CTFMapInfoMenu::CheckIntroState()
{
	if ( CheckForIntroMovie() && HasViewedMovieForMap() )
	{
		if ( m_pIntro && !m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( true );
		}
	}
	else
	{
		if ( m_pIntro && m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( false );
		}
	}
}


void CTFMapInfoMenu::CheckBackContinueButtons()
{
	if ( m_pBack && m_pContinue )
	{
		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			m_pBack->SetVisible( true );
			m_pContinue->SetText( "#TF_Continue" );
		}
		else
		{
			m_pBack->SetVisible( false );
			m_pContinue->SetText( "#TF_Close" );
		}
	}
}


void CTFMapInfoMenu::OnCommand( const char *command )
{
	m_KeyRepeat.Reset();

	if ( !Q_strcmp( command, "back" ) )
	{
		 // only want to go back to the Welcome menu if we're not already on a team
		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_INFO, true );
		}
	}
	else if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() && !HasViewedMovieForMap() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );	
		}
		else if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			GetTFViewPort()->ShowTeamMenu( true );
		}

		UTIL_IncrementMapKey( "viewed" );
	}
	else if ( !Q_strcmp( command, "intro" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );
		}
		else
		{
			GetTFViewPort()->ShowTeamMenu( true );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


void CTFMapInfoMenu::Update()
{
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::LoadMapPage( const char *mapName )
{
	// load the map image (if it exists for the current map)
	char szMapImage[ MAX_PATH ];
	V_sprintf_safe( szMapImage, "VGUI/maps/menu_photos_%s", mapName );
	Q_strlower( szMapImage );

	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if ( m_pMapImage )
		{
			if ( !m_pMapImage->IsVisible() )
			{
				m_pMapImage->SetVisible( true );
			}

			// take off the vgui/ at the beginning when we set the image
			V_sprintf_safe( szMapImage, "maps/menu_photos_%s", mapName );
			Q_strlower( szMapImage );

			m_pMapImage->SetImage( szMapImage );
		}
	}
	else
	{
		if ( m_pMapImage && m_pMapImage->IsVisible() )
		{
			m_pMapImage->SetVisible( false );
		}
	}

	// load the map description files
	char mapLocalizationString[ MAX_PATH ];
	V_sprintf_safe( mapLocalizationString, "%s_description", mapName );
	const wchar_t *pLocalized = g_pVGuiLocalize->Find( mapLocalizationString );
	if ( pLocalized )
	{
		m_pMapInfo->SetText( pLocalized );
	}
	else // try loading from mapname.txt files
	{
		char mapRES[MAX_PATH];

		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

		V_snprintf( mapRES, sizeof( mapRES ), "maps/%s_%s.txt", mapName, uilanguage );

		// _english fallback if language missing
		if ( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			V_snprintf( mapRES, sizeof( mapRES ), "maps/%s_english.txt", mapName );

			// lastly, try with no language suffix
			if ( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
			{
				V_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName );
			}
		}

		// found mapname.txt...
		if ( g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			FileHandle_t f = g_pFullFileSystem->Open( mapRES, "r" );

			// read into a memory block
			int fileSize = g_pFullFileSystem->Size( f );
			int dataSize = fileSize + sizeof( wchar_t );
			if ( dataSize % 2 )
				++dataSize;
			wchar_t *memBlock = (wchar_t *)malloc( dataSize );
			memset( memBlock, 0x0, dataSize );
			int bytesRead = g_pFullFileSystem->Read( memBlock, fileSize, f );
			if ( bytesRead < fileSize )
			{
				// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
				// return fewer bytes than we were expecting.
				char *data = reinterpret_cast<char *>( memBlock );
				data[bytesRead] = 0;
				data[bytesRead + 1] = 0;
			}

#ifndef WIN32
			if ( ( (ucs2 *)memBlock )[0] == 0xFEFF )
			{
				// convert the win32 ucs2 data to wchar_t
				dataSize *= 2;// need to *2 to account for ucs2 to wchar_t (4byte) growth
				wchar_t *memBlockConverted = (wchar_t *)malloc( dataSize );
				V_UCS2ToUnicode( (ucs2 *)memBlock, memBlockConverted, dataSize );
				free( memBlock );
				memBlock = memBlockConverted;
			}
#else
			// null-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
			memBlock[dataSize / sizeof( wchar_t ) - 1] = 0x0000;
#endif
			// ensure little-endian unicode reads correctly on all platforms
			CByteswap byteSwap;
			byteSwap.SetTargetBigEndian( false );
			byteSwap.SwapBufferToTargetEndian( memBlock, memBlock, dataSize / sizeof( wchar_t ) );

			// check the first character, make sure this a little-endian unicode file
			if ( memBlock[0] != 0xFEFF )
			{
				// its a ascii char file
				m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
			}
			else
			{
				m_pMapInfo->SetText( memBlock + 1 );
			}
			// go back to the top of the text buffer
			m_pMapInfo->GotoTextStart();

			g_pFullFileSystem->Close( f );
			free( memBlock );

			//InvalidateLayout();
			//Repaint();
		}
		else // if no map specific description exists, load default text
		{
			if ( TFGameRules() )
			{
				const char *pszGameTypeAbbreviation = NULL;
				switch ( TFGameRules()->GetGameType() )
				{
				case TF_GAMETYPE_CTF:
					pszGameTypeAbbreviation = "ctf";
					break;
				case TF_GAMETYPE_CP:
					if (TFGameRules()->IsInArenaMode())
						pszGameTypeAbbreviation = "arena";
					else if (TFGameRules()->IsInKothMode())
						pszGameTypeAbbreviation = "koth";
					else if (TFGameRules()->IsInDominationMode())
						pszGameTypeAbbreviation = "dom";
					else if (TFGameRules()->IsInTerritorialDominationMode())
						pszGameTypeAbbreviation = "td";
					else
						pszGameTypeAbbreviation = "cp";
					break;
				case TF_GAMETYPE_ESCORT:
					pszGameTypeAbbreviation = TFGameRules()->HasMultipleTrains() ? "payload_race" : "payload";
					break;
				case TF_GAMETYPE_ARENA:	// never gets triggered unless we are playing on pure arena map with no control points
					pszGameTypeAbbreviation = "arena";
					break;
				case TF_GAMETYPE_VIP:
					pszGameTypeAbbreviation = "vip";
					break;
				case TF_GAMETYPE_POWERSIEGE:
					pszGameTypeAbbreviation = "ps";
					break;
				case TF_GAMETYPE_INFILTRATION:
					pszGameTypeAbbreviation = "inf";
					break;
				}

				if ( pszGameTypeAbbreviation )
				{
					V_sprintf_safe( mapLocalizationString, "default_%s_description", pszGameTypeAbbreviation );
					pLocalized = g_pVGuiLocalize->Find( mapLocalizationString );
				}
			}
			m_pMapInfo->SetText( pLocalized ? pLocalized : L"" );
		}
	}

	// we haven't loaded a valid map image for the current map
	if ( m_pMapImage && !m_pMapImage->IsVisible() )
	{
		if ( m_pMapInfo )
		{
			m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
		}
	}
}


void CTFMapInfoMenu::SetMapTitle()
{
	SetDialogVariable( "mapname", GetMapDisplayName( m_szMapName ) );
}


void CTFMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "continue" );
	}
	else if ( code == KEY_XBUTTON_Y )
	{
		OnCommand( "intro" );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", 1) );
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", -1) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


void CTFMapInfoMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}


void CTFMapInfoMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	BaseClass::OnThink();
}

static s_MapInfo s_Maps[] =
{
    //---------------------- CTF maps ----------------------
	{ "ctf_2fort",            "2Fort",            "#Gametype_CTF",                "Valve" },
	{ "ctf_well",             "Well",	          "#Gametype_CTF",                "Valve" },
	{ "ctf_doublecross",      "Double Cross",	  "#Gametype_CTF",                "Valve" },
	{ "ctf_sawmill",          "Sawmill",		  "#Gametype_CTF",                "Valve" },
	{ "ctf_turbine",          "Turbine",		  "#Gametype_CTF",                "Flobster" },
	{ "ctf_casbah",			  "Casbah",			  "#Gametype_CTF",				  "Volcom82, MacD11, Drudlyclean" },
	{ "ctf_landfall",		  "Landfall",		  "#Gametype_CTF",				  "Dr. Spud" },
	{ "ctf_pelican_peak",	  "Pelican Peak",	  "#Gametype_CTF",				  "abp, chin, phi" },
	{ "ctf_penguin_peak",	  "Penguin Peak",	  "#Gametype_CTF_Penguin",		  "abp, chin, phi, erk" },
    //---------------------- CP maps ----------------------
	{ "cp_dustbowl",          "Dustbowl",         "#Gametype_AttackDefense",      "Valve" },
	{ "cp_gravelpit",         "Gravel Pit",       "#Gametype_AttackDefense",      "Valve" },
	{ "cp_gorge",             "Gorge",            "#Gametype_AttackDefense",      "Valve" },
	{ "cp_mountainlab",       "Mountain Lab",     "#Gametype_AttackDefense",      "Valve, 3Dnj" },
	{ "cp_granary",           "Granary",          "#Gametype_CP",                 "Valve" },
	{ "cp_well",              "Well",             "#Gametype_CP",                 "Valve" },
	{ "cp_foundry",           "Foundry",          "#Gametype_CP",                 "Valve" },
	{ "cp_badlands",          "Badlands",         "#Gametype_CP",                 "Valve" },
	{ "cp_degrootkeep",       "Degroot Keep",     "#Gametype_MedAttackDefense",   "Valve" },
	{ "cp_powerhouse",        "Powerhouse",       "#Gametype_CP",                 "Valve" },
	{ "cp_furnace_rc",		  "Furnace Creek",	  "#Gametype_AttackDefense",	  "Nineaxis, Youme" },
	{ "cp_amaranth",		  "Amaranth",		  "#Gametype_AttackDefense",	  "Berry" },
	{ "cp_tidal_v4",		  "Tidal",			  "#Gametype_CP",				  "Heyo" },
	{ "cp_yukon_final",		  "Yukon",			  "#Gametype_CP",				  "MangyCarface, Acegikmo" },
	{ "cp_5gorge",			  "Gorge",			  "#Gametype_CP",				  "Valve" },
	{ "cp_egypt_final",		  "Egypt",			  "#Gametype_AttackDefense",	  "Heyo" },
	{ "cp_fastlane",		  "Fastlane",		  "#Gametype_CP",				  "SK" },
	{ "cp_freight_final1",	  "Freight",		  "#Gametype_CP",				  "Fishbus, Ol" },
	{ "cp_junction_final",	  "Junction",		  "#Gametype_AttackDefense",	  "Heyo" },
	{ "cp_steel",			  "Steel",			  "#Gametype_AttackDefense",	  "Fishbus, Irish Taxi Driver, FLOOR_MASTER" },
	{ "cp_coldfront",		  "Coldfront",		  "#Gametype_CP",				  "Icarus, Selentic, Void, YM" },
	{ "cp_gullywash",  "Gullywash",		  "#Gametype_CP",				  "Arnold" },
    //---------------------- TC maps ----------------------
	{ "tc_hydro",             "Hydro",            "#Gametype_TC",				  "Valve" },
    //---------------------- PL maps ----------------------
	{ "pl_goldrush",          "Gold Rush",        "#Gametype_Escort",             "Valve" },
	{ "pl_badwater",          "Badwater Basin",   "#Gametype_Escort",             "Valve" },
	{ "pl_thundermountain",   "Thunder Mountain", "#Gametype_Escort",             "Valve" },
	{ "pl_barnblitz",         "Barnblitz",        "#Gametype_Escort",             "Valve" },
	{ "pl_upward",            "Upward",           "#Gametype_Escort",			  "Valve" },
	{ "pl_frontier_final",	  "Frontier",		  "#Gametype_Escort",			  "MangyCarface, Arhurt" },
	{ "pl_hoodoo_final",	  "Hoodoo",			  "#Gametype_Escort",			  "YM, Snipergen, Nineaxis, ..oxy.." },
	{ "pl_jinn",	          "Jinn",	          "#Gametype_Escort",			  "abp, 14bit" },
	{ "plr_pipeline",         "Pipeline",         "#Gametype_EscortRace",         "Valve" },
	{ "plr_nightfall_final",  "Nightfall",		  "#Gametype_EscortRace",		  "Psy" },
	{ "plr_hightower",        "Hightower",        "#Gametype_EscortRace",         "Valve" },
    //---------------------- KOTH maps ----------------------
	{ "koth_viaduct",         "Viaduct",		  "#Gametype_Koth",               "Valve" },
	{ "koth_king",            "Kong King",		  "#Gametype_Koth",               "3Dnj"  },
	{ "koth_nucleus",         "Nucleus",		  "#Gametype_Koth",               "Valve" },
	{ "koth_sawmill",         "Sawmill",		  "#Gametype_Koth",               "Valve" },
	{ "koth_badlands",		  "Badlands",		  "#Gametype_Koth",				  "Valve" },
	{ "koth_harvest_final",	  "Harvest",		  "#Gametype_Koth",				  "Heyo" },
	{ "koth_harvest_event",	  "Harvest (Event)",  "#Gametype_Koth",				  "Heyo" },
	{ "koth_highpass",		  "Highpass",		  "#Gametype_Koth",				  "Bloodhound, Psy, Drawer" },
	{ "koth_lakeside_final",  "Lakeside",		  "#Gametype_Koth",				  "ElectroSheep" },
	{ "koth_frigid",		  "Frigid",			  "#Gametype_Koth_FourTeam",	  "Wheat, savva" },
	//---------------------- SD maps ----------------------
	{ "sd_doomsday",          "Doomsday",         "#Gametype_SD",                 "Valve" },
	//---------------------- VIP maps ----------------------
	{ "vip_harbor",			  "Blackstone Harbor", "#Gametype_VIP",				  "Gadget, Drudlyclean, Suomimies55, Hutty" },
	{ "vip_mineside",		  "Mineside",		  "#Gametype_VIP",				  "Suomimies55" },
	{ "vip_trainyard",		  "Trainyard",		  "#Gametype_VIP",				  "theatreTECHIE, Drudlyclean" },
	{ "vip_badwater",		  "Badwater Basin",	  "#Gametype_VIP",				  "Drudlyclean" },
	//---------------------- DOM maps ----------------------
	{ "dom_oilcanyon",		  "Oil Canyon",		  "#Gametype_Domination",		  "MaartenS11, Suomimies55, Trotim, Drudlyclean" },
	{ "dom_hydro",			  "Hydro",			  "#Gametype_Domination_FourTeam", "Drudlyclean" },
	{ "dom_krepost",		  "Krepost",		  "#Gametype_MedDomination_FourTeam", "Suomimies55" },
	//---------------------- TD maps ----------------------
	{ "td_caper",			  "Caper",			  "#Gametype_TD",				  "abp, Emil" },
	//---------------------- ARENA maps ----------------------
	{ "arena_flask",		  "Flask",			  "#Gametype_Arena_FourTeam",	  "Drudlyclean, Trotim" },
	{ "arena_floodgate",	  "Floodgate",		  "#Gametype_Arena_FourTeam",	  "savva" },
	{ "arena_badlands",		  "Badlands",		  "#Gametype_Arena",			  "Valve" },
	{ "arena_granary",		  "Granary",		  "#Gametype_Arena",			  "Valve" },
	{ "arena_lumberyard",	  "Lumberyard",		  "#Gametype_Arena",			  "Valve" },
	{ "arena_nucleus",		  "Nucleus",		  "#Gametype_Arena",			  "Valve" },
	{ "arena_offblast_final", "Offblast",		  "#Gametype_Arena",			  "Insta" },
	{ "arena_ravine",		  "Ravine",			  "#Gametype_Arena",			  "Valve" },
	{ "arena_sawmill",		  "Sawmill",		  "#Gametype_Arena",			  "Valve" },
	{ "arena_watchtower",	  "Watchtower",		  "#Gametype_Arena",			  "JoshuaC" },
	{ "arena_well",			  "Well",			  "#Gametype_Arena",			  "Valve" },
};

s_MapTypeInfo s_MapTypes[MAX_MAP_TYPES] =
{
	{ "cp_",		3, "#Gametype_CP",				"#Gametype_CP_FourTeam" },
	{ "ctf_",		4, "#Gametype_CTF",				"#Gametype_CTF_FourTeam" },
	{ "pl_",		3, "#Gametype_Escort",			"#Gametype_Escort_FourTeam" },
	{ "plr_",		4, "#Gametype_EscortRace",		"#Gametype_EscortRace_FourTeam" },
	{ "koth_",		5, "#Gametype_Koth",			"#Gametype_Koth_FourTeam" },
	{ "arena_",		6, "#Gametype_Arena",			"#Gametype_Arena_FourTeam" },
	{ "sd_",		3, "#Gametype_SD",				"#Gametype_SD_FourTeam" },
	{ "tr_",		3, "#Gametype_Training",		"#Gametype_Training_FourTeam" },
	{ "tc_",		3, "#Gametype_TC",				"#Gametype_TC_FourTeam" },
	{ "vip_",		4, "#Gametype_VIP",				"#Gametype_VIP_FourTeam" },
	{ "vipr_",		5, "#Gametype_VIPRace",			"#Gametype_VIPRace_FourTeam" },
	{ "dom_",		4, "#Gametype_Domination",		"#Gametype_Domination_FourTeam" },
	{ "td_",		3, "#Gametype_TD",				"#Gametype_TD_FourTeam" },
	{ "kotf_",		5, "#Gametype_KOTF",			"#Gametype_KOTF_FourTeam" },
	{ "ps_",		3, "#Gametype_Powersiege",		"#Gametype_Powersiege_FourTeam" },
	{ "inf_",		4, "#Gametype_Infiltration",	"#Gametype_Infiltration_FourTeam" },
	// { "swdn_",		5, "#Gametype_Showdown" },
};


const char *GetMapDisplayName( const char *mapName )
{
	static char szDisplayName[256];
	char szTempName[256];
	const char *pszSrc = NULL;

	szDisplayName[0] = '\0';

	if ( !mapName )
		return szDisplayName;

	/*
	// check the worldspawn entity to see if the map author has specified a name
	if ( GetClientWorldEntity() )
	{
	const char *pszMapDescription = GetClientWorldEntity()->m_iszMapDescription;
	if ( Q_strlen( pszMapDescription ) > 0 )
	{
	V_strcpy_safe( szDisplayName, pszMapDescription );
	Q_strupr( szDisplayName );

	return szDisplayName;
	}
	}
	*/
	// check our lookup table
	V_strcpy_safe( szTempName, mapName );
	Q_strlower( szTempName );

	for ( int i = 0; i < ARRAYSIZE(s_Maps); ++i )
	{
		if ( !Q_stricmp( s_Maps[i].pDiskName, szTempName ) )
		{
			return s_Maps[i].pDisplayName;
		}
	}

	// we haven't found a "friendly" map name, so let's just clean up what we have
	pszSrc = szTempName;

	for ( int i = 0; i < ARRAYSIZE(s_MapTypes); ++i )
	{
		if ( !Q_strncmp( mapName, s_MapTypes[i].pDiskPrefix, s_MapTypes[i].iLength ) )
		{
			pszSrc = szTempName + s_MapTypes[i].iLength;
			break;
		}
	}

	V_strcpy_safe( szDisplayName, pszSrc );
	Q_strupr( szDisplayName );

	return szDisplayName;
}


const char *GetMapType( const char *mapName, bool bFourTeam /*= false*/ )
{
	if ( mapName )
	{
		// Have we got a registered map named that?
		for ( int i = 0; i < ARRAYSIZE(s_Maps); ++i )
		{
			if ( !Q_stricmp(s_Maps[i].pDiskName, mapName) )
			{
				// If so, return the registered gamemode
				return s_Maps[i].pGameType;
			}
		}
		// If not, see what the prefix is and try and guess from that
		for ( int i = 0; i < ARRAYSIZE(s_MapTypes); ++i )
		{
			if ( !Q_strncmp( mapName, s_MapTypes[i].pDiskPrefix, s_MapTypes[i].iLength ) )
				return bFourTeam ? s_MapTypes[i].pGameType_FourTeam : s_MapTypes[i].pGameType;
		}
	}

	return "";
}


const char *GetMapAuthor( const char *mapName )
{
	if ( mapName )
	{
		// Have we got a registered map named that?
		for ( int i = 0; i < ARRAYSIZE(s_Maps); ++i )
		{
			if ( !Q_stricmp(s_Maps[i].pDiskName, mapName) )
			{
				// If so, return the registered author
				return s_Maps[i].pAuthor;
			}
		}
	}

	return ""; // Otherwise, return NULL
}
