#ifndef TFMAINMENUNOTIFICATIONPANEL_H
#define TFMAINMENUNOTIFICATIONPANEL_H

#include "tf_dialogpanelbase.h"
#include "controls/tf_advbutton.h"
#include "tf_notificationmanager.h"


class CTFNotificationPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFNotificationPanel, CTFMenuPanelBase );

public:
	CTFNotificationPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTFNotificationPanel();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Show();
	virtual void Hide();
	virtual void OnCommand( const char* command );

	void OnNotificationUpdate();
	void UpdateLabels();
	void RemoveCurrent();

private:
	CTFButton	*m_pPrevButton;
	CTFButton	*m_pNextButton;
	CExLabel		*m_pMessageLabel;

	int				m_iMinHeight;
	int				m_iCurrent;
	int				m_iCount;
};

#endif // TFMAINMENUNOTIFICATIONPANEL_H
