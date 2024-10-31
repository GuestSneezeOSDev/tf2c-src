/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_tactical_monitor.h"
#include "tf_bot_scenario_monitor.h"
#include "tf_bot_get_ammo.h"
#include "tf_bot_get_health.h"
#include "tf_bot_taunt.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_bot_destroy_enemy_sentry.h"
#include "tf_bot_use_teleporter.h"
#include "nav_entities/tf_bot_nav_ent_wait.h"
#include "nav_entities/tf_bot_nav_ent_move_to.h"
#include "training/tf_bot_training.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_weapon_pipebomblauncher.h"
#include "func_nav_prerequisite.h"


static ConVar tf_bot_force_jump("tf_bot_force_jump", "0", FCVAR_CHEAT, "Force bots to continuously jump");


ActionResult<CTFBot> CTFBotTacticalMonitor::Update(CTFBot *actor, float dt)
{
	if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) {
		if (TFGameRules()->GetWinningTeam() == actor->GetTeamNumber()) {
			SuspendFor(new CTFBotSeekAndDestroy(), "Get the losers!");
		} else {
			if (actor->GetVisionInterface()->GetPrimaryKnownThreat(true) != nullptr) {
				SuspendFor(new CTFBotRetreatToCover(), "Run away from threat!");
			} else {
				actor->PressCrouchButton();
				Continue();
			}
		}
	}
	
	if (tf_bot_force_jump.GetBool() && !actor->GetLocomotionInterface()->IsClimbingOrJumping()) {
		actor->GetLocomotionInterface()->Jump();
	}
	
	if (TFGameRules()->State_Get() == GR_STATE_PREROUND) {
		actor->GetLocomotionInterface()->ClearStuckStatus("In preround");
	}
	
	Action<CTFBot> *action = actor->OpportunisticallyUseWeaponAbilities();
	if (action != nullptr) {
		SuspendFor(action, "Opportunistically using buff item");
	}
	
	if (TFGameRules()->InSetup() && this->m_ctHumanTauntB.IsElapsed()) {
		CTFPlayer *human = actor->GetClosestHumanLookingAtMe(TEAM_ANY);
		if (human != nullptr) {
			if (!this->m_ctHumanTauntC.HasStarted()) {
				this->m_ctHumanTauntC.Start(0.5f);
			} else if (this->m_ctHumanTauntC.IsElapsed()) {
				if (!this->m_ctHumanTauntA.HasStarted()) {
					actor->GetBodyInterface()->AimHeadTowards(human, IBody::PRI_IMPORTANT, 3.0f, nullptr, "Acknowledging friendly human attention");
					this->m_ctHumanTauntA.Start(RandomFloat(0.0f, 2.0f));
				} else if (this->m_ctHumanTauntA.IsElapsed()) {
					this->m_ctHumanTauntA.Invalidate();
					this->m_ctHumanTauntB.Start(RandomFloat(10.0f, 20.0f));
					
					SuspendFor(new CTFBotTaunt(), "Acknowledging friendly human attention");
				}
			}
		} else {
			this->m_ctHumanTauntC.Invalidate();
		}
	}
	
	ThreeState_t should_retreat = actor->GetIntentionInterface()->ShouldRetreat(actor);
	if (!TFGameRules()->IsMannVsMachineMode()) {
		if (should_retreat == TRS_TRUE) {
			SuspendFor(new CTFBotRetreatToCover(), "Backing off");
		}
		
		if (should_retreat == TRS_NONE && !actor->m_Shared.IsInvulnerable() && actor->GetSkill() >= CTFBot::HARD) {
			CTFWeaponBase *primary = actor->GetTFWeapon_Primary();
			// BUG: need to check that Clip1() < GetMaxClip1(); otherwise, weapons with clip size of 1 will probably
			// force bots into a continual loop of moving into cover to reload
			if (primary != nullptr && actor->GetAmmoCount(TF_AMMO_PRIMARY) > 0 && actor->IsBarrageAndReloadWeapon(primary) && primary->Clip1() <= 1) {
				SuspendFor(new CTFBotRetreatToCover(), "Moving to cover to reload");
			}
		}
	}
	
	ThreeState_t should_hurry = actor->GetIntentionInterface()->ShouldHurry(actor);
	if (should_hurry != TRS_TRUE && !(TFGameRules()->IsMannVsMachineMode() && actor->HasTheFlag())) {
		if (this->m_ctNonHurryStuff.IsElapsed()) {
			this->m_ctNonHurryStuff.Start(RandomFloat(0.3f, 0.5f));
			
			bool should_get_health = false;
			if (actor->GetTimeSinceWeaponFired() < 2.0f || actor->IsPlayerClass(TF_CLASS_SNIPER, true)) {
				extern ConVar tf_bot_health_critical_ratio;
				should_get_health = (actor->HealthFraction() < tf_bot_health_critical_ratio.GetFloat());
			} else {
				extern ConVar tf_bot_health_ok_ratio;
				should_get_health = (actor->HealthFraction() < tf_bot_health_ok_ratio.GetFloat() || actor->m_Shared.InCond(TF_COND_BURNING));
			}
			
			if (should_get_health && CTFBotGetHealth::IsPossible(actor)) {
				SuspendFor(new CTFBotGetHealth(), "Grabbing nearby health");
			}
			
			if (actor->IsAmmoLow() && CTFBotGetAmmo::IsPossible(actor)) {
				SuspendFor(new CTFBotGetAmmo(), "Grabbing nearby ammo");
			}
			
			if (!TFGameRules()->IsMannVsMachineMode() && actor->HasTargetSentry() && CTFBotDestroyEnemySentry::IsPossible(actor)) {
				SuspendFor(new CTFBotDestroyEnemySentry(), "Going after an enemy sentry to destroy it");
			}
		}
	}


	if ( this->ShouldOpportunisticallyTeleport( actor ) && this->m_ctFindNearbyTele.IsElapsed() ) {
		this->m_ctFindNearbyTele.Start( 1.0f );

		CObjectTeleporter *tele = CTFBotUseTeleporter::FindNearbyTeleporter( actor, CTFBotUseTeleporter::HOW_NORMAL, actor->GetCurrentPath() );
		if (tele != nullptr) {
			SuspendFor( new CTFBotUseTeleporter( tele, CTFBotUseTeleporter::HOW_NORMAL, actor->GetCurrentPath() ), "Using nearby teleporter" );
		}
	}
	
	this->MonitorArmedStickybombs(actor);
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true)) {
		this->AvoidBumpingEnemies(actor);

		if ( actor->m_Shared.IsStealthed() && ( actor->m_Shared.InCond( TF_COND_BURNING ) || actor->m_Shared.InCond( TF_COND_BLEEDING ) ) )
		{
			actor->PressAltFireButton();
		}

		if ( actor->GetVisionInterface()->GetPrimaryKnownThreat( true ) != nullptr && actor->m_Shared.GetLastStealthExposedTime() >= gpGlobals->curtime - 1.0f )
		{
			SuspendFor( new CTFBotRetreatToCover(), "Moving to cover after being revealed" );
		}
	}
	
	actor->UpdateDelayedThreatNotices();
	
	if (actor->IsSquadLeader() && actor->GetSquad()->ShouldSquadLeaderWaitForFormation()) {
		// TODO: SuspendFor CTFBotWaitForOutOfPositionSquadMember
		Assert(false);
	}
	
	Continue();
}


