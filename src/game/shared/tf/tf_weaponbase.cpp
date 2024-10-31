//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//=============================================================================
#include "cbase.h"
#include "animation.h"
#include "tf_weaponbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "eventlist.h"
#include "tf_viewmodel.h"
#include "baseobject_shared.h"
#include "tf_weapon_wrench.h"
#include "tf_weapon_brimstonelauncher.h" // !!! azzy brimstone

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_heallauncher.h"
#include "tf_weapon_medigun.h"
#include "tf_gamestats.h"
#include "achievements_tf.h"
#else
// Client specific.
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "hud_crosshair.h"
#include "c_tf_playerresource.h"
#include "clientmode_tf.h"
#include "r_efx.h"
#include "dlight.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "toolframework_client.h"
#include "c_droppedmagazine.h"
#include "bone_setup.h"

// For the Spy's material proxies.
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#endif

extern ConVar r_drawviewmodel;

#ifdef CLIENT_DLL
extern ConVar tf2c_muzzleflash;
extern ConVar tf2c_model_muzzleflash;
extern ConVar tf2c_legacy_invulnerability_material;
#endif

ConVar tf_weapon_criticals( "tf_weapon_criticals", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Whether or not random crits are enabled." );
ConVar tf2c_weapon_noreload( "tf2c_weapon_noreload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disables reloading for all weapons." );
ConVar tf2c_vip_criticals( "tf2c_vip_criticals", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Whether or not random crits are enabled in VIP mode." );
ConVar tf2c_storedcrits_rapidfire_duration("tf2c_storedcrits_rapidfire_duration", "1.5", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of critical hits when stored crit is used on rapid fire weapon.");

//#define TF2C_TRANQ_DEPLOY_FACTOR	1.33f // Moved to header so animation can find it
#define TF2C_TRANQ_RELOAD_FACTOR	1.33f
//#define TF2C_HASTE_RELOAD_FACTOR	0.50f // Moved to header so animation can find it
#define TF2C_HASTE_FIRERATE_FACTOR	0.75f

//=============================================================================
//
// Global functions.
//


void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, ITraceFilter *pFilter )
{
	int	i, j, k;
	trace_t tmpTrace;
	Vector vecEnd;
	float distance = 1e6f;
	Vector minmaxs[2] = { mins, maxs };
	Vector vecHullEnd = tr.endpos;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pFilter, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pFilter, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.endpos - vecSrc ).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

#ifdef CLIENT_DLL
void RecvProxy_Sequence( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_WeaponSequence( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFWeaponBase *pWeapon = (C_TFWeaponBase *)pStruct;

	// Weapons carried by other players have different models on server and client
	// so we should ignore sequence changes in such case.
	if ( !pWeapon->GetOwner() || pWeapon->UsingViewModel() )
	{
		RecvProxy_Sequence( pData, pStruct, pOut );
	}
}
#endif

//=============================================================================
//
// TFWeaponBase tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBase, DT_TFWeaponBase )

BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBase, DT_LocalTFWeaponData )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iReloadMode ) ),
	RecvPropTime( RECVINFO( m_flClipRefillTime ) ),
	RecvPropBool( RECVINFO( m_bReloadedThroughAnimEvent ) ),
	RecvPropTime( RECVINFO( m_flLastCritCheckTime ) ),
	RecvPropTime( RECVINFO( m_flCritTime ) ),
	RecvPropBool( RECVINFO( m_bCritTimeIsStoredCrit ) ),
	RecvPropTime( RECVINFO( m_flLastPrimaryAttackTime ) ),
	RecvPropTime( RECVINFO( m_flDamageBoostTime ) ),
	RecvPropTime( RECVINFO( m_flEffectBarRegenTime ) ),
	RecvPropBool( RECVINFO( m_bUsesAmmoMeter ) ),
#else
	SendPropInt( SENDINFO( m_iReloadMode ), 2, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flClipRefillTime ) ),
	SendPropBool( SENDINFO( m_bReloadedThroughAnimEvent ) ),
	SendPropTime( SENDINFO( m_flLastCritCheckTime ) ),
	SendPropTime( SENDINFO( m_flCritTime ) ),
	SendPropBool( SENDINFO( m_bCritTimeIsStoredCrit ) ),
	SendPropTime( SENDINFO( m_flLastPrimaryAttackTime ) ),
	SendPropTime( SENDINFO( m_flDamageBoostTime ) ),
	SendPropTime( SENDINFO(m_flEffectBarRegenTime) ),
	SendPropBool( SENDINFO( m_bUsesAmmoMeter ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFWeaponBase, DT_TFWeaponBase )
#ifdef CLIENT_DLL
	// Client specific.
	RecvPropInt( RECVINFO( m_iWeaponMode ) ),
	RecvPropBool( RECVINFO( m_bResetParity ) ),
#ifdef ITEM_TAUNTING
	RecvPropInt( RECVINFO( m_nTauntModelIndex ) ),
#endif

	RecvPropDataTable( "LocalActiveTFWeaponData", 0, 0, &REFERENCE_RECV_TABLE( DT_LocalTFWeaponData ) ),

	RecvPropInt( RECVINFO( m_nSequence ), 0, RecvProxy_WeaponSequence ),

	RecvPropInt( RECVINFO( m_iViewModelType ) ),

	RecvPropBool( RECVINFO( m_bDisguiseWeapon ) ),
	RecvPropEHandle( RECVINFO( m_hExtraWearable ) ),
	RecvPropEHandle( RECVINFO( m_hExtraWearableViewModel ) ),

	RecvPropInt( RECVINFO( m_iKillCount ) ),
#else
	// Server specific.
	SendPropInt( SENDINFO( m_iWeaponMode ), 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bResetParity ) ),
#ifdef ITEM_TAUNTING
	SendPropModelIndex( SENDINFO( m_nTauntModelIndex ) ),
#endif

	SendPropDataTable( "LocalActiveTFWeaponData", 0, &REFERENCE_SEND_TABLE( DT_LocalTFWeaponData ), SendProxy_SendLocalWeaponDataTable ),

	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropInt( SENDINFO( m_nSequence ), ANIMATION_SEQUENCE_BITS, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_iViewModelType ) ),

	SendPropBool( SENDINFO( m_bDisguiseWeapon ) ),
	SendPropEHandle( SENDINFO( m_hExtraWearable ) ),
	SendPropEHandle( SENDINFO( m_hExtraWearableViewModel ) ),

	SendPropInt( SENDINFO( m_iKillCount ), 8, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBase )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iWeaponMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCurrentAttackIsCrit, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInAttack, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInAttack2, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iReloadMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flClipRefillTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReloadedThroughAnimEvent, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastCritCheckTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCritTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritTimeIsStoredCrit, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iReloadStartClipAmount, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_flLastPrimaryAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDamageBoostTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flEffectBarRegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
#endif
END_PREDICTION_DATA()

BEGIN_ENT_SCRIPTDESC( CTFWeaponBase, CBaseCombatWeapon, "Team Fortress 2 weapon" )
END_SCRIPTDESC()

//=============================================================================
//
// TFWeaponBase shared functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBase::CTFWeaponBase()
{
	SetPredictionEligible( true );

	// Nothing collides with these, but they get touch calls.
	AddSolidFlags( FSOLID_TRIGGER );

	// Weapons can fire underwater.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	// Initialize the weapon modes.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iReloadMode.Set( TF_RELOAD_START );

	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flCritTime = 0;
	m_bCritTimeIsStoredCrit = false;
	m_flLastCritCheckTime = 0;
	m_flDamageBoostTime = 0;
	m_iLastCritCheckFrame = 0;
	m_bCurrentAttackIsCrit = false;
	m_flLastPrimaryAttackTime = 0.0f;
	m_bDisguiseWeapon = false;
	m_bUsesAmmoMeter = false;
	m_iKillCount = 0;

#ifdef CLIENT_DLL
	m_pModelKeyValues = NULL;
	m_pModelWeaponData = NULL;

	m_bInitViewmodelOffset = false;
	m_vecViewmodelOffset = vec3_origin;
#endif

	m_iViewModelType = VMTYPE_NONE;
}

CTFWeaponBase::~CTFWeaponBase()
{
#ifdef CLIENT_DLL
	if ( m_pModelKeyValues )
	{
		m_pModelKeyValues->deleteThis();
	}
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Spawn()
{
	InitializeAttributes();

	// Base class spawn.
	BaseClass::Spawn();

	if ( GetPlayerOwner() )
	{
		ChangeTeam( GetPlayerOwner()->GetTeamNumber() );
	}

	int iAmmoMeter = 0;
	CALL_ATTRIB_HOOK_INT( iAmmoMeter, use_ammo_meter );
	m_bUsesAmmoMeter = iAmmoMeter != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Precache()
{
	BaseClass::Precache();

#ifdef GAME_DLL
	if ( GetMuzzleFlashModel() )
	{
		PrecacheModel( GetMuzzleFlashModel() );
	}

	// Explosion sound.
	const CTFWeaponInfo *pTFInfo = &GetTFWpnData();
	if ( pTFInfo->m_szExplosionSound[0] )
	{
		PrecacheScriptSound( pTFInfo->m_szExplosionSound );
	}

	// Eject brass shells model.
	if ( pTFInfo->m_szBrassModel[0] )
	{
		PrecacheModel( pTFInfo->m_szBrassModel );
	}

	// Ejected magazine model.
	if ( pTFInfo->m_szMagazineModel[0] )
	{
		PrecacheModel( pTFInfo->m_szMagazineModel );
	}

	// Muzzle particle.
	if ( pTFInfo->m_szMuzzleFlashParticleEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffect );
	}

	if ( !pTFInfo->m_bHasTeamColoredExplosions )
	{
		// Not using per-team explosion effects.
		for ( int i = 0; i < TF_EXPLOSION_COUNT; i++ )
		{
			if ( pTFInfo->m_szExplosionEffects[i][0] )
			{
				PrecacheParticleSystem( pTFInfo->m_szExplosionEffects[i] );
			}
		}
	}

	ForEachTeamName( [=]( const char *pszTeamName )
	{
		// Tracers.
		if ( pTFInfo->m_szTracerEffect[0] )
		{
			PrecacheParticleSystem( UTIL_VarArgs( "%s_%s", pTFInfo->m_szTracerEffect, pszTeamName ) );
			PrecacheParticleSystem( UTIL_VarArgs( "%s_%s_crit", pTFInfo->m_szTracerEffect, pszTeamName ) );
		}

		for ( int i = 0; i < TF_EXPLOSION_COUNT; i++ )
		{
			if ( pTFInfo->m_szExplosionEffects[i][0] )
			{
				if ( pTFInfo->m_bHasTeamColoredExplosions )
				{
					PrecacheParticleSystem( UTIL_VarArgs( "%s_%s", pTFInfo->m_szExplosionEffects[i], pszTeamName ) );
				}

				if ( pTFInfo->m_bHasCritExplosions )
				{
					PrecacheParticleSystem( UTIL_VarArgs( "%s_%s_crit", pTFInfo->m_szExplosionEffects[i], pszTeamName ) );
				}
			}
		}
	} );

	string_t strImpactParticle = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING( strImpactParticle, particle_on_melee_hit );
	if ( strImpactParticle != NULL_STRING )
	{
		PrecacheParticleSystem( STRING( strImpactParticle ) );
	}

	CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( GetItemID() );
	if ( pItemDef )
	{
		if( pItemDef->GetVisuals( GetTeamNumber() )->tracer_effect[0] )
		{
			ForEachTeamName( [=]( const char *pszTeamName )
			{
				PrecacheParticleSystem( UTIL_VarArgs( "%s_%s", pItemDef->GetVisuals( GetTeamNumber() )->tracer_effect, pszTeamName) );
				PrecacheParticleSystem( UTIL_VarArgs( "%s_%s_crit", pItemDef->GetVisuals( GetTeamNumber() )->tracer_effect, pszTeamName) );
			} );
		}

		if( pItemDef->GetVisuals( GetTeamNumber() )->explosion_effect[0] )
		{
			PrecacheParticleSystem( pItemDef->GetVisuals( GetTeamNumber() )->explosion_effect );
		}

		if ( pItemDef->GetVisuals( GetTeamNumber() )->explosion_effect_crit[0] )
		{
			PrecacheParticleSystem( pItemDef->GetVisuals( GetTeamNumber() )->explosion_effect_crit );
		}
	}

	PrecacheScriptSound("InstantReloadPing");
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const CTFWeaponInfo &CTFWeaponBase::GetTFWpnData() const
{
	return static_cast<const CTFWeaponInfo &>( GetWpnData() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
ETFWeaponID CTFWeaponBase::GetWeaponID( void ) const
{
	Assert( false );
	return TF_WEAPON_NONE;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::IsWeapon( ETFWeaponID iWeapon ) const
{
	return GetWeaponID() == iWeapon;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetFireRate( void )
{
	float flFireDelay = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	if (IsBursting())
	{
		CALL_ATTRIB_HOOK_FLOAT(flFireDelay, mult_burstfiredelay);
	}
	else
	{
		CALL_ATTRIB_HOOK_FLOAT(flFireDelay, mult_postfiredelay);
	}

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return flFireDelay;

	if ( pPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flFireDelay *= TF2C_HASTE_FIRERATE_FACTOR;
	}

	float flFireDelayLowHealthMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelayLowHealthMult, mult_postfiredelay_with_reduced_health );
	if ( flFireDelayLowHealthMult != 1.0f && flFireDelayLowHealthMult != 0.0f )
	{
		flFireDelayLowHealthMult = RemapValClamped( pPlayer->GetHealth() / (float)pPlayer->GetMaxHealth(), 0.1f, 1.0f, flFireDelayLowHealthMult, 1.0f );
		flFireDelay *= flFireDelayLowHealthMult;
		//DevMsg( "firedelay: %2.2f, hp: %i\n", flFireDelay, pPlayer->GetHealth() );
	}

	return flFireDelay;
}

bool CTFWeaponBase::IsBursting(void)
{
	return BaseClass::IsBursting();
}

int CTFWeaponBase::GetSlot( void )
{
	int iSlot = BaseClass::GetSlot();

	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if( pStatic )
		if( pStatic->bucket != -1 )
			iSlot = pStatic->bucket;

	return iSlot;
}

int CTFWeaponBase::GetPosition( void )
{
	int iPosition = BaseClass::GetPosition();

	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if( pStatic )
		if( pStatic->bucket_position != -1 )
			iPosition = pStatic->bucket_position;

	return iPosition;
}

void CTFWeaponBase::SetViewModel()
{
	BaseClass::SetViewModel();

	UpdateViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: Get the proper attachment model for VM.
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateViewModel( void )
{
	CTFPlayer *pTFPlayer = GetTFPlayerOwner();
	if ( !pTFPlayer )
		return;

	CTFViewModel *pViewModel = static_cast<CTFViewModel *>( GetPlayerViewModel() );
	if ( !pViewModel )
		return;

	int iViewModelType = GetViewModelType();

	const char *pszModel;
	if ( iViewModelType == VMTYPE_L4D )
	{
		string_t strHandModelName = MAKE_STRING(pTFPlayer->GetPlayerClass()->GetHandModelName());
		CALL_ATTRIB_HOOK_STRING_ON_OTHER( pTFPlayer, strHandModelName, custom_hand_viewmodel );
		pszModel = STRING(strHandModelName);
	}
	else if ( iViewModelType == VMTYPE_TF2 )
	{
		if ( HasItemDefinition() )
		{
			pszModel = GetItem()->GetPlayerDisplayModel( pTFPlayer->GetPlayerClass()->GetClassIndex() );
		}
		else
		{
			pszModel = GetTFWpnData().szViewModel;
		}
	}
	else
	{
		pszModel = NULL;
	}

	if ( pszModel && pszModel[0] != '\0' )
	{
		pViewModel->UpdateViewmodelAddon( modelinfo->GetModelIndex( pszModel ), this );
	}
	else
	{
		pViewModel->RemoveViewmodelAddon();
	}
}


CBaseViewModel *CTFWeaponBase::GetPlayerViewModel( bool bObserverOK /*= false*/ ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return NULL;

	return pPlayer->GetViewModel( m_nViewModelIndex, bObserverOK );
}


const char *CTFWeaponBase::DetermineViewModelType( const char *pszViewModel ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return pszViewModel;

	CTFWeaponBase *pWeapon = const_cast<CTFWeaponBase *>( this );
	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if ( pStatic )
	{
		pWeapon->SetViewModelType( pStatic->attach_to_hands );
		if ( pWeapon->GetViewModelType() == VMTYPE_TF2 )
			return pPlayer->GetPlayerClass()->GetHandModelName();
	}
	else
	{
		// Default to legacy viewmodels.
		pWeapon->SetViewModelType( VMTYPE_HL2 );
	}

	return pszViewModel;
}


const char *CTFWeaponBase::GetViewModel( int iViewModel ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	int iClass = pOwner ? pOwner->GetPlayerClass()->GetClassIndex() : TF_CLASS_UNDEFINED;
	return DetermineViewModelType( HasItemDefinition() ? GetItem()->GetPlayerDisplayModel( iClass ) : BaseClass::GetViewModel( iViewModel ) );
}


const char *CTFWeaponBase::GetWorldModel( void ) const
{
	// Use model from item schema if we have an item ID.
	if ( HasItemDefinition() )
		return GetItem()->GetWorldDisplayModel();

	return BaseClass::GetWorldModel();
}


bool CTFWeaponBase::CanHolster( void ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.IsMovementLocked() )
		return false;

	return BaseClass::CanHolster();
}


bool CTFWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	if ( m_hMuzzleFlashModel.Get() )
		m_hMuzzleFlashModel->Release();
#endif
	AbortReload();

	return BaseClass::Holster( pSwitchingTo );
}


bool CTFWeaponBase::Deploy( void )
{
	float flOriginalPrimaryAttack = m_flNextPrimaryAttack;
	float flOriginalSecondaryAttack = m_flNextSecondaryAttack;

	bool bDeploy = BaseClass::Deploy();
	if ( bDeploy )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return false;

		// Overrides the anim length for calculating ready time.
		// Don't override primary attacks that are already further out than this. This prevents
		// people exploiting weapon switches to allow weapons to fire faster.
		float flDeployTime = GetTFWpnData().m_flDeployTime;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flDeployTime, mult_deploy_time );
		CALL_ATTRIB_HOOK_FLOAT( flDeployTime, mult_single_wep_deploy_time );

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer->GetLastWeapon(), flDeployTime, mult_switch_from_wep_deploy_time );

		if (pPlayer->m_Shared.InCond(TF_COND_BLASTJUMPING) || pPlayer->m_Shared.InCond(TF_COND_LAUNCHED))
			CALL_ATTRIB_HOOK_FLOAT( flDeployTime, mult_rocketjump_deploy_time );

		if ( pPlayer->m_Shared.GetNumHealers() == 0 )
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flDeployTime, mod_medic_healed_deploy_time );

		if ( pPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
			flDeployTime *= TF2C_TRANQ_DEPLOY_FACTOR;

		m_flNextPrimaryAttack = Max( flOriginalPrimaryAttack, gpGlobals->curtime + flDeployTime );
		m_flNextSecondaryAttack = Max( flOriginalSecondaryAttack, gpGlobals->curtime + flDeployTime );

		pPlayer->SetNextAttack( m_flNextPrimaryAttack );

		CBaseViewModel *pViewModel = pPlayer->GetViewModel( m_nViewModelIndex );
		if ( pViewModel )
		{
			float flDeployTimeAnim = flDeployTime;
			float flDeployTimeAnimMod = 0.0f;

			CALL_ATTRIB_HOOK_FLOAT(flDeployTimeAnimMod, override_mult_deploy_time_anim);
			if (flDeployTimeAnimMod)
			{
				flDeployTimeAnim = GetTFWpnData().m_flDeployTime * flDeployTimeAnimMod;
			}

		    pViewModel->SetPlaybackRate( GetTFWpnData().m_flDeployTime / flDeployTimeAnim);
		}
	}

	return bDeploy;
}


void CTFWeaponBase::GiveTo( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	// Base code kind of assumes that we're getting weapons by picking them up like in HL2
	// but in TF2 we only get weapons at spawn and through special ents and all those extra checks
	// can mess with this.
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );
	if ( pPlayer )
	{
		AddEffects( EF_NODRAW );
		pPlayer->Weapon_Equip( this );
		OnPickedUp( pPlayer );
	}
#endif
}


void CTFWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	// Add it to attribute providers list.
	ReapplyProvision();

	BaseClass::Equip( pOwner );

	CTFPlayer *pTFOwner = GetTFPlayerOwner();
	if ( pTFOwner )
	{
		pTFOwner->TeamFortress_SetSpeed();
		pTFOwner->TeamFortress_SetGravity();
	}

#ifdef GAME_DLL
	UpdateExtraWearable();
#endif
}

#ifdef GAME_DLL

void CTFWeaponBase::UnEquip( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		if ( pOwner->GetActiveWeapon() == this )
		{
			pOwner->ResetAutoaim();
			Holster();
			SetThink( NULL );
			pOwner->ClearActiveWeapon();
		}

		if ( pOwner->GetLastWeapon() == this )
		{
			pOwner->Weapon_SetLast( NULL );
		}

		pOwner->Weapon_Detach( this );
	}

	UTIL_Remove( this );
}
#endif


void CTFWeaponBase::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	ReapplyProvision();

#ifndef CLIENT_DLL
	// Never allow weapons to lie around on the ground
	UTIL_Remove( this );
#endif
}


bool CTFWeaponBase::IsViewModelFlipped( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	bool bFlipped = pOwner ? m_bFlipViewModel != pOwner->ShouldFlipViewModel() : false;
	CEconItemDefinition *pItemDef = GetItem()->GetStaticData();
	if ( HasItemDefinition() && pItemDef && pItemDef->flip_viewmodel )
	{
		bFlipped = !bFlipped;
	}

	return bFlipped;
}


void CTFWeaponBase::ReapplyProvision( void )
{
	// Disguise items never provide.
	if ( m_bDisguiseWeapon )
		return;

	int iProvideOnActive = 0;
	CALL_ATTRIB_HOOK_INT( iProvideOnActive, provide_on_active );
	if ( !iProvideOnActive || m_iState == WEAPON_IS_ACTIVE )
	{
		BaseClass::ReapplyProvision();
	}
	else
	{
		// Weapon not active, remove it from providers list.
		GetAttributeContainer()->StopProvidingTo( GetOwner() );
		m_hOldOwner = NULL;
	}
}


void CTFWeaponBase::OnActiveStateChanged( int iOldState )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		int iProvideOnActive = 0;
		CALL_ATTRIB_HOOK_INT( iProvideOnActive, provide_on_active );

		// If set to only provide attributes while active, update the status now.
		if ( iProvideOnActive )
		{
			ReapplyProvision();
		}

		// Weapon might be giving us speed boost when active.
		pOwner->TeamFortress_SetSpeed();
		pOwner->TeamFortress_SetGravity();
	}

	BaseClass::OnActiveStateChanged( iOldState );
}


