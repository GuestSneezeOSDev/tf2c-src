/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_scenario_monitor.h"
#include "tf_bot_seek_and_destroy.h"
#include "scenario/capture_point/tf_bot_capture_point.h"
#include "scenario/capture_point/tf_bot_defend_point.h"
#include "scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "scenario/payload/tf_bot_payload_push.h"
#include "scenario/payload/tf_bot_payload_guard.h"
#include "scenario/vip/tf_bot_escort_vip.h"
#include "engineer/tf_bot_engineer_build.h"
#include "medic/tf_bot_medic_heal.h"
#include "sniper/tf_bot_sniper_lurk.h"
#include "spy/tf_bot_spy_infiltrate.h"
#include "squad/tf_bot_escort_squad_leader.h"
#include "missions/tf_bot_mission_suicide_bomber.h"
#include "tf_bot_manager.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "tf_train_watcher.h"


static ConVar tf_bot_fetch_lost_flag_time("tf_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT, "How long busy TFBots will ignore the dropped flag before they give up what they are doing and go after it");
static ConVar tf_bot_flag_kill_on_touch  ("tf_bot_flag_kill_on_touch",    "0", FCVAR_CHEAT, "If nonzero, any bot that picks up the flag dies. For testing.");


ActionResult<CTFBot> CTFBotScenarioMonitor::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctFetchFlagInitial.Start(20.0f);
	this->m_ctFetchFlag.Invalidate();
	
	Continue();
}

ActionResult<CTFBot> CTFBotScenarioMonitor::Update(CTFBot *actor, float dt)
{
	if (actor->HasTheFlag()) {
		if (tf_bot_flag_kill_on_touch.GetBool()) {
			actor->CommitSuicide(false, true);
			Done("Flag kill");
		}
		
		SuspendFor(new CTFBotDeliverFlag(), "I've picked up the flag! Running it in...");
	}

	if ( TFGameRules()->IsInArenaMode() ) {
		if ( actor->MedicGetHealTarget() == nullptr ) {
			CTeamControlPoint *point = actor->GetMyControlPoint();
			if ( point != nullptr ) {
				SuspendFor( new CTFBotCapturePoint(), "I must fight for the point" );
			}
		}
	}
	
	if (!actor->IsOnAnyMission() && this->m_ctFetchFlagInitial.IsElapsed() && actor->IsAllowedToPickUpFlag()) {
		CCaptureFlag *flag = actor->GetFlagToFetch();
		if (flag != nullptr) {
			CTFPlayer *owner = ToTFPlayer(flag->GetOwnerEntity());
			if (owner != nullptr) {
				this->m_ctFetchFlag.Invalidate();
			} else {
				if (this->m_ctFetchFlag.HasStarted()) {
					if (this->m_ctFetchFlag.IsElapsed()) {
						this->m_ctFetchFlag.Invalidate();
						
						if (actor->MedicGetHealTarget() == nullptr) {
							SuspendFor(new CTFBotFetchFlag(true), "Fetching lost flag...");
						}
					}
				} else {
					this->m_ctFetchFlag.Start(tf_bot_fetch_lost_flag_time.GetFloat());
				}
			}
		}
	}
	
	Continue();
}


Action<CTFBot> *CTFBotScenarioMonitor::InitialContainedAction(CTFBot *actor)
{
	if (actor->IsSquadLeader()) {
		if (actor->IsPlayerClass(TF_CLASS_MEDIC, true)) {
			return new CTFBotMedicHeal();
		} else {
			// TODO
		//	return new CTFBotEscortSquadLeader(this->DesiredScenarioAndClassAction(actor));
			Assert(false);
		}
	}
	
	return this->DesiredScenarioAndClassAction(actor);
}


