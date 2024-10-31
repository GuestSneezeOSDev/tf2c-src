//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_TRANQ_H
#define TF_WEAPON_TRANQ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFTranq C_TFTranq
#endif

//=============================================================================
//
// TF Weapon Tranq.
//
class CTFTranq : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFTranq, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFTranq() {}

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_TRANQ; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

#ifdef GAME_DLL
	virtual bool TFBot_IsContinuousFireWeapon() OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()          OVERRIDE { return true;  }
#endif

private:
	CTFTranq( const CTFTranq & );
};

#endif // TF_WEAPON_TRANQ_H