void CTFWeaponBase::UpdateOnRemove( void )
{
#ifdef GAME_DLL
	RemoveExtraWearable();
#endif
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PrimaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	// Set when this attack occurred.
	m_flLastPrimaryAttackTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFWeaponBase::SecondaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

	// Don't hook secondary for now.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Most calls use the prediction seed
//-----------------------------------------------------------------------------
void CTFWeaponBase::CalcIsAttackCritical( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( gpGlobals->framecount == m_iLastCritCheckFrame )
		return;

	m_iLastCritCheckFrame = gpGlobals->framecount;

	int iCannotUseStoredCrits = 0;
	CALL_ATTRIB_HOOK_INT(iCannotUseStoredCrits, cannot_use_stored_crits);
	int iCritsWhenHasFullStoredCrits = 0;
	CALL_ATTRIB_HOOK_INT(iCritsWhenHasFullStoredCrits, always_crit_full_stored_crits);
	int iCritWhileBlastJumping = 0;
	CALL_ATTRIB_HOOK_INT(iCritWhileBlastJumping, crit_while_airborne);
	if ( pPlayer->m_Shared.IsCritBoosted() ||
		(m_flCritTime > gpGlobals->curtime) || // Are we still firing a crit burst, whether random crit or stored crit?
		(iCritWhileBlastJumping && (pPlayer->m_Shared.InCond(TF_COND_BLASTJUMPING) || pPlayer->m_Shared.InCond(TF_COND_LAUNCHED))) ||
		(iCritsWhenHasFullStoredCrits && pPlayer->m_Shared.GetStoredCrits() == pPlayer->m_Shared.GetStoredCritsCapacity()))
	{
		m_bCurrentAttackIsCrit = true;
	}
	else if (pPlayer->m_Shared.GetStoredCrits() && !iCannotUseStoredCrits)
	{
		pPlayer->m_Shared.RemoveStoredCrits(1);
		if (GetTFWpnData().GetWeaponData(m_iWeaponMode).m_bUseRapidFireCrits)
		{
			float fCritTimeModifier = 1;
			CALL_ATTRIB_HOOK_FLOAT(fCritTimeModifier, mult_rapidfire_crit_duration);
			m_flCritTime = gpGlobals->curtime + (fCritTimeModifier * tf2c_storedcrits_rapidfire_duration.GetFloat());
			m_bCritTimeIsStoredCrit = true;
		}
		m_bCurrentAttackIsCrit = true;
	}
	else
	{
		// Call the weapon-specific helper method.
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelper();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weapon-specific helper method to calculate if attack is crit
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT bool CTFWeaponBase::CalcIsAttackCriticalHelper()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	if ( !CanFireRandomCrit() )
		return false;
	
	float flDamage = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	int nBulletsPerShot = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_nBulletsPerShot;
	CALL_ATTRIB_HOOK_INT( nBulletsPerShot, mult_bullets_per_shot );

	flDamage *= (float)nBulletsPerShot;
	AddToCritBucket( flDamage );

	float flPlayerCritMult = pPlayer->GetCritMult();

	if ( GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_bUseRapidFireCrits )
	{
		// Only perform one crit check per second for rapid fire weapons.
		if ( gpGlobals->curtime < m_flLastCritCheckTime + 1.0f )
			return false;

		m_flLastCritCheckTime = gpGlobals->curtime;

		// Get the total crit chance (ratio of total shots fired we want to be crits) and bail if it's 0.
		float flTotalCritChance = GetCritChance() * flPlayerCritMult;
		CALL_ATTRIB_HOOK_FLOAT( flTotalCritChance, mult_crit_chance );
		if ( flTotalCritChance == 0.0f )
			return false;

		flTotalCritChance = clamp( flTotalCritChance, 0.01f, 0.99f );
		// Get the fixed amount of time that we start firing crit shots for.
		float flCritDuration = TF_DAMAGE_CRIT_DURATION_RAPID;
		// Calculate the amount of time, on average, that we want to NOT fire crit shots for in order to achive the total crit chance we want.
		float flNonCritDuration = ( flCritDuration / flTotalCritChance ) - flCritDuration;
		// Calculate the chance per second of non-crit fire that we should transition into critting such that on average we achieve the total crit chance we want.
		float flStartCritChance = 1 / flNonCritDuration;

		// See if we should start firing crit shots.
		m_nCritChecks++;
		if ( SharedRandomInt( "RandomCritRapid", 0, WEAPON_RANDOM_RANGE - 1 ) < ( flStartCritChance * WEAPON_RANDOM_RANGE ) && IsAllowedToWithdrawFromCritBucket( flDamage ) )
		{
			m_flCritTime = gpGlobals->curtime + TF_DAMAGE_CRIT_DURATION_RAPID;
			m_bCritTimeIsStoredCrit = false;
			return true;
		}

		return false;
	}
	else
	{
		// Single-shot weapon, just use random pct per shot and bail if it's 0.
		float flCritChance = GetCritChance() * flPlayerCritMult;
		CALL_ATTRIB_HOOK_FLOAT( flCritChance, mult_crit_chance );
		if ( flCritChance == 0.0f )
			return false;

		m_nCritChecks++;
		return ( SharedRandomInt( "RandomCrit", 0, WEAPON_RANDOM_RANGE - 1 ) < ( flCritChance * WEAPON_RANDOM_RANGE ) && IsAllowedToWithdrawFromCritBucket( flDamage ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can this weapon fire a random crit?
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT bool CTFWeaponBase::CanFireRandomCrit( void )
{
	bool bForce = false;
	CALL_ATTRIB_HOOK_INT(bForce, mod_crit_chance_force_enable);
	if (bForce)
		return true;

	// Random crits are disabled in VIP due to balancing reasons.
	if ( !tf2c_vip_criticals.GetBool() && TFGameRules()->IsVIPMode() )
		return false;

	if ( !tf_weapon_criticals.GetBool() )
		return false;

	return true;
}

bool CTFWeaponBase::ShouldShowCritMeter(void) 
{
	bool shouldShow = CanFireRandomCrit();
	if (GetWeaponID() == TF_WEAPON_ANCHOR || GetWeaponID() == TF_WEAPON_KNIFE)
		shouldShow = false;
	float flAddsUber = 0;
	CALL_ATTRIB_HOOK_FLOAT(flAddsUber, add_onhit_ubercharge);
	if (flAddsUber > 0)
		shouldShow = false;
	float flCritMult = 1;
	CALL_ATTRIB_HOOK_FLOAT(flCritMult, mult_crit_chance);
	if (flCritMult == 0.0f)
		shouldShow = false;
	return shouldShow;
}

float CTFWeaponBase::GetCritChance( void )
{
	if ( GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_bUseRapidFireCrits )
		return TF_DAMAGE_CRIT_CHANCE_RAPID;

	return TF_DAMAGE_CRIT_CHANCE;
}


bool CTFWeaponBase::CanHeadshot( void )
{
	int iCanHeadshot = 0;
	CALL_ATTRIB_HOOK_INT(iCanHeadshot, can_headshot);
	if (iCanHeadshot)
		return true;

	int iNoHeadshot = 0;
	CALL_ATTRIB_HOOK_INT(iNoHeadshot, no_headshot);
	if (iNoHeadshot)
		return false;

	return false;
}


int CTFWeaponBase::GetMaxClip1( void ) const
{
	float flMaxClip = (float)CBaseCombatWeapon::GetMaxClip1();
	if ( flMaxClip == WEAPON_NOCLIP )
		return (int)flMaxClip;

	CALL_ATTRIB_HOOK_FLOAT( flMaxClip, mult_clipsize );

	int iIncreaseClipSizeOnKill = 0;
	CALL_ATTRIB_HOOK_INT( iIncreaseClipSizeOnKill, clipsize_increase_on_kill );
	if ( iIncreaseClipSizeOnKill )
	{
		int iFinalIncrease = iIncreaseClipSizeOnKill * GetKillCount();

		int iIncreaseClipSizeOnKillCap = 0;
		CALL_ATTRIB_HOOK_INT( iIncreaseClipSizeOnKillCap, clipsize_increase_on_kill_cap );
		if(iIncreaseClipSizeOnKillCap > 0)
			iFinalIncrease = Min( iFinalIncrease, iIncreaseClipSizeOnKillCap );

		flMaxClip += iFinalIncrease;
	}

	int iMaxClip = (int)( flMaxClip + 0.5f );
	if ( iMaxClip != 1 && tf2c_weapon_noreload.GetBool() )
		return WEAPON_NOCLIP;

	// Round to the nearest integer.
	return iMaxClip;
}


int CTFWeaponBase::GetDefaultClip1( void ) const
{
	float flDefaultClip = (float)CBaseCombatWeapon::GetDefaultClip1();
	CALL_ATTRIB_HOOK_FLOAT( flDefaultClip, mult_clipsize );
	if ( flDefaultClip == WEAPON_NOCLIP )
		return (int)flDefaultClip;

	int iIncreaseClipSizeOnKill = 0;
	CALL_ATTRIB_HOOK_INT( iIncreaseClipSizeOnKill, clipsize_increase_on_kill );
	if ( iIncreaseClipSizeOnKill )
	{
		int iFinalIncrease = iIncreaseClipSizeOnKill * GetKillCount();

		int iIncreaseClipSizeOnKillCap = 0;
		CALL_ATTRIB_HOOK_INT( iIncreaseClipSizeOnKillCap, clipsize_increase_on_kill_cap );
		if(iIncreaseClipSizeOnKillCap > 0)
			iFinalIncrease = Min( iFinalIncrease, iIncreaseClipSizeOnKillCap );

		flDefaultClip += iFinalIncrease;
	}

	int iDefaultClip = (int)( flDefaultClip + 0.5f );
	if ( iDefaultClip != 1 && tf2c_weapon_noreload.GetBool() )
		return WEAPON_NOCLIP;

	// Round to the nearest integer.
	return iDefaultClip;
}


int CTFWeaponBase::GetMaxAmmo( void )
{
	int iMaxAmmo = GetTFWpnData().m_iMaxAmmo;

	if ( TFGameRules()->IsInMedievalMode() )
	{
		iMaxAmmo = Max( iMaxAmmo/2, 1 );
	}

	return iMaxAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: Whether the gun is loaded or not.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::HasPrimaryAmmoToFire( void )
{
	if ( !UsesPrimaryAmmo() )
		return true;

	if ( UsesClipsForAmmo1() )
		return ( Clip1() > 0 );

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	return (pOwner->GetAmmoCount( GetPrimaryAmmoType() ) > 0) || IsAmmoInfinite();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Reload( void )
{
	// Sorry people, no speeding it up.
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	// Can't reload if we're already reloading.
	if ( IsReloading() )
		return false;

	// Can't reload if there's no one to reload us.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( m_bUsesAmmoMeter )
		return false;

	// Can't reload while taunting.
	if ( pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	// Can't reload while cloaked.
	int iCanReloadWhileCloaked = 0;
	CALL_ATTRIB_HOOK_INT(iCanReloadWhileCloaked, mod_can_reload_in_cloak);
	if (pOwner->m_Shared.InCond(TF_COND_STEALTHED) && !iCanReloadWhileCloaked)
		return false;

	// If our stats dictate that we have infinite reserve ammo, ignore the remaining ammo checks.
	// If I don't have any spare ammo, I can't reload.
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 && !IsAmmoInfinite() )
		return false;

	// I can't fit anymore bullets into me, I can't reload.
	int iClip1 = Clip1(), iMaxClip1 = GetMaxClip1();
	if ( iClip1 >= iMaxClip1 )
		return false;

	// Reload one object at a time.
	if ( ReloadsSingly() )
		return ReloadSingly();

	// If we have this sequence, use it when we're out of bullets.
	int iEmptyReloadSequence = SelectWeightedSequence( ACT_VM_RELOAD_EMPTY );
	if ( iEmptyReloadSequence != ACTIVITY_NOT_AVAILABLE && iClip1 == 0 )
		return DefaultReload( iMaxClip1, GetMaxClip2(), ACT_VM_RELOAD_EMPTY );

	// Normal reload.
	return DefaultReload( iMaxClip1, GetMaxClip2(), ACT_VM_RELOAD );
}


void CTFWeaponBase::CheckReload( void )
{
	if ( !IsReloading() )
		return;

	if ( ReloadsSingly() )
	{
		ReloadSingly();
	}
	else if ( gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		FinishReload();
	}
}


void CTFWeaponBase::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		bool bInfiniteReserveAmmo = IsAmmoInfinite();

		// If I use primary clips, reload primary.
		if ( UsesClipsForAmmo1() )
		{
			int iPrimary = Min( GetMaxClip1() - m_iClip1, bInfiniteReserveAmmo ? GetMaxClip1() - m_iClip1 : pOwner->GetAmmoCount( m_iPrimaryAmmoType ) );
			m_iClip1 += iPrimary;
			if (!bInfiniteReserveAmmo)
				pOwner->RemoveAmmo( iPrimary, m_iPrimaryAmmoType );
		}

		// If I use secondary clips, reload secondary.
		if ( UsesClipsForAmmo2() )
		{
			int iSecondary = Min( GetMaxClip2() - m_iClip2, bInfiniteReserveAmmo ? GetMaxClip2() - m_iClip2 : pOwner->GetAmmoCount( m_iSecondaryAmmoType ) );
			m_iClip2 += iSecondary;
			if (!bInfiniteReserveAmmo)
				pOwner->RemoveAmmo( iSecondary, m_iSecondaryAmmoType );
		}
	}

	m_iReloadMode.Set( TF_RELOAD_START );
}


void CTFWeaponBase::AbortReload( void )
{
	if ( IsReloading() )
	{
		if ( !ReloadsSingly() )
		{
			// Be more generous when reload is interrupted as many of them include lengthy return to idle frames.
			// Count it as a successful reload if we went through 75% of the animation.
			// (Around that time most weapons play their reload sounds and begin fading to idle.)
			float flFullReloadTime = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReload;

			CALL_ATTRIB_HOOK_FLOAT( flFullReloadTime, mult_reload_time );
			CALL_ATTRIB_HOOK_FLOAT( flFullReloadTime, mult_reload_time_hidden );
			CALL_ATTRIB_HOOK_FLOAT( flFullReloadTime, mult_reload_time_noanimmod );
			CALL_ATTRIB_HOOK_FLOAT( flFullReloadTime, fast_reload );

			CTFPlayer* pOwner = GetTFPlayerOwner();
			if ( pOwner )
			{
				if ( pOwner->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
				{
					flFullReloadTime *= TF2C_TRANQ_RELOAD_FACTOR;
				}

				if ( pOwner->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
				{
					flFullReloadTime *= TF2C_HASTE_RELOAD_FACTOR;
				}
			}

			if ( gpGlobals->curtime >= m_flNextPrimaryAttack - ( flFullReloadTime * 0.20f ) )
			{
				FinishReload();
			}
		}

		BaseClass::AbortReload();

		m_iReloadMode.Set( TF_RELOAD_START );
		m_flNextPrimaryAttack = gpGlobals->curtime;
		float flTimeIdle = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flTimeIdle;
		CALL_ATTRIB_HOOK_FLOAT( flTimeIdle, mult_weapon_idle_time ); 
		SetWeaponIdleTime( gpGlobals->curtime + flTimeIdle );

		// Make sure any reloading bodygroups are hidden.
		int iReloadBodygroup = FindBodygroupByName( "reload" );
		if ( iReloadBodygroup >= 0 )
		{
			SetBodygroup( iReloadBodygroup, 0 );
		}
	}
}


float CTFWeaponBase::ApplyFireDelay( float flDelay ) const
{
	float flDelayMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDelayMult, mult_postfiredelay );

	// Faster fire delay during Haste buff
	CTFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flDelayMult *= 0.5f;
	}

	return flDelay * flDelayMult;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ReloadSingly( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// check to see if we're ready to reload
	switch ( m_iReloadMode )
	{
		case TF_RELOAD_START:
		{
			// Play weapon and player animations.
			SendWeaponAnim( ACT_RELOAD_START );
			UpdateReloadTimers( true );

			// Next reload the shells.
			m_iReloadMode.Set( TF_RELOADING );

			m_iReloadStartClipAmount = Clip1();

			return true;
		}
		case TF_RELOADING:
		{
			// Waiting for the reload start animation to complete.
			if ( m_flNextPrimaryAttack > gpGlobals->curtime )
				return false;

			if ( Clip1() == GetMaxClip1() )
			{
				// Clip was refilled during reload start animation, abort.
				m_iReloadMode.Set( TF_RELOAD_FINISH );
				return true;
			}

			// Play weapon reload animations and sound.
			if ( Clip1() == m_iReloadStartClipAmount )
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
			}
			else
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
			}

			m_bReloadedThroughAnimEvent = false;

			SendWeaponAnim( ACT_VM_RELOAD );
			UpdateReloadTimers( false );
			OnReloadSinglyUpdate();

			PlayReloadSound();

			// Next continue to reload shells?
			m_iReloadMode.Set( TF_RELOADING_CONTINUE );

			return true;
		}
		case TF_RELOADING_CONTINUE:
		{
			// Check if this weapon refills clip midway through reload.
			if ( !m_bReloadedThroughAnimEvent && m_flClipRefillTime != 0.0f && gpGlobals->curtime >= m_flClipRefillTime )
			{
				IncrementAmmo();

				m_bReloadedThroughAnimEvent = true;
				return true;
			}

			// Still waiting for the reload to complete.
			if ( m_flNextPrimaryAttack > gpGlobals->curtime )
				return false;

			IncrementAmmo();

			if ( Clip1() == GetMaxClip1() || (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 && !IsAmmoInfinite()) )
			{
				m_iReloadMode.Set( TF_RELOAD_FINISH );
			}
			else
			{
				m_iReloadMode.Set( TF_RELOADING );
			}

			return true;
		}

		case TF_RELOAD_FINISH:
		default:
		{
			SendWeaponAnim( ACT_RELOAD_FINISH );

			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );
			OnReloadSinglyUpdate();

			m_iReloadMode.Set( TF_RELOAD_START );
			return true;
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup and check for reload.
	bool bReloadPrimary = false;
	bool bReloadSecondary = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// Need to reload primary clip?
		int iPrimary = Min( iClipSize1 - m_iClip1, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		if ( iPrimary != 0 )
		{
			bReloadPrimary = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// Need to reload secondary clip?
		int iSecondary = Min( iClipSize2 - m_iClip2, pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) );
		if ( iSecondary != 0 )
		{
			bReloadSecondary = true;
		}
	}

	// We didn't reload.
	if ( !( bReloadPrimary || bReloadSecondary ) )
		return false;

	// Play the reload sound.
	PlayReloadSound();

	// Play the player's reload animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	SendWeaponAnim( iActivity );

	// Don't rely on animations, always use script time.
	UpdateReloadTimers( false );
	m_iReloadMode.Set( TF_RELOADING );

	return true;
}


void CTFWeaponBase::UpdateReloadTimers( bool bStart )
{
	// Starting a reload?
	if ( bStart )
	{
		// Get the reload start time.
		float flTimeReloadStart = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flTimeReloadStart;
		CALL_ATTRIB_HOOK_FLOAT( flTimeReloadStart, mult_reload_time_start );
		SetReloadTimer( flTimeReloadStart );
	}
	// In reload.
	else
	{
		float flTimeReload = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flTimeReload;
		CALL_ATTRIB_HOOK_FLOAT( flTimeReload, mult_reload_time_noanimmod );
		float flTimeReloadRefill = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flTimeReloadRefill;
		CALL_ATTRIB_HOOK_FLOAT( flTimeReloadRefill, mult_reload_time_refill );
		SetReloadTimer( flTimeReload, flTimeReloadRefill );
	}
}


void CTFWeaponBase::SetReloadTimer( float flReloadTime, float flRefillTime /*= 0.0f*/ )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	float flModifiedTime = flReloadTime;
	CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, mult_reload_time );
	CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, mult_reload_time_hidden );
	CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, fast_reload );

	if ( pPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		flModifiedTime *= TF2C_TRANQ_RELOAD_FACTOR;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flModifiedTime *= TF2C_HASTE_RELOAD_FACTOR;
	}

	CBaseViewModel *pViewModel = GetPlayerViewModel();
	if ( pViewModel )
	{
		float flReloadTimeAnimOverride = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT(flReloadTimeAnimOverride, override_mult_reload_time_anim);
		if (flReloadTimeAnimOverride)
		{
			flModifiedTime = flReloadTime * flReloadTimeAnimOverride;
		}

		pViewModel->SetPlaybackRate( flReloadTime / flModifiedTime );
	}

	// Set next weapon attack times (based on reloading).
	m_flNextPrimaryAttack = gpGlobals->curtime + flModifiedTime;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = flTime;

	float flAddRefillTime = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flAddRefillTime, add_reload_refill_time );

	float flTimeRatio = flModifiedTime / flReloadTime;
	if ( flRefillTime != 0.0f )
	{
		m_flClipRefillTime = gpGlobals->curtime + flRefillTime + flAddRefillTime * flTimeRatio;
	}
	else
	{
		m_flClipRefillTime = 0.0f;
	}

	// Set next idle time (based on reloading).
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() * flTimeRatio );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::PlayReloadSound( void )
{
#ifdef CLIENT_DLL
	// Don't play world reload sound in first person, viewmodel will take care of this.
	if ( UsingViewModel() )
		return;
#endif

	WeaponSound( RELOAD );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::IncrementAmmo( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	bool bInfiniteReserveAmmo = IsAmmoInfinite();

	// If we have ammo, remove ammo and add it to clip.
	int iMaxClip1 = GetMaxClip1();
	if ( !m_bReloadedThroughAnimEvent && (Clip1() < iMaxClip1 && (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0) || bInfiniteReserveAmmo) )
	{
		int iAmmoToReload = 1;
		CALL_ATTRIB_HOOK_INT ( iAmmoToReload, mod_ammo_per_reload );
		iAmmoToReload = Max( iAmmoToReload, 0 );

		m_iClip1 = Min( m_iClip1 + iAmmoToReload, iMaxClip1 );
		if ( !bInfiniteReserveAmmo )
			pPlayer->RemoveAmmo( iAmmoToReload, m_iPrimaryAmmoType );
	}
}

// -----------------------------------------------------------------------------
// Purpose: Reload as much as possible into clip instantly on function call
// -----------------------------------------------------------------------------
void CTFWeaponBase::InstantlyReload(int iAmount)
{
	if (!UsesClipsForAmmo1())
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	bool bInfiniteReserveAmmo = IsAmmoInfinite();

	int iAmmoUsedToFill = Min( Min( GetMaxClip1() - m_iClip1, iAmount ), bInfiniteReserveAmmo ? iAmount : pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
	m_iClip1 += iAmmoUsedToFill;
	if ( !bInfiniteReserveAmmo )
		pPlayer->RemoveAmmo(iAmmoUsedToFill, m_iPrimaryAmmoType);
}

// -----------------------------------------------------------------------------
// Purpose: Returns override from item schema if there is one.
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetShootSound( int iIndex ) const
{
	if ( HasItemDefinition() )
	{
		const char *pszOverride = GetItem()->GetSoundOverride( iIndex, GetTeamNumber() );
		if ( pszOverride && pszOverride[0] )
			return pszOverride;
	}

	return BaseClass::GetShootSound( iIndex );
}


void CTFWeaponBase::ItemBusyFrame( void )
{
	// Call into the base ItemBusyFrame.
	BaseClass::ItemBusyFrame();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		if ( !m_bInAttack2 )
		{
			pOwner->DoClassSpecialSkill();
			m_bInAttack2 = true;
		}
	}
	else
	{
		m_bInAttack2 = false;
	}

	CheckEffectBarRegen();
}


void CTFWeaponBase::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// Interrupt a reload.
	if ( IsReloading() && Clip1() > 0 && ( pOwner->m_nButtons & IN_ATTACK ) )
	{
		AbortReload();
	}

	// debounce InAttack flags
	if ( m_bInAttack && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

	CheckEffectBarRegen();

	// Call the base item post frame.
	BaseClass::ItemPostFrame();
}


void CTFWeaponBase::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();
	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetWeaponVisible( bool visible )
{
	if ( visible && WeaponShouldBeVisible() )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}

#ifdef CLIENT_DLL
	UpdateVisibility();

	// Force an update
	PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );
#endif
}


bool CTFWeaponBase::WeaponShouldBeVisible( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( HideWhileStunned() && ( pOwner->m_Shared.IsControlStunned() || pOwner->m_Shared.IsLoser() ) )
		return false;

	return true;
}

bool CTFWeaponBase::CanAutoReload( void )
{
	bool bCanAutoReload = false;
	CALL_ATTRIB_HOOK_INT( bCanAutoReload, mod_no_autoreload );
	return !bCanAutoReload;
}

