/* NextBotPlayerLocomotion
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERLOCOMOTION_H
#define NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERLOCOMOTION_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotLocomotionInterface.h"
#include "nav.h"


class PlayerLocomotion : public ILocomotion
{
public:
	PlayerLocomotion(INextBot *nextbot);
	virtual ~PlayerLocomotion() {}
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual void Approach(const Vector& dst, float f1 = 1.0f) override;
	virtual void DriveTo(const Vector& dst) override;
	
	virtual bool ClimbUpToLedge(const Vector& dst, const Vector& dir, const CBaseEntity *ent = nullptr) override;
	virtual void JumpAcrossGap(const Vector& dst, const Vector& dir) override;
	virtual void Jump() override;
	virtual bool IsClimbingOrJumping() const override;
	virtual bool IsClimbingUpToLedge() const override { return this->m_bClimbing; }
	virtual bool IsJumpingAcrossGap() const override  { return this->m_bGapJumping; }
	
	virtual void Run() override             { this->m_flDesiredSpeed = this->GetRunSpeed(); }
	virtual void Walk() override            { this->m_flDesiredSpeed = this->GetWalkSpeed(); }
	virtual void Stop() override            { this->m_flDesiredSpeed = 0.0f; }
	virtual bool IsRunning() const override { return true; }
	
	virtual void SetDesiredSpeed(float speed) override { this->m_flDesiredSpeed = speed; }
	virtual float GetDesiredSpeed() const override     { return Clamp(this->m_flDesiredSpeed, this->m_flMinSpeedLimit, this->m_flMaxSpeedLimit); }
	
	virtual bool IsOnGround() const override        { return (this->m_Player->GetGroundEntity() != nullptr); }
	virtual CBaseEntity *GetGround() const override { return this->m_Player->GetGroundEntity(); }
	virtual const Vector& GetGroundNormal() const override;
	
	virtual void ClimbLadder(const CNavLadder *ladder, const CNavArea *area) override;
	virtual void DescendLadder(const CNavLadder *ladder, const CNavArea *area) override;
	virtual bool IsUsingLadder() const override { return (this->m_iLadderState != LS_NONE); }
	virtual bool IsAscendingOrDescendingLadder() const override;
	virtual bool IsAbleToAutoCenterOnLadder() const override;
	
	virtual void FaceTowards(const Vector& vec) override;
	virtual void SetDesiredLean(const QAngle& ang) override {}
	virtual const QAngle& GetDesiredLean() const override;
	
	virtual const Vector& GetFeet() const override { return this->m_Player->GetAbsOrigin(); }
	
	virtual float GetStepHeight() const override      { return StepHeight; }
	virtual float GetMaxJumpHeight() const override   { return  57.0f; } // TODO: determine source of magic number if possible
	virtual float GetDeathDropHeight() const override { return 200.0f; } // TODO: determine source of magic number if possible
	
	virtual float GetRunSpeed() const override  { return this->m_Player->MaxSpeed(); }
	virtual float GetWalkSpeed() const override { return this->m_Player->MaxSpeed() * 0.5f; }
	
	virtual float GetMaxAcceleration() const override { return 100.0f; } // TODO: determine source of magic number if possible
	virtual float GetMaxDeceleration() const override { return 200.0f; } // TODO: determine source of magic number if possible
	
	virtual const Vector& GetVelocity() const override { return this->m_Player->GetAbsVelocity(); }
	
	virtual void AdjustPosture(const Vector& dst) override;
	
	virtual void SetMinimumSpeedLimit(float limit) { this->m_flMinSpeedLimit = limit; }
	virtual void SetMaximumSpeedLimit(float limit) { this->m_flMaxSpeedLimit = limit; }
	
private:
	enum LadderState
	{
		LS_NONE             = 0,
		LS_ASCEND_APPROACH  = 1,
		LS_DESCEND_APPROACH = 2,
		LS_ASCEND           = 3,
		LS_DESCEND          = 4,
		LS_ASCEND_DISMOUNT  = 5,
		LS_DESCEND_DISMOUNT = 6,
	};
	
	bool IsClimbPossible(INextBot *nextbot, const CBaseEntity *ent);
	
	bool TraverseLadder();
	LadderState ApproachAscendingLadder();
	LadderState ApproachDescendingLadder();
	LadderState AscendLadder();
	LadderState DescendLadder();
	LadderState DismountLadderTop();
	LadderState DismountLadderBottom();
	
	CBasePlayer *m_Player = nullptr;
	mutable bool m_bJumping;
	CountdownTimer m_ctJump;
	bool m_bClimbing;
	bool m_bGapJumping;
	Vector m_vecClimbJump;
	bool m_bHasJumped;
	float m_flDesiredSpeed;
	float m_flMinSpeedLimit;
	float m_flMaxSpeedLimit;
	LadderState m_iLadderState;
	const CNavLadder *m_NavLadder;
	const CNavArea *m_NavArea;
	CountdownTimer m_ctLadderDismount;
};


#endif
