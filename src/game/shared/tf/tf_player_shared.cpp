//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "baseobject_shared.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_invis.h"
#include "in_buttons.h"
#include "tf_viewmodel.h"
#include "econ_wearable.h"
#include "tf_fx_shared.h"
#include "soundenvelope.h"
#include "tf_weapon_riot.h"
#include "tf_weapon_paintballrifle.h"

#ifdef TF2C_BETA
#include "tf_weapon_heallauncher.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "dt_utlvector_recv.h"
#include "prediction.h"
#include "tf_weapon_beacon.h"	// !!! foxysen beacon

#define CTFPlayerClass C_TFPlayerClass
#define CRecipientFilter C_RecipientFilter

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#include "tf_weapon_builder.h"
#include "tf_inventory.h"
#include "tf_weapon_invis.h"
#include "NextBotManager.h"
#include "tf_bot.h"
#include "dt_utlvector_send.h"
#include "tf_obj_dispenser.h"
#include "inetchannel.h"
#include "tf_lagcompensation.h"

#include "tf_weapon_beacon.h"	// !!! foxysen beacon

#include "achievements_tf.h"
#endif

#include "tf_randomizer_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_REPLICATED );	// no cap
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers", true, 1.0, false, 0 );
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_REPLICATED, "Time it takes for invulnerability to wear off" );

