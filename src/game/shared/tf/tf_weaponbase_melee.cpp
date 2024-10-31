//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"
#include "tf_lagcompensation.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_weaponbase_rocket.h"
// Client specific.
#else
#include "c_tf_player.h"
#include "particles_new.h"
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMelee, DT_TFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTFWeaponBaseMelee, DT_TFWeaponBaseMelee )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSmackTime ) ),
	RecvPropBool( RECVINFO( m_bLandedCrit ) ),
	RecvPropBool( RECVINFO (m_bForgiveMiss) ),
#else
	SendPropTime( SENDINFO( m_flSmackTime ) ),
	SendPropBool( SENDINFO( m_bLandedCrit ) ),
	SendPropBool( SENDINFO( m_bForgiveMiss ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flSmackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bLandedCrit, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

#ifndef CLIENT_DLL
ConVar tf2c_groundedprojectiledistance( "tf2c_groundedprojectiledistance", "16", FCVAR_CHEAT | FCVAR_GAMEDLL );
extern ConVar tf2c_afterburn_damage;
#endif

ConVar tf_meleeattackforcescale( "tf_meleeattackforcescale", "80.0", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Controls random crits for melee weapons.\n0 - Melee weapons do not randomly crit. \n1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. \n2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting.", true, 0, true, 2 );
extern ConVar tf2c_airblast_players;
extern ConVar tf_debug_airblast;
extern ConVar tf2c_airblast_gracetime;
extern ConVar tf_weapon_criticals;
extern ConVar tf2c_vip_criticals;

#define TF2C_TRANQ_FIRERATE_MELEE_FACTOR	1.33f
#define TF2C_HASTE_FIRERATE_MELEE_FACTOR	0.75f

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBaseMelee::CTFWeaponBaseMelee()
{
#ifdef CLIENT_DLL
	m_iCritBodygroup = -1;
#endif
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Deploy( void )
{
	bool bRet = BaseClass::Deploy();

	if ( bRet )
	{
#ifdef CLIENT_DLL
		SwitchBodyGroups();
#endif
	}

#ifdef GAME_DLL
	int iDeflectSwing = 0;
	CALL_ATTRIB_HOOK_INT( iDeflectSwing, deflect_on_swing );
	m_bCanDeflect = iDeflectSwing > 0;
#endif

	return bRet;
}

void CTFWeaponBaseMelee::Precache( void )
{
	PrecacheScriptSound( "Weapon_Bat.Deflect" );
	BaseClass::Precache();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	
#ifdef GAME_DLL
	int iDeflectSwing = 0;
	CALL_ATTRIB_HOOK_INT( iDeflectSwing, deflect_on_swing );
	m_bCanDeflect = iDeflectSwing > 0;
#endif

	ResetCritBodyGroup();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanHolster( void ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) )
			return false;
	
		int iSelfMark = 0;
		CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
		if ( iSelfMark )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_flNextAttack = gpGlobals->curtime + 0.5f;
	}

	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// Swing the weapon.
	Swing( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired( this );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();
	m_bInAttack2 = true;
}


void CTFWeaponBaseMelee::DoImpactEffect( trace_t &tr, int nDamageType )
{
#ifdef GAME_DLL
	if ( tr.m_pEnt->IsPlayer() )
	{
		WeaponSound( MELEE_HIT );
	}
	else
	{
		WeaponSound( MELEE_HIT_WORLD );
	}

	// Play the sound to the Client as well
	const char *shootsound = GetShootSound( tr.m_pEnt->IsPlayer() ? MELEE_HIT : MELEE_HIT_WORLD );
	if( shootsound && shootsound[0] && GetOwner() )
	{
		CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );

		EmitSound_t params;
		params.m_pSoundName = shootsound;
		params.m_flSoundTime = 0;
		params.m_pflSoundDuration = NULL;
		params.m_bWarnOnDirectWaveReference = true;

		EmitSound( filter, entindex(), params );
	}
#endif

	string_t strImpactParticle = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING( strImpactParticle, particle_on_melee_hit );
	if ( strImpactParticle != NULL_STRING )
	{
		//DispatchParticleEffect( STRING( strImpactParticle ), tr.endpos, vec3_angle );
#ifdef CLIENT_DLL
		CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, strImpactParticle );
		if ( pEffect->IsValid() )
		{
			Vector vecImpactPoint = ( tr.fraction != 1.0f ) ? tr.endpos : tr.startpos;
			Vector vecShotDir = vecImpactPoint - tr.startpos;

			Vector	vecReflect;
			float	flDot = DotProduct( vecShotDir, tr.plane.normal );
			VectorMA( vecShotDir, -2.0f * flDot, tr.plane.normal, vecReflect );

			Vector vecShotBackward;
			VectorMultiply( vecShotDir, -1.0f, vecShotBackward );

			Vector vecImpactY, vecImpactZ;
			VectorVectors( tr.plane.normal, vecImpactY, vecImpactZ );
			vecImpactY *= -1.0f;

			// CP 0
			pEffect->SetControlPoint( 0, vecImpactPoint );
			pEffect->SetControlPointOrientation( 0, tr.plane.normal, vecImpactY, vecImpactZ );
			pEffect->SetControlPointEntity( 0, tr.m_pEnt );

			VectorVectors( vecReflect, vecImpactY, vecImpactZ );
			vecImpactY *= -1.0f;

			// CP 1
			pEffect->SetControlPoint( 1, vecImpactPoint );
			pEffect->SetControlPointOrientation( 1, vecReflect, vecImpactY, vecImpactZ );
			pEffect->SetControlPointEntity( 1, tr.m_pEnt );

			VectorVectors( vecShotBackward, vecImpactY, vecImpactZ );
			vecImpactY *= -1.0f;

			// CP 2
			pEffect->SetControlPoint( 2, vecImpactPoint );
			pEffect->SetControlPointOrientation( 2, vecShotBackward, vecImpactY, vecImpactZ );
			pEffect->SetControlPointEntity( 2, tr.m_pEnt );

			// CP 3 (scale)
			pEffect->SetControlPoint( 3, Vector( 1.0f ) );
		}
#endif //CLIENT_DLL
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
	CalcIsAttackCritical();

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );
	DoViewModelAnimation( false );

	float flFireRate = GetFireRate();

	// rework time, no more melee debuffs!
	/*if (pPlayer->m_Shared.InCond(TF_COND_TRANQUILIZED))
	{
		flFireRate *= TF2C_TRANQ_FIRERATE_MELEE_FACTOR;
	}*/

	if ( pPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flFireRate *= TF2C_HASTE_FIRERATE_MELEE_FACTOR;
	}

	if ( flFireRate != GetFireRate() )
	{
		CBaseViewModel *pViewModel = pPlayer->GetViewModel( m_nViewModelIndex );
		if ( pViewModel )
		{
			pViewModel->SetPlaybackRate( GetFireRate() / flFireRate );
		}
	}

	// Set when this attack occurred.
	m_flLastPrimaryAttackTime = gpGlobals->curtime;

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + flFireRate;

	//SetWeaponIdleTime( m_flNextPrimaryAttack + GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}

	m_bForgiveMiss = CheckForgiveness();

#ifndef CLIENT_DLL
	string_t strAddCondOnSwing = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING(strAddCondOnSwing, add_onswing_addcond);
	if (strAddCondOnSwing != NULL_STRING)
	{
		float args[1];
		UTIL_StringToFloatArray(args, 2, strAddCondOnSwing.ToCStr());
		DevMsg("add_swing_addcond: %2.2f, %2.2f\n", args[0], args[1]);

		pPlayer->m_Shared.AddCond((ETFCond)Floor2Int(args[0]), args[1], pPlayer);
	}
#endif

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback_swing);
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
	}

	// Must do this before Deflect, since Deflect can set the smack timer to 0 if it deflects something.
	m_flSmackTime = gpGlobals->curtime + GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
	CALL_ATTRIB_HOOK_FLOAT( m_flSmackTime, mult_melee_smack_delay );

	//int iDeflect = 0;
	//CALL_ATTRIB_HOOK_INT( iDeflect, deflect_on_swing );
