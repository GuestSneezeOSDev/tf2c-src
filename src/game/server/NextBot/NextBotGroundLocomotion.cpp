#include "cbase.h"
#include "NextBot.h"
#include "NextBotGroundLocomotion.h"
#include "NextBotUtil.h"
#include "nav_mesh.h"
#include "props.h"
#include "BasePropDoor.h"
#include "vprof.h"

extern ConVar NextBotStop;

class GroundLocomotionCollisionTraceFilter : public CTraceFilterSimple
{
public:
	GroundLocomotionCollisionTraceFilter( INextBot *pBot, const IHandleEntity *passentity, int collisionGroup ) : CTraceFilterSimple( passentity, collisionGroup ), m_pBot( pBot )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
			if ( pEntity && m_pBot->IsSelf( pEntity ) )
				return false;

			return m_pBot->GetLocomotionInterface()->ShouldCollideWith( pEntity );
		}

		return false;
	}

private:
	INextBot *m_pBot;
};



NextBotGroundLocomotion::NextBotGroundLocomotion( INextBot *bot )
	: ILocomotion( bot )
{
	m_pNextBot = NULL;
	m_pLadderUsing = NULL;
	m_pLadderDismount = NULL;

	m_qDesiredLean.x = 0;
	m_qDesiredLean.y = 0;
	m_qDesiredLean.z = 0;

	field_12C = false;
}

void NextBotGroundLocomotion::Reset( void )
{
	ILocomotion::Reset();

	m_pNextBot = static_cast<NextBotCombatCharacter *>( GetBot()->GetEntity() );

	field_130.Invalidate();

	m_flDesiredSpeed = 0;
	m_vecVelocity = vec3_origin;
	m_vecAcceleration = vec3_origin;

	m_qDesiredLean.x = 0;
	m_qDesiredLean.y = 0;
	m_qDesiredLean.z = 0;

	m_vecGroundNormal = Vector( 0, 0, 1.0f );
	m_vecApproach = Vector( 1.0f, 0, 0 );

	m_pLadderUsing = NULL;
	m_pLadderDismount = NULL;

	m_bIsClimbingUpToLedge = false;
	m_bIsJumping = false;
	m_bIsJumpingAcrossGap = false;

	m_hGround.Term();

	// Seems unused?
	//field_60 = m_pNextBot->GetPosition();

	m_vecLastValidPos = m_pNextBot->GetPosition();

	// Seems unused?
	//field_E4.Invalidate();

	m_vecAccumulatedPositions = vec3_origin;
	m_flAccumulatedWeights = 0;
}

