#ifdef TF2C_BETA
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_ANCHOR_H
#define TF_WEAPON_ANCHOR_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFAnchor C_TFAnchor
#endif

#define ANCHOR_TF2C_AIRBORNE_TIME 1.0f
//=============================================================================
//
// Anchor class.
//
class CTFAnchor : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFAnchor, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFAnchor();
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_ANCHOR; }
	bool				IsOwnerAirborne( void );
	void				ChargeThink( void );
	void				ChargeDrain( void );
	void				FallThink(void);

	void				Precache(void);
	// vm animation
	void				ViewmodelThink(void);
	virtual void		DoViewModelAnimation(void);
	virtual bool		SendWeaponAnim(int iActivity);

	virtual void		WeaponReset( void );
	virtual void		ItemPostFrame( void );
	virtual void		ItemHolsterFrame( void );
	virtual void		ItemBusyFrame( void );

	virtual float		GetProgress(void)
#ifdef CLIENT_DLL
	override
#endif
	{
		return m_flChargeMeter;
	}

	const char*			GetEffectLabelText(void) { return "EARTHQUAKE"; }

protected:
	CNetworkVar(float, m_flChargeMeter);
	CNetworkVar(bool, m_bIsDraining);
	CNetworkVar(bool, m_bReadyToEarthquake);
	bool m_bSoundPlayed;
private:
	CTFAnchor( const CTFAnchor & );
};

#endif // TF_WEAPON_ANCHOR_H

#endif // TF2C_BETA
