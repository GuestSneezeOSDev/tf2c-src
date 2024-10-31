//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseprojectile.h"

ConVar tf2c_projectile_ally_collide( "tf2c_projectile_ally_collide", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "If enabled, projectiles start colliding with teammates 0.25s after being fired or reflected." );
ConVar tf2c_experimental_projectilehitdet("tf2c_experimental_projectilehitdet", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT, "Enables the new hit detection logic for projectiles. WARNING: Almost definitly broken until Hogyn says otherwise, don't use in any real playtests yet.");

IMPLEMENT_NETWORKCLASS_ALIASED( BaseProjectile, DT_BaseProjectile )

BEGIN_NETWORK_TABLE( CBaseProjectile, DT_BaseProjectile )
#if !defined( CLIENT_DLL )
	SendPropEHandle( SENDINFO( m_hOriginalLauncher ) ),
#else
	RecvPropEHandle( RECVINFO( m_hOriginalLauncher ) ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()


#ifndef CLIENT_DLL
IMPLEMENT_AUTO_LIST( IBaseProjectileAutoList );
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CBaseProjectile::CBaseProjectile()
{
#ifdef GAME_DLL
	m_iDestroyableHitCount = 0;

	m_bCanCollideWithTeammates = false;
#endif
	m_hOriginalLauncher = NULL;
}



void CBaseProjectile::SetLauncher( CBaseEntity *pLauncher )
{
	if ( m_hOriginalLauncher == NULL )
	{
		m_hOriginalLauncher = pLauncher;
	}

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}



void CBaseProjectile::Spawn()
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}


#ifdef GAME_DLL


void CBaseProjectile::CollideWithTeammatesThink()
{
	m_bCanCollideWithTeammates = true;
}



void CBaseProjectile::ResetCollideWithTeammates()
{
	// Don't collide with players on the owner's team for the first bit of our life
	m_bCanCollideWithTeammates = false;
	
	if ( tf2c_projectile_ally_collide.GetBool() )
	{
		SetContextThink( &CBaseProjectile::CollideWithTeammatesThink, gpGlobals->curtime + GetCollideWithTeammatesDelay(), "CollideWithTeammates" );
	}
}

#endif // GAME_DLL

