#include "cbase.h"
#include "tf_mainmenupanel.h"
#include "ienginevgui.h"
#include "controls/tf_advbutton.h"
#include "controls/tf_advslider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ImagePanel.h"
#include "tf_notificationmanager.h"
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "vgui_avatarimage.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/QueryBox.h>
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define BLOG_URL "https://tf2classic.com/?game=1"

static void OnBlogToggle( IConVar *var, const char *pOldValue, float flOldValue )
{
	GET_MAINMENUPANEL( CTFMainMenuPanel )->ShowBlogPanel( ( (ConVar *)var )->GetBool() );
}
ConVar tf2c_mainmenu_music( "tf2c_mainmenu_music", "1", FCVAR_ARCHIVE, "Toggles the music on the main menu" );
ConVar tf2c_mainmenu_showblog( "tf2c_mainmenu_showblog", "0", FCVAR_ARCHIVE, "Toggles the blog on the main menu", OnBlogToggle );
ConVar tf2c_mainmenu_showfriendslist( "tf2c_mainmenu_showfriendslist", "0", FCVAR_ARCHIVE, "Toggles the friends list on the main menu" );
ConVar tf2c_mainmenu_showserverlist( "tf2c_mainmenu_showserverlist", "1", FCVAR_ARCHIVE, "Toggles the server list on the main menu" );

ConVar tf2c_mainmenu_friendslist_refreshrate( "tf2c_mainmenu_friendslist_refreshrate", "10.0", FCVAR_ARCHIVE, "How fast the friends list automatically refreshes when it's visible" );

ConVar tf2c_createserver_show_public_ip( "tf2c_createserver_show_public_ip", "0", FCVAR_ARCHIVE, "Shows the player's public IP in the Create Server dialog box" );
ConVar tf2c_createserver_has_prompted_for_public_ip( "tf2c_createserver_has_prompted_for_public_ip", "0", FCVAR_HIDDEN | FCVAR_ARCHIVE, "Whether the player has been prompted about wanting to show his IP or not" );

// ConVar tf2c_mainmenu_has_declined_supported_dxlevel( "tf2c_mainmenu_has_declined_supported_dxlevel", "0", FCVAR_HIDDEN | FCVAR_ARCHIVE, "Whether the player has been prompted about using an unsupported dxlevel" );

extern ConVar tf2c_streamer_mode;