void NextBotGroundLocomotion::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );
	ILocomotion::Update();

	ApplyAccumulatedApproach();

	IBody *pBody = m_pNextBot->GetBodyInterface();
	if ( !pBody->IsPostureMobile() )
	{
		m_vecAcceleration.x = 0;
		m_vecAcceleration.y = 0;

		m_vecVelocity.x = 0;
		m_vecVelocity.z = 0;
	}

	bool bOnGround = IsOnGround();
	const float flUpdateInterval = GetUpdateInterval();
	const Vector vecOldPos = GetFeet();

	if ( pBody->HasActivityType( 2 ) )
	{
		if ( !IsOnGround() )
			m_vecAcceleration.z -= GetGravity();

		if ( !IsClimbingOrJumping() || m_vecVelocity.z <= 0 )
			UpdateGroundConstraint();
	}

	if ( IsOnGround() )
	{
		if ( IsAttemptingToMove() )
		{
			float flForwardMovement = DotProduct( m_vecApproach, m_vecVelocity );
			float flRightMovement = DotProduct( {m_vecApproach.y, -m_vecApproach.x, 0.0f}, m_vecVelocity );
			float flForwardFriction = GetFrictionForward(), flSidewaysFriction = GetFrictionSideways();

			Vector vecForwardMovement = vec3_origin;
			if ( flForwardMovement < 0.0f )
			{
				vecForwardMovement.x = ( m_vecApproach.x * flForwardMovement ) * -flForwardFriction;
				vecForwardMovement.y = ( m_vecApproach.y * flForwardMovement ) * -flForwardFriction;
			}

			m_vecAcceleration.x += vecForwardMovement.x + ( m_vecApproach.y * flRightMovement ) * -flSidewaysFriction;
			m_vecAcceleration.y += vecForwardMovement.y + ( -m_vecApproach.x * flRightMovement ) * -flSidewaysFriction;
		}
		else
		{
			m_vecAcceleration = vec3_origin;
			m_vecVelocity = vec3_origin;
		}
	}

	Vector vecNewPos = vecOldPos;
	if ( pBody->HasActivityType( 1 ) )
	{
		m_vecAcceleration.x = 0.0f;
		m_vecAcceleration.y = 0.0f;

		m_vecVelocity.x = m_pNextBot->GetAbsVelocity().x;
		m_vecVelocity.y = m_pNextBot->GetAbsVelocity().y;
	}
	else
	{
		m_vecVelocity.x += m_vecAcceleration.x * flUpdateInterval;
		m_vecVelocity.y += m_vecAcceleration.y * flUpdateInterval;

		vecNewPos.x += m_vecVelocity.x * flUpdateInterval;
		vecNewPos.y += m_vecVelocity.y * flUpdateInterval;
	}

	if ( pBody->HasActivityType( 2 ) )
	{
		m_vecAcceleration.z = 0.0f;

		m_vecVelocity.z = m_pNextBot->GetAbsVelocity().z;
	}
	else
	{
		m_vecVelocity.z += m_vecAcceleration.z * flUpdateInterval;

		vecNewPos.z += m_vecVelocity.z * flUpdateInterval;
	}

	UpdatePosition( vecNewPos );

	const Vector vecCurrentPos = GetFeet();
	if ( !pBody->HasActivityType( 1 ) )
	{
		m_vecVelocity.x = vecCurrentPos.x - vecOldPos.x * 1.0 / flUpdateInterval;
		m_vecVelocity.y = vecCurrentPos.y - vecOldPos.y * 1.0 / flUpdateInterval;
	}

	if ( !pBody->HasActivityType( 2 ) )
	{
		m_vecVelocity.z = vecCurrentPos.z - vecOldPos.z * 1.0 / flUpdateInterval;
	}

	const Vector vecVelNormal = m_vecVelocity.Normalized();
	m_flSpeed = vecVelNormal.Length2D();

	if ( IsOnGround() )
	{
		if ( m_flSpeed > GetRunSpeed() )
		{
			m_flSpeed = GetRunSpeed();

			m_vecVelocity.x = m_flSpeed * vecVelNormal.x;
			m_vecVelocity.y = m_flSpeed * vecVelNormal.y;
		}

		if ( !bOnGround )
		{
			m_vecVelocity.z = 0.0f;
			m_vecAcceleration.z = 0.0f;
		}
	}
	else if ( m_vecVelocity.IsLengthLessThan( 1.0f ) )
	{
		m_vecVelocity = GetGroundMotionVector() * GetRunSpeed();
	}

	m_pNextBot->SetAbsVelocity( m_vecVelocity );
	m_vecAcceleration = vec3_origin;

	if ( m_pNextBot->IsDebugging( INextBot::DEBUG_LOCOMOTION ) )
	{
		if ( IsOnGround() )
			NDebugOverlay::Cross3D( GetFeet(), 1.0f, 0, 255, 0, true, 15.0f );
		else
			NDebugOverlay::Cross3D( GetFeet(), 1.0f, 0, 255, 255, true, 15.0f );
	}
}

void NextBotGroundLocomotion::Approach( const Vector &pos, float weight )
{
	ILocomotion::Approach( pos, weight );

	field_12C = true;
	m_flAccumulatedWeights += weight;
	m_vecAccumulatedPositions += ( pos - GetFeet() ) * weight;
}

void NextBotGroundLocomotion::DriveTo( const Vector &pos )
{
	ILocomotion::DriveTo( pos );

	field_12C = true;
	UpdatePosition( pos );
}

bool NextBotGroundLocomotion::ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle )
{
	// NOP
	return false;
}

void NextBotGroundLocomotion::JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward )
{
	if ( !IsOnGround() )
		return;

	if ( m_pNextBot->GetBodyInterface()->StartActivity( ACT_JUMP ) )
	{
		m_bIsJumping = true;
		m_bIsJumpingAcrossGap = true;
		m_bIsClimbingUpToLedge = false;

		Vector vecOrigin = GetFeet();
		Vector vecDirection = vecOrigin - landingGoal;
		float flDistance = vecDirection.NormalizeInPlace();
		float flGravity = GetGravity();
		float flMinDistance = Min( vecOrigin.z - landingGoal.z, flDistance * 0.9f );
		const float flArcCoefficient = 1.4142271f;

		// TODO: Investigate for inline
		float flArc = ( ( flDistance - flMinDistance ) + ( flDistance - flMinDistance ) ) / flGravity;
		flArc = ( flDistance * flArcCoefficient ) / FastSqrt( flArc );

		m_vecVelocity = vecOrigin * vecDirection * flArc;
		m_vecAcceleration = vec3_origin;

		m_pNextBot->OnLeaveGround( m_pNextBot->GetGroundEntity() );
	}
}

