//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_server.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "eventqueue.h"
#include "characterset.h"
#include "sceneentity.h"		// for exposing scene precache function
#include "isaverestore.h"
#include "gamerules.h"
#include "netpropmanager.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "vscript_server_nut.h"
#include "ScriptGameEventListener.h"
#include "vscript_singletons.h"
#include "world.h"
#include "nav_mesh.h"

extern ScriptClassDesc_t *GetScriptDesc( CBaseEntity * );

ConVar vscript_script_hooks( "vscript_script_hooks", "1" );
extern ConVar sv_allow_point_servercommand;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptEntityIterator
{
public:
	HSCRIPT GetLocalPlayer()
	{
		return ToHScript( UTIL_GetLocalPlayerOrListenServerHost() );
	}

	HSCRIPT First() { return Next(NULL); }

	HSCRIPT Next( HSCRIPT hStartEntity )
	{
		return ToHScript( gEntList.NextEnt( ToEnt( hStartEntity ) ) );
	}

	HSCRIPT CreateByClassname( const char *className )
	{
		return ToHScript( CreateEntityByName( className ) );
	}

	void DispatchSpawn( HSCRIPT hEntity )
	{
		CBaseEntity *pEntity = ToEnt( hEntity );
		if (pEntity)
		{
			::DispatchSpawn( pEntity, false );
		}
	}

	HSCRIPT FindByClassname( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByClassname( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByName( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByName( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindInSphere( HSCRIPT hStartEntity, const Vector &vecCenter, float flRadius )
	{
		return ToHScript( gEntList.FindEntityInSphere( ToEnt( hStartEntity ), vecCenter, flRadius ) );
	}

	HSCRIPT FindByTarget( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByTarget( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByModel( HSCRIPT hStartEntity, const char *szModelName )
	{
		return ToHScript( gEntList.FindEntityByModel( ToEnt( hStartEntity ), szModelName ) );
	}

	HSCRIPT FindByNameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByNameWithin( HSCRIPT hStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameWithin( HSCRIPT hStartEntity , const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameWithinBox( HSCRIPT hStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecMins, vecMaxs ) );
	}

	HSCRIPT FindByClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, const char *classname )
	{
		return ToHScript( gEntList.FindEntityClassNearestFacing( origin, facing, threshold, const_cast<char *>( classname ) ) );
	}
} g_ScriptEntityIterator;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptEntityIterator, "CEntities", SCRIPT_SINGLETON "The global list of entities" )
	DEFINE_SCRIPTFUNC( GetLocalPlayer, "Get local player or listen server host" )
	DEFINE_SCRIPTFUNC( First, "Begin an iteration over the list of entities" )
	DEFINE_SCRIPTFUNC( Next, "Continue an iteration over the list of entities, providing reference to a previously found entity" )
	DEFINE_SCRIPTFUNC( CreateByClassname, "Creates an entity by classname" )
	DEFINE_SCRIPTFUNC( DispatchSpawn, "Spawns an unspawned entity" )
	DEFINE_SCRIPTFUNC( FindByClassname, "Find entities by class name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByName, "Find entities by name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindInSphere, "Find entities within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByTarget, "Find entities by targetname. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByModel, "Find entities by model name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByNameNearest, "Find entities by name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByNameWithin, "Find entities by name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassnameNearest, "Find entities by class name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByClassnameWithin, "Find entities by class name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassnameWithinBox, "Find entities by class name within an AABB. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search" )
	DEFINE_SCRIPTFUNC( FindByClassNearestFacing, "Find the nearest entity along the facing direction from the given origin within the angular threshold with the given classname." )
END_SCRIPTDESC();

class CScriptResponseCriteria
{
public:
	const char *GetValue( HSCRIPT hEntity, const char *pCriteria )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return "";

		AI_CriteriaSet criteria;
		pBaseEntity->ModifyOrAppendCriteria( criteria );

		int index = criteria.FindCriterionIndex( pCriteria );
		if ( !criteria.IsValidIndex( index ) )
			return "";

		return criteria.GetValue( index );
	}

	void GetTable( HSCRIPT hEntity, HSCRIPT hTable )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity || !hTable )
			return;

		AI_CriteriaSet criteria;
		pBaseEntity->ModifyOrAppendCriteria( criteria );

		for ( int nCriteriaIdx = 0; nCriteriaIdx < criteria.GetCount(); nCriteriaIdx++ )
		{
			if ( !criteria.IsValidIndex( nCriteriaIdx ) )
				continue;

			g_pScriptVM->SetValue( hTable, criteria.GetName( nCriteriaIdx ), criteria.GetValue( nCriteriaIdx ) );
		}
	}

	bool HasCriterion( HSCRIPT hEntity, const char *pCriteria )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return false;

		AI_CriteriaSet criteria;
		pBaseEntity->ModifyOrAppendCriteria( criteria );

		int index = criteria.FindCriterionIndex( pCriteria );
		return criteria.IsValidIndex( index );
	}
} g_ScriptResponseCriteria;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptResponseCriteria, "CScriptResponseCriteria", SCRIPT_SINGLETON "Used to get response criteria" )
	DEFINE_SCRIPTFUNC( GetValue, "Arguments: ( entity, criteriaName ) - returns a string" )
	DEFINE_SCRIPTFUNC( GetTable, "Arguments: ( entity ) - returns a table of all criteria" )
	DEFINE_SCRIPTFUNC( HasCriterion, "Arguments: ( entity, criteriaName ) - returns true if the criterion exists" )
