//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "input.h"
#include "tf_viewport.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "voice_status.h"
#include "tf_clientscoreboard.h"
#include "tf_spectatorgui.h"
#include "tf_textwindow.h"
#include "tf_mapinfomenu.h"
#include "tf_roundinfo.h"
#include "tf_teammenu.h"
#include "tf_classmenu.h"
#include "tf_intromenu.h"
#include "tf_fourteamscoreboard.h"
#include "tf_hud_notification_panel.h"
#include <vgui_controls/AnimationController.h>

/*
CON_COMMAND( spec_help, "Show spectator help screen")
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowPanel( PANEL_INFO, true );
}

CON_COMMAND( spec_menu, "Activates spectator menu")
{
	bool bShowIt = true;

	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();

	if ( pPlayer && !pPlayer->IsObserver() )
		return;

	if ( args.ArgC() == 2 )
	{
		bShowIt = atoi( args[ 1 ] ) == 1;
	}

	if ( gViewPortInterface )
		gViewPortInterface->ShowPanel( PANEL_SPECMENU, bShowIt );
}
*/

CON_COMMAND( showmapinfo, "Show map info panel" )
{
	if ( !gViewPortInterface )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// don't let the player open the team menu themselves until they're a spectator or they're on a regular team and have picked a class
	if ( pPlayer )
	{
		if ( ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR ) || 
		     ( ( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) && ( pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) ) )
		{
			// close all the other panels that could be open
			gViewPortInterface->ShowPanel( PANEL_TEAM, false );
			gViewPortInterface->ShowPanel( PANEL_CLASS, false );
			gViewPortInterface->ShowPanel( PANEL_INTRO, false );
			gViewPortInterface->ShowPanel( PANEL_ROUNDINFO, false );
			gViewPortInterface->ShowPanel( PANEL_FOURTEAMSELECT, false );
			gViewPortInterface->ShowPanel( PANEL_MAPINFO, true );
		}
	}
}


TFViewport::TFViewport()
{
	ivgui()->AddTickSignal( GetVPanel(), 0 );
}

//-----------------------------------------------------------------------------
// Purpose: called when the VGUI subsystem starts up
//			Creates the sub panels and initialises them
//-----------------------------------------------------------------------------
void TFViewport::Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 * pGameEventManager )
{
	BaseClass::Start( pGameUIFuncs, pGameEventManager );
}


void TFViewport::OnTick( void )
{
	m_pAnimController->UpdateAnimations( gpGlobals->curtime );
}


void TFViewport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	gHUD.InitColors( pScheme );

	SetPaintBackgroundEnabled( false );
}


void TFViewport::LoadControlSettings( const char *resourceName, const char *pathID, KeyValues *pKeyValues, KeyValues *pConditions )
{
	BaseClass::LoadControlSettings( resourceName, pathID, pKeyValues, pConditions );

	// HACK: Load up our custom file as well. This needs to be removed later.
	BaseClass::LoadControlSettings( "scripts/HudLayout_tf2c.res", pathID, pKeyValues, pConditions );
}

