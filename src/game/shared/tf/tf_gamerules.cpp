//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "time.h"
#include "viewport_panel_names.h"
#include "tf_announcer.h"
#ifdef CLIENT_DLL
#include <game/client/iviewport.h>
#include "c_tf_player.h"
#include "c_tf_objective_resource.h"
#include "voice_status.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "c_tf_projectile_arrow.h"
#include "tf_autorp.h"
#define CTeam C_Team
#else
#include "mapentities.h"
#include "gameinterface.h"
#include "serverbenchmark_base.h"
#include "basemultiplayerplayer.h"
#include "voice_gamemgr.h"
#include "items.h"
#include "team.h"
#include "tf_bot_temp.h"
#include "tf_player.h"
#include "tf_team.h"
#include "player_resource.h"
#include "entity_tfstart.h"
#include "filesystem.h"
#include "tf_obj.h"
#include "tf_objective_resource.h"
#include "tf_player_resource.h"
#include "playerclass_info_parse.h"
#include "tf_control_point_master.h"
#include "coordsize.h"
#include "entity_healthkit.h"
#include "tf_gamestats.h"
#include "entity_capture_flag.h"
#include "tf_player_resource.h"
#include "tf_obj_sentrygun.h"
#include "tier0/icommandline.h"
#include "activitylist.h"
#include "AI_ResponseSystem.h"
#include "hl2orange.spa.h"
#include "hltvdirector.h"
#include "tf_train_watcher.h"
#include "vote_controller.h"
#include "tf_voteissues.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_weaponbase_nail.h"
#include "eventqueue.h"
#include "nav_mesh.h"
#include "bot/tf_bot_manager.h"
#include "bot/map_entities/tf_bot_roster.h"
#include "game.h"
#include "pathtrack.h"
#include "entity_ammopack.h"
#include <tier1/netadr.h>
#include "effect_dispatch_data.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "props.h"
#include "func_worker_zone.h"
#include "tf_weapon_paintballrifle.h"
#include "tf_weapon_riot.h"

#ifdef TF2C_BETA
#include "tf_weapon_heallauncher.h"
#endif

#include "achievements_tf.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
CUtlVector<CHandle<CTeamControlPointMaster>>		g_hControlPointMasters;
static CTFGameRulesProxy *g_pGameRulesProxy;

extern bool IsInCommentaryMode( void );

#if defined( REPLAY_ENABLED )
extern IReplaySystem *g_pReplay;
#endif // REPLAY_ENABLED
#endif

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

static int g_TauntCamAchievements[TF_CLASS_COUNT_ALL] =
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
};

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;
extern ConVar mp_chattime;
extern ConVar tf_arena_max_streak;
extern ConVar sv_alltalk;
extern ConVar tv_delaymapchange;
extern ConVar sv_vote_issue_nextlevel_allowed;
extern ConVar sv_vote_issue_nextlevel_choicesmode;

ConVar mp_capstyle( "mp_capstyle", "1", FCVAR_REPLICATED, "Sets the style of capture points used. 0 = Fixed players required to cap. 1 = More players cap faster, but longer cap times." );
ConVar mp_blockstyle( "mp_blockstyle", "1", FCVAR_REPLICATED, "Sets the style of capture point blocking used. 0 = Blocks break captures completely. 1 = Blocks only pause captures." );
ConVar mp_respawnwavetime( "mp_respawnwavetime", "10.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Time between respawn waves." );
ConVar mp_capdeteriorate_time( "mp_capdeteriorate_time", "90.0", FCVAR_REPLICATED, "Time it takes for a full capture point to deteriorate." );
ConVar mp_tournament( "mp_tournament", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar mp_highlander( "mp_highlander", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow only 1 of each player class type." );

//Arena Mode
ConVar tf_arena_preround_time( "tf_arena_preround_time", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of the Pre-Round time", true, 5.0, true, 15.0 );
ConVar tf_arena_round_time( "tf_arena_round_time", "180", FCVAR_NOTIFY | FCVAR_REPLICATED, "Time (in seconds) before an Arena round ends in stalemate. 0 to disable." );
ConVar tf_arena_max_streak( "tf_arena_max_streak", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Teams will be scrambled if one team reaches this streak. Requires tf_arena_use_queue" );
ConVar tf_arena_use_queue( "tf_arena_use_queue", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enables the spectator queue system for Arena." );

ConVar tf_arena_first_blood( "tf_arena_first_blood", "1", FCVAR_REPLICATED, "Rewards the first player to get a kill each round." );
ConVar tf_arena_override_cap_enable_time( "tf_arena_override_cap_enable_time", "-1", FCVAR_REPLICATED, "Overrides the time (in seconds) it takes for the capture point to become enable, -1 uses the level designer specified time." );
ConVar tf_arena_override_team_size( "tf_arena_override_team_size", "64", FCVAR_REPLICATED, "Overrides the maximum team size in arena mode. Set to zero to keep the default behavior of 1/3 maxplayers." );
ConVar tf2c_arena_swap_teams("tf2c_arena_swap_teams", "0", FCVAR_REPLICATED, "Swap teams every round on arena mode");
ConVar tf2c_allow_maptime_reset("tf2c_allow_maptime_reset", "1", FCVAR_REPLICATED, "If disabled, map time will not reset due to mp_restartgame, scramble, or waiting for players");

ConVar mp_teams_unbalance_limit( "mp_teams_unbalance_limit", "1", FCVAR_REPLICATED,
	"Teams are unbalanced when one team has this many more players than the other team. (0 disables check)",
	true, 0,	// min value
	true, 30	// max value
	);

ConVar mp_maxrounds( "mp_maxrounds", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "max number of rounds to play before server changes maps", true, 0, false, 0 );
ConVar mp_winlimit( "mp_winlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Max score one team can reach before server changes maps", true, 0, false, 0 );
ConVar mp_disable_respawn_times( "mp_disable_respawn_times", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar mp_bonusroundtime( "mp_bonusroundtime", "10", FCVAR_REPLICATED, "Time after round win until round restarts", true, 5, true, 15 );
ConVar mp_bonusroundtime_final( "mp_bonusroundtime_final", "15", FCVAR_REPLICATED, "Time after final round ends until round restarts", true, 5, true, 300 );
ConVar mp_stalemate_meleeonly( "mp_stalemate_meleeonly", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Restrict everyone to melee weapons only while in Sudden Death." );
ConVar mp_forceautoteam( "mp_forceautoteam", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Automatically assign players to teams when joining." );

#if defined( _DEBUG ) || defined( STAGING_ONLY )
ConVar mp_developer( "mp_developer", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_NOTIFY, "1: basic conveniences (instant respawn and class change, etc).  2: add combat conveniences (infinite ammo, buddha, etc)" );
#endif // _DEBUG || STAGING_ONLY

ConVar tf2c_ctf_attacker_bonus( "tf2c_ctf_attacker_bonus", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "CTF players respawn faster when far away from their flag base" );
ConVar tf2c_ctf_attacker_bonus_dist( "tf2c_ctf_attacker_bonus_dist", "2048.0", FCVAR_REPLICATED, "Minimum distance from own flag base to qualify for fast respawn" );

// VIP (Very Important Pocket)
ConVar tf2c_vip_round_limit( "tf2c_vip_round_limit", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Teams will be scrambled if one team reaches this streak" );

ConVar tf2c_vip_bonus_time( "tf2c_vip_bonus_time", "5.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of crit time for the VIP killer" );
ConVar tf2c_vip_boost_time( "tf2c_vip_boost_time", "8.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of boost time for the VIP's target" );
ConVar tf2c_vip_boost_cooldown( "tf2c_vip_boost_cooldown", "16.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of boost cooldown for the VIP himself" );
ConVar tf2c_vip_abilities( "tf2c_vip_abilities", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Abilities the VIP can use (0 = None, 1 = Aura (Armor & Heal), 2 = Boost (Melee), 3 = Both)" );
ConVar tf2c_vip_persist("tf2c_vip_persist", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "If enabled, the game does not change VIP across rounds");

ConVar tf2c_damagescale_stun("tf2c_damagescale_stun", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Damagescale for a stunned character");
ConVar tf2c_bullets_pass_teammates("tf2c_bullets_pass_teammates", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "If set to 1, all ranged hitscan will pass through teammates. If set to 0 - only sniper rifle weapon type shots.");

#ifdef GAME_DLL
extern ConVar tf_damage_disablespread;
extern ConVar tf_damage_range;
extern ConVar tf_debug_damage;
extern ConVar tf2c_vip_armor;
extern ConVar tf2c_vip_eagle_armor;
extern ConVar tf2c_spy_gun_mettle;

ConVar tf_stealth_damage_reduction( "tf_stealth_damage_reduction", "0.8", FCVAR_CHEAT );

ConVar tf_weapon_criticals_distance_falloff( "tf_weapon_criticals_distance_falloff", "0", FCVAR_CHEAT, "Critical weapon damage will take distance into account." );
ConVar tf_weapon_minicrits_distance_falloff( "tf_weapon_minicrits_distance_falloff", "0", FCVAR_CHEAT, "Mini-crit weapon damage will take distance into account." );

ConVar tf2c_log_item_details( "tf2c_log_item_details", "1", FCVAR_GAMEDLL, "On player death, give item and score details to log & send event" );
ConVar tf2c_log_achievements("tf2c_log_achievements", "1", FCVAR_GAMEDLL, "Log earned achievements.");

ConVar mp_showroundtransitions( "mp_showroundtransitions", "0", FCVAR_CHEAT, "Show gamestate round transitions." );
ConVar mp_enableroundwaittime( "mp_enableroundwaittime", "1", FCVAR_REPLICATED, "Enable timers to wait between rounds." );
ConVar mp_showcleanedupents( "mp_showcleanedupents", "0", FCVAR_CHEAT, "Show entities that are removed on round respawn." );
ConVar mp_restartround( "mp_restartround", "0", FCVAR_GAMEDLL, "If non-zero, the current round will restart in the specified number of seconds" );

ConVar mp_stalemate_timelimit( "mp_stalemate_timelimit", "240", FCVAR_REPLICATED, "Timelimit (in seconds) of the stalemate round." );
ConVar mp_autoteambalance( "mp_autoteambalance", "1", FCVAR_NOTIFY );

ConVar mp_stalemate_enable( "mp_stalemate_enable", "1", FCVAR_NOTIFY, "Enable/Disable stalemate mode." );
ConVar mp_match_end_at_timelimit( "mp_match_end_at_timelimit", "0", FCVAR_NOTIFY, "Allow the match to end when mp_timelimit hits instead of waiting for the end of the current round." );

ConVar mp_holiday_nogifts( "mp_holiday_nogifts", "0", FCVAR_NOTIFY, "Set to 1 to prevent holiday gifts from spawning when players are killed." );

ConVar tf2c_defenceboost_resistance( "tf2c_defenceboost_resistance", "0.65", FCVAR_NOTIFY, "The proportion of damage blocked by the DEFENCEBOOST cond" );
ConVar tf2c_defenceboost_sentry_resistance( "tf2c_defenceboost_sentry_resistance", "0.5", FCVAR_NOTIFY, "The proportion of sentry damage blocked by the DEFENCEBOOST cond" );

ConVar tf2c_tranq_ranged_damage_factor("tf2c_tranq_ranged_damage_factor", "0.7", FCVAR_NOTIFY, "The proportion of ranged gun damage blocked by the TRANQUILIZED cond");

	
void cc_SwitchTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		if ( TFGameRules() )
		{
			TFGameRules()->SetSwitchTeams( true );
			mp_restartgame.SetValue( 5 );
			TFGameRules()->ShouldResetScores( false, false );
			TFGameRules()->ShouldResetRoundsPlayed( false );
		}
	}
}

static ConCommand mp_switchteams( "mp_switchteams", cc_SwitchTeams, "Switch teams and restart the game" );

	
void cc_ScrambleTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		if ( TFGameRules() )
		{
			TFGameRules()->SetScrambleTeams( true );
			mp_restartgame.SetValue( 5 );
			TFGameRules()->ShouldResetScores( true, false );

			if ( args.ArgC() == 2 )
			{
				// Don't reset the roundsplayed when mp_scrambleteams 2 is passed
				if ( atoi( args[1] ) == 2 )
				{
					TFGameRules()->ShouldResetRoundsPlayed( false );
				}
			}
		}
	}
}

static ConCommand mp_scrambleteams( "mp_scrambleteams", cc_ScrambleTeams, "Scramble the teams and restart the game" );
ConVar mp_scrambleteams_auto( "mp_scrambleteams_auto", "1", FCVAR_NOTIFY, "Server will automatically scramble the teams if criteria met.  Only works on dedicated servers." );
ConVar mp_scrambleteams_auto_arena("mp_scrambleteams_auto_arena", "0", FCVAR_NOTIFY, "Server will automatically scramble the teams if criteria met even on arena mode.  Only works on dedicated servers.");
ConVar mp_scrambleteams_auto_windifference( "mp_scrambleteams_auto_windifference", "2", FCVAR_NOTIFY, "Number of round wins a team must lead by in order to trigger an auto scramble." );

CON_COMMAND_F( mp_forcewin, "Forces team to win", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( TFGameRules() )
	{
		int iTeam = TEAM_UNASSIGNED;
		if ( args.ArgC() == 1 )
		{
			if ( engine->IsDedicatedServer() )
				return;

			// if no team specified, use player 1's team
			CBasePlayer *pPlayer = UTIL_GetListenServerHost();
			if ( pPlayer )
			{
				iTeam = pPlayer->GetTeamNumber();
			}
		}
		else if ( args.ArgC() == 2 )
		{
			// if team # specified, use that
			iTeam = atoi( args[1] );
		}
		else
		{
			Msg( "Usage: mp_forcewin <opt: team#>" );
			return;
		}

		int iWinReason = ( TEAM_UNASSIGNED == iTeam ? WINREASON_STALEMATE : WINREASON_ALL_POINTS_CAPTURED );
		TFGameRules()->SetWinningTeam( iTeam, iWinReason );
	}
}

#endif // GAME_DLL

// Replicated cvars.
ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_medieval( "tf_medieval", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable Medieval Mode." );
ConVar tf_forced_holiday( "tf_forced_holiday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Forces a specific holiday." );

ConVar tf_medieval_autorp( "tf_medieval_autorp", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode auto-roleplaying.\n", true, 0, true, 2 );

#ifdef GAME_DLL
void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		for ( int iTeam = FIRST_GAME_TEAM, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}
ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9, ValidateCapturesPerRound );
#else
ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9 );
#endif

#ifdef GAME_DLL
// TF overrides the default value of this convar
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "30", FCVAR_GAMEDLL, "WaitingForPlayers time length in seconds" );

ConVar mp_scrambleteams_mode( "mp_scrambleteams_mode", "0", FCVAR_NOTIFY, "Sets team scramble mode:\n0 - score/connection time ratio\n1 - kill/death ratio\n2 - score\n3 - class\n4 - random" );

ConVar hide_server( "hide_server", "0", FCVAR_GAMEDLL, "Whether the server should be hidden from the master server" );

ConVar tf_gamemode_arena( "tf_gamemode_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_cp( "tf_gamemode_cp", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_ctf( "tf_gamemode_ctf", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_sd( "tf_gamemode_sd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_rd( "tf_gamemode_rd", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_payload( "tf_gamemode_payload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_mvm( "tf_gamemode_mvm", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_gamemode_passtime( "tf_gamemode_passtime", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
// ConVar tf_gamemode_siege( "tf_gamemode_siege", "0", FCVAR_NOTIFY | FCVAR_REPLICATED ); // Possibly for a future update.

ConVar tf_bot_count( "tf_bot_count", "0", FCVAR_NOTIFY );

ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat." );
ConVar tf_spectalk( "tf_spectalk", "0", FCVAR_NOTIFY, "Allows living players to hear spectators using text chat." );
ConVar tf_ctf_bonus_time( "tf_ctf_bonus_time", "0", FCVAR_NOTIFY, "Length of team crit time for CTF capture." );

ConVar tf_tournament_classlimit_scout( "tf_tournament_classlimit_scout", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Scouts.\n" );
ConVar tf_tournament_classlimit_sniper( "tf_tournament_classlimit_sniper", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Snipers.\n" );
ConVar tf_tournament_classlimit_soldier( "tf_tournament_classlimit_soldier", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Soldiers.\n" );
ConVar tf_tournament_classlimit_demoman( "tf_tournament_classlimit_demoman", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Demomen.\n" );
ConVar tf_tournament_classlimit_medic( "tf_tournament_classlimit_medic", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Medics.\n" );
ConVar tf_tournament_classlimit_heavy( "tf_tournament_classlimit_heavy", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Heavies.\n" );
ConVar tf_tournament_classlimit_pyro( "tf_tournament_classlimit_pyro", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Pyros.\n" );
ConVar tf_tournament_classlimit_spy( "tf_tournament_classlimit_spy", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Spies.\n" );
ConVar tf_tournament_classlimit_engineer( "tf_tournament_classlimit_engineer", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Engineers.\n" );
ConVar tf_tournament_classlimit_civilian( "tf_tournament_classlimit_civilian", "-1", FCVAR_NOTIFY, "Tournament mode per-team class limit for Civilians.\n" );
ConVar tf_tournament_classchange_allowed( "tf_tournament_classchange_allowed", "1", FCVAR_NOTIFY, "Allow players to change class while the game is active?.\n" );
ConVar tf_tournament_classchange_ready_allowed( "tf_tournament_classchange_ready_allowed", "1", FCVAR_NOTIFY, "Allow players to change class after they are READY?.\n" );
ConVar tf_classlimit( "tf_classlimit", "0", FCVAR_NOTIFY, "Limit on how many players can be any class (i.e. tf_class_limit 2 would limit 2 players per class).\n" );

ConVar mp_humans_must_join_team( "mp_humans_must_join_team", "any", FCVAR_NOTIFY, "Restricts human players to a single team {any, red, blue, green, yellow, spectator}" );

// TF2C cvars
ConVar tf2c_tournament_classlimits( "tf2c_tournament_classlimits", "0", FCVAR_NOTIFY, "Use Tournament class limits outside of Tournament mode." );
#else
extern ConVar english;

ConVar tf_particles_disable_weather( "tf_particles_disable_weather", "0", FCVAR_ARCHIVE, "Disable particles related to weather effects." );
#endif

// TF2C specific replicated cvars.
ConVar tf2c_falldamage_disablespread( "tf2c_falldamage_disablespread", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles random 20% fall damage spread" );
ConVar tf2c_allow_thirdperson( "tf2c_allow_thirdperson", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow players to switch to third person mode" );
ConVar tf2c_domination_override_pointlimit( "tf2c_domination_override_pointlimit", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Overrides the amount of points required to win a Domination round. -1 uses the level designer specified amount" );
ConVar tf2c_building_gun_mettle( "tf2c_building_gun_mettle", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Engineer's hauling move speed, construction, deploy speed for certain buildings, building costs, and minigun resistance" );
ConVar tf2c_allow_special_classes( "tf2c_allow_special_classes", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables gamemode specific classes (Civilian, ...) in normal gameplay (1 = Enabled, 2 = Enabled + Special Class Disguising)" );
ConVar tf2c_domination_uncap_factor( "tf2c_domination_uncap_factor", "6.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "How much faster partially capped Domination CPs should lose progress." );
ConVar tf2c_civilian_capture_rate( "tf2c_civilian_capture_rate", "5", FCVAR_NOTIFY | FCVAR_REPLICATED, "Point capture rate of the Civilian outside of VIP mode." );
ConVar tf2c_civilian_capture_rate_vip( "tf2c_civilian_capture_rate_vip", "5", FCVAR_NOTIFY | FCVAR_REPLICATED, "Point capture rate of the Civilian in VIP mode." );

// Very hacky way of sending the map info to the loading screen.
ConVar tf2c_loadinginfo_mapname( "tf2c_loadinginfo_mapname", "", FCVAR_REPLICATED );
ConVar tf2c_loadinginfo_gametype( "tf2c_loadinginfo_gametype", "0", FCVAR_REPLICATED );
ConVar tf2c_loadinginfo_fourteam( "tf2c_loadinginfo_fourteam", "0", FCVAR_REPLICATED );
// This is at the bottom so it's processed last when received from the server.
#ifdef GAME_DLL
ConVar tf2c_loadinginfo_sender( "tf2c_loadinginfo_sender", "0", FCVAR_REPLICATED, "" );
#else
static void LoadingInfoChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVar *pCvar = (ConVar *)var;
	if ( !pCvar->GetBool() )
		return;

	// On a listen server this is another backdoor between server and client.
	// Changing a replicated cvar from the server side will trigger the callback if it's in the client module.
	IGameEvent *event = gameeventmanager->CreateEvent( "serverinfo_received" );
	if ( event )
	{
		event->SetString( "mapname", tf2c_loadinginfo_mapname.GetString() );
		event->SetInt( "gametype", tf2c_loadinginfo_gametype.GetInt() );
		event->SetBool( "fourteam", tf2c_loadinginfo_fourteam.GetBool() );

		gameeventmanager->FireEventClientSide( event );
	}
}
ConVar tf2c_loadinginfo_sender( "tf2c_loadinginfo_sender", "0", FCVAR_REPLICATED, "", LoadingInfoChangedCallback );
#endif

#ifdef GAME_DLL
static void NemesisSystemChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	// Clear all and any domination relationships when changing cvar.
	if ( TFGameRules() )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer )
			{
				pPlayer->RemoveNemesisRelationships( false );
			}
		}
	}
}
ConVar tf2c_nemesis_relationships( "tf2c_nemesis_relationships", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable domination/revenge system.", NemesisSystemChangedCallback );
#else
ConVar tf2c_nemesis_relationships( "tf2c_nemesis_relationships", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable domination/revenge system." );
#endif

// Utility function
bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}

	return false;
}


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),			// VEC_VIEW					(m_vView) eye position
							
	Vector(-24, -24, 0 ),		// VEC_HULL_MIN				(m_vHullMin) hull min
	Vector( 24,  24, 82 ),		// VEC_HULL_MAX				(m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),		// VEC_DUCK_HULL_MIN		(m_vDuckHullMin) duck hull min
	Vector( 24,  24, 62 ),		// VEC_DUCK_HULL_MAX		(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),			// VEC_DUCK_VIEW			(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	// VEC_OBS_HULL_MIN			(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	// VEC_OBS_HULL_MAX			(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )			// VEC_DEAD_VIEWHEIGHT		(m_vDeadViewHeight) dead view height
);							

Vector g_TFClassViewVectors[TF_CLASS_COUNT_ALL] =
{
	Vector( 0, 0, 72 ),										// TF_CLASS_UNDEFINED

	Vector( 0, 0, 65 ),			// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	Vector( 0, 0, 75 ),			// TF_CLASS_SNIPER,
	Vector( 0, 0, 68 ),			// TF_CLASS_SOLDIER,
	Vector( 0, 0, 68 ),			// TF_CLASS_DEMOMAN,
	Vector( 0, 0, 75 ),			// TF_CLASS_MEDIC,
	Vector( 0, 0, 75 ),			// TF_CLASS_HEAVYWEAPONS,
	Vector( 0, 0, 68 ),			// TF_CLASS_PYRO,
	Vector( 0, 0, 75 ),			// TF_CLASS_SPY,
	Vector( 0, 0, 68 ),			// TF_CLASS_ENGINEER,

	Vector( 0, 0, 61 ),			// TF_CLASS_CIVILIAN,		// TF_LAST_NORMAL_CLASS
};

const CViewVectors *CTFGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundState( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFGameRules *pGamerules = (C_TFGameRules *)pStruct;
	int iRoundState = pData->m_Value.m_Int;
	pGamerules->SetRoundState( iRoundState );
}
#endif 

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iRoundState ), 0, RecvProxy_TeamplayRoundState ),
	RecvPropBool( RECVINFO( m_bInWaitingForPlayers ) ),
	RecvPropInt( RECVINFO( m_iWinningTeam ) ),
	RecvPropInt( RECVINFO( m_bInOvertime ) ),
	RecvPropInt( RECVINFO( m_bInSetup ) ),
	RecvPropInt( RECVINFO( m_bSwitchedTeamsThisRound ) ),
	RecvPropBool( RECVINFO( m_bAwaitingReadyRestart ) ),
	RecvPropTime( RECVINFO( m_flRestartRoundTime ) ),
	RecvPropTime( RECVINFO( m_flMapResetTime ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flNextRespawnWave), RecvPropTime( RECVINFO(m_flNextRespawnWave[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamRespawnWaveTimes), RecvPropFloat( RECVINFO(m_TeamRespawnWaveTimes[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTeamReady), RecvPropBool( RECVINFO(m_bTeamReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bStopWatch ) ),
	RecvPropBool( RECVINFO( m_bMultipleTrains ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bPlayerReady), RecvPropBool( RECVINFO(m_bPlayerReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bCheatsEnabledDuringLevel ) ),

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringGreen ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringYellow ) ),
	RecvPropBool( RECVINFO( m_bFourTeamMode ) ),
	RecvPropBool( RECVINFO( m_bArenaMode ) ),
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),
	RecvPropInt( RECVINFO( m_nHudType ) ),
	RecvPropBool( RECVINFO( m_bPlayingKoth ) ),
	RecvPropBool( RECVINFO( m_bPlayingMedieval ) ),
	RecvPropBool( RECVINFO( m_bPlayingHybrid_CTF_CP ) ),
	RecvPropBool( RECVINFO( m_bPlayingSpecialDeliveryMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingRobotDestructionMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingMannVsMachine ) ),
	RecvPropBool( RECVINFO( m_bCompetitiveMode ) ),
	RecvPropBool( RECVINFO( m_bPowerupMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingDomination ) ),
	RecvPropBool( RECVINFO( m_bPlayingTerritorialDomination ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hGreenKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hYellowKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hWaitingForPlayersTimer ) ),
	RecvPropEHandle( RECVINFO( m_hStalemateTimer ) ),
	RecvPropEHandle( RECVINFO( m_hTimeLimitTimer ) ),
	RecvPropEHandle( RECVINFO( m_hRoundTimer ) ),
	RecvPropInt( RECVINFO( m_iMapTimeBonus ) ),
	RecvPropInt( RECVINFO( m_iPointLimit) ),
#else
	SendPropInt( SENDINFO( m_iRoundState ), 5 ),
	SendPropBool( SENDINFO( m_bInWaitingForPlayers ) ),
	SendPropInt( SENDINFO( m_iWinningTeam ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bInOvertime ) ),
	SendPropBool( SENDINFO( m_bInSetup ) ),
	SendPropBool( SENDINFO( m_bSwitchedTeamsThisRound ) ),
	SendPropBool( SENDINFO( m_bAwaitingReadyRestart ) ),
	SendPropTime( SENDINFO( m_flRestartRoundTime ) ),
	SendPropTime( SENDINFO( m_flMapResetTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flNextRespawnWave ), SendPropTime( SENDINFO_ARRAY( m_flNextRespawnWave ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_TeamRespawnWaveTimes ), SendPropFloat( SENDINFO_ARRAY( m_TeamRespawnWaveTimes ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bTeamReady ), SendPropBool( SENDINFO_ARRAY( m_bTeamReady ) ) ),
	SendPropBool( SENDINFO( m_bStopWatch ) ),
	SendPropBool( SENDINFO( m_bMultipleTrains ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerReady ), SendPropBool( SENDINFO_ARRAY( m_bPlayerReady ) ) ),
	SendPropBool( SENDINFO( m_bCheatsEnabledDuringLevel ) ),

	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringGreen ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringYellow ) ),
	SendPropBool( SENDINFO( m_bFourTeamMode ) ),
	SendPropBool( SENDINFO( m_bArenaMode ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),
	SendPropInt( SENDINFO( m_nHudType ) ),
	SendPropBool( SENDINFO( m_bPlayingKoth ) ),
	SendPropBool( SENDINFO( m_bPlayingMedieval ) ),
	SendPropBool( SENDINFO( m_bPlayingHybrid_CTF_CP ) ),
	SendPropBool( SENDINFO( m_bPlayingSpecialDeliveryMode ) ),
	SendPropBool( SENDINFO( m_bPlayingRobotDestructionMode ) ),
	SendPropBool( SENDINFO( m_bPlayingMannVsMachine ) ),
	SendPropBool( SENDINFO( m_bCompetitiveMode ) ),
	SendPropBool( SENDINFO( m_bPowerupMode ) ),
	SendPropBool( SENDINFO( m_bPlayingDomination ) ),
	SendPropBool( SENDINFO( m_bPlayingTerritorialDomination ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hGreenKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hYellowKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hWaitingForPlayersTimer ) ),
	SendPropEHandle( SENDINFO( m_hStalemateTimer ) ),
	SendPropEHandle( SENDINFO( m_hTimeLimitTimer ) ),
	SendPropEHandle( SENDINFO( m_hRoundTimer ) ),
	SendPropInt( SENDINFO( m_iMapTimeBonus ) ),
	SendPropInt( SENDINFO( m_iPointLimit ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy );

#ifdef CLIENT_DLL
void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTFGameRules *pRules = TFGameRules();
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
	RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
END_RECV_TABLE()

void CTFGameRulesProxy::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	TFGameRules()->OnPreDataChanged( updateType );
}
void CTFGameRulesProxy::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	TFGameRules()->OnDataChanged( updateType );
}

#else
void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTFGameRules *pRules = TFGameRules();
	Assert( pRules );
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
	SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules ),
END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
	DEFINE_KEYFIELD( m_iHud_Type, FIELD_INTEGER, "hud_type"),
	DEFINE_KEYFIELD( m_bFourTeamMode, FIELD_BOOLEAN, "fourteammode"),
	DEFINE_KEYFIELD( m_bCTF_Overtime, FIELD_BOOLEAN, "ctf_overtime" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetStalemateOnTimelimit", InputSetStalemateOnTimelimit ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetGreenTeamRespawnWaveTime", InputSetGreenTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetYellowTeamRespawnWaveTime", InputSetYellowTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddGreenTeamRespawnWaveTime", InputAddGreenTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddYellowTeamRespawnWaveTime", InputAddYellowTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetGreenTeamGoalString", InputSetGreenTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetYellowTeamGoalString", InputSetYellowTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenTeamRole", InputSetGreenTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowTeamRole", InputSetYellowTeamRole ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddGreenTeamScore", InputAddGreenTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddYellowTeamScore", InputAddYellowTeamScore),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamScore", InputSetRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamScore", InputSetBlueTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenTeamScore", InputSetGreenTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowTeamScore", InputSetYellowTeamScore),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetGreenKothClockActive", InputSetGreenKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetYellowKothClockActive", InputSetYellowKothClockActive ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCTFCaptureBonusTime", InputSetCTFCaptureBonusTime ),

	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOGreen", InputPlayVOGreen ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOYellow", InputPlayVOYellow ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),

	DEFINE_INPUTFUNC( FIELD_STRING, "HandleMapEvent", InputHandleMapEvent ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetRoundRespawnFreezeEnabled", InputSetRoundRespawnFreezeEnabled ),

	// Outputs.
	DEFINE_OUTPUT( m_OnWonByTeam1, "OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2, "OnWonByTeam2" ),
	DEFINE_OUTPUT( m_OnWonByTeam3, "OnWonByTeam3" ),
	DEFINE_OUTPUT( m_OnWonByTeam4, "OnWonByTeam4" ),
	DEFINE_OUTPUT( m_Team1PlayersChanged, "Team1PlayersChanged" ),
	DEFINE_OUTPUT( m_Team2PlayersChanged, "Team2PlayersChanged" ),
	DEFINE_OUTPUT( m_Team3PlayersChanged, "Team3PlayersChanged" ),
	DEFINE_OUTPUT( m_Team4PlayersChanged, "Team4PlayersChanged" ),
	DEFINE_OUTPUT( m_OnStateEnterBetweenRounds, "OnStateEnterBetweenRounds" ),
	DEFINE_OUTPUT( m_OnStateEnterPreRound, "OnStateEnterPreRound" ),
	DEFINE_OUTPUT( m_OnStateExitPreRound, "OnStateExitPreRound" ),
	DEFINE_OUTPUT( m_OnStateEnterRoundRunning, "OnStateEnterRoundRunning" ),

	DEFINE_OUTPUT( m_OnOvertime, "OnOvertime" ),
	DEFINE_OUTPUT( m_OnSuddenDeath, "OnSuddenDeath"),
END_DATADESC()

CTFGameRulesProxy::CTFGameRulesProxy()
{
	g_pGameRulesProxy = this;

	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "player_team" );
}

CTFGameRulesProxy::~CTFGameRulesProxy()
{
	if ( g_pGameRulesProxy == this )
	{
		g_pGameRulesProxy = NULL;
	}
}


void CTFGameRulesProxy::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "teamplay_round_win" ) )
	{
		switch ( event->GetInt( "team" ) )
		{
		case TF_TEAM_RED:
			m_OnWonByTeam1.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_OnWonByTeam2.FireOutput( this, this );
			break;
		case TF_TEAM_GREEN:
			m_OnWonByTeam3.FireOutput( this, this );
			break;
		case TF_TEAM_YELLOW:
			m_OnWonByTeam4.FireOutput( this, this );
			break;
		}
	}
	else if ( FStrEq( event->GetName(), "player_team" ) )
	{
		switch ( event->GetInt( "team" ) )
		{
		case TF_TEAM_RED:
			m_Team1PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_Team2PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_GREEN:
			m_Team3PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_YELLOW:
			m_Team4PlayersChanged.FireOutput( this, this );
			break;
		}

		switch ( event->GetInt( "oldteam" ) )
		{
		case TF_TEAM_RED:
			m_Team1PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_Team2PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_GREEN:
			m_Team3PlayersChanged.FireOutput( this, this );
			break;
		case TF_TEAM_YELLOW:
			m_Team4PlayersChanged.FireOutput( this, this );
			break;
		}
	}
}


void CTFGameRulesProxy::InputSetStalemateOnTimelimit( inputdata_t &inputdata )
{
	TFGameRules()->SetStalemateOnTimelimit( inputdata.value.Bool() );
}


void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}


void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}


void CTFGameRulesProxy::InputSetGreenTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamRespawnWaveTime(TF_TEAM_GREEN, inputdata.value.Float());
}


void CTFGameRulesProxy::InputSetYellowTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamRespawnWaveTime(TF_TEAM_YELLOW, inputdata.value.Float());
}


void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}


void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}


void CTFGameRulesProxy::InputAddGreenTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->AddTeamRespawnWaveTime(TF_TEAM_GREEN, inputdata.value.Float());
}


void CTFGameRulesProxy::InputAddYellowTeamRespawnWaveTime(inputdata_t &inputdata)
{
	TFGameRules()->AddTeamRespawnWaveTime(TF_TEAM_YELLOW, inputdata.value.Float());
}


void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}


void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}


void CTFGameRulesProxy::InputSetGreenTeamGoalString(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamGoalString(TF_TEAM_GREEN, inputdata.value.String());
}


void CTFGameRulesProxy::InputSetYellowTeamGoalString(inputdata_t &inputdata)
{
	TFGameRules()->SetTeamGoalString(TF_TEAM_YELLOW, inputdata.value.String());
}


void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetGreenTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetYellowTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetRequiredObserverTarget( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String() );
	TFGameRules()->SetRequiredObserverTarget( pEntity );
}


void CTFGameRulesProxy::InputAddRedTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputAddBlueTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputAddGreenTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputAddYellowTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

void CTFGameRulesProxy::InputSetRedTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetBlueTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetGreenTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->SetScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetYellowTeamScore( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->SetScore( inputdata.value.Int() );
	}
}


void CTFGameRulesProxy::InputSetRedKothClockActive( inputdata_t &inputdata )
{
	SetKothClockActive( TF_TEAM_RED, inputdata );
}


void CTFGameRulesProxy::InputSetBlueKothClockActive( inputdata_t &inputdata )
{
	SetKothClockActive( TF_TEAM_BLUE, inputdata );
}


void CTFGameRulesProxy::InputSetGreenKothClockActive( inputdata_t &inputdata )
{
	SetKothClockActive( TF_TEAM_GREEN, inputdata );
}


void CTFGameRulesProxy::InputSetYellowKothClockActive( inputdata_t &inputdata )
{
	SetKothClockActive( TF_TEAM_YELLOW, inputdata );
}


void CTFGameRulesProxy::SetKothClockActive( int iActiveTeam, inputdata_t &inputdata )
{
	int c = GetNumberOfTeams();
	if ( iActiveTeam >= c )
	{
		Warning( "Attempted to activate KOTH timer for team %s but team %s is not active!\n", g_aTeamNames[iActiveTeam], g_aTeamNames[iActiveTeam] );
		return;
	}

	variant_t emptyVariant;

	for ( int i = FIRST_GAME_TEAM; i < c; i++ )
	{
		CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( i );
		if ( !pTimer )
			continue;

		if ( i == iActiveTeam )
		{
			pTimer->AcceptInput( "Resume", NULL, NULL, emptyVariant, 0 );
		}
		else
		{
			pTimer->AcceptInput( "Pause", NULL, NULL, emptyVariant, 0 );
		}
	}
}


void CTFGameRulesProxy::InputSetCTFCaptureBonusTime( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_flCTFBonusTime = inputdata.value.Float();
	}
}


void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( 255, inputdata.value.String() );
	}
}



void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
	}
}


void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
	}
}


void CTFGameRulesProxy::InputPlayVOGreen( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_GREEN, inputdata.value.String() );
	}
}


void CTFGameRulesProxy::InputPlayVOYellow( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_YELLOW, inputdata.value.String() );
	}
}


void CTFGameRulesProxy::InputHandleMapEvent( inputdata_t &inputdata )
{
	// Do nothing.
}


void CTFGameRulesProxy::InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata )
{
	TFGameRules()->SetPreRoundFreezeTimeEnabled( inputdata.value.Bool() );
}


void CTFGameRulesProxy::Activate()
{
	TFGameRules()->SetFourTeamGame( m_bFourTeamMode );

	TFGameRules()->Activate();

	// Now that we know how many teams there should be remove any that we don't need.
	TFTeamMgr()->RemoveExtraTeams();

	TFGameRules()->SetHudType( m_iHud_Type );
	TFGameRules()->SetCTFOvertime( m_bCTF_Overtime );

	BaseClass::Activate();
}

class CTFFuncVIPProgressPathBlocker : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTFFuncVIPProgressPathBlocker, CBaseTrigger );
	void Spawn();
};

LINK_ENTITY_TO_CLASS( func_vip_path_blocker, CTFFuncVIPProgressPathBlocker );

void CTFFuncVIPProgressPathBlocker::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}

ConVar tf_debug_vip_progress( "tf_debug_vip_progress", "0", FCVAR_CHEAT );

class CTFLogicVIP : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTFLogicVIP, CLogicalEntity );
	DECLARE_DATADESC();

	CTFLogicVIP();
	~CTFLogicVIP();

	void	Spawn();
	void	Activate();
	void	VIPLogicThink();

	void	OnVIPKilled( CTFPlayer *pPlayer, CTFPlayer *pKiller, CTFPlayer *pAssister, const CTakeDamageInfo &info );
	void	OnVIPEscaped( CTFPlayer *pPlayer, bool bWin );
	void	SetProgressPath( int iTeam, const char *pszName );
	void	InputSetRedProgressPath( inputdata_t &inputdata );
	void	InputSetBlueProgressPath( inputdata_t &inputdata );
	void	InputSetGreenProgressPath( inputdata_t &inputdata );
	void	InputSetYellowProgressPath( inputdata_t &inputdata );

private:
	bool	PointsCrossProgressPathBlocker(const Vector &vecStart, const Vector &vecEnd);
	float	CalcProgressFrac( int iTeam );

	bool m_bRedEscort;
	bool m_bBlueEscort;
	bool m_bGreenEscort;
	bool m_bYellowEscort;

	CUtlVector<Vector> m_vecPathNodes[TF_TEAM_COUNT];

	bool m_bForceMapReset;
	bool m_bSwitchTeamsOnWin;

	bool m_bShowEscortProgress;
	bool m_bDisableVipDeathAnnouncer;
	int m_iBonusTime;

	COutputEvent m_outputOnRedVIPDeath;
	COutputEvent m_outputOnBlueVIPDeath;
	COutputEvent m_outputOnGreenVIPDeath;
	COutputEvent m_outputOnYellowVIPDeath;
	COutputEvent m_outputOnRedVIPEscape;
	COutputEvent m_outputOnBlueVIPEscape;
	COutputEvent m_outputOnGreenVIPEscape;
	COutputEvent m_outputOnYellowVIPEscape;
};

LINK_ENTITY_TO_CLASS( tf_logic_vip, CTFLogicVIP );

CTFLogicVIP *g_pVIPLogic = NULL;

BEGIN_DATADESC( CTFLogicVIP )
	DEFINE_KEYFIELD( m_bRedEscort, FIELD_BOOLEAN, "red_escort" ),
	DEFINE_KEYFIELD( m_bBlueEscort, FIELD_BOOLEAN, "blue_escort" ),
	DEFINE_KEYFIELD( m_bGreenEscort, FIELD_BOOLEAN, "green_escort" ),
	DEFINE_KEYFIELD( m_bYellowEscort, FIELD_BOOLEAN, "yellow_escort" ),
	DEFINE_KEYFIELD( m_bForceMapReset, FIELD_BOOLEAN, "force_map_reset" ),
	DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "switch_teams" ),
	DEFINE_KEYFIELD( m_bShowEscortProgress, FIELD_BOOLEAN, "show_escort_progress" ),
	DEFINE_KEYFIELD( m_bDisableVipDeathAnnouncer, FIELD_BOOLEAN, "disable_vip_death_announcer" ),
	DEFINE_KEYFIELD( m_iBonusTime, FIELD_INTEGER, "vip_bonus_time" ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnRedVIPDeath, "OnRedVIPDeath" ),
	DEFINE_OUTPUT( m_outputOnBlueVIPDeath, "OnBlueVIPDeath" ),
	DEFINE_OUTPUT( m_outputOnGreenVIPDeath, "OnGreenVIPDeath" ),
	DEFINE_OUTPUT( m_outputOnYellowVIPDeath, "OnYellowVIPDeath" ),
	DEFINE_OUTPUT( m_outputOnRedVIPEscape, "OnRedVIPEscape" ),
	DEFINE_OUTPUT( m_outputOnBlueVIPEscape, "OnBlueVIPEscape" ),
	DEFINE_OUTPUT( m_outputOnGreenVIPEscape, "OnGreenVIPEscape" ),
	DEFINE_OUTPUT( m_outputOnYellowVIPEscape, "OnYellowVIPEscape" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedProgressPath", InputSetRedProgressPath ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueProgressPath", InputSetBlueProgressPath ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetGreenProgressPath", InputSetGreenProgressPath ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetYellowProgressPath", InputSetYellowProgressPath ),
END_DATADESC()

CTFLogicVIP::CTFLogicVIP()
{
	g_pVIPLogic = this;

	m_bForceMapReset = false;
	m_bSwitchTeamsOnWin = false;

	m_bShowEscortProgress = true;
	m_bDisableVipDeathAnnouncer = false;
	m_iBonusTime = -1;
}

CTFLogicVIP::~CTFLogicVIP()
{
	g_pVIPLogic = NULL;
}

void CTFLogicVIP::Spawn()
{
	BaseClass::Spawn();

	SetThink( &CTFLogicVIP::VIPLogicThink );
	SetNextThink( gpGlobals->curtime );
}

void CTFLogicVIP::Activate()
{
	BaseClass::Activate();

	ForEachTFTeam( [&]( int iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( pTeam )
		{
			switch ( iTeam )
			{
				case TF_TEAM_RED:
					pTeam->SetEscorting( m_bRedEscort );
					break;
				case TF_TEAM_BLUE:
					pTeam->SetEscorting( m_bBlueEscort );
					break;
				case TF_TEAM_GREEN:
					pTeam->SetEscorting( m_bGreenEscort );
					break;
				case TF_TEAM_YELLOW:
					pTeam->SetEscorting( m_bYellowEscort );
					break;
			}

			return true;
		}

		return false;
	} );
}

void CTFLogicVIP::VIPLogicThink()
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	CTFObjectiveResource *pObjectiveResource = ObjectiveResource();
	if ( pObjectiveResource )
	{
		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			pObjectiveResource->SetVIPProgress( CalcProgressFrac( i ), i );
		}

		pObjectiveResource->SetDisplayObjectiveHUD( m_bShowEscortProgress );
	}
}

void CTFLogicVIP::OnVIPKilled( CTFPlayer *pPlayer, CTFPlayer *pKiller, CTFPlayer *pAssister, const CTakeDamageInfo &info )
{
	if ( pKiller && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		// Reward the killer with some temporary crits so the Civilian will be likely to be spawn some protectors.
		pKiller->m_Shared.AddCond( TF_COND_CRITBOOSTED_ON_KILL, m_iBonusTime >= 0 ? m_iBonusTime : tf2c_vip_bonus_time.GetFloat(), pPlayer );
		// Announce the vip death
		if( !m_bDisableVipDeathAnnouncer ) 
		{
			g_TFAnnouncer.Speak( pKiller->GetTeamNumber(), TF_ANNOUNCER_VIP_KILLED );
			g_TFAnnouncer.Speak( pPlayer->GetTeamNumber(), TF_ANNOUNCER_VIP_FRIENDLY_KILLED );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "vip_death" );
	if ( event )
	{
		event->SetInt( "userid", pKiller ? pKiller->GetUserID() : 0 );
		event->SetInt( "victim", pPlayer->entindex() );
		event->SetInt( "attacker", pKiller ? pKiller->entindex() : 0 );
		event->SetInt( "assister", pAssister ? pAssister->entindex() : 0 );
		event->SetBool( "headshot", info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT );
		event->SetFloat( "timeleft", TFGameRules()->GetRoundTimer() ? TFGameRules()->GetRoundTimer()->GetTimeRemaining() - gpGlobals->curtime : 0 );

		gameeventmanager->FireEvent( event );
	}

	// Output.
	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_outputOnRedVIPDeath.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_outputOnBlueVIPDeath.FireOutput( this, this );
			break;
		case TF_TEAM_GREEN:
			m_outputOnGreenVIPDeath.FireOutput( this, this );
			break;
		case TF_TEAM_YELLOW:
			m_outputOnYellowVIPDeath.FireOutput( this, this );
			break;
	}
}

void CTFLogicVIP::OnVIPEscaped( CTFPlayer *pPlayer, bool bWin )
{
	Assert( pPlayer->IsVIP() );

	int iTeam = pPlayer->GetTeamNumber();

	IGameEvent *event = gameeventmanager->CreateEvent( "vip_escaped" );

	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );

		gameeventmanager->FireEvent( event );
	}

	// Automatic win only if CVIPSafetyZone requests it
	if ( bWin )
	{
		TFGameRules()->SetWinningTeam( iTeam, WINREASON_VIP_ESCAPED, m_bForceMapReset, m_bSwitchTeamsOnWin );
	}

	// Output.
	switch ( iTeam )
	{
	case TF_TEAM_RED:
		m_outputOnRedVIPEscape.FireOutput( this, this );
		break;
	case TF_TEAM_BLUE:
		m_outputOnBlueVIPEscape.FireOutput( this, this );
		break;
	case TF_TEAM_GREEN:
		m_outputOnGreenVIPEscape.FireOutput( this, this );
		break;
	case TF_TEAM_YELLOW:
		m_outputOnYellowVIPEscape.FireOutput( this, this );
		break;
	}
}

void CTFLogicVIP::InputSetRedProgressPath( inputdata_t &inputdata )
{
	SetProgressPath( TF_TEAM_RED, inputdata.value.String() );
}

void CTFLogicVIP::InputSetBlueProgressPath( inputdata_t &inputdata )
{
	SetProgressPath( TF_TEAM_BLUE, inputdata.value.String() );
}

void CTFLogicVIP::InputSetGreenProgressPath( inputdata_t &inputdata )
{
	SetProgressPath( TF_TEAM_GREEN, inputdata.value.String() );
}

void CTFLogicVIP::InputSetYellowProgressPath( inputdata_t &inputdata )
{
	SetProgressPath( TF_TEAM_YELLOW, inputdata.value.String() );
}

void CTFLogicVIP::SetProgressPath( int iTeam, const char *pszName )
{
	m_vecPathNodes[iTeam].RemoveAll();
	CPathTrack *pPathTrack = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, pszName ) );

	for ( ; pPathTrack; pPathTrack = pPathTrack->GetNext() )
	{
		m_vecPathNodes[iTeam].AddToTail( pPathTrack->GetAbsOrigin() );
	}
}

bool CTFLogicVIP::PointsCrossProgressPathBlocker(const Vector &vecStart, const Vector &vecEnd)
{
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_vip_path_blocker" ) ) != NULL )
	{
		CTFFuncVIPProgressPathBlocker *pBlocker = (CTFFuncVIPProgressPathBlocker *)pEntity;

		Ray_t ray;
		ray.Init( vecStart, vecEnd );

		trace_t tr;
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pBlocker, &tr );

		if ( tr.fraction < 1.f )
			return true;
	}

	return false;
}

float CTFLogicVIP::CalcProgressFrac( int iTeam )
{
	if ( m_vecPathNodes[iTeam].Count() < 2 )
	{
		return 0.0f;
	}

	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam || !pTeam->IsEscorting() )
		return 0.0f;

	CTFPlayer *pPlayer = pTeam->GetVIP();
	if ( !pPlayer || !pPlayer->IsVIP() || !pPlayer->IsAlive() )
		return 0.0f;

	Vector vecPositionVIP = pPlayer->GetAbsOrigin();

	Vector vecLineA, vecLineB;
	vecLineA = m_vecPathNodes[iTeam][0];

	// Debugging
	Vector vecActualClosest;

	float flTotalDistance = 0.f;
	float flFinalDistance = 0.f;
	float flLowestDistanceToLine = FLT_MAX;
	for ( int i = 1; i < m_vecPathNodes[iTeam].Count(); ++i )
	{
		vecLineB = m_vecPathNodes[iTeam][i];

		float flDistance = vecLineA.DistTo( vecLineB );

		Vector vecClosest;
		float flPercentage;
		CalcClosestPointOnLineSegment( vecPositionVIP, vecLineA, vecLineB, vecClosest, &flPercentage );

		float flDistanceToLine = vecClosest.DistTo( vecPositionVIP );
		if ( flLowestDistanceToLine > flDistanceToLine && !PointsCrossProgressPathBlocker( vecPositionVIP, vecClosest ) )
		{
			flLowestDistanceToLine = flDistanceToLine;
			flFinalDistance = flPercentage * flDistance + flTotalDistance;

			// Debugging
			vecActualClosest = vecClosest;
		}

		flTotalDistance += flDistance;
		vecLineA = vecLineB;
	}

	if ( tf_debug_vip_progress.GetBool() )
	{
		DevMsg( "%.2f\n", flFinalDistance / flTotalDistance );
		if ( flLowestDistanceToLine != FLT_MAX )
		{
			NDebugOverlay::Line( vecPositionVIP, vecActualClosest, 255, 0, 0, true, 0.11f );
		}

		for ( int i = 0; i < m_vecPathNodes[iTeam].Count() - 1; ++i )
		{
			NDebugOverlay::Line( m_vecPathNodes[iTeam][i], m_vecPathNodes[iTeam][i + 1], 0, 255, 0, true, 0.11f );
		}
	}

	if ( flLowestDistanceToLine == FLT_MAX )
	{
		return 0.0f;
	}

	return flFinalDistance / flTotalDistance;
}

class CTFClassLimits : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTFClassLimits, CLogicalEntity );
	DECLARE_DATADESC();

	void	Spawn( void );

	inline int GetTeam() { return m_iTeam; }

	int GetLimitForClass( int iClass )
	{
		int result;

		if ( iClass < TF_CLASS_COUNT_ALL )
		{
			switch ( iClass )
			{
				default:
					result = -1;
				case TF_CLASS_ENGINEER:
					result = m_nEngineerLimit;
					break;
				case TF_CLASS_SPY:
					result = m_nSpyLimit;
					break;
				case TF_CLASS_PYRO:
					result = m_nPyroLimit;
					break;
				case TF_CLASS_HEAVYWEAPONS:
					result = m_nHeavyLimit;
					break;
				case TF_CLASS_MEDIC:
					result = m_nMedicLimit;
					break;
				case TF_CLASS_DEMOMAN:
					result = m_nDemomanLimit;
					break;
				case TF_CLASS_SOLDIER:
					result = m_nSoldierLimit;
					break;
				case TF_CLASS_SNIPER:
					result = m_nSniperLimit;
					break;
				case TF_CLASS_SCOUT:
					result = m_nScoutLimit;
					break;
			}
		}
		else
		{
			result = -1;
		}

		return result;
	}

private:
	int		m_iTeam;

	int		m_nScoutLimit;
	int		m_nSoldierLimit;
	int		m_nPyroLimit;
	int		m_nDemomanLimit;
	int		m_nHeavyLimit;
	int		m_nEngineerLimit;
	int		m_nMedicLimit;
	int		m_nSniperLimit;
	int		m_nSpyLimit;
};

LINK_ENTITY_TO_CLASS( tf_logic_classlimits, CTFClassLimits );

BEGIN_DATADESC( CTFClassLimits )
	DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_nScoutLimit, FIELD_INTEGER, "ScoutLimit" ),
	DEFINE_KEYFIELD( m_nSoldierLimit, FIELD_INTEGER, "SoldierLimit" ),
	DEFINE_KEYFIELD( m_nPyroLimit, FIELD_INTEGER, "PyroLimit" ),
	DEFINE_KEYFIELD( m_nDemomanLimit, FIELD_INTEGER, "DemomanLimit" ),
	DEFINE_KEYFIELD( m_nHeavyLimit, FIELD_INTEGER, "HeavyLimit" ),
	DEFINE_KEYFIELD( m_nEngineerLimit, FIELD_INTEGER, "EngineerLimit" ),
	DEFINE_KEYFIELD( m_nMedicLimit, FIELD_INTEGER, "MedicLimit" ),
	DEFINE_KEYFIELD( m_nSniperLimit, FIELD_INTEGER, "SniperLimit" ),
	DEFINE_KEYFIELD( m_nSpyLimit, FIELD_INTEGER, "SpyLimit" ),
END_DATADESC()


void CTFClassLimits::Spawn( void )
{
	BaseClass::Spawn();
}

class CTFLogicHoliday : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicHoliday, CBaseEntity );
	DECLARE_DATADESC();

	void	Spawn( void );

private:

	int	m_iHolidayType;	//Holiday type
};

LINK_ENTITY_TO_CLASS( tf_logic_holiday, CTFLogicHoliday );

BEGIN_DATADESC( CTFLogicHoliday )
	DEFINE_KEYFIELD( m_iHolidayType, FIELD_INTEGER, "holiday_type" ),
END_DATADESC()

void CTFLogicHoliday::Spawn( void )
{
	switch ( m_iHolidayType ) {
		case 2:
			TFGameRules()->m_iHolidayMode = TF_HOLIDAY_HALLOWEEN;
			break;
		case 3:
			TFGameRules()->m_iHolidayMode = TF_HOLIDAY_BIRTHDAY;
			break;
		case 4:
			TFGameRules()->m_iHolidayMode = TF_HOLIDAY_WINTER;
			break;
		default:
			break;
	}

	BaseClass::Spawn();
}

class CArenaLogic : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CArenaLogic, CLogicalEntity );
	DECLARE_DATADESC();

	CArenaLogic();

	void	Spawn( void );
	void	ArenaLogicThink( void );
	void	FireGameEvent( IGameEvent *event );
	void	InputRoundActivate( inputdata_t &inputdata );

	COutputEvent	m_OnArenaRoundStart;
	COutputEvent	m_OnTimeLimitEnd;
	int				m_iRoundTimeOverride;
	bool			m_bSwitchTeamsOnWin;

private:
	int				m_iCapEnableDelay;
	bool			m_bCapUnlocked;

	COutputEvent	m_OnCapEnabled;
};

BEGIN_DATADESC( CArenaLogic )

	DEFINE_KEYFIELD( m_iCapEnableDelay, FIELD_INTEGER, "CapEnableDelay" ),
	DEFINE_KEYFIELD( m_iRoundTimeOverride, FIELD_INTEGER, "RoundTimeOverride"),
	DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "switch_teams" ),
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	// Outputs
	DEFINE_OUTPUT(	m_OnArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT(	m_OnTimeLimitEnd, "OnTimeLimitEnd" ),
	DEFINE_OUTPUT(	m_OnCapEnabled, "OnCapEnabled" ),

	DEFINE_THINKFUNC( ArenaLogicThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_arena, CArenaLogic );

CArenaLogic::CArenaLogic()
{
	m_iCapEnableDelay = 0;
	m_bSwitchTeamsOnWin = false;
	m_iRoundTimeOverride = -1;
	m_bCapUnlocked = false;

	ListenForGameEvent( "arena_round_start" );
}

void CArenaLogic::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( &CArenaLogic::ArenaLogicThink );
	SetNextThink( gpGlobals->curtime );
}

void CArenaLogic::ArenaLogicThink( void )
{
	// Live TF2 checks m_fCapEnableTime from TFGameRules here.
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( TFGameRules()->State_Get() == GR_STATE_STALEMATE )
	{
		if ( !m_bCapUnlocked && ObjectiveResource() )
		{
			for ( int i = 0; i < ObjectiveResource()->GetNumControlPoints(); i++ )
			{
				if ( !ObjectiveResource()->GetCPLocked( i ) )
				{
					m_bCapUnlocked = true;
					m_OnCapEnabled.FireOutput( this, this );
					break;
				}
			}
		}
	}
}

void CArenaLogic::FireGameEvent( IGameEvent *event )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	// Set unlock time.
	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		int iUnlockTime = tf_arena_override_cap_enable_time.GetInt() >= 0 ? tf_arena_override_cap_enable_time.GetInt() : m_iCapEnableDelay;

		variant_t sVariant;
		sVariant.SetInt( iUnlockTime );
		pPoint->AcceptInput( "SetUnlockTime", NULL, NULL, sVariant, 0 );
	}
}

void CArenaLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	// Lock the CP.
	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_iCapEnableDelay );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
	}

	m_bCapUnlocked = false;
}


class CKothLogic : public CLogicalEntity
{
public:
	DECLARE_CLASS( CKothLogic, CLogicalEntity );
	DECLARE_DATADESC();

	CKothLogic();

	virtual void	InputAddBlueTimer( inputdata_t &inputdata );
	virtual void	InputAddRedTimer( inputdata_t &inputdata );
	virtual void	InputAddGreenTimer( inputdata_t &inputdata );
	virtual void	InputAddYellowTimer( inputdata_t &inputdata );
	virtual void	InputSetBlueTimer( inputdata_t &inputdata );
	virtual void	InputSetRedTimer( inputdata_t &inputdata );
	virtual void	InputSetGreenTimer( inputdata_t &inputdata );
	virtual void	InputSetYellowTimer( inputdata_t &inputdata );
	virtual void	InputRoundSpawn( inputdata_t &inputdata );
	virtual void	InputRoundActivate( inputdata_t &inputdata );

private:
	int m_iTimerLength;
	int m_iUnlockPoint;

};

BEGIN_DATADESC( CKothLogic )

	DEFINE_KEYFIELD( m_iTimerLength, FIELD_INTEGER, "timer_length" ),
	DEFINE_KEYFIELD( m_iUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTimer", InputAddRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddGreenTimer", InputAddGreenTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddYellowTimer", InputAddYellowTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTimer", InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenTimer", InputSetGreenTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowTimer", InputSetYellowTimer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_koth, CKothLogic );

CKothLogic::CKothLogic()
{
	m_iTimerLength = 180;
	m_iUnlockPoint = 30;
}

void CKothLogic::InputRoundSpawn( inputdata_t &inputdata )
{
	variant_t sVariant;
	sVariant.SetInt( m_iTimerLength );

	for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
	{
		// Create the timer.
		CTeamRoundTimer *pTimer = static_cast<CTeamRoundTimer *>( CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );
		TFGameRules()->SetKothTimer( pTimer, i );

		// Make sure it was added properly.
		pTimer = TFGameRules()->GetKothTimer( i );
		if ( pTimer )
		{
			char szName[64];
			V_sprintf_safe( szName, "zz_%s_koth_timer", g_aTeamLowerNames[i] );

			pTimer->SetName( AllocPooledString( szName ) );
			pTimer->SetShowInHud( true );
			pTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			pTimer->ChangeTeam( i );
		}
	}
}

void CKothLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_iUnlockPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

void CKothLogic::InputAddRedTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_RED );
	if ( pTimer )
	{
		pTimer->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddBlueTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_BLUE );
	if ( pTimer )
	{
		pTimer->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddGreenTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_GREEN );
	if ( pTimer )
	{
		pTimer->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddYellowTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_YELLOW );
	if ( pTimer )
	{
		pTimer->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetRedTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_RED );
	if ( pTimer )
	{
		pTimer->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetBlueTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_BLUE );
	if ( pTimer )
	{
		pTimer->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetGreenTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_GREEN );
	if ( pTimer )
	{
		pTimer->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetYellowTimer( inputdata_t &inputdata )
{
	CTeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( TF_TEAM_YELLOW );
	if ( pTimer )
	{
		pTimer->SetTimeRemaining( inputdata.value.Int() );
	}
}

class CCPTimerLogic : public CLogicalEntity, public TAutoList<CCPTimerLogic>
{
public:
	DECLARE_CLASS( CCPTimerLogic, CLogicalEntity );
	DECLARE_DATADESC();

	CCPTimerLogic()
	{
		m_iTimerLength = 0;
		m_flCountdownEndTime = -1.0f;
		m_bFire15Sec = false;
		m_bFire10Sec = false;
		m_bFire5Sec = false;
	}

	void Spawn( void )
	{
		BaseClass::Spawn();

		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15f, "CCPTimerLogicThink" );
	}

	void InputRoundSpawn( inputdata_t &input )
	{
		m_hControlPoint = dynamic_cast<CTeamControlPoint *>( gEntList.FindEntityByName( NULL, m_iszControlPoint ) );

		if ( m_hControlPoint.Get() == NULL )
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING( m_iszControlPoint ) );
		}
	}

	bool TimerMayExpire( void )
	{
		if ( m_hControlPoint )
		{
			if ( TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, m_hControlPoint->GetPointIndex() ) )
				return false;
		}

		return true;
	}

	void Think( void )
	{
		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15f, "CCPTimerLogicThink" );

		if ( !m_hControlPoint || !ObjectiveResource() || !TFGameRules()->PointsMayBeCaptured() )
			return;

		if ( TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, m_hControlPoint->GetPointIndex() ) )
		{
			if ( m_flCountdownEndTime == -1.0f )
			{
				// Begin the countdown once the point becomes available for capture.
				float flTime = (float)m_iTimerLength;
				m_flCountdownEndTime = gpGlobals->curtime + flTime;

				m_bFire15Sec = true;
				m_bFire10Sec = true;
				m_bFire5Sec = true;
				
				m_OnCountdownStart.FireOutput( this, this );
				ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), m_flCountdownEndTime );
			}
			else
			{
				if ( gpGlobals->curtime <= m_flCountdownEndTime )
				{
					float flTimeLeft = m_flCountdownEndTime - gpGlobals->curtime;

					if ( flTimeLeft <= 15.0f && m_bFire15Sec )
					{
						m_OnCountdown15SecRemain.FireOutput( this, this );
						m_bFire15Sec = false;
					}
					else if ( flTimeLeft <= 10.0f && m_bFire10Sec )
					{
						m_OnCountdown10SecRemain.FireOutput( this, this );
						m_bFire10Sec = false;
					}
					else if ( flTimeLeft <= 5.0f && m_bFire5Sec )
					{
						m_OnCountdown5SecRemain.FireOutput( this, this );
						m_bFire5Sec = false;
					}
				}
				else if ( ObjectiveResource()->GetCappingTeam( m_hControlPoint->GetPointIndex() ) == TEAM_UNASSIGNED )
				{
					// If the point is not being capped finish the countdown.
					m_flCountdownEndTime = -1.0f;

					m_bFire15Sec = false;
					m_bFire10Sec = false;
					m_bFire5Sec = false;

					m_OnCountdownEnd.FireOutput( this, this );
					ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), -1.0f );
				}
			}
		}
		else
		{
			m_flCountdownEndTime = -1.0f;
		}
	}

private:
	string_t m_iszControlPoint;
	CHandle<CTeamControlPoint> m_hControlPoint;

	int m_iTimerLength;
	float m_flCountdownEndTime;
	bool m_bFire15Sec;
	bool m_bFire10Sec;
	bool m_bFire5Sec;

	COutputEvent m_OnCountdownStart;
	COutputEvent m_OnCountdown15SecRemain;
	COutputEvent m_OnCountdown10SecRemain;
	COutputEvent m_OnCountdown5SecRemain;
	COutputEvent m_OnCountdownEnd;
};

BEGIN_DATADESC( CCPTimerLogic )
	DEFINE_KEYFIELD( m_iszControlPoint, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_iTimerLength, FIELD_INTEGER, "timer_length" ),
	
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	DEFINE_OUTPUT( m_OnCountdownStart, "OnCountdownStart" ),
	DEFINE_OUTPUT( m_OnCountdown15SecRemain, "OnCountdown15SecRemain" ),
	DEFINE_OUTPUT( m_OnCountdown10SecRemain, "OnCountdown10SecRemain" ),
	DEFINE_OUTPUT( m_OnCountdown5SecRemain, "OnCountdown5SecRemain" ),
	DEFINE_OUTPUT( m_OnCountdownEnd, "OnCountdownEnd" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_cp_timer, CCPTimerLogic );

class CDominationLogic : public CLogicalEntity
{
public:
	DECLARE_CLASS( CDominationLogic, CLogicalEntity );
	DECLARE_DATADESC();

	CDominationLogic();
	~CDominationLogic();

	void	ThinkRed();
	void	ThinkBlue();
	void	ThinkGreen();
	void	ThinkYellow();

	int		GetPointLimitMap( void ){ return m_iPointLimitMap; };
	bool	GetWinOnLimit( void ){ return m_bWinOnLimit; };
	bool	GetKillsGivePoints( void ){ return m_bKillsGivePoints; };
	void	DoScore( int iTeam );
	void	UpdateScoreOuput( int iTeam );
	void	CheckWinner();

	void	OutputPointLimitAny();
	void	OutputPointLimitRed();
	void	OutputPointLimitBlue();
	void	OutputPointLimitGreen();
	void	OutputPointLimitYellow();

	void	InputAddRedPoints( inputdata_t &inputdata );
	void	InputAddBluePoints( inputdata_t &inputdata );
	void	InputAddGreenPoints( inputdata_t &inputdata );
	void	InputAddYellowPoints( inputdata_t &inputdata );
	void	InputSetRedPoints( inputdata_t &inputdata );
	void	InputSetBluePoints( inputdata_t &inputdata );
	void	InputSetGreenPoints( inputdata_t &inputdata );
	void	InputSetYellowPoints( inputdata_t &inputdata );

private:

	int		m_iPointLimitMap;
	int		m_iWinner;
	bool	m_bWinOnLimit;
	bool	m_bKillsGivePoints;

	COutputEvent m_outputOnPointLimitAny;
	COutputEvent m_outputOnPointLimitRed;
	COutputEvent m_outputOnPointLimitBlue;
	COutputEvent m_outputOnPointLimitGreen;
	COutputEvent m_outputOnPointLimitYellow;

	COutputInt m_outputOnRedScoreChanged;
	COutputInt m_outputOnBlueScoreChanged;
	COutputInt m_outputOnGreenScoreChanged;
	COutputInt m_outputOnYellowScoreChanged;
};

LINK_ENTITY_TO_CLASS( tf_logic_domination, CDominationLogic );

CDominationLogic *g_pDominationLogic = NULL;

BEGIN_DATADESC( CDominationLogic )

	DEFINE_THINKFUNC( ThinkRed ),
	DEFINE_THINKFUNC( ThinkBlue ),
	DEFINE_THINKFUNC( ThinkGreen ),
	DEFINE_THINKFUNC( ThinkYellow ),

	DEFINE_KEYFIELD( m_iPointLimitMap, FIELD_INTEGER, "point_limit" ),
	DEFINE_KEYFIELD( m_bWinOnLimit, FIELD_BOOLEAN, "win_on_limit" ),
	DEFINE_KEYFIELD( m_bKillsGivePoints, FIELD_BOOLEAN, "kills_give_points" ),

	// Inputs.

	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedPoints", InputAddRedPoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBluePoints", InputAddBluePoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddGreenPoints", InputAddGreenPoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddYellowPoints", InputAddYellowPoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedPoints", InputSetRedPoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBluePoints", InputSetBluePoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenPoints", InputSetGreenPoints ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowPoints", InputSetYellowPoints ),

	// Outputs.

	DEFINE_OUTPUT( m_outputOnPointLimitAny, "OnPointLimitAny" ),
	DEFINE_OUTPUT( m_outputOnPointLimitRed, "OnPointLimitRed" ),
	DEFINE_OUTPUT( m_outputOnPointLimitBlue, "OnPointLimitBlue" ),
	DEFINE_OUTPUT( m_outputOnPointLimitGreen, "OnPointLimitGreen" ),
	DEFINE_OUTPUT( m_outputOnPointLimitYellow, "OnPointLimitYellow" ),

	DEFINE_OUTPUT( m_outputOnRedScoreChanged, "OnRedScoreChanged" ),
	DEFINE_OUTPUT( m_outputOnBlueScoreChanged, "OnBlueScoreChanged" ),
	DEFINE_OUTPUT( m_outputOnGreenScoreChanged, "OnGreenScoreChanged" ),
	DEFINE_OUTPUT( m_outputOnYellowScoreChanged, "OnYellowScoreChanged" ),

END_DATADESC()

CDominationLogic::CDominationLogic()
{
	m_iPointLimitMap = 100;
	m_bWinOnLimit = true;
	m_iWinner = TEAM_UNASSIGNED;
	m_bKillsGivePoints = false;

	g_pDominationLogic = this;

	RegisterThinkContext( "ThinkRedContext" );
	RegisterThinkContext( "ThinkBlueContext" );
	RegisterThinkContext( "ThinkGreenContext" );
	RegisterThinkContext( "ThinkYellowContext" );
	SetContextThink( &CDominationLogic::ThinkRed, gpGlobals->curtime, "ThinkRedContext" );
	SetContextThink( &CDominationLogic::ThinkBlue, gpGlobals->curtime, "ThinkBlueContext" );
	SetContextThink( &CDominationLogic::ThinkGreen, gpGlobals->curtime, "ThinkGreenContext" );
	SetContextThink( &CDominationLogic::ThinkYellow, gpGlobals->curtime, "ThinkYellowContext" );
}

CDominationLogic::~CDominationLogic()
{
	g_pDominationLogic = NULL;
}

// Timers for point gathering

void CDominationLogic::DoScore( int iTeam )
{
	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( pTeam && pTeam->GetControlPointsOwned() > 0 && ( pTeam->GetDominationPointRate() > 0 ) && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		pTeam->AddRoundScore( 1 );
		UpdateScoreOuput( iTeam );

		// Gain 1 every ( 1 / Sum of CP gain ) seconds
		// 1 CP gain = 1 point per 5 seconds

		float flPointRate = (float)pTeam->GetDominationPointRate() / 5.0f;
		float flPointInterval = ( 1.0f / flPointRate );

		// DevMsg( "Think %i %f\n", pTeam->GetControlPointsOwned(), fPointInterval );
		// Old point rate: 1 every ( 5 / Number of CPs ) seconds
		// float fPointInterval = ( 5.0 / pTeam->GetControlPointsOwned() );

		switch ( iTeam )
		{
			case TF_TEAM_RED:
				SetNextThink( gpGlobals->curtime + flPointInterval, "ThinkRedContext" );
				break;
			case TF_TEAM_BLUE:
				SetNextThink( gpGlobals->curtime + flPointInterval, "ThinkBlueContext" );
				break;
			case TF_TEAM_YELLOW:
				SetNextThink( gpGlobals->curtime + flPointInterval, "ThinkYellowContext" );
				break;
			case TF_TEAM_GREEN:
				SetNextThink( gpGlobals->curtime + flPointInterval, "ThinkGreenContext" );
				break;
			case TEAM_UNASSIGNED:
			default:
				break;
		}
	}
	else
	{
		// Update to catch changes.
		switch ( iTeam )
		{
			case TF_TEAM_RED:
				SetNextThink( gpGlobals->curtime + 0.5f, "ThinkRedContext" );
				break;
			case TF_TEAM_BLUE:
				SetNextThink( gpGlobals->curtime + 0.5f, "ThinkBlueContext" );
				break;
			case TF_TEAM_YELLOW:
				SetNextThink( gpGlobals->curtime + 0.5f, "ThinkYellowContext" );
				break;
			case TF_TEAM_GREEN:
				SetNextThink( gpGlobals->curtime + 0.5f, "ThinkGreenContext" );
				break;
			case TEAM_UNASSIGNED:
			default:
				break;
		}
	}

	if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
		return;

	CheckWinner();
}

void CDominationLogic::UpdateScoreOuput(int iTeam)
{
	CTFTeam* pTeam = GetGlobalTFTeam( iTeam );
	if ( pTeam )
	{
		switch ( iTeam )
		{
			case TF_TEAM_RED:
				m_outputOnRedScoreChanged.Set( pTeam->GetRoundScore(), this, this );
				break;
			case TF_TEAM_BLUE:
				m_outputOnBlueScoreChanged.Set( pTeam->GetRoundScore(), this, this );
				break;
			case TF_TEAM_YELLOW:
				m_outputOnYellowScoreChanged.Set( pTeam->GetRoundScore(), this, this );
				break;
			case TF_TEAM_GREEN:
				m_outputOnGreenScoreChanged.Set( pTeam->GetRoundScore(), this, this );
				break;
			default: break;
		}
	}
}

void CDominationLogic::CheckWinner()
{
	int iTeamCount = TFGameRules()->IsFourTeamGame() ? TF_TEAM_COUNT : TF_TEAM_COUNT - 2;

	for ( int i = TF_TEAM_RED; i < iTeamCount; i++ )
	{
		// Check point limit.

		if ( TFGameRules()->GetPointLimit() > 0 )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( i );
			if ( pTeam && pTeam->GetRoundScore() >= TFGameRules()->GetPointLimit() )
			{
				m_iWinner = pTeam->GetTeamNumber();

				OutputPointLimitAny();

				switch ( m_iWinner )
				{
					case TF_TEAM_RED:
						OutputPointLimitRed();
						break;
					case TF_TEAM_BLUE:
						OutputPointLimitBlue();
						break;
					case TF_TEAM_GREEN:
						OutputPointLimitGreen();
						break;
					case TF_TEAM_YELLOW:
						OutputPointLimitYellow();
						break;
				}
			}
		}

	}

	if ( m_iWinner != TEAM_UNASSIGNED )
	{
		// Maps can prevent a win.
		if ( GetWinOnLimit() )
		{
			TFGameRules()->SetWinningTeam( m_iWinner, WINREASON_ROUNDSCORELIMIT );
		}
	}
}

void CDominationLogic::ThinkRed()
{
	DoScore( TF_TEAM_RED );
}

void CDominationLogic::ThinkBlue()
{
	DoScore( TF_TEAM_BLUE );
}

void CDominationLogic::ThinkGreen()
{
	DoScore( TF_TEAM_GREEN );
}

void CDominationLogic::ThinkYellow()
{
	DoScore( TF_TEAM_YELLOW );
}

// Add points input

void CDominationLogic::InputAddRedPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_RED ) ;
	if ( pTeam )
	{
		pTeam->AddRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_RED );
	}
}

void CDominationLogic::InputAddBluePoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE ) ;
	if ( pTeam )
	{
		pTeam->AddRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_BLUE );
	}
}

void CDominationLogic::InputAddGreenPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_GREEN ) ;
	if ( pTeam )
	{
		pTeam->AddRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_GREEN );
	}
}

void CDominationLogic::InputAddYellowPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW ) ;
	if ( pTeam )
	{
		pTeam->AddRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_YELLOW );
	}
}

// Set points input

void CDominationLogic::InputSetRedPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_RED ) ;
	if ( pTeam )
	{
		pTeam->SetRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_RED );
	}
}

void CDominationLogic::InputSetBluePoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_BLUE );
	}
}

void CDominationLogic::InputSetGreenPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
	if ( pTeam )
	{
		pTeam->SetRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_GREEN );
	}
}

void CDominationLogic::InputSetYellowPoints( inputdata_t &inputdata )
{
	CTFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
	if ( pTeam )
	{
		pTeam->SetRoundScore( inputdata.value.Int() );
		UpdateScoreOuput( TF_TEAM_YELLOW );
	}
}

// Outputs

void CDominationLogic::OutputPointLimitAny()
{
	m_outputOnPointLimitAny.FireOutput( this, this );
}

void CDominationLogic::OutputPointLimitRed()
{
	m_outputOnPointLimitRed.FireOutput( this, this );
}

void CDominationLogic::OutputPointLimitBlue()
{
	m_outputOnPointLimitBlue.FireOutput( this, this );
}

void CDominationLogic::OutputPointLimitGreen()
{
	m_outputOnPointLimitGreen.FireOutput( this, this );
}

void CDominationLogic::OutputPointLimitYellow()
{
	m_outputOnPointLimitYellow.FireOutput( this, this );
}

// End of Domination logic

#ifdef TF_INFILTRATION
class CInfiltrationLogic : public CLogicalEntity
{
public:
	DECLARE_CLASS( CInfiltrationLogic, CLogicalEntity );
	DECLARE_DATADESC();
	CInfiltrationLogic()
	{
		m_iPointLimit = 0;
	}
	float DoScore( int iTeam );
	void ThinkRed();
	void ThinkBlue();
	void ThinkGreen();
	void ThinkYellow();
	void Spawn();
private:
	int m_iPointLimit;
};

LINK_ENTITY_TO_CLASS( tf_logic_infiltration, CInfiltrationLogic );

BEGIN_DATADESC( CInfiltrationLogic )
DEFINE_KEYFIELD( m_iPointLimit, FIELD_INTEGER, "point_limit" ),
DEFINE_THINKFUNC( Think ),
END_DATADESC();

void CInfiltrationLogic::Spawn()
{
	SetContextThink( &CInfiltrationLogic::ThinkRed, gpGlobals->curtime, "ThinkRed" );
	SetContextThink( &CInfiltrationLogic::ThinkBlue, gpGlobals->curtime, "ThinkBlue" );
	SetContextThink( &CInfiltrationLogic::ThinkGreen, gpGlobals->curtime, "ThinkGreen" );
	SetContextThink( &CInfiltrationLogic::ThinkYellow, gpGlobals->curtime, "ThinkYellow" );
}

float CInfiltrationLogic::DoScore( int iTeam )
{
	//DevMsg( "Do score for team: %i\n", iTeam );
	int iTotalWorkers = 0;

	for( CWorkerZone *zone : CWorkerZone::AutoList() )
	{
		if( zone->GetTeamNumber() == iTeam )
			iTotalWorkers += zone->GetActiveWorkers();
	}
	
	float m_flTime = 0.5f;

	if( iTotalWorkers > 0 )
	{
		//TODO: Add 1 to the score of team iTeam;
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if( pTeam )
		{
			pTeam->AddRoundScore( 1 );
			DevMsg( "Score = %i Team: %i\n", pTeam->GetRoundScore(), iTeam );
		}

		m_flTime = ( 1.0 / iTotalWorkers ) * 5.0; //5.0 is a factor to increase time between points.
		DevMsg( "m_flTime = %f Team: %i\n", m_flTime, iTeam );
	}
	
	return m_flTime;
}

void CInfiltrationLogic::ThinkRed()
{
	SetNextThink( gpGlobals->curtime + DoScore( TF_TEAM_RED ), "ThinkRed" );
}

void CInfiltrationLogic::ThinkBlue()
{
	SetNextThink( gpGlobals->curtime + DoScore( TF_TEAM_BLUE ), "ThinkBlue" );
}

void CInfiltrationLogic::ThinkGreen()
{
	SetNextThink( gpGlobals->curtime + DoScore( TF_TEAM_GREEN ), "ThinkGreen" );
}

void CInfiltrationLogic::ThinkYellow()
{
	SetNextThink( gpGlobals->curtime + DoScore( TF_TEAM_YELLOW ), "ThinkYellow" );
}
#endif
// End of infiltration logic.

LINK_ENTITY_TO_CLASS( tf_logic_medieval, CLogicalEntity );
LINK_ENTITY_TO_CLASS( tf_logic_multiple_escort, CLogicalEntity );
LINK_ENTITY_TO_CLASS( tf_logic_hybrid_ctf_cp, CLogicalEntity );
LINK_ENTITY_TO_CLASS( tf_logic_competitive, CLogicalEntity );
// LINK_ENTITY_TO_CLASS( tf_logic_siege, CLogicalEntity ); // Possibly for a future update.

#define SF_TF_DYNAMICPROP_GRENADE_COLLISION			512
class CTFTrainingDynamicProp : public CDynamicProp
{
	DECLARE_CLASS( CTFTrainingDynamicProp, CDynamicProp );
};

LINK_ENTITY_TO_CLASS( training_prop_dynamic, CTFTrainingDynamicProp );

bool PropDynamic_CollidesWithGrenades( CBaseEntity *pBaseEntity )
{
	CTFTrainingDynamicProp *pTrainingDynamicProp = dynamic_cast<CTFTrainingDynamicProp *>( pBaseEntity );
	return ( pTrainingDynamicProp && pTrainingDynamicProp->HasSpawnFlags( SF_TF_DYNAMICPROP_GRENADE_COLLISION ) );
}
#endif

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade", "0" );		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}


int CTFGameRules::Damage_GetTimeBased( void )
{
	return ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
}


int CTFGameRules::Damage_GetShowOnHud( void )
{
	return ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
}


int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

#ifdef GAME_DLL
unsigned char g_aAuthDataKey[8] = { 0x38, 0x46, 0x66, 0x37, 0x51, 0x73, 0x4E, 0x47 };
unsigned char g_aAuthDataXOR[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif


CTFGameRules::CTFGameRules()
{
#ifdef GAME_DLL
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_flNextRespawnWave.Set( i, 0.0f );
		m_TeamRespawnWaveTimes.Set( i, -1.0f );
		m_flOriginalTeamRespawnWaveTime[i] = -1.0f;
	}

	m_bInOvertime = false;
	m_bInSetup = false;
	m_bSwitchedTeamsThisRound = false;
	m_flStopWatchTotalTime = -1.0f;
	m_bMultipleTrains = false;
	m_bAllowBetweenRounds = true;

	ListenForGameEvent( "server_changelevel_failed" );

	m_pCurStateInfo = NULL;
	State_Transition( GR_STATE_PREGAME );

	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	InitTeams();
	ResetScores();
	SetForceMapReset( true );
	SetRoundToPlayNext( NULL_STRING );
	m_bInWaitingForPlayers = false;
	m_bAwaitingReadyRestart = false;
	m_flRestartRoundTime = -1.0f;
	m_flMapResetTime = 0.0f;
	m_bPrevRoundWasWaitingForPlayers = false;
	m_iWinningTeam = TEAM_UNASSIGNED;

	m_iszPreviousRounds.RemoveAll();
	SetFirstRoundPlayed( NULL_STRING );

	m_bAllowStalemateAtTimelimit = false;
	m_bChangelevelAfterStalemate = false;
	m_flRoundStartTime = 0.0f;
	m_flNewThrottledAlertTime = 0.0f;
	m_flStartBalancingTeamsAt = 0.0f;
	m_bPrintedUnbalanceWarning = false;
	m_flFoundUnbalancedTeamsTime = -1.0f;
	m_flWaitingForPlayersTimeEnds = -1.0f;
	m_flLastTeamWin = -1.0f;

	m_nRoundsPlayed = 0;
	m_bUseAddScoreAnim = false;

	m_bStopWatch = false;
	m_bAwaitingReadyRestart = false;

	if ( IsInTournamentMode() )
	{
		m_bAwaitingReadyRestart = true;
	}

	m_flAutoBalanceQueueTimeEnd = -1.0f;
	m_nAutoBalanceQueuePlayerIndex = -1;
	m_nAutoBalanceQueuePlayerScore = -1;

	m_bCheatsEnabledDuringLevel = false;

	ResetPlayerAndTeamReadyState();

	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );	
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "player_escort_score" );
	ListenForGameEvent( "vip_death" );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;

	m_bFirstBlood = false;
	m_iArenaTeamCount = 0;
	m_flNextArenaNotify = 0.0f;

	m_flCTFBonusTime = -1;
	m_flTimerMayExpireAt = -1.0f;

	m_bVoteMapOnNextRound = false;
	m_bVotedForNextMap = false;
	m_iMapTimeBonus = 0;
	m_iMaxRoundsBonus = 0;
	m_iWinLimitBonus = 0;

	m_flNextDominationThink = 0.0f;
	m_flNextVoteThink = 0.0f;

	m_bNeedSteamServerCheck = false;

	m_bPreRoundFreezeTime = true;

	// Lets execute a map specific cfg file.
	// ** execute this after server.cfg!
	char szCommand[32];
	V_sprintf_safe( szCommand, "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

	// Load 'authenticated' data.
	unsigned char szPassword[8];
	V_memcpy( szPassword, g_aAuthDataKey, sizeof( szPassword ) );
	for ( unsigned int i = 0; i < sizeof( szPassword ); ++i )
	{
		szPassword[i] ^= g_aAuthDataXOR[i] ^ 0x00;
	}

	if (m_pAuthData)
	{
		m_pAuthData->deleteThis();
		m_pAuthData = NULL;
	}
	m_pAuthData = ReadEncryptedKVFile( filesystem, "scripts/authdata", szPassword, true );
	V_memset( szPassword, 0x00, sizeof( szPassword ) );
#else // GAME_DLL
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "overtime_nag" );
	ListenForGameEvent( "arrow_impact" );

	// Using our own voice bubble.
	GetClientVoiceMgr()->SetHeadLabelsDisabled( true );
#endif

	m_flCapturePointEnableTime = 0;

	// Initialize the game type.
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	// Set turbo physics on, and do it here for now.
	sv_turbophysics.SetValue( 1 );

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[TF_WEAPON_COUNT] == TF_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[TF_WEAPON_COUNT], "TF_WEAPON_COUNT" ) );	

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	m_iHolidayMode = 0;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
	m_pszTeamGoalStringGreen.GetForModify()[0] = '\0';
	m_pszTeamGoalStringYellow.GetForModify()[0] = '\0';
}

// Classnames of entities that are preserved across round restarts.
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"point_commentary_node",
	"point_viewcontrol",
	"func_precipitation",
	"func_team_wall",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"commentary_auto",
	"point_commentary_node",
	"point_commentary_viewpoint",
	"bot_roster",
	"info_populator",
	"fog_volume",
	"trigger_tonemap",
	"postprocess_controller",
	"env_dof_controller",

	"tf_gamerules",
	"tf_randomizer_manager",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	//"keyframe_rope",
	//"move_rope",
	"tf_viewmodel",
	"tf_logic_training",
	"tf_logic_training_mode",
	"tf_powerup_bottle",
	"tf_mann_vs_machine_stats",
	"tf_wearable",
	"tf_wearable_demoshield",
	"tf_wearable_robot_arm",
	"tf_wearable_vm",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_medieval",
	"tf_logic_cp_timer",
	"tf_logic_tower_defense",
	"tf_logic_mann_vs_machine",
	"func_upgradestation"
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"entity_suacer",
	"info_ladder",
	"", // END Marker
};


int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints - 1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return 0;

	int iCapSpeed = BaseClass::GetCaptureValueForPlayer( pPlayer );

	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT, true ) )
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			iCapSpeed = 2;
		}
		else
		{
			// Scouts can cap all points on their own.
			iCapSpeed = 10;
		}
	}
	else if ( pTFPlayer->IsPlayerClass( TF_CLASS_CIVILIAN, true ) )
	{
		if ( pTFPlayer->IsVIP() )
		{
			iCapSpeed = tf2c_civilian_capture_rate_vip.GetInt();
		}
		else
		{
			iCapSpeed = tf2c_civilian_capture_rate.GetInt();
		}
	}

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iCapSpeed, add_player_capturevalue );

	return iCapSpeed;
}


bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex )
{
#ifdef CLIENT_DLL
	// Display points as locked during waiting players period.
	if ( IsInWaitingForPlayers() )
		return false;
#endif

	// If the point is explicitly locked it can't be capped.
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	if ( !tf_caplinear.GetBool() )
		return true;

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return ( abs( iFarthestPoint - iPointIndex ) <= 1 );
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			//tagES TFTODO: need to figure out a good algorithm for this
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}


bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Disguised and invisible spies cannot capture points.
	if ( pTFPlayer->m_Shared.IsStealthed() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}

		return false;
	}

	if ( pTFPlayer->m_Shared.IsInvulnerable() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}

		return false;
	}

	if ( pTFPlayer->m_Shared.IsDisguised() && pTFPlayer->m_Shared.GetTrueDisguiseTeam() != pTFPlayer->GetTeamNumber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}

		return false;
	}

	if ( IsVIPMode() )
	{
		CTFPlayer *pVIPPlayer = pTFPlayer->GetTFTeam()->GetVIP();
		if ( pVIPPlayer && pVIPPlayer != pTFPlayer )
		{
			if ( pszReason )
			{
				if ( ObjectiveResource()->GetCPCapPercentage( iPointIndex ) > 0.0f )
				{
					Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_civilian_only_frozen" );
				}
				else
				{
					Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_civilian_only" );
				}
			}

			return false;
		}
	}

	return true;
}


bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Invuln players can block points.
	if ( pTFPlayer->m_Shared.IsInvulnerable() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}

		return true;
	}

	// Teammates of VIP can "block" points, but not capture,
	// this is so they can stop CP progress deterioration.
	if ( IsVIPMode() )
	{
		CTFPlayer *pVIPPlayer = pTFPlayer->GetTFTeam()->GetVIP();
		if ( pVIPPlayer && pVIPPlayer != pTFPlayer )
		{
			if ( pszReason )
			{
				Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_civilian_only" );
			}

			return true;
		}
	}

	return false;
}


bool CTFGameRules::PointsMayBeCaptured( void )
{
	if ( IsInWaitingForPlayers() )
		return false;

	return ( State_Get() == GR_STATE_RND_RUNNING || State_Get() == GR_STATE_STALEMATE );
}

//-----------------------------------------------------------------------------
// Purpose: don't let us spawn before our freezepanel time would have ended, even if we skip it
//-----------------------------------------------------------------------------
float CTFGameRules::GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer )
{
	if ( State_Get() == GR_STATE_STALEMATE )
		return 0;

	// If we are purely checking when the next respawn wave is for this team
	if ( !pPlayer )
		return m_flNextRespawnWave[iTeam];

	// The soonest this player may spawn
	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer );
	if ( ShouldRespawnQuickly( pPlayer ) )
		return flMinSpawnTime;

	// the next scheduled respawn wave time
	float flNextRespawnTime = m_flNextRespawnWave[iTeam];

	// the length of one respawn wave. We'll check in increments of this
	float flRespawnWaveMaxLen = GetRespawnWaveMaxLength( iTeam );
	if ( flRespawnWaveMaxLen <= 0 )
		return flNextRespawnTime;

	// Keep adding the length of one respawn until we find a wave that
	// this player will be eligible to spawn in.
	while ( flNextRespawnTime < flMinSpawnTime )
	{
		flNextRespawnTime += flRespawnWaveMaxLen;
	}

	return flNextRespawnTime;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer );
	return ( gpGlobals->curtime > flMinSpawnTime );
}


float CTFGameRules::GetMinTimeWhenPlayerMaySpawn( CBasePlayer *pPlayer )
{
	// Min respawn time is the sum of
	//
	// a) the length of one full *unscaled* respawn wave for their team
	//		and
	// b) death anim length + freeze panel length

	float flDeathAnimLength = 2.0f + spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();

	// TF2C respawn time overrides
	// TODO: trigger_player_respawn_override brush
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->GetRespawnTimeOverride() >= 0.0f )
		return pPlayer->GetDeathTime() + flDeathAnimLength + pTFPlayer->GetRespawnTimeOverride();

	float fMinDelay = flDeathAnimLength;
	if ( !ShouldRespawnQuickly( pPlayer ) )
	{
		fMinDelay += GetRespawnWaveMaxLength( pPlayer->GetTeamNumber(), false );
	}

	return pPlayer->GetDeathTime() + fMinDelay;
}


void CTFGameRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	m_bCheatsEnabledDuringLevel = sv_cheats && sv_cheats->GetBool();
#endif // GAME_DLL
}


float CTFGameRules::GetRespawnTimeScalar( int iTeam )
{
	// For long respawn times, scale the time as the number of players drops.
	// 16 players total, 8 per team.
	return RemapValClamped( GetGlobalTeam( iTeam )->GetNumPlayers(), 1, IsFourTeamGame() ? 4 : 8, 0.25f, 1.0f );
}

float CTFGameRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers /*= true*/ )
{
	if ( State_Get() != GR_STATE_RND_RUNNING )
		return 0.0f;

	if ( mp_disable_respawn_times.GetBool() )
		return 0.0f;

	// Let's just turn off respawn times while players are messing around waiting for the tournament to start.
	if ( IsInTournamentMode() && IsInPreMatch() )
		return 0.0f;

	// For long respawn times, scale the time as the number of players drops.
	float flTime = ( m_TeamRespawnWaveTimes[iTeam] >= 0 ? m_TeamRespawnWaveTimes[iTeam] : mp_respawnwavetime.GetFloat() );

	// TF2C Domination mode scales respawn times based on team score delta
	if ( TFGameRules() && TFGameRules()->IsInDominationMode() )
	{
		int iHighestScore = -1;
		int iTeamInLead = -1;

		// Collect all team scores to compare against the leader.
		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			int iScore = GetGlobalTFTeam( i )->GetRoundScore();
			if ( iScore > iHighestScore )
			{
				iHighestScore = iScore;
				iTeamInLead = GetGlobalTFTeam( i )->GetTeamNumber();
			}
		}

		// Only kick in once 20% of goal has been reached
		int iGoalScore = TFGameRules()->GetPointLimit();
		if ( iHighestScore >= ( iGoalScore * 0.2 ) )
		{
			int iScoreDelta = iHighestScore - GetGlobalTFTeam( iTeam )->GetRoundScore();
			// If our score difference is 0, respawn at normal speed. (We're the leader!)
			// If our score diff is half the goal score or more, respawn asap.
			// Linear scaling between the two extremes. Note: 5s is still the limit due to freezecam etc.
			float flRespawnTimeFactor = RemapValClamped( iScoreDelta, 0.0f, (float)iGoalScore * 0.5f, 1.0, 0.0 );
			flTime = flTime * flRespawnTimeFactor;
			DevMsg( "Team %i: Respawn Time Factor: %2.2f, Score Delta: %i\n", iTeam, flRespawnTimeFactor, iScoreDelta );
		}
	}

	if ( bScaleWithNumPlayers && flTime > 5.0f )
	{
		flTime = Max( 5.0f, flTime * GetRespawnTimeScalar( iTeam ) );
	}

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if we are running tournament mode.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsInTournamentMode( void )
{
	return mp_tournament.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if we are running highlander mode.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsInHighlanderMode( void )
{
	// Can't use highlander mode and the queue system.
	if ( IsInArenaQueueMode() )
		return false;

	return mp_highlander.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if we should even bother to do balancing stuff.
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldBalanceTeams( void )
{
	if ( IsInTournamentMode() )
		return false;

	if ( IsInTraining() || IsInItemTestingMode() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the passed team change would cause unbalanced teams.
//-----------------------------------------------------------------------------
bool CTFGameRules::WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam )
{
	// Players are allowed to change to their own team.
	if ( iNewTeam == iCurrentTeam )
		return false;

	// If mp_teams_unbalance_limit is 0, don't check.
	if ( !ShouldBalanceTeams() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	// If they are joining a non-playing team, allow.
	if ( iNewTeam < FIRST_GAME_TEAM )
		return false;

	CTeam *pNewTeam = GetGlobalTeam( iNewTeam );
	if ( !pNewTeam )
	{
		// tf_teammenu.cpp | CTFTeamButton::IsTeamFull() is spamming the consoles with asserts when calling this function
		// commenting this out for now for those with debug binaries.
		//Assert( 0 );
		return true;
	}

	// Add one because we're joining this team.
	int iNewTeamPlayers = pNewTeam->GetNumPlayers() + 1;

#ifndef CLIENT_DLL
	CTFTeam *pNewTFTeam = dynamic_cast<CTFTeam *>( pNewTeam );
#else
	C_TFTeam *pNewTFTeam = dynamic_cast<C_TFTeam *>( pNewTeam );
#endif
	if ( IsVIPMode() && pNewTFTeam && pNewTFTeam->GetVIP() )
	{
		iNewTeamPlayers = iNewTeamPlayers == 1 ? 0 : iNewTeamPlayers - 1;
	}

	// For each game team.
	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		if ( pTeam == pNewTeam )
			continue;

		int iNumPlayers = pTeam->GetNumPlayers();

		// Don't count VIPs.
#ifndef CLIENT_DLL
		CTFTeam *pTFTeam = dynamic_cast< CTFTeam * >( pTeam );
#else
		C_TFTeam *pTFTeam = dynamic_cast< C_TFTeam * >( pTeam );
#endif
		if ( IsVIPMode() && pTFTeam && pTFTeam->GetVIP() )
		{
			iNumPlayers = iNumPlayers == 1 ? 0 : iNumPlayers - 1;
		}

		if ( i == iCurrentTeam )
		{
			iNumPlayers = Max( 0, iNumPlayers - 1 );
		}

		if ( ( iNewTeamPlayers - iNumPlayers ) > mp_teams_unbalance_limit.GetInt() )
			return true;
	}

	return false;
}


bool CTFGameRules::AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam )
{
	if ( !IsInArenaQueueMode() && !ShouldBalanceTeams() )
		return false;

#ifndef CLIENT_DLL
	if ( IsInCommentaryMode() )
		return false;
#endif

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		int iNumPlayers = pTeam->GetNumPlayers();

		// Don't count VIPs.
#ifndef CLIENT_DLL
		CTFTeam *pTFTeam = dynamic_cast<CTFTeam *>( pTeam );
#else
		C_TFTeam *pTFTeam = dynamic_cast<C_TFTeam *>( pTeam );
#endif
		if ( IsVIPMode() && pTFTeam && pTFTeam->GetVIP() )
		{
			iNumPlayers = iNumPlayers == 1 ? 0 : iNumPlayers - 1;
		}

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;
		}
	}

	if ( IsInArenaQueueMode() )
	{
		if ( iMostPlayers == 0 && iMostPlayers == iLeastPlayers )
			return true;

		if ( iMostPlayers != iLeastPlayers )
			return true;

		return false;
	}

	if ( ( iMostPlayers - iLeastPlayers ) > mp_teams_unbalance_limit.GetInt() )
		return true;

	return false;
}


int CTFGameRules::GetBonusRoundTime( bool bFinal /*= false*/ )
{
	return bFinal ? mp_bonusroundtime_final.GetInt() : Max( 5, mp_bonusroundtime.GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) +
					( pRoundStats->m_iStat[TFSTAT_DAMAGE] / TF_SCORE_DAMAGE_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TF_SCORE_CAPTURE ) +
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] * TF_SCORE_REVENGE ) +
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) +
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] / TF_SCORE_HEADSHOTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_HEALING] / TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] * TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) +
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] / TF_SCORE_DAMAGE_ASSIST_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] / TF_SCORE_DAMAGE_BLOCKED_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_JUMPPAD_JUMPS] / TF_SCORE_JUMPPAD_JUMPS_PER_POINT );

	return Max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int	CTFGameRules::CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIndex )
{
#ifdef GAME_DLL
	int iSupportScore =		( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] ) +
							( pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] ) +
							( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] * 0); // intended, we decided so
#else
	int iSupportScore =		( g_TF_PR->GetDamageAssist( iPlayerIndex ) ) +
							( g_TF_PR->GetDamageBlocked( iPlayerIndex ) ) +
							( g_TF_PR->GetBonusPoints( iPlayerIndex ) * 0 );
#endif

	return Max( iSupportScore, 0 );
}


bool CTFGameRules::IsBirthday( void )
{
	if ( m_iBirthdayMode == BIRTHDAY_RECALCULATE )
	{
		m_iBirthdayMode = BIRTHDAY_OFF;
		if ( tf_birthday.GetBool() || IsHolidayActive( TF_HOLIDAY_BIRTHDAY ) )
		{
			m_iBirthdayMode = BIRTHDAY_ON;
		}
		else
		{
			time_t ltime = time( 0 );
			const time_t *ptime = &ltime;
			struct tm *today = localtime( ptime );
			if ( today )
			{
				if ( today->tm_mon == 7 && today->tm_mday == 24 )
				{
					m_iBirthdayMode = BIRTHDAY_ON;
				}
			}
		}
	}

	return m_iBirthdayMode == BIRTHDAY_ON;
}


bool CTFGameRules::IsHolidayActive( int eHoliday ) const
{
	if ( m_iHolidayMode == eHoliday || eHoliday == tf_forced_holiday.GetInt() )
		return true;

	return false;
}

bool CTFGameRules::IsHolidayMap( int eHoliday )
{
	return false;
}

static const char *g_pszOfficialMaps[] =
{
	"ctf_2fort",
	"ctf_doublecross",
	"ctf_landfall",
	"ctf_sawmill",
	"ctf_turbine",
	"ctf_well",
	"cp_5gorge",
	"cp_badlands",
	"cp_coldfront",
	"cp_fastlane",
	"cp_foundry",
	"cp_freight_final1",
	"cp_granary",
	"cp_gullywash",
	"cp_powerhouse",
	"cp_well",
	"cp_yukon_final",
	"cp_dustbowl",
	"cp_egypt_final",
	"cp_gorge",
	"cp_gravelpit",
	"cp_junction_final",
	"cp_mountainlab",
	"cp_steel",
	"cp_degrootkeep",
	"tc_hydro",
	"pl_badwater",
	"pl_barnblitz",
	"pl_frontier_final",
	"pl_goldrush",
	"pl_hoodoo_final",
	"pl_thundermountain",
	"pl_upward",
	"plr_hightower",
	"plr_nightfall_final",
	"plr_pipeline",
	"arena_badlands",
	"arena_granary",
	"arena_lumberyard",
	"arena_nucleus",
	"arena_offblast_final",
	"arena_ravine",
	"arena_sawmill",
	"arena_watchtower",
	"arena_well",
	"koth_badlands",
	"koth_harvest_final",
	"koth_harvest_event",
	"koth_highpass",
	"koth_king",
	"koth_lakeside_final",
	"koth_nucleus",
	"koth_sawmill",
	"koth_viaduct",
	"sd_doomsday",
	"itemtest",

	// Put official TF2C maps here.
	"vip_harbor",
	"vip_mineside",
	"vip_badwater",
	"vip_trainyard",
	"dom_oilcanyon",
	"dom_hydro",
	"ctf_casbah",
	"cp_amaranth",
	"cp_furnace_rc",
	"cp_tidal_v4",
	"arena_flask",

	// 2.0.4
	"ctf_pelican_peak",

	// 2.1.0
	"arena_floodgate",
	"dom_krepost"
	"koth_frigid",
	"pl_jinn",
	"td_caper",
};


bool CTFGameRules::IsValveMap( void )
{
#ifdef CLIENT_DLL
	char mapname[MAX_MAP_NAME];
	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );
	const char *pszMapName = mapname;
#else
	const char *pszMapName = STRING( gpGlobals->mapname );
#endif
	for ( int i = 0, c = ARRAYSIZE( g_pszOfficialMaps ); i < c; i++ )
	{
		if ( !V_strcmp( pszMapName, g_pszOfficialMaps[i] ) )
			return true;
	}

	return false;
}

#ifdef GAME_DLL

bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested.
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint++ )
	{
		if ( ObjectiveResource()->GetCappingTeam( iPoint ) )
		{
			m_flTimerMayExpireAt = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	if ( m_flTimerMayExpireAt >= gpGlobals->curtime )
		return false;

	// team_train_watchers can also prevent timer expiring (overtime).
	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->TimerMayExpire() )
			return false;

		pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
	{
		if ( !pFlag->IsHome() && m_bCTF_Overtime )
			return false;
	}

	for ( CCPTimerLogic *pTimerLogic : CCPTimerLogic::AutoList() )
	{
		if ( !pTimerLogic->TimerMayExpire() )
			return false;
	}

	return BaseClass::TimerMayExpire();
}

// Sort functor for the list of players that we're going to use to scramble the teams.
static int ScramblePlayersSort( CTFPlayer *const *p1, CTFPlayer *const *p2 )
{
	return ( *p2 )->GetTeamScrambleScore() - ( *p1 )->GetTeamScrambleScore();
}

// Sort functor used by arena queue code.
static int SortPlayerSpectatorQueue( CTFPlayer *const *lhs, CTFPlayer *const *rhs )
{
	float tBalanceLHS = ( *lhs )->GetArenaBalanceTime();
	float tBalanceRHS = ( *rhs )->GetArenaBalanceTime();
	if ( tBalanceLHS > tBalanceRHS )
		return -1;

	if ( tBalanceLHS < tBalanceRHS )
		return 1;

	float tConnLHS = ( *lhs )->GetConnectionTime();
	float tConnRHS = ( *rhs )->GetConnectionTime();
	if ( tConnLHS > tConnRHS )
		return -1;

	if ( tConnLHS < tConnRHS )
		return 1;

	return 0;
}


void CTFGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;
	CUtlVector<CTFPlayer *> vecListPlayers;

	// Add all the players (that are on blue or red) to our temp list.
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer || pTFPlayer->IsHLTV() || pTFPlayer->IsReplay() )
			continue;

		if ( pTFPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			continue;
		
		vecListPlayers.AddToHead( pTFPlayer );
	}

	// Sort the list.
	if ( mp_scrambleteams_mode.GetInt() == TF_SCRAMBLEMODE_RANDOM )
	{
		vecListPlayers.Shuffle();
	}
	else
	{
		FOR_EACH_VEC( vecListPlayers, i )
		{
			vecListPlayers[i]->CalculateTeamScrambleScore();
		}

		vecListPlayers.Sort( ScramblePlayersSort );
	}

	// Loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly).
	FOR_EACH_VEC( vecListPlayers, i )
	{
		vecListPlayers[i]->ForceChangeTeam( TEAM_SPECTATOR );
		vecListPlayers[i]->RemoveNemesisRelationships( false );
	}

	// Loop through and auto team everyone.
	FOR_EACH_VEC( vecListPlayers, i )
	{
		vecListPlayers[i]->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
	}

	if ( IsVIPMode() )
	{
		AssignTeamVIPs();
	}

	ResetTeamsRoundWinTracking();
}


void CTFGameRules::HandleSwitchTeams( void )
{
	// Don't do this if we're about to scramble.
	if ( ShouldScrambleTeams() )
		return;

	// Respawn the players.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
			continue;

		if ( IsVIPMode() && pPlayer->IsVIP() )
		{
			// Skip previous VIP so they don't join enemy team, e.g.
			// BLU 6 + 1 Civ	->	RED 7
			// RED 6			->	BLU 5 + 1 new Civ
			// results in an imbalanced team state.
			// They get to play as non-VIP this round instead.
			continue;
		}

		// 4Team rotates teams
		if ( IsFourTeamGame() )
		{
			int iTeamToJoin = NULL;
			switch ( pPlayer->GetTeamNumber() )
			{
			case TF_TEAM_RED:
				iTeamToJoin = TF_TEAM_BLUE; break;
			case TF_TEAM_BLUE:
				iTeamToJoin = TF_TEAM_GREEN; break;
			case TF_TEAM_GREEN:
				iTeamToJoin = TF_TEAM_YELLOW; break;
			case TF_TEAM_YELLOW:
				iTeamToJoin = TF_TEAM_RED; break;
			}

			if ( iTeamToJoin != NULL )
			{
				pPlayer->ForceChangeTeam( iTeamToJoin );
			}
		}
		else
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED );
			}
		}
	}

	// Switch the team scores.
	// 4Team rotates teams
	if ( IsFourTeamGame() )
	{
		CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
		CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		CTFTeam *pGreenTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
		CTFTeam *pYellowTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
		if ( pRedTeam && pBlueTeam && pGreenTeam && pYellowTeam )
		{
			int nRed = pRedTeam->GetScore();
			int nBlue = pBlueTeam->GetScore();
			int nGreen = pGreenTeam->GetScore();
			int nYellow = pYellowTeam->GetScore();
			pRedTeam->SetScore( nYellow );
			pBlueTeam->SetScore( nRed );
			pGreenTeam->SetScore( nBlue );
			pYellowTeam->SetScore( nGreen );

			nRed = pRedTeam->GetWinCount();
			nBlue = pBlueTeam->GetWinCount();
			nGreen = pGreenTeam->GetWinCount();
			nYellow = pYellowTeam->GetWinCount();

			pRedTeam->SetWinCount( nYellow );
			pBlueTeam->SetWinCount( nRed );
			pGreenTeam->SetWinCount( nBlue );
			pYellowTeam->SetWinCount( nGreen );
		}
	}
	else
	{
		CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
		CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		if ( pRedTeam && pBlueTeam )
		{
			int nRed = pRedTeam->GetScore();
			int nBlue = pBlueTeam->GetScore();
			pRedTeam->SetScore( nBlue );
			pBlueTeam->SetScore( nRed );

			nRed = pRedTeam->GetWinCount();
			nBlue = pBlueTeam->GetWinCount();

			pRedTeam->SetWinCount( nBlue );
			pBlueTeam->SetWinCount( nRed );
		}
	}

	if ( IsVIPMode() )
	{
		AssignTeamVIPs();
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_TeamsSwitched" );
}


bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( !stricmp( STRING( pEnt->GetEntityName() ), "map_additional_model" ) )
		return true;

	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	if ( pEnt->IsBaseCombatWeapon() )
		return true;

	return false;
}


bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	return !FindInList( s_PreserveEnts, pszClassName );
}

// Runs think for all player's conditions.
// Need to do this here instead of the player so players that crash still run their important thinks.
void CTFGameRules::RunPlayerConditionThink( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->m_Shared.ConditionGameRulesThink();
		}
	}
}

void CTFGameRules::FrameUpdatePostEntityThink()
{
	BaseClass::FrameUpdatePostEntityThink();

	RunPlayerConditionThink();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	int i;
	for ( i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	// Let all entities know that a new round is starting.
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );
		pEnt = gEntList.NextEnt( pEnt );
	}

	ResetBotPayloadInfo();

	// All entities have been spawned, now activate them.
	pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints.
		memset( m_bControlSpawnsPerTeam, 0, sizeof( bool ) * MAX_TEAMS * MAX_CONTROL_POINTS );
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>( pSpot );
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[pTFSpawn->GetTeamNumber()][pTFSpawn->GetControlPoint()->GetPointIndex()] = true;
				pTFSpawn->SetDisabled( true );
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

#ifdef GAME_DLL
	m_szMostRecentCappers[0] = 0;
#endif

	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateEnterPreRound.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}

	// If we're about to switch teams we'll assign VIP later.
	if ( IsVIPMode() && !IsInWaitingForPlayers() && m_bForceMapReset && !ShouldSwitchTeams() && !ShouldScrambleTeams() )
	{
		AssignTeamVIPs();
	}

	// For the Entrepreneur Achievement
	for ( int i = 0; i < gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		pPlayer->ResetPlayerHasDied();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running.
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	// Let out control point masters know that the round has started.
	FOR_EACH_VEC( g_hControlPointMasters, i )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->TeamFortress_SetGravity();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}

	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateEnterRoundRunning.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}
}


void CTFGameRules::PreRound_End( void )
{
	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateExitPreRound.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end).
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// Before we enter a new round, fire the "end output" for the previous round.
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}


void CTFGameRules::SendWinPanelInfo( void )
{
	if ( IsInArenaMode() )
	{
		SendArenaWinPanelInfo();
		return;
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_win_panel" );
	if ( event )
	{
		event->SetInt( "panel_style", WINPANEL_BASIC );
		event->SetInt( "winning_team", m_iWinningTeam );
		event->SetInt( "winreason", m_iWinReason );
		event->SetString( "cappers", m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ? m_szMostRecentCappers : "" );
		event->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );

		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			CTeam *pTeam = GetGlobalTeam( i );	
			if ( pTeam )
			{
				event->SetInt( UTIL_VarArgs( "%s_score", g_aTeamLowerNames[i] ), pTeam->GetScore() );
			}
		}

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && GetTimeLeft() <= 0 );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = pMaster ? pMaster->ShouldScorePerCapture() : false;

		event->SetInt( "scoring_team", bRoundComplete && !bScoringPerCapture ? m_iWinningTeam : TEAM_UNASSIGNED );

		event->SetInt( "round_complete", bRoundComplete );

		// Determine the 3 players on winning team who scored the most points this round.

		// Build a vector of players & round scores.
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;

		for ( int iPlayerIndex = 1; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;

			// Filter out spectators and, if not stalemate, all players not on winning team.
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( iPlayerTeam < FIRST_GAME_TEAM || ( m_iWinningTeam != TEAM_UNASSIGNED && m_iWinningTeam != iPlayerTeam ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );

			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
		}

		// Sort the players by round score.
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// Set the top (up to) 3 players by round score in the event data.
		int numPlayers = Min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// Only include players who have non-zero points this round; if we get to a player with 0 round points, stop.
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// Set the player index and their round score in the event.
			char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
			V_sprintf_safe( szPlayerIndexVal, "player_%d", i + 1 );
			V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i + 1 );
			event->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			event->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );
		}

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// If this was not a full round ending, include how many mini-rounds remain for winning team to win.
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				// Blegh, Payload Race has to use a different algorithm because of how the maps are constructed.
				if ( GetGameType() == TF_GAMETYPE_ESCORT && HasMultipleTrains() )
				{
					event->SetInt( "rounds_remaining", g_hControlPointMasters[0]->NumPlayableControlPointRounds() );
				}
				else
				{
					event->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
				}
			}
		}

		// Send the event.
		gameeventmanager->FireEvent( event );
	}
}


void CTFGameRules::SendArenaWinPanelInfo( void )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "arena_win_panel" );
	if ( event )
	{
		event->SetInt( "panel_style", WINPANEL_BASIC );
		event->SetInt( "scoring_team", m_iWinningTeam );
		event->SetInt( "winreason", m_iWinReason );
		event->SetString( "cappers", m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ? m_szMostRecentCappers : "" );
		event->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );

		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			CTeam *pTeam = GetGlobalTeam( i );
			if ( pTeam )
			{
				event->SetInt( UTIL_VarArgs( "%s_score", g_aTeamLowerNames[i] ), pTeam->GetScore() );
				//event->SetInt( UTIL_VarArgs( "%s_score_prev", g_aTeamLowerNames[i] ), i == m_iWinningTeam ? pTeam->GetScore() - 1 : pTeam->GetScore() );
			}
		}

		// Build a vector of players and round scores.
		CUtlVector<ArenaPlayerRoundScore_t> vecPlayerScore;

		for ( int iPlayerIndex = 1; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;

			// Filter out spectators.
			if ( pTFPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
				continue;

			// Ignore players who joined the game after the round has started.
			if ( !pTFPlayer->IsAlive() && pTFPlayer->GetDeathTime() < m_flRoundStartTime )
				continue;

			RoundStats_t *pStats = &CTF_GameStats.FindPlayerStats( pTFPlayer )->statsCurrentRound;

			ArenaPlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = CalcPlayerScore( pStats );
			playerRoundScore.iDamage = pStats->m_iStat[TFSTAT_DAMAGE];
			playerRoundScore.iHealing = pStats->m_iStat[TFSTAT_HEALING];
			playerRoundScore.iKills = pStats->m_iStat[TFSTAT_KILLS];

			// If they're alive count the time up to this point, otherwise count the time up to their death time.
			if ( pTFPlayer->IsAlive() )
			{
				playerRoundScore.iLifeTime = (int)( gpGlobals->curtime - m_flRoundStartTime );
			}
			else
			{
				playerRoundScore.iLifeTime = (int)( pTFPlayer->GetDeathTime() - m_flRoundStartTime );
			}
		}

		vecPlayerScore.Sort( ArenaPlayerRoundScoreSortFunc );

		// Collect top 3 winning players.
		int iNumWinners = 0;

		FOR_EACH_VEC( vecPlayerScore, i )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( vecPlayerScore[i].iPlayerIndex ) );
			if ( m_iWinningTeam != TEAM_UNASSIGNED && m_iWinningTeam != pTFPlayer->GetTeamNumber() )
				continue;

			iNumWinners++;

			event->SetInt( UTIL_VarArgs( "player_%d", iNumWinners ), vecPlayerScore[i].iPlayerIndex );
			event->SetInt( UTIL_VarArgs( "player_%d_damage", iNumWinners ), vecPlayerScore[i].iDamage );
			event->SetInt( UTIL_VarArgs( "player_%d_healing", iNumWinners ), vecPlayerScore[i].iHealing );
			event->SetInt( UTIL_VarArgs( "player_%d_lifetime", iNumWinners ), vecPlayerScore[i].iLifeTime );
			event->SetInt( UTIL_VarArgs( "player_%d_kills", iNumWinners ), vecPlayerScore[i].iKills );

			if ( iNumWinners == 3 )
				break;
		}

		if ( m_iWinningTeam != TEAM_UNASSIGNED )
		{
			// Now go through the the list again and collect top 3 losing players.
			int iNumLosers = 0, iNumLoserers;

			FOR_EACH_VEC( vecPlayerScore, i )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( vecPlayerScore[i].iPlayerIndex ) );
				if ( m_iWinningTeam == pTFPlayer->GetTeamNumber() )
					continue;

				iNumLosers++;
				iNumLoserers = iNumLosers + 3;

				event->SetInt( UTIL_VarArgs( "player_%d", iNumLoserers ), vecPlayerScore[i].iPlayerIndex );
				event->SetInt( UTIL_VarArgs( "player_%d_damage", iNumLoserers ), vecPlayerScore[i].iDamage );
				event->SetInt( UTIL_VarArgs( "player_%d_healing", iNumLoserers ), vecPlayerScore[i].iHealing );
				event->SetInt( UTIL_VarArgs( "player_%d_lifetime", iNumLoserers ), vecPlayerScore[i].iLifeTime );
				event->SetInt( UTIL_VarArgs( "player_%d_kills", iNumLoserers ), vecPlayerScore[i].iKills );

				if ( iNumLosers == 3 )
					break;
			}
		}

		// Send the event.
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score.
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// Sort first by round score.
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// If round scores are the same, sort next by total score.
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// If scores are the same, sort next by player index so we get deterministic sorting.
	return pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score.
//-----------------------------------------------------------------------------
int CTFGameRules::ArenaPlayerRoundScoreSortFunc( const ArenaPlayerRoundScore_t *pRoundScore1, const ArenaPlayerRoundScore_t *pRoundScore2 )
{
	// Sort priority:
	// 1) Score
	// 2) Healing
	// 3) Damage
	// 4) Lifetime
	// 5) Kills
	// 6) Index
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	if ( pRoundScore1->iHealing != pRoundScore2->iHealing )
		return pRoundScore2->iHealing - pRoundScore1->iHealing;

	if ( pRoundScore1->iDamage != pRoundScore2->iDamage )
		return pRoundScore2->iDamage - pRoundScore1->iDamage;

	if ( pRoundScore1->iLifeTime != pRoundScore2->iLifeTime )
		return pRoundScore2->iLifeTime - pRoundScore1->iLifeTime;

	if ( pRoundScore1->iKills != pRoundScore2->iKills )
		return pRoundScore2->iKills - pRoundScore1->iKills;

	// If scores are the same, sort next by player index so we get deterministic sorting.
	return pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex;
}


void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
		return;

	// Loop through the spawn points in the map and find which ones are associated with this round or the control points in this round.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while ( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn *>( pSpot );
		if ( pTFSpawn )
		{
			CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
			CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
			CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();
			CHandle<CTeamControlPointRound> hRoundGreen = pTFSpawn->GetRoundGreenSpawn();
			CHandle<CTeamControlPointRound> hRoundYellow = pTFSpawn->GetRoundYellowSpawn();
			if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
			{
				// This spawn is associated with one of our control points.
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
			}
			else if ( hRoundBlue && hRoundBlue == pCurrentRound )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
			}
			else if ( hRoundRed && hRoundRed == pCurrentRound )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_RED );
			}
			else if ( hRoundGreen && hRoundGreen == pCurrentRound )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_GREEN );
			}
			else if ( hRoundYellow && hRoundYellow == pCurrentRound )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_YELLOW );
			}
			else
			{
				// This spawn isn't associated with this round or the control points in this round.
				pTFSpawn->SetDisabled( true );
			}
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out).
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	if ( TFGameRules()->IsInArenaMode() )
	{
		CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
		{
			pArena->m_OnArenaRoundStart.FireOutput( pArena, pArena );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}

			g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_ROUNDSTART );
		}

		SetupOnRoundRunning();
		return;
	}

	// Respawn all the players.
	RespawnPlayers( true );

	// Remove everyone's objects.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
		}
	}

	m_hArenaDroppedTeamHealthKits.Purge();
	for ( bool& bHealthKit : m_bArenaTeamHealthKitsTeamDied )
		bHealthKit = false;


	// Disable all the active health packs in the world.
	m_hDisabledHealthKits.Purge();

	for ( CHealthKit *pHealthPack : CHealthKit::AutoList() )
	{
		if ( !pHealthPack->IsDisabled() )
		{
			pHealthPack->SetDisabled( true );
			m_hDisabledHealthKits.AddToTail( pHealthPack );
		}
	}
}


void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled.
	FOR_EACH_VEC( m_hDisabledHealthKits, i )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
	m_hArenaDroppedTeamHealthKits.Purge();
	for ( bool& bHealthKit : m_bArenaTeamHealthKitsTeamDied )
		bHealthKit = false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point.
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
		return false;

	return true;
}


bool CTFGameRules::CheckNextLevelCvar( bool bAllowEnd /*= true*/ )
{
	if ( m_bForceMapReset )
	{
		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "NextLevel CVAR" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

void CC_CH_ForceRespawn( void )
{
	if ( TFGameRules() )
	{
		TFGameRules()->RespawnPlayers( true );
	}
}
static ConCommand mp_forcerespawnplayers( "mp_forcerespawnplayers", CC_CH_ForceRespawn, "Force all players to respawn.", FCVAR_CHEAT );

static ConVar mp_tournament_allow_non_admin_restart( "mp_tournament_allow_non_admin_restart", "1", FCVAR_NONE, "Allow mp_tournament_restart command to be issued by players other than admin." );
void CC_CH_TournamentRestart( void )
{
	if ( !mp_tournament_allow_non_admin_restart.GetBool() )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;
	}

	CTFGameRules *pTFGameRules = TFGameRules();
	if ( pTFGameRules && !pTFGameRules->IsMannVsMachineMode() )
	{
		pTFGameRules->RestartTournament();
	}
}
static ConCommand mp_tournament_restart( "mp_tournament_restart", CC_CH_TournamentRestart, "Restart Tournament Mode on the current level." );

void CTFGameRules::RestartTournament( void )
{
	if ( !IsInTournamentMode() )
		return;

	SetInWaitingForPlayers( true );
	m_bAwaitingReadyRestart = true;
	m_flStopWatchTotalTime = -1.0f;
	m_bStopWatch = false;

	// We might have had a stalemate during the last round,
	// so reset this bool each time we restart the tournament.
	m_bChangelevelAfterStalemate = false;

	int i;
	for ( i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}
}

#ifdef GAME_DLL

// TODO: Execute even when TFGameRules doesn't exist yet - i.e. from main menu, or when restarting a server.
void cc_randommap_srv( const CCommand& args )
{
    if (!TFGameRules())
    {
        return;
    }

    if (!UTIL_IsCommandIssuedByServerAdmin())
    {
        return;
    }

    char szNextMap[MAX_MAP_NAME] = {};
	TFGameRules()->GetNextLevelName( szNextMap, sizeof( szNextMap ), true );
	engine->ChangeLevel( szNextMap, NULL );
}

static ConCommand randommap( "randommap", cc_randommap_srv, "Changelevel to a random map in the mapcycle file" );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bForceRespawn - respawn player even if dead or dying
//			bTeam - if true, only respawn the passed team
//			iTeam  - team to respawn
//-----------------------------------------------------------------------------
void CTFGameRules::RespawnPlayers( bool bForceRespawn, bool bTeam /* = false */, int iTeam/* = TEAM_UNASSIGNED */ )
{
	if ( bTeam )
	{
		Assert( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() );
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		// Check for team specific spawn.
		if ( bTeam && pPlayer->GetTeamNumber() != iTeam )
			continue;

		// Players that haven't chosen a team/class can never spawn.
		if ( !pPlayer->IsReadyToPlay() )
		{
			// Let the player spawn immediately when they do pick a class.
			if ( pPlayer->ShouldGainInstantSpawn() )
			{
				pPlayer->AllowInstantSpawn();
			}

			continue;
		}

		// If we aren't force respawning, don't respawn players that:
		// - are alive.
		// - are still in the death anim stage of dying.
		if ( !bForceRespawn )
		{
			if ( pPlayer->IsAlive() )
				continue;

			if ( m_iRoundState != GR_STATE_PREROUND )
			{
				// If the player hasn't been dead the minimum respawn time, he
				// waits until the next wave.
				if ( bTeam && !HasPassedMinRespawnTime( pPlayer ) )
					continue;

				if ( !pPlayer->IsReadyToSpawn() )
				{
					// Let the player spawn immediately when they do pick a class.
					if ( pPlayer->ShouldGainInstantSpawn() )
					{
						pPlayer->AllowInstantSpawn();
					}

					continue;
				}
			}
		}

		// Respawn this player.
		pPlayer->ForceRespawn();
	}
}


void CTFGameRules::SetForceMapReset( bool reset )
{
	m_bForceMapReset = reset;
}


void CTFGameRules::AddPlayedRound( string_t strName )
{
	if ( strName != NULL_STRING )
	{
		m_iszPreviousRounds.AddToHead( strName );

		// We only need to store the last two rounds that we've played.
		if ( m_iszPreviousRounds.Count() > 2 )
		{
			// Remove all but two of the entries (should only ever have to remove 1 when we're at 3).
			for ( int i = m_iszPreviousRounds.Count() - 1; i > 1; i-- )
			{
				m_iszPreviousRounds.Remove( i );
			}
		}
	}
}


bool CTFGameRules::IsPreviouslyPlayedRound( string_t strName )
{
	return m_iszPreviousRounds.Find( strName ) != m_iszPreviousRounds.InvalidIndex();
}


string_t CTFGameRules::GetLastPlayedRound( void )
{
	return m_iszPreviousRounds.Count() ? m_iszPreviousRounds[0] : NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a round winner.
//-----------------------------------------------------------------------------
void CTFGameRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	// Commentary doesn't let anyone win.
	if ( IsInCommentaryMode() )
		return;

	if ( team != TEAM_UNASSIGNED && ( team <= LAST_SHARED_TEAM || team >= GetNumberOfTeams() ) )
	{
		Assert( !"SetWinningTeam() called with invalid team." );
		return;
	}

	// Are we already in this state?
	if ( State_Get() == GR_STATE_TEAM_WIN )
		return;

	SetForceMapReset( bForceMapReset );
	SetSwitchTeams( bSwitchTeams );

	m_iWinningTeam = team;
	m_iWinReason = iWinReason;

	PlayWinSong( team );

	// Only reward the team if they have won the map and we're going to do a full reset or the time has run out and we're changing maps.
	bool bRewardTeam = bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

	if ( bDontAddScore )
	{
		bRewardTeam = false;
	}

	m_bUseAddScoreAnim = false;
	if ( bRewardTeam && ( team != TEAM_UNASSIGNED ) && ShouldScorePerRound() )
	{
		GetGlobalTeam( team )->AddScore( TEAMPLAY_ROUND_WIN_SCORE );
		m_bUseAddScoreAnim = true;
	}

	// This was a sudden death win if we were in stalemate then a team won it.
	bool bWasSuddenDeath = ( InStalemate() && m_iWinningTeam >= FIRST_GAME_TEAM );

	State_Transition( GR_STATE_TEAM_WIN );

	m_flLastTeamWin = gpGlobals->curtime;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_win" );
	if ( event )
	{
		event->SetInt( "team", team );
		event->SetInt( "winreason", iWinReason );
		event->SetBool( "full_round", bForceMapReset );
		event->SetFloat( "round_time", gpGlobals->curtime - m_flRoundStartTime );
		event->SetBool( "was_sudden_death", bWasSuddenDeath );

		// Let derived classes add more fields to the event.
		FillOutTeamplayRoundWinEvent( event );
		gameeventmanager->FireEvent( event );
	}

	// Send team scores.
	SendTeamScoresEvent();

	int i;
	if ( team == TEAM_UNASSIGNED )
	{
		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_STALEMATE );
		}
	}

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		pTFPlayer->SpeakConceptIfAllowed( pTFPlayer->GetTeamNumber() == team ? MP_CONCEPT_PLAYER_SUCCESS : MP_CONCEPT_PLAYER_FAILURE );
	}

	// Auto scramble teams?
	if ( bForceMapReset && mp_scrambleteams_auto.GetBool() )
	{
		if ( ( IsInArenaMode() && !mp_scrambleteams_auto_arena.GetBool() ) || IsInTournamentMode() || ShouldSkipAutoScramble())
			return;

#ifndef DEBUG
		// Don't bother on a listen server - usually not desirable.
		if ( !engine->IsDedicatedServer() )
			return;
#endif // DEBUG

		// Skip if we have a nextlevel set.
		if ( !FStrEq( nextlevel.GetString(), "" ) )
			return;

		// Look for impending level change.
		if ( ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate ) && GetTimeLeft() <= 300 )
			return;

		if ( GetRoundsRemaining() == 1 )
			return;

		if ( ShouldScorePerRound() && GetWinsRemaining() == 1 )
			return;

		// Increment win counters.
		if ( m_iWinningTeam >= FIRST_GAME_TEAM )
		{
			GetGlobalTFTeam( m_iWinningTeam )->IncrementWins();
		}

		// Did we hit our win delta?
		int nWinDelta = -1;
		int nHighestWins = 0;

		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			int nWins = GetGlobalTFTeam( i )->GetWinCount();
			if ( nWins > nHighestWins )
			{
				if ( nWinDelta == -1 )
				{
					nWinDelta = 0;
				}
				else
				{
					nWinDelta = nWins - nHighestWins;
				}

				nHighestWins = nWins;
			}
		}

		if ( nWinDelta >= mp_scrambleteams_auto_windifference.GetInt() )
		{
			// Let the server know we're going to scramble on round restart.
#if defined( TF_DLL ) || defined( TF_CLASSIC )
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
			if ( event )
			{
				event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
				gameeventmanager->FireEvent( event );
			}
#else
			const char *pszMessage = "#game_scramble_onrestart";
			if ( pszMessage )
			{
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pszMessage );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pszMessage );
			}
#endif
			UTIL_LogPrintf( "World triggered \"ScrambleTeams_Auto\"\n" );

			SetScrambleTeams( true );
			ShouldResetScores( true, false );
			ShouldResetRoundsPlayed( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a stalemate.
// Most often a team_control_point_master saying that the
// round timer expired.
//-----------------------------------------------------------------------------
void CTFGameRules::SetStalemate( int iReason, bool bForceMapReset /*= true*/, bool bSwitchTeams /*= false*/ )
{
	if ( IsInTournamentMode() && IsInPreMatch() )
		return;

	if ( !mp_stalemate_enable.GetBool() )
	{
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bForceMapReset, bSwitchTeams );
		return;
	}

	if ( InStalemate() )
		return;

	SetForceMapReset( bForceMapReset );

	m_iWinningTeam = TEAM_UNASSIGNED;

	PlaySuddenDeathSong();

	State_Transition( GR_STATE_STALEMATE );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
	if ( event )
	{
		event->SetInt( "reason", iReason );
		gameeventmanager->FireEvent( event );
	}

	if (g_pGameRulesProxy)
	{
		g_pGameRulesProxy->m_OnSuddenDeath.FireOutput(g_pGameRulesProxy, g_pGameRulesProxy);
	}
}


void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();
		if ( pRound )
		{
			// Do we have opposing points in this round?
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );
			if ( pRedPoint && pBluePoint )
			{
				SetMiniRoundBitMask( ( 1 << pBluePoint->GetPointIndex() ) | ( 1 << pRedPoint->GetPointIndex() ) );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}
}


void CTFGameRules::SetTeamRespawnWaveTime( int iTeam, float flValue )
{
	if ( flValue < 0 )
	{
		flValue = 0;
	}

	// Initialized to -1 so we can try to determine if this is the first spawn time we have received for this team.
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flValue;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flValue );
}


void CTFGameRules::AddTeamRespawnWaveTime( int iTeam, float flValue )
{
	float flCurrentSetting = m_TeamRespawnWaveTimes[iTeam];
	if ( flCurrentSetting < 0 )
	{
		flCurrentSetting = mp_respawnwavetime.GetFloat();
	}

	// Initialized to -1 so we can try to determine if this is the first spawn time we have received for this team.
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flCurrentSetting;
	}

	float flNewValue = flCurrentSetting + flValue;
	if ( flNewValue < 0 )
	{
		flNewValue = 0;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
// this method a chance to add more data to it.
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	// Determine the losing team.
	int iLosingTeam;

	switch ( event->GetInt( "team" ) )
	{
		case TF_TEAM_RED:
			iLosingTeam = TF_TEAM_BLUE;
			break;
		case TF_TEAM_BLUE:
			iLosingTeam = TF_TEAM_RED;
			break;
		case TEAM_UNASSIGNED:
		default:
			iLosingTeam = TEAM_UNASSIGNED;
			break;
	}

	// Set the number of caps that team got any time during the round.
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}


bool CTFGameRules::IsGameUnderTimeLimit( void )
{
	return mp_timelimit.GetInt() > 0;
}


CTeamRoundTimer *CTFGameRules::GetActiveRoundTimer( void )
{
	return ( dynamic_cast<CTeamRoundTimer *>( UTIL_EntityByIndex( ObjectiveResource()->GetTimerInHUD() ) ) );
}


void CTFGameRules::HandleTimeLimitChange( void )
{
	// Check that we have an active timer in the HUD and use mp_timelimit if we don't.
	if ( !MapHasActiveTimer() && ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0 ) )
	{
		CreateTimeLimitTimer();
	}
	else if ( m_hTimeLimitTimer )
	{
		UTIL_Remove( m_hTimeLimitTimer );
		m_hTimeLimitTimer = NULL;
	}
}


void CTFGameRules::ResetPlayerAndTeamReadyState( void )
{
	int i;
	for ( i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}

#ifdef GAME_DLL
	// NOTE: <= MAX_PLAYERS vs < MAX_PLAYERS above.
	for ( i = 0; i <= MAX_PLAYERS; i++ )
	{
		m_bPlayerReadyBefore[i] = false;
	}
#endif // GAME_DLL
}


void CTFGameRules::PlayTrainCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap )
{
	if ( m_bMultipleTrains )
		return;

	if ( !pPoint || !PointsMayBeCaptured() )
		return;

	if ( bFinalPointInMap )
	{
		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			if ( i != pPoint->GetOwner() )
			{
				g_TFAnnouncer.Speak( i, TF_ANNOUNCER_CART_FINALWARNING_ATTACKER );
			}
			else
			{
				g_TFAnnouncer.Speak( i, TF_ANNOUNCER_CART_FINALWARNING_DEFENDER );
			}
		}
	}
	else
	{
		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			if ( i != pPoint->GetOwner() )
			{
				g_TFAnnouncer.Speak( i, TF_ANNOUNCER_CART_WARNING_ATTACKER );
			}
			else
			{
				g_TFAnnouncer.Speak( i, TF_ANNOUNCER_CART_WARNING_DEFENDER );
			}
		}
	}
}


void CTFGameRules::PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint )
{
	if ( GetGameType() == TF_GAMETYPE_CP )
	{
		bool bPlayControlPointCappedSound = IsInKothMode();
		if ( !bPlayControlPointCappedSound )
		{
			if ( pPoint && ShouldScorePerRound() )
			{
				CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
				if ( pMaster && !pMaster->WouldNewCPOwnerWinGame( pPoint, iCappingTeam ) )
				{
					bPlayControlPointCappedSound = true;
				}
			}
		}

		if ( bPlayControlPointCappedSound )
		{
			if ( IsInKothMode() || IsInDominationMode() )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_DING );
			}

			int iOriginalTeam = pPoint->GetTeamNumber();
			if ( iOriginalTeam == TEAM_UNASSIGNED )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_FAILURE );
			}
			else
			{
				g_TFAnnouncer.Speak( iOriginalTeam, TF_ANNOUNCER_FAILURE );
			}

			g_TFAnnouncer.Speak( iCappingTeam, TF_ANNOUNCER_SUCCESS );
		}
	}
}


bool CTFGameRules::PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext )
{
	if ( m_flNewThrottledAlertTime <= gpGlobals->curtime )
	{
		BroadcastSound( iTeam, sound );
		m_flNewThrottledAlertTime = gpGlobals->curtime + fDelayBeforeNext;
		return true;
	}

	return false;
}


void CTFGameRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags )
{
	// Send it to everyone.
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		gameeventmanager->FireEvent( event );
	}
}


void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM + 1, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point.
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn *>( pSpot );
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					pTFSpawn->SetDisabled( pTFSpawn->GetControlPoint()->GetPointIndex() != iFarthestPoint );
				}
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}
	}
}


bool CTFGameRules::ShouldSkipAutoScramble( void )
{
	return false;
}


void CTFGameRules::SetOvertime( bool bOvertime )
{
	if ( m_bInOvertime == bOvertime )
		return;

	if ( bOvertime )
	{
		UTIL_LogPrintf( "World triggered \"Round_Overtime\"\n" );
	}

	m_bInOvertime = bOvertime;

	if ( m_bInOvertime )
	{
		// Tell train watchers that we've transitioned to overtime.
		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			variant_t emptyVariant;
			pWatcher->AcceptInput( "OnStartOvertime", NULL, NULL, emptyVariant, 0 );
			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}

		if ( g_pGameRulesProxy )
		{
			g_pGameRulesProxy->m_OnOvertime.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
		}
	}
}


void CTFGameRules::Activate()
{
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );
	tf_gamemode_cp.SetValue( 0 );
	tf_gamemode_ctf.SetValue( 0 );
	tf_gamemode_sd.SetValue( 0 );
	tf_gamemode_payload.SetValue( 0 );
	tf_gamemode_mvm.SetValue( 0 );
	tf_gamemode_rd.SetValue( 0 );
	tf_gamemode_passtime.SetValue( 0 );

	tf_bot_count.SetValue( 0 );

	if ( IsInTraining() || TheTFBots().IsInOfflinePractice() || IsInItemTestingMode() )
	{
		hide_server.SetValue( 1 );
	}

	m_bPlayingKoth = false;
	m_bMultipleTrains = false;
	m_bPlayingMedieval = false;
	m_bPlayingHybrid_CTF_CP = false;
	m_bPlayingDomination = false;
	m_bPlayingTerritorialDomination = false;

	ResetBotPayloadInfo();
	ResetBotRosters();

	CTFBotRoster *pBotRoster = NULL;
	while ( ( pBotRoster = dynamic_cast<CTFBotRoster *>( gEntList.FindEntityByClassname( pBotRoster, "bot_roster" ) ) ) != NULL )
	{
		if ( FStrEq( pBotRoster->GetTeamName(), "red" )    && m_hBotRosterRed    == NULL ) m_hBotRosterRed    = pBotRoster;
		if ( FStrEq( pBotRoster->GetTeamName(), "blue" )   && m_hBotRosterBlue   == NULL ) m_hBotRosterBlue   = pBotRoster;
		if ( FStrEq( pBotRoster->GetTeamName(), "green" )  && m_hBotRosterGreen  == NULL ) m_hBotRosterGreen  = pBotRoster;
		if ( FStrEq( pBotRoster->GetTeamName(), "yellow" ) && m_hBotRosterYellow == NULL ) m_hBotRosterYellow = pBotRoster;
	}

	// Arena is Seperate from every other mode so we can combine it with them
	// But it still first sets the gametype to arena in case its the only logic present
	if ( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_ARENA );
		tf_gamemode_arena.SetValue( 1 );
		m_bArenaMode = true;
		Msg( "Executing server arena config file\n" );
		engine->ServerCommand( "exec config_arena.cfg\n" );
	}

	if ( gEntList.FindEntityByClassname( NULL, "tf_logic_vip" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_VIP );
	}
	else if ( !V_strncmp( STRING( gpGlobals->mapname ), "sd_", 3 ) )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		m_bPlayingSpecialDeliveryMode = true;
		tf_gamemode_sd.SetValue( 1 );
	}
	else if ( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_ESCORT );

		if ( gEntList.FindEntityByClassname( NULL, "tf_logic_multiple_escort" ) != NULL )
		{
			m_bMultipleTrains = true;
		}

		tf_gamemode_payload.SetValue( 1 );
	}
	else if ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		tf_gamemode_ctf.SetValue( 1 );
	}
	else if ( g_hControlPointMasters.Count() != 0 )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_cp.SetValue( 1 );
	}
	else if ( gEntList.FindEntityByClassname( NULL, "team_generator" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_POWERSIEGE );
	}
	else if ( gEntList.FindEntityByClassname( NULL, "tf_logic_infiltration" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_INFILTRATION );
	}
	// Possibly for a future update.
	/*
	else if ( gEntList.FindEntityByClassname( NULL, "tf_logic_siege" ) != NULL )
	{
		m_nGameType.Set( TF_GAMETYPE_SIEGE );
		tf_gamemode_siege.SetValue( 1 );
	}
	*/

	if ( gEntList.FindEntityByClassname( NULL, "tf_logic_koth" ) != NULL )
	{
		m_bPlayingKoth = true;
	}

	if ( tf_medieval.GetBool() || gEntList.FindEntityByClassname( NULL, "tf_logic_medieval" ) != NULL )
	{
		m_bPlayingMedieval = true;
	}

	if ( gEntList.FindEntityByClassname( NULL, "tf_logic_hybrid_ctf_cp" ) != NULL )
	{
		m_bPlayingHybrid_CTF_CP = true;
	}

	if ( gEntList.FindEntityByClassname( NULL, "tf_logic_domination" ) != NULL )
	{
		m_bPlayingDomination = true;
	}
	
	if ( !V_strncmp( STRING( gpGlobals->mapname ), "td_", 3 ) )
	{
		m_bPlayingTerritorialDomination = true;
	}

	SendLoadingScreenInfo();
}


bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	if ( State_Get() == GR_STATE_TEAM_WIN && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			// We don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time.
			CBaseTrigger *pTrigger = dynamic_cast<CBaseTrigger *>( info.GetInflictor() );
			if ( pTrigger && pTrigger->UsesFilter() )
				return false;
		}
	}

	return true;
}


void CTFGameRules::CheckChatText( CBasePlayer *pPlayer, char *pText )
{
	CheckChatForReadySignal( pPlayer, pText );

	BaseClass::CheckChatText( pPlayer, pText );
}


void CTFGameRules::CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg )
{
	if ( !IsInTournamentMode() )
	{
		if ( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_clan_ready_signal.GetString() ) )
		{
			int iTeam = pPlayer->GetTeamNumber();
			if ( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() )
			{
				m_bTeamReady.Set( iTeam, true );

				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_team_ready" );
				if ( event )
				{
					event->SetInt( "team", iTeam );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SERVERONLY_DLL_EXPORT void CTFGameRules::SetInWaitingForPlayers( bool bWaitingForPlayers )
{
	// Never waiting for players when loading a bug report.
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	if ( m_bInWaitingForPlayers == bWaitingForPlayers )
		return;

	if ( ShouldWaitForPlayersInPregame() && m_flWaitingForPlayersTimeEnds == -1 && !IsInTournamentMode() )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	m_bInWaitingForPlayers = bWaitingForPlayers;

	if ( m_bInWaitingForPlayers )
	{
		m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();
	}
	else
	{
		m_flWaitingForPlayersTimeEnds = -1.0f;

		if ( m_hWaitingForPlayersTimer )
		{
			UTIL_Remove( m_hWaitingForPlayersTimer );
		}
	}
}


void CTFGameRules::SetSetup( bool bSetup )
{
	if ( m_bInSetup == bSetup )
		return;

	m_bInSetup = bSetup;
}


void CTFGameRules::CheckWaitingForPlayers( void )
{
	// Never waiting for players when loading a bug report, or training.
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background || !AllowWaitingForPlayers() )
		return;

	if ( mp_waitingforplayers_restart.GetBool() )
	{
		if ( m_bInWaitingForPlayers )
		{
			m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();

			if ( m_hWaitingForPlayersTimer )
			{
				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			}
		}
		else
		{
			SetInWaitingForPlayers( true );
		}

		mp_waitingforplayers_restart.SetValue( 0 );
	}

	bool bCancelWait = ( mp_waitingforplayers_cancel.GetBool() || IsInItemTestingMode() ) && !IsInTournamentMode();

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		bCancelWait = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bCancelWait )
	{
		mp_waitingforplayers_cancel.SetValue( 0 );

		if ( m_bInWaitingForPlayers )
		{
			if ( ShouldWaitForPlayersInPregame() )
			{
				// Reset asap.
				m_flRestartRoundTime = gpGlobals->curtime;
				return;
			}

			// Cancel the wait period and manually Resume() the timer if 
			// it's not supposed to start paused at the beginning of a round.
			// We must do this before SetInWaitingForPlayers() is called because it will
			// restore the timer in the HUD and set the handle to NULL.
			CTeamRoundTimer *pActiveTimer = dynamic_cast<CTeamRoundTimer *>( UTIL_EntityByIndex( ObjectiveResource()->GetTimerInHUD() ) );
			if ( pActiveTimer && !pActiveTimer->StartPaused() )
			{
				pActiveTimer->ResumeTimer();
			}

			SetInWaitingForPlayers( false );
			return;
		}
	}

	if ( m_bInWaitingForPlayers )
	{
		if ( IsInTournamentMode() )
			return;

		// Only exit the waitingforplayers if the time is up, and we are not in a round,
		// restart countdown already, and we are not waiting for a ready restart.
		if ( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds && m_flRestartRoundTime < 0 && !m_bAwaitingReadyRestart )
		{
			// Reset asap.
			m_flRestartRoundTime = gpGlobals->curtime;

			// If "waiting for players" is ending and we're restarting...
			// Keep the current round that we're already running around in as the first round after the restart.
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster && pMaster->PlayingMiniRounds() && pMaster->GetCurrentRound() )
			{
				SetRoundToPlayNext( pMaster->GetRoundToUseAfterRestart() );
			}
		}
		else
		{
			if ( !m_hWaitingForPlayersTimer )
			{
				// Stop any timers, and bring up a new one.
				m_hWaitingForPlayersTimer = (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
				m_hWaitingForPlayersTimer->SetName( AllocPooledString( "zz_teamplay_waiting_timer" ) );
				m_hWaitingForPlayersTimer->SetShowInHud( true );

				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}
		}
	}
}


void CTFGameRules::CheckRestartRound( void )
{
	if ( mp_clan_readyrestart.GetBool() && !IsInTournamentMode() )
	{
		m_bAwaitingReadyRestart = true;

		for ( int i = LAST_SHARED_TEAM + 1, c = GetNumberOfTeams(); i < c; i++ )
		{
			m_bTeamReady.Set( i, false );
		}

		const char *pszReadyString = mp_clan_ready_signal.GetString();
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#clan_ready_rules", pszReadyString );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#clan_ready_rules", pszReadyString );

		// Don't let them put anything malicious in there.
		if ( !pszReadyString || Q_strlen( pszReadyString ) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_ready_restart" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		mp_clan_readyrestart.SetValue( 0 );

		// Cancel any restart round in progress.
		m_flRestartRoundTime = -1;
	}

	// Restart the game if specified by the server.
	int iRestartDelay = mp_restartround.GetInt();
	bool bRestartGameNow = mp_restartgame_immediate.GetBool();
	if ( iRestartDelay == 0 && !bRestartGameNow )
	{
		iRestartDelay = mp_restartgame.GetInt();
	}

	if ( iRestartDelay > 0 || bRestartGameNow )
	{
		int iDelayMax = 60;

#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
		if ( TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() ) )
		{
			iDelayMax = 180;
		}
#endif // #if defined( TF_CLIENT_DLL ) || defined( TF_DLL )

		if ( iRestartDelay > iDelayMax )
		{
			iRestartDelay = iDelayMax;
		}

		if ( mp_restartgame.GetInt() > 0 || bRestartGameNow )
		{
			SetForceMapReset( true );
		}
		else
		{
			SetForceMapReset( false );
		}

		SetInStopWatch( false );

		if ( bRestartGameNow )
		{
			iRestartDelay = 0;
		}

		m_flRestartRoundTime = gpGlobals->curtime + iRestartDelay;

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_restart_seconds" );
		if ( event )
		{
			event->SetInt( "seconds", iRestartDelay );
			gameeventmanager->FireEvent( event );
		}

		if ( !IsInTournamentMode() )
		{
			// Let the players know.
			const char *pFormat = NULL;

			if ( mp_restartgame.GetInt() > 0 )
			{
				if ( ShouldSwitchTeams() )
				{
					pFormat = iRestartDelay > 1 ? "#game_switch_in_secs" : "#game_switch_in_sec";
				}
				else if ( ShouldScrambleTeams() )
				{
					pFormat = iRestartDelay > 1 ? "#game_scramble_in_secs" : "#game_scramble_in_sec";

#if defined( TF_DLL ) || defined( TF_CLASSIC )
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
					if ( event )
					{
						event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
						gameeventmanager->FireEvent( event );
					}

					pFormat = NULL;
#endif
				}
			}
			else if ( mp_restartround.GetInt() > 0 )
			{
				pFormat = iRestartDelay > 1 ? "#round_restart_in_secs" : "#round_restart_in_sec";
			}

			if ( pFormat )
			{
				char strRestartDelay[64];
				Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pFormat, strRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pFormat, strRestartDelay );
			}
		}

		mp_restartround.SetValue( 0 );
		mp_restartgame.SetValue( 0 );
		mp_restartgame_immediate.SetValue( 0 );

		// Cancel any ready restart in progress.
		m_bAwaitingReadyRestart = false;
	}
}


bool CTFGameRules::CheckTimeLimit( bool bAllowEnd /*= true*/ )
{
	if ( IsInPreMatch() )
		return false;

	if ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate )
	{
		// If there's less than 5 minutes to go, just switch now. This avoids the problem
		// of sudden death modes starting shortly after a new round starts.
		const int iMinTime = 5;
		bool bSwitchDueToTime = ( mp_timelimit.GetInt() > iMinTime && GetTimeLeft() < ( iMinTime * 60 ) );

		if ( IsInTournamentMode() )
		{
			if ( !TournamentModeCanEndWithTimelimit() )
				return false;

			bSwitchDueToTime = false;
		}
		else if ( IsInArenaMode() )
		{
			bSwitchDueToTime = false;
		}

		if ( GetTimeLeft() <= 0 || m_bChangelevelAfterStalemate || bSwitchDueToTime )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Time Limit" );
					gameeventmanager->FireEvent( event );
				}

				SendTeamScoresEvent();

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}


bool CTFGameRules::CheckWinLimit( bool bAllowEnd /*= true*/ )
{
	// Has one team won the specified number of rounds?
	if ( mp_winlimit.GetInt() <= 0 )
		return false;

	for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		if ( pTeam->GetScore() >= mp_winlimit.GetInt() + m_iWinLimitBonus )
		{
			if ( bAllowEnd )
			{
				UTIL_LogPrintf( "Team \"%s\" triggered \"Intermission_Win_Limit\"\n", pTeam->GetName() );

				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Win Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}


bool CTFGameRules::CheckMaxRounds( bool bAllowEnd /*= true*/ )
{
	if ( mp_maxrounds.GetInt() > 0 && !IsInPreMatch() )
	{
		if ( GetRoundsRemaining() == 0 )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Round Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}


int CTFGameRules::GetWinsRemaining( void )
{
	if ( mp_winlimit.GetInt() <= 0 )
		return -1;

	int iMaxWins = 0;

	for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		if ( pTeam->GetScore() > iMaxWins )
		{
			iMaxWins = pTeam->GetScore();
		}
	}

	return Max( mp_winlimit.GetInt() + m_iWinLimitBonus - iMaxWins, 0 );
}


int CTFGameRules::GetRoundsRemaining( void )
{
	if ( mp_maxrounds.GetInt() <= 0 )
		return -1;

	return Max( mp_maxrounds.GetInt() + m_iMaxRoundsBonus - m_nRoundsPlayed, 0 );
}


void CTFGameRules::CheckReadyRestart( void )
{
	// Check round restart.
	if ( m_flRestartRoundTime > 0.0f && m_flRestartRoundTime <= gpGlobals->curtime && !g_pServerBenchmark->IsBenchmarkRunning() )
	{
		m_flRestartRoundTime = -1.0f;

#ifdef TF_DLL
		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				if ( g_pPopulationManager && TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
				{
					g_pPopulationManager->StartCurrentWave();
					m_bAllowBetweenRounds = true;
					return;
				}
			}
			else if ( TFGameRules()->IsCompetitiveMode() )
			{
				TFGameRules()->StartCompetitiveMatch();
				return;
			}
			else if ( mp_tournament.GetBool() )
			{
				// Temp.
				TFGameRules()->StartCompetitiveMatch();
				return;
			}
		}
#endif // TF_DLL

		// Time to restart!
		State_Transition( GR_STATE_RESTART );
	}

	bool bProcessReadyRestart = m_bAwaitingReadyRestart;

#ifdef TF_DLL
	bProcessReadyRestart &= TFGameRules() && !TFGameRules()->UsePlayerReadyStatusMode();
#endif // TF_DLL

	// Check ready restart.
	if ( bProcessReadyRestart )
	{
		bool bTeamNotReady = false;
		for ( int i = LAST_SHARED_TEAM + 1, c = GetNumberOfTeams(); i < c; i++ )
		{
			if ( !m_bTeamReady[i] )
			{
				bTeamNotReady = true;
				break;
			}
		}

		if ( !bTeamNotReady )
		{
			mp_restartgame.SetValue( 5 );
			m_bAwaitingReadyRestart = false;

			ShouldResetScores( true, true );
			ShouldResetRoundsPlayed( true );
		}
	}
}

#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )

bool CTFGameRules::AreLobbyPlayersOnTeamReady( int iTeam )
{
	if ( !TFGameRules() )
		return false;

	if ( TFGameRules()->IsMannVsMachineMode() && iTeam == TF_TEAM_PVE_INVADERS )
		return true;

	bool bAtLeastOnePersonReady = false;

	CUtlVector<LobbyPlayerInfo_t> vecLobbyPlayers;
	GetPotentialPlayersLobbyPlayerInfo( vecLobbyPlayers );

	FOR_EACH_VEC( vecLobbyPlayers, i )
	{
		const LobbyPlayerInfo_t &p = vecLobbyPlayers[i];

		// Make sure all lobby players are connected.
		if ( !AreLobbyPlayersConnected() )
			return false;

		// All are connected, make sure their team is ready.
		if ( p.m_iTeam == iTeam )
		{
			if ( !m_bPlayerReady[p.m_nEntNum] )
				return false;

			// He's totally ready.
			bAtLeastOnePersonReady = true;
		}
		else
		{
			// In MvM, only the red team should pass through here.
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				// And you may ask yourself, "How did I get here?".
				Assert( p.m_iTeam == iTeam );
			}
		}
	}

	// We didn't find anybody who we should wait for, so
	// if at least one person is ready, then we're ready.
	return bAtLeastOnePersonReady;
}

//-----------------------------------------------------------------------------
// Purpose: Is everyone in the lobby connected to the server?
//-----------------------------------------------------------------------------
bool CTFGameRules::AreLobbyPlayersConnected( void )
{
	CUtlVector<LobbyPlayerInfo_t> vecLobbyPlayers;
	GetPotentialPlayersLobbyPlayerInfo( vecLobbyPlayers );

	// If you're calling this, you should have lobby members.
	Assert( vecLobbyPlayers.Count() );

	FOR_EACH_VEC( vecLobbyPlayers, i )
	{
		const LobbyPlayerInfo_t &pLobbyPlayer = vecLobbyPlayers[i];
		if ( !pLobbyPlayer.m_bConnected ||
			pLobbyPlayer.m_nEntNum <= 0 ||
			pLobbyPlayer.m_nEntNum >= MAX_PLAYERS ||
			( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pLobbyPlayer.m_iTeam == TEAM_UNASSIGNED ) )
		{
			if ( pLobbyPlayer.m_bInLobby )
				return false;
		}
	}

	return true;
}
#endif // #if defined( TF_CLIENT_DLL ) || defined( TF_DLL )

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change.
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	// We only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl).
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() )
			return false;
	}

	return true;
}


bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home.
	for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
	{
		if ( !pFlag->IsHome() )
			return false;
	}

	// Check that one team hasn't won by capping.
	if ( CheckCapsPerRound() )
		return false;

	return true;
}


void CTFGameRules::State_Transition( gamerules_roundstate_t newState )
{
	m_prevState = State_Get();

	State_Leave();
	State_Enter( newState );
}


void CTFGameRules::State_Enter( gamerules_roundstate_t newState )
{
	m_iRoundState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flLastRoundStateChangeTime = gpGlobals->curtime;

	if ( mp_showroundtransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
		{
			Msg( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		}
		else
		{
			Msg( "Gamerules: entering state #%d\n", newState );
		}
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		( this->*m_pCurStateInfo->pfnEnterState )( );
	}
}


void CTFGameRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		( this->*m_pCurStateInfo->pfnLeaveState )( );
	}
}



void CTFGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		( this->*m_pCurStateInfo->pfnThink )( );
	}
}


CGameRulesRoundStateInfo* CTFGameRules::State_LookupInfo( gamerules_roundstate_t state )
{
	static CGameRulesRoundStateInfo playerStateInfos[] =
	{
		{ GR_STATE_INIT, "GR_STATE_INIT", &CTFGameRules::State_Enter_INIT, NULL, &CTFGameRules::State_Think_INIT },
		{ GR_STATE_PREGAME, "GR_STATE_PREGAME", &CTFGameRules::State_Enter_PREGAME, NULL, &CTFGameRules::State_Think_PREGAME },
		{ GR_STATE_STARTGAME, "GR_STATE_STARTGAME", &CTFGameRules::State_Enter_STARTGAME, NULL, &CTFGameRules::State_Think_STARTGAME },
		{ GR_STATE_PREROUND, "GR_STATE_PREROUND", &CTFGameRules::State_Enter_PREROUND, &CTFGameRules::State_Leave_PREROUND, &CTFGameRules::State_Think_PREROUND },
		{ GR_STATE_RND_RUNNING, "GR_STATE_RND_RUNNING", &CTFGameRules::State_Enter_RND_RUNNING, &CTFGameRules::State_Leave_RND_RUNNING, &CTFGameRules::State_Think_RND_RUNNING },
		{ GR_STATE_TEAM_WIN, "GR_STATE_TEAM_WIN", &CTFGameRules::State_Enter_TEAM_WIN, NULL, &CTFGameRules::State_Think_TEAM_WIN },
		{ GR_STATE_RESTART, "GR_STATE_RESTART", &CTFGameRules::State_Enter_RESTART, NULL, &CTFGameRules::State_Think_RESTART },
		{ GR_STATE_STALEMATE, "GR_STATE_STALEMATE", &CTFGameRules::State_Enter_STALEMATE, &CTFGameRules::State_Leave_STALEMATE, &CTFGameRules::State_Think_STALEMATE },
		{ GR_STATE_GAME_OVER, "GR_STATE_GAME_OVER", NULL, NULL, NULL },
		{ GR_STATE_BONUS, "GR_STATE_BONUS", &CTFGameRules::State_Enter_BONUS, &CTFGameRules::State_Leave_BONUS, &CTFGameRules::State_Think_BONUS },
		{ GR_STATE_BETWEEN_RNDS, "GR_STATE_BETWEEN_RNDS", &CTFGameRules::State_Enter_BETWEEN_RNDS, &CTFGameRules::State_Leave_BETWEEN_RNDS, &CTFGameRules::State_Think_BETWEEN_RNDS },
	};

	for ( int i = 0, c = ARRAYSIZE( playerStateInfos ); i < c; i++ )
	{
		if ( playerStateInfos[i].m_iRoundState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}


void CTFGameRules::State_Enter_INIT( void )
{
	InitTeams();
	ResetMapTime();
}


void CTFGameRules::State_Think_INIT( void )
{
	State_Transition( GR_STATE_PREGAME );
}

//-----------------------------------------------------------------------------
// Purpose: The server is idle and waiting for enough players to start up again. 
// When we find an active player go to GR_STATE_STARTGAME.
//-----------------------------------------------------------------------------
void CTFGameRules::State_Enter_PREGAME( void )
{
	m_flNextPeriodicThink = gpGlobals->curtime + 0.1;
	SetInWaitingForPlayers( false );
	m_flRestartRoundTime = -1.0f;
}


void CTFGameRules::State_Think_PREGAME( void )
{
	CheckRespawnWaves();

	// We'll just stay in pregame for the bugbait reports.
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
		return;

	// Commentary stays in this mode too.
	if ( IsInCommentaryMode() )
		return;

	if ( ShouldWaitForPlayersInPregame() )
	{
		// In Arena mode, start waiting for players period once we have enough players ready to play.
		if ( CountActivePlayers() > 0 )
		{
			if ( !IsInWaitingForPlayers() )
			{
				m_flWaitingForPlayersTimeEnds = 0.0f;
				SetInWaitingForPlayers( true );
			}

			CheckReadyRestart();
		}
		else
		{
			if ( IsInWaitingForPlayers() )
			{
				SetInWaitingForPlayers( false );
				m_flRestartRoundTime = -1.0f;
			}
		}
	}
	else if ( CountActivePlayers() > 0 )
	{
		State_Transition( GR_STATE_STARTGAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Wait a bit and then spawn everyone into the preround.
//-----------------------------------------------------------------------------
void CTFGameRules::State_Enter_STARTGAME( void )
{
	m_flStateTransitionTime = gpGlobals->curtime;
}


void CTFGameRules::State_Think_STARTGAME()
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( !IsInTraining() && !IsInItemTestingMode() )
		{
			ConVarRef tf_bot_offline_practice( "tf_bot_offline_practice" );
			if ( mp_waitingforplayers_time.GetFloat() > 0 && tf_bot_offline_practice.GetInt() == 0 )
			{
				// Go into waitingforplayers, reset at end of it.
				SetInWaitingForPlayers( true );
			}
		}

		State_Transition( GR_STATE_PREROUND );
	}
}


void CTFGameRules::State_Enter_PREROUND( void )
{
	BalanceTeams( false );

	m_flStartBalancingTeamsAt = gpGlobals->curtime + 60.0;

	RoundRespawn();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_start" );
	if ( event )
	{
		event->SetBool( "full_reset", m_bForceMapReset );
		gameeventmanager->FireEvent( event );
	}

	if ( IsInArenaMode() )
	{
		if ( CountActivePlayers() > 0 )
		{
			variant_t sVariant;
			if ( !m_hStalemateTimer )
			{
				m_hStalemateTimer = (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
				m_hStalemateTimer->SetName( AllocPooledString( "zz_arena_preround_timer" ) );
				m_hStalemateTimer->SetShowInHud( true );
			}

			sVariant.SetInt( tf_arena_preround_time.GetInt() );
			m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );

			event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}
		}

		m_flStateTransitionTime = gpGlobals->curtime + tf_arena_preround_time.GetInt();
	}
#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	// Only allow at the very beginning of the game, or between waves in MvM.
	else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && m_bAllowBetweenRounds )
	{
		State_Transition( GR_STATE_BETWEEN_RNDS );
		m_bAllowBetweenRounds = false;

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
		}
	}
#endif // #if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	else if ( m_bPreRoundFreezeTime )
	{
		m_flStateTransitionTime = gpGlobals->curtime + 5.0f * mp_enableroundwaittime.GetFloat();
	}
	else
	{
		m_flStateTransitionTime = gpGlobals->curtime;
	}

	StopWatchModeThink();
}


void CTFGameRules::State_Leave_PREROUND( void )
{
	PreRound_End();
}


void CTFGameRules::State_Think_PREROUND( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( IsInArenaMode() )
		{
			if ( IsInWaitingForPlayers() )
			{
				if ( IsInTournamentMode() )
				{
					// Check round restart.
					CheckReadyRestart();
					State_Transition( GR_STATE_STALEMATE );
				}

				return;
			}

			State_Transition( GR_STATE_STALEMATE );

			// Hide the class composition panel.
		}
		else
		{
			State_Transition( GR_STATE_RND_RUNNING );
		}
	}

	CheckRespawnWaves();
}


void CTFGameRules::State_Enter_RND_RUNNING( void )
{
	SetupOnRoundRunning();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_active" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if ( !IsInWaitingForPlayers() )
	{
		PlayStartRoundVoice();

		// Only do this in VIP mode and make sure we have a VIP as well before sending out the signal to start a short tutorial sequence.
		if ( IsVIPMode() )
		{
			int i = FIRST_GAME_TEAM;
			for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
			{
				CTFTeam *pTFTeam = static_cast<CTFTeam *>( pTeam );
				if ( !pTFTeam )
					continue;

				CTFPlayer *pVIP = pTFTeam->GetVIP();
				if ( pVIP )
				{
					IGameEvent *pVIPTutorialEvent = gameeventmanager->CreateEvent( "vip_tutorial" );
					if ( pVIPTutorialEvent )
					{
						pVIPTutorialEvent->SetInt( "userid", pVIP->GetUserID() );
						pVIPTutorialEvent->SetInt( "team", i );

						gameeventmanager->FireEvent( pVIPTutorialEvent );
					}
				}
			}
		}
	}

	m_bChangeLevelOnRoundEnd = false;
	m_bPrevRoundWasWaitingForPlayers = false;

	m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
}


void CTFGameRules::State_Think_RND_RUNNING( void )
{
	// If we don't find any active players, return to GR_STATE_PREGAME.
	if ( CountActivePlayers() <= 0 )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate.
			g_pReplay->SV_EndRecordingSession();
		}
#endif

#ifdef TF_DLL
		// Mass time-out? Clean everything up...
		if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		{
			TFGameRules()->EndCompetitiveMatch();
			return;
		}
#endif // TF_DLL

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( m_flNextBalanceTeamsTime < gpGlobals->curtime )
	{
		BalanceTeams( true );
		m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
	}

	CheckRespawnWaves();

	// Check round restart.
	CheckReadyRestart();

	// See if we're coming up to the server timelimit, in which case force a stalemate immediately.
	if ( mp_timelimit.GetInt() > 0 && !IsInPreMatch() && GetTimeLeft() <= 0 )
	{
		if ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) )
		{
			int iDrawScoreCheck = -1;
			int iWinningTeam = 0;
			bool bTeamsAreDrawn = true;
			for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c && bTeamsAreDrawn; i++ )
			{
				int iTeamScore = GetGlobalTeam( i )->GetScore();
				if ( iTeamScore > iDrawScoreCheck )
				{
					iWinningTeam = i;
				}

				if ( iTeamScore != iDrawScoreCheck )
				{
					if ( iDrawScoreCheck == -1 )
					{
						iDrawScoreCheck = iTeamScore;
					}
					else
					{
						bTeamsAreDrawn = false;
					}
				}
			}

			if ( bTeamsAreDrawn )
			{
				if ( CanGoToStalemate() )
				{
					m_bChangelevelAfterStalemate = true;
					SetStalemate( STALEMATE_SERVER_TIMELIMIT, m_bForceMapReset );
				}
				else
				{
					SetOvertime( true );
				}
			}
			else
			{
				SetWinningTeam( iWinningTeam, WINREASON_TIMELIMIT, true, false, true );
			}
		}
	}

	StopWatchModeThink();
}


void CTFGameRules::State_Leave_RND_RUNNING( void )
{
	if ( tf2c_log_item_details.GetBool() )
	{
		for ( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			CTFPlayer* pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pTFPlayer )
				continue;

			LogLifeStats( pTFPlayer );
		}
	}
}


void CTFGameRules::State_Enter_TEAM_WIN( void )
{
	m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime();

	// If we're forcing the map to reset it must be the end of a "full" round not a mini-round.
	if ( m_bForceMapReset )
	{
		m_nRoundsPlayed++;
	}

	InternalHandleTeamWin( m_iWinningTeam );

	SendWinPanelInfo();

	// For the Entrepreneur Achievement.
	if ( IsVIPMode() )
	{
		for ( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			if ( pPlayer->GetTeamNumber() != GetWinningTeam() )
				continue;

			if ( !pPlayer->PlayerHasDiedBefore() && pPlayer->IsPlayerClass( TF_CLASS_CIVILIAN, true ) )
			{
				pPlayer->AwardAchievement( TF2C_ACHIEVEMENT_WIN_CIVILIAN_NODEATHS );
			}
		}
	}

#ifdef TF_DLL
	// Do this now, so players don't leave before the usual CheckWinLimit() call happens.
	bool bDone = ( CheckTimeLimit( false ) || CheckWinLimit( false ) || CheckMaxRounds( false ) || CheckNextLevelCvar( false ) );
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() && bDone )
	{
		TFGameRules()->StopCompetitiveMatch( CMsgGC_Match_Result_Status_MATCH_SUCCEEDED );
	}
#endif // TF_DLL
}


void CTFGameRules::State_Think_TEAM_WIN( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		// Check the win limit, max rounds, time limit and nextlevel cvar before starting the next round.
		if ( !( CheckTimeLimit() || CheckWinLimit() || CheckMaxRounds() || CheckNextLevelCvar() ) )
		{
			PreviousRoundEnd();

			if ( ShouldGoToBonusRound() )
			{
				State_Transition( GR_STATE_BONUS );
			}
			else
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();.
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
		}
		else if ( IsInTournamentMode() )
		{
			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( !pPlayer )
					continue;

				pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
			}

			RestartTournament();

			if ( IsInArenaMode() )
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate.
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
#ifdef TF_DLL
			else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_pPopulationManager )
			{
				// One of the convars mp_timelimit, mp_winlimit, mp_maxrounds, or nextlevel has been triggered.
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				g_pPopulationManager->SetMapRestartTime( gpGlobals->curtime + 10.0f );
				State_Enter( GR_STATE_GAME_OVER );
				return;
			}
			else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() )
			{
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				State_Enter( GR_STATE_GAME_OVER );
				m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime( true );
				return;
			}
#endif // TF_DLL
			else
			{
				State_Transition( GR_STATE_RND_RUNNING );
			}
		}
	}
}


void CTFGameRules::State_Enter_STALEMATE( void )
{
	m_flStalemateStartTime = gpGlobals->curtime;
	SetupOnStalemateStart();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
		m_hStalemateTimer = NULL;
	}

	int iTimeLimit = mp_stalemate_timelimit.GetInt();

	if ( IsInArenaMode() )
	{
		iTimeLimit = tf_arena_round_time.GetInt();

		CArenaLogic* pArena = dynamic_cast<CArenaLogic*>(gEntList.FindEntityByClassname(NULL, "tf_logic_arena"));
		if (pArena && pArena->m_iRoundTimeOverride >= 0)
			iTimeLimit = pArena->m_iRoundTimeOverride;
	}

	if ( iTimeLimit > 0 )
	{
		variant_t sVariant;
		if ( !m_hStalemateTimer )
		{
			m_hStalemateTimer = (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
			m_hStalemateTimer->SetName( AllocPooledString( "zz_stalemate_timer" ) );
			m_hStalemateTimer->SetShowInHud( true );
		}

		sVariant.SetInt( iTimeLimit );
		m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}


void CTFGameRules::State_Leave_STALEMATE( void )
{
	SetupOnStalemateEnd();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
	}

	if ( !IsInArenaMode() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		CArenaLogic *pArena = dynamic_cast<CArenaLogic *>( gEntList.FindEntityByClassname(NULL, "tf_logic_arena") );
		if( pArena )
			pArena->m_OnTimeLimitEnd.FireOutput( NULL, NULL );
	}
}


void CTFGameRules::State_Enter_BONUS( void )
{
	SetupOnBonusStart();
}


void CTFGameRules::State_Leave_BONUS( void )
{
	SetupOnBonusEnd();
}


void CTFGameRules::State_Think_BONUS( void )
{
	BonusStateThink();
}


void CTFGameRules::State_Enter_BETWEEN_RNDS( void )
{
	BetweenRounds_Start();
}


void CTFGameRules::State_Leave_BETWEEN_RNDS( void )
{
	BetweenRounds_End();
}


void CTFGameRules::State_Think_BETWEEN_RNDS( void )
{
	BetweenRounds_Think();
}


void CTFGameRules::State_Think_STALEMATE( void )
{
	// If we don't find any active players, return to GR_STATE_PREGAME.
	if ( CountActivePlayers() <= 0 && !IsInArenaMode() )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate.
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( IsInTournamentMode() && IsInWaitingForPlayers() )
	{
		CheckReadyRestart();
		CheckRespawnWaves();
		return;
	}

	// If a game has more than 2 active teams, the old function won't work.
	// Which is why we had to replace it with this one.
	CUtlVector<CTeam *> pAliveTeams;

	// Last team standing wins.
	for ( int i = LAST_SHARED_TEAM + 1, c = GetNumberOfTeams(); i < c; i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		Assert( pTeam );

		int iPlayers = pTeam->GetNumPlayers();
		if ( iPlayers )
		{
			bool bFoundLiveOne = false;
			for ( int player = 0; player < iPlayers; player++ )
			{
				if ( pTeam->GetPlayer( player ) && pTeam->GetPlayer( player )->IsAlive() )
				{
					bFoundLiveOne = true;
					break;
				}
			}

			if ( bFoundLiveOne )
			{
				pAliveTeams.AddToTail( pTeam );
			}
			else
			{
				if ( IsInArenaMode() && !m_bArenaTeamHealthKitsTeamDied[i] )
				{
					bool bAtLeastOne = false;

					// Reenable all the health packs we disabled.
					FOR_EACH_VEC(m_hArenaDroppedTeamHealthKits, j)
					{
						if ( m_hArenaDroppedTeamHealthKits[j] && m_hArenaDroppedTeamHealthKits[j]->GetTeamNumber() == pTeam->GetTeamNumber() )
						{
							m_hArenaDroppedTeamHealthKits[j]->m_nSkin = 4; // global team, grey
							m_hArenaDroppedTeamHealthKits[j]->ChangeTeam( TEAM_UNASSIGNED );
							bAtLeastOne = true;
						}
					}

					if( bAtLeastOne )
						m_bArenaTeamHealthKitsTeamDied[i] = true;
				}
			}
		}
	}

	if ( pAliveTeams.Count() == 1 )
	{
		// The live team has won. 
		int iAliveTeam = pAliveTeams[0]->GetTeamNumber();
		bool bMasterHandled = false;
		if ( !m_bForceMapReset )
		{
			// We're not resetting the map, so give the winners control
			// of all the points that were in play this round.
			// Find the control point master.
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster )
			{
				variant_t sVariant;
				sVariant.SetInt( iAliveTeam );
				pMaster->AcceptInput( "SetWinnerAndForceCaps", NULL, NULL, sVariant, 0 );
				bMasterHandled = true;
			}
		}

		if ( !bMasterHandled )
		{
			bool iResetByArena = false;

			if ( IsInArenaMode() )
			{
				CArenaLogic* pArena = dynamic_cast<CArenaLogic*>(gEntList.FindEntityByClassname(NULL, "tf_logic_arena"));
				if (pArena)
					iResetByArena = tf2c_arena_swap_teams.GetBool() || pArena->m_bSwitchTeamsOnWin;
			}

			SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD, m_bForceMapReset, iResetByArena );
		}
	}
	else if ( pAliveTeams.Count() == 0 || ( m_hStalemateTimer && TimerMayExpire() && m_hStalemateTimer->GetTimeRemaining() <= 0 ) )
	{
		bool bFullReset = true;

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster && pMaster->PlayingMiniRounds() )
		{
			// We don't need to do a full map reset for maps with mini-rounds.
			bFullReset = false;
		}

		// Both teams are dead. Pure stalemate.
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bFullReset, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: manual restart
//-----------------------------------------------------------------------------
void CTFGameRules::State_Enter_RESTART( void )
{
	// Send scores.
	SendTeamScoresEvent();

	// Send restart event.
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_restart_round" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	m_bPrevRoundWasWaitingForPlayers = m_bInWaitingForPlayers;
	SetInWaitingForPlayers( false );

	ResetScores();

	// Reset the round time.
	ResetMapTime();

	State_Transition( GR_STATE_PREROUND );
}


void CTFGameRules::State_Think_RESTART( void )
{
	// Should never get here, State_Enter_RESTART sets us into a different state.
	Assert( 0 );
}


void CTFGameRules::ResetTeamsRoundWinTracking( void )
{
	for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
	{
		GetGlobalTFTeam( i )->ResetWins();
	}
}


void CTFGameRules::InitTeams( void )
{
	// Clear the player class data.
	ResetFilePlayerClassInfoDatabase();
}


int CTFGameRules::CountActivePlayers( void )
{
	int i, count = 0;

	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		if ( pPlayer->IsReadyToPlay() )
		{
			++count;
		}
	}

	if ( IsInArenaMode() )
	{
		Assert( !IsFourTeamGame() );

		int count_arena = 0;

		for ( i = 1; i <= MAX_PLAYERS; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if ( !pPlayer->IsHLTV() && !pPlayer->IsReplay() )
			{
				++count_arena;
			}
		}

		if ( m_ArenaPlayerQueue.Count() >= 2 )
			return count_arena;

		if ( count_arena < 2 )
			return 0;

		// We have to check if we have atleast 2 teams with atleast one player.
		int iActiveTeams = 0;
		
		// g_Teams scales in 4team/2team mode to house the amount existing teams.
		FOR_EACH_VEC( g_Teams, iTeam )
		{
			// Exclude spectator and unassigned and check if the team has players. 
			if ( iTeam >= TF_TEAM_RED && g_Teams[iTeam]->GetNumPlayers() > 0 ) 
			{
				iActiveTeams++;
			}
		}
		
		if ( iActiveTeams < 2 )
			return 0;
	}

	return count;
}


void CTFGameRules::RoundRespawn( void )
{
	int i;

	// Remove any buildings, grenades, rockets, etc. the player put into the world.
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}
	}

	if ( !IsInTournamentMode() )
	{
		Arena_RunTeamLogic();
	}

	// Reset the flag captures.
	for ( int iTeam = FIRST_GAME_TEAM, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
		pTeam->ResetRoundScore();
	}

	m_flNextDominationThink = gpGlobals->curtime;

	CTF_GameStats.ResetRoundStats();

	m_flRoundStartTime = gpGlobals->curtime;

	if ( m_bForceMapReset || m_bPrevRoundWasWaitingForPlayers )
	{
		CleanUpMap();

		// Clear out the previously played rounds.
		m_iszPreviousRounds.RemoveAll();

		if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0 )
		{
			// Check that we have an active timer in the HUD and use mp_timelimit if we don't.
			if ( !MapHasActiveTimer() )
			{
				CreateTimeLimitTimer();
			}
		}

		m_iLastCapPointChanged = 0;
	}

	// Reset our spawn times to the original values.
	for ( i = 0; i < MAX_TEAMS; i++ )
	{
		if ( m_flOriginalTeamRespawnWaveTime[i] >= 0 )
		{
			m_TeamRespawnWaveTimes.Set( i, m_flOriginalTeamRespawnWaveTime[i] );
		}
	}

	if ( !IsInWaitingForPlayers() )
	{
		if ( m_bForceMapReset )
		{
			UTIL_LogPrintf( "World triggered \"Round_Start\"\n" );
		}
	}

	// Setup before respawning players, so we can mess with spawnpoints.
	SetupOnRoundStart();

	// Do we need to switch the teams?
	m_bSwitchedTeamsThisRound = false;
	if ( ShouldSwitchTeams() )
	{
		m_bSwitchedTeamsThisRound = true;
		HandleSwitchTeams();
		SetSwitchTeams( false );
	}

	// Do we need to switch the teams?
	if ( ShouldScrambleTeams() )
	{
		HandleScrambleTeams();
		SetScrambleTeams( false );
	}

#if defined( REPLAY_ENABLED )
	bool bShouldWaitToStartRecording = ShouldWaitToStartRecording();
	if ( g_pReplay && g_pReplay->SV_ShouldBeginRecording( bShouldWaitToStartRecording ) )
	{
		// Tell the replay manager that it should begin recording the new round as soon as possible.
		g_pReplay->SV_GetContext()->GetSessionRecorder()->StartRecording();
	}
#endif

	// Free any edicts that were marked deleted. This should hopefully clear some out
	// so the below function can use the now freed ones.
	engine->AllowImmediateEdictReuse();

	RespawnPlayers( true );

	// Reset per-round scores for each player.
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->ResetPerRoundStats();
		}
	}

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL.
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}

	if ( m_bVoteMapOnNextRound )
	{
		// Vote has been scheduled.
		StartNextMapVote();
	}
}


void CTFGameRules::CleanUpMap( void )
{
	if ( mp_showcleanedupents.GetInt() )
	{
		Msg( "CleanUpMap\n===============\n" );
		Msg( "  Entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );
	}

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		if ( !RoundCleanupShouldIgnore( pCur ) )
		{
			if ( mp_showcleanedupents.GetInt() & 1 )
			{
				Msg( "Removed Entity: %s\n", pCur->GetClassname() );
			}
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Clear out the event queue.
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	engine->AllowImmediateEdictReuse();

	if ( mp_showcleanedupents.GetInt() & 2 )
	{
		Msg( "  Entities Left:\n" );
		pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			Msg( "  %s (%d)\n", pCur->GetClassname(), pCur->entindex() );
			pCur = gEntList.NextEnt( pCur );
		}
	}

	// Now reload the map entities.
	class CTeamplayMapEntityFilter : public IMapEntityFilter
	{
	public:
		CTeamplayMapEntityFilter()
		{
			m_pRules = TFGameRules();
		}

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( m_pRules->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator ); // Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
		CTFGameRules *m_pRules;

	};
	CTeamplayMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!
	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );

	CHLTVDirector *pHLTVDirector = HLTVDirector();
	if ( pHLTVDirector )
	{
		pHLTVDirector->BuildCameraList();
	}

	// Process event queue now, we want all inputs to be received before players spawn.
	g_EventQueue.ServiceEvents();
}


void CTFGameRules::CheckRespawnWaves( void )
{
	for ( int iTeam = LAST_SHARED_TEAM + 1, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; iTeam++ )
	{
		if ( m_flNextRespawnWave[iTeam] && m_flNextRespawnWave[iTeam] > gpGlobals->curtime )
			continue;

		RespawnTeam( iTeam );

		// Set m_flNextRespawnWave to 0 when we don't have a respawn time to reduce networking.
		float flNextRespawnLength = GetRespawnWaveMaxLength( iTeam );
		if ( flNextRespawnLength )
		{
			m_flNextRespawnWave.Set( iTeam, gpGlobals->curtime + flNextRespawnLength );
		}
		else
		{
			m_flNextRespawnWave.Set( iTeam, 0.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected (user ID).
//-----------------------------------------------------------------------------
static int SwitchPlayersSort( CBaseMultiplayerPlayer *const *p1, CBaseMultiplayerPlayer *const *p2 )
{
	// Sort by score.
	return ( *p2 )->GetTeamBalanceScore() - ( *p1 )->GetTeamBalanceScore();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the teams are balanced after this function.
//-----------------------------------------------------------------------------
void CTFGameRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( !mp_autoteambalance.GetBool() || IsInArenaQueueMode() )
		return;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return;
#endif // _DEBUG || STAGING_ONLY

	if ( IsInTraining() || IsInItemTestingMode() )
		return;

	// We don't balance for a period of time at the start of the game.
	if ( gpGlobals->curtime < m_flStartBalancingTeamsAt )
		return;

	// Wrap with this bool, indicates it's a round running switch and not a between rounds insta-switch.
	if ( bRequireSwitcheesToBeDead )
	{
		// We don't balance if there is less than 60 seconds on the active timer.
		CTeamRoundTimer *pActiveTimer = GetActiveRoundTimer();
		if ( pActiveTimer && pActiveTimer->GetTimeRemaining() < 60 )
			return;
	}

	// Figure out if we're unbalanced.
	int iHeaviestTeam = TEAM_UNASSIGNED, iLightestTeam = TEAM_UNASSIGNED;
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
	{
		m_flFoundUnbalancedTeamsTime = -1;
		m_bPrintedUnbalanceWarning = false;
		return;
	}

	if ( m_flFoundUnbalancedTeamsTime < 0.0f )
	{
		m_flFoundUnbalancedTeamsTime = gpGlobals->curtime;
	}

	// If teams have been unbalanced for X seconds, play a warning.
	if ( !m_bPrintedUnbalanceWarning && ( ( gpGlobals->curtime - m_flFoundUnbalancedTeamsTime ) > 1.0f ) )
	{
		// Print unbalance warning.
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_AutoBalance_Warning" );
		m_bPrintedUnbalanceWarning = true;
	}

	// Teams are unblanced, figure out some players that need to be switched.
	DevMsg( "Auto-balanced started\n" );
	bool bTeamsChanged;

	do
	{
		CTeam *pHeavyTeam = GetGlobalTeam( iHeaviestTeam );
		CTeam *pLightTeam = GetGlobalTeam( iLightestTeam );

		Assert( pHeavyTeam && pLightTeam );

		bTeamsChanged = false;

		int iNumSwitchesRequired = ( pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers() ) / 2;

#ifndef CLIENT_DLL
		CTFTeam *pTFTeam = dynamic_cast<CTFTeam *>( pLightTeam );
		if ( IsVIPMode() && pTFTeam && pTFTeam->GetVIP() )
		{
			iNumSwitchesRequired = ( pHeavyTeam->GetNumPlayers() - ( pLightTeam->GetNumPlayers() == 1 ? 0 : pLightTeam->GetNumPlayers() - 1 ) ) / 2;		
		}
#endif

		// Sort the eligible players and switch the n best candidates.
		CUtlVector<CBaseMultiplayerPlayer *> vecPlayers;

		CBaseMultiplayerPlayer *pPlayer;

		int i, c;
		for ( i = 0, c = pHeavyTeam->GetNumPlayers(); i < c; i++ )
		{
			pPlayer = ToBaseMultiplayerPlayer( pHeavyTeam->GetPlayer( i ) );
			if ( !pPlayer )
				continue;

			if ( !pPlayer->CanBeAutobalanced() )
				continue;

			// Calculate a score for this player. higher is more likely to be switched.
			pPlayer->SetTeamBalanceScore( pPlayer->CalculateTeamBalanceScore() );

			vecPlayers.AddToTail( pPlayer );
		}

		// Sort the vector.
		vecPlayers.Sort( SwitchPlayersSort );

		DevMsg( "Taking players from %s to %s. %d switches required.\n", pHeavyTeam->GetName(), pLightTeam->GetName(), iNumSwitchesRequired );

		int iNumCandiates = iNumSwitchesRequired + 2, nPlayerTeamBalanceScore, iOldLightest, iOldHeaviest;
		IGameEvent *event;

		for ( i = 0, c = vecPlayers.Count(); i < c && iNumSwitchesRequired > 0 && i < iNumCandiates; i++ )
		{
			pPlayer = vecPlayers.Element( i );
			Assert( pPlayer );
			if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
				continue;

			if ( !bRequireSwitcheesToBeDead || !pPlayer->IsAlive() )
			{
				// We're trying to avoid picking a player that's recently
				// been auto-balanced by delaying their selection in the hope
				// that a better candidate comes along.
				if ( bRequireSwitcheesToBeDead )
				{
					nPlayerTeamBalanceScore = pPlayer->CalculateTeamBalanceScore();

					// Do we already have someone in the queue?
					if ( m_nAutoBalanceQueuePlayerIndex > 0 )
					{
						// Is this player's score worse?
						if ( nPlayerTeamBalanceScore < m_nAutoBalanceQueuePlayerScore )
						{
							m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
							m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
						}
					}
					// Has this person been switched recently?
					else if ( nPlayerTeamBalanceScore < -10000 )
					{
						// Put them in the queue.
						m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
						m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
						m_flAutoBalanceQueueTimeEnd = gpGlobals->curtime + 3.0f;

						continue;
					}

					// If this is the player in the queue...
					if ( m_nAutoBalanceQueuePlayerIndex == pPlayer->entindex() )
					{
						// Pass until their timer is up.
						if ( m_flAutoBalanceQueueTimeEnd > gpGlobals->curtime )
							continue;
					}
				}

				pPlayer->ChangeTeam( iLightestTeam, false, true );
				pPlayer->SetLastForcedChangeTeamTimeToNow();

				m_nAutoBalanceQueuePlayerScore = -1;
				m_nAutoBalanceQueuePlayerIndex = -1;

				// Tell people that we've switched this player.
				event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
				if ( event )
				{
					event->SetInt( "player", pPlayer->entindex() );
					event->SetInt( "team", iLightestTeam );
					gameeventmanager->FireEvent( event );
				}

				iNumSwitchesRequired--;

				DevMsg( "Moved %s(%d) to team %s, %d switches remaining\n", pPlayer->GetPlayerName(), i, pLightTeam->GetName(), iNumSwitchesRequired );

				iOldLightest = iLightestTeam;
				iOldHeaviest = iHeaviestTeam;

				// Re-calculate teams unbalanced state after each swap.
				if ( AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
				{
					if ( iHeaviestTeam != iOldHeaviest || iLightestTeam != iOldLightest )
					{
						// Recalculate players to be swapped.
						bTeamsChanged = true;
						break;
					}
				}
			}
		}
	}
	while ( bTeamsChanged );
}


void CTFGameRules::ResetScores( void )
{
	int i, c;
	if ( m_bResetTeamScores )
	{
		for ( i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			GetGlobalTeam( i )->ResetScores();
		}

		m_iWinLimitBonus = 0;
	}

	if ( m_bResetPlayerScores )
	{
		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			if ( FNullEnt( pPlayer->edict() ) )
				continue;

			pPlayer->ResetScores();
		}
	}

	if ( m_bResetRoundsPlayed )
	{
		m_nRoundsPlayed = 0;
		m_iMaxRoundsBonus = 0;
	}

	// Assume we always want to reset the scores,
	// unless someone tells us not to for the next reset.
	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	//m_flStopWatchTime = -1.0f;

	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_reset" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}


void CTFGameRules::ResetMapTime( void )
{
	if( !tf2c_allow_maptime_reset.GetBool() )
		return;

	m_flMapResetTime = gpGlobals->curtime;
	m_iMapTimeBonus = 0;

	m_bVoteMapOnNextRound = false;
	m_bVotedForNextMap = false;

	// Send an event with the time remaining until map change.
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_map_time_remaining" );
	if ( event )
	{
		event->SetInt( "seconds", GetTimeLeft() );
		gameeventmanager->FireEvent( event );
	}
}


void CTFGameRules::PlayStartRoundVoice( void )
{
	if ( IsInSpecialDeliveryMode() )
	{
		if ( !IsInWaitingForPlayers() )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_SD_ROUNDSTART );
		}
	}

	/*for ( int i = LAST_SHARED_TEAM + 1, c = GetNumberOfTeams(); i < c; i++ )
	{
		BroadcastSound( i, UTIL_VarArgs( "Game.TeamRoundStart%d", i ) );
	}*/
}


void CTFGameRules::PlayWinSong( int team )
{
	if ( IsInSpecialDeliveryMode() )
		return;

	if ( team == TEAM_UNASSIGNED )
	{
		PlayStalemateSong();
	}
	else
	{
		//BroadcastSound( TEAM_UNASSIGNED, UTIL_VarArgs( "Game.TeamWin%d", team ) );

		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			g_TFAnnouncer.Speak( i, i == team ? TF_ANNOUNCER_VICTORY : TF_ANNOUNCER_DEFEAT );
		}
	}
}


void CTFGameRules::PlaySuddenDeathSong( void )
{
	g_TFAnnouncer.Speak( TF_ANNOUNCER_SUDDENDEATH );
}


void CTFGameRules::PlayStalemateSong( void )
{
	g_TFAnnouncer.Speak( TF_ANNOUNCER_STALEMATE );
}


void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// Remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed).
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
			continue;

		if ( pPlayer->GetTeamNumber() != iWinningTeam )
		{
			pPlayer->RemoveInvisibility();
			//pPlayer->RemoveDisguise();
			pPlayer->ClearExpression();

			pPlayer->DropFlag();

			// Hide their weapon.
			CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
			if ( pWeapon )
			{
				pWeapon->SetWeaponVisible( false );
			}
		}
		else
		{
			pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_BONUS_TIME );
		}

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->TeamFortress_SetGravity();
	}

	// Disable any sentry guns the losing team has built.
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "obj_sentrygun" ) ) != NULL )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( pEnt );
		if ( pSentry )
		{
			if ( pSentry->GetTeamNumber() != iWinningTeam )
			{
				pSentry->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
		m_bFirstBlood = false;
	}
}


bool CTFGameRules::MapHasActiveTimer( void )
{
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" ) ) != NULL )
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer *>( pEntity );
		if ( pTimer && pTimer->ShowInHud() && ( Q_stricmp( STRING( pTimer->GetEntityName() ), "zz_teamplay_timelimit_timer" ) != 0 ) )
			return true;
	}

	return false;
}


void CTFGameRules::CreateTimeLimitTimer( void )
{
	if ( IsInArenaMode() || IsInKothMode() )
		return;

	// This is the same check we use in State_Think_RND_RUNNING()
	// don't show the timelimit timer if we're not going to end the map when it runs out.
	bool bAllowStalemate = ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) );
	if ( !bAllowStalemate )
		return;

	if ( !m_hTimeLimitTimer )
	{
		m_hTimeLimitTimer = (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		m_hTimeLimitTimer->SetName( AllocPooledString( "zz_teamplay_timelimit_timer" ) );
		m_hTimeLimitTimer->SetShowInHud( true );
	}

	variant_t sVariant;
	sVariant.SetInt( GetTimeLeft() );
	m_hTimeLimitTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
}

// Skips players except for the specified one.
class CTraceFilterHitPlayer : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterHitPlayer, CTraceFilterSimple );

	CTraceFilterHitPlayer( const IHandleEntity *passentity, IHandleEntity *pHitEntity, int collisionGroup, int iTeamNum )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		m_pHitEntity = pHitEntity;
		m_iTeamNum = iTeamNum;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( !pEntity )
			return false;

		if ( pEntity->IsPlayer() && pEntity != m_pHitEntity )
			return false;

		if ( pEntity->IsBaseObject() && pEntity->GetTeamNumber() == m_iTeamNum )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pHitEntity;
	int	m_iTeamNum;
};

CTFRadiusDamageInfo::CTFRadiusDamageInfo()
{
	m_flRadius = 0.0f;
	m_iClassIgnore = CLASS_NONE;
	m_pEntityIgnore = NULL;
	m_flSelfDamageRadius = 0.0f;
	m_bStockSelfDamage = true;
}

bool CTFRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity )
{
	if (!pEntity)
	{
		return false;
	}
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&( ~CONTENTS_HITBOX );
	trace_t tr;
	float falloff;
	Vector vecSpot;

	int ibIgnoreFalloff = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), ibIgnoreFalloff, disable_explosion_falloff );
	
	if ( info.GetDamageType() & DMG_HALF_FALLOFF || ( pEntity == info.GetAttacker() && info.GetWeapon() ) )
	{
		// Always use 0.5 for self-damage so that rocket jumping is not screwed up.
		falloff = 0.5;
	}
	else
	{
		//falloff = 0.0;
		falloff = ibIgnoreFalloff;
	}

	CBaseEntity *pInflictor = info.GetInflictor();

	//float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// This value is used to scale damage when the explosion is blocked by some other object.
	float flBlockedDamagePercent = 0.0f;

	// Check that the explosion can 'see' this entity, trace through players.
	vecSpot = pEntity->BodyTarget( m_vecSrc, false );
	CTraceFilterHitPlayer filter( info.GetInflictor(), pEntity, COLLISION_GROUP_PROJECTILE, info.GetInflictor()->GetTeamNumber() );
	UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );
	if ( tr.fraction != 1.0f && tr.m_pEnt != pEntity )
		return false;

	// Adjust the damage - apply falloff.
	float flAdjustedDamage = 0.0f;

	float flDistanceToEntity;

	// Rockets store the ent they hit as the enemy and have already
	// dealt full damage to them by this time.
	if ( pInflictor && pEntity == pInflictor->GetEnemy() )
	{
		// Full damage, we hit this entity directly.
		flDistanceToEntity = 0;
	}
	else if ( pEntity->IsPlayer() )
	{
		// Use whichever is closer, absorigin or worldspacecenter.
		flDistanceToEntity = Min( ( m_vecSrc - pEntity->WorldSpaceCenter() ).Length(), ( m_vecSrc - pEntity->GetAbsOrigin() ).Length() );
	}
	else
	{
		flDistanceToEntity = ( m_vecSrc - tr.endpos ).Length();
	}

	flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, m_flRadius, info.GetDamage(), info.GetDamage() * falloff );

	// Take a little less damage from yourself
	if ( pEntity == info.GetAttacker() )
	{
		CTFPlayer* pPlayer = ToTFPlayer(pEntity);

		if (!pPlayer)
		{
			return false;
		}

		if (!pPlayer->IsPlayerClass(TF_CLASS_SOLDIER, true)) // Soldier uses his own set of self-damage rules somewhere else
		{
			flAdjustedDamage = flAdjustedDamage * 0.75f;
		}
	}
	else if (pInflictor && pEntity != pInflictor->GetEnemy())
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), flAdjustedDamage, mult_explosion_splash_damage);
	}

	if ( flAdjustedDamage <= 0 )
		return false;

	// The explosion can 'see' this entity, so hurt them!
	if ( tr.startsolid )
	{
		// If we're stuck inside them, fixup the position and distance.
		tr.endpos = m_vecSrc;
		tr.fraction = 0.0f;
	}

	CTakeDamageInfo adjustedInfo = info;
	//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
	adjustedInfo.SetDamage( flAdjustedDamage - ( flAdjustedDamage * flBlockedDamagePercent ) );

	// Now make a consideration for skill level!
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
	{
		// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
		adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
	}

	Vector dir = vecSpot - m_vecSrc;
	VectorNormalize( dir );

	// If we don't have a damage force, manufacture one.
	if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
	{
		CalculateExplosiveDamageForce( &adjustedInfo, dir, m_vecSrc );
	}
	else
	{
		// Assume the force passed in is the maximum force. Decay it based on falloff.
		float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
		adjustedInfo.SetDamageForce( dir * flForce );
		adjustedInfo.SetDamagePosition( m_vecSrc );
	}

	if ( tr.fraction != 1.0f && pEntity == tr.m_pEnt )
	{
		adjustedInfo.SetDamagePosition( tr.endpos );

		ClearMultiDamage();
		pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
		ApplyMultiDamage();
	}
	else
	{
		pEntity->TakeDamage( adjustedInfo );
	}

	// Now hit all triggers along the way that respond to damage... 
	pEntity->TraceAttackToTriggers( adjustedInfo, m_vecSrc, tr.endpos, dir );

	return true;
}



void CTFGameRules::RadiusDamage( CTFRadiusDamageInfo &radiusInfo )
{
	CTakeDamageInfo &info = radiusInfo.info;
	CBaseEntity *pAttacker = info.GetAttacker();
	int iPlayersDamaged = 0;

	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity == radiusInfo.m_pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: This should check a damage mask, not an ignore.
		if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore )
			continue;

		// Skip the attacker as we'll handle him separately.
		if ( pEntity == pAttacker )
			continue;

		// Checking distance from source because Valve were apparently too lazy to fix the engine function.
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
		if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) > Square( radiusInfo.m_flRadius ) )
			continue;

		if ( radiusInfo.ApplyToEntity( pEntity ) )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pEntity );
			if ( pPlayer && pPlayer->IsEnemy( pAttacker ) )
			{
				iPlayersDamaged++;
			}
		}
	}

	info.SetDamagedOtherPlayers( iPlayersDamaged );

	// For attacker, radius and damage need to be consistent so custom weapons don't screw up rocket jumping.
	if ( pAttacker )
	{
		if ( pAttacker == radiusInfo.m_pEntityIgnore )
			return;

		if ( radiusInfo.m_bStockSelfDamage )
		{
			// Get stock damage.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				info.SetDamage( pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage );
			}
		}

		if ( radiusInfo.m_flSelfDamageRadius )
		{
			// Use stock radius.
			radiusInfo.m_flRadius = radiusInfo.m_flSelfDamageRadius;
		}

		Vector vecHitPoint;
		pAttacker->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );

		if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) <= Square( radiusInfo.m_flRadius ) )
		{
			radiusInfo.ApplyToEntity( pAttacker );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = info;
	radiusInfo.m_vecSrc = vecSrcIn;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_iClassIgnore = iClassIgnore;
	radiusInfo.m_pEntityIgnore = pEntityIgnore;
	RadiusDamage( radiusInfo );
}

CTFRadiusHealingInfo::CTFRadiusHealingInfo()
{
	m_flRadius = 0.0f;
	m_flSelfHealRadius = 0.0f;
	m_iClassIgnore = CLASS_NONE;
	m_hEntityIgnore = NULL;
	m_flHealingAmountRadial = 0.0f;
	m_flHealingAmountDirect = 0.0f;
	m_iClassIgnore = CLASS_NONE;
	m_bUseFalloffCalcs = false;
	m_condConditionsToApply = TF_COND_INVALID;
	m_flConditionDuration = -1.0f;
	m_bSelfHeal = false;
	m_hEntityDirectlyHit = NULL;
	m_hPlayerResponsible = NULL;
	m_flOverhealMultiplier = 1.0f;
	m_flResidualHealDuration = 0.0f;
	m_flResidualHealRate = 0.0f;
	m_bApplyResidualHeal = false;
}


bool CTFRadiusHealingInfo::ApplyToEntity(CBaseEntity *pEntity)
{
	// Early out if self healing is disallowed and it's us we're trying to heal. (Or we're trying to heal an enemy)
	if (pEntity == m_hPlayerResponsible && !m_bSelfHeal)
		return false;
	
	CTFPlayer *pPlayer = ToTFPlayer(pEntity);
	CTFPlayer *pPlayerResponsible = ToTFPlayer(m_hPlayerResponsible);
	// Semi-hack: Ignore this entity if they're not a player.
	if (!pPlayer)
		return false;
	
	if ( pPlayer->IsEnemy( m_hPlayerResponsible ) && !pPlayer->m_Shared.DisguiseFoolsTeam( pPlayerResponsible->GetTeamNumber() ) )
	{
		DevMsg("Heal failed! Was not an ally or disguised spy.\n");
		return false;		
	}

	const int MASK_RADIUS_HEAL = MASK_SHOT&(~CONTENTS_HITBOX);
	trace_t tr;
	float falloff = 0.5f;
	Vector vecSpot;

	bool bWasDirect = false;

	// Check that the healsplosion can see this entity, trace through players.
	vecSpot = pEntity->BodyTarget(m_vecSrc, false);
	//CTraceFilterHitPlayer filter( m_hPlayerResponsible, pEntity, COLLISION_GROUP_PROJECTILE, m_hPlayerResponsible->GetTeamNumber() );
	CTraceFilterHitPlayer filter( m_hPlayerResponsible, pEntity, COLLISION_GROUP_DEBRIS, m_hPlayerResponsible->GetTeamNumber() );
	UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_HEAL, &filter, &tr );

	if ( tr.fraction != 1.0f && tr.m_pEnt != pEntity ) {
		DevMsg( "Heal failed! Trace did not intersect the entity in question.\n" );  return false;
	}

	// By default, just use the plain healing amount.
	float flAdjustedHealing = m_flHealingAmountRadial;

	float flDistanceToEntity;

	if (pEntity->IsPlayer())
	{
		// Use whichever is closer, absorigin or worldspacecenter.
		flDistanceToEntity = Min((m_vecSrc - pEntity->WorldSpaceCenter()).Length(), (m_vecSrc - pEntity->GetAbsOrigin()).Length());
	}
	else
	{
		flDistanceToEntity = (m_vecSrc - tr.endpos).Length();
	}

	
	// Direct hits = full healing amount
	if ( pEntity == m_hEntityDirectlyHit )
	{
		// Full damage, we hit this entity directly.
		bWasDirect = true;
		flDistanceToEntity = 0;
	}

	// Direct hits have their own specified healing amount
	if (bWasDirect)
	{
		flAdjustedHealing = m_flHealingAmountDirect;
		DevMsg("Direct hit on an ally.\n");
	}
	else if (m_bUseFalloffCalcs)
	{
		// If specified, apply falloff logic.
		flAdjustedHealing = RemapValClamped(flDistanceToEntity, 32, m_flRadius, m_flHealingAmountRadial, m_flHealingAmountRadial * falloff);
		DevMsg("Hit on ally via AoE heal.\n");
	}

	DevMsg( "Heal: (%f) \n", flAdjustedHealing );

	if (flAdjustedHealing <= 0)
		return false;

	// The explosion can 'see' this entity, so heal them!
	if (tr.startsolid)
	{
		// If we're stuck inside them, fixup the position and distance.
		tr.endpos = m_vecSrc;
		tr.fraction = 0.0f;
	}

	Vector dir = vecSpot - m_vecSrc;
	VectorNormalize(dir);

	/*
	if (tr.fraction != 1.0f && pEntity == tr.m_pEnt)
	{
		//ClearMultiDamage();
		//pEntity->DispatchTraceAttack(adjustedInfo, dir, &tr);
		//ApplyMultiDamage();
	}
	else
	{
		pEntity->TakeDamage()
	}
	*/

	// TODO check any heal bonuses/penalties here. Might be worth shifting this handling to the TakeHealth function instead.
	/*if (m_flOverhealMultiplier != 1.0f)
	{
		// Clip any overheal and multiply it by the penalty specified
		float flOverhealAmoumt = max((pEntity->GetHealth() + flAdjustedHealing) - pEntity->GetMaxHealth(), 0);
		if (flOverhealAmoumt > 0)
		{
			flAdjustedHealing = (flAdjustedHealing - flOverhealAmoumt) + (flOverhealAmoumt * m_flOverhealMultiplier);
		}
	}*/

	// Apply AoE or direct healing and charge Uber.
 	//pPlayerResponsible->GetMedigun()->OnHealedPlayer(pPlayer, pPlayer->TakeHealth(flAdjustedHealing, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP, pPlayerResponsible, true));
	pPlayer->m_Shared.AddBurstHealer( pPlayerResponsible, flAdjustedHealing );

#ifdef TF2C_BETA
	if ( bWasDirect )
	{
		CTFHealLauncher *pGL = dynamic_cast<CTFHealLauncher *>(pPlayerResponsible->GetMedigun());
		if ( pGL )
			pGL->OnDirectHit( pPlayer );
	}

	// Heal Launcher (Nurnberg Nader) impact particle effect
	const char* pszEffect = ConstructTeamParticle( "repair_claw_heal_%s3", pPlayerResponsible->GetTeamNumber() );
	DispatchParticleEffect( pszEffect, pPlayer->WorldSpaceCenter(), vec3_angle );
#endif

	// If desired, apply the residual healing effect
	if (pPlayerResponsible && pPlayer && m_bApplyResidualHeal)
		pPlayer->m_Shared.HealTimed( pPlayerResponsible, m_flResidualHealRate, m_flResidualHealDuration, true, true );

	// Now hit all triggers along the way that respond to damage... 
	//pEntity->TraceAttackToTriggers(adjustedInfo, m_vecSrc, tr.endpos, dir);

	if (pPlayer && m_condConditionsToApply > TF_COND_INVALID && m_flConditionDuration > 0)
		pPlayer->m_Shared.AddCond(m_condConditionsToApply, m_flConditionDuration, m_hPlayerResponsible);

	return true;
}

void CTFGameRules::RadiusHeal(CTFRadiusHealingInfo &radiusInfo, int nMaxPlayers /*= -1*/, bool bDivideHealing /*= false*/)
{
	CBaseEntity *pPlayerResponsible = radiusInfo.m_hPlayerResponsible;
	//int iPlayersHealed = 0;
	
	CBaseEntity *pEntity = NULL;
	
	if ( nMaxPlayers > 0 ) {
		CUtlVector<CBaseEntity *> vPriorityPlayers;
		int nPlayersHealed = 0;

		// First ascertain how many allied/healable players there are and divide if necessary
		for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// Directly hit players get the full healing, always
			if ( pEntity == radiusInfo.m_hEntityIgnore || pEntity == radiusInfo.m_hEntityDirectlyHit )
				continue;

			if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore )
				continue;

			// Skip the individual administering healing as we'll handle them separately.
			if ( pEntity == pPlayerResponsible )
				continue;

			// Checking distance from source because Valve were apparently too lazy to fix the engine function.
			Vector vecHitPoint;
			pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
			if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) > Square( radiusInfo.m_flRadius ) )
				continue;

			CTFPlayer *pPlayer = ToTFPlayer( pEntity );
			CTFPlayer *pPlayerResponsible = ToTFPlayer( radiusInfo.m_hPlayerResponsible );

			if ( !(pPlayer && pPlayerResponsible) )
				continue;

			if ( pPlayer->IsEnemy( pPlayerResponsible ) && !pPlayer->m_Shared.DisguiseFoolsTeam( pPlayerResponsible->GetTeamNumber() ) )
				continue;

			if ( pPlayer->IsDormant() || !pPlayer->IsAlive() )
				continue;

			if ( nPlayersHealed >= nMaxPlayers )
			{
				float flDistToEpicentreSq = (radiusInfo.m_vecSrc - pPlayer->GetAbsOrigin()).LengthSqr();
				FOR_EACH_VEC( vPriorityPlayers, i )
				{
					//vPriorityPlayers.AddToTail( pPlayer );
					if ( flDistToEpicentreSq < (radiusInfo.m_vecSrc - vPriorityPlayers[i]->GetAbsOrigin()).LengthSqr() )
					{
						vPriorityPlayers[i] = pPlayer;
						break;
					}
				}
			}
			else
			{
				vPriorityPlayers.AddToTail( pPlayer );
				nPlayersHealed++;
			}
		}

		// Full healing for our direct hit
		if ( radiusInfo.m_hEntityDirectlyHit )
			radiusInfo.ApplyToEntity( radiusInfo.m_hEntityDirectlyHit );

		// Should we divide the radial healing amount among the players we hit with the blast?
		if ( bDivideHealing && nPlayersHealed > 0 )
			radiusInfo.m_flHealingAmountRadial /= nPlayersHealed;

		FOR_EACH_VEC( vPriorityPlayers, i )
		{
			radiusInfo.ApplyToEntity( vPriorityPlayers[i] );
		}
	}
	else
	{

		for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			if ( pEntity == radiusInfo.m_hEntityIgnore ) {
				continue; DevMsg( "Entity caught in blast was the entity to be ignored.\n" );
			}

			// UNDONE: This should check a damage mask, not an ignore.
			if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore ) {
				continue; DevMsg( "Class was the class to ignore.\n" );
			}

			// Skip the individual administering healing as we'll handle them separately.
			if ( pEntity == pPlayerResponsible ) {
				continue; DevMsg( "Deferring self-healing processing until later in the function.\n" );
			}

			// Checking distance from source because Valve were apparently too lazy to fix the engine function.
			Vector vecHitPoint;
			pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );
			if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) > Square( radiusInfo.m_flRadius ) ) {
				continue; DevMsg( "Player was too far from the epicentre of the blast (distance %f vs radius %f)\n", vecHitPoint.DistTo( radiusInfo.m_vecSrc ), radiusInfo.m_flRadius );
			}

			if ( radiusInfo.ApplyToEntity( pEntity ) )
			{
				/*CTFPlayer *pPlayer = ToTFPlayer(pEntity);
				if (pPlayer && !pPlayer->IsEnemy(pPlayerResponsible))
				{
				iPlayersHealed++;
				}*/
			}
		}
	}
	//info.SetDamagedOtherPlayers(iPlayersHealed);

	if (pPlayerResponsible)
	{
		if (pPlayerResponsible == radiusInfo.m_hEntityIgnore)
			return;

		if (radiusInfo.m_flSelfHealRadius)
		{
			radiusInfo.m_flRadius = radiusInfo.m_flSelfHealRadius;
		}

		Vector vecHitPoint;
		pPlayerResponsible->CollisionProp()->CalcNearestPoint(radiusInfo.m_vecSrc, &vecHitPoint);
		
		if (vecHitPoint.DistToSqr(radiusInfo.m_vecSrc) <= Square(radiusInfo.m_flRadius) && radiusInfo.m_bSelfHeal)
		{
			radiusInfo.ApplyToEntity(pPlayerResponsible);
		}
	}
}



bool CTFGameRules::ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool bAllowDamage )
{
	// Allow the damage amount used for force calculations to be overwritten with a specific value if specified and flagged.
	info.SetDamageForForceCalc( info.GetDamageForForceCalcOverriden() ? info.GetDamageForForceCalc() :  info.GetDamage() );
	bool bDebug = tf_debug_damage.GetBool();

	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
	CBaseEntity* pInflictor = info.GetInflictor();

	// for achievement event
	bool bCritByTranq = false;

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );

	bool bShowDisguisedCrit = false;
	bool bAllSeeCrit = false;
	EAttackBonusEffects_t eBonusEffect = kBonusEffect_None;

	pVictim->SetSeeCrit( false, false, false );
	pVictim->SetAttackBonusEffect( kBonusEffect_None );

	// For awarding assist damage stat later. Replaces ETFCond eDamageBonusCond. Usually set it to cond provider.
	CTFPlayer* pTFCritAssister = nullptr;

	// Damage type was already crit (flares / headshot).
	int bitsDamage = info.GetDamageType();
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_BOIOING || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_STOMP || pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF ) )
	{
		// Never crit on a suicide. :)
		bitsDamage &= ~DMG_CRITICAL;

		goto CritChecksDone;
	}

	if ( bitsDamage & DMG_CRITICAL )
	{
		info.SetCritType( CTakeDamageInfo::CRIT_FULL );

		goto CritChecksDone;
	}

	// First figure out whether this is going to be a full forced crit for some specific reason. It's
	// important that we do this before figuring out whether we're going to be a minicrit or not.

	// Make sure we're not hurting ourselves first.
	if ( pVictim != pAttacker )
	{
		// The order of these crit checks affects the priority of deciding which player gets Damage Assist by setting pTFCritAssister.
		// Use same order logic as for kill assists: victim cond priority of g_eVictimAssistConditions, attacker cond priority of g_eAttackerAssistConditions, then the rest

		// You've entered Full-Crit isle.


		// Super Marked for Death
		if (pWeapon && pWeapon->IsMeleeWeapon() && pInflictor == pAttacker && !IsDOTDmg(info) &&
			pVictim->m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH))
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_FULL);

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_SUPERMARKEDFORDEATH));

			goto CritChecksDone;
		}

		// Melee crit on tranq targets. ShouldN'T support throwing knives.
		bool iNoTranqCrit = false;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim->m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED), iNoTranqCrit, no_melee_crit_from_tranq);
		if (pWeapon && pWeapon->IsMeleeWeapon() && !IsDOTDmg(info) &&
			pVictim->m_Shared.InCond(TF_COND_TRANQUILIZED) && !iNoTranqCrit)
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_TRANQ);

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED));

			bCritByTranq = true;

			goto CritChecksDone;
		}

		// Allow attributes to force critical hits on players with specific conditions
		// Crit against players that have these conditions
		int iCritDamageTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritDamageTypes, or_crit_vs_playercond );

		if ( iCritDamageTypes )
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for ( int i = 0; g_eAttributeConditions[i] != TF_COND_LAST; i++ )
			{
				if ( iCritDamageTypes & ( 1 << i ) )
				{
					if ( pVictim->m_Shared.InCond( g_eAttributeConditions[i] ) )
					{
						bitsDamage |= DMG_CRITICAL;
						info.AddDamageType( DMG_CRITICAL );
						info.SetCritType( CTakeDamageInfo::CRIT_FULL );

						if ( g_eAttributeConditions[i] == TF_COND_DISGUISED || 
							 g_eAttributeConditions[i] == TF_COND_DISGUISING )
						{
							// if our attribute specifically crits disguised enemies we need to show it on the client
							bShowDisguisedCrit = true;
						}

						pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(g_eAttributeConditions[i]));

						goto CritChecksDone;
					}
				}
			}
		}

		// Crit versus a single condition.
		// Does not accept DoT because 99.9% of times it's not what people want.
		int iCritSingleDamageTypesNoDot = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritSingleDamageTypesNoDot, single_crit_vs_playercond_no_dot);
		if (iCritSingleDamageTypesNoDot && !IsDOTDmg(info) && pVictim->m_Shared.InCond(ETFCond(iCritSingleDamageTypesNoDot - 1)))
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_FULL);

			if (ETFCond(iCritSingleDamageTypesNoDot - 1) == TF_COND_DISGUISED ||
				ETFCond(iCritSingleDamageTypesNoDot - 1) == TF_COND_DISGUISING)
			{
				// If our attribute specifically crits disguised enemies we need to show it on the client.
				bShowDisguisedCrit = true;
			}

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(ETFCond(iCritSingleDamageTypesNoDot - 1)));

			goto CritChecksDone;
		}
 
		// Crit against players that don't have these conditions
		int iCritDamageNotTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritDamageNotTypes, or_crit_vs_not_playercond );
		if ( iCritDamageNotTypes )
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for ( int i = 0; g_eAttributeConditions[i] != TF_COND_LAST; i++ )
			{
				if ( iCritDamageNotTypes & ( 1 << i ) )
				{
					if ( !pVictim->m_Shared.InCond( g_eAttributeConditions[i] ) )
					{
						bitsDamage |= DMG_CRITICAL;
						info.AddDamageType( DMG_CRITICAL );
						info.SetCritType( CTakeDamageInfo::CRIT_FULL );

						if ( g_eAttributeConditions[i] == TF_COND_DISGUISED || 
							 g_eAttributeConditions[i] == TF_COND_DISGUISING )
						{
							// If our attribute specifically crits disguised enemies we need to show it on the client.
							bShowDisguisedCrit = true;
						}

						goto CritChecksDone;
					}
				}
			}
		}


		// Super Marked for Death Silent
		if (pWeapon && pWeapon->IsMeleeWeapon() && pInflictor == pAttacker && !IsDOTDmg(info) &&
			pVictim->m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH_SILENT))
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_FULL);

			goto CritChecksDone;
		}

		// Crit versus wet targets
		int iCritVsWet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritVsWet, crit_vs_wet_players );
		if ( iCritVsWet && pVictim->IsWet() )
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );

			goto CritChecksDone;
		}

		// Some weapons force crits on bleeding targets.
		int iForceCritOnBleeding = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iForceCritOnBleeding, crit_vs_bleeding_players );
		if ( iForceCritOnBleeding == 1 && pVictim && pVictim->m_Shared.InCond( TF_COND_BLEEDING ) && info.GetDamageCustom() != TF_DMG_CUSTOM_BLEEDING )
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );

			goto CritChecksDone;
		}

		// Crit airborne targets
		int iCritAirborne = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritAirborne, crit_airborne_simple);
		float flCritAirborneMinHeight = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, flCritAirborneMinHeight, crit_airborne_simple_min_height);
		if (pVictim && ((iCritAirborne && pVictim->IsAirborne())
			|| (flCritAirborneMinHeight && pVictim->IsAirborne(flCritAirborneMinHeight))))
		{
			int iDirectHitOnly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iDirectHitOnly, mod_crit_airborne_direct_hit_only);
			if (!(iDirectHitOnly && (info.GetDamageType() & DMG_BLAST) && pWeapon && pWeapon->GetEnemy() != pVictim))
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType(DMG_CRITICAL);
				info.SetCritType(CTakeDamageInfo::CRIT_FULL);

				goto CritChecksDone;
			}
		}

		// Crit blast jumping targets
		int iCritBlastjump = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritBlastjump, crit_airborne);
		if (iCritBlastjump && pVictim && (pVictim->m_Shared.InCond(TF_COND_BLASTJUMPING) || pVictim->m_Shared.InCond(TF_COND_LAUNCHED)))
		{
			int iDirectHitOnly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iDirectHitOnly, mod_crit_airborne_direct_hit_only);
			if (!(iDirectHitOnly && (info.GetDamageType() & DMG_BLAST) && pWeapon && pWeapon->GetEnemy() != pVictim))
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType(DMG_CRITICAL);
				info.SetCritType(CTakeDamageInfo::CRIT_FULL);

				// Might as well, but very unlikely, only a spy, thus I am just leaving it here and not caring for order
				pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));

				goto CritChecksDone;
			}
		}

		// Crits players on which you are standing on top of
		int iCritVsPlayerUnderFeet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritVsPlayerUnderFeet, crit_vs_player_under_feet );
		if (iCritVsPlayerUnderFeet && pVictim == pTFAttacker->GetGroundEntity())
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );

			goto CritChecksDone;
		}

		// Crit against targets that have attributes to get crit by melee if you are melee weapon
		int iMeleeDamageTakenBecomesCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iMeleeDamageTakenBecomesCrit, melee_taken_becomes_crit_wearer);
		if (pVictim)
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim->GetActiveTFWeapon(), iMeleeDamageTakenBecomesCrit, melee_taken_becomes_crit_active);
		if (pVictim && pWeapon && pWeapon->IsMeleeWeapon() && pInflictor == pAttacker && !IsDOTDmg(info) && iMeleeDamageTakenBecomesCrit)
		{
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_FULL);

			goto CritChecksDone;
		}

		// Allow attributes to force critical hits on specific classes
		int iCritVsClass = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritVsClass, crit_vs_class);
		if (iCritVsClass)
		{
			for (int iClass = 0; iClass < TF_CLASS_COUNT_ALL - 1; ++iClass)
			{
				if (iCritVsClass & (1 << iClass))
				{
					if (pVictim->IsPlayerClass(iClass + 1))
					{
						bitsDamage |= DMG_CRITICAL;
						info.AddDamageType(DMG_CRITICAL);
						info.SetCritType(CTakeDamageInfo::CRIT_FULL);

						goto CritChecksDone;
					}
				}
			}
		}

		// Get crit by projectiles while blast jumping
		int iCritProjBlastJump = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iCritProjBlastJump, take_crits_projectile_blast_jump);
		if (iCritProjBlastJump)
		{
			if (!pAttacker->IsBSPModel() && (pVictim->m_Shared.InCond(TF_COND_BLASTJUMPING) || pVictim->m_Shared.InCond(TF_COND_LAUNCHED)) &&
				pInflictor && pInflictor->IsProjectile() &&
				!(info.GetDamageType() & DMG_BLAST && pInflictor->GetEnemy() != pVictim))
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType(DMG_CRITICAL);
				info.SetCritType(CTakeDamageInfo::CRIT_FULL);
#ifdef TF2C_BETA
				if (pVictim && pVictim->Weapon_OwnsThisID(TF_WEAPON_ANCHOR))
				{
					info.SetDamageCustom(TF_DMG_ANCHOR_AIRSHOTCRIT);

					bool bIsNail = false;
					CTFBaseNail* pNail = dynamic_cast<CTFBaseNail*>(pInflictor);
					if (pNail && pNail->GetBaseProjectileType() == TF_PROJECTILE_BASE_NAIL)
					{
						bIsNail = true;
					}

					if (!bIsNail)
					{
						EmitSound_t params;
						params.m_pSoundName = "Weapon_Anchor.CritDonked";

						// world, without attacker
						CPASAttenuationFilter filter(pVictim->GetAbsOrigin(), 0.5f);
						filter.RemoveRecipient(pTFAttacker);
						pVictim->EmitSound(filter, pVictim->entindex(), params);

						// attacker
						CSingleUserRecipientFilter attackerFilter(pTFAttacker);
						pTFAttacker->EmitSound(attackerFilter, pTFAttacker->entindex(), params);
					}
				}
#endif // TF2C_BETA
				// Might as well, but very unlikely, only a spy, thus I am just leaving it here and not caring for order
				pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));

				goto CritChecksDone;
			}
		}

		// Get crit by projectiles while in the air
		int iCritProjAirborne = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iCritProjAirborne, take_crits_projectile_airborne_simple);
		if (iCritProjAirborne && pVictim->IsAirborne(iCritProjAirborne))
		{
			if ( pInflictor && pInflictor->IsProjectile() &&
				!(info.GetDamageType() & DMG_BLAST && pInflictor->GetEnemy() != pVictim))
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType(DMG_CRITICAL);
				info.SetCritType(CTakeDamageInfo::CRIT_FULL);
#ifdef TF2C_BETA
				if ( pVictim && pVictim->Weapon_OwnsThisID(TF_WEAPON_ANCHOR) )
				{
					info.SetDamageCustom( TF_DMG_ANCHOR_AIRSHOTCRIT );
					
					bool bIsNail = false;
					CTFBaseNail *pNail = dynamic_cast<CTFBaseNail*>(pInflictor);
					if (pNail && pNail->GetBaseProjectileType() == TF_PROJECTILE_BASE_NAIL)
					{
						bIsNail = true;
					}

					if( !bIsNail )
					{
						EmitSound_t params;
						params.m_pSoundName = "Weapon_Anchor.CritDonked";

						// world, without attacker
						CPASAttenuationFilter filter(pVictim->GetAbsOrigin(), 0.5f);
						filter.RemoveRecipient(pTFAttacker);
						pVictim->EmitSound(filter, pVictim->entindex(), params);

						// attacker
						CSingleUserRecipientFilter attackerFilter(pTFAttacker);
						pTFAttacker->EmitSound(attackerFilter, pTFAttacker->entindex(), params);
					}
				}
#endif // TF2C_BETA
				// Might as well, but very unlikely, only a spy, thus I am just leaving it here and not caring for order
				pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));

				goto CritChecksDone;
			}
		}

		// Do crit when performing objectives
		int iCritOnObjectives = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritOnObjectives, crit_on_objectives);
		if (iCritOnObjectives)
		{
			bool bDoCrit = false;
			if (pTFAttacker)
			{
				// !!! TODO foxysen
				// in a good world it should check whether this condition was given by a VIP
				// besides do we want him to minicrit people who got boosted by VIP
				// but this is not a good world, babe
				// this is literally 2022
				if (pTFAttacker->m_Shared.InCond(TF_COND_RESISTANCE_BUFF) || pTFAttacker->HasTheFlag() ||
					pTFAttacker->GetControlPointStandingOn()) // not using IsCapturingPoint so even person defending their own point or standing on blocked on get minicrit
				{
					bDoCrit = true;
				}
			}
			if (!bDoCrit && pVictim)
			{
				if (pVictim->m_Shared.InCond(TF_COND_RESISTANCE_BUFF) || pVictim->HasTheFlag() || pVictim->IsVIP() ||
					pVictim->GetControlPointStandingOn()) // not using IsCapturingPoint so even person defending their own point or standing on blocked on get minicrit
				{
					bDoCrit = true;
				}
			}

			if (bDoCrit)
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType(DMG_CRITICAL);
				info.SetCritType(CTakeDamageInfo::CRIT_FULL);

				goto CritChecksDone;
			}
		}

		// Crit from behind
		int iCritFromBehind = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritFromBehind, crit_from_behind );
		if ( !IsDOTDmg( info ) && iCritFromBehind )
		{
			// Get the forward view vector of the target, ignore Z
			Vector vecVictimForward;
			AngleVectors( pVictim->EyeAngles(), &vecVictimForward );
			vecVictimForward.z = 0.0f;
			vecVictimForward.NormalizeInPlace();

			// Get a vector from my origin to my targets origin
			Vector vecToTarget;
			vecToTarget = pVictim->WorldSpaceCenter() - pTFAttacker->WorldSpaceCenter();
			vecToTarget.z = 0.0f;
			vecToTarget.NormalizeInPlace();

			// Get a forward vector of the attacker.
			Vector vecOwnerForward;
			AngleVectors( pTFAttacker->EyeAngles(), &vecOwnerForward );
			vecOwnerForward.z = 0.0f;
			vecOwnerForward.NormalizeInPlace();

			float flDotOwner = DotProduct( vecOwnerForward, vecToTarget );
			float flDotVictim = DotProduct( vecVictimForward, vecToTarget );
			float flDotViews = DotProduct( vecOwnerForward, vecVictimForward );

			// Backstab requires 3 conditions to be met:
			// 1) Spy must be behind the victim (180 deg cone).
			// 2) Spy must be looking at the victim (120 deg cone).
			// 3) Spy must be looking in roughly the same direction as the victim (~215 deg cone).

			if ( flDotVictim > 0.0f && flDotOwner > 0.5f && flDotViews > -0.3f )
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType( DMG_CRITICAL );
				info.SetCritType( CTakeDamageInfo::CRIT_FULL );

				goto CritChecksDone;
			}
		}

		// You've entered Mini-Crit isle.
		
		// Super Marked for Death
		if (pVictim->m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH))
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_SUPERMARKEDFORDEATH));

			goto CritChecksDone;
		}

		// Marked for Death
		if (pVictim->m_Shared.InCond(TF_COND_MARKEDFORDEATH))
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_MARKEDFORDEATH));

			goto CritChecksDone;
		}

		// At the top so little ol' Civilian will always get the support points.
		// Foxysen - We decided to follow strict order, so victim conds go first. He gets to be the first attacked cond guy tho!
		if (pTFAttacker)
		{
			// Civilian's Cash Incentive
			if (pTFAttacker->m_Shared.InCond(TF_COND_DAMAGE_BOOST))
			{
				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				pTFCritAssister = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_DAMAGE_BOOST));

				goto CritChecksDone;
			}
		}

		// Allow attributes to force critical hits on players with specific conditions
		// Minicrit against players that have these conditions
		int iMiniCritDamageTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritDamageTypes, or_minicrit_vs_playercond);

		if (iMiniCritDamageTypes)
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for (int i = 0; g_eAttributeConditions[i] != TF_COND_LAST; i++)
			{
				if (iMiniCritDamageTypes & (1 << i))
				{
					if (pVictim->m_Shared.InCond(g_eAttributeConditions[i]))
					{
						bAllSeeCrit = true;
						info.SetCritType(CTakeDamageInfo::CRIT_MINI);
						eBonusEffect = kBonusEffect_MiniCrit;

						if (g_eAttributeConditions[i] == TF_COND_DISGUISED ||
							g_eAttributeConditions[i] == TF_COND_DISGUISING)
						{
							// if our attribute specifically crits disguised enemies we need to show it on the client
							bShowDisguisedCrit = true;
						}

						pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(g_eAttributeConditions[i]));

						goto CritChecksDone;
					}
				}
			}
		}

		// Minicrit versus a single condition.
		// Does not accept DoT because 99.9% of times it's not what people want.
		int iMiniCritSingleDamageTypesNoDot = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritSingleDamageTypesNoDot, single_minicrit_vs_playercond_no_dot);
		if (iMiniCritSingleDamageTypesNoDot && !IsDOTDmg(info) && pVictim->m_Shared.InCond(ETFCond(iMiniCritSingleDamageTypesNoDot - 1)))
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			if (ETFCond(iMiniCritSingleDamageTypesNoDot - 1) == TF_COND_DISGUISED ||
				ETFCond(iMiniCritSingleDamageTypesNoDot - 1) == TF_COND_DISGUISING)
			{
				// If our attribute specifically crits disguised enemies we need to show it on the client.
				bShowDisguisedCrit = true;
			}

			pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(ETFCond(iMiniCritSingleDamageTypesNoDot - 1)));

			goto CritChecksDone;
		}

		// Minicrit against players that don't have these conditions
		int iMiniCritDamageNotTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritDamageNotTypes, or_minicrit_vs_not_playercond);
		if (iMiniCritDamageNotTypes)
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for (int i = 0; g_eAttributeConditions[i] != TF_COND_LAST; i++)
			{
				if (iMiniCritDamageNotTypes & (1 << i))
				{
					if (!pVictim->m_Shared.InCond(g_eAttributeConditions[i]))
					{
						bAllSeeCrit = true;
						info.SetCritType(CTakeDamageInfo::CRIT_MINI);
						eBonusEffect = kBonusEffect_MiniCrit;

						if (g_eAttributeConditions[i] == TF_COND_DISGUISED ||
							g_eAttributeConditions[i] == TF_COND_DISGUISING)
						{
							// If our attribute specifically crits disguised enemies we need to show it on the client.
							bShowDisguisedCrit = true;
						}

						goto CritChecksDone;
					}
				}
			}
		}
		
		// (Super) Marked for Death Silent
		if (pVictim->m_Shared.InCond(TF_COND_MARKEDFORDEATH_SILENT)
			|| pVictim->m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH_SILENT))
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		// This is only done for disguise based conditions since the disguise gets removed before this check
		if (pTFAttacker)
		{
			if (pTFAttacker->IsNextAttackMinicrit())
			{
				// We did the minicrit, so remove our flag
				pTFAttacker->SetNextAttackMinicrit(false);

				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				goto CritChecksDone;
			}
		}


		// Weapon own attack boost
		if (pWeapon && pWeapon->IsWeaponDamageBoosted())
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		// Minicrits airborne targets
		int iMiniCritAirborne = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritAirborne, mini_crit_airborne_simple);
		float flMiniCritAirborneMinHeight = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, flMiniCritAirborneMinHeight, mini_crit_airborne_simple_min_height);
		if (pVictim && ((iMiniCritAirborne && pVictim->IsAirborne())
			|| (flMiniCritAirborneMinHeight && pVictim->IsAirborne(flMiniCritAirborneMinHeight))))
		{
			int iDirectHitOnly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iDirectHitOnly, mod_minicrit_airborne_direct_hit_only);
			if (!(iDirectHitOnly && (info.GetDamageType() & DMG_BLAST) && pWeapon && pWeapon->GetEnemy() != pVictim))
			{
				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				goto CritChecksDone;
			}
		}

		// Minicrit blast jumping targets		
		int iMiniCritBlastjump = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritBlastjump, mini_crit_airborne);
		if (iMiniCritBlastjump && pVictim && (pVictim->m_Shared.InCond(TF_COND_BLASTJUMPING) || pVictim->m_Shared.InCond(TF_COND_LAUNCHED)))
		{
			int iDirectHitOnly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iDirectHitOnly, mod_minicrit_airborne_direct_hit_only);
			if (!(iDirectHitOnly && (info.GetDamageType() & DMG_BLAST) && pWeapon && pWeapon->GetEnemy() != pVictim))
			{
				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				// Might as well, but very unlikely, only a spy, thus I am just leaving it here and not caring for order
				pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));

				goto CritChecksDone;
			}
		}


		// Minicrit wet targets
		int iMiniCritVsWet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritVsWet, minicrit_vs_wet_players);
		if (iMiniCritVsWet && pVictim->IsWet())
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		int iTakeMiniWhileDisguised = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iTakeMiniWhileDisguised, minicrit_take_while_disguised);
		if( iTakeMiniWhileDisguised && pVictim->m_Shared.IsDisguised() )
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		// Minicrit players that you are standing on top of
		int iMiniCritVsPlayerUnderFeet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritVsPlayerUnderFeet, minicrit_vs_player_under_feet);
		if (iMiniCritVsPlayerUnderFeet && pVictim == pTFAttacker->GetGroundEntity())
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		// Minicrit with melee non-dot damage against players with attributes to take minicrit damage
		int iMeleeDamageTakenBecomesMiniCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iMeleeDamageTakenBecomesMiniCrit, melee_taken_becomes_minicrit_wearer);
		if (pVictim)
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim->GetActiveTFWeapon(), iMeleeDamageTakenBecomesMiniCrit, melee_taken_becomes_minicrit_active);
		if (pVictim && pWeapon && pWeapon->IsMeleeWeapon() && pInflictor == pAttacker && !IsDOTDmg(info) && iMeleeDamageTakenBecomesMiniCrit)
		{
			bAllSeeCrit = true;
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;

			goto CritChecksDone;
		}

		// Allow attributes to force minicrits on specific classes
		int iMiniCritVsClass = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritVsClass, minicrit_vs_class);
		if (iMiniCritVsClass)
		{
			for (int iClass = 0; iClass < TF_CLASS_COUNT_ALL - 1; ++iClass)
			{
				if (iMiniCritVsClass & (1 << iClass))
				{
					if (pVictim->IsPlayerClass(iClass + 1))
					{
						bAllSeeCrit = true;
						info.SetCritType(CTakeDamageInfo::CRIT_MINI);
						eBonusEffect = kBonusEffect_MiniCrit;

						goto CritChecksDone;
					}
				}
			}
		}

		// Get mini-crit by projectiles while blast jumping
		int iMiniCritProjBlastJump = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pVictim, iMiniCritProjBlastJump, take_minicrits_projectile_blast_jump);
		if (iMiniCritProjBlastJump)
		{
			if (!pAttacker->IsBSPModel() && (pVictim->m_Shared.InCond(TF_COND_BLASTJUMPING) || pVictim->m_Shared.InCond(TF_COND_LAUNCHED)) &&
				pInflictor && !pInflictor->IsBaseObject() && !pInflictor->IsPlayer() &&
				!(info.GetDamageType() & DMG_BLAST && pInflictor->GetEnemy() != pVictim))
			{
				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				// Might as well, but very unlikely, only a spy, thus I am just leaving it here and not caring for order
				pTFCritAssister = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_LAUNCHED));

				goto CritChecksDone;
			}
		}

		// Do mini-crit when performing objectives
		int iMiniCritOnObjectives = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritOnObjectives, mini_crit_on_objectives);
		if (iMiniCritOnObjectives)
		{
			bool bDoMinicrit = false;
			if (pTFAttacker)
			{
				// !!! TODO foxysen
				// in a good world it should check whether this condition was given by a VIP
				// besides do we want him to minicrit people who got boosted by VIP
				// but this is not a good world, babe
				// this is literally 2022
				if (pTFAttacker->m_Shared.InCond(TF_COND_RESISTANCE_BUFF) || pTFAttacker->HasTheFlag() ||
					pTFAttacker->GetControlPointStandingOn()) // not using IsCapturingPoint so even person defending their own point or standing on blocked on get minicrit
				{
					bDoMinicrit = true;
				}
			}
			if (!bDoMinicrit && pVictim)
			{
				if (pVictim->m_Shared.InCond(TF_COND_RESISTANCE_BUFF) || pVictim->HasTheFlag() || pVictim->IsVIP() ||
					pVictim->GetControlPointStandingOn()) // not using IsCapturingPoint so even person defending their own point or standing on blocked on get minicrit
				{
					bDoMinicrit = true;
				}
			}

			if (bDoMinicrit)
			{
				bAllSeeCrit = true;
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;

				goto CritChecksDone;
			}
		}

		CBaseEntity *pInflictor = info.GetInflictor();
		CTFGrenadePipebombProjectile *pBaseGrenade = dynamic_cast<CTFGrenadePipebombProjectile *>( pInflictor );
		CTFBaseRocket *pBaseRocket = dynamic_cast<CTFBaseRocket *>( pInflictor );
		if ( ( pInflictor && !pInflictor->IsPlayer() ) && ( ( pBaseRocket && pBaseRocket->m_iDeflected ) || ( pBaseGrenade && pBaseGrenade->m_iDeflected ) ) )
		{
			// Reflected rockets, grenades (non-remote detonate), arrows always mini-crit
			int iDeflectNoMinicrit = 0;
			if( pTFAttacker )
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iDeflectNoMinicrit, mod_deflect_no_minicrit );

			if( !iDeflectNoMinicrit )
			{
				info.SetCritType(CTakeDamageInfo::CRIT_MINI);
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			// Uncomment when another check is added below this one, we're avoiding using a jmp instruction here.
			//goto CritChecksDone;
		}
	}

CritChecksDone:

	// And now for our minicrit -> crit and crit -> minicrit attributes carousel
	if (info.GetCritType() == CTakeDamageInfo::CRIT_MINI)
	{
		int iMiniCritIntoCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iMiniCritIntoCrit, minicrits_become_crits);
		if (iMiniCritIntoCrit)
		{
			bAllSeeCrit = false;	//maybe
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType(DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_FULL);
			eBonusEffect = kBonusEffect_None;
		}
	}
	else if (info.GetCritType() == CTakeDamageInfo::CRIT_FULL || info.GetCritType() == CTakeDamageInfo::CRIT_TRANQ)
	{
		int iCritIntoMiniCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iCritIntoMiniCrit, crits_become_minicrits);
		if (iCritIntoMiniCrit)
		{
			bAllSeeCrit = true;		//maybe
			bitsDamage &= ~DMG_CRITICAL;
			info.SetDamageType(info.GetDamageType() & ~DMG_CRITICAL);
			info.SetCritType(CTakeDamageInfo::CRIT_NONE);	// CRIT_MINI is not allowed to override CRIT_FULL so we are doing this absolutely amazing hack
			info.SetCritType(CTakeDamageInfo::CRIT_MINI);
			eBonusEffect = kBonusEffect_MiniCrit;
		}
	}

	// Some forms of damage override long range damage falloff.
	bool bIgnoreLongRangeDmgEffects = false;

	if ( pVictim )
	{
		pVictim->SetSeeCrit( bAllSeeCrit, info.GetCritType() == CTakeDamageInfo::CRIT_MINI, bShowDisguisedCrit );
		pVictim->SetAttackBonusEffect( eBonusEffect );
	}

	// If we're invulnerable, force ourselves to only take damage events only, so we still get pushed.
	int iSelfDamagePenetratesUber = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iSelfDamagePenetratesUber, mod_self_damage_penetrates_uber);
	if( pVictim && ( pVictim->m_Shared.IsInvulnerable() || pVictim->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB) ) && !( iSelfDamagePenetratesUber && pVictim == pAttacker ) )	// !!! foxysen speedwatch
	{
		if ( !bAllowDamage )
		{
			int iOldTakeDamage = pVictim->m_takedamage;
			pVictim->m_takedamage = DAMAGE_EVENTS_ONLY;
			// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice.
			pVictim->CBaseCombatCharacter::OnTakeDamage( info );
			pVictim->m_takedamage = iOldTakeDamage;

			// Burn sounds are handled in ConditionThink().
			if ( !( bitsDamage & DMG_BURN ) )
			{
				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
			}

			return false;
		}
	}

	// A note about why crits now go through the randomness/variance code:
	// Normally critical damage is not affected by variance. However, we always want to measure what that variance 
	// would have been so that we can lump it into the DamageBonus value inside the info. 
	// This means crits actually boost more than 3X when you factor the reduction we avoided.

	// Example: a rocket that normally would do 50 damage due to range now does the original 100,
	// which is then multiplied by 3, resulting in a 6x increase.
	bool bCrit = ( bitsDamage & DMG_CRITICAL ) ? true : false;

	// If we're not damaging ourselves, apply randomness
	if ( pAttacker != pVictim && !( bitsDamage & ( DMG_DROWN | DMG_FALL ) ) ) 
	{
		float flDamage = info.GetDamage();
		float flDmgVariance = 0.f;

		// Minicrits still get short range damage bonus
		bool bForceCritFalloff = ( bitsDamage & DMG_USEDISTANCEMOD ) && 
								 ( ( bCrit && tf_weapon_criticals_distance_falloff.GetBool() ) || 
								 ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI && tf_weapon_minicrits_distance_falloff.GetBool() ) );
		
		if ( bCrit )
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, bForceCritFalloff, crit_dmg_falloff );
		
		if ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI )
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, bForceCritFalloff, minicrit_dmg_falloff );

		bool bDoShortRangeDistanceIncrease = !bCrit || info.GetCritType() == CTakeDamageInfo::CRIT_MINI;
		bool bDoLongRangeDistanceDecrease = !bIgnoreLongRangeDmgEffects && ( bForceCritFalloff || ( !bCrit && info.GetCritType() != CTakeDamageInfo::CRIT_MINI ) );

		// If we're doing any distance modification, we need to do that first.
		float flRandomDamage = info.GetDamage() * tf_damage_range.GetFloat();

		float flRandomDamageSpread = 0.1f;
		float flMin = 0.5f - flRandomDamageSpread;
		float flMax = 0.5f + flRandomDamageSpread;

		float flCustomRampup = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCustomRampup, mod_custom_rampup );
		float flCustomFalloff = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCustomFalloff, mod_custom_falloff );
		float flMultFalloffDistance = 1.0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flMultFalloffDistance, mult_falloff_distance );

		if ( bCrit && bForceCritFalloff )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCustomFalloff, mod_custom_falloff_crit );
		}

		if ( bitsDamage & DMG_USEDISTANCEMOD || flCustomRampup || flCustomFalloff || ( flMultFalloffDistance != 1.0 ) )
		{
			Vector vAttackerPos = pAttacker->WorldSpaceCenter();
			float flOptimalDistance = 512.0;

			// Use Sentry position for distance mod.
			CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( info.GetWeapon() );
			if ( pSentry )
			{
				vAttackerPos = pSentry->WorldSpaceCenter();
				// Sentries have a much further optimal distance.
				flOptimalDistance = SENTRY_MAX_RANGE;
			}
			// The base sniper rifle doesn't have DMG_USEDISTANCEMOD, so this isn't used.
			else if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE )
			{
				flOptimalDistance *= 2.5f;
			}

			if ( flMultFalloffDistance != 1.0 )
			{
				flOptimalDistance *= flMultFalloffDistance;
				DevMsg( "custom falloff dist: %2.2f, optimal dist: %2.2f\n", flMultFalloffDistance, flOptimalDistance );
			}

			float flDistance = Max<float>( 1.0f, ( pVictimBaseEntity->WorldSpaceCenter() - vAttackerPos ).Length() );
				
			float flCenter = RemapValClamped( flDistance / flOptimalDistance, 0.0f, 2.0f, 1.0f, 0.0f );
			if ( ( flCenter > 0.5 && bDoShortRangeDistanceIncrease ) || flCenter <= 0.5 )
			{
				if (bitsDamage & DMG_NOCLOSEDISTANCEMOD && !flCustomRampup)
				{
					if ( flCenter > 0.5f )
					{
						// Reduce the damage bonus at close range.
						flCenter = RemapVal( flCenter, 0.5f, 1.0f, 0.5f, 0.65f );
					}
				}

				flMin = Max<float>( 0.0f, flCenter - flRandomDamageSpread );
				flMax = Min<float>( 1.0f, flCenter + flRandomDamageSpread );

				if ( bDebug )
				{
					Warning( "    RANDOM: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
			else
			{
				if ( bDebug )
				{
					Warning( "    NO DISTANCE MOD: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
		}

		//Msg("Range: %.2f - %.2f\n", flMin, flMax );
		float flRandomRangeVal;
		if ( tf_damage_disablespread.GetBool() )
		{
			flRandomRangeVal = flMin + flRandomDamageSpread;
		}
		else
		{
			flRandomRangeVal = RandomFloat( flMin, flMax );
		}

		// Weapon Based Damage Mod.
		if ( pWeapon && pAttacker && pAttacker->IsPlayer() )
		{
			float flCustomRampup = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flCustomRampup, mod_custom_rampup);
			if (flCustomRampup && flRandomRangeVal > 0.5f)
			{
				float flCustomRampupValue = (flCustomRampup - 1) * 2;
				flRandomDamage *= flCustomRampupValue;
			}
			else
			{
				switch (pWeapon->GetWeaponID())
				{
				// Rocket launcher only has half the bonus of the other weapons at short range.
				case TF_WEAPON_ROCKETLAUNCHER:
					if (flRandomRangeVal > 0.5f)
					{
						flRandomDamage *= 0.5f;
					}
					break;
				case TF_WEAPON_PIPEBOMBLAUNCHER:
				case TF_WEAPON_GRENADELAUNCHER:
					if (!(bitsDamage & DMG_NOCLOSEDISTANCEMOD))
					{
						flRandomDamage *= 0.2f;
					}
					break;
				// Scattergun gets 50% bonus at short range.
				case TF_WEAPON_SCATTERGUN:
					if (flRandomRangeVal > 0.5f)
					{
						flRandomDamage *= 1.5f;
					}
					break;
				}
			}

			if ( flRandomRangeVal < 0.5f )
			{
				int iDisableFalloff = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iDisableFalloff, mod_no_dmg_falloff );
				if (iDisableFalloff > 0 || !(bitsDamage & DMG_USEDISTANCEMOD))
				{
					flRandomRangeVal = 0.5f;
				}
				else if (flCustomFalloff)
				{
					float flCustomFalloffValue = -((flCustomFalloff - 1) * 2);
					flRandomDamage *= flCustomFalloffValue;
				}
			}
		}

		// Random damage variance.
		flDmgVariance = SimpleSplineRemapValClamped( flRandomRangeVal, 0, 1, -flRandomDamage, flRandomDamage );
		if ( ( bDoShortRangeDistanceIncrease && flDmgVariance > 0.0f ) || bDoLongRangeDistanceDecrease )
		{
			flDamage = info.GetDamage() + flDmgVariance;
		}

		if ( bDebug )
		{
			Warning( "            Out: %.2f -> Final %.2f\n", flDmgVariance, flDamage );
		}

		/*for ( float flVal = flMin; flVal <= flMax; flVal += 0.05 )
		{
			float flOut = SimpleSplineRemapValClamped( flVal, 0, 1, -flRandomDamage, flRandomDamage );
			Msg("Val: %.2f, Out: %.2f, Dmg: %.2f\n", flVal, flOut, info.GetDamage() + flOut );
		}*/

		// Burn sounds are handled in ConditionThink().
		if ( !( bitsDamage & DMG_BURN ) && pVictim )
		{
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}

		if (info.GetDamageCustom() != TF_DMG_CUSTOM_HEADSHOT)
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flDamage, bodyshot_damage_modify);
		}

		// Save any bonus damage as a separate value.
		float flCritDamage = 0.0f;

		// Yes, it's weird that we sometimes fabs flDmgVariance. Here's why: In the case of a crit rocket, we
		// know that number will generally be negative due to dist or randomness. In this case, we want to track
		// that effect - even if we don't apply it. In the case of our crit rocket that normally would lose 50 
		// damage, we fabs'd so that we can account for it as a bonus - since it's present in a crit.
		float flBonusDamage = bForceCritFalloff ? 0.0f : fabs( flDmgVariance );
		CTFPlayer *pProvider = NULL;

		if ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI )
		{
			// We should never have both of these flags set or Weird Things will happen with the damage numbers
			// that aren't clear to the players. Or us, really.
			Assert( !( bitsDamage & DMG_CRITICAL ) );

			if ( bDebug )
			{
				Warning( "    MINICRIT: Dmg %.2f -> ", flDamage );
			}

			COMPILE_TIME_ASSERT( TF_DAMAGE_MINICRIT_MULTIPLIER > 1.0f );
			flCritDamage = ( TF_DAMAGE_MINICRIT_MULTIPLIER - 1.0f ) * flDamage;

			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );

			// Damage Assist time!
			// Crit assister must be on our team and not be attacker!
			if (pTFCritAssister && pTFAttacker && pVictim &&
				!pTFAttacker->IsEnemy(pTFCritAssister) && pTFAttacker != pTFCritAssister)
			{
				CTF_GameStats.Event_PlayerDamageAssist(pTFCritAssister, flCritDamage + flBonusDamage);
			}

			if ( bDebug )
			{
				Warning( "reduced to %.2f before crit mult\n", flDamage );
			}
		}

		if ( bCrit )
		{
			float flCritMult = TF_DAMAGE_CRIT_MULTIPLIER;

			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flCritMult, mult_dmg_crit_multiplier);

			if ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCritMult, headshot_damage_modify );
			}

			if ( info.GetCritType() != CTakeDamageInfo::CRIT_MINI )
			{
				COMPILE_TIME_ASSERT( TF_DAMAGE_CRIT_MULTIPLIER > 1.0f );
				flCritDamage = ( flCritMult - 1.0f ) * flDamage;
			}

			if ( bDebug )
			{
				Warning( "    CRITICAL! Damage: %.2f\n", flDamage );
			}

			// Burn sounds are handled in ConditionThink().
			if ( !( bitsDamage & DMG_BURN ) && pVictim )
			{
				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT, "damagecritical:1" );
			}

			// Damage Assist time!
			// Since Kritz crit are decided when projectiles are shot instead of on impact, projectile remembering normal crit, this special older block goes first
			if ( pTFAttacker && pTFAttacker->m_Shared.IsCritBoosted() )
			{
				pProvider = ToTFPlayer( pTFAttacker->m_Shared.GetConditionProvider( TF_COND_CRITBOOSTED ) );
				if ( pProvider && pTFAttacker && pProvider != pTFAttacker )
				{
					CTF_GameStats.Event_PlayerDamageAssist( pProvider, flCritDamage + flBonusDamage );	
				}
			}
			else
			{
				// Crit assister must be on our team and not be attacker!
				if (pTFCritAssister && pTFAttacker && pVictim &&
					!pTFAttacker->IsEnemy(pTFCritAssister) && pTFAttacker != pTFCritAssister)
				{
					CTF_GameStats.Event_PlayerDamageAssist(pTFCritAssister, flCritDamage + flBonusDamage);

					if (bCritByTranq)
					{
						IGameEvent* event = gameeventmanager->CreateEvent("tranq_support");
						if (event)
						{
							event->SetInt("userid", pTFCritAssister->GetUserID());
							event->SetInt("amount", int(flCritDamage + flBonusDamage));
							gameeventmanager->FireEvent(event);
						}
					}
				}
			}
		}

		// Store the extra damage and update actual damage.
		if ( bCrit || info.GetCritType() == CTakeDamageInfo::CRIT_MINI )
		{
			// Order-of-operations sensitive, but fine as long as TF_COND_CRITBOOSTED is last.
			info.SetDamageBonus( flCritDamage + flBonusDamage, pProvider );
		}

		info.SetDamage( flDamage + flCritDamage );
	}

	bool bShieldBlocked = false;
	if( pVictim && pVictim->GetActiveTFWeapon() && pVictim->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_RIOT_SHIELD )
	{
		CTFRiot *pRiotShield = static_cast<CTFRiot*> (pVictim->GetActiveTFWeapon());
		bShieldBlocked = pRiotShield->AbsorbDamage(info) > 0;
	}

	if( bShieldBlocked && !bAllowDamage )
	{
		int iOldTakeDamage = pVictim->m_takedamage;
		pVictim->m_takedamage = DAMAGE_EVENTS_ONLY;
		// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice.
		pVictim->CBaseCombatCharacter::OnTakeDamage( info );
		pVictim->m_takedamage = iOldTakeDamage;

		// Burn sounds are handled in ConditionThink().
		if ( !( bitsDamage & DMG_BURN ) )
		{
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}

		return false;
	}

	if ( pVictim->m_Shared.InCond( TF_COND_DEFLECT_BULLETS ) && bitsDamage & (DMG_BULLET | DMG_BUCKSHOT) && pAttacker != pVictim)
	{
		// Fire bullets
		//Vector vecSrc = pVictim->EyePosition();
		Vector vecSrc = pVictim->Weapon_ShootPosition();
		QAngle vecShootAngle = pVictim->EyeAngles() + pVictim->GetPunchAngle();
		Vector vecShootForward, vecShootRight, vecShootUp;
		AngleVectors( vecShootAngle, &vecShootForward, &vecShootRight, &vecShootUp );
		Vector vecEnd = vecSrc + vecShootForward * MAX_TRACE_LENGTH;

		// Only deflect shots that come from our front 180 degrees
		Vector2D vecLookHorizontal = vecShootForward.AsVector2D();
		Vector2D vecShotHorizontal = (pVictim->GetAbsOrigin() - pAttacker->GetAbsOrigin()).AsVector2D();
		float dotP = vecLookHorizontal.Dot( vecShotHorizontal );
		if ( dotP < 0 )
		{

			trace_t tr;
			UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pVictim, COLLISION_GROUP_NONE, &tr );

			float flDistToTarget = (tr.endpos - tr.startpos).Length();

			FireBulletsInfo_t bulletInfo;

			bulletInfo.m_vecSrc = pVictim->EyePosition();
			bulletInfo.m_vecDirShooting = vecShootForward;
			bulletInfo.m_iTracerFreq = 1;
			bulletInfo.m_iShots = 1;
			bulletInfo.m_pAttacker = pVictim;
			bulletInfo.m_vecSpread = vec3_origin;// pWeapon->GetBulletSpread();
			bulletInfo.m_flDistance = 100 + flDistToTarget;
			bulletInfo.m_iAmmoType = GetAmmoDef()->Index( "TF_AMMO_PRIMARY" );//max(info.GetAmmoType(), 0);
			bulletInfo.m_flDamage = info.GetDamage();

			ClearMultiDamage();

			//		CTraceFilterIgnoreTeammatesExceptEntity filter( nullptr, COLLISION_GROUP_NONE, pAttacker->GetTeamNumber(), pAttacker );
			//		bulletInfo.m_pCustomFilter = &filter;
			pVictim->FireBullets( bulletInfo );

			return false;
		}
	}

	// Apply on-hit attributes.
	if ( pWeapon && pVictim && ( pAttacker && pAttacker->IsPlayer() && pAttacker->GetTeam() != pVictim->GetTeam() ) )
	{
		pWeapon->ApplyOnHitAttributes( pAttacker, pVictim, info );
	}

	// Give assist points to the provider of any stun on the victim.
	if ( pVictim && pVictim->m_Shared.InCond( TF_COND_STUNNED ) )
	{
		CTFPlayer *pProvider = ToTFPlayer( pVictim->m_Shared.GetConditionProvider( TF_COND_STUNNED ) );
		if ( pProvider && pTFAttacker && pProvider != pTFAttacker && !pProvider->IsEnemy(pTFAttacker))
		{
			float flStunAmount = pVictim->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT );
			if ( flStunAmount < 1.0f && pVictim->m_Shared.IsControlStunned() )
			{
				flStunAmount = 1.0f;
			}

			int nAssistPoints = RemapValClamped( flStunAmount, 0.1f, 1.0f, 1, info.GetDamage() / 2 );
			if ( nAssistPoints )
			{
				CTF_GameStats.Event_PlayerDamageAssist( pProvider, nAssistPoints );	
			}
		}
	}

	// Also assist points to the provider of lite-stun-like Tranq effect on the victim.
	if (pVictim && pVictim->m_Shared.InCond(TF_COND_TRANQUILIZED))
	{
		CTFPlayer* pProvider = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED));
		if (pProvider && pTFAttacker && pProvider != pTFAttacker && !pProvider->IsEnemy(pTFAttacker))
		{
			CTF_GameStats.Event_PlayerDamageAssist(pProvider, info.GetDamage() * 0.15f);

			IGameEvent* event = gameeventmanager->CreateEvent("tranq_support");
			if (event)
			{
				event->SetInt("userid", pProvider->GetUserID());
				event->SetInt("amount", int(info.GetDamage() * 0.15f));
				gameeventmanager->FireEvent(event);
			}
		}
	}

	// Not sure if TNT slow should assist but here we go
	if (pVictim && pVictim->m_Shared.InCond(TF_COND_MIRV_SLOW))
	{
		CTFPlayer* pProvider = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_MIRV_SLOW));
		if (pProvider && pTFAttacker && pProvider != pTFAttacker && !pProvider->IsEnemy(pTFAttacker))
		{
			CTF_GameStats.Event_PlayerDamageAssist(pProvider, info.GetDamage() * 0.15f);
		}
	}

	// And also assist points for haste so Haste Civ gets fair chance against Minicrit Civ in Support Points Esport
	if (pTFAttacker && pTFAttacker->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
	{
		CTFPlayer* pProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF));
		if (pProvider && pProvider != pTFAttacker && !pProvider->IsEnemy(pTFAttacker))
		{
			CTF_GameStats.Event_PlayerDamageAssist(pProvider, info.GetDamage() * 0.35f); // Magic const bad only if it annoys coder :>
		}
	}

	return true;
}


static bool CheckForDamageTypeImmunity( int nDamageType, CTFPlayer* pVictim, float &flDamageBase, float &flCritBonusDamage )
{
	bool bImmune = false;
	if ( nDamageType & ( DMG_BURN | DMG_IGNITE ) )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_FIRE_IMMUNE );
	}
	else if ( nDamageType & ( DMG_BULLET | DMG_BUCKSHOT ) )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_BULLET_IMMUNE );
	}
	else if ( nDamageType & DMG_BLAST )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_BLAST_IMMUNE );
	}

	if ( bImmune )
	{
		flDamageBase = flCritBonusDamage = 0.0f;

		IGameEvent* event = gameeventmanager->CreateEvent( "damage_resisted" );
		if ( event )
		{
			event->SetInt( "entindex", pVictim->entindex() );
			gameeventmanager->FireEvent( event ); 
		}

		return true;
	}

	return false;
}


float CTFGameRules::ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, DamageModifyExtras_t &outParams )
{
	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

	float flRealDamage = info.GetDamage();

	int iSelfDamagePenetratesUber = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iSelfDamagePenetratesUber, mod_self_damage_penetrates_uber);
	if ( pVictimBaseEntity && (pVictimBaseEntity->m_takedamage != DAMAGE_EVENTS_ONLY || (iSelfDamagePenetratesUber && pVictim == pAttacker))
		&& !IsTauntKillDmg(info.GetDamageCustom()) &&
		!(info.GetDamageType() & DMG_FALL && info.GetDamageCustom() != TF_DMG_FALL_DAMAGE)) // Oh, this line should protect against impacting trigger_hurt events
	{
		int iDamageTypeBits = info.GetDamageType();

		// Handle attributes that want to change our damage type, but only if we're taking damage from a non-DOT. This
		// stops fire DOT damage from constantly reigniting us, this will also prevent ignites from happening on the
		// damage *from-a-bleed-DOT*, but not from the bleed application attack.
		if ( !IsDOTDmg( info ) )
		{
			int iAddBurningDamageType = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iAddBurningDamageType, set_dmgtype_ignite );
			if ( iAddBurningDamageType )
			{
				iDamageTypeBits |= DMG_IGNITE;
			}

			if ( pTFAttacker )
			{
				int iDeflectedIgnites = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFAttacker, iDeflectedIgnites, mod_deflected_projectiles_ignite);
				if (iDeflectedIgnites)
				{
					CBaseEntity* pInflictor = info.GetInflictor();
					CTFGrenadePipebombProjectile* pBaseGrenade = dynamic_cast<CTFGrenadePipebombProjectile*>(pInflictor);
					CTFBaseRocket* pBaseRocket = dynamic_cast<CTFBaseRocket*>(pInflictor);
					if ((pInflictor && !pInflictor->IsPlayer()) && ((pBaseRocket && pBaseRocket->m_iDeflected) || (pBaseGrenade && pBaseGrenade->m_iDeflected)))
					{
						iDamageTypeBits |= DMG_IGNITE;
					}
				}
			}
		}

		// Start burning if we took ignition damage.
		outParams.bIgniting = ( ( iDamageTypeBits & DMG_IGNITE ) && ( !pVictim || pVictim->GetWaterLevel() < WL_Waist ) );

		// - Begin Resists and Boosts

		float flDamageBonus = info.GetDamageBonus();
		float flDamageBase = flRealDamage - flDamageBonus;
		Assert( flDamageBase >= 0.0f );

		int iPierceResists = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iPierceResists, mod_pierce_resists_absorbs );

		// This raw damage wont get scaled, used for determining how much health to give resist medics.
		float flRawDamage = flDamageBase;
		
		// Check if we're immune.
		outParams.bPlayDamageReductionSound = CheckForDamageTypeImmunity(info.GetDamageType(), pVictim, flDamageBase, flDamageBonus);
		
		bool bPlayDamageReductionSoundOverride = false;

		if (pTFAttacker != pVictim)
		{
			if (pTFAttacker)
				outParams.vecPlayDamageReductionSoundFullPlayers.AddToTail(pTFAttacker);

			if (!iPierceResists)
			{
				// Reduce only the crit portion of the damage with crit resist.
				if (info.GetDamageType() & DMG_CRITICAL)
				{
					// Break the damage down and reassemble.
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBonus, mult_dmgtaken_from_crit);
				}

				if (pVictim)
				{
					// VIP resistance should come first so his resistance will be the first applied and he will get more points
					if (pVictim->m_Shared.InCond(TF_COND_CIV_DEFENSEBUFF))
					{
						float flOriginalDamage = flDamageBase;
						flDamageBase *= tf2c_vip_eagle_armor.GetFloat();
						CTFPlayer* pProvider = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_CIV_DEFENSEBUFF));
						if (pProvider && pProvider != pVictim)
						{
							if (!outParams.vecPlayDamageReductionSoundFullPlayers.HasElement(pProvider))
								outParams.vecPlayDamageReductionSoundFullPlayers.AddToTail(pProvider);
							if (flRealDamage <= pVictim->GetMaxHealth() * 2) // Try to safeguard against backstab, overkill or trigger_hurt damages giving ridiculous amount of points)
							{
								int iOriginalDamage = Ceil2Int(flOriginalDamage + 0.5f);
								int iDamageBase = Ceil2Int(flDamageBase + 0.5f);
								int iDamageBlocked = max(iOriginalDamage - iDamageBase, 0);

								IGameEvent* event = gameeventmanager->CreateEvent("damage_blocked");
								if (event)
								{
									event->SetInt("victim", pVictim ? pVictim->GetUserID() : -1);
									event->SetInt("provider", pProvider ? pProvider->GetUserID() : -1);
									event->SetInt("attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1);
									event->SetInt("amount", iDamageBlocked);
									gameeventmanager->FireEvent(event);
								}

								if (!pProvider->IsEnemy(pVictim))
								{
									CTF_GameStats.Event_PlayerBlockedDamage(pProvider, iDamageBlocked);
								}
							}
						}
					}
					// VIP resistance should come first so his resistance will be the first applied and he will get more points
					if (pVictim->m_Shared.InCond(TF_COND_RESISTANCE_BUFF))
					{
						float flOriginalDamage = flDamageBase;
						flDamageBase *= tf2c_vip_armor.GetFloat();
						CTFPlayer* pProvider = ToTFPlayer(pVictim->m_Shared.GetConditionProvider(TF_COND_RESISTANCE_BUFF));
						if (pProvider && pProvider != pVictim)
						{
							if (!outParams.vecPlayDamageReductionSoundFullPlayers.HasElement(pProvider))
								outParams.vecPlayDamageReductionSoundFullPlayers.AddToTail(pProvider);
							if (flRealDamage <= pVictim->GetMaxHealth() * 2) // Try to safeguard against backstab, overkill or trigger_hurt damages giving ridiculous amount of points)
							{
								int iOriginalDamage = Ceil2Int(flOriginalDamage + 0.5f);
								int iDamageBase = Ceil2Int(flDamageBase + 0.5f);
								int iDamageBlocked = max(iOriginalDamage - iDamageBase, 0);

								IGameEvent* event = gameeventmanager->CreateEvent("damage_blocked");
								if (event)
								{
									event->SetInt("victim", pVictim ? pVictim->GetUserID() : -1);
									event->SetInt("provider", pProvider ? pProvider->GetUserID() : -1);
									event->SetInt("attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1);
									event->SetInt("amount", iDamageBlocked);
									gameeventmanager->FireEvent(event);
								}

								if (!pProvider->IsEnemy(pVictim))
								{
									CTF_GameStats.Event_PlayerBlockedDamage(pProvider, iDamageBlocked);
								}
							}
						}
					}
					if ( pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF ) )
					{
						float flOriginalDamage = flDamageBase;
						CObjectSentrygun *pSentryGun = dynamic_cast<CObjectSentrygun *>(pAttacker);
						if ( pSentryGun )
						{
							flDamageBase *= tf2c_defenceboost_sentry_resistance.GetFloat();
							DevMsg( "Blocked sentry damage via TF_COND_DEFENSEBUFF\n" );
						}
						else
						{
							flDamageBase *= tf2c_defenceboost_resistance.GetFloat();
							DevMsg("Blocked damage via TF_COND_DEFENSEBUFF\n");
						}

						CTFPlayer* pProvider = ToTFPlayer( pVictim->m_Shared.GetConditionProvider( TF_COND_DEFENSEBUFF ) );
						if (pProvider && pProvider != pVictim) // ok maybe it's about time it got its own function but later
						{
							if (!outParams.vecPlayDamageReductionSoundFullPlayers.HasElement(pProvider))
								outParams.vecPlayDamageReductionSoundFullPlayers.AddToTail(pProvider);
							if (flRealDamage <= pVictim->GetMaxHealth() * 2) // Try to safeguard against backstab, overkill or trigger_hurt damages giving ridiculous amount of points)
							{
								int iOriginalDamage = Ceil2Int(flOriginalDamage + 0.5f);
								int iDamageBase = Ceil2Int(flDamageBase + 0.5f);
								int iDamageBlocked = max(iOriginalDamage - iDamageBase, 0);

								IGameEvent* event = gameeventmanager->CreateEvent("damage_blocked");
								if (event)
								{
									event->SetInt("victim", pVictim ? pVictim->GetUserID() : -1);
									event->SetInt("provider", pProvider ? pProvider->GetUserID() : -1);
									event->SetInt("attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1);
									event->SetInt("amount", iDamageBlocked);
									gameeventmanager->FireEvent(event);
								}

								if (!pProvider->IsEnemy(pVictim))
								{
									CTF_GameStats.Event_PlayerBlockedDamage(pProvider, iDamageBlocked);
								}
							}
						}
					}

					if ((pVictim->m_Shared.IsLoserStateStunned() || pVictim->m_Shared.IsControlStunned()))
					{
						flDamageBase *= tf2c_damagescale_stun.GetFloat();
					}

					if (info.GetDamageCustom() == TF_DMG_FALL_DAMAGE)
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_fall);
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_fall_active);
					}

					if (pTFAttacker)
					{
						if (pVictim->m_Shared.InCond(TF_COND_BURNING))
						{
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFAttacker->GetActiveWeapon(), flDamageBase, mult_dmg_vs_burning);
						}

						if (pTFAttacker->m_Shared.InCond(TF_COND_BURNING))
						{
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_burning_target_wearer);
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_burning_target_active);
						}

						if (pTFAttacker->m_Shared.InCond(TF_COND_TRANQUILIZED) && !IsDOTDmg(info) && pWeapon && !pWeapon->IsMeleeWeapon())
						{
							float flOriginalDamage = flDamageBase;
							flDamageBase *= tf2c_tranq_ranged_damage_factor.GetFloat();
							CTFPlayer* pProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED));
							if (pProvider && pProvider != pVictim)
							{
								if (!outParams.vecPlayDamageReductionSoundFullPlayers.HasElement(pProvider))
									outParams.vecPlayDamageReductionSoundFullPlayers.AddToTail(pProvider);
								if (flRealDamage <= pVictim->GetMaxHealth() * 2) // Try to safeguard against backstab, overkill or trigger_hurt damages giving ridiculous amount of points)
								{
									int iOriginalDamage = Ceil2Int(flOriginalDamage + 0.5f);
									int iDamageBase = Ceil2Int(flDamageBase + 0.5f);
									int iDamageBlocked = max(iOriginalDamage - iDamageBase, 0);

									IGameEvent* event = gameeventmanager->CreateEvent("damage_blocked");
									if (event)
									{
										event->SetInt("victim", pVictim ? pVictim->GetUserID() : -1);
										event->SetInt("provider", pProvider ? pProvider->GetUserID() : -1);
										event->SetInt("attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1);
										event->SetInt("amount", iDamageBlocked);
										gameeventmanager->FireEvent(event);
									}

									if (!pProvider->IsEnemy(pVictim))
									{
										CTF_GameStats.Event_PlayerBlockedDamage(pProvider, iDamageBlocked);

										IGameEvent* event = gameeventmanager->CreateEvent("tranq_support");
										if (event)
										{
											event->SetInt("userid", pProvider ? pProvider->GetUserID() : -1);
											event->SetInt("amount", iDamageBlocked);
											gameeventmanager->FireEvent(event);
										}
									}
								}
							}
						}
					}

					// Should these damage resistances be under in !pierce check or not... Will figure out when someone finds a use for that attribute

					// Damage taken from all sources.
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken);
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_active);

					if (info.GetDamageType() & DMG_BLAST)
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_explosions);
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_explosions_active);
						if (info.GetInflictor() && info.GetInflictor()->GetEnemy() == pVictim)
						{
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_explosions_direct);
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_explosions_direct_active);
						}
						else
						{
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_explosions_splash);
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_explosions_splash_active);
						}
					}

					if ((info.GetDamageType() & DMG_IGNITE) || (info.GetDamageType() & DMG_BURN))
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_fire);
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_fire_active);
					}

					if (info.GetDamageType() & DMG_BURN)
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_afterburn);
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_afterburn_active);
					}

					if (info.GetDamageType() & DMG_BULLET)
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, mult_dmgtaken_from_bullets);
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_bullets_active);
					}

					if (tf2c_spy_gun_mettle.GetInt() == 1)
					{
						if (pVictim->IsPlayerClass(TF_CLASS_SPY, true))
						{
							// Standard Stealth gives small damage reduction.
							if (pVictim->m_Shared.InCond(TF_COND_STEALTHED))
							{
								flDamageBase *= tf_stealth_damage_reduction.GetFloat();
								// Live TF2 Spy reduces the crit damage portion, too
								flDamageBonus *= tf_stealth_damage_reduction.GetFloat();
								bPlayDamageReductionSoundOverride = true;
							}
						}
					}
				}
			}

			if (info.GetInflictor() && info.GetInflictor()->IsBaseObject())
			{
				CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>(info.GetInflictor());
				if (pSentry)
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flDamageBase, dmg_from_sentry_reduced);
					if (pVictim && pVictim->m_Shared.InCond(TF_COND_SHIELD_CHARGE))
					{
						float flSentryHitChanceShieldCharge = 1.0f;
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, flSentryHitChanceShieldCharge, sentry_hit_chance_shield_charge);
						if (flSentryHitChanceShieldCharge < 1.0f)
						{
							// simple rand functions as it's server-side only with no client effect so far
							if (RandomFloat() > flSentryHitChanceShieldCharge)
							{
								flDamageBase = 0.0f;
								flDamageBonus = 0.0f;
							}
						}
					}
				}
			}
		}

		// If the damage changed at all play the resist sound.
		if ( flDamageBase != flRawDamage )
		{
			if (!bPlayDamageReductionSoundOverride)
			{
				outParams.bPlayDamageReductionSound = true;
			}
		}

		// Stomp flRealDamage with resist adjusted values.
		flRealDamage = flDamageBase + flDamageBonus;

		// - End Resists and Boosts

		// Do a hard out in the caller.
		if ( flRealDamage == 0.0f )
			return -1;

		if ( pAttacker == pVictimBaseEntity && ( info.GetDamageType() & DMG_BLAST ) && info.GetDamagedOtherPlayers() == 0 )
		{
			// If we attacked ourselves, hurt no other players, and it is a blast,
			// check the attribute that reduces rocket jump damage.
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetAttacker(), flRealDamage, rocket_jump_dmg_reduction );
			if( pTFAttacker->GetActiveWeapon() )
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFAttacker->GetActiveWeapon(), flRealDamage, rocket_jump_dmg_reduction_weapon );
			outParams.bSelfBlastDmg = true;
		}

		if ( pAttacker == pVictimBaseEntity )
		{
			if ( info.GetWeapon() )
			{
				int iNoSelfBlastDamage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iNoSelfBlastDamage, no_self_blast_dmg );

				const bool bIgnoreThisSelfDamage = ( iNoSelfBlastDamage > 0 );
				if ( bIgnoreThisSelfDamage )
				{
					flRealDamage = 0;
				}

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flRealDamage, blast_dmg_to_self );
			}
			else
			{
				// If the self damage if from a explosive flag remove the damage.
				CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( info.GetInflictor() );
				if ( pFlag )
				{
					flRealDamage = 0.0f;
				}
			}
		}
	}

	return flRealDamage;
}

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		CTFPlayer *PTFTalker = ToTFPlayer( pTalker );
		bProximity = PTFTalker->ShouldUseProximityVoice();

		if ( sv_alltalk.GetBool() )
			return true;

		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( !pTalker->IsAlive() )
			{
				if ( !pListener->IsAlive() || tf_gravetalk.GetBool() )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

// Load the objects.txt file.
class CObjectsFileLoad : public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		LoadObjectInfos( filesystem );
		return true;
	}
} g_ObjectsFileLoad;

// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
/*
// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};
*/
// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //
	
// World.cpp calls this but we don't use it in TF.
void InitBodyQue()
{
}


CTFGameRules::~CTFGameRules()
{
	// NOTE: Don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	TFTeamMgr()->Shutdown();
	ShutdownCustomResponseRulesDicts();
}


void CTFGameRules::LevelShutdown()
{
	TheTFBots().LevelShutdown();
	hide_server.Revert();
}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEdict );

	// Handle some player commands here as they relate more directly to gamerules state.
	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "nextmap" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			char szNextMap[32];
			if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
			{
				V_strcpy_safe( szNextMap, nextlevel.GetString() );
			}
			else
			{
				GetNextLevelName( szNextMap, sizeof( szNextMap ) );
			}

			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap );

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1.0f;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{	
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			if ( mp_timelimit.GetInt() > 0 )
			{
				int iTimeLeft = GetTimeLeft();

				char szMinutes[5];
				char szSeconds[3];
				if ( iTimeLeft <= 0 )
				{
					V_sprintf_safe( szMinutes, "0" );
					V_sprintf_safe( szSeconds, "00" );
				}
				else
				{
					V_sprintf_safe( szMinutes, "%d", iTimeLeft / 60 );
					V_sprintf_safe( szSeconds, "%02d", iTimeLeft % 60 );
				}				

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
			}

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1.0f;
		}

		return true;
	}
	else if ( pPlayer->ClientCommand( args ) )
        return true;

	return BaseClass::ClientCommand( pEdict, args );
}

// Add the ability to ignore the world trace.
void CTFGameRules::Think()
{
	if ( IsInArenaMode() && m_flNextArenaNotify != 0.0f && gpGlobals->curtime >= m_flNextArenaNotify )
	{
		Arena_SendPlayerNotifications();
	}

	if ( m_ctBotCountUpdate.IsElapsed() )
	{
		m_ctBotCountUpdate.Start( 5.0f );

		int nBots = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsFakeClient() )
			{
				++nBots;
			}
		}

		tf_bot_count.SetValue( nBots );
	}

	if ( m_bNeedSteamServerCheck &&
		steamgameserverapicontext->SteamGameServer() &&
		steamgameserverapicontext->SteamGameServer()->BLoggedOn() )
	{
		m_bNeedSteamServerCheck = false;
		SendLoadingScreenInfo();
	}

	if ( !g_fGameOver )
	{
		if ( gpGlobals->curtime > m_flNextPeriodicThink )
		{
			if ( State_Get() != GR_STATE_TEAM_WIN )
			{
				if ( CheckCapsPerRound() )
					return;
			}
		}

		if ( IsInDominationMode() && gpGlobals->curtime >= m_flNextDominationThink )
		{
			if ( Domination_RunLogic() )
				return;
		}

		ManageServerSideVoteCreation();
	}
	else
	{
		// Check to see if we should change levels now.
		if ( m_flIntermissionEndTime && ( m_flIntermissionEndTime < gpGlobals->curtime ) )
		{
			// Intermission is over.
			ChangeLevel();

			// Don't run this code again!
			m_flIntermissionEndTime = 0.0f;
		}

		return;
	}

	State_Think();

	if ( m_hWaitingForPlayersTimer )
	{
		Assert( m_bInWaitingForPlayers );
	}

	if ( gpGlobals->curtime > m_flNextPeriodicThink )
	{
		// Don't end the game during win or stalemate states.
		if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_STALEMATE && State_Get() != GR_STATE_GAME_OVER )
		{
			if ( CheckWinLimit() )
				return;

			if ( CheckMaxRounds() )
				return;
		}

		CheckRestartRound();
		CheckWaitingForPlayers();

		m_flNextPeriodicThink = gpGlobals->curtime + 1.0f;
	}

	// Watch dog for cheats ever being enabled during a level.
	if ( !m_bCheatsEnabledDuringLevel && sv_cheats && sv_cheats->GetBool() )
	{
		m_bCheatsEnabledDuringLevel = true;
	}

	// Bypass teamplay think.
	CGameRules::Think();
}

void CTFGameRules::GoToIntermission( void )
{
	if ( IsInTournamentMode() )
		return;

	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	CTF_GameStats.Event_GameEnd();

	float flWaitTime = mp_chattime.GetInt();

	if ( tv_delaymapchange.GetBool() )
	{
		if ( HLTVDirector()->IsActive() )
			flWaitTime = MAX( flWaitTime, HLTVDirector()->GetDelay() );
	}

	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

	// Set all players to FL_FROZEN.
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );

			// Show the proper scoreboard.
			if ( IsFourTeamGame() )
			{
				pPlayer->ShowViewPortPanel( PANEL_FOURTEAMSCOREBOARD );
			}
			else
			{
				pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
			}
		}
	}

	// Print out map stats to a text file.
	//WriteStatsFile( "stats.xml" );

	State_Enter( GR_STATE_GAME_OVER );
}

bool CTFGameRules::CheckCapsPerRound()
{
	if ( tf_flag_caps_per_round.GetInt() > 0 )
	{
		int iMaxCaps = -1;
		CTFTeam *pMaxTeam = NULL;

		// Check to see if any team has won a "round".
		for ( int iTeam = FIRST_GAME_TEAM, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			// We might have more than one team over the caps limit (if the server op lowered the limit)
			// so loop through to see who has the most among teams over the limit.
			if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
			{
				if ( pTeam->GetFlagCaptures() > iMaxCaps )
				{
					iMaxCaps = pTeam->GetFlagCaptures();
					pMaxTeam = pTeam;
				}
			}
		}

		if ( iMaxCaps != -1 && pMaxTeam )
		{
			SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
			return true;
		}
	}

	return false;
}

bool CTFGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	// Guard against NULL pointers if players disconnect.
	if ( !pPlayer || !pAttacker )
		return false;

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

int CTFGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pAttacker )
{
	if ( !pPlayer || !pAttacker || ( !pAttacker->IsPlayer() && !pAttacker->IsBaseObject() ) )
		return GR_NOTTEAMMATE;

	// Either you are on the other player's team or you're not.
	if ( pPlayer->InSameTeam( pAttacker ) )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}


Vector DropToGround( CBaseEntity *pMainEnt, const Vector &vPos, const Vector &vMins, const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );
	while ( pSpot )
	{
		// Trace a box here.
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// The successful spawn point's location.
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// Drop down to ground.
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// The location the player will spawn at.
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// Draw the spawn angles.
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// Failed spawn point location.
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// Increment pSpot.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	if ( pRules )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
		if ( pPlayer )
		{
			for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
			{
				float flMin = pRules->GetRespawnWaveMaxLength( i );
				float flScalar = pRules->GetRespawnTimeScalar( i );
				float flNextRespawn = pRules->GetNextRespawnWave( i, NULL ) - gpGlobals->curtime;

				char szText[128];
				V_sprintf_safe( szText, "%s: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", g_aTeamNames[i], flMin, flScalar, flNextRespawn );

				ClientPrint( pPlayer, HUD_PRINTTALK, szText );
			}
		}
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams" );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	// Get valid spawn point.
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// Drop down to ground.
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

	// Move the player to the place it said..
	pPlayer->SetLocalOrigin( GroundPos + Vector( 0, 0, 1 ) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
// not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	bool bSpawnsLimitedClasses = false;

	bool bSpawnOverride = false;
	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn *>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		bSpawnOverride = pCTFSpawn->SpawnsAllTeams();
		bSpawnsLimitedClasses = pCTFSpawn->SpawnsLimitedClasses();
	}

	// Check the team.
	if ( !bSpawnOverride && pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
		return false;

	// Check info_player_teamspawn spawnflags for class restrictions.
	// Need new keyvalue set because we can't guarantee old maps had spawnflags set.
	if ( bSpawnsLimitedClasses )
	{
		CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
		if ( pTFPlayer )
		{
			int iClassID = pTFPlayer->GetPlayerClass()->GetClassIndex();
			if ( !pSpot->HasSpawnFlags( 1 << ( iClassID - 1 ) ) )
			{
				return false;
			}
		}
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return !trace.DidHit();
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

int CTFGameRules::ItemShouldRespawn( CItem *pItem )
{
	return BaseClass::ItemShouldRespawn( pItem );
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return 10.0f;
}

bool CTFGameRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return BaseClass::CanHaveItem( pPlayer, pItem );
}

void CTFGameRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
	CTFPowerup *pTFItem = dynamic_cast<CTFPowerup *>( pItem );
	if ( pTFItem )
	{
		pTFItem->FireOutputsOnPickup( pPlayer );
	}
}


const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	// Dedicated server output.
	if ( !pPlayer )
		return NULL;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	const char *pszFormat = NULL;

	// Team only.
	if ( bTeamOnly )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( !pTFPlayer->IsAlive() && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	else if ( pTFPlayer->PlayerIsDeveloper() && pTFPlayer->GetClientConVarBoolValue( "tf2c_dev_mark" ) )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_DevSpec";
		}
		else
		{
			if ( !pTFPlayer->IsAlive() && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_DevDead";
			}
			else
			{
				pszFormat = "TF_Chat_Dev";
			}
		}
	}
	else
	{	
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( !pTFPlayer->IsAlive() && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );
	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );
		if ( iActivity != ACT_INVALID )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
			if ( pTFPlayer )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}


void CTFGameRules::OnNavMeshLoad()
{
	TheNavMesh->SetPlayerSpawnName( "info_player_teamspawn" );
}


void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	int iPlayerIndex = pPlayer->entindex();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive.
	const char *pszName = engine->GetClientConVarValue( iPlayerIndex, "name" );
	const char *pszOldName = pPlayer->GetPlayerName();
	if ( pszOldName[0] != 0 && V_strcmp( pszOldName, pszName ) )		
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// No change allowed, force engine to use old name again.
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// Tell client that he hit the name change time limit.
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// Keep track of their hud_classautokill value.
	pTFPlayer->SetHudClassAutoKill( pTFPlayer->GetClientConVarBoolValue( "hud_classautokill" ) );

	// Keep track of their tf_medigun_autoheal value.
	pTFPlayer->SetMedigunAutoHeal( pTFPlayer->GetClientConVarBoolValue( "tf_medigun_autoheal" ) );

	// Keep track of their cl_autorezoom value.
	pTFPlayer->SetAutoRezoom( pTFPlayer->GetClientConVarBoolValue( "cl_autorezoom" ) );

	// Keep track of their cl_autoreload value.
	pTFPlayer->SetAutoReload( pTFPlayer->GetClientConVarBoolValue( "cl_autoreload" ) );

	pTFPlayer->SetRememberActiveWeapon( pTFPlayer->GetClientConVarBoolValue( "tf_remember_activeweapon" ) );
	pTFPlayer->SetRememberLastWeapon( pTFPlayer->GetClientConVarBoolValue( "tf_remember_lastswitched" ) );
	pTFPlayer->SetFlipViewModel( pTFPlayer->GetClientConVarBoolValue( "cl_flipviewmodels" ) );
	pTFPlayer->SetProximityVoice( pTFPlayer->GetClientConVarBoolValue( "tf2c_proximity_voice" ) );

	pTFPlayer->SetViewModelFOV( clamp( pTFPlayer->GetClientConVarFloatValue( "viewmodel_fov" ), 0.1f, 179.0f ) );

	Vector vecVMOffset;
	vecVMOffset.x = pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_x" );
	vecVMOffset.y = pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_y" );
	vecVMOffset.z = pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_z" );

	pTFPlayer->SetMinimizedViewModels( pTFPlayer->GetClientConVarBoolValue( "tf_use_min_viewmodels" ) );

	// This should prevent clients from sending weird values.
	if ( IsEntityPositionReasonable( vecVMOffset ) )
	{
		pTFPlayer->SetViewModelOffset( vecVMOffset );
	}
	else
	{
		pTFPlayer->SetViewModelOffset( vec3_origin );
	}

	pTFPlayer->SetSniperHoldToZoom( pTFPlayer->GetClientConVarBoolValue( "tf2c_zoom_hold_sniper" ) );

	pTFPlayer->SetShouldHaveInvisibleArms( pTFPlayer->GetClientConVarBoolValue( "tf2c_invisible_arms" ) );

	pTFPlayer->SetFixedSpreadPreference(pTFPlayer->GetClientConVarBoolValue("tf2c_fixedspread_preference"));

	pTFPlayer->SetAvoidBecomingVIP(pTFPlayer->GetClientConVarBoolValue("tf2c_avoid_becoming_vip"));

	pTFPlayer->SetSpywalkInverted(pTFPlayer->GetClientConVarBoolValue("tf2c_spywalk_inverted"));

	pTFPlayer->SetCenterFirePreference(pTFPlayer->GetClientConVarBoolValue("tf2c_centerfire_preference"));

	// Force player's FOV to 75 in background maps as they were designed with that FOV in mind.
	int iDesiredFov = pTFPlayer->GetClientConVarIntValue( "fov_desired" );
	pTFPlayer->SetDefaultFOV( gpGlobals->eLoadType != MapLoad_Background ? clamp( iDesiredFov, 75.0f, MAX_FOV ) : 75.0f );
}

static const char *g_aTaggedConVars[] =
{
	"tf_gamemode_arena",
	"arena",

	"tf_gamemode_cp",
	"cp",

	"tf_gamemode_ctf",
	"ctf",

	"tf2c_ctf_attacker_bonus",
	"ctfattackerbonus",

	"tf2c_ctf_carry_slow",
	"ctfslowcarry",

	"tf2c_ctf_reset_time_decay",
	"ctfnoresetflagtime",

	"tf2c_ctf_touch_return",
	"ctftouchreturn",

	"tf_gamemode_mvm",
	"mvm",

	"tf_gamemode_passtime",
	"passtime",

	"tf_gamemode_payload",
	"payload",

	"tf_gamemode_rd",
	"rd",

	"tf_gamemode_sd",
	"sd",

	"tf_gamemode_vip",
	"vip",

	"tf2c_vip_criticals",
	"vipcriticals",

	"tf2c_autojump",
	"autojump",

	"tf2c_bunnyjump_max_speed_factor",
	"bunnyjump",

	"tf2c_duckjump",
	"duckjump",

	"tf2c_building_gun_mettle",
	"nogunmettlebuildings",

	"tf2c_spy_gun_mettle",
	"gunmettlespies",

	"tf2c_infinite_ammo",
	"infiniteammo",

	"tf2c_medigun_setup_uber",
	"nomedigunsetupuber",

	"tf2c_airblast",
	"noairblast",

	"tf2c_building_hauling",
	"nobuildinghauling",

	"tf2c_building_upgrades",
	"nobuildingupgrades",

	"tf2c_nemesis_relationships",
	"nodominations",

	"tf2c_disablefreezecam",
	"nofreezecam",

	"tf2c_disable_player_shadows",
	"noplayershadows",

	"tf2c_stunned_taunting",
	"stunnedtaunting",

	"tf2c_disable_loser_taunting",
	"nolosertaunting",

	"tf2c_weapon_noreload",
	"noreload",

	"tf2c_grenadelauncher_old_maxammo",
	"oldglmaxammo",

	"tf2c_rocketlauncher_old_maxammo",
	"oldrlmaxammo",

	"tf2c_bot_random_loadouts",
	"randombotloadouts",

	"tf2c_randomizer",
	"randomizer",

	"tf2c_allow_special_classes",
	"specialclasses",

	"tf2c_force_stock_weapons",
	"stockweapons",

	"tf2c_allow_thirdperson",
	"thirdperson",

	"tf_birthday",
	"birthday",

	"tf_bot_count",
	"bots",

	"tf_damage_disablespread",
	"dmgspread",

	"mp_fadetoblack",
	"fadetoblack",

	"mp_friendlyfire",
	"friendlyfire",

	"mp_highlander",
	"highlander",

	"tf_weapon_criticals",
	"nocrits",

	"tf_use_fixed_weaponspreads",
	"nospread",

	"mp_disable_respawn_times",
	"norespawntime",

	"mp_respawnwavetime",
	"respawntimes",

	"mp_stalemate_enable",
	"suddendeath",
};

//-----------------------------------------------------------------------------
// Purpose: Tags
//-----------------------------------------------------------------------------
void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	// FIXME: GETTAGGEDCVARLIST CAUSES A VILE KV LEAK + BUFFER OVERRUN!!
	// -sappho
	return;

	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aTaggedConVars ) % 2 == 0 );

	BaseClass::GetTaggedConVarList( pCvarTagList );

	for ( int i = 0, c = ARRAYSIZE( g_aTaggedConVars ); i < c; i += 2 )
	{
		KeyValues *pKeyValue = new KeyValues( g_aTaggedConVars[i] );
		pKeyValue->SetString( "convar", g_aTaggedConVars[i] );
		pKeyValue->SetString( "tag", g_aTaggedConVars[i + 1] );
		pCvarTagList->AddSubKey( pKeyValue );
	}
}


void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );
	if ( !pPlayer )
		return;

	if ( FStrEq( pKeyValues->GetName(), "FreezeCamTaunt" ) )
	{
		int iCmdPlayerID = pPlayer->GetUserID();
		int iAchieverIndex = pKeyValues->GetInt( "achiever" );

		CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );
		if ( pAchiever && ( pAchiever->GetUserID() != iCmdPlayerID ) )
		{
			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();
			if ( g_TauntCamAchievements[iClass] != 0 )
			{
				pAchiever->AwardAchievement( g_TauntCamAchievements[iClass] );
			}
		}
	}
#if 0
#ifndef CLIENT_DLL
	else if ( FStrEq( pKeyValues->GetName(), "LoadInventory" ) )
	{
		// This could be made more efficient by eliminating all the back-and-forth string conversions
		// for class names and slot names (both here and in CTFInventory::GetAllItemPresets)
		// and just using the order of subkeys as an implicit integer index
		for ( int iClass = TF_CLASS_UNDEFINED; iClass < TF_CLASS_COUNT_ALL; ++iClass )
		{
			KeyValues *pClass = pKeyValues->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
			if ( pClass )
			{
				for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; ++iSlot )
				{
					int iPreset = pClass->GetInt( g_LoadoutSlots[iSlot], -1 );
					if ( iPreset != -1 )
					{
						pPlayer->HandleCommand_ItemPreset( iClass, iSlot, iPreset );
					}
				}
			}
		}
	}
#endif
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type.
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;
		if ( pTFPlayer )
		{
			// Get the max carrying capacity for this ammo
			// and check if they have room for more of this type.
			int iMaxCarry = pTFPlayer->GetMaxAmmo( iAmmoIndex, true );
			if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				return true;
		}
	}

	return false;
}


void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer and the scorer.
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CTFPlayer *pScorer = ToTFPlayer( info.GetAttacker() );
	CBaseEntity *pAssisterWeapon = NULL;
	CTFPlayer *pAssister = GetAssister( pTFPlayerVictim, pScorer, info, pAssisterWeapon );
	CBaseObject *pObject = dynamic_cast<CBaseObject *>( info.GetWeapon() );

	// If killer is a base object, tell them that they got a kill.
	if ( pObject )
	{
		pObject->IncrementKills();
		pInflictor = pObject;

		if ( pObject->GetType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetBuilder();
			if ( pOwner )
			{
				// Keep track of max kills per a single sentry gun in the player object.
				int iKills = pObject->GetKills();
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
					CTF_GameStats.Event_MaxSentryKills( pOwner, iKills );
				}

				// UNDONE: Not used by any TF2C achievements.
				/*// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}*/
			}
		}
	}

	// Find the area the player is in and see if his death causes a block.
	for ( CTriggerAreaCapture *pArea : CTriggerAreaCapture::AutoList() )
	{
		if ( pArea->CheckIfDeathCausesBlock( ToBaseMultiplayerPlayer( pVictim ), pScorer ) )
			break;
	}

	// Determine if this kill affected a nemesis relationship.
	int iDeathFlags = 0;

	if ( pScorer )
	{	
		CalcDominationAndRevenge( pScorer, pTFPlayerVictim, false, &iDeathFlags );

		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, pTFPlayerVictim, true, &iDeathFlags );
		}

		if ( IsInArenaMode() && tf_arena_first_blood.GetBool() && !m_bFirstBlood && pScorer != pTFPlayerVictim && State_Get() == GR_STATE_STALEMATE )
		{
			m_bFirstBlood = true;

			float flElapsedTime = gpGlobals->curtime - m_flRoundStartTime;
			if ( flElapsedTime <= 20.0f )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FIRSTBLOOD_FAST );
			}
			else if ( flElapsedTime < 50.0f )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FIRSTBLOOD );
			}
			else
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_FIRSTBLOOD_FINALLY );
			}

			iDeathFlags |= TF_DEATH_FIRST_BLOOD;
			pScorer->m_Shared.AddCond( TF_COND_CRITBOOSTED_FIRST_BLOOD, 5.0f );
		}

		// Arena announcer line for last living player per team.
		if ( IsInArenaMode() && pTFPlayerVictim && State_Get() == GR_STATE_STALEMATE )
		{
			for ( int iTeam = LAST_SHARED_TEAM + 1, iTeamNum = GetNumberOfTeams(); iTeam < iTeamNum; iTeam++ )
			{
				CTFTeam *pTFTeam = GetGlobalTFTeam( iTeam );
				if ( !pTFTeam )
					continue;

				if ( pTFPlayerVictim->GetTeamNumber() != iTeam )
					continue;

				CUtlVector<CBasePlayer *> playersAlive;

				int iPlayers = pTFTeam->GetNumPlayers();
				if ( iPlayers )
				{
					int iAlive = 0;
					for ( int player = 0; player < iPlayers; player++ )
					{
						CBasePlayer *pPlayer = pTFTeam->GetPlayer( player );
						if ( pPlayer && pPlayer->IsAlive() )
						{
							iAlive++;
							playersAlive.AddToTail( pPlayer );
						}
					}

					// Called as the 2nd to last player dies, i.e. 1 remains.
					if ( iAlive == 2 )
					{
						for ( CBasePlayer *pPlayer : playersAlive )
						{
							if ( pPlayer && pPlayer != pTFPlayerVictim )
							{
								g_TFAnnouncer.Speak( pPlayer, TF_ANNOUNCER_ARENA_LASTSTANDING );
							}
						}
					}
				}
			}
		}

		if ( IsInDominationMode() )
		{
			if ( g_pDominationLogic->GetKillsGivePoints() && pScorer->IsEnemy( pTFPlayerVictim ) )
			{
				pScorer->GetTFTeam()->AddRoundScore( 1 );
				g_pDominationLogic->UpdateScoreOuput( pScorer->GetTFTeam()->GetTeamNumber() );
			}
		}

		// Feign death, purgatory death, australium death etc. are all processed here.
	}

	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	if ( pAssister )
	{
		CTF_GameStats.Event_AssistKill( pAssister, pVictim );

		if ( pAssisterWeapon && pAssisterWeapon->IsBaseObject() )
		{
			static_cast<CBaseObject *>( pAssisterWeapon )->IncrementAssists();
		}
	}

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge.
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	if ( !pAttacker || !pVictim )
		return;

	if ( !tf2c_nemesis_relationships.GetBool() || ( !friendlyfire.GetBool() && pVictim->InSameTeam( pAttacker ) ) )
		return;

	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// Calculate # of unanswered kills between killer & victim - add 1 to include current kill.
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;		
	if ( iKillsUnanswered == TF_KILLS_DOMINATION )
	{			
		// This is the Nth unanswered kill between killer and victim, killer is now dominating victim.
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );

		// Set victim to be dominated by killer.
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		pAttacker->UpdateDominationsCount();

		// Record stats.
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );

		if ( pVictim->IsVIP() )
		{
			pAttacker->AwardAchievement( TF2C_ACHIEVEMENT_DOMINATE_CIVILIAN );
		}

		//TF2C_ACHIEVEMENT_DOMINATE_DEVELOPER
		if (pVictim->PlayerIsCredited())
		{
			pAttacker->AwardAchievement( TF2C_ACHIEVEMENT_DOMINATE_DEVELOPER );
		}
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// The killer killed someone who was dominating him, gains revenge.
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );

		// Set victim to no longer be dominating the killer.
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		pVictim->UpdateDominationsCount();

		// Record stats.
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Trace line rules.
//-----------------------------------------------------------------------------
float CTFGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
	unsigned int mask, trace_t *ptr )
{
	if ( mask & CONTENTS_HITBOX )
	{
		// Special case for arrows.
		UTIL_TraceLine( vecStart, vecEnd, mask, pEntity, pEntity->GetCollisionGroup(), ptr );
		return 1.0f;
	}

	return BaseClass::WeaponTraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: Create some proxy entities that we use for transmitting data.
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource.
	g_pPlayerResource = (CPlayerResource *)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource.
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	CBaseEntity *pGameRulesProxyEnt = CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle );
	Assert( pGameRulesProxyEnt );
	pGameRulesProxyEnt->SetName( AllocPooledString( "tf_gamerules" ) );

	CBaseEntity *pRandomizerManagerProxyEnt = CBaseEntity::Create( "tf_randomizer_manager", vec3_origin, vec3_angle );
	Assert( pRandomizerManagerProxyEnt );
	pRandomizerManagerProxyEnt->SetName( AllocPooledString( "tf_randomizer_manager" ) );

	CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );

	// Add vote issues, they automatically add themselves to vote controller which deletes them on level shutdown.
	new CChangeCivilian( "ChangeCivilian" );
	new CKickIssue( "Kick" );
	new CRestartGameIssue( "RestartGame" );
	new CChangeLevelIssue( "ChangeLevel" );
	new CNextLevelIssue( "NextLevel" );
	new CExtendLevelIssue( "ExtendLevel" );
	new CScrambleTeams( "ScrambleTeams" );
}

//-----------------------------------------------------------------------------
// Purpose: Determine the class name of the weapon that got a kill.
//-----------------------------------------------------------------------------
void CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, KillingWeaponData_t &weaponData )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CTFPlayer *pScorer = ToTFPlayer( info.GetAttacker() );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );

	const char *killer_weapon_name = "world";
	const char *killer_weapon_log_name = "";
	ETFWeaponID iWeaponID = TF_WEAPON_NONE;

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// Player stores last weapon that burned him so if he burns to death we know what killed him.
		if ( pWeapon )
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();

			if ( pInflictor && pInflictor != pScorer )
			{
				CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket *>( pInflictor );
				if ( pRocket && pRocket->m_iDeflected )
				{
					// Fire weapon deflects go here.
					switch ( pRocket->GetWeaponID() )
					{
						case TF_WEAPON_FLAREGUN:
							killer_weapon_name = "deflect_flare";
							break;
					}
				}
			}
		}
		else
		{
			// Default to flamethrower if no burn weapon is specified.
			killer_weapon_name = "tf_weapon_flamethrower";
			iWeaponID = TF_WEAPON_FLAMETHROWER;
		}
	}
	else if ( pScorer && pInflictor && pInflictor == pScorer )
	{
		// If the inflictor is the killer, then it must be their current weapon doing the damage.
		if ( pWeapon )
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();
		}
	}
	else if ( IsSentryDamager( info ) )
	{
		CBaseObject *pObject = dynamic_cast<CBaseObject *>( info.GetWeapon() );
		killer_weapon_name = pObject->GetClassname();

		// In case of a sentry kill change the icon according to sentry level.
		if ( pObject->GetType() == OBJ_SENTRYGUN )
		{
			switch ( pObject->GetUpgradeLevel() )
			{
				case 2:
					killer_weapon_name = "obj_sentrygun2";
					break;
				case 3:
					killer_weapon_name = "obj_sentrygun3";
					break;
			}

			iWeaponID = info.GetDamageType() & DMG_BLAST ? TF_WEAPON_SENTRY_ROCKET : TF_WEAPON_SENTRY_BULLET;
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = pInflictor->GetClassname();

		// Ideally, we need a derived method here but 95% of the time this is going to be CBaseProjectile
		// and this is still better than doing 3 dynamic casts.
		CTFBaseProjectile *pProjectile = dynamic_cast<CTFBaseProjectile *>( pInflictor );
		if ( pProjectile )
		{
			switch ( pProjectile->GetBaseProjectileType() )
			{
				case TF_PROJECTILE_BASE_NAIL:
				{
					CTFWeaponBase *pLauncher = assert_cast<CTFWeaponBase *>( pProjectile->GetOriginalLauncher() );
					if ( pLauncher )
					{
						killer_weapon_name = pLauncher->GetClassname();
					}

					iWeaponID = pProjectile->GetWeaponID();

					break;
				}
				case TF_PROJECTILE_BASE_ROCKET:
				{
					CTFBaseRocket *pRocket = static_cast<CTFBaseRocket *>( pInflictor );
					if ( pRocket->m_iDeflected )
					{
						if ( pRocket->GetProjectileType() == TF_PROJECTILE_SENTRY_ROCKET )
						{
							killer_weapon_name = "deflect_rocket_sentry";
						}
						else
						{
							switch ( pRocket->GetWeaponID() )
							{
								case TF_WEAPON_ROCKETLAUNCHER:
									killer_weapon_name = "deflect_rocket";
									break;
								case TF_WEAPON_COMPOUND_BOW:
									killer_weapon_name = "deflect_arrow";
									break;
								case TF_WEAPON_COILGUN:
									killer_weapon_name = "deflect_coilgun";
									break;
								case TF_WEAPON_TRANQ:
									killer_weapon_name = "deflect_tranq";
									break;
							}
						}
					}

					iWeaponID = pRocket->GetWeaponID();

					break;
				}
				case TF_PROJECTILE_BASE_GRENADE:
				{
					CTFBaseGrenade *pGrenade = static_cast<CTFBaseGrenade *>( pInflictor );
					if ( pGrenade->m_iDeflected )
					{
						switch ( pGrenade->GetWeaponID() )
						{
							case TF_WEAPON_GRENADE_PIPEBOMB:
								killer_weapon_name = "deflect_sticky";
								break;
							case TF_WEAPON_GRENADE_DEMOMAN:
								killer_weapon_name = "deflect_promode";
								break;
							case TF_WEAPON_THROWABLE_BRICK:
								killer_weapon_name = "deflect_brick";
								break;
						}
					}

					iWeaponID = pGrenade->GetWeaponID();

					break;
				}
				default:
					Assert( false );
					break;
			}
		}
	}

	// Handle custom kill types after we've figured out weapon ID.
	const char *pszCustomKill = NULL;

	switch ( info.GetDamageCustom() )
	{
		case TF_DMG_CUSTOM_SUICIDE:
		case TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE:
		case TF_DMG_CUSTOM_SUICIDE_BOIOING:
		case TF_DMG_CUSTOM_SUICIDE_STOMP:
			pszCustomKill = "world";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_HADOUKEN:
			pszCustomKill = "taunt_pyro";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON:
			pszCustomKill = "taunt_heavy";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_FENCING:
			pszCustomKill = "taunt_spy";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB:
			pszCustomKill = "taunt_sniper";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_UBERSLICE:
			pszCustomKill = "taunt_medic";
			break;
		case TF_DMG_CUSTOM_TELEFRAG:
			pszCustomKill = "telefrag";
			break;
		case TF_DMG_BUILDING_CARRIED:
			pszCustomKill = "building_carried_destroyed";
			break;
		case TF_DMG_CUSTOM_PUMPKIN_BOMB:
			pszCustomKill = "pumpkindeath";
			break;
		case TF_DMG_CUSTOM_BLEEDING:
			pszCustomKill = "bleed_kill";
			break;
		case TF_DMG_CUSTOM_BOOTS_STOMP:
			pszCustomKill = "mantreads";
			break;
		case TF_DMG_CUSTOM_JUMPPAD_STOMP:
			pszCustomKill = "jumppad_stomp";
			break;
		case TF_DMG_CUSTOM_CROC:
			pszCustomKill = "crocodile";
			break;
		case TF_DMG_CUSTOM_TAUNTATK_MIRV:
			pszCustomKill = "taunt_mirv";
			break;	
		case TF_DMG_EARTHQUAKE:
			pszCustomKill = "earthquake";
			break;
	}

	if ( pszCustomKill )
	{
		V_strcpy_safe( weaponData.szWeaponName, pszCustomKill );
		weaponData.iWeaponID = iWeaponID;
		return;
	}

	// Strip certain prefixes from inflictor's classname.
	const char *prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0, c = ARRAYSIZE( prefix ); i < c; i++ )
	{
		// If prefix matches, advance the string pointer past the prefix.
		int iLength = V_strlen( prefix[i] );
		if ( !V_strncmp( killer_weapon_name, prefix[i], iLength ) )
		{
			killer_weapon_name += iLength;
			break;
		}
	}

	if ( pScorer )
	{
		// See if item schem specfifies a custom icon.
		// Only do this if the killing weapon is held by the scoring player.
		// Catches deflect kills and such.
		if ( pWeapon && pWeapon->GetOwner() == pScorer )
		{
			CEconItemDefinition *pItemDef = pWeapon->GetItem()->GetStaticData();
			if ( pItemDef )
			{
				if ( pItemDef->item_iconname[0] )
				{
					killer_weapon_name = pItemDef->item_iconname;
				}

				if ( pItemDef->item_logname[0] )
				{
					killer_weapon_log_name = pItemDef->item_logname;
				}
			}
		}
	}

	V_strcpy_safe( weaponData.szWeaponName, killer_weapon_name );
	V_strcpy_safe( weaponData.szWeaponLogName, killer_weapon_log_name );
	weaponData.iWeaponID = iWeaponID;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetAssister( CTFPlayer *pVictim, CTFPlayer *pScorer, const CTakeDamageInfo &info, CBaseEntity *&pAssisterWeapon )
{
	if ( pScorer && pVictim )
	{
		// If victim killed himself, don't award an assist to anyone else, even if there was a recent damager.
		if ( pScorer == pVictim )
			return NULL;

		if (info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_BOIOING || info.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE_STOMP)
			return NULL;

		// If an assist has been specified already, use it.
		if ( pVictim->m_Shared.GetAssist() )
			return pVictim->m_Shared.GetAssist();

		// If we're under the effect of a condition that grants assists, give one to the player that buffed us.
		CTFPlayer *pAssister = ToTFPlayer( pScorer->m_Shared.GetConditionAssistFromAttacker() );
		if ( pAssister )
			return pAssister;

		// If a player is healing the scorer, give that player credit for the assist.
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		pAssister = ToTFPlayer( pScorer->m_Shared.GetFirstHealer().Get() );
		if ( pAssister && pAssister->GetMedigun() && !IsSentryDamager( info ) )
			return pAssister;

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist (only 1 assist granted, to single other most recent damager).
		DamagerHistory_t *pDamagerHistory = pVictim->GetRecentDamagerInHistory( pScorer, TF_TIME_ASSIST_KILL );
		if ( pDamagerHistory )
		{
			pAssister = ToTFPlayer( pDamagerHistory->hDamager.Get() );
			pAssisterWeapon = pDamagerHistory->hWeapon.Get();
		}
		else
		{
			pAssister = NULL;
			pAssisterWeapon = NULL;
		}

		if ( pAssister && pAssister != pScorer )
			return pAssister;

		// If a teammate has recently helped this sentry (i.e. wrench hit), they assisted in the kill.
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( info.GetInflictor() );
		if ( pSentry )
		{
			pAssister = pSentry->GetAssistingTeammate( TF_TIME_ASSIST_KILL );
			if ( pAssister )
				return pAssister;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill.
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( pKiller == pVictim && pKiller == pInflictor )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window. If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			DamagerHistory_t *pDamagerHistory = pTFVictim->GetRecentDamagerInHistory( pVictim, TF_TIME_SUICIDE_KILL_CREDIT );
			return ToTFPlayer( pDamagerHistory ? pDamagerHistory->hDamager.Get() : pVictim );
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - Killed player
//			&info - Killing damage
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	int killer_ID = 0;

	// Find the killer and the scorer.
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CBaseEntity *pAssisterWeapon = NULL;
	CTFPlayer *pAssister = GetAssister( pTFPlayerVictim, pScorer, info, pAssisterWeapon );
	int iWeaponDefIndex = -1;

	// Work out what killed the player, and send a message to all clients about it.
	KillingWeaponData_t weaponData;
	GetKillingWeaponName( info, pTFPlayerVictim, weaponData );

	// Is the killer a client?
	if ( pScorer )
	{
		killer_ID = pScorer->GetUserID();

		if ( pScorer->GetActiveTFWeapon() && pScorer->GetActiveTFWeapon()->HasItemDefinition() )
		{
			iWeaponDefIndex = pScorer->GetActiveTFWeapon()->GetItemID();
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", weaponData.szWeaponName );
		event->SetString( "weapon_logclassname", weaponData.szWeaponLogName );
		event->SetInt( "weaponid", weaponData.iWeaponID );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted.
		event->SetInt( "death_flags", pTFPlayerVictim->GetDeathFlags() );
		event->SetInt( "weapon_def_index", iWeaponDefIndex );
		event->SetInt( "stun_flags", pTFPlayerVictim->m_iOldStunFlags );
		event->SetInt( "crittype", info.GetCritType() );
		int iIsSilentKill = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), iIsSilentKill, set_silent_killer_no_killfeed);
		event->SetBool("silent_kill", iIsSilentKill != 0);

		pTFPlayerVictim->m_iOldStunFlags = 0;

		if (pInflictor && pInflictor->GetEnemy())
		{
			event->SetInt("inflictor_enemy", pInflictor->GetEnemy()->entindex());
		}
		else
		{
			event->SetInt("inflictor_enemy", 0);
		}

		event->SetInt("multicount", info.GetMultiCount());

		gameeventmanager->FireEvent( event );
	}

	// If this is VIP then his team loses time and/or loses the round.
	// Potentially Controversial: Don't register anything if they manage to suicide,
	// whether it's an "escape" attempt or they're trolling their team.
	if ( pTFPlayerVictim->IsVIP() && !( pTFPlayerVictim == pScorer || !( pScorer || pAssister ) ) )
	{
		// I don't fully understand the above condition to be honest...
		// Exclude VIP deaths before the round has properly started, too.
		if ( IsInWaitingForPlayers() || InSetup() )
			return;

		OnVIPKilled( pTFPlayerVictim, pScorer, pAssister, info );
	}

	LogLifeStats(pTFPlayerVictim);
}

//-----------------------------------------------------------------------------
// Purpose: TF2C Player Statistics
// Input  : *pPlayer - Player on death, round respawn, or disconnect
//-----------------------------------------------------------------------------
void CTFGameRules::LogLifeStats( CTFPlayer* pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	if ( tf2c_log_item_details.GetBool() )
	{
		IGameEvent* event = gameeventmanager->CreateEvent( "player_life_stats", true );
		if ( event )
		{
			RoundStats_t* pStats = &CTF_GameStats.FindPlayerStats( pTFPlayer )->statsCurrentLife;

			if ( pStats )
			{
				int iPoints = CalcPlayerScore( pStats );
				int iKills = pStats->m_iStat[TFSTAT_KILLS];
				int iDamage = pStats->m_iStat[TFSTAT_DAMAGE];
				int iHealing = pStats->m_iStat[TFSTAT_HEALING];
				int iUbers = pStats->m_iStat[TFSTAT_INVULNS];
				int iDamageAssist = pStats->m_iStat[TFSTAT_DAMAGE_ASSIST];
				int iDamageBlocked = pStats->m_iStat[TFSTAT_DAMAGE_BLOCKED];
				int iBonusPoints = pStats->m_iStat[TFSTAT_BONUS_POINTS];
				int iLifeSeconds = Floor2Int( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );

				event->SetInt( "userid", pTFPlayer->GetUserID() );
				event->SetInt( "points", iPoints );
				event->SetInt( "kills", iKills );
				event->SetInt( "damage", iDamage );
				event->SetInt( "healing", iHealing );
				event->SetInt( "ubers", iUbers );
				event->SetInt( "damageassist", iDamageAssist );
				event->SetInt( "damageblocked", iDamageBlocked );
				event->SetInt( "bonuspoints", iBonusPoints );
				event->SetInt( "lifetime", iLifeSeconds );

				// Console log
				char buf[2048];

				V_sprintf_safe( buf, "Player \"%s<%i><%s><%s><%s>\" had %i points, %i kills, %i damage, %i healing, %i ubers, %i damage assist, %i damage blocked, %i bonus points, %i seconds alive, and used ",
					pTFPlayer->GetPlayerName(),
					pTFPlayer->GetUserID(),
					pTFPlayer->GetNetworkIDString(),
					pTFPlayer->GetTeam()->GetName(),
					g_aPlayerClassNames_NonLocalized[pTFPlayer->GetPlayerClass()->GetClassIndex()],

					iPoints,
					iKills,
					iDamage,
					iHealing,
					iUbers,
					iDamageAssist,
					iDamageBlocked,
					iBonusPoints,
					iLifeSeconds );

				for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; ++iSlot )
				{
					char itemBuf[256];
					CEconEntity* pItem = pTFPlayer->GetEntityForLoadoutSlot( (ETFLoadoutSlot)iSlot );
					if ( pItem )
					{
						CEconItemView* pItemView = pItem->GetItem();
						if ( pItemView )
						{
							CEconItemDefinition* pItemDef = pItemView->GetStaticData();
							if ( pItemDef )
							{
								V_sprintf_safe( itemBuf, "\"%s\" (%i), ", pItemDef->name, pItemView->GetItemDefIndex() );
								V_strcat_safe( buf, itemBuf );

								switch ( iSlot )
								{
								case TF_LOADOUT_SLOT_PRIMARY:
									event->SetString( "primary", pItemDef->name );
									event->SetInt( "primaryid", pItemView->GetItemDefIndex() );
									break;
								case TF_LOADOUT_SLOT_SECONDARY:
									event->SetString( "secondary", pItemDef->name );
									event->SetInt( "secondaryid", pItemView->GetItemDefIndex() );
									break;
								case TF_LOADOUT_SLOT_MELEE:
									event->SetString( "melee", pItemDef->name );
									event->SetInt( "meleeid", pItemView->GetItemDefIndex() );
									break;
								case TF_LOADOUT_SLOT_PDA1:
									event->SetString( "pda1", pItemDef->name );
									event->SetInt( "pda1id", pItemView->GetItemDefIndex() );
									break;
								case TF_LOADOUT_SLOT_PDA2:
									event->SetString( "pda2", pItemDef->name );
									event->SetInt( "pda2id", pItemView->GetItemDefIndex() );
									break;
								}
							}
						}
					}
				}

				UTIL_LogPrintf( "%s\n", buf );

				gameeventmanager->FireEvent( event );
			}
			else
			{
				gameeventmanager->FreeEvent( event );
			}
		}
	}
}


bool CTFGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	const CSteamID *psteamID = engine->GetClientSteamID( pEntity );
	KeyValues *pKV = m_pAuthData->FindKey( "bans" );
	if ( pKV )
	{
		for ( KeyValues *pSub = pKV->GetFirstTrueSubKey(); pSub; pSub = pSub->GetNextTrueSubKey() )
		{
			if ( psteamID )
			{
				KeyValues *pIDSub = pSub->FindKey( "id" );
				if ( pIDSub && pIDSub->GetUint64() == psteamID->ConvertToUint64() )
				{
					// SteamID is banned!
					KeyValues *pMsgSub = pSub->FindKey( "message" );
					if ( pMsgSub )
					{
						V_strncpy( reject, pMsgSub->GetString(), maxrejectlen );
					}

					return false;
				}
			}

			KeyValues *pIPSub = pSub->FindKey( "ip" );
			if ( pIPSub && pszAddress && !V_strcmp( pIPSub->GetString(), pszAddress ) )
			{
				// IP is banned!
				KeyValues *pMsgSub = pSub->FindKey( "message" );
				if ( pMsgSub )
				{
					V_strncpy( reject, pMsgSub->GetString(), maxrejectlen );
				}

				return false;
			}
		}
	}

	// Send use input to all entities named "game_playerjoin".
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEntity );
	if ( pPlayer )
	{
		FireTargets( "game_playerjoin", pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	// Clean up anything they left behind.
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_ClientDisconnected();
	}

	Arena_ClientDisconnect( pPlayer->GetPlayerName() );

	// Are any of the spies disguising as this player?
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget( true );
			}
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650.0f

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula.
		float flFallDamage = 5.0f * ( pPlayer->m_Local.m_flFallVelocity / 300.0f );

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pTFPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		if ( !tf2c_falldamage_disablespread.GetBool() )
		{
			flFallDamage *= random->RandomFloat( 0.8f, 1.2f );
		}

		return flFallDamage;
	}

	// Fall caused no damage.
	return 0.0f;
}

int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return 0;

	int iState = 0;

	for ( int i = 0, c = pMaster->GetNumPoints(); i < c; i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by BLU.
			iState |= ( 1 << i );
		}
	}

	return ( m_iCurrentRoundState = iState );
}


void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL player means show the panel to everyone.
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	// Haven't set up the round state yet.
	if ( m_iCurrentRoundState < 0 )
		return;

	KeyValues *data = new KeyValues( "data" );

	// If prev and cur are equal, we are starting from a fresh round.
	if ( m_iPrevRoundState >= 0 && !pPlayer ) 
	{
		// We have data about a previous state.
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// Don't send a delta if this is just to one player, they are joining mid-round.
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// Get bitmask representing the current miniround.
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}

	data->deleteThis();
}

bool CTFGameRules::CanChangeClassInStalemate( void )
{
	return gpGlobals->curtime < ( m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat() );
}


void CTFGameRules::SetKothTimer( CTeamRoundTimer *pTimer, int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			m_hRedKothTimer = pTimer;
			break;
		case TF_TEAM_BLUE:
			m_hBlueKothTimer = pTimer;
			break;
		case TF_TEAM_GREEN:
			m_hGreenKothTimer = pTimer;
			break;
		case TF_TEAM_YELLOW:
			m_hYellowKothTimer = pTimer;
			break;
	}
}


void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
		{
			Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
		{
			Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
		}
	}
	else if ( iTeam == TF_TEAM_GREEN )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringGreen.GetForModify()[0] = '\0';
		}
		else if ( Q_stricmp( m_pszTeamGoalStringGreen.Get(), pszGoal ) )
		{
			Q_strncpy( m_pszTeamGoalStringGreen.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
		}
	}
	else if ( iTeam == TF_TEAM_YELLOW )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringYellow.GetForModify()[0] = '\0';
		}
		else if ( Q_stricmp( m_pszTeamGoalStringYellow.Get(), pszGoal ) )
		{
			Q_strncpy( m_pszTeamGoalStringYellow.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
		}
	}
}

int CTFGameRules::GetClassLimit( int iDesiredClassIndex, int iTeam )
{
	int result = -1;

	if ( IsInTournamentMode() || tf2c_tournament_classlimits.GetBool() )
	{
		switch ( iDesiredClassIndex )
		{
			case TF_CLASS_ENGINEER:
				result = tf_tournament_classlimit_engineer.GetInt();
				break;
			case TF_CLASS_SPY:
				result = tf_tournament_classlimit_spy.GetInt();
				break;
			case TF_CLASS_PYRO:
				result = tf_tournament_classlimit_pyro.GetInt();
				break;
			case TF_CLASS_HEAVYWEAPONS:
				result = tf_tournament_classlimit_heavy.GetInt();
				break;
			case TF_CLASS_MEDIC:
				result = tf_tournament_classlimit_medic.GetInt();
				break;
			case TF_CLASS_DEMOMAN:
				result = tf_tournament_classlimit_demoman.GetInt();
				break;
			case TF_CLASS_SOLDIER:
				result = tf_tournament_classlimit_soldier.GetInt();
				break;
			case TF_CLASS_SNIPER:
				result = tf_tournament_classlimit_sniper.GetInt();
				break;
			case TF_CLASS_SCOUT:
				result = tf_tournament_classlimit_scout.GetInt();
				break;
			case TF_CLASS_CIVILIAN:
				result = tf_tournament_classlimit_civilian.GetInt();
				break;
			default:
				result = -1;
		}
	}
	else if ( IsInHighlanderMode() )
	{
		result = 1;
	}
	else if ( tf_classlimit.GetBool() )
	{
		result = tf_classlimit.GetInt();
	}
	else if ( CTFClassLimits *pLimits = dynamic_cast<CTFClassLimits *>( gEntList.FindEntityByClassname( NULL, "tf_logic_classlimits" ) ) )
	{
		do
		{
			if ( pLimits->GetTeam() == iTeam )
			{
				result = pLimits->GetLimitForClass( iDesiredClassIndex );
			}
		}
		while ( ( pLimits = dynamic_cast<CTFClassLimits *> ( gEntList.FindEntityByClassname( pLimits, "tf_logic_classlimits" ) ) ) != NULL );
	}
	else
	{
		result = -1;
	}

	return result;
}


bool CTFGameRules::CanPlayerChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex )
{
	// Allow players to join the Civilian class if the CVar is enabled.
	if ( iDesiredClassIndex > TF_LAST_NORMAL_CLASS && !tf2c_allow_special_classes.GetBool() )
		return false;

	// Don't allow manually becoming Civilian in VIP Escort mode.
	if ( iDesiredClassIndex == TF_CLASS_CIVILIAN && IsVIPMode() )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	CTFTeam *pTFTeam = pTFPlayer->GetTFTeam();
	int iClassLimit = 0;
	int iClassCount = 0;

	iClassLimit = GetClassLimit( iDesiredClassIndex, pTFTeam->GetTeamNumber() );
	if ( iClassLimit != -1 && pTFTeam && pTFPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		for ( int i = 0, c = pTFTeam->GetNumPlayers(); i < c; i++ )
		{
			CTFPlayer *pOther = ToTFPlayer( pTFTeam->GetPlayer( i ) );
			if ( pOther && pOther != pTFPlayer && pOther->IsPlayerClass( iDesiredClassIndex, true ) )
				iClassCount++;
		}

		return iClassLimit > iClassCount;
	}

	return true;
}


void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		for ( int iRule = 0, nRuleCount = m_ResponseRules.Count(); iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}

		m_ResponseRules.Purge();
	}
}


void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_sprintf_safe( szName, "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}
	}
}


void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
		WRITE_BYTE( iTeam );
	MessageEnd();
}


void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the game description in the server browser.
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetGameDescription( void )
{
	return "Team Fortress 2 Classic";
}


void CTFGameRules::HandleCTFCaptureBonus( int iTeam )
{
	float flBoostTime = tf_ctf_bonus_time.GetFloat();
	if ( m_flCTFBonusTime > -1.0f )
	{
		flBoostTime = m_flCTFBonusTime;
	}

	if ( flBoostTime > 0.0f )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == iTeam )
			{
				pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_CTF_CAPTURE, flBoostTime );
			}
		}
	}
}


void CTFGameRules::Arena_RunTeamLogic()
{
	Assert( !IsFourTeamGame() );

	if ( !IsInArenaMode() )
		return;

	if ( IsInWaitingForPlayers() )
		return;

	int nActivePlayers = CountActivePlayers();

	if ( IsInArenaQueueMode() )
	{
		bool bStreakReached = false;

		if ( tf_arena_max_streak.GetInt() > 0 && GetWinningTeam() != TEAM_UNASSIGNED && GetGlobalTFTeam( GetWinningTeam() )->GetScore() >= tf_arena_max_streak.GetInt() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "arena_match_maxstreak" );
			if ( event )
			{
				event->SetInt( "team", GetWinningTeam() );
				event->SetInt( "streak", tf_arena_max_streak.GetInt() );
				gameeventmanager->FireEvent( event );
			}

			g_TFAnnouncer.Speak( TF_ANNOUNCER_ARENA_TEAMSCRAMBLE );
			m_iWinningTeam = TEAM_UNASSIGNED;
			bStreakReached = true;
		}

		bool bReset = bStreakReached || nActivePlayers <= 0;
		if ( !IsInTournamentMode() )
		{
			Arena_ResetLosersScore( bReset );
		}

		Arena_PrepareNewPlayerQueue( bReset );

		if ( nActivePlayers > 0 )
		{
			int nPlayersNeeded = Arena_PlayersNeededForMatch();

			int iHeaviestTeam;
			int iLightestTeam;
			if ( AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) && nPlayersNeeded > 0 )
			{
				nPlayersNeeded = Min( nPlayersNeeded, m_ArenaPlayerQueue.Count() );
				int nWinners = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
				int nThreshold = Floor2Int( ( ( nWinners + nPlayersNeeded ) * 0.5f ) - nWinners );

				int iTeamNum = GetWinningTeam();
				if ( iTeamNum == TEAM_UNASSIGNED )
				{
					iTeamNum = TF_TEAM_AUTOASSIGN;
				}

				if ( nPlayersNeeded > 0 )
				{
					for ( int i = 0; i != nPlayersNeeded; ++i )
					{
						CTFPlayer *pPlayer = m_ArenaPlayerQueue[i];
						if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
							continue;

						if ( i >= nThreshold )
						{
							iTeamNum = TF_TEAM_AUTOASSIGN;
						}

						pPlayer->ForceChangeTeam( iTeamNum );
						pPlayer->SetArenaBalanceTime();
					}
				}
			}

			m_flNextArenaNotify = gpGlobals->curtime + 1.0f;

			Arena_CleanupPlayerQueue();
			Arena_NotifyTeamSizeChange();
		}
		else
		{
			State_Transition( GR_STATE_PREGAME );
		}
	}
	else
	{
		int iActiveTeams = 0;

		FOR_EACH_VEC( g_Teams, iTeam )
		{
			// Exclude spectator and unassigned and check if the team has players. 
			if ( iTeam >= TF_TEAM_RED && g_Teams[iTeam]->GetNumPlayers() > 0 )
			{
				iActiveTeams++;
			}
		}

		if ( nActivePlayers <= 0 || iActiveTeams < 2 )
		{
			State_Transition( GR_STATE_PREGAME );
		}
	}
}


void CTFGameRules::Arena_PrepareNewPlayerQueue( bool bStreakReached )
{
	CUtlVector<CTFPlayer *> players;

	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
			continue;

		if ( bStreakReached || ( GetWinningTeam() >= FIRST_GAME_TEAM && pPlayer->GetTeamNumber() != GetWinningTeam() && pPlayer->IsReadyToPlay() ) )
		{
			players.AddToTail( pPlayer );
		}
	}

	players.Sort( bStreakReached ? ScramblePlayersSort : SortPlayerSpectatorQueue );

	for ( CTFPlayer *pPlayer : players )
	{
		if ( !pPlayer->IsReadyToPlay() )
			continue;
		
		pPlayer->ChangeTeam( TEAM_SPECTATOR, false, true );
	}
}


void CTFGameRules::Arena_CleanupPlayerQueue()
{
	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
			continue;

		if ( pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			RemovePlayerFromQueue( pPlayer );
		}
	}
}


void CTFGameRules::Arena_ResetLosersScore( bool bStreakReached )
{
	if ( bStreakReached )
	{
		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			GetGlobalTeam( i )->ResetScores();
		}
	}
	else
	{
		for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
		{
			if ( i != GetWinningTeam() && GetWinningTeam() >= FIRST_GAME_TEAM )
			{
				GetGlobalTeam( i )->ResetScores();
			}
		}
	}
}


void CTFGameRules::Arena_SendPlayerNotifications()
{
	Assert( !IsFourTeamGame() );

	m_flNextArenaNotify = 0.0f;

	int nPlayersWaiting = 0;

	int nRedPlayers = GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers();
	int nBluePlayers = GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();

	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		// FIXME: This is a lame way to detect if the player is in the queue. Should it iterate through m_ArenaPlayerQueue instead?
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() || pPlayer->GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
			continue;

		++nPlayersWaiting;

		// If this guy was just switched over to spectators, notify him.
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR && pPlayer->GetLastTeamNumber() != TEAM_UNASSIGNED )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			UserMessageBegin( filter, "HudArenaNotify" );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_BYTE( TF_ARENA_MESSAGE_SITOUT );
			MessageEnd();
		}
	}

	if ( nRedPlayers + nBluePlayers == nPlayersWaiting )
		return;

	int nDiff = nPlayersWaiting - ( nRedPlayers + nBluePlayers );

	for ( int i = FIRST_GAME_TEAM; i < TF_ORIGINAL_TEAM_COUNT; ++i )
	{
		CUtlVector<CTFPlayer *> players; // TODO: Name?

		CTFTeam *pTeam = GetGlobalTFTeam( i );
		for ( int j = 0; j < pTeam->GetNumPlayers(); ++j )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( j ) );
			if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
				continue;

			players.AddToTail( pPlayer );
		}

		players.Sort( SortPlayerSpectatorQueue );

		for ( int j = players.Count() - 1; j >= 0; --j )
		{
			if ( j < players.Count() - nDiff )
				continue;

			CTFPlayer *pPlayer = players[j];
			CSingleUserRecipientFilter filter( pPlayer );
			UserMessageBegin( filter, "HudArenaNotify" );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_BYTE( TF_ARENA_MESSAGE_CAREFUL );
			MessageEnd();
		}
	}
}


void CTFGameRules::Arena_ClientDisconnect( const char *pszPlayerName )
{
	Assert( !IsFourTeamGame() );

	if ( !IsInArenaMode() )
		return;

	if ( IsInWaitingForPlayers() || State_Get() != GR_STATE_PREROUND || IsInTournamentMode() )
		return;

	if ( m_ArenaPlayerQueue.IsEmpty() )
		return;

	int iHeaviestTeam, iLightestTeam;
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
		return;

	CTFTeam *pHeaviestTeam = GetGlobalTFTeam( iHeaviestTeam );
	CTFTeam *pLightestTeam = GetGlobalTFTeam( iLightestTeam );
	if ( !pHeaviestTeam|| !pLightestTeam )
		return;

	int nPlayerDiff = pHeaviestTeam->GetNumPlayers() - pLightestTeam->GetNumPlayers();
	if ( nPlayerDiff <= 0 )
		return;

	for ( int i = 0; i != nPlayerDiff; ++i )
	{
		// NOTE: Live TF2 doesn't seem to do this check to prevent m_ArenaPlayerQueue bounds overruns.
		if ( i >= m_ArenaPlayerQueue.Count() )
			break;

		CTFPlayer *pPlayer = m_ArenaPlayerQueue[i];
		if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
			continue;

		pPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );

		UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_ClientDisconnect", pPlayer->GetPlayerName(), GetGlobalTFTeam( pPlayer->GetTeamNumber() )->GetName(), pszPlayerName );

		pPlayer->SetArenaBalanceTime();
		RemovePlayerFromQueue( pPlayer );
	}
}


void CTFGameRules::Arena_NotifyTeamSizeChange()
{
	Assert( !IsFourTeamGame() );

	CTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	int iTeamCount = pTeam->GetNumPlayers();
	if ( iTeamCount != m_iArenaTeamCount )
	{
		if ( m_iArenaTeamCount )
		{
			if ( iTeamCount >= m_iArenaTeamCount )
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeIncreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
			else
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeDecreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
		}
		m_iArenaTeamCount = iTeamCount;
	}
}


int CTFGameRules::Arena_PlayersNeededForMatch()
{
	Assert( !IsFourTeamGame() );

	int nTeamSize = tf_arena_override_team_size.GetInt();
	if ( nTeamSize <= 0 )
	{
		int nMaxPlayers = gpGlobals->maxClients - ( HLTVDirector()->IsActive() ? 1 : 0 );
		nTeamSize = (int)( ( nMaxPlayers / 3.0f ) + 0.5f );
	}

	int nReadyToPlay = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsReadyToPlay() )
		{
			++nReadyToPlay;
		}
	}

	int nThreshold = (int)( ( nReadyToPlay + m_ArenaPlayerQueue.Count() ) * 0.5f );
	int nPlayersNeeded = ( 2 * nThreshold ) - nReadyToPlay;

	nPlayersNeeded = clamp( nPlayersNeeded, 0, ( 2 * nTeamSize ) - nReadyToPlay );

	if ( GetWinningTeam() < FIRST_GAME_TEAM )
		return nPlayersNeeded;

	CTFTeam *pWinningTeam = GetGlobalTFTeam( GetWinningTeam() );
	if ( nTeamSize >= pWinningTeam->GetNumPlayers() )
		return nPlayersNeeded;

	while ( nThreshold < pWinningTeam->GetNumPlayers() )
	{
		float flBestTime = 9999.9f;
		CTFPlayer *pBestPlayer = NULL;

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer || pPlayer->IsHLTV() || pPlayer->IsReplay() )
				continue;

			if ( pPlayer->GetTeamNumber() == GetWinningTeam() && ( gpGlobals->curtime - pPlayer->GetArenaBalanceTime() ) < flBestTime )
			{
				flBestTime = gpGlobals->curtime - pPlayer->GetArenaBalanceTime();
				pBestPlayer = pPlayer;
			}
		}

		if ( pBestPlayer )
		{
			pBestPlayer->ForceChangeTeam( TEAM_SPECTATOR );
			pBestPlayer->SetArenaBalanceTime();

			if ( nPlayersNeeded < nThreshold )
				++nPlayersNeeded;

			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if ( event )
			{
				event->SetInt( "player", pBestPlayer->entindex() );
				event->SetInt( "team", ( GetWinningTeam() == TF_TEAM_BLUE ? TF_TEAM_RED : TF_TEAM_BLUE ) );
				gameeventmanager->FireEvent( event );
			}

			//UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pBestPlayer->GetPlayerName() );
		}
	}

	return nPlayersNeeded;
}


bool CTFGameRules::IsPlayerInQueue( CTFPlayer *pPlayer ) const
{
	return m_ArenaPlayerQueue.Find( pPlayer ) != -1;
}


void CTFGameRules::AddPlayerToQueue( CTFPlayer *pPlayer )
{
	if ( !IsPlayerInQueue( pPlayer ) )
	{
		m_ArenaPlayerQueue.AddToTail( pPlayer );
	}
}


void CTFGameRules::AddPlayerToQueueHead( CTFPlayer *pPlayer )
{
	if ( !IsPlayerInQueue( pPlayer ) )
	{
		m_ArenaPlayerQueue.AddToHead( pPlayer );
	}
}


void CTFGameRules::RemovePlayerFromQueue( CTFPlayer *pPlayer )
{
	m_ArenaPlayerQueue.FindAndRemove( pPlayer );
}


void CTFGameRules::ManageServerSideVoteCreation( void )
{
	if ( State_Get() == GR_STATE_PREGAME )
		return;

	if ( IsInTournamentMode() )
		return;

	if ( IsInArenaMode() )
		return;

	if ( IsInTraining() || IsInItemTestingMode() )
		return;

	if ( gpGlobals->curtime < m_flNextVoteThink )
		return;

	if ( sv_vote_issue_nextlevel_allowed.GetBool() &&
		sv_vote_issue_nextlevel_choicesmode.GetBool() &&
		!nextlevel.GetString()[0] &&
		!m_bVotedForNextMap && !m_bVoteMapOnNextRound )
	{	
		int iWinsLeft = GetWinsRemaining();
		if ( iWinsLeft == 1 )
		{
			if ( ShouldScorePerRound() )
			{
				ScheduleNextMapVote();
			}
			else
			{
				StartNextMapVote();
			}
		}
		else if ( iWinsLeft == 0 )
		{
			// We're about to change map and we didn't choose the next map yet.
			// Bring up the vote now!
			StartNextMapVote();
		}

		int iRoundsLeft = GetRoundsRemaining();
		if ( iRoundsLeft == 1 )
		{
			// The next round will be the last one, schedule the vote.
			ScheduleNextMapVote();
		}
		else if ( iRoundsLeft == 0 )
		{
			// We're about to change map and we didn't choose the next map yet.
			// Bring up the vote now!
			StartNextMapVote();
		}
		
		if ( IsGameUnderTimeLimit() && GetTimeLeft() <= 180 )
		{
			// 3 minutes left, call the map vote now.
			StartNextMapVote();
		}
	}

	m_flNextVoteThink = gpGlobals->curtime + 0.5f;
}


void CTFGameRules::StartNextMapVote( void )
{
	if ( m_bVotedForNextMap )
		return;

	if ( g_voteController )
	{
		g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
	}

	m_bVotedForNextMap = true;
	m_bVoteMapOnNextRound = false;
}


void CTFGameRules::ScheduleNextMapVote( void )
{
	if ( !m_bVotedForNextMap )
	{
		m_bVoteMapOnNextRound = true;
	}
}


void CTFGameRules::ExtendCurrentMap( void )
{
	if ( mp_timelimit.GetInt() > 0 )
	{
		m_iMapTimeBonus += 15 * 60;
		HandleTimeLimitChange();
	}

	if ( mp_maxrounds.GetInt() > 0 )
	{
		m_iMaxRoundsBonus += 5;
	}

	if ( mp_winlimit.GetInt() > 0 )
	{
		m_iWinLimitBonus += 3;
	}

	// Bring up the vote again when the time is right.
	m_bVotedForNextMap = false;
	m_bVoteMapOnNextRound = false;
}


bool CTFGameRules::IsSentryDamager( const CTakeDamageInfo &info )
{
	// Check if the weapon is a building.
	CBaseEntity *pWeapon = info.GetWeapon();
	CBaseEntity *pInflictor = info.GetInflictor();
	if ( !pWeapon || !pWeapon->IsBaseObject() )
		return false;

	if ( pInflictor && pInflictor != pWeapon )
	{
		// Verify that the building is the owner of the projectile. Catches deflects.
		if ( pWeapon != pInflictor->GetOwnerEntity() )
			return false;
	}

	return true;
}


void CTFGameRules::SendLoadingScreenInfo( void )
{
	// Fill in server info cvars, needed for the loading screen and matchmaking.
	// We're setting this to 0 first and then setting it back to 1 so callback is triggered on a listen server properly.
	tf2c_loadinginfo_sender.SetValue( 0 );

	tf2c_loadinginfo_mapname.SetValue( STRING( gpGlobals->mapname ) );
	tf2c_loadinginfo_gametype.SetValue( m_nGameType );
	tf2c_loadinginfo_fourteam.SetValue( IsFourTeamGame() );

	tf2c_loadinginfo_sender.SetValue( 1 );
}

void CTFGameRules::AssignTeamVIPs( void )
{
	for ( int i = FIRST_GAME_TEAM, c = GetNumberOfTeams(); i < c; i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );
		if ( pTeam && pTeam->IsEscorting() )
		{
			AssignVIP( i );
		}
	}
}

CTFPlayer *CTFGameRules::AssignVIP( int iTeam, CTFPlayer *pSkipPlayer /*= NULL*/ )
{
	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return NULL;

	Assert( pTeam->IsEscorting() );

	// Release previous VIP.
	CTFPlayer *pOldVIP = pTeam->GetVIP();
	if ( pOldVIP )
	{
		if ( tf2c_vip_persist.GetBool() )
			return pOldVIP;

		pOldVIP->ReturnFromVIP();
	}

	pTeam->SetVIP( NULL );

	CTFPlayer *pVIP = NULL;

	CUtlVector<CTFPlayer *> aCandidates;

	int i, c;

	// Find a player who wasn't VIP during this match.
	for ( i = 0, c = pTeam->GetNumPlayers(); i < c; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( i ) );
		if ( pPlayer && pPlayer->CanBecomeVIP() && pPlayer != pSkipPlayer )
		{
			aCandidates.AddToTail( pPlayer );
		}
	}

	if ( aCandidates.IsEmpty() )
	{
		// Reset VIP history and try again.
		ResetVIPHistoryOnTeam( pTeam->GetTeamNumber() );

		for ( i = 0, c = pTeam->GetNumPlayers(); i < c; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( i ) );
			if ( pPlayer && pPlayer->CanBecomeVIP() && pPlayer != pSkipPlayer )
			{
				aCandidates.AddToTail( pPlayer );
			}
		}
	}

	if (aCandidates.IsEmpty())
	{
		// Ok, it seems like we came up empty because all gamers have "don't pick me" set in options.
		// Let's add them all, no matter VIP history because we have already nuked it, but this is not a normal situation anyway.
		// We can't have VIP-less VIP game!
		for (i = 0, c = pTeam->GetNumPlayers(); i < c; i++)
		{
			CTFPlayer* pPlayer = ToTFPlayer(pTeam->GetPlayer(i));
			if (pPlayer && pPlayer != pSkipPlayer)
			{
				aCandidates.AddToTail(pPlayer);
			}
		}
	}

	if ( !aCandidates.IsEmpty() )
	{
		pVIP = aCandidates.Random();
		pVIP->BecomeVIP();
	}

	return pVIP;
}

void CTFGameRules::ResetVIPHistoryOnTeam( int iTeam )
{
	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return;

	for ( int i = 0, c = pTeam->GetNumPlayers(); i < c; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( i ) );
		if ( pPlayer )
		{
			pPlayer->ResetWasVIP();
		}
	}
}

void CTFGameRules::OnVIPLeft( CTFPlayer *pPlayer )
{
	// Called when VIP leaves or changes team.
	CTFPlayer *pNewVIP = AssignVIP( pPlayer->GetTeamNumber(), pPlayer );
	if ( pNewVIP )
	{
		pNewVIP->ForceRespawn();
	}
}

void CTFGameRules::OnVIPKilled( CTFPlayer *pPlayer, CTFPlayer *pKiller /*= NULL*/, CTFPlayer *pAssister /*= NULL*/, const CTakeDamageInfo &info /*= NULL*/ )
{
	if ( g_pVIPLogic )
	{
		g_pVIPLogic->OnVIPKilled( pPlayer, pKiller, pAssister, info );
	}
}

void CTFGameRules::OnVIPEscaped( CTFPlayer *pPlayer, bool bWin )
{
	if ( g_pVIPLogic )
	{
		g_pVIPLogic->OnVIPEscaped( pPlayer, bWin );
	}
}

bool CTFGameRules::Domination_RunLogic( void )
{
	if ( tf2c_domination_override_pointlimit.GetInt() >= 0 )
	{
		m_iPointLimit = tf2c_domination_override_pointlimit.GetInt();
	}
	else
	{
		if ( g_pDominationLogic )
		{
			m_iPointLimit = g_pDominationLogic->GetPointLimitMap();
		}
	}

	/*if ( PointsMayBeCaptured() && ObjectiveResource() )
	{
		if ( GetWinningTeam() != TEAM_UNASSIGNED )
		{
			// Maps can prevent win.
			if ( g_pDominationLogic->GetWinOnLimit() )
				return true;
		}
	}*/

	m_flNextDominationThink = gpGlobals->curtime + 1.0f;
	return false;
}


int CTFGameRules::GetAssignedHumanTeam()
{
	const char *pszHumanTeamName = mp_humans_must_join_team.GetString();
	for ( int iTeamNum = TF_TEAM_COUNT - 1; iTeamNum != TEAM_UNASSIGNED; --iTeamNum )
	{
		if ( !V_stricmp( pszHumanTeamName, g_aTeamNames[iTeamNum] ) )
			return iTeamNum;
	}
	
	// The default value of mp_humans_must_join_team is assumed to be "any".
	return TEAM_ANY;
}

#endif  // GAME_DLL


bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// Swap so that lowest is always first.
		V_swap( collisionGroup0, collisionGroup1 );
	}
	
	// Don't stand on COLLISION_GROUP_WEAPONs.
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == COLLISION_GROUP_WEAPON )
		return false;

	// Don't stand on projectiles.
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == COLLISION_GROUP_PROJECTILE )
		return false;

	// Rockets fired by tf_point_weapon_mimic don't collide with other rockets.
	if ( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS_NOTSOLID )
	{
		if ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS || collisionGroup0 == TFCOLLISION_GROUP_ROCKETS_NOTSOLID )
			return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks.
	if ( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKETS_NOTSOLID )
	{
		if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
			return true;

		if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
			return false;

		if ( collisionGroup0 == COLLISION_GROUP_WEAPON )
			return false;

		if ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES )
			return false;
	}

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER && collisionGroup1 == TF_COLLISIONGROUP_GRENADES )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == TF_COLLISIONGROUP_GRENADES )
		return false;

	// Respawn rooms only collide with players.
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );

#if 0
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players.
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more.
		// Nor with objects or grenades.
		switch ( collisionGroup0 )
		{
			default:
				break;
			case COLLISION_GROUP_PLAYER:
				return false;
		}
	}
#endif

	// Don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.).
	if ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES && 
		 collisionGroup1 == TF_COLLISIONGROUP_GRENADES )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER && collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}


int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60 + (float)m_iMapTimeBonus;
	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	// If the round timer is longer, let the round complete.
	// TFTODO: Do we need to worry about the timelimit running our during a round?
	int iTime = (int)( flMapChangeTime - gpGlobals->curtime );
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return iTime;
}


void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( !Q_strcmp( pszEventName, "teamplay_point_captured" ) )
	{
#ifdef GAME_DLL
		RecalculateControlPointState();

		// Keep track of how many times each team caps.
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < GetNumberOfTeams() );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		V_strcpy_safe( m_szMostRecentCappers, cappers );
		for ( int i = 0, c = Q_strlen( cappers ); i < c; i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );				
			}
		}
#endif
	}
	else if ( !Q_strcmp( pszEventName, "teamplay_capture_blocked" ) )
	{
#ifdef GAME_DLL
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "blocker" ) ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );
#endif
	}	
	else if ( !Q_strcmp( pszEventName, "teamplay_round_win" ) )
	{
#ifdef GAME_DLL
		CTF_GameStats.Event_RoundEnd( event->GetInt( "team" ), event->GetBool( "full_round" ), event->GetFloat( "round_time" ), event->GetBool( "was_sudden_death" ) );
#endif
	}
	else if ( !Q_strcmp( pszEventName, "teamplay_flag_event" ) )
	{
#ifdef GAME_DLL
		// If this is a capture event, remember the player who made the capture.
		ETFFlagEventTypes iEventType = (ETFFlagEventTypes)event->GetInt( "eventtype" );
		if ( iEventType == TF_FLAGEVENT_CAPTURE )
		{
			m_szMostRecentCappers[0] = event->GetInt( "player" );
			m_szMostRecentCappers[1] = 0;
		}
#endif
	}
#ifdef GAME_DLL
	else if ( !Q_strcmp( pszEventName, "player_escort_score" ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "player" ) ) );
		if ( pPlayer )
		{
			CTF_GameStats.Event_PlayerScoresEscortPoints( pPlayer, event->GetInt( "points" ) );
		}
	}
	else if ( !Q_strcmp( pszEventName, "vip_death" ) )
	{
		m_szMostRecentCappers[0] = event->GetInt( "attacker" );
		m_szMostRecentCappers[1] = event->GetInt( "assister" );
	}
	else if ( g_fGameOver && !Q_strcmp( pszEventName, "server_changelevel_failed" ) )
	{
		Warning( "In gameover, but failed to load the next map. Trying next map in cycle.\n" );
		nextlevel.SetValue( "" );
		ChangeLevel();
	}
#endif
#ifdef CLIENT_DLL
	else if ( !Q_strcmp( pszEventName, "game_newmap" ) )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
	else if ( !Q_strcmp( pszEventName, "overtime_nag" ) )
	{
		HandleOvertimeBegin();
	}
	else if ( !Q_strcmp( pszEventName, "arrow_impact" ) )
	{
		int iAttachedEntity = event->GetInt( "attachedEntity" );
		C_BaseFlex *pFlex = dynamic_cast<C_BaseFlex *>( ClientEntityList().GetEnt( iAttachedEntity ) );
		if ( !pFlex )
			return;

		// Create a client side arrow and have it attach itself.
		C_TFProjectile_Arrow *pArrow = new C_TFProjectile_Arrow();
		if ( !pArrow )
			return;
		
		/*const char *pszModelName = NULL;
		switch ( event->GetInt( "projectileType" ) )
		{
			case TF_PROJECTILE_ARROW:
				pszModelName = g_pszArrowModels[MODEL_ARROW_REGULAR];
				break;
			default:
				Warning( " Unsupported Projectile type on event arrow_impact!\n" );
				return;
		}*/

		pArrow->InitializeAsClientEntity( g_pszArrowModels[MODEL_ARROW_REGULAR], RENDER_GROUP_OPAQUE_ENTITY );

		C_TFPlayer *pPlayer = ToTFPlayer( ClientEntityList().GetEnt( event->GetInt( "shooter" ) ) );
		if ( pPlayer )
		{
			pArrow->m_nSkin = GetTeamSkin( pPlayer->GetTeamNumber() );
		}

		pArrow->AttachEntityToBone( pFlex, event->GetInt( "boneIndexAttached" ),
			Vector( event->GetFloat( "bonePositionX" ), event->GetFloat( "bonePositionY" ), event->GetFloat( "bonePositionZ" ) ),
			QAngle( event->GetFloat( "boneAnglesX" ), event->GetFloat( "boneAnglesY" ), event->GetFloat( "boneAnglesZ" ) ) );
		
		C_TFPlayer *pVictim = dynamic_cast<C_TFPlayer *>(pFlex);
		if( pVictim )
			pVictim->AddArrow( pArrow );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions.
//-----------------------------------------------------------------------------

// Shared ammo definition.
// JAY: Trying to make a more physical bullet response.
#define BULLET_MASS_GRAINS_TO_LB( grains )	( 0.002285f * ( grains ) / 16.0f )
#define BULLET_MASS_GRAINS_TO_KG( grains )	lbs2kg( BULLET_MASS_GRAINS_TO_LB( grains ) )

// Exaggerate all of the forces, but use real numbers to keep them consistent.
#define BULLET_IMPULSE_EXAGGERATION			1	

// Convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s.
#define BULLET_IMPULSE( grains, ftpersec )	( ( ftpersec ) * 12 * BULLET_MASS_GRAINS_TO_KG( grains ) * BULLET_IMPULSE_EXAGGERATION )


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i = 1; i < TF_AMMO_COUNT; i++ )
		{
			def.AddAmmoType( g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, 5000, 2400, 0, 10, 14 );
			Assert( def.Index( g_aAmmoNames[i] ) == i );
		}
	}

	return &def;
}


bool CTFGameRules::FlagsMayBeCapped( void )
{
	return State_Get() != GR_STATE_TEAM_WIN && !IsInWaitingForPlayers();
}


const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_pszTeamGoalStringRed.Get();
		case TF_TEAM_BLUE:
			return m_pszTeamGoalStringBlue.Get();
		case TF_TEAM_GREEN:
			return m_pszTeamGoalStringGreen.Get();
		case TF_TEAM_YELLOW:
			return m_pszTeamGoalStringYellow.Get();
	}

	return NULL;
}


CTeamRoundTimer *CTFGameRules::GetKothTimer( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_hRedKothTimer.Get();
		case TF_TEAM_BLUE:
			return m_hBlueKothTimer.Get();
		case TF_TEAM_GREEN:
			return m_hGreenKothTimer.Get();
		case TF_TEAM_YELLOW:
			return m_hYellowKothTimer.Get();
	}

	return NULL;
}


bool CTFGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
#ifdef GAME_DLL
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
#else
	CTFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
#endif
	if ( pTFPlayer )
	{
		// We can change if we're not alive.
		if ( pTFPlayer->m_lifeState != LIFE_ALIVE )
			return true;
		
		// We can change if we're not on team RED, BLU, GRN, or YLW.
		int iPlayerTeam = pTFPlayer->GetTeamNumber();
		if ( iPlayerTeam <= LAST_SHARED_TEAM )
			return true;
		
		// We can change if we've respawned/changed classes within the last 2 seconds,
		// this allows for <classname>.cfg files to change these types of ConVars.
#ifdef GAME_DLL
		// Called everytime the player respawns.
		float flRespawnTime = pTFPlayer->GetSpawnTime();
#else
		// Called when the player changes class and respawns.
		float flRespawnTime = pTFPlayer->GetClassChangeTime();
#endif
		if ( ( gpGlobals->curtime - flRespawnTime ) < 2.0f )
			return true;
	}

	return false;
}


bool CTFGameRules::AllowThirdPersonCamera( void )
{
	return tf2c_allow_thirdperson.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon in the player's inventory that would be better than
// the given weapon.
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CTFGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	CBaseCombatWeapon *pCheck;
	CBaseCombatWeapon *pBest; // This will be used in the event that we don't find a weapon in the same category.

	int iCurrentWeight = -1;
	int iBestWeight = -1; // No weapon lower than -1 can be autoswitched to.
	pBest = NULL;

	// If I have a weapon, make sure I'm allowed to holster it.
	if ( pCurrentWeapon )
	{
		if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
		{
			// Either this weapon doesn't allow autoswitching away from it or I
			// can't put this weapon away right now, so I can't switch.
			return NULL;
		}

		iCurrentWeight = pCurrentWeapon->GetWeight();
	}

	for ( int i = 0, c = pPlayer->WeaponCount(); i < c; ++i )
	{
		pCheck = pPlayer->GetWeapon( i );
		if ( !pCheck )
			continue;

		// If we have an active weapon and this weapon doesn't allow autoswitching away
		// from another weapon, skip it.
		if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
			continue;

		if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
		{
			// This weapon is from the same category.
			if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
				return pCheck;
		}
		// Don't reselect the weapon we're trying to get rid of.
		else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )
		{
			//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
			// We keep updating the 'best' weapon just in case we can't find a weapon of the same weight
			// that the player was using. This will end up leaving the player with his heaviest-weighted 
			// weapon. 
			if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
			{
				// If this weapon is useable, flag it as the best.
				iBestWeight = pCheck->GetWeight();
				pBest = pCheck;
			}
		}
	}

	// If we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 

	// If pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	return pBest;
}

#ifdef GAME_DLL
Vector MaybeDropToGround( 
						CBaseEntity *pMainEnt, 
						bool bDropToGround, 
						const Vector &vPos, 
						const Vector &vMins, 
						const Vector &vMaxs )
{
	if ( bDropToGround )
	{
		trace_t trace;
		UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
		return trace.endpos;
	}
	
	return vPos;
}

//-----------------------------------------------------------------------------
// Purpose: This function can be used to find a valid placement location for an entity.
//			Given an origin to start looking from and a minimum radius to place the entity at,
//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
//			where mins and maxs will fit.
// Input  : *pMainEnt - Entity to place
//			&vOrigin - Point to search around
//			fRadius - Radius to search within
//			nTries - Number of tries to attempt
//			&mins - mins of the Entity
//			&maxs - maxs of the Entity
//			&outPos - Return point
// Output : Returns true and fills in outPos if it found a spot.
//-----------------------------------------------------------------------------
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
{
	// This function moves the box out in each dimension in each step trying to find empty space like this:
	//
	//											  X  
	//							   X			  X  
	// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
	//							   X 			  X  
	//											  X  
	//

	Vector mins, maxs;
	pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
	mins -= pMainEnt->GetAbsOrigin();
	maxs -= pMainEnt->GetAbsOrigin();

	// Put some padding on their bbox.
	float flPadSize = 5;
	Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
	Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

	// First test the starting origin.
	if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
	{
		outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
		return true;
	}

	Vector vDims = vTestMaxs - vTestMins;


	// Keep branching out until we get too far.
	int iCurIteration = 0;
	int nMaxIterations = 15;

	int offset = 0;
	do
	{
		for ( int iDim=0; iDim < 3; iDim++ )
		{
			float flCurOffset = offset * vDims[iDim];

			for ( int iSign=0; iSign < 2; iSign++ )
			{
				Vector vBase = vOrigin;
				vBase[iDim] += (iSign*2-1) * flCurOffset;

				if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
				{
					// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
					// (Useful for keeping things from spawning behind walls near a spawn point)
					trace_t tr;
					UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

					if ( tr.fraction != 1.0 )
					{
						continue;
					}

					outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
					return true;
				}
			}
		}

		++offset;
	} while ( iCurIteration++ < nMaxIterations );

	//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
	return false;
}
#else // GAME_DLL

void CTFGameRules::SetRoundState( int iRoundState )
{
	m_iRoundState = iRoundState;
	m_flLastRoundStateChangeTime = gpGlobals->curtime;
}


void CTFGameRules::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_bOldInWaitingForPlayers = m_bInWaitingForPlayers;
	m_bOldInOvertime = m_bInOvertime;
	m_bOldInSetup = m_bInSetup;
}


void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED || 
		m_bOldInWaitingForPlayers != m_bInWaitingForPlayers ||
		m_bOldInOvertime != m_bInOvertime ||
		m_bOldInSetup != m_bInSetup )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( State_Get() == GR_STATE_STALEMATE )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
			if ( event )
			{
				event->SetInt( "reason", STALEMATE_JOIN_MID );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}

	if ( m_bInOvertime && ( m_bOldInOvertime != m_bInOvertime ) )
	{
		HandleOvertimeBegin();
	}

	if ( State_Get() == GR_STATE_STARTGAME )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: OVERTIME! OVERTIME! OVERTIME!
//-----------------------------------------------------------------------------
void CTFGameRules::HandleOvertimeBegin()
{
	g_TFAnnouncer.Speak( TF_ANNOUNCER_OVERTIME );
}


bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}


void CTFGameRules::GetTeamGlowColor( int nTeam, float &r, float &g, float &b )
{
	switch ( nTeam )
	{
		case TF_TEAM_BLUE:
			r = 0.49f; g = 0.66f; b = 0.77f;
			break;

		case TF_TEAM_RED:
			r = 0.74f; g = 0.23f; b = 0.23f;
			break;

		case TF_TEAM_GREEN:
			r = 0.23f; g = 0.68f; b = 0.23f;
			break;

		case TF_TEAM_YELLOW:
			r = 1.0f; g = 0.62f; b = 0.23f;
			break;

		default:
			r = 0.76f; g = 0.76f; b = 0.76f;
			break;
	}
}


const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// Need to remove the .360 extension on the end of the map name.
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	V_strcpy_safe( strFullpath, "media/" );	// Assume we must play out of the media directory.

	if ( IsInArenaMode() )
	{
		// Arena plays the same movie for all maps.
		V_strcat_safe( strFullpath, "arena_intro" );
	}
	else if ( IsMannVsMachineMode() )
	{
		// Same goes for MvM.
		V_strcat_safe( strFullpath, "mvm_intro" );
	}
	else if ( IsVIPMode() )
	{
		// and VIP.
		V_strcat_safe( strFullpath, "vip_intro" );
	}
	else if ( IsInDominationMode() )
	{
		// Ending with Domination.
		V_strcat_safe( strFullpath, "dom_intro" );
	}
	else
	{
		V_strcat_safe( strFullpath, mapname );
	}

	if ( bWithExtension )
	{
		// Assume we're a .bik extension type.
		V_strcat_safe( strFullpath, ".bik" );
	}

	return strFullpath;
}


bool CTFGameRules::AllowWeatherParticles( void )
{
	return !tf_particles_disable_weather.GetBool();
}


void CTFGameRules::ModifySentChat( char *pBuf, int iBufSize )
{
	if ( english.GetBool() && tf_medieval_autorp.GetBool() && ( IsInMedievalMode() || tf_medieval_autorp.GetInt() > 1 ) )
	{
		AutoRP()->ApplyRPTo( pBuf, iBufSize );
	}

	// Replace all " with ' to prevent exploits related to chat text
	// example: ";exit
	for ( char *ch = pBuf; *ch != 0; ch++ )
	{
		if ( *ch == '"' )
		{
			*ch = '\'';
		}
	}
}


bool CTFGameRules::AllowMapParticleEffect( const char *pszParticleEffect )
{
	static const char *s_WeatherEffects[] =
	{
		"tf_gamerules",
		"env_rain_001",
		"env_rain_002_256",
		"env_rain_ripples",
		"env_snow_light_001",
		"env_rain_gutterdrip",
		"env_rain_guttersplash",
		// 2.1.3
		"ash",
		"asw_snow",
		"rain",
		"rain_storm",
		"snow",
		"", // End Marker.
	};

	if ( !AllowWeatherParticles() )
	{
		if ( FindInList( s_WeatherEffects, pszParticleEffect ) )
			return false;
	}

	return true;
}

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	KeyValues *pKeyvalToAdd = new KeyValues( pszName );
	if ( pKeyvalToAdd )
	{
		pKeys->AddSubKey( pKeyvalToAdd );
	}
}
#endif

#ifdef GAME_DLL // TFBot stuff

bool CTFGameRules::CanBotChangeClass( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer || pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	CTFBotRoster *pBotRoster = NULL;
	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:    pBotRoster = m_hBotRosterRed;    break;
		case TF_TEAM_BLUE:   pBotRoster = m_hBotRosterBlue;   break;
		case TF_TEAM_GREEN:  pBotRoster = m_hBotRosterGreen;  break;
		case TF_TEAM_YELLOW: pBotRoster = m_hBotRosterYellow; break;
	}

	if ( !pBotRoster )
		return true;

	return pBotRoster->IsClassChangeAllowed();
}


bool CTFGameRules::CanBotChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex )
{
	if ( !CanPlayerChooseClass( pPlayer, iDesiredClassIndex ) )
		return false;

	CTFBotRoster *pBotRoster = NULL;
	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:    pBotRoster = m_hBotRosterRed;    break;
		case TF_TEAM_BLUE:   pBotRoster = m_hBotRosterBlue;   break;
		case TF_TEAM_GREEN:  pBotRoster = m_hBotRosterGreen;  break;
		case TF_TEAM_YELLOW: pBotRoster = m_hBotRosterYellow; break;
	}

	if ( !pBotRoster )
		return true;

	return pBotRoster->IsClassAllowed( iDesiredClassIndex );
}



void CTFGameRules::CollectCapturePoints( const CTFPlayer *pPlayer, CUtlVector<CTeamControlPoint *> *pPoints )
{
	Assert( pPoints );
	if ( !pPoints )
		return;

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Head();
	if ( IsInKothMode() && pMaster->GetNumPoints() == 1 )
	{
		pPoints->AddToTail( pMaster->GetControlPoint( 0 ) );
		return;
	}

	for ( int i = 0, c = pMaster->GetNumPoints(); i < c; ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( !pPoint )
			continue;

		if ( !pMaster->IsInRound( pPoint ) )
			continue;

		int iPoint  = pPoint->GetPointIndex();
		int nMyTeam = pPlayer->GetTeamNumber();

		// Must not own the point and must be able to capture it.
		if ( TFObjectiveResource()->GetOwningTeam( iPoint ) == nMyTeam )
			continue;

		if ( !TFObjectiveResource()->TeamCanCapPoint( iPoint, nMyTeam ) )
			continue;

		if ( !TeamMayCapturePoint( nMyTeam, iPoint ) )
			continue;

		pPoints->AddToTail( pPoint );
	}
}


void CTFGameRules::CollectDefendPoints( const CTFPlayer *pPlayer, CUtlVector<CTeamControlPoint *> *pPoints )
{
	Assert( pPoints );
	if ( !pPoints )
		return;

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Head();
	for ( int i = 0, c = pMaster->GetNumPoints(); i < c; ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( !pPoint )
			continue;

		if ( !pMaster->IsInRound( pPoint ) )
			continue;

		// Must currently own the point.
		int iPoint = pPoint->GetPointIndex();
		int nMyTeam = pPlayer->GetTeamNumber();
		if ( TFObjectiveResource()->GetOwningTeam( iPoint ) != nMyTeam )
			continue;

		// At least one enemy team must be able to capture the point.
		bool bEnemyCanCap = false;
		ForEachEnemyTFTeam( nMyTeam, [&]( int nEnemyTeam )
		{
			if ( TFObjectiveResource()->TeamCanCapPoint( iPoint, nEnemyTeam ) &&
				TeamMayCapturePoint( nEnemyTeam, iPoint ) )
			{
				bEnemyCanCap = true;
				return false;
			}

			return true;
		} );

		if ( bEnemyCanCap )
		{
			pPoints->AddToTail( pPoint );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: TFBots use this to determine which cart to push.
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToPush( int iTeam )
{
	if ( GetGameType() != TF_GAMETYPE_ESCORT )
		return NULL;

	//if ( !m_bBotPayloadToPushInit )
	//{
		if ( HasMultipleTrains() )
		{
			CTeamTrainWatcher *pPayloadRed = NULL;
			CTeamTrainWatcher *pPayloadBlue = NULL;
			CTeamTrainWatcher *pPayloadGreen = NULL;
			CTeamTrainWatcher *pPayloadYellow = NULL;

			CTeamTrainWatcher *pPayload = NULL;
			while ( ( pPayload = assert_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pPayload, "team_train_watcher" ) ) ) != NULL )
			{
				if ( pPayload->IsDisabled() )
					continue;

				if ( pPayloadRed && pPayloadBlue &&
					pPayloadGreen && pPayloadYellow )
					break;

				if ( !pPayloadRed && pPayload->GetTeamNumber() == TF_TEAM_RED )
				{
					pPayloadRed = pPayload;
				}

				if ( !pPayloadBlue && pPayload->GetTeamNumber() == TF_TEAM_BLUE )
				{
					pPayloadBlue = pPayload;
				}

				if ( !pPayloadGreen && pPayload->GetTeamNumber() == TF_TEAM_GREEN )
				{
					pPayloadGreen = pPayload;
				}

				if ( !pPayloadYellow && pPayload->GetTeamNumber() == TF_TEAM_YELLOW )
				{
					pPayloadYellow = pPayload;
				}
			}

			// Both teams: Push your own payload.
			m_hBotPayloadToPushRed = pPayloadRed;
			m_hBotPayloadToPushBlue = pPayloadBlue;
			m_hBotPayloadToPushGreen = pPayloadGreen;
			m_hBotPayloadToPushYellow = pPayloadYellow;
		}
		else
		{
			CTeamTrainWatcher *pPayload = NULL;
			while ( ( pPayload = assert_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pPayload, "team_train_watcher" ) ) ) != NULL )
			{
				if ( pPayload->IsDisabled() )
					continue;

				m_hBotPayloadToPushBlue = pPayload;
				break;
			}

			m_hBotPayloadToPushRed = NULL;
			m_hBotPayloadToPushGreen = NULL;
			m_hBotPayloadToPushYellow = NULL;
		}

		//m_bBotPayloadToPushInit = true;
	//}

	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_hBotPayloadToPushRed;
		case TF_TEAM_BLUE:
			return m_hBotPayloadToPushBlue;
		case TF_TEAM_GREEN:
			return m_hBotPayloadToPushGreen;
		case TF_TEAM_YELLOW:
			return m_hBotPayloadToPushYellow;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: TFBots use this to determine which cart to block.
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToBlock( int iTeam )
{
	if ( GetGameType() != TF_GAMETYPE_ESCORT )
		return NULL;

	if ( m_ctBotPayloadBlockUpdate.HasStarted() && m_ctBotPayloadBlockUpdate.IsElapsed() )
	{
		DevMsg( "TFGameRules: Checking PayloadToBlock status...\n" );
		//m_bBotPayloadToBlockInit = false;
	}

	// Re-evaluate PLR status every so often.
	//if ( !m_bBotPayloadToBlockInit )
	//{
		if ( HasMultipleTrains() )
		{
			CTeamTrainWatcher *pPayloadRed = NULL;
			CTeamTrainWatcher *pPayloadBlue = NULL;
			CTeamTrainWatcher *pPayloadGreen = NULL;
			CTeamTrainWatcher *pPayloadYellow = NULL;
			CTeamTrainWatcher *pPayloadLeader = NULL;
			float flProgressMax = 0.0f;

			CTeamTrainWatcher *pPayload = NULL;
			while ( ( pPayload = assert_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pPayload, "team_train_watcher" ) ) ) != NULL )
			{
				if ( pPayload->IsDisabled() )
					continue;

				if ( pPayloadRed && pPayloadBlue &&
					pPayloadGreen && pPayloadYellow &&
					pPayloadLeader )
					break;

				if ( !pPayloadRed && pPayload->GetTeamNumber() == TF_TEAM_RED )
				{
					pPayloadRed = pPayload;
					pPayloadLeader = pPayloadRed;
					DevMsg( "PayLoadLeader:\n" );
					
					if ( pPayloadLeader->GetTrainProgress() > flProgressMax )
					{
						flProgressMax = pPayloadLeader->GetTrainProgress();
					}
				}

				if ( !pPayloadBlue && pPayload->GetTeamNumber() == TF_TEAM_BLUE )
				{
					pPayloadBlue = pPayload;
					if ( pPayloadBlue->GetTrainProgress() > flProgressMax )
					{
						pPayloadLeader = pPayloadBlue;
						flProgressMax = pPayloadLeader->GetTrainProgress();
					}
				}

				if ( !pPayloadGreen && pPayload->GetTeamNumber() == TF_TEAM_GREEN )
				{
					pPayloadGreen = pPayload;
					if ( pPayloadGreen->GetTrainProgress() > flProgressMax )
					{
						pPayloadLeader = pPayloadGreen;
						flProgressMax = pPayloadLeader->GetTrainProgress();
					}
				}

				if ( !pPayloadYellow && pPayload->GetTeamNumber() == TF_TEAM_YELLOW )
				{
					pPayloadYellow = pPayload;
					if ( pPayloadYellow->GetTrainProgress() > flProgressMax )
					{
						pPayloadLeader = pPayloadYellow;
						flProgressMax = pPayloadLeader->GetTrainProgress();
					}
				}
			}

			// Both teams: Block the enemy's payload.
			m_hBotPayloadToBlockRed = pPayloadBlue;
			m_hBotPayloadToBlockBlue = pPayloadRed;
			m_hBotPayloadToBlockGreen = pPayloadYellow;
			m_hBotPayloadToBlockYellow = pPayloadGreen;

			// Prioritize blocking team about to win.
			if ( pPayloadLeader )
			{
				if ( pPayloadLeader->GetTeamNumber() == TF_TEAM_RED )
				{
					m_hBotPayloadToBlockBlue = pPayloadLeader;
					m_hBotPayloadToBlockGreen = pPayloadLeader;
					m_hBotPayloadToBlockYellow = pPayloadLeader;
				}

				if ( pPayloadLeader->GetTeamNumber() == TF_TEAM_BLUE )
				{
					m_hBotPayloadToBlockRed = pPayloadLeader;
					m_hBotPayloadToBlockGreen = pPayloadLeader;
					m_hBotPayloadToBlockYellow = pPayloadLeader;
				}
					
				if ( pPayloadLeader->GetTeamNumber() == TF_TEAM_GREEN )
				{
					m_hBotPayloadToBlockRed = pPayloadLeader;
					m_hBotPayloadToBlockBlue = pPayloadLeader;
					m_hBotPayloadToBlockYellow = pPayloadLeader;
				}
					
				if ( pPayloadLeader->GetTeamNumber() == TF_TEAM_YELLOW )
				{
					m_hBotPayloadToBlockRed = pPayloadLeader;
					m_hBotPayloadToBlockBlue = pPayloadLeader;
					m_hBotPayloadToBlockGreen = pPayloadLeader;
				}
			}

			m_ctBotPayloadBlockUpdate.Start( 10.0f );
		}
		else
		{
			CTeamTrainWatcher *pPayload = NULL;
			while ( ( pPayload = assert_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pPayload, "team_train_watcher" ) ) ) != NULL )
			{
				if ( pPayload->IsDisabled() )
					continue;

				m_hBotPayloadToBlockRed = pPayload;
				break;
			}

			m_hBotPayloadToBlockBlue = NULL;
			m_hBotPayloadToBlockGreen = NULL;
			m_hBotPayloadToBlockYellow = NULL;
		}

		//m_bBotPayloadToBlockInit = true;
	//}

	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_hBotPayloadToBlockRed;
		case TF_TEAM_BLUE:
			return m_hBotPayloadToBlockBlue;
		case TF_TEAM_GREEN:
			return m_hBotPayloadToBlockGreen;
		case TF_TEAM_YELLOW:
			return m_hBotPayloadToBlockYellow;
	}

	return NULL;
}


void CTFGameRules::ResetBotRosters()
{
	m_hBotRosterRed = NULL;
	m_hBotRosterBlue = NULL;
	m_hBotRosterGreen = NULL;
	m_hBotRosterYellow = NULL;
}


void CTFGameRules::ResetBotPayloadInfo()
{
	//m_bBotPayloadToPushInit = false;
	m_hBotPayloadToPushRed = NULL;
	m_hBotPayloadToPushBlue = NULL;
	m_hBotPayloadToPushGreen = NULL;
	m_hBotPayloadToPushYellow = NULL;

	//m_bBotPayloadToBlockInit = false;
	m_hBotPayloadToBlockRed = NULL;
	m_hBotPayloadToBlockBlue = NULL;
	m_hBotPayloadToBlockGreen = NULL;
	m_hBotPayloadToBlockYellow = NULL;
}
#endif
