﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_sniperrifle.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"
#include "c_tf_player.h"

// For TFGameRules() and Player resources
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

ConVar tf_sniper_fullcharge_bell( "tf_sniper_fullcharge_bell", "0", FCVAR_ARCHIVE );
ConVar tf_hud_no_crosshair_on_scope_zoom( "tf_hud_no_crosshair_on_scope_zoom", "0", FCVAR_ARCHIVE );

#else
#include "tf_player.h"
#endif

#define SNIPER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"
#define SNIPER_DOT_SPRITE_GREEN		"effects/sniperdot_green.vmt"
#define SNIPER_DOT_SPRITE_YELLOW	"effects/sniperdot_yellow.vmt"

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle, DT_TFSniperRifle )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle, DT_SniperRifleLocalData )
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flUnzoomTime ) ),
	SendPropTime( SENDINFO( m_flRezoomTime ) ),
	SendPropBool( SENDINFO( m_bRezoomAfterShot ) ),
#else
	RecvPropTime( RECVINFO( m_flUnzoomTime ) ),
	RecvPropTime( RECVINFO( m_flRezoomTime ) ),
	RecvPropBool( RECVINFO( m_bRezoomAfterShot ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle, DT_TFSniperRifle )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleLocalData ), SendProxy_SendLocalWeaponDataTable ),
	SendPropFloat( SENDINFO( m_flChargedDamage ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iShotsFired ), 4, SPROP_UNSIGNED ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleLocalData ) ),
	RecvPropFloat( RECVINFO( m_flChargedDamage ) ),
	RecvPropInt( RECVINFO( m_iShotsFired ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flUnzoomTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flRezoomTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDinged, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle );

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::CTFSniperRifle()
{
	ResetTimers();

	// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::~CTFSniperRifle()
{
	// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}


void CTFSniperRifle::Precache()
{
	BaseClass::Precache();

	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );
	PrecacheModel( SNIPER_DOT_SPRITE_GREEN );
	PrecacheModel( SNIPER_DOT_SPRITE_YELLOW );

	PrecacheTeamParticles( "dxhr_sniper_rail_%s" );
}


void CTFSniperRifle::ResetTimers( void )
{
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}


bool CTFSniperRifle::CanHolster( void ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() ) && ( m_flNextPrimaryAttack > gpGlobals->curtime ) )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}


bool CTFSniperRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#else
	m_bDinged = false;
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}

	m_flChargedDamage = 0.0f;
	ResetTimers();

	return BaseClass::Holster( pSwitchingTo );
}


void CTFSniperRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	ZoomOut();
}


float CTFSniperRifle::OwnerMaxSpeedModifier( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimingMoveSpeed = 80.0f;
		return flAimingMoveSpeed;
	}

	return 0.0f;
}


bool CTFSniperRifle::OwnerCanJump( void )
{
	return gpGlobals->curtime > m_flUnzoomTime;
}


