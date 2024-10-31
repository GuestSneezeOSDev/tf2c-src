//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_paintballrifle.h"
#include "tf_weapon_sniperrifle.h"
#include "in_buttons.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "functionproxy.h"
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

extern ConVar tf2c_medigun_setup_uber;
extern ConVar tf2c_medigun_multi_uber_drain;
extern ConVar tf2c_medigun_critboostable;
extern ConVar tf2c_medigun_4team_uber_rate;
extern ConVar weapon_medigun_damage_modifier;
extern ConVar weapon_medigun_construction_rate;
extern ConVar weapon_medigun_charge_rate;
extern ConVar weapon_medigun_chargerelease_rate;

//=============================================================================
//
// Weapon Paintball Rifle tables.
//

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Healing list UtlVector to entindices
//-----------------------------------------------------------------------------
void SendProxy_MarkedList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFPaintballRifle *pPainter = (CTFPaintballRifle *)pStruct;

	// If this assertion fails, then SendProxyArrayLength_HealingArray must have failed.
	Assert( iElement < pPainter->m_vAutoHealTargets.Count() );

	EHANDLE hOther = pPainter->m_vAutoHealTargets[iElement].Get();
	SendProxy_EHandleToInt( pProp, pStruct, &hOther, pOut, iElement, objectID );
}

int SendProxyArrayLength_MarkedArray( const void *pStruct, int objectID )
{
	return ((CTFPaintballRifle *)pStruct)->m_vAutoHealTargets.Count();
}
#else
//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes.
//-----------------------------------------------------------------------------
void RecvProxy_MarkedList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTFPaintballRifle *pPainter = (CTFPaintballRifle *)pStruct;

	CBaseHandle *pHandle = (CBaseHandle *)(&pPainter->m_vAutoHealTargets[pData->m_iElement]);
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );

	// Update the heal beams.
	pPainter->UpdateBackpackTargets();
}

void RecvProxyArrayLength_MarkedArray( void *pStruct, int objectID, int currentArrayLength )
{
	CTFPaintballRifle *pPainter = (CTFPaintballRifle *)pStruct;
	if ( pPainter->m_vAutoHealTargets.Size() != currentArrayLength )
	{
		pPainter->m_vAutoHealTargets.SetSize( currentArrayLength );
	}

	// Update the beams.
	pPainter->UpdateBackpackTargets();
}

void RecvProxy_MainPatient( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTFPaintballRifle *pMedigun = (CTFPaintballRifle *)pStruct;
	if ( pMedigun )
	{
		pMedigun-> // Trigger main target update
			m_bMainTargetParity = !pMedigun->m_bMainTargetParity;
	}

	RecvProxy_IntToEHandle( pData, pStruct, pOut );
}

#endif

//CREATE_SIMPLE_WEAPON_TABLE( TFPaintballRifle, tf_weapon_paintballrifle )

