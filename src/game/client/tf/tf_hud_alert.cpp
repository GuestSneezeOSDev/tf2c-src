//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


class CHudAlert : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudAlert, EditablePanel );

public:
	CHudAlert( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );

private:
	Label			*m_pAlertLabel;
	float			m_flHideAt;

};

DECLARE_HUDELEMENT( CHudAlert );


CHudAlert::CHudAlert( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAlert" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flHideAt = 0;
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CHudAlert::Init( void )
{
	// listen for events
	ListenForGameEvent( "teamplay_alert" );

	SetVisible( false );
	CHudElement::Init();
}


void CHudAlert::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_alert", pEventName ) == 0 )
	{
		int iAlertType = event->GetInt( "alert_type" );

		if ( iAlertType == HUD_ALERT_SCRAMBLE_TEAMS )
		{
			if ( m_pAlertLabel )
			{
				m_pAlertLabel->SetText( g_pVGuiLocalize->Find( "#game_scramble_onrestart" ) );
			}

			m_flHideAt = gpGlobals->curtime + 5.0;
			SetVisible( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudAlert::OnTick( void )
{
	if ( m_flHideAt && m_flHideAt < gpGlobals->curtime )
	{
		SetVisible( false );
		m_flHideAt = 0;
	}
}


void CHudAlert::LevelInit( void )
{
	m_flHideAt = 0;
	SetVisible( false );
}


bool CHudAlert::ShouldDraw( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return ( IsVisible() );
}


void CHudAlert::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudAlert.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pAlertLabel = dynamic_cast<Label *>( FindChildByName( "AlertLabel" ) );
}
