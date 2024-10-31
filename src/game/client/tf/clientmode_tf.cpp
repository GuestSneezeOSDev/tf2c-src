//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_tf.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/ISurface.h"
#include "vgui/IPanel.h"
#include "GameUI/IGameUI.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "buymenu.h"
#include "filesystem.h"
#include "vgui/IVGui.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <KeyValues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "c_tf_player.h"
#include "ienginevgui.h"
#include "in_buttons.h"
#include "voice_status.h"
#include "tf_hud_menu_engy_build.h"
#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_spy_disguise.h"
#include "tf_hud_menu_taunt_selection.h"
#include "tf_clientscoreboard.h"
#include "tf_statsummary.h"
#include "tf_hud_freezepanel.h"
#include "clienteffectprecachesystem.h"
#include "glow_outline_effect.h"
#include "cam_thirdperson.h"
#include "tf_gamerules.h"
#include "tf_hud_chat.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "hud_macros.h"
#include "iviewrender.h"
#include "tf_mainmenu.h"
#include <tier0/icommandline.h>
#include "vgui/panels/tf_loadingscreen.h"
#include "clientsteamcontext.h"
#ifdef TF_CLASSIC_CLIENT
#include "tf/tf_colorblind_helper.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_VIP_TUTORIALS_VIEWED		3

void __MsgFunc_BreakModel( bf_read &msg );
void __MsgFunc_CheapBreakModel( bf_read &msg );
void __MsgFunc_PlayerBonusPoints( bf_read& msg );

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "90", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, MAX_FOV );

void SteamScreenshotsCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( GetClientModeTFNormal() )
	{
		GetClientModeTFNormal()->UpdateSteamScreenshotsHooking();
	}
}
ConVar cl_steamscreenshots( "cl_steamscreenshots", "1", FCVAR_ARCHIVE, "Enable/disable saving screenshots to Steam", SteamScreenshotsCallback );

void HUDMinModeChangedCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ExecuteClientCmd( "hud_reloadscheme" );
}
ConVar cl_hud_minmode( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );
ConVar tf2c_music_cues( "tf2c_music_cues", "1", FCVAR_ARCHIVE, "Enable musical cues triggered by certain in-game events." );
ConVar tf2c_vip_tutorials_viewed( "tf2c_vip_tutorials_viewed", "0", FCVAR_ARCHIVE, "How many times the tutorial annotation has appeared for the VIP mode." );
extern ConVar tf2c_colorblind_pattern_enable;

CUtlVector<IStreamerModeChangedCallback *> pStreamerModeChangeCallbacks;
void AnonymousModeChangedCallBack( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	FOR_EACH_VEC( pStreamerModeChangeCallbacks, i )
	{
		// J.I.T.
		if ( !pStreamerModeChangeCallbacks[i] )
		{
			pStreamerModeChangeCallbacks.FastRemove( i );
			continue;
		}

		// Tell all of our listensers that this variable changed.
		ConVarRef var( pConVar );
		pStreamerModeChangeCallbacks[i]->OnStreamerModeChanged( var.GetBool() );
	}
}
ConVar tf2c_streamer_mode( "tf2c_streamer_mode", "0", FCVAR_ARCHIVE, "Streamer Mode." /*"0 = Disabled, 1 = Anonymous Player Names, 2 = Anonymous Player Names + Disabled Chatting"*/, AnonymousModeChangedCallBack );

void UISteamOverlayCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( GetClientModeTFNormal() )
	{
		GetClientModeTFNormal()->UpdateSteamOverlayPosition();
	}
}
ConVar ui_steam_overlay_notification_position( "ui_steam_overlay_notification_position", "topleft", FCVAR_ARCHIVE, "Steam overlay notification position", UISteamOverlayCallback );

extern ConVar tf_scoreboard_mouse_mode;

IClientMode *g_pClientMode = NULL;
// --------------------------------------------------------------------------------- //
// CTFModeManager.
// --------------------------------------------------------------------------------- //

class CTFModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CTFModeManager g_ModeManager;
IVModeManager *modemanager = (IVModeManager *)&g_ModeManager;

CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffectsGlow )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_x" )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_y" )
	CLIENTEFFECT_MATERIAL( "dev/glow_color" )
	CLIENTEFFECT_MATERIAL( "dev/glow_cba" )
	CLIENTEFFECT_MATERIAL("effects/colourblind/cba_redteam")
	CLIENTEFFECT_MATERIAL("effects/colourblind/cba_greenteam")
	CLIENTEFFECT_MATERIAL("effects/colourblind/cba_blueteam")
	CLIENTEFFECT_MATERIAL("effects/colourblind/cba_yellowteam")
	CLIENTEFFECT_MATERIAL( "dev/glow_downsample" )
	CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )

// --------------------------------------------------------------------------------- //
// CTFModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

