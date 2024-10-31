/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_manager.h"
#include "tf_bot.h"
#include "tf_nav_mesh.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "tf_hint.h"
#include "tf_sentrygun.h"
#include "tf_teleporter_exit.h"
#include "filesystem.h"

#include "tf_randomizer_manager.h"


// TODO: look for all references in live TF2 to these convars
       ConVar tf_bot_difficulty       ("tf_bot_difficulty",             "1", FCVAR_NONE,    "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert.");
static ConVar tf_bot_quota            ("tf_bot_quota",                  "0", FCVAR_NONE,    "Determines the total number of tf bots in the game.");
static ConVar tf_bot_quota_mode       ("tf_bot_quota_mode",        "normal", FCVAR_NONE,    "Determines the type of quota. Allowed values: 'normal', 'fill', and 'match'. If 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota. If 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota.");
static ConVar tf_bot_join_after_player("tf_bot_join_after_player",      "1", FCVAR_NONE,    "If nonzero, bots wait until a player joins before entering the game.");
static ConVar tf_bot_auto_vacate      ("tf_bot_auto_vacate",            "1", FCVAR_NONE,    "If nonzero, bots will automatically leave to make room for human players.");
static ConVar tf_bot_offline_practice ("tf_bot_offline_practice",       "0", FCVAR_NONE,    "Tells the server that it is in offline practice mode.");
static ConVar tf_bot_melee_only       ("tf_bot_melee_only",             "0", FCVAR_GAMEDLL, "If nonzero, TFBots will only use melee weapons");

static CTFBotManager sTFBotManager;


static bool UTIL_KickBotFromTeam(int teamnum)
{
	/* first, attempt to find a dead bot on the desired team */
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
		if (bot == nullptr) continue;
		
		if (!bot->HasAttribute(CTFBot::QUOTA_MANAGED)) continue;
		if ((bot->GetFlags() & FL_FAKECLIENT) == 0)    continue; // why?
		if (bot->IsAlive())                            continue;
		if (bot->GetTeamNumber() != teamnum)           continue;
		
		bot->Kick();
		return true;
	}
	
	/* failing that, look for any bot on the desired team, dead or alive */
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
		if (bot == nullptr) continue;
		
		if (!bot->HasAttribute(CTFBot::QUOTA_MANAGED)) continue;
		if ((bot->GetFlags() & FL_FAKECLIENT) == 0)    continue; // why?
		if (bot->GetTeamNumber() != teamnum)           continue;
		
		bot->Kick();
		return true;
	}
	
	return false;
}


void CTFBotManager::Update()
{
	this->MaintainBotQuota();
	this->DrawStuckBotData(0.1f);
	
	NextBotManager::Update();
}

void CTFBotManager::OnMapLoaded()
{
	NextBotManager::OnMapLoaded();
	
	this->ClearStuckBotData();
}

void CTFBotManager::OnRoundRestart()
{
	NextBotManager::OnRoundRestart();
	
	for (auto hint : CTFBotHint::AutoList()) {
		hint->SetOwnerEntity(nullptr);
	}
	for (auto hint : CTFBotHintSentrygun::AutoList()) {
		hint->SetOwnerEntity(nullptr);
	}
	for (auto hint : CTFBotHintTeleporterExit::AutoList()) {
		hint->SetOwnerEntity(nullptr);
	}
	
	// byte @ +0x54 = 0
}


void CTFBotManager::LevelShutdown()
{
	this->m_flQuotaTime = 0.0f;
	
	if (tf_bot_offline_practice.GetBool()) {
		this->RevertOfflinePracticeConvars();
		this->SetIsInOfflinePractice(false);
	}
}


void CTFBotManager::ClearStuckBotData()
{
	this->m_StuckBots.PurgeAndDeleteElements();
}