#ifdef GAME_DLL
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_NONE, "Time it takes for a full health boost to drain away from a playe.", true, 0.1, false, 0 );
ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30" );
#endif

ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );
ConVar tf2c_spy_cloak_ammo_refill( "tf2c_spy_cloak_ammo_refill", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows Spy to re-fill his cloak charge from ammo pickups" );

//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );

ConVar tf_damage_disablespread( "tf_damage_disablespread", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the random damage spread applied to all player damage" );
ConVar tf_always_loser( "tf_always_loser", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Force loserstate to true" );

extern ConVar weapon_medigun_chargerelease_rate;

// TF2C ConVars.
ConVar tf2c_building_hauling( "tf2c_building_hauling", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggle Engineer's building hauling ability" );
ConVar tf2c_spy_gun_mettle( "tf2c_spy_gun_mettle", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Spy's higher move speed, debuff reduction while cloaked, and damage to sapped Sentries. Value 2 only enables higher move speed." );

ConVar tf2c_infinite_ammo( "tf2c_infinite_ammo", "0", FCVAR_REPLICATED, "Enabled infinite ammo for all players. Weapons still need to be reloaded" );
ConVar tf2c_disable_player_shadows( "tf2c_disable_player_shadows", "0", FCVAR_REPLICATED, "Disables rendering of player shadows regardless of client's graphical settings" );
ConVar tf2c_disablefreezecam( "tf2c_disablefreezecam", "0", FCVAR_REPLICATED );

ConVar tf2c_ctf_carry_slow( "tf2c_ctf_carry_slow", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Makes flags slow down their carrier. 0 = DISABLED, 1 = ENABLED, 2 = BASED ON MULTIPLIER" );
ConVar tf2c_ctf_carry_slow_mult( "tf2c_ctf_carry_slow_mult", "0.85", FCVAR_REPLICATED, "Slows down flag carriers by a specified percentage" );
ConVar tf2c_ctf_carry_slow_blastjumps( "tf2c_ctf_carry_slow_blastjumps", "1", FCVAR_REPLICATED, "Also reduce blast jump force of flag carriers" );

ConVar tf2c_spywalk( "tf2c_spywalk", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows Disguised Spies to use their regular movement speed rather than their disguise's by using +SPEED" );
ConVar tf2c_taunting_detonate_stickies("tf2c_taunting_detonate_stickies", "1", FCVAR_REPLICATED, "Demoman can detonate stickies while taunting");

ConVar tf2c_afterburn_damage( "tf2c_afterburn_damage", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Afterburn damage per tick" );
ConVar tf2c_afterburn_time( "tf2c_afterburn_time", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "Afterburn duration" );

// !!! foxysen detonator
ConVar tf2c_detonator_movespeed_const( "tf2c_detonator_movespeed_const", "720", FCVAR_NOTIFY | FCVAR_REPLICATED, "Movespeed multiplier on detonator activation" );

ConVar tf2c_projectile_fraction_scout( "tf2c_projectile_fraction_scout", "0.3", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_soldier( "tf2c_projectile_fraction_soldier", "0.27", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_pyro( "tf2c_projectile_fraction_pyro", "0.25", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_demo( "tf2c_projectile_fraction_demo", "0.22", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_heavy( "tf2c_projectile_fraction_heavy", "0.11", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_engie( "tf2c_projectile_fraction_engie", "0.25", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_medic( "tf2c_projectile_fraction_medic", "0.15", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_sniper( "tf2c_projectile_fraction_sniper", "0.15", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_spy( "tf2c_projectile_fraction_spy", "0.125", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf2c_projectile_fraction_civ( "tf2c_projectile_fraction_civ", "0.35", FCVAR_NOTIFY | FCVAR_REPLICATED );

#ifdef TF2C_BETA
ConVar tf2c_medicgl_movespeed_mult( "tf2c_medicgl_movespeed_mult", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiplier for the default speed boost values, for the Nuernberg 'Nader UberCharge" );
#endif

#ifdef GAME_DLL
extern ConVar tf2c_allow_special_classes;
extern ConVar tf2c_vip_abilities;
#else
extern ConVar tf2c_medigun_heal_progress_sound;
#endif

extern ConVar tf2c_building_gun_mettle;
extern ConVar tf2c_allow_special_classes;
extern ConVar tf_meleeattackforcescale;

extern ConVar tf2c_mirv_impact_stun_secondary_movespeed;

#define TF_SPY_STEALTH_BLINKTIME			0.3f
#define TF_SPY_STEALTH_BLINKSCALE			0.85f

#define TF_BUILDING_PICKUP_RANGE			150
#define TF_BUILDING_RESCUE_MIN_RANGE_SQ		62500 // 250 * 250
#define TF_BUILDING_RESCUE_MAX_RANGE		5500

#define TF_PLAYER_CONDITION_CONTEXT			"TFPlayerConditionContext"

#define TF_CIVILIAN_BUFF_RADIUS				450.0f
#define TF_CIVILIAN_REBUFF_TIME				1.2f

#define TF2C_TRANQ_MOVE_SPEED_FACTOR		0.8f

#define MAX_DAMAGE_EVENTS					128

#define TF2C_MAX_STORED_CRITS				3	// Increase or decrease send int offset if you are to change it

const char *g_pszBDayGibs[22] =
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_TFPlayerConditionListExclusive );

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
RecvPropInt( RECVINFO( m_iDesiredWeaponID ) ),
RecvPropVector( RECVINFO( m_vecAirblastPos ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
RecvPropInt( RECVINFO( m_nPlayerCond ) ),
RecvPropInt( RECVINFO( m_nPlayerCondEx ) ),
RecvPropInt( RECVINFO( m_nPlayerCondEx2 ) ),
RecvPropInt( RECVINFO( m_nPlayerCondEx3 ) ),
RecvPropInt( RECVINFO( m_nPlayerCondEx4 ) ),
RecvPropInt( RECVINFO( m_bJumping ) ),
RecvPropInt( RECVINFO( m_nNumHealers ) ),
RecvPropInt( RECVINFO( m_nNumHumanHealers ) ),
RecvPropInt( RECVINFO( m_iCritMult ) ),
RecvPropInt( RECVINFO( m_bAirDash ) ),
RecvPropInt( RECVINFO( m_nAirDucked ) ),
RecvPropFloat( RECVINFO( m_flMovementStunTime ) ),
RecvPropInt( RECVINFO( m_iMovementStunAmount ) ),
RecvPropInt( RECVINFO( m_iMovementStunParity ) ),
RecvPropEHandle( RECVINFO( m_hStunner ) ),
RecvPropInt( RECVINFO( m_iStunFlags ) ),
RecvPropInt( RECVINFO( m_nPlayerState ) ),
RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
RecvPropArray3( RECVINFO_ARRAY( m_flCondExpireTimeLeft ), RecvPropFloat( RECVINFO( m_flCondExpireTimeLeft[0] ) ) ),
RecvPropArray3( RECVINFO_ARRAY( m_nPreventedDamageFromCondition ), RecvPropInt( RECVINFO( m_nPreventedDamageFromCondition[0] ) ) ),
RecvPropArray3( RECVINFO_ARRAY( m_bPrevActive ), RecvPropBool( RECVINFO( m_bPrevActive[0] ) ) ),
RecvPropTime( RECVINFO( m_flFlameRemoveTime ) ),
RecvPropEHandle( RECVINFO( m_hCarriedObject ) ),
RecvPropBool( RECVINFO( m_bCarryingObject ) ),
RecvPropInt( RECVINFO( m_nTeamTeleporterUsed ) ),
RecvPropFloat( RECVINFO( m_flChargeTooSlowTime ) ),

// Spy.
RecvPropFloat( RECVINFO( m_flInvisibility ) ),
RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
RecvPropInt( RECVINFO( m_nMaskClass ) ),
RecvPropInt( RECVINFO( m_iDisguiseTargetIndex ) ),
RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
RecvPropInt( RECVINFO( m_iDisguiseMaxHealth ) ),
RecvPropFloat( RECVINFO( m_flDisguiseChargeLevel ) ),
RecvPropInt( RECVINFO( m_nDisguiseBody ) ),
RecvPropFloat( RECVINFO( m_flCloakMeter ) ),
RecvPropEHandle( RECVINFO( m_hDisguiseWeapon ) ),
RecvPropInt( RECVINFO( m_iDisguiseClip ) ),
RecvPropInt( RECVINFO( m_iDisguiseAmmo ) ),

// TF2Classic
RecvPropTime( RECVINFO( m_flNextCivilianBuffCheckTime ) ),
//RecvPropBool( RECVINFO( m_bEmittedResistanceBuff ) ),

RecvPropInt( RECVINFO( m_iStoredCrits ) ),

RecvPropInt( RECVINFO( m_nRestoreBody ) ),
RecvPropInt( RECVINFO( m_nRestoreDisguiseBody ) ),
RecvPropInt( RECVINFO( m_iPlayerKillCount ) ),

// Local Data.
RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFPlayerSharedLocal ) ),
RecvPropDataTable( RECVINFO_DT( m_ConditionList ),0, &REFERENCE_RECV_TABLE( DT_TFPlayerConditionListExclusive ) ),

RecvPropInt( RECVINFO( m_iStunIndex ) ),

RecvPropUtlVector( RECVINFO_UTLVECTOR( m_vecProvider ), TF_COND_LAST, RecvPropEHandle( NULL, 0 ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_nPlayerCondEx, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_nPlayerCondEx2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD(m_nPlayerCondEx3, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nPlayerCondEx4, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD( m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bAirDash, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_nAirDucked, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bScaredEffects, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_flInvisibility, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
//DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
// Server specific.
#else
EXTERN_SEND_TABLE( DT_TFPlayerConditionListExclusive );

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
SendPropInt( SENDINFO( m_iDesiredWeaponID ) ),
SendPropVector( SENDINFO( m_vecAirblastPos ), -1, SPROP_COORD ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nPlayerCondEx ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nPlayerCondEx2 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nPlayerCondEx3 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nPlayerCondEx4 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),

SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropInt( SENDINFO( m_nNumHumanHealers ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropInt( SENDINFO( m_bAirDash ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropInt( SENDINFO( m_nAirDucked ), 2, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
SendPropFloat( SENDINFO( m_flMovementStunTime ) ),
SendPropInt( SENDINFO( m_iMovementStunAmount ), 8, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iMovementStunParity ), MOVEMENTSTUN_PARITY_BITS, SPROP_UNSIGNED ),
SendPropEHandle( SENDINFO( m_hStunner ) ),
SendPropInt( SENDINFO( m_iStunFlags ), 12, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT ) + 1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL ) + 1, SPROP_UNSIGNED ),
SendPropArray3( SENDINFO_ARRAY3( m_flCondExpireTimeLeft ), SendPropFloat( SENDINFO_ARRAY( m_flCondExpireTimeLeft ) ) ),
SendPropArray3( SENDINFO_ARRAY3( m_nPreventedDamageFromCondition ), SendPropInt( SENDINFO_ARRAY( m_nPreventedDamageFromCondition ) ) ),
SendPropArray3( SENDINFO_ARRAY3( m_bPrevActive ), SendPropBool( SENDINFO_ARRAY( m_bPrevActive ) ) ),
SendPropTime( SENDINFO( m_flFlameRemoveTime ) ),
SendPropEHandle( SENDINFO( m_hCarriedObject ) ),
SendPropBool( SENDINFO( m_bCarryingObject ) ),
SendPropInt( SENDINFO( m_nTeamTeleporterUsed ), 3, SPROP_UNSIGNED ),
SendPropFloat( SENDINFO( m_flChargeTooSlowTime ) ),

// Spy
SendPropFloat( SENDINFO( m_flInvisibility ), 32, SPROP_CHANGES_OFTEN, 0.0f, 1.0f ),
SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nMaskClass ), 4, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iDisguiseTargetIndex ), Q_log2( MAX_PLAYERS ) + 1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iDisguiseHealth ), 10 ),
SendPropInt( SENDINFO( m_iDisguiseMaxHealth ), 10 ),
SendPropFloat( SENDINFO( m_flDisguiseChargeLevel ) ),
SendPropInt( SENDINFO( m_nDisguiseBody ) ),
SendPropFloat( SENDINFO( m_flCloakMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0, 100.0 ),
SendPropEHandle( SENDINFO( m_hDisguiseWeapon ) ),
SendPropInt( SENDINFO( m_iDisguiseClip ) ),
SendPropInt( SENDINFO( m_iDisguiseAmmo ) ),

// TF2Classic
SendPropTime( SENDINFO( m_flNextCivilianBuffCheckTime ) ),
//SendPropBool( SENDINFO( m_bEmittedResistanceBuff ) ),

SendPropInt( SENDINFO( m_iStoredCrits ), 2, SPROP_UNSIGNED ),

SendPropInt( SENDINFO( m_nRestoreBody ) ),
SendPropInt( SENDINFO( m_nRestoreDisguiseBody ) ),

SendPropInt( SENDINFO( m_iPlayerKillCount ), 8, SPROP_UNSIGNED ),

// Local Data.
SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),
SendPropDataTable( SENDINFO_DT( m_ConditionList ), &REFERENCE_SEND_TABLE( DT_TFPlayerConditionListExclusive ) ),

SendPropInt( SENDINFO( m_iStunIndex ), 8 ),

SendPropUtlVector( SENDINFO_UTLVECTOR( m_vecProvider ), TF_COND_LAST, SendPropEHandle( NULL, 0 ) ),
END_SEND_TABLE()
#endif


// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_bAirDash = false;
	m_nAirDucked = 0;
	m_bScaredEffects = false;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;
	m_bLastDisguiseWasEnemyTeam = true;

	m_flCloakConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
	m_flCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();

	m_iDesiredWeaponID = -1;

	m_hStunner = NULL;
	m_iStunFlags = 0;
	m_flLastMovementStunChange = 0;
	m_bStunNeedsFadeOut = false;
	m_iStunIndex = -1;

	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;

	m_flChargeTooSlowTime = 0.0f;

	m_iPlayerKillCount = 0;

#ifdef CLIENT_DLL
	m_iDisguiseTargetIndex = 0;
	m_pCritSound = NULL;
	m_pCritEffect = NULL;
	m_pBurningEffect = NULL;
	m_pBurningSound = NULL;
	m_pDisguisingEffect = NULL;
	m_pTranqEffect = NULL;
	m_pTranqSound = NULL;
	m_pScaredEffect = NULL;
	m_pResistanceBuffEffect = NULL;

	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;
#else
	m_flTranqStartTime = 0;
	m_nNumHumanHealers = 0;
	memset( m_flChargeOffTime, 0, sizeof( m_flChargeOffTime ) );
	memset( m_bChargeSounds, 0, sizeof( m_bChargeSounds ) );
#endif

	// Make sure we have all conditions in the list.
	m_vecProvider.EnsureCount( TF_COND_LAST );
}

void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	m_iStunAnimState = STUN_ANIM_NONE;
	m_hStunner = NULL;

	SetJumping( false );

	Spawn();
}

void CTFPlayerShared::Spawn( void )
{
	SetAssist( NULL );

#ifdef GAME_DLL
	m_PlayerStuns.RemoveAll();
	m_iStunIndex = -1;
	m_PlayerDeathBurns.RemoveAll();
#else
	m_bSyncingConditions = false;
#endif

	m_iStoredCrits = 0;
	m_iPlayerKillCount = 0;

	m_bEmittedResistanceBuff = false;
}


template < typename tIntType >
class CConditionVars
{
public:
	CConditionVars(tIntType& nPlayerCond, tIntType& nPlayerCondEx, tIntType& nPlayerCondEx2, tIntType& nPlayerCondEx3, tIntType& nPlayerCondEx4, ETFCond eCond)
	{
		if (eCond >= 128)
		{
			Assert(eCond < 128 + 32);
			m_pnCondVar = &nPlayerCondEx4;
			m_nCondBit = eCond - 128;
		}
		else if ( eCond >= 96 )
		{
			Assert( eCond < 96 + 32 );
			m_pnCondVar = &nPlayerCondEx3;
			m_nCondBit = eCond - 96;
		}
		else if ( eCond >= 64 )
		{
			Assert( eCond < (64 + 32) );
			m_pnCondVar = &nPlayerCondEx2;
			m_nCondBit = eCond - 64;
		}
		else if ( eCond >= 32 )
		{
			Assert( eCond < (32 + 32) );
			m_pnCondVar = &nPlayerCondEx;
			m_nCondBit = eCond - 32;
		}
		else
		{
			m_pnCondVar = &nPlayerCond;
			m_nCondBit = eCond;
		}
	}

	tIntType& CondVar() const
	{
		return *m_pnCondVar;
	}

	int CondBit() const
	{
		return 1 << m_nCondBit;
	}

private:
	tIntType *m_pnCondVar;
	int m_nCondBit;

};

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayerShared::AddCond( ETFCond eCond, float flDuration /* = PERMANENT_CONDITION */, CBaseEntity *pProvider /*= NULL */ )
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );

	// If we're dead, don't take on any new conditions.
	if ( !m_pOuter || !m_pOuter->IsAlive() )
		return;

#ifdef CLIENT_DLL
	if ( m_pOuter->IsDormant() )
		return;
#endif

	// Which bitfield are we tracking this condition variable in? Which bit within
	// that variable will we track it as?
	CConditionVars<int> cPlayerCond(m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond);

	// See if there is an object representation of the condition.
	bool bAddedToExternalConditionList = m_ConditionList.Add( eCond, flDuration, m_pOuter, pProvider );
	if ( !bAddedToExternalConditionList )
	{
		// Set the condition bit for this condition.
		cPlayerCond.CondVar() |= cPlayerCond.CondBit();

		// Flag for gamecode to query.
		m_bPrevActive.Set( eCond, m_flCondExpireTimeLeft[eCond] != 0.0f ? true : false );

		if ( flDuration != PERMANENT_CONDITION )
		{
			// if our current condition is permanent or we're trying to set a new
			// time that's less our current time remaining, use our current time instead
			if ( m_flCondExpireTimeLeft[eCond] == PERMANENT_CONDITION || flDuration < m_flCondExpireTimeLeft[eCond] )
			{
				flDuration = m_flCondExpireTimeLeft[eCond];
			}
		}

		m_flCondExpireTimeLeft.Set( eCond, flDuration );
		m_vecProvider[eCond].Set( pProvider );
		m_nPreventedDamageFromCondition.Set( eCond, 0 );

		OnConditionAdded( eCond );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayerShared::RemoveCond( ETFCond eCond, bool ignore_duration )
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	if ( !InCond( eCond ) )
		return;

	// If this variable is handled by the condition list, abort before doing the
	// work for the condition flags.
	if ( m_ConditionList.Remove( eCond, ignore_duration ) )
		return;

	CConditionVars<int> cPlayerCond(m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond);
	cPlayerCond.CondVar() &= ~cPlayerCond.CondBit();
	OnConditionRemoved( eCond );

	if ( m_nPreventedDamageFromCondition[eCond] )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "damage_prevented" );
		if ( pEvent )
		{
			CBaseEntity *pProvider = m_vecProvider[eCond].Get();
			pEvent->SetInt( "preventor", pProvider ? pProvider->entindex() : m_pOuter->entindex() );
			pEvent->SetInt( "victim", m_pOuter->entindex() );
			pEvent->SetInt( "amount", m_nPreventedDamageFromCondition[eCond] );
			pEvent->SetInt( "condition", eCond );

			gameeventmanager->FireEvent( pEvent, true );
		}

		m_nPreventedDamageFromCondition.Set( eCond, 0 );
	}

	m_flCondExpireTimeLeft.Set( eCond, 0 );
	m_vecProvider[eCond].Set( NULL );
	m_bPrevActive.Set( eCond, false );
}


bool CTFPlayerShared::InCond( ETFCond eCond ) const
{
	// Old condition system, only used for the first 32 conditions.
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	if ( eCond < 32 && m_ConditionList.InCond( eCond ) )
		return true;

	CConditionVars<const int> cPlayerCond(m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond);
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return whether or not we were in this condition before.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::WasInCond( ETFCond eCond ) const
{
	// I don't know if this actually works for conditions < 32, because we definitely cannot peak into m_ConditionList (back in time).
	// But others think that m_ConditionList is propogated into m_nOldConditions, so just check if you hit the assert
	// (And then remove the assert and this comment).
	Assert( eCond >= 32 && eCond < TF_COND_LAST );

	CConditionVars<const int> cPlayerCond( m_nOldConditions, m_nOldConditionsEx, m_nOldConditionsEx2, m_nOldConditionsEx3, m_nOldConditionsEx4, eCond );
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Set a bit to force this condition off and then back on next time we sync bits from the server. 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ForceRecondNextSync( ETFCond eCond )
{
	// I don't know if this actually works for conditions < 32. We may need to set this bit in m_ConditionList, too.
	// Please check if you hit the assert (And then remove the assert. And this comment).
	Assert( eCond >= 32 && eCond < TF_COND_LAST );

	CConditionVars<int> playerCond(m_nForceConditions, m_nForceConditionsEx, m_nForceConditionsEx2, m_nForceConditionsEx3, m_nForceConditionsEx4, eCond);
	playerCond.CondVar() |= playerCond.CondBit();
}


float CTFPlayerShared::GetConditionDuration( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	if ( InCond( eCond ) )
		return m_flCondExpireTimeLeft[eCond];

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that provided the passed in condition
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionProvider( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	if ( InCond( eCond ) )
		return eCond == TF_COND_CRITBOOSTED ? m_ConditionList.GetProvider( eCond ) : m_vecProvider[eCond].Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that applied this condition to us - for granting an assist when we kill someone
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionAssistFromAttacker( void )
{
	// We only give an assist to one person. That means this list is order
	// sensitive, so consider how "powerful" an effect is when adding it here.
	for ( int i = 0; g_eAttackerAssistConditions[i] != TF_COND_LAST; i++ )
	{
		if ( InCond( g_eAttackerAssistConditions[i] ) )
		{
			// Check to make sure we're not providing the condition to ourselves.
			CBaseEntity *pPotentialProvider = GetConditionProvider( g_eAttackerAssistConditions[i] );
			if ( pPotentialProvider != m_pOuter )
				return pPotentialProvider;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that applied this condition to us - for granting an assist when we die.
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionAssistFromVictim( void )
{
	// We only give an assist to one person. That means this list is order
	// sensitive, so consider how "powerful" an effect is when adding it here.
	for ( int i = 0; g_eVictimAssistConditions[i] != TF_COND_LAST; i++ )
	{
		if ( InCond( g_eVictimAssistConditions[i] ) )
			return GetConditionProvider( g_eVictimAssistConditions[i] );
	}

	return NULL;
}


void CTFPlayerShared::DebugPrintConditions( void )
{
#ifndef CLIENT_DLL
	static const char *szDll = "SERVER";
#else
	static const char *szDll = "CLIENT";
#endif

	Msg( "[%s] Conditions for player (%d)\n", szDll, m_pOuter->entindex() );

	int iNumFound = 0;
	for ( int i = 0; i < TF_COND_LAST; i++ )
	{
		if ( InCond( (ETFCond)i ) )
		{
			if ( m_flCondExpireTimeLeft[i] == PERMANENT_CONDITION )
			{
				Msg( "[%s] Condition %d - (permanent cond)\n", szDll, i );
			}
			else
			{
				Msg( "[%s] Condition %d - (%.1f left)\n", szDll, i, m_flCondExpireTimeLeft[i] );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "[%s] No active conditions\n", szDll );
	}
}


bool CTFPlayerShared::IsCritBoosted( void ) const
{
	// Oh man...
	return (InCond( TF_COND_CRITBOOSTED ) ||
		InCond( TF_COND_CRITBOOSTED_PUMPKIN ) ||
		InCond( TF_COND_CRITBOOSTED_USER_BUFF ) ||
		InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) ||
		InCond( TF_COND_CRITBOOSTED_FIRST_BLOOD ) ||
		InCond( TF_COND_CRITBOOSTED_BONUS_TIME ) ||
		InCond( TF_COND_CRITBOOSTED_CTF_CAPTURE ) ||
		InCond( TF_COND_CRITBOOSTED_ON_KILL ) ||
		InCond( TF_COND_CRITBOOSTED_CARD_EFFECT ) ||
		InCond( TF_COND_CRITBOOSTED_RUNE_TEMP ) ||
		InCond( TF_COND_CRITBOOSTED_HIDDEN ));
}


bool CTFPlayerShared::IsInvulnerable( void ) const
{
	// Oh man again...
	return (InCond( TF_COND_INVULNERABLE ) ||
		InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) ||
		InCond( TF_COND_INVULNERABLE_USER_BUFF ) ||
		InCond( TF_COND_INVULNERABLE_CARD_EFFECT ));
}


bool CTFPlayerShared::IsDisguised( void ) const
{
	return InCond( TF_COND_DISGUISED );
}


bool CTFPlayerShared::IsStealthed( void ) const
{
	return (InCond( TF_COND_STEALTHED ) || InCond( TF_COND_STEALTHED_USER_BUFF ));
}

bool CTFPlayerShared::IsImmuneToPushback( void ) const
{
	if ( InCond( TF_COND_MEGAHEAL ) )
		return true;

	if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( InCond( TF_COND_AIMING ) )
		{
			int nSpunUpPushForceImmunity = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nSpunUpPushForceImmunity, spunup_push_force_immunity );

			return nSpunUpPushForceImmunity != 0;
		}
	}

	return false;
}


bool CTFPlayerShared::IsMovementLocked( void )
{
	return InCond( TF_COND_TAUNTING ) || IsControlStunned();
}

bool CTFPlayerShared::CanBeDebuffed( void ) const
{
	if( IsInvulnerable() )
		return false;

	if ( InCond( TF_COND_PHASE ) )
		return false;

	if ( InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return false;

	return true;
}

#ifdef CLIENT_DLL

void CTFPlayerShared::OnPreDataChanged( void )
{
	m_ConditionList.OnPreDataChanged();

	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;

	m_nOldDisguiseClass = GetDisguiseClass();
	m_nOldDisguiseTeam = GetDisguiseTeam();

	m_bWasCritBoosted = IsCritBoosted();

	m_flOldFlameRemoveTime = m_flFlameRemoveTime;

	m_iOldMovementStunParity = m_iMovementStunParity;
}


void CTFPlayerShared::OnDataChanged( void )
{
	m_ConditionList.OnDataChanged( m_pOuter );

	if ( m_iOldMovementStunParity != m_iMovementStunParity )
	{
		m_flStunFade = gpGlobals->curtime + m_flMovementStunTime;
		m_flStunEnd  = m_flStunFade;
		if ( IsControlStunned() && ( m_iStunAnimState == STUN_ANIM_NONE ) )
		{
			m_flStunEnd += CONTROL_STUN_ANIM_TIME;
		}

		UpdateLegacyStunSystem();
	}

	// Update conditions from last network change.
	SyncConditions( m_nOldConditions, m_nPlayerCond, m_nForceConditions, 0 );
	SyncConditions( m_nOldConditionsEx, m_nPlayerCondEx, m_nForceConditionsEx, 32 );
	SyncConditions( m_nOldConditionsEx2, m_nPlayerCondEx2, m_nForceConditionsEx2, 64 );
	SyncConditions( m_nOldConditionsEx3, m_nPlayerCondEx3, m_nForceConditionsEx3, 96 );
	SyncConditions( m_nOldConditionsEx4, m_nPlayerCondEx4, m_nForceConditionsEx4, 128 );

	// Make sure these items are present.
	m_nPlayerCond |= m_nForceConditions;
	m_nPlayerCondEx |= m_nForceConditionsEx;
	m_nPlayerCondEx2 |= m_nForceConditionsEx2;
	m_nPlayerCondEx3 |= m_nForceConditionsEx3;
	m_nPlayerCondEx4 |= m_nForceConditionsEx4;

	// Clear our force bits now that we've used them.
	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;

	if ( m_nOldDisguiseClass != GetDisguiseClass() || m_nOldDisguiseTeam != GetDisguiseTeam() )
	{
		OnDisguiseChanged();
	}

	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->UpdateVisibility();
	}

	if ( m_flFlameRemoveTime != m_flOldFlameRemoveTime )
	{
		m_pOuter->m_flBurnRenewTime = gpGlobals->curtime;
	}

	CTFWeaponBase *pActiveTFWeapon = GetActiveTFWeapon();
	if ( IsLoser() && pActiveTFWeapon && !pActiveTFWeapon->IsEffectActive( EF_NODRAW ) )
	{
		pActiveTFWeapon->SetWeaponVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes.
//-----------------------------------------------------------------------------
void CTFPlayerShared::SyncConditions( int nPreviousConditions, int nNewConditions, int nForceConditions, int nBaseCondBit )
{
	if ( nPreviousConditions == nNewConditions )
		return;

	int nCondChanged = nNewConditions ^ nPreviousConditions;
	int nCondAdded = nCondChanged & nNewConditions;
	int nCondRemoved = nCondChanged & nPreviousConditions;
	m_bSyncingConditions = true;

	for ( int i = 0; i < 32; i++ )
	{
		const int testBit = 1 <<i ;
		if ( nForceConditions & testBit )
		{
			if ( nPreviousConditions & testBit )
			{
				OnConditionRemoved( (ETFCond)( nBaseCondBit + i ) );
			}

			OnConditionAdded( (ETFCond)( nBaseCondBit + i ) );
		}
		else
		{
			if ( nCondAdded & testBit )
			{
				OnConditionAdded( (ETFCond)( nBaseCondBit + i ) );
			}
			else if ( nCondRemoved & testBit )
			{
				OnConditionRemoved( (ETFCond)( nBaseCondBit + i)  );
			}
		}
	}

	m_bSyncingConditions = false;
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond( void )
{
	m_ConditionList.RemoveAll();

	for ( int i = 0; i < TF_COND_LAST; i++ )
	{
		ETFCond eCond = (ETFCond)i;
		if ( InCond( eCond ) )
		{
			RemoveCond( eCond );
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
	m_nPlayerCondEx = 0;
	m_nPlayerCondEx2 = 0;
	m_nPlayerCondEx3 = 0;
	m_nPlayerCondEx4 = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it recieves the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded( ETFCond nCond )
{
	switch ( nCond )
	{
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0.0f;
		m_flDisguiseHealFraction = 0.0f;
#endif
		break;

	case TF_COND_STEALTHED:
	case TF_COND_STEALTHED_USER_BUFF:
		OnAddStealthed();
		break;

	case TF_COND_INVULNERABLE:
	case TF_COND_INVULNERABLE_USER_BUFF:
	case TF_COND_INVULNERABLE_CARD_EFFECT:
		OnAddInvulnerable();
		break;

	case TF_COND_TELEPORTED:
    case TF_COND_TELEPORTED_ALWAYS_SHOW:
		OnAddTeleported();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_TAUNTING:
		OnAddTaunting();
		break;

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
		//case TF_COND_CRITBOOSTED_HIDDEN: deliberate skip
#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
		// Assert( !"TF_COND_CRITBOOSTED should be handled by the condition list!" );
		break;

	case TF_COND_SHIELD_CHARGE:
		OnAddShieldCharge();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif
		break;

	case TF_COND_TRANQUILIZED:
		OnAddSlowed();
		break;

	case TF_COND_CIV_SPEEDBUFF:
		OnAddCivSpeedBoost();
		break;

	case TF_COND_DISGUISED_AS_DISPENSER:
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnAddHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnAddHalloweenTiny();
		break;

	case TF_COND_STUNNED:
		OnAddStunned();
		break;

	case TF_COND_BLEEDING:
		OnAddBleeding();
		break;

	case TF_COND_MARKEDFORDEATH:
	case TF_COND_SUPERMARKEDFORDEATH:
		OnAddMarkedForDeath();
		break;

	case TF_COND_MARKEDFORDEATH_SILENT:
	case TF_COND_SUPERMARKEDFORDEATH_SILENT:
		OnAddMarkedForDeathSilent();
		break;

	case TF_COND_CANNOT_SWITCH_FROM_MELEE:
		OnAddMeleeOnly();
		break;

	case TF_COND_CIV_DEFENSEBUFF:
		OnAddCivResistanceBoost();
		break;

	case TF_COND_RESISTANCE_BUFF:
		OnAddResistanceBuff();
		break;

		// !!! foxysen detonator
	case TF_COND_SPEEDBOOST_DETONATOR:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif
		break;

	case TF_COND_LAUNCHED:
		OnAddLaunched();
		break;

	case TF_COND_RADIAL_UBER:
		AddCond( TF_COND_DAMAGE_BOOST );
		AddCond( TF_COND_CIV_DEFENSEBUFF );
		//AddCond( TF_COND_SPEED_BOOST );
		break;
	case TF_COND_SPEED_BOOST:
	case TF_COND_MIRV_SLOW:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif
		break;
	case TF_COND_MARKED_OUTLINE:
#ifdef GAME_DLL
		// Alert enemy bots to the outlining
		TheNextBots().ForEachBot( [&]( INextBot *nextbot ){
			if ( m_pOuter->IsEnemy( nextbot->GetEntity() ) )
			{
				nextbot->GetVisionInterface()->AddKnownEntity( this->m_pOuter );
			}
			return true;
		} );
#endif
		break;
	default:
		break;
	}

	// Tell the weapon that their owner got a condition added onto them.
	CTFWeaponBase *pActiveWeapon = m_pOuter->GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		pActiveWeapon->OwnerConditionAdded( nCond );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it recieves the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved( ETFCond nCond )
{
	switch ( nCond )
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0.0f;
		m_flDisguiseHealFraction = 0.0f;
#endif
		break;

	case TF_COND_STEALTHED:
		OnRemoveStealthed();
		break;

	case TF_COND_STEALTHED_USER_BUFF:
		OnRemoveStealthed();
		FadeInvis( tf_spy_invis_unstealth_time.GetFloat(), false );
		break;

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
	case TF_COND_INVULNERABLE_USER_BUFF:
	case TF_COND_INVULNERABLE_CARD_EFFECT:
		OnRemoveInvulnerable();
		break;

    case TF_COND_TELEPORTED:
    case TF_COND_TELEPORTED_ALWAYS_SHOW:
        OnRemoveTeleported();
		break;

	case TF_COND_TAUNTING:
		OnRemoveTaunting();
		break;

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
		//case TF_COND_CRITBOOSTED_HIDDEN: deliberate skip
#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
		// Assert( !"TF_COND_CRITBOOSTED should be handled by the condition list!" );
		break;

	case TF_COND_SHIELD_CHARGE:
		OnRemoveShieldCharge();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif
		break;

	case TF_COND_TRANQUILIZED:
		OnRemoveSlowed();
		break;

	case TF_COND_CIV_SPEEDBUFF:
		OnRemoveCivSpeedBoost();
		break;

	case TF_COND_DISGUISED_AS_DISPENSER:
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnRemoveHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnRemoveHalloweenTiny();
		break;

	case TF_COND_STUNNED:
		OnRemoveStunned();
		break;

	case TF_COND_BLEEDING:
		OnRemoveBleeding();
		break;

	case TF_COND_MARKEDFORDEATH:
	case TF_COND_SUPERMARKEDFORDEATH:
		OnRemoveMarkedForDeath();
		break;

	case TF_COND_MARKEDFORDEATH_SILENT:
	case TF_COND_SUPERMARKEDFORDEATH_SILENT:
		OnRemoveMarkedForDeathSilent();
		break;

	case TF_COND_AIRBLASTED:
		m_vecAirblastPos = vec3_origin;
		break;

	case TF_COND_CANNOT_SWITCH_FROM_MELEE:
		OnRemoveMeleeOnly();
		break;

	case TF_COND_CIV_DEFENSEBUFF:
		OnRemoveCivResistanceBoost();
		break;

	case TF_COND_RESISTANCE_BUFF:
		OnRemoveResistanceBuff();
		break;

		// !!! foxysen detonator
	case TF_COND_SPEEDBOOST_DETONATOR:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif
		break;

	case TF_COND_LAUNCHED:
		OnRemoveLaunched();
		break;

	case TF_COND_RADIAL_UBER:
		RemoveCond( TF_COND_CIV_DEFENSEBUFF );
		RemoveCond( TF_COND_DAMAGE_BOOST );
		//RemoveCond( TF_COND_SPEED_BOOST );
		break;

	case TF_COND_SPEED_BOOST:
	case TF_COND_MIRV_SLOW:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif
		break;
	case TF_COND_MARKED_OUTLINE:
		// Bots should no longer know where this guy is
#ifdef GAME_DLL
		// Alert enemy bots to the outlining
		TheNextBots().ForEachBot( [&]( INextBot *nextbot ){
			if ( m_pOuter->IsEnemy( nextbot->GetEntity() ) )
			{
				if ( nextbot->GetVisionInterface()->IsAbleToSee( m_pOuter->GetAbsOrigin(), IVision::DISREGARD_FOV ) )
				{
					nextbot->GetVisionInterface()->ForgetEntity( this->m_pOuter );
				}
			}
			return true;
		} );
#endif
		break;
	default:
		break;
	}

#ifdef GAME_DLL
	// HACK: This already runs on the server in CTFPlayerShared::OnRemoveTaunting()
	if ( nCond == TF_COND_TAUNTING )
		return;
#endif

	// Tell the weapon that their owner got a condition removed onto them.
	CTFWeaponBase *pActiveWeapon = m_pOuter->GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		pActiveWeapon->OwnerConditionRemoved( nCond );
	}
}

int CTFPlayerShared::GetMaxBuffedHealth( void ) const
{
    Assert(m_pOuter);
    if (!m_pOuter)
    {
        // Error("NULL m_pOuter in %s\n", __FUNCTION__);
        return 0;
    }

    float flMultOverheal = 1.0f;
    // healer (==m_pOuter) overheal penalty
    if (m_pOuter)
    {
        CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_pOuter, flMultOverheal, mult_patient_overheal_penalty);
    }
    // patient(==m_pOuter->tfWeapon->Target) overheal penalty
    if ( m_pOuter && m_pOuter->GetActiveTFWeapon() )
    {
        CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_pOuter->GetActiveTFWeapon(), flMultOverheal, mult_patient_overheal_penalty_active);
    }
    
	return m_pOuter->GetMaxHealth() + Floor2Int( ((m_pOuter->GetMaxHealth() * tf_max_health_boost.GetFloat()) - m_pOuter->GetMaxHealth()) * flMultOverheal / 5.0f ) * 5;
}

int CTFPlayerShared::GetDisguiseMaxBuffedHealth( void ) const
{
	// We aren't giving spy any attributes
	return Floor2Int( (GetDisguiseMaxHealth() * tf_max_health_boost.GetFloat()) / 5.0f ) * 5;
}


bool CTFPlayerShared::HealNegativeConds( void )
{
	bool bSuccess = false;
	for ( int i = 0; g_eDebuffConditions[i] != TF_COND_LAST; i++ )
	{
		if ( InCond( g_eDebuffConditions[i] ) )
		{
			RemoveCond( g_eDebuffConditions[i] );
			bSuccess = true;
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL
	m_ConditionList.ServerThink();

	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	for ( int i = 0; i < TF_COND_LAST; ++i )
	{
		// if we're in this condition and it's not already being handled by the condition list
		if ( InCond( (ETFCond)i ) && (i >= 32 || !m_ConditionList.InCond( (ETFCond)i )) )
		{
			// Ignore permanent conditions
			// (TF_COND_BURNING and TF_COND_BLEEDING)
			if ( m_flCondExpireTimeLeft[i] != PERMANENT_CONDITION )
			{
				// If we're being healed, we reduce bad conditions faster.
				float flReduction = gpGlobals->frametime;
				if ( ConditionExpiresFast( (ETFCond)i ) && m_vecHealers.Count() > 0 )
				{

					if ( i == TF_COND_URINE || i == TF_COND_TRANQUILIZED )
					{
						flReduction += (m_vecHealers.Count() * flReduction);
					}
					else // TF_COND_MAD_MILK
					{
						flReduction += (m_vecHealers.Count() * flReduction * 4);
					}
				}

				m_flCondExpireTimeLeft.Set( i, Max( m_flCondExpireTimeLeft[i] - flReduction, 0.0f ) );

				if ( m_flCondExpireTimeLeft[i] <= 0.0f )
				{
					RemoveCond( (ETFCond)i );
				}
			}
			else
			{
#if !defined( DEBUG )
				// Prevent hacked usercommand exploits!
				if ( m_pOuter->GetTimeSinceLastUserCommand() > 5.0f /*|| m_pOuter->GetTimeSinceLastThink() > 5.0f*/ )
				{
					RemoveCond( (ETFCond)i );

					// ShouldRemoveConditionOnTimeout

					// Reset active weapon to prevent stale-state bugs.
					CTFWeaponBase *pTFWeapon = m_pOuter->GetActiveTFWeapon();
					if ( pTFWeapon )
					{
						pTFWeapon->WeaponReset();
					}

					m_pOuter->TeamFortress_SetSpeed();
					m_pOuter->TeamFortress_SetGravity();
				}
#endif
			}
		}
	}

	// Our health will only decay (from being medic buffed) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s.
	bool bDecayHealth = true, bDecayDisguiseHealth = true;
	bool bDisguised = IsDisguised();

	// If we're being healed, heal ourselves.
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10.0f, 15.0f, 1.0f, 3.0f );

		bool bHasFullHealth = m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth();
		bool bHasFullDisguiseHealth = m_iDisguiseHealth >= m_iDisguiseMaxHealth;

		float flTotalHealAmount = 0.0f;
		float flTotalFakeHealAmount = 0.0f;
		float flHealFractionThisFrame = 0.0f;

		float flMedigunHealScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flMedigunHealScale, mult_medigun_healing_received );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter->GetActiveTFWeapon(), flMedigunHealScale, mult_medigun_healing_received_active );
		float flDispenserHealScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flDispenserHealScale, mult_dispenser_healing_received );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter->GetActiveTFWeapon(), flDispenserHealScale, mult_dispenser_healing_received_active );
		int iCanBeOverhealedByDispenser = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iCanBeOverhealedByDispenser, dispenser_can_overheal_wearer );
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter->GetActiveTFWeapon(), iCanBeOverhealedByDispenser, dispenser_can_overheal_active );
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iCanBeOverhealedByDispenser, everything_can_overheal_wearer );
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter->GetActiveTFWeapon(), iCanBeOverhealedByDispenser, everything_can_overheal_active );

		FOR_EACH_VEC( m_vecHealers, i )
		{
			if ( m_vecHealers[i].pDispenser )
			{
				if ( tf2c_spy_cloak_ammo_refill.GetBool() )
				{
					bool bWasNotFull = GetSpyCloakMeter() < 100.0f;

					// Dispensers refill cloak.
					AddSpyCloak( m_vecHealers[i].flAmount * gpGlobals->frametime );

					// TF2C_ACHIEVEMENT_INVIS_RECHARGE_CLOAKED
					if (bWasNotFull && GetSpyCloakMeter() >= 100.0f)
					{

						/* YOU CANT HAVE EVENT NAMES OVER 32 CHARACTERS
							ALSO NOBODY IS LISTENING TO THIS EVENT ANYWAY
							-SAPPHO

						IGameEvent* event = gameeventmanager->CreateEvent("player_just_restocked_cloak_from_dispenser");

						if (event)
						{
							if (m_pOuter)
							{
								event->SetInt("userid", m_pOuter->GetUserID());
								gameeventmanager->FireEvent(event);
							}
							else
							{
								gameeventmanager->FreeEvent(event);
							}
						}
						*/

						if (m_pOuter && m_pOuter->IsEnemy(m_vecHealers[i].pDispenser))
						{
							if (InCond(TF_COND_STEALTHED))
							{
								CTFWeaponBase* pWatch = m_pOuter->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PDA2);
								if (pWatch && pWatch->GetItemID() == 30)
								{
									m_pOuter->AwardAchievement(TF2C_ACHIEVEMENT_INVIS_RECHARGE_CLOAKED);
								}
							}
						}
					}
				}

				// Dispensers don't heal above 100%.
				if ( bHasFullHealth && iCanBeOverhealedByDispenser <= 0 )
					continue;
			}

			// Being healed by a medigun/medic AND WE'RE NOT AT 0 h/s, don't decay our health.
			if ( m_vecHealers[i].flAmount > 0 )
				bDecayHealth = false;

			// Overheal capacity multiplier
			float flOverhealMultiplier = 1.0f;

			// Dispensers heal at a constant rate.
			if ( m_vecHealers[i].pDispenser )
			{
				// Dispensers heal at a slower rate, but ignore flScale.
				flHealFractionThisFrame += gpGlobals->frametime * m_vecHealers[i].flAmount * flDispenserHealScale;
			}
			else
			{
				float flOverhealRate = 1.0f;
				if ( ToTFPlayer( m_vecHealers[i].pPlayer ) ) {
					CTFWeaponBase *pWeaponActive = ToTFPlayer( m_vecHealers[i].pPlayer )->GetActiveTFWeapon();
					if ( pWeaponActive )
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeaponActive, flOverhealRate, overheal_fill_rate );
				}

				// Are we allowed to overheal?
				flOverhealRate *= m_vecHealers[i].bCanOverheal;

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_vecHealers[i].pPlayer, flOverhealMultiplier, mult_medigun_overheal_amount );

				// Player heals are affected by the last damage time.
				flHealFractionThisFrame += (m_vecHealers[i].bRemoveAfterOneFrame ? 1.0f : gpGlobals->frametime) * m_vecHealers[i].flAmount * (m_vecHealers[i].bCritHeals ? flScale : 1.0f) * flMedigunHealScale * (bHasFullHealth ? flOverhealRate : 1.0f);

				float flMaxBuffedHealth = m_pOuter->GetMaxHealth() + ((GetMaxBuffedHealth() - m_pOuter->GetMaxHealth())*flOverhealMultiplier);

				if ( flHealFractionThisFrame > flMaxBuffedHealth - m_pOuter->GetHealth() )
				{
					flHealFractionThisFrame = flMaxBuffedHealth - m_pOuter->GetHealth();
				}
				
				CTFPlayer *pMedic = ToTFPlayer( m_vecHealers[i].pPlayer.Get() );

				// Build Uber
				if ( pMedic->GetMedigun() )
					pMedic->GetMedigun()->BuildUberForTarget( m_pOuter, false );
			}

			flTotalHealAmount += m_vecHealers[i].flAmount;
		}
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flHealFractionThisFrame, mult_healing_received );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter->GetActiveTFWeapon(), flHealFractionThisFrame, mult_healing_received_active );
		
		m_flHealFraction += flHealFractionThisFrame;

		if ( bDisguised )
		{
			// Reloop the healers for now.
			FOR_EACH_VEC( m_vecHealers, i )
			{
				// Dispensers don't heal above 100%.
				if ( m_vecHealers[i].pDispenser && bHasFullDisguiseHealth )
					continue;

				// Being healed by a medigun, don't decay our health.
				bDecayDisguiseHealth = false;

				// Dispensers heal at a constant rate.
				if ( m_vecHealers[i].pDispenser )
				{
					// Dispensers heal at a slower rate, but ignore flScale.
					m_flDisguiseHealFraction += gpGlobals->frametime * m_vecHealers[i].flAmount;
				}
				else
				{
					// Player heals are affected by the last damage time.
					m_flDisguiseHealFraction += (gpGlobals->frametime * m_vecHealers[i].flAmount) * flScale;
				}

				flTotalFakeHealAmount += m_vecHealers[i].flAmount;
			}
		}


		// THIS PART IS REALLY FUCKEN STUPID!!! It divvies up the healing evenly, even if certain healers are healing at differing rates.
		// It should divide it up based on the proportion of the attempted healing e.g. A heals 50/s, B heals 100/s - 100 health actually gets healed in 1s, so give A 33 and B 66
		int nHealthToAdd = Floor2Int( m_flHealFraction );
		//if ( nHealthToAdd > 0 )
		if ( m_flHealFraction > 0 )
		{
//			Msg( "ConditionGameRulesThink called %2.2f, frame time %2.2f \n", gpGlobals->curtime, gpGlobals->frametime );

			m_flHealFraction -= nHealthToAdd;

			int nHealthRestored = m_pOuter->TakeHealth( nHealthToAdd, HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP, NULL, false );

			// Split up total healing based on the amount each healer contributes.
			FOR_EACH_VEC( m_vecHealers, i )
			{
				// Burst healers should track their entire healing in a single frame rather than frame-by-frame like a medigun.
				//int nHealthRestored = m_vecHealers[i].bRemoveAfterOneFrame ? m_vecHealers[i].flAmount : nHealthRestoredBase;

				CTFPlayer *pPlayer = ToTFPlayer( m_vecHealers[i].pPlayer.Get() );
				if ( pPlayer )
				{
					if ( nHealthRestored > 0 )
					{
						float flAmount = (float)nHealthRestored * (m_vecHealers[i].flAmount / flTotalHealAmount);
						if ( !m_pOuter->IsEnemy( pPlayer ) )
						{
							CTF_GameStats.Event_PlayerHealedOther( pPlayer, flAmount );
						}
						else
						{
							CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_vecHealers[i].pDispenser != NULL, flAmount );
						}

						if ( pPlayer->GetMedigun() )
							pPlayer->GetMedigun()->OnHealedPlayer( m_pOuter, flAmount, m_vecHealers[i].healerType );

						if ( !bDisguised )
						{
							// Store off how much this guy healed.
							m_vecHealers[i].iRecentAmount += nHealthRestored;

							if ( m_pOuter->IsVIP() )
							{
								IGameEvent *event = gameeventmanager->CreateEvent( "vip_healed" );
								if ( event )
								{
									event->SetInt( "priority", 1 );
									event->SetInt( "healer", pPlayer->GetUserID() );
									event->SetInt( "amount", nHealthRestored );

									gameeventmanager->FireEvent( event );
								}
							}
						}
					}

					// Show how much this player healed every second.
					if ( !bDisguised && gpGlobals->curtime >= m_vecHealers[i].flNextNofityTime && m_vecHealers[i].flAmount > 0 )
					{
						if ( m_vecHealers[i].iRecentAmount > 0 )
						{
							IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
							if ( event )
							{
								event->SetInt( "priority", 1 );
								event->SetInt( "patient", m_pOuter->GetUserID() );
								event->SetInt( "healer", pPlayer->GetUserID() );
								event->SetInt( "amount", m_vecHealers[i].iRecentAmount );
								event->SetInt( "class", m_pOuter->GetPlayerClass()->GetClassIndex() );

								gameeventmanager->FireEvent( event );
							}
						}

						m_vecHealers[i].iRecentAmount = 0;
						m_vecHealers[i].flNextNofityTime = gpGlobals->curtime + 1.0f;
					}
				}

				// Remove this healer we've hit the health cap and we're meant to stop, or if we're a burst healer, or if we've passed the expiry time.
				if ( (m_vecHealers[i].bRemoveOnMaxHealth && m_pOuter->GetHealth() >= GetMaxBuffedHealth()) || m_vecHealers[i].bRemoveAfterOneFrame || (m_vecHealers[i].bRemoveAfterExpiry && gpGlobals->curtime > m_vecHealers[i].flAutoRemoveTime) )
				{
					if ( m_vecHealers[i].condApplyOnRemove > TF_COND_INVALID )
						AddCond( m_vecHealers[i].condApplyOnRemove, m_vecHealers[i].condApplyTime );
					//StopHealing( ToTFPlayer( m_vecHealers[i].pPlayer.Get() ) );
					StopHealing( i ); // More precise this way; allows parallel healing by the same individual.
					continue;
				}
			}
		}



		// Copied stop-healing checks to here to allow healers that are contributed 0 healing to be removed.
		// Remove this healer we've hit the health cap and we're meant to stop, or if we're a burst healer, or if we've passed the expiry time.
		if ( m_nNumHealers > 0 )
		{
			FOR_EACH_VEC( m_vecHealers, i )
			{
				if ( (m_vecHealers[i].bRemoveOnMaxHealth && m_pOuter->GetHealth() >= GetMaxBuffedHealth()) || m_vecHealers[i].bRemoveAfterOneFrame || (m_vecHealers[i].bRemoveAfterExpiry && gpGlobals->curtime > m_vecHealers[i].flAutoRemoveTime) )
				{
					//StopHealing( ToTFPlayer( m_vecHealers[i].pPlayer.Get() ) );
					StopHealing( i ); // More precise this way; allows parallel healing by the same individual.
					continue;
				}
			}
		}

		if ( bDisguised )
		{
			int nFakeHealthToAdd = (int)m_flDisguiseHealFraction;
			if ( nFakeHealthToAdd > 0 )
			{
				m_flDisguiseHealFraction -= nFakeHealthToAdd;

				int nHealthRestored = m_pOuter->TakeDisguiseHealth( nFakeHealthToAdd, HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );

				// Split up total healing based on the amount each healer contributes.
				FOR_EACH_VEC( m_vecHealers, i )
				{
					CTFPlayer *pPlayer = ToTFPlayer( m_vecHealers[i].pPlayer.Get() );
					if ( pPlayer )
					{
						if ( nHealthRestored > 0 )
						{
							// Store off how much this guy healed.
							m_vecHealers[i].iRecentAmount += nHealthRestored;

							if ( m_hDisguiseTarget.Get() && (static_cast<CTFPlayer *>(m_hDisguiseTarget.Get()))->IsVIP() )
							{
								IGameEvent *event = gameeventmanager->CreateEvent( "vip_healed" );
								if ( event )
								{
									event->SetInt( "priority", 1 );
									event->SetInt( "healer", pPlayer->GetUserID() );
									event->SetInt( "amount", nHealthRestored );

									gameeventmanager->FireEvent( event );
								}
							}
						}

						// Show how much this player healed every second.
						if ( gpGlobals->curtime >= m_vecHealers[i].flNextNofityTime )
						{
							if ( m_vecHealers[i].iRecentAmount > 0 )
							{
								IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
								if ( event )
								{
									event->SetInt( "priority", 1 );
									event->SetInt( "patient", m_pOuter->GetUserID() );
									event->SetInt( "healer", pPlayer->GetUserID() );
									event->SetInt( "amount", m_vecHealers[i].iRecentAmount );
									event->SetInt( "class", m_nDisguiseClass );

									gameeventmanager->FireEvent( event );
								}
							}

							m_vecHealers[i].iRecentAmount = 0;
							m_vecHealers[i].flNextNofityTime = gpGlobals->curtime + 1.0f;
						}
					}

					if ( m_vecHealers[i].bRemoveOnMaxHealth && m_iDisguiseHealth >= m_iDisguiseMaxHealth || m_vecHealers[i].bRemoveAfterOneFrame )
					{
						StopHealing( ToTFPlayer( m_vecHealers[i].pPlayer.Get() ), m_vecHealers[i].healerType );
						continue;
					}
				}
			}
		}

		if ( InCond( TF_COND_BURNING ) )
		{
			// Reduce the duration of this burn 
			float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction
			m_flFlameRemoveTime -= flReduction * gpGlobals->frametime;
		}

		if ( InCond( TF_COND_BLEEDING ) )
		{
			// Reduce the duration of this bleeding 
			float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction
			FOR_EACH_VEC( m_PlayerBleeds, i )
			{
				m_PlayerBleeds[i].flBleedingRemoveTime -= flReduction * gpGlobals->frametime;
			}
		}
	}

	// A check because we implemented to change our max overheal limit by deploying or holstering weapon with one attribute, so it drops suddenly if we suddenly get less max overheal than current health.
	/*if ( m_pOuter->GetHealth() > GetMaxBuffedHealth() )
	{
	m_pOuter->SetHealth( GetMaxBuffedHealth() );
	}*/

	float flDecayTime = tf_boost_drain_time.GetFloat();
	if ( bDecayHealth )
	{
		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			m_flHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / flDecayTime));

			int nHealthToDrain = (int)m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds/etc.
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}
	}

	// Fake health for disguises
	if ( bDisguised && bDecayDisguiseHealth )
	{
		if ( m_iDisguiseHealth > m_iDisguiseMaxHealth )
		{
			float flBoostMaxAmount = GetDisguiseMaxBuffedHealth() - m_iDisguiseMaxHealth;
			m_flDisguiseHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / flDecayTime));

			int nHealthToDrain = (int)m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;

				// Reduce our fake disguised health by roughly the same amount.
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
	{
		if ( !InCond( TF_COND_HEALTH_OVERHEALED ) )
		{
			AddCond( TF_COND_HEALTH_OVERHEALED );
		}
	}
	else
	{
		if ( InCond( TF_COND_HEALTH_OVERHEALED ) )
		{
			RemoveCond( TF_COND_HEALTH_OVERHEALED );
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( m_flTauntRemoveTime >= 0.0f && gpGlobals->curtime > m_flTauntRemoveTime )
		{
			//m_pOuter->SnapEyeAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );

			RemoveCond( TF_COND_TAUNTING );
		}
	}

	if ( !bDisguised )
	{
		// Remove our disguise weapon if we are ever not disguised and we have one,
		// and also clear the disguise weapon list.
		RemoveDisguiseWeapon();
		m_pOuter->ClearDisguiseWeaponList();
	}

	if ( InCond( TF_COND_BURNING ) && m_pOuter->m_flPowerPlayTime < gpGlobals->curtime )
	{
		// If we're underwater, put the fire out.
		if ( gpGlobals->curtime > m_flFlameRemoveTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TF_COND_BURNING );
		}
		else if ( gpGlobals->curtime >= m_flFlameBurnTime && !m_pOuter->IsPlayerClass( TF_CLASS_PYRO, true ) )
		{
			// !!! foxysen beacon
			// Maybe not the best place, but perhaps people will like exintguishing not happening instantly
			int iExtinguishSelf = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hBurnAttacker, iExtinguishSelf, on_death_remove_targets_afterburn );
			if ( iExtinguishSelf && m_hBurnAttacker && !m_hBurnAttacker->IsAlive() )
			{
				m_pOuter->EmitSound( "TFPlayer.FlameOut" );
				RemoveCond( TF_COND_BURNING );
			}
			else
			{
				float flBurnDamage = tf2c_afterburn_damage.GetFloat();
				int nKillType = TF_DMG_CUSTOM_BURNING;

				// If you add any further afterburn damage modifier, also change Damage Blocked stats
				// modifiers for Extinguish functions in base melee, Flamethrower and Jumppad
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnWeapon, flBurnDamage, mult_wpn_burndmg );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnAttacker, flBurnDamage, mult_burndmg_wearer );
				if ( m_hBurnAttacker )
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnAttacker->GetActiveTFWeapon(), flBurnDamage, mult_burndmg_active );
				if ( m_hBurnWeapon )
				{
					if ( m_hBurnWeapon.Get()->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
					{
						nKillType = TF_DMG_CUSTOM_BURNING_ARROW;
					}
				}

				// Burn the player (if not Pyro, who does not take persistent burning damage).
				CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, m_hBurnWeapon, Vector( 0, 0, 0 ), m_pOuter->GetAbsOrigin() + Vector( 0, 0, m_pOuter->GetPlayerMaxs().z / 2 ), flBurnDamage, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, nKillType );
				m_pOuter->TakeDamage( info );


				
				// !!! foxysen beacon
				// We give health here to attacker if they have beacon attribute here... which doesn't check whether it has taken damage.
				// Should it do damage if player will someday get 100% afterburn resistance yet stays on fire state? Who knows yet!
				if ( m_hBurnAttacker )
				{
					bool bBeaconStopHealing = false;
					bool iBeaconLastTick = false;

					CTFWeaponBase* pMeleeWeapon = m_hBurnAttacker->Weapon_OwnsThisID(TF_WEAPON_BEACON);
					if (pMeleeWeapon && pMeleeWeapon->GetWeaponID() == TF_WEAPON_BEACON)
					{
						CTFBeacon* pBeacon = static_cast<CTFBeacon*>(pMeleeWeapon);
						if ( pBeacon )
						{
							bool bBeaconStopHealingOnCrit = false;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(pBeacon, bBeaconStopHealingOnCrit, beacon_dont_heal_crit_full);
							if (bBeaconStopHealingOnCrit && pBeacon->IsStoredBeaconCritFilled())
							{
								bBeaconStopHealing = true;
							}

							// abp harvester
							bool bBeaconAlwaysStoreCrits = false;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hBurnAttacker, bBeaconAlwaysStoreCrits, beacon_always_store_crits);
							if ( bBeaconAlwaysStoreCrits || ( pBeacon == m_hBurnAttacker->GetActiveTFWeapon() ) )
							{
								if (bBeaconStopHealingOnCrit && pBeacon->GetStoredBeaconCrit() == (pBeacon->GetMaxBeaconTicks() - 1))
									iBeaconLastTick = true;

								pBeacon->AddStoredBeaconCrit(1);
							}
						}
					}

					if (!bBeaconStopHealing || iBeaconLastTick)
					{
						int iToHealAttacker = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hBurnAttacker, iToHealAttacker, flat_heal_on_afterburn_wearer);
						CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hBurnAttacker->GetActiveTFWeapon(), iToHealAttacker, flat_heal_on_afterburn_active);
						if (iToHealAttacker > 0)
						{
							// !!! foxysen beacon
							int iHealed = m_hBurnAttacker->TakeHealth(iToHealAttacker, HEAL_NOTIFY);
							CTF_GameStats.Event_PlayerHealedOther(m_hBurnAttacker, iHealed);

							if (iHealed)
							{
								IGameEvent* event = gameeventmanager->CreateEvent("healed_on_afterburn");
								if (event)
								{
									event->SetInt("amount", iHealed);
									event->SetInt("entindex", m_hBurnAttacker->entindex());

									gameeventmanager->FireEvent(event);
								}
							}
						}

						if (!iBeaconLastTick)
						{
							int iNotifyAfterburnHeal = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hBurnAttacker, iNotifyAfterburnHeal, afterburn_heal_attempt_sound);
							if (iNotifyAfterburnHeal && m_hBurnAttacker->IsAlive() && !(IsDisguised() && m_hBurnAttacker->m_Shared.IsFooledByDisguise(GetDisguiseTeam())))
							{
								m_hBurnAttacker->PlayTickTockSound();
							}
						}
					}
				}

				m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
			}
		}

		// remove cond check if we move extinguish check above
		if ( InCond( TF_COND_BURNING ) && m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5f;
		}
	}

	// The part where we try and leech health off the remembered remaining afterburns
	if ( !m_PlayerDeathBurns.IsEmpty() )
	{
		FOR_EACH_VEC_BACK( m_PlayerDeathBurns, i )
		{
			deathburn_struct_t &deathburn = m_PlayerDeathBurns[i];
			if ( gpGlobals->curtime >= deathburn.flBurningRemoveTime )
			{
				m_PlayerDeathBurns.FastRemove( i );
			}
			else if ( (gpGlobals->curtime >= deathburn.flBurningNextTick) )
			{
				bool iBeaconStopHealing = false;
				bool iBeaconLastTick = false;

				CTFWeaponBase* pMeleeWeapon = m_pOuter->Weapon_OwnsThisID(TF_WEAPON_BEACON);
				if (pMeleeWeapon && pMeleeWeapon->GetWeaponID() == TF_WEAPON_BEACON)
				{
					CTFBeacon* pBeacon = static_cast<CTFBeacon*>(pMeleeWeapon);
					if (pBeacon)
					{
						bool bBeaconStopHealingOnCrit = false;
						CALL_ATTRIB_HOOK_INT_ON_OTHER(pBeacon, bBeaconStopHealingOnCrit, beacon_dont_heal_crit_full);
						if (bBeaconStopHealingOnCrit && pBeacon->IsStoredBeaconCritFilled())
						{
							iBeaconStopHealing = true;
						}

						// abp harvester
						bool bBeaconAlwaysStoreCrits = false;
						CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter, bBeaconAlwaysStoreCrits, beacon_always_store_crits);
						if (bBeaconAlwaysStoreCrits || (pBeacon == m_pOuter->GetActiveTFWeapon()))
						{
							if ( bBeaconStopHealingOnCrit && pBeacon->GetStoredBeaconCrit() == ( pBeacon->GetMaxBeaconTicks() - 1 ) )
								iBeaconLastTick = true;

							pBeacon->AddStoredBeaconCrit(1);
						}
					}
				}

				if ( !iBeaconStopHealing || iBeaconLastTick )
				{
					// !!! foxysen beacon
					// We give health here to attacker if they have beacon attribute here... which doesn't check whether it has taken damage.
					// Should it do damage if player will someday get 100% afterburn resistance yet stays on fire state? Who knows yet!
					int iToHealAttacker = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter, iToHealAttacker, flat_heal_on_afterburn_wearer);
					CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter->GetActiveTFWeapon(), iToHealAttacker, flat_heal_on_afterburn_active);
					if (iToHealAttacker)
					{
						// !!! foxysen beacon
						int iHealed = m_pOuter->TakeHealth(iToHealAttacker, HEAL_NOTIFY);
						CTF_GameStats.Event_PlayerHealedOther(m_pOuter, iHealed);

						if (iHealed)
						{
							IGameEvent* event = gameeventmanager->CreateEvent("healed_on_afterburn");
							if (event)
							{
								event->SetInt("amount", iHealed);
								event->SetInt("entindex", m_pOuter->entindex());

								gameeventmanager->FireEvent(event);
							}
						}
					}

					if ( !iBeaconLastTick )
					{
						int iNotifyAfterburnHeal = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter, iNotifyAfterburnHeal, afterburn_heal_attempt_sound);
						if ( iNotifyAfterburnHeal )
						{
							m_pOuter->PlayTickTockSound();
						}
					}
				}

				deathburn.flBurningNextTick = gpGlobals->curtime + TF_BURNING_FREQUENCY;

				// It's very possible we died from the take damage, which clears all our conditions
				// and nukes m_PlayerDeathBurns. If that happens, bust out of this loop.
				if ( m_PlayerDeathBurns.Count() == 0 )
					break;
			}
		}
	}

	if ( InCond( TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > m_flDisguiseCompleteTime )
		{
			CompleteDisguise();
		}
	}

	// Stops the drain hack.
	if ( m_pOuter->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		ITFHealingWeapon *pWeapon = m_pOuter->GetMedigun();
		if ( pWeapon && pWeapon->IsReleasingCharge() )
		{
			pWeapon->DrainCharge();
		}
	}

	TestAndExpireChargeEffect( TF_CHARGE_INVULNERABLE );
	TestAndExpireChargeEffect( TF_CHARGE_CRITBOOSTED );
	TestAndExpireChargeEffect( TF_CHARGE_PUSHBUFF );

	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		float flBlinkPenalty = TF_SPY_STEALTH_BLINKTIME;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flBlinkPenalty, cloak_blink_time_penalty );
		if ( (gpGlobals->curtime - m_flLastStealthExposeTime) >= flBlinkPenalty/*tf_spy_stealth_blink_time.GetFloat()*/ )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}

	if ( InCond( TF_COND_BLEEDING ) )
	{
		FOR_EACH_VEC_BACK( m_PlayerBleeds, i )
		{
			bleed_struct_t &bleed = m_PlayerBleeds[i];
			if ( gpGlobals->curtime >= bleed.flBleedingRemoveTime && !bleed.bPermanentBleeding )
			{
				m_PlayerBleeds.FastRemove( i );
			}
			else if ( (gpGlobals->curtime >= bleed.flBleedingTime) )
			{
				bleed.flBleedingTime = gpGlobals->curtime + TF_BLEEDING_FREQUENCY;

				CTakeDamageInfo info( bleed.hBleedingAttacker, bleed.hBleedingAttacker, bleed.hBleedingWeapon, Vector( 0, 0, 0 ), m_pOuter->EyePosition(), bleed.nBleedDmg, DMG_SLASH, TF_DMG_CUSTOM_BLEEDING );
				m_pOuter->TakeDamage( info );

				// It's very possible we died from the take damage, which clears all our conditions
				// and nukes m_PlayerBleeds. If that happens, bust out of this loop.
				if ( m_PlayerBleeds.Count() == 0 )
					break;
			}
		}

		if ( !m_PlayerBleeds.Count() )
		{
			RemoveCond( TF_COND_BLEEDING );
		}
	}

	if ( InCond( TF_COND_LAUNCHED ) && m_pOuter->GetGroundEntity() )	// We perform this check here so cond will not get removed before ontakedamage check
	{
		m_pOuter->EmitSound( "TFPlayer.SafestLanding" );
		RemoveCond( TF_COND_LAUNCHED );

		int iTeam = IsDisguised() ? GetDisguiseTeam() : m_pOuter->GetTeamNumber();
		if ( iTeam != TF_TEAM_GLOBAL ) // hack. doesnt display particle for global disguises for now. - azzy
		{
			const char* pszEffect = ConstructTeamParticle( "jumppad_jump_puff_smoke_%s", iTeam );
			DispatchParticleEffect( pszEffect, m_pOuter->GetAbsOrigin(), vec3_angle );
		}
	}

	if ( InCond( TF_COND_LAUNCHED_SELF ) && m_pOuter->GetGroundEntity() )
	{
		RemoveCond( TF_COND_LAUNCHED_SELF );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{

	UpdateCloakMeter();

	m_ConditionList.Think();

#ifdef CLIENT_DLL
	if (InCond(TF_COND_TRANQUILIZED))
	{
		if (m_pOuter->IsLocalPlayer() && GetConditionDuration(TF_COND_TRANQUILIZED) <= 1.0f)
		{
			CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

			// Fade out the sound towards the end.
			if (m_pTranqSound && controller.SoundGetVolume(m_pTranqSound) == 1.0f)
			{
				controller.SoundFadeOut(m_pTranqSound, 1.0f);
			}
		}

		// ConditionThink doesn't seem to trigger for non-local player on client-side (non-bots-only?)
		/*
		//Msg("(%.1f left)\n", GetConditionDuration(TF_COND_TRANQUILIZED));
		if (GetConditionDuration(TF_COND_TRANQUILIZED) <= tf2c_tranq_particles_pre_remove.GetFloat())
		{
			if (m_pTranqEffect)
			{
				m_pOuter->ParticleProp()->StopEmission(m_pTranqEffect);
				m_pTranqEffect = NULL;
			}
		}*/
	}

#else
	if ( tf2c_vip_abilities.GetInt() == 1 || tf2c_vip_abilities.GetInt() > 2 )
	{
		UpdateVIPBuff();
	}
#endif

	if ( InCond( TF_COND_STUNNED ) )
	{
		if ( GetActiveStunInfo() && gpGlobals->curtime > GetActiveStunInfo()->flExpireTime )
		{
#ifdef GAME_DLL	
			m_PlayerStuns.Remove( m_iStunIndex );
			m_iStunIndex = -1;

			// Apply our next stun.
			int c = m_PlayerStuns.Count();
			if ( c )
			{
				int iStrongestIdx = 0;
				for ( int i = 1; i < c; i++ )
				{
					if ( m_PlayerStuns[i].flStunAmount > m_PlayerStuns[iStrongestIdx].flStunAmount )
					{
						iStrongestIdx = i;
					}
				}

				m_iStunIndex = iStrongestIdx;

				AddCond( TF_COND_STUNNED, -1.0f, m_PlayerStuns[m_iStunIndex].hPlayer );
				m_iMovementStunParity = (m_iMovementStunParity + 1) & ((1 << MOVEMENTSTUN_PARITY_BITS) - 1);

				Assert( GetActiveStunInfo() );
			}
			else
			{
				RemoveCond( TF_COND_STUNNED );
			}
#endif // GAME_DLL

			UpdateLegacyStunSystem();
		}
		else if ( IsControlStunned() && GetActiveStunInfo() && (gpGlobals->curtime > GetActiveStunInfo()->flStartFadeTime) )
		{
			// Control stuns have a final anim to play.
			ControlStunFading();
			}

#ifdef CLIENT_DLL
		// Turn off stun effect that gets turned on when incomplete stun message is received on the client.
		if ( GetActiveStunInfo() && GetActiveStunInfo()->iStunFlags & TF_STUN_NO_EFFECTS )
		{
			if ( m_pOuter->m_pStunnedEffect )
			{
				// Remove stun stars if they are still around.
				// They might be if we died, etc.
				m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
				m_pOuter->m_pStunnedEffect = NULL;
			}
		}
#endif
		}
	}


void CTFPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0.0f, 0.1f );
#endif
}


