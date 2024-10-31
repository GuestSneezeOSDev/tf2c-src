//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pillstreak.h" 
#include "tf_fx_shared.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

#ifdef TF2C_BETA
//=============================================================================
//
// Weapon Pillstreak tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFPillstreak, DT_TFPillstreak);
BEGIN_NETWORK_TABLE(CTFPillstreak, DT_TFPillstreak)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iStreakCount)),
//RecvPropFloat( RECVINFO( m_flTimeDecrementStreak ) )
RecvPropInt(RECVINFO(m_iMissCount))
#else
SendPropInt(SENDINFO(m_iStreakCount), 6, SPROP_UNSIGNED),
//SendPropFloat(SENDINFO(m_flTimeDecrementStreak), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN)
SendPropInt(SENDINFO(m_iMissCount), 2, SPROP_UNSIGNED)
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFPillstreak)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_pillstreak, CTFPillstreak);
PRECACHE_WEAPON_REGISTER(tf_weapon_pillstreak);

#define PILLSTREAK_TF2C_MAX_VISUAL_STREAK 63
#define PILLSTREAK_TF2C_MAX_DAMAGE_STREAK 6
#define PILLSTREAK_TF2C_DAMAGE_GAIN_PER_STREAK 5.0
#define PILLSTREAK_TF2C_STREAK_START_FIRE 6
#define PILLSTREAK_TF2C_DECAY_TIME 10
#define PILLSTREAK_TF2C_MISS_COUNT 4

//=============================================================================
//
// Weapon Pillstreak functions.
//

void CTFPillstreak::Precache( void )
{
	PrecacheScriptSound( "ArrowLight" );

	PrecacheParticleSystem( "flaming_arrow" );
	PrecacheParticleSystem( "v_flaming_arrow" );

	//PrecacheScriptSound( "Weapon_PillStreak.SingleComboLow" );
	//PrecacheScriptSound( "Weapon_PillStreak.SingleComboMedium" );
	//PrecacheScriptSound( "Weapon_PillStreak.SingleComboHigh" );
	PrecacheScriptSound( "Weapon_PillStreak.Miss" );
	PrecacheScriptSound( "Weapon_PillStreak.Reset" );
	PrecacheScriptSound( "Weapon_Fist.MissCrit" );

	BaseClass::Precache();
}

void CTFPillstreak::WeaponReset(void)
{
	BaseClass::WeaponReset();
	m_iStreakCount = 0;
	//m_flTimeDecrementStreak = -1;
	m_iMissCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so DEMOMAN IS VERY DRUNK
//-----------------------------------------------------------------------------
float CTFPillstreak::GetProjectileDamage(void)
{
	float flDamage = BaseClass::GetProjectileDamage();
	flDamage += Min(GetPipeStreak(), PILLSTREAK_TF2C_MAX_DAMAGE_STREAK) * PILLSTREAK_TF2C_DAMAGE_GAIN_PER_STREAK;
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Base damage without the streak bonus
//-----------------------------------------------------------------------------
float CTFPillstreak::GetProjectileDamageNoStreak( void )
{
	return BaseClass::GetProjectileDamage();
}


void CTFPillstreak::IncrementPipeStreak(void)
{
	m_iMissCount = 0;
	if (m_iStreakCount < PILLSTREAK_TF2C_MAX_VISUAL_STREAK)
	{
		++m_iStreakCount;
		if (m_iStreakCount == PILLSTREAK_TF2C_MAX_DAMAGE_STREAK)
		{
#ifdef GAME_DLL
			CTFPlayer* pPlayer = GetTFPlayerOwner();
			if (pPlayer)
			{
				if (pPlayer->IsSpeaking())
				{
					CMultiplayer_Expresser *pExpresser = pPlayer->GetMultiplayerExpresser();
					if (pExpresser)
					{
						pExpresser->ForceNotSpeaking();
					}
				}
				pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_PILLSTREAK_CHARGED);
			}
#endif
		}
	}

	//m_flTimeDecrementStreak = gpGlobals->curtime + PILLSTREAK_TF2C_DECAY_TIME;
}


// Currently doesn't do what you expect, rather on_miss alike
void CTFPillstreak::DecrementPipeStreak(void)
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	if (m_iStreakCount > 0)
	{
		m_iMissCount += 1;

		if (m_iMissCount >= PILLSTREAK_TF2C_MISS_COUNT)
		{
			ClearPipeStreak();

			if ( pPlayer )
			{
				CSingleUserRecipientFilter filter( pPlayer );
				filter.UsePredictionRules();
				CBaseEntity::EmitSound( filter, pPlayer->entindex(), "Weapon_PillStreak.Reset" );
			}
		}
		else
		{
			if ( pPlayer )
			{
				CSingleUserRecipientFilter filter( pPlayer );
				filter.UsePredictionRules();
				CBaseEntity::EmitSound( filter, pPlayer->entindex(), "Weapon_PillStreak.Miss" );
			}
		}
	}

	/*if (m_iStreakCount > 0)
	{
		--m_iStreakCount;
	}*/


	/*if (m_iStreakCount > 0)
	{
		m_flTimeDecrementStreak = gpGlobals->curtime + PILLSTREAK_TF2C_DECAY_TIME;
	}
	else
	{
		m_flTimeDecrementStreak = -1;
	}*/
}


