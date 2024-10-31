//=============================================================================//
//
// Purpose: TF2 projectile base.
//
//=============================================================================//
#include "cbase.h"
#include "tf_baseprojectile.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#include "tf_gamerules.h"
#include "particles_new.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_player.h"
#else
#include "te_effect_dispatch.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFBaseProjectile, DT_TFBaseProjectile );
BEGIN_NETWORK_TABLE( CTFBaseProjectile, DT_TFBaseProjectile )
END_NETWORK_TABLE()

void CTFBaseProjectile::Precache( void )
{
	if ( GetFlightSound() )
	{
		PrecacheScriptSound( GetFlightSound() );
	}

	BaseClass::Precache();
}

void CTFBaseProjectile::StopLoopingSounds( void )
{
	//DevMsg( "stop looping sound: %s\n", GetFlightSound() );
	if ( GetFlightSound() )
	{
		StopSound( GetFlightSound() );
	}
}

#ifdef GAME_DLL

ETFWeaponID CTFBaseProjectile::GetWeaponID( void ) const
{
	// Derived classes should override this.
	Assert( false );
	return TF_WEAPON_NONE;
}


void CTFBaseProjectile::Spawn( void )
{
	int iBase = GetBaseProjectileType();

	if ( iBase == TF_PROJECTILE_BASE_ROCKET || iBase == TF_PROJECTILE_BASE_GRENADE )
	{
		string_t strModelOverride = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING_ON_OTHER( GetOriginalLauncher(), strModelOverride, custom_projectile_model );
		if ( strModelOverride != NULL_STRING )
		{
			// Make sure it's precached to avoid a crash
			int i = modelinfo->GetModelIndex( STRING( strModelOverride ) );
			if ( i == -1 )
			{
				Warning( "custom_projectile_model not precached: %s\n", STRING(strModelOverride) );
			}
			else
			{
				SetModel( STRING( strModelOverride ) );
			}
		}
	}
	
	bool bFlightSound = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOriginalLauncher(), bFlightSound, mod_projectile_flight_sound_disable );
	if ( GetFlightSound() && !bFlightSound )
	{
		EmitSound( GetFlightSound() );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Water surface impact effects
//-----------------------------------------------------------------------------
void CTFBaseProjectile::Splash()
{
	CEffectData	data;
	data.m_vOrigin = GetAbsOrigin();
	data.m_flScale = random->RandomFloat( 8, 12 );

	const char *pszEffectName = "tf_gunshotsplash";
	DispatchEffect( pszEffectName, data );
}



float CTFBaseProjectile::GetSelfDamageRadius(void)
{
	// Original rocket radius?
	return 121.0f;
}
#else

int CTFBaseProjectile::GetSkin( void )
{
	if ( HasTeamSkins() )
	{
		return GetTeamSkin( GetTeamNumber() );
	}

	return BaseClass::GetSkin();
}
#endif

const char* CTFBaseProjectile::GetProjectileParticleName( const char* strDefaultName, CBaseEntity* m_hProjLauncher, bool bCritical )
{
	CTFWeaponBase* pWeapon = static_cast<CTFWeaponBase*>( m_hProjLauncher );
	if ( pWeapon )
	{
		CEconItemDefinition *pItemDef = pWeapon->GetItem()->GetStaticData();

		if ( pItemDef )
		{
			EconItemVisuals *pVisuals = pItemDef->GetVisuals( pWeapon->GetTeamNumber() );

			if ( bCritical && pVisuals->trail_effect_crit[0] )
				return pVisuals->trail_effect_crit;

			else if ( pVisuals->trail_effect[0] )
				return pVisuals->trail_effect;
		}

		return ConstructTeamParticle( strDefaultName, pWeapon->GetTeamNumber() );
	}

	return " ";
}