//-----------------------------------------------------------------------------
// Purpose: If the current weapon has more ammo, reload it. Otherwise, switch 
//			to the next best weapon we've got. Returns true if it took any action.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ReloadOrSwitchWeapons( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// Weapon isn't useable, switch.
		if ( !( GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY ) && g_pGameRules->SwitchToNextBestWeapon( pOwner, this ) )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;
			return true;
		}
	}
	// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing,
	// also auto-reload if owner has auto-reload enabled.
	else if ( UsesClipsForAmmo1() && !AutoFiresFullClip() && 
		( m_iClip1 == 0 || ( pOwner && pOwner->ShouldAutoReload() && m_iClip1 < GetMaxClip1() && CanAutoReload() ) ) &&
		!( GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD ) &&
		m_flNextPrimaryAttack < gpGlobals->curtime &&
		m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// If we're successfully reloading, we're done.
		if ( Reload() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CTFWeaponBase::WeaponIdle( void )
{
	if ( IsReloading() )
		return;

	BaseClass::WeaponIdle();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashModel( void )
{
	const char *pszModel = GetTFWpnData().m_szMuzzleFlashModel;
	if ( pszModel[0] != '\0' )
		return pszModel;

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashParticleEffect( void )
{
	const char *pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffect;
	if ( pszPEffect[0] != '\0' )
		return pszPEffect;

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetMuzzleFlashModelLifetime( void )
{
	return GetTFWpnData().m_flMuzzleFlashModelDuration;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetMuzzleFlashModelScale( void )
{
	return GetTFWpnData().m_flMuzzleFlashModelScale;
}


const char *CTFWeaponBase::GetTracerType( void )
{
	if ( !m_szTracerName[0] )
	{
		const char *pszTracerName;
		CEconItemDefinition *pItemDef = GetItem()->GetStaticData();
		if ( pItemDef && pItemDef->GetVisuals( GetTeamNumber() )->tracer_effect[0] )
		{
			pszTracerName = pItemDef->GetVisuals( GetTeamNumber() )->tracer_effect;
		}
		else
		{
			pszTracerName = GetTFWpnData().m_szTracerEffect;
		}

		if ( pszTracerName[0] )
		{
			const char *pszTeamName = GetTeamSuffix( GetTeamNumber() );
			V_sprintf_safe( m_szTracerName, "%s_%s", pszTracerName, pszTeamName );
		}
	}

	if ( IsCurrentAttackACrit() && m_szTracerName[0] )
	{
		static char szCritEffect[MAX_TRACER_NAME];
		V_sprintf_safe( szCritEffect, "%s_crit", m_szTracerName );
		return szCritEffect;
	}

	return m_szTracerName;
}


void CTFWeaponBase::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr )
{
	const char *pszTracerEffect = GetTracerType();
	if ( pszTracerEffect && pszTracerEffect[0] )
	{
		int iAttachment = GetTracerAttachment();

		CEffectData data;
		data.m_vStart = vecTracerSrc;
		data.m_vOrigin = tr.endpos;
#ifdef CLIENT_DLL
		data.m_hEntity = this;
#else
		data.m_nEntIndex = entindex();
#endif
		data.m_nHitBox = GetParticleSystemIndex( pszTracerEffect );

		// Flags
		data.m_fFlags |= TRACER_FLAG_WHIZ;

		if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
		{
			data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
			data.m_nAttachmentIndex = iAttachment;
		}

		DispatchEffect( "TFParticleTracer", data );
	}
}

//-----------------------------------------------------------------------------
// Default tracer attachment
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetTracerAttachment( void )
{
#ifdef GAME_DLL
	// NOTE: The server has no way of knowing the correct index due to differing models.
	// So we're only returning 1 to indicate that this weapon does use an attachment for tracers.
	return 1;
#else
	return GetWeaponForEffect()->LookupAttachment( "muzzle" );
#endif
}


bool CTFWeaponBase::CanFireAccurateShot( int nBulletsPerShot )
{
	float flFireInterval = gpGlobals->curtime - m_flLastPrimaryAttackTime;
	return ( nBulletsPerShot == 1 ? flFireInterval > 1.25f : flFireInterval > 0.25f );
}

//-----------------------------------------------------------------------------
// Purpose: Get the current bar state (will return a value from 0.0 to 1.0)
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetEffectBarProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) || (m_bUsesAmmoMeter && Clip1() < GetMaxClip1()) ) )
	{
		float flTime = GetEffectBarRechargeTime();
		return ( flTime - ( m_flEffectBarRegenTime - gpGlobals->curtime ) ) / flTime;
	}

	return 1.0f;
}

float CTFWeaponBase::GetEffectBarRechargeTime( void )
{
	float flTime = InternalGetEffectBarRechargeTime();
	CALL_ATTRIB_HOOK_FLOAT( flTime, effectbar_recharge_rate );

	// Faster recharge rate during Haste buff
	CTFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		flTime *= 0.33f;
	}

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: Decrement time, like giving bonus
//-----------------------------------------------------------------------------
void CTFWeaponBase::DecrementBarRegenTime(float flTime, bool bCarryOver )
{
	m_flEffectBarRegenTime = bCarryOver
		? m_flEffectBarRegenTime - flTime
		: Max(gpGlobals->curtime, m_flEffectBarRegenTime - flTime);
	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: Start the regeneration bar charging from this moment in time
//-----------------------------------------------------------------------------
void CTFWeaponBase::StartEffectBarRegen( bool bForceReset )
{
	// Only reset regen if its less then curr time or we were full
	//CTFPlayer *pPlayer = GetTFPlayerOwner();
	/*bool bWasFull = false;
	if ( pPlayer && ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) ) )
	{
		bWasFull = true;
	}*/

	if ( m_flEffectBarRegenTime < gpGlobals->curtime /*|| bWasFull*/ || bForceReset ) 
	{
		m_flEffectBarRegenTime = gpGlobals->curtime + GetEffectBarRechargeTime();
	}	
}


void CTFWeaponBase::CheckEffectBarRegen( void ) 
{ 
	if ( !m_flEffectBarRegenTime )
		return;
	
	if ( !m_bUsesAmmoMeter )
	{
		// If we're full stop the timer.  Fixes a bug with "double" throws after respawning or touching a supply cab.
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
		{
			m_flEffectBarRegenTime = 0.0f;
			return;
		}
	}

	if ( m_flEffectBarRegenTime < gpGlobals->curtime ) 
	{
		float flCarryOverTime = gpGlobals->curtime - m_flEffectBarRegenTime;
		m_flEffectBarRegenTime = 0.0f;
		EffectBarRegenFinished(flCarryOverTime);
	}
}


void CTFWeaponBase::EffectBarRegenFinished( float flCarryOverTime )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		if ( m_bUsesAmmoMeter )
		{
			if ( Clip1() < GetMaxClip1() )
			{
#ifdef GAME_DLL
				IncrementAmmo();

				// If we still have more ammo space, recharge.
				if ( Clip1() < GetMaxClip1() )
#else
				// On the client, we assume we'll get 1 more ammo as soon as the server updates us, so only restart if that still won't make us full.
				if ( Clip1() + 1 < GetMaxClip1() )
#endif
				{
					if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 || IsAmmoInfinite() )
					{
						StartEffectBarRegen();
						DecrementBarRegenTime( flCarryOverTime );
					}
				}
				else
				{
					m_flEffectBarRegenTime = 0.0f;
				}
			}
		}
		else if ( (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() )) )
		{
#ifdef GAME_DLL
			pPlayer->GiveAmmo( 1, GetEffectBarAmmo(), true );
#endif

#ifdef GAME_DLL
			// If we still have more ammo space, recharge.
			if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#else
			// On the client, we assume we'll get 1 more ammo as soon as the server updates us, so only restart if that still won't make us full.
			if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#endif
			{
				StartEffectBarRegen();
				DecrementBarRegenTime( flCarryOverTime );
			}
			else
			{
				m_flEffectBarRegenTime = 0.0f;
			}
		}
	}
}


void CTFWeaponBase::OnControlStunned( void )
{
	AbortReload();
#ifdef CLIENT_DLL
	UpdateVisibility();
#endif
}

