/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_build_teleport_exit.h"
#include "tf_bot_get_ammo.h"
#include "tf_obj_teleporter.h"
#include "tf_weapon_builder.h"


ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Building() != nullptr && typeid(actor->GetTFWeapon_Building()) == typeid(CTFWeaponBuilder));
	
	this->m_PathFollower.Initialize(actor);
	
	if (!this->m_bPrecise) {
		this->m_vecPosition = actor->GetAbsOrigin();
	}
	
	this->m_ctImpatience.Start(3.1f);
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::Update(CTFBot *actor, float dt)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		Done("Ouch! I'm under attack");
	}
	
	if (actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT) != nullptr) {
		Done("Teleport exit built");
	}
	
	if (actor->CanBuild(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT) == CB_NEED_RESOURCES) {
		if (this->m_ctGetAmmo.IsElapsed() && CTFBotGetAmmo::IsPossible(actor)) {
			this->m_ctGetAmmo.Start(1.0f);
			SuspendFor(new CTFBotGetAmmo(), "Need more metal to build my Teleporter Exit");
		}
	}
	
	if (actor->IsRangeGreaterThan(this->m_vecPosition, 50.0f)) {
		if (!this->m_PathFollower.IsValid() && this->m_ctRecomputePath.IsElapsed()) {
			this->m_PathFollower.Compute(actor, this->m_vecPosition, CTFBotPathCost(actor, FASTEST_ROUTE));
			this->m_ctRecomputePath.Start(RandomFloat(2.0f, 3.0f));
		}
		
		this->m_PathFollower.Update(actor);
		this->m_ctImpatience.Reset();
		
		Continue();
	}
	
	if (this->m_ctImpatience.IsElapsed()) {
		Done("Taking too long - giving up");
	}
	
	if (this->m_bPrecise) {
		actor->GetBodyInterface()->AimHeadTowards(this->m_vecPosition, IBody::PRI_CRITICAL, 1.0f, nullptr, "Looking toward my precise build location");
		
		auto tele = assert_cast<CObjectTeleporter *>(CreateEntityByName("obj_teleporter"));
		if (tele != nullptr) {
			tele->SetObjectMode(TELEPORTER_TYPE_EXIT);
			tele->SetAbsOrigin(this->m_vecPosition);
			tele->SetAbsAngles(QAngle(0.0f, this->m_flYaw, 0.0f));
			tele->Spawn();
			
			tele->StartPlacement(actor);
			tele->StartBuilding(actor);
			tele->SetBuilder(actor);
			
			actor->SetAbsOrigin(VecPlusZ(actor->GetAbsOrigin(), actor->GetLocomotionInterface()->GetStepHeight()));
			
			Done("Teleport exit built at precise location");
		}
	} else {
		// TODO: maybe do this check via weapon ID rather than dynamic_cast?
		// (see also: CTFBotEngineerBuildTeleportEntrance::Update)
		auto builder = dynamic_cast<CTFWeaponBuilder *>(actor->GetActiveTFWeapon());
		if (builder != nullptr) {
			if (builder->IsValidPlacement()) {
				actor->PressFireButton();
			} else {
				if (this->m_ctPlacementAim.IsElapsed()) {
					Vector random; random.z = 0.0f;
					FastSinCos(RandomFloat(-M_PI_F, M_PI_F), &random.x, &random.y);
					
					// TODO: WTF is this even doing? does this even make sense, trig-wise? maybe it does...?
					// it seems like CTFBotEngineerBuildDispenser::Update does something more sane
					
					actor->GetBodyInterface()->AimHeadTowards(actor->EyePosition() - (100.0f * random), IBody::PRI_CRITICAL, 1.0f, nullptr, "Trying to place my teleport exit");
					
					this->m_ctPlacementAim.Start(1.0f);
				}
			}
		} else {
			actor->StartBuildingObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT);
		}
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctImpatience.Reset();
	this->m_PathFollower.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	Continue();
}
