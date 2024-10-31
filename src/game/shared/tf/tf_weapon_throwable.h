//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: TF2C basic throwable.
//
//=============================================================================

#ifndef TF_WEAPON_THROWABLE_H
#define TF_WEAPON_THROWABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponThrowable C_TFWeaponThrowable
#endif

//=============================================================================
//
// TF Weapon Throwable.
//
class CTFWeaponThrowable : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS( CTFWeaponThrowable, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponThrowable();
	~CTFWeaponThrowable();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_THROWABLE_BRICK; }

	bool			CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual float	GetProjectileSpeed( void );
	virtual void	WeaponReset( void );

	float			GetProgress( void ) { return GetEffectBarProgress(); }
	const char		*GetEffectLabelText( void ) { return "#TF_Throwable"; }

	virtual float	InternalGetEffectBarRechargeTime( void ) { return 6.0f; }

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

	CTFWeaponThrowable( const CTFWeaponThrowable & ) {}
};

#endif // TF_WEAPON_THROWABLE_H
