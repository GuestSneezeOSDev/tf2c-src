//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:			The Medic's Medikit weapon
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"
#include "tf_lagcompensation.h"
#include <filesystem.h>

#if defined( CLIENT_DLL )
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "particles_simple.h"
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "tf_hud_mediccallers.h"
#include "prediction.h"
#else
#include "ndebugoverlay.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "inetchannelinfo.h"
#endif

#include "tf_weapon_medigun.h"

#include <_cpp_stdlib_on.h>
#include <string>
#include <_cpp_stdlib_off.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *g_pszMedigunHealSounds[TF_MEDIGUN_COUNT] =
{
	"WeaponMedigun.Healing",
	"WeaponMedigun.Healing.Kritz",
	//"Weapon_Quick_Fix.Healing",
	//"WeaponMedigun_Vaccinator.Healing",
	//"WeaponMedigun.Healing"
};

const char *g_pszMedigunParticles[TF_MEDIGUN_COUNT] =
{
	"medicgun_beam_%s", // Stock
	"kritz_beam_%s", // Kritzkrieg
	//"medicgun_beam_%s", // Quick-Fix
	//"medicgun_beam_%s", // Vaccinator
	//"overhealer_%s_beam", // Overhealer
};

//#ifndef DEBUGBACKPACKBEAMS
//#define DEBUGBACKPACKBEAMS
//#endif

// Buff ranges
ConVar weapon_medigun_damage_modifier( "tf_weapon_medigun_damage_modifier", "1.5", FCVAR_CHEAT | FCVAR_REPLICATED, "Scales the damage a player does while being healed with the medigun" );
ConVar weapon_medigun_construction_rate( "tf_weapon_medigun_construction_rate", "10", FCVAR_CHEAT | FCVAR_REPLICATED, "Constructing object health healed per second by the medigun" );
ConVar weapon_medigun_charge_rate( "tf_weapon_medigun_charge_rate", "40", FCVAR_CHEAT | FCVAR_REPLICATED, "Amount of time healing it takes to fully charge the medigun" );
ConVar weapon_medigun_chargerelease_rate( "tf_weapon_medigun_chargerelease_rate", "8", FCVAR_CHEAT | FCVAR_REPLICATED, "Amount of time it takes the a full charge of the medigun to be released" );

#if defined( CLIENT_DLL )
void CallbackHealProgressSoundChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	bool bSuccess = false;
	ConVar *cvar = (ConVar *)var;
	if ( UTIL_GetHSM() )
	{
		g_pHealSoundManager->RemoveSound();
		bSuccess = g_pHealSoundManager->TrySelectSoundCollage( cvar->GetString() );
	}

	if ( bSuccess )
		DevMsg( "Successfully set heal progress sound collage to \"%s\". \n", cvar->GetString() );
	else
		DevWarning( "Heal progress sound collage was unable to be set to \"%s\"!\n", cvar->GetString() );
}

ConVar tf_medigun_autoheal( "tf_medigun_autoheal", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will cause the Medigun's primary attack to be a toggle instead of needing to be held down" );
ConVar hud_medicautocallers( "hud_medicautocallers", "0", FCVAR_ARCHIVE );
ConVar hud_medicautocallersthreshold( "hud_medicautocallersthreshold", "75", FCVAR_ARCHIVE );
ConVar hud_medichealtargetmarker( "hud_medichealtargetmarker", "0", FCVAR_ARCHIVE );
ConVar tf2c_medigun_heal_progress_sound("tf2c_medigun_heal_progress_sound_enable", "0", FCVAR_ARCHIVE, "Enables a healing sound that changes pitch based on patient health.");
ConVar tf2c_medigun_heal_progress_sound_name( "tf2c_medigun_heal_progress_sound", "basic", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Selects a particular heal progress collage to use. Collages are loaded from scripts/tf_healsounds.txt", CallbackHealProgressSoundChanged );
ConVar tf2c_medigun_heal_progress_sound_slide_rate("tf2c_medigun_heal_progress_sound_slide_rate", "0.2", FCVAR_ARCHIVE, "How quickly the healing progress sound changes pitch.");
ConVar tf2c_medigun_heal_progress_sound_slide_rate_down( "tf2c_medigun_heal_progress_sound_slide_rate_down", "0.65", FCVAR_ARCHIVE, "How quickly the healing progress sound changes pitch down." );
ConVar tf2c_medigun_heal_progress_sound_volume( "tf2c_medigun_heal_progress_sound_volume", "0.25", FCVAR_ARCHIVE, "The volume multiplier for the heal progress sound." );
ConVar tf2c_medigun_heal_progress_sound_volume_maxhp( "tf2c_medigun_heal_progress_sound_volume_maxp", "0.2", FCVAR_ARCHIVE, "The volume multiplier for the heal progress sound when a patient is at max overheal." );

static void ReloadCollageSchema( const CCommand &args ) {
	bool bSuccess = false;
	if ( UTIL_GetHSM() )
	{
		g_pHealSoundManager->RemoveSound();
		bSuccess = g_pHealSoundManager->ParseCollageSchema( "scripts/tf_healsounds.txt" );
		g_pHealSoundManager->TrySelectSoundCollage( tf2c_medigun_heal_progress_sound_name.GetString() );
	}

	if ( bSuccess )
		DevMsg( "Successfully reloaded the collage schema. \n" );
	else
		DevWarning( "Heal progress sound collage schema was unable to be loaded!\n" );
}
ConCommand tf2c_medigun_heal_progress_sound_reload( "tf2c_medigun_heal_progress_sound_reload", ReloadCollageSchema, "Reloads the sound collage schema used for the medigun heal-progress sounds.", 0 );
#endif

ConVar tf2c_uber_readiness_threshold( "tf2c_uber_readiness_threshold", "0.74", FCVAR_REPLICATED );

#if !defined( CLIENT_DLL )
ConVar tf_medigun_lagcomp( "tf_medigun_lagcomp", "1", FCVAR_DEVELOPMENTONLY );
#endif

extern ConVar tf_invuln_time;

ConVar tf2c_medigun_setup_uber( "tf2c_medigun_setup_uber", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Grants the medigun an increased charge rate during setup" );
ConVar tf2c_medigun_multi_uber_drain( "tf2c_medigun_multi_uber_drain", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Increases the medigun drain rate when charging multiple teammates" );
ConVar tf2c_medigun_critboostable( "tf2c_medigun_critboostable", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Grants the medigun increased charge and heal rate under (mini)crits. 0 = off 1 = minicrit 2 = minicrit + crit" );
ConVar tf2c_medigun_4team_uber_rate( "tf2c_medigun_4team_uber_rate", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "In 4team, medigun charge rate is multiplied by this (except in Arena)" );

ConVar tf2c_uberratestacks_removetime( "tf2c_uberratestacks_removetime", "6", FCVAR_NOTIFY | FCVAR_REPLICATED, "The time after an uber build rate stack is added to remove all stacks from the medigun." );
ConVar tf2c_uberratestacks_max( "tf2c_uberratestacks_max", "8", FCVAR_NOTIFY | FCVAR_REPLICATED, "The maximum number of uber build rate bonus stacks." );


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Healing list UtlVector to entindices
//-----------------------------------------------------------------------------
void SendProxy_BackpackList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CWeaponMedigun *pMedigun = (CWeaponMedigun *)pStruct;

	// If this assertion fails, then SendProxyArrayLength_HealingArray must have failed.
	Assert( iElement < pMedigun->m_vBackpackTargets.Count() );

	EHANDLE hOther = pMedigun->m_vBackpackTargets[iElement].Get();
	SendProxy_EHandleToInt( pProp, pStruct, &hOther, pOut, iElement, objectID );
}

int SendProxyArrayLength_BackpackArray( const void *pStruct, int objectID )
{
	return ((CWeaponMedigun *)pStruct)->m_vBackpackTargets.Count();
}
#else
//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes.
//-----------------------------------------------------------------------------
void RecvProxy_BackpackList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponMedigun *pMedigun = (CWeaponMedigun *)pStruct;

	CBaseHandle *pHandle = (CBaseHandle *)(&pMedigun->m_vBackpackTargets[pData->m_iElement]);
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );

	// Update the heal beams.
	pMedigun->UpdateBackpackTargets();
}

void RecvProxyArrayLength_BackpackArray( void *pStruct, int objectID, int currentArrayLength )
{
	CWeaponMedigun *pMedigun = (CWeaponMedigun *)pStruct;
	if ( pMedigun->m_vBackpackTargets.Size() != currentArrayLength )
	{
		pMedigun->m_vBackpackTargets.SetSize( currentArrayLength );
	}

	// Update the beams.
	pMedigun->UpdateBackpackTargets();
}

#endif

#ifdef CLIENT_DLL

void RecvProxy_HealingTarget( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponMedigun *pMedigun = (CWeaponMedigun *)pStruct;
	if ( pMedigun )
	{
		pMedigun->ForceHealingTargetUpdate();
	}

	RecvProxy_IntToEHandle( pData, pStruct, pOut );
}
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_medigun, CWeaponMedigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_medigun );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMedigun, DT_WeaponMedigun )