#ifdef GAME_DLL
	if ( m_bCanDeflect )
	{
		int iHitsPlayers = 0;
		CALL_ATTRIB_HOOK_INT( iHitsPlayers, disable_airblasting_players );
		Deflect( iHitsPlayers > 0, true );
	}

	float flBulletDeflectDuration = 0;
	CALL_ATTRIB_HOOK_FLOAT( flBulletDeflectDuration, bullet_deflect_duration_on_swing );
	if ( flBulletDeflectDuration > 0 )
	{
		pPlayer->m_Shared.AddCond( TF_COND_DEFLECT_BULLETS, flBulletDeflectDuration );
	}
#endif
}


void CTFWeaponBaseMelee::DoViewModelAnimation( bool bIsDeflect /*= false*/ )
{
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	if ( IsCurrentAttackACrit() )
		act = ACT_VM_SWINGHARD;

	if ( bIsDeflect )
		act = ACT_VM_PRIMARYATTACK_SPECIAL;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :  -
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPreFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		int iSelfMark = 0;
		CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
		CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death_alt );
		if ( iSelfMark )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}
	
	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}

#ifdef GAME_DLL
	if ( m_bCanDeflect )
	{
		float flGraceTime = tf2c_airblast_gracetime.GetFloat();
		if ( flGraceTime > 0.0f )
		{
			float flDelay = GetFireRate();//GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
			//CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_airblast_refire_time );
			if ( (m_flNextPrimaryAttack - gpGlobals->curtime) > (flDelay - flGraceTime) && m_flNextPrimaryAttack > gpGlobals->curtime )
			{
				// Repeat reflect checks.
				Deflect( true, false, false );
			}
		}
	}
