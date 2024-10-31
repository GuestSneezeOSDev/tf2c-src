//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Stickybomb.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_STICKYBOMB_H
#define TF_WEAPON_GRENADE_STICKYBOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeStickybombProjectile C_TFGrenadeStickybombProjectile
#endif

//=============================================================================
//
// TF Pipebomb Grenade
//
class CTFGrenadeStickybombProjectile : public CTFBaseGrenade
{
public:
	DECLARE_CLASS( CTFGrenadeStickybombProjectile, CTFBaseGrenade );
	DECLARE_NETWORKCLASS();

	CTFGrenadeStickybombProjectile();
	~CTFGrenadeStickybombProjectile();

	virtual void		UpdateOnRemove( void );

	float				GetCreationTime( void ) { return m_flCreationTime; }
	void				SetChargeBeginTime( float chargeBeginTime ) { m_flChargeBeginTime = chargeBeginTime; }
	void				SetProxyMine( bool isProxy ) { m_bProxyMine = isProxy; }
	bool				IsProxyMine( void ) { return m_bProxyMine; }

private:
	CNetworkVar( float, m_flCreationTime );
	CNetworkVar( float, m_flChargeBeginTime );
	CNetworkVar( float, m_flTouchedTime );
	CNetworkVar( bool, m_bProxyMine );

#ifdef CLIENT_DLL

public:
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		CreateTrails( void );
	virtual int			DrawModel( int flags );
	virtual void		Simulate( void );
	void				UpdateGlowEffect( void );

	CGlowObject			*m_pGlowEffect;

private:
	bool		m_bPulsed;	// This pulse is when it becomes armed, being able to finally detonate
	bool		m_bPulsed2;	// This pulse is when it gets fully armed at 5 seconds, getting no falloff to damage

#else

public:
	DECLARE_DATADESC();

	// Creation.
	static CTFGrenadeStickybombProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseEntity *pOwner, CBaseEntity *pWeapon );

	// Unique identifier.
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADE_PIPEBOMB; }

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();

	virtual int		GetDamageType() const;
	virtual float	GetDamage();
	virtual float	GetDamageRadius();

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;

	virtual void	Detonate();
	virtual void	Fizzle();
	virtual void	DetonateThink();
	virtual void	BlipSound();

	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual bool	IsDeflectableSwapTeam( void ) { return false; }

	virtual int		DrawDebugTextOverlays( void );

	bool			HasTouched() const { return m_bTouched; }
private:
	CBaseEntity*	FindProxyTarget( void );

	bool		m_bTouched;
	bool		m_bFizzle;
	float		m_flMinSleepTime;
public:
	virtual ETFDmgCustom	GetCyclopsComboDamageCustom(void) { return IsProxyMine() ? TF_DMG_CYCLOPS_COMBO_PROXYMINE : TF_DMG_CYCLOPS_COMBO_STICKYBOMB; }
#endif
};
#endif // TF_WEAPON_GRENADE_STICKYBOMB_H
