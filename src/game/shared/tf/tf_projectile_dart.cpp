//====== Copyright � 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Dart used by the Tranquilizer Gun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_dart.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#endif

#define TF_WEAPON_DART_MODEL "models/weapons/w_models/w_dart.mdl"
#define TF_WEAPON_DART_IMPACT_SOUND "Weapon_Tranq.ImpactFlesh"

BEGIN_DATADESC( CTFProjectile_Dart )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_dart, CTFProjectile_Dart );
PRECACHE_REGISTER( tf_projectile_dart );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Dart, DT_TFProjectile_Dart )
BEGIN_NETWORK_TABLE( CTFProjectile_Dart, DT_TFProjectile_Dart )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Dart::CTFProjectile_Dart()
{
#ifdef CLIENT_DLL
	m_pEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Dart::~CTFProjectile_Dart()
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

void CTFProjectile_Dart::Precache()
{
	PrecacheModel( TF_WEAPON_DART_MODEL );

	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s" );
	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s_crit" );

	PrecacheScriptSound(TF_WEAPON_DART_IMPACT_SOUND);

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Dart::Spawn()
{
	SetModel( TF_WEAPON_DART_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 0.2f );
}


float CTFProjectile_Dart::GetRocketSpeed( void )
{
	return 2400.0f;
}


void CTFProjectile_Dart::Explode( trace_t *pTrace, CBaseEntity *pOther )
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
	if ( pOther->GetTeamNumber() != GetTeamNumber() )
	{
		UTIL_ImpactTrace( &trHit, DMG_BULLET );
	}

	Vector vecDir = GetAbsVelocity();
	VectorNormalize( vecDir );

	// Do damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), GetDamage(), GetDamageType() );
	info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
	CalculateBulletDamageForce( &info, TF_AMMO_PRIMARY, vecDir, trHit.endpos );

	pOther->DispatchTraceAttack( info, vecDir, &trHit );
	ApplyMultiDamage();

	// Play impact sound and instantly remove on hit.
	if ( (pPlayer && !pPlayer->m_Shared.IsDisguised()) || pOther->IsBaseObject() )
	{
		// Impact sound copypasta from tf_weaponbase_nail
		CTFPlayer* pOwnerPlayer = ToTFPlayer(pAttacker);
		if (pOwnerPlayer)
		{
			// Play one sound to everyone but the owner.
			CPASFilter filter(GetAbsOrigin());

			// Always play sound to owner and remove them from the general recipient list.
			CSingleUserRecipientFilter singleFilter(pOwnerPlayer);
			EmitSound(singleFilter, pOwnerPlayer->entindex(), TF_WEAPON_DART_IMPACT_SOUND);
			filter.RemoveRecipient(pOwnerPlayer);

			EmitSound(filter, entindex(), TF_WEAPON_DART_IMPACT_SOUND);
		}

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


CTFProjectile_Dart *CTFProjectile_Dart::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	return static_cast<CTFProjectile_Dart *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_dart", vecOrigin, vecAngles, pOwner ) );
}
#else

void CTFProjectile_Dart::OnDataChanged( DataUpdateType_t updateType )
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


void CTFProjectile_Dart::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	const char *pszFormat = m_bCritical ? "tranq_tracer_teamcolor_%s_crit" : "tranq_tracer_teamcolor_%s";
	const char *pszEffectName = GetProjectileParticleName(pszFormat, m_hLauncher, m_bCritical);
	m_pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}



int CTFProjectile_Dart::DrawModel(int flags)
{
	// During the first 0.1 seconds of our life, don't draw ourselves.
	if (gpGlobals->curtime - m_flSpawnTime < 0.1f)
		return 0;

	return BaseClass::BaseClass::DrawModel(flags);
}
#endif
