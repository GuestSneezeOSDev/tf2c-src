//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun OMG
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_sentrygun.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "world.h"
#include "tf_projectile_rocket.h"
#include "tf_projectile_dart.h"
#include "te_effect_dispatch.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "NextBotManager.h"
#include "tf_weapon_wrench.h"
#include "bot/tf_bot.h"

//Flame Sentry Includes
#include "tf_projectile_arrow.h"
#include "tf_weapon_compound_bow.h"
#include "collisionutils.h"
#include "tf_lagcompensation.h"

static int te_modelidx = 0;



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool IsInCommentaryMode();

// Ground placed version
#define SENTRY_MODEL_PLACEMENT							"models/buildables/sentry1_blueprint.mdl"
#define SENTRY_MODEL_LEVEL_1							"models/buildables/sentry1.mdl"
#define SENTRY_MODEL_LEVEL_1_UPGRADE					"models/buildables/sentry1_heavy.mdl"
#define SENTRY_MODEL_LEVEL_2							"models/buildables/sentry2.mdl"
#define SENTRY_MODEL_LEVEL_2_UPGRADE					"models/buildables/sentry2_heavy.mdl"
#define SENTRY_MODEL_LEVEL_3							"models/buildables/sentry3.mdl"
#define SENTRY_MODEL_LEVEL_3_UPGRADE					"models/buildables/sentry3_heavy.mdl"

#define SENTRY_ROCKET_MODEL								"models/buildables/sentry3_rockets.mdl"

#define SENTRYGUN_MINS									Vector( -20, -20, 0 )
#define SENTRYGUN_MAXS									Vector( 20, 20, 66 )

#define SENTRYGUN_MAX_HEALTH							150

#define SENTRYGUN_ADD_SHELLS							40
#define SENTRYGUN_ADD_ROCKETS							8

#define SENTRY_THINK_DELAY								0.05f
#define SENTRY_ENABLE_DELAY								0.5f

#define	SENTRYGUN_CONTEXT								"SentrygunContext"

#define SENTRYGUN_MINIGUN_RESIST_LVL_1					0.0f
#define ORIGINAL_SENTRYGUN_MINIGUN_RESIST_LVL_2			0.20f
#define ORIGINAL_SENTRYGUN_MINIGUN_RESIST_LVL_3			0.33f
#define GUN_METTLE_SENTRYGUN_MINIGUN_RESIST_LVL_2		0.15f
#define GUN_METTLE_SENTRYGUN_MINIGUN_RESIST_LVL_3		0.20f

#define SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER						0.33f
#define GUN_METTLE_SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER			0.66f


//Flame Sentry TEMP NAME TO AVOID FUCKERY
#define SENTRY_MODEL_FLAME						"models/buildables/flamesentry1.mdl"
#define SENTRY_MODEL_FLAME_PLACEMENT			"models/buildables/flamesentry1.mdl"
#define SENTRY_MODEL_FLAME_UPGRADE				"models/buildables/flamesentry1_heavy.mdl"



enum
{	
	SENTRYGUN_ATTACHMENT_MUZZLE = 0,
	SENTRYGUN_ATTACHMENT_MUZZLE_ALT,
	SENTRYGUN_ATTACHMENT_ROCKET_L,
	SENTRYGUN_ATTACHMENT_ROCKET_R,
};

enum target_ranges
{
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR,
};

#define VECTOR_CONE_TF_SENTRY		Vector( 0.1f, 0.1f, 0 )

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void *SendProxy_SendLocalObjectDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseObject *pObject = (CBaseObject *)pVarData;
	if ( pObject )
	{
		// Only send this chunk of data to the player carrying this weapon
		CTFPlayer *pPlayer = pObject->GetBuilder();
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void *)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalObjectDataTable );

BEGIN_NETWORK_TABLE_NOBASE( CObjectSentrygun, DT_SentrygunLocalData )
	SendPropInt( SENDINFO( m_iKills ), 12, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iAssists ), 12, SPROP_CHANGES_OFTEN ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST( CObjectSentrygun, DT_ObjectSentrygun )
	SendPropInt( SENDINFO( m_iAmmoShells ), 9, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iAmmoRockets ), 6, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iState ), Q_log2( SENTRY_NUM_STATES ) + 1, SPROP_UNSIGNED ),
	SendPropDataTable( "SentrygunLocalData", 0, &REFERENCE_SEND_TABLE( DT_SentrygunLocalData ), SendProxy_SendLocalObjectDataTable ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectSentrygun )
END_DATADESC()

LINK_ENTITY_TO_CLASS( obj_sentrygun, CObjectSentrygun );
PRECACHE_REGISTER( obj_sentrygun );

ConVar tf_sentrygun_damage( "tf_sentrygun_damage", "16", FCVAR_CHEAT );
ConVar tf_sentrygun_damage_falloff( "tf2c_sentrygun_damage_falloff", "0.4", FCVAR_CHEAT, "The amount that damage should be multiplied by at max overriden range. (Lerp between usual max range and target-override max range)" );
ConVar tf_sentrygun_ammocheat( "tf_sentrygun_ammocheat", "0", FCVAR_CHEAT );
ConVar tf_sentrygun_upgrade_per_hit( "tf_sentrygun_upgrade_per_hit", "25", FCVAR_CHEAT );
ConVar tf_sentrygun_newtarget_dist( "tf_sentrygun_newtarget_dist", "200", FCVAR_CHEAT );
ConVar tf_sentrygun_metal_per_shell( "tf_sentrygun_metal_per_shell", "1", FCVAR_CHEAT );
ConVar tf_sentrygun_metal_per_rocket( "tf_sentrygun_metal_per_rocket", "2", FCVAR_CHEAT );
ConVar tf_sentrygun_notarget( "tf_sentrygun_notarget", "0", FCVAR_CHEAT );
ConVar tf_sentrygun_shorttermmemory("tf2c_sentrygun_shorttermmemory", "3", FCVAR_CHEAT, "The time a sentry gun should remember its overriden target for.");

extern ConVar tf2c_building_gun_mettle;
extern ConVar tf2c_spy_gun_mettle;

extern ConVar tf2c_bullets_pass_teammates;


//Flame sentry
ConVar tf_debug_flamesentry("tf_debug_flamesentry", "0", FCVAR_CHEAT, "Visualize the flamethrower damage.");
ConVar tf_flamesentry_velocity("tf_flamesentry_velocity", "2450.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Initial velocity of flame damage entities.");
ConVar tf_flamesentry_drag("tf_flamesentry_drag", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Air drag of flame damage entities.");
ConVar tf_flamesentry_float("tf_flamesentry_float", "10.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Upward float velocity of flame damage entities.");
ConVar tf_flamesentry_flametime("tf_flamesentry_flametime", "0.6", FCVAR_CHEAT | FCVAR_REPLICATED, "Time to live of flame damage entities.");
ConVar tf_flamesentry_vecrand("tf_flamesentry_vecrand", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Random vector added to initial velocity of flame damage entities.");
ConVar tf_flamesentry_boxsize("tf_flamesentry_boxsize", "25.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Size of flame damage entities.");
ConVar tf_flamesentry_maxdamagedist("tf_flamesentry_maxdamagedist", "350.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Maximum damage distance for flamethrower.");
ConVar tf_flamesentry_shortrangedamagemultiplier("tf_flamesentry_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT | FCVAR_REPLICATED, "Damage multiplier for close-in flamethrower damage.");
ConVar tf_flamesentry_velocityfadestart("tf_flamesentry_velocityfadestart", ".3", FCVAR_CHEAT | FCVAR_REPLICATED, "Time at which attacker's velocity contribution starts to fade.");
ConVar tf_flamesentry_velocityfadeend("tf_flamesentry_velocityfadeend", ".5", FCVAR_CHEAT | FCVAR_REPLICATED, "Time at which attacker's velocity contribution finishes fading.");
ConVar tf2c_flamesentry_wallslide("tf2c_flamesentry_wallslide", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Whether flame entities slide along hit walls.");
ConVar tf2c_flamesentry_damage("tf2c_flamesentry_damage", "9", FCVAR_REPLICATED);

CObjectSentrygun::CObjectSentrygun()
{
	SetMaxHealth( SENTRYGUN_MAX_HEALTH );
	m_iHealth = SENTRYGUN_MAX_HEALTH;

	SetType( OBJ_SENTRYGUN );
	m_lastTeammateWrenchHit = NULL;
	m_lastTeammateWrenchHitTimer.Invalidate();
	m_flRangeMultiplier = 1.0f;
	m_flRangeMultiplierOverridenTarget = 1.0f;
	m_bCurrentlyTargetingOverriddenTarget = false;
	m_flFireRateMult = 1.0f;
}


void CObjectSentrygun::Spawn()
{
	m_iPitchPoseParameter = -1;
	m_iYawPoseParameter = -1;

	SetModel( SENTRY_MODEL_PLACEMENT );
	
	m_takedamage = DAMAGE_YES;

	// Rotate Details.
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 6;
	m_flFieldOfView = VIEW_FIELD_NARROW;

	// Give the Gun some ammo.
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
	m_iMaxAmmoRockets = SENTRYGUN_MAX_ROCKETS;

	m_iAmmoType = GetAmmoDef()->Index( "TF_AMMO_PRIMARY" );

	// Start searching for enemies.
	m_hEnemy = NULL;
	m_hEnemyOverride = NULL;
	m_flOverrideForgetTime = 0;

	// Pipes explode when they hit this.
	m_takedamage = DAMAGE_AIM;

	m_flLastAttackedTime = 0;

	m_iFireMode = 0;

	m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_1;

	m_lastTeammateWrenchHit = NULL;
	m_lastTeammateWrenchHitTimer.Invalidate();

	m_flRangeMultiplier = 1.0f;
	m_flRangeMultiplierOverridenTarget = 1.0f;
	m_bCurrentlyTargetingOverriddenTarget = false;

	m_flFireRateMult = 1.0f;

	BaseClass::Spawn();

	SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_1 );

	UTIL_SetSize( this, SENTRYGUN_MINS, SENTRYGUN_MAXS );

	m_iState.Set( SENTRY_STATE_INACTIVE );

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );
}


void CObjectSentrygun::FirstSpawn()
{
	m_flLastAttackedTime = 0;

	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	m_iAmmoShells = 0;
	m_iAmmoRockets = 0;

	BaseClass::FirstSpawn();
}

void CObjectSentrygun::MakeCarriedObject( CTFPlayer *pPlayer )
{
	// Stop thinking.
	m_iState.Set( SENTRY_STATE_INACTIVE );

	// Clear enemy.
	m_hEnemy = NULL;
	m_hEnemyOverride = NULL;
	m_flOverrideForgetTime = 0;

	// Reset upgrade values.
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
	m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_1;
	SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_1 );

	BaseClass::MakeCarriedObject( pPlayer );
}

void CObjectSentrygun::SetTargetOverride( CBaseEntity *pEnt )
{
	if ( GetAbsOrigin().DistTo( pEnt->GetAbsOrigin() ) <= GetMaxRangeOverride() )
	{
		m_hEnemyOverride = pEnt;
		m_flOverrideForgetTime = gpGlobals->curtime + tf_sentrygun_shorttermmemory.GetFloat();
	}
}

float CObjectSentrygun::GetMaxRange() const {
	if ( m_bCurrentlyTargetingOverriddenTarget )
		return SENTRYGUN_MAX_RANGE * m_flRangeMultiplier * m_flRangeMultiplierOverridenTarget;

	return SENTRYGUN_MAX_RANGE * m_flRangeMultiplier;
}

float CObjectSentrygun::GetPushMultiplier()
{
	float flSentryKnockback = SENTRY_PUSH_MULTIPLIER;
	if ( GetBuilder() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flSentryKnockback, mult_sentry_knockback );
	}
	return flSentryKnockback;
}