BEGIN_NETWORK_TABLE( CWeaponMedigun, DT_WeaponMedigun )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flChargeLevel ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hHealingTarget ) ),
	SendPropBool( SENDINFO( m_bHealing ) ),
	SendPropBool( SENDINFO( m_bAttacking ) ),
	SendPropBool( SENDINFO( m_bChargeRelease ) ),
	SendPropBool( SENDINFO( m_bHolstered ) ),
	SendPropTime( SENDINFO( m_flNextTargetCheckTime ) ),
	SendPropBool( SENDINFO( m_bCanChangeTarget ) ),
	SendPropInt( SENDINFO( m_nUberRateBonusStacks ) ),
	SendPropFloat( SENDINFO( m_flUberRateBonus ) ),
	SendPropInt( SENDINFO( m_iMaxBackpackTargets ) ),
	SendPropBool( SENDINFO( m_bBackpackTargetsParity ) ),
	SendPropArray2(
	SendProxyArrayLength_BackpackArray,
	SendPropInt( "backpack_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_BackpackList ),
	MAX_PLAYERS,
	0,
	"backpack_array"
	),
	SendPropArray3( SENDINFO_ARRAY3( m_bIsBPTargetActive ), SendPropBool( SENDINFO_ARRAY( m_bIsBPTargetActive ) ) ),
#else
	RecvPropFloat( RECVINFO( m_flChargeLevel ) ),
	RecvPropEHandle( RECVINFO( m_hHealingTarget ), RecvProxy_HealingTarget ),
	RecvPropBool( RECVINFO( m_bHealing ) ),
	RecvPropBool( RECVINFO( m_bAttacking ) ),
	RecvPropBool( RECVINFO( m_bChargeRelease ) ),
	RecvPropBool( RECVINFO( m_bHolstered ) ),
	RecvPropTime( RECVINFO( m_flNextTargetCheckTime ) ),
	RecvPropBool( RECVINFO( m_bCanChangeTarget ) ),
	RecvPropInt( RECVINFO( m_nUberRateBonusStacks ) ),
	RecvPropFloat( RECVINFO( m_flUberRateBonus ) ),
	RecvPropInt( RECVINFO( m_iMaxBackpackTargets ) ),
	RecvPropBool( RECVINFO( m_bBackpackTargetsParity ) ),
	RecvPropArray2(
	RecvProxyArrayLength_BackpackArray,
	RecvPropInt( "backpack_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_BackpackList ),
	MAX_PLAYERS,
	0,
	"backpack_array"
	),
	RecvPropArray3( RECVINFO_ARRAY( m_bIsBPTargetActive ), RecvPropBool( RECVINFO( m_bIsBPTargetActive[0] ) ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMedigun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bHealing, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAttacking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bHolstered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hHealingTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_flChargeLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bChargeRelease, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_flNextTargetCheckTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCanChangeTarget, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

//	DEFINE_PRED_FIELD( m_bPlayingSound, FIELD_BOOLEAN ),
//	DEFINE_PRED_FIELD( m_bUpdateHealingTargets, FIELD_BOOLEAN ),
#endif
END_PREDICTION_DATA()


CWeaponMedigun::CWeaponMedigun( void )
{
	WeaponReset();

	m_bHolstered = true;

	SetPredictionEligible( true );

	m_bBackpackTargetsParity = false;

#ifdef CLIENT_DLL
	m_bUpdateBackpackTargets = m_bOldBackpackTargetsParity = false;

#else
	m_iPlayersHealedOld = 1;
#endif
}

CWeaponMedigun::~CWeaponMedigun()
{
#ifdef CLIENT_DLL
	if ( m_pChargedSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
		m_pChargedSound = NULL;
	}
#else
	m_vBackpackTargets.Purge();
	m_vBackpackTargetAddedTime.Purge();
	m_vBackpackTargetRemoveTime.Purge();
#endif
}


void CWeaponMedigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bHealing = false;
	m_bAttacking = false;
	m_bChargeRelease = false;
	m_DetachedTargets.Purge();
	//m_bHolstered = true;	// WeaponReset gets called sometime after the Deploy function on the very first spawn. Moved to constructor.

	m_bCanChangeTarget = true;

	m_flChargeLevel = 0.0f;

	RemoveHealingTarget( true );

	m_vBackpackTargets.RemoveAll();
	m_vBackpackTargets.Purge();
	m_vBackpackTargetAddedTime.RemoveAll();
	m_vBackpackTargetAddedTime.Purge();
	m_vBackpackTargetRemoveTime.RemoveAll();
	m_vBackpackTargetRemoveTime.Purge();

	m_flUberRateBonus = 0.0f;
	m_nUberRateBonusStacks = 0;

	m_iMaxBackpackTargets = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetTFPlayerOwner(), m_iMaxBackpackTargets, backpack_patient_limit );

#if defined( CLIENT_DLL )
	m_flNextBuzzTime = 0;
	m_bPlayingSound = false;
	m_bUpdateHealingTargets = false;
	m_bOldChargeRelease = false;

	UpdateEffects();
	ManageChargeEffect();

	m_pChargeEffect = NULL;
	m_pChargedSound = NULL;

	if ( GetTFPlayerOwner()->IsLocalPlayer() )
	{
		if ( g_pHealSoundManager )
		{
			g_pHealSoundManager->RemoveSound();
			delete g_pHealSoundManager;
			g_pHealSoundManager = NULL;
		}
		g_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );
	}

	memset( m_bPlayerHurt, 0, sizeof( m_bPlayerHurt ) );
#else
	m_iPlayersHealedOld = 1;
#endif
}

void CWeaponMedigun::Precache()
{
	BaseClass::Precache();

#ifdef GAME_DLL
	for ( int i = 0; i < TF_MEDIGUN_COUNT; i++ )
	{
		PrecacheScriptSound( g_pszMedigunHealSounds[i] );

		PrecacheTeamParticles( g_pszMedigunParticles[i] );
		PrecacheTeamParticles( UTIL_VarArgs( "%s_invun", g_pszMedigunParticles[i] ) );
		PrecacheTeamParticles( UTIL_VarArgs( "%s_targeted", g_pszMedigunParticles[i] ) );
		PrecacheTeamParticles( UTIL_VarArgs( "%s_invun_targeted", g_pszMedigunParticles[i] ) );
	}

	// precache custom medibeam particle
	CEconItemDefinition* pItemDef = GetItem()->GetStaticData();

	if (pItemDef)
	{
		EconItemVisuals* pVisuals = pItemDef->GetVisuals(GetTeamNumber());

		// beam_effect itself is precached already in visuals / item schema code
		if (pVisuals && pVisuals->beam_effect && pVisuals->beam_effect[0])
		{
			// cant use VarArgs... https://en.wikipedia.org/wiki/Uncontrolled_format_string
			const char* beam = pVisuals->beam_effect;
			std::string beamParticle = {};

			beamParticle = beam;
			beamParticle.append("_invun");
			PrecacheParticleSystem(beamParticle.c_str());

			beamParticle = beam;
			beamParticle.append("_targeted");
			PrecacheParticleSystem(beamParticle.c_str());

			beamParticle = beam;
			beamParticle.append("_invun_targeted");
			PrecacheParticleSystem(beamParticle.c_str());
		}
	}

	PrecacheScriptSound( "WeaponMedigun.NoTarget" );
	PrecacheScriptSound( "WeaponMedigun.Charged" );
	PrecacheScriptSound( BP_SOUND_HEALON );
	PrecacheScriptSound( BP_SOUND_HEALOFF );

	PrecacheTeamParticles( "medicgun_invulnstatus_fullcharge_%s" );

	PrecacheTeamParticles( "overhealer_%s_beam" );

	// Precache charge sounds.
	for ( int i = 0; i < TF_CHARGE_COUNT; i++ )
	{
		PrecacheScriptSound( g_MedigunEffects[i].sound_enable );
		PrecacheScriptSound( g_MedigunEffects[i].sound_disable );
	}
#else
	if ( g_pHealSoundManager )
	{
		g_pHealSoundManager->RemoveSound();
		delete g_pHealSoundManager;
		g_pHealSoundManager = NULL;
	}
	g_pHealSoundManager = new CTFHealSoundManager( GetTFPlayerOwner(), this );
	g_pHealSoundManager->Precache(this);
#endif
}


bool CWeaponMedigun::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bHolstered = false;

#ifdef GAME_DLL
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( m_bChargeRelease && pOwner )
		{
			pOwner->m_Shared.RecalculateChargeEffects();
		}

		m_iMaxBackpackTargets = 0;

		CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, m_iMaxBackpackTargets, backpack_patient_limit );

		// Need to do this check here so we can remove any active heal targets.
		ApplyBackpackHealing();
#else
		ManageChargeEffect();
#endif

		m_flNextTargetCheckTime = gpGlobals->curtime;

		return true;
	}

	return false;
}


bool CWeaponMedigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bHolstered = true;

#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		pOwner->m_Shared.RecalculateChargeEffects( true );
	}
#endif

#ifdef CLIENT_DLL
	UpdateEffects();
	ManageChargeEffect();
#endif

	return BaseClass::Holster( pSwitchingTo );
}


void CWeaponMedigun::UpdateOnRemove( void )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bChargeRelease = false;
	m_bHolstered = true;

#ifndef CLIENT_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		pOwner->m_Shared.RecalculateChargeEffects( true );
	}
#else

	if ( m_bPlayingSound )
	{
		m_bPlayingSound = false;
		StopHealSound();
	}

	if ( g_pHealSoundManager && GetTFPlayerOwner()->IsLocalPlayer() )
	{
		g_pHealSoundManager->RemoveSound();
	}

	UpdateEffects();
	ManageChargeEffect();
#endif

	BaseClass::UpdateOnRemove();
}


float CWeaponMedigun::GetTargetRange( bool bBackpack /*= false*/ )
{
	//return (float)GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flRange;
	float flRange = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flRange;
	CALL_ATTRIB_HOOK_FLOAT( flRange, mult_weapon_range );
	if ( GetTFPlayerOwner() && bBackpack )
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetTFPlayerOwner(), flRange, mult_backpack_range );
	return flRange;
}


float CWeaponMedigun::GetStickRange( bool bBackpack /*= false*/ )
{
	return (GetTargetRange( bBackpack ) * 1.2f);
}


float CWeaponMedigun::GetHealRate( bool bMultiheal /*= false*/ )
{
	float flHealRate = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flHealRate, mult_medigun_healrate );

		if ( bMultiheal )
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flHealRate, mult_medigun_healrate_multiple_targets );

		// Increase heal rate when under minicrit or crit buffs.
		if ( tf2c_medigun_critboostable.GetBool() )
		{
			if ( pOwner->m_Shared.IsCritBoosted() && tf2c_medigun_critboostable.GetInt() == 2 )
			{
				flHealRate *= 3.0f;
			}
			else if ( pOwner->m_Shared.InCond( TF_COND_DAMAGE_BOOST ) || IsWeaponDamageBoosted() )
			{
				flHealRate *= 1.35f;
			}
		}
		if (pOwner->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
		{
			flHealRate *= TF2C_HASTE_HEALING_FACTOR;
		}
	}

	return flHealRate;
}

#ifdef CLIENT_DLL
void CWeaponMedigun::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldBackpackTargetsParity = m_bBackpackTargetsParity;
}
#endif



