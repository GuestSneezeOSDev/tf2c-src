/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_defend_point_block_capture.h"
#include "medic/tf_bot_medic_heal.h"
#include "demoman/tf_bot_prepare_stickybomb_trap.h"
#include "tf_control_point.h"
#include "tf_nav_mesh.h"


static ConVar tf_bot_defend_owned_point_percent("tf_bot_defend_owned_point_percent", "0.5", FCVAR_CHEAT, "Stay on the contested point we own until enemy cap percent falls below this");


ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		Done("Point is NULL");
	}
	this->m_hPoint = point;
	
	this->m_PointArea = TheTFNavMesh->GetNearestTFNavArea(point->GetAbsOrigin());
	if (this->m_PointArea == nullptr) {
		Done("Can't find nav area on point");
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotDefendPointBlockCapture::Update(CTFBot *actor, float dt)
{
	if (this->IsPointSafe(actor)) {
		Done("Point is safe again");
	}
	
	// BUG: WTF is this here for?
	// Test with this being disabled.
#if 0
	if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
		Warning("CTFBotDefendPointBlockCapture::Update(#%d): SuspendFor(new CTFBotMedicHeal()); this shouldn't happen!\n", actor->entindex());
		SuspendFor(new CTFBotMedicHeal());
	}
#endif
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	actor->EquipBestWeaponForThreat(threat);
	
	Extent ext;
	ext.Init(this->m_hPoint);
	bool is_on_point = ext.Contains(actor->GetAbsOrigin());
	
	CTFNavArea *actor_lkarea = actor->GetLastKnownTFArea();
	const CUtlVector<CTFNavArea *> *point_areas = TheTFNavMesh->GetControlPointAreas(this->m_hPoint->GetPointIndex());
	if (!is_on_point && actor_lkarea != nullptr && point_areas != nullptr) {
		for (auto area : *point_areas) {
			if (actor_lkarea->GetID() == area->GetID()) {
				is_on_point = true;
				break;
			}
		}
	}
	
	if (is_on_point && CTFBotPrepareStickybombTrap::IsPossible(actor)) {
		SuspendFor(new CTFBotPrepareStickybombTrap(), "Placing stickies for defense");
	}
	
	if (point_areas != nullptr) {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.5f, 1.0f));
			
			CTFNavArea *goal_area = TheTFNavMesh->ChooseWeightedRandomArea(point_areas);
			if (goal_area != nullptr) {
				this->m_PathFollower.Compute(actor, goal_area->GetRandomPoint(), CTFBotPathCost(actor, DEFAULT_ROUTE));
			}
		}
	} else {
		if (is_on_point) {
			TFGameRules()->VoiceCommand( actor, 0, 2 );
			Continue();
		}
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.5f, 1.0f));
			
			this->m_PathFollower.Compute(actor, (ext.lo + ext.hi) / 2.0f, CTFBotPathCost(actor, DEFAULT_ROUTE));
		}
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryLost(CTFBot *actor, int idx)
{
	Done("Lost the point", SEV_CRITICAL);
}


bool CTFBotDefendPointBlockCapture::IsPointSafe(CTFBot *actor)
{
	if (actor->LostControlPointRecently()) {
		return false;
	}
	
	if (this->m_hPoint == nullptr) {
		return true;
	}
	
	if (this->m_hPoint->GetTeamCapPercentage(actor->GetTeamNumber()) < tf_bot_defend_owned_point_percent.GetFloat()) {
		return false;
	}
	
	if (this->m_hPoint->HasBeenContested() && (gpGlobals->curtime - this->m_hPoint->LastContestedAt()) < 5.0f) {
		return false;
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat == nullptr) return true;
	
	return (this->m_hPoint->GetAbsOrigin().DistToSqr(threat->GetLastKnownPosition()) >= Square(500.0f));
}
