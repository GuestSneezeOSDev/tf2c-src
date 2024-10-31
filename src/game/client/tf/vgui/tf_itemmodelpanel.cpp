//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_itemmodelpanel.h"
#include <vgui/ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEmbeddedItemModelPanel::CEmbeddedItemModelPanel( Panel *parent, const char* name ) : EditablePanel( parent, name )
{
	m_iWeaponTextureID = -1;
	m_pItemDef = NULL;
}


void CEmbeddedItemModelPanel::Paint( void )
{
	if ( !m_pItemDef )
		return;

	// Normal is 128x128, large is 512x512.
	int inv_w, inv_h;
	inv_w = m_pItemDef->image_inventory_size_w * 4;
	inv_h = m_pItemDef->image_inventory_size_h * 4;

	float texx, texy, texw, texh;
	texw = inv_w / 512.0f;
	texh = inv_h / 512.0f;
	texx = ( 1.0f - texw ) * 0.5f;
	texy = ( 1.0f - texh ) * 0.5f;

	int x, y, w, h;
	x = 0;
	y = 0;
	GetSize( w, h );

	// Fit it to fill up the whole height.
	float flRatio = ( (float)inv_w / (float)inv_h );
	if ( flRatio != ( (float)w / (float)h ) )
	{
		int center = x + ( w * 0.5f );
		w = h * flRatio;
		x = center - ( w * 0.5f );
	}

	surface()->DrawSetTexture( m_iWeaponTextureID );
	surface()->DrawSetColor( COLOR_WHITE );
	surface()->DrawTexturedSubRect( x, y, x + w, y + h, texx, texy, texx + texw, texy + texh );
}


