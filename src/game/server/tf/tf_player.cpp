//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for TF2Classic.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "KeyValues.h"
#include "viewport_panel_names.h"
#include "client.h"
#include "team.h"
#include "tf_weaponbase.h"
#include "tf_client.h"
#include "tf_team.h"
#include "tf_viewmodel.h"
#include "tf_item.h"
#include "in_buttons.h"
#include "entity_capture_flag.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "game.h"
#include "tf_weapon_builder.h"
#include "tf_obj.h"
#include "tf_ammo_pack.h"
#include "datacache/imdlcache.h"
#include "particle_parse.h"
#include "props_shared.h"
#include "filesystem.h"
#include "toolframework_server.h"
#include "IEffects.h"
#include "func_respawnroom.h"
#include "networkstringtable_gamedll.h"
#include "tf_control_point_master.h"
#include "tf_weapon_pda.h"
#include "sceneentity.h"
#include "fmtstr.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_minigun.h"
#include "trigger_capture_area.h"
#include "triggers.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_umbrella.h"
#include "hl2orange.spa.h"
#include "te_tfblood.h"
#include "activitylist.h"
#include "steam/steam_api.h"
#include "cdll_int.h"
#include "tf_weaponbase.h"
#include "tf_weapon_mirv.h"
#include "tf_wearable.h"
#include "tf_inventory.h"
#include "econ_item_schema.h"
#include "baseprojectile.h"
#include "tf_weapon_grenade_mirv.h"
#include "eventlist.h"
#include "tf_fx.h"
#include "tf_nav_area.h"
#include "tf_bot.h"
#include "animation.h"
#include "tf_player_resource.h"
#include "entity_healthkit.h"
#include "voice_gamemgr.h"
#include "player_command.h"
#include "engine/IEngineSound.h"
#include "choreoscene.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "trains.h"
#include "tf_train_watcher.h"
#include "soundenvelope.h"

#ifdef TF2C_BETA
#include "tf_weapon_heallauncher.h"
#include "tf_weapon_anchor.h"
#include "tf_weapon_pillstreak.h" // !!! foxysen pillstreak
#endif

#include "NextBotManager.h"

#include "achievements_tf.h"
#include "tf_randomizer_manager.h"
#include "tf_voteissues.h"

#include "tf_weapon_riot.h"
#include "tf_weapon_detonator.h"	// !!! foxysen detonator
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_compound_bow.h"

//#include "tf_weapon_grenade_stickybomb.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DAMAGE_FORCE_SCALE_SELF				9
#define JUMPPAD_STOMP_DAMAGE_SCALE			3

#define PILLSTREAK_DEATH_EXPLOSION_DAMAGE		60
#define PILLSTREAK_DEATH_EXPLOSION_RADIUS		136
#define GENERIC_DEATH_EXPLOSION_DAMAGE			125
#define GENERIC_DEATH_EXPLOSION_RADIUS			136

#define DISGUISE_BREAKING_ACTION_STATE_DURATION		3.0f

#define EARTHQUAKE_PARTICLE_NAME		"doublejump_smoke"
#define EARTHQUAKE_PARTICLE_NAME_SNOW	"snow_steppuff01"

extern bool IsInCommentaryMode( void );

EHANDLE g_pLastSpawnPoints[TF_TEAM_COUNT];

ConVar tf_playerstatetransitions( "tf_playerstatetransitions", "-2", FCVAR_CHEAT, "tf_playerstatetransitions <ent index or -1 for all>. Show player state transitions." );
ConVar tf_playergib( "tf_playergib", "1", FCVAR_PROTECTED, "Allow player gibbing. 0: never, 1: normal, 2: always", true, 0, true, 2 );
ConVar tf_scout_air_dash_count( "tf_scout_air_dash_count", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_weapon_ragdoll_velocity_min( "tf_weapon_ragdoll_velocity_min", "100", FCVAR_CHEAT );
ConVar tf_weapon_ragdoll_velocity_max( "tf_weapon_ragdoll_velocity_max", "150", FCVAR_CHEAT );
ConVar tf_weapon_ragdoll_maxspeed( "tf_weapon_ragdoll_maxspeed", "300", FCVAR_CHEAT );

ConVar tf_damageforcescale_other( "tf_damageforcescale_other", "6.0", FCVAR_CHEAT );
ConVar tf_damageforcescale_self_soldier_rj( "tf_damageforcescale_self_soldier_rj", "10.0", FCVAR_CHEAT );
ConVar tf_damageforcescale_self_soldier_badrj( "tf_damageforcescale_self_soldier_badrj", "5.0", FCVAR_CHEAT );
ConVar tf_damagescale_self_soldier( "tf_damagescale_self_soldier", "0.60", FCVAR_CHEAT );

ConVar tf_damage_lineardist( "tf_damage_lineardist", "0", FCVAR_DEVELOPMENTONLY );
ConVar tf_damage_range( "tf_damage_range", "0.5", FCVAR_DEVELOPMENTONLY );

ConVar tf_max_voice_speak_delay( "tf_max_voice_speak_delay", "1.5", FCVAR_NOTIFY, "Max time after a voice command until player can do another one" );

ConVar tf_allow_player_use( "tf_allow_player_use", "0", FCVAR_NOTIFY, "Allow players to execute + use while playing." );

ConVar tf_allow_sliding_taunt( "tf_allow_sliding_taunt", "0", FCVAR_NONE, "Allow player to slide for a bit after taunting." );
ConVar tf_highfive_max_range( "tf_highfive_max_range", "150", FCVAR_CHEAT, "The farthest away a high five partner can be." );
ConVar tf_highfive_height_tolerance( "tf_highfive_height_tolerance", "12", FCVAR_CHEAT, "The maximum height difference allowed for two high-fivers." );

ConVar tf_nav_in_combat_range( "tf_nav_in_combat_range", "1000", FCVAR_CHEAT );

// Team Fortress 2 Classic commands
ConVar tf2c_force_stock_weapons( "tf2c_force_stock_weapons", "0", FCVAR_NOTIFY, "Forces players to use the stock loadout." );
ConVar tf2c_legacy_weapons( "tf2c_legacy_weapons", "0", FCVAR_DEVELOPMENTONLY | FCVAR_NOTIFY, "Disables all new weapons as well as Econ Item System." );
ConVar tf2c_pumpkin_loot_drop_rate( "tf2c_pumpkin_loot_drop_rate", "0.3", FCVAR_REPLICATED, "Sets drop percentage for pumpkin loot on Halloween maps." );
ConVar tf2c_vip_armor( "tf2c_vip_armor", "0.8", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiply damage taken under Civilian's morale boost by this much" );
ConVar tf2c_vip_eagle_armor( "tf2c_vip_eagle_armor", "0.75", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiply damage taken under the Eagle Eye's defense boost by this much" );
ConVar tf2c_vip_regen( "tf2c_vip_regen", "5.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Regenerate this much HP per second under Civilian's morale boost" );
ConVar tf2c_spawncamp_window("tf2c_spawncamp_window", "6.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Seconds after spawn during which killed players can respawn faster");
ConVar tf2c_earthquake_vertical_speed_mult("tf2c_earthquake_vertical_speed_mult", "3.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiply damage by this amount to get vertical velocity push on earthquake attack");
ConVar tf2c_earthquake_vertical_speed_const("tf2c_earthquake_vertical_speed_const", "200.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Const part of vertical velocity push on earthquake attack");

ConVar tf2c_stunned_taunting( "tf2c_stunned_taunting", "0", FCVAR_NONE, "Allows taunting while stunned" );
ConVar tf2c_disable_loser_taunting( "tf2c_disable_loser_taunting", "0", FCVAR_NONE, "Prevents losers from taunting" );

ConVar tf2c_debug_airblast_min_height_line("tf2c_debug_airblast_min_height_line", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Visualize airblast minimum height check trace.");
ConVar tf2c_debug_shadows( "tf2c_vampire", "0", FCVAR_HIDDEN, "Toggles shadow casting on the player." );

ConVar tf2c_arena_drop_healthkit_on_death("tf2c_arena_drop_healthkit_on_death", "1", FCVAR_REPLICATED, "If enabled, players will drop health packs that only heal their killer's team.");

extern ConVar tf2c_item_testing;

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_maxunlag;
extern ConVar tf_damage_disablespread;
extern ConVar tf_gravetalk;
extern ConVar tf_spectalk;
extern ConVar tf_stalematechangeclasstime;

extern ConVar tf2c_disablefreezecam;
extern ConVar mp_scrambleteams_mode;
extern ConVar tf2c_spy_cloak_ammo_refill;
extern ConVar tf2c_ctf_attacker_bonus;
extern ConVar tf2c_ctf_attacker_bonus_dist;
extern ConVar tf2c_ctf_carry_slow;
extern ConVar tf2c_ctf_carry_slow_mult;
extern ConVar tf2c_ctf_carry_slow_blastjumps;

extern ConVar tf2c_spywalk;
extern ConVar tf2c_building_gun_mettle;
extern ConVar tf2c_bullets_pass_teammates;
extern ConVar tf2c_building_sharing;
extern ConVar tf2c_uber_readiness_threshold;
extern ConVar tf2c_taunting_detonate_stickies;

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
		m_iPlayerIndex = 0;
	}

	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );

};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropInt( SENDINFO( m_iPlayerIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
    Vector vecEyePos = pPlayer->EyePosition();
	CPVSFilter filter( vecEyePos );
	filter.UsePredictionRules();

	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_iPlayerIndex = pPlayer->entindex();
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < ( 1 << ANIMATION_SEQUENCE_BITS ) );
	Assert( ( 1 << ANIMATION_SEQUENCE_BITS ) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//=================================================================================
//
// Ragdoll Entity
//
//=================================================================================

class CTFRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CTFRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	CTFRagdoll()
	{
		m_iRagdollFlags = 0;
		m_flInvisibilityLevel = 0.0f;
		m_iDamageCustom = TF_DMG_CUSTOM_NONE;
		m_bitsDamageType = DMG_GENERIC;
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
		UseClientSideAnimation();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar( int, m_iRagdollFlags );
	CNetworkVar( float, m_flInvisibilityLevel );
	CNetworkVar( ETFDmgCustom, m_iDamageCustom );
	CNetworkVar( int, m_bitsDamageType );
	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iClass );

};

LINK_ENTITY_TO_CLASS( tf_ragdoll, CTFRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTFRagdoll, DT_TFRagdoll )
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1, SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hOwnerEntity ) ),
	SendPropVector( SENDINFO( m_vecForce ), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f ),
	SendPropInt( SENDINFO( m_nForceBone ) ),
	SendPropInt( SENDINFO( m_iRagdollFlags ) ),
	SendPropFloat( SENDINFO( m_flInvisibilityLevel ), 8, 0, 0.0f, 1.0f ),
	SendPropInt( SENDINFO( m_iDamageCustom ) ),
	SendPropInt( SENDINFO( m_bitsDamageType ) ),
	SendPropInt( SENDINFO( m_iTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), 4, SPROP_UNSIGNED ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: Filters updates to a variable so that only non-local players see
// the changes.  This is so we can send a low-res origin to non-local players
// while sending a hi-res one to the local player.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendNonLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient( objectID - 1 );
	return (void *)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalDataTable );

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_PlayerObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer *)pStruct;

	// If this fails, then SendProxyArrayLength_PlayerObjects didn't work.
	Assert( iElement < pPlayer->GetObjectCount() );

	EHANDLE hObject = pPlayer->GetObject( iElement );
	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

int SendProxyArrayLength_PlayerObjects( const void *pStruct, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer *)pStruct;
	int iObjects = pPlayer->GetObjectCount();
	Assert( iObjects <= MAX_OBJECTS_PER_PLAYER );
	return iObjects;
}

BEGIN_DATADESC( CTFPlayer )
	DEFINE_INPUTFUNC( FIELD_STRING,	"SpeakResponseConcept",	InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"IgnitePlayer",	InputIgnitePlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomModel", InputSetCustomModel ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelOffset", InputSetCustomModelOffset ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelRotation", InputSetCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearCustomModelRotation", InputClearCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelRotates", InputSetCustomModelRotates ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelVisibleToSelf", InputSetCustomModelVisibleToSelf ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ExtinguishPlayer",	InputExtinguishPlayer ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"BleedPlayer", InputBleedPlayer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetForcedTauntCam", InputSetForcedTauntCam ),
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
END_DATADESC()

// Specific to the local player.
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFLocalPlayerExclusive )
	// Send a hi-res origin to the local player for use in prediction.
	SendPropVectorXY( SENDINFO( m_vecOrigin ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),
	SendPropArray2( 
		SendProxyArrayLength_PlayerObjects,
		SendPropInt( "player_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_PlayerObjectList ), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"
	),

	SendPropVector( SENDINFO( m_angEyeAngles ), 32, SPROP_CHANGES_OFTEN ),

	SendPropEHandle( SENDINFO( m_hOffHandWeapon ) ),
	SendPropInt( SENDINFO( m_nForceTauntCam ), 2, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bArenaSpectator ) ),
#ifdef ITEM_TAUNTING
	SendPropBool( SENDINFO( m_bAllowMoveDuringTaunt ) ),
	SendPropBool( SENDINFO( m_bTauntForceForward ) ),
	SendPropFloat( SENDINFO( m_flTauntSpeed ) ),
	SendPropFloat( SENDINFO( m_flTauntTurnSpeed ) ),
#endif

	SendPropFloat( SENDINFO( m_flRespawnTimeOverride ) ),
END_SEND_TABLE()

// All players except the local player.
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFNonLocalPlayerExclusive )
	// Send a lo-res origin to other players.
	SendPropVectorXY( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),

	// https://youtu.be/mlhw6RqvOgs
	// send higher res eye angs to clients so sniper dots actually match where the client is looking
	// thanks shounic!
	// -sappho
	SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 16, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 1 ), 16, SPROP_CHANGES_OFTEN ),

	SendPropInt( SENDINFO( m_nActiveWpnClip ), 9, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nActiveWpnAmmo ), 11, SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bTyping ) ),
END_SEND_TABLE()

//============

LINK_ENTITY_TO_CLASS( player, CTFPlayer );
PRECACHE_REGISTER( player );

IMPLEMENT_SERVERCLASS_ST( CTFPlayer, DT_TFPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_nModelIndex" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// cs_playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropBool( SENDINFO( m_bSaveMeParity ) ),
	SendPropBool( SENDINFO( m_bIsABot ) ),

	// This will create a race condition will the local player, but the data will be the same so.....
	SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	SendPropEHandle( SENDINFO( m_hItem ) ),

	// Ragdoll.
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropDataTable( SENDINFO_DT( m_PlayerClass ), &REFERENCE_SEND_TABLE( DT_TFPlayerClassShared ) ),
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_TFPlayerShared ) ),
	SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE( DT_AttributeContainerPlayer ) ),

	// Data that only gets sent to the local player
	SendPropDataTable( "tflocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFLocalPlayerExclusive ), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "tfnonlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFNonLocalPlayerExclusive ), SendProxy_SendNonLocalDataTable ),

	SendPropInt( SENDINFO( m_iSpawnCounter ) ),
	SendPropTime( SENDINFO( m_flLastDamageTime ) ),
	SendPropBool( SENDINFO( m_bAutoReload ) ),
	SendPropBool( SENDINFO( m_bFlipViewModel ) ),
	SendPropFloat( SENDINFO( m_flViewModelFOV ) ),
	SendPropVector( SENDINFO( m_vecViewModelOffset ) ),
	SendPropBool( SENDINFO( m_bMinimizedViewModels ) ),
	SendPropBool( SENDINFO( m_bInvisibleArms ) ),
	SendPropBool( SENDINFO( m_bFixedSpreadPreference ) ),
	SendPropBool( SENDINFO(m_bSpywalkInverted) ),
	SendPropBool( SENDINFO( m_bCenterFirePreference ) ),
END_SEND_TABLE()


BEGIN_ENT_SCRIPTDESC( CTFPlayer, CBaseMultiplayerPlayer, "Team Fortress player class" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetActiveWeapon, "GetActiveWeapon", "Get the player's current weapon" )

	DEFINE_SCRIPTFUNC( ForceRespawn, "Force respawns the player" )
	DEFINE_SCRIPTFUNC( ForceRegenerateAndRespawn, "Force regenerates and respawns the player" )
	DEFINE_SCRIPTFUNC( Regenerate, "Resupplies a player. If regen health/ammo is set, clears negative conds, gives back player health/ammo" )

	DEFINE_SCRIPTFUNC( HasItem, "Currently holding an item? Eg. capture flag" )

	DEFINE_SCRIPTFUNC( GetNextRegenTime, "Get next health regen time." )
	DEFINE_SCRIPTFUNC( SetNextRegenTime, "Set next health regen time." )

	DEFINE_SCRIPTFUNC( GetNextChangeClassTime, "Get next change class time." )
	DEFINE_SCRIPTFUNC( SetNextChangeClassTime, "Set next change class time." )

	DEFINE_SCRIPTFUNC( GetNextChangeTeamTime, "Get next change team time." )
	DEFINE_SCRIPTFUNC( SetNextChangeTeamTime, "Set next change team time." )

	DEFINE_SCRIPTFUNC( DropFlag, "Force player to drop the flag." )
	DEFINE_SCRIPTFUNC( DropRune, "Force player to drop their rune." )

	DEFINE_SCRIPTFUNC( ForceChangeTeam, "Force player to change their team." )

	DEFINE_SCRIPTFUNC( IsMiniBoss, "Is this player an MvM mini-boss?" )
	DEFINE_SCRIPTFUNC( SetIsMiniBoss, "Make this player an MvM mini-boss." )

	DEFINE_SCRIPTFUNC( CanJump, "Can the player jump?" )
	DEFINE_SCRIPTFUNC( CanDuck, "Can the player duck?" )
	DEFINE_SCRIPTFUNC( CanPlayerMove, "Can the player move?" )

	DEFINE_SCRIPTFUNC( RemoveAllObjects, "Remove all player objects. Eg. dispensers/sentries." )
	DEFINE_SCRIPTFUNC( IsPlacingSapper, "Returns true if we placed a sapper in the last few moments" )
	DEFINE_SCRIPTFUNC( IsSapping, "Returns true if we are currently sapping" )
	DEFINE_SCRIPTFUNC( RemoveInvisibility, "Un-invisible a spy." )
	DEFINE_SCRIPTFUNC( RemoveDisguise, "Undisguise a spy." )
	DEFINE_SCRIPTFUNC( TryToPickupBuilding, "Make the player attempt to pick up a building in front of them" )

	DEFINE_SCRIPTFUNC( IsCallingForMedic, "Is this player calling for medic?" )
	DEFINE_SCRIPTFUNC( GetTimeSinceCalledForMedic, "When did the player call medic" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealTarget, "GetHealTarget", "Who is the medic healing?" )

	DEFINE_SCRIPTFUNC( GetClassEyeHeight, "Gets the eye height of the player" )
	DEFINE_SCRIPTFUNC( FiringTalk, "Makes eg. a heavy go AAAAAAAAAAaAaa like they are firing their minigun." )
	DEFINE_SCRIPTFUNC( CanAirDash, "" )
	DEFINE_SCRIPTFUNC( CanBreatheUnderwater, "" )
	DEFINE_SCRIPTFUNC( CanGetWet, "" )
	DEFINE_SCRIPTFUNC( InAirDueToExplosion, "" )
	DEFINE_SCRIPTFUNC( InAirDueToKnockback, "" )
	DEFINE_SCRIPTFUNC( ApplyAbsVelocityImpulse, "" )
	DEFINE_SCRIPTFUNC( ApplyPunchImpulseX, "" )
	DEFINE_SCRIPTFUNC( SetUseBossHealthBar, "" )
	DEFINE_SCRIPTFUNC( IsFireproof, "" )
	DEFINE_SCRIPTFUNC( IsAllowedToTaunt, "" )
	DEFINE_SCRIPTFUNC( IsViewingCYOAPDA, "" )
	DEFINE_SCRIPTFUNC( IsRegenerating, "" )

	DEFINE_SCRIPTFUNC( GetCurrentTauntMoveSpeed, "" )
	DEFINE_SCRIPTFUNC( SetCurrentTauntMoveSpeed, "" )

	DEFINE_SCRIPTFUNC( IsUsingActionSlot, "" )
	DEFINE_SCRIPTFUNC( IsInspecting, "" )

	DEFINE_SCRIPTFUNC( GetGrapplingHookTarget, "What entity is the player grappling?" )
	DEFINE_SCRIPTFUNC( SetGrapplingHookTarget, "Set the player's target grapple entity" )

	DEFINE_SCRIPTFUNC( AddCustomAttribute, "Add a custom attribute to the player" )
	DEFINE_SCRIPTFUNC( RemoveCustomAttribute, "Remove a custom attribute from the player" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCond, "AddCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCondEx, "AddCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCond, "RemoveCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCondEx, "RemoveCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptInCond, "InCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWasInCond, "WasInCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllCond, "RemoveAllCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCondDuration, "GetCondDuration", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetCondDuration, "SetCondDuration", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTarget, "GetDisguiseTarget", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseAmmoCount, "GetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetDisguiseAmmoCount, "SetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTeam, "GetDisguiseTeam", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsCarryingRune, "IsCarryingRune", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsCritBoosted, "IsCritBoosted", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsInvulnerable, "IsInvulnerable", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsStealthed, "IsStealthed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptCanBeDebuffed, "CanBeDebuffed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsImmuneToPushback, "IsImmuneToPushback", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFullyInvisible, "IsFullyInvisible", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSpyCloakMeter, "GetSpyCloakMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetSpyCloakMeter, "SetSpyCloakMeter", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsRageDraining, "IsRageDraining", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetRageMeter, "GetRageMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetRageMeter, "SetRageMeter", "" )
	
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsHypeBuffed, "IsHypeBuffed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetScoutHypeMeter, "GetScoutHypeMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetScoutHypeMeter, "SetScoutHypeMeter", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsJumping, "IsJumping", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsAirDashing, "IsAirDashing", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsControlStunned, "IsControlStunned", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsSnared, "IsSnared", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCaptures, "GetCaptures", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDefenses, "GetDefenses", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDominations, "GetDominations", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetRevenge, "GetRevenge", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBuildingsDestroyed, "GetBuildingsDestroyed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHeadshots, "GetHeadshots", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBackstabs, "GetBackstabs", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealPoints, "GetHealPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetInvulns, "GetInvulns", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetTeleports, "GetTeleports", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetResupplyPoints, "GetResupplyPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKillAssists, "GetKillAssists", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBonusPoints, "GetBonusPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptResetScores, "ResetScores", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsParachuteEquipped, "IsParachuteEquipped", "" )

	DEFINE_SCRIPTFUNC( GetCurrency, "Get player's cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( SetCurrency, "Set player's cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( AddCurrency, "Kaching! Give the player some cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( RemoveCurrency, "Take away money from a player for reasons such as ie. spending." )

	DEFINE_SCRIPTFUNC( IgnitePlayer, "" )
	DEFINE_SCRIPTFUNC( ExtinguishPlayerBurning, "" )

	DEFINE_SCRIPTFUNC( SetCustomModel, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelWithClassAnimations, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelOffset, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( ClearCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotates, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelVisibleToSelf, "" )

	DEFINE_SCRIPTFUNC( SetForcedTauntCam, "" )
	DEFINE_SCRIPTFUNC( BleedPlayer, "" )
	DEFINE_SCRIPTFUNC( BleedPlayerEx, "" )
	DEFINE_SCRIPTFUNC( RollRareSpell, "" )
	DEFINE_SCRIPTFUNC( ClearSpells, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetPlayerClass, "GetPlayerClass", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetPlayerClass, "SetPlayerClass", "" )
	DEFINE_SCRIPTFUNC( RemoveTeleportEffect, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllItems, "RemoveAllItems", "" )
	DEFINE_SCRIPTFUNC( UpdateSkin, "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_ShootPosition, "Weapon_ShootPosition", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_CanUse, "Weapon_CanUse", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_Equip, "Weapon_Equip", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_Drop, "Weapon_Drop", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_DropEx, "Weapon_DropEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_Switch, "Weapon_Switch", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWeapon_SetLast, "Weapon_SetLast", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetLastWeapon, "GetLastWeapon", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptEquipWearableViewModel, "EquipWearableViewModel", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFakeClient, "IsFakeClient", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBotType, "GetBotType", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsBotOfType, "IsBotOfType", "" )
	DEFINE_SCRIPTFUNC( GrantOrRemoveAllUpgrades, "Grants or removes all upgrades the player has purchased." )
END_SCRIPTDESC()



// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );



CTFPlayer::CTFPlayer()
{
	m_pAttributes = this;

	m_PlayerAnimState = CreateTFPlayerAnimState( this );

	SetArmorValue( 10 );

	m_hItem = NULL;
	m_hTauntScene = NULL;

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_pStateInfo = NULL;
	m_lifeState = LIFE_DEAD; // Start "dead".
	m_iMaxSentryKills = 0;
	m_flNextNameChangeTime = 0.0f;

	m_flNextTimeCheck = gpGlobals->curtime;
	m_flSpawnTime = 0.0f;

	m_flNextCarryTalkTime = 0.0f;

	SetViewOffset( Vector( 0, 0, 64 ) ); // To be overriden.

	m_Shared.Init( this );

	m_bHudClassAutoKill = false;
	m_bMedigunAutoHeal = false;
	m_bAutoRezoom = false;
	m_bRememberActiveWeapon = false;
	m_bRememberLastWeapon = false;
	m_bProximityVoice = false;

	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );

	ResetScores();

	m_flLastAction = gpGlobals->curtime;

	m_bInitTaunt = false;
#ifdef ITEM_TAUNTING
	m_bHoldingTaunt = false;
	m_bIsReadyToHighFive = false;
	m_pTauntItem = NULL;
	m_bPlayingTauntSound = false;
#endif

	m_bSpeakingConceptAsDisguisedSpy = false;

	m_flTauntAttackTime = 0.0f;
	m_iTauntAttack = TAUNTATK_NONE;

	m_flLastThinkTime = -1.0f;

	m_iBlastJumpState = 0;
	m_bBlastLaunched = false;
	m_bSelfKnockback = false;
	m_bCreatedRocketJumpParticles = false;

	m_iDominations = 0;

	memset( m_iItemPresets, 0, static_cast<int>(TF_CLASS_COUNT_ALL) * static_cast<int>(TF_LOADOUT_SLOT_COUNT) * sizeof( int ) );
	m_nActiveWpnClip = -1;
	m_nActiveWpnAmmo = -1;

	m_iLastTeamNum = TEAM_UNASSIGNED;

	m_flWaterEntryTime = 0.0f;
	m_flWaterExitTime = 0.0f;

	m_bIsBasicBot = false;

	m_bPreserveRandomLoadout = false;
	m_flTeamScrambleScore = 0.0f;
	
	m_iPreviousClassIndex = TF_CLASS_UNDEFINED;
	m_bWasVIP = false;

	m_bArenaSpectator = false;
	m_flSpectatorTime = 0.0f;

	m_flViewModelFOV = 54.0f;
	m_vecViewModelOffset.Init();

	// New stuff.
	m_flLastDamageResistSoundTime = -1.0f;

	m_iOldStunFlags = 0;

	m_pJumpPadSoundLoop = nullptr;
	bTickTockOrder = false;

	m_pVIPInstantlyRespawned = false;
}



void CTFPlayer::TFPlayerThink()
{
	if ( m_pStateInfo && m_pStateInfo->pfnThink )
	{
		( this->*m_pStateInfo->pfnThink )();
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if ( IsAlive() && m_flNextRandomExpressionTime >= 0 && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();
		m_iszExpressionScene = NULL_STRING;
		UpdateExpression();
	}

	// Check to see if we are in the air and taunting, stop if so.
	if ( !GetGroundEntity() && GetWaterLevel() == WL_NotInWater )
	{
		if ( m_Shared.InCond( TF_COND_TAUNTING ) && m_hTauntScene.Get()
#ifdef ITEM_TAUNTING
			&& !m_bAllowMoveDuringTaunt
#endif
			)
		{
			StopScriptedScene( this, m_hTauntScene );
			m_Shared.m_flTauntRemoveTime = 0.0f;
			m_hTauntScene = NULL;
		}

		// Add rocket trail if we haven't already.
		if ( !m_bCreatedRocketJumpParticles && ( ( m_iBlastJumpState & TF_PLAYER_ROCKET_JUMPED ) || ( m_iBlastJumpState & TF_PLAYER_STICKY_JUMPED ) ) && IsAlive() )
		{
			DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, this, "foot_L" );
			DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, this, "foot_R" );
			m_bCreatedRocketJumpParticles = true;
		}
	}
	else
	{
		// Clear blast jumping state if we landed on the ground or in the water.
		if ( m_iBlastJumpState )
		{
			bool bHadAnyBlastJump = m_iBlastJumpState;
			const char *pszEvent;
			if ( m_iBlastJumpState & TF_PLAYER_STICKY_JUMPED )
			{
				pszEvent = "sticky_jump_landed";
			}
			else if ( m_iBlastJumpState & TF_PLAYER_ROCKET_JUMPED )
			{
				pszEvent = "rocket_jump_landed";
			}
			else
			{
				pszEvent = NULL;
			}

			ClearBlastJumpState();

			if ( pszEvent )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( pszEvent );
				if ( event )
				{
					event->SetInt( "userid", GetUserID() );
					gameeventmanager->FireEvent( event );
				}
			}
			if (bHadAnyBlastJump)
			{
				IGameEvent* event = gameeventmanager->CreateEvent("blast_jump_landed");
				if (event)
				{
					event->SetInt("userid", GetUserID());
					gameeventmanager->FireEvent(event);
				}
			}
		}
	}

	// If player is hauling a building have him talk about it from time to time.
	if ( m_flNextCarryTalkTime != 0.0f && m_flNextCarryTalkTime < gpGlobals->curtime )
	{
		CBaseObject *pObject = m_Shared.GetCarriedObject();
		if ( pObject )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_CARRYING_BUILDING, pObject->GetResponseRulesModifier() );
			m_flNextCarryTalkTime = gpGlobals->curtime + RandomFloat( 6.0f, 12.0f );
		}
		else
		{
			// No longer hauling, shut up.
			m_flNextCarryTalkTime = 0.0f;
		}
	}

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );
	m_flLastThinkTime = gpGlobals->curtime;
}


void CTFPlayer::RegenThink( void )
{
	if ( !IsAlive() )
		return;

	// Queue the next think.
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );

	// Bail out just incase we manage to regen too fast.
	if ( ( m_flLastRegenTime + TF_REGEN_TIME ) > gpGlobals->curtime )
		return;


	// Attribute can delay regen when recently damaged.
	bool bBoolNoSelfRegen = false;
	float flItemHealthRegenHurtDelay = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT(flItemHealthRegenHurtDelay, mod_health_regen_hurt_delay);
	if (flItemHealthRegenHurtDelay != 0.0f)
	{
		float flTimeSinceHurt = gpGlobals->curtime - GetLastDamageReceivedTime();
		//DevMsg( "delay regen: %2.2f, %2.2f\n", flTimeSinceHurt, flItemHealthRegenHurtDelay );
		if (flTimeSinceHurt <= flItemHealthRegenHurtDelay)
		{
			bBoolNoSelfRegen = true;
		}
	}

	// There is a bit change to queue order
	// Each separate regen cause now handles it's own healing so we can send different events for each cause

	// But we do sum up healing that has to be notified with event so the number is combined together
	int iHealingDoneToShow = 0;

	// And since item-based regen is sometimes used to just lower natural regen, we calculate it first
	float flRegenDamage = 0.0f;

	// Item-based Regen:
	float flItemRegenAmount = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT(flItemRegenAmount, add_health_regen);
	if (!bBoolNoSelfRegen && flItemRegenAmount != 0.0f)
	{
		float flScale = 1.0f;

		// Uncomment this (or add another attribute) to disable scaling on health drains.
		//if ( flItemRegenAmount > 0 )
		{
			float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageReceivedTime();
			if (flTimeSinceDamage < 5.0f)
			{
				flScale = 0.25f; // ok why does it go 0.25 only to suddenly jump to 0.5 at 5 seconds?
			}
			else
			{
				flScale = RemapValClamped(flTimeSinceDamage, 5.0f, 10.0f, 0.5f, 1.0f);
			}
		}
		flItemRegenAmount *= flScale;

		int iHealAmount = 0;
		if (flItemRegenAmount >= 1.0f)
		{
			iHealAmount = floor(flItemRegenAmount);
			if (GetHealth() < GetMaxHealth())
			{
				// A slightly different way of showing health gains (or drains) is done here.
				int iHealedAmount = TakeHealth(iHealAmount, DMG_GENERIC);
				if (iHealedAmount > 0)
				{
					IGameEvent* event = gameeventmanager->CreateEvent("player_healed");
					if (event)
					{
						event->SetInt("priority", 1);	// HLTV event priority.
						event->SetInt("patient", GetUserID());
						event->SetInt("healer", GetUserID());
						event->SetInt("amount", iHealedAmount);

						gameeventmanager->FireEvent(event);
					}

					CTF_GameStats.Event_PlayerHealedOther(this, (float)iHealedAmount);

					iHealingDoneToShow += iHealedAmount;
				}
			}
		}
		else if (flItemRegenAmount <= -1.0f)
		{
			flRegenDamage = -flItemRegenAmount;
		}
	}

	// Medic Regen
	if ( !bBoolNoSelfRegen && IsPlayerClass( TF_CLASS_MEDIC, true ) )
	{
		float flScale, flRegenAmount = TF_REGEN_AMOUNT;
		// Heal faster if we haven't been in combat for a while.
		flScale = RemapValClamped( gpGlobals->curtime - GetLastDamageReceivedTime(), 5.0f, 10.0f, 1.0f, 2.0f );

		// If you are healing a hurt patient, increase your base regen.
		CTFPlayer *pPatient = ToTFPlayer( MedicGetHealTarget() );
		if ( pPatient && pPatient->GetHealth() < pPatient->GetMaxHealth() )
		{
			// Double regen amount.
			flRegenAmount *= 2;
		}

		flRegenAmount *= flScale;

		float flOldRegenAmount = flRegenAmount;
		flRegenAmount -= flRegenDamage;
		flRegenDamage = Max(flRegenDamage - flOldRegenAmount, 0.0f);

		if (flRegenAmount >= 1.0f)
		{
			int iHealAmount = floor(flRegenAmount);

			if (GetHealth() < GetMaxHealth())
			{
				// A slightly different way of showing health gains (or drains) is done here.
				int iHealedAmount = TakeHealth(iHealAmount, DMG_GENERIC);
				if (iHealedAmount > 0)
				{
					IGameEvent* event = gameeventmanager->CreateEvent("player_healed");
					if (event)
					{
						event->SetInt("priority", 1);	// HLTV event priority.
						event->SetInt("patient", GetUserID());
						event->SetInt("healer", GetUserID());
						event->SetInt("amount", iHealedAmount);

						gameeventmanager->FireEvent(event);
					}

					CTF_GameStats.Event_PlayerHealedOther(this, (float)iHealedAmount);
				}
			}
		}
	}

	// Civilian Aura Regen:
	if (m_Shared.InCond(TF_COND_RESISTANCE_BUFF) || 
		(!bBoolNoSelfRegen && IsPlayerClass(TF_CLASS_CIVILIAN, true)))
	{
		float flCivRegenScale = 1.0f;
		float flCivRegenAmount = tf2c_vip_regen.GetFloat();
		float flCivRegenTotal = 0.0f;

		// Heal faster if we haven't been in combat for a while.
		flCivRegenScale = RemapValClamped(gpGlobals->curtime - GetLastDamageReceivedTime(), 10.0f, 15.0f, 1.0f, 3.0f);
		flCivRegenTotal += flCivRegenAmount * flCivRegenScale;

		float flOldRegenAmount = flCivRegenTotal;
		flCivRegenTotal -= flRegenDamage;
		flRegenDamage = Max(flRegenDamage - flOldRegenAmount, 0.0f);

		int iCivHealAmount = Floor2Int(flCivRegenTotal);
		if (iCivHealAmount > 0)
		{
			if (m_Shared.InCond(TF_COND_RESISTANCE_BUFF) && iCivHealAmount >= 1)
			{
				if (m_Shared.GetDisguiseHealth() < m_Shared.GetDisguiseMaxHealth())
				{
					// A slightly different way of showing health gains (or drains) is done here.
					/*int iHealedAmount =*/ TakeDisguiseHealth(iCivHealAmount, HEAL_NOTIFY);
					/*if (iHealedAmount > 0)
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
						if ( event )
						{
							event->SetInt( "priority", 1 );	// HLTV event priority.
							event->SetInt( "patient", GetUserID() );
							event->SetInt( "healer", GetUserID() );
							event->SetInt( "amount", iHealedAmount );

							gameeventmanager->FireEvent( event );
						}
					}*/
					// why?
				}
			}

			if (GetHealth() < GetMaxHealth())
			{
				CTFPlayer* pProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_RESISTANCE_BUFF));
				if (!pProvider && IsPlayerClass(TF_CLASS_CIVILIAN, true))
					pProvider = this;
				/*if (pProvider)
				{
					m_Shared.HealTimed(pProvider, flCivRegenTotal, 1.0, true);
				}
				else if (IsPlayerClass(TF_CLASS_CIVILIAN, true))
				{
					m_Shared.HealTimed(this, flCivRegenTotal, 1.0, true);
				}*/
				

				// A slightly different way of showing health gains (or drains) is done here.
				int iHealedAmount = TakeHealth( iCivHealAmount, DMG_GENERIC );
				if ( iHealedAmount > 0 )
				{
					IGameEvent* event = gameeventmanager->CreateEvent( "player_healed" );
					if ( event )
					{
						event->SetInt( "priority", 1 );	// HLTV event priority.
						event->SetInt( "patient", GetUserID() );
						event->SetInt( "healer", pProvider ? pProvider->GetUserID() : GetUserID() );
						event->SetInt( "amount", iHealedAmount );

						gameeventmanager->FireEvent( event );
					}

					if (pProvider && IsEnemy(pProvider))
					{
						CTF_GameStats.Event_PlayerLeachedHealth(this, false, iHealedAmount);
					}
					else
					{
						CTF_GameStats.Event_PlayerHealedOther(pProvider ? pProvider : this, iHealedAmount);
					}

					if (pProvider && pProvider != this)
						iHealingDoneToShow += iHealedAmount;
				}
				
			}
		}
	}

	else if (flRegenDamage >= 1.0f )
	{
		int iRegenDamage = Ceil2Int(flRegenDamage);
		TakeDamage( CTakeDamageInfo( this, this, NULL, vec3_origin, WorldSpaceCenter(), iRegenDamage, DMG_GENERIC ) );
	}

	// Show the player their gainage/drainage.
	if (iHealingDoneToShow >= 1)
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iHealingDoneToShow);
			event->SetInt( "entindex", entindex() );

			gameeventmanager->FireEvent( event ); 
		}
	}


	// And now, disguise, separately, for self-regen types
	if ( m_Shared.IsDisguised() )
	{
		// Disguise Medic Regen:
		if ( m_Shared.GetDisguiseClass() == TF_CLASS_MEDIC )
		{
			// Heal faster if we haven't been in combat for a while.
			float flScale = RemapValClamped( gpGlobals->curtime - GetLastDamageReceivedTime(), 5.0f, 10.0f, 1.0f, 2.0f );

			float flDisguiseHealthRegen = ( TF_REGEN_AMOUNT * flScale );

			if (flDisguiseHealthRegen >= 1.0f )
			{
				int iHealAmount = floor(flDisguiseHealthRegen);
				if ( m_Shared.GetDisguiseHealth() < m_Shared.GetDisguiseMaxHealth() )
				{
					// A slightly different way of showing health gains (or drains) is done here.
					/*int iHealedAmount =*/ TakeDisguiseHealth(iHealAmount, DMG_GENERIC);
					/*if (iHealedAmount > 0)
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
						if ( event )
						{
							event->SetInt( "priority", 1 );	// HLTV event priority.
							event->SetInt( "patient", GetUserID() );
							event->SetInt( "healer", GetUserID() );
							event->SetInt( "amount", iHealedAmount );

							gameeventmanager->FireEvent( event );
						}
					}*/
					// why?
				}
			}
		}
		// Disguise Civ Regen:
		if (m_Shared.GetDisguiseClass() == TF_CLASS_CIVILIAN && !m_Shared.InCond(TF_COND_RESISTANCE_BUFF))
		{
			float flCivRegenScale = 1.0f;
			float flCivRegenAmount = tf2c_vip_regen.GetFloat();
			float flCivRegenTotal = 0.0f;

			// Heal faster if we haven't been in combat for a while.
			flCivRegenScale = RemapValClamped(gpGlobals->curtime - GetLastDamageReceivedTime(), 10.0f, 15.0f, 1.0f, 3.0f);
			flCivRegenTotal += flCivRegenAmount * flCivRegenScale;
			// Heal faster if we haven't been in combat for a while.

			float flDisguiseHealthRegen = flCivRegenTotal;

			if (flDisguiseHealthRegen >= 1.0f)
			{
				int iHealAmount = floor(flDisguiseHealthRegen);
				if (m_Shared.GetDisguiseHealth() < m_Shared.GetDisguiseMaxHealth())
				{
					// A slightly different way of showing health gains (or drains) is done here.
					/*int iHealedAmount = */ TakeDisguiseHealth(iHealAmount, DMG_GENERIC);
					/*if (iHealedAmount > 0)
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
						if ( event )
						{
							event->SetInt( "priority", 1 );	// HLTV event priority.
							event->SetInt( "patient", GetUserID() );
							event->SetInt( "healer", GetUserID() );
							event->SetInt( "amount", iHealedAmount );

							gameeventmanager->FireEvent( event );
						}
					}*/
					// why?
				}
			}
		}
	}

	m_flLastRegenTime = gpGlobals->curtime;
}


CTFPlayer::~CTFPlayer()
{
	DestroyRagdoll();
	m_PlayerAnimState->Release();
    m_pAttributes = nullptr;
}


CTFPlayer *CTFPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CTFPlayer::s_PlayerEdict = ed;
	return (CTFPlayer *)CreateEntityByName( className );
}


void CTFPlayer::UpdateTimers( void )
{
	m_Shared.ConditionThink();
	m_Shared.InvisibilityThink();
}


void CTFPlayer::PreThink()
{
	// Riding a vehicle?
	if ( IsInAVehicle() )
	{
		// Update timers.
		UpdateTimers();

		// Make sure we update the client, check for timed damage and update suit even if we are in a vehicle.
		UpdateClientData();
		CheckTimeBasedDamage();

		WaterMove();

		m_vecTotalBulletForce = vec3_origin;

		CheckForIdle();
		return;
	}

	// Pass through to the base class think.
	BaseClass::PreThink();

	UpdateTimers();

#if 0
	if ( m_nButtons & IN_GRENADE1 )
	{
		TFPlayerClassData_t *pData = m_PlayerClass.GetData();
		CTFWeaponBase *pGrenade = Weapon_OwnsThisID( pData->m_aGrenades[0] );
		if ( pGrenade )
		{
			pGrenade->Deploy();
		}
	}
#endif

	// Reset bullet force accumulator, only lasts one frame, for ragdoll forces from multiple shots.
	m_vecTotalBulletForce = vec3_origin;

	CheckForIdle();
}

ConVar mp_idledealmethod( "mp_idledealmethod", "1", 0, "Deals with Idle Players. 1 = Sends them into Spectator mode then kicks them if they're still idle, 2 = Kicks them out of the game.", true, 0, true, 2 );
ConVar mp_idlemaxtime( "mp_idlemaxtime", "3", 0, "Maximum time a player is allowed to be idle (in minutes)." );
ConVar tf2c_vip_idlemaxtime( "tf2c_vip_idlemaxtime", "20", 0, "Maximum time a VIP is allowed to be idle (in seconds)." );


void CTFPlayer::CheckForIdle( void )
{
	if ( m_afButtonLast != m_nButtons )
	{
		m_flLastAction = gpGlobals->curtime;
	}

	if ( mp_idledealmethod.GetInt() )
	{
		if ( IsHLTV() || IsReplay() )
			return;

		if ( IsFakeClient() )
			return;

		if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
			return;

		// Don't mess with the host on a listen server (probably one of us debugging something).
		if ( !engine->IsDedicatedServer() && entindex() == 1 )
			return;

		if ( IsAutoKickDisabled() )
			return;

		if ( !m_bIsIdle )
		{
			if ( StateGet() != TF_STATE_ACTIVE )
				return;
		}

		float flIdleTime;
		if ( TFGameRules()->InStalemate() )
		{
			flIdleTime = mp_stalemate_timelimit.GetInt() * 0.5f;
		}
		else
		{
			flIdleTime = mp_idlemaxtime.GetFloat() * 60;
		}

		// TF2C Idle VIP needs to be dealt with quicker than normal players.
		// We can be more generous outside of active rounds.
		if ( IsVIP() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING && !TFGameRules()->InSetup() && !TFGameRules()->IsInWaitingForPlayers() )
		{
			flIdleTime = tf2c_vip_idlemaxtime.GetFloat();
		}

		if ( ( gpGlobals->curtime - m_flLastAction ) > flIdleTime )
		{
			bool bKickPlayer = false;

			ConVarRef mp_allowspectators( "mp_allowspectators" );
			if ( mp_allowspectators.IsValid() && !mp_allowspectators.GetBool() )
			{
				// Just kick the player if this server doesn't allow spectators.
				bKickPlayer = true;
			}
			else if ( mp_idledealmethod.GetInt() == 1 )
			{
				// First send them into spectator mode then kick him.
				if ( !m_bIsIdle )
				{
					ChangeTeam( TEAM_SPECTATOR, false, true );
					m_flLastAction = gpGlobals->curtime;
					m_bIsIdle = true;
					return;
				}
				else
				{
					bKickPlayer = true;
				}
			}
			else if ( mp_idledealmethod.GetInt() == 2 )
			{
				bKickPlayer = true;
			}

			if ( bKickPlayer )
			{
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_idle_kick", GetPlayerName() );
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", GetUserID(), "#TF_Idle_kicked" ) );
				m_flLastAction = gpGlobals->curtime;
			}
		}
	}
}

extern ConVar flashlight;


int CTFPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}


void CTFPlayer::FlashlightTurnOn( void )
{
	if ( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
	}
}


void CTFPlayer::FlashlightTurnOff( void )
{
	if ( IsEffectActive( EF_DIMLIGHT ) )
	{
		RemoveEffects( EF_DIMLIGHT );
	}
}


void CTFPlayer::PostThink()
{
	BaseClass::PostThink();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	if ( m_flTauntAttackTime > 0.0f && m_flTauntAttackTime < gpGlobals->curtime )
	{
		m_flTauntAttackTime = 0.0f;
		DoTauntAttack();
	}

	ProcessSceneEvents();

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon && pActiveWeapon->UsesPrimaryAmmo() )
	{
		m_nActiveWpnClip = pActiveWeapon->Clip1();
		m_nActiveWpnAmmo = GetAmmoCount( pActiveWeapon->GetPrimaryAmmoType() );
	}
	else
	{
		m_nActiveWpnClip = -1;
		m_nActiveWpnAmmo = -1;
	}
	
	// The "Spy-Walk"
	if ( tf2c_spywalk.GetBool() )
	{
		if ( !IsSpywalkInverted() )
		{
			if ( m_afButtonPressed & IN_SPEED || m_afButtonReleased & IN_SPEED )
			{
				TeamFortress_SetSpeed();
			}
		}
		else
		{
			if ( !(m_afButtonPressed & IN_SPEED) || !(m_afButtonReleased & IN_SPEED) )
			{
				TeamFortress_SetSpeed();
			}
		}
	}

	// Check if player is typing.
	m_bTyping = ( m_nButtons & IN_TYPING ) != 0;
}


void CTFPlayer::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	m_touchedPhysObject = false;

	// Zero out roll on view angles, it should always be zero under normal conditions and hacking it messes up movement (speedhacks).
	ucmd->viewangles[ROLL] = 0.0f;

	// Handle FL_FROZEN.
	if ( GetFlags() & FL_FROZEN )
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
		ucmd->buttons = 0;
		ucmd->impulse = 0;
		VectorCopy( pl.v_angle, ucmd->viewangles );
	}
	else if ( m_Shared.IsMovementLocked() )
	{
		// Don't allow player to perform any actions while taunting or stunned.
		// Not preventing movement since some taunts have special movement which needs to be handled in CTFGameMovement.
		// This is duplicated on client side in C_TFPlayer::PhysicsSimulate.
		int nRemoveButtons = ucmd->buttons;

		// Allow IN_ATTACK2 if player owns sticky launcher. This is done so they can detonate stickies while taunting.
		if ( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) && tf2c_taunting_detonate_stickies.GetBool() )
		{
			nRemoveButtons &= ~IN_ATTACK2;
		}

		ucmd->buttons &= ~nRemoveButtons;
		ucmd->weaponselect = 0;
		ucmd->weaponsubtype = 0;

		// Don't allow the player to turn around.
		VectorCopy( pl.v_angle, ucmd->viewangles );
	}

	PlayerMove()->RunCommand( this, ucmd, moveHelper );
}


void CTFPlayer::Precache()
{
	// Precache the player models and gibs.
	PrecachePlayerModels();

	// Precache the player sounds.
	PrecacheScriptSound( "Player.Spawn" );
	PrecacheScriptSound( "TFPlayer.Pain" );
	PrecacheScriptSound( "TFPlayer.CritHit" );
	PrecacheScriptSound( "TFPlayer.CritHitMini" );
	PrecacheScriptSound( "TFPlayer.CritHitBlocked" );
	PrecacheScriptSound( "TFPlayer.CritHitMiniBlocked" );
	PrecacheScriptSound( "TFPlayer.BlockedDamage" );
	PrecacheScriptSound( "TFPlayer.CritPain" );
	PrecacheScriptSound( "TFPlayer.CritDeath" );
	PrecacheScriptSound( "TFPlayer.FreezeCam" );
	PrecacheScriptSound( "TFPlayer.Drown" );
	PrecacheScriptSound( "TFPlayer.AttackerPain" );
	PrecacheScriptSound( "TFPlayer.SaveMe" );
	PrecacheScriptSound( "TFPlayer.MedicChargedDeath" );
	PrecacheScriptSound( "TFPlayer.ArmorStepLeft" );
	PrecacheScriptSound( "TFPlayer.ArmorStepRight" );
	PrecacheScriptSound( "TFPlayer.VIPDeath" );
	PrecacheScriptSound( "TFPlayer.Disconnect" );
	PrecacheScriptSound( "TFPlayer.HeadshotHelmet" );
	PrecacheScriptSound( "Halloween.PlayerScream" );

	// Precache particle systems
	PrecacheParticleSystem( "crit_text" );
	PrecacheParticleSystem( "minicrit_text" );
	PrecacheParticleSystem( "cig_smoke" );
	PrecacheParticleSystem( "bday_blood" );
	PrecacheParticleSystem( "bday_confetti" );
	PrecacheParticleSystem( "speech_mediccall" );
	PrecacheParticleSystem( "speech_mediccall_auto" );
	PrecacheParticleSystem( "speech_typing" );
	PrecacheParticleSystem( "speech_voice" );
	PrecacheTeamParticles( "particle_nemesis_%s" );
	PrecacheTeamParticles( "spy_start_disguise_%s" );
	PrecacheTeamParticles( "burningplayer_%s" );
	PrecacheParticleSystem( "burningplayer_corpse" );
	PrecacheParticleSystem( "burninggibs" );
	PrecacheTeamParticles( "healthlost_%s", g_aTeamNamesShort );
	PrecacheTeamParticles( "healthgained_%s", g_aTeamNamesShort );
	PrecacheTeamParticles( "healthgained_%s_large", g_aTeamNamesShort );
	PrecacheTeamParticles( "electrocuted_%s" );
	PrecacheTeamParticles( "electrocuted_gibbed_%s" );
	PrecacheTeamParticles( "healhuff_%s", g_aTeamNamesShort );
	PrecacheTeamParticles( "highfive_%s" );
	PrecacheTeamParticles( "highfive_%s_old" );
	PrecacheParticleSystem( "blood_spray_red_01" );
	PrecacheParticleSystem( "blood_spray_red_01_far" );
	PrecacheParticleSystem( "water_blood_impact_red_01" );
	PrecacheParticleSystem( "blood_impact_red_01" );
	PrecacheParticleSystem( "blood_headshot" );
	PrecacheParticleSystem( "water_playerdive" );
	PrecacheParticleSystem( "water_playeremerge" );
	PrecacheParticleSystem( "doublejump_puff" );
	PrecacheParticleSystem( "rocketjump_smoke" );
	PrecacheParticleSystem( "speed_boost_trail" );
	PrecacheParticleSystem( "player_poof" );
	PrecacheParticleSystem( "blood_decap" );

	PrecacheTeamParticles( "jumppad_trail_%s" );
	PrecacheTeamParticles( "jumppad_trail_%s_long" );
	PrecacheTeamParticles( "jumppad_jump_puff_smoke_%s" );

	// Civilian gibbed blood effects.
	PrecacheParticleSystem( "blood_trail_red_01_money_initial" );
	PrecacheParticleSystem( "cicilian_coins_1" );
	PrecacheParticleSystem( "cicilian_coins_2" );

	// For Medieval mode.
	UTIL_PrecacheOther( "item_healthkit_small" );

	// Condition Effects.
	PrecacheScriptSound( "Player.ResistanceLight" );
	PrecacheScriptSound( "Player.ResistanceMedium" );
	PrecacheScriptSound( "Player.ResistanceHeavy" );
	PrecacheScriptSound( "TFPlayer.TranqHit" );
	PrecacheScriptSound( "TFPlayer.VIPBuffUp" );
	PrecacheScriptSound( "TFPlayer.VIPBuffDown" );
	PrecacheScriptSound( "DisciplineDevice.PowerUp" ); // Haste i.e. TF_COND_CIV_SPEEDBUFF
	PrecacheScriptSound( "DisciplineDevice.PowerDown" );


	// Used for stored crits
	PrecacheScriptSound("TFPlayer.GainStoredCrit");
	PrecacheScriptSound("TFPlayer.LoseStoredCrit");

	// Squish!
	PrecacheScriptSound( "Weapon_Mantreads.Impact" );
#ifdef TF2C_BETA
	PrecacheScriptSound("Weapon_Anchor.Impact");
	PrecacheScriptSound("Weapon_Anchor.ImpactBoom");
#endif

	// Jumppad
	PrecacheScriptSound("TFPlayer.JumppadJumpLoop");
	PrecacheScriptSound("TFPlayer.SafeLanding");
	PrecacheScriptSound("TFPlayer.SafestLanding");

	// Beacon
	PrecacheScriptSound("TFPlayer.Beacon_Tick");
	PrecacheScriptSound("TFPlayer.Beacon_Tock");

	PrecacheTeamParticles( "overhealedplayer_%s_pluses" );
	PrecacheTeamParticles( "critgun_weaponmodel_%s", g_aTeamNamesShort );
	PrecacheTeamParticles( "critgun_weaponmodel_viewmodel_%s", g_aTeamNamesShort );
	PrecacheTeamParticles( "player_recent_teleport_%s" );
	PrecacheParticleSystem( "mark_for_death" );
	PrecacheParticleSystem( "tranq_stars" );
	PrecacheTeamParticles( "civilianbuff_%s_buffed" );

#ifdef TF2C_BETA
	// Anchor
	PrecacheParticleSystem( EARTHQUAKE_PARTICLE_NAME );
	PrecacheParticleSystem( EARTHQUAKE_PARTICLE_NAME_SNOW );

	// Heal Launcher (Nurnberg Nader) impact particle effect
	PrecacheTeamParticles( "repair_claw_heal_%s3" );
	PrecacheScriptSound("Weapon_Anchor.CritDonked");
#endif

					 
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the player models and player model gibs.
//-----------------------------------------------------------------------------
void CTFPlayer::PrecachePlayerModels( void )
{
	int i, c;
	const char *pszModel;
	for ( i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		pszModel = GetPlayerClassData( i )->m_szModelName;
		if ( pszModel[0] )
		{
			PrecacheGibsForModel( PrecacheModel( pszModel ) );
		}

		// Precache the hardware facial morphed models as well.
		pszModel = GetPlayerClassData( i )->m_szHWMModelName;
		if ( pszModel[0] )
		{
			PrecacheModel( pszModel );
		}

		pszModel = GetPlayerClassData( i )->m_szModelHandsName;
		if ( pszModel[0] )
		{
			PrecacheModel( pszModel );
		}
	}

	if ( TFGameRules() )
	{
		if (TFGameRules()->IsBirthday())
		{
			for (i = 0, c = ARRAYSIZE(g_pszBDayGibs); i < c; i++)
			{
				PrecacheModel(g_pszBDayGibs[i]);
			}
			PrecacheModel("models/effects/bday_hat.mdl");
		}

		if ( TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
		{
			PrecacheModel("models/effects/santa_hat.mdl");
		}
	}

	PrecacheModel( TF_SPY_MASK_MODEL );

	// The Soldier's helmet sometimes pops off upon a headshot.
	PrecacheModel( "models/player/gibs/soldiergib008.mdl" );

	// Precache player class sounds.
	for ( i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; ++i )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( i );
		PrecacheScriptSound( pData->m_szDeathSound );
		PrecacheScriptSound( pData->m_szCritDeathSound );
		PrecacheScriptSound( pData->m_szMeleeDeathSound );
		PrecacheScriptSound( pData->m_szExplosionDeathSound );
	}
}


bool CTFPlayer::IsReadyToPlay( void )
{
	return ( GetTeamNumber() >= FIRST_GAME_TEAM &&
		GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED );
}


bool CTFPlayer::IsReadyToSpawn( void )
{
	if ( IsClassMenuOpen() )
		return false;

	return ( StateGet() != TF_STATE_DYING );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
// when they next finish picking a class.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGainInstantSpawn( void )
{
	if ( GetTeamNumber() < FIRST_GAME_TEAM )
		return false;

	return ( GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED || IsClassMenuOpen() );
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores.
//-----------------------------------------------------------------------------
void CTFPlayer::ResetPerRoundStats( void )
{
	BaseClass::ResetPerRoundStats();
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores.
//-----------------------------------------------------------------------------
void CTFPlayer::ResetScores( void )
{
	CTF_GameStats.ResetPlayerStats( this );
	RemoveNemesisRelationships();
	ResetWasVIP();
	BaseClass::ResetScores();
}

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
extern ConVar tf2c_remote_item_schema;
#endif


void CTFPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	// Sets some default cvar values for bots.
	engine->SetFakeClientConVarValue( edict(), "hud_classautokill", "1" );
	engine->SetFakeClientConVarValue( edict(), "tf_medigun_autoheal", "0" );
	engine->SetFakeClientConVarValue( edict(), "cl_autorezoom", "1" );
	engine->SetFakeClientConVarValue( edict(), "cl_autoreload", "0" );
	engine->SetFakeClientConVarValue( edict(), "cl_flipviewmodels", "0" );

	engine->SetFakeClientConVarValue( edict(), "fov_desired", "90" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_fov", "54" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_x", "0" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_y", "0" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_z", "0" );

	engine->SetFakeClientConVarValue( edict(), "tf_use_min_viewmodels", "0" );
	engine->SetFakeClientConVarValue( edict(), "tf2c_invisible_arms", "0" );

	engine->SetFakeClientConVarValue( edict(), "tf2c_zoom_hold_sniper", "0" );
	engine->SetFakeClientConVarValue( edict(), "tf2c_proximity_voice", "0" );
	engine->SetFakeClientConVarValue( edict(), "tf2c_dev_mark", "1" );

	engine->SetFakeClientConVarValue(edict(), "tf2c_fixedspread_preference", "0");

	engine->SetFakeClientConVarValue(edict(), "tf2c_avoid_becoming_vip", "0");

	engine->SetFakeClientConVarValue(edict(), "tf2c_spywalk_inverted", "0");
	engine->SetFakeClientConVarValue(edict(), "tf2c_centerfire_preference", "0");

	m_AttributeManager.InitializeAttributes( this );
	m_AttributeManager.SetPlayer( this );

	m_AttributeList.SetManager( &m_AttributeManager );

	m_iMaxSentryKills = 0;
	CTF_GameStats.Event_MaxSentryKills( this, 0 );

	StateEnter( TF_STATE_WELCOME );

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
	const char *pszRemoteItemSchema = tf2c_remote_item_schema.GetString();
	if ( V_strcmp( pszRemoteItemSchema, "" ) )
	{
		engine->ClientCommand( edict(), "_tf2c_remote_item_schema \"%s\"\n", pszRemoteItemSchema );
	}
#endif
}


void CTFPlayer::Spawn()
{
	MDLCACHE_CRITICAL_SECTION();

	m_bIsABot = IsBot();

	CTFBot *pBot = ToTFBot( this );
	m_nBotSkill = ( pBot ? pBot->GetSkill() : 0 );

	m_bIsMiniBoss = false;

	m_flSpawnTime = gpGlobals->curtime;
	UpdateModel();

	SetMoveType( MOVETYPE_WALK );
	BaseClass::Spawn();

	// Create our off hand viewmodel if necessary.
	CreateViewModel( 1 );

	// Make sure it has no model set, in case it had one before.
	GetViewModel( 1 )->SetWeaponModel( NULL, NULL );

	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on,
	// so if we're in the welcome state, call its enter function to reset.
	if ( m_Shared.InState( TF_STATE_WELCOME ) )
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if ( m_Shared.InState( TF_STATE_DYING ) )
	{
		StateTransition( TF_STATE_ACTIVE );
	}

	// If they're spawning into the world as fresh meat, give them items and stuff.
	if ( m_Shared.InState( TF_STATE_ACTIVE ) )
	{
		// Reset our bodygroups to default.
		m_nBody = 0;
		m_Shared.m_nDisguiseBody = 0;

		// Remove our disguise each time we spawn.
		m_Shared.RemoveDisguise();

		InitClass();

		// Remove conc'd, burning, rotting, hallucinating, etc.
		m_Shared.RemoveAllCond();

		m_Shared.RemoveAllRemainingAfterburn();


		UpdateSkin( GetTeamNumber() );

		DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
	
		// If this is true it means I respawned without dying (changing class inside the spawn room) but doesn't necessarily mean that my healers have stopped healing me
		// This means that medics can still be linked to me but my health would not be affected since this condition is not set.
		// So instead of going and forcing every healer on me to stop healing we just set this condition back on. 
		// If the game decides I shouldn't be healed by someone (LOS, Distance, etc) they will break the link themselves like usual.
		if ( m_Shared.GetNumHealers() > 0 )
		{
			m_Shared.AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, m_Shared.GetFirstHealer() );
		}

		if ( !m_bSeenRoundInfo )
		{
			TFGameRules()->ShowRoundInfoPanel( this );
			m_bSeenRoundInfo = true;
		}

		if ( IsInCommentaryMode() && !IsFakeClient() )
		{
			// Player is spawning in commentary mode. Tell the commentary system.
			CBaseEntity *pEnt = NULL;
			variant_t emptyVariant;
			while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "commentary_auto" ) ) != NULL )
			{
				pEnt->AcceptInput( "MultiplayerSpawned", this, this, emptyVariant, 0 );
			}
		}
	}

	m_nForceTauntCam = 0;

	CTF_GameStats.Event_PlayerSpawned( this );

	m_iSpawnCounter++;
	m_bAllowInstantSpawn = false;

	m_Shared.SetSpyCloakMeter( 100.0f );

	m_Shared.ClearDamageEvents();
	ClearDamagerHistory();

	m_flLastDamageTime = 0.0f;

	m_flNextVoiceCommandTime = gpGlobals->curtime;

	ClearZoomOwner();
	SetFOV( this , 0 );

	SetViewOffset( GetClassEyeHeight() );

	RemoveAllScenesInvolvingActor( this );
	ClearExpression();
	ClearWeaponFireScene();
	m_flNextSpeakWeaponFire = gpGlobals->curtime;

	m_bIsIdle = false;
	m_flPowerPlayTime = 0.0f;

	m_iLastLifeActiveWeapon = 0;
	m_iLastLifeLastWeapon = 1;

	m_iBlastJumpState = 0;

	m_Shared.SetDesiredWeaponIndex( -1 );

	m_bPreserveRandomLoadout = false;

	m_flRespawnTimeOverride = -1.0;

	m_iOldStunFlags = 0;

	m_flTotalHealthRegen = m_flTotalDisguiseHealthRegen = 0.0f;

	m_bNextAttackIsMinicrit = false;

	m_pJumpPadSoundLoop = nullptr;

	bTickTockOrder = false;

	// This makes the surrounding box always the same size as the standing collision box
	// helps with parts of the hitboxes that extend out of the crouching hitbox, eg. Heavy.
	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	// Hack to hide the chat on the background map.
	if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_Local.m_iHideHUD |= HIDEHUD_CHAT;
	}

	// Force respawn every single person on our team when we spawn.
	if ( IsVIP() && !m_pVIPInstantlyRespawned )
	{
		CTFPlayer *pTeamPlayer;
		for ( int i = 0, c = GetTeam()->GetNumPlayers(); i < c; i++ )
		{
			pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer( i ) );
			if ( pTeamPlayer && !pTeamPlayer->IsAlive() )
			{
				pTeamPlayer->ForceRespawn();
			}
		}
	}

	m_pVIPInstantlyRespawned = false;

	IGameEvent *event = gameeventmanager->CreateEvent( "player_spawn" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "class", GetPlayerClass()->GetClassIndex() );
		gameeventmanager->FireEvent( event );
	}

	m_Shared.Spawn();

	// Send use input to all entities named "game_playerspawn"
	FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

	m_itMedicCall.Invalidate();
	m_ctRecentSap.Invalidate();

	RemoveNemesisRelationships( true );
}

extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;


int CTFPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pInfo->m_pClientEnt ) );
	Assert( pPlayer );

	// Always transmit all players to spectators.
	if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return FL_EDICT_ALWAYS;

#ifdef ITEM_TAUNTING
	// Always transmit players with looping taunt sounds.
	if ( m_bPlayingTauntSound )
		return FL_EDICT_ALWAYS;
#endif

	bool bProximity;
	if ( g_pVoiceGameMgrHelper->CanPlayerHearPlayer( pPlayer, this, bProximity ) && bProximity )
		return FL_EDICT_ALWAYS;

	if ( InSameTeam( pPlayer ) )
	{
		// Transmit teammates to dead players.
		if ( !pPlayer->IsAlive() )
			return FL_EDICT_ALWAYS;

		// Transmit all teammates to Medics. This is required for Medic caller icons to work.
		if ( pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			return FL_EDICT_ALWAYS;

		// Transmit flag carrier.
		if ( HasTheFlag() )
			return FL_EDICT_ALWAYS;

		// Transmit VIP.
		if ( IsVIP() )
			return FL_EDICT_ALWAYS;
	}
	else
	{
		if( m_Shared.InCond( TF_COND_MARKED_OUTLINE ) )
			return FL_EDICT_ALWAYS;
		// surprise spy radar
		if (!(m_Shared.IsStealthed() || m_Shared.DisguiseFoolsTeam(pPlayer->GetTeamNumber())))
		{
			float flRadarRange = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer->GetActiveTFWeapon(), flRadarRange, sapper_is_radar);
			if (flRadarRange > 0.0f)
			{
				float flRadius2 = Square(flRadarRange);
				float flDist2 = 0.0f;
				Vector vecSegment;
				Vector vecTargetPoint;

				vecTargetPoint = pPlayer->GetAbsOrigin();
				vecTargetPoint += pPlayer->GetViewOffset();
				VectorSubtract(vecTargetPoint, GetAbsOrigin(), vecSegment);
				flDist2 = vecSegment.LengthSqr();
				if (flDist2 <= flRadius2)
				{
					return FL_EDICT_ALWAYS;
				}
			}
		}
	}

	return BaseClass::ShouldTransmit( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all nemesis relationships between this player and others.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveNemesisRelationships( bool bTeammatesOnly /*= false*/ )
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != this && ( !bTeammatesOnly || !IsEnemy( pTemp ) ) )
		{
			// Set this player to be not dominating anyone else.
			m_Shared.SetPlayerDominated( pTemp, false );

			// Set no one else to be dominating this player.
			pTemp->m_Shared.SetPlayerDominated( this, false );
			pTemp->UpdateDominationsCount();
		}
	}

	UpdateDominationsCount();

	// Reset the matrix of who has killed whom with respect to this player.
	CTF_GameStats.ResetKillHistory( this, bTeammatesOnly );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayer::Regenerate( void )
{
	// We may have been boosted over our max health.
	// If we have, restore it after we reset out class values.
	int iCurrentHealth = GetHealth();
	int iCurrentMaxHealth = GetMaxHealth();
	m_bRegenerating = true;
	InitClass();
	m_bRegenerating = false;

	SetHealth( Max( iCurrentHealth + ( GetMaxHealth() - iCurrentMaxHealth ), GetMaxHealth() ) );

	m_Shared.HealNegativeConds();

	if ( tf2c_spy_cloak_ammo_refill.GetBool() )
	{
		// Fill Spy cloak.
		m_Shared.SetSpyCloakMeter( 100.0f );
	}
}


void CTFPlayer::Restock( bool bHealth, bool bAmmo )
{
	if ( bHealth )
	{
		SetHealth( Max( GetHealth(), GetMaxHealth() ) );
		m_Shared.HealNegativeConds();
	}

	if ( bAmmo )
	{
		// Refill clip in all weapons.
		int i, c;
		CBaseCombatWeapon *pWeapon;
		for ( i = 0, c = WeaponCount(); i < c; i++ )
		{
			pWeapon = GetWeapon( i );
			if ( !pWeapon )
				continue;

			pWeapon->GiveDefaultAmmo();
		}

		for ( i = TF_AMMO_PRIMARY; i < TF_AMMO_COUNT; i++ )
		{
			SetAmmoCount( GetMaxAmmo( i ), i );
		}
	}
}


void CTFPlayer::InitClass( void )
{
	// Give default items for class.
	GiveDefaultItems();

	// Set initial health and armor based on class.
	SetMaxHealth( GetPlayerClass()->GetMaxHealth() );
	SetHealth( GetMaxHealth() );

	SetArmorValue( GetPlayerClass()->GetMaxArmor() );

	// Init the anim movement vars.
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5f );

	// Update the speed.
	TeamFortress_SetSpeed();
	TeamFortress_SetGravity();
}


void CTFPlayer::CreateViewModel( int iViewModel )
{
	Assert( iViewModel >= 0 && iViewModel < MAX_VIEWMODELS );
	if ( GetViewModel( iViewModel ) )
		return;

	CTFViewModel *pViewModel = static_cast<CTFViewModel *>( CreateEntityByName( "tf_viewmodel" ) );
	if ( pViewModel )
	{
		pViewModel->SetAbsOrigin( GetAbsOrigin() );
		pViewModel->SetOwner( this );
		pViewModel->SetIndex( iViewModel );
		DispatchSpawn( pViewModel );
		pViewModel->FollowEntity( this, false );
		pViewModel->SetOwnerEntity( this );
		m_hViewModel.Set( iViewModel, pViewModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the view model for the player's off hand.
//-----------------------------------------------------------------------------
CBaseViewModel *CTFPlayer::GetOffHandViewModel()
{
	// Off hand model is slot 1.
	return GetViewModel( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends the specified animation activity to the off hand view model.
//-----------------------------------------------------------------------------
void CTFPlayer::SendOffHandViewModelActivity( Activity activity )
{
	CBaseViewModel *pViewModel = GetOffHandViewModel();
	if ( pViewModel )
	{
		int iSequence = pViewModel->SelectWeightedSequence( activity );
		pViewModel->SendViewModelMatchingSequence( iSequence );
	}
}


bool CTFPlayer::ItemsMatch( CEconItemView *pItem1, CEconItemView *pItem2, CTFWeaponBase *pWeapon )
{
	if ( pItem1 && pItem2 )
	{
		if ( pItem1->GetItemDefIndex() != pItem2->GetItemDefIndex() )
			return false;

		// Item might have different entities for each class (i.e. shotgun).
		if ( pWeapon )
		{
			int iClass = GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ? pItem1->GetClassIndex() : m_PlayerClass.GetClassIndex();
			const char *pszClassname = TranslateWeaponEntForClass( pItem1->GetEntityName(), iClass );
			if ( !FClassnameIs( pWeapon, pszClassname ) )
				return false;
		}

		return true;
	}

	return false;
}


bool CTFPlayer::ItemIsAllowed( CEconItemView *pItem )
{
	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return false;

	if ( !pItemDef->baseitem && !pItemDef->show_in_armory && !(tf2c_item_testing.GetInt() == 1 && pItemDef->testing) && !(tf2c_item_testing.GetInt() == 2) )
		return false;

	CTFGameRules *pTFGameRules = TFGameRules();
	if ( pTFGameRules->InStalemateMeleeOnly() )
	{
		// Don't allow any weapons in Melee-only Sudden Death except for melees and Spy's PDAs.
		int iSlot = pItemDef->GetLoadoutSlot( m_PlayerClass.GetClassIndex() );
		if ( iSlot < TF_FIRST_COSMETIC_SLOT && iSlot != TF_LOADOUT_SLOT_MELEE )
		{
			if ( m_PlayerClass.GetClassIndex() != TF_CLASS_SPY || iSlot < TF_LOADOUT_SLOT_PDA1 )
				return false;
		}
	}

	if ( pTFGameRules->IsInMedievalMode() )
	{
		// If we're in medieval mode don't allow any non-melee weapons that are not specifically marked.
		int iSlot = pItemDef->GetLoadoutSlot( m_PlayerClass.GetClassIndex() );
		if (iSlot < TF_FIRST_COSMETIC_SLOT)
		{
			if (iSlot != TF_LOADOUT_SLOT_MELEE)
			{
				if (m_PlayerClass.GetClassIndex() != TF_CLASS_SPY || iSlot < TF_LOADOUT_SLOT_PDA1)
				{
					int iAllowed = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(pItem, iAllowed, allowed_in_medieval_mode);
					if (!iAllowed)
						return false;
				}
			}
			else
			{
				int iBanned = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pItem, iBanned, banned_in_medieval_mode);
				if (iBanned)
					return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the player up with the default weapons, ammo, etc.
//-----------------------------------------------------------------------------
void CTFPlayer::GiveDefaultItems()
{
	// Get the player class data.
	TFPlayerClassData_t *pData = m_PlayerClass.GetData();

	RemoveAllAmmo();

	// Give ammo. Must be done before weapons, so weapons know the player has ammo for them.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		GiveAmmo( GetMaxAmmo( iAmmo ), iAmmo, true, TF_AMMO_SOURCE_RESUPPLY );
	}

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	int iActiveWeaponSlot = pActiveWeapon ? pActiveWeapon->GetSlot() : 0;

	// Give weapons.
	if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) )
	{
		ManageRandomWeapons();
	}
	else if ( tf2c_legacy_weapons.GetBool() )
	{
		ManageRegularWeaponsLegacy( pData );
	}
	else
	{
		ManageRegularWeapons();
	}

	// Give a builder weapon for each object the playerclass is allowed to build.
	ManageBuilderWeapons( pData );

	// Now that we've got weapons update our ammo counts since weapons may override max ammo.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		SetAmmoCount( GetMaxAmmo( iAmmo ), iAmmo );
	}

	if ( !m_bRegenerating )
	{
		Weapon_Switch( Weapon_GetSlot( m_bRememberActiveWeapon ? m_iLastLifeActiveWeapon : 0 ) );

		Weapon_SetLast( Weapon_GetSlot( m_bRememberLastWeapon ? m_iLastLifeLastWeapon : 1 ) );
	}
	else
	{
		Weapon_Switch( Weapon_GetSlot( iActiveWeaponSlot ) );
	}

	// We may have swapped away our current weapon at resupply locker.
	if ( !GetActiveWeapon() )
	{
		SwitchToNextBestWeapon( NULL );
	}
}


void CTFPlayer::ManageBuilderWeapons( TFPlayerClassData_t *pData )
{
	if ( pData->m_aBuildable[0] != OBJ_LAST && !TFGameRules()->InStalemateMeleeOnly() )
	{
		CEconItemView *pItem = GetLoadoutItem( m_PlayerClass.GetClassIndex(), TF_LOADOUT_SLOT_BUILDING );
		CTFWeaponBase *pBuilder = GetWeaponForLoadoutSlot( TF_LOADOUT_SLOT_BUILDING );

		if ( TFGameRules()->IsInMedievalMode() )
		{
			int iAllowed = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, iAllowed, allowed_in_medieval_mode );
			if ( !iAllowed )
				return;
		}

		// Give the player a new builder weapon when they switch between Engy and Spy.
		if ( pBuilder && !ItemsMatch( pBuilder->GetItem(), pItem, pBuilder ) )
		{
			pBuilder->UnEquip();
			pBuilder = NULL;
		}
		
		if ( pBuilder )
		{
			pBuilder->GiveDefaultAmmo();
			pBuilder->ChangeTeam( GetTeamNumber() );
			pBuilder->WeaponRegenerate();

			if ( !m_bRegenerating )
			{
				pBuilder->WeaponReset();
			}
		}
		else
		{
			GiveNamedItem( pItem->GetStaticData()->item_class, pData->m_aBuildable[0], pItem );
		}
	}
	else
	{
		// Not supposed to be holding a builder, nuke it from orbit!
		CTFWeaponBase *pWpn = GetWeaponForLoadoutSlot( TF_LOADOUT_SLOT_BUILDING );
		if ( pWpn )
		{
			pWpn->UnEquip();
		}
	}
}


void CTFPlayer::ValidateWeapons( bool bRegenerate )
{
	int iClass = m_PlayerClass.GetClassIndex();

	CTFWeaponBase *pWeapon;
	CEconItemView *pItem;
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		pWeapon = GetTFWeapon( i );
		if ( !pWeapon )
			continue;

		// Skip builder as we'll handle it separately.
		if ( pWeapon->IsWeapon( TF_WEAPON_BUILDER ) )
			continue;

		pItem = pWeapon->GetItem();
		if ( ItemIsAllowed( pItem ) )
		{
			if ( !ItemsMatch( pItem, GetLoadoutItem( iClass, pItem->GetLoadoutSlot( iClass ) ), pWeapon ) )
			{
				// If this is not a weapon we're supposed to have in this loadout slot then nuke it.
				// Either changed class or changed loadout.
				pWeapon->UnEquip();
			}
			else if ( bRegenerate )
			{
				pWeapon->ChangeTeam( GetTeamNumber() );
				pWeapon->GiveDefaultAmmo();
				pWeapon->WeaponRegenerate();

				if ( !m_bRegenerating )
				{
					pWeapon->WeaponReset();
				}
			}
		}
		else
		{
			// Not supposed to be carrying this weapon, nuke it from orbit!
			pWeapon->UnEquip();
		}
	}
}


void CTFPlayer::ValidateWearables( void )
{
	int iClass = m_PlayerClass.GetClassIndex();
	bool bIsDisguisedSpy = IsPlayerClass( TF_CLASS_SPY ) && m_Shared.InCond( TF_COND_DISGUISED );

	for ( int i = GetNumWearables() - 1; i >= 0; i-- )
	{
		CTFWearable *pWearable = static_cast<CTFWearable *>( GetWearable( i ) );
		if ( !pWearable )
		{
			m_hMyWearables.Remove( i );
			continue;
		}

		if ( bIsDisguisedSpy && pWearable->IsDisguiseWearable() )
			continue;

		// Check if this wearable is associated with a weapon, because if so,
		// it might be an extra wearable (Sniper Arrows, Medic Backpack).
		CBaseEntity *pAssociatedEntity = pWearable->GetWeaponAssociatedWith();
		if ( pAssociatedEntity )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pAssociatedEntity );
			if ( pWeapon )
			{
				// Code snippet from the above function.
				ETFLoadoutSlot iSlot = pWeapon->GetItem()->GetLoadoutSlot( iClass );
				CEconItemView *pLoadoutItem = GetLoadoutItem( iClass, iSlot );
				if ( !ItemsMatch( pWeapon->GetItem(), pLoadoutItem, pWeapon ) )
				{
					// If this is not a weapon we're supposed to have in this loadout slot then nuke its extra wearable.
					RemoveWearable( pWearable );
				}
			}
		}
		else
		{
			ETFLoadoutSlot iSlot = pWearable->GetItem()->GetLoadoutSlot( iClass );
			if ( iSlot == TF_LOADOUT_SLOT_INVALID || !ItemsMatch( pWearable->GetItem(), GetLoadoutItem( iClass, iSlot ), NULL ) )
			{
				// Not supposed to be carrying this wearable, nuke it from orbit!
				RemoveWearable( pWearable );
			}
			else if ( !m_bRegenerating )
			{
				pWearable->UpdateModelToClass();
				pWearable->Reset();
			}
			else
				pWearable->Reset();
		}
	}
}


void CTFPlayer::ManageRegularWeapons( void )
{
	ValidateWeapons( true );
	ValidateWearables();

	CBaseEntity *pExistingItem;
	CEconItemView *pItem;
	const char *pszClassname;
	for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; ++iSlot )
	{
		// Skip building slot as we'll handle it separately.
		if ( iSlot == TF_LOADOUT_SLOT_BUILDING )
			continue;

		pExistingItem = GetEntityForLoadoutSlot( (ETFLoadoutSlot)iSlot );
		if ( pExistingItem )
		{
			// Nothing else to do here.
			continue;
		}

		// Give us an item from the inventory.
		pItem = GetLoadoutItem( m_PlayerClass.GetClassIndex(), (ETFLoadoutSlot)iSlot );
		if ( pItem && ItemIsAllowed( pItem ) )
		{
			pszClassname = pItem->GetEntityName();
			Assert( pszClassname );
			if ( !V_stricmp( pszClassname, "no_entity" ) )
				continue;

			if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ATTRIBUTES ) )
			{
				GetRandomizerManager()->ApplyRandomizerAttributes( pItem, (ETFLoadoutSlot)iSlot, m_PlayerClass.GetClassIndex() );
			}

			GiveNamedItem( pszClassname, 0, pItem );
		}
	}

	PostInventoryApplication();
}


void CTFPlayer::PostInventoryApplication( void )
{
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// Using weapons lockers destroys our disguise weapon, so we might need a new one.
		m_Shared.DetermineDisguiseWeapon( false );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "post_inventory_application" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		gameeventmanager->FireEvent( event );
	}
}


void CTFPlayer::ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData )
{
	int i, c;
	CTFWeaponBase *pWeapon;
	for ( i = 0, c = WeaponCount(); i < c; i++ )
	{
		pWeapon = GetTFWeapon( i );
		if ( !pWeapon || pWeapon->IsWeapon( TF_WEAPON_BUILDER ) )
			continue;

		if ( !ClassOwnsWeapon( m_PlayerClass.GetClassIndex(), pWeapon->GetWeaponID() ) )
		{
			// Not supposed to carry this weapon, nuke it.
			// Don't nuke builders since they will be nuked if we don't need them later.
			pWeapon->UnEquip();
		}
		else
		{
			pWeapon->ChangeTeam( GetTeamNumber() );
			pWeapon->GiveDefaultAmmo();
			pWeapon->WeaponRegenerate();

			if ( !m_bRegenerating )
			{
				pWeapon->WeaponReset();
			}
		}
	}

	ETFWeaponID iWeaponID;
	for ( i = 0; i < TF_PLAYER_WEAPON_COUNT; ++i )
	{
		iWeaponID = pData->m_aWeapons[i];
		if ( iWeaponID == TF_WEAPON_NONE )
			continue;

		if ( Weapon_OwnsThisID( iWeaponID ) )
			continue;

		GiveNamedItem( WeaponIdToClassname( iWeaponID ) );
	}
}


void CTFPlayer::RememberLastWeapons( void )
{
	// Don't remember PDAs.
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon && pWeapon->GetSlot() < 3 )
	{
		m_iLastLifeActiveWeapon = pWeapon->GetSlot();
	}
	else
	{
		m_iLastLifeActiveWeapon = 0;
	}

	pWeapon = static_cast<CTFWeaponBase *>( GetLastWeapon() );
	if ( pWeapon )
	{
		m_iLastLifeLastWeapon = pWeapon->GetSlot();
	}
	else
	{
		m_iLastLifeLastWeapon = 1;
	}
}


void CTFPlayer::ForgetLastWeapons( void )
{
	m_iLastLifeActiveWeapon = 0;
	m_iLastLifeLastWeapon = 1;
}


void CTFPlayer::ManageRandomWeapons( void )
{
	// Cheap way to check if we generated a random loadout yet.
	if ( !m_RandomItems[0].IsValid() || ( !m_bPreserveRandomLoadout && !m_bRegenerating ) )
	{
		// Generate new random loadout loadout.
		for ( int iSlot = 0; iSlot < TF_FIRST_COSMETIC_SLOT; ++iSlot )
		{
			// Skip building slot as we'll handle it separately.
			if ( iSlot == TF_LOADOUT_SLOT_BUILDING )
				continue;

			// Skip secondary if we're a spy, otherwise it will mess with Sapper.
			if ( iSlot == TF_LOADOUT_SLOT_SECONDARY && IsPlayerClass( TF_CLASS_SPY, true ) )
			{
				m_RandomItems[iSlot].Invalidate();
				continue;
			}

			int iClass = TF_CLASS_UNDEFINED, iWeapon = 0;

			if ( iSlot > TF_LOADOUT_SLOT_MELEE )
			{
				// Give the approciate tools.
				iClass = m_PlayerClass.GetClassIndex();
			}
			else
			{
				// Choose random class that has weapons at this slot.
				CUtlVector<int> aClasses;

				for ( int i = 0; i <= TF_CLASS_COUNT; i++ )
				{
					if ( GetTFInventory()->GetItem( i, (ETFLoadoutSlot)iSlot, 0 ) )
					{
						aClasses.AddToTail( i );
					}
				}

				Assert( !aClasses.IsEmpty() );

				if ( aClasses.IsEmpty() )
				{
					// Wat.
					m_RandomItems[iSlot].Invalidate();
					continue;
				}

				iClass = aClasses.Random();
				iWeapon = !tf2c_force_stock_weapons.GetBool() ? RandomInt( 0, GetTFInventory()->GetNumItems( iClass, (ETFLoadoutSlot)iSlot ) - 1 ) : 0;
			}

			// Select an item from the inventory.
			CEconItemView *pItem = GetTFInventory()->GetItem( iClass, (ETFLoadoutSlot)iSlot, iWeapon );
			if ( pItem )
			{
				m_RandomItems[iSlot] = *pItem;
			}
			else
			{
				m_RandomItems[iSlot].Invalidate();
			}
		}
	}

	ManageRegularWeapons();
}

//-----------------------------------------------------------------------------
// Purpose: Get preset from the vector.
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayer::GetLoadoutItem( int iClass, ETFLoadoutSlot iSlot )
{
	if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) && m_RandomItems[iSlot].IsValid() )
		return &m_RandomItems[iSlot];

	int iPreset = !tf2c_force_stock_weapons.GetBool() ? m_iItemPresets[iClass][iSlot] : 0;

	// Check if experimental weapons are allowed, otherwise equip the default weapon.
	if (!tf2c_force_stock_weapons.GetBool() && iSlot < TF_LOADOUT_SLOT_COUNT && iSlot >= 0)
	{
		CEconItemView *pItem = GetTFInventory()->GetItem( iClass, iSlot, iPreset );
		if ( pItem )
		{
			if (!ItemIsExperimental(pItem))
			{
				iPreset = m_iItemPresets[iClass][iSlot];
			}
			else
			{
				iPreset = 0;
			}
		}
	}

	return GetTFInventory()->GetItem( iClass, iSlot, iPreset );
}

//-----------------------------------------------------------------------------
// Purpose: Check if item is experimental
//-----------------------------------------------------------------------------
int CTFPlayer::ItemIsExperimental( CEconItemView *pItem ) 
{
	int iIsExperimental = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, iIsExperimental, experimental_weapon );
	return ( iIsExperimental != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: ItemPreset command handle
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_ItemPreset( int iClass, int iSlotNum, int iPresetNum )
{
	if ( !GetTFInventory()->CheckValidSlot( iClass, (ETFLoadoutSlot)iSlotNum ) )
		return;

	if ( !GetTFInventory()->CheckValidItem( iClass, (ETFLoadoutSlot)iSlotNum, iPresetNum ) )
		return;

	m_iItemPresets[iClass][iSlotNum] = iPresetNum;
}

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player, setting the item ID. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CTFPlayer::GiveNamedItem( const char *pszName, int iSubType, CEconItemView *pItem /*= NULL*/, int iAmmo /*= TF_GIVEAMMO_NONE*/, bool bDisguiseWeapon /*= false*/ )
{
	if ( !bDisguiseWeapon )
	{
		// In Randomizer, translate for the class we "stole" this weapon from.
		int iClass = m_PlayerClass.GetClassIndex();
		if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) && pItem && pItem->GetClassIndex() )
		{
			iClass = pItem->GetClassIndex();
		}

		pszName = TranslateWeaponEntForClass( pszName, iClass );
	}

	if ( !pszName )
		return NULL;

	// If I already own this type don't create one.
	if ( Weapon_OwnsThisType( pszName, iSubType ) && !bDisguiseWeapon )
		return NULL;

	CBaseEntity *pEntity = CBaseEntity::CreateNoSpawn( pszName, GetAbsOrigin(), vec3_angle );
	if ( !pEntity )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	// Set Econ Item on the weapon.
	CEconEntity *pEcon = dynamic_cast<CEconEntity *>( pEntity );
	if ( pEcon && pItem )
	{
		pEcon->SetItem( *pItem );

		if ( bDisguiseWeapon )
		{
			// Putting this here for now, since items are the only thing to really need this.
			pEcon->ChangeTeam( m_Shared.IsDisguised() && !m_Shared.InCond( TF_COND_DISGUISING ) ? m_Shared.GetDisguiseTeam() : GetTeamNumber() );
		}
	}

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pEntity );
	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );

		// Give ammo for this weapon if asked.
		if ( pWeapon->UsesPrimaryAmmo() )
		{
			if ( iAmmo == TF_GIVEAMMO_MAX )
			{
				SetAmmoCount( GetMaxAmmo( pWeapon->GetPrimaryAmmoType() ), pWeapon->GetPrimaryAmmoType() );
			}
			else if ( iAmmo != TF_GIVEAMMO_NONE )
			{
				SetAmmoCount( iAmmo, pWeapon->GetPrimaryAmmoType() );
			}
		}
	}

	DispatchSpawn( pEntity );
	pEntity->Activate();

	pEntity->AddSpawnFlags( SF_NORESPAWN );

	if ( !pEntity->IsMarkedForDeletion() )
	{
		if ( pEcon && !bDisguiseWeapon )
		{
			pEcon->GiveTo( this );
		}
		else
		{
			pEntity->Touch( this );
		}

		if ( pWeapon )
		{
			if ( !GetActiveWeapon() && !bDisguiseWeapon )
			{
				// Failed to switch to a new weapon or replaced a weapon with a wearable.
				SwitchToNextBestWeapon( NULL );
			}
		}
	}

	return pEntity;
}


CBaseEntity	*CTFPlayer::GiveEconItem( const char *pszName, int iSubType, int iAmmo )
{
	CEconItemView econItem( pszName );
	if ( !econItem.IsValid() )
	{
		Warning( "Attempted to give unknown item %s!\n", pszName );
		return NULL;
	}

	const char *pszClassname = econItem.GetEntityName();
	if ( V_stricmp( pszClassname, "no_entity" ) == 0 )
	{
		Warning( "Item %s is not an entity!\n", pszName );
		return NULL;
	}

	return GiveNamedItem( pszClassname, iSubType, &econItem, iAmmo );
}

//-----------------------------------------------------------------------------
// Purpose: Find a spawn point for the player.
//-----------------------------------------------------------------------------
CBaseEntity* CTFPlayer::EntSelectSpawnPoint()
{
	int iTeamNumber = GetTeamNumber();

	CBaseEntity *pSpot = g_pLastSpawnPoints[ iTeamNumber ];
	const char *pSpawnPointName = "";

	switch( iTeamNumber )
	{
		case TF_TEAM_RED:
		case TF_TEAM_BLUE:
		case TF_TEAM_GREEN:
		case TF_TEAM_YELLOW:
		{
			pSpawnPointName = "info_player_teamspawn";

			if ( SelectSpawnSpot( pSpawnPointName, pSpot ) )
			{
				g_pLastSpawnPoints[ iTeamNumber ] = pSpot;
			}

			break;
		}
		case TEAM_SPECTATOR:
		case TEAM_UNASSIGNED:
		default:
		{
			pSpot = CBaseEntity::Instance( INDEXENT( 0 ) );
			break;		
		}
	}

	if ( !pSpot )
	{
		Warning( "PutClientInServer: no %s on level\n", pSpawnPointName );
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	return pSpot;
} 


bool CTFPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Since we're not searching from the start the first result can be NULL.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	if ( !pSpot )
	{
		// Still NULL? That means there're no spawn points at all, bail.
		return false;
	}

	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	bool bIgnorePlayers = false;

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if( TFGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// Found a valid spawn point.
				return true;
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		if ( pSpot == pFirstSpot && !bIgnorePlayers )
		{
			// Loop through again, ignoring players
			bIgnorePlayers = true;
			pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
		}
	} 
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot ); 

	return false;
}


void CTFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData ); // Send to any clients who can see this guy.
}


void CTFPlayer::PhysObjectSleep()
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	if ( pPhysObject )
	{
		pPhysObject->Sleep();
	}
}


void CTFPlayer::PhysObjectWake()
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	if ( pPhysObject )
	{
		pPhysObject->Wake();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create an array of teams that have the same least amount of players and pick a random one of them
//-----------------------------------------------------------------------------
int CTFPlayer::GetAutoTeam( void )
{
	int iLeastPlayers = MAX_PLAYERS + 1;

	CUtlVector<int> vec_TeamsLeastPlayers;

	int i, c, iNumPlayers;
	CTeam *pTeam;
	for (i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++)
	{
		pTeam = GetGlobalTeam(i);
		iNumPlayers = pTeam->GetNumPlayers();

		// Don't count VIPs.
		CTFTeam *pTFTeam = dynamic_cast<CTFTeam *>(pTeam);
		if (TFGameRules()->IsVIPMode() && pTFTeam && pTFTeam->GetVIP())
		{
			iNumPlayers = iNumPlayers == 1 ? 0 : pTeam->GetNumPlayers() - 1;
		}

		if (iNumPlayers == iLeastPlayers)
		{
			vec_TeamsLeastPlayers.AddToTail(i);
		}
		else if (iNumPlayers < iLeastPlayers)
		{
			iLeastPlayers = iNumPlayers;
			vec_TeamsLeastPlayers.RemoveAll();
			vec_TeamsLeastPlayers.AddToTail(i);
		}
	}

	//I have no idea how can this even happen but here is error handling
	if (!vec_TeamsLeastPlayers.Count())
		return TEAM_INVALID;

	return vec_TeamsLeastPlayers[RandomInt(0, vec_TeamsLeastPlayers.Count() - 1)];
}


void CTFPlayer::HandleCommand_JoinTeam( const char *pTeamName )
{
	// Losers can't change team during bonus time.
	if ( !IsBot() &&
		TFGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
		GetTeamNumber() != TFGameRules()->GetWinningTeam() &&
		GetTeamNumber() >= FIRST_GAME_TEAM )
		return;

	if ( TFGameRules()->IsInArenaQueueMode() )
	{
		HandleCommand_JoinTeam_Arena( pTeamName );
		return;
	}

	int iTeam = TEAM_INVALID;
	bool bAuto = false;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = GetAutoTeam();
		bAuto = true;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs( "Invalid team \"%s\".", pTeamName ) );
		return;
	}

	// We wouldn't change the team.
	if ( iTeam == GetTeamNumber() )
		return;

	if ( iTeam == TEAM_SPECTATOR )
	{
		// Prevent this if the cvar is set.
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			CSingleUserRecipientFilter filter( this );
			TFGameRules()->SendHudNotification( filter, "#Cannot_Be_Spectator", "ico_notify_flag_moving", GetTeamNumber() );
			return;
		}

		ChangeTeam( TEAM_SPECTATOR );
	}
	else
	{
		// If this join would unbalance the teams, refuse
		// come up with a better way to tell the player they tried to join a full team!
		if ( !IsBot() && TFGameRules()->WouldChangeUnbalanceTeams( iTeam, GetTeamNumber() ) )
		{
			ShowTeamMenu();
			return;
		}

		ChangeTeam( iTeam, bAuto, false );

		SelectInitialClass();
	}
}


void CTFPlayer::HandleCommand_JoinTeam_Arena( const char *pTeamName )
{
	int iTeam = TEAM_INVALID;

	// Some genius at Valve decided that "spectate" should put you IN the queue
	// and "spectatearena" should put you OUT of the queue.
	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = FIRST_GAME_TEAM;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = FIRST_GAME_TEAM;
	}
	else if ( stricmp( pTeamName, "spectatearena" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( V_stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
		return;

	bool bWantSpectate = ( iTeam < FIRST_GAME_TEAM );

	// We're not actually changing teams, jointeam command puts us in or out of the queue instead.
	if ( bWantSpectate == m_bArenaSpectator && GetTeamNumber() != TEAM_UNASSIGNED )
		return;

	// NOTE: the usual "cannot join spectator" test is NOT done here for arena mode
	m_bArenaSpectator = bWantSpectate;
	ChangeTeam( TEAM_SPECTATOR, false, true );

	if ( m_bArenaSpectator )
	{
		SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );
		TFGameRules()->RemovePlayerFromQueue( this );
		TFGameRules()->Arena_ClientDisconnect( GetPlayerName() );
	}
	else
	{
		SelectInitialClass();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without using the game menus
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam_NoMenus( const char *pTeamName )
{
	Msg( "Client command HandleCommand_JoinTeam_NoMenus: %s\n", pTeamName );

	// Only expected to be used on the 360 when players leave the lobby to start a new game.
	if ( !IsInCommentaryMode() )
	{
		Assert( GetTeamNumber() == TEAM_UNASSIGNED );
	}

	int iTeam = TEAM_SPECTATOR;
	if ( Q_stricmp( pTeamName, "spectate" ) )
	{
		for ( int i = 0; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	ForceChangeTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without suiciding.
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam_NoKill( const char *pTeamName )
{
	int iTeam = TEAM_INVALID;
	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = GetAutoTeam();
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs( "Invalid team \"%s\".", pTeamName ) );
		return;
	}

	// We wouldn't change the team.
	if ( iTeam == GetTeamNumber() )
		return;

	BaseClass::ChangeTeam( iTeam, false, true );
}

//-----------------------------------------------------------------------------
// Purpose: Player has been forcefully changed to another team.
//-----------------------------------------------------------------------------
void CTFPlayer::ForceChangeTeam( int iTeamNum )
{
	int iNewTeam = iTeamNum;
	bool bAuto = false;

	if ( iNewTeam == TF_TEAM_AUTOASSIGN )
	{
		iNewTeam = GetAutoTeam();
		bAuto = true;
	}

	if ( !TFTeamMgr()->IsValidTeam( iNewTeam ) )
	{
		Warning( "CTFPlayer::ForceChangeTeam( %d ) - invalid team index.\n", iNewTeam );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// If this is our current team, just abort.
	if ( iNewTeam == iOldTeam )
		return;

	m_iLastTeamNum = iOldTeam;

	if ( iNewTeam == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iNewTeam == TEAM_SPECTATOR )
	{
		CTF_GameStats.Event_PlayerMovedToSpectators( this );
		CleanupOnDeath( false );

		m_flSpectatorTime = gpGlobals->curtime;
		m_bIsIdle = false;
		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();

		if ( TFGameRules()->IsInArenaQueueMode() )
		{
			TFGameRules()->AddPlayerToQueueHead( this );
		}

		// Do we have fadetoblack on? (need to fade their screen back in).
		if ( mp_fadetoblack.GetBool() )
		{
			color32_s clr = { 0, 0, 0, 255 };
			UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
		}
	}

	// Don't modify living players in any way.
	BaseClass::ChangeTeam( iNewTeam, bAuto, true );
}


void CTFPlayer::HandleFadeToBlack( void )
{
	if ( mp_fadetoblack.GetBool() )
	{
		color32_s clr = { 0, 0, 0, 255 };
		UTIL_ScreenFade( this, clr, 0.75f, 0, FFADE_OUT | FFADE_STAYOUT );
	}
}


void CTFPlayer::ChangeTeam( int iTeamNum, bool bAutoTeam /*= false*/, bool bSilent /*= false*/ )
{
	if ( !TFTeamMgr()->IsValidTeam( iTeamNum ) )
	{
		Warning( "CTFPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// If this is our current team, just abort.
	int iOldTeam = GetTeamNumber();
	if ( iTeamNum == iOldTeam )
		return;

	RemoveAllOwnedEntitiesFromWorld( false );
	DropFlag();

	if ( IsVIP() )
	{
		TFGameRules()->OnVIPLeft( this );
	}

	m_iLastTeamNum = iOldTeam;

	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else // Active player.
	{
		if ( iTeamNum == TEAM_SPECTATOR )
		{
			CTF_GameStats.Event_PlayerMovedToSpectators( this );
			CleanupOnDeath( true );

			m_flSpectatorTime = gpGlobals->curtime;
			m_bIsIdle = false;
			StateTransition( TF_STATE_OBSERVER );

			RemoveAllWeapons();
			DestroyViewModels();

			if ( TFGameRules()->IsInArenaQueueMode() && !m_bArenaSpectator )
			{
				TFGameRules()->AddPlayerToQueue( this );
			}

			// Do we have fadetoblack on? (need to fade their screen back in).
			if ( mp_fadetoblack.GetBool() )
			{
				color32_s clr = { 0, 0, 0, 255 };
				UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
			}
		}
		else if ( IsAlive() )
		{
			// Kill player if switching teams while alive.
			CommitSuicide( false, true );
		}
		else if ( iOldTeam < FIRST_GAME_TEAM && iTeamNum >= FIRST_GAME_TEAM )
		{
			SetObserverMode( OBS_MODE_CHASE );
			HandleFadeToBlack();
		}

		// Let any spies disguising as me know that I've changed teams.
		CTFPlayer *pTemp;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTemp && pTemp != this )
			{
				if ( ( pTemp->m_Shared.GetDisguiseTarget() == this ) || // They were disguising as me and I've changed teams.
					( pTemp->m_Shared.GetDisguiseTarget() == pTemp && pTemp->m_Shared.GetDisguiseTeam() == iTeamNum ) ) // They don't have a disguise and I'm joining the team they're disguising as.
				{
					// Choose someone else...
					pTemp->m_Shared.FindDisguiseTarget( true );
				}
			}
		}
	}

	BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent );

	RemoveNemesisRelationships( true );
}


void CTFPlayer::HandleCommand_JoinClass( const char *pClassName )
{
	// In case we don't get the class menu message before the spawn timer
	// comes up, fake that we've closed the menu.
	SetClassMenuOpen( false );

	// Can I change class at all?
	if ( !CanShowClassMenu() )
		return;

	if ( TFGameRules()->InStalemate() )
	{
		if ( IsAlive() && !TFGameRules()->CanChangeClassInStalemate() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#game_stalemate_cant_change_class", UTIL_VarArgs( "%.f", tf_stalematechangeclasstime.GetFloat() ) );
			return;
		}
	}

	int iClass = TF_CLASS_UNDEFINED;

	if ( stricmp( pClassName, "random" ) != 0 )
	{
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++ )
		{
			if ( V_stricmp( pClassName, g_aRawPlayerClassNames[i] ) == 0 )
			{
				iClass = i;
				break;
			}
		}

		if ( iClass == TF_CLASS_UNDEFINED )
		{
			ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs( "Invalid class name \"%s\".", pClassName ) );
			return;
		}

		if ( !TFGameRules()->CanPlayerChooseClass( this, iClass ) )
			return;
	}
	else
	{
		iClass = TF_CLASS_RANDOM;
	}

	bool bCanAutoRespawn = CanInstantlyRespawn();

	// Joining the same class?
	if ( iClass != TF_CLASS_RANDOM && iClass == GetDesiredPlayerClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// were a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.
		if ( m_bAllowInstantSpawn && !IsAlive() )
		{
			ForceRespawn();
		}

		return;
	}

	if ( TFGameRules()->IsInArenaQueueMode() && GetTeamNumber() < FIRST_GAME_TEAM )
	{
		TFGameRules()->AddPlayerToQueue( this );
	}

	SetDesiredPlayerClassIndex( iClass );
	IGameEvent *event = gameeventmanager->CreateEvent( "player_changeclass" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "class", iClass );

		gameeventmanager->FireEvent( event );
	}

	if ( bCanAutoRespawn )
	{
		ForceRespawn();
		return;
	}

	if ( iClass == TF_CLASS_RANDOM )
	{
		if ( IsAlive() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#game_respawn_asrandom" );
		}
		else
		{
			ClientPrint( this, HUD_PRINTTALK, "#game_spawn_asrandom" );
		}
	}
	else
	{
		if ( IsAlive() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#game_respawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
		else
		{
			ClientPrint( this, HUD_PRINTTALK, "#game_spawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
	}

	if ( IsAlive() && GetHudClassAutoKill() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		CommitSuicide( false, true );
	}
}


void CTFPlayer::ShowTeamMenu( bool bShow /*= true*/ )
{
	if ( TFGameRules()->IsFourTeamGame() )
	{
		ShowViewPortPanel( PANEL_FOURTEAMSELECT, bShow );
	}
	else if ( TFGameRules()->IsInArenaQueueMode() )
	{
		ShowViewPortPanel( PANEL_ARENA_TEAM, bShow );
	}
	else
	{
		ShowViewPortPanel( PANEL_TEAM, bShow );
	}
}


void CTFPlayer::ShowClassMenu( bool bShow /*= true*/ )
{
	KeyValues *data = new KeyValues( "data" );
	data->SetInt( "team", GetTeamNumber() );
	ShowViewPortPanel( PANEL_CLASS, bShow, data );
	data->deleteThis();
}


void CTFPlayer::SelectInitialClass( void )
{
	CTFTeam *pTFTeam = GetTFTeam();
	if ( TFGameRules()->IsVIPMode() &&
		pTFTeam->IsEscorting() &&
		pTFTeam->GetVIP() == NULL )
	{
		// If there's no VIP on our team become one.
		BecomeVIP();
	}
	else if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_CLASSES ) )
	{
		SetDesiredPlayerClassIndex( TF_CLASS_RANDOM );
	}
	else
	{
		ShowClassMenu( true );
	}
}


bool CTFPlayer::CanBecomeVIP( void )
{
	// Only if we weren't VIP already in this match.
	return !(m_bWasVIP || AvoidBecomingVIP());
}


void CTFPlayer::BecomeVIP( void )
{
	m_iPreviousClassIndex = GetDesiredPlayerClassIndex();
	SetDesiredPlayerClassIndex( TF_CLASS_CIVILIAN );
	m_bWasVIP = true;
	ShowClassMenu( false );
	SetClassMenuOpen( false );
	
	CTFTeam *pTeam = GetTFTeam();
	if ( pTeam )
	{
		pTeam->SetVIP( this );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "vip_assigned" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "team", GetTeamNumber() );

		gameeventmanager->FireEvent( event );
	}
}


void CTFPlayer::ReturnFromVIP( void )
{
	// Go back to our previous class.
	int iClass = m_iPreviousClassIndex;
	if ( iClass == TF_CLASS_UNDEFINED || iClass == TF_CLASS_CIVILIAN )
	{
		iClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
	}

	SetDesiredPlayerClassIndex( iClass );
}


bool CTFPlayer::CanInstantlyRespawn( void )
{
	int iGameState = TFGameRules()->State_Get();
	if ( iGameState == GR_STATE_TEAM_WIN )
	{
		m_bAllowInstantSpawn = false;
		return false;
	}

	if ( iGameState == GR_STATE_PREROUND )
		return true;

	// We can respawn instantly if:
	//	- We're dead, and we're past the required post-death time
	//	- We're inside a respawn room
	//	- We're in the stalemate grace period
	if ( m_bAllowInstantSpawn )
		return true;

	if ( !IsAlive() )
		return false;

	if ( TFGameRules()->InStalemate() )
		return TFGameRules()->CanChangeClassInStalemate();

	// If we're dead, ignore respawn rooms. Otherwise we'll get instant spawns
	// by spectating someone inside a respawn room.
	return PointInRespawnRoom( this, WorldSpaceCenter(), true );
}


bool CTFPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	m_flLastAction = gpGlobals->curtime;

	if ( FStrEq( pcmd, "addcond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TF_COND_LAST-1 );

				CTFPlayer *pTargetPlayer = this;
				if ( args.ArgC() >= 4 )
				{
					// Find the matching netname.
					CTFPlayer *pPlayer;
					for ( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
						if ( pPlayer )
						{
							if ( V_strstr( pPlayer->GetPlayerName(), args[3] ) )
							{
								pTargetPlayer = pPlayer;
								break;
							}
						}
					}
				}

				if ( args.ArgC() >= 3 )
				{
					float flDuration = atof( args[2] );
					pTargetPlayer->m_Shared.AddCond( (ETFCond)iCond, flDuration );
				}
				else
				{
					pTargetPlayer->m_Shared.AddCond( (ETFCond)iCond );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "removecond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TF_COND_LAST-1 );
				m_Shared.RemoveCond( (ETFCond)iCond );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "burn" ) ) 
	{
		if ( sv_cheats->GetBool() )
		{
			m_Shared.Burn( this );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam( args[1] );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			HandleCommand_JoinTeam( "spectate" );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nomenus" ) )
	{
		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nokill" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoKill( args[1] );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "closedwelcomemenu" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( GetTeamNumber() == TEAM_UNASSIGNED )
			{
				ShowTeamMenu( true );
			}
			else if ( IsPlayerClass( TF_CLASS_UNDEFINED, true ) )
			{
				ShowClassMenu( true );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		if ( ShouldRunRateLimitedCommand( args ) && args.ArgC() >= 2 )
		{
			HandleCommand_JoinClass( args[1] );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "setitempreset" ) )
	{
		if ( args.ArgC() >= 4 )
		{
			HandleCommand_ItemPreset( abs( atoi( args[1] ) ), abs( atoi( args[2] ) ), abs( atoi( args[3] ) ) );
		}
	
		return true;
	}
	else if ( FStrEq( pcmd, "loadoutchanged" ) )
	{
		// Respawn player upon changing loadout but not too often.
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			CheckInstantLoadoutRespawn();
		}

		return true;
	}
	else if ( FStrEq( pcmd, "respawn" ) )
	{
		// Respawn player upon changing loadout but not too often.
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			CheckInstantLoadoutRespawn();
		}

		return true;
	}
	else if ( FStrEq( pcmd, "disguise" ) ) 
	{
		if ( args.ArgC() >= 3 )
		{
			if ( CanDisguise() )
			{
				bool bFourTeams = TFGameRules() && TFGameRules()->IsFourTeamGame();
				int nClass = atoi( args[1] );
				int nTeam = atoi( args[2] );

				// Disguise as enemy team.
				if ( nTeam == -1 )
				{
					do
					{
						nTeam = RandomInt( FIRST_GAME_TEAM, GetNumberOfTeams() - 1 );
					}
					while ( nTeam == GetTeamNumber() || ( bFourTeams && nTeam == m_Shared.GetDisguiseTeam() ) );
				}
				// Disguise as my own team.
				else if ( nTeam == -2 )
				{
					nTeam = GetTeamNumber();
				}
				else
				{
					nTeam += FIRST_GAME_TEAM;
					if( nTeam == TF_TEAM_COUNT )
						nTeam = TF_TEAM_GLOBAL;
				}
				
				// Intercepting the team value and reassigning what gets passed into 'Disguise()'
				// because the team numbers in the client menu don't match the #define values for the teams.
				m_Shared.Disguise( nTeam, nClass );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "lastdisguise" ) )
	{
		// Disguise as our last known disguise. desired disguise will be initted to something sensible.
		if ( CanDisguise() && ( gpGlobals->curtime >= m_Shared.m_flDisguiseCompleteTime ) )
		{
			// Disguise as the previous class, if one exists.
			int nClass = m_Shared.GetDesiredDisguiseClass();

			// If we pass in "random" or whatever then just make it pick a random class.
			if ( args.ArgC() > 1 )
			{
				nClass = TF_CLASS_UNDEFINED;
			}

			int nTeam = TEAM_INVALID;
			if ( nClass != TF_CLASS_UNDEFINED )
			{
				if ( m_Shared.GetLastDisguiseWasEnemyTeam() )
				{
					if ( GetNumberOfTeams() == TF_ORIGINAL_TEAM_COUNT )
					{
						nTeam = ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
					}
					else
					{
						if ( GetTeamNumber() != m_Shared.GetDesiredDisguiseTeam() )
						{
							nTeam = m_Shared.GetDesiredDisguiseTeam();
						}
						// if we were disguised as enemy team in >2 teams mode and switched to them, force spy to get a new random disguise
						else
						{
							nClass = TF_CLASS_UNDEFINED;
						}
					}
				}
				else
				{
					nTeam = GetTeamNumber();
				}
			}

			if ( nClass == TF_CLASS_UNDEFINED )
			{
				// They haven't disguised yet, pick a nice one for them.
				// exclude some undesirable classes.
				// PistonMiner: Made it so it doesnt pick your own team.
				do
				{
					nClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
					nTeam = GetNumberOfTeams() == TF_ORIGINAL_TEAM_COUNT ? RandomInt( FIRST_GAME_TEAM, GetNumberOfTeams() - 1 ) : TF_TEAM_GLOBAL;
				}
				while ( nClass == TF_CLASS_SCOUT || nClass == TF_CLASS_SPY || nTeam == GetTeamNumber() );
			}

			m_Shared.Disguise( nTeam, nClass );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playgesture" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playgesture: Gesture activity or sequence must be specified!\n" );
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlayGesture( args[1] ) )
				{
					Warning( "mp_playgesture: unknown sequence or activity name \"%s\"\n", args[1] );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playanimation" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playanimation: Activity or sequence must be specified!\n" );
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlaySpecificSequence( args[1] ) )
				{
					Warning( "mp_playanimation: Unknown sequence or activity name \"%s\"\n", args[1] );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
		SetClassMenuOpen( true );
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		SetClassMenuOpen( false );
		return true;
	}
	else if ( FStrEq( pcmd, "pda_click" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// Player clicked on the PDA, play attack animation.
			CTFWeaponPDA *pPDA = dynamic_cast<CTFWeaponPDA *>( GetActiveTFWeapon() );
			if ( pPDA && !m_Shared.IsDisguised() )
			{
				DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "taunt" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( !IsAlive() )
				return true;

#ifdef ITEM_TAUNTING
			// Cancel a hold taunt.
			if ( m_bHoldingTaunt )
			{
				EndLongTaunt();
				return true;
			}

			// Check if we want to use a taunt from item schema.
			if ( args.ArgC() >= 2 )
			{
				int iItem = atoi( args[1] );
				if ( iItem != 0 )
				{
					CEconItemView *pItem = GetTFInventory()->GetItem( m_PlayerClass.GetClassIndex(), TF_LOADOUT_SLOT_TAUNT, iItem );
					if ( pItem )
					{
						PlayTauntSceneFromItem( pItem );
					}
				}

				return true;
			}

			CTFPlayer *pPlayer = FindPartnerTauntInitiator();
			if ( pPlayer )
			{
				AcceptTauntWithPartner( pPlayer );
				return true;
			}
#endif

			Taunt();
		}

		return true;
	}
	else if ( FStrEq( pcmd, "build" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iBuilding = 0;
			int iMode = 0;

			if ( args.ArgC() == 2 )
			{
				// Player wants to build something.
				iBuilding = atoi( args[1] );
				iMode = 0;


				// For backwards compatibility.
				if ( iBuilding == 3 )
				{
					iBuilding = OBJ_TELEPORTER;
					iMode = TELEPORTER_TYPE_EXIT;
				}
				if (iBuilding == 4)
				{
					iBuilding = OBJ_ATTACHMENT_SAPPER;
				}
				if (iBuilding == 5)
				{
					iBuilding = OBJ_JUMPPAD;
					iMode = JUMPPAD_TYPE_A;
				}
				if (iBuilding == 6)
				{
					iBuilding = OBJ_JUMPPAD;
					iMode = JUMPPAD_TYPE_B;
				}
				if (iBuilding == 7)
				{
					iBuilding = OBJ_MINIDISPENSER;
				}
				if (iBuilding == 8)
				{
					iBuilding = OBJ_FLAMESENTRY;
				}
				// Yep

				StartBuildingObjectOfType( iBuilding, iMode );
			}
			else if ( args.ArgC() == 3 )
			{
				// Player wants to build something.
				iBuilding = atoi( args[1] );
				iMode = atoi(args[2]);
				
				StartBuildingObjectOfType( iBuilding, iMode );
			}
			else
			{
				Warning( "Usage: build <building> <mode>\n" );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "destroy" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( IsPlayerClass( TF_CLASS_ENGINEER, true ) )
			{
				if ( args.ArgC() == 2 )
				{
					// Player wants to destroy something.
					int iBuilding = atoi( args[1] );

					// For backwards compatibility.
					if (iBuilding == 3)
					{
						DetonateOwnedObjectsOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT);
					}
					if (iBuilding == 4)
					{
						DetonateOwnedObjectsOfType(OBJ_ATTACHMENT_SAPPER);
					}
					if (iBuilding == 5)
					{
						DetonateOwnedObjectsOfType(OBJ_JUMPPAD, JUMPPAD_TYPE_A);
					}
					if (iBuilding == 6)
					{
						DetonateOwnedObjectsOfType(OBJ_JUMPPAD, JUMPPAD_TYPE_B);
					}
					if (iBuilding == 7)
					{
						DetonateOwnedObjectsOfType(OBJ_MINIDISPENSER);
					}
					if (iBuilding == 8)
					{
						DetonateOwnedObjectsOfType(OBJ_FLAMESENTRY);
					}
				}
				else if ( args.ArgC() == 3 )
				{
					DetonateOwnedObjectsOfType(atoi(args[1]), atoi(args[2]));
				}
				else
				{
					Warning( "Usage: destroy <building> <mode>\n" );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "extendfreeze" ) )
	{
		if ( !IsVIP() )
		{
			if ( ShouldRunRateLimitedCommand( args ) )
			{
				m_flDeathTime += 2.0f;
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "show_motd" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			KeyValues *data = new KeyValues( "data" );

			// Info panel title.
			if ( TFGameRules()->IsBirthday() ) 
			{
				data->SetString( "title", "#TF_Welcome_birthday" );
			}
			else if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
			{
				data->SetString( "title", "#TF_Welcome_halloween" );
			}
			else if ( TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			{
				data->SetString("title", "#TF_Welcome_christmas");
			}
			else 
			{
				data->SetString( "title", "#TF_Welcome" );
			}

			data->SetString( "type", "1" );				// Show userdata from stringtable entry.
			data->SetString( "msg",	"motd" );			// Use this stringtable entry.
			data->SetString( "cmd", "mapinfo" );		// Exec this command if panel closed.

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
	}
	else if ( FStrEq( pcmd, "powerplay_on" ) )
	{
		if ( !PlayerIsDeveloper() )
		{
			Msg( "You do not have permission to use Powerplay.\n" );
			return true;
		}
		else 
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0, c = GetTeam()->GetNumPlayers(); i < c; i++ )
				{
					CTFPlayer *pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer(i) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( true );
					}
				}
				return true;
			}
			else if ( SetPowerplayEnabled( true ) )
				return true;
		}
	}
	else if ( FStrEq( pcmd, "powerplay_off" ) )
	{
		if ( !PlayerIsDeveloper() )
		{
			Msg( "You do not have permission to use Powerplay.\n" );
			return true;
		}
		else
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0, c = GetTeam()->GetNumPlayers(); i < c; i++ )
				{
					CTFPlayer *pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer( i ) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( false );
					}
				}
				return true;
			}
			else if ( SetPowerplayEnabled( false ) )
				return true;
		}
	}

	return BaseClass::ClientCommand( args );
}

enum
{
	TF_CUSTOM_SUICIDE_TYPE_DISINTEGRATE,
	TF_CUSTOM_SUICIDE_TYPE_BOIOING,
	TF_CUSTOM_SUICIDE_TYPE_STOMP
};

//-----------------------------------------------------------------------------
// Purpose: Debug concommand to set the player on fire
//-----------------------------------------------------------------------------
void suicide_helper_tf( const CCommand &args, int iType )
{
	CTFPlayer *pPlayer = NULL;
	if ( args.ArgC() > 1 && sv_cheats->GetBool() )
	{
		// Find the matching netname
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pTmpPlayer = ToTFPlayer( UTIL_PlayerByIndex(i) );
			if ( pTmpPlayer )
			{
				if ( Q_strstr( pTmpPlayer->GetPlayerName(), args[1] ) )
				{
					pPlayer = pTmpPlayer;
					break;
				}
			}
		}
	}
	else
	{
		pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	}
	
	if( !pPlayer )
		return;

	if( !pPlayer->IsAlive() )
		return;	

	switch( iType )
	{
		case TF_CUSTOM_SUICIDE_TYPE_DISINTEGRATE:
			pPlayer->CommitCustomSuicide( false, false, TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE );
			break;
		case TF_CUSTOM_SUICIDE_TYPE_BOIOING:
			pPlayer->CommitCustomSuicide( false, false, TF_DMG_CUSTOM_SUICIDE_BOIOING, vec3_origin, true ); // i kinda just dont wanna whack the vtable again
			break;
		case TF_CUSTOM_SUICIDE_TYPE_STOMP:
			pPlayer->CommitCustomSuicide( false, false, TF_DMG_CUSTOM_SUICIDE_STOMP, vec3_origin, true );
			break;
	}		

	return;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( disintegrate, "Disintegrates you" )
{
	suicide_helper_tf( args, TF_CUSTOM_SUICIDE_TYPE_DISINTEGRATE );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( boioing, "Ascend to heaven" )
{
	suicide_helper_tf( args, TF_CUSTOM_SUICIDE_TYPE_BOIOING );
}

CON_COMMAND( stomp, "Descend to the ground" )
{
	suicide_helper_tf( args, TF_CUSTOM_SUICIDE_TYPE_STOMP );
}


void CTFPlayer::SetClassMenuOpen( bool bOpen )
{
	m_bIsClassMenuOpen = bOpen;
}


bool CTFPlayer::IsClassMenuOpen( void )
{
	return m_bIsClassMenuOpen;
}


bool CTFPlayer::PlayGesture( const char *pGestureName )
{
	Activity nActivity = (Activity)LookupActivity( pGestureName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pGestureName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


bool CTFPlayer::PlaySpecificSequence( const char *pAnimationName )
{
	Activity nActivity = (Activity)LookupActivity( pAnimationName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pAnimationName );
	if ( nSequence != -1 )
	{
#if TF_PLAYER_SCRIPED_SEQUENCES
		m_Shared.AddCond( TF_COND_TAUNTING );
		m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + SequenceDuration( nSequence ) + 0.2f;
		SetAbsAngles( QAngle( 0, EyeAngles()[YAW], 0 ) );
#endif

		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


bool CTFPlayer::CanDisguise( void )
{
	if ( !IsAlive() )
		return false;

	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
		return false;

	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	int iNoDisguiseAttribute = 0;
	CALL_ATTRIB_HOOK_INT(iNoDisguiseAttribute, set_cannot_disguise);
	if (iNoDisguiseAttribute)
		return false;

	/*if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		//HintMessage( HINT_CANNOT_DISGUISE_WITH_FLAG );
		CSingleUserRecipientFilter filter( this );
		TFGameRules()->SendHudNotification( filter, "#Hint_Cannot_Disguise_With_Flag", "ico_notify_flag_moving", GetTeamNumber() );
		return false;
	}*/

	return true;
}


void CTFPlayer::DetonateOwnedObjectsOfType( int iType, int iMode /*= 0*/, bool bIgnoreSapperState /*= false*/ )
{
	CBaseObject *pObject = GetObjectOfType( iType, iMode );
	if ( !pObject )
		return;

	// Don't allow spies to destroy their own sappers.
	if ( !bIgnoreSapperState && pObject->HasSapper() )
		return;

	int iUserID = GetUserID();
	IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
	if ( event )
	{
		event->SetInt( "userid", iUserID );
		event->SetInt( "objecttype", iType );
		event->SetInt( "index", pObject->entindex() );
		gameeventmanager->FireEvent( event );
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DETONATED_OBJECT, pObject->GetResponseRulesModifier() );
	pObject->DetonateObject();

	const CObjectInfo *pInfo = GetObjectInfo( iType );
	if ( pInfo )
	{
		CTeam *pTeam = GetTeam();
		const char *pszPlayerName = GetPlayerName(), *pszNetworkIDString = GetNetworkIDString(), *pszTeamName = pTeam->GetName();
		Vector vecAbsOrigin = GetAbsOrigin();
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (weapon \"%s\") (objectowner \"%s<%i><%s><%s>\") (attacker_position \"%d %d %d\")\n",
			pszPlayerName,
			iUserID,
			pszNetworkIDString,
			pszTeamName,
			pInfo->m_pObjectName,
			"pda_engineer",
			pszPlayerName,
			iUserID,
			pszNetworkIDString,
			pszTeamName,
			(int)vecAbsOrigin.x,
			(int)vecAbsOrigin.y,
			(int)vecAbsOrigin.z );
	}
}


void CTFPlayer::StartBuildingObjectOfType( int iType, int iMode /*= 0*/ )
{
	// Remote PDA.
	CBaseObject *pBuilding = GetObjectOfType( iType, iMode );
	if ( pBuilding && pBuilding->InRemoteConstruction() )
	{
		pBuilding->SetRemoteConstruction( false );
		return;
	}

	// We're stuck with what we've got in our hands.
	if ( m_Shared.IsCarryingObject() )
		return;

	// Early out if we can't build this type of object.
	if ( CanBuild( iType, iMode ) != CB_CAN_BUILD )
		return;

	CTFWeaponBuilder *pBuilder = static_cast<CTFWeaponBuilder *>( GetEntityForLoadoutSlot( TF_LOADOUT_SLOT_BUILDING ) );
	if ( pBuilder )
	{
		pBuilder->SetSubType( iType );
		pBuilder->SetObjectMode( iMode );

		if ( GetActiveWeapon() == pBuilder )
		{
			// Redeploy.
			pBuilder->Deploy();
		}
		else
		{
			// Try to switch to this weapon.
			Weapon_Switch( pBuilder );
		}
	}
}


float CTFPlayer::GetObjectBuildSpeedMultiplier( int iObjectType, bool bIsRedeploy ) const
{
	// Base Build-Rate.
	float flBuildRate = 1.0f;
	bool bGunMettle = tf2c_building_gun_mettle.GetBool();

	switch ( iObjectType )
	{
		case OBJ_SENTRYGUN:
		case OBJ_FLAMESENTRY:
			CALL_ATTRIB_HOOK_FLOAT( flBuildRate, sentry_build_rate_multiplier );
			if ( bGunMettle )
				flBuildRate += bIsRedeploy ? 2.0f : 0.0f;
			else
				flBuildRate *= bIsRedeploy ? 2.0f : 1.0f;
			break;

		case OBJ_JUMPPAD:
			CALL_ATTRIB_HOOK_FLOAT(flBuildRate, jumppad_build_rate_multiplier);
			if (bGunMettle)
				flBuildRate += bIsRedeploy ? 3.0f : 0.0f;
			else
				flBuildRate *= bIsRedeploy ? 2.0f : 1.0f;
			break;

		case OBJ_TELEPORTER:
			CALL_ATTRIB_HOOK_FLOAT( flBuildRate, teleporter_build_rate_multiplier );
			if ( bGunMettle )
				flBuildRate += bIsRedeploy ? 3.0f : 0.0f;
			else
				flBuildRate *= bIsRedeploy ? 2.0f : 1.0f;
			break;

		case OBJ_DISPENSER:
		case OBJ_MINIDISPENSER:
			CALL_ATTRIB_HOOK_FLOAT( flBuildRate, dispenser_build_rate_multiplier );
			if ( bGunMettle )
				flBuildRate += bIsRedeploy ? 3.0f : 0.0f;
			else
				flBuildRate *= bIsRedeploy ? 2.0f : 1.0f;
			break;
	}

	// Final Result.
	return flBuildRate - ( bGunMettle ? 1.0f : 0.0f );
}

void CTFPlayer::OnSapperStarted( float flStartTime )
{
	if ( m_iSapState == SAPPING_NONE && m_flSapTime == 0.0f )
	{
		m_flSapTime = flStartTime;
		m_bSapping = true;
		m_iSapState = SAPPING_PLACED;
	}
}
void CTFPlayer::OnSapperFinished( float flStartTime )
{
	if ( m_iSapState == SAPPING_NONE && flStartTime == m_flSapTime )
	{
		m_bSapping = false;
		m_flSapTime = 0;
		m_iSapState = SAPPING_DONE;
	}
}
void CTFPlayer::ResetSappingState( void )
{
	m_iSapState = SAPPING_NONE;
	m_bSapping = false;
	m_flSapTime = 0;
}

void CTFPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage != DAMAGE_YES )
		return;

	// Let the weapon handle special behaviour for hitting allies.
	if ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
	{
		CTFWeaponBaseGun *pGun = dynamic_cast<CTFWeaponBaseGun *>(info.GetWeapon());
		if ( pGun )
			pGun->HitAlly( this );
	}		

	// Prevent team damage here so blood doesn't appear.
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
		return;

	// Save this bone for the ragdoll.
	m_nForceBone = ptr->physicsbone;

	SetLastHitGroup( ptr->hitgroup );

	// Ignore hitboxes for all weapons except the sniper rifle.
	CTakeDamageInfo info_modified = info;
	//if ( info_modified.GetDamageType() & DMG_USE_HITLOCATIONS )
	//{
	switch ( ptr->hitgroup )
	{
		case HITGROUP_HEAD:
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info_modified.GetWeapon() );
			if ( pWeapon && pWeapon->CanHeadshot() )
			{
				int iHeadshotIsMinicrit = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iHeadshotIsMinicrit, headshot_is_minicrit);
				if (iHeadshotIsMinicrit)
				{
					auto pTFAttacker = ToTFPlayer(info.GetAttacker());
					if (pTFAttacker)
						pTFAttacker->SetNextAttackMinicrit(true);
				}
				else
				{
					info_modified.AddDamageType(DMG_CRITICAL);
				}
				info_modified.SetDamageCustom( TF_DMG_CUSTOM_HEADSHOT );
			}

			break;
		}
		default:
			break;
	}
	//}

	// No blood splatters while disguised and hurt by our enemy, our man has supreme discipline.
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( m_Shared.IsDisguised() && ( pAttacker && m_Shared.DisguiseFoolsTeam( pAttacker->GetTeamNumber() ) ) )
	{
		// No impact effects.
	}
	else if ( m_Shared.IsInvulnerable() )
	{ 
		// Make bullet impacts.
		g_pEffects->Ricochet( ptr->endpos - ( vecDir * 8 ), -vecDir );
	}
	else
	{	
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		CDisablePredictionFiltering disabler;

		// This does smaller splotches on the guy and splats blood on the world.
		TraceBleed( info_modified.GetDamage(), vecDir, ptr, info_modified.GetDamageType() );
	}

	AddMultiDamage( info_modified, this );
}


int CTFPlayer::TakeHealth( float flAmount, int bitsDamageType, CTFPlayer *pHealer /*= NULL*/, bool bCritHealLogic /*= false*/ )
{
	int iResult = false;

	int iCritHealsDisallowed = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( this, iCritHealsDisallowed, crit_heals_disabled );

	// Apply crit-heal logic if desired
	if ( bCritHealLogic && iCritHealsDisallowed < 1)
	{
		float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10.0f, 15.0f, 1.0f, 3.0f );
		flAmount *= flScale;
	}

	if ( m_Shared.InCond( TF_COND_TAKEBONUSHEALING ) )
		flAmount *= TF_COND_TAKEBONUSHEALING_MULTIPLIER;

	if (pHealer) {

		if (pHealer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC)
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( this, flAmount, mult_healing_from_medics ); // Not sure if this is the best way to acquire this data, copied from shared player Heal
		}

		if ( pHealer->GetMedigun() )
		{
			CTFWeaponBase *pMedigun = pHealer->GetMedigun()->GetWeapon();
			if ( pMedigun ) {
				// Overheal build rate bonuses/penalties:
				float flOverHealRate = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pMedigun, flOverHealRate, overheal_fill_rate );

				float flUnderHeal = min( max( GetMaxHealth() - GetHealth(), 0 ), flAmount );
				float flOverHeal = flAmount - flUnderHeal;
				flAmount = flUnderHeal + (flOverHeal*flOverHealRate);
			}
		}
	}

	// if we are already going to overheal while ignoring max overheal cap, we shouldn't accidentally limit it with max overheal cap
	if (!(bitsDamageType & (HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP)))
	{
		int iEverythingOverheals = 0;
		CALL_ATTRIB_HOOK_INT(iEverythingOverheals, everything_can_overheal_wearer);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(GetActiveTFWeapon(), iEverythingOverheals, everything_can_overheal_active);
		if (iEverythingOverheals > 0)
		{
			bitsDamageType |= (HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP);
		}
	}

	// If the bit's set, add over the max health.
	if ( bitsDamageType & HEAL_IGNORE_MAXHEALTH )
	{
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~( bitsDamageType & ~iTimeBasedDamage );

		if ( bitsDamageType & HEAL_MAXBUFFCAP )
		{
			float flOverhealMultiplier = 1.0f;

			if ( pHealer )
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pHealer, flOverhealMultiplier, mult_medigun_overheal_amount );

			float flMaxBuffedHealth = GetMaxHealth() + ((m_Shared.GetMaxBuffedHealth() - GetMaxHealth())*flOverhealMultiplier);

			if ( flAmount > flMaxBuffedHealth - GetHealth() )
			{
				flAmount = flMaxBuffedHealth - GetHealth();
			}
		}

		if ( flAmount <= 0 )
		{
			iResult = 0;
		}
		else
		{
			m_iHealth += flAmount;
			iResult = (int)flAmount;
		}
	}
	else
	{
		float flHealthToAdd = flAmount;
		int iMaxHealth = GetMaxHealth();
		
		// Don't want to add more than we're allowed to have.
		if ( flHealthToAdd > iMaxHealth - GetHealth() )
		{
			flHealthToAdd = iMaxHealth - GetHealth();
		}

		if ( flHealthToAdd <= 0 )
		{
			iResult = 0;
		}
		else
		{
			if ( !m_takedamage )
				return 0;

			// Modified from base code.
			int bitsDmgTimeBased = g_pGameRules->Damage_GetTimeBased();
			m_bitsDamageType &= ~( bitsDamageType & ~bitsDmgTimeBased );

			if ( !edict() || m_takedamage < DAMAGE_YES )
				return 0;

			// Healing.
			if ( m_iHealth >= iMaxHealth )
				return 0;

			int iOldHealth = GetHealth();

			m_iHealth += flAmount;
			if ( m_iHealth > iMaxHealth )
			{
				m_iHealth = iMaxHealth;
			}

			iResult = m_iHealth - iOldHealth;
		}
	}


	// This needs to be looked into. Without the amendments Hogyn made to TakeHealth, this below 
	// portion doesn't actually seem to do anything. It doesn't affect stats or notify anyone.
	if ( ( bitsDamageType & HEAL_NOTIFY ) && iResult > 0 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iResult );
			event->SetInt( "entindex", entindex() );

			gameeventmanager->FireEvent( event );
		}
	}

	m_Shared.RecalculateChargeEffects();

	// This is dumb
	//if (pHealer) {
	//	// Attribute the healing by adding a 1-frame healer.
	//	m_Shared.AddBurstHealer( pHealer, flAmount );
	//}

	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: The same as the above function, but for disguise spies.
//-----------------------------------------------------------------------------
int CTFPlayer::TakeDisguiseHealth( float flHealth, int bitsDamageType )
{
	int iResult = false;

	// If the bit's set, add over the max health.
	if ( bitsDamageType & HEAL_IGNORE_MAXHEALTH )
	{
		if ( bitsDamageType & HEAL_MAXBUFFCAP )
		{
			if ( flHealth > m_Shared.GetDisguiseMaxBuffedHealth() - m_Shared.GetDisguiseHealth() )
			{
				flHealth = m_Shared.GetDisguiseMaxBuffedHealth() - m_Shared.GetDisguiseHealth();
			}
		}

		if ( flHealth <= 0 )
		{
			iResult = 0;
		}
		else
		{
			m_Shared.SetDisguiseHealth( m_Shared.GetDisguiseHealth() + flHealth );
			iResult = (int)flHealth;
		}
	}
	else
	{
		float flHealthToAdd = flHealth;
		int iMaxHealth = m_Shared.GetDisguiseMaxHealth();
		
		// Don't want to add more than we're allowed to have.
		if ( flHealthToAdd > iMaxHealth - m_Shared.GetDisguiseHealth() )
		{
			flHealthToAdd = iMaxHealth - m_Shared.GetDisguiseHealth();
		}

		if ( flHealthToAdd <= 0 )
		{
			iResult = 0;
		}
		else
		{
			if ( !m_takedamage )
				return 0;

			if ( !edict() || m_takedamage < DAMAGE_YES )
				return 0;

			// Healing.
			if ( m_Shared.GetDisguiseHealth() >= iMaxHealth )
				return 0;

			int iOldHealth = m_Shared.GetDisguiseHealth();

			m_Shared.SetDisguiseHealth( m_Shared.GetDisguiseHealth() + flHealth );
			if ( m_Shared.GetDisguiseHealth() > iMaxHealth )
			{
				m_Shared.SetDisguiseHealth( iMaxHealth );
			}

			iResult = m_Shared.GetDisguiseHealth() - iOldHealth;
		}
	}

	if ( ( bitsDamageType & HEAL_NOTIFY ) && iResult > 0 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iResult );
			event->SetInt( "entindex", entindex() );

			gameeventmanager->FireEvent( event );
		}
	}

	return iResult;
}


void CTFPlayer::DropFlag( void )
{
	CCaptureFlag *pFlag = GetTheFlag();
	if ( pFlag )
	{
		pFlag->Drop( this, true, true );
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
			event->SetInt( "team", pFlag->GetTeamNumber() );
			event->SetInt( "priority", 8 );

			gameeventmanager->FireEvent( event );
		}
	}
}


CTFPlayer *CTFPlayer::TeamFortress_GetDisguiseTarget( int nTeam, int nClass, bool bFindOther )
{
	if ( (nTeam < FIRST_GAME_TEAM || nTeam > TF_TEAM_YELLOW) && nTeam != TF_TEAM_GLOBAL )
	{
		// Invalid disguise.
		return NULL;
	}

	CUtlVector<int> potentialTargets;

	// Find another taget if asked, otherwise don't redisguise self as this person.
	CBaseEntity *pLastTarget = bFindOther ? m_Shared.GetDisguiseTarget() : NULL;
	
	// Find a player on the team the spy is disguised as to pretend to be.
	CTFPlayer *pPlayer = NULL;

	// Loop through players and attempt to find a player as the team/class we're disguising as.
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer != pLastTarget ) )
		{
			// First, try to find a player with the same color AND skin.
			if ( pPlayer->IsAlive() && (pPlayer->GetTeamNumber() == nTeam || nTeam == TF_TEAM_GLOBAL) && pPlayer->GetPlayerClass()->GetClassIndex() == nClass )
			{
				potentialTargets.AddToHead( i );
			}
		}
	}

	// Do we have any potential targets in the list?
	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// We didn't find someone with the class, so just find someone with the same team color.
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer->GetTeamNumber() == nTeam ) )
		{
			potentialTargets.AddToHead( i );
		}
	}

	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// We didn't find anyone... it's just me. :(
	return this;
}


static float DamageForce( const Vector &size, float damage, float scale )
{
	float flForce = damage * ( ( 48.0f * 48.0f * 82.0f )  / ( size.x * size.y * size.z ) ) * scale;
	if ( flForce > 1000.0f )
	{
		flForce = 1000.0f;
	}

	return flForce;
}

extern ConVar mp_developer;
ConVar tf_debug_damage( "tf_debug_damage", "0", FCVAR_CHEAT );


int CTFPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( GetFlags() & FL_GODMODE )
		return 0;

	if ( IsInCommentaryMode() )
		return 0;

	bool bBuddha = !!(m_debugOverlays & OVERLAY_BUDDHA_MODE) || inputInfo.GetDamageCustom() == TF_DMG_CUSTOM_BUDDHA;	// !!! foxysen detonator

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetInt() > 1 && !IsBot() )
	{
		bBuddha = true;
	}
#endif // _DEBUG || STAGING_ONLY

	CTakeDamageInfo info = inputInfo;
	if ( bBuddha && ( GetHealth() - info.GetDamage() ) <= 0 )
	{
		m_iHealth = 1;
		return 0;
	}

	if ( !IsAlive() )
		return 0;

	// Early out if there's no damage.
	if ( !info.GetDamage() )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

	bool bDebug = tf_debug_damage.GetBool();

	// Make sure the player can take damage from the attacking entity.
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player can't take damage from that attacker!\n" );
		}

		return 0;
	}

	int m_iHealthBefore = GetHealth();

	bool bIsSoldierRocketJumping = ( IsPlayerClass( TF_CLASS_SOLDIER ) && pAttacker == this && !( GetFlags() & FL_ONGROUND ) && !( GetFlags() & FL_INWATER ) ) && ( inputInfo.GetDamageType() & DMG_BLAST );
	bool bIsDemomanPipeJumping = ( IsPlayerClass( TF_CLASS_DEMOMAN ) && pAttacker == this && !( GetFlags() & FL_ONGROUND ) && !( GetFlags() & FL_INWATER ) ) && ( inputInfo.GetDamageType() & DMG_BLAST );

	
	if ( bDebug )
	{
		Warning( "%s taking damage from %s, via %s. Damage: %.2f\n", GetDebugName(), pInflictor ? pInflictor->GetDebugName() : "Unknown Inflictor", pAttacker ? pAttacker->GetDebugName() : "Unknown Attacker", info.GetDamage() );
	}

// wrong! Anchor bomb
	// nullify earthquake damage
//	if (info.GetDamageCustom() == TF_DMG_EARTHQUAKE && IsAirborne())
//		info.SetDamage(0.0f);

	// Damage may not come from a weapon (ie: Bosses, etc),
	// but the existing code below already checks for a NULL pWeapon, anyways.
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( inputInfo.GetWeapon() );

	if (info.GetDamageType() & DMG_FALL && info.GetDamageCustom() == TF_DMG_FALL_DAMAGE && info.GetDamage() > 0.0f)
	{
		bool bHitEnemy = false;
		bool bDoImpactEffect = false;

		//Earthquake explosion for Soldier Mace!
		float flEarthquakeRadius = 0.0f;
		float flEarthquakeDamageConst = 0.0f;
		float flEarthquakeDamageMult = 0.0f;
		string_t strArgsEqarthquake = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING(strArgsEqarthquake, earthquake_attack_wearer);
		if (strArgsEqarthquake != NULL_STRING)
		{
			float args[3];
#ifdef GAME_DLL
			UTIL_StringToFloatArray(args, 3, strArgsEqarthquake.ToCStr());
#else
			UTIL_StringToFloatArray(args, 3, strArgsEqarthquake);
#endif

			flEarthquakeRadius += args[0];
			flEarthquakeDamageConst += args[1];
			flEarthquakeDamageMult += args[2];
		}
		string_t strArgsEqarthquakeActive = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING_ON_OTHER(GetActiveTFWeapon(), strArgsEqarthquakeActive, earthquake_attack_active);
		if (strArgsEqarthquakeActive != NULL_STRING)
		{
			float args[3];
#ifdef GAME_DLL
			UTIL_StringToFloatArray(args, 3, strArgsEqarthquakeActive.ToCStr());
#else
			UTIL_StringToFloatArray(args, 3, strArgsEqarthquakeActive);
#endif

			flEarthquakeRadius += args[0];
			flEarthquakeDamageConst += args[1];
			flEarthquakeDamageMult += args[2];
		}
#ifdef TF2C_BETA
		float flAnchorMult = 0.0f;

		CTFWeaponBase* pActiveWeapon = GetActiveTFWeapon();
		if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_ANCHOR )
		{
			CTFAnchor* pAnchor = static_cast<CTFAnchor*>(pActiveWeapon);
			if (pAnchor)
			{
				pAnchor->StopSound("Weapon_Anchor.Fall");
				flAnchorMult = pAnchor->GetProgress();
				flEarthquakeRadius += 192.0f * flAnchorMult;
				flEarthquakeDamageMult += 4.0f * flAnchorMult;
			}
		}
#endif
		if (flEarthquakeRadius > 0.0f)
		{
			float flStompDamage = flEarthquakeDamageConst + info.GetDamage() * flEarthquakeDamageMult;
			CTFRadiusDamageInfo radiusInfo;
			radiusInfo.info.Set(this, this, vec3_origin, GetAbsOrigin(), flStompDamage, DMG_GENERIC | DMG_HALF_FALLOFF, TF_DMG_EARTHQUAKE);
			radiusInfo.m_vecSrc = GetAbsOrigin();
			radiusInfo.m_flRadius = flEarthquakeRadius;
			radiusInfo.m_pEntityIgnore = this;

			TFGameRules()->RadiusDamage(radiusInfo);

			bDoImpactEffect = true;

			// Spawn earthquake particles
			Vector vecQuakeOrigin = radiusInfo.m_vecSrc + Vector( 0, 0, 128 );

			// Find suitable positions on the floor
			for ( int i = 0; i < 24; i++ )
			{
				Vector vecQuakeDir = UTIL_YawToVector( 360 / 24 * i + random->RandomFloat( -7.5, 7.5 ) );
				Vector vecQuakeRandomOffset = ( RandomVector( -4.0f, 4.0f ) + vecQuakeOrigin ) + ( vecQuakeDir * flEarthquakeRadius * RandomFloat( 0.5f, 1.0f ) );

				trace_t tr;
				UTIL_TraceLine( vecQuakeRandomOffset, vecQuakeRandomOffset + Vector( 0, 0, -256 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
				//NDebugOverlay::Line( vecQuakeRandomOffset, vecQuakeRandomOffset + Vector( 0, 0, -256 ), NB_RGB_GRAY, false, 6.0f );
				
				if ( tr.fraction < 1.0f )
				{
					//NDebugOverlay::Box( tr.endpos, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), NB_RGBA_GREEN, 6.0f );

					QAngle vecQuakeAngle = vec3_angle;

					// Differentiate ground surfaces
					surfacedata_t* psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );

					const char* pszEffectName;
					switch ( psurf ? psurf->game.material : 'D' )
					{
					case 91: //CHAR_TEX_TFSNOW
						pszEffectName = EARTHQUAKE_PARTICLE_NAME_SNOW;
						break;
					default:
						pszEffectName = EARTHQUAKE_PARTICLE_NAME;
						break;
					}

					// Align to surface?
					//VectorAngles( tr.plane.normal, vecQuakeAngle );
					//DispatchParticleEffect( pszEffect, tr.endpos, vecQuakeAngle );
					CPVSFilter filter( vecQuakeOrigin );
					Vector vecQuakeDrawPos = tr.endpos + ( tr.plane.normal * 2 );
					TE_TFParticleEffect( filter, 0.0f, pszEffectName, vecQuakeDrawPos, vecQuakeAngle );
				}
				else
				{
					//NDebugOverlay::Box( tr.endpos, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), NB_RGBA_RED, 6.0f );
				}
			}
		}

		// Are we transferring falling damage to someone else?
		int iHeadStomp = 0;
		CALL_ATTRIB_HOOK_INT( iHeadStomp, boots_falling_stomp );
		int iLaunchedNoStomp = 0;
		CALL_ATTRIB_HOOK_INT( iLaunchedNoStomp, mod_launched_cond_no_stomp );
		bool bDamagingStomp = ( iHeadStomp || ( m_Shared.InCond( TF_COND_LAUNCHED ) && !iLaunchedNoStomp ) );
		if ( bDamagingStomp &&
			 GetGroundEntity() &&
			 GetGroundEntity()->IsPlayer() )
		{
			// Did we land on a guy from the enemy team?
			CTFPlayer *pOther = ToTFPlayer( GetGroundEntity() );
			if ( pOther && pOther->GetTeamNumber() != GetTeamNumber() )
			{
				float flStompDamage = 0.0f;
				CBaseEntity *pKillingWeapon = nullptr;
				ETFDmgCustom eCustomDamageType = TF_DMG_CUSTOM_NONE;

				CTFPlayer* pTFJumppadCondProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_LAUNCHED));
				if (iHeadStomp)
				{
					flStompDamage += 10.0f + info.GetDamage() * 3.0f;
					pKillingWeapon = GetWearableForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY);
					eCustomDamageType = TF_DMG_CUSTOM_BOOTS_STOMP;
				}
				if (m_Shared.InCond(TF_COND_LAUNCHED))
				{
					float flJumppadStompDamage = 10.0f + info.GetDamage() * 3.0f * JUMPPAD_STOMP_DAMAGE_SCALE;
					flStompDamage += flJumppadStompDamage;
					pKillingWeapon = nullptr;
					eCustomDamageType = TF_DMG_CUSTOM_JUMPPAD_STOMP;

					if (pTFJumppadCondProvider && !IsEnemy(pTFJumppadCondProvider) && this != pTFJumppadCondProvider)
					{
						CTF_GameStats.Event_PlayerDamageAssist(pTFJumppadCondProvider, flJumppadStompDamage);
					}
				}

				CTakeDamageInfo infoInner( this, this, pKillingWeapon, flStompDamage, DMG_FALL, eCustomDamageType );
				pOther->TakeDamage( infoInner );


				if (pTFJumppadCondProvider && !IsEnemy(pTFJumppadCondProvider) && this != pTFJumppadCondProvider)
				{
					CTF_GameStats.Event_PlayerBlockedDamage(pTFJumppadCondProvider, info.GetDamage() / 3.0f);
				}
				info.SetDamage(0.0f);
				bDoImpactEffect = true;

				bHitEnemy = true;
			}
		}

		if (bDoImpactEffect)
		{
#ifdef TF2C_BETA
			EmitSound("Weapon_Anchor.ImpactBoom");
			if (flAnchorMult >= 1.0f)
			{
				EmitSound("Weapon_Anchor.Impact");
			}
#else
			EmitSound("Weapon_Mantreads.Impact");
#endif		
			UTIL_ScreenShake(GetAbsOrigin(), 15.0f, 150.0, 1.0f, 500, SHAKE_START);
			//UTIL_ScreenShake(pOther->WorldSpaceCenter(), 15.0f, 150.0, 1.0f, 500, SHAKE_START);
		}

		bool bLandedOnValidJumppad = false;
		CTFPlayer* pTFLandedJumppadOwner = nullptr;
		if (GetGroundEntity() && GetGroundEntity()->IsBaseObject())
		{
			CBaseObject* pBaseObject = static_cast<CBaseObject*>(GetGroundEntity());
			if (pBaseObject->GetType() == OBJ_JUMPPAD &&
				(tf2c_building_sharing.GetBool() || IsPlayerClass(TF_CLASS_SPY, true) || !IsEnemy(pBaseObject)))
			{
				bLandedOnValidJumppad = (pBaseObject->GetType() == OBJ_JUMPPAD);
				if (bLandedOnValidJumppad)
				{
					pTFLandedJumppadOwner = ToTFPlayer(pBaseObject->GetOwnerEntity());
				}
			}
		}

		if (m_Shared.InCond(TF_COND_LAUNCHED) || bLandedOnValidJumppad)
		{
			CTFPlayer* pTFFallDamagePreventer = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_LAUNCHED));
			if (!pTFFallDamagePreventer && bLandedOnValidJumppad)
			{
				pTFFallDamagePreventer = pTFLandedJumppadOwner;
			}
			if (pTFFallDamagePreventer && !IsEnemy(pTFFallDamagePreventer) && this != pTFFallDamagePreventer)
			{
				CTF_GameStats.Event_PlayerBlockedDamage(pTFFallDamagePreventer, m_Shared.InCond(TF_COND_LAUNCHED) ? info.GetDamage()/3.0f : info.GetDamage()); // if we used jumppad then count it as potential damage as we may not have gotten it without jumping in the first place
			}
			info.SetDamage(0.0f);
			m_Shared.RemoveCond(TF_COND_LAUNCHED);
			if ( !bHitEnemy )
			{
				EmitSound( "TFPlayer.SafeLanding" );

				int iTeam = m_Shared.IsDisguised() ? m_Shared.GetDisguiseTeam() : GetTeamNumber();
				if ( iTeam != TF_TEAM_GLOBAL ) // hack. doesnt display particle for global disguises for now. - azzy
				{
					// This particle doesn't seem to play...?
					const char* pszEffect = ConstructTeamParticle( "jumppad_jump_puff_smoke_%s", iTeam );
					DispatchParticleEffect( pszEffect, GetAbsOrigin(), vec3_angle );
				}
			}
		}
	}

	// Keep track of amount of damage last sustained.
	m_lastDamageAmount = info.GetDamage();
	m_LastDamageType = info.GetDamageType();

	if ( bIsSoldierRocketJumping || bIsDemomanPipeJumping )
	{
		int nJumpType = 0;

		// If this is our own rocket, scale down the damage if we're rocket jumping.
		if ( bIsSoldierRocketJumping ) 
		{
			float flDamage = info.GetDamage() * tf_damagescale_self_soldier.GetFloat();
			info.SetDamage( flDamage );

			if ( m_iHealthBefore - flDamage > 0.0f )
			{
				nJumpType = TF_PLAYER_ROCKET_JUMPED;
			}
		}
		else if ( bIsDemomanPipeJumping )
		{
			nJumpType = TF_PLAYER_STICKY_JUMPED;

			// TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY prep work
			if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER)
			{
				CTFPipebombLauncher* pMineLauncher = static_cast<CTFPipebombLauncher*>(pWeapon);
				pMineLauncher->MarkMinesForAchievementJumpAndDestroy(pInflictor);
			}
		}

		if ( nJumpType )
		{
			bool bPlaySound = false;
			if ( pWeapon )
			{
				int iNoBlastDamage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iNoBlastDamage, no_self_blast_dmg );
				bPlaySound = iNoBlastDamage ? true : false;
			}

			SetBlastJumpState( nJumpType, bPlaySound );
		}
	}

	// TF2C_ACHIEVEMENT_SURF_ENEMY_MINE
	if (pAttacker != this && !(GetFlags() & FL_ONGROUND) && !(GetFlags() & FL_INWATER))
	{
		if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER && \
			!(info.GetDamageType() & DMG_USEDISTANCEMOD))
		{
			int proxyMine = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, proxyMine, mod_sticky_is_proxy);
			if (proxyMine > 0)
			{
				IGameEvent* event = gameeventmanager->CreateEvent("enemy_primed_mine_jump");
				if (event)
				{
					event->SetInt("userid", GetUserID());
					gameeventmanager->FireEvent(event);
				}
			}
		}
	}

	// Save damage force for ragdolls.
	m_vecTotalBulletForce = info.GetDamageForce();
	m_vecTotalBulletForce.x = clamp( m_vecTotalBulletForce.x, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.y = clamp( m_vecTotalBulletForce.y, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.z = clamp( m_vecTotalBulletForce.z, -15000.0f, 15000.0f );

	int iTookDamage = 0;
 	int bitsDamage = inputInfo.GetDamageType();

	bool bAllowDamage = false;

	// Check to see if our attacker is a trigger_hurt entity (and allow it to kill us even if we're invuln).
	if ( pAttacker && pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		CTriggerHurt *pTrigger = dynamic_cast<CTriggerHurt *>( pAttacker );
		if ( pTrigger )
		{
			bAllowDamage = true;
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG )
	{
		bAllowDamage = true;
	}

	if ( !TFGameRules()->ApplyOnDamageModifyRules( info, this, bAllowDamage ) )
	{
		if ( bDebug )
		{
			Msg( "ABORTED: TFGameRules() denied the damage to be taken!\n" );
		}
		
		return 0;
	}

	bool bFatal = ( GetHealth() - info.GetDamage() ) <= 0;

	bool bTrackEvent = pTFAttacker && pTFAttacker != this && !pTFAttacker->IsBot() && !IsBot();
	if ( bTrackEvent )
	{
		float flHealthRemoved = bFatal ? GetHealth() : info.GetDamage();
		if ( info.GetDamageBonus() )
		{
			// Don't deal with raw damage numbers, only health removed.
			// Example based on a crit rocket to a player with 120 hp:
			// Actual damage is 120, but potential damage is 300, where
			// 100 is the base, and 200 is the bonus. Apply this ratio
			// to actual (so, attacker did 40, and provider added 80).
			float flBonusMult = info.GetDamage() / abs( info.GetDamageBonus() - info.GetDamage() );
			float flBonus = flHealthRemoved - ( flHealthRemoved / flBonusMult );
			flHealthRemoved -= flBonus;
		}
	}

	// This should kill us.
	if ( bFatal )
	{
		// Damage could have been modified since we started,
		// try to prevent death with buddha one more time.
		if ( bBuddha )
		{
			m_iHealth = 1;
			return 0;
		}
	}

	// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice.
	iTookDamage = CBaseCombatCharacter::OnTakeDamage( info );

	// Early out if the base class took no damage.
	if ( !iTookDamage )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player failed to take the damage!\n" );
		}

		return 0;
	}

	if ( bDebug )
	{
		Warning( "    DEALT: Player took %.2f damage\n", info.GetDamage() );
		Warning( "    HEALTH LEFT: %d\n", GetHealth() );
	}

	// Some weapons have the ability to impart extra moment just because they feel like it.
	// Let their attributes do so if they're in the mood.
	if ( pWeapon )
	{
		float flZScale = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flZScale, apply_z_velocity_on_damage );
		if ( flZScale != 0.0f )
		{
			ApplyAbsVelocityImpulse( Vector( 0.0f, 0.0f, flZScale ) );
		}

		float flDirScale = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDirScale, apply_look_velocity_on_damage );
		if ( flDirScale != 0.0f && pAttacker )
		{
			Vector vecForward;
			AngleVectors( pAttacker->EyeAngles(), &vecForward );

			Vector vecForwardNoDownward = Vector( vecForward.x, vecForward.y, Min( 0.0f, vecForward.z ) ).Normalized();
			ApplyAbsVelocityImpulse( vecForwardNoDownward * flDirScale );
		}
	}
	if (info.GetDamageCustom() == TF_DMG_EARTHQUAKE)
	{
		/*Vector vecDir;
		QAngle angDir = info.get
		AngleVectors(angDir, &vecDir);

		Vector vecPushDir;
		QAngle angPushDir = angDir;
		float flPitch = AngleNormalize(angPushDir[PITCH]);

		// If they're on the ground, always push them at least 45 degrees up.
		angPushDir[PITCH] = Min(-45.0f, flPitch);

		AngleVectors(angPushDir, &vecPushDir);*/

		auto pTFAttacker = ToTFPlayer(info.GetAttacker());
		if (pTFAttacker)
		{
			float flVerticalPushStrength = tf2c_earthquake_vertical_speed_const.GetFloat() + tf2c_earthquake_vertical_speed_mult.GetFloat() * info.GetDamage();
			if (IsPlayerClass(TF_CLASS_HEAVYWEAPONS, true))
			{
				// Heavies take less push from non sentryguns.
				flVerticalPushStrength *= 0.5f;
			}
			m_Shared.AirblastPlayer(pTFAttacker, Vector(0.0f, 0.0f, 1.0f), flVerticalPushStrength);
		}
	}

	// Let weapons react to their owner being injured.
	CTFWeaponBase *pMyWeapon = GetActiveTFWeapon();
	if ( pMyWeapon )
	{
		pMyWeapon->ApplyOnInjuredAttributes( this, pTFAttacker, info );
	}

	// Send the damage message to the client for the hud damage indicator
	// Try and figure out where the damage is coming from.
	Vector vecDamageOrigin = info.GetReportedPosition();

	// If we didn't get an origin to use, try using the attacker's origin.
	if ( vecDamageOrigin == vec3_origin && pInflictor )
	{
		vecDamageOrigin = pInflictor->GetAbsOrigin();
	}

	if (bitsDamage & DMG_FALL)	// We have moved the fall damage sound here from movehelper_server so we don't have to play it if player was invulnerable to fall damage
	{
		CRecipientFilter filter;
		filter.AddRecipientsByPAS(GetAbsOrigin());

		CBaseEntity::EmitSound(filter, entindex(), "Player.FallDamage");
	}

	// Tell the player's client that he's been hurt.
	if ( m_iHealthBefore != GetHealth() )
	{
 		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
			WRITE_SHORT( clamp( (int)info.GetDamage(), 0, 32000 ) );
			// Tell the client whether they should show it in the indicator.
			if ( bitsDamage != DMG_GENERIC &&
				!( bitsDamage & DMG_DROWN || bitsDamage & DMG_FALL || bitsDamage & DMG_BURN ) &&
				!IsDOTDmg( info ) )
			{
				WRITE_BOOL( true );
				WRITE_VEC3COORD( vecDamageOrigin );
			}
			else
			{
				WRITE_BOOL( false );
			}
		MessageEnd();
	}

	// Add to the damage total for clients, which will be sent as a single
	// message at the end of the frame.
	if ( pInflictor && pInflictor->edict() )
	{
		m_DmgOrigin = pInflictor->GetAbsOrigin();
	}

	m_DmgTake += (int)info.GetDamage();

	// Reset damage time countdown for each type of time based damage player just sustained.
	for ( int i = 0; i < CDMG_TIMEBASED; i++ )
	{
		// Make sure the damage type is really time-based,
		// this is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	// Display any effect associate with this damage type.
	DamageEffect( info );

	m_bitsDamageType |= bitsDamage;	// Save this so we can report it to the client.
	m_bitsHUDDamage = -1; // Make sure the damage bits get reset.

	// Flinch!
	bool bFlinch = true;
	if ( bitsDamage != DMG_GENERIC )
	{
		if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
		{
			if ( pAttacker && ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN ) )
			{
				float flDistSqr = ( pAttacker->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr > 750 * 750 )
				{
					bFlinch = false;
				}
			}
		}

		if ( bFlinch )
		{
			ApplyPunchImpulseX( -2 );
			//m_Local.m_vecPunchAngle.SetX( -2 );
			PlayFlinch( info );
		}
	}

	// Do special explosion damage effect.
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	if ( m_iHealthBefore != GetHealth() )
	{
		// TF2C VIP Protect me voice line
		//Don't call for protection on fall damage for example by checking if damage has player attacker
		if ( IsPlayerClass( TF_CLASS_CIVILIAN ) && IsVIP() && pTFAttacker)
		{
			SpeakConceptIfAllowed( MP_CONCEPT_CIVILIAN_PROTECT );
		}

		PainSound( info );

		// If we're disguised and attacked by a team we're not disguised as, decrease our
		// disguised health to fool any enemies that can see our health on the Target ID.
		if ( m_Shared.IsDisguised() && ( pAttacker && !m_Shared.DisguiseFoolsTeam( pAttacker->GetTeamNumber() ) ) )
		{
			// If it becomes less than zero, set it to one, can't really do much other than that, so
			// our cover may have been blown for those keeping a close eye on me.
			int iDisguiseHealth = m_Shared.GetDisguiseHealth() - m_DmgTake;
			if ( iDisguiseHealth < 1 )
			{
				iDisguiseHealth = 1;
			}

			m_Shared.SetDisguiseHealth( iDisguiseHealth );
		}
	}

	// Detect drops below 25% health and restart expression, so that characters look worried.
	int iHealthBoundary = ( GetMaxHealth() * 0.25f );
	if ( GetHealth() <= iHealthBoundary && m_iHealthBefore > iHealthBoundary )
	{
		ClearExpression();
	}

#ifdef _DEBUG
	// Report damage from the info in debug so damage against targetdummies goes
	// through the system, as m_iHealthBefore - GetHealth() will always be 0.
	CTF_GameStats.Event_PlayerDamage( this, info, info.GetDamage() );
#else
	CTF_GameStats.Event_PlayerDamage( this, info, m_iHealthBefore - GetHealth() );
#endif // _DEBUG

	if ( pWeapon ) 
	{
		pWeapon->ApplyPostHitEffects( inputInfo, this );
	}
	
	// Mark the damage that's been taken.
	AddDamagerToHistory( pAttacker, info.GetWeapon() );
	m_Shared.NoteLastDamageTime( m_lastDamageAmount );

	return info.GetDamage();
}


void CTFPlayer::DamageEffect( float flDamage, int iDamageType )
{
	CTakeDamageInfo info( NULL, NULL, flDamage, iDamageType );
	DamageEffect( info );
}
void CTFPlayer::DamageEffect( CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();
	bool bDisguised = ( m_Shared.IsDisguised() && ( pAttacker && m_Shared.DisguiseFoolsTeam( pAttacker->GetTeamNumber() ) ) );

	int iDamageType = info.GetDamageType();
	if ( iDamageType & DMG_CRUSH )
	{
		// Red damage indicator.
		color32 red = { 128, 0, 0, 128 };
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if ( iDamageType & DMG_DROWN )
	{
		// Blue damage indicator.
		color32 blue = { 0, 0, 128, 128 };
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if ( !bDisguised )
	{
		if ( iDamageType & DMG_SLASH )
		{
			// If slash damage shoot some blood.
			SpawnBlood( EyePosition(), g_vecAttackDir, BloodColor(), info.GetDamage() );
		}
		else if ( iDamageType & DMG_BULLET )
		{
			EmitSound( "Flesh.BulletImpact" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is the player the passed player class?
//-----------------------------------------------------------------------------
bool CTFPlayer::IsPlayerClass( int iClass, bool bIgnoreRandomizer /*= false*/ ) const
{
	if ( !bIgnoreRandomizer && GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) )
		return true;

	return m_PlayerClass.IsClass( iClass );
}


void CTFPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state.
	if ( IsPlayerClass( TF_CLASS_UNDEFINED, true ) || !m_Shared.InState( TF_STATE_ACTIVE ) )
		return;

	// Prevent VIP from suiciding to prevent griefing.
	if ( !bForce )
	{
		if ( IsVIP() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
			return;

		// Don't suicide during the "bonus time" if we're not on the winning team.
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 GetTeamNumber() != TFGameRules()->GetWinningTeam() )
			return;
	}
	
	m_iSuicideCustomKillFlags = TF_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}


void CTFPlayer::CommitCustomSuicide( bool bExplode /* = false */, bool bForce /*= false*/, ETFDmgCustom iCustomDamageType /*= TF_DMG_CUSTOM_SUICIDE*/, Vector vecVelocityOverride /*= 0,0,0*/, bool bUseForceOverride /*= false*/)
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state.
	if (IsPlayerClass(TF_CLASS_UNDEFINED, true) || !m_Shared.InState(TF_STATE_ACTIVE))
		return;

	// Prevent VIP from suiciding to prevent griefing.
	if (!bForce)
	{
		if (IsVIP() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN)
			return;

		// Don't suicide during the "bonus time" if we're not on the winning team.
		if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
			GetTeamNumber() != TFGameRules()->GetWinningTeam())
			return;
	}

	m_iSuicideCustomKillFlags = iCustomDamageType;

	if ( bUseForceOverride )
	{
		BaseClass::CommitSuicide( vecVelocityOverride, bExplode, bForce );
	}
	else
	{
		BaseClass::CommitSuicide( bExplode, bForce );
	}
}


void CTFPlayer::PlayDamageResistSound( float flStartDamage, float flModifiedDamage, const CUtlVector<CTFPlayer*>& vecPlayersPlayFullVolume)
{
	if ( flStartDamage <= 0.0f )
		return;

	// Spam control.
	if ( gpGlobals->curtime - m_flLastDamageResistSoundTime <= 0.1f )
		return;

	// Play an absorb sound based on the percentage the damage has been reduced to.
	float flDamagePercent = flModifiedDamage / flStartDamage;
	if ( flDamagePercent > 0.0f && flDamagePercent < 1.0f )
	{
		const char *pszSoundName = flDamagePercent >= 0.75f ? "Player.ResistanceLight" :
								   flDamagePercent <= 0.25f ? "Player.ResistanceHeavy" : "Player.ResistanceMedium";

		CSoundParameters params;
		if ( CBaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
		{
			CPASAttenuationFilter filter( GetAbsOrigin(), params.soundlevel );
			EmitSound_t ep( params );
			ep.m_flVolume *= RemapValClamped( flStartDamage, 1.0f, 70.0f, 0.7f, 1.0f );

			FOR_EACH_VEC(vecPlayersPlayFullVolume, i)
			{
				// Always play sound to owner and remove them from the general recipient list.
				CTFPlayer* pSingleFilterPlayer = vecPlayersPlayFullVolume[i];
				if (pSingleFilterPlayer)
				{
					CSingleUserRecipientFilter singleFilter(pSingleFilterPlayer);
					EmitSound(singleFilter, pSingleFilterPlayer->entindex(), ep);
					filter.RemoveRecipient(pSingleFilterPlayer);
				}
			}

			EmitSound( filter, entindex(), ep );
			m_flLastDamageResistSoundTime = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( TFGameRules()->IsInItemTestingMode() && !IsFakeClient() )
		return 0;

	CTFGameRules::DamageModifyExtras_t outParams;
	outParams.bIgniting = false;
	outParams.bSelfBlastDmg = false;
	outParams.bPlayDamageReductionSound = false; 
	//outParams.vecPlayDamageReductionSoundFullPlayers.Purge(); // Thinks vectors always start properly empty

	CTakeDamageInfo localInfo = info;

	// Hard out requested from ApplyOnDamageModifyRules.
	float realDamage = TFGameRules()->ApplyOnDamageAliveModifyRules( info, this, outParams );
	if ( realDamage == -1 )
		return 0;

	// This needs to be here for further calculations to not 
	// wipe out previous ones, otherwise it's all screwed.
	localInfo.SetDamage( realDamage );

	// Allow active weapons to do something with this information.
	CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		// Our active weapon requested us to bail out.
		realDamage = pActiveWeapon->OwnerTakeDamage( localInfo );
		if ( realDamage == -1 )
			return 0;

		// Ditto.
		localInfo.SetDamage( realDamage );
	}

	// PDA2 slot weapons would also like to know (tf_weapon_invis).
	CTFWeaponBase *pPDA2Weapon = Weapon_OwnsThisID( TF_WEAPON_INVIS );
	if ( pPDA2Weapon )
	{
		// Our weapon requested us to bail out.
		realDamage = pPDA2Weapon->OwnerTakeDamage( localInfo );
		if ( realDamage == -1 )
			return 0;

		// Ditto, if future ones are added below.
		//localInfo.SetDamage( realDamage );
	}

	if ( outParams.bPlayDamageReductionSound )
	{
		PlayDamageResistSound( info.GetDamage(), realDamage, outParams.vecPlayDamageReductionSoundFullPlayers );
	}

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

	// Grab the vector of the incoming attack.
	// (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;



	CBaseEntity *pInflictor = info.GetInflictor();
	if ( pInflictor )
	{
		int iExplosiveBullet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iExplosiveBullet, explosive_bullets);

		if (iExplosiveBullet)
		{
			if (pWeapon->GetEnemy() == this)
			{
				if (info.GetAttacker())
				{
					vecDir = info.GetAttacker()->WorldSpaceCenter() - Vector(0.0f, 0.0f, 10.0f) - WorldSpaceCenter();
				}
				else
				{
					vecDir = pInflictor->WorldSpaceCenter() - Vector(0.0f, 0.0f, 10.0f) - WorldSpaceCenter();
				}
			}
			else
			{
				vecDir = info.GetDamagePosition() - Vector(0.0f, 0.0f, 10.0f) - WorldSpaceCenter();
			}
		}
		else
		{
			if (info.GetDamageCustom() == TF_DMG_EARTHQUAKE)
			{
				vecDir = info.GetDamagePosition() - Vector(0.0f, 0.0f, 10.0f) - WorldSpaceCenter();
			}
			else
			{
				vecDir = pInflictor->WorldSpaceCenter() - Vector(0.0f, 0.0f, 10.0f) - WorldSpaceCenter();
			}
		}
		pInflictor->AdjustDamageDirection( info, vecDir, this );
		VectorNormalize( vecDir );
	}

	g_vecAttackDir = vecDir;

	// Do the damage.
	m_bitsDamageType |= info.GetDamageType();

	float flBleedingTime = 0.0f;
	int iPrevHealth = GetHealth();

	int iSelfDamagePenetratesUber = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iSelfDamagePenetratesUber, mod_self_damage_penetrates_uber);
	if (m_takedamage != DAMAGE_EVENTS_ONLY || (iSelfDamagePenetratesUber && this == info.GetAttacker()))
	{
		if ( info.GetDamageCustom() != TF_DMG_CUSTOM_BLEEDING && !outParams.bSelfBlastDmg )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flBleedingTime, bleeding_duration );
		}

		// Take damage and round to the nearest integer.
		m_iHealth -= ( realDamage + 0.5f );
	}

	// DMG_GENERIC doesn't update our last damage time.
	bool bGenericDamage = ( info.GetDamageType() == DMG_GENERIC );
	if ( !bGenericDamage )
	{
		m_flLastDamageTime = gpGlobals->curtime;
	}

	// Apply a damage force.
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( !pAttacker )
		return 0;



	bool bDoNotDoKnockback = false;

	int iNoKnockbackOnNotMiniCritOrCrit = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iNoKnockbackOnNotMiniCritOrCrit, no_knockback_not_minicrit_crit);
	bDoNotDoKnockback = iNoKnockbackOnNotMiniCritOrCrit && info.GetCritType() == CTakeDamageInfo::CRIT_NONE;

	int iNoKnockbackOnNotAirborne = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iNoKnockbackOnNotAirborne, no_knockback_not_airborne);
	bDoNotDoKnockback = bDoNotDoKnockback || (iNoKnockbackOnNotAirborne && !IsAirborne());

	int iNoKnockbackOnNotAirborneAndNotMinicritOrCrit = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iNoKnockbackOnNotAirborneAndNotMinicritOrCrit, no_knockback_not_airborne_and_not_minicrit_crit);
	bDoNotDoKnockback = bDoNotDoKnockback || (iNoKnockbackOnNotAirborneAndNotMinicritOrCrit && !IsAirborne() && info.GetCritType() == CTakeDamageInfo::CRIT_NONE);

	if (!(info.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE || bDoNotDoKnockback))
	{
		if ( pInflictor && GetMoveType() == MOVETYPE_WALK && 
		   !pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) && 
		   !m_Shared.InCond( TF_COND_DISGUISED ) )	
		{
			if ( !m_Shared.InCond( TF_COND_MEGAHEAL ) || outParams.bSelfBlastDmg )
			{
				ApplyPushFromDamage( info, vecDir );
			}
		}
	}

	// Always NULL check this below.
	CTFPlayer *pTFAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pTFAttacker && pTFAttacker != this )
	{
		if ( outParams.bIgniting )
		{
			m_Shared.Burn( pTFAttacker, dynamic_cast<CTFWeaponBase *>( info.GetWeapon() ) );
		}

		if ( flBleedingTime > 0.0f )
		{
			m_Shared.MakeBleed( pTFAttacker, dynamic_cast<CTFWeaponBase *>( info.GetWeapon() ), flBleedingTime );
		}
	}

	// No bleeding while invul, disguised, or DMG_GENERIC.
	bool bBleed = !bGenericDamage && ( ( !m_Shared.InCond( TF_COND_DISGUISED ) || !m_Shared.DisguiseFoolsTeam(pAttacker->GetTeamNumber()) ) && !m_Shared.IsInvulnerable() && !m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB) );
										   
	// Except if we are really bleeding!
	bBleed |= m_Shared.InCond( TF_COND_BLEEDING );
	
	if ( bBleed && pTFAttacker )
	{
		CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			bBleed = false;
		}
	}

	if ( bBleed && realDamage > 0.0f )
	{
		Vector vecDamagePos = info.GetDamagePosition();
		if ( vecDamagePos == vec3_origin )
		{
			vecDamagePos = WorldSpaceCenter();
		}

		CPVSFilter filter( vecDamagePos );
		TE_TFBlood( filter, 0.0f, vecDamagePos, -vecDir, entindex(), (ETFDmgCustom)info.GetDamageCustom() );
	}

	if ( pTFAttacker )
	{
		// If we're invuln, give whomever provided it a reward.
		if ( m_Shared.IsInvulnerable() && realDamage > 0.0f )
		{
			CTFPlayer* pProvider = ToTFPlayer( m_Shared.GetConditionProvider( TF_COND_INVULNERABLE ));
			if ( pProvider )
			{
				CTF_GameStats.Event_PlayerBlockedDamage( pProvider, realDamage );
			}

			IGameEvent* event = gameeventmanager->CreateEvent("damage_blocked");
			if (event)
			{
				event->SetInt("victim", this->GetUserID());
				event->SetInt("provider", pProvider ? pProvider->GetUserID() : -1);
				event->SetInt("attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1);
				event->SetInt("amount", realDamage);
				gameeventmanager->FireEvent(event);
			}
		}

		if ( pTFAttacker != this )
		{
			pTFAttacker->RecordDamageEvent( info, m_iHealth <= 0 );

			FOR_EACH_VEC(pTFAttacker->m_Shared.m_vecHealers, i)
			{
				CTFPlayer* pHealer = ToTFPlayer(pTFAttacker->m_Shared.m_vecHealers[i].pPlayer);
				if (pHealer && !pTFAttacker->m_Shared.HealerIsDispenser(i))
				{
					pHealer->RecordDamageEvent(info, m_iHealth <= 0);
				}
			}
		}
	}

	// Fire a global game event - "player_hurt"
	IGameEvent *event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "health", Max( 0, GetHealth() ) );

		// HLTV event priority, not transmitted
		event->SetInt( "priority", 5 );	

		int iDamageAmount = ( iPrevHealth - GetHealth() );
		if( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_RIOT_SHIELD )
		{
			CTFRiot *pShield = static_cast<CTFRiot *>(pActiveWeapon);
			if( pShield->GetBlockedDamage() )
			{
				iDamageAmount = pShield->GetBlockedDamage();
				pShield->ClearBlockedDamage();
				event->SetBool( "damagewasblocked", true );
			}
		}
		event->SetInt( "damageamount", iDamageAmount );

		// Hurt by another player.
		if ( pAttacker->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pAttacker );
			event->SetInt( "attacker", pPlayer->GetUserID() );
			
			event->SetInt( "custom", info.GetDamageCustom() );
			event->SetBool( "showdisguisedcrit", m_bShowDisguisedCrit );
			event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) != 0 );
			event->SetBool( "minicrit", m_bMiniCrit );
			event->SetBool( "allseecrit", m_bAllSeeCrit );
			Assert( (int)m_eBonusAttackEffect < 256 );
			event->SetInt( "bonuseffect", (int)m_eBonusAttackEffect );
			event->SetBool("usedistancemod", info.GetDamageType() & DMG_USEDISTANCEMOD);
			event->SetBool("is_afterburn", info.GetDamageType() & DMG_BURN);

			if ( pTFAttacker )
			{
				pActiveWeapon = pTFAttacker->GetActiveTFWeapon();
				event->SetInt( "weaponid", pActiveWeapon ? pActiveWeapon->GetWeaponID() : 0 );
			}
			event->SetInt("weaponid2", pWeapon ? pWeapon->GetWeaponID() : 0);
			if (pInflictor && pInflictor->GetEnemy())
			{
				event->SetInt("inflictor_enemy", pInflictor->GetEnemy()->entindex());
			}
			else
			{
				event->SetInt("inflictor_enemy", 0);
			}

			event->SetInt("multicount", info.GetMultiCount());
			
		}
		// Hurt by world.
		else
		{
			event->SetInt( "attacker", 0 );
		}

		event->SetFloat( "x", info.GetDamagePosition().x );
		event->SetFloat( "y", info.GetDamagePosition().y );
		event->SetFloat( "z", info.GetDamagePosition().z );

        gameeventmanager->FireEvent( event );
	}

	// Done.
	return 1;
}

ConVar tf_preround_push_from_damage_enable( "tf_preround_push_from_damage_enable", "0", FCVAR_NONE, "If enabled, this will allow players using certain type of damage to move during pre-round freeze time." );


void CTFPlayer::ApplyPushFromDamage( const CTakeDamageInfo &info, Vector vecDir )
{
	// Check if player can be moved.
	if ( !tf_preround_push_from_damage_enable.GetBool() && !CanPlayerMove() )
		return;

	if ( info.GetDamageType() == DMG_GENERIC )
		return;

	Vector vecForce;
	vecForce.Init();
	if ( info.GetAttacker() == this )
	{
		Vector vecSize = WorldAlignSize();
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
		if ( vecSize == hullSizeCrouch )
		{
			// Use the original hull for damage force calculation to ensure our RJ height doesn't change due to crouch hull increase.
			// ^^ Comment above is an ancient lie, ducking actually increases blast force, this value increases it even more 82 standing, 62 ducking, 55 modified...
			vecSize.z = 55.0f;
		}

		float flDamageForForce = info.GetDamageForForceCalc() ? info.GetDamageForForceCalc() : info.GetDamage();

		float flSelfPushMult = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flSelfPushMult, mult_dmgself_push_force );

		
		if ( IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			// Rocket Jump.
			if ( ( info.GetDamageType() & DMG_BLAST ) )
			{
				if ( GetFlags() & FL_ONGROUND )
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_badrj.GetFloat() ) * flSelfPushMult;
				}
				else
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_rj.GetFloat() ) * flSelfPushMult;
				}

				//SetBlastJumpState( TF_PLAYER_ROCKET_JUMPED );	// is already handled elsewhere for proper rocketjumps.
				// Commented out because otherwise we get double "rocket_jump" event on jumping RJ. Demo doesn't get his state set to sticky jumping when jumping off ground either.

				// Reset duck in air on self rocket impulse.
				m_Shared.ResetAirDucks();
			}
			else
			{
				// Self Damage no force.
				vecForce.Zero();
			}
		}
		else
		{
			// Jumps (Stickies).
			vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, DAMAGE_FORCE_SCALE_SELF ) * flSelfPushMult;

			// Reset duck in air on self grenade impulse.
			m_Shared.ResetAirDucks();
		}

		// TF2C Lower rocket jump force for slowed flag carriers.
		if ( HasTheFlag() && tf2c_ctf_carry_slow.GetInt() > 0 && tf2c_ctf_carry_slow_blastjumps.GetBool() )
		{
			if ( tf2c_ctf_carry_slow.GetInt() < 2 )
			{
				int iPlayerClass = GetPlayerClass()->GetClassIndex();
				vecForce *= GetPlayerClassData( iPlayerClass )->m_flFlagMaxSpeed / GetPlayerClassData( iPlayerClass )->m_flMaxSpeed;
			}
			else
			{
				vecForce *= tf2c_ctf_carry_slow_mult.GetFloat();
			}
		}

		// TF2C Limit the extent to which we can boost Jump Pad launches.
		if ( m_Shared.InCond( TF_COND_JUST_USED_JUMPPAD ) )
		{
			//DevMsg( "rocket jump velocity reduced from %2.2f ", vecForce.Length() );
			vecForce *= 0.5f;
			//DevMsg( "to %2.2f\n", vecForce.Length() );
		}
	}
	else
	{
		// Don't let bot get pushed while they're in spawn area.
		if ( m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) )
			return;
		
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

		// Sentryguns push a lot harder.
		CBaseEntity *pInflictor = info.GetInflictor();
		if ( ( info.GetDamageType() & DMG_BULLET ) && pInflictor && pInflictor->IsBaseObject() )
		{
			// Only sentry buildings can damage us at the moment.
			float flSentryPushMultiplier = 16.0f;
			CObjectSentrygun *pSentry = static_cast<CObjectSentrygun *>( info.GetInflictor() );
			if ( pSentry )
			{
				flSentryPushMultiplier = pSentry->GetPushMultiplier();

				// Scale the force based on Distance.
				float flDistSqr = ( pSentry->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr > SENTRY_MAX_RANGE_SQRD )
				{
					flSentryPushMultiplier *= 0.5f;
				}
			}

			vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), flSentryPushMultiplier );
		}
		else
		{
			if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
			{
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() );
				vecForce.z = 0;
			}
			else
			{
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() );
			}

			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS, true ) )
			{
				// Heavies take less push from non sentryguns.
				vecForce *= 0.5f;
			}
		}

		if (info.GetDamageType() & DMG_BLAST)
		{
			m_bBlastLaunched = true;
		}

		float flDamageForceReduction = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDamageForceReduction, damage_force_reduction );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flDamageForceReduction, weapon_enemy_knockback_mod);

		if ( m_Shared.InCond( TF_COND_LAUNCHED_SELF ) )
		{
			CALL_ATTRIB_HOOK_FLOAT( flDamageForceReduction, damage_force_reduction_self_launch );
		}

		vecForce *= flDamageForceReduction;
	}

#ifdef ITEM_TAUNTING
	if ( m_Shared.InCond( TF_COND_TAUNTING ) && m_pTauntItem )
	{
		CEconItemDefinition *pItemDef = m_pTauntItem->GetStaticData();
		if ( pItemDef && pItemDef->taunt.stop_taunt_if_moved )
		{
			StopTaunt();
		}
	}
#endif

	ApplyAbsVelocityImpulse( vecForce );
}

void CTFPlayer::SetBlastJumpState( int iState, bool bPlaySound /*= false*/ )
{
	m_iBlastJumpState |= iState;

	const char *pszEvent = NULL;
	if ( iState == TF_PLAYER_STICKY_JUMPED )
	{
		pszEvent = "sticky_jump";
	}
	else if ( iState == TF_PLAYER_ROCKET_JUMPED )
	{
		pszEvent = "rocket_jump";
	}

	if ( pszEvent )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( pszEvent );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetBool( "playsound", bPlaySound );
			gameeventmanager->FireEvent( event );
		}
	}

	if (m_iBlastJumpState)
	{
		IGameEvent* event = gameeventmanager->CreateEvent("blast_jump");
		if (event)
		{
			event->SetInt("userid", GetUserID());
			event->SetBool("playsound", bPlaySound);
			gameeventmanager->FireEvent(event);
		}
	}

	m_Shared.AddCond( TF_COND_BLASTJUMPING );
}


void CTFPlayer::ClearBlastJumpState( void )
{
	m_bCreatedRocketJumpParticles = false;
	m_iBlastJumpState = 0;
	m_Shared.RemoveCond( TF_COND_BLASTJUMPING );
}

//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people who damaged player.
//-----------------------------------------------------------------------------
void CTFPlayer::AddDamagerToHistory( CBaseEntity *pDamager, CBaseEntity *pWeapon /*= NULL*/ )
{
	// Ignore teammates, ourselves, and anything that isn't a player.
	if ( !pDamager || pDamager == this || !pDamager->IsPlayer() || !IsEnemy( pDamager ) )
		return;

	// Set this damager as most recent and note the time.
	DamagerHistory_t newDamagerHistory;
	newDamagerHistory.hDamager = pDamager;
	newDamagerHistory.flTimeDamage = gpGlobals->curtime;

	// Remember the weapon that he used to damage us.
	newDamagerHistory.hWeapon = pWeapon;

	// Make sure it's not in the list already.
	int i, c;
	for ( i = 0, c = m_vecDamagerHistory.Count(); i < c; i++ )
	{
		if ( m_vecDamagerHistory[i].hDamager == newDamagerHistory.hDamager )
		{
			m_vecDamagerHistory.Remove( i );
			c = m_vecDamagerHistory.Count();
			break;
		}
	}

	m_vecDamagerHistory.AddToHead( newDamagerHistory );

	// Remove the oldest entry if we're over max size.
	if ( c > MAX_DAMAGER_HISTORY )
	{
		m_vecDamagerHistory.Remove( c - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clears damager history.
//-----------------------------------------------------------------------------
void CTFPlayer::ClearDamagerHistory()
{
	m_vecDamagerHistory.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the most recent damager in the history.
//-----------------------------------------------------------------------------
DamagerHistory_t *CTFPlayer::GetRecentDamagerInHistory( CBaseEntity *pIgnore /*= NULL*/, float flMaxElapsed /*= -1.0f*/ )
{
	for ( int i = 0, c = m_vecDamagerHistory.Count(); i < c; i++ )
	{
		CBaseEntity *pDamager = m_vecDamagerHistory[i].hDamager.Get();
		if ( !pDamager )
			continue;

		if ( pIgnore && pDamager == pIgnore )
			continue;

		if ( flMaxElapsed != -1.0f && ( gpGlobals->curtime - m_vecDamagerHistory[i].flTimeDamage ) > flMaxElapsed )
			continue;

		return &m_vecDamagerHistory[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGib( const CTakeDamageInfo &info )
{
	// Check to see if we should allow players to gib.
	int nGibCvar = tf_playergib.GetInt();
	if ( nGibCvar == 0 )
		return false;

	if ( nGibCvar == 2 )
		return true;

	if ( info.GetDamageType() & DMG_NEVERGIB )
		return false;

	if ( info.GetDamageType() & DMG_ALWAYSGIB )
		return true;

	if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( ( info.GetDamageType() & DMG_CRITICAL ) || GetHealth() <= -30 )
			return true;
	}

	if (info.GetDamageCustom() == TF_DMG_CUSTOM_GIB)
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Figures out if there is a special assist responsible for our death.
// Must be called before conditions are cleared druing death.
//-----------------------------------------------------------------------------
void CTFPlayer::DetermineAssistForKill( const CTakeDamageInfo &info )
{
	CTFPlayer *pPlayerAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pPlayerAttacker )
		return;

	CTFPlayer *pPlayerAssist = NULL;

	// If we've been stunned, the stunner gets credit for the assist.
	if ( m_Shared.IsControlStunned() )
	{
		pPlayerAssist = m_Shared.GetStunner();
	}
	else
	{
		// If we are marked for death or tranquilized, etc. then give the provider an assist.
		CBaseEntity *pEntityAssist = m_Shared.GetConditionAssistFromVictim();
		if ( pEntityAssist )
		{
			pPlayerAssist = ToTFPlayer( pEntityAssist );
		}
	}

	// Can't assist ourself.
	m_Shared.SetAssist( pPlayerAttacker != pPlayerAssist ? pPlayerAssist : NULL );
}


void CTFPlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	if ( pVictim->IsPlayer() )
	{
		// Custom death handlers.
		const char *pszCustomDeath = "customdeath:none";
		if ( TFGameRules()->IsSentryDamager( info ) )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT )
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_DECAPITATION)
		{
			pszCustomDeath = "customdeath:decapitation";
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_GIB)
		{
			pszCustomDeath = "customdeath:gibbed";
		}

		// Revenge handler.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			const char *pszDomination = "domination:none";
			if ( pTFVictim->GetDeathFlags() & TF_DEATH_REVENGE )
			{
				pszDomination = "domination:revenge";
			}
			else if ( pTFVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
			{
				pszDomination = "domination:dominated";
			}

			CFmtStrN<128> modifiers( "%s,%s,victimclass:%s", pszCustomDeath, pszDomination, g_aPlayerClassNames_NonLocalized[pTFVictim->GetPlayerClass()->GetClassIndex()] );
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );
		}

		if (IsTauntKillDmg(info.GetDamageCustom()) && pVictim != this) // Telefrag intended
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints(this, pVictim, 1);
		}

		if ( IsAlive() )
		{
			// Sentry probably won't have a "weapon", so process on-sentry-kill stuff first.
			if ( TFGameRules()->IsSentryDamager( info ) )
			{
				int iAddMetalOnSentryKill = 0;
				CALL_ATTRIB_HOOK_INT( iAddMetalOnSentryKill, metal_on_sentry_kill );
				if ( iAddMetalOnSentryKill > 0 )
				{
					GiveAmmo( iAddMetalOnSentryKill, TF_AMMO_METAL, true );
					CSingleUserRecipientFilter filter( this );
					EmitSound( filter, entindex(), "Weapon_Designator.RewardMetal" );
				}
			}

			// Apply on-kill effects.
			int iTauntKillFillStoredCrits = 0;
			CALL_ATTRIB_HOOK_INT(iTauntKillFillStoredCrits, mod_taunt_kill_fill_stored_crits);
			if (iTauntKillFillStoredCrits && IsTauntKillDmg(info.GetDamageCustom()))
			{
				m_Shared.GainStoredCrits(m_Shared.GetStoredCritsCapacity());
			}

			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				pWeapon->IncrementKillCount();
				m_Shared.IncrementPlayerKillCount();

				float flCritOnKill = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCritOnKill, add_onkill_critboost_time );
				if ( flCritOnKill )
				{
					m_Shared.AddCond( TF_COND_CRITBOOSTED_ON_KILL, flCritOnKill );
				}

				// For heal_on_kill, we don't want the killing weapon, but the currently held weapon,
				// that way, afterburn or projectile kills still grant this bonus as expected.
				CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
				if ( pActiveWeapon )
				{
					float flHealOnKill = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActiveWeapon, flHealOnKill, heal_on_kill );
					if ( flHealOnKill )
					{
						if ( flHealOnKill > 0 )
						{
							int iHealthRestored = TakeHealth( flHealOnKill, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
							CTF_GameStats.Event_PlayerHealedOther(this, (float)iHealthRestored);
						}
						else
						{
							CTakeDamageInfo localinfo( this, this, pWeapon, -flHealOnKill, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
							TakeDamage( localinfo );
						}
					}

					float flHealOnKillCrit = 0.0f;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, flHealOnKillCrit, heal_on_kill_crit );
					if ( flHealOnKillCrit && info.GetDamageType() & DMG_CRITICAL )
					{
						if ( flHealOnKillCrit > 0 )
						{
							int iHealthRestored = TakeHealth( flHealOnKillCrit, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
							CTF_GameStats.Event_PlayerHealedOther(this, (float)iHealthRestored);
						}
						else
						{
							CTakeDamageInfo localinfo( this, this, pWeapon, -flHealOnKillCrit, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
							TakeDamage( localinfo );
						}
					}
				}

				int iGainStoredCrit = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iGainStoredCrit, gain_stored_crits_on_kill);
				if (iGainStoredCrit)
				{
					m_Shared.GainStoredCrits(iGainStoredCrit);
				}

				string_t strAddCondOnKillSelf = NULL_STRING;
				CALL_ATTRIB_HOOK_STRING_ON_OTHER(pWeapon, strAddCondOnKillSelf, add_onkill_addcond_self);
				if (strAddCondOnKillSelf != NULL_STRING)
				{
					float args[2];
					UTIL_StringToFloatArray(args, 2, strAddCondOnKillSelf.ToCStr());
					DevMsg("onhit_addcond_self: %2.2f, %2.2f\n", args[0], args[1]);

					m_Shared.AddCond((ETFCond)Floor2Int(args[0]), args[1], this);
				}

				float flAddCloakOnKill = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT(flAddCloakOnKill, add_cloak_on_kill);
				if (flAddCloakOnKill)
				{
					m_Shared.AddSpyCloak(flAddCloakOnKill);
				}

				int iIsRussianRoulette = 0;
				CALL_ATTRIB_HOOK_INT(iIsRussianRoulette, mod_russian_roulette_taunt);
				if (iIsRussianRoulette && info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON)
				{
					m_Shared.GainStoredCrits(m_Shared.GetStoredCritsCapacity());
				}

				int iAddAmmoOnKill = 0;
				CALL_ATTRIB_HOOK_INT(iAddAmmoOnKill, add_ammo_on_kill);
				int iAddAmmoOnDirectHitKill = 0;
				CALL_ATTRIB_HOOK_INT(iAddAmmoOnDirectHitKill, add_ammo_on_direct_hit_kill);
				if (iAddAmmoOnKill)
				{
					if (iAddAmmoOnKill > 0)
						GiveAmmo(iAddAmmoOnKill, pWeapon->m_iPrimaryAmmoType, true);
					else
						RemoveAmmo(iAddAmmoOnKill, pWeapon->m_iPrimaryAmmoType);
				}
				if ( iAddAmmoOnDirectHitKill && (!(info.GetDamageType() & DMG_BLAST) || info.GetInflictor()->GetEnemy() == pVictim) )
				{
					if ( iAddAmmoOnDirectHitKill > 0 )
						GiveAmmo( iAddAmmoOnDirectHitKill, pWeapon->m_iPrimaryAmmoType, true );
					else
						RemoveAmmo( iAddAmmoOnDirectHitKill, pWeapon->m_iPrimaryAmmoType );
				}
				

				int iInstantReloadOnKill = 0;
				CALL_ATTRIB_HOOK_INT(iInstantReloadOnKill, instant_reload_on_kill);
				int iInstantReloadOnDirectHitKill = 0;
				CALL_ATTRIB_HOOK_INT(iInstantReloadOnDirectHitKill, instant_reload_on_direct_hit_kill);
				if (iInstantReloadOnKill > 0)
				{
					pWeapon->InstantlyReload(iInstantReloadOnKill);
				}
				if (iInstantReloadOnDirectHitKill > 0 && (!(info.GetDamageType() & DMG_BLAST) || (info.GetInflictor() && info.GetInflictor()->GetEnemy() == pVictim)))
				{
					pWeapon->InstantlyReload(iInstantReloadOnDirectHitKill);
				}

				int iInstantReloadAllWeaponsOnKill = 0;
				CALL_ATTRIB_HOOK_INT(iInstantReloadAllWeaponsOnKill, instant_reload_all_weapons_on_kill);
				int iInstantReloadAllWeaponsOnDirectHitKill = 0;
				CALL_ATTRIB_HOOK_INT(iInstantReloadAllWeaponsOnDirectHitKill, instant_reload_all_weapons_on_direct_hit_kill);
				if (iInstantReloadAllWeaponsOnKill > 0)
				{
					for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot <= TF_LOADOUT_SLOT_MELEE; ++iSlot)
					{
						CTFWeaponBase* pSlotWeapon = GetWeaponForLoadoutSlot(ETFLoadoutSlot(iSlot));
						if (pSlotWeapon)
						{
							pSlotWeapon->InstantlyReload(pSlotWeapon->GetDefaultClip1());
						}
					}
				}
				if (iInstantReloadAllWeaponsOnDirectHitKill > 0 && (!(info.GetDamageType() & DMG_BLAST) || info.GetInflictor()->GetEnemy() == pVictim))
				{
					for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot <= TF_LOADOUT_SLOT_MELEE; ++iSlot)
					{
						CTFWeaponBase* pSlotWeapon = GetWeaponForLoadoutSlot(ETFLoadoutSlot(iSlot));
						if (pSlotWeapon)
						{
							pSlotWeapon->InstantlyReload(pSlotWeapon->GetDefaultClip1());
						}
					}
				}

				int flWeaponDamageBoostOnKill = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flWeaponDamageBoostOnKill, apply_weapon_damage_boost_on_kill);
				if (flWeaponDamageBoostOnKill)
				{
					pWeapon->ApplyWeaponDamageBoostDuration(flWeaponDamageBoostOnKill);
				}

				// !!! foxysen detonator
				if (pWeapon->GetWeaponID() == TF_WEAPON_DETONATOR)
				{
					CTFDetonator* pDetonator = static_cast<CTFDetonator*>(pWeapon);
					pDetonator->m_bKilledSomeone = true;
				}
			}
		}
	}
	else if ( pVictim->IsBaseObject() )
	{
		CBaseObject *pObject = dynamic_cast<CBaseObject *>( pVictim );
		if ( pObject )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_OBJECT, pObject->GetResponseRulesModifier() );
		}
	}
}


void CTFPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	// !!! foxysen detonator
	if (GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_DETONATOR)
	{
		GetActiveTFWeapon()->StopSound("Weapon_Detonator.Charging");
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DIED );

	StateTransition( TF_STATE_DYING ); // Transition into the dying state.

	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

	// !!! foxysen pillstreak
	Vector OldWorldSpaceCenter = WorldSpaceCenter();
	bool bShouldExplodeOnDeath = false;
	bool bExplosionDamageEnemy = false;
	if (pAttacker && !(pAttacker == this || pAttacker->IsBSPModel()))
	{
		CTFWeaponBase* pAttackerWeapon = dynamic_cast<CTFWeaponBase*>(info.GetWeapon());
		/*CTFPillstreak* pWpnPillstreak = static_cast<CTFPillstreak*>(Weapon_OwnsThisID(TF_WEAPON_PILLSTREAK));
		if (pWpnPillstreak && pWpnPillstreak->GetPipeStreak() >= pWpnPillstreak->GetMaxDamagePipeStreak()
			&& info.GetDamageCustom() != TF_DMG_CUSTOM_BACKSTAB)
		{
			bShouldExplodeOnDeath = true;
			bExplosionDamageEnemy = false;
		}
		else*/ if ((pInflictor && info.GetInflictor()->IsBaseObject()) || (pAttackerWeapon && !pAttackerWeapon->IsMeleeWeapon()))
		{
			int iExplode = 0;
			CALL_ATTRIB_HOOK_INT( iExplode, explode_on_death );
			bExplosionDamageEnemy = bShouldExplodeOnDeath = iExplode > 0;
		}
	}

	// This is going to become a mess if we ever let Pyro take afterburn damage
	if ( m_Shared.InCond(TF_COND_BURNING) && m_Shared.m_hBurnAttacker && !IsPlayerClass( TF_CLASS_PYRO, true ) && m_Shared.m_hBurnAttacker->IsAlive())
	{
		int iStoreRemainingAfterburn = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(m_Shared.m_hBurnAttacker, iStoreRemainingAfterburn, remember_targets_afterburn);
		if (iStoreRemainingAfterburn)
		{
			m_Shared.m_hBurnAttacker->m_Shared.StoreNewRemainingAfterburn(m_Shared.m_flFlameRemoveTime, m_Shared.m_flFlameBurnTime);
		}
	}

	int iExtinguishOnDeath = 0;
	CALL_ATTRIB_HOOK_INT(iExtinguishOnDeath, on_death_remove_targets_afterburn);
	if (iExtinguishOnDeath)
	{
		// Loop through enemy players.
		int i, c;
		ForEachEnemyTFTeam(GetTeamNumber(), [&](int iTeam)
			{
			CTFTeam* pTeam = GetGlobalTFTeam(iTeam);

			if (!pTeam)
				return true;

			for (i = 0, c = pTeam->GetNumPlayers(); i < c; ++i)
			{
				CTFPlayer* pPlayer = pTeam->GetTFPlayer(i);
				if (!pPlayer)
					continue;

				if (!pPlayer->IsAlive() || pPlayer->IsPlayerClass(TF_CLASS_PYRO))
					continue;

				if (pPlayer->m_Shared.InCond(TF_COND_BURNING) && pPlayer->m_Shared.GetBurnAttacker() == this)
				{
					pPlayer->EmitSound("TFPlayer.FlameOut");
					pPlayer->m_Shared.RemoveCond(TF_COND_BURNING);

					IGameEvent* event = gameeventmanager->CreateEvent( "player_extinguished" );
					if ( event )
					{
						event->SetInt( "victim", pPlayer->entindex() );
						event->SetInt( "healer", pTFAttacker->entindex() );

						gameeventmanager->FireEvent( event, true );
					}

					if (!pPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
					{
						if (pTFAttacker)
						{
							if (pPlayer == pTFAttacker && pTFAttacker->IsPlayerClass(TF_CLASS_SPY))
							{
								pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_HARVESTER_COUNTER_SPY);
							}
							if (!pPlayer->IsEnemy(pTFAttacker) && pPlayer->GetHealth() <= ACHIEVEMENT_TF2C_HARVESTER_COUNTER_LOW_HEALTH_THRESHOLD && \
								!pPlayer->m_Shared.GetNumHealers() && pPlayer != pTFAttacker)
							{
								pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_HARVESTER_COUNTER_LOW_HEALTH);
							}
						}
					}
				}
			}

			return true;
			});
	}

	// Achievement conditions memories!
	bool bWasInTranqCond = m_Shared.InCond(TF_COND_TRANQUILIZED);
	bool bMedigunWasFullyCharged = false;	// is set below
	CTFPlayer* pTFTranqProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED));
	CTFWeaponBase* pHeldActiveWeapon = GetActiveTFWeapon();
	bool bWasHoldingMeleeWeapon = (pHeldActiveWeapon && pHeldActiveWeapon->IsMeleeWeapon());
	bool bWasBlastOrJumppadJumping = (m_Shared.InCond(TF_COND_LAUNCHED) || m_Shared.InCond(TF_COND_BLASTJUMPING));
	bool bWasAirborneForAirCombatHeight = IsAirborne(ACHIEVEMENT_TF2C_AIR_COMBAT_HEIGHT);
	bool bWasAirborneHeadshotKillMidflightHeight = IsAirborne(ACHIEVEMENT_TF2C_HEADSHOT_KILL_MIDFLIGHT_HEIGHT);
	bool bWasAirborne = IsAirborne();
	bool bWasBurning = m_Shared.InCond(TF_COND_BURNING);
	bool bWasAiming = m_Shared.InCond(TF_COND_AIMING);
	//CTFPlayer* pTFBurnProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_BURNING));

	if ( m_Shared.GetCarriedObject() )
	{
		// Blow it up at our position.
		CBaseObject *pObject = m_Shared.GetCarriedObject();
		pObject->Teleport( &WorldSpaceCenter(), &GetAbsAngles(), &vec3_origin );
		pObject->DropCarriedObject( this );

		CTakeDamageInfo newInfo( pInflictor, pAttacker, info.GetWeapon(), (float)pObject->GetHealth(), DMG_GENERIC, TF_DMG_BUILDING_CARRIED );
		pObject->Killed( newInfo );

		// Switch away from the builder so we don't drop the toolbox.
		SwitchToNextBestWeapon( GetActiveWeapon() );
	}

	// Capture The Flag
	// If the player has a capture flag and was killed by another player, award that player a defense.
	if ( HasTheFlag() && pTFAttacker && pTFAttacker != this )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			CCaptureFlag *pFlag = GetTheFlag();
			event->SetInt( "player", pTFAttacker->entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DEFEND );
			event->SetInt( "team", pFlag ? pFlag->GetTeamNumber() : 0 );
			event->SetInt( "priority", 8 );
			gameeventmanager->FireEvent( event );
		}

		CTF_GameStats.Event_PlayerDefendedPoint( pTFAttacker );
	}

	// If tf2c_ctf_attacker_bonus is set, respawn faster when not near own flag base.
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CTF && tf2c_ctf_attacker_bonus.GetBool() )
	{
		float flDistance = -1.0f;
		float flMinDist = tf2c_ctf_attacker_bonus_dist.GetFloat();

		for ( auto pFlag : CCaptureFlag::AutoList() ) 
		{
			if ( pFlag->IsDisabled() )
				continue;

			if ( InSameTeam( pFlag ) )
			{
				flDistance = GetAbsOrigin().DistTo( pFlag->GetResetPos() );
			}
		}

		if ( flDistance > flMinDist )
		{
			float flBonus = RemapValClamped( flDistance, flMinDist, flMinDist * 2.0, 1.0, 0.0 );
			m_flRespawnTimeOverride = flBonus * TFGameRules()->GetRespawnWaveMaxLength( GetTeamNumber(), true );
		}
	}

	// Try to respawn as soon as possible if we died shortly after spawning
	if (gpGlobals->curtime - GetSpawnTime() < tf2c_spawncamp_window.GetFloat())
	{
		m_flRespawnTimeOverride = 0.0f;
	}

	// Also respawn as fast as possible during setup
	if (TFGameRules()->InSetup())
	{
		m_flRespawnTimeOverride = 0.0f;
	}

	// Record if we were stunned for achievement tracking.
	m_iOldStunFlags = m_Shared.GetStunFlags();

	// Determine the optional assist for the kill.
	DetermineAssistForKill( info );

	int iRagdollFlags = 0;

	// We want the ragdoll to burn if the player was burning and was not a Pyro (who only burns momentarily).
	if ( m_Shared.InCond( TF_COND_BURNING ) && !IsPlayerClass( TF_CLASS_PYRO, true ) )
	{
		iRagdollFlags |= TF_RAGDOLL_BURNING;
	}

	if ( GetFlags() & FL_ONGROUND )
	{
		iRagdollFlags |= TF_RAGDOLL_ONGROUND;
	}

	CTFWeaponBase* pTFWeapon = dynamic_cast<CTFWeaponBase*>(info.GetWeapon());
	// Bonus points:
	// Killed Civilian in VIP mode!
	if ( IsVIP() )
	{
		if ( pTFAttacker && pTFAttacker != this )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pTFAttacker, this, 2 );
		}
	}

	// Killed a fully charged Medic!
	if ( MedicGetChargeLevel() == 1.0f )
	{
		iRagdollFlags |= TF_RAGDOLL_CHARGED;
		bMedigunWasFullyCharged = true;

		if ( pTFAttacker && pTFAttacker != this )
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints( pTFAttacker, this, 2 );
		}
	}

	// Killed someone using Jumppad! So rare!
	if (pTFAttacker && pTFAttacker != this \
		&& info.GetDamageCustom() == TF_DMG_CUSTOM_JUMPPAD_STOMP)
	{
		// First, reward the killer since it's so rare
		CTF_GameStats.Event_PlayerAwardBonusPoints(pTFAttacker, this, 1);

		// Now reward the builder of jumppad if it's not the killer. Or, rather, provider of the cond.
		CTFPlayer* pJumppadProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));
		if (pJumppadProvider && pJumppadProvider != pTFAttacker && !pTFAttacker->IsEnemy(pJumppadProvider)) // That's right, no bonus on stomp by spy
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints(pJumppadProvider, pTFAttacker, 1);
		}
	}

	// Melee crit kill someone under Tranq. Good teamwork, bonus to both!
	if (pTFAttacker && pTFAttacker != this \
		&& pTFWeapon && pTFWeapon->IsMeleeWeapon() && !IsDOTDmg(info) && m_Shared.InCond(TF_COND_TRANQUILIZED))
	{
		CTFPlayer* pTranqProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED));
		if (pTranqProvider && pTranqProvider != pTFAttacker && !pTFAttacker->IsEnemy(pTranqProvider))
		{
			CTF_GameStats.Event_PlayerAwardBonusPoints(pTFAttacker, this, 1);
			CTF_GameStats.Event_PlayerAwardBonusPoints(pTranqProvider, pTFAttacker, 1);
		}
	}

	int iRagdollsBecomeAsh = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRagdollsBecomeAsh, ragdolls_become_ash );
	if ( iRagdollsBecomeAsh || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE )
	{
		iRagdollFlags |= TF_RAGDOLL_ASH;
	}

	CTakeDamageInfo info_modified = info;

	// !!! foxysen pillstreak
	// Make player with fully filled Pillstreak damage bonus explode on death
	if (bShouldExplodeOnDeath)
	{
		info_modified.AddDamageType(DMG_BLAST);
		info_modified.AddDamageType(DMG_ALWAYSGIB);
		// Not terribly useful but who knows
		if (!bExplosionDamageEnemy)
		{
			info_modified.AddDamage(PILLSTREAK_DEATH_EXPLOSION_DAMAGE);
		}
		else
		{
			info_modified.AddDamage(GENERIC_DEATH_EXPLOSION_DAMAGE);
		}
	}

	// See if we should gib.
	if ( ShouldGib( info_modified ) )
	{
		iRagdollFlags |= TF_RAGDOLL_GIB;
	}


	// Remember the active weapon for later.
	RememberLastWeapons();

	CleanupOnDeath( true );

	if (pTFAttacker)
	{
		bool bDropHealthPack = false;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFAttacker, bDropHealthPack, drop_health_pack_on_kill);
		if( bDropHealthPack )
			DropHealthPack();

		bool bDropTeamHealthPack = false;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFAttacker, bDropTeamHealthPack, drop_team_health_pack_on_kill);
		if (bDropTeamHealthPack || ( TFGameRules()->IsInArenaMode() && tf2c_arena_drop_healthkit_on_death.GetBool() ) )
		{
			if ( pTFAttacker != this )
			{
				DropTeamHealthPack( pTFAttacker->GetTeamNumber() );
			}
		}
	}

	// If we died in sudden death and we're an Engineer, explode our buildings.
	if ( IsPlayerClass( TF_CLASS_ENGINEER, true ) && TFGameRules()->InStalemate() )
	{
		RemoveAllObjects( false );
	}

	// Show killer in death cam mode.
	// chopped down version of SetObserverTarget without the team check.
	if ( tf2c_disablefreezecam.GetBool() )
	{
		m_hObserverTarget.Set( NULL );
	}
	else if ( TFGameRules()->IsSentryDamager( info ) )
	{
		// See if we were killed by a sentrygun. If so, look at that instead of the player.
		m_hObserverTarget.Set( info.GetWeapon() );
	}
	else if ( pTFAttacker )
	{
		// Look at the player.
		m_hObserverTarget.Set( pAttacker );
	}
	else
	{
		m_hObserverTarget.Set( NULL );
	}

	bool bUsedSuicideButton = (info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE || info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE || info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_BOIOING || info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_STOMP);
	if (bUsedSuicideButton || pAttacker == this || pAttacker->IsBSPModel())
	{
		// Recalculate attacker if player killed himself or this was environmental death.
		DamagerHistory_t *pDamagerHistory = GetRecentDamagerInHistory(this, bUsedSuicideButton ? 10.0f : 5.0f);
		if (pDamagerHistory && pDamagerHistory->hDamager.Get() && pDamagerHistory->hDamager->IsPlayer())
		{
			pTFAttacker = (CTFPlayer *)pDamagerHistory->hDamager.Get();
			info_modified.SetAttacker(pTFAttacker);
			info_modified.SetInflictor(NULL);

			// If player pressed the suicide button set to the weapon used by attacker so they
			// still get on-kill effects.
			info_modified.SetWeapon(bUsedSuicideButton ? pDamagerHistory->hWeapon.Get() : NULL);

			info_modified.SetDamageType(DMG_GENERIC);
			if (!bUsedSuicideButton)
			{
				info_modified.SetDamageCustom(TF_DMG_CUSTOM_SUICIDE);

				// TF2C_ACHIEVEMENT_AA_GUN_BLAST_SUICIDE
				if (pTFWeapon && pTFWeapon->GetItemID() == 51)
				{
					if (pTFAttacker)
					{
						pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_AA_GUN_BLAST_SUICIDE);
					}
				}
			}
		}
		else if (GetRandomizerManager()->RandomizerMode(TF_RANDOMIZER_ITEMS) && (!bUsedSuicideButton && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN))
		{
			m_bPreserveRandomLoadout = true;
		}

		// Could consider anything a suicide "button" really if you believe hard enough.
		bUsedSuicideButton = true;
	}

	m_OnDeath.FireOutput( this, this );

	BaseClass::Event_Killed(info_modified);

	if (pTFAttacker)
	{
		if (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT)
		{
			CTF_GameStats.Event_Headshot(pTFAttacker);
		}
		else if (info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB)
		{
			CTF_GameStats.Event_Backstab(pTFAttacker);
		}
	}

	// Create the ragdoll entity.
	CreateRagdollEntity( iRagdollFlags, m_Shared.m_flInvisibility, (ETFDmgCustom)info.GetDamageCustom(), info.GetDamageType() );

	// Don't overflow the value for this.
	m_iHealth = 0;

	if ( pAttacker && pAttacker == pInflictor && pAttacker->IsBSPModel() )
	{
		DamagerHistory_t *pDamagerHistory = GetRecentDamagerInHistory( pAttacker, 5.0f );
		if ( pDamagerHistory && pDamagerHistory->hDamager.Get() && pDamagerHistory->hDamager->IsPlayer() )
		{
			// nobody is listening to this event??
			// -sappho
			#if 0
			IGameEvent *event = gameeventmanager->CreateEvent( "environmental_death" );
			if ( event )
			{
				event->SetInt( "killer", static_cast<CBasePlayer *>( pDamagerHistory->hDamager.Get() )->GetUserID() );
				event->SetInt( "victim", GetUserID() );
				event->SetInt( "priority", 9 ); // HLTV event priority, not transmitted.
				
				gameeventmanager->FireEvent( event );
			}
			#endif
		}
	}


	// !!! foxysen pillstreak
	// Make player with fully filled Pillstreak damage bonus explode on death
	if ( bShouldExplodeOnDeath && pTFWeapon )
	{
		Vector vecReported = OldWorldSpaceCenter;

		CPVSFilter filter(OldWorldSpaceCenter);
		if(!bExplosionDamageEnemy)
			TE_TFExplosion(filter, 0.0f, OldWorldSpaceCenter, vec3_origin, pTFWeapon->GetWeaponID(), entindex(), this, GetTeamNumber(), true);
		else
			TE_TFExplosion( filter, 0.0f, OldWorldSpaceCenter, vec3_origin, pTFWeapon->GetWeaponID(), entindex(), this, pAttacker->GetTeamNumber() );

		trace_t	tr;
		Vector vecEnd = GetAbsOrigin() - Vector(0, 0, 60);
		UTIL_TraceLine(GetAbsOrigin(), vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
		if (tr.m_pEnt && !tr.m_pEnt->IsPlayer())
			UTIL_DecalTrace(&tr, "Scorch");


		if (!bExplosionDamageEnemy)
			UTIL_ScreenShake(GetAbsOrigin(), 15.0, 150.0, 1.0, 300.0, SHAKE_START);
		else
			UTIL_ScreenShake(GetAbsOrigin(), 10.0, 150.0, 1.0, 300.0, SHAKE_START);

		// Blow up everyone
		float flDamage;
		float flRadius;
		int iDmgType;
		if (!bExplosionDamageEnemy)
		{
			flDamage = PILLSTREAK_DEATH_EXPLOSION_DAMAGE;
			flRadius = PILLSTREAK_DEATH_EXPLOSION_RADIUS;
			iDmgType = DMG_BLAST | DMG_HALF_FALLOFF | DMG_CRITICAL;
		}
		else
		{
			flDamage = GENERIC_DEATH_EXPLOSION_DAMAGE;
			flRadius = GENERIC_DEATH_EXPLOSION_RADIUS;
			iDmgType = DMG_BLAST | DMG_HALF_FALLOFF;
		}

		CTFRadiusDamageInfo radiusInfo;
		if ( !bExplosionDamageEnemy )
			radiusInfo.info.Set(this, info_modified.GetAttacker(), vec3_origin, OldWorldSpaceCenter, flDamage, iDmgType, TF_DMG_CUSTOM_NONE, &vecReported);
		else
			radiusInfo.info.Set( info_modified.GetAttacker(), this, vec3_origin, OldWorldSpaceCenter, flDamage, iDmgType, TF_DMG_CUSTOM_NONE, &vecReported );

		radiusInfo.m_vecSrc = OldWorldSpaceCenter;
		radiusInfo.m_flRadius = flRadius;
		//radiusInfo.m_pEntityIgnore = this;

		TFGameRules()->RadiusDamage(radiusInfo);
	}


	// Death by suicide, whether intentionally or not, prevents some 
	// achievements from being obtained by the, would be, assister.
	if (pTFAttacker && !pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME) && !bUsedSuicideButton)
	{
		switch (pTFAttacker->GetPlayerClass()->GetClassIndex())
		{
		case TF_CLASS_SOLDIER:
		{
			if (pTFWeapon)
			{
				float flRocketGravity = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFWeapon, flRocketGravity, mod_rocket_gravity);

				// TF2C_KILL_WITH_DISTANTRPG
				if (pTFWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER &&
					flRocketGravity > 0.0f && pTFAttacker->GetAbsOrigin().DistTo(GetAbsOrigin()) >= ACHIEVEMENT_TF2C_CALCULATED_ROCKET_DISTANCE_REQUIREMENT)
				{
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_KILL_WITH_DISTANTRPG);
				}
			}
			break;
		}
		case TF_CLASS_PYRO:
		{
			// TF2C_ACHIEVEMENT_PYRO_RAW
			if (!bWasBurning)
			{
				//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_PYRO_RAW);
			}
			break;
		}
		case TF_CLASS_DEMOMAN:
		{
			if (pTFWeapon)
			{
				// TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY
				if (pTFWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER)
				{
					CTFPipebombLauncher* pMineLauncher = static_cast<CTFPipebombLauncher*>(pTFWeapon);
					if (pMineLauncher->IsMineForAchievementJumpAndDestroy(pInflictor))
					{
						//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY);
					}
				}

				// TF2C_ACHIEVEMENT_TNT_AIRBORNE_KILL
				if (pTFWeapon->GetItemID() == 44)
				{
					CTFBaseGrenade* pBaseGrenade = dynamic_cast<CTFBaseGrenade*>(pInflictor);
					if (pBaseGrenade)
					{
						if (pBaseGrenade->GetWeaponID() == TF_WEAPON_GRENADE_MIRV)
						{
							CTFGrenadeMirvProjectile* pGrenade = static_cast<CTFGrenadeMirvProjectile*>(pBaseGrenade);
							if (pGrenade->m_bAchievementMainPackAirborne)
							{
								//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_TNT_AIRBORNE_KILL);
							}
						}
						else if (pBaseGrenade->GetWeaponID() == TF_WEAPON_GRENADE_MIRVBOMB)
						{
							CTFGrenadeMirvBomb* pGrenade = static_cast<CTFGrenadeMirvBomb*>(pBaseGrenade);
							if (pGrenade->m_bAchievementMainPackAirborne)
							{
								//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_TNT_AIRBORNE_KILL);
							}
						}
					}
				}
			}
			break;
		}
		case TF_CLASS_HEAVYWEAPONS:
		{
			if (pTFWeapon)
			{
				// TF2C_ACHIEVEMENT_AA_VS_AIRBORNE
				if (pTFWeapon->GetWeaponID() == TF_WEAPON_AAGUN)
				{
					/*float flAirborneMinDistance = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFWeapon, flAirborneMinDistance, mini_crit_airborne_simple_min_height);
					if (IsAirborne(flAirborneMinDistance))*/
					if (bWasAirborne && info.GetCritType() == CTakeDamageInfo::CRIT_MINI)
					{
						pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_AA_VS_AIRBORNE);
					}
				}

				if (bWasHoldingMeleeWeapon)
				{
					// TF2C_ACHIEVEMENT_FISTS_MELEE_DUEL
					if (pTFWeapon->GetItemID() == 5 && TFGameRules() && !TFGameRules()->IsInMedievalMode())
					{
						pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_FISTS_MELEE_DUEL);
					}
					// TF2C_ACHIEVEMENT_CHEKHOV_CLOSE_MELEE_KILLS
					/*CTFWeaponBase* pAttackerMeleeWeapon = pTFAttacker->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_MELEE);
					if (pTFAttacker->IsAlive() && pAttackerMeleeWeapon && pAttackerMeleeWeapon->GetItemID() == 49) //check for alive in case of delayed AA shot
					{
						// oh so there is a simple vector distance operation, why did I even write more complex stuff from sentry code
						if (pTFAttacker->GetAbsOrigin().DistTo(GetAbsOrigin()) <= ACHIEVEMENT_TF2C_CHEKHOV_CLOSE_MELEE_KILLS_DISTANCE)
						{
							pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_CHEKHOV_CLOSE_MELEE_KILLS);
						}
					}*/
				}
			}
			break;
		}
		case TF_CLASS_SPY:
		{
			// TF2C_ACHIEVEMENT_DISGUISE_MASTERY
			// Thank gosh that knife calls for 'weapon fired' only after the instant smack call in swing function
			if (info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB && pTFAttacker->m_Shared.IsDisguised() &&
				pTFAttacker->m_Shared.GetDisguiseClass() >= TF_FIRST_NORMAL_CLASS && pTFAttacker->m_Shared.GetDisguiseClass() <= TF_LAST_NORMAL_CLASS)
			{
				IGameEvent* event = gameeventmanager->CreateEvent("disguise_mastery_achievement");
				if (event)
				{
					event->SetInt("attacker", pTFAttacker->GetUserID());
					event->SetInt("disguiseclass", pTFAttacker->m_Shared.GetDisguiseClass());

					gameeventmanager->FireEvent(event);
				}
			}
			// TF2C_KILL_CIVILIAN_DISGUISEBOOST
			// This might as well be client-side in achievement system
			if (IsVIP())
			{
				if (pTFAttacker->m_Shared.GetConditionProvider(TF_COND_DAMAGE_BOOST) == this ||
					pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF) == this) // TF_COND_CIV_DEFENSEBUFF might like it here if its ever to be used
				{
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_KILL_CIVILIAN_DISGUISEBOOST);
				}
			}
			break;
		}
		}

		switch (GetPlayerClass()->GetClassIndex())
		{
		case TF_CLASS_MEDIC:
		{
			// TF2C_ACHIEVEMENT_POP_THE_MEDIC
			if (bMedigunWasFullyCharged)
			{
				//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_POP_THE_MEDIC);
			}
			break;
		}
		case TF_CLASS_HEAVYWEAPONS:
		{
			// TF2C_ACHIEVEMENT_MINIGUN_DUEL
			if (bWasAiming && pTFWeapon && pTFWeapon->GetItemID() == 15)
			{
				pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_MINIGUN_DUEL);
			}
			break;
		}
		}

		// It's server-side Tranq Achievements time!
		if (bWasInTranqCond)
		{
			// Melee crits!
			if (pTFWeapon && pTFWeapon->IsMeleeWeapon() && !IsDOTDmg(info))
			{
				if (pTFAttacker->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY)
				{
					// TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL);
				}
				else
				{
					// TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_AS_TEAMMATE
					//if (info.GetCritType() == CTakeDamageInfo::CRIT_TRANQ)
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_AS_TEAMMATE);
				}

				// TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_TEAMMATE_ASSIST
				if (pTFTranqProvider && pTFAttacker != pTFTranqProvider)
				{
					//pTFTranqProvider->AwardAchievement(TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_TEAMMATE_ASSIST);
				}
			}

			// TF2C_ACHIEVEMENT_TRANQ_TEAMMATE_ASSIST
			if (pTFTranqProvider && pTFTranqProvider != pTFAttacker)
			{
				//pTFTranqProvider->AwardAchievement(TF2C_ACHIEVEMENT_TRANQ_TEAMMATE_ASSIST);
			}
		}

		if (bWasBlastOrJumppadJumping)
		{
			// TF2C_ACHIEVEMENT_AIR_COMBAT
			if (bWasAirborneForAirCombatHeight)
			{
				if ((pTFAttacker->m_Shared.InCond(TF_COND_LAUNCHED) || pTFAttacker->m_Shared.InCond(TF_COND_BLASTJUMPING)) \
					&& pTFAttacker->IsAirborne(ACHIEVEMENT_TF2C_AIR_COMBAT_HEIGHT))
				{
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_AIR_COMBAT);
				}
			}
			//TF2C_ACHIEVEMENT_HEADSHOT_KILL_MIDFLIGHT
			if (bWasAirborneHeadshotKillMidflightHeight)
			{
				if (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT)
				{
					pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_HEADSHOT_KILL_MIDFLIGHT);
				}
			}
		}

		// TF2C_ACHIEVEMENT_DOMINATE_DEVELOPER
		/*if (PlayerIsCredited())
		{
			pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_DOMINATE_DEVELOPER);
		}*/
	}
	else
	{
		switch (GetPlayerClass()->GetClassIndex())
		{
		case TF_CLASS_DEMOMAN:
		{
			/*
			// Old achievement
			// TF2C_KILL_DEMO_WITH_OWNMIRV
			CTFBaseGrenade *pBaseGrenade = dynamic_cast<CTFBaseGrenade *>( pInflictor );
			if ( pBaseGrenade )
			{
			if ( pBaseGrenade->GetWeaponID() == TF_WEAPON_GRENADE_MIRV ||
			pBaseGrenade->GetWeaponID() == TF_WEAPON_GRENADE_MIRVBOMB )
			{
			CTFPlayer *pAchievingPlayer = ToTFPlayer( pBaseGrenade->GetDeflectedBy() );
			if ( pAchievingPlayer )
			{
			pAchievingPlayer->AwardAchievement( TF2C_ACHIEVEMENT_KILL_DEMO_WITH_OWNMIRV );
			}
			}
			}*/
			break;
		}
		}
	}
	// For the Entrepreneur Achievement.
	m_bHasDied = true;
}

//-----------------------------------------------------------------------------
// Purpose: Put any cleanups that should happen when player either dies or is force swithed to spec here.
//-----------------------------------------------------------------------------
void CTFPlayer::CleanupOnDeath( bool bDropItems /*= false*/ )
{
	if ( !IsAlive() )
		return;

	if ( bDropItems )
	{
		// Drop a pack with their leftover ammo.
		DropAmmoPack();

		if ( TFGameRules()->IsInMedievalMode() )
		{
			DropHealthPack();
		}
	}

	// Remove all conditions.
	m_Shared.RemoveAllCond();

	m_Shared.RemoveAllRemainingAfterburn();

	// Remove all items.
	RemoveAllItems( true );

	// Reset all weapons.
	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if ( pActiveWeapon )
	{
		pActiveWeapon->SendWeaponAnim( ACT_VM_IDLE );
		pActiveWeapon->Holster();
		SetActiveWeapon( NULL );
	}

	int iNumWeapons = WeaponCount();
	CTFWeaponBase *pWeapon;
	for ( int iWeapon = 0; iWeapon < iNumWeapons; ++iWeapon )
	{
		pWeapon = GetTFWeapon( iWeapon );
		if ( pWeapon )
		{
			pWeapon->WeaponReset();
		}
	}

	ClearZoomOwner();

	m_Shared.ResetStoredCrits();

	m_Shared.RemovePlayerKillCount();

	// Trigger this here instead of in CMultiplayRules::PlayerKilled
	// as custom maps may use it to reset e.g. parented entities.
	FireTargets( "game_playerdie", this, this, USE_TOGGLE, 0 );
}


bool CTFPlayer::Event_Gibbed( const CTakeDamageInfo &info )
{
	// CTFRagdoll takes care of gibbing.
	return false;
}


bool CTFPlayer::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	if ( CanBecomeRagdoll() )
	{
		VPhysicsDestroyObject();
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_nRenderFX = kRenderFxRagdoll;

		ClampRagdollForce( forceVector, &m_vecForce.GetForModify() );

		SetParent( NULL );

		AddFlag( FL_TRANSRAGDOLL );

		SetMoveType( MOVETYPE_NONE );

		return true;
	}

	return false;
}


bool CTFPlayer::PlayDeathAnimation( const CTakeDamageInfo &info, CTakeDamageInfo &info_modified )
{
	if ( SelectWeightedSequence( ACT_DIESIMPLE ) == -1 )
		return false;

	// Get the attacking player.
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
		return false;

	bool bPlayDeathAnim = false;

	// Check for a sniper headshot.
	if ( pAttacker->GetPlayerClass()->IsClass( TF_CLASS_SNIPER ) && ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT ) )
	{
		bPlayDeathAnim = true;
	}
	// Check for a spy backstab.
	else if ( pAttacker->GetPlayerClass()->IsClass( TF_CLASS_SPY ) && ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB ) )
	{
		bPlayDeathAnim = true;
	}

	// Play death animation?
	if ( bPlayDeathAnim )
	{
		info_modified.SetDamageType( info_modified.GetDamageType() | DMG_REMOVENORAGDOLL | DMG_PREVENT_PHYSICS_FORCE );

		SetAbsVelocity( vec3_origin );
		DoAnimationEvent( PLAYERANIMEVENT_DIE );

		// No ragdoll yet.
		if ( m_hRagdoll.Get() )
		{
			UTIL_Remove( m_hRagdoll );
		}
	}

	return bPlayDeathAnim;
}

ConVar tf_debug_weapondrop( "tf_debug_weapondrop", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//			&vecOrigin - 
//			&vecAngles - 
//-----------------------------------------------------------------------------
bool CTFPlayer::CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles )
{
	// Look up the hand and weapon bones.
	int iHandBone = LookupBone( "weapon_bone" );
	if ( iHandBone == -1 )
		return false;

	GetBonePosition( iHandBone, vecOrigin, vecAngles );

	// Draw the position and angles.
	Vector vecDebugForward2, vecDebugRight2, vecDebugUp2;
	AngleVectors( vecAngles, &vecDebugForward2, &vecDebugRight2, &vecDebugUp2 );

	if ( tf_debug_weapondrop.GetBool() )
	{
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugForward2 * 25.0f ), 255, 0, 0, false, 10.0f );
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugRight2 * 25.0f ), 0, 255, 0, false, 10.0f );
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugUp2 * 25.0f ), 0, 0, 255, false, 10.0f );
	}

	VectorAngles( vecDebugUp2, vecAngles );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: If we don't let players drop ammo boxes, we don't need this code..
//-----------------------------------------------------------------------------
void CTFPlayer::AmmoPackCleanUp( void )
{
	// If we have more than 3 ammo packs out now, destroy the oldest one.
	int iNumPacks = 0;
	CTFAmmoPack *pOldestBox = NULL;

	// Cycle through all ammobox in the world and remove them.
	CBaseEntity *pOwner;
	for ( CTFAmmoPack *pAmmoPack : CTFAmmoPack::AutoList() )
	{
		pOwner = pAmmoPack->GetOwnerEntity();
		if ( pOwner == this )
		{
			iNumPacks++;

			// Find the oldest one.
			if ( !pOldestBox || pOldestBox->GetCreationTime() > pAmmoPack->GetCreationTime() )
			{
				pOldestBox = pAmmoPack;
			}
		}
	}

	// If they have more than 3 packs active, remove the oldest one.
	if ( iNumPacks > 3 && pOldestBox )
	{
		UTIL_Remove( pOldestBox );
	}
}


void CTFPlayer::DropAmmoPack( void )
{
	// Since weapon is hidden in loser state don't drop ammo pack.
	if ( m_Shared.IsLoser() )
		return;

	// We want the ammo packs to look like the player's weapon model they were carrying.
	// except if they are melee or building weapons.
	CTFWeaponBase *pWeapon = NULL;
	CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( !pActiveWeapon || pActiveWeapon->GetTFWpnData().m_bDontDrop )
	{
		// Don't drop this one, find another one to drop.
		int iWeight = -1;

		// Find the highest weighted weapon.
		CTFWeaponBase *pFoundWeapon;
		for ( int i = 0, c = WeaponCount(); i < c; i++ )
		{
			pFoundWeapon = GetTFWeapon( i );
			if ( !pFoundWeapon )
				continue;

			if ( pFoundWeapon->GetTFWpnData().m_bDontDrop )
				continue;

			int iThisWeight = pFoundWeapon->GetTFWpnData().iWeight;
			if ( iThisWeight > iWeight )
			{
				iWeight = iThisWeight;
				pWeapon = pFoundWeapon;
			}
		}
	}
	else
	{
		pWeapon = pActiveWeapon;
	}

	// If we didn't find one, bail.
	if ( !pWeapon )
		return;

	// Find the position and angle of the weapons so the "ammo box" matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if ( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
		return;

	// Fill the ammo pack with unused player ammo, if out add a minimum amount.
	int iPrimary = Max( 5, GetAmmoCount( TF_AMMO_PRIMARY ) );
	int iSecondary = Max( 5, GetAmmoCount( TF_AMMO_SECONDARY ) );
	int iMetal = Max( 5, GetAmmoCount( TF_AMMO_METAL ) );

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, vecPackAngles, this, pWeapon, 0.5f );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		// Remove all of the players ammo.
		RemoveAllAmmo();

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( iPrimary, TF_AMMO_PRIMARY );
		pAmmoPack->GiveAmmo( iSecondary, TF_AMMO_SECONDARY );
		pAmmoPack->GiveAmmo( iMetal, TF_AMMO_METAL );

		IPhysicsObject *pPhysObj = pAmmoPack->VPhysicsGetObject();
		if ( pPhysObj )
		{
			// All weapons must have same weight.
			pPhysObj->SetMass( 25.0f );

			Vector vecRight, vecUp;
			AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );

			// Calculate the initial impulse on the weapon.
			Vector vecImpulse( 0.0f, 0.0f, 0.0f );

			vecImpulse += vecUp * RandomFloat( -0.25, 0.25 );
			vecImpulse += vecRight * RandomFloat( -0.25, 0.25 );
			VectorNormalize( vecImpulse );
			vecImpulse *= RandomFloat( tf_weapon_ragdoll_velocity_min.GetFloat(), tf_weapon_ragdoll_velocity_max.GetFloat() );
			vecImpulse += GetAbsVelocity() + m_vecTotalBulletForce * pPhysObj->GetInvMass();

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_weapon_ragdoll_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_weapon_ragdoll_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			AngularImpulse angImpulse( 0, RandomFloat( 0, 100 ), 0 );
			pPhysObj->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
		}

		pAmmoPack->m_nSkin = GetTeamSkin( pWeapon->GetTeamNumber() );
		pAmmoPack->m_nBody = pWeapon->m_nBody;
		
		if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) && RandomFloat( 0.0f, 1.0f ) < tf2c_pumpkin_loot_drop_rate.GetFloat() )
		{
			pAmmoPack->SetPumpKinLoot();
		}

		// Clean up old ammo packs if they exist in the world.
		AmmoPackCleanUp();	
	}
}


void CTFPlayer::DropHealthPack( void )
{
	CHealthKit *pHealth = static_cast<CHealthKit *>( CBaseEntity::CreateNoSpawn( "item_healthkit_small", WorldSpaceCenter(), vec3_angle ) );
	if ( pHealth )
	{
		// Throw a health pack in a random direction.
		Vector vecVelocity;
		QAngle angDir;
		angDir[PITCH] = RandomFloat( -20.0f, -90.0f );
		angDir[YAW] = RandomFloat( -180.0f, 180.0f );
		AngleVectors( angDir, &vecVelocity );
		vecVelocity *= 250.0f;

		pHealth->DropSingleInstance( vecVelocity, NULL, 0.0f, 0.1f );
	}
}

void CTFPlayer::DropTeamHealthPack( int iTeamOfKiller )
{
	if (iTeamOfKiller && iTeamOfKiller != GetTeamNumber())
	{
		CHealthKit* pHealth = static_cast<CHealthKit*>(CBaseEntity::CreateNoSpawn("item_healthkit_small", WorldSpaceCenter(), vec3_angle));
		if (pHealth)
		{
			// Throw a health pack in a random direction.
			Vector vecVelocity;
			QAngle angDir;
			angDir[PITCH] = RandomFloat(-20.0f, -90.0f);
			angDir[YAW] = RandomFloat(-180.0f, 180.0f);
			AngleVectors(angDir, &vecVelocity);
			vecVelocity *= 250.0f;

			pHealth->ChangeTeam(iTeamOfKiller);
			pHealth->m_nSkin = GetTeamSkin(iTeamOfKiller);
			pHealth->DropSingleInstance(vecVelocity, NULL, 0.0f, 0.1f);

			if ( TFGameRules()->IsInArenaMode() )
			{
				TFGameRules()->m_hArenaDroppedTeamHealthKits.AddToTail(pHealth);
			}
		}
	}
}

void CTFPlayer::PlayerDeathThink( void )
{

}


int CTFPlayer::GetMaxHealthForBuffing( void )
{
	return 0;
}


int CTFPlayer::GetMaxHealth( void )
{
	int iModMaxHealth = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( this, iModMaxHealth, add_maxhealth );
	return BaseClass::GetMaxHealth() + iModMaxHealth;
}

//-----------------------------------------------------------------------------
// Purpose: Remove the tf items from the player then call into the base class
// removal of items.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems( bool removeSuit )
{
	// If the player has a capture flag, drop it.
	if ( HasItem() )
	{
		CCaptureFlag *pFlag = GetTheFlag();

		GetItem()->Drop( this, true );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
			event->SetInt( "team", pFlag ? pFlag->GetTeamNumber() : 0 );
			event->SetInt( "priority", 8 );
			gameeventmanager->FireEvent( event );
		}
	}

	if ( m_hOffHandWeapon.Get() )
	{ 
		HolsterOffHandWeapon();

		// Hide the weapon model.
		// Don't normally have to do this, unless we have a holster animation.
		CTFViewModel *pViewModel = static_cast<CTFViewModel *>( GetViewModel( 1 ) );
		if ( pViewModel )
		{
			pViewModel->SetWeaponModel( NULL, NULL );
		}

		m_hOffHandWeapon = NULL;
	}

	Weapon_SetLast( NULL );
	UpdateClientData();
}


void CTFPlayer::RemoveAllWeapons( void )
{
	int i, c;

	CTFWeaponBase *pWeapon;
	for ( i = 0, c = WeaponCount(); i < c; i++ )
	{
		pWeapon = GetTFWeapon( i );
		if ( !pWeapon )
			continue;

		pWeapon->UnEquip();
	}

	// Remove all wearables.
	CEconWearable *pWearable;
	for ( i = GetNumWearables() - 1; i >= 0; i-- )
	{
		pWearable = GetWearable( i );
		if ( !pWearable )
			continue;

		RemoveWearable( pWearable );
	}
}


void CTFPlayer::AddCustomAttribute( char const *pszAttribute, float flValue, float flDuration )
{
	if ( flDuration > 0 )
		flDuration += gpGlobals->curtime;

	uint16 nIndex = m_mapCustomAttributes.Find( pszAttribute );
	if ( nIndex == m_mapCustomAttributes.InvalidIndex() )
		m_mapCustomAttributes.Insert( pszAttribute, flDuration );
	else
		m_mapCustomAttributes[nIndex] = flDuration;

	m_Shared.AddAttributeToPlayer( pszAttribute, flValue );
}

void CTFPlayer::RemoveCustomAttribute( char const *pszAttribute )
{
	uint16 nIndex = m_mapCustomAttributes.Find( pszAttribute );
	if ( nIndex != m_mapCustomAttributes.InvalidIndex() )
	{
		m_Shared.RemoveAttributeFromPlayer( pszAttribute );
		m_mapCustomAttributes.RemoveAt( nIndex );
	}
}

void CTFPlayer::RemoveAllCustomAttributes()
{
	FOR_EACH_MAP_FAST( m_mapCustomAttributes, i )
	{
		m_Shared.RemoveAttributeFromPlayer( m_mapCustomAttributes.Key( i ) );
	}

	m_mapCustomAttributes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Mapmaker input to force this player to speak a response rules concept.
//-----------------------------------------------------------------------------
void CTFPlayer::InputSetForcedTauntCam( inputdata_t &inputdata )
{
	m_nForceTauntCam = clamp( inputdata.value.Int(), 0, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Ignite a player.
//-----------------------------------------------------------------------------
void CTFPlayer::InputIgnitePlayer( inputdata_t &inputdata )
{
	m_Shared.Burn( ToTFPlayer( inputdata.pActivator ), NULL );
}


void CTFPlayer::InputSetCustomModel( inputdata_t &inputdata )
{
	m_PlayerClass.SetCustomModel( inputdata.value.String() );
	UpdateModel();
}


void CTFPlayer::InputSetCustomModelRotation( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	QAngle angTmp(vecTmp.x, vecTmp.y, vecTmp.z);
	m_PlayerClass.SetCustomModelRotation( angTmp );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}


void CTFPlayer::InputClearCustomModelRotation( inputdata_t &inputdata )
{
	m_PlayerClass.ClearCustomModelRotation();
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}


void CTFPlayer::InputSetCustomModelOffset( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	m_PlayerClass.SetCustomModelOffset( vecTmp );
	InvalidatePhysicsRecursive( POSITION_CHANGED );
}


void CTFPlayer::InputSetCustomModelRotates( inputdata_t &inputdata )
{
	m_PlayerClass.SetCustomModelRotates( inputdata.value.Bool() );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}


void CTFPlayer::InputSetCustomModelVisibleToSelf( inputdata_t &inputdata )
{
	m_PlayerClass.SetCustomModelVisibleToSelf( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: Extinguish a player.
//-----------------------------------------------------------------------------
void CTFPlayer::InputExtinguishPlayer( inputdata_t &inputdata )
{
	if ( m_Shared.InCond( TF_COND_BURNING ) )
	{
		EmitSound( "TFPlayer.FlameOut" );
		m_Shared.RemoveCond( TF_COND_BURNING );
	}
}


void CTFPlayer::InputBleedPlayer( inputdata_t &inputdata )
{
	m_Shared.MakeBleed( this, GetActiveTFWeapon(), inputdata.value.Float() );
}


void CTFPlayer::UpdateModel( void )
{
	SetModel( GetPlayerClass()->GetModelName() );

	// Immediately reset our collision bounds - our collision bounds will be set to the model's bounds.
	SetCollisionBounds( GetPlayerMins(), GetPlayerMaxs() );

	m_PlayerAnimState->OnNewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSkin - 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateSkin( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			m_nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			m_nSkin = 1;
			break;
		case TF_TEAM_GREEN:
			m_nSkin = 4;
			break;
		case TF_TEAM_YELLOW:
			m_nSkin = 5;
			break;
		default:
			m_nSkin = 0;
			break;
	}
}

void CTFPlayer::SetCustomModel( char const *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel );
	UpdateModel();
}

void CTFPlayer::SetCustomModelWithClassAnimations( char const *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel, USE_CLASS_ANIMATIONS );
	UpdateModel();
}

void CTFPlayer::SetCustomModelOffset( Vector const &vecOffs )
{
	m_PlayerClass.SetCustomModelOffset( vecOffs );
	InvalidatePhysicsRecursive( POSITION_CHANGED );
}

void CTFPlayer::SetCustomModelRotation( QAngle const &angRot )
{
	m_PlayerClass.SetCustomModelRotation( angRot );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::ClearCustomModelRotation( void )
{
	m_PlayerClass.ClearCustomModelRotation();
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::SetCustomModelRotates( bool bSet )
{
	m_PlayerClass.SetCustomModelRotates( bSet );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::SetCustomModelVisibleToSelf( bool bSet )
{
	m_PlayerClass.SetCustomModelVisibleToSelf( bSet );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the player disconnects from the server.
//-----------------------------------------------------------------------------
void CTFPlayer::TeamFortress_ClientDisconnected( void )
{
	if ( IsAlive() )
	{
		DispatchParticleEffect( "player_poof", GetAbsOrigin(), vec3_angle );

		// Playing the sound from world since the player is about to be removed.
		CPASAttenuationFilter filter( WorldSpaceCenter(), "TFPlayer.Disconnect" );
		EmitSound( filter, 0, "TFPlayer.Disconnect", &WorldSpaceCenter() );
	}

	CleanupOnDeath( true );
	RemoveAllOwnedEntitiesFromWorld( false );
	RemoveNemesisRelationships();
	RemoveAllWeapons();
	StopTaunt();
	TFGameRules()->RemovePlayerFromQueue( this );
	
	if ( IsVIP() )
	{
		// Assigned a new VIP.
		TFGameRules()->OnVIPLeft( this );
	}

	// Stop players from dodging votekick. If they disconnect before it concludes, they're guilty
	if ( g_voteController && g_voteController->IsPlayerBeingKicked( this ) )
	{
		CKickIssue *pKickIssue = dynamic_cast< CKickIssue* >( g_voteController->GetActiveVoteIssue() );
		if ( pKickIssue )
		{
			pKickIssue->ExecuteCommand();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes everything this player has (buildings, grenades, etc.) from the world.
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayer::RemoveAllOwnedEntitiesFromWorld( bool bSilent /* = true */ )
{
	RemoveOwnedProjectiles();
	
	// Destroy any buildables - this should replace TeamFortress_RemoveBuildings.
	RemoveAllObjects( bSilent );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all projectiles player has fired into the world.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveOwnedProjectiles( void )
{
	for ( CBaseProjectile *pProjectile : CBaseProjectile::AutoList() )
	{
		// If the player owns this entity, remove it.
		if ( pProjectile->GetOwnerEntity() == this )
		{
			pProjectile->SetThink( &CBaseEntity::SUB_Remove );
			pProjectile->SetNextThink( gpGlobals->curtime );
			pProjectile->SetTouch( NULL );
			pProjectile->AddEffects( EF_NODRAW );
		}
	}
}


void CTFPlayer::NoteWeaponFired( CTFWeaponBase *pWeapon )
{
	Assert( m_pCurrentCommand );
	if ( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}

	// Remember the tickcount when the weapon was fired and lock viewangles here!
	if ( m_iLockViewanglesTickNumber != gpGlobals->tickcount )
	{
		LockViewAngles();
	}

	// In case we use projectiles for some reason, set the minicrit to false
	// So that we don't minicrit after a miss
	m_bNextAttackIsMinicrit = false;
	bool bWasDisguised = m_Shared.IsDisguised();

	int iDisguisedMiniCrit = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iDisguisedMiniCrit, minicrit_while_disguised );

	int iKeepDisguise = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iKeepDisguise, set_keep_disguise );
	if( iKeepDisguise <= 0 )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iDisguisedMiniCrit, minicrit_on_disguise_removed );
		RemoveDisguise();
		m_Shared.AddCond(TF_COND_DID_DISGUISE_BREAKING_ACTION, DISGUISE_BREAKING_ACTION_STATE_DURATION);
	}

	if( iDisguisedMiniCrit && bWasDisguised )
		m_bNextAttackIsMinicrit = true;

	if ( !pWeapon->IsMinigun() )
	{
		SpeakWeaponFire();
	}

	CTF_GameStats.Event_PlayerFiredWeapon( this, pWeapon->IsCurrentAttackACrit() );
}

//=============================================================================
//
// Player state functions.
//


CPlayerStateInfo *CTFPlayer::StateLookupInfo( int nState )
{
	// This table MUST match the 
	static CPlayerStateInfo playerStateInfos[] =
	{
		{ TF_STATE_ACTIVE,				"TF_STATE_ACTIVE",				&CTFPlayer::StateEnterACTIVE,				NULL,	NULL },
		{ TF_STATE_WELCOME,				"TF_STATE_WELCOME",				&CTFPlayer::StateEnterWELCOME,				NULL,	&CTFPlayer::StateThinkWELCOME },
		{ TF_STATE_OBSERVER,			"TF_STATE_OBSERVER",			&CTFPlayer::StateEnterOBSERVER,				NULL,	&CTFPlayer::StateThinkOBSERVER },
		{ TF_STATE_DYING,				"TF_STATE_DYING",				&CTFPlayer::StateEnterDYING,				NULL,	&CTFPlayer::StateThinkDYING },
	};

	for ( int i = 0, c = ARRAYSIZE( playerStateInfos ); i < c; ++i )
	{
		if ( playerStateInfos[i].m_nPlayerState == nState )
			return &playerStateInfos[i];
	}

	return NULL;
}


void CTFPlayer::StateEnter( int nState )
{
	m_Shared.m_nPlayerState = nState;
	m_pStateInfo = StateLookupInfo( nState );

	if ( tf_playerstatetransitions.GetInt() == -1 || tf_playerstatetransitions.GetInt() == entindex() )
	{
		if ( m_pStateInfo )
		{
			Msg( "ShowStateTransitions: entering '%s'\n", m_pStateInfo->m_pStateName );
		}
		else
		{
			Msg( "ShowStateTransitions: entering #%d\n", nState );
		}
	}

	// Initialize the new state.
	if ( m_pStateInfo && m_pStateInfo->pfnEnterState )
	{
		(this->*m_pStateInfo->pfnEnterState)();
	}
}


void CTFPlayer::StateLeave( void )
{
	if ( m_pStateInfo && m_pStateInfo->pfnLeaveState )
	{
		(this->*m_pStateInfo->pfnLeaveState)();
	}
}


void CTFPlayer::StateTransition( int nState )
{
	StateLeave();
	StateEnter( nState );
}


void CTFPlayer::StateEnterWELCOME( void )
{
	PickWelcomeObserverPoint();  
	
	StartObserverMode( OBS_MODE_FIXED );

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );		

	PhysObjectSleep();

	if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( ( TFGameRules() && TFGameRules()->IsLoadingBugBaitReport() ) )
	{
		m_bSeenRoundInfo = true;
		
		ChangeTeam( TF_TEAM_BLUE );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
	else if ( IsInCommentaryMode() )
	{
		m_bSeenRoundInfo = true;
	}
	else
	{
		KeyValues *data = new KeyValues( "data" );
		// info panel title
		if ( TFGameRules()->IsBirthday() )
		{
			data->SetString( "title", "#TF_Welcome_birthday" );
		}
		else if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
		{
			data->SetString( "title", "#TF_Welcome_halloween" );
		}
		else if ( TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
		{
			data->SetString("title", "#TF_Welcome_christmas");
		}
		else
		{
			data->SetString( "title", "#TF_Welcome" );
		}
		data->SetString( "type", "1" );	// Show userdata from stringtable entry.
		data->SetString( "msg",	"motd" ); // Use this stringtable entry.
		data->SetString( "cmd", "mapinfo" ); // Exec this command if panel closed.

		ShowViewPortPanel( PANEL_INFO, true, data );

		data->deleteThis();

		m_bSeenRoundInfo = false;
	}

	m_bIsIdle = false;
}


void CTFPlayer::StateThinkWELCOME( void )
{
	if ( IsInCommentaryMode() && !IsFakeClient() )
	{
		ChangeTeam( TF_TEAM_BLUE );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
}


void CTFPlayer::StateEnterACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveEffects( EF_NODRAW | EF_NOSHADOW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	m_flLastAction = gpGlobals->curtime;
	m_flLastRegenTime = gpGlobals->curtime;
	m_bIsIdle = false;

	// Start regenning once we're created.
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );
}


bool CTFPlayer::SetObserverMode( int mode )
{
	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;

	// Skip OBS_MODE_POI as we're not using that.
	if ( mode == OBS_MODE_POI )
	{
		mode++;
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if( GetTeamNumber() > TEAM_SPECTATOR )
	{
		if ( IsDead() && ( mode > OBS_MODE_FIXED ) && mp_fadetoblack.GetBool() )
		{
			mode = OBS_MODE_CHASE;
		}
		else if ( mode == OBS_MODE_ROAMING )
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	m_flLastAction = gpGlobals->curtime;

	switch ( mode )
	{
		case OBS_MODE_NONE:
		case OBS_MODE_FIXED:
		case OBS_MODE_DEATHCAM:
			SetFOV( this, 0 ); // Reset FOV.
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			break;

		case OBS_MODE_CHASE:
		case OBS_MODE_IN_EYE:	
			// Update FOV and viewmodels.
			SetObserverTarget( m_hObserverTarget );	
			SetMoveType( MOVETYPE_OBSERVER );
			break;

		case OBS_MODE_ROAMING:
			SetFOV( this, 0 ); // Reset FOV.
			SetObserverTarget( m_hObserverTarget );
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_OBSERVER );
			break;
		
		case OBS_MODE_FREEZECAM:
			SetFOV( this, 0 ); // Reset FOV.
			SetObserverTarget( m_hObserverTarget );
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_OBSERVER );
			break;
	}

	CheckObserverSettings();

	return true;	
}


void CTFPlayer::StateEnterOBSERVER( void )
{
	// Always start a spectator session in chase mode.
	m_iObserverLastMode = OBS_MODE_CHASE;

	if ( !m_hObserverTarget )
	{
		// Find a new observer target.
		CheckObserverSettings();
	}

	if ( !m_bAbortFreezeCam )
	{
		FindInitialObserverTarget();
	}

	StartObserverMode( m_iObserverLastMode );

	PhysObjectSleep();

	m_bIsIdle = false;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		HandleFadeToBlack();
	}
}


void CTFPlayer::StateThinkOBSERVER()
{
	// Make sure nobody has changed any of our state.
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}


void CTFPlayer::StateEnterDYING( void )
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlayedFreezeCamSound = false;
	m_bAbortFreezeCam = false;
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to observer mode once the dying process is over.
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkDYING( void )
{
	// If we have a ragdoll, it's time to go to deathcam.
	if ( !m_bAbortFreezeCam && m_hRagdoll && 
		( m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD ) && 
		GetObserverMode() != OBS_MODE_FREEZECAM )
	{
		if ( GetObserverMode() != OBS_MODE_DEATHCAM )
		{
			StartObserverMode( OBS_MODE_DEATHCAM );	// Go to observer mode.
		}

		RemoveEffects( EF_NODRAW | EF_NOSHADOW ); // Still draw player body.
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = ( m_flDeathTime + TF_DEATH_ANIMATION_TIME + flTimeInFreeze );
	if ( !m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this )
	{
		// Start the sound so that it ends at the freezecam lock on time.
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if ( gpGlobals->curtime >= flFreezeSoundTime )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound( filter, entindex(), "TFPlayer.FreezeCam" );

			m_bPlayedFreezeCamSound = true;
		}
	}

	if ( gpGlobals->curtime >= ( m_flDeathTime + TF_DEATH_ANIMATION_TIME ) ) // Allow x seconds death animation/death cam.
	{
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			if ( !m_bAbortFreezeCam && gpGlobals->curtime < flFreezeEnd )
			{
				if ( GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					StartObserverMode( OBS_MODE_FREEZECAM );
					PhysObjectSleep();
				}
				return;
			}
		}

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack).
			if ( m_bAbortFreezeCam && !mp_fadetoblack.GetBool() )
			{
				if ( !m_hObserverTarget )
				{
					// Find a new observer target.
					CheckObserverSettings();
				}

				FindInitialObserverTarget();
				SetObserverMode( OBS_MODE_CHASE );
				ShowViewPortPanel( "specgui", ModeWantsSpectatorGUI( OBS_MODE_CHASE ) );
			}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if ( gpGlobals->curtime < flFreezeEnd )
			return;

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		AddEffects( EF_NOINTERP );

		if ( GetMoveType() != MOVETYPE_NONE && ( GetFlags() & FL_ONGROUND ) )
		{
			SetMoveType( MOVETYPE_NONE );
		}

		StateTransition( TF_STATE_OBSERVER );
	}
}


void CTFPlayer::AttemptToExitFreezeCam( void )
{
	float flFreezeTravelTime = ( m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() + 0.5f;
	if ( gpGlobals->curtime < flFreezeTravelTime )
		return;

	m_bAbortFreezeCam = true;
}


int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource ammosource )
{
	if ( iCount <= 0 )
		return 0;

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
		return 0;

	if ( iAmmoIndex == TF_AMMO_METAL )
	{
		if ( ammosource != TF_AMMO_SOURCE_RESUPPLY )
		{
			CALL_ATTRIB_HOOK_INT( iCount, mult_metal_pickup );
		}
	}
	/*else if ( CALL_ATTRIB_HOOK_INT( bBool, ammo_becomes_health ) == 1 )
	{
	if ( !ammosource )
	{
	v7 = (*(int (__cdecl **)(CBaseEntity *, float, _DWORD))(*(_DWORD *)a3 + 260))(a3, (float)iCount, 0);
	if ( v7 > 0 )
	{
	if ( !bSuppressSound )
	EmitSound( "BaseCombatCharacter.AmmoPickup" );

	*(float *)&a2.m128i_i32[0] = (float)iCount;
	HealthKitPickupEffects( iCount );
	}
	return v7;
	}

	if ( ammosource == TF_AMMO_SOURCE_DISPENSER )
	return v7;
	}*/

	int iMaxAmmo = GetMaxAmmo( iAmmoIndex, true );
	int iAmmoCount = GetAmmoCount( iAmmoIndex );
	int iAdd = Min( iCount, iMaxAmmo - iAmmoCount );
	if ( iAdd < 1 )
		return 0;

	// Ammo pickup sound.
	if ( !bSuppressSound )
	{
		//StopSound( "BaseCombatCharacter.AmmoPickup" );
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	m_iAmmo.Set( iAmmoIndex, m_iAmmo[iAmmoIndex] + iAdd );

	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
	return GiveAmmo( iCount, iAmmoIndex, bSuppressSound, TF_AMMO_SOURCE_AMMOPACK );
}

//-----------------------------------------------------------------------------
// Purpose: Reset player's information and force him to spawn.
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRespawn( void )
{
	if ( GetTeamNumber() < FIRST_GAME_TEAM )
		return;

	int iDesiredClass = GetDesiredPlayerClassIndex();
	if ( iDesiredClass == TF_CLASS_UNDEFINED )
		return;

	CTF_GameStats.Event_PlayerForceRespawn( this );

	m_flSpawnTime = gpGlobals->curtime;

	// The player has selected the Random class or is in Randomizer Mode, let's pick a random one for them.
	if ( iDesiredClass == TF_CLASS_RANDOM || ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_CLASSES ) && !m_bPreserveRandomLoadout && !IsVIP() ) )
	{
		int iTries = 20;

		// Don't let them be the same class twice in a row.
		do
		{
			iDesiredClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_CLASS_COUNT_ALL - 1 );
			iTries--;
		}
		while ( ( iDesiredClass == GetPlayerClass()->GetClassIndex() && !( TFGameRules()->IsVIPMode() && iDesiredClass == TF_CLASS_CIVILIAN ) ) ||
			( iTries > 0 && !TFGameRules()->CanPlayerChooseClass( this, iDesiredClass ) ) );

		// We failed to find a random class.
		if ( iTries <= 0 )
			return;
	}

	SetDesiredPlayerClassIndex( iDesiredClass );

	DropFlag();

	if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
	{
		Assert( iDesiredClass > TF_CLASS_UNDEFINED && iDesiredClass < TF_CLASS_COUNT_ALL );

		// Clean up any Projectiles/Buildings in the world (no explosions).
		RemoveAllOwnedEntitiesFromWorld();

		GetPlayerClass()->Init( iDesiredClass );

		// Forget weapons from last life when changing class.
		ForgetLastWeapons();

		CTF_GameStats.Event_PlayerChangedClass( this );
	}
	else if ( IsAlive() )
	{
		RememberLastWeapons();
	}

	m_Shared.RemoveAllCond();

	m_Shared.RemoveAllRemainingAfterburn();

	RemoveAllItems( true );

	// Reset ground state for airwalk animations.
	SetGroundEntity( NULL );

	// Remove invisibility very quickly.
	m_Shared.FadeInvis( 0.1f );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	StateTransition( TF_STATE_ACTIVE );
	Spawn();
}

void CTFPlayer::ForceRegenerateAndRespawn()
{
	m_bRegenerating = true;
	ForceRespawn();
	m_bRegenerating = false;
}

//-----------------------------------------------------------------------------
// Purpose: This player wants to respawn now that their loadout has changed.
//-----------------------------------------------------------------------------
void CTFPlayer::CheckInstantLoadoutRespawn( void )
{
	// Not have stock weapons.
	if ( tf2c_force_stock_weapons.GetBool() )
		return;

	// Not in randomized weapons mode.
	if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) )
		return;

	// Not able to instantly respawn.
	if ( !CanInstantlyRespawn() )
		return;

	int iGameState = TFGameRules()->State_Get();

	// Not in Arena mode.
	if ( TFGameRules()->IsInArenaMode() && TFGameRules()->State_Get() != GR_STATE_PREROUND )
		return;

	// Not if we're on the losing team.
	if ( iGameState == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() ) 
		return;

	if ( IsVIP() )
		m_pVIPInstantlyRespawned = true;

	// Not if our current class's loadout hasn't changed
	int iClass = GetPlayerClass() ? GetPlayerClass()->GetClassIndex() : TF_CLASS_UNDEFINED;
	if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT_ALL )
	{
		if ( m_Shared.InCond( TF_COND_AIMING ) )
		{
			// If we are in condition TF_COND_AIMING it will be removed during the ForceRespawn() so we need to reset the weapon
			// (which is normally skipped while regenerating)... this only affects the Minigun and the Sniper Rifle.
			CTFWeaponBase *pWeapon = GetActiveTFWeapon();
			if ( pWeapon && ( pWeapon->IsMinigun() || pWeapon->IsSniperRifle() ) )
			{
				pWeapon->WeaponReset();
			}
		}

		if ( IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( GetActiveTFWeapon() );
			if ( pMedigun )
			{
				pMedigun->Lower();
			}
		}

		// We want to use ForceRespawn() here so the player is physically moved back
		// into the spawn room and not just regenerated instantly in the doorway
		ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CTFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTFPlayer::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 81:
			GiveEconItem( "Cubemaps" );
			break;
		case 101:
			if ( sv_cheats->GetBool() )
			{
				Regenerate();
				Restock( true, true );
				ITFHealingWeapon *pMedigun = GetMedigun();
				if ( pMedigun )
				{
					pMedigun->AddCharge( 1.0f );
				}

				CTFUmbrella *pUmbrella = static_cast<CTFUmbrella *>( Weapon_OwnsThisID( TF_WEAPON_UMBRELLA ) );
				if ( pUmbrella )
				{
					pUmbrella->WeaponReset();
				}
			}
			break;
		default:
			BaseClass::CheatImpulseCommands( iImpulse );
			break;
	}
}


void CTFPlayer::RemoveBuildResources( int iAmount )
{
	RemoveAmmo( iAmount, TF_AMMO_METAL );
}


void CTFPlayer::AddBuildResources( int iAmount )
{
	GiveAmmo( iAmount, TF_AMMO_METAL );	
}


CBaseObject	*CTFPlayer::GetObject( int index )
{
	return (CBaseObject *)( m_aObjects[index].Get() );
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
CBaseObject *CTFPlayer::GetObjectOfType( int iObjectType, int iObjectMode /*= 0*/ )
{
	CBaseObject *pObject;
	FOR_EACH_VEC( m_aObjects, i )
	{
		pObject = GetObject( i );
		if ( !pObject )
			continue;

		if ( pObject->GetType() != iObjectType )
			continue;

		if ( pObject->GetObjectMode() != iObjectMode )
			continue;

		if ( pObject->IsDisposableBuilding() )
			continue;

		return pObject;
	}

	return NULL;
}


int	CTFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all the player's objects.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllObjects( bool bSilent )
{
	// Remove all the player's objects.
	CBaseObject *pObject;
	for ( int i = GetObjectCount() - 1; i >= 0; i-- )
	{
		pObject = GetObject( i );
		Assert( pObject );
		if ( pObject )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
			if ( event )
			{
				event->SetInt( "userid", GetUserID() );
				event->SetInt( "objecttype", pObject->GetType() );
				event->SetInt( "index", pObject->entindex() );
				gameeventmanager->FireEvent( event );
			}

			bSilent ? UTIL_Remove( pObject ) : pObject->DetonateObject();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Player has started building an object.
//-----------------------------------------------------------------------------
int	CTFPlayer::StartedBuildingObject( int iObjectType )
{
	// Deduct the cost of the object.
	// Player must have lost resources since he started placing.
	int iCost = CTFPlayerShared::CalculateObjectCost( this, iObjectType );
	if ( iCost > GetBuildResources() )
		return 0;

	RemoveBuildResources( iCost );

	// If the object costs 0, we need to return non-0 to mean success.
	if ( !iCost )
		return 1;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Object has been built by this player.
//-----------------------------------------------------------------------------
void CTFPlayer::FinishedObject( CBaseObject *pObject )
{
	AddObject( pObject );
	CTF_GameStats.Event_PlayerCreatedBuilding( this, pObject );
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this player's object list.
//-----------------------------------------------------------------------------
void CTFPlayer::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::AddObject adding object %p:%s to player %s\n", gpGlobals->curtime, pObject, pObject->GetClassname(), GetPlayerName() ) );

	// Make a handle out of it.
	CHandle<CBaseObject> hObject;
	hObject = pObject;

	bool bAlreadyInList = PlayerOwnsObject( pObject );
	Assert( !bAlreadyInList );
	if ( !bAlreadyInList )
	{
		m_aObjects.AddToTail( hObject );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Object built by this player has been destroyed.
//-----------------------------------------------------------------------------
void CTFPlayer::OwnedObjectDestroyed( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::OwnedObjectDestroyed player %s object %p:%s\n", gpGlobals->curtime, 
		GetPlayerName(),
		pObject,
		pObject->GetClassname() ) );

	RemoveObject( pObject );
}

//-----------------------------------------------------------------------------
// Purpose: Removes an object from the player.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::RemoveObject %p:%s from player %s\n", gpGlobals->curtime, 
		pObject,
		pObject->GetClassname(),
		GetPlayerName() ) );

	Assert( pObject );

	// TODO: Was just 'm_aObjects.Count()', check why that was.
	FOR_EACH_VEC_BACK( m_aObjects, i ) 
	{
		// Also, while we're at it, remove all other bogus ones too...
		if ( !m_aObjects[i].Get() || m_aObjects[i] == pObject )
		{
			m_aObjects.FastRemove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if the player owns this object.
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerOwnsObject( CBaseObject *pObject )
{
	return ( m_aObjects.Find( pObject ) != -1 );
}


void CTFPlayer::PlayFlinch( const CTakeDamageInfo &info )
{
	// Don't play flinches if we just died. 
	if ( !IsAlive() )
		return;

	// No pain flinches while disguised and hurt by our enemy, our man has supreme discipline.
	if ( m_Shared.IsDisguised() )
	{
		CBaseEntity *pAttacker = info.GetAttacker();
		if ( pAttacker && m_Shared.DisguiseFoolsTeam( pAttacker->GetTeamNumber() ) )
			return;
	}

	PlayerAnimEvent_t flinchEvent;

	switch ( LastHitGroup() )
	{
		// Pick a region-specific flinch.
		case HITGROUP_HEAD:
			flinchEvent = PLAYERANIMEVENT_FLINCH_HEAD;
			break;
		case HITGROUP_LEFTARM:
			flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTARM;
			break;
		case HITGROUP_RIGHTARM:
			flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTARM;
			break;
		case HITGROUP_LEFTLEG:
			flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTLEG;
			break;
		case HITGROUP_RIGHTLEG:
			flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
			break;
		case HITGROUP_STOMACH:
		case HITGROUP_CHEST:
		case HITGROUP_GEAR:
		case HITGROUP_GENERIC:
		default:
			// Just get a generic flinch.
			flinchEvent = PLAYERANIMEVENT_FLINCH_CHEST;
			break;
	}

	DoAnimationEvent( flinchEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the crit sound that players that get crit hear
//-----------------------------------------------------------------------------
void CTFPlayer::PlayCritReceivedSound( void )
{
	// Play a custom pain sound to the guy taking the damage.
	CSingleUserRecipientFilter receiverfilter( this );
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "TFPlayer.CritPain";
	EmitSound( receiverfilter, entindex(), params );
}


void CTFPlayer::PainSound( const CTakeDamageInfo &info )
{
	// Don't make sounds if we just died, DeathSound will handle that.
	if ( !IsAlive() )
		return;

	if ( m_flNextPainSoundTime > gpGlobals->curtime )
		return;

	// No sound for DMG_GENERIC!
	if ( info.GetDamageType() == DMG_GENERIC || info.GetDamageType() == DMG_PREVENT_PHYSICS_FORCE )
		return;

	// Looping fire pain sound is done in CTFPlayerShared::ConditionThink.
	if ( info.GetDamageType() & DMG_BURN )
		return;

	// TF2C Don't interrupt voice lines with common pain grunts
	if ( IsSpeaking() )
	{
		if ( info.GetDamageType() & DMG_CRITICAL )
		{
			PlayCritReceivedSound();
			m_flNextPainSoundTime = gpGlobals->curtime + 0.3f;
		}
		return;
	}

	int iSilentKillerNoSound = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), iSilentKillerNoSound, set_silent_killer_no_pain_scream);
	if (iSilentKillerNoSound)
		return;

	if ( info.GetDamageType() & DMG_DROWN )
	{
		EmitSound( "TFPlayer.Drown" );
		return;
	}

	// No yelping while disguised and hurt by our enemy, our man has supreme discipline.
	if ( m_Shared.IsDisguised() )
	{
		CBaseEntity *pAttacker = info.GetAttacker();
		if ( pAttacker && m_Shared.DisguiseFoolsTeam( pAttacker->GetTeamNumber() ) )
			return;
	}

	float flPainLength = 0.0f;

	bool bAttackerIsPlayer = ( info.GetAttacker() && info.GetAttacker()->IsPlayer() );

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );

	pExpresser->AllowMultipleScenes();

	// Speak a pain concept here, send to everyone but the attacker.
	CPASFilter filter( GetAbsOrigin() );
	if ( bAttackerIsPlayer )
	{
		filter.RemoveRecipient( ToBasePlayer( info.GetAttacker() ) );
	}

	// Play a crit sound to the victim (me).
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		PlayCritReceivedSound();
		// Don't play our own pain voice line to ourselves over the crit pain sound
		filter.RemoveRecipient( this );
	}

	char szResponse[AI_Response::MAX_RESPONSE_NAME];
	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &filter ) )
	{
		flPainLength = GetSceneDuration( szResponse );
	}

	// Speak a louder pain concept to just the attacker.
	if ( bAttackerIsPlayer )
	{
		CSingleUserRecipientFilter attackerFilter( ToBasePlayer( info.GetAttacker() ) );
		SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_ATTACKER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &attackerFilter );
	}

	pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = ( info.GetDamageType() & DMG_CRITICAL ) ? 0.3f : ( gpGlobals->curtime + flPainLength );
}


void CTFPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Don't make death sounds when choosing a class.
	if ( IsPlayerClass( TF_CLASS_UNDEFINED, true ) )
		return;

	TFPlayerClassData_t *pData = GetPlayerClass()->GetData();

	// VIP announces his death to everyone.
	if ( IsVIP() )
	{
		CBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), "TFPlayer.VIPDeath" );
	}

	if (!(m_LastDamageType & DMG_CRITICAL) && info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING)
		return;	// the scream for non-crit burning will be handled client-wise due to possibility of special burning animation

	int iSilentKillerNoScream = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), iSilentKillerNoScream, set_silent_killer_no_scream);
	if (iSilentKillerNoScream)
		return;

	if ( m_LastDamageType & DMG_FALL ) // Did we die from falling?
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else if ( m_LastDamageType & DMG_BLAST )
	{
		EmitSound( pData->m_szExplosionDeathSound );
	}
	else if ( m_LastDamageType & DMG_CRITICAL )
	{
		EmitSound( pData->m_szCritDeathSound );

		PlayCritReceivedSound();
	}
	else if ( m_LastDamageType & ( DMG_SLASH | DMG_CLUB ) )
	{
		EmitSound( pData->m_szMeleeDeathSound );
	}
	else if (info.GetDamageCustom() == TF_DMG_EARTHQUAKE)
	{
		EmitSound(pData->m_szMeleeDeathSound);
	}
	else if (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT)
	{
		EmitSound( pData->m_szCritDeathSound );
	}
	else
	{
		EmitSound( pData->m_szDeathSound );
	}
}


void CTFPlayer::StunSound( CTFPlayer *pAttacker, int iStunFlags, int iOldStunFlags /*= 0*/ )
{
	if ( !IsAlive() )
		return;

	if ( !( iStunFlags & TF_STUN_CONTROLS ) && !( iStunFlags & TF_STUN_LOSER_STATE ) )
		return;

	if ( ( iStunFlags & TF_STUN_BY_TRIGGER ) && ( iOldStunFlags != 0 ) )
		return; // Only play stun triggered sounds when not already stunned.

	// Play the stun sound for everyone but the attacker.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );

	pExpresser->AllowMultipleScenes();

	float flStunSoundLength = 0.0f;

	EmitSound_t params;
	params.m_flSoundTime = 0;
	if ( iStunFlags & TF_STUN_SPECIAL_SOUND )
	{
		params.m_pSoundName = "TFPlayer.StunImpactRange";
	}
	else if ( ( iStunFlags & TF_STUN_LOSER_STATE ) && !pAttacker )
	{
		params.m_pSoundName = "Halloween.PlayerScream";
	}
	else
	{
		params.m_pSoundName = "TFPlayer.StunImpact";
	}
	params.m_pflSoundDuration = &flStunSoundLength;

	if ( pAttacker )
	{
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( pAttacker );
		EmitSound( filter, entindex(), params );

		// Play a louder pain sound for the person who got the stun.
		CSingleUserRecipientFilter attackerFilter( pAttacker );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( params.m_pSoundName );
	}

	pExpresser->DisallowMultipleScenes();

	// Suppress any pain sound that might come right after this stun sound.
	m_flNextPainSoundTime = gpGlobals->curtime + 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: called when this player burns another player
//-----------------------------------------------------------------------------
void CTFPlayer::OnBurnOther( CTFPlayer *pTFPlayerVictim )
{
	// UNDONE: Not used by any TF2C achievements.
	/*
#define ACHIEVEMENT_BURN_TIME_WINDOW	30.0f
#define ACHIEVEMENT_BURN_VICTIMS	5
	// Add current time we burned another player to head of vector
	m_aBurnOtherTimes.AddToHead( gpGlobals->curtime );

	// Remove any burn times that are older than the burn window from the list
	float flTimeDiscard = gpGlobals->curtime - ACHIEVEMENT_BURN_TIME_WINDOW;
	for ( int i = 1; i < m_aBurnOtherTimes.Count(); i++ )
	{
		if ( m_aBurnOtherTimes[i] < flTimeDiscard )
		{
			m_aBurnOtherTimes.RemoveMultiple( i, m_aBurnOtherTimes.Count() - i );
			break;
		}
	}

	// See if we've burned enough players in time window to satisfy achievement
	if ( m_aBurnOtherTimes.Count() >= ACHIEVEMENT_BURN_VICTIMS )
	{
		CSingleUserRecipientFilter filter( this );
		UserMessageBegin( filter, "AchievementEvent" );
			WRITE_BYTE( ACHIEVEMENT_TF_BURN_PLAYERSINMINIMIMTIME );
		MessageEnd();
	}
	*/
}

void CTFPlayer::IgnitePlayer()
{
	const char *pszMapName = STRING( gpGlobals->mapname );
	if ( FStrEq( "sd_doomsday", pszMapName ) )
	{
		m_Shared.Burn( this );
		return;
	}

	/*CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager(this, 0, 5.0);
	if ( pRecentDamager )
	{
		if ( pRecentDamager->GetTeamNumber() != GetTeamNumber() )
			pRecentDamager->AwardAchievement( 2410 );
	}*/

	m_Shared.Burn( this );
}

void CTFPlayer::ExtinguishPlayerBurning()
{
	if ( m_Shared.InCond( TF_COND_BURNING ) )
	{
		EmitSound( "TFPlayer.FlameOut" );
		m_Shared.RemoveCond( TF_COND_BURNING );
	}
}

void CTFPlayer::BleedPlayer( float flDuration )
{
	m_Shared.MakeBleed( this, GetActiveTFWeapon(), flDuration );
}

void CTFPlayer::BleedPlayerEx( float flDuration, int nBleedDamage, bool bPermanent, int )
{
	m_Shared.MakeBleed( this, GetActiveTFWeapon(), flDuration, nBleedDamage, bPermanent/*, */ );
}


CTFTeam *CTFPlayer::GetTFTeam( void ) const
{
	return assert_cast<CTFTeam *>( GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Give this player the "i just teleported" effect for 12 seconds
//-----------------------------------------------------------------------------
void CTFPlayer::TeleportEffect( int iTeam )
{
	m_Shared.SetTeleporterEffectColor( iTeam );
	m_Shared.AddCond( TF_COND_TELEPORTED, 12.0f );
}

void CTFPlayer::RemoveTeleportEffect()
{
	m_Shared.RemoveCond( TF_COND_TELEPORTED );
}


void CTFPlayer::PlayerUse( void )
{
	if ( tf_allow_player_use.GetBool() || IsInCommentaryMode() )
		BaseClass::PlayerUse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyAbsVelocityImpulse( Vector const &vecImpulse )
{
	if ( m_Shared.InCond( TF_COND_MEGAHEAL ) )
		return;
	
	Vector vecModImpulse = vecImpulse;
	float flImpulseScale = 1.0f;
	if ( m_Shared.InCond( TF_COND_AIMING ) )
		CALL_ATTRIB_HOOK_FLOAT( flImpulseScale, mult_aiming_knockback_resistance );

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) )
		flImpulseScale *= 2.0f;

	if ( m_Shared.IsParachuting() )
	{
		vecModImpulse.x *= 1.5f;
		vecModImpulse.y *= 1.5f;
	}

	BaseClass::ApplyAbsVelocityImpulse( vecModImpulse * flImpulseScale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ApplyPunchImpulseX( float flPunch )
{
	CTFWeaponBase* pActive = GetActiveTFWeapon();
	if ( !pActive )
	{
		m_Local.m_vecPunchAngle.SetX( flPunch );
		return true;
	}

	if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
	{
		if ( pActive->GetWeaponID() == TF_WEAPON_SNIPERRIFLE && static_cast<CTFSniperRifle *>( pActive )->IsFullyCharged() )
		{
			int nAimingNoFlinch = 0;
			CALL_ATTRIB_HOOK_INT( nAimingNoFlinch, aiming_no_flinch );
			if ( nAimingNoFlinch > 0 )
				return false;
		}
	}

	if ( m_Shared.InCond( TF_COND_AIMING ) )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActive, flPunch, mult_flinch_resist_aiming );
	}
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActive, flPunch, mult_flinch_resist_active );
	CALL_ATTRIB_HOOK_FLOAT( flPunch, mult_flinch_resist_wearer );

	m_Local.m_vecPunchAngle.SetX( flPunch );
	return true;
}


void CTFPlayer::CreateRagdollEntity( void )
{
	CreateRagdollEntity( 0, 0.0f, TF_DMG_CUSTOM_NONE, DMG_GENERIC );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity to pass to the client.
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( int nFlags, float flInvisLevel, ETFDmgCustom iDamageCustom, int bitsDamageType )
{
	// If we already have a ragdoll destroy it.
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll *>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTFRagdoll *>( CreateEntityByName( "tf_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->m_nForceBone = m_nForceBone;
		Assert( entindex() >= 1 && entindex() <= MAX_PLAYERS );
		pRagdoll->SetOwnerEntity( this );
		pRagdoll->m_iRagdollFlags = nFlags;
		pRagdoll->m_flInvisibilityLevel = flInvisLevel;
		pRagdoll->m_iDamageCustom = iDamageCustom;
		pRagdoll->m_bitsDamageType = bitsDamageType;
		pRagdoll->m_iTeam = GetTeamNumber();
		pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
	}

	// Turn off the player.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );
	SetMoveType( MOVETYPE_NONE );

	// Add additional gib setup.
	if ( nFlags & TF_RAGDOLL_GIB )
	{
		EmitSound( "BaseCombatCharacter.CorpseGib" ); // Squish!
		m_nRenderFX = kRenderFxRagdoll;
	}

	// Save ragdoll handle.
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy's a ragdoll, called with a player is disconnecting.
//-----------------------------------------------------------------------------
void CTFPlayer::DestroyRagdoll( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}


void CTFPlayer::StopRagdollDeathAnim( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		pRagdoll->m_iDamageCustom = TF_DMG_CUSTOM_NONE;
	}
}


void CTFPlayer::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
#ifdef ITEM_TAUNTING
		case AE_TAUNT_ENABLE_MOVE:
			m_bAllowMoveDuringTaunt = true;
			return;
		case AE_TAUNT_DISABLE_MOVE:
			m_bAllowMoveDuringTaunt = false;
			return;
#endif
		case AE_WPN_HIDE:
		case AE_WPN_UNHIDE:
		case AE_WPN_PLAYWPNSOUND:
			// These are supposed to be processed on client-side only, swallow...
			return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}


void CTFPlayer::Weapon_FrameUpdate( void )
{
	BaseClass::Weapon_FrameUpdate();

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		m_hOffHandWeapon->Operator_FrameUpdate( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::Weapon_HandleAnimEvent( pEvent );

	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) 
{
	
}

//-----------------------------------------------------------------------------
// Purpose: Remove invisibility, called when player attacks
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveInvisibility( void )
{
	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		// Remove quickly.
		m_Shared.RemoveCond( TF_COND_STEALTHED );
		m_Shared.FadeInvis( 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove disguise
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveDisguise( void )
{
	// Remove quickly.
	if ( m_Shared.IsDisguised() || m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		m_Shared.RemoveDisguise();
	}
}


void CTFPlayer::SaveMe( void )
{
	if ( !IsAlive() || IsPlayerClass( TF_CLASS_UNDEFINED, true ) || GetTeamNumber() < TF_TEAM_RED )
		return;

	m_bSaveMeParity = !m_bSaveMeParity;
}

//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
CON_COMMAND( dropitem, "Drop the flag." )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer )
	{
		pPlayer->DropFlag();
	}
}

class CObserverPoint : public CPointEntity
{
	DECLARE_CLASS( CObserverPoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual void Activate( void )
	{
		BaseClass::Activate();

		if ( m_iszAssociateTeamEntityName != NULL_STRING )
		{
			m_hAssociatedTeamEntity = gEntList.FindEntityByName( NULL, m_iszAssociateTeamEntityName );
			if ( !m_hAssociatedTeamEntity )
			{
				Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName) );
			}
		}
	}

	bool CanUseObserverPoint( CTFPlayer *pPlayer )
	{
		if ( m_bDisabled )
			return false;

		if ( m_hAssociatedTeamEntity && ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM ) )
		{
			// If we don't own the associated team entity, we can't use this point
			if ( m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
				return false;
		}

		// Only spectate observer points on control points in the current miniround
		if ( g_pObjectiveResource->PlayingMiniRounds() && m_hAssociatedTeamEntity )
		{
			CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>(m_hAssociatedTeamEntity.Get());
			if ( pPoint )
			{
				bool bInRound = g_pObjectiveResource->IsInMiniRound( pPoint->GetPointIndex() );
				if ( !bInRound )
					return false;
			}
		}

		return true;
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void InputEnable( inputdata_t &inputdata )
	{
		m_bDisabled = false;
	}
	void InputDisable( inputdata_t &inputdata )
	{
		m_bDisabled = true;
	}
	bool IsDefaultWelcome( void ) { return m_bDefaultWelcome; }

public:
	bool		m_bDisabled;
	bool		m_bDefaultWelcome;
	EHANDLE		m_hAssociatedTeamEntity;
	string_t	m_iszAssociateTeamEntityName;
	float		m_flFOV;
};

BEGIN_DATADESC( CObserverPoint )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome" ),
	DEFINE_KEYFIELD( m_iszAssociateTeamEntityName,	FIELD_STRING,	"associated_team_entity" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_observer_point, CObserverPoint );
LINK_ENTITY_TO_CLASS( game_intro_viewpoint, CPointEntity ); // For compatibility.

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
// Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CTFPlayer::BuildObservableEntityList( void )
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Check if an override is set.
	if ( TFGameRules()->GetRequiredObserverTarget() )
		return m_hObservableEntities.AddToTail( TFGameRules()->GetRequiredObserverTarget() );

	// Add all the map-placed observer points.
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		m_hObservableEntities.AddToTail( pObserverPoint );

		if ( m_hObserverTarget.Get() == pObserverPoint )
		{
			iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
		}

		pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}

	// Add all the players.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pObserverPoint = UTIL_PlayerByIndex( i );
		if ( pObserverPoint )
		{
			m_hObservableEntities.AddToTail( pObserverPoint );

			if ( m_hObserverTarget.Get() == pObserverPoint )
			{
				iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
			}
		}
	}

	// Add all my objects.
	int iNumObjects = GetObjectCount();
	for ( int i = 0; i < iNumObjects; i++ )
	{
		pObserverPoint = GetObject( i );
		if ( pObserverPoint )
		{
			m_hObservableEntities.AddToTail( pObserverPoint );

			if ( m_hObserverTarget.Get() == pObserverPoint )
			{
				iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
			}
		}
	}

	// If there are any team_train_watchers, add the train they are linked to.
	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->IsDisabled() )
		{
			CBaseEntity *pTrain = pWatcher->GetTrainEntity();
			if ( pTrain )
			{
				m_hObservableEntities.AddToTail( pTrain );

				if ( m_hObserverTarget.Get() == pTrain )
				{
					iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
				}
			}
		}		

		pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	return iCurrentIndex;
}


int CTFPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1;
	int iStartIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count() - 1;

	iStartIndex += iDir;
	if ( iStartIndex > iMax )
		iStartIndex = 0;
	else if ( iStartIndex < 0 )
		iStartIndex = iMax;

	return iStartIndex;
}


CBaseEntity *CTFPlayer::FindNextObserverTarget( bool bReverse )
{
	int iStartIndex = GetNextObserverSearchStartPoint( bReverse );

	int	iIndex = iStartIndex;
	int iDir = bReverse ? -1 : 1;

	int iMax = m_hObservableEntities.Count() - 1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if ( iStartIndex > iMax )
	{
		iIndex = iStartIndex = 1;
	}

	do
	{
		CBaseEntity *pNextTarget = m_hObservableEntities[iIndex];
		if ( IsValidObserverTarget( pNextTarget ) )
			return pNextTarget;

		iIndex += iDir;

		// Loop through the entities.
		if ( iIndex > iMax )
		{
			iIndex = 0;
		}
		else if ( iIndex < 0 )
		{
			iIndex = iMax;
		}
	}
	while ( iIndex != iStartIndex );

	return NULL;
}


bool CTFPlayer::IsValidObserverTarget( CBaseEntity *target )
{
	if ( !target || target == this )
		return false;

	if ( TFGameRules()->GetRequiredObserverTarget() && target != TFGameRules()->GetRequiredObserverTarget() )
		return false;

	if ( !target->IsPlayer() )
	{
		// Can only spectate players in Tournament Mode.
		if ( TFGameRules()->IsInTournamentMode() )
			return false;

		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
		if ( pObsPoint && !pObsPoint->CanUseObserverPoint( this ) )
			return false;

		CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( target );
		if ( pTrain )
		{
			// Can only spec the trains while the round is running.
			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
				return false;
		}
	}
	else
	{
		// Don't watch invisible players.
		CTFPlayer *pPlayer = ToTFPlayer( target );
		if ( pPlayer->IsEffectActive( EF_NODRAW ) )
			return false;

		// Don't spectate dead players.
		if ( !pPlayer->IsAlive() )
		{
			// Allow watching until 3 seconds after death to see death animation.
			if ( ( pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
				return false;	
		}
	}

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )
		{
			case OBS_ALLOW_ALL:
				// No team restrictions.
				break;
			case OBS_ALLOW_TEAM:
				if ( TFGameRules()->IsInArenaMode() )
				{
					bool bTeamIsAlive = false;

					FOR_EACH_VEC( GetTeam()->m_aPlayers, i )
					{
						if ( GetTeam()->GetPlayer( i )->IsAlive() )
						{
							bTeamIsAlive = true;
							break;
						}
					}

					if ( !bTeamIsAlive )
						break;
				}

				// Can only spectate teammates and unassigned cameras.
				if ( target->IsPlayer() )
				{
					if ( IsEnemy( target ) )
						return false;
				}
				else
				{
					if ( target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber() )
						return false;
				}

				break;
			case OBS_ALLOW_NONE:
				// Can't spectate anyone.
				return false;
		}
	}

	return true;
}


void CTFPlayer::PickWelcomeObserverPoint( void )
{
	//Don't just spawn at the world origin, find a nice spot to look from while we choose our team and class.
	CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		if ( IsValidObserverTarget( pObserverPoint ) )
		{
			SetObserverTarget( pObserverPoint );
		}

		if ( pObserverPoint->IsDefaultWelcome() )
			break;

		pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}
}


void CTFPlayer::JumptoPosition( const Vector &origin, const QAngle &angles )
{
	// Check if something's up with our QAngles.
	if ( !IsEntityQAngleReasonable( angles ) )
	{
		if ( CheckEmitReasonablePhysicsSpew() )
		{
			Warning( "CTFPlayer::JumptoPosition - Ignoring bogus angles (%f,%f,%f) from JumptoPosition!\n", angles.x, angles.y, angles.z );
		}

		return;
	}
    
	// Check our Position if our QAngle is fine.
	if ( !IsEntityPositionReasonable( origin ) )
	{
		if ( CheckEmitReasonablePhysicsSpew() )
		{
			Warning( "CTFPlayer::JumptoPosition - Ignoring unreasonable position (%f,%f,%f) from JumptoPosition!\n", origin.x, origin.y, origin.z );
		}

		return;
	}

	// All's good.
	BaseClass::JumptoPosition( origin, angles );
}


bool CTFPlayer::SetObserverTarget( CBaseEntity *target )
{
	ClearZoomOwner();
	SetFOV( this, 0 );
		
	if ( !BaseClass::SetObserverTarget( target ) )
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
	if ( pObsPoint )
	{
		SetViewOffset( vec3_origin );
		JumptoPosition( pObsPoint->GetAbsOrigin(), pObsPoint->EyeAngles() );

		if ( m_iObserverMode != OBS_MODE_ROAMING )
		{
			// info_observer_point in TF2 are made with at most 90 FOV in mind.
			// Default of 0 resets to player FOV, which in TF2C can be higher than that.
			// Clamp to 90 to avoid visual issues on some maps.
			SetFOV( pObsPoint, pObsPoint->m_flFOV );
			float fFOV = GetFOV();
			SetFOV( pObsPoint, fFOV > 90.0f ? 90.0f : fFOV );
		}
	}

	m_flLastAction = gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
// Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNearestObservableTarget( Vector vecOrigin, float flMaxDist )
{
	CTeam *pTeam = GetTeam();
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = ( flMaxDist * flMaxDist );

	int i, c;
	CTFPlayer *pPlayer;
	float flDistSqr;
	for ( i = 0, c = pTeam->GetTeamNumber() == TEAM_SPECTATOR ? gpGlobals->maxClients : pTeam->GetNumPlayers(); i < c; i++ )
	{
		if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		}
		else
		{
			pPlayer = ToTFPlayer( pTeam->GetPlayer(i) );
		}

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget( pPlayer ) )
			continue;

		flDistSqr = ( pPlayer->GetAbsOrigin() - vecOrigin ).LengthSqr();
		if ( flDistSqr < flCurDistSqr )
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			if ( !bFoundClass || pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex(), true ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;

				if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex(), true ) )
				{
					bFoundClass = true;
				}
			}
		}
		else if ( !bFoundClass && pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex(), true ) )
		{
			pReturnTarget = pPlayer;
			flCurDistSqr = flDistSqr;
			bFoundClass = true;
		}
	}

	if ( !bFoundClass && IsPlayerClass( TF_CLASS_ENGINEER, true ) )
	{
		// Let's spectate our sentry instead if we didn't find any other Engineers to spectate.
		CBaseObject *pObject;
		for ( i = 0, c = GetObjectCount(); i < c; i++ )
		{
			pObject = GetObject( i );
			if ( pObject && pObject->GetType() == OBJ_SENTRYGUN )
			{
				pReturnTarget = pObject;
			}
		}
	}		

	return pReturnTarget;
}


void CTFPlayer::FindInitialObserverTarget( void )
{
	// If we're on a team (i.e. not a pure observer), try and find
	// a target that'll give the player the most useful information.
	if ( GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			// Has our forward cap point been contested recently?
			int iFarthestPoint = TFGameRules()->GetFarthestOwnedControlPoint( GetTeamNumber(), false );
			if ( iFarthestPoint != -1 )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > ( gpGlobals->curtime - 30 ) )
				{
					// Does it have an associated viewpoint?
					CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
					while ( pObserverPoint )
					{
						CObserverPoint *pObsPoint = assert_cast<CObserverPoint *>( pObserverPoint );
						if ( pObsPoint && pObsPoint->m_hAssociatedTeamEntity == pMaster->GetControlPoint( iFarthestPoint ) )
						{
							if ( IsValidObserverTarget( pObsPoint ) )
							{
								m_hObserverTarget.Set( pObsPoint );
								return;
							}
						}

						pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
					}
				}
			}

			// Has the point beyond our farthest been contested lately?
			iFarthestPoint += ( ObjectiveResource()->GetBaseControlPointForTeam( GetTeamNumber() ) == 0 ? 1 : -1 );
			if ( iFarthestPoint >= 0 && iFarthestPoint < MAX_CONTROL_POINTS )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > ( gpGlobals->curtime - 30 ) )
				{
					// Try and find a player near that cap point
					CBaseEntity *pCapPoint = pMaster->GetControlPoint( iFarthestPoint );
					if ( pCapPoint )
					{
						CBaseEntity *pTarget = FindNearestObservableTarget( pCapPoint->GetAbsOrigin(), 1500 );
						if ( pTarget )
						{
							m_hObserverTarget.Set( pTarget );
							return;
						}
					}
				}
			}
		}
	}

	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget( GetAbsOrigin(), FLT_MAX );
	if ( pTarget )
	{
		m_hObserverTarget.Set( pTarget );
	}
}


void CTFPlayer::ValidateCurrentObserverTarget( void )
{
	// If our current target is a dead player who's gibbed / died, refind as if 
	// we were finding our initial target, so we end up somewhere useful.
	if ( m_hObserverTarget && m_hObserverTarget->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( m_hObserverTarget.Get() );
		if ( !pPlayer->IsAlive() )
		{
			// Once we're past the pause after death, find a new target
			if ( ( pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				FindInitialObserverTarget();
			}

			return;
		}

		if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			if ( m_iObserverMode == OBS_MODE_IN_EYE )
			{
				ForceObserverMode( OBS_MODE_CHASE );
			}
		}
	}

	// Don't spectate buildings or payload carts in first person.
	if ( m_hObserverTarget && !m_hObserverTarget->IsPlayer() )
	{
		if ( m_iObserverMode == OBS_MODE_IN_EYE )
		{
			ForceObserverMode( OBS_MODE_CHASE );
		}
	}
	//DevMsg( "m_hObserverTarget: %s\n", m_hObserverTarget->GetDebugName() );

	BaseClass::ValidateCurrentObserverTarget();
}


void CTFPlayer::CheckObserverSettings()
{
	if ( TFGameRules() )
	{
		// Is there a current entity that is the required spectator target?
		if ( TFGameRules()->GetRequiredObserverTarget() )
		{
			SetObserverTarget( TFGameRules()->GetRequiredObserverTarget() );
			return;
		}
		
		// Make sure we're not trying to spec the train during a team win.
		// If we are, switch to spectating the last control point instead (where the train ended).
		if ( m_hObserverTarget && m_hObserverTarget->IsBaseTrain() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		{
			// find the nearest spectator point to use instead of the train
			CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );
			CObserverPoint *pClosestPoint = NULL;
			float flMinDistance = -1.0f;
			Vector vecTrainOrigin = m_hObserverTarget->GetAbsOrigin();

			while ( pObserverPoint )
			{
				if ( IsValidObserverTarget( pObserverPoint ) )
				{
					float flDist = pObserverPoint->GetAbsOrigin().DistTo( vecTrainOrigin );
					if ( flMinDistance < 0 || flDist < flMinDistance )
					{
						flMinDistance = flDist;
						pClosestPoint = pObserverPoint;
					}
				}

				pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
			}

			if ( pClosestPoint )
			{
				SetObserverTarget( pClosestPoint );
			}
		}
	}

	BaseClass::CheckObserverSettings();
}


void CTFPlayer::Touch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		CheckUncoveringSpies( pPlayer );
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has seen through an enemy spy's disguise
//-----------------------------------------------------------------------------
void CTFPlayer::CheckUncoveringSpies( CTFPlayer *pTouchedPlayer )
{
	// Only uncover enemies.
	if ( !IsEnemy( pTouchedPlayer ) )
		return;

	// Only uncover if they're stealthed.
	if ( !pTouchedPlayer->m_Shared.IsStealthed() )
		return;

	// Pulse their invisibility.
	pTouchedPlayer->m_Shared.OnSpyTouchedByEnemy();
}


bool CTFPlayer::IsAllowedToTaunt( void )
{
	// Already taunting?
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return false;

	// Can't taunt while stunned or being a loser... loser.
	if ( ( !tf2c_stunned_taunting.GetBool() && m_Shared.IsControlStunned() ) || ( tf2c_disable_loser_taunting.GetBool() && m_Shared.IsLoser() ) )
		return false;

	// Check to see if we are in water (above our waist).
	if ( GetWaterLevel() > WL_Waist )
		return false;

	// Check to see if we are on the ground.
	if ( GetGroundEntity() == NULL )
		return false;

	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		// Don't taunt if we have no ammo and we're about to switch
		if( !Weapon_CanSwitchTo( pActiveWeapon ) )
			return false;
		// TODO: If we need it.
		/*if ( !pActiveWeapon->OwnerCanTaunt() )
			return false;*/

		// Ignore taunt key if one of these if active weapon
		if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD 
			 ||	pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_DESTROY )
			return false;
	}

	// Can't taunt while carrying an object.
	if ( IsPlayerClass( TF_CLASS_ENGINEER, true ) && m_Shared.IsCarryingObject() )
		return false;

	// Can't taunt while cloaked.
	if ( m_Shared.IsStealthed() )
		return false;

	// Can't taunt while disguised.
	// Now we can because we just break disguise
	/*if (IsPlayerClass(TF_CLASS_SPY, true))
	{
		if (  m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
			return false;
	}*/

	return true;
}


void CTFPlayer::Taunt( void )
{
	if ( !IsAllowedToTaunt() )
		return;

	RemoveDisguise();
	// This can't be after SpeakConceptIfAllowed, that would fail

	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_bInitTaunt = true;
	char szResponse[AI_Response::MAX_RESPONSE_NAME];
	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_TAUNT, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
	{
		// Setup a taunt attack if necessary.
		CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
		if ( pActiveWeapon && pActiveWeapon->SetupTauntAttack( m_iTauntAttack, m_flTauntAttackTime ) )
		{
			// Let the weapon do all the work.
		}
		else if ( V_stricmp( szResponse, "scenes/player/pyro/low/taunt02.vcd" ) == 0 )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 2.0f;
			m_iTauntAttack = TAUNTATK_PYRO_HADOUKEN;
		}
		else if ( V_stricmp( szResponse, "scenes/player/heavy/low/taunt03_v1.vcd" ) == 0 )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 1.8f;
			m_iTauntAttack = TAUNTATK_HEAVY_HIGH_NOON;
		}
		else if ( V_strnicmp( szResponse, "scenes/player/spy/low/taunt03", 29 ) == 0 )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 1.8f;
			m_iTauntAttack = TAUNTATK_SPY_FENCING_SLASH_A;
		}
		else if ( V_stricmp( szResponse, "scenes/player/sniper/low/taunt04.vcd" ) == 0 )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 0.85f;
			m_iTauntAttack = TAUNTATK_SNIPER_ARROW_STAB_IMPALE;
		}
		else if ( V_stricmp( szResponse, "scenes/player/medic/low/taunt08.vcd" ) == 0 )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 2.2f;
			m_iTauntAttack = TAUNTATK_MEDIC_UBERSLICE_IMPALE;
		}
		else if ( V_stricmp( szResponse, "scenes/player/medic/low/taunt06.vcd" ) == 0 )
		{
			const char *pszEffect = ConstructTeamParticle( "healhuff_%s", GetTeamNumber(), g_aTeamNamesShort );
			DispatchParticleEffect( pszEffect, PATTACH_POINT_FOLLOW, this, "eyes" );

			m_iTauntAttack = TAUNTATK_MEDIC_INHALE;
			m_flTauntAttackTime = gpGlobals->curtime + 0.8f;
			m_flTauntAttackEndTime = gpGlobals->curtime + 1.8f;
		}
		else if ( FStrEq( szResponse, "scenes/player/demoman/low/taunt07_v1.vcd" ) 
			|| FStrEq( szResponse, "scenes/player/demoman/low/taunt07_v2.vcd" )
			|| FStrEq( szResponse, "scenes/player/demoman/low/taunt07_v3.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 3.84f;
			m_iTauntAttack = TAUNTATK_DEMOMAN_MIRV_KILL;
		}

		OnTauntSucceeded( szResponse );
	}

	m_bInitTaunt = false;
	pExpresser->DisallowMultipleScenes();
}


void CTFPlayer::StopTaunt( void )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_Shared.m_flTauntRemoveTime = 0.0f;
		m_hTauntScene = NULL;
	}

#ifdef ITEM_TAUNTING
	if ( m_hTauntProp.Get() )
	{
		UTIL_Remove( m_hTauntProp.Get() );
		m_hTauntProp = NULL;
	}

	StopTauntSoundLoop();
	m_bHoldingTaunt = false;
	m_bIsReadyToHighFive = false;
	m_bAllowMoveDuringTaunt = false;
	m_hTauntPartner = NULL;
#endif

	// Restart idle expression.
	ClearExpression();
}

#ifdef ITEM_TAUNTING
//-----------------------------------------------------------------------------
// Purpose: Called when a taunt is interrupted.
//-----------------------------------------------------------------------------
void CTFPlayer::StopTauntSoundLoop( void )
{
	if ( m_pTauntItem )
	{
		CEconItemDefinition *pItemDef = m_pTauntItem->GetStaticData();

		if ( pItemDef && pItemDef->taunt.taunt_success_sound_loop[0] )
		{
			CBroadcastRecipientFilter filter;
			filter.MakeReliable();

			UserMessageBegin( filter, "PlayerTauntSoundLoopEnd" );
			WRITE_BYTE( entindex() );
			MessageEnd();
		}
	}

	m_bPlayingTauntSound = false;
}


void CTFPlayer::PlayTauntSceneFromItem( CEconItemView *pItem )
{
	if ( !IsAllowedToTaunt() )
		return;

	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return;

	int iClass = m_PlayerClass.GetClassIndex();

	if ( pItemDef->taunt.custom_taunt_scene_per_class[iClass].Count() == 0 )
		return;

	// Get class-specific scene.
	const char *pszScene = pItemDef->taunt.custom_taunt_scene_per_class[iClass].Random();
	if ( !pszScene[0] )
		return;

	// Exclude mimic taunts as mimicking is pointless here.
	bool bHold = pItemDef->taunt.is_hold_taunt;
	bool bPartnerTaunt = ( pItemDef->taunt.is_partner_taunt && !pItemDef->taunt.taunt_mimic );

	// If this a partner taunt, check if there is enough space.
	if ( bPartnerTaunt )
	{
		// Ignore players here, we'll do an additional check later.
		Vector vecTauntPos;
		QAngle vecTauntAngles;

		int iResult = GetTauntPartnerPosition( pItemDef, vecTauntPos, vecTauntAngles, MASK_PLAYERSOLID );

		if ( iResult != TAUNT_PARTNER_OK )
		{
			const char *pszReason = "";

			if ( iResult == TAUNT_PARTNER_BLOCKED )
			{
				pszReason = "#TF_PartnerTaunt_Blocked";
			}
			else if ( iResult == TAUNT_PARTNER_TOO_HIGH )
			{
				pszReason = "#TF_PartnerTaunt_TooHigh";
			}

			CSingleUserRecipientFilter filter( this );
			EmitSound( filter, entindex(), "Player.DenyWeaponSelection" );
			TFGameRules()->SendHudNotification( filter, pszReason, "ico_notify_partner_taunt" );
			return;
		}
	}

	// Team Fortress 2 Classic
	if ( pItemDef->taunt.taunt_force_weapon_slot_per_class[iClass] != TF_LOADOUT_SLOT_INVALID )
	{
		// If taunt wants us to switch to a weapon and we fail then cancel taunt.
		CTFWeaponBase *pWeapon = GetWeaponForLoadoutSlot( pItemDef->taunt.taunt_force_weapon_slot_per_class[iClass] );

		if ( !pWeapon || ( pWeapon != GetActiveTFWeapon() && !Weapon_Switch( pWeapon ) ) )
			return;
	}
	else if ( pItemDef->taunt.taunt_force_weapon_slot != TF_LOADOUT_SLOT_INVALID )
	{
		// If taunt wants us to switch to a weapon and we fail then cancel taunt.
		CTFWeaponBase *pWeapon = GetWeaponForLoadoutSlot( pItemDef->taunt.taunt_force_weapon_slot );

		if ( !pWeapon || ( pWeapon != GetActiveTFWeapon() && !Weapon_Switch( pWeapon ) ) )
			return;
	}

	m_pTauntItem = pItem;

	// Play a sound if there is one.
	if ( pItemDef->taunt.is_partner_taunt && pItemDef->taunt.taunt_success_sound[0] )
		EmitSound( pItemDef->taunt.taunt_success_sound );

	if ( pItemDef->taunt.taunt_success_sound_loop[0] )
	{
		CBroadcastRecipientFilter filter;
		filter.MakeReliable();

		UserMessageBegin( filter, "PlayerTauntSoundLoopStart" );
			WRITE_BYTE( entindex() );
			WRITE_STRING( pItemDef->taunt.taunt_success_sound_loop );
		MessageEnd();

		m_bPlayingTauntSound = true;
	}

	m_flTauntSpeed = pItemDef->taunt.taunt_move_speed;
	m_flTauntTurnSpeed = pItemDef->taunt.taunt_turn_speed;
	m_bTauntForceForward = pItemDef->taunt.taunt_force_move_forward;

	PlayTauntScene( pszScene );
	OnTauntSucceeded( pszScene, bHold, bPartnerTaunt );

	// Set up a taunt attack if needed.
	if ( !pItemDef->taunt.is_hold_taunt )
	{
		int iTauntAttack = pItemDef->taunt.taunt_attack;

		if ( iTauntAttack )
		{
			SetupTauntAttack( iTauntAttack, pItemDef->taunt.taunt_attack_time );
		}
	}

	const char *pszTauntModel = pItemDef->taunt.custom_taunt_prop_per_class[iClass];

	if ( pszTauntModel[0] )
	{
		// If we have a scene for the prop then create a model and play it.
		// Otherwise just re-use the weapon model.
		const char *pszTauntPropScene = pItemDef->taunt.custom_taunt_prop_scene_per_class[iClass];
		
		if ( pszTauntPropScene[0] )
		{
			Vector vecOrigin = GetAbsOrigin();
			QAngle vecAngles( 0, EyeAngles()[YAW], 0 );
			CBaseCombatCharacter *pTauntProp = ToBaseCombatCharacter( CBaseEntity::CreateNoSpawn( "tf_taunt_prop", vecOrigin, vecAngles, this ) );

			if ( pTauntProp )
			{
				pTauntProp->SetModel( pszTauntModel );

				pTauntProp->m_nSkin = GetTeamSkin( GetTeamNumber() );
				DispatchSpawn( pTauntProp );
				pTauntProp->SetSolidFlags( FSOLID_NOT_SOLID );
				pTauntProp->PlayScene( pszTauntPropScene );

				m_hTauntProp = pTauntProp;
			}
		}
		else
		{
			CTFWeaponBase *pWeapon = GetActiveTFWeapon();

			if ( pWeapon )
			{
				pWeapon->UseForTaunt( m_pTauntItem );
			}
		}
	}
}
#endif


void CTFPlayer::OnTauntSucceeded( const char *pszScene, bool bHoldTaunt /*= false*/, bool bPartnerTaunt /*= false*/ )
{
	// Clear disguising state.
	m_Shared.RemoveCond( TF_COND_DISGUISING );

	// Set player state as taunting.
	m_Shared.AddCond( TF_COND_TAUNTING );

#ifdef ITEM_TAUNTING
	// If this a hold taunt the first part loops indefinitely.
	if ( bHoldTaunt )
	{
		m_bHoldingTaunt = true;
		m_bIsReadyToHighFive = bPartnerTaunt;
		m_Shared.m_flTauntRemoveTime = -1.0f;
	}
	else
#endif
	{
		m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + GetSceneDuration( pszScene ) + 0.2f;
	}

	// Slam velocity to zero.
	if ( !tf_allow_sliding_taunt.GetBool() )
	{
		SetAbsVelocity( vec3_origin );
	}

	// Stop idle expressions.
	m_flNextRandomExpressionTime = -1.0f;
}


void CTFPlayer::PlayTauntScene( const char *pszScene )
{
	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_bInitTaunt = true;
	PlayScene( pszScene );

	pExpresser->DisallowMultipleScenes();
}

#ifdef ITEM_TAUNTING

void CTFPlayer::EndLongTaunt( void )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_Shared.m_flTauntRemoveTime = 0.0f;
		m_hTauntScene = NULL;
	}

	m_bHoldingTaunt = false;
	m_bIsReadyToHighFive = false;
	StopTauntSoundLoop();

	// Play outro scene if possible.
	if ( !m_pTauntItem )
		return;

	CEconItemDefinition *pItemDef = m_pTauntItem->GetStaticData();
	if ( !pItemDef )
		return;

	int iClass = m_PlayerClass.GetClassIndex();

	if ( pItemDef->taunt.custom_taunt_outro_scene_per_class[iClass].Count() == 0 )
		return;

	const char *pszScene = pItemDef->taunt.custom_taunt_outro_scene_per_class[iClass].Random();
	PlayTauntScene( pszScene );

	m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + GetSceneDuration( pszScene ) + 0.2f;

	// Play outro for taunt prop if possible.
	CBaseCombatCharacter *pTauntProp = ToBaseCombatCharacter( m_hTauntProp.Get() );

	if ( pTauntProp )
	{
		const char *pszTauntPropScene = pItemDef->taunt.custom_taunt_prop_outro_scene_per_class[iClass];

		if ( pszTauntPropScene[0] )
		{
			pTauntProp->PlayScene( pszTauntPropScene );
		}
	}
}


CTFPlayer *CTFPlayer::FindPartnerTauntInitiator( void )
{
	// Get the player in front of us.
	Vector vecStart, vecDir, vecEnd;
	vecStart = EyePosition();
	AngleVectors( EyeAngles(), &vecDir );
	vecEnd = vecStart + vecDir * tf_highfive_max_range.GetFloat();

	trace_t tr;
	UTIL_TraceLine( vecStart, vecEnd, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &tr );

	CTFPlayer *pPlayer = ToTFPlayer( tr.m_pEnt );
	if ( pPlayer && pPlayer->IsPartnerTauntReady() )
		return pPlayer;

	return NULL;
}


int CTFPlayer::GetTauntPartnerPosition( CEconItemDefinition *pItemDef, Vector &vecPos, QAngle &vecAngles, unsigned int nMask, CTFPlayer *pPartner /*= NULL*/ )
{
	vecAngles.Init( 0.0f, AngleNormalize( EyeAngles()[YAW] - 180.0f ), 0.0f );

	Vector vecForward, vecRight, vecUp;
	QAngle angForward( 0.0f, EyeAngles()[YAW], 0.0f );
	AngleVectors( angForward, &vecForward, &vecRight, &vecUp );

	// First, bring it up to max possible height.
	vecPos = GetAbsOrigin() +
		vecForward * pItemDef->taunt.taunt_separation_forward_distance +
		vecUp * tf_highfive_height_tolerance.GetFloat();

	// Now it drop it down.
	trace_t tr;
	CTFPlayer *pTraceEntity = pPartner ? pPartner : this;
	CTraceFilterObject tracefilter( pTraceEntity, COLLISION_GROUP_PLAYER_MOVEMENT );
	UTIL_TraceEntity( pPartner ? pPartner : this,
		vecPos, vecPos - vecUp * tf_highfive_height_tolerance.GetFloat() * 2.0f,
		nMask, &tracefilter,
		&tr );

	vecPos = tr.endpos;

	if ( tr.startsolid || tr.allsolid )
		return TAUNT_PARTNER_BLOCKED;

	if ( tr.fraction == 1.0f )
		return TAUNT_PARTNER_TOO_HIGH;

	return TAUNT_PARTNER_OK;
}


void CTFPlayer::AcceptTauntWithPartner( CTFPlayer *pPlayer )
{
	if ( !IsAllowedToTaunt() )
		return;

	CEconItemView *pItem = pPlayer->GetTauntItem();
	if ( !pItem )
		return;

	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return;

	Vector vecTauntPos;
	QAngle vecTauntAngles;
	
	if ( pPlayer->GetTauntPartnerPosition( pItemDef, vecTauntPos, vecTauntAngles, PlayerSolidMask(), this ) != TAUNT_PARTNER_OK )
		return;

	// Make sure there's nothing between us and target position.
	Vector vecStart = GetAbsOrigin();
	trace_t tr;
	CTraceFilterObject tracefilter( this, COLLISION_GROUP_PLAYER_MOVEMENT, pPlayer );
	UTIL_TraceEntity( this, vecStart, vecTauntPos, PlayerSolidMask(), &tracefilter, &tr );
	if ( tr.fraction != 1.0f )
	{
		// Try again with tolerance. This should prevent the taunt from being screwed over by small ledges.
		vecStart.z += tf_highfive_height_tolerance.GetFloat();

		UTIL_TraceEntity( this, vecStart, vecTauntPos, PlayerSolidMask(), &tracefilter, &tr );
		if ( tr.fraction != 1.0f )
			return;
	}

	// Select scenes for iniator and receiver.
	CUtlVector<const char *> &aInitScenes = pItemDef->taunt.custom_partner_taunt_initiator_per_class[pPlayer->GetPlayerClass()->GetClassIndex()];
	CUtlVector<const char *> &aRecvScenes = pItemDef->taunt.custom_partner_taunt_receiver_per_class[GetPlayerClass()->GetClassIndex()];

	if ( aInitScenes.Count() != 0 && aRecvScenes.Count() != 0 )
	{
		// Place the receiver in front of the initiator.
		SetAbsOrigin( vecTauntPos );
		SetAbsAngles( vecTauntAngles );
		SetEyeAngles( vecTauntAngles );

		// Stop the waiting loop.
		pPlayer->StopTaunt();
		pPlayer->SetTauntPartner( this );

		int iInitScene = 0, iRecvScene = 0;

		// Randomize either initiator or receiver scene.
		if ( RandomInt( 0, 1 ) == 0 )
		{
			if ( aInitScenes.Count() > 1 )
			{
				iInitScene = RandomInt( 1, aInitScenes.Count() - 1 );
			}
		}
		else
		{
			if ( aRecvScenes.Count() > 1 )
			{
				iRecvScene = RandomInt( 1, aRecvScenes.Count() - 1 );
			}
		}

		pPlayer->PlayTauntScene( aInitScenes[iInitScene] );
		pPlayer->m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + GetSceneDuration( aInitScenes[iInitScene] ) + 0.2f;

		PlayTauntScene( aRecvScenes[iRecvScene] );
		OnTauntSucceeded( aRecvScenes[iRecvScene] );

		// Set up a taunt attack if needed.
		int iTauntAttack = pItemDef->taunt.taunt_attack;

		if ( iTauntAttack )
		{
			pPlayer->SetupTauntAttack( iTauntAttack, pItemDef->taunt.taunt_attack_time );
		}

		// Play a sound if there is one.
		EconItemVisuals *pVisuals = pItemDef->GetVisuals( GetTeamNumber() );
		if ( pVisuals->custom_sound[0][0] != '\0' )
		{
			pPlayer->EmitSound( pVisuals->custom_sound[0] );
		}
	}
}
#endif


void CTFPlayer::DoTauntAttack( void )
{
	int iTauntType = m_iTauntAttack;
	if ( !iTauntType )
		return;

	CDisablePredictionFiltering disabler;
	switch ( iTauntType )
	{
		case TAUNTATK_PYRO_HADOUKEN:
		case TAUNTATK_SPY_FENCING_SLASH_A:
		case TAUNTATK_SPY_FENCING_SLASH_B:
		case TAUNTATK_SPY_FENCING_STAB:
		{
			Vector vecOrigin, vecForward;
			QAngle angForward( 0, EyeAngles()[YAW], 0 );
			AngleVectors( angForward, &vecForward );
			vecOrigin = WorldSpaceCenter() + vecForward * 64;

			QAngle angForce( -45, angForward[YAW], 0 );
			Vector vecForce;
			AngleVectors( angForce, &vecForce );
			float flDamage = 0.0f;
			int nDamageType = DMG_GENERIC;
			int iDamageCustom = 0;

			switch ( iTauntType )
			{
				case TAUNTATK_PYRO_HADOUKEN:
					vecForce *= 25000;
					flDamage = 500;
					nDamageType = DMG_IGNITE;
					iDamageCustom = TF_DMG_CUSTOM_TAUNTATK_HADOUKEN;
					break;
				case TAUNTATK_SPY_FENCING_STAB:
					vecForce *= 20000;
					flDamage = 500;
					nDamageType = DMG_SLASH;
					iDamageCustom = TF_DMG_CUSTOM_TAUNTATK_FENCING;
					break;
				default:
					vecForce *= 100;
					flDamage = 25;
					nDamageType = DMG_SLASH | DMG_PREVENT_PHYSICS_FORCE;
					iDamageCustom = TF_DMG_CUSTOM_TAUNTATK_FENCING;
					break;
			}

			// Spy taunt has 3 hits, set up the next one.
			if ( iTauntType == TAUNTATK_SPY_FENCING_SLASH_A )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 0.47;
				m_iTauntAttack = TAUNTATK_SPY_FENCING_SLASH_B;
			}
			else if ( iTauntType == TAUNTATK_SPY_FENCING_SLASH_B )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 1.73;
				m_iTauntAttack = TAUNTATK_SPY_FENCING_STAB;
			}

			const Vector vecMins( -24, -24, -24 );
			const Vector vecMaxs( 24, 24, 24 );

			if ( tf_debug_damage.GetBool() )
			{
				NDebugOverlay::Box( vecOrigin, vecMins, vecMaxs, 255, 0, 0, 40, 10.0f );
			}

			CBaseEntity *pList[256];
			int count = UTIL_EntitiesInBox( pList, 256, vecOrigin + vecMins, vecOrigin + vecMaxs, FL_CLIENT | FL_OBJECT );

			for ( int i = 0; i < count; i++ )
			{
				CBaseEntity *pEntity = pList[i];
				if ( pEntity == this || !pEntity->IsAlive() || !FVisible( pEntity, MASK_SOLID ) )
					continue;

				Vector vecDamagePos;
				VectorLerp( WorldSpaceCenter(), pEntity->WorldSpaceCenter(), 0.75f, vecDamagePos );

				CTakeDamageInfo info( this, this, GetActiveWeapon(), vecForce, vecDamagePos, flDamage, nDamageType, iDamageCustom );
				pEntity->TakeDamage( info );
			}

			break;
		}
		case TAUNTATK_HEAVY_HIGH_NOON:
		{
			// Fire a bullet in the direction player was looking at.
			Vector vecSrc, vecShotDir, vecEnd;
			QAngle angShot = EyeAngles();
			AngleVectors( angShot, &vecShotDir );
			vecSrc = Weapon_ShootPosition();
			vecEnd = vecSrc + vecShotDir * 500;

			trace_t tr;

			int iIsRussianRoulette = 0;
			CALL_ATTRIB_HOOK_INT(iIsRussianRoulette, mod_russian_roulette_taunt);
			bool bRicochet = iIsRussianRoulette && !RandomInt(0, 5);
			if (bRicochet)
			{
				vecSrc = WorldSpaceCenter() + Vector(RandomFloat(-10.0, 10.0), RandomFloat(-10.0, 10.0), RandomFloat(-10.0, 10.0)); // May not end up hitting ourselves
				
				UTIL_TraceLine(vecSrc, vecEnd, MASK_TFSHOT, this, COLLISION_GROUP_PLAYER, &tr);
				UTIL_TraceLine(tr.endpos, vecSrc, MASK_TFSHOT, tr.m_pEnt, COLLISION_GROUP_PLAYER, &tr);	// Sending trace forward and then back seems overkill, but gotta do it for ragdoll sake
				if (tr.fraction == 1.0f || (tr.fraction < 1.0f && tr.m_pEnt != this))
				{
					vecSrc = WorldSpaceCenter();
					UTIL_TraceLine(vecSrc, vecEnd, MASK_TFSHOT, this, COLLISION_GROUP_PLAYER, &tr);
					UTIL_TraceLine(tr.endpos, vecSrc, MASK_TFSHOT, tr.m_pEnt, COLLISION_GROUP_PLAYER, &tr);
				}
			}
			else
			{
				if (tf2c_bullets_pass_teammates.GetBool())
				{
					CTraceFilterIgnoreTeammates filter(this, COLLISION_GROUP_PLAYER, GetTeamNumber());
					UTIL_TraceLine(vecSrc, vecEnd, MASK_TFSHOT, &filter, &tr);
				}
				else
				{
					UTIL_TraceLine(vecSrc, vecEnd, MASK_TFSHOT, this, COLLISION_GROUP_PLAYER, &tr);
				}
			}

			if ( tf_debug_damage.GetBool() )
			{
				NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 10.0f );
			}

			if ( tr.fraction < 1.0f && tr.m_pEnt->IsPlayer() )
			{
				Vector vecForce, vecDamagePos;
				QAngle angForce(-45, angShot[YAW], 0);
				AngleVectors(angForce, &vecForce);
				vecForce *= 25000;
				if (bRicochet)
				{
					vecForce.x = vecForce.x * -1.0f;
					vecForce.y = vecForce.y * -1.0f;
					VectorScale(vecShotDir, -1.0f, vecShotDir);
				}

				vecDamagePos = tr.endpos;

				CTakeDamageInfo info(this, this, GetActiveWeapon(), vecForce, vecDamagePos, 500, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON);
				if (bRicochet)
				{
					info.AddDamageType(DMG_PREVENT_PHYSICS_FORCE);	// It seems your ragdolls will keep a high upward velocity of damage if you set influencer on yourself. 
																	// This is a quick fix to this very unlikely edge case. TODO: Fix the bug(?) without regression.
				}
				tr.m_pEnt->DispatchTraceAttack(info, vecShotDir, &tr);
				ApplyMultiDamage();
			}

			break;
		}
		case TAUNTATK_HEAVY_EAT:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon )
			{
				pActiveWeapon->SetupTauntAttack( m_iTauntAttack, m_flTauntAttackTime );
			}

			break;
		}
		case TAUNTATK_MEDIC_INHALE:
		{
			// Heal 10 HP over 1 second.
			int iHealthRestored = TakeHealth( 1, DMG_GENERIC, NULL, true );
			CTF_GameStats.Event_PlayerHealedOther(this, (float)iHealthRestored);

			if ( gpGlobals->curtime < m_flTauntAttackEndTime )
			{
				m_iTauntAttack = TAUNTATK_MEDIC_INHALE;
				m_flTauntAttackTime = gpGlobals->curtime + 0.1;
			}

			break;
		}
		case TAUNTATK_SNIPER_ARROW_STAB_IMPALE:
		case TAUNTATK_SNIPER_ARROW_STAB_KILL:
		case TAUNTATK_MEDIC_UBERSLICE_IMPALE:
		case TAUNTATK_MEDIC_UBERSLICE_KILL:
		{
			// Trace a bit ahead.
			Vector vecSrc, vecShotDir, vecEnd;
			QAngle angShot = EyeAngles();
			AngleVectors( angShot, &vecShotDir );
			vecSrc = Weapon_ShootPosition();
			vecEnd = vecSrc + vecShotDir * 128;

			trace_t tr;
			if (tf2c_bullets_pass_teammates.GetBool())
			{
				CTraceFilterIgnoreTeammates filter(this, COLLISION_GROUP_PLAYER, GetTeamNumber());
				UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID, &filter, &tr);
			}
			else
			{
				UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID, this, COLLISION_GROUP_PLAYER, &tr);
			}

			if ( tf_debug_damage.GetBool() )
			{
				NDebugOverlay::Line( vecSrc, tr.endpos, 255, 0, 0, true, 10.0f );
			}

			if ( tr.fraction < 1.0f )
			{
				CTFPlayer *pPlayer = ToTFPlayer( tr.m_pEnt );
				if ( pPlayer && IsEnemy( pPlayer ) )
				{
					// First hit stuns, next hit kills.
					bool bStun = ( iTauntType == TAUNTATK_SNIPER_ARROW_STAB_IMPALE || iTauntType == TAUNTATK_MEDIC_UBERSLICE_IMPALE );
					Vector vecForce, vecDamagePos;

					if ( bStun )
					{
						vecForce = vec3_origin;
					}
					else
					{
						// Pull them towards us.
						Vector vecDir = WorldSpaceCenter() - pPlayer->WorldSpaceCenter();
						VectorNormalize( vecDir );
						vecForce = vecDir * 12000;
					}

					float flDamage = bStun ? 1.0f : 500.0f;
					int nDamageType = DMG_SLASH | DMG_PREVENT_PHYSICS_FORCE;
					ETFDmgCustom iCustomDamage = TF_DMG_CUSTOM_NONE;
					if ( iTauntType == TAUNTATK_SNIPER_ARROW_STAB_IMPALE || iTauntType == TAUNTATK_SNIPER_ARROW_STAB_KILL )
					{
						iCustomDamage = TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB;

						CTFCompoundBow *pBow = static_cast<CTFCompoundBow *>( GetActiveWeapon() );
						if ( pBow && pBow->GetArrowAlight() )
						{
							nDamageType |= DMG_IGNITE; // For some reason this alone isn't enough
							pPlayer->m_Shared.Burn( this, dynamic_cast<CTFWeaponBase*>( GetActiveWeapon() ) );
						}
					}
					else if ( iTauntType == TAUNTATK_MEDIC_UBERSLICE_IMPALE || iTauntType == TAUNTATK_MEDIC_UBERSLICE_KILL )
					{
						iCustomDamage = TF_DMG_CUSTOM_TAUNTATK_UBERSLICE;
					}

					vecDamagePos = tr.endpos;

					if ( bStun )
					{
						pPlayer->m_Shared.StunPlayer( 3.0f, 0.0f, TF_STUN_BOTH | TF_STUN_NO_EFFECTS, this );
					}
				
					CTakeDamageInfo info( this, this, GetActiveWeapon(), vecForce, vecDamagePos, flDamage, nDamageType, iCustomDamage );
					pPlayer->TakeDamage( info );

					if ( iTauntType == TAUNTATK_MEDIC_UBERSLICE_KILL )
					{
						// A taunt kill will always fill Ubercharge completely.
						ITFHealingWeapon *pMedigun = GetMedigun();
						if ( pMedigun )
						{
							pMedigun->AddCharge( 1.0f - pMedigun->GetChargeLevel() );
						}
					}
				}
			}

			if ( iTauntType == TAUNTATK_SNIPER_ARROW_STAB_IMPALE )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 1.3f;
				m_iTauntAttack = TAUNTATK_SNIPER_ARROW_STAB_KILL;
			}
			else if ( iTauntType == TAUNTATK_MEDIC_UBERSLICE_IMPALE )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 0.75f;
				m_iTauntAttack = TAUNTATK_MEDIC_UBERSLICE_KILL;
			}

			break;
		}
		case TAUNTATK_SOLDIER_GRENADE_KILL:
		case TAUNTATK_DEMOMAN_MIRV_KILL:
		{
			trace_t		tr;
			Vector		vecSpot;// trace starts here!

			SetThink( NULL );

			vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

			// Figure out Econ ID.
			int iItemID = -1;
	
			CTFWeaponBase *pWeapon = GetActiveTFWeapon();
			if ( !pWeapon )
				return;

			RemoveAmmo(1, pWeapon->GetPrimaryAmmoType());

			pWeapon->StartEffectBarRegen();

			iItemID = pWeapon->GetItemID();

			// Pull out of the wall a bit.
			if ( tr.fraction != 1.0 )
			{
				SetAbsOrigin(tr.endpos + (tr.plane.normal * 1.0f));
			}

			EmitSound("Weapon_Grenade_Mirv.MainExplode");

			// Explosion effect on client.
			Vector vecOrigin = pWeapon->GetAbsOrigin();
			CPVSFilter filter( vecOrigin );
			int iEntIndex = ( tr.m_pEnt && tr.m_pEnt->IsPlayer() ) ? tr.m_pEnt->entindex() : -1;
			const Vector &vecNormal = tr.plane.normal;

			TE_TFExplosion(filter, 0.0f, vecOrigin, vecNormal, pWeapon->GetWeaponID(), iEntIndex, this, GetTeamNumber(), false, iItemID);

			// Scorch mark
			trace_t	tr_scorch;
			Vector vecEnd = GetAbsOrigin() - Vector(0, 0, 60);
			UTIL_TraceLine(GetAbsOrigin(), vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr_scorch);
			if (tr_scorch.m_pEnt && !tr_scorch.m_pEnt->IsPlayer())
				UTIL_DecalTrace(&tr_scorch, "Scorch");

			// Screen shake
			UTIL_ScreenShake(GetAbsOrigin(), 10.0f, 150.0, 1.0, 600.0f, SHAKE_START);

			// Use the thrower's position as the reported position
			Vector vecReported = GetAbsOrigin();

			float flRadius = 192.0f;

			CTFRadiusDamageInfo radiusInfo;
			radiusInfo.info.Set( pWeapon, this, pWeapon, vec3_origin, vecOrigin, 1000, pWeapon->GetDamageType(), 0, &vecReported );

			if( iTauntType == TAUNTATK_DEMOMAN_MIRV_KILL )  
				radiusInfo.info.SetDamageCustom(TF_DMG_CUSTOM_TAUNTATK_MIRV);
			else
				radiusInfo.info.SetDamageCustom(TF_DMG_CUSTOM_TAUNTATK_GRENADE);

			radiusInfo.m_vecSrc = vecOrigin;
			radiusInfo.m_flRadius = flRadius;
			radiusInfo.m_flSelfDamageRadius = 96.0f;

			// Demo needs to recieve the full damage
			// But not also the base radius damage
			radiusInfo.m_pEntityIgnore = this;
			radiusInfo.ApplyToEntity(this);

			TFGameRules()->RadiusDamage( radiusInfo );
		}
#ifdef ITEM_TAUNTING
		case TAUNTATK_HIGHFIVE_PARTICLE:
		{
			// Create high-five slap effect ahead and above.
			Vector vecPos, vecForward, vecRight, vecUp;
			QAngle vecAngles = EyeAngles();
			vecAngles[PITCH] = 0.0f;
			AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );
			vecPos = GetAbsOrigin() + vecForward * 30.0f + vecRight * -3.0f + vecUp * 87.0f;

			CTFPlayer *pOther = m_hTauntPartner.Get();
			const char *pszFormat = ( !pOther || IsEnemy( pOther ) ) ? "highfive_%s" : "highfive_%s_old";
			const char *pszEffect = ConstructTeamParticle( pszFormat, GetTeamNumber() );
			DispatchParticleEffect( pszEffect, vecPos, vec3_angle );

			break;
		}
#endif
	}
}


void CTFPlayer::ClearTauntAttack( void )
{
	m_flTauntAttackTime = 0.0f;
	m_flTauntAttackEndTime = 0.0f;
	m_iTauntAttack = TAUNTATK_NONE;
#ifdef ITEM_TAUNTING
	m_pTauntItem = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CTFPlayer::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	// This is a lame way to detect a taunt!
	if ( m_bInitTaunt )
	{
		m_bInitTaunt = false;
		return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
	}
	
	return InstancedScriptedScene( this, pszScene, NULL, flDelay, false, response, true, filter );
}


bool CTFPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
		{
			info->m_nSequence = LookupSequence( event->GetParameters() );

			// Make sure the sequence exists.
			if ( info->m_nSequence < 0 )
			{
				Warning( "CSceneEntity %s :\"%s\" unable to find sequence \"%s\"\n", STRING( GetEntityName() ), actor->GetName(), event->GetParameters() );
				return false;
			}

			// VCD animations are all assigned to specific gesture slot.
			info->m_iLayer = GESTURE_SLOT_VCD;
			info->m_pActor = actor;
			return true;
		}
		default:
			return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}


bool CTFPlayer::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
		{
			float flCycle = 0.0f;

			if ( !( ( GetSequenceFlags( GetModelPtr(), info->m_nSequence ) & STUDIO_LOOPING ) != 0 ) )
			{
				float dt = scene->GetTime() - event->GetStartTime();
				float seq_duration = SequenceDuration( info->m_nSequence );
				flCycle = clamp( dt / seq_duration, 0.0f, 1.0f );
			}

			if ( !info->m_bStarted )
			{
				m_PlayerAnimState->ResetGestureSlot( info->m_iLayer );
				m_PlayerAnimState->AddVCDSequenceToGestureSlot( info->m_iLayer, info->m_nSequence, flCycle );
				SetLayerCycle( info->m_iLayer, flCycle, flCycle, 0.0f );
			}
			else
			{
				SetLayerCycle( info->m_iLayer, flCycle );
			}

			// Keep layer weight at 1, we don't want taunt animations to be interrupted by anything.
			SetLayerWeight( info->m_iLayer, 1.0f );

			return true;
		}
		default:
			return BaseClass::ProcessSceneEvent( info, scene, event );
	}
}


void CTFPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	// If we have 'disguiseclass' criteria, pretend that we are actually our
	// disguise class. That way we just look up the scene we would play as if 
	// we were that class.
	int iDisguiseIndex = criteriaSet.FindCriterionIndex( "disguiseclass" );
	if ( iDisguiseIndex != -1 )
	{
		criteriaSet.AppendCriteria( "playerclass", criteriaSet.GetValue( iDisguiseIndex ) );
	}
	else if ( GetPlayerClass() )
	{
		criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[GetPlayerClass()->GetClassIndex()] );
	}

	criteriaSet.AppendCriteria( "playerpitch", UTIL_VarArgs( "%.3f", EyeAngles().x ) );

	criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs( "%d", m_Shared.GetNumKillsInTime( 30.0f ) ) );

	int iTotalKills = 0;
	PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( this );
	if ( pStats )
	{
		iTotalKills = pStats->statsCurrentLife.m_iStat[TFSTAT_KILLS] + pStats->statsCurrentLife.m_iStat[TFSTAT_KILLASSISTS] + 
			pStats->statsCurrentLife.m_iStat[TFSTAT_BUILDINGSDESTROYED];
	}

	criteriaSet.AppendCriteria( "killsthislife", UTIL_VarArgs( "%d", iTotalKills ) );
	criteriaSet.AppendCriteria( "disguised", m_Shared.IsDisguised() ? "1" : "0" );
	criteriaSet.AppendCriteria( "invulnerable", m_Shared.IsInvulnerable() ? "1" : "0" );
	criteriaSet.AppendCriteria( "beinghealed", m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "waitingforplayers", ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->IsInPreMatch() ) ? "1" : "0" );
	switch ( GetTFTeam()->GetRole() )
	{
		case TEAM_ROLE_DEFENDERS:
			criteriaSet.AppendCriteria( "teamrole", "defense" );
			break;
		case TEAM_ROLE_ATTACKERS:
			criteriaSet.AppendCriteria( "teamrole", "offense" );
			break;
	}

	criteriaSet.AppendCriteria( "stunned", m_Shared.IsControlStunned() ? "1" : "0" );
	criteriaSet.AppendCriteria( "snared", m_Shared.IsSnared() ? "1" : "0" );
	criteriaSet.AppendCriteria( "doublejumping", m_Shared.IsAirDashing() ? "1" : "0" );

	// Current weapon role.
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		ETFWeaponType iWeaponRole = pActiveWeapon->GetTFWpnData().m_iWeaponType;
		if ( iWeaponRole >= 0 && iWeaponRole < TF_WPN_TYPE_COUNT )
		{
			criteriaSet.AppendCriteria( "weaponmode", g_AnimSlots[iWeaponRole] );
		}

		if ( pActiveWeapon->IsSniperRifle() )
		{
			if ( m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				criteriaSet.AppendCriteria( "sniperzoomed", "1" );
			}
		}
		else if ( pActiveWeapon->IsMinigun() )
		{
			criteriaSet.AppendCriteria( "minigunfiretime", UTIL_VarArgs( "%.1f", static_cast<CTFMinigun *>( pActiveWeapon )->GetFiringTime() ) );
		}

		CEconItemDefinition *pItemDef = pActiveWeapon->GetItem()->GetStaticData();
		if ( pItemDef )
		{
			criteriaSet.AppendCriteria( "item_name", pItemDef->name );
			criteriaSet.AppendCriteria( "item_type_name", pItemDef->item_type_name );
		}
	}

	CEconEntity *pEntity;
	CEconItemDefinition *pItemDef;
	for ( int i = 0; i < TF_LOADOUT_SLOT_COUNT; i++ )
	{
		pEntity = GetEntityForLoadoutSlot( (ETFLoadoutSlot)i );
		if ( !pEntity )
			continue;

		pItemDef = pEntity->GetItem()->GetStaticData();
		if ( pItemDef && !pItemDef->baseitem )
		{
			criteriaSet.AppendCriteria( UTIL_VarArgs( "loadout_slot_%s", g_LoadoutSlots[i] ), pItemDef->item_name );
		}
	}

	// Player under crosshair.
	trace_t tr;
	Vector forward;
	EyeVectors( &forward );
	UTIL_TraceLine( EyePosition(), EyePosition() + forward * MAX_TRACE_LENGTH, MASK_VISIBLE_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
			if ( pTFPlayer )
			{
				int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
				bool bEnemyDisguisedAsAlly = false;
				if ( IsEnemy( pEntity ) )
				{
					// Prevent spotting stealthed enemies who haven't been exposed recently.
					if ( pTFPlayer->m_Shared.IsStealthed() )
					{
						if ( pTFPlayer->m_Shared.GetLastStealthExposedTime() < ( gpGlobals->curtime - 3.0f ) )
						{
							iClass = TF_CLASS_UNDEFINED;
						}
						else
						{
							iClass = TF_CLASS_SPY;
						}
					}
					else if ( pTFPlayer->m_Shared.IsDisguised() )
					{
						iClass = pTFPlayer->m_Shared.GetDisguiseClass();
						bEnemyDisguisedAsAlly = (!pTFPlayer->m_Shared.DisguiseFoolsTeam(GetTeamNumber()));
					}
				}

				criteriaSet.AppendCriteria( "crosshair_enemy", IsEnemy( pTFPlayer ) && !bEnemyDisguisedAsAlly ? "Yes" : "No" );

				if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_COUNT_ALL )
				{
					criteriaSet.AppendCriteria( "crosshair_on", g_aPlayerClassNames_NonLocalized[iClass] );
				}
			}
		}
	}

	// Previous round win.
	bool bLoser = ( TFGameRules()->GetPreviousRoundWinners() != TEAM_UNASSIGNED && TFGameRules()->GetPreviousRoundWinners() != GetTeamNumber() );
	criteriaSet.AppendCriteria( "LostRound", UTIL_VarArgs( "%d", bLoser ) );

	// Control points.
	for ( CTriggerAreaCapture *pAreaTrigger : CTriggerAreaCapture::AutoList() )
	{
		if ( !pAreaTrigger->IsTouching( this ) )
			continue;

		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( pCP->GetOwner() == GetTeamNumber() )
			{
				criteriaSet.AppendCriteria( "OnFriendlyControlPoint", "1" );
			}
			else
			{
				if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) &&
					TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
				{
					criteriaSet.AppendCriteria( "OnCappableControlPoint", "1" );
				}
			}
		}
	}

	criteriaSet.AppendCriteria( "OnWinningTeam", GetTeamNumber() == TFGameRules()->GetWinningTeam() ? "1" : "0" );
	
	// Weapons with "special taunts" i.e. taunt kills, or abilities (Kritzkrieg).
	int iSpecialTaunt = 0;
	if ( pActiveWeapon )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iSpecialTaunt, special_taunt );
	}

	// Only roll the dice if the active weapon doesn't have special taunt attribute.
	if ( !iSpecialTaunt && TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
	{
		if ( RandomFloat( 0.0f, 0.1f ) < 0.4f )
		{
			criteriaSet.AppendCriteria( "IsHalloweenTaunt", "1" );
		}
	}

	// The current game state.
	criteriaSet.AppendCriteria( "GameRound", UTIL_VarArgs( "%d", TFGameRules()->State_Get() ) );

	// Team-specific criteria.
	criteriaSet.AppendCriteria( "OnRedTeam", UTIL_VarArgs( "%d", GetTeamNumber() == TF_TEAM_RED ) );
	criteriaSet.AppendCriteria( "OnBlueTeam", UTIL_VarArgs( "%d", GetTeamNumber() == TF_TEAM_BLUE ) );
	criteriaSet.AppendCriteria( "OnGreenTeam", UTIL_VarArgs( "%d", GetTeamNumber() == TF_TEAM_GREEN ) );
	criteriaSet.AppendCriteria( "OnYellowTeam", UTIL_VarArgs( "%d", GetTeamNumber() == TF_TEAM_YELLOW ) );
}


bool CTFPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE )
		return false;

	if ( !pPlayer->IsAlive() && IsAlive() )
	{
		// Everyone can chat like normal when the round/game ends
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN || TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
			return true;

		// Separate rule for spectators.
		if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			return tf_spectalk.GetBool();

		// Living players can't hear dead ones unless gravetalk is enabled.
		return tf_gravetalk.GetBool();
	}

	return true;
}


bool CTFPlayer::CanBeAutobalanced( void )
{
	return !IsVIP();
}


IResponseSystem *CTFPlayer::GetResponseSystem()
{
	int iClass;
	if ( m_bSpeakingConceptAsDisguisedSpy && m_Shared.IsDisguised() )
	{
		iClass = m_Shared.GetDisguiseClass();
	}
	else
	{
		iClass = GetPlayerClass()->GetClassIndex();
	}

	bool bValidClass = ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_CLASS_COUNT );
	bool bValidConcept = ( m_iCurrentConcept >= 0 && m_iCurrentConcept < MP_TF_CONCEPT_COUNT );
	Assert( bValidClass );
	Assert( bValidConcept );

	if ( !bValidClass || !bValidConcept )
		return BaseClass::GetResponseSystem();
	
	return TFGameRules()->m_ResponseRules[iClass].m_ResponseSystems[m_iCurrentConcept];
}


bool CTFPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	if ( !IsAlive() )
		return false;

	if ( IsSpeaking() && iConcept != MP_CONCEPT_DIED && iConcept != MP_CONCEPT_ACHIEVEMENT_AWARD && iConcept != MP_CONCEPT_KILLED_PLAYER && iConcept != MP_CONCEPT_CIVILIAN_PROTECT )
		return false;

	// Allow Uber faking.
	bool bChargeReadyOverriden = false;
	ITFHealingWeapon *pMedigun = GetMedigun();
	if ( pMedigun && (iConcept == MP_CONCEPT_MEDIC_CHARGEREADY || iConcept == MP_CONCEPT_PLAYER_CHARGEREADY) )
	{
		if ( pMedigun->GetChargeLevel() < 1 )
		{
			if ( pMedigun->GetChargeLevel() >= tf2c_uber_readiness_threshold.GetFloat() )
				iConcept = MP_CONCEPT_PLAYER_POSITIVE;
			else
				iConcept = MP_CONCEPT_PLAYER_NEGATIVE;

			bChargeReadyOverriden = true;
		}
	}

	// Save the current concept.
	m_iCurrentConcept = iConcept;

	bool bRet = false;

	if ( m_Shared.IsDisguised() /*&& !filter*/ && iConcept != MP_CONCEPT_KILLED_PLAYER )
	{
		// Test, enemies and myself.
		CRecipientFilter disguisedFilter;
		ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
		{
			disguisedFilter.AddRecipientsByTeam( GetGlobalTeam( iTeam ) );
			return true;
		} );
		disguisedFilter.AddRecipient( this );

		CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
		Assert( pExpresser );

		pExpresser->AllowMultipleScenes();

		// Play disguised concept to enemies and myself.
		char szBuffer[128];
		V_sprintf_safe( szBuffer, "disguiseclass:%s", g_aPlayerClassNames_NonLocalized[m_Shared.GetDisguiseClass()] );

		if ( modifiers )
		{
			V_strcat_safe( szBuffer, "," );
			V_strcat_safe( szBuffer, modifiers );
		}

		m_bSpeakingConceptAsDisguisedSpy = true;

		bool bPlayedDisguised = SpeakIfAllowed( g_pszMPConcepts[iConcept], szBuffer, pszOutResponseChosen, bufsize, &disguisedFilter );

		m_bSpeakingConceptAsDisguisedSpy = false;

		// Test, everyone except enemies and myself.
		CBroadcastRecipientFilter undisguisedFilter;
		ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
		{
			undisguisedFilter.RemoveRecipientsByTeam( GetGlobalTeam( iTeam ) );
			return true;
		} );
		undisguisedFilter.RemoveRecipient( this );

		// Play normal concept to teammates.
		bool bPlayedNormally = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &undisguisedFilter );

		pExpresser->DisallowMultipleScenes();

		bRet = ( bPlayedDisguised && bPlayedNormally );
	}
	else
	{
		if ( bChargeReadyOverriden )
		{
			CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
			Assert( pExpresser );

			pExpresser->AllowMultipleScenes();

			CBroadcastRecipientFilter allyFilter;
			ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
			{
				allyFilter.RemoveRecipientsByTeam( GetGlobalTeam( iTeam ) );
				return true;
			} );
			allyFilter.RemoveRecipient( this );
			bool bAllyPlayed = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &allyFilter );

			CBroadcastRecipientFilter enemyFilter;
			ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
			{
				allyFilter.AddRecipientsByTeam( GetGlobalTeam( iTeam ) );
				return true;
			} );
			allyFilter.RemoveRecipient( this );
			bool bEnemyPlayed = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &enemyFilter );

			bRet = bEnemyPlayed && bAllyPlayed;

			pExpresser->DisallowMultipleScenes();
		}
		else
		{
			// Play normally.
			bRet = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );
		}
	}

	//Add bubble on top of a player calling for medic.
	if ( bRet && iConcept == MP_CONCEPT_PLAYER_MEDIC )
	{
		SaveMe();
	}

	return bRet;
}


void CTFPlayer::UpdateExpression( void )
{
	AI_Response response;
	bool bResult;
	m_iCurrentConcept = MP_CONCEPT_PLAYER_EXPRESSION;

	if ( m_Shared.IsDisguised() )
	{
		m_bSpeakingConceptAsDisguisedSpy = true;	
		char modifiers[128];
		V_sprintf_safe( modifiers, "disguiseclass:%s", g_aPlayerClassNames_NonLocalized[m_Shared.GetDisguiseClass()] );

		bResult = SpeakFindResponse( response, g_pszMPConcepts[MP_CONCEPT_PLAYER_EXPRESSION], modifiers );
		m_bSpeakingConceptAsDisguisedSpy = false;
	}
	else
	{
		bResult = SpeakFindResponse( response, g_pszMPConcepts[MP_CONCEPT_PLAYER_EXPRESSION] );
	}

	if ( !bResult )
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat( 30.0f, 40.0f );
		return;
	}
	
	// Ignore updates that choose the same scene.
	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING( m_iszExpressionScene ), response.GetResponsePtr() ) == 0 )
		return;

	if ( m_hExpressionSceneEnt )
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString( response.GetResponsePtr() );
	float flDuration = InstancedScriptedScene( this, response.GetResponsePtr(), &m_hExpressionSceneEnt, 0.0f, true, NULL, true );
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}


void CTFPlayer::ClearExpression( void )
{
	if ( m_hExpressionSceneEnt )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Only show subtitle to enemy if we're disguised as the enemy
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return ( m_Shared.IsDisguised() && !m_Shared.DisguiseFoolsTeam( GetTeamNumber() ) );
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow rapid-fire voice commands
//-----------------------------------------------------------------------------
bool CTFPlayer::CanSpeakVoiceCommand( void )
{
	return ( gpGlobals->curtime > m_flNextVoiceCommandTime );
}

//-----------------------------------------------------------------------------
// Purpose: Note the time we're allowed to next speak a voice command
//-----------------------------------------------------------------------------
void CTFPlayer::NoteSpokeVoiceCommand( const char *pszScenePlayed )
{
	Assert( pszScenePlayed );
	m_flNextVoiceCommandTime = gpGlobals->curtime + Min( GetSceneDuration( pszScenePlayed ), tf_max_voice_speak_delay.GetFloat() );
}


bool CTFPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	// Don't lag compensate dead players.
	if ( pPlayer->m_lifeState != LIFE_ALIVE )
		return false;

	bool bIsMedic = false;

	// Do Lag comp on medics trying to heal team mates.
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon && ( pWeapon->IsWeapon( TF_WEAPON_MEDIGUN ) || pWeapon->IsWeapon( TF_WEAPON_TASER ) ) )
	{
		bIsMedic = true;

		CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( pWeapon );
		if ( pMedigun && pMedigun->GetHealTarget() )
			return ( pMedigun->GetHealTarget() == pPlayer );
	}

	// Don't lag compensate allies unless we're a Medic.
	if ( !IsEnemy( pPlayer ) && !bIsMedic )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pPlayer->GetAbsOrigin();

	// Get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5f * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// Lag compensate everyone nearby when using flamethrower.
	// FIXME: Magic number! Possibly calculate max range based on cvar values?
	CTFFlameThrower *pFlamethrower = dynamic_cast<CTFFlameThrower *>( Weapon_OwnsThisID( TF_WEAPON_FLAMETHROWER ) );
	if ( pFlamethrower && pFlamethrower->IsSimulatingFlames() )
		return ( vHisOrigin.DistToSqr( vMyOrigin ) < Square( 1024.0f ) );

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );

	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	// 45 degree angle
	float flCosAngle = 0.707107f;
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Mapmaker input to force this player to speak a response rules concept
//-----------------------------------------------------------------------------
void CTFPlayer::InputSpeakResponseConcept( inputdata_t &inputdata )
{
	int iConcept = GetMPConceptIndexFromString( inputdata.value.String() );
	if ( iConcept != MP_CONCEPT_NONE )
	{
		SpeakConceptIfAllowed( iConcept );
	}
}


void CTFPlayer::SpeakWeaponFire( int iCustomConcept )
{
	if ( iCustomConcept == MP_CONCEPT_NONE )
	{
		if ( m_flNextSpeakWeaponFire > gpGlobals->curtime )
			return;

		iCustomConcept = MP_CONCEPT_FIREWEAPON;
	}

	m_flNextSpeakWeaponFire = gpGlobals->curtime + 5.0f;

	// Don't play a weapon fire scene if we already have one.
	if ( m_hWeaponFireSceneEnt )
	{
		return;
	}

	char szScene[MAX_PATH];
	if ( !GetResponseSceneFromConcept( iCustomConcept, szScene, sizeof( szScene ) ) )
		return;

	float flDuration = InstancedScriptedScene( this, szScene, &m_hWeaponFireSceneEnt, 0.0, true, NULL, true );
	m_flNextSpeakWeaponFire = gpGlobals->curtime + flDuration;
}


void CTFPlayer::ClearWeaponFireScene( void )
{
	if ( m_hWeaponFireSceneEnt )
	{
		StopScriptedScene( this, m_hWeaponFireSceneEnt );
		m_hWeaponFireSceneEnt = NULL;
	}
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
}


void CTFPlayer::CreateDisguiseWeaponList( CTFPlayer *pDisguiseTarget )
{
	ClearDisguiseWeaponList();

	// Copy disguise target's weapons.
	if ( pDisguiseTarget && ( pDisguiseTarget->IsPlayerClass( m_Shared.GetDisguiseClass(), true ) && pDisguiseTarget->IsAlive() ) )
	{
		for ( int i = 0; i < TF_PLAYER_WEAPON_COUNT; ++i )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pDisguiseTarget->GetWeapon( i ) );
			if ( !pWeapon )
				continue;

			DisguiseWeapon_t disguiseWeapon;
			disguiseWeapon.iClassname = AllocPooledString( pWeapon->GetClassname() );
			disguiseWeapon.iSlot = pWeapon->GetSlot();
			disguiseWeapon.iSubType = !V_strcmp( pWeapon->GetClassname(), "tf_weapon_builder" ) || !V_strcmp( pWeapon->GetClassname(), "tf_weapon_sapper" ) ? pDisguiseTarget->GetPlayerClass()->GetData()->m_aBuildable[0] : 0;
			disguiseWeapon.pItem = new CEconItemView;
			*disguiseWeapon.pItem = *pWeapon->GetItem();
			m_hDisguiseWeaponList.AddToTail( disguiseWeapon );
		}
	}
}


void CTFPlayer::ClearDisguiseWeaponList()
{
	for ( int i = 0, c = m_hDisguiseWeaponList.Count(); i < c; ++i )
	{
		delete m_hDisguiseWeaponList[i].pItem;
	}
	m_hDisguiseWeaponList.RemoveAll();
}


int CTFPlayer::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT ) 
	{
		char tempstr[512];

		V_sprintf_safe( tempstr, "Health: %d / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CTFPlayer::GetResponseSceneFromConcept( int iConcept, char *pszSceneBuffer, int numSceneBufferBytes )
{
	AI_Response response;
	bool bResult = SpeakConcept( response, iConcept );

	if ( bResult )
	{
		if ( response.IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( response.GetContext() );
			}
		}
		else
		{
			AddContext( response.GetContext() );
		}

		V_strncpy( pszSceneBuffer, response.GetResponsePtr(), numSceneBufferBytes );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate a score for this player. higher is more likely to be switched
// The default -120 score "switch engineers less often" code block was replaced in favor of giving score per building.
//-----------------------------------------------------------------------------
int	CTFPlayer::CalculateTeamBalanceScore( void )
{
	int iScore = BaseClass::CalculateTeamBalanceScore();

	int iBuildingScore = 0;

	for (int i = GetObjectCount() - 1; i >= 0; i--)
	{
		CBaseObject *pObject = GetObject(i);
		Assert(pObject);

		// We will subtract this amount from the score.
		if (pObject)
		{
			iBuildingScore += pObject->CalculateTeamBalanceScore();
		}
	}

	// Building score limit.
	if (iBuildingScore > 120)
		iBuildingScore = 120;

	iScore -= iBuildingScore;

	return iScore;
}

//-----------------------------------------------------------------------------
// Purpose: Update TF Nav Mesh visibility as the player moves from area to area
//-----------------------------------------------------------------------------
void CTFPlayer::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
	VPROF_BUDGET( "CTFPlayer::OnNavAreaChanged", "NextBot" );

	if ( !IsAlive() || GetTeamNumber() == TEAM_SPECTATOR ) return;

	if ( leftArea )
	{
		leftArea->ForAllPotentiallyVisibleAreas( [=]( CNavArea *pv_area ) {
			static_cast<CTFNavArea *>( pv_area )->RemovePotentiallyVisibleActor( this );
			return true;
		} );
	}

	if ( enteredArea )
	{
		enteredArea->ForAllPotentiallyVisibleAreas( [=]( CNavArea *pv_area ) {
			static_cast<CTFNavArea *>( pv_area )->AddPotentiallyVisibleActor( this );
			return true;
		} );
	}
}


CTriggerAreaCapture *CTFPlayer::GetControlPointStandingOn( void )
{
	auto root = static_cast<touchlink_t *>( GetDataObject( TOUCHLINK ) );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->CollisionProp()->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				auto pCapTrigger = dynamic_cast<CTriggerAreaCapture *>( pTouch );
				if ( pCapTrigger )
				{
					return pCapTrigger;
				}
			}
		}
	}

	return nullptr;
}


bool CTFPlayer::IsCapturingPoint( void )
{
	CTriggerAreaCapture *pCapTrigger = GetControlPointStandingOn();
	if ( pCapTrigger )
	{
		CTeamControlPoint *pPoint = pCapTrigger->GetControlPoint();
		if ( pPoint && TFGameRules()->TeamMayCapturePoint( GetTeamNumber(), pPoint->GetPointIndex() ) &&
			TFGameRules()->PlayerMayCapturePoint( this, pPoint->GetPointIndex() ) && pPoint->GetOwner() != GetTeamNumber() )
		{
			return true;
		}
	}

	return false;
}


void CTFPlayer::CalculateTeamScrambleScore( void )
{
	CTFPlayerResource *pResource = GetTFPlayerResource();
	if ( !pResource )
	{
		m_flTeamScrambleScore = 0.0f;
		return;
	}

	int iMode = mp_scrambleteams_mode.GetInt();
	float flScore = 0.0f;

	switch ( iMode )
	{
		case TF_SCRAMBLEMODE_SCORETIME_RATIO:
		default:
		{
			// Points per minute ratio.
			float flTime = GetConnectionTime() / 60.0f;
			float flScore = (float)pResource->GetTotalScore( entindex() );

			flScore = ( flScore / flTime );
			break;
		}
		case TF_SCRAMBLEMODE_KILLDEATH_RATIO:
		{
			// Don't divide by zero.
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( this );
			int iKills = pStats->statsAccumulated.m_iStat[TFSTAT_KILLS];
			int iDeaths = Max( 1, pStats->statsAccumulated.m_iStat[TFSTAT_DEATHS] );

			flScore = ( (float)iKills / (float)iDeaths );
			break;
		}
		case TF_SCRAMBLEMODE_SCORE:
			flScore = (float)pResource->GetTotalScore( entindex() );
			break;
		case TF_SCRAMBLEMODE_CLASS:
			flScore = (float)m_PlayerClass.GetClassIndex();
			break;
	}

	m_flTeamScrambleScore = flScore;
}


HSCRIPT CTFPlayer::ScriptGetActiveWeapon( void )
{
	if ( GetActiveWeapon() )
		return GetActiveWeapon()->GetScriptInstance();

	return NULL;
}

HSCRIPT CTFPlayer::ScriptGetHealTarget()
{
	if ( MedicGetHealTarget() )
		return MedicGetHealTarget()->GetScriptInstance();

	return NULL;
}

void CTFPlayer::ScriptAddCond( int cond )
{
	if ( cond < TF_COND_LAST )
		m_Shared.AddCond( (ETFCond)cond );
}

void CTFPlayer::ScriptAddCondEx( int cond, float flDuration, HSCRIPT hProvider )
{
	if ( cond < TF_COND_LAST )
	{
		CBaseEntity *pProvider = NULL;
		if ( hProvider )
			pProvider = ToEnt( hProvider );

		m_Shared.AddCond( (ETFCond)cond, flDuration, pProvider );
	}
}

void CTFPlayer::ScriptRemoveCond( int cond )
{
	if ( cond < TF_COND_LAST )
		m_Shared.RemoveCond( (ETFCond)cond );
}

void CTFPlayer::ScriptRemoveCondEx( int cond, bool bIgnoreDuration )
{
	if ( cond < TF_COND_LAST )
		m_Shared.RemoveCond( (ETFCond)cond, bIgnoreDuration );
}

bool CTFPlayer::ScriptInCond( int cond )
{
	if ( cond >= TF_COND_LAST )
		return false;
	return m_Shared.InCond( (ETFCond)cond );
}

bool CTFPlayer::ScriptWasInCond( int cond )
{
	if ( cond >= TF_COND_LAST )
		return false;
	return m_Shared.WasInCond( (ETFCond)cond );
}

void CTFPlayer::ScriptRemoveAllCond()
{
	m_Shared.RemoveAllCond();
}

float CTFPlayer::ScriptGetCondDuration( int cond )
{
	if ( cond >= TF_COND_LAST )
		return 0.0f;
	return m_Shared.GetConditionDuration( (ETFCond)cond );
}

void CTFPlayer::ScriptSetCondDuration( int cond, float flDuration )
{
	if ( cond < TF_COND_LAST )
		m_Shared.SetConditionDuration( (ETFCond)cond, flDuration );
}

HSCRIPT CTFPlayer::ScriptGetDisguiseTarget()
{
	if ( m_Shared.GetDisguiseTarget().Get() )
		return m_Shared.GetDisguiseTarget()->GetScriptInstance();

	return NULL;
}

int CTFPlayer::ScriptGetDisguiseAmmoCount()
{
	return m_Shared.GetDisguiseAmmoCount();
}

void CTFPlayer::ScriptSetDisguiseAmmoCount( int nCount )
{
	m_Shared.SetDisguiseAmmoCount( nCount );
}

int CTFPlayer::ScriptGetDisguiseTeam()
{
	return m_Shared.GetDisguiseTeam();
}

bool CTFPlayer::ScriptIsCarryingRune()
{
	return false;
}

bool CTFPlayer::ScriptIsCritBoosted()
{
	return m_Shared.IsCritBoosted();
}

bool CTFPlayer::ScriptIsInvulnerable()
{
	return m_Shared.IsInvulnerable();
}

bool CTFPlayer::ScriptIsStealthed()
{
	return m_Shared.IsStealthed();
}

bool CTFPlayer::ScriptCanBeDebuffed()
{
	return m_Shared.CanBeDebuffed();
}

bool CTFPlayer::ScriptIsImmuneToPushback()
{
	return m_Shared.IsImmuneToPushback();
}

bool CTFPlayer::ScriptIsFullyInvisible()
{
	return m_Shared.GetPercentInvisible() >= 1.0f;
}

float CTFPlayer::ScriptGetSpyCloakMeter()
{
	return m_Shared.GetSpyCloakMeter();
}

void CTFPlayer::ScriptSetSpyCloakMeter( float flMeter )
{
	m_Shared.SetSpyCloakMeter( flMeter );
}

bool CTFPlayer::ScriptIsRageDraining()
{
	return false;
}

float CTFPlayer::ScriptGetRageMeter()
{
	return 0.0f;
}

void CTFPlayer::ScriptSetRageMeter( float flMeter )
{
}

bool CTFPlayer::ScriptIsHypeBuffed()
{
	return false;
}

float CTFPlayer::ScriptGetScoutHypeMeter()
{
	return 0.0f;
}

void CTFPlayer::ScriptSetScoutHypeMeter( float flMeter )
{
}

bool CTFPlayer::ScriptIsJumping()
{
	return m_Shared.IsJumping();
}

bool CTFPlayer::ScriptIsAirDashing()
{
	return m_Shared.IsAirDashing();
}

bool CTFPlayer::ScriptIsControlStunned()
{
	return m_Shared.IsControlStunned();
}

bool CTFPlayer::ScriptIsSnared()
{
	return m_Shared.IsSnared();
}

int CTFPlayer::ScriptGetCaptures()
{
	return 0;
}

int CTFPlayer::ScriptGetDefenses()
{
	return 0;
}

int CTFPlayer::ScriptGetDominations()
{
	return 0;
}

int CTFPlayer::ScriptGetRevenge()
{
	return 0;
}

int CTFPlayer::ScriptGetBuildingsDestroyed()
{
	return 0;
}

int CTFPlayer::ScriptGetHeadshots()
{
	return 0;
}

int CTFPlayer::ScriptGetBackstabs()
{
	return 0;
}

int CTFPlayer::ScriptGetHealPoints()
{
	return 0;
}

int CTFPlayer::ScriptGetInvulns()
{
	return 0;
}

int CTFPlayer::ScriptGetTeleports()
{
	return 0;
}

int CTFPlayer::ScriptGetResupplyPoints()
{
	return 0;
}

int CTFPlayer::ScriptGetKillAssists()
{
	return 0;
}

int CTFPlayer::ScriptGetBonusPoints()
{
	return 0;
}

void CTFPlayer::ScriptResetScores()
{
}

bool CTFPlayer::ScriptIsParachuteEquipped()
{
	return false;
}

int CTFPlayer::ScriptGetPlayerClass()
{
	return GetPlayerClass()->GetClassIndex();
}

void CTFPlayer::ScriptSetPlayerClass( int nClass )
{
	m_PlayerClass.Init( nClass );
}

void CTFPlayer::ScriptRemoveAllItems( bool removeSuit )
{
	RemoveAllItems( removeSuit );
}

Vector CTFPlayer::ScriptWeapon_ShootPosition()
{
	return Weapon_ShootPosition();
}

bool CTFPlayer::ScriptWeapon_CanUse( HSCRIPT hWeapon )
{
	return Weapon_CanUse( HScriptToClass<CBaseCombatWeapon>( hWeapon ) );
}

void CTFPlayer::ScriptWeapon_Equip( HSCRIPT hWeapon )
{
	Weapon_Equip( HScriptToClass<CBaseCombatWeapon>( hWeapon ) );
}

void CTFPlayer::ScriptWeapon_Drop( HSCRIPT hWeapon )
{
	Weapon_Drop( HScriptToClass<CBaseCombatWeapon>( hWeapon ), NULL, NULL );
}

void CTFPlayer::ScriptWeapon_DropEx( HSCRIPT hWeapon, Vector vecDir, Vector vecVelocity )
{
	Weapon_Drop( HScriptToClass<CBaseCombatWeapon>( hWeapon ), &vecDir, &vecVelocity );
}

void CTFPlayer::ScriptWeapon_Switch( HSCRIPT hWeapon )
{
	Weapon_Switch( HScriptToClass<CBaseCombatWeapon>( hWeapon ) );
}

void CTFPlayer::ScriptWeapon_SetLast( HSCRIPT hWeapon )
{
	Weapon_SetLast( HScriptToClass<CBaseCombatWeapon>( hWeapon ) );
}

HSCRIPT CTFPlayer::ScriptGetLastWeapon()
{
	if ( GetLastWeapon() )
		return GetLastWeapon()->GetScriptInstance();

	return NULL;
}

void CTFPlayer::ScriptEquipWearableViewModel( HSCRIPT hViewModel )
{
	CBaseEntity *pEnt = ToEnt( hViewModel );
	CTFWearableVM *pViewModel = dynamic_cast<CTFWearableVM *>( pEnt );
	if ( pViewModel )
	{
		if ( pViewModel->IsViewModelWearable() )
			EquipWearable( pViewModel );
	}
}

bool CTFPlayer::ScriptIsFakeClient()
{
	return IsFakeClient();
}

int CTFPlayer::ScriptGetBotType()
{
	return GetBotType();
}

bool CTFPlayer::ScriptIsBotOfType( int botType )
{
	return IsBotOfType( botType );
}


// Debugging Stuff
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void DebugParticles( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		// print out their conditions
		pPlayer->m_Shared.DebugPrintConditions();	
	}
}

static ConCommand sv_debug_stuck_particles( "sv_debug_stuck_particles", DebugParticles, "Debugs particles attached to the player under your crosshair.", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: Debug concommand to set the player on fire
//-----------------------------------------------------------------------------
void IgnitePlayer()
{
	CTFPlayer *pPlayer = ToTFPlayer( ToTFPlayer( UTIL_PlayerByIndex( 1 ) ) );
	pPlayer->m_Shared.Burn( pPlayer );
}
static ConCommand cc_IgnitePlayer( "tf_ignite_player", IgnitePlayer, "Sets you on fire", FCVAR_CHEAT );



void TestVCD( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			if ( args.ArgC() >= 2 )
			{
				InstancedScriptedScene( pPlayer, args[1], NULL, 0.0f, false, NULL, true );
			}
			else
			{
				InstancedScriptedScene( pPlayer, "scenes/heavy_test.vcd", NULL, 0.0f, false, NULL, true );
			}
		}
	}
}
static ConCommand tf_testvcd( "tf_testvcd", TestVCD, "Run a vcd on the player currently under your crosshair. Optional parameter is the .vcd name (default is 'scenes/heavy_test.vcd')", FCVAR_CHEAT );


void TestRR( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("No concept specified. Format is tf_testrr <concept>\n");
		return;
	}

	CBaseEntity *pEntity = NULL;
	const char *pszConcept = args[1];

	if ( args.ArgC() == 3 )
	{
		pszConcept = args[2];
		pEntity = UTIL_PlayerByName( args[1] );
	}

	if ( !pEntity || !pEntity->IsPlayer() )
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( !pEntity || !pEntity->IsPlayer() )
		{
			pEntity = ToTFPlayer( UTIL_GetCommandClient() ); 
		}
	}

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			int iConcept = GetMPConceptIndexFromString( pszConcept );
			if ( iConcept != MP_CONCEPT_NONE )
			{
				pPlayer->SpeakConceptIfAllowed( iConcept );
			}
			else
			{
				Msg( "Attempted to speak unknown multiplayer concept: %s\n", pszConcept );
			}
		}
	}
}
static ConCommand tf_testrr( "tf_testrr", TestRR, "Force the player under your crosshair to speak a response rule concept. Format is tf_testrr <concept>, or tf_testrr <player name> <concept>", FCVAR_CHEAT );

#ifdef _DEBUG
CON_COMMAND_F( tf_crashclients, "testing only, crashes about 50 percent of the connected clients.", FCVAR_DEVELOPMENTONLY )
{
	for ( int i = 1; i < gpGlobals->maxClients; ++i )
	{
		if ( RandomFloat( 0.0f, 1.0f ) < 0.5f )
		{
			CBasePlayer *pl = UTIL_PlayerByIndex( i + 1 );
			if ( pl )
			{
				engine->ClientCommand( pl->edict(), "crash\n" );
			}
		}
	}
}
#endif

CON_COMMAND_F( give_weapon, "Give specified weapon.", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( args.ArgC() < 2 )
		return;

	const char *pszWeaponName = args[1];

	ETFWeaponID iWeaponID = GetWeaponId( pszWeaponName );

	CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfo( iWeaponID );
	if ( !pWeaponInfo )
		return;

	CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->Weapon_GetSlot( pWeaponInfo->iSlot );
	//If we already have a weapon in this slot but is not the same type then nuke it
	if ( pWeapon && pWeapon->GetWeaponID() != iWeaponID )
	{
		pWeapon->UnEquip();
		pWeapon = NULL;
	}

	if ( !pWeapon )
	{
		pPlayer->GiveNamedItem( pszWeaponName, 0, NULL );
	}
}

CON_COMMAND_F( give_econ, "Give ECON item with specified ID from item schema.\nFormat: <id> <classname> <attribute1> <value1> <attribute2> <value2> ... <attributeN> <valueN>", FCVAR_CHEAT )
{
	if ( args.ArgC() < 2 )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	int iItemID = atoi( args[1] );
	CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemID );
	if ( !pItemDef )
		return;

	CEconItemView econItem( iItemID );

	bool bAddedAttributes = false;

	// Additional params are attributes.
	for ( int i = 3; i + 1 < args.ArgC(); i += 2 )
	{
		int iAttribIndex = atoi( args[i] );

		CEconAttributeDefinition *pAttribDef = GetItemSchema()->GetAttributeDefinition( iAttribIndex );
		if ( !pAttribDef )
		{
			Warning( "Attempted to apply non existant attribute, skipping...\n" );
			continue;
		}

		if ( pAttribDef->string_attribute )
		{
			CEconItemAttribute econAttribute( iAttribIndex, args[i + 1], pAttribDef->attribute_class );
			econAttribute.m_strAttributeClass = AllocPooledString( econAttribute.attribute_class );
			bAddedAttributes = econItem.AddAttribute( &econAttribute );
		}
		else
		{
			CEconItemAttribute econAttribute( iAttribIndex, atof( args[i + 1] ) );
			econAttribute.m_strAttributeClass = AllocPooledString( econAttribute.attribute_class );
			bAddedAttributes = econItem.AddAttribute( &econAttribute );
		}
	}

	econItem.SkipBaseAttributes( bAddedAttributes );

	// Nuke whatever we have in this slot.
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	ETFLoadoutSlot iSlot = pItemDef->GetLoadoutSlot( iClass );
	CEconEntity *pEntity = pPlayer->GetEntityForLoadoutSlot( iSlot );
	if ( pEntity )
	{
		if ( pEntity->IsBaseCombatWeapon() )
		{
			CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( pEntity );
			pWeapon->UnEquip();
		}
		else if ( pEntity->IsWearable() )
		{
			pPlayer->RemoveWearable( static_cast<CEconWearable *>( pEntity ) );
		}
		else
		{
			AssertMsg( false, "Player has unknown entity in loadout slot %d.", iSlot );
			UTIL_Remove( pEntity );
		}
	}

	pPlayer->GiveNamedItem( args.ArgC() > 2 ? args[2] : pItemDef->item_class, 0, &econItem, TF_GIVEAMMO_MAX );
}


bool CTFPlayer::SetPowerplayEnabled( bool bOn )
{
	if ( bOn )
	{
		m_flPowerPlayTime = gpGlobals->curtime + 99999.9f;
		m_Shared.RecalculateChargeEffects();
		m_Shared.Burn( this );
		m_Shared.AddCond( TF_COND_RESISTANCE_BUFF );
		m_Shared.AddCond( TF_COND_DAMAGE_BOOST );

		PowerplayThink();
	}
	else
	{
		m_flPowerPlayTime = 0.0f;
		m_Shared.RecalculateChargeEffects();
		m_Shared.RemoveCond( TF_COND_BURNING );
		m_Shared.RemoveCond( TF_COND_RESISTANCE_BUFF );
		m_Shared.RemoveCond( TF_COND_DAMAGE_BOOST );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether player is a developer (not tester or contributor)
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerIsDeveloper( void )
{
	CTFPlayerResource *pResource = GetTFPlayerResource();
	int iPlayerRank = pResource ? pResource->GetPlayerRank( entindex() ) : TF_RANK_INVALID;
	if ( ( iPlayerRank > TF_RANK_NONE && iPlayerRank <= TF_RANK_COUNT ) && ( iPlayerRank & TF_RANK_DEVELOPER ) )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether player is a tester/contributor/developer
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerIsCredited(void)
{
	CTFPlayerResource *pResource = GetTFPlayerResource();
	int iPlayerRank = pResource ? pResource->GetPlayerRank(entindex()) : TF_RANK_INVALID;
	if ((iPlayerRank > TF_RANK_NONE && iPlayerRank <= TF_RANK_COUNT) && (iPlayerRank != TF_RANK_NONE))
		return true;

	return false;
}


void CTFPlayer::PowerplayThink( void )
{
	if ( m_flPowerPlayTime > gpGlobals->curtime )
	{
		m_Shared.Burn( this );

		float flDuration = 0;
		/* None of these exist... -sappho
		if ( GetPlayerClass() )
		{

			switch ( GetPlayerClass()->GetClassIndex() )
			{
				case TF_CLASS_SCOUT:        flDuration = InstancedScriptedScene( this, "scenes/player/scout/low/laughlong02.vcd",       NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_SNIPER:       flDuration = InstancedScriptedScene( this, "scenes/player/sniper/low/laughlong01.vcd",      NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_SOLDIER:      flDuration = InstancedScriptedScene( this, "scenes/player/soldier/low/laughevil02.vcd",     NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_DEMOMAN:      flDuration = InstancedScriptedScene( this, "scenes/player/demoman/low/laughlong02.vcd",     NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_MEDIC:        flDuration = InstancedScriptedScene( this, "scenes/player/medic/low/laughlong02.vcd",       NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_HEAVYWEAPONS: flDuration = InstancedScriptedScene( this, "scenes/player/heavy/low/laughlong01.vcd",       NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_PYRO:         flDuration = InstancedScriptedScene( this, "scenes/player/pyro/low/laughlong01.vcd",        NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_SPY:          flDuration = InstancedScriptedScene( this, "scenes/player/spy/low/laughevil01.vcd",         NULL, 0.0f, false, NULL, true ); break;
				case TF_CLASS_ENGINEER:     flDuration = InstancedScriptedScene( this, "scenes/player/engineer/low/laughlong01.vcd",    NULL, 0.0f, false, NULL, true ); break;
			}
		}
		*/
		SetContextThink( &CTFPlayer::PowerplayThink, gpGlobals->curtime + flDuration + RandomFloat( 2, 5 ), "TFPlayerLThink" );
	}
}


bool CTFPlayer::ShouldAnnouceAchievement( void )
{ 
	if ( IsPlayerClass( TF_CLASS_SPY, true ) )
	{
		if ( m_Shared.InCond( TF_COND_STEALTHED ) ||
			 m_Shared.IsDisguised() ||
			 m_Shared.InCond( TF_COND_DISGUISING ) )
			return false;
	}

	return true; 
}


void CTFPlayer::OnAchievementEarned( int iAchievement )
{
	BaseClass::OnAchievementEarned( iAchievement );

	if (!m_Shared.IsStealthed() && !m_Shared.IsDisguised())
	{
		SpeakConceptIfAllowed(MP_CONCEPT_ACHIEVEMENT_AWARD);
	}
}


void CTFPlayer::UpdateDominationsCount( void )
{
	m_iDominations = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( m_Shared.m_bPlayerDominated[i] )
		{
			m_iDominations++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CTFPlayer::PlayerSolidMask( bool brushOnly ) const
{
	unsigned int uMask = 0;

	switch ( GetTeamNumber() )
	{
	case TF_TEAM_RED:
		uMask = CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
		break;

	case TF_TEAM_BLUE:
		uMask = CONTENTS_REDTEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
		break;

	case TF_TEAM_GREEN:
		uMask = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_YELLOWTEAM;
		break;

	case TF_TEAM_YELLOW:
		uMask = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM;
		break;
	}

	return ( uMask | BaseClass::PlayerSolidMask( brushOnly ) );
}


bool CTFPlayer::GetClientConVarBoolValue( const char *pszValue )
{
	return !!atoi( engine->GetClientConVarValue( entindex(), pszValue ) );
}


int CTFPlayer::GetClientConVarIntValue( const char *pszValue )
{
	return atoi( engine->GetClientConVarValue( entindex(), pszValue ) );
}


float CTFPlayer::GetClientConVarFloatValue( const char *pszValue )
{
	return atof( engine->GetClientConVarValue( entindex(), pszValue ) );
}

extern IResponseSystem *g_pResponseSystem;
CChoreoScene *BlockingLoadScene( const char *filename );

bool WriteSceneFile( const char *filename )
{
	CChoreoScene *pScene = BlockingLoadScene( filename );
	if ( pScene )
	{
		if ( !filesystem->FileExists( filename ) )
		{
			char szDir[MAX_PATH];
			V_strcpy_safe( szDir, filename );
			V_StripFilename( szDir );

			filesystem->CreateDirHierarchy( szDir );
			if ( pScene->SaveToFile( filename ) )
			{
				Msg( "Wrote %s\n", filename );
			}
		}

		delete pScene;
		return true;
	}

	return false;
}

CON_COMMAND_F( tf2c_writevcds_rr, "Writes all VCD files referenced by response rules.", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CUtlVector<AI_Response *> responses;
	g_pResponseSystem->GetAllResponses( &responses );

	FOR_EACH_VEC( responses, i )
	{
		if ( responses[i]->GetType() != RESPONSE_SCENE )
			continue;

		// fixup $gender references
		char file[_MAX_PATH];
		V_strcpy_safe( file, responses[i]->GetNamePtr() );
		char *gender = strstr( file, "$gender" );
		if ( gender )
		{
			// replace with male & female
			const char *postGender = gender + strlen( "$gender" );
			*gender = 0;
			char genderFile[_MAX_PATH];
			// male
			V_sprintf_safe( genderFile, "%smale%s", file, postGender );

			WriteSceneFile( genderFile );

			V_sprintf_safe( genderFile, "%sfemale%s", file, postGender );

			WriteSceneFile( genderFile );
		}
		else
		{
			WriteSceneFile( file );
		}
	}

	responses.PurgeAndDeleteElements();
}

#ifdef ITEM_TAUNTING
CON_COMMAND_F( tf2c_writevcds_itemschema, "Writes all VCD files referenced by item schema entries.", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	GetItemSchema()->WriteScenes();
}
#endif

CON_COMMAND_F( tf2c_writevcd_name, "Writes VCD with the specified name.", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
		return;

	WriteSceneFile( args[1] );
}

extern ISoundEmitterSystemBase *soundemitterbase;

CON_COMMAND_F( tf2c_generatevcds, "Generates VCDs for all sounds of the player's class.", FCVAR_CHEAT )
{
	// Dedicated server has no sounds.
	if ( engine->IsDedicatedServer() )
		return;

	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	const char *pszClassname = g_aPlayerClassNames_NonLocalized[pPlayer->GetPlayerClass()->GetClassIndex()];
	int len = V_strlen( pszClassname );

	// Generate VCDs for all soundscripts starting with my class name.
	for ( int i = soundemitterbase->First(); i != soundemitterbase->InvalidIndex(); i = soundemitterbase->Next( i ) )
	{
		const char *pszSoundScriptName = soundemitterbase->GetSoundName( i );
		if ( !pszSoundScriptName || V_strnicmp( pszSoundScriptName, pszClassname, len ) != 0 )
			continue;

		const char *pszWavName = soundemitterbase->GetWavFileForSound( pszSoundScriptName, GENDER_MALE );

		float duration = enginesound->GetSoundDuration( pszWavName );
		if ( duration <= 0.0f )
		{
			Warning( "Couldn't determine duration of %s\n", pszWavName );
			continue;
		}

		CChoreoScene *pScene = new CChoreoScene( NULL );
		if ( !pScene )
		{
			Warning( "Failed to allocated new scene!!!\n" );
		}
		else
		{
			CChoreoActor *pActor = pScene->AllocActor();
			CChoreoChannel *pChannel = pScene->AllocChannel();
			CChoreoEvent *pEvent = pScene->AllocEvent();

			Assert( pActor );
			Assert( pChannel );
			Assert( pEvent );

			if ( !pActor || !pChannel || !pEvent )
			{
				Warning( "CSceneEntity::GenerateSceneForSound:  Alloc of actor, channel, or event failed!!!\n" );
				delete pScene;
				continue;
			}

			// Set us up the actorz
			pActor->SetName( pszClassname );  // Could be pFlexActor->GetName()?
			pActor->SetActive( true );

			// Set us up the channelz
			pChannel->SetName( "audio" );
			pChannel->SetActor( pActor );

			// Add to actor
			pActor->AddChannel( pChannel );

			// Set us up the eventz
			pEvent->SetType( CChoreoEvent::SPEAK );
			pEvent->SetName( pszSoundScriptName );
			pEvent->SetParameters( pszSoundScriptName );
			pEvent->SetStartTime( 0.0f );
			pEvent->SetUsingRelativeTag( false );
			pEvent->SetEndTime( duration );
			pEvent->SnapTimes();

			// Add to channel
			pChannel->AddEvent( pEvent );

			// Point back to our owners
			pEvent->SetChannel( pChannel );
			pEvent->SetActor( pActor );

			// Now write it to VCD file.
			char szFile[MAX_PATH];
			char szWavNameNoExt[MAX_PATH];
			V_StripExtension( V_GetFileName( pszWavName ), szWavNameNoExt, MAX_PATH );
			V_sprintf_safe( szFile, "scenes/player/%s/low/%s.vcd", pszClassname, szWavNameNoExt );

			if ( !filesystem->FileExists( szFile ) )
			{
				char szDir[MAX_PATH];
				V_strcpy_safe( szDir, szFile );
				V_StripFilename( szDir );

				filesystem->CreateDirHierarchy( szDir );
				if ( pScene->SaveToFile( szFile ) )
				{
					Msg( "Wrote %s\n", szFile );
				}
			}

			delete pScene;
		}
	}
}


float CTFPlayer::GetTimeSinceLastInjuryByAnyEnemyTeam() const
{
	float flTimeSinceInjured = FLT_MAX;

	ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeamNum ) {
		flTimeSinceInjured = Min( flTimeSinceInjured, GetTimeSinceLastInjury( iTeamNum ) );
		return true;
	} );

	return flTimeSinceInjured;
}

//-----------------------------------------------------------------------------
// Purpose: Set nearby nav areas "in-combat" so bots are aware of the danger
//-----------------------------------------------------------------------------
void CTFPlayer::OnMyWeaponFired( CBaseCombatWeapon *weapon )
{
	BaseClass::OnMyWeaponFired( weapon );

	if ( !m_ctNavCombatUpdate.IsElapsed() )
		return;

	auto pTFWeapon = static_cast<CTFWeaponBase *>( weapon );
	if ( pTFWeapon->TFNavMesh_ShouldRaiseCombatLevelWhenFired() )
	{
		m_ctNavCombatUpdate.Start( 1.0f );

		GetLastKnownTFArea()->AddCombatToSurroundingAreas();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks whether player is airborne and not underwater. Supports basic check for distance above ground.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAirborne (float flMinHeight)
{
	if (GetWaterLevel() <= WL_Feet && !GetGroundEntity()) // we aren't touching ground and not swimming, that's good
	{
		if (GetWaterLevel() == WL_Feet) // But seems like we are still touching water. 
			// It would be a shame to be airborne while waterjumping on surface of deep water. (2fort under bridge)
			// But we should be airborne if jumping off ground and still touching water with our tiptoes. (2fort sewer) 
			// Let's check it to make sure that there is so much space under us that we actually would be swimming
		{
			float flWaistZ = (GetPlayerMins().z + GetPlayerMaxs().z) * 0.5f + 12.0f;

			//trace_t trace;
			//CTraceFilterIgnorePlayers filter(this, COLLISION_GROUP_PROJECTILE); // Not sure about collision group, help
			//UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, flWaistZ), MASK_TFSHOT&(~CONTENTS_HITBOX), &filter, &trace);

			// below ray trace copies mostly from TracePlayerBBox of CTFGameMovement

			trace_t trace;
			CTraceFilterObject traceFilter(this, COLLISION_GROUP_PLAYER_MOVEMENT);
			UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, flWaistZ), GetPlayerMins(), GetPlayerMaxs(), PlayerSolidMask(), &traceFilter, &trace);

			if (trace.fraction == 1.0f)
				return false;
		}

		if (!flMinHeight)
			return true;
		//trace_t trace;
		//CTraceFilterIgnorePlayers filter(this, COLLISION_GROUP_PROJECTILE); // Not sure about collision group, help
		//UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -flMinHeight), MASK_TFSHOT&(~CONTENTS_HITBOX), &filter, &trace);

		trace_t trace;
		CTraceFilterObject traceFilter(this, COLLISION_GROUP_PLAYER_MOVEMENT);
		UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, flMinHeight), GetPlayerMins(), GetPlayerMaxs(), PlayerSolidMask(), &traceFilter, &trace);

		if (tf2c_debug_airblast_min_height_line.GetBool())
		{
			NDebugOverlay::Box(GetAbsOrigin(), GetPlayerMins(), GetPlayerMaxs(), 0, 255, 0, 100, 5);
			NDebugOverlay::Line(GetAbsOrigin(), trace.endpos, 0, 255, 0, true, 5);
			NDebugOverlay::Box(trace.endpos, GetPlayerMins(), GetPlayerMaxs(), 0, 0, 255, 100, 5);
			if (trace.m_pEnt)
				Msg("Airborne min height check hull trace managed to hit %s\n", trace.m_pEnt->GetDebugName());
		}
		return trace.fraction == 1.0f;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks whether player is wet. New conditions like under jarate or milk effect to be entered here as implemented.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsWet()
{
	float flWaterExitTime = GetWaterExitTime();

	return GetWaterLevel() > WL_NotInWater ||
		(flWaterExitTime > 0 && gpGlobals->curtime - flWaterExitTime < 5.0f);  // or they exited the water in the last few seconds.
}



void CTFPlayer::UpdateJumppadTrailEffect(void)
{
	if (!m_Shared.InCond(TF_COND_LAUNCHED) && m_pJumpPadSoundLoop)
	{
		CSoundEnvelopeController::GetController().SoundDestroy(m_pJumpPadSoundLoop);
		m_pJumpPadSoundLoop = nullptr;
	}

	if (m_Shared.InCond(TF_COND_LAUNCHED) && !m_pJumpPadSoundLoop)
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter(this);
		m_pJumpPadSoundLoop = controller.SoundCreate(filter, entindex(), "TFPlayer.JumppadJumpLoop");
		controller.Play(m_pJumpPadSoundLoop, 1.0, 100);
	}
}


void CTFPlayer::PlayTickTockSound(void)
{
	if ( !IsAlive() )
		return;

	CSingleUserRecipientFilter filter(this);
	if (!bTickTockOrder)
		EmitSound(filter, entindex(), "TFPlayer.Beacon_Tick");
	else
		EmitSound(filter, entindex(), "TFPlayer.Beacon_Tock");
	bTickTockOrder = !bTickTockOrder;
}

bool CTFPlayer::CanAirDash() const
{
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_SPEED_BOOST ) )
		return true;

	if ( IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( m_Shared.InCond( TF_COND_SODAPOPPER_HYPE ) )
			return true;

		int nAirDashCount = tf_scout_air_dash_count.GetInt();
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetActiveWeapon(), nAirDashCount, air_dash_count );

		int nScoutDoubleJumpDisabled = 0;
		CALL_ATTRIB_HOOK_INT( nScoutDoubleJumpDisabled, set_scout_doublejump_disabled );
		return nScoutDoubleJumpDisabled == 0;
	}

	return false;
}

bool CTFPlayer::CanGetWet() const
{
	int nWetImmunity = 0;
	CALL_ATTRIB_HOOK_INT( nWetImmunity, wet_immunity );
	return nWetImmunity == 0;
}

bool CTFPlayer::IsFireproof() const
{
	return m_Shared.InCond( TF_COND_FIRE_IMMUNE );
}

bool CTFPlayer::CanBreatheUnderwater() const
{
	if ( m_Shared.InCond( TF_COND_SWIMMING_CURSE ) )
		return true;

	int nCanBreatheUnderWater = 0;
	CALL_ATTRIB_HOOK_INT( nCanBreatheUnderWater, can_breathe_under_water );
	return nCanBreatheUnderWater != 0;
}

bool CTFPlayer::InAirDueToExplosion( void )
{ 
	return ( !( GetFlags() & FL_ONGROUND ) && ( GetWaterLevel() == WL_NotInWater ) && ( m_iBlastJumpState != 0 ) );
}

bool CTFPlayer::InAirDueToKnockback( void )
{
	return ( !( GetFlags() & FL_ONGROUND ) && ( GetWaterLevel() == WL_NotInWater ) && ( ( m_iBlastJumpState != 0 ) || m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) || m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) || m_Shared.InCond( TF_COND_GRAPPLINGHOOK_SAFEFALL ) ) );
}