END_SCRIPTDESC();

class CScriptEntityOutputs
{
public:
	int GetNumElements( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return -1;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return -1;

		return pOutput->NumberOfElements();
	}

	void GetOutputTable( HSCRIPT hEntity, const char *szOutputName, HSCRIPT hOutputTable, int element )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity || !hOutputTable || element < 0 )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( pOutput )
		{
			int iCount = 0;
			CEventAction *pAction = pOutput->GetFirstAction();
			while ( pAction )
			{
				if ( iCount == element )
				{
					g_pScriptVM->SetValue( hOutputTable, "target", STRING( pAction->m_iTarget ) );
					g_pScriptVM->SetValue( hOutputTable, "input", STRING( pAction->m_iTargetInput ) );
					g_pScriptVM->SetValue( hOutputTable, "parameter", STRING( pAction->m_iParameter ) );
					g_pScriptVM->SetValue( hOutputTable, "delay", pAction->m_flDelay );
					g_pScriptVM->SetValue( hOutputTable, "times_to_fire", pAction->m_nTimesToFire );
					break;
				}
				else
				{
					iCount++;
					pAction = pAction->m_pNext;
				}
			}
		}
	}

	void AddOutput( HSCRIPT hEntity, const char *pszOutputName, const char *pszTargetName, const char *pszInputName, const char *pszParameter, float flDelay, int nTimesToFire )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( pszOutputName );
		if ( !pOutput )
			return;

		CEventAction *pAction = new CEventAction();
		pAction->m_iTarget = AllocPooledString( pszTargetName );
		pAction->m_iTargetInput = AllocPooledString( pszInputName );
		pAction->m_iParameter = AllocPooledString( pszParameter );
		pAction->m_flDelay = flDelay;
		pAction->m_nTimesToFire = nTimesToFire;
		pOutput->AddEventAction( pAction );
	}

	void RemoveOutput( HSCRIPT hEntity, const char *pszOutputName, const char *pszTargetName, const char *pszInputName, const char *pszParameter )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( pszOutputName );
		if ( !pOutput )
			return;

		if ( pszTargetName && pszTargetName[0] )
		{
			pOutput->ScriptRemoveEventAction( pOutput->GetFirstAction(), pszTargetName, pszInputName, pszParameter );
		}
		else
		{
			pOutput->DeleteAllElements();
		}
	}

	bool HasOutput( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return false;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return false;

		return true;
	}

	bool HasAction( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt(hEntity);
		if ( !pBaseEntity )
			return false;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( pOutput )
		{
			CEventAction *pAction = pOutput->GetFirstAction();
			if ( pAction )
				return true;
		}

		return false;
	}
} g_ScriptEntityOutputs;

BEGIN_SCRIPTDESC_ROOT( CScriptEntityOutputs, SCRIPT_SINGLETON "Used to get entity output data" )
	DEFINE_SCRIPTFUNC( GetNumElements, "Arguments: ( entity, outputName ) - returns the number of array elements" )
	DEFINE_SCRIPTFUNC( GetOutputTable, "Arguments: ( entity, outputName, table, arrayElement ) - returns a table of output information" )
	DEFINE_SCRIPTFUNC( AddOutput, "Arguments: ( entity, outputName, targetName, inputName, parameter, delay, timesToFire ) - add a new output to the entity" )
	DEFINE_SCRIPTFUNC( RemoveOutput, "Arguments: ( entity, outputName, targetName, inputName, parameter ) - remove an output from the entity" )
	DEFINE_SCRIPTFUNC( HasOutput, "Arguments: ( entity, outputName ) - returns true if the output exists" )
	DEFINE_SCRIPTFUNC( HasAction, "Arguments: ( entity, outputName ) - returns true if an action exists for the output" )
