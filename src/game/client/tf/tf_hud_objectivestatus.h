//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_OBJECTIVESTATUS_H
#define TF_HUD_OBJECTIVESTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_escort.h"
#include "tf_hud_vip.h"
#include "tf_hud_domination.h"
#include "tf_hud_powersiege.h"
#include "tf_hud_controlpointicons.h"
#include "GameEventListener.h"
#include "c_tf_player.h"


class CTFProgressBar : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFProgressBar, vgui::ImagePanel );

	CTFProgressBar( vgui::Panel *parent, const char *name );

	virtual void Paint();
	void SetPercentage( float flPercentage ){ m_flPercent = flPercentage; }	
	void SetIcon( const char* szIcon );

private:

	float	m_flPercent;
	int		m_iTexture;

	CPanelAnimationVar( Color, m_clrActive, "color_active", "TimerProgress.Active" );
	CPanelAnimationVar( Color, m_clrInActive, "color_inactive", "TimerProgress.InActive" );
	CPanelAnimationVar( Color, m_clrWarning, "color_warning", "TimerProgress.Active" );
	CPanelAnimationVar( float, m_flPercentWarning, "percent_warning", "0.75" );
};


// Floating delta text items, float off the top of the frame to 
// show changes to the timer value
typedef struct 
{
	// amount of delta
	int m_nAmount;

	// die time
	float m_flDieTime;

} timer_delta_t;

#define NUM_TIMER_DELTA_ITEMS 10


class CTFHudTimeStatus : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudTimeStatus, EditablePanel );

public:

	CTFHudTimeStatus( Panel *parent, const char *name );

	virtual void Paint( void );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	int GetTimerIndex( void ){ return m_iTimerIndex; }
	void SetTimerIndex( int index ){ m_iTimerIndex = ( index >= 0 ) ? index : 0; SetExtraTimePanels(); }
	
	virtual void FireGameEvent( IGameEvent *event );

	void SetExtraTimePanels();

protected:

	virtual void OnThink();

private:

	void SetTimeAdded( int iIndex, int nSeconds );
	void CheckClockLabelLength( CExLabel *pLabel, CTFImagePanel *pBG );
	void SetTeamBackground();

public:

	int					m_iTeamIndex;

private:

	float				m_flNextThink;
	int					m_iTimerIndex;
	bool				m_bSuddenDeath;
	bool				m_bOvertime;

	CExLabel			*m_pTimeValue;
	CTFProgressBar		*m_pProgressBar;

	CExLabel			*m_pWaitingForPlayersLabel;
	CTFImagePanel		*m_pWaitingForPlayersBG;

	CExLabel			*m_pOvertimeLabel;
	CTFImagePanel		*m_pOvertimeBG;

	CExLabel			*m_pSetupLabel;
	CTFImagePanel		*m_pSetupBG;

	// we'll have a second label/bg set for the SuddenDeath panel in case we want to change the look from the Overtime label
	CExLabel			*m_pSuddenDeathLabel;
	CTFImagePanel		*m_pSuddenDeathBG;

	ScalableImagePanel  *m_pTimePanelBG;

	int					m_iBaseYPos;
	CPanelAnimationVarAliasType( int, m_iTimerOffset, "timer_offset", "-10", "proportional_int" );

	// delta stuff
	int m_iTimerDeltaHead;
	timer_delta_t m_TimerDeltaItems[NUM_TIMER_DELTA_ITEMS];
	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
};

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the various objective displays
//-----------------------------------------------------------------------------
class CTFHudObjectiveStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudObjectiveStatus, vgui::EditablePanel );

public:
	CTFHudObjectiveStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void Think();

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

	CControlPointProgressBar *GetControlPointProgressBar( void );

private:

	void	SetVisiblePanels( void );
	void	TurnOffPanels( void );

private:
	CTFHudFlagObjectives	*m_pFlagPanel;
//	CTFHudKothTimeStatus	*m_pKothTimePanel;
	CHudControlPointIcons	*m_pControlPointIconsPanel;
	CControlPointProgressBar *m_pControlPointProgressBar;
	CTFHudEscort			*m_pEscortPanel;
	CTFHudMultipleEscort	*m_pEscortRacePanel;
	CTFHudVIP				*m_pVIPPanel;
	CTFHudDomination		*m_pDominationPanel;
	CTFHudPowerSiege		* m_pPowerSiegePanel;

	friend CTFHudTimeStatus;
};

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the various objective displays
//-----------------------------------------------------------------------------
class CTFHudKothTimeStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudKothTimeStatus, vgui::EditablePanel );

public:
	CTFHudKothTimeStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void Reset( void );
	virtual void OnTick( void );
	virtual void FireGameEvent( IGameEvent *event );

	void UpdateActiveTeam( void );
	CTFHudTimeStatus *GetTimer( int iTeam );

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

private:

	CPanelAnimationVarAliasType( int, m_nBlueActiveXPos, "blue_active_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nRedActiveXPos, "red_active_xpos", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_n4BlueActiveXPos, "4team_blue_active_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_n4RedActiveXPos, "4team_red_active_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_n4GreenActiveXPos, "4team_green_active_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_n4YellowActiveXPos, "4team_yellow_active_xpos", "0", "proportional_int" );


	CTFHudTimeStatus		*m_pBlueKothTimer;
	CTFHudTimeStatus		*m_pRedKothTimer;
	CTFHudTimeStatus		*m_pGreenKothTimer;
	CTFHudTimeStatus		*m_pYellowKothTimer;
	vgui::ImagePanel		*m_pActiveTimerBG;
	CTFHudTimeStatus		*m_pActiveKothTimerPanel;
};

#endif	// TF_HUD_OBJECTIVESTATUS_H