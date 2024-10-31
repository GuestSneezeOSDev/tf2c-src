//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_throwingknife.h"
#include "tf_projectile_throwingknife.h"
#include "tf_lagcompensation.h"
// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
// Client specific.
#else
#include "c_tf_player.h"
#endif

extern ConVar tf_meleeattackforcescale;

#define ANTI_PROJECTILE_PASSTHROUGH_VALUE 0.0235f

//=============================================================================
//
// Weapon Throwing Knife tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFThrowingKnife, tf_weapon_throwingknife )

//=============================================================================
//
// Weapon Throwing Knife functions.
//



//-----------------------------------------------------------------------------
// Purpose: Check for that knife
//-----------------------------------------------------------------------------
void CTFThrowingKnife::PrimaryAttack(void)
{
	if (!HasPrimaryAmmoToFire())
	{
		HandleFireOnEmpty();
		return;
	}

	BaseClass::PrimaryAttack();
}


//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFThrowingKnife::GetMeleeDamage(CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage)
{
	return 0;	// Yep, nothing on melee hit!

	float flBaseDamage = BaseClass::BaseClass::GetMeleeDamage(pTarget, iCustomDamage);

	if (pTarget->IsPlayer())
	{
		// Since Swing and Smack are done in the same frame now we don't need to run additional checks anymore.
		if (m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget)
		{
			// this will be a backstab, do the strong anim.
			// Do twice the target's health so that random modification will still kill him.
			//flBaseDamage = pTarget->GetHealth() * 2;

			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;
		}
	}

	return flBaseDamage;
}

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFThrowingKnife::Smack(void)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION(pPlayer, pPlayer->GetCurrentCommand());
	
	// no melee in this version
	/*
	// We hit, setup the smack.
	trace_t trace;

	if (DoSwingTrace(trace))
	{
		Vector vecForward;
		AngleVectors(pPlayer->EyeAngles(), &vecForward);
		Vector vecSwingStart = pPlayer->EyePosition();
		Vector vecSwingEnd = vecSwingStart + vecForward * 48;

#ifndef CLIENT_DLL
		// Do Damage.
		ETFDmgCustom iCustomDamage = TF_DMG_CUSTOM_NONE;
		float flDamage = GetMeleeDamage(trace.m_pEnt, iCustomDamage);
		int iDmgType = GetDamageType();
		if (IsCurrentAttackACrit())
		{
			iDmgType |= DMG_CRITICAL;
		}

		{
			CDisablePredictionFiltering disabler;

			CTakeDamageInfo info(pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage);
			CalculateMeleeDamageForce(&info, vecForward, vecSwingEnd, 1.0f / flDamage * tf_meleeattackforcescale.GetFloat());
			trace.m_pEnt->DispatchTraceAttack(info, vecForward, &trace);
			ApplyMultiDamage();
		}
#endif

		OnEntityHit(trace.m_pEnt);

		// Don't impact trace friendly players or objects
		if (pPlayer->IsEnemy(trace.m_pEnt))
		{
			DoImpactEffect(trace, DMG_CLUB);
		}
	}
	else if (DoSwingTrace(trace, true))
	{
		if (HasPrimaryAmmoToFire())
			ThrowKnife(pPlayer);

		// No enemy hit. Found teammate to play impact sounds on:
		DoImpactEffect(trace, DMG_CLUB);
	}
	else
	{
		if (HasPrimaryAmmoToFire())
			ThrowKnife(pPlayer);

		if (!m_bForgiveMiss)
		{
			float flMarkSelfOnMiss = 0.0f;
			CALL_ATTRIB_HOOK_INT(flMarkSelfOnMiss, self_mark_for_death_on_miss);
			if (flMarkSelfOnMiss > 0.0f)
			{
				pPlayer->m_Shared.AddCond(TF_COND_MARKEDFORDEATH_SILENT, flMarkSelfOnMiss, pPlayer);
			}

			int iLoseStoredCrits = 0;
			CALL_ATTRIB_HOOK_INT(iLoseStoredCrits, lose_stored_crits_on_miss);
			if (iLoseStoredCrits)
			{
				pPlayer->m_Shared.RemoveStoredCrits(iLoseStoredCrits, true);
			}
		}
	}
	*/

	if (HasPrimaryAmmoToFire())
	{
		ThrowKnife(pPlayer);
		StartEffectBarRegen(false);
	}

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback);
	if (flSelfKnockback)
	{
		Vector vecForward;
		AngleVectors(pPlayer->EyeAngles(), &vecForward);

		Vector vecForwardNormalized = vecForward.Normalized();
		pPlayer->ApplyAbsVelocityImpulse(-vecForwardNormalized * flSelfKnockback);

#ifdef GAME_DLL
		pPlayer->m_bSelfKnockback = true;
#endif
	}

	FINISH_LAG_COMPENSATION();
}


