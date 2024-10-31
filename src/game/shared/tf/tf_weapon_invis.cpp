//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_invis.h"
#include "in_buttons.h"

#if !defined( CLIENT_DLL )
#include "vguiscreen.h"
#include "tf_player.h"
#include "achievements_tf.h"
#include "particle_parse.h"
#else
#include "c_tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_spy_invis_unstealth_time;
extern ConVar tf_spy_cloak_consume_rate;
extern ConVar tf_spy_cloak_regen_rate;

//=============================================================================
//
// TFWeaponBase Melee tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFWeaponInvis, tf_weapon_invis )

#define CLOAKED_DAMAGE_SOUNDSCRIPT	"Weapon_Pomson.DrainedVictim"
#define CLOAKED_DAMAGE_SOUNDSCRIPT_ATTACKER	"Weapon_Pomson.DrainedVictim_Attacker"


CTFWeaponInvis::CTFWeaponInvis()
{
#ifdef CLIENT_DLL
	ListenForGameEvent( "player_cloak_drained" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Use the offhand view model
//-----------------------------------------------------------------------------
void CTFWeaponInvis::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( 1 );
}


void CTFWeaponInvis::Precache( void )
{
	BaseClass::Precache();

	PrecacheParticleSystem("speedwatch_damageW_parent");
	PrecacheParticleSystem("speedwatch_damageV_parent");
	PrecacheScriptSound( CLOAKED_DAMAGE_SOUNDSCRIPT );
	PrecacheScriptSound( CLOAKED_DAMAGE_SOUNDSCRIPT_ATTACKER );
}


void CTFWeaponInvis::OnActiveStateChanged( int iOldState )
{
	BaseClass::OnActiveStateChanged( iOldState );

	// If we are being removed, we need to remove all stealth effects from our owner!
	if ( m_iState == WEAPON_NOT_CARRIED && iOldState != WEAPON_NOT_CARRIED )
	{
		CleanupInvisibilityWatch();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the view model when we hide.
//-----------------------------------------------------------------------------
void CTFWeaponInvis::HideThink( void )
{ 
	SetWeaponVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetWeaponVisible( bool bVisible )
{
	CBaseViewModel *pViewModel = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		pViewModel = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( bVisible )
	{
		RemoveEffects( EF_NODRAW );
		if ( pViewModel )
		{
			pViewModel->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( pViewModel )
		{
			pViewModel->AddEffects( EF_NODRAW );
		}
	}
}


bool CTFWeaponInvis::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		SetWeaponIdleTime( gpGlobals->curtime + 1.5f );
		return true;
	}

	return false;
}


bool CTFWeaponInvis::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
		// Distant future of 20XX.
		SetWeaponIdleTime( gpGlobals->curtime + 10.0f );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Our owner activated the functionality of this invisibility watch.
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::ActivateInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	SetCloakRates();

	bool bDoSkill = false;

	float flMinCloakToActivate = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT(flMinCloakToActivate, mod_min_cloak_to_activate);

	if ( pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		// De-cloak.
		float flDecloakRate = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDecloakRate, mult_decloak_rate );
		if ( flDecloakRate <= 0.0f )
		{
			flDecloakRate = 1.0f;
		}

		float flLoseCloakOnDecloak = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT(flLoseCloakOnDecloak, lose_cloak_on_decloak);
		if (flLoseCloakOnDecloak > 0.0f)
		{
			pOwner->m_Shared.SetSpyCloakMeter(Max(0.0f, pOwner->m_Shared.GetSpyCloakMeter() - flLoseCloakOnDecloak));
		}

		pOwner->m_Shared.FadeInvis( 1.0f );
	}
	// Must have over 10% cloak to start the standard cloaking process.
	else if ( pOwner->CanGoInvisible() && 
				((flMinCloakToActivate && pOwner->m_Shared.GetSpyCloakMeter() >= flMinCloakToActivate) || 
				(!flMinCloakToActivate && pOwner->m_Shared.GetSpyCloakMeter() > 8.0f)))
	{
		pOwner->m_Shared.AddCond( TF_COND_STEALTHED, -1.0f, pOwner );


		// !!! foxysen achievement
#ifdef GAME_DLL
		IGameEvent *event_cloaked = gameeventmanager->CreateEvent("player_cloaked");
		if (event_cloaked)
		{
			event_cloaked->SetInt("userid", pOwner->GetUserID());
			gameeventmanager->FireEvent(event_cloaked);
		}
#endif

		bDoSkill = true;
	}

	if ( bDoSkill )
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.5f );
	}
	else
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.1f );
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: Our owner might've switching to another watch. :(
//-----------------------------------------------------------------------------
void CTFWeaponInvis::CleanupInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->m_Shared.IsStealthed() )
	{
		// De-cloak.
		pOwner->m_Shared.FadeInvis( 1.0f );
	}

	pOwner->HolsterOffHandWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Set the correct cloak consume & regen rates for this item.
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetCloakRates( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		float fCloakConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
		float fCloakConsumeFactor = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, fCloakConsumeFactor, mult_cloak_meter_consume_rate );
	
		// This value is an inverse scale.
		// 2 - 0.75 = 1.25... 1 / 1.25 = 0.8.. Consume rate "8". 10s / 0.8 = 12.5s
		if ( fCloakConsumeFactor < 1.0f )
		{
			fCloakConsumeFactor = 1.0f / ( 2.0f - fCloakConsumeFactor );
		}
	
		pOwner->m_Shared.SetCloakConsumeRate( fCloakConsumeRate * fCloakConsumeFactor );

		float fCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();
		float fCloakRegenFactor = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( fCloakRegenFactor, mult_cloak_meter_regen_rate );
		pOwner->m_Shared.SetCloakRegenRate( fCloakRegenRate * fCloakRegenFactor );
	}
}


