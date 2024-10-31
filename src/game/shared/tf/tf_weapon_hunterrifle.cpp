//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Hunter Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_weapon_hunterrifle.h"
#include "in_buttons.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

//=============================================================================
//
// Weapon Hunter Rifles tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFHunterRifle, tf_weapon_hunterrifle )

//=============================================================================
//
// Weapon Hunter Rifles funcions.
//


float CTFHunterRifle::OwnerMaxSpeedModifier( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimingMoveSpeed = 120.0f;
		return flAimingMoveSpeed;
	}

	return 0.0f;
}