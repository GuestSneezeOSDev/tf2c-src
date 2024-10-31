/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ENGINEER_BUILD_H
#define TF_BOT_ENGINEER_BUILD_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotEngineerBuild final : public Action<CTFBot>
{
public:
	CTFBotEngineerBuild() {}
	virtual ~CTFBotEngineerBuild() {}
	
	virtual const char *GetName() const override { return "EngineerBuild"; }
	
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
};


#endif
