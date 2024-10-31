//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flamethrower.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "tf_lagcompensation.h"
#include "tf_weapon_compound_bow.h"

#if defined( CLIENT_DLL )
#include "c_tf_player.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "iefx.h"
#include "prediction.h"
#else
#include "explode.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "collisionutils.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "particle_parse.h"
#include "NextBotManager.h"
#include "tf_projectile_arrow.h"
#include "tf_generator.h"

ConVar tf_debug_flamethrower( "tf_debug_flamethrower", "0", FCVAR_CHEAT, "Visualize the flamethrower damage." );
ConVar tf_flamethrower_velocity( "tf_flamethrower_velocity", "2450.0", FCVAR_CHEAT, "Initial velocity of flame damage entities." );
ConVar tf_flamethrower_drag( "tf_flamethrower_drag", "0.89", FCVAR_CHEAT, "Air drag of flame damage entities." );
ConVar tf_flamethrower_float( "tf_flamethrower_float", "50.0", FCVAR_CHEAT, "Upward float velocity of flame damage entities." );
ConVar tf_flamethrower_flametime( "tf_flamethrower_flametime", "0.6", FCVAR_CHEAT, "Time to live of flame damage entities." );
ConVar tf_flamethrower_vecrand( "tf_flamethrower_vecrand", "0.05", FCVAR_CHEAT, "Random vector added to initial velocity of flame damage entities." );
ConVar tf_flamethrower_boxsize( "tf_flamethrower_boxsize", "12.0", FCVAR_CHEAT, "Size of flame damage entities." );
ConVar tf_flamethrower_maxdamagedist( "tf_flamethrower_maxdamagedist", "350.0", FCVAR_CHEAT, "Maximum damage distance for flamethrower." );
ConVar tf_flamethrower_shortrangedamagemultiplier( "tf_flamethrower_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT, "Damage multiplier for close-in flamethrower damage." );
ConVar tf_flamethrower_velocityfadestart( "tf_flamethrower_velocityfadestart", ".3", FCVAR_CHEAT, "Time at which attacker's velocity contribution starts to fade." );
ConVar tf_flamethrower_velocityfadeend( "tf_flamethrower_velocityfadeend", ".5", FCVAR_CHEAT, "Time at which attacker's velocity contribution finishes fading." );
//ConVar tf_flame_force( "tf_flame_force", "30" );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Position of end of muzzle relative to shoot position.
#define TF_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TF_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TF_FLAMETHROWER_MUZZLEPOS_UP			-12.0f

#ifdef CLIENT_DLL
#else
extern ConVar tf2c_afterburn_damage;
#endif

ConVar tf2c_airblast( "tf2c_airblast", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable/Disable the Airblast function of the Flamethrower." );
ConVar tf2c_airblast_players( "tf2c_airblast_players", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable/Disable the Airblast pushing players." );
#ifdef GAME_DLL
ConVar tf_debug_airblast( "tf_debug_airblast", "0", FCVAR_CHEAT, "Visualize airblast box." );
#endif
ConVar tf2c_extinguish_heal( "tf2c_extinguish_heal", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Extinguishing a teammate returns by default 20 health to the Pyro" );

ConVar tf2c_flamethrower_wallslide( "tf2c_flamethrower_wallslide", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Whether flame entities slide along hit walls." );
ConVar tf2c_airblast_gracetime( "tf2c_airblast_gracetime", "0.06", FCVAR_NOTIFY | FCVAR_REPLICATED, "How long Airblast reflect hitbox lingers" );

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameThrower, DT_WeaponFlameThrower )

BEGIN_NETWORK_TABLE( CTFFlameThrower, DT_WeaponFlameThrower )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritFire ) ),
	RecvPropFloat( RECVINFO( m_flAmmoUseRemainder ) ),
	RecvPropBool( RECVINFO( m_bHitTarget ) )
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritFire ) ),
	SendPropFloat( SENDINFO( m_flAmmoUseRemainder ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bHitTarget ) )
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFFlameThrower )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flAmmoUseRemainder, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.001f ),
	DEFINE_PRED_FIELD( m_flStartFiringTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flNextPrimaryAttackAnim, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlameThrower );
PRECACHE_WEAPON_REGISTER( tf_weapon_flamethrower );

// ------------------------------------------------------------------------------------------------ //
// CTFFlameThrower implementation.
// ------------------------------------------------------------------------------------------------ //
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFlameThrower::CTFFlameThrower()
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFlameEffect = NULL;
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pPilotLightSound = NULL;
	m_pHitTargetSound = NULL;
	m_pDynamicLight = NULL;
	m_bHasFlame = false;
#else
	m_bSimulatingFlames = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFFlameThrower::~CTFFlameThrower()
{
	DestroySounds();
}


void CTFFlameThrower::DestroySounds( void )
{
#if defined( CLIENT_DLL )
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pPilotLightSound )
	{
		controller.SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}

	if ( m_pHitTargetSound )
	{
		controller.SoundDestroy( m_pHitTargetSound );
		m_pHitTargetSound = NULL;
	}

	m_bHitTarget = false;
	m_bOldHitTarget = false;
#endif

}
void CTFFlameThrower::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;
	m_flStartFiringTime = 0.0f;
	m_flAmmoUseRemainder = 0.0f;

#ifdef GAME_DLL
	m_flStopHitSoundTime = 0.0f;
#else
	m_iParticleWaterLevel = -1;
#endif

	DestroySounds();
}


