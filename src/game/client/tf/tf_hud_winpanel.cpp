//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf2c_streamer_mode;

vgui::IImage *GetDefaultAvatarImage( int iPlayerIndex );
extern ConVar mp_bonusroundtime;

DECLARE_HUDELEMENT_DEPTH( CTFWinPanel, 1 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWinPanel::CTFWinPanel( const char *pElementName ) : EditablePanel( NULL, "WinPanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	SetAlpha( 0 );

	// This is needed for custom colors.
	SetScheme( vgui::scheme()->LoadSchemeFromFile( ClientSchemesArray[SCHEME_CLIENT_PATHSTRINGTF2C], ClientSchemesArray[SCHEME_CLIENT_STRINGTF2C] ) );

	m_pBGPanel = new EditablePanel( this, "WinPanelBGBorder" );
	m_pTeamScorePanel = new EditablePanel( this, "TeamScoresPanel" );
	m_pBlueBG = new EditablePanel( m_pTeamScorePanel, "BlueScoreBG" );
	m_pRedBG = new EditablePanel( m_pTeamScorePanel, "RedScoreBG" );

	m_pBlueBorder = NULL;
	m_pRedBorder = NULL;
	m_pGreenBorder = NULL;
	m_pYellowBorder = NULL;
	m_pBlackBorder = NULL;

	m_flTimeUpdateTeamScore = 0;
	m_flTimeSwitchTeams = 0.0f;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;
	m_iGreenTeamScore = 0;
	m_iYellowTeamScore = 0;
	m_iScoringTeam = 0;
	m_bShowingGreenYellow = false;

	RegisterForRenderGroup( "mid" );
}


void CTFWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}


void CTFWinPanel::Reset()
{
	m_bShouldBeVisible = false;
	m_flTimeUpdateTeamScore = 0.0f;
	m_flTimeSwitchTeams = 0.0f;
	m_iScoringTeam = 0;
}


void CTFWinPanel::Init()
{
	// listen for events
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;

	CHudElement::Init();
}

void CTFWinPanel::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	if ( state )
	{
		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}


