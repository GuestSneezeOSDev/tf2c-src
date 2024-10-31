//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_lunchbox.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "entity_healthkit.h"
#include "tf_gamestats.h"
#include "achievements_tf.h"
#include <particle_parse.h>
#else
#include "c_tf_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFLunchBox, tf_weapon_lunchbox );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFLunchBox )
END_DATADESC()
#endif

#define LUNCHBOX_DROP_MODEL			"models/items/plate.mdl"

#define LUNCHBOX_DROPPED_MINS	Vector( -17, -17, -10 )
#define LUNCHBOX_DROPPED_MAXS	Vector( 17, 17, 10 )

static const char *s_pszLunchboxMaxHealThink = "LunchboxMaxHealThink";

//=============================================================================
//
// Weapon Lunchbox functions.
//

ConVar tf2c_sandvich_old( "tf2c_sandvich_old", "1", FCVAR_REPLICATED, "Use old Sandvich behavior (on taunt: +120HP but no recharge)" );

CTFLunchBox::CTFLunchBox()
{
}


void CTFLunchBox::UpdateOnRemove( void )
{
#ifndef CLIENT_DLL
	// If we're removed, we also remove any dropped powerups.
	if ( m_hThrownPowerup )
	{
		UTIL_Remove( m_hThrownPowerup );
	}
#endif

	BaseClass::UpdateOnRemove();
}


void CTFLunchBox::Precache( void )
{
	if ( DropAllowed() )
	{
		PrecacheModel( "models/items/medkit_medium.mdl" );
		PrecacheModel( "models/items/medkit_medium_bday.mdl" );
		PrecacheTeamParticles("critgun_weaponmodel_%s", g_aTeamNamesShort);
		PrecacheModel( LUNCHBOX_DROP_MODEL );
	}

	BaseClass::Precache();
}

#ifdef GAME_DLL

void CTFLunchBox::WeaponRegenerate( void )
{
	BaseClass::WeaponRegenerate();

	// Reset the "eaten" bodygroup.
	SetBodygroup( 0, 0 );
}
#endif


bool CTFLunchBox::UsesPrimaryAmmo( void )
{
	return CBaseCombatWeapon::UsesPrimaryAmmo();
}


bool CTFLunchBox::DropAllowed( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
			return false;
	}

	return true;
}


void CTFLunchBox::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !HasAmmo() )
		return;

#if GAME_DLL
	pOwner->Taunt();
	m_flNextPrimaryAttack = pOwner->GetTauntRemoveTime() + 0.1f;
#else
	m_flNextPrimaryAttack = gpGlobals->curtime + 2.0f;
#endif
}


void CTFLunchBox::SecondaryAttack( void )
{
	if ( !DropAllowed() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !HasAmmo() )
		return;

	bool bIsCritboosted = IsCritboostedLunchbox( pPlayer );

#ifndef CLIENT_DLL
	if ( m_hThrownPowerup )
	{
		UTIL_Remove( m_hThrownPowerup );
	}

	// Throw out the medikit
	Vector vecSrc = pPlayer->EyePosition() + Vector( 0, 0, -8 );
	QAngle angForward = pPlayer->EyeAngles() + QAngle( -10, 0, 0 );

	// Switch this string out with whatever entity you may want to spawn.
	const char *pszHealthKit = "item_healthkit_medium";
	if ( bIsCritboosted )
	{
		pszHealthKit = "item_healthkit_full";
	}

	int iThrowableType = 0;
	CALL_ATTRIB_HOOK_INT( iThrowableType, custom_lunchbox_throwable_type );
	switch ( iThrowableType )
	{
		case 1:
		{
			pszHealthKit = "item_healthkit_small";
			break;
		}
		case 3:
		{
			pszHealthKit = "item_healthkit_full";
			break;
		}
	}

	CHealthKit *pMedKit = assert_cast<CHealthKit*>( CBaseEntity::Create( pszHealthKit, vecSrc, angForward, pPlayer) );
	if ( pMedKit )
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( angForward, &vecForward, &vecRight, &vecUp );
		float flVelocity = 500.0f;
		CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_lunchbox_throwable_velocity );
		Vector vecVelocity = vecForward * flVelocity;

		pMedKit->SetAbsAngles( vec3_angle );
		pMedKit->SetSize( LUNCHBOX_DROPPED_MINS, LUNCHBOX_DROPPED_MAXS );

		// The one that threw this has to wait 0.3 to pickup the powerup (so he can't immediately pick it up while running forward).
		pMedKit->DropSingleInstance( vecVelocity, pPlayer, 0.3, 0.1f );

		// Changes this if you want some other model.
		string_t strLunchboxModel = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING( strLunchboxModel, custom_lunchbox_throwable_model );

		if( strLunchboxModel != NULL_STRING )
			pMedKit->SetModel( STRING(strLunchboxModel) );
		else
			pMedKit->SetModel( LUNCHBOX_DROP_MODEL );

		if( bIsCritboosted )
		{
			const char* pszEffect = ConstructTeamParticle("critgun_weaponmodel_%s", pPlayer->GetTeamNumber(), g_aTeamNamesShort);
			DispatchParticleEffect( pszEffect, PATTACH_ABSORIGIN_FOLLOW, pMedKit );
		}
	}

	m_hThrownPowerup = pMedKit;
