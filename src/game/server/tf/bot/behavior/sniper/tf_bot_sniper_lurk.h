/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SNIPER_LURK_H
#define TF_BOT_SNIPER_LURK_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotHint;


class CTFBotSniperLurk final : public Action<CTFBot>
{
public:
	CTFBotSniperLurk() {}
	virtual ~CTFBotSniperLurk() {}
	
	virtual const char *GetName() const override { return "SniperLurk"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
private:
	bool FindNewHome(CTFBot *actor);
	bool FindHint(CTFBot *actor);
	
	void ReleaseHint(CTFBot *actor);
	
	CountdownTimer m_ctPatience;             // +0x0034
	CountdownTimer m_ctRecomputePath;        // +0x0040
	PathFollower m_PathFollower;             // +0x004c
	int m_iImpatience;                       // +0x4820
	Vector m_vecHome;                        // +0x4824
	bool m_bHasHome;                         // +0x4830
	bool m_bNearHome;                        // +0x4831
	CountdownTimer m_ctFindNewHome;          // +0x4834
	bool m_bOpportunistic;                   // +0x4840
	CUtlVector<CHandle<CTFBotHint>> m_Hints; // +0x4844
	CHandle<CTFBotHint> m_hHint;             // +0x4858
};
// TODO: remove offsets


#endif
