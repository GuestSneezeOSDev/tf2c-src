/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_payload_guard.h"
#include "tf_bot_payload_block.h"
#include "demoman/tf_bot_prepare_stickybomb_trap.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_train_watcher.h"
#include "tf_bot_payload_push.h"


static ConVar tf_bot_payload_guard_range               ("tf_bot_payload_guard_range",                 "1000", FCVAR_CHEAT);
static ConVar tf_bot_debug_payload_guard_vantage_points("tf_bot_debug_payload_guard_vantage_points", nullptr, FCVAR_CHEAT);


class CCollectPayloadGuardVantagePoints : public ISearchSurroundingAreasFunctor
{
public:
	CCollectPayloadGuardVantagePoints(CBaseEntity *target) :
		m_pTarget(target) {}
	virtual ~CCollectPayloadGuardVantagePoints() {}
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override
	{
		for (int tries = 3; tries != 0; --tries) {
			Vector point = area->GetRandomPoint();
			
			if (CTFBotPayloadGuard::IsVantagePointValid(this->m_pTarget, point)) {
				this->m_VantagePoints.AddToTail(point);
				if (tf_bot_debug_payload_guard_vantage_points.GetBool()) {
					NDebugOverlay::Cross3D(point, 5.0f, NB_RGB_MAGENTA, true, 120.0f);
				}
			}
		}
		
		return true;
	}
	
	Vector GetVantagePoint() const
	{
		if (!this->m_VantagePoints.IsEmpty()) {
			return this->m_VantagePoints.Random();
		} else {
			return this->m_pTarget->WorldSpaceCenter();
		}
	}
	
private:
	CBaseEntity *m_pTarget;
	CUtlVector<Vector> m_VantagePoints;
};


ActionResult<CTFBot> CTFBotPayloadGuard::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_vecVantagePoint = actor->GetAbsOrigin();
	
	Continue();
}

ActionResult<CTFBot> CTFBotPayloadGuard::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
	if (payload == nullptr) {
		Continue();
	}
	
	CBaseEntity *train = payload->GetTrainEntity();
	if (train == nullptr) {
		Continue();
	}
	
	// NOTE: live TF2 does a ton of really idiotic stuff in this AI behavior, such as making bots recompute their path
	// on EVERY SINGLE UPDATE if they don't have a clear LOS to the payload cart, and more...
	// so, much of the logic here has been rewritten to hopefully make it operate more sensibly
	
	/* if the cart is being pushed without interruption for a good while, then move to block it */
	if (!payload->IsDisabled() && payload->GetCapturerCount() > 0) {
		if (!this->m_ctBlockHoldoff.HasStarted()) {
			this->m_ctBlockHoldoff.Start(RandomFloat(0.5f, 3.0f));
		}
	} else {
		this->m_ctBlockHoldoff.Invalidate();
	}
	
	if (this->m_ctBlockHoldoff.HasStarted() && this->m_ctBlockHoldoff.IsElapsed()) {
		this->m_ctBlockHoldoff.Invalidate();
		
		/* GetCapturerCount returns -1 if the cart is being blocked already */
		if (payload->GetCapturerCount() >= 0) {
			SuspendFor(new CTFBotPayloadBlock(), "Moving to block the cart's forward motion");
		}
	}
	
	/* if we are due for a new vantage point, or if our current vantage point has become invalid, find a new one */
	if ((this->m_ctFindVantagePoint.HasStarted() && this->m_ctFindVantagePoint.IsElapsed()) || !this->IsVantagePointValid(train, this->m_vecVantagePoint)) {
		/* disallow super-frequent vantage point changes */
		if (!this->m_itLastVantagePoint.HasStarted() || this->m_itLastVantagePoint.GetElapsedTime() >= 1.0f) {
			/* find a new vantage point AND force a re-path immediately */
			this->m_vecVantagePoint = this->FindVantagePoint(train);
			this->m_ctRecomputePath.Invalidate();
		}

		m_ctFindVantagePoint.Invalidate();
	}

	/* occasionally find a new vantage point from which to guard */
	float distsqr = DistSqrXY( this->m_vecVantagePoint, actor->GetAbsOrigin() );
	if ( distsqr > Square( 25.0f ) ) {
		if ( this->m_ctRecomputePath.IsElapsed() ) {
			this->m_PathFollower.Compute( actor, this->m_vecVantagePoint, CTFBotPathCost( actor, DEFAULT_ROUTE ) );
			this->m_ctRecomputePath.Start( RandomFloat( 0.5f, 1.0f ) );
		}

		this->m_PathFollower.Update( actor );
		m_ctFindVantagePoint.Invalidate();
	}
	else {
		if ( CTFBotPrepareStickybombTrap::IsPossible( actor ) ) {
			SuspendFor( new CTFBotPrepareStickybombTrap(), "Laying sticky bombs!" );
		}

		if ( !this->m_ctFindVantagePoint.HasStarted() )
		{
			this->m_ctFindVantagePoint.Start( RandomFloat( 3.0f, 15.0f ) );
		}
	}

	// Payload Race: push when own Payload is safe
	if ( TFGameRules()->HasMultipleTrains() )
	{
		CTeamTrainWatcher *payloadPush = TFGameRules()->GetPayloadToPush( actor->GetTeamNumber() );
		if ( payloadPush != nullptr )
		{
			if ( payload->GetCapturerCount() == 0 && actor->GetTimeSinceLastInjuryByAnyEnemyTeam() > 5.0f )
			{
				ChangeTo( new CTFBotPayloadPush(), "Done guarding own cart" );
			}
		}
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotPayloadGuard::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnResume", "NextBot");
	
	this->m_ctFindVantagePoint.Invalidate();
	this->m_ctRecomputePath   .Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnMoveToFailure", "NextBot");
	
	this->m_ctFindVantagePoint.Invalidate();
	this->m_ctRecomputePath   .Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnStuck", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}


ThreeState_t CTFBotPayloadGuard::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
	if (payload != nullptr && payload->IsTrainNearCheckpoint()) {
		return TRS_FALSE;
	}
	
	return TRS_NONE;
}


bool CTFBotPayloadGuard::IsVantagePointValid(CBaseEntity *target, const Vector& point)
{
	static NextBotTraceFilterIgnoreActors filter;
	
	// TODO: magic number 62.0f (eye height or something)
	trace_t tr;
	UTIL_TraceLine(VecPlusZ(point, 62.0f), target->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &tr);
	
	return (!tr.DidHit() || tr.m_pEnt == target);
}


Vector CTFBotPayloadGuard::FindVantagePoint(CBaseEntity *target)
{
	CCollectPayloadGuardVantagePoints functor(target);
	SearchSurroundingAreas(TheTFNavMesh->GetNearestTFNavArea(target), functor);
	
	return functor.GetVantagePoint();
}