LINK_ENTITY_TO_CLASS( tf_weapon_paintballrifle, CTFPaintballRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_paintballrifle );
IMPLEMENT_NETWORKCLASS_ALIASED( TFPaintballRifle, DT_TFPaintballRifle )
BEGIN_NETWORK_TABLE( CTFPaintballRifle, DT_TFPaintballRifle )
#if !defined( CLIENT_DLL )
SendPropBool( SENDINFO( m_bBackpackTargetsParity ) ),
SendPropArray2(
SendProxyArrayLength_MarkedArray,
SendPropInt( "paintballbp_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_MarkedList ),
MAX_PLAYERS,
0,
"paintballbp_array"
),
SendPropEHandle( SENDINFO( m_hMainPatient ) ),
SendPropInt( SENDINFO( m_iMainPatientHealthLast ) ),
SendPropBool( SENDINFO( m_bMainTargetParity ) ),
SendPropTime( SENDINFO( m_flUnzoomTime ) ),
SendPropTime( SENDINFO( m_flRezoomTime ) ),
SendPropBool( SENDINFO( m_bRezoomAfterShot ) ),
SendPropFloat( SENDINFO( m_flChargeLevel ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
SendPropBool( SENDINFO( m_bChargeRelease ) ),
#else
RecvPropBool( RECVINFO( m_bBackpackTargetsParity ) ),
RecvPropArray2(
RecvProxyArrayLength_MarkedArray,
RecvPropInt( "paintballbp_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_MarkedList ),
MAX_PLAYERS,
0,
"paintballbp_array"
),
RecvPropEHandle( RECVINFO( m_hMainPatient ), RecvProxy_MainPatient ),
RecvPropInt( RECVINFO( m_iMainPatientHealthLast ) ),
RecvPropBool( RECVINFO( m_bMainTargetParity ) ),
RecvPropTime( RECVINFO( m_flUnzoomTime ) ),
RecvPropTime( RECVINFO( m_flRezoomTime ) ),
RecvPropBool( RECVINFO( m_bRezoomAfterShot ) ),
RecvPropFloat( RECVINFO( m_flChargeLevel ) ),
RecvPropBool( RECVINFO( m_bChargeRelease ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPaintballRifle )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_bUpdateBackpackTargets, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

//=============================================================================
//
// Weapon Paintball Rifle functions.
//


CTFPaintballRifle::CTFPaintballRifle()
{
	m_bMainTargetParity = false;
#ifdef CLIENT_DLL
	m_bMainTargetParityOld = false;
	m_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );

	// Not sure which of these two methods is better.
	//gameeventmanager->AddListener( this, "patient_healed_notify", false );
	ListenForGameEvent( "patient_healed_notify" );
#else
	m_bMainPatientFlaggedForRemoval = false;
#endif
}

CTFPaintballRifle::~CTFPaintballRifle()
{
#ifdef CLIENT_DLL
	if ( m_pHealSoundManager )
	{
		m_pHealSoundManager->RemoveSound();
		delete m_pHealSoundManager;
		m_pHealSoundManager = NULL;
	}
#endif
}


void CTFPaintballRifle::WeaponReset()
{
	BaseClass::WeaponReset();
	
	m_flChargeLevel = 0.0f;
	m_bChargeRelease = false;

	m_bMainTargetParity = false;
#ifdef CLIENT_DLL
	m_bMainTargetParityOld = false;
	if ( m_pHealSoundManager )
	{
		m_pHealSoundManager->RemoveSound();
		delete m_pHealSoundManager;
		m_pHealSoundManager = NULL;
	}
	m_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );

	UpdateEffects();	

	m_flNextBuzzTime = 0;
#else
	m_bMainPatientFlaggedForRemoval = false;

	RemoveAllHealTargets();

	if ( m_pUberTarget && GetTFPlayerOwner() ) {
		m_pUberTarget->m_Shared.StopHealing( GetTFPlayerOwner(), HEALER_TYPE_BEAM );
		m_pUberTarget = NULL;
	}
#endif

	m_bBuiltChargeThisFrame = false;

	ZoomOut();
}

void CTFPaintballRifle::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	if ( m_pHealSoundManager )
		m_pHealSoundManager->RemoveSound();
#else
	RemoveAllHealTargets();
#endif

	BaseClass::UpdateOnRemove();
}

medigun_charge_types CTFPaintballRifle::GetChargeType( void )
{
	int iChargeType = TF_CHARGE_INVULNERABLE;
	CALL_ATTRIB_HOOK_INT( iChargeType, set_charge_type );
	if ( iChargeType > TF_CHARGE_NONE && iChargeType < TF_CHARGE_COUNT )
		return (medigun_charge_types)iChargeType;

	AssertMsg( 0, "Invalid charge type!\n" );
	return TF_CHARGE_NONE;
}

void CTFPaintballRifle::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	int bBlockZoom = 0;
	CALL_ATTRIB_HOOK_INT( bBlockZoom, unimplemented_mod_sniper_no_charge );
	if ( bBlockZoom > 0 )
		return;

	// Unzoom if we're jumping or taunting.
	if ( pPlayer->m_Shared.IsJumping() || pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		//ResetTimers();
		return;
	}

	if ( m_flUnzoomTime > 0 && gpGlobals->curtime > m_flUnzoomTime )
	{
		if ( m_bRezoomAfterShot )
		{
			ZoomOutIn();
			m_bRezoomAfterShot = false;
		}
		else
		{
			ZoomOut();
		}

		m_flUnzoomTime = -1;
	}

	if ( m_flRezoomTime > 0 )
	{
		if ( gpGlobals->curtime > m_flRezoomTime )
		{
			ZoomIn();
			m_flRezoomTime = -1;
		}
	}

	if ( pPlayer->ShouldHoldToZoom() )
	{
		if ( pPlayer->m_nButtons & IN_ATTACK2 )
		{
			if ( !IsZoomed() )
			{
				if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
				{
					// If we're in the process of rezooming, just cancel it
					if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
					{
						// Prevent them from rezooming in less time than they would have
						m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
						m_flRezoomTime = -1;
					}
					else
					{
						Zoom();
					}
				}

				// Zoom();
			}
		}
		else
		{
			if ( IsZoomed() )
			{
				Zoom();
			}
		}
	}
	else
	{
		if ( (pPlayer->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
		{
			// If we're in the process of rezooming, just cancel it
			if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
			{
				// Prevent them from rezooming in less time than they would have
				m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
				m_flRezoomTime = -1;
			}
			else
			{
				Zoom();
			}
		}
	}
}

float CTFPaintballRifle::OwnerMaxSpeedModifier( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimingMoveSpeed = 120.0f;
		return flAimingMoveSpeed;
	}

	return 0.0f;
}

bool CTFPaintballRifle::CanHolster( void ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( (pPlayer->GetFOV() < pPlayer->GetDefaultFOV()) && (m_flNextPrimaryAttack > gpGlobals->curtime) )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

bool CTFPaintballRifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
#ifdef CLIENT_DLL
	if ( m_pHealSoundManager )
	{
		if ( !m_pHealSoundManager->IsSetupCorrectly() )
		{
			m_pHealSoundManager->RemoveSound();
			delete m_pHealSoundManager;
			m_pHealSoundManager = NULL;

			m_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );
		}
	}

	UpdateEffects();

	//ManageChargeEffect();

#else
	if ( !GetTFPlayerOwner()->IsAlive() )
		RemoveAllHealTargets();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}

	return BaseClass::Holster( pSwitchingTo );
}

