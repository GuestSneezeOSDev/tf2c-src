/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_destroy_enemy_sentry.h"
#include "tf_bot_retreat_to_cover.h"
#include "demoman/tf_bot_stickybomb_sentrygun.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_nav_mesh.h"

static ConVar tf_bot_debug_destroy_enemy_sentry        ("tf_bot_debug_destroy_enemy_sentry",            "0", FCVAR_CHEAT);
static ConVar tf_bot_max_grenade_launch_at_sentry_range("tf_bot_max_grenade_launch_at_sentry_range", "1500", FCVAR_CHEAT);
static ConVar tf_bot_max_sticky_launch_at_sentry_range ("tf_bot_max_sticky_launch_at_sentry_range",  "1500", FCVAR_CHEAT);


class FindSafeSentryApproachAreaScan : public ISearchSurroundingAreasFunctor
{
public:
	FindSafeSentryApproachAreaScan(CTFBot *actor) : m_Actor(actor)
	{
		CTFNavArea *actor_lkarea = actor->GetLastKnownTFArea();
		this->m_b001c = (actor_lkarea != nullptr && actor_lkarea->IsTFMarked());
	}
	
	virtual ~FindSafeSentryApproachAreaScan() {}
	
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override
	{
		auto tf_area      = static_cast<CTFNavArea *>(area);
		auto tf_priorArea = static_cast<CTFNavArea *>(priorArea);
		
		if (this->m_b001c) {
			if (!tf_area->IsTFMarked()) {
				this->m_Areas.AddToTail(tf_area);
				return false;
			}
		} else {
			if (tf_area->IsTFMarked() && tf_priorArea != nullptr) {
				this->m_Areas.AddToTail(tf_priorArea);
			}
		}
		
		return true;
	}
	
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override
	{
		if (!this->m_b001c && static_cast<CTFNavArea *>(currentArea)->IsTFMarked()) {
			return false;
		}
		
		return this->m_Actor->GetLocomotionInterface()->IsAreaTraversable(adjArea);
	}
	
	virtual void PostSearch() override
	{
		if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
			for (auto area : this->m_Areas) {
				area->DrawFilled(NB_RGBA_GREEN, 60.0f, true, 5.0f);
			}
		}
	}
	
	CTFNavArea *GetRandomArea() const
	{
		if (!this->m_Areas.IsEmpty()) {
			return this->m_Areas.Random();
		} else {
			return nullptr;
		}
	}
	
private:
	CTFBot *m_Actor;                  // +0x0004
	CUtlVector<CTFNavArea *> m_Areas; // +0x0008
	bool m_b001c;                     // +0x001c TODO: name
};
// TODO: remove offsets


// TODO: move this elsewhere or put this into one of the classes, or something
static bool FindGrenadeAim(CTFBot *bot, CBaseEntity *target, float& pitch, float& yaw)
{
	CTFWeaponBase *launcher = bot->GetTFWeapon_Primary();
	if (launcher == nullptr) return false;
	
	Vector delta = (target->WorldSpaceCenter() - bot->EyePosition());
	if (delta.IsLengthGreaterThan(tf_bot_max_grenade_launch_at_sentry_range.GetFloat())) return false;
	
	QAngle dir = VectorAngles(delta);
	
	const QAngle& eyes = bot->EyeAngles();
	float try_pitch = eyes.x;
	float try_yaw   = eyes.y;
	
	for (int tries = 10; tries != 0; --tries) {
		Vector est = bot->EstimateGrenadeProjectileImpactPosition(launcher, try_pitch, try_yaw);
		
		// TODO: maybe get rid of this hard-coded blast radius approximation value
		if (est.DistToSqr(target->WorldSpaceCenter()) < Square(75.0f)) {
			trace_t tr;
			UTIL_TraceLine(target->WorldSpaceCenter(), est, MASK_SOLID_BRUSHONLY, NextBotTraceFilterIgnoreActors(target), &tr);
			
			if (!tr.DidHit()) {
				pitch = try_pitch;
				yaw   = try_yaw;
				
				return true;
			}
		}
		
		try_yaw   = dir.y + RandomFloat(-30.0f, 30.0f);
		try_pitch =         RandomFloat(-85.0f, 85.0f);
	}
	
	return false;
}

