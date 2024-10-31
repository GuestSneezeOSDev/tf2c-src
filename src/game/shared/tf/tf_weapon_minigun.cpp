//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_minigun.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "prediction.h"
#include "bone_setup.h"
#include "c_tf_viewmodeladdon.h"

// Server specific.
#else
#include "tf_player.h"
#include "NextBotManager.h"
#endif

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigun, DT_WeaponMinigun )

BEGIN_NETWORK_TABLE( CTFMinigun, DT_WeaponMinigun )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropTime( RECVINFO( m_flStateUpdateTime ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) ),
	RecvPropFloat( RECVINFO( m_flBarrelCurrentVelocity ) ),
	RecvPropFloat( RECVINFO( m_flBarrelTargetVelocity ) ),
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 3, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropTime( SENDINFO( m_flStateUpdateTime ) ),
	SendPropBool( SENDINFO( m_bCritShot ) ),
	SendPropFloat( SENDINFO( m_flBarrelCurrentVelocity ), -1, SPROP_CHANGES_OFTEN ),
	SendPropFloat( SENDINFO( m_flBarrelTargetVelocity ), -1, SPROP_CHANGES_OFTEN ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFMinigun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flStateUpdateTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( m_bCritShot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flBarrelCurrentVelocity, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flBarrelTargetVelocity, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_minigun, CTFMinigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_minigun );

#ifdef CLIENT_DLL
extern ConVar tf2c_model_muzzleflash;
#endif

ConVar tf2c_minigun_rampup( "tf2c_minigun_rampup", "0", FCVAR_REPLICATED, "Toggles Minigun spinup time and damage/spread rampup over the first 1s after spinup (from Love & War)" );

// NOTE: If you change 'MINIGUN_SPINDOWN_TIME', make sure to take a look at 'UpdateBarrelMovement()'.
#define MINIGUN_SPINUP_TIME			0.75f
#define MINIGUN_SPINDOWN_TIME		0.5f // Buffed: From 1.0s to 0.5s.

#define MINIGUN_SPINUP_TIME_ORIG	1.0f // Original spinup time (also used for animation)

#define MINIGUN_RAMPUP_TIME			1.0f // How long it takes for Minigun fire to settle
#define MINIGUN_RAMPUP_TIME_MIDAIR	2.0f 
#define MINIGUN_RAMPUP_ACCURACY		1.5f // Bullet spread starts this much worse before settling
#define MINIGUN_RAMPUP_DAMAGE		0.5f // Damage starts this much worse before settling

#define TF2C_TRANQ_SPINUP_FACTOR	1.33f
#define TF2C_TRANQ_SPINDOWN_FACTOR	1.33f

//=============================================================================
//
// Weapon Minigun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMinigun::CTFMinigun()
{
#ifdef CLIENT_DLL
	m_pSoundCur = NULL;
	m_iMinigunSoundCur = -1;

	m_flSpinningSoundTime = -1.0f;

	m_pEjectBrassEffect = NULL;
	m_pMuzzleEffect = NULL;
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMinigun::~CTFMinigun()
{
	WeaponReset();
}


void CTFMinigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = AC_STATE_IDLE;
	m_flStateUpdateTime = 0.0f;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1;
	m_flStartedSpinupAt = -1;
	m_flLeftGroundAt = -1;

	m_flBarrelCurrentVelocity = 0;
	m_flBarrelTargetVelocity = 0;

#ifdef GAME_DLL
	m_flNextFiringSpeech = 0;
#else
	m_flBarrelAngle = 0;
	m_flEjectBrassTime = 0.0f;

	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;

	m_flSpinningSoundTime = -1.0f;

	StopMuzzleEffect();
	StopBrassEffect();
#endif
}


float CTFMinigun::OwnerMaxSpeedModifier( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimingMoveSpeed = 110.0f;
		return flAimingMoveSpeed;
	}

	return 0.0f;
}


bool CTFMinigun::OwnerCanJump( void )
{
	int bRevvedJump = 0;
	CALL_ATTRIB_HOOK_INT(bRevvedJump, minigun_jump_while_revved);
	return m_iWeaponState == AC_STATE_IDLE || bRevvedJump;
}

#ifdef GAME_DLL

int CTFMinigun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif


void CTFMinigun::Precache( void )
{
#ifdef GAME_DLL
	PrecacheParticleSystem( "eject_minigunbrass" );
#endif

	BaseClass::Precache();
}


void CTFMinigun::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}


