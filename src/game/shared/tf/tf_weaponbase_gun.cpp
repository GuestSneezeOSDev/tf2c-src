//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_fx_shared.h"
#include "tf_projectile_nail.h"
#include "in_buttons.h"
#include "tf_lagcompensation.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"

#if !defined( CLIENT_DLL )	// Server specific.

	#include "tf_player.h"

	#include "te_effect_dispatch.h"
	#include "tf_projectile_rocket.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "tf_weapon_grenade_stickybomb.h"
#ifdef TF2C_BETA
	#include "tf_weapon_grenade_healimpact.h"
	#include "tf_weapon_generator_uber.h"
#endif
	#include "tf_projectile_paintball.h"
	#include "tf_projectile_flare.h"
	#include "tf_projectile_arrow.h"
	#include "tf_projectile_coil.h"
	#include "tf_projectile_dart.h"
	#include "tf_projectile_paintball.h"
	#include "tf_projectile_brick.h"
	#include "tf_weapon_grenade_mirv.h"
	#include "tf_fx.h"

#else	// Client specific.

	#include "c_tf_player.h"
	#include "c_te_effect_dispatch.h"
	#include "engine/ivdebugoverlay.h"

#endif
#include <tf_projectile_cyclopsgrenade.h>
#include <collisionutils.h>

#define ANTI_PROJECTILE_PASSTHROUGH_VALUE 0.0235f

ConVar tf_debug_bullets( "tf_debug_bullets", "0", FCVAR_REPLICATED, "Visualize bullet traces." );
ConVar tf_use_fixed_weaponspreads( "tf_use_fixed_weaponspreads", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "0 - enforce random pellet distribution on weapons that fire multiple pellets per shot. 1 - enforce non-random pellet distrubtion. 2 - allow per-player preference." );
ConVar sv_showplayerhitboxes( "sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires." );
ConVar sv_showimpacts( "sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)" );
ConVar tf2c_show_aagun_explosion_radius("tf2c_show_aagun_explosion_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Shows explosive bullets explosion radius");
ConVar tf2c_show_aagun_hull( "tf2c_show_aagun_hull", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Shows explosive bullets hull" );
ConVar tf2c_sniperrifle_tracer("tf2c_sniperrifle_tracer", "1", FCVAR_REPLICATED, "Draw tracer for tf_weapon_sniperrifle class weapons");
ConVar tf2c_aagun_forgiveness("tf2c_aagun_forgiveness", "1", FCVAR_REPLICATED, "Adjusts bullet explosion position to match target on direct hit");
extern ConVar tf2c_bullets_pass_teammates;

Vector2D g_avecFixedWpnSpreadPellets[] =
{
	Vector2D( 0, 0 ),
	Vector2D( 0.5f, 0 ),
	Vector2D( -0.5f, 0 ),
	Vector2D( 0, -0.5f ),
	Vector2D( 0, 0.5f ),
	Vector2D( 0.425f, -0.425f ),
	Vector2D( 0.425f, 0.425f ),
	Vector2D( -0.425f, -0.425f ),
	Vector2D( -0.425f, 0.425f ),
	Vector2D( 0, 0 ),
};

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGun, DT_TFWeaponBaseGun )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGun, DT_TFWeaponBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGun )
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseGun )
	DEFINE_THINKFUNC( ZoomOutIn ),
	DEFINE_THINKFUNC( ZoomOut ),
	DEFINE_THINKFUNC( ZoomIn ),
	DEFINE_THINKFUNC( ExplosiveBulletDelay ),
END_DATADESC()
#endif

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGun::CTFWeaponBaseGun()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
}


void CTFWeaponBaseGun::ItemPostFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	BaseClass::ItemPostFrame();
}


void CTFWeaponBaseGun::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;
}


void CTFWeaponBaseGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
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

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	FireProjectile( pPlayer );

	if ( m_bUsesAmmoMeter ) {
		StartEffectBarRegen( true );
	}


	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	m_iReloadMode = TF_RELOAD_START;
}


void CTFWeaponBaseGun::SecondaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();
	m_bInAttack2 = true;
}


CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer )
{
#ifndef CLIENT_DLL
	m_hEnemy = nullptr;
#endif

	ProjectileType_t iProjectile = GetProjectileType();

	CBaseEntity *pProjectile = NULL;

	int iAmmoCost = Min(abs(m_iClip1.Get()), GetAmmoPerShot());

	if (m_iClip1.Get() < 0)
		iAmmoCost = GetAmmoPerShot();

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

	switch ( iProjectile )
	{
		case TF_PROJECTILE_BULLET:
			FireBullet( pPlayer );
			break;

		case TF_PROJECTILE_ARROW:
			pProjectile = FireArrow( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_ROCKET:
			pProjectile = FireRocket( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_DART:
			pProjectile = FireDart( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_COIL:
			pProjectile = FireCoil( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_FLARE:
			pProjectile = FireFlare( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_SYRINGE:
		case TF_PROJECTILE_NAIL:
			pProjectile = FireNail( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;

		case TF_PROJECTILE_PIPEBOMB:
		case TF_PROJECTILE_CANNONBALL:
		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE:
		case TF_PROJECTILE_MIRV:
#ifdef TF2C_BETA
		case TF_PROJECTILE_HEALGRENADEIMPACT:
		case TF_PROJECTILE_UBERGENERATOR:
		case TF_PROJECTILE_CYCLOPSGRENADE:
#endif
		case TF_PROJECTILE_BRICK: {
			pProjectile = FireGrenade( pPlayer, iProjectile );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
		}
		case TF_PROJECTILE_PAINTBALL: {
			pProjectile = FirePaintball( pPlayer );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			break;
		}
		case TF_PROJECTILE_NONE:
		default:
			// do nothing!
			DevMsg( "Weapon does not have a projectile type set!\n" );
			break;
	}

	// Set when this attack occurred.
	m_flLastPrimaryAttackTime = gpGlobals->curtime;

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback);
	int iSelfKnockbackDamageBoostModifier = 0;
	CALL_ATTRIB_HOOK_INT(iSelfKnockbackDamageBoostModifier, self_knockback_damage_boost_modifier);
	if (iSelfKnockbackDamageBoostModifier && (pPlayer->m_Shared.InCond(TF_COND_DAMAGE_BOOST) || IsWeaponDamageBoosted()))
		flSelfKnockback *= 1.35f;
	if (flSelfKnockback)
	{
		Vector vecForward;
		AngleVectors(pPlayer->EyeAngles(), &vecForward);

		Vector vecForwardNormalized = vecForward.Normalized();
		pPlayer->ApplyAbsVelocityImpulse(-vecForwardNormalized * flSelfKnockback);

#ifdef GAME_DLL
		pPlayer->m_bSelfKnockback = true;
#endif

		pPlayer->m_Shared.AddCond( TF_COND_LAUNCHED_SELF );
	}

	DoFireEffects();

	UpdatePunchAngles( pPlayer );

	return pProjectile;
}


void CTFWeaponBaseGun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	CALL_ATTRIB_HOOK_FLOAT(flPunchAngle, punch_angle_mod);

	if ( flPunchAngle > 0 )
	{
		// Consistent viewpunch by attribute
		int iConstantPunchAngle = 0;
		CALL_ATTRIB_HOOK_INT(iConstantPunchAngle, punch_angle_is_constant);
		if (iConstantPunchAngle)
		{
			angle.x -= flPunchAngle;
		}
		else
		{
			angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		}

		pPlayer->SetPunchAngle( angle );
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Hits triggers with raycasts
//-----------------------------------------------------------------------------
class CTFTriggerTraceEnum : public IEntityEnumerator
{
public:
	CTFTriggerTraceEnum( Ray_t *pRay, const CTakeDamageInfo &info, const Vector& dir, int contentsMask ) :
		m_info( info ), m_VecDir( dir ), m_ContentsMask( contentsMask ), m_pRay( pRay )
	{
	}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		trace_t tr;

		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );

		// Done to avoid hitting an entity that's both solid & a trigger.
		if ( pEnt->IsSolid() )
			return true;

		enginetrace->ClipRayToEntity( *m_pRay, m_ContentsMask, pHandleEntity, &tr );
		if ( tr.fraction < 1.0f )
		{
			pEnt->DispatchTraceAttack( m_info, m_VecDir, &tr );
		}

		return true;
	}

private:
	Vector m_VecDir;
	int m_ContentsMask;
	Ray_t *m_pRay;
	CTakeDamageInfo m_info;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ImpactWaterTrace( CTFPlayer *pPlayer, trace_t &trace, const Vector &vecStart )
{
	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_TFSHOT | CONTENTS_WATER | CONTENTS_SLIME ),
		pPlayer, COLLISION_GROUP_NONE, &traceWater );
	if ( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		if ( IsMinigun() )
		{
			// For the minigun, use a different, cheaper splash effect because it can create so many of them.
			pszEffectName = "tf_gunshotsplash_minigun";
		}
		DispatchEffect( pszEffectName, data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bullet!
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}

	Vector vecShootOrigin = pPlayer->Weapon_ShootPosition();
	QAngle vecShootAngle = pPlayer->EyeAngles() + pPlayer->GetPunchAngle();
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	float flSpread = GetWeaponSpread();
	int nBulletsPerShot = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_nBulletsPerShot;
	CALL_ATTRIB_HOOK_INT( nBulletsPerShot, mult_bullets_per_shot );

	int iExplosiveBullets = 0;
	CALL_ATTRIB_HOOK_INT( iExplosiveBullets, explosive_bullets );

	// Fire bullets, calculate impacts & effects.

	// Get the shooting angles.
	Vector vecShootForward, vecShootRight, vecShootUp;
	AngleVectors( vecShootAngle, &vecShootForward, &vecShootRight, &vecShootUp );

	// Reset multi-damage structures.
	ClearMultiDamage();

	int bFixedSpread = false;
	int iHorizontalSpread = 0;
	// Only shotguns should get spread pattern modifications.
	if ((GetDamageType() & DMG_BUCKSHOT) &&	nBulletsPerShot > 1)
	{
		int iServerSpread = tf_use_fixed_weaponspreads.GetInt();
		bool bPlayerPreference = pPlayer->FixedSpreadPreference();
		if (iServerSpread == 1 || (iServerSpread == 2 && bPlayerPreference == true))
			bFixedSpread = true;
		CALL_ATTRIB_HOOK_INT(iHorizontalSpread, mod_horizontal_spread);
	}

	// Move other players back to history positions based on local player's lag.
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	if ( sv_showplayerhitboxes.GetInt() > 0 )
	{
		CBasePlayer *pLagPlayer = UTIL_PlayerByIndex( sv_showplayerhitboxes.GetInt() );
		if ( pLagPlayer )
		{
#ifdef CLIENT_DLL
			pLagPlayer->DrawClientHitboxes( 4, true );
#else
			pLagPlayer->DrawServerHitboxes( 4, true );
#endif
		}
	}

	if (iHorizontalSpread && bFixedSpread)	// This gets its own loop
	{
		// Initialize random system with this seed.
		RandomSeed(iSeed);

		float x = 0.0f;
		float y = 0.0f;

		if (nBulletsPerShot)	// the first accurate shot
		{
			VectorNormalize(vecShootForward);
			FireIndividualBullet(pPlayer, vecShootOrigin, vecShootForward, iExplosiveBullets);
			++iSeed;
		}

		if (nBulletsPerShot > 1)
		{
			// init the pattern
			int iBulletsRemainder = (nBulletsPerShot - 1) % 3;
			int iBulletsLongLine = (iBulletsRemainder) ? (nBulletsPerShot - 1) / 3 + 1 : (nBulletsPerShot - 1) / 3;
			int iBulletsShortLine = (nBulletsPerShot - 1) / 3;
			int bPatternArray[3];	// true means to use long line bullets count, otherwise short
			float flBoundaryLength = 2.0f / iBulletsLongLine;
			for (int i = 0; i < 3; ++i)
			{
				if (!iBulletsRemainder)
				{
					bPatternArray[i] = true;
				}
				else if (iBulletsRemainder == 1 && i % 2)
				{
					bPatternArray[i] = true;
				}
				else
				{
					bPatternArray[i] = false;
				}
			}

			// iLine - vertical line number, iBullet - horizontal bullet number
			for (int iLine = 0; iLine < 3; ++iLine)
			{
				int iBulletsAmount = (bPatternArray[iLine]) ? iBulletsLongLine : iBulletsShortLine;
				for (int iBullet = 0; iBullet < iBulletsAmount; ++iBullet)
				{
					if (bPatternArray[iLine])
					{
						x = -1.0f + flBoundaryLength / 2.0f + flBoundaryLength * iBullet;
					}
					else
					{
						x = -1.0f + flBoundaryLength + ((2.0f - flBoundaryLength) / iBulletsShortLine) * iBullet;
					}
					y = -0.1f + 0.1f * iLine;
					// I don't understand this code block either anymore

					// Initialize the variable firing information.
					Vector vecBulletDir = vecShootForward + (x *  flSpread * vecShootRight) + (y * flSpread * vecShootUp);
					VectorNormalize(vecBulletDir);

					FireIndividualBullet(pPlayer, vecShootOrigin, vecBulletDir, iExplosiveBullets);

					// Use new seed for next bullet.
					++iSeed;
				}
			}
		}
	}
	else
	{
		for (int iBullet = 0; iBullet < nBulletsPerShot; ++iBullet)
		{
			// Initialize random system with this seed.
			RandomSeed(iSeed);

			float x = 0.0f;
			float y = 0.0f;

			// Determine if the first bullet should be perfectly accurate.
			bool bPerfectAccuracy = (iBullet == 0 && CanFireAccurateShot(nBulletsPerShot)
				&& !bFixedSpread);

			// See if we're using pre-determined spread pattern.
			if (!bPerfectAccuracy)
			{
				if (iHorizontalSpread)
				{
					x = -1.0f + (2.0f / (nBulletsPerShot - 1)) / 2.0f + (2.0f / (nBulletsPerShot - 1)) * iBullet;
					x += RandomFloat(-0.1f, 0.1f);
					y = RandomFloat(-0.2f, 0.2f);
				}
				else
				{
					if (bFixedSpread)
					{
						int idx = iBullet % ARRAYSIZE(g_avecFixedWpnSpreadPellets);
						x = g_avecFixedWpnSpreadPellets[idx].x;
						y = g_avecFixedWpnSpreadPellets[idx].y;
					}
					else
					{
						x = RandomFloat(-0.5, 0.5) + RandomFloat(-0.5, 0.5);
						y = RandomFloat(-0.5, 0.5) + RandomFloat(-0.5, 0.5);
					}
				}
			}

			// Initialize the variable firing information.
			Vector vecBulletDir = vecShootForward + (x *  flSpread * vecShootRight) + (y * flSpread * vecShootUp);
			VectorNormalize(vecBulletDir);

			FireIndividualBullet(pPlayer, vecShootOrigin, vecBulletDir, iExplosiveBullets);

			// Use new seed for next bullet.
			++iSeed;
		}
	}

	// Apply damage if any.
	CDisablePredictionFiltering disabler;
	if (!iExplosiveBullets)
		ApplyMultiDamage();

	// Lag comp goes AFTER Apply Multi Damage
	// So the shield properly lag comps
	// The flamethrower already does this and it seems to work fine
	FINISH_LAG_COMPENSATION();
}

//-----------------------------------------------------------------------------
// Purpose: Inner version after setting the bullet placement in FireBullet
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireIndividualBullet(CTFPlayer *pPlayer, const Vector& vecShootOrigin, const Vector& vecBulletDir, int iExplosiveBullets)
{
	int iDamageType = GetDamageType();
	if (IsCurrentAttackACrit())
	{
		iDamageType |= DMG_CRITICAL;
	}

	if (iExplosiveBullets)
	{
		iDamageType |= DMG_BLAST;
		iDamageType |= DMG_HALF_FALLOFF;
		iDamageType &= ~DMG_BULLET;
	}

#ifdef GAME_DLL
	float flDamage = GetProjectileDamage();
	ETFDmgCustom iCustomDamage = GetCustomDamageType();
#endif

	// Always collide with allies, bar the owner.
	// Whether this hitscan bullet can damage or heal is up to gamerules and attributes.
	int iHitsAllies = 0;
	CALL_ATTRIB_HOOK_INT( iHitsAllies, hitscan_hits_allies );

	float flHullSize = 0.0;
	CALL_ATTRIB_HOOK_FLOAT(flHullSize, bullet_is_a_hull);
	flHullSize /= 2;
	Vector vecHullSizeMin = Vector(-flHullSize);
	Vector vecHullSizeMax = Vector(flHullSize);

	// Fire a bullet (ignoring the shooter).
	float flRangeMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT(flRangeMult, mult_projectile_range);
	Vector vecEnd = vecShootOrigin + vecBulletDir * GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flRange * flRangeMult;
	trace_t trace;

	int iCustomBulletImpactCollisionMode = 0;
	CALL_ATTRIB_HOOK_INT(iCustomBulletImpactCollisionMode, mod_bullet_impact_collision_mode);
	switch (iCustomBulletImpactCollisionMode)
	{
	case 1:
		if (flHullSize)
		{
			UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &trace);

#ifdef GAME_DLL
			// Debug!
			if ( tf2c_show_aagun_hull.GetBool() )
			{
				NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
			}
#endif

			CTraceFilterSimple filter(pPlayer, COLLISION_GROUP_NONE);
			if (trace.fraction < 1.0f)
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if (!pHit || pHit->IsBSPModel())
				{
					FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
				}
			}
		}
		else
		{
			UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &trace);
		}
		break;
	case 2:
		if (flHullSize)
		{
			UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace);

#ifdef GAME_DLL
			// Debug!
			if ( tf2c_show_aagun_hull.GetBool() )
			{
				NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
			}
#endif

			CTraceFilterSimple filter(pPlayer, COLLISION_GROUP_NONE);
			if (trace.fraction < 1.0f)
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if (!pHit || pHit->IsBSPModel())
				{
					FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
				}
			}
		}
		else
		{
			UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace);
		}
		break;
	case 3:
	{
		CTraceFilterIgnoreTeammates filter(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());
		if (flHullSize)
		{
			UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_TFSHOT, &filter, &trace);

#ifdef GAME_DLL
			// Debug!
			if ( tf2c_show_aagun_hull.GetBool() )
			{
				NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
			}
#endif

			if (trace.fraction < 1.0f)
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if (!pHit || pHit->IsBSPModel())
				{
					FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
				}
			}
		}
		else
		{
			UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_TFSHOT, &filter, &trace);
		}
		break;
	}
	case 4:
	{
		CTraceFilterIgnoreTeammates filter(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());
		if (flHullSize)
		{
			UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_SOLID, &filter, &trace);

#ifdef GAME_DLL
			// Debug!
			if ( tf2c_show_aagun_hull.GetBool() )
			{
				NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
			}
#endif

			if (trace.fraction < 1.0f)
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if (!pHit || pHit->IsBSPModel())
				{
					FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
				}
			}
		}
		else
		{
			UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_SOLID, &filter, &trace);
		}
		break;
	}
	default:
		// Explosive bullets should behave like what you expect of rockets
		if (iExplosiveBullets)
		{
			CTraceFilterIgnoreTeammatesAndProjectiles filter(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());

			if (flHullSize)
			{
				UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_SOLID, &filter, &trace);

#ifdef GAME_DLL
				// Debug!
				if ( tf2c_show_aagun_hull.GetBool() )
				{
					NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
				}
#endif

				if (trace.fraction < 1.0f)
				{
					// Calculate the point of intersection of the line (or hull) and the object we hit
					// This is and approximation of the "best" intersection
					CBaseEntity *pHit = trace.m_pEnt;
					if (!pHit || pHit->IsBSPModel())
					{
						FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
					}
				}
			}
			else
			{
				UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_SOLID, &filter, &trace);
			}
		}
		// Let Sniper primaries ignore (non-lag-compensated) teammates to fix some "hit registration" complaints.
		// hitscan_hits_allies will override this behaviour.
		else if (( tf2c_bullets_pass_teammates.GetBool() || IsSniperRifle() ) && iHitsAllies<1)
		{
			CTraceFilterIgnoreTeammates filter(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());
			if (flHullSize)
			{
				UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_TFSHOT, &filter, &trace);

#ifdef GAME_DLL
				// Debug!
				if ( tf2c_show_aagun_hull.GetBool() )
				{
					NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
				}
#endif

				if (trace.fraction < 1.0f)
				{
					// Calculate the point of intersection of the line (or hull) and the object we hit
					// This is and approximation of the "best" intersection
					CBaseEntity *pHit = trace.m_pEnt;
					if (!pHit || pHit->IsBSPModel())
					{
						FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
					}
				}
			}
			else
			{
				UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_TFSHOT, &filter, &trace);
			}
		}
		else
		{
			if (flHullSize)
			{
				UTIL_TraceHull(vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &trace);

#ifdef GAME_DLL
				// Debug!
				if ( tf2c_show_aagun_hull.GetBool() )
				{
					NDebugOverlay::SweptBox( vecShootOrigin, vecEnd, vecHullSizeMin, vecHullSizeMax, vec3_angle, 255, 100, 100, 255, 2.0f );
				}
#endif

				CTraceFilterSimple filter(pPlayer, COLLISION_GROUP_NONE);
				if (trace.fraction < 1.0f)
				{
					// Calculate the point of intersection of the line (or hull) and the object we hit
					// This is and approximation of the "best" intersection
					CBaseEntity *pHit = trace.m_pEnt;
					if (!pHit || pHit->IsBSPModel())
					{
						FindHullIntersection(vecShootOrigin, trace, vecHullSizeMin, vecHullSizeMax, &filter);
					}
				}
			}
			else
			{
				UTIL_TraceLine(vecShootOrigin, vecEnd, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &trace);
			}
		}
		break;
	}


