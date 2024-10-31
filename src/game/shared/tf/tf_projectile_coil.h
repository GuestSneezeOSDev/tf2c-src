//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Rail projectile used by the Coilgun.
//
//=============================================================================//
#ifndef TF_PROJECTILE_COIL_H
#define TF_PROJECTILE_COIL_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_Coil C_TFProjectile_Coil
#endif

#define MAX_COIL_BOUNCES 3

class CTFProjectile_Coil : public CTFBaseRocket
{
public:
	DECLARE_CLASS( CTFProjectile_Coil, CTFBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_Coil();
	~CTFProjectile_Coil();

	void			SetBounces( int numBounces ) { m_iNumBounces = numBounces; }
	bool			HasBounced( void )	{ return m_bBounced; }

	virtual const char *GetFlightSound() { return NULL; }

private:
	int m_iNumBounces;
	bool m_bBounced;
	float m_flMinBounceLifeTime;

#ifdef GAME_DLL
public:
	static CTFProjectile_Coil *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, CBaseEntity *pOwner = NULL );
	virtual void	Spawn();
	virtual void	Precache();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_COILGUN; }

	// Overrides.
	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual void	RemoveThink( void );

#else
public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			CreateTrails( void );
	virtual bool	HasTeamSkins( void ) { return true; }

private:
	CNewParticleEffect *m_pEffect;
#endif
};
#endif //TF_PROJECTILE_COIL_H
