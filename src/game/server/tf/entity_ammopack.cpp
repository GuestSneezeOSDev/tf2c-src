//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "entity_ammopack.h"
#include "tf_shareddefs.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf2c_spy_cloak_ammo_refill;

//=============================================================================
//
// CTF AmmoPack defines.
//

LINK_ENTITY_TO_CLASS( item_ammopack_full, CAmmoPack );
LINK_ENTITY_TO_CLASS( item_ammopack_small, CAmmoPackSmall );
LINK_ENTITY_TO_CLASS( item_ammopack_medium, CAmmoPackMedium );

//=============================================================================
//
// CTF AmmoPack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CAmmoPack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		float flCannotPickupAmmoPackWhileCloaked = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer, flCannotPickupAmmoPackWhileCloaked, cannot_pickup_ammo_packs_while_cloaked);
		if (!(flCannotPickupAmmoPackWhileCloaked && pTFPlayer->m_Shared.GetPercentInvisible() >= flCannotPickupAmmoPackWhileCloaked && 
			!(pTFPlayer->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF) || pTFPlayer->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING))))	// User buff invisibility doesn't count!
		{
			for (int iAmmo = TF_AMMO_PRIMARY; iAmmo < TF_AMMO_GRENADES1; iAmmo++)
			{
				// Full ammo packs re-fill all ammo and account for fired clip.
				int iMax = pTFPlayer->GetMaxAmmo(iAmmo, GetPowerupSize() == POWERUP_FULL);

				float flPackRatio = PackRatios[GetPowerupSize()];
				if (iAmmo == TF_AMMO_PRIMARY)
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer, flPackRatio, mult_primary_ammo_from_ammo_packs);
				}
				else if (iAmmo == TF_AMMO_SECONDARY)
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer, flPackRatio, mult_secondary_ammo_from_ammo_packs);
				}

				if (pTFPlayer->GiveAmmo(Ceil2Int(iMax * flPackRatio), iAmmo, true, TF_AMMO_SOURCE_AMMOPACK))
				{
					bSuccess = true;
				}
			}

			if (tf2c_spy_cloak_ammo_refill.GetBool())
			{
				if (pTFPlayer->m_Shared.AddSpyCloak(100.0f * PackRatios[GetPowerupSize()]) > 0.0f)
				{
					bSuccess = true;
				}
			}

			// did we give them anything?
			if (bSuccess)
			{
				CSingleUserRecipientFilter filter(pPlayer);
				EmitSound(filter, entindex(), TF_AMMOPACK_PICKUP_SOUND);


				IGameEvent* event_picked = gameeventmanager->CreateEvent("player_picked_ammo");

				if (event_picked)
				{
					event_picked->SetInt("userid", pPlayer->GetUserID());

					gameeventmanager->FireEvent(event_picked);
				}
			}
		}
	}

	return bSuccess;
}
