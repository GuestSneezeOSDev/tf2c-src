//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Melee 
//
//=============================================================================

#ifndef TF_WEAPONBASE_MELEE_H
#define TF_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "tf_baseprojectile.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseMelee C_TFWeaponBaseMelee
#endif

//=============================================================================
//
// Weapon Base Melee Class
//
class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponBaseMelee, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponBaseMelee();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool	HasPrimaryAmmo() { return true; }
	virtual bool	CanBeSelected() { return true; }
	virtual void	ItemPreFrame();
	virtual void	ItemPostFrame();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	Deploy();
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	DoImpactEffect( trace_t &tr, int nDamageType );
	virtual bool	ShouldDrawCrosshair( void ) { return true; }
	virtual void	WeaponReset( void );

	virtual void	Precache( void );

	virtual bool	CanFireRandomCrit( void );
	virtual float	GetCritChance( void );

	virtual void	DoViewModelAnimation( bool bIsDeflect = false );

	bool			DoSwingTrace( trace_t &tr, bool hitAlly = false );
	virtual void	Smack( void );

	virtual float	GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage );
	virtual float	GetSwingRange( void );

	// Call when we hit an entity. Use for special weapon effects on hit.
	virtual void	OnEntityHit( CBaseEntity *pEntity );

	virtual void	SendPlayerAnimEvent( CTFPlayer *pPlayer );

	virtual bool	CheckForgiveness(void);

	Vector			GetDeflectionSize();

#ifdef GAME_DLL
	virtual void	Deflect( bool bProjectilesOnly = false, bool bDoAnimations = true, bool bHitGroundedProjectiles = false );
	virtual void	DeflectEntity( CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir );
	virtual bool	DeflectPlayer( CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir, bool bCanExtinguish = true );
	virtual void	GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates = true, bool bUseHitboxes = false );
	bool			m_bCanDeflect;

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

	virtual bool	ShouldSwitchCritBodyGroup( CBaseEntity *pEntity );
	virtual void	ResetCritBodyGroup( void );

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t type );
	CStudioHdr		*OnNewModel( void );
	void			SwitchBodyGroups( void );
#endif

protected:
	void			Swing( CTFPlayer *pPlayer );

protected:
	CNetworkVar( float, m_flSmackTime );
	CNetworkVar( bool, m_bLandedCrit );
	CNetworkVar( bool, m_bForgiveMiss );

#ifdef CLIENT_DLL
	int m_iCritBodygroup;
	bool m_bOldCritLandedState;
#endif

private:
	CTFWeaponBaseMelee( const CTFWeaponBaseMelee & ) {}

};

#endif // TF_WEAPONBASE_MELEE_H