void NextBotGroundLocomotion::Jump( void )
{
	if ( !IsOnGround() )
		return;

	if ( m_pNextBot->GetBodyInterface()->StartActivity( ACT_JUMP ) )
	{
		m_bIsJumping = true;
		m_bIsClimbingUpToLedge = false;
		m_vecVelocity.z = FastSqrt( Square( GetGravity() * GetMaxJumpHeight() ) );

		m_pNextBot->OnLeaveGround( m_pNextBot->GetGroundEntity() );
	}
}

bool NextBotGroundLocomotion::IsClimbingOrJumping( void ) const
{
	return m_bIsJumping;
}

bool NextBotGroundLocomotion::IsClimbingUpToLedge( void ) const
{
	return m_bIsClimbingUpToLedge;
}

bool NextBotGroundLocomotion::IsJumpingAcrossGap( void ) const
{
	return m_bIsJumpingAcrossGap;
}

void NextBotGroundLocomotion::Run( void )
{
	m_flDesiredSpeed = GetRunSpeed();
}

void NextBotGroundLocomotion::Walk( void )
{
	m_flDesiredSpeed = GetWalkSpeed();
}

void NextBotGroundLocomotion::Stop( void )
{
	m_flDesiredSpeed = 0;
}

bool NextBotGroundLocomotion::IsRunning( void ) const
{
	return m_flSpeed > ( GetRunSpeed() * 0.9f );
}

void NextBotGroundLocomotion::SetDesiredSpeed( float speed )
{
	m_flDesiredSpeed = speed;
}

float NextBotGroundLocomotion::GetDesiredSpeed( void ) const
{
	return m_flDesiredSpeed;
}

float NextBotGroundLocomotion::GetSpeedLimit( void ) const
{
	if ( !GetBot()->GetBodyInterface()->IsActualPosture( IBody::POSTURE_STAND ) )
		return GetRunSpeed() * 0.75f;

	return 100000000.0f;
}

bool NextBotGroundLocomotion::IsOnGround( void ) const
{
	return m_pNextBot->GetBaseEntity()->GetGroundEntity() != NULL;
}

void NextBotGroundLocomotion::OnLeaveGround( CBaseEntity *ground )
{
	m_pNextBot->SetGroundEntity( NULL );
	m_hGround.Term();

	if( GetBot()->IsDebugging( INextBot::DEBUG_LOCOMOTION ) )
		DevMsg( "%3.2f: NextBotGroundLocomotion::GetBot()->OnLeaveGround\n", gpGlobals->curtime );
}

void NextBotGroundLocomotion::OnLandOnGround( CBaseEntity *ground )
{
	if( GetBot()->IsDebugging( INextBot::DEBUG_LOCOMOTION ) )
		DevMsg( "%3.2f: NextBotGroundLocomotion::GetBot()->OnLandOnGround\n", gpGlobals->curtime );
}

CBaseEntity *NextBotGroundLocomotion::GetGround( void ) const
{
	return m_hGround;
}

const Vector &NextBotGroundLocomotion::GetGroundNormal( void ) const
{
	return m_vecGroundNormal;
}

void NextBotGroundLocomotion::ClimbLadder( const CNavLadder *ladder, const CNavArea *area )
{
	if ( ladder == m_pLadderUsing && m_bAscendingLadder )
		return;

	m_pLadderUsing = ladder;
	m_bAscendingLadder = true;
	m_pLadderDismount = area;

	float flWidth = m_pNextBot->GetBodyInterface()->GetHullWidth();
	Vector vecLadderStart = ladder->GetNormal() * ( flWidth * 0.75 ) + ladder->m_bottom;
	vecLadderStart.z = m_pNextBot->GetPosition().z;

	UpdatePosition( vecLadderStart );
	m_pNextBot->GetBodyInterface()->StartActivity( ACT_CLIMB_UP, 2 );
}

