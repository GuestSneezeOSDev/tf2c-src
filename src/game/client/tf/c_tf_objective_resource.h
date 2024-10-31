//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_OBJECTIVE_RESOURCE_H
#define C_TF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>

class C_TeamRoundTimer;

#define TEAM_ARRAY( index, team )		(index + (team * MAX_CONTROL_POINTS))

class C_TFObjectiveResource : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFObjectiveResource, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFObjectiveResource();
	~C_TFObjectiveResource();

	virtual void	ClientThink();
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	void			UpdateControlPoint( const char *pszEvent, int index = -1 );
	float			GetCPCapPercentage( int index );
	int				GetNumControlPoints( void ) { return m_iNumControlPoints; }
	int				GetNumControlPointsOwned( void );
	void			SetOwningTeam( int index, int team );
	void			SetCappingTeam( int index, int team );
	void			SetCapLayout( const char *pszLayout );

	// Is the point visible in the objective display
	bool	IsCPVisible( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bCPIsVisible[index];
	}

	bool	IsCPBlocked( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bBlocked[index];
	}

	// Get the world location of this control point
	Vector& GetCPPosition( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_vCPPositions[index];
	}

	int GetOwningTeam( int index )
	{
		if ( index >= m_iNumControlPoints )
			return TEAM_UNASSIGNED;

		return m_iOwner[index];
	}

	int GetCappingTeam( int index )
	{
		if ( index >= m_iNumControlPoints )
			return TEAM_UNASSIGNED;

		return m_iCappingTeam[index];
	}

	int GetTeamInZone( int index )
	{
		if ( index >= m_iNumControlPoints )
			return TEAM_UNASSIGNED;

		return m_iTeamInZone[index];
	}

	// Icons
	int GetCPCurrentOwnerIcon( int index, int iOwner )
	{
		Assert( index < m_iNumControlPoints );

		return GetIconForTeam( index, iOwner );
	}

	int GetCPCappingIcon( int index )
	{
		Assert( index < m_iNumControlPoints );

		int iCapper = GetCappingTeam( index );

		Assert( iCapper != TEAM_UNASSIGNED );

		return GetIconForTeam( index, iCapper );
	}

	// Icon for the specified team
	int GetIconForTeam( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_iTeamIcons[TEAM_ARRAY( index, team )];
	}

	// Overlay for the specified team
	int GetOverlayForTeam( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_iTeamOverlays[TEAM_ARRAY( index, team )];
	}

	// Number of players in the area
	int GetNumPlayersInArea( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_iNumTeamMembers[TEAM_ARRAY( index, team )];
	}

	// get the required cappers for the passed team
	int GetRequiredCappers( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_iTeamReqCappers[TEAM_ARRAY( index, team )];
	}

	// Base Icon for the specified team
	int GetBaseIconForTeam( int team )
	{
		Assert( team < MAX_TEAMS );
		return m_iTeamBaseIcons[team];
	}

	int GetBaseControlPointForTeam( int iTeam )
	{
		Assert( iTeam < MAX_TEAMS );
		return m_iBaseControlPoints[iTeam];
	}

	int GetPreviousPointForPoint( int index, int team, int iPrevIndex )
	{
		Assert( index < m_iNumControlPoints );
		Assert( iPrevIndex >= 0 && iPrevIndex < MAX_PREVIOUS_POINTS );
		int iIntIndex = iPrevIndex + ( index * MAX_PREVIOUS_POINTS ) + ( team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS );
		return m_iPreviousPoints[iIntIndex];
	}

	bool TeamCanCapPoint( int index, int team )
	{
		Assert( index < m_iNumControlPoints );
		return m_bTeamCanCap[TEAM_ARRAY( index, team )];
	}

	const char *GetCapLayoutInHUD( void ) { return m_pszCapLayoutInHUD; }
	void GetCapLayoutCustomPosition( float& flCustomPositionX, float& flCustomPositionY ) { flCustomPositionX = m_flCustomPositionX; flCustomPositionY = m_flCustomPositionY; }

	bool PlayingMiniRounds( void ){ return m_bPlayingMiniRounds; }
	bool IsInMiniRound( int index ) { return m_bInMiniRound[index]; }

	int GetCapWarningLevel( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_iWarnOnCap[index];
	}

	int GetCPGroup( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_iCPGroup[index];
	}

	const char *GetWarnSound( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_iszWarnSound[index];
	}

	const char *GetGameSpecificCPCappingSwipe( int index, int iCappingTeam );
	const char *GetGameSpecificCPBarFG( int index, int iOwningTeam );
	const char *GetGameSpecificCPBarBG( int index, int iCappingTeam );

	bool CapIsBlocked( int index );

	C_TeamRoundTimer *GetTimerToShowInHUD( void );
	int GetStopWatchTimer( void ) { return m_iStopWatchTimer; }

	float GetPathDistance( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_flPathDistance[index];
	}

	bool GetCPLocked( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bCPLocked[index];
	}

	bool GetTrackAlarm( int index )
	{
		Assert( index < TF_TEAM_COUNT );
		return m_bTrackAlarm[index];
	}

	int GetNumNodeHillData( int team ){ return ( team < TF_TEAM_COUNT ) ? m_nNumNodeHillData[team] : 0; }

	void GetHillData( int team, int hill, float &flStart, float &flEnd )
	{
		if ( hill < TF_TRAIN_MAX_HILLS && team < TF_TEAM_COUNT )
		{
			int index = ( hill * TF_TRAIN_FLOATS_PER_HILL ) + ( team * TF_TRAIN_MAX_HILLS * TF_TRAIN_FLOATS_PER_HILL );
			if ( index < TF_TRAIN_HILLS_ARRAY_SIZE - 1 ) // - 1 because we want to look at 2 entries
			{
				flStart = m_flNodeHillData[index];
				flEnd = m_flNodeHillData[index + 1];
			}
		}
	}

	void SetTrainOnHill( int team, int hill, bool state )
	{
		if ( team < TF_TEAM_COUNT && hill < TF_TRAIN_MAX_HILLS )
		{
			int index = hill + ( team * TF_TRAIN_MAX_HILLS );
			m_bTrainOnHill[index] = state;
		}
	}

	bool IsTrainOnHill( int team, int hill )
	{
		if ( team < TF_TEAM_COUNT && hill < TF_TRAIN_MAX_HILLS )
		{
			return m_bTrainOnHill[hill + ( team * TF_TRAIN_MAX_HILLS )];
		}

		return false;
	}

	bool IsHillDownhill( int team, int hill )
	{
		if ( team < TF_TEAM_COUNT && hill < TF_TRAIN_MAX_HILLS )
		{
			return m_bHillIsDownhill[hill + ( team * TF_TRAIN_MAX_HILLS )];
		}

		return true;
	}

	float GetVIPProgress( int iTeam );

	int GetDominationRate( int index );

	bool IsCPHalted( int index )
	{
		Assert( index < m_iNumControlPoints );
		return m_bHalted[index];
	}

	bool GetShouldDisplayObjectiveHUD() { return m_bDisplayObjectiveHUD; }

