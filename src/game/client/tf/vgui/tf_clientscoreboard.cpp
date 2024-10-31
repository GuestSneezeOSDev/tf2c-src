//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ScrollBar.h>
#include "vgui_avatarimage.h"
#include "tf_clientscoreboard.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui/IInput.h"
#include <inputsystem/iinputsystem.h>
#include "voice_status.h"
#include "gamevars_shared.h"
#include "c_tf_playerresource.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "tf_hud_statpanel.h"
#include "tf_gamerules.h"
#include "tf_fourteamscoreboard.h"

using namespace vgui;

void cc_scoreboard_convar_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CTFClientScoreBoardDialog *pScoreboard = static_cast<CTFClientScoreBoardDialog *>( gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD ) );
	if ( pScoreboard )
	{
		pScoreboard->Reset();
	}

	pScoreboard = static_cast<CTFFourTeamScoreBoardDialog *>( gViewPortInterface->FindPanelByName( PANEL_FOURTEAMSCOREBOARD ) );
	if ( pScoreboard )
	{
		pScoreboard->Reset();
	}
}
ConVar tf_scoreboard_ping_as_text( "tf_scoreboard_ping_as_text", "1", FCVAR_ARCHIVE, "Show ping values as text in the scoreboard.", cc_scoreboard_convar_changed );
ConVar tf_scoreboard_mouse_mode( "tf_scoreboard_mouse_mode", "2", FCVAR_ARCHIVE );
ConVar tf_scoreboard_alt_class_icons( "tf_scoreboard_alt_class_icons", "0", FCVAR_ARCHIVE, "Show alternate class icons in the scoreboard." );

extern ConVar cl_hud_playerclass_use_playermodel;

extern ConVar tf2c_streamer_mode;

extern ConVar tf2c_show_nemesis_relationships;

bool IsInCommentaryMode( void );
const char *GetMapDisplayName( const char *mapName );
const wchar_t *GetPointsString( int iPoints );

extern const char *g_aPlayerNamesGeneric[TF_TEAM_COUNT];

// Corresponds to what ping a player roughly has or displays their status as a bot.
enum
{
	PING_LOW,
	PING_MED,
	PING_HIGH,
	PING_VERY_HIGH,
	PING_BOT_RED,
	PING_BOT_BLUE,
	PING_BOT_GREEN,
	PING_BOT_YELLOW
};

// Texture names for the ping icons.
static const char *pszPingIcons[TF_SCOREBOARD_MAX_PING_ICONS * 2] =
{
	// Alive
	"../hud/scoreboard_ping_low",
	"../hud/scoreboard_ping_med",
	"../hud/scoreboard_ping_high",
	"../hud/scoreboard_ping_very_high",
	"../hud/scoreboard_ping_bot_red",
	"../hud/scoreboard_ping_bot_blue",
	"../hud/scoreboard_ping_bot_green",
	"../hud/scoreboard_ping_bot_yellow",

	// Dead
	"../hud/scoreboard_ping_low_d",
	"../hud/scoreboard_ping_med_d",
	"../hud/scoreboard_ping_high_d",
	"../hud/scoreboard_ping_very_high_d",
	"../hud/scoreboard_ping_bot_red_d",
	"../hud/scoreboard_ping_bot_blue_d",
	"../hud/scoreboard_ping_bot_green_d",
	"../hud/scoreboard_ping_bot_yellow_d",
};
COMPILE_TIME_ASSERT( TF_SCOREBOARD_MAX_PING_ICONS == ( PING_BOT_YELLOW + 1 ) );

// Panel names for stat labels.
static const char *pszStatLabels[TFSTAT_MAX] =
{
	"",				// TFSTAT_UNDEFINED
	"",				// TFSTAT_SHOTS_HIT
	"",				// TFSTAT_SHOTS_FIRED
	"Kills",		// TFSTAT_KILLS
	"Deaths",		// TFSTAT_DEATHS
	"Damage",		// TFSTAT_DAMAGE
	"Captures",		// TFSTAT_CAPTURES
	"Defenses",		// TFSTAT_DEFENSES
	"Domination",	// TFSTAT_DOMINATIONS
	"Revenge",		// TFSTAT_REVENGE
	"",				// TFSTAT_POINTSSCORED
	"Destruction",	// TFSTAT_BUILDINGSDESTROYED
	"Headshots",	// TFSTAT_HEADSHOTS
	"",				// TFSTAT_PLAYTIME
	"Healing",		// TFSTAT_HEALING
	"Invuln",		// TFSTAT_INVULNS
	"Assists",		// TFSTAT_KILLASSISTS
	"Backstabs",	// TFSTAT_BACKSTABS
	"",				// TFSTAT_HEALTHLEACHED
	"",				// TFSTAT_BUILDINGSBUILT
	"",				// TFSTAT_MAXSENTRYKILLS
	"Teleports",	// TFSTAT_TELEPORTS
	"",				// TFSTAT_SUICIDES
	"",				// TFSTAT_ENV_DEATHS
	"Bonus",		// TFSTAT_BONUS_POINTS
	"Support",		// TFSTAT_DAMAGE_ASSIST - This stat kind of corresponds to this panel (and the one below this), so I'll put it here.
	"",				// TFSTAT_DAMAGE_BLOCKED
	"JumppadJumps",	//TFSTAT_JUMPPAD_JUMPS
};
COMPILE_TIME_ASSERT( TF_SCOREBOARD_MAX_STAT_LABELS == TFSTAT_MAX );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::CTFClientScoreBoardDialog( IViewPort *pViewPort, const char *pszName, int iTeamCount ) : CClientScoreBoardDialog( pViewPort )
{
	m_iTeamCount = iTeamCount;

	MakePopup( true );

	// Create player lists.
	int i;
	for ( i = FIRST_GAME_TEAM; i < m_iTeamCount; i++ )
	{
		m_pPlayerLists[i] = new SectionedListPanel( this, VarArgs( "%sPlayerList", g_aTeamNames[i] ) );
		m_pPlayerListsVIP[i] = new SectionedListPanel(this, VarArgs( "%sVIPPlayerList", g_aTeamNames[i] ) );
	}

	m_pLabelPlayerName = new CExLabel( this, "PlayerNameLabel", "" );
	m_pLabelMapName = new CExLabel( this, "MapName", "" );
	m_pImagePanelHorizLine = new ImagePanel( this, "HorizontalLine" );
	m_pClassImage = new CTFClassImage( this, "ClassImage" );
	m_pClassModelPanel = new CTFPlayerModelPanel( this, "classmodelpanel" );
	m_pLocalPlayerStatsPanel = new EditablePanel( this, "LocalPlayerStatsPanel" );
	m_pLocalPlayerDuelStatsPanel = new EditablePanel( this, "LocalPlayerDuelStatsPanel" );
	m_pServerTimeLeftValue = NULL;
	m_pRightClickMenu = NULL;

	m_iImageDead = 0;
	m_iImageDominated = 0;
	m_iImageNemesis = 0;

	V_memset( m_iImageClasses, 0, sizeof( m_iImageClasses ) );
	V_memset( m_iImageClassesAlt, 0, sizeof( m_iImageClasses ) );
	V_memset( m_iImageDominations, 0, sizeof( m_iImageDominations ) );
	V_memset( m_iImageDefaultAvatars, 0, sizeof( m_iImageDefaultAvatars ) );
	V_memset( m_iImagePings, 0, sizeof( m_iImagePings ) );
	V_memset( m_iImageMedals, 0, sizeof( m_iImageMedals ) );

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );

	m_iSelectedPlayerIndex = -1;
	m_bMouseActivated = false;
	for ( i = FIRST_GAME_TEAM; i < m_iTeamCount; i++ )
	{
		m_bListScrollBarVisible[i] = false;
	}
	m_bListScrollBarVisibleVIP = false;
	m_iExtraSpace = 0;

	ListenForGameEvent( "game_maploaded" );

	V_wcscpy_safe( m_wzServerLabel, TF2C_STREAMER_MODE_SERVER_NAME );

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::~CTFClientScoreBoardDialog()
{
}


void CTFClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions;
	C_TFGameRules *pTFGameRules = TFGameRules();
	bool bIsVIPMode = ( pTFGameRules && pTFGameRules->IsVIPMode() );
	if ( bIsVIPMode )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_vip" );

		// Count the number of VIPs.
		int iVIPCount = 0;
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < TF_TEAM_COUNT; iTeam++ ) 
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( pTeam && pTeam->IsEscorting() )
			{
				iVIPCount++;
				AddSubKeyNamed( pConditions, VarArgs( "if_vip_%s", g_aTeamNames[iTeam] ) );
			}
		}

		if ( iVIPCount > 1 )
		{
			AddSubKeyNamed( pConditions, "if_vipr" );
		}
	}
	else
	{
		pConditions = NULL;
	}

	LoadControlSettings( GetResFilename(), NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_mapAvatarsToImageList.RemoveAll();

	int i, c, j;
	if ( m_pImageList )
	{
		m_iImageDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dead", true ) );
		m_iImageDominated = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dominated", true ) );
		m_iImageNemesis = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_nemesis", true ) );

		for ( i = TF_FIRST_NORMAL_CLASS, c = TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 ); i < c; i++ )
		{
			m_iImageClasses[i] = m_pImageList->AddImage( scheme()->GetImage( g_aPlayerClassEmblems[i - 1], true ) );
			m_iImageClassesAlt[i] = m_pImageList->AddImage( scheme()->GetImage( g_aPlayerClassEmblemsAlt[i - 1], true ) );
		}

		for ( i = 0; i < TF_SCOREBOARD_MAX_DOMINATIONS; i++ )
		{
			m_iImageDominations[i] = m_pImageList->AddImage( scheme()->GetImage( VarArgs( "../hud/leaderboard_dom%d", i + 1 ), true ) );
		}

		for ( i = 0, c = TF_SCOREBOARD_MAX_PING_ICONS * 2; i < c; i++ )
		{
			m_iImagePings[i] = m_pImageList->AddImage( scheme()->GetImage( pszPingIcons[i], true ) );
		}

		// Resize the images to our resolution.
		for ( i = 1, c = m_pImageList->GetImageCount(); i < c; i++ )
		{
			m_pImageList->GetImage( i )->SetSize( scheme()->GetProportionalScaledValueEx( GetScheme(), 13 ), scheme()->GetProportionalScaledValueEx( GetScheme(), 13 ) );
		}

		const char *pszMedalImage;
		for ( i = 0; i < 3; i++ )
		{
			for ( j = FIRST_GAME_TEAM; j < TF_TEAM_COUNT; j++ )
			{
				pszMedalImage = GetTeamMedalString( j, i == 0 ? TF_RANK_PLAYTESTER : i == 1 ? TF_RANK_CONTRIBUTOR : TF_RANK_DEVELOPER );
				if ( !pszMedalImage )
					continue;

				// Uniquely scale the medals because of their rectangular size.
				vgui::IImage *pMedalImage = scheme()->GetImage( pszMedalImage, true );
				pMedalImage->SetSize( scheme()->GetProportionalScaledValueEx( GetScheme(), 8 ), scheme()->GetProportionalScaledValueEx( GetScheme(), 16 ) );
				m_iImageMedals[j][i] = m_pImageList->AddImage( pMedalImage );
			}
		}

		for ( i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
		{
			CAvatarImage *pImage = new CAvatarImage();
			pImage->SetAvatarSize( 32, 32 ); // Deliberately non-scaling.
			pImage->SetDefaultImage( scheme()->GetImage( VarArgs( "../vgui/avatar_default_%s", g_aTeamLowerNames[i] ), true ) );
			m_iImageDefaultAvatars[i] = m_pImageList->AddImage( pImage );
		}
	}

	for ( i = TFSTAT_UNDEFINED; i < TFSTAT_MAX; i++ )
	{
		const char* szLabel = pszStatLabels[i];
		m_pLocalPlayerStatLabels[i] = dynamic_cast<CExLabel *>(m_pLocalPlayerStatsPanel->FindChildByName(szLabel));
		if (szLabel)
		{
			char szLabelLabel[64];
			V_strcpy_safe(szLabelLabel, szLabel);
			V_strcat_safe(szLabelLabel, "Label");
			m_pLocalPlayerStatLabelsLabels[i] = dynamic_cast<CExLabel *>(m_pLocalPlayerStatsPanel->FindChildByName(szLabelLabel));
		}
		else
		{
			m_pLocalPlayerStatLabelsLabels[i] = nullptr;
		}
	}

	// There's an underlying player list that we aren't using, hide it.
	if ( m_pPlayerList )
	{
		m_pPlayerList->SetVisible( false );
	}

	for ( i = FIRST_GAME_TEAM; i < m_iTeamCount; i++ )
	{
		SetPlayerListImages( m_pPlayerLists[i] );

		if ( m_pPlayerListsVIP[i] )
		{
			SetPlayerListImages( m_pPlayerListsVIP[i] );
			m_pPlayerListsVIP[i]->SetVisible( bIsVIPMode &&m_iTeamCount != TF_TEAM_COUNT );
		}
	}

	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetBorder( NULL );
	SetVisible( false );

	m_hTimeLeftFont = pScheme->GetFont( "ScoreboardMedium", true );
	m_hTimeLeftNotSetFont = pScheme->GetFont( "ScoreboardVerySmall", true );

	// Dueling isn't in yet, so we don't need it.
	m_pLocalPlayerDuelStatsPanel->SetVisible( false );

	m_pServerTimeLeftValue = dynamic_cast<CExLabel *>( FindChildByName( "ServerTimeLeftValue" ) );

	Reset();
}


