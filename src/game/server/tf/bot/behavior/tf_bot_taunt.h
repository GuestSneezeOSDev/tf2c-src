/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_TAUNT_H
#define TF_BOT_TAUNT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotTaunt final : public Action<CTFBot>
{
public:
	CTFBotTaunt() {}
	virtual ~CTFBotTaunt() {}
	
	virtual const char *GetName() const override { return "Taunt"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CountdownTimer m_ctWait;
	bool m_bTaunted;
};


#endif
