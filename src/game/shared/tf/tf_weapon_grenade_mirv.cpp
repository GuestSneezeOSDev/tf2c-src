//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Mirv Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_mirv.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "particle_parse.h"
#include "achievements_tf.h"
#include "tf_gamestats.h"
#endif

#define TF_MIRV_AIR_DRAG				0.6f
#define TF_MIRV_BLIP_SOUND				"Weapon_Grenade_Mirv.Timer"
#define TF_MIRV_LEADIN_SOUND			"Weapon_Grenade_Mirv.LeadIn"
#define TF_BOMBLETS_COUNT				4

ConVar tf2c_mirv_impact_damage("tf2c_mirv_impact_damage", "30", FCVAR_NOTIFY | FCVAR_REPLICATED, "Damage of MIRV impact.");
ConVar tf2c_mirv_impact_stun_value("tf2c_mirv_impact_stun_value", "0.85", FCVAR_NOTIFY | FCVAR_REPLICATED, "Value of stun of MIRV impact on player, multiplier.");
ConVar tf2c_mirv_impact_stun_duration("tf2c_mirv_impact_stun_duration", "0.3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of stun of MIRV impact on player.");
ConVar tf2c_mirv_impact_stun_secondary_movespeed("tf2c_mirv_impact_stun_secondary_movespeed", "0.75", FCVAR_NOTIFY | FCVAR_REPLICATED, "Value of stun of MIRV impact on player, movespeed.");
ConVar tf2c_mirv_impact_stun_secondary_duration("tf2c_mirv_impact_stun_secondary_duration", "1.5", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of stun of MIRV impact on player.");


//=============================================================================
//
// TF Mirv Grenade Projectile functions (Server specific).
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeMirvProjectile, DT_TFProjectile_Mirv );
BEGIN_NETWORK_TABLE( CTFGrenadeMirvProjectile, DT_TFProjectile_Mirv )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_mirv_projectile, CTFGrenadeMirvProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_mirv_projectile );

CTFGrenadeMirvProjectile::CTFGrenadeMirvProjectile()
{
#ifdef GAME_DLL
	m_bPlayedLeadIn = false;
	m_bDefused = false;
	m_bAchievementMainPackAirborne = true;
	m_bTouched = false;
#endif
}

CTFGrenadeMirvProjectile::~CTFGrenadeMirvProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL
#define GRENADE_MODEL "models/weapons/w_models/w_grenade_mirv.mdl"


CTFGrenadeMirvProjectile* CTFGrenadeMirvProjectile::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseCombatCharacter *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CTFGrenadeMirvProjectile *>( CTFBaseGrenade::Create( "tf_weapon_grenade_mirv_projectile", position, angles, velocity, angVelocity, pOwner, pWeapon ) );
}


void CTFGrenadeMirvProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );
	SetDetonateTimerLength( 2.5f );
	SetTouch(&CTFGrenadeMirvProjectile::MIRVTouch);

	BaseClass::Spawn();

	// Players need to be able to hit it with their weapons.
	UTIL_SetSize( this, -Vector( 10.0f, 10.0f, 10.0f ), Vector( 10.0f, 10.0f, 10.0f ) );
	AddSolidFlags( FSOLID_TRIGGER );

	// Reduce air resistance
	if ( VPhysicsGetObject() )
	{
		float drag = physenv->GetAirDensity() * TF_MIRV_AIR_DRAG;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), drag, mirv_drag_mod);
		VPhysicsGetObject()->SetDragCoefficient( &drag, NULL );
	}
}


void CTFGrenadeMirvProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheScriptSound( TF_MIRV_LEADIN_SOUND );
	PrecacheScriptSound( TF_MIRV_BLIP_SOUND );
	PrecacheScriptSound( "Weapon_Grenade_Mirv.Disarm" );
	PrecacheScriptSound( "Weapon_Grenade_Mirv.MainExplode" );
	PrecacheScriptSound("Weapon_Grenade_Mirv.Impact");

	PrecacheTeamParticles( "MIRV_trail_%s" );
	PrecacheTeamParticles( "MIRV_trail_%s_crit" );

	BaseClass::Precache();
}


int CTFGrenadeMirvProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !info.GetAttacker() || !IsEnemy( info.GetAttacker() ) )
		return 0;

	// Wrench hit defuses the dynamite pack.
	if ( !m_bDefused && info.GetDamageCustom() == TF_DMG_WRENCH_FIX )
	{
		CDisablePredictionFiltering disabler;

		m_bDefused = true;
		EmitSound( "Weapon_Grenade_Mirv.Disarm" );
		StopParticleEffects( this );

		SetThink( &CBaseEntity::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 5.0f );

		IGameEvent * event = gameeventmanager->CreateEvent( "mirv_defused" );

		CTFPlayer *pScorer = ToTFPlayer( info.GetAttacker() );

		// Work out what killed the player, and send a message to all clients about it
		KillingWeaponData_t weaponData;
		TFGameRules()->GetKillingWeaponName( info, NULL, weaponData );

		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );

		// send bonus and suppport, many kisses
		if (pScorer)
		{
			int iPotentialDamage = GetDamage();
			int iPotentialBombletsCount = TF_BOMBLETS_COUNT;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iPotentialBombletsCount, bomblets_count_mod);
			if (iPotentialBombletsCount > 0)
			{
				iPotentialDamage += iPotentialBombletsCount * GetDamage() * 0.5;
			}
			if (m_bCritical)
			{
				CTF_GameStats.Event_PlayerBlockedDamage(pScorer, iPotentialDamage);
				CTF_GameStats.Event_PlayerAwardBonusPoints(pScorer, this, 2); // Engineer disarming crit TNT that was about to overkill him and team is epic 100
			}
			else
			{
				CTF_GameStats.Event_PlayerBlockedDamage(pScorer, iPotentialDamage / 3.0f);
				CTF_GameStats.Event_PlayerAwardBonusPoints(pScorer, this, 1);
			}
		}

		if ( event )
		{
			if ( pTFPlayer )
			{
				event->SetInt( "userid", pTFPlayer->GetUserID() );
			}
			
			event->SetInt( "attacker", pScorer->GetUserID() );	// attacker
			event->SetString( "weapon", weaponData.szWeaponName );
			event->SetInt( "weaponid", weaponData.iWeaponID );
			event->SetString( "weapon_logclassname", weaponData.szWeaponLogName );
			event->SetInt( "priority", 6 );		// HLTV event priority, not transmitted

			gameeventmanager->FireEvent( event );
		}
	}

	return BaseClass::OnTakeDamage( info );
}


void CTFGrenadeMirvProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Mirv.Bounce" );
}


void CTFGrenadeMirvProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}


void CTFGrenadeMirvProjectile::Explode( trace_t *pTrace, int bitsDamageType )
{
	// Pass through.
	BaseClass::Explode( pTrace, bitsDamageType );

	StopSound( "Weapon_Grenade_Mirv.Explode" );
	EmitSound( "Weapon_Grenade_Mirv.MainExplode" );

	// Create the bomblets.
	int iBombletsCount = TF_BOMBLETS_COUNT;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iBombletsCount, bomblets_count_mod);
	for (int iBomb = 0; iBomb < iBombletsCount; ++iBomb)
	{
		Vector vecSrc = pTrace->endpos + Vector(0, 0, 1.0f);
		Vector vecVelocity(random->RandomFloat(-75.0f, 75.0f) * 5.0f,
			random->RandomFloat(-75.0f, 75.0f) * 5.0f,
			random->RandomFloat(30.0f, 70.0f) * 5.0f);
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), vecVelocity.x, bomblets_velocity_horizontal);
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), vecVelocity.y, bomblets_velocity_horizontal);
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), vecVelocity.z, bomblets_velocity_vertical);
		Vector vecZero(0, 0, 0);
		CBaseCombatCharacter *pOwner = ToBaseCombatCharacter(GetOwnerEntity());

		CTFGrenadeMirvBomb *pBomb = CTFGrenadeMirvBomb::Create(vecSrc, vec3_angle, vecVelocity, vecZero, pOwner, RandomFloat(1.0f, 2.0f));
		pBomb->SetDamage(GetDamage() * 0.5f); // Don't forget to change amount of damage blocked when Engineer wrench defuses it if you change it
		pBomb->SetDetonatedByCyclops(m_bDetonatedByCyclops);
		pBomb->SetDamageRadius(GetDamageRadius());
		pBomb->SetCritical(m_bCritical);
		pBomb->SetLauncher(GetOriginalLauncher());
		pBomb->SetDeflectedBy(GetDeflectedBy());
		pBomb->m_bAchievementMainPackAirborne = m_bAchievementMainPackAirborne;
	}
}


void CTFGrenadeMirvProjectile::BlipSound( void )
{
	if ( GetDetonateTime() - gpGlobals->curtime <= 0.5f )
	{
		if ( !m_bPlayedLeadIn )
		{
			EmitSound( TF_MIRV_LEADIN_SOUND );
			m_bPlayedLeadIn = true;
		}
	}
	else if ( gpGlobals->curtime >= m_flNextBlipTime )
	{
		EmitSound( TF_MIRV_BLIP_SOUND );
		m_flNextBlipTime = gpGlobals->curtime + 1.0f;
	}
}