ConVar tf2c_sentry_laser("tf2c_sentry_laser", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "");
ConVar tf2c_sentry_laser_alpha("tf2c_sentry_laser_alpha", "192", FCVAR_REPLICATED | FCVAR_NOTIFY, "");
ConVar tf2c_sentry_laser_while_searching("tf2c_sentry_laser_while_searching", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "");
ConVar tf2c_sentry_laser_while_attacking("tf2c_sentry_laser_while_attacking", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "");

void CObjectSentrygun::SentryThink(void)
{
    // Don't think while re-deploying so we don't target anything inbetween upgrades.
    if (IsRedeploying())
    {
        SetContextThink(&CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT);
        return;
    }

    Vector CurAnglesForward;
    // Vector GoalAnglesForward;

    AngleVectors(m_vecCurAngles, &CurAnglesForward);
    // AngleVectors(m_vecGoalAngles, &GoalAnglesForward);

    // our starting vector for drawing from
    Vector sentryEyePos = EyePosition();
    Assert(!sentryEyePos.IsZero() && sentryEyePos.IsValid());


    auto doLaserCheckForCollide = [&]()
        {
            // our owner aka the engineer (i hope)
            auto ownerent = this->GetBuilder();
            AssertMsg(ownerent, "bad ownerent in doLaserCheckForCollide");
            if (!ownerent)
            {
                return (Vector(0, 0, 0));
            }

            // BUG(ish) - we draw through the engineer but we're shooting the person behind him -
            // this is supposed to filter out teammates and projectiles so we dont cut our drawing early
            // if a friendly or rocket or something flies in the way... i think we might want a custom filter here though
            // since i'm pretty sure this is hitting like, resup cabinets and models and shit
            CTraceFilterIgnoreTeammatesAndProjectiles filter(
                ownerent,
                COLLISION_GROUP_NONE,
                ownerent->GetTeamNumber()
            );

            Assert(&filter);
            trace_t tr = {};

            UTIL_TraceLine(sentryEyePos, sentryEyePos + CurAnglesForward * 5000,
                MASK_SHOT, &filter, &tr);
            //NDebugOverlay::Line(vecSrc, vecSrc + CurAnglesForward * 5000, 123, 222, 222, false, 0.1);

            // we hit a player
            if (tr.DidHitNonWorldEntity())
            {
                AssertMsg(false, "hit something in doLaserCheckForCollide [removeme]");
                // hopefully this is the point we hit on the player we hit
                return (tr.endpos);
            }
            // we hit the world itself
            else
            {
                // hopefully this is the point that we hit on the world and it doesnt trace through
                // but it would be funny if it did
                return (tr.endpos);
            }
        };

    auto doBeamDraw = [&](Vector sentryLazerEndPoint)
        {
            Assert(!sentryLazerEndPoint.IsZero() && sentryLazerEndPoint.IsValid());

            #ifdef _DEBUG
            // seems to be needed when hot reloading
            te_modelidx = PrecacheModel("sprites/lgtning.vmt");
            #endif

            int clampedLaserAlpha = clamp(tf2c_sentry_laser_alpha.GetInt(), 0, 255);
            // We need to draw the laser beam color the same color
            // as the gun's team. this is self explanatory
            Color clrBlue   = Color(  2, 126, 250,  clampedLaserAlpha);
            Color clrRed    = Color(255,  64,  64,  clampedLaserAlpha);
            Color clrGreen  = Color(  8, 174,   0,  clampedLaserAlpha);
            Color clrYellow = Color(255, 160,   0,  clampedLaserAlpha);
            Color ourColor  = Color(255, 255, 255,  clampedLaserAlpha);
            int teamNum = this->GetTeamNumber();
            switch (teamNum)
            {
            case TF_TEAM_RED:
                ourColor = clrRed;
                break;
            case TF_TEAM_BLUE:
                ourColor = clrBlue;
                break;
            case TF_TEAM_GREEN:
                ourColor = clrGreen;
                break;
            case TF_TEAM_YELLOW:
                ourColor = clrYellow;
                break;
            default:
                break;
            }
            // this should just be "all players"
            CBroadcastRecipientFilter filter;
            te->BeamPoints(filter,
                0.0,                        // delay
                &sentryEyePos,              // startpoint
                &sentryLazerEndPoint,       // endpoint
                te_modelidx,                // model idx
                te_modelidx,                // halo idx
                0,                          // startframe
                255,                        // framerate
                0.051,                      // lifetime
                2.0,                        // width
                2.0,                        // endwidth
                2.0,                        // fadelength
                0.0,                        // amplitude
                ourColor.r(),
                ourColor.g(),
                ourColor.b(),
                ourColor.a(),               // r g b a
                10                          // "speed"
            );
        };


    auto doLaserStuff = [&]()
        {
            if (!tf2c_sentry_laser.GetBool())
            {
                return;
            }

            Vector finalBeamLocation = doLaserCheckForCollide();
            if (!finalBeamLocation.IsZero() && finalBeamLocation.IsValid())
            {
                doBeamDraw(finalBeamLocation);
            }
            AssertMsg(false, "bad vector in doLaserStuff");
        };



	switch ( m_iState )
	{
		case SENTRY_STATE_INACTIVE:
			break;

		case SENTRY_STATE_SEARCHING:
            if (tf2c_sentry_laser_while_searching.GetBool())
            {
                doLaserStuff();
            }
			SentryRotate();
			break;

		case SENTRY_STATE_ATTACKING:
            if (tf2c_sentry_laser_while_attacking.GetBool())
            {
                doLaserStuff();
            }
			Attack();
			break;

		case SENTRY_STATE_UPGRADING:
			UpgradeThink();
			break;
	}

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );
}

void CObjectSentrygun::StartPlacement( CTFPlayer *pPlayer )
{
	BaseClass::StartPlacement( pPlayer );

	// Set my build size
	m_vecBuildMins = SENTRYGUN_MINS;
	m_vecBuildMaxs = SENTRYGUN_MAXS;
	m_vecBuildMins -= Vector( 4, 4, 0 );
	m_vecBuildMaxs += Vector( 4, 4, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectSentrygun::StartBuilding( CBaseEntity *pBuilder )
{
	UseClientSideAnimation();

	SetModel( SENTRY_MODEL_LEVEL_1_UPGRADE );

	CreateBuildPoints();

	SetPoseParameter( m_iPitchPoseParameter, 0.0f );
	SetPoseParameter( m_iYawPoseParameter, 0.0f );

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CObjectSentrygun::CanBeUpgraded( CTFPlayer *pPlayer )
{
	if ( !m_bWasMapPlaced || HasSpawnFlags( SF_OBJ_UPGRADABLE ) )
		return BaseClass::CanBeUpgraded( pPlayer );

	return false;
}


void CObjectSentrygun::OnGoActive( void )
{
	UseServerSideAnimation();

	SetModel( SENTRY_MODEL_LEVEL_1 );

	m_iState.Set( SENTRY_STATE_SEARCHING );

	// Orient it
	QAngle angles = GetAbsAngles();

	m_vecCurAngles.y = UTIL_AngleMod( angles.y );
	m_iRightBound = UTIL_AngleMod( (int)angles.y - 50.0f );
	m_iLeftBound = UTIL_AngleMod( (int)angles.y + 50.0f );
	if ( m_iRightBound > m_iLeftBound )
	{
		m_iRightBound = m_iLeftBound;
		m_iLeftBound = UTIL_AngleMod( (int)angles.y - 50.0f );
	}

	// Start it rotating.
	m_vecGoalAngles.y = m_iRightBound;
	m_vecGoalAngles.x = m_vecCurAngles.x = 0;
	m_bTurningRight = true;

	EmitSound( "Building_Sentrygun.Built" );

	// if our eye pos is underwater, we're waterlevel 3, else 0.
	SetWaterLevel( ( UTIL_PointContents( EyePosition() ) & MASK_WATER ) ? 3 : 0 );	

	if ( !IsRedeploying() )
	{
		m_iAmmoShells = m_iMaxAmmoShells;
	}

	// Init attachments for level 1 sentry gun
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = 0;

	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetBuilder(), m_iFireMode, set_sentry_projectile );

	m_flFireRateMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), m_flFireRateMult, mult_sentry_firerate );

	m_flRangeMultiplier = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), m_flRangeMultiplier, mult_sentry_range );

	m_flRangeMultiplierOverridenTarget = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), m_flRangeMultiplierOverridenTarget, mult_sentry_range_overriden );

	m_iTurnRateOverride = m_iBaseTurnRate;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), m_iTurnRateOverride, mult_sentry_turnrate_overriden );

	m_bCurrentlyTargetingOverriddenTarget = false;

	BaseClass::OnGoActive();
}


void CObjectSentrygun::Precache()
{
	BaseClass::Precache();

	int iModelIndex;

	// Models
	PrecacheModel( SENTRY_MODEL_PLACEMENT );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel(SENTRY_MODEL_FLAME);
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel(SENTRY_MODEL_FLAME_UPGRADE);
	PrecacheGibsForModel(iModelIndex);

	PrecacheModel( SENTRY_ROCKET_MODEL );
	PrecacheModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl" );

    te_modelidx = PrecacheModel("sprites/lgtning.vmt");


	// Sounds
	PrecacheScriptSound( "Building_Sentrygun.Fire" );
	PrecacheScriptSound( "Building_Sentrygun.Fire2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Fire3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.FireRocket" );
	PrecacheScriptSound( "Building_Sentrygun.Alert" );
	PrecacheScriptSound( "Building_Sentrygun.AlertTarget" );
	PrecacheScriptSound( "Building_Sentrygun.Idle" );
	PrecacheScriptSound( "Building_Sentrygun.Idle2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Idle3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.Built" );
	PrecacheScriptSound( "Building_Sentrygun.Empty" );

	PrecacheScriptSound( "Weapon_Designator.TargetSuccess" ); // Designator targeting mechanic
	PrecacheScriptSound( "Weapon_Designator.TargetFail" );
	PrecacheScriptSound( "Weapon_Designator.TargetSuccessVictim" );
	PrecacheScriptSound( "Weapon_Designator.RewardMetal" );

	PrecacheParticleSystem( "sentrydamage_1" );
	PrecacheParticleSystem( "sentrydamage_2" );
	PrecacheParticleSystem( "sentrydamage_3" );
	PrecacheParticleSystem( "sentrydamage_4" );
	PrecacheParticleSystem( "muzzle_sentry" );
	PrecacheParticleSystem( "muzzle_sentry2" );
	PrecacheTeamParticles( "bullet_tracer01_%s" );

	// Flame Sentry
	PrecacheScriptSound("Building_Sentrygun.FireFlame");
	PrecacheScriptSound("Building_Sentrygun.StopFlame");
}

