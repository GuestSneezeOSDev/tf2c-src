/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILDING_H
#define TF_BOT_ENGINEER_BUILDING_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CObjectSentrygun;
class CObjectDispenser;
class CTFBotHintSentrygun;


class CTFBotEngineerBuilding final : public Action<CTFBot>, public CTFBotActionHelper_LookAroundForEnemies
{
public:
	CTFBotEngineerBuilding() {}
	CTFBotEngineerBuilding(CTFBotHintSentrygun *hint) : m_hHint(hint) {}
	virtual ~CTFBotEngineerBuilding() {}
	
	virtual const char *GetName() const override { return "EngineerBuilding"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	enum
	{
		UPGRADE_REEVAL     = 0,
		UPGRADE_METAL_NOPE = 1,
		UPGRADE_METAL_YEP  = 2,
	};
	
	bool CheckIfSentryIsOutOfPosition(CTFBot *actor) const;
	bool IsMetalSourceNearby(CTFBot *actor) const;
	void UpgradeAndMaintainBuildings(CTFBot *actor);
	
	CBaseObject *ChooseBuildingToWorkOn(CTFBot *actor, CObjectSentrygun *sentry, CObjectDispenser *dispenser) const;
	
	CountdownTimer m_ct0034;               // +0x0034 TODO: name
	CountdownTimer m_ct0040;               // +0x0040 TODO: name
	CountdownTimer m_ctRecomputePath;      // +0x004c
	CountdownTimer m_ct0058;               // +0x0058 TODO: name
	int m_nAttempts;                       // +0x0064
	CountdownTimer m_ctBuildDispenser;     // +0x0068
	CountdownTimer m_ctBuildTeleporter;    // +0x0074
	PathFollower m_PathFollower;           // +0x0080
	CHandle<CTFBotHintSentrygun> m_hHint;  // +0x4854
	bool m_b4858;                          // +0x4848 TODO: name
	int m_nUpgradeStatus;                  // +0x485c
	CountdownTimer m_ctCheckOutOfPosition; // +0x4860
	bool m_b486c;                          // +0x486c TODO: name
	
	bool m_bDontLookAroundForEnemies;
};
// TODO: remove offsets


#endif
