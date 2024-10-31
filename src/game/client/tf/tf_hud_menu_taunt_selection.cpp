//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_hud_menu_taunt_selection.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "tf_inventory.h"

#ifdef ITEM_TAUNTING
using namespace vgui;

DECLARE_HUDELEMENT( CHudMenuTauntSelection );

CHudMenuTauntSelection::CHudMenuTauntSelection( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuTauntSelection" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_bShow = false;

	for ( int i = 0; i < ARRAYSIZE( m_pItemPanels ); i++ )
	{
		m_pItemPanels[i] = new CItemModelPanel( this, VarArgs( "TauntModelPanel%d", i + 1 ) );
	}
}


void CHudMenuTauntSelection::ApplySchemeSettings( IScheme *pSceme )
{
	BaseClass::ApplySchemeSettings( pSceme );

	LoadControlSettings( "Resource/UI/HudMenuTauntSelection.res" );
}


bool CHudMenuTauntSelection::ShouldDraw( void )
{
	if ( !m_bShow )
		return false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		m_bShow = false;
		return false;
	}

	return CHudElement::ShouldDraw();
}


void CHudMenuTauntSelection::SetVisible( bool bVisible )
{
	if ( bVisible )
	{
		const char *key = engine->Key_LookupBinding( "taunt" );
		if ( !key )
		{
			key = "< not bound >";
		}
		SetDialogVariable( "taunt", key );

		key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}
		SetDialogVariable( "lastinv", key );

		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

		// Set the proper taunt icons based on our loadout.
		for ( int i = 0; i < ARRAYSIZE( m_pItemPanels ); i++ )
		{
			CEconItemDefinition *pItemDef = NULL;

			C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pPlayer )
			{
				// Well, this is a bit awkward. Due to how inventory works taunts start at index 1.
				int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
				CEconItemView *pItem = GetTFInventory()->GetItem( iClass, TF_LOADOUT_SLOT_TAUNT, i + 1 );
				if ( pItem )
				{
					pItemDef = pItem->GetStaticData();
				}
			}

			m_pItemPanels[i]->SetItem( pItemDef );

			if ( pItemDef )
			{
				m_pItemPanels[i]->SetBorder( pScheme->GetBorder( g_szItemBorders[pItemDef->item_quality][0] ) );
			}
			else
			{
				m_pItemPanels[i]->SetBorder( pScheme->GetBorder( g_szItemBorders[0][0] ) );
			}
		}
	}

	BaseClass::SetVisible( bVisible );
}


void CHudMenuTauntSelection::Reset( void )
{
	m_bShow = false;
}


int CHudMenuTauntSelection::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1;

	if ( !down )
		return 1;

	int iSlot = 0;

	switch ( keynum )
	{
		case KEY_1:
		case KEY_PAD_1:
			iSlot = 1;
			break;
		case KEY_2:
		case KEY_PAD_2:
			iSlot = 2;
			break;
		case KEY_3:
		case KEY_PAD_3:
			iSlot = 3;
			break;
		case KEY_4:
		case KEY_PAD_4:
			iSlot = 4;
			break;
		case KEY_5:
		case KEY_PAD_5:
			iSlot = 5;
			break;
		case KEY_6:
		case KEY_PAD_6:
			iSlot = 6;
			break;
		case KEY_7:
		case KEY_PAD_7:
			iSlot = 7;
			break;
		case KEY_8:
		case KEY_PAD_8:
			iSlot = 8;
			break;
		default:
			if ( pszCurrentBinding && FStrEq( pszCurrentBinding, "lastinv" ) )
			{
				m_bShow = false;
				return 0;
			}

			return 1;	// key not handled
	}

	m_bShow = false;
	engine->ExecuteClientCmd( VarArgs( "taunt %d", iSlot ) );
	return 0;
}


void CHudMenuTauntSelection::TauntDown( const CCommand &args )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	// If the taunt is menu is already open, do the weapon taunt.
	if ( m_bShow )
	{
		m_bShow = false;
		engine->ExecuteClientCmd( "taunt" );
		return;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		// Interrupt a hold taunt.
		engine->ExecuteClientCmd( "taunt" );
		return;
	}

	// If we don't have any taunts equpped, just do a weapon taunt.
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( GetTFInventory()->GetItem( iClass, TF_LOADOUT_SLOT_TAUNT, 1 ) == NULL )
	{
		engine->ExecuteClientCmd( "taunt" );
		return;
	}

	m_bShow = !m_bShow;
}


void CHudMenuTauntSelection::TauntUp( const CCommand &args )
{
	// Do nothing.
}
#endif
