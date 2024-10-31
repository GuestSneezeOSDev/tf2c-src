//=============================================================================
//
// Purpose: Payload HUD
//
//=============================================================================
#include "cbase.h"
#include "tf_hud_escort.h"
#include "tf_hud_freezepanel.h"
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar cl_hud_minmode;
extern ConVar mp_blockstyle;
extern ConVar mp_capstyle;

//=============================================================================
// CTFHudEscort
//=============================================================================
CTFHudEscort::CTFHudEscort( Panel *pParent, const char *pszName, int iTeam, bool bMultipleTrains ) : EditablePanel( pParent, pszName )
{
	m_iTeamNum = iTeam;

	m_pLevelBar = new ImagePanel( this, "LevelBar" );

	m_pEscortItemPanel = new EditablePanel( this, "EscortItemPanel" );
	m_pEscortItemImage = new ImagePanel( m_pEscortItemPanel, "EscortItemImage" );
	m_pEscortItemImageBottom = new ImagePanel( m_pEscortItemPanel, "EscortItemImageBottom" );
	m_pEscortItemImageAlert = new ImagePanel( m_pEscortItemPanel, "EscortItemImageAlert" );
	m_pCapNumPlayers = new CExLabel( m_pEscortItemPanel, "CapNumPlayers", "x0" );
	m_pRecedeTime = new CExLabel( m_pEscortItemPanel, "RecedeTime", "0" );
	m_pCapPlayerImage = new ImagePanel( m_pEscortItemPanel, "CapPlayerImage" );
	m_pBackwardsImage = new ImagePanel( m_pEscortItemPanel, "Speed_Backwards" );
	m_pBlockedImage = new ImagePanel( m_pEscortItemPanel, "Blocked" );
	m_pTearDrop = new CEscortStatusTeardrop( m_pEscortItemPanel, "EscortTeardrop" );

	m_pCapHighlightImage = new CControlPointIconSwoop( m_pEscortItemPanel, "CapHighlightImage" );
	m_pCapHighlightImage->SetZPos( 10 );
	m_pCapHighlightImage->SetShouldScaleImage( true );

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		m_pCPImages[i] = new ImagePanel( this, VarArgs( "cp_%d", i ) );
	}

	m_pCPImageTemplate = new ImagePanel( this, "SimpleControlPointTemplate" );

	for ( int i = 0; i < TF_TRAIN_MAX_HILLS; i++ )
	{
		m_pHillPanels[i] = new CEscortHillPanel( this, VarArgs( "hill_%d", i ) );
	}

	m_pProgressBar = new CTFHudEscortProgressBar( this, "ProgressBar", m_iTeamNum );

	m_flProgress = -1.0f;
	m_iSpeedLevel = 0;
	m_flRecedeTime = 0.0f;
	m_iCurrentCP = -1;

	m_bMultipleTrains = bMultipleTrains;
	m_bOnTop = true;
	m_bAlarm = false;
	m_iNumHills = 0;
	m_bCPReady = true;
#if STAGING_FOURTEAM_FLIPPED
	m_bFlipped = false;
#endif

	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "escort_progress" );
	ListenForGameEvent( "escort_speed" );
	ListenForGameEvent( "escort_recede" );
	ListenForGameEvent( "controlpoint_initialized" );
	ListenForGameEvent( "controlpoint_updateimages" );
	ListenForGameEvent( "controlpoint_updatecapping" );
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
}


void CTFHudEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Get hill data.
	if ( ObjectiveResource() )
	{
		m_iNumHills = ObjectiveResource()->GetNumNodeHillData( m_iTeamNum );
	}

	// Setup conditions.
	KeyValues *pConditions = NULL;
	if ( m_iTeamNum >= FIRST_GAME_TEAM )
	{
		pConditions = new KeyValues( "conditions" );

		if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
		{
			AddSubKeyNamed( pConditions, "if_fourteams" );
		}

		switch ( m_iTeamNum )
		{
			case TF_TEAM_RED:
				AddSubKeyNamed( pConditions, "if_team_red" );
				break;
			default:
			case TF_TEAM_BLUE:
				AddSubKeyNamed( pConditions, "if_team_blue" );
				break;
			case TF_TEAM_GREEN:
				AddSubKeyNamed( pConditions, "if_team_green" );
				break;
			case TF_TEAM_YELLOW:
				AddSubKeyNamed( pConditions, "if_team_yellow" );
				break;
		}

		if ( m_bMultipleTrains )
		{
			AddSubKeyNamed( pConditions, "if_multiple_trains" );

			switch ( m_iTeamNum )
			{
				case TF_TEAM_RED:
					AddSubKeyNamed( pConditions, "if_multiple_trains_red" );
					break;
				default:
				case TF_TEAM_BLUE:
					AddSubKeyNamed( pConditions, "if_multiple_trains_blue" );
					break;
				case TF_TEAM_GREEN:
					AddSubKeyNamed( pConditions, "if_multiple_trains_green" );
					break;
				case TF_TEAM_YELLOW:
					AddSubKeyNamed( pConditions, "if_multiple_trains_yellow" );
					break;
			}

			AddSubKeyNamed( pConditions, m_bOnTop ? "if_multiple_trains_top" : "if_multiple_trains_bottom" );
		}
		else if ( m_iNumHills > 0 )
		{
			AddSubKeyNamed( pConditions, "if_single_with_hills" );

			switch ( m_iTeamNum )
			{
				case TF_TEAM_RED:
					AddSubKeyNamed( pConditions, "if_single_with_hills_red" );
					break;
				default:
				case TF_TEAM_BLUE:
					AddSubKeyNamed( pConditions, "if_single_with_hills_blue" );
					break;
				case TF_TEAM_GREEN:
					AddSubKeyNamed( pConditions, "if_single_with_hills_green" );
					break;
				case TF_TEAM_YELLOW:
					AddSubKeyNamed( pConditions, "if_single_with_hills_yellow" );
					break;
			}
		}
	}

	LoadControlSettings( "resource/ui/ObjectiveStatusEscort.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}


void CTFHudEscort::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	// Apply settings from template to all CP icons.
	if ( pChild == m_pCPImageTemplate )
	{
		for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
		{
			m_pCPImages[i]->ApplySettings( pInResourceData );
			m_pCPImages[i]->SetName( VarArgs("cp_%d", i) );
		}
	}

	BaseClass::OnChildSettingsApplied( pInResourceData, pChild );
}


void CTFHudEscort::Reset( void )
{
	m_iCurrentCP = -1;
}


void CTFHudEscort::PerformLayout( void )
{
	// If the tracker's at the bottom show the correct cart image.
	m_pEscortItemImage->SetVisible( m_bOnTop );
	m_pEscortItemImageBottom->SetVisible( !m_bOnTop );

	// Place the swooping thingamajig on top of the cart icon.
	int x, y, wide, tall;
	m_pEscortItemImage->GetBounds( x, y, wide, tall );

	int iSwoopHeight = ( m_bMultipleTrains || cl_hud_minmode.GetBool() ) ? YRES( 72 ) : YRES( 96 );

	m_pCapHighlightImage->SetBounds( x + CAP_BOX_INDENT_X, y - iSwoopHeight + CAP_BOX_INDENT_Y, wide - ( CAP_BOX_INDENT_X * 2 ), iSwoopHeight );

	UpdateHillPanels();
	UpdateCPImages( true, -1 );
}


bool CTFHudEscort::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	return BaseClass::IsVisible();
}


