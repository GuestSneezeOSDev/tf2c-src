/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_taunt.h"


// to some extent, this is based on the 20091217a version, because taunts
// weren't so damn complicated back then


ActionResult<CTFBot> CTFBotTaunt::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctWait.Start(RandomFloat(0.0f, 1.0f));
	this->m_bTaunted = false;
	
	Continue();
}

ActionResult<CTFBot> CTFBotTaunt::Update(CTFBot *actor, float dt)
{
	/* wait a bit before starting */
	if (!this->m_ctWait.IsElapsed()) {
		Continue();
	}
	
	/* start the taunt */
	if (!this->m_bTaunted) {
		actor->Taunt();
		this->m_bTaunted = true;
		
		Continue();
	}
	
	/* don't end the action until the taunt is done */
	if (actor->IsTaunting()) {
		Continue();
	}
	
	Done("Taunt finished");
}
