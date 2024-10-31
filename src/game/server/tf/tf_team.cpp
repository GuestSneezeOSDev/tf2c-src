//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#include "cbase.h"
#include "tf_team.h"
#include "entitylist.h"
#include "util.h"
#include "tf_obj.h"
#include "tf_gamerules.h"
#include "tf_player.h"
#include "tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_TeamObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFTeam *pTeam = (CTFTeam*)pStruct;

	Assert( iElement < pTeam->GetNumObjects() );

	CBaseObject *pObject = pTeam->GetObject(iElement);

	EHANDLE hObject;
	hObject = pObject;

	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

int SendProxyArrayLength_TeamObjects( const void *pStruct, int objectID )
{
	CTFTeam *pTeam = (CTFTeam*)pStruct;
	int iObjects = pTeam->GetNumObjects();
	Assert( iObjects <= MAX_PLAYERS * MAX_OBJECTS_PER_PLAYER );
	return iObjects;
}

//=============================================================================
//
// TF Team tables.
//
IMPLEMENT_SERVERCLASS_ST( CTFTeam, DT_TFTeam )

	SendPropInt( SENDINFO( m_nFlagCaptures ), 8 ),
	SendPropInt( SENDINFO( m_iRole ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bEscorting ) ),
	SendPropInt( SENDINFO( m_iRoundScore ) ),

	SendPropArray2( 
	SendProxyArrayLength_TeamObjects,
	SendPropInt( "team_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_TeamObjectList ), 
	MAX_PLAYERS * MAX_OBJECTS_PER_PLAYER, 
	0, 
	"team_object_array"
	),

	SendPropInt( SENDINFO( m_iVIP ) ),

END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( tf_team, CTFTeam );

//=============================================================================
//
// TF Team Manager Functions.
//
CTFTeamManager s_TFTeamManager;

CTFTeamManager *TFTeamMgr()
{
	return &s_TFTeamManager;
}


CTFTeamManager::CTFTeamManager()
{
	m_UndefinedTeamColor.r = 255;
	m_UndefinedTeamColor.g = 255;
	m_UndefinedTeamColor.b = 255;
	m_UndefinedTeamColor.a = 0;

}


bool CTFTeamManager::Init( void )
{
	// Clear the list.
	Shutdown();

	// Create the team list.
	for ( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		int index = Create( g_aTeamNames[iTeam] );
		Assert( index == iTeam );
		if ( index != iTeam )
			return false;
	}

	return true;
}


void CTFTeamManager::Shutdown( void )
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
}


int CTFTeamManager::Create( const char *pName )
{
	CTFTeam *pTeam = static_cast<CTFTeam*>( CreateEntityByName( "tf_team" ) );
	if ( pTeam )
	{
		// Add the team to the global list of teams.
		int iTeam = g_Teams.AddToTail( pTeam );

		// Initialize the team.
		pTeam->Init( pName, iTeam );
		pTeam->NetworkProp()->SetUpdateInterval( 0.75f );

		return iTeam;
	}

	// Error.
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Remove any teams that are not active on the current map.
//-----------------------------------------------------------------------------
void CTFTeamManager::RemoveExtraTeams( void )
{
	int nActiveTeams = TF_ORIGINAL_TEAM_COUNT;

	if ( TFGameRules()->IsFourTeamGame() )
	{
		nActiveTeams = TF_TEAM_COUNT;
	}

	for ( int i = g_Teams.Count() - 1; i >= nActiveTeams; i-- )
	{
		UTIL_Remove( g_Teams[i] );
		g_Teams.Remove( i );
	}
}


int CTFTeamManager::GetFlagCaptures( int iTeam )
{
	if ( !IsValidTeam( iTeam ) )
		return -1;

	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return -1;

	return pTeam->GetFlagCaptures();
}


void CTFTeamManager::IncrementFlagCaptures( int iTeam )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->IncrementFlagCaptures();
}


void CTFTeamManager::AddTeamScore( int iTeam, int iScoreToAdd )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->AddScore( iScoreToAdd );
}


bool CTFTeamManager::IsValidTeam( int iTeam )
{
	if ( ( iTeam >= 0 ) && ( iTeam < GetNumberOfTeams() ) )
		return true;

	return false;
}


int	CTFTeamManager::GetTeamCount( void )
{
	return GetNumberOfTeams();
}


CTFTeam *CTFTeamManager::GetTeam( int iTeam )
{
	Assert( IsValidTeam( iTeam ) );

	return GetGlobalTFTeam( iTeam );
}


CTFTeam *CTFTeamManager::GetSpectatorTeam()
{
	return GetTeam( 0 );
}


CTFTeam *CTFTeamManager::GetTeamByRole( int iRole )
{
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );
		if ( pTeam->GetRole() == iRole )
			return pTeam;
	}

	return NULL;
}