bool CTFPaintballRifle::Deploy( void )
{
#ifdef CLIENT_DLL
	//ManageChargeEffect();

		if ( m_pHealSoundManager )
		{
			if ( !m_pHealSoundManager->IsSetupCorrectly() )
			{
				m_pHealSoundManager->RemoveSound();
				delete m_pHealSoundManager;
				m_pHealSoundManager = NULL;
	
				m_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );
			}
		}
#endif
	return BaseClass::Deploy();
}

void CTFPaintballRifle::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();
#ifdef GAME_DLL
	ApplyBackpackHealing();
#else
#if 0
	m_pHealSoundManager->SetPatient( ToTFPlayer(m_hMainPatient.Get()) );
	m_pHealSoundManager->SoundManagerThink();
#endif
#endif

	m_bBuiltChargeThisFrame = false;

	DrainCharge();
}

void CTFPaintballRifle::ItemPostFrame( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	//BaseClass::ItemPostFrame();
#ifdef GAME_DLL
	ApplyBackpackHealing();
#else
#if 0
	m_pHealSoundManager->SetPatient( ToTFPlayer( m_hMainPatient.Get() ) );
	m_pHealSoundManager->SoundManagerThink();
#endif
#endif

	m_bBuiltChargeThisFrame = false;
	
	if ( !IsZoomed() )
		CheckReload();

	// Interrupt a reload.
	if ( IsReloading() && Clip1() > 0 && (pPlayer->m_nButtons & IN_ATTACK) )
	{
		AbortReload();
	}

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}

	if ( pPlayer->m_nButtons & IN_ATTACK3 )
		TertiaryAttack();

	// Idle.
	if ( !((pPlayer->m_nButtons & IN_ATTACK) || (pPlayer->m_nButtons & IN_ATTACK2)) )
	{
		// No fire buttons down or reloading
		if ( (!ReloadOrSwitchWeapons() && !IsReloading()) || IsZoomed() )
		{
			WeaponIdle();
		}
	}

	// Reload.
	if ( !IsZoomed() )
	{
		if ( (pPlayer->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() )
		{
			Reload();
		}

		if ( IsReloading() && IsZoomed() )
		{
			ToggleZoom();
		}
	}
}

void CTFPaintballRifle::TertiaryAttack()
{
	//if ( !CanAttack() )
	//	return;

	// Ensure they have a full charge 
	if ( m_flChargeLevel < GetMinChargeAmount() || m_bChargeRelease )
	{
#ifdef CLIENT_DLL
		// Deny, buzz.
		if ( gpGlobals->curtime >= m_flNextBuzzTime )
		{
			EmitSound( "Player.DenyWeaponSelection" );
			m_flNextBuzzTime = gpGlobals->curtime + 0.5f; // Only buzz every so often.
		}
#endif
		return;
	}

#ifdef GAME_DLL

	CTFPlayer *pOwner = assert_cast<CTFPlayer *>(GetOwner());

	trace_t tr;
	UTIL_TraceLine( pOwner->EyePosition(), pOwner->EyePosition() + pOwner->EyeDirection3D() * MAX_TRACE_LENGTH, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction >= 1 )
		return;

	CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
	if ( !pTarget ) {
		//DevMsg( "Could not Uber: pEnt was not a player.\n" );
		return;
	}
	if ( pTarget->IsEnemy( pOwner ) ) {
		//DevMsg( "Could not Uber: player was an enemy.\n" );
		return;
	}

	// Stop the player shootin' for a lil while
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_bChargeRelease = true;
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pOwner->NoteWeaponFired( this );

	pTarget->m_Shared.Heal( pOwner, 0 );
	pTarget->m_Shared.RecalculateChargeEffects();
	m_pUberTarget = pTarget;

	// Make weaponbase load our secondary projectile type!
	// m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

	//CTF_GameStats.Event_PlayerInvulnerable( pOwner );
	// Handled by the generator
	//pOwner->m_Shared.RecalculateChargeEffects();

	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEDEPLOYED );

	pTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED ); //MP_CONCEPT_PLAYER_BATTLECRY //MP_CONCEPT_PLAYER_TAUNT MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED;

	IGameEvent *event = gameeventmanager->CreateEvent( "player_chargedeployed" );
	if ( event )
	{
		event->SetInt( "userid", pOwner->GetUserID() );

		gameeventmanager->FireEvent( event );
	}
