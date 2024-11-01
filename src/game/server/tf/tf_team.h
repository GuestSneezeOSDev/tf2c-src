//========= Copyright � 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TF_TEAM_H
#define TF_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "team.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

class CBaseObject;
class CTFPlayer;

//=============================================================================
// TF Teams.
//
class CTFTeam : public CTeam
{
	DECLARE_CLASS( CTFTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

	CTFTeam();

	virtual void	Init( const char *pName, int iNumber );

	// Classes.
//	int				GetNumOfClass( TFClass iClass );
	CTFPlayer		*GetVIP( void );
	void			SetVIP( CTFPlayer *pPlayer );

	// Score.
	void			ShowScore( CBasePlayer *pPlayer );

	CTFPlayer		*GetTFPlayer( int iIndex );

	// Objects.
	void			AddObject( CBaseObject *pObject );
	void			RemoveObject( CBaseObject *pObject );
	bool			IsObjectOnTeam( CBaseObject *pObject ) const;
	int				GetNumObjects( int iObjectType = -1 );
	CBaseObject		*GetObject( int num );

	// Flag Captures
	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }
	void			SetFlagCaptures( int nCaptures ) { m_nFlagCaptures = nCaptures; }
	void			IncrementFlagCaptures( void ) { m_nFlagCaptures++; }

	// Points
	int				GetRoundScore( void ) { return m_iRoundScore; }
	void			AddRoundScore( int nPoints );
	void			SetRoundScore( int nPoints );
	void			ResetRoundScore( void );

	int				GetControlPointsOwned( void );
	int				GetDominationPointRate( void );

	// Roles
	void			SetRole( int iTeamRole ) { m_iRole = iTeamRole; }
	int				GetRole( void ) { return m_iRole; }

	void			SetEscorting( bool bEscorting ) { m_bEscorting = bEscorting; }
	bool			IsEscorting( void ) { return m_bEscorting; }

	int				GetWinCount( void ) { return m_iWins; }
	void			SetWinCount( int iWins ) { m_iWins = iWins; }
	void			IncrementWins( void ) { m_iWins++; }
	void			ResetWins( void ) { m_iWins = 0; }

private:

	CUtlVector< CHandle<CBaseObject> >	m_aObjects;			// List of team objects.

	CNetworkVar( int, m_nFlagCaptures );
	CNetworkVar( int, m_iRole );
	CNetworkVar( bool, m_bEscorting );
	CNetworkVar( int, m_iRoundScore );
	CNetworkVar( int, m_iControlPointsOwned );
	CNetworkVar( int, m_iVIP );
	int m_iWins;
};

class CTFTeamManager
{
public:

	CTFTeamManager();

	// Creation/Destruction.
	bool	Init( void );
	void    Shutdown( void );
	void	RemoveExtraTeams( void );

	bool	IsValidTeam( int iTeam );
	int		GetTeamCount( void );
	CTFTeam *GetTeam( int iTeam );
	CTFTeam *GetSpectatorTeam();
	CTFTeam *GetTeamByRole( int iRole );

	color32 GetUndefinedTeamColor( void );

	void AddTeamScore( int iTeam, int iScoreToAdd );

	void IncrementFlagCaptures( int iTeam );
	int GetFlagCaptures( int iTeam );

	// Screen prints.
	void PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void TeamCenterPrint( int iTeam, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

	// Vox

private:

	int		Create( const char *pName );

private:

	color32	m_UndefinedTeamColor;
};

extern CTFTeamManager *TFTeamMgr();
extern CTFTeam *GetGlobalTFTeam( int iIndex );

template<typename Functor>
bool ForEachTFTeam( Functor&& func )
{
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		if ( !func( i ) )
			return false;
	}

	return true;
}

template<typename Functor>
bool ForEachEnemyTFTeam( int iTeam, Functor&& func )
{
	// Iterate over every game team except for ourselves.
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		if ( i != iTeam )
		{
			if ( !func( i ) )
				return false;
		}
	}

	return true;
}

#endif // TF_TEAM_H
