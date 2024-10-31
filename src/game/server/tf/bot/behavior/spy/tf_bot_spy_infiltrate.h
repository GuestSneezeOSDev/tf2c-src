/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SPY_INFILTRATE_H
#define TF_BOT_SPY_INFILTRATE_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotSpyInfiltrate final : public Action<CTFBot>
{
public:
	CTFBotSpyInfiltrate() {}
	virtual ~CTFBotSpyInfiltrate() {}
	
	virtual const char *GetName() const override { return "SpyInfiltrate"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int idx) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override;
	
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_FALSE; }
	
private:
	bool FindHidingSpot(CTFBot *actor);
	
	CountdownTimer m_ctRecomputePath;  // +0x0034
	PathFollower m_PathFollower;       // +0x0040
	CTFNavArea *m_HidingArea;          // +0x4814
	CountdownTimer m_ctFindHidingArea; // +0x4818
	CountdownTimer m_ctWait;           // +0x4824
	bool m_bCloaked;                   // +0x4830
};
// TODO: remove offsets


#endif