void CTFPlayerShared::OnAddDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pDisguisingEffect );
		m_pDisguisingEffect = NULL;
	}

	if ( (!m_pOuter->IsLocalPlayer() || !m_pOuter->InFirstPersonView()) && !IsStealthed() )
	{
		m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( ConstructTeamParticle( "spy_start_disguise_%s", m_pOuter->GetTeamNumber() ), PATTACH_ABSORIGIN_FOLLOW );
	}
#else
	const char *szDisguiseSound = "Player.Spy_Disguise";

	string_t szCustomSound = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( m_pOuter, szCustomSound, custom_disguise_sound );

	// Make sure its the disguise Kit and we have a valid sound
	if ( szCustomSound.ToCStr()[0] != '\0' )
		szDisguiseSound = szCustomSound.ToCStr();

	m_pOuter->EmitSound( szDisguiseSound );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised( void )
{
#ifdef CLIENT_DLL
	if ( m_pDisguisingEffect )
	{
		// Turn off disguising particles.
		m_pOuter->ParticleProp()->StopEmission( m_pDisguisingEffect );
		m_pDisguisingEffect = NULL;
	}
#else
	m_pOuter->DropFlag();
#endif
	}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes.
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	// Recalculate disguise model index.
	//RecalcDisguiseWeapon();
	m_pOuter->UpdateSpyStateChange();
}
#endif


void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifndef CLIENT_DLL
	// Stock uber removes negative conditions.
	HealNegativeConds();
#endif
}


void CTFPlayerShared::OnRemoveInvulnerable( void )
{
}

#ifdef CLIENT_DLL

bool CTFPlayerShared::ShouldShowRecentlyTeleported( void )
{
    if (InCond(TF_COND_TELEPORTED_ALWAYS_SHOW))
    {
        return true;
    }
	if ( !InCond( TF_COND_TELEPORTED ) )
		return false;

	if ( InCond(TF_COND_STEALTHED) ) 
		return false;

	if (IsDisguised() && (m_pOuter->IsLocalPlayer() || m_pOuter->IsEnemyPlayer()))
	{
		if (GetDisguiseTeam() != m_nTeamTeleporterUsed)
			return false;
	}
	else if (m_pOuter->GetTeamNumber() != m_nTeamTeleporterUsed)
		return false;

	return true;
	}
#endif


void CTFPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}


void CTFPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#else
	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;
#endif
}


void CTFPlayerShared::OnAddLaunched( void )
{
	m_pOuter->UpdateJumppadTrailEffect();
}


void CTFPlayerShared::OnRemoveLaunched( void )
{
	m_pOuter->UpdateJumppadTrailEffect();
	if (InCond(TF_COND_JUMPPAD_ASSIST) && GetConditionDuration(TF_COND_JUMPPAD_ASSIST) == PERMANENT_CONDITION)
		SetConditionDuration(TF_COND_JUMPPAD_ASSIST, TF_COND_JUMPPAD_ASSIST_DURATION);
}


void CTFPlayerShared::OnAddTaunting( void )
{
	CTFWeaponBase *pActiveWeapon = m_pOuter->GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		// Cancel any reload in progress.
		pActiveWeapon->AbortReload();
	}
}


void CTFPlayerShared::OnRemoveTaunting( void )
{
#ifdef GAME_DLL
	m_pOuter->StopTaunt();

	CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
#ifdef ITEM_TAUNTING
		if ( pActiveWeapon->IsUsedForTaunt() )
		{
			pActiveWeapon->UseForTaunt( NULL );
		}
#endif

		// HACK: For weapons that rely on Taunt Attack data (Lunch Box).
		pActiveWeapon->OwnerConditionRemoved( TF_COND_TAUNTING );
	}

	m_pOuter->ClearTauntAttack();
#endif
}


void CTFPlayerShared::OnAddStunned( void )
{
	if ( IsControlStunned() || IsLoserStateStunned() )
	{
#ifdef CLIENT_DLL
		if ( GetActiveStunInfo() )
		{
			if ( !m_pOuter->m_pStunnedEffect && !(GetActiveStunInfo()->iStunFlags & TF_STUN_NO_EFFECTS) )
			{
				if ( (GetActiveStunInfo()->iStunFlags & TF_STUN_BY_TRIGGER) )
				{
					m_pOuter->m_pStunnedEffect = m_pOuter->ParticleProp()->Create( "yikes_fx", PATTACH_POINT_FOLLOW, "head" );
				}
				else
				{
					m_pOuter->m_pStunnedEffect = m_pOuter->ParticleProp()->Create( "conc_stars", PATTACH_POINT_FOLLOW, "head" );
				}
				}
			}
#endif

		// Notify our weapon that we have been stunned.
		CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
		if ( pWeapon )
		{
			pWeapon->OnControlStunned();
		}

		m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
	}
}


void CTFPlayerShared::OnRemoveStunned( void )
{
	m_iStunFlags = 0;
	m_hStunner = NULL;

#ifdef CLIENT_DLL
	if ( m_pOuter->m_pStunnedEffect )
	{
		// Remove stun stars if they are still around.
		// They might be if we died, etc.
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
		m_pOuter->m_pStunnedEffect = NULL;
	}
#else
	m_iStunIndex = -1;
	m_PlayerStuns.RemoveAll();
#endif

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif
}


void CTFPlayerShared::ControlStunFading( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pStunnedEffect )
	{
		// Remove stun stars early...
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
		m_pOuter->m_pStunnedEffect = NULL;
	}
#endif
	}


