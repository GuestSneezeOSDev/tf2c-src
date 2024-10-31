//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TF_LOADINGSCREEN_H
#define TF_LOADINGSCREEN_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "GameEventListener.h"
#include "tf_hud_statpanel.h"

class CTFLoadingScreen : public vgui::EditablePanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFLoadingScreen, vgui::EditablePanel );

public:
	CTFLoadingScreen();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void FireGameEvent( IGameEvent *event );
private:
	MESSAGE_FUNC( OnActivate, "activate" );
	MESSAGE_FUNC( OnDeactivate, "deactivate" );

	void Reset();
	void SetDefaultSelections();
	void UpdateDialog();
	void UpdateBarCharts();
	void UpdateClassDetails();
	void UpdateTip();
	void ClearMapLabel();
	void GetRandomImage( char *pszBuf, int iBufLength, bool bWidescreen );

	vgui::ImagePanel *m_pBackgroundImage;
	vgui::ImagePanel *m_pClassImage;

	int m_iSelectedClass;							// what class we selected
};

#endif // TF_LOADINGSCREEN_H
