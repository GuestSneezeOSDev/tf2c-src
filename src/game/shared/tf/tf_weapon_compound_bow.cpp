//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_compound_bow.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "prediction.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_projectile_arrow.h"
#endif

//=============================================================================
//
// Weapon tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCompoundBow, DT_WeaponCompoundBow )

BEGIN_NETWORK_TABLE( CTFCompoundBow, DT_WeaponCompoundBow )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bArrowAlight ) ),
	RecvPropBool( RECVINFO( m_bNoFire ) ),
#else
	SendPropBool( SENDINFO( m_bArrowAlight ) ),
	SendPropBool( SENDINFO( m_bNoFire ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCompoundBow )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bNoFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_compound_bow, CTFCompoundBow );
PRECACHE_WEAPON_REGISTER( tf_weapon_compound_bow );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFCompoundBow )
END_DATADESC()
#endif

#define TF_ARROW_MAX_CHARGE_TIME	5.0f

#define TF_ARROW_MAX_CHARGE_TIME_START_ANIMATION (TF_ARROW_MAX_CHARGE_TIME - 1.5f)
// Huntsman has a 2 second transition animation. Let's start it early.
// P.S. For seemingly generic way to get Huntsman animation duration, use this: SequenceDuration(SelectWeightedSequence(static_cast<Activity>(TranslateViewmodelHandActivity(ACT_ITEM2_VM_CHARGE_IDLE_3))))
// More generic, should work better with mods that replace model with new animation, but messy

//=============================================================================
//
// Weapon functions.
//


CTFCompoundBow::CTFCompoundBow()
{
	m_flLastDenySoundTime = 0.0f;
	m_bNoFire = false;
	m_bReloadsSingly = false;
}


void CTFCompoundBow::Precache( void )
{
	PrecacheScriptSound( "Weapon_CompoundBow.SinglePull" );
	PrecacheScriptSound( "ArrowLight" );

	PrecacheParticleSystem( "flaming_arrow" );
	PrecacheParticleSystem( "v_flaming_arrow" );

	BaseClass::Precache();
}


void CTFCompoundBow::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bArrowAlight = false;
	m_bNoAutoRelease = true;
	m_bNoFire = false;
}


void CTFCompoundBow::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_bWantsToShoot = false;

#ifdef GAME_DLL
	CBaseEntity *MainProjectile = FireProjectile(pPlayer);
	CTFProjectile_Arrow *pMainArrow = dynamic_cast<CTFProjectile_Arrow *>( MainProjectile );
	if ( pMainArrow )
	{
		pMainArrow->SetArrowAlight( m_bArrowAlight );
	}
#else
	FireProjectile( pPlayer );
#endif

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flBaseFireDelay = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	float flFireDelay = ApplyFireDelay( flBaseFireDelay );

	ApplyRefireSpeedModifications( flFireDelay );
	
	float flRateMultiplyer = flBaseFireDelay / flFireDelay;

	// Speed up the reload animation built in to firing.
	CBaseViewModel *pViewModel = pPlayer->GetViewModel();
	if ( pViewModel )
	{
		pViewModel->SetPlaybackRate( flRateMultiplyer );
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	m_flLastDenySoundTime = gpGlobals->curtime;

	SetWeaponIdleTime( m_flNextPrimaryAttack + ( 0.5f * flRateMultiplyer ) );

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

	m_flChargeBeginTime = 0;
	m_bArrowAlight = false;

	// The bow doesn't actually reload, it instead uses the AE_WPN_INCREMENTAMMO anim event in the fire to reload the clip.
	// We need to reset this bool each time we fire so that anim event works.
	m_bReloadedThroughAnimEvent = false;
}


void CTFCompoundBow::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( m_bNoFire )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0.0f;
		return;
	}

	if ( m_flChargeBeginTime <= 0.0f )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;

		SendWeaponAnim( ACT_VM_PULLBACK );

		float flRateMultiplyer = ApplyFireDelay( 1.0f );
		ApplyRefireSpeedModifications( flRateMultiplyer );
		if ( flRateMultiplyer > 0.0f )
		{
			flRateMultiplyer = 1.0f / flRateMultiplyer;
		}

		// Speed up the reload animation built in to firing.
		CBaseViewModel *pViewModel = pPlayer->GetViewModel();
		if ( pViewModel )
		{
			pViewModel->SetPlaybackRate( flRateMultiplyer );
		}

#ifndef GAME_DLL
		if ( prediction->IsFirstTimePredicted() )
