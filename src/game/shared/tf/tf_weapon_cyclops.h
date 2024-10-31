#ifdef TF2C_BETA

//====== Copyright 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_CYCLOPS_H
#define TF_WEAPON_CYCLOPS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
#define CTFCyclops C_TFCyclops
#endif

//=============================================================================
//
// Brimstone weapon class.
//
class CTFCyclops : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS(CTFCyclops, CTFWeaponBaseGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFCyclops();
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_CYCLOPS; }

	void AddCyclopsGrenade(CBaseEntity* pCyclopsGrenade);
	void RemoveCyclopsGrenade(CBaseEntity* pCyclopsGrenade);
	void PrimaryAttack();
	bool Reload();

	void GiveDefaultAmmo(void) override;

private:
	// This is here so we can network the pipebomb count for prediction purposes
	CNetworkVar(int, m_iCyclopsGrenadeCount);

	// List of active pipebombs
	typedef CHandle<CBaseEntity>	CyclopsGrenadeHandle;
	CUtlVector<CyclopsGrenadeHandle> m_CyclopsGrenades;

	CTFCyclops(const CTFCyclops&);
};

#endif

#endif // TF2C_BETA