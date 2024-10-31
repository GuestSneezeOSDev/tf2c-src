//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"

#define SAPPHO_CARVED_CUTTER
#ifdef SAPPHO_CARVED_CUTTER
#include <in_buttons.h>
#endif
#include "tf_weapon_knife.h"
#include "decals.h"
#include "tf_lagcompensation.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Knife tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFKnife, DT_TFWeaponKnife );

BEGIN_NETWORK_TABLE( CTFKnife, DT_TFWeaponKnife )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bReadyToBackstab ) ),
#else
	SendPropBool( SENDINFO( m_bReadyToBackstab ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKnife )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bReadyToBackstab, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
PRECACHE_WEAPON_REGISTER( tf_weapon_knife );

//=============================================================================
//
// Weapon Knife functions.
//


CTFKnife::CTFKnife()
{
	m_bUnderwater = false;
}


void CTFKnife::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// You've got blood on my knife, now I have to clean it off.
	bool bPlayerIsUnderwater = pPlayer->IsPlayerUnderwater();
	if ( m_bUnderwater != bPlayerIsUnderwater )
	{
		ResetCritBodyGroup();
		m_bUnderwater = bPlayerIsUnderwater;
	}
}


void CTFKnife::ItemPostFrame( void )
{
	BackstabVMThink();
	BaseClass::ItemPostFrame();
}

bool CTFKnife::Deploy( void )
{
	bool bRet = BaseClass::Deploy();
	if ( !bRet )
		return bRet;

	// Probably a hack.
	ItemPreFrame();

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Set stealth attack bool
//-----------------------------------------------------------------------------
void CTFKnife::PrimaryAttack(void)
{
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());

	if (!CanAttack())
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION(pPlayer, pPlayer->GetCurrentCommand());

	trace_t trace;
	if (DoSwingTrace(trace))
	{
		// We will hit something with the attack.
		if (trace.m_pEnt && trace.m_pEnt->IsPlayer() && pPlayer->IsEnemy(trace.m_pEnt))
		{
			// Deal extra damage to players when stabbing them from behind.
			if (IsBehindAndFacingTarget(trace.m_pEnt))
			{
				// This will be a backstab, do the strong anim.
				m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

				// Store the victim to compare when we do the damage.
				m_hBackstabVictim = trace.m_pEnt;
			}
		}
	}

	FINISH_LAG_COMPENSATION();

	// Swing the weapon.
	Swing(pPlayer);

	// And hit instantly.
	if (m_flSmackTime == gpGlobals->curtime)
	{
		Smack();
		m_flSmackTime = 0.0f;
	}

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired(this);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFKnife::GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );

	if ( pTarget->IsPlayer() )
	{
		// Since Swing and Smack are done in the same frame now we don't need to run additional checks anymore.
		if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget )
		{
			// this will be a backstab, do the strong anim.
			// Do twice the target's health so that random modification will still kill him.
			flBaseDamage = pTarget->GetHealth() * 2;

			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;
		}
	}

	return flBaseDamage;
}



bool CTFKnife::IsBehindAndFacingTarget( CBaseEntity *pTarget )
{
	Assert( pTarget );

	int iCannotBeBackstabbed = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTarget, iCannotBeBackstabbed, cannot_be_backstabbed );
	if ( iCannotBeBackstabbed )
	{
		return false;
	}

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	// Get a forward vector of the attacker.
	Vector vecOwnerForward;
	AngleVectors( GetOwner()->EyeAngles(), &vecOwnerForward );
	vecOwnerForward.z = 0.0f;
	vecOwnerForward.NormalizeInPlace();

	float flDotOwner = DotProduct( vecOwnerForward, vecToTarget );
	float flDotVictim = DotProduct( vecVictimForward, vecToTarget );
	float flDotViews = DotProduct( vecOwnerForward, vecVictimForward );

	// Backstab requires 3 conditions to be met:
	// 1) Spy must be behind the victim (180 deg cone).
	// 2) Spy must be looking at the victim (120 deg cone).
	// 3) Spy must be looking in roughly the same direction as the victim (~215 deg cone).
	return ( flDotVictim > 0.0f && flDotOwner > 0.5f && flDotViews > -0.3f );
}


bool CTFKnife::CalcIsAttackCriticalHelper( void )
{
	// Always crit from behind, never from front
	return ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
//-----------------------------------------------------------------------------
void CTFKnife::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overriding so it doesn't do backstab animation on crit.
//-----------------------------------------------------------------------------
void CTFKnife::DoViewModelAnimation( void )
{
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to backstab.
//-----------------------------------------------------------------------------
bool CTFKnife::SendWeaponAnim( int iActivity )
{
	switch( iActivity )
	{
	case ACT_VM_IDLE:
		if ( m_bReadyToBackstab )
			iActivity = ACT_BACKSTAB_VM_IDLE;

		break;
	case ACT_BACKSTAB_VM_UP:
		m_bReadyToBackstab = true;
		break;
	case ACT_BACKSTAB_VM_DOWN:
	default:
		m_bReadyToBackstab = false;
		break;
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Check for knife raise conditions.
//-----------------------------------------------------------------------------
void CTFKnife::BackstabVMThink( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( GetActivity() == TranslateViewmodelHandActivity( ACT_VM_IDLE ) || GetActivity() == TranslateViewmodelHandActivity( ACT_BACKSTAB_VM_IDLE ) )
	{
		trace_t tr;
		if ( CanAttack() && DoSwingTrace( tr ) &&
			tr.m_pEnt->IsPlayer() && pOwner->IsEnemy( tr.m_pEnt ) &&
			IsBehindAndFacingTarget( tr.m_pEnt ) )
		{
			if ( !m_bReadyToBackstab )
			{
				SendWeaponAnim( ACT_BACKSTAB_VM_UP );
			}
		}
		else
		{
			if ( m_bReadyToBackstab )
			{
				SendWeaponAnim( ACT_BACKSTAB_VM_DOWN );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only switch to crit bodygroup on backstab
//-----------------------------------------------------------------------------
bool CTFKnife::ShouldSwitchCritBodyGroup( CBaseEntity *pEntity )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pEntity && !pOwner->IsPlayerUnderwater() )
	{
		CTFPlayer *pVictim = ToTFPlayer( m_hBackstabVictim.Get() );
		if ( pVictim && (pVictim->m_Shared.IsInvulnerable() || pVictim->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB)) ) return false;

		return BaseClass::ShouldSwitchCritBodyGroup( pEntity );
	}

	return false;
}
