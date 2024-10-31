//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADEPROJ_H
#define TF_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_baseprojectile.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFBaseGrenade C_TFBaseGrenade
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFBaseGrenade : public CTFBaseProjectile
{
public:
	DECLARE_CLASS( CTFBaseGrenade, CTFBaseProjectile );
	DECLARE_NETWORKCLASS();

							CTFBaseGrenade();
	virtual					~CTFBaseGrenade();
	virtual void			Spawn();
	virtual void			Precache();

	CNetworkVar( int, m_iDeflected );

protected:
	CNetworkHandle( CBaseEntity, m_hLauncher );
	CNetworkVar( bool, m_bCritical );
	CNetworkHandle( CBaseEntity, m_hDeflectOwner );
	CNetworkVar( int, m_iType ); 

private:
	CTFBaseGrenade( const CTFBaseGrenade & );

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vecInitialVelocity );

	// Client specific.
#ifdef CLIENT_DLL
public:
	virtual void			OnPreDataChanged( DataUpdateType_t updateType );
	virtual void			OnDataChanged( DataUpdateType_t type );
	virtual bool			HasTeamSkins( void ) { return true; }

protected:
	EHANDLE					m_hOldOwner;

	// Server specific.
#else
public:
	DECLARE_DATADESC();

	static CTFBaseGrenade *Create( const char *szName, const Vector &position, const QAngle &angles, 
				const Vector &velocity, const AngularImpulse &angVelocity, 
				CBaseEntity *pOwner, CBaseEntity *pWeapon, int iType = TF_PROJECTILE_NONE );
	 
	virtual int				GetDamageType() const;
	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	virtual void			Detonate( void );

	// Damage accessors.
	virtual float			GetDamage( void ) { return m_flDamage; }
	virtual float			GetDamageRadius( void ) { return m_flRadius; }

	void					SetDamage( float flDamage ) { m_flDamage = flDamage; }
	void					SetDamageRadius( float flDamageRadius ) { m_flRadius = flDamageRadius; }

	void					SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	virtual Vector			GetBlastForce() { return vec3_origin; }

	virtual void			BounceSound( void ) {}

	virtual float			GetShakeAmplitude( void ) { return 10.0; }
	virtual float			GetShakeRadius( void ) { return 300.0; }

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity ) { m_vecInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	void					RemoveGrenade( bool bBlinkOut = true, bool bSparks = false );

	void					SetTimer( float flTime ) { m_flDetonateTime = flTime; }
	float					GetDetonateTime( void ){ return m_flDetonateTime; }
	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual void			Explode( trace_t *pTrace, int bitsDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }

	bool					IsEnemy( CBaseEntity *pOther );

	virtual void			SetLauncher( CBaseEntity *pLauncher );

	virtual bool			IsDeflectable() { return true; }
	virtual void			Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual void			IncremenentDeflected( void );

	float					GetSelfDamageRadius(void);

	virtual void			BlipSound( void ) {}
	void					SetNextBlipTime( float flTime ) { m_flNextBlipTime = flTime; }

	virtual int				GetBaseProjectileType( void ) const { return TF_PROJECTILE_BASE_GRENADE; }

protected:
	void					DrawRadius( float flRadius );

	float					m_flDamage;
	float					m_flRadius;

	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

	float					m_flNextBlipTime;

private:
	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );
	bool					m_bInSolid;

	CHandle<CBaseEntity>	m_hDeflectedBy;

public:
	CHandle<CBaseEntity>	m_pProxyTarget; // best for achievements!

public:
		virtual void			SetDetonatedByCyclops(bool bSet);
		bool					m_bDetonatedByCyclops;
		virtual ETFDmgCustom	GetCyclopsComboDamageCustom(void) { return TF_DMG_CUSTOM_NONE; } // no base
#endif
protected:
	CNetworkVar( float, m_flDetonateTime );
};

#endif // TF_WEAPONBASE_GRENADEPROJ_H
