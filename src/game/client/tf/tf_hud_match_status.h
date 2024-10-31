//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TF_HUD_MATCH_STATUS
#define TF_HUD_MATCH_STATUS

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tf_hud_objectivestatus.h"

//-----------------------------------------------------------------------------
// Purpose: Stub HUD element from matchmaking that we need for the timer.
//-----------------------------------------------------------------------------
class CTFHudMatchStatus : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudMatchStatus, vgui::EditablePanel );

	CTFHudMatchStatus( const char *pElementName );
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset( void );
	virtual void OnThink( void );

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

private:
	CTFHudTimeStatus *m_pTimePanel;
};

#endif // TF_HUD_MATCH_STATUS
