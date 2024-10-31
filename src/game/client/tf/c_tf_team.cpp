//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client side C_TFTeam class
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_team.h"
#include "tf_shareddefs.h"
#include <vgui/ILocalize.h>
#include "c_baseobject.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_TeamObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFTeam *pPlayer = (C_TFTeam*)pStruct;
	CBaseHandle *pHandle = (CBaseHandle*)(&(pPlayer->m_aObjects[pData->m_iElement])); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}

void RecvProxyArrayLength_TeamObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFTeam *pPlayer = (C_TFTeam*)pStruct;

	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}
}

IMPLEMENT_CLIENTCLASS_DT( C_TFTeam, DT_TFTeam, CTFTeam )

	RecvPropInt( RECVINFO( m_nFlagCaptures ) ),
	RecvPropInt( RECVINFO( m_iRole ) ),
	RecvPropBool( RECVINFO( m_bEscorting ) ),
	RecvPropInt( RECVINFO( m_iRoundScore ) ),

	RecvPropArray2( 
	RecvProxyArrayLength_TeamObjects,
	RecvPropInt( "team_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_TeamObjectList ), 
	MAX_PLAYERS * MAX_OBJECTS_PER_PLAYER, 
	0, 
	"team_object_array"	),

	RecvPropInt( RECVINFO( m_iVIP ) ),

END_RECV_TABLE()


C_TFTeam::C_TFTeam()
{
	m_nFlagCaptures = 0;
	m_wszLocalizedTeamName[0] = '\0';
}


C_TFTeam::~C_TFTeam()
{
}


C_TFPlayer *C_TFTeam::GetVIP( void )
{
	return ToTFPlayer( UTIL_PlayerByIndex( m_iVIP ) );
}


void C_TFTeam::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		UpdateTeamName();
		SetNextClientThink( gpGlobals->curtime + 0.5f );
	}
}


void C_TFTeam::ClientThink( void )
{
	UpdateTeamName();
	SetNextClientThink( gpGlobals->curtime + 0.5f );
}


int C_TFTeam::GetNumObjects( int iObjectType )
{
	// Asking for a count of a specific object type?
	if ( iObjectType > 0 )
	{
		int iCount = 0;
		for ( int i = 0; i < GetNumObjects(); i++ )
		{
			CBaseObject *pObject = GetObject(i);
			if ( pObject && pObject->GetType() == iObjectType )
			{
				iCount++;
			}
		}
		return iCount;
	}

	return m_aObjects.Count();
}


CBaseObject *C_TFTeam::GetObject( int num )
{
	Assert( num >= 0 && num < m_aObjects.Count() );
	return m_aObjects[ num ];
}


void C_TFTeam::UpdateTeamName( void )
{
	// TODO: Add tournament mode team name handling here.
	const wchar_t *pwszLocalized = g_pVGuiLocalize->Find( g_aTeamNames_Localized[GetTeamNumber()] );
	if ( pwszLocalized )
	{
		V_wcscpy_safe( m_wszLocalizedTeamName, pwszLocalized );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( g_aTeamNames[GetTeamNumber()], m_wszLocalizedTeamName, sizeof( m_wszLocalizedTeamName ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the C_TFTeam for the specified team number
//-----------------------------------------------------------------------------
C_TFTeam *GetGlobalTFTeam( int iTeamNumber )
{
	return assert_cast<C_TFTeam *>( GetGlobalTeam( iTeamNumber ) );
}

const wchar_t *GetLocalizedTeamName( int iTeamNumber )
{
	C_TFTeam *pTeam = GetGlobalTFTeam( iTeamNumber );
	if ( pTeam )
		return pTeam->GetTeamName();

	return L"";
}
