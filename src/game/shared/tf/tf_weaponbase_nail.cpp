//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_nail.h"
#include "effect_dispatch_data.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "movevars_shared.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#else
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_player.h"
#include "prediction.h"
#endif

struct TFNailProjectileData_t
{
	const char *model;
	const char *trail;
	const char *trail_crit;
	float speed;
	float gravity;

	int modelIndex;
	int collisionGroup;
	const char *impactSound;
};

static TFNailProjectileData_t g_aNailData[TF_NAIL_COUNT] =
{
	{
		"models/weapons/w_models/w_syringe_proj.mdl",
		"nailtrails_medic_%s",
		"nailtrails_medic_%s_crit",
		1000.0f,
		0.3f,
		0,
		TFCOLLISION_GROUP_ROCKETS_NOTSOLID,
		"Weapon_SyringeGun.ImpactFlesh",
	},
	{
		"models/weapons/w_models/w_nail.mdl",
		"nailtrails_scout_%s",
		"nailtrails_scout_%s_crit",
		2000.0f,
		0.3f,
		0,
		TFCOLLISION_GROUP_ROCKETS_NOTSOLID,
		"Weapon_NailGun.ImpactFlesh",
	},
};

static void PrecacheNails( void *pUser )
{
	for ( int i = 0; i < TF_NAIL_COUNT; i++ )
	{
		g_aNailData[i].modelIndex = CBaseEntity::PrecacheModel( g_aNailData[i].model );

#ifdef GAME_DLL
		PrecacheTeamParticles( g_aNailData[i].trail );
		PrecacheTeamParticles( g_aNailData[i].trail_crit );
#endif
	}
}

