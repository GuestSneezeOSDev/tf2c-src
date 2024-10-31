/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ATTACK_FLAG_DEFENDERS_H
#define TF_BOT_ATTACK_FLAG_DEFENDERS_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot_attack.h"


class CTFBotAttackFlagDefenders final : public CTFBotAttack
{
public:
	CTFBotAttackFlagDefenders(float duration = -1.0f);
	virtual ~CTFBotAttackFlagDefenders() {}
	
	virtual const char *GetName() const override { return "AttackFlagDefenders"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	CountdownTimer m_ctActionDuration; // +0x9014
	CountdownTimer m_ctCheckFlag;      // +0x9020
	CHandle<CTFPlayer> m_hTarget;      // +0x902c
	PathFollower m_PathFollower;       // +0x9030
	CountdownTimer m_ctRecomputePath;  // +0xd804
};


#endif