void CTFFlameThrower::Precache( void )
{
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	PrecacheScriptSound( "TFPlayer.FlameOut" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheScriptSound( "Weapon_FlameThrower.FireHit" );

	PrecacheTeamParticles( "flamethrower_%s" );
	PrecacheTeamParticles( "flamethrower_crit_%s" );

	PrecacheParticleSystem( "pyro_blast" );
	PrecacheParticleSystem( "deflect_fx" );
	PrecacheParticleSystem( "flamethrower_underwater" );

	// airblast_destroy_projectile
	PrecacheScriptSound( "Fire.Engulf" );
	PrecacheParticleSystem( "explosioncore_sapperdestroyed" );

	BaseClass::Precache();
}


bool CTFFlameThrower::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;

#if defined( CLIENT_DLL )
	StopFlame();
	StopPilotLight();
#endif

	return BaseClass::Holster( pSwitchingTo );
}


void CTFFlameThrower::ItemPostFrame()
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );
	bool bFired = false;

	if ( ( pOwner->m_nButtons & IN_ATTACK2 ) && m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		float flAmmoPerSecondaryAttack = (float)GetTFWpnData().GetWeaponData( TF_WEAPON_SECONDARY_MODE ).m_iAmmoPerShot;
		CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecondaryAttack, mult_airblast_cost );

		if ( iAmmo >= flAmmoPerSecondaryAttack )
		{
			SecondaryAttack();
			bFired = true;
		}
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && iAmmo > 0 && m_iWeaponState != FT_STATE_AIRBLASTING )
	{
		PrimaryAttack();
		bFired = true;
	}

	if ( !bFired )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			if ( m_iWeaponState < FT_STATE_AIRBLASTING )
			{
				SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
			}
			m_iWeaponState = FT_STATE_IDLE;
			m_bCritFire = false;
			m_bHitTarget = false;
		}

		if ( !ReloadOrSwitchWeapons() )
		{
			WeaponIdle();
		}
	}

#ifdef GAME_DLL
	float flGraceTime = tf2c_airblast_gracetime.GetFloat();
	if ( flGraceTime > 0.0f )
	{
		float flDelay = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
		CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_airblast_refire_time );
		if ( ( m_flNextSecondaryAttack - gpGlobals->curtime ) > ( flDelay - flGraceTime ) && m_flNextSecondaryAttack > gpGlobals->curtime )
		{
			// Repeat reflect checks.
			Deflect( true );
		}
	}
#endif

	//BaseClass::ItemPostFrame();
}


void CTFFlameThrower::ItemPreFrame( void )
{
#ifdef GAME_DLL
	SimulateFlames();
#endif

	BaseClass::ItemBusyFrame();
}


void CTFFlameThrower::ItemHolsterFrame( void )
{
#ifdef GAME_DLL
	SimulateFlames();
#endif

	BaseClass::ItemHolsterFrame();
}

class CTraceFilterIgnoreObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here.
	DECLARE_CLASS( CTraceFilterIgnoreObjects, CTraceFilterSimple );

	CTraceFilterIgnoreObjects( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity && pEntity->IsBaseObject() )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

};


void CTFFlameThrower::PrimaryAttack()
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined( CLIENT_DLL )
		StopFlame();
#endif
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	CalcIsAttackCritical();

	// Because the muzzle is so long, it can stick through a wall if the player is right up against it.
	// Make sure the weapon can't fire in this condition by tracing a line between the eye point and the end of the muzzle.
	trace_t trace;	
	Vector vecEye = pOwner->EyePosition();
	Vector vecMuzzlePos = GetVisualMuzzlePos();
	CTraceFilterIgnoreObjects traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecMuzzlePos, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction < 1.0f && trace.m_pEnt->m_takedamage == DAMAGE_NO )
	{
		// There is something between the eye and the end of the muzzle, most likely a wall, don't fire, and stop firing if we already are.
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
#if defined( CLIENT_DLL )
			StopFlame();
#endif
			m_iWeaponState = FT_STATE_IDLE;
			SendWeaponAnim( ACT_VM_IDLE );
		}
		return;
	}

	switch ( m_iWeaponState )
	{
		case FT_STATE_IDLE:
		case FT_STATE_AIRBLASTING:
		{
			// Just started, play PRE and start looping view model anim.
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			m_flStartFiringTime = gpGlobals->curtime + 0.16f; // 5 frames at 30 fps.

			m_iWeaponState = FT_STATE_STARTFIRING;
			break;
		}
		case FT_STATE_STARTFIRING:
		{
			// If some time has elapsed, start playing the looping third person anim.
			if ( gpGlobals->curtime > m_flStartFiringTime )
			{
				m_iWeaponState = FT_STATE_FIRING;
				m_flNextPrimaryAttackAnim = gpGlobals->curtime;
			}
			break;
		}
		case FT_STATE_FIRING:
		{
			if ( gpGlobals->curtime >= m_flNextPrimaryAttackAnim )
			{
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime + 1.4f; // Fewer than 45 frames!
			}
			break;
		}
	}

#ifdef CLIENT_DLL
	// Restart our particle effect if we've transitioned across water boundaries.
	if ( m_iParticleWaterLevel != -1 && pOwner->GetWaterLevel() != m_iParticleWaterLevel )
	{
		if ( m_iParticleWaterLevel == WL_Eyes || pOwner->GetWaterLevel() == WL_Eyes )
		{
			RestartParticleEffect();
		}
	}
#endif

#if !defined( CLIENT_DLL )
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired( this );
#endif

	// Don't attack if we're underwater.
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
#ifdef CLIENT_DLL
		bool bWasCritical = m_bCritFire;
#endif

		m_bCritFire = IsCurrentAttackACrit();

#ifdef CLIENT_DLL
		if ( bWasCritical != m_bCritFire )
		{
			RestartParticleEffect();
		}
#endif