#endif

	BaseClass::ItemPostFrame();
}


bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &trace, bool hitAlly /*= false*/ )
{
	float flBounds = 18.0f;
	CALL_ATTRIB_HOOK_FLOAT( flBounds, melee_bounds_multiplier );

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -flBounds, -flBounds, -flBounds );
	static Vector vecSwingMaxs( flBounds, flBounds, flBounds );

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->EyePosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

	// See if we hit anything.
	CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	CTraceFilterSimple filterSimple( pPlayer, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, hitAlly ? &filterSimple : &filter, &trace );

	if ( trace.fraction >= 1.0f )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, hitAlly ? &filterSimple : &filter, &trace );

		if ( trace.fraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = trace.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
			{
				// Why duck hull min/max?
				FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, hitAlly ? &filterSimple : &filter );
			}

			// This is the point on the actual surface (the hull could have hit space)
			vecSwingEnd = trace.endpos;
		}
	}

	return ( trace.fraction < 1.0f );
}

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	// We hit, setup the smack.
	trace_t trace;

	if ( DoSwingTrace( trace ) )
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

			CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );
			if ( flDamage >= 1.0f )
			{
				CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * tf_meleeattackforcescale.GetFloat() );
			}
			else
			{
				info.SetDamageForce( Vector( 0, 0, 1 ) );
				info.SetDamagePosition( vecSwingEnd );
			}
			trace.m_pEnt->DispatchTraceAttack( info, vecForward, &trace );
			ApplyMultiDamage();
		}
#endif

		OnEntityHit(trace.m_pEnt);

		// Don't impact trace friendly players or objects
		if (pPlayer->IsEnemy(trace.m_pEnt))
		{
			DoImpactEffect(trace, DMG_CLUB);

#ifdef GAME_DLL
			int iDeflect = 0;
			CALL_ATTRIB_HOOK_INT(iDeflect, deflect_on_smack_hit_player);
			if ( iDeflect > 0 && !trace.m_pEnt->IsBSPModel() )
			{
				Deflect(false, false);
			}
#endif
		}
	}
	else if ( DoSwingTrace( trace, true ) )
	{
		// No enemy hit. Found teammate to play impact sounds on:
		DoImpactEffect( trace, DMG_CLUB );
	}
	else
	{
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
	}

