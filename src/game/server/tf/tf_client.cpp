/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== tf_client.cpp ========================================================

 TF client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tier0/vprof.h"
#include "tf_bot_temp.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool g_fGameOver;


void FinishClientPutInServer( CTFPlayer *pPlayer )
{
	bool bLocked = engine->LockNetworkStringTables( false );
	
	pPlayer->InitialSpawn();
	pPlayer->Spawn();
	
	engine->LockNetworkStringTables( bLocked );

	char sName[128];
	V_strcpy_safe( sName, pPlayer->GetPlayerName() );
	
	// First parse the name and remove any %'s
	for ( char *pCharacter = sName; pCharacter != NULL && *pCharacter != 0; pCharacter++ )
	{
		// Replace it with a space.
		if ( *pCharacter == '%' )
		{
			*pCharacter = ' ';
		}
	}

	// Notify other clients of player joining the game.
	if ( !pPlayer->IsBot() && !pPlayer->IsFakeClient() )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] ? sName : "<unconnected>" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a player is spawned into the game.
//-----------------------------------------------------------------------------
void ClientPutInServer( edict_t *pEdict, const char *pszPlayername )
{
	// Allocate a CTFPlayer for pev, and call spawn.
	CTFPlayer *pPlayer = CTFPlayer::CreatePlayer( "player", pEdict );
	if ( pPlayer )
	{
		pPlayer->SetPlayerName( pszPlayername );
	}
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in multiplayer!
	Assert( !bLoadGame );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEdict ) );
	if ( pPlayer )
	{
		FinishClientPutInServer( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the descriptive name of this .dll. E.g., Half-Life, or Team Fortress.
//-----------------------------------------------------------------------------
const char *GetGameDescription()
{
	// This function may be called before the world has spawned, before the GameRules was initialized.
	if ( g_pGameRules )
		return g_pGameRules->GetGameDescription();
	
	return "Team Fortress 2 Classic";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models and sounds.
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	const char *pFilename = "scripts/client_precache.txt";

	KeyValues *pValues = new KeyValues( "ClientPrecache" );
	if ( !pValues->LoadFromFile( filesystem, pFilename, "GAME" ) )
	{
		Error( "Can't open '%s' for client precache info!", pFilename );
		pValues->deleteThis();
		return;
	}

	for ( KeyValues *pData = pValues->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		if ( !V_stricmp( pData->GetName(), "model" ) )
		{
			CBaseEntity::PrecacheModel( pData->GetString() );
		}
		else if ( !V_stricmp( pData->GetName(), "scriptsound" ) )
		{
			CBaseEntity::PrecacheScriptSound( pData->GetString() );
		}
	}

	pValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Called by ClientKill and DeadThink.
//-----------------------------------------------------------------------------
void respawn( CBaseEntity *pEdict, bool bCopyCorpse )
{
	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		if ( bCopyCorpse )
		{
			// Make a copy of the dead body for appearances sake.
			static_cast<CBasePlayer *>( pEdict )->CreateCorpse();
		}

		// Respawn player.
		pEdict->Spawn();
	}
	else
	{
		// Restart the entire server.
		engine->ServerCommand( "reload\n" );
	}
}


void GameStartFrame( void )
{
	VPROF( "GameStartFrame" );

	if ( g_pGameRules )
	{
		g_pGameRules->Think();
	}

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.GetBool();

	Bot_RunAll();
}

//-----------------------------------------------------------------------------
// Purpose: Instantiate the proper game rules object
//-----------------------------------------------------------------------------
void InstallGameRules()
{
	CreateGameRulesObject( "CTFGameRules" );
}