static void SetNextFriendsListRefreshTime( float flNextRefresh )
{
	GET_MAINMENUPANEL( CTFMainMenuPanel )->SetNextFriendsListRefreshTime( flNextRefresh );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::CTFMainMenuPanel( Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pVersionLabel = new CExLabel( this, "VersionLabel", "" );
	m_pNotificationButton = new CTFButton( this, "NotificationButton", "" );
	m_pProfileAvatar = new CAvatarImagePanel( this, "AvatarImage" );
	m_pFakeBGImage = new ImagePanel( this, "FakeBGImage" );

	m_pBlogPanel = new CTFBlogPanel( this, "BlogPanel" );
	m_pServerlistPanel = new CTFServerlistPanel( this, "ServerlistPanel" );
	m_pFriendlistPanel = new CTFFriendlistPanel( this, "FriendlistPanel" );

	m_pGCUserCountLabel = NULL;

	m_psMusicStatus = MUSIC_FIND;
	m_nSongGuid = 0;

	m_iShowFakeIntro = 4;

	m_bInLevel = false;

	m_flNextFriendsListRefresh = 0.0f;

	if ( steamapicontext->SteamUser() )
	{
		m_SteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::~CTFMainMenuPanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}


void CTFMainMenuPanel::ApplySchemeSettings( IScheme *pScheme )
{
	bool bGCUserCountLabelVisible = false;
	if ( m_pGCUserCountLabel )
	{
		bGCUserCountLabelVisible = m_pGCUserCountLabel->IsVisible();
	}

	BaseClass::ApplySchemeSettings( pScheme );

	m_pszNickname = steamapicontext->SteamFriends() ? steamapicontext->SteamFriends()->GetPersonaName() : "Unknown";

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( V_strlen( m_pszNickname ) > m_iLongName )
	{
		AddSubKeyNamed( pConditions, "if_longname" );
	}

	if ( guiroot->IsInLevel() )
	{
		AddSubKeyNamed( pConditions, "if_inlevel" );
	}
	else
	{
		AddSubKeyNamed( pConditions, "if_inmenu" );
	}

	LoadControlSettings( "resource/UI/main_menu/MainMenuPanel.res", NULL, NULL, pConditions );

	pConditions->deleteThis();

	SetVersionLabel();

	m_pGCUserCountLabel = dynamic_cast<CExLabel *>( FindChildByName( "GCUserCountLabel" ) );
	if ( m_pGCUserCountLabel )
	{
		m_pGCUserCountLabel->SetVisible( bGCUserCountLabelVisible );
	}
}


void CTFMainMenuPanel::PerformLayout()
{
	if ( m_pProfileAvatar )
	{
		m_pProfileAvatar->SetPlayer( m_SteamID, k_EAvatarSize64x64 );
		m_pProfileAvatar->SetShouldDrawFriendIcon( false );
	}

	SetDialogVariable( "nickname", m_pszNickname );

	ShowBlogPanel( tf2c_mainmenu_showblog.GetBool() || GetNotificationManager()->IsOutdated() );
	OnNotificationUpdate();

	// Menu fade-in animation. TODO Breaks -tools, also breaks with map_background. Make optional?
	if ( m_iShowFakeIntro > 0 )
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		char szImage[128];
		V_sprintf_safe( szImage, "../console/%s", szBGName );
		int width, height;
		surface()->GetScreenSize( width, height );
		float fRatio = (float)width / (float)height;
		bool bWidescreen = ( fRatio < 1.5 ? false : true );
		if ( bWidescreen )
			V_strcat_safe( szImage, "_widescreen" );
		m_pFakeBGImage->SetImage( szImage );
		m_pFakeBGImage->SetVisible( true );
		m_pFakeBGImage->SetAlpha( 255 );
	}

/*
	// Warn if DirectX level is too outdated for some TF2C shaders.
	ConVarRef mat_dxlevel( "mat_dxlevel" );
	if ( mat_dxlevel.GetInt() > 50 && mat_dxlevel.GetInt() < 90 )
	{
		if ( !tf2c_mainmenu_has_declined_supported_dxlevel.GetBool() )
		{
			QueryBox *pUnsupportedDXLevelQuery = new QueryBox( "#TF_UnsupportedDXLevel_Title", "#TF_UnsupportedDXLevel_Message", guiroot->GetParent() );
			if ( pUnsupportedDXLevelQuery )
			{
				pUnsupportedDXLevelQuery->AddActionSignalTarget( this );

				pUnsupportedDXLevelQuery->SetOKButtonText( "#GameUI_Yes" );
				pUnsupportedDXLevelQuery->SetOKCommand( new KeyValues( "OnPromptAccepted" ) );

				pUnsupportedDXLevelQuery->SetCancelButtonText( "#GameUI_No" );
				pUnsupportedDXLevelQuery->SetCancelCommand( new KeyValues( "OnPromptDeclined" ) );

				pUnsupportedDXLevelQuery->DoModal();
			}
		}
	}
	else
	{
		tf2c_mainmenu_has_declined_supported_dxlevel.SetValue( false );
	}
*/
	/*
	// Ask user consent for error reporting.
	ConVarRef tf2c_send_error_reports( "tf2c_send_error_reports" );
	if ( tf2c_send_error_reports.GetInt() < 0 )
	{
		QueryBox *pSentryReportQuery = new QueryBox( "#TF_SentryReport_Title", "#TF_SentryReport_Message", guiroot->GetParent() );
		if ( pSentryReportQuery )
		{
			pSentryReportQuery->AddActionSignalTarget( this );

			pSentryReportQuery->SetOKButtonText( "#GameUI_Yes" );
			pSentryReportQuery->SetOKCommand( new KeyValues( "OnSentryAccepted" ) );

			pSentryReportQuery->SetCancelButtonText( "#GameUI_No" );
			pSentryReportQuery->SetCancelCommand( new KeyValues( "OnSentryDeclined" ) );

			pSentryReportQuery->DoModal();
		}
	}
	*/
}


void CTFMainMenuPanel::ShowBlogPanel( bool show )
{
	if ( m_pBlogPanel )
	{
		m_pBlogPanel->SetVisible( show );
		if ( show )
		{
			m_pBlogPanel->LoadBlogPost( BLOG_URL );
		}
	}
}


void CTFMainMenuPanel::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "newquit" ) )
	{
		guiroot->ShowPanel( QUIT_MENU );
	}
	else if ( !V_stricmp( command, "newoptionsdialog" ) )
	{
		guiroot->ShowPanel( OPTIONSDIALOG_MENU );
	}
	else if ( !V_stricmp( command, "newloadout" ) )
	{
		guiroot->ShowPanel( LOADOUT_MENU );
	}
	else if ( !V_stricmp( command, "newcreateserver" ) )
	{
		bool bStreamerMode = tf2c_streamer_mode.GetBool();
		if ( bStreamerMode || tf2c_createserver_has_prompted_for_public_ip.GetBool() )
		{
			// Reset this value when Streamer Mode is turned off just incase.
			if ( bStreamerMode )
			{
				tf2c_createserver_has_prompted_for_public_ip.SetValue( false );
			}

			guiroot->ShowPanel( CREATESERVER_MENU );
		}
		else
		{
			// Ask the user if they want their IP to be shown.
			QueryBox *pShowPublicIPQuery = new QueryBox( "#TF_ConfirmShowIP_Title", "#TF_ConfirmShowIP_Message", guiroot->GetParent() );
			if ( pShowPublicIPQuery )
			{
				pShowPublicIPQuery->AddActionSignalTarget( this );

				pShowPublicIPQuery->SetOKCommand( new KeyValues( "OnWarningAccepted" ) );
				pShowPublicIPQuery->SetCancelCommand( new KeyValues( "OnWarningDeclined" ) );

				pShowPublicIPQuery->SetOKButtonText( "#TF_ConfirmShowIP_Yes" );
				pShowPublicIPQuery->SetCancelButtonText( "#TF_ConfirmShowIP_No" );

				pShowPublicIPQuery->DoModal();
			}
		}
	}
	else if ( !V_stricmp( command, "newachievement" ) )
	{
		guiroot->ShowPanel( ACHIEVEMENTS_MENU );
	}
	else if ( !V_stricmp( command, "newstats" ) )
	{
		guiroot->ShowPanel( STATSUMMARY_MENU );
	}
	else if ( !V_stricmp( command, "callvote" ) )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide" );
		engine->ClientCmd( command );
	}
	else if ( !V_stricmp( command, "checkversion" ) )
	{
		//guiroot->CheckVersion();
	}
	else if ( !V_stricmp( command, "shownotification" ) )
	{
		if ( m_pNotificationButton )
		{
			m_pNotificationButton->SetGlowing( false );
		}
		guiroot->ShowPanel( NOTIFICATION_MENU );
	}
	else if ( !V_stricmp( command, "testnotification" ) )
	{
		wchar_t resultString[128];
		V_swprintf_safe( resultString, L"test %d", GetNotificationManager()->GetNotificationsCount() );
		MessageNotification Notification( L"Yoyo", resultString, time( NULL ) );
		GetNotificationManager()->SendNotification( Notification );
	}
	else if ( !V_stricmp( command, "randommusic" ) )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
	}
	else if ( V_stristr( command, "gamemenucommand " ) )
	{
		engine->ClientCmd_Unrestricted( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


void CTFMainMenuPanel::OnTick()
{
	bool bInLevel = guiroot->IsInLevel();
	if ( m_bInLevel != bInLevel )
	{
		InvalidateLayout( false, true );
		m_bInLevel = bInLevel;
	}

	if ( tf2c_mainmenu_music.GetBool() && !guiroot->IsInLevel() )
	{
		if ( ( m_psMusicStatus == MUSIC_FIND || m_psMusicStatus == MUSIC_STOP_FIND ) && !enginesound->IsSoundStillPlaying( m_nSongGuid ) )
		{
			m_psMusicStatus = MUSIC_PLAY;
		}
		else if ( m_psMusicStatus == MUSIC_PLAY || m_psMusicStatus == MUSIC_STOP_PLAY )
		{
			enginesound->StopSoundByGuid( m_nSongGuid );

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, "music.gamestartup" );
			m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();

			ConVar *snd_musicvolume = cvar->FindVar( "snd_musicvolume" );
			if ( enginesound->IsSoundStillPlaying( m_nSongGuid ) )
			{
				enginesound->SetVolumeByGuid( m_nSongGuid, snd_musicvolume->GetFloat() );
			}

			m_psMusicStatus = MUSIC_FIND;
		}
	}
	else if ( m_psMusicStatus == MUSIC_FIND )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
		m_psMusicStatus = m_nSongGuid == 0 ? MUSIC_STOP_FIND : MUSIC_STOP_PLAY;
	}

	if ( enginevgui->IsGameUIVisible() )
	{
		m_pServerlistPanel->SetVisible( tf2c_mainmenu_showserverlist.GetBool() );

		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			bool bShowFriends = tf2c_mainmenu_showfriendslist.GetBool();

			m_pFriendlistPanel->SetVisible( bShowFriends );
			if ( bShowFriends && gpGlobals->curtime > m_flNextFriendsListRefresh )
			{
				m_pFriendlistPanel->UpdateFriends();
				SetNextFriendsListRefreshTime( gpGlobals->curtime + tf2c_mainmenu_friendslist_refreshrate.GetFloat() );
			}
		}
	}
	else
	{
		m_pServerlistPanel->SetVisible( false );

		m_pFriendlistPanel->SetVisible( false );
	}
}


