/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_payload_push.h"
#include "tf_gamerules.h"
#include "tf_train_watcher.h"
#include "tf_bot_payload_guard.h"
#include "tf_bot_seek_and_destroy.h"


static ConVar tf_bot_cart_push_radius("tf_bot_cart_push_radius", "112", FCVAR_NONE );


ActionResult<CTFBot> CTFBotPayloadPush::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);

	this->m_ctTimeSpentPushing.Start( RandomFloat( 3.0f, 15.0f ) );
	
	Continue();
}

ActionResult<CTFBot> CTFBotPayloadPush::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (TFGameRules()->InSetup()) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		Continue();
	}
	
	CTeamTrainWatcher *payload = TFGameRules()->GetPayloadToPush(actor->GetTeamNumber());
	if (payload != nullptr) {
		// We're not needed, roam instead
		if ( payload->GetCapturerCount() >= 3 )
		{
			SuspendFor( new CTFBotSeekAndDestroy( 10.0f ), "Payload is pushing at max speed - go roam" );
		}

		CBaseEntity *train = payload->GetTrainEntity();
		if (train != nullptr) {
			if (this->m_ctRecomputePath.IsElapsed()) {
				VPROF_BUDGET("CTFBotPayloadPush::Update( repath )", "NextBot");
				
				Vector goal = train->WorldSpaceCenter() - (tf_bot_cart_push_radius.GetFloat() * GetVectorsFwd(train));
				
				if (threat != nullptr) {
					Vector dir_xy = VectorXY(train->WorldSpaceCenter() - threat->GetLastKnownPosition()).Normalized();
					goal = train->WorldSpaceCenter() + (tf_bot_cart_push_radius.GetFloat() * dir_xy);
				}
				
				this->m_PathFollower.Compute(actor, goal, CTFBotPathCost(actor, DEFAULT_ROUTE));
				
				this->m_ctRecomputePath.Start(RandomFloat(0.2f, 0.4f));
			}
			
			this->m_PathFollower.Update(actor);
		}
	}

	// Payload Race: switch to defense when behind
	if ( TFGameRules()->HasMultipleTrains() )
	{
		CTeamTrainWatcher *payloadBlock = TFGameRules()->GetPayloadToBlock( actor->GetTeamNumber() );
		float progress = payload->GetTrainProgress();
		float enemyProgress = payloadBlock->GetTrainProgress();

		if ( payloadBlock != nullptr )
		{
			if ( payloadBlock->GetCapturerCount() >= 0 && enemyProgress > progress )
			{
				if ( actor->TransientlyConsistentRandomValue( 3.0f ) < enemyProgress ) {
					if ( this->m_ctTimeSpentPushing.IsElapsed() ) {
						ChangeTo( new CTFBotPayloadGuard(), "Have to defend own cart first" );
					}
				}
			}
		}
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotPayloadPush::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnResume", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadPush::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnMoveToFailure", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadPush::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnStuck", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus();
	
	Continue();
}
