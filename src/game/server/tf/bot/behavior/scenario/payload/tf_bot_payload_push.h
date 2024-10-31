/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_PAYLOAD_PUSH_H
#define TF_BOT_PAYLOAD_PUSH_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotPayloadPush final : public Action<CTFBot>
{
public:
	CTFBotPayloadPush() {}
	virtual ~CTFBotPayloadPush() {}
	
	virtual const char *GetName() const override { return "PayloadPush"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	CountdownTimer m_ctTimeSpentPushing;
};
// TODO: remove offsets


#endif
