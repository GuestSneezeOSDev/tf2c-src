//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TF_HUD_MENU_TAUNT_SELECTION_H
#define TF_HUD_MENU_TAUNT_SELECTION_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tf_itemmodelpanel.h"
#include "tf_controls.h"

#ifdef ITEM_TAUNTING
class CHudMenuTauntSelection : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudMenuTauntSelection, vgui::EditablePanel );

	CHudMenuTauntSelection( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pSceme );
	virtual bool ShouldDraw( void );
	virtual void Reset( void );
	virtual void SetVisible( bool bVisible );
	int HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	CON_COMMAND_MEMBER_F( CHudMenuTauntSelection, "+taunt", TauntDown, "Open taunt selection menu.", 0 );
	CON_COMMAND_MEMBER_F( CHudMenuTauntSelection, "-taunt", TauntUp, NULL, 0 )

private:
	CItemModelPanel *m_pItemPanels[8];

	bool m_bShow;
};
#endif

#endif // TF_HUD_MENU_TAUNT_SELECTION_H
