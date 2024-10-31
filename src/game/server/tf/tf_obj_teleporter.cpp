//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleporter Object
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_teleporter.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "world.h"
#include "explode.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "tf_fx.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_bot.h"
#include "props.h"
#include "IEffects.h"

#include "tf_weapon_pda.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define TELEPORTER_MODEL_ENTRANCE_PLACEMENT	"models/buildables/teleporter_blueprint_enter.mdl"
#define TELEPORTER_MODEL_EXIT_PLACEMENT		"models/buildables/teleporter_blueprint_exit.mdl"
#define TELEPORTER_MODEL_BUILDING			"models/buildables/teleporter.mdl"
#define TELEPORTER_MODEL_LIGHT				"models/buildables/teleporter_light.mdl"

#define TELEPORTER_MINS			Vector( -24, -24, 0 )
#define TELEPORTER_MAXS			Vector( 24, 24, 12 )	

IMPLEMENT_SERVERCLASS_ST( CObjectTeleporter, DT_ObjectTeleporter )
	SendPropInt( SENDINFO( m_iState ), 5 ),
	SendPropTime( SENDINFO( m_flRechargeTime ) ),
	SendPropInt( SENDINFO( m_iTimesUsed ), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flYawToExit ), 8, 0, -180.0f, 180.0f ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectTeleporter )
	DEFINE_KEYFIELD( m_iTeleporterType, FIELD_INTEGER, "teleporterType" ),
	DEFINE_KEYFIELD( m_szMatchingTeleporterName, FIELD_STRING, "matchingTeleporter" ),
	DEFINE_THINKFUNC( TeleporterThink ),
	DEFINE_ENTITYFUNC( TeleporterTouch ),
END_DATADESC()

PRECACHE_REGISTER( obj_teleporter );

#define TELEPORTER_MAX_HEALTH			150

#define TELEPORTER_THINK_CONTEXT		"TeleporterContext"

#define TELEPORTER_FADEOUT_TIME			0.25f	// Time to teleport a player out (teleporter with full health).
#define TELEPORTER_FADEIN_TIME			0.25f	// Time to teleport a player in (teleporter with full health).

#define TELEPORTER_NEXT_THINK			0.05f

#define TELEPORTER_PLAYER_OFFSET		20		// How far above the origin of the teleporter to place a player.

#define TELEPORTER_EFFECT_TIME			12.0f	// Seconds that player glows after teleporting.

ConVar tf_teleporter_fov_start( "tf_teleporter_fov_start", "120", FCVAR_CHEAT, "Starting FOV for teleporter zoom.", true, 1, false, 0 );
ConVar tf_teleporter_fov_time( "tf_teleporter_fov_time", "0.5", FCVAR_CHEAT, "How quickly to restore FOV after teleport.", true, 0.0, false, 0 );

extern ConVar tf2c_building_gun_mettle;

LINK_ENTITY_TO_CLASS( obj_teleporter, CObjectTeleporter );


CObjectTeleporter::CObjectTeleporter()
{
	UseClientSideAnimation();

	SetMaxHealth( TELEPORTER_MAX_HEALTH );
	m_iHealth = TELEPORTER_MAX_HEALTH;

	SetType( OBJ_TELEPORTER );
	m_iTeleporterType = 0;
}


CObjectTeleporter::~CObjectTeleporter()
{
	CObjectTeleporter *pMatch = GetMatchingTeleporter();
	if ( pMatch && !IsBeingPlaced() )
	{
		// Reset our matching teleporter just incase.
		pMatch->FactoryReset();
	}
}


void CObjectTeleporter::Spawn()
{
	// Only used by teleporters placed in hammer.
	if ( m_iTeleporterType == 1 )
	{
		SetObjectMode( TELEPORTER_TYPE_ENTRANCE );
	}
	else if ( m_iTeleporterType == 2 )
	{
		SetObjectMode( TELEPORTER_TYPE_EXIT );
	}

	SetSolid( SOLID_BBOX );

	m_takedamage = DAMAGE_NO;

	SetState( TELEPORTER_STATE_BUILDING );

	m_flNextEnemyTouchHint = gpGlobals->curtime;

	m_flYawToExit = 0.0f;

	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
	{
		SetModel( TELEPORTER_MODEL_ENTRANCE_PLACEMENT );
	}
	else
	{
		SetModel( TELEPORTER_MODEL_EXIT_PLACEMENT );
	}

	BaseClass::Spawn();
}


void CObjectTeleporter::FirstSpawn()
{
	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	m_bMatchingTeleporterDown = true;
	
	BaseClass::FirstSpawn();

	FindMatchAndCopyStaticStats();
}

void CObjectTeleporter::MakeCarriedObject( CTFPlayer *pPlayer )
{
	SetState( TELEPORTER_STATE_BUILDING );

	// Stop thinking.
	SetContextThink( NULL, 0, TELEPORTER_THINK_CONTEXT );
	SetTouch( NULL );

	ShowDirectionArrow( false );

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;

	BaseClass::MakeCarriedObject( pPlayer );
}

//-----------------------------------------------------------------------------
// Receive a teleporting player 
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterReceive( CTFPlayer *pPlayer, float flDelay )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = GetTeamNumber();

	const char *pszEffectName = ConstructTeamParticle( "teleportedin_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0f, pszEffectName, origin, vec3_angle );

	EmitSound( "Building_Teleporter.Receive" );

	SetState( TELEPORTER_STATE_RECEIVING );
	m_flMyNextThink = gpGlobals->curtime + TELEPORTER_FADEOUT_TIME;

	m_iTimesUsed++;
}

