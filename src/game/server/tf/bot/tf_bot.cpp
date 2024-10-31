/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "tf_bot_behavior.h"
#include "tf_bot_manager.h"
#include "tf_bot_dead.h"
#include "team.h"
#include "iserver.h"
#include "iclient.h"
#include "tf_objective_resource.h"
#include "tf_obj.h"
#include "tf_nav_mesh.h"
#include "tf_team.h"
#include "tf_bot_proxy.h"
#include "tf_bot_generator.h"
#include "tf_bot_use_item.h"
#include "tf_control_point_master.h"
#include "tf_train_watcher.h"
#include "func_capture_zone.h"
#include "entity_capture_flag.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_flamethrower.h"
#include "movevars_shared.h"
#include "tf_inventory.h"
#include "func_vip_safetyzone.h"
#include "tf_weapon_umbrella.h"
#include "tf_nav_area.h"
#include "func_flagdetectionzone.h"
#include "tf_weapon_taser.h"

#include "tf_randomizer_manager.h"


// TODO: look for all references in live TF2 to these convars
       ConVar tf_bot_force_class                      ( "tf_bot_force_class",                           "", FCVAR_GAMEDLL, "If set to a class name, all TFBots will respawn as that class" );
static ConVar tf_bot_notice_gunfire_range             ( "tf_bot_notice_gunfire_range",              "3000", FCVAR_GAMEDLL );
static ConVar tf_bot_notice_quiet_gunfire_range       ( "tf_bot_notice_quiet_gunfire_range",         "500", FCVAR_GAMEDLL );
static ConVar tf_bot_sniper_personal_space_range      ( "tf_bot_sniper_personal_space_range",       "1000", FCVAR_CHEAT );
static ConVar tf_bot_pyro_deflect_tolerance           ( "tf_bot_pyro_deflect_tolerance",             "0.5", FCVAR_CHEAT );
static ConVar tf_bot_keep_class_after_death           ( "tf_bot_keep_class_after_death",               "0", FCVAR_GAMEDLL );
static ConVar tf_bot_prefix_name_with_difficulty      ( "tf_bot_prefix_name_with_difficulty",          "0", FCVAR_GAMEDLL, "Prepend the skill level of the bot to the bot's name" );
static ConVar tf_bot_near_point_travel_distance       ( "tf_bot_near_point_travel_distance",         "750", FCVAR_CHEAT );
static ConVar tf_bot_pyro_shove_away_range            ( "tf_bot_pyro_shove_away_range",              "250", FCVAR_CHEAT,   "If a Pyro bot's target is closer than this, compression blast them away" );
static ConVar tf_bot_pyro_always_reflect              ( "tf_bot_pyro_always_reflect",                  "0", FCVAR_CHEAT,   "Pyro bots will always reflect projectiles fired at them. For tesing/debugging purposes." );
static ConVar tf_bot_sniper_spot_min_range            ( "tf_bot_sniper_spot_min_range",             "1000", FCVAR_CHEAT );
static ConVar tf_bot_sniper_spot_max_count            ( "tf_bot_sniper_spot_max_count",               "10", FCVAR_CHEAT,   "Stop searching for sniper spots when each side has found this many" );
static ConVar tf_bot_sniper_spot_search_count         ( "tf_bot_sniper_spot_search_count",            "10", FCVAR_CHEAT,   "Search this many times per behavior update frame" );
static ConVar tf_bot_sniper_spot_point_tolerance      ( "tf_bot_sniper_spot_point_tolerance",        "750", FCVAR_CHEAT );
static ConVar tf_bot_sniper_spot_epsilon              ( "tf_bot_sniper_spot_epsilon",                "100", FCVAR_CHEAT );
static ConVar tf_bot_sniper_goal_entity_move_tolerance( "tf_bot_sniper_goal_entity_move_tolerance",  "500", FCVAR_CHEAT );
static ConVar tf_bot_suspect_spy_touch_interval       ( "tf_bot_suspect_spy_touch_interval",           "5", FCVAR_CHEAT,   "How many seconds back to look for touches against suspicious spies" );
static ConVar tf_bot_suspect_spy_forced_cooldown      ( "tf_bot_suspect_spy_forced_cooldown",          "5", FCVAR_CHEAT,   "How long to consider a suspicious spy as suspicious" );
static ConVar tf_bot_debug_tags                       ( "tf_bot_debug_tags",                           "0", FCVAR_CHEAT,   "ent_text will only show tags on bots" );

static ConVar tf2c_bot_random_loadouts                ( "tf2c_bot_random_loadouts",                    "0", FCVAR_GAMEDLL,   "Bots spawn with random items" );
static ConVar tf2c_bot_random_loadouts_debug		  ( "tf2c_bot_random_loadouts_debug",              "0", FCVAR_CHEAT, "Show debug info about random bot items & items with tag bots_cant_use" );
static ConVar tf_bot_reevaluate_class_in_spawnroom    ( "tf_bot_reevaluate_class_in_spawnroom",        "1", FCVAR_GAMEDLL, "Bots change team to another needed class while alive and in spawn" );
extern ConVar tf_bot_health_ok_ratio;

extern ConVar tf2c_vip_abilities;


LINK_ENTITY_TO_CLASS( tf_bot, CTFBot );