//=============================================================================
//
// TFWeaponBase functions (Server specific).
//
#if !defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM ) /*&& ( pEvent->type & AE_TYPE_SERVER )*/ )
	{
		// Do nothing.
		if ( pEvent->event == AE_WPN_INCREMENTAMMO )
			return;
	}

	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::FallInit( void )
{
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::CheckRespawn()
{
	// Do not respawn.
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	m_iReloadMode.Set( TF_RELOAD_START );

	m_bResetParity = !m_bResetParity;

	m_flCritTime = 0.0f;
	m_bCritTimeIsStoredCrit = false;
	m_flLastPrimaryAttackTime = 0.0f;
	m_flDamageBoostTime = 0.0f;
	m_flEffectBarRegenTime = 0.0f;
#ifdef ITEM_TAUNTING
	m_nTauntModelIndex = -1;
#endif
	m_szTracerName[0] = '\0';

	m_iKillCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponRegenerate( void )
{
	UpdateExtraWearable();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
const Vector &CTFWeaponBase::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::ApplyOnHitAttributes( CBaseEntity *pAttacker, CBaseEntity *pBaseVictim, const CTakeDamageInfo &info )
{
	CTFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
	CTFPlayer* pVictim = ToTFPlayer(pBaseVictim);
	CBaseObject *pBuilding = dynamic_cast<CBaseObject *>(pBaseVictim);

	// Below code applies no matter if victim was a player or building and attacker is alive and player
	
	if (pTFAttacker && pTFAttacker->IsAlive() && pTFAttacker == GetOwnerEntity())
	{
		bool bPlayInstantReloadPing = false;
		int iInstantReloadOnHit = 0;
		CALL_ATTRIB_HOOK_INT(iInstantReloadOnHit, instant_reload_on_hit);
		int iInstantReloadOnDirectHit = 0;
		CALL_ATTRIB_HOOK_INT(iInstantReloadOnDirectHit, instant_reload_on_direct_hit);
		if (iInstantReloadOnHit > 0)
		{
			InstantlyReload(iInstantReloadOnHit);
			bPlayInstantReloadPing = true;
		}
		if (iInstantReloadOnDirectHit > 0 && (!(info.GetDamageType() & DMG_BLAST) || (info.GetInflictor() && info.GetInflictor()->GetEnemy() == pBaseVictim)))
		{
			InstantlyReload(iInstantReloadOnDirectHit);
			bPlayInstantReloadPing = true;
		}

		if (bPlayInstantReloadPing)
		{
			EmitSound_t params;
			params.m_pSoundName = "InstantReloadPing";
			CSingleUserRecipientFilter filter(pTFAttacker);
			EmitSound(filter, pTFAttacker->entindex(), params);
		}

		// !!! azzy brimstone
		if (GetWeaponID() == TF_WEAPON_BRIMSTONELAUNCHER) // anti-airblast
		{
			CTFBrimstoneLauncher* pBrimstone = static_cast<CTFBrimstoneLauncher*>(this);
			pBrimstone->OnDamage( info.GetDamage() - info.GetDamageBonus() );
		}
	}

	// Outling applies to buildings, so do that here:
	if ( pBuilding )
	{
		float flOutlineOnHit = 0;
		CALL_ATTRIB_HOOK_FLOAT( flOutlineOnHit, outline_on_hit );
		if ( flOutlineOnHit != 0 )
		{
			pBuilding->OutlineForEnemy( flOutlineOnHit );
		}
	}

	if (!pVictim)
		return;

	// Below code applies only if victim is a player, attacker may or may not exist or be alive

	// I think that this should go into player's on-hit function
	int iViewPunch = 0;
	CALL_ATTRIB_HOOK_INT( iViewPunch, add_viewpunch_onhit );
	if ( iViewPunch )
	{
		QAngle angPunch(RandomFloat(45.0f, 60.0f), RandomFloat(-60.0f, 60.0f), 0.0f);
		pVictim->ViewPunch(angPunch);

		/*
		QAngle oldang = pVictim->GetAbsAngles();

		QAngle newang;
		newang = oldang + angPunch;

		pVictim->Teleport(NULL, &newang, NULL);
		pVictim->SnapEyeAngles(newang);
		pVictim->DoAnimationEvent(PLAYERANIMEVENT_SNAP_YAW);
		*/
	}

	string_t strCustomViewPunch = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING(strCustomViewPunch, add_custom_viewpunch_onhit);
	if ( strCustomViewPunch != NULL_STRING )
	{
		float args[6];
		UTIL_StringToFloatArray(args, 6, strCustomViewPunch.ToCStr());

		QAngle angPunch(RandomFloat(args[0], args[1]), RandomFloat(args[2], args[3]), RandomFloat(args[4], args[5]));
		pVictim->ViewPunch(angPunch);
	}

	// We're passing in attacker since we want to apply on hit effects to the actual attacker in case of deflect kills.
	if ( !pTFAttacker || !pTFAttacker->IsAlive() )
		return;

	// Below code applies only if victim is a player and attacker exists and is alive and is a player

#define TF_EXPLODE_ON_DAMAGE_TO_SELF		50
#define TF_EXPLODE_ON_DAMAGE_TO_ENEMY		60

	// Victim explodes when a damage threshold is met
	float flExplodeOnDamage = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flExplodeOnDamage, explode_on_damage );
	if ( flExplodeOnDamage > 0.0f )
	{
		if ( info.GetDamage() >= flExplodeOnDamage && pVictim && info.GetAttacker() != pVictim && info.GetDamageCustom() != TF_DMG_CUSTOM_PUMPKIN_BOMB )
		{
#ifdef GAME_DLL
			CDisablePredictionFiltering disabler;

			Vector vecBlastOrigin = info.GetDamagePosition();
			float flRadius = 146.0f;
			int iDmgType = DMG_BLAST | DMG_HALF_FALLOFF;

			// This explosion hurts enemies of the victim. This way it can be used offensively and not for griefing.
			CTFRadiusDamageInfo radiusInfo;
			radiusInfo.info.Set( pVictim, pVictim, pVictim->GetActiveTFWeapon(), vec3_origin, vecBlastOrigin, TF_EXPLODE_ON_DAMAGE_TO_ENEMY, iDmgType );
			radiusInfo.m_pEntityIgnore = pVictim;
			radiusInfo.m_vecSrc = vecBlastOrigin;
			radiusInfo.m_flRadius = flRadius;
			radiusInfo.m_bStockSelfDamage = false;
			radiusInfo.info.SetDamageCustom( TF_DMG_CUSTOM_PUMPKIN_BOMB );

			TFGameRules()->RadiusDamage( radiusInfo );

			// This explosion hurts only the victim. This is so enemies are credited and get to see the extra damage they inflicted.
			CTFRadiusDamageInfo radiusInfoVictim;
			radiusInfoVictim.info.Set( pTFAttacker, pTFAttacker, pTFAttacker->GetActiveTFWeapon(), vec3_origin, pVictim->GetAbsOrigin(), TF_EXPLODE_ON_DAMAGE_TO_SELF, iDmgType );
			radiusInfoVictim.info.SetReportedPosition( pTFAttacker->GetAbsOrigin() );
			radiusInfoVictim.m_vecSrc = pVictim->GetAbsOrigin();
			radiusInfoVictim.m_flRadius = 1.0f;
			radiusInfoVictim.m_bStockSelfDamage = false;
			radiusInfoVictim.info.SetDamageCustom( TF_DMG_CUSTOM_PUMPKIN_BOMB );

			TFGameRules()->RadiusDamage( radiusInfoVictim );
#endif

			const char *pszEffect = ConstructTeamParticle( "coilgun_boom_%s", pVictim->GetTeamNumber() );
			DispatchParticleEffect( pszEffect, pVictim->WorldSpaceCenter(), vec3_angle );
			DispatchParticleEffect( "ExplosionCore_MidAir", pVictim->WorldSpaceCenter(), vec3_angle );
			EmitSound( "BaseExplosionEffect.Sound" );
		}
	}

	int iCopyVictim = 0;
	CALL_ATTRIB_HOOK_INT( iCopyVictim, mod_onhit_copy_victim );
	if ( iCopyVictim )
	{
		// Prevent switch to Civilian in VIP mode
		if ( pVictim->IsVIP() )
			return;

		Vector oldPos = pTFAttacker->GetAbsOrigin();
		QAngle oldAngles = pTFAttacker->EyeAngles();
		Vector oldVel = pTFAttacker->GetAbsVelocity();
		float oldHP = pTFAttacker->GetHealth();
		float oldMaxHP = pTFAttacker->GetMaxHealth();
		
		pTFAttacker->GetPlayerClass()->Init( pVictim->GetPlayerClass()->GetClassIndex() );

		// Placeholder teleporter effects
		Vector origin = pTFAttacker->GetAbsOrigin();
		CPVSFilter filter( origin );

		int iTeam = pTFAttacker->GetTeamNumber();
		const char *pszEffect = ConstructTeamParticle( "teleported_%s", iTeam );
		DispatchParticleEffect( pszEffect, PATTACH_ABSORIGIN_FOLLOW, pTFAttacker );
		pTFAttacker->EmitSound( "Building_Teleporter.Receive" );

		// Copy items
		CEconItemView *pItem;
		const char *pszClassname;
		for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; ++iSlot )
		{
			CEconEntity *pItemEntity = pVictim->GetEntityForLoadoutSlot( (ETFLoadoutSlot)iSlot );
			if ( pItemEntity )
			{
				pItem = pItemEntity->GetItem();
				if ( pItem )
				{
					pszClassname = pItemEntity->GetClassname();
					if ( !V_stricmp( pszClassname, "no_entity" ) )
						continue;

					// Purge our old items
					pTFAttacker->RemoveAllAmmo();

					CEconEntity *pAttackerItem = pTFAttacker->GetEntityForLoadoutSlot( (ETFLoadoutSlot)iSlot );
					if ( pAttackerItem )
					{
						if ( pAttackerItem->IsBaseCombatWeapon() )
						{
							CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( pAttackerItem );
							pWeapon->UnEquip();
						}
						else if ( pAttackerItem->IsWearable() )
						{
							CEconWearable *pWearable = static_cast<CEconWearable *>( pAttackerItem );
							pTFAttacker->RemoveWearable( pWearable );
						}
						else
						{
							DevWarning( 1, "Player had unknown entity in loadout slot %d.", iSlot );
							UTIL_Remove( pAttackerItem );
						}
					}

					TFPlayerClassData_t *pData = pVictim->GetPlayerClass()->GetData();
					pTFAttacker->ManageBuilderWeapons( pData );

					// Copy weapon and ammo.
					pTFAttacker->GiveNamedItem( pszClassname, 0, pItem );

					for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
					{
						pTFAttacker->SetAmmoCount( pVictim->GetAmmoCount( iAmmo ), iAmmo );
					}

					// Copy Medigun charge.
					if ( !V_stricmp( pszClassname, "tf_weapon_medigun" ) )
					{
						ITFHealingWeapon *pVictimHealingTool = pVictim->GetMedigun();
						ITFHealingWeapon *pAttackerHealingTool = pTFAttacker->GetMedigun();

						// TODO this is a mess, replace it with a healing tool interface

						if ( pVictimHealingTool && pAttackerHealingTool )
						{
							pAttackerHealingTool->AddCharge( pVictimHealingTool->GetChargeLevel() );
						}

					}

					// Copy Spy cloak.
					if ( pVictim->Weapon_OwnsThisID( TF_WEAPON_INVIS ) )
					{
						pTFAttacker->m_Shared.SetSpyCloakMeter( pVictim->m_Shared.GetSpyCloakMeter() );
					}
				}
			}
		}
		
		pTFAttacker->SwitchToNextBestWeapon( NULL );
		pTFAttacker->SetHealth( pTFAttacker->GetPlayerClass()->GetMaxHealth() * oldHP/oldMaxHP );
		pTFAttacker->SetMaxHealth( pTFAttacker->GetPlayerClass()->GetMaxHealth() );
		pTFAttacker->Teleport( &oldPos, &oldAngles, &oldVel );
	}

	float flStunTime = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flStunTime, mod_tranq_onhit );
	if ( flStunTime )
	{
		float flAdjustedTime = flStunTime;
		if (info.GetCritType() == CTakeDamageInfo::CRIT_NONE)
		{
			float flDist = (pTFAttacker->GetAbsOrigin() - pVictim->GetAbsOrigin()).Length();
			flAdjustedTime = RemapValClamped(flDist, 128.0f, 1152.0f, flStunTime, flStunTime * 0.3333f);
		}

		pVictim->m_Shared.AddCond( TF_COND_TRANQUILIZED, flAdjustedTime, pAttacker );
		pVictim->m_Shared.m_aTranqAttackers.AddToHead( pTFAttacker );
#ifdef GAME_DLL
		//TF2C_ACHIEVEMENT_TRANQ_HIT_BLASTJUMP
		if (pVictim->m_Shared.InCond(TF_COND_LAUNCHED) || 
			(pVictim->m_Shared.InCond(TF_COND_BLASTJUMPING))) // Yes, if are blasted by enemy then we aren't ruining player intentional plan
		{
			if (pVictim->GetAbsVelocity().Length2DSqr() > Square(pVictim->MaxSpeed()) && pVictim->IsAirborne(ACHIEVEMENT_TF2C_TRANQ_HIT_BLASTJUMP_HEIGHT))
			pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_TRANQ_HIT_BLASTJUMP);
		}
#endif
	}

	string_t strAddCondOnHit = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( pTFAttacker, strAddCondOnHit, add_onhit_addcond );
	CALL_ATTRIB_HOOK_STRING( strAddCondOnHit, add_onhit_addcond_weapon );
	if ( strAddCondOnHit != NULL_STRING )
	{
		float args[2];
		UTIL_StringToFloatArray( args, 2, strAddCondOnHit.ToCStr() );
		DevMsg( "onhit_addcond: %2.2f, %2.2f\n", args[0], args[1] );

		pVictim->m_Shared.AddCond( (ETFCond)Floor2Int( args[0] ), args[1], pAttacker );
	}

	string_t strAddCondOnHitSelf = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( pTFAttacker, strAddCondOnHitSelf, add_onhit_addcond_self );
	CALL_ATTRIB_HOOK_STRING( strAddCondOnHitSelf, add_onhit_addcond_self_weapon );
	if ( strAddCondOnHitSelf != NULL_STRING )
	{
		float args[2];
		UTIL_StringToFloatArray( args, 2, strAddCondOnHitSelf.ToCStr() );
		DevMsg( "onhit_addcond_self: %2.2f, %2.2f\n", args[0], args[1] );

		pTFAttacker->m_Shared.AddCond( (ETFCond)Floor2Int( args[0] ), args[1], pAttacker );
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		float flAddHealthBurn = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFAttacker, flAddHealthBurn, add_burn_heals );
		if ( flAddHealthBurn )
		{
			if ( flAddHealthBurn > 0 )
			{
				int iHealthRestored = pTFAttacker->TakeHealth( flAddHealthBurn, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
				CTF_GameStats.Event_PlayerHealedOther(pTFAttacker, (float)iHealthRestored);
			}
			else
			{
				CTakeDamageInfo info( pTFAttacker, pTFAttacker, this, -flAddHealthBurn, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
				pTFAttacker->TakeDamage( info );
			}
		}
	}

	// Afterburn shouldn't trigger on-hit effects, neither should disguised spies.
	if ( IsDOTDmg(info) || (pVictim->m_Shared.IsDisguised() && pVictim->m_Shared.DisguiseFoolsTeam(pTFAttacker->GetTeamNumber())))
		return;

	float flAddCharge = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flAddCharge, add_onhit_ubercharge );
	if ( flAddCharge )
	{
		ITFHealingWeapon *pMedigun = pTFAttacker->GetMedigun();
		if ( pMedigun )
		{
			int iAddChargeAffectedByRate = 0;
			CALL_ATTRIB_HOOK_INT(iAddChargeAffectedByRate, add_onhit_ubercharge_charge_rate_modifier);
			if (iAddChargeAffectedByRate)
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pMedigun->GetWeapon(), flAddCharge, mult_medigun_uberchargerate);
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFAttacker, flAddCharge, mult_medigun_uberchargerate_wearer);
			}
			pMedigun->AddCharge( flAddCharge );
		}
	}

	float flUberRateBonusOnHit = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flUberRateBonusOnHit, uber_build_rate_on_hit );
	if ( flUberRateBonusOnHit )
	{
		ITFHealingWeapon *pMedigun = pTFAttacker->GetMedigun();
		if ( pMedigun )
		{
			pMedigun->AddUberRateBonusStack( flUberRateBonusOnHit );
		}
	}


	float flAddHealth = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flAddHealth, add_onhit_addhealth );
	if ( flAddHealth )
	{
		if ( flAddHealth > 0 )
		{
			int iHealthRestored = pTFAttacker->TakeHealth( flAddHealth, HEAL_NOTIFY );
			CTF_GameStats.Event_PlayerHealedOther(pTFAttacker, (float)iHealthRestored);
		}
		else
		{
			CTakeDamageInfo info( pTFAttacker, pTFAttacker, this, -flAddHealth, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
			pTFAttacker->TakeDamage( info );
		}
	}

	float flLifesteal = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flLifesteal, lifesteal );
	if ( flLifesteal )
	{
		float flMaxDamageTaken = pVictim->m_iHealth < info.GetDamage() ? pVictim->m_iHealth : info.GetDamage();

		// minimum health is attribute value, max is weapon damage
		int iHealthToSteal = Max( flLifesteal, flMaxDamageTaken );

		if ( flLifesteal > 0 )
		{
			int iHealthRestored = pTFAttacker->TakeHealth( iHealthToSteal, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
			CTF_GameStats.Event_PlayerHealedOther(pTFAttacker, (float)iHealthRestored);
		}
		else
		{
			CTakeDamageInfo info( pTFAttacker, pTFAttacker, this, -iHealthToSteal, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
			pTFAttacker->TakeDamage( info );
		}
	}

	float flLifestealCrit = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flLifestealCrit, lifesteal_crit );
	if ( flLifestealCrit )
	{
		if( info.GetDamageType() & DMG_CRITICAL )
		{
			float flMaxDamageTaken = pVictim->m_iHealth < info.GetDamage() ? pVictim->m_iHealth : info.GetDamage();

			// minimum health is attribute value, max is weapon damage
			int iHealthToSteal = Max( flLifestealCrit, flMaxDamageTaken );

			if ( flLifestealCrit > 0 )
			{
				int iHealthRestored = pTFAttacker->TakeHealth( iHealthToSteal, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
				CTF_GameStats.Event_PlayerHealedOther(pTFAttacker, (float)iHealthRestored);
			}
			else
			{
				CTakeDamageInfo info( pTFAttacker, pTFAttacker, this, -iHealthToSteal, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
				pTFAttacker->TakeDamage( info );
			}
		}
	}

	if ( pVictim->m_Shared.InCond( TF_COND_BURNING ) )
	{
		float flAddHealthBurning = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( flAddHealthBurning, add_onhit_addhealth_vs_burning );
		if ( flAddHealthBurning )
		{
			if ( flAddHealthBurning > 0 )
			{
				int iHealthRestored = pTFAttacker->TakeHealth( flAddHealthBurning, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
				CTF_GameStats.Event_PlayerHealedOther(pTFAttacker, (float)iHealthRestored);
			}
			else
			{
				CTakeDamageInfo info( pTFAttacker, pTFAttacker, this, -flAddHealthBurning, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
				pTFAttacker->TakeDamage( info );
			}
		}
	}

	float flAddConstruction = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFAttacker, flAddConstruction, add_onhit_construction );
	if ( flAddConstruction )
	{
		int i, c;
		CBaseObject *pObject;
		for ( i = 0, c = pTFAttacker->GetObjectCount(); i < c; i++ )
		{
			pObject = pTFAttacker->GetObject( i );
			if ( pObject && !pObject->HasSapper() && !pObject->IsPlacing() )
			{
				CTFWrench *pWrench = dynamic_cast<CTFWrench *>( pTFAttacker->Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
				if ( pWrench )
				{
					pObject->OnConstructionHit( pTFAttacker, pWrench, pObject->WorldSpaceCenter() );
				}

				bool bRepair = false;
				bool bUpgrade = false;
				
				if ( pObject->GetHealth() < pObject->GetMaxHealth() && !pObject->IsBuilding() )
				{
					float flTargetHeal = info.GetDamage() * flAddConstruction;
					
					//int iAmountToHeal = Min<float>( flTargetHeal, pObject->GetMaxHealth() - RoundFloatToInt( pObject->GetHealth() ) );
					//float flNewHealth = Min( pObject->GetMaxHealth(), pObject->GetHealth() + iAmountToHeal );
					//pObject->SetHealth( flNewHealth );
					pObject->BoltHeal( flTargetHeal, pTFAttacker );
					bRepair = true;
				}

				if ( !bRepair )
				{
					if ( pObject->CanBeUpgraded( pTFAttacker ) )
					{
						pObject->AddUpgradeMetal( pTFAttacker, (int)info.GetDamage() * flAddConstruction );
						bUpgrade = true;
					}
				}

				pObject->DoWrenchHitEffect( pObject->WorldSpaceCenter(), bRepair, bUpgrade );
			}
		}
	}

	int iSentryTargetOverride = 0;
	CALL_ATTRIB_HOOK_INT( iSentryTargetOverride, sentry_target_override_on_hit );
	if( iSentryTargetOverride > 0 && pTFAttacker->GetObjectOfType( OBJ_SENTRYGUN ) )
	{
		CObjectSentrygun *pSentry = static_cast<CObjectSentrygun *>( pTFAttacker->GetObjectOfType(OBJ_SENTRYGUN) );
		pSentry->SetTargetOverride( pVictim );
	}

	int iGainStoredCrit = 0;
	CALL_ATTRIB_HOOK_INT(iGainStoredCrit, gain_stored_crits_on_hit);
	if (iGainStoredCrit)
	{
		pTFAttacker->m_Shared.GainStoredCrits(iGainStoredCrit);
	}

	float flAddCloakOnHit = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT(flAddCloakOnHit, add_cloak_on_hit);
	if (flAddCloakOnHit)
	{
		pTFAttacker->m_Shared.AddSpyCloak(flAddCloakOnHit);
	}


	int iAddAmmoOnHit = 0;
	CALL_ATTRIB_HOOK_INT(iAddAmmoOnHit, add_ammo_on_hit);
	int iAddAmmoOnDirectHit = 0;
	CALL_ATTRIB_HOOK_INT(iAddAmmoOnDirectHit, add_ammo_on_direct_hit);
	if (iAddAmmoOnHit > 0)
	{
		if (iAddAmmoOnHit > 0)
			pTFAttacker->GiveAmmo(iAddAmmoOnHit, m_iPrimaryAmmoType, true);
		else
			pTFAttacker->RemoveAmmo(iAddAmmoOnHit, m_iPrimaryAmmoType);
	}
	if (iAddAmmoOnDirectHit > 0 && (!(info.GetDamageType() & DMG_BLAST) || info.GetInflictor()->GetEnemy() == pVictim))
	{
		if (iAddAmmoOnDirectHit > 0)
			pTFAttacker->GiveAmmo(iAddAmmoOnDirectHit, m_iPrimaryAmmoType, true);
		else
			pTFAttacker->RemoveAmmo(iAddAmmoOnDirectHit, m_iPrimaryAmmoType);
	}

	int iInstantReloadAllWeaponsOnHit = 0;
	CALL_ATTRIB_HOOK_INT(iInstantReloadAllWeaponsOnHit, instant_reload_all_weapons_on_hit);
	int iInstantReloadAllWeaponsOnDirectHit = 0;
	CALL_ATTRIB_HOOK_INT(iInstantReloadAllWeaponsOnDirectHit, instant_reload_all_weapons_on_direct_hit);
	if (iInstantReloadAllWeaponsOnHit > 0)
	{
		for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot <= TF_LOADOUT_SLOT_MELEE; ++iSlot)
		{
			CTFWeaponBase* pSlotWeapon = pTFAttacker->GetWeaponForLoadoutSlot(ETFLoadoutSlot(iSlot));
			if (pSlotWeapon)
			{
				pSlotWeapon->InstantlyReload(pSlotWeapon->GetDefaultClip1());
			}
		}
	}
	if (iInstantReloadAllWeaponsOnDirectHit > 0 && (!(info.GetDamageType() & DMG_BLAST) || info.GetInflictor()->GetEnemy() == pVictim))
	{
		for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot <= TF_LOADOUT_SLOT_MELEE; ++iSlot)
		{
			CTFWeaponBase* pSlotWeapon = pTFAttacker->GetWeaponForLoadoutSlot(ETFLoadoutSlot(iSlot));
			if (pSlotWeapon)
			{
				pSlotWeapon->InstantlyReload(pSlotWeapon->GetDefaultClip1());
			}
		}
	}

	float flOutlineOnHit = 0;
	CALL_ATTRIB_HOOK_FLOAT( flOutlineOnHit, outline_on_hit );
	if( flOutlineOnHit != 0 )
		pVictim->m_Shared.AddCond( TF_COND_MARKED_OUTLINE, flOutlineOnHit );
}

#define TF2C_SCATTERGUN_KNOCKBACK_MINDMG	40
#define TF2C_SCATTERGUN_KNOCKBACK_MAXDIST	384
#define TF2C_JUMP_VELOCITY					268.3281572999747f

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
void CTFWeaponBase::ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer )
{
	CTFPlayer *pVictim = pPlayer;
	if ( !pVictim )
		return;

	CTFPlayer *pAttacker = ToTFPlayer( inputInfo.GetAttacker() );
	if ( !pAttacker )
		return;

	float flDamage = inputInfo.GetDamage();
	float flDist = pAttacker->GetAbsOrigin().DistTo( pVictim->GetAbsOrigin() );

	// Mimic Force-a-Nature's knockback on enemies
	int iScattergunKnockback = 0;
	CALL_ATTRIB_HOOK_INT( iScattergunKnockback, set_scattergun_has_knockback );
	if ( iScattergunKnockback != 0 )
	{
		if ( flDamage < TF2C_SCATTERGUN_KNOCKBACK_MINDMG )
			return;

		if ( flDist > TF2C_SCATTERGUN_KNOCKBACK_MAXDIST )
			return;

		float flKnockback = 500.0f; // Same as airblast
		CALL_ATTRIB_HOOK_FLOAT( flKnockback, scattergun_knockback_mult );

		Vector vForce = pVictim->WorldSpaceCenter() - pAttacker->WorldSpaceCenter();
		vForce = vForce.Normalized();
		// Need some upward force to get off the ground
		vForce.z += TF2C_JUMP_VELOCITY / flKnockback;

		// NDebugOverlay::HorzArrow( pVictim->WorldSpaceCenter(), pVictim->WorldSpaceCenter() + vForce * flKnockback, 8.0f, 255, 255, 255, 255, false, 2.0f );

		pVictim->m_Shared.AirblastPlayer( pAttacker, vForce, flKnockback );
	}
}


void CTFWeaponBase::DisguiseWeaponThink( void )
{
	// Periodically check to make sure we are valid.
	// Disguise weapons are attached to a player, but not managed through the owned weapons list.
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
	if ( !pTFOwner )
	{
		// We must have an owner to be valid.
		Drop( Vector( 0, 0, 0 ) );
		return;
	}

	if ( pTFOwner->m_Shared.GetDisguiseWeapon() != this )
	{
		// The owner's disguise weapon must be us, otherwise we are invalid.
		Drop( Vector( 0, 0, 0 ) );
		return;
	}

	SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5f, "DisguiseWeaponThink" );
}