//-----------------------------------------------------------------------------
// Teleport the passed player to our destination
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterSend( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );
	pPlayer->m_Shared.AddCond( TF_COND_SELECTED_TO_TELEPORT );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = GetTeamNumber();

	const char *pszTeleportedEffect = ConstructTeamParticle( "teleported_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0f, pszTeleportedEffect, origin, vec3_angle );

	const char *pszSparklesEffect = ConstructTeamParticle( "player_sparkles_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0f, pszSparklesEffect, PATTACH_ABSORIGIN, pPlayer );

	EmitSound( "Building_Teleporter.Send" );

	SetState( TELEPORTER_STATE_SENDING );
	m_flMyNextThink = gpGlobals->curtime + 0.1f;

	m_iTimesUsed++;
}

//-----------------------------------------------------------------------------
// Purpose: Resets a teleporter to factory default.
//-----------------------------------------------------------------------------
void CObjectTeleporter::FactoryReset( void )
{
	bool bGreaterUpgrade = ( m_iUpgradeLevel > 1 || m_iTargetUpgradeLevel > 1 );
	int iOldMaxHealth = 1;

	if ( bGreaterUpgrade )
	{
		iOldMaxHealth = GetMaxHealthForCurrentLevel();
	}

	m_iUpgradeLevel = 1;
	m_iTargetUpgradeLevel = 1;
	m_iUpgradeMetal = 0;

	if ( bGreaterUpgrade )
	{
		// We need to adjust for any damage received if we downgraded.
		int iNewHealth = AdjustHealthForLevel( iOldMaxHealth );
		if ( m_iTargetHealth > iNewHealth )
		{
			m_iTargetHealth = iNewHealth;
		}
	}
}


void CObjectTeleporter::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );

	// Reset this after model change
	UTIL_SetSize( this, TELEPORTER_MINS, TELEPORTER_MAXS );

	CreateBuildPoints();

	ReattachChildren();

	m_iDirectionBodygroup = FindBodygroupByName( "teleporter_direction" );
	m_iBlurBodygroup = FindBodygroupByName( "teleporter_blur" );

	if ( m_iBlurBodygroup >= 0 )
	{
		SetBodygroup( m_iBlurBodygroup, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectTeleporter::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( TELEPORTER_MODEL_BUILDING );

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsPlacementPosValid( void )
{
	bool bResult = BaseClass::IsPlacementPosValid();
	if ( !bResult )
		return false;

	// Start above the teleporter position.
	Vector vecPosition, vecMin, vecMax;
	GetTeleportBounds( vecPosition, vecMin, vecMax );

	// Make sure we can fit a player on top in this position.
	trace_t tr;
	UTIL_TraceHull( vecPosition, vecPosition, vecMin, vecMax, MASK_SOLID | CONTENTS_PLAYERCLIP, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	return tr.fraction == 1.0f;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::StartUpgrading( void )
{
	BaseClass::StartUpgrading();

	SetState( TELEPORTER_STATE_UPGRADING );
}

void CObjectTeleporter::FinishUpgrading( void )
{
	SetState( TELEPORTER_STATE_IDLE );

	m_flRechargeTime = gpGlobals->curtime;

	BaseClass::FinishUpgrading();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::OnGoActive( void )
{
	SetModel( TELEPORTER_MODEL_LIGHT );
	SetActivity( ACT_OBJ_IDLE );

	SetContextThink( &CObjectTeleporter::TeleporterThink, gpGlobals->curtime + 0.1f, TELEPORTER_THINK_CONTEXT );
	SetTouch( &CObjectTeleporter::TeleporterTouch );

	SetState( TELEPORTER_STATE_IDLE );

	BaseClass::OnGoActive();

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;	// Used as a flag to initialize the playback rate to 0 in the first DeterminePlaybackRate.
}


void CObjectTeleporter::Precache()
{
	BaseClass::Precache();

	// Precache Object Models.
	PrecacheModel( TELEPORTER_MODEL_ENTRANCE_PLACEMENT );
	PrecacheModel( TELEPORTER_MODEL_EXIT_PLACEMENT );

	PrecacheGibsForModel( PrecacheModel( TELEPORTER_MODEL_BUILDING ) );
	PrecacheGibsForModel( PrecacheModel( TELEPORTER_MODEL_LIGHT ) );

	// Precache Sounds.
	PrecacheScriptSound( "Building_Teleporter.Ready" );
	PrecacheScriptSound( "Building_Teleporter.Send" );
	PrecacheScriptSound( "Building_Teleporter.Receive" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel1" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel2" );
	PrecacheScriptSound( "Building_Teleporter.SpinLevel3" );

	char szEffect[128];
	for (int i = 1, c = GetObjectInfo(GetType())->m_MaxUpgradeLevel; i <= c; i++)
	{
		V_sprintf_safe( szEffect, "teleporter_%%s_charged_level%d", i );
		PrecacheTeamParticles( szEffect );

		V_sprintf_safe( szEffect, "teleporter_%%s_entrance_level%d", i );
		PrecacheTeamParticles( szEffect );

		V_sprintf_safe( szEffect, "teleporter_%%s_exit_level%d", i );
		PrecacheTeamParticles( szEffect );
	}

	PrecacheTeamParticles( "teleporter_arms_circle_%s" );
	PrecacheTeamParticles( "teleported_%s" );
	PrecacheTeamParticles( "teleportedin_%s" );
	PrecacheTeamParticles( "player_sparkles_%s" );
	PrecacheParticleSystem( "tpdamage_1" );
	PrecacheParticleSystem( "tpdamage_2" );
	PrecacheParticleSystem( "tpdamage_3" );
	PrecacheParticleSystem( "tpdamage_4" );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterTouch( CBaseEntity *pOther )
{
	if ( IsDisabled() )
		return;

	// If it's not a player, ignore.
	if ( !pOther->IsPlayer() )
		return;

	// Don't teleport them if they're not the first in queue or not in the queue at all.
	// This ensures that humans take the tele in the right order and
	// bots don't accidentally take tele they don't want to take.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !IsFirstInQueue( pPlayer ) )
		return;

	int bTwoWayTeleporter = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, bTwoWayTeleporter, bidirectional_teleport );

	// Is this an entrance and do we have an exit?
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE || bTwoWayTeleporter > 0 )
	{
		if ( m_iState == TELEPORTER_STATE_READY )
		{
			// Are we able to teleport?
			if ( !PlayerCanBeTeleported( pPlayer ) )
			{
				if ( pPlayer->HasTheFlag() && pPlayer->GetTeamNumber() == this->GetTeamNumber() )
				{
					// If they have the flag, print a warning that you can't tele with the flag.
					CSingleUserRecipientFilter filter( pPlayer );
					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_TELE_WITH_FLAG );
				}

				return;
			}

			// Get the velocity of the player touching the teleporter.
			if ( pPlayer->GetAbsVelocity().Length() < ( pPlayer->IsPlayerUnderwater() ? 10.0f : 5.0f ) )
			{
				CObjectTeleporter *pDestination = GetMatchingTeleporter();
				if ( pDestination )
				{
					TeleporterSend( pPlayer );
				}
			}
		}
	}
}


void CObjectTeleporter::StartTouch( CBaseEntity *pOther )
{
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer && !pPlayer->IsBotOfType( CTFBot::BOT_TYPE ) )
		{
			// Add human player to the top of the queue.
			AddPlayerToQueue( pPlayer, !pPlayer->IsPlayerClass( TF_CLASS_SCOUT ) );
		}
	}
}


void CObjectTeleporter::EndTouch( CBaseEntity *pOther )
{
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer && !pPlayer->IsBotOfType( CTFBot::BOT_TYPE ) )
		{
			RemovePlayerFromQueue( pPlayer );
		}
	}
}


