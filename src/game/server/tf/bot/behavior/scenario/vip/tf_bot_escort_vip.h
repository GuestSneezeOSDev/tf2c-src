/* TFBot
* based on code in modern TF2, reverse engineered by sigsegv
*/


#ifndef TF_BOT_ESCORT_VIP_H
#define TF_BOT_ESCORT_VIP_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot_melee_attack.h"


class CTFBotEscortVIP final : public Action<CTFBot>
{
public:
	CTFBotEscortVIP() {}
	virtual ~CTFBotEscortVIP() {}

	virtual const char *GetName() const override { return "EscortVIP"; }

	virtual ActionResult<CTFBot> OnStart( CTFBot *actor, Action<CTFBot> *action ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *actor, float dt ) override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
};


int GetBotVIPEscortCount( int teamnum );


#endif