#ifdef ITEM_TAUNTING

void CTFWeaponBase::UseForTaunt( CEconItemView *pItem )
{
	if ( !pItem )
	{
		m_nTauntModelIndex = -1;
		return;
	}

	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	const char *pszModel = pItemDef->taunt.custom_taunt_prop_per_class[pOwner->GetPlayerClass()->GetClassIndex()];
	m_nTauntModelIndex = modelinfo->GetModelIndex( pszModel );
}
#endif
#else
void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

//=============================================================================
//
// TFWeaponBase functions (Client specific).
//
void CTFWeaponBase::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt )
{
	const char *pszMuzzleFlashModel = GetMuzzleFlashModel();
	const char *pszMuzzleFlashParticleEffect = GetMuzzleFlashParticleEffect();

	if( !tf2c_muzzleflash.GetBool() )
		return;

	// If we have an attachment, then stick a light on it.
	int iMuzzleAttachment = GetTracerAttachment();
	if ( iMuzzleAttachment > 0 && ( pszMuzzleFlashModel || pszMuzzleFlashParticleEffect ) )
	{
		Vector vecOrigin;
		QAngle angAngles;
		pAttachEnt->GetAttachment( iMuzzleAttachment, vecOrigin, angAngles );

		if ( pszMuzzleFlashModel && tf2c_model_muzzleflash.GetBool() )
		{
			float flEffectLifetime = GetMuzzleFlashModelLifetime();

			// Using a model as a muzzle flash.
			if ( m_hMuzzleFlashModel.Get() )
			{
				// Increase the lifetime of the muzzleflash.
				m_hMuzzleFlashModel->SetLifetime( flEffectLifetime );
			}
			else
			{
				m_hMuzzleFlashModel = C_MuzzleFlashModel::CreateMuzzleFlashModel( pszMuzzleFlashModel, pAttachEnt, iMuzzleAttachment, flEffectLifetime );
			}

			m_hMuzzleFlashModel->SetModelScale( GetMuzzleFlashModelScale() );
		}
		
		if ( pszMuzzleFlashParticleEffect && ( !IsMinigun() || GetWeaponID() == TF_WEAPON_AAGUN ) )
		{
			// Don't do the particle effect for minigun since it already has a looping effect.
			DispatchParticleEffect( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, pAttachEnt, iMuzzleAttachment );
		}
	}
}

const char* CTFWeaponBase::GetBeamParticleName(const char* strDefaultName, bool bCritical)
{
	CEconItemDefinition* pItemDef = GetItem()->GetStaticData();

	if (pItemDef)
	{
		EconItemVisuals* pVisuals = pItemDef->GetVisuals(GetTeamNumber());

		if (pVisuals)
		{
			if (bCritical && pVisuals->beam_effect_crit[0])
				return pVisuals->beam_effect_crit;

			else if (pVisuals->beam_effect[0])
				return pVisuals->beam_effect;
		}
	}

	return ConstructTeamParticle(strDefaultName, GetTeamNumber());
}

int	CTFWeaponBase::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	bool bUseInvulnMaterial = ( tf2c_legacy_invulnerability_material.GetBool() && ( pOwner && pOwner->m_Shared.IsInvulnerable() && !pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) ) );
	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int iRet = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldDraw( void )
{
	// We don't have a reason to be visible if we have no owner.
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner || !pOwner->ShouldDrawThisPlayer() )
		return false;

	if ( !BaseClass::ShouldDraw() )
		return false;

	if ( HideWhileStunned() && ( pOwner->m_Shared.IsControlStunned() || pOwner->m_Shared.IsLoser() ) )
		return false;

	if ( pOwner->m_Shared.IsDisguised() )
	{
		// This weapon is part of our disguise, we may want to draw it.
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		int iLocalPlayerTeam = pLocalPlayer ? pLocalPlayer->GetTeamNumber() : TEAM_SPECTATOR;
		if ( iLocalPlayerTeam != pOwner->GetTeamNumber() && iLocalPlayerTeam != TEAM_SPECTATOR )
		{
			// The local player is an enemy, we can see his disguise's weapons.
			if ( pOwner->m_Shared.GetDisguiseWeapon() != this )
				return false;
		}
		// We are friendly, never draw this disguise weapon.
		else if ( m_bDisguiseWeapon )
			return false;
	}
	// We are not disguised, never draw this disguise weapon.
	else if ( m_bDisguiseWeapon )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::UpdateVisibility( void )
{
	BaseClass::UpdateVisibility();

	UpdateExtraWearableVisibility();
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t CTFWeaponBase::ShadowCastType( void )
{
	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
		return SHADOWS_NONE;

	if ( !ShouldDraw() )
		return SHADOWS_NONE;

	C_BaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
		return pOwner->ShadowCastType();

	return SHADOWS_RENDER_TO_TEXTURE;
}

//-----------------------------------------------------------------------------
// Purpose: Use the correct model based on this player's camera.
// ----------------------------------------------------------------------------
int CTFWeaponBase::CalcOverrideModelIndex()
{
	if ( UsingViewModel() )
		return m_iViewModelIndex;

	return GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
		return true;

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	m_szTracerName[0] = '\0';

	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponRegenerate( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bOldResetParity = m_bResetParity;

}

extern ConVar tf2c_invisible_arms;

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		ListenForGameEvent( "localplayer_changeteam" );

		int iInfAmmo = 0;
		CALL_ATTRIB_HOOK_INT( iInfAmmo, energy_weapon_no_ammo );
		m_bInfiniteAmmo = iInfAmmo > 0;
	}

	if ( GetPredictable() && !ShouldPredict() )
	{
		ShutdownPredictable();
	}

	if ( m_bResetParity != m_bOldResetParity )
	{
		WeaponReset();
	}

	// Here we go...
	// Since we can't get a repro for the invisible weapon thing, I'll fix it right up here:
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Our owner is alive, 
	if ( pLocalPlayer && ( pOwner && pOwner->IsAlive() ) )
	{
		// and he is NOT taunting, 
		if ( !pOwner->m_Shared.InCond( TF_COND_TAUNTING ) && WeaponShouldBeVisible() )
		{
			// then why the hell am I NODRAW?
			if ( IsEffectActive( EF_NODRAW ) && ( !pOwner->m_Shared.IsDisguised() || pOwner->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() ? pOwner->GetActiveWeapon() : pOwner->m_Shared.GetDisguiseWeapon() ) == this )
			{
				RemoveEffects( EF_NODRAW );
				UpdateVisibility();
				CreateShadow();
			}
		}
	}
}


void CTFWeaponBase::FireGameEvent( IGameEvent *event )
{
	// If we were the active weapon, we need to update our visibility 
	// because we may switch visibility due to Spy disguises.
	const char *pszEventName = event->GetName();
	if ( !Q_strcmp( pszEventName, "localplayer_changeteam" ) )
	{
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
int CTFWeaponBase::GetWorldModelIndex( void )
{
#ifdef ITEM_TAUNTING
	// See if it's being used for a taunt.
	if ( m_nTauntModelIndex > 0 )
		return m_nTauntModelIndex;
#endif

	return BaseClass::GetWorldModelIndex();
}

bool CTFWeaponBase::ShouldDrawCrosshair( void )
{
	return GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair;
}

void CTFWeaponBase::Redraw()
{
	if ( ShouldDrawCrosshair() && g_pClientMode->ShouldDrawCrosshair() )
	{
		DrawCrosshair();
	}
}
#endif

acttable_t CTFWeaponBase::s_acttablePrimary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PRIMARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PRIMARY, false },
	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_PRIMARY, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PRIMARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PRIMARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PRIMARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PRIMARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PRIMARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PRIMARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PRIMARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PRIMARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PRIMARY, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MP_SWIM_DEPLOYED_PRIMARY, false },
	//{ ACT_MP_DEPLOYED,		ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PRIMARY, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_PRIMARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PRIMARY, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_PRIMARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_PRIMARY, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_PRIMARY_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_PRIMARY, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_PRIMARY_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_PRIMARY, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_PRIMARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_PRIMARY, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_PRIMARY_END, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_PRIMARY_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_PRIMARY_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_PRIMARY_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_PRIMARY_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_PRIMARY_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_PRIMARY_GRENADE2_ATTACK, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PRIMARY, false },
};

acttable_t CTFWeaponBase::s_acttableSecondary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_SECONDARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SECONDARY, false },
	{ ACT_MP_RUN, ACT_MP_RUN_SECONDARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_SECONDARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_SECONDARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_SECONDARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_SECONDARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_SECONDARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_SECONDARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_SECONDARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_SECONDARY, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_SECONDARY, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_SECONDARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_SECONDARY, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_SECONDARY_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_SECONDARY, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_SECONDARY_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_SECONDARY, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_SECONDARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_SECONDARY, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_SECONDARY_END, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_SECONDARY_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_SECONDARY_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_SECONDARY_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_SECONDARY_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_SECONDARY_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_SECONDARY_GRENADE2_ATTACK, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_SECONDARY, false },
};

acttable_t CTFWeaponBase::s_acttableMelee[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_MELEE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_MELEE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE, false },
	{ ACT_MP_WALK, ACT_MP_WALK_MELEE, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_MELEE, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_MELEE, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_MELEE, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_MELEE, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_MELEE, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_MELEE, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_MELEE, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_MELEE, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_MELEE, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_MELEE, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_MELEE_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_MELEE_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_MELEE_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_MELEE_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_MELEE_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_MELEE_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_MELEE, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_MELEE, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_MELEE, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_MELEE, false },
};

acttable_t CTFWeaponBase::s_acttableBuilding[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_BUILDING, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_BUILDING, false },
	{ ACT_MP_RUN, ACT_MP_RUN_BUILDING, false },
	{ ACT_MP_WALK, ACT_MP_WALK_BUILDING, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_BUILDING, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_BUILDING, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_BUILDING, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_BUILDING, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_BUILDING, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_BUILDING, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_BUILDING, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_BUILDING, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_BUILDING, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_BUILDING, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_BUILDING, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_BUILDING, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_BUILDING, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_BUILDING, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_BUILDING, false },
};


acttable_t CTFWeaponBase::s_acttablePDA[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PDA, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PDA, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PDA, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PDA, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PDA, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PDA, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PDA, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PDA, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PDA, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PDA, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PDA, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PDA, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PDA, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PDA, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PDA, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PDA, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PDA, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PDA, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PDA, false },
};

acttable_t CTFWeaponBase::s_acttableItem1[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_ITEM1, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_ITEM1, false },
	{ ACT_MP_RUN, ACT_MP_RUN_ITEM1, false },
	{ ACT_MP_WALK, ACT_MP_WALK_ITEM1, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_ITEM1, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_ITEM1, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_ITEM1, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_ITEM1, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_ITEM1, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_ITEM1, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_ITEM1, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_ITEM1, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_ITEM1, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM1, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_ITEM1, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM1, false },
	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_ITEM1_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_ITEM1, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM1, false },

	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_ITEM1, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MP_DEPLOYED_IDLE_ITEM1, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED_ITEM1, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM1, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_ITEM1, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_ITEM1_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_ITEM1_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_ITEM1_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_ITEM1_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_ITEM1_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_ITEM1_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_ITEM1, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_ITEM1, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_ITEM1, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_ITEM1, false },
};

acttable_t CTFWeaponBase::s_acttableItem2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_ITEM2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_ITEM2, false },
	{ ACT_MP_RUN, ACT_MP_RUN_ITEM2, false },
	{ ACT_MP_WALK, ACT_MP_WALK_ITEM2, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_ITEM2, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_ITEM2, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_ITEM2, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_ITEM2, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_ITEM2, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_ITEM2, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_ITEM2, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_ITEM2, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_ITEM2, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_ITEM2, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_ITEM2, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_ITEM2, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM2, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM2, false },

	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_ITEM2, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MP_DEPLOYED_IDLE_ITEM2, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED_ITEM2, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM2, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_ITEM2_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM2, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_ITEM2, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_ITEM2_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_ITEM2_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_ITEM2_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_ITEM2_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_ITEM2_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_ITEM2_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_ITEM2, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_ITEM2, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_ITEM2, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_ITEM2, false },


	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_ITEM2_END, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_ITEM2_END, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_ITEM2_END, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_ITEM2_END, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM2, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_CROUCH_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_SWIM_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_AIRWALK_GRENADE_ITEM2, false },

};

acttable_t CTFWeaponBase::s_acttableMeleeAllClass[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE_ALLCLASS, false },
	{ ACT_MP_WALK, ACT_MP_WALK_MELEE_ALLCLASS, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_MELEE_ALLCLASS, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_MELEE_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_MELEE_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_MELEE_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_MELEE_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_MELEE_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_MELEE_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_MELEE, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_MELEE, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_MELEE, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_MELEE, false },
};

acttable_t CTFWeaponBase::s_acttableSecondary2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_SECONDARY2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SECONDARY2, false },
	{ ACT_MP_RUN, ACT_MP_RUN_SECONDARY2, false },
	{ ACT_MP_WALK, ACT_MP_WALK_SECONDARY2, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_SECONDARY2, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_SECONDARY2, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_SECONDARY2, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_SECONDARY2, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_SECONDARY2, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_SECONDARY2, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_SECONDARY2, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_SECONDARY2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_SECONDARY2, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_SECONDARY2, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_SECONDARY2, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_SECONDARY2, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_SECONDARY2, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_SECONDARY2, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_SECONDARY2, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_SECONDARY2_END, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_SECONDARY, false },

};

acttable_t CTFWeaponBase::s_acttablePrimary2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PRIMARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PRIMARY, false },
	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_PRIMARY, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PRIMARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PRIMARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PRIMARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PRIMARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PRIMARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PRIMARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PRIMARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PRIMARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PRIMARY, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MP_SWIM_DEPLOYED_PRIMARY, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY, false },
	{ ACT_MP_ATTACK_STAND_PRIMARY_SUPER, ACT_MP_ATTACK_STAND_PRIMARY_SUPER, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER, ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARY_SUPER, ACT_MP_ATTACK_SWIM_PRIMARY_SUPER, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_PRIMARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_PRIMARY_END_ALT, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_PRIMARY_END_ALT, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_PRIMARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_PRIMARY_END_ALT, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PRIMARY, false },
};

