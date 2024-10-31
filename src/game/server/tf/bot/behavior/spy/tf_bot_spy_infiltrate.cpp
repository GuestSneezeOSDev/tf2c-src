/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_spy_infiltrate.h"
#include "tf_bot_spy_attack.h"
#include "tf_bot_spy_sap.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"


static ConVar tf_bot_debug_spy("tf_bot_debug_spy", "0", FCVAR_CHEAT);


ActionResult<CTFBot> CTFBotSpyInfiltrate::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_HidingArea = nullptr;
	this->m_bCloaked = false;
	
	Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::Update(CTFBot *actor, float dt)
{
	actor->SwitchToPrimary();
	
	auto area = actor->GetLastKnownTFArea();
	if (area == nullptr) {
		Continue();
	}
	
	if (!actor->m_Shared.IsStealthed() && !area->HasAnySpawnRoom() && area->IsInCombat() && !this->m_bCloaked) {
		this->m_bCloaked = true;
		actor->PressAltFireButton();
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->GetEntity() != nullptr && threat->GetEntity()->IsBaseObject()) {
		auto obj = assert_cast<CBaseObject *>(threat->GetEntity());
		if (!obj->HasSapper() && actor->IsEnemy(obj)) {
			SuspendFor(new CTFBotSpySap(obj), "Sapping an enemy object");
		}
	}
	
	if (actor->HasTargetSentry() && !actor->GetTargetSentry()->HasSapper() && actor->GetDistanceBetween( actor->GetTargetSentry() ) < 1500.0f ) {
		SuspendFor(new CTFBotSpySap(actor->GetTargetSentry()), "Sapping a Sentry");
	}
	
	if (this->m_HidingArea == nullptr && this->m_ctFindHidingArea.IsElapsed()) {
		this->FindHidingSpot(actor);
		this->m_ctFindHidingArea.Start(3.0f);
	}
	
	if (!TFGameRules()->InSetup() && threat != nullptr && threat->GetTimeSinceLastKnown() < 3.0f) {
		CTFPlayer *victim = ToTFPlayer(threat->GetEntity());
		if (victim != nullptr) {
			CTFNavArea *victim_area = victim->GetLastKnownTFArea();
			if (victim_area != nullptr && victim_area->GetIncursionDistance(victim->GetTeamNumber()) > area->GetIncursionDistance(victim->GetTeamNumber())) {
				if (actor->m_Shared.IsStealthed()) {
					SuspendFor(new CTFBotRetreatToCover(new CTFBotSpyAttack(victim)), "Hiding to decloak before going after a backstab victim");
				} else {
					SuspendFor(new CTFBotSpyAttack(victim), "Going after a backstab victim");
				}
			}
		}
	}
	
	if (this->m_HidingArea != nullptr) {
		if (tf_bot_debug_spy.GetBool()) {
			this->m_HidingArea->DrawFilled(0xff, 0xff, 0x00, 0xff, 0.0f);
		}
		
		if (this->m_HidingArea == area) {
			if (TFGameRules()->InSetup()) {
				this->m_ctWait.Start(RandomFloat(0.0f, 5.0f));
			} else {
				if (this->m_ctWait.HasStarted() && this->m_ctWait.IsElapsed()) {
					this->m_ctWait.Invalidate();
				} else {
					this->m_ctWait.Start(RandomFloat(5.0f, 10.0f));
				}
			}
		} else {
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				
				this->m_PathFollower.Compute(actor, this->m_HidingArea->GetCenter(), CTFBotPathCost(actor, SAFEST_ROUTE));
			}
			
			this->m_PathFollower.Update(actor);
			
			this->m_ctWait.Invalidate();
		}
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotSpyInfiltrate::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	this->m_HidingArea = nullptr;
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnStuck(CTFBot *actor)
{
	this->m_ctFindHidingArea.Invalidate();
	this->m_HidingArea = nullptr;
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryCaptured(CTFBot *actor, int idx)
{
	this->m_ctFindHidingArea.Start(5.0f);
	this->m_HidingArea = nullptr;
	
	Continue();
}

EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryLost(CTFBot *actor, int idx)
{
	this->m_ctFindHidingArea.Start(5.0f);
	this->m_HidingArea = nullptr;
	
	Continue();
}


bool CTFBotSpyInfiltrate::FindHidingSpot(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotSpyInfiltrate::FindHidingSpot", "NextBot");
	
	this->m_HidingArea = nullptr;
	
	if (actor->GetAliveDuration() < 5.0f || TFGameRules()->InSetup()) {
		return false;
	}
	
	// NOTE: live TF2 does a very performance-costly thing in this function:
	// It collects the surrounding areas in a 2500 HU radius around each spawn exit area
	// and adds them to the pool of potential areas without checking for duplication of areas;
	// with a map with sufficiently many nav areas (many spawn exits, low max nav area size),
	// this can lead to preposterously high quantities of calls to CNavArea::IsPotentiallyVisible,
	// resulting in lag spikes when spy bots spawn.
	
	// To counteract this, we intentionally ensure that duplicate areas are omitted (~80x reduction!),
	// and we also take a faster-but-more-slapdash approach to random area selection
	
	/* collect all areas covering enemy spawn exits */
	CUtlVector<CTFNavArea *> exit_areas;
	{
		VPROF_BUDGET("CTFBotSpyInfiltrate::FindHidingSpot( CollectEnemySpawnExitAreas )", "NextBot");
		
		TheTFNavMesh->CollectEnemySpawnExitAreas(&exit_areas, actor->GetTeamNumber());
		
		if (exit_areas.IsEmpty()) {
			if (tf_bot_debug_spy.GetBool()) {
				DevMsg("%3.2f: No enemy spawn room exit areas found\n", gpGlobals->curtime);
			}
			
			return false;
		}
	}
	
	/* expand our search to areas that are merely *near* enemy spawn exits */
	CUtlVector<CTFNavArea *> near_exit_areas;
	{
		VPROF_BUDGET("CTFBotSpyInfiltrate::FindHidingSpot( CollectSurroundingTFAreas )", "NextBot");
		
		for (auto exit_area : exit_areas) {
			CUtlVector<CTFNavArea *> areas;
			CollectSurroundingTFAreas(&areas, exit_area, 2500.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight());
			
			for (auto area : areas) {
				if (!near_exit_areas.HasElement(area)) {
					near_exit_areas.AddToTail(area);
				}
			}
		}
	}
	
	/* shuffle the vector so that we can iterate over it sequentially but still effectively make a random choice */
	near_exit_areas.Shuffle();
	
	/* attempt to find a traversable area that isn't easily visible from the enemy spawn exits */
	CUtlVector<CTFNavArea *> traversable_areas;
	{
		VPROF_BUDGET("CTFBotSpyInfiltrate::FindHidingSpot( IsAreaTraversable / IsPotentiallyVisible )", "NextBot");
		
		for (auto near_exit_area : near_exit_areas) {
			if (!actor->GetLocomotionInterface()->IsAreaTraversable(near_exit_area)) continue;
			
			bool visible = false;
			for (auto exit_area : exit_areas) {
				if (near_exit_area->IsPotentiallyVisible(exit_area)) {
					visible = true;
					break;
				}
			}
			
			/* we found a good spot; immediately return it as a random selection (thanks to shuffling earlier) */
			if (!visible) {
				this->m_HidingArea = near_exit_area;
				return true;
			}
			
			/* save this area in case we need to do the fallback case later */
			traversable_areas.AddToTail(near_exit_area);
		}
	}
	
	if (tf_bot_debug_spy.GetBool()) {
		DevMsg("%3.2f: Can't find any non-visible hiding areas, trying for anything near the spawn exit...\n", gpGlobals->curtime);
	}
	
	if (!traversable_areas.IsEmpty()) {
		this->m_HidingArea = traversable_areas.Random();
		return true;
	}
	
	if (tf_bot_debug_spy.GetBool()) {
		DevMsg("%3.2f: Can't find any areas near the enemy spawn exit - just heading to the enemy spawn and hoping...\n", gpGlobals->curtime);
	}
	
	this->m_HidingArea = exit_areas.Random();
	return false;
}