#ifdef GAME_DLL
	if (tf_debug_bullets.GetBool())
	{
		NDebugOverlay::Line(vecShootOrigin, trace.endpos, 0, 255, 0, true, 30);
	}
#endif

#ifdef CLIENT_DLL
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2)
	{
		// Draw red client impact markers.
		debugoverlay->AddBoxOverlay(trace.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 0, 0, 127, 4);

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			C_BasePlayer *player = ToBasePlayer(trace.m_pEnt);
			player->DrawClientHitboxes(4, true);
		}
	}
#else
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3)
	{
		// Draw blue server impact markers.
		NDebugOverlay::Box(trace.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 255, 127, 4);

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			CBasePlayer *player = ToBasePlayer(trace.m_pEnt);
			player->DrawServerHitboxes(4, true);
		}
	}
#endif

	static int tracerCount;
	bool bNeverDrawTracer = false;
	CALL_ATTRIB_HOOK_INT( bNeverDrawTracer, mod_never_draw_tracer_effect );
	if ( !bNeverDrawTracer )
	{
		bool bAlwaysDrawTracer = false;
		CALL_ATTRIB_HOOK_INT( bAlwaysDrawTracer, mod_always_draw_tracer_effect );
		bool bCanDrawTracer = GetTFWpnData().m_iTracerFreq != 0 && (tracerCount++ % GetTFWpnData().m_iTracerFreq) == 0;
		bool bCanDrawSniperTracer = bCanDrawTracer && tf2c_sniperrifle_tracer.GetBool();
		if ( ( ( GetWeaponID() == TF_WEAPON_SNIPERRIFLE ) ? bCanDrawSniperTracer : bCanDrawTracer ) || bAlwaysDrawTracer )
		{
			MakeTracer(vecShootOrigin, trace);
		}
	}

	if (trace.fraction < 1.0f)
	{
		// Verify we have an entity at the point of impact.
		Assert(trace.m_pEnt);

		// If shot starts out of water and ends in water.
		if (!(enginetrace->GetPointContents(trace.startpos) & (CONTENTS_WATER | CONTENTS_SLIME)) &&
			(enginetrace->GetPointContents(trace.endpos) & (CONTENTS_WATER | CONTENTS_SLIME)))
		{
			// Water impact effects.
			ImpactWaterTrace(pPlayer, trace, vecShootOrigin);
		}
		else
		{
			// Regular impact effects.

			// Don't decal your teammates or objects on your team, unless we're meant to hit allies.
			if ( pPlayer->IsEnemy( trace.m_pEnt ) || iHitsAllies > 0)
			{
				if (iExplosiveBullets)
				{
					//if (!trace.m_pEnt->IsPlayer())
					//{
					//	UTIL_DecalTrace(&trace, "Scorch");
					//}
				}
				else
				{
					DoImpactEffect(trace, iDamageType);
				}
			}

		}

		// False positive would frustrate players too much
		/*
		if (GetDamageType() & DMG_USE_HITLOCATIONS)
		{
			if (trace.m_pEnt->IsPlayer() &&
				pPlayer->IsEnemy(trace.m_pEnt) &&
				trace.hitgroup == HITGROUP_HEAD &&
				CanHeadshot())
			{
				// Play the critical shot sound to the shooter.
				WeaponSound(BURST);
			}
		}
		*/

		// Server specific.
#ifndef CLIENT_DLL
		if ( ( trace.m_pEnt->IsPlayer() || trace.m_pEnt->IsBaseObject() ) && pPlayer->IsEnemy( trace.m_pEnt ) )
		{
			m_hEnemy = trace.m_pEnt;
			// Guarantee that the bullet that hit an enemy trumps the player viewangles
			// that are locked in for the duration of the server simulation ticks.
			pPlayer->LockViewAngles();
		}

		CTakeDamageInfo dmgInfo(pPlayer, pPlayer, this, flDamage, iDamageType, iCustomDamage);
		if (!iExplosiveBullets)
		{
			// See what material we hit.
			CalculateBulletDamageForce(&dmgInfo, GetPrimaryAmmoType(), vecBulletDir, trace.endpos, 1.0f);
			trace.m_pEnt->DispatchTraceAttack(dmgInfo, vecBulletDir, &trace);
		}
		// Trace the bullet against triggers.
		Ray_t ray;
		ray.Init(vecShootOrigin, trace.endpos);

		CTFTriggerTraceEnum triggerTraceEnum(&ray, dmgInfo, vecBulletDir, MASK_TFSHOT);
		enginetrace->EnumerateEntities(ray, true, &triggerTraceEnum);
#endif
	}