void CTFPlayerShared::SetStunExpireTime( float flTime )
{
#ifdef GAME_DLL
	stun_struct_t *pActiveStun = GetActiveStunInfo();
	if ( pActiveStun )
	{
		pActiveStun->flExpireTime = flTime;
	}
#else
	m_flStunEnd = flTime;
#endif

	UpdateLegacyStunSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Mirror the stun info to the old system for networking.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateLegacyStunSystem( void )
{
	// What a mess.
#ifdef GAME_DLL
	stun_struct_t *pStun = GetActiveStunInfo();
	if ( pStun )
	{
		m_hStunner = pStun->hPlayer;
		m_flStunFade = gpGlobals->curtime + pStun->flDuration;
		m_flMovementStunTime = pStun->flDuration;
		m_flStunEnd = pStun->flExpireTime;
		if ( pStun->iStunFlags & TF_STUN_CONTROLS )
		{
			m_flStunEnd = pStun->flExpireTime;
		}
		m_iMovementStunAmount = pStun->flStunAmount;
		m_iStunFlags = pStun->iStunFlags;

		m_iMovementStunParity = (m_iMovementStunParity + 1) & ((1 << MOVEMENTSTUN_PARITY_BITS) - 1);
}
#else
	m_ActiveStunInfo.hPlayer = m_hStunner;
	m_ActiveStunInfo.flDuration = m_flMovementStunTime;
	m_ActiveStunInfo.flExpireTime = m_flStunEnd;
	m_ActiveStunInfo.flStartFadeTime = m_flStunEnd;
	m_ActiveStunInfo.flStunAmount = m_iMovementStunAmount;
	m_ActiveStunInfo.iStunFlags = m_iStunFlags;
#endif // GAME_DLL
}


stun_struct_t *CTFPlayerShared::GetActiveStunInfo( void ) const
{
#ifdef GAME_DLL
	return m_PlayerStuns.IsValidIndex( m_iStunIndex ) ? const_cast<stun_struct_t *>(&m_PlayerStuns[m_iStunIndex]) : NULL;
#else
	return m_iStunIndex >= 0 ? const_cast<stun_struct_t *>(&m_ActiveStunInfo) : NULL;
#endif
}


CTFPlayer *CTFPlayerShared::GetStunner( void )
{
	stun_struct_t *pActiveStun = GetActiveStunInfo();
	return pActiveStun ? pActiveStun->hPlayer : NULL;
}


void CTFPlayerShared::OnAddBleeding( void )
{
#ifdef GAME_DLL
	// We should have at least one bleed entry.
	Assert( m_PlayerBleeds.Count() );
#endif
}


void CTFPlayerShared::OnRemoveBleeding( void )
{
#ifdef GAME_DLL
	m_PlayerBleeds.RemoveAll();
#endif
}


void CTFPlayerShared::OnAddMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();

	if ( m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->EmitSound( "Weapon_Marked_for_Death.Indicator" );
}
	else if ( !IsDisguised() && !IsStealthed() )
	{
		m_pOuter->EmitSound( "Weapon_Marked_for_Death.Initial" );
	}
#endif
		}


void CTFPlayerShared::OnRemoveMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();
#endif
}


void CTFPlayerShared::OnAddMarkedForDeathSilent( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();
#endif
}
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMarkedForDeathSilent( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();
#endif
}


void CTFPlayerShared::OnAddHalloweenGiant( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 2.0f, 0.0f );

	m_pOuter->SetMaxHealth( m_pOuter->GetPlayerClass()->GetMaxHealth() * 10 );
	m_pOuter->SetHealth( m_pOuter->GetMaxHealth() );
#endif
}


void CTFPlayerShared::OnRemoveHalloweenGiant( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 1.0f, 0.0f );

	m_pOuter->SetMaxHealth( m_pOuter->GetPlayerClass()->GetMaxHealth() );
	m_pOuter->SetHealth( m_pOuter->GetMaxHealth() );
#endif
}


void CTFPlayerShared::OnAddHalloweenTiny( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 0.5f, 0.0f );
#endif
}


void CTFPlayerShared::OnRemoveHalloweenTiny( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 1.0f, 0.0f );
#endif
}


void CTFPlayerShared::OnAddSlowed( void )
{
#ifdef GAME_DLL
	if ( m_aTranqAttackers.Count() <= 0 )
	{
		m_flTranqStartTime = gpGlobals->curtime;
	}
#endif

	m_pOuter->TeamFortress_SetSpeed();

	// Clamp velocity to max run speed to affect rocket jumps.
	float flMaxVelX = m_pOuter->GetAbsVelocity().x;
	float flMaxVelY = m_pOuter->GetAbsVelocity().y;

	if ( flMaxVelX > m_pOuter->MaxSpeed() ) flMaxVelX = m_pOuter->MaxSpeed();
	if ( flMaxVelY > m_pOuter->MaxSpeed() ) flMaxVelY = m_pOuter->MaxSpeed();
	if ( flMaxVelX < -m_pOuter->MaxSpeed() ) flMaxVelX = -m_pOuter->MaxSpeed();
	if ( flMaxVelY < -m_pOuter->MaxSpeed() ) flMaxVelY = -m_pOuter->MaxSpeed();

	m_pOuter->SetAbsVelocity( Vector( flMaxVelX, flMaxVelY, m_pOuter->GetAbsVelocity().z ) );

#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		if ( !m_pTranqSound )
		{
			CLocalPlayerFilter filter;
			m_pTranqSound = controller.SoundCreate( filter, m_pOuter->entindex(), "TFPlayer.TranqHit" );
}

		controller.Play( m_pTranqSound, 1.0f, 100 );

		//gHUD.m_flMouseSensitivityFactor = 0.5f;
}

	if ( m_pTranqEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pTranqEffect );
	}

	m_pTranqEffect = m_pOuter->ParticleProp()->Create( "tranq_stars", PATTACH_POINT_FOLLOW, m_pOuter->LookupAttachment( "head" ) );
#endif

/*
// TF2C_INFLICT_TRANQUILIZATION
// Old version of achievement
#ifdef GAME_DLL
	if ( m_flTranqStartTime && (gpGlobals->curtime - m_flTranqStartTime >= ACHIEVEMENT_TF2C_TRANQ_TORTURE_DURATION_REQUIREMENT) )
	{
		FOR_EACH_VEC( m_aTranqAttackers, i )
		{
			if ( m_aTranqAttackers[i] )
			{
				m_aTranqAttackers[i]->AwardAchievement( TF2C_ACHIEVEMENT_INFLICT_TRANQUILIZATION );
			}
		}

		m_aTranqAttackers.RemoveAll();
	}
#endif
*/
}


void CTFPlayerShared::OnRemoveSlowed( void )
{
#ifdef GAME_DLL
	m_flTranqStartTime = 0;
#endif

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		if ( m_pTranqSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundDestroy( m_pTranqSound );
			m_pTranqSound = NULL;
		}

		//gHUD.m_flMouseSensitivityFactor = 0.0f;
	}

	if ( m_pTranqEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pTranqEffect );
		m_pTranqEffect = NULL;
	}
#else
	m_aTranqAttackers.RemoveAll();
#endif
	}


void CTFPlayerShared::OnAddMeleeOnly( void )
{
#ifdef GAME_DLL
	m_pOuter->Weapon_Switch( m_pOuter->GetWeaponForLoadoutSlot( TF_LOADOUT_SLOT_MELEE ) );
#endif
}


void CTFPlayerShared::OnRemoveMeleeOnly( void )
{
#ifdef GAME_DLL
	m_pOuter->SwitchToNextBestWeapon( m_pOuter->GetActiveWeapon() );
#endif
}


void CTFPlayerShared::OnAddResistanceBuff( void )
{
	if ( m_bEmittedResistanceBuff )
		return;

#ifdef CLIENT_DLL
	if ( m_pResistanceBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pResistanceBuffEffect );
	}

	const char *pszEffectName = ConstructTeamParticle( "civilianbuff_%s_buffed", m_pOuter->IsDisguisedEnemy() ? GetDisguiseTeam() : m_pOuter->GetTeamNumber() );
	m_pResistanceBuffEffect = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, 0 );
#else
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_POSITIVE );

	CSingleUserRecipientFilter filter( m_pOuter );
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.VIPBuffUp" );
#endif

	m_bEmittedResistanceBuff = true;
}


void CTFPlayerShared::OnRemoveResistanceBuff( void )
{
#ifdef CLIENT_DLL
	if ( m_pResistanceBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pResistanceBuffEffect );
		m_pResistanceBuffEffect = NULL;
	}
#else
	CSingleUserRecipientFilter filter( m_pOuter );
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.VIPBuffDown" );
#endif

	m_bEmittedResistanceBuff = false;
}

void CTFPlayerShared::OnAddCivResistanceBoost( void )
{
#ifdef CLIENT_DLL
	if ( m_pCivResistanceBoostEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pCivResistanceBoostEffect );
}

	const char *pszEffectName = ConstructTeamParticle( "civilianbuff_%s_buffed", m_pOuter->IsDisguisedEnemy() ? GetDisguiseTeam() : m_pOuter->GetTeamNumber() );
	m_pCivResistanceBoostEffect = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, 0 );
#endif
}

void CTFPlayerShared::OnRemoveCivResistanceBoost( void )
{
#ifdef CLIENT_DLL
	if( m_pCivResistanceBoostEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pCivResistanceBoostEffect );
		m_pCivResistanceBoostEffect = NULL;
	}
#endif
}

void CTFPlayerShared::OnAddCivSpeedBoost( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateSpeedBoostTrailEffect();
#else
	CSingleUserRecipientFilter filter( m_pOuter );
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "DisciplineDevice.PowerUp" );
	m_pOuter->TeamFortress_SetSpeed();
	AddCond( TF_COND_SPEED_BOOST );
#endif
}

void CTFPlayerShared::OnRemoveCivSpeedBoost( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateSpeedBoostTrailEffect();
#else
	CSingleUserRecipientFilter filter( m_pOuter );
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "DisciplineDevice.PowerDown" );
	m_pOuter->TeamFortress_SetSpeed();
	RemoveCond( TF_COND_SPEED_BOOST );
#endif
}


void CTFPlayerShared::RecalculatePlayerBodygroups( bool bForce /*= false*/ )
{
	// Backup our current bodygroups for restoring them
	// after the player is drawn with the modified ones.
	m_nRestoreBody = m_pOuter->m_nBody;
	m_nRestoreDisguiseBody = m_nDisguiseBody;

	// Let our weapons update our bodygroups.
	CEconEntity::UpdateWeaponBodygroups( m_pOuter, bForce );

	// Let our disguise weapon update our bodygroups as well.
	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->UpdateBodygroups( m_pOuter, bForce );
	}

	// Let our wearables update our bodygroups.
	CEconWearable::UpdateWearableBodygroups( m_pOuter, bForce );
}


void CTFPlayerShared::RestorePlayerBodygroups( void )
{
	// Restore our bodygroups to these values.
	m_pOuter->m_nBody = m_nRestoreBody;
	m_nDisguiseBody = m_nRestoreDisguiseBody;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayerShared::Burn( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon /*= NULL*/ )
{
#ifndef CLIENT_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	// Pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit.
	bool bVictimIsPyro = m_pOuter->IsPlayerClass( TF_CLASS_PYRO, true );

	if ( !InCond( TF_COND_BURNING ) )
	{
		// Start burning!
		AddCond( TF_COND_BURNING, PERMANENT_CONDITION, pAttacker );
		m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;	// ASAP!

		// Let the attacker know he burned me.
		if ( pAttacker && !bVictimIsPyro )
		{
			pAttacker->OnBurnOther( m_pOuter );
		}
	}

	float flFlameLife = TF_BURNING_FREQUENCY + (bVictimIsPyro ? 0.0f : tf2c_afterburn_time.GetFloat());
	if ( !bVictimIsPyro )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pAttacker, flFlameLife, mult_wpn_burntime );
	}

	m_flFlameRemoveTime = gpGlobals->curtime + flFlameLife;
	m_hBurnAttacker = pAttacker;
	m_hBurnWeapon = pWeapon;
#endif
}


void CTFPlayerShared::MakeBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, float flBleedingTime, int nBleedDmg /* = TF_BLEEDING_DMG */, bool bPermanentBleeding /*= false*/ )
{
#ifndef CLIENT_DLL
	// Don't bother if they are dead.
	if ( !m_pOuter->IsAlive() )
		return;

	// Required for the CTakeDamageInfo we create later.
	Assert( pPlayer && pWeapon );
	if ( !pPlayer && !pWeapon )
		return;

	float flStartTime = gpGlobals->curtime + TF_BLEEDING_FREQUENCY;
	float flExpireTime = gpGlobals->curtime + flBleedingTime;

	// See if this weapon has already applied a bleed and extend the time.
	FOR_EACH_VEC( m_PlayerBleeds, i )
	{
		if ( m_PlayerBleeds[i].hBleedingAttacker && m_PlayerBleeds[i].hBleedingAttacker == pPlayer &&
			m_PlayerBleeds[i].hBleedingWeapon && m_PlayerBleeds[i].hBleedingWeapon == pWeapon )
		{
			if ( flExpireTime > m_PlayerBleeds[i].flBleedingRemoveTime )
			{
				m_PlayerBleeds[i].flBleedingRemoveTime = flExpireTime;
				return;
			}
		}
	}

	// New bleed source.
	bleed_struct_t bleedinfo =
	{
		pPlayer,			// Attacker
		pWeapon,			// Weapon
		flStartTime,		// Bleeding Time
		flExpireTime,		// Expiration Time
		nBleedDmg,			// Damage
		bPermanentBleeding
	};
	m_PlayerBleeds.AddToTail( bleedinfo );

	if ( !InCond( TF_COND_BLEEDING ) )
	{
		AddCond( TF_COND_BLEEDING, PERMANENT_CONDITION, pPlayer );
	}
#endif
}

#ifdef GAME_DLL

void CTFPlayerShared::StopBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon )
{
	FOR_EACH_VEC_BACK( m_PlayerBleeds, i )
	{
		const bleed_struct_t &bleed = m_PlayerBleeds[i];
		if ( bleed.hBleedingAttacker == pPlayer && bleed.hBleedingWeapon == pWeapon )
		{
			m_PlayerBleeds.FastRemove( i );
		}
	}

	// Remove the bleeding condition right away when the list empties out.
	if ( !m_PlayerBleeds.Count() )
	{
		RemoveCond( TF_COND_BLEEDING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stun & Snare Application
//-----------------------------------------------------------------------------
void CTFPlayerShared::StunPlayer( float flTime, float flReductionAmount, int iStunFlags, CTFPlayer *pAttacker )
{
	// Insanity prevention.
	if ( (m_PlayerStuns.Count() + 1) >= 250 )
		return;

	if ( InCond( TF_COND_PHASE ) )
		return;

	if ( InCond( TF_COND_MEGAHEAL ) )
		return;

	if ( InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) )
		return;

	stun_struct_t *pActiveStun = GetActiveStunInfo();
	int iOldStunFlags = GetStunFlags();
	float flRemapAmount = RemapValClamped( flReductionAmount, 0.0, 1.0, 0, 255 );

	// Already stunned!
	bool bStomp = false;
	if ( InCond( TF_COND_STUNNED ) )
	{
		if ( pActiveStun )
		{
			// Is it stronger than the active?
			if ( flRemapAmount > pActiveStun->flStunAmount || (iStunFlags & TF_STUN_CONTROLS) || (iStunFlags & TF_STUN_LOSER_STATE) )
			{
				bStomp = true;
			}
			// It's weaker, check if it would expire before the active stack.
			else if ( gpGlobals->curtime + flTime < pActiveStun->flExpireTime )
				return;
		}
	}
	else if ( pActiveStun )
	{
		// Something yanked our TF_COND_STUNNED in an unexpected way.
		if ( !HushAsserts() )
		{
			Assert( !"Something yanked out TF_COND_STUNNED." );
		}

		m_PlayerStuns.RemoveAll();
		return;
	}

	// Add it to the stack.
	stun_struct_t stunEvent =
	{
		pAttacker,						// Player
		flTime,							// Duration
		gpGlobals->curtime + flTime,	// Expiration Time
		gpGlobals->curtime + flTime,	// Fading Time
		flRemapAmount,					// Stun Amount
		iStunFlags						// Stun Flags
	};

	// Should this become the active stun?
	if ( bStomp || !pActiveStun )
	{
		// If stomping, see if the stun we're replacing has a stronger slow.
		// This can happen when stuns use TF_STUN_CONTROLS or TF_STUN_LOSER_STATE.
		float flOldStun = pActiveStun ? pActiveStun->flStunAmount : 0.0f;

		m_iStunIndex = m_PlayerStuns.AddToTail( stunEvent );

		pActiveStun = GetActiveStunInfo();
		if ( flOldStun > flRemapAmount )
		{
			pActiveStun->flStunAmount = flOldStun;
		}
	}
	else
	{
		// Done for now
		m_PlayerStuns.AddToTail( stunEvent );
		return;
	}

	// Add in extra time when controls get stunned.
	if ( pActiveStun->iStunFlags & TF_STUN_CONTROLS )
	{
		pActiveStun->flExpireTime += CONTROL_STUN_ANIM_TIME;
	}

	pActiveStun->flStartFadeTime = gpGlobals->curtime + pActiveStun->flDuration;

	// Update the old systems.
	UpdateLegacyStunSystem();

	if ( pActiveStun->iStunFlags & TF_STUN_CONTROLS || pActiveStun->iStunFlags & TF_STUN_LOSER_STATE )
	{
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_STUNNED );
		if ( pAttacker )
		{
			pAttacker->SpeakConceptIfAllowed( MP_CONCEPT_STUNNED_TARGET );
		}
	}

	if ( !(pActiveStun->iStunFlags & TF_STUN_NO_EFFECTS) )
	{
		m_pOuter->StunSound( pAttacker, pActiveStun->iStunFlags, iOldStunFlags );
	}

	// Clear off all taunts, expressions, and scenes.
	if ( (pActiveStun->iStunFlags & TF_STUN_CONTROLS) == TF_STUN_CONTROLS || (pActiveStun->iStunFlags & TF_STUN_LOSER_STATE) == TF_STUN_LOSER_STATE )
	{
		m_pOuter->StopTaunt();
		m_pOuter->ClearExpression();
		m_pOuter->ClearWeaponFireScene();
	}

	AddCond( TF_COND_STUNNED, PERMANENT_CONDITION, pAttacker );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns the intensity of the current stun effect, if we have the type of stun indicated.
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetAmountStunned( int iStunFlags )
{
	if ( GetActiveStunInfo() && (InCond( TF_COND_STUNNED ) && (iStunFlags & GetActiveStunInfo()->iStunFlags) && GetActiveStunInfo()->flExpireTime > gpGlobals->curtime) )
		return Min( Max( GetActiveStunInfo()->flStunAmount, 0.0f ), 255.0f ) * (1.0f / 255.0f);

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our controls are stunned.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsControlStunned( void )
{
	if ( GetActiveStunInfo() && (InCond( TF_COND_STUNNED ) && (m_iStunFlags & TF_STUN_CONTROLS)) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our controls are stunned.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoserStateStunned( void ) const
{
	if ( GetActiveStunInfo() && (InCond( TF_COND_STUNNED ) && (m_iStunFlags & TF_STUN_LOSER_STATE)) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our movement is slowed, but our controls are still free.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsSnared( void )
{
	if ( InCond( TF_COND_STUNNED ) && !IsControlStunned() )
		return true;

	return false;
}


void CTFPlayerShared::AirblastPlayer( CTFPlayer *pAttacker, const Vector &vecDir, float flSpeed )
{
	m_pOuter->SetGroundEntity( NULL );
	m_pOuter->ApplyAbsVelocityImpulse( vecDir * flSpeed );
	m_vecAirblastPos = pAttacker->GetAbsOrigin();
	AddCond( TF_COND_AIRBLASTED, 0.5f, pAttacker );
}

bool CTFPlayerShared::CheckShieldBash( void )
{
	bool bHitSomething = false;

#ifdef GAME_DLL
	START_LAG_COMPENSATION( m_pOuter, m_pOuter->GetCurrentCommand() );
#endif

	float flRange = 48.0f;
	int iDamage = 50;
	int iDamageType = DMG_CLUB;
	float flWidePushback = 0;
	ETFDmgCustom iDamageCustom = TF_DMG_CUSTOM_NONE;

	CBaseEntity *pProvider = GetConditionProvider(TF_COND_SHIELD_CHARGE);
	if( pProvider && dynamic_cast<CTFWeaponBaseMelee*>(pProvider) )
	{
		flRange = ((CTFWeaponBaseMelee*)pProvider)->GetSwingRange();
		iDamage = ((CTFWeaponBaseMelee*)pProvider)->GetMeleeDamage( NULL, iDamageCustom );
		iDamageType = ((CTFWeaponBaseMelee*)pProvider)->GetDamageType();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pProvider, iDamage, mult_shield_bash_damage );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pProvider, flWidePushback, add_shield_bash_aoe_knockback );
	}
	else
	{
		int iShieldBashDamage = -1;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iShieldBashDamage, add_set_shield_bash_damage );
		if( iShieldBashDamage != -1 )
			iDamage = iShieldBashDamage;

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, iDamage, mult_shield_bash_damage );
	}

	Vector vecForward; 
	AngleVectors( m_pOuter->EyeAngles(), &vecForward );
	Vector vecStart = m_pOuter->Weapon_ShootPosition();
	Vector vecEnd = vecStart + vecForward * flRange;

	trace_t tr;
	CTraceFilterIgnoreTeammatesAndWorld filter(m_pOuter, COLLISION_GROUP_NONE, m_pOuter->GetTeamNumber());
	UTIL_TraceHull( vecStart, vecEnd, m_pOuter->GetPlayerMins(), m_pOuter->GetPlayerMaxs(),
		MASK_SOLID, &filter, &tr );

	if( tr.m_pEnt && !tr.m_pEnt->IsWorld() )
	{
		bHitSomething = true;

		// Buildings and similar ents
		const char* pszSoundName = "DemoCharge.HitWorld";

		// Players
		if( tr.m_pEnt->IsPlayer() )
			pszSoundName = "DemoCharge.HitFleshRange";

		m_pOuter->EmitSound( pszSoundName );

		CTakeDamageInfo info( m_pOuter, m_pOuter, pProvider, iDamage, iDamageType, iDamageCustom );
		CalculateMeleeDamageForce( &info, vecForward, vecEnd, 1.0f / iDamage * tf_meleeattackforcescale.GetFloat() );
		tr.m_pEnt->DispatchTraceAttack( info, vecForward, &tr );
		ApplyMultiDamage();

		if( flWidePushback > 0 )
		{
			// If we hit a player, we also push players near ourselves back
			CBaseEntity *pList[64];

			// Do a spehere check to get all the players around  us
			int count = UTIL_EntitiesInSphere( pList, 64, m_pOuter->EyePosition(), flWidePushback, 0 );

			for( int i = 0; i < count; i++ )
			{
				CBaseEntity *pEntity = pList[ i ];

				if( !pEntity )
					continue;

				if( !pEntity->IsAlive() )
					continue;

				if( !pEntity->IsPlayer() )
					continue;

				if( !m_pOuter->IsEnemy(pEntity) )
					continue;

#ifdef GAME_DLL
				// Make sure we can actually see this entity so we don't hit anything through walls.
				if( !m_pOuter->FVisible( pEntity, MASK_SOLID ) )
					continue;
#endif

				CTFPlayer *pEnemy = static_cast<CTFPlayer *>( pEntity );
			
				// Code borrowed from the airblast function
				Vector vecPushDir;
				QAngle angPushDir = m_pOuter->EyeAngles();
				float flPitch = AngleNormalize( angPushDir[PITCH] );

				if( pEnemy->GetGroundEntity() )
				{
					// If they're on the ground, always push them at least 45 degrees up.
					angPushDir[PITCH] = Min( -45.0f, flPitch );
				}
				else if( flPitch > -45.0f )
				{
					// Proportionally raise the pitch.
					float flScale = RemapValClamped( flPitch, 0.0f, 90.0f, 1.0f, 0.0f );
					angPushDir[PITCH] = Max( -45.0f, flPitch - 45.0f * flScale );
				}

				AngleVectors( angPushDir, &vecPushDir );
				// Don't push players if they're too far off to the side. Ignore Z.
				Vector2D vecVictimDir = pEntity->WorldSpaceCenter().AsVector2D() - m_pOuter->WorldSpaceCenter().AsVector2D();
				Vector2DNormalize( vecVictimDir );

				Vector2D vecDir2D = vecPushDir.AsVector2D();
				Vector2DNormalize( vecDir2D );

				float flDot = DotProduct2D( vecDir2D, vecVictimDir );
				if ( flDot >= 0.8f )
				{
					// Push enemy players.
					// Strenght is half of a regular airblast
					pEnemy->m_Shared.AirblastPlayer( m_pOuter, vecPushDir, 250 );
#ifdef GAME_DLL
					pEnemy->AddDamagerToHistory( m_pOuter, pProvider );
#endif
				}
			}
		}
	}

#ifdef GAME_DLL
	FINISH_LAG_COMPENSATION();
#endif

	return bHitSomething;
}

void CTFPlayerShared::StopCharge( bool bHitSomething )
{
	if( bHitSomething )
		UTIL_ScreenShake( m_pOuter->WorldSpaceCenter(), 25.0, 150.0, 1.0, 750, SHAKE_START );

	// HACK HACK, stop charge should probably be some sort of Interface or something like that later on
	CTFWearable *pWearable = dynamic_cast<CTFWearable*>( GetConditionProvider(TF_COND_SHIELD_CHARGE) );
	CTFRiot *pRiot = dynamic_cast<CTFRiot*>( GetConditionProvider(TF_COND_SHIELD_CHARGE) );
	if( pRiot )
		pRiot->StopCharge();
	else
		if( pWearable )
			pWearable->StopEffect();

	RemoveCond( TF_COND_SHIELD_CHARGE );
//	RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
}


void CTFPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	if ( m_pBurningSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBurningSound );
		m_pBurningSound = NULL;
	}

	if ( m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pBurningEffect );
		m_pBurningEffect = NULL;
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
#else
	m_hBurnAttacker = NULL;
	m_hBurnWeapon = NULL;
#endif
}


void CTFPlayerShared::OnAddStealthed( void )
{
	CTFWeaponInvis *pInvisWatch = static_cast<CTFWeaponInvis *>(m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ));

	float flInstantCloakOnSmokeBomb = 0.0f;	// !!! foxysen speedwatch
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flInstantCloakOnSmokeBomb, smoke_bomb_on_full_cloak );

