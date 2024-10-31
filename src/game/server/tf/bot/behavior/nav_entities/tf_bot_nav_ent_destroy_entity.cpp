/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_nav_ent_destroy_entity.h"
#include "func_nav_prerequisite.h"
#include "tf_weapon_pipebomblauncher.h"


ActionResult<CTFBot> CTFBotNavEntDestroyEntity::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed before we started");
	}
	
	this->m_PathFollower.Initialize(actor);
	
	this->m_bIgnoringEnemies = false;
	
	this->m_bFireToggle = true;
	
	Continue();
}

ActionResult<CTFBot> CTFBotNavEntDestroyEntity::Update(CTFBot *actor, float dt)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed");
	}
	
	CBaseEntity *target = prereq->GetTaskEntity();
	if (target == nullptr) {
		Done("Target entity is NULL");
	}
	
	float max_range = actor->GetMaxAttackRange();
	if (prereq->GetTaskValue() > 0.0f) {
		max_range = Min(max_range, prereq->GetTaskValue());
	}
	
	if (target->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin()) < Square(max_range) && actor->GetVisionInterface()->IsLineOfSightClearToEntity(target)) {
		if (!this->m_bIgnoringEnemies) {
			this->IgnoreEnemies_Set(actor, true);
			this->m_bIgnoringEnemies = true;
		}
		
		actor->GetBodyInterface()->AimHeadTowards(target->WorldSpaceCenter(), IBody::PRI_CRITICAL, 0.2f, nullptr, "Aiming at target we need to destroy to progress");
		if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
			if (actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
				CTFWeaponBase *active = actor->GetActiveTFWeapon();
				CTFPipebombLauncher *pipe_launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
				
				if (active != nullptr && !active->IsWeapon(TF_WEAPON_PIPEBOMBLAUNCHER)) {
					actor->Weapon_Switch(pipe_launcher);
				}
				
				if (this->m_bFireToggle) {
					actor->PressFireButton();
				}
				this->m_bFireToggle = !this->m_bFireToggle;
				
				this->DetonateStickiesWhenSet(actor, pipe_launcher);
			} else {
				actor->EquipBestWeaponForThreat(nullptr);
				actor->PressFireButton();
			}
		}
	} else {
		if (this->m_bIgnoringEnemies) {
			this->IgnoreEnemies_Reset(actor);
			this->m_bIgnoringEnemies = false;
		}
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			this->m_PathFollower.Compute(actor, target->GetAbsOrigin(), CTFBotPathCost(actor, FASTEST_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
	}
	
	Continue();
}

void CTFBotNavEntDestroyEntity::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	this->IgnoreEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotNavEntDestroyEntity::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_bIgnoringEnemies) {
		this->IgnoreEnemies_Reset(actor);
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotNavEntDestroyEntity::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_bIgnoringEnemies) {
		this->IgnoreEnemies_Set(actor, true);
	}
	
	Continue();
}


void CTFBotNavEntDestroyEntity::DetonateStickiesWhenSet(CTFBot *actor, CTFPipebombLauncher *launcher) const
{
	if (launcher == nullptr) return;
	
	/* don't det until we've either laid the max possible number of stickies or
	 * exhausted our ammo */
	// TODO: check that this properly takes into account in-clip ammo
	if (launcher->GetPipeBombCount() < launcher->GetMaxPipeBombCount() && actor->GetAmmoCount(TF_AMMO_SECONDARY) > 0) {
		return;
	}
	
	/* don't det until all stickies are done moving and have stuck */
	for (int i = launcher->GetPipeBombCount() - 1; i >= 0; --i) {
		CTFGrenadeStickybombProjectile *sticky = launcher->GetPipeBomb(i);
		if (sticky == nullptr) continue;
		
		if (!sticky->HasTouched()) {
			return;
		}
	}
	
	actor->PressAltFireButton();
}
