/* TFBot
* based on code in modern TF2, reverse engineered by sigsegv
*/

// very rough copy of CTFBotEscortFlagCarrier

#include "cbase.h"
#include "tf_bot_escort_vip.h"
#include "tf_bot_seek_and_destroy.h"
#include "scenario/capture_point/tf_bot_capture_point.h"
#include "scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "entity_capture_flag.h"
#include "tf_bot_attack.h"
#include "tf_nav_mesh.h"


ConVar tf_bot_vip_escort_max_count( "tf_bot_vip_escort_max_count", "2", FCVAR_NONE );


ActionResult<CTFBot> CTFBotEscortVIP::OnStart( CTFBot *actor, Action<CTFBot> *action )
{
	this->m_PathFollower.Initialize( actor );
	this->m_ctRecomputePath.Invalidate();

	Continue();
}

ActionResult<CTFBot> CTFBotEscortVIP::Update( CTFBot *actor, float dt )
{
	CTFTeam *pTeam = GetGlobalTFTeam( actor->GetTeamNumber() );

	CTFPlayer *vip = pTeam->GetVIP();
	if ( vip == nullptr ) {
		Done( "There is no civilian to escort" );
	}

	// Stay still while waiting for players or setup.
	// Otherwise, attacking bots get themselves stuck in corners due to setup gates.
	if ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->InSetup() ) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

		Continue();
	}

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && !threat->IsObsolete() ) {
		actor->EquipBestWeaponForThreat( threat );
		if ( actor->IsRangeLessThan( threat->GetLastKnownPosition(), 1500.0f ) ) {
			SuspendFor( new CTFBotAttack(), "Help VIP by going after an enemy" );
		}
	}

	if ( this->m_ctRecomputePath.IsElapsed() ) {
		if ( ( gpGlobals->curtime - actor->GetSpawnTime() > 60.0f && actor->GetTimeSinceLastInjuryByAnyEnemyTeam() > 15.0f && actor->GetTimeSinceWeaponFired() > 15.0f ) ||
			GetBotVIPEscortCount( actor->GetTeamNumber() ) > tf_bot_vip_escort_max_count.GetInt() ) {
			// either too many escorts or I'm not seeing any action.
			// let's try doing the same objective the VIP probably is:

			// are there CPs?
			CUtlVector<CTeamControlPoint *> points_capture;
			TFGameRules()->CollectCapturePoints( actor, &points_capture );

			if ( !points_capture.IsEmpty() ) {
				SuspendFor( new CTFBotCapturePoint(), "Help VIP capture a point" );
			}

			// are there flags to capture?
			CCaptureFlag *flag = actor->GetFlagToFetch();
			if ( flag != nullptr ) {
				if ( flag->GetOwnerEntity() != nullptr ) {
					SuspendFor( new CTFBotPushToCapturePoint(), "Help VIP push flag cap zone");
				}
				else {
					SuspendFor( new CTFBotFetchFlag(), "Help VIP fetch flag");
				}
			}
			
			SuspendFor( new CTFBotSeekAndDestroy( 30.0f ), "Help VIP by finding enemies" );
		}

		CUtlVector<CTFNavArea *> escortAreas;
		CTFNavArea *vipArea = vip->GetLastKnownTFArea();
		if ( vipArea != nullptr ) {
			CUtlVector<CTFNavArea *> nearAreas;
			CollectSurroundingTFAreas( &nearAreas, vipArea, 450.0f, actor->GetLocomotionInterface()->GetStepHeight(), actor->GetLocomotionInterface()->GetStepHeight() );

			for ( auto area : nearAreas ) {
				if ( area != nullptr )
					escortAreas.AddToTail( area );
			}

			this->m_PathFollower.Compute( actor, escortAreas.Random()->GetRandomPoint(), CTFBotPathCost( actor, DEFAULT_ROUTE ) );
		}
		else
		{
			this->m_PathFollower.Compute( actor, vip, CTFBotPathCost( actor, DEFAULT_ROUTE ) );
		}

		this->m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	this->m_PathFollower.Update( actor );

	Continue();
}


int GetBotVIPEscortCount( int teamnum )
{
	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers( &teammates, teamnum, true );

	int count = 0;
	for ( auto teammate : teammates ) {
		CTFBot *bot = ToTFBot( teammate );
		if ( bot == nullptr ) continue;

		auto behavior = bot->GetBehavior();
		if ( behavior == nullptr ) continue;

		auto action = behavior->FirstContainedResponder();
		if ( action == nullptr ) continue;

		while ( action->FirstContainedResponder() != nullptr ) {
			action = action->FirstContainedResponder();
		}

		if ( assert_cast<Action<CTFBot> *>( action )->IsNamed( "EscortVIP" ) ) {
			++count;
		}
	}

	return count;
}
