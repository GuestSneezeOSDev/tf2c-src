//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_BUILDING_STATUS_H
#define TF_HUD_BUILDING_STATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "tf_controls.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ProgressBar.h>
#include "utlpriorityqueue.h"
#include "c_baseobject.h"

class CIconPanel;


class CBuildingHealthBar : public vgui::ProgressBar
{
	DECLARE_CLASS_SIMPLE( CBuildingHealthBar, vgui::ProgressBar );

public:
	CBuildingHealthBar( Panel *parent, const char *panelName );

	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	Color m_cHealthColor;
	Color m_cLowHealthColor;
};


class CBuildingStatusAlertTray : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusAlertTray, vgui::Panel );

public:
	CBuildingStatusAlertTray(Panel *parent, const char *panelName);

	void ApplySettings( KeyValues *inResourceData );

	virtual void Paint( void );
	virtual void PaintBackground( void );

	void LevelInit( void );

	void ShowTray( void );
	void HideTray( void );

	bool IsTrayOut( void ) { return m_bIsTrayOut; }

	void SetAlertType( BuildingHudAlert_t alertLevel );

	float GetPercentDeployed( void ) { return m_flAlertDeployedPercent; }
	BuildingHudAlert_t GetAlertType( void ) { return m_lastAlertType; }

private:
	bool m_bIsTrayOut;
	bool m_bUseTallImage;

	CHudTexture *m_pAlertPanelHudTexture;
	IMaterial *m_pAlertPanelMaterial;

	BuildingHudAlert_t m_lastAlertType;

	CPanelAnimationVar( float, m_flAlertDeployedPercent, "deployed", "0.0" );

};

#include <tf_inventory.h>
class CBuildingStatusItem : public vgui::EditablePanel, public CAutoGameSystem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem, vgui::EditablePanel );

public:
	// Actual panel constructor.
	CBuildingStatusItem( Panel *parent, const char *szLayout, int iObjectType, int iObjectMode );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint( void );
	virtual void PaintBackground( void );
	virtual void OnTick( void );

	virtual void PerformLayout( void );

	virtual void LevelInit( void );

	bool HasBeenPositioned() const { return bPositioned; }
	void SetPositioned(bool val) { bPositioned = val; }

	int GetRepresentativeObjectType();
	int GetRepresentativeObjectMode();
	C_BaseObject *GetRepresentativeObject();

	virtual int GetObjectPriority();

	virtual const char *GetBackgroundImage( void );
	virtual const char *GetInactiveBackgroundImage( void );

	vgui::EditablePanel *GetBuiltPanel() { return m_pBuiltPanel; }
	vgui::EditablePanel *GetNotBuiltPanel() { return m_pNotBuiltPanel; }

	vgui::EditablePanel *GetBuildingPanel() { return m_pBuildingPanel; }
	vgui::EditablePanel *GetRunningPanel() { return m_pRunningPanel; }

	void SetObject( C_BaseObject *pObj );

	bool IsActive( void ) { return m_bActive; }
	bool IsUpgradable( void );

	virtual bool ShouldShowTray( BuildingHudAlert_t iAlertLevel );

private:
	// False if we have not yet faded in and been positioned.
	bool bPositioned;

	char m_szLayout[128];

	int m_iObjectType;
	int m_iObjectMode;
	bool m_bActive;

	// Two main sub panels.
	vgui::EditablePanel *m_pNotBuiltPanel;
	vgui::EditablePanel *m_pBuiltPanel;

	// Sub panels of the m_pBuiltPanel.
	vgui::EditablePanel *m_pBuildingPanel; // Sub panel shown while building.
	vgui::EditablePanel *m_pRunningPanel; // Sub panel shown while built and running.
	vgui::ProgressBar *m_pHealthBar; // Health bar element.

	CHandle<C_BaseObject> m_pObject; // Pointer to the object we represent.

	// Alert side panel.
	CBuildingStatusAlertTray *m_pAlertTray;
	CIconPanel *m_pLevelIcons[3];
	CIconPanel *m_pJumppadModeIcons[2];
	CIconPanel *m_pWrenchIcon;
	CIconPanel *m_pSapperIcon;

	// Children of buildingPanel.
	vgui::ContinuousProgressBar *m_pBuildingProgress;

	// Elements that are always on.

	// Background.
	CIconPanel *m_pBackground;

};



class CBuildingStatusItem_SentryGun : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_SentryGun, CBuildingStatusItem );

public:
	CBuildingStatusItem_SentryGun( Panel *parent );

	virtual void OnTick( void );
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual const char *GetBackgroundImage( void );
	virtual const char *GetInactiveBackgroundImage( void );

private:
	CIconPanel *m_pSentryIcons[3];

	vgui::ImagePanel *m_pRocketsIcon;
	CIconPanel *m_pUpgradeIcon;
	CExLabel *m_pKillsLabel;

	vgui::ContinuousProgressBar *m_pShellsProgress;
	vgui::ContinuousProgressBar *m_pRocketsProgress;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;

	int m_iUpgradeLevel;

	// Kills.
	int m_iKills;

	// Ammo.
	bool m_bLowShells;
	bool m_bLowRockets;

	Color m_cLowAmmoColor;
	Color m_cNormalAmmoColor;

};

class CBuildingStatusItem_FlameSentry : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE(CBuildingStatusItem_FlameSentry, CBuildingStatusItem);

public:
	CBuildingStatusItem_FlameSentry(Panel* parent);

	virtual void OnTick(void);
	virtual void PerformLayout(void);
	virtual void ApplySchemeSettings(vgui::IScheme* scheme);

	virtual const char* GetBackgroundImage(void);
	virtual const char* GetInactiveBackgroundImage(void);