#ifdef STEAM_GROUP_CHECKPOINT
extern volatile uint64 *pulMaskGroup;
extern volatile uint64 ulGroupID;
#endif

void CTFModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	// Load the objects.txt file.
	LoadObjectInfos( ::filesystem );

	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );

#ifdef STEAM_GROUP_CHECKPOINT
	// Try to randomize this mask by using pointer addresses and hide it.
	pulMaskGroup = (uint64 *)( ( g_pClientMode - ( (int)this / 2 ) ) - sizeof( uint64 ) );
	ulGroupID = ( ( 3314649326771178240 / 64 ) * 2 ) ^ (uint64)pulMaskGroup;
#endif
}

void CTFModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );

	ConVarRef voice_steal( "voice_steal" );
	if ( voice_steal.IsValid() )
	{
		voice_steal.SetValue( 1 );
	}

	g_ThirdPersonManager.Init();
}

void CTFModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}

#ifdef ITEM_TAUNTING
static void __MsgFunc_PlayerTauntSoundLoopStart( bf_read &msg )
{
	int iPlayer = msg.ReadByte();
	char szSound[128];
	msg.ReadString( szSound, sizeof( szSound ), true );

	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayer ) );
	if ( pPlayer )
	{
		pPlayer->PlayTauntSoundLoop( szSound );
	}
}

static void __MsgFunc_PlayerTauntSoundLoopEnd( bf_read &msg )
{
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( msg.ReadByte() ) );
	if ( pPlayer )
	{
		pPlayer->StopTauntSoundLoop();
	}
}
#endif