//-----------------------------------------------------------------------------
// Raises the Sentrygun one level
//-----------------------------------------------------------------------------
void CObjectSentrygun::StartUpgrading( void )
{
	UseClientSideAnimation();

	BaseClass::StartUpgrading();

	int iOldMaxAmmoShells = m_iMaxAmmoShells;

	switch ( m_iUpgradeLevel )
	{
		case 2:
			SetModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
			m_flHeavyBulletResist = tf2c_building_gun_mettle.GetBool() ? GUN_METTLE_SENTRYGUN_MINIGUN_RESIST_LVL_2 : ORIGINAL_SENTRYGUN_MINIGUN_RESIST_LVL_2;
			SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_2 );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2;
			break;
		case 3:
			SetModel( SENTRY_MODEL_LEVEL_3_UPGRADE );

			if ( !IsRedeploying() )
			{
				m_iAmmoRockets = SENTRYGUN_MAX_ROCKETS;
			}

			m_flHeavyBulletResist = tf2c_building_gun_mettle.GetBool() ? GUN_METTLE_SENTRYGUN_MINIGUN_RESIST_LVL_3 : ORIGINAL_SENTRYGUN_MINIGUN_RESIST_LVL_3;
			SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_3 );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
			break;
		default:
			Assert( 0 );
			break;
	}

	// More ammo capability.
	if ( !IsRedeploying() )
	{
		m_iAmmoShells += Max<int>( Min<int>( m_iMaxAmmoShells - iOldMaxAmmoShells, m_iMaxAmmoShells ), 0 );
	}

	m_iState.Set( SENTRY_STATE_UPGRADING );

	// Start upgrade anim instantly
	DetermineAnimation();

	RemoveAllGestures();
}

void CObjectSentrygun::FinishUpgrading( void )
{
	UseServerSideAnimation();

	m_iState.Set( SENTRY_STATE_SEARCHING );
	m_hEnemy = NULL;
	//m_hEnemyOverride = NULL;

	switch ( m_iUpgradeLevel )
	{
		case 1:
			SetModel( SENTRY_MODEL_LEVEL_1 );
			break;
		case 2:
			SetModel( SENTRY_MODEL_LEVEL_2 );
			break;
		case 3:
			SetModel( SENTRY_MODEL_LEVEL_3 );
			break;
		default:
			Assert( 0 );
			break;
	}

	// Look up the new attachments
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle_l" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = LookupAttachment( "muzzle_r" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = LookupAttachment( "rocket_l" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = LookupAttachment( "rocket_r" );

	BaseClass::FinishUpgrading();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::IsUpgrading( void ) const
{
	return m_iState == SENTRY_STATE_UPGRADING;
}

//-----------------------------------------------------------------------------
// Are we disabled?
//-----------------------------------------------------------------------------
bool CObjectSentrygun::IsDisabled( void ) const
{
	if ( gpGlobals->curtime < m_flEnableDelay )
		return true;

	return BaseClass::IsDisabled();
}

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CObjectSentrygun::OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos )
{
	bool bRepair = false;
	bool bUpgrade = false;

	bRepair = Command_Repair( pPlayer, pWrench->GetRepairValue() );

	// Don't put in upgrade metal until the object is fully healed
	if ( !bRepair && CanBeUpgraded( pPlayer ) )
	{
		bUpgrade = CheckUpgradeOnHit( pPlayer );
	}

	DoWrenchHitEffect( vecHitPos, bRepair, bUpgrade );

	if ( !IsUpgrading() )
	{
		// Player ammo into rockets:
		//	1 ammo = 1 shell
        //	2 ammo = 1 rocket
        // Only fill rockets if we have extra shells.

int iPlayerMetal = pPlayer->GetAmmoCount(TF_AMMO_METAL);

// If the sentry has less that 100% ammo, put some ammo in it
if (m_iAmmoShells < m_iMaxAmmoShells && iPlayerMetal > 0)
{
    int iMaxShellsPlayerCanAfford = (int)((float)iPlayerMetal / tf_sentrygun_metal_per_shell.GetFloat());

    // cap the amount we can add
    int iAmountToAdd = Min(SENTRYGUN_ADD_SHELLS, iMaxShellsPlayerCanAfford);

    iAmountToAdd = Min((m_iMaxAmmoShells - m_iAmmoShells), iAmountToAdd);

    pPlayer->RemoveAmmo(iAmountToAdd * tf_sentrygun_metal_per_shell.GetInt(), TF_AMMO_METAL);
    m_iAmmoShells += iAmountToAdd;

    if (iAmountToAdd > 0)
    {
        bRepair = true;
    }
}

// One rocket per two ammo
iPlayerMetal = pPlayer->GetAmmoCount(TF_AMMO_METAL);

if (m_iAmmoRockets < m_iMaxAmmoRockets && m_iUpgradeLevel == 3 && iPlayerMetal > 0)
{
    int iMaxRocketsPlayerCanAfford = (int)((float)iPlayerMetal / tf_sentrygun_metal_per_rocket.GetFloat());

    int iAmountToAdd = Min((SENTRYGUN_ADD_ROCKETS), iMaxRocketsPlayerCanAfford);
    iAmountToAdd = Min((m_iMaxAmmoRockets - m_iAmmoRockets), iAmountToAdd);

    pPlayer->RemoveAmmo(iAmountToAdd * tf_sentrygun_metal_per_rocket.GetFloat(), TF_AMMO_METAL);
    m_iAmmoRockets += iAmountToAdd;

    if (iAmountToAdd > 0)
    {
        bRepair = true;
    }
}
    }

    if (GetBuilder() != pPlayer)
    {
        // Keep track of who last hit us with a wrench for kill assists
        m_lastTeammateWrenchHit = pPlayer;
        m_lastTeammateWrenchHitTimer.Start();
    }

    return bRepair || bUpgrade;
}

int CObjectSentrygun::GetBaseHealth(void) const
{
    return 150;
}