BEGIN_ENT_SCRIPTDESC( CTFBot, CTFPlayer, "Beep boop beep boop :3" )
	DEFINE_SCRIPTFUNC_NAMED( SetAttribute, "AddBotAttribute", "Sets attribute flags on this TFBot" )
	DEFINE_SCRIPTFUNC_NAMED( ClearAttribute, "RemoveBotAttribute", "Removes attribute flags on this TFBot" )
	DEFINE_SCRIPTFUNC_NAMED( ClearAllAttributes, "ClearAllBotAttributes", "Clears all attribute flags on this TFBot" )
	DEFINE_SCRIPTFUNC_NAMED( HasAttribute, "HasBotAttribute", "Checks if this TFBot has the given attributes" )

	DEFINE_SCRIPTFUNC_NAMED( AddTag, "AddBotTag", "Adds a bot tag" )
	DEFINE_SCRIPTFUNC_NAMED( RemoveTag, "RemoveBotTag", "Removes a bot tag" )
	DEFINE_SCRIPTFUNC_NAMED( ClearTags, "ClearAllBotTags", "Clears bot tags" )
	DEFINE_SCRIPTFUNC_NAMED( HasTag, "HasBotTag", "Checks if this TFBot has the given bot tag" )

	DEFINE_SCRIPTFUNC_NAMED( SetWeaponRestriction, "AddWeaponRestriction", "Adds weapon restriction flags" )
	DEFINE_SCRIPTFUNC( RemoveWeaponRestriction, "Removes weapon restriction flags" )
	DEFINE_SCRIPTFUNC_NAMED( ClearWeaponRestrictions, "ClearAllWeaponRestrictions", "Removes all weapon restriction flags" )
	DEFINE_SCRIPTFUNC( HasWeaponRestriction, "Checks if this TFBot has the given weapons restriction flags" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsWeaponRestricted, "IsWeaponRestricted", "Checks if the given weapon is restricted for use on the bot" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDifficulty, "GetDifficulty", "Returns the bot's difficulty level" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetDifficulty, "SetDifficulty", "Sets the bot's difficulty level" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsDifficulty, "IsDifficulty", "Returns if the bot's difficulty level matches" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHomeArea, "GetHomeArea", "Returns the home nav area of the bot -- may be nil." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetHomeArea, "SetHomeArea", "Sets the home nav area of the bot" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptDelayedThreatNotice, "DelayedThreatNotice", "" )
	DEFINE_SCRIPTFUNC( UpdateDelayedThreatNotices, "" )

	DEFINE_SCRIPTFUNC( SetMaxVisionRangeOverride, "Sets max vision range for the bot" )
	DEFINE_SCRIPTFUNC( GetMaxVisionRangeOverride, "Gets max vision range override for the bot" )
	DEFINE_SCRIPTFUNC( SetScaleOverride, "Sets the scale override for the bot" )
	DEFINE_SCRIPTFUNC( SetAutoJump, "Sets if bot should automaticaly jump" )
	DEFINE_SCRIPTFUNC( ShouldAutoJump, "Returns if the bot should automatically jump" )
	DEFINE_SCRIPTFUNC( ShouldQuickBuild, "Returns if the bot should build instantly" )
	DEFINE_SCRIPTFUNC( SetShouldQuickBuild, "Sets if the bot should build instantly" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNearestKnownSappableTarget, "GetNearestKnownSappableTarget", "Gets the nearest known sappable target" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGenerateAndWearItem, "GenerateAndWearItem", "Give me an item!" )

	DEFINE_SCRIPTFUNC( IsInASquad, "Checks if we are in a squad" )
	DEFINE_SCRIPTFUNC( LeaveSquad, "Makes us leave the current squad (if any)" )
	DEFINE_SCRIPTFUNC( GetSquadFormationError, "Gets our formation error coefficient" )
	DEFINE_SCRIPTFUNC( SetSquadFormationError, "Sets our formation error coefficient" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptDisbandCurrentSquad, "DisbandCurrentSquad", "Forces the current squad to be entirely disbaned by everyone" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptFindVantagePoint, "FindVantagePoint", "Get the nav area of the closest vantage point (within distance)" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetAttentionFocus, "SetAttentionFocus", "Sets our current attention focus to this entity" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsAttentionFocusedOn, "IsAttentionFocusedOn", "Is our attention focused on this entity" )
	DEFINE_SCRIPTFUNC( ClearAttentionFocus, "Clear current focus" )
	DEFINE_SCRIPTFUNC( IsAttentionFocused, "Is our attention focused right now?" )

	DEFINE_SCRIPTFUNC( IsAmmoLow, "" )
	DEFINE_SCRIPTFUNC( IsAmmoFull, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSpawnArea, "GetSpawnArea", "Return the nav area of where we spawned" )

	DEFINE_SCRIPTFUNC( PressFireButton, "" )
	DEFINE_SCRIPTFUNC( PressAltFireButton, "" )
	DEFINE_SCRIPTFUNC( PressSpecialFireButton, "" )

	DEFINE_SCRIPTFUNC( GetBotId, "Get this bot's id" )
	DEFINE_SCRIPTFUNC( FlagForUpdate, "Flag this bot for update" )
	DEFINE_SCRIPTFUNC( IsFlaggedForUpdate, "Is this bot flagged for update" )
	DEFINE_SCRIPTFUNC( GetTickLastUpdate, "Get last update tick" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetLocomotionInterface, "GetLocomotionInterface", "Get this bot's locomotion interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBodyInterface, "GetBodyInterface", "Get this bot's body interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetIntentionInterface, "GetIntentionInterface", "Get this bot's intention interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetVisionInterface, "GetVisionInterface", "Get this bot's vision interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsEnemy, "IsEnemy", "Return true if given entity is our enemy" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFriend, "IsFriend", "Return true if given entity is our friend" )
	DEFINE_SCRIPTFUNC( IsImmobile, "Return true if we haven't moved in awhile" )
	DEFINE_SCRIPTFUNC( GetImmobileDuration, "How long have we been immobile" )
	DEFINE_SCRIPTFUNC( ClearImmobileStatus, "Clear immobile status" )
	DEFINE_SCRIPTFUNC( GetImmobileSpeedThreshold, "Return units/second below which this actor is considered immobile" )
END_SCRIPTDESC()


// this is only needed for an alternative implementation of CTFBot::Kick
#if 0
static IClient *GetClientPtrForPlayer( CBasePlayer *player )
{
	int userid = player->GetUserID();
	
	IServer *server = engine->GetIServer();
	for ( int i = server->GetClientCount() - 1; i >= 0; --i ) {
		IClient *client = server->GetClient( i );
		
		if ( client == nullptr )      continue;
		if ( !client->IsConnected() ) continue;
		
		if ( client->GetUserID() == userid ) {
			return client;
		}
	}
	
	/* not found */
	return nullptr;
}
#endif


static CTFBot::DifficultyType StringToDifficultyLevel( const char *str )
{
	if ( stricmp( str, "easy"  ) == 0 ) return CTFBot::EASY;
	if ( stricmp( str, "normal" ) == 0 ) return CTFBot::NORMAL;
	if ( stricmp( str, "hard"  ) == 0 ) return CTFBot::HARD;
	if ( stricmp( str, "expert" ) == 0 ) return CTFBot::EXPERT;
	
	return CTFBot::UNDEFINED_SKILL;
}

static const char *DifficultyLevelToString( CTFBot::DifficultyType diff )
{
	switch ( diff ) {
	case CTFBot::EASY:   return   "Easy ";
	case CTFBot::NORMAL: return "Normal ";
	case CTFBot::HARD:   return   "Hard ";
	case CTFBot::EXPERT: return "Expert ";
	}
	
	return "Undefined ";
}

BEGIN_SCRIPTENUM( EBotType, "Bot type" )
	DEFINE_ENUMCONST_NAMED( CTFBot::BOT_TYPE, "TF_BOT_TYPE", "" )
END_SCRIPTENUM()

BEGIN_SCRIPTENUM( ETFBotDifficultyType, "" )

	DEFINE_ENUMCONST_NAMED( CTFBot::UNDEFINED_SKILL, "UNDEFINED", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::EASY, "EASY", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::NORMAL, "NORMAL", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::HARD, "HARD", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::EXPERT, "EXPERT", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::NUM_DIFFICULTY_LEVELS, "NUM_DIFFICULTY_LEVELS", "" )

END_SCRIPTENUM()

BEGIN_SCRIPTENUM( FTFBotAttributeType, "" )

	DEFINE_ENUMCONST_NAMED( CTFBot::REMOVE_ON_DEATH, "REMOVE_ON_DEATH", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::AGGRESSIVE, "AGGRESSIVE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::PROXY_MANAGED, "IS_NPC", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::SUPPRESS_FIRE, "SUPPRESS_FIRE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::DISABLE_DODGE, "DISABLE_DODGE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::BECOME_SPECTATOR_ON_DEATH, "BECOME_SPECTATOR_ON_DEATH", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::QUOTA_MANAGED, "QUOTA_MANAGED", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::RETAIN_BUILDINGS, "RETAIN_BUILDINGS", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::SPAWN_WITH_FULL_CHARGE, "SPAWN_WITH_FULL_CHARGE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::ALWAYS_CRIT, "ALWAYS_CRIT", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::IGNORE_ENEMIES, "IGNORE_ENEMIES", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::HOLD_FIRE_UNTIL_FULL_RELOAD, "HOLD_FIRE_UNTIL_FULL_RELOAD", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::ALWAYS_FIRE_WEAPON, "ALWAYS_FIRE_WEAPON", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::TELEPORT_TO_HINT, "TELEPORT_TO_HINT", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::MINI_BOSS, "MINIBOSS", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::USE_BOSS_HEALTH_BAR, "USE_BOSS_HEALTH_BAR", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::IGNORE_FLAG, "IGNORE_FLAG", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::AUTO_JUMP, "AUTO_JUMP", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::AIR_CHARGE_ONLY, "AIR_CHARGE_ONLY", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::VACCINATOR_BULLETS, "PREFER_VACCINATOR_BULLETS", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::VACCINATOR_BLAST, "PREFER_VACCINATOR_BLAST", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::VACCINATOR_FIRE, "PREFER_VACCINATOR_FIRE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::BULLET_IMMUNE, "BULLET_IMMUNE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::BLAST_IMMUNE, "BLAST_IMMUNE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::FIRE_IMMUNE, "FIRE_IMMUNE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::PARACHUTE, "PARACHUTE", "" )
	DEFINE_ENUMCONST_NAMED( CTFBot::PROJECTILE_SHIELD, "PROJECTILE_SHIELD", "" )

END_SCRIPTENUM()


CTFBot::CTFBot()
{
	this->m_pLocomotion = new CTFBotLocomotion( this );
	this->m_pBody       = new CTFBotBody      ( this );
	this->m_pIntention  = new CTFBotIntention ( this );
	this->m_pVision     = new CTFBotVision    ( this );
	
	this->ClearMission();
	
	// m_hSBTarget = nullptr
	// 2c04 .Clear()
	
	this->ClearSniperSpots();
	
	this->ListenForGameEvent( "teamplay_point_startcapture" );
	this->ListenForGameEvent( "teamplay_point_captured" );
	this->ListenForGameEvent( "teamplay_round_win" ); // not actually used in FireGameEvent...
	this->ListenForGameEvent( "teamplay_flag_event" );
	this->ListenForGameEvent( "player_changeclass" );
}

CTFBot::~CTFBot()
{
	if ( this->m_pLocomotion != nullptr ) delete this->m_pLocomotion;
	if ( this->m_pBody       != nullptr ) delete this->m_pBody;
	if ( this->m_pIntention  != nullptr ) delete this->m_pIntention;
	if ( this->m_pVision     != nullptr ) delete this->m_pVision;
}


void CTFBot::Spawn()
{
	NextBotPlayer<CTFPlayer>::Spawn();
	
	this->m_HomeArea = nullptr;
	
	this->m_ctRecentPointLost.Invalidate();
	
	this->m_Squad = nullptr;
	
	this->m_bKeepClassAfterDeath = false;
	
	this->m_bLookAroundForEnemies = true;
	this->ClearAttentionFocus();
	
	this->m_SuspectedSpies.RemoveAll();
	this->m_KnownSpies    .RemoveAll();
	
	this->m_DelayedThreatNotices.RemoveAll();
	
	// TODO: do we actually want to call ClearMyControlPoint() here?
	// so that we invalidate the countdown timer as well?
	this->m_hMyControlPoint = nullptr;
	
	this->ClearSniperSpots();
	
	this->ClearTags();
	
	this->m_hFlagTarget = nullptr;
	
	this->m_RequiredWeapons.Clear();
	
	// m_bTeleQuickBuild = false
	
	this->m_flFormationError = 0.0f;
	this->m_bIsInFormation   = false;
	
	this->GetVisionInterface()->ForgetAllKnownEntities();

	// testing spawning with unlocks
	if ( tf2c_bot_random_loadouts.GetBool() )
	{
		int playerClass = GetPlayerClass()->GetClassIndex();

		// Skip picking item preset if we don't have a class yet
		if ( playerClass == TF_CLASS_UNDEFINED )
			return;

		for ( int i = 0; i < TF_FIRST_COSMETIC_SLOT; i++ )
		{
			ETFLoadoutSlot iSlot = (ETFLoadoutSlot)i;
			int iWeapons = GetTFInventory()->GetNumItems( playerClass, iSlot );

			// Filter out econ items bots aren't allowed to use
			CUtlVector<int> candidateItems;

			if ( tf2c_bot_random_loadouts_debug.GetBool() )
			{
				DevMsg( "-------\n" );
				DevMsg( "Bot %s (%s) considering slot %i...\n", this->GetPlayerName(), this->GetPlayerClass()->GetName(), iSlot );
			}

			for ( int j = 0; j < iWeapons; j++ )
			{
				CEconItemView* candidateItemEcon = GetTFInventory()->GetItem( playerClass, iSlot, j );

				if ( tf2c_bot_random_loadouts_debug.GetBool() )
					DevMsg( "Item %i: %s", j, candidateItemEcon->GetStaticData()->item_name );

				bool bCantUse = candidateItemEcon->HasTag( "bots_cant_use" );
				if ( bCantUse )
				{
					if ( tf2c_bot_random_loadouts_debug.GetBool() )
						DevMsg( " - NOT ALLOWED!\n" );
					
					continue;
				}
				DevMsg( " \n" );

				candidateItems.AddToTail( j );
			}

			int iRandomPreset = candidateItems.IsEmpty() ? 0 : candidateItems.Random();
			this->HandleCommand_ItemPreset( playerClass, i, iRandomPreset );
			if ( tf2c_bot_random_loadouts_debug.GetBool() )
				DevMsg( "Bot %s (%s) picks %i\n", this->GetPlayerName(), this->GetPlayerClass()->GetName(), iRandomPreset );
		}

		// Equip our new choice immediately instead of next life
		this->Regenerate();
	}

	this->m_flFrontlineIncDistAvg = -1.0f;
	this->m_flFrontlineIncDistFar = -1.0f;

	this->m_pOpposingTeam = this->GetOpposingTFTeam();
}

int CTFBot::DrawDebugTextOverlays()
{
	int line = 1;
	if ( !tf_bot_debug_tags.GetBool() ) {
		line = NextBotPlayer<CTFPlayer>::DrawDebugTextOverlays();
	}
	
	CUtlString str( "Tags : " );
	for ( const auto& tag : this->m_Tags ) {
		str += tag;
		str += ' ';
	}
	this->EntityText( line, str, 0.0f );
	
	return line + 1;
}

void CTFBot::Event_Killed( const CTakeDamageInfo& info )
{
	NextBotPlayer<CTFPlayer>::Event_Killed( info );
	
	if ( this->m_hProxy != nullptr ) {
		this->m_hProxy->OnKilled();
	}
	
	if ( TFGameRules()->IsMannVsMachineMode() ) {
		// TODO: MvM stuff
		Assert( false );
	}
	
	if ( this->m_hGenerator != nullptr ) {
		this->m_hGenerator->OnBotKilled( this );
	}
	
	if ( this->IsInASquad() ) {
		this->LeaveSquad();
	}
	
	CTFNavArea *lkarea = this->GetLastKnownTFArea();
	if ( lkarea != nullptr ) {
		lkarea->ForAllPotentiallyVisibleTFAreas( [=]( CTFNavArea *area ){
			area->RemovePotentiallyVisibleActor( this );
			return true;
		} );
	}
	
	CBaseEntity *inflictor = info.GetInflictor();
	if ( inflictor != nullptr && this->IsEnemy( inflictor ) ) {
		auto sentry = dynamic_cast<CObjectSentrygun *>( inflictor );
		if ( sentry != nullptr ) {
			this->SetTargetSentry( sentry );
		}
	}
	
	this->StopIdleSound();
}

void CTFBot::Touch( CBaseEntity *pOther )
{
	NextBotPlayer<CTFPlayer>::Touch( pOther );
	
	CTFPlayer *player = ToTFPlayer( pOther );
	if ( player != nullptr && this->IsEnemy( player ) ) {
		if ( player->m_Shared.IsStealthed() || player->m_Shared.InCond( TF_COND_DISGUISED ) ) {
			if ( TFGameRules()->IsMannVsMachineMode() ) {
				this->SuspectSpy( player );
			} else {
				this->DelayedThreatNotice( player, RandomFloat( this->GetVisionInterface()->GetMinRecognizeTime(), 1.0f ) );
			}
		}
		
		// TODO: come up with a better way to handle this idiocy
		TheNextBots().OnWeaponFired( player, player->GetActiveTFWeapon() );
	}
}

void CTFBot::PhysicsSimulate()
{
	/* live TF2 does this as overrides to PressFireButton etc but it makes more
	 * sense to do the check in here ( it's more robust this way ) */
	if ( this->m_Shared.IsControlStunned() || this->m_Shared.IsLoserStateStunned() || this->HasAttribute( SUPPRESS_FIRE ) ) {
		this->ReleaseFireButton();
		this->ReleaseAltFireButton();
		this->ReleaseSpecialFireButton();
	}
	
	NextBotPlayer<CTFPlayer>::PhysicsSimulate();
	
	if ( this->m_HomeArea == nullptr ) {
		this->m_HomeArea = this->GetLastKnownTFArea();
	}
	
	if ( this->HasAttribute( ALWAYS_CRIT ) && !this->m_Shared.InCond( TF_COND_CRITBOOSTED_USER_BUFF ) ) {
		this->m_Shared.AddCond( TF_COND_CRITBOOSTED_USER_BUFF );
	}
	
	this->TeamFortress_SetSpeed();
	this->TeamFortress_SetGravity();
	
	if ( this->IsInASquad() ) {
		if ( this->GetSquad()->GetMemberCount() <= 1 || this->GetSquad()->GetLeader() == nullptr ) {
			this->LeaveSquad();
		}
	}
	
	if ( !this->IsAlive() && !this->m_bKeepClassAfterDeath && !tf_bot_keep_class_after_death.GetBool() && TFGameRules()->CanBotChangeClass( this ) && !TFGameRules()->IsMannVsMachineMode() ) {
		if ( FStrEq( tf_bot_force_class.GetString(), "" ) ) {
			this->HandleCommand_JoinClass( this->GetNextSpawnClassname() );
		} else {
			this->HandleCommand_JoinClass( tf_bot_force_class.GetString() );
		}
		
		this->m_bKeepClassAfterDeath = true;
	}
}


void CTFBot::AvoidPlayers( CUserCmd *usercmd )
{
	// CTFPlayerShared +0x7d8 bool:   set true in CTFPlayer::Spawn
	// CTFPlayerShared +0x7dc Vector: set to vec3_origin in CTFPlayer::Spawn
	// ^^^ these variables appear to only actually be used within this function
	
	if ( !tf_avoidteammates         .GetBool() ) return;
	if ( !tf_avoidteammates_pushaway.GetBool() ) return;
	
	if ( this->HasTheFlag() ) return;
	
	Vector eye_fwd;
	Vector eye_right;
	this->EyeVectors( &eye_fwd, &eye_right );
	
	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers( &teammates, this->GetTeamNumber(), true );
	
	Vector move = vec3_origin;
	
	float dist = ( TFGameRules()->IsMannVsMachineMode() ? 150.0f : 50.0f );
	
	for ( auto teammate : teammates ) {
		if ( this->IsSelf( teammate ) ) continue;
		
		if ( this->IsPlayerClass( TF_CLASS_MEDIC ) && !teammate->IsPlayerClass( TF_CLASS_MEDIC ) ) {
			continue;
		}
		
		if ( this->IsInASquad() ) {
			continue;
		}
		
		Vector delta = this->GetAbsOrigin() - teammate->GetAbsOrigin();
		if ( delta.IsLengthGreaterThan( dist ) ) continue;
		
		float delta_len = delta.NormalizeInPlace();
		
		move += delta * ( 1.0f - ( delta_len / dist ) );
	}
	
	if ( !move.IsZero() ) {
		move.NormalizeInPlace();
		
		// this->m_Shared.bool_7d8   = true
		// this->m_Shared.vector_7dc = 50.0f * move
		
		usercmd->forwardmove += 50.0f * move.Dot( eye_fwd );
		usercmd->sidemove    += 50.0f * move.Dot( eye_right );
	} else {
		// this->m_Shared.bool_7d8   = false
		// this->m_Shared.vector_7dc = vec3_origin
	}
}


void CTFBot::FireGameEvent( IGameEvent *event )
{
	const char *name = event->GetName();
	
	if ( FStrEq( name, "teamplay_point_captured" ) ) {
		this->ClearMyControlPoint();
		
		if ( event->GetInt( "team" ) == this->GetTeamNumber() ) {
			this->OnTerritoryCaptured( event->GetInt( "cp" ) );
		} else {
			this->OnTerritoryLost( event->GetInt( "cp" ) );
			this->m_ctRecentPointLost.Start( RandomFloat( 10.0f, 20.0f ) );
		}
	} else if ( FStrEq( name, "teamplay_point_startcapture" ) ) {
		this->OnTerritoryContested( event->GetInt( "cp" ) );
	} else if ( FStrEq( name, "teamplay_flag_event" ) && ( ETFFlagEventTypes )event->GetInt( "eventtype" ) == TF_FLAGEVENT_PICKUP ) {
		if ( event->GetInt( "player" ) == this->entindex() ) {
			this->OnPickUp( nullptr, nullptr );
		}
	}
	else if ( FStrEq(name, "player_changeclass") ) 
	{
		if (tf_bot_reevaluate_class_in_spawnroom.GetBool())
		{
			// if on same class with the guy who switched ...
			int iClass = event->GetInt("class");
			if ( iClass == this->GetPlayerClass()->GetClassIndex() )
			{
				// ... and if alive and can change class ...
				if ( this->IsAlive() && TFGameRules()->CanBotChangeClass(this) )
				{
					// ... and inside a spawn room ...
					CTFNavArea* lkarea = this->GetLastKnownTFArea();
					if ( lkarea && lkarea->HasFriendlySpawnRoom(this) )
					{
						// ... and on the same team ...
						CBasePlayer* pPlayer = UTIL_PlayerByUserId( event->GetInt("userid") );
						if (pPlayer && pPlayer != this && pPlayer->GetTeamNumber() == this->GetTeamNumber())
						{
							// ... then switch class!
							if (FStrEq(tf_bot_force_class.GetString(), ""))
							{
								this->HandleCommand_JoinClass(this->GetNextSpawnClassname());
							}
							else
							{
								this->HandleCommand_JoinClass(tf_bot_force_class.GetString());
							}
						}
					}
				}
			}
		}
	}
}


void CTFBot::OnWeaponFired( CBaseCombatCharacter *who, CBaseCombatWeapon *weapon )
{
	VPROF_BUDGET( "CTFBot::OnWeaponFired", "NextBot" );
	
	NextBotPlayer<CTFPlayer>::OnWeaponFired( who, weapon );
	
	if ( who == nullptr )  return;
	if ( !who->IsAlive() ) return;
	
	if ( !this->IsRangeGreaterThan( who, tf_bot_notice_gunfire_range.GetFloat() ) ) {
		int chance = 100;
		
		if ( this->IsQuietWeapon( assert_cast<CTFWeaponBase *>( weapon ) ) ) {
			if ( this->IsRangeGreaterThan( who, tf_bot_notice_quiet_gunfire_range.GetFloat() ) ) {
				return;
			}
			
			switch ( this->GetSkill() ) {
			case EXPERT: chance = 90; break;
			case HARD:   chance = 60; break;
			case NORMAL: chance = 30; break;
			case EASY:   chance = 10; break;
			}
			
			/* if a non-quiet weapons has been heard lately, then the chance of
			 * hearing a quiet weapon is cut in half */
			if ( !this->m_ctLoudWeaponHeard.IsElapsed() ) {
				chance /= 2;
			}
		} else {
			if ( this->IsRangeLessThan( who, 1000.0f ) ) {
				this->m_ctLoudWeaponHeard.Start( 3.0f );
			}
		}
		
		if ( chance >= RandomInt( 1, 100 ) ) {
			this->GetVisionInterface()->AddKnownEntity( who );
		}
	}
}


bool CTFBot::IsEnemy( const CBaseEntity *ent ) const
{
	if ( ent == nullptr )    return false;
	if ( this->IsSelf( ent ) ) return false;
	
	return this->IsTeamEnemy( ent->GetTeamNumber() );
}

bool CTFBot::IsFriend( const CBaseEntity *ent ) const
{
	if ( ent == nullptr )    return false;
	if ( this->IsSelf( ent ) ) return true;
	
	return this->IsTeamFriend( ent->GetTeamNumber() );
}

bool CTFBot::IsDebugFilterMatch( const char *filter ) const
{
	if ( V_strnicmp( this->GetPlayerClass()->GetName(), filter, strlen( filter ) ) == 0 ) {
		return true;
	}
	
	return NextBotPlayer<CTFPlayer>::IsDebugFilterMatch( filter );
}

float CTFBot::GetDesiredPathLookAheadRange() const
{
	extern ConVar tf_bot_path_lookahead_range;
	return this->GetModelScale() * tf_bot_path_lookahead_range.GetFloat();
}


bool CTFBot::IsTeamEnemy( int team ) const
{
	return ( this->GetTeamNumber() != team );
}

bool CTFBot::IsTeamFriend( int team ) const
{
	return ( this->GetTeamNumber() == team );
}

//-----------------------------------------------------------------------------
// Purpose: Get most fitting enemy team
//-----------------------------------------------------------------------------
CTFTeam *CTFBot::GetOpposingTFTeam( void ) const
{
	int iTeam = GetTeamNumber();

	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		// Attempt to set diagonals by finding furthest enemy spawn
		// TFBots use this for incursion distance calcs
		if ( GetLastKnownArea() )
		{
			float flMaxDist = -1.0f;
			CTFTeam *maxDistTeam = NULL;
			CUtlVector<CTFNavArea *> enemySpawns;
			TheTFNavMesh->CollectEnemySpawnRoomAreas( &enemySpawns, iTeam );
			if ( !enemySpawns.IsEmpty() )
			{
				for ( auto enemySpawn : enemySpawns ) {
					float flDist = enemySpawn->GetIncursionDistance( iTeam );
					if ( flMaxDist < flDist )
					{
						flMaxDist = flDist;
						//NDebugOverlay::Line( GetAbsOrigin(), enemySpawn->GetCenter(), NB_RGB_WHITE, true, 0.001f * flMaxDist );
						switch ( enemySpawn->GetStaticTFAttributes() )
						{
						case CTFNavArea::RED_SPAWN_ROOM:
							maxDistTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
							break;
						case CTFNavArea::BLUE_SPAWN_ROOM:
							maxDistTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
							break;
						case CTFNavArea::GREEN_SPAWN_ROOM:
							maxDistTeam = TFTeamMgr()->GetTeam( TF_TEAM_GREEN );
							break;
						case CTFNavArea::YELLOW_SPAWN_ROOM:
							maxDistTeam = TFTeamMgr()->GetTeam( TF_TEAM_YELLOW );
							break;
						}
					}
				}

				if ( maxDistTeam != NULL )
				{
					return maxDistTeam;
				}
			}
		}

		// Fallback to common convention
		switch ( iTeam )
		{
		case TF_TEAM_RED:
			return TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
		case TF_TEAM_BLUE:
			return TFTeamMgr()->GetTeam( TF_TEAM_RED );
		case TF_TEAM_GREEN:
			return TFTeamMgr()->GetTeam( TF_TEAM_YELLOW );
		case TF_TEAM_YELLOW:
			return TFTeamMgr()->GetTeam( TF_TEAM_GREEN );
		}
	}

	return iTeam == TF_TEAM_RED ? TFTeamMgr()->GetTeam( TF_TEAM_BLUE ) : TFTeamMgr()->GetTeam( TF_TEAM_RED );
}


bool CTFBot::IsLineOfFireClear( const Vector& from, CBaseEntity *to ) const
{
	trace_t tr;
	UTIL_TraceLine( from, to->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, NextBotTraceFilterIgnoreActors(), &tr );
	
	if ( tr.DidHit() ) {
		return ( to == tr.m_pEnt );
	} else {
		return true;
	}
}

bool CTFBot::IsLineOfFireClear( const Vector& from, const Vector& to ) const
{
	trace_t tr;
	UTIL_TraceLine( from, to, MASK_SOLID_BRUSHONLY, NextBotTraceFilterIgnoreActors(), &tr );
	
	return !tr.DidHit();
}


bool CTFBot::HasTag( const char *str ) const
{
	for ( const auto& tag : this->m_Tags ) {
		if ( V_stricmp( str, tag ) == 0 ) return true;
	}
	
	return false;
}

void CTFBot::AddTag( const char *str )
{
	if ( !this->HasTag( str ) ) {
		this->m_Tags.AddToTail( str );
	}
}

void CTFBot::RemoveTag( const char *str )
{
	for ( int i = 0; i < this->m_Tags.Count(); ++i ) {
		if ( V_stricmp( str, this->m_Tags[i] ) == 0 ) {
			this->m_Tags.Remove( i );
			return;
		}
	}
}

void CTFBot::ClearTags()
{
	this->m_Tags.RemoveAll();
}


