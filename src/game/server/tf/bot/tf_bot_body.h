/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_BODY_H
#define TF_BOT_BODY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotPlayerBody.h"


class CTFBot;


class CTFBotBody final : public PlayerBody
{
public:
	CTFBotBody(INextBot *nextbot);
	virtual ~CTFBotBody() {}
	
	virtual float GetHeadAimTrackingInterval() const override;

	virtual float GetMaxHeadAngularVelocity() const override;
	
private:
	CTFBot *GetTFBot() const { return this->m_pTFBot; }
	CTFBot *m_pTFBot;
};


#endif