#ifdef GAME_DLL
	int iDeflect = 0;
	CALL_ATTRIB_HOOK_INT( iDeflect, deflect_on_smack );
	if ( iDeflect > 0 )
	{
		int iHitsPlayers = 0;
		CALL_ATTRIB_HOOK_INT( iHitsPlayers, disable_airblasting_players );
		Deflect( iHitsPlayers > 0, false);
	}
#endif

	FINISH_LAG_COMPENSATION();
}

#ifdef GAME_DLL
void CTFWeaponBaseMelee::Deflect( bool bProjectilesOnly /*= false*/, bool bDoAnimations /*= true*/, bool bHitGroundedProjectiles /*= false*/)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	Vector vecDir;
	QAngle angDir = pPlayer->EyeAngles();
	AngleVectors( angDir, &vecDir );

	Vector vecBlastSize = GetDeflectionSize();
	Vector vecOrigin = pPlayer->Weapon_ShootPosition() + (vecDir * Max( vecBlastSize.x, vecBlastSize.y ));

	CBaseEntity *pList[64];
	int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0 );

	if ( tf_debug_airblast.GetBool() )
	{
		NDebugOverlay::Box( vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0 );
	}

	bool bHitSomething = false;

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[i];
		if ( !pEntity || pEntity == pPlayer || !pEntity->IsDeflectable() )
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		if ( !pPlayer->FVisible( pEntity, MASK_SOLID ) )
			continue;

		CDisablePredictionFiltering disabler;

		if ( pEntity->IsPlayer() )
		{
			if ( bProjectilesOnly )
				continue;

			if ( !pEntity->IsAlive() )
				continue;

			Vector vecPushDir;
			QAngle angPushDir = angDir;
			float flPitch = AngleNormalize( angPushDir[PITCH] );

			CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
			if ( pTFPlayer->GetGroundEntity() )
			{
				// If they're on the ground, always push them at least 45 degrees up.
				angPushDir[PITCH] = Min( -45.0f, flPitch );
			}
			else if ( flPitch > -45.0f )
			{
				// Proportionally raise the pitch.
				float flScale = RemapValClamped( flPitch, 0.0f, 90.0f, 1.0f, 0.0f );
				angPushDir[PITCH] = Max( -45.0f, flPitch - 45.0f * flScale );
			}

			AngleVectors( angPushDir, &vecPushDir );
			DeflectPlayer( pTFPlayer, pPlayer, vecPushDir, false );
		}
		else if ( pPlayer->IsEnemy( pEntity ) )
		{
			bool bDont = false;
			CALL_ATTRIB_HOOK_INT(bDont, mod_melee_dont_deflect_projectiles);
			if (bDont)
				continue;

			// Don't deflect projectiles we just deflected multiple times.
			CTFBaseProjectile *pProjectile = static_cast<CTFBaseGrenade *>(pEntity);
			if ( pProjectile->GetDeflectedBy() == pPlayer )
				continue;

			if ( !bHitGroundedProjectiles )
			{
				// If we're not allowed to hit grounded projectiles, check if the pipe or sticky we hit is grounded
				CTFBaseGrenade *pGrenade = dynamic_cast<CTFBaseGrenade *>(pEntity);
				if ( pGrenade )
				{
					if ( pGrenade->GetGroundEntity() )
						continue;

					CTraceFilterWorldOnly traceFilter;
					trace_t tr;
					UTIL_TraceLine( pGrenade->GetAbsOrigin(), pGrenade->GetAbsOrigin() + Vector( 0, 0, -tf2c_groundedprojectiledistance.GetFloat() ), MASK_SHOT_HULL, &traceFilter, &tr );
					if ( tr.m_pEnt && tr.fraction < 1 )
						continue;
				}
			}

			// Deflect projectile to the point that we're aiming at, similar to rockets.
			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecDeflect;
			GetProjectileReflectSetup( pPlayer, vecPos, vecDeflect, false, (pEntity->GetDamageType() & DMG_USE_HITLOCATIONS) != 0 );

			DeflectEntity( pEntity, pPlayer, vecDeflect );

			bHitSomething = true;
		}
	}

	// Now do a swing or a deflect depending on if we hit something.
	if ( bDoAnimations )
		DoViewModelAnimation( bHitSomething );

	// Deny the melee hit if we've deflected something.
	if ( bHitSomething )
		m_flSmackTime = -1;
}

