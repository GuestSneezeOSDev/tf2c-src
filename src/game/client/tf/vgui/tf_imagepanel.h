//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef TF_IMAGEPANEL_H
#define TF_IMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ScalableImagePanel.h"
#include "GameEventListener.h"
#include "tf_shareddefs.h"

class CTFImagePanel : public vgui::ScalableImagePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFImagePanel, vgui::ScalableImagePanel );

	CTFImagePanel(vgui::Panel *parent, const char *name);

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void UpdateBGImage( void );
	void SetBGImage( int iTeamNum );
	void UpdateBGTeam( void );

	virtual Color GetDrawColor( void );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );
	int		m_iVerticalOffsetSpecial;
	int		m_iHorizontalOffsetSpecial;

private:
	char	m_szTeamBG[TF_TEAM_COUNT][128];
	int		m_iBGTeam;
};


#endif // TF_IMAGEPANEL_H
