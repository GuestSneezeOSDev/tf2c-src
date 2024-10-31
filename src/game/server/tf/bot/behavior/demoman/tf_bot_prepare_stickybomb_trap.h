/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_PREPARE_STICKYBOMB_TRAP_H
#define TF_BOT_PREPARE_STICKYBOMB_TRAP_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotPrepareStickybombTrap final : public Action<CTFBot>, public CTFBotActionHelper_LookAroundForEnemies
{
public:
	struct BombTargetArea
	{
		CTFNavArea *area;
		int stickies;
	};
	
	CTFBotPrepareStickybombTrap() {}
	virtual ~CTFBotPrepareStickybombTrap() {}
	
	virtual const char *GetName() const override { return "PrepareStickybombTrap"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2) override;
	
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_FALSE; }
	
	static bool IsPossible(CTFBot *actor);
	
private:
	void InitBombTargetAreas(CTFBot *actor);
	
	bool m_bReload;                               // +0x0032
	CUtlVector<BombTargetArea> m_BombTargetAreas; // +0x0038
	CountdownTimer m_ctAimTimeout;                // +0x004c
};
// TODO: remove offsets


#endif
