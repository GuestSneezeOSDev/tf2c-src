//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_rocketlauncher.h"

ConVar tf2c_rocketlauncher_old_maxammo( "tf2c_rocketlauncher_old_maxammo", "0", FCVAR_REPLICATED );

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFRocketLauncher, tf_weapon_rocketlauncher )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::CTFRocketLauncher()
{
	m_bReloadsSingly = true;
}

#ifndef CLIENT_DLL

void CTFRocketLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "rocketbackblast" );
}
#endif


void CTFRocketLauncher::ItemBusyFrame( void )
{
	CBasePlayer *pOwner = GetPlayerOwner();
	if ( !pOwner )
		return;

	CheckBodyGroups();

	BaseClass::ItemPostFrame();
}


void CTFRocketLauncher::ItemPostFrame( void )
{
	CBasePlayer *pOwner = GetPlayerOwner();
	if ( !pOwner )
		return;

	CheckBodyGroups();

	BaseClass::ItemPostFrame();
}


int CTFRocketLauncher::GetMaxAmmo( void )
{
	if ( tf2c_rocketlauncher_old_maxammo.GetBool() )
	{
		return BaseClass::GetMaxAmmo() + 16;
	}

	return BaseClass::GetMaxAmmo();
}

#ifdef CLIENT_DLL

void CTFRocketLauncher::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt )
{
	BaseClass::CreateMuzzleFlashEffects( pAttachEnt );

	// Don't do this effect for local player since it's distracting.
	if ( IsCarriedByLocalPlayer() )
		return;

	ParticleProp()->Create( "rocketbackblast", PATTACH_POINT_FOLLOW, "backblast" );
}

#endif


void CTFRocketLauncher::CheckBodyGroups( void )
{
	SetBodygroup( 1, ( Clip1() > 0 || IsReloading() ) ? 0 : 1 );
}
