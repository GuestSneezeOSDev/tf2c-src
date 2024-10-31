/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_attack_flag_defenders.h"
#include "tf_bot_escort_flag_carrier.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"


ConVar tf_bot_flag_escort_range("tf_bot_flag_escort_range", "500", FCVAR_CHEAT);


CTFBotAttackFlagDefenders::CTFBotAttackFlagDefenders(float duration)
{
	if (duration > 0.0f) {
		this->m_ctActionDuration.Start(duration);
	} else {
		this->m_ctActionDuration.Invalidate();
	}
}


ActionResult<CTFBot> CTFBotAttackFlagDefenders::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctRecomputePath.Invalidate();
	
	this->m_hTarget = nullptr;
	
	return CTFBotAttack::OnStart(actor, action);
}

ActionResult<CTFBot> CTFBotAttackFlagDefenders::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctCheckFlag.IsElapsed() && this->m_ctActionDuration.IsElapsed()) {
		this->m_ctCheckFlag.Start(RandomFloat(1.0f, 3.0f));
		
		CCaptureFlag *flag = actor->GetFlagToFetch();
		if (flag == nullptr) {
			Done("No flag");
		}
		
		if (!(TFGameRules()->IsMannVsMachineMode() && flag->IsHome())) {
			CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
			if (carrier == nullptr) {
				Done("Flag was dropped");
			}
			
			if (actor->IsSelf(carrier)) {
				Done("I picked up the flag!");
			}
			
			CTFBot *carrier_bot = ToTFBot(carrier);
			bool carrier_in_squad = (carrier_bot != nullptr && carrier_bot->IsInASquad());
			
			if (!carrier_in_squad) {
				extern ConVar tf_bot_flag_escort_max_count;
				if (actor->IsRangeLessThan(carrier, tf_bot_flag_escort_range.GetFloat()) && GetBotEscortCount(actor->GetTeamNumber()) < tf_bot_flag_escort_max_count.GetInt()) {
					ChangeTo(new CTFBotEscortFlagCarrier(), "Near flag carrier - escorting");
				}
			}
		}
	}
	
	auto result = CTFBotAttack::Update(actor, dt);
	if (result.transition != ACT_T_DONE) {
		Continue();
	}
	
	if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive() || !actor->IsRandomReachableEnemyStillValid(this->m_hTarget)) {
		CTFPlayer *target = actor->SelectRandomReachableEnemy();
		if (target == nullptr) {
			ChangeTo(new CTFBotEscortFlagCarrier(), "No reachable victim - escorting flag");
		}
		
		this->m_hTarget = target;
	}
	
	// BUG: this seems REALLY sketchy; does this actually go through IsIgnored / IsVisibleEntityNoticed?
	actor->GetVisionInterface()->AddKnownEntity(this->m_hTarget);
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
		this->m_PathFollower.Compute(actor, this->m_hTarget, CTFBotPathCost(actor, DEFAULT_ROUTE));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


bool CTFBotAttackFlagDefenders::IsPossible(CTFBot *actor)
{
	return (actor->SelectRandomReachableEnemy() != nullptr);
}

// TODO: not only does SelectRandomReachableEnemy need to filter out stuff like cloaked spies,
// but we also need to periodically re-evaluate those things in the action Update routine
