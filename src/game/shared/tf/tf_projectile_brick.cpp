//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF2C Brick
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_brick.h"

#ifdef GAME_DLL
#include "tf_gamerules.h"
#include "tf_fx.h"
#else
#include "effect_dispatch_data.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFBrickProjectile, DT_TFProjectile_Brick )

BEGIN_NETWORK_TABLE( CTFBrickProjectile, DT_TFProjectile_Brick )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bTouched ) ),
#else
SendPropBool( SENDINFO( m_bTouched ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_brick, CTFBrickProjectile );
PRECACHE_REGISTER( tf_projectile_brick );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFBrickProjectile::CTFBrickProjectile()
{
	m_flCreationTime = 0.0f;
#ifdef GAME_DLL
	m_bTouched = false;
#else
	m_hTrailParticle = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFBrickProjectile::~CTFBrickProjectile()
{
#ifdef CLIENT_DLL
	if( m_hTrailParticle )
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hTrailParticle );

	ParticleProp()->StopEmissionAndDestroyImmediately();
#endif
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Brick Grenade Projectile functions (Client specific).
//


void CTFBrickProjectile::CreateTrails( void )
{
	const char *pszEffect = GetProjectileParticleName( m_bCritical ? "throwable_trail_%s_crit" : "throwable_trail_%s", m_hLauncher, m_bCritical );
	m_hTrailParticle = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
}


void CTFBrickProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;

		CreateTrails();
	}
	else 
	{
		if ( m_hOldOwner.Get() != GetOwnerEntity() )
		{
			ParticleProp()->StopEmission();
			CreateTrails();
		}

		if ( m_bTouched == true )
		{
			ParticleProp()->StopEmission();
		}
	}
}

void CTFBrickProjectile::Simulate( void )
{
	BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFBrickProjectile::DrawModel( int flags )
{
	return BaseClass::DrawModel( flags );
}

#else

//=============================================================================
//
// TF Brick Grenade Projectile functions (Server specific).
//
#define TF_WEAPON_BRICK_MODEL			"models/weapons/w_models/w_brick_scout_projectile.mdl"
#define TF_WEAPON_BRICK_BOUNCE_SOUND	"Weapon_Brick.ImpactSoft"
#define TF_WEAPON_BRICK_IMPACT_SOUND	"Weapon_Brick.ImpactHard"

BEGIN_DATADESC( CTFBrickProjectile )
	DEFINE_ENTITYFUNC( BrickTouch ),
END_DATADESC()


CTFBrickProjectile *CTFBrickProjectile::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseEntity *pOwner, CBaseEntity *pWeapon, int iType )
{
	return static_cast<CTFBrickProjectile *>( CTFBaseGrenade::Create( "tf_projectile_brick",
		position, angles, velocity, angVelocity, pOwner, pWeapon, iType ) );
}


ETFWeaponID CTFBrickProjectile::GetWeaponID( void ) const
{
	return TF_WEAPON_THROWABLE_BRICK;
}


void CTFBrickProjectile::Spawn()
{
	SetModel( TF_WEAPON_BRICK_MODEL );
	SetDetonateTimerLength( 10.0f ); // Simply despawn
	SetTouch( &CTFBrickProjectile::BrickTouch );

	BaseClass::Spawn();

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );

	m_flCreationTime = gpGlobals->curtime;
}


void CTFBrickProjectile::Precache()
{
	PrecacheModel( TF_WEAPON_BRICK_MODEL );

	PrecacheTeamParticles( "throwable_trail_%s" );
	PrecacheTeamParticles( "throwable_trail_%s_crit" );
	PrecacheParticleSystem( "taunt_headbutt_impact" );
	PrecacheParticleSystem( "brick_impact" );
	PrecacheScriptSound( TF_WEAPON_BRICK_BOUNCE_SOUND );
	PrecacheScriptSound( TF_WEAPON_BRICK_IMPACT_SOUND );

	BaseClass::Precache();
}


void CTFBrickProjectile::BounceSound( void )
{
	EmitSound( TF_WEAPON_BRICK_BOUNCE_SOUND );
}


void CTFBrickProjectile::Detonate()
{
	bool bBrickExplode = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), bBrickExplode, brick_explode);
	if ( bBrickExplode )
	{
		BaseClass::Detonate();
		return;
	}

	RemoveGrenade( false, false );
}

#define TF2C_JUMP_VELOCITY					268.3281572999747f
// With 2000 proj speed and its arc, an 0.8s lifetime is ~1200 HU horizontal distance
#define TF2C_BRICK_MINICRIT_TIME			0.8f

