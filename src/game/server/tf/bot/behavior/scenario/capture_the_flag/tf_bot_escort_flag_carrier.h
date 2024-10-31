/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ESCORT_FLAG_CARRIER_H
#define TF_BOT_ESCORT_FLAG_CARRIER_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot_melee_attack.h"


class CTFBotEscortFlagCarrier final : public Action<CTFBot>
{
public:
	CTFBotEscortFlagCarrier() {}
	virtual ~CTFBotEscortFlagCarrier() {}
	
	virtual const char *GetName() const override { return "EscortFlagCarrier"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	CTFBotMeleeAttack m_MeleeAttack;
};


int GetBotEscortCount(int teamnum);


#endif