// TODO: move this elsewhere or put this into one of the classes, or something
static bool FindStickybombAim(CTFBot *bot, CBaseEntity *target, float& pitch, float& yaw, float& charge)
{
	CTFWeaponBase *launcher = bot->GetTFWeapon_Secondary();
	if (launcher == nullptr) return false;
	
	Vector delta = (target->WorldSpaceCenter() - bot->EyePosition());
	if (delta.IsLengthGreaterThan(tf_bot_max_sticky_launch_at_sentry_range.GetFloat())) return false;
	
	QAngle dir = VectorAngles(delta);
	
	const QAngle& eyes = bot->EyeAngles();
	float try_pitch  = eyes.x;
	float try_yaw    = eyes.y;
	float try_charge = 0.0f;
	
	for (int tries = 100; tries != 0; --tries) {
		Vector est = bot->EstimateStickybombProjectileImpactPosition(launcher, try_pitch, try_yaw, try_charge);
		
		// TODO: maybe get rid of this hard-coded blast radius approximation value
		if (est.DistToSqr(target->WorldSpaceCenter()) < Square(75.0f)) {
			trace_t tr;
			UTIL_TraceLine(target->WorldSpaceCenter(), est, MASK_SOLID_BRUSHONLY, NextBotTraceFilterIgnoreActors(target), &tr);
			
			if (!tr.DidHit()) {
				pitch  = try_pitch;
				yaw    = try_yaw;
				charge = try_charge;
				
				return true;
			}
		}
		
		try_yaw   = dir.y + RandomFloat(-30.0f, 30.0f);
		try_pitch =         RandomFloat(-85.0f, 85.0f);
		// NOTE: try_charge always stays at 0.0f
	}
	
	return false;
}


ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_bFiringAtSentry = false;
	this->m_bHasBeenInvuln  = false;
	
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN, true)) {
		this->ComputeCornerAttackSpot(actor);
	} else {
		this->ComputeSafeAttackSpot(actor);
	}
	
	this->m_hSentry = actor->GetTargetSentry();
	
	Continue();
}

ActionResult<CTFBot> CTFBotDestroyEnemySentry::Update(CTFBot *actor, float dt)
{
	CObjectSentrygun *sentry = actor->GetTargetSentry();
	if (sentry == nullptr) {
		Done("Enemy sentry is destroyed");
	}
	
	if (sentry != this->m_hSentry) {
		ChangeTo(new CTFBotDestroyEnemySentry(), "Changed sentry target");
	}
	
	if (actor->m_Shared.IsInvulnerable()) {
		if (!this->m_bHasBeenInvuln) {
			this->m_bHasBeenInvuln = true;
			
			if (NavAreaTravelDistance(actor->GetLastKnownTFArea(), sentry->GetLastKnownTFArea(), CTFBotPathCost(actor, FASTEST_ROUTE), 500.0f, actor->GetTeamNumber()) >= 0.0f) {
				SuspendFor(new CTFBotUberAttackEnemySentry(sentry), "Go get it!");
			}
		}
	} else {
		this->m_bHasBeenInvuln = false;
	}
	
	if (!actor->HasAttribute(CTFBot::IGNORE_ENEMIES)) {
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
		if (threat != nullptr && threat->IsVisibleInFOVNow()) {
			if (actor->GetRangeTo(threat->GetLastKnownPosition()) < (0.5f * actor->GetRangeTo(sentry))) {
				Done("Enemy near");
			}
		}
	}
	
	bool danger = false;
	if (sentry->GetTimeSinceLastFired() < 1.0f) {
		danger = sentry->GetTurretVector().Dot((actor->GetAbsOrigin() - sentry->GetAbsOrigin()).Normalized()) > 0.80f;
	}
	
	// Forced to disable this for now. Doesn't work and causes massive performance hits
#if 0
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
		const Vector& attack_spot = (this->m_bHaveAttackSpot ? this->m_vecAttackSpot : sentry->GetAbsOrigin());
		
		if (!this->m_PathFollower.IsValid() || this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(1.0f);
			this->m_PathFollower.Compute(actor, attack_spot, CTFBotPathCost(actor, SAFEST_ROUTE));
		}
		
		if (danger) {
			actor->EquipLongRangeWeapon();
			actor->PressFireButton();
		} else {
			float aim_pitch;
			float aim_yaw;
			float aim_charge;
			if (FindStickybombAim(actor, sentry, aim_pitch, aim_yaw, aim_charge)) {
				ChangeTo(new CTFBotStickybombSentrygun(sentry, aim_pitch, aim_yaw, aim_charge), "Destroying sentry with opportunistic sticky shot");
			}
		}
		
	//	if (this->m_b4814) {
	//		this->m_PathFollower.Update(actor);
	//	}
		
		// TODO: magic number 1000... is this meant to be just-less-than-sentry-max-range,
		// or something based on sticky launcher range, or what?
		if ((actor->IsRangeLessThan(attack_spot, 50.0f) && DistSqrXY(attack_spot, actor->GetAbsOrigin()) < Square(25.0f)) ||
			(actor->IsLineOfFireClear(sentry) && actor->IsRangeLessThan(sentry, 1000.0f))) {
			ChangeTo(new CTFBotStickybombSentrygun(sentry), "Destroying sentry with stickies");
		}
		
	//	if (!actor->IsRangeLessThan(attack_spot, 200.0f)) {
	//		this->m_b4814 = true;
	//	}
		
		// BUG: the bool @ +0x4814 is never initialized,
		// and the logic is just weird; so we'll do this instead
		if (!actor->IsRangeLessThan(attack_spot, 200.0f)) {
			this->m_PathFollower.Update(actor);
		}
		
		Continue();
	}
	else