bool CObjectTeleporter::IsReady( void )
{
	return ( IsMatchingTeleporterReady() && GetState() != TELEPORTER_STATE_BUILDING && !IsUpgrading() && !IsDisabled() && !IsBeingCarried() );
}

//-----------------------------------------------------------------------------
// Receive a teleporting player 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsMatchingTeleporterReady( void )
{
	CObjectTeleporter *pMatch = GetMatchingTeleporter();
	if ( pMatch &&
		pMatch->GetState() != TELEPORTER_STATE_BUILDING &&
		!pMatch->IsDisabled() &&
		!pMatch->IsUpgrading() &&
		!pMatch->IsRedeploying() )
		return true;

	return false;
}

SERVERONLY_DLL_EXPORT bool CObjectTeleporter::PlayerCanBeTeleported( CTFPlayer *pSender )
{
	if ( pSender )
	{
		if ( !pSender->HasTheFlag() )
		{
			if (tf2c_building_sharing.GetBool())
				return true;

			// Don't teleport enemies (unless it's a spy).
			int iTeleporterTeam = GetTeamNumber(), iSenderTeam = pSender->GetTeamNumber();

			// New behavior:
			if ( iTeleporterTeam != iSenderTeam )
			{
				if ( pSender->IsPlayerClass( TF_CLASS_SPY, true ) )
				{
					return true;
				}
			}
			else
			{
				return true;
			}

			// Old behavior:
			/*if ( iTeleporterTeam != iSenderTeam && pSender->m_Shared.IsDisguised() )
			{
				iSenderTeam = pSender->m_Shared.GetTrueDisguiseTeam();
				if( iSenderTeam == TF_TEAM_GLOBAL )
					iSenderTeam = iTeleporterTeam;
			}

			if ( iTeleporterTeam == iSenderTeam )
			{
				return true
			}*/
		}
	}

	return false;
}

bool CObjectTeleporter::IsSendingPlayer( CTFPlayer *pSender )
{
	bool bResult = false;

	if ( pSender )
	{
		CTFPlayer *pTeleportingPlayer = m_hTeleportingPlayer.Get();
		if ( pTeleportingPlayer )
		{
			bResult = pTeleportingPlayer == pSender;
		}
	}

	return bResult;
}

void CObjectTeleporter::CopyUpgradeStateToMatch( CObjectTeleporter *pMatch, bool bCopyFrom )
{
	if ( !pMatch )
		return;

	CObjectTeleporter *pObjToCopyFrom = bCopyFrom ? pMatch : this;
	CObjectTeleporter *pObjToCopyTo = bCopyFrom ? this : pMatch;
	pObjToCopyTo->m_iUpgradeMetal = pObjToCopyFrom->m_iUpgradeMetal;
	pObjToCopyTo->m_iUpgradeMetalRequired = pObjToCopyFrom->m_iUpgradeMetalRequired;
	pObjToCopyTo->m_iTargetUpgradeLevel = pObjToCopyFrom->m_iUpgradeLevel;
	pObjToCopyTo->m_iHighestUpgradeLevel = pObjToCopyFrom->m_iHighestUpgradeLevel;

	/**(pObjToCopyTo + 632) = *(this + 632);
	*(pObjToCopyTo + 629) = *(this + 629);
	*(pObjToCopyTo + 630) = *(this + 630);
	*(pObjToCopyTo + 631) = *(this + 631);
	*(pObjToCopyTo + 633) = *(this + 633);
	*(pObjToCopyTo + 634) = *(this + 634);*/
}

bool CObjectTeleporter::CheckUpgradeOnHit( CTFPlayer *pPlayer )
{
	bool bUpgradeSuccesful = false;

	if ( BaseClass::CheckUpgradeOnHit( pPlayer ) )
	{
		CObjectTeleporter *pMatch = GetMatchingTeleporter();
		if ( pMatch )
		{
			//pMatch->m_iUpgradeMetal = m_iUpgradeMetal;
			if ( pMatch && pMatch->CanBeUpgraded( pPlayer ) && GetUpgradeLevel() > pMatch->GetUpgradeLevel() )
			{
				// This end just got upgraded so make another end play upgrade anim if possible.
				pMatch->StartUpgrading();
			}

			// Other end still needs to keep up even while hauled etc.
			CopyUpgradeStateToMatch( pMatch, false );
		}

		bUpgradeSuccesful = true;
	}

	return bUpgradeSuccesful;
}

void CObjectTeleporter::InitializeMapPlacedObject( void )
{
	BaseClass::InitializeMapPlacedObject();

	CObjectTeleporter *pMatch = dynamic_cast<CObjectTeleporter *>( gEntList.FindEntityByName( NULL, m_szMatchingTeleporterName ) );
	if ( pMatch )
	{
		// Copy upgrade state from higher level end.
		bool bCopyFrom = pMatch->GetUpgradeLevel() > GetUpgradeLevel();

		if ( pMatch->GetUpgradeLevel() == GetUpgradeLevel() )
		{
			// If same level use it if it has more metal.
			bCopyFrom = pMatch->m_iUpgradeMetal > m_iUpgradeMetal;
		}

		CopyUpgradeStateToMatch( pMatch, bCopyFrom );

		m_hMatchingTeleporter = pMatch;
	}
}