void CTFMinigun::ItemPostFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->GetGroundEntity() )
	{
		m_flLeftGroundAt = gpGlobals->curtime;
	}

	if ( ( pPlayer->m_nButtons & ( IN_ATTACK | IN_ATTACK2 ) ) && CanAttack() )
	{
		SharedAttack();
	}
	else if ( m_iWeaponState != AC_STATE_IDLE )
	{
		// Wind down if player releases fire buttons.
		if ( gpGlobals->curtime >= m_flStateUpdateTime )
		{
			WindDown();
		}
	}
	else if ( !ReloadOrSwitchWeapons() )
	{
		WeaponIdle();
	}
}


void CTFMinigun::SharedAttack()
{
	if ( gpGlobals->curtime < m_flNextPrimaryAttack )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if ( pPlayer->m_nButtons & IN_ATTACK2 )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}

	switch ( m_iWeaponState )
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
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
					m_iWeaponState = AC_STATE_SPINNING;
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				}
				else
				{
					m_iWeaponState = AC_STATE_FIRING;
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
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
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_iWeaponState = AC_STATE_SPINNING;

				m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;

#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
#endif
			}
			else if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
			{
				m_iWeaponState = AC_STATE_DRYFIRE;
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0f;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

				// Only fire if we're actually shooting
				PrimaryAttack(); // fire and do timers
				m_bCritShot = IsCurrentAttackACrit();
			}
			break;
		}
		case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1;
			if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
			{
				m_iWeaponState = AC_STATE_FIRING;
#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
#endif
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				m_iWeaponState = AC_STATE_SPINNING;

#ifdef CLIENT_DLL 
				WeaponSoundUpdate();
#endif
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
		case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1;
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					m_iWeaponState = AC_STATE_FIRING;

					if (m_flStartedFiringAt < 0)
					{
						m_flStartedFiringAt = gpGlobals->curtime;
					}

#ifdef GAME_DLL
					if (m_flNextFiringSpeech < gpGlobals->curtime)
					{
						m_flNextFiringSpeech = gpGlobals->curtime + 5.0f;
						pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_MINIGUN_FIREWEAPON);
					}
#endif

					// Only fire if we're actually shooting
					PrimaryAttack(); // fire and do timers
					m_bCritShot = IsCurrentAttackACrit();

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

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}
}


void CTFMinigun::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_STARTFIRING;
	pPlayer->m_Shared.AddCond( TF_COND_AIMING );

#ifndef CLIENT_DLL
	pPlayer->StopRandomExpressions();
	TheNextBots().OnWeaponFired( pPlayer, this );
#else
	WeaponSoundUpdate();
#endif

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

	float flWindUpTime = GetWindUpTime();
	CBaseViewModel *pViewModel = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( pViewModel )
	{
		pViewModel->SetPlaybackRate( MINIGUN_SPINUP_TIME_ORIG / Max( flWindUpTime, 0.00001f ) );
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flStateUpdateTime = gpGlobals->curtime + flWindUpTime;
	m_flStartedFiringAt = -1;
	m_flStartedSpinupAt = gpGlobals->curtime;
	//DevMsg( "Spinup time: %2.2f\n", m_flStartedSpinupAt );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

#ifdef GAME_DLL
// nobody is listening to this event??
// -sappho
#if 0
	IGameEvent* event = gameeventmanager->CreateEvent("heavy_windup");

	if (event)
	{
		event->SetInt("userid", pPlayer->GetUserID());

		gameeventmanager->FireEvent(event);
	}
#endif
#endif
}


bool CTFMinigun::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_flStateUpdateTime = gpGlobals->curtime;
		return true;
	}

	return false;
}


bool CTFMinigun::CanHolster( void ) const
{
	if ( m_iWeaponState > AC_STATE_IDLE )
		return false;

	if ( gpGlobals->curtime < m_flStateUpdateTime )
		return false;

	return BaseClass::CanHolster();
}


bool CTFMinigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	m_flBarrelCurrentVelocity = 0;
	m_flBarrelTargetVelocity = 0;

	return BaseClass::Holster( pSwitchingTo );
}


bool CTFMinigun::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}


void CTFMinigun::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

#ifdef CLIENT_DLL
	if ( !HasSpinSounds() && m_iWeaponState == AC_STATE_FIRING )
	{
		PlayStopFiringSound();
	}
#endif

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_IDLE;
	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

	float flWindDownTime = GetWindDownTime();
	CBaseViewModel *pViewModel = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( pViewModel )
	{
		pViewModel->SetPlaybackRate( MINIGUN_SPINDOWN_TIME / Max( flWindDownTime, 0.00001f ) );
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flStateUpdateTime = gpGlobals->curtime + flWindDownTime;
	m_flBarrelTargetVelocity = 0;
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );

	m_flStartedSpinupAt = -1;
}


