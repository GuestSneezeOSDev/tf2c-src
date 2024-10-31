//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_PIPEBOMB_H
#define TF_WEAPON_GRENADE_PIPEBOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadePipebombProjectile C_TFGrenadePipebombProjectile
#endif

//=============================================================================
//
// TF Pipebomb Grenade
//
class CTFGrenadePipebombProjectile : public CTFBaseGrenade
{
public:
	DECLARE_CLASS( CTFGrenadePipebombProjectile, CTFBaseGrenade );
	DECLARE_NETWORKCLASS();

	CTFGrenadePipebombProjectile();
	~CTFGrenadePipebombProjectile();

#ifdef CLIENT_DLL

public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual int		DrawModel( int flags );
	virtual void	Simulate( void );

private:
	float			m_flCreationTime;
	HPARTICLEFFECT	m_hTimerParticle;

#else

public:
	DECLARE_DATADESC();

	// Creation.
	static CTFGrenadePipebombProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseEntity *pOwner, CBaseEntity *pWeapon, int iType );

	// Unique identifier.
	virtual ETFWeaponID	GetWeaponID( void ) const;

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();

	virtual void	BounceSound( void );
	virtual void	Detonate();

	virtual void	PipebombTouch( CBaseEntity *pOther );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual bool	ShouldExplodeOnEntity( CBaseEntity *pOther );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	virtual CBaseEntity *GetEnemy( void ) { return m_hEnemy; }
	virtual void SetEnemy( CBaseEntity* pEnemy ) { m_hEnemy = pEnemy; }

private:
	EHANDLE			m_hEnemy;
	bool			m_bTouched;

#endif
};
#endif // TF_WEAPON_GRENADE_PIPEBOMB_H
