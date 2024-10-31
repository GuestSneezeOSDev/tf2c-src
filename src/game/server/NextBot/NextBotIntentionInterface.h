/* NextBotIntentionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTINTENTIONINTERFACE_H
#define NEXTBOT_NEXTBOTINTENTIONINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotComponentInterface.h"
#include "NextBotContextualQueryInterface.h"


#define FOR_EACH_RESPONDER_QUERY(func, ...) \
	for (auto responder = this->FirstContainedResponder(); responder != nullptr; responder = this->NextContainedResponder(responder)) { \
		IContextualQuery *query = dynamic_cast<IContextualQuery *>(responder); \
		if (query == nullptr) continue; \
		ThreeState_t result = query->func(nextbot, ##__VA_ARGS__); \
		if (result != TRS_NONE) return result; \
	} \
	return TRS_NONE;


class IIntention : public INextBotComponent, public IContextualQuery
{
public:
	IIntention(INextBot *nextbot) :
		INextBotComponent(nextbot) {}
	virtual ~IIntention() {}
	
	virtual void Reset() override  { INextBotComponent::Reset(); }
	virtual void Update() override {}
	
	virtual ThreeState_t ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const override            { FOR_EACH_RESPONDER_QUERY(ShouldPickUp, it); }
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override                              { FOR_EACH_RESPONDER_QUERY(ShouldHurry); }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override                            { FOR_EACH_RESPONDER_QUERY(ShouldRetreat); }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override { FOR_EACH_RESPONDER_QUERY(ShouldAttack, threat); }
	virtual ThreeState_t IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override             { FOR_EACH_RESPONDER_QUERY(IsHindrance, it); }
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
	virtual ThreeState_t IsPositionAllowed(const INextBot *nextbot, const Vector& pos) const override     { FOR_EACH_RESPONDER_QUERY(IsPositionAllowed, pos); }
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
};


#endif
