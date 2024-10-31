//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pipebomblauncher.h"
#include "in_buttons.h"
#ifdef GAME_DLL
#include "tf_player.h"
#include "achievements_tf.h"
#endif

ConVar tf2c_stickylauncher_old_maxammo( "tf2c_stickylauncher_old_maxammo", "0", FCVAR_REPLICATED );

// Delete me and put in script
// extern ConVar tf_grenadelauncher_livetime;

#define TF_PIPEBOMB_MIN_CHARGE_VEL 900
#define TF_PIPEBOMB_MAX_CHARGE_VEL 2400
#define TF_PIPEBOMB_MAX_CHARGE_TIME 4.0f

#define TF_WEAPON_PIPEBOMB_COUNT 8
#define TF_WEAPON_PIPEBOMB_ARMTIME 0.8f

//=============================================================================
//
// Weapon Pipebomb Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPipebombLauncher, DT_WeaponPipebombLauncher )

BEGIN_NETWORK_TABLE_NOBASE( CTFPipebombLauncher, DT_PipebombLauncherLocalData )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPipebombCount ) ),
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropInt( SENDINFO( m_iPipebombCount ), 5, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()


BEGIN_NETWORK_TABLE( CTFPipebombLauncher, DT_WeaponPipebombLauncher )
#ifdef CLIENT_DLL
	RecvPropDataTable( "PipebombLauncherLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PipebombLauncherLocalData ) ),
#else
	SendPropDataTable( "PipebombLauncherLocalData", 0, &REFERENCE_SEND_TABLE( DT_PipebombLauncherLocalData ), SendProxy_SendLocalWeaponDataTable ),
#endif	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFPipebombLauncher )
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iPipebombCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_pipebomblauncher, CTFPipebombLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_pipebomblauncher );

//=============================================================================
//
// Weapon Pipebomb Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::CTFPipebombLauncher()
{
	m_bReloadsSingly = true;
	m_flLastDenySoundTime = 0.0f;
	m_bNoAutoRelease = false;
	m_bWantsToShoot = false;
	m_bInDetonation = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::~CTFPipebombLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0.0f;
	StopWeaponSound( WPN_DOUBLE );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0.0f;

	return BaseClass::Deploy();
}


void CTFPipebombLauncher::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifndef CLIENT_DLL
	DetonateRemotePipebombs( true );

	m_flChargeBeginTime = 0.0f;
	StopWeaponSound( WPN_DOUBLE );
#endif
}


void CTFPipebombLauncher::PrimaryAttack( void )
{
	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0.0f;
		return;
	}

	if ( m_flChargeBeginTime <= 0.0f )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// Save that we had the attack button down.
		m_flChargeBeginTime = gpGlobals->curtime;
		WeaponSound( WPN_DOUBLE );

		SendWeaponAnim( ACT_VM_PULLBACK );
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
		if ( flTotalChargeTime >= GetChargeMaxTime() )
		{
			LaunchGrenade();
		}
	}
}


void CTFPipebombLauncher::LaunchGrenade( void )
{
	// Deliberately skipping to base class since our function starts charging the shot.
	BaseClass::PrimaryAttack();

	m_flLastDenySoundTime = gpGlobals->curtime;
	m_flChargeBeginTime = 0.0f;
}

float CTFPipebombLauncher::GetProjectileSpeed( void )
{
	float flVelocity = TF_PIPEBOMB_MIN_CHARGE_VEL;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	int iNoExtraSpeed = 0;
	CALL_ATTRIB_HOOK_INT(iNoExtraSpeed, chargeweapon_no_extra_speed);

	if ( iNoExtraSpeed )
		return flVelocity;

	float flForwardSpeed = RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		GetChargeMaxTime(),
		flVelocity,
		TF_PIPEBOMB_MAX_CHARGE_VEL );

	return flForwardSpeed;
}

