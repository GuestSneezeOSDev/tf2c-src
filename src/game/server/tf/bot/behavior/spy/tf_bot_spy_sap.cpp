/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_spy_sap.h"
#include "tf_bot_spy_attack.h"


ActionResult<CTFBot> CTFBotSpySap::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->LookAroundForEnemies_Set(actor, false);
	
	if (actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotSpySap::Update(CTFBot *actor, float dt)
{
	CBaseObject *nearest = actor->GetNearestKnownSappableTarget();
	if (nearest != nullptr) {
		this->m_hTarget = nearest;
	}
	
	if (this->m_hTarget == nullptr) {
		Done("Sap target gone");
	}
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	CTFPlayer *engie = nullptr;
	for (const auto& known : knowns) {
		CTFPlayer *player = ToTFPlayer(known.GetEntity());
		if (player == nullptr) continue;
		
		if (player->IsPlayerClass(TF_CLASS_ENGINEER, true) && actor->IsEnemy(player)) {
			engie = player;
			break;
		}
	}
	
	if (engie != nullptr && this->m_hTarget->GetBuilder() == engie && actor->IsRangeLessThan(engie, 150.0f) && actor->IsEntityBetweenTargetAndSelf(engie, this->m_hTarget)) {
		SuspendFor(new CTFBotSpyAttack(engie), "Backstabbing the engineer before I sap his buildings");
	}
	
	if (actor->IsRangeLessThan(this->m_hTarget, 80.0f)) {
		CTFWeaponBase *sapper = actor->GetTFWeapon_Building();
		if (sapper == nullptr) {
			Done("I have no sapper");
		}
		
		actor->Weapon_Switch(sapper);
		
		if (actor->m_Shared.IsStealthed()) {
			actor->PressAltFireButton();
		}
		
		actor->GetBodyInterface()->AimHeadTowards(this->m_hTarget, IBody::PRI_OVERRIDE, 0.1f, nullptr, "Aiming my sapper");
		actor->PressFireButton();
	}
	
	if (actor->IsRangeGreaterThan(this->m_hTarget, 40.0f)) {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			if (!this->m_PathFollower.Compute(actor, this->m_hTarget, CTFBotPathCost(actor, FASTEST_ROUTE))) {
				Done("No path to sap target!");
			}
		}
		
		this->m_PathFollower.Update(actor);
		
		Continue();
	}
	
	if (this->m_hTarget->HasSapper()) {
		nearest = actor->GetNearestKnownSappableTarget();
		if (nearest != nullptr) {
			this->m_hTarget = nearest;
		} else {
			if (engie != nullptr) {
				SuspendFor(new CTFBotSpyAttack(engie), "Attacking an engineer");
			} else {
				Done("All targets sapped");
			}
		}
	}
	
	Continue();
}

void CTFBotSpySap::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	this->LookAroundForEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotSpySap::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	this->LookAroundForEnemies_Reset(actor);
	Continue();
}

ActionResult<CTFBot> CTFBotSpySap::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->LookAroundForEnemies_Set(actor, false);
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSpySap::OnStuck(CTFBot *actor)
{
	Done("I'm stuck, probably on a sapped building that hasn't exploded yet", SEV_CRITICAL);
}


ThreeState_t CTFBotSpySap::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	Assert(actor != nullptr);
	
	if (this->m_hTarget != nullptr && !this->m_hTarget->HasSapper()) {
		return TRS_FALSE;
	}
	
	if (actor->m_Shared.InCond(TF_COND_DISGUISED) || actor->m_Shared.InCond(TF_COND_DISGUISING) || actor->m_Shared.IsStealthed()) {
		return (this->AreAllDangerousSentriesSapped(actor) ? TRS_TRUE : TRS_FALSE);
	} else {
		return TRS_TRUE;
	}
}

ThreeState_t CTFBotSpySap::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	if (this->m_hTarget != nullptr && nextbot->IsRangeLessThan(this->m_hTarget, 300.0f)) {
		return TRS_FALSE;
	}
	
	return TRS_NONE;
}


bool CTFBotSpySap::AreAllDangerousSentriesSapped(CTFBot *actor) const
{
	// TODO: is there any 'for each known entity' thing we could do here instead of collecting into a vector?
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	for (const auto& known : knowns) {
		auto obj = dynamic_cast<CBaseObject *>(known.GetEntity());
		if (obj == nullptr) continue;
		
		if (!obj->IsSentry())     continue;
		if (obj->HasSapper())     continue;
		if (!actor->IsEnemy(obj)) continue;
		
		auto sentry = assert_cast<CObjectSentrygun *>(obj);
		if (actor->IsRangeLessThan(actor, sentry->GetMaxRange()) && actor->IsLineOfFireClear(sentry)) {
			return false;
		}
	}
	
	return true;
}