END_SCRIPTDESC();

// When a scripter wants to change a netprop value, they can use the
// CNetPropManager class; it checks for errors and such on its own.
CNetPropManager g_NetPropManager;

BEGIN_SCRIPTDESC_ROOT( CNetPropManager, SCRIPT_SINGLETON "Used to get/set entity network fields" )
	DEFINE_SCRIPTFUNC( GetPropInt, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropFloat, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropVector, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropEntity, "Arguments: ( entity, propertyName ) - returns an entity" )
	DEFINE_SCRIPTFUNC( GetPropString, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropBool, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( SetPropInt, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropFloat, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropVector, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropEntity, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropString, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropBool, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( GetPropIntArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropFloatArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropVectorArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropEntityArray, "Arguments: ( entity, propertyName, arrayElement ) - returns an entity" )
	DEFINE_SCRIPTFUNC( GetPropStringArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropBoolArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropIntArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropFloatArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropVectorArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropEntityArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropStringArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropBoolArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropArraySize, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( HasProp, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropType, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropInfo, "Arguments: ( entity, propertyName, arrayElement, table ) - Fills in a passed table with property info for the provided entity" )
	DEFINE_SCRIPTFUNC( GetTable, "Arguments: ( entity, iPropType, table ) - Fills in a passed table with all props of a specified type for the provided entity (set iPropType to 0 for SendTable or 1 for DataMap)" )
END_SCRIPTDESC()


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float Time()
{
	return gpGlobals->curtime;
}

static float FrameTime()
{
	return gpGlobals->frametime;
}

static int MaxClients()
{
	return gpGlobals->maxClients;
}

static int GetLoadType()
{
	return gpGlobals->eLoadType;
}

static void SendToConsole( const char *pszCommand )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
	{
		DevMsg ("Cannot execute \"%s\", no player\n", pszCommand );
		return;
	}

	engine->ClientCommand( pPlayer->edict(), pszCommand );
}

static void SendToServerConsole( const char *pszCommand )
{
	if ( !sv_allow_point_servercommand.GetBool() )
		return;

	char szCommand[512];
	Q_snprintf( szCommand, sizeof(szCommand), "%s\n", pszCommand );
	engine->ServerCommand( szCommand );
}

HSCRIPT Script_GetLocalPlayerOrListenServerHost()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	return ToHScript( pPlayer );
}

static void SetFakeClientConVarValue( HSCRIPT hClient, const char *pszName, const char *pszValue )
{
	CBasePlayer *pFakeClient = HScriptToClass<CBasePlayer>( hClient );
	Assert( pFakeClient != NULL );

	engine->SetFakeClientConVarValue( pFakeClient->edict(), pszName, pszValue );
}

static const char *GetMapName()
{
	return STRING( gpGlobals->mapname );
}

static const char *DoUniqueString( const char *pszBase )
{
	static char szBuf[512];
	g_pScriptVM->GenerateUniqueKey( pszBase, szBuf, ARRAYSIZE(szBuf) );
	return szBuf;
}

bool Script_IsModelPrecached( const char *modelname )
{
	return engine->IsModelPrecached( VScriptCutDownString( modelname ) );
}

int Script_GetModelIndex( const char *modelname )
{
	return modelinfo->GetModelIndex( modelname );
}

static void DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
{
	const char *target = "", *action = "Use";
	variant_t value;

	target = STRING( AllocPooledString( pszTarget ) );

	// Don't allow them to run anything on a point_servercommand unless they're the host player. Otherwise they can ent_fire
	// and run any command on the server. Admittedly, they can only do the ent_fire if sv_cheats is on, but 
	// people complained about users resetting the rcon password if the server briefly turned on cheats like this:
	//    give point_servercommand
	//    ent_fire point_servercommand command "rcon_password mynewpassword"
	if ( gpGlobals->maxClients > 1 && V_stricmp( target, "point_servercommand" ) == 0 )
	{
		return;
	}

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	g_EventQueue.AddEvent( target, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}


bool DoIncludeScript( const char *pszScript, HSCRIPT hScope )
{
	if ( !VScriptRunScript( pszScript, hScope, true ) )
	{
		g_pScriptVM->RaiseException( CFmtStr( "Failed to include script \"%s\"", ( pszScript ) ? pszScript : "unknown" ) );
		return false;
	}
	return true;
}

void RegisterScriptGameEventListener( char const *pszEvent )
{
	if ( pszEvent && pszEvent[0] )
	{
		ScriptGameEventListener()->ListenForGameEvent( pszEvent );
	}
	else
	{
		Msg( "No event name specified!\n" );
		Log( "No event name specified\n" );
	}
}

void RegisterScriptHookListener( char const *pszHook )
{
	if ( pszHook && pszHook[0] )
	{
		ScriptGameEventListener()->ListenForScriptHook( pszHook );
	}
	else
	{
		Msg( "No event name specified!\n" );
		Log( "No event name specified\n" );
	}
}

void ClearScriptGameEventListeners()
{
	ScriptGameEventListener()->ClearAllScriptHooks();
	ScriptGameEventListener()->StopListeningForAllEvents();
}

void ScriptFireGameEvent( char const *pszEvent, HSCRIPT params )
{
	ScriptGameEventListener()->RunGameEventCallbacks( pszEvent, params );
}

void ScriptFireScriptHook( char const *pszHook, HSCRIPT params )
{
	ScriptGameEventListener()->FireScriptHook( pszHook, params );
}

bool ScriptSendGlobalGameEvent( char const *pszEvent, HSCRIPT params )
{
	if ( !pszEvent || !pszEvent[0] )
		return false;

	IGameEvent *event = gameeventmanager->CreateEvent( pszEvent );
	if ( event )
	{
		int nEntries = g_pScriptVM->GetNumTableEntries( params );
		if ( nEntries > 0 )
		{
			int nNext = 0;
			for ( int i = 0; i < nEntries; ++i )
			{
				ScriptVariant_t key, val;
				nNext = g_pScriptVM->GetKeyValue( params, nNext, &key, &val );
				if ( key.m_type != FIELD_CSTRING )
				{
					Warning( "VSCRIPT: ScriptSendRealGameEvent: Key must be a FIELD_CSTRING" );
					Log( "VSCRIPT: ScriptSendRealGameEvent: Key must be a FIELD_CSTRING" );
					break;
				}

				switch ( val.m_type )
				{
					case FIELD_INTEGER:
						event->SetInt( key, val );
						break;
					case FIELD_FLOAT:
						event->SetFloat( key, val );
						break;
					case FIELD_BOOLEAN:
						event->SetBool( key, val );
						break;
					case FIELD_CSTRING:
						event->SetString( key, val );
						break;
					default:
						Warning( "VSCRIPT: ScriptSendRealGameEvent: Don't understand FIELD_TYPE of value for key %s.", key.m_pszString );
						Log( "VSCRIPT: ScriptSendRealGameEvent: Don't understand FIELD_TYPE of value for key %s.", key.m_pszString );
						return false;
				}
			}
		}

		return gameeventmanager->FireEvent( event );
	}

	return false;
}

bool ScriptHooksEnabled()
{
	return g_pScriptVM != NULL && vscript_script_hooks.GetBool();
}

bool ScriptHookEnabled( const char *pszName )
{
	if ( ScriptHooksEnabled() )
	{
		if ( pszName && pszName[0] )
			return ScriptGameEventListener()->HasScriptHook( pszName );

		Msg( "No event name specified\n" );
		Log( "No event name specified\n" );
	}

	return false;
}

bool RunScriptHook( const char *pszName, HSCRIPT params )
{
	if ( pszName && pszName[0] )
		return ScriptGameEventListener()->FireScriptHook( pszName, params );

	Msg( "No event name specified\n" );
	Log( "No event name specified\n" );
	return false;
}

HSCRIPT CreateProp( const char *pszEntityName, const Vector &vOrigin, const char *pszModelName, int iAnim )
{
	CBaseAnimating *pBaseEntity = (CBaseAnimating *)CreateEntityByName( pszEntityName );
	pBaseEntity->SetAbsOrigin( vOrigin );
	pBaseEntity->SetModel( pszModelName );
	pBaseEntity->SetPlaybackRate( 1.0f );

	int iSequence = pBaseEntity->SelectWeightedSequence( (Activity)iAnim );

	if ( iSequence != -1 )
	{
		pBaseEntity->SetSequence( iSequence );
	}

	return ToHScript( pBaseEntity );
}

//--------------------------------------------------------------------------------------------------
// Use an entity's script instance to add an entity IO event (used for firing events on unnamed entities from vscript)
//--------------------------------------------------------------------------------------------------
static void DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
{
	const char *action = "Use";
	variant_t value;

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	CBaseEntity* pTarget = ToEnt(hTarget);

	if ( !pTarget )
	{
		Warning( "VScript error: DoEntFire was passed an invalid entity instance.\n" );
		return;
	}

	g_EventQueue.AddEvent( pTarget, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}

static float ScriptTraceLine( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	trace_t tr;
	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, MASK_NPCWORLDSTATIC, pLooker, COLLISION_GROUP_NONE, &tr );

	if ( tr.fractionleftsolid != 0.0 && tr.startsolid )
		return ( 1.0 - tr.fractionleftsolid );
	return tr.fraction;
}

static float ScriptTraceLinePlayersIncluded( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	trace_t tr;
	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pLooker, COLLISION_GROUP_NONE, &tr );

	if ( tr.fractionleftsolid != 0.0 && tr.startsolid )
		return ( 1.0 - tr.fractionleftsolid );
	return tr.fraction;
}

HSCRIPT Script_EntIndexToHScript( int entIndex )
{
	CBaseEntity *pEntity = UTIL_EntityByIndex( entIndex );
	return ToHScript( pEntity );
}

HSCRIPT Script_PlayerInstanceFromIndex( int playerIndex )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );
	return ToHScript( pPlayer );
}