void CTFMainMenuPanel::OnThink()
{
	if ( m_iShowFakeIntro > 0 )
	{
		m_iShowFakeIntro--;
		if ( m_iShowFakeIntro == 0 )
		{
			Assert( g_pClientMode->GetViewportAnimationController() );
			if (g_pClientMode->GetViewportAnimationController())
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(m_pFakeBGImage, "Alpha", 0, 0.0f, 2.0f, AnimationController::INTERPOLATOR_ACCEL);
			}
			else
			{
				Warning("No viewport animation controller??\n");
			}
		}
	}

	if ( m_pFakeBGImage->IsVisible() && m_pFakeBGImage->GetAlpha() == 0 )
	{
		m_pFakeBGImage->SetVisible( false );
	}
}


void CTFMainMenuPanel::Show()
{
	BaseClass::Show();

	RequestFocus();
}


void CTFMainMenuPanel::Hide()
{
	BaseClass::Hide();
}


void CTFMainMenuPanel::PlayMusic()
{

}


void CTFMainMenuPanel::OnNotificationUpdate()
{
	if ( m_pNotificationButton )
	{
		if ( GetNotificationManager()->GetNotificationsCount() > 0 )
		{
			m_pNotificationButton->SetVisible( true );
		}
		else
		{
			m_pNotificationButton->SetVisible( false );
		}

		if ( GetNotificationManager()->GetUnreadNotificationsCount() > 0 )
		{
			m_pNotificationButton->SetGlowing( true );
		}
		else
		{
			m_pNotificationButton->SetGlowing( false );
		}
	}

	if ( GetNotificationManager()->IsOutdated() )
	{
		if ( m_pVersionLabel )
		{
			m_pVersionLabel->SetFgColor( Color( 255, 20, 50, 100 ) );
		}
	}
}