#endif
}

bool CTFPaintballRifle::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFPaintballRifle::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsJumping() )
	{
		if ( pPlayer->GetFOV() >= 75 )
			return;
	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = Max( m_flNextPrimaryAttack.Get(), gpGlobals->curtime + 0.1f );
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

void CTFPaintballRifle::ZoomOutIn( void )
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->ShouldAutoRezoom() )
	{
		m_flRezoomTime = gpGlobals->curtime + 0.1f;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
	}
}

void CTFPaintballRifle::ZoomIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( !HasPrimaryAmmoToFire() )
		return;

	// Interrupt a reload.
	if ( IsReloading() && Clip1() > 0 )
	{
		AbortReload();
	}

	BaseClass::ZoomIn();

	pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();
}

bool CTFPaintballRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}

void CTFPaintballRifle::ZoomOut( void )
{
	BaseClass::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

	// if we are thinking about zooming, cancel it
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}

void CTFPaintballRifle::Fire( CTFPlayer *pPlayer )
{
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Check the ammo.
	if ( !HasPrimaryAmmoToFire() )
	{
		HandleFireOnEmpty();
		return;
	}

	float iScopedFireDelay = GetTFWpnData().GetWeaponData( TF_WEAPON_SECONDARY_MODE ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( iScopedFireDelay, mult_postfiredelay );
	
	// Shoot
	PrimaryAttack();

	if ( iScopedFireDelay != 0.0f && IsZoomed() )
	{
		CALL_ATTRIB_HOOK_FLOAT( iScopedFireDelay, mult_postfiredelay_scoped );
		m_flNextPrimaryAttack = gpGlobals->curtime + iScopedFireDelay;
	}

	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( !iNoScope && (IsZoomed() && !pPlayer->ShouldHoldToZoom()) )
	{
		// If we have more bullets, zoom out, play the bolt animation and zoom back in
		if ( HasPrimaryAmmoToFire() )
		{
			SetRezoom( true, 0.5f );	// zoom out in 0.5 seconds, then rezoom
		}
		else
		{
			//just zoom out
			SetRezoom( false, 0.5f );	// just zoom out in 0.5 seconds
		}
	}
	else
	{
		// Prevent primary fire preventing zooms
		// m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.05f;
	}
}

void CTFPaintballRifle::SetRezoom( bool bRezoom, float flDelay )
{
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

void CTFPaintballRifle::AddCharge( float flAmount )
{
	float flChargeRate = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_medigun_uberchargerate );
	CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_medigun_uberchargerate_wearer );
	if ( !flChargeRate ) // Can't earn uber.
		return;

	float flNewLevel = Min( m_flChargeLevel + flAmount, 1.0f );
	flNewLevel = Max( flNewLevel, 0.0f );

#ifdef GAME_DLL
	bool bSpeak = (flNewLevel >= GetMinChargeAmount() && m_flChargeLevel < GetMinChargeAmount());
#endif
	m_flChargeLevel = flNewLevel;
#ifdef GAME_DLL
	if ( bSpeak )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );
		}
	}
#endif
}

void CTFPaintballRifle::DrainCharge( void )
{
	if ( !m_bChargeRelease )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	// If we're in charge release mode, drain our charge.
	float flUberTime = weapon_medigun_chargerelease_rate.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flUberTime, add_uber_time );
	CALL_ATTRIB_HOOK_FLOAT( flUberTime, add_uber_time_active );

	float flChargeAmount = gpGlobals->frametime / flUberTime;

	m_flChargeLevel = Max( m_flChargeLevel - flChargeAmount, 0.0f );
	if ( !m_flChargeLevel )
	{
		m_bChargeRelease = false;
#ifdef GAME_DLL
		if ( m_pUberTarget )
		{
			m_pUberTarget->m_Shared.StopHealing( pOwner, HEALER_TYPE_BEAM );
			m_pUberTarget->m_Shared.RecalculateChargeEffects();
		}

		pOwner->m_Shared.RecalculateChargeEffects();
#endif
	}
}