CObjectTeleporter *CObjectTeleporter::GetMatchingTeleporter( void )
{
	if ( !m_hMatchingTeleporter.Get() )
	{
		m_hMatchingTeleporter = FindMatch();
	}

	return m_hMatchingTeleporter.Get();
}


bool CObjectTeleporter::InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos )
{
	if ( HasSapper() && GetMatchingTeleporter() )
	{
		CObjectTeleporter *pMatch = GetMatchingTeleporter();
		// Do damage to any attached buildings.
		CTakeDamageInfo info( pPlayer, pPlayer, 65, DMG_CLUB, TF_DMG_WRENCH_FIX );

		IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints *>( pMatch );
		int iNumObjects = pBPInterface->GetNumObjectsOnMe();
		for ( int iPoint = 0; iPoint < iNumObjects; iPoint++ )
		{
			CBaseObject *pObject = pMatch->GetBuildPointObject( iPoint );
			if ( pObject && pObject->IsHostileUpgrade() )
			{
				pObject->TakeDamage( info );
				DoWrenchHitEffect( pMatch->WorldSpaceCenter(), true, false );
			}
		}
	}

	return BaseClass::InputWrenchHit( pPlayer, pWrench, vecHitPos );
}

int CObjectTeleporter::OnTakeDamage( const CTakeDamageInfo& info )
{
	// Show sparks on our matching end to warn players it's dangerous to take
	// but not when we already show a Sapper.
	CObjectTeleporter* pMatch = GetMatchingTeleporter();
	if ( pMatch && !HasSapper() )
	{
		//CPVSFilter filter( pMatch->WorldSpaceCenter() );
		//TE_TFParticleEffect( filter, 0.0f, "sparks_metal", pMatch->WorldSpaceCenter(), QAngle( 0, 0, 0 ) );
		g_pEffects->Sparks( pMatch->WorldSpaceCenter(), 2, 2);
	}

	return BaseClass::OnTakeDamage( info );
}

