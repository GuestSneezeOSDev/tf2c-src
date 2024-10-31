//=============================================================================//
//
// Purpose: Arena player counter
//
//=============================================================================//
#include "cbase.h"
#include "tf_hud_arena_player_count.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudArenaPlayerCount );

CHudArenaPlayerCount::CHudArenaPlayerCount( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaPlayerCount" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pRedPanel = new EditablePanel( this, "redteam" );
	m_pBluePanel = new EditablePanel( this, "blueteam" );
	m_pGreenPanel = new EditablePanel( this, "greenteam" );
	m_pYellowPanel = new EditablePanel( this, "yellowteam" );

	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "game_maploaded" );
}


void CHudArenaPlayerCount::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		AddSubKeyNamed( pConditions, "if_fourteams" );
	}

	LoadControlSettings( "resource/UI/HudArenaPlayerCount.res", NULL, NULL, pConditions );

	pConditions->deleteThis();
}


bool CHudArenaPlayerCount::ShouldDraw( void )
{
	// Only in Arena mode.
	if ( !TFGameRules() )
		return false;

	if ( !TFGameRules()->IsInArenaMode() )
		return false;

	// Not in preround state since we're showing the timer at that time.
	if ( TFGameRules()->State_Get() == GR_STATE_PREROUND || TFGameRules()->IsInWaitingForPlayers() )
		return false;

	return CHudElement::ShouldDraw();
}


void CHudArenaPlayerCount::OnTick( void )
{
	if ( !IsVisible() || !g_TF_PR )
		return;

	if ( TFGameRules()->IsInArenaMode() )
	{
		// Update alive player counts for all teams.
		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			int iNumAlive = g_TF_PR->GetNumPlayersForTeam( i, true );
			EditablePanel *pPanel = GetTeamPanel( i );

			pPanel->SetDialogVariable( VarArgs( "%s_alive", g_aTeamLowerNames[i] ), iNumAlive );
		}
	}
}


void CHudArenaPlayerCount::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "game_maploaded" ) == 0 )
	{
		InvalidateLayout( false, true );
	}
}



EditablePanel *CHudArenaPlayerCount::GetTeamPanel( int iTeam )
{
	switch ( iTeam )
	{
	case TF_TEAM_RED:
		return m_pRedPanel;
	case TF_TEAM_BLUE:
		return m_pBluePanel;
	case TF_TEAM_GREEN:
		return m_pGreenPanel;
	case TF_TEAM_YELLOW:
		return m_pYellowPanel;
	default:
		Assert( false );
		return NULL;
	}
}