PRECACHE_REGISTER_FN( PrecacheNails );

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFBaseNail )
	DEFINE_ENTITYFUNC( ProjectileTouch ),
	DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBaseNail::CTFBaseNail()
{
	m_bCritical = false;
	m_flDamage = 0.0f;
	m_iType = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBaseNail::~CTFBaseNail()
{
}


void CTFBaseNail::Precache( void )
{
	for ( int i = 0; i < TF_NAIL_COUNT; i++ )
	{
		PrecacheScriptSound( g_aNailData[i].impactSound );
	}

	BaseClass::Precache();
}


void CTFBaseNail::Spawn( void )
{
	// Precache.
	Precache();

	BaseClass::Spawn();

	SetModelIndex( g_aNailData[m_iType].modelIndex );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	UTIL_SetSize( this, -Vector( 2.0f, 2.0f, 2.0f ), Vector( 2.0f, 2.0f, 2.0f ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetDamage( 25.0f );

	SetCollisionGroup( g_aNailData[m_iType].collisionGroup );

	// Setup the touch and think functions.
	SetTouch( &CTFBaseNail::ProjectileTouch );
	SetThink( &CTFBaseNail::FlyThink );
	SetNextThink( gpGlobals->curtime );
}


unsigned int CTFBaseNail::PhysicsSolidMaskForEntity( void ) const
{
	int teamContents = 0;

	if ( !CanCollideWithTeammates() )
	{
		// Only collide with the other team

		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			teamContents = CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_BLUE:
			teamContents = CONTENTS_REDTEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_GREEN:
			teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_YELLOW:
			teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM;
			break;
		}
	}
	else
	{
		// Collide with all teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}



void CTFBaseNail::AdjustDamageDirection(const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt)
{
	if (pEnt)
	{
		dir = info.GetDamagePosition() - info.GetDamageForce() - pEnt->WorldSpaceCenter();
	}
}


void CTFBaseNail::ProjectileTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// pass through ladders
	if ( pTrace->surface.flags & CONTENTS_LADDER )
		return;

	if ( pOther->IsWorld() )
	{
		SetAbsVelocity( vec3_origin );
		AddSolidFlags( FSOLID_NOT_SOLID );

		// Remove immediately. Clientside projectiles will stick in the wall for a bit.
		UTIL_Remove( this );
		return;
	}
	else
	{
		if ( !pOther->InSameTeam( GetOwnerEntity() ) )
		{
			CTFPlayer* pVictim = ToTFPlayer( pOther );
			if ( ( pVictim && !pVictim->m_Shared.IsDisguised() ) || pOther->IsBaseObject() )
			{
				// Impact sound
				CTFPlayer* pOwnerPlayer = ToTFPlayer( GetOwnerEntity() );
				if ( pOwnerPlayer )
				{
					// Play one sound to everyone but the owner.
					CPASFilter filter( GetAbsOrigin() );

					// Always play sound to owner and remove them from the general recipient list.
					CSingleUserRecipientFilter singleFilter( pOwnerPlayer );
					EmitSound( singleFilter, pOwnerPlayer->entindex(), g_aNailData[m_iType].impactSound );
					filter.RemoveRecipient( pOwnerPlayer );

					EmitSound( filter, entindex(), g_aNailData[m_iType].impactSound );
				}
			}
		}
	}

	CBaseEntity *pAttacker = GetOwnerEntity();
	CTakeDamageInfo info(this, pAttacker, GetOriginalLauncher(), GetDamageForce(), GetAbsOrigin(), GetDamage(), GetDamageType());

	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );

	info.SetReportedPosition(pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin);
	CalculateBulletDamageForce(&info, TF_AMMO_PRIMARY, dir, pNewTrace->endpos);

	pOther->DispatchTraceAttack( info, dir, pNewTrace );
	ApplyMultiDamage();

	UTIL_Remove( this );
}

Vector CTFBaseNail::GetDamageForce( void )
{
	Vector vecVelocity = GetAbsVelocity();
	VectorNormalize( vecVelocity );
	return ( vecVelocity * GetDamage() );
}

void CTFBaseNail::FlyThink( void )
{
	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetAbsAngles( angles );

	SetNextThink( gpGlobals->curtime + 0.1f );
}


int CTFBaseNail::GetDamageType( void ) const
{
	Assert( GetWeaponID() != TF_WEAPON_NONE );
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	return iDmgType;
}

#else


void ClientsideProjectileCallback( const CEffectData &data )
{
	C_TFPlayer *pPlayer = ToTFPlayer( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );

	Vector vecSrc = data.m_vOrigin;
	QAngle angForward = data.m_vAngles;

	// If we're seeing another player fire a projectile move it to the weapon's muzzle,
	// otherwise if we're in prediction then it's already taken care of.
	if ( !prediction->InPrediction() && pPlayer && !pPlayer->IsDormant() )
	{
		C_TFWeaponBaseGun *pWeapon = dynamic_cast<C_TFWeaponBaseGun *>( pPlayer->GetActiveWeapon() );
		if ( pWeapon )
		{
			pWeapon->GetProjectileFireSetup( pPlayer, vec3_origin, &vecSrc, &angForward );
		}
	}

	Vector vecVelocity;
	AngleVectors( angForward, &vecVelocity );
	vecVelocity *= data.m_flScale;

	Vector vecGravity( 0, 0, -( data.m_flMagnitude * GetCurrentGravity() ) );

	C_LocalTempEntity *pNail = tempents->ClientProjectile( vecSrc, vecVelocity, vecGravity, data.m_nMaterial, 6, pPlayer, "NailImpact" );
	if ( !pNail )
		return;

	pNail->ChangeTeam( data.m_nDamageType );
	pNail->m_nSkin = GetTeamSkin( data.m_nDamageType );
	pNail->AddParticleEffect( GetParticleSystemNameFromIndex( data.m_nHitBox ) );
	pNail->AddEffects( EF_NOSHADOW );
	pNail->flags |= ( FTENT_USEFASTCOLLISIONS | FTENT_COLLISIONGROUP );
	pNail->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS_NOTSOLID );
}

DECLARE_CLIENT_EFFECT( "ClientProjectile_Nail", ClientsideProjectileCallback );
#endif


CTFBaseNail *CTFBaseNail::Create( const char *pszClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iType, bool bCritical )
{
	CTFBaseNail *pProjectile = NULL;

	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	CTFPlayer *pPlayer = ToTFPlayer( pOwner );

	float flSpeed = g_aNailData[iType].speed;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer->GetActiveTFWeapon(), flSpeed, mult_projectile_speed );
	Vector vecVelocity = vecForward * flSpeed;

#ifdef GAME_DLL
	pProjectile = static_cast<CTFBaseNail *>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pProjectile )
		return NULL;

	pProjectile->m_iType = iType;

	// Spawn.
	pProjectile->Spawn();

	pProjectile->SetAbsVelocity( vecVelocity );
	//pProjectile->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	pProjectile->SetAbsAngles( vecAngles );

	pProjectile->SetGravity( g_aNailData[iType].gravity );

	// Set team.
	pProjectile->ChangeTeam( pPlayer->GetTeamNumber() );

	// Hide the projectile and create a fake one on the client.
	pProjectile->AddEffects( EF_NODRAW );
#endif
	const char *pszFormat = bCritical ? g_aNailData[iType].trail_crit : g_aNailData[iType].trail;
	const char *pszParticleName = GetProjectileParticleName( pszFormat, pPlayer->GetActiveTFWeapon(), bCritical );

	CEffectData data;
	data.m_vOrigin = vecOrigin;
	data.m_vAngles = vecAngles;
	data.m_flScale = flSpeed;
	data.m_flMagnitude = g_aNailData[iType].gravity;
	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	data.m_nDamageType = pPlayer->GetTeamNumber();
#ifdef GAME_DLL
	data.m_nMaterial = pProjectile->GetModelIndex();
	data.m_nEntIndex = pPlayer->entindex();
#else
	data.m_nMaterial = g_aNailData[iType].modelIndex;
	data.m_hEntity = pPlayer;
#endif
	DispatchEffect( "ClientProjectile_Nail", data );

	return pProjectile;
}
