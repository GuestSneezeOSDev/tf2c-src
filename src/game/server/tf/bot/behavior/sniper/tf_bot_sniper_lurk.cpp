/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_sniper_lurk.h"
#include "tf_bot_melee_attack.h"
#include "tf_hint.h"
#include "tf_gamerules.h"
#include "tf_nav_mesh.h"


static ConVar tf_bot_sniper_patience_duration     ("tf_bot_sniper_patience_duration",      "10", FCVAR_CHEAT,                         "How long a Sniper bot will wait without seeing an enemy before picking a new spot");
static ConVar tf_bot_sniper_target_linger_duration("tf_bot_sniper_target_linger_duration",  "2", FCVAR_CHEAT,                         "How long a Sniper bot will keep toward at a target it just lost sight of");
static ConVar tf_bot_sniper_allow_opportunistic   ("tf_bot_sniper_allow_opportunistic",     "1", FCVAR_NONE,                          "If set, Snipers will stop on their way to their preferred lurking spot to snipe at opportunistic targets");
static ConVar tf_mvm_bot_sniper_target_by_dps     ("tf_mvm_bot_sniper_target_by_dps",       "1", FCVAR_CHEAT, "If set, Snipers in MvM mode target the victim that has the highest DPS");

extern ConVar tf_bot_debug_sniper;


ActionResult<CTFBot> CTFBotSniperLurk::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_ctPatience.Start(tf_bot_sniper_patience_duration.GetFloat() * RandomFloat(0.9f, 1.1f));
	
	this->m_vecHome   = actor->GetAbsOrigin();
	this->m_bHasHome  = false;
	this->m_bNearHome = false;
	
	this->m_iImpatience = 0;
	
	this->m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();
	
	for (auto hint : CTFBotHint::AutoList()) {
		if (hint->IsSniperSpot()) {
			this->m_Hints.AddToTail(hint);
			
			if (actor->IsSelf(hint->GetOwnerEntity())) {
				hint->SetOwnerEntity(nullptr);
			}
		}
	}
	
	this->m_hHint = nullptr;
	
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		actor->SetMission(CTFBot::MISSION_SNIPER, false);
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::Update(CTFBot *actor, float dt)
{
	actor->AccumulateSniperSpots();
	
	if (!this->m_bHasHome) {
		this->FindNewHome(actor);
	}
	
	bool attacking = false;
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->GetEntity()->IsAlive() && actor->GetIntentionInterface()->ShouldAttack(actor, threat)) {
		if (threat->IsVisibleInFOVNow()) {
			this->m_iImpatience = 0;
			
			extern ConVar tf_bot_sniper_melee_range;
			if (threat->GetLastKnownPosition().DistToSqr(actor->GetAbsOrigin()) < Square(tf_bot_sniper_melee_range.GetFloat())) {
				SuspendFor(new CTFBotMeleeAttack(1.25f * tf_bot_sniper_melee_range.GetFloat()), "Melee attacking nearby threat");
			}
		}
		
		if (threat->GetTimeSinceLastSeen() < tf_bot_sniper_target_linger_duration.GetFloat() && actor->IsLineOfFireClear(threat->GetEntity())) {
			if (this->m_bOpportunistic) {
				actor->SwitchToPrimary();
				
				this->m_ctPatience.Reset();
				
				attacking = true;
				
				if (!this->m_bHasHome) {
					this->m_vecHome = actor->GetAbsOrigin();
					
					this->m_ctPatience.Start(tf_bot_sniper_patience_duration.GetFloat() * RandomFloat(0.9f, 1.1f));
				}
			} else {
				attacking = false;
				
				actor->SwitchToSecondary();
			}
		}
	}
	
	float dsqr_from_home = DistSqrXY(this->m_vecHome, actor->GetAbsOrigin());
	this->m_bNearHome = (dsqr_from_home < Square(25.0f));
	
	if (this->m_bNearHome) {
		this->m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();
		
		if (this->m_ctPatience.IsElapsed()) {
			++this->m_iImpatience;
			
			if (this->FindNewHome(actor)) {
				actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_NEGATIVE);
				
				this->m_ctPatience.Start(tf_bot_sniper_patience_duration.GetFloat() * RandomFloat(0.9f, 1.1f));
			} else {
				this->m_ctPatience.Start(1.0f);
			}
		}
	} else {
		this->m_ctPatience.Reset();
		
		if (!attacking) {
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				this->m_PathFollower.Compute(actor, this->m_vecHome, CTFBotPathCost(actor, SAFEST_ROUTE));
			}
			
			this->m_PathFollower.Update(actor);
			
			if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
				actor->PressAltFireButton();
			}
			
			Continue();
		}
	}
	
	CTFWeaponBase *primary = actor->SwitchToPrimary();
	if (primary != nullptr) {
		if (!actor->m_Shared.InCond(TF_COND_ZOOMED) && actor->IsSniperRifle(primary)) {
			actor->PressAltFireButton();
		}
	}
	
	Continue();
}