int CWeaponMedigun::GetMedigunType( void )
{
	int iType = 0;
	CALL_ATTRIB_HOOK_INT( iType, set_weapon_mode );
	if ( iType >= 0 && iType < TF_MEDIGUN_COUNT )
		return iType;

	AssertMsg( 0, "Invalid medigun type!\n" );
	return TF_MEDIGUN_STOCK;
}


medigun_charge_types CWeaponMedigun::GetChargeType( void )
{
	int iChargeType = TF_CHARGE_INVULNERABLE;
	CALL_ATTRIB_HOOK_INT( iChargeType, set_charge_type );
	if ( iChargeType > TF_CHARGE_NONE && iChargeType < TF_CHARGE_COUNT )
		return (medigun_charge_types)iChargeType;

	AssertMsg( 0, "Invalid charge type!\n" );
	return TF_CHARGE_NONE;
}


const char *CWeaponMedigun::GetHealSound( void )
{
	return g_pszMedigunHealSounds[GetMedigunType()];
}


bool CWeaponMedigun::HealingTarget( CBaseEntity *pTarget )
{
	if ( pTarget == m_hHealingTarget )
		return true;

	return false;
}


bool CWeaponMedigun::AllowedToHealTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	if ( !pTFPlayer )
		return false;

	// We can heal teammates and enemies that are disguised as teammates.
	if ( !pTFPlayer->m_Shared.IsStealthed() &&
		( pTFPlayer->InSameTeam( pOwner ) ||
		( pTFPlayer->m_Shared.DisguiseFoolsTeam( pOwner->GetTeamNumber() ) ) ) )
		return true;

	return false;
}


bool CWeaponMedigun::CouldHealTarget( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() && pTarget->IsAlive() && !HealingTarget( pTarget ) )
		return AllowedToHealTarget( pTarget );

	return false;
}


void CWeaponMedigun::MaintainTargetInSlot()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	CBaseEntity *pTarget = m_hHealingTarget;
	Assert( pTarget );

	// Make sure the guy didn't go out of range.
	bool bLostTarget = true;
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecTargetPoint = pTarget->WorldSpaceCenter();
	Vector vecPoint;

	// If it's brush built, use absmins/absmaxs.
	pTarget->CollisionProp()->CalcNearestPoint( vecSrc, &vecPoint );

	float flDistance = ( vecPoint - vecSrc ).Length();
	if ( flDistance < GetStickRange() )
	{
		if ( m_flNextTargetCheckTime > gpGlobals->curtime )
			return;

		m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;

		trace_t tr;
		CMedigunFilter drainFilter( pOwner );

		Vector vecAiming;
		pOwner->EyeVectors( &vecAiming );

		Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
		UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );

		// Still visible?
		if ( tr.m_pEnt == pTarget )
			return;

		UTIL_TraceLine( vecSrc, vecTargetPoint, MASK_SHOT, &drainFilter, &tr );

		// Still visible?
		if ( tr.fraction == 1.0f || tr.m_pEnt == pTarget )
			return;

		// If we failed, try the target's eye point as well
		UTIL_TraceLine( vecSrc, pTarget->EyePosition(), MASK_SHOT, &drainFilter, &tr );
		if ( tr.fraction == 1.0f || tr.m_pEnt == pTarget )
			return;
	}

	// We've lost this guy...
	if ( bLostTarget )
	{
		RemoveHealingTarget();
	}
}


void CWeaponMedigun::FindNewTargetForSlot()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition();
	if ( m_hHealingTarget )
	{
		RemoveHealingTarget();
	}

	// In Normal mode, we heal players under our crosshair.
	Vector vecAiming;
	pOwner->EyeVectors( &vecAiming );

	// Find a player in range of this player, and make sure they're healable.
	Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecEnd, ( MASK_SHOT & ~CONTENTS_HITBOX ), pOwner, DMG_GENERIC, &tr );
	if ( tr.fraction != 1.0f && tr.m_pEnt )
	{
		if ( CouldHealTarget( tr.m_pEnt ) )
		{
#ifdef GAME_DLL
			pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STARTEDHEALING );
			if ( tr.m_pEnt->IsPlayer() )
			{
				CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
				pTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STARTEDHEALING );
			}

			// Start the heal target thinking.
			SetContextThink( &CWeaponMedigun::HealTargetThink, gpGlobals->curtime, s_pszMedigunHealTargetThink );
#endif

			pOwner->TeamFortress_SetSpeed();
			m_hHealingTarget.Set( tr.m_pEnt );
			m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;
		}			
	}
}

#ifdef GAME_DLL

void CWeaponMedigun::HealTargetThink( void )
{	
	// Verify that we still have a valid heal target.
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( !pTarget || !pTarget->IsAlive() )
	{
		SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
		return;
	}

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	float flTime = gpGlobals->curtime - pOwner->GetTimeBase();
	if ( flTime > 5.0f || !AllowedToHealTarget( pTarget ) || pOwner->IsTaunting() )
	{
		RemoveHealingTarget( false );
	}

	SetNextThink( gpGlobals->curtime + 0.2f, s_pszMedigunHealTargetThink );
}
#endif

void CWeaponMedigun::BuildUberForTarget( CBaseEntity *pNewTarget, bool bMultiTarget /*= false*/)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( pNewTarget );
	if ( !pTFPlayer )
		return;

	if ( !m_bChargeRelease )
	{
		if ( weapon_medigun_charge_rate.GetFloat() )
		{
			int iBoostMax = floor( pTFPlayer->m_Shared.GetMaxBuffedHealth() * 0.95f );
			float flChargeAmount = gpGlobals->frametime / weapon_medigun_charge_rate.GetFloat();

			bool bInSetup = (TFGameRules() && TFGameRules()->InSetup() &&
#ifdef GAME_DLL
				TFGameRules()->GetActiveRoundTimer() &&
#endif
				tf2c_medigun_setup_uber.GetBool());

			if ( pNewTarget->GetHealth() >= pNewTarget->GetMaxHealth() && !bInSetup )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_overheal_uberchargerate );
			}

			CALL_ATTRIB_HOOK_FLOAT( flChargeAmount, mult_medigun_uberchargerate );

			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_uberchargerate_wearer );

			if ( bMultiTarget )
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_uberchargerate_wearer_multiheal );

			if ( bInSetup )
			{
				// Build charge at triple rate during setup.
				flChargeAmount *= 3.0f;
			}
			else if ( pNewTarget->GetHealth() >= iBoostMax )
			{
				// Reduced charge for healing fully healed guys.
				flChargeAmount *= 0.5f;
			}

			// Speed up charge rate when under minicrit or crit buffs.
			if ( tf2c_medigun_critboostable.GetBool() )
			{
				if ( pOwner->m_Shared.IsCritBoosted() && tf2c_medigun_critboostable.GetInt() == 2 )
				{
					flChargeAmount *= 3.0f;
				}
				else if ( pOwner->m_Shared.InCond( TF_COND_DAMAGE_BOOST ) || IsWeaponDamageBoosted() )
				{
					flChargeAmount *= 1.35f;
				}
			}

			if (pOwner->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
			{
				flChargeAmount *= TF2C_HASTE_UBER_FACTOR;
			}

			// In 4team, speed up charge rate to make up for smaller teams and decreased survivability.
			if ( TFGameRules() && TFGameRules()->IsFourTeamGame() && !TFGameRules()->IsInArenaMode() )
			{
				flChargeAmount *= tf2c_medigun_4team_uber_rate.GetFloat();
			}

			// Reduce charge rate when healing someone already being healed.
			int iTotalHealers = pTFPlayer->m_Shared.GetNumHumanHealers();
			if ( !bInSetup && iTotalHealers > 1 )
			{
				flChargeAmount /= (float)iTotalHealers;
			}

			// Build rate bonus stacks
#ifdef GAME_DLL
			CheckAndExpireStacks();
#endif
			flChargeAmount *= 1.0f + GetUberRateBonus();
			//DevMsg( "Charge bonus: %2.4f \n", 1.0f + GetUberRateBonus() );

			float flNewLevel = Min( m_flChargeLevel + flChargeAmount, 1.0f );
			bool bSpeak = (flNewLevel >= GetMinChargeAmount() && m_flChargeLevel < GetMinChargeAmount());
			m_flChargeLevel = flNewLevel;

			if ( bSpeak )
			{
#ifdef GAME_DLL
				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );

				// !!! foxysen uber
				// Will be done later as part of more complete system
				/*if (TFGameRules() && !TFGameRules()->InSetup() && GetNumberOfTeams() == TF_ORIGINAL_TEAM_COUNT)
				{
				for (int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam)
				{
				if (pOwner->GetTeamNumber() == iTeam) continue;

				CTeamRecipientFilter filter(iTeam, true);
				TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_ENEMY_GOT_UBER);
				TFGameRules()->BroadcastSound(iTeam, "Game.EnemyGotUber");
				}
				}*/

				if ( pTFPlayer )
				{
					pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
				}
#endif
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to a healable target
//-----------------------------------------------------------------------------
bool CWeaponMedigun::FindAndHealTargets( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

#ifdef GAME_DLL
	if ( !pOwner->IsBot() )
	{
		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( pOwner->entindex() );
		if ( !pNetChanInfo || pNetChanInfo->IsTimingOut() )
			return false;
	}
#endif // GAME_DLL

	bool bFound = false;

	// Maintaining beam to existing target?
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( pTarget && pTarget->IsAlive() )
	{
		MaintainTargetInSlot();
	}
	else
	{	
		FindNewTargetForSlot();
	}

	CBaseEntity *pNewTarget = m_hHealingTarget;
	if ( pNewTarget && pNewTarget->IsAlive() )
	{
		bFound = true;

		// HACK: For now, just deal with players.
		CTFPlayer *pTFPlayer = ToTFPlayer( pNewTarget );
		if ( pTFPlayer )
		{
#ifdef GAME_DLL
			if ( pTarget != pNewTarget && pNewTarget->IsPlayer() )
			{
				pTFPlayer->m_Shared.Heal( pOwner, GetHealRate() );
			}

			pTFPlayer->m_Shared.RecalculateChargeEffects( false );
#else
            // UNDONE: CRASH! https://github.com/tf2classic/tf2classic-issue-tracker/issues/272
            // I dont know why and i dont really want to know why, but somewhere, somehow, a full update whacks some logic,
            // and some cached value ends up being wrong for some variable relating to m_pOuter down in GetMaxBuffedHealth and
            // sometimes trickling down into the attrib functions
#if FIX_HEALSOUND_MGR_FIRST
            if (g_pHealSoundManager && GetTFPlayerOwner()->IsLocalPlayer())
            {
                g_pHealSoundManager->SetLastPatientHealth(pTFPlayer->GetHealth());
            }
#endif


#endif
	
			// Charge up our power if we're not releasing it, and our target
			// isn't receiving any benefit from our healing.
			// BuildUberForTarget( pNewTarget, false );
		}
	}

	return bFound;
}


void CWeaponMedigun::AddCharge( float flAmount )
{
	float flChargeRate = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT(flChargeRate, mult_medigun_uberchargerate);
	CALL_ATTRIB_HOOK_FLOAT(flChargeRate, mult_medigun_uberchargerate_wearer);
	if ( !flChargeRate ) // Can't earn uber.
		return;

	float flNewLevel = Min( m_flChargeLevel + flAmount, 1.0f );
	flNewLevel = Max( flNewLevel, 0.0f );

#ifdef GAME_DLL
	bool bSpeak = !m_bChargeRelease && (flNewLevel >= 1.0f && m_flChargeLevel < 1.0f);
#endif
	m_flChargeLevel = flNewLevel;

#ifdef GAME_DLL
	if ( bSpeak )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );

			// !!! foxysen uber
			// Will be done later as part of more complete system
			/*if (TFGameRules() && !TFGameRules()->InSetup() && GetNumberOfTeams() == TF_ORIGINAL_TEAM_COUNT)
			{
				for (int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam)
				{
					if (pPlayer->GetTeamNumber() == iTeam) continue;

					CTeamRecipientFilter filter(iTeam, true);
					TFGameRules()->SendHudNotification(filter, HUD_NOTIFY_ENEMY_GOT_UBER);
					TFGameRules()->BroadcastSound(iTeam, "Game.EnemyGotUber");
				}
			}*/
		}

		pPlayer = ToTFPlayer( m_hHealingTarget );
		if ( pPlayer )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
		}
	}
