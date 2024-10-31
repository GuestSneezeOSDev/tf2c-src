/* NextBotLocomotionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTLOCOMOTIONINTERFACE_H
#define NEXTBOT_NEXTBOTLOCOMOTIONINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotComponentInterface.h"


class CNavLadder;


// TODO: parameter names
class ILocomotion : public INextBotComponent
{
public:
	enum TraverseWhenType
	{
		// TODO: improve these names a bit
		TRAVERSE_DEFAULT   = 0,
		TRAVERSE_BREAKABLE = 1,
		
		
		// preliminary findings:
		// (TraverseWhenType)0 => default traversal
		// (TraverseWhenType)1 => path obstacle avoidance traversal
		
		// further findings: (TraverseWhenType)1 seems to ultimately only really
		// affect ILocomotion::IsEntityTraversable, where it changes the logic
		// so that breakable/damageable entities are deemed traversable
		
		
		// NextBotTraversableTraceFilter ctor ==================================
		// (TraverseWhenType)0 in ILocomotion::AdjustPosture
		// (TraverseWhenType)0 in NextBotGroundLocomotion::ResolveCollision (case A)
		// (TraverseWhenType)0 in NextBotGroundLocomotion::ResolveCollision (case B)
		// (TraverseWhenType)0 in PathFollower::JumpOverGaps
		// (TraverseWhenType)0 in PathFollower::Climbing
		// (TraverseWhenType)1 in PathFollower::Avoid
		
		// ILocomotion::IsPotentiallyTraversable ===============================
		// TODO: find all callers
		
		// ILocomotion::IsEntityTraversable ====================================
		// TODO: find all callers
		
		
		// TODO: Left4Dead
		// TODO: Left4Dead2
		// TODO: GarrysMod
	};
	
	ILocomotion(INextBot *nextbot) :
		INextBotComponent(nextbot) { this->Reset(); }
	virtual ~ILocomotion() {}
	
	virtual void OnLeaveGround(CBaseEntity *ent) override  {}
	virtual void OnLandOnGround(CBaseEntity *ent) override {}
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual void Approach(const Vector& dst, float f1 = 1.0f) { this->m_itApproach.Start(); }
	virtual void DriveTo(const Vector& dst)                   { this->m_itApproach.Start(); }
	
	virtual bool ClimbUpToLedge(const Vector& dst, const Vector& dir, const CBaseEntity *ent = nullptr) { return true; }
	virtual void JumpAcrossGap(const Vector& dst, const Vector& dir)                                    {}
	virtual void Jump()                                                                                 {}
	virtual bool IsClimbingOrJumping() const                                                            { return false; }
	virtual bool IsClimbingUpToLedge() const                                                            { return false; }
	virtual bool IsJumpingAcrossGap() const                                                             { return false; }
	virtual bool IsScrambling() const;
	
	virtual void Run()             {}
	virtual void Walk()            {}
	virtual void Stop()            {}
	virtual bool IsRunning() const { return false; }
	
	virtual void SetDesiredSpeed(float speed) {}
	virtual float GetDesiredSpeed() const     { return 0.0f; }
	virtual void SetSpeedLimit(float limit)   {}
	virtual float GetSpeedLimit() const       { return 1000.0f; }
	
	virtual bool IsOnGround() const                     { return false; }
	virtual CBaseEntity *GetGround() const              { return nullptr; }
	virtual const Vector& GetGroundNormal() const       { return vec3_origin; }
	virtual float GetGroundSpeed() const                { return this->m_flGroundSpeed; }
	virtual const Vector& GetGroundMotionVector() const { return this->m_vecGroundMotion; }
	
	virtual void ClimbLadder(const CNavLadder *ladder, const CNavArea *area)   {}
	virtual void DescendLadder(const CNavLadder *ladder, const CNavArea *area) {}
	virtual bool IsUsingLadder() const                                         { return false; }
	virtual bool IsAscendingOrDescendingLadder() const                         { return false; }
	virtual bool IsAbleToAutoCenterOnLadder() const                            { return false; }
	
	virtual void FaceTowards(const Vector& vec)    {}
	virtual void SetDesiredLean(const QAngle& ang) {}
	virtual const QAngle& GetDesiredLean() const   { return vec3_angle; }
	
	virtual bool IsAbleToJumpAcrossGaps() const { return true; }
	virtual bool IsAbleToClimb() const          { return true; }
	
	virtual const Vector& GetFeet() const;
	
	virtual float GetStepHeight() const      { return 0.0f; }
	virtual float GetMaxJumpHeight() const   { return 0.0f; }
	virtual float GetDeathDropHeight() const { return 0.0f; }
	
	virtual float GetRunSpeed() const  { return 0.0f; }
	virtual float GetWalkSpeed() const { return 0.0f; }
	
	virtual float GetMaxAcceleration() const { return 0.0f; }
	virtual float GetMaxDeceleration() const { return 0.0f; }
	
	virtual const Vector& GetVelocity() const     { return vec3_origin; }
	virtual float GetSpeed() const                { return this->m_flSpeed; }
	virtual const Vector& GetMotionVector() const { return this->m_vecMotion; }
	
	virtual bool IsAreaTraversable(const CNavArea *area) const;
	virtual float GetTraversableSlopeLimit() const { return 0.6f; }
	virtual bool IsPotentiallyTraversable(const Vector& from, const Vector& to, TraverseWhenType when, float *pFraction = nullptr) const;
	virtual bool HasPotentialGap(const Vector& vec1, const Vector& vec2, float *pFraction = nullptr) const;
	virtual bool IsGap(const Vector& from, const Vector& to) const;
	virtual bool IsEntityTraversable(CBaseEntity *ent, TraverseWhenType when) const;
	
	virtual bool IsStuck() const { return this->m_bStuck; }
	virtual float GetStuckDuration() const;
	virtual void ClearStuckStatus(char const *msg = "");
	virtual bool IsAttemptingToMove() const;
	
	virtual bool ShouldCollideWith(const CBaseEntity *ent) const { return true; }
	
	virtual void AdjustPosture(const Vector& dst);
	
	virtual void StuckMonitor();
	
	void TraceHull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace);

	virtual ScriptClassDesc_t *GetScriptDesc();
	bool ScriptClimbUpToLedge( const Vector &dst, const Vector &dir, HSCRIPT hEntity );
	void ScriptOnLeaveGround( HSCRIPT hGround );
	void ScriptOnLandOnGround( HSCRIPT hGround );
	HSCRIPT ScriptGetGround();
	bool ScriptIsAreaTraversable( HSCRIPT hArea );
	float ScriptIsPotentiallyTraversable( const Vector& from, const Vector& to, bool bImmediate );
	float ScriptFractionPotentiallyTraversable( const Vector& from, const Vector& to, bool bImmediate );
	float ScriptHasPotentialGap( const Vector& from, const Vector& to );
	float ScriptFractionPotentialGap( const Vector& from, const Vector& to );
	bool ScriptIsEntityTraversable( HSCRIPT hEntity, bool bImmediate );
	
private:
	Vector m_vecMotion;
	Vector m_vecGroundMotion;
	float m_flSpeed;
	float m_flGroundSpeed;
	bool m_bStuck;
	IntervalTimer m_itStuck;
	CountdownTimer m_ctStuckAlert;
	Vector m_vecStuck;
	IntervalTimer m_itApproach;
};
// TODO: move some of these magic constant values into an enum, at the very least...


#endif