void CTFBotSniperLurk::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	this->ReleaseHint(actor);
}

ActionResult<CTFBot> CTFBotSniperLurk::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	this->ReleaseHint(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	this->m_hHint = nullptr;
	
	this->FindNewHome(actor);
	
	Continue();
}


// not sure if redundant
ThreeState_t CTFBotSniperLurk::ShouldRetreat(const INextBot *nextbot) const
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		auto actor = assert_cast<CTFBot *>(nextbot->GetEntity());
		return (actor->GetTeamNumber() == TF_TEAM_BLUE ? TRS_FALSE : TRS_TRUE);
	}
	
	return TRS_NONE;
}

const CKnownEntity *CTFBotSniperLurk::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	if (TFGameRules()->IsMannVsMachineMode() && tf_mvm_bot_sniper_target_by_dps.GetBool()) {
		auto actor = assert_cast<CTFBot *>(nextbot->GetEntity());
		
		if (!threat1->IsVisibleRecently() && threat2->IsVisibleRecently()) {
			return threat2;
		}
		if (threat1->IsVisibleRecently() && !threat2->IsVisibleRecently()) {
			return threat1;
		}
		
		CTFPlayer *player1 = ToTFPlayer(threat1->GetEntity());
		CTFPlayer *player2 = ToTFPlayer(threat2->GetEntity());
		if (player1 == nullptr || player2 == nullptr) {
			return nullptr;
		}
		
		float dsqr1 = actor->GetRangeSquaredTo(player1);
		float dsqr2 = actor->GetRangeSquaredTo(player2);
		
		// TODO: weapon restriction stuff
		Assert(false);
		
	//	if ((actor->m_nRestrict & CTFBot::WeaponRestriction::MELEEONLY) == 0) {
	//		if (dsqr1 >= Square(500.0f) && dsqr2 < Square(500.0f)) {
	//			return threat2;
	//		}
	//		if (dsqr1 < Square(500.0f) && dsqr2 >= Square(500.0f)) {
	//			return threat1;
	//		}
	//		
	//		// TODO: CTFPlayer+0x2930
	//		// (prefer threat with higher value, presumably this is a DPS figure)
	//	}
		
		if (dsqr2 > dsqr1) {
			return threat1;
		}
		return threat2;
	}
	
	return nullptr;
}


bool CTFBotSniperLurk::FindNewHome(CTFBot *actor)
{
	if (!this->m_ctFindNewHome.IsElapsed()) {
		return false;
	}
	this->m_ctFindNewHome.Start(RandomFloat(1.0f, 2.0f));
	
	if (this->FindHint(actor)) {
		return true;
	}
	
	const CTFBot::SniperSpotInfo *spot = actor->GetRandomSniperSpot();
	if (spot != nullptr) {
		this->m_vecHome  = spot->from_vec;
		this->m_bHasHome = true;
		
		return true;
	}
	
	this->m_bHasHome = false;
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point != nullptr && !point->IsLocked()) {
		CTFNavArea *area = TheTFNavMesh->GetControlPointRandomArea(point->GetPointIndex());
		if (area != nullptr) {
			this->m_vecHome = area->GetRandomPoint();
			return false;
		}
	}
	
	CUtlVector<CTFNavArea *> areas;
	TheTFNavMesh->CollectEnemySpawnRoomThresholdAreas(&areas, actor->GetTeamNumber());

	// Arena has no respawn room visualizers for CollectEnemySpawnRoomThresholdAreas to find
	if ( TFGameRules()->IsInArenaMode() )
	{
		TheTFNavMesh->CollectEnemySpawnExitAreas( &areas, actor->GetTeamNumber() );
	}
	
	if (!areas.IsEmpty()) {
		this->m_vecHome = areas.Random()->GetCenter();
	} else {
		if ( tf_bot_debug_sniper.GetBool() ) {
			DevMsg( "%3.2f: %s: No Sniper areas found!\n", gpGlobals->curtime, actor->GetPlayerName() );
		}
		this->m_vecHome = actor->GetAbsOrigin();
	}
	
	return false;
}