void __MsgFunc_PlayerBonusPoints( bf_read& msg )
{
	int nPoints = (int)msg.ReadByte();
	int iPlayerIndex = (int)msg.ReadByte();
	int iSourceIndex = (int)msg.ReadShort();

	IGameEvent* event = gameeventmanager->CreateEvent( "player_bonuspoints" );
	if ( event )
	{
		event->SetInt( "points", nPoints );
		event->SetInt( "player_entindex", iPlayerIndex );
		event->SetInt( "source_entindex", iSourceIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}


ClientModeTFNormal::ClientModeTFNormal() :
m_sScreenshotRequestedCallback( this, &ClientModeTFNormal::OnScreenshotRequested ),
m_sScreenshotReadyCallback( this, &ClientModeTFNormal::OnScreenshotReady ),
m_LobbyJoinCallback( this, &ClientModeTFNormal::OnLobbyJoinRequested ),
m_LobyChatUpdateCallback( this, &ClientModeTFNormal::OnLobbyChatUpdate ),
m_LobbyChatMsgCallback( this, &ClientModeTFNormal::OnLobbyChatMsg )
{
	m_pMenuEngyBuild = NULL;
	m_pMenuEngyDestroy = NULL;
	m_pMenuSpyDisguise = NULL;
	m_pGameUI = NULL;
	m_pFreezePanel = NULL;
#ifdef ITEM_TAUNTING
	m_pTauntMenu = NULL;
#endif
	m_pLoadingScreen = NULL;

	m_pScoreboard = NULL;

#ifdef ITEM_TAUNTING
	HOOK_MESSAGE( PlayerTauntSoundLoopStart );
	HOOK_MESSAGE( PlayerTauntSoundLoopEnd );
#endif

	HOOK_MESSAGE( BreakModel );
	HOOK_MESSAGE( CheapBreakModel );

	HOOK_MESSAGE( PlayerBonusPoints );

	ResetVIPTutorial( false );
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeTFNormal::~ClientModeTFNormal()
{
}

// See interface.h/.cpp for specifics: Basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
// over and over again.
static CDllDemandLoader g_GameUI( "GameUI" );


void ClientModeTFNormal::Init()
{
#ifdef STEAM_GROUP_CHECKPOINT
	if ( ClientSteamContext().BLoggedOn() )
	{
		int numGroups = steamapicontext->SteamFriends()->GetClanCount();
		for ( int i = 0; i < numGroups; i++ )
		{
			CSteamID clanID = steamapicontext->SteamFriends()->GetClanByIndex( i );
			Assert( clanID.IsValid() );
			volatile uint64 ulMaskGroup = (uint64)pulMaskGroup;
			if ( ( clanID.ConvertToUint64() ^ ulMaskGroup ) == ulGroupID )
			{
				QAngle ang( 0, 0, 180 );
				engine->SetViewAngles( ang );
				break;
			}
		}
	}
#endif

	m_pMenuEngyBuild = GET_HUDELEMENT( CHudMenuEngyBuild );
	Assert( m_pMenuEngyBuild );

	m_pMenuEngyDestroy = GET_HUDELEMENT( CHudMenuEngyDestroy );
	Assert( m_pMenuEngyDestroy );

	m_pMenuSpyDisguise = GET_HUDELEMENT( CHudMenuSpyDisguise );
	Assert( m_pMenuSpyDisguise );

	m_pFreezePanel = GET_HUDELEMENT( CTFFreezePanel );
	Assert( m_pFreezePanel );

#ifdef ITEM_TAUNTING
	m_pTauntMenu = GET_HUDELEMENT( CHudMenuTauntSelection );
	Assert( m_pTauntMenu );
#endif

	CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
	if ( gameUIFactory )
	{
		m_pGameUI = (IGameUI *)gameUIFactory( GAMEUI_INTERFACE_VERSION, NULL );
		if ( m_pGameUI )
		{
			// Create the loading screen panel.
			m_pLoadingScreen = new CTFLoadingScreen();
			m_pLoadingScreen->InvalidateLayout( false, true );
			m_pLoadingScreen->SetVisible( false );
			m_pLoadingScreen->MakePopup( false );
			m_pGameUI->SetLoadingBackgroundDialog( m_pLoadingScreen->GetVPanel() );

			// Create the new main menu.
			if ( CommandLine()->CheckParm( "-nonewmenu" ) == NULL )
			{
				// Disable normal BG music played in GameUI.
				CommandLine()->AppendParm( "-nostartupsound", NULL );
				m_pGameUI->SetMainMenuOverride( ( new CTFMainMenu() )->GetVPanel() );
			}
		}
		else
		{
			Warning( "Unable to interface with GameUI!\n" );
		}
	}
	else
	{
		Warning( "Unable to retrieve GameUI Factory!\n" );
	}

	ListenForGameEvent( "teamplay_teambalanced_player" );
	ListenForGameEvent( "vip_assigned" );
	ListenForGameEvent( "vip_tutorial" );
	ListenForGameEvent( "game_maploaded" );

	UpdateSteamOverlayPosition();
	UpdateSteamScreenshotsHooking();

	BaseClass::Init();
}


void ClientModeTFNormal::Shutdown()
{
	DestroyStatsSummaryPanel();
	delete m_pLoadingScreen;

	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamMatchmaking()->LeaveLobby( m_LobbyID );
	}
}


void ClientModeTFNormal::LevelShutdown( void )
{
	ResetVIPTutorial( false );
}


void ClientModeTFNormal::StartVIPTutorial( int iTeamNumber, int iUserID, bool bResume /*= false*/ )
{
	int iVIPAnnotationProgress = Max( m_iVIPAnnotationProgress - 1, 0 );
	bool bVIPAnnotationHasDisplayed = m_bVIPAnnotationHasDisplayed;

	// Hide the previous annotation, if any.
	if ( m_iVIPUserID != -1 )
	{
		IGameEvent *pAnnotationEvent = gameeventmanager->CreateEvent( "hide_annotation" );
		if ( pAnnotationEvent )
		{
			pAnnotationEvent->SetInt( "id", m_iVIPUserID );
	
			gameeventmanager->FireEventClientSide( pAnnotationEvent );
		}
	}

	ResetVIPTutorial( !bResume );

	m_bVIPAnnotationHasDisplayed = bVIPAnnotationHasDisplayed;

	if ( m_bVIPAnnotationHasDisplayed || tf2c_vip_tutorials_viewed.GetInt() >= MAX_VIP_TUTORIALS_VIEWED )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !iTeamNumber || pLocalPlayer->GetTeamNumber() != iTeamNumber )
		return;

	int iEntID = engine->GetPlayerForUserID( iUserID );
	if ( !iEntID || pLocalPlayer->GetUserID() == iUserID )
		return;

	m_pVIP = static_cast<C_TFPlayer *>( ClientEntityList().GetEnt( iEntID ) );
	if ( !m_pVIP )
		return;

	m_iVIPUserID = m_pVIP->GetUserID();

	if ( bResume )
	{
		m_iVIPAnnotationProgress = iVIPAnnotationProgress;
		m_flNextVIPAnnotationTime = gpGlobals->curtime;
	}
	else
	{
		m_flNextVIPAnnotationTime = gpGlobals->curtime + 3.0f;
	}
}


void ClientModeTFNormal::ResetVIPTutorial( bool bOnlyAnnotations /*= true*/ )
{
	m_flNextVIPAnnotationTime = -1.0f;
	m_iVIPAnnotationProgress = 0;

	if ( !bOnlyAnnotations )
	{
		m_pVIP = NULL;
		m_iVIPUserID = -1;
		m_bVIPAnnotationHasDisplayed = false;
	}
}

void ClientModeTFNormal::InitViewport()
{
	m_pViewport = new TFViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModeTFNormal g_ClientModeNormal;


IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}


ClientModeTFNormal* GetClientModeTFNormal()
{
	return assert_cast<ClientModeTFNormal*>( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose: Fixes some bugs from base class.
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OverrideView( CViewSetup *pSetup )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// Let the player override the view.
	pPlayer->OverrideView( pSetup );

	if ( ::input->CAM_IsThirdPerson() )
	{
		int iObserverMode = pPlayer->GetObserverMode();
		if ( iObserverMode == OBS_MODE_NONE || iObserverMode == OBS_MODE_IN_EYE )
		{
			QAngle camAngles;
			if ( pPlayer->InThirdPersonShoulder() && !pPlayer->m_Shared.IsMovementLocked() )
			{
				VectorCopy( pSetup->angles, camAngles );
			}
			else
			{
				const Vector& cam_ofs = g_ThirdPersonManager.GetCameraOffsetAngles();
				camAngles[PITCH] = cam_ofs[PITCH];
				camAngles[YAW] = cam_ofs[YAW];
				camAngles[ROLL] = 0;

				// Override angles from third person camera
				VectorCopy( camAngles, pSetup->angles );
			}

			Vector camForward, camRight, camUp, cam_ofs_distance;

			// get the forward vector
			AngleVectors( camAngles, &camForward, &camRight, &camUp );

			cam_ofs_distance = g_ThirdPersonManager.GetDesiredCameraOffset();
			cam_ofs_distance *= g_ThirdPersonManager.GetDistanceFraction();

			VectorMA( pSetup->origin, -cam_ofs_distance[DIST_FORWARD], camForward, pSetup->origin );
			VectorMA( pSetup->origin, cam_ofs_distance[DIST_RIGHT], camRight, pSetup->origin );
			VectorMA( pSetup->origin, cam_ofs_distance[DIST_UP], camUp, pSetup->origin );
		}
	}
	else if ( ::input->CAM_IsOrthographic() )
	{
		pSetup->m_bOrtho = true;
		float w, h;
		::input->CAM_OrthographicSize( w, h );
		w *= 0.5f;
		h *= 0.5f;
		pSetup->m_OrthoLeft = -w;
		pSetup->m_OrthoTop = -h;
		pSetup->m_OrthoRight = w;
		pSetup->m_OrthoBottom = h;
	}
}

extern ConVar v_viewmodel_fov;
float ClientModeTFNormal::GetViewModelFOV( void )
{
	C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer )
		return v_viewmodel_fov.GetFloat();

	return pPlayer->GetViewModelFOV();
}


bool ClientModeTFNormal::ShouldDrawViewModel()
{
	C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer || pPlayer->m_Shared.InCond(TF_COND_ZOOMED) )
	{
		bool bOverride = false;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, bOverride, mod_zoomed_dont_hide_viewmodel );
		return bOverride;
	}
	return true;
}


