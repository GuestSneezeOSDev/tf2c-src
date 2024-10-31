/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SPY_SAP_H
#define TF_BOT_SPY_SAP_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotSpySap final : public Action<CTFBot>, public CTFBotActionHelper_LookAroundForEnemies
{
public:
	CTFBotSpySap(CBaseObject *target) : m_hTarget(target) {}
	virtual ~CTFBotSpySap() {}
	
	virtual const char *GetName() const override { return "SpySap"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override { return TRS_FALSE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual ThreeState_t IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override;
	
private:
	bool AreAllDangerousSentriesSapped(CTFBot *actor) const;
	
	CHandle<CBaseObject> m_hTarget;   // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x0038
	PathFollower m_PathFollower;      // +0x0044
};
// TODO: remove offsets


#endif
