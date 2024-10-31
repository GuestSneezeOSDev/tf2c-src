/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_build_sentrygun.h"
#include "tf_bot_get_ammo.h"
#include "tf_obj_sentrygun.h"
#include "tf_sentrygun.h"
#include "tf_weapon_builder.h"


ActionResult<CTFBot> CTFBotEngineerBuildSentrygun::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Building() != nullptr && typeid(actor->GetTFWeapon_Building()) == typeid(CTFWeaponBuilder));
	
	this->m_PathFollower.Initialize(actor);
	
	this->m_nAttempts = 5;
	
//	this->m_ct0040.Invalidate();
	this->m_ctPlacementDir.Invalidate();
	
	this->m_nDir  = 1;
//	this->m_b485c = true;
	
	if (this->m_pHint != nullptr) {
		this->m_vecPosition = this->m_pHint->GetAbsOrigin();
	} else {
		this->m_vecPosition = actor->GetAbsOrigin();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildSentrygun::Update(CTFBot *actor, float dt)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		Done("Ouch! I'm under attack");
	}
	
	if (actor->GetObjectOfType(OBJ_SENTRYGUN) != nullptr) {
		Done("Sentry built");
	}
	
	if (actor->CanBuild(OBJ_SENTRYGUN) == CB_NEED_RESOURCES) {
		if (this->m_ctGetAmmo.IsElapsed() && CTFBotGetAmmo::IsPossible(actor)) {
			this->m_ctGetAmmo.Start(1.0f);
			SuspendFor(new CTFBotGetAmmo(), "Need more metal to build my Sentry");
		}
	}
	
	if (actor->IsRangeGreaterThan(this->m_vecPosition, 25.0f)) {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			this->m_PathFollower.Compute(actor, this->m_vecPosition, CTFBotPathCost(actor, FASTEST_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
		
		if (!this->m_PathFollower.IsValid()) {
			Done("Path failed");
		}
		
		Continue();
	}
	
	if (this->m_nAttempts <= 0) {
		Done("Couldn't find a place to build Sentry");
	}
	
	if (this->m_pHint != nullptr) {
		auto sentry = assert_cast<CObjectSentrygun *>(CreateEntityByName("obj_sentrygun"));
		if (sentry != nullptr) {
			// ++this->m_pHint->field_370
			
			sentry->SetAbsOrigin(this->m_pHint->GetAbsOrigin());
			sentry->SetAbsAngles(QAngle(0.0f, this->m_pHint->GetAbsAngles().y, 0.0f));
			sentry->Spawn();
			
			sentry->StartPlacement(actor);
			sentry->StartBuilding(actor);
		}
	} else {
		auto builder = dynamic_cast<CTFWeaponBuilder *>(actor->GetActiveTFWeapon());
		if (builder != nullptr && builder->GetType() == OBJ_SENTRYGUN && builder->m_hObjectBeingBuilt != nullptr) {
			// this block makes zero sense; clearly it's incomplete
			// TODO: look at it in disassembly of other platforms' builds
		//	if (this->m_b485c) {
		//		if (actor->GetLastKnownTFArea() != nullptr) {
		//			CTFBotPathCost cost_func;
		//			(void)actor->GetTeamNumber();
		//		}
		//	}
			
			if (actor->GetBodyInterface()->IsHeadSteady()) {
				if (builder->IsValidPlacement()) {
					actor->PressFireButton();
				} else {
					if (this->m_ctPlacementDir.IsElapsed()) {
					//	this->m_b485c = true;
						this->m_nDir  = RandomInt(0, 3);
						
						--this->m_nAttempts;
						
						this->m_ctPlacementDir.Start(RandomFloat(0.10f, 0.25f));
					}
					
					switch (this->m_nDir) {
					case 0: actor->PressForwardButton();  break;
					case 1: actor->PressBackwardButton(); break;
					case 2: actor->PressRightButton();    break;
					case 3: actor->PressLeftButton();     break;
					}
				}
			}
		} else {
			actor->StartBuildingObjectOfType(OBJ_SENTRYGUN);
		}
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotEngineerBuildSentrygun::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}