void CTFWeaponBaseMelee::GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates /* = true */, bool bUseHitboxes /* = false */ )
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

void CTFWeaponBaseMelee::DeflectEntity( CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir )
{
	pEntity->Deflected( pAttacker, vecDir );
	pEntity->EmitSound( "Weapon_Bat.Deflect" );
	DispatchParticleEffect( "deflect_fx", PATTACH_ABSORIGIN_FOLLOW, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether it extinguished friendly player
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::DeflectPlayer( CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir, bool bCanExtinguish /*= true*/ )
{
	if ( !pAttacker->IsEnemy( pVictim ) )
	{
		if ( bCanExtinguish && pVictim->m_Shared.InCond( TF_COND_BURNING ) )
		{
			// I feel bad actually typing out so many lines for mere damage block, but it's worth a try
			float flAfterburnBlocked = (pVictim->m_Shared.GetFlameRemoveTime() - gpGlobals->curtime) * tf2c_afterburn_damage.GetFloat();
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->m_Shared.GetBurnWeapon(), flAfterburnBlocked, mult_wpn_burndmg);
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flAfterburnBlocked, mult_burndmg_wearer);
			if (pVictim)
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveTFWeapon(), flAfterburnBlocked, mult_burndmg_active);
			if (flAfterburnBlocked < 1000.0f)	// safeguard against infinite or insane value
				CTF_GameStats.Event_PlayerBlockedDamage(pAttacker, flAfterburnBlocked);

			// Extinguish teammates.
			pVictim->m_Shared.RemoveCond( TF_COND_BURNING );
			pVictim->EmitSound( "TFPlayer.FlameOut" );

			IGameEvent* event = gameeventmanager->CreateEvent( "player_extinguished" );
			if ( event )
			{
				event->SetInt( "victim", pVictim->entindex() );
				event->SetInt( "healer", pAttacker->entindex() );

				gameeventmanager->FireEvent( event, true );
			}

			// Thank the Pyro player.
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_THANKS );

			CTF_GameStats.Event_PlayerAwardBonusPoints( pAttacker, pVictim, 1 );
			return true;
		}
	}
	else if ( tf2c_airblast_players.GetBool() )
	{
		// Don't push players if they're too far off to the side. Ignore Z.
		Vector2D vecVictimDir = pVictim->WorldSpaceCenter().AsVector2D() - pAttacker->WorldSpaceCenter().AsVector2D();
		Vector2DNormalize( vecVictimDir );

		Vector2D vecDir2D = vecDir.AsVector2D();
		Vector2DNormalize( vecDir2D );

		float flDot = DotProduct2D( vecDir2D, vecVictimDir );
		if ( flDot >= 0.5f )
		{
			// Push enemy players.
			float flKnockback = 500.0f;
			CALL_ATTRIB_HOOK_FLOAT(flKnockback, mult_airblast_pushback_scale);
			pVictim->m_Shared.AirblastPlayer( pAttacker, vecDir, flKnockback);
		//	pVictim->EmitSound( "TFPlayer.AirBlastImpact" );

			// Add pusher as recent damager so he can get a kill credit for pushing a player to his death.
			pVictim->AddDamagerToHistory( pAttacker, this );

			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:0,victim:1" );
		}
	}
	return false;
}

