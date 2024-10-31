#include "cbase.h"
#include "tf_loadoutpanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advitembutton.h"
#include "tf_playermodelpanel.h"
#include "basemodelpanel.h"
#include <vgui/ILocalize.h>
#include "econ_item_view.h"
#include "controls/tf_advtabs.h"
#include "c_tf_player.h"
#include <game/client/iviewport.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/QueryBox.h>
#include "tf_gamerules.h"

#include "tf_randomizer_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void HUDLoadoutGridChangedCallBack(IConVar* var, const char* pOldString, float flOldValue)
{
	CTFLoadoutPanel* pLoadoutPanel = GET_MAINMENUPANEL(CTFLoadoutPanel);
	// EVIL hack: if the cvar is enabled this callback gets called on startup
	// and CTFLoadoutPanel::InvalidateLayout for some unexplainable reason draws the loadout panel
	// so if the cvar is enabled it will draw on startup... 
	// however at this point in time m_iCurrentClass is TF_CLASS_UNDEFINED
	// so just check for that

	// ALSO if in any case m_iCurrentClass is NOT TF_CLASS_UNDEFINED on startup
	// because it tries to load items from that class 
	// but inventory hasn't been set up yet
	// m_pInventory is null so it crashes
	if( pLoadoutPanel && pLoadoutPanel->GetCurrentClass() )
	{
		pLoadoutPanel->InvalidateLayout( false, true );
		pLoadoutPanel->GetItemSelectorPanel()->InvalidateLayout( false, true );
	}
}

ConVar tf_respawn_on_loadoutchanges( "tf_respawn_on_loadoutchanges", "1", FCVAR_ARCHIVE, "When set to 1, you will automatically respawn whenever you change loadouts inside a respawn zone." );
static ConVar tf2c_loadout_grid( "tf2c_loadout_grid", "0", FCVAR_ARCHIVE, "Toggles the item grid on the loadout menu", HUDLoadoutGridChangedCallBack);

#define PANEL_Y_OFFSET YRES( 5 )

static ETFLoadoutSlot g_aClassLoadoutSlots[TF_CLASS_COUNT_ALL][INVENTORY_ROWNUM] =
{
	{	// Undefined
		TF_LOADOUT_SLOT_INVALID,
		TF_LOADOUT_SLOT_INVALID,
		TF_LOADOUT_SLOT_INVALID,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Scout
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Sniper
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Soldier
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Demoman
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Medic
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Heavy
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Pyro
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
	},
	{	// Spy
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_BUILDING,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_PDA2,
	},
	{	// Engineer
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_PDA1,
	},
	{	// Civilian
		TF_LOADOUT_SLOT_INVALID,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_INVALID,
		TF_LOADOUT_SLOT_INVALID,
	},
};

