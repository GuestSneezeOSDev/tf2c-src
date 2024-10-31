//====== Copyright � 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Dart used by the Tranquilizer Gun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_paintball.h"
#include "tf_weapon_paintballrifle.h"
// #include "engine/ienginesound.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#endif

#define TF_WEAPON_DART_MODEL "models/weapons/w_models/w_dart.mdl"
#define PAINTBALL_TEAM_CHECK_RADIUS 32.0f
#define PAINTBALL_TEAM_CHECK_STEPS 6
#define PAINTBALL_THINK_INTERVAL 0.1f

// If a player is at/above this health ratio, they cannot be hit with the paintball. This is to ease healing groups.
#define HEALTH_IGNORE_RATIO 0.9f

LINK_ENTITY_TO_CLASS( tf_projectile_paintball, CTFProjectile_Paintball );
PRECACHE_REGISTER( tf_projectile_paintball );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Paintball, DT_TFProjectile_Paintball )
BEGIN_NETWORK_TABLE( CTFProjectile_Paintball, DT_TFProjectile_Paintball )
END_NETWORK_TABLE()

ConVar debug_paintball( "debug_paintball", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Debugs the special sphere tracing performed by paintballs.");

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Paintball::CTFProjectile_Paintball()
{
#ifdef CLIENT_DLL
	m_pEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Paintball::~CTFProjectile_Paintball()
{
#ifdef CLIENT_DLL
	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
		m_pEffect = NULL;
	}
#endif
}


// Server specific

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_Paintball )
	DEFINE_ENTITYFUNC( PaintballTouch ),
	DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()


void CTFProjectile_Paintball::Precache()
{
	PrecacheModel( TF_WEAPON_DART_MODEL );

	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s" );
	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s_crit" );

	PrecacheScriptSound( "PaintballGun.Hit" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Paintball::Spawn()
{
	SetModel( TF_WEAPON_DART_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );

	UTIL_SetSize( this, -Vector( 1, 1, 1 ), Vector( 1, 1, 1 ) );
	m_bCanCollideWithTeammates = true;
	SetTouch( &CTFProjectile_Paintball::PaintballTouch );
	SetThink( &CTFProjectile_Paintball::FlyThink );
	SetNextThink( gpGlobals->curtime );
}


float CTFProjectile_Paintball::GetRocketSpeed( void )
{
	return 2400.0f;
}


void CTFProjectile_Paintball::Explode( trace_t *pTrace, CBaseEntity *pOther )
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

	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	// Pull out a bit.
	if ( pTrace->fraction != 1.0f )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Don't decal your teammates or objects on your team.
	//if ( pOther->GetTeamNumber() != GetTeamNumber() )
	{
		UTIL_ImpactTrace( &trHit, DMG_BULLET );
	}

	Vector vecDir = GetAbsVelocity();
	VectorNormalize( vecDir );

	CPASFilter filter( pOther->GetAbsOrigin() );
	EmitSound( filter, entindex(), "PaintballGun.Hit" );

	if ( IsEnemy(pOther) )
	{
		// Do damage.
		CBaseEntity *pAttacker = GetOwnerEntity();
		CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), GetDamage(), GetDamageType() );
		info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
		CalculateBulletDamageForce( &info, TF_AMMO_PRIMARY, vecDir, trHit.endpos );

		pOther->DispatchTraceAttack( info, vecDir, &trHit );
		ApplyMultiDamage();
	}
	else if ( pPlayer )
	{
		CTFWeaponBaseGun *pGun = dynamic_cast<CTFWeaponBaseGun *>(GetOriginalLauncher());
		if ( pGun )
			pGun->HitAlly( pPlayer );
	}
	// Instantly remove on hit.
	if ( pPlayer || pOther->IsBaseObject() )
	{
		UTIL_Remove( this );
		return;
	}

	// Otherwise, stick in surfaces.
	if ( !pOther->IsWorld() )
	{
		SetParent( pOther );
	}
	SetTouch( NULL );
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 5.0f );
}

