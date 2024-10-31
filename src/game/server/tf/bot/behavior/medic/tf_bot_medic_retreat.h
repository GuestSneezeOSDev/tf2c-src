/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_MEDIC_RETREAT_H
#define TF_BOT_MEDIC_RETREAT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotMedicRetreat final : public Action<CTFBot>
{
public:
	CTFBotMedicRetreat() {}
	virtual ~CTFBotMedicRetreat() {}
	
	virtual const char *GetName() const override { return "MedicRetreat"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_TRUE; }
	
private:
	PathFollower m_PathFollower;
	CountdownTimer m_ctLookForPatients;
};


#endif
