
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_THROWINGKNIFE_H
#define TF_WEAPON_THROWINGKNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_knife.h"

#ifdef CLIENT_DLL
#define CTFThrowingKnife C_TFThrowingKnife
#endif

extern ConVar tf2c_throwingknife_regen_time;

//=============================================================================
//
// Fists weapon class.
//
class CTFThrowingKnife : public CTFKnife
{
public:
	DECLARE_CLASS(CTFThrowingKnife, CTFKnife);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFThrowingKnife() {}
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_THROWINGKNIFE; }

	virtual float		GetMeleeDamage(CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage);

	virtual bool	HasPrimaryAmmo() { return BaseClass::BaseClass::BaseClass::HasPrimaryAmmo(); }
	virtual bool	CanBeSelected() { return BaseClass::BaseClass::BaseClass::HasPrimaryAmmo(); }

	virtual void		PrimaryAttack(void);
	virtual void		Smack();

	virtual CBaseEntity*	ThrowKnife(CTFPlayer *pPlayer);

	virtual CBaseEntity*	FireKnifeProjectile(CTFPlayer *pPlayer);

	virtual bool		IsBehindAndFacingTarget(CBaseEntity *pTarget);

	virtual void		GetProjectileFireSetup(CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bDoPassthroughCheck = false);


	float			GetProgress(void) { return GetEffectBarProgress(); }
	const char		*GetEffectLabelText(void) { return "#TF_ThrowingKnife"; }

	virtual float	InternalGetEffectBarRechargeTime(void) { return tf2c_throwingknife_regen_time.GetFloat(); }
private:
	CTFThrowingKnife(const CTFThrowingKnife &);
};

#endif // TF_WEAPON_THROWINGKNIFE_H