void CTFPaintballRifle::BuildUberForTarget( CBaseEntity *pTarget, bool bMultiTarget /*= false*/ )
{
	if ( m_bBuiltChargeThisFrame )
		return;

	CTFPlayer *pPatient = ToTFPlayer( pTarget );
	// Charge up our power if we're not releasing it, and our target
	// isn't receiving any benefit from our healing. (what da heck does this part mean? - hogyn)
	if ( !m_bChargeRelease )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();

		if ( weapon_medigun_charge_rate.GetFloat() )
		{
			int iBoostMax = floor( pPatient->m_Shared.GetMaxBuffedHealth() * 0.95f );
			//float flChargeAmount = flAmount / (tf2c_medicgl_health_to_uber_exchange_rate.GetFloat() * 100.0f);//weapon_medigun_charge_rate.GetFloat();
			float flChargeAmount;

			flChargeAmount = gpGlobals->frametime / weapon_medigun_charge_rate.GetFloat();

			bool bInSetup = (TFGameRules() && TFGameRules()->InSetup() &&
#ifdef GAME_DLL
				TFGameRules()->GetActiveRoundTimer() &&
#endif
				tf2c_medigun_setup_uber.GetBool());

			// We can optionally skip this part since we already have a reduced overheal rate that's reflected in flAmount already.
			if ( pPatient->GetHealth() >= pPatient->GetMaxHealth() && !bInSetup )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_overheal_uberchargerate );
			}

			// On the gun we're using
			CALL_ATTRIB_HOOK_FLOAT( flChargeAmount, mult_medigun_uberchargerate );

			// On the Healer themselves
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_uberchargerate_wearer );

			if ( bInSetup )
			{
				// Build charge at an increased rate during setup.
				flChargeAmount *= 3.0f;
			}
			else if ( pPatient->GetHealth() >= iBoostMax )
			{
				// Reduced charge for healing fully healed guys.
				flChargeAmount *= 0.5f;
			}

			// Speed up charge rate when under minicrit or crit buffs.
			if ( tf2c_medigun_critboostable.GetBool() )
			{
				if ( pOwner->m_Shared.IsCritBoosted() )
				{
					flChargeAmount *= 3.0f;
				}
				else if ( pOwner->m_Shared.InCond( TF_COND_DAMAGE_BOOST ) || IsWeaponDamageBoosted() )
				{
					flChargeAmount *= 1.35f;
				}
			}

			//if (pOwner->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
			//{
			//	flChargeAmount *= TF2C_HASTE_UBER_FACTOR;
			//}

			// In 4team, speed up charge rate to make up for smaller teams and decreased survivability.
			if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
			{
				flChargeAmount *= tf2c_medigun_4team_uber_rate.GetFloat();
			}

			// Reduce charge rate when healing someone already being healed.
			int iTotalHealers = pPatient->m_Shared.GetNumHumanHealers();
			if ( !bInSetup && iTotalHealers > 1 )
			{
				flChargeAmount /= (float)iTotalHealers;
			}

			// Build rate bonus stacks
#ifdef GAME_DLL
			CheckAndExpireStacks();
#endif
			flChargeAmount *= 1.0f + GetUberRateBonus();

			float flNewLevel = Min( m_flChargeLevel + flChargeAmount, 1.0f );

			bool bSpeak = (flNewLevel >= GetMinChargeAmount() && m_flChargeLevel < GetMinChargeAmount());
			m_flChargeLevel = flNewLevel;

			if ( bSpeak )
			{
#ifdef GAME_DLL
				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );
#endif
			}

			m_bBuiltChargeThisFrame = true;
		}
	}
}

#ifdef GAME_DLL
void CTFPaintballRifle::HitAlly( CBaseEntity *pEntity )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	CTFPlayer *pPlayer = ToTFPlayer( pEntity );

	//if ( pOwner->GetMedigun() )
	//{
	//	pOwner->GetMedigun()->AddBackpackPatient( pPlayer );
	//}

	// Play the sound for the owner at the owner's ears, and for everyone else at the recipient
	CPASFilter filter( pPlayer->GetAbsOrigin() );
	if ( pOwner )
	{
		// Not directly on/in our ears but 10 units in the direction of our recipient.
		Vector covector = (pPlayer->GetAbsOrigin() - pOwner->EyePosition());
		float flMaxDist = covector.Length();
		Vector ownerEarPos = pOwner->EyePosition() + covector.Normalized() * min( 128.0f, flMaxDist );
		CSingleUserRecipientFilter singleFilter( pOwner );
		pOwner->EmitSound( singleFilter, -1, "HealGrenade.Success", &ownerEarPos );
	}

	CSingleUserRecipientFilter patientFilter( pPlayer );
	pPlayer->EmitSound( patientFilter, pPlayer->entindex(), "HealGrenade.Success" );

	float flDuration = 0;
	CALL_ATTRIB_HOOK_FLOAT( flDuration, marked_target_autoheal_duration );
	if ( flDuration > 0 )
		AddHealTarget( pPlayer, flDuration );

	// Do the actual insta-healing last so that the heal progress sound manager has the right old/new health values.
	int iHealAmount = 0;
	CALL_ATTRIB_HOOK_INT( iHealAmount, apply_heal_explosion );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), iHealAmount, mult_medigun_healrate );
	if ( iHealAmount )
	{
		float flBonusHealthDuration = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( flBonusHealthDuration, marked_target_bonusheal_duration );
		pPlayer->m_Shared.AddBurstHealer( pOwner, iHealAmount, flBonusHealthDuration > 0 ? TF_COND_TAKEBONUSHEALING : TF_COND_INVALID, flBonusHealthDuration );

		// For the sake of attribution of assists etc., at a "zero healer" for a short period.
		pPlayer->m_Shared.HealTimed( pOwner, 0, 3, true );
	}

	OnDirectHit( pPlayer );
}

