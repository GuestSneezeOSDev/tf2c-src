#ifdef TF2C_BETA
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_PILLSTREAK_H
#define TF_WEAPON_PILLSTREAK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
#define CTFPillstreak C_TFPillstreak
#endif

//=============================================================================
//
// Pillstreak weapon class.
//
class CTFPillstreak : public CTFGrenadeLauncher
{
public:
	DECLARE_CLASS(CTFPillstreak, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFPillstreak() {}
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_PILLSTREAK; }

	void				Precache(); //part of particles copypaste

	void				WeaponReset(void);

	float				GetProjectileDamage(void);

	int					GetCount(void) { return GetPipeStreak(); };
#ifdef CLIENT_DLL
	virtual float		GetProgress( void ) override;
	const char			*GetEffectLabelText(void) { return "#TF_Streak"; }
#endif

	void				IncrementPipeStreak(void);
	void				DecrementPipeStreak(void);
	int					GetPipeStreak(void);
	int					GetMaxDamagePipeStreak(void);
	void				ClearPipeStreak(void);
	virtual void		ItemPostFrame( void ) override;
	virtual void		ItemHolsterFrame( void ) override;
	virtual void		PlayWeaponShootSound( void );
	float				GetProjectileDamageNoStreak( void );

	// hack
#ifdef CLIENT_DLL
	virtual void		OnDataChanged(DataUpdateType_t type);
	virtual void		UpdateOnRemove(void);
#endif
protected:
	CNetworkVar(int, m_iStreakCount);
	//CNetworkVar( float, m_flTimeDecrementStreak );
	CNetworkVar(int, m_iMissCount);
private:
	CTFPillstreak(const CTFPillstreak &);

	//hack
#ifdef CLIENT_DLL
	virtual void		StartBurningEffect(void);
	virtual void		StopBurningEffect(void);
	virtual void		ThirdPersonSwitch(bool bThirdperson);

	EHANDLE		   m_hParticleEffectOwner;
	HPARTICLEFFECT m_pBurningArrowEffect;
#endif 
};

#endif // TF_WEAPON_PILLSTREAK_H

#endif // TF2C_BETA