bool CTFProjectile_Paintball::IsEnemy( CBaseEntity *pOther )
{
	if ( pOther == GetOwnerEntity() )
		return false;

	if ( pOther->GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	return (pOther->GetTeamNumber() != GetTeamNumber());

}

void CTFProjectile_Paintball::PaintballTouch( CBaseEntity *pOther )
{
	// Verify a correct "other".
	//if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
	//	return;
	// Verify a correct "other".

	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
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

	// Removed this as it causes paintballs to just stick into people and not move past em, need to change the collision group.
	//bool bIgnore = false;
	CTFPlayer *pPlayerHit = ToTFPlayer( pOther );
	//if ( pPlayerHit )
	//	bIgnore = (pPlayerHit->GetHealth() / (float)pPlayerHit->m_Shared.GetMaxBuffedHealth() >= HEALTH_IGNORE_RATIO);

	if ( pOther != GetOwnerEntity() )
	{
		// Ignore allied disguised/cloaked spies and full health, targeted allies
		CTFPlayer *pOwner = dynamic_cast<CTFPlayer *>(GetOwnerEntity());
		if ( pOwner && pPlayerHit && pOwner->GetMedigun() )
		{
			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>(pOwner->GetMedigun()->GetWeapon());
			if ( pMedigun )
			{
				if ( pMedigun->IsBackpackPatient( pPlayerHit ) && pPlayerHit->GetHealth() >= pPlayerHit->GetMaxHealth() )
					return;
			}
		}

		// Only apply damage if we're timing out or hitting an enemy directly.
		Explode( &pTrace, pOther );
	}
}

void CTFProjectile_Paintball::FlyThink( void )
{

	float flMinDistToCentre = FLT_MAX;
	CBaseEntity *pClosestEnt = NULL;

	CTraceFilterSimple filter( GetOwnerEntity(), COLLISION_GROUP_NONE );
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + (GetAbsVelocity() * PAINTBALL_THINK_INTERVAL), MASK_SHOT_HULL, &filter, &tr );
	if ( tr.fraction < 1 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( tr.m_pEnt );
		if( pPlayer )
		{
			if ( pPlayer->GetHealth() / (float)pPlayer->m_Shared.GetMaxBuffedHealth() < HEALTH_IGNORE_RATIO )
			{
				SetThink( NULL );			// No more think updates
				Explode( &tr, tr.m_pEnt );
				return;
			}
		}
	}

	float t = 0.0f;
	for ( int n = 0; n < PAINTBALL_TEAM_CHECK_STEPS; n++ )
	{
		// Project our position forward in time.
		Vector projectedPosition = GetAbsOrigin() + (GetAbsVelocity() * t);

		CTFPlayer *prospectivePlayer = NULL;
		// Do a sphere check, and keep track of the nearest, NEAREST player to the centre of any of the projected spheres
		// This way we get the closest player to the centre "rod" of the shot, rather than the first closest-player from the earliest sphere we check
		if ( SphereStepCheck( projectedPosition, prospectivePlayer ) )
		{
			//SetThink( NULL );
			//return;
			if ( prospectivePlayer )
			{
				float flDistSqr = (prospectivePlayer->GetAbsOrigin()).DistToSqr(projectedPosition);
				if ( flDistSqr < flMinDistToCentre )
				{
					flMinDistToCentre = flDistSqr;
					pClosestEnt = prospectivePlayer;
				}
			}
		}

		t += PAINTBALL_THINK_INTERVAL / PAINTBALL_TEAM_CHECK_STEPS;
	}

	if ( pClosestEnt )
	{
		PaintballTouch( pClosestEnt );
	}

	SetNextThink( gpGlobals->curtime + PAINTBALL_THINK_INTERVAL );
}

bool CTFProjectile_Paintball::SphereStepCheck( Vector vCentre, CTFPlayer *&pClosest )
{
	CBaseEntity *pEntity;
	float flMinDistSq = FLT_MAX;
	CTFPlayer *pPlayerClosest = NULL;
	if ( debug_paintball.GetBool() )
		NDebugOverlay::Sphere( vCentre, PAINTBALL_TEAM_CHECK_RADIUS, 255, 255, 255, true, 3.0f );
	for ( CEntitySphereQuery sphere( vCentre, PAINTBALL_TEAM_CHECK_RADIUS, FL_CLIENT | FL_FAKECLIENT ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		CTFPlayer *pPlayer = static_cast<CTFPlayer *>(pEntity);
		if ( pPlayer && pPlayer != GetOwnerEntity() )
		{
			if ( !IsEnemy( pPlayer ) && pPlayer->IsAlive() && !pPlayer->IsDormant() && pPlayer->GetHealth() / (float)pPlayer->m_Shared.GetMaxBuffedHealth() < HEALTH_IGNORE_RATIO )
			{
				float flDistSq = pPlayer->GetAbsOrigin().DistToSqr( vCentre );
				if ( flDistSq < flMinDistSq )
				{
					flMinDistSq = flDistSq;
					pPlayerClosest = pPlayer;
				}
			}
		}
	}

	//NDebugOverlay::Sphere( vCentre, PAINTBALL_TEAM_CHECK_RADIUS, 255, 255, 255, true, 1.5f );

	if ( pPlayerClosest )
	{
		//PaintballTouch( pPlayerClosest );
 		pClosest = pPlayerClosest;
		return true;
	}

	return false;
}


CTFProjectile_Paintball *CTFProjectile_Paintball::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	return static_cast<CTFProjectile_Paintball *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_paintball", vecOrigin, vecAngles, pOwner ) );
}
#else

void CTFProjectile_Paintball::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();		
//		m_fBBoxVisFlags |= VISUALIZE_SURROUNDING_BOUNDS;
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		CreateTrails();
	}
	// Don't draw trail when static.
	else if ( GetMoveType() == MOVETYPE_NONE )
	{
		if ( m_pEffect )
		{
			ParticleProp()->StopEmission( m_pEffect );
			m_pEffect = NULL;
		}
	}
}


void CTFProjectile_Paintball::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	//const char *pszFormat = m_bCritical ? "tranq_tracer_teamcolor_%s_crit" : "tranq_tracer_teamcolor_%s";
	const char *pszFormat = m_bCritical ? "nailtrails_medic_%s_crit" : "nailtrails_medic_%s";
	const char *pszEffectName = GetProjectileParticleName( pszFormat, m_hLauncher, m_bCritical );
	m_pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}



int CTFProjectile_Paintball::DrawModel(int flags)
{
	// During the first 0.1 seconds of our life, don't draw ourselves.
	if (gpGlobals->curtime - m_flSpawnTime < 0.1f)
		return 0;

	return BaseClass::BaseClass::DrawModel(flags);
}
#endif