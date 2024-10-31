//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Slowly damages the object it's attached to
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_sapper.h"
#include "ndebugoverlay.h"
#include "tf_gamestats.h"

// ------------------------------------------------------------------------ //

#define SAPPER_MINS				Vector( 0, 0, 0 )
#define SAPPER_MAXS				Vector( 1, 1, 1 )

#define SAPPER_MODEL						"models/buildables/sapper_placed.mdl"
#define SAPPER_MODEL_SENTRY_1				"models/buildables/sapper_sentry1.mdl"
#define SAPPER_MODEL_SENTRY_2				"models/buildables/sapper_sentry2.mdl"
#define SAPPER_MODEL_SENTRY_3				"models/buildables/sapper_sentry3.mdl"
#define SAPPER_MODEL_TELEPORTER				"models/buildables/sapper_teleporter.mdl"
#define SAPPER_MODEL_DISPENSER				"models/buildables/sapper_dispenser.mdl"

#define SAPPER_MODEL_PLACEMENT				"models/buildables/sapper_placement.mdl"
#define SAPPER_MODEL_SENTRY_1_PLACEMENT		"models/buildables/sapper_placement_sentry1.mdl"
#define SAPPER_MODEL_SENTRY_2_PLACEMENT		"models/buildables/sapper_placement_sentry2.mdl"
#define SAPPER_MODEL_SENTRY_3_PLACEMENT		"models/buildables/sapper_placement_sentry3.mdl"
#define SAPPER_MODEL_TELEPORTER_PLACEMENT	"models/buildables/sapper_placement_teleporter.mdl"
#define SAPPER_MODEL_DISPENSER_PLACEMENT	"models/buildables/sapper_placement_dispenser.mdl"

BEGIN_DATADESC( CObjectSapper )
	DEFINE_THINKFUNC( SapperThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CObjectSapper, DT_ObjectSapper )
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS( obj_attachment_sapper, CObjectSapper );
PRECACHE_REGISTER( obj_attachment_sapper );

ConVar	obj_sapper_health( "obj_sapper_health", "100", FCVAR_NONE, "Sapper health" );
ConVar	obj_sapper_amount( "obj_sapper_amount", "25", FCVAR_NONE, "Amount of health inflicted by a Sapper object per second" );

#define SAPPER_THINK_CONTEXT		"SapperThink"


CObjectSapper::CObjectSapper()
{
	m_iHealth = GetBaseHealth();
	SetMaxHealth( GetBaseHealth() );

	UseClientSideAnimation();
}


void CObjectSapper::UpdateOnRemove()
{
	StopSound( "Weapon_Sapper.Timer" );

	if( GetBuilder() )
	{
		GetBuilder()->OnSapperFinished( m_flSappingStartTime );
	}

	BaseClass::UpdateOnRemove();
}


void CObjectSapper::Spawn()
{
	SetModel( GetSapperModelName( SAPPER_MODE_PLACEMENT ) );

	m_takedamage = DAMAGE_YES;
	m_iHealth = GetBaseHealth();

	SetType( OBJ_ATTACHMENT_SAPPER );

	BaseClass::Spawn();

	Vector mins = SAPPER_MINS;
	Vector maxs = SAPPER_MAXS;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

    m_fObjectFlags.Set( m_fObjectFlags | OF_ALLOW_REPEAT_PLACEMENT );

	SetSolid( SOLID_NONE );
}


void CObjectSapper::Precache()
{
	PrecacheGibsForModel( PrecacheModel( GetSapperModelName( SAPPER_MODE_PLACED ) ) );

	PrecacheModel( GetSapperModelName( SAPPER_MODE_PLACEMENT ) );

	PrecacheScriptSound( "Weapon_Sapper.Plant" );
	PrecacheScriptSound( "Weapon_Sapper.Timer" );

	BaseClass::Precache();
}


