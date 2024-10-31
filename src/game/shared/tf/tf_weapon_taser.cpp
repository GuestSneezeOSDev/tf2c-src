//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_taser.h"
#include "IEffects.h"
#include "tf_lagcompensation.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "prediction.h"
	#include "particles_simple.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "tf_gamestats.h"
	#include "tf_fx.h"
	#include "achievements_tf.h"
#endif

//=============================================================================
//
// Weapon Taser tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFTaser, tf_weapon_taser )

//=============================================================================
//
// Weapon Taser functions.
//


CTFTaser::CTFTaser()
{
}


void CTFTaser::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Taser.Spark" );
	PrecacheScriptSound( "Weapon_Taser.Heal" );

	PrecacheTeamParticles( "medic_taser_PARENT_%s" );
}


void CTFTaser::Smack( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	bool bPlaySound = true;
#ifdef CLIENT_DLL
	bPlaySound = prediction->IsFirstTimePredicted();
#endif

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	float flElectricCharge = GetEffectBarProgress();
	bool bDrainElectricCharge = false;

	// See if we hit anything.
	trace_t trace;
	if ( DoSwingTrace( trace, true ) )
	{
		CTFPlayer *pTarget = ToTFPlayer( trace.m_pEnt );

		// Prevent potential self-healing exploit
		if ( pTarget == pPlayer )
			return;

		CDisablePredictionFiltering disabler;

		// Spark player bodies or hit point
		if ( pTarget )
		{
			g_pEffects->Sparks( pTarget->WorldSpaceCenter() );
		}
		else
		{
			// Drain charge if we hit an enemy building.
			if ( trace.m_pEnt->IsBaseObject() && pPlayer->IsEnemy( trace.m_pEnt ) )
			{
				bDrainElectricCharge = true;
			}

			g_pEffects->Sparks( trace.endpos );
		}

		if ( bPlaySound )
		{
			EmitSound( "Weapon_Taser.Spark" );
		}

		// Heal teammates or Spies disguised as our team if cooldown is ready
		if ( pTarget )
		{
			bDrainElectricCharge = true;

			if ( !pPlayer->IsEnemy( pTarget ) || pTarget->m_Shared.DisguiseFoolsTeam(pPlayer->GetTeamNumber()) )
			{
				if ( flElectricCharge >= 1.0f )
				{
					if ( bPlaySound )
					{
						EmitSound( "Weapon_Taser.Heal" );
					}

#ifdef CLIENT_DLL
					if (prediction->IsFirstTimePredicted())
					{
						const char* pszEffect = ConstructTeamParticle("medic_taser_PARENT_%s", GetTeamNumber());
						pTarget->ParticleProp()->Create(pszEffect, PATTACH_ABSORIGIN_FOLLOW, 0, Vector(0, 0, 50));
					}
#endif

#ifdef GAME_DLL
					// ACHIEVEMENT_TF2C_EXTREME_SHOCK_THERAPY_TARGET_HEALTH
					/*if (pTarget->GetHealth() < ACHIEVEMENT_TF2C_EXTREME_SHOCK_THERAPY_TARGET_HEALTH && pTarget->m_Shared.InCond(TF_COND_BURNING))
					{
						pPlayer->AwardAchievement(TF2C_ACHIEVEMENT_EXTREME_SHOCK_THERAPY);
					}*/

					int iHealthToAdd = Min( pTarget->m_Shared.GetMaxBuffedHealth() - pTarget->GetHealth(), pTarget->GetMaxHealth() );
					CALL_ATTRIB_HOOK_INT( iHealthToAdd, mult_taser_heal_scale );
					//DevMsg( "iHealthToAdd: %i\n", iHealthToAdd );
					int iHealthRestored = pTarget->TakeHealth( iHealthToAdd, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP, pPlayer );
					pTarget->m_Shared.HealNegativeConds();

					// Restore disguise health.
					int iDisguisedHealthRestored = 0;
					if (pTarget->m_Shared.IsDisguised())
					{
						int iDisguisedHealthToAdd = Min(pTarget->m_Shared.GetDisguiseMaxBuffedHealth() - pTarget->m_Shared.GetDisguiseHealth(), pTarget->m_Shared.GetDisguiseMaxHealth());
						CALL_ATTRIB_HOOK_INT( iDisguisedHealthToAdd, mult_taser_heal_scale );
						iDisguisedHealthRestored = pTarget->m_Shared.AddDisguiseHealth(iDisguisedHealthToAdd, true);
					}

					if (!pPlayer->IsEnemy(pTarget))
					{
						CTF_GameStats.Event_PlayerHealedOther(pPlayer, (float)iHealthRestored);
					}
					else
					{
						CTF_GameStats.Event_PlayerLeachedHealth(pTarget, false, (float)iHealthRestored);
					}

					IGameEvent* event_healed = gameeventmanager->CreateEvent("player_healed");
					if (event_healed)
					{
						event_healed->SetInt("patient", pTarget->GetUserID());
						event_healed->SetInt("healer", pPlayer->GetUserID());
						event_healed->SetInt("amount", pTarget->m_Shared.IsDisguised() ? iDisguisedHealthRestored : iHealthRestored);
						//event_healed->SetInt("class", pTarget->m_Shared.IsDisguised() ? pTarget->m_Shared.GetDisguiseClass() : pTarget->GetPlayerClass()->GetClassIndex());

						gameeventmanager->FireEvent(event_healed);
					}

					// VIP healing achievement.
					if ( pTarget->IsVIP() )
					{
						IGameEvent *event_civilian_healed = gameeventmanager->CreateEvent( "vip_healed" );

						if ( event_civilian_healed )
						{
							event_civilian_healed->SetInt( "healer", pPlayer->GetUserID() );
							event_civilian_healed->SetInt( "amount", iHealthRestored);

							gameeventmanager->FireEvent( event_civilian_healed );
						}
					}

					//This would need reworking to deliver a set amount instead of only stopping when full
					//pTarget->m_Shared.Heal( pPlayer, iHealthToAdd, NULL, true );
#endif
				}
				else
				{
					bDrainElectricCharge = false;
				}
			}
		}
	}

	FINISH_LAG_COMPENSATION();

	BaseClass::Smack();

	// Shocking aftermath!
	// This is down here because CTFTaser::GetMeleeDamage needs to the progress before it gets reset!
	if ( bDrainElectricCharge )
	{
		if ( flElectricCharge >= 1.0f )
		{
			pPlayer->RemoveAmmo( 1, GetEffectBarAmmo() );
		}

		StartEffectBarRegen();
	}
}


float CTFTaser::GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage )
{
	// Don't damage disguised Spies if we heal them
	if ( GetEffectBarProgress() >= 1.0f )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		CTFPlayer *pTargetPlayer = ToTFPlayer( pTarget );
		if ( pPlayer && pTargetPlayer && pTargetPlayer->m_Shared.DisguiseFoolsTeam( pPlayer->GetTeamNumber() ) )
			return 0.0f;
	}

	float flDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );
	float flScaledDamage = RemapValClamped( GetProgress(), 0.0f, 1.0f, 0.0f, flDamage );

	bool bDontUseScaledDamage = false;
	CALL_ATTRIB_HOOK_INT( bDontUseScaledDamage, mod_taser_damage_noscale );

	return bDontUseScaledDamage ? flDamage : flScaledDamage;
}


void CTFTaser::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// No electrical powers for this doctor.
	if ( pPlayer->IsPlayerUnderwater() )
	{
		if ( GetEffectBarProgress() >= 1.0f )
		{
			pPlayer->RemoveAmmo( 1, GetEffectBarAmmo() );
		}

		StartEffectBarRegen();
	}
}
