//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifdef TF2C_BETA

#ifndef TF_HUD_PATIENTHEALTHBAR_H
#define TF_HUD_PATIENTHEALTHBAR_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "tf_spectatorgui.h"


class CTFPatientHealthbarPanel : public vgui::EditablePanel, public TAutoList<CTFPatientHealthbarPanel>
{
	DECLARE_CLASS_SIMPLE( CTFPatientHealthbarPanel, vgui::EditablePanel );
public:
	CTFPatientHealthbarPanel( vgui::Panel *parent, const char *name );
	~CTFPatientHealthbarPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	//virtual void Paint( void );
	static void SetAlphaRecursive( Panel *pPanel, int iAlpha );
	static void SetSizeRecursive( Panel *pPanel, int iWidth, int iHeight );

	void	GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void	SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto );
	static void AddPatientHealthbar( C_TFPlayer *pPlayer, Vector &vecOffset );
	//void	SetDurationRemaining( float flDurationNew );
	void	UpdatePatientHealth();

	float			m_flHealthbarOpacity;
protected:
	int				m_iWidthDefault;
	int				m_iHeightDefault;
private:
	CTFSpectatorGUIHealth	*m_pPatientHealthbar;

	float			m_flRemoveAt;
	float			m_flFadeOutTime;
	Vector			m_vecOffset;
	CHandle<C_TFPlayer> m_hPlayer;
	int				m_iLastAlpha; // Prevents redundant uses of recursive setalpha
};

#endif // TF_HUD_PATIENTHEALTHBAR_H

#endif // TF2C_BETA