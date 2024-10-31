//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Intercepts game events for VScript and call OnGameEvent_<eventname>.
//
//==========================================================================================//
#include "cbase.h"
#include "filesystem.h"
#include "ScriptGameEventListener.h"
#include "vscript_server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *const g_sGameEventTypeMap[] =
{
	"local",	// 0 : don't network this field
	"string",	// 1 : zero terminated ASCII string
	"float",	// 2 : float 32 bit
	"long",		// 3 : signed int 32 bit
	"short",	// 4 : signed int 16 bit
	"byte",		// 5 : unsigned int 8 bit
	"bool",		// 6 : unsigned int 1 bit
};

static ConVar sv_debug_script_events( "sv_debug_script_events", "0", FCVAR_CHEAT );


CScriptGameEventListener::~CScriptGameEventListener()
{
	StopListeningForAllEvents();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptGameEventListener::FireGameEvent( IGameEvent *event )
{
	if ( !g_pScriptVM )
		return;

	char szGameEventFunctionName[256];
	Q_snprintf( szGameEventFunctionName, sizeof(szGameEventFunctionName), "OnGameEvent_%s", event->GetName() );

	if ( sv_debug_script_events.GetBool() )
		DevMsg( "\t\"%s\"\n", szGameEventFunctionName );

	ScriptVariant_t hGameEventTable;
	g_pScriptVM->CreateTable(hGameEventTable);
	SetVScriptEventValues(event, hGameEventTable);
	RunGameEventCallbacks(event->GetName(), hGameEventTable );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptGameEventListener::SetVScriptEventValues( IGameEvent *event, HSCRIPT table )
{
	int index = m_GameEvents.Find( event->GetName() );
	Assert( index != m_GameEvents.InvalidIndex() );

	if ( m_GameEvents.IsValidIndex( index ) )
	{
		GameEvents_t &gameEvent = m_GameEvents[ index ];
		FOR_EACH_MAP_FAST( gameEvent.m_EventParams, i )
		{
			const char *keyName = gameEvent.m_EventParams.Key( i );
			switch ( gameEvent.m_EventParams.Element( i ) )
			{
				case KeyValues::TYPE_INT:
					if ( sv_debug_script_events.GetBool() )
						DevMsg( "\t\t\"%s\"\t\"%i\"\n", keyName, event->GetInt( keyName ) );
					g_pScriptVM->SetValue( table, keyName, event->GetInt( keyName ) );
					break;
				case KeyValues::TYPE_FLOAT:
					if ( sv_debug_script_events.GetBool() )
						DevMsg( "\t\t\"%s\"\t\"%.4f\"\n", keyName, event->GetFloat( keyName ) );
					g_pScriptVM->SetValue( table, keyName, event->GetFloat( keyName ) );
					break;
				case KeyValues::TYPE_STRING:
					if ( sv_debug_script_events.GetBool() )
						DevMsg( "\t\t\"%s\"\t\"%s\"\n", keyName, event->GetString( keyName ) );
					g_pScriptVM->SetValue( table, keyName, event->GetString( keyName ) );
					break;
			}
		}
	}
}

void CScriptGameEventListener::CollectGameEventCallbacksInScope( HSCRIPT scope )
{
	if ( m_hCollectGameEventCallbacks == INVALID_HSCRIPT )
		m_hCollectGameEventCallbacks = g_pScriptVM->LookupFunction( "__CollectGameEventCallbacks" );

	if ( m_hCollectGameEventCallbacks )
	{
		g_pScriptVM->Call( m_hCollectGameEventCallbacks, NULL, true, NULL, scope );
	}
}

void CScriptGameEventListener::RunGameEventCallbacks( char const *pszEvent, HSCRIPT params )
{
	if ( !pszEvent || !pszEvent[0] )
		return;

	if ( m_hRunGameEventCallbacks == INVALID_HSCRIPT )
		m_hRunGameEventCallbacks = g_pScriptVM->LookupFunction( "__RunGameEventCallbacks" );

	if ( m_hRunGameEventCallbacks )
	{
		g_pScriptVM->Call( m_hRunGameEventCallbacks, NULL, true, NULL, pszEvent, params );
	}
}

void CScriptGameEventListener::ListenForScriptHook( char const *pszHook )
{
	m_symScriptHooks.AddString( pszHook );
}

bool CScriptGameEventListener::HasScriptHook( char const *pszHook )
{
	return m_symScriptHooks.Find( pszHook ).IsValid();
}

bool CScriptGameEventListener::FireScriptHook( char const *pszHook, HSCRIPT params )
{
	if ( !pszHook || !pszHook[0] )
		return false;

	if ( !m_symScriptHooks.Find( pszHook ).IsValid() )
		return false;

	return RunScriptHookCallbacks( pszHook, params );
}

bool CScriptGameEventListener::RunScriptHookCallbacks( char const *pszHook, HSCRIPT params )
{
	if ( !pszHook || !pszHook[0] )
		return false;

	if ( m_hRunScriptHookCallbacks == INVALID_HSCRIPT )
		m_hRunScriptHookCallbacks = g_pScriptVM->LookupFunction( "__RunScriptHookCallbacks" );

	if ( m_hRunScriptHookCallbacks )
	{
		ScriptStatus_t status = g_pScriptVM->Call( m_hRunScriptHookCallbacks, NULL, true, NULL, pszHook, params );
		return status == SCRIPT_DONE;
	}

	return false;
}

void CScriptGameEventListener::ClearAllScriptHooks()
{
	m_symScriptHooks.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CScriptGameEventListener::Init()
{
	auto RegisterEvent =[ & ]( KeyValues *pEvent ) {
		if ( pEvent == NULL )
			return false;

		GameEvents_t event;
		event.m_szEventName = pEvent->GetName();

		FOR_EACH_VALUE( pEvent, pSubKey )
		{
			const char *keyName = pSubKey->GetName();
			const char *type = pSubKey->GetString();

			auto const GetType =[ type ] () {
				for ( int i=0; i < ARRAYSIZE( g_sGameEventTypeMap ); ++i )
				{
					if ( Q_stricmp( type, g_sGameEventTypeMap[ i ] ) == 0 )
						return i;
				}

				return 0;
			};
			switch ( GetType() )
			{
				case TYPE_BOOL:
				case TYPE_LONG:
				case TYPE_SHORT:
				case TYPE_BYTE:
					event.m_EventParams.Insert( keyName, KeyValues::TYPE_INT );
					break;
				case TYPE_STRING:
					event.m_EventParams.Insert( keyName, KeyValues::TYPE_STRING );
					break;
				case TYPE_FLOAT:
					event.m_EventParams.Insert( keyName, KeyValues::TYPE_FLOAT );
					break;
				default:
					DevMsg( "Received unsupported event type '%s' while seting up event listeners\n", type );
					break;
			}
		}
		
		return m_GameEvents.Insert( pEvent->GetName(), event ) >= 0;
	};

	// Generic events
	{
		const char *filename = "resource/gameevents.res";
		KeyValues *key = new KeyValues("gameevents");
		KeyValuesAD autodelete( key );

		if  ( !key->LoadFromFile( filesystem, filename, "GAME" ) )
		{
			Error( "%s not found.\n", filename );
			return false;
		}

		FOR_EACH_TRUE_SUBKEY( key, pSubKey )
		{
			RegisterEvent( pSubKey );
		}

		if ( sv_debug_script_events.GetBool() )
			DevMsg( "Event System loaded %i events from file %s.\n", m_GameEvents.Count(), filename );
	}

	// Server events
	{
		const char *filename = "resource/serverevents.res";
		KeyValues *key = new KeyValues("serverevents");
		KeyValuesAD autodelete( key );

		if  ( !key->LoadFromFile( filesystem, filename, "GAME" ) )
		{
			Error( "%s not found.\n", filename );
			return false;
		}

		FOR_EACH_TRUE_SUBKEY( key, pSubKey )
		{
			RegisterEvent( pSubKey );
		}

		if ( sv_debug_script_events.GetBool() )
			DevMsg( "Event System loaded %i events from file %s.\n", m_GameEvents.Count(), filename );
	}

	// HLTV events
	{
		const char *filename = "resource/hltvevents.res";
		KeyValues *key = new KeyValues("hltvevents");
		KeyValuesAD autodelete( key );

		if  ( !key->LoadFromFile( filesystem, filename, "GAME" ) )
		{
			Error( "%s not found.\n", filename );
			return false;
		}

		FOR_EACH_TRUE_SUBKEY( key, pSubKey )
		{
			RegisterEvent( pSubKey );
		}

		if ( sv_debug_script_events.GetBool() )
			DevMsg( "Event System loaded %i events from file %s.\n", m_GameEvents.Count(), filename );
	}

	// Mod specific events
	{
		const char *filename = "resource/ModEvents.res";
		KeyValues *key = new KeyValues("ModEvents");
		KeyValuesAD autodelete( key );

		if  ( !key->LoadFromFile( filesystem, filename, "GAME" ) )
		{
			Error( "%s not found.\n", filename );
			return false;
		}

		FOR_EACH_TRUE_SUBKEY( key, pSubKey )
		{
			RegisterEvent( pSubKey );
		}

		if ( sv_debug_script_events.GetBool() )
			DevMsg( "Event System loaded %i events from file %s.\n", m_GameEvents.Count(), filename );
	}

	FOR_EACH_DICT_FAST( m_GameEvents, i )
	{
		const char *eventName = m_GameEvents[i].m_szEventName;
		if ( eventName && eventName[0] )
			ListenForGameEvent( eventName );
	}

	m_hCollectGameEventCallbacks = INVALID_HSCRIPT;
	m_hRunGameEventCallbacks = INVALID_HSCRIPT;
	m_hRunScriptHookCallbacks = INVALID_HSCRIPT;

	return true;
}

static CScriptGameEventListener s_ScriptGameEventListener;
CScriptGameEventListener *ScriptGameEventListener() {
	return &s_ScriptGameEventListener;
}