#ifdef GAME_DLL
	if (iExplosiveBullets)
	{
		RegisterThinkContext( "ExplosiveBulletDelay" );

		m_pExplosiveBulletTrace = trace;
		m_flExplosiveBulletDamage = flDamage;
		m_iExplosiveBulletDamageType = iDamageType;
		m_hExplosiveBulletAttacker = pPlayer;
		m_hExplosiveBulletTarget = trace.m_pEnt;

		float flTraceLength = Vector(trace.endpos - trace.startpos).Length();
		float flDelayTime = flTraceLength * trace.fraction / 6000.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDelayTime, mod_explosive_bullet_delay_time );
		flDelayTime = Max( flDelayTime, 0.0f );

		SetContextThink( &CTFWeaponBaseGun::ExplosiveBulletDelay, gpGlobals->curtime + flDelayTime, "ExplosiveBulletDelay" );

		/*
		CDisablePredictionFiltering disabler;

		Vector vecBlastOrigin = trace.endpos;
		float flRadius = GetTFWpnData().m_flDamageRadius;
		if (flRadius <= 0.0f) flRadius = 64.0f;

		Vector vecReported = GetOwnerEntity() ? GetOwnerEntity()->GetAbsOrigin() : vecBlastOrigin;

		CTFRadiusDamageInfo radiusInfo;
		radiusInfo.info.Set(this, pPlayer, this, vec3_origin, vecBlastOrigin, flDamage, iDamageType, 0, &vecReported);
		radiusInfo.m_vecSrc = vecBlastOrigin;
		radiusInfo.m_flRadius = flRadius;
		radiusInfo.m_bStockSelfDamage = false;
		radiusInfo.m_flSelfDamageRadius = flRadius;

		TFGameRules()->RadiusDamage(radiusInfo);

		// Debug!
		if (tf2c_show_aagun_explosion_radius.GetBool())
		{
			NDebugOverlay::Sphere(vecBlastOrigin, flRadius, 255, 0, 0, false, 3);
		}

		CPVSFilter filter(vecBlastOrigin);
		int iEntIndex = (trace.m_pEnt && trace.m_pEnt->IsPlayer()) ? trace.m_pEnt->entindex() : -1;
		TE_TFExplosion(filter, 0.0f, vecBlastOrigin, trace.plane.normal, GetWeaponID(), iEntIndex, pPlayer, GetTeamNumber(), (iDamageType & DMG_CRITICAL) ? true : false, GetItemID());
		*/
	}
