//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_MEDICCALLERS_H
#define TF_HUD_MEDICCALLERS_H
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


class CTFMedicCallerPanel : public vgui::EditablePanel, public TAutoList<CTFMedicCallerPanel>
{
	DECLARE_CLASS_SIMPLE( CTFMedicCallerPanel, vgui::EditablePanel );
public:
	CTFMedicCallerPanel( vgui::Panel *parent, const char *name );
	~CTFMedicCallerPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void Paint( void );

	void	GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void	SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto );
	static void AddMedicCaller( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto );

private:
	enum
	{
		DRAW_ARROW_UP,
		DRAW_ARROW_LEFT,
		DRAW_ARROW_RIGHT
	};

	CTFImagePanel	*m_pCallerBG;
	CTFImagePanel	*m_pCallerBurning;
	CTFImagePanel	*m_pCallerBleeding;
	CTFImagePanel	*m_pCallerHurt;
	CTFImagePanel	*m_pCallerAuto;

	CMaterialReference		m_ArrowMaterial;
	float			m_flRemoveAt;
	Vector			m_vecOffset;
	CHandle<C_TFPlayer> m_hPlayer;
	int				m_iDrawArrow;
	bool			m_bOnscreen;
	bool			m_bAuto;
};

#endif // TF_HUD_MEDICCALLERS_H