#ifdef GAME_DLL
		// Create the flame.
		// NOTE: Flames are no longer entities since there's no need for them to be such
		// plus we want to simulate them from the weapon during player tick.
		// So I've changed them to be a simple child class of the flamethrower. (Nicknine)
		m_Flames[m_Flames.AddToTail()].Init( GetFlameOriginPos(), pOwner->EyeAngles(), pOwner, this );
#endif
	}

	// Figure how much ammo we're using per shot and add it to our remainder to subtract (We may be using less than 1.0 ammo units
	// per frame, depending on how constants are tuned, so keep an accumulator so we can expend fractional amounts of ammo per shot).
	float flAmmoPerSecond = (float)GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
	CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecond, mult_flame_ammopersec );

	m_flAmmoUseRemainder += flAmmoPerSecond * GetFireRate();

	// Take the integer portion of the ammo use accumulator and subtract it from player's ammo count; any fractional amount of ammo use
	// remains and will get used in the next shot.
	int iAmmoToSubtract = (int)m_flAmmoUseRemainder;
	if ( iAmmoToSubtract > 0 )
	{
		pOwner->RemoveAmmo( iAmmoToSubtract, m_iPrimaryAmmoType );
		m_flAmmoUseRemainder -= (float)iAmmoToSubtract;
		// Round to 2 digits of precision.
		//m_flAmmoUseRemainder = (float)( (int)( m_flAmmoUseRemainder * 100 ) ) / 100.0f;
	}

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback);
	if ( flSelfKnockback )
	{
		Vector vecForward;
		AngleVectors(pOwner->EyeAngles(), &vecForward);

		Vector vecForwardNormalized = vecForward.Normalized();
		pOwner->ApplyAbsVelocityImpulse(-vecForwardNormalized * flSelfKnockback);

#ifdef GAME_DLL
		pOwner->m_bSelfKnockback = true;
#endif
	}

	m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + GetFireRate();
}


void CTFFlameThrower::SecondaryAttack()
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;


	if ( !CanAttack() || !CanAirBlast())
	{
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

	m_bCurrentAttackIsCrit = false;

#ifdef CLIENT_DLL
	StopFlame();
#endif

	m_iWeaponState = FT_STATE_AIRBLASTING;
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	WeaponSound( WPN_DOUBLE );

#ifdef CLIENT_DLL
	if (prediction->IsFirstTimePredicted())
		StartFlame();
#else
	Deflect( false );
#endif

	float flSelfKnockback = 0;
	CALL_ATTRIB_HOOK_FLOAT(flSelfKnockback, apply_self_knockback_airblast);
	if ( flSelfKnockback )
	{
		Vector vecForward;
		AngleVectors(pOwner->EyeAngles(), &vecForward);

		Vector vecForwardNormalized = vecForward.Normalized();
		pOwner->ApplyAbsVelocityImpulse(-vecForwardNormalized * flSelfKnockback);

#ifdef GAME_DLL
		pOwner->m_bSelfKnockback = true;
#endif
	}

	float flAmmoPerSecondaryAttack = (float)GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
	CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecondaryAttack, mult_airblast_cost );

	pOwner->RemoveAmmo( flAmmoPerSecondaryAttack, m_iPrimaryAmmoType );

	// Don't allow firing immediately after airblasting.
	float flDelay = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_airblast_refire_time );

	// Faster airblast cooldown during Haste buff
	if ( pOwner->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flDelay *= 0.75f;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flDelay;
}

#ifdef GAME_DLL

void CTFFlameThrower::Deflect( bool bProjectilesOnly )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired( this );

	// Move other players back to history positions based on local player's lag.
	START_LAG_COMPENSATION( pOwner, pOwner->GetCurrentCommand() );

	Vector vecDir;
	QAngle angDir = pOwner->EyeAngles();
	AngleVectors( angDir, &vecDir );

	Vector vecBlastSize = GetDeflectionSize();
	Vector vecOrigin = pOwner->Weapon_ShootPosition() + ( vecDir * Max( vecBlastSize.x, vecBlastSize.y ) );

	CBaseEntity *pList[64];
	int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0 );

	if ( tf_debug_airblast.GetBool() )
	{
		NDebugOverlay::Box( vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0 );
	}

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[i];
		if ( !pEntity || pEntity == pOwner || !pEntity->IsDeflectable() )
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		if ( !pOwner->FVisible( pEntity, MASK_SOLID ) )
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
			DeflectPlayer( pTFPlayer, pOwner, vecPushDir );
		}
		else if ( pOwner->IsEnemy( pEntity ) )
		{
			CTFBaseProjectile *pProjectile = static_cast<CTFBaseProjectile *>( pEntity );
			if ( !pProjectile )
				continue;
			
			pProjectile->SetDeflectedBy( pOwner );

			// Don't deflect projectiles we just deflected multiple times.
			if ( bProjectilesOnly )
			{
				/*CTFBaseGrenade *pGrenade = static_cast<CTFBaseGrenade *>( pEntity );
				if ( pGrenade->GetDeflectedBy() == pOwner )
					continue;

				CTFBaseRocket *pRocket = static_cast<CTFBaseRocket *>( pEntity );
				if ( pRocket->GetDeflectedBy() == pOwner )
					continue;*/
				
				if ( pProjectile->GetDeflectedBy() == pOwner )
				{
					continue;
				}
			}

			// Deflect projectile to the point that we're aiming at, similar to rockets.
			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecDeflect;
			GetProjectileReflectSetup( pOwner, vecPos, vecDeflect, false, ( pEntity->GetDamageType() & DMG_USE_HITLOCATIONS ) != 0 );

			DeflectEntity( pEntity, pOwner, vecDeflect );
		}
	}

	FINISH_LAG_COMPENSATION();
}


