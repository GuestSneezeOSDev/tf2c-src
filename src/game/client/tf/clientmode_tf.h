//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_CLIENTMODE_H
#define TF_CLIENTMODE_H

#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tf_viewport.h"
#include "GameUI/IGameUI.h"

#define TF2C_STREAMER_MODE_SERVER_NAME L"Team Fortress 2 Classic"

class CHudMenuEngyBuild;
class CHudMenuEngyDestroy;
class CHudMenuSpyDisguise;
class CTFFreezePanel;
#ifdef ITEM_TAUNTING
class CHudMenuTauntSelection;
#endif
class CTFClientScoreBoardDialog;
class CTFLoadingScreen;
class C_TFPlayer;

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	LevelShutdown( void );

	void			StartVIPTutorial( int iTeamNumber, int iUserID, bool bResume = false );
	void			ResetVIPTutorial( bool bOnlyAnnotations = true );

	virtual void	OverrideView( CViewSetup *pSetup );

//	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual bool	DoPostScreenSpaceEffects( const CViewSetup *pSetup );

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui();

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );

	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void	Update( void );

	void			UpdateSteamOverlayPosition( void );
	void			UpdateSteamScreenshotsHooking( void );
	void			OnScreenshotRequested( ScreenshotRequested_t *info );
	void			OnScreenshotReady( ScreenshotReady_t *info );

	void			OnLobbyCreated( LobbyCreated_t *pCallResult, bool iofailure );
	void			OnLobbyEnter( LobbyEnter_t *pCallResult, bool iofailure );
	void			OnLobbyJoinRequested( GameLobbyJoinRequested_t *pInfo );
	void			OnLobbyChatUpdate( LobbyChatUpdate_t *pInfo );
	void			OnLobbyChatMsg( LobbyChatMsg_t *pInfo );

	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_create", CreateLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_leave", LeaveLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_players", ListLobbyPlayers, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_invite", InviteLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "connect_lobby", ConnectLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "say_lobby", SayLobby, "", 0 );
	
private:
	//	void	UpdateSpectatorMode( void );

private:
	CHudMenuEngyBuild *m_pMenuEngyBuild;
	CHudMenuEngyDestroy *m_pMenuEngyDestroy;
	CHudMenuSpyDisguise *m_pMenuSpyDisguise;
	CTFFreezePanel		*m_pFreezePanel;
#ifdef ITEM_TAUNTING
	CHudMenuTauntSelection *m_pTauntMenu;
#endif
	CTFLoadingScreen	*m_pLoadingScreen;
	IGameUI			*m_pGameUI;

	CTFClientScoreBoardDialog *m_pScoreboard;

	CSteamID m_LobbyID;

	CCallback<ClientModeTFNormal, ScreenshotRequested_t> m_sScreenshotRequestedCallback;
	CCallback<ClientModeTFNormal, ScreenshotReady_t> m_sScreenshotReadyCallback;

	CCallResult<ClientModeTFNormal, LobbyCreated_t> m_LobbyCreateResult;
	CCallResult<ClientModeTFNormal, LobbyEnter_t> m_LobbyEnterResult;
	CCallback<ClientModeTFNormal, GameLobbyJoinRequested_t> m_LobbyJoinCallback;
	CCallback<ClientModeTFNormal, LobbyChatUpdate_t> m_LobyChatUpdateCallback;
	CCallback<ClientModeTFNormal, LobbyChatMsg_t> m_LobbyChatMsgCallback;

	C_TFPlayer *m_pVIP;
	int m_iVIPUserID;
	float m_flNextVIPAnnotationTime;
	int m_iVIPAnnotationProgress;
	bool m_bVIPAnnotationHasDisplayed;
};

extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

class IStreamerModeChangedCallback
{
public:
	IStreamerModeChangedCallback();
	~IStreamerModeChangedCallback();

	virtual void OnStreamerModeChanged( bool bEnabled ) = 0;

};

#endif // TF_CLIENTMODE_H
