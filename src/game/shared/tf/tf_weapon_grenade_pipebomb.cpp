//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weapon_grenade_pipebomb.h"

#ifdef TF2C_BETA
#include "tf_weapon_pillstreak.h" // !!! foxysen pillstreak
#endif

#ifdef GAME_DLL
#include "tf_gamerules.h"
#else
#include "effect_dispatch_data.h"
#endif
#include <tf_weapon_cyclops.h>

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )

BEGIN_NETWORK_TABLE( CTFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_pipe, CTFGrenadePipebombProjectile );
PRECACHE_REGISTER( tf_projectile_pipe );

#ifdef GAME_DLL
static string_t s_iszTrainName;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::CTFGrenadePipebombProjectile()
{
#ifdef GAME_DLL
	m_bTouched = false;
	s_iszTrainName = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
#else
	m_hTimerParticle = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::~CTFGrenadePipebombProjectile()
{
#ifdef CLIENT_DLL
	if( m_hTimerParticle )
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hTimerParticle );

	ParticleProp()->StopEmissionAndDestroyImmediately();
#endif
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Client specific).
//


void CTFGrenadePipebombProjectile::CreateTrails( void )
{
	const char *pszEffect = GetProjectileParticleName( "pipebombtrail_%s", m_hLauncher );
	m_hTimerParticle = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );

	if ( m_bCritical )
	{
		const char *pszEffectName = GetProjectileParticleName( "critical_pipe_%s", m_hLauncher, m_bCritical );
		ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
}


void CTFGrenadePipebombProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;

		CreateTrails();
	}
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

void CTFGrenadePipebombProjectile::Simulate( void )
{
	BaseClass::Simulate();

	float flTimer = (m_flDetonateTime - SpawnTime()) ? (m_flDetonateTime - SpawnTime()) : 1.0f;

	if( m_hTimerParticle )
	{
		m_hTimerParticle->SetControlPoint(RADIUS_CP1, Vector(1.0f - ((m_flDetonateTime - gpGlobals->curtime) / flTimer), 0, 0));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}

#else

//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Server specific).
//
#define TF_WEAPON_PIPEGRENADE_MODEL		"models/weapons/w_models/w_grenade_grenadelauncher.mdl"
#define TF_WEAPON_PIPEBOMB_BOUNCE_SOUND	"Weapon_Grenade_Pipebomb.Bounce"
#define TF_WEAPON_GRENADE_DETONATE_TIME 2.0f

BEGIN_DATADESC( CTFGrenadePipebombProjectile )
	DEFINE_ENTITYFUNC( PipebombTouch ),
END_DATADESC()


CTFGrenadePipebombProjectile *CTFGrenadePipebombProjectile::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseEntity *pOwner, CBaseEntity *pWeapon, int iType )
{
	return static_cast<CTFGrenadePipebombProjectile *>( CTFBaseGrenade::Create( "tf_projectile_pipe",
		position, angles, velocity, angVelocity, pOwner, pWeapon, iType ) );
}


ETFWeaponID CTFGrenadePipebombProjectile::GetWeaponID( void ) const
{
	return TF_WEAPON_GRENADE_DEMOMAN;
}


void CTFGrenadePipebombProjectile::Spawn()
{
	SetModel( TF_WEAPON_PIPEGRENADE_MODEL );
	SetDetonateTimerLength( TF_WEAPON_GRENADE_DETONATE_TIME );
	SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );

	BaseClass::Spawn();

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );
}


void CTFGrenadePipebombProjectile::Precache()
{
	PrecacheModel( TF_WEAPON_PIPEGRENADE_MODEL );

	PrecacheTeamParticles( "pipebombtrail_%s" );
	PrecacheTeamParticles( "critical_pipe_%s" );

	BaseClass::Precache();
}


void CTFGrenadePipebombProjectile::BounceSound( void )
{
	EmitSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );
}


void CTFGrenadePipebombProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// Reduce the damage by 40% if it explodes on timeout.
	m_flDamage *= 0.6f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOriginalLauncher(), m_flDamage, grenade_detonation_damage_penalty );

	BaseClass::Detonate();
}


void CTFGrenadePipebombProjectile::PipebombTouch( CBaseEntity *pOther )
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
#ifdef TF2C_BETA
		// !!! foxysen pillstreak
		CTFWeaponBase* pWepn = static_cast<CTFWeaponBase*>(GetOriginalLauncher());
		if (pWepn && pWepn->GetWeaponID() == TF_WEAPON_PILLSTREAK && !m_bTouched && !GetDeflectedBy())
		{
			CTFPillstreak* pWepnGL = static_cast<CTFPillstreak*>(pWepn);
			pWepnGL->DecrementPipeStreak();
		}
#endif
		UTIL_Remove( this );
		return;
	}

	// Blow up if we hit an enemy we can damage
	if ( ShouldExplodeOnEntity( pOther ) )
	{
		// Save this entity as enemy, they will take 100% damage.
		m_hEnemy = pOther;
		Explode( &pTrace, GetDamageType() );

#ifdef TF2C_BETA
		// !!! foxysen pillstreak
		CTFWeaponBase* pWepn = static_cast<CTFWeaponBase*>(GetOriginalLauncher());
		CTFPlayer* pTFPlayerOther = ToTFPlayer(pOther);
		if (pTFPlayerOther && pWepn && pWepn->GetWeaponID() == TF_WEAPON_PILLSTREAK && !GetDeflectedBy() // anti-airblast
			&& (!(pTFPlayerOther->m_Shared.IsDisguised() && pTFPlayerOther->m_Shared.DisguiseFoolsTeam(GetTeamNumber())) || pTFPlayerOther->IsDead()))
		{
			CTFPillstreak* pWepnGL = static_cast<CTFPillstreak*>(pWepn);
			pWepnGL->IncrementPipeStreak();
		}
#endif
	}
}


void CTFGrenadePipebombProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( !pHitEntity )
		return;

	if ( PropDynamic_CollidesWithGrenades( pHitEntity ) )
	{
		PipebombTouch( pHitEntity );
	}

	if (!IsMarkedForDeletion()) // not yet sure if this works with base entities. In case PipebombTouch called for UTIL_Remove. YEP IT DOESN'T WORK!
	{
		// !!! foxysen pillstreak
		CTFWeaponBase* pWeapon = static_cast<CTFWeaponBase*>(GetOriginalLauncher());
#ifdef TF2C_BETA
		if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PILLSTREAK && !m_bTouched && !GetDeflectedBy()) // anti-airblast
		{
			CTFPillstreak* pWepnGL = static_cast<CTFPillstreak*>(pWeapon);
			pWepnGL->DecrementPipeStreak();
		}
#endif
		int iDetonateMode = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iDetonateMode, set_detonate_mode);
		if (iDetonateMode == TF_DETMODE_FIZZLEONWORLD)
		{
			RemoveGrenade( false, true );
			m_bTouched = true;
			return;
		}
		else if ( iDetonateMode == TF_DETMODE_EXPLODEONWORLD )
		{
			Detonate();
		}
	}

	// !!! foxysen
	// Write a much better friction system, dumbo
	int iGrenadeNoBounce = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOriginalLauncher(), iGrenadeNoBounce, grenade_no_bounce );
	if ( iGrenadeNoBounce )
	{
		Vector vel;
		AngularImpulse angImp;
		VPhysicsGetObject()->GetVelocity( &vel, &angImp );
		vel *= 0.5;
		angImp *= 0.5;
		VPhysicsGetObject()->SetVelocity( &vel, &angImp );
	}

	// Remember that it collided with something so it no longer explodes on contact.
	m_bTouched = true;
}


bool CTFGrenadePipebombProjectile::ShouldExplodeOnEntity( CBaseEntity *pOther )
{
	// Train hack!
	if ( pOther->GetModelName() == s_iszTrainName && pOther->GetAbsVelocity().LengthSqr() > 1.0f )
		return true;

	// If we already touched a surface then we're not exploding on contact anymore.
	if ( m_bTouched )
		return false;

	if ( PropDynamic_CollidesWithGrenades( pOther ) )
		return true;

	if ( pOther->m_takedamage == DAMAGE_NO )
		return false;

	int iShouldExplodeOnWorldAfterBounces = 0;
	iShouldExplodeOnWorldAfterBounces = CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher.Get(), iShouldExplodeOnWorldAfterBounces, explode_on_impact );

	return IsEnemy( pOther ) || iShouldExplodeOnWorldAfterBounces > 0;
}

ConVar tf_grenade_forcefrom_bullet( "tf_grenade_forcefrom_bullet", "0.8", FCVAR_CHEAT );
ConVar tf_grenade_forcefrom_buckshot( "tf_grenade_forcefrom_buckshot", "0.5", FCVAR_CHEAT );
ConVar tf_grenade_forcefrom_blast( "tf_grenade_forcefrom_blast", "0.08", FCVAR_CHEAT );
ConVar tf_pipebomb_force_to_move( "tf_pipebomb_force_to_move", "1500.0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit, unless we're a sticky, in which case, fizzle out and die.
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_takedamage )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	Assert( pAttacker );
	if ( !pAttacker )
		return 0;

	if ( m_bTouched &&
		( info.GetDamageType() & ( DMG_BULLET | DMG_BLAST | DMG_CLUB | DMG_SLASH ) ) &&
		IsEnemy( info.GetAttacker() ) )
	{
		Vector vecForce = info.GetDamageForce();
		if ( info.GetDamageType() & DMG_BULLET )
		{
			vecForce *= tf_grenade_forcefrom_bullet.GetFloat();
		}
		else if ( info.GetDamageType() & DMG_BUCKSHOT )
		{
			vecForce *= tf_grenade_forcefrom_buckshot.GetFloat();
		}
		else if ( info.GetDamageType() & DMG_BLAST )
		{
			vecForce *= tf_grenade_forcefrom_blast.GetFloat();
		}

		// If the force is sufficient, detach & move the pipebomb.
		if ( vecForce.IsLengthGreaterThan( tf_pipebomb_force_to_move.GetFloat() ) )
		{
			if ( VPhysicsGetObject() )
			{
				VPhysicsGetObject()->EnableMotion( true );
			}

			CTakeDamageInfo newInfo = info;
			newInfo.SetDamageForce( vecForce );

			VPhysicsTakeDamage( newInfo );
			m_bTouched = false;

			// It has moved the data is no longer valid.
			m_bUseImpactNormal = false;
			m_vecImpactNormal.Init();

			return 1;
		}
	}

	return 0;
}
#endif
