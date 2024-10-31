/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_DELIVER_FLAG_H
#define TF_BOT_DELIVER_FLAG_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotDeliverFlag final : public Action<CTFBot>
{
public:
	CTFBotDeliverFlag() {}
	virtual ~CTFBotDeliverFlag() {}
	
	virtual const char *GetName() const override { return "DeliverFlag"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool UpgradeOverTime(CTFBot *actor);
	
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	float m_flDistToCapZone;
	CountdownTimer m_ctUpgrade;
	int m_iUpgradeLevel;
	CountdownTimer m_ctPulseBuff;
	
	// TODO: find a less problematic way to handle temporary bot attrs
	bool m_bAddedSuppressFire;
};


class CTFBotPushToCapturePoint final : public Action<CTFBot>
{
public:
	CTFBotPushToCapturePoint(Action<CTFBot> *done_action = nullptr) :
		m_DoneAction(done_action) {}
	virtual ~CTFBotPushToCapturePoint() {}
	
	virtual const char *GetName() const override { return "PushToCapturePoint"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2) override;
	
private:
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	Action<CTFBot> *m_DoneAction;
};


#endif
