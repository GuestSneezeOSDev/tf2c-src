//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PLAYERRESOURCE_H
#define C_TF_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "c_playerresource.h"

class C_TF_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_TF_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_TF_PlayerResource();
	virtual ~C_TF_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetMaxHealth( int iIndex ) { return GetArrayValue( iIndex, m_iMaxHealth, 1 ); }
	int GetPlayerClass( int iIndex ) { return GetArrayValue( iIndex, m_iPlayerClass, TF_CLASS_UNDEFINED ); }
	int GetNumberOfDominations( int iIndex ) { return GetArrayValue( iIndex, m_iActiveDominations, 0 ); }
	bool IsArenaSpectator( int iIndex ) { return GetArrayValue( iIndex, m_bArenaSpectator ); }
	int GetDamageAssist( int iIndex ) { return GetArrayValue( iIndex, m_iDamageAssist, 0 ); }
	int GetDamageBlocked( int iIndex ) { return GetArrayValue( iIndex, m_iDamageBlocked, 0 ); }
	int GetBonusPoints( int iIndex ) { return GetArrayValue( iIndex, m_iBonusPoints, 0 ); }

	bool IsPlayerDominated( int iIndex );
	bool IsPlayerDominating( int iIndex );
	bool IsEnemyPlayer( int iIndex );
	int GetUserID( int iIndex );
	bool GetSteamID( int iIndex, CSteamID *pID );

	int GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer = false );
	int GetNumPlayersForTeam( int iTeam, bool bAlive = false );

	int GetPlayerRank( int iIndex );

protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );
	bool GetArrayValue( int iIndex, bool *pArray, bool defaultVal = false );

	int		m_iTotalScore[MAX_PLAYERS + 1];
	int		m_iMaxHealth[MAX_PLAYERS + 1];
	int		m_iPlayerClass[MAX_PLAYERS + 1];
	int		m_iActiveDominations[MAX_PLAYERS + 1];
	bool	m_bArenaSpectator[MAX_PLAYERS + 1];
	int		m_iDamageAssist[MAX_PLAYERS + 1];
	int		m_iDamageBlocked[MAX_PLAYERS + 1];
	int		m_iBonusPoints[MAX_PLAYERS + 1];
	int		m_iPlayerRank[MAX_PLAYERS + 1];

};

extern C_TF_PlayerResource *g_TF_PR;

#endif // C_TF_PLAYERRESOURCE_H
