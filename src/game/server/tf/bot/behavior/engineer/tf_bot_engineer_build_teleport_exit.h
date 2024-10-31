/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILD_TELEPORT_EXIT_H
#define TF_BOT_ENGINEER_BUILD_TELEPORT_EXIT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotEngineerBuildTeleportExit final : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildTeleportExit() : m_bPrecise(false) {}
	CTFBotEngineerBuildTeleportExit(const Vector& pos, float yaw) : m_bPrecise(true), m_vecPosition(pos), m_flYaw(yaw) {}
	virtual ~CTFBotEngineerBuildTeleportExit() {}
	
	virtual const char *GetName() const override { return "EngineerBuildTeleportExit"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	bool m_bPrecise;                  // +0x4808
	Vector m_vecPosition;             // +0x480c
	float m_flYaw;                    // +0x4818
	CountdownTimer m_ctImpatience;    // +0x481c
	CountdownTimer m_ctRecomputePath; // +0x4828
	CountdownTimer m_ctGetAmmo;       // +0x4834
	CountdownTimer m_ctPlacementAim;  // +0x4840
};
// TODO: remove offsets


#endif