#endif
}


void CWeaponMedigun::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();
#ifdef GAME_DLL
	ApplyBackpackHealing();
#endif

#ifdef CLIENT_DLL
	if(GetTFPlayerOwner()->IsLocalPlayer())
		UTIL_HSMSetPlayerThink( NULL );
#endif

	DrainCharge();
}


#ifdef GAME_DLL

void CWeaponMedigun::UpdateBackpackMaxTargets( void )
{
	m_iMaxBackpackTargets = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetTFPlayerOwner(), m_iMaxBackpackTargets, backpack_patient_limit );
}


bool CWeaponMedigun::IsBackpackPatient( CTFPlayer *pPlayer )
{
	return m_vBackpackTargets.HasElement( pPlayer );
}

void CWeaponMedigun::AddBackpackPatient( CTFPlayer *pPlayer )
{
	if ( !m_iMaxBackpackTargets )
		return;

	// Don't re-add the same player, just refresh their timer.
	if ( m_vBackpackTargets.HasElement( pPlayer ) )
	{
		int iPlayer = m_vBackpackTargets.Find( pPlayer );
		m_vBackpackTargetRemoveTime[iPlayer] = gpGlobals->curtime + BP_REMOVE_TIME;
		m_vBackpackTargetAddedTime[iPlayer] = gpGlobals->curtime;
		return;
	}

	// Stick us on the end...
	m_vBackpackTargets.AddToTail( pPlayer );
	m_vBackpackTargetRemoveTime.AddToTail( gpGlobals->curtime + BP_REMOVE_TIME );
	m_vBackpackTargetAddedTime.AddToTail( gpGlobals->curtime );		// Keep track of the time we were added so we know the queue order

	int iAmountOverCapacity = Max( m_vBackpackTargets.Count() - m_iMaxBackpackTargets, 0 );

	// We're over capacity! Start removing people based on their age.
	while ( iAmountOverCapacity > 0 )
	{
		float flMinimum = FLT_MAX;
		int iMinimumIndex = 0;
		FOR_EACH_VEC( m_vBackpackTargets, i )
		{
			if ( m_vBackpackTargetAddedTime[i] < flMinimum )
			{
				flMinimum = m_vBackpackTargetAddedTime[i];
				iMinimumIndex = i;
			}

		}
		RemoveBackpackPatient( iMinimumIndex );

		iAmountOverCapacity--;
	}

	Assert( m_vBackpackTargets.Count() == m_vBackpackTargetAddedTime.Count() == m_vBackpackTargetRemoveTime.Count() );

	UpdateBackpackTargets();
}

void CWeaponMedigun::RemoveBackpackPatient( int iIndex )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_vBackpackTargets[iIndex] ), *pOwner = GetTFPlayerOwner();

	pPlayer->m_Shared.StopHealing( pOwner, HEALER_TYPE_BEAM );
	pPlayer->m_Shared.RecalculateChargeEffects( true );

	m_vBackpackTargets.Remove( iIndex );
	m_vBackpackTargetRemoveTime.Remove( iIndex );
	m_vBackpackTargetAddedTime.Remove( iIndex );

	UpdateBackpackTargets();
	// Particle effects are now orphaned and should delete themselves.
}

//-----------------------------------------------------------------------------
// Purpose: Serverside think logic for backpack healing beams.
//-----------------------------------------------------------------------------
void CWeaponMedigun::ApplyBackpackHealing( void )
{
	if ( !m_iMaxBackpackTargets ) {
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( !pOwner )
		return;

	//CUtlVector<EHANDLE> hOldBackpackTargetsActive;
	//hOldBackpackTargetsActive.CopyArray( m_vBackpackTargets.Base(), m_vBackpackTargets.Count() );

	bool bNeedsUpdate = false;
	if ( pOwner->IsAlive() )
	{
		int iPlayersHealed = 0;

		int j = 0;
		FOR_EACH_VEC( m_vBackpackTargets, iter )
		{
			bool bActive = false;

			float flRange2 = m_bIsBPTargetActive[j] ? GetStickRange( true ) : GetTargetRange( true );
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flRange2, mult_weapon_range );
			flRange2 *= flRange2;

			Vector vecSegment;
			Vector vecTargetPoint;
			float flDist2 = 0;

			CTFPlayer *pPlayer = ToTFPlayer( m_vBackpackTargets.Element( iter ) );
			if ( !pPlayer )
				goto skipandcontinue;

			// Only do the AoE heal if this Medigun is holstered.
			if ( !m_bHolstered )
				goto skipandcontinue;

			if ( !pPlayer->IsAlive() )
				goto skipandcontinue;

			if ( pPlayer->GetFlags() & FL_NOTARGET )
				goto skipandcontinue;

			if ( pPlayer->m_Shared.IsStealthed() )	// Don't out any invisible spies, that's rude!
				goto skipandcontinue;

			if ( gpGlobals->curtime > m_vBackpackTargetRemoveTime[j] ) {
				RemoveBackpackPatient( j );
				DevMsg( "Removing BP target due to expiry date\n" );
				goto skipandcontinue;
			}

			// Enemies aren't allowed to be healed, but they can be if they're a disguised spy with a disguise that works on us.
			if ( pPlayer->IsEnemy( pOwner ) && !(pPlayer->m_Shared.IsDisguised() &&
				pPlayer->m_Shared.DisguiseFoolsTeam( pOwner->GetTeamNumber() )) )
			{
				goto skipandcontinue;
			}

			vecTargetPoint = pPlayer->GetAbsOrigin();
			vecTargetPoint += pPlayer->GetViewOffset();
			VectorSubtract( vecTargetPoint, GetAbsOrigin(), vecSegment );
			flDist2 = vecSegment.LengthSqr();
			if ( flDist2 <= flRange2 )
			{
				trace_t	tr;
				UTIL_TraceLine( GetAbsOrigin(), vecTargetPoint, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
				if ( tr.fraction >= 1.0f )
				{
					// Remove this target after however many seconds
					m_vBackpackTargetRemoveTime[j] = gpGlobals->curtime + BP_REMOVE_TIME;
					//DevMsg( "Resetting remove time for BP target to %2.2f\n ", m_vBackpackTargetRemoveTime [j] );

					// If we are already not healing this guy, start healing
					if ( !m_bIsBPTargetActive.Get( j ) )
					{
						int iNoOverheal = 0;
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, iNoOverheal, backpack_no_overheal );

						if(iNoOverheal > 0)
							pPlayer->m_Shared.HealNoOverheal( pOwner, GetHealRate( true ) / Max( m_iPlayersHealedOld, 0 ) );
						else
							pPlayer->m_Shared.Heal( pOwner, GetHealRate( true ) / Max( m_iPlayersHealedOld, 0 ) );

#ifdef DEBUGBACKPACKBEAMS
						DevMsg("Started healing element %i. \n", j);
#endif

						bNeedsUpdate = true;
					}
					bActive = true;
					BuildUberForTarget( pPlayer, true );
				}
			}

		skipandcontinue:
			if ( pPlayer )
			{
				// The "active" state has changed, so update the network var and stop healing.
				if ( m_bIsBPTargetActive.Get( j ) != bActive )
				{
					m_bIsBPTargetActive.Set( j, bActive );
					bNeedsUpdate = true;
#ifdef DEBUGBACKPACKBEAMS
					DevMsg( "Active state changed for element %i. \n", j );
#endif

					// Becoming "active" is already handled above, so handle becoming "inactive" here:
					if ( !bActive )
					{
						if ( pOwner )
							pPlayer->m_Shared.StopHealing( pOwner, HEALER_TYPE_BEAM );

#ifdef DEBUGBACKPACKBEAMS
						DevMsg( "Element %i became inactive; stopping healing.\n", j );
#endif
						pPlayer->m_Shared.RecalculateChargeEffects( true );
					}
				}
			}

			// might need a label here to skip to if we remove a patient?

			j++;
		}

		m_iPlayersHealedOld = iPlayersHealed;
	}

	if ( bNeedsUpdate )
	{
		UpdateBackpackTargets();
#ifdef DEBUGBACKPACKBEAMS
		DevMsg( "Something updated! Sending to client.\n" );
#endif
	}
#ifdef DEBUGBACKPACKBEAMS
	DevMsg( "ApplyBackpackHealing done! -----------------------------\n" );
#endif
}
#endif

