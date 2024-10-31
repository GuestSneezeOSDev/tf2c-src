//=============================================================================
//
// Purpose: Domination HUD
//
//=============================================================================
#ifndef TF_HUD_DOMINATION_H
#define TF_HUD_DOMINATION_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "GameEventListener.h"
#include "c_tf_team.h"

// Sort out text depth issue before enabling this for release.
// #define USE_POINT_DELTA_TEXT

#ifdef USE_POINT_DELTA_TEXT
// Floating delta text items, float off from the bottom of a team's score to 
// show points received.
typedef struct
{
	int m_iAmount;
	float m_flDieTime;
	vgui::Panel *m_pPanel;
} point_account_delta_t;
#endif

class CTFHudDomination : public vgui::EditablePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudDomination, vgui::EditablePanel );

	CTFHudDomination( vgui::Panel *pParent, const char *pszName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void OnTick( void );
	virtual void UpdateActiveTeam( int iTeam = TEAM_UNASSIGNED );
	virtual void UpdateScore( int iTeam = TEAM_UNASSIGNED, int iPoints = 0, int iAmount = 0 );
	virtual void Announce( void );
	virtual void Reset( void );

#ifdef USE_POINT_DELTA_TEXT
	virtual void Paint( void );
#endif

	virtual void FireGameEvent( IGameEvent *event );

private:
	int						m_iHighestScore;
	C_TFTeam				*m_pTeamInFirst;

	bool					m_bFire5Pts;
	bool					m_bFire4Pts;
	bool					m_bFire3Pts;
	bool					m_bFire2Pts;
	bool					m_bFire1Pts;
	bool					m_bFire80Pct;

#ifdef USE_POINT_DELTA_TEXT
	vgui::Label				*m_pScoreRedLabel;
	vgui::Label				*m_pScoreBlueLabel;
	vgui::Label				*m_pScoreGreenLabel;
	vgui::Label				*m_pScoreYellowLabel;

	CUtlVector<point_account_delta_t> m_AccountDeltaItems;

	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "50", "float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFontBig, "delta_item_font_big", "Default" );
#endif

};

#endif // TF_HUD_DOMINATION_H