void CTFWinPanel::FireGameEvent( IGameEvent * event )
{
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( "teamplay_round_start", pszEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_game_over", pszEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "tf_game_over", pszEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_win_panel", pszEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		int iWinningTeam = event->GetInt( "winning_team" );
		int iWinReason = event->GetInt( "winreason" );
		int iFlagCapLimit = event->GetInt( "flagcaplimit" );
		bool bRoundComplete = (bool)event->GetInt( "round_complete" );
		int iRoundsRemaining = event->GetInt( "rounds_remaining" );

		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "AdvancingTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );

		// set the appropriate background image and label text
		const char *pTeamLabel = NULL;
		const char *pTopPlayersLabel = NULL;
		const wchar_t *pLocalizedTeamName = GetLocalizedTeamName( iWinningTeam );
		// this is an area defense, but not a round win, if this was a successful defend until time limit but not a complete round
		bool bIsAreaDefense = ( ( WINREASON_DEFEND_UNTIL_TIME_LIMIT == iWinReason ) && !bRoundComplete );

		switch ( iWinningTeam )
		{
			case TF_TEAM_BLUE:
			{
				m_pBGPanel->SetBorder( m_pBlueBorder );
				pTeamLabel = ( bRoundComplete ? "#Winpanel_BlueWins" : ( bIsAreaDefense ? "#Winpanel_BlueDefends" : "#Winpanel_BlueAdvances" ) );
				pTopPlayersLabel = "#Winpanel_BlueMVPs";
				break;
			}
			case TF_TEAM_RED:
			{
				m_pBGPanel->SetBorder( m_pRedBorder );
				pTeamLabel = ( bRoundComplete ? "#Winpanel_RedWins" : ( bIsAreaDefense ? "#Winpanel_RedDefends" : "#Winpanel_RedAdvances" ) );
				pTopPlayersLabel = "#Winpanel_RedMVPs";
				break;
			}
			case TF_TEAM_GREEN:
			{
				m_pBGPanel->SetBorder( m_pGreenBorder );
				pTeamLabel = ( bRoundComplete ? "#Winpanel_GreenWins" : ( bIsAreaDefense ? "#Winpanel_GreenDefends" : "#Winpanel_GreenAdvances" ) );
				pTopPlayersLabel = "#Winpanel_GreenMVPs";
				break;
			}
			case TF_TEAM_YELLOW:
			{
				m_pBGPanel->SetBorder( m_pYellowBorder );
				pTeamLabel = ( bRoundComplete ? "#Winpanel_YellowWins" : ( bIsAreaDefense ? "#Winpanel_YellowDefends" : "#Winpanel_YellowAdvances" ) );
				pTopPlayersLabel = "#Winpanel_YellowMVPs";
				break;
			}
			case TEAM_UNASSIGNED:	// stalemate
			{
				m_pBGPanel->SetBorder( m_pBlackBorder );
				pTeamLabel = "#Winpanel_Stalemate";
				pTopPlayersLabel = "#Winpanel_TopPlayers";
				break;
			}
			default:
			{
				Assert( false );
				break;
			}
		}

		SetDialogVariable( bRoundComplete ? "WinningTeamLabel" : "AdvancingTeamLabel", g_pVGuiLocalize->Find( pTeamLabel ) );
		SetDialogVariable( "TopPlayersLabel", g_pVGuiLocalize->Find( pTopPlayersLabel ) );

		wchar_t wzWinReason[256] = L"";
		switch ( iWinReason )
		{
			case WINREASON_ALL_POINTS_CAPTURED:
			{
				if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
				{
					g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_PayloadPushed" ), 1, pLocalizedTeamName );
				}
				else if ( TFGameRules()->IsInKothMode() )
				{
					g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_KOTHPointHeld" ), 1, pLocalizedTeamName );
				}
				else if ( TFGameRules()->IsInArenaMode() )
				{
					g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_ArenaPointCaptured" ), 1, pLocalizedTeamName );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_AllPointsCaptured" ), 1, pLocalizedTeamName );
				}
				break;
			}
			case WINREASON_FLAG_CAPTURE_LIMIT:
			{
				wchar_t wzFlagCaptureLimit[16];
				V_swprintf_safe( wzFlagCaptureLimit, L"%i", iFlagCapLimit );
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( iFlagCapLimit > 1 ? "#Winreason_FlagCaptureLimit" : "Winreason_FlagCaptureLimit_One" ), 2,
					pLocalizedTeamName, wzFlagCaptureLimit );
				break;
			}
			case WINREASON_OPPONENTS_DEAD:
			{
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_OpponentsDead" ), 1, pLocalizedTeamName );
				break;
			}
			case WINREASON_DEFEND_UNTIL_TIME_LIMIT:
			{
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_DefendedUntilTimeLimit" ), 1, pLocalizedTeamName );
				break;
			}
			case WINREASON_STALEMATE:
			{
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_Stalemate" ), 0 );
				break;
			}
			case WINREASON_TIMELIMIT:
			{
				g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_TimeLimit"), 1, pLocalizedTeamName);
				break;
			}
			case WINREASON_WINLIMIT:
			{
				g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_WinLimit"), 1, pLocalizedTeamName);
				break;
			}
			case WINREASON_WINDIFFLIMIT:
			{
				g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_WinDiffLimit"), 1, pLocalizedTeamName);
				break;
			}
			case WINREASON_VIP_ESCAPED:
			{
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_VIPEscaped" ), 1, pLocalizedTeamName );
				break;
			}
			case WINREASON_ROUNDSCORELIMIT:
			{
				wchar_t wszScoreLimit[16];
				V_swprintf_safe(wszScoreLimit, L"%d", TFGameRules()->GetPointLimit() );

				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_RoundScoreLimit" ), 2,
					pLocalizedTeamName, wszScoreLimit );
				break;
			}
			case WINREASON_VIP_KILLED:
			{
				g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_VIPKilled" ), 1, pLocalizedTeamName );
				break;
			}
			default:
			{
				Assert( false );
				break;
			}
		}
		SetDialogVariable( "WinReasonLabel", wzWinReason );

		if ( !bRoundComplete && ( WINREASON_STALEMATE != iWinReason ) )
		{
			// if this was a mini-round, show # of capture points remaining
			wchar_t wzNumCapturesRemaining[16];
			wchar_t wzCapturesRemainingMsg[256] = L"";
			V_swprintf_safe( wzNumCapturesRemaining, L"%i", iRoundsRemaining );
			g_pVGuiLocalize->ConstructString( wzCapturesRemainingMsg, sizeof( wzCapturesRemainingMsg ),
				g_pVGuiLocalize->Find( 1 == iRoundsRemaining ? "#Winpanel_CapturePointRemaining" : "Winpanel_CapturePointsRemaining" ),
				1, wzNumCapturesRemaining );
			SetDialogVariable( "DetailsLabel", wzCapturesRemainingMsg );
		}
		else if ( iWinReason == WINREASON_ALL_POINTS_CAPTURED || iWinReason == WINREASON_FLAG_CAPTURE_LIMIT )
		{
			// if this was a full round that ended with point capture or flag capture, show the winning cappers
			const char *pCappers = event->GetString( "cappers" );
			int iCappers = Q_strlen( pCappers );
			if ( iCappers > 0 )
			{
				char szPlayerNames[256] = "";
				wchar_t wzPlayerNames[256] = L"";
				wchar_t wzCapMsg[512] = L"";
				for ( int i = 0; i < iCappers; i++ )
				{
					V_strcat_safe( szPlayerNames, g_PR->GetPlayerName( (int)pCappers[i] ) );
					if ( i < iCappers - 1 )
					{
						V_strcat_safe( szPlayerNames, ", " );
					}
				}
				g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerNames, wzPlayerNames, sizeof( wzPlayerNames ) );

				g_pVGuiLocalize->ConstructString( wzCapMsg, sizeof( wzCapMsg ), g_pVGuiLocalize->Find( "#Winpanel_WinningCapture" ), 1, wzPlayerNames );
				SetDialogVariable( "DetailsLabel", wzCapMsg );
			}
		}

		// get the current & previous team scores
		m_iBlueTeamScore = event->GetInt( "blue_score", 0 );
		m_iRedTeamScore = event->GetInt( "red_score", 0 );
		m_iGreenTeamScore = event->GetInt( "green_score", 0 );
		m_iYellowTeamScore = event->GetInt( "yellow_score", 0 );
		m_iScoringTeam = event->GetInt( "scoring_team" );

		if ( bRoundComplete )
		{
			// Show the team pair that has the scoring team first.
			SwitchScorePanels( m_iScoringTeam > TF_TEAM_BLUE, true );

			if ( TFGameRules()->IsFourTeamGame() )
			{
				// Show another team pair halfway through bonus time.
				m_flTimeSwitchTeams = gpGlobals->curtime + mp_bonusroundtime.GetFloat() * 0.5f;
			}
		}
		// only show team scores if round is complete
		m_pTeamScorePanel->SetVisible( bRoundComplete );

		if ( !g_TF_PR )
			return;

		// look for the top 3 players sent in the event
		for ( int i = 1; i <= 3; i++ )
		{
			bool bShow = false;
			char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
			// get player index and round points from the event
			V_sprintf_safe( szPlayerIndexVal, "player_%d", i );
			V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i );
			int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );
			int iRoundScore = event->GetInt( szPlayerScoreVal, 0 );
			// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
			if ( iRoundScore > 0 )
				bShow = true;