void CTFPaintballRifle::RemoveAllHealTargets( void )
{
	int iCount = m_vAutoHealTargets.Count(); // Need to cache this first because it'll change throughout the for loop otherwise.
	for ( int i = 0; i < iCount; i++ )
	{
		RemoveHealTarget( ToTFPlayer( m_vAutoHealTargets[i] ) );
	}
}

void CTFPaintballRifle::AddHealTarget( CTFPlayer *pPlayer, float flDuration )
{
	// Reset the timer
	if ( m_vAutoHealTargets.HasElement( pPlayer ) )
	{
		m_vTargetRemoveTime[m_vAutoHealTargets.Find( pPlayer )] = gpGlobals->curtime + flDuration;
	}
	// Add to list
	else
	{
		m_vAutoHealTargets.AddToTail( pPlayer );
		m_vTargetRemoveTime.AddToTail( gpGlobals->curtime + flDuration );
		m_vTargetsActive.AddToTail( false );
	}

	m_hMainPatient = pPlayer;
	m_iMainPatientHealthLast = pPlayer->GetHealth();
}

void CTFPaintballRifle::RemoveHealTarget( CTFPlayer *pPlayer )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if(m_hMainPatient.Get() == pPlayer)
	{
		//m_hMainPatient = NULL;
		// Set Main patient to null next update.
		m_bMainPatientFlaggedForRemoval = true;
	}

	pPlayer->m_Shared.StopHealing( pOwner, HEALER_TYPE_BEAM );

	int iIndex = m_vAutoHealTargets.Find( pPlayer );
	m_vAutoHealTargets.Remove( iIndex );
	m_vTargetRemoveTime.Remove( iIndex );
	m_vTargetsActive.Remove( iIndex );
}

void CTFPaintballRifle::ApplyBackpackHealing( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if( m_bMainPatientFlaggedForRemoval )
	{
		m_hMainPatient = NULL;
		m_bMainPatientFlaggedForRemoval = false;
	}

	FOR_EACH_VEC(m_vAutoHealTargets, i)
	{
		CTFPlayer *pPlayer = ToTFPlayer( m_vAutoHealTargets[i] );
		if ( !pPlayer )
			continue;

		float flRange2 = pPlayer->GetAbsOrigin().DistToSqr(pOwner->GetAbsOrigin());
		float flMaxRange2 = PAINTBALL_BACKPACK_RANGE_SQ;
		CALL_ATTRIB_HOOK_FLOAT( flMaxRange2, mult_backpack_range );
		CALL_ATTRIB_HOOK_FLOAT( flMaxRange2, mult_weapon_range );

		if ( gpGlobals->curtime > m_vTargetRemoveTime[i] || flRange2 > flMaxRange2 )
		{
			RemoveHealTarget( pPlayer );
			continue;
		}
		
		// Do we need to initiate a heal? Do so here:
		if ( !m_vTargetsActive[i] )
		{
			pPlayer->m_Shared.Heal( pOwner, GetHealRate() );
			m_vTargetsActive[i] = true;
		}
	}
}

float CTFPaintballRifle::GetHealRate( void )
{
	float flBaseHealRate = 0;
	CALL_ATTRIB_HOOK_FLOAT( flBaseHealRate, marked_target_autoheal_rate );
	// TODO call extra heal rate attribs here
	return flBaseHealRate;
}