void CTFFlameThrower::DeflectEntity( CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir )
{
	if ( !SupportsAirBlastFunction( AIRBLAST_CAN_DEFLECT_PROJECTILES ) )
		return;

	// Send network event before calling Deflected() so old owner is intact
	CTFPlayer* pOtherOwner = ToTFPlayer( pEntity->GetOwnerEntity() );
	if ( pOtherOwner && pAttacker )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "object_deflected", true );
		if ( event )
		{
			event->SetInt( "userid", pAttacker->GetUserID() );
			event->SetInt( "ownerid", pOtherOwner->GetUserID() );
			event->SetInt( "weaponid", GetWeaponID() );
			event->SetInt( "object_entindex", pEntity->entindex() );

			gameeventmanager->FireEvent( event );
		}
	}

	int iAirblastDestroyProjectile = 0;
	CALL_ATTRIB_HOOK_INT( iAirblastDestroyProjectile, airblast_destroy_projectile );
	if ( iAirblastDestroyProjectile )
	{
		pEntity->Deflected( pAttacker, vecDir );
		DispatchParticleEffect( "explosioncore_sapperdestroyed", pEntity->GetAbsOrigin(), vec3_angle, pAttacker );
		pAttacker->StopSound( "Fire.Engulf" );
		pAttacker->EmitSound( "Fire.Engulf" );

		pEntity->SetTouch( NULL );
		pEntity->AddEffects( EF_NODRAW );
		pEntity->SetThink( &BaseClass::SUB_Remove );
		pEntity->SetNextThink( gpGlobals->curtime );

		return;
	}

	pEntity->Deflected( pAttacker, vecDir );
	pAttacker->StopSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	pAttacker->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	DispatchParticleEffect( "deflect_fx", PATTACH_ABSORIGIN_FOLLOW, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether it extinguished friendly player
//-----------------------------------------------------------------------------
bool CTFFlameThrower::DeflectPlayer( CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir, bool bCanExtinguish /*= true*/ )
{
	if ( !pAttacker->IsEnemy( pVictim ) )
	{
		if ( bCanExtinguish && pVictim->m_Shared.InCond(TF_COND_BURNING) && SupportsAirBlastFunction(AIRBLAST_CAN_EXTINGUISH_TEAMMATES))
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

			// Heal the Pyro (introduced in Tough Break).
			// Custom weapon attribute overrides convar setting.
			int iExtinguishHeal = tf2c_extinguish_heal.GetBool() ? 20 : 0;
			int iExtinguishHealFromAttribute = 0;
			CALL_ATTRIB_HOOK_INT( iExtinguishHealFromAttribute, extinguish_restores_health );
			if ( iExtinguishHealFromAttribute > 0 )
			{
				pAttacker->TakeHealth( iExtinguishHealFromAttribute, HEAL_NOTIFY );
			}
			else if ( iExtinguishHeal > 0 )
			{
				pAttacker->TakeHealth( iExtinguishHeal, HEAL_NOTIFY );
			}

			// Thank the Pyro player.
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_THANKS );

			CTF_GameStats.Event_PlayerAwardBonusPoints( pAttacker, pVictim, 1 );
			return true;
		}
	}
	else if ( tf2c_airblast_players.GetBool() && SupportsAirBlastFunction( AIRBLAST_CAN_PUSH_ENEMIES ) )
	{
		// Don't push players if they're too far off to the side. Ignore Z.
		Vector2D vecVictimDir = pVictim->WorldSpaceCenter().AsVector2D() - pAttacker->WorldSpaceCenter().AsVector2D();
		Vector2DNormalize( vecVictimDir );

		Vector2D vecDir2D = vecDir.AsVector2D();
		Vector2DNormalize( vecDir2D );

		float flDot = DotProduct2D( vecDir2D, vecVictimDir );
		if ( flDot >= 0.8f )
		{
			// Push enemy players.
			pVictim->m_Shared.AirblastPlayer( pAttacker, vecDir, 500 );
			pVictim->EmitSound( "TFPlayer.AirBlastImpact" );

			// Add pusher as recent damager so he can get a kill credit for pushing a player to his death.
			pVictim->AddDamagerToHistory( pAttacker, this );

			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:0,victim:1" );

			// Send network event
			IGameEvent * event = gameeventmanager->CreateEvent( "object_deflected", true );
			if ( event )
			{
				event->SetInt( "userid", pAttacker->GetUserID() );
				event->SetInt( "ownerid", pVictim->GetUserID() );
				event->SetInt( "weaponid", 0 ); // 0 means the player in ownerid was pushed
				event->SetInt( "object_entindex", pVictim->entindex() );

				gameeventmanager->FireEvent( event );
			}
		}
	}
	return false;
}
#endif


bool CTFFlameThrower::Lower( void )
{
	if ( !BaseClass::Lower() )
		return false;

	// If we were firing, stop.
	if ( m_iWeaponState > FT_STATE_IDLE )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		m_iWeaponState = FT_STATE_IDLE;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle at it appears visually.
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities.
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle.
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetMuzzlePosHelper( bool bVisualPos )
{
	Vector vecMuzzlePos;

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner ) 
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );
		vecMuzzlePos = pOwner->Weapon_ShootPosition();
		vecMuzzlePos +=  vecRight * TF_FLAMETHROWER_MUZZLEPOS_RIGHT;
		// If asking for visual position of muzzle, include the forward component.
		if ( bVisualPos )
		{
			vecMuzzlePos += vecForward * TF_FLAMETHROWER_MUZZLEPOS_FORWARD;
		}
	}

	return vecMuzzlePos;
}

