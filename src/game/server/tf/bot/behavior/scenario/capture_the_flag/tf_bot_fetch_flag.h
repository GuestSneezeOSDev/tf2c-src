/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_FETCH_FLAG_H
#define TF_BOT_FETCH_FLAG_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotFetchFlag final : public Action<CTFBot>
{
public:
	CTFBotFetchFlag(bool give_up_when_done = false) :
		m_bGiveUpWhenDone(give_up_when_done) {}
	virtual ~CTFBotFetchFlag() {}
	
	virtual const char *GetName() const override { return "FetchFlag"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ThreeState_t ShouldHurry( const INextBot *nextbot ) const override;
	virtual ThreeState_t ShouldRetreat( const INextBot *nextbot ) const override;
	
private:
	bool m_bGiveUpWhenDone;
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
};


#endif
