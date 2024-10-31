/* NextBotChasePath
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotChasePath.h"
#include "NextBotUtil.h"


void ChasePath::Invalidate()
{
	this->m_ctRepath.Invalidate();
	this->m_ctLifetime.Invalidate();
	
	PathFollower::Invalidate();
}


void ChasePath::Update(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec)
{
	VPROF_BUDGET("ChasePath::Update", "NextBot");
	
	Assert(this->IsInitialized());
	
	this->RefreshPath(nextbot, ent, cost_func, vec);
	PathFollower::Update(nextbot);
}

Vector ChasePath::PredictSubjectPosition(INextBot *nextbot, CBaseEntity *ent) const
{
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	Vector bot_to_ent_xy = VectorXY(ent->GetAbsOrigin() - nextbot->GetPosition());
	
	if (bot_to_ent_xy.IsLengthGreaterThan(this->GetLeadRadius())) {
		return ent->GetAbsOrigin();
	}
	
	float dist_to_ent = VectorNormalize(bot_to_ent_xy);
	float time_to_ent = 0.5f + (dist_to_ent / (loco->GetRunSpeed() + 1.0e-5f));
	
	Vector pred_move = VectorXY(ent->GetAbsVelocity()) * time_to_ent;
	if (DotProduct(pred_move, bot_to_ent_xy) < 0.0f) {
		// TODO: this 0.0f special case may not be necessary
		if (bot_to_ent_xy.Length() == 0.0f) {
			pred_move.Zero();
		} else {
			// this is some kind of matrix transformation or cross product thing
			
			float dunno = (pred_move.x * -bot_to_ent_xy.y) * (pred_move.y * bot_to_ent_xy.x);
			pred_move.x = dunno * -bot_to_ent_xy.y;
			pred_move.y = dunno *  bot_to_ent_xy.x;
		}
	}
	
	Vector pred_ent_pos = ent->GetAbsOrigin() + pred_move;
	
	float frac;
	if (pred_move.IsLengthGreaterThan(6.0f) && !loco->IsPotentiallyTraversable(ent->GetAbsOrigin(), pred_ent_pos, ILocomotion::TRAVERSE_DEFAULT, &frac)) {
		// TODO: variable names
		// also probably simplify/compactify the math here if possible
		
		float f1 =        frac;
		float f2 = 1.0f - frac;
		
		Vector v1 = f1 * pred_ent_pos;
		Vector v2 = f2 * ent->GetAbsOrigin();
		
		pred_ent_pos = v1 + v2;
	}
	
	CNavArea *pred_area = TheNavMesh->GetNearestNavArea(pred_ent_pos);
	if (pred_area == nullptr || (pred_ent_pos.z - loco->GetMaxJumpHeight()) > pred_area->GetZ(pred_ent_pos.x, pred_ent_pos.y)) {
		return ent->GetAbsOrigin();
	}
	
	return pred_ent_pos;
}

bool ChasePath::IsRepathNeeded(INextBot *nextbot, CBaseEntity *ent) const
{
	float ent_from_endpos  = nextbot->GetPosition().DistTo(ent->GetAbsOrigin());
	float ent_from_nextbot = this->GetEndPosition().DistTo(ent->GetAbsOrigin());
	
	return (ent_from_endpos > ent_from_nextbot * 0.33f);
}


void ChasePath::RefreshPath(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec)
{
	VPROF_BUDGET("ChasePath::RefreshPath", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	if (this->IsValid() && loco->IsUsingLadder()) {
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			DevMsg("%3.2f: bot(#%d) ChasePath::RefreshPath failed. Bot is on a ladder.\n", gpGlobals->curtime, nextbot->GetEntity()->entindex());
		}
		
		this->m_ctRepath.Start(1.0f);
		
		return;
	}
	
	if (ent == nullptr) {
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			DevMsg("%3.2f: bot(#%d) ChasePath::RefreshPath failed. No subject.\n", gpGlobals->curtime, nextbot->GetEntity()->entindex());
		}
		
		return;
	}
	
	if (!this->m_ctFailWait.IsElapsed()) return;
	
	if (this->m_hChaseSubject == nullptr || this->m_hChaseSubject != ent) {
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			DevMsg("%3.2f: bot(#%d) Chase path subject changed (from %p to %p).\n", gpGlobals->curtime, nextbot->GetEntity()->entindex(), this->m_hChaseSubject.Get(), ent);
		}
		
		this->Invalidate();
		this->m_ctFailWait.Invalidate();
	}
	
	if (this->IsValid() && !this->m_ctRepath.IsElapsed()) return;
	
	if (this->IsValid() && this->m_ctLifetime.HasStarted() && this->m_ctLifetime.IsElapsed()) {
		this->Invalidate();
	}
	
	if (this->IsValid() && !this->IsRepathNeeded(nextbot, ent)) return;
	
	Vector vecTarget = ent->GetAbsOrigin();
	
	bool success;
	if (this->dword_0x47fc == 0) {
		if (vec != nullptr) {
			vecTarget = *vec;
		} else {
			vecTarget = this->PredictSubjectPosition(nextbot, ent);
		}
		
		success = this->Compute(nextbot, vecTarget, cost_func, this->GetMaxPathLength());
	} else {
		if (ToBaseCombatCharacter(ent) != nullptr && ToBaseCombatCharacter(ent)->GetLastKnownArea() != nullptr) {
			success = this->Compute(nextbot, ToBaseCombatCharacter(ent), cost_func, this->GetMaxPathLength());
		} else {
			success = this->Compute(nextbot, vecTarget, cost_func, this->GetMaxPathLength());
		}
	}
	
	if (success) {
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			DevMsg("%3.2f: bot(#%d) REPATH\n", gpGlobals->curtime, nextbot->GetEntity()->entindex());
		}
		
		this->m_hChaseSubject = ent;
		
		this->m_ctRepath.Start(0.5f);
		
		float lifetime = this->GetLifetime();
		if (lifetime == 0.0f) {
			this->m_ctLifetime.Invalidate();
		} else {
			this->m_ctLifetime.Start(lifetime);
		}
	} else {
		this->m_ctFailWait.Start(nextbot->GetRangeTo(ent) / 200.0f);
		
		nextbot->OnMoveToFailure(this, INextBotEventResponder::FAIL_INVALID_PATH);
		
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			int rnd = RandomInt(0, 100);
			NDebugOverlay::HorzArrow(nextbot->GetPosition(), vecTarget, 5.0f, 0xff, rnd, rnd, 0xff, true, 90.0f);
			
			DevMsg("%3.2f: bot(#%d) REPATH FAILED\n", gpGlobals->curtime, nextbot->GetEntity()->entindex());
		}
		
		this->Invalidate();
	}
}
