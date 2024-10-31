//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF2C Heavy Anti-Aircraft / Flak Cannon
//
//=============================================================================//
#include "cbase.h" 
#include "tf_weapon_aagun.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "prediction.h"
#include "soundenvelope.h"
#endif
#include <in_buttons.h>

//=============================================================================
//
// Weapon AA Gun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFAAGun, tf_weapon_aagun )

//=============================================================================
//
// Weapon AA Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Skip Minigun sound functions, handle normally
//-----------------------------------------------------------------------------
void CTFAAGun::PlayWeaponShootSound( void )
{
	BaseClass::BaseClass::PlayWeaponShootSound();
#ifdef CLIENT_DLL
	EjectBrass();
#endif
}


void CTFAAGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /*= true*/, bool bDoPassthroughCheck /*= false*/ )
{
	vecOffset.z += -24.0f;

	BaseClass::BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, bDoPassthroughCheck );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFAAGun::HandleFireOnEmpty( void )
{
	BaseClass::HandleFireOnEmpty();

	WeaponSound( EMPTY );
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

void CTFAAGun::SharedAttack()
{
	if (gpGlobals->curtime < m_flNextPrimaryAttack)
		return;

	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	if (pPlayer->m_nButtons & IN_ATTACK)
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if (pPlayer->m_nButtons & IN_ATTACK2)
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	if (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		HandleFireOnEmpty();
		return;
	}

	switch (m_iWeaponState)
	{
		default:
		case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();
			break;
		}
		case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if (m_flNextPrimaryAttack <= gpGlobals->curtime)
			{
				if (m_iWeaponMode == TF_WEAPON_SECONDARY_MODE)
				{
					m_iWeaponState = AC_STATE_SPINNING;
	#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire(MP_CONCEPT_WINDAAGUN);
	#endif
				}
				else
				{
					m_iWeaponState = AC_STATE_FIRING;
	#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire(MP_CONCEPT_FIREAAGUN);
	#endif
				}

	#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
	#endif
			}
			break;
		}
		case AC_STATE_FIRING:
		{
			if (m_iWeaponMode == TF_WEAPON_SECONDARY_MODE)
			{
	#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire(MP_CONCEPT_WINDAAGUN);
	#endif
				m_iWeaponState = AC_STATE_SPINNING;

				m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;

	#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
	#endif
			}
			else if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
			{
				m_iWeaponState = AC_STATE_DRYFIRE;
			}
			else
			{
				if (GetFireStartTime() < 0)
				{
					SetFireStartTime( gpGlobals->curtime );
				}

	#ifdef GAME_DLL
				if (GetNextFiringSpeech() < gpGlobals->curtime)
				{
					SetNextFiringSpeech( gpGlobals->curtime + 5.0f );
					pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_AAGUN_FIREWEAPON);
				}
	#endif

				// Only fire if we're actually shooting
				PrimaryAttack(); // fire and do timers
				SetCritShot( IsCurrentAttackACrit() );
			}
			break;
		}
		case AC_STATE_DRYFIRE:
		{
			SetFireStartTime( -1.0f );
			if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0)
			{
				m_iWeaponState = AC_STATE_FIRING;
	#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
	#endif
			}
			else if (m_iWeaponMode == TF_WEAPON_SECONDARY_MODE)
			{
				m_iWeaponState = AC_STATE_SPINNING;

	#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
	#endif
			}
			SendWeaponAnim(ACT_VM_SECONDARYATTACK);
			break;
		}
		case AC_STATE_SPINNING:
		{
			SetFireStartTime(-1.0f);
			if (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE)
			{
				if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0)
				{
	#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire(MP_CONCEPT_FIREMINIGUN);
	#endif
					m_iWeaponState = AC_STATE_FIRING;

					if (GetFireStartTime() < 0)
					{
						SetFireStartTime(gpGlobals->curtime);
					}

#ifdef GAME_DLL
					if (GetNextFiringSpeech() < gpGlobals->curtime)
					{
						SetNextFiringSpeech(gpGlobals->curtime + 5.0f);
						pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_AAGUN_FIREWEAPON);
					}
#endif

					// Only fire if we're actually shooting
					PrimaryAttack(); // fire and do timers
					SetCritShot(IsCurrentAttackACrit());

#ifdef CLIENT_DLL 
					WeaponSoundUpdate();
#endif
					return;
				}
				else
				{
					m_iWeaponState = AC_STATE_DRYFIRE;
				}

	#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
	#endif
			}

			SendWeaponAnim(ACT_VM_SECONDARYATTACK);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play attack anim on each fire, don't loop
