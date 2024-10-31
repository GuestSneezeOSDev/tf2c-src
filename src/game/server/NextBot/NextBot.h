/* NextBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOT_H
#define NEXTBOT_NEXTBOT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotInterface.h"


class NextBotCombatCharacter : public CBaseCombatCharacter, public INextBot
{
public:
	NextBotCombatCharacter(); // TODO (btw: should this be protected?)
	virtual ~NextBotCombatCharacter() {}
	
	DECLARE_CLASS(NextBotCombatCharacter, CBaseCombatCharacter);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DECLARE_ENT_SCRIPTDESC(NextBotCombatCharacter);
	
	/* CBaseEntity overrides */
	virtual void Spawn() override;
	virtual void SetModel(const char *szModelName) override;
	virtual void Event_Killed(const CTakeDamageInfo& info) override;
	virtual INextBot *MyNextBotPointer() override { return this; }
	virtual void Touch(CBaseEntity *pOther) override;
	virtual Vector EyePosition() override;
	virtual void PerformCustomPhysics(Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity) override;
	
	/* CBaseAnimating overrides */
	virtual void HandleAnimEvent(animevent_t *pEvent) override;
	virtual void Ignite(float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false) override;
	
	/* CBaseCombatCharacter overrides */
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info) override;
	virtual int OnTakeDamage_Dying(const CTakeDamageInfo& info) override;
	virtual bool BecomeRagdoll(const CTakeDamageInfo& info, const Vector& forceVector) override;
	virtual bool IsAreaTraversable(const CNavArea *area) const override;
	
	/* INextBotEventResponder overrides */
	virtual void OnNavAreaChanged(CNavArea *area1, CNavArea *area2) override;
	
	/* INextBot overrides */
	virtual CBaseCombatCharacter *GetEntity() const override                   { return const_cast<NextBotCombatCharacter *>(this); }
	virtual NextBotCombatCharacter *GetNextBotCombatCharacter() const override { return const_cast<NextBotCombatCharacter *>(this); }
	
	virtual void Ignite(float flFlameLifetime, CBaseEntity *pEntity);
	virtual bool IsUseableEntity(CBaseEntity *pEntity, unsigned int requiredCaps);
	virtual CBaseCombatCharacter *GetLastAttacker() const;
	
	void DoThink();
	void UseEntity(CBaseEntity *pEntity, USE_TYPE useType);
	
private:
	CHandle<CBaseCombatCharacter> m_hLastAttacker; // +0x8b0
	bool m_bModelChanged = false;                  // +0x8b4
};


#endif
