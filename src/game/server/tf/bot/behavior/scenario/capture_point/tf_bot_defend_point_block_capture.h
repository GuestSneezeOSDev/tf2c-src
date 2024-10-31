/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
#define TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotDefendPointBlockCapture final : public Action<CTFBot>
{
public:
	CTFBotDefendPointBlockCapture() {}
	virtual ~CTFBotDefendPointBlockCapture() {}
	
	virtual const char *GetName() const override { return "DefendPointBlockCapture"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override   { return TRS_TRUE; }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override { return TRS_FALSE; }
	
private:
	bool IsPointSafe(CTFBot *actor);
	
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
	CHandle<CTeamControlPoint> m_hPoint; // +0x4814
	CTFNavArea *m_PointArea;             // +0x4818
};
// TODO: remove offsets


#endif
