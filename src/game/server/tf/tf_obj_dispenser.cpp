//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Dispenser
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_obj_dispenser.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "vguiscreen.h"
#include "world.h"
#include "explode.h"
#include "triggers.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define DISPENSER_MODEL_PLACEMENT			"models/buildables/dispenser_blueprint.mdl"
// *_UPGRADE models are models used during the upgrade transition
// Valve fucked up the naming of the models, the _light ones (which should be the transition models)
// are actually the ones that are set AFTER the upgrade transition.

#define DISPENSER_MODEL_LEVEL_1				"models/buildables/dispenser_light.mdl"
#define DISPENSER_MODEL_LEVEL_1_UPGRADE		"models/buildables/dispenser.mdl"
#define DISPENSER_MODEL_LEVEL_2				"models/buildables/dispenser_lvl2_light.mdl"
#define DISPENSER_MODEL_LEVEL_2_UPGRADE		"models/buildables/dispenser_lvl2.mdl"
#define DISPENSER_MODEL_LEVEL_3				"models/buildables/dispenser_lvl3_light.mdl"
#define DISPENSER_MODEL_LEVEL_3_UPGRADE		"models/buildables/dispenser_lvl3.mdl"

#define DISPENSER_TRIGGER_MINS				Vector( -70, -70, 0 )
#define DISPENSER_TRIGGER_MAXS				Vector( 70, 70, 50 )

#define REFILL_CONTEXT						"RefillContext"
#define DISPENSE_CONTEXT					"DispenseContext"

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Healing list UtlVector to entindices
//-----------------------------------------------------------------------------
void SendProxy_HealingList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CObjectDispenser *pDispenser = (CObjectDispenser *)pStruct;

	// If this assertion fails, then SendProxyArrayLength_HealingArray must have failed.
	Assert( iElement < pDispenser->m_hHealingTargets.Size() );

	EHANDLE hOther = pDispenser->m_hHealingTargets[iElement].Get();
	SendProxy_EHandleToInt( pProp, pStruct, &hOther, pOut, iElement, objectID );
}

int SendProxyArrayLength_HealingArray( const void *pStruct, int objectID )
{
	return ( (CObjectDispenser *)pStruct )->m_hHealingTargets.Count();
}

IMPLEMENT_SERVERCLASS_ST( CObjectDispenser, DT_ObjectDispenser )
	SendPropBool( SENDINFO( m_bHealingTargetsParity ) ),
	SendPropInt( SENDINFO( m_iAmmoMetal ), 10 ),
	SendPropArray2( 
		SendProxyArrayLength_HealingArray,
		SendPropInt( "healing_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_HealingList ), 
		MAX_PLAYERS, 
		0, 
		"healing_array"
		)
END_SEND_TABLE()

BEGIN_DATADESC( CObjectDispenser )
	DEFINE_THINKFUNC( RefillThink ),
	DEFINE_THINKFUNC( DispenseThink ),

	DEFINE_KEYFIELD( m_szTriggerName, FIELD_STRING, "touch_trigger" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( obj_dispenser, CObjectDispenser );
PRECACHE_REGISTER( obj_dispenser );

#define DISPENSER_MAX_HEALTH	150

// How much of each ammo gets added per refill
#define DISPENSER_REFILL_METAL_AMMO	40


// How much ammo is given our per use
#define DISPENSER_DROP_PRIMARY		40
#define DISPENSER_DROP_SECONDARY	40
#define DISPENSER_DROP_METAL		40

ConVar obj_dispenser_heal_rate( "obj_dispenser_heal_rate", "10.0", FCVAR_CHEAT );

extern ConVar tf_cheapobjects;

class CDispenserTouchTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CDispenserTouchTrigger, CBaseTrigger );

public:
	CDispenserTouchTrigger() {}

	void Spawn( void )
	{
		BaseClass::Spawn();
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
		InitTrigger();
	}

	virtual void StartTouch( CBaseEntity *pEntity )
	{
		if ( PassesTriggerFilters( pEntity ) )
		{
			CObjectDispenser *pParent = static_cast<CObjectDispenser *>( GetOwnerEntity() );
			if ( pParent )
			{
				pParent->StartTriggerTouch( pEntity );
			}
		}
	}

	virtual void EndTouch( CBaseEntity *pEntity )
	{
		if ( PassesTriggerFilters( pEntity ) )
		{
			CObjectDispenser *pParent = static_cast<CObjectDispenser *>( GetOwnerEntity() );
			if ( pParent )
			{
				pParent->EndTriggerTouch( pEntity );
			}
		}
	}
};