#endif

	pPlayer->RemoveAmmo( GetTFWeaponInfo( GetWeaponID() )->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, GetEffectBarAmmo() );
	pPlayer->SwitchToNextBestWeapon( this );
	if ( bIsCritboosted )
	{
		pPlayer->m_Shared.RemoveStoredCrits(1, true);
	}

	StartEffectBarRegen();
}


void CTFLunchBox::DrainAmmo( bool bForceCooldown )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// The old Sandvich doesn't cause a regen.
	bool bForce = false;
	CALL_ATTRIB_HOOK_INT( bForce, lunchbox_force_cooldown );
	if ( bForce )
		bForceCooldown = true;

	if ( tf2c_sandvich_old.GetBool() && !bForce )
		return;

#ifdef GAME_DLL
	// If we're damaged while eating/taunting, bForceCooldown will be true
	if ( pOwner->GetHealth() < pOwner->GetMaxHealth() || bForceCooldown )
	{
		StartEffectBarRegen();
	}
	else
	{	
		// I CAN EAT FOREVER
		return;
	}

	pOwner->RemoveAmmo( 1, GetEffectBarAmmo() );
#else
	pOwner->RemoveAmmo( 1, GetEffectBarAmmo() );
	StartEffectBarRegen();
#endif
}

#ifdef GAME_DLL

void CTFLunchBox::ApplyBiteEffects( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Heal the player
	int iHeal = tf2c_sandvich_old.GetBool() ? 30 : 75;
	int iHealType = DMG_GENERIC;

	if( IsCritboostedLunchbox(pPlayer) )
	{
		iHeal *= 3;
		iHealType |= (HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP);
	}

	float flHealMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flHealMult, lunchbox_healing_scale );
	iHeal = iHeal * flHealMult;

	int iHealed = pPlayer->TakeHealth( iHeal, iHealType );
	if ( iHealed > 0 )
	{
		CTF_GameStats.Event_PlayerHealedOther( pPlayer, iHealed );
	}

	// Make this appear bitten.
	SetBodygroup( 0, 1 );

	// Add condition if requested by custom attribute.
	string_t strAddCondOnEat = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( pPlayer, strAddCondOnEat, lunchbox_addcond );
	if ( strAddCondOnEat != NULL_STRING )
	{
		float args[2];
		UTIL_StringToFloatArray( args, 2, strAddCondOnEat.ToCStr() );
		DevMsg( "lunchbox_addcond: %2.2f, %2.2f\n", args[0], args[1] );

		pPlayer->m_Shared.AddCond( (ETFCond)Floor2Int( args[0] ), args[1], pPlayer );
	}
}


bool CTFLunchBox::SetupTauntAttack( int &iTauntAttack, float &flTauntAttackTime )
{
	if ( iTauntAttack == TAUNTATK_HEAVY_EAT )
	{
		ApplyBiteEffects( ToTFPlayer( GetOwner() ) );
	}

	iTauntAttack = TAUNTATK_HEAVY_EAT;
	flTauntAttackTime = gpGlobals->curtime + 1.0f;

	return true;
}


float CTFLunchBox::OwnerTakeDamage( const CTakeDamageInfo &info )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

	// The sandwich is hurting us with HEALING POWER...
	if ( pOwner->GetTauntAttackType() == TAUNTATK_HEAVY_EAT && pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		DrainAmmo( true );
	}

	return BaseClass::OwnerTakeDamage( info );
}


void CTFLunchBox::OwnerConditionAdded( ETFCond nCond )
{
	if ( nCond == TF_COND_TAUNTING )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

		// Time to have a meal.
		if ( pOwner->GetTauntAttackType() == TAUNTATK_HEAVY_EAT )
		{
			DrainAmmo();
		}
	}
}


void CTFLunchBox::OwnerConditionRemoved( ETFCond nCond )
{
	if ( nCond == TF_COND_TAUNTING )
	{
		// Speak out their 'eaten food' lines.
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if (!pOwner)
			return;

		if ( pOwner->GetTauntAttackType() == TAUNTATK_HEAVY_EAT )
		{
			if ( IsCritboostedLunchbox(pOwner) )
			{
				pOwner->m_Shared.RemoveStoredCrits(1, true);
			}

			if ( pOwner->IsSpeaking() )
			{
				// The player may technically still be speaking even though the actual VO is over and just 
				// hasn't been cleared yet. We need to force it to end so our next concept can be played.
				CMultiplayer_Expresser *pExpresser = pOwner->GetMultiplayerExpresser();
				if ( pExpresser )
				{
					pExpresser->ForceNotSpeaking();
				}
			}

			pOwner->SpeakConceptIfAllowed( MP_CONCEPT_ATE_FOOD );
		}
	}
}
#endif // GAME_DLL

bool CTFLunchBox::IsCritboostedLunchbox( CTFPlayer *pOwner )
{
	bool bCritboostAttribute = false;
	CALL_ATTRIB_HOOK_INT(bCritboostAttribute, mod_lunchbox_critboostable);
	return bCritboostAttribute ? (pOwner->m_Shared.IsCritBoosted() || pOwner->m_Shared.GetStoredCrits()) : false;
}