#define TFBOT_WEAPON_QUERY_WRAPPER( name ) \
	bool CTFBot::name( CTFWeaponBase *weapon ) const \
	{ \
		if ( weapon == nullptr ) { \
			weapon = this->GetActiveTFWeapon(); \
		} \
		 \
		if ( weapon == nullptr ) { \
			DevWarning( "CTFBot::" #name "( #%d ): active weapon is null!\n", this->entindex() ); \
			return false; \
		} \
		 \
		return weapon->TFBot_##name(); \
	}

TFBOT_WEAPON_QUERY_WRAPPER( IsCombatWeapon );
//TFBOT_WEAPON_QUERY_WRAPPER( IsHitScanWeapon );
TFBOT_WEAPON_QUERY_WRAPPER( IsExplosiveProjectileWeapon );
TFBOT_WEAPON_QUERY_WRAPPER( IsContinuousFireWeapon );
TFBOT_WEAPON_QUERY_WRAPPER( IsBarrageAndReloadWeapon );
TFBOT_WEAPON_QUERY_WRAPPER( IsQuietWeapon );
TFBOT_WEAPON_QUERY_WRAPPER( ShouldFireAtInvulnerableEnemies );
TFBOT_WEAPON_QUERY_WRAPPER( ShouldAimForHeadshots );
TFBOT_WEAPON_QUERY_WRAPPER( ShouldCompensateAimForGravity );
TFBOT_WEAPON_QUERY_WRAPPER( IsSniperRifle );
TFBOT_WEAPON_QUERY_WRAPPER( IsRocketLauncher );


float CTFBot::TransientlyConsistentRandomValue( float duration, int seed ) const
{
	CTFNavArea *area = this->GetLastKnownTFArea();
	if ( area == nullptr ) return 0.0f;
	
	return this->TransientlyConsistentRandomValue( area, duration, seed );
}

float CTFBot::TransientlyConsistentRandomValue( CNavArea *area, float duration, int seed ) const
{
	int time_seed = 1 + ( int )( gpGlobals->curtime / duration );
	seed += ( area->GetID() * time_seed * this->entindex() );
	
	return abs( FastCos( seed ) );
}


bool CTFBot::EquipRequiredWeapon()
{
	if ( this->m_RequiredWeapons.Count() != 0 ) {
		return this->Weapon_Switch( this->m_RequiredWeapons.Top() );
	}
	
	if ( ( this->m_nRestrict & MELEE_ONLY ) != 0 || TheTFBots().IsMeleeOnly() || TFGameRules()->IsInMedievalMode() ) {
		this->SwitchToMelee();
		return true;
	} else if ( ( this->m_nRestrict & PRIMARY_ONLY ) != 0 ) {
		this->SwitchToPrimary();
		return true;
	} else if ( ( this->m_nRestrict & SECONDARY_ONLY ) != 0 ) {
		this->SwitchToSecondary();
		return true;
	}
	
	return false;
}

bool CTFBot::EquipLongRangeWeapon()
{
	if ( TFGameRules()->IsMannVsMachineMode() ) return false;
	
	if ( ( this->IsPlayerClass( TF_CLASS_SOLDIER, true ) || this->IsPlayerClass( TF_CLASS_SNIPER, true ) ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ) {
		auto primary = this->GetTFWeapon_Primary();
		if ( primary != nullptr && this->GetAmmoCount( TF_AMMO_PRIMARY ) > 0 ) {
			this->Weapon_Switch( primary );
			return true;
		}
	}
	
	auto secondary = this->GetTFWeapon_Secondary();
	if ( secondary != nullptr && this->GetAmmoCount( TF_AMMO_SECONDARY ) > 0 ) {
		this->Weapon_Switch( secondary );
		return true;
	}
	
	return false;
}

void CTFBot::EquipBestWeaponForThreat( const CKnownEntity *threat )
{
	if ( this->EquipRequiredWeapon() ) return;
	
	CTFWeaponBase *primary = this->GetTFWeapon_Primary();
	if ( primary != nullptr && !this->IsCombatWeapon( primary ) ) primary = nullptr;
	
	CTFWeaponBase *secondary = this->GetTFWeapon_Secondary();
	if ( secondary != nullptr && !this->IsCombatWeapon( secondary ) ) secondary = nullptr;
	
	if ( TFGameRules()->IsMannVsMachineMode() ) {
		// TODO
		Assert( false );
	}
	
	CTFWeaponBase *melee = this->GetTFWeapon_Melee();
	if ( melee != nullptr && !this->IsCombatWeapon( melee ) ) melee = nullptr;
	
	CTFWeaponBase *weapon = primary;
	if ( weapon == nullptr ) weapon = secondary;
	if ( weapon == nullptr ) weapon = melee;
	
	if ( this->GetSkill() != EASY && threat != nullptr && threat->WasEverVisible() && threat->GetTimeSinceLastSeen() <= 5.0f ) {
		// TODO: check if this logic works properly for non-ammo-using weapons
		if ( this->GetAmmoCount( TF_AMMO_PRIMARY )   <= 0 ) primary   = nullptr;
		if ( this->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 ) secondary = nullptr;
		
		// TODO: rework this logic for DM
		// ( also: see similar logic in CTFBotRetreatToCover::Update )
		
		// TODO: check if the Clip1() logic works properly for non-clip-using weapons
		
		switch ( this->GetPlayerClass()->GetClassIndex() ) {
		
		// TODO: rework this
		case TF_CLASS_SCOUT:
		{
			if ( weapon != nullptr && weapon->Clip1() == 0 && secondary != nullptr ) {
				weapon = secondary;
			}
			
			break;
		}
		
		// TODO: rework this
		case TF_CLASS_SNIPER:
		{
			if ( secondary != nullptr && this->IsRangeLessThan( threat->GetLastKnownPosition(), 750.0f ) ) {
				weapon = secondary;
			}
			
			break;
		}
		
		// TODO: rework this
		case TF_CLASS_SOLDIER:
		{
			if ( weapon != nullptr && weapon->Clip1() == 0 && secondary != nullptr && secondary->Clip1() != 0 &&
				this->IsRangeLessThan( threat->GetLastKnownPosition(), 500.0f ) ) {
				weapon = secondary;
			}
			
			break;
		}

		// TODO: rework this
		case TF_CLASS_DEMOMAN:
		{
			if ( secondary != nullptr && secondary->Clip1() != 0 && this->IsRangeGreaterThan( threat->GetLastKnownPosition(), 512.0f ) ) {
				weapon = secondary;
			}

			if ( melee != nullptr && this->IsRangeLessThan( threat->GetLastKnownPosition(), 144.0f ) ) {
				weapon = melee;
			}

			break;
		}
		
		// TODO: rework this
		case TF_CLASS_PYRO:
		{
			if ( secondary != nullptr && this->IsRangeGreaterThan( threat->GetLastKnownPosition(), 400.0f ) ) {
				weapon = secondary;
			}
			
			CTFPlayer *enemy = ToTFPlayer( threat->GetEntity() );
			if ( enemy != nullptr && ( enemy->IsPlayerClass( TF_CLASS_SOLDIER, true ) || enemy->IsPlayerClass( TF_CLASS_DEMOMAN, true ) ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ) {
				weapon = primary;
			}
			
			break;
		}

		case TF_CLASS_HEAVYWEAPONS:
		{
			if ( secondary != nullptr && this->IsRangeGreaterThan( threat->GetLastKnownPosition(), 750.0f ) ) {
				weapon = secondary;
			}

			break;
		}

		case TF_CLASS_ENGINEER:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon != nullptr && ( pActiveWeapon->IsWeapon( TF_WEAPON_PDA ) || pActiveWeapon->IsWeapon( TF_WEAPON_PDA_ENGINEER_BUILD ) || pActiveWeapon->IsWeapon( TF_WEAPON_PDA_ENGINEER_DESTROY ) ) )
			{
				weapon = primary;
			}

			if ( weapon != nullptr && weapon->Clip1() == 0 ) {
				weapon = secondary;
			}

			if ( secondary != nullptr && this->IsRangeGreaterThan( threat->GetLastKnownPosition(), 750.0f ) ) {
				weapon = secondary;
			}

			break;
		}
		
		}
	}
	
	if ( weapon != nullptr ) {
		this->Weapon_Switch( weapon );
	}
}


bool CTFBot::LostControlPointRecently() const
{
	return ( this->m_ctRecentPointLost.HasStarted() && !this->m_ctRecentPointLost.IsElapsed() );
}


bool CTFBot::IsWeaponRestricted( CTFWeaponBase *weapon ) const
{
	Assert( weapon != nullptr );
	if ( weapon == nullptr ) return false;
	
	CEconItemDefinition *pItemDef = weapon->GetItem()->GetStaticData();
	if ( pItemDef == nullptr ) return false;
	
	ETFLoadoutSlot iSlot = pItemDef->GetLoadoutSlot( this->GetPlayerClass()->GetClassIndex() );
	if ( ( this->m_nRestrict &     MELEE_ONLY ) != 0 ) return ( iSlot != TF_LOADOUT_SLOT_MELEE );
	if ( ( this->m_nRestrict &   PRIMARY_ONLY ) != 0 ) return ( iSlot != TF_LOADOUT_SLOT_PRIMARY );
	if ( ( this->m_nRestrict & SECONDARY_ONLY ) != 0 ) return ( iSlot != TF_LOADOUT_SLOT_SECONDARY );
	
	// TODO: rework this function so it works more like this:
//	switch ( pItemDef->GetLoadoutSlot( this->GetPlayerClass()->GetClassIndex() ) ) {
//	case TF_LOADOUT_SLOT_PRIMARY: return ( this->m_nRestrict & PRIMARY_ALLOWED ) != 0;
//	...
//	...
//	}
	
	return false;
}


void CTFBot::PushAttribute( AttributeType attr, bool set )
{
	Assert( this->m_AttributeStack.Count() % 2 == 0 );
	
	AttributeType mask = attr;
	AttributeType val  = ( this->m_nAttributes & mask );
	
	/* remember which bits we modified and their original value */
	this->m_AttributeStack.Push( mask );
	this->m_AttributeStack.Push( val );
	
	/* set or clear the attribute( s ) as requested */
	if ( set ) {
		this->SetAttribute( mask );
	} else {
		this->ClearAttribute( mask );
	}
}

void CTFBot::PopAttribute( AttributeType attr )
{
	Assert( this->m_AttributeStack.Count() % 2 == 0 );
	
	AttributeType val;  this->m_AttributeStack.Pop( val );
	AttributeType mask; this->m_AttributeStack.Pop( mask );
	
	/* check: must be popping the same bits that were pushed */
	Assert( attr == mask );
	
	/* check: the saved value should only consist of bits in the mask */
	Assert( ( val & mask ) == val );
	val &= mask;
	
	/* reset the modified attribute( s ) to their former state */
	this->ClearAttribute( mask );
	this->SetAttribute( val );
}


void CTFBot::JoinSquad( CTFBotSquad *squad )
{
	Assert( this->m_Squad == nullptr );
	
	if ( squad != nullptr ) {
		squad->Join( this );
		this->m_Squad = squad;
	}
}

void CTFBot::LeaveSquad()
{
	if ( this->m_Squad != nullptr ) {
		this->m_Squad->Leave( this );
		this->m_Squad = nullptr;
	}
}

bool CTFBot::IsSquadmate( CTFPlayer *player ) const
{
	// TODO
	Assert( false );
	return false;
}

void CTFBot::DeleteSquad()
{
	if ( this->m_Squad != nullptr ) {
		this->m_Squad = nullptr;
	}
}


bool CTFBot::IsKnownSpy( CTFPlayer *spy ) const
{
	for ( CTFPlayer *known : this->m_KnownSpies ) {
		if ( known == nullptr ) continue;
		
		if ( spy->entindex() == known->entindex() ) {
			return true;
		}
	}
	
	return false;
}

void CTFBot::RealizeSpy( CTFPlayer *spy )
{
	if ( this->IsKnownSpy( spy ) ) return;
	
	if ( this->IsSuspectedSpy( spy ) ) {
		this->m_SuspectedSpies.FindAndFastRemove( spy );
	}
	
	this->m_KnownSpies.AddToHead( spy );
	
	TFGameRules()->VoiceCommand( this, 1, 1 );
}

void CTFBot::ForgetSpy( CTFPlayer *spy )
{
	this->m_SuspectedSpies.FindAndFastRemove( spy );
	this->m_KnownSpies    .FindAndFastRemove( spy );
}


bool CTFBot::IsSuspectedSpy( CTFPlayer *spy ) const
{
	for ( CTFPlayer *suspected : this->m_SuspectedSpies ) {
		if ( suspected == nullptr ) continue;
		
		if ( spy->entindex() == suspected->entindex() ) {
			return true;
		}
	}
	
	return false;
}

void CTFBot::SuspectSpy( CTFPlayer *spy )
{
	if ( this->IsSuspectedSpy( spy ) ) return;
	if ( this->IsKnownSpy    ( spy ) ) return;
	
	this->m_SuspectedSpies.AddToHead( spy );
}

void CTFBot::StopSuspectingSpy( CTFPlayer *spy )
{
	this->m_SuspectedSpies.FindAndFastRemove( spy );
}


void CTFBot::SetupSniperSpotAccumulation()
{
	VPROF_BUDGET( "CTFBot::SetupSniperSpotAccumulation", "NextBot" );
	
	CBaseEntity *goal = nullptr;
	
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) {
		CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToPush( this->GetTeamNumber() );
		if ( payload == nullptr ) {
			payload = TFGameRules()->GetPayloadToBlock( this->GetTeamNumber() );
		}
		
		if ( payload != nullptr ) {
			goal = payload->GetTrainEntity();
		}
	} else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP ) {
		goal = this->GetMyControlPoint();
	} else {
		// No particular goal found. For now, fall back to select any enemy
		// TODO: Properly support more modes than PL and CP.
		CUtlVector<CTFPlayer *> enemies;
		this->CollectEnemyPlayers( &enemies, false );
		enemies.Shuffle();

		if ( !enemies.IsEmpty() )
		{
			goal = enemies.Random();
		}
	}
	
	if ( goal == nullptr ) {
		this->ClearSniperSpots();
		return;
	}
	
	if ( goal == this->m_hSniperGoalEntity && this->m_vecSniperGoalEntity.DistToSqr( goal->WorldSpaceCenter() ) < Square( tf_bot_sniper_goal_entity_move_tolerance.GetFloat() ) ) {
		return;
	}
	
	this->ClearSniperSpots();
	
	int bot_teamnum   = this->GetTeamNumber();
	int enemy_teamnum = m_pOpposingTeam->GetTeamNumber();
	//int enemy_teamnum = ( bot_teamnum == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
	
	bool goal_is_enemy = false;
	CTFNavArea *goal_area = nullptr;
	
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) {
		goal_is_enemy = this->IsTeamEnemy( goal->GetTeamNumber() );
		goal_area = TheTFNavMesh->GetNearestTFNavArea( goal->WorldSpaceCenter(), true, 500.0f );
	} else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP ) {
		goal_is_enemy = this->IsTeamEnemy( this->GetMyControlPoint()->GetOwner() );
		goal_area = TheTFNavMesh->GetControlPointCenterArea( this->GetMyControlPoint()->GetPointIndex() );
	} else {
		// No particular goal found. For now, fall back to select any enemy
		// TODO: Properly support more modes than PL and CP.
		goal_is_enemy = this->IsTeamEnemy( goal->GetTeamNumber() );
		goal_area = TheTFNavMesh->GetNearestTFNavArea( goal->WorldSpaceCenter(), true, 500.0f );
	}
	
	this->m_SniperAreasFrom.RemoveAll();
	this->m_SniperAreasTo.RemoveAll();
	
	if ( goal_area == nullptr ) return;
	
	for ( auto area : TheTFNavAreas ) {
		float bot_incdist = area->GetIncursionDistance( bot_teamnum );
		if ( bot_incdist < 0.0f ) continue;
		
		float enemy_incdist = area->GetIncursionDistance( enemy_teamnum );
		if ( enemy_incdist < 0.0f ) continue;
		
		float tolerance = tf_bot_sniper_spot_point_tolerance.GetFloat();
		if ( !goal_is_enemy ) tolerance = -tolerance;
		
		if ( enemy_incdist <= goal_area->GetIncursionDistance( enemy_teamnum ) ) {
			this->m_SniperAreasTo.AddToTail( area );
		}
		
		// Sniper wants to stay safely behind the objective, not walk in front
		if ( bot_incdist < goal_area->GetIncursionDistance( bot_teamnum ) + tolerance ) {
			this->m_SniperAreasFrom.AddToTail( area );
		}

		if ( area->HasAnyTFAttributes( CTFNavArea::SNIPER_SPOT ) )
		{
			this->m_SniperAreasFrom.AddToTail( area );
		}
	}
	
	this->m_hSniperGoalEntity   = goal;
	this->m_vecSniperGoalEntity = goal->WorldSpaceCenter();
}

