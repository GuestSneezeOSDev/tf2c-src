#ifndef TFMAINMENUPANEL_H
#define TFMAINMENUPANEL_H

#include "tf_menupanelbase.h"
#include "steam/steam_api.h"
#include <vgui_controls/HTML.h>

class CAvatarImagePanel;
class CTFButton;
class CTFBlogPanel;
class CTFServerlistPanel;
class CTFFriendlistPanel;
class CTFSlider;

enum MusicStatus
{
	MUSIC_STOP,
	MUSIC_FIND,
	MUSIC_PLAY,
	MUSIC_STOP_FIND,
	MUSIC_STOP_PLAY,
};


class CTFMainMenuPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFMainMenuPanel, CTFMenuPanelBase );

	friend class CTFOptionsAudioPanel;

public:
	CTFMainMenuPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTFMainMenuPanel();

	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick();
	virtual void OnThink();
	virtual void Show();
	virtual void Hide();
	virtual void OnCommand( const char* command );

	void SetVersionLabel();
	void PlayMusic();
	void OnNotificationUpdate();
	void ShowBlogPanel( bool show );
	void SetServerlistSize( int size );
	void UpdateServerInfo();
	void SetNextFriendsListRefreshTime( float flNextRefresh ) { m_flNextFriendsListRefresh = flNextRefresh; }

	MESSAGE_FUNC( OnWarningAccepted, "OnWarningAccepted" );
	MESSAGE_FUNC( OnWarningDeclined, "OnWarningDeclined" );
#if 0
	MESSAGE_FUNC( OnPromptAccepted, "OnPromptAccepted" );
	MESSAGE_FUNC( OnPromptDeclined, "OnPromptDeclined" );
#endif
	MESSAGE_FUNC( OnSentryAccepted, "OnSentryAccepted" );
	MESSAGE_FUNC( OnSentryDeclined, "OnSentryDeclined" );

private:
	CExLabel			*m_pVersionLabel;
	CTFButton			*m_pNotificationButton;
	CAvatarImagePanel	*m_pProfileAvatar;
	vgui::ImagePanel	*m_pFakeBGImage;

	int					m_iShowFakeIntro;

	int					m_nSongGuid;
	MusicStatus			m_psMusicStatus;

	CSteamID			m_SteamID;
	CTFBlogPanel		*m_pBlogPanel;
	CTFServerlistPanel	*m_pServerlistPanel;
	CTFFriendlistPanel	*m_pFriendlistPanel;

	const char			*m_pszNickname;
	bool				m_bInLevel;

	float				m_flNextFriendsListRefresh;
	CExLabel			*m_pGCUserCountLabel;

	CPanelAnimationVar( int, m_iLongName, "longName", "21" );

};


class CTFBlogPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFBlogPanel, CTFMenuPanelBase );

public:
	CTFBlogPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTFBlogPanel();
	void PerformLayout();
	void ApplySchemeSettings( vgui::IScheme *pScheme );
	void LoadBlogPost( const char* URL );

private:
	vgui::HTML			*m_pHTMLPanel;
};


class CTFServerlistPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFServerlistPanel, CTFMenuPanelBase );

public:
	CTFServerlistPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTFServerlistPanel();
	void ApplySchemeSettings( vgui::IScheme *pScheme );
	void SetServerlistSize( int size );
	void UpdateServerInfo();
	void OnThink();
	void OnCommand( const char* command );

private:
	static bool ServerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );
	vgui::SectionedListPanel	*m_pServerList;
	CTFButton					*m_pConnectButton;
	vgui::ImagePanel			*m_pLoadingSpinner;

	CPanelAnimationVarAliasType( int, m_iServerWidth, "server_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPlayersWidth, "players_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPingWidth, "ping_width", "23", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMapWidth, "map_width", "23", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iScrollWidth, "scroll_width", "23", "proportional_int" );

	int		m_iSize;
};

struct FriendListInfo_t
{
	FriendListInfo_t()
	{
		m_FriendGame = NULL;
		m_PlayingTF2C = false;
		m_PlayingGame = false;
		m_Online = true;
		m_Away = false;
	}

	FriendGameInfo_t *m_FriendGame;
	bool m_PlayingTF2C;
	bool m_PlayingGame;
	bool m_Online;
	bool m_Away;
};

class CTFFriendlistPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFFriendlistPanel, CTFMenuPanelBase );

public:
	CTFFriendlistPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTFFriendlistPanel();

	void ApplySchemeSettings( vgui::IScheme *pScheme );
	bool UpdateFriends();

#ifdef UPDATE_CHANGED_FRIENDS
	void OnPersonaStateChanged( PersonaStateChange_t *pInfo );
#endif

	bool Fail() { SetDialogVariable( "friend_count", "0" ); SetVisible( false ); return false; }

private:
	enum ETFSimilarFriendStates
	{
		TF_FRIENDSTATE_ONLINE = 0,
		TF_FRIENDSTATE_AWAY,
		TF_FRIENDSTATE_COUNT
	};

	vgui::PanelListPanel	*m_pFriendsList;

#ifdef UPDATE_CHANGED_FRIENDS
	CCallback<CTFFriendlistPanel, PersonaStateChange_t> m_PersonaChangeCallback;
#endif

};

class CTFFriendlistPanelItem : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFFriendlistPanelItem, vgui::EditablePanel );

public:
	CTFFriendlistPanelItem( vgui::PanelListPanel *parent, const char* name, CSteamID pFriendID, FriendListInfo_t *pFriendInfo );
	~CTFFriendlistPanelItem() {};

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	FriendListInfo_t	*m_pFriendInfo;

	CAvatarImagePanel	*m_pProfileAvatar;

	int					m_iFriendNameLength;

	CPanelAnimationVar( int, m_iLongName, "longName", "18" );

};

#endif // TFMAINMENUPANEL_H
