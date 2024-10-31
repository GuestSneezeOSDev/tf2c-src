/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_deliver_flag.h"
#include "tf_bot_taunt.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_objective_resource.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "particle_parse.h"
#include "tf_announcer.h"
#include "tf_bot_attack.h"
#include "func_flagdetectionzone.h"

static ConVar tf_mvm_bot_allow_flag_carrier_to_fight         ("tf_mvm_bot_allow_flag_carrier_to_fight",              "1", FCVAR_CHEAT);
static ConVar tf_mvm_bot_flag_carrier_interval_to_1st_upgrade("tf_mvm_bot_flag_carrier_interval_to_1st_upgrade",     "5", FCVAR_CHEAT);
static ConVar tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade("tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade",    "15", FCVAR_CHEAT);
static ConVar tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade("tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade",    "15", FCVAR_CHEAT);
static ConVar tf_mvm_bot_flag_carrier_health_regen           ("tf_mvm_bot_flag_carrier_health_regen",            "45.0f", FCVAR_CHEAT);


ActionResult<CTFBot> CTFBotDeliverFlag::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctRecomputePath.Invalidate();
	
	if (TFGameRules()->IsMannVsMachineMode() && !tf_mvm_bot_allow_flag_carrier_to_fight.GetBool() && !actor->HasAttribute(CTFBot::SUPPRESS_FIRE)) {
		actor->SetAttribute(CTFBot::SUPPRESS_FIRE);
		this->m_bAddedSuppressFire = true;
	} else {
		this->m_bAddedSuppressFire = false;
	}
	
	if (actor->IsMiniBoss()) {
		this->m_iUpgradeLevel = -1;
		
		if (TFObjectiveResource() != nullptr) {
			// TFObjectiveResource()->m_nFlagCarrierUpgradeLevel = 4;
			// TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = -1.0f;
			// TFObjectiveResource()->m_flMvMNextBombUpgradeTime = -1.0f;
		}
	} else {
		this->m_iUpgradeLevel = 0;
		
		this->m_ctUpgrade.Start(tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat());
		
		if (TFObjectiveResource() != nullptr) {
			// BUG(?): we don't set TFObjectiveResource()->m_nFlagCarrierUpgradeLevel here...
			// (should be set to zero)
			// TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = gpGlobals->curtime;
			// TFObjectiveResource()->m_flMvMNextBombUpgradeTime = gpGlobals->curtime + this->m_ctUpgrade.GetRemainingTime();
		}
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotDeliverFlag::Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if (flag == nullptr) {
		Done("No flag");
	}
	
	CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
	if (carrier == nullptr || !actor->IsSelf(carrier)) {
		Done("I'm no longer carrying the flag");
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		// TODO: is this really necessary? seems like duplication with CTFBotTacticalMonitor...
		Action<CTFBot> *action = actor->OpportunisticallyUseWeaponAbilities();
		if (action != nullptr) {
			// TODO: it looks like the code also zeroes out this->m_Result here; is that necessary?
			SuspendFor(action, "Opportunistically using buff item");
		}
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		// Don't repath on moving platforms (sd_doomsday).
		Vector vecStart = actor->EyePosition();
		Vector vecEnd = vecStart;
		vecEnd.z -= DeathDrop;
		trace_t trace;
		UTIL_TraceHull( vecStart, vecEnd, actor->GetPlayerMins(), actor->GetPlayerMaxs(), MASK_PLAYERSOLID, actor, COLLISION_GROUP_DEBRIS, &trace );

		// We'll be on a lift of some kind, don't panic and stay on (sd_doomsday).
		if ( trace.m_pEnt && trace.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
		{
			if ( actor->IsDebugging( INextBot::DEBUG_PATH ) ) {
				DevMsg( "%3.2f: %s On a moving platform.\n", gpGlobals->curtime, actor->GetDebugIdentifier() );
			}
			this->m_PathFollower.Invalidate();
			this->m_ctRecomputePath.Start( RandomFloat( 4.0f, 5.0f ) );
			Continue();
		}

		CCaptureZone *zone = actor->GetFlagCaptureZone();
		CFlagDetectionZone* detectzone = actor->GetFlagDetectionZone();
		if (zone == nullptr) {
			if ( detectzone == nullptr )
			{
				Done( "No flag capture zone exists!" );
			}
		}
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		this->m_PathFollower.Compute(actor, zone ? zone->WorldSpaceCenter() : detectzone->WorldSpaceCenter(), cost_func);
		
		// BUG: do we actually initialize m_flDistToCapZone to -1.0f at any point?
		
		// BUG: where the hell is the MvM gamemode check for this bomb reset stuff?
		// (make sure to check whether anything else uses m_flDistToCapZone before we conditionalize the NavAreaTravelDistance call)
		
		float old_dist = this->m_flDistToCapZone;
		float new_dist = NavAreaTravelDistance(actor->GetLastKnownTFArea(), TheTFNavMesh->GetTFNavArea(zone ? zone->WorldSpaceCenter() : detectzone->WorldSpaceCenter()), cost_func, 0.0f, actor->GetTeamNumber());
		
		float delta = new_dist - old_dist;
		
		this->m_flDistToCapZone = new_dist;
		
		if (old_dist != -1.0f && delta > 2000.0f) {
			// g_TFAnnouncer.Speak( TF_ANNOUNCER_MVM_BOMBRESET );
			
			CUtlVector<CTFPlayer *> defenders;
			CollectPlayers(&defenders, TF_TEAM_RED, false);
			
			for (auto defender : defenders) {
				if (defender == nullptr) continue;
				
				// TODO: CAchievementData::IsPusherInHistory
				// TODO: associated event stuff
			}
		}
		
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	if (this->UpgradeOverTime(actor)) {
		// TODO: it looks like the code also zeroes out this->m_Result here; is that necessary?
		SuspendFor(new CTFBotTaunt(), "Taunting for our new upgrade");
	}
	
	Continue();
}

void CTFBotDeliverFlag::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_bAddedSuppressFire) {
		actor->ClearAttribute(CTFBot::SUPPRESS_FIRE);
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		// BUG: this doesn't revert critboost if gained via level 3 bomb carrier upgrade
		// (see: mvm_mannhattan; gate capture resets flags, but ex-bomb-carrier retains crit buff)
		
		// BUG: this likely screws up any active soldier buffs gained via non-bomb-carrier-upgrade means
	//	actor->m_Shared.ResetSoldierBuffs();
	}
}