LINK_ENTITY_TO_CLASS( dispenser_touch_trigger, CDispenserTouchTrigger );


CObjectDispenser::CObjectDispenser()
{
	m_bHealingTargetsParity = false;

	UseClientSideAnimation();

	SetMaxHealth( DISPENSER_MAX_HEALTH );
	m_iHealth = DISPENSER_MAX_HEALTH;
	m_flRadius = 0.0f;

	SetType( OBJ_DISPENSER );
	m_hTouchingEntities.Purge();
	m_hTouchTrigger = NULL;
}

CObjectDispenser::~CObjectDispenser()
{
	if ( m_hTouchTrigger.Get() )
	{
		UTIL_Remove( m_hTouchTrigger );
	}

	ResetHealingTargets();

	StopSound( "Building_Dispenser.Idle" );
}


void CObjectDispenser::Spawn()
{
	SetModel( GetPlacementModel() );
	SetSolid( SOLID_BBOX );

	SetTouch( &CObjectDispenser::Touch );

	UTIL_SetSize( this, GetMins(), GetMaxs());
	m_takedamage = DAMAGE_YES;

	BaseClass::Spawn();
}

const char* CObjectDispenser::GetDispenserModelForLevel(int level) const
{
	switch (level)
	{
	case 1:
		return DISPENSER_MODEL_LEVEL_1;
	case 2:
		return DISPENSER_MODEL_LEVEL_2;
	case 3:
		return DISPENSER_MODEL_LEVEL_3;
	default:
		return DISPENSER_MODEL_LEVEL_1;
	}
}

const char* CObjectDispenser::GetDispenserUpgradeModelForLevel(int level) const
{
	switch (level)
	{
		case 1:
			return DISPENSER_MODEL_LEVEL_1_UPGRADE;
		case 2:
			return DISPENSER_MODEL_LEVEL_2_UPGRADE;
		case 3:
			return DISPENSER_MODEL_LEVEL_3_UPGRADE;
		default:
			return DISPENSER_MODEL_LEVEL_1_UPGRADE;
	}
}

void CObjectDispenser::FirstSpawn()
{
	SetSolid( SOLID_BBOX );

	m_iAmmoMetal = 0;

	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	BaseClass::FirstSpawn();
}


void CObjectDispenser::MakeCarriedObject( CTFPlayer *pPlayer )
{
	StopSound( "Building_Dispenser.Idle" );

	// Remove our healing trigger.
	if ( m_hTouchTrigger.Get() )
	{
		UTIL_Remove( m_hTouchTrigger );
		m_hTouchTrigger = NULL;
	}

	// Stop healing everyone.
	ResetHealingTargets();

	m_hTouchingEntities.Purge();

	// Stop all thinking, we'll resume it once we get re-deployed.
	SetContextThink( NULL, 0, DISPENSE_CONTEXT );
	SetContextThink( NULL, 0, REFILL_CONTEXT );

	BaseClass::MakeCarriedObject( pPlayer );
}