void CTFPipebombLauncher::AddPipeBomb( CTFGrenadeStickybombProjectile *pBomb )
{
	PipebombHandle hHandle;
	hHandle = pBomb;
	m_Pipebombs.AddToTail( hHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Add pipebombs to our list as they're fired
//-----------------------------------------------------------------------------
CBaseEntity *CTFPipebombLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
#ifdef GAME_DLL
	if ( pProjectile )
	{
		int proxyMine = 0;
		CALL_ATTRIB_HOOK_INT(proxyMine, mod_sticky_is_proxy);

		int forceNoTumble = 0;
		CALL_ATTRIB_HOOK_INT(proxyMine, mod_sticky_no_tumble);

		// If we've gone over the max pipebomb count, detonate the oldest.
		if ( m_Pipebombs.Count() >= GetMaxPipeBombCount() )
		{
			CTFGrenadeStickybombProjectile *pTemp = m_Pipebombs[0];
			if ( pTemp )
			{
				if (proxyMine > 0)
				{
					if (pTemp->GetDetonateTime() == FLT_MAX)
					{
						pTemp->Fizzle();	// KA-BOOOM big not
						pTemp->SetTimer(gpGlobals->curtime);
					}
				}
				else
				{
					pTemp->SetTimer(gpGlobals->curtime); // KA-BOOOM!
				}
			}

			m_Pipebombs.Remove( 0 );
		}

		CTFGrenadeStickybombProjectile *pPipebomb = dynamic_cast<CTFGrenadeStickybombProjectile *>( pProjectile );
		if ( pPipebomb )
		{
			AddPipeBomb( pPipebomb );

			pPipebomb->SetChargeBeginTime( m_flChargeBeginTime );

			if ( proxyMine )
				pPipebomb->SetProxyMine( true );

			if( proxyMine || forceNoTumble )
			{
				IPhysicsObject *pPhysicsObject = pPipebomb->VPhysicsGetObject();
				if ( pPhysicsObject )
				{
					// Disk shape doesn't tumble.
					AngularImpulse angImpulse( random->RandomFloat( -30.0f, 30.0f ), random->RandomFloat( -30.0f, 30.0f ), 180.0f );
					pPhysicsObject->SetVelocityInstantaneous( NULL, &angImpulse );
				}
			}
		}

		m_iPipebombCount = m_Pipebombs.Count();
	}
#endif

	return pProjectile;
}


void CTFPipebombLauncher::ItemPostFrame( void )
{
	// Allow player to fire and detonate at the same time.
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		if ( m_flChargeBeginTime > 0.0f )
		{
			LaunchGrenade();
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs if secondary fire is down.
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemBusyFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// We need to do this to catch the case of player trying to detonate
		// pipebombs while in the middle of reloading.
		SecondaryAttack();
	}

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs if secondary fire is down.
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemHolsterFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// sticky det delay finally fixed!!!!!!!
		SecondaryAttack();
	}

	BaseClass::ItemHolsterFrame();
}


//-----------------------------------------------------------------------------
// Purpose: Detonate active pipebombs
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

	int iStickyFizzles = 0;
	CALL_ATTRIB_HOOK_INT( iStickyFizzles, mod_sticky_fizzles );

	int iNoDetonation = 0;
	CALL_ATTRIB_HOOK_INT( iNoDetonation, mod_sticky_no_detonation );

	if ( m_iPipebombCount )
	{
		// If one or more pipebombs failed to detonate then play a sound.
		if ( iNoDetonation > 0 || ( iStickyFizzles > 0 && DetonateRemotePipebombs( true, false ) ) || DetonateRemotePipebombs( false, false ) )
		{
			if ( m_flLastDenySoundTime <= gpGlobals->curtime )
			{
				// Deny!
				m_flLastDenySoundTime = gpGlobals->curtime + 1.0f;
				WeaponSound( SPECIAL2 );
				return;
			}
		}
		else
		{
			// Play a detonate sound.
			WeaponSound( SPECIAL3 );
		}
	}
}

//=============================================================================
//
// Server specific functions.
//
#ifdef GAME_DLL

void CTFPipebombLauncher::UpdateOnRemove( void )
{
	// If we just died, we want to fizzle our pipebombs.
	// If the player switched classes, our pipebombs have already been removed.
	DetonateRemotePipebombs( true );

	BaseClass::UpdateOnRemove();
}

// TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY
void CTFPipebombLauncher::MarkMinesForAchievementJumpAndDestroy(CBaseEntity* pJumpMine)
{
	m_PipebombsAchievementJumpAndDestroy.Purge();
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner && pOwner->IsAlive())
	{
		FOR_EACH_VEC(m_Pipebombs, i)
		{
			CTFGrenadeStickybombProjectile* pMine = m_Pipebombs[i];
			if (pMine && pMine->IsProxyMine())
			{
				if (pMine != pJumpMine)
				{
					m_PipebombsAchievementJumpAndDestroy.AddToTail(pMine);
				}
			}
		}
	}
}

// TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY
bool CTFPipebombLauncher::IsMineForAchievementJumpAndDestroy(CBaseEntity* pKillerMine)
{
	if (!pKillerMine)
		return false;
	FOR_EACH_VEC(m_PipebombsAchievementJumpAndDestroy, i)
	{
		if (m_PipebombsAchievementJumpAndDestroy[i] == pKillerMine)
		{
			m_PipebombsAchievementJumpAndDestroy.Purge();
			return true;
		}
	}
	return false;
}

#endif


//-----------------------------------------------------------------------------
// Purpose: If a pipebomb has been removed, remove it from our list
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::DeathNotice( CBaseEntity *pVictim )
{
	PipebombHandle hHandle = assert_cast<CTFGrenadeStickybombProjectile *>( pVictim );
	m_Pipebombs.FindAndRemove( hHandle );

	m_iPipebombCount = m_Pipebombs.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Remove *with* explosions
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::DetonateRemotePipebombs( bool bFizzle, bool bPlayerDeath )
{
	// Guard in case player kills himself in detonation to prevent the remaining stickies from fizzling.
	if ( m_bInDetonation )
	{
#ifdef GAME_DLL
		if ( bFizzle )
		{
			// Fizzle any stickies that failed to detonate on the next tick.
			SetContextThink( &CTFPipebombLauncher::FizzlePipebombs, gpGlobals->curtime, "DetonateThink" );
		}
#endif

		return false;
	}

	m_bInDetonation = true;
	bool bFailedToDetonate = false;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	for ( int i = m_Pipebombs.Count() - 1; i >= 0; i-- )
	{
		CTFGrenadeStickybombProjectile *pTemp = m_Pipebombs[i];
		if ( !pTemp )
			continue;

		if ( ( gpGlobals->curtime - pTemp->GetCreationTime() ) < TF_WEAPON_PIPEBOMB_ARMTIME && pOwner && pOwner->GetHealth() > 0 )
		{
			bFailedToDetonate = true;
			continue;
		}

#ifdef GAME_DLL
		if (bFizzle)
		{
			pTemp->Fizzle();


			// TF2C_ACHIEVEMENT_MINES_PREVENT_SELF_DETONATION
			/*if (pTemp->m_pProxyTarget && pOwner && pOwner == pTemp->m_pProxyTarget && !bPlayerDeath)
			{
				CTFPlayer* pTFOwner = ToTFPlayer(pOwner);
				if (pTFOwner)
				{

					Vector vecSegment;

					VectorSubtract(pOwner->GetAbsOrigin(), pTemp->GetAbsOrigin(), vecSegment); // approx cus we could use gamer
					float flDist2 = vecSegment.LengthSqr();
					if (flDist2 <= Square(pTemp->GetDamageRadius()))
					{
						pTFOwner->AwardAchievement(TF2C_ACHIEVEMENT_MINES_PREVENT_SELF_DETONATION);
					}
				}
			}*/
		}

		pTemp->Detonate();
#else
		m_Pipebombs.FindAndRemove( m_Pipebombs[i] );
#endif
	}

	m_iPipebombCount = m_Pipebombs.Count();
	m_bInDetonation = false;

#ifdef GAME_DLL
	m_PipebombsAchievementJumpAndDestroy.Purge();
#endif

	return bFailedToDetonate;
}


void CTFPipebombLauncher::FizzlePipebombs( void )
{
	DetonateRemotePipebombs( true );
}


float CTFPipebombLauncher::GetChargeMaxTime( void )
{
	float flChargeTime = TF_PIPEBOMB_MAX_CHARGE_TIME;
	CALL_ATTRIB_HOOK_FLOAT( flChargeTime, stickybomb_charge_rate );
	return flChargeTime;
}


float CTFPipebombLauncher::GetCurrentCharge( void )
{
	if ( m_flChargeBeginTime == 0.0f )
		return 0.0f;

	return Min( gpGlobals->curtime - m_flChargeBeginTime, GetChargeMaxTime() );
}


bool CTFPipebombLauncher::Reload( void )
{
	if ( m_flChargeBeginTime > 0.0f )
		return false;

	return BaseClass::Reload();
}


int CTFPipebombLauncher::GetMaxAmmo( void )
{
	return BaseClass::GetMaxAmmo() + ( tf2c_stickylauncher_old_maxammo.GetBool() ? 16 : 0 );
}


int CTFPipebombLauncher::GetMaxPipeBombCount() const
{
	int nMaxPipebombs = TF_WEAPON_PIPEBOMB_COUNT;
	CALL_ATTRIB_HOOK_INT( nMaxPipebombs, add_max_pipebombs );
	return nMaxPipebombs;
}