//-----------------------------------------------------------------------------
// Debug infos
//-----------------------------------------------------------------------------
int CObjectSentrygun::DrawDebugTextOverlays(void)
{
    int text_offset = BaseClass::DrawDebugTextOverlays();

    if (m_debugOverlays & OVERLAY_TEXT_BIT)
    {
        char tempstr[512];

        V_sprintf_safe(tempstr, "Shells: %d / %d", m_iAmmoShells.Get(), m_iMaxAmmoShells.Get());
        EntityText(text_offset, tempstr, 0);
        text_offset++;

        if (m_iUpgradeLevel == 3)
        {
            V_sprintf_safe(tempstr, "Rockets: %d / %d", m_iAmmoRockets.Get(), m_iMaxAmmoRockets.Get());
            EntityText(text_offset, tempstr, 0);
            text_offset++;
        }

        Vector vecSrc = EyePosition();
        Vector forward;

        // m_vecCurAngles
        AngleVectors(m_vecCurAngles, &forward);
        NDebugOverlay::Line(vecSrc, vecSrc + forward * 200, 0, 255, 0, false, 0.1);
        
        // m_vecGoalAngles
        AngleVectors(m_vecGoalAngles, &forward);
        NDebugOverlay::Line(vecSrc, vecSrc + forward * 200, 0, 0, 255, false, 0.1);
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Returns the sentry targeting range the target is in
//-----------------------------------------------------------------------------
int CObjectSentrygun::Range( CBaseEntity *pTarget )
{
	Vector vecOrg = EyePosition();
	Vector vecTargetOrg = pTarget->EyePosition();

	int iDist = ( vecTargetOrg - vecOrg ).Length();
	if ( iDist < 132 )
		return RANGE_MELEE;

	if ( iDist < GetMaxRange() / 2 )
		return RANGE_NEAR;

	if ( iDist < GetMaxRange() )
		return RANGE_MID;

	return RANGE_FAR;
}

//-----------------------------------------------------------------------------
// Look for a target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FindTarget()
{
	// Disable the sentry guns for IFM.
	if ( tf_sentrygun_notarget.GetBool() )
		return false;

	if ( IsInCommentaryMode() )
		return false;

	// Sapper, etc.
	if ( IsDisabled() )
		return false;

	// Loop through players within sentry range.
	Vector vecSentryOrigin = EyePosition();

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = Square( GetMaxRange() );
	CBaseEntity *pTargetCurrent = nullptr;
	CBaseEntity *pTargetOld = m_hEnemy.Get();
	float flOldTargetDist2 = FLT_MAX;

	m_bCurrentlyTargetingOverriddenTarget = false;

	if( m_hEnemyOverride.Get() && m_hEnemyOverride->IsAlive() )
	{
		vecTargetCenter = m_hEnemyOverride->GetAbsOrigin();
		vecTargetCenter += m_hEnemyOverride->GetViewOffset();
		VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
		bool bValid = ( m_hEnemyOverride->IsPlayer() 
		&& ValidTargetPlayer( static_cast<CTFPlayer*>( m_hEnemyOverride.Get() ), vecSentryOrigin, vecTargetCenter ) )
		|| ( m_hEnemyOverride->IsBaseObject() 
		&& ValidTargetObject( static_cast<CBaseObject*>( m_hEnemyOverride.Get() ), vecSentryOrigin, vecTargetCenter ) );
		

		CPASFilter filter( vecSentryOrigin );

		if( bValid )
		{
			if ( m_hEnemyOverride.Get() != pTargetOld ) {
				FoundTarget( m_hEnemyOverride, vecSentryOrigin );
				EmitSound( filter, entindex(), "Weapon_Designator.TargetSuccess" );
				
				CBasePlayer *pVictim = dynamic_cast<CBasePlayer *>(m_hEnemyOverride.Get());
				if ( pVictim )
				{
					CSingleUserRecipientFilter victimFilter ( pVictim );
					Vector vOffset = pVictim->EyePosition() + ((GetAbsOrigin() - pVictim->EyePosition()).Normalized() * 5.0f);
					EmitSound( victimFilter, entindex(), "Weapon_Designator.TargetSuccessVictim", &vOffset );
				}
			}

			m_bCurrentlyTargetingOverriddenTarget = true;

			m_flOverrideForgetTime = gpGlobals->curtime + tf_sentrygun_shorttermmemory.GetFloat();

			return true;
		}
		else if ( gpGlobals->curtime > m_flOverrideForgetTime )
		{
			EmitSound( filter, entindex(), "Weapon_Designator.TargetFail" );
			m_hEnemyOverride.Set( NULL );
		}
	}

	//m_hEnemyOverride.Set( NULL );

	// Sentries will try to target players (including bots) first, then NextBotCombatCharacter NPCs, then objects.
	// However, if the enemy held was a NBCC NPC or an object, it will continue to try and attack it first.
	ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			return true;

		for ( int iPlayer = 0; iPlayer < pTeam->GetNumPlayers(); ++iPlayer )
		{
			CTFPlayer *pTargetPlayer = pTeam->GetTFPlayer( iPlayer );
			if ( pTargetPlayer == nullptr )
				continue;

			// Make sure the player is alive.
			if ( !pTargetPlayer->IsAlive() )
				continue;

			if ( pTargetPlayer->GetFlags() & FL_NOTARGET )
				continue;

			vecTargetCenter = pTargetPlayer->GetAbsOrigin();
			vecTargetCenter += pTargetPlayer->GetViewOffset();
			VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
			float flDist2 = vecSegment.LengthSqr();

			// Check to see if the target is closer than the already validated target.
			if ( flDist2 > flMinDist2 )
				continue;

			// It is closer, check to see if the target is valid.
			if ( ValidTargetPlayer( pTargetPlayer, vecSentryOrigin, vecTargetCenter ) )
			{
				flMinDist2 = flDist2;
				pTargetCurrent = pTargetPlayer;

				// Store the current target distance if we come across it.
				if ( pTargetPlayer == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}
			}
		}

		return true;
	} );

	// If we already have a target, don't check NextBotCombatCharacter NPCs.
	// (Note that NextBotPlayers, e.g. TFBots, are handled in the player block above.)
	if ( pTargetCurrent == nullptr )
	{
		CUtlVector<INextBot *> nextbots;
		TheNextBots().CollectAllBots( &nextbots );

		// This NBCC-specific block of code compares against its own, separate minimum distance
		float flMinNBCCDist2 = Square( GetMaxRange() );

		for ( auto nextbot : nextbots )
		{
			CBaseCombatCharacter *pTargetNPC = nextbot->GetEntity();
			if ( pTargetNPC == nullptr )
				continue;

			vecTargetCenter = pTargetNPC->GetAbsOrigin();
			vecTargetCenter += pTargetNPC->GetViewOffset();
			VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
			float flDist2 = vecSegment.LengthSqr();

			// Check to see if the target is closer than the already validated NextBotCombatCharacter target.
			if ( flDist2 > flMinNBCCDist2 )
				continue;

			// It is closer, check to see if the target is valid.
			if ( ValidTargetNextBotNPC( pTargetNPC, vecSentryOrigin, vecTargetCenter ) )
			{
				flMinNBCCDist2 = flDist2;
				pTargetCurrent = pTargetNPC;
			}
		}
	}

	// If we already have a target, don't check objects.
	if ( pTargetCurrent == nullptr )
	{
		ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				return true;

			for ( int iObject = 0; iObject < pTeam->GetNumObjects(); ++iObject )
			{
				CBaseObject *pTargetObject = pTeam->GetObject( iObject );
				if ( !pTargetObject )
					continue;

				vecTargetCenter = pTargetObject->GetAbsOrigin();
				vecTargetCenter += pTargetObject->GetViewOffset();
				VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Store the current target distance if we come across it
				if ( pTargetObject == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// It is closer, check to see if the target is valid.
				if ( ValidTargetObject( pTargetObject, vecSentryOrigin, vecTargetCenter ) )
				{
					flMinDist2 = flDist2;
					pTargetCurrent = pTargetObject;
				}
			}

			return true;
		} );
	}

	// If we already have a target, don't check Power Siege generators.
	if ( pTargetCurrent == nullptr )
	{
		for ( CTeamGenerator* pGenerator : CTeamGenerator::AutoList() )
		{
			CTeamGenerator *pTargetGenerator = pGenerator;
			if ( !pTargetGenerator )
				continue;

			if ( pTargetGenerator->GetTeamNumber() == GetTeamNumber() )
				continue;

			if ( pTargetGenerator->GetShield() && pTargetGenerator->GetShield()->m_takedamage != DAMAGE_NO )
			{
				vecTargetCenter = pTargetGenerator->GetShield()->GetAbsOrigin();
			}
			else
			{
				vecTargetCenter = pTargetGenerator->GetAbsOrigin();
			}
			//NDebugOverlay::Line( vecSentryOrigin, vecTargetCenter, 255, 255, 255, false, SENTRY_THINK_DELAY );

			VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
			float flDist2 = vecSegment.LengthSqr();

			// Store the current target distance if we come across it
			if ( pTargetGenerator == pTargetOld )
			{
				flOldTargetDist2 = flDist2;
			}

			// Check to see if the target is closer than the already validated target.
			if ( flDist2 > flMinDist2 )
				continue;

			// It is closer, check to see if the target is valid.
			if ( ValidTargetGenerator( pTargetGenerator, vecSentryOrigin, vecTargetCenter ) )
			{
				flMinDist2 = flDist2;
				pTargetCurrent = pTargetGenerator;
			}
		}
	}

	// We have a target.
	if ( pTargetCurrent )
	{
		if ( pTargetCurrent != pTargetOld )
		{
			// flMinDist2 is the new target's distance
			// flOldTargetDist2 is the old target's distance
			// Don't switch unless the new target is closer by some percentage
			if ( flMinDist2 < ( flOldTargetDist2 * 0.75f ) )
			{
				FoundTarget( pTargetCurrent, vecSentryOrigin );
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Note that NextBotPlayers (e.g. TFBots) are handled by this function.
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd )
{
	// Keep shooting at spies that go invisible after we acquire them as a target.
	if ( pPlayer->m_Shared.GetPercentInvisible() > 0.5 )
		return false;

	if (pPlayer != m_hEnemy)
	{
		// Keep shooting at spies that disguise after we acquire them as at a target.
		if (pPlayer->m_Shared.DisguiseFoolsTeam(GetTeamNumber()))
		{
			return false;
		}
		
		int iSentryNoAggro = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pPlayer, iSentryNoAggro, sentries_no_aggro);
		if (iSentryNoAggro)
		{
			if (!pPlayer->m_Shared.InCond(TF_COND_DID_DISGUISE_BREAKING_ACTION))
			{
				return false;
			}
		}
	}

	// Not across water boundary.
	if ( ( GetWaterLevel() == WL_NotInWater && pPlayer->GetWaterLevel() == WL_Eyes ) || ( GetWaterLevel() == WL_Eyes && pPlayer->GetWaterLevel() == WL_NotInWater ) )
		return false;

	// Ray trace!!!
	return FVisible( pPlayer, MASK_SHOT | CONTENTS_GRATE );
}


bool CObjectSentrygun::ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd )
{
	// Ignore objects being placed, they are not real objects yet.
	if ( pObject->IsPlacing() )
		return false;

	// Ignore sappers.
	if ( pObject->MustBeBuiltOnAttachmentPoint() )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == WL_NotInWater && pObject->GetWaterLevel() == WL_Eyes ) || ( GetWaterLevel() == WL_Eyes && pObject->GetWaterLevel() == WL_NotInWater ) )
		return false;

	if ( pObject->GetObjectFlags() & OF_IS_CART_OBJECT )
		return false;

	// Ray trace.
	return FVisible( pObject, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// This function is called 'ValidTargetBot' in live TF2. However, the code here
// solely concerns NextBotCombatCharacters (NextBot-based NPC bots). TFBots, on
// the other hand, are NextBotPlayers. NBCC's and NBP's are completely distinct
// types of things, and it's confusing to call this function 'ValidTargetBot'
// given that the entities most people think of as "bots" (TFBots) are actually
// not validated here, but rather in ValidTargetPlayer.
//
// As such, it's been renamed to 'ValidTargetNextBotNPC' for greater clarity.
//
// NBCC's in live TF2 include: Halloween bosses, MvM tanks, RD robots, etc.
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetNextBotNPC( CBaseCombatCharacter *pNPC, const Vector &vecStart, const Vector &vecEnd )
{
	// Ignore NextBotPlayers; they are handled by the normal player logic, e.g. ValidTargetPlayer.
	if ( pNPC->IsPlayer() )
		return false;

	// The NPC must be alive.
	if ( !pNPC->IsAlive() )
		return false;

	// Ignore NPCs who are our teammates.
	if ( pNPC->InSameTeam( this ) )
		return false;

	// Ignore non-solid NPCs.
	if ( pNPC->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == WL_NotInWater && pNPC->GetWaterLevel() == WL_Eyes ) || ( GetWaterLevel() == WL_Eyes && pNPC->GetWaterLevel() == WL_NotInWater ) )
		return false;

	// Ray trace.
	CBaseEntity *pBlocker;
	return FVisible( pNPC, MASK_SHOT | CONTENTS_GRATE, &pBlocker ) || pBlocker == pNPC->GetParent();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetGenerator( CTeamGenerator *pGenerator, const Vector &vecStart, const Vector &vecEnd )
{
	if ( pGenerator->HealthFraction() <= 0.0f )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == WL_NotInWater && pGenerator->GetWaterLevel() == WL_Eyes ) || ( GetWaterLevel() == WL_Eyes && pGenerator->GetWaterLevel() == WL_NotInWater ) )
		return false;

	// Ray trace.
	if ( pGenerator->GetShield() && pGenerator->GetShield()->m_takedamage != DAMAGE_NO )
	{
		return FVisible( pGenerator->GetShield(), MASK_SHOT | CONTENTS_GRATE );
	}

	return FVisible( pGenerator, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Found a Target
//-----------------------------------------------------------------------------
void CObjectSentrygun::FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter )
{
	m_hEnemy = pTarget;

	if ( m_iAmmoShells > 0 || ( m_iAmmoRockets > 0 && m_iUpgradeLevel == 3 ) )
	{
		// Play one sound to everyone but the target.
		CPASFilter filter( vecSoundCenter );

		if ( pTarget->IsPlayer() )
		{
			// Play a specific sound just to the target and remove it from the genral recipient list.
			CTFPlayer *pPlayer = ToTFPlayer( pTarget );
			CSingleUserRecipientFilter singleFilter( pPlayer );
			EmitSound( singleFilter, entindex(), "Building_Sentrygun.AlertTarget" );
			filter.RemoveRecipient( pPlayer );

			// If the target is a bot, alert it.
			CTFBot *bot = ToTFBot( pPlayer );
			if ( bot )
			{
				bot->GetVisionInterface()->AddKnownEntity( this );
				bot->SetTargetSentry( this );
			}
		}

		EmitSound( filter, entindex(), "Building_Sentrygun.Alert" );
	}

	// Update timers, we are attacking now!
	m_iState.Set( SENTRY_STATE_ATTACKING );
	m_flNextAttack = gpGlobals->curtime + SENTRY_THINK_DELAY;
	if ( m_flNextRocketAttack < gpGlobals->curtime )
	{
		m_flNextRocketAttack = gpGlobals->curtime + 0.5f;
	}
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FInViewCone( CBaseEntity *pEntity )
{
	Vector forward;
	AngleVectors( m_vecCurAngles, &forward );

	Vector2D vec2LOS = ( pEntity->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	vec2LOS.NormalizeInPlace();

	if ( vec2LOS.Dot( forward.AsVector2D() ) > m_flFieldOfView )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Make sure our target is still valid, and if so, fire at it
//-----------------------------------------------------------------------------
void CObjectSentrygun::Attack()
{
	StudioFrameAdvance();

	if ( !FindTarget() )
	{
		m_iState.Set( SENTRY_STATE_SEARCHING );
		m_hEnemy = NULL;
		return;
	}

	// Track enemy.
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	QAngle angToTarget;
	VectorAngles( vecDirToEnemy, angToTarget );

	angToTarget.y = UTIL_AngleMod( angToTarget.y );
	if ( angToTarget.x < -180.0f )
	{
		angToTarget.x += 360.0f;
	}
	if ( angToTarget.x > 180.0f )
	{
		angToTarget.x -= 360.0f;
	}

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-50...50]
	if ( angToTarget.x > 50.0f )
	{
		angToTarget.x = 50.0f;
	}
	else if ( angToTarget.x < -50.0f )
	{
		angToTarget.x = -50.0f;
	}
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	MoveTurret();

	// Fire on the target if it's within 10 units of being aimed right at it.
	if ( m_flNextAttack <= gpGlobals->curtime && ( m_vecGoalAngles - m_vecCurAngles ).Length() <= 10 )
	{
		Fire();

		if ( m_iUpgradeLevel == 1 )
		{
			// Level 1 sentries fire slower.
			m_flNextAttack = gpGlobals->curtime + ( 0.2f * m_flFireRateMult );
		}
		else
		{
			m_flNextAttack = gpGlobals->curtime + ( 0.1f * m_flFireRateMult );
		}
	}
	else
	{
		//SetSentryAnim( TFTURRET_ANIM_SPIN );
	}
}

//-----------------------------------------------------------------------------
// Fire on our target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::Fire()
{
	// NDebugOverlay::Cross3D( m_hEnemy->WorldSpaceCenter(), 10, 255, 0, 0, false, 0.1 );

	Vector vecAimDir;

	// Level 3 Turrets fire rockets every 3 seconds.
	if ( m_iUpgradeLevel == 3 &&
		m_iAmmoRockets > 0 &&
		m_flNextRocketAttack < gpGlobals->curtime )
	{
		Vector vecSrc;
		QAngle vecAng;

		// Alternate between the 2 rocket launcher ports.
		if ( m_iAmmoRockets & 1 )
		{
			GetAttachment( m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L], vecSrc, vecAng );
		}
		else
		{
			GetAttachment( m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R], vecSrc, vecAng );
		}

		vecAimDir = m_hEnemy->WorldSpaceCenter() - vecSrc;
		vecAimDir.NormalizeInPlace();

		// NOTE: vecAng is not actually set by GetAttachment!!!
		QAngle angDir;
		VectorAngles( vecAimDir, angDir );

		EmitSound( "Building_Sentrygun.FireRocket" );

		AddGesture( ACT_RANGE_ATTACK2 );

		QAngle angAimDir;
		VectorAngles( vecAimDir, angAimDir );
		CBaseEntity *pAttacker = GetBuilder();
		if ( !pAttacker )
		{
			pAttacker = this;
		}

		CTFProjectile_SentryRocket *pProjectile = CTFProjectile_SentryRocket::Create( vecSrc, angAimDir, this, pAttacker );
		if ( pProjectile )
		{
			int iDamage = 100;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iDamage, mult_engy_sentry_damage );
			pProjectile->SetDamage( iDamage );
		}

		// Setup next rocket shot.
		m_flNextRocketAttack = gpGlobals->curtime + ( 3 * m_flFireRateMult );

		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoRockets--;
		}
	}

	// All turrets fire shells
	if ( m_iAmmoShells > 0 )
	{
		if ( !IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
		{
			RemoveGesture( ACT_RANGE_ATTACK1_LOW );
			AddGesture( ACT_RANGE_ATTACK1 );
		}

		Vector vecSrc;
		QAngle vecAng;

		int iAttachment;

		if ( m_iUpgradeLevel > 1 && ( m_iAmmoShells & 1 ) )
		{
			// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];
		}
		else
		{
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
		}

		GetAttachment( iAttachment, vecSrc, vecAng );

		// Alternate projectile.
		if ( m_iFireMode > 0 )
		{
			// Fire darts.
			vecAimDir = m_hEnemy->WorldSpaceCenter() - vecSrc;
			vecAimDir.NormalizeInPlace();

			vecSrc = vecSrc + vecAimDir * 32.0f;

			// NOTE: vecAng is not actually set by GetAttachment!!!
			QAngle angDir;
			VectorAngles( vecAimDir, angDir );

			QAngle angAimDir;
			VectorAngles( vecAimDir, angAimDir );
			CBaseEntity *pAttacker = GetBuilder();
			if ( !pAttacker )
			{
				pAttacker = this;
			}

			CTFProjectile_Dart *pProjectile = CTFProjectile_Dart::Create( this, vecSrc, angAimDir, pAttacker );
			if ( pProjectile )
			{
				int iDamage = 16;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iDamage, mult_engy_sentry_damage );
				pProjectile->SetDamage( iDamage );
				pProjectile->SetLauncher( this );
			}
		}
		else
		{
			// Fire bullets
			Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
			CBaseEntity *pAttacker = GetBuilder();
			bool bPassThroughTeammates = tf2c_bullets_pass_teammates.GetBool();

			trace_t tr;
			if (pAttacker && bPassThroughTeammates)
			{
				CTraceFilterIgnoreTeammatesExceptEntity filter(nullptr, COLLISION_GROUP_NONE, pAttacker->GetTeamNumber(), pAttacker);
				UTIL_TraceLine(vecSrc, vecMidEnemy, MASK_SOLID, &filter, &tr);
			}
			else
			{
				UTIL_TraceLine(vecSrc, vecMidEnemy, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
			}

			// If we cannot see their WorldSpaceCenter (possible, as we do our target finding based
			// on the eye position of the target) then fire at the eye position.
			tr = {};
			UTIL_TraceLine( vecSrc, vecMidEnemy, MASK_TFSHOT, this, COLLISION_GROUP_NONE, &tr );

			if ( !tr.m_pEnt || tr.m_pEnt->IsWorld() )
			{
				// Hack it lower a little bit..
				// The eye position is not always within the hitboxes for a standing TF Player
				vecMidEnemy = m_hEnemy->EyePosition() + Vector( 0, 0, -5 );
			}

			vecAimDir = vecMidEnemy - vecSrc;

			float flDistToTarget = vecAimDir.Length();

			vecAimDir.NormalizeInPlace();

			//NDebugOverlay::Cross3D( vecSrc, 10, 255, 0, 0, false, 0.1 );

			float flDamgeMultiplierRange = RemapValClamped( flDistToTarget + 100, GetMaxRangeDefault(), GetMaxRangeOverride(), 1.0f, tf_sentrygun_damage_falloff.GetFloat() );

			FireBulletsInfo_t info;

			info.m_vecSrc = vecSrc;
			info.m_vecDirShooting = vecAimDir;
			info.m_iTracerFreq = 1;
			info.m_iShots = 1;
			info.m_pAttacker = GetBuilder();
			info.m_vecSpread = vec3_origin;
			info.m_flDistance = flDistToTarget + 100;
			info.m_iAmmoType = m_iAmmoType;
			info.m_flDamage = tf_sentrygun_damage.GetFloat() * ( m_bCurrentlyTargetingOverriddenTarget ? flDamgeMultiplierRange : 1.0f );

			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.m_pAttacker, info.m_flDamage, mult_engy_sentry_damage );

			if (pAttacker && bPassThroughTeammates)
			{
				CTraceFilterIgnoreTeammatesExceptEntity filter(nullptr, COLLISION_GROUP_NONE, pAttacker->GetTeamNumber(), pAttacker);
				info.m_pCustomFilter = &filter;
				FireBullets(info);
			}
			else
			{
				FireBullets(info);
			}
		}

		UpdateNavMeshCombatStatus();

		//NDebugOverlay::Line( vecSrc, vecSrc + vecAimDir * 1000, 255, 0, 0, false, 0.1 );

		CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = iAttachment;
		data.m_fFlags = m_iUpgradeLevel;
		data.m_vOrigin = vecSrc;
		DispatchEffect( "TF_3rdPersonMuzzleFlash_SentryGun", data );

		switch ( m_iUpgradeLevel )
		{
			case 1:
			default:
				EmitSound( "Building_Sentrygun.Fire" );
				break;
			case 2:
				EmitSound( "Building_Sentrygun.Fire2" );
				break;
			case 3:
				EmitSound( "Building_Sentrygun.Fire3" );
				break;
		}

		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoShells--;
		}
	}
	else
	{
		if ( m_iUpgradeLevel > 1 )
		{
			if ( !IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
			{
				RemoveGesture( ACT_RANGE_ATTACK1 );
				AddGesture( ACT_RANGE_ATTACK1_LOW );
			}
		}

		// Out of ammo, play a click
		EmitSound( "Building_Sentrygun.Empty" );
		m_flNextAttack = gpGlobals->curtime + 0.2;
	}

	m_LastFireTime.Start();

	return true;
}