Action<CTFBot> *CTFBotScenarioMonitor::DesiredScenarioAndClassAction(CTFBot *actor)
{
	if (actor->HasMission(CTFBot::MISSION_DESTROY_SENTRIES)) {
		Assert(false);
//		return new CTFBotMissionSuicideBomber();
	}
	
	if (actor->HasMission(CTFBot::MISSION_SNIPER)) {
		return new CTFBotSniperLurk();
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		// TODO: MvM stuff
		Assert(false);
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !TheTFBots().IsRandomizer()) {
		return new CTFBotSpyInfiltrate();
	}
	
	if (!TheTFBots().IsMeleeOnly() && !TheTFBots().IsRandomizer()) {
		switch (actor->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SNIPER:   return new CTFBotSniperLurk();
		case TF_CLASS_MEDIC:    return new CTFBotMedicHeal();
		case TF_CLASS_ENGINEER: return new CTFBotEngineerBuild();
		}
	}

	CCaptureFlag *flag = actor->GetFlagToFetch();

	if ( TFGameRules()->IsVIPMode() || TFGameRules()->IsInHybridCTF_CPMode() ) {
		// is there a VIP to protect?
		if ( !actor->IsVIP() ) {
			CTFTeam *pTeam = GetGlobalTFTeam( actor->GetTeamNumber() );
			if ( pTeam->GetVIP() != nullptr ) {
				return new CTFBotEscortVIP();
			}
		}

		// VIP can be basically any mode...
		// CTFBotEscortVIP also checks for these:
		// are there CPs?
		CUtlVector<CTeamControlPoint *> points_capture;
		TFGameRules()->CollectCapturePoints( actor, &points_capture );

		if ( !points_capture.IsEmpty() ) {
			return new CTFBotCapturePoint();
		}

		CUtlVector<CTeamControlPoint *> points_defend;
		TFGameRules()->CollectDefendPoints( actor, &points_defend );

		if ( !points_defend.IsEmpty() ) {
			return new CTFBotDefendPoint();
		}

		// are there flags to capture?
		if ( flag != nullptr ) {
			int flagClass = flag->GetLimitToClass();
			if ( flagClass == TF_CLASS_UNDEFINED || flagClass == actor->GetPlayerClass()->GetClassIndex() ) {
				return new CTFBotFetchFlag();
			}
			else {
				return new CTFBotPushToCapturePoint();
			}
		}

		if ( actor->GetFlagCaptureZone() != nullptr ) {
			// TODO: CTFBotDefendCaptureZone
		}
	}
	
	if ( flag != nullptr ) {
		int flagClass = flag->GetLimitToClass();
		if ( flagClass == TF_CLASS_UNDEFINED || flagClass == actor->GetPlayerClass()->GetClassIndex() ) {
			return new CTFBotFetchFlag();
		}
		else {
			return new CTFBotPushToCapturePoint();
		}
	}
	
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) {

		CTeamTrainWatcher *payloadPush = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
		CTeamTrainWatcher *payloadBlock = TFGameRules()->GetPayloadToBlock( actor->GetTeamNumber() );

		// TODO: handle multiple trains (payload race) properly
		// (this probably would require a whole new AI though, to continually re-evaluate whether to push or block)
		if ( TFGameRules()->HasMultipleTrains() )
		{
			// rudimentary Payload Race role assignment
			float enemyProgress = payloadBlock->GetTrainProgress();

			if ( payloadPush != nullptr && payloadBlock != nullptr ) {
				if ( actor->TransientlyConsistentRandomValue( 3.0f ) < enemyProgress ) {
					return new CTFBotPayloadGuard();
				}
				else {
					return new CTFBotPayloadPush();
				}
			}
		}
		
		// not entirely sure whether roles will work properly or if we should
		// just use hardcoded teamnum stuff like live TF does
		switch ( actor->GetTFTeam()->GetRole() )
		{
		case TEAM_ROLE_ATTACKERS: return new CTFBotPayloadPush();
		case TEAM_ROLE_DEFENDERS: return new CTFBotPayloadGuard();
		
		default:
		case TEAM_ROLE_NONE:
			// fallback if Payload map doesn't have roles set
			if ( payloadPush != nullptr ) {
				return new CTFBotPayloadPush();
			}
			if ( payloadBlock != nullptr ) {
				return new CTFBotPayloadGuard();
			}

			return new CTFBotSeekAndDestroy();
		}
	}
	
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_CP || TFGameRules()->IsInDominationMode() || TFGameRules()->IsInArenaMode()) {
		CUtlVector<CTeamControlPoint *> points_capture;
		TFGameRules()->CollectCapturePoints(actor, &points_capture);
		
		if (!points_capture.IsEmpty()) {
			return new CTFBotCapturePoint();
		}
		
		CUtlVector<CTeamControlPoint *> points_defend;
		TFGameRules()->CollectDefendPoints(actor, &points_defend);
		
		if (!points_defend.IsEmpty()) {
			return new CTFBotDefendPoint();
		}
		
		DevMsg("%3.2f: %s: Gametype is CP, but I can't find a point to capture or defend!\n", gpGlobals->curtime, actor->GetDebugIdentifier());
		return new CTFBotCapturePoint();
	}
	
	return new CTFBotSeekAndDestroy();
}
