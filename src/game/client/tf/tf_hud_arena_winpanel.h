//=============================================================================//
//
// Purpose: Arena win panel
//
//=============================================================================//
#ifndef TF_HUD_ARENA_WINPANEL_H
#define TF_HUD_ARENA_WINPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tf_gamerules.h"
#include "tf_controls.h"

class CTFArenaWinPanel : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFArenaWinPanel, vgui::EditablePanel );

	CTFArenaWinPanel( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void SetVisible( bool bVisible );
	virtual void Reset( void );
	virtual void OnThink( void );
	virtual int GetRenderGroupPriority() { return 70; }
	virtual void FireGameEvent( IGameEvent *event );
	
	void SetTeamBG( int iTeam );
	void SetupPlayerStats( bool bWinners );

	void SwitchScorePanels( bool bShow, bool bSetScores = false );
	int GetLeftTeam( void );
	int GetRightTeam( void );
	int GetTeamScore( int iTeam, bool bPrevious );

private:
	EditablePanel *m_pBGPanel;
	EditablePanel *m_pTeamScorePanel;
	EditablePanel *m_pWinnersPanel;
	EditablePanel *m_pBlueBG;
	EditablePanel *m_pRedBG;

	vgui::IBorder *m_pBlackBorder;
	vgui::IBorder *m_pBlueBorder;
	vgui::IBorder *m_pRedBorder;
	vgui::IBorder *m_pGreenBorder;
	vgui::IBorder *m_pYellowBorder;

	vgui::ScalableImagePanel *m_pArenaStreakBG;
	CExLabel *m_pArenaStreakLabel;

	int		m_iWinningTeam;
	bool	m_bFlawlessVictory;
	float	m_flTimeUpdateTeamScore;
	float	m_flTimeSwitchTeams;
	int		m_iBlueTeamScore;
	int		m_iRedTeamScore;
	int		m_iGreenTeamScore;
	int		m_iYellowTeamScore;
	int		m_iScoringTeam;
	ArenaPlayerRoundScore_t m_PlayerData[6];

	bool	m_bShowingGreenYellow;

	bool m_bShouldBeVisible;
};

#endif // TF_HUD_ARENA_WINPANEL_H
