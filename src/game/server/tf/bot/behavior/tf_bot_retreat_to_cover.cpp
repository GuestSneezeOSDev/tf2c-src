/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_nav_mesh.h"


static ConVar tf_bot_retreat_to_cover_range("tf_bot_retreat_to_cover_range", "1000", FCVAR_CHEAT);
static ConVar tf_bot_debug_retreat_to_cover("tf_bot_debug_retreat_to_cover",    "0", FCVAR_CHEAT);
static ConVar tf_bot_wait_in_cover_min_time("tf_bot_wait_in_cover_min_time",  "0.1", FCVAR_CHEAT);
static ConVar tf_bot_wait_in_cover_max_time("tf_bot_wait_in_cover_max_time",  "1.0", FCVAR_CHEAT);


class CTestAreaAgainstThreats : public IVision::IForEachKnownEntity
{
public:
	CTestAreaAgainstThreats(CTFBot *actor, CNavArea *area) : m_Actor(actor), m_Area(area) {}
	virtual ~CTestAreaAgainstThreats() {}
	
	virtual bool Inspect(const CKnownEntity& known) override
	{
		VPROF_BUDGET("CTestAreaAgainstThreats::Inspect", "NextBot");
		
		if (this->m_Actor->IsEnemy(known.GetEntity())) {
			CNavArea *enemy_lkarea = known.GetLastKnownArea();
			if (enemy_lkarea != nullptr && this->m_Area->IsPotentiallyVisible(enemy_lkarea)) {
				++this->m_nVisible;
			}
		}
		
		return true;
	}
	
	int GetNumVisible() const { return this->m_nVisible; }
	
private:
	CTFBot *m_Actor;
	CNavArea *m_Area;
	int m_nVisible = 0;
};


class CSearchForCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCover(CTFBot *actor) : m_Actor(actor)
	{
		if (tf_bot_debug_retreat_to_cover.GetBool()) {
			TheTFNavMesh->ClearSelectedSet();
		}
	}
	virtual ~CSearchForCover() {}
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override
	{
		VPROF_BUDGET("CSearchForCover::operator()", "NextBot");
		
		CTestAreaAgainstThreats functor(this->m_Actor, area);
		this->m_Actor->GetVisionInterface()->ForEachKnownEntity(functor);
		
		if (functor.GetNumVisible() <= this->m_nMinVisible) {
			if (functor.GetNumVisible() < this->m_nMinVisible) {
				this->m_nMinVisible = functor.GetNumVisible();
				this->m_Areas.RemoveAll();
			}
			
			this->m_Areas.AddToTail(static_cast<CTFNavArea *>(area));
		}
		
		return true;
	}
	
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override
	{
		if (travelDistanceSoFar <= tf_bot_retreat_to_cover_range.GetFloat()) {
			return (currentArea->ComputeAdjacentConnectionHeightChange(adjArea) < this->m_Actor->GetLocomotionInterface()->GetStepHeight());
		} else {
			return false;
		}
	}
	
	virtual void PostSearch() override
	{
		if (tf_bot_debug_retreat_to_cover.GetBool()) {
			for (auto area : this->m_Areas) {
				TheTFNavMesh->AddToSelectedSet(area);
			}
		}
	}
	
	const CUtlVector<CTFNavArea *>& GetAreas() const { return this->m_Areas; }
	
private:
	CTFBot *m_Actor;
	CUtlVector<CTFNavArea *> m_Areas;
	int m_nMinVisible = 9999;
};


ActionResult<CTFBot> CTFBotRetreatToCover::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_CoverArea = this->FindCoverArea(actor);
	if (this->m_CoverArea == nullptr) {
		m_CoverArea = actor->GetHomeArea();
	}
	
	if (this->m_flDuration < 0.0f) {
		this->m_flDuration = RandomFloat(tf_bot_wait_in_cover_min_time.GetFloat(), tf_bot_wait_in_cover_max_time.GetFloat());
	}
	
	this->m_ctActionDuration.Start(this->m_flDuration);
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !actor->m_Shared.IsStealthed() && actor->GetTimeSinceLastInjuryByAnyEnemyTeam() > 0.5f ) {
		actor->PressAltFireButton();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotRetreatToCover::Update(CTFBot *actor, float dt)
{
	if (actor->m_Shared.IsInvulnerable()) {
		Done("I'm invulnerable - no need to retreat!");
	}
	
	// BUG: does this even query the right thing?
	if (!this->ShouldRetreat(actor)) {
		Done("No longer need to retreat");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(true);
	actor->EquipBestWeaponForThreat(threat);

	if ( threat == nullptr )
	{
		Done( "Done retreating as threats are gone" );
	}
	
	CTFWeaponBase *primary = actor->GetTFWeapon_Primary();
	
	// TODO: rework ammo count logic
	bool reloading = false;
	if (primary != nullptr && actor->GetAmmoCount(TF_AMMO_PRIMARY) > 0 && actor->IsBarrageAndReloadWeapon(primary) && primary->Clip1() < primary->GetMaxClip1()) {
		actor->PressReloadButton();
		reloading = true;
	}
	
	if (actor->GetLastKnownTFArea() == this->m_CoverArea && threat != nullptr) {
		this->m_CoverArea = this->FindCoverArea(actor);
		if (this->m_CoverArea == nullptr) {
			Done("My cover is exposed, and there is no other cover available!");
		}
	}
	
	if (actor->GetLastKnownTFArea() != this->m_CoverArea && threat != nullptr) {
		/* start the countdown timer back to the beginning */
		this->m_ctActionDuration.Reset();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.3f, 0.5f));
			this->m_PathFollower.Compute(actor, this->m_CoverArea->GetCenter(), CTFBotPathCost(actor, RETREAT_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
		
		Continue();
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !actor->m_Shared.InCond(TF_COND_DISGUISED)) {
		Continue();
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	if (this->m_DoneAction != nullptr) {
		ChangeTo(this->m_DoneAction, "Doing given action now that I'm in cover");
	}
	
	for (int i = 0; i < actor->m_Shared.GetNumHealers(); ++i) {
		CTFPlayer *healer = ToTFPlayer(actor->m_Shared.GetHealerByIndex(i));
		if (healer == nullptr) continue;
		if ( actor->HealthFraction() >= 1.0f ) Done( "No longer need cover, I'm fully healed" );
		
		if (healer->MedicGetChargeLevel() == 1.0f) {
			TFGameRules()->VoiceCommand( actor, 1, 6 );
			Continue();
		} else if (healer->MedicGetChargeLevel() > 0.90f) {
			Continue();
		}
	}
	
	if (!reloading && this->m_ctActionDuration.IsElapsed()) {
		Done("Been in cover long enough");
	}
	
	Continue();
}


CTFNavArea *CTFBotRetreatToCover::FindCoverArea(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotRetreatToCover::FindCoverArea", "NextBot");
	
	CSearchForCover functor(actor);
	SearchSurroundingAreas(actor->GetLastKnownTFArea(), functor);
	
	if (!functor.GetAreas().IsEmpty()) {
		int idx = RandomInt(0, Min(functor.GetAreas().Count(), 10) - 1);
		return functor.GetAreas()[idx];
	} else {
		return nullptr;
	}
}
