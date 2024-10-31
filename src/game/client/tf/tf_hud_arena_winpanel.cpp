//=============================================================================//
//
// Purpose: Arena win panel
//
//=============================================================================//
#include "cbase.h"
#include "tf_hud_arena_winpanel.h"
#include "iclientmode.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include "vgui_avatarimage.h"
#include <engine/IEngineSound.h>
#include "tf_statsummary.h"
#include "tf_announcer.h"
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar tf2c_streamer_mode;

vgui::IImage *GetDefaultAvatarImage( int iPlayerIndex );
extern ConVar tf_arena_max_streak;
extern ConVar mp_bonusroundtime;

DECLARE_HUDELEMENT_DEPTH( CTFArenaWinPanel, 1 );

CTFArenaWinPanel::CTFArenaWinPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "ArenaWinPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	// This is needed for custom colors.
	SetScheme( vgui::scheme()->LoadSchemeFromFile( ClientSchemesArray[SCHEME_CLIENT_PATHSTRINGTF2C], ClientSchemesArray[SCHEME_CLIENT_STRINGTF2C] ) );

	m_pBGPanel = new EditablePanel( this, "WinPanelBGBorder" );
	m_pTeamScorePanel = new EditablePanel( this, "ArenaWinPanelScores" );
	m_pBlueBG = new EditablePanel( m_pTeamScorePanel, "BlueScoreBG" );
	m_pRedBG = new EditablePanel( m_pTeamScorePanel, "RedScoreBG" );
	m_pArenaStreakBG = new ScalableImagePanel( m_pTeamScorePanel, "ArenaStreaksBG" );
	m_pArenaStreakLabel = new CExLabel( m_pTeamScorePanel, "ArenaStreakLabel", "" );
	m_pWinnersPanel = new EditablePanel( this, "ArenaWinPanelWinnersPanel" );

	m_pBlueBorder = NULL;
	m_pRedBorder = NULL;
	m_pGreenBorder = NULL;
	m_pYellowBorder = NULL;
	m_pBlackBorder = NULL;

	m_flTimeUpdateTeamScore = 0.0f;
	m_flTimeSwitchTeams = 0.0f;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;
	m_bShouldBeVisible = false;
	m_iWinningTeam = TEAM_UNASSIGNED;
	m_bFlawlessVictory = false;
	memset( m_PlayerData, 0, sizeof( m_PlayerData ) );

	RegisterForRenderGroup( "mid" );

	ListenForGameEvent( "arena_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFArenaWinPanel::ShouldDraw( void )
{
	if ( !m_bShouldBeVisible )
		return false;

	return CHudElement::ShouldDraw();
}


void CTFArenaWinPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudArenaWinPanel.res" );

	m_pBlueBorder = pScheme->GetBorder( "TFFatLineBorderBlueBG" );
	m_pRedBorder = pScheme->GetBorder( "TFFatLineBorderRedBG" );
	m_pGreenBorder = pScheme->GetBorder( "TFFatLineBorderGreenBG" );
	m_pYellowBorder = pScheme->GetBorder( "TFFatLineBorderYellowBG" );
	m_pBlackBorder = pScheme->GetBorder( "TFFatLineBorder" );
}


void CTFArenaWinPanel::SetVisible( bool state )
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


void CTFArenaWinPanel::Reset()
{
	m_bShouldBeVisible = false;
	m_flTimeUpdateTeamScore = 0.0f;
	m_flTimeSwitchTeams = 0.0f;
	m_iWinningTeam = TEAM_UNASSIGNED;
	m_bFlawlessVictory = false;
	memset( m_PlayerData, 0, sizeof( m_PlayerData ) );
}