bool CTFMinigun::SendWeaponAnim( int iActivity )
{
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = 20 / ( Max( 0.0001f, GetFireRate() / GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay ) );
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK || iActivity == ACT_VM_SECONDARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == TranslateViewmodelHandActivity( iActivity ) )
			return true;
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFMinigun::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		m_iWeaponState = AC_STATE_DRYFIRE;

		SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		{
			m_iWeaponState = AC_STATE_SPINNING;
		}
	}
	else if ( m_iWeaponState == AC_STATE_IDLE )
	{
		ReloadOrSwitchWeapons();
		m_fFireDuration = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateBarrelMovement()
{
#ifdef CLIENT_DLL
	// Clear out of the forced local weapon state if this duration has passed.
	if ( m_flSpinningSoundTime != -1.0f && gpGlobals->curtime >= m_flSpinningSoundTime )
	{
		WeaponSoundUpdate();
	}
#endif

	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		// Update barrel velocity to bring it up to speed or to rest.
		// Added half a second to the wind down time (+ 0.5f) so the barrel spinning looks normal on stock.
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, ( gpGlobals->frametime * 6.0f ) / 
			( m_iWeaponState == AC_STATE_STARTFIRING ? GetWindUpTime() : m_iWeaponState > AC_STATE_IDLE ? Max( 0.0001f, GetFireRate() / 
				GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay ) : ( GetWindDownTime() + 0.5f ) ) );

#ifdef CLIENT_DLL
		if ( m_flBarrelCurrentVelocity == 0 )
		{
			// If we've stopped rotating, turn off the wind-down sound.
			WeaponSoundUpdate();
		}
#endif
	}
}


float CTFMinigun::GetWindUpTime( bool bIgnoreTranq /*= false*/ )
{
	float flWindUpTime = tf2c_minigun_rampup.GetBool() ? MINIGUN_SPINUP_TIME : MINIGUN_SPINUP_TIME_ORIG;
	CALL_ATTRIB_HOOK_FLOAT( flWindUpTime, mult_minigun_spinup_time );

	if ( !bIgnoreTranq )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
		{
			flWindUpTime *= TF2C_TRANQ_SPINUP_FACTOR;
		}
	}

	return flWindUpTime;
}


float CTFMinigun::GetWindDownTime( bool bIgnoreTranq /*= false*/ )
{
	float flWindDownTime = MINIGUN_SPINDOWN_TIME;
	CALL_ATTRIB_HOOK_FLOAT( flWindDownTime, mult_minigun_spindown_time );

	if ( !bIgnoreTranq )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
		{
			flWindDownTime *= TF2C_TRANQ_SPINDOWN_FACTOR;
		}
	}

	return flWindDownTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetProjectileDamage( void )
{
	float flDamage = BaseClass::GetProjectileDamage();

	if ( tf2c_minigun_rampup.GetBool() )
	{
		float flTimeSinceSpinupFinish = GetSpunupTime() - GetWindUpTime();

		if ( flTimeSinceSpinupFinish < MINIGUN_RAMPUP_TIME )
		{
			float flDamageMult = 1.0f;
			flDamageMult = RemapValClamped( flTimeSinceSpinupFinish, 0.0f, MINIGUN_RAMPUP_TIME, MINIGUN_RAMPUP_DAMAGE, 1.0f );
			flDamage *= flDamageMult;
			//DevMsg( "Minigun damage: %2.2f (x%2.2f) - SpunupTime: %2.2f\n", flDamage, flDamageMult, flTimeSinceSpinupFinish );
		}
	}

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetWeaponSpread( void )
{
	float flSpread = BaseClass::GetWeaponSpread();

	if ( tf2c_minigun_rampup.GetBool() )
	{
		float flTimeSinceSpinupFinish = GetSpunupTime() - GetWindUpTime();

		if ( flTimeSinceSpinupFinish < MINIGUN_RAMPUP_TIME )
		{
			float flSpreadMult = RemapValClamped( flTimeSinceSpinupFinish, 0.0f, MINIGUN_RAMPUP_TIME, MINIGUN_RAMPUP_ACCURACY, 1.0f );
			flSpread *= flSpreadMult;
			//DevMsg( "Minigun spread: %f (x%f)\n", flSpread, flSpreadMult );
		}
	}

	// TF2C: Worse spread when in mid-air (make attribute so custom weapons unaffected?)
	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		float flTimeSinceGround = gpGlobals->curtime - m_flLeftGroundAt;
		float flSpreadMultAir = RemapValClamped( flTimeSinceGround, 0.0f, MINIGUN_RAMPUP_TIME_MIDAIR, 2.0f, 1.0f );
		flSpread *= flSpreadMultAir;
		//DevMsg( "Minigun spread: %f (x%f), airtime: %f, curtime: %f\n", flSpread, flSpreadMultAir, m_flLeftGroundAt, gpGlobals->curtime );
	}

	return flSpread;
}

