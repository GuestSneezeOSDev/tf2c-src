//=============================================================================//
//
// Purpose: HUD for drawing screen overlays.
//
//=============================================================================//
#ifndef TF_HUD_SCREENOVERLAYS_H
#define TF_HUD_SCREENOVERLAYS_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "clienteffectprecachesystem.h"

enum
{
	TF_OVERLAY_BURNING,
	TF_OVERLAY_INVULN,
	TF_OVERLAY_BLEED,
	TF_OVERLAY_COUNT,
};


class CHudScreenOverlays : public CHudElement, public vgui::EditablePanel, public CClientEffect
{
public:
	DECLARE_CLASS_SIMPLE( CHudScreenOverlays, vgui::EditablePanel );

	CHudScreenOverlays( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual void Cache( bool bCache = true );
	virtual void Paint( void );

	void InitOverlayMaterials( int iTeam );

private:
	CMaterialReference m_ScreenOverlays[TF_OVERLAY_COUNT];

	float		m_fColorValue;
};

#endif // TF_HUD_SCREENOVERLAYS_H