void CObjectSentrygun::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	// Using tr.startpos instead of vecTracerSrc to fix the stupid bit in CBaseEntity::ComputeTracerStartPosition 
	// that sets tracer start pos to 999,999,999 counting on attachment being used on client
	// which doesn't happen if the sentry is outside of PVS.

	// Sentryguns are perfectly accurate, but this doesn't look good for tracers.
	// Add a little noise to them, but not enough so that it looks like they're missing.
	Vector vecEnd = tr.endpos + RandomVector( -10, 10 );

	const char *pszEffect = ConstructTeamParticle( "bullet_tracer01_%s", GetTeamNumber() );
	int iParticleIndex = GetParticleSystemIndex( pszEffect );
	UTIL_Tracer( tr.startpos, vecEnd, entindex(), GetTracerAttachment(), 0.0f, true, "TFParticleTracer", iParticleIndex );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CObjectSentrygun::GetPlacementModel( void ) const
{
	return SENTRY_MODEL_PLACEMENT;
}

//-----------------------------------------------------------------------------
// Purpose: MakeTracer asks back for the attachment index
//-----------------------------------------------------------------------------
int	CObjectSentrygun::GetTracerAttachment( void )
{
	// Level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
	if ( m_iUpgradeLevel > 1 && ( m_iAmmoShells & 1 ) )
		return m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];

	return m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
}

