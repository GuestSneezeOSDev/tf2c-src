/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_building.h"
#include "tf_bot_engineer_move_to_build.h"
#include "tf_bot_engineer_build_sentrygun.h"
#include "tf_bot_engineer_build_dispenser.h"
#include "tf_bot_engineer_build_teleport_exit.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_obj_teleporter.h"
#include "tf_train_watcher.h"
#include "tf_sentrygun.h"
#include "tf_teleporter_exit.h"
#include "tf_bot_attack.h"


//static ConVar tf_bot_engineer_retaliate_range                    ("tf_bot_engineer_retaliate_range",                      "750", FCVAR_CHEAT, "If attacker who destroyed sentry is closer than this, attack. Otherwise, retreat");
static ConVar tf_bot_engineer_exit_near_sentry_range             ("tf_bot_engineer_exit_near_sentry_range",              "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and its Teleporter Exit");
static ConVar tf_bot_engineer_max_sentry_travel_distance_to_point("tf_bot_engineer_max_sentry_travel_distance_to_point", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and the currently contested point");


ActionResult<CTFBot> CTFBotEngineerBuilding::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Melee() != nullptr && actor->GetTFWeapon_Melee()->IsWeapon(TF_WEAPON_WRENCH));
	
	this->m_PathFollower.Initialize(actor);
	
	this->m_nAttempts = 5;
	
	this->m_b4858 = false;
	this->m_b486c = false;
	
	this->m_nUpgradeStatus = UPGRADE_REEVAL;
	
	this->m_bDontLookAroundForEnemies = false;
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuilding::Update(CTFBot *actor, float dt)
{
	auto sentry    = assert_cast<CObjectSentrygun  *>(actor->GetObjectOfType(OBJ_SENTRYGUN));
	auto dispenser = assert_cast<CObjectDispenser  *>(actor->GetObjectOfType(OBJ_DISPENSER));
	auto tele1     = assert_cast<CObjectTeleporter *>(actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE));
	auto tele2     = assert_cast<CObjectTeleporter *>(actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT));
	
	if (this->m_bDontLookAroundForEnemies) {
		this->LookAroundForEnemies_Reset(actor);
		this->m_bDontLookAroundForEnemies = false;
	}
	
	if (sentry == nullptr) {
		this->m_nUpgradeStatus = UPGRADE_REEVAL;
		
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
		if (threat != nullptr && threat->IsVisibleRecently()) {
			actor->EquipBestWeaponForThreat(threat);
		}
		
		if (!this->m_b4858 && this->m_nAttempts > 0) {
			--this->m_nAttempts;
			
			if (this->m_hHint != nullptr) {
				SuspendFor(new CTFBotEngineerBuildSentrygun(this->m_hHint), "Building a Sentry at a hint location");
			} else {
				SuspendFor(new CTFBotEngineerBuildSentrygun(), "Building a Sentry");
			}
		} else {
			ChangeTo(new CTFBotEngineerMoveToBuild(), "Couldn't find a place to build");
		}
	}
	
	this->m_b4858 = true;
	
	if (this->m_hHint != nullptr && this->m_hHint->IsDisabled()) {
		this->m_hHint = nullptr;
	}
	
	if (this->m_hHint == nullptr || !this->m_hHint->IsSticky()) {
		if (!this->m_b486c) {
			if (this->m_ctCheckOutOfPosition.IsElapsed()) {
				this->m_ctCheckOutOfPosition.Start(RandomFloat(3.0f, 5.0f));
				
				if ( this->CheckIfSentryIsOutOfPosition(actor) )
				{
					// Destroy everything and move up.
					TFGameRules()->VoiceCommand( actor, 0, 3 );
					actor->DetonateOwnedObjectsOfType( OBJ_SENTRYGUN );
					actor->DetonateOwnedObjectsOfType( OBJ_DISPENSER );
					actor->DetonateOwnedObjectsOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );
					Continue();
				}
			}
		}
		
		this->m_b486c = false;
	}
	
	if (dispenser != nullptr && dispenser->GetAbsOrigin().DistToSqr(sentry->GetAbsOrigin()) > Square(500.0f)) {
		dispenser->DetonateObject();
	}

	// Attack enemy spies.
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && threat->GetEntity()->IsPlayer() ) {
		CTFPlayer *player = ToTFPlayer( threat->GetEntity() );

		if ( TFGameRules()->IsInArenaMode() )
		{
			SuspendFor( new CTFBotAttack(), "Attacking an enemy in Arena" );
		}

		if ( player->IsPlayerClass( TF_CLASS_SPY, true ) )
		{
			SuspendFor( new CTFBotAttack(), "Attacking an enemy spy" );
		}
	}
	
	bool actor_under_fire = (actor->GetTimeSinceLastInjury() < 1.0f);
	bool    sentry_sapped = (sentry    != nullptr &&    sentry->HasSapper());
	bool dispenser_sapped = (dispenser != nullptr && dispenser->HasSapper());
	
	bool actor_sentry_dispenser_safe = (!actor_under_fire && !sentry_sapped && !dispenser_sapped);
	
	// Build Dispenser whenever possible
	if (dispenser != nullptr) {
		this->m_ctBuildDispenser.Start(10.0f);
	} else if (actor_sentry_dispenser_safe) {
		if (this->m_ctBuildDispenser.IsElapsed()) {
			this->m_ctBuildDispenser.Start(10.0f);
				
			SuspendFor(new CTFBotEngineerBuildDispenser(), "Building a Dispenser");
		}
	}
	
	float tele_duration = (TFGameRules()->IsInTraining() ? 5.0f : 30.0f);
	
	if (tele2 != nullptr) {
		this->m_ctBuildTeleporter.Start(tele_duration);
		
		this->UpgradeAndMaintainBuildings(actor);
		Continue();
	}
	
	if (this->m_ctBuildTeleporter.IsElapsed() && tele1 != nullptr && actor_sentry_dispenser_safe) {
		this->m_ctBuildTeleporter.Start(tele_duration);
		
		if (this->m_hHint != nullptr) {
			CUtlVector<CTFBotHintTeleporterExit *> hints;
			for (auto hint : CTFBotHintTeleporterExit::AutoList()) {
				if (hint->IsDisabled())       continue;
				if (!hint->InSameTeam(actor)) continue;
				
				hints.AddToTail(hint);
			}
			
			if (!hints.IsEmpty()) {
				sentry->UpdateLastKnownArea();
				CTFNavArea *sentry_area = sentry->GetLastKnownTFArea();
				
				MarkSurroundingTFAreas(sentry_area, tf_bot_engineer_exit_near_sentry_range.GetFloat(),
					actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetDeathDropHeight());
				
				CTFBotHintTeleporterExit *hint_tele_best = nullptr;
				float min_cost = FLT_MAX;
				
				for (auto hint : hints) {
					CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(hint, GETNAVAREA_CHECK_LOS, 500.0f);
					if (area == nullptr) continue;
					
					if (area->IsMarked() && area->GetCostSoFar() < min_cost) {
						min_cost = area->GetCostSoFar();
						hint_tele_best = hint;
					}
				}
				
				if (hint_tele_best != nullptr) {
					const Vector& pos = hint_tele_best->GetAbsOrigin();
					float yaw         = hint_tele_best->GetAbsAngles().y;
					
					SuspendFor(new CTFBotEngineerBuildTeleportExit(pos, yaw), "Building teleporter exit at nearby hint");
				}
			}
		} else {
			if (actor->IsRangeLessThan(sentry, 300.0f)) {
				SuspendFor(new CTFBotEngineerBuildTeleportExit(), "Building teleporter exit");
			}
		}
	}

	if ( sentry->GetUpgradeLevel() < sentry->GetMaxUpgradeLevel() ) {
		if ( this->m_nUpgradeStatus == UPGRADE_REEVAL ) {
			this->m_nUpgradeStatus = ( this->IsMetalSourceNearby( actor ) ? UPGRADE_METAL_YEP : UPGRADE_METAL_NOPE );
		}

		if ( this->m_nUpgradeStatus == UPGRADE_METAL_YEP ) {
			this->UpgradeAndMaintainBuildings( actor );
			Continue();
		}
	}
	
	this->UpgradeAndMaintainBuildings(actor);
	Continue();
}

