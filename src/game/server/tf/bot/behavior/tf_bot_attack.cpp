/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_attack.h"
#include "tf_gamerules.h"


ActionResult<CTFBot> CTFBotAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ChasePath   .Initialize(actor);
	
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}

ActionResult<CTFBot> CTFBotAttack::Update(CTFBot *actor, float dt)
{
	bool is_melee = false;
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon != nullptr && (weapon->IsWeapon(TF_WEAPON_FLAMETHROWER) || weapon->IsMeleeWeapon())) {
		is_melee = true;
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat == nullptr || threat->IsObsolete() || actor->GetIntentionInterface()->ShouldAttack(actor, threat) == TRS_FALSE) {
		Done("No threat");
	}
	
	actor->EquipBestWeaponForThreat(threat);
	
	if (is_melee && threat->IsVisibleRecently() && actor->IsRangeLessThan(threat->GetLastKnownPosition(), 1.1f * actor->GetDesiredAttackRange())) {
		if (actor->TransientlyConsistentRandomValue(3.0f) < 0.5f) {
			actor->PressLeftButton();
		} else {
			actor->PressRightButton();
		}
	}
	
	if (threat->IsVisibleRecently() && !actor->IsRangeGreaterThan(threat->GetEntity()->GetAbsOrigin(), actor->GetDesiredAttackRange()) && actor->IsLineOfFireClear(threat->GetEntity()->EyePosition())) {
		Continue();
	}
	
	if (threat->IsVisibleRecently()) {
		this->m_ChasePath.Update(actor, threat->GetEntity(), CTFBotPathCost(actor, (is_melee && TFGameRules()->IsMannVsMachineMode() ? SAFEST_ROUTE : DEFAULT_ROUTE)));
		Continue();
	}
	
	this->m_ChasePath.Invalidate();
	
	if (actor->IsRangeLessThan(threat->GetLastKnownPosition(), 20.0f)) {
		actor->GetVisionInterface()->ForgetEntity(threat->GetEntity());
		Done("I lost my target!");
	}
	
	if (actor->IsRangeLessThan(threat->GetLastKnownPosition(), actor->GetMaxAttackRange())) {
		// BUG: HumanEyeHeight = 62.0f, from nav.h; value is from CS:S and is actually a bit wrong for TF2
		Vector eye = VecPlusZ(threat->GetLastKnownPosition(), HumanEyeHeight);
		actor->GetBodyInterface()->AimHeadTowards(eye, IBody::PRI_IMPORTANT, 0.2f, nullptr, "Looking towards where we lost sight of our victim");
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(3.0f, 5.0f));
		this->m_PathFollower.Compute(actor, threat->GetLastKnownPosition(), CTFBotPathCost(actor, (is_melee && TFGameRules()->IsMannVsMachineMode() ? SAFEST_ROUTE : DEFAULT_ROUTE)));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}
