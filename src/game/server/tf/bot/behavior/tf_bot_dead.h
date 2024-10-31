/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_DEAD_H
#define TF_BOT_DEAD_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotDead final : public Action<CTFBot>
{
public:
	CTFBotDead() {}
	virtual ~CTFBotDead() {}
	
	virtual const char *GetName() const override { return "Dead"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	IntervalTimer m_itDeathEpoch;
};


#endif
