//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#ifndef TF_WEAPONBASE_GUN_H
#define TF_WEAPONBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseGun C_TFWeaponBaseGun
#endif

#define ZOOM_CONTEXT		"ZoomContext"
#define ZOOM_REZOOM_TIME	1.4f

//=============================================================================
//
// Weapon Base Melee Gun
//
class CTFWeaponBaseGun : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseGun, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseGun();

	virtual void ItemPostFrame( void );
	virtual void ItemBusyFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual void DoFireEffects();

	void ToggleZoom( void );

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void		GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bDoPassthroughCheck = false );
	void				GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates = true, bool bUseHitboxes = false );

	void ImpactWaterTrace( CTFPlayer *pPlayer, trace_t &trace, const Vector &vecStart );

	void FireBullet ( CTFPlayer *pPlayer );
	CBaseEntity *FireRocket( CTFPlayer *pPlayer, int iRocketType = 0 );
	CBaseEntity *FireNail( CTFPlayer *pPlayer, ProjectileType_t iSpecificNail );
	CBaseEntity *FireFlare( CTFPlayer *pPlayer );
	virtual CBaseEntity *FireArrow( CTFPlayer *pPlayer, ProjectileType_t projectileType );
	CBaseEntity *FireCoil( CTFPlayer *pPlayer );
	CBaseEntity *FireDart( CTFPlayer *pPlayer );
	CBaseEntity *FirePaintball( CTFPlayer *pPlayer );
	CBaseEntity *FireGrenade( CTFPlayer *pPlayer, ProjectileType_t iType );

	ProjectileType_t GetProjectileType( void );
	int GetAmmoPerShot( void );
	virtual float GetWeaponSpread( void );
	virtual float GetProjectileSpeed( void );
	virtual float GetProjectileGravity( void );

	void UpdatePunchAngles( CTFPlayer *pPlayer );
	virtual float GetProjectileDamage( void );

	virtual void ZoomIn( void );
	virtual void ZoomOut( void );
	void ZoomOutIn( void );

	void ExplosiveBulletDelay( void );
	trace_t m_pExplosiveBulletTrace;
	float m_flExplosiveBulletDamage;
	int m_iExplosiveBulletDamageType;
	CHandle<CTFPlayer> m_hExplosiveBulletAttacker;
	EHANDLE m_hExplosiveBulletTarget;

	virtual void PlayWeaponShootSound( void );

#ifdef GAME_DLL
	virtual void HitAlly( CBaseEntity *pEntity ){};

	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE { return true;  }
	virtual bool TFBot_IsCombatWeapon()                      OVERRIDE { return true;  }
	virtual bool TFBot_IsExplosiveProjectileWeapon()         OVERRIDE { return false; }
	virtual bool TFBot_IsContinuousFireWeapon()              OVERRIDE { return true;  }
	virtual bool TFBot_IsBarrageAndReloadWeapon()            OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE { return false; }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies()     OVERRIDE { return false; }
	virtual bool TFBot_ShouldAimForHeadshots()               OVERRIDE { return false; }
	virtual bool TFBot_ShouldCompensateAimForGravity()       OVERRIDE { return false; }
	virtual bool TFBot_IsSniperRifle()                       OVERRIDE { return false; }
	virtual bool TFBot_IsRocketLauncher()                    OVERRIDE { return false; }
#endif

protected:
	void FireIndividualBullet(CTFPlayer *pPlayer, const Vector& vecShootOrigin, const Vector& vecBulletDir,  int iExplosiveBullets = 0); // Inner function for FireBullet, do not use outside

private:
	CTFWeaponBaseGun( const CTFWeaponBaseGun & );
};

#endif // TF_WEAPONBASE_GUN_H