void CTFClientScoreBoardDialog::ShowPanel( bool bShow )
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if ( !m_pImageList )
	{
		InvalidateLayout( true, true );
	}

	// Don't show in commentary mode
	if ( IsInCommentaryMode() )
	{
		bShow = false;
	}

	if ( bShow == IsVisible() )
		return;

	int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "global" );

	if ( bShow )
	{
		SetVisible( true );
		MoveToFront();
		InitializeInputScheme();

		gHUD.LockRenderGroup( iRenderGroup );

		// Clear the selected item, this forces the default to the local player
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			pList->ClearSelection();
		}

		int iMouseMode = tf_scoreboard_mouse_mode.GetInt();
		if ( iMouseMode )
		{
			if ( iMouseMode > 1 )
			{
				m_bMouseActivated = false;
			}
			else
			{
				m_fNextUpdateTime = 0;
			}
		}
	}
	else
	{
		SetVisible( false );

		gHUD.UnlockRenderGroup( iRenderGroup );

		m_bMouseActivated = false;

		if ( m_pRightClickMenu )
		{
			delete m_pRightClickMenu;
			m_pRightClickMenu = NULL;
		}
	}
}


void CTFClientScoreBoardDialog::OnCommand( const char *command )
{
	if ( m_iSelectedPlayerIndex <= 0 )
		return;

	if ( !V_strcmp( command, "muteplayer" ) )
	{
		GetClientVoiceMgr()->SetPlayerBlockedState( m_iSelectedPlayerIndex, !GetClientVoiceMgr()->IsPlayerBlocked( m_iSelectedPlayerIndex ) );
	}
	else if ( !V_strcmp( command, "kick" ) )
	{
		if ( g_TF_PR )
		{
			int iUserID = g_TF_PR->GetUserID( m_iSelectedPlayerIndex );
			if ( iUserID )
			{
				engine->ClientCmd( VarArgs( "callvote Kick %d", iUserID ) );
			}
		}
	}
	else if ( !V_strcmp( command, "specplayer" ) )
	{
		engine->ClientCmd( VarArgs( "spec_player %d", m_iSelectedPlayerIndex ) );
	}
	else if ( !V_strcmp( command, "profile" ) )
	{
		CSteamID steamID;
		if ( g_TF_PR && g_TF_PR->GetSteamID( m_iSelectedPlayerIndex, &steamID ) )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "steamid", steamID );
		}
	}
}


void CTFClientScoreBoardDialog::OnItemSelected( vgui::Panel *panel )
{
	int i, j;
	for ( i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		bool bIPlayerListsSelected = ( panel == m_pPlayerLists[i] && m_pPlayerLists[i]->GetSelectedItem() >= 0 );
		bool bIPlayerListsVIPSelected = ( panel == m_pPlayerListsVIP[i] && m_pPlayerListsVIP[i]->GetSelectedItem() >= 0 );

		if ( bIPlayerListsSelected || bIPlayerListsVIPSelected )
		{
			// There can be only one selection.
			for ( j = m_iTeamCount - 1; j >= FIRST_GAME_TEAM; j-- )
			{
				bool bJPlayerListsSelected = m_pPlayerLists[j] && m_pPlayerLists[j]->GetSelectedItem() >= 0;
				bool bJPlayerListsVIPSelected = m_pPlayerListsVIP[j] && m_pPlayerListsVIP[j]->GetSelectedItem() >= 0;

				/*
				 * If we release our mouse on a different item then we clear the selection.
				 *
				 * We first check they type of the selected list, if one is a vip list and the
				 * other a regular list then it's different so we clear our selection.
				 *
				 * If it's the same list type then these first 2 cases won't be triggered, that is
				 * where the third case comes in. This case checks if the team number is different.
				 * So if the first list was a player list and the second list was also a player
				 * list but from a different team we clear the selection on the original list and
				 * will have selected the item on our new list.
				 */
				if ( bJPlayerListsVIPSelected && bIPlayerListsSelected )
				{
					m_pPlayerListsVIP[j]->ClearSelection();
				}
				else if ( bJPlayerListsSelected && bIPlayerListsVIPSelected )
				{
					m_pPlayerLists[j]->ClearSelection();
				}
				else if ( i != j )
				{
					if ( bJPlayerListsVIPSelected ) m_pPlayerListsVIP[j]->ClearSelection();
					else if ( bJPlayerListsSelected ) m_pPlayerLists[j]->ClearSelection();
				}
			}

			if ( vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
			{
				OnScoreBoardMouseRightRelease();
			}
			break;
		}
	}
}


void CTFClientScoreBoardDialog::OnItemContextMenu( vgui::Panel *panel )
{
	// Check if the right mouse button is down before we commit to the for loop.
	if ( vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
	{
		for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
		{
			if ( panel == m_pPlayerLists[i] || panel == m_pPlayerListsVIP[i] )
			{
				OnScoreBoardMouseRightRelease();
				break;
			}
		}
	}
}


void CTFClientScoreBoardDialog::OnScoreBoardMouseRightRelease( void )
{
	if ( !IsVisible() )
		return;

	SectionedListPanel *pList = GetSelectedPlayerList();
	if ( !pList )
		return;

	int iSelectedItem = pList->GetSelectedItem();
	if ( iSelectedItem < 0 )
		return;

	KeyValues *pKeyValues = pList->GetItemData( iSelectedItem );
	if ( !pKeyValues )
		return;
		
	int playerIndex = pKeyValues->GetInt( "playerIndex", 0 );
	if ( GetLocalPlayerIndex() == playerIndex )
		return;

	m_iSelectedPlayerIndex = playerIndex;

	if ( m_pRightClickMenu )
		delete m_pRightClickMenu;

	m_pRightClickMenu = new Menu( pList, "RightClickMenu" );
	m_pRightClickMenu->SetKeyBoardInputEnabled( false );
	m_pRightClickMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( "DarkComboBoxBorder" ) );
	m_pRightClickMenu->SetFgColor( Color( 0, 0, 255, 255 ) );
	m_pRightClickMenu->SetPaintBackgroundEnabled( true );
	m_pRightClickMenu->SetPaintBackgroundType( 0 );
	m_pRightClickMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( "HudFontMediumSecondary" ) );

	bool bFakeClient = ( g_TF_PR->IsFakePlayer( playerIndex ) );

	MenuBuilder contextMenuBuilder( m_pRightClickMenu, this );

	// Mute
	if ( !bFakeClient && GetClientVoiceMgr() )
	{
		contextMenuBuilder.AddMenuItem( GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) ? "#TF_ScoreBoard_Unmute" : "#TF_ScoreBoard_Mute", "muteplayer", "mute" );
	}

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer )
	{
		if ( !engine->IsPlayingDemo() )
		{
			// Kick
			if ( g_TF_PR->GetTeam( playerIndex ) == pLocalTFPlayer->GetTeamNumber() )
			{
				contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Kick", "kick", "kick" );
			}
		}

		// Spectate
		if ( g_TF_PR->IsAlive( playerIndex ) && ( !pLocalTFPlayer->IsAlive() || pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR ) )
		{
			contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_Spectate", "specplayer", "spectate" );
		}
	}

	// Profile
	if ( !bFakeClient )
	{
		contextMenuBuilder.AddMenuItem( "#TF_ScoreBoard_ShowProfile", "profile", "profile" );
	}

	int x, y;
	vgui::input()->GetCursorPosition( x, y );
	m_pRightClickMenu->SetPos( x, y );
	m_pRightClickMenu->SetVisible( true );
	m_pRightClickMenu->AddActionSignalTarget( this );
}


