//=============================================================================//
//
// Purpose: TF2 projectile base.
//
//=============================================================================//
#ifndef TF_BASEPROJECTILE_H
#define TF_BASEPROJECTILE_H

#ifdef _WIN32
#pragma once
#endif

#include "baseprojectile.h"

#ifdef CLIENT_DLL
#define CTFBaseProjectile C_TFBaseProjectile
#endif

class CTFBaseProjectile : public CBaseProjectile
{
public:
	DECLARE_CLASS( CTFBaseProjectile, CBaseProjectile );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	virtual ETFWeaponID		GetWeaponID( void ) const;

	virtual void			Spawn( void );

	// from CBaseEntity
	virtual void			Splash( void );

	virtual bool			UseStockSelfDamage( void ) { return true; }
	virtual float			GetSelfDamageRadius( void );
#else
	virtual bool			HasTeamSkins( void ) { return false; }
	virtual int				GetSkin( void );
#endif
	static const char*		GetProjectileParticleName( const char* strDefaultName, CBaseEntity* m_hLauncher, bool bCritical = false );
	void					SetDeflectedBy(CBaseEntity *pDeflectedBy)		{ m_hDeflectedBy = pDeflectedBy; }
	CBaseEntity				*GetDeflectedBy(void)							{ return m_hDeflectedBy; }
	
	virtual const char		*GetFlightSound()								{ return NULL; }
	virtual void			Precache( void );
	virtual void			StopLoopingSounds( void );

	struct HurtCylinder {
		float radius;
		float height;
		Vector centre;
		float RayIntersect(Vector start, Vector dir)
		{
			float a = (dir.x * dir.x) + (dir.y * dir.y);
			float b = 2 * (dir.x*(start.x - centre.x) + dir.y*(start.y - centre.y));
			float c = (start.x - centre.x) * (start.x - centre.x) + (start.y - centre.y) * (start.y - centre.y) - (radius*radius);

			float delta = b*b - 4 * (a*c);
			if (fabs(delta) < 0.001) return -1.0;
			if (delta < 0.0) return -1.0;

			float t1 = (-b - sqrt(delta)) / (2 * a);
			float t2 = (-b + sqrt(delta)) / (2 * a);
			float t;

			if (t1>t2) t = t2;
			else t = t1;

			float r = start.z + t*dir.z;

			if ((r >= centre.z) && (r <= centre.z + height))return t;
			else return -1;
		}
	};

	virtual bool	IsProjectile(void) const { return true; }
protected:
	CHandle<CBaseEntity>	m_hDeflectedBy;
};

#endif // TF_BASEPROJECTILE_H