bool ClientModeTFNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	if (!IsInFreezeCam()) {
		g_GlowObjectManager.RenderGlowEffects(pSetup, 0);
	}

	return BaseClass::DoPostScreenSpaceEffects( pSetup );
}


int ClientModeTFNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}


bool IsInCommentaryMode( void );
bool PlayerNameNotSetYet( const char *pszName );


void ClientModeTFNormal::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( !pszEventName || !pszEventName[0] )
		return;

	bool bStreamerMode = tf2c_streamer_mode.GetBool();

	if ( V_strcmp( "vip_tutorial", pszEventName ) == 0 )
	{
		StartVIPTutorial( event->GetInt( "team", 0 ), event->GetInt( "userid" ) );
	}
	else if ( V_strcmp( "duel_start", pszEventName ) == 0 )
	{
		CHudChat *pChat = GetTFChatHud();
		if ( !pChat || !g_PR )
			return;

		int iPlayer1 = event->GetInt( "player_1" );
		int iPlayer2 = event->GetInt( "player_2" );

		wchar_t wszPlayerName1[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName2[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer1 ), wszPlayerName1, sizeof( wszPlayerName1 ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer2 ), wszPlayerName2, sizeof( wszPlayerName2 ) );

		wchar_t wszLocalized[128];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_DuelStart" ), 2, wszPlayerName1, wszPlayerName2 );

		char szLocalized[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( 0, CHAT_FILTER_NONE, "%s", szLocalized );
	}
	else if ( V_strcmp( "game_maploaded", pszEventName ) == 0 )
	{
		// Remember which scoreboard should be used on this map.
		m_pScoreboard = (CTFClientScoreBoardDialog *)GetTFViewPort()->FindPanelByName( GetTFViewPort()->GetModeSpecificScoreboardName() );
	}
	else if ( !bStreamerMode )
	{
		if ( V_strcmp( "player_changename", pszEventName ) == 0 )
		{
			CHudChat *pChat = GetTFChatHud();
			if ( !pChat )
				return;

			const char *pszOldName = event->GetString( "oldname" );
			if ( PlayerNameNotSetYet( pszOldName ) )
				return;

			int iPlayerIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

			wchar_t wszOldName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pszOldName, wszOldName, sizeof( wszOldName ) );

			wchar_t wszNewName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString( "newname" ), wszNewName, sizeof( wszNewName ) );

			wchar_t wszLocalized[128];
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_Name_Change" ), 2, wszOldName, wszNewName );

			char szLocalized[128];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

			pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_NAMECHANGE, "%s", szLocalized );
		}
		else if ( V_strcmp( "player_team", pszEventName ) == 0 )
		{
			// Using our own strings here.
			CHudChat *pChat = GetTFChatHud();
			if ( !pChat )
				return;

			if ( event->GetBool( "silent" ) || event->GetBool( "disconnect" ) || IsInCommentaryMode() )
				return;

			int iPlayerIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			int iTeam = event->GetInt( "team" );
			bool bAutoTeamed = event->GetBool( "autoteam" );

			const char *pszName = event->GetString( "name" );
			if ( PlayerNameNotSetYet( pszName ) )
				return;

			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

			wchar_t wszTeam[64];
			V_wcscpy_safe( wszTeam, GetLocalizedTeamName( iTeam ) );

			// Client isn't going to catch up on team change so we have to set the color manually here.
			Color col = pChat->GetTeamColor( iTeam );

			wchar_t wszLocalized[128];

			if ( bAutoTeamed )
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_Joined_AutoTeam" ), 2, wszPlayerName, wszTeam );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_Joined_Team" ), 2, wszPlayerName, wszTeam );
			}

			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

			pChat->SetCustomColor( col );
			pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_TEAMCHANGE, "%s", szLocalized );

			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
			if ( pPlayer && pPlayer->IsLocalPlayer() )
			{
				// That's me!
				pPlayer->TeamChange( iTeam );

				// Totally bail out of the VIP tutorial if we switch teams.
				if ( m_flNextVIPAnnotationTime != -1.0f )
				{
					// Hide the previous annotation, if any.
					if ( m_iVIPUserID != -1 )
					{
						IGameEvent *pAnnotationEvent = gameeventmanager->CreateEvent( "hide_annotation" );
						if ( pAnnotationEvent )
						{
							pAnnotationEvent->SetInt( "id", m_iVIPUserID );
			
							gameeventmanager->FireEventClientSide( pAnnotationEvent );
						}
					}
		
					ResetVIPTutorial( false );
				}
			}
		}
		else if ( V_strcmp( "teamplay_teambalanced_player", pszEventName ) == 0 )
		{
			CHudChat *pChat = GetTFChatHud();
			if ( !pChat || !g_PR )
				return;

			int iPlayerIndex = event->GetInt( "player" );
			const char *pszName = g_PR->GetPlayerName( iPlayerIndex );
			int iTeam = event->GetInt( "team" );

			// Get the names of player and team.
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

			wchar_t wszTeam[64];
			V_wcscpy_safe( wszTeam, GetLocalizedTeamName( iTeam ) );

			wchar_t wszLocalized[128];
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_AutoBalanced" ), 2, wszPlayerName, wszTeam );

			char szLocalized[128];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

			// Set the color to team's color.
			pChat->SetCustomColor( pChat->GetTeamColor( iTeam ) );
			pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_TEAMCHANGE, "%s", szLocalized );
		}
		else if ( V_strcmp( "vip_assigned", pszEventName ) == 0 )
		{
			CHudChat *pChat = GetTFChatHud();
			if ( !g_PR )
				return;

			int iUserID = event->GetInt( "userid" );
			int iEntID = engine->GetPlayerForUserID( iUserID );
			const char *pszName = g_PR->GetPlayerName( iEntID );
			int iTeam = event->GetInt( "team" );

			if ( pChat )
			{
				// Get the names of player and team.
				wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

				wchar_t wszLocalized[128];
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_VIP_Assigned" ), 1, wszPlayerName );

				char szLocalized[128];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

				// Set the color to team's color.
				pChat->SetCustomColor( pChat->GetTeamColor( iTeam ) );
				pChat->ChatPrintf( iEntID, CHAT_FILTER_NONE, "%s", szLocalized );
			}

			if ( m_flNextVIPAnnotationTime != -1.0f )
			{
				// Save where we left off or reset if now invalid.
				StartVIPTutorial( iTeam, iUserID, true );
			}
		}
		else if ( V_strcmp( "player_disconnect", pszEventName ) == 0 )
		{
			CHudChat *pChat = GetTFChatHud();
			if ( !pChat )
				return;

			if ( IsInCommentaryMode() )
				return;

			const char *pszName = event->GetString( "name" );
			if ( PlayerNameNotSetYet( pszName ) )
				return;

			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

			wchar_t wszReason[64];
			const char *pszReason = event->GetString( "reason" );
			if ( pszReason && ( pszReason[0] == '#' ) && g_pVGuiLocalize->Find( pszReason ) )
			{
				V_wcscpy_safe( wszReason, g_pVGuiLocalize->Find( pszReason ) );
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( pszReason, wszReason, sizeof( wszReason ) );
			}

			wchar_t wszLocalized[100];
			if ( IsPC() )
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 2, wszPlayerName, wszReason );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 1, wszPlayerName );
			}

			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

			pChat->ChatPrintf( 0, CHAT_FILTER_JOINLEAVE, "%s", szLocalized );
		}
		else
		{
			BaseClass::FireGameEvent( event );
		}
	}
	else if ( V_strcmp( "teamplay_broadcast_audio", pszEventName ) == 0 ||
		V_strcmp( "server_cvar", pszEventName ) == 0 ||
		V_strcmp( "achievement_earned", pszEventName ) == 0 ||
		V_strcmp("achievement_earned_local", pszEventName) == 0) // dunno tho why but adding just in case
	{
		BaseClass::FireGameEvent( event );
	}
}