#if !defined( _X360 )
			CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( CFmtStr( "Player%dAvatar", i ) ) );
			if ( pPlayerAvatar )
			{
				pPlayerAvatar->ClearAvatar();
				pPlayerAvatar->SetShouldScaleImage( true );
				pPlayerAvatar->SetShouldDrawFriendIcon( false );

				if ( bShow )
				{
					pPlayerAvatar->SetDefaultAvatar( GetDefaultAvatarImage( iPlayerIndex ) );

					if ( !tf2c_streamer_mode.GetBool() )
					{
						pPlayerAvatar->SetPlayer( iPlayerIndex );
					}
				}
				pPlayerAvatar->SetVisible( bShow );
			}
#endif
			vgui::Label *pPlayerName = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dName", i ) ) );
			vgui::Label *pPlayerClass = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dClass", i ) ) );
			vgui::Label *pPlayerScore = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dScore", i ) ) );
			if ( !pPlayerName || !pPlayerClass || !pPlayerScore )
				return;

			if ( bShow )
			{
				// set the player labels to team color
				Color clr = g_PR->GetTeamColor( g_PR->GetTeam( iPlayerIndex ) );
				pPlayerName->SetFgColor( clr );
				pPlayerClass->SetFgColor( clr );
				pPlayerScore->SetFgColor( clr );

				// set label contents
				pPlayerName->SetText( g_PR->GetPlayerName( iPlayerIndex ) );
				pPlayerClass->SetText( g_aPlayerClassNames[g_TF_PR->GetPlayerClass( iPlayerIndex )] );
				pPlayerScore->SetText( CFmtStr( "%d", iRoundScore ) );
			}

			// show or hide labels for this player position
			pPlayerName->SetVisible( bShow );
			pPlayerClass->SetVisible( bShow );
			pPlayerScore->SetVisible( bShow );
		}

		m_bShouldBeVisible = true;

		MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/WinPanel.res" );

	m_pBlueBorder = pScheme->GetBorder( "TFFatLineBorderBlueBG" );
	m_pRedBorder = pScheme->GetBorder( "TFFatLineBorderRedBG" );
	m_pGreenBorder = pScheme->GetBorder( "TFFatLineBorderGreenBG" );
	m_pYellowBorder = pScheme->GetBorder( "TFFatLineBorderYellowBG" );
	m_pBlackBorder = pScheme->GetBorder( "TFFatLineBorder" );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFWinPanel::ShouldDraw()
{
	if ( !m_bShouldBeVisible )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTFWinPanel::OnThink()
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if ( m_flTimeUpdateTeamScore > 0 && ( gpGlobals->curtime >= m_flTimeUpdateTeamScore ) )
	{
		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );

		// update the team scores
		m_pTeamScorePanel->SetDialogVariable( "blueteamscore", GetTeamScore( GetLeftTeam(), false ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamscore", GetTeamScore( GetRightTeam(), false ) );

		m_flTimeUpdateTeamScore = 0;
	}

	if ( m_flTimeSwitchTeams > 0 && gpGlobals->curtime >= m_flTimeSwitchTeams )
	{
		SwitchScorePanels( !m_bShowingGreenYellow, true );
		m_flTimeSwitchTeams = 0;
	}
}


void CTFWinPanel::SwitchScorePanels( bool bShow, bool bSetScores )
{
	m_bShowingGreenYellow = bShow;

	// Reusing RED and BLU BGs and labels for GRN and YLW.
	if ( bShow )
	{
		m_pBlueBG->SetBorder( m_pGreenBorder );
		m_pRedBG->SetBorder( m_pYellowBorder );
	}
	else
	{
		m_pBlueBG->SetBorder( m_pBlueBorder );
		m_pRedBG->SetBorder( m_pRedBorder );
	}

	if ( bSetScores )
	{
		m_pTeamScorePanel->SetDialogVariable( "blueteamscore", GetTeamScore( GetLeftTeam(), true ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamscore", GetTeamScore( GetRightTeam(), true ) );

		m_pTeamScorePanel->SetDialogVariable( "blueteamname", GetLocalizedTeamName( GetLeftTeam() ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamname", GetLocalizedTeamName( GetRightTeam() ) );

		if ( m_iScoringTeam == GetLeftTeam() || m_iScoringTeam == GetRightTeam() )
		{
			// if the new scores are different, set ourselves to update the scoreboard to the new values after a short delay, so players
			// see the scores tick up
			m_flTimeUpdateTeamScore = gpGlobals->curtime + 2.0f;
		}
	}
}


int CTFWinPanel::GetLeftTeam( void )
{
	return ( m_bShowingGreenYellow ? TF_TEAM_GREEN : TF_TEAM_BLUE );
}


int CTFWinPanel::GetRightTeam( void )
{
	return ( m_bShowingGreenYellow ? TF_TEAM_YELLOW : TF_TEAM_RED );
}


int CTFWinPanel::GetTeamScore( int iTeam, bool bPrevious )
{
	int iScore = 0;

	switch ( iTeam )
	{
		case TF_TEAM_RED:
			iScore = m_iRedTeamScore;
			break;
		case TF_TEAM_BLUE:
			iScore = m_iBlueTeamScore;
			break;
		case TF_TEAM_GREEN:
			iScore = m_iGreenTeamScore;
			break;
		case TF_TEAM_YELLOW:
			iScore = m_iYellowTeamScore;
			break;
	}

	// If this is the winning team then their previous score is 1 point lower.
	if ( bPrevious && iTeam == m_iScoringTeam )
		iScore--;

	return iScore;
}