void NextBotGroundLocomotion::DescendLadder( const CNavLadder *ladder, const CNavArea *area )
{
	if ( ladder == m_pLadderUsing && !m_bAscendingLadder )
		return;

	m_pLadderUsing = ladder;
	m_bAscendingLadder = false;
	m_pLadderDismount = area;

	float flWidth = m_pNextBot->GetBodyInterface()->GetHullWidth();
	Vector vecLadderStart = ladder->GetNormal() * ( flWidth * 0.75 ) + ladder->m_top;
	vecLadderStart.z = m_pNextBot->GetPosition().z;

	UpdatePosition( vecLadderStart );
	m_pNextBot->GetBodyInterface()->StartActivity( ACT_CLIMB_DOWN, 2 );

	Vector vecBehindLadder{ladder->GetNormal().x, -ladder->GetNormal().y, -ladder->GetNormal().z};
	float flYaw = UTIL_VecToYaw( vecBehindLadder );

	QAngle lookAngles = m_pNextBot->GetLocalAngles();
	lookAngles.y = flYaw;
	m_pNextBot->SetLocalAngles( lookAngles );
}

bool NextBotGroundLocomotion::IsUsingLadder( void ) const
{
	return m_pLadderUsing != NULL;
}

bool NextBotGroundLocomotion::IsAscendingOrDescendingLadder( void ) const
{
	return IsUsingLadder();
}

void NextBotGroundLocomotion::FaceTowards( const Vector &target )
{
	Vector vecDirection = target - GetFeet();
	QAngle angFacing = m_pNextBot->GetLocalAngles();

	float flYaw = UTIL_VecToYaw( vecDirection );
	float flDiff = AngleDiff( flYaw, angFacing[YAW]);

	float flMaxYaw = GetMaxYawRate();
	float flDeltaYaw = flMaxYaw * GetUpdateInterval();
	if ( -flDeltaYaw > flDiff )
	{
		angFacing[YAW] -= flDeltaYaw;
	}
	else if ( flDiff > flDeltaYaw )
	{
		angFacing[YAW] += flDeltaYaw;
	}
	else
	{
		angFacing[YAW] += flDiff;
	}

	m_pNextBot->SetLocalAngles( angFacing );
}

void NextBotGroundLocomotion::SetDesiredLean( const QAngle &lean )
{
	m_qDesiredLean = lean;
}

const QAngle &NextBotGroundLocomotion::GetDesiredLean( void ) const
{
	return m_qDesiredLean;
}

const Vector &NextBotGroundLocomotion::GetFeet( void ) const
{
	return m_pNextBot->GetPosition();
}

const Vector &NextBotGroundLocomotion::GetAcceleration( void ) const
{
	return m_vecAcceleration;
}

void NextBotGroundLocomotion::SetAcceleration( const Vector &accel )
{
	m_vecAcceleration = accel;
}

const Vector &NextBotGroundLocomotion::GetVelocity( void ) const
{
	return m_vecVelocity;
}

void NextBotGroundLocomotion::SetVelocity( const Vector &vel )
{
	m_vecVelocity = vel;
}

void NextBotGroundLocomotion::OnMoveToSuccess( const Path *path )
{
	m_vecVelocity = vec3_origin;
	m_vecAcceleration = vec3_origin;
}

void NextBotGroundLocomotion::OnMoveToFailure( const Path *path, MoveToFailureType reason )
{
	m_vecVelocity = vec3_origin;
	m_vecAcceleration = vec3_origin;
}

