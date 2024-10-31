//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Base Projectile
//
//=============================================================================
#ifndef TF_WEAPONBASE_NAIL_H
#define TF_WEAPONBASE_NAIL_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_baseprojectile.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
#define CTFBaseNail C_TFBaseNail
#endif

//=============================================================================
//
// Nail projectile
//
class CTFBaseNail : public CTFBaseProjectile
{
public:
	DECLARE_CLASS( CTFBaseNail, CTFBaseProjectile );

protected:
	static CTFBaseNail *Create( const char *pszClassname, const Vector &vecOrigin, 
		const QAngle &vecAngles, CBaseEntity *pOwner, int iType, bool bCritical );

#ifdef GAME_DLL
public:
	DECLARE_DATADESC();

	CTFBaseNail();
	~CTFBaseNail();

	void			Precache( void );
	void			Spawn( void );
	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual int		GetBaseProjectileType( void ) const { return TF_PROJECTILE_BASE_NAIL; }

	bool			IsCritical( void )				{ return m_bCritical; }
	void			SetCritical( bool bCritical )	{ m_bCritical = bCritical; }


	void			AdjustDamageDirection(const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt);

	void			ProjectileTouch( CBaseEntity *pOther );

	float			GetDamage() { return m_flDamage; }
	void			SetDamage( float flDamage ) { m_flDamage = flDamage; }

	Vector			GetDamageForce( void );
	virtual int		GetDamageType( void ) const;

private:
	void			FlyThink( void );

protected:
	bool			m_bCritical;
	float			m_flDamage;
	int				m_iType;
#endif
};

#endif // TF_WEAPONBASE_NAIL_H