#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	if ( m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pDisguisingEffect );
		m_pDisguisingEffect = NULL;
	}

	if ( !flInstantCloakOnSmokeBomb || GetSpyCloakMeter() < 100.0f )
	{
		if ( pInvisWatch && pInvisWatch->HasSpeedCloak() )
		{
			m_pOuter->EmitSound( "Player.Spy_SpeedCloak" );
		}
		else if ( pInvisWatch )
		{
			pInvisWatch->WeaponSound( SPECIAL1 );
		}
	}

	m_pOuter->RemoveAllDecals();
	m_pOuter->UpdateSpyStateChange();
#endif

	bool bSetInvisChangeTime = true;
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsLocalPlayer() )
	{
		// We only clientside predict changetime for the local player.
		bSetInvisChangeTime = false;
	}
#endif

	if ( bSetInvisChangeTime )
	{
		float flInvisTime = tf_spy_invis_time.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flInvisTime, mult_cloak_rate );

		if ( flInstantCloakOnSmokeBomb && GetSpyCloakMeter() >= 100.0f )
		{
			m_flInvisChangeCompleteTime = gpGlobals->curtime;
		}
		else
		{
			m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisTime;
		}
	}

	if ( InCond( TF_COND_STEALTHED ) )
	{
		m_pOuter->SetOffHandWeapon( pInvisWatch );
	}

	m_pOuter->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	m_pOuter->DropFlag();

	// Tell all dispensers that are healing us to update their particles.
	for ( int i = 0, c = m_vecHealers.Count(); i < c; i++ )
	{
		if ( !m_vecHealers[i].pDispenser )
			continue;

		assert_cast<CObjectDispenser *>(m_vecHealers[i].pDispenser.Get())->UpdateHealingTargets();
	}
#endif
}


void CTFPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	if ( !m_bSyncingConditions )
		return;

	if ( !m_pDisguisingEffect && InCond( TF_COND_DISGUISING ) && ( !m_pOuter->IsLocalPlayer() || !m_pOuter->InFirstPersonView() ) && ( !IsStealthed() || !m_pOuter->IsEnemyPlayer() ) )
	{
		m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( ConstructTeamParticle( "spy_start_disguise_%s", m_pOuter->GetTeamNumber() ), PATTACH_ABSORIGIN_FOLLOW );
	}

	CTFWeaponInvis *pInvisWatch = static_cast<CTFWeaponInvis *>( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );

	int iReducedCloak = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iReducedCloak, set_quiet_unstealth );
	if ( iReducedCloak == 1 )
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloakReduced" );
	}
	else if ( pInvisWatch )
	{
		if( pInvisWatch->HasSpeedCloak() )
		{
			m_pOuter->EmitSound( "Player.Spy_SpeedUnCloak" );
		}
		else
		{
			pInvisWatch->WeaponSound( SPECIAL2 );
		}
	}
#endif

	RemoveCond( TF_COND_INVULNERABLE_SMOKE_BOMB );	// !!! foxysen speedwatch

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_pOuter->UpdateSpyStateChange();
#else
	// Tell all dispensers that are healing us to update their particles.
	for ( int i = 0, c = m_vecHealers.Count(); i < c; i++ )
	{
		if ( !m_vecHealers[i].pDispenser )
			continue;

		assert_cast<CObjectDispenser *>(m_vecHealers[i].pDispenser.Get())->UpdateHealingTargets();
	}
#endif
}


void CTFPlayerShared::OnRemoveDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && m_bSyncingConditions )
		return;

	if ( m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pDisguisingEffect );
		m_pDisguisingEffect = NULL;
	}

	if ( InCond( TF_COND_RESISTANCE_BUFF ) )
	{
		m_bEmittedResistanceBuff = false;
		OnAddResistanceBuff();
	}

	if ( InCond( TF_COND_CIV_DEFENSEBUFF ) )
	{
		OnAddCivResistanceBoost();
	}

	if ( InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		OnAddCivSpeedBoost();
	}
#else
	// PistonMiner: Removed the reset as we need this for later.

	//m_nDesiredDisguiseTeam = TEAM_UNASSIGNED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}


void CTFPlayerShared::OnRemoveDisguised( void )
{
#ifdef CLIENT_DLL
	// If local player is on the other team, reset the model of this player
	if ( m_pOuter->IsEnemyPlayer() )
	{
		m_pOuter->SetModelIndex( modelinfo->GetModelIndex( m_pOuter->GetPlayerClass()->GetModelName() ) );
	}

	// They may have called for medic and created a visible medic bubble
	m_pOuter->ParticleProp()->StopParticlesNamed( "speech_mediccall", true );

	if ( InCond( TF_COND_RESISTANCE_BUFF ) )
	{
		m_bEmittedResistanceBuff = false;
		OnAddResistanceBuff();
	}

	if ( InCond( TF_COND_CIV_DEFENSEBUFF ) )
	{
		OnAddCivResistanceBoost();
	}

	if ( InCond( TF_COND_CIV_SPEEDBUFF ) )
	{
		OnAddCivSpeedBoost();
	}
#else
	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass.Set( TF_CLASS_UNDEFINED );
	m_nMaskClass = TF_CLASS_UNDEFINED;
	m_hDisguiseTarget.Set( NULL );
	m_iDisguiseTargetIndex = 0;
	m_iDisguiseMaxHealth = 0;
	m_iDisguiseHealth = 0;
	m_flDisguiseChargeLevel = 0.0f;
	m_nDisguiseBody = 0;
	m_iDisguiseClip = -1;
	m_iDisguiseAmmo = -1;

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->ClearExpression();

	m_pOuter->ClearDisguiseWeaponList();
#endif

#ifdef GAME_DLL
	RemoveDisguiseWearables();
#endif
	RemoveDisguiseWeapon();

	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->TeamFortress_SetGravity();
}

void CTFPlayerShared::OnAddShieldCharge( void )
{
	m_flChargeTooSlowTime = 0.0f;
	m_pOuter->TeamFortress_SetSpeed();
#ifdef GAME_DLL
	// Will do for the first sound try
	m_pOuter->EmitSound("GenericShieldCharge");
#endif
}

void CTFPlayerShared::OnRemoveShieldCharge( void )
{
	m_flChargeTooSlowTime = 0.0f;
	m_pOuter->TeamFortress_SetSpeed();
#ifdef GAME_DLL
	// Will do for the first sound try
	m_pOuter->StopSound("GenericShieldCharge");
#endif
}


void CTFPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect.
	if ( !m_pBurningEffect )
	{
		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
	}
	else
	{
		m_pOuter->ParticleProp()->StopEmission( m_pBurningEffect );
		m_pBurningEffect = NULL;
	}

	m_pBurningEffect = m_pOuter->ParticleProp()->Create( ConstructTeamParticle( "burningplayer_%s", m_pOuter->IsDisguisedEnemy() ? GetDisguiseTeam() : m_pOuter->GetTeamNumber() ), PATTACH_ABSORIGIN_FOLLOW );

	// Start the looping burn sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( !m_pBurningSound )
	{
		CLocalPlayerFilter filter;
		m_pBurningSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Player.OnFire" );
	}

	controller.Play( m_pBurningSound, 0.0, 100 );
	controller.SoundChangeVolume( m_pBurningSound, 1.0, 0.1 );
#else
	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
#endif
}


float CTFPlayerShared::GetStealthNoAttackExpireTime( void )
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player.
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated )
{
	m_bPlayerDominated.Set( pPlayer->entindex(), bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player.
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated )
{
	m_bPlayerDominatingMe.Set( pPlayer->entindex(), bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
	return m_bPlayerDominated.Get( iPlayerIndex );
}


bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}


void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// We took damage.
	if ( (nDamage > 5 || InCond( TF_COND_BLEEDING )) && InCond( TF_COND_STEALTHED ) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}


void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	m_flLastStealthExposeTime = gpGlobals->curtime;
	AddCond( TF_COND_STEALTHED_BLINK );
}


void CTFPlayerShared::FadeInvis( float flCloakRateScale, bool bNoAttack /*= false*/ )
{
	ETFCond nExpiringCondition = TF_COND_LAST;
	if ( InCond( TF_COND_STEALTHED ) )
	{
		nExpiringCondition = TF_COND_STEALTHED;
		RemoveCond( TF_COND_STEALTHED );
	}

#ifdef GAME_DLL
	// Whisper to the bots of this event.
	CTFWeaponInvis *pWeaponInvis = assert_cast<CTFWeaponInvis *>(m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ));
	if ( pWeaponInvis )
	{
		TheNextBots().OnWeaponFired( m_pOuter, pWeaponInvis );
	}
#endif

	// Give a custom invisibility weapon a chance to override the decloak rate scale.
	float flDecloakRateScale = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flDecloakRateScale, mult_decloak_rate );
	if ( flDecloakRateScale <= 0.0f )
	{
		flDecloakRateScale = 1.0f;
	}

	float flInvisFadeTime = flCloakRateScale * (tf_spy_invis_unstealth_time.GetFloat() * flDecloakRateScale);
	if ( flInvisFadeTime >= 0.15f || bNoAttack )
	{
		// Attack sometime later.
		m_flStealthNoAttackExpire = gpGlobals->curtime + (tf_spy_cloak_no_attack_time.GetFloat() * flDecloakRateScale * flCloakRateScale);
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;/*tf_spy_stealth_blink_scale.GetFloat();*/
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f - ((m_flInvisChangeCompleteTime - gpGlobals->curtime));
		}
		else
		{
			flTargetInvis = ((m_flInvisChangeCompleteTime - gpGlobals->curtime) * 0.5f);
		}
	}
	else
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f;
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;
	m_flInvisibility = clamp( flTargetInvis, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player? [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible( void )
{
	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass )
{
	// Invalid team.
	if ( nTeam != TF_TEAM_GLOBAL && (nTeam < FIRST_GAME_TEAM || nTeam >= GetNumberOfTeams()) )
		return;

	if ( nTeam == TF_TEAM_GLOBAL && !TFGameRules()->IsFourTeamGame() )
		return;

	// We're not Spy.
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();
	if ( nRealClass != TF_CLASS_SPY )
		return;

	// We're not disguising as anything but ourselves (so reset everything).
	int nRealTeam = m_pOuter->GetTeamNumber();
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	Assert( nClass >= TF_FIRST_NORMAL_CLASS && nClass <= TF_LAST_NORMAL_CLASS );

	// Not allowed to disguise while taunting.
	if ( InCond( TF_COND_TAUNTING ) )
		return;

	// Ignore disguise of the same type, switch disguise weapon instead.
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass )
	{
		DetermineDisguiseWeapon( false );
		return;
	}

	// Invalid class; Servers can allow Spies to disguise as gamemode-specific classes.
	if ( nClass <= TF_CLASS_UNDEFINED || nClass > (tf2c_allow_special_classes.GetInt() > 1 ? TF_CLASS_COUNT : TF_LAST_NORMAL_CLASS) )
		return;

	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	m_bLastDisguiseWasEnemyTeam = (m_nDesiredDisguiseTeam != nRealTeam);

	AddCond( TF_COND_DISGUISING );

	// Multiplier for the time taken to disguise
	float flDisguiseTimeMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flDisguiseTimeMult, mult_disguise_speed );

	// Not used but this attribute is defined in TF2
	// so I implemented it because it could be neat to have

	float flDisguiseTime = 2.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flDisguiseTime, disguise_speed_penalty );

	// Start the think to complete our disguise.
	// Switching disguises is faster if we're already disguised.
	m_flDisguiseCompleteTime = gpGlobals->curtime + ((IsDisguised() ? flDisguiseTime / 4 : flDisguiseTime) * flDisguiseTimeMult);
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
void CTFPlayerShared::FindDisguiseTarget( bool bFindOther /*= false*/ )
{
#ifndef CLIENT_DLL
	m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass, bFindOther );

	if ( m_hDisguiseTarget )
	{
		m_iDisguiseTargetIndex.Set( m_hDisguiseTarget.Get()->entindex() );
		Assert( m_iDisguiseTargetIndex >= 1 && m_iDisguiseTargetIndex <= MAX_PLAYERS );
	}
	else
	{
		m_iDisguiseTargetIndex.Set( TF_DISGUISE_TARGET_INDEX_NONE );
	}

	m_pOuter->CreateDisguiseWeaponList( ToTFPlayer( m_hDisguiseTarget.Get() ) );
#endif
}

bool CTFPlayerShared::DisguiseFoolsTeam( int iTeam ) const
{
	return IsDisguised() && (m_nDisguiseTeam == iTeam || m_nDisguiseTeam == TF_TEAM_GLOBAL);
}

bool CTFPlayerShared::IsFooledByDisguise( int iTeam ) const
{
	return m_pOuter->GetTeamNumber() == iTeam || iTeam == TF_TEAM_GLOBAL;
}

int	CTFPlayerShared::GetDisguiseTeam( void ) const
{
#ifdef CLIENT_DLL
	// Clientside, the disguise team is the same as the local player's
	if ( m_nDisguiseTeam == TF_TEAM_GLOBAL && C_TFPlayer::GetLocalTFPlayer() )
	{
		return C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber();
	}
#endif
	return m_nDisguiseTeam;
}

//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise( void )
{
	AddCond( TF_COND_DISGUISED );

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	RemoveCond( TF_COND_DISGUISING );

#ifdef GAME_DLL
	// Update the player model and skin.
	m_pOuter->UpdateModel();
	m_pOuter->ClearExpression();

	FindDisguiseTarget();
	CTFPlayer *pDisguiseTarget = ToTFPlayer( GetDisguiseTarget() );

	// If we have a disguise target with matching class then take their values.
	// Otherwise, generate random health and uber.
	if ( !pDisguiseTarget || pDisguiseTarget && (!pDisguiseTarget->IsPlayerClass( m_nDisguiseClass, true ) || !pDisguiseTarget->IsAlive()) )
	{
		// If we disguised as an enemy who is currently dead or the wrong class, just set us to full health.
		int iMaxHealth = GetPlayerClassData( m_nDisguiseClass )->m_nMaxHealth;
		m_iDisguiseMaxHealth = iMaxHealth;
		m_iDisguiseHealth = random->RandomInt( iMaxHealth / 2, iMaxHealth );

		if ( m_nDisguiseClass == TF_CLASS_MEDIC )
		{
			m_flDisguiseChargeLevel = RandomFloat( 0.0f, 0.99f );
		}
	}
	else
	{
		m_iDisguiseMaxHealth = pDisguiseTarget->GetMaxHealth();
		m_iDisguiseHealth = pDisguiseTarget->GetHealth();

		if ( m_nDisguiseClass == TF_CLASS_MEDIC )
		{
			m_flDisguiseChargeLevel = clamp( pDisguiseTarget->MedicGetChargeLevel(), 0.0f, 0.99f );
		}
	}
#endif

#ifdef GAME_DLL
	DetermineDisguiseWearables();
#endif
	// If we're the Civilian or in Medieval mode, don't force primary weapon because they'll just have melee weapons either way.
	DetermineDisguiseWeapon( !TFGameRules()->IsInMedievalMode() );

	if ( m_nDisguiseClass == TF_CLASS_SPY )
	{
		m_nMaskClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
	}

	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->TeamFortress_SetGravity();

	m_flDisguiseCompleteTime = 0.0f;
}


EHANDLE CTFPlayerShared::GetDisguiseTarget( void ) const
{
#ifdef CLIENT_DLL
	if ( m_iDisguiseTargetIndex == TF_DISGUISE_TARGET_INDEX_NONE )
		return NULL;

	return cl_entitylist->GetNetworkableHandle( m_iDisguiseTargetIndex );
#else
	return m_hDisguiseTarget.Get();
#endif
}


void CTFPlayerShared::SetDisguiseHealth( int iDisguiseHealth )
{
	m_iDisguiseHealth = iDisguiseHealth;
}


int CTFPlayerShared::AddDisguiseHealth( int iHealthToAdd, bool bOverheal /*= false*/ )
{
	Assert( IsDisguised() );

	iHealthToAdd = clamp( iHealthToAdd, 0, (bOverheal ? GetDisguiseMaxBuffedHealth() : GetDisguiseMaxHealth()) - m_iDisguiseHealth );
	if ( iHealthToAdd <= 0 )
		return 0;

	m_iDisguiseHealth += iHealthToAdd;

	return iHealthToAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFPlayerShared::RemoveDisguise( void )
{
	/*if (GetDisguiseTeam() != m_pOuter->GetTeamNumber())
	{
		if ( InCond( TF_COND_TELEPORTED ) )
		{
			RemoveCond( TF_COND_TELEPORTED );
		}
	}*/

	RemoveCond( TF_COND_DISGUISED );
	RemoveCond( TF_COND_DISGUISING );

#ifdef GAME_DLL
	m_pOuter->ClearDisguiseWeaponList();
#endif
}


void CTFPlayerShared::RemoveDisguiseWeapon( void )
{
	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->Drop( Vector( 0, 0, 0 ) );
		m_hDisguiseWeapon = NULL;
	}

#ifdef CLIENT_DLL
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->UpdateVisibility();
	}
#endif
}


void CTFPlayerShared::DetermineDisguiseWeapon( bool bForcePrimary /*= false*/ )
{
	if ( !IsDisguised() )
	{
		RemoveDisguiseWeapon();
		return;
	}

#ifdef GAME_DLL
	const char *strDisguiseWeapon = NULL;

	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	if ( pDisguiseTarget && (!pDisguiseTarget->IsPlayerClass( m_nDisguiseClass, true ) || !pDisguiseTarget->IsAlive()) )
	{
		pDisguiseTarget = NULL;
	}

	// Determine which slot we have active.
	// Primary Slot = 0
	int iCurrentSlot = 0;
	if ( m_pOuter->GetActiveTFWeapon() && !bForcePrimary )
	{
		iCurrentSlot = m_pOuter->GetActiveTFWeapon()->GetSlot();
		
		// Slot 3 is the Disguise Kit, so they are probably using the menu and not a key bind to disguise.
		if ( iCurrentSlot == m_pOuter->Weapon_OwnsThisID( TF_WEAPON_PDA_SPY )->GetSlot() && m_pOuter->GetLastWeapon() )
		{
			iCurrentSlot = m_pOuter->GetLastWeapon()->GetSlot();
		}
	}

	DisguiseWeapon_t *pItemWeapon = NULL;
	if ( pDisguiseTarget )
	{
		DisguiseWeapon_t *pFirstValidWeapon = NULL;

		// Cycle through the target's weapons and see if we have a match.
		// Note that it's possible the disguise target doesn't have a weapon in the slot we want,
		// for example if they have replaced it with an unlockable that isn't a weapon (wearable).
		FOR_EACH_VEC( m_pOuter->m_hDisguiseWeaponList, i )
		{
			DisguiseWeapon_t *pWeapon = &m_pOuter->m_hDisguiseWeaponList[i];
			if ( !pWeapon )
				continue;

			if ( !pFirstValidWeapon )
			{
				pFirstValidWeapon = pWeapon;
			}

			if ( pWeapon->iSlot == iCurrentSlot )
			{
				pItemWeapon = pWeapon;
				break;
			}
		}

		if ( !pItemWeapon )
		{
			pItemWeapon = pFirstValidWeapon;
		}

		if ( pItemWeapon )
		{
			strDisguiseWeapon = STRING( pItemWeapon->iClassname );
		}
	}

	// Grab their weapon.
	CEconItemView *pItem = NULL;
	if ( pItemWeapon )
	{
		pItem = pItemWeapon->pItem;
		if ( pItem )
		{
			// We're already using this weapon, no reason to equip it again.
			if ( m_hDisguiseWeapon && (GetDisguiseTeam() == m_hDisguiseWeapon->GetTeamNumber() && pItem->GetItemDefIndex() == m_hDisguiseWeapon->GetItemID()) )
			{
				// But make sure the extra wearable for it is still visible.
				m_hDisguiseWeapon->UpdateExtraWearable();
				return;
			}

			// It's possible for Spies to be equipped with wrong items for some reason, so put a check here until we can find the root cause.
			if ( !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) && pItem->GetStaticData() && !(pItem->GetStaticData()->used_by_classes & (1 << m_nDisguiseClass)) )
			{
				DevWarning( "Disguise Target is holding an INVALID item! (Class: %d, Item ID: %d)\n", static_cast<int>(m_nDisguiseClass), pItem->GetItemDefIndex() );
				DevWarning( "Equipping with a stock item instead...\n" );

				// NULL it out to forcefully replace it with a Stock weapon.
				pItem = NULL;
				pItemWeapon = NULL;
			}
		}
	}

	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );
	if ( !pItemWeapon && pData )
	{
		int iFirstValidSlot = iCurrentSlot;
		const char *strFirstValidWeapon = NULL;

		// We have not found our item yet, so cycle through the class's default weapons
		// to find a match.
		for ( int i = 0; i < TF_PLAYER_WEAPON_COUNT; ++i )
		{
			if ( pData->m_aWeapons[i] == TF_WEAPON_NONE )
				continue;

			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( WeaponIdToAlias( pData->m_aWeapons[i] ) );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo *>(GetFileWeaponInfoFromHandle( hWpnInfo ));
			if ( !strFirstValidWeapon && pWeaponInfo->szClassName )
			{
				iFirstValidSlot = pWeaponInfo->iSlot;
				strFirstValidWeapon = pWeaponInfo->szClassName;
			}

			if ( pWeaponInfo->iSlot == iCurrentSlot && pWeaponInfo->szClassName )
			{
				strDisguiseWeapon = pWeaponInfo->szClassName;
				break;
			}
		}

		if ( !strDisguiseWeapon && strFirstValidWeapon )
		{
			iCurrentSlot = iFirstValidSlot;
			strDisguiseWeapon = strFirstValidWeapon;
		}

		// We're already using this weapon, no reason to equip it again.
		if ( m_hDisguiseWeapon && (GetDisguiseTeam() == m_hDisguiseWeapon->GetTeamNumber() && !Q_strcmp( strDisguiseWeapon, m_hDisguiseWeapon->GetClassname() )) )
		{
			// But make sure the extra wearable for it is still visible.
			m_hDisguiseWeapon->UpdateExtraWearable();
			return;
		}
	}

	if ( strDisguiseWeapon )
	{
		// Remove the old disguise weapon, if any.
		RemoveDisguiseWeapon();

		// We may need a sub-type if we're a building weapon (Toolbox, Sapper). Otherwise we might always appear as just the Toolbox.
		int iSubType = 0;
		if ( pItemWeapon )
		{
			iSubType = pItemWeapon->iSubType;
		}
		else if ( pDisguiseTarget && (!V_strcmp( strDisguiseWeapon, "tf_weapon_builder" ) || !V_strcmp( strDisguiseWeapon, "tf_weapon_sapper" )) )
		{
			iSubType = pDisguiseTarget->GetPlayerClass()->GetData()->m_aBuildable[0];
		}

		// If they don't have a weapon or an item in the system, just grab a stock one from the inventory.
		if ( !pItem )
		{
			pItem = GetTFInventory()->GetItem( m_nDisguiseClass, (ETFLoadoutSlot)iCurrentSlot, 0 );
		}

		m_hDisguiseWeapon.Set( dynamic_cast<CTFWeaponBase *>(m_pOuter->GiveNamedItem( strDisguiseWeapon, iSubType, pItem, TF_GIVEAMMO_NONE, true )) );
		if ( m_hDisguiseWeapon )
		{
			m_hDisguiseWeapon->SetTouch( NULL ); // NO TOUCHY TOUCHY!!
			m_hDisguiseWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
			m_hDisguiseWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
			m_hDisguiseWeapon->SetOwner( dynamic_cast<CBaseCombatCharacter *>(m_pOuter) );
			m_hDisguiseWeapon->SetOwnerEntity( m_pOuter );
			m_hDisguiseWeapon->SetParent( m_pOuter );
			m_hDisguiseWeapon->FollowEntity( m_pOuter, true );
			m_hDisguiseWeapon->m_iState = WEAPON_IS_ACTIVE;
			m_hDisguiseWeapon->m_bDisguiseWeapon = true;
			m_hDisguiseWeapon->UpdateExtraWearable(); // Make sure that extra wearables on a disguise target's weapon is visible.
			m_hDisguiseWeapon->SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5f, "DisguiseWeaponThink" );

			// Ammo/clip state is displayed to attached medics
			m_iDisguiseClip = -1;
			m_iDisguiseAmmo = -1;
			if ( !m_hDisguiseWeapon->IsMeleeWeapon() )
			{
				// Use the player we're disguised as if possible...
				if ( pDisguiseTarget )
				{
					CTFWeaponBase *pWeapon = pDisguiseTarget->GetActiveTFWeapon();
					if ( pWeapon && pWeapon->GetWeaponID() == m_hDisguiseWeapon->GetWeaponID() )
					{
						if ( pWeapon->UsesClipsForAmmo1() )
						{
							m_iDisguiseClip = pWeapon->Clip1();
						}

						m_iDisguiseAmmo = pDisguiseTarget->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
					}
				}

				// Otherwise display faked ammo clip and count.
				if ( m_iDisguiseClip == -1 && m_hDisguiseWeapon->UsesClipsForAmmo1() )
				{
					m_iDisguiseClip = random->RandomInt( 1, m_hDisguiseWeapon->GetMaxClip1() );
				}

				if ( m_iDisguiseAmmo == -1 )
				{
					m_iDisguiseAmmo = random->RandomInt( 1, m_pOuter->GetMaxAmmo( m_hDisguiseWeapon->GetPrimaryAmmoType(), false, m_nDisguiseClass ) );
				}
			}
		}
	}
#endif
}

#ifdef GAME_DLL

void CTFPlayerShared::DetermineDisguiseWearables()
{
	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	if ( !pDisguiseTarget )
		return;

	// Remove any existing disguise wearables.
	RemoveDisguiseWearables();

	if ( GetDisguiseClass() != pDisguiseTarget->GetPlayerClass()->GetClassIndex() )
		return;

	// Equip us with copies of our disguise target's wearables.
	for ( int i = 0, c = pDisguiseTarget->GetNumWearables(); i < c; ++i )
	{
		CTFWearable *pWearable = dynamic_cast<CTFWearable *>(pDisguiseTarget->GetWearable( i ));
		if ( pWearable )
		{
			// Never copy a disguise target's wearables, that could be scary.
			if ( pWearable->IsDisguiseWearable() )
				continue;

			CEconItemView *pItemWearable = pWearable->GetItem();
			if ( pItemWearable )
			{
				CTFWearable *pDisguiseWearable = dynamic_cast<CTFWearable *>(m_pOuter->GiveNamedItem( pItemWearable->GetEntityName(), 0, pItemWearable ));
				if ( pDisguiseWearable )
				{
					pDisguiseWearable->SetDisguiseWearable( true );
					pDisguiseWearable->GiveTo( m_pOuter );
				}
			}
		}
	}
}


