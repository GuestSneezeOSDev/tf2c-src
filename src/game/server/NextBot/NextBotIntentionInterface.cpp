/* NextBotIntentionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotIntentionInterface.h"
#include "NextBotInterface.h"
#include "NextBotUtil.h"


Vector IIntention::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	for (auto responder = this->FirstContainedResponder(); responder != nullptr; responder = this->NextContainedResponder(responder)) {
		IContextualQuery *query = dynamic_cast<IContextualQuery *>(responder);
		if (query == nullptr) continue;
		
		Vector result = query->SelectTargetPoint(nextbot, them);
		if (result != vec3_origin) return result;
	}
	
	Vector mins;
	Vector maxs;
	them->CollisionProp()->WorldSpaceAABB(&mins, &maxs);
	
	float dz = (maxs.z - mins.z) * 0.7f;
	return VecPlusZ(them->GetAbsOrigin(), dz);
}

const CKnownEntity *IIntention::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	bool obsolete1 = (threat1 == nullptr || threat1->IsObsolete());
	bool obsolete2 = (threat2 == nullptr || threat2->IsObsolete());
	
	if (obsolete1 && obsolete2) {
		return nullptr;
	} else if (obsolete1) {
		return threat2;
	} else if (obsolete2) {
		return threat1;
	}
	
	for (auto responder = this->FirstContainedResponder(); responder != nullptr; responder = this->NextContainedResponder(responder)) {
		IContextualQuery *query = dynamic_cast<IContextualQuery *>(responder);
		if (query == nullptr) continue;
		
		const CKnownEntity *result = query->SelectMoreDangerousThreat(nextbot, them, threat1, threat2);
		if (result != nullptr) return result;
	}
	
	const Vector& lkpos1 = threat1->GetLastKnownPosition();
	float distsqr1 = lkpos1.DistToSqr(them->GetAbsOrigin());
	
	const Vector& lkpos2 = threat2->GetLastKnownPosition();
	float distsqr2 = lkpos2.DistToSqr(them->GetAbsOrigin());
	
	if (distsqr2 > distsqr1) {
		return threat1;
	} else {
		return threat2;
	}
}
