#ifndef TFMAINMENUPANELBASE_H
#define TFMAINMENUPANELBASE_H

#include "tf_controls.h"


class CTFMenuPanelBase : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFMenuPanelBase, vgui::EditablePanel );

public:
	CTFMenuPanelBase( vgui::Panel *parent, const char *panelName );

	virtual ~CTFMenuPanelBase();
	virtual void SetShowSingle( bool ShowSingle );
	virtual void Show();
	virtual void Hide();

protected:
	bool				m_bShowSingle;
};

#endif // TFMAINMENUPANELBASE_H