void CTFPlayerShared::RemoveDisguiseWearables()
{
	bool bFoundDisguiseWearable = true;
	while ( bFoundDisguiseWearable )
	{
		int i, c;
		for ( i = 0, c = m_pOuter->GetNumWearables(); i < c; ++i )
		{
			CTFWearable *pWearable = dynamic_cast<CTFWearable *>(m_pOuter->GetWearable( i ));
			if ( pWearable && pWearable->IsDisguiseWearable() )
			{
				// Every time we do this the list changes, so we have to loop through again.
				m_pOuter->RemoveWearable( pWearable );
				break;
			}
		}

		if ( i == m_pOuter->GetNumWearables() )
		{
			bFoundDisguiseWearable = false;
		}
	}

}
#endif

#ifdef CLIENT_DLL

void CTFPlayerShared::UpdateLoopingSounds( bool bDormant )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( bDormant )
	{
		// Pause sounds.
		if ( m_pBurningSound )
		{
			controller.Shutdown( m_pBurningSound );
		}

		if ( m_pCritSound )
		{
			controller.Shutdown( m_pCritSound );
		}
	}
	else
	{
		// Resume any sounds that should still be playing.
		if ( m_pBurningSound )
		{
			controller.Play( m_pBurningSound, 1.0f, 100.0f );
		}

		if ( m_pCritSound )
		{
			controller.Play( m_pCritSound, 1.0f, 100.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Crit effects handling.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritBoostEffect( void )
{
	bool bShouldShow;

	CTFWeaponBase* pWpn = GetActiveTFWeapon();	// !!! foxysen beacon
	CTFBeacon* pWpnBeacon = nullptr;
	int iCannotUseStoredCrits = 0;
	int iCanUseStoredCritAtFull = 0;
	if (pWpn)
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWpn, iCannotUseStoredCrits, cannot_use_stored_crits);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWpn, iCanUseStoredCritAtFull, always_crit_full_stored_crits);
		if (pWpn->GetWeaponID() == TF_WEAPON_BEACON)
		{
			pWpnBeacon = static_cast<CTFBeacon*>(pWpn);
		}
	}

	if ( m_pOuter->IsDormant() )
	{
		bShouldShow = false;
	}
	else if ( IsStealthed() )
	{
		bShouldShow = false;
	}
	else if (pWpnBeacon && pWpnBeacon->IsStoredBeaconCritFilled())
	{
		bShouldShow = true;
	}
	else if (GetStoredCrits() && (!iCannotUseStoredCrits || (iCanUseStoredCritAtFull && GetStoredCrits() == GetStoredCritsCapacity())))
	{
		bShouldShow = true;
	}
	else if (IsCritBoosted())
	{
		bShouldShow = true;
	}
	else
	{
		bShouldShow = false;
	}

	if ( bShouldShow )
	{
		// Update crit effect model.
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
			{
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );
			}

			m_pCritEffect = NULL;
		}

		C_TFPlayer *pPlayer = m_pOuter->GetObservedPlayer( true );
		bool bEnemyDisguise = pPlayer->IsDisguisedEnemy();
		if ( pPlayer->ShouldDrawThisPlayer() )
		{
			// Don't add crit effect to weapons without a model.
			C_BaseCombatWeapon *pWeapon = bEnemyDisguise ? GetDisguiseWeapon() : pPlayer->GetActiveWeapon();
			if ( pWeapon && pWeapon->ShouldDraw() )
			{
				m_hCritEffectHost = pWeapon;
			}
			else
			{
				m_hCritEffectHost = m_pOuter;
			}
		}
		else
		{
			// Get the ViewModel or its attachment if we have one (m_hCritEffectHost is fine being given a NULL pointer).
			C_TFViewModel *pViewModel = static_cast<C_TFViewModel *>( pPlayer->GetViewModel( 0, false ) );
			m_hCritEffectHost = pViewModel;
			if ( pViewModel && pViewModel->m_hViewmodelAddon && pViewModel->GetViewModelType() == VMTYPE_TF2 )
			{
				m_hCritEffectHost = pViewModel->m_hViewmodelAddon.Get();
			}
		}

		if ( m_hCritEffectHost.Get() )
		{
			int iTeamNumber = bEnemyDisguise ? GetDisguiseTeam() : pPlayer->GetTeamNumber();
			if ( pPlayer->ShouldDrawThisPlayer() )
			{
				m_pCritEffect = m_hCritEffectHost->ParticleProp()->Create( ConstructTeamParticle( "critgun_weaponmodel_%s", iTeamNumber, g_aTeamNamesShort ), PATTACH_ABSORIGIN_FOLLOW );
			}
			else
			{
				// first person particles require view model effect to be true.
				// m_pCritEffect->SetIsViewModelEffect doesn't seem to work?
				m_pCritEffect = m_hCritEffectHost->ParticleProp()->Create( ConstructTeamParticle( "critgun_weaponmodel_viewmodel_%s", iTeamNumber, g_aTeamNamesShort ), PATTACH_ABSORIGIN_FOLLOW );
			}
		}

		if ( !m_pCritSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Weapon_General.CritPower" );
			controller.Play( m_pCritSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
			{
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );
			}

			m_pCritEffect = NULL;
		}

		m_hCritEffectHost = NULL;

		if ( m_pCritSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pCritSound );
			m_pCritSound = NULL;
		}
	}
}
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Heal players.
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal( CTFPlayer *pPlayer, float flAmount, CBaseObject *pDispenser /*= NULL*/, bool bRemoveOnMaxHealth /*= false*/, bool bCanCritHeal /*= true*/ )
{
	// Only allow this healer to have a single healing slot.
	//StopHealing(pPlayer);

	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flAmount, mult_healing_from_medics );
	}

	healers_t &newHealer = m_vecHealers[m_vecHealers.AddToTail()];
	newHealer.pPlayer = pPlayer;
	newHealer.pDispenser = pDispenser;
	newHealer.flAmount = flAmount;
	newHealer.bRemoveOnMaxHealth = bRemoveOnMaxHealth;
	newHealer.iRecentAmount = 0;
	newHealer.flNextNofityTime = gpGlobals->curtime + 1.0f;
	newHealer.bRemoveAfterExpiry = false;
	newHealer.healerType = HEALER_TYPE_BEAM;
	newHealer.bCanOverheal = true;
	newHealer.bCritHeals = bCanCritHeal;

	if ( flAmount > 0 )
		AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pPlayer );

	RecalculateChargeEffects();

	m_nNumHealers = m_vecHealers.Count();
	if ( !pDispenser )
		m_nNumHumanHealers++;
}

//-----------------------------------------------------------------------------
// Purpose: Heal players with explicitly no overheal.
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealNoOverheal( CTFPlayer *pPlayer, float flAmount, CBaseObject *pDispenser /*= NULL*/, bool bRemoveOnMaxHealth /*= false*/ )
{
	// Only allow this healer to have a single healing slot.
	//StopHealing(pPlayer);

	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flAmount, mult_healing_from_medics );
	}

	healers_t &newHealer = m_vecHealers[m_vecHealers.AddToTail()];
	newHealer.pPlayer = pPlayer;
	newHealer.pDispenser = pDispenser;
	newHealer.flAmount = flAmount;
	newHealer.bRemoveOnMaxHealth = bRemoveOnMaxHealth;
	newHealer.iRecentAmount = 0;
	newHealer.flNextNofityTime = gpGlobals->curtime + 1.0f;
	newHealer.bRemoveAfterExpiry = false;
	newHealer.healerType = HEALER_TYPE_BEAM;
	newHealer.bCanOverheal = false;

	if ( flAmount > 0 )
		AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pPlayer );

	RecalculateChargeEffects();

	m_nNumHealers = m_vecHealers.Count();
	if ( !pDispenser )
		m_nNumHumanHealers++;
}

//-----------------------------------------------------------------------------
// Purpose: Heal players for a set amount of time, optionally refreshing if reapplied.
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealTimed( CTFPlayer *pPlayer, float flAmount, float flDuration, bool bRefresh, bool bOverheal /* = false*/, CBaseObject *pDispenser /* = NULL*/ )
{
	// Only allow this healer to have a single healing slot.
	//StopHealing(pPlayer);

	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flAmount, mult_healing_from_medics );
	}

	bool bAlreadyHealingTimed = false;
	int i = 0;
	FOR_EACH_VEC( m_vecHealers, i )
	{
		if ( pPlayer == m_vecHealers[i].pPlayer && m_vecHealers[i].bRemoveAfterExpiry ) {
			bAlreadyHealingTimed = true;
			break;
		}
	}

	// Only proceed if we're a new healer or we're allowed to refresh
	if ( bAlreadyHealingTimed && !bRefresh )
		return;

	if ( bAlreadyHealingTimed )
	{
		// Refresh our expiry time for this healer.
		m_vecHealers[i].flAutoRemoveTime = gpGlobals->curtime + flDuration;
	}
	else
	{
		// Add a new healer
		healers_t &newHealer = m_vecHealers[m_vecHealers.AddToTail()];
		newHealer.pPlayer = pPlayer;
		newHealer.pDispenser = pDispenser;
		newHealer.flAmount = flAmount;
		newHealer.bRemoveOnMaxHealth = !bOverheal;
		newHealer.bCanOverheal = bOverheal;
		newHealer.iRecentAmount = 0;
		newHealer.flNextNofityTime = gpGlobals->curtime + 1.0f;
		newHealer.bRemoveAfterOneFrame = false;
		newHealer.bRemoveAfterExpiry = true;
		newHealer.flAutoRemoveTime = gpGlobals->curtime + flDuration;
		newHealer.healerType = HEALER_TYPE_PASSIVE;

		if ( flAmount > 0 )
			AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pPlayer );

		m_nNumHealers = m_vecHealers.Count();
		if ( !pDispenser )
			m_nNumHumanHealers++;
	}

	RecalculateChargeEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Remove a particular healer, identified by index.
// This becomes necessary if the same medic is added as multiple healers for
// the sake of healing via multiple methods at the same time.
//-----------------------------------------------------------------------------
void CTFPlayerShared::StopHealing( int iIndex )
{
	if ( iIndex != m_vecHealers.InvalidIndex() )
	{
		CTFPlayer *pHealer = ToTFPlayer( m_vecHealers[iIndex].pPlayer.Get() );

		if ( !m_vecHealers[iIndex].pDispenser )
			m_nNumHumanHealers--;

		if ( m_vecHealers[iIndex].iRecentAmount > 0 && m_vecHealers[iIndex].flAmount > 0 )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
			if ( event )
			{
				event->SetInt( "priority", 1 );
				event->SetInt( "patient", m_pOuter->GetUserID() );
				event->SetInt( "healer", pHealer ? pHealer->GetUserID() : -1 );
				event->SetInt( "amount", m_vecHealers[iIndex].iRecentAmount );
				event->SetInt( "class", m_pOuter->GetPlayerClass()->GetClassIndex() );

				gameeventmanager->FireEvent( event );
			}
		}

		//DevMsg( "#### Player stopped healing patient (%s), removed at index %i ####\n", m_pOuter->GetPlayerName(), iIndex );

		m_vecHealers.Remove( iIndex );

		if ( !m_vecHealers.Count() )
		{
			RemoveCond( TF_COND_HEALTH_BUFF );
			//DevMsg( "#### TF_COND_HEALTH_BUFF removed from %s ####\n", m_pOuter->GetPlayerName() );
		}

		RecalculateChargeEffects();

		m_nNumHealers = m_vecHealers.Count();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop healing players, but specifically select the type of healing
// to remove, as healers may be applying multiple kinds at once.
//-----------------------------------------------------------------------------
void CTFPlayerShared::StopHealing( CTFPlayer *pPlayer, HealerType healType)
{
	int iIndex = FindHealerIndex( pPlayer, healType );
	StopHealing( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a healer for attribution that is flagged for to be deleted
// after 1 frame. Use when healing is to attributed to a "burst" healing weapon.
// The healing amount specified in flAmount will be interpreted without multiplying
// it by the frame time.
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddBurstHealer( CTFPlayer *pHealer, float flAmount, ETFCond condToApply /*= ETFCond::TF_COND_INVALID*/, float flCondApplyTime /*= 0.0f*/ )
{
	healers_t &newHealer = m_vecHealers[m_vecHealers.AddToTail()];
	newHealer.pPlayer = pHealer;
	newHealer.pDispenser = NULL;
	newHealer.flAmount = flAmount;
	newHealer.bRemoveOnMaxHealth = false;
	newHealer.iRecentAmount = 0;
	newHealer.flNextNofityTime = gpGlobals->curtime + 1.0f;
	newHealer.bRemoveAfterOneFrame = true;
	newHealer.healerType = HEALER_TYPE_BURST;
	newHealer.bCanOverheal = true;
	newHealer.condApplyOnRemove = condToApply;
	newHealer.condApplyTime = flCondApplyTime;

	if ( flAmount > 0 )
		AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pHealer );

	RecalculateChargeEffects();

	/*if ( m_vecHealers[iIndex].iRecentAmount > 0 && m_vecHealers[iIndex].flAmount > 0 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
		if ( event )
		{
			event->SetInt( "priority", 1 );
			event->SetInt( "patient", m_pOuter->GetUserID() );
			event->SetInt( "healer", pHealer ? pHealer->GetUserID() : -1 );
			event->SetInt( "amount", m_vecHealers[iIndex].iRecentAmount );
			event->SetInt( "class", m_pOuter->GetPlayerClass()->GetClassIndex() );

			gameeventmanager->FireEvent( event );
		}
	}*/

	m_nNumHealers = m_vecHealers.Count();
	m_nNumHumanHealers++;
}


medigun_charge_types CTFPlayerShared::GetChargeEffectBeingProvided( CTFPlayer *pPlayer, bool bAutoSelfCheck )
{
	if ( !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		return TF_CHARGE_NONE;

	if ( !m_pOuter->IsBot() )
	{
		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( m_pOuter->entindex() );
		if ( !pNetChanInfo || pNetChanInfo->IsTimingOut() )
			return TF_CHARGE_NONE;

		float flUberDuration = weapon_medigun_chargerelease_rate.GetFloat();

		// Return invalid when the medic hasn't sent a usercommand in awhile.
		if ( m_pOuter->GetTimeSinceLastUserCommand() > flUberDuration + 1.0f )
			return TF_CHARGE_NONE;

		// Prevent an exploit where clients invalidate tickcount,
		// which causes their think functions to shut down.
		if ( m_pOuter->GetTimeSinceLastThink() > flUberDuration )
			return TF_CHARGE_NONE;
	}

	ITFHealingWeapon *pMedigun = pPlayer->GetMedigun();
	if ( pMedigun )
	{
		if (pMedigun->IsReleasingCharge())
		{
			return bAutoSelfCheck && !pMedigun->AutoChargeOwner() ? TF_CHARGE_NONE : pMedigun->GetChargeType();
		}
		
		CBaseEntity *pOwnedGenerator = pPlayer->m_pOwnedUberGenerator.Get();
		if (pOwnedGenerator)
		{
			CTFGeneratorUber* pGenerator = static_cast<CTFGeneratorUber*>(pOwnedGenerator);
			if( pGenerator && pGenerator->IsGeneratorActive() )
				return bAutoSelfCheck && !pMedigun->AutoChargeOwner() ? TF_CHARGE_NONE : pMedigun->GetChargeType();
		}
	}

	return TF_CHARGE_NONE;
}


void CTFPlayerShared::RecalculateChargeEffects( bool bInstantRemove )
{
	bool bShouldCharge[TF_CHARGE_COUNT] = {};
	CTFPlayer *pProviders[TF_CHARGE_COUNT] = {};

	if ( m_pOuter->m_flPowerPlayTime > gpGlobals->curtime )
	{
		for ( int i = 0; i < TF_CHARGE_COUNT; i++ )
		{
			bShouldCharge[i] = true;
			pProviders[i] = NULL;
		}
	}
	else
	{
		// Charging self?
		medigun_charge_types selfCharge = GetChargeEffectBeingProvided( m_pOuter, true );
		if ( selfCharge != TF_CHARGE_NONE )
		{
			bShouldCharge[selfCharge] = true;
			pProviders[selfCharge] = m_pOuter;
		}
		else
		{
			// Check players healing us.
			FOR_EACH_VEC( m_vecHealers, i )
			{
				CTFPlayer *pPlayer = ToTFPlayer( m_vecHealers[i].pPlayer );
				if ( !pPlayer )
					continue;

				// "Burst" and passive healers don't provide uber effects via healing directly, like Mediguns do.
				if ( m_vecHealers[i].bRemoveAfterOneFrame || m_vecHealers[i].healerType != HEALER_TYPE_BEAM )
					continue;

				medigun_charge_types chargeType = GetChargeEffectBeingProvided( pPlayer );
				if ( chargeType != TF_CHARGE_NONE )
				{
					bShouldCharge[chargeType] = true;
					pProviders[chargeType] = pPlayer;
				}
			}
		}
	}

	// Drop the flag with stock uber.
	if ( bShouldCharge[TF_CHARGE_INVULNERABLE] )
	{
		m_pOuter->DropFlag();
	}

	for ( int i = 0; i < TF_CHARGE_COUNT; i++ )
	{
		SetChargeEffect( (medigun_charge_types)i, bShouldCharge[i], bInstantRemove, g_MedigunEffects[i], i == TF_CHARGE_INVULNERABLE ? tf_invuln_time.GetFloat() : 0.0f, pProviders[i] );
	}
}


void CTFPlayerShared::SetChargeEffect( medigun_charge_types chargeType, bool bShouldCharge, bool bInstantRemove, const MedigunEffects_t &chargeEffect, float flRemoveTime, CTFPlayer *pProvider )
{
	if ( InCond( chargeEffect.condition_enable ) == bShouldCharge )
	{
		if ( bShouldCharge && m_flChargeOffTime[chargeType] != 0.0f )
		{
			m_flChargeOffTime[chargeType] = 0.0f;

			if ( chargeEffect.condition_disable != TF_COND_LAST )
			{
				RemoveCond( chargeEffect.condition_disable );
			}
		}

		return;
	}

	if ( bShouldCharge )
	{
		Assert( chargeType != TF_CHARGE_INVULNERABLE || !m_pOuter->HasTheFlag() );

		if ( m_flChargeOffTime[chargeType] != 0.0f )
		{
			m_pOuter->StopSound( chargeEffect.sound_disable );

			m_flChargeOffTime[chargeType] = 0.0f;

			if ( chargeEffect.condition_disable != TF_COND_LAST )
			{
				RemoveCond( chargeEffect.condition_disable );
			}
		}

		// Charge on.
		AddCond( chargeEffect.condition_enable, PERMANENT_CONDITION, pProvider );

		//CSingleUserRecipientFilter filter( m_pOuter );
		//m_pOuter->EmitSound( filter, m_pOuter->entindex(), chargeEffect.sound_enable );
		m_pOuter->EmitSound(chargeEffect.sound_enable);
		m_bChargeSounds[chargeType] = true;
	}
	else
	{
		if ( m_bChargeSounds[chargeType] )
		{
			m_pOuter->StopSound( chargeEffect.sound_enable );
			m_bChargeSounds[chargeType] = false;
		}

		if ( m_flChargeOffTime[chargeType] == 0.0f )
		{
			//CSingleUserRecipientFilter filter( m_pOuter );
			//m_pOuter->EmitSound( filter, m_pOuter->entindex(), chargeEffect.sound_disable );
			m_pOuter->EmitSound( chargeEffect.sound_disable );
		}

		if ( bInstantRemove )
		{
			m_flChargeOffTime[chargeType] = 0.0f;
			RemoveCond( chargeEffect.condition_enable );

			if ( chargeEffect.condition_disable != TF_COND_LAST )
			{
				RemoveCond( chargeEffect.condition_disable );
			}
		}
		else
		{
			// Already turning it off?
			if ( m_flChargeOffTime[chargeType] != 0.0f )
				return;

			if ( chargeEffect.condition_disable != TF_COND_LAST )
			{
				AddCond( chargeEffect.condition_disable, PERMANENT_CONDITION, pProvider );
			}

			m_flChargeOffTime[chargeType] = gpGlobals->curtime + flRemoveTime;
		}
	}
}


void CTFPlayerShared::TestAndExpireChargeEffect( medigun_charge_types chargeType )
{
	if ( InCond( g_MedigunEffects[chargeType].condition_enable ) )
	{
		bool bRemoveCharge = false;

		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber() )
		{
			bRemoveCharge = true;
		}

		if ( m_flChargeOffTime[chargeType] != 0.0f )
		{
			if ( gpGlobals->curtime > m_flChargeOffTime[chargeType] )
			{
				bRemoveCharge = true;
			}
		}

		// Check healers for a possible usercommand invuln exploit.
		FOR_EACH_VEC( m_vecHealers, i )
		{
			CTFPlayer *pTFHealer = ToTFPlayer( m_vecHealers[i].pPlayer );
			if ( !pTFHealer )
				continue;

			CTFPlayer *pTFProvider = ToTFPlayer( GetConditionProvider( g_MedigunEffects[chargeType].condition_enable ) );
			if ( !pTFProvider )
				continue;

			if ( pTFProvider == pTFHealer && pTFHealer->GetTimeSinceLastUserCommand() > weapon_medigun_chargerelease_rate.GetFloat() + 1.0f )
			{
				// Force remove uber and detach the medigun.
				bRemoveCharge = true;
				pTFHealer->Weapon_Switch( pTFHealer->Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
			}
		}

		if ( bRemoveCharge )
		{
			m_flChargeOffTime[chargeType] = 0.0f;

			if ( g_MedigunEffects[chargeType].condition_disable != TF_COND_LAST )
			{
				RemoveCond( g_MedigunEffects[chargeType].condition_disable );
			}

			RemoveCond( g_MedigunEffects[chargeType].condition_enable );
		}
	}
	else if ( m_bChargeSounds[chargeType] )
	{
		// If we're still playing charge sound but not actually charged, stop the sound.
		// This can happen if player respawns while crit boosted.
		m_pOuter->StopSound( g_MedigunEffects[chargeType].sound_enable );
		m_bChargeSounds[chargeType] = false;
	}
}


int	CTFPlayerShared::FindHealerIndex( CTFPlayer *pPlayer, HealerType healType )
{
	FOR_EACH_VEC( m_vecHealers, i )
	{
		if ( m_vecHealers[i].pPlayer == pPlayer && m_vecHealers[i].healerType == healType )
			return i;
	}

	return m_vecHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_vecHealers.Count() > 0 )
		return m_vecHealers.Head().pPlayer;

	return NULL;
}


CBaseEntity *CTFPlayerShared::GetHealerByIndex( int iHealerIndex )
{
	if ( m_vecHealers.IsValidIndex( iHealerIndex ) )
		return m_vecHealers[iHealerIndex].pPlayer;

	Assert( false );
	return NULL;
}


bool CTFPlayerShared::HealerIsDispenser( int iHealerIndex )
{
	if ( m_vecHealers.IsValidIndex( iHealerIndex ) )
		return m_vecHealers[iHealerIndex].pDispenser != NULL;

	Assert( false );
	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Get all of our conditions in a nice CBitVec.
//-----------------------------------------------------------------------------
void CTFPlayerShared::GetConditionsBits( CBitVec<TF_COND_LAST>& vbConditions ) const
{
	vbConditions.Set( 0u, (uint32)m_nPlayerCond );
	vbConditions.Set( 1u, (uint32)m_nPlayerCondEx );
	vbConditions.Set( 2u, (uint32)m_nPlayerCondEx2 );
	vbConditions.Set( 3u, (uint32)m_nPlayerCondEx3 );
	vbConditions.Set( 4u, (uint32)m_nPlayerCondEx4 );
	COMPILE_TIME_ASSERT( 32 + 32 + 32 + 32 + 32 > TF_COND_LAST );
}


CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine if player should do loser animations.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoser( void )
{
	if ( !m_pOuter->IsAlive() )
		return false;

	if ( tf_always_loser.GetBool() )
		return true;

	if ( !m_pOuter->GetActiveWeapon() )
		return true;

	if ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		if ( iWinner != m_pOuter->GetTeamNumber() )
		{
			if ( IsDisguised() )
				return iWinner != GetDisguiseTeam() && GetTrueDisguiseTeam() != TF_TEAM_GLOBAL;

			return true;
		}
	}
	else if ( IsLoserStateStunned() )
		return true;

	return false;
}


void CTFPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}


void CTFPlayerShared::SetAirDash( bool bAirDash )
{
#ifdef GAME_DLL
	bool bPrevAirDash = m_bAirDash;
#endif

	m_bAirDash = bAirDash;

#ifdef GAME_DLL
	if ( m_bAirDash != bPrevAirDash )
	{
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DOUBLE_JUMP, m_bAirDash ? "started_jumping:1" : "started_jumping:0" );
	}
#endif
}


void CTFPlayerShared::IncrementAirDucks( void )
{
	m_nAirDucked++;
}


void CTFPlayerShared::ResetAirDucks( void )
{
	m_nAirDucked = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Checks if we have a parachute to deploy.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HasParachute( void ) const
{
	// No parachute, no deploy.
	int nHasParachute = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nHasParachute, parachute_attribute );
	if ( nHasParachute == 0 )
		return false;
	else if ( !InCond( TF_COND_PARACHUTE_DEPLOYED ) )
	{
		// If we haven't already pulled the parachute this jump, allow.
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we can deploy a parachute.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanParachute( void ) const
{
	if ( InCond(TF_COND_PARACHUTE_DEPLOYED) )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we are currently parachuting.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsParachuting( void ) const
{
	if ( InCond(TF_COND_PARACHUTE_ACTIVE) )
		return true;
	
	return false;
}


int CTFPlayerShared::GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom )
{
	const char *pszSequence = NULL;

	switch ( iDamageCustom )
	{
	case TF_DMG_CUSTOM_BACKSTAB:
		pszSequence = "primary_death_backstab";
		break;
	case TF_DMG_CUSTOM_DECAPITATION:
	case TF_DMG_CUSTOM_HEADSHOT:
		pszSequence = "primary_death_headshot";
		break;
	case TF_DMG_CUSTOM_BURNING:
		pszSequence = "primary_death_burning";
		break;
	}

	if ( pszSequence )
		return pAnim->LookupSequence( pszSequence );

	return -1;
}

void CTFPlayerShared::AddAttributeToPlayer( char const *szName, float flValue )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );
	if ( pDefinition )
		m_pOuter->GetAttributeList()->SetRuntimeAttributeValue( pDefinition, flValue );
}

