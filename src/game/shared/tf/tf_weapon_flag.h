//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FLAG_H
#define TF_WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "entity_capture_flag.h"

#ifdef CLIENT_DLL
#define CTFFlag C_TFFlag
#endif

//=============================================================================
//
// Flag class.
//
class CTFFlag : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFFlag, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFFlag();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_FLAG; }
	virtual void		SecondaryAttack();
	virtual bool		CanHolster( void ) const;
	virtual const char *GetViewModel( int iViewModel = 0 ) const;

	CCaptureFlag		*GetOwnerFlag( void ) const;

#ifdef CLIENT_DLL
	virtual int			GetSkin( void );
#endif

#ifdef GAME_DLL
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE{ return true; }
#endif

private:
	bool m_bDropping;

	CTFFlag( const CTFFlag & ) {}
};

#endif // TF_WEAPON_FLAG_H