void ClientModeTFNormal::PostRenderVGui()
{
}


bool ClientModeTFNormal::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
	return BaseClass::CreateMove( flInputSampleTime, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: See if hud elements want key input. Return 0 if the key is swallowed
//-----------------------------------------------------------------------------
int	ClientModeTFNormal::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( tf_scoreboard_mouse_mode.GetInt() > 1 )
	{
		if ( m_pScoreboard && !m_pScoreboard->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	// check for hud menus
	if ( m_pMenuEngyBuild )
	{
		if ( !m_pMenuEngyBuild->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pMenuEngyDestroy )
	{
		if ( !m_pMenuEngyDestroy->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pMenuSpyDisguise )
	{
		if ( !m_pMenuSpyDisguise->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pFreezePanel )
	{
		m_pFreezePanel->HudElementKeyInput( down, keynum, pszCurrentBinding );
	}

#ifdef ITEM_TAUNTING
	if ( m_pTauntMenu )
	{
		if ( !m_pTauntMenu->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}
#endif

	return BaseClass::HudElementKeyInput( down, keynum, pszCurrentBinding );
}

//-----------------------------------------------------------------------------
// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
//-----------------------------------------------------------------------------
int ClientModeTFNormal::HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( tf_scoreboard_mouse_mode.GetInt() > 1 )
	{
		// Scoreboard allows enabling mouse input with right click so don't steal input from it.
		if ( m_pScoreboard && m_pScoreboard->IsVisible() )
		{
			return 1;
		}
	}

	return BaseClass::HandleSpectatorKeyInput( down, keynum, pszCurrentBinding );
}


void ClientModeTFNormal::Update( void )
{
	// Don't do any of this if we're done for this map session.
	if ( !m_bVIPAnnotationHasDisplayed )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && m_pVIP )
		{
			if ( pLocalPlayer->GetTeamNumber() != m_pVIP->GetTeamNumber() )
			{
				// Hide the current annotation.
				IGameEvent *pAnnotationEvent = gameeventmanager->CreateEvent( "hide_annotation" );
				if ( pAnnotationEvent )
				{
					pAnnotationEvent->SetInt( "id", m_iVIPUserID );

					gameeventmanager->FireEventClientSide( pAnnotationEvent );
				}

				// Pull the plug, we're done here.
				ResetVIPTutorial( true );
			}
			else if ( m_flNextVIPAnnotationTime != -1.0f && gpGlobals->curtime > m_flNextVIPAnnotationTime )
			{
				IGameEvent *pAnnotationEvent = gameeventmanager->CreateEvent( "show_annotation" );
				if ( pAnnotationEvent )
				{
					float flLifeTime = 8.0f;
					const char *pszText = "vip_tutorial";
					const char *pszSound = "coach/coach_look_here.wav";

					switch ( m_iVIPAnnotationProgress )
					{
						case 0:
							// Get the name of the VIP.
							wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
							g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( m_pVIP->entindex() ), wszPlayerName, sizeof( wszPlayerName ) );

							wchar_t wszLocalized[128];
							g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TF_VIP_Assigned" ), 1, wszPlayerName );

							char szLocalized[128];
							g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

							flLifeTime = 4.0f;
							pszText = szLocalized;
							m_flNextVIPAnnotationTime = gpGlobals->curtime + flLifeTime;
							break;
						case 1:
							flLifeTime = 6.0f;
							pszText = "#TF_VIP_Tutorial_Annotation_2";
							pszSound = "coach/coach_go_here.wav";
							m_flNextVIPAnnotationTime = gpGlobals->curtime + flLifeTime;
							break;
						case 2:
							pszText = "#TF_VIP_Tutorial_Annotation_3";
							pszSound = "coach/coach_defend_here.wav";
							m_flNextVIPAnnotationTime = -1.0f;
							break;
					}

					Vector pPosition = m_pVIP->GetAbsOrigin();
	
					pAnnotationEvent->SetFloat( "worldPosX", pPosition.x );
					pAnnotationEvent->SetFloat( "worldPosY", pPosition.y );
					pAnnotationEvent->SetFloat( "worldPosZ", pPosition.z + 24.0f );
					pAnnotationEvent->SetInt( "id", m_iVIPUserID );
					pAnnotationEvent->SetFloat( "lifetime", flLifeTime );
					pAnnotationEvent->SetString( "text", pszText );
					pAnnotationEvent->SetInt( "follow_entindex", m_pVIP->entindex() );
					pAnnotationEvent->SetBool( "show_distance", m_iVIPAnnotationProgress == 2 );
					pAnnotationEvent->SetString( "play_sound", pszSound );
	
					gameeventmanager->FireEventClientSide( pAnnotationEvent );

					m_iVIPAnnotationProgress++;

					// Job's done.
					if ( m_iVIPAnnotationProgress == 3 )
					{
						ResetVIPTutorial( false );

						m_bVIPAnnotationHasDisplayed = true;
						tf2c_vip_tutorials_viewed.SetValue( tf2c_vip_tutorials_viewed.GetInt() + 1 );
					}
				}
			}
		}
	}

	BaseClass::Update();

	if ( !engine->IsInGame() )
	{
		// Main Menu Animations
		g_pClientMode->GetViewportAnimationController()->UpdateAnimations( gpGlobals->curtime );
	}
}


void ClientModeTFNormal::UpdateSteamOverlayPosition( void )
{
	if ( steamapicontext->SteamUtils() )
	{
		// Move Steam notications to the upper left corner by default.
		ENotificationPosition ePosition = k_EPositionTopLeft;

		ConVarRef ui_steam_overlay_notification_position( "ui_steam_overlay_notification_position" );

		if ( V_strcasecmp( ui_steam_overlay_notification_position.GetString(), "topright" ) == 0 )
		{
			ePosition = k_EPositionTopRight;
		}
		else if ( V_strcasecmp( ui_steam_overlay_notification_position.GetString(), "bottomleft" ) == 0 )
		{
			ePosition = k_EPositionBottomLeft;
		}
		else if ( V_strcasecmp( ui_steam_overlay_notification_position.GetString(), "bottomright" ) == 0 )
		{
			ePosition = k_EPositionBottomRight;
		}

		steamapicontext->SteamUtils()->SetOverlayNotificationPosition( ePosition );
	}
}


void ClientModeTFNormal::UpdateSteamScreenshotsHooking( void )
{
	if ( steamapicontext->SteamScreenshots() )
	{
		ConVarRef cl_savescreenshotstosteam( "cl_savescreenshotstosteam" );
		if ( cl_savescreenshotstosteam.IsValid() )
		{
			cl_savescreenshotstosteam.SetValue( cl_steamscreenshots.GetBool() );
			steamapicontext->SteamScreenshots()->HookScreenshots( cl_steamscreenshots.GetBool() );
		}
	}
}


void ClientModeTFNormal::OnScreenshotRequested( ScreenshotRequested_t *info )
{
	// Fake screenshot button press.
	HudElementKeyInput( 0, BUTTON_CODE_INVALID, "screenshot" );
	engine->ClientCmd( "screenshot" );
}

const char *GetMapDisplayName( const char *mapName );


void ClientModeTFNormal::OnScreenshotReady( ScreenshotReady_t *info )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pPlayer && !enginevgui->IsGameUIVisible() && !vgui::surface()->IsCursorVisible() && info->m_eResult == k_EResultOK )
	{
		// Set map name.
		char szMapName[MAX_MAP_NAME];
		V_FileBase( engine->GetLevelName(), szMapName, sizeof( szMapName ) );
		steamapicontext->SteamScreenshots()->SetLocation( info->m_hLocal, GetMapDisplayName( szMapName ) );

		// Now tag users.
		CTFFreezePanel *pFreezePanel = GET_HUDELEMENT( CTFFreezePanel );
		if ( pFreezePanel && pFreezePanel->IsVisible() )
		{
			for ( CSteamID steamID : pFreezePanel->GetUsersInScreenshot() )
			{
				steamapicontext->SteamScreenshots()->TagUser( info->m_hLocal, steamID );
			}
		}
		else
		{
			CUtlVector<CSteamID> userList;
			pPlayer->CollectVisibleSteamUsers( userList );

			for ( CSteamID steamID : userList )
			{
				steamapicontext->SteamScreenshots()->TagUser( info->m_hLocal, steamID );
			}
		}
	}
}


