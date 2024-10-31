/* NextBotPathFollow
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_PATH_NEXTBOTPATHFOLLOW_H
#define NEXTBOT_PATH_NEXTBOTPATHFOLLOW_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotPath.h"


class PathFollower : public Path
{
public:
	enum ProgressType
	{
		PROGRESS_OK  = 0,
		PROGRESS_BAD = 1,
	};
	
	PathFollower() {}
	virtual ~PathFollower();
	
	virtual const Segment *GetCurrentGoal() const override { return this->m_CurrentGoal; }
	
	virtual void Invalidate() override;
	
	virtual void Draw(const Segment *seg) const override;
	
	virtual void OnPathChanged(INextBot *nextbot, ResultType rtype) override { this->m_CurrentGoal = this->FirstSegment(); }
	
	virtual void Update(INextBot *nextbot);
	virtual void SetMinLookAheadDistance(float dist) { this->m_flMinLookAheadDistance = dist; }
	virtual CBaseEntity *GetHindrance() const        { return this->m_hHindrance; }
	virtual bool IsDiscontinuityAhead(INextBot *nextbot, SegmentType type, float max_dist) const;
	
	void Initialize(INextBot *nextbot);
	
protected:
	bool IsInitialized() const { return this->m_bInitialized; }
	
private:
	void AdjustSpeed(INextBot *nextbot);
	Vector Avoid(INextBot *nextbot, const Vector& goal, const Vector& dir, const Vector& norm);
	ProgressType CheckProgress(INextBot *nextbot);
	bool Climbing(INextBot *nextbot, const Segment *seg, const Vector& dir, const Vector& norm, float len);
	CBaseEntity *FindBlocker(INextBot *nextbot);
	bool GapJumping(INextBot *nextbot, const Segment *seg);
	bool IsAtGoal(INextBot *nextbot) const;
	bool JumpOverGaps(INextBot *nextbot, const Segment *seg, const Vector& dir, const Vector& norm, float len);
	bool LadderUpdate(INextBot *nextbot);
	
	bool m_bInitialized = false;
	const Segment *m_CurrentGoal = nullptr; // +0x4754
	float m_flMinLookAheadDistance = -1.0f; // +0x4758
	// 475c: entirely unused
	CountdownTimer m_ctUnknown1;            // +0x4760
	CountdownTimer m_ctUnknown2;            // +0x476c
	CHandle<CBaseEntity> m_hHindrance;      // +0x4778
	mutable bool m_bAvoidA = false;         // +0x477c
	Vector m_vecAvoid1Start;                // +0x4780
	Vector m_vecAvoid1End;                  // +0x478c
	bool m_bAvoidB;                         // +0x4798
	Vector m_vecAvoid2Start;                // +0x479c
	Vector m_vecAvoid2End;                  // +0x47a8
	bool m_bAvoidC;                         // +0x47b4
	Vector m_vecAvoidMins;                  // +0x47b8
	Vector m_vecAvoidMaxs;                  // +0x47c4
	float m_flGoalTolerance = 50.0f;        // +0x47d0; was 25.0
};
// TODO: parameter names for all functions
// TODO: remove variable offsets when done RE'ing this class
// TODO: rename countdown timers
// TODO: rename m_bAvoid*
// TODO: rename m_vecAvoid1*
// TODO: rename m_vecAvoid2*


#endif
