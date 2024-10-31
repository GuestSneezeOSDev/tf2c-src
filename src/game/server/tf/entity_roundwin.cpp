//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "entity_roundwin.h"
#include "tf_gamerules.h"
#include "tf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *g_aWinReasons[] =
{
	"WINREASON_NONE",
	"WINREASON_ALL_POINTS_CAPTURED",
	"WINREASON_OPPONENTS_DEAD",
	"WINREASON_FLAG_CAPTURE_LIMIT",
	"WINREASON_DEFEND_UNTIL_TIME_LIMIT",
	"WINREASON_STALEMATE",
	"WINREASON_TIMELIMIT",
	"WINREASON_WINLIMIT",
	"WINREASON_WINDIFFLIMIT",
	"WINREASON_RD_REACTOR_CAPTURED",
	"WINREASON_RD_CORES_COLLECTED",
	"WINREASON_RD_REACTOR_RETURNED",
	// TF2C
	"WINREASON_VIP_ESCAPED",
	"WINREASON_ROUNDSCORELIMIT",
	"WINREASON_VIP_KILLED",
};

//=============================================================================
//
// CTeamplayRoundWin tables.
//
BEGIN_DATADESC( CTeamplayRoundWin )

	DEFINE_KEYFIELD( m_bForceMapReset, FIELD_BOOLEAN, "force_map_reset" ),
	DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "switch_teams" ),
	DEFINE_KEYFIELD( m_strWinReason, FIELD_STRING, "win_reason" ),
	DEFINE_KEYFIELD( m_bWinBasedOnScore, FIELD_BOOLEAN, "score_based" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundWin", InputRoundWin ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnRoundWin, "OnRoundWin" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( game_round_win, CTeamplayRoundWin );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTeamplayRoundWin::CTeamplayRoundWin()
{
	m_iWinReason = WINREASON_NONE;
}


void CTeamplayRoundWin::Spawn( void )
{
	BaseClass::Spawn();

	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWinReasons ) == WINREASON_COUNT );
	m_iWinReason = UTIL_StringFieldToInt( STRING( m_strWinReason ), g_aWinReasons, WINREASON_COUNT );

	if ( m_iWinReason == -1 )
	{
		// default win reason for map-fired event (map may change it)
		m_iWinReason = WINREASON_DEFEND_UNTIL_TIME_LIMIT;
	}
}


void CTeamplayRoundWin::RoundWin( void )
{
	if ( m_bWinBasedOnScore )
	{
		RoundWinScore();
		return;
	}

	int iTeam = GetTeamNumber();

	if ( iTeam > LAST_SHARED_TEAM )
	{
		if ( !m_bForceMapReset )
		{
			TFGameRules()->SetWinningTeam( iTeam, m_iWinReason, m_bForceMapReset );
		}
		else
		{
			TFGameRules()->SetWinningTeam( iTeam, m_iWinReason, m_bForceMapReset, m_bSwitchTeamsOnWin );
		}
	}
	else
	{
		TFGameRules()->SetStalemate( STALEMATE_TIMER, m_bForceMapReset, m_bSwitchTeamsOnWin );
	}

	// Output.
	m_outputOnRoundWin.FireOutput( this, this );
}


void CTeamplayRoundWin::RoundWinScore(void)
{
	int iMax = -100000000;
	int iMaxTeam = 0;
	bool bStalemate = false;
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );
		if ( pTeam )
		{
			if ( pTeam->GetScore() > iMax )
			{
				iMax = pTeam->GetScore();
				iMaxTeam = i;
				bStalemate = false;
			}
			else if ( pTeam->GetScore() == iMax )
			{
				bStalemate = true;
			}
		}
	}

	if (bStalemate || iMaxTeam == 0)
	{
		TFGameRules()->SetStalemate( STALEMATE_TIMER, m_bForceMapReset, m_bSwitchTeamsOnWin );
	}
	else
	{
		if ( !m_bForceMapReset )
		{
			TFGameRules()->SetWinningTeam( iMaxTeam, m_iWinReason, m_bForceMapReset );
		}
		else
		{
			TFGameRules()->SetWinningTeam( iMaxTeam, m_iWinReason, m_bForceMapReset, m_bSwitchTeamsOnWin );
		}
	}

	m_outputOnRoundWin.FireOutput( this, this );
}

void CTeamplayRoundWin::InputRoundWin( inputdata_t &inputdata )
{
	RoundWin();
}

