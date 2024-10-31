//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_brimstonelauncher.h" 
#include "tf_fx_shared.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif
#include <in_buttons.h>

//=============================================================================
//
// Weapon Brimstone Launcher tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFBrimstoneLauncher, DT_TFBrimstoneLauncher);
BEGIN_NETWORK_TABLE(CTFBrimstoneLauncher, DT_TFBrimstoneLauncher)

#ifdef GAME_DLL
SendPropFloat( SENDINFO( m_flChargeMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
RecvPropFloat( RECVINFO( m_flChargeMeter ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFBrimstoneLauncher)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_brimstonelauncher, CTFBrimstoneLauncher);
PRECACHE_WEAPON_REGISTER(tf_weapon_brimstonelauncher);

#define BRIMSTONE_TF2C_SLAMFIRE_DAMAGEREQUIRED 500.0f
#define BRIMSTONE_TF2C_SLAMFIRE_CHARGETIME 6.0f
#define BRIMSTONE_TF2C_SLAMFIRE_MULT_POSTFIREDELAY 0.75f

CTFBrimstoneLauncher::CTFBrimstoneLauncher( void )
{
	m_flChargeMeter = 0.0;
	m_bIsChargeActive = false;
}

void CTFBrimstoneLauncher::WeaponReset( void )
{
	BaseClass::WeaponReset();
	m_flChargeMeter = 0.0;
	m_bIsChargeActive = false;
}

void CTFBrimstoneLauncher::Precache( void )
{
	PrecacheScriptSound( "Weapon_PillStreak.Reset" );
	PrecacheScriptSound( "TFPlayer.LoseStoredCrit" );

	BaseClass::Precache();
}

void CTFBrimstoneLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( pOwner->m_nButtons & IN_USE )
	{
		ActivateCharge();
	}

	ChargeThink();
}

void CTFBrimstoneLauncher::ActivateCharge( void )
{
	if( m_flChargeMeter >= 1.0f )
	{
		EmitSound( "TFPlayer.LoseStoredCrit" );

		// temporary, can move to voice line eventually
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
			pPlayer->EmitSound( "Demoman.LaughShortRnd" );

		m_bIsChargeActive = true;
	}
}

void CTFBrimstoneLauncher::ChargeThink( void )
{
	if( m_bIsChargeActive )
	{
		float flChargeDuration = BRIMSTONE_TF2C_SLAMFIRE_CHARGETIME;
		CALL_ATTRIB_HOOK_FLOAT( flChargeDuration, mult_brimstone_slamfire_duration );

		float flChargeAmount = gpGlobals->frametime / flChargeDuration;
		float flNewLevel = max( m_flChargeMeter - flChargeAmount, 0.0 );
		m_flChargeMeter = flNewLevel;

		if( m_flChargeMeter <= 0.0f )
		{
			EmitSound( "Weapon_PillStreak.Reset" );
			m_bIsChargeActive = false;
		}
	}
}

void CTFBrimstoneLauncher::PlayWeaponShootSound( void )
{
	// Add a crit sound on top
	if ( m_bIsChargeActive )
	{
		if ( IsCurrentAttackACrit() )
		{
			WeaponSound( SPECIAL3 );
		}
		else
		{
			WeaponSound( SPECIAL2 );
		}
	}
	else
	{
		BaseClass::PlayWeaponShootSound();
	}
}

// always deplete charge even when not active
void CTFBrimstoneLauncher::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();
	ChargeThink();
}

void CTFBrimstoneLauncher::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();
	ChargeThink();
}

float CTFBrimstoneLauncher::GetFireRate( void )
{
	float flFireDelay = BaseClass::GetFireRate();

	if( m_bIsChargeActive )
	{
		float flSlamPostFireDelay = BRIMSTONE_TF2C_SLAMFIRE_MULT_POSTFIREDELAY;
		CALL_ATTRIB_HOOK_FLOAT( flSlamPostFireDelay, mult_brimstone_slamfire_postfiredelay );

		flFireDelay *= flSlamPostFireDelay;
	}

	return flFireDelay;
}

void CTFBrimstoneLauncher::OnDamage( float flDamage )
{
	if( !m_bIsChargeActive )
	{
		float flDamageRequired = BRIMSTONE_TF2C_SLAMFIRE_DAMAGEREQUIRED;
		CALL_ATTRIB_HOOK_FLOAT( flDamageRequired, mod_brimstone_slamfire_damagerequired );

		m_flChargeMeter += flDamage / flDamageRequired;
	}

	if( m_flChargeMeter > 1 )
		m_flChargeMeter = 1;
}

bool CTFBrimstoneLauncher::Reload(void)
{
	if ( m_bIsChargeActive )
		return false;

	return BaseClass::Reload();
}

bool CTFBrimstoneLauncher::ReloadOrSwitchWeapons( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( m_bIsChargeActive && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if( m_iClip1 == 0 )
			InstantlyReload( 1 );
		
		return false;
	}

	return BaseClass::ReloadOrSwitchWeapons();
}

bool CTFBrimstoneLauncher::IsChargeActive( void )
{
	return m_bIsChargeActive;
}

#ifdef CLIENT_DLL
float CTFBrimstoneLauncher::GetProgress( void )
{
	return m_flChargeMeter;
}

// everything below is hack and ugly copy paste. 
// copy from pillstreak. ty foxysen

void CTFBrimstoneLauncher::StartBurningEffect(void)
{
	// Clear any old effect before adding a new one.
	if (m_pBurningArrowEffect)
	{
		StopBurningEffect();
	}

	const char *pszEffect;
	m_hParticleEffectOwner = GetWeaponForEffect();
	if (m_hParticleEffectOwner)
	{
		if (m_hParticleEffectOwner != this)
		{
			// We're on the viewmodel.
			pszEffect = "v_flaming_arrow";
		}
		else
		{
			pszEffect = "flaming_arrow";
		}

		m_pBurningArrowEffect = m_hParticleEffectOwner->ParticleProp()->Create(pszEffect, PATTACH_POINT_FOLLOW, "muzzle");
	}
}


void CTFBrimstoneLauncher::StopBurningEffect(void)
{
	if (m_pBurningArrowEffect)
	{
		if (m_hParticleEffectOwner && m_hParticleEffectOwner->ParticleProp())
		{
			m_hParticleEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pBurningArrowEffect);
		}

		m_pBurningArrowEffect = NULL;
	}
}


void CTFBrimstoneLauncher::ThirdPersonSwitch(bool bThirdperson)
{
	// Switch out the burning effects.
	BaseClass::ThirdPersonSwitch(bThirdperson);
	StopBurningEffect();
	StartBurningEffect();
}


void CTFBrimstoneLauncher::UpdateOnRemove(void)
{
	StopBurningEffect();
	BaseClass::UpdateOnRemove();
}


void CTFBrimstoneLauncher::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	// Handle particle effect creation and destruction.
	if ((!IsHolstered() && m_bIsChargeActive) && !m_pBurningArrowEffect)
	{
		StartBurningEffect();
		EmitSound("ArrowLight");
	}
	else if ((IsHolstered() || !m_bIsChargeActive) && m_pBurningArrowEffect)
	{
		StopBurningEffect();
	}
}
#endif