#endif
	{
		bool at_attack_spot = (this->m_bHaveAttackSpot && actor->IsRangeLessThan(this->m_vecAttackSpot, 20.0f));
		
		if (at_attack_spot || actor->IsLineOfFireClear(sentry)) {
			actor->GetBodyInterface()->AimHeadTowards(sentry, IBody::PRI_OVERRIDE, 1.0f, nullptr, "Aiming at enemy sentry");
			
			if (EyeVectorsFwd(actor).Dot((sentry->WorldSpaceCenter() - actor->EyePosition()).Normalized()) > 0.95f) {
				if (!actor->EquipLongRangeWeapon()) {
					SuspendFor(new CTFBotRetreatToCover(0.1f), "No suitable range weapon available right now");
				}
				
				actor->PressFireButton();
				
				this->m_bFiringAtSentry = true;
			} else {
				this->m_bFiringAtSentry = false;
			}
			
			if (actor->IsRangeGreaterThan(sentry, 1.1f * sentry->GetMaxRange())) {
				Continue();
			}
			
			if (danger) {
				SuspendFor(new CTFBotRetreatToCover(0.1f), "Taking cover from sentry fire");
			}
			
			if (at_attack_spot) {
				Continue();
			}
		}
		
		if (!this->m_PathFollower.IsValid() || this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(1.0f);
			
			const Vector& attack_spot = (this->m_bHaveAttackSpot ? this->m_vecAttackSpot : sentry->GetAbsOrigin());
			if (!this->m_PathFollower.Compute(actor, attack_spot, CTFBotPathCost(actor, SAFEST_ROUTE))) {
				Done("No path");
			}
		}
		
		this->m_PathFollower.Update(actor);
		
		Continue();
	}
}


ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN, true)) {
		this->ComputeCornerAttackSpot(actor);
	} else {
		this->ComputeSafeAttackSpot(actor);
	}
	
	Continue();
}


ThreeState_t CTFBotDestroyEnemySentry::ShouldHurry(const INextBot *nextbot) const
{
	if (this->m_bFiringAtSentry) return TRS_TRUE;
	
	return TRS_NONE;
}

ThreeState_t CTFBotDestroyEnemySentry::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	if (this->m_bFiringAtSentry) return TRS_FALSE;
	
	return TRS_NONE;
}


