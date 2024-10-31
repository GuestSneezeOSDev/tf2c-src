/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_MEDIC_HEAL_H
#define TF_BOT_MEDIC_HEAL_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotChasePath.h"


class CTFBotMedicHeal final : public Action<CTFBot>
{
public:
	CTFBotMedicHeal() {}
	virtual ~CTFBotMedicHeal() {}
	
	// TODO: change to "MedicHeal" for consistency?
	virtual const char *GetName() const override { return "Heal"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int emoteconcept) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override;
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	CTFPlayer *SelectPatient(CTFBot *actor, CTFPlayer *old_patient);
	void ComputeFollowPosition(CTFBot *actor);
	
	bool IsStable(CTFPlayer *player) const;
	bool IsGoodUberTarget(CTFPlayer *player) const;
	bool IsVisibleToEnemy(CTFBot *actor, const Vector& vec) const;
	bool IsReadyToDeployUber(const ITFHealingWeapon *medigun) const;
	bool IsDeployingUber( const ITFHealingWeapon *medigun ) const;
	
	ChasePath m_ChasePath;         // +0x0034
	CountdownTimer m_ct4834;       // +0x4834 TODO: name
	CountdownTimer m_ctUberDelay;  // +0x4840
	CHandle<CTFPlayer> m_hPatient; // +0x484c
	Vector m_vecPatientPosition;   // +0x4850
	CountdownTimer m_ct485c;       // +0x485c TODO: name
	// 0x4868 dword
	CountdownTimer m_ct486c;       // +0x486c TODO: name
	PathFollower m_PathFollower;   // +0x4878
	Vector m_vecFollowPosition;    // +0x904c
};
// TODO: remove offsets


#endif