//-----------------------------------------------------------------------------
// Purpose: Return the size of the airblast detection box.
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetDeflectionSize()
{
	float flMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMult, deflection_size_multiplier );
	return flMult * Vector( 128.0f, 128.0f, 64.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Can this flamethrower airblast in any capacity at all?
//-----------------------------------------------------------------------------
bool CTFFlameThrower::CanAirBlast() const
{
	if ( !tf2c_airblast.GetBool() )
		return false;

	int iNoAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iNoAirblast, set_flamethrower_push_disabled );
	if ( iNoAirblast )
		return false;

	iNoAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iNoAirblast, airblast_disabled );
	if ( iNoAirblast )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Does this flamethrower support the given airblast function bit?
//-----------------------------------------------------------------------------
bool CTFFlameThrower::SupportsAirBlastFunction( EFlameThrowerAirblastFunction nFunc ) const
{
	int iAttrFlags = 0;
	CALL_ATTRIB_HOOK_INT( iAttrFlags, airblast_functionality_flags );

	// Return true if ANY requested bit is set.
	if ( iAttrFlags != 0 )
		return ( iAttrFlags & nFunc ) != 0;
	
	// Live TF2 does it this way: if a FT has zero flags whatsoever, then it
	// just returns true for everything if airblast is supported at all.
	return CanAirBlast();
}

#if defined( CLIENT_DLL )

bool CTFFlameThrower::Deploy( void )
{
	StartPilotLight();

	return BaseClass::Deploy();
}


void CTFFlameThrower::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( IsCarrierAlive() && WeaponState() == WEAPON_IS_ACTIVE && GetPlayerOwner()->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			if ( m_iWeaponState != FT_STATE_AIRBLASTING || !GetPredictable() )
			{
				StartFlame();
			}
		}
		else
		{
			StartPilotLight();
		}		
	}
	else 
	{
		StopFlame();
		StopPilotLight();
	}
}


void CTFFlameThrower::UpdateOnRemove( void )
{
	StopFlame();
	StopPilotLight();

	BaseClass::UpdateOnRemove();
}


void CTFFlameThrower::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		if ( !IsDormant() && bDormant )
		{
			StopFlame();
			StopPilotLight();
		}
	}

	// Deliberately skip base combat weapon to avoid being holstered.
	C_BaseEntity::SetDormant( bDormant );
}

void CTFFlameThrower::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	if ( m_pFlameEffect )
	{
		RestartParticleEffect();
	}
}


void CTFFlameThrower::StartFlame()
{
	if ( m_iWeaponState == FT_STATE_AIRBLASTING )
	{
		DispatchParticleEffect( "pyro_blast", PATTACH_POINT_FOLLOW, GetWeaponForEffect(), GetTracerAttachment() );

		/*CLocalPlayerFilter filter;
		EmitSound( filter, entindex(), "Weapon_FlameThrower.AirBurstAttack" );*/
	}
	else
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		// Normally, crossfade between start sound & firing loop in 3.5 sec.
		float flCrossfadeTime = 3.5;

		if ( m_pFiringLoop && m_bCritFire != m_bFiringLoopCritical )
		{
			// If we're firing and changing between critical & noncritical, just need to change the firing loop.
			// Set crossfade time to zero so we skip the start sound and go to the loop immediately.
			flCrossfadeTime = 0.0f;
			StopFlame( true );
		}

		StopPilotLight();

		if (!m_bHasFlame) 
		{
			RestartParticleEffect();
			m_bHasFlame = true;
		}

		if ( !m_pFiringStartSound && !m_pFiringLoop )
		{
			CLocalPlayerFilter filter;

			// Play the fire start sound.
			const char *shootsound = GetShootSound( SINGLE );
			if ( flCrossfadeTime > 0.0f )
			{
				// Play the firing start sound and fade it out.
				m_pFiringStartSound = controller.SoundCreate( filter, entindex(), shootsound );
				controller.Play( m_pFiringStartSound, 1.0f, 100 );
				controller.SoundChangeVolume( m_pFiringStartSound, 0.0f, flCrossfadeTime );
			}

			// Start the fire sound loop and fade it in.
			if ( m_bCritFire )
			{
				shootsound = GetShootSound( BURST );
			}
			else
			{
				shootsound = GetShootSound( RELOAD ); // HACK! special1 is used for projectile explosion :(
			}
			m_pFiringLoop = controller.SoundCreate( filter, entindex(), shootsound );
			m_bFiringLoopCritical = m_bCritFire;

			// Play the firing loop sound and fade it in.
			if ( flCrossfadeTime > 0.0f )
			{
				controller.Play( m_pFiringLoop, 0.0f, 100 );
				controller.SoundChangeVolume( m_pFiringLoop, 1.0f, flCrossfadeTime );
			}
			else
			{
				controller.Play( m_pFiringLoop, 1.0f, 100 );
			}
		}

		if ( m_bHitTarget != m_bOldHitTarget )
		{
			if ( m_bHitTarget )
			{
				CLocalPlayerFilter filter;
				m_pHitTargetSound = controller.SoundCreate( filter, entindex(), "Weapon_FlameThrower.FireHit" );
				controller.Play( m_pHitTargetSound, 1.0f, 100.0f );
			}
			else if ( m_pHitTargetSound )
			{
				controller.SoundDestroy( m_pHitTargetSound );
				m_pHitTargetSound = NULL;
			}

			m_bOldHitTarget = m_bHitTarget;
		}
	}
}


