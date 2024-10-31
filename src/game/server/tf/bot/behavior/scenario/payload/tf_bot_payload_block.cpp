/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_payload_block.h"
#include "tf_gamerules.h"
#include "tf_train_watcher.h"


ActionResult<CTFBot> CTFBotPayloadBlock::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	this->m_ctBlockDuration.Start(RandomFloat(3.0f, 5.0f));
	
	Continue();
}

ActionResult<CTFBot> CTFBotPayloadBlock::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctBlockDuration.IsElapsed()) {
		Done("Been blocking long enough");
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		VPROF_BUDGET("CTFBotPayloadBlock::Update( repath )", "NextBot");
		
		CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
		if (payload == nullptr) {
			Done("Train Watcher is missing");
		}
		
		CBaseEntity *train = payload->GetTrainEntity();
		if (train == nullptr) {
			Done("Cart is missing");
		}

		// Don't count block duration if we haven't actually been blocking yet
		if ( actor->GetDistanceBetween( train ) > 112.0f )
		{
			m_ctBlockDuration.Reset();
		}
		
		this->m_PathFollower.Compute(actor, train->WorldSpaceCenter(), CTFBotPathCost(actor, DEFAULT_ROUTE));
		
		this->m_ctRecomputePath.Start(RandomFloat(0.2f, 0.4f));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


ActionResult<CTFBot> CTFBotPayloadBlock::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnResume", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnMoveToFailure", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnStuck", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}
