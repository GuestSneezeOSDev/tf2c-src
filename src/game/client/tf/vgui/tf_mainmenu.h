#ifndef TFMAINMENU_H
#define TFMAINMENU_H

#include <vgui_controls/EditablePanel.h>
#include "panels/tf_tooltippanel.h"
#include "panels/tf_itemtooltippanel.h"

struct ClassStats_t;

enum MenuPanel //position in this enum = zpos on the screen
{
	NONE_MENU,
	BACKGROUND_MENU,
	MAIN_MENU,
	LOADOUT_MENU,
	SHADEBACKGROUND_MENU, //add popup/additional menus below:		
	STATSUMMARY_MENU,
	NOTIFICATION_MENU,
	OPTIONSDIALOG_MENU,
	CREATESERVER_MENU,
	ACHIEVEMENTS_MENU,
	QUIT_MENU,
	TOOLTIP_MENU,
	ITEMTOOLTIP_MENU,
	COUNT_MENU,

	FIRST_MENU = NONE_MENU + 1
};

#define GET_MAINMENUPANEL( className ) assert_cast<className *>( guiroot->GetMenuPanel( #className ) )

class CTFMenuPanelBase;


class CTFMainMenu : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFMainMenu, vgui::EditablePanel );

public:
	CTFMainMenu();
	virtual ~CTFMainMenu();

	enum
	{
		TFMAINMENU_STATUS_UNDEFINED = 0,
		TFMAINMENU_STATUS_MENU,
		TFMAINMENU_STATUS_BACKGROUNDMAP,
		TFMAINMENU_STATUS_INGAME,
	};

	CTFMenuPanelBase *GetMenuPanel( int iPanel );
	CTFMenuPanelBase *GetMenuPanel( const char *name );
	MenuPanel GetCurrentMainMenu( void ) { return MAIN_MENU; }
	int GetMainMenuStatus( void ) { return m_iMainMenuStatus; }

	virtual void OnTick();

	void AddMenuPanel( CTFMenuPanelBase *m_pPanel, int iPanel );
	void ShowPanel( MenuPanel iPanel, bool m_bShowSingle = false );
	void HidePanel( MenuPanel iPanel );
	void InvalidatePanelsLayout( bool layoutNow = false, bool reloadScheme = false );
	void LaunchInvalidatePanelsLayout();
	bool IsInLevel();
	bool IsInBackgroundLevel();
	void UpdateCurrentMainMenu();
	void SetStats( CUtlVector<ClassStats_t> &vecClassStats );
	CTFToolTipPanel *GetToolTip( void );
	void ShowToolTip( const char *pszText, Panel *pFocusPanel = NULL );
	void HideToolTip();
	CTFItemToolTipPanel *GetItemToolTip( void );
	void ShowItemToolTip( CEconItemDefinition *pItemData, Panel *pFocusPanel = NULL );
	void HideItemToolTip();
	void OnNotificationUpdate();
	void SetServerlistSize( int size );
	void OnServerInfoUpdate();
	void FadeMainMenuIn();
	void OpenLoadoutToClass( int iClass );

private:
	CTFMenuPanelBase	*m_pPanels[COUNT_MENU];
	const char			*m_pPanelNames[COUNT_MENU];

	int					m_iMainMenuStatus;
	int					m_iUpdateLayout;

};

extern CTFMainMenu *guiroot;

#endif // TFMAINMENU_H