//-----------------------------------------------------------------------------
// Purpose: This is the main function of the viewport. Right here is where we create our class menu, 
// team menu, and anything else that we want to turn on and off in the UI.
//-----------------------------------------------------------------------------
IViewPortPanel* TFViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	// overwrite MOD specific panel creation

	if ( FStrEq( PANEL_SCOREBOARD, szPanelName ) )
	{
		newpanel = new CTFClientScoreBoardDialog( this, szPanelName );
	}
	else if ( FStrEq( PANEL_SPECGUI, szPanelName ) )
	{
		newpanel = new CTFSpectatorGUI( this );	
	}
	else if ( FStrEq( PANEL_SPECMENU, szPanelName ) )
	{
//		newpanel = new CTFSpectatorGUI( this );	
	}
	else if ( FStrEq( PANEL_OVERVIEW, szPanelName ) )
	{
//		newpanel = new CTFMapOverview( this );
	}
	else if ( FStrEq( PANEL_INFO, szPanelName ) )
	{
		newpanel = new CTFTextWindow( this );
	}
	else if ( FStrEq( PANEL_MAPINFO, szPanelName ) )
	{
		newpanel = new CTFMapInfoMenu( this );
	}
	else if ( FStrEq( PANEL_ROUNDINFO, szPanelName ) )
	{
		newpanel = new CTFRoundInfo( this );
	}
	else if ( FStrEq( PANEL_TEAM, szPanelName ) )
	{
		CTFTeamMenu *pTeamMenu = new CTFTeamMenu( this, szPanelName );	
		pTeamMenu->CreateTeamButtons();
		newpanel = pTeamMenu;
	}
	else if ( FStrEq( PANEL_ARENA_TEAM, szPanelName ) )
	{
		CTFTeamMenu *pTeamMenu = new CTFArenaTeamMenu( this, szPanelName );
		pTeamMenu->CreateTeamButtons();
		newpanel = pTeamMenu;
	}
	else if ( FStrEq( PANEL_FOURTEAMSELECT, szPanelName ) )
	{
		CTFTeamMenu *pTeamMenu = new CTFFourTeamMenu( this, szPanelName );
		pTeamMenu->CreateTeamButtons();
		newpanel = pTeamMenu;
	}
	else if ( FStrEq( PANEL_CLASS, szPanelName ) )
	{
		newpanel = new CTFClassMenu( this );	
	}
	else if ( FStrEq( PANEL_INTRO, szPanelName ) )
	{
		newpanel = new CTFIntroMenu( this );
	}
	else if ( FStrEq( PANEL_FOURTEAMSCOREBOARD, szPanelName ) )
	{
		newpanel = new CTFFourTeamScoreBoardDialog( this, szPanelName );
	}	
	else
	{
		// create a generic base panel, don't add twice
		newpanel = BaseClass::CreatePanelByName( szPanelName );
	}

	return newpanel; 
}


void TFViewport::CreateDefaultPanels( void )
{
	AddNewPanel( CreatePanelByName( PANEL_MAPINFO ), "PANEL_MAPINFO" );
	AddNewPanel( CreatePanelByName( PANEL_TEAM ), "PANEL_TEAM" );
	AddNewPanel( CreatePanelByName( PANEL_ARENA_TEAM ), "PANEL_ARENA_TEAM" );
	AddNewPanel( CreatePanelByName( PANEL_CLASS ), "PANEL_CLASS" );
	AddNewPanel( CreatePanelByName( PANEL_INTRO ), "PANEL_INTRO" );
	AddNewPanel( CreatePanelByName( PANEL_ROUNDINFO ), "PANEL_ROUNDINFO" );
	AddNewPanel( CreatePanelByName( PANEL_FOURTEAMSELECT ), "PANEL_FOURTEAMSELECT" );
	AddNewPanel( CreatePanelByName( PANEL_FOURTEAMSCOREBOARD ), "PANEL_FOURTEAMSCOREBOARD" );

	BaseClass::CreateDefaultPanels();
}


int TFViewport::GetDeathMessageStartHeight( void )
{
	int y = YRES( 2 );


	if ( g_pSpectatorGUI && g_pSpectatorGUI->IsVisible() )
	{
		y = YRES( 2 ) + g_pSpectatorGUI->GetTopBarHeight();
	}
	else if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		if ( TFGameRules()->IsInKothMode() || TFGameRules()->IsInDominationMode() )
		{
			// 4-team HUD in KOTH and Domination is quite wide so push the killfeed down.
			y = YRES( 30 );
		}
	}

	return y;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for handling multiple scoreboard.
//-----------------------------------------------------------------------------
const char *TFViewport::GetModeSpecificScoreboardName( void )
{
	// Override scoreboard and team select screen based on game mode.
	CTFGameRules *pTFGameRules = TFGameRules();
	if ( pTFGameRules && pTFGameRules->IsFourTeamGame() )
		return PANEL_FOURTEAMSCOREBOARD;

	return PANEL_SCOREBOARD;
}

