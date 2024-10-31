/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_defend_point.h"
#include "tf_bot_capture_point.h"
#include "tf_bot_defend_point_block_capture.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_attack.h"
#include "demoman/tf_bot_prepare_stickybomb_trap.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_control_point_master.h"


static ConVar tf_bot_defense_must_defend_time("tf_bot_defense_must_defend_time", "300", FCVAR_CHEAT, "If timer is less than this, bots will stay near point and guard");
static ConVar tf_bot_max_point_defend_range  ("tf_bot_max_point_defend_range",  "1250", FCVAR_CHEAT, "How far (in travel distance) from the point defending bots will take up positions");
static ConVar tf_bot_defense_debug           ("tf_bot_defense_debug",              "0", FCVAR_CHEAT);


class CSelectDefenseAreaForPoint : public ISearchSurroundingAreasFunctor
{
public:
	CSelectDefenseAreaForPoint(CTFNavArea *point_center_area, CUtlVector<CTFNavArea *> *p_areas, float inc_limit, int team) :
		m_PointCenterArea(point_center_area), m_Areas(p_areas), m_flIncursionLimit(inc_limit), m_iTeam(team) {}
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override
	{
		auto tf_area = static_cast<CTFNavArea *>(area);
		
		if (TFGameRules()->IsInKothMode() || tf_area->GetIncursionDistance(this->m_iTeam) <= this->m_flIncursionLimit) {
			if (area->IsPotentiallyVisible(this->m_PointCenterArea) && (this->m_PointCenterArea->GetCenter().z - area->GetCenter().z) < 220.0f) {
				this->m_Areas->AddToTail(tf_area);
			}
		}
		
		return true;
	}
	
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override
	{
		if (travelDistanceSoFar > tf_bot_max_point_defend_range.GetFloat()) return false;
		
		int team = (TFGameRules()->IsInKothMode() ? TEAM_ANY : this->m_iTeam);
		if (adjArea->IsBlocked(team)) return false;
		
		float delta_z = currentArea->ComputeAdjacentConnectionHeightChange(adjArea);
		return (abs(delta_z) < 65.0f);
	}
	
private:
	CTFNavArea *m_PointCenterArea;
	CUtlVector<CTFNavArea *> *m_Areas;
	float m_flIncursionLimit;
	int m_iTeam;
};


ActionResult<CTFBot> CTFBotDefendPoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ChasePath   .Initialize(actor);
	
	this->m_DefenseArea = nullptr;
	
	static const float roamChance[] = {
		10.0f, // easy
		50.0f, // normal
		75.0f, // hard
		90.0f, // expert
	};
	
	Assert(actor->GetSkill() >= 0 && actor->GetSkill() <= 3);
	this->m_bShouldRoam = (RandomFloat(0.0f, 100.0f) < roamChance[actor->GetSkill()]);
	
	Continue();
}