void CTFPlayerShared::RemoveAttributeFromPlayer( char const *szName )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );
	m_pOuter->GetAttributeList()->RemoveAttribute( pDefinition );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Update the Civilian Buff.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateVIPBuff( void )
{
	CTFPlayer *pOuter = m_pOuter;
	if ( !pOuter )
		return;

	if ( m_pOuter->IsAlive() && (pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_CIVILIAN && !IsLoser()) )
	{
		if ( gpGlobals->curtime > m_flNextCivilianBuffCheckTime )
		{
			m_flNextCivilianBuffCheckTime = gpGlobals->curtime + 1.0f;
			PulseVIPBuff( pOuter );
		}
	}
	else
	{
		ResetVIPBuffPulse();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sends out the Civilian buff in a timely manner.
//-----------------------------------------------------------------------------
void CTFPlayerShared::PulseVIPBuff( CTFPlayer *pPulser )
{
	CBaseEntity *pEntity = NULL;
	Vector vecOrigin = pPulser->GetAbsOrigin();
	for ( CEntitySphereQuery sphere( vecOrigin, TF_CIVILIAN_BUFF_RADIUS ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( vecOrigin, &vecHitPoint );

		Vector vecDir = vecHitPoint - vecOrigin;
		if ( vecDir.LengthSqr() < (TF_CIVILIAN_BUFF_RADIUS * TF_CIVILIAN_BUFF_RADIUS) )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pEntity );
			if ( pPlayer )
			{
				// Bail out early if they're cloaked.
				if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
					continue;

				// If the player is a spy, we should check if he is disguised as the pulsers team.
				bool bValidTeam;
				if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
				{
					bValidTeam = pPlayer->m_Shared.DisguiseFoolsTeam( pPulser->GetTeamNumber() );
				}
				else
				{
					bValidTeam = pPlayer->InSameTeam( pPulser );
				}

				if ( pPlayer != pPulser && pPlayer->IsAlive() && bValidTeam && pPlayer->m_flPowerPlayTime <= gpGlobals->curtime )
				{
					int iAuraCond = -1;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iAuraCond, add_civ_aura_override );
					pPlayer->m_Shared.AddCond( iAuraCond != -1 ? (ETFCond)(iAuraCond + 1) : TF_COND_RESISTANCE_BUFF, TF_CIVILIAN_REBUFF_TIME, m_pOuter );
				}
			}
		}
	}
}
#endif


float CTFPlayerShared::GetCritMult( void )
{
	/*float flRemapCritMul = RemapValClamped( m_iCritMult, 0, 255, 1.0, TF_DAMAGE_CRITMOD_MAXMULT );
	#ifdef CLIENT_DLL
		Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
		#else
		Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
		#endif*/

	return RemapValClamped( m_iCritMult, 0, 255, 1.0f, TF_DAMAGE_CRITMOD_MAXMULT );
}

#ifdef GAME_DLL

void CTFPlayerShared::UpdateCritMult( void )
{
	const float flMinMult = 1.0f;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if ( m_DamageEvents.Count() == 0 )
	{
		m_iCritMult = RemapValClamped( flMinMult, flMinMult, flMaxMult, 0, 255 );
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	FOR_EACH_VEC_BACK( m_DamageEvents, i )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta > tf_damage_events_track_for.GetFloat() )
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove( i );
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if ( flDelta < TF_DAMAGE_CRITMOD_MINTIME )
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if ( flDelta > TF_DAMAGE_CRITMOD_MAXTIME )
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage;
	}

	float flMult = RemapValClamped( flTotalDamage, 0, TF_DAMAGE_CRITMOD_DAMAGE, flMinMult, flMaxMult );

	//Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped( flMult, flMinMult, flMaxMult, 0, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Record damage in player's last damage queue. Recorded values later used for random crit chance calculation.
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent( const CTakeDamageInfo &info, bool bKill )
{
	int c = m_DamageEvents.Count();
	if ( c >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event.
		m_DamageEvents.Remove( c - 1 );
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = info.GetDamage();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].bKill = bKill;

	// Don't count critical damage
	if ( (info.GetDamageType() & DMG_CRITICAL) )
	{
		m_DamageEvents[iIndex].flDamage -= info.GetDamageBonus();
	}
}


int	CTFPlayerShared::GetNumKillsInTime( float flTime )
{
	if ( tf_damage_events_track_for.GetFloat() < flTime )
	{
		Warning( "Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat() );
	}

	int iKills = 0;
	FOR_EACH_VEC_BACK( m_DamageEvents, i )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta < flTime )
		{
			if ( m_DamageEvents[i].bKill )
			{
				iKills++;
			}
		}
	}

	return iKills;
}

#endif

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT && tf_avoidteammates.GetBool()) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS_NOTSOLID ||
		collisionGroup == TFCOLLISION_GROUP_PUMPKINBOMBS )
	{
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
		{
			if ( !(contentsMask & CONTENTS_REDTEAM) )
				return false;

			break;
		}
		case TF_TEAM_BLUE:
		{
			if ( !(contentsMask & CONTENTS_BLUETEAM) )
				return false;

			break;
		}
		case TF_TEAM_GREEN:
		{
			if ( !(contentsMask & CONTENTS_GREENTEAM) )
				return false;

			break;
		}
		case TF_TEAM_YELLOW:
		{
			if ( !(contentsMask & CONTENTS_YELLOWTEAM) )
				return false;

			break;
		}
		}
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}


CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	return static_cast<CTFWeaponBase *>(GetActiveWeapon());
}


bool CTFPlayer::IsActiveTFWeapon( ETFWeaponID iWeaponID ) const
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
		return pWeapon->GetWeaponID() == iWeaponID;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}


bool CTFPlayer::CanPlayerMove() const
{
	return !TFGameRules()->InRoundRestart();
}

bool CTFPlayer::CanJump() const
{
	int nNoJump = 0;
	CALL_ATTRIB_HOOK_INT( nNoJump, no_jump );
	return nNoJump == 0;
}

bool CTFPlayer::CanDuck() const
{
	int nNoDuck = 0;
	CALL_ATTRIB_HOOK_INT( nNoDuck, no_duck );
	return nNoDuck == 0;
}


void CTFPlayer::TeamFortress_SetSpeed()
{
	// Main idea of the function:
	// 1) Special cases
	// 2) Get 'default' speed of the class, or weapon aiming
	// 3) Do buffs to speed
	// 4) Max it against various cases of "you have to be at least this fast"
	// 5) Do debuffs to speed
	// 6) Get 'default' speed of the disguise class and override with it if we are faster
	
	// This is done so big burst of speed like charge or haste don't get to be multiplied too far by speed buffs when they are big already
	// While also allowing those bursts to get slowed down by actions like getting tranqd.
	// Of course it's not a silver bullet and modders can still easily break it
	// Example: Have item that slows you on wearer. Have weapon that speeds you up when active. Get haste. You are slowed down and switching weapons doesn't speed you up.
	// But that's a cursed problem of trying to keep abstract numbers sane within a context by coming up with formulas and order, that's what you hire professional game designers for.
	// And we try to keep it mostly similar to TF2 buff/debuff handling, without soft caps or any bigger formula.
	// Tho maybe


	// 1) Special cases
	// Spectators can move while in Classic Observer mode.
	if ( IsObserver() )
	{
		SetMaxSpeed( GetObserverMode() == OBS_MODE_ROAMING ? GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed : 0.0f );
		return;
	}

	// Check for any reason why they can't move at all.
	int iPlayerClass = GetPlayerClass()->GetClassIndex();
	if ( iPlayerClass == TF_CLASS_UNDEFINED || GameRules()->InRoundRestart() )
	{
		SetMaxSpeed( 1.0f );
		return;
	}

	float maxfbspeed, maxfbspeedDefault;
	bool bHasTheFlag = HasTheFlag();
	int iSlowCarrier = tf2c_ctf_carry_slow.GetInt();
	float fFlaggedMult = tf2c_ctf_carry_slow_mult.GetFloat();
	bool bIsWeaponAimingSpeed = false;


	// 2) Get 'default' speed of the class, or weapon aiming
	if ( bHasTheFlag && iSlowCarrier == 1)
	{
		// Gun Mettle Spy is as fast as Medic
		if (tf2c_spy_gun_mettle.GetInt() > 0 && iPlayerClass == TF_CLASS_SPY)
			maxfbspeedDefault = maxfbspeed = GetPlayerClassData(TF_CLASS_MEDIC)->m_flFlagMaxSpeed;
		else
			maxfbspeedDefault = maxfbspeed = GetPlayerClassData(iPlayerClass)->m_flFlagMaxSpeed;
	}
	else
	{
		// Gun Mettle Spy is as fast as Medic
		if (tf2c_spy_gun_mettle.GetInt() > 0 && iPlayerClass == TF_CLASS_SPY)
			maxfbspeedDefault = maxfbspeed = GetPlayerClassData(TF_CLASS_MEDIC)->m_flMaxSpeed;
		else
			maxfbspeedDefault = maxfbspeed = GetPlayerClassData( iPlayerClass )->m_flMaxSpeed;
	}
	// Their weapon can modify our max speed. 
	CTFWeaponBase* pWeapon = GetActiveTFWeapon();
	if (pWeapon)
	{
		float flMaxSpeed = pWeapon->OwnerMaxSpeedModifier();
		if (flMaxSpeed > 0.0f)
		{
			bIsWeaponAimingSpeed = true;
			maxfbspeedDefault = maxfbspeed = flMaxSpeed; // no more min because modders wanted to be able to go faster
		}
	}


	// 3) Do buffs to speed
	// Don't be weirded out by my weird attribute handling algorithm, 
	// that's the only way to keep buff/debuff done by attribute in separate blocks
	// and keep their ability to either be a multiplier or additive
	// And yes, we end up calling same attributes again later in code
	float maxfbspeedOnAttribute = maxfbspeed;
	CALL_ATTRIB_HOOK_FLOAT(maxfbspeedOnAttribute, mult_player_movespeed );
	if (maxfbspeedOnAttribute > maxfbspeed)
		maxfbspeed = maxfbspeedOnAttribute;

	maxfbspeedOnAttribute = maxfbspeed;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetActiveTFWeapon(), maxfbspeedOnAttribute, mult_player_movespeed_active);
	if (maxfbspeedOnAttribute > maxfbspeed)
		maxfbspeed = maxfbspeedOnAttribute;

	if (m_Shared.InCond(TF_COND_STEALTHED))
	{
		maxfbspeedOnAttribute = maxfbspeed;
		CALL_ATTRIB_HOOK_FLOAT(maxfbspeedOnAttribute, mult_player_movespeed_cloaked);
		if (maxfbspeedOnAttribute > maxfbspeed)
			maxfbspeed = maxfbspeedOnAttribute;
	}

	if (bIsWeaponAimingSpeed)
	{
		maxfbspeedOnAttribute = maxfbspeed;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, maxfbspeedOnAttribute, mult_player_aiming_movespeed);
		if (maxfbspeedOnAttribute > maxfbspeed)
			maxfbspeed = maxfbspeedOnAttribute;
	}


	if (m_Shared.InCond(TF_COND_DISGUISED_AS_DISPENSER)) // unused, who knows
	{
		maxfbspeed *= 2.0f;
	}

	// if we're in bonus time because a team has won, give the winners 110% speed.
	if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN)
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		if (iWinner != TEAM_UNASSIGNED && iWinner == GetTeamNumber())
		{
			maxfbspeed *= 1.1f;
		}
	}


	// 4) Max it against various cases of "you have to be at least this fast"
	// !!! foxysen detonator
	if (m_Shared.InCond(TF_COND_SPEEDBOOST_DETONATOR))
	{
		maxfbspeed = max(tf2c_detonator_movespeed_const.GetFloat(), maxfbspeed);
	}

	if (m_Shared.InCond(TF_COND_SHIELD_CHARGE))
		maxfbspeed = max(720.0f, maxfbspeed);

	// This speed boost overrides rather than stacks, but only if it's faster than the previously calculated speeds
	if (m_Shared.InCond(TF_COND_SPEED_BOOST))
	{
		float prospectiveSpeed = maxfbspeedDefault 
#ifdef TF2C_BETA
		* tf2c_medicgl_movespeed_mult.GetFloat()
#endif
		* GetSpeedBoostMultiplier(); // Already accounts for Gun Mettle Spy

		maxfbspeed = max(prospectiveSpeed, maxfbspeed);
	}


	// 5) Do debuffs to speed
	maxfbspeedOnAttribute = maxfbspeed;
	CALL_ATTRIB_HOOK_FLOAT(maxfbspeedOnAttribute, mult_player_movespeed);
	if (maxfbspeedOnAttribute < maxfbspeed)
		maxfbspeed = maxfbspeedOnAttribute;

	maxfbspeedOnAttribute = maxfbspeed;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetActiveTFWeapon(), maxfbspeedOnAttribute, mult_player_movespeed_active);
	if (maxfbspeedOnAttribute < maxfbspeed)
		maxfbspeed = maxfbspeedOnAttribute;

	if (m_Shared.InCond(TF_COND_STEALTHED))
	{
		maxfbspeedOnAttribute = maxfbspeed;
		CALL_ATTRIB_HOOK_FLOAT(maxfbspeedOnAttribute, mult_player_movespeed_cloaked);
		if (maxfbspeedOnAttribute < maxfbspeed)
			maxfbspeed = maxfbspeedOnAttribute;
	}

	if (bIsWeaponAimingSpeed)
	{
		maxfbspeedOnAttribute = maxfbspeed;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, maxfbspeedOnAttribute, mult_player_aiming_movespeed);
		if (maxfbspeedOnAttribute < maxfbspeed)
			maxfbspeed = maxfbspeedOnAttribute;
	}

	// See if any flags are slowing them down
	//CCaptureFlag *pFlag = GetTheFlag();
	if (bHasTheFlag && iSlowCarrier == 2 && fFlaggedMult > 0.0f)
	{
		maxfbspeed *= fFlaggedMult;
	}

	// Engineer moves slower while a hauling a building.
	if ( iPlayerClass == TF_CLASS_ENGINEER && m_Shared.IsCarryingObject() )
	{
		maxfbspeed *= tf2c_building_gun_mettle.GetBool() ? 0.9f : 0.75f;
	}

	// Reduce our speed if we were tranquilized.
	if ( m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		maxfbspeed *= TF2C_TRANQ_MOVE_SPEED_FACTOR;
	}

	// Reduce our speed if we were hit by MIRV
	if (m_Shared.InCond(TF_COND_MIRV_SLOW))
	{
		maxfbspeed *= tf2c_mirv_impact_stun_secondary_movespeed.GetFloat();
	}

	// Reduce our speed to 75% when scared.
	bool bAllowSlowing = m_Shared.InCond(TF_COND_HALLOWEEN_BOMB_HEAD) ? false : true;
	if ( m_Shared.IsLoserStateStunned() && bAllowSlowing )
	{
		// Yikes is not as slow, terrible gotcha.
		if ( m_Shared.GetActiveStunInfo()->iStunFlags & TF_STUN_BY_TRIGGER )
		{
			maxfbspeed *= 0.75f;
		}
		else
		{
			maxfbspeed *= 0.5f;
		}
	}

	// if we're in bonus time because a team has won, give the losers 90% speed.
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		if ( iWinner != TEAM_UNASSIGNED && iWinner != GetTeamNumber())
		{
			maxfbspeed *= 0.9f;
		}
	}

	// 6) Get 'default' speed of the disguise class and override with it if we are faster
	// Slow us down if we're disguised as a slower class
	// unless we're cloaked and not "Spy-Walking"...
	if (!( tf2c_spywalk.GetBool() && ( IsSpywalkInverted() ? !(m_nButtons & IN_SPEED) : (m_nButtons & IN_SPEED) ) ) 
		&& ( m_Shared.IsDisguised() && !m_Shared.IsStealthed() ) )
	{
		if (tf2c_spy_gun_mettle.GetInt() > 0 && m_Shared.GetDisguiseClass() == TF_CLASS_SPY)
			maxfbspeed = GetPlayerClassData(TF_CLASS_MEDIC)->m_flMaxSpeed;
		else
			maxfbspeed = Min(GetPlayerClassData(m_Shared.GetDisguiseClass())->m_flMaxSpeed, maxfbspeed);
	}

#if 0 // unused, don't even know who did it
	if (m_Shared.IsStealthed())
	{
		float flMaxCloakedSpeed = tf_spy_max_cloaked_speed.GetFloat();
		if (maxfbspeed > flMaxCloakedSpeed)
		{
			maxfbspeed = flMaxCloakedSpeed;
		}
	}
#endif

	// Set the speed.
	SetMaxSpeed( maxfbspeed );
}

void CTFPlayer::TeamFortress_SetGravity()
{
	float flPlayerGravity = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT(flPlayerGravity, gravity_mod_wearer);
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(GetActiveTFWeapon(), flPlayerGravity, gravity_mod_active);
	SetGravity(flPlayerGravity);
}

float CTFPlayer::GetSpeedBoostMultiplier( void )
{
	int iClassIndex = this->GetPlayerClass()->GetClassIndex();
	if (tf2c_spy_gun_mettle.GetInt() > 0 && iClassIndex == TF_CLASS_SPY)
		return 1.328f; // same as Medic
	switch (iClassIndex)
	{
	case TF_CLASS_SCOUT:
		return 1.263f;
	case TF_CLASS_MEDIC:
		return 1.328f;
	case TF_CLASS_PYRO:
	case TF_CLASS_ENGINEER:
	case TF_CLASS_SNIPER:
	case TF_CLASS_SPY:
		return 1.35f;
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_CIVILIAN:
		return 1.375f;
	case TF_CLASS_HEAVYWEAPONS:
	case TF_CLASS_SOLDIER:
		return 1.4f;
	default:
		return 1.0f;
	}
}


bool CTFPlayer::HasItem( void ) const
{
	return m_hItem.Get() != NULL;
}


void CTFPlayer::SetItem( CTFItem *pItem )
{
	m_hItem = pItem;

#ifndef CLIENT_DLL
	/*if ( pItem )
	{
	if ( pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
	RemoveInvisibility();
	}
	}*/
#endif
}


CTFItem	*CTFPlayer::GetItem( void ) const
{
	return m_hItem;
}


CCaptureFlag *CTFPlayer::GetTheFlag( void ) const
{
	if ( HasTheFlag() )
		return assert_cast<CCaptureFlag *>(m_hItem.Get());

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player allowed to use a teleporter ?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag( void ) const
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Are we allowed to pick the flag up?
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAllowedToPickUpFlag( void ) const
{
	int bNotAllowedToPickUpFlag = 0;
	CALL_ATTRIB_HOOK_INT( bNotAllowedToPickUpFlag, cannot_pick_up_intelligence );
	if ( bNotAllowedToPickUpFlag > 0 )
		return false;

	// This Spy might have some other priorities aside from taking the flag.
	if ( m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_STEALTHED ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild( int iObjectType, int iObjectMode /*= 0*/ )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return CB_UNKNOWN_OBJECT;

#ifndef CLIENT_DLL
	CTFPlayerClass *pPlayerClass = GetPlayerClass();
	if ( m_Shared.IsCarryingObject() )
	{
		CBaseObject *pObject = m_Shared.GetCarriedObject();
		if ( pObject && pObject->GetType() == iObjectType && pObject->GetObjectMode() == iObjectMode )
			return CB_CAN_BUILD;

		Assert( 0 );
		return CB_CANNOT_BUILD;
	}

	if ( pPlayerClass && !pPlayerClass->CanBuildObject( iObjectType ) )
		return CB_CANNOT_BUILD;

	if ( GetObjectInfo( iObjectType )->m_AltModes.Count() != 0
		&& GetObjectInfo( iObjectType )->m_AltModes.Count() <= iObjectMode * 3 )
		return CB_CANNOT_BUILD;
	else if ( GetObjectInfo( iObjectType )->m_AltModes.Count() == 0 && iObjectMode != 0 )
		return CB_CANNOT_BUILD;
#endif

	// Check attributes for building replacements
	int iHasJumpPads = 0;
	CALL_ATTRIB_HOOK_INT( iHasJumpPads, set_teleporter_mode );
	if ( iHasJumpPads && iObjectType == OBJ_TELEPORTER )
		return CB_CANNOT_BUILD;
	else if ( !iHasJumpPads && iObjectType == OBJ_JUMPPAD )
		return CB_CANNOT_BUILD;

	int iHasMiniDispenser = 0;
	CALL_ATTRIB_HOOK_INT(iHasMiniDispenser, set_dispenser_mode);
	if (iHasMiniDispenser && iObjectType == OBJ_DISPENSER)
		return CB_CANNOT_BUILD;
	else if (!iHasMiniDispenser && iObjectType == OBJ_MINIDISPENSER)
		return CB_CANNOT_BUILD;

	int iHasFlameSentry = 0;
	CALL_ATTRIB_HOOK_INT(iHasFlameSentry, set_sentry_mode);
	if (iHasFlameSentry && iObjectType == OBJ_SENTRYGUN)
		return CB_CANNOT_BUILD;
	else if (!iHasFlameSentry && iObjectType == OBJ_FLAMESENTRY)
		return CB_CANNOT_BUILD;

	// Make sure we haven't hit maximum number.
	int iObjectCount = GetNumObjects( iObjectType, iObjectMode );
	if ( iObjectCount >= GetObjectInfo( iObjectType )->m_nMaxObjects && GetObjectInfo( iObjectType )->m_nMaxObjects != -1 )
		return CB_LIMIT_REACHED;

	// Find out how much the object should cost and 
	// make sure that we have enough resources.
	if ( GetBuildResources() < CTFPlayerShared::CalculateObjectCost( this, iObjectType ) )
		return CB_NEED_RESOURCES;

	return CB_CAN_BUILD;
}

ConVar tf_cheapobjects( "tf_cheapobjects", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Set to 1 and all objects will cost 0" );


int CTFPlayerShared::CalculateObjectCost( CTFPlayer *pBuilder, int iObjectType )
{
	if ( tf_cheapobjects.GetInt() )
		return 0;

	int iCost = GetObjectInfo( iObjectType )->m_Cost;

	if ( !pBuilder )
		return iCost;

	float flCostMod = 1.0f;
	switch ( iObjectType )
	{
		case OBJ_TELEPORTER:
		{
			// Meet your Match teleporters are 75 metal cheaper (50).
			if ( tf2c_building_gun_mettle.GetBool() )
				iCost -= 75;

			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flCostMod, mod_teleporter_cost );
			break;
		}
		case OBJ_MINIDISPENSER:
		case OBJ_DISPENSER:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flCostMod, mod_dispenser_cost );
			break;
		case OBJ_FLAMESENTRY:
		case OBJ_SENTRYGUN:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flCostMod, mod_sentrygun_cost );
			break;
		case OBJ_JUMPPAD:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flCostMod, mod_jumppad_cost );
			break;
	}
	iCost *= flCostMod;

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pBuilder, iCost, building_cost_reduction );

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has.
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects( int iObjectType, int iObjectMode /*= 0*/ )
{
	int iCount = 0;

	for ( int i = 0, c = GetObjectCount(); i < c; i++ )
	{
		CBaseObject	*pObject = GetObject( i );
		if ( pObject && pObject->GetType() == iObjectType && pObject->GetObjectMode() == iObjectMode && !pObject->IsBeingCarried() )
		{
			iCount++;
		}
	}

	return iCount;
}


void CTFPlayer::ItemPostFrame()
{
	for( int i = 0; i < GetNumWearables(); i++ )
	{
		CTFWearable *pWearable = (CTFWearable*)GetWearable(i);
		if( pWearable )
			pWearable->WearableFrame();
	}

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if ( m_hOffHandWeapon->IsPredicted() )
#endif
			{
				m_hOffHandWeapon->ItemPostFrame();
			}
		}
	}

	BaseClass::ItemPostFrame();
}


void CTFPlayer::SetOffHandWeapon( CTFWeaponBase *pWeapon )
{
	m_hOffHandWeapon = pWeapon;
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Deploy();
	}
}


void CTFPlayer::HolsterOffHandWeapon( void )
{
	if ( m_hOffHandWeapon.Get() )
	{
		// Set to NULL at the end of the holster?
		m_hOffHandWeapon->Holster();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!	
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(pOldWeapon);
	if ( !pWeapon->AllowsAutoSwitchTo() )
		return false;

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}


bool CTFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );

	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}


void CTFPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	// Don't play footstep sounds while crouched.
	if ( GetFlags() & FL_DUCKING )
		return;

#ifdef CLIENT_DLL
	// In third person, only make footstep sounds through animevents.
	if ( m_flStepSoundTime != -1.0f )
	{
		if ( ShouldDrawThisPlayer() )
			return;
	}
	else
	{
		m_flStepSoundTime = 0.0f;
	}
#endif

	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
}


void CTFPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();
	if ( (GetFlags() & FL_DUCKING) || GetMoveType() == MOVETYPE_LADDER )
	{
		*velwalk = flMaxSpeed * 0.25f;
		*velrun = flMaxSpeed * 0.3f;
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3f;
		*velrun = flMaxSpeed * 0.8f;
	}
}


void CTFPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( MaxSpeed(), 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( MaxSpeed(), 200, 450, 600, 400 );
		break;

	default:
		Assert( 0 );
		break;
	}

	if ( (GetFlags() & FL_DUCKING) || GetMoveType() == MOVETYPE_LADDER )
	{
		m_flStepSoundTime += 100;
	}
}


const char *CTFPlayer::GetOverrideStepSound( const char *pszBaseStepSoundName )
{
	// Inverted because base code flips it *before* calling this.
	int iArmorFootsteps = 0;
	CALL_ATTRIB_HOOK_INT( iArmorFootsteps, mod_armor_footsteps );
	if ( iArmorFootsteps )
		return !m_Local.m_nStepside ? "TFPlayer.ArmorStepLeft" : "TFPlayer.ArmorStepRight";

	return pszBaseStepSoundName;
}



void CTFPlayer::OnEmitFootstepSound( const CSoundParameters &params, const Vector &vecOrigin, float fVolume )
{
}


bool CTFPlayer::CanAttack( void )
{
	if ( !CanPlayerMove() )
		return false;

	if ( m_Shared.IsLoserStateStunned() || m_Shared.IsControlStunned() )
		return false;

	// Only regular cloak prevents us from firing.
	if ( m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime || m_Shared.InCond( TF_COND_STEALTHED ) )
	{
#ifdef CLIENT_DLL
		HintMessage( HINT_CANNOT_ATTACK_WHILE_CLOAKED, true, true );
#endif
		return false;
	}

	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
		return false;

	return true;
}