#endif
}

#ifdef GAME_DLL
void CTFWeaponBaseGun::ExplosiveBulletDelay( void )
{
	CDisablePredictionFiltering disabler;
	CBaseEntity *pTarget = m_hExplosiveBulletTarget.Get();
	CTFPlayer *pAttacker = m_hExplosiveBulletAttacker.Get();

	if (pAttacker)
	{
		int iEntIndex = (pTarget && pTarget->IsPlayer()) ? pTarget->entindex() : -1;
		Vector vecBlastOrigin = m_pExplosiveBulletTrace.endpos;

		if (tf2c_aagun_forgiveness.GetBool() && iEntIndex != -1)
		{
			// Do a bounding box check against the entity.
			Vector vecMins, vecMaxs;
			pTarget->GetCollideable()->WorldSpaceSurroundingBounds(&vecMins, &vecMaxs);
			CBaseTrace trace;
			Ray_t ray;
			ray.Init( m_pExplosiveBulletTrace.startpos, pTarget->GetLocalOrigin() );
			if (IntersectRayWithBox(ray, vecMins, vecMaxs, 0.0, &trace))
			{
				vecBlastOrigin.x = trace.endpos.x;
				vecBlastOrigin.y = trace.endpos.y;
			}
		}

		float flRadius = GetTFWpnData().m_flDamageRadius;
		CALL_ATTRIB_HOOK_FLOAT( flRadius, mult_explosion_radius );
		if ( flRadius <= 0.0f ) flRadius = 64.0f;

		Vector vecReported = GetOwnerEntity() ? GetOwnerEntity()->GetAbsOrigin() : vecBlastOrigin;

		CTFRadiusDamageInfo radiusInfo;
		radiusInfo.info.Set( this, pAttacker, this, vec3_origin, vecBlastOrigin, m_flExplosiveBulletDamage, m_iExplosiveBulletDamageType, 0, &vecReported);
		radiusInfo.m_vecSrc = vecBlastOrigin;
		radiusInfo.m_flRadius = flRadius;
		radiusInfo.m_bStockSelfDamage = false;
		radiusInfo.m_flSelfDamageRadius = flRadius;

		TFGameRules()->RadiusDamage( radiusInfo );

		// Debug!
		if ( tf2c_show_aagun_explosion_radius.GetBool() )
		{
			NDebugOverlay::Sphere( vecBlastOrigin, flRadius, 255, 0, 0, false, 3 );
		}

		if ( pTarget && !pTarget->IsPlayer() )
		{
			UTIL_DecalTrace( &m_pExplosiveBulletTrace, "Scorch" );
		}

		CPVSFilter filter( vecBlastOrigin );
		TE_TFExplosion( filter, 0.0f, vecBlastOrigin, m_pExplosiveBulletTrace.plane.normal, GetWeaponID(), iEntIndex, pAttacker, GetTeamNumber(), ( m_iExplosiveBulletDamageType & DMG_CRITICAL ) ? true : false, GetItemID() );
	}

	SetNextThink( TICK_NEVER_THINK, "ExplosiveBulletDelay" );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Return angles for a projectile reflected by airblast
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates /* = true */, bool bUseHitboxes /* = false */ )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = MASK_SOLID;

	if ( bUseHitboxes )
	{
		nMask |= CONTENTS_HITBOX;
	}

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}

	// vecPos is projectile's current position. Use that to find angles.

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 || bUseHitboxes )
	{
		vecDeflect = tr.endpos - vecPos;
	}
	else
	{
		vecDeflect = endPos - vecPos;
	}

	VectorNormalize( vecDeflect );
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /*= true*/, bool bDoPassthroughCheck /*= false*/ )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = MASK_SOLID;
	bool bUseHitboxes = ( GetDamageType() & DMG_USE_HITLOCATIONS ) != 0;

	if ( bUseHitboxes )
	{
		nMask |= CONTENTS_HITBOX;
	}

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}

