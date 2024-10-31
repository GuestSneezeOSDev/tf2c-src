//======= Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#include "cbase.h"
#include "func_capture_zone.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"

//=============================================================================
//
// CTF Flag Capture Zone tables.
//

BEGIN_DATADESC( CCaptureZone )

// Keyfields.
DEFINE_KEYFIELD( m_nCapturePoint, FIELD_INTEGER, "CapturePoint" ),
DEFINE_KEYFIELD( m_bWinOnCapture, FIELD_BOOLEAN, "WinOnCapture" ),
DEFINE_KEYFIELD( m_bForceMapReset, FIELD_BOOLEAN, "ForceMapReset" ),
DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "SwitchTeamsOnWin" ),
DEFINE_KEYFIELD( m_strAssociatedFlag, FIELD_STRING, "AssociatedFlag" ),

// Functions.
DEFINE_ENTITYFUNC( Touch ),

// Outputs.
DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
DEFINE_OUTPUT( m_outputOnCapTeam1, "OnCapTeam1" ),
DEFINE_OUTPUT( m_outputOnCapTeam2, "OnCapTeam2" ),
DEFINE_OUTPUT( m_outputOnCapTeam3, "OnCapTeam3" ),
DEFINE_OUTPUT( m_outputOnCapTeam4, "OnCapTeam4" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_capturezone, CCaptureZone );


IMPLEMENT_SERVERCLASS_ST( CCaptureZone, DT_CaptureZone )
END_SEND_TABLE()

//=============================================================================
//
// CTF Flag Capture Zone functions.
//

CCaptureZone::CCaptureZone()
{
	m_nCapturePoint = 0;
	m_bWinOnCapture = false;
	m_bForceMapReset = true;
	m_bSwitchTeamsOnWin = false;
}


void CCaptureZone::Spawn( void )
{
	InitTrigger();
	SetTouch( &CCaptureZone::Touch );

	m_flNextTouchingEnemyZoneWarning = -1;
}



void CCaptureZone::Activate( void )
{
	BaseClass::Activate();

	m_hAssociatedFlag = dynamic_cast<CCaptureFlag *>( gEntList.FindEntityByName( NULL, m_strAssociatedFlag, this ) );
}


void CCaptureZone::Touch( CBaseEntity *pOther )
{
	// Is the zone enabled?
	if ( m_bDisabled )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( m_hAssociatedFlag && !m_hAssociatedFlag->IsHome() )
		return;

	// Get the TF player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	// Check to see if the player has the capture flag.
	CCaptureFlag *pFlag = pPlayer->GetTheFlag();
	if ( !pFlag || pFlag->IsCaptured() )
		return;

	// If this capture point have a team number asssigned check to see
	// if the capture zone team matches the player's team.
	if ( GetTeamNumber() != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != GetTeamNumber() )
	{
		// Do this at most once every 5 seconds
		if ( m_flNextTouchingEnemyZoneWarning < gpGlobals->curtime )
		{
			if ( pFlag->GetGameType() == TF_FLAGTYPE_CTF )
			{
				CSingleUserRecipientFilter filter( pPlayer );
				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP, pFlag->GetTeamNumber() );
			}
			else if ( pFlag->GetGameType() == TF_FLAGTYPE_INVADE )
			{
				CSingleUserRecipientFilter filter( pPlayer );
				TFGameRules()->SendHudNotification( filter, "#TF_Invade_Wrong_Goal", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );
			}

			m_flNextTouchingEnemyZoneWarning = gpGlobals->curtime + 5.0f;
		}

		return;
	}

	pFlag->Capture( pPlayer, m_nCapturePoint );

	// Output.
	m_outputOnCapture.FireOutput( this, this );

	int iCappingTeam = pPlayer->GetTeamNumber();

	switch ( iCappingTeam )
	{
	case TF_TEAM_RED:
		m_outputOnCapTeam1.FireOutput( this, this );
		break;
	case TF_TEAM_BLUE:
		m_outputOnCapTeam2.FireOutput( this, this );
		break;
	case TF_TEAM_GREEN:
		m_outputOnCapTeam3.FireOutput( this, this );
		break;
	case TF_TEAM_YELLOW:
		m_outputOnCapTeam4.FireOutput( this, this );
		break;
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "ctf_flag_captured" );
	if ( event )
	{
		int	iCappingTeamScore = 0;
		CTFTeam* pCappingTeam = pPlayer->GetTFTeam();
		if ( pCappingTeam )
		{
			iCappingTeamScore = pCappingTeam->GetFlagCaptures();
		}

		event->SetInt( "capping_team", iCappingTeam );
		event->SetInt( "capping_team_score", iCappingTeamScore );
		event->SetInt( "capper", pPlayer->GetUserID() );
		event->SetInt( "priority", 9 ); // HLTV priority

		gameeventmanager->FireEvent( event );
	}

	if ( m_bWinOnCapture )
	{
		TFGameRules()->SetWinningTeam( pPlayer->GetTeamNumber(), WINREASON_ALL_POINTS_CAPTURED, m_bForceMapReset, m_bSwitchTeamsOnWin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureZone::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
