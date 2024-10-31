//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFFlareGun, tf_weapon_flaregun );


void CTFFlareGun::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( pPlayer->GetWaterLevel() >= WL_Eyes )
	{
		if ( gpGlobals->curtime >= m_flNextEmptySoundTime )
		{
			WeaponSound( EMPTY );
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5f;
		}
		return;
	}

	BaseClass::PrimaryAttack();
}
