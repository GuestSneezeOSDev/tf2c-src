//====== Copyright � 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Bouncy rail projectile used by the Coilgun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_coil.h"
#include "IEffects.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "decals.h"
#endif

#include "achievements_tf.h"

#define TF_WEAPON_COIL_MODEL "models/weapons/w_models/w_coilgun_projectile.mdl"
#define TF_COILGUN_BOUNCE_PENALTY 15.0f

BEGIN_DATADESC( CTFProjectile_Coil )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_coil, CTFProjectile_Coil );
PRECACHE_REGISTER( tf_projectile_coil );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Coil, DT_TFProjectile_Coil )
BEGIN_NETWORK_TABLE( CTFProjectile_Coil, DT_TFProjectile_Coil )
END_NETWORK_TABLE()

ConVar tf2c_coilgun_min_bounce_life("tf2c_coilgun_min_bounce_life", "0.05", FCVAR_REPLICATED);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Coil::CTFProjectile_Coil()
{
	m_iNumBounces = 0;
	m_bBounced = false;
	m_flMinBounceLifeTime = 0.0f;

#ifdef CLIENT_DLL
	m_pEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Coil::~CTFProjectile_Coil()
{
#ifdef CLIENT_DLL
	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
		m_pEffect = NULL;
	}
#endif
}

#ifdef GAME_DLL

void CTFProjectile_Coil::Precache()
{
	PrecacheModel( TF_WEAPON_COIL_MODEL );

	PrecacheTeamParticles( "coilgun_trail_%s" );
	PrecacheTeamParticles( "coilgun_trail_crit_%s" );
	PrecacheParticleSystem( "coilgun_destroyed" );

	PrecacheScriptSound( "Weapon_CoilGun.Ricochet" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Coil::Spawn()
{
	SetModel( TF_WEAPON_COIL_MODEL );
	m_iNumBounces = 0;
	m_bBounced = false;
	m_flMinBounceLifeTime = 0.0f;

	BaseClass::Spawn();
}


void CTFProjectile_Coil::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other".
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t trHit;
	trHit = CBaseEntity::GetTouchTrace();
	if ( trHit.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Stop on enemy players, enemy buildings, and entities able to take damage (e.g. explosive pumpkins)
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer || pOther->IsBaseObject() || pOther->m_takedamage != DAMAGE_NO )
	{
		return BaseClass::RocketTouch( pOther );
	}

	// Bounce off surfaces.
	if ( m_iNumBounces > 0 || gpGlobals->curtime < m_flMinBounceLifeTime)
	{
		EmitSound( "Weapon_CoilGun.Ricochet" );
		UTIL_ImpactTrace( &trHit, DMG_BULLET );

		if (gpGlobals->curtime >= m_flMinBounceLifeTime)
		{
			m_iNumBounces--;
			m_flMinBounceLifeTime = gpGlobals->curtime + tf2c_coilgun_min_bounce_life.GetFloat();

			// Lower damage per bounce, but not below a minimum value
			// (was able to heal enemies when custom damage penalty set).
			float iBouncePenalty = TF_COILGUN_BOUNCE_PENALTY;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOriginalLauncher(), iBouncePenalty, mod_coil_bounce_damage_penalty );
			m_flDamage -= iBouncePenalty;
			if (m_flDamage < 0.0f)
			{
				m_flDamage = 0.0f;
			}
		}

		Vector vecDir = GetAbsVelocity();
		float flDot = DotProduct( vecDir, trHit.plane.normal );
		Vector vecReflect = vecDir - 2.0f * flDot * trHit.plane.normal;
		SetAbsVelocity( vecReflect );

		VectorNormalize( vecReflect );

		QAngle vecAngles;
		VectorAngles( vecReflect, vecAngles );
		SetAbsAngles( vecAngles );

		//float flAngle = RAD2DEG( acos( vecReflect.Dot( trHit.plane.normal ) ) );
		//CFmtStr str;
		//NDebugOverlay::EntityTextAtPosition( GetAbsOrigin(), 0, str.sprintf("Angle %2.2f", flAngle), 3.0f );

		if ( !m_bBounced )
		{
			m_bBounced = true;
		}
			
		return;
	}

	return BaseClass::RocketTouch( pOther );
}


void CTFProjectile_Coil::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	// AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;
	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( vec3_origin );

	// Pull out a bit.
	if ( pTrace->fraction != 1.0f )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
		EmitSound( "Flesh.BulletImpact" );
	}
	else if ( pOther->IsBaseObject() )
	{
		EmitSound( "SolidMetal.BulletImpact" );
	}

	// don't decal your teammates or objects on your team
	if ( pOther->GetTeamNumber() != GetTeamNumber() )
	{
		UTIL_ImpactTrace( pTrace, DMG_BULLET );
	}
	
	DispatchParticleEffect( "coilgun_destroyed", GetAbsOrigin(), vec3_angle );

	Vector vecDir = GetAbsVelocity();
	VectorNormalize( vecDir );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();

	Vector damageForce = GetAbsVelocity();
	VectorNormalize( damageForce );
	damageForce *= GetDamage();

	CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), damageForce, GetAbsOrigin(), GetDamage(), GetDamageType() );
	info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
	pOther->TakeDamage( info );

	// TF2C_ACHIEVEMENT_KILL_WITH_BLINDCOILRICOCHET
	if ( HasBounced() && !pOther->IsAlive() && pOther->IsPlayer() )
	{
		if ( pAttacker )
		{
			trace_t tr;
			UTIL_TraceLine( pAttacker->WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0f )
			{
				CBaseMultiplayerPlayer* pMultiplayerPlayer = dynamic_cast<CBaseMultiplayerPlayer*>( pAttacker );
				if ( pMultiplayerPlayer )
				{
					pMultiplayerPlayer->AwardAchievement( TF2C_ACHIEVEMENT_KILL_WITH_BLINDCOILRICOCHET );
				}
			}
		}
	}

	// Start remove timer so trail has time to fade
	SetContextThink( &CTFProjectile_Coil::RemoveThink, gpGlobals->curtime + 0.2f, "COIL_REMOVE_THINK" );
}


void CTFProjectile_Coil::RemoveThink( void )
{
	UTIL_Remove( this );
}


CTFProjectile_Coil *CTFProjectile_Coil::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, CBaseEntity *pOwner )
{
	CTFProjectile_Coil *pCoil = static_cast<CTFProjectile_Coil *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_coil", vecOrigin, vecAngles, pOwner, TF_PROJECTILE_COIL ) );
	if ( pCoil )
	{
		// Overriding speed.
		Vector vecForward;
		AngleVectors( vecAngles, &vecForward );

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flSpeed, mult_projectile_speed );

		Vector vecVelocity = vecForward * flSpeed;
		pCoil->SetAbsVelocity( vecVelocity );
		pCoil->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		int iMaxCoilBounces = MAX_COIL_BOUNCES;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMaxCoilBounces, mod_coil_max_bounces );
		pCoil->SetBounces( flSpeed >= 3000.0f ? iMaxCoilBounces : 0 );
	}

	return pCoil;
}
#else

void CTFProjectile_Coil::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();		
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		CreateTrails();
	}
}


void CTFProjectile_Coil::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	const char *pszFormat = m_bCritical ? "coilgun_trail_crit_%s" : "coilgun_trail_%s";
	const char *pszEffectName = GetProjectileParticleName(pszFormat, m_hLauncher, m_bCritical);
	m_pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}
#endif
