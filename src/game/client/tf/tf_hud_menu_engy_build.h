//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_ENGY_BUILD_H
#define TF_HUD_MENU_ENGY_BUILD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "IconPanel.h"
#include "tf_controls.h"

using namespace vgui;

#define ALL_BUILDINGS	-1

class CHudMenuEngyBuild : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuEngyBuild, EditablePanel );

public:
	CHudMenuEngyBuild( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	SetVisible( bool state );

	virtual void	OnTick( void );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority() { return 50; }

private:

	void GetBuildingIDAndModeFromSlot( int iSlot, int &iBuildingID, int &iObjectMode );

	void SendBuildMessage( int iSlot );
	bool SendDestroyMessage( int iSlot );

	void SetSelectedItem( int iSlot );

	// New sub-functions to make it easier to manage
	void CalculateCostOnAllPanels(C_TFPlayer* pLocalPlayer);
	void SetCostOnPanel(int iPanel, int iCost);
	int GetPanelCost(int iPanel, C_TFPlayer* pLocalPlayer);
	void UpdatePanel(int iPanel, C_TFPlayer* pLocalPlayer, int iAccount);
	void SetPanelVisibility(int iPanel, bool bVisibility);

	void UpdateHintLabels( void );	// show/hide the bright and dim build, destroy hint labels

private:
	EditablePanel *m_pAvailableObjects[8];
	EditablePanel *m_pAlreadyBuiltObjects[8];
	EditablePanel *m_pCantAffordObjects[8];

	// 360 layout only
	EditablePanel *m_pActiveSelection;

	int m_iSelectedItem;

	CExLabel *m_pBuildLabelBright;
	CExLabel *m_pBuildLabelDim;

	CExLabel *m_pDestroyLabelBright;
	CExLabel *m_pDestroyLabelDim;

	bool m_bInConsoleMode;
};

#endif	// TF_HUD_MENU_ENGY_BUILD_H