void CTFBot::AccumulateSniperSpots()
{
	VPROF_BUDGET( "CTFBot::AccumulateSniperSpots", "NextBot" );
	
	this->SetupSniperSpotAccumulation();
	
	if ( this->m_SniperAreasFrom.IsEmpty() || this->m_SniperAreasTo.IsEmpty() ) {
		if ( this->m_ctSniperSpots.IsElapsed() ) {
			this->ClearSniperSpots();
		}
		
		return;
	}

	int enemy_teamnum = m_pOpposingTeam->GetTeamNumber();
	
	for ( int i = tf_bot_sniper_spot_search_count.GetInt(); i > 0; --i ) {
		SniperSpotInfo spot;
		
		spot.from_area = this->m_SniperAreasFrom.Random();
		spot.from_vec  = spot.from_area->GetRandomPoint();

		// Make sure potential spot is not blocked
		trace_t trace;
		UTIL_TraceHull( spot.from_vec, spot.from_vec, this->GetPlayerMins(), this->GetPlayerMaxs(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( trace.DidHit() )
			continue;
		
		spot.to_area = this->m_SniperAreasTo.Random();
		spot.to_vec  = spot.to_area->GetRandomPoint();
		
		// Sightline too short
		spot.dist = spot.to_vec.DistTo( spot.from_vec );
		if ( spot.dist < tf_bot_sniper_spot_min_range.GetFloat() ) {
			continue;
		}
		
		if ( !this->IsLineOfFireClear( VecPlusZ( spot.from_vec, g_TFClassViewVectors[TF_CLASS_SNIPER].z ), VecPlusZ( spot.to_vec, g_TFClassViewVectors[TF_CLASS_SNIPER].z ) ) ) {
			continue;
		}
		
		//int bot_teamnum   = this->GetTeamNumber();
		//int enemy_teamnum = TF_TEAM_RED + ( bot_teamnum == TF_TEAM_RED );
		
		float from_incdist = spot.from_area->GetIncursionDistance( enemy_teamnum );
		float to_incdist   = spot.to_area  ->GetIncursionDistance( enemy_teamnum );
		
		// BUG: live TF2 doesn't validate the incursion distances
		if ( from_incdist == -1.0f || to_incdist == -1.0f ) {
			continue;
		}
		
		// Find sniper spot as far away from enemies as possible
		spot.delta_incdist = ( from_incdist - to_incdist );
		
		if ( this->m_SniperSpots.Count() < tf_bot_sniper_spot_max_count.GetInt() ) {
			this->m_SniperSpots.AddToTail( spot );
		} else if ( !this->m_SniperSpots.IsEmpty() ) {
			int worst_idx = -1;
			for ( int j = 0; j < this->m_SniperSpots.Count(); ++j ) {
				if ( j == 0 || this->m_SniperSpots[j].delta_incdist < this->m_SniperSpots[worst_idx].delta_incdist ) {
					worst_idx = j;
				}
			}
			
			if ( spot.delta_incdist > this->m_SniperSpots[worst_idx].delta_incdist ) {
				this->m_SniperSpots[worst_idx] = spot;
			}
		}
	}
	
	if ( this->IsDebugging( INextBot::DEBUG_BEHAVIOR ) ) {
		for ( const auto& spot : this->m_SniperSpots ) {
			switch ( this->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					NDebugOverlay::Cross3D( spot.from_vec, 8.0f, NB_RGB_RED, true, 3.1f );
					break;
				case TF_TEAM_BLUE:
					NDebugOverlay::Cross3D( spot.from_vec, 8.0f, NB_RGB_BLUE, true, 3.1f );
					break;
				case TF_TEAM_GREEN:
					NDebugOverlay::Cross3D( spot.from_vec, 8.0f, NB_RGB_GREEN, true, 3.1f );
					break;
				case TF_TEAM_YELLOW:
					NDebugOverlay::Cross3D( spot.from_vec, 8.0f, NB_RGB_YELLOW, true, 3.1f );
					break;
				default:
					NDebugOverlay::Cross3D( spot.from_vec, 8.0f, NB_RGB_MAGENTA, true, 3.1f );
					break;
			}
			NDebugOverlay::Line( spot.from_vec, spot.to_vec, NB_RGB_DKGREEN_C8, true, 3.1f );
		}
	}
}

void CTFBot::ClearSniperSpots()
{
	this->m_SniperSpots.RemoveAll();
	this->m_SniperAreasFrom.RemoveAll();
	this->m_SniperAreasTo.RemoveAll();
	
	this->m_hSniperGoalEntity = nullptr;
	
	this->m_ctSniperSpots.Start( RandomFloat( 5.0f, 10.0f ) );
}

const CTFBot::SniperSpotInfo *CTFBot::GetRandomSniperSpot() const
{
	if ( !this->m_SniperSpots.IsEmpty() ) {
		return &this->m_SniperSpots.Random();
	} else {
		return nullptr;
	}
}


bool CTFBot::IsAttentionFocusedOn( CBaseEntity *ent ) const
{
	CBaseEntity *focus = this->m_hAttentionFocus;
	
	if ( focus == nullptr ) return false;
	
	if ( focus->entindex() == ent->entindex() ) {
		return true;
	} else {
		auto action_point = dynamic_cast<const CTFBotActionPoint *>( focus );
		if ( action_point == nullptr ) return false;
		
		return action_point->IsWithinRange( ent );
	}
}


void CTFBot::SetMission( CTFBot::MissionType mission, bool reset_intention )
{
	this->m_iLastMission = this->m_iMission;
	this->m_iMission     = mission;
	
	if ( reset_intention ) {
		this->GetIntentionInterface()->Reset();
	}
	
	if ( mission != NO_MISSION ) {
		this->StartIdleSound();
	}
}


void CTFBot::DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay )
{
	/* update existing threat notice, if one exists */
	for ( auto& notice : this->m_DelayedThreatNotices ) {
		if ( notice.what == ent ) {
			notice.when = Min( notice.when, gpGlobals->curtime + delay );
			return;
		}
	}
	
	DelayedNoticeInfo notice = { ent, gpGlobals->curtime + delay };
	this->m_DelayedThreatNotices.AddToTail( notice );
}

void CTFBot::UpdateDelayedThreatNotices()
{
	FOR_EACH_VEC( this->m_DelayedThreatNotices, i ) {
		auto& notice = this->m_DelayedThreatNotices[i];
		if ( gpGlobals->curtime >= notice.when ) {
			CBaseEntity *ent = notice.what;
			if ( ent != nullptr ) {
				CTFPlayer *player = ToTFPlayer( ent );
				if ( player != nullptr && player->IsPlayerClass( TF_CLASS_SPY, true ) ) {
					this->RealizeSpy( player );
				}
				
				this->GetVisionInterface()->AddKnownEntity( ent );
			}
			
			this->m_DelayedThreatNotices.FastRemove( i );
		}
	}
}


CBasePlayer *CTFBot::AllocatePlayerEntity( edict_t *pEdict, const char *playername )
{
	CBasePlayer::s_PlayerEdict = pEdict;
	return assert_cast<CTFBot *>( CreateEntityByName( "tf_bot" ) );
}

bool CTFBot::ScriptIsWeaponRestricted( HSCRIPT weapon )
{
	return IsWeaponRestricted( HScriptToClass<CTFWeaponBase>( weapon ) );
}

int CTFBot::ScriptGetDifficulty()
{
	return m_iSkill;
}

void CTFBot::ScriptSetDifficulty( int difficulty )
{
	m_iSkill = (DifficultyType)difficulty;
}

bool CTFBot::ScriptIsDifficulty( int difficulty )
{
	return m_iSkill == difficulty;
}

HSCRIPT CTFBot::ScriptGetHomeArea()
{
	return m_HomeArea ? m_HomeArea->GetScriptInstance() : NULL;
}

void CTFBot::ScriptSetHomeArea( HSCRIPT area )
{
	m_HomeArea = HScriptToClass<CTFNavArea>( area );
}

void CTFBot::ScriptDelayedThreatNotice( HSCRIPT threat, float delay )
{
	DelayedThreatNotice( ToEnt( threat ), delay );
}

HSCRIPT CTFBot::ScriptGetNearestKnownSappableTarget()
{
	HSCRIPT hKnown = NULL;
	CBaseObject *pObject = GetNearestKnownSappableTarget();
	if ( pObject )
		hKnown = ToHScript( pObject );

	return hKnown;
}

void CTFBot::ScriptGenerateAndWearItem( char const *itemName )
{
}

HSCRIPT CTFBot::ScriptFindVantagePoint( float distance )
{
	return NULL;
}

void CTFBot::ScriptSetAttentionFocus( HSCRIPT entity )
{
	SetAttentionFocus( ToEnt( entity ) );
}

bool CTFBot::ScriptIsAttentionFocusedOn( HSCRIPT entity )
{
	return IsAttentionFocusedOn( ToEnt( entity ) );
}

HSCRIPT CTFBot::ScriptGetSpawnArea()
{
	return NULL;
}

void CTFBot::ScriptDisbandCurrentSquad()
{
	if ( m_Squad )
		m_Squad->DisbandAndDeleteSquad();
}


class CCountClassMembers
{
public:
	CCountClassMembers( const CTFBot *bot ) :
		m_pBot( bot ), m_iTeam( bot->GetTeamNumber() ) {}
	
	bool operator()( CBasePlayer *player )
	{
		if ( player->GetTeamNumber() == this->m_iTeam ) {
			++this->m_nTeamSize;
			
			if ( !this->m_pBot->IsSelf( player ) ) {
				++this->m_nClassMembers[ToTFPlayer( player )->GetDesiredPlayerClassIndex()];
			}
		}
		
		return true;
	}
	
private:
	const CTFBot *m_pBot;                         // +0x00
	int m_iTeam;                                  // +0x04
	
public:
	int m_nClassMembers[TF_CLASS_COUNT_ALL] = {}; // +0x08
	// 30 output uninitialized
	int m_nTeamSize = 0;                          // +0x34
};
// TODO: remove offsets when done RE'ing this

const char *CTFBot::GetNextSpawnClassname() const
{
	struct RosterEntry
	{
		int classidx;    // TF class index
		
		int minteamsize; // don't choose this class unless the team has at least this many members
		int classratio;  // choose 1 of this class per this many players
		int mincount;    // always choose this class if the team doesn't already have this many of it
		
		int skill[4];    // maximum on team, per difficulty level. -1 is infinite
	};
	
	const static RosterEntry offenseRoster[] = {
		{ TF_CLASS_SCOUT,         0,  0,  0, {  3,  3,  3,  3 } },
		{ TF_CLASS_SOLDIER,       0,  0,  0, { -1, -1, -1, -1 } },
		{ TF_CLASS_DEMOMAN,       0,  0,  0, {  2,  3,  3,  3 } },
		{ TF_CLASS_PYRO,          3,  0,  0, { -1, -1, -1, -1 } },
		{ TF_CLASS_HEAVYWEAPONS,  3,  0,  0, {  1,  1,  2,  2 } },
		{ TF_CLASS_MEDIC,         4,  4,  1, {  1,  1,  2,  2 } },
		{ TF_CLASS_SNIPER,        5,  0,  0, {  0,  1,  1,  1 } },
		{ TF_CLASS_SPY,           5,  0,  0, {  0,  1,  2,  2 } },
		{ TF_CLASS_ENGINEER,      5,  0,  0, {  1,  1,  1,  1 } },
		{ TF_CLASS_CIVILIAN,      5,  0,  0, {  1,  1,  1,  1 } },
		{ TF_CLASS_UNDEFINED,     0, -1,  0, {  0,  0,  0,  0 } },
	};
	
	const static RosterEntry defenseRoster[] = {
		{ TF_CLASS_ENGINEER,      0,  4,  1, {  1,  2,  3,  3 } },
		{ TF_CLASS_SOLDIER,       0,  0,  0, { -1, -1, -1, -1 } },
		{ TF_CLASS_DEMOMAN,       0,  0,  0, {  2,  3,  3,  3 } },
		{ TF_CLASS_PYRO,          3,  0,  0, { -1, -1, -1, -1 } },
		{ TF_CLASS_HEAVYWEAPONS,  3,  0,  0, {  1,  1,  2,  2 } },
		{ TF_CLASS_MEDIC,         4,  4,  1, {  1,  1,  2,  2 } },
		{ TF_CLASS_SNIPER,        5,  0,  0, {  0,  1,  1,  1 } },
		{ TF_CLASS_SPY,           5,  0,  0, {  0,  1,  2,  2 } },
		{ TF_CLASS_CIVILIAN,      5,  0,  0, {  1,  1,  1,  1 } },
		{ TF_CLASS_UNDEFINED,     0, -1,  0, {  0,  0,  0,  0 } },
	};
	
#if 0
	const static RosterEntry compRoster[] = {
		{ TF_CLASS_SCOUT,         0,  0,  0, {  0,  0,  2,  2 } },
		{ TF_CLASS_SOLDIER,       0,  0,  0, {  0,  0, -1, -1 } },
		{ TF_CLASS_DEMOMAN,       0,  0,  0, {  0,  0,  2,  2 } },
		{ TF_CLASS_PYRO,          0, -1,  0, {  0,  0,  0,  0 } },
		{ TF_CLASS_HEAVYWEAPONS,  3,  0,  0, {  0,  0,  2,  2 } },
		{ TF_CLASS_MEDIC,         1,  0,  1, {  0,  0,  1,  1 } },
		{ TF_CLASS_SNIPER,        0, -1,  0, {  0,  0,  0,  0 } },
		{ TF_CLASS_SPY,           0, -1,  0, {  0,  0,  0,  0 } },
		{ TF_CLASS_ENGINEER,      0, -1,  0, {  0,  0,  0,  0 } },
		{ TF_CLASS_UNDEFINED,     0, -1,  0, {  0,  0,  0,  0 } },
	};
#endif

	if ( TheTFBots().IsRandomizer() ) {
		return "random";
	}
	
	if ( this->IsPlayerClass( TF_CLASS_ENGINEER, true ) ) {
		if ( const_cast<CTFBot *>( this )->GetObjectOfType( OBJ_SENTRYGUN )                        != nullptr ||
			const_cast<CTFBot *>( this )->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT ) != nullptr ) {
			return "engineer";
		}
	}
	
	CCountClassMembers functor( this );
	ForEachPlayer( functor );
	
	const RosterEntry *roster = offenseRoster;
	
	if ( TFGameRules()->IsInKothMode() ) {
		CTeamControlPoint *point = this->GetMyControlPoint();
		if ( point != nullptr && this->GetTeamNumber() == TFObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) ) {
			roster = defenseRoster;
		}
	} else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP ) {
		CUtlVector<CTeamControlPoint *> points_capture;
		TFGameRules()->CollectCapturePoints( this, &points_capture );
		
		CUtlVector<CTeamControlPoint *> points_defend;
		TFGameRules()->CollectDefendPoints( this, &points_defend );
		
		if ( points_capture.IsEmpty() && !points_defend.IsEmpty() ) {
			roster = defenseRoster;
		}
	} else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) {
		CTeamTrainWatcher *payloadPush = TFGameRules()->GetPayloadToPush( this->GetTeamNumber() );
		CTeamTrainWatcher *payloadBlock = TFGameRules()->GetPayloadToBlock( this->GetTeamNumber() );

		switch ( this->GetTFTeam()->GetRole() )
		{
		case TEAM_ROLE_ATTACKERS:
			roster = offenseRoster;
			break;
		case TEAM_ROLE_DEFENDERS:
			roster = defenseRoster;
			break;

		default:
		case TEAM_ROLE_NONE:
			// fallback if Payload map doesn't have roles set
			if ( payloadPush != nullptr ) {
				roster = offenseRoster;
			}
			if ( payloadBlock != nullptr ) {
				roster = defenseRoster;
			}
			break;
		}

		if ( TFGameRules()->HasMultipleTrains() )
		{
			roster = offenseRoster;
		}
	}
	
	CUtlVector<int> candidates;
	CUtlVector<int> backup;
	
	for ( auto entry = roster; entry->classidx != TF_CLASS_UNDEFINED; ++entry ) {
		if ( !TFGameRules()->CanBotChooseClass( const_cast<CTFBot *>( this ), entry->classidx ) ) continue;
		
		backup.AddToTail( entry->classidx );
		
		if ( functor.m_nTeamSize < entry->minteamsize ) continue;
		
		int num_already_this_class = functor.m_nClassMembers[entry->classidx];
		if ( num_already_this_class < entry->mincount ) {
			candidates.RemoveAll();
			candidates.AddToTail( entry->classidx );
			break;
		}
		
		int skill = Clamp( this->GetSkill(), EASY, EXPERT );
		
		int skillnum = entry->skill[skill];
		if ( skillnum < 0 || num_already_this_class < skillnum ) {
			if ( entry->classratio > 0 && functor.m_nTeamSize / entry->classratio > num_already_this_class - entry->minteamsize ) {
				// nope
			} else {
				candidates.AddToTail( entry->classidx );
			}
		}
	}
	
	if ( candidates.IsEmpty() ) {
		if ( backup.IsEmpty() ) {
			Warning( "TFBot unable to choose a class, defaulting to 'auto'\n" );
			return "auto";
		}
		
		candidates.RemoveAll();
		candidates.AddVectorToTail( backup );
	}
	
	int classidx = candidates.Random();
	
	if ( this->m_hTargetSentry != nullptr ) {
		[&]{
			for ( int candidate : candidates ) {
				if ( candidate == TF_CLASS_DEMOMAN ) {
					classidx = TF_CLASS_DEMOMAN;
					return;
				}
			}
			
			for ( int candidate : candidates ) {
				if ( candidate == TF_CLASS_SPY ) {
					classidx = TF_CLASS_SPY;
					return;
				}
			}
			
			for ( int candidate : candidates ) {
				if ( candidate == TF_CLASS_SOLDIER ) {
					classidx = TF_CLASS_SOLDIER;
					return;
				}
			}
		}();
	}
	
	const char *classname = GetPlayerClassData( classidx )->m_szClassName;
	if ( classname == nullptr ) {
		Warning( "TFBot unable to get data for desired class, defaulting to 'auto'\n" );
		classname = "auto";
	}
	
	return classname;
}


float CTFBot::GetDesiredAttackRange() const
{
	CTFWeaponBase *weapon = this->GetActiveTFWeapon();
	if ( weapon == nullptr ) return 0.0f;
	
	// TODO: update this for new weapon IDs
	
	/* order matters */
	if ( weapon->IsWeapon( TF_WEAPON_KNIFE ) )          return   70.0f;
	if ( weapon->IsMeleeWeapon() )                      return  50.0f;
	if ( weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )   return  100.0f;
	if ( weapon->IsMinigun() )							return 500.0f;
	if ( this->IsSniperRifle( weapon ) )                return FLT_MAX;
	if ( weapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) ) return ( TFGameRules()->IsMannVsMachineMode() ? 500.0f : 1250.0f );
	
	return 500.0f;
}

float CTFBot::GetMaxAttackRange() const
{
	CTFWeaponBase *weapon = this->GetActiveTFWeapon();
	if ( weapon == nullptr ) return 0.0f;

	// TODO: update this for new weapon IDs
	if ( weapon->IsMeleeWeapon() )
	{
		CTFWeaponBaseMelee *pMelee = dynamic_cast<CTFWeaponBaseMelee *>( weapon );
		if ( pMelee )
			return pMelee->GetSwingRange();
	}
	if ( weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )   return ( 250.0f );
	if ( weapon->IsMinigun() )							return ( m_Shared.IsCritBoosted() ? 1500.0f : 750.0f );
	if ( this->IsSniperRifle( weapon ) )                return FLT_MAX;
	if ( weapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) ) return 3000.0f;
	
	return FLT_MAX;
}