private:
	int		m_iTimerToShowInHUD;
	int		m_iStopWatchTimer;

	int		m_iNumControlPoints;
	int		m_iPrevNumControlPoints;
	bool	m_bPlayingMiniRounds;
	bool	m_bControlPointsReset;
	bool	m_bOldControlPointsReset;
	int		m_iUpdateCapHudParity;
	int		m_iOldUpdateCapHudParity;

	// data variables
	Vector		m_vCPPositions[MAX_CONTROL_POINTS];
	bool		m_bCPIsVisible[MAX_CONTROL_POINTS];
	float		m_flLazyCapPerc[MAX_CONTROL_POINTS];
	float		m_flOldLazyCapPerc[MAX_CONTROL_POINTS];
	int			m_iTeamIcons[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	int			m_iTeamOverlays[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	int			m_iTeamReqCappers[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	float		m_flTeamCapTime[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	int			m_iPreviousPoints[MAX_CONTROL_POINTS * TF_TEAM_COUNT * MAX_PREVIOUS_POINTS];
	bool		m_bTeamCanCap[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	int			m_iTeamBaseIcons[MAX_TEAMS];
	int			m_iBaseControlPoints[MAX_TEAMS];
	bool		m_bInMiniRound[MAX_CONTROL_POINTS];
	int			m_iWarnOnCap[MAX_CONTROL_POINTS];
	char		m_iszWarnSound[MAX_CONTROL_POINTS][255];
	float		m_flPathDistance[MAX_CONTROL_POINTS];
	int			m_iCPGroup[MAX_CONTROL_POINTS];
	bool		m_bCPLocked[MAX_CONTROL_POINTS];
	float		m_flUnlockTimes[MAX_CONTROL_POINTS];
	float		m_flOldUnlockTimes[MAX_CONTROL_POINTS];
	float		m_flCPTimerTimes[MAX_CONTROL_POINTS];
	float		m_flOldCPTimerTimes[MAX_CONTROL_POINTS];

	// state variables
	int		m_iNumTeamMembers[MAX_CONTROL_POINTS * TF_TEAM_COUNT];
	int		m_iCappingTeam[MAX_CONTROL_POINTS];
	int		m_iTeamInZone[MAX_CONTROL_POINTS];
	bool	m_bBlocked[MAX_CONTROL_POINTS];
	int		m_iOwner[MAX_CONTROL_POINTS];
	bool	m_bCPCapRateScalesWithPlayers[MAX_CONTROL_POINTS];

	// client calculated state
	float	m_flCapTimeLeft[MAX_CONTROL_POINTS];
	float	m_flCapLastThinkTime[MAX_CONTROL_POINTS];

	bool	m_bWarnedOnFinalCap[MAX_CONTROL_POINTS];
	float	m_flLastCapWarningTime[MAX_CONTROL_POINTS];
	char	m_pszCapLayoutInHUD[MAX_CAPLAYOUT_LENGTH];
	float	m_flOldCustomPositionX;
	float	m_flOldCustomPositionY;
	float	m_flCustomPositionX;
	float	m_flCustomPositionY;

	// hill data for multi-escort payload maps
	int		m_nNumNodeHillData[TF_TEAM_COUNT];
	float	m_flNodeHillData[TF_TRAIN_HILLS_ARRAY_SIZE];
	bool	m_bTrainOnHill[TF_TRAIN_MAX_HILLS*TF_TEAM_COUNT];

	bool	m_bTrackAlarm[TF_TEAM_COUNT];
	bool	m_bHillIsDownhill[TF_TRAIN_MAX_HILLS*TF_TEAM_COUNT];

	float	m_flVIPProgress[TF_TEAM_COUNT];
	int		m_iDominationRate[MAX_CONTROL_POINTS];
	bool	m_bHalted[MAX_CONTROL_POINTS];

	bool	m_bDisplayObjectiveHUD;
};

extern C_TFObjectiveResource *g_pObjectiveResource;

inline C_TFObjectiveResource *ObjectiveResource()
{
	return g_pObjectiveResource;
}

inline C_TFObjectiveResource *TFObjectiveResource()
{
	return g_pObjectiveResource;
}

#endif // C_TF_OBJECTIVE_RESOURCE_H
