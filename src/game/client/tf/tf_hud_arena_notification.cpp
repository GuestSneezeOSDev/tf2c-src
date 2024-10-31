//=============================================================================//
//
// Purpose: Arena notification panel
//
//=============================================================================//
#include "cbase.h"
#include "tf_hud_arena_notification.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include "tf_gamerules.h"
#include <vgui/ILocalize.h>
#include "hud_macros.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar tf_arena_preround_time;

DECLARE_HUDELEMENT( CHudArenaNotification );
DECLARE_HUD_MESSAGE( CHudArenaNotification, HudArenaNotify );

CHudArenaNotification::CHudArenaNotification( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaNotification" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pBalanceLabel = new Label( this, "BalanceLabel", "" );
	m_pBalanceLabelTip = new Label( this, "BalanceLabelTip", "" );
	m_iMessage = TF_ARENA_MESSAGE_NONE;
	m_flHideAt = -1.0f;

	HOOK_HUD_MESSAGE( CHudArenaNotification, HudArenaNotify );
	ivgui()->AddTickSignal( GetVPanel() );
}


bool CHudArenaNotification::ShouldDraw( void )
{
	// Only in Arena mode.
	if ( !TFGameRules() || !TFGameRules()->IsInArenaMode() )
		return false;

	if ( gpGlobals->curtime > m_flHideAt )
		return false;

	return CHudElement::ShouldDraw();
}


void CHudArenaNotification::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudArenaNotification.res" );
}


void CHudArenaNotification::LevelInit( void )
{
	m_iMessage = TF_ARENA_MESSAGE_NONE;
	m_flHideAt = -1.0f;
}


void CHudArenaNotification::OnTick( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsInArenaMode() )
		return;

	// If we're in pregame state, we're waiting for players.
	if ( TFGameRules()->State_Get() == GR_STATE_PREGAME && !TFGameRules()->IsInWaitingForPlayers() )
	{
		if ( m_iMessage != TF_ARENA_MESSAGE_NOPLAYERS )
		{
			SetupSwitchPanel( TF_ARENA_MESSAGE_NOPLAYERS );

			// Never hide.
			m_flHideAt = FLT_MAX;
		}
	}
	else
	{
		if ( m_iMessage == TF_ARENA_MESSAGE_NOPLAYERS )
		{
			m_flHideAt = -1.0f;
			m_iMessage = TF_ARENA_MESSAGE_NONE;
		}

		if ( TFGameRules()->State_Get() == GR_STATE_STALEMATE )
		{
			// Hide any notifications once the round stats.
			m_flHideAt = -1.0f;
		}
	}
}


void CHudArenaNotification::SetupSwitchPanel( int iMessage )
{
	switch ( iMessage )
	{
	case TF_ARENA_MESSAGE_NOPLAYERS:
		m_pBalanceLabel->SetText( "#TF_Arena_Welcome" );
		m_pBalanceLabelTip->SetText( "#TF_Arena_NoPlayers" );
		break;
	case TF_ARENA_MESSAGE_SITOUT:
		m_pBalanceLabel->SetText( "#TF_Arena_SitOut" );
		m_pBalanceLabelTip->SetText( "#TF_Arena_Protip" );
		break;
	case TF_ARENA_MESSAGE_CAREFUL:
		m_pBalanceLabel->SetText( "#TF_Arena_Careful" );
		m_pBalanceLabelTip->SetText( "#TF_Arena_Protip" );
		break;
	}

	m_iMessage = iMessage;
}


void CHudArenaNotification::MsgFunc_HudArenaNotify( bf_read &msg )
{
	int iPlayer = msg.ReadByte();
	int iMessage = msg.ReadByte();

	// Why does it even send player index??? You can use filters with user messages.
	if ( iPlayer == GetLocalPlayerIndex() )
	{
		SetupSwitchPanel( iMessage );
		m_flHideAt = gpGlobals->curtime + tf_arena_preround_time.GetFloat();
	}
}
