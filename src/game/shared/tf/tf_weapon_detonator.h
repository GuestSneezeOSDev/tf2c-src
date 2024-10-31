
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//=============================================================================

#ifndef TF_WEAPON_DETONATOR_H
#define TF_WEAPON_DETONATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFDetonator C_TFDetonator
#endif

//=============================================================================
//
// 
//

class CTFDetonator : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFDetonator, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFDetonator() {};
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_DETONATOR; }

	virtual void	Precache();

	virtual void	PrimaryAttack();

	virtual void	Smack( void );

	virtual void	WeaponReset(void);

//protected: !!! foxysen detonator
	CNetworkVar(bool, m_bKilledSomeone);
private:
	CTFDetonator(const CTFDetonator &) {}

};

#endif // TF_WEAPON_DETONATOR_H