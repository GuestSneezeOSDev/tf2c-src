//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: TF2C basic throwable.
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_throwable.h"
#include "tf_fx_shared.h"
#include "tf_projectile_brick.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif
#define TF_THROWABLE_MAX_CHARGE_TIME 0.15f

//=============================================================================
//
// Weapon Throwable tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponThrowable, DT_WeaponThrowable )

BEGIN_NETWORK_TABLE( CTFWeaponThrowable, DT_WeaponThrowable )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponThrowable )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_throwable, CTFWeaponThrowable );
PRECACHE_WEAPON_REGISTER( tf_weapon_throwable );

//=============================================================================
//
// Weapon Throwable functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponThrowable::CTFWeaponThrowable()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponThrowable::~CTFWeaponThrowable()
{
}

//-----------------------------------------------------------------------------
// Purpose: Can't holster during windup
//-----------------------------------------------------------------------------
bool CTFWeaponThrowable::CanHolster( void ) const
{
	if ( m_flChargeBeginTime != 0.0f )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFWeaponThrowable::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0;
	StopWeaponSound( WPN_DOUBLE );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFWeaponThrowable::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}


void CTFWeaponThrowable::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	m_flChargeBeginTime = 0.0f;
	StopWeaponSound( WPN_DOUBLE );
#endif
}


void CTFWeaponThrowable::ItemPostFrame( void )
{
	CTFPlayer* pOwner = ToTFPlayer( GetOwner() );

	if ( pOwner )
	{
		// Reset charge if taunting
		if ( pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			m_flChargeBeginTime = 0;
			StopWeaponSound( WPN_DOUBLE );
		}
	}

	// Wait for initial throw delay. Feels better with windup.
	// i.e. unlike MIRV, letting go of M1 does not launch early!
	if ( m_flChargeBeginTime > 0 )
	{
		if ( ( gpGlobals->curtime - m_flChargeBeginTime ) >= GetChargeMaxTime() )
		{
			LaunchGrenade();
#ifdef CLIENT_DLL
			if (ParticleProp())
				ParticleProp()->StopEmissionAndDestroyImmediately();
#endif
		}
	}

	BaseClass::ItemPostFrame();
}


void CTFWeaponThrowable::PrimaryAttack( void )
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;
		WeaponSound( WPN_DOUBLE );

		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	}
}


void CTFWeaponThrowable::LaunchGrenade( void )
{
	// Copy of BaseClass::PrimaryAttack() without the SendWeaponAnim
	// and m_iReloadMode tweak to stop it showing at 0 ammo before holster.
	
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->NoteWeaponFired( this );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	FireProjectile( pPlayer );

	if ( m_bUsesAmmoMeter ) {
		StartEffectBarRegen( true );
	}

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Don't go back to idle anim unless we have more than 1 ammo
	if ( HasPrimaryAmmoToFire() )
	{
		m_iReloadMode = TF_RELOAD_START;
	}
	else
	{
		m_iReloadMode = TF_RELOAD_FINISH;
	}

	// Unique additions start here
	m_flChargeBeginTime = 0;
	StartEffectBarRegen();
}


float CTFWeaponThrowable::GetProjectileSpeed( void )
{
	float flVelocity = 2000.0f;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );
	return flVelocity;
}


float CTFWeaponThrowable::GetChargeMaxTime( void )
{
	float flChargeTime = TF_THROWABLE_MAX_CHARGE_TIME;
	CALL_ATTRIB_HOOK_FLOAT(flChargeTime, stickybomb_charge_rate);
	return flChargeTime;
}