void CTFMainMenuPanel::SetVersionLabel()
{
	SetDialogVariable( "version", GetNotificationManager()->GetVersionName() );
}


void CTFMainMenuPanel::SetServerlistSize( int size )
{
	m_pServerlistPanel->SetServerlistSize( size );
}


void CTFMainMenuPanel::UpdateServerInfo()
{
	m_pServerlistPanel->UpdateServerInfo();
}


void CTFMainMenuPanel::OnWarningAccepted( void )
{
	tf2c_createserver_has_prompted_for_public_ip.SetValue( 1 );
	tf2c_createserver_show_public_ip.SetValue( 1 );

	guiroot->ShowPanel( CREATESERVER_MENU );
}


void CTFMainMenuPanel::OnWarningDeclined( void )
{
	tf2c_createserver_has_prompted_for_public_ip.SetValue( 1 );
	tf2c_createserver_show_public_ip.SetValue( 0 );

	guiroot->ShowPanel( CREATESERVER_MENU );
}

/*
void CTFMainMenuPanel::OnPromptAccepted( void )
{
	engine->ClientCmd_Unrestricted( "mat_dxlevel 90\nhost_writeconfig\n" );
}


void CTFMainMenuPanel::OnPromptDeclined( void )
{
	tf2c_mainmenu_has_declined_supported_dxlevel.SetValue( true );
	engine->ClientCmd_Unrestricted( "host_writeconfig\n" );
}
*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMainMenuPanel::OnSentryAccepted( void )
{
	ConVarRef tf2c_send_error_reports( "tf2c_send_error_reports" );
	tf2c_send_error_reports.SetValue( 1 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMainMenuPanel::OnSentryDeclined( void )
{
	ConVarRef tf2c_send_error_reports( "tf2c_send_error_reports" );
	tf2c_send_error_reports.SetValue( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFBlogPanel::CTFBlogPanel( Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	m_pHTMLPanel = new HTML( this, "HTMLPanel" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFBlogPanel::~CTFBlogPanel()
{
}


void CTFBlogPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/BlogPanel.res" );
}


void CTFBlogPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	LoadBlogPost( BLOG_URL );
}


void CTFBlogPanel::LoadBlogPost( const char* URL )
{
	if ( m_pHTMLPanel )
	{
		m_pHTMLPanel->SetVisible( true );
		m_pHTMLPanel->OpenURL( URL, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFServerlistPanel::CTFServerlistPanel( Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	m_iSize = 0;
	m_pServerList = new SectionedListPanel( this, "ServerList" );
	m_pConnectButton = new CTFButton( this, "ConnectButton", "#TF_ServerList_Connect" );
	m_pLoadingSpinner = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFServerlistPanel::~CTFServerlistPanel()
{
}


void CTFServerlistPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/ServerlistPanel.res" );

	CExLabel *pServerLabel = dynamic_cast<CExLabel *>( FindChildByName( "ServerLabel" ) );
	if ( pServerLabel )
	{
#ifdef STAGING_ONLY
		pServerLabel->SetText( "#TF_ServerList_Title_Beta" );
#else
		pServerLabel->SetText( "#TF_ServerList_Title" );
#endif
	}

	m_pLoadingSpinner = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "LoadingSpinner" ) );

	m_pServerList->RemoveAll();
	m_pServerList->RemoveAllSections();
	m_pServerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pServerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pServerList->SetBorder( NULL );
	m_pServerList->AddSection( 0, "Servers", ServerSortFunc );
	m_pServerList->AddColumnToSection( 0, "Ping", "#TF_ServerList_Ping", SectionedListPanel::COLUMN_BRIGHT, m_iPingWidth );
	m_pServerList->AddColumnToSection( 0, "Name", "#TF_ServerList_Servers", SectionedListPanel::COLUMN_BRIGHT, m_iServerWidth );
	m_pServerList->AddColumnToSection( 0, "Players", "#TF_ServerList_Players", SectionedListPanel::COLUMN_BRIGHT, m_iPlayersWidth );
	m_pServerList->AddColumnToSection( 0, "Map", "#TF_ServerList_Map", SectionedListPanel::COLUMN_BRIGHT, m_iMapWidth );
	m_pServerList->SetSectionAlwaysVisible( 0, true );

	m_pConnectButton->SetVisible( false );
	UpdateServerInfo();
}


void CTFServerlistPanel::OnThink()
{
	if ( m_pLoadingSpinner )
	{
		m_pLoadingSpinner->SetVisible( GetNotificationManager()->GatheringServerList() );
		if ( m_pLoadingSpinner->IsVisible() )
		{
			SetVisible( true );
		}
	}

	m_pServerList->ClearSelection();
	m_pConnectButton->SetVisible( false );

	if ( !IsCursorOver() )
		return;

	for ( int i = 0; i < m_pServerList->GetItemCount(); i++ )
	{
		int _x, _y;
		m_pServerList->GetPos( _x, _y );
		int x, y, wide, tall;
		m_pServerList->GetItemBounds( i, x, y, wide, tall );
		int cx, cy;
		surface()->SurfaceGetCursorPos( cx, cy );
		m_pServerList->ScreenToLocal( cx, cy );

		if ( cx > x && cx < x + wide && cy > y && cy < y + tall )
		{
			m_pServerList->SetSelectedItem( i );
			int by = y + _y;
			m_pConnectButton->SetPos( m_iServerWidth + m_iPlayersWidth + m_iPingWidth, by );
			m_pConnectButton->SetVisible( true );

			char szCommand[128];
			V_sprintf_safe( szCommand, "connect %s", m_pServerList->GetItemData( i )->GetString( "ServerIP", "" ) );
			m_pConnectButton->SetCommand( szCommand );
		}
	}
}


void CTFServerlistPanel::OnCommand( const char* command )
{
	if ( V_stristr( command, "connect " ) )
	{
		engine->ClientCmd_Unrestricted( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting servers
//-----------------------------------------------------------------------------
bool CTFServerlistPanel::ServerSortFunc( SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData( itemID1 );
	KeyValues *it2 = list->GetItemData( itemID2 );
	Assert( it1 && it2 );

	int v1 = it1->GetInt( "CurPlayers" );
	int v2 = it2->GetInt( "CurPlayers" );
	if ( v1 > v2 )
		return true;
	else if ( v1 < v2 )
		return false;

	/*
	int iOff1 = it1->GetBool("Official");
	int iOff2 = it2->GetBool("Official");
	if (iOff1 && !iOff2)
	return true;
	else if (!iOff1 && iOff2)
	return false;
	*/

	int iPing1 = it1->GetInt( "Ping" );
	if ( iPing1 == 0 )
		return false;

	int iPing2 = it2->GetInt( "Ping" );
	return ( iPing1 < iPing2 );
}