void CTFSniperRifle::HandleZooms( void )
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
	int bZoomJump = 0;
	CALL_ATTRIB_HOOK_INT(bZoomJump, mod_sniper_zoom_while_jumping);

	if ((pPlayer->m_Shared.IsJumping() && !bZoomJump) || pPlayer->m_Shared.InCond(TF_COND_TAUNTING))
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
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
			m_bRezoomAfterShot = true;

			// Only allow toggling zoom when altfire is ready.
			if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
			{
				if ( !IsZoomed() && gpGlobals->curtime > m_flNextPrimaryAttack )
				{
					Zoom();
					m_flNextSecondaryAttack = Max( m_flNextSecondaryAttack.Get(), gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME );
				}
			}
		}
		else
		{
			m_bRezoomAfterShot = false;

			// Only allow toggling zoom when altfire is ready.
			if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
			{
				if ( IsZoomed() && gpGlobals->curtime > m_flUnzoomTime )
				{
					Zoom();
					m_flNextSecondaryAttack = Max( m_flNextSecondaryAttack.Get(), gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME );
				}
			}
		}
	}
	else
	{
		if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
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


void CTFSniperRifle::ItemPostFrame( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	CheckReload();

	// Interrupt a reload.
	if (IsReloading() && Clip1() > 0 && (pPlayer->m_nButtons & IN_ATTACK))
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

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot )
	{
		UpdateSniperDot();
	}
#endif

	// Start charging when we're zoomed in, and allowed to fire
	// Don't start charging in the time just after a shot before we unzoom to play rack anim.
	if ( IsZoomed() && gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		m_flChargedDamage = Min<float>( m_flChargedDamage + gpGlobals->frametime * GetChargeSpeed(), TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );

#ifdef CLIENT_DLL
		if ( m_flChargedDamage >= TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX && !m_bDinged )
		{
			m_bDinged = true;
			if ( tf_sniper_fullcharge_bell.GetBool() )
			{
				CSingleUserRecipientFilter filter( pPlayer );
				filter.UsePredictionRules();
				C_BaseEntity::EmitSound( filter, pPlayer->entindex(), "TFPlayer.Recharged" );
			}
		}
#endif
	}
	else
	{
		m_flChargedDamage = 0.0f;
	}

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK ) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && !IsReloading() )
		{
			WeaponIdle();
		}
	}

	// Reload.
	if ( ( pPlayer->m_nButtons & IN_RELOAD ) && UsesClipsForAmmo1() ) 
	{
		Reload();
	}

	if ( IsReloading() && IsZoomed() )
	{
		ToggleZoom();
	}
}


bool CTFSniperRifle::Lower( void )
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
void CTFSniperRifle::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	int bZoomJump = 0;
	CALL_ATTRIB_HOOK_INT(bZoomJump, mod_sniper_zoom_while_jumping);
	if ( pPlayer && pPlayer->m_Shared.IsJumping() && !bZoomJump )
	{
		if ( !IsZoomed() )
			return;
	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = Max( m_flNextPrimaryAttack.Get(), gpGlobals->curtime + 0.1f );
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}


void CTFSniperRifle::ZoomOutIn( void )
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->ShouldAutoRezoom() )
	{
		m_flRezoomTime = gpGlobals->curtime + 0.9f;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	}
}


void CTFSniperRifle::ZoomIn( void )
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

#ifdef GAME_DLL
	// Create the sniper dot.
	CreateSniperDot();
	pPlayer->ClearExpression();
#endif
}

bool CTFSniperRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}


void CTFSniperRifle::ZoomOut( void )
{
	BaseClass::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	pPlayer->ClearExpression();
#else
	m_bDinged = false;
#endif

	// if we are thinking about zooming, cancel it
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
	m_flChargedDamage = 0.0f;
}


void CTFSniperRifle::Fire( CTFPlayer *pPlayer )
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

	// Fire the sniper shot.
	PrimaryAttack();

	if ( iScopedFireDelay != 0.0f && IsZoomed() )
	{
		CALL_ATTRIB_HOOK_FLOAT(iScopedFireDelay, mult_postfiredelay_scoped);
		m_flNextPrimaryAttack = gpGlobals->curtime + iScopedFireDelay;
	}

	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( !iNoScope && IsZoomed() )
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
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	}

	m_flChargedDamage = 0.0f;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#else
	m_bDinged = false;
#endif
}


void CTFSniperRifle::SetRezoom( bool bRezoom, float flDelay )
{
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProjectileDamage( void )
{
	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( iNoScope )
		return CTFWeaponBaseGun::GetProjectileDamage();

	// Uncharged? Min damage.
	float flDamage = Max<float>( m_flChargedDamage, TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN );

	// Damage Modifier
	float flDamageMod = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDamageMod, mult_dmg );
	flDamage *= flDamageMod;

	if ( m_flChargedDamage >= TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX )
	{
		CALL_ATTRIB_HOOK_FLOAT( flDamage, sniper_full_charge_damage_bonus );
	}

	return flDamage;
}


float CTFSniperRifle::GetWeaponSpread( void )
{
	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( iNoScope )
	{
		// Perfect accuracy while zoomed in.
		if ( IsZoomed() )
		{
			float flMod = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT( flMod, mod_spread_scale_scoped_override );
			return flMod;
		}
	}

	return BaseClass::GetWeaponSpread();
}



