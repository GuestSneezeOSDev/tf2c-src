//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_AMMO_PACK_H
#define TF_AMMO_PACK_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"

class CTFWeaponBase;

class CTFAmmoPack : public CItem, public TAutoList<CTFAmmoPack>
{
public:
	DECLARE_CLASS( CTFAmmoPack, CItem );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFAmmoPack();

	virtual void Spawn();
	virtual void Precache();

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	int GiveAmmo( int iCount, int iAmmoType );

	static CTFAmmoPack *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTFWeaponBase *pWeapon = NULL, float m_flPackRatio = 0.5 );
	static CTFAmmoPack *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, float m_flPackRatio = 0.5 );

	float GetCreationTime() const { return m_flCreationTime; }
	bool IsActuallyHealth() const { return m_bHealing; }

	void SetPumpKinLoot();

	// CEconEntity
	virtual void SetItem( CEconItemView &newItem );
	CEconItemView *GetItem( void );

private:
	int m_iAmmo[MAX_AMMO_SLOTS];

	float m_flCreationTime;
	bool m_bAllowOwnerPickup;
	bool m_bHealing;
	float m_flPackRatio;

	bool m_bPumpkinLoot;

	CNetworkVar( bool, m_bPilotLight );

	// Mirrors CEconEntity
protected:
	CEconItemView m_Item;

private:
	CTFAmmoPack( const CTFAmmoPack & );
};

#endif //TF_AMMO_PACK_H