/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_PAYLOAD_BLOCK_H
#define TF_BOT_PAYLOAD_BLOCK_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotPayloadBlock final : public Action<CTFBot>
{
public:
	CTFBotPayloadBlock() {}
	virtual ~CTFBotPayloadBlock() {}
	
	virtual const char *GetName() const override { return "PayloadBlock"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryContested(CTFBot *actor, int idx) override { Sustain(nullptr, SEV_MEDIUM); }
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int idx) override  { Sustain(nullptr, SEV_MEDIUM); }
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override      { Sustain(nullptr, SEV_MEDIUM); }
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	
private:
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	CountdownTimer m_ctBlockDuration; // +0x4814
};
// TODO: remove offsets


#endif
