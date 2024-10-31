//=============================================================================
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_coilgun.h"
#include "tf_lagcompensation.h"
#include "in_buttons.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_baseprojectile.h"
#include "func_nogrenades.h"
#include "Sprite.h"
#include "tf_fx.h"
#else
#include "c_tf_player.h"
#endif

#ifdef GAME_DLL
ConVar tf_debug_coilgun( "tf_debug_coilgun", "0", FCVAR_CHEAT );
#endif

ConVar tf2c_coilgun_charge_time( "tf2c_coilgun_charge_time", "1.5", FCVAR_REPLICATED );
ConVar tf2c_coilgun_explode_time( "tf2c_coilgun_explode_time", "1.5", FCVAR_REPLICATED );

IMPLEMENT_NETWORKCLASS_ALIASED( TFCoilGun, DT_TFCoilGun )
BEGIN_NETWORK_TABLE( CTFCoilGun, DT_TFCoilGun )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCoilGun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_coilgun, CTFCoilGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_coilgun );

CTFCoilGun::CTFCoilGun()
{
#ifdef CLIENT_DLL
	m_pChargeSound = NULL;
	m_bDidBeep = false;
#endif
}


void CTFCoilGun::WeaponReset( void )
{
	BaseClass::WeaponReset();
	StopCharging();
#ifdef CLIENT_DLL
	m_bDidBeep = false;
#endif
}


void CTFCoilGun::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "TFPlayer.ReCharged" );
	PrecacheTeamParticles( "coilgun_boom_%s" );
}


bool CTFCoilGun::CanHolster( void ) const
{
	if ( m_flChargeBeginTime != 0.0f )
		return false;
	
	return BaseClass::CanHolster();
}


bool CTFCoilGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopCharging();

	return BaseClass::Holster( pSwitchingTo );
}


void CTFCoilGun::ItemPostFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( m_flChargeBeginTime != 0.0f )
	{
		if ( !( pPlayer->m_nButtons & IN_ATTACK2 ) )
		{
			PrimaryAttack();
			StopCharging();
		}
#ifdef CLIENT_DLL
		else if (!m_bDidBeep && gpGlobals->curtime - m_flChargeBeginTime >= GetCoilChargeTime() && GetOwner() == C_TFPlayer::GetLocalTFPlayer())
		{
			m_bDidBeep = true;
			C_TFPlayer::GetLocalTFPlayer()->EmitSound("TFPlayer.ReCharged");
		}
#endif
	}

	BaseClass::ItemPostFrame();
}


void CTFCoilGun::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	/*
	if ( HasPrimaryAmmoToFire() )
	{
		if ( gpGlobals->curtime >= m_flNextEmptySoundTime )
		{
			WeaponSound( EMPTY );
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5f;
		}
		return;
	}
	*/

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->NoteWeaponFired( this );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	
	// Play different shoot sound when at full charge.
	if ( m_flChargeBeginTime != 0.0f && gpGlobals->curtime - m_flChargeBeginTime >= GetCoilChargeTime() )
	{
		WeaponSound( WPN_DOUBLE );
	}
	else
	{
		WeaponSound( SINGLE );
	}

	BaseClass::PrimaryAttack();

	// Set next attack times.
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() + ( m_flChargeBeginTime != 0.0f ? TF_COILGUN_CHARGE_FIRE_DELAY : 0.0f );

}


void CTFCoilGun::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0.0;
		return;
	}

	if ( !HasPrimaryAmmoToFire() )
	{
		StopCharging();
		Reload();
		return;
	}

	AbortReload();
	CalcIsAttackCritical();

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;

		SendWeaponAnim( ACT_VM_PULLBACK );
#ifdef CLIENT_DLL
		UpdateChargeEffects();
		m_bDidBeep = false;
