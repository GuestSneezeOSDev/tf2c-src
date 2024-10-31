//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Healnade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weapon_cyclops.h"
#include "tf_projectile_cyclopsgrenade.h"

#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "tf_gamerules.h"
#include "props.h"
#endif
#include <in_buttons.h>
#include <tf_weaponbase_rocket.h>
#include <tf_weapon_grenade_stickybomb.h>

#ifdef TF2C_BETA

#ifdef GAME_DLL
static ConVar tf2c_cyclops_grenade_enemy_projectiles("tf2c_cyclops_grenade_enemy_projectiles", "2", FCVAR_GAMEDLL, "0=no interaction, 1=explode, 2=fizzle");
static string_t s_iszTrainName; // shut up
#endif

IMPLEMENT_NETWORKCLASS_ALIASED(TFCyclopsGrenadeProjectile, DT_TFCyclopsGrenadeProjectile)

BEGIN_NETWORK_TABLE(CTFCyclopsGrenadeProjectile, DT_TFCyclopsGrenadeProjectile)
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(tf_projectile_grenade_cyclops, CTFCyclopsGrenadeProjectile);
PRECACHE_REGISTER(tf_projectile_grenade_cyclops);

CTFCyclopsGrenadeProjectile::CTFCyclopsGrenadeProjectile()
{
#ifdef GAME_DLL
	m_bTouched = false;
#ifdef CYCLOPSIMPACT
	m_flImpactTime = 0.0f;
#endif
	m_iLatestDetonationDamageCustom = TF_DMG_CUSTOM_NONE;
#endif
}

CTFCyclopsGrenadeProjectile::~CTFCyclopsGrenadeProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

