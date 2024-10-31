/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_BEHAVIOR_H
#define TF_BOT_BEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotMainAction final : public Action<CTFBot>
{
public:
	CTFBotMainAction() {}
	virtual ~CTFBotMainAction() {}
	
	virtual const char *GetName() const override { return "MainAction"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	virtual EventDesiredResult<CTFBot> OnKilled(CTFBot *actor, const CTakeDamageInfo& info) override;
	virtual EventDesiredResult<CTFBot> OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
	virtual ThreeState_t IsPositionAllowed(const INextBot *nextbot, const Vector& pos) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
private:
	void Dodge(CTFBot *actor);
	void FireWeaponAtEnemy(CTFBot *actor);
	const CKnownEntity *GetHealerOfThreat(const CKnownEntity *threat) const;
	bool IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const;
	const CKnownEntity *SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const;
	const CKnownEntity *SelectMoreDangerousThreatInternal(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const;

	bool FindNavMesh( CTFBot *actor );
	void FindIncursionFrontline( CTFBot *actor );
	
	CountdownTimer m_ct0034;               // +0x0034 TODO: name
	mutable CountdownTimer m_ctSniperAim;  // +0x0040
	mutable float m_flSniperAim1;          // +0x004c TODO: name
	mutable float m_flSniperAim2;          // +0x0050 TODO: name
	float m_flSniperYawRate;               // +0x0054
	float m_flSniperLastYaw;               // +0x0058
	IntervalTimer m_itSniperRifleTrace;    // +0x005c
	int m_iDisguiseClass;                  // +0x0060
	int m_iDisguiseTeam;                   // NEW!
	bool m_bReloadingBarrage;              // +0x0064
	CHandle<CBaseEntity> m_hContactEntity; // +0x0068
	float m_flContactTime;                 // +0x006c
	IntervalTimer m_itUnderground;         // +0x0070
};
// TODO: remove offsets



#include "NextBotPathFollow.h"

class CTFBotPuppet final : public Action<CTFBot>
{
public:
	CTFBotPuppet() {}
	virtual ~CTFBotPuppet() {}
	
	virtual const char *GetName() const override { return "Puppet"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
	
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override                            { return TRS_FALSE; }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { return TRS_FALSE; }
	
private:
	PathFollower m_PathFollower;
};


#endif