void CObjectTeleporter::DeterminePlaybackRate( void )
{
	float flPlaybackRate = GetPlaybackRate();

	bool bWasBelowFullSpeed = ( flPlaybackRate < 1.0f );

	if ( IsBuilding() )
	{
		// Default half rate, author build anim as if one player is building.
		SetPlaybackRate( GetConstructionMultiplier() * 0.5f );
	}
	else if ( IsPlacing() )
	{
		SetPlaybackRate( 1.0f );
	}
	else
	{
		float flFrameTime = 0.1f; // BaseObjectThink delay.

		switch ( m_iState )
		{
			case TELEPORTER_STATE_READY:
			{
				// Spin up to 1.0 from whatever we're at, at some high rate.
				flPlaybackRate = Approach( 1.0f, flPlaybackRate, 0.5f * flFrameTime );
				break;
			}

			case TELEPORTER_STATE_RECHARGING:
			{
				// Recharge - spin down to low and back up to full speed over 10 seconds

				// 0 -> 4, spin to low
				// 4 -> 6, stay at low
				// 6 -> 10, spin to 1.0

				float flScale = g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1] / g_flTeleporterRechargeTimes[0];

				float flToLow = 4.0f * flScale;
				float flToHigh = 6.0f * flScale;
				float flRechargeTime = g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];

				float flTimeSinceChange = gpGlobals->curtime - m_flLastStateChangeTime;

				float flLowSpinSpeed = 0.15f;

				if ( flTimeSinceChange <= flToLow )
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime,
						m_flLastStateChangeTime + flToLow,
						1.0f,
						flLowSpinSpeed );
				}
				else if ( flTimeSinceChange > flToLow && flTimeSinceChange <= flToHigh )
				{
					flPlaybackRate = flLowSpinSpeed;
				}
				else
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime + flToHigh,
						m_flLastStateChangeTime + flRechargeTime,
						flLowSpinSpeed,
						1.0f );
				}
				break;
			}

			default:
			{
				if ( m_flLastStateChangeTime <= 0.0f )
				{
					flPlaybackRate = 0.0f;
				}
				else
				{
					// Lost connect - spin down to 0.0 from whatever we're at, slowish rate.
					flPlaybackRate = Approach( 0.0f, flPlaybackRate, 0.25f * flFrameTime );
				}
				break;
			}
		}

		SetPlaybackRate( flPlaybackRate );
	}

	bool bBelowFullSpeed = ( GetPlaybackRate() < 1.0f );

	if ( m_iBlurBodygroup >= 0 && bBelowFullSpeed != bWasBelowFullSpeed )
	{
		if ( bBelowFullSpeed )
		{
			// Turn off blur bodygroup.
			SetBodygroup( m_iBlurBodygroup, 0 );
		}
		else
		{
			// Turn on blur bodygroup.
			SetBodygroup( m_iBlurBodygroup, 1 );
		}
	}

	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterThink( void )
{
	SetContextThink( &CObjectTeleporter::TeleporterThink, gpGlobals->curtime + TELEPORTER_NEXT_THINK, TELEPORTER_THINK_CONTEXT );

	// pMatch is not NULL and is not building.
	CObjectTeleporter *pMatch = GetMatchingTeleporter();

	// At any point, if our match is not ready, revert to IDLE.
	if ( IsDisabled() || IsBuilding() || IsRedeploying() || !IsMatchingTeleporterReady() )
	{
		ShowDirectionArrow( false );

		if ( GetState() != TELEPORTER_STATE_IDLE && !IsUpgrading() )
		{
			SetState( TELEPORTER_STATE_IDLE );
		}
		else
		{
			// Keeps this teleporter updated on the other's status.
			FindMatch();
		}
		
		if ( !pMatch )
		{
			if ( !m_bMatchingTeleporterDown )
			{
				// The other end has been destroyed. Revert back to L1.
				FactoryReset();

				m_bMatchingTeleporterDown = true;
			}
		}
		else
		{
			m_bMatchingTeleporterDown = false;
		}

		return;
	}

	if ( m_flMyNextThink && m_flMyNextThink > gpGlobals->curtime )
		return;

	Assert( pMatch );
	Assert( pMatch->m_iState != TELEPORTER_STATE_BUILDING );

	switch ( m_iState )
	{
		// Teleporter is not yet active, do nothing.
		case TELEPORTER_STATE_BUILDING:
		case TELEPORTER_STATE_UPGRADING:
			ShowDirectionArrow( false );
			break;

		default:
		case TELEPORTER_STATE_IDLE:
			// Do we have a match that is active?
			// Make sure both ends wait through full recharge time in case they get upgraded while recharging.
			if ( IsMatchingTeleporterReady() && !IsUpgrading() && gpGlobals->curtime > m_flRechargeTime )
			{
				SetState( TELEPORTER_STATE_READY );
				EmitSound( "Building_Teleporter.Ready" );

				if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
				{
					ShowDirectionArrow( true );
				}
			}
			break;

		case TELEPORTER_STATE_READY:
			break;

		case TELEPORTER_STATE_SENDING:
		{
			pMatch->TeleporterReceive( m_hTeleportingPlayer, 1.0f );

			m_flRechargeTime = gpGlobals->curtime + ( TELEPORTER_FADEOUT_TIME + TELEPORTER_FADEIN_TIME + g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1] );

			// Change state to recharging...
			SetState( TELEPORTER_STATE_RECHARGING );
		}
		break;

		case TELEPORTER_STATE_RECEIVING:
		{
			// Move the player. Make sure he's still ready to teleport.
			CTFPlayer *pTeleportingPlayer = m_hTeleportingPlayer.Get();
			if ( pTeleportingPlayer && pTeleportingPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
			{
				CUtlVector<CBaseEntity *> vecTelefraggedEntities;
				bool bClear = true;

				// Get the position we'll move the player to.
				Vector vecPosition, vecMin, vecMax;
				GetTeleportBounds( vecPosition, vecMin, vecMax );

				// Go through all of the entities in a box, and see if they'll either prevent the teleport, or die.
				CBaseEntity *pEnts[256];
				int i, c = UTIL_EntitiesInBox( pEnts, sizeof( pEnts ), vecPosition + vecMin, vecPosition + vecMax, 0 );
				if ( c )
				{
					for ( i = 0; i < c; i++ )
					{
						if ( !pEnts[i] )
							continue;

						if ( pEnts[i] == this )
							continue;

						// Telefrag enemy players and buildings.
						if ( pEnts[i]->IsPlayer() )
						{
							if ( pEnts[i]->GetTeamNumber() >= FIRST_GAME_TEAM && !pTeleportingPlayer->InSameTeam( pEnts[i] ) )
							{
								vecTelefraggedEntities.AddToTail( pEnts[i] );
							}
							continue;
						}
						if ( pEnts[i]->IsBaseObject() )
						{
							if ( pEnts[i]->GetTeamNumber() >= FIRST_GAME_TEAM && !pTeleportingPlayer->InSameTeam( pEnts[i] ) )
							{
								vecTelefraggedEntities.AddToTail( pEnts[i] );
							}
							else if ( static_cast<CBaseObject *>( pEnts[i] )->GetType() == OBJ_TELEPORTER )
							{
								bClear = false;
							}
							continue;
						}

						// Other solid entities will prevent teleportation.
						if ( pEnts[i]->IsSolid() && pEnts[i]->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), MASK_SOLID ) &&
								g_pGameRules->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), pEnts[i]->GetCollisionGroup() ) )
						{
							// HACK: Solves the problem of building teleporter exits in CDynamicProp entities at
							// the end of maps like Badwater that have the VPhysics explosions when the final point is capped.
							CDynamicProp *pProp = dynamic_cast<CDynamicProp *>( pEnts[i] );
							if ( !pProp )
							{
 								CBaseProjectile *pProjectile = dynamic_cast<CBaseProjectile *>( pEnts[i] );
 								if ( !pProjectile )
 								{
									bClear = false;
								}
							}
							else if ( !pProp->IsEffectActive( EF_NODRAW ) )
							{
								bClear = false;
							}

							// If we're overlapping geometry, check if we're actually overlapping, since we may be overlapping it's bounding box.
							if ( !bClear )
							{
								Ray_t ray;
								ray.Init( vecPosition, vecPosition, vecMin, vecMax );

								trace_t trace;
								enginetrace->ClipRayToEntity( ray, MASK_PLAYERSOLID, pEnts[i], &trace );
								if ( trace.fraction >= 1.0f )
								{
									bClear = true;
								}
							}
						}
					}
				}

				if ( bClear )
				{
					// Now telefrag all entities we've found.
					for ( i = 0, c = vecTelefraggedEntities.Count(); i < c; i++ )
					{
						vecTelefraggedEntities[i]->TakeDamage( CTakeDamageInfo( pTeleportingPlayer, pTeleportingPlayer, 1000, DMG_CRUSH | DMG_ALWAYSGIB, TF_DMG_CUSTOM_TELEFRAG ) );
					}

					// Check if the place we're going to teleport to is actually clear.
					// Telefrag any players and buildings in the way.
					pTeleportingPlayer->Teleport( &vecPosition, &( GetAbsAngles() ), &vec3_origin );
					pTeleportingPlayer->DoAnimationEvent( PLAYERANIMEVENT_SNAP_YAW );

					// Unzoom if we are zoomed!
					CTFWeaponBase *pWeapon = pTeleportingPlayer->GetActiveTFWeapon();
					if ( pWeapon && pWeapon->IsSniperRifle() )
					{
						CTFSniperRifle *pSniperRifle = static_cast<CTFSniperRifle *>( pWeapon );
						if ( pSniperRifle->IsZoomed() )
						{
							// Let the sniper rifle clean up conditions and state.
							pSniperRifle->ToggleZoom();

							// Slam the FOV back down.
							pTeleportingPlayer->SetFOV( pTeleportingPlayer, 0, 0.0f );
						}
					}

					pTeleportingPlayer->SetFOV( pTeleportingPlayer, 0, tf_teleporter_fov_time.GetFloat(), tf_teleporter_fov_start.GetInt() );

					color32 fadeColor = { 255, 255, 255, 100 };
					UTIL_ScreenFade( pTeleportingPlayer, fadeColor, 0.25f, 0.4f, FFADE_IN );
				}
				else
				{
					// We're going to teleport into something solid, abort and destroy this exit.
					DetonateObject();
				}
			}

			SetState( TELEPORTER_STATE_RECEIVING_RELEASE );

			m_flMyNextThink = gpGlobals->curtime + ( TELEPORTER_FADEIN_TIME );
		}
		break;

		case TELEPORTER_STATE_RECEIVING_RELEASE:
		{
			CTFPlayer *pTeleportingPlayer = m_hTeleportingPlayer.Get();
			if ( pTeleportingPlayer )
			{
				CTFPlayer *pBuilder = GetBuilder();
				int iTeam = pBuilder ? pBuilder->GetTeamNumber() : GetTeamNumber();
				pTeleportingPlayer->TeleportEffect( iTeam );
				pTeleportingPlayer->m_Shared.RemoveCond( TF_COND_SELECTED_TO_TELEPORT );

				if ( !m_bWasMapPlaced && pBuilder )
				{
					CTF_GameStats.Event_PlayerUsedTeleport( pBuilder, pTeleportingPlayer );
				}

				IGameEvent *event = gameeventmanager->CreateEvent( "player_teleported" );
				if ( event )
				{
					event->SetInt( "userid", pTeleportingPlayer->GetUserID() );

					if ( pBuilder )
					{
						event->SetInt( "builderid", pBuilder->GetUserID() );
					}

					Vector vecOrigin = GetAbsOrigin();
					Vector vecDestinationOrigin = GetMatchingTeleporter()->GetAbsOrigin();
					Vector vecDifference = Vector( vecOrigin.x - vecDestinationOrigin.x, vecOrigin.y - vecDestinationOrigin.y, vecOrigin.z - vecDestinationOrigin.z );

					float flDist = sqrtf( pow( vecDifference.x, 2 ) + pow( vecDifference.y, 2 ) + pow( vecDifference.z, 2 ) );
					event->SetFloat( "dist", flDist );

					gameeventmanager->FireEvent( event );
				}

				// Don't thank ourselves.
				if ( pTeleportingPlayer != pBuilder )
				{
					pTeleportingPlayer->SpeakConceptIfAllowed( MP_CONCEPT_TELEPORTED );
				}
			}

			// Reset the pointers to the player now that we're done teleporting.
			SetTeleportingPlayer( NULL );
			pMatch->SetTeleportingPlayer( NULL );

			SetState( TELEPORTER_STATE_RECHARGING );

			m_flMyNextThink = gpGlobals->curtime + ( g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1] );
		}
		break;

		case TELEPORTER_STATE_RECHARGING:
			// If we are finished recharging, go active.
			if ( gpGlobals->curtime > m_flRechargeTime )
			{
				SetState( TELEPORTER_STATE_READY );
				EmitSound( "Building_Teleporter.Ready" );
			}
			break;
	}
}