bool CTFBot::IsAmmoFull() const
{
	// TODO: perhaps we should take into consideration TF_AMMO_GRENADES1?
	
	if ( this->GetAmmoCount( TF_AMMO_PRIMARY ) < this->GetMaxAmmo( TF_AMMO_PRIMARY ) ) return false;
	if ( this->GetAmmoCount( TF_AMMO_SECONDARY ) < this->GetMaxAmmo( TF_AMMO_SECONDARY ) ) return false;
	
	if ( this->IsPlayerClass( TF_CLASS_ENGINEER ), true ) {
		if ( this->GetAmmoCount( TF_AMMO_METAL ) < this->GetMaxAmmo( TF_AMMO_METAL ) ) return false;
	}

	if ( this->IsPlayerClass( TF_CLASS_SPY ), true ) {
		if ( this->m_Shared.GetSpyCloakMeter() < 100.0f ) return false;
	}
	
	return true;
}

bool CTFBot::IsAmmoLow() const
{
	CTFWeaponBase *weapon = this->GetActiveTFWeapon();
	if ( weapon == nullptr ) return false;
	
	if ( weapon->IsWeapon( TF_WEAPON_WRENCH ) ) {
		return ( this->GetAmmoCount( TF_AMMO_METAL ) <= 0 );
	}
	
	// TODO: take into consideration that some melee weapons will have special functions that *do* have ammo requirements
	if ( weapon->IsMeleeWeapon() ) return false;
	
	// TODO: figure out wtf this is supposed to be here for
	if ( weapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE ) return false;
	
	// TODO: take into account slots other than just primary
	float flAmmo    = ( float )this->GetAmmoCount( TF_AMMO_PRIMARY );
	float flMaxAmmo = ( float )this->GetMaxAmmo  ( TF_AMMO_PRIMARY );
	
	/* avoid divide-by-zero situations */
	if ( flMaxAmmo == 0.0f ) return false;
	
	return ( ( flAmmo / flMaxAmmo ) < 0.2f );
}


int CTFBot::GetUberHealthThreshold() const
{
	int iThreshold = 0;
	CALL_ATTRIB_HOOK_INT( iThreshold, bot_medic_uber_health_threshold );
	// TODO: add to items_game.txt
	
	if ( iThreshold <= 0 ) {
		return 50;
	} else {
		return iThreshold;
	}
}

float CTFBot::GetUberDeployDelayDuration() const
{
	int iDelay = 0;
	CALL_ATTRIB_HOOK_INT( iDelay, bot_medic_uber_deploy_delay_duration );
	// TODO: why the hell is this an integer attribute?
	// TODO: add to items_game.txt
	
	if ( iDelay == 0 ) {
		return -1.0f;
	} else {
		return ( float )iDelay;
	}
}


void CTFBot::UpdateLookingAroundForEnemies()
{
	if ( !this->m_bLookAroundForEnemies )     return;
	if ( this->HasAttribute( IGNORE_ENEMIES ) ) return;
	if ( this->m_Shared.IsControlStunned() )  return;
	
	const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat == nullptr ) {
		this->UpdateLookingAroundForIncomingPlayers( true );
		return;
	}
	
	if ( threat->IsVisibleInFOVNow() ) {
		if ( this->IsPlayerClass( TF_CLASS_SPY, true ) && this->GetSkill() >= HARD && this->m_Shared.InCond( TF_COND_DISGUISED ) && !this->m_Shared.IsStealthed() ) {
			this->UpdateLookingAroundForIncomingPlayers( false );
		} else {
			this->GetBodyInterface()->AimHeadTowards( threat->GetEntity(), IBody::PRI_CRITICAL, 1.0f, nullptr, "Aiming at a visible threat" );
		}
		
		return;
	}
	
	if ( this->IsLineOfSightClear( threat->GetEntity(), CBaseCombatCharacter::IGNORE_ACTORS ) ) {
		/* 0.5f: sin( 30 ); 30-degree cone in a sense */
		float variance = 0.5f * this->GetAbsOrigin().DistTo( threat->GetEntity()->GetAbsOrigin() );
		
		Vector aim = threat->GetEntity()->WorldSpaceCenter();
		
		aim.x += RandomFloat( -variance, variance );
		aim.y += RandomFloat( -variance, variance );
		
		this->GetBodyInterface()->AimHeadTowards( aim, IBody::PRI_IMPORTANT, 1.0f, nullptr, "Turning around to find threat out of our FOV" );
		
		return;
	}
	
	if ( this->IsPlayerClass( TF_CLASS_SNIPER, true ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ) {
		this->UpdateLookingAroundForIncomingPlayers( true );
		return;
	}
	
	CTFNavArea *lkarea = this->GetLastKnownTFArea();
	if ( lkarea == nullptr ) return;
	
	Vector threat_lkpos = threat->GetLastKnownPosition();
	
	CTFNavArea *best_area = nullptr;
	float best_distsqr = FLT_MAX;
	
	lkarea->ForAllPotentiallyVisibleTFAreas( [&]( CTFNavArea *area ){
		Vector close;
		area->GetClosestPointOnArea( threat_lkpos, &close );
		
		float distsqr = threat_lkpos.DistToSqr( close );
		if ( distsqr < best_distsqr ) {
			best_distsqr = distsqr;
			best_area    = area;
		}
		
		return true;
	} );
	
	if ( best_area != nullptr ) {
		for ( int tries = 10; tries != 0; --tries ) {
			// TODO: magic number: 53.25f
			Vector point = VecPlusZ( best_area->GetRandomPoint(), 53.25f );
			
			if ( this->GetVisionInterface()->IsLineOfSightClear( point ) ) {
				this->GetBodyInterface()->AimHeadTowards( point, IBody::PRI_IMPORTANT, 1.0f, nullptr, "Looking toward potentially visible area near known but hidden threat" );
				return;
			}
			
			if ( this->IsDebugging( INextBot::DEBUG_VISION | INextBot::DEBUG_ERRORS ) ) {
				ConColorMsg( NB_COLOR_YELLOW, "%3.2f: %s can't find clear line to look at potentially visible near known but hidden entity %s( #%d )\n",
					gpGlobals->curtime, this->GetDebugIdentifier(), threat->GetEntity()->GetClassname(), threat->GetEntity()->entindex() );
			}
		}
	} else {
		if ( this->IsDebugging( INextBot::DEBUG_VISION | INextBot::DEBUG_ERRORS ) ) {
			ConColorMsg( NB_COLOR_YELLOW, "%3.2f: %s no potentially visible area to look toward known but hidden entity %s( #%d )\n",
				gpGlobals->curtime, this->GetDebugIdentifier(), threat->GetEntity()->GetClassname(), threat->GetEntity()->entindex() );
		}
	}
}

void CTFBot::UpdateLookingAroundForIncomingPlayers( bool friendly_areas )
{
	if ( !this->m_ctLookAroundForIncomingPlayers.IsElapsed() ) return;
	this->m_ctLookAroundForIncomingPlayers.Start( RandomFloat( 1.0f, 3.0f ) );
	
	float range = ( this->m_Shared.InCond( TF_COND_ZOOMED ) ? 750.0f : 150.0f );
	
	auto lkarea = this->GetLastKnownTFArea();
	if ( lkarea == nullptr ) return;
	
	CUtlVector<CTFNavArea *> invasion_areas;
	
	if ( friendly_areas ) {
		invasion_areas.AddVectorToTail( lkarea->GetInvasionAreas( this->GetTeamNumber() ) );
	} else {
		this->ForEachEnemyTeam( [&]( int team ){
			invasion_areas.AddVectorToTail( lkarea->GetInvasionAreas( team ) );
			return true;
		} );
	}
	
	if ( invasion_areas.IsEmpty() ) return;
	
	for ( int tries = 20; tries != 0; --tries ) {
		// TODO: magic number: 53.25f
		Vector point = VecPlusZ( invasion_areas.Random()->GetRandomPoint(), 53.25f );
		if ( this->IsRangeGreaterThan( point, range ) && this->GetVisionInterface()->IsLineOfSightClear( point ) ) {
			this->GetBodyInterface()->AimHeadTowards( point, IBody::PRI_INTERESTING, 0.5f, nullptr, "Looking toward enemy invasion areas" );
			return;
		}
	}
}


void CTFBot::DisguiseAsMemberOfEnemyTeam()
{
	CUtlVector<CTFPlayer *> enemies;
	this->CollectEnemyPlayers( &enemies, false );
	enemies.Shuffle();
	
	for ( auto enemy : enemies ) {
		int teamnum = TFGameRules()->IsFourTeamGame() ? TF_TEAM_GLOBAL : enemy->GetTeamNumber();
		if ( teamnum < FIRST_GAME_TEAM || teamnum >= GetNumberOfTeams() ) continue;
		
		int classidx = enemy->GetPlayerClass()->GetClassIndex();
		if ( classidx < TF_FIRST_NORMAL_CLASS || classidx > TF_LAST_NORMAL_CLASS ) continue;
		
		this->m_Shared.Disguise( teamnum, classidx );
		return;
	}
	
	/* fallback in case there are no valid enemy players */
	this->DisguiseAsRandomClass();
}

void CTFBot::DisguiseAsRandomClass()
{
	// NEW ( from CTFBotMainAction::Update and CTFBotTacticalMonitor::OnCommandString )
	
	CUtlVector<int> enemy_teams;
	this->ForEachEnemyTeam( [&]( int team ){
		enemy_teams.AddToTail( team );
		return true;
	} );
	
	int teamnum;
	int classidx;
	
	if( TFGameRules()->IsFourTeamGame() ) {
		teamnum = TF_TEAM_GLOBAL;
	}
	else {
		Assert(!enemy_teams.IsEmpty());
		if (!enemy_teams.IsEmpty()) {
			teamnum = enemy_teams.Random();
		}
		else {
			/* this should never happen */
			teamnum = this->GetTeamNumber();
		}
	}
	
	classidx = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
	
	this->m_Shared.Disguise( teamnum, classidx );
}


void CTFBot::ClearMyControlPoint()
{
	this->m_hMyControlPoint = nullptr;
	this->m_ctMyControlPoint.Invalidate();
}

CTeamControlPoint *CTFBot::GetMyControlPoint() const
{
	if ( this->m_hMyControlPoint == nullptr || this->m_ctMyControlPoint.IsElapsed() ) {
		this->m_ctMyControlPoint.Start( RandomFloat( 1.0f, 2.0f ) );
		
		CUtlVector<CTeamControlPoint *> points_capture;
		TFGameRules()->CollectCapturePoints( this, &points_capture );
		
		CUtlVector<CTeamControlPoint *> points_defend;
		TFGameRules()->CollectDefendPoints( this, &points_defend );
		
		if ( !points_defend.IsEmpty() && ( this->IsPlayerClass( TF_CLASS_ENGINEER, true ) || this->IsPlayerClass( TF_CLASS_SNIPER, true ) || this->HasAttribute( DEFEND_CLOSEST_POINT ) ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ) {
			this->m_hMyControlPoint = this->SelectPointToDefend( &points_defend );
		} else {
			this->m_hMyControlPoint = this->SelectPointToCapture( &points_capture );
			if ( this->m_hMyControlPoint == nullptr ) {
				this->m_hMyControlPoint = this->SelectPointToDefend( &points_defend );
			}
		}
	}
	
	return this->m_hMyControlPoint;
}

float CTFBot::GetTimeLeftToCapture() const
{
	// Unlike live TF2, we return FLT_MAX if there's no active round timer, to
	// indicate that there's infinite time left; returning 0.0f gives AI actions
	// the impression that there is always little-to-no time remaining
	
	// Also, for 4-team purposes, we use the timer of the enemy team with the
	// least time remaining
	
	if ( TFGameRules()->IsInKothMode() ) {
		float t_remaining = FLT_MAX;
		this->ForEachEnemyTeam( [&]( int team ){
			CTeamRoundTimer *timer = TFGameRules()->GetKothTimer( team );
			if ( timer != nullptr ) {
				t_remaining = Min( t_remaining, timer->GetTimeRemaining() );
			}
			
			return true;
		} );
		
		return t_remaining;
	}

	// in Domination, time remaining is predicted based on:
	// How many points does an enemy team need to acquire, and
	// how quickly does it get them currently?

	if ( TFGameRules()->IsInDominationMode() ) {
		float t_remaining = FLT_MAX;
		int scoreLeft = INT_MAX;
		this->ForEachEnemyTeam( [&]( int team ){
			CTFTeam *pTeam = GetGlobalTFTeam( team );

			if ( pTeam != nullptr ) {
				scoreLeft = TFGameRules()->GetPointLimit() - pTeam->GetRoundScore();
				if ( pTeam->GetDominationPointRate() > 0 )
				{
					float timeLeft = scoreLeft * ( 5.0f / pTeam->GetDominationPointRate() );
					t_remaining = Min( t_remaining, timeLeft );
				}
			}

			return true;
		} );

		return t_remaining;
	}
	
	CTeamRoundTimer *timer = TFGameRules()->GetActiveRoundTimer();
	if ( timer != nullptr ) {
		return timer->GetTimeRemaining();
	} else {
		return FLT_MAX;
	}
}

bool CTFBot::IsAnyPointBeingCaptured() const
{
	if ( g_hControlPointMasters.IsEmpty() ) return false;
	
	CTeamControlPointMaster *master = g_hControlPointMasters[0];
	if ( master == nullptr ) return false;
	
	for ( int i = 0; i < master->GetNumPoints(); ++i ) {
		if ( this->IsPointBeingCaptured( master->GetControlPoint( i ) ) ) {
			return true;
		}
	}
	
	return false;
}

bool CTFBot::IsNearPoint( CTeamControlPoint *point ) const
{
	if ( point == nullptr ) return false;
	
	auto lkarea = this->GetLastKnownTFArea();
	if ( lkarea == nullptr ) return false;
	
	CTFNavArea *point_center_area = TheTFNavMesh->GetControlPointCenterArea( point->GetPointIndex() );
	if ( point_center_area == nullptr ) return false;
	
	float    my_incdist = lkarea           ->GetIncursionDistance( this->GetTeamNumber() );
	float point_incdist = point_center_area->GetIncursionDistance( this->GetTeamNumber() );
	
	return ( abs( my_incdist - point_incdist ) < tf_bot_near_point_travel_distance.GetFloat() );
}

bool CTFBot::IsPointBeingCaptured( CTeamControlPoint *point ) const
{
	if ( point == nullptr ) return false;
	
	return ( point->HasBeenContested() && ( gpGlobals->curtime - point->LastContestedAt() ) < 5.0f );
}

CTeamControlPoint *CTFBot::SelectPointToCapture( const CUtlVector<CTeamControlPoint *> *points ) const
{
	if ( points == nullptr ) return nullptr;
	if ( points->IsEmpty() ) return nullptr;
	
	if ( points->Count() == 1 ) return points->Head();
	
	if ( const_cast<CTFBot *>( this )->IsCapturingPoint() ) {
		CTriggerAreaCapture *trigger = const_cast<CTFBot *>( this )->GetControlPointStandingOn();
		if ( trigger != nullptr ) {
			return trigger->GetControlPoint();
		}
	}

	CTeamControlPoint *closest_point = this->SelectClosestControlPointByTravelDistance( points );
	if ( closest_point != nullptr ) {
		if ( this->IsPointBeingCaptured( closest_point ) || this->IsNearPoint( closest_point ) )
		{
			return closest_point;
		}
		if ( TFGameRules()->IsInDominationMode() && closest_point->GetOwner() != this->GetTeamNumber() )
		{
			return closest_point;
		}
	}

	for ( auto point : *points ) {
		if ( point->GetOwner() == TEAM_UNASSIGNED ) {
			return point;
		}
	}
	
	bool point_is_safe = true;
	
	CTeamControlPoint *best_point = nullptr;
	float best_intensity = FLT_MAX;
	
	for ( auto point : *points ) {
		CTFNavArea *point_center_area = TheTFNavMesh->GetControlPointCenterArea( point->GetPointIndex() );
		if ( point_center_area == nullptr ) continue;
		
		float intensity = point_center_area->GetCombatIntensity();
		if ( intensity > 0.1f ) {
			point_is_safe = false;

			if ( intensity < best_intensity ) {
				best_intensity = intensity;
				best_point = point;
			}
		}
		else {
			return point;
		}
	}
	
	if ( !point_is_safe ) {
		return best_point;
	}
	
	// TODO: make sure this is both fair and safe
	int idx = Clamp( ( int )( points->Count() * this->TransientlyConsistentRandomValue( 60.0f ) ), 0, points->Count() - 1 );
	return ( *points )[idx];
}

CTeamControlPoint *CTFBot::SelectPointToDefend( const CUtlVector<CTeamControlPoint *> *points ) const
{
	if ( points == nullptr ) return nullptr;
	if ( points->IsEmpty() ) return nullptr;
	
	if ( points->Count() == 1 ) return points->Head();

	CTeamControlPoint *closest_point = this->SelectClosestControlPointByTravelDistance( points );
	if ( closest_point != nullptr ) {
		if ( this->IsPointBeingCaptured( closest_point ) )
		{
			return closest_point;
		}
	}

	for ( auto point : *points ) {
		if ( this->IsPointBeingCaptured( point ) ) {
			return point;
		}
	}
	
	if ( this->HasAttribute( DEFEND_CLOSEST_POINT ) ) {
		return this->SelectClosestControlPointByTravelDistance( points );
	} else {
		// in Attack/Defend modes, prioritize the lowest index.
		// good for maps like cp_steel
		if ( this->GetTFTeam()->GetRole() == TEAM_ROLE_DEFENDERS ) {
			return points->Head();
		}
		return points->Random();
	}
}

CTeamControlPoint *CTFBot::SelectClosestControlPointByTravelDistance( const CUtlVector<CTeamControlPoint *> *points ) const
{
	if ( points == nullptr ) return nullptr;
	if ( points->IsEmpty() ) return nullptr;
	
	if ( this->GetLastKnownTFArea() == nullptr ) return nullptr;
	
	CTeamControlPoint *best_point = nullptr;
	float best_dist = FLT_MAX;
	
	for ( auto point : *points ) {
		float dist = NavAreaTravelDistance( this->GetLastKnownTFArea(), TheTFNavMesh->GetControlPointCenterArea( point->GetPointIndex() ), CTFBotPathCost( this, FASTEST_ROUTE ), 0.0f, this->GetTeamNumber() );
		if ( dist < 0.0f ) continue;
		
		if ( dist < best_dist ) {
			best_dist  = dist;
			best_point = point;
		}
	}
	
	return best_point;
}


CCaptureZone *CTFBot::GetFlagCaptureZone() const
{
	for ( auto zone : CCaptureZone::AutoList() ) {
		if ( zone != nullptr && this->GetTeamNumber() == zone->GetTeamNumber() ) {
			return zone;

			// If no zone exists for us, try to find a neutral one.
			if ( zone->GetTeamNumber() == TEAM_UNASSIGNED )
			{
				return zone;
			}
		}
	}
	
	return nullptr;
}

CVIPSafetyZone *CTFBot::GetVIPEscapeZone() const
{
	for ( auto zone : CVIPSafetyZone::AutoList() ) {
		if ( zone != nullptr ) {
			return zone;
		}
	}

	return nullptr;
}

CFlagDetectionZone* CTFBot::GetFlagDetectionZone() const
{
	for ( auto zone : CFlagDetectionZone::AutoList() ) {
		if ( zone != nullptr ) {
			return zone;
		}
	}

	return nullptr;
}

CCaptureFlag *CTFBot::GetFlagToFetch() const
{
	if ( TFGameRules()->IsMannVsMachineMode() ) {
		if ( this->GetTeamNumber() == TF_TEAM_BLUE && this->IsPlayerClass( TF_CLASS_ENGINEER, true ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) ) {
			return nullptr;
		}
		
		if ( this->HasAttribute( IGNORE_FLAG ) ) {
			return nullptr;
		}
		
		if ( this->m_hFlagTarget != nullptr ) {
			return this->m_hFlagTarget;
		}
	}
	
	/* these checks aren't in live TF2 but they seemed like a good idea */
	if ( TFGameRules()->IsInWaitingForPlayers() ) return nullptr;
	if ( !TFGameRules()->FlagsMayBeCapped() )     return nullptr;
	if ( !this->IsAllowedToPickUpFlag() )         return nullptr;
	
	CUtlVector<CCaptureFlag *> flags;
	int num_stolen = 0;
	
	for ( auto flag : CCaptureFlag::AutoList() ) {
		if ( flag->IsDisabled() ) continue;

		if ( !flag->TeamCanTouchThisFlag( this ) ) continue;
		
		if ( this->HasTheFlag() && flag->GetOwnerEntity() == this ) {
			return flag;
		}
		
		if ( !flag->CanTouchThisFlagType( this ) ) {
			continue;
		}
		
		flags.AddToTail( flag );
		
		if ( flag->IsStolen() ) {
			++num_stolen;
		}
	}
	
	if ( TFGameRules()->IsMannVsMachineMode() ) {
		// TODO: MvM stuff
		Assert( false );
		return nullptr;
		
		// incidentally, live TF2 has what I can only assume _must_ be a bug in
		// here, which is that they compare flags' m_flAutoCapTime against a
		// float that's initialized to NaN ( 0x7fffffff ); surely they meant for
		// that float to be initialized to FLT_MAX instead...
		
		// ^^^^^^^
		// I think I'm an idiot; the actual comparison instruction is an integer
		// comparison, and I'm skeptical that it's *actually* comparing against
		// *reinterpret_cast<int *>( some_flag->m_flAutoCapTime ) and not some
		// other thing that's just been misidentified
	} else {
		float best_distsqr = FLT_MAX;
		CCaptureFlag *best_flag = nullptr;
		
		float best_distsqr_nonstolen = FLT_MAX;
		CCaptureFlag *best_flag_nonstolen = nullptr;
		
		for ( auto flag : flags ) {
			if ( flag == nullptr ) continue;
			
			float distsqr = this->GetAbsOrigin().DistToSqr( flag->GetAbsOrigin() );
			
			if ( distsqr < best_distsqr ) {
				best_flag = flag;
				best_distsqr = distsqr;
			}
			
			if ( num_stolen < flags.Count() ) {
				if ( !flag->IsStolen() && distsqr < best_distsqr_nonstolen ) {
					best_flag_nonstolen = flag;
					best_distsqr_nonstolen = distsqr;
				}
			}
		}
		
		if ( best_flag_nonstolen != nullptr ) {
			return best_flag_nonstolen;
		} else {
			return best_flag;
		}
	}
}

bool CTFBot::IsAllowedToPickUpFlag() const
{
	if ( IsPlayerClass( TF_CLASS_ENGINEER, true ) && !GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) )
		return false;

	if ( !NextBotPlayer<CTFPlayer>::IsAllowedToPickUpFlag() ) {
		return false;
	}
	
	if ( this->IsInASquad() ) {
		// TODO
		Assert( false );
	}
	
	return !this->IsOnAnyMission();
}