void CWeaponMedigun::DrainCharge( void )
{
	if ( !m_bChargeRelease )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	// If we're in charge release mode, drain our charge.
	float flUberTime = weapon_medigun_chargerelease_rate.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pOwner, flUberTime, add_uber_time);
	CALL_ATTRIB_HOOK_FLOAT(flUberTime, add_uber_time_active);

	float flChargeAmount = gpGlobals->frametime / flUberTime;

	if ( tf2c_medigun_multi_uber_drain.GetBool() && GetMedigunType() != TF_CHARGE_CRITBOOSTED )
	{
		float flExtraPlayerCost = flChargeAmount * 0.5f;

		// Drain faster the more targets we're applying to.
		for ( int i = m_DetachedTargets.Count() - 1; i >= 0; i-- )
		{
			if ( !m_DetachedTargets[i].hTarget || m_DetachedTargets[i].hTarget.Get() == m_hHealingTarget.Get() || 
				!m_DetachedTargets[i].hTarget->IsAlive() || m_DetachedTargets[i].flTime < ( gpGlobals->curtime - tf_invuln_time.GetFloat() ) )
			{
				m_DetachedTargets.Remove( i );
			}
			else
			{
				flChargeAmount += flExtraPlayerCost;
			}
		}
	}

	m_flChargeLevel = Max( m_flChargeLevel - flChargeAmount, 0.0f );
	if ( !m_flChargeLevel )
	{
		m_bChargeRelease = false;
		m_DetachedTargets.Purge();

#ifdef GAME_DLL
		/*if ( m_bHealingSelf )
		{
			m_bHealingSelf = false;
			pOwner->m_Shared.StopHealing( pOwner );
		}*/

		pOwner->m_Shared.RecalculateChargeEffects();
#endif
	}
}

void CWeaponMedigun::AddUberRateBonusStack( float flBonus, int nStack /*= 1*/ )
{
#ifdef GAME_DLL
	CheckAndExpireStacks();
#endif

	if ( m_nUberRateBonusStacks < tf2c_uberratestacks_max.GetInt() ) {
		m_flUberRateBonus += flBonus * nStack;
		m_nUberRateBonusStacks += nStack;
		//DevMsg( "Added charge bonus: %2.4f (%2.4f total) \n", flBonus, m_flUberRateBonus );
		//DevMsg( "Stacks: %i (%i total) \n", nStack, m_nUberRateBonusStacks );
	}

#ifdef GAME_DLL
	m_flStacksRemoveTime = gpGlobals->curtime + tf2c_uberratestacks_removetime.GetFloat();
#endif
}

#ifdef GAME_DLL
void CWeaponMedigun::CheckAndExpireStacks()
{
	if ( m_nUberRateBonusStacks )
	{
		if ( gpGlobals->curtime > m_flStacksRemoveTime )
		{
			//DevMsg( "Charge bonus stacks removed! \n" );
			m_nUberRateBonusStacks = 0;
			m_flUberRateBonus = 0.0f;
		}
	}
}
#endif

int	CWeaponMedigun::GetUberRateBonusStacks( void ) const
{
	return m_nUberRateBonusStacks; 
}

float	CWeaponMedigun::GetUberRateBonus( void ) const
{
	return m_flUberRateBonus / 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the hold-down healing
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemPostFrame( void )
{
#ifdef GAME_DLL
	CheckAndExpireStacks();
#endif
	// If we're lowered, we're not allowed to fire.
	if ( !CanAttack() )
	{
		RemoveHealingTarget( true );
		WeaponIdle();
		return;
	}

	// The check above will return false if the owner is NULL, so we can safely assume here.
	CTFPlayer *pOwner = assert_cast<CTFPlayer *>( GetOwner() );

#if !defined( CLIENT_DLL )
	if ( AppliesModifier() )
	{
		m_DamageModifier.SetModifier( weapon_medigun_damage_modifier.GetFloat() );
	}
#endif

	// Try to start healing.
	m_bAttacking = false;
	if ( pOwner->GetMedigunAutoHeal() )
	{
		if (m_bHealing || (pOwner->m_nButtons & IN_ATTACK))
		{
			PrimaryAttack();
			m_bAttacking = true;
		}

#if defined( CLIENT_DLL )
		if (m_hHealingTarget && !m_hHealingTargetEffect.pTarget)
		{
			UpdateEffects();
		}
#endif

		if ( pOwner->m_nButtons & IN_ATTACK )
		{
			if ( m_bCanChangeTarget )
			{
#if defined( CLIENT_DLL )
				m_bPlayingSound = false;
				StopHealSound();
#endif
				RemoveHealingTarget();
				// Can't change again until we release the attack button.
				m_bCanChangeTarget = false;
			}
		}
		else
		{
			m_bCanChangeTarget = true;
		}

	}
	else
	{
		if ( /*m_bChargeRelease ||*/ pOwner->m_nButtons & IN_ATTACK )
		{
			PrimaryAttack();
			m_bAttacking = true;
		}
 		else if ( m_bHealing )
 		{
 			// Detach from the player if they release the attack button.
 			RemoveHealingTarget();
 		}
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		SecondaryAttack();
	}

#ifdef CLIENT_DLL
	if ( GetTFPlayerOwner()->IsLocalPlayer() && prediction->IsFirstTimePredicted())
		UTIL_HSMSetPlayerThink( ToTFPlayer( m_hHealingTarget ) );
#endif

	WeaponIdle();
}


bool CWeaponMedigun::Lower( void )
{
	// Stop healing if we are.
	if ( m_bHealing )
	{
		RemoveHealingTarget( true );
		m_bAttacking = false;

#ifdef CLIENT_DLL
		UpdateEffects();
#endif
	}

	return BaseClass::Lower();
}


void CWeaponMedigun::RemoveHealingTarget( bool bSilent )
{
	// If this person is already in our detached target list, update the time, otherwise add them in.
	if ( m_bChargeRelease )
	{
		if ( tf2c_medigun_multi_uber_drain.GetBool() && GetMedigunType() != TF_CHARGE_CRITBOOSTED )
		{
			int i, c;
			for ( i = 0, c = m_DetachedTargets.Count(); i < c; i++ )
			{
				// might be a bug if the Get()s somehow don't match... -sappho
				// if ( m_DetachedTargets[i].hTarget == m_hHealingTarget )
				if ( m_DetachedTargets[i].hTarget.Get() == m_hHealingTarget.Get() )
				{
					m_DetachedTargets[i].flTime = gpGlobals->curtime;
					break;
				}
			}

			if ( i == c )
			{
				int iIdx = m_DetachedTargets.AddToTail();
				m_DetachedTargets[iIdx].hTarget = m_hHealingTarget;
				m_DetachedTargets[iIdx].flTime = gpGlobals->curtime;
			}
		}
	}

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

#ifdef GAME_DLL
	if ( m_hHealingTarget )
	{
		// HACK: For now, just deal with players.
		if ( m_hHealingTarget->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
			pTFPlayer->m_Shared.StopHealing( pOwner, HEALER_TYPE_BEAM );
			pTFPlayer->m_Shared.RecalculateChargeEffects( false );

			if ( !bSilent )
			{
				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STOPPEDHEALING, pTFPlayer->IsAlive() ? "healtarget:alive" : "healtarget:dead" );
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STOPPEDHEALING );
			}
		}
	}

	// Stop thinking - we no longer have a heal target.
	SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
#endif

	pOwner->TeamFortress_SetSpeed();
	m_hHealingTarget.Set( NULL );

	// Stop the welding animation
	if ( m_bHealing && !bSilent )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
	}

#ifndef CLIENT_DLL
	m_DamageModifier.RemoveModifier();
#else
	UpdateEffects();
#endif

	m_bHealing = false;

}

//-----------------------------------------------------------------------------
// Purpose: Attempt to heal any player within range of the medikit
//-----------------------------------------------------------------------------
void CWeaponMedigun::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	// The check above will return false if the owner is NULL, so we can safely assume here.
	CTFPlayer *pOwner = assert_cast<CTFPlayer *>( GetOwner() );

#ifdef GAME_DLL
	/*// Start boosting ourself if we're not
	if ( m_bChargeRelease && !m_bHealingSelf )
	{
		pOwner->m_Shared.Heal( pOwner, GetHealRate() * 2 );
		m_bHealingSelf = true;
	}*/
#endif

	START_LAG_COMPENSATION_CONDITIONAL( pOwner, pOwner->GetCurrentCommand(), tf_medigun_lagcomp.GetBool() );

	if ( FindAndHealTargets() )
	{
		// Start the animation.
		if ( !m_bHealing )
		{
#ifdef GAME_DLL
			pOwner->SpeakWeaponFire();
#endif

			SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
		}

		m_bHealing = true;
	}
	else
	{
		RemoveHealingTarget();
	}

	FINISH_LAG_COMPENSATION();
}

//-----------------------------------------------------------------------------
// Purpose: Burn charge level to generate invulnerability
//-----------------------------------------------------------------------------
void CWeaponMedigun::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

	// Ensure they have a full charge and are not already in charge release mode.
	if ( m_flChargeLevel < GetMinChargeAmount() || m_bChargeRelease )
	{
#ifdef CLIENT_DLL
		// THIS DOES NOTHING WHY IS THIS HEEEERE
		/*// Deny, flash.
		if ( !m_bChargeRelease && m_flFlashCharge <= 0.0f )
		{
			m_flFlashCharge = 10;
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
		}
		*/
		if ( gpGlobals->curtime >= m_flNextBuzzTime )
		{
			EmitSound( "Player.DenyWeaponSelection" );
			m_flNextBuzzTime = gpGlobals->curtime + 0.5f; // Only buzz every so often.
		}
#endif
		return;
	}

	// Start super charge.
	m_bChargeRelease = true;

