//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_playerresource.h"
#include <shareddefs.h>
#include <tf_shareddefs.h>
#include "hud.h"
#include "tf_gamerules.h"
#include "clientsteamcontext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// IF THIS DATATABLE IS  DT_TFPlayerResource  THAN SOURCEMOD DETECTS THIS MOD AS TF2
// THIS IS NO GOOD
// -sappho.io
IMPLEMENT_CLIENTCLASS_DT( C_TF_PlayerResource, DT_TF2CPlayerResource, CTFPlayerResource )
	RecvPropArray3( RECVINFO_ARRAY( m_iTotalScore ), RecvPropInt( RECVINFO( m_iTotalScore[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxHealth ), RecvPropInt( RECVINFO( m_iMaxHealth[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerClass ), RecvPropInt( RECVINFO( m_iPlayerClass[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iActiveDominations ), RecvPropInt( RECVINFO( m_iActiveDominations[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bArenaSpectator ), RecvPropBool( RECVINFO( m_bArenaSpectator[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamageAssist ), RecvPropInt( RECVINFO( m_iDamageAssist[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamageBlocked ), RecvPropInt( RECVINFO( m_iDamageBlocked[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iBonusPoints ), RecvPropInt( RECVINFO( m_iBonusPoints[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerRank ), RecvPropInt( RECVINFO( m_iPlayerRank[0] ) ) ),
END_RECV_TABLE()

C_TF_PlayerResource *g_TF_PR = NULL;


C_TF_PlayerResource::C_TF_PlayerResource()
{
	m_Colors[TEAM_UNASSIGNED] = Color( 245, 229, 196, 255 );
	m_Colors[TEAM_SPECTATOR] = Color( 245, 229, 196, 255 );
	m_Colors[TF_TEAM_RED] = COLOR_RED;
	m_Colors[TF_TEAM_BLUE] = COLOR_BLUE;
	m_Colors[TF_TEAM_GREEN] = COLOR_GREEN;
	m_Colors[TF_TEAM_YELLOW] = COLOR_YELLOW;

	g_TF_PR = this;
}


C_TF_PlayerResource::~C_TF_PlayerResource()
{
	g_TF_PR = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetArrayValue( int iIndex, int *pArray, int iDefaultVal )
{
	if ( !IsConnected( iIndex ) )
		return iDefaultVal;

	return pArray[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
bool C_TF_PlayerResource::GetArrayValue( int iIndex, bool *pArray, bool bDefaultVal )
{
	if ( !IsConnected( iIndex ) )
		return bDefaultVal;

	return pArray[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is an enemy of the local player.
//-----------------------------------------------------------------------------
bool C_TF_PlayerResource::IsEnemyPlayer( int iIndex )
{
	// Spectators are nobody's enemy.
	int iLocalTeam = GetLocalPlayerTeam();
	if ( iLocalTeam < FIRST_GAME_TEAM )
		return false;

	// Players from other teams are enemies.
	return ( GetTeam( iIndex ) != iLocalTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is dominated by the local player.
//-----------------------------------------------------------------------------
bool C_TF_PlayerResource::IsPlayerDominated( int iIndex )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	return pPlayer->m_Shared.IsPlayerDominated( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is dominating the local player.
//-----------------------------------------------------------------------------
bool C_TF_PlayerResource::IsPlayerDominating( int iIndex )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	return pPlayer->m_Shared.IsPlayerDominatingMe( iIndex );
}


int C_TF_PlayerResource::GetUserID( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	player_info_t pi;
	if ( engine->GetPlayerInfo( iIndex, &pi ) )
		return pi.userID;

	return 0;
}


bool C_TF_PlayerResource::GetSteamID( int iIndex, CSteamID *pID )
{
	if ( !IsConnected( iIndex ) )
		return false;

	// Copied from C_BasePlayer.
	player_info_t pi;
	if ( engine->GetPlayerInfo( iIndex, &pi ) && pi.friendsID && ClientSteamContext().BLoggedOn() )
	{
		pID->InstancedSet( pi.friendsID, 1, ClientSteamContext().GetConnectedUniverse(), k_EAccountTypeIndividual );
		return true;
	}

	return false;
}


int C_TF_PlayerResource::GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer /*= false*/ )
{
	int iCount = 0;
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( bExcludeLocalPlayer && ( i == iLocalPlayerIndex ) )
			continue;

		if ( GetTeam( i ) == iTeam && GetPlayerClass( i ) == iClass )
		{
			iCount++;
		}
	}

	return iCount;
}


int C_TF_PlayerResource::GetNumPlayersForTeam( int iTeam, bool bAlive /*= false*/ )
{
	int iCount = 0;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !IsConnected( i ) )
			continue;

		if ( GetTeam( i ) == iTeam && ( !bAlive || IsAlive( i ) ) )
		{
			iCount++;
		}
	}

	return iCount;
}


int C_TF_PlayerResource::GetPlayerRank( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return TF_RANK_NONE;

	return m_iPlayerRank[iIndex];
}
