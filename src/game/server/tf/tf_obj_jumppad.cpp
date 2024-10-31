//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Jumppad Object
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_jumppad.h"
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
#include "achievements_tf.h"

#include "entity_healthkit.h"

#include "tf_weapon_pda.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define JUMPPAD_MODEL_PLACEMENT				"models/buildables/jumppad_blueprint.mdl"
#define JUMPPAD_MODEL_BUILDING				"models/buildables/jumppad.mdl"
#define JUMPPAD_MODEL_LIGHT					"models/buildables/jumppad_light.mdl"

#define JUMPPAD_MINS			Vector( -16, -16, 0 )
#define JUMPPAD_MAXS			Vector( 16, 16, 8 )	

IMPLEMENT_SERVERCLASS_ST( CObjectJumppad, DT_ObjectJumppad )
	SendPropInt( SENDINFO( m_iState ), 5 ),
	SendPropInt( SENDINFO( m_iTimesUsed ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_DATADESC(CObjectJumppad)
	DEFINE_KEYFIELD ( m_iJumppadType, FIELD_INTEGER, "jumppadType" ),
	DEFINE_THINKFUNC( JumppadThink ),
END_DATADESC()

PRECACHE_REGISTER( obj_jumppad );

#define JUMPPAD_MAX_HEALTH				90

#define JUMPPAD_THINK_CONTEXT			"JumppadContext"

#define JUMPPAD_NEXT_THINK				0.05f

#define JUMPPAD_PER_PLAYER_RECHARGE		0.75f

ConVar tf2c_jumppad_speed( "tf2c_jumppad_speed", "500", FCVAR_REPLICATED, "Horizontal speed applied to jump pad users", true, 0, false, 0 );
ConVar tf2c_jumppad_height( "tf2c_jumppad_height", "700", FCVAR_REPLICATED, "Vertical force applied to jump pad users", true, 0, false, 0 );
ConVar tf2c_jumppad_underwater_depth( "tf2c_jumppad_underwater_depth", "65.0", FCVAR_REPLICATED, "Depth at which jumppad stops working" );

extern ConVar tf2c_building_gun_mettle;
extern ConVar tf2c_afterburn_damage;

LINK_ENTITY_TO_CLASS( obj_jumppad, CObjectJumppad );


CObjectJumppad::CObjectJumppad()
{
	UseClientSideAnimation();

	SetMaxHealth( JUMPPAD_MAX_HEALTH );
	m_iHealth = JUMPPAD_MAX_HEALTH;

	SetType( OBJ_JUMPPAD );
	m_iJumppadType = 0;
}


CObjectJumppad::~CObjectJumppad()
{
}


void CObjectJumppad::Spawn()
{
	// Only used by teleporters placed in hammer.
	if (m_iJumppadType == 1)
	{
		SetObjectMode(JUMPPAD_TYPE_A);
	}
	else if (m_iJumppadType == 2)
	{
		SetObjectMode(JUMPPAD_TYPE_B);
	}

	SetSolid( SOLID_BBOX );

	m_takedamage = DAMAGE_NO;

	SetState( JUMPPAD_STATE_INACTIVE );

	SetModel( JUMPPAD_MODEL_PLACEMENT );

	BaseClass::Spawn();
}


void CObjectJumppad::FirstSpawn()
{
	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );
	
	BaseClass::FirstSpawn();
}

void CObjectJumppad::MakeCarriedObject( CTFPlayer *pPlayer )
{
	SetState( JUMPPAD_STATE_INACTIVE );

	// Stop thinking.
	SetContextThink( NULL, 0, JUMPPAD_THINK_CONTEXT );
	SetTouch( NULL );

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;

	BaseClass::MakeCarriedObject( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Check everyone above
//-----------------------------------------------------------------------------
void CObjectJumppad::CheckAndLaunch(void)
{
	// TF2C Jump Pads check if a player is near & above us.
	Vector vecJumperPos, mins, maxs;
	GetJumppadBounds(vecJumperPos, mins, maxs);

	CBaseEntity *pList[32];
	int count = UTIL_EntitiesInBox(pList, 32, vecJumperPos + mins, vecJumperPos + maxs, 0);
	//NDebugOverlay::Box(vecJumperPos, mins, maxs, 0, 0, 255, 100, 2.0);

	bool bLaunchedSomeone = false;

	for (int i = 0; i < count; i++)
	{
		CBaseEntity *pEntity = pList[i];
		if (!pEntity || pEntity == this)
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		if (!pEntity->FVisible(this, MASK_SOLID))
			continue;

		CTFPlayer* pPlayer = ToTFPlayer(pEntity);

		if (pPlayer)
		{
			if (CheckAndLaunchPlayer(pPlayer))
				bLaunchedSomeone = true;
		}
		else
		{
			CHealthKit* pSandvich = dynamic_cast<CHealthKit*>(pEntity);
			if (pSandvich && ToTFPlayer(pSandvich->GetOwnerEntity()))
			{
				pSandvich->ApplyLocalVelocityImpulse(Vector(0, 0, 125));
			}
		}
	}

	if (bLaunchedSomeone)
	{
		Vector origin = GetAbsOrigin();
		CPVSFilter filter(origin);

		const char *pszEffectName = ConstructTeamParticle("jumppad_jump_puff_%s", GetTeamNumber());
		TE_TFParticleEffect(filter, 0.0f, pszEffectName, origin, vec3_angle);

		EmitSound("Building_JumpPad.Launch");

		m_flMyNextThink = gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check individual player
//-----------------------------------------------------------------------------
bool CObjectJumppad::CheckAndLaunchPlayer(CTFPlayer *pPlayer)
{
	if (!pPlayer->IsAlive())
		return false;

	if (!(pPlayer->m_nButtons & IN_JUMP))
		return false;

	if (!tf2c_building_sharing.GetBool() &&
		pPlayer->IsEnemy(this) && !pPlayer->IsPlayerClass(TF_CLASS_SPY, true))
		return false;

	if (pPlayer->m_Shared.InCond(TF_COND_JUST_USED_JUMPPAD))
		return false;

	CTFWeaponBase* pWpn = pPlayer->GetActiveTFWeapon();
	if ( pWpn && !pWpn->OwnerCanJump() )
		return false;

	if ( pPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) )
		return false;
	
	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	CTFPlayer* pBuilder = GetBuilder();

	if (!m_bWasMapPlaced && pBuilder && !(TFGameRules() && TFGameRules()->InSetup()))
	{
		// Don't give points for players holding Jump over and over
		if ( !pPlayer->m_Shared.InCond( TF_COND_JUMPPAD_ASSIST ) )
		{
			CTF_GameStats.Event_PlayerUsedJumppad( pBuilder, pPlayer );

			// Achievement event CAchievementTF2C_JumppadProgression
			IGameEvent* event = gameeventmanager->CreateEvent( "player_used_jumppad" );
			if ( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );

				if ( pBuilder )
				{
					event->SetInt( "builderid", pBuilder->GetUserID() );
				}

				gameeventmanager->FireEvent( event );
			}
		}
	}

	LaunchPlayer( pPlayer );

	if (pPlayer != pBuilder)
	{
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_JUMPPAD_LAUNCHED );
	}

	m_iTimesUsed++;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Apply velocity to passed player.
//-----------------------------------------------------------------------------
void CObjectJumppad::LaunchPlayer( CTFPlayer *pPlayer )
{
	float flMaxSpeed = pPlayer->GetPlayerClass()->GetMaxSpeed();
	float flLaunchSpeed = tf2c_jumppad_speed.GetFloat();
	float flLaunchHeight = tf2c_jumppad_height.GetFloat();
	float flRatio = flLaunchSpeed / flMaxSpeed;

	// Ensure distance is the same for every class.
	Vector vVel = pPlayer->GetAbsVelocity();
	vVel *= flRatio;

	// Scale horizontal speed based on current speed.
	float flHorSpeed = vVel.Length2D();
	if ( flHorSpeed > flLaunchSpeed )
	{
		vVel *= ( flLaunchSpeed / flHorSpeed );
	}

	vVel.z = flLaunchHeight;

	// achievement
	bool bWasLaunched = pPlayer->m_Shared.InCond(TF_COND_LAUNCHED);

	pPlayer->SetGroundEntity( NULL );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
	pPlayer->m_Shared.SetJumping(true);
	pPlayer->m_Shared.SetAirDash(false); // Reset double jump for Scout
	pPlayer->m_Shared.AddCond(TF_COND_LAUNCHED, PERMANENT_CONDITION, GetBuilder());
	pPlayer->m_Shared.AddCond(TF_COND_JUMPPAD_ASSIST, PERMANENT_CONDITION, GetBuilder());
	pPlayer->m_Shared.AddCond(TF_COND_JUST_USED_JUMPPAD, JUMPPAD_PER_PLAYER_RECHARGE);

	if (pPlayer->m_Shared.InCond(TF_COND_BURNING))
	{
		//CTFPlayer* pOwner = ToTFPlayer(GetOwnerEntity());
		CTFPlayer* pOwner = ToTFPlayer(GetBuilder());

		if (pOwner && !pOwner->IsEnemy(pPlayer) && pOwner!=pPlayer)
		{
			// I feel bad actually typing out so many lines for mere damage block, but it's worth a try
			float flAfterburnBlocked = (pPlayer->m_Shared.GetFlameRemoveTime() - gpGlobals->curtime) * tf2c_afterburn_damage.GetFloat();
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer->m_Shared.GetBurnWeapon(), flAfterburnBlocked, mult_wpn_burndmg);
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer, flAfterburnBlocked, mult_burndmg_wearer);
			if (pPlayer)
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer->GetActiveTFWeapon(), flAfterburnBlocked, mult_burndmg_active);
			if (flAfterburnBlocked < 1000.0f)	// safeguard against infinite or insane value
				CTF_GameStats.Event_PlayerBlockedDamage(pOwner, flAfterburnBlocked);

			IGameEvent* event = gameeventmanager->CreateEvent( "player_extinguished" );
			if ( event )
			{
				event->SetInt( "victim", pPlayer->entindex() );
				event->SetInt( "healer", pOwner->entindex() );

				gameeventmanager->FireEvent( event, true );
			}
		}

		pPlayer->EmitSound("TFPlayer.FlameOut");
		pPlayer->m_Shared.RemoveCond(TF_COND_BURNING);

		if (!bWasLaunched)
			pPlayer->AwardAchievement(TF2C_ACHIEVEMENT_JUMPPAD_EXTINGUISH);

		if (pOwner && !pOwner->IsEnemy(pPlayer) && pOwner != pPlayer)
		{
			// Thank the ~~Pyro~~ Engineer player.
			pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_THANKS);

			CTF_GameStats.Event_PlayerAwardBonusPoints(pOwner, pPlayer, 1);
		}
	}
	
	pPlayer->SetLocalVelocity( vec3_origin );
	pPlayer->SetBaseVelocity( vVel );

	/*QAngle vecAngles = pPlayer->EyeAngles();

	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );
	vecForward.z = 0.f;
	VectorNormalize( vecForward );

	pPlayer->SetAbsVelocity( ( vecForward * flLaunchSpeed ) + Vector( 0, 0, flLaunchHeight ) );*/
}


