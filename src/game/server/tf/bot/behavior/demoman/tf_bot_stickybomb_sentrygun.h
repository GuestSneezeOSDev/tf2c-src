/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_STICKYBOMB_SENTRYGUN_H
#define TF_BOT_STICKYBOMB_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CObjectSentrygun;


class CTFBotStickybombSentrygun final : public Action<CTFBot>, public CTFBotActionHelper_LookAroundForEnemies
{
public:
	CTFBotStickybombSentrygun(CObjectSentrygun *sentry)                                       : m_hSentry(sentry), m_bOpportunistic(false) {}
	CTFBotStickybombSentrygun(CObjectSentrygun *sentry, float pitch, float yaw, float charge) : m_hSentry(sentry), m_bOpportunistic(true), m_flOptPitch(pitch), m_flOptYaw(yaw), m_flOptCharge(charge) {}
	virtual ~CTFBotStickybombSentrygun() {}
	
	virtual const char *GetName() const override { return "StickybombSentrygun"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override                              { return TRS_TRUE;  }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override                            { return TRS_FALSE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_FALSE; }
	
private:
	bool IsAimOnTarget(CTFBot *actor, float pitch, float yaw, float charge);
	
	CHandle<CObjectSentrygun> m_hSentry; // +0x0044
	bool m_bOpportunistic;               // +0x0040
	float m_flOptPitch;                  // +0x0034
	float m_flOptYaw;                    // +0x0038
	float m_flOptCharge;                 // +0x003c
	bool m_bReload;                      // +0x0041
	bool m_b0048;                        // +0x0048 TODO: name
	CountdownTimer m_ctAimTimeout;       // +0x004c
	bool m_b0058;                        // +0x0058 TODO: name
	Vector m_vecAimTarget;               // +0x005c
	Vector m_vec0068;                    // +0x0068 TODO: name
	float m_flChargeLevel;               // +0x0074
	// 0078 dword
};
// TODO: remove offsets


#endif
