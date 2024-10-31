/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_move_to_build.h"
#include "tf_bot_engineer_building.h"
#include "tf_bot_engineer_build_teleport_exit.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_obj_teleporter.h"
#include "func_capture_zone.h"
#include "tf_train_watcher.h"
#include "func_vip_safetyzone.h"


static ConVar tf_bot_debug_sentry_placement           ("tf_bot_debug_sentry_placement",               "0", FCVAR_CHEAT);
static ConVar tf_bot_max_teleport_exit_travel_to_point("tf_bot_max_teleport_exit_travel_to_point", "2500", FCVAR_CHEAT, "In an offensive engineer bot's tele exit is farther from the point than this, destroy it");
static ConVar tf_bot_min_teleport_travel              ("tf_bot_min_teleport_travel",               "3000", FCVAR_CHEAT, "Minimum travel distance between teleporter entrance and exit before engineer bot will build one");


ActionResult<CTFBot> CTFBotEngineerMoveToBuild::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->SelectBuildLocation(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerMoveToBuild::Update(CTFBot *actor, float dt)
{
	// Stay still while waiting for players or no path to build location.
	// Otherwise, attacking bots get themselves stuck in corners due to setup gates.
	if ( TFGameRules()->IsInWaitingForPlayers() || ( !this->m_PathFollower.Compute( actor, this->m_vecBuildLocation, CTFBotPathCost( actor, SAFEST_ROUTE ) ) ) ) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

		Continue();
	}

	if (actor->LostControlPointRecently()) {
		if (this->m_ctLostControlPoint.IsElapsed()) {
			this->SelectBuildLocation(actor);
			this->m_ctLostControlPoint.Invalidate();
		} else {
			Continue();
		}
	}
	
	if (actor->GetObjectOfType(OBJ_SENTRYGUN) != nullptr) {
		for (auto hint : CTFBotHintSentrygun::AutoList()) {
			if (hint->GetOwnerPlayer() == actor) {
				ChangeTo(new CTFBotEngineerBuilding(hint), "Going back to my existing sentry nest and reusing a sentry hint");
			}
		}
		
		ChangeTo(new CTFBotEngineerBuilding(), "Going back to my existing sentry nest");
	}
	
	/* A/D mode: offense */
	if ( ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP || TFGameRules()->GetGameType() == TF_GAMETYPE_VIP ) &&
		!TFGameRules()->IsInKothMode() &&
		( actor->GetTeamNumber() == TF_TEAM_BLUE || actor->GetTFTeam()->GetRole() == TEAM_ROLE_ATTACKERS ) ) {
		auto tele_exit = assert_cast<CObjectTeleporter *>(actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT));
		if (tele_exit != nullptr) {
			CTeamControlPoint *point = actor->GetMyControlPoint();
			if (point != nullptr) {
				CTFNavArea *point_area = TheTFNavMesh->GetControlPointCenterArea(point->GetPointIndex());
				
				tele_exit->UpdateLastKnownArea();
				CTFNavArea *tele_area = tele_exit->GetLastKnownTFArea();
				
				if (point_area != nullptr && tele_area != nullptr) {
					float delta_incdist = abs(tele_area->GetIncursionDistance(actor->GetTeamNumber()) - point_area->GetIncursionDistance(actor->GetTeamNumber()));
					
					if (delta_incdist > tf_bot_max_teleport_exit_travel_to_point.GetFloat()) {
						tele_exit->DetonateObject();
					}

					m_vecBuildLocation = tele_area->GetRandomPoint();
				}
			}
		} else {
			bool consider_retreat = true;
			
			auto tele_entrance = assert_cast<CObjectTeleporter *>(actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE));
			if (tele_entrance != nullptr) {
				CTFNavArea *actor_area = actor->GetLastKnownTFArea();
				
				tele_entrance->UpdateLastKnownArea();
				CTFNavArea *tele_area = tele_entrance->GetLastKnownTFArea();
				
				if (actor_area != nullptr && tele_area != nullptr) {
					float delta_incdist = abs(tele_area->GetIncursionDistance(actor->GetTeamNumber()) - actor_area->GetIncursionDistance(actor->GetTeamNumber()));
					
					if (delta_incdist < tf_bot_min_teleport_travel.GetFloat()) {
						consider_retreat = false;
					}
				}
			}
			
			if (consider_retreat && actor->GetVisionInterface()->GetPrimaryKnownThreat(true) != nullptr && !actor->m_Shared.IsInvulnerable() && this->ShouldRetreat(actor)) {
				SuspendFor(new CTFBotRetreatToCover(new CTFBotEngineerBuildTeleportExit()), "Retreating to a safe place to build my teleporter exit");
			}
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		this->m_PathFollower.Compute(actor, this->m_vecBuildLocation, CTFBotPathCost(actor, SAFEST_ROUTE));
	}
	
	if (!actor->GetLocomotionInterface()->IsOnGround()) {
		Continue();
	}
	
	Vector delta1 = VectorXY(this->m_vecBuildLocation - actor->GetAbsOrigin());
	Vector delta2 = delta1 + VectorXY(50.0f * EyeVectorsFwd(actor));
	
	if (delta1.IsLengthLessThan(25.0f) || delta2.IsLengthLessThan(25.0f)) {
		if (this->m_hBuildLocation != nullptr) {
			ChangeTo(new CTFBotEngineerBuilding(this->m_hBuildLocation), "Reached my precise build location");
		} else {
			ChangeTo(new CTFBotEngineerBuilding(), "Reached my build location");
		}
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->SelectBuildLocation(actor);
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnTerritoryLost(CTFBot *actor, int idx)
{
	this->m_ctLostControlPoint.Start(0.2f);
	
	Continue();
}


void CTFBotEngineerMoveToBuild::CollectBuildAreas(CTFBot *actor)
{
	if (actor->GetEngieBuildAreaOverride() != nullptr) return;
	
	this->m_BuildAreas.RemoveAll();
	
	CUtlVector<CTFNavArea *> areas;
	Vector avg_pos = vec3_origin;
	
	int actor_teamnum = actor->GetTeamNumber();

	CTeamTrainWatcher *payload = nullptr;

	// In Arena: Choose nearby safe areas, build asap
	if ( TFGameRules()->IsInArenaMode() ) {
		CUtlVector<CTFNavArea *> quick_areas;
		CollectSurroundingTFAreas( &quick_areas, actor->GetLastKnownTFArea(), 750.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight() );

		for ( auto quick_area : quick_areas ) {
			if ( quick_area != nullptr && !quick_area->HasAnySpawnRoom() ) {
				areas.AddToTail( quick_area );
				avg_pos += quick_area->GetCenter();
			}
		}
	}
	
	CCaptureZone *zone = actor->GetFlagCaptureZone();
	if (zone != nullptr) 
	{
		CTFNavArea *zone_area = TheTFNavMesh->GetNearestTFNavArea(zone->WorldSpaceCenter(), false, 500.0f, true, true, TEAM_ANY);
		if (zone_area != nullptr) {
			areas.AddToTail(zone_area);
			avg_pos += zone_area->GetCenter();
		}
	}
	else if ( TFGameRules()->IsInHybridCTF_CPMode() )
	{
		// Trying to detect flag delivery modes
		if ( actor->GetTFTeam()->GetRole() == TEAM_ROLE_DEFENDERS )
		{
			for ( auto zone : CCaptureZone::AutoList() ) {
				if ( zone != nullptr && actor->GetTeamNumber() != zone->GetTeamNumber() ) {
					CTFNavArea *delivery_area = TheTFNavMesh->GetNearestTFNavArea( zone->WorldSpaceCenter(), false, 500.0f, true, true, TEAM_ANY );
					if ( delivery_area != nullptr ) {
						areas.AddToTail( delivery_area );
						avg_pos += delivery_area->GetCenter();
					}
				}
			}
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) 
	{
		// Payload Race.
		if ( TFGameRules()->HasMultipleTrains() )
		{
			payload = TFGameRules()->GetPayloadToPush( actor_teamnum );

			if ( payload != nullptr ) {
				CBaseEntity *train = payload->GetTrainEntity();
				if ( train != nullptr ) {
					CTFNavArea *payload_area = TheTFNavMesh->GetNearestTFNavArea( train->WorldSpaceCenter(), false, 500.0f, false, true, actor_teamnum );
					if ( payload_area != nullptr ) {
						areas.AddToTail( payload_area );
						avg_pos += payload_area->GetCenter();
					}
				}
			}
		}
		else // A/D Payload.
		{
			switch ( actor_teamnum ) {
			case TF_TEAM_BLUE: payload = TFGameRules()->GetPayloadToPush( TF_TEAM_BLUE ); break;
			case TF_TEAM_RED:  payload = TFGameRules()->GetPayloadToBlock( TF_TEAM_RED ); break;
			default:           Assert( false );
			}

			if ( payload != nullptr ) {
				CTFNavArea *payload_area = TheTFNavMesh->GetNearestTFNavArea( payload->GetNextCheckpointPosition(), false, 500.0f, false, true, actor_teamnum );
				if ( payload_area != nullptr ) {
					areas.AddToTail( payload_area );
					avg_pos += payload_area->GetCenter();
				}
			}
		}
	} 
	else if ( TFGameRules()->IsVIPMode() ) 
	{
		CVIPSafetyZone *zone = actor->GetVIPEscapeZone();
		if ( zone != nullptr ) {
			CTFNavArea *zone_area = TheTFNavMesh->GetNearestTFNavArea( zone->WorldSpaceCenter(), false, 500.0f, true, true, TEAM_ANY );
			if ( zone_area != nullptr ) {
				areas.AddToTail( zone_area );
				avg_pos += zone_area->GetCenter();
			}
		}

		CTeamControlPoint *point = actor->GetMyControlPoint();
		if ( point != nullptr ) {
			const auto *point_areas = TheTFNavMesh->GetControlPointAreas( point->GetPointIndex() );
			if ( point_areas != nullptr ) {
				for ( auto area : *point_areas ) {
					areas.AddToTail( area );
					avg_pos += area->GetCenter();
				}
			}
		}
	} 
	else 
	{
		CTeamControlPoint *point = actor->GetMyControlPoint();
		if (point != nullptr) {
			const auto *point_areas = TheTFNavMesh->GetControlPointAreas(point->GetPointIndex());
			if (point_areas != nullptr) {
				for (auto area : *point_areas) {
					areas.AddToTail(area);
					avg_pos += area->GetCenter();
				}
			}
		}
	}
	
	if ( areas.IsEmpty() )
	{
		if ( actor->IsDebugging( INextBot::DEBUG_BEHAVIOR | INextBot::DEBUG_ERRORS ) ) {
			ConColorMsg( NB_COLOR_YELLOW, "%3.2f: %s no good Move to Build location found\n",
				gpGlobals->curtime, actor->GetDebugIdentifier() );
		}
		return;
	}
	avg_pos /= areas.Count();
	
	for (auto area : areas) {
		area->ForAllPotentiallyVisibleTFAreas([&](CTFNavArea *vis_area){
			float actor_incdist = vis_area->GetIncursionDistance(actor_teamnum);
			if (actor_incdist < 0.0f) return true;
			
			float enemy_incdist = -1.0f;
			actor->ForEachEnemyTeam([&](int enemy_teamnum){
				// Get shortest distance for any enemy team to get here
				enemy_incdist = Max(enemy_incdist, vis_area->GetIncursionDistance(enemy_teamnum));
				return true;
			});
			if (enemy_incdist < 0.0f) return true;
			
			if (TFGameRules()->IsInKothMode() && actor_incdist >= enemy_incdist) {
				return true;
			}
			
			if (TFGameRules()->GetGameType() == TF_GAMETYPE_CP) {
				if (vis_area->HasAnyTFAttributes(CTFNavArea::CONTROL_POINT))    return true;
				if (vis_area->GetCenter().z < (avg_pos.z - 150.0f))             return true;
				if (avg_pos.DistToSqr(vis_area->GetCenter()) > Square(1210.0f)) return true;
				// TODO: replace magic number 1210 with 110% of sentry range constant
			}

			// Payload
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
			{
				// Payload Race
				if ( TFGameRules()->HasMultipleTrains() )
				{
					// Only build in friendly territory
					if ( actor_incdist >= enemy_incdist )
						return true;

					// Only build in front of the cart, not behind
					payload = TFGameRules()->GetPayloadToBlock( actor_teamnum );

					if ( !payload->IsAheadOfTrain( vis_area->GetCenter() ) )
					{
						return true;
					}
				}
			}
			
			// TODO: figure out magic numbers 60.0f and 70.0f
			// (sentry eye height and player eye height? or something?)
			if (!actor->IsLineOfFireClear(VecPlusZ(vis_area->GetCenter(), 60.0f), VecPlusZ(avg_pos, 70.0f))) {
				return true;
			}
			
			if (!this->m_BuildAreas.HasElement(vis_area)) {
				this->m_BuildAreas.AddToTail(vis_area);
			}
			
			return true;
		});
	}
	
	/* NOTE: in live TF2, the sort functor was CompareRangeToPoint, but we're
	 * gonna be cool and use a lambda instead, since this sort functor is used
	 * literally nowhere else in the entire game */
	static Vector s_pointCentroid = avg_pos;
	this->m_BuildAreas.Sort([](CTFNavArea *const *area1, CTFNavArea *const *area2){
		float distsqr1 = s_pointCentroid.DistToSqr((*area1)->GetCenter());
		float distsqr2 = s_pointCentroid.DistToSqr((*area2)->GetCenter());
		
		if (distsqr1 > distsqr2) return  1;
		if (distsqr1 < distsqr2) return -1;
		return 0;
	});
	
	this->m_flTotalArea = 0.0f;
	for (auto area : this->m_BuildAreas) {
		this->m_flTotalArea += (area->GetSizeX() * area->GetSizeY());
		
		if (tf_bot_debug_sentry_placement.GetBool()) {
			TheTFNavMesh->AddToSelectedSet(area);
		}
	}
}

void CTFBotEngineerMoveToBuild::SelectBuildLocation(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	this->m_hBuildLocation   = nullptr;
	this->m_vecBuildLocation = vec3_origin;
	
	CTFNavArea *override = actor->GetEngieBuildAreaOverride();
	if (override != nullptr) {
		this->m_vecBuildLocation = override->GetCenter();
		return;
	}
	
	CUtlVector<CTFBotHintSentrygun *> hints;
	for (auto hint : CTFBotHintSentrygun::AutoList()) {
		if (hint->GetOwnerPlayer() == actor) {
			hint->SetOwnerPlayer(nullptr);
		}
		
		if (hint->IsAvailableForSelection(actor)) {
			hints.AddToTail(hint);
		}
	}
	
	if (!hints.IsEmpty()) {
		auto hint = hints.Random();
		
		this->m_hBuildLocation = hint;
		hint->SetOwnerPlayer(actor);
		
		this->m_vecBuildLocation = hint->GetAbsOrigin();
		return;
	}
	
	this->CollectBuildAreas(actor);
	
	CTFNavArea *build_area = TheTFNavMesh->ChooseWeightedRandomArea(&this->m_BuildAreas);
	if (build_area != nullptr) {
		this->m_vecBuildLocation = build_area->GetRandomPoint();
	} else {
		if ( actor->IsDebugging( INextBot::DEBUG_BEHAVIOR | INextBot::DEBUG_ERRORS ) ) {
			ConColorMsg( NB_COLOR_YELLOW, "%3.2f: %s no build location found\n",
				gpGlobals->curtime, actor->GetDebugIdentifier() );
			NDebugOverlay::Cross3D( actor->GetAbsOrigin(), 32.0f, NB_RGB_YELLOW, true, 5.0f );
		}
		this->m_vecBuildLocation = actor->GetAbsOrigin();
	}
}
