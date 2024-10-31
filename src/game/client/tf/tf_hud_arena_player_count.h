//=============================================================================//
//
// Purpose: Arena player counter
//
//=============================================================================//
#ifndef TF_HUD_ARENA_PLAYER_COUNT_H
#define TF_HUD_ARENA_PLAYER_COUNT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"

class CHudArenaPlayerCount : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudArenaPlayerCount, vgui::EditablePanel );

	CHudArenaPlayerCount( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );
	virtual void FireGameEvent( IGameEvent *event );

	vgui::EditablePanel *GetTeamPanel( int iTeam );

private:
	vgui::EditablePanel *m_pRedPanel;
	vgui::EditablePanel *m_pBluePanel;
	vgui::EditablePanel *m_pGreenPanel;
	vgui::EditablePanel *m_pYellowPanel;

};

#endif // TF_HUD_ARENA_PLAYER_COUNT_H
