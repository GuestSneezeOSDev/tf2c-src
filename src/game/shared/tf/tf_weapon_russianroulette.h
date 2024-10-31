//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

// Hai, this is only used by counter meter on HUD

#ifndef TF_WEAPON_RUSSIANROULETTE_H
#define TF_WEAPON_RUSSIANROULETTE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_fists.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
#define CTFRussianRoulette C_TFRussianRoulette
#endif

//=============================================================================
//
// Fists weapon class.
//
class CTFRussianRoulette : public CTFFists
{
public:
	DECLARE_CLASS(CTFRussianRoulette, CTFFists);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFRussianRoulette() {}
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_RUSSIANROULETTE; }

	int					GetCount(void);
	const char			*GetEffectLabelText(void) { return "#TF_StoredCrits"; }

private:
	CTFRussianRoulette(const CTFRussianRoulette &);
};

#endif // TF_WEAPON_RUSSIANROULETTE_H
