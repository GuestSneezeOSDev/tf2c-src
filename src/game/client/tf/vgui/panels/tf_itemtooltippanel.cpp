#include "cbase.h"
#include "tf_itemtooltippanel.h"
#include "tf_mainmenupanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advbuttonbase.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemToolTipPanel::CTFItemToolTipPanel( Panel *parent, const char *panelName ) : CTFToolTipPanel( parent, panelName )
{
	m_pTitle = new CExLabel( this, "TitleLabel", "Title" );
	m_pClassName = new CExLabel( this, "ClassNameLabel", "ClassName" );
	m_pAttributeText = new CExLabel( this, "AttributeLabel", "Attribute" );
	for ( int i = 0; i < 20; i++ )
	{
		m_pAttributes.AddToTail( new CExLabel( this, VarArgs( "attribute_%d", i ), "Attribute" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFItemToolTipPanel::~CTFItemToolTipPanel()
{

}


void CTFItemToolTipPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pTitle )
	{
		m_colorTitle = m_pTitle->GetFgColor();
	}
}


void CTFItemToolTipPanel::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	// Apply settings from template to all attribute strings.
	if ( pChild == m_pAttributeText )
	{
		for ( int i = 0; i < m_pAttributes.Count(); i++ )
		{
			m_pAttributes[i]->ApplySettings( pInResourceData );
		}
	}

	BaseClass::OnChildSettingsApplied( pInResourceData, pChild );
}


void CTFItemToolTipPanel::ShowToolTip( CEconItemDefinition *pItemData, Panel *pFocusPanel /*= NULL*/ )
{
	Show();

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	if ( !pScheme )
		return;

	// Set item name.
	if ( m_pTitle )
	{
		m_pTitle->SetText( pItemData->GenerateLocalizedFullItemName() );

		// Set the color according to quality.	
		const char *pszColor = EconQuality_GetColorString( pItemData->item_quality );
		if ( pszColor )
		{
			m_pTitle->SetFgColor( pScheme->GetColor( pszColor, m_colorTitle ) );
		}

		if ( pItemData->item_name_color != Color(0, 0, 0, 0) )
		{
			m_pTitle->SetFgColor( pItemData->item_name_color );
		}
	}

	// Set item type name.
	if ( m_pClassName )
	{
		m_pClassName->SetText( pItemData->item_type_name );
	}

	// List atrributes.
	int i;
	for ( i = 0; i < m_pAttributes.Count(); i++ )
	{
		m_pAttributes[i]->SetVisible( false );
	}

	if ( m_pAttributeText )
	{
		bool bSkip = false;
		for ( i = 0; i <= pItemData->attributes.Count() && i < m_pAttributes.Count(); i++ )
		{
			CExLabel *pLabel = m_pAttributes[i];

			if ( i == pItemData->attributes.Count() )
			{
				// Show item description at the end.
				if ( pItemData->item_description[0] == '\0' )
					continue;

				pLabel->SetText( pItemData->item_description );

				pLabel->SetFgColor( pScheme->GetColor( "ItemAttribNeutral", COLOR_WHITE ) );
				pLabel->SetVisible( true );
			}
			else
			{
				if ( bSkip ) 
					continue;

				CEconItemAttribute *pAttribute = &pItemData->attributes[i];
				CEconAttributeDefinition *pStatic = pAttribute->GetStaticData();
				if ( !pStatic || pStatic->hidden )
					continue;

				if ( pStatic->hidden_separator )
				{
					bSkip = true;
					continue;
				}

				if ( !pStatic->description_string || !pStatic->description_string[0] )
					continue;

				const wchar_t *pszLocalized = g_pVGuiLocalize->Find( pStatic->description_string );
				wchar_t wszDescriptionString[128] = {}; // just to be safe
				if ( !pszLocalized || !pszLocalized[0] )
				{
					// there's probably a better way to do this - azzy
					g_pVGuiLocalize->ConvertANSIToUnicode(pStatic->description_string, wszDescriptionString, sizeof(wszDescriptionString));
					pszLocalized = wszDescriptionString;
				}

				float flValue = pAttribute->value;

				switch ( pStatic->description_format )
				{
					case ATTRIB_FORMAT_PERCENTAGE:
						flValue = flValue - 1.0f;
						flValue *= 100.0f;
						break;
					case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
						flValue = 1.0f - flValue;
						flValue *= 100.0f;
						break;
					case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
						flValue *= 100.0f;
						break;
				}

				wchar_t wszValue[32];
				V_swprintf_safe( wszValue, L"%.f", flValue );

				wchar_t wszAttrib[128];
				g_pVGuiLocalize->ConstructString( wszAttrib, sizeof( wszAttrib ), pszLocalized, 1, wszValue );

				pLabel->SetText( wszAttrib );

				Color attrcolor;
				switch ( pStatic->effect_type )
				{
					case ATTRIB_EFFECT_POSITIVE:
						attrcolor = pScheme->GetColor( "ItemAttribPositive", COLOR_WHITE );
						break;
					case ATTRIB_EFFECT_NEGATIVE:
						attrcolor = pScheme->GetColor( "ItemAttribNegative", COLOR_WHITE );
						break;			
					case ATTRIB_EFFECT_NEUTRAL:
					default:
						attrcolor = pScheme->GetColor( "ItemAttribNeutral", COLOR_WHITE );
						break;
				}

				if ( pStatic->custom_color != Color(0, 0, 0, 0) )
					attrcolor = pStatic->custom_color;

				pLabel->SetFgColor( attrcolor );
				pLabel->SetVisible( true );
			}
		}
	}

	m_pFocusPanel = pFocusPanel;

	InvalidateLayout( true );
}


void CTFItemToolTipPanel::HideToolTip( void )
{
	Hide();
}


void CTFItemToolTipPanel::AdjustToolTipSize( void )
{
	// Place attributes one under the other.
	int x, y;
	m_pAttributeText->GetPos( x, y );

	int iTotalHeight = y;
	for ( int i = 0; i < m_pAttributes.Count(); i++ )
	{
		CExLabel *pLabel = m_pAttributes[i];
		if ( pLabel->IsVisible() )
		{
			pLabel->SetPos( x, iTotalHeight );

			int twide, ttall;
			pLabel->GetTextImage()->GetContentSize( twide, ttall );
			pLabel->SetTall( ttall );

			iTotalHeight += ttall;
		}
	}

	// Set the tooltip size based on attribute list size.
	SetTall( iTotalHeight + YRES( 10 ) );
}
