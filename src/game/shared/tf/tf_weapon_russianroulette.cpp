//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: This weapon base only exists to make the progress bar of stored crits show up properly!
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_russianroulette.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif
//=============================================================================
//
// Weapon Russian Roulette tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFRussianRoulette, tf_weapon_russianroulette )

//=============================================================================
//
// Weapon Russian Roulette functions.
//


int CTFRussianRoulette::GetCount(void)
{
	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return -1;
	
	return pPlayer->m_Shared.GetStoredCrits();
}