float CTFWeaponInvis::OwnerTakeDamage( const CTakeDamageInfo &info )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && info.GetDamage() > 0.0f && !pOwner->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB) )
	{
		float flCloakLoss = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( flCloakLoss, lose_cloak_on_damage );
		if ( flCloakLoss > 0.0f )
		{
			int iNoDOTModifier = 0;
			CALL_ATTRIB_HOOK_INT(iNoDOTModifier, attributes_no_DOT_trigger_modifier);
			// FIXME: Is the latter part of this check necessary?
			if ( (pOwner->m_Shared.IsStealthed() || pOwner->m_Shared.InCond( TF_COND_STEALTHED_BLINK )) && (!iNoDOTModifier || !IsDOTDmg(info)))
			{
				pOwner->m_Shared.SetSpyCloakMeter( Max( 0.0f, pOwner->m_Shared.GetSpyCloakMeter() - flCloakLoss ) );

				EmitSound_t params;
				params.m_SoundLevel = SNDLVL_NORM;
				params.m_pSoundName = CLOAKED_DAMAGE_SOUNDSCRIPT;
				params.m_flVolume = 0.5f;
				params.m_nFlags = SND_CHANGE_VOL;

				CSingleUserRecipientFilter filter( pOwner );
				CBaseEntity::StopSound( pOwner->entindex(), params.m_pSoundName );
				CBaseEntity::EmitSound( filter, pOwner->entindex(), params );
#ifdef GAME_DLL
				IGameEvent *event_cloak_drained = gameeventmanager->CreateEvent( "player_cloak_drained" );
				if ( event_cloak_drained )
				{
					event_cloak_drained->SetInt( "userid", pOwner->entindex() );
					gameeventmanager->FireEvent( event_cloak_drained );
				}

				CTFPlayer* pTFAttacker = ToTFPlayer(info.GetAttacker());
				if (pTFAttacker)
				{
					EmitSound_t paramsAttacker;
					paramsAttacker.m_pSoundName = CLOAKED_DAMAGE_SOUNDSCRIPT_ATTACKER;
					CSingleUserRecipientFilter filterAttacker(pTFAttacker);
					pOwner->EmitSound(filterAttacker, pOwner->entindex(), paramsAttacker);
				}

				//TF2C_ACHIEVEMENT_SPEEDWATCH_COUNTER
				if (pOwner->m_Shared.GetSpyCloakMeter() <= 0.0f && pTFAttacker && pOwner->IsAlive())
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_SPEEDWATCH_COUNTER);
#endif
			}
		}
	}

	return BaseClass::OwnerTakeDamage( info );
}


float CTFWeaponInvis::GetProgress( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
		return pOwner->m_Shared.GetSpyCloakMeter() / 100.0f;

	return 1.0f;
}

#ifdef CLIENT_DLL
void CTFWeaponInvis::FireGameEvent( IGameEvent* pEvent )
{
	if ( !strcmp( "player_cloak_drained", pEvent->GetName() ) )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner && pEvent->GetInt( "userid" ) == pOwner->entindex() )
		{
			if( pOwner->ShouldDrawThisPlayer() )
			{
				DispatchParticleEffect( "speedwatch_damageW_parent", PATTACH_ABSORIGIN_FOLLOW, pOwner );
			}
			else
			{
				C_BaseEntity* pEffectOwner = pOwner->GetViewModel(m_nViewModelIndex); 
				if ( pEffectOwner )
				{
					DispatchParticleEffect( "speedwatch_damageV_parent", PATTACH_ABSORIGIN_FOLLOW, pEffectOwner );
				}
			}			
		}
	}
}
#endif

#ifndef CLIENT_DLL

void CTFWeaponInvis::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	int iWatch = 0;
	CALL_ATTRIB_HOOK_INT( iWatch, set_watch_panel );
	switch ( iWatch )
	{
		case 0:
			pPanelName = "pda_panel_spy_invis";
			break;
		case 1:
			pPanelName = "pda_panel_spy_invis_pocket";
			break;
	}
}
#endif