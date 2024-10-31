#include "cbase.h"
#include "tf_advitembutton.h"
#include "tf_inventory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


#define ADVITEMBUTTON_DEFAULT_BG "AdvRoundedButtonDisabled"
#define ADVITEMBUTTON_ARMED_BG "AdvRoundedButtonArmed"
#define ADVITEMBUTTON_DEPRESSED_BG "AdvRoundedButtonDepressed"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemButton::CTFItemButton( Panel *parent, const char *panelName, const char *text ) : CTFButton( parent, panelName, text )
{
	m_pItemPanel = new CItemModelPanel( this, "ModelPanel" );
	m_pItem = NULL;
	m_iLoadoutSlot = TF_LOADOUT_SLOT_PRIMARY;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFItemButton::~CTFItemButton()
{
	if ( guiroot && guiroot->GetItemToolTip()->GetFocusedPanel() == this )
	{
		guiroot->HideItemToolTip();
	}
}


void CTFItemButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetArmedSound( "#ui/item_info_mouseover.wav" );

	// Don't want to darken weapon images.
	m_colorImageDefault = COLOR_WHITE;
	m_colorImageArmed = COLOR_WHITE;

	m_pItemPanel->SetMouseInputEnabled( false );
}


void CTFItemButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pModelPanelKeys = inResourceData->FindKey( "ModelPanel" );
	if ( pModelPanelKeys )
	{
		m_pItemPanel->ApplySettings( pModelPanelKeys );
	}
}


void CTFItemButton::PerformLayout()
{
	BaseClass::PerformLayout();

	// Position icon at the center.
	//m_pItemPanel->SetBounds( 0, 0, GetWide(), GetTall() );
}


void CTFItemButton::ShowToolTip( bool bShow )
{
	// Using a custom tooltip.
	if ( !m_pItem )
		return;

	CEconItemDefinition *pItemDef = m_pItem->GetStaticData();
	if ( pItemDef )
	{
		if ( bShow )
		{
			guiroot->ShowItemToolTip( pItemDef, this );
			m_pItemPanel->SetNameVisibility( !m_bHideNameDuringTooltip );
		}
		else
		{
			guiroot->HideItemToolTip();
			m_pItemPanel->SetNameVisibility( true );
		}
	}
}


void CTFItemButton::DoClick( void )
{
	BaseClass::DoClick();

#ifdef ITEM_ACKNOWLEDGEMENTS
	if ( GetTFInventory()->MarkItemAsAcknowledged( m_pItem ? m_pItem->GetItemDefIndex() : 0 ) )
	{
		SetSelected( false );
	}
#endif
}


void CTFItemButton::OnKeyCodeReleased( KeyCode keycode )
{
	// Controller support.
	vgui::KeyCode code = GetBaseButtonCode( keycode );
	if ( _buttonFlags.IsFlagSet( BUTTON_KEY_DOWN ) )
	{
		SetArmed( true );
	}
	else if ( code != KEY_XBUTTON_A && code != KEY_XBUTTON_START )
	{
		BaseClass::OnKeyCodeReleased( keycode );
	}
}


void CTFItemButton::SetItem( CEconItemView *pItem, bool bUseDropSound /*= false*/, bool bEquipped /*= false*/ )
{
	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return;

	m_pItem = pItem;
	m_pItemPanel->SetItem( pItemDef, -1, false, bEquipped );

	SetDepressedSound( NULL );

	// Set the weapon sound from schema.
	if ( !bUseDropSound )
	{
		if ( pItemDef->mouse_pressed_sound[0] != '\0' )
		{
			SetDepressedSound( pItemDef->mouse_pressed_sound );
		}
	}
	else
	{
		if ( pItemDef->drop_sound[0] != '\0' )
		{
			SetDepressedSound( pItemDef->drop_sound );
		}
	}

	SetReleasedSound( NULL );
}


void CTFItemButton::SetLoadoutSlot( ETFLoadoutSlot iSlot, int iPreset )
{
	m_iLoadoutSlot = iSlot;

	SetCommand( VarArgs( "loadout %d %d", iSlot, iPreset ) );
}