void CObjectJumppad::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );

	// Reset this after model change
	UTIL_SetSize( this, JUMPPAD_MINS, JUMPPAD_MAXS );

	CreateBuildPoints();

	ReattachChildren();

	m_iBlurBodygroup = FindBodygroupByName( "teleporter_blur" );

	if ( m_iBlurBodygroup >= 0 )
	{
		SetBodygroup( m_iBlurBodygroup, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectJumppad::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( JUMPPAD_MODEL_BUILDING );

	return BaseClass::StartBuilding(pBuilder);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectJumppad::OnGoActive( void )
{
	SetModel( JUMPPAD_MODEL_LIGHT );
	SetActivity( ACT_OBJ_IDLE );

	SetContextThink( &CObjectJumppad::JumppadThink, gpGlobals->curtime + 0.1f, JUMPPAD_THINK_CONTEXT );

	if (IsUnderwater())
	{
		SetState(JUMPPAD_STATE_UNDERWATER);
	}
	else
	{
		SetState(JUMPPAD_STATE_READY);
	}


	BaseClass::OnGoActive();

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;	// Used as a flag to initialize the playback rate to 0 in the first DeterminePlaybackRate.
}


void CObjectJumppad::Precache()
{
	BaseClass::Precache();

	// Precache Object Models.
	PrecacheModel( JUMPPAD_MODEL_PLACEMENT );

	PrecacheGibsForModel( PrecacheModel( JUMPPAD_MODEL_BUILDING ) );
	PrecacheGibsForModel( PrecacheModel( JUMPPAD_MODEL_LIGHT ) );

	// Precache Sounds.
	PrecacheScriptSound( "Building_Jumppad.Spin" );

	PrecacheScriptSound( "Building_JumpPad.Launch" );

	PrecacheTeamParticles( "jumppad_fan_air_%s" );
	PrecacheTeamParticles( "jumppad_jump_puff_%s" );
	PrecacheParticleSystem( "tpdamage_1" );
	PrecacheParticleSystem( "tpdamage_2" );
	PrecacheParticleSystem( "tpdamage_3" );
	PrecacheParticleSystem( "tpdamage_4" );
}


bool CObjectJumppad::IsReady( void )
{
	return GetState() == JUMPPAD_STATE_READY;
}

void CObjectJumppad::DeterminePlaybackRate( void )
{
	float flPlaybackRate = GetPlaybackRate();

	bool bWasBelowFullSpeed = ( flPlaybackRate < 1.0f );

	if ( IsBuilding() )
	{
		// Default rate, author build anim as if one player is building.
		SetPlaybackRate( GetConstructionMultiplier());
	}
	else if ( IsPlacing() )
	{
		SetPlaybackRate( 2.0f );
	}
	else
	{
		float flFrameTime = 0.1f; // BaseObjectThink delay.

		switch ( m_iState )
		{
			case JUMPPAD_STATE_READY:
			{
				// Spin up to 1.0 from whatever we're at, at some high rate.
				flPlaybackRate = Approach( 1.0f, flPlaybackRate, 0.5f * flFrameTime );
				break;
			}

			case JUMPPAD_STATE_UNDERWATER:
			{
				flPlaybackRate = Approach( 0.1f, flPlaybackRate, 0.1f * flFrameTime);
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
void CObjectJumppad::JumppadThink( void )
{
	SetContextThink( &CObjectJumppad::JumppadThink, gpGlobals->curtime + JUMPPAD_NEXT_THINK, JUMPPAD_THINK_CONTEXT );

	if (IsDisabled() || IsBuilding() || IsRedeploying())
	{
		SetState(JUMPPAD_STATE_INACTIVE);
		return;
	}
	else if (IsUnderwater())
	{
		SetState(JUMPPAD_STATE_UNDERWATER);
		return;
	}

	if ( m_flMyNextThink && m_flMyNextThink > gpGlobals->curtime )
		return;

	SetState(JUMPPAD_STATE_READY);
	CheckAndLaunch();
}

int CObjectJumppad::GetBaseHealth( void ) const
{
	return JUMPPAD_MAX_HEALTH;
}
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CObjectJumppad::GetPlacementModel( void ) const
{
	return JUMPPAD_MODEL_PLACEMENT;
}


void CObjectJumppad::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	SetActivity( ACT_OBJ_RUNNING );
	SetPlaybackRate( 0.0f );
}

void CObjectJumppad::SetState( int state )
{
	if ( state != m_iState )
	{
		m_iState = state;
		m_flLastStateChangeTime = gpGlobals->curtime;
	}
}

int CObjectJumppad::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[512];

		// Jumppad State
		V_sprintf_safe( tempstr, "State: %d", m_iState.Get() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}

void CObjectJumppad::GetJumppadBounds(Vector &vecPos, Vector &vecMins, Vector &vecMaxs)
{
	vecPos = GetAbsOrigin();
	vecPos.z += JUMPPAD_MAXS.z + 1.0f;

	Vector vecExpand = Vector(8, 8, 16);

	vecMins = VEC_HULL_MIN - vecExpand;
	vecMaxs = VEC_HULL_MAX + vecExpand;
}

bool CObjectJumppad::IsUnderwater(void)
{
	Vector vecStartPos = m_vecBuildOrigin;
	Vector vecEndPos = vecStartPos + Vector(0, 0, tf2c_jumppad_underwater_depth.GetFloat());

	trace_t trace;
	UTIL_TraceLine(vecStartPos, vecEndPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace);
	//NDebugOverlay::Line(vecStartPos, trace.endpos, 0, 255, 0, true, 30);

	return (UTIL_PointContents(vecEndPos) & MASK_WATER) != 0;
}