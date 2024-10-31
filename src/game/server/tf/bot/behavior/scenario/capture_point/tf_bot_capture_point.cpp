/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_capture_point.h"
#include "tf_bot_defend_point.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_gamerules.h"
#include "tf_nav_mesh.h"


       ConVar tf_bot_offense_must_push_time               ("tf_bot_offense_must_push_time",					"30", FCVAR_CHEAT, "If timer is less than this, bots will push hard to cap");
static ConVar tf_bot_capture_seek_and_destroy_min_duration("tf_bot_capture_seek_and_destroy_min_duration",  "15", FCVAR_CHEAT, "If a capturing bot decides to go hunting, this is the min duration he will hunt for before reconsidering");
static ConVar tf_bot_capture_seek_and_destroy_max_duration("tf_bot_capture_seek_and_destroy_max_duration",  "30", FCVAR_CHEAT, "If a capturing bot decides to go hunting, this is the max duration he will hunt for before reconsidering");


ActionResult<CTFBot> CTFBotCapturePoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotCapturePoint::OnStart", "NextBot");
	
	this->m_PathFollower.Initialize(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotCapturePoint::Update(CTFBot *actor, float dt)
{
	// Stay still while waiting for players or setup.
	// Otherwise, attacking bots get themselves stuck in corners due to setup gates.
	if ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->InSetup() ) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		Continue();
	}
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		SuspendFor(new CTFBotSeekAndDestroy(10.0f), "Seek and destroy until a point becomes available");
	}
	
	if (point->GetOwner() == actor->GetTeamNumber()) {
		ChangeTo(new CTFBotDefendPoint(), "We need to defend our point(s)");
		TFGameRules()->VoiceCommand( actor, 2, 0 );
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if ((!actor->IsPointBeingCaptured(point) || actor->GetTimeSinceWeaponFired() < 2.0f) && !actor->IsCapturingPoint()) {
		if (!TFGameRules()->InOvertime() && actor->GetTimeLeftToCapture() >= tf_bot_offense_must_push_time.GetFloat()) {
			if (!TFGameRules()->IsInTraining() && !actor->IsPlayerClass(TF_CLASS_CIVILIAN, true) && !actor->IsNearPoint(point)) {
				if (threat != nullptr && threat->IsVisibleRecently()) {
					float min_duration = tf_bot_capture_seek_and_destroy_min_duration.GetFloat();
					float max_duration = tf_bot_capture_seek_and_destroy_max_duration.GetFloat();
					float duration = RandomFloat(min_duration, max_duration);
					
					SuspendFor(new CTFBotSeekAndDestroy(duration), "Too early to capture - hunting");
				}
			}
		}
	}
	
	if (actor->IsCapturingPoint() && this->m_ctRecomputePath.IsElapsed()) {
		const CUtlVector<CTFNavArea *> *point_areas = TheTFNavMesh->GetControlPointAreas(point->GetPointIndex());
		if (point_areas == nullptr) {
			Continue();
		}
		
		this->m_ctRecomputePath.Start(RandomFloat(0.5f, 1.0f));
		
		CTFNavArea *goal_area = TheTFNavMesh->ChooseWeightedRandomArea(point_areas);
		if (goal_area != nullptr) {
			this->m_PathFollower.Compute(actor, goal_area->GetRandomPoint(), CTFBotPathCost(actor, DEFAULT_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
		
		Continue();
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		VPROF_BUDGET("CTFBotCapturePoint::Update( repath )", "NextBot");
		
		this->m_PathFollower.Compute(actor, point->GetAbsOrigin(), CTFBotPathCost(actor, SAFEST_ROUTE));
		this->m_ctRecomputePath.Start(RandomFloat(2.0f, 3.0f));
	}
	
	if (TFGameRules()->IsInTraining() && !actor->IsAnyPointBeingCaptured() && this->m_PathFollower.GetLength() < 1000.0f) {
		TFGameRules()->VoiceCommand( actor, 0, 2 );
	} else {
		this->m_PathFollower.Update(actor);
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotCapturePoint::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	Continue();
}


EventDesiredResult<CTFBot> CTFBotCapturePoint::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_ctRecomputePath.Invalidate();
	Continue();
}


EventDesiredResult<CTFBot> CTFBotCapturePoint::OnStuck(CTFBot *actor)
{
	this->m_ctRecomputePath.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotCapturePoint::OnTerritoryCaptured(CTFBot *actor, int idx)
{
	this->m_ctRecomputePath.Invalidate();
	Continue();
}


ThreeState_t CTFBotCapturePoint::ShouldHurry(const INextBot *nextbot) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	if (actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat()) {
		return TRS_TRUE;
	}
	
	return TRS_NONE;
}

ThreeState_t CTFBotCapturePoint::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	if (actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat()) {
		return TRS_FALSE;
	}
	
	return TRS_NONE;
}