ActionResult<CTFBot> CTFBotDefendPoint::Update(CTFBot *actor, float dt)
{
	// TODO: is this necessary? looks like a pseudo-KOTH situation handler
	if (!g_hControlPointMasters.IsEmpty() && g_hControlPointMasters[0] != nullptr) {
		CTeamControlPointMaster *master = g_hControlPointMasters[0];
		if (master->GetNumPoints() == 1) {
			CTeamControlPoint *point = master->GetControlPoint(0);
			if (point->GetOwner() != actor->GetTeamNumber()) {
				ChangeTo(new CTFBotCapturePoint(), "We need to capture the point!");
			}
		}
	}
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		SuspendFor(new CTFBotSeekAndDestroy(10.0f), "Seek and destroy until a point becomes available");
	}
	
	// BUG: is point->GetTeamNumber() even correct at all?
	// whatever happened to ->GetOwner()
	if (point->GetOwner() != actor->GetTeamNumber()) {
		ChangeTo(new CTFBotCapturePoint(), "We need to capture our point(s)");
	}
	
	if (this->IsPointThreatened(actor) && this->WillBlockCapture(actor)) {
		SuspendFor(new CTFBotDefendPointBlockCapture(), "Moving to block point capture!");
	}
	
	// FIX: live TF2 only checks TF_COND_INVULNERABLE
	// however... whether this should even exist, and now whether the duration
	// is valid for non-medic-ubercharge situations, is in doubt
	if (actor->m_Shared.IsInvulnerable()) {
		SuspendFor(new CTFBotSeekAndDestroy(6.0f), "Attacking because I'm uber'd!");
	}
	
	if (point->IsLocked()) {
		SuspendFor(new CTFBotSeekAndDestroy(), "Seek and destroy until the point unlocks");
	}
	
	if (this->m_bShouldRoam && actor->GetTimeLeftToCapture() > tf_bot_defense_must_defend_time.GetFloat()) {
		SuspendFor(new CTFBotSeekAndDestroy(15.0f), "Seek and destroy - we have lots of time");
	}
	
	if (TFGameRules()->InSetup()) {
		this->m_ctSelectDefendArea.Reset();
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	actor->EquipBestWeaponForThreat(threat);
	
	if (threat != nullptr && threat->IsVisibleRecently()) {
		this->m_ctSelectDefendArea.Reset();
		
		// BUG: why is this even here when the code block right below it exists?
		if (actor->IsPlayerClass(TF_CLASS_PYRO, true)) {
			// FIX: live TF2 uses CTFBotSeekAndDestroy, which makes no sense
			SuspendFor(new CTFBotAttack(), "Going after an enemy");
		}
		
		// TODO: make this weapon choice condition into less of a hard-coded monstrosity
		CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
		if (weapon != nullptr && (weapon->IsMeleeWeapon() || weapon->IsWeapon(TF_WEAPON_FLAMETHROWER))) {
			RouteType route = (actor->IsPlayerClass(TF_CLASS_PYRO) ? SAFEST_ROUTE : FASTEST_ROUTE);
			this->m_ChasePath.Update(actor, threat->GetEntity(), CTFBotPathCost(actor, route));
			Continue();
		}
	}
	
	if (this->m_DefenseArea == nullptr || this->m_ctSelectDefendArea.IsElapsed()) {
		this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	}
	
	if (this->m_DefenseArea != nullptr) {
		if (actor->GetLastKnownTFArea() != this->m_DefenseArea) {
			VPROF_BUDGET("CTFBotDefendPoint::Update( repath )", "NextBot");
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(2.0f, 3.0f));
				
				this->m_PathFollower.Compute(actor, this->m_DefenseArea->GetCenter(), CTFBotPathCost(actor, DEFAULT_ROUTE));
			}
			
			this->m_PathFollower.Update(actor);
			
			this->m_ctSelectDefendArea.Reset();
		} else {
			if (CTFBotPrepareStickybombTrap::IsPossible(actor)) {
				SuspendFor(new CTFBotPrepareStickybombTrap(), "Laying sticky bombs!");
			}
		}
	}
	
	// not really sure why I put this assertion here, to be honest
	//Assert(false);
	Continue();
}


ActionResult<CTFBot> CTFBotDefendPoint::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	actor->ClearMyControlPoint();
	
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPoint::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Invalidate();
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPoint::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryLost(CTFBot *actor, int idx)
{
	actor->ClearMyControlPoint();
	
	this->m_PathFollower.Invalidate();
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


bool CTFBotDefendPoint::IsPointThreatened(CTFBot *actor)
{
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) return false;
	
	if (point->HasBeenContested() && (gpGlobals->curtime - point->LastContestedAt()) < 5.0f) {
		return true;
	}
	
	return actor->LostControlPointRecently();
}

CTFNavArea *CTFBotDefendPoint::SelectAreaToDefendFrom(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotDefendPoint::SelectAreaToDefendFrom", "NextBot");
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) return nullptr;
	
	CTFNavArea *point_center_area = TheTFNavMesh->GetControlPointCenterArea(point->GetPointIndex());
	if (point_center_area == nullptr) return nullptr;
	
	CUtlVector<CTFNavArea *> areas;
	CSelectDefenseAreaForPoint functor(point_center_area, &areas, point_center_area->GetIncursionDistance(actor->GetTeamNumber()), actor->GetTeamNumber());
	SearchSurroundingAreas(point_center_area, functor);
	
	if (!areas.IsEmpty()) {
		this->m_ctSelectDefendArea.Start(RandomFloat(10.0f, 20.0f));
		
		if (tf_bot_defense_debug.GetBool()) {
			for (auto area : areas) {
				area->DrawFilled(NB_RGBA_DKMAGENTA_C8, 0.1f, true, 5.0f);
			}
		}
		
		return areas.Random();
	} else {
		return nullptr;
	}
}

bool CTFBotDefendPoint::WillBlockCapture(CTFBot *actor) const
{
	if (TFGameRules()->IsInTraining()) return false;
	
	switch (actor->GetSkill()) {
	case CTFBot::EASY:
		return false;
	
	case CTFBot::NORMAL:
		return (actor->TransientlyConsistentRandomValue(10.0f) > 0.5f);
	
	case CTFBot::HARD:
	case CTFBot::EXPERT:
	default:
		return true;
	}
}
