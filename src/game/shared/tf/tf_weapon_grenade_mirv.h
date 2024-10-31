//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Mirv Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_MIRV_H
#define TF_WEAPON_GRENADE_MIRV_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"

//=============================================================================
//
// TF Mirv Grenade Projectile and Bombs (Server specific.)
//
#ifdef CLIENT_DLL
#define CTFGrenadeMirvProjectile C_TFGrenadeMirvProjectile
#endif

class CTFGrenadeMirvProjectile : public CTFBaseGrenade
{
public:
	DECLARE_CLASS( CTFGrenadeMirvProjectile, CTFBaseGrenade );
	DECLARE_NETWORKCLASS();

	CTFGrenadeMirvProjectile();
	~CTFGrenadeMirvProjectile();

	// Unique identifier.
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADE_MIRV; }

#ifdef GAME_DLL
	// Creation.
	static CTFGrenadeMirvProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pWeapon );

	// Overrides.
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	BounceSound( void );
	virtual void	Detonate( void );
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );

	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual bool	IsDeflectable( void ) { return !m_bDefused; }
	virtual bool	IsDeflectableSwapTeam( void ) { return false; }

	virtual bool	UseStockSelfDamage(void) { return false; }

	void			BlipSound( void );

#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );

#endif

#ifdef GAME_DLL
	virtual void	VPhysicsCollision(int index, gamevcollisionevent_t* pEvent);
	virtual void	MIRVTouch(CBaseEntity* pOther);
	virtual bool	ShouldExplodeOnEntity(CBaseEntity* pOther);
	void			ImpactSound(const char* pszSoundName, bool bLoudForAttacker = false);

	bool m_bAchievementMainPackAirborne;
#endif
private:
#ifdef GAME_DLL
	bool	m_bPlayedLeadIn;
	bool	m_bDefused;
	bool	m_bTouched;
public:
	virtual void SetDetonatedByCyclops(bool bSet);
	virtual ETFDmgCustom	GetCyclopsComboDamageCustom(void) { return TF_DMG_CYCLOPS_COMBO_MIRV; }
#endif
};

#ifdef CLIENT_DLL
#define CTFGrenadeMirvBomb C_TFGrenadeMirvBomb
#endif

class CTFGrenadeMirvBomb : public CTFBaseGrenade
{
public:
	DECLARE_CLASS( CTFGrenadeMirvBomb, CTFBaseGrenade );
	DECLARE_NETWORKCLASS();

	CTFGrenadeMirvBomb();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADE_MIRVBOMB; }

#ifdef GAME_DLL
	// Creation.
	static CTFGrenadeMirvBomb *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, float timer );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	StopLoopingSounds( void );
	virtual void	BounceSound(void);
	virtual void	Detonate(void);

	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual bool	IsDeflectable( void ) { return GetDetonateTime() < FLT_MAX; }
	virtual bool	IsDeflectableSwapTeam( void ) { return false; }

	virtual bool	UseStockSelfDamage(void) { return false; }

#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			CreateSparks( void );
#endif

#ifdef GAME_DLL
	bool m_bAchievementMainPackAirborne;
public:
	virtual void SetDetonatedByCyclops(bool bSet);
	virtual ETFDmgCustom	GetCyclopsComboDamageCustom(void) { return TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET; }
#endif
};

#endif // TF_WEAPON_GRENADE_MIRV_H