#endif
	}
	else if ( gpGlobals->curtime - m_flChargeBeginTime >= GetCoilChargeTime() + GetCoilChargeExplodeTime() )
	{
		// Explode if charged for too long.
		WeaponSound( SPECIAL2 );
		StopCharging();

		int iAmmoCost = Min( abs( m_iClip1.Get() ), GetAmmoPerShot() );

		if (m_iClip1.Get() < 0)
			iAmmoCost = GetAmmoPerShot();

		if ( UsesClipsForAmmo1() )
		{
			m_iClip1 -= iAmmoCost;
		}
		else
		{
			pPlayer->RemoveAmmo( iAmmoCost, m_iPrimaryAmmoType );
		}

		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

		// Longer post-fire delay.
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetTFWpnData().GetWeaponData( TF_WEAPON_SECONDARY_MODE ).m_flTimeFireDelay;

#ifdef GAME_DLL
		CDisablePredictionFiltering disabler;

		Vector vecBlastOrigin = pPlayer->WorldSpaceCenter();
		float flDamage = GetTFWpnData().GetWeaponData(TF_WEAPON_SECONDARY_MODE).m_flDamage;
		float flRadius = GetTFWpnData().m_flDamageRadius;
		int iDmgType = DMG_BLAST | DMG_HALF_FALLOFF;

		if ( m_bCurrentAttackIsCrit )
		{
			iDmgType |= DMG_CRITICAL;
		}

		CTakeDamageInfo damageInfo (pPlayer, pPlayer, this, vec3_origin, vecBlastOrigin, flDamage, iDmgType);

		CTFRadiusDamageInfo radiusInfo;
		radiusInfo.info = damageInfo;
		radiusInfo.m_vecSrc = vecBlastOrigin;
		radiusInfo.m_flRadius = flRadius;
		radiusInfo.m_bStockSelfDamage = false;
		radiusInfo.m_pEntityIgnore = pPlayer;

		TFGameRules()->RadiusDamage( radiusInfo );
		pPlayer->TakeDamage(damageInfo);
#endif

		const char *pszEffect = ConstructTeamParticle( "coilgun_boom_%s", GetTeamNumber() );
		DispatchParticleEffect( pszEffect, pPlayer->WorldSpaceCenter(), vec3_angle );
	}
}


void CTFCoilGun::UpdateOnRemove( void )
{
	StopCharging();

	BaseClass::UpdateOnRemove();
}


void CTFCoilGun::StopCharging( void )
{
	m_flChargeBeginTime = 0.0f;
#ifdef CLIENT_DLL
	UpdateChargeEffects();
#endif
}


int CTFCoilGun::GetDamageType() const
{
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	if ( m_bCurrentAttackIsCrit )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

#ifdef GAME_DLL

int CTFCoilGun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

#ifdef CLIENT_DLL

void CTFCoilGun::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasCharging = ( m_flChargeBeginTime != 0.0f );
}


void CTFCoilGun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateChargeEffects();
}


void CTFCoilGun::UpdateChargeEffects( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_flChargeBeginTime != 0.0f )
	{
		if ( !m_pChargeSound )
		{
			CLocalPlayerFilter filter;
			m_pChargeSound = controller.SoundCreate( filter, pPlayer->entindex(), GetShootSound( SPECIAL3 ) );
			controller.Play( m_pChargeSound, 1.0f, 50 );
			controller.SoundChangePitch( m_pChargeSound, 150, GetCoilChargeTime() );
		}
	}
	else
	{
		if ( m_pChargeSound )
		{
			controller.SoundDestroy( m_pChargeSound );
			m_pChargeSound = NULL;
		}
	}
}
#endif


float CTFCoilGun::GetCoilChargeTime( void )
{
	float flCoilChargeTime = tf2c_coilgun_charge_time.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT( flCoilChargeTime, mod_charge_time );

	// Faster charge rate during Haste buff
	CTFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flCoilChargeTime *= 0.33f;
	}

	return flCoilChargeTime;
}


float CTFCoilGun::GetCoilChargeExplodeTime( void )
{
	float flCoilChargeExplodeTime = tf2c_coilgun_explode_time.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT( flCoilChargeExplodeTime, mod_coilgun_explode_time );

	// Faster charge rate during Haste buff
	CTFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flCoilChargeExplodeTime *= 0.33f;
	}

	return flCoilChargeExplodeTime;
}


float CTFCoilGun::GetChargeMaxTime( void )
{
	return GetCoilChargeTime() + GetCoilChargeExplodeTime();
}


float CTFCoilGun::GetProgress( void )
{
	if ( m_flChargeBeginTime != 0.0f )
	{
		float flMaxProgress = GetChargeMaxTime();
		return RemapValClamped( gpGlobals->curtime - m_flChargeBeginTime, flMaxProgress, 0.0f, 1.0f, 0.0f );
	}

	return 0.0f;
}


bool CTFCoilGun::ChargeMeterShouldFlash( void )
{
	return GetCurrentCharge() >= GetCoilChargeTime();
}


float CTFCoilGun::GetProjectileDamage( void )
{
	float flChargeDamage = BaseClass::GetProjectileDamage();
	float flMaxDamage = TF_COILGUN_MAX_CHARGE_DAMAGE;
	CALL_ATTRIB_HOOK_FLOAT( flMaxDamage, mult_dmg );

	if ( m_flChargeBeginTime <= 0.0f )
		return flChargeDamage;

	return RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		GetCoilChargeTime(),
		flChargeDamage,
		flMaxDamage );
}