HSCRIPT Script_GetPlayerFromUserID( int userID )
{
	CBasePlayer *pPlayer = UTIL_PlayerByUserId( userID );
	return ToHScript( pPlayer );
}

bool Script_IsPlayerABot( HSCRIPT hPlayer )
{
	if ( hPlayer )
	{
		CBaseEntity *pEntity = ToEnt( hPlayer );
		if ( pEntity && pEntity->IsPlayer() )
		{
			return assert_cast<CBasePlayer *>( pEntity )->IsBot();
		}
	}

	return false;
}

void Script_SetFakeClientConVarValue( HSCRIPT hPlayer, const char *pszConVar, const char *pszValue )
{
	CBasePlayer *pPlayer = HScriptToClass<CBasePlayer>( hPlayer );
	if ( pPlayer && pPlayer->IsFakeClient() )
	{
		if ( pszConVar && pszConVar[0] && pszValue && pszValue[0] )
			engine->SetFakeClientConVarValue( pPlayer->edict(), pszConVar, pszValue );
	}
}

static void Script_Say( HSCRIPT hPlayer, const char *pText )
{
	CBaseEntity *pBaseEntity = ToEnt(hPlayer);
	CBasePlayer *pPlayer = NULL;
	char szText[256];

	if ( pBaseEntity )
		pPlayer = dynamic_cast<CBasePlayer*>(pBaseEntity);

	if ( pPlayer )
	{
		Q_snprintf( szText, sizeof(szText), "say %s", pText );
		engine->ClientCommand( pPlayer->edict(), szText );
	}
	else
	{
		Q_snprintf( szText, sizeof(szText), "Console: %s", pText );
		UTIL_SayTextAll( szText, pPlayer );
	}
}

