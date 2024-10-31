//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_PIPEBOMBLAUNCHER_H
#define TF_WEAPON_PIPEBOMBLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_stickybomb.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPipebombLauncher C_TFPipebombLauncher
#endif

//=============================================================================
//
// TF Weapon Pipebomb Launcher.
//
class CTFPipebombLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:
	DECLARE_CLASS( CTFPipebombLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFPipebombLauncher();
	~CTFPipebombLauncher();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PIPEBOMBLAUNCHER; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual void	ItemHolsterFrame(void);
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	PrimaryAttack( void );
	virtual float	GetProjectileSpeed( void );
	virtual bool	Reload( void );
	virtual void	WeaponReset( void );
	virtual int		GetMaxAmmo( void );

public:
	// ITFChargeUpWeapon
	virtual float GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float GetChargeMaxTime( void );
	virtual float GetCurrentCharge( void );

	int	GetPipeBombCount() const { return m_iPipebombCount; }
	CTFGrenadeStickybombProjectile *GetPipeBomb( int idx ) const { return m_Pipebombs[idx]; }

	int GetMaxPipeBombCount() const;

	virtual void LaunchGrenade( void );
	bool DetonateRemotePipebombs( bool bFizzle, bool bPlayerDeath = true );
	void FizzlePipebombs( void );
	void AddPipeBomb( CTFGrenadeStickybombProjectile *pBomb );
	void			DeathNotice( CBaseEntity *pVictim );

#ifdef GAME_DLL
	void			UpdateOnRemove( void );

	// TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY
	void			MarkMinesForAchievementJumpAndDestroy(CBaseEntity* pJumpMine);
	bool			IsMineForAchievementJumpAndDestroy(CBaseEntity* pKillerMine);
#endif

#ifdef GAME_DLL
	virtual bool TFBot_IsExplosiveProjectileWeapon()     OVERRIDE { return true;  }
	virtual bool TFBot_IsContinuousFireWeapon()          OVERRIDE { return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()        OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies() OVERRIDE { return true;  }
	virtual bool TFBot_ShouldCompensateAimForGravity()   OVERRIDE { return true;  }
#endif

	// This is here so we can network the pipebomb count for prediction purposes
	CNetworkVar( int,				m_iPipebombCount );	

	// List of active pipebombs
	typedef CHandle<CTFGrenadeStickybombProjectile>	PipebombHandle;
	CUtlVector<PipebombHandle>		m_Pipebombs;

#ifdef GAME_DLL
	CUtlVector<PipebombHandle>		m_PipebombsAchievementJumpAndDestroy;
#endif

	CNetworkVar( float, m_flChargeBeginTime );
	float	m_flLastDenySoundTime;
	bool	m_bNoAutoRelease;
	bool	m_bWantsToShoot;
	bool	m_bInDetonation;

	CTFPipebombLauncher( const CTFPipebombLauncher & ) {}
};

#endif // TF_WEAPON_PIPEBOMBLAUNCHER_H