void CTFBotEngineerBuilding::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	this->LookAroundForEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotEngineerBuilding::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_bDontLookAroundForEnemies) {
		this->LookAroundForEnemies_Reset(actor);
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuilding::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_bDontLookAroundForEnemies) {
		this->LookAroundForEnemies_Set(actor, false);
	}
	
	Continue();
}


bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition(CTFBot *actor) const
{
	auto sentry = assert_cast<CObjectSentrygun *>(actor->GetObjectOfType(OBJ_SENTRYGUN));
	if (sentry == nullptr) return false;
	
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT) {
		CTeamTrainWatcher *payload = nullptr;

		// Payload Race.
		if ( TFGameRules()->HasMultipleTrains() )
		{
			payload = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
		}
		else // A/D Payload.
		{
			switch ( actor->GetTeamNumber() ) {
			case TF_TEAM_BLUE: payload = TFGameRules()->GetPayloadToPush( TF_TEAM_BLUE ); break;
			case TF_TEAM_RED:  payload = TFGameRules()->GetPayloadToBlock( TF_TEAM_RED ); break;
			default:           Assert( false );
			}
		}

		if ( payload != nullptr ) {
			float dist_along_path;
			payload->ProjectPointOntoPath( sentry->GetAbsOrigin(), nullptr, &dist_along_path );

			return ( ( dist_along_path + sentry->GetMaxRange() ) < payload->GetTrainDistanceAlongTrack() );
		}
	}
	
	sentry->UpdateLastKnownArea();
	CTFNavArea *sentry_area = sentry->GetLastKnownTFArea();
	if (sentry_area == nullptr) return false;
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) return false;
	
	CTFNavArea *point_area = TheTFNavMesh->GetControlPointCenterArea(point->GetPointIndex());
	if (point_area == nullptr) return false;
	
	// TODO: double check my SSE math here with disassembly from a different build...
	CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
	if ( NavAreaTravelDistance( sentry_area, point_area, cost_func, 0.0f, actor->GetTeamNumber() ) > tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() &&
		NavAreaTravelDistance( point_area, sentry_area, cost_func, 0.0f, actor->GetTeamNumber() ) > tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) {
		return true;
	} else {
		return false;
	}
}

