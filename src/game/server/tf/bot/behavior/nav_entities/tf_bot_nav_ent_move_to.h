/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_NAV_ENT_MOVE_TO_H
#define TF_BOT_NAV_ENT_MOVE_TO_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CFuncNavPrerequisite;


class CTFBotNavEntMoveTo final : public Action<CTFBot>
{
public:
	CTFBotNavEntMoveTo(CFuncNavPrerequisite *prereq) : m_hPrereq(prereq) {}
	virtual ~CTFBotNavEntMoveTo() {}
	
	virtual const char *GetName() const override { return "NavEntMoveTo"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x0034
	Vector m_vecGoal;                        // +0x0038
	CNavArea *m_pGoalArea = nullptr;         // +0x0044
	CountdownTimer m_ctWait;                 // +0x0048
	PathFollower m_PathFollower;             // +0x0054
	CountdownTimer m_ctRecomputePath;        // +0x4828
};
// TODO: remove offsets


#endif
