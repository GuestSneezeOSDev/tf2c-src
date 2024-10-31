#ifndef TFMAINMENULOADOUTPANEL_H
#define TFMAINMENULOADOUTPANEL_H

#include "tf_dialogpanelbase.h"
#include "tf_inventory.h"

#include "vgui_controls/PanelListPanel.h"

class CTFPlayerModelPanel;
class CTFItemSetPanel;
class CTFItemSelector;
class CModelPanel;
class CTFButton;
class CTFItemButton;
class CAdvTabs;

enum ETFLoadoutModifier
{
	TF_FIRST_LOADOUT_MODIFIER = 0,

	TF_STOCK_WEAPONS_ONLY_MODIFIER = TF_FIRST_LOADOUT_MODIFIER,
	TF_MEDIEVAL_WEAPONS_ONLY_MODIFIER,
	TF_RANDOMIZER_CLASSES_MODIFIER,
	TF_RANDOMIZER_ITEMS_MODIFIER,
	TF_RANDOMIZER_ATTRIBUTES_MODIFIER,
	TF_RANDOMIZER_MAYHEM_MODIFIER,

	TF_MODIFIER_COUNT
};


class CTFLoadoutPanel : public CTFDialogPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFLoadoutPanel, CTFDialogPanelBase );

public:
	CTFLoadoutPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CTFLoadoutPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnCommand( const char* command );
	virtual void Hide();
	virtual void Show();

	void SetupSlotIcon( CTFItemButton *pButton, ETFLoadoutSlot iSlot, int iPreset );

	void SetItemPreset( int iClass, ETFLoadoutSlot iSlot, int iPreset );

	int	GetCurrentClass() { return m_iCurrentClass; }
	void SetCurrentClass( int iClass );

	void UpdateSlotButtons( void );
	void ResetRows( void );

	void UpdateLoadoutModifierLabels( void );

	void OnItemSelectorOpened( void );
	void OnItemSelectorClosed( void );

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	bool UseController( void ) { return m_bUseController; }
#endif

	CTFItemSelector* GetItemSelectorPanel() { return m_pItemSelectorPanel; }

private:
	MESSAGE_FUNC( OnWarningAccepted, "OnWarningAccepted" );

	KeyValues *m_pItemButtonKeys;
	KeyValues *m_pLoadoutModifierKeys;

	CAdvTabs *m_pClassButtons;
	CAdvTabs *m_pTeamButtons;
	CTFPlayerModelPanel *m_pClassModelPanel;

	CTFItemSetPanel *m_pWeaponSetPanel;
	CTFItemSelector *m_pItemSelectorPanel;
	CTFItemButton *m_pWeaponSlotIcons[INVENTORY_ROWNUM];

	CExLabel *m_pLoadoutModifierLabel[TF_MODIFIER_COUNT];
	CExLabel *m_pEquipLabel;

	int	m_iCurrentClass;
	bool m_bLoadoutChanged;
	bool m_bHasAdjusted;
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	bool m_bUseController;
#endif

};


class CTFItemSetPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFItemSetPanel, vgui::EditablePanel );

public:
	CTFItemSetPanel( vgui::Panel* parent, const char *panelName );
	void OnCommand( const char* command );
};

struct ItemPreset_t
{
	int iPreset;
	CEconItemView *pItem;
};

struct ItemPage_t
{
	CUtlVector<ItemPreset_t> vecItems;
};


class CTFItemSelector : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFItemSelector, vgui::EditablePanel );

public:
	CTFItemSelector( vgui::Panel *parent, const char *panelName );
	~CTFItemSelector();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	void OnCommand( const char *command );

	virtual void OnKeyCodePressed( vgui::KeyCode code );

	void OpenItemSelector( int iClass, ETFLoadoutSlot iSlot, int iPreset );
	void CloseItemSelector( void );

	void CreateSortedItems( void );
	void RefreshItemButtons( bool bRecreateList = true );

	ETFLoadoutSlot GetSelectedSlot( void ) { return m_iSelectedSlot; }

private:
	CPanelAnimationVarAliasType( bool, m_bItemGridDEPRECATED, "item_grid", "0", "bool" );
	CPanelAnimationVarAliasType( int, m_iItemYPos, "item_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemXSpacing, "item_xspacing", "0", "int" );
	CPanelAnimationVarAliasType( int, m_iItemYSpacing, "item_yspacing", "0", "int" );
	CPanelAnimationVarAliasType( int, m_iItemXOffset, "item_xoffset", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemXLimit, "item_xlimit", "0", "int" );
	CPanelAnimationVarAliasType( int, m_iItemYLimit, "item_ylimit", "0", "int" );

	KeyValues				*m_pItemButtonKeys;

	CUtlVector<ItemPage_t>	m_vecPages;
	CUtlVector<CTFItemButton *> m_vecItemButtonsGrid;
	vgui::PanelListPanel	*m_pItemsList;

	int						m_iSelectedClass;
	ETFLoadoutSlot			m_iSelectedSlot;
	int						m_iSelectedPage;
	int						m_iSelectedPreset;

	vgui::Button			*m_pPreviousPageButton;
	vgui::Button			*m_pNextPageButton;
	vgui::Label				*m_pPageCountLabel;
};

#endif // TFMAINMENULOADOUTPANEL_H