void NextBotGroundLocomotion::ApplyAccumulatedApproach( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	const float flInterval = GetUpdateInterval();
	if ( flInterval <= 0.0f )
		return;
	
	if ( !m_pNextBot->GetBodyInterface()->IsPostureMobile() )
		return;

	Vector vecOrigin = GetFeet();
	if ( m_flAccumulatedWeights > 0.0f )
	{
		Vector vecApproach = m_vecAccumulatedPositions / m_flAccumulatedWeights;
		float flApproachAccumulation = vecApproach.NormalizeInPlace();
		float flDesiredMovement = Min( flApproachAccumulation, flInterval * GetRunSpeed() );

		vecOrigin += flDesiredMovement * vecApproach;

		m_flAccumulatedWeights = 0.0f;
		m_vecAccumulatedPositions = vec3_origin;
	}

	Vector const vecNewOrigin{vecOrigin.x, vecOrigin.y, GetFeet().z};
	m_vecApproach = m_pNextBot->GetPosition() - vecNewOrigin;
	const float flDiff = m_vecApproach.NormalizeInPlace();
	if ( flDiff < 0.001f )
	{
		//field_9C = 0;
		//field_A0 = 0;
		return;
	}

	if ( !DidJustJump() && IsOnGround() )
	{
		m_bIsClimbingUpToLedge = false;

		Vector const &vecGroundNormal = GetGroundNormal();

		Vector const vecLeft{-m_vecApproach.y, m_vecApproach.x, 0.0f};
		m_vecApproach = CrossProduct( vecLeft, vecGroundNormal );
		m_vecApproach.NormalizeInPlace();

		float flVelProduct = DotProduct( m_vecVelocity, m_vecApproach );
		float flMaxSpeed = Min( m_flDesiredSpeed, GetSpeedLimit() );
		if ( flVelProduct < flMaxSpeed )
		{
			float flRatio = ( flVelProduct + FLT_EPSILON ) / flMaxSpeed;
			m_vecAcceleration += ( 1.0f - ( flRatio * flRatio * flRatio * flRatio ) ) * GetMaxAcceleration() * m_vecApproach;
		}
	}
}

bool NextBotGroundLocomotion::DetectCollision( trace_t *pTrace, int &recursionDepth, const Vector &from, const Vector &to, const Vector &vecMins, const Vector &vecMaxs )
{
	IBody *pBody = m_pNextBot->GetBodyInterface();
	if ( field_130.IsElapsed() )
		field_13C.Term();

	GroundLocomotionCollisionTraceFilter filter( m_pNextBot, field_13C, pBody->GetCollisionGroup() );
	TraceHull( from, to, vecMins, vecMaxs, pBody->GetSolidMask(), &filter, pTrace );
	if ( !pTrace->DidHit() )
		return false;

	if ( pTrace->DidHitNonWorldEntity() )
	{
		if ( !pTrace->m_pEnt || pTrace->m_pEnt->MyCombatCharacterPointer() )
			return false;

		if ( IsEntityTraversable( pTrace->m_pEnt, ILocomotion::TRAVERSE_DEFAULT ) && recursionDepth > 0 )
		{
			--recursionDepth;

			CTakeDamageInfo info( m_pNextBot, m_pNextBot, 100.0f, DMG_CRUSH );
			CalculateExplosiveDamageForce( &info, GetMotionVector(), pTrace->endpos );
			pTrace->m_pEnt->TakeDamage( info );

			return DetectCollision( pTrace, recursionDepth, from, to, vecMins, vecMaxs );
		}
	}

	if ( m_pNextBot->ShouldTouch( pTrace->m_pEnt ) )
		m_pNextBot->OnContact( pTrace->m_pEnt, pTrace );

	INextBot *pOther = dynamic_cast<INextBot *>( pTrace->m_pEnt );
	if( pOther && pOther->ShouldTouch( m_pNextBot ) )
	{
		pOther->OnContact( m_pNextBot, pTrace );
	}
	else
	{
		pTrace->m_pEnt->Touch( m_pNextBot );
	}

	return true;
}

bool NextBotGroundLocomotion::DidJustJump( void ) const
{
	if ( IsClimbingOrJumping() )
	{
		return m_pNextBot->GetAbsVelocity().z > 0.0f;
	}

	return false;
}