bool CTFBotDestroyEnemySentry::IsPossible(CTFBot *actor)
{
	// TODO: rework this mess...
	
	// TODO: handle new TF2C classes
	switch (actor->GetPlayerClass()->GetClassIndex()) {
	case TF_CLASS_HEAVYWEAPONS:
	case TF_CLASS_SNIPER:
	case TF_CLASS_MEDIC:
	case TF_CLASS_ENGINEER:
	case TF_CLASS_PYRO:
	case TF_CLASS_SPY:
	case TF_CLASS_SCOUT:
	case TF_CLASS_CIVILIAN:
		return false;
	}
	// soldier, demo --> allowed
	
	if (actor->GetAmmoCount(TF_AMMO_PRIMARY) <= 0 || actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0) {
		return false;
	}
	
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		return false;
	}
	
	return true;
}


void CTFBotDestroyEnemySentry::ComputeCornerAttackSpot(CTFBot *actor)
{
	this->m_bHaveAttackSpot = false;
	this->m_vecAttackSpot = vec3_origin;
	
	CObjectSentrygun *sentry = this->m_hSentry;
	if (sentry == nullptr) return;
	
	sentry->UpdateLastKnownArea();
	CTFNavArea *sentry_lkarea = sentry->GetLastKnownTFArea();
	if (sentry_lkarea == nullptr) return;
	
	CUtlVector<CTFNavArea *> visible_areas;
	sentry_lkarea->ForAllCompletelyVisibleTFAreas([&](CTFNavArea *area){
		if (!visible_areas.HasElement(area)) {
			visible_areas.AddToTail(area);
		}
		return true;
	});
	
	CTFNavArea::MakeNewTFMarker();
	for (auto area : visible_areas) {
		Vector close; area->GetClosestPointOnArea(sentry->GetAbsOrigin(), &close);
		
		if (close.DistToSqr(sentry->GetAbsOrigin()) < Square(sentry->GetMaxRange())) {
			area->TFMark();
			
			if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
				area->DrawFilled(NB_RGBA_RED, 60.0f, true, 5.0f);
			}
		}
	}
	
	FindSafeSentryApproachAreaScan functor(actor);
	SearchSurroundingAreas(actor->GetLastKnownTFArea(), functor);
	
	CTFNavArea *area = functor.GetRandomArea();
	if (area == nullptr) return;
	
	for (int tries = 25; tries != 0; --tries) {
		this->m_vecAttackSpot = area->GetRandomPoint();
		
		if (this->m_vecAttackSpot.DistToSqr(sentry->WorldSpaceCenter()) > Square(sentry->GetMaxRange()) ||
			actor->IsLineOfFireClear(sentry->WorldSpaceCenter(), this->m_vecAttackSpot)) {
			break;
		}
	}
	
	this->m_bHaveAttackSpot = true;
	
	if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
		NDebugOverlay::Cross3D(this->m_vecAttackSpot, 5.0f, NB_RGB_YELLOW, true, 60.0f);
	}
}

