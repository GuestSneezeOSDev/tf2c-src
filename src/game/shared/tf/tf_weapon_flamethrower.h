//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_FLAMETHROWER_H
#define TF_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "particlemgr.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "dlight.h"

#define CTFFlameThrower C_TFFlameThrower
#define CTFFlameRocket C_TFFlameRocket
#else
#include "tf_baseprojectile.h"
#endif

class CTFFlameEntity;

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING,
	FT_STATE_AIRBLASTING
};

enum EFlameThrowerAirblastFunction
{
	AIRBLAST_CAN_PUSH_ENEMIES          = 0x01,
	AIRBLAST_CAN_EXTINGUISH_TEAMMATES  = 0x02,
	AIRBLAST_CAN_DEFLECT_PROJECTILES   = 0x04,

	AIRBLAST_STUNS_PUSHED_ENEMIES      = 0x08, // Live TF2: Applies 100% stun (TF_STUNFLAG_SLOWDOWN) for tf_player_movement_stun_time (unless victim has TF_COND_KNOCKED_INTO_AIR).
	AIRBLAST_AIMPUNCHES_PUSHED_ENEMIES = 0x10, // Live TF2: Calls ApplyPunchImpulseX( RandomInt( 10, 15 ) ) on the airblast victim.
};

//=========================================================
// Flamethrower Weapon
//=========================================================
class CTFFlameThrower : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFFlameThrower, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlameThrower();
	~CTFFlameThrower();

	virtual void	Precache( void );

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_FLAMETHROWER; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	ItemPreFrame( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual bool	Lower( void );
	virtual void	WeaponReset( void );

	virtual void	DestroySounds( void );

	Vector			GetVisualMuzzlePos();
	Vector			GetFlameOriginPos();

	bool			CanAirBlast() const;
	bool			SupportsAirBlastFunction( EFlameThrowerAirblastFunction nFunc ) const;
	Vector			GetDeflectionSize();

	// Client specific.
#if defined( CLIENT_DLL )
	virtual bool	Deploy( void );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove( void );
	virtual void	SetDormant( bool bDormant );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	void			RestartParticleEffect();	
	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();

	// Server specific.
#else
	virtual void	Deflect( bool bProjectilesOnly );
	virtual void	DeflectEntity( CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir );
	virtual bool	DeflectPlayer( CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir, bool bCanExtinguish = true );

	void			SetHitTarget( void );
	void			HitTargetThink( void );
	void			SimulateFlames( void );
	bool			IsSimulatingFlames( void ) { return m_bSimulatingFlames; }
#endif

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( bool, m_bCritFire );
	CNetworkVar( float, m_flAmmoUseRemainder );
	CNetworkVar( bool, m_bHitTarget );

	float		m_flStartFiringTime;
	float		m_flNextPrimaryAttackAnim;

#if defined( CLIENT_DLL )
	int			m_iParticleWaterLevel;

	CSoundPatch	*m_pFiringStartSound;

	CSoundPatch	*m_pFiringLoop;
	bool		m_bFiringLoopCritical;

	CNewParticleEffect *m_pFlameEffect;
	EHANDLE		m_hFlameEffectHost;

	CSoundPatch *m_pPilotLightSound;

	bool		m_bOldHitTarget;
	CSoundPatch *m_pHitTargetSound;

	dlight_t	*m_pDynamicLight;

	bool		m_bHasFlame;
#else
	CUtlVector<CTFFlameEntity> m_Flames;
	bool		m_bSimulatingFlames;
	float		m_flStopHitSoundTime;
#endif

private:
	CTFFlameThrower( const CTFFlameThrower & );
};

#ifdef GAME_DLL
class CTFFlameEntity
{
public:
	void Init( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTFFlameThrower *pWeapon );
	bool FlameThink( void );
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );

private:
	void OnCollide( CBaseEntity *pOther );
	void OnCollideWithTeammate( CTFPlayer *pPlayer );

	Vector						m_vecOrigin;
	Vector						m_vecInitialPos;		// position the flame was fired from
	Vector						m_vecPrevPos;			// position from previous frame
	Vector						m_vecVelocity;
	Vector						m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector						m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	Vector						m_vecMins;
	Vector						m_vecMaxs;
	float						m_flTimeRemove;			// time at which the flame should be removed
	int							m_iTeamNum;
	int							m_iDmgType;				// damage type
	float						m_flDmgAmount;			// amount of base damage
	EHANDLE						m_hOwner;
	CTFFlameThrower				*m_pOuter;
	CUtlVector<EHANDLE>			m_hEntitiesBurnt;		// list of entities this flame has burnt
};

#endif // GAME_DLL

#endif // TF_WEAPON_FLAMETHROWER_H
