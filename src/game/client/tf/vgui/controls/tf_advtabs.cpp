﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <assert.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include "vgui_controls/Frame.h"

#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include "tf_advtabs.h"
#include "tf_advbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


DECLARE_BUILD_FACTORY( CAdvTabs );

CAdvTabs::CAdvTabs( vgui::Panel *parent, char const *panelName ) : EditablePanel( parent, panelName )
{
	m_pCurrentButton = NULL;
}


CAdvTabs::~CAdvTabs()
{
}


void CAdvTabs::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void CAdvTabs::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void CAdvTabs::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_pButtons.RemoveAll();
	int iCount = GetChildCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CTFButton *pButton = dynamic_cast<CTFButton *>( GetChild( i ) );
		if ( pButton )
		{
			m_pButtons.AddToTail( pButton );
		}
	}

	if ( !m_pCurrentButton && !m_pButtons.IsEmpty() )
	{
		m_pCurrentButton = m_pButtons[0];
		m_pCurrentButton->SetSelected( true );
		m_pCurrentButton->SetZPos( m_pCurrentButton->GetZPos() + 2 );
	}
}


void CAdvTabs::OnCommand( const char* command )
{
	if ( GetParent() )
	{
		GetParent()->OnCommand( command );
	}
}


void CAdvTabs::OnButtonPressed( Panel *pPanel )
{
	if ( pPanel == m_pCurrentButton )
		return;

	if ( m_pCurrentButton )
	{
		m_pCurrentButton->SetSelected( false );
		m_pCurrentButton->SetZPos( m_pCurrentButton->GetZPos() - 2 );
	}

	m_pCurrentButton = assert_cast<CTFButton *>( pPanel );
	m_pCurrentButton->SetSelected( true );
	m_pCurrentButton->SetZPos( m_pCurrentButton->GetZPos() + 2 );
}


void CAdvTabs::SetSelectedButton( const char *pszName )
{
	FOR_EACH_VEC( m_pButtons, i )
	{
		CTFButton *pButton = m_pButtons[i];

		if ( pButton && V_stricmp( pButton->GetName(), pszName ) == 0 )
		{
			OnButtonPressed( pButton );
			break;
		}
	}
}
