//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Melee weapon that can accumulate extra damage and uses them all on hit
//
//=============================================================================

#ifndef TF_WEAPON_BEACON_H
#define TF_WEAPON_BEACON_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBeacon C_TFBeacon
#endif

//=============================================================================
//
// Beacon weapon class.
//
class CTFBeacon : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS(CTFBeacon, CTFWeaponBaseMelee);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFBeacon() { m_bUnderwater = false; }
	~CTFBeacon();

	virtual void	WeaponReset(void);

	void			Precache();

	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_BEACON; }

	virtual float		GetMeleeDamage(CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage);	// This one changes resets extra damage when called! Was just easy to inherit

	virtual void	CalcIsAttackCritical(void);
	virtual bool	CalcIsAttackCriticalHelper(void);

	virtual void		AddStoredBeaconCrit(int iAmount);
	virtual int			GetStoredBeaconCrit();
	virtual void		ClearStoredBeaconCrit();
	virtual bool		IsStoredBeaconCritFilled();

	float			GetProgress(void);
	const char		*GetEffectLabelText(void) { return "#TF_StoredCrit"; }

	virtual void		ItemPreFrame( void );
	virtual bool		ShouldSwitchCritBodyGroup( CBaseEntity *pEntity );
	int			GetMaxBeaconTicks(void);

#ifdef CLIENT_DLL
	virtual void	OnDataChanged(DataUpdateType_t type);
	virtual void	OnPreDataChanged(DataUpdateType_t updateType);
	virtual void	ManageHealEffect(void);
	virtual void	ThirdPersonSwitch(bool bThirdperson);
	virtual void	UpdateOnRemove(void);
#endif

protected:
	CNetworkVar(int, m_iStoredBeaconCrit);
	bool m_bCurrentAttackUsesStoredBeaconCrit;

#ifdef CLIENT_DLL
	bool m_bOldStoredBeaconCritFilled;
	CNewParticleEffect *m_pHealEffect;
	EHANDLE				m_hHealEffectHost;
#endif

private:
	CTFBeacon(const CTFBeacon &);
	
	bool	m_bUnderwater;

public:
	CNetworkVar(float, m_flLastAfterburnTime);
};

#endif // TF_WEAPON_BEACON_H
