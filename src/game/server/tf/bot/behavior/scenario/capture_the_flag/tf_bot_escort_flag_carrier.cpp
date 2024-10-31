/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_escort_flag_carrier.h"
#include "tf_bot_attack_flag_defenders.h"
#include "entity_capture_flag.h"


static ConVar tf_bot_flag_escort_give_up_range("tf_bot_flag_escort_give_up_range", "1000", FCVAR_CHEAT);
       ConVar tf_bot_flag_escort_max_count    ("tf_bot_flag_escort_max_count",        "4", FCVAR_CHEAT);


ActionResult<CTFBot> CTFBotEscortFlagCarrier::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctRecomputePath.Invalidate();
	
	// BUG: need to call this->m_MeleeAttack.OnStart() here!
	
	Continue();
}

ActionResult<CTFBot> CTFBotEscortFlagCarrier::Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if (flag == nullptr) {
		Done("No flag");
	}
	
	CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
	if (carrier == nullptr) {
		Done("Flag was dropped");
	}
	
	if (actor->IsSelf(carrier)) {
		Done("I picked up the flag!");
	}
	
	/* avoid double-calling CTFBotAttackFlagDefenders::IsPossible, as it's expensive */
	bool attackdefenders_checked  = false;
	bool attackdefenders_possible = false;
	auto l_AttackFlagDefenders_IsPossible = [&]{
		if (!attackdefenders_checked) {
			attackdefenders_possible = CTFBotAttackFlagDefenders::IsPossible(actor);
			attackdefenders_checked  = true;
		}
		return attackdefenders_possible;
	};
	
	if (actor->IsRangeGreaterThan(carrier, tf_bot_flag_escort_give_up_range.GetFloat()) && l_AttackFlagDefenders_IsPossible()) {
		ChangeTo(new CTFBotAttackFlagDefenders(), "Too far from flag carrier - attack defenders!");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	extern ConVar tf_bot_flag_escort_range;
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon != nullptr && weapon->IsMeleeWeapon() && actor->IsRangeLessThan(carrier, tf_bot_flag_escort_range.GetFloat()) && actor->IsLineOfSightClear(carrier, CBaseCombatCharacter::IGNORE_NOTHING)) {
		auto result = this->m_MeleeAttack.Update(actor, dt);
		if (result.transition == ACT_T_CONTINUE) {
			Continue();
		}
	}
	
	if (!actor->IsRangeGreaterThan(carrier, 0.5f * tf_bot_flag_escort_range.GetFloat())) {
		Continue();
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		if (GetBotEscortCount(actor->GetTeamNumber()) > tf_bot_flag_escort_max_count.GetInt() && l_AttackFlagDefenders_IsPossible()) {
			// NOTE: live TF2 does a Done transition here, which would land us back in CTFBotFetchFlag...
			// this seems wrong, so we instead do a ChangeTo CTFBotAttackFlagDefenders, like in the too-far-away case above
			ChangeTo(new CTFBotAttackFlagDefenders(), "Too many flag escorts - attack defenders!");
		}
		
		this->m_PathFollower.Compute(actor, carrier, CTFBotPathCost(actor, FASTEST_ROUTE));
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


int GetBotEscortCount(int teamnum)
{
	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers(&teammates, teamnum, true);
	
	int count = 0;
	for (auto teammate : teammates) {
		CTFBot *bot = ToTFBot(teammate);
		if (bot == nullptr) continue;
		
		auto behavior = bot->GetBehavior();
		if (behavior == nullptr) continue;
		
		auto action = behavior->FirstContainedResponder();
		if (action == nullptr) continue;
		
		while (action->FirstContainedResponder() != nullptr) {
			action = action->FirstContainedResponder();
		}
		
		if (assert_cast<Action<CTFBot> *>(action)->IsNamed("EscortFlagCarrier")) {
			++count;
		}
	}
	
	return count;
}
