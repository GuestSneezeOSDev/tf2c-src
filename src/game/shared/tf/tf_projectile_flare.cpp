//====== Copyright � 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_flare.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
#include "achievements_tf.h"
#endif

#define TF_WEAPON_FLARE_MODEL "models/weapons/w_models/w_flaregun_shell.mdl"

BEGIN_DATADESC( CTFProjectile_Flare )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_flare, CTFProjectile_Flare );
PRECACHE_REGISTER( tf_projectile_flare );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Flare, DT_TFProjectile_Flare )
BEGIN_NETWORK_TABLE( CTFProjectile_Flare, DT_TFProjectile_Flare )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::CTFProjectile_Flare()
{
#ifdef CLIENT_DLL
	m_pEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::~CTFProjectile_Flare()
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

void CTFProjectile_Flare::Precache()
{
	PrecacheModel( TF_WEAPON_FLARE_MODEL );

	PrecacheTeamParticles( "flaregun_trail_%s" );
	PrecacheTeamParticles( "flaregun_trail_crit_%s" );

	PrecacheScriptSound( "TFPlayer.FlareImpact" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Spawn()
{
	SetModel( TF_WEAPON_FLARE_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 0.3f );
}


float CTFProjectile_Flare::GetRocketSpeed( void )
{
	return 2000.0f;
}


void CTFProjectile_Flare::Explode( trace_t *pTrace, CBaseEntity *pOther )
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

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
		// Hit player, do impact sound.
		if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			bool bWasCrit = m_bCritical;

			// no more hardcoded crits!
			int iNoFlareCritVsBurning = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iNoFlareCritVsBurning, not_crit_vs_burning_flare);
			if (!iNoFlareCritVsBurning)
			{
				m_bCritical = true;
			}

			if (m_bCritical)
			{
				// following the rule of Damage Assist points, this hardcode piece of pride gets a hax of its own
				CTFPlayer* pTFBurnProvider = ToTFPlayer(pPlayer->m_Shared.GetConditionProvider(TF_COND_BURNING));
				if (pAttacker)
				{
					if (!bWasCrit && pTFBurnProvider && !pTFBurnProvider->IsEnemy(pAttacker) && pTFBurnProvider != pAttacker)
					{
						CTF_GameStats.Event_PlayerDamageAssist(pTFBurnProvider, GetDamage() * 2); // GetDamage() * 2 is additional crit transformation amount
					}
					if (pTFBurnProvider != pAttacker) // means that it works on kritz flare and even if there is no provider for burn
					{
						CTFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
						CTFWeaponBase* pTFWeapon = dynamic_cast<CTFWeaponBase*>(GetOriginalLauncher());
						if (pTFAttacker && pTFWeapon && pTFWeapon->GetItemID() == 35)
						{
							pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_FLARE_CRIT_TEAMWORK);
						}
					}
				}
			}
		}
		
		CPVSFilter filter( vecOrigin );
		EmitSound( filter, pPlayer->entindex(), "TFPlayer.FlareImpact" );
	}
	else
	{
		// Hit world, do the explosion effect.
		CPVSFilter filter( vecOrigin );
		TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex(), ToBasePlayer( pAttacker ), GetTeamNumber(), m_bCritical );
	}

	CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING );
	info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
	pOther->TakeDamage( info );

	// Start remove timer so trail has time to fade
	SetContextThink( &CTFProjectile_Flare::RemoveThink, gpGlobals->curtime + 0.2f, "FLARE_REMOVE_THINK" );
}


void CTFProjectile_Flare::RemoveThink( void )
{
	UTIL_Remove( this );
}


CTFProjectile_Flare *CTFProjectile_Flare::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	return static_cast<CTFProjectile_Flare *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_flare", vecOrigin, vecAngles, pOwner, TF_PROJECTILE_FLARE ) );
}
#else

void CTFProjectile_Flare::OnDataChanged( DataUpdateType_t updateType )
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


void CTFProjectile_Flare::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	const char *pszFormat = m_bCritical ? "flaregun_trail_crit_%s" : "flaregun_trail_%s";
	const char *pszEffectName = GetProjectileParticleName(pszFormat, m_hLauncher, m_bCritical);
	m_pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}
#endif