void CTFHudEscort::FireGameEvent( IGameEvent *event )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( V_strcmp( event->GetName(), "controlpoint_initialized" ) == 0 )
	{
		InvalidateLayout( true, true );
		m_iCurrentCP = -1;
		m_pTearDrop->SetupForPoint( -1 );
	}
	else if ( V_strcmp( event->GetName(), "controlpoint_updateimages" ) == 0 )
	{
		int iIndex = event->GetInt( "index" );
		UpdateCPImages( false, iIndex );
		UpdateStatusTeardropFor( iIndex );
	}
	else if ( V_strcmp( event->GetName(), "controlpoint_updatecapping" ) == 0 )
	{
		UpdateStatusTeardropFor( event->GetInt( "index" ) );
	}
	else if ( V_strcmp( event->GetName(), "controlpoint_starttouch" ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pLocalPlayer && iPlayer == pLocalPlayer->entindex() )
		{
			m_iCurrentCP = event->GetInt( "area" );
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
	}
	else if ( V_strcmp( event->GetName(), "controlpoint_endtouch" ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		if ( pLocalPlayer && iPlayer == pLocalPlayer->entindex() )
		{
			m_iCurrentCP = -1;
			UpdateStatusTeardropFor( m_iCurrentCP );
		}
	}

	// Ignore events not related to the watched team.
	if ( event->GetInt( "team" ) != m_iTeamNum )
		return;

	if ( V_strcmp( event->GetName(), "escort_progress" ) == 0 )
	{
		if ( event->GetBool( "reset" ) )
		{
			m_flProgress = 0.0f;
		}
		else
		{
			m_flProgress = event->GetFloat( "progress" );
		}
	}
	else if ( V_strcmp( event->GetName(), "escort_speed" ) == 0 )
	{
		// Get the number of cappers.
		int iNumCappers = event->GetInt( "players" );
		int iSpeedLevel = event->GetInt( "speed" );

		m_pEscortItemPanel->SetDialogVariable( "numcappers", iNumCappers );

		// Show the number and icon if there any cappers present.
		bool bShowCappers = ( iNumCappers > 0 );
		m_pCapNumPlayers->SetVisible( bShowCappers );
		m_pCapPlayerImage->SetVisible( bShowCappers );

		// -1 cappers means the cart is blocked.
		m_pBlockedImage->SetVisible( iNumCappers == -1 );

		// -1 speed level means the cart is receding.
		m_pBackwardsImage->SetVisible( iSpeedLevel == -1 );

		if ( m_iSpeedLevel <= 0 && iSpeedLevel > 0 )
		{
			// Do the swooping animation when the cart starts moving but only for the top icon.
			if ( m_bOnTop )
			{		
				m_pCapHighlightImage->SetVisible( true );
				m_pCapHighlightImage->StartSwoop();
			}	
		}

		m_iSpeedLevel = iSpeedLevel;
	}
	else if ( V_strcmp( event->GetName(), "escort_recede" ) == 0 )
	{
		// Get the current recede time of the cart.
		m_flRecedeTime = event->GetFloat( "recedetime" );
	}
}


void CTFHudEscort::OnTick( void )
{
	if ( !IsVisible() )
		return;

	// Position the cart icon so the arrow points at its position on the track.
	int x, y, wide, tall, pos;
	m_pLevelBar->GetBounds( x, y, wide, tall );

	pos = (int)( wide * m_flProgress ) - m_pEscortItemPanel->GetWide() / 2;

#if STAGING_FOURTEAM_FLIPPED
	m_pEscortItemPanel->SetPos( m_bFlipped ? wide - pos : x + pos, m_pEscortItemPanel->GetYPos() );
#else
	m_pEscortItemPanel->SetPos( x + pos, m_pEscortItemPanel->GetYPos() );
#endif

	// Only show progress bar in Payload Race.
	if ( m_bMultipleTrains )
	{
		// Update the progress bar.
		m_pProgressBar->SetVisible( true );
		m_pProgressBar->SetProgress( m_flProgress );
#if STAGING_FOURTEAM_FLIPPED
		m_pProgressBar->SetFlipped( m_bFlipped );
#endif
	}
	else
	{
		m_pProgressBar->SetVisible( false );
	}

	// Calculate time left until receding.
	float flRecedeTimeLeft = ( m_flRecedeTime != 0.0f ) ? m_flRecedeTime - gpGlobals->curtime : 0.0f;
	m_pEscortItemPanel->SetDialogVariable( "recede", (int)ceil( flRecedeTimeLeft ) );

	// Show the timer if the cart is close to starting to recede.
	m_pRecedeTime->SetVisible( flRecedeTimeLeft > 0.0f && flRecedeTimeLeft < 20.0f );

	// Check for alarm animation.
	bool bInAlarm = false;
	if ( ObjectiveResource() )
	{
		bInAlarm = ObjectiveResource()->GetTrackAlarm( m_iTeamNum );
	}

	if ( bInAlarm != m_bAlarm )
	{
		m_bAlarm = bInAlarm;
		UpdateAlarmAnimations();
	}

	// Apparently, this is Valve's fix for delayed activation of team_train_watcher.
	if ( !m_bCPReady )
	{
		UpdateHillPanels();
		UpdateCPImages( true, -1 );
	}
}


void CTFHudEscort::UpdateCPImages( bool bUpdatePositions, int iIndex )
{
	if ( !ObjectiveResource() )
		return;
	
	if ( bUpdatePositions )
	{
		m_bCPReady = false;
	}

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		// If an index is specified only update the specified point.
		if ( iIndex != -1 && i != iIndex )
			continue;

		ImagePanel *pImage = m_pCPImages[i];
		if ( bUpdatePositions )
		{
			// Check if this point exists and should be shown.
			if ( i >= ObjectiveResource()->GetNumControlPoints() ||
				!ObjectiveResource()->IsInMiniRound( i ) ||
				!ObjectiveResource()->IsCPVisible( i ) )
			{
				pImage->SetVisible( false );
				continue;
			}

			pImage->SetVisible( true );

			// Get the control point position.
			float flDist = ObjectiveResource()->GetPathDistance( i );

			int x, y, wide, tall, pos;
			m_pLevelBar->GetBounds( x, y, wide, tall );

			pos = (int)( wide * flDist ) - pImage->GetWide() / 2;

#if STAGING_FOURTEAM_FLIPPED
			pImage->SetPos( m_bFlipped ? wide - pos : x + pos, pImage->GetYPos() );
#else
			pImage->SetPos( x + pos, pImage->GetYPos() );
#endif

			if ( flDist > 0.0f )
			{
				m_bCPReady = true;
			}
		}

		// Set the icon according to team.
		const char *pszImage = NULL;
		bool bOpaque = m_bMultipleTrains || m_iNumHills > 0;
		switch ( ObjectiveResource()->GetOwningTeam( i ) )
		{
			case TF_TEAM_RED:
				pszImage = bOpaque ? "../hud/cart_point_red_opaque" : "../hud/cart_point_red";
				break;
			case TF_TEAM_BLUE:
				pszImage = bOpaque ? "../hud/cart_point_blue_opaque" : "../hud/cart_point_blue";
				break;
			case TF_TEAM_GREEN:
				pszImage = bOpaque ? "../hud/cart_point_green_opaque" : "../hud/cart_point_green";
				break;
			case TF_TEAM_YELLOW:
				pszImage = bOpaque ? "../hud/cart_point_yellow_opaque" : "../hud/cart_point_yellow";
				break;
			default:
				pszImage = bOpaque ? "../hud/cart_point_neutral_opaque" : "../hud/cart_point_neutral";
				break;
		}

		pImage->SetImage( pszImage );
	}
}


