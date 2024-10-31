//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_MIRV_H
#define TF_WEAPON_MIRV_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponMirv C_TFWeaponMirv
#define CTFWeaponMirv2 C_TFWeaponMirv2
#endif

//=============================================================================
//
// TF Weapon Pipebomb Launcher.
//
class CTFWeaponMirv : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS( CTFWeaponMirv, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponMirv();
	~CTFWeaponMirv();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADE_MIRV; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual float	GetProjectileSpeed( void );
	virtual void	WeaponReset( void );

	float			GetProgress( void ) { return GetEffectBarProgress(); }
	const char		*GetEffectLabelText( void ) { return "#TF_MIRV"; }

	virtual float	InternalGetEffectBarRechargeTime( void ) { return 10.0f; }

public:
	// ITFChargeUpWeapon
	virtual float GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float GetChargeMaxTime( void );

	void			LaunchGrenade( void );

#ifdef GAME_DLL
	virtual bool TFBot_IsContinuousFireWeapon()          OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()                   OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies() OVERRIDE { return true;  }
#endif

private:
	CNetworkVar( float, m_flChargeBeginTime );

	CTFWeaponMirv( const CTFWeaponMirv & ) {}
};

class CTFWeaponMirv2 : public CTFWeaponMirv
{
public:
		DECLARE_CLASS( CTFWeaponMirv2, CTFWeaponMirv );
		DECLARE_NETWORKCLASS();
		DECLARE_PREDICTABLE();
		
		CTFWeaponMirv2() {};
		~CTFWeaponMirv2() {};

		virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADE_MIRV2; }
private:
		CTFWeaponMirv2( const CTFWeaponMirv2 & ) {}
};
#endif // TF_WEAPON_MIRV_H