void CTFBotManager::DrawStuckBotData(float duration)
{
	if (engine->IsDedicatedServer())          return;
	if (!this->m_ctDrawStuckBots.IsElapsed()) return;
	
	this->m_ctDrawStuckBots.Start(duration);
	
	if (UTIL_GetListenServerHost() == nullptr) return;
	
	for (const CStuckBot *stuckbot : this->m_StuckBots) {
		for (const CStuckBotEvent *event : stuckbot->events) {
			NDebugOverlay::Cross3D(event->position, 5.0f, NB_RGB_YELLOW, true, duration);
			
			if (event->has_goal) {
				if (event->duration > 6.0f) {
					NDebugOverlay::HorzArrow(event->position, event->path_goal, 2.0f, NB_RGBA_RED,    true, duration);
				} else if (event->duration > 3.0f) {
					NDebugOverlay::HorzArrow(event->position, event->path_goal, 2.0f, NB_RGBA_YELLOW, true, duration);
				} else {
					NDebugOverlay::HorzArrow(event->position, event->path_goal, 2.0f, NB_RGBA_GREEN,  true, duration);
				}
			}
		}
		
		for (int i = 1; i < stuckbot->events.Count(); ++i) {
			NDebugOverlay::HorzArrow(stuckbot->events[i - 1]->position, stuckbot->events[i]->position, 3.0f, NB_RGBA_VIOLET, true, duration);
		}
		
		// POSSIBLE BUG: this invokes undefined behavior if stuckbot->events.IsEmpty() (...is this actually possible though?)
		NDebugOverlay::Text(stuckbot->events.Head()->position, CFmtStr("%s(#%d)", stuckbot->name, stuckbot->entindex), false, duration);
	}
}

const CStuckBot *CTFBotManager::FindOrCreateStuckBot(int entindex, const char *name)
{
	for (const CStuckBot *stuckbot : this->m_StuckBots) {
		if (entindex == stuckbot->entindex && FStrEq(name, stuckbot->name)) {
			return stuckbot;
		}
	}
	
	CStuckBot *stuckbot = new CStuckBot();
	stuckbot->entindex = entindex;
	V_strcpy_safe(stuckbot->name, name);
	
	this->m_StuckBots.AddToHead(stuckbot);
	return stuckbot;
}


bool CTFBotManager::IsInOfflinePractice() const
{
	return tf_bot_offline_practice.GetBool();
}

void CTFBotManager::SetIsInOfflinePractice(bool val)
{
	tf_bot_offline_practice.SetValue(val);
}


CTFBot *CTFBotManager::GetAvailableBotFromPool()
{
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
		if (bot == nullptr) continue;
		
		if ((bot->GetFlags() & FL_FAKECLIENT) == 0) continue; // why?
		
		if (bot->GetTeamNumber() == TEAM_SPECTATOR || bot->GetTeamNumber() == TEAM_UNASSIGNED) {
			bot->ClearAttribute(CTFBot::QUOTA_MANAGED);
			return bot;
		}
	}
	
	return nullptr;
}

#if 0
bool CTFBotManager::RemoveBotFromTeamAndKick(int teamnum)
{
	CUtlVector<CTFBot *> kickable;
	this->CollectKickableBots(&kickable, teamnum);
	
	CTFBot *bot = this->SelectBotToKick(&kickable);
	if (bot == nullptr) return false;
	
	if (bot->IsAlive()) {
		bot->CommitSuicide();
	}
	bot->ForceChangeTeam(TEAM_UNASSIGNED);
	
	UTIL_KickBotFromTeam(TEAM_UNASSIGNED);
	
	return true;
}
#endif


void CTFBotManager::OnForceAddedBots(int count)
{
	tf_bot_quota.SetValue(tf_bot_quota.GetInt() + count);
	this->m_flQuotaTime = gpGlobals->curtime + 1.0f;
}

void CTFBotManager::OnForceKickedBots(int count)
{
	tf_bot_quota.SetValue(Max(tf_bot_quota.GetInt() - count, 0));
	this->m_flQuotaTime = gpGlobals->curtime + 2.0f;
}


bool CTFBotManager::IsAllBotTeam(int teamnum)
{
	CTFTeam *team = GetGlobalTFTeam(teamnum);
	if (team == nullptr) return false;
	
	int num_players = team->GetNumPlayers();
	if (num_players == 0) {
		// BUG: this is probably going to do the wrong thing if GetAssignedHumanTeam is TEAM_ANY (-2)
		return (teamnum != TFGameRules()->GetAssignedHumanTeam());
	}
	
	for (int i = 0; i < num_players; ++i) {
		CTFPlayer *player = ToTFPlayer(team->GetPlayer(i));
		if (player == nullptr) continue;
		
		if (!player->IsBot()) return false;
	}
	
	return true;
}

bool CTFBotManager::IsMeleeOnly() const
{
	return tf_bot_melee_only.GetBool();
}

bool CTFBotManager::IsRandomizer() const
{
	return GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS );
}


