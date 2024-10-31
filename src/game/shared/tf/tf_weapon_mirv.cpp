//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: TODO: Redo this into a base grenade throwing weapon.
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_mirv.h"
#include "tf_fx_shared.h"
#include "tf_weapon_grenade_mirv.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif
#define TF_MIRV_MAX_CHARGE_TIME 2.0f

//=============================================================================
//
// Weapon Mirv tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponMirv, DT_WeaponMirv )

BEGIN_NETWORK_TABLE( CTFWeaponMirv, DT_WeaponMirv )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponMirv )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_mirv, CTFWeaponMirv );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_mirv );

// alternative charged grenade for modders
CREATE_SIMPLE_WEAPON_TABLE( TFWeaponMirv2, tf_weapon_grenade_mirv2 )

//=============================================================================
//
// Weapon Mirv functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponMirv::CTFWeaponMirv()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponMirv::~CTFWeaponMirv()
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFWeaponMirv::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Drop MIRV on death if it interrupted our charging
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( ( pOwner && pOwner->HealthFraction() <= 0.0f ) && m_flChargeBeginTime > 0 )
	{
		LaunchGrenade();
	}

	m_flChargeBeginTime = 0;
	StopWeaponSound( WPN_DOUBLE );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFWeaponMirv::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}


void CTFWeaponMirv::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	m_flChargeBeginTime = 0.0f;
	StopWeaponSound( WPN_DOUBLE );
#endif
}


void CTFWeaponMirv::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

	if( pOwner )
	{
		// Reset charge if taunting
		if( pOwner->m_Shared.InCond(TF_COND_TAUNTING) )
		{
			m_flChargeBeginTime = 0;
			StopWeaponSound( WPN_DOUBLE );
		}
		if( !( pOwner->m_nButtons & IN_ATTACK ) )
		{
			if ( m_flChargeBeginTime > 0 )
			{
				LaunchGrenade();
			}
		}
	}
	BaseClass::ItemPostFrame();
}


void CTFWeaponMirv::PrimaryAttack( void )
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

		SendWeaponAnim( ACT_VM_PULLBACK );
	}
	else if ((gpGlobals->curtime - m_flChargeBeginTime) >= GetChargeMaxTime())
	{
		LaunchGrenade();
	}
}


void CTFWeaponMirv::LaunchGrenade( void )
{
	BaseClass::PrimaryAttack();

	m_flChargeBeginTime = 0;
	StartEffectBarRegen();
}


float CTFWeaponMirv::GetProjectileSpeed( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->HealthFraction() <= 0.0f )
		return 100.0f;

	float flVelocity = 750.0f;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	int iNoExtraSpeed = 0;
	CALL_ATTRIB_HOOK_INT(iNoExtraSpeed, chargeweapon_no_extra_speed);

	if ( iNoExtraSpeed )
		return flVelocity;

	return RemapValClamped( gpGlobals->curtime - m_flChargeBeginTime,
		0.0f, GetChargeMaxTime(),
		flVelocity, 3000.0f );
}


float CTFWeaponMirv::GetChargeMaxTime( void )
{
	float flChargeTime = TF_MIRV_MAX_CHARGE_TIME;
	CALL_ATTRIB_HOOK_FLOAT(flChargeTime, stickybomb_charge_rate);
	return flChargeTime;
}
