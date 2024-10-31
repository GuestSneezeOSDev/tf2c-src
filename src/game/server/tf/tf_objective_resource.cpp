//========= Copyright  1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_objective_resource.h"
#include "team.h"

#include "tf_gamerules.h"

#include "achievements_tf.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAPHUD_PARITY_BITS		6
#define CAPHUD_PARITY_MASK		((1<<CAPHUD_PARITY_BITS)-1)

#define LAZY_UPDATE_TIME		3

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFObjectiveResource, DT_TFObjectiveResource )
	SendPropInt( SENDINFO( m_iTimerToShowInHUD ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iStopWatchTimer ), MAX_EDICT_BITS, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_iNumControlPoints ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bPlayingMiniRounds ) ),
	SendPropBool( SENDINFO( m_bControlPointsReset ) ),
	SendPropInt( SENDINFO( m_iUpdateCapHudParity ), CAPHUD_PARITY_BITS, SPROP_UNSIGNED ),

	// data variables
	SendPropArray( SendPropVector( SENDINFO_ARRAY( m_vCPPositions ), -1, SPROP_COORD ), m_vCPPositions ),
	SendPropArray3( SENDINFO_ARRAY3( m_bCPIsVisible ), SendPropInt( SENDINFO_ARRAY( m_bCPIsVisible ), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flLazyCapPerc ), SendPropFloat( SENDINFO_ARRAY( m_flLazyCapPerc ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeamIcons ), SendPropInt( SENDINFO_ARRAY( m_iTeamIcons ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeamOverlays ), SendPropInt( SENDINFO_ARRAY( m_iTeamOverlays ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeamReqCappers ), SendPropInt( SENDINFO_ARRAY( m_iTeamReqCappers ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flTeamCapTime ), SendPropTime( SENDINFO_ARRAY( m_flTeamCapTime ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPreviousPoints ), SendPropInt( SENDINFO_ARRAY( m_iPreviousPoints ), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bTeamCanCap ), SendPropBool( SENDINFO_ARRAY( m_bTeamCanCap ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeamBaseIcons ), SendPropInt( SENDINFO_ARRAY( m_iTeamBaseIcons ), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBaseControlPoints ), SendPropInt( SENDINFO_ARRAY( m_iBaseControlPoints ), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bInMiniRound ), SendPropBool( SENDINFO_ARRAY( m_bInMiniRound ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iWarnOnCap ), SendPropInt( SENDINFO_ARRAY( m_iWarnOnCap ), 4, SPROP_UNSIGNED ) ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszWarnSound ) ), m_iszWarnSound ),
	SendPropArray3( SENDINFO_ARRAY3( m_flPathDistance ), SendPropFloat( SENDINFO_ARRAY( m_flPathDistance ), 8, 0, 0.0f, 1.0f ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCPGroup ), SendPropInt( SENDINFO_ARRAY( m_iCPGroup ), 5 ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bCPLocked ), SendPropBool( SENDINFO_ARRAY( m_bCPLocked ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nNumNodeHillData ), SendPropInt( SENDINFO_ARRAY( m_nNumNodeHillData ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flNodeHillData ), SendPropFloat( SENDINFO_ARRAY( m_flNodeHillData ), 8, 0, 0.0f, 1.0f ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bTrackAlarm ), SendPropBool( SENDINFO_ARRAY( m_bTrackAlarm ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flUnlockTimes ), SendPropFloat( SENDINFO_ARRAY( m_flUnlockTimes ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bHillIsDownhill ), SendPropBool( SENDINFO_ARRAY( m_bHillIsDownhill ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCPTimerTimes ), SendPropFloat( SENDINFO_ARRAY( m_flCPTimerTimes ) ) ),

	// state variables
	SendPropArray3( SENDINFO_ARRAY3( m_iNumTeamMembers ), SendPropInt( SENDINFO_ARRAY( m_iNumTeamMembers ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCappingTeam ), SendPropInt( SENDINFO_ARRAY( m_iCappingTeam ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeamInZone ), SendPropInt( SENDINFO_ARRAY( m_iTeamInZone ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bBlocked ), SendPropInt( SENDINFO_ARRAY( m_bBlocked ), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iOwner ), SendPropInt( SENDINFO_ARRAY( m_iOwner ), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bCPCapRateScalesWithPlayers ), SendPropBool( SENDINFO_ARRAY( m_bCPCapRateScalesWithPlayers ) ) ),
	SendPropString( SENDINFO( m_pszCapLayoutInHUD ) ),
	SendPropFloat( SENDINFO( m_flCustomPositionX ) ),
	SendPropFloat( SENDINFO( m_flCustomPositionY ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_flVIPProgress ), SendPropFloat( SENDINFO_ARRAY( m_flVIPProgress ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDominationRate ), SendPropInt( SENDINFO_ARRAY( m_iDominationRate ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bHalted ), SendPropInt( SENDINFO_ARRAY( m_bHalted ), 1, SPROP_UNSIGNED ) ),

	SendPropBool( SENDINFO( m_bDisplayObjectiveHUD ) ),
END_SEND_TABLE()


BEGIN_DATADESC( CTFObjectiveResource )
	DEFINE_THINKFUNC( ObjectiveThink ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( tf_objective_resource, CTFObjectiveResource );

CTFObjectiveResource *g_pObjectiveResource = NULL;


CTFObjectiveResource::CTFObjectiveResource()
{
	g_pObjectiveResource = this;
	m_bPlayingMiniRounds = false;
	m_iUpdateCapHudParity = 0;
	m_bControlPointsReset = false;
	m_flCustomPositionX = -1.f;
	m_flCustomPositionY = -1.f;
}


CTFObjectiveResource::~CTFObjectiveResource()
{
	Assert( g_pObjectiveResource == this );
	g_pObjectiveResource = NULL;
}


void CTFObjectiveResource::Spawn( void )
{
	m_iNumControlPoints = 0;

	// If you hit this, you've got too many teams for the control point system to handle.
	Assert( GetNumberOfTeams() <= TF_TEAM_COUNT );

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		// data variables
		m_vCPPositions.Set( i, vec3_origin );
		m_bCPIsVisible.Set( i, true );
		m_bBlocked.Set( i, false );

		// state variables
		m_iOwner.Set( i, TEAM_UNASSIGNED );
		m_iCappingTeam.Set( i, TEAM_UNASSIGNED );
		m_iTeamInZone.Set( i, TEAM_UNASSIGNED );
		m_bInMiniRound.Set( i, true );
		m_iWarnOnCap.Set( i, CP_WARN_NORMAL );
		m_iCPGroup.Set( i, TEAM_INVALID );
		m_flLazyCapPerc.Set( i, 0.0 );
		m_bCPLocked.Set( i, false );
		m_flUnlockTimes.Set( i, 0.0 );
		m_flCPTimerTimes.Set( i, -1.0 );
		m_bCPCapRateScalesWithPlayers.Set( i, true );
		m_iDominationRate.Set( i, 0 );
		m_bHalted.Set( i, false );

		for ( int team = 0; team < TF_TEAM_COUNT; team++ )
		{
			int iTeamIndex = TEAM_ARRAY( i, team );

			m_iTeamIcons.Set( iTeamIndex, 0 );
			m_iTeamOverlays.Set( iTeamIndex, 0 );
			m_iTeamReqCappers.Set( iTeamIndex, 0 );
			m_flTeamCapTime.Set( iTeamIndex, 0.0f );
			m_iNumTeamMembers.Set( TEAM_ARRAY( i, team ), 0 );
			for ( int ipoint = 0; ipoint < MAX_PREVIOUS_POINTS; ipoint++ )
			{
				int iIntIndex = ipoint + ( i * MAX_PREVIOUS_POINTS ) + ( team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS );
				m_iPreviousPoints.Set( iIntIndex, -1 );
			}
			m_bTeamCanCap.Set( iTeamIndex, false );
		}
	}

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_iBaseControlPoints.Set( i, -1 );
	}

	int nNumEntriesPerTeam = TF_TRAIN_MAX_HILLS * TF_TRAIN_FLOATS_PER_HILL;
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_nNumNodeHillData.Set( i, 0 );
		m_bTrackAlarm.Set( i, false );

		int iStartingIndex = i * nNumEntriesPerTeam;
		for ( int j = 0; j < nNumEntriesPerTeam; j++ )
		{
			m_flNodeHillData.Set( iStartingIndex + j, 0 );
		}

		iStartingIndex = i * TF_TRAIN_MAX_HILLS;
		for ( int j = 0; j < TF_TRAIN_MAX_HILLS; j++ )
		{
			m_bHillIsDownhill.Set( iStartingIndex + j, 0 );
		}
	}

	SetThink( &CTFObjectiveResource::ObjectiveThink );
	SetNextThink( gpGlobals->curtime + LAZY_UPDATE_TIME );
}


void CTFObjectiveResource::ObjectiveThink( void )
{
	SetNextThink( gpGlobals->curtime + LAZY_UPDATE_TIME );

	for ( int i = 0; i < m_iNumControlPoints; i++ )
	{
		if ( m_iCappingTeam[i] )
		{
			m_flLazyCapPerc.Set( i, m_flCapPercentages[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The objective resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CTFObjectiveResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Round is starting, reset state
//-----------------------------------------------------------------------------
void CTFObjectiveResource::ResetControlPoints( void )
{
	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		m_iCappingTeam.Set( i, TEAM_UNASSIGNED );
		m_iTeamInZone.Set( i, TEAM_UNASSIGNED );
		m_bInMiniRound.Set( i, true );

		for ( int team = 0; team < TF_TEAM_COUNT; team++ )
		{
			m_iNumTeamMembers.Set( TEAM_ARRAY( i, team ), 0.0f );
		}
	}

	UpdateCapHudElement();
	m_bControlPointsReset = !m_bControlPointsReset;
}

//-----------------------------------------------------------------------------
// Purpose: Data setting functions
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetNumControlPoints( int num )
{
	Assert( num <= MAX_CONTROL_POINTS );
	m_iNumControlPoints = num;
}


void CTFObjectiveResource::SetCPIcons( int index, int iTeam, int iIcon )
{
	AssertValidIndex( index );
	m_iTeamIcons.Set( TEAM_ARRAY( index, iTeam ), iIcon );
}


void CTFObjectiveResource::SetCPOverlays( int index, int iTeam, int iIcon )
{
	AssertValidIndex( index );
	m_iTeamOverlays.Set( TEAM_ARRAY( index, iTeam ), iIcon );
}


void CTFObjectiveResource::SetTeamBaseIcons( int iTeam, int iBaseIcon )
{
	Assert( iTeam >= 0 && iTeam < MAX_TEAMS );
	m_iTeamBaseIcons.Set( iTeam, iBaseIcon );
}


void CTFObjectiveResource::SetCPPosition( int index, const Vector& vPosition )
{
	AssertValidIndex( index );
	m_vCPPositions.Set( index, vPosition );
}


void CTFObjectiveResource::SetCPVisible( int index, bool bVisible )
{
	AssertValidIndex( index );
	m_bCPIsVisible.Set( index, bVisible );
}


void CTFObjectiveResource::SetWarnOnCap( int index, int iWarnLevel )
{
	AssertValidIndex( index );
	m_iWarnOnCap.Set( index, iWarnLevel );
}


void CTFObjectiveResource::SetWarnSound( int index, string_t iszSound )
{
	AssertValidIndex( index );
	m_iszWarnSound.Set( index, iszSound );
}


void CTFObjectiveResource::SetCPGroup( int index, int iCPGroup )
{
	AssertValidIndex( index );
	m_iCPGroup.Set( index, iCPGroup );
}


void CTFObjectiveResource::SetCPRequiredCappers( int index, int iTeam, int iReqPlayers )
{
	AssertValidIndex( index );
	m_iTeamReqCappers.Set( TEAM_ARRAY( index, iTeam ), iReqPlayers );
}


void CTFObjectiveResource::SetCPCapTime( int index, int iTeam, float flTime )
{
	AssertValidIndex( index );
	m_flTeamCapTime.Set( TEAM_ARRAY( index, iTeam ), flTime );
}


void CTFObjectiveResource::SetCPCapPercentage( int index, float flTime )
{
	AssertValidIndex( index );
	m_flCapPercentages[index] = flTime;
}


float CTFObjectiveResource::GetCPCapPercentage( int index )
{
	AssertValidIndex( index );
	return m_flCapPercentages[index];
}


void CTFObjectiveResource::SetCPUnlockTime( int index, float flTime )
{
	AssertValidIndex( index );
	m_flUnlockTimes.Set( index, flTime );
}


void CTFObjectiveResource::SetCPTimerTime( int index, float flTime )
{
	AssertValidIndex( index );
	m_flCPTimerTimes.Set( index, flTime );
}


void CTFObjectiveResource::SetCPCapTimeScalesWithPlayers( int index, bool bScales )
{
	AssertValidIndex( index );
	m_bCPCapRateScalesWithPlayers.Set( index, bScales );
}


void CTFObjectiveResource::SetTeamCanCap( int index, int iTeam, bool bCanCap )
{
	AssertValidIndex( index );
	m_bTeamCanCap.Set( TEAM_ARRAY( index, iTeam ), bCanCap );
	UpdateCapHudElement();
}


void CTFObjectiveResource::SetBaseCP( int index, int iTeam )
{
	Assert( iTeam < MAX_TEAMS );
	m_iBaseControlPoints.Set( iTeam, index );
}


void CTFObjectiveResource::SetPreviousPoint( int index, int iTeam, int iPrevIndex, int iPrevPoint )
{
	AssertValidIndex( index );
	Assert( iPrevIndex >= 0 && iPrevIndex < MAX_PREVIOUS_POINTS );
	int iIntIndex = iPrevIndex + ( index * MAX_PREVIOUS_POINTS ) + ( iTeam * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS );
	m_iPreviousPoints.Set( iIntIndex, iPrevPoint );
}


int CTFObjectiveResource::GetPreviousPointForPoint( int index, int team, int iPrevIndex )
{
	AssertValidIndex( index );
	Assert( iPrevIndex >= 0 && iPrevIndex < MAX_PREVIOUS_POINTS );
	int iIntIndex = iPrevIndex + ( index * MAX_PREVIOUS_POINTS ) + ( team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS );
	return m_iPreviousPoints[iIntIndex];
}


bool CTFObjectiveResource::TeamCanCapPoint( int index, int team )
{
	AssertValidIndex( index );
	return m_bTeamCanCap[TEAM_ARRAY( index, team )];
}

//-----------------------------------------------------------------------------
// Purpose: Data setting functions
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetNumPlayers( int index, int team, int iNumPlayers )
{
	AssertValidIndex( index );
	m_iNumTeamMembers.Set( TEAM_ARRAY( index, team ), iNumPlayers );
	UpdateCapHudElement();
}


void CTFObjectiveResource::StartCap( int index, int team )
{
	AssertValidIndex( index );
	if ( m_iCappingTeam.Get( index ) != team )
	{
		m_iCappingTeam.Set( index, team );
		UpdateCapHudElement();
	}
}


void CTFObjectiveResource::SetOwningTeam( int index, int team )
{
	AssertValidIndex( index );
	m_iOwner.Set( index, team );

	// clear the capper
	m_iCappingTeam.Set( index, TEAM_UNASSIGNED );
	UpdateCapHudElement();

	// For the Total Domination achievement.
	if ( TFGameRules()->IsInDominationMode() && team >= TF_TEAM_RED )
	{
		int iPointsOwnedByTeam = 0;

		for ( int i = 0; i < m_iNumControlPoints; i++ )
		{
			if ( m_iOwner[i] == team )
			{
				iPointsOwnedByTeam++;
			}
		}

		if ( iPointsOwnedByTeam == m_iNumControlPoints )
		{
			CRecipientFilter filter;
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( !pPlayer )
					continue;

				if ( pPlayer->GetTeamNumber() != team )
					continue;

				filter.AddRecipient( pPlayer );
			}
			filter.MakeReliable();

			UserMessageBegin( filter, "AchievementEvent" );
				WRITE_SHORT( TF2C_ACHIEVEMENT_HOLD_ALLPOINTS_DOMINATION );
			MessageEnd();
		}
	}
}


void CTFObjectiveResource::SetCappingTeam( int index, int team )
{
	AssertValidIndex( index );
	if ( m_iCappingTeam.Get( index ) != team )
	{
		m_iCappingTeam.Set( index, team );
		UpdateCapHudElement();
	}
}


void CTFObjectiveResource::SetTeamInZone( int index, int team )
{
	AssertValidIndex( index );
	if ( m_iTeamInZone.Get( index ) != team )
	{
		m_iTeamInZone.Set( index, team );
		UpdateCapHudElement();
	}
}


void CTFObjectiveResource::SetCapBlocked( int index, bool bBlocked )
{
	AssertValidIndex( index );
	if ( m_bBlocked.Get( index ) != bBlocked )
	{
		m_bBlocked.Set( index, bBlocked );
		UpdateCapHudElement();
	}
}


int CTFObjectiveResource::GetOwningTeam( int index )
{
	if ( index >= m_iNumControlPoints )
		return TEAM_UNASSIGNED;

	return m_iOwner[index];
}


void CTFObjectiveResource::UpdateCapHudElement( void )
{
	m_iUpdateCapHudParity = ( m_iUpdateCapHudParity + 1 ) & CAPHUD_PARITY_MASK;
}


void CTFObjectiveResource::SetTrainPathDistance( int index, float flDistance )
{
	AssertValidIndex( index );

	m_flPathDistance.Set( index, flDistance );
}


void CTFObjectiveResource::SetCPLocked( int index, bool bLocked )
{
	// This assert always fires on map load and interferes with daily development
	//AssertValidIndex(index);
	m_bCPLocked.Set( index, bLocked );
}


void CTFObjectiveResource::SetTrackAlarm( int index, bool bAlarm )
{
	Assert( index < TF_TEAM_COUNT );
	m_bTrackAlarm.Set( index, bAlarm );
}


int CTFObjectiveResource::GetDominationRate( int index )
{
	AssertValidIndex( index );
	return m_iDominationRate[index];
}

//-----------------------------------------------------------------------------
// Purpose: Whether CP deterioration is paused
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetCapHalted( int index, bool bHalted )
{
	AssertValidIndex( index );
	if ( m_bHalted.Get( index ) != bHalted )
	{
		m_bHalted.Set( index, bHalted );
		UpdateCapHudElement();
	}
}