void CTFBotManager::MaintainBotQuota()
{
	if (TheTFNavMesh->IsGenerating())                              return;
	if (TFGameRules() != nullptr && TFGameRules()->IsInTraining()) return;
	
	if (gpGlobals->curtime < this->m_flQuotaTime) return;
	this->m_flQuotaTime = gpGlobals->curtime + 0.25f;
	
	if (!engine->IsDedicatedServer() && UTIL_GetListenServerHost() == nullptr) return;
	
	int n_players_total = 0;
	
	int n_tfbots_total   = 0;
	int n_tfbots_playing = 0;
	int n_tfbots_spec    = 0;
	
	int n_humans_total   = 0;
	int n_humans_playing = 0;
	int n_humans_spec    = 0;
	
	ForEachPlayer([&](CBasePlayer *player){
		++n_players_total;
		
		int teamnum = player->GetTeamNumber();
		
		CTFBot *bot = ToTFBot(player);
		if (bot != nullptr && bot->HasAttribute(CTFBot::QUOTA_MANAGED)) {
			++n_tfbots_total;
			
			if (teamnum >= FIRST_GAME_TEAM && teamnum < GetNumberOfTeams()) {
				++n_tfbots_playing;
			} else if (teamnum == TEAM_SPECTATOR) {
				++n_tfbots_spec;
			}
		} else {
			++n_humans_total;
			
			if (teamnum >= FIRST_GAME_TEAM && teamnum < GetNumberOfTeams()) {
				++n_humans_playing;
			} else if (teamnum == TEAM_SPECTATOR) {
				++n_humans_spec;
			}
		}
		
		return true;
	});
	
	int n_desired = tf_bot_quota.GetInt();
	if (FStrEq(tf_bot_quota_mode.GetString(), "fill")) {
		n_desired = Max(0, n_desired - n_humans_playing);
	} else if (FStrEq(tf_bot_quota_mode.GetString(), "match")) {
		n_desired = (int)Max(0.0f, tf_bot_quota.GetFloat() * n_humans_playing);
	}
	
	if (tf_bot_join_after_player.GetBool() && n_humans_playing == 0) {
		n_desired = 0;
	}
	
	if (tf_bot_auto_vacate.GetBool()) {
		n_desired = Min(n_desired, gpGlobals->maxClients - (n_humans_total + 1));
	} else {
		n_desired = Min(n_desired, gpGlobals->maxClients - n_humans_total);
	}
	
	if (n_desired > n_tfbots_playing) {
		bool bWouldUnbalance = true;
		ForEachTFTeam([&](int team){
			if (!TFGameRules()->WouldChangeUnbalanceTeams(team, TEAM_UNASSIGNED)) {
				bWouldUnbalance = false;
				return false;
			}
			return true;
		});
		
		if (!bWouldUnbalance) {
			CTFBot *bot = this->GetAvailableBotFromPool();
			if (bot == nullptr) {
				bot = CreateTFBot();
			}
			
			if (bot != nullptr) {
				bot->SetAttribute(CTFBot::QUOTA_MANAGED);
				bot->HandleCommand_JoinTeam("auto");
				
				extern ConVar tf_bot_force_class;
				if (FStrEq(tf_bot_force_class.GetString(), "")) {
					bot->HandleCommand_JoinClass(bot->GetNextSpawnClassname());
				} else {
					bot->HandleCommand_JoinClass(tf_bot_force_class.GetString());
				}
			}
		}
	} else if (n_desired < n_tfbots_playing && !UTIL_KickBotFromTeam(TEAM_UNASSIGNED)) {
		CUtlVector<CTFTeam *> teams;
		ForEachTFTeam([&](int team){
			teams.AddToTail(GetGlobalTFTeam(team));
			return true;
		});
		
		/* shuffle the teams first to ensure no bias between equally-considered teams */
		teams.Shuffle();
		
		/* sort the teams such that we attempt to remove bots from the most heavily populated
		 * and highly scoring teams before teams with lesser players/scores */
		teams.Sort([](CTFTeam *const *lhs, CTFTeam *const *rhs) -> int {
			int lhs_players = (*lhs)->GetNumPlayers();
			int rhs_players = (*rhs)->GetNumPlayers();
			if (lhs_players > rhs_players) return -1;
			if (lhs_players < rhs_players) return  1;
			
			int lhs_score = (*lhs)->GetScore();
			int rhs_score = (*rhs)->GetScore();
			if (lhs_score > rhs_score) return -1;
			if (lhs_score < rhs_score) return  1;
			
			return 0;
		});
		
#if 0
		Msg("CTFBotManager::MaintainBotQuota\n");
		Msg("%-1s %-20s %-7s %-5s\n", "#", "NAME", "PLAYERS", "SCORE");
		for (auto team : teams) {
			Msg("%-1d %-20s %-7d %-5d\n", team->GetTeamNumber(), team->GetName(), team->GetNumPlayers(), team->GetScore());
		}
#endif
		
		for (auto team : teams) {
			if (UTIL_KickBotFromTeam(team->GetTeamNumber())) break;
		}
	}
}