int CObjectTeleporter::GetBaseHealth( void ) const
{
	return 150;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsUpgrading( void ) const
{
	return ( m_iState == TELEPORTER_STATE_UPGRADING );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CObjectTeleporter::GetPlacementModel( void ) const
{
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
		return TELEPORTER_MODEL_ENTRANCE_PLACEMENT;

	return TELEPORTER_MODEL_EXIT_PLACEMENT;
}


void CObjectTeleporter::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	SetActivity( ACT_OBJ_RUNNING );
	SetPlaybackRate( 0.0f );
}

void CObjectTeleporter::SetState( int state )
{
	if ( state != m_iState )
	{
		m_iState = state;
		m_flLastStateChangeTime = gpGlobals->curtime;
	}
}

void CObjectTeleporter::ShowDirectionArrow( bool bShow )
{
	if ( bShow != m_bShowDirectionArrow )
	{
		if ( m_iDirectionBodygroup >= 0 )
		{
			SetBodygroup( m_iDirectionBodygroup, bShow ? 1 : 0 );
		}

		m_bShowDirectionArrow = bShow;

		if ( bShow )
		{
			CObjectTeleporter *pMatch = GetMatchingTeleporter();
			Assert( pMatch );

			Vector vecToOwner = pMatch->GetAbsOrigin() - GetAbsOrigin();
			QAngle angleToExit;
			VectorAngles( vecToOwner, Vector( 0, 0, 1 ), angleToExit );
			angleToExit -= GetAbsAngles();

			// Pose parameter is flipped and backwards, adjust.
			//m_flYawToExit = anglemod( -angleToExit.y + 180.0f );
			m_flYawToExit = AngleNormalize( -angleToExit[YAW] + 180.0f );
			// For whatever reason the original code normalizes angle 0 to 360 while pose param
			// takes angle from -180 to 180. I have no idea how this has been working properly
			// in official TF2 all this time. (Nicknine)
		}
	}
}

int CObjectTeleporter::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[512];

		// Teleporter State
		V_sprintf_safe( tempstr, "State: %d", m_iState.Get() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Recharge Time
		if ( gpGlobals->curtime < m_flRechargeTime )
		{
			float flPercent = ( m_flRechargeTime - gpGlobals->curtime ) / g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];
			V_sprintf_safe( tempstr, "Recharging: %.1f", flPercent );
			EntityText( text_offset, tempstr, 0 );
			text_offset++;
		}
	}

	return text_offset;
}

