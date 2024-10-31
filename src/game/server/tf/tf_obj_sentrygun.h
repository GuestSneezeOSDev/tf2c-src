//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_SENTRYGUN_H
#define TF_OBJ_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"
#include "tf_projectile_rocket.h"
#include "../../shared/tf/tf_generator.h"

class CTFPlayer;

class CTFFlameEntitySentry;

enum
{
	SENTRY_LEVEL_1 = 0,
	// SENTRY_LEVEL_1_UPGRADING,
	SENTRY_LEVEL_2,
	// SENTRY_LEVEL_2_UPGRADING,
	SENTRY_LEVEL_3,
};

#define SF_OBJ_UPGRADABLE			0x0004
#define SF_SENTRY_INFINITE_AMMO		0x0008

#define SENTRY_MAX_RANGE 1100.0f		// magic numbers are evil, people. adding this #define to demystify the value. (MSB 5/14/09)
#define SENTRY_MAX_RANGE_SQRD 1210000.0f
#define SENTRY_PUSH_MULTIPLIER 16.0f

// ------------------------------------------------------------------------ //
// Sentrygun object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectSentrygun : public CBaseObject
{
	DECLARE_CLASS( CObjectSentrygun, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectSentrygun();

	static CObjectSentrygun* Create(const Vector &vOrigin, const QAngle &vAngles);

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	Precache();
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void);
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );
	virtual void	SetModel( const char *pModel );
	virtual void	ModifyFireBulletsDamage( CTakeDamageInfo* dmgInfo );

	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	StartPlacement( CTFPlayer *pPlayer );

	// Engineer hit me with a wrench
	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );
	// If the players hit us with a wrench, should we upgrade
	virtual bool	CanBeUpgraded( CTFPlayer *pPlayer );

	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	virtual int		GetTracerAttachment( void );

	virtual bool	IsUpgrading( void ) const;
	virtual bool	IsDisabled( void ) const;

	CTFPlayer		*GetAssistingTeammate( float maxAssistDuration ) const;

	virtual int		GetBaseHealth( void ) const;
	virtual const char	*GetPlacementModel( void ) const;

	virtual void	MakeCarriedObject( CTFPlayer *pPlayer );

	float			GetTimeSinceLastFired( void ) const { return m_LastFireTime.GetElapsedTime(); }
	const QAngle&	GetTurretAngles( void ) const       { return m_vecCurAngles; }

	Vector GetTurretVector() const { Vector vec; AngleVectors( m_vecCurAngles, &vec ); return vec; }

	float GetMaxRange() const;
	float GetMaxRangeOverride() const { return SENTRYGUN_MAX_RANGE * m_flRangeMultiplier * m_flRangeMultiplierOverridenTarget; }
	float GetMaxRangeDefault() const { return SENTRYGUN_MAX_RANGE * m_flRangeMultiplier; }

	void	SetTargetOverride( CBaseEntity *pEnt );

	float GetPushMultiplier();

	friend class CObjectFlameSentry;

private:
	// Main think
	virtual void SentryThink( void );

	void StartUpgrading( void );
	virtual void FinishUpgrading( void );

	// Target acquisition
	bool FindTarget( void );
	bool ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetNextBotNPC( CBaseCombatCharacter *pNPC, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetGenerator( CTeamGenerator *pGenerator, const Vector &vecStart, const Vector &vecEnd );
	void FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter );
	bool FInViewCone ( CBaseEntity *pEntity );
	int Range( CBaseEntity *pTarget );

	// Rotations
	void SentryRotate( void );
	bool MoveTurret( void );

	// Attack
	void Attack( void );
	virtual bool Fire( void );
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	void UpdateNavMeshCombatStatus();

	int GetBaseTurnRate( void );

	

