/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_dead.h"
#include "tf_bot_behavior.h"


ActionResult<CTFBot> CTFBotDead::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_itDeathEpoch.Start();
	
	Continue();
}

ActionResult<CTFBot> CTFBotDead::Update(CTFBot *actor, float dt)
{
	if (actor->IsAlive()) {
		ChangeTo(new CTFBotMainAction(), "This should not happen!");
	}
	
	if (!this->m_itDeathEpoch.IsGreaterThan(5.0f)) {
		Continue();
	}
	
	if (actor->HasAttribute(CTFBot::REMOVE_ON_DEATH)) {
		actor->Kick();
		Continue();
	}
	
	if (actor->HasAttribute(CTFBot::BECOME_SPECTATOR_ON_DEATH)) {
		actor->ChangeTeam(TEAM_SPECTATOR, false, true);
		Done();
	}
	
	Continue();
}
