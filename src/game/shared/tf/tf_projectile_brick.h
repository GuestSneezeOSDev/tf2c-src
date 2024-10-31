//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF2C Brick
//
//=============================================================================//
#ifndef TF_PROJECTILE_BRICK_H
#define TF_PROJECTILE_BRICK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFBrickProjectile C_TFBrickProjectile
#endif

//=============================================================================
//
// TF2C Brick
//
class CTFBrickProjectile : public CTFBaseGrenade
{
public:
	DECLARE_CLASS( CTFBrickProjectile, CTFBaseGrenade );
	DECLARE_NETWORKCLASS();

	CTFBrickProjectile();
	~CTFBrickProjectile();

private:
	float			m_flCreationTime;

	CNetworkVar( bool, m_bTouched );

#ifdef CLIENT_DLL

public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual int		DrawModel( int flags );
	virtual void	Simulate( void );

private:
	HPARTICLEFFECT	m_hTrailParticle;

#else

public:
	DECLARE_DATADESC();

	// Creation.
	static CTFBrickProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseEntity *pOwner, CBaseEntity *pWeapon, int iType );

	// Unique identifier.
	virtual ETFWeaponID	GetWeaponID( void ) const;

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();

	virtual void	BounceSound( void );
	virtual void	Detonate();

	virtual void	BrickTouch( CBaseEntity *pOther );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual bool	ShouldExplodeOnEntity( CBaseEntity *pOther );

	virtual void	Deflected( CBaseEntity* pDeflectedBy, Vector& vecDir );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	void			ImpactSound( const char* pszSoundName, bool bLoudForAttacker = false );

	virtual CBaseEntity* GetEnemy( void ) { return NULL; }

private:
	

#endif
};
#endif // TF_PROJECTILE_BRICK_H
