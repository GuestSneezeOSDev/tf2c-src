//====== Copyright  1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "entity_forcerespawn.h"
#include "tf_player.h"
#include "tf_ammo_pack.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTFReset tables.
//
BEGIN_DATADESC( CTFForceRespawn )
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawn", InputForceRespawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawnSwitchTeams", InputForceRespawnSwitchTeams ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ForceTeamRespawn", InputForceTeamRespawn ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnForceRespawn, "OnForceRespawn" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_forcerespawn, CTFForceRespawn );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFForceRespawn::CTFForceRespawn()
{
}


void CTFForceRespawn::ForceRespawn( bool bSwitchTeams, int iTeam /*= TEAM_UNASSIGNED*/, bool bRemoveOwnedEnts /*= true*/ )
{
	if ( bSwitchTeams )
	{
		TFGameRules()->HandleSwitchTeams();
	}

	// respawn the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
		{
			continue;
		}

		// Do owned-entity-removal before checking the player's team or class for two reasons:
		// 1. Just in case the player does have projectiles or buildings in the world somehow
		// 2. Parity with the live TF2 implementation
		if ( bRemoveOwnedEnts )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}

		// Ignore players who haven't picked a game team yet (i.e. red/blu/grn/ylw)
		// And ignore players who haven't picked a class yet
		if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM || pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		{
			// Allow them to spawn instantly when they have chosen a team and class
			pPlayer->AllowInstantSpawn();
			continue;
		}

		if ( iTeam == TEAM_UNASSIGNED )
		{
			// No particular team was specified (i.e. we came from InputForceRespawn/InputForceRespawnSwitchTeams):
			// Force-respawn ALL players who are on a game team and have picked a class.
			pPlayer->ForceRespawn();
		}
		else
		{
			// A particular team was specified (i.e. we came from InputForceTeamRespawn):
			// Force-respawn ONLY players who are on that specific team; but NOT if they are not currently alive!
			// (In other words: make dead players instantly spawn; but don't make living players teleport back into the spawn room.)
			if ( pPlayer->GetTeamNumber() == iTeam && !pPlayer->IsAlive() )
			{
				pPlayer->ForceRespawn();
			}
		}
	}

	// remove any dropped weapons/ammo packs	
	for ( CTFAmmoPack *pPack : CTFAmmoPack::AutoList() )
	{
		UTIL_Remove( pPack );
	}

	// Output.
	m_outputOnForceRespawn.FireOutput( this, this );
}


void CTFForceRespawn::InputForceRespawn( inputdata_t &inputdata )
{
	ForceRespawn( false );
}


void CTFForceRespawn::InputForceRespawnSwitchTeams( inputdata_t &inputdata )
{
	ForceRespawn( true );
}


void CTFForceRespawn::InputForceTeamRespawn( inputdata_t &inputdata )
{
	ForceRespawn( false, inputdata.value.Int(), false );
}