void CTFPaintballRifle::OnDirectHit( CTFPlayer *pPatient )
{
	// Build direct-hit Uber based on our firerate

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( !m_bChargeRelease && pOwner)
	{
		if ( weapon_medigun_charge_rate.GetFloat() && pPatient->GetHealth() < pPatient->GetMaxHealth() )
		{
			int iBoostMax = floor( pPatient->m_Shared.GetMaxBuffedHealth() * 0.95f );
			//float flChargeAmount = flAmount / (tf2c_medicgl_health_to_uber_exchange_rate.GetFloat() * 100.0f);//weapon_medigun_charge_rate.GetFloat();
			float flChargeAmount;

			// Medigun healing for T seconds, where T is the fire interval -> 1 direct hit's uber build amount
			flChargeAmount = GetFireRate() / (weapon_medigun_charge_rate.GetFloat());

			bool bInSetup = (TFGameRules() && TFGameRules()->InSetup() &&
#ifdef GAME_DLL
				TFGameRules()->GetActiveRoundTimer() &&
#endif
				tf2c_medigun_setup_uber.GetBool());

			// We can optionally skip this part since we already have a reduced overheal rate that's reflected in flAmount already.
			if ( pPatient->GetHealth() >= pPatient->GetMaxHealth() && !bInSetup )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_overheal_uberchargerate );
			}

			// On the gun we're using
			CALL_ATTRIB_HOOK_FLOAT( flChargeAmount, mult_medigun_uberchargerate );

			// On the Healer themselves
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_uberchargerate_wearer );

			if ( bInSetup )
			{
				// Build charge at an increased rate during setup.
				flChargeAmount *= 3.0f;
			}
			else if ( pPatient->GetHealth() >= iBoostMax )
			{
				// Reduced charge for healing fully healed guys.
				flChargeAmount *= 0.5f;
			}

			// Speed up charge rate when under minicrit or crit buffs.
			if ( tf2c_medigun_critboostable.GetBool() )
			{
				if ( pOwner->m_Shared.IsCritBoosted() )
				{
					flChargeAmount *= 3.0f;
				}
				else if ( pOwner->m_Shared.InCond( TF_COND_DAMAGE_BOOST ) || IsWeaponDamageBoosted() )
				{
					flChargeAmount *= 1.35f;
				}
			}

			//if (pOwner->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
			//{
			//	flChargeAmount *= TF2C_HASTE_UBER_FACTOR;
			//}

			// In 4team, speed up charge rate to make up for smaller teams and decreased survivability.
			if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
			{
				flChargeAmount *= tf2c_medigun_4team_uber_rate.GetFloat();
			}

			// Reduce charge rate when healing someone already being healed.
			int iTotalHealers = pPatient->m_Shared.GetNumHumanHealers();
			if ( !bInSetup && iTotalHealers > 1 )
			{
				flChargeAmount /= (float)iTotalHealers;
			}

			// Build rate bonus stacks
#ifdef GAME_DLL
			CheckAndExpireStacks();
#endif
			flChargeAmount *= 1.0f + GetUberRateBonus();

			float flNewLevel = Min( m_flChargeLevel + flChargeAmount, 1.0f );

			bool bSpeak = (flNewLevel >= GetMinChargeAmount() && m_flChargeLevel < GetMinChargeAmount());
			m_flChargeLevel = flNewLevel;

			if ( bSpeak )
			{
#ifdef GAME_DLL
				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );
#endif
			}
		}
	}

#ifdef GAME_DLL
	IGameEvent* event = gameeventmanager->CreateEvent( "patient_healed_notify" );
	if ( event )
	{
		event->SetInt( "userid", pPatient->entindex() );
		event->SetInt( "healerid", pOwner->entindex() );
		gameeventmanager->FireEvent( event );
	}
#endif

	m_hMainPatient = pPatient;
	m_iMainPatientHealthLast = pPatient->GetHealth();
	m_bMainPatientFlaggedForRemoval = true;
}

#else

void CTFPaintballRifle::UpdateEffects( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	// Find all the targets we've stopped healing.
	int i, j, c, x;
	bool bStillBPHealing[MAX_PLAYERS];
	for ( i = 0, c = m_hBackpackTargetEffects.Count(); i < c; i++ )
	{
		bStillBPHealing[i] = false;

		// Are we still healing this target?
		for ( j = 0, x = m_vAutoHealTargets.Count(); j < x; j++ )
		{
			if ( m_vAutoHealTargets[j] &&
				m_vAutoHealTargets[j] == m_hBackpackTargetEffects[i].pTarget &&
				ShouldShowEffectForPlayer( ToTFPlayer( m_vAutoHealTargets[j] ) ))
			{
				bStillBPHealing[i] = true;
				break;
			}
		}
	}

	// Now remove all the dead effects.
	for ( i = m_hBackpackTargetEffects.Count() - 1; i >= 0; i-- )
	{
		if ( !bStillBPHealing[i] )
		{
			pOwner->ParticleProp()->StopEmission( m_hBackpackTargetEffects[i].pEffect );
			pOwner->ParticleProp()->StopEmission( m_hTargetOverheadEffects[i].pEffect );
			m_hBackpackTargetEffects.Remove( i );
			m_hTargetOverheadEffects.Remove( i );
			CPASFilter filter( GetAbsOrigin() );
			EmitSound_t eSoundParams;
			eSoundParams.m_pSoundName = BP_SOUND_HEALOFF;
			eSoundParams.m_flVolume = 0.35f;
			eSoundParams.m_nFlags |= SND_CHANGE_VOL;
			EmitSound( filter, entindex(), eSoundParams );
		}
	}

	// Now add any new targets.
	for ( i = 0, c = m_vAutoHealTargets.Count(); i < c; i++ )
	{
		C_TFPlayer *pTarget = ToTFPlayer( m_vAutoHealTargets[i].Get() );
		if ( !pTarget || !ShouldShowEffectForPlayer( pTarget ) )
			continue;

		// Loops through the healing targets, and make sure we have an effect for each of them.
		bool bHaveEffect = false;
		for ( j = 0, x = m_hBackpackTargetEffects.Count(); j < x; j++ )
		{
			if ( m_hBackpackTargetEffects[j].pTarget == pTarget )
			{
				bHaveEffect = true;
				break;
			}
		}

		if ( bHaveEffect )
			continue;

		const char *pszEffectName = "backpack_beam";
		//const char *pszEffectName = ConstructTeamParticle( "overhealer_%s_beam", GetTeamNumber() );
		CNewParticleEffect *pEffect = pOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "flag" );
		pOwner->ParticleProp()->AddControlPoint( pEffect, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 50 ) );

		int iIndex = m_hBackpackTargetEffects.AddToTail();
		m_hBackpackTargetEffects[iIndex].pTarget = pTarget;
		m_hBackpackTargetEffects[iIndex].pEffect = pEffect;
		m_hBackpackTargetEffects[iIndex].hOwner = pOwner;

		const char *pszEffectNameOverhead = BP_PARTICLE_ACTIVE;
		CNewParticleEffect *pEffectOverhead = pOwner->ParticleProp()->Create( pszEffectNameOverhead, PATTACH_ABSORIGIN_FOLLOW );
		pOwner->ParticleProp()->AddControlPoint( pEffectOverhead, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 60 ) ); // Position it above his head.

		int iIndexAdd = m_hTargetOverheadEffects.AddToTail();
		m_hTargetOverheadEffects[iIndexAdd].pTarget = pTarget;
		m_hTargetOverheadEffects[iIndexAdd].pEffect = pEffectOverhead;
		m_hTargetOverheadEffects[iIndexAdd].hOwner = pOwner;

		CPASFilter filter( GetAbsOrigin() );
		EmitSound_t eSoundParams;
		eSoundParams.m_pSoundName = BP_SOUND_HEALON;
		eSoundParams.m_flVolume = 0.65f;
		eSoundParams.m_nFlags |= SND_CHANGE_VOL;
		EmitSound( filter, entindex(), eSoundParams );
	}
}