#ifdef CLIENT_DLL

void CTFMinigun::GetWeaponCrosshairScale( float& flScale )
{
	// Compare current spread vs base spread to signify spread rampup
	float flCrosshairScaleGoal = RemapValClamped( GetWeaponSpread(), BaseClass::GetWeaponSpread() * 0.5f, BaseClass::GetWeaponSpread() * 2.0f, 0.66f, 1.5f );
	flScale = Lerp( 0.5f, flScale, flCrosshairScaleGoal );
}

CStudioHdr *CTFMinigun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );
	m_flBarrelAngle = 0;

	return hdr;
}


void CTFMinigun::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if ( m_iBarrelBone != -1 )
	{
		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}


void CTFMinigun::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	//HandleBrassEffect();

	//	if (!ShouldMuzzleFlash())
	if ( !tf2c_model_muzzleflash.GetBool() )
	{
		HandleMuzzleEffect();
	}

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();

	// Turn off the firing sound here.
	if ( m_iPrevMinigunState == AC_STATE_FIRING && ( m_iWeaponState == AC_STATE_SPINNING || m_iWeaponState == AC_STATE_IDLE ) )
	{
		if ( !HasSpinSounds() )
		{
			PlayStopFiringSound();
		}
	}

	m_iPrevMinigunState = m_iWeaponState;
}


void CTFMinigun::Simulate( void )
{
	BaseClass::Simulate();

	if ( !IsDormant() )
	{
		// Eject brass shells every 100ms.
		if ( m_iWeaponState == AC_STATE_FIRING && gpGlobals->curtime >= m_flEjectBrassTime )
		{
			EjectBrass();
			m_flEjectBrassTime = gpGlobals->curtime + 0.1f;
		}

		// Update the barrel rotation based on current velocity.
		m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
	}
}


void CTFMinigun::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();
	StopBrassEffect();

	BaseClass::UpdateOnRemove();
}


void CTFMinigun::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
			StopBrassEffect();
		}
	}

	// Deliberately skip base combat weapon
	//C_BaseEntity::SetDormant( bDormant );
	BaseClass::SetDormant( bDormant );
}

extern ConVar cl_ejectbrass;


void CTFMinigun::StartBrassEffect()
{
	if ( !cl_ejectbrass.GetBool() )
		return;

	StopBrassEffect();

	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	// Start the brass ejection, if a system hasn't already been started.
	int iBrassAttachment = pEffectOwner->LookupAttachment( "eject_brass" );
	if ( iBrassAttachment > 0 && m_pEjectBrassEffect == NULL )
	{
		m_pEjectBrassEffect = pEffectOwner->ParticleProp()->Create( "eject_minigunbrass", PATTACH_POINT_FOLLOW, iBrassAttachment );
		m_hBrassEffectHost = pEffectOwner;
	}
}


void CTFMinigun::StartMuzzleEffect()
{
	StopMuzzleEffect();

	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	int iMuzzleAttachment = GetTracerAttachment();

	// Start the muzzle flash, if a system hasn't already been started.
	if ( iMuzzleAttachment != -1 && m_pMuzzleEffect == NULL )
	{
		m_pMuzzleEffect = pEffectOwner->ParticleProp()->Create( GetMuzzleFlashParticleEffect(), PATTACH_POINT_FOLLOW, iMuzzleAttachment );
		m_hMuzzleEffectHost = pEffectOwner;
	}
}


void CTFMinigun::StopBrassEffect()
{
	// Stop the brass ejection.
	if ( m_pEjectBrassEffect )
	{
		C_BaseEntity *pEffectOwner = m_hBrassEffectHost.Get();
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_pEjectBrassEffect );
			m_hBrassEffectHost = NULL;
		}

		m_pEjectBrassEffect = NULL;
	}
}


void CTFMinigun::StopMuzzleEffect()
{
	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		C_BaseEntity *pEffectOwner = m_hMuzzleEffectHost.Get();
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_pMuzzleEffect );
			m_hMuzzleEffectHost = NULL;
		}

		m_pMuzzleEffect = NULL;
	}
}


void CTFMinigun::HandleBrassEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pEjectBrassEffect == NULL )
	{
		StartBrassEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pEjectBrassEffect )
	{
		StopBrassEffect();
	}
}