bool CTFBotSniperLurk::FindHint(CTFBot *actor)
{
	CUtlVector<CTFBotHint *> all_hints;
	for (auto hint : this->m_Hints) {
		if (hint->IsFor(actor)) {
			all_hints.AddToTail(hint);
		}
	}
	
	if (all_hints.IsEmpty()) {
		return false;
	}
	
	this->ReleaseHint(actor);
	
	CTFBotHint *new_hint = nullptr;
	
	if (this->m_hHint != nullptr && this->m_iImpatience < 2) {
		CUtlVector<CTFBotHint *> unowned_hints;
		
		for (auto hint : all_hints) {
			if (hint == this->m_hHint) continue;
			
			if (this->m_hHint->WorldSpaceCenter().DistToSqr(hint->WorldSpaceCenter()) <= Square(500.0f) && hint->GetOwnerEntity() == nullptr) {
				unowned_hints.AddToTail(hint);
			}
		}
		
		if (unowned_hints.IsEmpty()) {
			++this->m_iImpatience;
			return false;
		}
		
		new_hint = unowned_hints.Random();
	} else {
		CUtlVector<CTFPlayer *> enemies;
		actor->CollectEnemyPlayers(&enemies, true);
		
		CUtlVector<CTFBotHint *> unowned_hints; // no owner
		CUtlVector<CTFBotHint *> good_hints;    // no owner AND clear LOS to an enemy
		
		for (auto hint : all_hints) {
			if (hint->GetOwnerEntity() == nullptr) {
				unowned_hints.AddToTail(hint);
				
				for (auto enemy : enemies) {
					if (enemy->IsLineOfSightClear(hint->WorldSpaceCenter(), CBaseCombatCharacter::IGNORE_ACTORS)) {
						good_hints.AddToTail(hint);
					}
				}
			}
		}
		
		if (!good_hints.IsEmpty()) {
			new_hint = good_hints.Random();
		} else {
			if (!unowned_hints.IsEmpty()) {
				new_hint = unowned_hints.Random();
			} else {
				new_hint = all_hints.Random();
				
				if (tf_bot_debug_sniper.GetBool()) {
					DevMsg("%3.2f: %s: No un-owned hints available! Doubling up.\n", gpGlobals->curtime, actor->GetPlayerName());
				}
			}
		}
	}
	
	if (new_hint == nullptr) {
		return false;
	}
	
	Extent ext;
	ext.Init(new_hint);
	
	Vector pos(RandomFloat(ext.lo.x, ext.hi.x), RandomFloat(ext.lo.y, ext.hi.y), (ext.lo.z + ext.hi.z) / 2.0f);
	TheTFNavMesh->GetSimpleGroundHeight(pos, &pos.z);
	
	this->m_bHasHome = true;
	this->m_vecHome  = pos;
	this->m_hHint    = new_hint;
	
	new_hint->SetOwnerEntity(actor);
	
	return true;
}


void CTFBotSniperLurk::ReleaseHint(CTFBot *actor)
{
	if (this->m_hHint != nullptr) {
		this->m_hHint->SetOwnerEntity(nullptr);
		
		if (tf_bot_debug_sniper.GetBool()) {
			DevMsg("%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, actor->GetPlayerName());
		}
	}
}
