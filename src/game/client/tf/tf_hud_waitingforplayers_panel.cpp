
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "iclientmode.h"
#include "hud.h"
#include "hudelement.h"
#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


class CTFWaitingForPlayersPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFWaitingForPlayersPanel, EditablePanel );

public:
	CTFWaitingForPlayersPanel( const char *pElementName );

	virtual void LevelInit();
	virtual void Init();
	virtual void FireGameEvent( IGameEvent * event );
	virtual void ApplySchemeSettings( IScheme *pScheme );

	virtual bool ShouldDraw( void );

private:
	Label	*m_pWaitingForPlayersLabel;
	Label	*m_pWaitingForPlayersEndingLabel;
};

DECLARE_HUDELEMENT_DEPTH( CTFWaitingForPlayersPanel, 1 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWaitingForPlayersPanel::CTFWaitingForPlayersPanel( const char *pElementName )
	: EditablePanel( NULL, "WaitingForPlayersPanel" ), CHudElement( pElementName )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetScheme( ClientSchemesArray[SCHEME_CLIENT_STRING] );
}


void CTFWaitingForPlayersPanel::LevelInit()
{
	SetVisible( false );
}


void CTFWaitingForPlayersPanel::Init()
{
	// listen for events
	ListenForGameEvent( "teamplay_waiting_begins" );
	ListenForGameEvent( "teamplay_waiting_ends" );
	ListenForGameEvent( "teamplay_waiting_abouttoend" );
	
	SetVisible( false );

	CHudElement::Init();
}


void CTFWaitingForPlayersPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/WaitingForPlayersPanel.res" );

	m_pWaitingForPlayersLabel = dynamic_cast<Label *>(FindChildByName( "WaitingForPlayersLabel" ));
	m_pWaitingForPlayersEndingLabel = dynamic_cast<Label *>(FindChildByName( "WaitingForPlayersEndingLabel" ));
}


void CTFWaitingForPlayersPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_waiting_ends", pEventName ) == 0 )
	{
		SetVisible( false );
	}
	else if ( Q_strcmp( "teamplay_waiting_begins", pEventName ) == 0 )
	{
		SetVisible( true );
		m_pWaitingForPlayersLabel->SetVisible( true );
		m_pWaitingForPlayersEndingLabel->SetVisible( false );
	}
	else if ( Q_strcmp( "teamplay_waiting_abouttoend", pEventName ) == 0 )
	{
		SetVisible( true );
		m_pWaitingForPlayersLabel->SetVisible( false );
		m_pWaitingForPlayersEndingLabel->SetVisible( true );
	}
}


bool CTFWaitingForPlayersPanel::ShouldDraw( void )
{
	return ( IsVisible() );
}
