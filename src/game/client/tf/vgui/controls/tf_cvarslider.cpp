//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "tf_cvarslider.h"
#include <stdio.h>
//#include "EngineInterface.h"
#include "tier1/KeyValues.h"
#include "tier1/convar.h"
#include <vgui/IVGui.h>
#include <vgui_controls/PropertyPage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


DECLARE_BUILD_FACTORY( CTFCvarSlider );


CTFCvarSlider::CTFCvarSlider( Panel *parent, const char *panelName )
	: CTFSlider( parent, panelName ),
	m_cvar( "", true )
{
	SetupSlider( 0, 1, false, false );
}


CTFCvarSlider::CTFCvarSlider( Panel *parent, const char *panelName,
	float minValue, float maxValue, char const *cvarname, bool bShowFrac, bool bAutoChange )
	: CTFSlider( parent, panelName ),
	m_cvar( cvarname, false )
{
	SetupSlider( minValue, maxValue, bShowFrac, bAutoChange );
}


CTFCvarSlider::~CTFCvarSlider()
{
}


void CTFCvarSlider::SetupSlider( float minValue, float maxValue, bool bShowFrac, bool bAutoChange )
{
	m_flStartValue = 0.0f;
	m_bShowFrac = bShowFrac;
	m_bAutoChange = bAutoChange;

	SetRange( minValue, maxValue );

	ivgui()->AddTickSignal( GetVPanel() );
	AddActionSignalTarget( this );

	Reset();
}


void CTFCvarSlider::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	float minValue = inResourceData->GetFloat( "minvalue", 0 );
	float maxValue = inResourceData->GetFloat( "maxvalue", 1 );
	bool bAutoChange = inResourceData->GetBool( "autochange" );
	const char *cvarname = inResourceData->GetString( "cvar_name", "" );

	if ( cvarname[0] != '\0' )
	{
		m_cvar.Init( cvarname, false );
	}

	SetupSlider( minValue, maxValue, m_bShowFrac, bAutoChange );
}


void CTFCvarSlider::OnTick( void )
{
	// Check if cvar value was changed from under this.
	if ( !m_cvar.IsValid() )
		return;

	float flValue = m_cvar.GetFloat();

	if ( flValue != m_flStartValue )
	{
		SetValue( m_flValue );
		m_flStartValue = flValue;
	}
}


void CTFCvarSlider::Reset( void )
{
	// Set slider to current value
	if ( !m_cvar.IsValid() )
	{
		m_flStartValue = 0.0f;
		return;
	}

	m_flStartValue = m_cvar.GetFloat();
	SetValue( m_cvar.GetFloat() );
}


bool CTFCvarSlider::HasBeenModified( void )
{
	return ( GetValue() != m_flStartValue );
}


void CTFCvarSlider::ApplyChanges( void )
{
	if ( !m_cvar.IsValid() )
		return;

	m_cvar.SetValue( GetFinalValue() );
	m_flStartValue = m_cvar.GetFloat();
}


void CTFCvarSlider::OnSliderMoved( void )
{
	if ( HasBeenModified() )
	{
		// Tell parent that we've been modified
		PostActionSignal( new KeyValues( "ControlModified" ) );

		// If set to auto-change cvar, apply changes now.
		if ( m_bAutoChange )
		{
			ApplyChanges();
		}
	}
}


void CTFCvarSlider::OnApplyChanges( void )
{
	ApplyChanges();
}