#ifdef GAME_DLL
	// The check above will return false if the owner is NULL, so we can safely assume here.
	CTFPlayer *pOwner = assert_cast<CTFPlayer *>(GetOwner());

	// Only increment the invuln stat when we give them god mode.
	/*if ( GetChargeType() == TF_CHARGE_INVULNERABLE )
	{
		CTF_GameStats.Event_PlayerInvulnerable( pOwner );
	}*/
	// Renamed invuln to ubers outside code, Medics of all mediguns want to get their scores for being good at ubering.
	CTF_GameStats.Event_PlayerInvulnerable(pOwner);

	pOwner->m_Shared.RecalculateChargeEffects();

	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEDEPLOYED );

	if ( m_hHealingTarget && m_hHealingTarget->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
		pTFPlayer->m_Shared.RecalculateChargeEffects();
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_chargedeployed" );
	if ( event )
	{
		event->SetInt( "userid", pOwner->GetUserID() );

		gameeventmanager->FireEvent( event );	// Why not send to clients?
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Idle tests to see if we're facing a valid target for the medikit
//			If so, move into the "heal-able" animation. 
//			Otherwise, move into the "not-heal-able" animation.
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		// Loop the welding animation.
		if ( m_bHealing )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			return;
		}

		return BaseClass::WeaponIdle();
	}
}

#if defined( CLIENT_DLL )

void CWeaponMedigun::StopHealSound( bool bStopHealingSound, bool bStopNoTargetSound )
{
	if ( bStopHealingSound )
	{
		StopSound( GetHealSound() );
		StopSound( "WeaponMedigun.HealingStart" );
	}

	if ( bStopNoTargetSound )
	{
		StopSound( "WeaponMedigun.NoTarget" );
	}
}


void CWeaponMedigun::ManageChargeEffect( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	bool bOwnerTaunting = ( pOwner && pOwner->m_Shared.InCond( TF_COND_TAUNTING ) );
	if ( pOwner && !bOwnerTaunting && !m_bHolstered && ( m_flChargeLevel >= GetMinChargeAmount() || m_bChargeRelease ) )
	{
		if ( !m_pChargeEffect )
		{
			C_BaseEntity *pEffectOwner = GetWeaponForEffect();
			if ( pEffectOwner )
			{
				const char *pszEffectName = ConstructTeamParticle( "medicgun_invulnstatus_fullcharge_%s", pOwner->GetTeamNumber() );
				m_pChargeEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
				m_hChargeEffectHost = pEffectOwner;
			}
		}

		if ( !m_pChargedSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

			CLocalPlayerFilter filter;
			m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun.Charged" );
			controller.Play( m_pChargedSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pChargeEffect )
		{
			C_BaseEntity *pEffectOwner = m_hChargeEffectHost.Get();
			if ( pEffectOwner )
			{
				// Kill charge effect instantly when holstering otherwise it looks bad.
				if ( m_bHolstered )
				{
					pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pChargeEffect );
				}
				else
				{
					pEffectOwner->ParticleProp()->StopEmission( m_pChargeEffect );
				}
				
				m_hChargeEffectHost = NULL;
			}

			m_pChargeEffect = NULL;
		}

		if ( m_pChargedSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
			m_pChargedSound = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CWeaponMedigun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

/*
    This is a full update
    if (updateType == DATA_UPDATE_CREATED)
    {
        Assert(0 == 1);

    }
*/
	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}

	if ( m_bBackpackTargetsParity != m_bOldBackpackTargetsParity )
	{
		m_bUpdateBackpackTargets = true;
	}

	if ( m_bUpdateBackpackTargets )
	{
		UpdateEffects();
		m_bUpdateBackpackTargets = false;
	}


	// Think?
	if ( m_bHealing )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}
	else
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound( true, false );

		// Are they holding the attack button but not healing anyone? Give feedback.
		CBaseCombatCharacter *pOwner = GetOwner();
		if ( pOwner && pOwner->IsAlive() && m_bAttacking && IsActiveByLocalPlayer() && pOwner == C_BasePlayer::GetLocalPlayer() && CanAttack() )
		{
			if ( gpGlobals->curtime >= m_flNextBuzzTime )
			{
				CLocalPlayerFilter filter;
				EmitSound( filter, entindex(), "WeaponMedigun.NoTarget" );
				m_flNextBuzzTime = gpGlobals->curtime + 0.5f; // Only buzz every so often.
			}
		}
		else
		{
			StopHealSound( false, true ); // Stop the "no target" sound.
			UpdateEffects();
		}
	}

	ManageChargeEffect();
	UpdateMedicAutoCallers();
}


void CWeaponMedigun::ClientThink()
{
	// Don't show it while the player is dead. Ideally, we'd respond to m_bHealing in OnDataChanged,
	// but it stops sending the weapon when it's holstered, and it gets holstered when the player dies.
	CTFPlayer *pFiringPlayer = GetTFPlayerOwner();
	if ( !pFiringPlayer || pFiringPlayer->IsPlayerDead() || pFiringPlayer->IsDormant() )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound();
		return;
	}
		
	// If the local player is the guy getting healed, let him know 
	// who's healing him, and their charge level.
	if ( m_hHealingTarget )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer == m_hHealingTarget )
		{
			pLocalPlayer->SetHealer( pFiringPlayer, m_flChargeLevel );
		}

		if ( !m_bPlayingSound )
		{
			m_bPlayingSound = true;
			CLocalPlayerFilter filter;
			EmitSound( filter, entindex(), GetHealSound() );
		}
	}

	if ( m_bOldChargeRelease != m_bChargeRelease )
	{
		m_bOldChargeRelease = m_bChargeRelease;
		ForceHealingTargetUpdate();
	}
}


void CWeaponMedigun::ThirdPersonSwitch( bool bThirdperson )
{
	BaseClass::ThirdPersonSwitch( bThirdperson );

	// Restart any effects.
	if ( m_hHealingTargetEffect.pEffect )
	{
		C_BaseEntity *pEffectOwner = m_hHealingTargetEffect.hOwner.Get();
		if ( m_hHealingTargetEffect.pEffect && pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_hHealingTargetEffect.pEffect );
		}

		UpdateEffects();
	}

	if ( m_pChargeEffect )
	{
		C_BaseEntity *pEffectOwner = m_hChargeEffectHost.Get();
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pChargeEffect );
			m_pChargeEffect = NULL;
			m_hChargeEffectHost = NULL;
		}

		ManageChargeEffect();
	}
}

bool CWeaponMedigun::ShouldShowEffectForPlayer( C_TFPlayer *pPlayer )
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


void CWeaponMedigun::UpdateEffects( void )
{
	// Remove all the effects.
	C_BaseEntity *pEffectOwner = m_hHealingTargetEffect.hOwner.Get();
	if ( m_hHealingTargetEffect.pEffect && pEffectOwner )
	{
		pEffectOwner->ParticleProp()->StopEmission( m_hHealingTargetEffect.pEffect );
	}

	m_hHealingTargetEffect.hOwner = NULL;
	m_hHealingTargetEffect.pTarget = NULL;
	m_hHealingTargetEffect.pEffect = NULL;

	pEffectOwner = GetWeaponForEffect();

	// Don't add targets if the medic is dead.
	CTFPlayer *pFiringPlayer = GetTFPlayerOwner();
	if ( !pFiringPlayer || !pEffectOwner || pFiringPlayer->IsPlayerDead() || pFiringPlayer->IsPlayerClass( TF_CLASS_UNDEFINED, true ) )
		return;

	// Add our targets
	// Loops through the healing targets, and make sure we have an effect for each of them.
	if ( m_hHealingTarget )
	{
		if ( m_hHealingTargetEffect.pTarget == m_hHealingTarget )
			return;

		char szFormat[128];
		V_strcpy_safe( szFormat, GetBeamParticleName(g_pszMedigunParticles[GetMedigunType()]) );

		if ( IsReleasingCharge() )
		{
			V_strcat_safe( szFormat, "_invun" );
		}

		if ( pFiringPlayer->IsLocalPlayer() && hud_medichealtargetmarker.GetBool() )
		{
			V_strcat_safe( szFormat, "_targeted" );
		}

		CNewParticleEffect *pEffect = pEffectOwner->ParticleProp()->Create( szFormat, PATTACH_POINT_FOLLOW, "muzzle" );
		pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 50 ) );

		m_hHealingTargetEffect.hOwner = pEffectOwner;
		m_hHealingTargetEffect.pTarget = m_hHealingTarget;
		m_hHealingTargetEffect.pEffect = pEffect;
	}

	// Backpack healing.
	if ( !m_iMaxBackpackTargets )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();

	// Find all the targets we've stopped healing.
	int i, j, c, x;
	bool bStillBPHealing[MAX_PLAYERS];
	for ( i = 0, c = m_hBackpackTargetEffects.Count(); i < c; i++ )
	{
		bStillBPHealing[i] = false;

		// Are we still healing this target?
		for ( j = 0, x = m_vBackpackTargets.Count(); j < x; j++ )
		{
			if ( m_bIsBPTargetActive[j] && m_vBackpackTargets[j] &&
				m_vBackpackTargets[j] == m_hBackpackTargetEffects[i].pTarget &&
				ShouldShowEffectForPlayer( ToTFPlayer( m_vBackpackTargets[j] ) ) &&
				m_bHolstered )
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
			m_hBackpackTargetEffects.Remove( i );
			CPASFilter filter( GetAbsOrigin() );
			EmitSound_t eSoundParams;
			eSoundParams.m_pSoundName = BP_SOUND_HEALOFF;
			eSoundParams.m_flVolume = 0.0f;
			eSoundParams.m_nFlags |= SND_CHANGE_VOL;
			EmitSound( filter, entindex(), eSoundParams );
		}
	}

	// Now add any new targets.
	for ( i = 0, c = m_vBackpackTargets.Count(); i < c; i++ )
	{
		C_TFPlayer *pTarget = ToTFPlayer( m_vBackpackTargets[i].Get() );
		if ( !pTarget || !ShouldShowEffectForPlayer( pTarget ) || !m_bIsBPTargetActive[i] || !m_bHolstered )
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

		const char *pszEffectName = ConstructTeamParticle( "overhealer_%s_beam", GetTeamNumber() );
		CNewParticleEffect *pEffect = pOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "flag" );
		pOwner->ParticleProp()->AddControlPoint( pEffect, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 50 ) );
		
		int iIndex = m_hBackpackTargetEffects.AddToTail();
		m_hBackpackTargetEffects[iIndex].pTarget = pTarget;
		m_hBackpackTargetEffects[iIndex].pEffect = pEffect;
		m_hBackpackTargetEffects[iIndex].hOwner = pOwner;

		CPASFilter filter( GetAbsOrigin() );
		EmitSound_t eSoundParams;
		eSoundParams.m_pSoundName = BP_SOUND_HEALON;
		eSoundParams.m_flVolume = 0.5f;
		eSoundParams.m_nFlags |= SND_CHANGE_VOL;
		EmitSound( filter, entindex(), eSoundParams );
	}

	// Manage overhead icons for the targets we've marked
	FOR_EACH_VEC( m_vBackpackTargets, ix )
	{
		int iIndex = -1;
		bool bHasTheCorrectEffect = false;

		if ( !m_vBackpackTargets[ix]->IsAlive() )
			continue;

		// Check if we have the right effect for this guy
		FOR_EACH_VEC( m_hTargetOverheadEffects, jx )
		{
			// Found his effect, but is it the right one?
			if ( m_hTargetOverheadEffects[jx].pTarget == m_vBackpackTargets[ix] )
			{
				// Is he being healed?
				bHasTheCorrectEffect = m_bIsBPTargetActive[ix] == m_hTargetOverheadEffects[jx].bActiveEffect;
				iIndex = jx;
				break;
			}
		}

		// This guy hasn't the right effect, so we'll give him one.
		if ( !bHasTheCorrectEffect )
		{
			// Does he have any affect at all? Delete it.
			if ( iIndex > -1 )
			{
				ParticleProp()->StopEmission( m_hTargetOverheadEffects[iIndex].pEffect );
				m_hTargetOverheadEffects.Remove( iIndex );
			}

			// Now give him the right one.
			const char *pszEffectName = m_bIsBPTargetActive[ix] ? BP_PARTICLE_ACTIVE : BP_PARTICLE_INACTIVE;
			CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pEffect, 1, m_vBackpackTargets[ix], PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 60 ) ); // Position it above his head.

			int iIndexAdd = m_hTargetOverheadEffects.AddToTail();
			m_hTargetOverheadEffects[iIndexAdd].pTarget = m_vBackpackTargets[ix];
			m_hTargetOverheadEffects[iIndexAdd].pEffect = pEffect;
			m_hTargetOverheadEffects[iIndexAdd].bActiveEffect = m_bIsBPTargetActive[ix];
		}
	}

	// Now delete any orphaned effects.
	FOR_EACH_VEC( m_hTargetOverheadEffects, kx )
	{
		if ( !m_vBackpackTargets.HasElement( m_hTargetOverheadEffects[kx].pTarget ) || !m_hTargetOverheadEffects[kx].pTarget->IsAlive() )
		{
			ParticleProp()->StopEmission( m_hTargetOverheadEffects[kx].pEffect );
			m_hTargetOverheadEffects.Remove( kx );
#ifdef DEBUGBACKPACKBEAMS
			DevMsg("Deleting orphaned effect at index %i. \n", kx);
#endif
		}
	}
}


