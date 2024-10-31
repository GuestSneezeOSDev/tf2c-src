/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_fetch_flag.h"
#include "tf_bot_attack_flag_defenders.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "scenario/capture_point/tf_bot_capture_point.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_nav_mesh.h"
#include "demoman/tf_bot_prepare_stickybomb_trap.h"


ActionResult<CTFBot> CTFBotFetchFlag::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctRecomputePath.Invalidate();
	
	Continue();
}

// TODO: make result messages more consistent / remove typos
ActionResult<CTFBot> CTFBotFetchFlag::Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if (flag == nullptr) {
		if (TFGameRules()->IsMannVsMachineMode()) {
			SuspendFor(new CTFBotAttackFlagDefenders(), "Flag flag exists - Attacking the enemy flag defenders");
		} else if ( TFGameRules()->IsInHybridCTF_CPMode() ) {
			CTeamControlPoint *point = actor->GetMyControlPoint();
			if ( point == nullptr ) {
				SuspendFor( new CTFBotSeekAndDestroy( 10.0f ), "Seek and destroy until a point becomes available" );
			}
			if ( point->GetOwner() != actor->GetTeamNumber() ) {
				ChangeTo( new CTFBotCapturePoint(), "We need to capture our point(s)" );
			}
		} else {
			Done("No flag");
		}
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}

	CCaptureZone *pFlagDefendZone = actor->GetFlagCaptureZone();
	if ( pFlagDefendZone )
	{
		CTFNavArea *pFlagDefendArea = TheTFNavMesh->GetTFNavArea( pFlagDefendZone->WorldSpaceCenter() );
		if ( pFlagDefendArea && actor->IsRangeLessThan( pFlagDefendArea->GetCenter(), 1024.0f ) )
		{
			if ( CTFBotPrepareStickybombTrap::IsPossible( actor ) ) {
				SuspendFor( new CTFBotPrepareStickybombTrap(), "Laying sticky bombs in own flag room!" );
			}
		}
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
	if (carrier != nullptr) {
		if (this->m_bGiveUpWhenDone) {
			Done("Someone else picked up the flag");
		} else {
			SuspendFor(new CTFBotAttackFlagDefenders(), "Someone has the flag - attacking the enemy defenders");
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		if (!this->m_PathFollower.Compute(actor, flag->WorldSpaceCenter(), CTFBotPathCost(actor, DEFAULT_ROUTE)) && flag->IsDropped()) {
			SuspendFor(new CTFBotAttackFlagDefenders(RandomFloat(5.0f, 10.0f)), "Flag unreachable - Attacking");
		}
		
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}

ThreeState_t CTFBotFetchFlag::ShouldRetreat( const INextBot *nextbot ) const
{
	auto actor = static_cast<CTFBot *>( nextbot->GetEntity() );
	CCaptureFlag *flag = actor->GetFlagToFetch();

	if ( actor->IsPlayerClass( TF_CLASS_CIVILIAN, true ) ) return TRS_NONE;

	if ( actor->HasTheFlag() ) return TRS_FALSE;
	if ( flag != nullptr && actor->IsRangeLessThan( flag->GetAbsOrigin(), 750.0f ) ) return TRS_FALSE;

	return TRS_NONE;
}

ThreeState_t CTFBotFetchFlag::ShouldHurry( const INextBot *nextbot ) const
{
	auto actor = static_cast<CTFBot *>( nextbot->GetEntity() );
	CCaptureFlag *flag = actor->GetFlagToFetch();

	if ( actor->IsPlayerClass( TF_CLASS_CIVILIAN, true ) ) return TRS_NONE;

	if ( actor->HasTheFlag() ) return TRS_TRUE;
	if ( flag != nullptr && actor->IsRangeLessThan( flag->GetAbsOrigin(), 750.0f ) ) return TRS_TRUE;

	return TRS_NONE;
}
