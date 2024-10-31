#ifdef TF2C_BETA
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_LEAPKNIFE_H
#define TF_WEAPON_LEAPKNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_knife.h"

#ifdef CLIENT_DLL
#define CTFLeapKnife C_TFLeapKnife
#endif

//=============================================================================
//
// Knife class.
//
class CTFLeapKnife : public CTFKnife
{
public:

	DECLARE_CLASS( CTFLeapKnife, CTFKnife );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFLeapKnife();

	void Precache();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_LEAPKNIFE; }

    virtual void        ItemHolsterFrame();
    virtual void        LeapThink();			// crouchjump
	virtual void		ItemBusyFrame();
	virtual void		ItemPostFrame();

	float			GetProgress(void);
	const char*		GetEffectLabelText(void) { return "LEAP"; }

#ifdef GAME_DLL
	virtual bool TFBot_IsQuietWeapon() OVERRIDE { return true; }
#endif

    void				PlayErrorSoundIfWeShould();
protected:
	CNetworkVar(bool, m_bReadyToBackstab);
	CNetworkVar(float, m_flLeapCooldown);
    float               m_flNextErrorBuzzTime;

	CTFLeapKnife( const CTFLeapKnife& ) {}
};

#endif // TF_WEAPON_KNIFE_H
#endif