void CTFGrenadeMirvProjectile::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );
	
	// Copied from CTFGrenadeStickybombProjectile to push stationary grenades better
	// This is kind of lame.
	Vector vecPushSrc = pDeflectedBy->WorldSpaceCenter();
	Vector vecPushDir = GetAbsOrigin() - vecPushSrc;

	CTakeDamageInfo info( pDeflectedBy, pDeflectedBy, 200.0f, DMG_BLAST );
	CalculateExplosiveDamageForce( &info, vecPushDir, vecPushSrc );
	TakeDamage( info );
}


void CTFGrenadeMirvProjectile::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision(index, pEvent);

	m_bAchievementMainPackAirborne = false;

	int otherIndex = !index;
	CBaseEntity* pHitEntity = pEvent->pEntities[otherIndex];
	if (!pHitEntity)
		return;

	if (PropDynamic_CollidesWithGrenades(pHitEntity))
	{
		MIRVTouch(pHitEntity);
	}

	m_bTouched = true;
}

void CTFGrenadeMirvProjectile::MIRVTouch(CBaseEntity* pOther)
{
	// Verify a correct "other".
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
		return;

	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize(velDir);
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine(vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace);

	// Handle hitting skybox (disappear).
	/*if (pTrace.fraction < 1.0f && pTrace.surface.flags & SURF_SKY)
	{
		UTIL_Remove(this);
		return;
	}*/

	// Did we hit an enemy we can damage?
	if (ShouldExplodeOnEntity(pOther))
	{
		// Play explosion sound and effect.
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

		// Impact effects
		bool bEnable = false;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), bEnable, mirv_impact);
		if ( bEnable )
		{
			float flMIRVDamage = tf2c_mirv_impact_damage.GetInt();
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetOriginalLauncher(), flMIRVDamage, mult_mirv_impact_damage);

			UTIL_ImpactTrace(&pTrace, DMG_CLUB);
			ImpactSound("Weapon_Grenade_Mirv.Impact", true);
			DispatchParticleEffect("taunt_headbutt_impact", WorldSpaceCenter(), vec3_angle);

			// Damage.
			CBaseEntity* pAttacker = GetOwnerEntity();

			int iDamageType = DMG_CLUB;
			if (m_bCritical)
			{
				iDamageType |= DMG_CRITICAL;
			}
			CTakeDamageInfo info(this, pAttacker, GetOriginalLauncher(), vec3_origin, GetAbsOrigin(), flMIRVDamage, iDamageType);
			info.SetReportedPosition(pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin);
			info.SetDamageCustom(TF_DMG_MIRV_DIRECT_HIT);
			pOther->TakeDamage(info);

			if (pPlayer)
			{
				pPlayer->m_Shared.StunPlayer(tf2c_mirv_impact_stun_duration.GetFloat(), tf2c_mirv_impact_stun_value.GetFloat(), TF_STUN_MOVEMENT, ToTFPlayer(pAttacker));
				pPlayer->m_Shared.AddCond(TF_COND_MIRV_SLOW, tf2c_mirv_impact_stun_secondary_duration.GetFloat(), pAttacker);
			}
			IPhysicsObject* pPhysicsObj = VPhysicsGetObject();
			Vector velocity;
			AngularImpulse angVelocity;

			if (pPhysicsObj)
			{
				pPhysicsObj->GetVelocity(&velocity, &angVelocity);
				velocity.x = 0.0f;
				velocity.y = 0.0f;
				pPhysicsObj->SetVelocity(&velocity, nullptr);
			}
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

bool CTFGrenadeMirvProjectile::ShouldExplodeOnEntity(CBaseEntity* pOther)
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


//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFGrenadeMirvProjectile::ImpactSound(const char* pszSoundName, bool bLoudForAttacker)
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

void CTFGrenadeMirvProjectile::SetDetonatedByCyclops(bool bSet)
{
	if (!m_bDetonatedByCyclops)
		SetDamage(GetDamage() * 0.667f);

	m_bDetonatedByCyclops = bSet;
}
#else
void CTFGrenadeMirvProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
	}
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

void CTFGrenadeMirvProjectile::CreateTrails( void )
{
	const char *pszFormat = m_bCritical ? "MIRV_trail_%s_crit" : "MIRV_trail_%s";
	const char *pszParticle = GetProjectileParticleName( pszFormat, m_hLauncher, m_bCritical );
	ParticleProp()->Create( pszParticle, PATTACH_ABSORIGIN_FOLLOW );
}
#endif

//=============================================================================
//
// TF Mirv Bomb functions (Server specific).
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeMirvBomb, DT_TFProjectile_MirvBomb );
BEGIN_NETWORK_TABLE( CTFGrenadeMirvBomb, DT_TFProjectile_MirvBomb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_mirv_bomb, CTFGrenadeMirvBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_mirv_bomb );