// This being a per-class array is used for slot aliases.
static const char *szLocalizedLoadoutSlots[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT] =
{
	{	// Undefined
	},
	{	// Scout
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Sniper
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Soldier
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Demoman
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Medic
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary_Medic",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Heavy
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Pyro
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building_Spy",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2_Spy",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Spy
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building_Spy",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2_Spy",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Engineer
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
	{	// Civilian
		"#TF_Loadout_Slot_Primary",
		"#TF_Loadout_Slot_Secondary",
		"#TF_Loadout_Slot_Melee",
		"#TF_Loadout_Slot_Utility",
		"#TF_Loadout_Slot_Building",
		"#TF_Loadout_Slot_PDA1",
		"#TF_Loadout_Slot_PDA2",
		"#TF_Loadout_Slot_Hat",
		"#TF_Loadout_Slot_Misc",
		"#TF_Loadout_Slot_Action",
		"#TF_Loadout_Slot_Taunt",
	},
};

// Menu buttons are not in the same order as the defines.
static const char *szRemapButtonToClass[TF_CLASS_MENU_BUTTONS - 2] =
{
	"Scout",
	"Scout",
	"Soldier",
	"Pyro",
	"Demoman",
	"Heavy",
	"Engineer",
	"Medic",
	"Sniper",
	"Spy",
	"Civilian",
};

static int iRemapButtonToClass[TF_CLASS_MENU_BUTTONS - 2] =
{
	TF_CLASS_SCOUT,
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,
	TF_CLASS_CIVILIAN,
};

static int iRemapClassToButton[TF_CLASS_COUNT_ALL] =
{
	1,		// TF_CLASS_UNDEFINED
	1,		// TF_CLASS_SCOUT
	8,		// TF_CLASS_SNIPER
	2,		// TF_CLASS_SOLDIER
	4,		// TF_CLASS_DEMOMAN
	7,		// TF_CLASS_MEDIC
	5,		// TF_CLASS_HEAVYWEAPONS
	3,		// TF_CLASS_PYRO
	9,		// TF_CLASS_SPY
	6,		// TF_CLASS_ENGINEER
	10,		// TF_CLASS_CIVILIAN
};

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT

inline void SetPanelFocus( vgui::FocusNavGroup *NavGroup, vgui::Panel *pPanel, bool bSetArmedState = false )
{
	vgui::Button *pButton = dynamic_cast<vgui::Button *>( NavGroup->GetCurrentFocus() );
	if ( pButton )
	{
		pButton->OnCursorExited();
	}

	NavGroup->SetCurrentFocus( pPanel->GetVPanel(), pPanel->GetVPanel() );

	if ( bSetArmedState )
	{
		pButton = dynamic_cast<vgui::Button *>( pPanel );
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutPanel::CTFLoadoutPanel( Panel *parent, const char *panelName ) : CTFDialogPanelBase( parent, panelName )
{
	m_pClassModelPanel = new CTFPlayerModelPanel( this, "classmodelpanel" );
	m_pWeaponSetPanel = new CTFItemSetPanel( this, "weaponsetpanel" );
	m_pItemSelectorPanel = new CTFItemSelector( this, "itemselector" );
	m_pTeamButtons = new CAdvTabs( this, "teamselection" );
	m_pClassButtons = new CAdvTabs( this, "classselection" );

	for ( int i = 0; i < ARRAYSIZE( m_pLoadoutModifierLabel ); i++ )
	{
		m_pLoadoutModifierLabel[i] = NULL;
	}

	// Make 5 weapon icons at first, though we can only really fit 4 with the way things are.
	for ( int i = 0; i < INVENTORY_ROWNUM; i++ )
	{
		m_pWeaponSlotIcons[i] = new CTFItemButton( m_pWeaponSetPanel, "WeaponIcons", "" );
	}

	// Setup UI navigation.
	for ( int iRow = 0; iRow < INVENTORY_ROWNUM; iRow++ )
	{
		if ( iRow > 0 )
		{
			m_pWeaponSlotIcons[iRow]->SetNavUp( m_pWeaponSlotIcons[Max( 0, iRow - 1 )] );
			m_pWeaponSlotIcons[iRow]->SetNavLeft( m_pWeaponSlotIcons[Max( 0, iRow - 1 )] );
		}

		if ( iRow < INVENTORY_ROWNUM - 1 )
		{
			m_pWeaponSlotIcons[iRow]->SetNavDown( m_pWeaponSlotIcons[Min( INVENTORY_ROWNUM, iRow + 1 )] );
			m_pWeaponSlotIcons[iRow]->SetNavRight( m_pWeaponSlotIcons[Min( INVENTORY_ROWNUM, iRow + 1 )] );
		}
	}

	m_pItemButtonKeys = NULL;
	m_pLoadoutModifierKeys = NULL;

	AddActionSignalTarget( this );

	m_iCurrentClass = TF_CLASS_UNDEFINED; // DO NOT!!!!!!!! CHANGE THIS IN THE CONSTRUCTOR. READ COMMENT ON HUDLoadoutGridChangedCallBack FUNCTION
	m_bLoadoutChanged = false;
	m_bHasAdjusted = false;
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	m_bUseController = IsConsole();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFLoadoutPanel::~CTFLoadoutPanel()
{
	if ( m_pItemButtonKeys )
	{
		m_pItemButtonKeys->deleteThis();
	}

	if ( m_pLoadoutModifierKeys )
	{
		m_pLoadoutModifierKeys->deleteThis();
	}
}


void CTFLoadoutPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bHasAdjusted = false;

	LoadControlSettings( "resource/UI/main_menu/LoadoutPanel.res" );

	for ( int iSlot = 0; iSlot < INVENTORY_ROWNUM; iSlot++ )
	{
		SetupSlotIcon( m_pWeaponSlotIcons[iSlot], (ETFLoadoutSlot)iSlot, 0 );
	}

	ResetRows();
}


void CTFLoadoutPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pButtonData = inResourceData->FindKey( "button_kv" );
	if ( pButtonData )
	{
		if ( m_pItemButtonKeys )
		{
			m_pItemButtonKeys->deleteThis();
		}

		m_pItemButtonKeys = new KeyValues( "button_kv" );
		pButtonData->CopySubkeys( m_pItemButtonKeys );
	}

	KeyValues *pLoadoutModifierData = inResourceData->FindKey( "loadoutmodifier_kv" );
	if ( pLoadoutModifierData )
	{
		if ( m_pLoadoutModifierKeys )
		{
			m_pLoadoutModifierKeys->deleteThis();
		}

		m_pLoadoutModifierKeys = new KeyValues( "loadoutmodifier_kv" );
		pLoadoutModifierData->CopySubkeys( m_pLoadoutModifierKeys );
	}
}


void CTFLoadoutPanel::PerformLayout()
{
	if ( m_pItemSelectorPanel->IsVisible() )
	{
		m_pItemSelectorPanel->CloseItemSelector();
	}

	BaseClass::PerformLayout();

	int iCurrentClass = m_iCurrentClass ? m_iCurrentClass : TF_CLASS_HEAVYWEAPONS;
	m_iCurrentClass = TF_CLASS_UNDEFINED;
	SetCurrentClass( iCurrentClass );
}


void CTFLoadoutPanel::OnKeyCodePressed( KeyCode code )
{
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	m_bUseController = ( IsConsole() || code >= JOYSTICK_FIRST );
	if ( code == KEY_XBUTTON_Y )
	{
		OnCommand( "resetinventory" );
	}
	else 
#endif
	if ( code >= KEY_1 && code <= KEY_9 )
	{
		SetCurrentClass( iRemapButtonToClass[( code - KEY_1 + 1 )] );
	}
	else if ( !m_pItemSelectorPanel->IsVisible() )
	{
		if ( code == KEY_XBUTTON_B )
		{
			Hide();
		}
		else if ( code == KEY_XBUTTON_LEFT_SHOULDER && m_iCurrentClass > TF_CLASS_SCOUT )
		{
			for (int iButton = iRemapClassToButton[m_iCurrentClass] - 1; iButton >= TF_CLASS_SCOUT; iButton--)
			{
				if (m_pClassButtons->FindChildByName(szRemapButtonToClass[iButton]))
				{
					SetCurrentClass(iRemapButtonToClass[iButton]);
					break;
				}
			}
		}
		else if ( code == KEY_XBUTTON_RIGHT_SHOULDER && m_iCurrentClass < TF_CLASS_COUNT )
		{
			for (int iButton = iRemapClassToButton[m_iCurrentClass] + 1; iButton <= TF_CLASS_COUNT; iButton++)
			{
				if (m_pClassButtons->FindChildByName(szRemapButtonToClass[iButton]))
				{
					SetCurrentClass(iRemapButtonToClass[iButton]);
					break;
				}
			}
		}
		else
		{
			BaseClass::OnKeyCodePressed( code );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


void CTFLoadoutPanel::OnKeyCodeReleased( KeyCode code )
{
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	m_bUseController = ( IsConsole() || code >= JOYSTICK_FIRST );
#endif
	BaseClass::OnKeyCodeReleased( code );
}


void CTFLoadoutPanel::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "selectteam_", 11 ) )
	{
		int iTeam = UTIL_StringFieldToInt( command + 11, g_aTeamNames, TF_TEAM_COUNT );

		if ( iTeam >= TF_TEAM_RED && iTeam < TF_TEAM_COUNT )
		{
			m_pClassModelPanel->SetTeam( iTeam );
		}
	}
	else if ( !V_strnicmp( command, "selectclass_", 12 ) )
	{
		int iClass = UTIL_StringFieldToInt( command + 12, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_CLASS_COUNT )
		{
			SetCurrentClass( iClass );
		}
	}
	else if ( !V_strcmp( command, "resetinventory" ) )
	{
		// Notify the user that this will reset all of their achievements (Dear Lord!)
		QueryBox *pResetInventoryQuery = new QueryBox( "#TF_ConfirmResetInventory_Title", "#TF_ConfirmResetInventory_Message", guiroot->GetParent() );
		if ( pResetInventoryQuery )
		{
			pResetInventoryQuery->AddActionSignalTarget( this );

			pResetInventoryQuery->SetOKCommand( new KeyValues( "OnWarningAccepted" ) );
			pResetInventoryQuery->SetOKButtonText( "#TF_ConfirmResetInventory_OK" );

			pResetInventoryQuery->DoModal();
		}
	}
	else if ( !V_strncmp( command, "loadout", 7 ) )
	{
		const char *sChar = strchr( command, ' ' );
		if ( sChar )
		{
			int iSlot = atoi( sChar + 1 );
			sChar = strchr( sChar + 1, ' ' );
			if ( sChar )
			{
				m_pItemSelectorPanel->OpenItemSelector( m_iCurrentClass, (ETFLoadoutSlot)iSlot, atoi( sChar + 1 ) );

				m_pClassModelPanel->HoldItemInSlot( (ETFLoadoutSlot)iSlot );
			}
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


void CTFLoadoutPanel::Show()
{
	BaseClass::Show();

	m_bLoadoutChanged = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		int iClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_CLASS_COUNT )
		{
			SetCurrentClass( iClass );
		}

		int iTeam = pPlayer->GetTeamNumber();
		if ( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT )
		{
			m_pClassModelPanel->SetTeam( iTeam );
		}
	}
	else
	{
		if ( m_iCurrentClass == TF_CLASS_UNDEFINED )
		{
			// Heavy Weapons Guy all the way.
			SetCurrentClass( TF_CLASS_HEAVYWEAPONS );
		}

		m_pClassModelPanel->SetTeam( TF_TEAM_RED );
	}

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	SetPanelFocus( &GetFocusNavGroup(), this );
#endif
}


void CTFLoadoutPanel::Hide()
{
	if ( m_pItemSelectorPanel->IsVisible() )
	{
		m_pItemSelectorPanel->CloseItemSelector();
	}

	if ( m_bLoadoutChanged )
	{
		m_bLoadoutChanged = false;
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && tf_respawn_on_loadoutchanges.GetBool() )
		{
			engine->ExecuteClientCmd( "loadoutchanged" );
		}
	}

	// Notify the class menu.
	gViewPortInterface->PostMessageToPanel( PANEL_CLASS, new KeyValues( "LoadoutChanged" ) );

	BaseClass::Hide();

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	SetPanelFocus( &GetFocusNavGroup(), GetParent() );
#endif
}


void CTFLoadoutPanel::SetupSlotIcon( CTFItemButton *pButton, ETFLoadoutSlot iSlot, int iPreset )
{
	if ( m_pItemButtonKeys )
	{
		pButton->ApplySettings( m_pItemButtonKeys );
		pButton->SetPos( 0, pButton->GetTall() + PANEL_Y_OFFSET );
		pButton->SetLoadoutSlot( iSlot, iPreset );
	}
}


void CTFLoadoutPanel::SetCurrentClass( int iClass )
{
	if ( m_pItemSelectorPanel->IsVisible() )
	{
		m_pItemSelectorPanel->CloseItemSelector();
	}

	if ( m_iCurrentClass == iClass )
		return;

	m_pClassButtons->SetSelectedButton( g_aPlayerClassNames_NonLocalized[iClass] );

	m_iCurrentClass = iClass;

	m_pClassModelPanel->SetToPlayerClass( m_iCurrentClass );
	m_pClassModelPanel->LoadItems();

	UpdateSlotButtons();

	const wchar_t *pszLocalized = g_pVGuiLocalize->Find( g_aPlayerClassNames[m_iCurrentClass] );
	if ( pszLocalized )
	{
		SetDialogVariable( "classname", pszLocalized );
	}
	else
	{
		SetDialogVariable( "classname", g_aPlayerClassNames[m_iCurrentClass] );
	}
}


void CTFLoadoutPanel::ResetRows()
{
	for ( int iSlot = 0; iSlot < INVENTORY_ROWNUM; iSlot++ )
	{
		m_pWeaponSlotIcons[iSlot]->SetPos( 0, iSlot * ( m_pWeaponSlotIcons[iSlot]->GetTall() + PANEL_Y_OFFSET ) );
	}
}


void CTFLoadoutPanel::UpdateSlotButtons( void )
{
	// Update the loadout modifier labels.
	UpdateLoadoutModifierLabels();

	// No point in updating all those buttons if they're not visible.
	if ( !m_pWeaponSetPanel->IsVisible() )
		return;

	ETFLoadoutSlot iSlot;
	int iPrevRow, iPreset;
	CTFItemButton *pItemButton;
	CEconItemView *pItem;
#ifndef ITEM_ACKNOWLEDGEMENTS
#ifdef HIGHLIGHT_SLOT_WITH_PRESETS
	bool bHasItems;
#endif
#endif
	bool bNavigated = false;
	for ( int iRow = 0; iRow < INVENTORY_ROWNUM; iRow++ )
	{
		iSlot = g_aClassLoadoutSlots[m_iCurrentClass][iRow];

		// Adjust the slot positioning if we happen to be missing one (just the last slot basically).
		if ( iSlot == TF_LOADOUT_SLOT_INVALID )
		{
			m_pWeaponSlotIcons[iRow]->SetVisible( false );

			if ( !m_bHasAdjusted )
			{
				for ( iPrevRow = INVENTORY_ROWNUM - 1; iPrevRow >= 0; iPrevRow-- )
				{
					pItemButton = m_pWeaponSlotIcons[iPrevRow];
					pItemButton->SetPos( pItemButton->GetXPos(), pItemButton->GetYPos() + ( pItemButton->GetTall() / 2 ) );
				}

				m_bHasAdjusted = true;
			}

			continue;
		}
		else if ( m_bHasAdjusted )
		{
			for ( iPrevRow = INVENTORY_ROWNUM - 1; iPrevRow >= 0; iPrevRow-- )
			{
				pItemButton = m_pWeaponSlotIcons[iPrevRow];
				pItemButton->SetPos( pItemButton->GetXPos(), pItemButton->GetYPos() - ( pItemButton->GetTall() / 2 ) );
			}

			m_bHasAdjusted = false;
		}

		iPreset = GetTFInventory()->GetItemPreset( m_iCurrentClass, iSlot );

		pItem = GetTFInventory()->GetItem( m_iCurrentClass, iSlot, iPreset );
		if ( pItem )
		{
#ifndef ITEM_ACKNOWLEDGEMENTS
#ifdef HIGHLIGHT_SLOT_WITH_PRESETS
			bHasItems = ( GetTFInventory()->GetNumItems( m_iCurrentClass, iSlot ) > 1 );
#endif
#endif

			pItemButton = m_pWeaponSlotIcons[iRow];
			pItemButton->SetVisible( true );
			pItemButton->SetItem( pItem, true );
			pItemButton->SetLoadoutSlot( iSlot, iPreset );

#ifdef ITEM_ACKNOWLEDGEMENTS
			pItemButton->SetSelected( !GetTFInventory()->ItemIsAcknowledged( pItem->GetItemDefIndex() ) );
#else
#ifdef HIGHLIGHT_SLOT_WITH_PRESETS
			pItemButton->SetSelected( bHasItems );
#endif
#endif

#ifdef DISABLE_SLOT_WITH_PRESETS
			pItemButton->SetEnabled( bHasItems );
#endif

			if ( !bNavigated )
			{
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
				SetPanelFocus( &GetFocusNavGroup(), pItemButton, UseController() );
#endif
				bNavigated = true;
			}
		}
	}
}


void CTFLoadoutPanel::SetItemPreset( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->IsPlayerClass( iClass, true ) && iPreset != GetTFInventory()->GetItemPreset( iClass, iSlot ) )
		{
			m_bLoadoutChanged = true;
		}

		engine->ExecuteClientCmd( VarArgs( "setitempreset %d %d %d", iClass, iSlot, iPreset ) );
	}

	GetTFInventory()->SetItemPreset( iClass, iSlot, iPreset );

	if ( m_pItemSelectorPanel->IsVisible() )
	{
		m_pItemSelectorPanel->CloseItemSelector();
	}
	else
	{
		UpdateSlotButtons();
	}

	m_pClassModelPanel->LoadItems( iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: Called after inventory is reset (?)
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnWarningAccepted( void )
{
	m_bLoadoutChanged = true;

	GetTFInventory()->ResetInventory();

	if ( m_pItemSelectorPanel->IsVisible() )
	{
		m_pItemSelectorPanel->CloseItemSelector();
	}
	else
	{
		UpdateSlotButtons();
	}

	m_pClassModelPanel->LoadItems();

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		pPlayer->LoadInventory();
	}
}

// Traverse backwards down the array and make sure it's valid so there's no blank spaces or anything.
int GetPreviousIndex( int iCurrentIndex, CExLabel *pPreviousLabels[TF_MODIFIER_COUNT] )
{
	for ( int j = iCurrentIndex - 1; j >= TF_FIRST_LOADOUT_MODIFIER; j-- )
	{
		if ( pPreviousLabels[j] == NULL )
			continue;

		return j;
	}

	return iCurrentIndex;
}

// Localizations for Each Special Condition
static const char *szLocalizedLoadoutModifiers[TF_MODIFIER_COUNT] =
{
	"#TF_Loadout_Modifier_StockItemsOnly",
	"#TF_Loadout_Modifier_MedievalItemsOnly",
	"#TF_Loadout_Modifier_RandomizerClasses",
	"#TF_Loadout_Modifier_RandomizerItems",
	"#TF_Loadout_Modifier_RandomizerAttributes",
	"#TF_Loadout_Modifier_RandomizerMayhem",
};

//-----------------------------------------------------------------------------
// Purpose: Tell the player the special conditions of the server (Medieval, Stock Weapons, Randomizer, etc).
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::UpdateLoadoutModifierLabels( void )
{
	// A server could have something enabled that affects the player's loadout, display it.
	if ( m_pLoadoutModifierKeys )
	{
		bool bRandomizerClasses = GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_CLASSES );
		bool bRandomizerItems = GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS );
		bool bRandomizerAttributes = GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ATTRIBUTES );
		bool bRandomizerMayhem = ( bRandomizerClasses && bRandomizerItems && bRandomizerAttributes );

		bool bLoadoutModifiers[TF_MODIFIER_COUNT] =
		{
			ConVarRef( "tf2c_force_stock_weapons" ).GetBool(),
			TFGameRules() && TFGameRules()->IsInMedievalMode(),
			bRandomizerClasses && !bRandomizerMayhem,
			bRandomizerItems && !bRandomizerMayhem,
			bRandomizerAttributes && !bRandomizerMayhem,
			bRandomizerMayhem,
		};

		for ( int i = 0; i < ARRAYSIZE( m_pLoadoutModifierLabel ); i++ )
		{
			if ( m_pLoadoutModifierLabel[i] )
			{
				m_pLoadoutModifierLabel[i]->MarkForDeletion();
				m_pLoadoutModifierLabel[i] = NULL;
			}

			if ( m_pItemSelectorPanel->IsVisible() )
				continue;

			if ( guiroot->IsInLevel() && bLoadoutModifiers[i] )
			{
				m_pLoadoutModifierLabel[i] = new CExLabel( this, "LoadoutModifierLabel", "LoadoutModifier" );
				m_pLoadoutModifierLabel[i]->ApplySettings( m_pLoadoutModifierKeys );
				m_pLoadoutModifierLabel[i]->SetText( szLocalizedLoadoutModifiers[i] );

				int x, y, w, h;
				m_pLoadoutModifierLabel[GetPreviousIndex( i, m_pLoadoutModifierLabel )]->GetPos( x, y );
				m_pLoadoutModifierLabel[GetPreviousIndex( i, m_pLoadoutModifierLabel )]->GetSize( w, h );
				m_pLoadoutModifierLabel[i]->SetPos( x, y + ( h + 6 ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hide certain things when the item selector opens.
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnItemSelectorOpened( void )
{
	UpdateSlotButtons();
	m_pWeaponSetPanel->SetVisible( false );
	m_pTeamButtons->SetVisible( false );
	m_pClassModelPanel->SetVisible( false );

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	SetPanelFocus( &GetFocusNavGroup(), m_pItemSelectorPanel, UseController() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Show certain things when the item selector closes.
//-----------------------------------------------------------------------------
void CTFLoadoutPanel::OnItemSelectorClosed( void )
{
	m_pClassModelPanel->SetVisible( true );
	m_pTeamButtons->SetVisible( true );
	m_pWeaponSetPanel->SetVisible( true );
	UpdateSlotButtons();

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	for ( int i = 0; i < INVENTORY_ROWNUM; i++ )
	{
		if ( m_pWeaponSlotIcons[i]->GetLoadoutSlot() == m_pItemSelectorPanel->GetSelectedSlot() )
		{
			SetPanelFocus( &GetFocusNavGroup(), m_pWeaponSlotIcons[i], UseController() );
			break;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemSetPanel::CTFItemSetPanel( vgui::Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
}


void CTFItemSetPanel::OnCommand( const char* command )
{
	GetParent()->OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemSelector::CTFItemSelector( vgui::Panel* parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	m_pItemsList = new vgui::PanelListPanel( this, "listpanel_items" );
	m_pItemsList->SetFirstColumnWidth( 0 );

	m_iSelectedClass = TF_CLASS_UNDEFINED;
	m_iSelectedSlot = TF_LOADOUT_SLOT_INVALID;
	m_iSelectedPage = 0;
	m_iSelectedPreset = 0;

	m_pItemButtonKeys = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFItemSelector::~CTFItemSelector()
{
	if ( m_pItemButtonKeys )
	{
		m_pItemButtonKeys->deleteThis();
	}
}


void CTFItemSelector::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/LoadoutPanel_ItemSelector.res" );

	m_pItemsList->GetScrollbar()->SetWide( XRES( 3 ) );
	m_pItemsList->SetBorder( NULL );
	m_pItemsList->SetBgColor( Color( 0, 0, 0, 0 ) );

	m_pPreviousPageButton = dynamic_cast<vgui::Button *>( FindChildByName( "PreviousPageButton" ) );
	m_pNextPageButton = dynamic_cast<vgui::Button *>( FindChildByName( "NextPageButton" ) );
	m_pPageCountLabel = dynamic_cast<vgui::Label*>( FindChildByName("CountLabel") );

	// Clear out all previously created item buttons.
	int i;
	for ( i = 0; i < m_vecItemButtonsGrid.Count(); i++ )
	{
		m_vecItemButtonsGrid[i]->MarkForDeletion();
	}

	m_vecItemButtonsGrid.RemoveAll();

	// Avoid executing these instructions if we don't need to.
	if ( tf2c_loadout_grid.GetBool() || m_bItemGridDEPRECATED )
	{
		int x, y;
		x = y = 0;

		int iNumItemButtons = ( m_iItemXLimit * m_iItemYLimit );
		for ( i = 0; i < iNumItemButtons; i++ )
		{
			m_vecItemButtonsGrid.AddToTail( new CTFItemButton( this, "ItemIcons", "DUK" ) );
			m_vecItemButtonsGrid[i]->MakeReadyForUse();
			m_vecItemButtonsGrid[i]->SetPos( ( GetWide() / 2 ) + m_iItemXOffset + ( x * g_pVGuiSchemeManager->GetProportionalScaledValue( m_iItemXSpacing ) ), m_iItemYPos + ( y * g_pVGuiSchemeManager->GetProportionalScaledValue( m_iItemYSpacing ) ) );

			// Set the position that corresponds to each item button.
			if ( ++x > ( m_iItemXLimit - 1 ) )
			{
				x = 0;
				if ( ++y > ( m_iItemYLimit - 1 ) )
				{
					y = 0;
				}
			}
		}

		// Setup UI navigation.
		for ( i = 0; i < iNumItemButtons; i++ )
		{
			if ( i > 0 )
			{
				m_vecItemButtonsGrid[i]->SetNavUp( m_vecItemButtonsGrid[Max( 0, i - m_iItemXLimit )] );
				m_vecItemButtonsGrid[i]->SetNavLeft( m_vecItemButtonsGrid[Max( 0, i - 1 )] );
			}

			if ( i < iNumItemButtons - 1 )
			{
				m_vecItemButtonsGrid[i]->SetNavDown( m_vecItemButtonsGrid[Min( iNumItemButtons - 1, i + m_iItemXLimit )] );
				m_vecItemButtonsGrid[i]->SetNavRight( m_vecItemButtonsGrid[Min( iNumItemButtons - 1, i + 1 )] );
			}
		}
	}

	// Reset if our layout resets.
	m_iSelectedClass = TF_CLASS_UNDEFINED;
	m_iSelectedSlot = TF_LOADOUT_SLOT_INVALID;
	m_iSelectedPage = 0;
	m_iSelectedPreset = 0;
}


void CTFItemSelector::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char* strButtonName = tf2c_loadout_grid.GetBool() && !m_bItemGridDEPRECATED ? "button_kv_loadoutgrid" : "button_kv";

	KeyValues *pButtonData = inResourceData->FindKey( strButtonName );
	if ( pButtonData )
	{
		if ( m_pItemButtonKeys )
		{
			m_pItemButtonKeys->deleteThis();
		}

		m_pItemButtonKeys = new KeyValues( strButtonName );
		pButtonData->CopySubkeys( m_pItemButtonKeys );
	}
}


void CTFItemSelector::OnCommand( const char *command )
{
	CTFLoadoutPanel *pLoadoutPanel = static_cast<CTFLoadoutPanel *>( GetParent() );
	if ( !pLoadoutPanel )
		return;

	if ( !V_stricmp( command, "back" ) )
	{
		CloseItemSelector();
	}
	else if ( !V_stricmp( command, "prevpage" ) )
	{
		--m_iSelectedPage;

		if ( m_iSelectedPage < 0 )
		{
			m_iSelectedPage = 0;
		}

		RefreshItemButtons();
	}
	else if ( !V_stricmp( command, "nextpage" ) )
	{
		++m_iSelectedPage;

		int iLastPage = m_vecPages.Count() - 1;
		if ( m_iSelectedPage > iLastPage )
		{
			m_iSelectedPage = iLastPage;
		}

		RefreshItemButtons();
	}
	else if ( !V_strncmp( command, "setpage", 7 ) )
	{
		const char *sChar = strchr( command, ' ' );
		if ( sChar )
		{
			sChar = strchr( sChar + 1, ' ' );
			if ( sChar )
			{
				m_iSelectedPage = atoi( sChar );

				if ( m_iSelectedPage < 0 )
				{
					m_iSelectedPage = 0;
				}

				int iLastPage = m_vecPages.Count() - 1;
				if ( m_iSelectedPage > iLastPage )
				{
					m_iSelectedPage = iLastPage;
				}
			}
		}
	}
	else if ( !V_strncmp( command, "loadout", 7 ) )
	{
		const char *sChar = strchr( command, ' ' );
		if ( sChar )
		{
			sChar = strchr( sChar + 1, ' ' );
			if ( sChar )
			{
				pLoadoutPanel->SetItemPreset( m_iSelectedClass, m_iSelectedSlot, atoi( sChar + 1 ) );

				CloseItemSelector();
			}
		}

		return;
	}

	pLoadoutPanel->OnCommand( command );
}


void CTFItemSelector::OnKeyCodePressed( KeyCode code )
{
	if ( !IsVisible() )
		return;

	if ( code == KEY_LEFT ||  code == KEY_XSTICK1_LEFT || code == KEY_XBUTTON_LEFT_SHOULDER )
	{
		OnCommand( "prevpage" );
	}
	else if ( code == KEY_RIGHT || code == KEY_XSTICK1_RIGHT || code == KEY_XBUTTON_RIGHT_SHOULDER )
	{
		OnCommand( "nextpage" );
	}
	else if ( code == KEY_XBUTTON_B )
	{
		CloseItemSelector();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


void CTFItemSelector::OpenItemSelector( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	CTFLoadoutPanel *pLoadoutPanel = static_cast<CTFLoadoutPanel *>( GetParent() );
	if ( !pLoadoutPanel )
		return;

	SetVisible( true );

	// If the class, slot, or preset is different, recalcuate.
	if ( m_iSelectedClass != iClass || m_iSelectedSlot != iSlot || m_iSelectedPreset != iPreset )
	{
		m_iSelectedClass = iClass;
		m_iSelectedSlot = iSlot;
		m_iSelectedPage = 0;
		m_iSelectedPreset = iPreset;

		CreateSortedItems();
		RefreshItemButtons();
	}
	else if ( m_iSelectedPage != 0 )
	{
		m_iSelectedPage = 0;
		RefreshItemButtons( false );
	}
#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
	else if ( tf2c_loadout_grid.GetBool() || m_bItemGridDEPRECATED )
	{
		SetPanelFocus( &GetFocusNavGroup(), m_vecItemButtonsGrid[0], pLoadoutPanel->UseController() );
	}
#endif

	const wchar_t *pszLocalized = g_pVGuiLocalize->Find( szLocalizedLoadoutSlots[iClass][iSlot] );
	if ( pszLocalized )
	{
		SetDialogVariable( "selectedslot", pszLocalized );
	}
	else
	{
		SetDialogVariable( "selectedslot", szLocalizedLoadoutSlots[iClass][iSlot] );
	}

	pLoadoutPanel->OnItemSelectorOpened();
}


void CTFItemSelector::CloseItemSelector( void )
{
	CTFLoadoutPanel *pLoadoutPanel = static_cast<CTFLoadoutPanel *>( GetParent() );
	if ( !pLoadoutPanel )
		return;

	SetVisible( false );
	pLoadoutPanel->OnItemSelectorClosed();
}


void CTFItemSelector::CreateSortedItems( void )
{
	CTFLoadoutPanel *pLoadoutPanel = static_cast<CTFLoadoutPanel *>( GetParent() );
	if ( !pLoadoutPanel )
		return;

	m_pItemsList->DeleteAllItems();
	m_vecPages.Purge();

	// First line up the items for this slot into the order we want them in, then into pages.
	int iNumPresetItems = GetTFInventory()->GetNumItems( m_iSelectedClass, m_iSelectedSlot );
	int iNumPages = m_vecPages.AddToTail();
	m_vecPages[iNumPages].vecItems.EnsureCapacity( iNumPresetItems );

	int iPreset;
	CEconItemView *pItem;
	int iItems = 0;
	for ( int i = -1; i < iNumPresetItems; i++ )
	{
		iPreset = i == -1 ? m_iSelectedPreset : i;
		pItem = GetTFInventory()->GetItem( m_iSelectedClass, m_iSelectedSlot, iPreset );
		if ( pItem && i != m_iSelectedPreset )
		{
			ItemPreset_t item;
			item.iPreset = iPreset;
			item.pItem = pItem;
			m_vecPages[iNumPages].vecItems.AddToTail( item );

			if ( ( tf2c_loadout_grid.GetBool() || m_bItemGridDEPRECATED ) && ( ++iItems >= ( m_iItemXLimit * m_iItemYLimit ) ) )
			{
				iItems = 0;

				// If we're at the end of the road, don't add another page.
				if ( i != ( iNumPresetItems - 1 ) )
				{
					iNumPages = m_vecPages.AddToTail();
				}
			}
		}
	}

	SetDialogVariable( "numitems", iNumPresetItems );
	SetDialogVariable( "numpages", iNumPages + 1 );
}


void CTFItemSelector::RefreshItemButtons( bool bRecreateList /*= true*/ )
{
	CTFLoadoutPanel *pLoadoutPanel = static_cast<CTFLoadoutPanel *>( GetParent() );
	if ( !pLoadoutPanel )
		return;

	if ( bRecreateList )
	{
		m_pItemsList->DeleteAllItems();
	}

	bool bIsLoadoutGrid = tf2c_loadout_grid.GetBool() || m_bItemGridDEPRECATED;

	int i;
	int iNumItemButtons = m_vecItemButtonsGrid.Count();
	int iNumPresetItems = m_vecPages[m_iSelectedPage].vecItems.Count();
	int iPreset;
	CEconItemView *pItem;
	CTFItemButton *pItemButton;
	bool bEquipped;
	for ( i = 0; i < ( bIsLoadoutGrid ? iNumItemButtons : iNumPresetItems ); i++ )
	{
		if ( i >= iNumPresetItems )
		{
			// This button won't have an item in it, make it invisible.
			m_vecItemButtonsGrid[i]->SetVisible( false );
			continue;
		}

		iPreset = m_vecPages[m_iSelectedPage].vecItems[i].iPreset;
		pItem = m_vecPages[m_iSelectedPage].vecItems[i].pItem;
		if ( pItem )
		{
			pItemButton = bIsLoadoutGrid ? m_vecItemButtonsGrid[i] : new CTFItemButton( this, "ItemIcons", "DUK" );
			if ( m_pItemButtonKeys )
			{
				pItemButton->ApplySettings( m_pItemButtonKeys );
			}

			bEquipped = ( m_iSelectedPage == 0 && i == 0 );

#ifdef SET_EQUIPPED_ITEM_AS_SELECTED
			pItemButton->SetItem( pItem );
#else
			pItemButton->SetItem( pItem, false, bEquipped );
#endif
			pItemButton->SetLoadoutSlot( m_iSelectedSlot, iPreset );

#ifdef SET_EQUIPPED_ITEM_AS_SELECTED
			if ( bEquipped )
			{
				pItemButton->SetEnabled( false );
				pItemButton->SetSelected( true );
			}
#endif
#ifdef ITEM_ACKNOWLEDGEMENTS
			else
			{
				pItemButton->SetSelected( !GetTFInventory()->ItemIsAcknowledged( pItem->GetItemDefIndex() ) );
			}
#endif

			if ( bIsLoadoutGrid )
			{
				pItemButton->SetVisible( true );
			}
			else if ( bRecreateList )
			{
				// Add them to the list.
				m_pItemsList->AddItem( NULL, pItemButton );
			}
		}
	}

	if ( bIsLoadoutGrid )
	{
		m_pItemsList->SetVisible( false );

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
		SetPanelFocus( &GetFocusNavGroup(), m_vecItemButtonsGrid[0], pLoadoutPanel->UseController() );
#endif

		// Updates the page changing buttons.
		if ( m_pPreviousPageButton )
			m_pPreviousPageButton->SetEnabled( true );

		if ( m_pNextPageButton )
			m_pNextPageButton->SetEnabled( true );

		if ( m_pPreviousPageButton && m_iSelectedPage == 0 )
			m_pPreviousPageButton->SetEnabled( false );
		
		if ( m_pNextPageButton && m_iSelectedPage == m_vecPages.Count() - 1 )
			m_pNextPageButton->SetEnabled( false );

		if (m_pPageCountLabel)
			m_pPageCountLabel->SetVisible( true );
	}
	else
	{
		m_pItemsList->MoveScrollBarToTop();

		if ( bRecreateList )
		{
			// Assume here that the item count of this list is the same as our item count.
			vgui::Panel *pItemButton;
			for ( i = 0; i < iNumPresetItems; i++ )
			{
				pItemButton = m_pItemsList->GetItemPanel( i );
				if ( i > 0 )
				{
					pItemButton->SetNavUp( m_pItemsList->GetItemPanel( Max( 0, i - 1 ) ) );
					pItemButton->SetNavLeft( m_pItemsList->GetItemPanel( Max( 0, i - 1 ) ) );
				}

				if ( i < iNumPresetItems - 1 )
				{
					pItemButton->SetNavDown( m_pItemsList->GetItemPanel( Min( iNumPresetItems - 1, i + 1 ) ) );
					pItemButton->SetNavRight( m_pItemsList->GetItemPanel( Min( iNumPresetItems - 1, i + 1 ) ) );
				}
			}
		}

#ifdef STAGING_LOADOUT_CONTROLLER_SUPPORT
		SetPanelFocus( &GetFocusNavGroup(), m_pItemsList->GetItemPanel( 0 ), pLoadoutPanel->UseController() );
#endif

		// Hides them, since we're not using a grid.
		if ( m_pPreviousPageButton )
			m_pPreviousPageButton->SetVisible( false );

		if ( m_pNextPageButton )
			m_pNextPageButton->SetVisible( false );

		if( m_pPageCountLabel )
			m_pPageCountLabel->SetVisible( false );
	}

	SetDialogVariable( "curpage", m_iSelectedPage + 1 );
}