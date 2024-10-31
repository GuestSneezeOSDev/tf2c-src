//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tf_imagepanel.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFImagePanel );


CTFImagePanel::CTFImagePanel( Panel *parent, const char *name ) : ScalableImagePanel( parent, name )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_szTeamBG[i][0] = '\0';
	}

	UpdateBGTeam();

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "server_spawn" );

	m_iVerticalOffsetSpecial = 0;
}


void CTFImagePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Since the default pannel color is gray, all CTFImagePanels become darker
	// Lets ensure that *doesnt* happen
	// Color overrides still work! So don't worry - Kay
	SetFgColor(Color(255, 255, 255));
}


void CTFImagePanel::ApplySettings( KeyValues *inResourceData )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		V_strcpy_safe( m_szTeamBG[i], inResourceData->GetString( VarArgs( "teambg_%d", i ), "" ) );
	}

	BaseClass::ApplySettings( inResourceData );

	m_iVerticalOffsetSpecial = inResourceData->GetInt("vertical_offset_special", 0);
	m_iHorizontalOffsetSpecial = inResourceData->GetInt( "horizontal_offset_special", 0 );
	UpdateBGImage();
}


void CTFImagePanel::UpdateBGImage( void )
{
	if ( m_iBGTeam >= 0 && m_iBGTeam < TF_TEAM_COUNT )
	{
		if ( m_szTeamBG[m_iBGTeam][0] != '\0' )
		{
			SetImage( m_szTeamBG[m_iBGTeam] );
		}
	}
}


void CTFImagePanel::SetBGImage( int iTeamNum )
{
	m_iBGTeam = iTeamNum;
	UpdateBGImage();
}


void CTFImagePanel::UpdateBGTeam( void )
{
	m_iBGTeam = GetLocalPlayerTeam();
}


void CTFImagePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		UpdateBGTeam();
		UpdateBGImage();
	}
	else if ( FStrEq( "server_spawn", event->GetName() ) )
	{
		m_iBGTeam = TEAM_SPECTATOR;
		UpdateBGImage();
	}
}


Color CTFImagePanel::GetDrawColor( void )
{
	Color tempColor = GetFgColor();
	tempColor[3] = GetAlpha();

	return tempColor;
}