void CObjectSapper::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	CBaseObject *pParentObject = GetParentObject();
	if ( pParentObject )
	{
		pParentObject->OnAddSapper();

		IGameEvent* event = gameeventmanager->CreateEvent("player_sapped_object");
		if (event)
		{
			if (GetBuilder())
				event->SetInt("userid", GetBuilder()->GetUserID());
			if (pParentObject->GetBuilder())
				event->SetInt("ownerid", pParentObject->GetBuilder()->GetUserID());
			event->SetInt("object", pParentObject->ObjectType());
			event->SetInt("sapperid", entindex());

			gameeventmanager->FireEvent(event);
		}
	}

	EmitSound( "Weapon_Sapper.Plant" );

	// Start looping "Weapon_Sapper.Timer", killed when we die.
	EmitSound( "Weapon_Sapper.Timer" );

	if( GetBuilder() )
	{
		m_flSappingStartTime = gpGlobals->curtime;
		GetBuilder()->OnSapperStarted( m_flSappingStartTime );
	}

	m_flSapperDamageAccumulator = 0.0f;
	m_flLastThinkTime = gpGlobals->curtime;

	SetContextThink( &CObjectSapper::SapperThink, gpGlobals->curtime + 0.1f, SAPPER_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: Change our model based on the object we are attaching to.
//-----------------------------------------------------------------------------
void CObjectSapper::SetupAttachedVersion( void )
{
	CBaseObject *pObject = dynamic_cast<CBaseObject *>( m_hBuiltOnEntity.Get() );
	Assert( pObject );
	if ( !pObject )
	{
		UTIL_Remove( this );
		return;
	}

	if ( IsPlacing() )
	{
		SetModel( GetSapperModelName( SAPPER_MODE_PLACEMENT ) );
	}	

	BaseClass::SetupAttachedVersion();
}


void CObjectSapper::OnGoActive( void )
{
	// Set new model.
	CBaseObject *pObject = dynamic_cast<CBaseObject *>( m_hBuiltOnEntity.Get() );
	Assert( pObject );
	if ( !pObject )
	{
		UTIL_Remove( this );
		return;
	}

	SetModel( GetSapperModelName( SAPPER_MODE_PLACED ) );

	UTIL_SetSize( this, SAPPER_MINS, SAPPER_MAXS );
	SetSolid( SOLID_NONE );

	BaseClass::OnGoActive();
}

const char *CObjectSapper::GetSapperModelName( SapperMode_t iMode )
{
	// Live gets builder model here for sapper model overrides.
	if ( iMode == SAPPER_MODE_PLACEMENT )
		return SAPPER_MODEL_PLACEMENT;

	return SAPPER_MODEL;
}


void CObjectSapper::DetachObjectFromObject( void )
{
	CBaseObject *pParentObject = GetParentObject();
	if ( pParentObject )
	{
		pParentObject->OnRemoveSapper();
	}

	BaseClass::DetachObjectFromObject();
}

//-----------------------------------------------------------------------------
// Purpose: Slowly destroy the object I'm attached to
//-----------------------------------------------------------------------------
void CObjectSapper::SapperThink( void )
{
	if ( !GetTeam() )
		return;

	CBaseObject *pObject = GetParentObject();
	if ( !pObject )
	{
		UTIL_Remove( this );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1f, SAPPER_THINK_CONTEXT );

	// Don't bring objects back from the dead.
	if ( !pObject->IsAlive() || pObject->IsDying() )
		return;

	// How much damage to give this think?
	float flTimeSinceLastThink = gpGlobals->curtime - m_flLastThinkTime;
	float flDamageToGive = ( flTimeSinceLastThink ) * obj_sapper_amount.GetFloat();
	CTFPlayer *pBuilder = GetBuilder();
	if ( pBuilder )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flDamageToGive, mult_sapper_damage );
	}

	// Add to accumulator.
	m_flSapperDamageAccumulator += flDamageToGive;

	int iDamage = (int)m_flSapperDamageAccumulator;
	m_flSapperDamageAccumulator -= iDamage;
	CTakeDamageInfo info( this, this, iDamage, DMG_CRUSH );

	pObject->TakeDamage( info );

	// Sapper leech health attribute
	float flSapperDamageHeal = 0.0f;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pBuilder, flSapperDamageHeal, sapper_damage_leaches_health );
	if ( flSapperDamageHeal )
	{
		CTFPlayer *pBuilder = GetBuilder();
		if ( pBuilder )
		{
			if ( pBuilder->IsAlive() )
			{
				pBuilder->TakeHealth( flSapperDamageHeal * 0.1f, HEAL_NOTIFY );
			}
		}
	}

	// Limited duration Sapper attribute
	int iSapperSelfDamage = 0.0f;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pBuilder, iSapperSelfDamage, sapper_damages_self );
	if ( iSapperSelfDamage )
	{
		m_iHealth -= iSapperSelfDamage;
		if ( m_iHealth <= 0 )
		{
			this->DetonateObject();
		}
	}

	// Sappers outline affected buildings
	pObject->OutlineForEnemy( 0.5f );

	m_flLastThinkTime = gpGlobals->curtime;
}


int CObjectSapper::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamageCustom() != TF_DMG_WRENCH_FIX )
		return 0;

	return BaseClass::OnTakeDamage( info );
}


void CObjectSapper::Killed( const CTakeDamageInfo &info )
{
	// If the sapper is removed by someone other than builder, award bonus points.
	CTFPlayer *pScorer = ToTFPlayer( info.GetAttacker() );
	if ( pScorer )
	{
		CBaseObject *pObject = GetParentObject();
		if ( pObject && pScorer != pObject->GetBuilder() )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pScorer, this, 1 );
		}
	}

	BaseClass::Killed( info );
}


int CObjectSapper::GetBaseHealth( void ) const
{
	int iBaseHealth = obj_sapper_health.GetInt();
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetBuilder(), iBaseHealth, mult_sapper_health );
	return iBaseHealth;
}
