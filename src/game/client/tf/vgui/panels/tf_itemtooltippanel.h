#ifndef TF_ITEMMODELTOOLTIPPANEL_H
#define TF_ITEMMODELTOOLTIPPANEL_H

#include "tf_dialogpanelbase.h"
#include "tf_tooltippanel.h"

class CTFAdvModelPanel;


class CTFItemToolTipPanel : public CTFToolTipPanel
{
	DECLARE_CLASS_SIMPLE( CTFItemToolTipPanel, CTFToolTipPanel );

public:
	CTFItemToolTipPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CTFItemToolTipPanel();
	void ApplySchemeSettings( vgui::IScheme *pScheme );
	void OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild );
	void ShowToolTip( CEconItemDefinition *pItemData, Panel *pFocusPanel = NULL );
	void HideToolTip( void );
	void AdjustToolTipSize( void );

	const char *GetResFilename( void ) { return "resource/UI/main_menu/ItemToolTipPanel.res"; }

private:
	int iItemID;
	CExLabel	*m_pTitle;
	CExLabel	*m_pClassName;
	CExLabel	*m_pAttributeText;
	CUtlVector<CExLabel *> m_pAttributes;
	Color	m_colorTitle;
};

#endif // TF_ITEMMODELTOOLTIPPANEL_H