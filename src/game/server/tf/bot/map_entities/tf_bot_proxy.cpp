/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_proxy.h"
#include "tf_bot.h"
#include "tf_bot_generator.h"


LINK_ENTITY_TO_CLASS(bot_proxy, CTFBotProxy);


BEGIN_DATADESC(CTFBotProxy)
	
	DEFINE_KEYFIELD(m_botName,         FIELD_STRING,  "bot_name"        ),
	DEFINE_KEYFIELD(m_className,       FIELD_STRING,  "class"           ),
	DEFINE_KEYFIELD(m_teamName,        FIELD_STRING,  "team"            ),
	DEFINE_KEYFIELD(m_respawnInterval, FIELD_FLOAT,   "respawn_interval"),
	DEFINE_KEYFIELD(m_actionPointName, FIELD_STRING,  "action_point"    ),
	DEFINE_KEYFIELD(m_spawnOnStart,    FIELD_INTEGER, "spawn_on_start"  ),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "SetTeam",         InputSetTeam        ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetClass",        InputSetClass       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetMovementGoal", InputSetMovementGoal),
	DEFINE_INPUTFUNC(FIELD_VOID, "Spawn",           InputSpawn          ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Delete",          InputDelete         ),
	
	DEFINE_OUTPUT(m_onSpawned,        "OnSpawned"       ),
	DEFINE_OUTPUT(m_onInjured,        "OnInjured"       ),
	DEFINE_OUTPUT(m_onKilled,         "OnKilled"        ),
	DEFINE_OUTPUT(m_onAttackingEnemy, "OnAttackingEnemy"),
	DEFINE_OUTPUT(m_onKilledEnemy,    "OnKilledEnemy"   ),
	
END_DATADESC()


CTFBotProxy::CTFBotProxy()
{
	V_strcpy_safe(this->m_botName,   "TFBot");
	V_strcpy_safe(this->m_className, "auto");
	V_strcpy_safe(this->m_teamName,  "auto");
}


void CTFBotProxy::InputSetTeam(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	if (str == nullptr) return;
	if (str[0] == '\0') return;
	
	V_strcpy_safe(this->m_teamName, str);
	
	if (this->m_hBot != nullptr) {
		this->m_hBot->HandleCommand_JoinTeam(this->m_teamName);
	}
}

void CTFBotProxy::InputSetClass(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	if (str == nullptr) return;
	if (str[0] == '\0') return;
	
	V_strcpy_safe(this->m_className, str);
	
	if (this->m_hBot != nullptr) {
		this->m_hBot->HandleCommand_JoinClass(this->m_className);
	}
}

void CTFBotProxy::InputSetMovementGoal(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	if (str == nullptr) return;
	if (str[0] == '\0') return;
	
	this->m_hActionPoint = dynamic_cast<CTFBotActionPoint *>(gEntList.FindEntityByName(nullptr, str));
	
	if (this->m_hBot != nullptr) {
		this->m_hBot->SetMovementGoal(this->m_hActionPoint);
	}
}

void CTFBotProxy::InputSpawn(inputdata_t& inputdata)
{
	CTFBot *bot = CreateTFBot(this->m_botName);
	this->m_hBot = bot;
	
	if (bot != nullptr) {
		bot->SetSpawnPoint(this);
		
		bot->SetAttribute(CTFBot::REMOVE_ON_DEATH);
		bot->SetAttribute(CTFBot::PROXY_MANAGED);
		
		bot->SetMovementGoal(bot);
		
		bot->HandleCommand_JoinTeam (this->m_teamName);
		bot->HandleCommand_JoinClass(this->m_className);
		
		this->OnSpawned();
	}
}

void CTFBotProxy::InputDelete(inputdata_t& inputdata)
{
	if (this->m_hBot != nullptr) {
		this->m_hBot->Kick();
		this->m_hBot = nullptr;
	}
}