void CObjectDispenser::DropCarriedObject( CTFPlayer *pPlayer )
{
	BaseClass::DropCarriedObject( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectDispenser::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( GetDispenserUpgradeModelForLevel( 1 ) );

	CreateBuildPoints();

	return BaseClass::StartBuilding( pBuilder );
}


void CObjectDispenser::InitializeMapPlacedObject( void )
{
	// Must set model here so we can add control panels.
	SetModel( GetDispenserModelForLevel(1) );
	BaseClass::InitializeMapPlacedObject();
}

void CObjectDispenser::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );
	UTIL_SetSize( this, GetMins(), GetMaxs());
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CObjectDispenser::OnGoActive( void )
{
	SetModel( GetDispenserModelForLevel(1) );
	CreateBuildPoints();
	ReattachChildren();

	if ( !m_bCarryDeploy )
	{
		// Put some ammo in the Dispenser.
		m_iAmmoMetal = 25;
	}

	// Begin thinking
	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 3.0f, REFILL_CONTEXT );
	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1f, DISPENSE_CONTEXT );

	m_flNextAmmoDispense = gpGlobals->curtime + 0.5f;

	if ( m_hTouchTrigger.Get() && dynamic_cast<CObjectCartDispenser *>( this ) )
	{
		UTIL_Remove( m_hTouchTrigger.Get() );
		m_hTouchTrigger = NULL;
	}

	if ( m_szTriggerName != NULL_STRING )
	{
		m_hTouchTrigger = dynamic_cast<CDispenserTouchTrigger *>( gEntList.FindEntityByName( NULL, m_szTriggerName ) );
		if ( m_hTouchTrigger.Get() )
		{	
			m_hTouchTrigger->SetOwnerEntity( this );
		}
	}
	
	if ( !m_hTouchTrigger.Get() )
	{
		m_hTouchTrigger = CBaseEntity::Create( "dispenser_touch_trigger", GetAbsOrigin(), vec3_angle, this );
		if ( m_hTouchTrigger.Get() )
		{
			// Remember the radius.
			m_flRadius = GetDispenserRadius();
			UTIL_SetSize( m_hTouchTrigger, Vector( -m_flRadius ), Vector( m_flRadius ) );
			m_hTouchTrigger->SetSolid( SOLID_BBOX );
		}
	}

	BaseClass::OnGoActive();

	EmitSound( "Building_Dispenser.Idle" );
}

//-----------------------------------------------------------------------------
// Spawn the vgui control screens on the object
//-----------------------------------------------------------------------------
void CObjectDispenser::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// Panels 0 and 1 are both control panels for now.
	if ( nPanelIndex == 0 || nPanelIndex == 1 )
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				pPanelName = "screen_obj_dispenser_red";
				break;
			case TF_TEAM_BLUE:
				pPanelName = "screen_obj_dispenser_blue";
				break;
			case TF_TEAM_GREEN:
				pPanelName = "screen_obj_dispenser_green";
				break;
			case TF_TEAM_YELLOW:
				pPanelName = "screen_obj_dispenser_yellow";
				break;
			default:
				pPanelName = "screen_obj_dispenser_red";
				break;
		}
	}
	else
	{
		BaseClass::GetControlPanelInfo( nPanelIndex, pPanelName );
	}
}



