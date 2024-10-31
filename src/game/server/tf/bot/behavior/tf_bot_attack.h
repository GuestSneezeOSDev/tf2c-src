/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ATTACK_H
#define TF_BOT_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotChasePath.h"


/* not a final class (CTFBotAttackFlagDefenders is derived from this) */
class CTFBotAttack : public Action<CTFBot>
{
public:
	CTFBotAttack() {}
	virtual ~CTFBotAttack() {}
	
	virtual const char *GetName() const override { return "Attack"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	PathFollower m_PathFollower;
	ChasePath m_ChasePath;
	CountdownTimer m_ctRecomputePath;
};


#endif
