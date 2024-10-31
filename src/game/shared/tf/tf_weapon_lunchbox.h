//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: The edible device.
//
//=============================================================================

#ifndef TF_WEAPON_LUNCHBOX_H
#define TF_WEAPON_LUNCHBOX_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFLunchBox C_TFLunchBox
#endif

#define TF_SANDWICH_REGENTIME		30

//=============================================================================
//
// TF Weapon Lunchbox.
//
class CTFLunchBox : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFLunchBox, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFLunchBox();

	virtual void	UpdateOnRemove( void );
	virtual void	Precache();
	virtual ETFWeaponID		GetWeaponID( void ) const			{ return TF_WEAPON_LUNCHBOX; }
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
#ifdef GAME_DLL
	virtual void	WeaponRegenerate( void );
#endif
	virtual bool	UsesPrimaryAmmo();

	virtual bool	DropAllowed( void );

	float			GetProgress( void ) { return GetEffectBarProgress(); }
	const char		*GetEffectLabelText( void )	{ return "#TF_Sandwich"; }

	void			DrainAmmo( bool bForceCooldown = false );

	virtual float	InternalGetEffectBarRechargeTime( void ) { return TF_SANDWICH_REGENTIME; }

#ifdef GAME_DLL
	void			ApplyBiteEffects( CTFPlayer *pPlayer );

	virtual bool	HealsWhenDropped( void ) { return true; }

	virtual bool	SetupTauntAttack( int &iTauntAttack, float &flTauntAttackTime );

	virtual float	OwnerTakeDamage( const CTakeDamageInfo &info );
	virtual void	OwnerConditionAdded( ETFCond nCond );
	virtual void	OwnerConditionRemoved( ETFCond nCond );
#endif

#ifdef GAME_DLL
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE { return false; }
	virtual bool TFBot_IsCombatWeapon()                      OVERRIDE { return false; }
	virtual bool TFBot_IsExplosiveProjectileWeapon()         OVERRIDE { return false; }
	virtual bool TFBot_IsContinuousFireWeapon()              OVERRIDE { return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()            OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies()     OVERRIDE { return false; }
	virtual bool TFBot_ShouldAimForHeadshots()               OVERRIDE { return false; }
	virtual bool TFBot_ShouldCompensateAimForGravity()       OVERRIDE { return false; }
	virtual bool TFBot_IsSniperRifle()                       OVERRIDE { return false; }
	virtual bool TFBot_IsRocketLauncher()                    OVERRIDE { return false; }
#endif

private:
	CTFLunchBox( const CTFLunchBox & ) {}

	// Prevent spamming with resupply cabinets: only 1 thrown at a time
	EHANDLE		m_hThrownPowerup;
public:
	bool		IsCritboostedLunchbox( CTFPlayer *pOwner );
};

#endif // TF_WEAPON_LUNCHBOX_H
