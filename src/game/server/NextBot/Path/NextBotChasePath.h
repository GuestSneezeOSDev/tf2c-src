/* NextBotChasePath
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_PATH_NEXTBOTCHASEPATH_H
#define NEXTBOT_PATH_NEXTBOTCHASEPATH_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotPathFollow.h"


class ChasePath final : public PathFollower
{
public:
	ChasePath() {} // TODO: 47fc initializer?
	virtual ~ChasePath() {}
	
	virtual void Invalidate() override;
	
	virtual void Update(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec = nullptr);
	virtual float GetLeadRadius() const    { return 500.0f; } // TODO: potentially re-evaluate this
	virtual float GetMaxPathLength() const { return 0.0f; }
	virtual Vector PredictSubjectPosition(INextBot *nextbot, CBaseEntity *ent) const;
	virtual bool IsRepathNeeded(INextBot *nextbot, CBaseEntity *ent) const;
	virtual float GetLifetime() const { return 0.0f; }
	
private:
	void RefreshPath(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec);
	
	CountdownTimer m_ctFailWait; // +0x47d4
	CountdownTimer m_ctRepath;   // +0x47e0
	CountdownTimer m_ctLifetime; // +0x47ec
	CHandle<CBaseEntity> m_hChaseSubject;
	int dword_0x47fc = 0;        // +0x47fc dword 0 or 1 // TODO: name; also: initialization from ctor...?
};
// TODO: remove offsets


#endif