EventDesiredResult<CTFBot> CTFBotDeliverFlag::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (TFGameRules()->IsMannVsMachineMode() && ent != nullptr && ent->ClassMatches("func_capturezone")) {
	//	// BUG: SEV_CRITICAL results in console spam every single tick
	//	SuspendFor(new CTFBotMvMDeployBomb(), "Delivering the bomb!", SEV_CRITICAL);
	}
	
	Continue();
}


ThreeState_t CTFBotDeliverFlag::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		return (tf_mvm_bot_allow_flag_carrier_to_fight.GetBool() ? TRS_TRUE : TRS_FALSE);
	} else {
		return TRS_NONE;
	}
}

ThreeState_t CTFBotDeliverFlag::ShouldRetreat( const INextBot *nextbot ) const
{
	auto actor = static_cast<CTFBot *>( nextbot->GetEntity() );

	if ( actor->IsPlayerClass( TF_CLASS_CIVILIAN, true ) ) return TRS_NONE;

	return TRS_FALSE;
}

ThreeState_t CTFBotDeliverFlag::ShouldHurry( const INextBot *nextbot ) const
{
	auto actor = static_cast<CTFBot *>( nextbot->GetEntity() );

	if ( actor->IsPlayerClass( TF_CLASS_CIVILIAN, true ) ) return TRS_NONE;

	return TRS_TRUE;
}