void CObjectDispenser::Precache()
{
	BaseClass::Precache();

	PrecacheModel( GetPlacementModel() );
	for (int i = 1; i <= 3; i++)
	{
		PrecacheGibsForModel( PrecacheModel( GetDispenserModelForLevel(i) ) );
		PrecacheGibsForModel( PrecacheModel( GetDispenserUpgradeModelForLevel(i) ) );
	}

	PrecacheVGuiScreen( "screen_obj_dispenser_blue" );
	PrecacheVGuiScreen( "screen_obj_dispenser_red" );
	PrecacheVGuiScreen( "screen_obj_dispenser_green" );
	PrecacheVGuiScreen( "screen_obj_dispenser_yellow" );

	PrecacheScriptSound( "Building_Dispenser.Idle" );
	PrecacheScriptSound( "Building_Dispenser.GenerateMetal" );
	PrecacheScriptSound( "Building_Dispenser.Heal" );

	PrecacheTeamParticles( "dispenser_heal_%s" );
	PrecacheParticleSystem( "dispenserdamage_1" );
	PrecacheParticleSystem( "dispenserdamage_2" );
	PrecacheParticleSystem( "dispenserdamage_3" );
	PrecacheParticleSystem( "dispenserdamage_4" );
}

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CObjectDispenser::OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos )
{
	return BaseClass::OnWrenchHit( pPlayer, pWrench, vecHitPos );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectDispenser::IsUpgrading( void ) const
{
	return m_bIsUpgrading;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CObjectDispenser::GetPlacementModel( void ) const
{
	return DISPENSER_MODEL_PLACEMENT;
}

//-----------------------------------------------------------------------------
// If detonated, do some damage
//-----------------------------------------------------------------------------
void CObjectDispenser::DetonateObject( void )
{
	/*float flDamage = Min( 100 + m_iAmmoMetal, 250 );

	ExplosionCreate( 
		GetAbsOrigin(),
		GetAbsAngles(),
		GetBuilder(),
		flDamage,			// Magnitude.
		flDamage,			// Radius.
		0,
		0.0f,				// Explosion force.
		this,				// Inflictor.
		DMG_BLAST | DMG_HALF_FALLOFF );
	*/

	BaseClass::DetonateObject();
}

//-----------------------------------------------------------------------------
// Raises the dispenser one level
//-----------------------------------------------------------------------------
void CObjectDispenser::StartUpgrading( void )
{
	SetModel( GetDispenserUpgradeModelForLevel( GetUpgradeLevel() + 1 ) );

	ResetHealingTargets();

	BaseClass::StartUpgrading();

	m_bIsUpgrading = true;

	// Start upgrade anim instantly.
	DetermineAnimation();
}

void CObjectDispenser::FinishUpgrading( void )
{
	BaseClass::FinishUpgrading();

	SetModel( GetDispenserModelForLevel( GetUpgradeLevel() ) );

	m_bIsUpgrading = false;

	SetActivity( ACT_RESET );
}

bool CObjectDispenser::DispenseAmmo( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	int iTotalPickedUp = 0;
	float flAmmoRate = g_flDispenserAmmoRates[GetUpgradeLevel() - 1];

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flAmmoRate, mult_dispenser_rate );

	// Primary.
	int iNoPrimaryAmmo = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iNoPrimaryAmmo, no_primary_ammo_from_dispensers_always );
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer->GetActiveWeapon(), iNoPrimaryAmmo, no_primary_ammo_from_dispensers );
	if ( iNoPrimaryAmmo == 0 )
	{
		iTotalPickedUp += pPlayer->GiveAmmo( Ceil2Int( pPlayer->GetMaxAmmo( TF_AMMO_PRIMARY ) * flAmmoRate ), TF_AMMO_PRIMARY, false, TF_AMMO_SOURCE_DISPENSER );
	}

	// Secondary.
	iTotalPickedUp += pPlayer->GiveAmmo( Ceil2Int( pPlayer->GetMaxAmmo( TF_AMMO_SECONDARY ) * flAmmoRate ), TF_AMMO_SECONDARY, false, TF_AMMO_SOURCE_DISPENSER );

	int iNoMetal = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer->GetActiveWeapon(), iNoMetal, no_metal_from_dispensers_while_active );
	if ( iNoMetal == 0 )
	{
		// Cart dispenser has infinite metal.
		int iMetalToGive = DISPENSER_DROP_METAL + 10 * ( GetUpgradeLevel() - 1 );
		if ( !( GetObjectFlags() & OF_IS_CART_OBJECT ) )
		{
			iMetalToGive = Min( m_iAmmoMetal.Get(), iMetalToGive );
		}

		// Metal.
		int iMetal = pPlayer->GiveAmmo( iMetalToGive, TF_AMMO_METAL, false, TF_AMMO_SOURCE_DISPENSER );
		iTotalPickedUp += iMetal;

		if ( !( GetObjectFlags() & OF_IS_CART_OBJECT ) )
		{
			m_iAmmoMetal -= iMetal;
		}
	}

	// Return false if we didn't pick up anything.
	return iTotalPickedUp > 0;
}