void ClientModeTFNormal::CreateLobby( const CCommand &args )
{
	if ( !steamapicontext->SteamMatchmaking() )
		return;

	if ( !m_LobbyID.IsValid() )
	{
		ELobbyType type = args.ArgC() >= 2 ? k_ELobbyTypePrivate : k_ELobbyTypeFriendsOnly;
		SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->CreateLobby( type, 24 );
		m_LobbyCreateResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyCreated );
	}
}


void ClientModeTFNormal::LeaveLobby( const CCommand &args )
{
	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamMatchmaking()->LeaveLobby( m_LobbyID );
		m_LobbyID.Clear();
		Msg( "You left the lobby.\n" );
	}
}


void ClientModeTFNormal::InviteLobby( const CCommand &arg )
{
	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayInviteDialog( m_LobbyID );
	}
}


void ClientModeTFNormal::ConnectLobby( const CCommand &args )
{
	if ( args.ArgC() < 2 )
		return;

	if ( !steamapicontext->SteamMatchmaking() )
		return;

	CSteamID lobbyID( V_atoui64( args[1] ) );
	SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->JoinLobby( lobbyID );
	m_LobbyEnterResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyEnter );
}


void ClientModeTFNormal::ListLobbyPlayers( const CCommand &args )
{
	if ( !m_LobbyID.IsValid() )
		return;

	char szUsers[1024] = "Players: ";
	
	int count = steamapicontext->SteamMatchmaking()->GetNumLobbyMembers( m_LobbyID );
	for ( int i = 0; i < count; i++ )
	{
		if ( i != 0 )
		{
			V_strcat_safe( szUsers, ", " );
		}

		CSteamID steamID = steamapicontext->SteamMatchmaking()->GetLobbyMemberByIndex( m_LobbyID, i );
		const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
		V_strcat_safe( szUsers, pszName );
	}

	Msg( "%s\n", szUsers );
}


