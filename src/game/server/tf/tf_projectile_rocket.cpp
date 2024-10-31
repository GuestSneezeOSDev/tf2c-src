
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_rocket.h"
#include "tf_player.h"

//=============================================================================
//
// TF Rocket functions (Server specific).
//
#define ROCKET_MODEL "models/weapons/w_models/w_rocket.mdl"

LINK_ENTITY_TO_CLASS( tf_projectile_rocket, CTFProjectile_Rocket );
PRECACHE_REGISTER( tf_projectile_rocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )
BEGIN_NETWORK_TABLE( CTFProjectile_Rocket, DT_TFProjectile_Rocket )
END_NETWORK_TABLE()


CTFProjectile_Rocket *CTFProjectile_Rocket::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, ProjectileType_t iType )
{
	return static_cast<CTFProjectile_Rocket *>( CTFBaseRocket::Create( pWeapon, "tf_projectile_rocket", vecOrigin, vecAngles, pOwner, iType ) );
}


ETFWeaponID CTFProjectile_Rocket::GetWeaponID( void ) const
{
	return TF_WEAPON_ROCKETLAUNCHER;
}


void CTFProjectile_Rocket::Spawn()
{
	SetModel( ROCKET_MODEL );
	BaseClass::Spawn();
}


void CTFProjectile_Rocket::Precache()
{
	PrecacheModel( ROCKET_MODEL );

	PrecacheTeamParticles( "critical_rocket_%s" );
	PrecacheParticleSystem( "rockettrail" );
	PrecacheParticleSystem( "rockettrail_underwater" );

	BaseClass::Precache();
}