Vector NextBotGroundLocomotion::ResolveCollision( const Vector &from, const Vector &to, int recursionDepth )
{
	IBody *pBody = m_pNextBot->GetBodyInterface();
	if ( !pBody || recursionDepth < 0 )
		return to;

	if ( field_12C )
	{
		if ( !pBody->IsActualPosture( IBody::POSTURE_STAND ) && !pBody->IsActualPosture( IBody::POSTURE_CROUCH ) )
		{
			field_12C = false;
		}
	}

	Vector vecMins, vecMaxs;
	if ( field_D4 )
	{
		vecMins = pBody->GetHullMins();
	}
	else
	{
		vecMins = pBody->GetHullMins();
		vecMins.z += GetStepHeight();
	}

	bool bCrouchTest = false;
	if ( field_12C )
	{
		vecMaxs.x = pBody->GetHullWidth() / 2;
		vecMaxs.y = pBody->GetHullWidth() / 2;
		vecMaxs.z = pBody->GetStandHullHeight();

		bCrouchTest = true;
	}
	else
	{
		vecMaxs = pBody->GetHullMaxs();
		if ( vecMaxs.z <= vecMins.z )
			vecMins.z = vecMaxs.z - 2.0f;
	}

	trace_t trace;
	Vector vecCollision = to;
	Vector vecResult;
	IBody::PostureType iDesiredPosture = IBody::POSTURE_STAND;
	while ( true )
	{
		if ( !DetectCollision( &trace, recursionDepth, from, vecCollision, vecMins, vecMaxs ) )
		{
			vecResult = vecCollision;
			break;
		}

		if ( !trace.startsolid && vecCollision.DistToSqr( trace.endpos ) < 1.0f )
		{
			vecResult = trace.endpos;;
			break;
		}

		if ( bCrouchTest )
		{
			bCrouchTest = false;
			iDesiredPosture = pBody->GetDesiredPosture();
			
			if ( trace.m_pEnt->MyNextBotPointer() || trace.m_pEnt->IsPlayer() )
			{
				if ( iDesiredPosture == IBody::POSTURE_CROUCH )
				{
					trace_t postureTrace;
					NextBotTraversableTraceFilter filter( m_pNextBot, ILocomotion::TRAVERSE_DEFAULT );
					TraceHull( from, to, vecMins, vecMaxs, pBody->GetSolidMask(), &filter, &postureTrace );
					if ( postureTrace.fraction >= 1.0f && !postureTrace.startsolid )
						iDesiredPosture = IBody::POSTURE_STAND;
				}
			}
			else if ( iDesiredPosture != IBody::POSTURE_CROUCH )
			{
				trace_t postureTrace;
				NextBotTraversableTraceFilter filter( m_pNextBot, ILocomotion::TRAVERSE_DEFAULT );
				TraceHull( from, to, vecMins, {vecMaxs.x, vecMaxs.y, pBody->GetCrouchHullHeight()}, pBody->GetSolidMask(), &filter, &postureTrace );
				if ( postureTrace.fraction >= 1.0f && !postureTrace.startsolid )
					iDesiredPosture = IBody::POSTURE_CROUCH;
			}

			if ( iDesiredPosture == IBody::POSTURE_CROUCH )
			{
				vecMaxs.z = pBody->GetCrouchHullHeight();
				continue;
			}
		}

		if ( trace.startsolid )
			break;

		if ( --recursionDepth <= 0 )
		{
			vecResult = trace.endpos;
			break;
		}

		if ( trace.plane.normal.z < 0.0f )
		{
			trace.plane.normal.z = 0.0f;
			trace.plane.normal.NormalizeInPlace();	
		}
		
		Vector vecToGoal = to - from;
		Vector vecMoveAmt = vecToGoal * ( 1.0f - trace.fraction );
		if ( !pBody->HasActivityType( 2 ) && GetTraversableSlopeLimit() > trace.plane.normal.z && vecToGoal.z > 0.0f )
		{
			vecToGoal.z = 0.0f;
			trace.plane.normal.z = 0.0f;
			trace.plane.normal.NormalizeInPlace();
		}

		float flMovementAmount = DotProduct( trace.plane.normal, vecMoveAmt );

		if ( m_pNextBot->IsDebugging( INextBot::DEBUG_LOCOMOTION ) )
		{
			NDebugOverlay::Line( trace.endpos, trace.endpos + trace.plane.normal * 20.0f, 255, 0, 150, true, 15.0f );
		}

		Vector vecDesiredMove = ( from + vecToGoal ) - ( trace.plane.normal * flMovementAmount );
		if ( vecDesiredMove.DistToSqr( trace.endpos ) < 1.0f )
		{
			vecResult = trace.endpos;
			break;
		}

		vecCollision = vecDesiredMove;
	}

	if ( trace.m_pEnt && !trace.m_pEnt->IsWorld() )
	{
		if ( dynamic_cast<CPhysicsProp *>( trace.m_pEnt ) && !dynamic_cast<CBasePropDoor *>( trace.m_pEnt ) )
		{
			IPhysicsObject *pObject = trace.m_pEnt->VPhysicsGetObject();
			if ( pObject && pObject->IsMoveable() )
			{
				field_13C = trace.m_pEnt;
				field_130.Start( 1.0f );
			}
		}
	}

	vecResult = m_vecLastValidPos;

	if ( !trace.startsolid )
		m_vecLastValidPos = vecResult;

	if ( field_12C )
	{
		field_12C = false;

		if ( !pBody->IsActualPosture( iDesiredPosture ) )
			pBody->SetDesiredPosture( iDesiredPosture );
	}

	return vecResult;
}