static void Script_ClientPrint( HSCRIPT hPlayer, int iDest, const char *pText )
{
	CBaseEntity *pBaseEntity = ToEnt(hPlayer);
	CBasePlayer *pPlayer = NULL;

	if ( pBaseEntity )
		pPlayer = dynamic_cast<CBasePlayer*>(pBaseEntity);

	if ( pPlayer )
		ClientPrint( pPlayer, iDest, pText );
	else
		UTIL_ClientPrintAll( iDest, pText );
}

static void Script_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, int eCommand, bool bAirShake )
{
	UTIL_ScreenShake( center, amplitude, frequency, duration, radius, (ShakeCommand_t)eCommand, bAirShake );
}

static void Script_ScreenFade( HSCRIPT hEntity, int r, int g, int b, int a, float fadeTime, float fadeHold, int flags )
{
	CBaseEntity *pEntity = ToEnt(hEntity);
	color32 color = { r & 0xFF, g & 0xFF, b & 0xFF, a & 0xFF };

	if ( pEntity )
		UTIL_ScreenFade( pEntity, color, fadeTime, fadeHold, flags );
	else
		UTIL_ScreenFadeAll( color, fadeTime, fadeHold, flags );
}

static void Script_FadeClientVolume( HSCRIPT hPlayer, float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds )
{
	CBaseEntity *pBaseEntity = ToEnt(hPlayer);
	CBasePlayer *pPlayer = NULL;

	if ( pBaseEntity )
		pPlayer = dynamic_cast<CBasePlayer*>(pBaseEntity);

	if ( pPlayer )
		engine->FadeClientVolume( pPlayer->edict(), fadePercent, fadeOutSeconds, holdTime, fadeInSeconds );
}

