/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_NAV_ENT_WAIT_H
#define TF_BOT_NAV_ENT_WAIT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CFuncNavPrerequisite;


class CTFBotNavEntWait final : public Action<CTFBot>
{
public:
	CTFBotNavEntWait(CFuncNavPrerequisite *prereq) : m_hPrereq(prereq) {}
	virtual ~CTFBotNavEntWait() {}
	
	virtual const char *GetName() const override { return "NavEntWait"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x34
	CountdownTimer m_ctWait;                 // +0x38
};
// TODO: remove offsets


#endif