bool CTFPaintballRifle::ShouldShowEffectForPlayer( C_TFPlayer *pPlayer )
{
	// Don't give away cloaked spies.
	// FIXME: Is the latter part of this check necessary?
	if ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		return false;

	// Don't show the effect for disguised spies unless they're the same color.
	if ( GetLocalPlayerTeam() >= FIRST_GAME_TEAM && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && (!pPlayer->m_Shared.DisguiseFoolsTeam( GetTeamNumber() )) )
		return false;

	return true;
}

void CTFPaintballRifle::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bBackpackTargetsParity != m_bOldBackpackTargetsParity )
	{
		m_bUpdateBackpackTargets = true;
	}

	if ( m_bMainTargetParityOld != m_bMainTargetParity && m_pHealSoundManager )
	{
		if ( !m_pHealSoundManager->IsSetupCorrectly() )
		{
			m_pHealSoundManager->RemoveSound();
			delete m_pHealSoundManager;
			m_pHealSoundManager = NULL;

			m_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );
		}

		// Let the heal sound manager know of the recorded health from before the healing was done.
#if 0
		m_pHealSoundManager->SetPatient( ToTFPlayer( m_hMainPatient ) );
		if ( m_hMainPatient )
			m_pHealSoundManager->SetLastPatientHealth( m_iMainPatientHealthLast );
#endif
		
	}

	if ( m_bUpdateBackpackTargets )
	{
		UpdateEffects();

		m_bUpdateBackpackTargets = false;
	}
}

void CTFPaintballRifle::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldBackpackTargetsParity = m_bBackpackTargetsParity;
	m_bMainTargetParityOld = m_bMainTargetParity;
}

#ifdef TF2C_BETA // public doesnt need this. not leaking cvars and CTFPatientHealthbarPanel
void CTFPaintballRifle::UpdateRecentPatientHealthbar( C_TFPlayer *pPatient )
{
	pPatient->CreateOverheadHealthbar();
}

void CTFPaintballRifle::FireGameEvent( IGameEvent* pEvent )
{
	//if ( tf2c_medicgl_show_patient_health.GetBool() )
	{
		if ( !strcmp( "patient_healed_notify", pEvent->GetName() ) )
		{
			int iUserID = pEvent->GetInt( "userid" );
			int iHealerID = pEvent->GetInt( "healerid" );
			CTFPlayer *pPatient = dynamic_cast<CTFPlayer *>(ClientEntityList().GetEnt( iUserID ));
			CTFPlayer *pHealer = dynamic_cast<CTFPlayer *>(ClientEntityList().GetEnt( iHealerID ));
			if ( !pHealer->IsLocalPlayer() )
				return;

			if ( pPatient )
				UpdateRecentPatientHealthbar( pPatient );
			else
				DevWarning( "patient_healed_notify called on client, but could not cast to player! userid : %i\n", iUserID );
			//DevMsg( "patient_healed_notify called on client! Woohoo! Name: %s, userid: %i, player\n" );
		}
	}
}
#endif // TF2C_BETA
#endif