void CWeaponMedigun::UpdateMedicAutoCallers( void )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner || !pOwner->IsLocalPlayer() || !hud_medicautocallers.GetBool() )
		return;

	// Monitor teammates' health levels.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// Ignore enemies and dead players.
		C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->IsDormant() || pPlayer->GetTeamNumber() != pOwner->GetTeamNumber() || !pPlayer->IsAlive() )
		{
			m_bPlayerHurt[i] = false;
			continue;
		}

		// Create auto caller if their health drops below threshold.
		// Only create auto callers for nearby players.
		bool bHurt = ( pPlayer->HealthFraction() <= hud_medicautocallersthreshold.GetFloat() * 0.01f );
		if ( pOwner->IsAlive() && bHurt && !m_bPlayerHurt[i] && pOwner->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() ) < 1000.0f )
		{
			pPlayer->CreateSaveMeEffect( true );
		}

		m_bPlayerHurt[i] = bHurt;
	}
}
#endif

#ifdef CLIENT_DLL
void CTFHealSoundManager::SoundManagerThink()
{
	if ( m_pPatient && m_pHealer && m_pMedigun && m_bHealingPatient && m_pActiveCollage && tf2c_medigun_heal_progress_sound.GetBool() )
	{
		int iCurrentHealth = m_pPatient->IsDisguisedEnemy() ? m_pPatient->m_Shared.GetDisguiseHealth() : m_pPatient->GetHealth();
		m_flCurrentRatio = iCurrentHealth / (m_pPatient->IsDisguisedEnemy() ? (float)m_pPatient->m_Shared.GetDisguiseMaxBuffedHealth() : (float)m_pPatient->m_Shared.GetMaxBuffedHealth());

		if ( m_pPatient->IsAlive() /* m_pPatient->m_Shared.InCond( TF_COND_HEALTH_BUFF )*/ )
		{
			if ( !m_bHealSoundPlaying )
			{
				m_bHealSoundPlaying = true;
				CreateSound();
			}

			// Reset the timer.
			m_flStopPlayingTime = gpGlobals->curtime + m_flRemoveTime;
		}

		if ( m_bHealSoundPlaying /*&& m_pHealProgressSound*/ && m_pPatient )
		{
			// If we're not as max health yet, do full volume. Otherwise, quieten down a bit.
			if ( m_flCurrentRatio >= 1.0f )
				m_flBaseVolume = tf2c_medigun_heal_progress_sound_volume_maxhp.GetFloat() * tf2c_medigun_heal_progress_sound_volume.GetFloat();
			else
				m_flBaseVolume = tf2c_medigun_heal_progress_sound_volume.GetFloat();

			CLocalPlayerFilter filter;
			Vector earPos = m_pHealer->EyePosition();
			if ( iCurrentHealth >= m_pPatient->GetMaxHealth() && m_iLastHealthValue < m_pPatient->GetMaxHealth() )
			{
				m_pMedigun->EmitSound( filter, m_pMedigun->entindex(), m_pActiveCollage->GetOverhealThresholdSound(), &earPos );
			}
			// Use maxbuffedhealth -1 to avoid the edge case where it goes from full overheal to full overheal -1 in some cases (in Live anyway, might be fixed here)
			else if ( iCurrentHealth >= m_pPatient->m_Shared.GetMaxBuffedHealth() - 1 && m_iLastHealthValue < m_pPatient->m_Shared.GetMaxBuffedHealth() - 1 )
			{
				m_pMedigun->EmitSound( filter, m_pMedigun->entindex(), m_pActiveCollage->GetOverhealThresholdMaxSound(), &earPos );
			}
		}

		// Unset this flag so we stop trying to read the patient's health.
		// It will be reset each frame while healing as SetPatient is called.
		m_bHealingPatient = false;

		m_iLastHealthValue = iCurrentHealth;
	}

	if ( m_bHealSoundPlaying /*&& m_pHealProgressSound*/ )
	{
		// Lerp* the pitch based on the removal-time part of the timer.
		// Disclaimer: This isn't a lerp per se. It's a fixed-rate slide.

		// Difference is small enough that we can just snap.
		if ( abs( m_flRatioForPitch - m_flCurrentRatio ) < tf2c_medigun_heal_progress_sound_slide_rate.GetFloat() * gpGlobals->frametime )
		{
			m_flRatioForPitch = m_flCurrentRatio;
		}
		else
		{
			// Which direction are we sliding?
			if ( m_flRatioForPitch > m_flCurrentRatio ) // Down
			{
				m_flRatioForPitch -= tf2c_medigun_heal_progress_sound_slide_rate_down.GetFloat() * gpGlobals->frametime;
				m_flStopPlayingTime = gpGlobals->curtime + m_flRemoveTime;
			}
			else if ( m_flRatioForPitch < m_flCurrentRatio ) // Up
			{
				m_flRatioForPitch += tf2c_medigun_heal_progress_sound_slide_rate.GetFloat() * gpGlobals->frametime;
				m_flStopPlayingTime = gpGlobals->curtime + m_flRemoveTime;
			}
		}
		

		EnvelopeSounds( m_flBaseVolume, m_flRatioForPitch );

		if ( gpGlobals->curtime > m_flStopPlayingTime )
			RemoveSound();
	}
}

void CTFHealSoundManager::SetPatient( CTFPlayer *pPatient )
{
    Assert(this);
    if (!this)
    {
        return;
    }
	bool bNeedsUpdate = !m_pPatient && pPatient;
//	bool bNeedsUpdate = (bool)pPatient;

	m_pPatient = pPatient;
	if ( pPatient )
	{
		m_flStopPlayingTime = gpGlobals->curtime + m_flRemoveTime;
		m_bHealingPatient = true;
		m_flBaseVolume = tf2c_medigun_heal_progress_sound_volume.GetFloat();
	}

	if ( bNeedsUpdate )
		
		SoundManagerThink();
}

void CTFHealSoundManager::EnvelopeSounds( float flBaseVolume, float flHealthRatio )
{
	if ( !m_pActiveCollage )
		return;

	// Fade out the volume based on the fadeout part of the timer.
	float flFadeoutTimeRemaining = clamp( m_flStopPlayingTime - gpGlobals->curtime, 0, m_flFadeoutTime );
	float flVolumeRemap = RemapVal( flFadeoutTimeRemaining, m_flFadeoutTime, 0.0f, flBaseVolume, 0.0f );
	
	m_pActiveCollage->EnvelopeSounds( flVolumeRemap, flHealthRatio );
}

void CTFHealSoundManager::SoundCollage::EnvelopeSounds( float flBaseVolume, float flHealthRatio )
{
	FOR_EACH_VEC( m_vEnvelopedSounds, i )
	{
		EnvelopeSound( &m_vEnvelopedSounds[i], flHealthRatio, flBaseVolume );
	}
}

void CTFHealSoundManager::SoundCollage::EnvelopeSound( layeredenvelope_t *pLayeredSound, float flBaseRatio, float flBaseVolume )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	
	Vector4D vVolumeParams = pLayeredSound->m_vVolumeBounds;
	Vector4D vPitchParams = pLayeredSound->m_vPitchBounds;
	
	float flRatioForPitch = RemapValClamped( clamp(flBaseRatio, vPitchParams.x, vPitchParams.y), vPitchParams.x, vPitchParams.y, vPitchParams.z, vPitchParams.w );
	float flPitch = (0.7f + flRatioForPitch * flRatioForPitch) * 100.0f;

	float flRatioForVolume = RemapValClamped( clamp( flBaseRatio, vVolumeParams.x, vVolumeParams.y ), vVolumeParams.x, vVolumeParams.y, vVolumeParams.z, vVolumeParams.w );
	float flVolume = flRatioForVolume * flBaseVolume; // Volume can be interpreted pretty straight; just take the remapped ratio.
	
	//DevMsg( "Sound %s modified: %2.3f pitch, %2.3f volume. \n", pLayeredSound->szSoundName, flPitch, flVolume );

	// Need to do an extra clamp here otherwise we hit 266.000001 or something.
	controller.SoundChangePitch( pLayeredSound->m_pSound, clamp(flPitch, 0, 266.0f), 0.0f );
	controller.SoundChangeVolume( pLayeredSound->m_pSound, flVolume, 0.0f );
}