int CTFPillstreak::GetPipeStreak(void)
{
	return m_iStreakCount;
}


int CTFPillstreak::GetMaxDamagePipeStreak(void)
{
	return PILLSTREAK_TF2C_MAX_DAMAGE_STREAK;
}


void CTFPillstreak::ClearPipeStreak(void)
{
	m_iStreakCount = 0;
	m_iMissCount = 0;
	//m_flTimeDecrementStreak = -1;
}

void CTFPillstreak::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	/*if ( gpGlobals->curtime > m_flTimeDecrementStreak && m_flTimeDecrementStreak > 0 )
	{
		DecrementPipeStreak();
		//ClearPipeStreak();
	}*/
}

void CTFPillstreak::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	/*if ( gpGlobals->curtime > m_flTimeDecrementStreak && m_flTimeDecrementStreak > 0 )
	{
		DecrementPipeStreak();
		//ClearPipeStreak();
	}*/
}

void CTFPillstreak::PlayWeaponShootSound( void )
{
	// Add a crit sound on top
	if ( IsCurrentAttackACrit() )
	{
		CPASAttenuationFilter filter( this, SNDLVL_100dB );
		if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
		{
			filter.UsePredictionRules();
		}
		EmitSound( filter, entindex(), "Weapon_Fist.MissCrit" );
	}

	if ( m_iStreakCount >= PILLSTREAK_TF2C_MAX_DAMAGE_STREAK )
	{
		WeaponSound( SPECIAL3 );
	}
	else if ( m_iStreakCount >= ceil( PILLSTREAK_TF2C_MAX_DAMAGE_STREAK * 0.5 ) )
	{
		WeaponSound( SPECIAL2 );
	}
	else if ( m_iStreakCount >= 1 )
	{
		WeaponSound( SPECIAL1 );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}


// everything below is hack and ugly copy paste




#ifdef CLIENT_DLL

void CTFPillstreak::StartBurningEffect(void)
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


void CTFPillstreak::StopBurningEffect(void)
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


void CTFPillstreak::ThirdPersonSwitch(bool bThirdperson)
{
	// Switch out the burning effects.
	BaseClass::ThirdPersonSwitch(bThirdperson);
	StopBurningEffect();
	StartBurningEffect();
}


void CTFPillstreak::UpdateOnRemove(void)
{
	StopBurningEffect();
	BaseClass::UpdateOnRemove();
}


void CTFPillstreak::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	// Handle particle effect creation and destruction.
	if ((!IsHolstered() && m_iStreakCount >= PILLSTREAK_TF2C_STREAK_START_FIRE) && !m_pBurningArrowEffect)
	{
		StartBurningEffect();
		EmitSound("ArrowLight");
	}
	else if ((IsHolstered() || m_iStreakCount < PILLSTREAK_TF2C_STREAK_START_FIRE) && m_pBurningArrowEffect)
	{
		StopBurningEffect();
	}
}

// not ugly hack and copypaste

float CTFPillstreak::GetProgress()
{
	//return (m_flTimeDecrementStreak - gpGlobals->curtime) / PILLSTREAK_TF2C_DECAY_TIME;
	if (m_iStreakCount > 0)
		return 1.0f - (float(m_iMissCount) / PILLSTREAK_TF2C_MISS_COUNT);
	else
		return 0.0f;
}
#endif

#endif // TF2C_BETA