viewmodel_acttable_t CTFWeaponBase::s_viewmodelacttable[] =
{
	{ ACT_VM_DRAW, ACT_PRIMARY_VM_DRAW, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_HOLSTER, ACT_PRIMARY_VM_HOLSTER, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_IDLE, ACT_PRIMARY_VM_IDLE, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_PULLBACK, ACT_PRIMARY_VM_PULLBACK, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_PRIMARYATTACK, ACT_PRIMARY_VM_PRIMARYATTACK, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_SECONDARYATTACK, ACT_PRIMARY_VM_SECONDARYATTACK, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_RELOAD, ACT_PRIMARY_VM_RELOAD, TF_WPN_TYPE_PRIMARY },
	{ ACT_RELOAD_START, ACT_PRIMARY_RELOAD_START, TF_WPN_TYPE_PRIMARY },
	{ ACT_RELOAD_FINISH, ACT_PRIMARY_RELOAD_FINISH, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_DRYFIRE, ACT_PRIMARY_VM_DRYFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_PRIMARY_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_IDLE_LOWERED, ACT_PRIMARY_VM_IDLE_LOWERED, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_PRIMARY_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_PRIMARY_ATTACK_STAND_PREFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_PRIMARY_ATTACK_STAND_POSTFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_PRIMARY_ATTACK_STAND_STARTFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_PRIMARY_ATTACK_CROUCH_PREFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_PRIMARY_ATTACK_CROUCH_POSTFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_PRIMARY_ATTACK_SWIM_PREFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_PRIMARY_ATTACK_SWIM_POSTFIRE, TF_WPN_TYPE_PRIMARY },
	{ ACT_VM_DRAW, ACT_SECONDARY_VM_DRAW, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_HOLSTER, ACT_SECONDARY_VM_HOLSTER, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_IDLE, ACT_SECONDARY_VM_IDLE, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_PULLBACK, ACT_SECONDARY_VM_PULLBACK, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_PRIMARYATTACK, ACT_SECONDARY_VM_PRIMARYATTACK, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_SECONDARYATTACK, ACT_SECONDARY_VM_SECONDARYATTACK, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_RELOAD, ACT_SECONDARY_VM_RELOAD, TF_WPN_TYPE_SECONDARY },
	{ ACT_RELOAD_START, ACT_SECONDARY_RELOAD_START, TF_WPN_TYPE_SECONDARY },
	{ ACT_RELOAD_FINISH, ACT_SECONDARY_RELOAD_FINISH, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_DRYFIRE, ACT_SECONDARY_VM_DRYFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_SECONDARY_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_IDLE_LOWERED, ACT_SECONDARY_VM_IDLE_LOWERED, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_SECONDARY_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_SECONDARY_ATTACK_STAND_PREFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_SECONDARY_ATTACK_STAND_POSTFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_SECONDARY_ATTACK_STAND_STARTFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_SECONDARY_ATTACK_CROUCH_PREFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_SECONDARY_ATTACK_CROUCH_POSTFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_SECONDARY_ATTACK_SWIM_PREFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_SECONDARY_ATTACK_SWIM_POSTFIRE, TF_WPN_TYPE_SECONDARY },
	{ ACT_VM_DRAW, ACT_MELEE_VM_DRAW, TF_WPN_TYPE_MELEE },
	{ ACT_VM_HOLSTER, ACT_MELEE_VM_HOLSTER, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE, ACT_MELEE_VM_IDLE, TF_WPN_TYPE_MELEE },
	{ ACT_VM_PULLBACK, ACT_MELEE_VM_PULLBACK, TF_WPN_TYPE_MELEE },
	{ ACT_VM_PRIMARYATTACK, ACT_MELEE_VM_PRIMARYATTACK, TF_WPN_TYPE_MELEE },
	{ ACT_VM_SECONDARYATTACK, ACT_MELEE_VM_SECONDARYATTACK, TF_WPN_TYPE_MELEE },
	{ ACT_VM_RELOAD, ACT_MELEE_VM_RELOAD, TF_WPN_TYPE_MELEE },
	{ ACT_VM_DRYFIRE, ACT_MELEE_VM_DRYFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_MELEE_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE_LOWERED, ACT_MELEE_VM_IDLE_LOWERED, TF_WPN_TYPE_MELEE },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_MELEE_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_MELEE },
	{ ACT_VM_HITCENTER, ACT_MELEE_VM_HITCENTER, TF_WPN_TYPE_MELEE },
	{ ACT_VM_SWINGHARD, ACT_MELEE_VM_SWINGHARD, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_MELEE_ATTACK_STAND_PREFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_MELEE_ATTACK_STAND_POSTFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_MELEE_ATTACK_STAND_STARTFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_MELEE_ATTACK_CROUCH_PREFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_MELEE_ATTACK_CROUCH_POSTFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_MELEE_ATTACK_SWIM_PREFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_MELEE_ATTACK_SWIM_POSTFIRE, TF_WPN_TYPE_MELEE },
	{ ACT_VM_DRAW_SPECIAL, ACT_VM_DRAW_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_HOLSTER_SPECIAL, ACT_VM_HOLSTER_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE_SPECIAL, ACT_VM_IDLE_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_PULLBACK_SPECIAL, ACT_VM_PULLBACK_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_PRIMARYATTACK_SPECIAL, ACT_VM_PRIMARYATTACK_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_SECONDARYATTACK_SPECIAL, ACT_VM_SECONDARYATTACK_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_HITCENTER_SPECIAL, ACT_VM_HITCENTER_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_SWINGHARD_SPECIAL, ACT_VM_SWINGHARD_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE_TO_LOWERED_SPECIAL, ACT_VM_IDLE_TO_LOWERED_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_IDLE_LOWERED_SPECIAL, ACT_VM_IDLE_LOWERED_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_VM_LOWERED_TO_IDLE_SPECIAL, ACT_VM_LOWERED_TO_IDLE_SPECIAL, TF_WPN_TYPE_MELEE },
	{ ACT_BACKSTAB_VM_DOWN, ACT_BACKSTAB_VM_DOWN, TF_WPN_TYPE_MELEE },
	{ ACT_BACKSTAB_VM_UP, ACT_BACKSTAB_VM_UP, TF_WPN_TYPE_MELEE },
	{ ACT_BACKSTAB_VM_IDLE, ACT_BACKSTAB_VM_IDLE, TF_WPN_TYPE_MELEE },
	{ ACT_VM_DRAW, ACT_PDA_VM_DRAW, TF_WPN_TYPE_PDA },
	{ ACT_VM_HOLSTER, ACT_PDA_VM_HOLSTER, TF_WPN_TYPE_PDA },
	{ ACT_VM_IDLE, ACT_PDA_VM_IDLE, TF_WPN_TYPE_PDA },
	{ ACT_VM_PULLBACK, ACT_PDA_VM_PULLBACK, TF_WPN_TYPE_PDA },
	{ ACT_VM_PRIMARYATTACK, ACT_PDA_VM_PRIMARYATTACK, TF_WPN_TYPE_PDA },
	{ ACT_VM_SECONDARYATTACK, ACT_PDA_VM_SECONDARYATTACK, TF_WPN_TYPE_PDA },
	{ ACT_VM_RELOAD, ACT_PDA_VM_RELOAD, TF_WPN_TYPE_PDA },
	{ ACT_VM_DRYFIRE, ACT_PDA_VM_DRYFIRE, TF_WPN_TYPE_PDA },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_PDA_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_PDA },
	{ ACT_VM_IDLE_LOWERED, ACT_PDA_VM_IDLE_LOWERED, TF_WPN_TYPE_PDA },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_PDA_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_PDA },
	{ ACT_VM_DRAW, ACT_ITEM1_VM_DRAW, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_HOLSTER, ACT_ITEM1_VM_HOLSTER, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_IDLE, ACT_ITEM1_VM_IDLE, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_PULLBACK, ACT_ITEM1_VM_PULLBACK, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_PRIMARYATTACK, ACT_ITEM1_VM_PRIMARYATTACK, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_SECONDARYATTACK, ACT_ITEM1_VM_SECONDARYATTACK, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_RELOAD, ACT_ITEM1_VM_RELOAD, TF_WPN_TYPE_ITEM1 },
	{ ACT_RELOAD_START, ACT_ITEM1_RELOAD_START, TF_WPN_TYPE_ITEM1 },
	{ ACT_RELOAD_FINISH, ACT_ITEM1_RELOAD_FINISH, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_DRYFIRE, ACT_ITEM1_VM_DRYFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_ITEM1_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_IDLE_LOWERED, ACT_ITEM1_VM_IDLE_LOWERED, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_ITEM1_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_ITEM1_ATTACK_STAND_PREFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_ITEM1_ATTACK_STAND_POSTFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_ITEM1_ATTACK_STAND_STARTFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_ITEM1_ATTACK_CROUCH_PREFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_ITEM1_ATTACK_CROUCH_POSTFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_ITEM1_ATTACK_SWIM_PREFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_ITEM1_ATTACK_SWIM_POSTFIRE, TF_WPN_TYPE_ITEM1 },
	{ ACT_VM_DRAW, ACT_ITEM2_VM_DRAW, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_HOLSTER, ACT_ITEM2_VM_HOLSTER, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_IDLE, ACT_ITEM2_VM_IDLE, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_PULLBACK, ACT_ITEM2_VM_PULLBACK, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_PRIMARYATTACK, ACT_ITEM2_VM_PRIMARYATTACK, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_SECONDARYATTACK, ACT_ITEM2_VM_SECONDARYATTACK, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_RELOAD, ACT_ITEM2_VM_RELOAD, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_DRYFIRE, ACT_ITEM2_VM_DRYFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_ITEM2_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_IDLE_LOWERED, ACT_ITEM2_VM_IDLE_LOWERED, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_ITEM2_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_STAND_PREFIRE, ACT_ITEM2_ATTACK_STAND_PREFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_STAND_POSTFIRE, ACT_ITEM2_ATTACK_STAND_POSTFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_STAND_STARTFIRE, ACT_ITEM2_ATTACK_STAND_STARTFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE, ACT_ITEM2_ATTACK_CROUCH_PREFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE, ACT_ITEM2_ATTACK_CROUCH_POSTFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_SWIM_PREFIRE, ACT_ITEM2_ATTACK_SWIM_PREFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE, ACT_ITEM2_ATTACK_SWIM_POSTFIRE, TF_WPN_TYPE_ITEM2 },
	{ ACT_VM_DRAW, ACT_MELEE_ALLCLASS_VM_DRAW, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_HOLSTER, ACT_MELEE_ALLCLASS_VM_HOLSTER, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_IDLE, ACT_MELEE_ALLCLASS_VM_IDLE, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_PULLBACK, ACT_MELEE_ALLCLASS_VM_PULLBACK, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_PRIMARYATTACK, ACT_MELEE_ALLCLASS_VM_PRIMARYATTACK, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_SECONDARYATTACK, ACT_MELEE_ALLCLASS_VM_SECONDARYATTACK, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_RELOAD, ACT_MELEE_ALLCLASS_VM_RELOAD, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_DRYFIRE, ACT_MELEE_ALLCLASS_VM_DRYFIRE, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_MELEE_ALLCLASS_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_IDLE_LOWERED, ACT_MELEE_ALLCLASS_VM_IDLE_LOWERED, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_MELEE_ALLCLASS_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_HITCENTER, ACT_MELEE_ALLCLASS_VM_HITCENTER, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_SWINGHARD, ACT_MELEE_ALLCLASS_VM_SWINGHARD, TF_WPN_TYPE_MELEE_ALLCLASS },
	{ ACT_VM_DRAW, ACT_SECONDARY2_VM_DRAW, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_HOLSTER, ACT_SECONDARY2_VM_HOLSTER, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_IDLE, ACT_SECONDARY2_VM_IDLE, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_PULLBACK, ACT_SECONDARY2_VM_PULLBACK, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_PRIMARYATTACK, ACT_SECONDARY2_VM_PRIMARYATTACK, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_RELOAD, ACT_SECONDARY2_VM_RELOAD, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_RELOAD_START, ACT_SECONDARY2_RELOAD_START, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_RELOAD_FINISH, ACT_SECONDARY2_RELOAD_FINISH, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_DRYFIRE, ACT_SECONDARY2_VM_DRYFIRE, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_SECONDARY2_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_IDLE_LOWERED, ACT_SECONDARY2_VM_IDLE_LOWERED, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_SECONDARY2_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_SECONDARY2 },
	{ ACT_VM_DRAW, ACT_PRIMARY_VM_DRAW, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_HOLSTER, ACT_PRIMARY_VM_HOLSTER, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_IDLE, ACT_PRIMARY_VM_IDLE, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_PULLBACK, ACT_PRIMARY_VM_PULLBACK, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_PRIMARYATTACK, ACT_PRIMARY_VM_PRIMARYATTACK, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_RELOAD, ACT_PRIMARY_VM_RELOAD_3, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_RELOAD_START, ACT_PRIMARY_RELOAD_START_3, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_RELOAD_FINISH, ACT_PRIMARY_RELOAD_FINISH_3, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_DRYFIRE, ACT_PRIMARY_VM_DRYFIRE, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_IDLE_TO_LOWERED, ACT_PRIMARY_VM_IDLE_TO_LOWERED, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_IDLE_LOWERED, ACT_PRIMARY_VM_IDLE_LOWERED, TF_WPN_TYPE_PRIMARY2 },
	{ ACT_VM_LOWERED_TO_IDLE, ACT_PRIMARY_VM_LOWERED_TO_IDLE, TF_WPN_TYPE_PRIMARY2 },
};

ConVar mp_forceactivityset( "mp_forceactivityset", "-1", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
ETFWeaponType CTFWeaponBase::GetActivityWeaponRole( void )
{
	ETFWeaponType iWeaponRole = GetTFWpnData().m_iWeaponType;

	if ( HasItemDefinition() )
	{
		ETFWeaponType iSchemaRole = GetItem()->GetAnimationSlot();
		if ( iSchemaRole >= 0 )
		{
			iWeaponRole = iSchemaRole;
		}
	}

	if ( mp_forceactivityset.GetInt() >= 0 )
	{
		iWeaponRole = (ETFWeaponType)mp_forceactivityset.GetInt();
	}

	return iWeaponRole;
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
acttable_t *CTFWeaponBase::ActivityList( int &iActivityCount )
{
	acttable_t *pTable;
	ETFWeaponType iWeaponRole = GetActivityWeaponRole();

#ifdef CLIENT_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->IsEnemyPlayer() )
	{
		CTFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
		if ( pDisguiseWeapon && pDisguiseWeapon != this )
			return pDisguiseWeapon->ActivityList( iActivityCount );
	}
#endif

	switch ( iWeaponRole )
	{
		case TF_WPN_TYPE_PRIMARY:
		default:
			pTable = s_acttablePrimary;
			iActivityCount = ARRAYSIZE( s_acttablePrimary );
			break;
		case TF_WPN_TYPE_SECONDARY:
			pTable = s_acttableSecondary;
			iActivityCount = ARRAYSIZE( s_acttableSecondary );
			break;
		case TF_WPN_TYPE_MELEE:
			pTable = s_acttableMelee;
			iActivityCount = ARRAYSIZE( s_acttableMelee );
			break;
		case TF_WPN_TYPE_BUILDING:
			pTable = s_acttableBuilding;
			iActivityCount = ARRAYSIZE( s_acttableBuilding );
			break;
		case TF_WPN_TYPE_PDA:
			pTable = s_acttablePDA;
			iActivityCount = ARRAYSIZE( s_acttablePDA );
			break;
		case TF_WPN_TYPE_ITEM1:
			pTable = s_acttableItem1;
			iActivityCount = ARRAYSIZE( s_acttableItem1 );
			break;
		case TF_WPN_TYPE_ITEM2:
			pTable = s_acttableItem2;
			iActivityCount = ARRAYSIZE( s_acttableItem2 );
			break;
		case TF_WPN_TYPE_MELEE_ALLCLASS:
			pTable = s_acttableMeleeAllClass;
			iActivityCount = ARRAYSIZE( s_acttableMeleeAllClass );
			break;
		case TF_WPN_TYPE_SECONDARY2:
			pTable = s_acttableSecondary2;
			iActivityCount = ARRAYSIZE( s_acttableSecondary2 );
			break;
		case TF_WPN_TYPE_PRIMARY2:
			pTable = s_acttablePrimary2;
			iActivityCount = ARRAYSIZE( s_acttablePrimary2 );
			break;
	}

	return pTable;
}

//-----------------------------------------------------------------------------
// Purpose: Seems to be mostly used to apply viewmodel animation override from items_game.txt
//-----------------------------------------------------------------------------
int CTFWeaponBase::TranslateViewmodelHandActivity( int iActivity )
{
	if (GetViewModelType() != VMTYPE_NONE)
	{
		ETFWeaponType iWeaponRole = GetTFWpnData().m_iWeaponType;
		if (HasItemDefinition())
		{
			ETFWeaponType iSchemaRole = GetItem()->GetAnimationSlot();
			if (iSchemaRole >= 0)
			{
				iWeaponRole = iSchemaRole;
			}

			Activity actActivityOverride = GetItem()->GetActivityOverride(GetTeamNumber(), (Activity)iActivity);
			if (actActivityOverride != iActivity)
				return actActivityOverride;
		}

		if (GetViewModelType() == VMTYPE_TF2)
		{
			for (int i = 0, c = ARRAYSIZE(s_viewmodelacttable); i < c; i++)
			{
				const viewmodel_acttable_t &act = s_viewmodelacttable[i];
				if (iActivity == act.actBaseAct && iWeaponRole == act.iWeaponRole)
					return act.actTargetAct;
			}
		}
	}

	return iActivity;
}


CBasePlayer *CTFWeaponBase::GetPlayerOwner() const
{
	return ToBasePlayer( GetOwner() );
}


CTFPlayer *CTFWeaponBase::GetTFPlayerOwner() const
{
	return ToTFPlayer( GetOwner() );
}

#ifdef CLIENT_DLL

bool CTFWeaponBase::IsFirstPersonView()
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	return pOwner->InFirstPersonView();
}


bool CTFWeaponBase::UsingViewModel( void )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && !pOwner->ShouldDrawThisPlayer() )
		return true;

	return false;
}