void ClientModeTFNormal::SayLobby( const CCommand &args )
{
	if ( !m_LobbyID.IsValid() || args.ArgC() < 2 )
		return;

	steamapicontext->SteamMatchmaking()->SendLobbyChatMsg( m_LobbyID, args[1], strlen( args[1] ) + 1 );
}


void ClientModeTFNormal::OnLobbyCreated( LobbyCreated_t *pCallResult, bool iofailure )
{
	if ( pCallResult->m_eResult == k_EResultOK )
	{
		m_LobbyID.SetFromUint64( pCallResult->m_ulSteamIDLobby );
		Msg( "Created a lobby.\n" );
	}
	else
	{
		Msg( "Failed to create a lobby.\n" );
	}
}


void ClientModeTFNormal::OnLobbyEnter( LobbyEnter_t *pCallResult, bool iofailure )
{
	if ( pCallResult->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess )
	{
		m_LobbyID.SetFromUint64( pCallResult->m_ulSteamIDLobby );
		Msg( "Entered a lobby with %d users.\n", steamapicontext->SteamMatchmaking()->GetNumLobbyMembers( m_LobbyID ) );
	}
	else
	{
		Msg( "Failed to join the lobby.\n" );
	}
}


void ClientModeTFNormal::OnLobbyJoinRequested( GameLobbyJoinRequested_t *pInfo )
{
	SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->JoinLobby( pInfo->m_steamIDLobby );
	m_LobbyEnterResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyEnter );
}


