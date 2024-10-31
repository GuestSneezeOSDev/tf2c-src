//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: DOD's objective resource, transmits all objective states to players
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJECTIVE_RESOURCE_H
#define TF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#define TEAM_ARRAY( index, team )		(index + (team * MAX_CONTROL_POINTS))

class CTFObjectiveResource : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFObjectiveResource, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFObjectiveResource();
	~CTFObjectiveResource();

	virtual void Spawn( void );
	virtual int  UpdateTransmitState( void );

	virtual void ObjectiveThink( void );

	//--------------------------------------------------------------------
	// CONTROL POINT DATA
	//--------------------------------------------------------------------
public:
	void ResetControlPoints( void );

	// Data functions, called to set up the state at the beginning of a round
	void SetNumControlPoints( int num );
	int	 GetNumControlPoints( void ) { return m_iNumControlPoints; }
	void SetCPIcons( int index, int iTeam, int iIcon );
	void SetCPOverlays( int index, int iTeam, int iIcon );
	void SetTeamBaseIcons( int iTeam, int iBaseIcon );
	void SetCPPosition( int index, const Vector& vPosition );
	void SetCPVisible( int index, bool bVisible );
	void SetCPRequiredCappers( int index, int iTeam, int iReqPlayers );
	void SetCPCapTime( int index, int iTeam, float flTime );
	void SetCPCapPercentage( int index, float flTime );
	float GetCPCapPercentage( int index );
	void SetTeamCanCap( int index, int iTeam, bool bCanCap );
	void SetBaseCP( int index, int iTeam );
	void SetPreviousPoint( int index, int iTeam, int iPrevIndex, int iPrevPoint );
	int GetPreviousPointForPoint( int index, int team, int iPrevIndex );
	bool TeamCanCapPoint( int index, int team );
	void SetCapLayoutInHUD( const char *pszLayout ) { Q_strncpy( m_pszCapLayoutInHUD.GetForModify(), pszLayout, MAX_CAPLAYOUT_LENGTH ); }
	void SetCapLayoutCustomPosition( float flPositionX, float flPositionY ) { m_flCustomPositionX = flPositionX; m_flCustomPositionY = flPositionY; }
	void SetWarnOnCap( int index, int iWarnLevel );
	void SetWarnSound( int index, string_t iszSound );
	void SetCPGroup( int index, int iCPGroup );
	void SetCPLocked( int index, bool bLocked );
	void SetTrackAlarm( int index, bool bAlarm );
	void SetCPUnlockTime( int index, float flTime );
	void SetCPTimerTime( int index, float flTime );
	void SetCPCapTimeScalesWithPlayers( int index, bool bScales );

	// State functions, called many times
	void SetNumPlayers( int index, int team, int iNumPlayers );
	void StartCap( int index, int team );
	void SetOwningTeam( int index, int team );
	void SetCappingTeam( int index, int team );
	void SetTeamInZone( int index, int team );
	void SetCapBlocked( int index, bool bBlocked );
	int  GetOwningTeam( int index );
	void SetCapHalted( int index, bool bHalted );

	void AssertValidIndex( int index )
	{
		Assert( 0 <= index && index < MAX_CONTROL_POINTS && index < m_iNumControlPoints );
	}

	int GetBaseControlPointForTeam( int iTeam )
	{
		Assert( iTeam < MAX_TEAMS );
		return m_iBaseControlPoints[iTeam];
	}

	int GetCappingTeam( int index )
	{
		if ( index >= m_iNumControlPoints )
			return TEAM_UNASSIGNED;

		return m_iCappingTeam[index];
	}

	// Number of players in the area
	int GetNumPlayersInArea( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_iNumTeamMembers[TEAM_ARRAY( index, team )];
	}

	void SetTimerInHUD( CBaseEntity *pTimer )
	{
		m_iTimerToShowInHUD = pTimer ? pTimer->entindex() : 0;
	}


	void SetStopWatchTimer( CBaseEntity *pTimer )
	{
		m_iStopWatchTimer = pTimer ? pTimer->entindex() : 0;
	}

	int GetTimerInHUD( void ) { return m_iTimerToShowInHUD; }
	bool IsActiveTimer( CBaseEntity *pTimer ) { return ( pTimer->entindex() == m_iTimerToShowInHUD ); }

	// Mini-rounds data
	void SetPlayingMiniRounds( bool bPlayingMiniRounds ){ m_bPlayingMiniRounds = bPlayingMiniRounds; }
	bool PlayingMiniRounds( void ){ return m_bPlayingMiniRounds; }
	void SetInMiniRound( int index, bool bInRound ) { m_bInMiniRound.Set( index, bInRound ); }
	bool IsInMiniRound( int index ) { return m_bInMiniRound[index]; }

	void UpdateCapHudElement( void );

	// Train Path data
	void SetTrainPathDistance( int index, float flDistance );

	bool GetCPLocked( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bCPLocked[index];
	}

	// Whether capture is currently being blocked
	bool IsCPBlocked( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bBlocked[index];
	}

	void ResetHillData( int team )
	{
		if ( team < TF_TEAM_COUNT )
		{
			m_nNumNodeHillData.Set( team, 0 );

			int nNumEntriesPerTeam = TF_TRAIN_MAX_HILLS * TF_TRAIN_FLOATS_PER_HILL;
			int iStartingIndex = team * nNumEntriesPerTeam;
			for ( int i = 0; i < nNumEntriesPerTeam; i++ )
			{
				m_flNodeHillData.Set( iStartingIndex + i, 0 );
			}

			iStartingIndex = team * TF_TRAIN_MAX_HILLS;
			for ( int i = 0; i < TF_TRAIN_MAX_HILLS; i++ )
			{
				m_bHillIsDownhill.Set( iStartingIndex + i, 0 );
			}
		}
	}

	void SetHillData( int team, float flStart, float flEnd, bool bDownhill )
	{
		if ( team < TF_TEAM_COUNT )
		{
			int index = ( m_nNumNodeHillData[team] * TF_TRAIN_FLOATS_PER_HILL ) + ( team * TF_TRAIN_MAX_HILLS * TF_TRAIN_FLOATS_PER_HILL );
			if ( index < TF_TRAIN_HILLS_ARRAY_SIZE - 1 ) // - 1 because we want to add 2 entries
			{
				m_flNodeHillData.Set( index, flStart );
				m_flNodeHillData.Set( index + 1, flEnd );

				if ( m_nNumNodeHillData[team] < TF_TRAIN_MAX_HILLS )
				{
					m_bHillIsDownhill.Set( m_nNumNodeHillData[team] + ( team * TF_TRAIN_MAX_HILLS ), bDownhill );
				}

				m_nNumNodeHillData.Set( team, m_nNumNodeHillData[team] + 1 );
			}
		}
	}

	float GetVIPProgress( int iTeam ) { return m_flVIPProgress[iTeam]; }
	void SetVIPProgress( float flValue, int iTeam ) { m_flVIPProgress.Set( iTeam, flValue ); }

	int GetDominationRate( int index );
	void SetDominationRate( int index, int iRate ) { m_iDominationRate.Set( index, iRate ); };

	// Whether CP deterioration is being halted, e.g. by a VIP teammate
	bool IsCPHalted( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bHalted[index];
	}

	bool GetShouldDisplayObjectiveHUD() { return m_bDisplayObjectiveHUD; }
	void SetDisplayObjectiveHUD( bool bShouldDisplay ) { m_bDisplayObjectiveHUD = bShouldDisplay; }

