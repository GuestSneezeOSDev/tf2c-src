//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_HUNTERRIFLE_H
#define TF_WEAPON_HUNTERRIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_sniperrifle.h"

#if defined( CLIENT_DLL )
#define CTFHunterRifle C_TFHunterRifle
#endif

//=============================================================================
//
// Hunter Rifle class.
//
class CTFHunterRifle : public CTFSniperRifle
{
public:

	DECLARE_CLASS( CTFHunterRifle, CTFSniperRifle );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFHunterRifle() {}

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_HUNTERRIFLE; }

	virtual float OwnerMaxSpeedModifier( void );

	virtual float DeployedPoseHoldTime( void )	{ return 0.6f; }

private:
	CTFHunterRifle( const CTFHunterRifle & );
};

#endif // TF_WEAPON_HUNTERRIFLE_H