bool CTFBotDeliverFlag::UpgradeOverTime(CTFBot *actor)
{
	if (!TFGameRules()->IsMannVsMachineMode()) return false;
	if (this->m_iUpgradeLevel == -1)           return false;
	
	CTFNavArea *lkarea = actor->GetLastKnownTFArea();
	if (lkarea != nullptr && !lkarea->HasFriendlySpawnRoom(actor)) {
		this->m_ctUpgrade.Start(tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat());
		
	//	TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = gpGlobals->curtime;
	//	TFObjectiveResource()->m_flMvMNextBombUpgradeTime = gpGlobals->curtime + this->m_ctUpgrade.GetRemainingTime();
	}
	
	if (this->m_iUpgradeLevel > 0) {
		if (this->m_ctPulseBuff.IsElapsed()) {
			this->m_ctPulseBuff.Start(1.0f);
			
			CUtlVector<CTFPlayer *> teammates;
			CollectPlayers(&teammates, actor->GetTeamNumber(), true);
			
			for (auto teammate : teammates) {
				if (actor->IsRangeLessThan(teammate, 450.0f)) {
					teammate->m_Shared.AddCond(TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, 1.2f);
				}
			}
		}
	}
	
	if (this->m_ctUpgrade.IsElapsed() && this->m_iUpgradeLevel < 3) {
		++this->m_iUpgradeLevel;
		TFGameRules()->BroadcastSound(0xff, "MVM.Warning");
		
		if (this->m_iUpgradeLevel == 1) {
			this->m_ctUpgrade.Start(tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade.GetFloat());
			
			if (TFObjectiveResource() != nullptr) {
			//	TFObjectiveResource()->m_nFlagCarrierUpgradeLevel = 1;
			//	TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = gpGlobals->curtime;
			//	TFObjectiveResource()->m_flMvMNextBombUpgradeTime = gpGlobals->curtime + this->m_ctUpgrade.GetRemainingTime();
				
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE1, TF_TEAM_RED);
				
				DispatchParticleEffect("mvm_levelup1", PATTACH_POINT_FOLLOW, actor, "head");
			}
			
			return true;
		} else if (this->m_iUpgradeLevel == 2) {
			this->m_ctUpgrade.Start(tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade.GetFloat());
			
			static CEconAttributeDefinition *pAttrDef_HealthRegen = GetItemSchema()->GetAttributeDefinitionByName("health regen");
			if (pAttrDef_HealthRegen != nullptr) {
				// TOOD: SetRuntimeAttributeValue etc
			} else {
				// BUG: we aren't TFBotSpawner
				Warning("TFBotSpawner: Invalid attribute 'health regen'\n");
			}
			
			if (TFObjectiveResource() != nullptr) {
			//	TFObjectiveResource()->m_nFlagCarrierUpgradeLevel = 2;
			//	TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = gpGlobals->curtime;
			//	TFObjectiveResource()->m_flMvMNextBombUpgradeTime = gpGlobals->curtime + this->m_ctUpgrade.GetRemainingTime();
				
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE2, TF_TEAM_RED);
				
				DispatchParticleEffect("mvm_levelup2", PATTACH_POINT_FOLLOW, actor, "head");
			}
			
			return true;
		} else if (this->m_iUpgradeLevel == 3) {
			// BUG: should use TF_COND_CRITBOOSTED_USER_BUFF, lest kritzkrieg should remove the cond
			actor->m_Shared.AddCond(TF_COND_CRITBOOSTED);
			
			if (TFObjectiveResource() != nullptr) {
			//	TFObjectiveResource()->m_nFlagCarrierUpgradeLevel = 3;
			//	TFObjectiveResource()->m_flMvMBaseBombUpgradeTime = -1.0f;
			//	TFObjectiveResource()->m_flMvMNextBombUpgradeTime = -1.0f;
				
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE3, TF_TEAM_RED);
				
				DispatchParticleEffect("mvm_levelup3", PATTACH_POINT_FOLLOW, actor, "head");
			}
			
			return true;
		}
	}
	
	return false;
}


ActionResult<CTFBot> CTFBotPushToCapturePoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}

ActionResult<CTFBot> CTFBotPushToCapturePoint::Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if ( flag == nullptr ) {
		Done( "No flag" );
	}

	CTFPlayer *carrier = ToTFPlayer( flag->GetOwnerEntity() );
	if ( carrier == nullptr ) {
		Done( "We're no longer carrying the flag" );
	}

	CCaptureZone *zone = actor->GetFlagCaptureZone();
	CFlagDetectionZone* detectzone = actor->GetFlagDetectionZone();
	if (zone == nullptr) {
		// Try to find a func_flagdetectionzone (sd_doomsday)
		if ( detectzone == nullptr ) {
			if ( this->m_DoneAction != nullptr ) {
				ChangeTo( this->m_DoneAction, "No flag capture zone exists!" );
			}
			else {
				Done( "No flag capture zone exists!" );
			}
		}
	}
	
	if (actor->GetAbsOrigin().DistToSqr(zone ? zone->WorldSpaceCenter() : detectzone->WorldSpaceCenter()) < Square(50.0f)) {
		if (this->m_DoneAction != nullptr) {
			ChangeTo(this->m_DoneAction, "At destination");
		} else {
			Done("At destination");
		}
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
		if ( actor->IsRangeLessThan( threat->GetLastKnownPosition(), 1000.0f ) ) {
			SuspendFor( new CTFBotAttack(), "Going after an enemy" );
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_PathFollower.Compute(actor, zone ? zone->WorldSpaceCenter() : detectzone->WorldSpaceCenter(), CTFBotPathCost(actor, DEFAULT_ROUTE));
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPushToCapturePoint::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)
{
	// TODO: func_nav_prerequisite stuff
	// TODO: CTFBotNavEntMoveTo transition
	// TODO: CTFBotNavEntWait transition
	
	Continue();
}
