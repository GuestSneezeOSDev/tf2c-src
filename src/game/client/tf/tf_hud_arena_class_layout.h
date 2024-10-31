//=============================================================================//
//
// Purpose: Arena team class composition
//
//=============================================================================//
#ifndef TF_HUD_ARENA_CLASS_LAYOUT_H
#define TF_HUD_ARENA_CLASS_LAYOUT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tf_controls.h"

#define TF_HUD_ARENA_NUM_CLASS_IMAGES 12

class CHudArenaClassLayout : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudArenaClassLayout, vgui::EditablePanel );

	CHudArenaClassLayout( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );

private:
	CTFImagePanel *m_pBackground;
	CTFImagePanel *m_pLocalPlayerBG;

	CExLabel *m_pTitleLabel;
	CExLabel *m_pChangeLabel;
	CExLabel *m_pChangeLabelShadow;

	CTFImagePanel *m_pClassImages[TF_HUD_ARENA_NUM_CLASS_IMAGES];
	int m_iClasses[TF_HUD_ARENA_NUM_CLASS_IMAGES];
	int m_iNumClasses;
	int m_iLocalPlayerClassImage;
};

#endif // TF_HUD_ARENA_CLASS_LAYOUT_H
