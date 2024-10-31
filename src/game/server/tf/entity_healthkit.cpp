//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "entity_healthkit.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_MODEL			"models/items/healthkit.mdl"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( "HealthKit.Touch" );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
// If you change anything, consider copying into CTFAmmoPack::PackTouch into Sandwich sub-code
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		Assert( pTFPlayer );

		if ( !pTFPlayer )
			return false;

		float flHealthToAdd = pTFPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()];
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer, flHealthToAdd, mult_health_frompacks);
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer->GetActiveTFWeapon(), flHealthToAdd, mult_health_frompacks_active);
		int iHealthToAdd = Floor2Int(flHealthToAdd);
		int iHealthRestored = 0;

		int iCanOverheal = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFPlayer, iCanOverheal, health_kits_can_overheal_wearer);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFPlayer->GetActiveTFWeapon(), iCanOverheal, health_kits_can_overheal_active);
		int iCannotOverhealUnlessInjured = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFPlayer, iCannotOverhealUnlessInjured, health_kits_cannot_overheal_unless_injured_modifier);
		
		int bitsHealingType = (iCanOverheal > 0 
			&& !(iCannotOverhealUnlessInjured > 0 && pTFPlayer->GetHealth() >= pTFPlayer->GetMaxHealth() )) 
			? HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP : HEAL_NOTIFY;

		// Don't heal the player who owns sandwich and dropped healthkit belongs to them.
		// This change doesn't break stock weapons but should just get new variable for more flexible modding combos
		int iHealInsteadRechargeWearer = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFPlayer, iHealInsteadRechargeWearer, heal_on_owned_health_pickup_no_recharge);

		if ( GetOwnerEntity() && pTFPlayer == GetOwnerEntity() ? iHealInsteadRechargeWearer : true)
		{
			iHealthRestored = pTFPlayer->TakeHealth(iHealthToAdd, bitsHealingType, NULL, false);

			if ( iHealthRestored )
				bSuccess = true;

			// Restore disguise health.
			if ( pTFPlayer->m_Shared.IsDisguised() )
			{
				int iFakeHealthToAdd = pTFPlayer->m_Shared.GetDisguiseMaxHealth() * PackRatios[GetPowerupSize()];
				if ( pTFPlayer->m_Shared.AddDisguiseHealth( iFakeHealthToAdd ) )
					bSuccess = true;
			}

			// Remove any negative conditions whether player got healed or not.
			if ( pTFPlayer->m_Shared.HealNegativeConds() )
				bSuccess = true;
		}

		if ( bSuccess )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			EmitSound( user, entindex(), "HealthKit.Touch" );

			CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
			if (pTFOwner)
			{
				if (!pTFOwner->IsEnemy(pTFPlayer))
				{
					// BONUS DUCKS!
					CTF_GameStats.Event_PlayerHealedOther(pTFOwner, (float)iHealthRestored);

					if (pTFOwner != pTFPlayer)
					{
						CTF_GameStats.Event_PlayerAwardBonusPoints(pTFOwner, pPlayer, 1);
						// Automatic heal thanks voice line
						pTFPlayer->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_THANKS);
					}
				}
				else
				{
					// BONUS QUACKS!
					CTF_GameStats.Event_PlayerLeachedHealth(pTFPlayer, false, (float)iHealthRestored);
				}
			}

			// Show healing to the one who dropped the healthkit.
			if (iHealthRestored && pTFOwner && pTFOwner != pTFPlayer)
			{
				IGameEvent *event_healed = gameeventmanager->CreateEvent("player_healed");

				if (event_healed)
				{
					event_healed->SetInt("patient", pPlayer->GetUserID());
					event_healed->SetInt("healer", pTFOwner->GetUserID());
					event_healed->SetInt("amount", iHealthRestored);
					event_healed->SetInt("class", pTFPlayer->GetPlayerClass()->GetClassIndex());

					gameeventmanager->FireEvent(event_healed);
				}

				// VIP healing achievement.
				if ( pTFPlayer && pTFPlayer->IsVIP() )
				{
					IGameEvent *event_civilian_healed = gameeventmanager->CreateEvent("vip_healed");

					if (event_civilian_healed)
					{
						event_civilian_healed->SetInt("healer", pTFOwner->GetUserID());
						event_civilian_healed->SetInt("amount", iHealthRestored);

						gameeventmanager->FireEvent(event_civilian_healed);
					}
				}
			}

			if (pTFPlayer)
			{
				string_t strApplyCondOnPickup = NULL_STRING;
				CALL_ATTRIB_HOOK_STRING_ON_OTHER(pTFOwner, strApplyCondOnPickup, apply_cond_on_pickup_wearer);
				if (strApplyCondOnPickup != NULL_STRING)
				{
					float args[3];
					UTIL_StringToFloatArray(args, 3, strApplyCondOnPickup.ToCStr());

					bool bApplyToOwner = (Floor2Int(args[0]) & (1 << 0)) != 0;
					bool bApplyToFriends = (Floor2Int(args[0]) & (1 << 1)) != 0;
					bool bApplyToEnemy = (Floor2Int(args[0]) & (1 << 2)) != 0;
					bool bApplyRespectDisguises = (Floor2Int(args[0]) & (1 << 3)) != 0;

					bool bApply = false;
					if (pTFPlayer == pTFOwner && bApplyToOwner)
					{
						bApply = true;
					}
					else if (pTFPlayer != pTFOwner && (!pTFPlayer->IsEnemy(pTFOwner) ||
						(bApplyRespectDisguises &&  pTFPlayer->m_Shared.DisguiseFoolsTeam(pTFOwner->GetTeamNumber()))) && bApplyToFriends)
					{
						bApply = true;
					}
					else if ((pTFPlayer->IsEnemy(pTFOwner) &&
						(!bApplyRespectDisguises || (bApplyRespectDisguises && !pTFPlayer->m_Shared.DisguiseFoolsTeam(pTFOwner->GetTeamNumber())))) 
						&& bApplyToEnemy)
					{
						bApply = true;
					}

					if (bApply)
					{
						pTFPlayer->m_Shared.AddCond((ETFCond)Floor2Int(args[1]), args[2], pTFOwner);
					}
				}
			}
		}
		else
		{
			if (!iHealInsteadRechargeWearer)
			{
				CTFWeaponBase* pWeapon = pTFPlayer->Weapon_OwnsThisID(TF_WEAPON_LUNCHBOX);
				if(pWeapon)
				{
					int iShouldNotRecharge = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iShouldNotRecharge, heal_on_owned_health_pickup_no_recharge);
					if (!iShouldNotRecharge && pWeapon->GetEffectBarProgress() < 1.0f)
					{
						if (pPlayer->GiveAmmo(1, pWeapon->GetPrimaryAmmoType(), false))
						{
							bSuccess = true;
						}
					}
				}
			}

		}
	}

	return bSuccess;
}
