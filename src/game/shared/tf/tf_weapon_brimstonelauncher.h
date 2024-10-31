//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BRIMSTONELAUNCHER_H
#define TF_WEAPON_BRIMSTONELAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
#define CTFBrimstoneLauncher C_TFBrimstoneLauncher
#endif

//=============================================================================
//
// Brimstone weapon class.
//
class CTFBrimstoneLauncher : public CTFGrenadeLauncher
{
public:
	DECLARE_CLASS(CTFBrimstoneLauncher, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFBrimstoneLauncher();
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_BRIMSTONELAUNCHER; }

	void				Precache(); //part of particles copypaste

	void				WeaponReset(void);

#ifdef CLIENT_DLL
	virtual float		GetProgress( void ) override;
	const char			*GetEffectLabelText(void) { return "SLAMFIRE"; }
#endif

	virtual void		ItemPostFrame( void ) override;
	virtual void		ItemBusyFrame( void ) override;
	virtual void		ItemHolsterFrame( void ) override;
	void				ChargeThink( void );
	void				OnDamage( float flDamage );
	float				GetFireRate( void );
	void				ActivateCharge( void );
	bool				ReloadOrSwitchWeapons( void );
	bool				Reload( void );
	void				PlayWeaponShootSound( void );
	bool				IsChargeActive( void );

	// hack
#ifdef CLIENT_DLL
	virtual void		OnDataChanged(DataUpdateType_t type);
	virtual void		UpdateOnRemove(void);
#endif
protected:
	CNetworkVar( float, m_flChargeMeter );
	CNetworkVar( bool,	m_bIsChargeActive );
private:
	CTFBrimstoneLauncher(const CTFBrimstoneLauncher &);
	//hack
#ifdef CLIENT_DLL
	virtual void		StartBurningEffect(void);
	virtual void		StopBurningEffect(void);
	virtual void		ThirdPersonSwitch(bool bThirdperson);

	EHANDLE				m_hParticleEffectOwner;
	HPARTICLEFFECT		m_pBurningArrowEffect;
#endif 
};

#endif // TF_WEAPON_BRIMSTONE_H