private:
	CNetworkVar( int, m_iState );

	float m_flNextAttack;

	// Rotation
	int m_iRightBound;
	int m_iLeftBound;
	int	m_iBaseTurnRate;
	bool m_bTurningRight;

	QAngle m_vecCurAngles;
	QAngle m_vecGoalAngles;

	float m_flTurnRate;

	// Ammo
	CNetworkVar( int, m_iAmmoShells );
	CNetworkVar( int, m_iMaxAmmoShells );
	CNetworkVar( int, m_iAmmoRockets );
	CNetworkVar( int, m_iMaxAmmoRockets );

	int	m_iAmmoType;

	float m_flNextRocketAttack;

	// Target player / object
	CHandle<CBaseEntity>	m_hEnemy;
	CHandle<CBaseEntity>	m_hEnemyOverride;
	float					m_flOverrideForgetTime;
	int						m_iTurnRateOverride;
	bool					m_bCurrentlyTargetingOverriddenTarget;	// Important: Need to keep track of if we're currently trying to kill a visible overriden target,
																	// so we don't give the extended range buff

	//cached attachment indeces
	int m_iAttachments[4];

	int m_iPitchPoseParameter;
	int m_iYawPoseParameter;

	float m_flLastAttackedTime;

	int m_iFireMode;

	float m_flHeavyBulletResist;

	int m_iPlacementBodygroup;

	float m_flEnableDelay;

	IntervalTimer m_LastFireTime;

	CHandle<CTFPlayer> m_lastTeammateWrenchHit;	// Which teammate last hit us with a wrench
	IntervalTimer m_lastTeammateWrenchHitTimer;		// Time since last wrench hit

	float m_flRangeMultiplier; // Cached bonuses to sentry range from owner and his weapons
	float m_flRangeMultiplierOverridenTarget; // Cached bonuses to sentry range from owner and his weapons when target is overriden

	float m_flFireRateMult; // Cached bonuses to sentry fire rate from owner and his weapons

	CountdownTimer m_ctNavCombatUpdate;

	DECLARE_DATADESC();
};

// Sentry rocket class just to give it a unique class name, so we can distinguish it from other rockets.
class CTFProjectile_SentryRocket : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_SentryRocket, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_SentryRocket();

	// Creation.
	static CTFProjectile_SentryRocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer );	

	virtual void Spawn();
	virtual CBaseEntity *GetAttacker( void );

	//virtual bool CanCollideWithTeammates() const override { return true; }
	virtual const char *GetFlightSound() { return "Building_Sentrygun.RocketFlight"; }

private:
	EHANDLE m_hAttacker;

};

class CObjectFlameSentry : public CObjectSentrygun
{
	DECLARE_CLASS(CObjectFlameSentry, CObjectSentrygun);

public:
	DECLARE_SERVERCLASS();

	CObjectFlameSentry();

	static CObjectFlameSentry* Create(const Vector& vOrigin, const QAngle& vAngles);

	virtual bool Fire( void );

	virtual Vector GetMins() { return Vector(-15, -15, 0); };
	virtual Vector GetMaxs() { return Vector(15, 15, 25); };
	virtual void OnGoActive(void);
	virtual const char* GetPlacementModel(void) const;
	virtual bool	CanBeUpgraded(CTFPlayer* pPlayer);
	virtual bool	StartBuilding(CBaseEntity* pBuilder);
	virtual int		GetBaseHealth(void) const;
	virtual void Attack();

private:
	virtual void SentryThink(void);
	virtual void FinishUpgrading(void);

	//keeps track of flames
	CUtlVector<CTFFlameEntitySentry> m_Flames;
	bool m_bSimulatingFlames;
	void SimulateFlames(void);
	DECLARE_DATADESC();

};

class CTFFlameEntitySentry
{
public:
	void Init(const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity* pOwner, CObjectFlameSentry* pWeapon);
	bool FlameThink(void);
	bool CheckCollision(CBaseEntity* pOther);

private:
	void OnCollide(CBaseEntity* pOther);
	void OnCollideWithTeammate(CTFPlayer* pPlayer);

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
	CObjectFlameSentry* m_pOuter;
	CUtlVector<EHANDLE>			m_hEntitiesBurnt;		// list of entities this flame has burnt
};

#endif // TF_OBJ_SENTRYGUN_H
