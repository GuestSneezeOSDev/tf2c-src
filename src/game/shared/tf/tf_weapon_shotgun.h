//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#define CTFScatterGun C_TFScatterGun
#define CTFShotgunBuildingRescue C_TFShotgunBuildingRescue
#endif

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PRIMARY; }

#ifdef GAME_DLL
	virtual bool TFBot_IsBarrageAndReloadWeapon() OVERRIDE { return true; }
#endif

private:
	CTFShotgun( const CTFShotgun & ) {}
};

// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
};

class CTFShotgun_Soldier : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Soldier, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_SOLDIER; }
};

// Secondary version. Different weapon slot, different ammo
class CTFShotgun_HWG : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_HWG, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_HWG; }
};

class CTFShotgun_Pyro : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Pyro, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_PYRO; }
};

#if 0
class CTFShotgunBuildingRescue : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgunBuildingRescue, CTFShotgun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN_BUILDING_RESCUE; }

	virtual float GetProjectileSpeed( void ) { return 2400.0f; }
	virtual float GetProjectileGravity( void ) { return 0.2f; }
};
#endif

#endif // TF_WEAPON_SHOTGUN_H
