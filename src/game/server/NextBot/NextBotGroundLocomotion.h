#ifndef NEXT_BOT_GROUND_LOCOMOTION_H
#define NEXT_BOT_GROUND_LOCOMOTION_H

#include "NextBotLocomotionInterface.h"


class NextBotCombatCharacter;
class CNavLadder;
class CNavArea;

class NextBotGroundLocomotion : public ILocomotion
{
public:
	NextBotGroundLocomotion( INextBot *bot );
	virtual ~NextBotGroundLocomotion() {}

	virtual void Reset( void ) OVERRIDE;
	virtual void Update( void ) OVERRIDE;

	virtual void Approach( const Vector &pos, float weight = 1.0f ) OVERRIDE;
	virtual void DriveTo( const Vector &pos ) OVERRIDE;

	virtual bool ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle ) OVERRIDE;
	virtual void JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward ) OVERRIDE;
	virtual void Jump( void ) OVERRIDE;
	virtual bool IsClimbingOrJumping( void ) const OVERRIDE;
	virtual bool IsClimbingUpToLedge( void ) const OVERRIDE;
	virtual bool IsJumpingAcrossGap( void ) const OVERRIDE;

	virtual void Run( void ) OVERRIDE;
	virtual void Walk( void ) OVERRIDE;
	virtual void Stop( void ) OVERRIDE;
	virtual bool IsRunning( void ) const OVERRIDE;
	virtual void SetDesiredSpeed( float speed ) OVERRIDE;
	virtual float GetDesiredSpeed( void ) const OVERRIDE;

	virtual float GetSpeedLimit( void ) const OVERRIDE;

	virtual bool IsOnGround( void ) const OVERRIDE;
	virtual void OnLeaveGround( CBaseEntity *ground ) OVERRIDE;
	virtual void OnLandOnGround( CBaseEntity *ground ) OVERRIDE;
	virtual CBaseEntity *GetGround( void ) const OVERRIDE;
	virtual const Vector &GetGroundNormal( void ) const OVERRIDE;

	virtual void ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal ) OVERRIDE;
	virtual void DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal ) OVERRIDE;
	virtual bool IsUsingLadder( void ) const OVERRIDE;
	virtual bool IsAscendingOrDescendingLadder( void ) const OVERRIDE;

	virtual void FaceTowards( const Vector &target ) OVERRIDE;

	virtual void SetDesiredLean( const QAngle &lean ) OVERRIDE;
	virtual const QAngle &GetDesiredLean( void ) const OVERRIDE;

	virtual const Vector &GetFeet( void ) const OVERRIDE;

	virtual float GetStepHeight( void ) const OVERRIDE { return 18.0; }
	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 180.0f; }
	virtual float GetDeathDropHeight( void ) const OVERRIDE { return 200.0f; }

	virtual float GetRunSpeed( void ) const OVERRIDE { return 150.0f; }
	virtual float GetWalkSpeed( void ) const OVERRIDE { return 75.0f; }

	virtual float GetMaxAcceleration( void ) const OVERRIDE { return 500.0f; }
	virtual float GetMaxDeceleration( void ) const OVERRIDE { return 500.0f; }

	virtual const Vector &GetAcceleration( void ) const;
	virtual void SetAcceleration( const Vector &accel );

	virtual const Vector &GetVelocity( void ) const OVERRIDE;
	virtual void SetVelocity( const Vector &vel );

	virtual void OnMoveToSuccess( const Path *path ) OVERRIDE;
	virtual void OnMoveToFailure( const Path *path, MoveToFailureType reason ) OVERRIDE;

private:
	void ApplyAccumulatedApproach( void );
	bool DetectCollision( trace_t *pTrace, int &recursionDepth, const Vector &from, const Vector &to, const Vector &vecMins, const Vector &vecMaxs );
	bool DidJustJump( void ) const;
	Vector ResolveCollision( const Vector &from, const Vector &to, int a3 );
	bool TraverseLadder( void );
	void UpdateGroundConstraint( void );
	void UpdatePosition( const Vector &newPos );

	virtual float GetGravity( void ) const { return 1000.0f; }
	virtual float GetFrictionForward( void ) const { return 0.0f; }
	virtual float GetFrictionSideways( void ) const { return 3.0f; }
	virtual float GetMaxYawRate( void ) const { return 250.0f; }


private:
	NextBotCombatCharacter *m_pNextBot;
	Vector m_vecLastValidPos;
	Vector m_vecVelocity;
	Vector m_vecAcceleration;
	float m_flDesiredSpeed;
	float m_flSpeed;
	QAngle m_qDesiredLean;
	bool m_bIsJumping;
	bool m_bIsJumpingAcrossGap;
	EHANDLE m_hGround;
	Vector m_vecGroundNormal;
	bool m_bIsClimbingUpToLedge;
	bool field_D4; // TODO: Name
	const CNavLadder *m_pLadderUsing;
	const CNavArea *m_pLadderDismount;
	bool m_bAscendingLadder;
	Vector m_vecApproach;
	Vector m_vecAccumulatedPositions;
	float m_flAccumulatedWeights;
	bool field_12C; // TODO: Name
	CountdownTimer field_130; // TODO: Name
	EHANDLE field_13C; // TODO: Name
};

#endif