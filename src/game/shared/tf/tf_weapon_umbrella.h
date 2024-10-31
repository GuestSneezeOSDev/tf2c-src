//====== Copyright ? 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_UMBRELLA_H
#define TF_WEAPON_UMBRELLA_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#define CTFUmbrella C_TFUmbrella
#endif

//=============================================================================
//
// 'Brella class.
//
class CTFUmbrella : public CTFWeaponBaseMelee
{
	DECLARE_CLASS( CTFUmbrella, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

public:
	CTFUmbrella();
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_UMBRELLA; }

#ifdef GAME_DLL
	virtual void		SecondaryAttack( void );
#endif
	virtual void		WeaponReset( void );

	float				GetProgress( void );
	const char			*GetEffectLabelText( void ) { return "#TF_Boost"; }

	float				GetNextBoostAttack( void) { return m_flNextBoostAttack; }

	float				m_flNextNoTargetTime;

private:
	CTFUmbrella( const CTFUmbrella & );

	CNetworkVar( float, m_flNextBoostAttack );

};

#endif // TF_WEAPON_UMBRELLA_H