bool CTFClientScoreBoardDialog::UseMouseMode( void )
{
	int iMouseMode = tf_scoreboard_mouse_mode.GetInt();
	return iMouseMode == 1 || ( iMouseMode > 1 && m_bMouseActivated );
}


void CTFClientScoreBoardDialog::InitializeInputScheme( void )
{
	if ( !IsVisible() )
		return;

	bool bUseMouseMode = UseMouseMode();
	//MakePopup( false, bUseMouseMode );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( bUseMouseMode );

	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		InitializeInputSchemeForList( m_pPlayerLists[i], bUseMouseMode );
		InitializeInputSchemeForList( m_pPlayerListsVIP[i], bUseMouseMode );
	}

	m_fNextUpdateTime = 0;
}

void CTFClientScoreBoardDialog::InitializeInputSchemeForList( vgui::SectionedListPanel *pListPanel, bool bUseMouseMode )
{
	if ( pListPanel )
	{
		//pListPanel->SetMouseInputEnabled( bUseMouseMode );
		pListPanel->AddActionSignalTarget( this );
		pListPanel->SetClickable( bUseMouseMode );

		vgui::ScrollBar *pScrollBar = pListPanel->GetScrollBar();
		if ( pScrollBar &&
			pScrollBar->GetButton( 0 ) &&
			pScrollBar->GetButton( 1 ) &&
			pScrollBar->GetSlider() )
		{
			pListPanel->SetVerticalScrollbar( bUseMouseMode );
			pScrollBar->SetMouseInputEnabled( bUseMouseMode );
			pScrollBar->GetButton( 0 )->SetMouseInputEnabled( bUseMouseMode );
			pScrollBar->GetButton( 1 )->SetMouseInputEnabled( bUseMouseMode );
			pScrollBar->GetSlider()->SetMouseInputEnabled( bUseMouseMode );
		}
	}
}


void CTFClientScoreBoardDialog::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	if ( !g_TF_PR )
		return;

	if ( g_TF_PR->IsFakePlayer( playerIndex ) || tf2c_streamer_mode.GetBool() )
	{
		// Show default avatars for bots.
		kv->SetInt( "avatar", GetDefaultAvatar( playerIndex ) );
	}
	else
	{
		// Get their avatar from Steam.
		CSteamID steamIDForPlayer;
		if ( g_TF_PR->GetSteamID( playerIndex, &steamIDForPlayer ) )
		{
			// See if we already have that avatar in our list
			int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
			int iImageIndex;
			if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
			{
				CAvatarImage *pImage = new CAvatarImage();
				pImage->SetAvatarSteamID( steamIDForPlayer );
				pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
				iImageIndex = m_pImageList->AddImage( pImage );

				m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
			}
			else
			{
				iImageIndex = m_mapAvatarsToImageList[iMapIndex];
			}

			kv->SetInt( "avatar", iImageIndex );

			CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( iImageIndex );
			pAvIm->UpdateFriendStatus();
		}
	}
}


int CTFClientScoreBoardDialog::GetDefaultAvatar( int playerIndex )
{
	return m_iImageDefaultAvatars[g_TF_PR->GetTeam( playerIndex )];
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CTFClientScoreBoardDialog::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1;

	if ( !down )
		return 1;

	if ( tf_scoreboard_mouse_mode.GetInt() > 1 )
	{
		if ( !m_bMouseActivated && keynum == MOUSE_RIGHT )
		{
			m_bMouseActivated = true;
			InitializeInputScheme();
			return 0;
		}
	}

	return 1;
}