void ClientModeTFNormal::OnLobbyChatUpdate( LobbyChatUpdate_t *pInfo )
{
	CSteamID steamID( pInfo->m_ulSteamIDUserChanged );

	if ( pInfo->m_rgfChatMemberStateChange == k_EChatMemberStateChangeEntered )
	{
		Msg( "%s joined the lobby.\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ) );
	}
	else
	{
		Msg( "%s left the lobby.\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ) );
	}
}


void ClientModeTFNormal::OnLobbyChatMsg( LobbyChatMsg_t *pInfo )
{
	char szMessage[4096];
	CSteamID steamID;
	EChatEntryType type;
	steamapicontext->SteamMatchmaking()->GetLobbyChatEntry( m_LobbyID, pInfo->m_iChatID, &steamID, szMessage, sizeof( szMessage ), &type );

	Msg( "%s: %s\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ), szMessage );
}

class CPostEntityHudInitSystem : public CAutoGameSystem
{
public:
	CPostEntityHudInitSystem( const char *name ) : CAutoGameSystem( name )
	{
	}

	virtual void LevelInitPostEntity( void )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "game_maploaded" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
} g_PostEntityHudInit( "PostEntityHudInit" );

IStreamerModeChangedCallback::IStreamerModeChangedCallback()
{
	pStreamerModeChangeCallbacks.AddToTail( this );
}

IStreamerModeChangedCallback::~IStreamerModeChangedCallback()
{
	pStreamerModeChangeCallbacks.FindAndFastRemove( this );
}