//-----------------------------------------------------------------------------
// Rotate and scan for targets
//-----------------------------------------------------------------------------
void CObjectSentrygun::SentryRotate( void )
{
	// If we're playing a fire gesture, stop it.
	if ( IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1 );
	}

	if ( IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1_LOW );
	}

	// Animate.
	StudioFrameAdvance();

	// Look for a target.
	if ( FindTarget() )
		return;

	// Rotate.
	if ( !MoveTurret() )
	{
		// Change direction.
		if ( IsDisabled() )
		{
			if ( tf2c_building_gun_mettle.GetBool() )
			{
				m_flEnableDelay = gpGlobals->curtime + SENTRY_ENABLE_DELAY;
			}

			EmitSound( "Building_Sentrygun.Disabled" );
			m_vecGoalAngles.x = 30;
		}
		else
		{
			switch ( m_iUpgradeLevel )
			{
				case 1:
				default:
					EmitSound( "Building_Sentrygun.Idle" );
					break;
				case 2:
					EmitSound( "Building_Sentrygun.Idle2" );
					break;
				case 3:
					EmitSound( "Building_Sentrygun.Idle3" );
					break;
			}

			// Switch rotation direction
			if ( m_bTurningRight )
			{
				m_bTurningRight = false;
				m_vecGoalAngles.y = m_iLeftBound;
			}
			else
			{
				m_bTurningRight = true;
				m_vecGoalAngles.y = m_iRightBound;
			}

			// Randomly look up and down a bit
			if ( random->RandomFloat( 0, 1 ) < 0.3f )
			{
				m_vecGoalAngles.x = (int)random->RandomFloat( -10, 10 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnStartDisabled( void )
{
	// Stay at current rotation, angle down.
	m_vecGoalAngles.x = m_vecCurAngles.x;
	m_vecGoalAngles.y = m_vecCurAngles.y;

	//target = NULL

	BaseClass::OnStartDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: Remove the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnEndDisabled( void )
{
	// Return to normal rotations.
	if ( m_bTurningRight )
	{
		m_bTurningRight = false;
		m_vecGoalAngles.y = m_iLeftBound;
	}
	else
	{
		m_bTurningRight = true;
		m_vecGoalAngles.y = m_iRightBound;
	}

	m_vecGoalAngles.x = 0;

	BaseClass::OnEndDisabled();
}


int CObjectSentrygun::GetBaseTurnRate( void )
{
	if ( m_bCurrentlyTargetingOverriddenTarget )
	{
		return m_iTurnRateOverride;
	}

	float flTurnRate = m_iBaseTurnRate;
	if ( GetBuilder() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flTurnRate, mult_sentry_turnrate );
	}

	return flTurnRate;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::MoveTurret( void )
{
	bool bMoved = false;

	int iBaseTurnRate = GetBaseTurnRate();

	// Any x movement?
	if ( m_vecCurAngles.x != m_vecGoalAngles.x )
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += SENTRY_THINK_DELAY * ( iBaseTurnRate * 5 ) * flDir;

		// If we started below the goal, and now we're past, peg to goal
		if ( flDir == 1 )
		{
			if ( m_vecCurAngles.x > m_vecGoalAngles.x )
			{
				m_vecCurAngles.x = m_vecGoalAngles.x;
			}
		} 
		else
		{
			if ( m_vecCurAngles.x < m_vecGoalAngles.x )
			{
				m_vecCurAngles.x = m_vecGoalAngles.x;
			}
		}

		SetPoseParameter( m_iPitchPoseParameter, -m_vecCurAngles.x );

		bMoved = true;
	}

	if ( m_vecCurAngles.y != m_vecGoalAngles.y )
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs( m_vecGoalAngles.y - m_vecCurAngles.y );
		bool bReversed = false;

		if ( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
			bReversed = true;
		}

		if ( !m_hEnemy.Get() )
		{
			if ( flDist > 30 )
			{
				if ( m_flTurnRate < ( iBaseTurnRate * 10 ) )
				{
					m_flTurnRate += iBaseTurnRate;
				}
			}
			else
			{
				// Slow down
				if ( m_flTurnRate > ( iBaseTurnRate * 5 ) )
				{
					m_flTurnRate -= iBaseTurnRate;
				}
			}
		}
		else
		{
			// When tracking enemies, move faster and don't slow
			if ( flDist > 30 )
			{
				if ( m_flTurnRate < ( iBaseTurnRate * 30 ) )
				{
					m_flTurnRate += iBaseTurnRate * 3;
				}
			}
		}

		m_vecCurAngles.y += SENTRY_THINK_DELAY * m_flTurnRate * flDir;

		// If we passed over the goal, peg right to it now.
		if ( flDir == -1 )
		{
			if ( ( !bReversed && m_vecGoalAngles.y > m_vecCurAngles.y ) ||
				( bReversed && m_vecGoalAngles.y < m_vecCurAngles.y ) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		} 
		else
		{
			if ( ( !bReversed && m_vecGoalAngles.y < m_vecCurAngles.y ) ||
                ( bReversed && m_vecGoalAngles.y > m_vecCurAngles.y ) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		}

		if ( m_vecCurAngles.y < 0 )
		{
			m_vecCurAngles.y += 360;
		}
		else if ( m_vecCurAngles.y >= 360 )
		{
			m_vecCurAngles.y -= 360;
		}

		if ( flDist < ( SENTRY_THINK_DELAY * 0.5f * iBaseTurnRate ) )
		{
			m_vecCurAngles.y = m_vecGoalAngles.y;
		}

		QAngle angles = GetAbsAngles();
		SetPoseParameter( m_iYawPoseParameter, -( m_vecCurAngles.y - angles.y ) );

		InvalidatePhysicsRecursive( ANIMATION_CHANGED );

		bMoved = true;
	}

	if ( !bMoved || m_flTurnRate <= 0 )
	{
		m_flTurnRate = iBaseTurnRate * 5;
	}

	return bMoved;
}

//-----------------------------------------------------------------------------
// Purpose: Note our last attacked time
//-----------------------------------------------------------------------------
int CObjectSentrygun::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;

	// As we increase in level, we get more resistant to minigun bullets, to compensate for
	// our increased surface area taking more minigun hits.
	if ( ( info.GetDamageType() & DMG_BULLET ) && info.GetDamageCustom() == TF_DMG_CUSTOM_MINIGUN )
	{
		newInfo.SetDamage( ( newInfo.GetDamage() * ( 1.0f - m_flHeavyBulletResist ) ) );
	}

	// Check to see if we are being sapped.
	if ( HasSapper() )
	{
		// Get the sapper owner.
		CBaseObject *pSapper = GetObjectOfTypeOnMe( OBJ_ATTACHMENT_SAPPER );
		Assert( pSapper );

		// Take less damage if the owner is causing additional damage.
		if ( pSapper && ( info.GetAttacker() == pSapper->GetBuilder() ) )
		{
			float flSentryDmgMult = tf2c_spy_gun_mettle.GetInt() == 1 ? GUN_METTLE_SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER : SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER;
			newInfo.SetDamage( newInfo.GetDamage() * flSentryDmgMult );
		}
	}

	int iDamageTaken = BaseClass::OnTakeDamage( newInfo );
	if ( iDamageTaken > 0 )
	{
		m_flLastAttackedTime = gpGlobals->curtime;
	}

	return iDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: Called when this object is destroyed
//-----------------------------------------------------------------------------
void CObjectSentrygun::Killed( const CTakeDamageInfo &info )
{
	// Do normal handling.
	BaseClass::Killed( info );
}


void CObjectSentrygun::SetModel( const char *pModel )
{
	float flPoseParam0 = 0.0f;
	float flPoseParam1 = 0.0f;

	// Save pose parameters across model change.
	if ( m_iPitchPoseParameter >= 0 )
	{
		flPoseParam0 = GetPoseParameter( m_iPitchPoseParameter );
	}

	if ( m_iYawPoseParameter >= 0 )
	{
		flPoseParam1 = GetPoseParameter( m_iYawPoseParameter );
	}

	BaseClass::SetModel( pModel );

	// Reset this after model change.
	UTIL_SetSize( this, SENTRYGUN_MINS, SENTRYGUN_MAXS );
	SetSolid( SOLID_BBOX );

	// Restore pose parameters.
	m_iPitchPoseParameter = LookupPoseParameter( "aim_pitch" );
	m_iYawPoseParameter = LookupPoseParameter( "aim_yaw" );

	SetPoseParameter( m_iPitchPoseParameter, flPoseParam0 );
	SetPoseParameter( m_iYawPoseParameter, flPoseParam1 );

	CreateBuildPoints();

	ReattachChildren();

	ResetSequenceInfo();
}


void CObjectSentrygun::ModifyFireBulletsDamage( CTakeDamageInfo* dmgInfo )
{
	dmgInfo->SetWeapon( this );
}


CTFPlayer *CObjectSentrygun::GetAssistingTeammate( float maxAssistDuration ) const
{
	if ( !m_lastTeammateWrenchHitTimer.HasStarted() || m_lastTeammateWrenchHitTimer.IsGreaterThan( maxAssistDuration ) )
		return NULL;

	return m_lastTeammateWrenchHit;
}

//-----------------------------------------------------------------------------
// Purpose: Inform bots of sentry danger via the nav mesh's "in-combat" system
//-----------------------------------------------------------------------------
void CObjectSentrygun::UpdateNavMeshCombatStatus()
{
	if ( m_ctNavCombatUpdate.IsElapsed() )
	{
		m_ctNavCombatUpdate.Start( 1.0f );

		UpdateLastKnownArea();
		GetLastKnownTFArea()->AddCombatToSurroundingAreas( 5 );
	}
}

LINK_ENTITY_TO_CLASS( tf_projectile_sentryrocket, CTFProjectile_SentryRocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( CTFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Creation
//-----------------------------------------------------------------------------
CTFProjectile_SentryRocket *CTFProjectile_SentryRocket::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pAttacker )
{
	CTFProjectile_SentryRocket *pRocket = static_cast<CTFProjectile_SentryRocket *>( CTFBaseRocket::Create( NULL, "tf_projectile_sentryrocket", vecOrigin, vecAngles, pOwner, TF_PROJECTILE_SENTRY_ROCKET ) );
	if ( pRocket )
	{
		pRocket->SetLauncher( pOwner );
		pRocket->m_hAttacker = pAttacker;
	}

	return pRocket;
}

CTFProjectile_SentryRocket::CTFProjectile_SentryRocket()
{
	UseClientSideAnimation();
}


void CTFProjectile_SentryRocket::Spawn()
{
	BaseClass::Spawn();

	SetModel( SENTRY_ROCKET_MODEL );

	ResetSequence( LookupSequence( "idle" ) );
}


CBaseEntity *CTFProjectile_SentryRocket::GetAttacker( void )
{
	if ( m_iDeflected > 0 )
		return BaseClass::GetAttacker();
	
	return m_hAttacker;
}




//FLAMETHROWER SENTRY
//Why? Flamethrower sentry. Thats why

BEGIN_NETWORK_TABLE_NOBASE(CObjectFlameSentry, DT_FlameSentryLocalData)
SendPropInt(SENDINFO(m_iKills), 12, SPROP_CHANGES_OFTEN),
SendPropInt(SENDINFO(m_iAssists), 12, SPROP_CHANGES_OFTEN),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST(CObjectFlameSentry, DT_ObjectFlameSentry)
SendPropInt(SENDINFO(m_iAmmoShells), 9, SPROP_CHANGES_OFTEN),
SendPropInt(SENDINFO(m_iAmmoRockets), 6, SPROP_CHANGES_OFTEN),
SendPropInt(SENDINFO(m_iState), Q_log2(SENTRY_NUM_STATES) + 1, SPROP_UNSIGNED),
SendPropDataTable("FlameSentryLocalData", 0, &REFERENCE_SEND_TABLE(DT_FlameSentryLocalData), SendProxy_SendLocalObjectDataTable),
END_SEND_TABLE()

BEGIN_DATADESC(CObjectFlameSentry)
END_DATADESC()

LINK_ENTITY_TO_CLASS(obj_flamesentry, CObjectFlameSentry);
PRECACHE_REGISTER(obj_flamesentry);


CObjectFlameSentry::CObjectFlameSentry()
{
	SetMaxHealth(175);
	m_iHealth = 175;
	SetType(OBJ_FLAMESENTRY);
	m_lastTeammateWrenchHit = NULL;
	m_lastTeammateWrenchHitTimer.Invalidate();
	m_flRangeMultiplier = 0.5f;
	m_flRangeMultiplierOverridenTarget = 1.0f;
	m_bCurrentlyTargetingOverriddenTarget = false;
	m_flFireRateMult = 0.3f;

}

int CObjectFlameSentry::GetBaseHealth(void) const
{
	return 175;
}


bool CObjectFlameSentry::StartBuilding(CBaseEntity* pBuilder)
{
	UseClientSideAnimation();

	SetModel(SENTRY_MODEL_FLAME_UPGRADE);

	CreateBuildPoints();

	SetPoseParameter(m_iPitchPoseParameter, 0.0f);
	SetPoseParameter(m_iYawPoseParameter, 0.0f);

	return BaseClass::BaseClass::StartBuilding(pBuilder);
}

void CObjectFlameSentry::FinishUpgrading(void)
{
	BaseClass::FinishUpgrading();
	SetModel(SENTRY_MODEL_FLAME);
}



bool CObjectFlameSentry::Fire()
{
	//NDebugOverlay::Cross3D( m_hEnemy->WorldSpaceCenter(), 10, 255, 0, 0, false, 0.1 );

	Vector vecAimDir;

	// All turrets fire shells
	if (m_iAmmoShells > 0)
	{
		if (!IsPlayingGesture(ACT_RANGE_ATTACK1))
		{
			RemoveGesture(ACT_RANGE_ATTACK1_LOW);
			//AddGesture(ACT_RANGE_ATTACK1);
		}

		Vector vecSrc;
		QAngle vecAng;

		int iAttachment;

		if (m_iUpgradeLevel > 1 && (m_iAmmoShells & 1))
		{
			// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];
		}
		else
		{
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
		}

		GetAttachment(iAttachment, vecSrc, vecAng);

		
		// Fire bullets
		Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
		CBaseEntity* pAttacker = GetBuilder();
		bool bPassThroughTeammates = tf2c_bullets_pass_teammates.GetBool();

		trace_t tr;
		if (pAttacker && bPassThroughTeammates)
		{
			CTraceFilterIgnoreTeammatesExceptEntity filter(nullptr, COLLISION_GROUP_NONE, pAttacker->GetTeamNumber(), pAttacker);
			UTIL_TraceLine(vecSrc, vecMidEnemy, MASK_SOLID, &filter, &tr);
		}
		else
		{
			UTIL_TraceLine(vecSrc, vecMidEnemy, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
		}

		// If we cannot see their WorldSpaceCenter (possible, as we do our target finding based
		// on the eye position of the target) then fire at the eye position.
		tr = {};
		UTIL_TraceLine(vecSrc, vecMidEnemy, MASK_TFSHOT, this, COLLISION_GROUP_NONE, &tr);

		if (!tr.m_pEnt || tr.m_pEnt->IsWorld())
		{
			// Hack it lower a little bit..
			// The eye position is not always within the hitboxes for a standing TF Player
			vecMidEnemy = m_hEnemy->EyePosition() + Vector(0, 0, -5);
		}

		vecAimDir = vecMidEnemy - vecSrc;

		float flDistToTarget = vecAimDir.Length();

		vecAimDir.NormalizeInPlace();

		//NDebugOverlay::Cross3D( vecSrc, 10, 255, 0, 0, false, 0.1 );

		float flDamgeMultiplierRange = RemapValClamped(flDistToTarget + 100, GetMaxRangeDefault(), GetMaxRangeOverride(), 1.0f, tf_sentrygun_damage_falloff.GetFloat());

		FireBulletsInfo_t info;

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecAimDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = GetBuilder();
		info.m_vecSpread = vec3_origin;
		info.m_flDistance = flDistToTarget + 100;
		info.m_iAmmoType = m_iAmmoType;
		info.m_flDamage = tf_sentrygun_damage.GetFloat() * (m_bCurrentlyTargetingOverriddenTarget ? flDamgeMultiplierRange : 1.0f);

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.m_pAttacker, info.m_flDamage, mult_engy_sentry_damage);

		if (pAttacker && bPassThroughTeammates)
		{
			CTraceFilterIgnoreTeammatesExceptEntity filter(nullptr, COLLISION_GROUP_NONE, pAttacker->GetTeamNumber(), pAttacker);
			info.m_pCustomFilter = &filter;
			m_Flames[m_Flames.AddToTail()].Init(vecSrc, vecAng, pAttacker, this);
		}
		else
		{
			m_Flames[m_Flames.AddToTail()].Init(vecSrc, vecAng, pAttacker, this );
		}
		

		UpdateNavMeshCombatStatus();

		//NDebugOverlay::Line( vecSrc, vecSrc + vecAimDir * 1000, 255, 0, 0, false, 0.1 );

		CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = iAttachment;
		data.m_fFlags = m_iUpgradeLevel;
		data.m_vOrigin = vecSrc;
		//DispatchEffect("TF_3rdPersonMuzzleFlash_SentryGun", data);

		if (!tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags(SF_SENTRY_INFINITE_AMMO))
		{
			m_iAmmoShells--;
		}
	}
	else
	{
		if (m_iUpgradeLevel > 1)
		{
			if (!IsPlayingGesture(ACT_RANGE_ATTACK1_LOW))
			{
				RemoveGesture(ACT_RANGE_ATTACK1);
				AddGesture(ACT_RANGE_ATTACK1_LOW);
			}
		}

		// Out of ammo, play a click
		EmitSound("Building_Sentrygun.Empty");
		m_flNextAttack = gpGlobals->curtime + 0.2;
	}

	m_LastFireTime.Start();

	return true;
}

void CObjectFlameSentry::Attack()
{
	StudioFrameAdvance();

	if (!FindTarget())
	{
		m_iState.Set(SENTRY_STATE_SEARCHING);
		m_hEnemy = NULL;
		return;
	}

	// Track enemy.
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	QAngle angToTarget;
	VectorAngles(vecDirToEnemy, angToTarget);

	angToTarget.y = UTIL_AngleMod(angToTarget.y);
	if (angToTarget.x < -180.0f)
	{
		angToTarget.x += 360.0f;
	}
	if (angToTarget.x > 180.0f)
	{
		angToTarget.x -= 360.0f;
	}

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-50...50]
	if (angToTarget.x > 50.0f)
	{
		angToTarget.x = 50.0f;
	}
	else if (angToTarget.x < -50.0f)
	{
		angToTarget.x = -50.0f;
	}
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	MoveTurret();

	// Fire on the target if it's within 10 units of being aimed right at it.
	if (m_flNextAttack <= gpGlobals->curtime)
	{
		Fire();

		if (m_iUpgradeLevel == 1)
		{
			// Level 1 sentries fire slower.
			m_flNextAttack = gpGlobals->curtime + (0.2f * m_flFireRateMult);
		}
		else
		{
			m_flNextAttack = gpGlobals->curtime + (0.1f * m_flFireRateMult);
		}
	}
	else
	{
		//SetSentryAnim( TFTURRET_ANIM_SPIN );
	}
}

void CObjectFlameSentry::SentryThink(void)
{
	// Don't think while re-deploying so we don't target anything inbetween upgrades.
	if (IsRedeploying())
	{
		SetContextThink(&CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT);
		return;
	}

	SimulateFlames();

	switch (m_iState)
	{
	case SENTRY_STATE_INACTIVE:
		break;

	case SENTRY_STATE_SEARCHING:
		SentryRotate();
		break;

	case SENTRY_STATE_ATTACKING:
		Attack();
		break;

	case SENTRY_STATE_UPGRADING:
		UpgradeThink();
		break;
	}

	SetContextThink(&CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT);
}

void CObjectFlameSentry::SimulateFlames(void)
{
	// Is this the best way? Have to do this since flames 
	// aren't real entities, but it seems fine for now though.
	if (TFGameRules()->InRoundRestart())
	{
		m_Flames.RemoveAll();
	}

	// Simulate all flames.
	m_bSimulatingFlames = true;
	for (int i = m_Flames.Count() - 1; i >= 0; i--)
	{
		if (!m_Flames[i].FlameThink())
		{
			m_Flames.FastRemove(i);
		}
	}
	m_bSimulatingFlames = false;

}

const char* CObjectFlameSentry::GetPlacementModel(void) const
{
	return SENTRY_MODEL_FLAME;
}

void CObjectFlameSentry::OnGoActive(void)
{
	BaseClass::OnGoActive();
	SetModel(SENTRY_MODEL_FLAME);
	m_flFireRateMult = 0.33f;
	m_flRangeMultiplier = 0.5f;
	m_flRangeMultiplierOverridenTarget = 1.0f;
}

bool CObjectFlameSentry::CanBeUpgraded(CTFPlayer* pPlayer)
{
	return false;
}


//Flame physics
void CTFFlameEntitySentry::Init(const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity* pOwner, CObjectFlameSentry* pBuilding)
{
	m_vecOrigin = m_vecPrevPos = m_vecInitialPos = vecOrigin;
	m_hOwner = pOwner;
	m_pOuter = pBuilding;

	float flBoxSize = tf_flamesentry_boxsize.GetFloat();
	m_vecMins.Init(-flBoxSize, -flBoxSize, -flBoxSize);
	m_vecMaxs.Init(flBoxSize, flBoxSize, flBoxSize);

	// Set team.
	m_iTeamNum = pBuilding->GetTeamNumber();
	m_iDmgType = DMG_BURN;

	m_flDmgAmount = tf2c_flamesentry_damage.GetInt();

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors(vecAngles, &vecForward, &vecRight, &vecUp);

	float velocity = tf_flamesentry_velocity.GetFloat();
	m_vecBaseVelocity = vecForward * velocity;
	m_vecBaseVelocity += RandomVector(-velocity * tf_flamesentry_vecrand.GetFloat(), velocity * tf_flamesentry_vecrand.GetFloat());
	m_vecAttackerVelocity = pBuilding->GetAbsVelocity();
	m_vecVelocity = m_vecBaseVelocity;

	m_flTimeRemove = gpGlobals->curtime + (tf_flamesentry_flametime.GetFloat() * random->RandomFloat(0.9, 1.1));
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
bool CTFFlameEntitySentry::FlameThink(void)
{
	// Remove it if it's expired.
	if (gpGlobals->curtime >= m_flTimeRemove)
		return false;


	// Do collision detection. We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server.
	if (m_vecOrigin != m_vecPrevPos)
	{
		// TF2C: Do world collision detection to enable flames to slide along surfaces
		// Fixes cases of damage boxes giving up too soon and not lining up with particles.
		if (tf2c_flamesentry_wallslide.GetBool())
		{
			trace_t traceWorld;
			CTraceFilterWorldAndPropsOnly traceFilter;
			UTIL_TraceLine(m_vecPrevPos, m_vecOrigin, MASK_SHOT, &traceFilter, &traceWorld);

			if (traceWorld.DidHit())
			{
				// Move back out first.
				m_vecOrigin = m_vecPrevPos;

				// Change direction to slide along surface.
				m_vecBaseVelocity = m_vecBaseVelocity - (DotProduct(m_vecBaseVelocity, traceWorld.plane.normal) * traceWorld.plane.normal);

				if (tf_debug_flamesentry.GetInt())
				{
					NDebugOverlay::Cross3D(traceWorld.endpos, m_vecMins, m_vecMaxs, 255, 255, 0, false, 0.2f);
				}
			}
		}

		/*CTFTeam *pTeam = pAttacker->GetOpposingTFTeam();
		if ( !pTeam )
			return;*/


		// HACK: Changed the first argument to '0' from 'm_iTeamNum', but it allows the Huntsman's arrow to light up.
		ForEachEnemyTFTeam(0, [&](int iTeam)
			{
				CTFTeam* pTeam = GetGlobalTFTeam(iTeam);
		if (!pTeam)
			return true;

		// check collision against all enemy players
		for (int iPlayer = 0, iPlayers = pTeam->GetNumPlayers(); iPlayer < iPlayers; iPlayer++)
		{
			// Is this player connected, alive, and an enemy?
			CBasePlayer* pPlayer = pTeam->GetPlayer(iPlayer);
			if (pPlayer && pPlayer != m_hOwner && pPlayer->IsConnected() && pPlayer->IsAlive())
			{
				if ( CheckCollision(pPlayer) )
					return false;
			}
		}

		// Check collision against all enemy objects.
		for (int iObject = 0, iObjects = pTeam->GetNumObjects(); iObject < iObjects; iObject++)
		{
			CBaseObject* pObject = pTeam->GetObject(iObject);
			if (pObject)
			{
				if ( CheckCollision(pObject) )
					return false;
			}
		}

		return true;
			});

		for (CTFProjectile_Arrow* pEntity : TAutoList<CTFProjectile_Arrow>::AutoList())
			CheckCollision(pEntity);


		// Check collision against all enemy non-player NextBots.
		CUtlVector<INextBot*> nextbots;
		TheNextBots().CollectAllBots(&nextbots);

		for (auto nextbot : nextbots)
		{
			CBaseCombatCharacter* pBot = nextbot->GetEntity();
			if (pBot != nullptr && !pBot->IsPlayer() && pBot->IsAlive())
			{
				if ( CheckCollision(pBot) )
					return false;
			}
		}
	}

	// Calculate how long the flame has been alive for.
	float flFlameElapsedTime = tf_flamesentry_flametime.GetFloat() - (m_flTimeRemove - gpGlobals->curtime);
	// Calculate how much of the attacker's velocity to blend in to the flame's velocity. The flame gets the attacker's velocity
	// added right when the flame is fired, but that velocity addition fades quickly to zero.
	float flAttackerVelocityBlend = RemapValClamped(flFlameElapsedTime, tf_flamesentry_velocityfadestart.GetFloat(),
		tf_flamesentry_velocityfadeend.GetFloat(), 1.0f, 0);

	// Reduce our base velocity by the air drag constant.
	m_vecBaseVelocity *= tf_flamesentry_drag.GetFloat();

	// Add our float upward velocity.
	m_vecVelocity = m_vecBaseVelocity + Vector(0, 0, tf_flamesentry_float.GetFloat()) + (flAttackerVelocityBlend * m_vecAttackerVelocity);

	// Render debug visualization if convar on
	if (tf_debug_flamesentry.GetInt())
	{
		if (m_hEntitiesBurnt.Count() > 0)
		{
			int val = (int)(gpGlobals->curtime * 10) % 255;
			NDebugOverlay::Box(m_vecOrigin, m_vecMins, m_vecMaxs, val, 255, val, 0, 0);
			//NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 ); bn 
		}
		else
		{
			NDebugOverlay::Box(m_vecOrigin, m_vecMins, m_vecMaxs, 0, 100, 255, 0, 0);
			//NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	//SetNextThink( gpGlobals->curtime );

	// Move!
	m_vecPrevPos = m_vecOrigin;
	m_vecOrigin += m_vecVelocity * gpGlobals->frametime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities.
//-----------------------------------------------------------------------------
bool CTFFlameEntitySentry::CheckCollision(CBaseEntity* pOther)
{
	bool pbHitWorld = false;

	// If we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity.
	int iIndex = m_hEntitiesBurnt.Find(pOther);
	if (iIndex != m_hEntitiesBurnt.InvalidIndex())
		return false;

	// Do a bounding box check against the entity.
	Vector vecMins, vecMaxs;
	pOther->GetCollideable()->WorldSpaceSurroundingBounds(&vecMins, &vecMaxs);
	CBaseTrace trace;
	Ray_t ray;
	float flFractionLeftSolid;
	ray.Init(m_vecPrevPos, m_vecOrigin, m_vecMins, m_vecMaxs);
	if (IntersectRayWithBox(ray, vecMins, vecMaxs, 0.0, &trace, &flFractionLeftSolid))
	{
		// If bounding box check passes, check player hitboxes.
		trace_t trHitbox;
		trace_t trWorld;
		trace_t trWorld2;
		trace_t trWorld3;
		trace_t trWorld4;

		Vector vecMinsTarget, vecMaxsTarget;
		pOther->GetCollideable()->WorldSpaceSurroundingBounds(&vecMinsTarget, &vecMaxsTarget);

		bool bTested = pOther->GetCollideable()->TestHitboxes(ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox);
		if (!bTested || !trHitbox.DidHit())
			return false;

		// Now, let's see if the flame visual could have actually hit this player. Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything. Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine(m_vecInitialPos, m_vecOrigin + vDir * m_vecMins.x, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld);
		UTIL_TraceLine(m_vecInitialPos, m_vecOrigin - vDir * m_vecMins.x, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld2);
		UTIL_TraceLine(vecMinsTarget, m_vecOrigin, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld3);
		UTIL_TraceLine(vecMaxsTarget, m_vecOrigin, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld4);


		if (tf_debug_flamesentry.GetInt())
		{
			NDebugOverlay::Line(trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f);
			NDebugOverlay::Line(trWorld2.startpos, trWorld2.endpos, 255, 0, 0, true, 3.0f);
			NDebugOverlay::Line(trWorld3.startpos, trWorld3.endpos, 0, 0, 255, true, 3.0f);
			NDebugOverlay::Line(trWorld4.startpos, trWorld4.endpos, 0, 0, 255, true, 3.0f);
		}

		if (trWorld.fraction == 1.0f && trWorld2.fraction == 1.0f && (trWorld3.fraction == 1.0f || trWorld4.fraction == 1.0f))
		{
			// If there is nothing solid in the way, damage the entity.
			if (pOther->IsPlayer() && pOther->InSameTeam(m_pOuter))
			{
				OnCollideWithTeammate(ToTFPlayer(pOther));
			}
			else
			{
				OnCollide(pOther);
			}
		}
		else
		{
			// We hit the world, remove ourselves.
			pbHitWorld = true;
		}
	}
	return pbHitWorld;
}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity.
//-----------------------------------------------------------------------------
void CTFFlameEntitySentry::OnCollide(CBaseEntity* pOther)
{
	int nContents = UTIL_PointContents(m_vecOrigin);
	if ((nContents & MASK_WATER))
	{
		m_flTimeRemove = gpGlobals->curtime;
		return;
	}

	if (!pOther || (pOther->IsBaseObject() && pOther->InSameTeam(m_pOuter)))
		return;

	// Remember that we've burnt this entity
	m_hEntitiesBurnt.AddToTail(pOther);


	if (V_strcmp(pOther->GetClassname(), "tf_projectile_arrow") == 0)
	{
		// Light any arrows that fly by.
		CTFProjectile_Arrow* pArrow = assert_cast<CTFProjectile_Arrow*>(pOther);
		pArrow->SetArrowAlight(true);
		return;
	}

	float flDistance = m_vecOrigin.DistToSqr(m_vecInitialPos);
	float flMultiplier;
	constexpr const int shortRangeDist = (125 * 125);
	if (flDistance <= shortRangeDist)
	{
		// At very short range, apply short range damage multiplier.
		flMultiplier = tf_flamesentry_shortrangedamagemultiplier.GetFloat();
	}
	else
	{
		// Make damage ramp down from 100% to 60% from half the max dist to the max dist.
		flMultiplier = RemapValClamped(FastSqrt(flDistance), tf_flamesentry_maxdamagedist.GetFloat() / 2, tf_flamesentry_maxdamagedist.GetFloat(), 1.0f, 0.6f);
	}


	float flDamage = m_flDmgAmount * flMultiplier;
	flDamage = Max(flDamage, 1.0f);
	if (tf_debug_flamesentry.GetInt())
	{
		Msg("Flame touch dmg: %.1f\n", flDamage);
	}

	//unsure if necessary, commenting out    
	//m_pOuter->SetHitTarget();

	CTakeDamageInfo info(m_pOuter, m_hOwner, m_pOuter, Vector(0, 0, 0), m_pOuter->EyePosition(), flDamage, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_BURNING);

	// We collided with pOther, so try to find a place on their surface to show blood.
	trace_t trace;
	UTIL_TraceLine(m_vecOrigin, pOther->WorldSpaceCenter(), MASK_SOLID | CONTENTS_HITBOX, NULL, COLLISION_GROUP_NONE, &trace);

	pOther->DispatchTraceAttack(info, m_vecVelocity, &trace);

	// Manually apply afterburn since damage does not
	CTFPlayer *pTFVictim = ToTFPlayer( pOther );
	CTFPlayer *pTFBuilder = m_pOuter->GetBuilder();
	if ( pTFBuilder && pTFVictim )
	{
		// Don't ignite ubered enemies
		if ( pTFVictim->m_Shared.IsInvulnerable() )
		{
			return;
		}

		pTFVictim->m_Shared.Burn( pTFBuilder, pTFBuilder->GetActiveTFWeapon() );
	}

	ApplyMultiDamage();
}


void CTFFlameEntitySentry::OnCollideWithTeammate(CTFPlayer* pPlayer)
{
	// Only care about Snipers
	if (!pPlayer->IsPlayerClass(TF_CLASS_SNIPER))
		return;

	int iIndex = m_hEntitiesBurnt.Find(pPlayer);
	if (iIndex != m_hEntitiesBurnt.InvalidIndex())
		return;

	m_hEntitiesBurnt.AddToTail(pPlayer);

	// Does he have the bow?
	CTFWeaponBase* pActiveWeapon = pPlayer->GetActiveTFWeapon();
	if (pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW)
	{
		static_cast<CTFCompoundBow*>(pActiveWeapon)->SetArrowAlight(true);
	}
}

