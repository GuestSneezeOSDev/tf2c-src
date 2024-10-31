//=============================================================================//
//
// Purpose: Totally redundant CP unlock countdown used in Arena.
//
//=============================================================================//
#ifndef TF_HUD_ARENA_CAPTUREPOINT_H
#define TF_HUD_ARENA_CAPTUREPOINT_H

#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"


class CHudArenaCapPointCountdown : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudArenaCapPointCountdown, vgui::EditablePanel );

	CHudArenaCapPointCountdown( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );

private:
	CExLabel *m_pCapCountdownLabel;

	bool	m_bFire5SecRemain;
	bool	m_bFire4SecRemain;
	bool	m_bFire3SecRemain;
	bool	m_bFire2SecRemain;
	bool	m_bFire1SecRemain;
	bool	m_bFire0SecRemain;
};

#endif // TF_HUD_ARENA_CAPTUREPOINT_H