void CTFFlameThrower::StopFlame( bool bAbrupt /*= false*/ )
{
	m_bHasFlame = false;
	if ( ( m_pFiringLoop || m_pFiringStartSound ) && !bAbrupt )
	{
		// Play a quick wind-down poof when the flame stops.
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( SPECIAL3 );
		EmitSound( filter, entindex(), shootsound );
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_pFlameEffect )
	{
		if ( m_hFlameEffectHost.Get() )
		{
			m_hFlameEffectHost->ParticleProp()->StopEmission( m_pFlameEffect );
			m_hFlameEffectHost = NULL;
		}

		m_pFlameEffect = NULL;
	}

	if ( !bAbrupt )
	{
		if ( m_pHitTargetSound )
		{
			controller.SoundDestroy( m_pHitTargetSound );
			m_pHitTargetSound = NULL;
		}

		m_bOldHitTarget = false;
		m_bHitTarget = false;
	}

	if ( m_pDynamicLight && m_pDynamicLight->key == LIGHT_INDEX_MUZZLEFLASH + entindex() )
	{
		m_pDynamicLight->die = gpGlobals->curtime;
		m_pDynamicLight = NULL;
	}

	m_iParticleWaterLevel = -1;
}


void CTFFlameThrower::StartPilotLight()
{
	if ( !m_pPilotLightSound )
	{
		StopFlame();

		// Create the looping pilot light sound.
		const char *pilotlightsound = GetShootSound( SPECIAL2 );
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}	
}


void CTFFlameThrower::StopPilotLight()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}


void CTFFlameThrower::RestartParticleEffect( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_pFlameEffect && m_hFlameEffectHost.Get() )
	{
		m_hFlameEffectHost->ParticleProp()->StopEmission( m_pFlameEffect );
	}

	m_iParticleWaterLevel = pOwner->GetWaterLevel();

	// Start the appropriate particle effect.
	const char *pszParticleEffect;
	if ( pOwner->GetWaterLevel() == WL_Eyes )
	{
		pszParticleEffect = "flamethrower_underwater";
	}
	else if ( m_bCritFire )
	{
		pszParticleEffect = GetBeamParticleName("flamethrower_crit_%s", m_bCritFire);
	}
	else
	{
		pszParticleEffect = GetBeamParticleName("flamethrower_%s");
	}

	int iMuzzleAttachment = GetTracerAttachment();

	// Start the effect on the viewmodel if our owner is the local player.
	C_BaseEntity *pModel = GetWeaponForEffect();
	m_pFlameEffect = pModel->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, iMuzzleAttachment );
	pModel->ParticleProp()->AddControlPoint( m_pFlameEffect, 2, pOwner, PATTACH_ABSORIGIN_FOLLOW );
	m_hFlameEffectHost = pModel;
}

