
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_RIOT_H
#define TF_WEAPON_RIOT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFRiot C_TFRiot
#endif

//=============================================================================
//
// Riot Shield class.
//
class CTFRiot : public CTFWeaponBaseMelee, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS(CTFRiot, CTFWeaponBaseMelee);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFRiot();

	virtual void Precache(void);

#ifdef GAME_DLL
	int		AbsorbDamage( CTakeDamageInfo &info );
	void	BreakShield( void );
	float	GetBlockedDamage( void ){ return m_flBlockedDamage; }
	void	ClearBlockedDamage( void ){ m_flBlockedDamage = 0.0f; }
#else
	void	DrawShieldWireframe( int iCurrentAmmo );
#endif

	// Since we use Ammo as health, use the default ammo behavior
	virtual bool	HasPrimaryAmmo() { return CTFWeaponBase::HasPrimaryAmmo(); }
	virtual bool	CanDeploy() { return !m_bBroken; }
	virtual bool	CanBeSelected() { return m_bBroken ? false : CTFWeaponBase::CanBeSelected(); }

	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual void	SecondaryAttack();
#ifdef GAME_DLL
	void			WeaponRegenerate( void );
#endif
	virtual bool	UsesPrimaryAmmo( void );

	virtual void	ItemBusyFrame( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemHolsterFrame( void );

	virtual void	Smack( void );

	virtual void	ShieldChargeThink( void );
	void			StopCharge( void );
	virtual float	GetProgress( void ){ return GetEffectBarProgress(); }
	virtual float	GetEffectBarProgress( void );
	const char		*GetEffectLabelText( void )	{ return "#TF_Shield"; }

	virtual float	InternalGetEffectBarRechargeTime( void );

	// ITFChargeUpWeapon
	virtual float	GetChargeBeginTime( void );
	virtual float	GetChargeMaxTime( void );
	virtual float	GetCurrentCharge( void );
	virtual bool	ChargeMeterShouldFlash() { return GetCurrentCharge() == GetChargeMaxTime(); }

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_RIOT_SHIELD; }

private:
	CTFRiot( const CTFRiot & );

	CNetworkVar( float, m_flRegenStartTime );
	CNetworkVar( float, m_flAmmoToGive );

	CNetworkVar( float,	m_flChargeMeter );

	CNetworkVar( float, m_flBlockedDamage );
	CNetworkVar( bool, m_bBroken );
#ifdef GAME_DLL
	float	m_flTimeSlow;
#endif
};

#endif // TF_WEAPON_RIOT_H