//-----------------------------------------------------------------------------
// Purpose: Use this when you need to open the scoreboard unless you want a specific scoreboard panel.
//-----------------------------------------------------------------------------
void TFViewport::ShowScoreboard( bool bState, int nPollHideCode /*= BUTTON_CODE_INVALID*/ )
{
	const char *pszScoreboard = GetModeSpecificScoreboardName();
	ShowPanel( pszScoreboard, bState );

	if ( bState && nPollHideCode != BUTTON_CODE_INVALID )
	{
		// The scoreboard was opened by another dialog so we need to send the close button code.
		// See CTFClientScoreBoardDialog::OnThink for the explanation.
		PostMessageToPanel( pszScoreboard, new KeyValues( "PollHideCode", "code", nPollHideCode ) );
	}
}


void TFViewport::CC_ScoresDown( const CCommand &args )
{
	ShowScoreboard( true );
}


void TFViewport::CC_ScoresUp( const CCommand &args )
{
	ShowScoreboard( false );
	GetClientVoiceMgr()->StopSquelchMode();
}


void TFViewport::CC_ToggleScores( const CCommand &args )
{
	IViewPortPanel *pPanel = FindPanelByName( GetModeSpecificScoreboardName() );
	if ( pPanel )
	{
		if ( pPanel->IsVisible() )
		{
			ShowPanel( pPanel, false );
			GetClientVoiceMgr()->StopSquelchMode();
		}
		else
		{
			ShowPanel( pPanel, true );
		}
	}
}


void TFViewport::ShowTeamMenu( bool bState )
{
	if ( !TFGameRules() )
		return;

	if ( TFGameRules()->IsInArenaQueueMode() )
	{
		ShowPanel( PANEL_ARENA_TEAM, bState );
	}
	else if ( TFGameRules()->IsFourTeamGame() )
	{
		ShowPanel( PANEL_FOURTEAMSELECT, bState );
	}
	else
	{
		ShowPanel( PANEL_TEAM, bState );
	}
}


void TFViewport::CC_ChangeTeam( const CCommand &args )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( engine->IsHLTV() )
		return;

	// don't let the player open the team menu themselves until they're on a team
	if ( pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		return;

	// Losers can't change team during bonus time.
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
		pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM &&
		pPlayer->GetTeamNumber() != TFGameRules()->GetWinningTeam() )
		return;

	ShowTeamMenu( true );
}


void TFViewport::ShowClassMenu( bool bState )
{
	if ( bState )
	{
		// Need to set the class menu to the proper team when showing it.
		PostMessageToPanel( PANEL_CLASS, new KeyValues( "ShowToTeam", "iTeam", GetLocalPlayerTeam() ) );
	}
	else
	{
		ShowPanel( PANEL_CLASS, false );
	}
}


void TFViewport::CC_ChangeClass( const CCommand &args )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( engine->IsHLTV() )
		return;

	if ( TFGameRules()->IsInArenaMode() && TFGameRules()->State_Get() == GR_STATE_STALEMATE && pPlayer->IsAlive() )
	{
		CHudNotificationPanel::SetupNotificationPanel( "#TF_Arena_NoClassChange", "ico_notify_flag_moving", GetLocalPlayerTeam() );
		return;
	}

	if ( pPlayer->CanShowClassMenu() )
	{
		ShowClassMenu( true );
	}
}


void TFViewport::OnScreenSizeChanged( int iOldWide, int iOldTall )
{
	BaseClass::OnScreenSizeChanged( iOldWide, iOldTall );

	// We've changed resolution, let's try to figure out if we need to show any of our menus
	if ( !gViewPortInterface )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		// are we on a team yet?
		if ( pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		{
			engine->ClientCmd( "show_motd" );
		}
		else if ( ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR ) && ( pPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED ) )
		{
			ShowClassMenu( true );
		}
	}
}
