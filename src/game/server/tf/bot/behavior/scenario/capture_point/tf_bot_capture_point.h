/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_CAPTURE_POINT_H
#define TF_BOT_CAPTURE_POINT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotCapturePoint final : public Action<CTFBot>
{
public:
	CTFBotCapturePoint() {}
	virtual ~CTFBotCapturePoint() {}
	
	virtual const char *GetName() const override { return "CapturePoint"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int idx) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
};
// TODO: remove offsets


#endif
