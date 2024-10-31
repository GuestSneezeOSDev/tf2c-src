/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_TACTICAL_MONITOR_H
#define TF_BOT_TACTICAL_MONITOR_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "tf_obj_teleporter.h"


class CTFBotTacticalMonitor final : public Action<CTFBot>
{
public:
	CTFBotTacticalMonitor() {}
	virtual ~CTFBotTacticalMonitor() {}
	
	virtual const char *GetName() const override { return "TacticalMonitor"; }
	
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2) override;
	virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
	
private:
	void AvoidBumpingEnemies(CTFBot *actor);
	void MonitorArmedStickybombs(CTFBot *actor);
	bool ShouldOpportunisticallyTeleport(CTFBot *actor) const;
	
	CountdownTimer m_ctNonHurryStuff;        // +0x0034
	CountdownTimer m_ctHumanTauntA;          // +0x0040 TODO: name
	CountdownTimer m_ctHumanTauntB;          // +0x004c TODO: name
	CountdownTimer m_ctHumanTauntC;          // +0x0058 TODO: name
	CountdownTimer m_ctMonitorArmedStickies; // +0x0064
	CountdownTimer m_ctFindNearbyTele;       // +0x0070
};


#endif
