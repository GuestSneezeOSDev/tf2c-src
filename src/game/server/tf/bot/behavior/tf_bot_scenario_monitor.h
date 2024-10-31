/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SCENARIO_MONITOR_H
#define TF_BOT_SCENARIO_MONITOR_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotScenarioMonitor final : public Action<CTFBot>
{
public:
	CTFBotScenarioMonitor() {}
	virtual ~CTFBotScenarioMonitor() {}
	
	virtual const char *GetName() const override { return "ScenarioMonitor"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
private:
	virtual Action<CTFBot> *DesiredScenarioAndClassAction(CTFBot *actor);
	
	CountdownTimer m_ctFetchFlagInitial;
	CountdownTimer m_ctFetchFlag;
};


#endif
