/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_generator.h"
#include "tf_gamerules.h"
#include "team.h"
#include "tf_bot_manager.h"


LINK_ENTITY_TO_CLASS(bot_generator, CTFBotGenerator);


BEGIN_DATADESC(CTFBotGenerator)
	
	DEFINE_KEYFIELD(m_spawnCount,              FIELD_INTEGER, "count"                 ),
	DEFINE_KEYFIELD(m_maxActiveCount,          FIELD_INTEGER, "maxActive"             ),
	DEFINE_KEYFIELD(m_spawnInterval,           FIELD_FLOAT,   "interval"              ),
	DEFINE_KEYFIELD(m_className,               FIELD_STRING,  "class"                 ),
	DEFINE_KEYFIELD(m_teamName,                FIELD_STRING,  "team"                  ),
	DEFINE_KEYFIELD(m_actionPointName,         FIELD_STRING,  "action_point"          ),
	DEFINE_KEYFIELD(m_initialCommand,          FIELD_STRING,  "initial_command"       ),
	DEFINE_KEYFIELD(m_bSuppressFire,           FIELD_BOOLEAN, "suppressFire"          ),
	DEFINE_KEYFIELD(m_bDisableDodge,           FIELD_BOOLEAN, "disableDodge"          ),
	DEFINE_KEYFIELD(m_iOnDeathAction,          FIELD_INTEGER, "actionOnDeath"         ),
	DEFINE_KEYFIELD(m_bUseTeamSpawnpoint,      FIELD_BOOLEAN, "useTeamSpawnPoint"     ),
	DEFINE_KEYFIELD(m_difficulty,              FIELD_INTEGER, "difficulty"            ),
	DEFINE_KEYFIELD(m_bRetainBuildings,        FIELD_BOOLEAN, "retainBuildings"       ),
	DEFINE_KEYFIELD(m_bSpawnOnlyWhenTriggered, FIELD_BOOLEAN, "spawnOnlyWhenTriggered"),
	DEFINE_KEYFIELD(m_spawnFlags,              FIELD_INTEGER, "spawnFlags"            ),
	// TODO: ensure that m_spawnFlags doesn't conflict with CBaseEntity::m_spawnFlags!
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable",                 InputEnable                ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable",                InputDisable               ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetSuppressFire",        InputSetSuppressFire       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetDisableDodge",        InputSetDisableDodge       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetDifficulty",          InputSetDifficulty         ),
	DEFINE_INPUTFUNC(FIELD_VOID, "CommandGotoActionPoint", InputCommandGotoActionPoint),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAttentionFocus",      InputSetAttentionFocus     ),
	DEFINE_INPUTFUNC(FIELD_VOID, "ClearAttentionFocus",    InputClearAttentionFocus   ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SpawnBot",               InputSpawnBot              ),
	DEFINE_INPUTFUNC(FIELD_VOID, "RemoveBots",             InputRemoveBots            ),
	
	DEFINE_OUTPUT(m_onSpawned,   "OnSpawned"  ),
	DEFINE_OUTPUT(m_onExpended,  "OnExpended" ),
	DEFINE_OUTPUT(m_onBotKilled, "OnBotKilled"),
	
	DEFINE_THINKFUNC(GeneratorThink),
	
END_DATADESC()


CTFBotGenerator::CTFBotGenerator()
{
	SetThink(nullptr);
}


void CTFBotGenerator::Activate()
{
	CPointEntity::Activate();
	
	if (FStrEq(STRING(this->m_className), "auto")) {
		this->m_bAutoClass = true;
	}
	
	this->m_hActionPoint = gEntList.FindEntityByName(nullptr, this->m_actionPointName);
}


void CTFBotGenerator::GeneratorThink()
{
	if ((TFGameRules()->State_Get() == GR_STATE_PREROUND || TFGameRules()->State_Get() == GR_STATE_RND_RUNNING) && !TFGameRules()->IsInWaitingForPlayers()) {
		if (!this->m_bSpawnOnlyWhenTriggered) {
			this->SpawnBot();
		}
	} else {
		this->SetNextThink(gpGlobals->curtime + 1.0f);
	}
}

void CTFBotGenerator::SpawnBot()
{
	FOR_EACH_VEC(this->m_Bots, i) {
		if (this->m_Bots[i] == nullptr || this->m_Bots[i]->GetTeamNumber() == TEAM_SPECTATOR) {
			this->m_Bots.FastRemove(i);
		}
	}
	
	if (this->m_Bots.Count() >= this->m_maxActiveCount) {
		this->SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}
	
	CTFBot *bot = TheTFBots().GetAvailableBotFromPool();
	if (bot == nullptr) {
		bot = CreateTFBot();
		if (bot == nullptr) return;
	}
	
	this->m_Bots.AddToTail(bot);
	
	bot->SetGenerator(this);
	
	if (!this->m_bUseTeamSpawnpoint) {
		bot->SetSpawnPoint(this);
	}
	
	if (this->m_bSuppressFire)    bot->SetAttribute(CTFBot::SUPPRESS_FIRE);
	if (this->m_bRetainBuildings) bot->SetAttribute(CTFBot::RETAIN_BUILDINGS);
	if (this->m_bDisableDodge)    bot->SetAttribute(CTFBot::DISABLE_DODGE);
	
	if (this->m_difficulty != -1) {
		bot->SetSkill(this->m_difficulty);
	}
	
	bot->SetIgnoreMask(static_cast<CTFBot::IgnoreMask>(this->m_spawnFlags));
	
	if (this->m_iOnDeathAction == REMOVE_ON_DEATH) {
		bot->SetAttribute(CTFBot::REMOVE_ON_DEATH);
	} else if (this->m_iOnDeathAction == BECOME_SPECTATOR_ON_DEATH) {
		bot->SetAttribute(CTFBot::BECOME_SPECTATOR_ON_DEATH);
	}
	
	bot->SetMovementGoal(dynamic_cast<CTFBotActionPoint *>(this->m_hActionPoint.Get()));
	
	const char *team_name = STRING(this->m_teamName);
	if (FStrEq(team_name, "auto")) {
		bot->ChangeTeam(bot->GetAutoTeam());
	} else if (FStrEq(team_name, "spectate")) {
		bot->ChangeTeam(TEAM_SPECTATOR);
	} else {
		int team_num = TEAM_INVALID;
		
		ForEachTFTeam([&](int team){
			if (FStrEq(team_name, g_aTeamNames[team])) {
				team_num = team;
				return false;
			}
			return true;
		});
		
		if (team_num != TEAM_INVALID) {
			bot->ChangeTeam(team_num);
		} else {
			bot->ChangeTeam(bot->GetAutoTeam());
		}
	}
	
	const char *class_name = STRING(this->m_className);
	if (this->m_bAutoClass) {
		class_name = bot->GetNextSpawnClassname();
	}
	bot->HandleCommand_JoinClass(class_name);
	
	if (TFGameRules()->IsInTraining()) {
		Assert(false);
		// TODO: some training-mode-specific name stuff
	}
	
	if (!bot->IsAlive()) {
		bot->ForceRespawn();
	}
	
	bot->SnapEyeAngles(this->GetAbsAngles());
	
	if (!FStrEq(STRING(this->m_initialCommand), "")) {
		bot->Update();
		bot->OnCommandString(STRING(this->m_initialCommand));
	}
	
	this->OnSpawned(bot);
	
	if (--this->m_spawnCount == 0) {
		SetThink(nullptr);
		this->OnExpended();
	} else {
		this->SetNextThink(gpGlobals->curtime + this->m_spawnInterval);
	}
}


void CTFBotGenerator::InputEnable(inputdata_t& inputdata)
{
	this->m_bEnabled = true;
	
	if (!this->m_bExpended) {
		SetThink(&CTFBotGenerator::GeneratorThink);
		
		// BUG: dunno wtf is going on here in the assembly code...
		this->SetNextThink(gpGlobals->curtime);
	}
}

void CTFBotGenerator::InputDisable(inputdata_t& inputdata)
{
	this->m_bEnabled = false;
	SetThink(nullptr);
}

void CTFBotGenerator::InputSetSuppressFire(inputdata_t& inputdata)
{
	this->m_bSuppressFire = inputdata.value.Bool();
}

void CTFBotGenerator::InputSetDisableDodge(inputdata_t& inputdata)
{
	this->m_bDisableDodge = inputdata.value.Bool();
}

void CTFBotGenerator::InputSetDifficulty(inputdata_t& inputdata)
{
	this->m_difficulty = inputdata.value.Int();
}

void CTFBotGenerator::InputCommandGotoActionPoint(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	auto action_point = dynamic_cast<CTFBotActionPoint *>(gEntList.FindEntityByName(nullptr, str));
	if (action_point == nullptr) return;
	
	FOR_EACH_VEC(this->m_Bots, i) {
		if (this->m_Bots[i] == nullptr || this->m_Bots[i]->GetTeamNumber() == TEAM_SPECTATOR) {
			this->m_Bots.FastRemove(i);
			continue;
		}
		
		this->m_Bots[i]->SetMovementGoal(action_point);
		this->m_Bots[i]->OnCommandString("goto action point");
	}
}

void CTFBotGenerator::InputSetAttentionFocus(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	CBaseEntity *ent = gEntList.FindEntityByName(nullptr, str);
	if (ent == nullptr) return;
	
	FOR_EACH_VEC(this->m_Bots, i) {
		if (this->m_Bots[i] == nullptr || this->m_Bots[i]->GetTeamNumber() == TEAM_SPECTATOR) {
			this->m_Bots.FastRemove(i);
			continue;
		}
		
		this->m_Bots[i]->SetAttentionFocus(ent);
	}
}

void CTFBotGenerator::InputClearAttentionFocus(inputdata_t& inputdata)
{
	FOR_EACH_VEC(this->m_Bots, i) {
		if (this->m_Bots[i] == nullptr || this->m_Bots[i]->GetTeamNumber() == TEAM_SPECTATOR) {
			this->m_Bots.FastRemove(i);
			continue;
		}
		
		this->m_Bots[i]->ClearAttentionFocus();
	}
}

void CTFBotGenerator::InputSpawnBot(inputdata_t& inputdata)
{
	if (this->m_bEnabled) {
		this->SpawnBot();
	}
}

void CTFBotGenerator::InputRemoveBots(inputdata_t& inputdata)
{
	FOR_EACH_VEC(this->m_Bots, i) {
		if (this->m_Bots[i] == nullptr) continue;
		
		this->m_Bots[i]->Remove();
		this->m_Bots[i]->Kick();
	}
	
	this->m_Bots.RemoveAll();
}


LINK_ENTITY_TO_CLASS(bot_action_point, CTFBotActionPoint);


BEGIN_DATADESC(CTFBotActionPoint)
	
	DEFINE_KEYFIELD(m_stayTime,            FIELD_FLOAT,  "stay_time"        ),
	DEFINE_KEYFIELD(m_desiredDistance,     FIELD_FLOAT,  "desired_distance" ),
	DEFINE_KEYFIELD(m_nextActionPointName, FIELD_STRING, "next_action_point"),
	DEFINE_KEYFIELD(m_command,             FIELD_STRING, "command"          ),
	
	DEFINE_OUTPUT(m_onReachedActionPoint, "OnBotReached"),
	
END_DATADESC()


void CTFBotActionPoint::Activate()
{
	CPointEntity::Activate();
	
	this->m_hNextActionPoint = gEntList.FindEntityByName(nullptr, STRING(this->m_nextActionPointName));
}


void CTFBotActionPoint::ReachedActionPoint(CTFBot *bot)
{
	if (!FStrEq(STRING(this->m_command), "")) {
		bot->OnCommandString(STRING(this->m_command));
	}
	
	this->m_onReachedActionPoint.FireOutput(bot, this);
}


bool CTFBotActionPoint::IsWithinRange(CBaseEntity *ent) const
{
	return (this->GetAbsOrigin().DistToSqr(ent->GetAbsOrigin()) <= Square(this->m_desiredDistance));
}