void CTFArenaWinPanel::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "arena_win_panel", pEventName ) == 0 )
	{
		if ( !g_TF_PR )
			return;

		m_bShouldBeVisible = true;

		m_iWinningTeam = event->GetInt( "scoring_team" );
		int iWinReason = event->GetInt( "winreason" );
		int iFlagCapLimit = event->GetInt( "flagcaplimit" );
		C_TFTeam *pWinningTeam = GetGlobalTFTeam( m_iWinningTeam );

		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "AdvancingTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );
		m_bFlawlessVictory = false;

		SetTeamBG( m_iWinningTeam );

		const wchar_t *pLocalizedTeamName = GetLocalizedTeamName( m_iWinningTeam );

		wchar_t wzWinReason[256] = L"";
		switch ( iWinReason )
		{
		case WINREASON_ALL_POINTS_CAPTURED:
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
		case WINREASON_FLAG_CAPTURE_LIMIT:
		{
			wchar_t wzFlagCaptureLimit[16];
			V_swprintf_safe( wzFlagCaptureLimit, L"%i", iFlagCapLimit );
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( iFlagCapLimit > 1 ? "#Winreason_FlagCaptureLimit" : "#Winreason_FlagCaptureLimit_One" ), 2,
				pLocalizedTeamName, wzFlagCaptureLimit );
		}
		break;
		case WINREASON_OPPONENTS_DEAD:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_Arena" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_DEFEND_UNTIL_TIME_LIMIT:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_DefendedUntilTimeLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_STALEMATE:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_Stalemate" ), 0 );
			break;
		case WINREASON_VIP_ESCAPED:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_VIPEscaped" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_ROUNDSCORELIMIT:
		{
			wchar_t wszScoreLimit[16];
			V_swprintf_safe(wszScoreLimit, L"%d", TFGameRules()->GetPointLimit());

			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_RoundScoreLimit" ), 2,
				pLocalizedTeamName, wszScoreLimit );
			break;
		}
		case WINREASON_VIP_KILLED:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_VIPKilled" ), 1, pLocalizedTeamName );
			break;
		default:
			Assert( false );
			break;
		}
		SetDialogVariable( "WinReasonLabel", wzWinReason );

		if ( m_iWinningTeam != TEAM_UNASSIGNED && pWinningTeam )
		{
			// See if this was a flawless victory.
			int iAlivePlayers = g_TF_PR->GetNumPlayersForTeam( m_iWinningTeam, true );

			if ( iAlivePlayers == pWinningTeam->GetNumPlayers() )
			{
				m_bFlawlessVictory = true;
			}
		}

		if ( iWinReason == WINREASON_ALL_POINTS_CAPTURED || iWinReason == WINREASON_FLAG_CAPTURE_LIMIT )
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
		else if ( m_bFlawlessVictory )
		{
			SetDialogVariable( "DetailsLabel", g_pVGuiLocalize->Find( "#TF_Arena_FlawlessVictory" ) );
		}

		// get the current & previous team scores
		m_iBlueTeamScore = event->GetInt( "blue_score", 0 );
		m_iRedTeamScore = event->GetInt( "red_score", 0 );
		m_iGreenTeamScore = event->GetInt( "green_score", 0 );
		m_iYellowTeamScore = event->GetInt( "yellow_score", 0 );
		m_iScoringTeam = event->GetInt( "scoring_team" );

		// Show the team pair that has the scoring team first.
		SwitchScorePanels( m_iScoringTeam > TF_TEAM_BLUE, true );

		if ( TFGameRules()->IsInArenaQueueMode() && tf_arena_max_streak.GetInt() > 0 )
		{
			m_pArenaStreakBG->SetVisible( true );
			m_pArenaStreakLabel->SetVisible( true );

			wchar_t wszMaxStreak[16], wszPlayingTo[128];
			V_swprintf_safe( wszMaxStreak, L"%d", tf_arena_max_streak.GetInt() );
			g_pVGuiLocalize->ConstructString( wszPlayingTo, sizeof( wszPlayingTo ), g_pVGuiLocalize->Find( "#TF_Arena_PlayingTo" ), 1, wszMaxStreak );

			m_pTeamScorePanel->SetDialogVariable( "arenastreaktext", wszPlayingTo );
		}
		else
		{
			m_pArenaStreakBG->SetVisible( false );
			m_pArenaStreakLabel->SetVisible( false );
		}

		if ( m_iWinningTeam != TEAM_UNASSIGNED )
		{
			// Show the losing team halfway through bonus time.
			m_flTimeSwitchTeams = gpGlobals->curtime + mp_bonusroundtime.GetFloat() * 0.5f;
		}

		// Remember the player data so we can show the losing team MVPs later.
		for ( int i = 1; i <= 6; i++ )
		{
			ArenaPlayerRoundScore_t &data = m_PlayerData[i - 1];

			data.iPlayerIndex = event->GetInt( VarArgs( "player_%d", i ) );
			data.iDamage = event->GetInt( VarArgs( "player_%d_damage", i ) );
			data.iHealing = event->GetInt( VarArgs( "player_%d_healing", i ) );
			data.iLifeTime = event->GetInt( VarArgs( "player_%d_lifetime", i ) );
			data.iKills = event->GetInt( VarArgs( "player_%d_kills", i ) );
		}

		SetupPlayerStats( true );
	}
}


