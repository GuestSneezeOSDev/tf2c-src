//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Dart used by the Tranquilizer Gun.
//
//=============================================================================//
#ifndef TF_PROJECTILE_PAINTBALL_H
#define TF_PROJECTILE_PAINTBALL_H

#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
#include "tf_player.h"
#endif
#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_Paintball C_TFProjectile_Paintball
#endif

class CTFProjectile_Paintball : public CTFBaseRocket
{
public:
	DECLARE_CLASS( CTFProjectile_Paintball, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_Paintball();
	~CTFProjectile_Paintball();

	virtual const char *GetFlightSound() { return NULL; }

#ifdef GAME_DLL
	DECLARE_DATADESC();

	static CTFProjectile_Paintball *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	virtual void	Spawn();
	virtual void	Precache();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PAINTBALLRIFLE; }
	virtual float	GetRocketSpeed( void );
	virtual void	FlyThink( void );
	virtual bool	SphereStepCheck( Vector vCentre, CTFPlayer *&pClosest );

	bool			IsEnemy( CBaseEntity *pOther );
	// Overrides.
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual void	PaintballTouch( CBaseEntity *pOther );
	//virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) override;

	virtual bool	IsDeflectable() { return false; }
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			CreateTrails( void );
	virtual bool	HasTeamSkins( void ) { return true; }
	int				DrawModel (int flags);

private:
	CNewParticleEffect *m_pEffect;
#endif

};
#endif // TF_PROJECTILE_PAINTBALL_H