C_BaseAnimating *CTFWeaponBase::GetAppropriateWorldOrViewModel()
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && UsingViewModel() )
	{
		// For w_* models the viewmodel itself is just arms + hands, and attached to them is the actual weapon.
		C_TFViewModel *pViewModel = static_cast<C_TFViewModel *>( pOwner->GetViewModel() );
		if ( pViewModel )
		{
			C_BaseAnimating *pVMAttachment = pViewModel->m_hViewmodelAddon.Get();
			if ( pVMAttachment && GetViewModelType() == VMTYPE_TF2 )
				return pVMAttachment;

			// Nope, it's a standard viewmodel.
			return pViewModel;
		}
	}

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
C_BaseAnimating *CTFWeaponBase::GetWeaponForEffect()
{
	// TODO: Just replace all references to this function with this.
	return GetAppropriateWorldOrViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
void CTFWeaponBase::UpdateExtraWearableVisibility()
{
	if ( m_hExtraWearable.Get() )
	{
		int iShouldHideOnActive = 0;
		CALL_ATTRIB_HOOK_INT(iShouldHideOnActive, hide_extra_wearable_active);
		if (iShouldHideOnActive && GetPlayerOwner() && GetPlayerOwner()->GetActiveWeapon() == this)
			m_hExtraWearable->AddEffects(EF_NODRAW);
		else
			m_hExtraWearable->RemoveEffects(EF_NODRAW);

		m_hExtraWearable->ValidateModelIndex();
		m_hExtraWearable->UpdateVisibility();
		m_hExtraWearable->CreateShadow();

	}

	if ( m_hExtraWearableViewModel.Get() )
	{
		m_hExtraWearableViewModel->UpdateVisibility();
	}
}
#else

void CTFWeaponBase::UpdateExtraWearable()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if (  m_hExtraWearable.Get() || m_hExtraWearableViewModel.Get() )
	{
		const int nTeam = pPlayer->GetTeamNumber();
		if ( m_hExtraWearable.Get() && m_hExtraWearable->GetTeamNumber() == nTeam )
		{
			if ( m_hExtraWearableViewModel.Get() && m_hExtraWearableViewModel->GetTeamNumber() == nTeam )
				return;
		}

		RemoveExtraWearable();
	}

	CEconItemView *pItem = GetItem();
	if ( pItem && pItem->GetExtraWearableModel() && pItem->GetExtraWearableModel()[0] )
	{
		CTFWearable *pExtraWearable = assert_cast<CTFWearable *>( CreateEntityByName( "tf_wearable" ) );
		if ( pExtraWearable )
		{
			pExtraWearable->AddSpawnFlags( SF_NORESPAWN );
			if ( m_bDisguiseWeapon && !pItem->GetExtraWearableModelVisibilityRules() )
			{
				pExtraWearable->SetDisguiseWearable( true );
				pExtraWearable->ChangeTeam( pPlayer->m_Shared.IsDisguised() ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->GetTeamNumber() );
			}
			pExtraWearable->SetWeaponAssociatedWith( this );

			DispatchSpawn( pExtraWearable );

			pExtraWearable->SetModel( pItem->GetExtraWearableModel() );
			pExtraWearable->GiveTo( pPlayer );

			m_hExtraWearable.Set( pExtraWearable );

			pExtraWearable->Equip( pPlayer );
		}
	}
}


void CTFWeaponBase::RemoveExtraWearable( void )
{
	if ( m_hExtraWearable.Get() )
	{
		m_hExtraWearable->RemoveFrom( GetOwnerEntity() );
		m_hExtraWearable = NULL;
	}

	if ( m_hExtraWearableViewModel.Get() )
	{
		m_hExtraWearableViewModel->RemoveFrom( GetOwnerEntity() );
		m_hExtraWearableViewModel = NULL;
	}
}
#endif


bool CTFWeaponBase::UpdateBodygroups( CBasePlayer *pOwner, bool bForce )
{
#ifdef CLIENT_DLL
	// Don't update if my extra wearable and I shouldn't draw.
	if ( !bForce && !ShouldDraw() && !( m_hExtraWearable && m_hExtraWearable->ShouldDraw() ) )
		return false;
#endif

	if ( m_bDisguiseWeapon )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
		if ( !pTFOwner )
			return false;

		CTFPlayer *pDisguiseTarget = ToTFPlayer( pTFOwner->m_Shared.GetDisguiseTarget() );
		if ( !pDisguiseTarget )
			return false;

		CEconItemView *pItem = GetItem();
		if ( !pItem )
			return false;

		CEconItemDefinition *pStatic = pItem->GetStaticData();
		if ( !pStatic )
			return false;

		EconItemVisuals *pVisuals = pStatic->GetVisuals( pTFOwner->m_Shared.GetDisguiseTeam() );
		if ( !pVisuals )
			return false;

		int iDisguiseBody = pTFOwner->m_Shared.GetDisguiseBody();

		const char *pszBodyGroupName;
		int iBodygroup;
		for ( unsigned int i = 0, c = pVisuals->player_bodygroups.Count(); i < c; i++ )
		{
			pszBodyGroupName = pVisuals->player_bodygroups.GetElementName( i );
			if ( pszBodyGroupName )
			{
				iBodygroup = pDisguiseTarget->FindBodygroupByName( pszBodyGroupName );
				if ( iBodygroup == -1 )
					continue;

				::SetBodygroup( pDisguiseTarget->GetModelPtr(), iDisguiseBody, iBodygroup, pVisuals->player_bodygroups.Element( i ) );
			}
		}

		// Set our disguised bodygroups.
		pTFOwner->m_Shared.SetDisguiseBody( iDisguiseBody );
		return true;
	}

	return BaseClass::UpdateBodygroups( pOwner, bForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
bool CTFWeaponBase::CanAttack( void ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
		return pOwner->CanAttack();

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: Apply attack boost. Does not add to existing, similar to how player states are handled.
// ----------------------------------------------------------------------------
void CTFWeaponBase::ApplyWeaponDamageBoostDuration(float flDuration)
{
	m_flDamageBoostTime = Max(float(m_flDamageBoostTime), gpGlobals->curtime + flDuration);
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
bool CTFWeaponBase::IsWeaponDamageBoosted(void)
{
	return m_flDamageBoostTime > gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// ----------------------------------------------------------------------------
bool CTFWeaponBase::IsAmmoInfinite( void )
{
	int iInfiniteReserveAmmo = 0;
	CALL_ATTRIB_HOOK_INT( iInfiniteReserveAmmo, energy_weapon_no_ammo );
	return iInfiniteReserveAmmo > 0;
}



#if defined( CLIENT_DLL )
static ConVar	cl_bobcycle( "cl_bobcycle", "0.8" );
static ConVar	cl_bobup( "cl_bobup", "0.5" );

//-----------------------------------------------------------------------------
// Purpose: Helper function to calculate head bob
//-----------------------------------------------------------------------------
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return 0;

	float	cycle;

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	Vector vecVelocity;
	player->EstimateAbsVelocity( vecVelocity );

	float speed = vecVelocity.Length2D();
	float flmaxSpeedDelta = Max( 0.0f, ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * 320.0f );

	// don't allow too big speed changes
	speed = clamp( speed, pBobState->m_flLastSpeed - flmaxSpeedDelta, pBobState->m_flLastSpeed + flmaxSpeedDelta );
	speed = clamp( speed, -320, 320 );

	pBobState->m_flLastSpeed = speed;

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	pBobState->m_flBobTime += ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * bob_offset;
	pBobState->m_flLastBobTime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = pBobState->m_flBobTime - (int)( pBobState->m_flBobTime / cl_bobcycle.GetFloat() )*cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI_F * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle - cl_bobup.GetFloat() ) / ( 1.0 - cl_bobup.GetFloat() );
	}

	pBobState->m_flVerticalBob = speed*0.005f;
	pBobState->m_flVerticalBob = pBobState->m_flVerticalBob*0.3 + pBobState->m_flVerticalBob*0.7*sin( cycle );

	pBobState->m_flVerticalBob = clamp( pBobState->m_flVerticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = pBobState->m_flBobTime - (int)( pBobState->m_flBobTime / cl_bobcycle.GetFloat() * 2 )*cl_bobcycle.GetFloat() * 2;
	cycle /= cl_bobcycle.GetFloat() * 2;

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI_F * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle - cl_bobup.GetFloat() ) / ( 1.0 - cl_bobup.GetFloat() );
	}

	pBobState->m_flLateralBob = speed*0.005f;
	pBobState->m_flLateralBob = pBobState->m_flLateralBob*0.3 + pBobState->m_flLateralBob*0.7*sin( cycle );
	pBobState->m_flLateralBob = clamp( pBobState->m_flLateralBob, -7.0f, 4.0f );

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add head bob
//-----------------------------------------------------------------------------
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, pBobState->m_flVerticalBob * 0.4f, forward, origin );

	// Z bob a bit more
	origin[2] += pBobState->m_flVerticalBob * 0.1f;

	// bob the angles
	angles[ROLL] += pBobState->m_flVerticalBob * 0.5f;
	angles[PITCH] -= pBobState->m_flVerticalBob * 0.4f;
	angles[YAW] -= pBobState->m_flLateralBob  * 0.3f;

	VectorMA( origin, pBobState->m_flLateralBob * 0.2f, right, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBase::CalcViewmodelBob( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
		return ::CalcViewModelBobHelper( pPlayer, pBobState );
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// Call helper functions to do the calculation.
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
	{
		CalcViewmodelBob();
		::AddViewModelBobHelper( origin, angles, pBobState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the head bob state for this weapon, which is stored
// in the view model. NOTE: This this function can return
// NULL if the player is dead or the view model is otherwise not present.
//-----------------------------------------------------------------------------
BobState_t *CTFWeaponBase::GetBobState()
{
	// Get the view model for this weapon and return it's bob state.
	CTFViewModel *pViewModel = static_cast<CTFViewModel *>( GetPlayerViewModel() );
	Assert( pViewModel );
	return &pViewModel->GetBobState();
}


int CTFWeaponBase::GetSkin( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	return GetTeamSkin( pOwner && pOwner->IsDisguisedEnemy() ? pOwner->m_Shared.GetDisguiseTeam() : GetTeamNumber() );
}


bool CTFWeaponBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == TF_AE_WPN_EJECTBRASS )
	{
		EjectBrass();
		return true;
	}
	// Do nothing.
	else if ( event == AE_WPN_INCREMENTAMMO )
		return true;

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}


void CTFWeaponBase::EjectBrass( void )
{
	C_BaseAnimating *pEffectOwner = GetWeaponForEffect();
	int iBrassAttachment = pEffectOwner->LookupAttachment( "eject_brass" );
	if ( iBrassAttachment <= 0 )
		return;

	CEffectData data;
	pEffectOwner->GetAttachment( iBrassAttachment, data.m_vOrigin, data.m_vAngles );
	data.m_nHitBox = (int)GetWeaponID();
	data.m_nDamageType = GetDamageType();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		data.m_vStart = pPlayer->GetAbsVelocity();
	}

	DispatchEffect( "TF_EjectBrass", data );
}


void CTFWeaponBase::EjectMagazine( const Vector &vecForce )
{
	if ( tf2c_ejectmag_max_count.GetInt() <= 0 )
		return;

	const char *pszMagModel = GetTFWpnData().m_szMagazineModel;
	string_t strCustomMagazineModel = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING( strCustomMagazineModel, custom_magazine_model );
	if ( strCustomMagazineModel != NULL_STRING )
		pszMagModel = STRING(strCustomMagazineModel);

	if ( !pszMagModel[0] )
		return;

	C_BaseAnimating *pEffectOwner = GetWeaponForEffect();
	int iMagAttachment = pEffectOwner->LookupAttachment( "mag_eject" );
	if ( iMagAttachment <= 0 )
		return;

	// Spawn the dropped mag at the attachment.
	Vector vecOrigin, vecVelocity;
	QAngle vecAngles;
	Quaternion angleVel;
	pEffectOwner->GetAttachment( iMagAttachment, vecOrigin, vecAngles );
	pEffectOwner->GetAttachmentVelocity( iMagAttachment, vecVelocity, angleVel );

	if ( vecForce != vec3_origin )
	{
		matrix3x4_t attachmentToWorld;
		pEffectOwner->GetAttachment( iMagAttachment, attachmentToWorld );
		
		Vector vecAbsForce;
		VectorRotate( vecForce, attachmentToWorld, vecAbsForce );
		vecVelocity += vecAbsForce;
	}

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		vecVelocity += pPlayer->GetAbsVelocity();
	}

	C_DroppedMagazine::Create( pszMagModel, vecOrigin, vecAngles, vecVelocity, this );

	// Hide the mag on weapon model.
	int iMagBodygroup = pEffectOwner->FindBodygroupByName( "magazine" );
	if ( iMagBodygroup >= 0 )
	{
		SetBodygroup( iMagBodygroup, 1 );
	}
}


void CTFWeaponBase::UnhideMagazine( void )
{
	C_BaseAnimating *pEffectOwner = GetWeaponForEffect();
	int iMagBodygroup = pEffectOwner->FindBodygroupByName( "magazine" );
	if ( iMagBodygroup >= 0 )
	{
		SetBodygroup( iMagBodygroup, 0 );
	}
}


CStudioHdr *CTFWeaponBase::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if ( m_pModelKeyValues )
	{
		m_pModelKeyValues->deleteThis();
	}

	m_pModelKeyValues = new KeyValues( "ModelKeys" );
	m_pModelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) );
	m_pModelWeaponData = m_pModelKeyValues->FindKey( "tf_weapon_data" );

	return hdr;
}


void CTFWeaponBase::ThirdPersonSwitch( bool bThirdperson )
{
	// Set the model to the correct one.
	int iOverrideModelIndex = CalcOverrideModelIndex();
	if ( iOverrideModelIndex != -1 && iOverrideModelIndex != GetModelIndex() )
	{
		SetModelIndex( iOverrideModelIndex );
	}
}


const Vector &CTFWeaponBase::GetViewmodelOffset()
{
	if ( !m_bInitViewmodelOffset )
	{
		const char *pszMinViewmodelOffset = "0 0 0";
		CALL_ATTRIB_HOOK_STRING( pszMinViewmodelOffset, min_viewmodel_offset );
		UTIL_StringToVector( m_vecViewmodelOffset.Base(), pszMinViewmodelOffset );

		m_bInitViewmodelOffset = true;
	}

	return m_vecViewmodelOffset;
}

//-----------------------------------------------------------------------------
// Purpose: If we are rapid fire weapon and firing stored crit, return true. So we can still shine while firing the last stored crit.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::IsFiringRapidFireStoredCrit()
{
	return m_flCritTime > gpGlobals->curtime && m_bCritTimeIsStoredCrit;
}


//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CWeaponInvisProxy : public CEntityMaterialProxy
{
public:

	CWeaponInvisProxy( void );
	virtual ~CWeaponInvisProxy( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial * GetMaterial();

private:
	IMaterialVar *m_pPercentInvisible;
};


CWeaponInvisProxy::CWeaponInvisProxy( void )
{
	m_pPercentInvisible = NULL;
}


CWeaponInvisProxy::~CWeaponInvisProxy( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input : *pMaterial - 
//-----------------------------------------------------------------------------
bool CWeaponInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var.
	bool bFound;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bFound );

	return bFound;
}

extern ConVar tf_teammate_max_invis;
//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CWeaponInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	if ( !pEnt )
		return;

	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();
	if ( !pMoveParent || !pMoveParent->IsPlayer() )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );
	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}

IMaterial *CWeaponInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}
EXPOSE_INTERFACE( CWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif // CLIENT_DLL

CTFWeaponInfo *GetTFWeaponInfo( ETFWeaponID iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
		return NULL;

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( GetInvalidWeaponInfoHandle() == hWpnInfo )
		return NULL;

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo *>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

CTFWeaponInfo *GetTFWeaponInfoForItem( CEconItemView *pItem, int iClass )
{
	// Get the weapon information.
	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return NULL;

	const char *pszClassname = TranslateWeaponEntForClass( pItemDef->item_class, iClass );
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszClassname );
	if ( GetInvalidWeaponInfoHandle() == hWpnInfo )
		return NULL;

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo *>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

float ITFChargeUpWeapon::GetCurrentCharge( void )
{
	return ( ( gpGlobals->curtime - GetChargeBeginTime() ) / GetChargeMaxTime() );
}

bool CTraceFilterIgnorePlayers::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
	return pEntity && !pEntity->IsPlayer();
}

bool CTraceFilterIgnoreTeammates::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
	if ( pEntity->IsCombatCharacter() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		return false;

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}

bool CTraceFilterIgnoreTeammatesExceptEntity::ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask)
{
	CBaseEntity *pEntity = EntityFromEntityHandle(pServerEntity);
	if (pEntity->IsCombatCharacter() && pEntity->GetTeamNumber() == m_iIgnoreTeam && pEntity != m_pExceptEntity)
		return false;

	return BaseClass::ShouldHitEntity(pServerEntity, contentsMask);
}

bool CTraceFilterIgnoreEnemies::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
	if ( pEntity->IsCombatCharacter() && pEntity->GetTeamNumber() != m_iIgnoreTeam )
		return false;

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}

bool CTraceFilterIgnoreEnemiesExceptSpies::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CTFPlayer *pPlayer = ToTFPlayer( EntityFromEntityHandle( pServerEntity ) );
	if ( pPlayer && !pPlayer->m_Shared.IsDisguised() && pPlayer->GetTeamNumber() != m_iIgnoreTeam )
		return false;
	
	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}