void CTFArenaWinPanel::OnThink( void )
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if ( m_flTimeUpdateTeamScore > 0 && gpGlobals->curtime > m_flTimeUpdateTeamScore )
	{
		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );

		// update the team scores
		if ( TFGameRules()->IsInArenaQueueMode() )
		{
			// Losing team's score is reset.
			m_pTeamScorePanel->SetDialogVariable( "blueteamscore", ( m_iWinningTeam == TF_TEAM_BLUE || m_iWinningTeam == TF_TEAM_GREEN ) ? GetTeamScore( GetLeftTeam(), false ) : 0 );
			m_pTeamScorePanel->SetDialogVariable( "redteamscore", ( m_iWinningTeam == TF_TEAM_RED || m_iWinningTeam == TF_TEAM_YELLOW ) ? GetTeamScore( GetRightTeam(), false ) : 0 );
		}
		else
		{
			m_pTeamScorePanel->SetDialogVariable( "blueteamscore", GetTeamScore( GetLeftTeam(), false ) );
			m_pTeamScorePanel->SetDialogVariable( "redteamscore", GetTeamScore( GetRightTeam(), false ) );
		}

		m_flTimeUpdateTeamScore = 0;
	}

	if ( m_flTimeSwitchTeams > 0 && gpGlobals->curtime >= m_flTimeSwitchTeams )
	{
		if ( TFGameRules()->IsFourTeamGame() )
			SetTeamBG( TEAM_UNASSIGNED );
		else
			SetTeamBG( m_iWinningTeam == TF_TEAM_BLUE ? TF_TEAM_RED : TF_TEAM_BLUE );

		SetupPlayerStats( false );

		// Announce flawless victory if applicable.
		if ( m_bFlawlessVictory )
		{
			int iLocalTeam = GetLocalPlayerTeam();

			if ( iLocalTeam == m_iWinningTeam )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FLAWLESSVICTORY_TEAM );
			}
			else if ( iLocalTeam < FIRST_GAME_TEAM )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FLAWLESSVICTORY );
			}
			else if ( !TFGameRules()->IsFourTeamGame() )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FLAWLESSVICTORY_ENEMY );
			}
		}

		if ( TFGameRules()->IsFourTeamGame() )
			SwitchScorePanels( !m_bShowingGreenYellow, true );
		m_flTimeSwitchTeams = 0;
	}
}


void CTFArenaWinPanel::SetTeamBG( int iTeam )
{
	// set the appropriate background image and label text
	const wchar_t *pLocalizedTeamName = GetLocalizedTeamName( iTeam );
	const wchar_t *pLocalizedTeamWord = g_pVGuiLocalize->Find( "#Winpanel_Team1" );
	const wchar_t *pLocalizedTeamsWord = g_pVGuiLocalize->Find( "#Winpanel_Teams1" );

	const char *pTopPlayersLabel = NULL;

	switch ( iTeam )
	{
	case TF_TEAM_BLUE:
		m_pBGPanel->SetBorder( m_pBlueBorder );
		pTopPlayersLabel = "#Winpanel_BlueMVPs";
		break;
	case TF_TEAM_RED:
		m_pBGPanel->SetBorder( m_pRedBorder );
		pTopPlayersLabel = "#Winpanel_RedMVPs";
		break;
	case TF_TEAM_GREEN:
		m_pBGPanel->SetBorder( m_pGreenBorder );
		pTopPlayersLabel = "#Winpanel_GreenMVPs";
		break;
	case TF_TEAM_YELLOW:
		m_pBGPanel->SetBorder( m_pYellowBorder );
		pTopPlayersLabel = "#Winpanel_YellowMVPs";
		break;
	case TEAM_UNASSIGNED:	// stalemate
		m_pBGPanel->SetBorder( m_pBlackBorder );
		pTopPlayersLabel = "#Winpanel_TopPlayers";
		break;
	default:
		Assert( false );
		break;
	}

	if ( m_iWinningTeam == TEAM_UNASSIGNED )
	{
		// Stalemate.
		SetDialogVariable( "WinningTeamLabel", g_pVGuiLocalize->Find( "#Winpanel_Stalemate" ) );
		SetDialogVariable( "LosingTeamLabel", L"" );
	}
	else if ( iTeam == m_iWinningTeam )
	{
		// This is the winning team.
		wchar_t wszTeamWins[128];
		g_pVGuiLocalize->ConstructString( wszTeamWins, sizeof( wszTeamWins ), g_pVGuiLocalize->Find( "#Winpanel_TeamWins" ), 2,
			pLocalizedTeamName, pLocalizedTeamWord );

		SetDialogVariable( "WinningTeamLabel", wszTeamWins );
		SetDialogVariable( "LosingTeamLabel", L"" );
	}
	else
	{
		// This is a losing team.
		wchar_t wszTeamLost[128];
		if ( TFGameRules()->IsFourTeamGame() )
		{
			g_pVGuiLocalize->ConstructString( wszTeamLost, sizeof( wszTeamLost ), g_pVGuiLocalize->Find( "#Winpanel_TeamLost" ), 2,
				g_pVGuiLocalize->Find( "#TF_Other_Name" ), pLocalizedTeamsWord );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( wszTeamLost, sizeof( wszTeamLost ), g_pVGuiLocalize->Find( "#Winpanel_TeamLost" ), 2,
				pLocalizedTeamName, pLocalizedTeamWord );
		}

		SetDialogVariable( "WinningTeamLabel", L"" );
		SetDialogVariable( "LosingTeamLabel", wszTeamLost );
	}

	SetDialogVariable( "TopPlayersLabel", g_pVGuiLocalize->Find( pTopPlayersLabel ) );
}


