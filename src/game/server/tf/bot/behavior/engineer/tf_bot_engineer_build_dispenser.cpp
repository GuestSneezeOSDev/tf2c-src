/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_build_dispenser.h"
#include "tf_bot_get_ammo.h"
#include "tf_nav_mesh.h"
#include "tf_weapon_builder.h"


class PressFireButtonIfValidBuildPositionReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override
	{
		if (this->m_hBuilder == nullptr)           return;
		if (!this->m_hBuilder->IsValidPlacement()) return;
		
		auto input = dynamic_cast<INextBotPlayerInput *>(nextbot->GetEntity());
		if (input != nullptr) input->PressFireButton();
	}
	
	void SetBuilder(CTFWeaponBuilder *builder) { this->m_hBuilder = builder; }
	
private:
	CHandle<CTFWeaponBuilder> m_hBuilder;
};


ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Building() != nullptr && typeid(actor->GetTFWeapon_Building()) == typeid(CTFWeaponBuilder));
	
	this->m_PathFollower.Initialize(actor);
	
	this->m_nAttempts = 3;
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildDispenser::Update(CTFBot *actor, float dt)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		Done("Ouch! I'm under attack");
	}
	
	auto sentry = assert_cast<CObjectSentrygun *>(actor->GetObjectOfType(OBJ_SENTRYGUN));
	if (sentry == nullptr) {
		Done("No Sentry");
	}
	
	if (sentry->GetTimeSinceLastInjury() < 1.0f || sentry->GetFloatHealth() < sentry->GetMaxHealth()) {
		Done("Need to repair my Sentry");
	}
	
	if (actor->GetObjectOfType(OBJ_DISPENSER) != nullptr) {
		Done("Dispenser built");
	}

	if (actor->GetObjectOfType(OBJ_MINIDISPENSER) != nullptr) {
		Done("Dispenser built");
	}

	if (this->m_nAttempts <= 0) {
		Done("Can't find a place to build a Dispenser");
	}
	
	if (actor->CanBuild(OBJ_DISPENSER) == CB_NEED_RESOURCES || actor->CanBuild(OBJ_MINIDISPENSER) == CB_NEED_RESOURCES) {
		if (this->m_ctGetAmmo.IsElapsed() && CTFBotGetAmmo::IsPossible(actor)) {
			this->m_ctGetAmmo.Start(1.0f);
			SuspendFor(new CTFBotGetAmmo(), "Need more metal to build");
		}
	}
	
	// TODO: source of magic number 71.0f; is that the height of the dispenser model or something?
	// or is it eye height of engineer? or some class?
	Vector where = VecPlusZ(sentry->GetAbsOrigin() - (75.0f * sentry->BodyDirection2D()), 71.0f);
	TheTFNavMesh->GetSimpleGroundHeight(where, &where.z);
	
	if (where.DistToSqr(actor->GetAbsOrigin()) < Square(100.0f)) {
		actor->PressCrouchButton();
	}
	
	if (where.DistToSqr(actor->GetAbsOrigin()) > Square(25.0f)) {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			this->m_PathFollower.Compute(actor, where, CTFBotPathCost(actor, FASTEST_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
		
		Continue();
	}
	
	auto builder = dynamic_cast<CTFWeaponBuilder *>(actor->GetActiveTFWeapon());
	if (builder != nullptr && (builder->GetType() == OBJ_DISPENSER || builder->GetType() == OBJ_MINIDISPENSER) && builder->m_hObjectBeingBuilt != nullptr) {
		if (this->m_ctPlacementAim.IsElapsed()) {
			static PressFireButtonIfValidBuildPositionReply buildReply;
			buildReply.SetBuilder(builder);
			
			Vector dir = VectorXY((sentry->GetAbsOrigin() - actor->GetAbsOrigin()).Normalized());
			VectorYawRotate(dir, RandomFloat(-90.0f, 90.0f), dir);
			
			actor->GetBodyInterface()->AimHeadTowards(actor->EyePosition() + (100.0f * dir), IBody::PRI_CRITICAL, 1.0f, &buildReply, "Trying to place my dispenser");
			
			this->m_ctPlacementAim.Start(1.0f);
			
			--this->m_nAttempts;
		}
	} else {
		actor->StartBuildingObjectOfType(OBJ_DISPENSER);
	}
	
	Continue();
}

void CTFBotEngineerBuildDispenser::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO: figure out why the hell this is only done in this particular engie build AI action
	// (in fact, why are these actions all so different from each other? the differences make very very little sense)
	actor->GetBodyInterface()->ClearPendingAimReply();
}


ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	actor->GetBodyInterface()->ClearPendingAimReply();
	
	Continue();
}