Vector CTFWeaponBaseMelee::GetDeflectionSize()
{
	float flMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMult, deflection_size_multiplier );
	//return flMult * Vector( 128.0f, 128.0f, 64.0f );
	return flMult * Vector( 128.0f, 128.0f, 100.0f );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage )
{
	float flDamage = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	if (IsCurrentAttackACrit())
	{
		int iModCustomDamage = 0;
		CALL_ATTRIB_HOOK_INT(iModCustomDamage, mod_melee_crit_swing_custom_damage);
		if (iModCustomDamage)
		{
			iCustomDamage = ETFDmgCustom(iModCustomDamage);
		}
	}

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetSwingRange( void )
{
	float flRange = 48.0f;
	CALL_ATTRIB_HOOK_FLOAT( flRange, melee_range_multiplier );
	if ( IsCurrentAttackACrit() )
		CALL_ATTRIB_HOOK_FLOAT( flRange, melee_range_multiplier_crit );
	return flRange;
}

#ifdef CLIENT_DLL

void CTFWeaponBaseMelee::SwitchBodyGroups( void )
{
	if ( m_iCritBodygroup >= 0 )
	{
		SetBodygroup( m_iCritBodygroup, (int)m_bLandedCrit );
	}
}


void CTFWeaponBaseMelee::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	m_bOldCritLandedState = m_bLandedCrit;
}


void CTFWeaponBaseMelee::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );
	if ( !GetPredictable() && m_bOldCritLandedState != m_bLandedCrit )
	{
		SwitchBodyGroups();
	}
}


CStudioHdr *CTFWeaponBaseMelee::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if ( m_iCritBodygroup < 0 )
	{
		int iMax = GetNumBodyGroups();
		for( int i = 0; i < iMax; i++ )
		{
			if ( !Q_strcasecmp( "crit", GetBodygroupName(i) ) )
			{
				m_iCritBodygroup = i;
				break;
			}
		}
	}

	return hdr;
}
#endif


bool CTFWeaponBaseMelee::ShouldSwitchCritBodyGroup( CBaseEntity *pEntity )
{
	return pEntity->IsPlayer();
}


void CTFWeaponBaseMelee::ResetCritBodyGroup( void )
{
	m_bLandedCrit = false;
#ifdef CLIENT_DLL
	m_bOldCritLandedState = false;

	SwitchBodyGroups();
#endif
}


void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity )
{
	if ( IsCurrentAttackACrit() && ShouldSwitchCritBodyGroup( pEntity ) )
	{
		m_bLandedCrit = true;

#ifdef CLIENT_DLL
		SwitchBodyGroups();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can this weapon fire a random crit?
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanFireRandomCrit( void )
{
	if ( !tf2c_vip_criticals.GetBool() && TFGameRules()->IsVIPMode() )
		return false;

	int nCvarValue = tf_weapon_criticals_melee.GetInt();

	if ( nCvarValue == 0 )
		return false;

	if ( nCvarValue == 1 && !tf_weapon_criticals.GetBool() )
		return false;

	return true;
}


float CTFWeaponBaseMelee::GetCritChance( void )
{
	return TF_DAMAGE_CRIT_CHANCE_MELEE;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether we have any entity in melee range 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CheckForgiveness(void)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return false;

#ifdef GAME_DLL
	Vector vCenter = pPlayer->EyePosition();
	float flRadius = GetSwingRange() * 1.2;
	CBaseEntity *pEntity = NULL;

	//NDebugOverlay::Sphere(vCenter, flRadius, 0, 255, 0, true, 5);
	
	for (CEntitySphereQuery sphere(vCenter, flRadius); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if (pEntity == pPlayer)
			continue;
		
		CTFPlayer *pAnotherPlayer = ToTFPlayer(pEntity);
		if (pAnotherPlayer && pAnotherPlayer->IsEnemy(pPlayer) && pAnotherPlayer->IsAlive() && !pAnotherPlayer->IsDormant())
		{		
			// Not sure if we need this check in such cheap function. Function for explosions radius attack does it.
			Vector vecHitPoint;
			pEntity->CollisionProp()->CalcNearestPoint(vCenter, &vecHitPoint);
			if (vecHitPoint.DistToSqr(vCenter) > Square(flRadius))
				continue;
			return true;
		}
	}
	return false;
#else
	return true;	//Don't punish players until server says so
#endif
}