#endif
		{
			// Increase the pitch of the pull sound when the fire rate is higher.
			CSoundParameters params;
			if ( CBaseEntity::GetParametersForSound( "Weapon_CompoundBow.SinglePull", params, NULL ) )
			{
				CPASAttenuationFilter filter( pPlayer->GetAbsOrigin(), params.soundlevel );
#ifdef GAME_DLL
				filter.RemoveRecipient( pPlayer );
#endif
				EmitSound_t ep( params );
				ep.m_nPitch *= flRateMultiplyer;

				pPlayer->EmitSound( filter, pPlayer->entindex(), ep );
			}
		}

		// Slow down movement speed while the bow is pulled back.
		pPlayer->m_Shared.AddCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
		if ( flTotalChargeTime >= GetChargeMaxTime() )
		{
			flTotalChargeTime = GetChargeMaxTime();
			//LaunchGrenade();
		}
	}
}


float CTFCompoundBow::GetChargeMaxTime( void )
{
	// It takes less time to charge if the fire rate is higher.
	float flChargeMaxTime = ApplyFireDelay( 1.0f );
	ApplyRefireSpeedModifications( flChargeMaxTime );
	return flChargeMaxTime;
}


float CTFCompoundBow::GetCurrentCharge( void )
{
	if ( m_flChargeBeginTime == 0.0f )
		return 0.0f;
	
	return Max( Min( gpGlobals->curtime - m_flChargeBeginTime, 1.0f ), 0.0f );
}


float CTFCompoundBow::GetProjectileDamage( void )
{
	return Max( 50.0f + BaseClass::GetProjectileDamage() * Min( GetCurrentCharge() / GetChargeMaxTime(), 1.0f ), 0.0f );
}


float CTFCompoundBow::GetProjectileSpeed( void )
{
	float flVelocity = 1800.0f;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	int iNoExtraSpeed = 0;
	CALL_ATTRIB_HOOK_INT(iNoExtraSpeed, chargeweapon_no_extra_speed);

	if ( iNoExtraSpeed )
		return flVelocity;

	return RemapValClamped( GetCurrentCharge(), 0.0f, 1.0f, flVelocity, 2600.0f );
}


float CTFCompoundBow::GetProjectileGravity( void )
{
	return RemapValClamped( GetCurrentCharge(), 0.0f, 1.0f, 0.5f, 0.1f );
}



void CTFCompoundBow::AddPipeBomb( CTFGrenadePipebombProjectile *pBomb )
{
}


void CTFCompoundBow::SecondaryAttack( void )
{
	LowerBow();
}

//-----------------------------------------------------------------------------
// Purpose: Un-nocks a ready arrow.
//-----------------------------------------------------------------------------
void CTFCompoundBow::LowerBow( void )
{
	if ( GetCurrentCharge() == 0.0f )
		return;

	m_flChargeBeginTime = 0.0f;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;

	m_bNoFire = true;
	m_bWantsToShoot = false;

	SendWeaponAnim( ACT_ITEM2_VM_DRYFIRE );
}


bool CTFCompoundBow::DetonateRemotePipebombs( bool bFizzle )
{
	return false;
}


bool CTFCompoundBow::OwnerCanJump( void )
{
	int bZoomJump = false;
	CALL_ATTRIB_HOOK_INT( bZoomJump, mod_sniper_zoom_while_jumping );

	if ( !bZoomJump && GetCurrentCharge() > 0.0f )
 		return false;
	
	return true;
}


bool CTFCompoundBow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

	m_bNoFire = false;
	//SetArrowAlight( false );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
