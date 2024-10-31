//=============================================================================
//
// Purpose: "Weapon" for testing cubemaps
//
//=============================================================================
#ifndef TF_WEAPON_CUBEMAP_H
#define TF_WEAPON_CUBEMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFWeaponCubemap C_TFWeaponCubemap
#endif

class CTFWeaponCubemap : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponCubemap, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID GetWeaponID( void ) const { return TF_WEAPON_CUBEMAP; }

	virtual void PrimaryAttack( void ) { /* Do nothing */ }
	virtual void SecondaryAttack( void ) { /* Do nothing */ }

#ifdef GAME_DLL
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE{ return false; }
	virtual bool TFBot_IsCombatWeapon()                      OVERRIDE{ return false; }
	virtual bool TFBot_IsExplosiveProjectileWeapon()         OVERRIDE{ return false; }
	virtual bool TFBot_IsContinuousFireWeapon()              OVERRIDE{ return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()            OVERRIDE{ return false; }
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE{ return true; }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies()     OVERRIDE{ return false; }
	virtual bool TFBot_ShouldAimForHeadshots()               OVERRIDE{ return false; }
	virtual bool TFBot_ShouldCompensateAimForGravity()       OVERRIDE{ return false; }
	virtual bool TFBot_IsSniperRifle()                       OVERRIDE{ return false; }
	virtual bool TFBot_IsRocketLauncher()                    OVERRIDE{ return false; }
#endif
};

#endif // TF_WEAPON_CUBEMAP_H