void CTFServerlistPanel::SetServerlistSize( int size )
{
	m_iSize = size;
}


void CTFServerlistPanel::UpdateServerInfo()
{
	// Can't do a visibility check with this command.
	if ( !tf2c_mainmenu_showserverlist.GetBool() )
		return;

	m_pServerList->RemoveAll();
	HFont Font = GETSCHEME()->GetFont( "FontStoreOriginalPrice", true );

	for ( int i = 0; i < m_iSize; i++ )
	{
		gameserveritem_t *pServer = GetNotificationManager()->GetServerInfo( i );
		if ( pServer->m_steamID.GetAccountID() == 0 )
			continue;

		bool bOfficial = GetNotificationManager()->IsOfficialServer( i );

#ifndef STAGING_ONLY
		if ( !bOfficial )
			continue;
#endif

		char szServerPlayers[16];
		V_sprintf_safe( szServerPlayers, "%i/%i", pServer->m_nPlayers, pServer->m_nMaxPlayers );

		KeyValues *curitem = new KeyValues( "data" );

		curitem->SetString( "Name", pServer->GetName() );
		curitem->SetString( "ServerIP", pServer->m_NetAdr.GetConnectionAddressString() );
		curitem->SetString( "Players", szServerPlayers );
		curitem->SetInt( "CurPlayers", pServer->m_nPlayers );
		curitem->SetInt( "Ping", pServer->m_nPing );
		curitem->SetString( "Map", pServer->m_szMap );
		curitem->SetBool( "Official", bOfficial );		

		int itemID = m_pServerList->AddItem( 0, curitem );

#ifdef STAGING_ONLY
		if ( bOfficial )
			m_pServerList->SetItemFgColor( itemID, GETSCHEME()->GetColor( "TeamYellow", Color( 255, 255, 255, 255 ) ) );
		else
			m_pServerList->SetItemFgColor( itemID, GETSCHEME()->GetColor( "AdvTextDefault", Color( 255, 255, 255, 255 ) ) );
#endif

		m_pServerList->SetItemFont( itemID, Font );
		curitem->deleteThis();
	}

	if ( m_pServerList->GetItemCount() > 0 || GetNotificationManager()->GatheringServerList() )
	{
		SetVisible( true );
	}
	else
	{
		SetVisible( false );
	}

	int min, max;
	m_pServerList->InvalidateLayout( true );
	m_pServerList->GetScrollBar()->GetRange( min, max );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFriendlistPanel::CTFFriendlistPanel( Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
#ifdef UPDATE_CHANGED_FRIENDS
	, m_PersonaChangeCallback( this, &CTFFriendlistPanel::OnPersonaStateChanged )
#endif
{
	m_pFriendsList = new vgui::PanelListPanel( this, "listpanel_friends" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFFriendlistPanel::~CTFFriendlistPanel()
{
	m_pFriendsList->DeleteAllItems();
	delete m_pFriendsList;
}


void CTFFriendlistPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/FriendlistPanel.res" );

	m_pFriendsList->SetFirstColumnWidth( 0 );
	m_pFriendsList->GetScrollbar()->SetWide( XRES( 3 ) );

	UpdateFriends();
}


bool CTFFriendlistPanel::UpdateFriends()
{
	m_pFriendsList->DeleteAllItems();

	// If all of these are true, then bail (Go invisible).
	if ( !steamapicontext || !steamapicontext->SteamUser() || !steamapicontext->SteamFriends() || !IsVisible() ||
		steamapicontext->SteamFriends()->GetFriendPersonaState( steamapicontext->SteamUser()->GetSteamID() ) == k_EPersonaStateOffline ||
		engine->IsDrawingLoadingImage() )
		return Fail();

	int iFriendCount = 0;
	
	// The sorting lists.
	CUtlVector<CTFFriendlistPanelItem *>	pTF2CFriends[TF_FRIENDSTATE_COUNT];
	CUtlVector<CTFFriendlistPanelItem *>	pInGameFriends[TF_FRIENDSTATE_COUNT];
	CUtlVector<CTFFriendlistPanelItem *>	pOnlineFriends[TF_FRIENDSTATE_COUNT];
#ifdef SHOW_OFFLINE_FRIENDS
	CUtlVector<CTFFriendlistPanelItem *>	pOfflineFriends;
#endif

	// Get all of our Steam Friends.
	int i, iCount;
	CSteamID pFriendID;
	EPersonaState pFriendPersonaState;
	FriendListInfo_t *pFriendInfo;
	FriendGameInfo_t *pFriendGame;
	bool bAway;
#ifdef INCLUDE_STEAM_USER
	for ( i = -1, iCount = steamapicontext->SteamFriends()->GetFriendCount( k_EFriendFlagImmediate ); i < iCount; i++ )
	{
		pFriendID = i == -1 ? steamapicontext->SteamUser()->GetSteamID() : steamapicontext->SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
#else
	for ( i = 0, iCount = steamapicontext->SteamFriends()->GetFriendCount( k_EFriendFlagImmediate ); i < iCount; i++ )
	{
		pFriendID = steamapicontext->SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
#endif
		
		pFriendInfo = new FriendListInfo_t;
		pFriendGame = new FriendGameInfo_t;
		pFriendInfo->m_PlayingGame = steamapicontext->SteamFriends()->GetFriendGamePlayed( pFriendID, pFriendGame );

		// Check if they're away.
		pFriendPersonaState = steamapicontext->SteamFriends()->GetFriendPersonaState( pFriendID );
		bAway = ( pFriendPersonaState > k_EPersonaStateOnline && pFriendPersonaState < k_EPersonaStateLookingToTrade );
		pFriendInfo->m_Away = bAway;

		// Check if they're playing a game of some sort.
		if ( pFriendInfo->m_PlayingGame )
		{
			pFriendInfo->m_FriendGame = pFriendGame;

			// Prioritize TF2Classic players.
            CGameID pTF2CModID((int32)engine->GetAppID(), (int32)TF2C_GAME_DIR, CGameID::k_EGameIDTypeGameMod);
			if ( pFriendGame->m_gameID.ModID() == pTF2CModID.ModID() )
			{
				pFriendInfo->m_PlayingTF2C = true;

				pTF2CFriends[bAway ? TF_FRIENDSTATE_AWAY : TF_FRIENDSTATE_ONLINE].AddToTail( new CTFFriendlistPanelItem( m_pFriendsList, "FriendlistPanelItem", pFriendID, pFriendInfo ) );
			}
			else
			{
				pInGameFriends[bAway ? TF_FRIENDSTATE_AWAY : TF_FRIENDSTATE_ONLINE].AddToTail( new CTFFriendlistPanelItem( m_pFriendsList, "FriendlistPanelItem", pFriendID, pFriendInfo ) );
			}
		}
		else if ( pFriendPersonaState == k_EPersonaStateOnline || bAway )
		{
			pOnlineFriends[bAway ? TF_FRIENDSTATE_AWAY : TF_FRIENDSTATE_ONLINE].AddToTail( new CTFFriendlistPanelItem( m_pFriendsList, "FriendlistPanelItem", pFriendID, pFriendInfo ) );
		}
		else
		{
#ifdef SHOW_OFFLINE_FRIENDS
			pFriendInfo->m_Online = false;

			pOfflineFriends.AddToTail( new CTFFriendlistPanelItem( m_pFriendsList, "FriendlistPanelItem", pFriendID, pFriendInfo ) );
#endif

			iFriendCount--;
		}

		iFriendCount++;
	}

	// Order friends in the list from TF2C (In Game) to Offline (Away).
	for ( i = 0, iCount = pTF2CFriends[TF_FRIENDSTATE_ONLINE].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pTF2CFriends[TF_FRIENDSTATE_ONLINE][i] );
	}
	for ( i = 0, iCount = pTF2CFriends[TF_FRIENDSTATE_AWAY].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pTF2CFriends[TF_FRIENDSTATE_AWAY][i] );
	}

	for ( i = 0, iCount = pInGameFriends[TF_FRIENDSTATE_ONLINE].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pInGameFriends[TF_FRIENDSTATE_ONLINE][i] );
	}
	for ( i = 0, iCount = pInGameFriends[TF_FRIENDSTATE_AWAY].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pInGameFriends[TF_FRIENDSTATE_AWAY][i] );
	}

	for ( i = 0, iCount = pOnlineFriends[TF_FRIENDSTATE_ONLINE].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pOnlineFriends[TF_FRIENDSTATE_ONLINE][i] );
	}
	for ( i = 0, iCount = pOnlineFriends[TF_FRIENDSTATE_AWAY].Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pOnlineFriends[TF_FRIENDSTATE_AWAY][i] );
	}

