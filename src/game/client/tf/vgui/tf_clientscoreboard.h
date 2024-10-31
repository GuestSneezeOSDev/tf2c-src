//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SCOREBOARD_H
#define TF_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include "clientscoreboarddialog.h"
#include "tf_hud_playerstatus.h"
#include "tf_controls.h"
#include "tf_playermodelpanel.h"
#include "tf_shareddefs.h"
#include "clientmode_tf.h"

#define TF_SCOREBOARD_MAX_DOMINATIONS	16
#define TF_SCOREBOARD_MAX_PING_ICONS	8
#define TF_SCOREBOARD_MAX_STAT_LABELS	28

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTFClientScoreBoardDialog : public CClientScoreBoardDialog, public IStreamerModeChangedCallback
{
public:
	DECLARE_CLASS_SIMPLE( CTFClientScoreBoardDialog, CClientScoreBoardDialog );

	CTFClientScoreBoardDialog( IViewPort *pViewPort, const char *pszName, int iTeamCount = TF_ORIGINAL_TEAM_COUNT );
	virtual ~CTFClientScoreBoardDialog();

	virtual void Reset();
	virtual void Update();
	virtual void ShowPanel( bool bShow );
	virtual void OnCommand( const char *command );

	virtual void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	virtual int GetDefaultAvatar( int playerIndex );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void OnStreamerModeChanged( bool bEnabled );

	MESSAGE_FUNC_PTR( OnItemSelected, "ItemSelected", panel );
	MESSAGE_FUNC_PTR( OnItemContextMenu, "ItemContextMenu", panel );
	void OnScoreBoardMouseRightRelease( void );
	
protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual const char *GetResFilename( void ) { return "Resource/UI/scoreboard.res"; }

	vgui::SectionedListPanel *GetPlayerList( int iTeam ) { return m_pPlayerLists[iTeam]; }

private:
	friend class CTFFourTeamScoreBoardDialog;

	virtual void InitPlayerList( vgui::SectionedListPanel *pPlayerList );
	void SetPlayerListImages( vgui::SectionedListPanel *pPlayerList );
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerList();
	virtual void UpdateSpectatorList();
	virtual void UpdateArenaWaitingToPlayList();
	virtual void UpdatePlayerDetails();
	void MoveToCenterOfScreen();
	bool UseMouseMode( void );
	void InitializeInputScheme( void );
	void InitializeInputSchemeForList( vgui::SectionedListPanel *pListPanel, bool bUseMouseMode );

	void AdjustForVisibleScrollbar( void );

	virtual void FireGameEvent( IGameEvent *event );

	vgui::SectionedListPanel *GetSelectedPlayerList( void );

	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );

	vgui::SectionedListPanel	*m_pPlayerLists[TF_TEAM_COUNT];
	vgui::SectionedListPanel	*m_pPlayerListsVIP[TF_TEAM_COUNT];

	CPanelAnimationVarAliasType( int, m_iMedalWidth, "medal_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpacerWidth, "spacer", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iStatusWidth, "status_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNemesisWidth, "nemesis_width", "15", "proportional_int" );

	int							m_iImageDead;
	int							m_iImageDominated;
	int							m_iImageNemesis;
	int							m_iImageClasses[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )];
	int							m_iImageClassesAlt[TF_CLASS_COUNT_ALL + ( TF_CLASS_COUNT_ALL - 1 )];
	int							m_iImageDominations[TF_SCOREBOARD_MAX_DOMINATIONS];
	int							m_iImageDefaultAvatars[TF_TEAM_COUNT];
	int							m_iImagePings[TF_SCOREBOARD_MAX_PING_ICONS * 2];
	int							m_iImageMedals[TF_TEAM_COUNT][3];

	int							m_iTeamCount;

	CExLabel					*m_pLabelPlayerName;
	CExLabel					*m_pLabelMapName;
	vgui::ImagePanel			*m_pImagePanelHorizLine;
	CTFClassImage				*m_pClassImage;
	CTFPlayerModelPanel			*m_pClassModelPanel;
	EditablePanel				*m_pLocalPlayerStatsPanel;
	EditablePanel				*m_pLocalPlayerDuelStatsPanel;
	CExLabel					*m_pLocalPlayerStatLabels[TF_SCOREBOARD_MAX_STAT_LABELS];
	CExLabel					*m_pLocalPlayerStatLabelsLabels[TF_SCOREBOARD_MAX_STAT_LABELS];	// So we can replace jumppad jumps and teleports names around
	CExLabel					*m_pSpectatorsInQueue;
	CExLabel					*m_pServerTimeLeftValue;
	vgui::HFont					m_hTimeLeftFont;
	vgui::HFont					m_hTimeLeftNotSetFont;
	vgui::Menu					*m_pRightClickMenu;

	int							m_iSelectedPlayerIndex;
	bool						m_bMouseActivated;
	bool						m_bListScrollBarVisible[TF_TEAM_COUNT];
	bool						m_bListScrollBarVisibleVIP;
	int							m_iExtraSpace;

	wchar_t						m_wzServerLabel[256];
};

#endif // TF_SCOREBOARD_H