void CTFHudEscort::UpdateHillPanels( void )
{
	for ( int i = 0; i < TF_TRAIN_MAX_HILLS; i++ )
	{
		CEscortHillPanel *pPanel = m_pHillPanels[i];
		if ( !ObjectiveResource() || i >= ObjectiveResource()->GetNumNodeHillData( m_iTeamNum ) || !m_pLevelBar )
		{
			pPanel->SetVisible( false );
			continue;
		}

		pPanel->SetVisible( true );
		pPanel->SetTeam( m_iTeamNum );
		pPanel->SetHillIndex( i );
#if STAGING_FOURTEAM_FLIPPED
		pPanel->SetFlipped( m_bFlipped );
#endif

		// Set the panel's bounds according to starting and ending points of the hill.
		int x, y, wide, tall;
		m_pLevelBar->GetBounds( x, y, wide, tall );

		float flStart = 0.0f;
		float flEnd = 0.0f;
		ObjectiveResource()->GetHillData( m_iTeamNum, i, flStart, flEnd );

		int iStartPos = flStart * wide;
		int iEndPos = flEnd * wide;
#if STAGING_FOURTEAM_FLIPPED
		pPanel->SetBounds( m_bFlipped ? wide - iStartPos : x + iStartPos, y, iEndPos - iStartPos, tall );
#else
		pPanel->SetBounds( x + iStartPos, y, iEndPos - iStartPos, tall );
#endif

		// Show it on top of the track bar.
		pPanel->SetZPos( m_pLevelBar->GetZPos() + 1 );
	}
}


void CTFHudEscort::UpdateStatusTeardropFor( int iIndex )
{
	// If they tell us to update all points, update only the one we're standing on.
	if ( iIndex == -1 )
	{
		iIndex = m_iCurrentCP;
	}

	// Ignore requests to display teardrop for points we're not standing on.
	if ( m_iCurrentCP != iIndex )
		return;

	if ( iIndex >= ObjectiveResource()->GetNumControlPoints() )
	{
		m_pTearDrop->SetupForPoint( -1 );
		return;
	}

	// Don't show teardrop for enemy carts in Payload Race.
	if ( m_bMultipleTrains )
	{
		int iLocalTeam = GetLocalPlayerTeam();
		if ( iLocalTeam != m_iTeamNum || !ObjectiveResource()->TeamCanCapPoint( iIndex, iLocalTeam ) )
		{
			m_pTearDrop->SetupForPoint( -1 );
			return;
		}
	}

	m_pTearDrop->SetupForPoint( iIndex );
}