color32 CTFTeamManager::GetUndefinedTeamColor( void )
{
	return m_UndefinedTeamColor;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's screen.
//-----------------------------------------------------------------------------
void CTFTeamManager::PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	ClientPrint( pPlayer, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the given teams screen.
//-----------------------------------------------------------------------------
void CTFTeamManager::TeamCenterPrint( int iTeam, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( iTeam, true );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's teams screen (minus
//          the player).
//-----------------------------------------------------------------------------
void CTFTeamManager::PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( pPlayer->GetTeamNumber(), true );
	filter.RemoveRecipient( pPlayer );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//=============================================================================
//
// TF Team Functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFTeam::CTFTeam()
{
	m_iVIP = 0;
	m_iWins = 0;
}


void CTFTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );
}


CTFPlayer *CTFTeam::GetVIP( void )
{
	return ToTFPlayer( UTIL_PlayerByIndex( m_iVIP ) );
}


void CTFTeam::SetVIP( CTFPlayer *pPlayer )
{
	m_iVIP = pPlayer ? pPlayer->entindex() : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return number of team's owned control points
//-----------------------------------------------------------------------------
int CTFTeam::GetControlPointsOwned()
{
	int nPointsOwned = 0;
	
	for ( int iPoint = 0; iPoint < ObjectiveResource()->GetNumControlPoints(); iPoint++ )
	{
		if ( ObjectiveResource()->IsInMiniRound( iPoint ) &&
			ObjectiveResource()->GetOwningTeam( iPoint ) == GetTeamNumber() )
			nPointsOwned++;
	}
	return nPointsOwned;
}

//-----------------------------------------------------------------------------
// Purpose: Return total of team's owned control points' Dom score rate
//-----------------------------------------------------------------------------
int CTFTeam::GetDominationPointRate()
{
	int iTotalPointRate = 0;

	for ( int iPoint = 0; iPoint < ObjectiveResource()->GetNumControlPoints(); iPoint++ )
	{
		if ( ObjectiveResource()->IsInMiniRound( iPoint ) &&
			ObjectiveResource()->GetOwningTeam( iPoint ) == GetTeamNumber() &&
			!ObjectiveResource()->IsCPBlocked( iPoint ) &&
			ObjectiveResource()->GetNumPlayersInArea( iPoint, ObjectiveResource()->GetCappingTeam( iPoint ) ) <= 0 )
			iTotalPointRate += ObjectiveResource()->GetDominationRate( iPoint );
	}
	return iTotalPointRate;
}

//-----------------------------------------------------------------------------
// Purpose:
//   Input: pPlayer - print to just that client, NULL = all clients
//-----------------------------------------------------------------------------
void CTFTeam::ShowScore( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		ClientPrint( pPlayer, HUD_PRINTNOTIFY,  UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
}


CTFPlayer *CTFTeam::GetTFPlayer( int iIndex )
{
	return ToTFPlayer( GetPlayer( iIndex ) );
}

//-----------------------------------------------------------------------------
// OBJECTS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this team.
//-----------------------------------------------------------------------------
void CTFTeam::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::AddObject adding object %p:%s to team %s\n", gpGlobals->curtime, 
		pObject, pObject->GetClassname(), GetName() ) );

	bool alreadyInList = IsObjectOnTeam( pObject );
	Assert( !alreadyInList );
	if ( !alreadyInList )
	{
		m_aObjects.AddToTail( pObject );
	}

	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Returns true if the object is in the team's list of objects
//-----------------------------------------------------------------------------
bool CTFTeam::IsObjectOnTeam( CBaseObject *pObject ) const
{
	return ( m_aObjects.Find( pObject ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this object from the team
//  Removes all references from all sublists as well
//-----------------------------------------------------------------------------
void CTFTeam::RemoveObject( CBaseObject *pObject )
{									   
	if ( m_aObjects.Count() <= 0 )
		return;

	if ( m_aObjects.Find( pObject ) != -1 )
	{
		TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::RemoveObject removing %p:%s from %s\n", gpGlobals->curtime, 
			pObject, pObject->GetClassname(), GetName() ) );

		m_aObjects.FindAndRemove( pObject );
	}
	else
	{
		TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::RemoveObject couldn't remove %p:%s from %s\n", gpGlobals->curtime, 
			pObject, pObject->GetClassname(), GetName() ) );
	}

	NetworkStateChanged();
}


int CTFTeam::GetNumObjects( int iObjectType )
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


CBaseObject *CTFTeam::GetObject( int num )
{
	Assert( num >= 0 && num < m_aObjects.Count() );
	return m_aObjects[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team
//-----------------------------------------------------------------------------
CTFTeam *GetGlobalTFTeam( int iIndex )
{
	return assert_cast<CTFTeam *>( GetGlobalTeam( iIndex ) );
}

//-----------------------------------------------------------------------------
// Purpose: TF2C Domination points
//-----------------------------------------------------------------------------
void CTFTeam::AddRoundScore( int nPoints )
{
	int iOldRoundScore = m_iRoundScore;
	m_iRoundScore += nPoints;

	IGameEvent *event = gameeventmanager->CreateEvent( "dom_score" );
	if ( event )
	{
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "points", m_iRoundScore );
		event->SetInt( "amount", m_iRoundScore - iOldRoundScore );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: TF2C Domination points
//-----------------------------------------------------------------------------
void CTFTeam::SetRoundScore( int nPoints )
{
	int iOldRoundScore = m_iRoundScore;
	m_iRoundScore = nPoints;

	IGameEvent *event = gameeventmanager->CreateEvent( "dom_score" );
	if ( event )
	{
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "points", m_iRoundScore );
		event->SetInt( "amount", m_iRoundScore - iOldRoundScore );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: TF2C Domination points
//-----------------------------------------------------------------------------
void CTFTeam::ResetRoundScore()
{
	m_iRoundScore = 0;

	IGameEvent *event = gameeventmanager->CreateEvent( "dom_score" );
	if ( event )
	{
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "points", m_iRoundScore );
		event->SetInt( "amount", 0 );

		gameeventmanager->FireEvent( event );
	}
}