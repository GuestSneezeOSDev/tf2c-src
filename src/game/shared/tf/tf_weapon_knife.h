//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_KNIFE_H
#define TF_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFKnife C_TFKnife
#endif

//=============================================================================
//
// Knife class.
//
class CTFKnife : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFKnife, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKnife();

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_KNIFE; }

	virtual bool		Deploy( void );
    virtual void		PrimaryAttack(void);

    virtual float		GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage );

	virtual void		ItemPreFrame( void );
	virtual void		ItemPostFrame( void );

	virtual void		SendPlayerAnimEvent( CTFPlayer *pPlayer );

	virtual bool		IsBehindAndFacingTarget( CBaseEntity *pTarget );

	virtual bool		CalcIsAttackCriticalHelper( void );

	virtual void		DoViewModelAnimation( void );
	virtual bool		SendWeaponAnim( int iActivity );

	void				BackstabVMThink( void );

	virtual bool		ShouldSwitchCritBodyGroup( CBaseEntity *pEntity );

	bool				IsReadyToBackstab( void ) const	{ return m_bReadyToBackstab; }

#ifdef GAME_DLL
	virtual bool TFBot_IsQuietWeapon() OVERRIDE { return true; }
#endif

protected:
	EHANDLE				m_hBackstabVictim;
	CNetworkVar(bool, m_bReadyToBackstab);
private:

	bool				m_bUnderwater;
	CTFKnife( const CTFKnife & ) {}
};

#endif // TF_WEAPON_KNIFE_H
