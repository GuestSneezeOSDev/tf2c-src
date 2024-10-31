//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of Pyro's Flare Gun from live TF2
//
//=============================================================================
#ifndef TF_WEAPON_FLAREGUN_H
#define TF_WEAPON_FLAREGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#endif

class CTFFlareGun : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFFlareGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFFlareGun() {}

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_FLAREGUN; }

	virtual void	PrimaryAttack( void );

#ifdef GAME_DLL
	virtual bool TFBot_IsContinuousFireWeapon() OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()          OVERRIDE { return true;  }
#endif

private:
	CTFFlareGun( CTFFlareGun & );
};

#endif // TF_WEAPON_FLAREGUN_H