bool CObjectTeleporter::Command_Repair( CTFPlayer *pActivator, float flRepairMod )
{
	CObjectTeleporter *pMatch = GetMatchingTeleporter();

	// There's got to be a better way a shorter way to mirror repairs and such.
	int iOldHealth = GetHealth(), iMaxHealth = GetMaxHealth();
	if ( iOldHealth < iMaxHealth )
	{
		// Per Metal Usage Ratio
		int iAmountToHeal = Min<float>( 100.0f * flRepairMod, iMaxHealth - RoundFloatToInt( iOldHealth ) );

		// Building Repair
		int iRepairCost;
		float flRepairRatio;
		if ( tf2c_building_gun_mettle.GetBool() )
		{
			iRepairCost = ceil( (float)iAmountToHeal / GUN_METTLE_METAL_RATIO );
			flRepairRatio = GUN_METTLE_METAL_RATIO;
		}
		else
		{
			iRepairCost = ceil( (float)iAmountToHeal * 0.2f );
			flRepairRatio = ORIGINAL_METAL_RATIO;
		}

		TRACE_OBJECT( UTIL_VarArgs( "%0.2f CObjectTeleporter::Command_Repair ( %d / %d / %d / %d ) - cost = %d\n", gpGlobals->curtime, 
			iAmountToHeal,
			iRepairCost,
			iOldHealth,
			iMaxHealth,
			iRepairCost ) );

		if ( iRepairCost > 0 )
		{
			int iMaxRepairCost = pActivator->GetBuildResources();
			if ( iRepairCost > iMaxRepairCost )
			{
				iRepairCost = iMaxRepairCost;
			}

			pActivator->RemoveBuildResources( iRepairCost );

			SetHealth( Min<float>( iMaxHealth, GetFloatHealth() + ( iRepairCost * flRepairRatio ) ) );
			if ( pMatch && pMatch->GetState() != TELEPORTER_STATE_BUILDING && !pMatch->IsUpgrading() )
			{
				iOldHealth = pMatch->GetHealth();

				// TF2C Make it clear this end is being repaired remotely
				CPVSFilter filter( pMatch->WorldSpaceCenter() );
				DoWrenchHitEffect( pMatch->WorldSpaceCenter(), true, false );

				const char* pszParticle = ConstructTeamParticle( "healthgained_%s", GetTeamNumber(), g_aTeamNamesShort );
				TE_TFParticleEffect( filter, 0.0f, pszParticle, pMatch->WorldSpaceCenter(), QAngle( 0, 0, 0 ) );

				pMatch->SetHealth( Min<float>( pMatch->GetMaxHealth(), pMatch->GetFloatHealth() + ( iRepairCost * flRepairRatio ) ) );

				if ( pActivator->GetBuildResources() != iMaxRepairCost || pMatch->GetHealth() != iOldHealth )
					return true;
			}
			else if ( pActivator->GetBuildResources() != iMaxRepairCost || GetHealth() != iOldHealth )
				return true;
		}
	}
	else if ( pMatch ) // See if the other teleporter needs repairing.
	{
		iOldHealth = pMatch->GetHealth(), iMaxHealth = pMatch->GetMaxHealth();
		if ( iOldHealth < iMaxHealth && pMatch->GetState() != TELEPORTER_STATE_BUILDING && !pMatch->IsUpgrading() )
		{
			// Per Metal Usage Ratio
			int iAmountToHeal = Min<float>( 100.0f * flRepairMod, iMaxHealth - RoundFloatToInt( iOldHealth ) );

			// Building Repair
			int iRepairCost;
			float flRepairRatio;
			if ( tf2c_building_gun_mettle.GetBool() )
			{
				iRepairCost = ceil( (float)iAmountToHeal / GUN_METTLE_METAL_RATIO );
				flRepairRatio = GUN_METTLE_METAL_RATIO;
			}
			else
			{
				iRepairCost = ceil( (float)iAmountToHeal * 0.2f );
				flRepairRatio = ORIGINAL_METAL_RATIO;
			}

			TRACE_OBJECT( UTIL_VarArgs( "%0.2f CObjectTeleporter::Command_Repair ( %d / %d / %d / %d ) - cost = %d\n", gpGlobals->curtime, 
				iAmountToHeal,
				iRepairCost,
				iOldHealth,
				iMaxHealth,
				iRepairCost ) );

			if ( iRepairCost > 0 )
			{
				int iMaxRepairCost = pActivator->GetBuildResources();
				if ( iRepairCost > iMaxRepairCost )
				{
					iRepairCost = iMaxRepairCost;
				}

				pActivator->RemoveBuildResources( iRepairCost );

				pMatch->SetHealth( Min<float>( iMaxHealth, iOldHealth + ( iRepairCost * flRepairRatio ) ) );

				// TF2C Make it clear this end is being repaired remotely
				CPVSFilter filter( pMatch->WorldSpaceCenter() );
				DoWrenchHitEffect( pMatch->WorldSpaceCenter(), true, false );

				const char* pszParticle = ConstructTeamParticle( "healthgained_%s", GetTeamNumber(), g_aTeamNamesShort);
				TE_TFParticleEffect( filter, 0.0f, pszParticle, pMatch->WorldSpaceCenter(), QAngle( 0, 0, 0 ) );

				if ( pActivator->GetBuildResources() != iMaxRepairCost || pMatch->GetHealth() != iOldHealth )
					return true;
			}
		}
	}

	return false;
}


CObjectTeleporter *CObjectTeleporter::FindMatch( void )
{
	int iObjMode = GetObjectMode();
	int iOppositeMode = iObjMode == TELEPORTER_TYPE_ENTRANCE ? TELEPORTER_TYPE_EXIT : TELEPORTER_TYPE_ENTRANCE;

	CTFPlayer *pBuilder = GetBuilder();
	if ( !pBuilder )
		return NULL;

	CObjectTeleporter *pMatch = NULL;
	for ( int i = 0, c = pBuilder->GetObjectCount(); i < c; i++ )
	{
		CBaseObject *pObject = pBuilder->GetObject( i );
		if ( pObject && pObject->GetType() == GetType() && pObject->GetObjectMode() == iOppositeMode && !pObject->IsDisabled() )
		{
			pMatch = static_cast<CObjectTeleporter *>( pObject );

			// Copy upgrade state from higher level end.
			bool bCopyFrom = pMatch->m_iTargetUpgradeLevel > m_iTargetUpgradeLevel;
			if ( pMatch->m_iTargetUpgradeLevel == m_iTargetUpgradeLevel )
			{
				// If same level use it if it has more metal.
				bCopyFrom = pMatch->m_iUpgradeMetal > m_iUpgradeMetal;
			}

			CopyUpgradeStateToMatch( pMatch, bCopyFrom );
			break;
		}
	}

	return pMatch;
}