Action<CTFBot> *CTFBotTacticalMonitor::InitialContainedAction(CTFBot *actor)
{
	return new CTFBotScenarioMonitor();
}


EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)
{
	if (area1 != nullptr && !actor->HasAttribute(CTFBot::AGGRESSIVE)) {
		for (CFuncNavPrerequisite *prereq : area1->GetPrerequisiteVector()) {
			if (prereq == nullptr)                    continue;
			if (prereq->IsDisabled())                 continue;
			if (!prereq->PassesTriggerFilters(actor)) continue;
			
			if (prereq->IsTask(CFuncNavPrerequisite::TASK_WAIT)) {
				SuspendFor(new CTFBotNavEntWait(prereq), "Prerequisite commands me to wait", SEV_MEDIUM);
			}
			if (prereq->IsTask(CFuncNavPrerequisite::TASK_MOVE_TO)) {
				SuspendFor(new CTFBotNavEntMoveTo(prereq), "Prerequisite commands me to move to an entity", SEV_MEDIUM);
			}
			// BUG(?): no handling for TASK_DESTROY prereqs
		}
	}
	
	Continue();
}

EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnCommandString(CTFBot *actor, const char *cmd)
{
//	if (FStrEq(cmd, "goto action point")) {
//		SuspendFor(new CTFGotoActionPoint(), "Received command to go to action point", SEV_MEDIUM);
//	}
	
//	if (FStrEq(cmd, "despawn")) {
//		SuspendFor(new CTFDespawn(), "Received command to go to de-spawn", SEV_CRITICAL);
//	}
	
	if (FStrEq(cmd, "taunt")) {
		SuspendFor(new CTFBotTaunt(), "Received command to taunt");
	}
	
	if (FStrEq(cmd, "cloak")) {
		if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !actor->m_Shared.IsStealthed()) {
			actor->PressAltFireButton();
		}
		Continue();
	}
	
	if (FStrEq(cmd, "uncloak")) {
		if (actor->IsPlayerClass(TF_CLASS_SPY, true) && actor->m_Shared.IsStealthed()) {
			actor->PressAltFireButton();
		}
		Continue();
	}
	
	if (FStrEq(cmd, "disguise")) {
		if (actor->IsPlayerClass(TF_CLASS_SPY, true) && actor->CanDisguise()) {
			actor->DisguiseAsRandomClass();
		}
		Continue();
	}
	
	if (FStrEq(cmd, "build sentry at nearest sentry hint")) {
		// TODO
	}
	