private:
	CIconPanel* m_pSentryIcons[3];

	vgui::ImagePanel* m_pRocketsIcon;
	CIconPanel* m_pUpgradeIcon;
	CExLabel* m_pKillsLabel;

	vgui::ContinuousProgressBar* m_pShellsProgress;
	vgui::ContinuousProgressBar* m_pRocketsProgress;
	vgui::ContinuousProgressBar* m_pUpgradeProgress;

	int m_iUpgradeLevel;

	// Kills.
	int m_iKills;

	// Ammo.
	bool m_bLowShells;
	bool m_bLowRockets;

	Color m_cLowAmmoColor;
	Color m_cNormalAmmoColor;

};


class CBuildingStatusItem_Dispenser : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_Dispenser, CBuildingStatusItem );

public:
	CBuildingStatusItem_Dispenser( Panel *parent );

	virtual void PerformLayout( void );

private:
	// Ammo.
	vgui::ContinuousProgressBar *m_pAmmoProgress;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;

	CIconPanel *m_pUpgradeIcon;

};

class CBuildingStatusItem_MiniDispenser : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE(CBuildingStatusItem_MiniDispenser, CBuildingStatusItem);

public:
	CBuildingStatusItem_MiniDispenser(Panel* parent);

	virtual void PerformLayout(void);

private:
	// Ammo.
	vgui::ContinuousProgressBar* m_pAmmoProgress;
	vgui::ContinuousProgressBar* m_pUpgradeProgress;

	CIconPanel* m_pUpgradeIcon;

};

class CBuildingStatusItem_TeleporterEntrance : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_TeleporterEntrance, CBuildingStatusItem );

public:
	CBuildingStatusItem_TeleporterEntrance( Panel *parent );
	virtual void OnTick( void );
	virtual void PerformLayout( void );

private:
	// 2 sub panels.
	vgui::EditablePanel *m_pChargingPanel;
	vgui::EditablePanel *m_pFullyChargedPanel;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;

	CIconPanel *m_pUpgradeIcon;

	// Children of m_pChargingPanel.
	vgui::ContinuousProgressBar *m_pRechargeTimer;

	// Local state.
	int m_iTeleporterState;
	int m_iTimesUsed;

};


class CBuildingStatusItem_TeleporterExit : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_TeleporterExit, CBuildingStatusItem );

public:
	CBuildingStatusItem_TeleporterExit( Panel *parent );
	virtual void PerformLayout( void );

private:
	vgui::ContinuousProgressBar *m_pUpgradeProgress;
	CIconPanel *m_pUpgradeIcon;

};


class CBuildingStatusItem_JumpPad : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_JumpPad, CBuildingStatusItem );

public:
	CBuildingStatusItem_JumpPad( Panel* parent, int iMode );
	virtual void PerformLayout( void );

private:
	// 2 sub panels.
	vgui::EditablePanel* m_pChargingPanel;
	vgui::EditablePanel* m_pFullyChargedPanel;

	CIconPanel* m_pUpgradeIcon;

	// Children of m_pChargingPanel.
	vgui::ContinuousProgressBar* m_pRechargeTimer;

	// Local state.
	int m_iJumpPadState;
	int m_iTimesUsed;

};


class CBuildingStatusItem_Sapper : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_Sapper, CBuildingStatusItem );

public:
	CBuildingStatusItem_Sapper( Panel *parent );

	virtual void PerformLayout( void );
	virtual bool ShouldShowTray( BuildingHudAlert_t iAlertLevel );

private:
	// Health of target building.
	vgui::ContinuousProgressBar *m_pTargetHealthBar;

	// Image of target building.
	CIconPanel *m_pTargetIcon;

	int m_iTargetType;

};

//-----------------------------------------------------------------------------
// Purpose: Container panel for object status panels
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer : public CHudElement, public vgui::Panel, public CAutoGameSystem /*<- includes, public CAutoGameSystem */
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer, vgui::Panel );

public:
	CHudBuildingStatusContainer( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void OnTick( void );

	virtual void LevelInit( void );

	void AddBuildingPanel( int iBuildingType, int iBuildingMode );
	CBuildingStatusItem *CreateItemPanel( int iObjectType, int iObjectMode );

	void UpdateAllBuildings( void );
	void OnBuildingChanged( int iBuildingType, int iBuildingMode );

	void RepositionObjectPanels();

	void FireGameEvent( IGameEvent *event );

	void RecalculateAlertState( void );

	virtual void PostInit();
private:
	// A list of CBuildingStatusItems that we're showing.
	CUtlVector<CBuildingStatusItem *> m_BuildingPanels;

	BuildingHudAlert_t m_AlertLevel;
	float m_flNextBeep;
	int m_iNumBeepsToBeep;

	float m_flNextSoundMix;
	bool m_bRevertSoundMixer;

	// if the client has jump pads or not
	bool bHasJumpPads = false;
	bool bHasMiniDispenser = false;
	bool bHasFlameSentry = false;

	// for OnTick optimization
	ConVar* pSndMixer = nullptr;

};

//-----------------------------------------------------------------------------
// Purpose: Separate panels for spy
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer_Spy : public CHudBuildingStatusContainer
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer_Spy, CHudBuildingStatusContainer );

public:
	CHudBuildingStatusContainer_Spy( const char *pElementName );

	virtual bool ShouldDraw( void );

};

//-----------------------------------------------------------------------------
// Purpose: Separate panels for engineer
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer_Engineer : public CHudBuildingStatusContainer
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer_Engineer, CHudBuildingStatusContainer );

public:
	CHudBuildingStatusContainer_Engineer( const char *pElementName );

	virtual bool ShouldDraw( void );

};
#endif //TF_HUD_BUILDING_STATUS_H