void CTFHudEscort::UpdateAlarmAnimations( void )
{
	// Only do alert animations in Payload Race.
	if ( !m_bMultipleTrains )
		return;

	if ( m_bAlarm )
	{
		m_pEscortItemImageAlert->SetVisible( true );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "HudCartAlarmPulse" );
	}
	else
	{
		m_pEscortItemImageAlert->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pEscortItemPanel, "HudCartAlarmPulseStop" );
	}
}


//=============================================================================
// CTFHudMultipleEscort
//=============================================================================
CTFHudMultipleEscort::CTFHudMultipleEscort( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	m_pRedEscort = new CTFHudEscort( this, "RedEscortPanel", TF_TEAM_RED, true );

	m_pBlueEscort = new CTFHudEscort( this, "BlueEscortPanel", TF_TEAM_BLUE, true );
	m_pRedEscort->SetOnTop( false );

	m_pGreenEscort = new CTFHudEscort( this, "GreenEscortPanel", TF_TEAM_GREEN, true );
#if STAGING_FOURTEAM_FLIPPED
	m_pGreenEscort->SetFlipped( true );
#endif

	m_pYellowEscort = new CTFHudEscort( this, "YellowEscortPanel", TF_TEAM_YELLOW, true );
	m_pYellowEscort->SetOnTop( false );
#if STAGING_FOURTEAM_FLIPPED
	m_pYellowEscort->SetFlipped( true );
#endif

	ListenForGameEvent( "localplayer_changeteam" );
}


void CTFHudMultipleEscort::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Setup conditions.
	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		AddSubKeyNamed( pConditions, "if_fourteams" );
	}

	switch ( GetLocalPlayerTeam() )
	{
		case TF_TEAM_RED:
			AddSubKeyNamed( pConditions, "if_red_is_top" );
			AddSubKeyNamed( pConditions, "if_yellow_is_top" );
			break;
		default:
		case TF_TEAM_BLUE:
			AddSubKeyNamed( pConditions, "if_blue_is_top" );
			AddSubKeyNamed( pConditions, "if_green_is_top" );
			break;
		case TF_TEAM_GREEN:
			AddSubKeyNamed( pConditions, "if_green_is_top" );
			AddSubKeyNamed( pConditions, "if_blue_is_top" );
			break;
		case TF_TEAM_YELLOW:
			AddSubKeyNamed( pConditions, "if_yellow_is_top" );
			AddSubKeyNamed( pConditions, "if_red_is_top" );
			break;
	}

	LoadControlSettings( "resource/ui/ObjectiveStatusMultipleEscort.res", NULL, NULL, pConditions );

	pConditions->deleteThis();
}


void CTFHudMultipleEscort::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		// Show the cart of the local player's team on top.
		switch ( GetLocalPlayerTeam() )
		{
			case TF_TEAM_RED:
				m_pRedEscort->SetOnTop( true );
				m_pBlueEscort->SetOnTop( false );
				m_pGreenEscort->SetOnTop( false );
				m_pYellowEscort->SetOnTop( true );
				break;
			default:
			case TF_TEAM_BLUE:
				m_pRedEscort->SetOnTop( false );
				m_pBlueEscort->SetOnTop( true );
				m_pGreenEscort->SetOnTop( true );
				m_pYellowEscort->SetOnTop( false );
				break;
			case TF_TEAM_GREEN:
				m_pRedEscort->SetOnTop( false );
				m_pBlueEscort->SetOnTop( true );
				m_pGreenEscort->SetOnTop( true );
				m_pYellowEscort->SetOnTop( false );
				break;
			case TF_TEAM_YELLOW:
				m_pRedEscort->SetOnTop( true );
				m_pBlueEscort->SetOnTop( false );
				m_pGreenEscort->SetOnTop( false );
				m_pYellowEscort->SetOnTop( true );
				break;
		}

		// Re-arrange panels when player changes teams.
		InvalidateLayout( false, true );
	}
}


void CTFHudMultipleEscort::SetVisible( bool bVisible )
{
	// Hide sub-panels as well.
	m_pRedEscort->SetVisible( bVisible );
	m_pBlueEscort->SetVisible( bVisible );

	bool bFourTeams = TFGameRules() && TFGameRules()->IsFourTeamGame();
	m_pGreenEscort->SetVisible( bFourTeams && bVisible );
	m_pYellowEscort->SetVisible( bFourTeams && bVisible );

	BaseClass::SetVisible( bVisible );
}