void CTFBot::SetFlagTarget( CCaptureFlag *flag )
{
	if ( flag == this->m_hFlagTarget ) return;
	
	this->m_hFlagTarget->RemoveFollower( this );
	
	this->m_hFlagTarget = flag;
	if ( flag != nullptr ) {
		flag->AddFollower( this );
	}
}


float CTFBot::GetThreatDanger( CBaseCombatCharacter *threat ) const
{
	if ( threat == nullptr ) return 0.0f;
	
	if ( this->IsPlayerClass( TF_CLASS_SNIPER ) && this->IsRangeGreaterThan( threat, tf_bot_sniper_personal_space_range.GetFloat() ) ) {
		return 0.0f;
	}
	
	CTFPlayer *player = ToTFPlayer( threat );
	if ( player != nullptr ) {
		if ( player->m_Shared.IsInvulnerable() ) return 1.0f;
		
		// TODO: perhaps make these into convars so they can be tweaked at runtime
		switch ( player->GetPlayerClass()->GetClassIndex() ) {
		case TF_CLASS_SCOUT:        return 0.6f;
		case TF_CLASS_SNIPER:       return 0.4f;
		case TF_CLASS_SOLDIER:      return 0.8f;
		case TF_CLASS_DEMOMAN:      return 0.6f;
		case TF_CLASS_MEDIC:        return 0.2f;
		case TF_CLASS_HEAVYWEAPONS: return 0.8f;
		case TF_CLASS_PYRO:         return 1.0f;
		case TF_CLASS_SPY:          return 0.5f;
		case TF_CLASS_ENGINEER:     return 0.5f;
		
		case TF_CLASS_CIVILIAN:     return 0.4f; // TODO: fine-tune this
		
		default: Assert( false ); return 0.0f;
		}
	}
	
	auto sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry != nullptr && sentry->IsAlive() && !sentry->IsPlacing() && !sentry->HasSapper() && !sentry->IsUpgrading() && !sentry->IsBuilding() ) {
		int level = sentry->GetUpgradeLevel();
		Assert( level >= 1 );
		Assert( level <= 3 );
		
		switch ( level ) {
		case 1: return 0.6f;
		case 2: return 0.8f;
		case 3: return 1.0f;
		}
	}
	
	return 0.0f;
}


CTFPlayer *CTFBot::GetClosestHumanLookingAtMe( int team ) const
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, team, true );
	
	CTFPlayer *best_human = nullptr;
	float best_distsqr = FLT_MAX;
	
	for ( auto player : players ) {
		if ( player->IsBot() ) continue;
		
		Vector delta = ( const_cast<CTFBot *>( this )->EyePosition() - player->EyePosition() );
		float distsqr = VectorNormalize( delta );
		
		if ( distsqr < best_distsqr && delta.Dot( EyeVectorsFwd( player ) ) > 0.98f &&
			this->IsLineOfSightClear( player->EyePosition(), CBaseCombatCharacter::IGNORE_NOTHING, player ) ) {
			best_human = player;
			best_distsqr = distsqr;
		}
	}
	
	return best_human;
}

CBaseObject *CTFBot::GetNearestKnownSappableTarget() const
{
	CUtlVector<CKnownEntity> knowns;
	this->GetVisionInterface()->CollectKnownEntities( &knowns );
	
	float best_distsqr = Square( 500.0f );
	CBaseObject *best_obj = nullptr;
	
	for ( const auto& known : knowns ) {
		auto obj = dynamic_cast<CBaseObject *>( known.GetEntity() );
		if ( obj == nullptr ) continue;
		
		if ( obj->HasSapper() )    continue;
		if ( !this->IsEnemy( obj ) ) continue;
		
		float distsqr = this->GetRangeSquaredTo( obj );
		if ( distsqr < best_distsqr ) {
			best_distsqr = distsqr;
			best_obj     = obj;
		}
	}
	
	return best_obj;
}

bool CTFBot::IsEntityBetweenTargetAndSelf( CBaseEntity *ent, CBaseEntity *target ) const
{
	Assert( ent    != nullptr );
	Assert( target != nullptr );
	
	Vector delta_target = ( target->GetAbsOrigin() - this->GetAbsOrigin() );
	Vector delta_ent    = ( ent   ->GetAbsOrigin() - this->GetAbsOrigin() );
	
	float dist_target = VectorNormalize( delta_target );
	float dist_ent    = VectorNormalize( delta_ent );
	
	if ( dist_target > dist_ent ) {
		return ( delta_target.Dot( delta_ent ) > 0.7071f );
	} else {
		return false;
	}
}

class CFindTeammates : public IVision::IForEachKnownEntity
{
public:
	CFindTeammates( CTFBot *bot, float range ) :
		m_pSearcher( bot ),
		m_flRangeLimit( range ) {}

	virtual bool Inspect( const CKnownEntity& known ) override
	{
		CTFPlayer *player = ToTFPlayer( known.GetEntity() );
		if ( player == nullptr ) return true;

		if ( this->m_pSearcher->IsRangeGreaterThan( player, this->m_flRangeLimit ) )	return true;
		if ( this->m_pSearcher->IsSelf( player ) )										return true;
		if ( !player->IsAlive() )														return true;
		if ( !this->m_pSearcher->IsFriend( player ) &&
			!player->m_Shared.DisguiseFoolsTeam( m_pSearcher->GetTeamNumber() ) )		return true;

		float dist;
		dist = m_pSearcher->GetDistanceBetween( player );

		if ( dist < this->m_flDistance ) {
			this->m_pClosest = player;
			this->m_flDistance = dist;
		}

		return true;
	}

	CTFPlayer *GetClosestAlly() const { return this->m_pClosest; }

	float GetDistance() const      { return this->m_flDistance; }

private:
	CTFBot *m_pSearcher;
	CTFPlayer *m_pClosest = nullptr;
	float m_flRangeLimit;
	float m_flDistance = FLT_MAX;
};

Action<CTFBot> *CTFBot::OpportunisticallyUseWeaponAbilities()
{
	if ( !this->m_ctUseWeaponAbilities.IsElapsed() ) return nullptr;
	this->m_ctUseWeaponAbilities.Start( RandomFloat( 0.5f, 1.0f ) );
	
	if ( this->IsPlayerClass( TF_CLASS_DEMOMAN ) && this->m_Shared.HasDemoShieldEquipped() && this->GetLocomotionInterface()->IsPotentiallyTraversable( this->GetAbsOrigin(), this->GetAbsOrigin() + ( 100.0f * EyeVectorsFwd( this ) ), ILocomotion::TRAVERSE_DEFAULT ) ) {
		if ( this->HasAttribute( AIR_CHARGE_ONLY ) ) {
			if ( this->GetGroundEntity() == nullptr && this->GetAbsVelocity().z <= 0.0f ) {
				this->PressAltFireButton();
			}
		} else {
			this->PressAltFireButton();
		}
	}
	
	for ( int i = 0; i < this->WeaponCount(); ++i ) {
		auto weapon = assert_cast<CTFWeaponBase *>( this->GetWeapon( i ) );
		if ( weapon == nullptr ) continue;
			
	//	if ( weapon->IsWeapon( TF_WEAPON_BUFF_ITEM ) ) {
	//		auto buff = assert_cast<CTFBuffItem *>( weapon );
	//		if ( buff->IsFull() ) {
	//			return new CTFBotUseItem( buff );
	//		}
	//	}
			
		if ( weapon->IsWeapon( TF_WEAPON_LUNCHBOX ) )
		{
			// TODO: implement scout lunchbox drink items if they ever exist
			Assert( !this->IsPlayerClass( TF_CLASS_SCOUT, true ) );

			if ( this->m_Shared.IsInvulnerable() ) continue;
				
			auto lunchbox = assert_cast<CTFLunchBox *>( weapon );
			if ( lunchbox->HasAmmo() ) {
				if ( this->HealthFraction() < tf_bot_health_ok_ratio.GetFloat() ) {
					const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat();
					if ( threat == nullptr || ( threat != nullptr && this->IsRangeGreaterThan( threat->GetLastKnownPosition(), 1500.0f ) ) ) {
						return new CTFBotUseItem( lunchbox, this );
					}
				}
			}
		}
			
	//	if ( weapon->IsWeapon( TF_WEAPON_BAT_WOOD ) ) {
	//		auto bat_wood = assert_cast<CTFBat_Wood *>( weapon );
	//		if ( bat_wood->GetAmmoCount( TF_AMMO_GRENADES1 ) > 0 ) {
	//			const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat();
	//			if ( threat != nullptr && threat->IsVisibleInFOVNow() ) {
	//				this->PressAltFireButton();
	//			}
	//		}
	//	}

		// Umbrella: give minicrit boost to allies
		if ( weapon->IsWeapon( TF_WEAPON_UMBRELLA ) && tf2c_vip_abilities.GetInt() > 1 )
		{
			auto umbrella = assert_cast<CTFUmbrella *>( weapon );
			if ( umbrella->GetProgress() >= 1.0f )
			{
				CFindTeammates functor( this, 750.0f );
				this->GetVisionInterface()->ForEachKnownEntity( functor );

				CTFPlayer* target = functor.GetClosestAlly();

				if ( target != nullptr ) 
				{
					const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat();
					if ( threat == nullptr ) continue;
					if ( TFGameRules()->InSetup() ) continue;
					if ( target->HealthFraction() < 0.5f ) continue;
					if ( GetThreatDanger( target ) < 0.5f ) continue;	// excludes Medic, Civilian, Sniper
					return new CTFBotUseItem( umbrella, target );
				}
			}
		}

		// Taser: heal critical health allies
		if ( weapon->IsWeapon( TF_WEAPON_TASER ) )
		{
			auto taser = assert_cast<CTFTaser *>( weapon );
			if ( taser->GetProgress() >= 1.0f )
			{
				CFindTeammates functor( this, 250.0f );
				this->GetVisionInterface()->ForEachKnownEntity( functor );

				CTFPlayer* target = functor.GetClosestAlly();

				if ( target != nullptr )
				{
					if ( TFGameRules()->InSetup() ) continue;
					if ( target->HealthFraction() > 0.8f ) continue;
					return new CTFBotUseItem( taser, target );
				}
			}
		}
	}
	
	return nullptr;
}


class CCollectReachableObjects : public ISearchSurroundingAreasFunctor
{
public:
	CCollectReachableObjects( const CTFBot *actor, float range, const CUtlVector<CHandle<CBaseEntity>> *ents_in, CUtlVector<CHandle<CBaseEntity>> *ents_out ) :
		m_Actor( actor ), m_flMaxRange( range ), m_pEntsIn( ents_in ), m_pEntsOut( ents_out ) {}
	virtual ~CCollectReachableObjects() {}
	
	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		for ( auto ent : *this->m_pEntsIn ) {
			if ( ent == nullptr ) continue;
			
			if ( area->Contains( ent->WorldSpaceCenter() ) && !this->m_pEntsOut->HasElement( ent ) ) {
				this->m_pEntsOut->AddToTail( ent );
			}
		}
		
		return true;
	}
	
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) override
	{
		if ( adjArea->IsBlocked( this->m_Actor->GetTeamNumber() ) ) return false;
		if ( travelDistanceSoFar > this->m_flMaxRange )           return false;
		//if ( !currentArea->IsContiguous( adjArea ) )                return false;
		
		return true;
	}
	
private:
	const CTFBot *m_Actor;
	float m_flMaxRange;
	const CUtlVector<CHandle<CBaseEntity>> *m_pEntsIn;
	CUtlVector<CHandle<CBaseEntity>> *m_pEntsOut;
};

void CTFBot::SelectReachableObjects( const CUtlVector<CHandle<CBaseEntity>>& ents_in, CUtlVector<CHandle<CBaseEntity>> *ents_out, const INextBotFilter& filter, CNavArea *area, float range ) const
{
	if ( area     == nullptr ) return;
	if ( ents_out == nullptr ) return;
	
	CUtlVector<CHandle<CBaseEntity>> ents;
	for ( const auto& ent : ents_in ) {
		if ( filter.IsSelected( ent ) ) {
			ents.AddToTail( ent );
		}
	}
	
	CCollectReachableObjects functor( this, range, &ents, ents_out );
	SearchSurroundingAreas( area, functor );
}

