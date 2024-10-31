//====== Copyright � 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Throwing Knife used by the Throwing Knife. Amazing.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_throwingknife.h"
#include "tf_weapon_throwingknife.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "decals.h"
#endif

#define TF_WEAPON_DART_MODEL "models/weapons/w_models/w_dart.mdl"

ConVar tf2c_throwingknife_speed("tf2c_throwingknife_speed", "2400", FCVAR_NOTIFY | FCVAR_REPLICATED, "The flight speed of throwing knife projectile");
ConVar tf2c_throwingknife_gravity("tf2c_throwingknife_gravity", "0.2", FCVAR_NOTIFY | FCVAR_REPLICATED, "The gravity of throwing knife projectile");
ConVar tf2c_throwingknife_throw_upward_force("tf2c_throwingknife_throw_upward_force", "50.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "The force upward with which knife is thrown");
ConVar tf2c_throwingknife_regen_time("tf2c_throwingknife_regen_time", "10.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Throwing knife recharge time");

BEGIN_DATADESC(CTFProjectile_ThrowingKnife)
END_DATADESC()

LINK_ENTITY_TO_CLASS(tf_projectile_throwingknife, CTFProjectile_ThrowingKnife);
PRECACHE_REGISTER( tf_projectile_throwingknife );

IMPLEMENT_NETWORKCLASS_ALIASED(TFProjectile_ThrowingKnife, DT_TFProjectile_ThrowingKnife)
BEGIN_NETWORK_TABLE(CTFProjectile_ThrowingKnife, DT_TFProjectile_ThrowingKnife)
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_ThrowingKnife::CTFProjectile_ThrowingKnife()
{
#ifdef CLIENT_DLL
	m_pEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_ThrowingKnife::~CTFProjectile_ThrowingKnife()
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

void CTFProjectile_ThrowingKnife::Precache()
{
	PrecacheModel( TF_WEAPON_DART_MODEL );

	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s" );
	PrecacheTeamParticles( "tranq_tracer_teamcolor_%s_crit" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_ThrowingKnife::Spawn()
{
	SetModel( TF_WEAPON_DART_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( tf2c_throwingknife_gravity.GetFloat() );
	//UTIL_SetSize(this, -Vector(3.0f, 3.0f, 3.0f), Vector(3.0f, 3.0f, 3.0f));
	SetModelScale(1.65);
	UTIL_SetSize(this, Vector(-1.5), Vector(1.5));
}


float CTFProjectile_ThrowingKnife::GetRocketSpeed( void )
{
	return tf2c_throwingknife_speed.GetFloat();
}


void CTFProjectile_ThrowingKnife::Explode( trace_t *pTrace, CBaseEntity *pOther )
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

	bool bIsBackstab = IsBehindAndFacingTarget(m_hEnemy);
	int iDamageType = bIsBackstab ? GetDamageType() | DMG_CRITICAL : GetDamageType();
	int iCustomDamage = bIsBackstab ? TF_DMG_CUSTOM_BACKSTAB : TF_DMG_CUSTOM_NONE;

	// Do damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	CTakeDamageInfo info(this, pAttacker, GetOriginalLauncher(), GetDamage(), iDamageType, iCustomDamage);
	info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
	CalculateBulletDamageForce( &info, TF_AMMO_PRIMARY, vecDir, trHit.endpos );

	pOther->DispatchTraceAttack( info, vecDir, &trHit );
	ApplyMultiDamage();

	// Recharge weapon depending on what we hit
	if (pPlayer)
	{
		CTFWeaponBase* pWeapon = static_cast<CTFWeaponBase*>(GetOriginalLauncher());
		if (pWeapon)
		{
			pWeapon->DecrementBarRegenTime(tf2c_throwingknife_regen_time.GetFloat() / 4.0f, true);
			/*if (bIsBackstab)
			{
				pWeapon->DecrementBarRegenTime(tf2c_throwingknife_regen_time.GetFloat(), true);
			}
			else
			{
				pWeapon->DecrementBarRegenTime(tf2c_throwingknife_regen_time.GetFloat() / 2.0f, true);
			}*/
		}
	}

	// Instantly remove on hit.
	if ( pPlayer || pOther->IsBaseObject() )
	{
		ImpactSound("Weapon_ThrowingKnife.ImpactFlesh", true);
		UTIL_Remove( this );
		return;
	}

	// Otherwise, stick in surfaces.
	if ( !pOther->IsWorld() )
	{
		// Wrotten quickly, copied from arrow, the impact sound code below doesn't work but we are hacking prototype
		/*if (!(pTrace->surface.flags & SURF_SKY))
		{
			// Play an impact sound.
			surfacedata_t *psurf = physprops->GetSurfaceData(pTrace->surface.surfaceProps);

			const char *pszSoundName;
			switch (psurf ? psurf->game.material : CHAR_TEX_METAL)
			{
			case CHAR_TEX_CONCRETE:
				pszSoundName = "Weapon_ThrowingKnife.ImpactConcrete";
				break;
			case CHAR_TEX_WOOD:
				pszSoundName = "Weapon_ThrowingKnife.ImpactWood";
				break;
			default:
				pszSoundName = "Weapon_ThrowingKnife.ImpactMetal";
				break;
			}
			ImpactSound(pszSoundName);
		}
		SetParent( pOther );*/
	}
	SetTouch( NULL );
	SetAbsVelocity( vec3_origin );
	SetMoveType(MOVETYPE_NONE);
	SetModelScale(1);
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 5.0f );
}


bool CTFProjectile_ThrowingKnife::IsBehindAndFacingTarget(CBaseEntity *pTarget)
{
	Assert(pTarget);

	// Get the forward view vector of the target, ignore Z (up)
	Vector vecVictimForward;
	AngleVectors(pTarget->EyeAngles(), &vecVictimForward);
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get the vector from this projectile's origin to the victim's origin, ignoring Z (up)
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	// Get a forward vector of this projectile; the direction it is pointing towards.
	Vector vecProjectileForward;
	AngleVectors(EyeAngles(), &vecProjectileForward);
	vecProjectileForward.z = 0.0f;
	vecProjectileForward.NormalizeInPlace();

	float flDotBehindVictim = DotProduct(vecToTarget, vecVictimForward);
	//float flDotPointingAtVictim = DotProduct(vecToTarget, vecProjectileForward);
	float flDotSimilarAngle = DotProduct(vecProjectileForward, vecVictimForward);

	// Backstab requires 3 conditions to be met:
	// 1) Spy must be behind the victim (180 deg cone).
	// 2) Spy must be looking at the victim (120 deg cone).
	// 3) Spy must be looking in roughly the same direction as the victim (~215 deg cone).

	// Projectiles are simpler in this sense, given the assumptions we can make based on the projectile nature of the throwing knife:
	// 1) The knife must be behind the victim
	// 2) The knife must be pointing in a similar direction as the victim (215 deg)
	bool bBehindVictim, bSimilarAngle;
	bBehindVictim = flDotBehindVictim > 0.0f;
	bSimilarAngle = flDotSimilarAngle > -0.3f;
	bool result = bBehindVictim && bSimilarAngle;
	/*if (!result)
	{
		//DevMsg("Behind victim: %s, pointing at victim: %s, similar angle: %s.\n", bBehindVictim ? "True" : "False", bPointingAtVictim ? "True" : "False", bSimilarAngle ? "True" : "False");
		DevMsg("Behind victim: %s, similar angle: %s.\n", bBehindVictim ? "True" : "False", bSimilarAngle ? "True" : "False");
	}*/
	
	return result;
}


void CTFProjectile_ThrowingKnife::AdjustDamageDirection(const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt)
{
	if (pEnt)
	{
		dir = info.GetDamagePosition() - info.GetDamageForce() - pEnt->WorldSpaceCenter();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFProjectile_ThrowingKnife::ImpactSound(const char *pszSoundName, bool bLoudForAttacker)
{
	CTFPlayer *pAttacker = ToTFPlayer(GetOwnerEntity());
	if (!pAttacker)
		return;

	if (bLoudForAttacker)
	{
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = pszSoundName;
		params.m_pflSoundDuration = &soundlen;
		CPASFilter filter(GetAbsOrigin());
		filter.RemoveRecipient(pAttacker);
		EmitSound(filter, entindex(), params);

		CSingleUserRecipientFilter attackerFilter(pAttacker);
		EmitSound(attackerFilter, pAttacker->entindex(), params);
	}
	else
	{
		EmitSound(pszSoundName);
	}
}


CTFProjectile_ThrowingKnife *CTFProjectile_ThrowingKnife::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFProjectile_ThrowingKnife* pRocket = static_cast<CTFProjectile_ThrowingKnife *>(CTFBaseRocket::Create(pWeapon, "tf_projectile_throwingknife", vecOrigin, vecAngles, pOwner));

	if (!pRocket)
		return nullptr;
	
	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors(vecAngles, &vecForward, &vecRight, &vecUp);

	float flSpeed = pRocket->GetRocketSpeed();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flSpeed, mult_projectile_speed);

	Vector vecVelocity = vecForward * flSpeed;

	// Some upward force to approach grenade shooting behavior.
	// Players are used to their arcs, and we don't want to
	// require big mouse movements every shot.
	CTFPlayer *pPlayer = ToTFPlayer(pOwner);
	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp);

	vecVelocity = vecVelocity + vecUp * tf2c_throwingknife_throw_upward_force.GetFloat() + vecRight + vecUp;

	pRocket->SetAbsVelocity(vecVelocity);
	pRocket->SetupInitialTransmittedGrenadeVelocity(vecVelocity);

	// Setup the initial angles.
	QAngle angles;
	VectorAngles(vecVelocity, angles);
	pRocket->SetAbsAngles(angles);

	return pRocket;
}
#else

void CTFProjectile_ThrowingKnife::OnDataChanged( DataUpdateType_t updateType )
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


void CTFProjectile_ThrowingKnife::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	const char *pszFormat = m_bCritical ? "tranq_tracer_teamcolor_%s_crit" : "tranq_tracer_teamcolor_%s";
	const char *pszEffectName = GetProjectileParticleName( pszFormat, m_hLauncher, m_bCritical );
	m_pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}



int CTFProjectile_ThrowingKnife::DrawModel(int flags)
{
	// During the first 0.1 seconds of our life, don't draw ourselves.
	if (gpGlobals->curtime - m_flSpawnTime < 0.1f)
		return 0;

	return BaseClass::BaseClass::DrawModel(flags);
}
#endif