CBaseEntity *CTFThrowingKnife::ThrowKnife(CTFPlayer *pPlayer)
{
#ifndef CLIENT_DLL
	m_hEnemy = nullptr;
#endif

	CBaseEntity *pProjectile = NULL;

	pProjectile = FireKnifeProjectile(pPlayer);

	// Set when this attack occurred.
	m_flLastPrimaryAttackTime = gpGlobals->curtime;

	int iAmmoPerShot = 1;

	int iOverride = 0;
	CALL_ATTRIB_HOOK_INT(iOverride, mod_ammo_per_shot);
	if (iOverride)
		iAmmoPerShot = iOverride;
	else
		iAmmoPerShot = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_iAmmoPerShot;

	int iAmmoCost = Min(abs(m_iClip1.Get()), iAmmoPerShot);

	if (UsesClipsForAmmo1())
	{
		m_iClip1 -= iAmmoCost;
	}
	else
	{
		if (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE)
		{
			pPlayer->RemoveAmmo(iAmmoCost, m_iPrimaryAmmoType);
		}
		else
		{
			pPlayer->RemoveAmmo(iAmmoCost, m_iSecondaryAmmoType);
		}
	}

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback);
	if (flSelfKnockback)
	{
		Vector vecForward;
		AngleVectors(pPlayer->EyeAngles(), &vecForward);

		Vector vecForwardNormalized = vecForward.Normalized();
		pPlayer->ApplyAbsVelocityImpulse(-vecForwardNormalized * flSelfKnockback);

#ifdef GAME_DLL
		pPlayer->m_bSelfKnockback = true;
#endif
	}

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity* CTFThrowingKnife::FireKnifeProjectile(CTFPlayer *pPlayer)
{
	if (IsCurrentAttackACrit())
	{
		WeaponSound(BURST);
	}
	else
	{
		WeaponSound(SINGLE);
	}

	// Server only - create the dart.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset(23.5f, 12.0f, -3.0f);

	if (pPlayer->GetFlags() & FL_DUCKING)
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup(pPlayer, vecOffset, &vecSrc, &angForward, false);

	trace_t trace;
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter(this, COLLISION_GROUP_NONE);
	UTIL_TraceLine(vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace);

	CTFProjectile_ThrowingKnife *pProjectile = CTFProjectile_ThrowingKnife::Create(this, trace.endpos, angForward, pPlayer);
	if (pProjectile)
	{
		pProjectile->SetCritical(IsCurrentAttackACrit());
		float flDamage = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;
		CALL_ATTRIB_HOOK_FLOAT(flDamage, mult_dmg);
		pProjectile->SetDamage(flDamage);
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFThrowingKnife::GetProjectileFireSetup(CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /*= true*/, bool bDoPassthroughCheck /*= false*/)
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp);

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = MASK_SOLID;
	bool bUseHitboxes = (GetDamageType() & DMG_USE_HITLOCATIONS) != 0;

	if (bUseHitboxes)
	{
		nMask |= CONTENTS_HITBOX;
	}

	if (bHitTeammates)
	{
		CTraceFilterSimple filter(pPlayer, COLLISION_GROUP_NONE);
		UTIL_TraceLine(vecShootPos, endPos, nMask, &filter, &tr);
	}
	else
	{
		CTraceFilterIgnoreTeammates filter(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());
		UTIL_TraceLine(vecShootPos, endPos, nMask, &filter, &tr);
	}

	int iCenterFire = 0;
	CALL_ATTRIB_HOOK_INT(iCenterFire, centerfire_projectile);

	if (iCenterFire)
	{
		vecOffset.y = 0.0f;
	}
	else if (IsViewModelFlipped())
	{
		// If viewmodel is flipped fire from the other side.
		vecOffset.y *= -1.0f;
	}

	// Offset actual start point
	*vecSrc = pPlayer->EyePosition() + vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;

	// Find angles that will get us to our desired end point
	if (bUseHitboxes)
	{
		// If too close, just fire from the center.
		if (tr.fraction < 0.02f)
		{
			*vecSrc = vecShootPos;
		}

		vecForward = tr.endpos - *vecSrc;
	}
	else
	{
		// Only use the trace end if it wasn't too close, which results
		// in visually bizarre forward angles
		vecForward = (tr.fraction > 0.1f) ? tr.endpos - *vecSrc : endPos - *vecSrc;

		// Attempt to totally prevent projectile passthrough, may screw up firing from certain angles.
		if (bDoPassthroughCheck && tr.fraction <= ANTI_PROJECTILE_PASSTHROUGH_VALUE)
		{
			float flOriginalZ = vecSrc->z;

			*vecSrc = pPlayer->GetAbsOrigin();
			vecSrc->z = flOriginalZ;
		}
	}

	VectorNormalize(vecForward);

	float flSpread = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flSpread;
	CALL_ATTRIB_HOOK_FLOAT(flSpread, mult_spread_scale);

	if (flSpread != 0.0f)
	{
		VectorVectors(vecForward, vecRight, vecUp);

		// Get circular gaussian spread.
		RandomSeed(GetPredictionRandomSeed() & 255);
		float x = RandomFloat(-0.5, 0.5) + RandomFloat(-0.5, 0.5);
		float y = RandomFloat(-0.5, 0.5) + RandomFloat(-0.5, 0.5);

		vecForward += vecRight * x * flSpread + vecUp * y * flSpread;
		VectorNormalize(vecForward);
	}

	VectorAngles(vecForward, *angForward);
}


bool CTFThrowingKnife::IsBehindAndFacingTarget(CBaseEntity *pTarget)
{
	return false;	// that's right, no backstabs in this version

	Assert(pTarget);

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors(pTarget->EyeAngles(), &vecVictimForward);
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	// Get a forward vector of the attacker.
	Vector vecOwnerForward;
	AngleVectors(GetOwner()->EyeAngles(), &vecOwnerForward);
	vecOwnerForward.z = 0.0f;
	vecOwnerForward.NormalizeInPlace();

	float flDotOwner = DotProduct(vecOwnerForward, vecToTarget);
	float flDotVictim = DotProduct(vecVictimForward, vecToTarget);
	float flDotViews = DotProduct(vecOwnerForward, vecVictimForward);

	// Backstab requires 3 conditions to be met:
	// 1) Spy must be behind the victim (180 deg cone).
	// 2) Spy must be looking at the victim (120 deg cone).
	// 3) Spy must be looking in roughly the same direction as the victim (~215 deg cone).
	return (flDotVictim > 0.0f && flDotOwner > 0.5f && flDotViews > -0.3f);
}