float CTFSniperRifle::GetChargeSpeed( void )
{
	int iNoCharge = 0;
	CALL_ATTRIB_HOOK_INT( iNoCharge, mod_sniper_no_dmg_charge );
	if ( iNoCharge )
		return 0.0f;

	float flChargeSpeed = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;

	CALL_ATTRIB_HOOK_FLOAT( flChargeSpeed, mult_sniper_charge_per_sec );

	// Faster charge rate during Haste buff
	CTFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flChargeSpeed *= 3.0f;
	}

	return flChargeSpeed;
}


void CTFSniperRifle::CreateSniperDot( void )
{
	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( iNoScope )
		return;

	// Server specific.
#ifdef GAME_DLL

	// Check to see if we have already been created?
	if ( m_hSniperDot )
		return;

	// Get the owning player (make sure we have one).
	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	// Create the sniper dot, but do not make it visible yet.
	m_hSniperDot = CSniperDot::Create( GetAbsOrigin(), pPlayer, true );
	m_hSniperDot->ChangeTeam( pPlayer->GetTeamNumber() );
	m_hSniperDot->SetChargeSpeed( GetChargeSpeed() );

#endif
}


void CTFSniperRifle::DestroySniperDot( void )
{
	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( iNoScope )
		return;

	// Server specific.
#ifdef GAME_DLL

	// Destroy the sniper dot.
	if ( m_hSniperDot )
	{
		UTIL_Remove( m_hSniperDot );
		m_hSniperDot = NULL;
	}

#endif
}


void CTFSniperRifle::UpdateSniperDot( void )
{
	int iNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );

	if ( iNoScope )
		return;

	// Server specific.
#ifdef GAME_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Get the start and endpoints.
	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, ( MASK_TFSHOT & ~CONTENTS_WINDOW ), GetOwner(), COLLISION_GROUP_NONE, &trace );

	// Update the sniper dot.
	if ( m_hSniperDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
		}

		m_hSniperDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}

#endif
}


bool CTFSniperRifle::CanFireRandomCrit( void )
{
	// No random critical hits.
	return false;
}


bool CTFSniperRifle::CanHeadshot( void )
{
	int iNoHeadshot = 0;
	CALL_ATTRIB_HOOK_INT(iNoHeadshot, no_headshot);
	if (iNoHeadshot)
		return false;

	int iCanHeadshot = 0;
	CALL_ATTRIB_HOOK_INT(iCanHeadshot, can_headshot);
	if (iCanHeadshot)
		return true;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// no crits if they're not zoomed
		if ( !pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			return false;
		}

		// no crits for 0.2 seconds after starting to zoom
		if ( ( gpGlobals->curtime - pPlayer->GetFOVTime() ) < TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME )
		{
			return false;
		}
	}

	return true;
}


void CTFSniperRifle::DoFireEffects()
{
	BaseClass::DoFireEffects();
	++m_iShotsFired;
	if ( m_iShotsFired >= GetMaxClip1() )
	{
		m_iShotsFired = 0;
	}
	UpdateCylinder();
}


void CTFSniperRifle::UpdateCylinder(void)
{
#ifdef CLIENT_DLL
	if (m_iCylinderPoseParameter != -1)
	{
		SetPoseParameter(m_iCylinderPoseParameter, (float)m_iShotsFired / (float)GetMaxClip1());
	}
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

CStudioHdr *CTFSniperRifle::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Check that the model is compatible.
	if ( m_pModelWeaponData && m_pModelWeaponData->GetInt( "revrifle_clip", -2 ) == GetMaxClip1() )
	{
		m_iCylinderPoseParameter = LookupPoseParameter( "cylinder_spin" );
	}
	else
	{
		m_iCylinderPoseParameter = -1;
	}

	return hdr;
}


float CTFSniperRifle::GetHUDDamagePerc( void )
{
	return ( m_flChargedDamage / TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );
}


bool CTFSniperRifle::ShouldDrawCrosshair( void )
{
	if ( IsZoomed() )
	{
		int iNoScope = 0;
		CALL_ATTRIB_HOOK_INT( iNoScope, mod_no_scope );
		if ( !iNoScope )
			return !tf_hud_no_crosshair_on_scope_zoom.GetBool();
	}

	return BaseClass::ShouldDrawCrosshair();
}

