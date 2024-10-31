#include "cbase.h"
#include "tf_tooltippanel.h"
#include "tf_mainmenupanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advbuttonbase.h"
#include <vgui_controls/TextImage.h>
#include <vgui/ISurface.h>

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TOOLTIP_XOFFSET		( iTipW / 2 );
#define TOOLTIP_YOFFSET		( 16 )

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFToolTipPanel::CTFToolTipPanel( vgui::Panel* parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	m_pText = new CExLabel( this, "TextLabel", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFToolTipPanel::~CTFToolTipPanel()
{
}


void CTFToolTipPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFilename() );
}


void CTFToolTipPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	AdjustToolTipSize();
}


void CTFToolTipPanel::ShowToolTip( const char *pszText, Panel *pFocusPanel /*= NULL*/ )
{
	Show();

	m_pText->SetText( pszText );
	InvalidateLayout( true );

	m_pFocusPanel = pFocusPanel;
}


void CTFToolTipPanel::HideToolTip( void )
{
	Hide();
}


void CTFToolTipPanel::Show( void )
{
	BaseClass::Show();
	MakePopup();

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
}


void CTFToolTipPanel::Hide( void )
{
	BaseClass::Hide();

	m_pFocusPanel = NULL;
}


void CTFToolTipPanel::OnThink( void )
{
	BaseClass::OnThink();
	int cursorX, cursorY;
	
	// If we were told to focus on something, sit under it, otherwise just stick to the cursor.
	if ( m_pFocusPanel )
	{
		m_pFocusPanel->GetPos( cursorX, cursorY );
		cursorX += ( m_pFocusPanel->GetWide() / 2 );
		cursorY += ( m_pFocusPanel->GetTall() / 1.25 );

		// Go through all the parents to make sure our position is correct.
		int parentX, parentY;
		for ( vgui::Panel *pParentPanel = m_pFocusPanel->GetParent(); pParentPanel != NULL; pParentPanel = pParentPanel->GetParent() )
		{
			pParentPanel->GetPos( parentX, parentY );
			cursorX += parentX;
			cursorY += parentY;
		}
	}
	else
	{
		surface()->SurfaceGetCursorPos( cursorX, cursorY );
	}

	int iTipW, iTipH;
	GetSize( iTipW, iTipH );

	int wide, tall;
	surface()->GetScreenSize( wide, tall );

	// First do the X-Position.
	cursorX -= TOOLTIP_XOFFSET;
	if ( cursorX < 0 )
	{
		cursorX += TOOLTIP_XOFFSET;
	}
	else if ( cursorX + iTipW > wide )
	{
		cursorX -= TOOLTIP_XOFFSET;
	}

	// Then the Y-Position, this is also the final step, so just do our calculations in a 'SetPos()'.
	cursorY += YRES( TOOLTIP_YOFFSET );
	if ( cursorY + iTipH > tall )
	{
		SetPos( cursorX, ( cursorY - iTipH ) - YRES( ( TOOLTIP_YOFFSET - 4 ) * 2 ) );
	}
	else
	{
		SetPos( cursorX, cursorY );
	}
}


void CTFToolTipPanel::AdjustToolTipSize( void )
{
	// Figure out text size and resize tooltip accordingly.
	if ( m_pText )
	{
		int x, y, wide, tall;
		m_pText->GetPos( x, y );
		m_pText->GetTextImage()->GetContentSize( wide, tall );

		m_pText->SetTall( tall );
		SetSize( x * 2 + wide, y * 2 + tall );
	}
}