void CTFMinigun::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{	
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFMinigun::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}


void CTFMinigun::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment )
{
	// Support both styles here.
	bool bIsTF2 = ( GetViewModelType() == VMTYPE_TF2 );

	int iBarrelBone = bIsTF2 ? Studio_BoneIndexByName( hdr, "barrel" ) : Studio_BoneIndexByName( hdr, "v_minigun_barrel" );
	if ( iBarrelBone != -1 && ( hdr->boneFlags( iBarrelBone ) & boneMask ) )
	{
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );
	
		if ( bIsTF2 )
		{
			a.z = GetBarrelRotation();
		}
		else
		{
			a.x = GetBarrelRotation();
		}
	
		AngleQuaternion( a, q[iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
// current weapon state.
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponSoundUpdate( void )
{
	if ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
		return;

	MinigunState_t iWeaponState = m_iWeaponState;
	if ( iWeaponState == AC_STATE_STARTFIRING )
	{
		// Force her onto the spinning sound if she's still spinning up and hold her there.
		if ( m_flSpinningSoundTime != -1.0f && gpGlobals->curtime >= m_flSpinningSoundTime )
		{
			iWeaponState = AC_STATE_SPINNING;
		}
	}
	else if ( iWeaponState == AC_STATE_SPINNING )
	{
		// If she's fully revved up but her spinning sound is still playing, hold her back there too.
		if ( m_flSpinningSoundTime != -1.0f && gpGlobals->curtime <= m_flSpinningSoundTime )
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
	switch ( iWeaponState )
	{
		case AC_STATE_IDLE:
		{
			bool bHasSpinSounds = HasSpinSounds();
			if ( !bHasSpinSounds && m_iMinigunSoundCur == SPECIAL2 )
			{
				// Don't turn off for non-spinning miniguns.
				return;
			}
			else if ( bHasSpinSounds && m_flBarrelCurrentVelocity > 0 )
			{
				iSound = SPECIAL2;	// wind down sound

				if ( m_flBarrelTargetVelocity > 0 )
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
			iSound = SINGLE;	// wind up sound. special1 is used for projectile explosion :(
			break;
		case AC_STATE_FIRING:
		{
			if ( m_bCritShot ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
		case AC_STATE_SPINNING:
			if ( !HasSpinSounds() )
				return;

			iSound = SPECIAL3;	// spinning sound
			break;
		case AC_STATE_DRYFIRE:
			iSound = EMPTY;		// out of ammo, still trying to fire
			break;
		default:
			Assert( false );
			break;
	}

	// If we're already playing the desired sound, nothing to do.
	if ( m_iMinigunSoundCur == iSound )
		return;

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = iSound;

	// If there's no sound to play for current state, we're done.
	if ( iSound == -1 )
		return;
	
	CSoundParameters params;
	if ( C_BaseEntity::GetParametersForSound( GetShootSound( iSound ), params, NULL ) )
	{
		// Play the appropriate sound.
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CLocalPlayerFilter filter;
		m_pSoundCur = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
		controller.Play( m_pSoundCur, 1.0f, 100.0f, params.delay_msec );
		controller.SoundChangeVolume( m_pSoundCur, params.volume, 0.0f );
		controller.SoundChangePitch( m_pSoundCur, params.pitch, 0.0f );

		if ( iSound == SINGLE && m_flSpinningSoundTime == -1.0f )
		{
			m_flSpinningSoundTime = gpGlobals->curtime + ( ( params.delay_msec + enginesound->GetSoundDuration( params.soundname ) ) * ( 100.0f / params.pitch ) );
		}
	}
}


void CTFMinigun::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	// Restart the looping muzzle effect.
	if ( m_pMuzzleEffect )
	{
		StartMuzzleEffect();
	}
}


void CTFMinigun::PlayStopFiringSound()
{
	// If we're already playing the desired sound, nothing to do.
	if ( m_iMinigunSoundCur == SPECIAL2 )
		return;

	// If we're playing some other sound, stop it.
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately.
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = SPECIAL2;

	CSoundParameters params;
	if ( C_BaseEntity::GetParametersForSound( GetShootSound( SPECIAL2 ), params, NULL ) )
	{
		// Play the appropriate sound.
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CLocalPlayerFilter filter;
		m_pSoundCur = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
		controller.Play( m_pSoundCur, 1.0f, 100.0f, params.delay_msec );
		controller.SoundChangeVolume( m_pSoundCur, params.volume, 0.0f );
		controller.SoundChangePitch( m_pSoundCur, params.pitch, 0.0f );
	}
}
#endif
