/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_nav_ent_wait.h"
#include "func_nav_prerequisite.h"


ActionResult<CTFBot> CTFBotNavEntWait::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed before we started");
	}
	
	this->m_ctWait.Start(prereq->GetTaskValue());
	
	Continue();
}

ActionResult<CTFBot> CTFBotNavEntWait::Update(CTFBot *actor, float dt)
{
	CFuncNavPrerequisite *prereq = this->m_hPrereq;
	if (prereq == nullptr) {
		Done("Prerequisite has been removed");
	}
	
	if (prereq->IsDisabled()) {
		Done("Prerequisite has been disabled");
	}
	
	if (this->m_ctWait.IsElapsed()) {
		Done("Wait time elapsed");
	}
	
	Continue();
}
