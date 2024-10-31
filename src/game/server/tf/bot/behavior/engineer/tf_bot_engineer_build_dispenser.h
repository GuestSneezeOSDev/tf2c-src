/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILD_DISPENSER_H
#define TF_BOT_ENGINEER_BUILD_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotEngineerBuildDispenser final : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildDispenser() {}
	virtual ~CTFBotEngineerBuildDispenser() {}
	
	virtual const char *GetName() const override { return "EngineerBuildDispenser"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	CountdownTimer m_ctPlacementAim;  // +0x0034
	CountdownTimer m_ctGetAmmo;       // +0x0040
	CountdownTimer m_ctRecomputePath; // +0x004c
	int m_nAttempts;                  // +0x0058
	PathFollower m_PathFollower;      // +0x005c
};
// TODO: remove offsets


#endif
