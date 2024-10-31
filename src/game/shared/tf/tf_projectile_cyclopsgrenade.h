//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#ifdef TF2C_BETA
#ifndef TF_PROJECTILE_CYCLOPSGRENADE_H
#define TF_PROJECTILE_CYCLOPSGRENADE_H
#ifdef _WIN32
#pragma once
#endif

//#include "tf_weaponbase_grenadeproj.h"
#include "tf_weapon_grenade_pipebomb.h"

// #define CYCLOPSIMPACT

// Client specific.
#ifdef CLIENT_DLL
#define CTFCyclopsGrenadeProjectile C_TFCyclopsGrenadeProjectile
#endif

//=============================================================================
//
// TF Pipebomb Grenade
//
class CTFCyclopsGrenadeProjectile : public CTFGrenadePipebombProjectile
{
public:
	DECLARE_CLASS(CTFCyclopsGrenadeProjectile, CTFGrenadePipebombProjectile);
	DECLARE_NETWORKCLASS();

	CTFCyclopsGrenadeProjectile();
	~CTFCyclopsGrenadeProjectile();

	virtual void UpdateOnRemove(void);

#ifdef GAME_DLL
//	DECLARE_DATADESC();

	// Creation.
	static CTFCyclopsGrenadeProjectile* Create(const Vector& position, const QAngle& angles, const Vector& velocity,
		const AngularImpulse& angVelocity, CBaseEntity* pOwner, CBaseEntity* pWeapon, int iType);

	// Overrides.
	virtual void	Spawn() override;
	virtual void	DetonateThink(void);
	virtual	void	BlipSound(void);
	virtual void	Precache() override;
	virtual int		OnTakeDamage(const CTakeDamageInfo& info);
	void VPhysicsCollision(int index, gamevcollisionevent_t* pEvent);
	bool ShouldExplodeOnEntity(CBaseEntity* pOther);
#ifdef CYCLOPSIMPACT
	virtual void	CyclopsTouch(CBaseEntity* pOther);
	void			ImpactSound(const char* pszSoundName, bool bLoudForAttacker);
#endif
	virtual void	Detonate();
	virtual void	Deflected(CBaseEntity* pDeflectedBy, Vector& vecDir);
	virtual void	Explode(trace_t* pTrace, int bitsDamageType);
	// Unique identifier.
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_CYCLOPS; }
private:
	float	m_flImpactTime;
	bool	m_bTouched;
public:
	ETFDmgCustom	m_iLatestDetonationDamageCustom;
	virtual ETFDmgCustom	GetCyclopsComboDamageCustom(void) { return m_iLatestDetonationDamageCustom; }
#endif

};

#endif
#endif // TF2C_BETA