bool CTFBotEngineerBuilding::IsMetalSourceNearby(CTFBot *actor) const
{
	CUtlVector<CTFNavArea *> areas;
	CollectSurroundingTFAreas(&areas, actor->GetLastKnownTFArea(), 2000.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight());
	
	for (auto area : areas) {
		if (area->HasAnyTFAttributes(CTFNavArea::AMMO)) return true;
		if (area->HasFriendlySpawnRoom(actor))          return true;
		
		// TODO: consider RESCUE_CLOSET?
	}
	
	return false;
}

void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings(CTFBot *actor)
{
	// TODO: eliminate some of the magic numbers here by making use of simple modifiers on top of:
	// - CObjectDispenser::GetDispenserRadius
	// - CTFWeaponBaseMelee::GetSwingRange
	// TODO: also do this in all other engiebot AI actions
	
	auto sentry = assert_cast<CObjectSentrygun *>(actor->GetObjectOfType(OBJ_SENTRYGUN));
	if (sentry == nullptr) return;
	
	actor->SwitchToMelee();
	
	bool stop_looking_around = false;
	
	auto dispenser = assert_cast<CObjectDispenser *>(actor->GetObjectOfType(OBJ_DISPENSER));
	if (dispenser != nullptr) {
		
		float distsqr_to_sentry    = sentry   ->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
		float distsqr_to_dispenser = dispenser->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
		
		if (distsqr_to_sentry < Square(90.0f) && distsqr_to_dispenser < Square(90.0f)) {
			actor->PressCrouchButton();
		}
		
		float distsqr_diff = abs(distsqr_to_dispenser - distsqr_to_sentry);
		if (distsqr_diff > Square(25.0f) || distsqr_to_sentry > Square(75.0f) || distsqr_to_dispenser > Square(75.0f)) {
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				
				Vector sentry_dispenser_avg_origin = 0.5f * (sentry->GetAbsOrigin() + dispenser->GetAbsOrigin());
				this->m_PathFollower.Compute(actor, sentry_dispenser_avg_origin, CTFBotPathCost(actor, FASTEST_ROUTE));
			}
			
			this->m_PathFollower.Update(actor);
		}
		
		if (distsqr_to_sentry < Square(75.0f) || distsqr_to_dispenser < Square(75.0f)) {
			this->m_ct0034.Invalidate();
			
			CBaseObject *obj = this->ChooseBuildingToWorkOn(actor, sentry, dispenser);
			
			stop_looking_around = true;
			
			actor->GetBodyInterface()->AimHeadTowards(obj->WorldSpaceCenter(), IBody::PRI_CRITICAL, 1.0f, nullptr, "Work on my buildings");
			actor->PressFireButton();
		}
	} else {
		float distsqr_to_sentry = sentry->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
		
		if (distsqr_to_sentry < Square(90.0f)) {
			actor->PressCrouchButton();
		}

		Vector sentryFwd = sentry->BodyDirection2D();
		bool inRange = ( distsqr_to_sentry < Square( 75.0f ) );
		bool behindSentry = ( sentryFwd.Dot( sentry->GetAbsOrigin() - actor->GetAbsOrigin() ) > 0.1f );

		if ( inRange )
		{
			stop_looking_around = true;

			actor->GetBodyInterface()->AimHeadTowards( sentry->WorldSpaceCenter(), IBody::PRI_CRITICAL, 1.0f, nullptr, "Work on my Sentry" );
			actor->PressFireButton();
		}

		// Attempt to stay behind the sentry.
		if ( !inRange || !behindSentry ) {
			if ( this->m_ctRecomputePath.IsElapsed() ) {
				this->m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

				Vector targetPos = sentry->GetAbsOrigin() - sentryFwd * 20.0f;
				this->m_PathFollower.Compute( actor, targetPos, CTFBotPathCost( actor, FASTEST_ROUTE ) );
			}

			this->m_PathFollower.Update( actor );
		}
	}
	
	if (stop_looking_around && !this->m_bDontLookAroundForEnemies) {
		this->LookAroundForEnemies_Set(actor, false);
		this->m_bDontLookAroundForEnemies = true;
	}
}


