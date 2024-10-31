//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_textwindow.h"
#include <cdll_client_int.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/Panel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/ImagePanel.h>

#include "tf_controls.h"
#include "tf_shareddefs.h"

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>
extern IGameUIFuncs *gameuifuncs; // for key binding details

#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTextWindow::CTFTextWindow( IViewPort *pViewPort ) : CTextWindow( pViewPort )
{
	m_pTFMessageTitle = new CExLabel( this, "TFMessageTitle", "" );
	m_pTFTextMessage = new CExRichText( this, "TFTextMessage" );

	SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFTextWindow::~CTFTextWindow()
{
}


void CTFTextWindow::ApplySchemeSettings( IScheme *pScheme )
{
	Frame::ApplySchemeSettings( pScheme );  // purposely skipping the CTextWindow version

	LoadControlSettings("Resource/UI/TextWindow.res");

	Reset();

	m_pHTMLMessage->SetBgColor( pScheme->GetColor( "HTMLBackground", Color( 255, 0, 0, 255 ) ) );
}


void CTFTextWindow::Reset( void )
{
	Update();
}


void CTFTextWindow::Update()
{
	m_pTFMessageTitle->SetText( m_szTitle );
	m_pTFTextMessage->SetVisible( false );

	BaseClass::Update();

	Panel *pOK = FindChildByName( "ok" );
	if ( pOK )
	{
		pOK->RequestFocus();
	}
}


void CTFTextWindow::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	if ( state )
	{
		Panel *pOK = FindChildByName( "ok" );
		if ( pOK )
		{
			pOK->RequestFocus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows the text window
//-----------------------------------------------------------------------------
void CTFTextWindow::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	BaseClass::ShowPanel( bShow );

	m_pViewPort->ShowBackGround( false );
}


void CTFTextWindow::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "okay" );		
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The background is painted elsewhere, so we should do nothing
//-----------------------------------------------------------------------------
void CTFTextWindow::PaintBackground()
{
}


void CTFTextWindow::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "okay" ) )
	{
		m_pViewPort->ShowPanel( this, false );
		m_pViewPort->ShowPanel( PANEL_MAPINFO, true );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


void CTFTextWindow::ShowText( const char *text )
{
	ShowTitleLabel( true );

	m_pTFTextMessage->SetVisible( true );
	m_pTFTextMessage->SetText( text );
	m_pTFTextMessage->GotoTextStart();
}


void CTFTextWindow::ShowURL( const char *URL, bool bAllowUserToDisable /*= true*/ )
{
	ShowTitleLabel( false );
	BaseClass::ShowURL( URL, bAllowUserToDisable );
}


void CTFTextWindow::ShowFile( const char *filename )
{
	ShowTitleLabel( false );
	BaseClass::ShowFile( filename );
}


void CTFTextWindow::ShowTitleLabel( bool show )
{
	m_pTFMessageTitle->SetVisible( show );
}
