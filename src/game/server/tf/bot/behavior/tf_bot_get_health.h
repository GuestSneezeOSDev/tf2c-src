/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_GET_HEALTH_H
#define TF_BOT_GET_HEALTH_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


// TODO: reconcile the weird differences between CTFBotGetAmmo and CTFBotGetHealth


class CTFBotGetHealth final : public Action<CTFBot>
{
public:
	CTFBotGetHealth() {}
	virtual ~CTFBotGetHealth() {}

	enum
	{
		TFBOT_HEALTHSOURCE_INVALID = -1,
		TFBOT_HEALTHSOURCE_HEALTHKIT = 0,
		TFBOT_HEALTHSOURCE_DROPPEDPACK,
		TFBOT_HEALTHSOURCE_DISPENSER,
		TFBOT_HEALTHSOURCE_RESUPPLY,
	};
	
	virtual const char *GetName() const override { return "GetHealth"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	
	static bool IsPossible(CTFBot *actor);
	
	static bool IsValidHealthSource( CTFBot *actor, CBaseEntity *ent, int type );
	static int DetermineHealthSourceType( CBaseEntity *ent );

	static void CallMedic( CTFBot *actor );
	
private:
	PathFollower m_PathFollower;    // +0x0034
	CHandle<CBaseEntity> m_hHealth; // +0x4808
	int m_HealthSourceType;
	bool m_bNearDispenser;
	
	static CHandle<CBaseEntity> s_possibleHealth;
	static CTFBot *s_possibleBot;
	static int s_possibleFrame;
};
// TODO: remove offsets


#endif