CTFGrenadeMirvBomb::CTFGrenadeMirvBomb()
{
#ifdef GAME_DLL
	m_bAchievementMainPackAirborne = false;
#endif
}

#ifdef GAME_DLL

#define GRENADE_MODEL_BOMBLET "models/weapons/w_models/w_grenade_bomblet.mdl"

CTFGrenadeMirvBomb *CTFGrenadeMirvBomb::Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
							                    const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, float timer )
{
	CTFGrenadeMirvBomb *pBomb = static_cast<CTFGrenadeMirvBomb*>( CBaseEntity::Create( "tf_weapon_grenade_mirv_bomb", position, angles, pOwner ) );
	if ( pBomb )
	{
		pBomb->SetDetonateTimerLength( timer );
		pBomb->SetupInitialTransmittedGrenadeVelocity( velocity );

		// To be overriden.
		pBomb->SetDamage( 180.0f );
		pBomb->SetDamageRadius( 198.0f );
		
		if ( pOwner )
			pBomb->ChangeTeam( pOwner->GetTeamNumber() );

		pBomb->SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );

		IPhysicsObject *pPhysicsObject = pBomb->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &velocity, &angVelocity );
		}

		pBomb->SetCycle( ( 3.0f - timer ) / 3.0f );
	}

	return pBomb;
}


void CTFGrenadeMirvBomb::Spawn()
{
	SetModel( GRENADE_MODEL_BOMBLET );

	BaseClass::Spawn();

	ResetSequence( LookupSequence( "dynamite_bomblet_fuse" ) );

	EmitSound( "Weapon_Grenade_Mirv.Fuse" );
}


void CTFGrenadeMirvBomb::Precache()
{
	PrecacheModel( GRENADE_MODEL_BOMBLET );
	PrecacheScriptSound( "Weapon_Grenade_Mirv.Fuse" );
	PrecacheParticleSystem( "fuse_sparks" );
	PrecacheTeamParticles( "fuse_sparks_crit_%s" );

	BaseClass::Precache();
}


void CTFGrenadeMirvBomb::StopLoopingSounds( void )
{
	StopSound( "Weapon_Grenade_Mirv.Fuse" );
}


void CTFGrenadeMirvBomb::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_MirvBomb.Bounce" );
}


void CTFGrenadeMirvBomb::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );

	// Copied from CTFGrenadeStickybombProjectile to push stationary grenades better
	// This is kind of lame.
	Vector vecPushSrc = pDeflectedBy->WorldSpaceCenter();
	Vector vecPushDir = GetAbsOrigin() - vecPushSrc;

	CTakeDamageInfo info( pDeflectedBy, pDeflectedBy, 200.0f, DMG_BLAST );
	CalculateExplosiveDamageForce( &info, vecPushDir, vecPushSrc );
	TakeDamage( info );

	// Dynamite sticks are extinguished
	SetDetonateTimerLength( FLT_MAX );
	StopParticleEffects( this );
	StopLoopingSounds();
	SetPlaybackRate( 0.0f );
	SetRenderColor( 192, 192, 192 );

#ifdef GAME_DLL
	CTFPlayer* pTFPlayerDeflectedBy = ToTFPlayer(pDeflectedBy);

	if (pTFPlayerDeflectedBy)
	{
		if (m_bCritical)
		{
			CTF_GameStats.Event_PlayerBlockedDamage(pTFPlayerDeflectedBy, GetDamage());
			// no bonus point cus too spammy and sad that Pyro gets more than Engineer would just by letting Demo do more damage already
		}
		else
		{
			CTF_GameStats.Event_PlayerBlockedDamage(pTFPlayerDeflectedBy, GetDamage() / 3.0f);
		}
	}

	// TF2C_ACHIEVEMENT_EXTINGUISH_BOMBLETS
	if (pTFPlayerDeflectedBy)
	{
		pTFPlayerDeflectedBy->AwardAchievement(TF2C_ACHIEVEMENT_EXTINGUISH_BOMBLETS);
	}
#endif

	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 4.0f );
}


void CTFGrenadeMirvBomb::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}

void CTFGrenadeMirvBomb::SetDetonatedByCyclops(bool bSet)
{
	if (!m_bDetonatedByCyclops)
		SetDamage(GetDamage() * 0.667f);

	m_bDetonatedByCyclops = bSet;
}
#else

void CTFGrenadeMirvBomb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateSparks();
	}
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateSparks();
	}
}


void CTFGrenadeMirvBomb::CreateSparks( void )
{
	const char *pszName = m_bCritical ? ConstructTeamParticle( "fuse_sparks_crit_%s", GetTeamNumber() ) : "fuse_sparks";
	ParticleProp()->Create( pszName, PATTACH_POINT_FOLLOW, "wick" );
}
#endif
