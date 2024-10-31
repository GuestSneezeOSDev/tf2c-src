/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_NAV_ENT_DESTROY_ENTITY_H
#define TF_BOT_NAV_ENT_DESTROY_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CFuncNavPrerequisite;
class CTFPipebombLauncher;


class CTFBotNavEntDestroyEntity final : public Action<CTFBot>, public CTFBotActionHelper_IgnoreEnemies
{
public:
	CTFBotNavEntDestroyEntity(CFuncNavPrerequisite *prereq) : m_hPrereq(prereq) {}
	virtual ~CTFBotNavEntDestroyEntity() {}
	
	virtual const char *GetName() const override { return "NavEntDestroyEntity"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	void DetonateStickiesWhenSet(CTFBot *actor, CTFPipebombLauncher *launcher) const;
	
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x0034
	PathFollower m_PathFollower;             // +0x0038
	CountdownTimer m_ctRecomputePath;        // +0x480c
	bool m_bIgnoringEnemies;                 // +0x4818
	bool m_bFireToggle;                      // +0x4819
};
// TODO: remove offsets


#endif