void CTFBrickProjectile::BrickTouch( CBaseEntity *pOther )
{
	// Verify a correct "other".
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0f && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Only apply to players or buildings
	if ( !( pOther->IsPlayer() || pOther->IsBaseObject() ) )
		return;

	// Did we hit an enemy we can damage?
	if ( ShouldExplodeOnEntity( pOther ) )
	{
		// Play explosion sound and effect.
		Vector vecOrigin = GetAbsOrigin();
		CTFPlayer* pPlayer = ToTFPlayer( pOther );

		if ( pPlayer )
		{
			EmitSound( "Flesh.BulletImpact" );
		}
		else if ( pOther->IsBaseObject() )
		{
			EmitSound( "SolidMetal.BulletImpact" );
		}

		// Impact effects
		UTIL_ImpactTrace( &pTrace, DMG_CLUB );
		ImpactSound( TF_WEAPON_BRICK_IMPACT_SOUND, true );
		DispatchParticleEffect( "taunt_headbutt_impact", WorldSpaceCenter(), vec3_angle );

		// Damage.
		CBaseEntity* pAttacker = GetOwnerEntity();

		int iDamageType = GetDamageType();
		iDamageType |= DMG_PREVENT_PHYSICS_FORCE;

		// Minicrit at long distances (actually travel time).
		float flMinicritAtDistance = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT(flMinicritAtDistance, mod_brick_minicrit_over_time);
		float flLifeTime = gpGlobals->curtime - m_flCreationTime;
		if ( flMinicritAtDistance > 0.0f && flLifeTime > flMinicritAtDistance )
		{
			auto pTFAttacker = ToTFPlayer( pAttacker );
			if ( pTFAttacker )
			{
				pTFAttacker->SetNextAttackMinicrit( true );
			}
		}

		CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), vec3_origin, GetAbsOrigin(), GetDamage(), iDamageType );
		info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
		pOther->TakeDamage( info );

		IPhysicsObject* pPhysicsObj = VPhysicsGetObject();

		// Knock victim back
		if ( pPlayer )
		{
			float flKnockback = 500.0f; // Same as airblast
			CALL_ATTRIB_HOOK_FLOAT(flKnockback, scattergun_knockback_mult);

			Vector velocity;
			AngularImpulse angVelocity;

			if ( pPhysicsObj )
			{
				pPhysicsObj->GetVelocity( &velocity, &angVelocity );
			}
			//DevMsg( "velocity: (%2.2f %2.2f %2.2f) len %2.2f\n", velocity.x, velocity.y, velocity.z, velocity.Length() );
			Vector vecPushDir = velocity.Normalized();
			//DevMsg( "vecpushdir: (%2.2f %2.2f %2.2f) len %2.2f\n", vecPushDir.x, vecPushDir.y, vecPushDir.z, vecPushDir.Length() );

			// Additional lift to get victim to not stick to the ground
			if ( pPlayer->GetGroundEntity() )
			{
				pPlayer->m_Shared.AirblastPlayer( ToTFPlayer( pAttacker ), Vector(0.0, 0.0, 1.0), TF2C_JUMP_VELOCITY );
			}
			
			pPlayer->m_Shared.AirblastPlayer( ToTFPlayer( pAttacker ), vecPushDir, flKnockback );
#ifdef GAME_DLL
			pPlayer->AddDamagerToHistory( pAttacker, this );
#endif

			// Sell the impact with a brief slow
			pPlayer->m_Shared.StunPlayer( 0.3f, 0.5f, TF_STUN_MOVEMENT, ToTFPlayer( pAttacker ) );

			// Brick recoils off of the victim
			velocity.x *= RandomFloat(0.4f, 0.25f) * RandomInt(-1, 1);
			velocity.y *= RandomFloat(0.4f, 0.25f) * RandomInt(-1, 1);
			pPhysicsObj->SetVelocity(&velocity, nullptr);
		}

	}

	// Set touched so we don't apply effects more than once
	// TODO: Remove trails on impact?
	if ( InSameTeam( pOther ) )
	{
		return;
	}
	m_bTouched = true;
}


void CTFBrickProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( !pHitEntity )
		return;

	if ( PropDynamic_CollidesWithGrenades( pHitEntity ) )
	{
		BrickTouch( pHitEntity );
	}

	if ( pEvent->collisionSpeed > 1000 )
	{
		DispatchParticleEffect("brick_impact", GetAbsOrigin(), vec3_angle);
		EmitSound( TF_WEAPON_BRICK_IMPACT_SOUND );
	}
	// DevMsg( "collspd: %2.2f\n", pEvent->collisionSpeed );
	
	// Set touched on world collisions too
	m_bTouched = true;
}


bool CTFBrickProjectile::ShouldExplodeOnEntity( CBaseEntity *pOther )
{
	// If we already touched a surface then we're not hitting anymore.
	if ( m_bTouched )
		return false;

	if ( PropDynamic_CollidesWithGrenades( pOther ) )
		return true;

	if ( pOther->m_takedamage == DAMAGE_NO )
		return false;

	return IsEnemy( pOther );
}

void CTFBrickProjectile::Deflected(CBaseEntity* pDeflectedBy, Vector& vecDir)
{
	if (m_bTouched)
		return;

	BaseClass::Deflected(pDeflectedBy, vecDir);
}

//-----------------------------------------------------------------------------
// Purpose: Immune to damage
//-----------------------------------------------------------------------------
int CTFBrickProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFBrickProjectile::ImpactSound( const char* pszSoundName, bool bLoudForAttacker )
{
	CTFPlayer* pAttacker = ToTFPlayer( GetOwnerEntity() );
	if ( !pAttacker )
		return;

	if ( bLoudForAttacker )
	{
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = pszSoundName;
		params.m_pflSoundDuration = &soundlen;
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( pAttacker );
		EmitSound( filter, entindex(), params );

		CSingleUserRecipientFilter attackerFilter( pAttacker );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( pszSoundName );
	}
}

#endif
