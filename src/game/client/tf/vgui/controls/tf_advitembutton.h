#ifndef TF_ADVITEMBUTTON_H
#define TF_ADVITEMBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/KeyCode.h>
#include "tf_advbutton.h"
#include "tf_itemmodelpanel.h"


class CTFItemButton : public CTFButton
{
public:
	DECLARE_CLASS_SIMPLE( CTFItemButton, CTFButton );

	CTFItemButton( vgui::Panel *parent, const char *panelName, const char *text );
	~CTFItemButton();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );

	virtual void PerformLayout();
	virtual void ShowToolTip( bool bShow );
	virtual void DoClick( void );

	virtual void OnKeyCodeReleased( vgui::KeyCode code );

	ETFLoadoutSlot GetLoadoutSlot( void ) { return m_iLoadoutSlot; }
	void SetItem( CEconItemView *pItem, bool bUseDropSound = false, bool bEquipped = false );
	void SetLoadoutSlot( ETFLoadoutSlot iSlot, int iPreset );

protected:
	CPanelAnimationVar( bool, m_bHideNameDuringTooltip, "hide_name_during_tooltip", "1" );

	CEconItemView *m_pItem;
	ETFLoadoutSlot m_iLoadoutSlot;
	CItemModelPanel *m_pItemPanel;

};


#endif // TF_ADVITEMBUTTON_H