void CTFClientScoreBoardDialog::OnStreamerModeChanged( bool bEnabled )
{
	if ( !bEnabled )
	{
		// Reset the server name back to the actual one.
		SetDialogVariable( "server", m_wzServerLabel );
	}
	else
	{
		SetDialogVariable( "server", TF2C_STREAMER_MODE_SERVER_NAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scoreboard panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Reset()
{
	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		InitPlayerList( m_pPlayerLists[i] );

		if ( m_pPlayerListsVIP[i] )
			InitPlayerList( m_pPlayerListsVIP[i]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::InitPlayerList( SectionedListPanel *pPlayerList )
{
	pPlayerList->SetVerticalScrollbar( UseMouseMode() );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	pPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	pPlayerList->SetBorder( NULL );

	pPlayerList->AddColumnToSection( 0, "medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iMedalWidth );

	// Avatars are always displayed at 32x32 regardless of resolution
	pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
	pPlayerList->AddColumnToSection( 0, "spacer", "", 0, m_iSpacerWidth );

	// Stretch player name column so that it fills up all unused space.
	m_iExtraSpace = pPlayerList->GetWide() - m_iMedalWidth - m_iAvatarWidth - m_iSpacerWidth - m_iNameWidth - m_iStatusWidth - m_iNemesisWidth - m_iNemesisWidth - m_iScoreWidth - m_iClassWidth - m_iPingWidth - m_iSpacerWidth - ( 2 * SectionedListPanel::COLUMN_DATA_INDENT );
	pPlayerList->AddColumnToSection( 0, "name", "#TF_Scoreboard_Name", 0, m_iNameWidth + m_iExtraSpace );
	pPlayerList->AddColumnToSection( 0, "status", "", SectionedListPanel::COLUMN_IMAGE, m_iStatusWidth );
	pPlayerList->AddColumnToSection( 0, "domination", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iNemesisWidth );
	pPlayerList->AddColumnToSection( 0, "nemesis", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iNemesisWidth );
	pPlayerList->AddColumnToSection( 0, "class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iClassWidth );
	pPlayerList->AddColumnToSection( 0, "score", "#TF_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, m_iScoreWidth );
	
	if ( tf_scoreboard_ping_as_text.GetBool() )
	{
		pPlayerList->AddColumnToSection( 0, "ping", "#TF_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );
	}
	else
	{
		pPlayerList->AddColumnToSection( 0, "ping", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds the image list to use in the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::SetPlayerListImages( vgui::SectionedListPanel *pPlayerList )
{
	pPlayerList->SetImageList( m_pImageList, false );
	pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Update()
{
	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateSpectatorList();
	UpdateArenaWaitingToPlayList();
	UpdatePlayerDetails();
	MoveToCenterOfScreen();
	AdjustForVisibleScrollbar();

	// Not really sure where to put this
	if ( TFGameRules() )
	{
		if ( mp_timelimit.GetInt() > 0 )
		{
			if ( TFGameRules()->GetTimeLeft() > 0 )
			{
				if ( m_pServerTimeLeftValue )
				{
					m_pServerTimeLeftValue->SetFont( m_hTimeLeftFont );
				}
				
				int iTimeLeft = TFGameRules()->GetTimeLeft();

				wchar_t wszHours[5] = L"";
				wchar_t wszMinutes[3] = L"";
				wchar_t wszSeconds[3] = L"";

				if ( iTimeLeft >= 3600 )
				{
					V_swprintf_safe( wszHours, L"%d", iTimeLeft / 3600 );
					V_swprintf_safe( wszMinutes, L"%02d", ( iTimeLeft / 60 ) % 60 );
					V_swprintf_safe( wszSeconds, L"%02d", iTimeLeft % 60 );
				}
				else
				{
					V_swprintf_safe( wszMinutes, L"%d", iTimeLeft / 60 );
					V_swprintf_safe( wszSeconds, L"%02d", iTimeLeft % 60 );
				}

				wchar_t wzTimeLabelOld[256] = L"";
				wchar_t wzTimeLabelNew[256] = L"";

				if ( iTimeLeft >= 3600 )
				{
					g_pVGuiLocalize->ConstructString( wzTimeLabelOld, sizeof( wzTimeLabelOld ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeft" ), 3, wszHours, wszMinutes, wszSeconds );
					g_pVGuiLocalize->ConstructString( wzTimeLabelNew, sizeof( wzTimeLabelNew ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNew" ), 3, wszHours, wszMinutes, wszSeconds );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( wzTimeLabelOld, sizeof( wzTimeLabelOld ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHours" ), 2, wszMinutes, wszSeconds );
					g_pVGuiLocalize->ConstructString( wzTimeLabelNew, sizeof( wzTimeLabelNew ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHoursNew" ), 2, wszMinutes, wszSeconds );
				}

				SetDialogVariable( "servertimeleft", wzTimeLabelOld );
				SetDialogVariable( "servertime", wzTimeLabelNew );
			}
			else // Timer is set and has run out.
			{
				if ( m_pServerTimeLeftValue )
				{
					m_pServerTimeLeftValue->SetFont( m_hTimeLeftNotSetFont );
				}

				SetDialogVariable( "servertimeleft", g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEnd" ) );
				SetDialogVariable( "servertime", g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEndNew" ) );
			}
		}
		else
		{
			if ( m_pServerTimeLeftValue )
			{
				m_pServerTimeLeftValue->SetFont( m_hTimeLeftNotSetFont );
			}

			SetDialogVariable( "servertimeleft", g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimit" ) );
			SetDialogVariable( "servertime", g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimitNew" ) );
		}
	}

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateTeamInfo( void )
{
	// Update the team sections in the scoreboard.
	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		C_TFTeam *team = GetGlobalTFTeam( i );
		if ( !team )
			continue;

		// Update # of players on each team.
		wchar_t string1[1024];
		wchar_t wNumPlayers[6];
		V_swprintf_safe( wNumPlayers, L"%i", team->Get_Number_Players() );
		if ( team->Get_Number_Players() == 1 )
		{
			g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Player" ), 1, wNumPlayers );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Players" ), 1, wNumPlayers );
		}

		// Set # of players for team in dialog.
		SetDialogVariable( VarArgs( "%steamplayercount", g_aTeamLowerNames[i] ), string1 );

		// Set team score in dialog.
		SetDialogVariable( VarArgs( "%steamscore", g_aTeamLowerNames[i] ), team->Get_Score() );

		// Set team name.
		SetDialogVariable( VarArgs( "%steamname", g_aTeamLowerNames[i] ), team->GetTeamName() );
	}
}


void CTFClientScoreBoardDialog::AdjustForVisibleScrollbar( void )
{
	vgui::ScrollBar *pScrollBar;
	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		if ( m_pPlayerLists[i] )
		{
			pScrollBar = m_pPlayerLists[i]->GetScrollBar();
			if ( pScrollBar && ( m_bListScrollBarVisible[i] != pScrollBar->IsVisible() ) )
			{
				m_bListScrollBarVisible[i] = pScrollBar->IsVisible();
				m_pPlayerLists[i]->SetColumnWidthBySection( 0, "name", m_iNameWidth + m_iExtraSpace - ( m_bListScrollBarVisible[i] ? pScrollBar->GetWide() : 0 ) );
			}
		}

		if ( m_pPlayerListsVIP[i] )
		{
			pScrollBar = m_pPlayerListsVIP[i]->GetScrollBar();
			if ( pScrollBar )
			{
				m_bListScrollBarVisibleVIP = pScrollBar->IsVisible();
				m_pPlayerListsVIP[i]->SetColumnWidthBySection(0, "name", m_iNameWidth + m_iExtraSpace - ( m_bListScrollBarVisibleVIP ? pScrollBar->GetWide() : 0 ));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerList( void )
{
	int iSelectedPlayerIndex = GetLocalPlayerIndex();

	// Save off which player we had selected.
	SectionedListPanel *pList = GetSelectedPlayerList();
	if ( pList )
	{
		int itemID = pList->GetSelectedItem();
		if ( itemID >= 0 )
		{
			KeyValues *pInfo = pList->GetItemData( itemID );
			if ( pInfo )
			{
				iSelectedPlayerIndex = pInfo->GetInt( "playerIndex" );
			}
		}
	}

	int i;
	for ( i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		m_pPlayerLists[i]->ClearSelection();
		m_pPlayerLists[i]->RemoveAll();

		if ( m_pPlayerListsVIP[i] )
		{
			m_pPlayerListsVIP[i]->ClearSelection();
			m_pPlayerListsVIP[i]->RemoveAll();
		}
	}

	if ( !g_TF_PR )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	C_TFGameRules *pTFGameRules = TFGameRules();
	bool bIsVIPMode = ( pTFGameRules && pTFGameRules->IsVIPMode() );

	int iLocalTeam = pLocalPlayer->GetTeamNumber();

	bool bMadeSelection = false;

	// Hopefully having all of these defined before the for loop will make
	// things that tinsy bit faster with an increase in player quantity.
	int iTeam, iClass, iRank, iMedal, iDesiredClass, iDominations, iPingIndex, iItemID;
	bool bAlive, bIsVIP;
	KeyValues *pKeyValues;
	SectionedListPanel *pPlayerList;
	const char *szPlayerName;
	bool bAltClassIcons = tf_scoreboard_alt_class_icons.GetBool();
	bool bShowPingAsText = tf_scoreboard_ping_as_text.GetBool();
	Color clr;
	for ( i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TF_PR->IsConnected( i ) )
			continue;

		iTeam = g_PR->GetTeam( i );
		if ( iTeam < FIRST_GAME_TEAM )
			continue;

		// Common variables.
		iClass = g_TF_PR->GetPlayerClass( i );
		bAlive = g_TF_PR->IsAlive( i );
		bIsVIP = ( bIsVIPMode && iClass == TF_CLASS_CIVILIAN ); // TODO: Replace 'iClass == TF_CLASS_CIVILIAN' with something more accurate.

		pKeyValues = new KeyValues( "data" );
		pPlayerList = m_pPlayerLists[iTeam];
		if ( bIsVIP )
		{
			pKeyValues->SetBool( "vip", true );
			pPlayerList = m_pPlayerListsVIP[iTeam];
		}

		if ( !pPlayerList )
			continue;

		szPlayerName = g_TF_PR->GetPlayerName( i );
		if ( tf2c_streamer_mode.GetBool() )
		{
			if ( GetLocalPlayerTeam() >= FIRST_GAME_TEAM && iTeam != GetLocalPlayerTeam() )
			{
				szPlayerName = g_pVGuiLocalize->FindAsUTF8( g_aPlayerNamesGeneric[tf2c_streamer_mode.GetInt() > 1 ? iTeam : 0] );
			}

			pKeyValues->SetInt( "medal", 0 );
		}
		else
		{
			iRank = g_TF_PR->GetPlayerRank( i );

			iMedal = 0;
			if ( iRank > TF_RANK_NONE && iRank <= TF_RANK_COUNT )
			{
				if ( iRank & TF_RANK_DEVELOPER )
				{
					iMedal = m_iImageMedals[iTeam][2];
				}
				else if ( iRank & TF_RANK_CONTRIBUTOR )
				{
					iMedal = m_iImageMedals[iTeam][1];
				}
				else if ( iRank & TF_RANK_PLAYTESTER )
				{
					iMedal = m_iImageMedals[iTeam][0];
				}
			}

			pKeyValues->SetInt( "medal", iMedal );
		}

		pKeyValues->SetInt( "playerIndex", i );
		pKeyValues->SetString( "name", szPlayerName );
		pKeyValues->SetInt( "score", g_TF_PR->GetTotalScore( i ) );

		// Can only see class information if we're on the same team, or if they're the Civilian.
		if ( bIsVIP || ( iLocalTeam == TEAM_SPECTATOR || iTeam == iLocalTeam ) )
		{
			// If this is local player and he is dead, show desired class (which he will spawn as) rather than current class.
			if ( g_PR->IsLocalPlayer( i ) && !bAlive )
			{
				// use desired class unless it's random -- if random, his future class is not decided until moment of spawn
				iDesiredClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
				if ( iDesiredClass != TF_CLASS_RANDOM )
				{
					iClass = iDesiredClass;
				}
			}

			//pKeyValues->SetString( "class", g_aPlayerClassNames[iClass] );

			if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_COUNT_ALL )
			{
				if ( bAltClassIcons )
				{
					// Use an alternate set of class icons.
					pKeyValues->SetInt( "class", bAlive ? m_iImageClassesAlt[iClass] : m_iImageClassesAlt[iClass + ( TF_CLASS_COUNT_ALL - 1 )] );
				}
				else
				{
					// Use the default set of class icons.
					pKeyValues->SetInt( "class", bAlive ? m_iImageClasses[iClass] : m_iImageClasses[iClass + ( TF_CLASS_COUNT_ALL - 1 )] );
				}
			}
		}

		// display whether player is alive or dead (all players see this for all other players on both teams)
		pKeyValues->SetInt( "status", bAlive ? 0 : m_iImageDead );

		if( tf2c_show_nemesis_relationships.GetBool() )
		{
			if ( g_TF_PR->IsPlayerDominating( i ) )
			{
				// if local player is dominated by this player, show a nemesis icon
				pKeyValues->SetInt( "nemesis", m_iImageNemesis );
				//pKeyValues->SetString( "class", "#TF_Nemesis" );
			}
			else if ( g_TF_PR->IsPlayerDominated( i ) )
			{
				// if this player is dominated by the local player, show the domination icon
				pKeyValues->SetInt( "nemesis", m_iImageDominated );
				//pKeyValues->SetString( "class", "#TF_Dominated" );
			}
		}

		// Show number of dominations.
		iDominations = Min( g_TF_PR->GetNumberOfDominations( i ), TF_SCOREBOARD_MAX_DOMINATIONS );
		pKeyValues->SetInt( "domination", iDominations > 0 ? m_iImageDominations[iDominations - 1] : 0 );

		// Show ping.
		if ( g_PR->IsFakePlayer( i ) )
		{
			if ( bShowPingAsText )
			{
				pKeyValues->SetString( "ping", "#TF_Scoreboard_Bot" );
			}
			else
			{
				pKeyValues->SetInt( "ping", bAlive ? m_iImagePings[PING_BOT_RED + GetTeamSkin( iTeam )] : m_iImagePings[( PING_BOT_RED + GetTeamSkin( iTeam ) ) + TF_SCOREBOARD_MAX_PING_ICONS] );
			}
		}
		else
		{
			int iPing = g_PR->GetPing( i );
			if ( iPing < 1 )
			{
				if ( bShowPingAsText )
				{
					pKeyValues->SetString( "ping", "" );
				}
				else
				{
					pKeyValues->SetInt( "ping", 0 );
				}
			}
			else
			{
				if ( bShowPingAsText )
				{
					pKeyValues->SetInt( "ping", iPing );
				}
				else
				{
					if ( iPing < 125 )
					{
						iPingIndex = PING_LOW;
					}
					else if ( iPing < 200 )
					{
						iPingIndex = PING_MED;
					}
					else if ( iPing < 275 )
					{
						iPingIndex = PING_HIGH;
					}
					else
					{
						iPingIndex = PING_VERY_HIGH;
					}
						
					pKeyValues->SetInt( "ping", m_iImagePings[iPingIndex] );
				}
			}
		}

		UpdatePlayerAvatar( i, pKeyValues );

		iItemID = pPlayerList->AddItem( 0, pKeyValues );
		clr = g_PR->GetTeamColor( g_PR->GetTeam( i ) );
		if ( !bAlive )
		{
			clr.SetColor( clr.r(), clr.g(), clr.b(), clr.a() / 2 );
		}

		pPlayerList->SetItemFgColor( iItemID, clr );
		if ( iSelectedPlayerIndex == i )
		{
			bMadeSelection = true;
			pPlayerList->SetSelectedItem( iItemID );
		}
		/*else
		{
			pPlayerList->SetItemBgColor( iItemID, Color( 0, 0, 0, 80 ) );
		}*/

		pKeyValues->deleteThis();
	}

	// If we're on spectator, find a default selection
#ifdef _X360
	if ( !bMadeSelection )
	{
		for ( i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
		{
			SectionedListPanel *pList = m_pPlayerLists[i];
			if ( pList->GetItemCount() > 0 )
			{
				pList->SetSelectedItem( 0 );
				break;
			}
		}
	}
#endif

	for ( i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		m_pPlayerLists[i]->InvalidateLayout( true );

		if ( m_pPlayerListsVIP[i] )
		{
			m_pPlayerListsVIP[i]->InvalidateLayout( true );
			m_pPlayerListsVIP[i]->SetVisible( bIsVIPMode && m_iTeamCount != TF_TEAM_COUNT );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateSpectatorList( void )
{
	if ( !g_TF_PR )
		return;

	// Spectators
	char szSpectatorList[512] = "";
	int nSpectators = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TF_PR->IsConnected( i ) )
			continue;

		if ( TFGameRules()->IsInArenaQueueMode() && !g_TF_PR->IsArenaSpectator( i ) )
			continue;

		if ( g_TF_PR->GetTeam( i ) == TEAM_SPECTATOR )
		{
			if ( nSpectators > 0 )
			{
				V_strcat_safe( szSpectatorList, ", " );
			}

			V_strcat_safe( szSpectatorList, g_TF_PR->GetPlayerName( i ) );
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#ScoreBoard_Spectator" : "#ScoreBoard_Spectators" );

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		V_swprintf_safe( wzSpectatorCount, L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString( wzSpectators, sizeof( wzSpectators ), g_pVGuiLocalize->Find( pchFormat ), 2, wzSpectatorCount, wzSpectatorList );
	}
	SetDialogVariable( "spectators", wzSpectators );
}


void CTFClientScoreBoardDialog::UpdateArenaWaitingToPlayList( void )
{
	if ( !g_TF_PR || !TFGameRules()->IsInArenaQueueMode() )
	{
		SetDialogVariable( "waitingtoplay", "" );
		return;
	}

	// Spectators
	char szSpectatorList[512] = "";
	int nSpectators = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TF_PR->IsConnected( i ) )
			continue;

		if ( g_TF_PR->GetTeam( i ) == TEAM_SPECTATOR && !g_TF_PR->IsArenaSpectator( i ) )
		{
			if ( nSpectators > 0 )
			{
				V_strcat_safe( szSpectatorList, ", " );
			}

			V_strcat_safe( szSpectatorList, g_TF_PR->GetPlayerName( i ) );
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#TF_Arena_ScoreBoard_Spectator" : "#TF_Arena_ScoreBoard_Spectators" );

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		V_swprintf_safe( wzSpectatorCount, L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString( wzSpectators, sizeof( wzSpectators ), g_pVGuiLocalize->Find( pchFormat ), 2, wzSpectatorCount, wzSpectatorList );
	}
	SetDialogVariable( "waitingtoplay", wzSpectators );
}

//-----------------------------------------------------------------------------
// Purpose: Updates details about a player
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerDetails( void )
{
	if ( !g_TF_PR )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	int playerIndex = pLocalPlayer->entindex();

	// Make sure the selected player is still connected. 
	if ( !g_TF_PR->IsConnected( playerIndex ) )
		return;

	if ( engine->IsHLTV() )
	{
		SetDialogVariable( "playername", g_TF_PR->GetPlayerName( playerIndex ) );
		SetDialogVariable( "playerscore", "" );

		m_pLocalPlayerStatsPanel->SetVisible( false );
		m_pClassImage->SetVisible( false );
		m_pClassModelPanel->SetVisible( false );
		return;
	}

	m_pLocalPlayerStatsPanel->SetVisible( true );

	Color cGreen = Color( 0, 255, 0, 255 );
	Color cWhite = Color( 255, 255, 255, 255 );

	int iHasJumppad = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasJumppad, set_teleporter_mode);

	RoundStats_t &roundStats = GetStatPanel()->GetRoundStatsCurrentGame();
	m_pLocalPlayerStatsPanel->SetDialogVariable( "kills", roundStats.m_iStat[TFSTAT_KILLS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_KILLS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_KILLS]->SetFgColor( roundStats.m_iStat[TFSTAT_KILLS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "deaths", roundStats.m_iStat[TFSTAT_DEATHS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_DEATHS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_DEATHS]->SetFgColor( roundStats.m_iStat[TFSTAT_DEATHS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "assists", roundStats.m_iStat[TFSTAT_KILLASSISTS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_KILLASSISTS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_KILLASSISTS]->SetFgColor( roundStats.m_iStat[TFSTAT_KILLASSISTS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "destruction", roundStats.m_iStat[TFSTAT_BUILDINGSDESTROYED] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_BUILDINGSDESTROYED] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_BUILDINGSDESTROYED]->SetFgColor( roundStats.m_iStat[TFSTAT_BUILDINGSDESTROYED] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "captures", roundStats.m_iStat[TFSTAT_CAPTURES] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_CAPTURES] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_CAPTURES]->SetFgColor( roundStats.m_iStat[TFSTAT_CAPTURES] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "defenses", roundStats.m_iStat[TFSTAT_DEFENSES] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_DEFENSES] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_DEFENSES]->SetFgColor( roundStats.m_iStat[TFSTAT_DEFENSES] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "dominations", roundStats.m_iStat[TFSTAT_DOMINATIONS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_DOMINATIONS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_DOMINATIONS]->SetFgColor( roundStats.m_iStat[TFSTAT_DOMINATIONS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "revenge", roundStats.m_iStat[TFSTAT_REVENGE] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_REVENGE] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_REVENGE]->SetFgColor( roundStats.m_iStat[TFSTAT_REVENGE] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "healing", roundStats.m_iStat[TFSTAT_HEALING] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_HEALING] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_HEALING]->SetFgColor( roundStats.m_iStat[TFSTAT_HEALING] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "invulns", roundStats.m_iStat[TFSTAT_INVULNS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_INVULNS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_INVULNS]->SetFgColor( roundStats.m_iStat[TFSTAT_INVULNS] ? cGreen : cWhite );
	}

	if (iHasJumppad)
	{
		m_pLocalPlayerStatsPanel->SetDialogVariable("jumppadjumps", roundStats.m_iStat[TFSTAT_JUMPPAD_JUMPS]);
		if (m_pLocalPlayerStatLabels[TFSTAT_JUMPPAD_JUMPS])
		{
			m_pLocalPlayerStatLabels[TFSTAT_JUMPPAD_JUMPS]->SetVisible(true);
			m_pLocalPlayerStatLabelsLabels[TFSTAT_JUMPPAD_JUMPS]->SetVisible(true);
			m_pLocalPlayerStatLabels[TFSTAT_JUMPPAD_JUMPS]->SetFgColor(roundStats.m_iStat[TFSTAT_JUMPPAD_JUMPS] ? cGreen : cWhite);
		}
		if (m_pLocalPlayerStatLabels[TFSTAT_TELEPORTS])
		{
			m_pLocalPlayerStatLabels[TFSTAT_TELEPORTS]->SetVisible(false);
			m_pLocalPlayerStatLabelsLabels[TFSTAT_TELEPORTS]->SetVisible(false);
		}
	}
	else
	{
		m_pLocalPlayerStatsPanel->SetDialogVariable("teleports", roundStats.m_iStat[TFSTAT_TELEPORTS]);
		if (m_pLocalPlayerStatLabels[TFSTAT_TELEPORTS])
		{
			m_pLocalPlayerStatLabels[TFSTAT_TELEPORTS]->SetVisible(true);
			m_pLocalPlayerStatLabelsLabels[TFSTAT_TELEPORTS]->SetVisible(true);
			m_pLocalPlayerStatLabels[TFSTAT_TELEPORTS]->SetFgColor(roundStats.m_iStat[TFSTAT_TELEPORTS] ? cGreen : cWhite);
		}
		if (m_pLocalPlayerStatLabels[TFSTAT_JUMPPAD_JUMPS])
		{
			m_pLocalPlayerStatLabels[TFSTAT_JUMPPAD_JUMPS]->SetVisible(false);
			m_pLocalPlayerStatLabelsLabels[TFSTAT_JUMPPAD_JUMPS]->SetVisible(false);
		}
	}

	m_pLocalPlayerStatsPanel->SetDialogVariable( "headshots", roundStats.m_iStat[TFSTAT_HEADSHOTS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_HEADSHOTS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_HEADSHOTS]->SetFgColor( roundStats.m_iStat[TFSTAT_HEADSHOTS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "backstabs", roundStats.m_iStat[TFSTAT_BACKSTABS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_BACKSTABS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_BACKSTABS]->SetFgColor( roundStats.m_iStat[TFSTAT_BACKSTABS] ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "bonus", roundStats.m_iStat[TFSTAT_BONUS_POINTS] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_BONUS_POINTS] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_BONUS_POINTS]->SetFgColor( roundStats.m_iStat[TFSTAT_BONUS_POINTS] ? cGreen : cWhite );
	}
	int nSupport = TFGameRules() ? TFGameRules()->CalcPlayerSupportScore( NULL, playerIndex ) : 0;
	m_pLocalPlayerStatsPanel->SetDialogVariable( "support", nSupport );
	if ( m_pLocalPlayerStatLabels[TFSTAT_DAMAGE_ASSIST] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_DAMAGE_ASSIST]->SetFgColor( nSupport ? cGreen : cWhite );
	}
	m_pLocalPlayerStatsPanel->SetDialogVariable( "damage", roundStats.m_iStat[TFSTAT_DAMAGE] );
	if ( m_pLocalPlayerStatLabels[TFSTAT_DAMAGE] )
	{
		m_pLocalPlayerStatLabels[TFSTAT_DAMAGE]->SetFgColor( roundStats.m_iStat[TFSTAT_DAMAGE] ? cGreen : cWhite );
	}

	SetDialogVariable( "playername", g_TF_PR->GetPlayerName( playerIndex ) );
	SetDialogVariable( "playerscore", GetPointsString( g_TF_PR->GetTotalScore( playerIndex ) ) );

	Color clr = g_PR->GetTeamColor( g_PR->GetTeam( playerIndex ) );
	m_pLabelPlayerName->SetFgColor( clr );
	m_pImagePanelHorizLine->SetFillColor( clr );

	int iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	int iTeam = pLocalPlayer->GetTeamNumber();
	if ( iTeam >= FIRST_GAME_TEAM && iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT_ALL )
	{
		if ( cl_hud_playerclass_use_playermodel.GetBool() )
		{
			m_pClassImage->SetVisible( false );
			m_pClassModelPanel->SetVisible( true );

			m_pClassModelPanel->SetToPlayerClass( iClass );
			m_pClassModelPanel->SetTeam( iTeam );
			m_pClassModelPanel->LoadItems();
		}
		else
		{
			m_pClassImage->SetVisible( true );
			m_pClassModelPanel->SetVisible( false );

			m_pClassImage->SetClass( iTeam, iClass, 0 );
		}
	}
	else
	{
		m_pClassImage->SetVisible( false );
		m_pClassModelPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData( itemID1 );
	KeyValues *it2 = list->GetItemData( itemID2 );
	Assert( it1 && it2 );

	// Check if they're the VIP...
	if ( !it1->GetBool( "vip", false ) && it2->GetBool( "vip", false ) )
		return true;

	// If not, compare their scores...
	int v1 = it1->GetInt( "score" );
	int v2 = it2->GetInt( "score" );
	if ( v1 > v2 )
		return true;
	else if ( v1 < v2 )
		return false;

	// If score is the same, use player index to get deterministic sort.
	return ( it1->GetInt( "playerIndex" ) < it2->GetInt( "playerIndex" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a localized string of form "1 point", "2 points", etc for specified # of points
//-----------------------------------------------------------------------------
const wchar_t *GetPointsString( int iPoints )
{
	wchar_t wzScoreVal[128];
	static wchar_t wzScore[128];
	V_swprintf_safe( wzScoreVal, L"%i", iPoints );
	if ( 1 == iPoints )
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof( wzScore ), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Point" ), 1, wzScoreVal );
	}
	else
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof( wzScore ), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Points" ), 1, wzScoreVal );
	}

	return wzScore;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();
	if ( !V_stricmp( type, "server_spawn" ) )
	{
		const char *szHostName = event->GetString( "hostname" );
		wchar_t wzHostName[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( szHostName, wzHostName, sizeof( wzHostName ) );
		g_pVGuiLocalize->ConstructString( m_wzServerLabel, sizeof( m_wzServerLabel ), g_pVGuiLocalize->Find( "#Scoreboard_Server" ), 1, wzHostName );

		OnStreamerModeChanged( tf2c_streamer_mode.GetBool() );

		// Set the level name after the server spawn
		SetDialogVariable( "mapname", GetMapDisplayName( event->GetString( "mapname" ) ) );
	}
	else if ( !V_stricmp( type, "game_maploaded" ) )
	{
		InvalidateLayout( false, true );
	}

	if ( IsVisible() )
	{
		Update();
	}
}


SectionedListPanel *CTFClientScoreBoardDialog::GetSelectedPlayerList( void )
{
	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		SectionedListPanel *pList = m_pPlayerLists[i];
		if ( pList->GetSelectedItem() >= 0 )
			return pList;


		pList = m_pPlayerListsVIP[i];
		if ( pList && pList->GetSelectedItem() >= 0 )
			return pList;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen.  (vgui has this method on
//			Frame, but we're an EditablePanel, need to roll our own.)
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds( wx, wy, ww, wt );
	SetPos( ( ww - GetWide() ) / 2, ( wt - GetTall() ) / 2 );
}