#else
//-----------------------------------------------------------------------------
// Purpose: Notify client that we're hitting an enemy.
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetHitTarget( void )
{
	if ( m_iWeaponState > FT_STATE_IDLE )
	{
		if ( !m_bHitTarget )
			m_bHitTarget = true;

		m_flStopHitSoundTime = gpGlobals->curtime + 0.2f;
		SetContextThink( &CTFFlameThrower::HitTargetThink, gpGlobals->curtime + 0.1f, "FlameThrowerHitTargetThink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notify client that we're hitting an enemy.
//-----------------------------------------------------------------------------
void CTFFlameThrower::HitTargetThink( void )
{
	if ( m_flStopHitSoundTime != 0.0f && m_flStopHitSoundTime > gpGlobals->curtime )
	{
		m_bHitTarget = false;
		m_flStopHitSoundTime = 0.0f;
		SetContextThink( NULL, 0, "FlameThrowerHitTargetThink" );
		return;
	}
	
	SetContextThink( &CTFFlameThrower::HitTargetThink, gpGlobals->curtime + 0.1f, "FlameThrowerHitTargetThink" );
}


void CTFFlameThrower::SimulateFlames( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Is this the best way? Have to do this since flames 
	// aren't real entities, but it seems fine for now though.
	if ( TFGameRules()->InRoundRestart() )
	{
		m_Flames.RemoveAll();
	}

	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	// Simulate all flames.
	m_bSimulatingFlames = true;
	for ( int i = m_Flames.Count() - 1; i >= 0; i-- )
	{
		if ( !m_Flames[i].FlameThink() )
		{
			m_Flames.FastRemove( i );
		}
	}
	m_bSimulatingFlames = false;

	FINISH_LAG_COMPENSATION();
}
#endif

#ifdef GAME_DLL

void CTFFlameEntity::Init( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTFFlameThrower *pWeapon )
{
	m_vecOrigin = m_vecPrevPos = m_vecInitialPos = vecOrigin;
	m_hOwner = pOwner;
	m_pOuter = pWeapon;

	float flBoxSize = tf_flamethrower_boxsize.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flBoxSize, mult_flame_size );
	m_vecMins.Init( -flBoxSize, -flBoxSize, -flBoxSize );
	m_vecMaxs.Init( flBoxSize, flBoxSize, flBoxSize );

	// Set team.
	m_iTeamNum = pOwner->GetTeamNumber();
	m_iDmgType = pWeapon->GetDamageType();
	if ( pWeapon->IsCurrentAttackACrit() )
	{
		m_iDmgType |= DMG_CRITICAL;
	}
	m_flDmgAmount = pWeapon->GetProjectileDamage() * pWeapon->GetFireRate();

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float velocity = tf_flamethrower_velocity.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, velocity, mult_flame_velocity);
	m_vecBaseVelocity = vecForward * velocity;
	m_vecBaseVelocity += RandomVector( -velocity * tf_flamethrower_vecrand.GetFloat(), velocity * tf_flamethrower_vecrand.GetFloat() );
	m_vecAttackerVelocity = pOwner->GetAbsVelocity();
	m_vecVelocity = m_vecBaseVelocity;

	m_flTimeRemove = gpGlobals->curtime + ( tf_flamethrower_flametime.GetFloat() * random->RandomFloat( 0.9, 1.1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
bool CTFFlameEntity::FlameThink( void )
{
	// Remove it if it's expired.
	if ( gpGlobals->curtime >= m_flTimeRemove )
		return false;

	// If the player changes team, remove all flames immediately to prevent griefing.
	if ( !m_hOwner || m_hOwner->GetTeamNumber() != m_iTeamNum )
		return false;

	// Do collision detection. We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server.
	if ( m_vecOrigin != m_vecPrevPos )
	{
		// TF2C: Do world collision detection to enable flames to slide along surfaces
		// Fixes cases of damage boxes giving up too soon and not lining up with particles.
		if ( tf2c_flamethrower_wallslide.GetBool() )
		{
			trace_t traceWorld;
			CTraceFilterWorldAndPropsOnly traceFilter;
			UTIL_TraceLine( m_vecPrevPos, m_vecOrigin, MASK_SHOT, &traceFilter, &traceWorld );

			if ( traceWorld.DidHit() )
			{
				// Move back out first.
				m_vecOrigin = m_vecPrevPos;

				// Change direction to slide along surface.
				m_vecBaseVelocity = m_vecBaseVelocity - ( DotProduct( m_vecBaseVelocity, traceWorld.plane.normal ) * traceWorld.plane.normal );

				if ( tf_debug_flamethrower.GetInt() )
				{
					NDebugOverlay::Cross3D( traceWorld.endpos, m_vecMins, m_vecMaxs, 255, 255, 0, false, 0.2f );
				}
			}
		}

		/*CTFTeam *pTeam = pAttacker->GetOpposingTFTeam();
		if ( !pTeam )
			return;*/
	
		bool bHitWorld = false;

		// HACK: Changed the first argument to '0' from 'm_iTeamNum', but it allows the Huntsman's arrow to light up.
		ForEachEnemyTFTeam( 0, [&]( int iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				return true;

			// check collision against all enemy players
			for ( int iPlayer = 0, iPlayers = pTeam->GetNumPlayers(); iPlayer < iPlayers; iPlayer++ )
			{
				// Is this player connected, alive, and an enemy?
				CBasePlayer *pPlayer = pTeam->GetPlayer( iPlayer );
				if ( pPlayer && pPlayer != m_hOwner && pPlayer->IsConnected() && pPlayer->IsAlive() )
				{
					CheckCollision( pPlayer, &bHitWorld );
					if ( bHitWorld )
						return false;
				}
			}

			// Check collision against all enemy objects.
			for ( int iObject = 0, iObjects = pTeam->GetNumObjects(); iObject < iObjects; iObject++ )
			{
				CBaseObject *pObject = pTeam->GetObject( iObject );
				if ( pObject )
				{
					CheckCollision( pObject, &bHitWorld );
					if ( bHitWorld )
						return false;
				}
			}

			return true;
		} );

		for ( CTFProjectile_Arrow *pEntity : TAutoList<CTFProjectile_Arrow>::AutoList() )
			CheckCollision( pEntity, &bHitWorld );

#ifdef TF_GENERATOR
		// Check collision on all team_shield entities
		for ( CTeamShield *pEntity : CTeamShield::AutoList() )
			CheckCollision(pEntity, &bHitWorld);
		// Check collision on all team_generator entities
		for ( CTeamGenerator *pEntity : CTeamGenerator::AutoList() )
			CheckCollision(pEntity, &bHitWorld);
#endif

		if ( bHitWorld )
			return false;

		// Check collision against all enemy non-player NextBots.
		CUtlVector<INextBot *> nextbots;
		TheNextBots().CollectAllBots( &nextbots );

		for ( auto nextbot : nextbots )
		{
			CBaseCombatCharacter *pBot = nextbot->GetEntity();
			if ( pBot != nullptr && !pBot->IsPlayer() && pBot->IsAlive() )
			{
				CheckCollision( pBot, &bHitWorld );
				if ( bHitWorld )
					return false;
			}
		}
	}

	// Calculate how long the flame has been alive for.
	float flFlameElapsedTime = tf_flamethrower_flametime.GetFloat() - ( m_flTimeRemove - gpGlobals->curtime );
	// Calculate how much of the attacker's velocity to blend in to the flame's velocity. The flame gets the attacker's velocity
	// added right when the flame is fired, but that velocity addition fades quickly to zero.
	float flAttackerVelocityBlend = RemapValClamped( flFlameElapsedTime, tf_flamethrower_velocityfadestart.GetFloat(), 
		tf_flamethrower_velocityfadeend.GetFloat(), 1.0f, 0 );

	// Reduce our base velocity by the air drag constant.
	m_vecBaseVelocity *= tf_flamethrower_drag.GetFloat();

	// Add our float upward velocity.
	m_vecVelocity = m_vecBaseVelocity + Vector( 0, 0, tf_flamethrower_float.GetFloat() ) + ( flAttackerVelocityBlend * m_vecAttackerVelocity );

	// Render debug visualization if convar on
	if ( tf_debug_flamethrower.GetInt() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = (int)( gpGlobals->curtime * 10 ) % 255;
			NDebugOverlay::Box( m_vecOrigin, m_vecMins, m_vecMaxs, val, 255, val, 0, 0 );
			//NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		} 
		else 
		{
			NDebugOverlay::Box( m_vecOrigin, m_vecMins, m_vecMaxs, 0, 100, 255, 0, 0 );
			//NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	//SetNextThink( gpGlobals->curtime );

	// Move!
	m_vecPrevPos = m_vecOrigin;
	m_vecOrigin += m_vecVelocity * gpGlobals->frametime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities.
//-----------------------------------------------------------------------------
void CTFFlameEntity::CheckCollision( CBaseEntity *pOther, bool *pbHitWorld )
{
	*pbHitWorld = false;

	// If we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity.
	int iIndex = m_hEntitiesBurnt.Find( pOther );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;

	// Do a bounding box check against the entity.
	Vector vecMins, vecMaxs;
	pOther->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
	CBaseTrace trace;
	Ray_t ray;
	float flFractionLeftSolid;				
	ray.Init( m_vecPrevPos, m_vecOrigin, m_vecMins, m_vecMaxs );
	if ( IntersectRayWithBox( ray, vecMins, vecMaxs, 0.0, &trace, &flFractionLeftSolid ) )
	{
		// If bounding box check passes, check player hitboxes.
		trace_t trHitbox;
		trace_t trWorld;
		trace_t trWorld2;
		trace_t trWorld3;
		trace_t trWorld4;

		Vector vecMinsTarget, vecMaxsTarget;
		pOther->GetCollideable()->WorldSpaceSurroundingBounds( &vecMinsTarget, &vecMaxsTarget);

		bool bTested = pOther->GetCollideable()->TestHitboxes( ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox );
		if ( !bTested || !trHitbox.DidHit() )
			return;

		// Now, let's see if the flame visual could have actually hit this player. Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything. Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine( m_vecInitialPos, m_vecOrigin + vDir * m_vecMins.x, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld );
		UTIL_TraceLine( m_vecInitialPos, m_vecOrigin - vDir * m_vecMins.x, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld2 );
		UTIL_TraceLine( vecMinsTarget, m_vecOrigin, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld3);
		UTIL_TraceLine( vecMaxsTarget, m_vecOrigin, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld4);

		
		if ( tf_debug_flamethrower.GetInt() )
		{
			NDebugOverlay::Line( trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f );
			NDebugOverlay::Line(trWorld2.startpos, trWorld2.endpos, 255, 0, 0, true, 3.0f);
			NDebugOverlay::Line(trWorld3.startpos, trWorld3.endpos, 0, 0, 255, true, 3.0f);
			NDebugOverlay::Line(trWorld4.startpos, trWorld4.endpos, 0, 0, 255, true, 3.0f);
		}
		
		if ( trWorld.fraction == 1.0f && trWorld2.fraction == 1.0f && (trWorld3.fraction == 1.0f || trWorld4.fraction == 1.0f))
		{						
			// If there is nothing solid in the way, damage the entity.
			if ( pOther->IsPlayer() && pOther->InSameTeam( m_hOwner ) )
			{
				OnCollideWithTeammate( ToTFPlayer( pOther ) );
			}
			else
			{
				OnCollide( pOther );
			}
		}					
		else
		{
			// We hit the world, remove ourselves.
			*pbHitWorld = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity.
//-----------------------------------------------------------------------------
void CTFFlameEntity::OnCollide( CBaseEntity *pOther )
{
	int nContents = UTIL_PointContents( m_vecOrigin );
	if ( ( nContents & MASK_WATER ) )
	{
		m_flTimeRemove = gpGlobals->curtime;
		return;
	}

	if ( pOther->IsBaseObject() && pOther->InSameTeam( m_hOwner ) )
		return;

	// Remember that we've burnt this entity
	m_hEntitiesBurnt.AddToTail( pOther );

	if (V_strcmp(pOther->GetClassname(), "tf_projectile_arrow") == 0)
	{
		// Light any arrows that fly by.
		CTFProjectile_Arrow* pArrow = assert_cast<CTFProjectile_Arrow*>(pOther);
		pArrow->SetArrowAlight(true);
		return;
	}

	float flDistance = m_vecOrigin.DistTo( m_vecInitialPos );
	float flMultiplier;
	if ( flDistance <= 125 )
	{
		// At very short range, apply short range damage multiplier.
		flMultiplier = tf_flamethrower_shortrangedamagemultiplier.GetFloat();
	}
	else
	{
		// Make damage ramp down from 100% to 60% from half the max dist to the max dist.
		flMultiplier = RemapValClamped( flDistance, tf_flamethrower_maxdamagedist.GetFloat() / 2, tf_flamethrower_maxdamagedist.GetFloat(), 1.0f, 0.6f );
	}

	float flDamage = m_flDmgAmount * flMultiplier;
	flDamage = Max( flDamage, 1.0f );
	if ( tf_debug_flamethrower.GetInt() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	m_pOuter->SetHitTarget();

	CTakeDamageInfo info( m_hOwner, m_hOwner, m_pOuter, Vector( 0, 0, 0 ), m_hOwner->EyePosition(), flDamage, m_iDmgType, TF_DMG_CUSTOM_BURNING );

	// We collided with pOther, so try to find a place on their surface to show blood.
	trace_t trace;
	UTIL_TraceLine( m_vecOrigin, pOther->WorldSpaceCenter(), MASK_SOLID | CONTENTS_HITBOX, NULL, COLLISION_GROUP_NONE, &trace );

	pOther->DispatchTraceAttack( info, m_vecVelocity, &trace );
	ApplyMultiDamage();
}


void CTFFlameEntity::OnCollideWithTeammate( CTFPlayer *pPlayer )
{
	// Only care about Snipers
	if ( !pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
		return;

	int iIndex = m_hEntitiesBurnt.Find( pPlayer );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;

	m_hEntitiesBurnt.AddToTail( pPlayer );

	// Does he have the bow?
	CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
	if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
	{
		static_cast<CTFCompoundBow *>( pActiveWeapon )->SetArrowAlight( true );
	}
}
#endif // GAME_DLL
