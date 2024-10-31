//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TF_WEAPON_ROCKETLAUNCHER_H
#define TF_WEAPON_ROCKETLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRocketLauncher C_TFRocketLauncher
#endif

//=============================================================================
//
// TF Weapon Rocket Launcher.
//
class CTFRocketLauncher : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFRocketLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRocketLauncher();
	~CTFRocketLauncher() {};

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_ROCKETLAUNCHER; }

#ifndef CLIENT_DLL
	virtual void	Precache();
#endif

	virtual void	ItemBusyFrame( void );
	virtual void	ItemPostFrame( void );
	virtual int		GetMaxAmmo( void );

#ifdef CLIENT_DLL
	virtual void	CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt );
#endif

	void			CheckBodyGroups( void );

#ifdef GAME_DLL
	virtual bool TFBot_IsExplosiveProjectileWeapon()     OVERRIDE { return true;  }
	virtual bool TFBot_IsContinuousFireWeapon()          OVERRIDE { return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()        OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies() OVERRIDE { return true;  }
	virtual bool TFBot_IsRocketLauncher()                OVERRIDE { return true;  }
#endif

	CTFRocketLauncher( const CTFRocketLauncher & );
};

#endif // TF_WEAPON_ROCKETLAUNCHER_H