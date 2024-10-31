/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_attack.h"
#include "tf_nav_mesh.h"
#include "tf_control_point.h"
#include "tf_gamerules.h"
#include "tf_train_watcher.h"


static ConVar tf_bot_debug_seek_and_destroy("tf_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT);


CTFBotSeekAndDestroy::CTFBotSeekAndDestroy(float duration)
{
	if (duration > 0.0f) {
		this->m_ctActionDuration.Start(duration);
	}
}


ActionResult<CTFBot> CTFBotSeekAndDestroy::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->RecomputeSeekPath(actor);
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point != nullptr) {
		this->m_bPointLocked = point->IsLocked();
	} else {
		this->m_bPointLocked = false;
	}
	
	/* start the countdown timer back to the beginning */
	if (this->m_ctActionDuration.HasStarted()) {
		this->m_ctActionDuration.Reset();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::Update(CTFBot *actor, float dt)
{
	if (this->m_ctActionDuration.HasStarted() && this->m_ctActionDuration.IsElapsed()) {
		Done("Behavior duration elapsed");
	}
	
	if (TFGameRules()->IsInTraining() && actor->IsAnyPointBeingCaptured()) {
		Done("Assist trainee in capturing the point");
	}
	
	// many of these Done transitions are based on the assumption that we got
	// here via CTFBotCapturePoint or CTFBotDefendPoint, which may not be true

	// 2018/08/02: stopped SeekAndDestroy ending early in non-CP modes

	CUtlVector<CTeamControlPoint *> points_capture;
	TFGameRules()->CollectCapturePoints( actor, &points_capture );

	if ( !points_capture.IsEmpty() ) {
		if ( actor->IsCapturingPoint() ) {
			if ( TFGameRules() && TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
			{
				CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
				if ( payload && payload->GetCapturerCount() > 3 )
				{
					Continue();
				}
				else
				{
					Done( "Keep capturing payload I happened to stumble upon" );
				}
			}
			else
			{
				Done( "Keep capturing point I happened to stumble upon" );
			}
		}

		if ( this->m_bPointLocked ) {
			CTeamControlPoint *point = actor->GetMyControlPoint();
			if ( point != nullptr && !point->IsLocked() ) {
				Done( "The point just unlocked" );
			}
		}

		extern ConVar tf_bot_offense_must_push_time;
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() ) {
			Done( "Time to push for the objective" );
		}
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr) {
		if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() == actor->GetTeamNumber()) {
			SuspendFor(new CTFBotAttack(), "Chasing down the losers");
		}
		
		if (actor->IsRangeLessThan(threat->GetLastKnownPosition(), 1000.0f)) {
			SuspendFor(new CTFBotAttack(), "Going after an enemy");
		}
	}
	
	if (!this->m_PathFollower.IsValid() && this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(1.0f);
		this->RecomputeSeekPath(actor);
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->RecomputeSeekPath(actor);
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	this->RecomputeSeekPath(actor);
	Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->RecomputeSeekPath(actor);
	Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnStuck(CTFBot *actor)
{
	this->RecomputeSeekPath(actor);
	Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryContested(CTFBot *actor, int idx)
{
	Done("Defending the point", SEV_MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryCaptured(CTFBot *actor, int idx)
{
	Done("Giving up due to point capture", SEV_MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryLost(CTFBot *actor, int idx)
{
	Done("Giving up due to point lost", SEV_MEDIUM);
}


ThreeState_t CTFBotSeekAndDestroy::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *bot = ToTFBot(nextbot->GetEntity());
	if (bot->IsPlayerClass(TF_CLASS_PYRO)) return TRS_FALSE;
	
	return TRS_NONE;
}


CTFNavArea *CTFBotSeekAndDestroy::ChooseGoalArea(CTFBot *actor)
{
	// In round win state, charge straight into enemy spawn room. Otherwise, go near the exits.
	CUtlVector<CTFNavArea *> areas;
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		TheTFNavMesh->CollectEnemySpawnRoomAreas( &areas, actor->GetTeamNumber() );
	}
	else
	{
		TheTFNavMesh->CollectEnemySpawnRoomThresholdAreas( &areas, actor->GetTeamNumber() );

		// Arena has no respawn room visualizers for CollectEnemySpawnRoomThresholdAreas to find
		if ( TFGameRules()->IsInArenaMode() )
		{
			TheTFNavMesh->CollectEnemySpawnExitAreas( &areas, actor->GetTeamNumber() );
		}
	}
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point != nullptr && !point->IsLocked()) {
		CTFNavArea *area = TheTFNavMesh->GetControlPointRandomArea(point->GetPointIndex());
		if (area != nullptr) {
			areas.AddToTail(area);
		}
	}

	// Roam towards enemy VIPs, but don't make the cheating too obvious
	if ( TFGameRules()->IsVIPMode() )
	{
		CTFPlayer *targetVIP = nullptr;
		actor->ForEachEnemyTeam( [&]( int enemy_teamnum ){
			targetVIP = GetGlobalTFTeam( enemy_teamnum )->GetVIP();

			if ( targetVIP != nullptr )
			{
				CTFNavArea *target_area = TheTFNavMesh->GetNearestTFNavArea( targetVIP );
				CUtlVector<CTFNavArea *> target_areas;
				CollectSurroundingTFAreas( &target_areas, target_area, 2000.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight() );
				for ( auto area : target_areas ) {
					if ( area != nullptr )
						areas.AddToTail( area );
				}
			}

			return true;
		} );
	}

	// Roam towards random enemies or areas in Arena
	// or if no other areas could be found
	if ( TFGameRules()->IsInArenaMode() || areas.IsEmpty() )
	{
		CTFPlayer *target = actor->SelectRandomReachableEnemy();
		if ( target == nullptr ) {
			CUtlVector<CTFNavArea *> random_areas;
			CollectSurroundingTFAreas( &random_areas, actor->GetLastKnownTFArea(), 2000.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight() );
			for ( auto area : random_areas ) {
				if ( area != nullptr )
					areas.AddToTail( area );
			}
		}
		else {
			CTFNavArea *target_area = TheTFNavMesh->GetNearestTFNavArea( target );
			if ( target_area != nullptr ) {
				areas.RemoveAll();
				areas.AddToTail( target_area );
			}
		}
	}
	
	if (tf_bot_debug_seek_and_destroy.GetBool()) {
		for (auto area : areas) {
			TheTFNavMesh->AddToSelectedSet(area);
		}
	}
	
	if (!areas.IsEmpty()) {
		return areas.Random();
	} else {
		if ( actor->IsDebugging( INextBot::DEBUG_BEHAVIOR ) ) {
			Warning( "%3.2f: No Seek and Destroy areas found\n", gpGlobals->curtime );
		}
		return nullptr;
	}
}

void CTFBotSeekAndDestroy::RecomputeSeekPath(CTFBot *actor)
{
	this->m_GoalArea = this->ChooseGoalArea(actor);
	
	if (this->m_GoalArea != nullptr) {
		this->m_PathFollower.Compute(actor, this->m_GoalArea->GetCenter(), CTFBotPathCost(actor, SAFEST_ROUTE));
	} else {
		this->m_PathFollower.Invalidate();
	}
}
