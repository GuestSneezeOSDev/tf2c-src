/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_melee_attack.h"


static ConVar tf_bot_melee_attack_abandon_range("tf_bot_melee_attack_abandon_range", "500", FCVAR_CHEAT, "If threat is farther away than this, bot will switch back to its primary weapon and attack");


CTFBotMeleeAttack::CTFBotMeleeAttack(float abandon_range)
{
	if (abandon_range < 0.0f) {
		this->m_flAbandonRange = tf_bot_melee_attack_abandon_range.GetFloat();
	} else {
		this->m_flAbandonRange = abandon_range;
	}
}


ActionResult<CTFBot> CTFBotMeleeAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.Initialize(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotMeleeAttack::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	// TODO: possibly add additional threat obsolescence checks like in CTFBotAttack::Update
	if (threat == nullptr) {
		Done("No threat");
	}
	
	if ((threat->GetLastKnownPosition() - actor->GetAbsOrigin()).IsLengthGreaterThan(this->m_flAbandonRange)) {
		Done("Threat is too far away for a melee attack");
	}
	
	actor->SwitchToMelee();
	actor->PressFireButton();
	
	// BUG: does this thing really recompute the path every single update?
	this->m_ChasePath.Update(actor, threat->GetEntity(), CTFBotPathCost(actor, FASTEST_ROUTE));
	
	Continue();
}
