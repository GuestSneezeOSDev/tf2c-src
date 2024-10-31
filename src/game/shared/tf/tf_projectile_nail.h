//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_NAIL_H
#define TF_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_nail.h"

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CTFProjectile_Syringe : public CTFBaseNail
{
	DECLARE_CLASS( CTFProjectile_Syringe, CTFBaseNail );

public:
	// Creation.
	static CTFProjectile_Syringe *Create( int iType, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, bool bCritical = false );

#ifdef GAME_DLL
	virtual ETFWeaponID GetWeaponID( void ) const;
#endif
};
#endif // TF_PROJECTILE_NAIL_H