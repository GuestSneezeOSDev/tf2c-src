/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_DESTROY_ENEMY_SENTRY_H
#define TF_BOT_DESTROY_ENEMY_SENTRY_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CObjectSentrygun;


class CTFBotDestroyEnemySentry final : public Action<CTFBot>
{
public:
	CTFBotDestroyEnemySentry() {}
	virtual ~CTFBotDestroyEnemySentry() {}
	
	virtual const char *GetName() const override { return "DestroyEnemySentry"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override { return TRS_FALSE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	void ComputeCornerAttackSpot(CTFBot *actor);
	void ComputeSafeAttackSpot(CTFBot *actor);
	
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
//	bool m_b4814;                        // +0x4814
	Vector m_vecAttackSpot;              // +0x4818
	bool m_bHaveAttackSpot;              // +0x4824
	bool m_bFiringAtSentry;              // +0x4825
	bool m_bHasBeenInvuln;               // +0x4826
	CHandle<CObjectSentrygun> m_hSentry; // +0x4828
};
// TODO: remove offsets


class CTFBotUberAttackEnemySentry final : public Action<CTFBot>, public CTFBotActionHelper_IgnoreEnemies
{
public:
	CTFBotUberAttackEnemySentry(CObjectSentrygun *sentry) : m_hSentry(sentry) {}
	virtual ~CTFBotUberAttackEnemySentry() {}
	
	virtual const char *GetName() const override { return "UberAttackEnemySentry"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override                              { return TRS_TRUE;  }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override                            { return TRS_FALSE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_TRUE;  }
	
private:
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
	CHandle<CObjectSentrygun> m_hSentry; // +0x4814
};
// TODO: remove offsets


#endif
