//=============================================================================
//
// Purpose: VIP HUD
//
//=============================================================================
#ifndef TF_HUD_VIP_H
#define TF_HUD_VIP_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

class CTFHudVIP : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudVIP, vgui::EditablePanel );

	CTFHudVIP( vgui::Panel *pParent, const char *pszName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void OnTick( void );

private:
	float m_flPreviousProgress = 0.f;

	vgui::ImagePanel *m_pLevelBar;

	vgui::EditablePanel *m_pEscortItemPanel;
};

#endif // TF_HUD_VIP_H