#ifdef SHOW_OFFLINE_FRIENDS
	for ( i = 0, iCount = pOfflineFriends.Count(); i < iCount; i++ )
	{
		m_pFriendsList->AddItem( NULL, pOfflineFriends[i] );
	}
#endif

	if ( iFriendCount == 0 )
		return Fail();

	SetDialogVariable( "friend_count", iFriendCount );

	SetNextFriendsListRefreshTime( gpGlobals->curtime + tf2c_mainmenu_friendslist_refreshrate.GetFloat() );

	return true;
}

#ifdef UPDATE_CHANGED_FRIENDS

void CTFFriendlistPanel::OnPersonaStateChanged( PersonaStateChange_t *pInfo )
{
	// Optimization for later: Simply add/modify rather than recreating the entire friends list when an update happens.
	UpdateFriends();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: creates child panels, passes down name to pick up any settings from res files.
//-----------------------------------------------------------------------------
CTFFriendlistPanelItem::CTFFriendlistPanelItem( vgui::PanelListPanel *parent, const char* name, CSteamID pFriendID, FriendListInfo_t *pFriendInfo ) : BaseClass( parent, name )
{
	m_pFriendInfo = pFriendInfo;

	m_pProfileAvatar = new CAvatarImagePanel( this, "AvatarImage" );
	if ( m_pProfileAvatar )
	{
		m_pProfileAvatar->SetPlayer( pFriendID, k_EAvatarSize64x64 );
		m_pProfileAvatar->SetShouldDrawFriendIcon( false );
	}

	const char *szFriendName = steamapicontext->SteamFriends()->GetFriendPersonaName( pFriendID );
	SetDialogVariable( "friend_name", szFriendName );
	m_iFriendNameLength = V_strlen( szFriendName );
}

//-----------------------------------------------------------------------------
// Purpose: This is kind of frightening to look at.
//-----------------------------------------------------------------------------
void CTFFriendlistPanelItem::ApplySchemeSettings( IScheme *pScheme )
{
	KeyValues *pConditions = new KeyValues( "conditions" );

#ifdef HIDE_ONLINE_STATUS
	bool bHasStatus = false;
#endif
	const char *szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Offline" );

	if ( m_iFriendNameLength > m_iLongName )
	{
		AddSubKeyNamed( pConditions, "if_longname" );
	}

	if ( m_pFriendInfo->m_FriendGame )
	{
		AddSubKeyNamed( pConditions, "if_ingame" );

		if ( m_pFriendInfo->m_Away )
		{
			AddSubKeyNamed( pConditions, "if_idle" );

			if ( m_pFriendInfo->m_PlayingTF2C )
			{
				AddSubKeyNamed( pConditions, "if_idle_intf2c" );

				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Idle_InTF2C" );
			}
			else if ( m_pFriendInfo->m_FriendGame->m_gameID.IsMod() )
			{
				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Idle_InMod" );
			}
			else
			{
				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Idle" );
			}
		}
		else
		{
			if ( m_pFriendInfo->m_PlayingTF2C )
			{
				AddSubKeyNamed( pConditions, "if_intf2c" );

				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_InTF2C" );
			}
			else if ( m_pFriendInfo->m_FriendGame->m_gameID.IsMod() )
			{
				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_InMod" );
			}
			else
			{
				szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_InGame" );
			}
		}

#ifdef HIDE_ONLINE_STATUS
		bHasStatus = true;
#endif
	}
	else if ( m_pFriendInfo->m_Online )
	{
		AddSubKeyNamed( pConditions, "if_online" );

		szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Online" );

		if ( m_pFriendInfo->m_Away )
		{
			AddSubKeyNamed( pConditions, "if_away" );

			szStatusMessage = g_pVGuiLocalize->FindAsUTF8( "#TF_Friends_Status_Away" );
#ifdef HIDE_ONLINE_STATUS
			bHasStatus = true;
#endif
		}
	}
#ifdef SHOW_OFFLINE_FRIENDS
	else
	{
		AddSubKeyNamed( pConditions, "if_offline" );

		
#ifdef HIDE_ONLINE_STATUS
		bHasStatus = true;
#endif
	}
#endif

#ifdef HIDE_ONLINE_STATUS
	if ( bHasStatus )
#endif
	{
		SetDialogVariable( "friend_status", szStatusMessage );
		AddSubKeyNamed( pConditions, "has_status" );
	}
	
	LoadControlSettings( "resource/ui/main_menu/FriendlistItem.res", NULL, NULL, pConditions );

	pConditions->deleteThis();
	
	BaseClass::ApplySchemeSettings( pScheme );
}
