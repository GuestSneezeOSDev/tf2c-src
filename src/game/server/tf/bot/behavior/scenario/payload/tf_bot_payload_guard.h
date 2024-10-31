/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_PAYLOAD_GUARD_H
#define TF_BOT_PAYLOAD_GUARD_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotPayloadGuard final : public Action<CTFBot>
{
public:
	CTFBotPayloadGuard() {}
	virtual ~CTFBotPayloadGuard() {}
	
	virtual const char *GetName() const override { return "PayloadGuard"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	
	static bool IsVantagePointValid(CBaseEntity *target, const Vector& point);
	
private:
	Vector FindVantagePoint(CBaseEntity *target);
	
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
	Vector m_vecVantagePoint;            // +0x4814
	CountdownTimer m_ctFindVantagePoint; // +0x4820
	CountdownTimer m_ctBlockHoldoff;     // +0x482c
	IntervalTimer m_itLastVantagePoint;  // NEW
};
// TODO: remove offsets


#endif
