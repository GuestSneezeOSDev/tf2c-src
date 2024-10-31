/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_RETREAT_TO_COVER_H
#define TF_BOT_RETREAT_TO_COVER_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotRetreatToCover final : public Action<CTFBot>
{
public:
	CTFBotRetreatToCover(float duration = -1.0f) : m_flDuration(duration), m_DoneAction(nullptr) {}
	CTFBotRetreatToCover(Action<CTFBot> *done_action) : m_flDuration(-1.0f), m_DoneAction(done_action) {}
	virtual ~CTFBotRetreatToCover() {}
	
	virtual const char *GetName() const override { return "RetreatToCover"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	
private:
	CTFNavArea *FindCoverArea(CTFBot *actor);
	
	float m_flDuration;
	Action<CTFBot> *m_DoneAction;
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	CTFNavArea *m_CoverArea;
	CountdownTimer m_ctActionDuration;
};


#endif