bool CTFPlayer::CanPickupBuilding( CBaseObject *pPickupObject )
{
	if ( !pPickupObject )
		return false;

	if ( !tf2c_building_hauling.GetBool() )
		return false;

	if ( pPickupObject->GetBuilder() != this )
		return false;

	if ( IsActiveTFWeapon( TF_WEAPON_BUILDER ) )
		return false;

	if ( IsActiveTFWeapon( TF_WEAPON_COILGUN ) )
		return false;

	if ( !CanAttack() )
		return false;

	if ( (!pPickupObject->InRemoteConstruction() || !pPickupObject->IsRedeploying()) && (pPickupObject->IsBuilding() || pPickupObject->IsUpgrading() || pPickupObject->IsRedeploying() || pPickupObject->IsDisabled()) )
		return false;

	int bCannotPickUpBuildings = 0;
	CALL_ATTRIB_HOOK_INT( bCannotPickUpBuildings, cannot_pick_up_buildings );
	if ( bCannotPickUpBuildings != 0 )
		return false;

	// Check it's within range.
	int nPickUpRangeSq = TF_BUILDING_PICKUP_RANGE * TF_BUILDING_PICKUP_RANGE;
	int iIncreasedRangeCost = 0;
	int nSqrDist = (EyePosition() - pPickupObject->GetAbsOrigin()).LengthSqr();

	// Extra range only works with primary weapon
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iIncreasedRangeCost, building_teleporting_pickup );
	if ( iIncreasedRangeCost != 0 )
	{
		// False on deadzone
		if ( nSqrDist > nPickUpRangeSq && nSqrDist < TF_BUILDING_RESCUE_MIN_RANGE_SQ )
			return false;

		if ( nSqrDist >= TF_BUILDING_RESCUE_MIN_RANGE_SQ && GetAmmoCount( TF_AMMO_METAL ) < iIncreasedRangeCost )
			return false;

		return true;
	}
	else if ( nSqrDist > nPickUpRangeSq )
		return false;

	return true;
}

#ifdef GAME_DLL

bool CTFPlayer::TryToPickupBuilding( void )
{
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );
	Vector vecSwingStart = Weapon_ShootPosition();

	int iRangedCost = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetActiveWeapon(), iRangedCost, building_teleporting_pickup );
	float flRange = (iRangedCost && GetAmmoCount( TF_AMMO_METAL ) >= iRangedCost) ? 5500.0f : 150.0f;
	Vector vecSwingEnd = vecSwingStart + vecForward * flRange;

	// See if we hit anything.
	trace_t trace;

	// only trace against objects
	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );

	if ( trace.m_pEnt &&
		trace.m_pEnt->IsBaseObject() &&
		trace.m_pEnt->GetTeamNumber() == GetTeamNumber() )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>(trace.m_pEnt);
		if ( CanPickupBuilding( pObject ) )
		{
			CTFWeaponBuilder *pBuilder = static_cast<CTFWeaponBuilder *>(GetEntityForLoadoutSlot( TF_LOADOUT_SLOT_BUILDING ));
			if ( pBuilder )
			{
				// Ranged pickup consumes metal.
				if ( iRangedCost && (trace.startpos - trace.endpos).IsLengthGreaterThan( 150.0f ) )
				{
					RemoveAmmo( iRangedCost, TF_AMMO_METAL );

					CPVSFilter filter( GetAbsOrigin() );

					Vector vecTeleportDir = pObject->WorldSpaceCenter() - WorldSpaceCenter();
					VectorNormalize( vecTeleportDir );
					QAngle angTeleportDir;
					VectorAngles( vecTeleportDir, angTeleportDir );

					const char *pszEffect = ConstructTeamParticle( "dxhr_sniper_rail_%s", GetTeamNumber() );
					TE_TFParticleEffect( filter, 0.0f, pszEffect, pObject->WorldSpaceCenter(), WorldSpaceCenter(), angTeleportDir );
					DispatchParticleEffect( pszEffect, pObject->WorldSpaceCenter(), WorldSpaceCenter(), angTeleportDir );

					pszEffect = ConstructTeamParticle( "teleported_%s", GetTeamNumber() );
					TE_TFParticleEffect( filter, 0.0f, pszEffect, pObject->GetAbsOrigin(), vec3_angle );

					pObject->EmitSound( "Building_Teleporter.Send" );
					EmitSound( "Building_Teleporter.Receive" );
				}

				pObject->MakeCarriedObject( this );

				pBuilder->SetSubType( pObject->GetType() );
				pBuilder->SetObjectMode( pObject->GetObjectMode() );

				SpeakConceptIfAllowed( MP_CONCEPT_PICKUP_BUILDING, pObject->GetResponseRulesModifier() );

				// Try to switch to this weapon.
				Weapon_Switch( pBuilder );

				m_flNextCarryTalkTime = gpGlobals->curtime + RandomFloat( 6.0f, 12.0f );

				return true;
			}
		}
	}
	return false;
}


void CTFPlayerShared::SetCarriedObject( CBaseObject *pObject )
{
	if ( pObject )
	{
		m_bCarryingObject = true;
		m_hCarriedObject = pObject;
	}
	else
	{
		m_bCarryingObject = false;
		m_hCarriedObject = NULL;
	}

	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->TeamFortress_SetGravity();
}


CBaseObject *CTFPlayerShared::GetCarriedObject( void )
{
	return m_hCarriedObject.Get();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
bool CTFPlayer::DoClassSpecialSkill( void )
{
	switch ( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_SPY:
	{
		if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
		{
			// Watches with custom abilities ie. Dead Ringer.
			CTFWeaponInvis *pInvisWatch = static_cast<CTFWeaponInvis *>(Weapon_OwnsThisID( TF_WEAPON_INVIS ));
			if ( pInvisWatch )
			{
				float flSmokeBombDuration = 0.0f;	// !!! foxysen speedwatch
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pInvisWatch, flSmokeBombDuration, smoke_bomb_on_full_cloak );
				bool bActivateSkill = pInvisWatch->ActivateInvisibilityWatch();
				if ( bActivateSkill && flSmokeBombDuration && m_Shared.GetSpyCloakMeter() >= 100.0f )
				{
					// just disconnect particles copy-paste
					DispatchParticleEffect( "player_poof", GetAbsOrigin(), vec3_angle );
					EmitSound( "TFPlayer.Disconnect" );

					m_Shared.AddCond( TF_COND_INVULNERABLE_SMOKE_BOMB, flSmokeBombDuration );
				}
				return true;
			}
		}
		break;
	}
	// Redundant, moved to CTFPipebombLauncher::ItemHolsterFrame()
	/*
	case TF_CLASS_DEMOMAN:
	{
		CTFWeaponBase *pPipebombLauncher = Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER );
		if ( pPipebombLauncher && pPipebombLauncher != GetActiveWeapon() )
		{
			pPipebombLauncher->SecondaryAttack();
			return true;
		}
		break;
	}
	*/
#ifdef GAME_DLL
	case TF_CLASS_ENGINEER:
		return TryToPickupBuilding();
#endif
	}

	for( int i = 0; i < GetNumWearables(); i++ )
	{
		CTFWearable *pWearable = (CTFWearable *)GetWearable(i);
		if( pWearable && !pWearable->IsDisguiseWearable() && pWearable->SecondaryAttack() )
			return true;
	}

	return false;
}


bool CTFPlayer::CanGoInvisible( void )
{
	/*if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
	//HintMessage( HINT_CANNOT_CLOAK_WITH_FLAG );
	#ifdef GAME_DLL
	CSingleUserRecipientFilter filter( this );
	TFGameRules()->SendHudNotification( filter, "#Hint_Cannot_Cloak_With_Flag", "ico_notify_flag_moving", GetTeamNumber() );
	#endif
	return false;
	}*/

	// Can't go invisible when scared.
	if ( m_Shared.IsLoserStateStunned() )
		return false;

	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
		return false;

	/*CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon && pWeapon->IsReloading() )
	return false;*/

	return true;
}


void CTFPlayerShared::UpdateCloakMeter( void )
{
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SPY, true ) )
		return;

	if ( InCond( TF_COND_STEALTHED ) )
	{
		// Default cloaking drains at a fixed rate.
		m_flCloakMeter -= gpGlobals->frametime * m_flCloakConsumeRate;

		if ( m_flCloakMeter <= 0.0f )
		{
			FadeInvis( 1.0f );
		}

#ifdef GAME_DLL
		if ( tf2c_spy_gun_mettle.GetInt() == 1 )
		{
			float flReduction = gpGlobals->frametime * 0.75f;
			for ( int i = 0; g_eDebuffConditions[i] != TF_COND_LAST; i++ )
			{
				if ( InCond( g_eDebuffConditions[i] ) )
				{
					if ( m_flCondExpireTimeLeft[g_eDebuffConditions[i]] != PERMANENT_CONDITION )
					{
						m_flCondExpireTimeLeft.Set( g_eDebuffConditions[i], Max<float>( m_flCondExpireTimeLeft[g_eDebuffConditions[i]] - flReduction, 0 ) );
					}

					if ( g_eDebuffConditions[i] == TF_COND_BURNING )
					{
						// Reduce the duration of this burn.
						m_flFlameRemoveTime -= flReduction;
					}
					else if ( g_eDebuffConditions[i] == TF_COND_BLEEDING )
					{
						// Reduce the duration of all bleeding stacks.
						FOR_EACH_VEC( m_PlayerBleeds, i )
						{
							m_PlayerBleeds[i].flBleedingRemoveTime -= flReduction;
						}
					}
				}
			}
		}
#endif
	}
	else
	{
		m_flCloakMeter = Min<float>( 100.0f, m_flCloakMeter + gpGlobals->frametime * m_flCloakRegenRate );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTFPlayer::GetClassEyeHeight( void )
{
	int iClass = m_PlayerClass.GetClassIndex();
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_CLASS_COUNT )
		return VEC_VIEW_SCALED( this );

	return g_TFClassViewVectors[iClass] * GetModelScale();
}

//-----------------------------------------------------------------------------
// Purpose: Can this player change class through class menu?
//-----------------------------------------------------------------------------
bool CTFPlayer::CanShowClassMenu( void )
{
	// Must be on a valid team.
	if ( TFGameRules()->IsInArenaQueueMode() )
	{
		if ( m_bArenaSpectator )
			return false;
	}
	else if ( GetTeamNumber() < FIRST_GAME_TEAM )
		return false;

	// Don't allow opening class menu while Arena round is running and we're out in the field.
	if ( TFGameRules()->IsInArenaMode() && TFGameRules()->State_Get() == GR_STATE_STALEMATE && IsAlive() )
		return false;

	// Losers can't change class during bonus time.
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
		GetTeamNumber() != TFGameRules()->GetWinningTeam() &&
		IsAlive() )
		return false;

	// VIP can't switch away from his class.
	if ( IsVIP() )
		return false;

	if ( GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_CLASSES ) )
		return false;

	return true;
}


float CTFPlayer::MedicGetChargeLevel( void )
{
	// Spy has a fake uber level.
	if ( IsPlayerClass( TF_CLASS_SPY, true ) && m_Shared.IsDisguised() )
		return m_Shared.m_flDisguiseChargeLevel;

	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		ITFHealingWeapon *pMedigun = GetMedigun();
		if ( pMedigun )
			return pMedigun->GetChargeLevel();

		return 0.0f;
	}

	return 0.0f;
}


CBaseEntity *CTFPlayer::MedicGetHealTarget( void )
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon && pWeapon->IsWeapon( TF_WEAPON_MEDIGUN ) )
		return static_cast<CWeaponMedigun *>(pWeapon)->GetHealTarget();

	return NULL;
}


ITFHealingWeapon *CTFPlayer::GetMedigun( void ) const
{
	CTFWeaponBase *pWeapon = Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
	if ( pWeapon )
		return static_cast<CWeaponMedigun *>(pWeapon);
#ifdef TF2C_BETA
	pWeapon = Weapon_OwnsThisID( TF_WEAPON_HEALLAUNCHER );
	if ( pWeapon )
		return static_cast<CTFHealLauncher *>(pWeapon);
#endif
	pWeapon = Weapon_OwnsThisID( TF_WEAPON_PAINTBALLRIFLE );
	if ( pWeapon )
		return static_cast<CTFPaintballRifle *>(pWeapon);

	return NULL;
}


CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( ETFWeaponID iWeaponID ) const
{
	for ( int i = 0, c = WeaponCount(); i < c; i++ )
	{
		CTFWeaponBase *pWeapon = GetTFWeapon( i );
		if ( pWeapon && pWeapon->GetWeaponID() == iWeaponID )
			return pWeapon;
	}

	return NULL;
}


CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType( ETFWeaponType iType ) const
{
	for ( int i = 0, c = WeaponCount(); i < c; i++ )
	{
		CTFWeaponBase *pWeapon = GetTFWeapon( i );
		if ( pWeapon && pWeapon->GetTFWpnData().m_iWeaponType == iType )
			return pWeapon;
}

	return NULL;
}

CTFWearable *CTFPlayer::Wearable_OwnsThisID( int iWearableID ) const
{
	for ( int i = 0, c = GetNumWearables(); i < c; i++ )
	{
		CEconWearable *pWearable = GetWearable( i );
		if( pWearable && pWearable->GetItemID() == iWearableID )
			return (CTFWearable*)pWearable;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose: Use this when checking if we can damage this player or something similar.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsEnemy( const CBaseEntity *pEntity ) const
{
#if 0
	// Spectators are nobody's enemy.
	if ( m_pOuter->GetTeamNumber() < FIRST_GAME_TEAM || pEntity->GetTeamNumber() < FIRST_GAME_TEAM )
		return false;
#endif

	return pEntity->GetTeamNumber() != GetTeamNumber();
}


bool CTFPlayer::IsVIP( void ) const
{
	if ( !TFGameRules()->IsVIPMode() )
		return false;

#ifndef CLIENT_DLL
	CTFTeam *pTFTeam = GetTFTeam();
#else
	C_TFTeam *pTFTeam = GetTFTeam();
#endif
	if ( !pTFTeam )
		return false;

	return pTFTeam->GetVIP() == this && pTFTeam->IsEscorting();
}

//-----------------------------------------------------------------------------
// Purpose: Get the weapon or wearable entity for a given loadout slot
//-----------------------------------------------------------------------------
CEconEntity *CTFPlayer::GetEntityForLoadoutSlot( ETFLoadoutSlot iSlot ) const
{
	// Only check for weapons if we're not querying a cosmetic slot.
	if ( iSlot < TF_LOADOUT_SLOT_HAT )
	{
		CTFWeaponBase *pWeapon = GetWeaponForLoadoutSlot( iSlot );
		if ( pWeapon )
			return pWeapon;
	}

	// Wearables can potentially be in weapon or cosmetic slots.
	CEconWearable *pWearable = GetWearableForLoadoutSlot( iSlot );
	if ( pWearable )
		return pWearable;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the wearable entity for a given loadout slot (ignore weapons)
//-----------------------------------------------------------------------------
CEconWearable *CTFPlayer::GetWearableForLoadoutSlot( ETFLoadoutSlot iSlot ) const
{
	int iClass = m_PlayerClass.GetClassIndex();

	for ( int i = 0, c = GetNumWearables(); i < c; i++ )
	{
		CEconWearable *pWearable = GetWearable( i );
		if ( pWearable && pWearable->GetItem()->GetLoadoutSlot( iClass ) == iSlot )
			return pWearable;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the weapon entity for a given loadout slot (ignore wearables)
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetWeaponForLoadoutSlot( ETFLoadoutSlot iSlot ) const
{
	int iClass = m_PlayerClass.GetClassIndex();

	for ( int i = 0, c = WeaponCount(); i < c; i++ )
	{
		CTFWeaponBase *pWeapon = GetTFWeapon( i );
		if ( pWeapon && pWeapon->GetItem()->GetLoadoutSlot( iClass ) == iSlot )
			return pWeapon;
	}

	return NULL;
}


void CTFPlayer::RemoveAmmo( int iCount, int iAmmoIndex )
{
	if ( HasInfiniteAmmo() )
		return;

	BaseClass::RemoveAmmo( iCount, iAmmoIndex );
}


int CTFPlayer::GetAmmoCount( int iAmmoIndex ) const
{
	if ( HasInfiniteAmmo() )
		return 999;

	return BaseClass::GetAmmoCount( iAmmoIndex );
}


int CTFPlayer::GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip /*= false*/, int iClassIndex /*= TF_CLASS_UNDEFINED*/ ) const
{
	int iMaxAmmo = (iClassIndex == TF_CLASS_UNDEFINED) ? m_PlayerClass.GetData()->m_aAmmoMax[iAmmoIndex] : GetPlayerClassData( iClassIndex )->m_aAmmoMax[iAmmoIndex];
	int iMissingClip = 0;

	// If we have a weapon that overrides max ammo, use its value.
	// BUG: If player has multiple weapons using same ammo type then only the first one's value is used.
	for ( int i = 0, c = WeaponCount(); i < c; i++ )
	{
		CTFWeaponBase *pWeapon = GetTFWeapon( i );
		if ( pWeapon && pWeapon->GetPrimaryAmmoType() == iAmmoIndex )
		{
			int iCustomMaxAmmo = pWeapon->GetMaxAmmo();
			if ( iCustomMaxAmmo )
			{
				iMaxAmmo = iCustomMaxAmmo;
			}

			// Allow getting ammo above maximum if the clip is not full ala L4D.
			if ( bAddMissingClip && pWeapon->UsesClipsForAmmo1() )
			{
				iMissingClip = pWeapon->GetMaxClip1() - pWeapon->Clip1();
			}

			break;
		}
	}

	switch ( iAmmoIndex )
	{
	case TF_AMMO_PRIMARY:
		CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_primary );
		break;
	case TF_AMMO_SECONDARY:
		CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_secondary );
		break;
	case TF_AMMO_METAL:
		CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_metal );
		break;
	case TF_AMMO_GRENADES1:
		CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_grenades1 );
		break;
	case TF_AMMO_GRENADES2:
		CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_grenades2 );
		break;
	}

	return iMaxAmmo + iMissingClip;
}


bool CTFPlayer::HasInfiniteAmmo( void ) const
{
	return tf2c_infinite_ammo.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Get stored crits
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetStoredCrits()
{
	return m_iStoredCrits;
}

//-----------------------------------------------------------------------------
// Purpose: Get capacity of stored crits
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetStoredCritsCapacity()
{
	return TF2C_MAX_STORED_CRITS;
}

//-----------------------------------------------------------------------------
// Purpose: Remove stored crits. bMissed stands on whether you did it by missing, thus play the lose sound.
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveStoredCrits( int iAmount, bool bMissed )
{
	if ( iAmount <= 0 ) return;
	int iNewStoredCrits = Max( 0, m_iStoredCrits - iAmount );
#ifdef GAME_DLL
	if ( bMissed && iNewStoredCrits < m_iStoredCrits )
	{
		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.LoseStoredCrit" );
	}
#endif
	m_iStoredCrits = iNewStoredCrits;
}

//-----------------------------------------------------------------------------
// Purpose: Resets to zero
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetStoredCrits()
{
	m_iStoredCrits = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Gain stored crits
//-----------------------------------------------------------------------------
void CTFPlayerShared::GainStoredCrits( int iAmount )
{
	if ( iAmount <= 0 ) return;
	int iNewStoredCrits = Min( GetStoredCritsCapacity(), m_iStoredCrits + iAmount );
#ifdef GAME_DLL
	if ( iNewStoredCrits > m_iStoredCrits )
	{
		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.GainStoredCrit" );
	}
#endif
	m_iStoredCrits = iNewStoredCrits;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the configured fraction of the bounding box is considered a headshot for a given class index.
//-----------------------------------------------------------------------------
float CTFPlayerShared::HeadshotFractionForClass( int iClassIndex )
{
	float fHeadshotFraction = 1.0f;
	switch ( iClassIndex )
	{
	case TF_CLASS_SCOUT:
		fHeadshotFraction = tf2c_projectile_fraction_scout.GetFloat();
		break;
	case TF_CLASS_SNIPER:
		fHeadshotFraction = tf2c_projectile_fraction_sniper.GetFloat();
		break;
	case TF_CLASS_SOLDIER:
		fHeadshotFraction = tf2c_projectile_fraction_soldier.GetFloat();
		break;
	case TF_CLASS_DEMOMAN:
		fHeadshotFraction = tf2c_projectile_fraction_demo.GetFloat();
		break;
	case TF_CLASS_MEDIC:
		fHeadshotFraction = tf2c_projectile_fraction_medic.GetFloat();
		break;
	case TF_CLASS_HEAVYWEAPONS:
		fHeadshotFraction = tf2c_projectile_fraction_heavy.GetFloat();
		break;
	case TF_CLASS_PYRO:
		fHeadshotFraction = tf2c_projectile_fraction_pyro.GetFloat();
		break;
	case TF_CLASS_SPY:
		fHeadshotFraction = tf2c_projectile_fraction_spy.GetFloat();
		break;
	case TF_CLASS_ENGINEER:
		fHeadshotFraction = tf2c_projectile_fraction_engie.GetFloat();
		break;
	case TF_CLASS_CIVILIAN:
		fHeadshotFraction = tf2c_projectile_fraction_civ.GetFloat();
		break;
	default:
		Warning( "Player class was invalid (when getting headshot fraction for class).\n" );
	}

	return fHeadshotFraction;
}

float CTFPlayerShared::ProjetileHurtCylinderForClass( int iClassIndex )
{
	// From right angles, the hittable area of the AABB player hull is 32 hammer units wide.
	// From 45 degree angles, the hittable area of the AABB is 32 * sqrt(2) wide (a hypotenuse, in a way)
	// So it makes that to achieve a consistent area that is irresepective of yaw angle, we must average these two values:
	const float sqrt2 = 1.41421356;
	const float defaultWidth = 32.0f;
	const float cylinderRadius = (defaultWidth + (defaultWidth + sqrt2)) / 2.0f;
	// All have the same (for now)
	return cylinderRadius;
}

//-----------------------------------------------------------------------------
// Purpose: Get the player from whose perspective we're viewing the world from.
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::GetObservedPlayer( bool bFirstPerson )
{
	if ( GetObserverMode() == OBS_MODE_IN_EYE || (!bFirstPerson && GetObserverMode() == OBS_MODE_CHASE) )
	{
		CTFPlayer *pTarget = ToTFPlayer( GetObserverTarget() );
		if ( pTarget )
			return pTarget;
	}

	return this;
}


Vector CTFPlayer::Weapon_ShootPosition( void )
{
	// If we're in over the shoulder third person fire from the side so it matches up with camera.
	if ( m_nButtons & IN_THIRDPERSON )
	{
		Vector vecForward, vecRight, vecUp, vecOffset;
		AngleVectors( EyeAngles(), &vecForward, &vecRight, &vecUp );
		vecOffset.Init( 0.0f, TF_CAMERA_DIST_RIGHT, TF_CAMERA_DIST_UP );

		if ( ShouldFlipViewModel() )
		{
			vecOffset.y *= -1.0f;
		}

		return EyePosition() + vecOffset.y * vecRight + vecOffset.z * vecUp;
	}

	return BaseClass::Weapon_ShootPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Sets player's eye angles - not camera angles!
//-----------------------------------------------------------------------------
void CTFPlayer::SetEyeAngles( const QAngle &angles )
{
#ifdef GAME_DLL
	if ( pl.fixangle != FIXANGLE_NONE )
		return;
#endif

	pl.v_angle = m_angEyeAngles = angles;
}

//-----------------------------------------------------------------------------
// Purpose: Like CBaseEntity::HealthFraction, but uses GetMaxBuffedHealth instead of GetMaxHealth.
//-----------------------------------------------------------------------------
float CTFPlayer::HealthFractionBuffed() const
{
	int iMaxBuffedHealth = m_Shared.GetMaxBuffedHealth();
	if ( iMaxBuffedHealth == 0 )
		return 1.0f;

	return clamp( (float)GetHealth() / (float)iMaxBuffedHealth, 0.0f, 1.0f );
}

bool CTraceFilterObject::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( pHandleEntity == m_pIgnoreOther )
		return false;

	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( pEntity )
	{
		const CBaseEntity *pPassEntity = EntityFromEntityHandle( GetPassEntity() );
		if ( pEntity->IsBaseObject() )
		{
			CBaseObject *pObject = assert_cast<CBaseObject *>(pEntity);
			if ( pObject->GetBuilder() == pPassEntity )
			{
#ifdef GAME_DLL
				// Allow TFBots to pass through their own objects, with minor exceptions.
				const CTFBot *pTFBot = ToTFBot( pPassEntity );
				if ( pTFBot )
				{
					if ( pObject->GetType() == OBJ_TELEPORTER || pObject->GetType() == OBJ_JUMPPAD )
					{
						// Allow teleporter pass-thru in MvM mode only.
						if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
							return false;
					}
				}
#endif

				return true;
			}
		}
#ifdef GAME_DLL
		else if ( pEntity->IsPlayer() )
		{
			// In MvM mode, bots on certain missions are allowed to move through objects
			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			{
				CTFBot *pTFBot = ToTFBot( pEntity );
				if ( pTFBot )
				{
					if ( pTFBot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) )
						return false;

					if ( pTFBot->HasMission( CTFBot::MISSION_REPROGRAMMED ) )
						return false;
				}
			}
		}
		else if ( ToNextBot( pEntity ) ) // NextBotCombatCharacters
		{
			if ( ToNextBot( pEntity )->GetLocomotionInterface()->ShouldCollideWith( pPassEntity ) )
				return false;
		}
#endif
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Store remaining afterburn info in vector from killed players. To be used for Baleful Beacon attribute.
//-----------------------------------------------------------------------------
void CTFPlayerShared::StoreNewRemainingAfterburn( float flRemoveTime, float flNextTick )
{
	// New deathburn source.
	deathburn_struct_t deathburninfo =
	{
		flNextTick,		// frame for next tick for supposed afterburn damage
		flRemoveTime	// frame for supposed afterburn removal
	};
	m_PlayerDeathBurns.AddToTail( deathburninfo );
}

//-----------------------------------------------------------------------------
// Purpose: Remove remaining afterburn info in vector from killed players. To be used for Baleful Beacon attribute.
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllRemainingAfterburn()
{
	m_PlayerDeathBurns.Purge();
}
#endif
