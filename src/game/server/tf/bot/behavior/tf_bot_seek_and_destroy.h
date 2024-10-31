/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SEEK_AND_DESTROY_H
#define TF_BOT_SEEK_AND_DESTROY_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CTFBotSeekAndDestroy final : public Action<CTFBot>
{
public:
	CTFBotSeekAndDestroy(float duration = -1.0f);
	virtual ~CTFBotSeekAndDestroy() {}
	
	virtual const char *GetName() const override { return "SeekAndDestroy"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested(CTFBot *actor, int idx) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int idx) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	
private:
	CTFNavArea *ChooseGoalArea(CTFBot *actor);
	void RecomputeSeekPath(CTFBot *actor);
	
	CTFNavArea *m_GoalArea;
	bool m_bPointLocked;
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	CountdownTimer m_ctActionDuration;
};


#endif
