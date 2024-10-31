//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include <KeyValues.h>
#include "tf_weapon_parse.h"
#include "tf_shareddefs.h"
#include "tf_playerclass_shared.h"
#include "activitylist.h"
#include "tf_gamerules.h"

const char *g_aExplosionNames[TF_EXPLOSION_COUNT] =
{
	"ExplosionEffect",
	"ExplosionPlayerEffect",
	"ExplosionWaterEffect",
};


FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CTFWeaponInfo;
}


CTFWeaponInfo::CTFWeaponInfo()
{
	m_WeaponData[0].Init();
	m_WeaponData[1].Init();

	m_flDeployTime = 0.0f;

	m_bGrenade = false;
	m_flDamageRadius = 0.0f;
	m_flPrimerTime = 0.0f;
	m_bSuppressGrenTimer = false;
	m_bLowerWeapon = false;

	m_szMuzzleFlashModel[0] = '\0';
	m_flMuzzleFlashModelDuration = 0;
	m_flMuzzleFlashModelScale = 0;
	m_szMuzzleFlashParticleEffect[0] = '\0';

	m_szTracerEffect[0] = '\0';
	m_iTracerFreq = 0;

	m_szBrassModel[0] = '\0';
	m_bDoInstantEjectBrass = true;

	m_szMagazineModel[0] = '\0';

	m_szExplosionSound[0] = '\0';
	memset( m_szExplosionEffects, 0, sizeof( m_szExplosionEffects ) );
	m_bHasTeamColoredExplosions = false;
	m_bHasCritExplosions = false;

	m_iWeaponType = TF_WPN_TYPE_PRIMARY;

	m_iMaxAmmo = 0;
}

CTFWeaponInfo::~CTFWeaponInfo()
{
}


void CTFWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{

	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Primary fire mode.
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flDamage				= pKeyValuesData->GetFloat( "Damage", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange				= pKeyValuesData->GetFloat( "Range", 8192.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot		= pKeyValuesData->GetInt( "BulletsPerShot", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBurstSize			= pKeyValuesData->GetInt( "BurstSize", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstDelay			= pKeyValuesData->GetFloat( "BurstDelay", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread				= pKeyValuesData->GetFloat( "Spread", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle			= pKeyValuesData->GetFloat( "PunchAngle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay		= pKeyValuesData->GetFloat( "TimeFireDelay", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "TimeIdle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty		= pKeyValuesData->GetFloat( "TimeIdleEmpty", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "TimeReloadStart", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload			= pKeyValuesData->GetFloat( "TimeReload", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadRefill	= pKeyValuesData->GetFloat( "TimeReloadRefill", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "DrawCrosshair", 1 ) > 0;
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot			= pKeyValuesData->GetInt( "AmmoPerShot", 1 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "UseRapidFireCrits", 0 ) != 0 );

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = TF_PROJECTILE_NONE;
	const char *pszProjectileType = pKeyValuesData->GetString( "ProjectileType", "projectile_none" );

	for ( int i = 0; i < TF_NUM_PROJECTILES; i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = (ProjectileType_t)i;
			break;
		}
	}	 

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flProjectileSpeed	= pKeyValuesData->GetFloat( "ProjectileSpeed", 1100.0f );		// Rocket speed

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSmackDelay			= pKeyValuesData->GetFloat( "SmackDelay", 0.2f );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSmackDelay		= pKeyValuesData->GetFloat( "Secondary_SmackDelay", 0.2f );

	m_bDoInstantEjectBrass = ( pKeyValuesData->GetInt( "DoInstantEjectBrass", 1 ) != 0 );
	const char *pszBrassModel = pKeyValuesData->GetString( "BrassModel", NULL );
	if ( pszBrassModel )
	{
		V_strcpy_safe( m_szBrassModel, pszBrassModel );
	}

	// Secondary fire mode.
	// Inherit from primary fire mode
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flDamage			= pKeyValuesData->GetFloat( "Secondary_Damage", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flDamage );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flRange			= pKeyValuesData->GetFloat( "Secondary_Range", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nBulletsPerShot	= pKeyValuesData->GetInt( "Secondary_BulletsPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSpread			= pKeyValuesData->GetFloat( "Secondary_Spread", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flPunchAngle		= pKeyValuesData->GetFloat( "Secondary_PunchAngle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeFireDelay	= pKeyValuesData->GetFloat( "Secondary_TimeFireDelay", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "Secondary_TimeIdle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdleEmpty	= pKeyValuesData->GetFloat( "Secondary_TimeIdleEmpy", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "Secondary_TimeReloadStart", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload		= pKeyValuesData->GetFloat( "Secondary_TimeReload", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReloadRefill = pKeyValuesData->GetFloat( "Secondary_TimeReloadRefill", 0.0f );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "Secondary_DrawCrosshair", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair ) > 0;
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iAmmoPerShot		= pKeyValuesData->GetInt( "Secondary_AmmoPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "Secondary_UseRapidFireCrits", 0 ) != 0 );

	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile;
	pszProjectileType = pKeyValuesData->GetString( "Secondary_ProjectileType", "projectile_none" );

	for ( int i = 0; i < TF_NUM_PROJECTILES; i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = (ProjectileType_t)i;
			break;
		}
	}	

	const char *pszWeaponType = pKeyValuesData->GetString( "WeaponType" );

	auto iType = (ETFWeaponType)UTIL_StringFieldToInt( pszWeaponType, g_AnimSlots, TF_WPN_TYPE_COUNT );

	if ( iType >= 0 )
	{
		m_iWeaponType = iType;
	}

	m_flDeployTime = pKeyValuesData->GetFloat( "DeployTime", 0.5f );

	// Grenade data.
	m_bGrenade				= ( pKeyValuesData->GetInt( "Grenade", 0 ) != 0 );
	m_flDamageRadius		= pKeyValuesData->GetFloat( "DamageRadius", 0.0f );
	m_flPrimerTime			= pKeyValuesData->GetFloat( "PrimerTime", 0.0f );
	m_bSuppressGrenTimer	= ( pKeyValuesData->GetInt( "PlayGrenTimer", 1 ) <= 0 );

	m_bLowerWeapon			= ( pKeyValuesData->GetInt( "LowerMainWeapon", 0 ) != 0 );


	// Model muzzleflash
	const char *pszMuzzleFlashModel = pKeyValuesData->GetString( "MuzzleFlashModel", NULL );

	if ( pszMuzzleFlashModel )
	{
		V_strcpy_safe( m_szMuzzleFlashModel, pszMuzzleFlashModel );
	}

	m_flMuzzleFlashModelDuration = pKeyValuesData->GetFloat( "MuzzleFlashModelDuration", 0.12 );

	m_flMuzzleFlashModelScale = pKeyValuesData->GetFloat("MuzzleFlashModelScale", 1.0);

	const char *pszMuzzleFlashParticleEffect = pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL );

	if ( pszMuzzleFlashParticleEffect )
	{
		V_strcpy_safe( m_szMuzzleFlashParticleEffect, pszMuzzleFlashParticleEffect );
	}

	// Tracer particle effect
	const char *pszTracerEffect = pKeyValuesData->GetString( "TracerEffect", NULL );

	if ( pszTracerEffect )
	{
		V_strcpy_safe( m_szTracerEffect, pszTracerEffect );
	}

	m_iTracerFreq = pKeyValuesData->GetInt( "TracerFreq", 2 );

	const char *pszMagModel = pKeyValuesData->GetString( "MagazineModel", NULL );

	if ( pszMagModel )
	{
		V_strcpy_safe( m_szMagazineModel, pszMagModel );
	}

	// Explosion effects (used for grenades)
	const char *pszSound = pKeyValuesData->GetString( "ExplosionSound", "BaseExplosionEffect.Sound" );
	if ( pszSound )
	{
		V_strcpy_safe( m_szExplosionSound, pszSound );
	}

	for ( int i = 0; i < TF_EXPLOSION_COUNT; i++ )
	{
		const char *pszEffect = pKeyValuesData->GetString( g_aExplosionNames[i], NULL );
		if ( pszEffect )
		{
			V_strcpy_safe( m_szExplosionEffects[i], pszEffect );
		}
		else
		{
			switch ( i ) {
			case TF_EXPLOSION_WALL:
				V_strcpy_safe( m_szExplosionEffects[i], "ExplosionCore_wall" );
			case TF_EXPLOSION_AIR:
				V_strcpy_safe( m_szExplosionEffects[i], "ExplosionCore_MidAir" );
			case TF_EXPLOSION_WATER:
				V_strcpy_safe( m_szExplosionEffects[i], "ExplosionCore_MidAir_underwater" );
			default:
				V_strcpy_safe( m_szExplosionEffects[i], "ExplosionCore_MidAir" );
			}
		}
	}

	m_bHasTeamColoredExplosions = pKeyValuesData->GetBool( "HasTeamColoredExplosions" );
	m_bHasCritExplosions = pKeyValuesData->GetBool( "HasCritExplosions" );

	m_bDontDrop = ( pKeyValuesData->GetInt( "DontDrop", 0 ) > 0 );

	m_iMaxAmmo = pKeyValuesData->GetInt( "MaxAmmo", 0 );
}