//#ifdef GAME_DLL
	// Lets just make the calculations 1:1 on both client and server

	string_t strCustomOffset = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING( strCustomOffset, custom_projectile_origin_offset );
	if ( strCustomOffset != NULL_STRING )
	{
		float flCustomOffset[3];
		UTIL_StringToVector( flCustomOffset, STRING(strCustomOffset) );

		vecOffset.x += flCustomOffset[0];
		vecOffset.y += flCustomOffset[1];
		vecOffset.z += flCustomOffset[2];
	}

	int iCenterFire = 0;
	CALL_ATTRIB_HOOK_INT( iCenterFire, centerfire_projectile );
	int iNoCenterFire = 0;
	CALL_ATTRIB_HOOK_INT(iNoCenterFire, no_centerfire_projectile);

	if ( iCenterFire || (pPlayer->CenterFirePreference() && !iNoCenterFire))
	{
		vecOffset.y = 0.0f;
	}
	else if ( IsViewModelFlipped() )
	{
		// If viewmodel is flipped fire from the other side.
		vecOffset.y *= -1.0f;
	}

	// Offset actual start point
	*vecSrc = pPlayer->EyePosition() + vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;
/* #else
	// Fire nails from the weapon's muzzle.
	// Disabled for gameplay consistency. Moving first and third person muzzle positions caused very misleading projectiles
	*vecSrc = pPlayer->EyePosition() + vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;
	if ( pPlayer )
	{
		
		if ( !UsingViewModel() )
		{
			GetAttachment( GetTracerAttachment(), *vecSrc );
		}
		else
		{
			C_BaseEntity *pViewModel = GetPlayerViewModel();

			if ( pViewModel )
			{
				QAngle vecAngles;
				pViewModel->GetAttachment( GetTracerAttachment(), *vecSrc, vecAngles );

				Vector vForward;
				AngleVectors( vecAngles, &vForward );

				trace_t trace;
				UTIL_TraceLine( *vecSrc + vForward * -50, *vecSrc, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );

				*vecSrc = trace.endpos;
			}
		}
		
	}

#endif
	*/
	// Find angles that will get us to our desired end point
	if ( bUseHitboxes )
	{
		// If too close, just fire from the center.
		if ( tr.fraction < 0.02f )
		{
			*vecSrc = vecShootPos;
		}

		vecForward = tr.endpos - *vecSrc;
	}
	else
	{
		// Only use the trace end if it wasn't too close, which results
		// in visually bizarre forward angles
		vecForward = ( tr.fraction > 0.1f ) ? tr.endpos - *vecSrc : endPos - *vecSrc;

		// Attempt to totally prevent projectile passthrough, may screw up firing from certain angles.
		if ( bDoPassthroughCheck && tr.fraction <= ANTI_PROJECTILE_PASSTHROUGH_VALUE )
		{
			float flOriginalZ = vecSrc->z;

			*vecSrc = pPlayer->GetAbsOrigin();
			vecSrc->z = flOriginalZ;
		}
	}
	
	VectorNormalize( vecForward );
	
	float flSpread = GetWeaponSpread();

	if ( flSpread != 0.0f )
	{
		VectorVectors( vecForward, vecRight, vecUp );

		// Get circular gaussian spread.
		RandomSeed( GetPredictionRandomSeed() & 255 );
		float x = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
		float y = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );

		vecForward += vecRight * x * flSpread + vecUp * y * flSpread;
		VectorNormalize( vecForward );
	}

	VectorAngles( vecForward, *angForward );
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer, int iRocketType )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create( this, trace.endpos, angForward, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a projectile nail
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, ProjectileType_t iSpecificNail )
{
	PlayWeaponShootSound();

	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, Vector( 16, 6, -8 ), &vecSrc, &angForward );

	CTFBaseNail *pProjectile = NULL;
	switch ( iSpecificNail )
	{
		case TF_PROJECTILE_SYRINGE:
			pProjectile = CTFProjectile_Syringe::Create( TF_NAIL_SYRINGE, vecSrc, angForward, pPlayer, IsCurrentAttackACrit() );
			break;
		case TF_PROJECTILE_NAIL:
			pProjectile = CTFProjectile_Syringe::Create( TF_NAIL_NORMAL, vecSrc, angForward, pPlayer, IsCurrentAttackACrit() );
			break;
		default:
			Assert( 0 );
			break;
	}

	if ( pProjectile )
	{
#ifdef GAME_DLL
		pProjectile->SetLauncher( this );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
#endif
	}
	
	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Use this for any grenades: pipes, stickies, MIRV...
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireGrenade( CTFPlayer *pPlayer, ProjectileType_t iType )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecVelocity = vecForward * GetProjectileSpeed() +
		vecUp * 200.0f +
		vecRight * RandomFloat( -10.0f, 10.0f ) +
		vecUp * RandomFloat( -10.0f, 10.0f );

	AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );

	// No spin if requested:
	int iGrenadeNoSpin = 0;
	CALL_ATTRIB_HOOK_INT( iGrenadeNoSpin, grenade_no_spin );
	if ( iGrenadeNoSpin )
	{
		angVelocity = AngularImpulse( 0, 0, 0 );
	}

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 16.0f, 8.0f, -6.0f );
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, true );

	CTFBaseGrenade *pProjectile = NULL;

	switch( iType )
	{
		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE:
			pProjectile = CTFGrenadeStickybombProjectile::Create( vecSrc, pPlayer->EyeAngles(),
				vecVelocity, angVelocity,
				pPlayer, this );
			break;
		case TF_PROJECTILE_PIPEBOMB:
			pProjectile = CTFGrenadePipebombProjectile::Create( vecSrc, pPlayer->EyeAngles(),
				vecVelocity, angVelocity,
				pPlayer, this, iType );
			break;
		case TF_PROJECTILE_MIRV:
			pProjectile = CTFGrenadeMirvProjectile::Create( vecSrc, pPlayer->EyeAngles(),
				vecVelocity, angVelocity,
				pPlayer, this );
			break;
#ifdef TF2C_BETA
		case TF_PROJECTILE_HEALGRENADEIMPACT: {
			AngularImpulse noSpinny = AngularImpulse();
			pProjectile = CTFGrenadeHealImpactProjectile::Create( vecSrc, pPlayer->EyeAngles(),
				vecVelocity, noSpinny,
				pPlayer, this, iType );
		}
			break;
		case TF_PROJECTILE_UBERGENERATOR:
		{
			AngularImpulse noSpinny = AngularImpulse();
			pProjectile = CTFGeneratorUber::Create(vecSrc, pPlayer->EyeAngles(),
				vecVelocity, noSpinny,
				pPlayer, this);
			break;
		}

		case TF_PROJECTILE_CYCLOPSGRENADE:
			pProjectile = CTFCyclopsGrenadeProjectile::Create(vecSrc, pPlayer->EyeAngles(),
				vecVelocity, angVelocity,
				pPlayer, this, iType);
			break;
#endif
		case TF_PROJECTILE_BRICK:
			pProjectile = CTFBrickProjectile::Create( vecSrc, pPlayer->EyeAngles(),
				vecVelocity, angVelocity,
				pPlayer, this, iType );
			break;
	}

	if ( pProjectile )
	{
		pProjectile->SetDamage( GetProjectileDamage() );
		pProjectile->SetCritical( IsCurrentAttackACrit() );

		float flRadius = GetTFWpnData().m_flDamageRadius;
		CALL_ATTRIB_HOOK_FLOAT( flRadius, mult_explosion_radius );
		pProjectile->SetDamageRadius( flRadius );
	}

	// Cooking lowers detonation time attribute
	bool bGrenadeCooking = 0;
	CALL_ATTRIB_HOOK_INT(bGrenadeCooking, grenade_cook);

	if ( bGrenadeCooking )
	{
		if( iType != TF_PROJECTILE_PIPEBOMB_REMOTE && iType != TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE )
		{
			ITFChargeUpWeapon* pChargeupWeapon = dynamic_cast<ITFChargeUpWeapon*>(this);
			if ( pChargeupWeapon )
			{
				float flChargePct = floor(pChargeupWeapon->GetCurrentCharge() / 0.05) * 0.05; // round to nearest 0.05 down
				float flDetTime = pProjectile->GetDetonateTime() - gpGlobals->curtime;
				float flDetTimeNew = RemapValClamped(flChargePct, 0.0f, 1.0f, flDetTime, 0.0);
				DevMsg( "cooked grenade: pct: %2.2f, now: %2.2f, max: %2.2f\n", flChargePct, flDetTimeNew, flDetTime );
				pProjectile->SetDetonateTimerLength(flDetTimeNew);
			}
		}
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a flare
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireFlare( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the flare.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, true );

	CTFProjectile_Flare *pProjectile = CTFProjectile_Flare::Create( this, vecSrc, angForward, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire an arrow
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireArrow( CTFPlayer *pPlayer, ProjectileType_t projectileType )
{
	PlayWeaponShootSound();

	// Server only - create the arrow.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, GetViewModelType() == VMTYPE_TF2 ? -8.0f : 8.0f, -3.0f );

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Arrow *pProjectile = CTFProjectile_Arrow::Create( this, vecSrc, angForward, GetProjectileSpeed(), GetProjectileGravity(), projectileType, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );

		int iPenetrate = 0;
		CALL_ATTRIB_HOOK_INT( iPenetrate, projectile_penetration );
		if ( iPenetrate == 1 )
		{
			pProjectile->SetPenetrate( true );
		}
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireCoil( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the coil.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Coil *pProjectile = CTFProjectile_Coil::Create( this, trace.endpos, angForward, GetProjectileSpeed(), pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireDart( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the dart.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Dart *pProjectile = CTFProjectile_Dart::Create( this, trace.endpos, angForward, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;
#endif

	return NULL;
}

CBaseEntity *CTFWeaponBaseGun::FirePaintball( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the dart.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Paintball *pProjectile = CTFProjectile_Paintball::Create( this, trace.endpos, angForward, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;
#endif

	return NULL;
}

void CTFWeaponBaseGun::PlayWeaponShootSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}


ProjectileType_t CTFWeaponBaseGun::GetProjectileType( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	ProjectileType_t iProjectile = TF_PROJECTILE_NONE;
	bool bZoomed = false;
	if ( pOwner )
		bZoomed = pOwner->m_Shared.InCond( TF_COND_ZOOMED );

	if ( bZoomed )
		CALL_ATTRIB_HOOK_ENUM( iProjectile, override_projectile_type_zoomed );
	else
		CALL_ATTRIB_HOOK_ENUM( iProjectile, override_projectile_type );

	if ( iProjectile == TF_PROJECTILE_NONE )
		iProjectile = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_iProjectile;

	return iProjectile;
}


int CTFWeaponBaseGun::GetAmmoPerShot( void )
{
	int iOverride = 0;
	CALL_ATTRIB_HOOK_INT( iOverride, mod_ammo_per_shot );
	if ( iOverride )
		return iOverride;

	return GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
}


float CTFWeaponBaseGun::GetProjectileSpeed( void )
{
	// placeholder for now
	// grenade launcher and pipebomb launcher hook this to set variable pipebomb speed

	return GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flProjectileSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: Arrows use this for variable arc.
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileGravity( void )
{
	return 0.001f;
}


float CTFWeaponBaseGun::GetWeaponSpread( void )
{
	float flSpread = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flSpread;
	CALL_ATTRIB_HOOK_FLOAT( flSpread, mult_spread_scale );
	return flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileDamage( void )
{
	float flDamage = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );
	return flDamage;
}


bool CTFWeaponBaseGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#if !defined( CLIENT_DLL )

	// Make sure to zoom out before we holster the weapon.
	ZoomOut();
	SetContextThink( NULL, 0, ZOOM_CONTEXT );

#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoFireEffects()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	CEffectData data;
#ifdef GAME_DLL
	data.m_nEntIndex = entindex();
#else
	data.m_hEntity = this;
#endif

	CPVSFilter filter( pOwner->GetAbsOrigin() );
	te->DispatchEffect( filter, 0.0f, pOwner->GetAbsOrigin(), "TF_MuzzleFlash", data );
}


void CTFWeaponBaseGun::ToggleZoom( void )
{
	// Toggle the zoom.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		if( !pPlayer->m_Shared.InCond(TF_COND_ZOOMED) )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

	// Get the zoom animation time.
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.2f;
}


void CTFWeaponBaseGun::ZoomIn( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	int iSoftZoom = 0;
	CALL_ATTRIB_HOOK_INT( iSoftZoom, mod_sniper_soft_zoom );
	int iCustomFov = 0;
	CALL_ATTRIB_HOOK_INT( iCustomFov, mod_sniper_zoom_fov_override );

	// Set the weapon zoom.
	// TODO: The weapon fov should be gotten from the script file.
	int iFov = iCustomFov ? iCustomFov : (iSoftZoom ? 40 : 20);

	pPlayer->SetFOV( pPlayer, iFov, 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );
}


void CTFWeaponBaseGun::ZoomOut( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TF_COND_ZOOMED );
	}
}


void CTFWeaponBaseGun::ZoomOutIn( void )
{
	//Zoom out, set think to zoom back in.
	ZoomOut();
	SetContextThink( &CTFWeaponBaseGun::ZoomIn, gpGlobals->curtime + ZOOM_REZOOM_TIME, ZOOM_CONTEXT );
}