int CObjectDispenser::GetBaseHealth( void ) const
{
	return DISPENSER_MAX_HEALTH;
}

float CObjectDispenser::GetDispenserRadius( void )
{
	float flRadius = 64.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flRadius, mult_dispenser_radius );
	return flRadius;
}

float CObjectDispenser::GetHealRate( void )
{
	return g_flDispenserHealRates[GetUpgradeLevel() - 1];
}

void CObjectDispenser::RefillThink( void )
{
	if ( GetObjectFlags() & OF_IS_CART_OBJECT )
		return;

	if ( IsDisabled() || IsUpgrading() || IsRedeploying() )
	{
		// Hit a refill time while disabled, so do the next refill ASAP.
		SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 0.1f, REFILL_CONTEXT );
		return;
	}

	// Auto-refill half the amount as TFC, but twice as often.
	if ( m_iAmmoMetal < DISPENSER_MAX_METAL_AMMO )
	{
		int iMetal = ( DISPENSER_MAX_METAL_AMMO * 0.1f ) + ( ( GetUpgradeLevel() - 1 ) * 10 );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), iMetal, mult_dispenser_rate );
		m_iAmmoMetal = Min( m_iAmmoMetal + iMetal, DISPENSER_MAX_METAL_AMMO );

		EmitSound( "Building_Dispenser.GenerateMetal" );
	}

	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + GetRefillDelay(), REFILL_CONTEXT );
}

//-----------------------------------------------------------------------------
// Generate ammo over time
//-----------------------------------------------------------------------------
void CObjectDispenser::DispenseThink( void )
{
	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1f, DISPENSE_CONTEXT );

	if ( IsDisabled() || IsRedeploying() )
	{
		// Don't heal or dispense ammo.
		ResetHealingTargets();
		return;
	}

	int i;
	if ( m_flNextAmmoDispense <= gpGlobals->curtime )
	{
		if ( GetBuilder() && m_hTouchTrigger.Get() )
		{
			// Keep the trigger size correct.
			float flRadius = GetDispenserRadius();
			if ( flRadius != m_flRadius )
			{
				m_flRadius = flRadius;
				UTIL_SetSize( m_hTouchTrigger.Get(), Vector( -m_flRadius ), Vector( m_flRadius ) );
			}
		}

		// Restock any players that we are currently healing.
		for ( i = m_hHealingTargets.Count() - 1; i >= 0; i-- )
		{
			DispenseAmmo( ToTFPlayer( m_hHealingTargets[i].Get() ) );
		}

		// Try to dispense more often when no players are around so we 
		// give it as soon as possible when a new player shows up.
		m_flNextAmmoDispense = gpGlobals->curtime + ( m_hHealingTargets.Count() != 0 ? 1.0f : 0.1f );
	}

	if ( !m_hTouchTrigger.Get() )
		return;

	// For each player in touching list.
	CBaseEntity *pOther;
	bool bHealingTarget, bValidHealTarget;
	for ( i = m_hTouchingEntities.Count() - 1; i >= 0; i-- )
	{
		pOther = m_hTouchingEntities[i].Get();
		if ( !pOther )
			continue;

		// Stop touching and healing a dead entity, or one that is grossly out of range (EndTouch() can be flakey).
		float flDistSqr = ( m_hTouchTrigger->WorldSpaceCenter() - pOther->WorldSpaceCenter() ).LengthSqr();
		Vector vecMins, vecMaxs;
		m_hTouchTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
		float flDoubleRadiusSqr = ( vecMaxs - vecMins ).LengthSqr();
		if ( !pOther->IsAlive() || flDistSqr > flDoubleRadiusSqr )
		{
			m_hTouchingEntities.FindAndRemove( pOther );
			StopHealing( pOther );
			continue;
		}
		
		bHealingTarget = IsHealingTarget( pOther );
		bValidHealTarget = CouldHealTarget( pOther );
		if ( bHealingTarget && !bValidHealTarget )
		{
			// If we can't see them, remove them from healing list
			// does nothing if we are not healing them already.
			StopHealing( pOther );
		}
		else if ( !bHealingTarget && bValidHealTarget )
		{
			// If we can see them, add to healing list.
			// Does nothing if we are healing them already.
			StartHealing( pOther );
		}
	}
}