bool VScriptServerInit()
{
	VMPROF_START

	if( scriptmanager != NULL )
	{
		ScriptLanguage_t scriptLanguage = SL_DEFAULT;

		char const *pszScriptLanguage;
		if ( CommandLine()->CheckParm( "-scriptlang", &pszScriptLanguage ) )
		{
			if( !Q_stricmp(pszScriptLanguage, "squirrel") )
			{
				scriptLanguage = SL_SQUIRREL;
			}
			else if( !Q_stricmp(pszScriptLanguage, "lua") )
			{
				scriptLanguage = SL_LUA;
			}
			else if ( !Q_stricmp( pszScriptLanguage, "angelscript" ) )
			{
				scriptLanguage = SL_ANGELSCRIPT;
			}
			else
			{
				DevWarning("-scriptlang does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}
		}

		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if( g_pScriptVM )
			{
				Log( "VSCRIPT SERVER: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );

				ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_ShowMessageAll, "ShowMessage", "Print a hud message on all clients" );
				ScriptRegisterFunction( g_pScriptVM, SendToConsole, "Send a string to the console as a command" );
				ScriptRegisterFunction( g_pScriptVM, SendToServerConsole, "Send a string that gets executed on the server as a ServerCommand. Respects sv_allow_point_servercommand." );
				ScriptRegisterFunctionNamed( g_pScriptVM, SendToServerConsole, "SendToConsoleServer", "Copy of SendToServerConsole with another name for compat." );
				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLine, "TraceLine", "given 2 points & ent to ignore, return fraction along line that hits world or models" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLinePlayersIncluded, "TraceLinePlayersIncluded", "given 2 points & ent to ignore, return fraction along line that hits world, models, players or npcs" );
				ScriptRegisterFunction( g_pScriptVM, MaxClients, "Get the maximum number of players allowed on this server" );
				ScriptRegisterFunction( g_pScriptVM, GetLoadType, "Get the way the current game was loaded (corresponds to the MapLoad enum)" );
				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, FrameTime, "Get the time spent on the server in the last frame" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetLocalPlayerOrListenServerHost, "GetListenServerHost", "Get the host player on a listen server.");
				ScriptRegisterFunction( g_pScriptVM, SetFakeClientConVarValue, "Sets a USERINFO client ConVar for a fakeclient" );
				ScriptRegisterFunction( g_pScriptVM, DoEntFire, SCRIPT_ALIAS( "EntFire", "Generate and entity i/o event" ) );
				ScriptRegisterFunction( g_pScriptVM, DoEntFireByInstanceHandle, SCRIPT_ALIAS( "EntFireByHandle", "Generate and entity i/o event. First parameter is an entity instance." ) );
				ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string. Useful for adding data to tables when not sure what keys are already in use in that table." ) );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCreateSceneEntity, "CreateSceneEntity", "Create a scene entity to play the specified scene." );
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, SCRIPT_ALIAS( "IncludeScript", "Execute a script (internal)" ) );
				ScriptRegisterFunction( g_pScriptVM, RegisterScriptGameEventListener, "Register as a listener for a game event from script." );
				ScriptRegisterFunction( g_pScriptVM, RegisterScriptHookListener, "Register as a listener for a script hook from script." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFireGameEvent, "FireGameEvent", "Fire a game event to a listening callback function in script. Parameters are passed in a squirrel table." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFireScriptHook, "FireScriptHook", "Fire a script hoook to a listening callback function in script. Parameters are passed in a squirrel table." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptSendGlobalGameEvent, "SendGlobalGameEvent", "Sends a real game event to everything. Parameters are passed in a squirrel table." );
				ScriptRegisterFunction( g_pScriptVM, ScriptHooksEnabled, "Returns whether script hooks are currently enabled." );
				ScriptRegisterFunction( g_pScriptVM, CreateProp, "Create a physics prop" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_Say, "Say", "Have player say string" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ClientPrint, "ClientPrint", "Print a client message" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ScreenShake, "ScreenShake", "Start a screenshake with the following parameters. vecCenter, flAmplitude, flFrequency, flDuration, flRadius, eCommand( SHAKE_START = 0, SHAKE_STOP = 1 ), bAirShake" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ScreenFade, "ScreenFade", "Start a screenfade with the following parameters. player, red, green, blue, alpha, flFadeTime, flFadeHold, flags" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsModelPrecached, "IsModelPrecached", "Checks if the modelname is precached." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetModelIndex, "GetModelIndex", "Returns the index of the named model." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_FadeClientVolume, "FadeClientVolume", "Fade out the client's volume level toward silence (or fadePercent)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_EntIndexToHScript, "EntIndexToHScript", "turn an entity index integer to an HScript representing that entity's script instance" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PlayerInstanceFromIndex, "PlayerInstanceFromIndex", "Get a script handle of a player using the player index." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetPlayerFromUserID, "GetPlayerFromUserID", "Given a user id, return the entity, or null." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsPlayerABot, "IsPlayerABot", "Is this player/entity a bot" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_SetFakeClientConVarValue, "SetFakeClientConVarValue", "Sets a USERINFO client ConVar for a fakeclient" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::ScreenTextLine, "DebugDrawScreenTextLine", "Draw text with a line offset" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Text, "DebugDrawText", "Draw text in 3d (origin, text, bViewCheck, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::BoxAngles, "DebugDrawBoxAngles", "Draw a debug overlay box" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Line, "DebugDrawLine", "Draw a debug overlay line" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Triangle, "DebugDrawTriangle", "Draw a debug overlay triangle" );
				
				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

				g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );
				g_pScriptVM->RegisterInstance( &g_NetPropManager, "NetProps" );
				g_pScriptVM->RegisterInstance( &g_ScriptResponseCriteria, "ResponseCriteria" );
				g_pScriptVM->RegisterInstance( &g_ScriptEntityOutputs, "EntityOutputs" );
				g_pScriptVM->RegisterInstance( TheNavMesh, "NavMesh" );

				RegisterSharedScriptConstants();
				RegisterSharedScriptFunctions();

				if ( scriptLanguage == SL_SQUIRREL )
				{
					g_pScriptVM->Run( g_Script_vscript_server );
				}

				VScriptRunScript( "mapspawn", false );

				// Since the world entity spawns before VScript is initted, RunVScripts() is called before the VM has started, so no scripts are run.
				// This gets around that by calling the same function right after the VM is initted.
				GetWorldEntity()->RunVScripts();

				VMPROF_SHOW( __FUNCTION__, "virtual machine startup" );

				return true;
			}
			else
			{
				DevWarning("VM Did not start!\n");
			}
		}
		else
		{
			Msg(  "VSCRIPT SERVER: Not starting because language is set to 'none'\n" );
		}
	}
	else
	{
		Msg( "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptServerTerm()
{
	if( scriptmanager != NULL )
	{
		if( g_pScriptVM )
		{
			scriptmanager->DestroyVM( g_pScriptVM );
			g_pScriptVM = NULL;
		}
	}
}


bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	HSCRIPT hReplaceClosuresFunc = g_pScriptVM->LookupFunction( "__ReplaceClosures" );
	if ( !hReplaceClosuresFunc )
	{
		return false;
	}
	HSCRIPT hNewScript =  VScriptCompileScript( pszScriptName, bWarnMissing );
	if ( !hNewScript )
	{
		return false;
	}

	g_pScriptVM->Call( hReplaceClosuresFunc, NULL, true, NULL, hNewScript, hScope );
	return true;
}

CON_COMMAND( script_reload_code, "Execute a vscript file, replacing existing functions with the functions in the run script" )
{
	if ( !*args[1] )
	{
		Warning( "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	VScriptServerReplaceClosures( args[1], NULL, true );
}

CON_COMMAND( script_reload_entity_code, "Execute all of this entity's VScripts, replacing existing functions with the functions in the run scripts" )
{
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszVScripts != NULL_STRING )
		{
			char szScriptsList[255];
			Q_strcpy( szScriptsList, STRING(pEntity->m_iszVScripts) );
			CUtlStringList szScripts;
			V_SplitString( szScriptsList, " ", szScripts);

			for( int i = 0 ; i < szScripts.Count() ; i++ )
			{
				VScriptServerReplaceClosures( szScripts[i], pEntity->m_ScriptScope, true );
			}
		}
	}
}

CON_COMMAND( script_reload_think, "Execute an activation script, replacing existing functions with the functions in the run script" )
{
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszScriptThinkFunction != NULL_STRING )
		{
			VScriptServerReplaceClosures( STRING(pEntity->m_iszScriptThinkFunction), pEntity->m_ScriptScope, true );
		}
	}
}