private:
	CNetworkVar( int, m_iTimerToShowInHUD );
	CNetworkVar( int, m_iStopWatchTimer );

	CNetworkVar( int, m_iNumControlPoints );
	CNetworkVar( bool, m_bPlayingMiniRounds );
	CNetworkVar( bool, m_bControlPointsReset );
	CNetworkVar( int, m_iUpdateCapHudParity );

	// data variables
	CNetworkArray( Vector, m_vCPPositions, MAX_CONTROL_POINTS );
	CNetworkArray( int, m_bCPIsVisible, MAX_CONTROL_POINTS );
	CNetworkArray( float, m_flLazyCapPerc, MAX_CONTROL_POINTS );
	CNetworkArray( int, m_iTeamIcons, MAX_CONTROL_POINTS * TF_TEAM_COUNT );
	CNetworkArray( int, m_iTeamOverlays, MAX_CONTROL_POINTS * TF_TEAM_COUNT );
	CNetworkArray( int, m_iTeamReqCappers, MAX_CONTROL_POINTS * TF_TEAM_COUNT );
	CNetworkArray( float, m_flTeamCapTime, MAX_CONTROL_POINTS * TF_TEAM_COUNT );
	CNetworkArray( int, m_iPreviousPoints, MAX_CONTROL_POINTS * TF_TEAM_COUNT * MAX_PREVIOUS_POINTS );
	CNetworkArray( bool, m_bTeamCanCap, MAX_CONTROL_POINTS * TF_TEAM_COUNT );
	CNetworkArray( int, m_iTeamBaseIcons, MAX_TEAMS );
	CNetworkArray( int, m_iBaseControlPoints, MAX_TEAMS );
	CNetworkArray( bool, m_bInMiniRound, MAX_CONTROL_POINTS );
	CNetworkArray( int, m_iWarnOnCap, MAX_CONTROL_POINTS );
	CNetworkArray( string_t, m_iszWarnSound, MAX_CONTROL_POINTS );
	CNetworkArray( float, m_flPathDistance, MAX_CONTROL_POINTS );
	CNetworkArray( bool, m_bCPLocked, MAX_CONTROL_POINTS );
	CNetworkArray( float, m_flUnlockTimes, MAX_CONTROL_POINTS );
	CNetworkArray( float, m_flCPTimerTimes, MAX_CONTROL_POINTS );

	// change when players enter/exit an area
	CNetworkArray( int, m_iNumTeamMembers, MAX_CONTROL_POINTS * TF_TEAM_COUNT );

	// changes when a cap starts. start and end times are calculated on client
	CNetworkArray( int, m_iCappingTeam, MAX_CONTROL_POINTS );

	CNetworkArray( int, m_iTeamInZone, MAX_CONTROL_POINTS );
	CNetworkArray( bool, m_bBlocked, MAX_CONTROL_POINTS );

	// changes when a point is successfully captured
	CNetworkArray( int, m_iOwner, MAX_CONTROL_POINTS );
	CNetworkArray( bool, m_bCPCapRateScalesWithPlayers, MAX_CONTROL_POINTS );

	// describes how to lay out the cap points in the hud
	CNetworkString( m_pszCapLayoutInHUD, MAX_CAPLAYOUT_LENGTH );

	// custom screen position for the cap points in the hud
	CNetworkVar( float, m_flCustomPositionX );
	CNetworkVar( float, m_flCustomPositionY );

	// the groups the points belong to
	CNetworkArray( int, m_iCPGroup, MAX_CONTROL_POINTS );

	// Not networked, because the client recalculates it
	float	m_flCapPercentages[MAX_CONTROL_POINTS];

	// hill data for multi-escort payload maps
	CNetworkArray( int, m_nNumNodeHillData, TF_TEAM_COUNT );
	CNetworkArray( float, m_flNodeHillData, TF_TRAIN_HILLS_ARRAY_SIZE );

	CNetworkArray( bool, m_bTrackAlarm, TF_TEAM_COUNT );
	CNetworkArray( bool, m_bHillIsDownhill, TF_TRAIN_MAX_HILLS*TF_TEAM_COUNT );

	CNetworkArray( float, m_flVIPProgress, TF_TEAM_COUNT );
	CNetworkArray( int, m_iDominationRate, MAX_CONTROL_POINTS );

	CNetworkArray( bool, m_bHalted, MAX_CONTROL_POINTS );

	CNetworkVar( bool, m_bDisplayObjectiveHUD );
};

extern CTFObjectiveResource *g_pObjectiveResource;

inline CTFObjectiveResource *ObjectiveResource()
{
	return g_pObjectiveResource;
}

inline CTFObjectiveResource *TFObjectiveResource( void )
{
	return g_pObjectiveResource;
}

#endif	// TF_OBJECTIVE_RESOURCE_H