void CTFCyclopsGrenadeProjectile::UpdateOnRemove(void)
{
	// Tell our launcher that we were removed.
	CTFCyclops* pLauncher = dynamic_cast<CTFCyclops*>(GetOriginalLauncher());
	if (pLauncher)
	{
		pLauncher->RemoveCyclopsGrenade(this);
	}

	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL
#define TF_WEAPON_CYCLOPSGRENADE_MODEL		"models/weapons/w_models/w_grenade_cyclops_bouncer.mdl"
#define TF_WEAPON_CYCLOPSGRENADE_DETONATE_TIME 0.5f


void CTFCyclopsGrenadeProjectile::Precache()
{
	BaseClass::Precache();
	PrecacheModel(TF_WEAPON_CYCLOPSGRENADE_MODEL);
	PrecacheScriptSound("Weapon_ProxyBomb.Timer");
	PrecacheTeamParticles("proxymine_pulse_%s");
}

CTFCyclopsGrenadeProjectile* CTFCyclopsGrenadeProjectile::Create(const Vector& position, const QAngle& angles,
	const Vector& velocity, const AngularImpulse& angVelocity,
	CBaseEntity* pOwner, CBaseEntity* pWeapon, int iType)
{
	return static_cast<CTFCyclopsGrenadeProjectile*>(CTFBaseGrenade::Create("tf_projectile_grenade_cyclops",
		position, angles, velocity, angVelocity, pOwner, pWeapon, iType));
}

void CTFCyclopsGrenadeProjectile::Spawn()
{
	BaseClass::Spawn();
	SetModel(TF_WEAPON_CYCLOPSGRENADE_MODEL);
	SetDetonateTimerLength(TF_WEAPON_CYCLOPSGRENADE_DETONATE_TIME);
#ifdef CYCLOPSIMPACT
	SetTouch( &CTFCyclopsGrenadeProjectile::CyclopsTouch );
#else
	SetTouch( &CTFCyclopsGrenadeProjectile::PipebombTouch );
#endif

	// Tell our launcher that we were created.
	CTFCyclops* pLauncher = dynamic_cast<CTFCyclops*>(m_hLauncher.Get());
	if (pLauncher)
	{
		pLauncher->AddCyclopsGrenade(this);
	}
}

void CTFCyclopsGrenadeProjectile::DetonateThink(void)
{
	if (!IsInWorld())
	{
		Remove();
		return;
	}

	BlipSound();

	if ( GetOriginalLauncher() )
	{
		CTFPlayer* pOwner = ToTFPlayer(GetOriginalLauncher()->GetOwnerEntity());
		if (pOwner && pOwner->GetActiveWeapon() == m_hLauncher && pOwner->m_nButtons & IN_ATTACK)
		{
			StudioFrameAdvance();
			DispatchAnimEvents(this);
			SetNextThink(gpGlobals->curtime + 0.1f);
			return;
		}
	}
#ifdef CYCLOPSIMPACT
	if (GetEnemy() && gpGlobals->curtime >= m_flImpactTime)
	{
		SetEnemy(NULL);
	}
#endif

	if (gpGlobals->curtime >= m_flDetonateTime)
	{
		if (gpGlobals->curtime > m_flDetonateTime)
			m_iLatestDetonationDamageCustom = TF_DMG_CYCLOPS_DELAYED;

		Detonate();
		return;
	}

	StudioFrameAdvance();
	DispatchAnimEvents(this);

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CTFCyclopsGrenadeProjectile::BlipSound(void)
{
	if ( gpGlobals->curtime >= m_flDetonateTime )
	{
		if (gpGlobals->curtime >= m_flNextBlipTime)
		{
			EmitSound("Weapon_ProxyBomb.Timer");
			const char* pszEffect = ConstructTeamParticle("proxymine_pulse_%s", GetTeamNumber());
			DispatchParticleEffect(pszEffect, PATTACH_ABSORIGIN_FOLLOW, this);

			m_flNextBlipTime = gpGlobals->curtime + 1.0f;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit, unless we're a sticky, in which case, fizzle out and die.
//-----------------------------------------------------------------------------
int CTFCyclopsGrenadeProjectile::OnTakeDamage(const CTakeDamageInfo& info)
{
	if (!m_takedamage)
		return 0;

	CBaseEntity* pAttacker = info.GetAttacker();
	Assert(pAttacker);
	if (!pAttacker)
		return 0;

	if (m_bTouched && (info.GetDamageType() & (DMG_BULLET | DMG_BLAST | DMG_CLUB | DMG_SLASH)) && IsEnemy(info.GetAttacker()))
	{
		Vector vecForce = info.GetDamageForce();
		if (info.GetDamageType() & DMG_BULLET)
		{
			vecForce *= 1.0f;
		}
		else if (info.GetDamageType() & DMG_BUCKSHOT)
		{
			vecForce *= 0.6f;
		}
		else if (info.GetDamageType() & DMG_BLAST)
		{
			vecForce *= 0.16f;
		}

		// If the force is sufficient, detach & move the pipebomb.
		if (vecForce.IsLengthGreaterThan( 1500.0f ))
		{
			if (VPhysicsGetObject())
			{
				VPhysicsGetObject()->EnableMotion(true);
			}

			CTakeDamageInfo newInfo = info;
			newInfo.SetDamageForce(vecForce);

			VPhysicsTakeDamage(newInfo);

			// It has moved the data is no longer valid.
			m_bUseImpactNormal = false;
			m_vecImpactNormal.Init();

			return 1;
		}
	}

	return 0;
}


void CTFCyclopsGrenadeProjectile::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision(index, pEvent);
#ifdef CYCLOPSIMPACT
	int otherIndex = !index;
	CBaseEntity* pHitEntity = pEvent->pEntities[otherIndex];
	if (!pHitEntity)
		return;

	if (PropDynamic_CollidesWithGrenades(pHitEntity))
	{
		CyclopsTouch(pHitEntity);
	}
#endif

	// !!! foxysen
// Write a much better friction system, dumbo
	Vector vel;
	AngularImpulse angImp;
	VPhysicsGetObject()->GetVelocity(&vel, &angImp);

	if (vel.Length() <= 50.0f)
	{
		vel *= 0.5;
		angImp *= 0.5;
		VPhysicsGetObject()->SetVelocity(&vel, &angImp);
	}

	// Set touched on world collisions too
	// m_bTouched = true;
}


bool CTFCyclopsGrenadeProjectile::ShouldExplodeOnEntity(CBaseEntity* pOther)
{
	// If we already touched a surface then we're not hitting anymore.
	if (m_bTouched)
		return false;

	if (PropDynamic_CollidesWithGrenades(pOther))
		return true;

	if (pOther->m_takedamage == DAMAGE_NO)
		return false;

	return IsEnemy(pOther);
}

#ifdef CYCLOPSIMPACT
void CTFCyclopsGrenadeProjectile::CyclopsTouch(CBaseEntity* pOther)
{
	// Verify a correct "other".
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize(velDir);
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine(vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace);

	if (pTrace.fraction < 1.0f && pTrace.surface.flags & SURF_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	// Only apply to players or buildings
	if (!(pOther->IsPlayer() || pOther->IsBaseObject()))
		return;

	// Blow up if we hit an enemy we can damage
	if (ShouldExplodeOnEntity(pOther))
	{
		// Save this entity as enemy, they will take 100% damage.
		SetEnemy(pOther);
		m_flImpactTime = gpGlobals->curtime + 0.3f;

		Vector vecOrigin = GetAbsOrigin();
		CTFPlayer* pPlayer = ToTFPlayer(pOther);


		if (pPlayer)
		{
			EmitSound("Flesh.BulletImpact");
		}
		else if (pOther->IsBaseObject())
		{
			EmitSound("SolidMetal.BulletImpact");
		}

		UTIL_ImpactTrace(&pTrace, DMG_CLUB);
		ImpactSound("Weapon_Grenade_Mirv.Impact", true);
		DispatchParticleEffect("taunt_headbutt_impact", WorldSpaceCenter(), vec3_angle);

		// Damage.
		float flMIRVDamage = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), flMIRVDamage, mod_cyclops_impact_damage);
		if (flMIRVDamage)
		{
			CBaseEntity* pAttacker = GetOwnerEntity();

			int iDamageType = DMG_CLUB;
			if (m_bCritical)
			{
				iDamageType |= DMG_CRITICAL;
			}
			CTakeDamageInfo info(this, pAttacker, GetOriginalLauncher(), vec3_origin, GetAbsOrigin(), flMIRVDamage, iDamageType);
			info.SetReportedPosition(pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin);
			pOther->TakeDamage(info);
		}

		IPhysicsObject* pPhysicsObj = VPhysicsGetObject();
		Vector velocity;
		AngularImpulse angVelocity;
		if (pPhysicsObj)
		{
			pPhysicsObj->GetVelocity(&velocity, &angVelocity);
			velocity.x *= -0.05f;
			velocity.y *= -0.05f;
			pPhysicsObj->SetVelocity(&velocity, nullptr);
		}
	}

	// Set touched so we don't apply effects more than once
	// TODO: Remove trails on impact?
	if (InSameTeam(pOther))
	{
		return;
	}

	m_bTouched = true;
}

//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFCyclopsGrenadeProjectile::ImpactSound(const char* pszSoundName, bool bLoudForAttacker)
{
	CTFPlayer* pAttacker = ToTFPlayer(GetOwnerEntity());
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
#endif

void CTFCyclopsGrenadeProjectile::Explode(trace_t* pTrace, int bitsDamageType)
{
	SetDetonatedByCyclops(true);

	bool bExplosiveDetonationRadius = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hLauncher, bExplosiveDetonationRadius, cyclops_detonate_other_grenades);
	if (bExplosiveDetonationRadius)
	{
		bool bDetonatedAnyProjectiles = false;

		for (auto& pProj : CTFBaseProjectile::AutoList())
		{
			CTFBaseGrenade* pGrenade = dynamic_cast<CTFBaseGrenade*>(pProj);
			if (!pGrenade)
				continue;

			if (pGrenade == this)
				continue;

			bool bIsStickybomb = pGrenade->ClassMatches("tf_projectile_pipe_remote");
			bool bIsMIRV = pGrenade->ClassMatches("tf_weapon_grenade_mirv_projectile");
			bool bIsMIRVBomblet = pGrenade->ClassMatches("tf_weapon_grenade_mirv_bomb");
			if ( !( bIsStickybomb || bIsMIRV || bIsMIRVBomblet ) )
				continue;
			
			bool bOwnGrenade = pGrenade->GetOwnerEntity() == GetOwnerEntity();
			
			if( !bOwnGrenade )
			{
				if ( pGrenade->GetTeamNumber() == GetTeamNumber() )
					continue;
					
				if( !tf2c_cyclops_grenade_enemy_projectiles.GetBool() )
					continue;

				if( bIsMIRV )
					continue;

				if ( bIsStickybomb )
				{
					CTFGrenadeStickybombProjectile* pSticky = dynamic_cast<CTFGrenadeStickybombProjectile*>(pGrenade);
					if ( pSticky && !pSticky->HasTouched() )
					{
						continue;
					}
				}
			}
					
			float dist = VectorLength(GetAbsOrigin() - pGrenade->GetAbsOrigin());
			// keep radius unaffected
			if (dist <= 146.0f)
			{
				pGrenade->SetOwnerEntity(GetOwnerEntity());
				pGrenade->ChangeTeam(GetTeamNumber());
				pGrenade->SetDetonatedByCyclops(true);
				
				if( bOwnGrenade || tf2c_cyclops_grenade_enemy_projectiles.GetInt() == 1 )
				{
					pGrenade->SetDetonateTimerLength( 0.0f );
				}
				else
				{
					pGrenade->RemoveGrenade(true, true);
				}
				
				if (bIsStickybomb)
				{
					m_iLatestDetonationDamageCustom = TF_DMG_CYCLOPS_COMBO_STICKYBOMB;
				}
				else if (bIsMIRV)
				{
					m_iLatestDetonationDamageCustom = TF_DMG_CYCLOPS_COMBO_MIRV;
				}
				else if (bIsMIRVBomblet)
				{
					m_iLatestDetonationDamageCustom = TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET;
				}

				bDetonatedAnyProjectiles = true;
			}

			CTFPlayer* pOwner = ToTFPlayer(GetOwnerEntity());
			IGameEvent* event = gameeventmanager->CreateEvent("cyclops_destroy_proj");
			if (event && bDetonatedAnyProjectiles && !bOwnGrenade)
			{
				event->SetInt("userid", pOwner->GetUserID());
				gameeventmanager->FireEvent(event);
			}
		}

		if (bDetonatedAnyProjectiles)
		{
			CTFPlayer* pTFAttacker = ToTFPlayer(m_hLauncher->GetOwnerEntity());
			if (pTFAttacker)
			{
				EmitSound_t params;
				params.m_pSoundName = "TFPlayer.CritHit";
				
				// world, without attacker
				CPASAttenuationFilter filter( GetAbsOrigin(), 0.5f );
				filter.RemoveRecipient( pTFAttacker );
				EmitSound( filter, entindex(), params );
				
				// attacker
				CSingleUserRecipientFilter attackerFilter(pTFAttacker);
				EmitSound(attackerFilter, pTFAttacker->entindex(), params);
			}
		}
	}

	BaseClass::Explode(pTrace, bitsDamageType);
}

void CTFCyclopsGrenadeProjectile::Detonate()
{
	if (ShouldNotDetonate())
	{
		RemoveGrenade();
		return;
	}

	BaseClass::BaseClass::Detonate();
}

void CTFCyclopsGrenadeProjectile::Deflected(CBaseEntity* pDeflectedBy, Vector& vecDir)
{
	SetDetonateTimerLength(TF_WEAPON_CYCLOPSGRENADE_DETONATE_TIME);

	BaseClass::Deflected(pDeflectedBy, vecDir);

	// Tell our launcher that we changed teams
	CTFCyclops* pLauncher = dynamic_cast<CTFCyclops*>(GetOriginalLauncher());
	if (pLauncher)
	{
		pLauncher->RemoveCyclopsGrenade(this);
	}
}

#endif


#endif // TF2C_BETA