bool CTFHudMultipleEscort::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	return BaseClass::IsVisible();
}


void CTFHudMultipleEscort::Reset( void )
{
	m_pRedEscort->Reset();
	m_pBlueEscort->Reset();
	m_pGreenEscort->Reset();
	m_pYellowEscort->Reset();
}


//=============================================================================
// CEscortStatusTeardrop
//=============================================================================
CEscortStatusTeardrop::CEscortStatusTeardrop( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	m_pProgressText = new Label( this, "ProgressText", "" );
	m_pTearDrop = new CIconPanel( this, "Teardrop" );
	m_pBlocked = new CIconPanel( this, "Blocked" );
	m_pCapping = new ImagePanel( this, "Capping" );

	m_iOrgHeight = 0;
	m_iMidGroupIndex = -1;
}


void CEscortStatusTeardrop::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_iOrgHeight = GetTall();
	m_iMidGroupIndex = gHUD.LookupRenderGroupIndexByName( "mid" );
}


bool CEscortStatusTeardrop::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	if ( m_iMidGroupIndex != -1 && gHUD.IsRenderGroupLockedFor( NULL, m_iMidGroupIndex ) )
		return false;

	return BaseClass::IsVisible();
}


void CEscortStatusTeardrop::SetupForPoint( int iCP )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	bool bInWinState = TFGameRules() ? TFGameRules()->RoundHasBeenWon() : false;
	if ( iCP != -1 && !bInWinState )
	{
		SetVisible( true );

		int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
		int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );
		int iPlayerTeam = pPlayer->GetTeamNumber();
		bool bCapBlocked = ObjectiveResource()->CapIsBlocked( iCP );
		if ( !bCapBlocked && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iOwnerTeam && iCappingTeam == iPlayerTeam )
		{
			m_pBlocked->SetVisible( false );
			m_pProgressText->SetVisible( false );
			m_pCapping->SetVisible( true );
		}
		else
		{
			m_pBlocked->SetVisible( true );
			m_pCapping->SetVisible( false );

			UpdateBarText( iCP );
		}
	}
	else
	{
		SetVisible( false );
	}
}


void CEscortStatusTeardrop::UpdateBarText( int iCP )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || iCP == -1 )
		return;

	m_pProgressText->SetVisible( true );

	int iCappingTeam = ObjectiveResource()->GetCappingTeam( iCP );
	int iPlayerTeam = pPlayer->GetTeamNumber();
	int iOwnerTeam = ObjectiveResource()->GetOwningTeam( iCP );

	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		m_pProgressText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( ObjectiveResource()->GetCPLocked( iCP ) )
	{
		m_pProgressText->SetText( "#Team_Capture_NotNow" );
		return;
	}

	if ( mp_blockstyle.GetInt() == 1 && iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iPlayerTeam )
	{
		if ( ObjectiveResource()->IsCPBlocked( iCP ) )
		{
			m_pProgressText->SetText( "#Team_Blocking_Capture" );
			return;
		}
		else if ( iOwnerTeam != iPlayerTeam )
		{
			m_pProgressText->SetText( "#Team_Reverting_Capture" );
			return;
		}
	}

	if ( ObjectiveResource()->GetOwningTeam( iCP ) == iPlayerTeam )
	{
		// If the opponents can never recapture this point back, we use a different string.
		if ( iPlayerTeam != TEAM_UNASSIGNED )
		{
			int iEnemyTeam = ( iPlayerTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
			if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iEnemyTeam ) )
			{
				m_pProgressText->SetText( "#Team_Capture_Owned" );
				return;
			}
		}

		m_pProgressText->SetText( "#Team_Capture_OwnPoint" );
		return;
	}

	if ( !TeamplayGameRules()->TeamMayCapturePoint( iPlayerTeam, iCP ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInArenaMode() )
		{
			m_pProgressText->SetText( "#Team_Capture_NotNow" );
		}
		else
		{
			m_pProgressText->SetText( "#Team_Capture_Linear" );
		}

		return;
	}

	char szReason[256];
	if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, iCP, szReason, sizeof( szReason ) ) )
	{
		m_pProgressText->SetText( szReason );
		return;
	}

	bool bHaveRequiredPlayers = true;

	// In Capstyle 1, more players simply cap faster, no required amounts.
	if ( mp_capstyle.GetInt() != 1 )
	{
		int nNumTeammates = ObjectiveResource()->GetNumPlayersInArea( iCP, iPlayerTeam );
		int nRequiredTeammates = ObjectiveResource()->GetRequiredCappers( iCP, iPlayerTeam );
		bHaveRequiredPlayers = ( nNumTeammates >= nRequiredTeammates );
	}

	if ( iCappingTeam == iPlayerTeam && bHaveRequiredPlayers )
	{
		m_pProgressText->SetText( "#Team_Capture_Blocked" );
		return;
	}

	if ( !ObjectiveResource()->TeamCanCapPoint( iCP, iPlayerTeam ) )
	{
		m_pProgressText->SetText( "#Team_Cannot_Capture" );
		return;
	}

	m_pProgressText->SetText( "#Team_Waiting_for_teammate" );
}

