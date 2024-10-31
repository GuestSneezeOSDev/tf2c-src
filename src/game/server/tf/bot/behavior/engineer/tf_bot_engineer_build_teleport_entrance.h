/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRANCE_H
#define TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRANCE_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotEngineerBuildTeleportEntrance final : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildTeleportEntrance() {}
	virtual ~CTFBotEngineerBuildTeleportEntrance() {}
	
	virtual const char *GetName() const override { return "EngineerBuildTeleportEntrance"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	int m_nAttempts;
};


#endif
