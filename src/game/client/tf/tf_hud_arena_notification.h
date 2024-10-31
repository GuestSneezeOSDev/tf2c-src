//=============================================================================//
//
// Purpose: Arena notification panel
//
//=============================================================================//
#ifndef TF_HUD_ARENA_NOTIFICATION_H
#define TF_HUD_ARENA_NOTIFICATION_H

#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

class CHudArenaNotification : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudArenaNotification, vgui::EditablePanel );

	CHudArenaNotification( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void LevelInit( void );
	virtual void OnTick( void );

	void SetupSwitchPanel( int iPanel );
	void MsgFunc_HudArenaNotify( bf_read &msg );

private:
	vgui::Label *m_pBalanceLabel;
	vgui::Label *m_pBalanceLabelTip;

	int m_iMessage;
	float m_flHideAt;
};

#endif // TF_HUD_ARENA_NOTIFICATION_H