//=============================================================================
// CEscortHillPanel
//=============================================================================
CEscortHillPanel::CEscortHillPanel( Panel *pParent, const char *pszName ) : Panel( pParent, pszName )
{
	// Load the texture.
	m_iTextureId = surface()->DrawGetTextureId( "hud/cart_track_arrow" );
	if ( m_iTextureId == -1 )
	{
		m_iTextureId = surface()->CreateNewTextureID( false );
		surface()->DrawSetTextureFile( m_iTextureId, "hud/cart_track_arrow", true, false );
	}

	m_bActive = false;
	m_bLowerAlpha = true;
	m_iWidth = 0;
	m_iHeight = 0;
	m_flScrollPerc = 0.0f;
	m_flTextureScale = 0.0f;

	m_iTeamNum = TEAM_UNASSIGNED;
	m_iHillIndex = 0;
	m_bDownhill = false;
#if STAGING_FOURTEAM_FLIPPED
	m_bFlipped = false;
#endif

	ivgui()->AddTickSignal( GetVPanel(), 750 );

	ListenForGameEvent( "teamplay_round_start" );
}


void CEscortHillPanel::Paint( void )
{
	if ( ObjectiveResource() )
	{
		m_bActive = ObjectiveResource()->IsTrainOnHill( m_iTeamNum, m_iHillIndex );
		m_bDownhill = ObjectiveResource()->IsHillDownhill( m_iTeamNum, m_iHillIndex );
	}
	else
	{
		m_bActive = false;
		m_bDownhill = false;
	}

	if ( m_bActive )
	{
		// Scroll the texture when the cart is on this hill.
		m_flScrollPerc += 0.02f;
		if ( m_flScrollPerc > 1.0f )
		{
			m_flScrollPerc -= 1.0f;
		}
	}

	surface()->DrawSetTexture( m_iTextureId );

	float flMod = m_flTextureScale + m_flScrollPerc;
	Vector2D x1( m_flScrollPerc, 0.0f );
	Vector2D x2( flMod, 0.0f );
	Vector2D y1( flMod, 1.0f );
	Vector2D y2( m_flScrollPerc, 1.0f );

	Vertex_t vert[4];
#if STAGING_FOURTEAM_FLIPPED
	if ( ( m_bDownhill && !m_bFlipped ) || ( !m_bDownhill && m_bFlipped ) )
#else
	if ( m_bDownhill )
#endif
	{
		vert[0].Init( Vector2D( 0.0f, 0.0f ), x2 );
		vert[1].Init( Vector2D( m_iWidth, 0.0f ), x1 );
		vert[2].Init( Vector2D( m_iWidth, m_iHeight ), y2 );
		vert[3].Init( Vector2D( 0.0f, m_iHeight ), y1 );
	}
	else
	{
		vert[0].Init( Vector2D( 0.0f, 0.0f ), x1 );
		vert[1].Init( Vector2D( m_iWidth, 0.0f ), x2 );
		vert[2].Init( Vector2D( m_iWidth, m_iHeight ), y1 );
		vert[3].Init( Vector2D( 0.0f, m_iHeight ), y2 );
	}

	surface()->DrawSetColor( COLOR_WHITE );
	surface()->DrawTexturedPolygon( 4, vert );
}


void CEscortHillPanel::PerformLayout( void )
{
	int x, y, textureWide, textureTall;
	GetBounds( x, y, m_iWidth, m_iHeight );
	surface()->DrawGetTextureSize( m_iTextureId, textureWide, textureTall );

	m_flTextureScale = (float)m_iWidth / ( (float)textureWide * ( (float)m_iHeight / (float)textureTall ) );

	SetAlpha( 64 );
}