void CObjectDispenser::ResetHealingTargets( void )
{
	// For each player in touching list.
	for ( int i = m_hHealingTargets.Count() - 1; i >= 0 ; i-- )
	{
		StopHealing( m_hHealingTargets[i].Get() );
	}
}


void CObjectDispenser::StartTriggerTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	// Add to touching entities.
	EHANDLE hOther = pOther;
	m_hTouchingEntities.AddToTail( hOther );

	if ( !IsBuilding() && ( !IsDisabled() && !IsRedeploying() ) && ( CouldHealTarget( pOther ) && !IsHealingTarget( pOther ) ) )
	{
		// Try to start healing them.
		StartHealing( pOther );
	}
}


void CObjectDispenser::Touch( CBaseEntity *pOther )
{
	// We dont want to touch these.
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}
}


void CObjectDispenser::EndTriggerTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	// Remove from touching entities.
	EHANDLE hOther = pOther;
	m_hTouchingEntities.FindAndRemove( hOther );

	// Remove from healing list.
	StopHealing( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Try to start healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StartHealing( CBaseEntity *pOther )
{
	AddHealingTarget( pOther );

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		// No crit-heals
		pPlayer->m_Shared.Heal( GetBuilder(), GetHealRate(), this, false, false );


		/* YOU CANT HAVE EVENT NAMES OVER 32 CHARACTERS
			ALSO NOBODY IS LISTENING TO THIS EVENT ANYWAY
			-SAPPHO
		IGameEvent* event = gameeventmanager->CreateEvent("player_start_healing_by_dispenser");

		if (event)
		{
			event->SetInt("userid", pPlayer->GetUserID());

			gameeventmanager->FireEvent(event);
		}
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StopHealing( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	EHANDLE hOther = pOther;
	if ( m_hHealingTargets.FindAndRemove( hOther ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer )
		{
			pPlayer->m_Shared.StopHealing( GetBuilder(), HEALER_TYPE_BEAM );
		}

		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid heal target? and not already healing them?
//-----------------------------------------------------------------------------
bool CObjectDispenser::CouldHealTarget( CBaseEntity *pTarget )
{
	if ( !pTarget )
		return false;

	if ( !HasSpawnFlags( SF_IGNORE_LOS ) && !pTarget->FVisible( this, MASK_BLOCKLOS ) )
		return false;

	if ( pTarget->IsPlayer() && pTarget->IsAlive() )
	{
		if (tf2c_building_sharing.GetBool() && !(GetObjectFlags() & OF_IS_CART_OBJECT))
			return true;

		// Don't heal enemies unless they are disguised as our team.
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
		if ( pTFPlayer )
		{
			int iTeam = GetTeamNumber();
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( iPlayerTeam != iTeam && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				iPlayerTeam = pTFPlayer->m_Shared.GetTrueDisguiseTeam();
				if( iPlayerTeam == TF_TEAM_GLOBAL )
					iPlayerTeam = iTeam;
			}

			bool bStealthed = pTFPlayer->m_Shared.IsStealthed();
			if( iPlayerTeam != iTeam && !bStealthed )
				return false;

			if ( HasSpawnFlags( SF_NO_DISGUISED_SPY_HEALING ) )
			{
				// If they're invis, no heals.
				if ( bStealthed )
					return false;

				// If they're disguised as enemy.
				if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) &&
					!pTFPlayer->m_Shared.DisguiseFoolsTeam( GetTeamNumber() ) )
					return false;
			}
		}

		return true;
	}

	return false;
}


void CObjectDispenser::AddHealingTarget( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	EHANDLE hOther = pOther;
	m_hHealingTargets.AddToTail( hOther );
	NetworkStateChanged();
}


void CObjectDispenser::RemoveHealingTarget( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	EHANDLE hOther = pOther;
	m_hHealingTargets.FindAndRemove( hOther );
}

//-----------------------------------------------------------------------------
// Purpose: Are we healing this target already
//-----------------------------------------------------------------------------
bool CObjectDispenser::IsHealingTarget( CBaseEntity *pTarget )
{
	if ( !pTarget )
		return false;

	EHANDLE hOther = pTarget;
	return m_hHealingTargets.HasElement( hOther );
}


int CObjectDispenser::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT ) 
	{
		char tempstr[512];
		V_sprintf_safe( tempstr, "Metal: %d", m_iAmmoMetal.Get() );
		EntityText( text_offset,tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}


IMPLEMENT_SERVERCLASS_ST( CObjectCartDispenser, DT_ObjectCartDispenser )
END_SEND_TABLE()

BEGIN_DATADESC( CObjectCartDispenser )
	DEFINE_KEYFIELD( m_szTriggerName, FIELD_STRING, "touch_trigger" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDispenserLevel", InputSetDispenserLevel ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( mapobj_cart_dispenser, CObjectCartDispenser );


void CObjectCartDispenser::Spawn( void )
{
	SetObjectFlags( OF_IS_CART_OBJECT );

	m_takedamage = DAMAGE_NO;

	m_iUpgradeLevel = 1;
	m_iUpgradeMetal = 0;

	AddFlag( FL_OBJECT ); 

	m_iAmmoMetal = 0;
}


void CObjectCartDispenser::SetModel( const char *pModel )
{
	// Deliberately skip dispenser since it has some stuff we don't want.
	CBaseObject::SetModel( pModel );
}


void CObjectCartDispenser::OnGoActive( void )
{
	// Hacky: Base class needs a model to init some things properly so we gotta clear it here.
	BaseClass::OnGoActive();
	SetModel( "" );
}


void CObjectCartDispenser::InputSetDispenserLevel( inputdata_t &inputdata )
{
	int iLevel = inputdata.value.Int();
	if ( iLevel >= 1 && iLevel <= 3 )
	{
		m_iUpgradeLevel = iLevel;
	}
}


void CObjectCartDispenser::InputEnable( inputdata_t &input )
{
	SetDisabled( false );
}


void CObjectCartDispenser::InputDisable( inputdata_t &input )
{
	SetDisabled( true );
}

IMPLEMENT_SERVERCLASS_ST(CObjectMiniDispenser, DT_ObjectMiniDispenser)
SendPropBool(SENDINFO(m_bHealingTargetsParity)),
SendPropInt(SENDINFO(m_iAmmoMetal), 10),
SendPropArray2(
	SendProxyArrayLength_HealingArray,
	SendPropInt("healing_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_HealingList),
	MAX_PLAYERS,
	0,
	"healing_array"
)
END_SEND_TABLE()

BEGIN_DATADESC(CObjectMiniDispenser)
DEFINE_THINKFUNC(DispenseThink),

DEFINE_KEYFIELD(m_szTriggerName, FIELD_STRING, "touch_trigger"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(obj_minidispenser, CObjectMiniDispenser);
PRECACHE_REGISTER(obj_minidispenser);

CObjectMiniDispenser::CObjectMiniDispenser()
{
	m_bHealingTargetsParity = false;

	UseClientSideAnimation();

	SetMaxHealth(100);
	m_iHealth = 100;
	m_flRadius = 0.0f;

	SetType(OBJ_MINIDISPENSER);
	m_hTouchingEntities.Purge();
	m_hTouchTrigger = NULL;
}

CObjectMiniDispenser::~CObjectMiniDispenser()
{
	if (m_hTouchTrigger.Get())
	{
		UTIL_Remove(m_hTouchTrigger);
	}

	ResetHealingTargets();

	StopSound("Building_Dispenser.Idle");
}

float CObjectMiniDispenser::GetDispenserRadius(void)
{
	float flRadius = 192.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetBuilder(), flRadius, mult_dispenser_radius);
	return flRadius;
}

float CObjectMiniDispenser::GetHealRate(void)
{
	// level 3
	return g_flDispenserHealRates[ 2 ];
}

int CObjectMiniDispenser::GetBaseHealth(void) const
{
	return 90;
}

void CObjectMiniDispenser::OnGoActive(void)
{
	SetModel(GetDispenserModelForLevel(1));
	CreateBuildPoints();
	ReattachChildren();

	// Begin thinking
	SetContextThink(&CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1f, DISPENSE_CONTEXT);

	m_flNextAmmoDispense = gpGlobals->curtime + 0.5f;

	if (m_hTouchTrigger.Get() && dynamic_cast<CObjectCartDispenser*>(this))
	{
		UTIL_Remove(m_hTouchTrigger.Get());
		m_hTouchTrigger = NULL;
	}

	if (m_szTriggerName != NULL_STRING)
	{
		m_hTouchTrigger = dynamic_cast<CDispenserTouchTrigger*>(gEntList.FindEntityByName(NULL, m_szTriggerName));
		if (m_hTouchTrigger.Get())
		{
			m_hTouchTrigger->SetOwnerEntity(this);
		}
	}

	if (!m_hTouchTrigger.Get())
	{
		m_hTouchTrigger = CBaseEntity::Create("dispenser_touch_trigger", GetAbsOrigin(), vec3_angle, this);
		if (m_hTouchTrigger.Get())
		{
			// Remember the radius.
			m_flRadius = GetDispenserRadius();
			UTIL_SetSize(m_hTouchTrigger, Vector(-m_flRadius), Vector(m_flRadius));
			m_hTouchTrigger->SetSolid(SOLID_BBOX);
		}
	}

	BaseClass::BaseClass::OnGoActive();

	EmitSound("Building_Dispenser.Idle");
}

void CObjectMiniDispenser::MakeCarriedObject(CTFPlayer* pPlayer)
{
	StopSound("Building_Dispenser.Idle");

	// Remove our healing trigger.
	if (m_hTouchTrigger.Get())
	{
		UTIL_Remove(m_hTouchTrigger);
		m_hTouchTrigger = NULL;
	}

	// Stop healing everyone.
	ResetHealingTargets();

	m_hTouchingEntities.Purge();

	// Stop all thinking, we'll resume it once we get re-deployed.
	SetContextThink(NULL, 0, DISPENSE_CONTEXT);

	BaseClass::BaseClass::MakeCarriedObject(pPlayer);
}

bool CObjectMiniDispenser::DispenseAmmo(CTFPlayer* pPlayer)
{
	return false;
}

const char* CObjectMiniDispenser::GetPlacementModel(void) const
{
	return "models/buildables/mini_dispenser_blueprint.mdl";
}


const char* CObjectMiniDispenser::GetDispenserModelForLevel(int level) const
{
	return "models/buildables/mini_dispenser_light.mdl";
}

const char* CObjectMiniDispenser::GetDispenserUpgradeModelForLevel(int level) const
{
	return "models/buildables/mini_dispenser.mdl";
}