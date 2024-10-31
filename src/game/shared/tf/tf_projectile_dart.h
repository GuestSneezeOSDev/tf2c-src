//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Dart used by the Tranquilizer Gun.
//
//=============================================================================//
#ifndef TF_PROJECTILE_DART_H
#define TF_PROJECTILE_DART_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_Dart C_TFProjectile_Dart
#endif

class CTFProjectile_Dart : public CTFBaseRocket
{
public:
	DECLARE_CLASS( CTFProjectile_Dart, CTFBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_Dart();
	~CTFProjectile_Dart();

	virtual const char *GetFlightSound() { return NULL; }

#ifdef GAME_DLL
	static CTFProjectile_Dart *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	virtual void	Spawn();
	virtual void	Precache();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_TRANQ; }
	virtual float	GetRocketSpeed( void );

	// Overrides.
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			CreateTrails( void );
	virtual bool	HasTeamSkins( void ) { return true; }
	int				DrawModel (int flags);

private:
	CNewParticleEffect *m_pEffect;

#endif
};
#endif // TF_PROJECTILE_DART_H