void CEscortHillPanel::OnTick( void )
{
	if ( !IsVisible() )
		return;

	if ( m_bActive )
	{
		if ( m_bLowerAlpha )
		{
			// Lower alpha.
			GetAnimationController()->RunAnimationCommand( this, "alpha", 32.0f, 0.0f, 0.75f, AnimationController::INTERPOLATOR_LINEAR );
			m_bLowerAlpha = false;
		}
		else
		{
			// Rise alpha.
			GetAnimationController()->RunAnimationCommand( this, "alpha", 96.0f, 0.0f, 0.75f, AnimationController::INTERPOLATOR_LINEAR );
			m_bLowerAlpha = true;
		}
	}
	else
	{
		// Stop flashing.
		SetAlpha( 64 );
		m_bLowerAlpha = true;
	}
}


void CEscortHillPanel::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "teamplay_round_start" ) == 0 )
	{
		// Reset scrolling.
		m_flScrollPerc = 0.0f;
	}
}


//=============================================================================
// CTFHudEscortProgressBar
//=============================================================================
CTFHudEscortProgressBar::CTFHudEscortProgressBar( Panel *pParent, const char *pszName, int iTeam ) : ImagePanel( pParent, pszName )
{
	m_iTeamNum = iTeam;

	const char *pszTextureName;
	switch ( m_iTeamNum )
	{
		case TF_TEAM_RED:
			pszTextureName = "hud/cart_track_red_opaque";
			break;
		default:
		case TF_TEAM_BLUE:
			pszTextureName = "hud/cart_track_blue_opaque";
			break;
		case TF_TEAM_GREEN:
			pszTextureName = "hud/cart_track_green_opaque";
			break;
		case TF_TEAM_YELLOW:
			pszTextureName = "hud/cart_track_yellow_opaque";
			break;
	}

	m_iTextureId = surface()->DrawGetTextureId( pszTextureName );
	if ( m_iTextureId == -1 )
	{
		m_iTextureId = surface()->CreateNewTextureID( false );
		surface()->DrawSetTextureFile( m_iTextureId, pszTextureName, true, false );
	}

#if STAGING_FOURTEAM_FLIPPED
	m_bFlipped = false;
#endif
}


void CTFHudEscortProgressBar::Paint( void )
{
	if ( m_flProgress == 0.0f )
		return;

	surface()->DrawSetTexture( m_iTextureId );

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );

#if STAGING_FOURTEAM_FLIPPED
	float flProgress = m_flProgress;
#endif

	// Draw the bar.
	Vertex_t vert[4];
#if STAGING_FOURTEAM_FLIPPED
	if ( m_bFlipped )
	{
		flProgress = wide * fabs( flProgress - 1.0f );
		vert[0].Init( Vector2D( flProgress, 0.0f ), Vector2D( 0.0f, 0.0f ) );
		vert[1].Init( Vector2D( wide, 0.0f ), Vector2D( 1.0f, 0.0f ) );
		vert[2].Init( Vector2D( wide, tall ), Vector2D( 1.0f, 1.0f ) );
		vert[3].Init( Vector2D( flProgress, tall ), Vector2D( 0.0f, 1.0f ) );
	}
	else
#endif
	{
		wide *= m_flProgress;
		vert[0].Init( Vector2D( 0.0f, 0.0f ), Vector2D( 0.0f, 0.0f ) );
		vert[1].Init( Vector2D( wide, 0.0f ), Vector2D( 1.0f, 0.0f ) );
		vert[2].Init( Vector2D( wide, tall ), Vector2D( 1.0f, 1.0f ) );
		vert[3].Init( Vector2D( 0.0f, tall ), Vector2D( 0.0f, 1.0f ) );
	}

	Color colBar( 255, 255, 255, 210 );
	surface()->DrawSetColor( colBar );
	surface()->DrawTexturedPolygon( 4, vert );

	// Draw a line at the end.
	Color colLine( 245, 229, 196, 210 );
	surface()->DrawSetColor( colLine );

#if STAGING_FOURTEAM_FLIPPED
	if ( m_bFlipped )
	{
		surface()->DrawLine( flProgress, 0, flProgress, tall );
	}
	else
#endif
	{
		surface()->DrawLine( wide - 1, 0, wide - 1, tall );
	}
}
