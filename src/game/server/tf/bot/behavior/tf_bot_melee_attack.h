/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_MELEE_ATTACK_H
#define TF_BOT_MELEE_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotChasePath.h"


class CTFBotMeleeAttack final : public Action<CTFBot>
{
public:
	CTFBotMeleeAttack(float abandon_range = -1.0f);
	virtual ~CTFBotMeleeAttack() {}
	
	virtual const char *GetName() const override { return "MeleeAttack"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	float m_flAbandonRange;
	ChasePath m_ChasePath;
};


#endif
