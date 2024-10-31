/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_nav_ent_move_to.h"
#include "func_nav_prerequisite.h"


ActionResult<CTFBot> CTFBotNavEntMoveTo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed before we started");
	}
	
	this->m_PathFollower.Initialize(actor);
	this->m_ctWait.Invalidate();
	
	CBaseEntity *target = prereq->GetTaskEntity();
	if (target == nullptr) {
		Done("Prerequisite target entity is NULL");
	}
	
	Extent box;
	box.Init(target);
	
	this->m_vecGoal = box.lo;
	this->m_vecGoal.x += RandomFloat(0.0f, box.SizeX());
	this->m_vecGoal.y += RandomFloat(0.0f, box.SizeY());
	this->m_vecGoal.z += box.SizeZ();
	
	TheNavMesh->GetSimpleGroundHeight(this->m_vecGoal, &this->m_vecGoal.z);
	
	this->m_pGoalArea = TheNavMesh->GetNavArea(this->m_vecGoal);
	if (this->m_pGoalArea == nullptr) {
		Done("There's no nav area for the goal position");
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotNavEntMoveTo::Update(CTFBot *actor, float dt)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed");
	}
	
	if (prereq->IsDisabled()) {
		Done("Prerequisite has been disabled");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctWait.HasStarted()) {
		if (this->m_ctWait.IsElapsed()) {
			Done("Wait duration elapsed");
		}
	} else {
		if (actor->GetLastKnownArea() == this->m_pGoalArea) {
			this->m_ctWait.Start(prereq->GetTaskValue());
			Continue();
		}
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			this->m_PathFollower.Compute(actor, this->m_vecGoal, CTFBotPathCost(actor, FASTEST_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
	}
	
	Continue();
}