void CEmbeddedItemModelPanel::SetItem( CEconItemDefinition *pItemDef )
{
	m_pItemDef = pItemDef;
	if ( !m_pItemDef )
		return;

	char szCorrectPath[MAX_PATH];
	V_sprintf_safe( szCorrectPath, "%s_large", m_pItemDef->image_inventory );

	m_iWeaponTextureID = surface()->DrawGetTextureId( szCorrectPath );
	if ( m_iWeaponTextureID == -1 )
	{
		m_iWeaponTextureID = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( m_iWeaponTextureID, szCorrectPath, true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CItemModelPanel::CItemModelPanel( Panel *parent, const char* name ) : EditablePanel( parent, name )
{
	m_pMainContainer = new EditablePanel( this, "MainContentsContainer" );
	m_pModelPanel = new CEmbeddedItemModelPanel( m_pMainContainer, "itemmodelpanel" );
	m_pNameLabel = new CExLabel( m_pMainContainer, "namelabel", "text" );
	m_pAttribLabel = new CExLabel( m_pMainContainer, "attriblabel", "" );
	m_pEquippedLabel = new CExLabel( m_pMainContainer, "equippedlabel", "" );
	m_pSlotID = new CExLabel( m_pMainContainer, "SlotID", "" );
	m_pItemDef = NULL;
}


void CItemModelPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/econ/itemmodelpanel.res" );

	m_pNameLabel->SetCenterWrap( true );

	m_pAttribLabel->SetVisible( false );
	m_pEquippedLabel->SetVisible( false );

	m_pSlotID->SetContentAlignment( Label::a_northeast );
	m_pSlotID->SetVisible( false );
	m_pSlotID->SetZPos( m_pModelPanel->GetZPos() + 1 );

	m_hLargeFont = pScheme->GetFont( "ItemFontNameLarge", true );
	m_hSmallFont = pScheme->GetFont( "ItemFontNameSmall", true );
	m_hSmallestFont = pScheme->GetFont( "ItemFontNameSmallest", true );
	m_hLargerFont = pScheme->GetFont( "ItemFontNameLarger", true );
	m_clrDefaultNameColor = m_pNameLabel->GetFgColor();
}


void CItemModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}


void CItemModelPanel::PerformLayout( void )
{
	// Position item model.
	int w, h;
	GetSize( w, h );

	int iModelWide = m_iModelWide ? Min( m_iModelWide, w ) : w;
	int iModelTall = m_iModelTall ? Min( m_iModelTall, h ) : h;
	int iModelX = m_bModelCenterX ? ( w - iModelWide ) / 2 : m_iModelXPos;
	int iModelY = m_bModelCenterY ? ( w - iModelTall ) / 2 : m_iModelYPos;
	m_pModelPanel->SetBounds( iModelX, iModelY, iModelWide, iModelTall );

	if ( m_bModelOnly )
		return;

	// Position item name.
	int iTextXPos = ( m_iTextXPos || m_iTextWide ) ? m_iTextXPos : YRES( 5 );
	int iTextW = m_iTextWide ? m_iTextWide : w - YRES( 5 ) * 2;

	if ( m_iForceTextSize > ITEMMODEL_TEXT_AUTO && m_iForceTextSize < ITEMMODEL_TEXT_COUNT )
	{
		switch ( m_iForceTextSize )
		{
			case ITEMMODEL_TEXT_LARGE:
				m_pNameLabel->SetFont( m_hLargeFont );
				break;
			case ITEMMODEL_TEXT_SMALL:
				m_pNameLabel->SetFont( m_hSmallFont );
				break;
			case ITEMMODEL_TEXT_SMALLEST:
				m_pNameLabel->SetFont( m_hSmallestFont );
				break;
			case ITEMMODEL_TEXT_LARGER:
				m_pNameLabel->SetFont( m_hLargerFont );
				break;
		}
	}
	else
	{
		// Auto-detect font size.
		wchar_t wszName[256];
		m_pNameLabel->GetText( wszName, ARRAYSIZE( wszName ) );

		// Try large font first.
		if ( UTIL_ComputeStringWidth( m_hLargeFont, wszName ) > iTextW )
		{
			// Then small font.
			if ( UTIL_ComputeStringWidth( m_hSmallFont, wszName ) > iTextW )
			{
				// And then fall back to the smallest font.
				m_pNameLabel->SetFont( m_hSmallestFont );
			}
			else
			{
				m_pNameLabel->SetFont( m_hSmallFont );
			}
		}
		else
		{
			m_pNameLabel->SetFont( m_hLargeFont );
		}
	}

	if ( m_bTextCenterX )
	{
		m_pNameLabel->SetBounds( 0, m_iTextYPos, iTextW, h );
		m_pNameLabel->SetContentAlignment( Label::a_north );
	}
	else if ( m_iTextYPos )
	{
		m_pNameLabel->SetBounds( iTextXPos, m_iTextYPos, iTextW, h );
		m_pNameLabel->SetContentAlignment( Label::a_north );
	}
	else if ( m_bTextCenter )
	{
		m_pNameLabel->SetBounds( iTextXPos, 0, iTextW, h );
		m_pNameLabel->SetContentAlignment( Label::a_center );
	}
	else
	{
		int iOffset = ( m_iTextYOffset ) ? m_iTextYOffset : YRES( 8 );
		m_pNameLabel->SetBounds( iTextXPos, 0, iTextW, h - iOffset - m_iHPadding );
		m_pNameLabel->SetContentAlignment( Label::a_south );
	}

	// BUG: We need to do this twice to make the text aligned properly.
	m_pNameLabel->InvalidateLayout( true );
	m_pNameLabel->InvalidateLayout( true );
}


void CItemModelPanel::SetItem( CEconItemDefinition *pItemDefinition, int iSlot /*= -1*/, bool bNoAmmo /*= false*/, bool bEquipped /*= false*/ )
{
	m_pItemDef = pItemDefinition;

	if ( m_pItemDef )
	{
		m_pModelPanel->SetItem( m_pItemDef );
		m_pNameLabel->SetVisible( !m_bModelOnly );
		
		if ( bNoAmmo )
		{
			m_pNameLabel->SetText( "#TF_OUT_OF_AMMO" );
			m_pNameLabel->SetFgColor( Color( 255, 0, 0, 255 ) );
		}
		else
		{
			m_pNameLabel->SetText( m_pItemDef->GenerateLocalizedFullItemName() );
			
			if ( m_bStandardTextColor )
			{
				m_pNameLabel->SetFgColor( m_clrDefaultNameColor );
			}
			else
			{
				// Set the color according to quality.	
				const char *pszColor = EconQuality_GetColorString( m_pItemDef->item_quality );
				if ( pszColor )
				{
					m_pNameLabel->SetFgColor( scheme()->GetIScheme( GetScheme() )->GetColor( pszColor, COLOR_WHITE ) );
				}

				if (m_pItemDef->item_name_color != Color(0, 0, 0, 0))
				{
					m_pNameLabel->SetFgColor(m_pItemDef->item_name_color);
				}
			}
		}

		m_pEquippedLabel->SetVisible( bEquipped );

		if ( iSlot != -1 && !m_bModelOnly )
		{
			m_pSlotID->SetVisible( true );
			m_pSlotID->SetText( VarArgs( "%d", iSlot + 1 ) );
		}
		else
		{
			m_pSlotID->SetVisible( false );
		}
	}
	else
	{
		m_pModelPanel->SetItem( NULL );
		m_pNameLabel->SetVisible( false );
		m_pEquippedLabel->SetVisible( true );
		m_pSlotID->SetVisible( false );
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Slot number used for weapon select HUD.
//-----------------------------------------------------------------------------
void CItemModelPanel::SetNumberSettings( int x, int y, vgui::HFont hFont, Color col, int a )
{
	m_pSlotID->SetBounds( 0, y + YRES( 5 ), GetWide() - YRES( 5 ) - x, YRES( 10 ) );
	m_pSlotID->SetFont( hFont );
	m_pSlotID->SetFgColor( col );
	m_pSlotID->SetAlpha( a );
}


void CItemModelPanel::SetNameVisibility( bool bVisibility )
{
	m_pNameLabel->SetVisible( bVisibility );
}
