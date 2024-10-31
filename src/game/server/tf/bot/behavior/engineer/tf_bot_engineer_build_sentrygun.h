/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
#define TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotHintSentrygun;


class CTFBotEngineerBuildSentrygun final : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildSentrygun() : m_pHint(nullptr) {}
	CTFBotEngineerBuildSentrygun(CTFBotHintSentrygun *hint) : m_pHint(hint) {}
	virtual ~CTFBotEngineerBuildSentrygun() {}
	
	virtual const char *GetName() const override { return "EngineerBuildSentryGun"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	CountdownTimer m_ctPlacementDir;  // +0x0034
	// 0040 CountdownTimer
	CountdownTimer m_ctGetAmmo;       // +0x004c
	CountdownTimer m_ctRecomputePath; // +0x0058
	// 0064 CountdownTimer
	int m_nAttempts;                  // +0x0070
	PathFollower m_PathFollower;      // +0x0074
	CTFBotHintSentrygun *m_pHint;     // +0x4848
	Vector m_vecPosition;             // +0x484c
	int m_nDir;                       // +0x4858
	// 485c bool
	// 4860 Vector?
};
// TODO: remove offsets


#endif