void CTFHealSoundManager::SoundCollage::Create(CTFWeaponBase *pMedigun)
{
	// Start the sound effect
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();
	CLocalPlayerFilter filter;
	
	if ( m_pszStartSound && Q_strcmp( m_pszStartSound, "NULL" ) != 0 && pMedigun )
	{
		m_pMedigun = pMedigun;
		CLocalPlayerFilter filter;
		Vector earPos = m_pMedigun->GetTFPlayerOwner()->EyePosition();
		m_pMedigun->EmitSound( filter, m_pMedigun->entindex(), m_pszStartSound, &earPos );
	}

	FOR_EACH_VEC( m_vEnvelopedSounds, i )
	{
		m_vEnvelopedSounds[i].m_pSound = controller.SoundCreate( filter, pMedigun->entindex(), m_vEnvelopedSounds[i].szSoundName );
		controller.Play( m_vEnvelopedSounds[i].m_pSound, 0.0f, 100.0f );
	}
}

void CTFHealSoundManager::SoundCollage::Remove()
{
	if ( m_pszFinishSound && Q_strcmp( m_pszFinishSound, "NULL" ) != 0 && m_pMedigun )
	{
		if ( m_pMedigun->GetTFPlayerOwner() )
		{
			CLocalPlayerFilter filter;
			Vector earPos = m_pMedigun->GetTFPlayerOwner()->EyePosition();
			m_pMedigun->EmitSound( filter, m_pMedigun->entindex(), m_pszFinishSound, &earPos );
			m_pMedigun = NULL;
		}
	}

	FOR_EACH_VEC( m_vEnvelopedSounds, i )
	{
		if ( m_vEnvelopedSounds[i].m_pSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_vEnvelopedSounds[i].m_pSound );
			m_vEnvelopedSounds[i].m_pSound = NULL;
		}
	}
}
void CTFHealSoundManager::SoundCollage::CleanupRemove()
{
	m_vEnvelopedSounds.Purge();
}

void CTFHealSoundManager::SoundCollage::Precache( CBaseEntity *pParent )
{
	FOR_EACH_VEC( m_vEnvelopedSounds, i )
	{
		pParent->PrecacheScriptSound( m_vEnvelopedSounds[i].szSoundName );
	}
	pParent->PrecacheScriptSound( m_pszFinishSound );
	pParent->PrecacheScriptSound( m_pszStartSound );
	pParent->PrecacheScriptSound( m_pszMaxHPSound );
	pParent->PrecacheScriptSound( m_pszMaxOverhealSound );
}

void CTFHealSoundManager::Precache( CBaseEntity *pParent )
{
	FOR_EACH_VEC( m_vCollages, i )
	{
		m_vCollages[i]->Precache(pParent);
	}
}

CTFHealSoundManager::SoundCollage::SoundCollage( KeyValues *pKeyValuesData )
{
	m_vEnvelopedSounds = CUtlVector<layeredenvelope_t>();
	m_pszName = pKeyValuesData->GetName();

	KeyValues *pLoops = pKeyValuesData->FindKey( "loops" );
	KeyValues *pStart = pKeyValuesData->FindKey( "start" );
	KeyValues *pFinish = pKeyValuesData->FindKey( "finish" );
	KeyValues *pFullHP = pKeyValuesData->FindKey( "fullhp" );
	KeyValues *pFullOH = pKeyValuesData->FindKey( "fulloverheal" );
	
	m_pMedigun = NULL;

	// Process the loop sounds
	if ( pLoops )
	{
		FOR_EACH_SUBKEY( pLoops, pSubData )
		{
			const char *pszSoundScriptName = pSubData->GetName();
			const char *pszVolumeParams = pSubData->GetString( "volume", "0.0 1.0 0.0 1.0" );
			const char *pszPitchParams = pSubData->GetString( "pitch", "0.0 1.0 0.0 1.0" );
            int idx = m_vEnvelopedSounds.AddToTail(layeredenvelope_t({}));
			Vector4D pitchVec, volumeVec;

			UTIL_StringToFloatArray( pitchVec.Base(), 4, pszPitchParams );
			UTIL_StringToFloatArray( volumeVec.Base(), 4, pszVolumeParams );

			m_vEnvelopedSounds[idx].m_vPitchBounds = pitchVec;
			m_vEnvelopedSounds[idx].m_vVolumeBounds = volumeVec;
			m_vEnvelopedSounds[idx].szSoundName = pszSoundScriptName;
			m_vEnvelopedSounds[idx].m_pSound = NULL;
		}
	}

	m_pszStartSound = "NULL";
	// Process the starting sound
	if ( pStart )
		m_pszStartSound = pStart->GetString( "sound", "NULL" );

	m_pszFinishSound = "NULL";
	// Process the finishing sound
	if ( pFinish )
		m_pszFinishSound = pFinish->GetString( "sound", "NULL" );

	m_pszMaxHPSound = "WeaponMedigun.Threshold.Overheal";
	if ( pFullHP )
		m_pszMaxHPSound = pFullHP->GetString( "sound", "WeaponMedigun.Threshold.Overheal" );

	m_pszMaxOverhealSound = "WeaponMedigun.Threshold.OverhealMax";
	if ( pFullOH )
		m_pszMaxOverhealSound = pFullOH->GetString( "sound", "WeaponMedigun.Threshold.OverhealMax" );
}

CTFHealSoundManager::SoundCollage::~SoundCollage( void )
{
	Remove();
	//m_vEnvelopedSounds.PurgeAndDeleteElements();
	m_vEnvelopedSounds.Purge();
}

CTFHealSoundManager::CTFHealSoundManager( CTFPlayer *pHealer, CTFWeaponBase *pMedigun )
{
	m_bSchemaLoaded = m_bHealingPatient = m_bHealSoundPlaying = false;
	m_flStopPlayingTime = -1;
	m_pHealer = pHealer;
	m_pMedigun = pMedigun;
	m_iLastHealthValue = 0;
	m_flRatioForPitch = 0.001f;

	m_pActiveCollage = NULL;
	ParseCollageSchema("scripts/tf_healsounds.txt");

	TrySelectSoundCollage( tf2c_medigun_heal_progress_sound_name.GetString() );
}

CTFHealSoundManager::~CTFHealSoundManager()
{
	//FOR_EACH_MAP( m_Collages, i )
	//{
	//	m_Collages[i]->Remove();
	//	delete m_Collages[i];
	//}
	m_vCollages.PurgeAndDeleteElements();
}

bool CTFHealSoundManager::ParseCollageSchema( const char *pszFile )
{
	FOR_EACH_VEC( m_vCollages, i ) {
		m_vCollages[i]->CleanupRemove();
	}
	m_vCollages.PurgeAndDeleteElements();


	m_bSchemaLoaded = false;

	// leaky leaky!
	// -sappho
	KeyValues *pSchemaData = new KeyValues( "SoundCollage" );
	if ( !pSchemaData->LoadFromFile( filesystem, pszFile, "GAME" ) )
	{
		DevWarning( "%s is missing or corrupt!\n", pszFile );
		pSchemaData->deleteThis();
		return false;
	}	

	float flStartTime = engine->Time();

	FOR_EACH_SUBKEY(pSchemaData, pSubData)
	{
		ParseCollage( pSubData );
	}
	pSchemaData->deleteThis();
	float flEndTime = engine->Time();
	DevMsg( "Processing of Sound Collage Schema \"%s\" took %.02fms. Total: %d collages.\n",
		pszFile,
		(flEndTime - flStartTime) * 1000.0f,
		m_vCollages.Count()
	);

	m_bSchemaLoaded = true;

	// pSchemaData->deleteThis();
	return true;
}

void CTFHealSoundManager::ParseCollage( KeyValues *pKeyValuesData ) {
	SoundCollage *collage = new SoundCollage(pKeyValuesData);
	m_vCollages.AddToTail( collage );
}

void CTFHealSoundManager::CreateSound()
{
	if ( !m_bSchemaLoaded || !tf2c_medigun_heal_progress_sound.GetBool())
		return;

	if ( m_pActiveCollage && m_pMedigun )
		m_pActiveCollage->Create(m_pMedigun);

	if ( m_pPatient )
		EnvelopeSounds( tf2c_medigun_heal_progress_sound_volume.GetFloat(), m_pPatient->GetHealth() / (float)m_pPatient->m_Shared.GetMaxBuffedHealth() );

	DevMsg( "Created sounds. \n" );
}

void CTFHealSoundManager::RemoveSound()
{
	if ( m_pActiveCollage )
		m_pActiveCollage->Remove();

	m_bHealingPatient = m_bHealSoundPlaying = false;

	DevMsg("Removed sounds. \n");
}

void CTFHealSoundManager::SetLastPatientHealth( int iLastHealth )
{
	if ( m_pPatient && iLastHealth )
	{
		int maxBufHP = m_pPatient->m_Shared.GetMaxBuffedHealth();
		if (!maxBufHP)
		{
			m_flRatioForPitch = 0.0001f;
		}
		else
		{
			// there was a crash here somehow. hopefully it was fixed
			// https://gl.ofdev.xyz/tf2c/source/tf2c-src/-/issues/19
			// -sappho
			m_flRatioForPitch = iLastHealth / (float)maxBufHP;
		}
	}
	else
	{
		m_flRatioForPitch = 0.0001f;
	}
}

bool CTFHealSoundManager::IsSetupCorrectly()
{
	return m_pHealer && m_pMedigun;
}

bool CTFHealSoundManager::TrySelectSoundCollage( const char *pszCollageName )
{
	FOR_EACH_VEC( m_vCollages, i )
	{
		if ( !Q_strcmp( m_vCollages[i]->GetName(), pszCollageName ) )
		{
			m_pActiveCollage = m_vCollages[i];
			return true;
		}
	}
	return false;
}
#endif
