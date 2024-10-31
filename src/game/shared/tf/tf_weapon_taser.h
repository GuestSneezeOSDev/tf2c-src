//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Medic's lightning melee, the Shock Therapy
//
//=============================================================================

#ifndef TF_WEAPON_TASER_H
#define TF_WEAPON_TASER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFTaser C_TFTaser
#endif

//=============================================================================
//
// Wrench class.
//

class CTFTaser : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFTaser, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFTaser();
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_TASER; }

	virtual void		Precache( void );
	virtual void		Smack( void );
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage );

	virtual void		ItemPreFrame( void );

	float				GetProgress( void ) { return GetEffectBarProgress(); }
	const char			*GetEffectLabelText( void ) { return "#TF_Rescue"; }

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 30.0f; }

private:
	CTFTaser( const CTFTaser & ) {}

};

#endif // TF_WEAPON_TASER_H