void CTFBotManager::RevertOfflinePracticeConvars()
{
	tf_bot_quota           .Revert();
	tf_bot_quota_mode      .Revert();
	tf_bot_auto_vacate     .Revert();
	tf_bot_difficulty      .Revert();
	tf_bot_offline_practice.Revert();
}


#if 0
// UH, wait, wtf is this, did I invent this or what? where did this come from?
void CTFBotManager::CollectKickableBots(CUtlVector<CTFBot *> *bots, int teamnum)
{
	Assert(bots != nullptr);
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
		if (player == nullptr)         continue;
		if (FNullEnt(player->edict())) continue;
		if (!player->IsConnected())    continue;
		
		CTFBot *bot = ToTFBot(player);
		if (bot != nullptr && bot->HasAttribute(CTFBot::QUOTA_MANAGED) && bot->GetTeamNumber() == teamnum) {
			bots->AddToTail(bot);
		}
	}
}

// UH, wait, wtf is this, did I invent this or what? where did this come from?
CTFBot *CTFBotManager::SelectBotToKick(CUtlVector<CTFBot *> *bots)
{
	Assert(bots != nullptr);
	
	/* prefer dead bots */
	for (auto bot : *bots) {
		if (bot == nullptr) continue;
		if (bot->IsAlive()) continue;
		
		return bot;
	}
	
	/* if no dead bots, choose from all kickable bots */
	for (auto bot : *bots) {
		if (bot == nullptr) continue;
		
		return bot;
	}
	
	return nullptr;
}
#endif


CTFBotManager& TheTFBots()
{
	return assert_cast<CTFBotManager&>(TheNextBots());
}


CON_COMMAND_F(tf_bot_debug_stuck_log, "Given a server logfile, visually display bot stuck locations.", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	#pragma message("STUB: tf_bot_debug_stuck_log")
	DevMsg("STUB: tf_bot_debug_stuck_log\n");
	
	Assert(false);

#if 0
	if (!UTIL_IsCommandIssuedByServerAdmin()) return;
	
	if (args.ArgC() < 2) {
		DevMsg("%s <logfilename>\n", args[0]);
		return;
	}
	
	FileHandle_t hFile = filesystem->Open(args[1], "r", "GAME");
	
	TheTFBots().ClearStuckBotData();
	
	if (hFile == FILESYSTEM_INVALID_HANDLE) {
		Warning("Can't open file '%s'\n", args[1]);
		return;
	}
	
	while (!filesystem->EndOfFile(hFile)) {
		char line[0x400];
		filesystem->ReadLine(line, sizeof(line), hFile);
		
		// L MM/DD/YYYY - HH:MM:SS:
		(void)strtok(line,    ":");
		(void)strtok(nullptr, ":");
		(void)strtok(nullptr, ":");
		
		const char *tok = strtok(nullptr, " ");
		if (tok == nullptr) continue;
		
		// Loading map "MapName"
		if (strncmp(tok, "Loading", 8) == 0) {
			(void)strtok(nullptr, " ");
			
			const char *map = strtok(nullptr, "\"");
			if (map != nullptr) {
				Warning("*** Log file from map '%s'\n", map);
			}
			
			continue;
		}
		
		// "PlayerName<X><Y><Z>"
		if (tok[0] == '"') {
			if (tok[1] != '\0' && tok[1] != '<') {
				char *v13 = tok;
				do {
					++v13;
				} while (v13[0] != '<' && v13[0] != '\0');
				*v13 = '\0';
				
				
				
				// TODO
			} else {
				
				
				// TODO
			}
			
			// TODO
		}
	}
	
	filesystem->Close(hFile);
	
	// L 08/19/2016 - 10:44:50: "Scout<310><BOT><Blue>" stuck (position "28.18 3254.71 133.49") (duration "2.52") L 08/19/2016 - 10:44:50:    path_goal ( "150.00 3250.00 124.89" )
#endif
}

CON_COMMAND_F(tf_bot_debug_stuck_log_clear, "Clear currently loaded bot stuck data", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	if (!UTIL_IsCommandIssuedByServerAdmin()) return;
	
	TheTFBots().ClearStuckBotData();
}