float CTFCoilGun::GetProjectileSpeed( void )
{
	if ( m_flChargeBeginTime <= 0.0f )
		return TF_COILGUN_MIN_CHARGE_VEL;

	float flVelocity = TF_COILGUN_MIN_CHARGE_VEL;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	int iNoExtraSpeed = 0;
	CALL_ATTRIB_HOOK_INT(iNoExtraSpeed, chargeweapon_no_extra_speed);

	if ( iNoExtraSpeed )
		return flVelocity;

	return RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		GetCoilChargeTime(),
		flVelocity,
		TF_COILGUN_MAX_CHARGE_VEL);
}


float CTFCoilGun::GetWeaponSpread( void )
{
	float flSpread = BaseClass::GetWeaponSpread();

	if ( m_flChargeBeginTime <= 0.0f )
		return flSpread;

	return RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		GetCoilChargeTime(),
		flSpread,
		0.0f );
}


float CTFCoilGun::GetCurrentCharge( void )
{
	if ( m_flChargeBeginTime == 0.0f )
		return 0.0f;

	return Min( gpGlobals->curtime - m_flChargeBeginTime, GetCoilChargeTime() );
}




/* Projectile deletion code

int iAmmoCost = GetAmmoPerShot();

if ( m_flChargeBeginTime != 0.0f )
{
float flMult = RemapValClamped( gpGlobals->curtime - m_flChargeBeginTime,
0.0f, GetCoilChargeTime(),
1.0f, TF_COILGUN_CHARGE_AMMO_MULT );
iAmmoCost = (int)( (float)iAmmoCost * flMult );
}

pPlayer->RemoveAmmo( iAmmoCost, m_iPrimaryAmmoType );

#ifdef GAME_DLL
START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

Vector vecDir;
QAngle angDir = pPlayer->EyeAngles();
AngleVectors( angDir, &vecDir );

float flMult = 1.0f;

if ( m_flChargeBeginTime != 0.0f )
{
flMult = RemapValClamped( gpGlobals->curtime - m_flChargeBeginTime,
0.0f, GetCoilChargeTime(),
1.0f, 2.0f );
}

const Vector vecBlastSize( 128 * flMult, 128 * flMult, 64 );
Vector vecOrigin = pPlayer->Weapon_ShootPosition() + vecDir * ( 128 * flMult );

if ( tf_debug_coilgun.GetBool() )
{
NDebugOverlay::Box( vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0f );
}

CBaseEntity *pList[64];
int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0 );

for ( int i = 0; i < count; i++ )
{
CBaseEntity *pEntity = pList[i];

if ( !pEntity || !pEntity->IsDeflectable() || !pPlayer->IsEnemy( pEntity ) )
continue;

// Make sure we can actually see this entity so we don't hit anything through walls.
if ( !pPlayer->FVisible( pEntity, MASK_SOLID ) )
continue;

CDisablePredictionFiltering disabler;

if ( pEntity->IsPlayer() )
{
// Damage players.
float flDist = ( pEntity->WorldSpaceCenter() - pPlayer->WorldSpaceCenter() ).Length();
float flDamage = GetProjectileDamage();

// Damage falls off over distance.
flDamage = RemapValClamped( flDist, 0.0f, 512.0f, flDamage, flDamage * 0.25f );

CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, GetDamageType() );
pEntity->TakeDamage( info );
}
else
{
CTFBaseProjectile *pProjectile = dynamic_cast<CTFBaseProjectile *>( pEntity );

if ( pProjectile && pProjectile->GetWeaponID() != TF_WEAPON_GRENADE_MIRV && pProjectile->GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB )
{
// Destroy anything deflectable.
UTIL_Remove( pProjectile );

// TEMP: Sprite flash
CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
if ( pGlowSprite )
{
pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
pGlowSprite->SetThink( &CSprite::SUB_Remove );
pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
}

if ( m_flChargeBeginTime != 0.0f && gpGlobals->curtime - m_flChargeBeginTime >= GetCoilChargeTime() )
{
pPlayer->GiveAmmo( 30, TF_AMMO_METAL, true, TF_AMMO_SOURCE_RESUPPLY );
}
}
}
}

FINISH_LAG_COMPENSATION();
#endif

m_flLastPrimaryAttackTime = gpGlobals->curtime;

DoFireEffects();

UpdatePunchAngles( pPlayer );
}

*/