void CTFArenaWinPanel::SetupPlayerStats( bool bWinners )
{
	for ( int i = 1; i <= 3; i++ )
	{
		ArenaPlayerRoundScore_t &data = m_PlayerData[bWinners ? i - 1 : i + 2];

		bool bShow = ( data.iPlayerIndex != 0 );

#if !defined( _X360 )
		CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dAvatar", i ) ) );

		if ( pPlayerAvatar )
		{
			pPlayerAvatar->ClearAvatar();
			pPlayerAvatar->SetShouldScaleImage( true );
			pPlayerAvatar->SetShouldDrawFriendIcon( false );

			if ( bShow )
			{
				pPlayerAvatar->SetDefaultAvatar( GetDefaultAvatarImage( data.iPlayerIndex ) );

				if ( !tf2c_streamer_mode.GetBool() )
				{
					pPlayerAvatar->SetPlayer( data.iPlayerIndex );
				}
			}
			pPlayerAvatar->SetVisible( bShow );
		}
#endif
		vgui::Label *pPlayerName = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dName", i ) ) );
		vgui::Label *pPlayerClass = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dClass", i ) ) );
		vgui::Label *pPlayerDamage = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dDamage", i ) ) );
		vgui::Label *pPlayerHealing = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dHealing", i ) ) );
		vgui::Label *pPlayerLifetime = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dLifetime", i ) ) );
		vgui::Label *pPlayerKills = dynamic_cast<Label *>( m_pWinnersPanel->FindChildByName( VarArgs( "Player%dKills", i ) ) );

		if ( !pPlayerName || !pPlayerClass || !pPlayerDamage || !pPlayerHealing || !pPlayerLifetime || !pPlayerKills )
			return;

		if ( bShow )
		{
			// set the player labels to team color
			Color clr = g_PR->GetTeamColor( g_PR->GetTeam( data.iPlayerIndex ) );
			pPlayerName->SetFgColor( clr );
			pPlayerClass->SetFgColor( clr );
			pPlayerDamage->SetFgColor( clr );
			pPlayerHealing->SetFgColor( clr );
			pPlayerLifetime->SetFgColor( clr );
			pPlayerKills->SetFgColor( clr );

			// set label contents
			pPlayerName->SetText( g_PR->GetPlayerName( data.iPlayerIndex ) );
			pPlayerClass->SetText( g_aPlayerClassNames[g_TF_PR->GetPlayerClass( data.iPlayerIndex )] );
			pPlayerDamage->SetText( VarArgs( "%d", data.iDamage ) );
			pPlayerHealing->SetText( VarArgs( "%d", data.iHealing ) );
			pPlayerLifetime->SetText( FormatSeconds( data.iLifeTime ) );
			pPlayerKills->SetText( VarArgs( "%d", data.iKills ) );
		}

		// show or hide labels for this player position
		pPlayerName->SetVisible( bShow );
		pPlayerClass->SetVisible( bShow );
		pPlayerDamage->SetVisible( bShow );
		pPlayerHealing->SetVisible( bShow );
		pPlayerLifetime->SetVisible( bShow );
		pPlayerKills->SetVisible( bShow );
	}
}


void CTFArenaWinPanel::SwitchScorePanels( bool bShow, bool bSetScores )
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


int CTFArenaWinPanel::GetLeftTeam( void )
{
	return ( m_bShowingGreenYellow ? TF_TEAM_GREEN : TF_TEAM_BLUE );
}


int CTFArenaWinPanel::GetRightTeam( void )
{
	return ( m_bShowingGreenYellow ? TF_TEAM_YELLOW : TF_TEAM_RED );
}


int CTFArenaWinPanel::GetTeamScore( int iTeam, bool bPrevious )
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