class CVScriptGameSystem : public CAutoGameSystemPerFrame
{
public:
	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void )
	{
		ClearScriptGameEventListeners();
		m_bAllowEntityCreationInScripts = true;
		VScriptServerInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
	}

	virtual void LevelShutdownPostEntity( void )
	{
		ClearScriptGameEventListeners();

		VScriptServerTerm();
	}

	virtual void FrameUpdatePostEntityThink() 
	{ 
		if ( g_pScriptVM )
			g_pScriptVM->Frame( gpGlobals->frametime );
	}

	bool m_bAllowEntityCreationInScripts;
};

CVScriptGameSystem g_VScriptGameSystem;

ConVar script_allow_entity_creation_midgame( "script_allow_entity_creation_midgame", "1", FCVAR_NOT_CONNECTED, "Allows VScript files to create entities mid-game, as opposed to only creating entities on startup." );
bool IsEntityCreationAllowedInScripts( void )
{
	if ( script_allow_entity_creation_midgame.GetBool() )
		return true;

	return g_VScriptGameSystem.m_bAllowEntityCreationInScripts;
}

static short VSCRIPT_SERVER_SAVE_RESTORE_VERSION = 2;


//-----------------------------------------------------------------------------

class CVScriptSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	CVScriptSaveRestoreBlockHandler() :
		m_InstanceMap( DefLessFunc(const char *) )
	{
	}
	const char *GetBlockName()
	{
		return "VScriptServer";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->StartBlock();

		int temp = g_pScriptVM != NULL;
		pSave->WriteInt( &temp );
		if ( g_pScriptVM )
		{
			temp = g_pScriptVM->GetLanguage();
			pSave->WriteInt( &temp );
			CUtlBuffer buffer;
			g_pScriptVM->WriteState( &buffer );
			temp = buffer.TellPut();
			pSave->WriteInt( &temp );
			if ( temp > 0 )
			{
				pSave->WriteData( (const char *)buffer.Base(), temp );
			}
		}

		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( !m_fDoLoad && g_pScriptVM )
		{
			return;
		}
		CBaseEntity *pEnt = gEntList.FirstEnt();
		while ( pEnt )
		{
			if ( pEnt->m_iszScriptId != NULL_STRING )
			{
				g_pScriptVM->RegisterClass( pEnt->GetScriptDesc() );
				m_InstanceMap.Insert( STRING( pEnt->m_iszScriptId ), pEnt );
			}
			pEnt = gEntList.NextEnt( pEnt );
		}

		pRestore->StartBlock();
		if ( pRestore->ReadInt() && pRestore->ReadInt() == g_pScriptVM->GetLanguage() )
		{
			int nBytes = pRestore->ReadInt();
			if ( nBytes > 0 )
			{
				CUtlBuffer buffer;
				buffer.EnsureCapacity( nBytes );
				pRestore->ReadData( (char *)buffer.AccessForDirectRead( nBytes ), nBytes, 0 );
				g_pScriptVM->ReadState( &buffer );
			}
		}
		pRestore->EndBlock();
	}

	void PostRestore( void )
	{
		for ( int i = m_InstanceMap.FirstInorder(); i != m_InstanceMap.InvalidIndex(); i = m_InstanceMap.NextInorder( i ) )
		{
			CBaseEntity *pEnt = m_InstanceMap[i];
			if ( pEnt->m_hScriptInstance )
			{
				ScriptVariant_t variant;
				if ( g_pScriptVM->GetValue( STRING(pEnt->m_iszScriptId), &variant ) && variant.m_type == FIELD_HSCRIPT )
				{
					pEnt->m_ScriptScope.Init( variant.m_hScript, false );
					pEnt->RunPrecacheScripts();
				}
			}
			else
			{
				// Script system probably has no internal references
				pEnt->m_iszScriptId = NULL_STRING;
			}
		}
		m_InstanceMap.Purge();
	}


	CUtlMap<const char *, CBaseEntity *> m_InstanceMap;

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CVScriptSaveRestoreBlockHandler g_VScriptSaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler()
{
	return &g_VScriptSaveRestoreBlockHandler;
}

//-----------------------------------------------------------------------------

bool CBaseEntityScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CBaseEntity *pEntity = (CBaseEntity *)p;
	if ( pEntity->GetEntityName() != NULL_STRING )
	{
		V_snprintf( pBuf, bufSize, "([%d] %s: %s)", pEntity->entindex(), STRING(pEntity->m_iClassname), STRING( pEntity->GetEntityName() ) );
	}
	else
	{
		V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), STRING(pEntity->m_iClassname) );
	}
	return true; 
}

void *CBaseEntityScriptInstanceHelper::BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
{
	int iEntity = g_VScriptSaveRestoreBlockHandler.m_InstanceMap.Find( pszId );
	if ( iEntity != g_VScriptSaveRestoreBlockHandler.m_InstanceMap.InvalidIndex() )
	{
		CBaseEntity *pEnt = g_VScriptSaveRestoreBlockHandler.m_InstanceMap[iEntity];
		pEnt->m_hScriptInstance = hInstance;
		return pEnt;
	}
	return NULL;
}


CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;


