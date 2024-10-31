//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_ROCKET_H
#define TF_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_baseprojectile.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
#define CTFBaseRocket C_TFBaseRocket
#endif

//=============================================================================
//
// TF Base Rocket.
//
class CTFBaseRocket : public CTFBaseProjectile
{

//=============================================================================
//
// Shared (client/server).
//
public:
	DECLARE_CLASS( CTFBaseRocket, CTFBaseProjectile );
	DECLARE_NETWORKCLASS();

	CTFBaseRocket();
	~CTFBaseRocket();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	StopLoopingSounds( void );

	virtual int		GetProjectileType() const { return m_iType; }
	virtual const char *GetFlightSound() { return "Weapon_RPG.Flight"; }

	CNetworkVar( int, m_iDeflected );
	CNetworkHandle( CBaseEntity, m_hLauncher );

protected:
	// Networked.
	CNetworkVector( m_vecInitialVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	CNetworkVar( bool, m_bCritical );
	CNetworkVar( ProjectileType_t, m_iType );

//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL
public:
	virtual int		DrawModel( int flags );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	PostDataUpdate( DataUpdateType_t type );
	virtual void	Simulate( void );

protected:
	EHANDLE	m_hOldOwner;

	float	m_flSpawnTime;	// moved from private
private:
	bool	m_bWhistling;

//=============================================================================
//
// Server specific.
//
#else
public:
	DECLARE_DATADESC();

	static CTFBaseRocket *Create( CBaseEntity *pWeapon, const char *szClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, ProjectileType_t iType = TF_PROJECTILE_NONE );

	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual bool	HurtCylinderCheck(CBaseEntity *pOther);
	//virtual bool	TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);

	void			SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual float	GetDamage( void ) { return m_flDamage; }
	virtual int		GetDamageType( void ) const;
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	void			SetType( ProjectileType_t iType ) { m_iType = iType; }
	virtual float	GetRadius( void );
	float			GetSelfDamageRadius( void );
	void			DrawRadius( float flRadius );
	virtual float	GetRocketSpeed( void );
	virtual CBaseEntity *GetAttacker( void );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vecInitialVelocity = velocity; }

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }

	virtual CBaseEntity	*GetEnemy( void )			{ return m_hEnemy; }

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual void	IncremenentDeflected( void );
	virtual void	SetLauncher( CBaseEntity *pLauncher );

	virtual int		GetBaseProjectileType( void ) const { return TF_PROJECTILE_BASE_ROCKET; }

	void			DetonateThink( void );

protected:
	virtual void			FlyThink( void );

protected:
	// Not networked.
	float					m_flDamage;

	CHandle<CBaseEntity>	m_hEnemy;
#endif
};

#endif // TF_WEAPONBASE_ROCKET_H