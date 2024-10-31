//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_PISTOL_H
#define TF_WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPistol C_TFPistol
#define CTFPistol_Scout C_TFPistol_Scout
#endif

//=============================================================================
//
// TF Weapon Pistol.
//
class CTFPistol : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFPistol, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFPistol();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PISTOL; }

	virtual void	Spawn( void );
	virtual void	ItemPostFrame( void );
	virtual float	GetFireRate( void );
	virtual CBaseEntity	*FireProjectile( CTFPlayer *pPlayer );

	bool UsingSemiAutoMode() const;

#ifdef GAME_DLL
	virtual bool TFBot_IsContinuousFireWeapon() OVERRIDE { return !UsingSemiAutoMode(); }
#endif

private:
	CTFPistol( const CTFPistol & );

	CNetworkVar( float, m_flSoonestPrimaryAttack );
	CNetworkVar( bool, m_bStock );
};

// Scout specific version
class CTFPistol_Scout : public CTFPistol
{
public:
	DECLARE_CLASS( CTFPistol_Scout, CTFPistol );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PISTOL_SCOUT; }
};

#endif // TF_WEAPON_PISTOL_H