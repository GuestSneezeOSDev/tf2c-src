/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SPY_ATTACK_H
#define TF_BOT_SPY_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotChasePath.h"


class CTFBotSpyAttack final : public Action<CTFBot>
{
public:
	CTFBotSpyAttack(CTFPlayer *victim) : m_hVictim(victim) {}
	virtual ~CTFBotSpyAttack() {}
	
	virtual const char *GetName() const override { return "SpyAttack"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual ThreeState_t IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
private:
	CHandle<CTFPlayer> m_hVictim;  // +0x0034
	ChasePath m_ChasePath;         // +0x0038
	bool m_bUseRevolver;           // +0x4838
	CountdownTimer m_ctMvMChuckle; // +0x483c
	CountdownTimer m_ctRecloak;    // +0x4848
};
// TODO: remove offsets


#endif