void CTFBotDestroyEnemySentry::ComputeSafeAttackSpot(CTFBot *actor)
{
	this->m_bHaveAttackSpot = false;
	this->m_vecAttackSpot = vec3_origin;
	
	CObjectSentrygun *sentry = this->m_hSentry;
	if (sentry == nullptr) return;
	
	sentry->UpdateLastKnownArea();
	CTFNavArea *sentry_lkarea = sentry->GetLastKnownTFArea();
	if (sentry_lkarea == nullptr) return;
	
	CUtlVector<CTFNavArea *> visible_areas;
	sentry_lkarea->ForAllPotentiallyVisibleTFAreas([&](CTFNavArea *area){
		if (!visible_areas.HasElement(area)) {
			visible_areas.AddToTail(area);
		}
		return true;
	});
	
	CUtlVector<CTFNavArea *> areas2; // TODO: name
	CUtlVector<CTFNavArea *> areas3; // TODO: name
	
	for (auto area : visible_areas) {
		Vector close; area->GetClosestPointOnArea(area->GetCenter() + (area->GetCenter() - sentry_lkarea->GetCenter()), &close);
		
		if (sentry->GetAbsOrigin().DistToSqr(close) > Square(sentry->GetMaxRange())) {
			areas2.AddToTail(area);
			
			if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
				area->DrawFilled(NB_RGBA_GREEN, 60.0f, true, 1.0f);
			}
		}
	}
	
	if (areas2.IsEmpty()) return;
	
	for (auto area : areas2) {
		Vector close; area->GetClosestPointOnArea(sentry->GetAbsOrigin(), &close);
		
		if (sentry->GetAbsOrigin().DistToSqr(close) < 1.5f * Square(sentry->GetMaxRange())) {
			areas3.AddToTail(area);
			
			if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
				area->DrawFilled(NB_RGBA_LIME, 60.0f, true, 5.0f);
			}
		}
	}
	
	CTFNavArea *area;
	if (!areas3.IsEmpty()) {
		area = areas3.Random();
	} else {
		area = areas2.Random();
	}
	
	this->m_vecAttackSpot = area->GetRandomPoint();
	this->m_bHaveAttackSpot = true;
	
	if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
		area->DrawFilled(NB_RGBA_YELLOW, 60.0f, true, 5.0f);
		NDebugOverlay::Cross3D(this->m_vecAttackSpot, 10.0f, NB_RGB_RED, true, 60.0f);
	}
}


ActionResult<CTFBot> CTFBotUberAttackEnemySentry::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->IgnoreEnemies_Set(actor, true);
	
	Continue();
}

ActionResult<CTFBot> CTFBotUberAttackEnemySentry::Update(CTFBot *actor, float dt)
{
	// NOTE: live TF2 only checks TF_COND_INVULNERABLE
	if (!actor->m_Shared.IsInvulnerable()) {
		Done("No longer uber");
	}
	
	CObjectSentrygun *sentry = this->m_hSentry;
	if (sentry == nullptr) {
		Done("Target sentry destroyed");
	}
	
	float aim_pitch;
	float aim_yaw;
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN, true) && FindGrenadeAim(actor, sentry, aim_pitch, aim_yaw)) {
		Vector aim_vectors = AngleVecFwd(QAngle(aim_pitch, aim_yaw, 0.0f));
		actor->GetBodyInterface()->AimHeadTowards(actor->EyePosition() + (5000.0f * aim_vectors), IBody::PRI_CRITICAL, 0.3f, nullptr, "Aiming at opportunistic grenade shot");
		
		if (EyeVectorsFwd(actor).Dot(aim_vectors) > 0.90f) {
			if (actor->EquipLongRangeWeapon()) {
				actor->PressFireButton();
			} else {
				SuspendFor(new CTFBotRetreatToCover(0.1f), "No suitable range weapon available right now");
			}
		}
	} else if (actor->IsLineOfFireClear(sentry)) {
		actor->GetBodyInterface()->AimHeadTowards(sentry, IBody::PRI_OVERRIDE, 1.0f, nullptr, "Aiming at target sentry");
		
		if (EyeVectorsFwd(actor).Dot((sentry->WorldSpaceCenter() - actor->EyePosition()).Normalized()) > 0.95f) {
			if (actor->EquipLongRangeWeapon()) {
				actor->PressFireButton();
			} else {
				SuspendFor(new CTFBotRetreatToCover(0.1f), "No suitable range weapon available right now");
			}
		}
		
		if (actor->IsRangeLessThan(sentry, 100.0f)) {
			Continue();
		}
	}
	
	if (!this->m_PathFollower.IsValid() || this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(1.0f);
		this->m_PathFollower.Compute(actor, sentry->WorldSpaceCenter(), CTFBotPathCost(actor, FASTEST_ROUTE));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}

void CTFBotUberAttackEnemySentry::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	this->IgnoreEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotUberAttackEnemySentry::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	this->IgnoreEnemies_Reset(actor);
	Continue();
}

ActionResult<CTFBot> CTFBotUberAttackEnemySentry::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->IgnoreEnemies_Set(actor, true);
	Continue();
}