bool CTFBot::ShouldFireCompressionBlast()
{
	if ( TFGameRules()->IsInTraining() ) return false;
	
	if ( !tf_bot_pyro_always_reflect.GetBool() ) {
		switch ( this->GetSkill() ) {
		case CTFBot::EASY:   return false;																		// 100% chance to not airblast
		case CTFBot::NORMAL: if ( this->TransientlyConsistentRandomValue( 1.0f ) < 0.9f ) return false; break;	//  90% chance to not airblast
		case CTFBot::HARD:   if ( this->TransientlyConsistentRandomValue( 1.0f ) < 0.8f ) return false; break;	//  80% chance to not airblast
		case CTFBot::EXPERT: if ( this->TransientlyConsistentRandomValue( 1.0f ) < 0.6f ) return false; break;	//  60% chance to not airblast
		}
	}

	// Projectile Reflection
	// before player pushback as it can return false

	auto flamethrower = dynamic_cast<CTFFlameThrower *>( this->GetTFWeapon_Primary() );
	if ( flamethrower == nullptr ) return false;

	Vector box_size = flamethrower->GetDeflectionSize();
	Vector box_center = this->Weapon_ShootPosition() + ( EyeVectorsFwd( this ) * Max( box_size.x, box_size.y ) );

	CBaseEntity *list[0x80];
	int num_in_box = UTIL_EntitiesInBox( list, 0x80, box_center - box_size, box_center + box_size, 0 );
	// TODO: look into these flags; not necessarily 100% convinced that FL_GRENADE covers the right things
	// 20180813: no mask for now. used to be FL_CLIENT | FL_GRENADE

	for ( int i = 0; i < num_in_box; ++i ) {
		CBaseEntity *ent = list[i];
		if ( ent == this )           continue;
		if ( ent->IsPlayer() )       continue;
		if ( !this->IsEnemy( ent ) )   continue;
		if ( !ent->IsDeflectable() ) continue;

		// TODO: any other entity classes we ought to check for here?
		if ( !ent->ClassMatches( "tf_projectile_rocket" ) && !ent->ClassMatches( "tf_projectile_energy_ball" ) ) {





			// TODO
			Assert( false );
		}

		if ( this->IsDebugging( INextBot::DEBUG_BEHAVIOR | INextBot::DEBUG_VISION ) ) {
			NDebugOverlay::Line( this->Weapon_ShootPosition(), ent->WorldSpaceCenter(), NB_RGB_WHITE, false, 3.0f );
		}

		// IsLineOfSightClear fails on grenades?
		if ( this->GetVisionInterface()->IsInFieldOfView( ent->WorldSpaceCenter() ) ) {
			return true;
		}
	}

	// Player Knockback
	
	if ( !TFGameRules()->IsMannVsMachineMode() ) {
		const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat( true );
		if ( threat != nullptr ) {
			CTFPlayer *enemy = ToTFPlayer( threat->GetEntity() );
			if ( enemy != nullptr && this->IsRangeLessThan( enemy, tf_bot_pyro_shove_away_range.GetFloat() ) ) {

				// is pushing back possible?
				trace_t trace;

				Vector vecPos = enemy->GetAbsOrigin();
				QAngle vecAngles = GetAbsAngles();
				Vector vecForward;
				AngleVectors( vecAngles, &vecForward );

				UTIL_TraceLine( vecPos, vecPos + vecForward * 200.0, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );

				Vector vecEndPos = trace.endpos;

				// don't airblast unless there's empty space behind our enemy
				if ( trace.fraction == 1.0 )
				{
					/* airblast ubered enemies 100% of the time */
					if ( enemy->m_Shared.IsInvulnerable() ) {
						return true;
					}

					// don't airblast easy prey:
					if ( ( enemy->HealthFraction() < 0.5f ) || ( GetThreatDanger( enemy ) <= 0.5f ) )
					{
						return false;
					}

					/* airblast airborne enemies 50% of the time */
					if ( enemy->GetGroundEntity() == nullptr && this->TransientlyConsistentRandomValue( 0.5f ) < 0.5f ) {
						return true;
					}

					/* airblast enemies capturing the point 100% of the time */
					if ( enemy->IsCapturingPoint() ) {
						return true;
					}

					// airblast enemies off high areas
					UTIL_TraceLine( vecEndPos, vecEndPos + Vector( 0, 0, -192.0 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
					if ( trace.fraction == 1.0 ) {
						return true;
					}

					/* airblast other enemies 10% of the time */
					if ( this->TransientlyConsistentRandomValue( 1.0f ) < 0.1f ) {
						return true;
					}
				}
			}
		}
	}
	
	return false;
}


// TODO: get rid of this horrendous function and rework its former users
CTFPlayer *CTFBot::SelectRandomReachableEnemy() const
{
	VPROF_BUDGET( "CTFBot::SelectRandomReachableEnemy", "NextBot" );
	
	// NOTE: live TF2 seems to define "reachable" as "not in spawn room" rather than "could actually be pathed to"
	// NOTE: live TF2 doesn't ignore stealthed/disguised spies!
	// BUG: visibility of enemies isn't checked
	// BUG: distance to enemies isn't taken into consideration whatsoever
	
	// AI actions that use this function are horrible trash, essentially:
	// this function is magic omniscience for discovering an enemy player ( who could be clear across the map and not
	// visible to this bot ) to which to start pathing toward... it's disgusting
	
	/* bypass everything if the reachability checks are all just going to fail anyway */
	if ( this->GetLastKnownTFArea() == nullptr ) {
		return nullptr;
	}
	
	CUtlVector<CTFPlayer *> enemies;
	this->CollectEnemyPlayers( &enemies, true );
	
	for ( int i = 0; i < enemies.Count(); ++i ) {
		CTFPlayer *enemy = enemies[i];
		
		/* we may be using idiotic selection criteria, but let's at least ignore cloaked/disguised spies */
		if ( enemy->m_Shared.IsStealthed() || ( enemy->m_Shared.InCond( TF_COND_DISGUISED ) && enemy->m_Shared.DisguiseFoolsTeam(this->GetTeamNumber()) ) ) {
			enemies.FastRemove( i );
			--i;
		}
	}
	
	enemies.Shuffle();
	
	/* do an actual reachability test and only return an enemy that could be successfully pathed to */
	// NOTE: using DEFAULT_ROUTE here purely based on the fact that CTFBotAttackFlagDefenders uses it too
	CTFBotPathCost cost_func( this, DEFAULT_ROUTE );
	for ( auto enemy : enemies ) {
		// TODO: use SearchSurroundingAreas ( in a similar manner as CTFBot::SelectReachableObjects does )
		// rather than doing a ton of individual path calculation calls like this
		float dist = NavAreaTravelDistance( this->GetLastKnownTFArea(), enemy->GetLastKnownTFArea(), cost_func, 0.0f, this->GetTeamNumber() );
		if ( dist != -1.0f ) {
			return enemy;
		}
	}
	
	return nullptr;
}

bool CTFBot::IsRandomReachableEnemyStillValid( CTFPlayer *enemy ) const
{
	if ( enemy->m_Shared.IsStealthed() || ( enemy->m_Shared.InCond( TF_COND_DISGUISED ) && enemy->m_Shared.DisguiseFoolsTeam(this->GetTeamNumber()) ) ) {
		return false;
	}
	
	return true;
}


// TODO: this function needs a LOT of work to make it match up with weapon code in an ideal manner
Vector CTFBot::EstimateGrenadeProjectileImpactPosition( CTFWeaponBase *launcher, float pitch, float yaw ) const
{
	// TODO: magic number for grenade projectile speed
	// get this in a proper manner so we actually account for item attrs on the launcher, etc
	return this->EstimateProjectileImpactPosition( pitch, yaw, 900.0f );
}

// TODO: this function needs a LOT of work to make it match up with weapon code in an ideal manner
Vector CTFBot::EstimateStickybombProjectileImpactPosition( CTFWeaponBase *launcher, float pitch, float yaw, float charge ) const
{
	// TODO: magic number/algorithm for stickybomb projectile speed
	// get this in a proper manner so we actually account for item attrs on the launcher, etc
	return this->EstimateProjectileImpactPosition( pitch, yaw, 900.0f + ( 1500.0f * charge ) );
}


#if 0
// NOTE: this is dead code in live TF2, so maybe just rip this function out entirely...
Vector CTFBot::EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon ) const
{
	Assert( weapon != nullptr );
	if ( weapon == nullptr ) return this->GetAbsOrigin();
	
	const QAngle& eye = this->EyeAngles();
	
	if ( weapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) ) {
		// TODO: access this properly
		float charge = -100.0f;
		Assert( false );
		
		return this->EstimateStickybombProjectileImpactPosition( eye.x, eye.y, charge );
	} else {
		return this->EstimateProjectileImpactPosition( eye.x, eye.y, weapon->GetProjectileSpeed() );
	}
}

Vector CTFBot::EstimateProjectileImpactPosition( float pitch, float yaw, float speed ) const
{
	// TODO
	Assert( false );
	
	// REMOVE ME
	return vec3_origin;
}

Vector CTFBot::EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge ) const
{
	// TODO: magic numbers
	// ( see if we can just call into CTFPipebombLauncher for this, or use constants, or something )
	float speed = 900.0f + ( charge * 1500.0f );
	CALL_ATTRIB_HOOK_FLOAT( speed, mult_projectile_range );
	// TODO: add mult_projectile_range to items_game.txt if necessary
	// TODO: ensure that mult_projectile_range is also hooked in all the other necessary places
	
	return this->EstimateProjectileImpactPosition( pitch, yaw, speed );
}
#endif

bool CTFBot::IsAnyEnemySentryAbleToAttackMe() const
{
	if ( this->m_Shared.IsStealthed() ) {
		return false;
	}
	
	bool disguised = ( this->m_Shared.InCond( TF_COND_DISGUISED ) || this->m_Shared.InCond( TF_COND_DISGUISING ) );
	
	for ( auto obj : CBaseObject::AutoList() ) {
		if ( !obj->IsSentry() ) continue;
		auto sentry = assert_cast<CObjectSentrygun *>( obj );
		
		if ( sentry->HasSapper() )      continue;
		if ( sentry->IsDisabled() )     continue;
		if ( sentry->IsBeingCarried() ) continue;
		if ( sentry->IsBuilding() )     continue;
		
		if ( disguised && this->m_Shared.DisguiseFoolsTeam( sentry->GetTeamNumber() ) ) {
			continue;
		}
		
		if ( this->GetAbsOrigin().DistToSqr( sentry->GetAbsOrigin() ) <= sentry->GetMaxRange() && this->IsThreatAimingTowardMe( sentry, 0.95f ) && this->IsLineOfSightClear( sentry, CBaseCombatCharacter::IGNORE_ACTORS ) ) {
			return true;
		}
	}
	
	return false;
}

bool CTFBot::IsThreatAimingTowardMe( CBaseEntity *threat, float dot ) const
{
	Vector dir = this->GetAbsOrigin() - threat->GetAbsOrigin();
	float dist = dir.NormalizeInPlace();
	
	CBasePlayer *player = ToBasePlayer( threat );
	if ( player != nullptr ) {
		return ( dir.Dot( EyeVectorsFwd( player ) ) > dot );
	}
	
	auto sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry != nullptr && dist < sentry->GetMaxRange() ) {
		return ( dir.Dot( sentry->GetTurretVector() ) > dot );
	}
	
	return false;
}

bool CTFBot::IsThreatFiringAtMe( CBaseEntity *threat ) const
{
	if ( !this->IsThreatAimingTowardMe( threat, 0.8f ) ) {
		return false;
	}
	
	CBasePlayer *player = ToBasePlayer( threat );
	if ( player != nullptr ) {
		return player->IsFiringWeapon();
	}
	
	auto sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry != nullptr ) {
		return ( sentry->GetTimeSinceLastFired() < 1.0f );
	}
	
	return false;
}


int CTFBot::CollectEnemyPlayers( CUtlVector<CTFPlayer *> *playerVector, bool isAlive, bool shouldAppend ) const
{
	if ( !shouldAppend ) {
		playerVector->RemoveAll();
	}
	
	this->ForEachEnemyTeam( [=]( int team ){
		CollectPlayers( playerVector, team, isAlive, true );
		return true;
	} );
	
	return playerVector->Count();
}

void CTFBot::FindFrontlineIncDist()
{
	// Define a safe frontline based on incursion so we don't run in front of teammates
	CUtlVector<CTFPlayer *> allies;
	int iTeamID = this->GetTeamNumber();
	CollectPlayers( &allies, iTeamID, true );
	if ( allies.Count() > 0 )
	{
		allies.Shuffle();

		float farthest_incdist = 0.0f;
		float average_incdist = 0.0f;

		for ( const auto *ally : allies ) {
			auto ally_area = ally->GetLastKnownTFArea();
			if ( ally_area )
			{
				float dist = ally_area->GetIncursionDistance( iTeamID );

				if ( dist < 0.0f )
					continue;

				if ( farthest_incdist < dist )
					farthest_incdist = dist;

				average_incdist += dist;
			}
		}

		average_incdist /= allies.Count();

		m_flFrontlineIncDistAvg = average_incdist;
		m_flFrontlineIncDistFar = farthest_incdist;
	}
}


// TODO: REMOVE ME
struct EstSegment
{
	EstSegment( int idx, float t, const CGameTrace& tr ) :
		idx( idx ), t( t ), startpos( tr.startpos ), endpos( tr.endpos ), didhit( tr.DidHit() ) {}
	
	int idx;
	float t;
	Vector startpos;
	Vector endpos;
	bool didhit;
};
static CUtlVector<EstSegment> est_segments;

// TODO: this function needs a LOT of work to make it match up with weapon code in an ideal manner
Vector CTFBot::EstimateProjectileImpactPosition( float pitch, float yaw, float proj_speed ) const
{
	// TODO: these magic numbers for initial offset adjustment are stolen/duplicated from CTFWeaponBase::FireGrenade
	// move these into #defines in a header and have that code and this code both reference those constants
	constexpr float off_fwd = 16.0f;
	constexpr float off_rt  =  8.0f;
	constexpr float off_up  = -6.0f;
	
	// TODO: this magic number for additional upward initial velocity is ALSO stolen/duplicated from CTFWeaponBase::FireGrenade
	// it ( as well as the +/-10 variance for up and right ) should be moved into header #defines and then shared, just like the initial pos offsets
	constexpr float extra_vel_up = 200.0f;
	
	// TODO: these mins/maxs for the grenade trace don't make much sense
	// e.g. why are they -0,+8 rather than -4,+4?
	// also, they don't match the actual bounds of the grenade projectile; but is that intentional to account for the aim randomness when fired?
	// and to top it all off, whether or not these are intentionally bloated, they aren't sourced from anywhere sane/common
	constexpr Vector proj_mins( -0.0f, -0.0f, -0.0f );
	constexpr Vector proj_maxs( 8.0f,  8.0f,  8.0f );
	
	// TODO: magic number for t_max ( is 5.0 seconds always sufficient? )
	constexpr float t_max = 5.00f;
	constexpr float dt    = 0.01f;
	
	
	Vector aim_fwd; Vector aim_rt; Vector aim_up;
	AngleVectors( QAngle( pitch, yaw, 0.0f ), &aim_fwd, &aim_rt, &aim_up );
	
	Vector pos = const_cast<CTFBot *>( this )->Weapon_ShootPosition() + ( off_fwd * aim_fwd ) + ( off_rt * aim_rt ) + ( off_up * aim_up );
	
	float half_gravity = 0.5f * GetCurrentGravity();
	
	Vector aim_xy_dir = VectorXY( aim_fwd ).Normalized();
	
	// TODO: magic number 0.9 for velocity multiplier; also this thing seems just generally questionable
	float speed_xy = 0.9f * ( ( proj_speed * aim_fwd  ) + ( extra_vel_up * aim_up  ) ).Length2D();
	float speed_z  = 0.9f * ( ( proj_speed * aim_fwd.z ) + ( extra_vel_up * aim_up.z ) );
	
	NextBotTraceFilterIgnoreActors filter;
	trace_t tr;
	
	// REMOVE ME vvv
	est_segments.RemoveAll();
	int i = 0;
	// REMOVE ME ^^^
	
	for ( float t = 0.00f; t < t_max; t += dt ) {
		// BUG: live TF2 multiplies by t to do this approximate integration, when it should to be using dt!
		Vector next = pos;
		next.x += ( dt * speed_xy * aim_xy_dir.x );
		next.y += ( dt * speed_xy * aim_xy_dir.y );
		next.z += ( dt * speed_z ) - ( half_gravity * Square( dt ) );
		
		UTIL_TraceHull( pos, next, proj_mins, proj_maxs, MASK_SOLID_BRUSHONLY, &filter, &tr );
		// REMOVE ME vvv
		est_segments.AddToTail( EstSegment( i++, t, tr ) );
		// REMOVE ME ^^^
		if ( tr.DidHit() ) break;
		
		pos = next;
	}
	
	return tr.endpos;
}

// TODO: REMOVE THIS LATER
// TODO: call this from all places that use Estimate\w*ProjectileImpactPosition
ConVar cvar_vis_enable_good( "sig_vis_est_enable_good",       "0", FCVAR_CHEAT, "" );
ConVar cvar_vis_enable_bad ( "sig_vis_est_enable_bad",        "0", FCVAR_CHEAT, "" );
ConVar cvar_vis_ignorez    ( "sig_vis_est_ignorez",           "1", FCVAR_CHEAT, "" );
ConVar cvar_vis_duration   ( "sig_vis_est_duration",       "0.09", FCVAR_CHEAT, "" );
ConVar cvar_vis_color_good ( "sig_vis_est_color_good", "00ff0080", FCVAR_CHEAT, "" );
ConVar cvar_vis_color_bad  ( "sig_vis_est_color_bad",  "ff000080", FCVAR_CHEAT, "" );
ConVar cvar_vis_color_hit  ( "sig_vis_est_color_hit",  "ffff0080", FCVAR_CHEAT, "" );
ConVar cvar_vis_box_good   ( "sig_vis_est_box_good",        "0.0", FCVAR_CHEAT, "Set to 0 to disable" );
ConVar cvar_vis_box_bad    ( "sig_vis_est_box_bad",         "0.0", FCVAR_CHEAT, "Set to 0 to disable" );
#define ColorRGB( c )  c[0], c[1], c[2]
#define ColorRGBA( c ) c[0], c[1], c[2], c[3]
void CTFBot::DrawProjectileImpactEstimation( bool good ) const
{
	if ( good && !cvar_vis_enable_good.GetBool() ) return;
	if ( !good && !cvar_vis_enable_bad .GetBool() ) return;
	
	auto l_getcolor = [&]( const ConVar& cvar ){
		unsigned int r = 0xff, g = 0xff, b = 0xff, a = 0x80;
		sscanf( cvar.GetString(), " %02x%02x%02x%02x ", &r, &g, &b, &a );
		return Color( r, g, b, a );
	};
	
	const Color c_nohit = l_getcolor( good ? cvar_vis_color_good : cvar_vis_color_bad );
	const Color c_hit   = l_getcolor( cvar_vis_color_hit );
	
	float box = ( good ? cvar_vis_box_good.GetFloat() : cvar_vis_box_bad.GetFloat() );
	
	for ( const auto& seg : est_segments ) {
		const Color& c = ( seg.didhit ? c_hit : c_nohit );
		
		NDebugOverlay::Line( seg.startpos, seg.endpos, ColorRGB( c ), cvar_vis_ignorez.GetBool(), cvar_vis_duration.GetFloat() );
		
		if ( box != 0.0f ) {
			NDebugOverlay::Box( seg.endpos, Vector( -box ), Vector( box ), ColorRGBA( c ), cvar_vis_duration.GetFloat() );
		}
		
		// TODO: indexes
		// TODO: times
	}
}


