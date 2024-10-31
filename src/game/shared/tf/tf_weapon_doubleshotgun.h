//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

// Hai, this is actually used by TF2C_ACHIEVEMENT_SSG_SCOUT_HIT_PELLETS achievement, that's all

#ifndef TF_WEAPON_DOUBLESHOTGUN_H
#define TF_WEAPON_DOUBLESHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_shotgun.h"

#ifdef CLIENT_DLL
#define CTFDoubleShotgun C_TFDoubleShotgun
#endif

//=============================================================================
//
// DoubleShotgun weapon class.
//
class CTFDoubleShotgun : public CTFShotgun
{
public:
	DECLARE_CLASS(CTFDoubleShotgun, CTFShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFDoubleShotgun() {}
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_DOUBLESHOTGUN; }

	float			GetProgress(void);
	const char		*GetEffectLabelText(void) { return "#TF_Turbo"; }
private:
	CTFDoubleShotgun(const CTFDoubleShotgun &);
};

#endif // TF_WEAPON_DOUBLESHOTGUN_H