bool NextBotGroundLocomotion::TraverseLadder( void )
{
	// NOP
	return false;
}

void NextBotGroundLocomotion::UpdateGroundConstraint( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBotExpensive" );

	if ( DidJustJump() || IsAscendingOrDescendingLadder() )
	{
		field_D4 = false;
	}
	else
	{
		IBody *pBody = m_pNextBot->GetBodyInterface();
		float flWidth = pBody->GetHullWidth();
		float flStepHeight = GetStepHeight();

		trace_t trace;
		NextBotTraceFilterIgnoreActors ignore( m_pNextBot->MyNextBotPointer() );
		TraceHull( m_pNextBot->GetPosition() + Vector( 0, 0, flStepHeight + 0.001f ), m_pNextBot->GetPosition() - Vector( 0, 0, GetStepHeight() + 0.01f ), 
					-Vector(flWidth / 2, flWidth / 2, 0), Vector(flWidth / 2, flWidth / 2, flWidth / 2),
					pBody->GetSolidMask(), &ignore, &trace );

		if ( trace.startsolid )
		{
			if ( m_pNextBot->IsDebugging( INextBot::DEBUG_LOCOMOTION ) && !( gpGlobals->framecount % 60 ) )
			{
				DevMsg( "%3.2f: Inside ground, ( %.0f, %.0f, %.0f )\n", gpGlobals->curtime, m_pNextBot->GetPosition().x, m_pNextBot->GetPosition().y, m_pNextBot->GetPosition().z );
			}
		}
		else if ( trace.fraction >= 1.0f )
		{
			if ( IsOnGround() )
			{
				m_pNextBot->OnLeaveGround( m_pNextBot->GetGroundEntity() );
				if ( !IsClimbingUpToLedge() && !IsJumpingAcrossGap() )
				{
					field_D4 = true;
					m_vecAcceleration.z -= GetGravity();
				}
			}
		}
		else
		{
			m_vecGroundNormal = trace.plane.normal;
			field_D4 = false;

			if ( GetTraversableSlopeLimit() > trace.plane.normal.z )
			{
				// ???
				if ( ( m_vecVelocity.x * trace.plane.normal.x + m_vecVelocity.y * trace.plane.normal.y ) <= 0.0f )
				{
					m_pNextBot->OnContact( trace.m_pEnt, &trace );			
				}

				float flDot = DotProduct( m_vecAcceleration, m_vecGroundNormal );
				m_vecAcceleration -= m_vecGroundNormal * flDot;

				if ( m_pNextBot->IsDebugging( INextBot::DEBUG_LOCOMOTION ) )
				{
					DevMsg( "%3.2f: NextBotGroundLocomotion - Too steep to stand here\n", gpGlobals->curtime );
					NDebugOverlay::Line( GetFeet(), GetFeet() + trace.plane.normal * 20.0f, 255, 150, 0, true, 5.0f );
				}

				m_vecVelocity.z = Min( 0.0f, m_vecVelocity.z );
				m_vecAcceleration.z = Min( 0.0f, m_vecAcceleration.z );
			}
			else
			{
				if ( trace.m_pEnt && !trace.m_pEnt->IsWorld() )
				{
					m_pNextBot->OnContact( trace.m_pEnt, &trace );
				}

				m_pNextBot->SetPosition( trace.endpos );

				if ( IsOnGround() )
				{
					m_pNextBot->SetGroundEntity( trace.m_pEnt );
					m_hGround = trace.m_pEnt;
					m_bIsJumping = m_bIsJumpingAcrossGap = false;

					m_pNextBot->OnLandOnGround( trace.m_pEnt );
				}
			}
		}
	}
}

void NextBotGroundLocomotion::UpdatePosition( const Vector &newPos )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( NextBotStop.GetBool() )
		return;

	if ( m_pNextBot->GetFlags() & FL_FROZEN )
		return;

	if ( m_pNextBot->GetPosition() == newPos )
		return;

	Vector currentPos = m_pNextBot->GetPosition();
	Vector vecCollision = ResolveCollision( currentPos, newPos, 3 );
	if ( m_pNextBot->GetIntentionInterface()->IsPositionAllowed( m_pNextBot, vecCollision ) )
		m_pNextBot->SetPosition( vecCollision );
}