bool CTFCompoundBow::SendWeaponAnim( int iActivity )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( iActivity );

	if ( iActivity == ACT_VM_PULLBACK )
	{
		iActivity = ACT_ITEM2_VM_CHARGE;
	}

	float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
	if ( GetCurrentCharge() > 0.0f )
	{
		switch ( iActivity )
		{
			case ACT_VM_IDLE:
				if (flTotalChargeTime >= TF_ARROW_MAX_CHARGE_TIME_START_ANIMATION)
				{
					int iAct = GetActivity();
					if ( iAct == TranslateViewmodelHandActivity( ACT_ITEM2_VM_IDLE_3 ) || iAct == TranslateViewmodelHandActivity( ACT_ITEM2_VM_CHARGE_IDLE_3 ) )
					{
						iActivity = ACT_ITEM2_VM_IDLE_3;
					}
					else
					{
						iActivity = ACT_ITEM2_VM_CHARGE_IDLE_3;
					}
				}
				else
				{
					iActivity = ACT_ITEM2_VM_IDLE_2;
				}
				break;
			default:
				break;
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
void CTFCompoundBow::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
		LowerBow();
	}

	// If we just fired, and we're past the point at which we tried to reload ourselves,
	// and we don't have any ammo in the clip, switch away to another weapon to stop us
	// from playing the "draw another arrow from the quiver" animation.
	if ( m_bReloadedThroughAnimEvent && m_iClip1 <= 0 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		g_pGameRules->SwitchToNextBestWeapon( pOwner, this );
		return;
	}

	BaseClass::ItemPostFrame();

	if ( !( pOwner->m_nButtons & IN_ATTACK ) && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		// Both buttons released. The player can draw the bow again.
		m_bNoFire = false;

		if ( GetActivity() == TranslateViewmodelHandActivity( ACT_ITEM2_VM_PRIMARYATTACK ) && IsViewModelSequenceFinished() )
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}

	if ( GetCurrentCharge() == 1.0f && IsViewModelSequenceFinished() )
	{
		if (IsViewModelSequenceFinished())
		{
			SendWeaponAnim(ACT_VM_IDLE);
		}
		// Let's force it to transist into special "idle2 -> transition" animation without waiting for current animation to finish if we about to reach overcharge
		// Transist animation is rather slow, so let's make it play a bit earlier with a new include const. Same change in SendWeaponAnim.
		else if (GetActivity() == TranslateViewmodelHandActivity(ACT_ITEM2_VM_IDLE_2) \
			&& (gpGlobals->curtime - m_flChargeBeginTime) >= TF_ARROW_MAX_CHARGE_TIME_START_ANIMATION)
		{
			SendWeaponAnim(ACT_VM_IDLE);
		}
	}

	if ( m_bNoFire )
	{
		WeaponIdle();
	}

	// Extinguish when underwater
	if ( pOwner->IsPlayerUnderwater() )
	{
		m_bArrowAlight = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Held the arrow drawn too long. Give up & play a fail animation.
//-----------------------------------------------------------------------------
void CTFCompoundBow::ForceLaunchGrenade( void ) 
{
	//LowerBow();
}


void CTFCompoundBow::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /*= true*/, bool bDoPassthroughCheck /*= false*/ )
{
	BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, bDoPassthroughCheck );

	float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
	if ( flTotalChargeTime >= TF_ARROW_MAX_CHARGE_TIME )
	{
		// We want to fire a really inaccurate shot.
		angForward->x += RandomFloat( -6.0f, +6.0f );
		angForward->y += RandomFloat( -6.0f, +6.0f );
	}
}


bool CTFCompoundBow::ChargeMeterShouldFlash()
{
	float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
	return flTotalChargeTime >= TF_ARROW_MAX_CHARGE_TIME;
}


void CTFCompoundBow::ApplyRefireSpeedModifications( float &flBaseRef )
{
	CALL_ATTRIB_HOOK_FLOAT( flBaseRef, fast_reload );
}

#ifdef CLIENT_DLL

void CTFCompoundBow::StartBurningEffect( void )
{
	// Clear any old effect before adding a new one.
	if ( m_pBurningArrowEffect )
	{
		StopBurningEffect();
	}

	// Sanity check.
	if ( !m_bArrowAlight )
		return;

	const char *pszEffect;
	m_hParticleEffectOwner = GetWeaponForEffect();
	if ( m_hParticleEffectOwner )
	{
		if ( m_hParticleEffectOwner != this )
		{
			// We're on the viewmodel.
			pszEffect = "v_flaming_arrow";
		}
		else
		{
			pszEffect = "flaming_arrow";
		}

		m_pBurningArrowEffect = m_hParticleEffectOwner->ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "muzzle" );
	}
}


void CTFCompoundBow::StopBurningEffect( void )
{
	if ( m_pBurningArrowEffect )
	{
		if ( m_hParticleEffectOwner && m_hParticleEffectOwner->ParticleProp() )
		{
			m_hParticleEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pBurningArrowEffect);
		}

		m_pBurningArrowEffect = NULL;
	}
}


void CTFCompoundBow::ThirdPersonSwitch( bool bThirdperson )
{
	// Switch out the burning effects.
	BaseClass::ThirdPersonSwitch( bThirdperson );
	StopBurningEffect();
	StartBurningEffect();
}


void CTFCompoundBow::UpdateOnRemove( void )
{
	StopBurningEffect();
	BaseClass::UpdateOnRemove();
}


void CTFCompoundBow::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	// Handle particle effect creation and destruction.
	if (!IsHolstered() && m_bArrowAlight && !m_pBurningArrowEffect)
	{
		StartBurningEffect();
		EmitSound( "ArrowLight" );
	}
	else if ((IsHolstered() || !m_bArrowAlight) && m_pBurningArrowEffect)
	{
		StopBurningEffect();
	}
}
#endif


bool CTFCompoundBow::Reload( void )
{
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	return BaseClass::Reload();
}


float CTFCompoundBow::OwnerMaxSpeedModifier( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimingMoveSpeed = 160.0f;
		return flAimingMoveSpeed;
	}

	return 0.0f;
}


bool CTFCompoundBow::CalcIsAttackCriticalHelper()
{ 
	// Crit boosted players fire all crits.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
		return true;

	return false; 
}


void CTFCompoundBow::SetArrowAlight( bool bAlight ) 
{ 
	// Don't light arrows if we're still firing one.
	if ( GetActivity() != TranslateViewmodelHandActivity( ACT_ITEM2_VM_PRIMARYATTACK ) ) 
	{
		m_bArrowAlight = bAlight; 
	}
}