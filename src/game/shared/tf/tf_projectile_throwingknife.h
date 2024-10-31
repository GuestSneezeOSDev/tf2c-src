//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Throwing Knife projectile
//
//=============================================================================//
#ifndef TF_PROJECTILE_THROWINGKNIFE_H
#define TF_PROJECTILE_THROWINGKNIFE_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_ThrowingKnife C_TFProjectile_ThrowingKnife
#endif

class CTFProjectile_ThrowingKnife : public CTFBaseRocket
{
public:
	DECLARE_CLASS( CTFProjectile_ThrowingKnife, CTFBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_ThrowingKnife();
	~CTFProjectile_ThrowingKnife();

	virtual const char *GetFlightSound() { return NULL; }

#ifdef GAME_DLL
	static CTFProjectile_ThrowingKnife *Create(CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL);
	virtual void	Spawn();
	virtual void	Precache();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_THROWINGKNIFE; }
	virtual float	GetRocketSpeed( void );

	virtual bool IsBehindAndFacingTarget(CBaseEntity *pTarget);
	virtual void AdjustDamageDirection(const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt);

	void ImpactSound(const char *pszSoundName, bool bLoudForAttacker = false);

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
#endif // TF_PROJECTILE_THROWINGKNIFE_H