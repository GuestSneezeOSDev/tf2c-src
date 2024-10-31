//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_tranq.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

//=============================================================================
//
// Weapon Tranq tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFTranq, tf_weapon_tranq )


bool CTFTranq::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	bool bRet = BaseClass::Holster( pSwitchingTo );
	if ( !bRet )
		return false;

	// The Tranq will reload when put away, like the Crusader's Crossbow.
	if ( m_iClip1 == 0 )
	{
		float flFireDelay = ApplyFireDelay( GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );
			
		float flReloadTime = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReload;
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time_hidden );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, fast_reload );
		
		float flIdleTime = GetLastPrimaryAttackTime() + flFireDelay + flReloadTime;
		if ( GetWeaponIdleTime() < flIdleTime )
		{
			SetWeaponIdleTime( flIdleTime );
			m_flNextPrimaryAttack = flIdleTime;
		}
		
		IncrementAmmo();
	}

	return true;
}