CBaseObject *CTFBotEngineerBuilding::ChooseBuildingToWorkOn(CTFBot *actor, CObjectSentrygun *sentry, CObjectDispenser *dispenser) const
{
	/* priority 1: unsap building(s) (sentry prioritized over dispenser) */
	if (sentry   ->HasSapper()) return sentry;
	if (dispenser->HasSapper()) return dispenser;
	
	/* priority 2: repair sentry if recently attacked or below max health */
	if (sentry->GetTimeSinceLastInjury() < 1.0f)                                    return sentry;
	if (sentry->GetFloatHealth() < sentry->GetMaxHealth() && !sentry->IsBuilding()) return sentry;
	
	/* priority 3: speed up dispenser construction */
	if (dispenser->IsBuilding()) return dispenser;
	
	/* priority 4: upgrade dispenser if behind sentry's level */
	if ( dispenser->GetUpgradeLevel() < dispenser->GetMaxUpgradeLevel() && dispenser->GetUpgradeLevel() < sentry->GetUpgradeLevel() ) return dispenser;
	
	/* priority 5: repair dispenser if below max health */
	if (dispenser->GetFloatHealth() < dispenser->GetMaxHealth()) return dispenser;

	/* priority 6: upgrade sentry to level 3 */
	if ( sentry->GetUpgradeLevel() < sentry->GetMaxUpgradeLevel() ) return sentry;
	
	/* priority 7: sentry */
	return sentry;
}
