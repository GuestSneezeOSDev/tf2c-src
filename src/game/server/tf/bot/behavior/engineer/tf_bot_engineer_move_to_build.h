/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_MOVE_TO_BUILD_H
#define TF_BOT_ENGINEER_MOVE_TO_BUILD_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"
#include "tf_sentrygun.h"


class CTFBotEngineerMoveToBuild final : public Action<CTFBot>
{
public:
	CTFBotEngineerMoveToBuild() {}
	virtual ~CTFBotEngineerMoveToBuild() {}
	
	virtual const char *GetName() const override { return "EngineerMoveToBuild"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int idx) override;
	
private:
	void CollectBuildAreas(CTFBot *actor);
	void SelectBuildLocation(CTFBot *actor);
	
	CHandle<CTFBotHintSentrygun> m_hBuildLocation; // +0x0034
	Vector m_vecBuildLocation;                     // +0x0038
	PathFollower m_PathFollower;                   // +0x0044
	CountdownTimer m_ctRecomputePath;              // +0x4818
	CUtlVector<CTFNavArea *> m_BuildAreas;         // +0x4824
	float m_flTotalArea;                           // +0x4838
	CountdownTimer m_ctLostControlPoint;           // +0x483c
};
// TODO: remove offsets


#endif