//-----------------------------------------------------------------------------
bool CTFAAGun::SendWeaponAnim( int iActivity )
{
	return BaseClass::BaseClass::SendWeaponAnim( iActivity );
	// TODO: also play third person model's "fire" sequence
	// This doesn't work:
	// ResetSequence( LookupSequence("fire") );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Skip Minigun brass eject and barrel rotation
//-----------------------------------------------------------------------------
void CTFAAGun::Simulate( void )
{
	BaseClass::BaseClass::Simulate();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Play AA Gun wind down since m_flBarrelCurrentVelocity is unavailable
//-----------------------------------------------------------------------------
void CTFAAGun::WindDown( void )
{
	BaseClass::WindDown();
	WeaponSound( SPECIAL3 );
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
// current weapon state.
// Ugly hack of Minigun function but will do for now
//-----------------------------------------------------------------------------
void CTFAAGun::WeaponSoundUpdate(void)
{
	if (prediction->InPrediction() && !prediction->IsFirstTimePredicted())
		return;

	MinigunState_t iWeaponState = m_iWeaponState;

	if (iWeaponState == AC_STATE_STARTFIRING)
	{
		// Force her onto the spinning sound if she's still spinning up and hold her there.
		if (m_flSpinningSoundTime != -1.0f && gpGlobals->curtime >= m_flSpinningSoundTime)
		{
			iWeaponState = AC_STATE_SPINNING;
		}
	}
	else if (iWeaponState == AC_STATE_SPINNING)
	{
		// If she's fully revved up but her spinning sound is still playing, hold her back there too.
		if (m_flSpinningSoundTime != -1.0f && gpGlobals->curtime <= m_flSpinningSoundTime)
		{
			iWeaponState = AC_STATE_STARTFIRING;
		}
	}
	else
	{
		m_flSpinningSoundTime = -1.0f;
	}

	// Otherwise, we'll determine the desired sound for our current state.
	int iSound = -1;
	switch (iWeaponState)
	{
	case AC_STATE_IDLE:
	{
		bool bHasSpinSounds = HasSpinSounds();
		if (!bHasSpinSounds && m_iMinigunSoundCur == SPECIAL3)
		{
			// Don't turn off for non-spinning miniguns.
			return;
		}
		else if (bHasSpinSounds && m_flBarrelCurrentVelocity > 0)
		{
			iSound = SPECIAL3;	// wind down sound

			if (m_flBarrelTargetVelocity > 0)
			{
				m_flBarrelTargetVelocity = 0;
			}
		}
		else
		{
			iSound = -1;
		}
		break;
	}
	case AC_STATE_STARTFIRING:
		iSound = SPECIAL2;	// wind up sound
		break;
	case AC_STATE_SPINNING:
	case AC_STATE_FIRING:
	case AC_STATE_DRYFIRE:
		if (!HasSpinSounds())
			return;

		iSound = RELOAD;	// spinning sound
		break;
	default:
		Assert(false);
		break;
	}

	// If we're already playing the desired sound, nothing to do.
	if (m_iMinigunSoundCur == iSound)
		return;

	// if we're playing some other sound, stop it
	if (m_pSoundCur)
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy(m_pSoundCur);
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = iSound;

	// If there's no sound to play for current state, we're done.
	if (iSound == -1)
		return;

	CSoundParameters params;
	if (C_BaseEntity::GetParametersForSound(GetShootSound(iSound), params, NULL))
	{
		// Play the appropriate sound.
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CLocalPlayerFilter filter;
		m_pSoundCur = controller.SoundCreate(filter, entindex(), params.channel, params.soundname, params.soundlevel);
		controller.Play(m_pSoundCur, 1.0f, 100.0f, params.delay_msec);
		controller.SoundChangeVolume(m_pSoundCur, params.volume, 0.0f);
		controller.SoundChangePitch(m_pSoundCur, params.pitch, 0.0f);
	}
}
#endif