//	if (FStrEq(cmd, "attack sentry at next action point")) {
//		SuspendFor(new CTFTrainingAttackSentryActionPoint(), "Received command to attack sentry gun at next action point", SEV_CRITICAL);
//	}
	
	Continue();
}


void CTFBotTacticalMonitor::AvoidBumpingEnemies(CTFBot *actor)
{
	if (actor->GetSkill() <= CTFBot::NORMAL) return;
	
	CUtlVector<CTFPlayer *> enemies;
	actor->CollectEnemyPlayers(&enemies, true);
	
	CTFPlayer *closest_enemy = nullptr;
	float closest_distsqr = Square(200.0f);
	
	for (auto enemy : enemies) {
		if (enemy->m_Shared.IsStealthed() ||
			(enemy->m_Shared.InCond(TF_COND_DISGUISED) && enemy->m_Shared.DisguiseFoolsTeam(actor->GetTeamNumber()))) {
			continue;
		}

		if ( actor->GetIntentionInterface()->IsHindrance( actor, enemy ) != TRS_TRUE )
		{
			continue;
		}
		
		float distsqr = actor->GetAbsOrigin().DistToSqr(enemy->GetAbsOrigin());
		if (distsqr < closest_distsqr) {
			closest_distsqr = distsqr;
			closest_enemy   = enemy;
		}
	}
	
	if (closest_enemy != nullptr) {
		actor->ReleaseForwardButton();
		actor->ReleaseLeftButton();
		actor->ReleaseRightButton();
		actor->ReleaseBackwardButton();
		
		Vector dest = actor->GetLocomotionInterface()->GetFeet() + (actor->GetAbsOrigin() - closest_enemy->GetAbsOrigin());
		actor->GetLocomotionInterface()->Approach(dest);
	}
}

void CTFBotTacticalMonitor::MonitorArmedStickybombs(CTFBot *actor)
{
	if (!this->m_ctMonitorArmedStickies.IsElapsed()) return;
	this->m_ctMonitorArmedStickies.Start(RandomFloat(0.3f, 1.0f));
	
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
	if (launcher == nullptr)               return;
	if (launcher->GetPipeBombCount() <= 0) return;
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	for (int i = 0; i < launcher->GetPipeBombCount(); ++i) {
		CTFGrenadeStickybombProjectile *pipe = launcher->GetPipeBomb(i);
		if (pipe == nullptr) continue;
		if ( pipe->IsProxyMine() ) continue;
		
		for (const auto& known : knowns) {
			if (known.IsObsolete())                continue;
			if (known.GetEntity()->IsBaseObject()) continue;
			
			// TODO: replace hard-coded blast radius number with dynamically determined value
			if (pipe->GetTeamNumber() != known.GetEntity()->GetTeamNumber() && pipe->GetAbsOrigin().DistToSqr(known.GetLastKnownPosition()) < Square(150.0f)) {
				actor->PressAltFireButton();
				return;
			}
		}
	}
}

bool CTFBotTacticalMonitor::ShouldOpportunisticallyTeleport(CTFBot *actor) const
{
	switch (actor->GetPlayerClass()->GetClassIndex()) {
	case TF_CLASS_ENGINEER:
		return (actor->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE) != nullptr);
	case TF_CLASS_SCOUT:
	case TF_CLASS_MEDIC:
		return false;
	default:
		return true;
	}
}