CTFBot::CTFBotIntention::CTFBotIntention( INextBot *nextbot ) :
	IIntention( nextbot )
{
	this->Reset();
}

CTFBot::CTFBotIntention::~CTFBotIntention()
{
	if ( this->m_pBehavior != nullptr ) {
		delete this->m_pBehavior;
	}
}


void CTFBot::CTFBotIntention::Reset()
{
	if ( this->m_pBehavior != nullptr ) {
		delete this->m_pBehavior;
	}
	
	// NOTE: live TF2 always starts on CTFBotMainAction; but for ( probably ) improved handling of instances where the bot
	// spawns in a dead state first, we'll go straight to CTFBotDead to avoid initializing a bunch of contained actions
	// that we're just going to end up tearing down in the very next update when we realize that we're dead
	if ( assert_cast<CTFBot *>( this->GetBot() )->IsAlive() ) {
		this->m_pBehavior = new Behavior<CTFBot>( new CTFBotMainAction(), "TFBot" );
	} else {
		this->m_pBehavior = new Behavior<CTFBot>( new CTFBotDead(), "TFBot" );
	}
}

void CTFBot::CTFBotIntention::Update()
{
	this->m_pBehavior->Update( assert_cast<CTFBot *>( this->GetBot() ), this->GetUpdateInterval() );
}


CTFBotPathCost::CTFBotPathCost( const CTFBot *actor, RouteType rtype ) :
	m_pBot( actor ), m_iRouteType( rtype ),
	m_flStepHeight     ( actor->GetLocomotionInterface()->GetStepHeight() ),
	m_flMaxJumpHeight  ( actor->GetLocomotionInterface()->GetMaxJumpHeight() ),
	m_flDeathDropHeight( actor->GetLocomotionInterface()->GetDeathDropHeight() )
{
	this->m_EnemyObjects.RemoveAll();
	if ( actor->IsPlayerClass( TF_CLASS_SPY, true ) ) {
		actor->ForEachEnemyTeam( [=]( int team ){
			CUtlVector<CBaseObject *> objs;
			TheTFNavMesh->CollectBuiltObjects( &objs, team );
			
			this->m_EnemyObjects.AddVectorToTail( objs );
			
			return true;
		} );
	}
}


float CTFBotPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	VPROF_BUDGET( "CTFBotPathCost::operator()", "NextBot" );
	
	auto tf_area = static_cast<CTFNavArea *>( area );
	
	/* first area in path: zero cost */
	if ( fromArea == nullptr ) return 0.0f;
	
	/* can't traverse this area */
	if ( !this->m_pBot->GetLocomotionInterface()->IsAreaTraversable( area ) ) {
		return -1.0f;
	}
	
	/* training mode: don't start the capture; join the player once he caps */
	if ( TFGameRules()->IsInTraining() && tf_area->HasAnyTFAttributes( CTFNavArea::CONTROL_POINT ) && !this->m_pBot->IsAnyPointBeingCaptured() && !this->m_pBot->IsPlayerClass( TF_CLASS_ENGINEER, true ) ) {
		return -1.0f;
	}
	
	/* can't enter enemy spawn rooms unless we've won the round */
	if ( tf_area->HasEnemySpawnRoom( this->m_pBot ) && !( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() == this->m_pBot->GetTeamNumber() ) ) {
		return -1.0f;
	}
	
	float cost;
	if ( ladder != nullptr ) {
		cost = ladder->m_length;
	} else if ( length != 0.0f ) {
		cost = length;
	} else {
		cost = ( area->GetCenter() - fromArea->GetCenter() ).Length();
	}
	
	// NOTE: live TF2 always does this block, but we only do it for non-ladder cases,
	// because otherwise, we'd return -1.0f due to the ladder's high delta-z,
	// which causes bots to never try to use ladders at all
	if ( ladder == nullptr ) {
		float delta_z = fromArea->ComputeAdjacentConnectionHeightChange( area );
		if ( delta_z >= this->m_flStepHeight ) {
			/* can't jump that high */
			if ( delta_z >= this->m_flMaxJumpHeight ) return -1.0f;
			
			/* cost penalty for jumping up ledges */
			cost *= 2;
		} else {
			/* don't want to fall down and die */
			if ( delta_z < -this->m_flDeathDropHeight ) return -1.0f;
		}
	}
	
	float multiplier = 1.0f;
	
	/* massive random cost modifier for non-giant bots */
	if ( this->m_iRouteType == DEFAULT_ROUTE && !this->m_pBot->IsMiniBoss() ) {
		multiplier += 50.0f * ( this->m_pBot->TransientlyConsistentRandomValue( area, 10.0f ) + 1.0f );
	}
	
	/* avoid in-combat areas and areas covered by sentries */
	if ( this->m_iRouteType != FASTEST_ROUTE ) {

		if ( tf_area->IsInCombat() ) {
			cost *= 2.0f * tf_area->GetCombatIntensity();
		}

		if ( tf_area->HasEnemySentry( this->m_pBot ) ) {
			cost *= 3.0f;
		}

		if ( this->m_iRouteType == SAFEST_ROUTE )
		{
			if ( tf_area->IsInCombat() ) {
				cost *= 4.0f * tf_area->GetCombatIntensity();
			}

			if ( tf_area->HasEnemySentry( this->m_pBot ) ) {
				cost *= 5.0f;
			}
		}
	}
	
	if ( this->m_pBot->IsPlayerClass( TF_CLASS_SPY, true ) ) {
		/* get away from sentry guns ASAP ( i.e. after sapping? ) */
		for ( auto obj : this->m_EnemyObjects ) {
			if ( obj->IsSentry() ) {
				obj->UpdateLastKnownArea();
				if ( area == obj->GetLastKnownTFArea() ) {
					cost *= 10.0f;
				}
			}
		}
		
		/* prefer to stay away from teammates */
		cost += ( cost * 10.0f * area->GetPlayerCount( this->m_pBot->GetTeamNumber() ) );
	}

	if ( this->m_pBot->IsPlayerClass( TF_CLASS_CIVILIAN, true ) ) {
		// Don't run in front of teammates
		int iTeamID = this->m_pBot->GetTeamNumber();
		if ( tf_area->GetIncursionDistance( iTeamID ) <= this->m_pBot->GetFrontlineIncDistAvg() )
		{
			//NDebugOverlay::Cross3D( tf_area->GetCenter() + Vector( 0, 0, 8 ), 4.0f, NB_RGB_GREEN, false, 0.5f );
		}
		else if ( tf_area->GetIncursionDistance( iTeamID ) > this->m_pBot->GetFrontlineIncDistAvg() && tf_area->GetIncursionDistance( iTeamID ) < this->m_pBot->GetFrontlineIncDistFar() )
		{
			cost *= 10.0f;
			//NDebugOverlay::Cross3D( tf_area->GetCenter() + Vector( 0, 0, 8 ), 8.0f, NB_RGB_YELLOW, false, 0.5f );
		}
		else if ( tf_area->GetIncursionDistance( iTeamID ) >= this->m_pBot->GetFrontlineIncDistFar() )
		{
			cost *= 100.0f;
			//NDebugOverlay::Cross3D( tf_area->GetCenter() + Vector( 0, 0, 8 ), 16.0f, NB_RGB_RED, false, 0.5f );
		}
	}
	
	cost *= multiplier;
	
	if ( area->HasAttributes( NAV_MESH_FUNC_COST ) ) {
		cost *= area->ComputeFuncNavCost( const_cast<CTFBot *>( this->m_pBot ) );
	}
	
	return fromArea->GetCostSoFar() + cost;
}


float CTFPlayertPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	VPROF_BUDGET( "CTFPlayertPathCost::operator()", "NextBot" );
	
	auto tf_area = static_cast<CTFNavArea *>( area );
	
	/* first area in path: zero cost */
	if ( fromArea == nullptr ) return 0.0f;
	
	/* can't traverse this area */
	if ( !this->m_pPlayer->IsAreaTraversable( area ) ) {
		return -1.0f;
	}
	
	/* can't enter enemy spawn rooms unless we've won the round */
	if ( tf_area->HasEnemySpawnRoom( this->m_pPlayer ) && !( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() == this->m_pPlayer->GetTeamNumber() ) ) {
		return -1.0f;
	}
	
	float cost;
	if ( ladder != nullptr ) {
		cost = ladder->m_length;
	} else if ( length != 0.0f ) {
		cost = length;
	} else {
		cost = ( area->GetCenter() - fromArea->GetCenter() ).Length();
	}
	
	float delta_z = fromArea->ComputeAdjacentConnectionHeightChange( area );
	if ( delta_z >= this->m_flStepHeight ) {
		/* can't jump that high */
		if ( delta_z >= this->m_flMaxJumpHeight ) return -1.0f;
		
		/* cost penalty for jumping up ledges */
		cost *= 2;
	} else {
		/* don't want to fall down and die */
		if ( delta_z < -this->m_flDeathDropHeight ) return -1.0f;
	}
	
	return fromArea->GetCostSoFar() + cost;
}


CTFBot *CreateTFBot( const char *name, CTFBot::DifficultyType skill )
{
	if ( skill == CTFBot::UNDEFINED_SKILL ) {
		extern ConVar tf_bot_difficulty;
		skill = Clamp( ( CTFBot::DifficultyType )tf_bot_difficulty.GetInt(), CTFBot::EASY, CTFBot::EXPERT );
	}
	
	if ( TFGameRules()->IsInTraining() ) {
		skill = CTFBot::EASY;
	}
	
	char buf[0x100];
	CreateBotName( buf, skill, name );
	
	CTFBot *bot = NextBotCreatePlayerBot<CTFBot>( buf );
	if ( bot == nullptr ) return nullptr;
	
	bot->SetSkill     ( skill ); // set the actual skill level
	bot->SetTFBotSkill( skill ); // just the CTFPlayer netvar
	
	return bot;
}

static void CMD_BotWarpTeamToMe( const CCommand& args )
{
	CBasePlayer *host = UTIL_GetListenServerHost();
	if ( host == nullptr ) return;
	
	CTeam *team = host->GetTeam();
	for ( int i = 0; i < team->GetNumPlayers(); ++i ) {
		CBasePlayer *player = team->GetPlayer( i );
		if ( player == host )     continue;
		if ( !player->IsAlive() ) continue;
		
		player->SetAbsOrigin( host->GetAbsOrigin() );
	}
}
static ConCommand tf_bot_warp_team_to_me( "tf_bot_warp_team_to_me", &CMD_BotWarpTeamToMe, "", FCVAR_CHEAT | FCVAR_GAMEDLL );


static bool IsPlayerClassname( const char *str )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; ++i ) {
		if ( FStrEq( str, GetPlayerClassData( i )->m_szClassName ) ) {
			return true;
		}
	}
	
	return false;
}

static bool IsTeamName( const char *str )
{
	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; ++i ) {
		if ( FStrEq( str, g_aTeamNames[i] ) ) {
			return true;
		}
	}
	
	return false;
}


CON_COMMAND_F( tf_bot_add, "Add a bot.", FCVAR_GAMEDLL )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() ) return;
	
	bool bQuotaManaged = true;
	
	auto iDifficulty = CTFBot::UNDEFINED_SKILL;
	
	int nCount = 1;
	
	const char *pszBotName   = nullptr;
	const char *pszJoinClass = nullptr;
	const char *pszJoinTeam  = "auto";
	
	for ( int i = 1; i < args.ArgC(); ++i ) {
		auto iArgDifficulty = StringToDifficultyLevel( args[i] );
		int nArgCount = strtol( args[i], nullptr, 10 );
		
		if ( IsPlayerClassname( args[i] ) ) {
			pszJoinClass = args[i];
			continue;
		}
		
		if ( IsTeamName( args[i] ) ) {
			pszJoinTeam = args[i];
			continue;
		}
		
		if ( FStrEq( args[i], "noquota" ) ) {
			bQuotaManaged = false;
			continue;
		}
		
		if ( iArgDifficulty != CTFBot::UNDEFINED_SKILL ) {
			iDifficulty = iArgDifficulty;
			continue;
		}
		
		if ( nArgCount > 0 ) {
			pszBotName = nullptr;
			nCount = nArgCount;
			continue;
		}
		
		if ( nCount == 1 ) {
			pszBotName = args[i];
			continue;
		}
		
		Warning( "Invalid argument '%s'\n", args[i] );
	}
	
	if ( !FStrEq( tf_bot_force_class.GetString(), "" ) ) {
		pszJoinClass = tf_bot_force_class.GetString();
	}
	
	int nAdded = 0;
	for ( int i = 0; i < nCount; ++i ) {
		auto pBot = CreateTFBot( pszBotName, iDifficulty );
		if ( pBot == nullptr ) continue;
		
		if ( bQuotaManaged ) {
			pBot->SetAttribute( CTFBot::QUOTA_MANAGED );
		}
		
		pBot->HandleCommand_JoinTeam( pszJoinTeam );
		Assert( pBot->GetTeamNumber() >= LAST_SHARED_TEAM );
		Assert( pBot->GetTeamNumber() <  TF_TEAM_COUNT );
		
		const char *pszThisClass = pszJoinClass;
		if ( pszThisClass == nullptr ) {
			pszThisClass = pBot->GetNextSpawnClassname();
		}

		pBot->HandleCommand_JoinClass( pszThisClass );
		Assert( pBot->GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED );
		Assert( pBot->GetDesiredPlayerClassIndex() == TF_CLASS_RANDOM || pBot->GetDesiredPlayerClassIndex() < TF_CLASS_COUNT_ALL );
		
		if ( TFGameRules()->IsInTraining() ) {
			Assert( false );
			// TODO: some training-mode-specific name stuff
		}
		
		++nAdded;
	}
	
	if ( bQuotaManaged ) {
		TheTFBots().OnForceAddedBots( nAdded );
	}
}

CON_COMMAND_F( tf_bot_kick, "Remove a TFBot by name, or an entire team ( \"red\" or \"blue\" or \"green\" or \"yellow\" ), or all bots ( \"all\" ).", FCVAR_GAMEDLL )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() ) return;
	
	if ( args.ArgC() <= 1 ) {
		DevMsg( "%s <bot name>, \"red\", \"blue\", \"green\", \"yellow\", or \"all\" ( optional: \"moveToSpectatorTeam\" )\n", args[0] );
		return;
	}
	
	// TODO: fix the ugly horrible arg handling that some idiot designed here
	
	int iTeamNum      = TEAM_UNASSIGNED;
	bool bMoveToSpec  = false;
	const char *pName = nullptr;
	
	for ( int i = 1; i < args.ArgC(); ++i ) {
		// TODO: probably use GetTeamNumberFromString or something like that
		if ( FStrEq( args[i], "all" ) ) {
			iTeamNum = TEAM_ANY;
		} else if ( FStrEq( args[i], "red" ) ) {
			iTeamNum = TF_TEAM_RED;
		} else if ( FStrEq( args[i], "blue" ) ) {
			iTeamNum = TF_TEAM_BLUE;
		} else if ( FStrEq( args[i], "green" ) ) {
			iTeamNum = TF_TEAM_GREEN;
		} else if ( FStrEq( args[i], "yellow" ) ) {
			iTeamNum = TF_TEAM_YELLOW;
		} else if ( FStrEq( args[i], "moveToSpectatorTeam" ) ) {
			bMoveToSpec = true;
		} else {
			pName = args[i];
		}
	}
	
	int nKicked = 0;
	
	for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
		CTFBot *pBot = ToTFBot( UTIL_PlayerByIndex( i ) );
		if ( pBot == nullptr ) continue;
		
		if ( iTeamNum == TEAM_ANY || iTeamNum == pBot->GetTeamNumber() || ( pName != nullptr && FStrEq( pName, pBot->GetPlayerName() ) ) ) {
			if ( bMoveToSpec ) {
				pBot->ChangeTeam( TEAM_SPECTATOR, false, true );
			} else {
				/* ideally we'd do this in a manner not involving strings, but that's essentially unviable */
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
			}
			
			if ( pBot->HasAttribute( CTFBot::QUOTA_MANAGED ) ) {
				++nKicked;
			}
		}
	}
	
	TheTFBots().OnForceKickedBots( nKicked );
}

CON_COMMAND_F( tf_bot_kill, "Kill a TFBot by name, or an entire team ( \"red\" or \"blue\" or \"green\" or \"yellow\" ), or all bots ( \"all\" ).", FCVAR_GAMEDLL )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() ) return;
	
	if ( args.ArgC() <= 1 ) {
		DevMsg( "%s <bot name>, \"red\", \"blue\", \"green\", \"yellow\", or \"all\"\n", args[0] );
		return;
	}
	
	// TODO: fix the ugly horrible arg handling that some idiot designed here
	
	int iTeamNum      = TEAM_UNASSIGNED;
	const char *pName = nullptr;
	
	for ( int i = 1; i < args.ArgC(); ++i ) {
		// TODO: probably use GetTeamNumberFromString or something like that
		if ( FStrEq( args[i], "all" ) ) {
			iTeamNum = TEAM_ANY;
		} else if ( FStrEq( args[i], "red" ) ) {
			iTeamNum = TF_TEAM_RED;
		} else if ( FStrEq( args[i], "blue" ) ) {
			iTeamNum = TF_TEAM_BLUE;
		} else if ( FStrEq( args[i], "green" ) ) {
			iTeamNum = TF_TEAM_GREEN;
		} else if ( FStrEq( args[i], "yellow" ) ) {
			iTeamNum = TF_TEAM_YELLOW;
		} else {
			pName = args[i];
		}
	}
	
	for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
		CTFBot *pBot = ToTFBot( UTIL_PlayerByIndex( i ) );
		if ( pBot == nullptr ) continue;
		
		if ( iTeamNum == TEAM_ANY || iTeamNum == pBot->GetTeamNumber() || ( pName != nullptr && FStrEq( pName, pBot->GetPlayerName() ) ) ) {
			pBot->CommitSuicide( false, true );
		}
	}
}