//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	CTFSniperRifle *pWeapon = assert_cast<CTFSniperRifle *>( pPlayer->GetActiveTFWeapon() );
	if ( pWeapon )
	{
		float flChargeValue = ( ( 1.0 - pWeapon->GetHUDDamagePerc() ) * 0.8 ) + 0.6;

		VMatrix mat, temp;

		Vector2D center( 0.5, 0.5 );
		MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );

		// scale
		{
			Vector2D scale( 1.0f, 0.25f );
			MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
			MatrixMultiply( temp, mat, mat );
		}

		MatrixBuildTranslation( temp, center.x, center.y, 0.0f );
		MatrixMultiply( temp, mat, mat );

		// translation
		{
			Vector2D translation( 0.0f, flChargeValue );
			MatrixBuildTranslation( temp, translation.x, translation.y, 0.0f );
			MatrixMultiply( temp, mat, mat );
		}

		m_pResult->SetMatrixValue( mat );
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CProxySniperRifleCharge, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( SniperDot, DT_SniperDot )

BEGIN_NETWORK_TABLE( CSniperDot, DT_SniperDot )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeStartTime ) ),
	RecvPropFloat( RECVINFO( m_flChargeSpeed ) ),
#else
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
	SendPropFloat( SENDINFO( m_flChargeSpeed ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_sniperdot, CSniperDot );

BEGIN_DATADESC( CSniperDot )
	DEFINE_FIELD( m_vecSurfaceNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_hTargetEnt, FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSniperDot::CSniperDot( void )
{
	m_vecSurfaceNormal.Init();
	m_hTargetEnt = NULL;

#ifdef CLIENT_DLL
	m_hSpriteMaterial = NULL;
#endif

	ResetChargeTime();
	m_flChargeSpeed = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CSniperDot::~CSniperDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CSniperDot
//-----------------------------------------------------------------------------
CSniperDot *CSniperDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
	// Client specific.
#ifdef CLIENT_DLL

	return NULL;

	// Server specific.
#else

	// Create the sniper dot entity.
	CSniperDot *pDot = static_cast<CSniperDot*>( CBaseEntity::Create( "env_sniperdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	//Create the graphic
	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	// Set owner.
	pDot->SetOwnerEntity( pOwner );

	// Force updates even though we don't have a model.
	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	return pDot;

#endif
}


void CSniperDot::Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal )
{
	SetAbsOrigin( vecOrigin );
	m_vecSurfaceNormal = vecNormal;
	m_hTargetEnt = pTarget;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL


int CSniperDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );

	// Get the sprite rendering position.
	Vector vecEndPos;

	float flSize = 6.0;

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		Vector vecAttachment, vecDir;
		QAngle angles;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			// Clamp the forward distance for the sniper's firstperson
			flDist = 384;

			flSize = 2.0;
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors( angles, &vecDir );
		}

		trace_t tr;
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

		// Backup off the hit plane, towards the source
		vecEndPos = tr.endpos + vecDir * -4;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	float flStrength = RemapValClamped( flLifeTime, 0.0, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX / m_flChargeSpeed, 0.1, 1.0 );

	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}


bool CSniperDot::ShouldDraw( void )			
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

#if 0
	// Don't draw the sniper dot when in thirdperson.
	if ( ::input->CAM_IsThirdPerson() )
		return false;
#endif

	return true;
}


void CSniperDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// If we are in FFA mode, we precache a clear, white
		// version of the sniper dot which we can color in later.
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_RED, TEXTURE_GROUP_CLIENT_EFFECTS );
			break;
		case TF_TEAM_BLUE:
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_BLUE, TEXTURE_GROUP_CLIENT_EFFECTS );
			break;
		case TF_TEAM_GREEN:
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_GREEN, TEXTURE_GROUP_CLIENT_EFFECTS );
			break;
		case TF_TEAM_YELLOW:
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_YELLOW, TEXTURE_GROUP_CLIENT_EFFECTS );
			break;
		}
	}
}

#endif
