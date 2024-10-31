/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_medic_retreat.h"
#include "tf_nav_area.h"


class CUsefulHealTargetFilter : public INextBotEntityFilter
{
public:
	CUsefulHealTargetFilter(CTFBot *bot) : m_pBot(bot) {}
	
	virtual bool IsAllowed(CBaseEntity *ent) const override
	{
		CTFPlayer *player = ToTFPlayer(ent);
		if (player == nullptr) return false;
		
		if (!this->m_pBot->IsFriend(ent)) {
			return false;
		}
		
		// TODO: re-evaluate this
		switch (player->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SCOUT:        return true;
		case TF_CLASS_SNIPER:       return false;
		case TF_CLASS_SOLDIER:      return true;
		case TF_CLASS_DEMOMAN:      return true;
		case TF_CLASS_MEDIC:        return false;
		case TF_CLASS_HEAVYWEAPONS: return true;
		case TF_CLASS_PYRO:         return true;
		case TF_CLASS_SPY:          return true;
		case TF_CLASS_ENGINEER:     return true;
		
		case TF_CLASS_CIVILIAN:     return true;
		
		default:                    return true;
		}
	}
	
private:
	CTFBot *m_pBot;
};


ActionResult<CTFBot> CTFBotMedicRetreat::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Primary() != nullptr && actor->GetTFWeapon_Primary()->IsWeapon(TF_WEAPON_SYRINGEGUN_MEDIC));
	
	this->m_PathFollower.Initialize(actor);
	
	this->m_ctLookForPatients.Invalidate();
	
	if (actor->GetHomeArea() == nullptr) {
		Done("No home area!");
	}
	
	this->m_PathFollower.Compute(actor, actor->GetHomeArea()->GetCenter(), CTFBotPathCost(actor, FASTEST_ROUTE));
	
	Continue();
}

ActionResult<CTFBot> CTFBotMedicRetreat::Update(CTFBot *actor, float dt)
{
	// TODO: rework this to accomodate *any* combat weapon, e.g. crossbow
	// TODO: also, maybe don't assume that primary is necessarily a combat weapon
	// (if it isn't, then perhaps melee would be a better choice...?)
	CTFWeaponBase *weapon = actor->m_Shared.GetActiveTFWeapon();
	if (weapon != nullptr && !weapon->IsWeapon(TF_WEAPON_SYRINGEGUN_MEDIC)) {
		actor->SwitchToPrimary();
	}
	
	this->m_PathFollower.Update(actor);
	
	if (this->m_ctLookForPatients.IsElapsed()) {
		this->m_ctLookForPatients.Start(RandomFloat(0.33f, 1.00f));
		
		// TODO: what the hell is this
		actor->GetBodyInterface()->AimHeadTowards(actor->EyePosition() + AngleVecFwd(QAngle(0.0f, RandomFloat(-180.0f, 180.0f), 0.0f)), IBody::PRI_IMPORTANT, 0.1f, nullptr, "Looking for someone to heal");
	}
	
	if (actor->GetVisionInterface()->GetClosestKnown(CUsefulHealTargetFilter(actor)) != nullptr) {
		Done("I know of a teammate");
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotMedicRetreat::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Compute(actor, actor->GetHomeArea()->GetCenter(), CTFBotPathCost(actor, FASTEST_ROUTE));
	Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Compute(actor, actor->GetHomeArea()->GetCenter(), CTFBotPathCost(actor, FASTEST_ROUTE));
	Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Compute(actor, actor->GetHomeArea()->GetCenter(), CTFBotPathCost(actor, FASTEST_ROUTE));
	Continue();
}
