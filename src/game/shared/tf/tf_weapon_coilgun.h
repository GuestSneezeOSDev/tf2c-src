//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_RAILGUN_H
#define TF_WEAPON_RAILGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "soundenvelope.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFCoilGun C_TFCoilGun
#endif

#define TF_COILGUN_MAX_CHARGE_DAMAGE 75.0f
#define TF_COILGUN_MIN_CHARGE_VEL 1800.0f
#define TF_COILGUN_MAX_CHARGE_VEL 3000.0f
#define TF_COILGUN_CHARGE_FIRE_DELAY 0.2f

//=============================================================================
//
// TF Weapon Classic Railgun
//
class CTFCoilGun : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:
	DECLARE_CLASS( CTFCoilGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFCoilGun();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_COILGUN; }

	virtual void	WeaponReset( void );
	virtual void	Precache( void );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Reload( void ) { if ( m_flChargeBeginTime > 0.0f ) return false; return BaseClass::Reload(); }
	virtual bool	ShouldBlockPrimaryFire( void ) { return true; }
	virtual void	UpdateOnRemove( void );
	void			StopCharging( void );

	virtual int		GetDamageType() const;
	virtual float	GetProjectileDamage( void );
	virtual float	GetProjectileSpeed( void );
	virtual float	GetWeaponSpread( void );

	virtual float	GetCoilChargeTime( void );
	virtual float	GetCoilChargeExplodeTime( void );


	// ITFChargeUpWeapon
	virtual float	GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float	GetChargeMaxTime( void );
	virtual float	GetCurrentCharge( void );
	virtual bool	ChargeMeterShouldFlash( void );

	float			GetProgress( void );
	const char		*GetEffectLabelText( void ) { return "#TF_Charge"; }

#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void );
#endif

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			UpdateChargeEffects( void );
#endif

private:
	CNetworkVar( float, m_flChargeBeginTime );

#ifdef CLIENT_DLL
	bool m_bWasCharging;
	CSoundPatch *m_pChargeSound;

	bool			m_bDidBeep;
#endif

	CTFCoilGun( const CTFCoilGun & );
};

#endif // TF_WEAPON_RAILGUN_H
