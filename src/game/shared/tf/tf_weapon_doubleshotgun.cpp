//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_doubleshotgun.h" 
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#else
// Client specific.
#include "c_tf_player.h"
#endif

//=============================================================================
//
// Weapon DoubleShotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE(TFDoubleShotgun, tf_weapon_doubleshotgun)

#define TF2C_DOUBLESHOTGUN_TURBO_CAPACITY_VISUAL 15.0f

//=============================================================================
//
// Weapon DoubleShotgun functions.
//

float CTFDoubleShotgun::GetProgress()
{
	float flTurboDuration = max(0, GetWeaponDamageBoostTime() - gpGlobals->curtime);

	// Let's show civ's normal attack boost on turbo charge bar too
	CTFPlayer* pTFOwner = ToTFPlayer(GetOwner());
	if (pTFOwner && pTFOwner->m_Shared.InCond(TF_COND_DAMAGE_BOOST))
	{
		if (pTFOwner->m_Shared.GetConditionDuration(TF_COND_DAMAGE_BOOST) == PERMANENT_CONDITION)
		{
			flTurboDuration = TF2C_DOUBLESHOTGUN_TURBO_CAPACITY_VISUAL;
		}
		else
		{
			flTurboDuration = max(flTurboDuration, pTFOwner->m_Shared.GetConditionDuration(TF_COND_DAMAGE_BOOST));
		}
	}

	return min(flTurboDuration / TF2C_DOUBLESHOTGUN_TURBO_CAPACITY_VISUAL, 1);
}