void CObjectTeleporter::FindMatchAndCopyStaticStats(void)
{
	// Copy 'static' stats on spawn even if another teleported is still under construction
	CTFPlayer *pBuilder = GetBuilder();
	if (!pBuilder)
		return;

	int iObjMode = GetObjectMode();
	int iOppositeMode = iObjMode == TELEPORTER_TYPE_ENTRANCE ? TELEPORTER_TYPE_EXIT : TELEPORTER_TYPE_ENTRANCE;
	CObjectTeleporter *pMatch = NULL;
	for (int i = 0, c = pBuilder->GetObjectCount(); i < c; i++)
	{
		CBaseObject *pObject = pBuilder->GetObject(i);
		if (pObject && pObject->GetType() == GetType() && pObject->GetObjectMode() == iOppositeMode)
		{
			pMatch = static_cast<CObjectTeleporter *>(pObject);

			m_iUpgradeMetalRequired = pMatch->m_iUpgradeMetalRequired;
			m_iHighestUpgradeLevel = pMatch->m_iHighestUpgradeLevel;
			break;
		}
	}
}

void CObjectTeleporter::GetTeleportBounds( Vector &vecPos, Vector &vecMins, Vector &vecMaxs )
{
	if ( IsPlacing() )
	{
		vecPos = m_vecBuildOrigin;
		vecPos.z += TELEPORTER_MAXS.z;

		vecMins = VEC_HULL_MIN;
		vecMaxs = VEC_HULL_MAX;
	}
	else
	{
		vecPos = GetAbsOrigin();
		vecPos.z += TELEPORTER_MAXS.z + 1.0f;

		Vector vecExpand = Vector( 4, 4, 4 );

		vecMins = VEC_HULL_MIN - vecExpand;
		vecMaxs = VEC_HULL_MAX + vecExpand;
	}
}


int CObjectTeleporter::CalculateTeamBalanceScore(void)
{
	// TODO: I don't know if I should let FindMatch happen outside the teleporter think function.
	// This check will probably do for now.
	if (m_hMatchingTeleporter &&
		m_hMatchingTeleporter.Get() != NULL &&
		m_hMatchingTeleporter->GetState() != TELEPORTER_STATE_BUILDING &&
		!m_hMatchingTeleporter->IsDisabled())
	{
		// Teleporters that are connected and functional gets a bonus.
		return 45;
	}
	else
	{
		return BaseClass::CalculateTeamBalanceScore();
	}
}

static int g_aClassTeleportPriorities[TF_CLASS_COUNT_ALL] =
{
	99,	// TF_CLASS_UNDEFINED,
	8,	// TF_CLASS_SCOUT,
	7,	// TF_CLASS_SNIPER,
	3,	// TF_CLASS_SOLDIER,
	4,	// TF_CLASS_DEMOMAN,
	1,	// TF_CLASS_MEDIC,
	2,	// TF_CLASS_HEAVYWEAPONS,
	5,	// TF_CLASS_PYRO,
	6,	// TF_CLASS_SPY,
	0,	// TF_CLASS_ENGINEER,

	-1,	// TF_CLASS_CIVILIAN,
};

int TeleporterQueueSort( const CObjectTeleporter::TeleporterQueueEntry_t *p1, const CObjectTeleporter::TeleporterQueueEntry_t *p2 )
{
	// Humans go first.
	if ( !p1->pPlayer->IsBot() && p2->pPlayer->IsBot() && !p1->pPlayer->IsPlayerClass( TF_CLASS_SCOUT, true ) )
		return -1;

	if ( p1->pPlayer->IsBot() && !p2->pPlayer->IsBot() && !p2->pPlayer->IsPlayerClass( TF_CLASS_SCOUT, true ) )
		return 1;

	int prio1 = g_aClassTeleportPriorities[p1->pPlayer->GetPlayerClass()->GetClassIndex()];
	int prio2 = g_aClassTeleportPriorities[p2->pPlayer->GetPlayerClass()->GetClassIndex()];
	int prioDiff = prio1 - prio2;
	if ( prioDiff )
		return prioDiff;

	// Newly joined players go after.
	float timeDiff = p1->flJoinTime - p2->flJoinTime;
	if ( timeDiff < 0.0f )
		return -1;

	if ( timeDiff > 0.0f )
		return 1;

	return ( p1->pPlayer->entindex() - p2->pPlayer->entindex() );
}

void CObjectTeleporter::AddPlayerToQueue( CTFPlayer *pPlayer, bool bAddToHead /*= false*/ )
{
	if ( bAddToHead )
	{
		m_PlayerQueue.AddToHead( { pPlayer, gpGlobals->curtime } );
	}
	else
	{
		m_PlayerQueue.AddToTail( { pPlayer, gpGlobals->curtime } );
	}

	m_PlayerQueue.Sort( TeleporterQueueSort );
}

void CObjectTeleporter::RemovePlayerFromQueue( CTFPlayer *pPlayer )
{
	for ( int i = 0, c = m_PlayerQueue.Count(); i < c; i++ )
	{
		if ( m_PlayerQueue[i].pPlayer == pPlayer )
		{
			m_PlayerQueue.Remove( i );
			break;
		}
	}
}

bool CObjectTeleporter::IsFirstInQueue( CTFPlayer *pPlayer )
{
	if ( m_PlayerQueue.Count() != 0 )
		return ( m_PlayerQueue[0].pPlayer == pPlayer );

	return false;
}

void CObjectTeleporter::CalcWaitTimeForPlayer( CTFPlayer *pPlayer, bool bAlreadyInQueue )
{
	m_flPotentialWaitTime = 0.0f;

	if ( GetState() == TELEPORTER_STATE_RECHARGING )
	{
		m_flPotentialWaitTime += m_flRechargeTime - gpGlobals->curtime;
	}

	if ( bAlreadyInQueue )
	{
		// Just find myself in queue.
		for ( int i = 0, c = m_PlayerQueue.Count(); i < c; i++ )
		{
			if ( m_PlayerQueue[i].pPlayer == pPlayer )
				break;

			m_flPotentialWaitTime += g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];
		}
	}
	else
	{
		// Put myself up as a candidate.
		TeleporterQueueEntry_t queueEntry = { pPlayer, gpGlobals->curtime };

		for ( int i = 0, c = m_PlayerQueue.Count(); i < c; i++ )
		{
			if ( TeleporterQueueSort( &queueEntry, &m_PlayerQueue[i] ) < 0 )
				break;

			m_flPotentialWaitTime += g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];
		}
	}
}
