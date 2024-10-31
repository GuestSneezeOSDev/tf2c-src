/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_DEFEND_POINT_H
#define TF_BOT_DEFEND_POINT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotChasePath.h"


class CTFBotDefendPoint final : public Action<CTFBot>
{
public:
	CTFBotDefendPoint() {}
	virtual ~CTFBotDefendPoint() {}
	
	virtual const char *GetName() const override { return "DefendPoint"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override;
	
private:
	bool IsPointThreatened(CTFBot *actor);
	CTFNavArea *SelectAreaToDefendFrom(CTFBot *actor);
	bool WillBlockCapture(CTFBot *actor) const;
	
	PathFollower m_PathFollower;         // +0x0034
	ChasePath m_ChasePath;               // +0x4808
	CountdownTimer m_ctRecomputePath;    // +0x9008
//	CountdownTimer m_ctUnknown9014;      // +0x9014 TODO: unused??
	CountdownTimer m_ctSelectDefendArea; // +0x9020
	CTFNavArea *m_DefenseArea;           // +0x902c
	bool m_bShouldRoam;                  // +0x9030
};
// TODO: remove offsets


#endif
