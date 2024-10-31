/* NextBotPlayerLocomotion
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotPlayerLocomotion.h"
#include "NextBotPlayerInput.h"
#include "NextBotInterface.h"
#include "NextBotUtil.h"
#include "NextBotPathFollow.h"
#include "vprof.h"
#include "nav_ladder.h"


static ConVar NextBotPlayerMoveDirect("nb_player_move_direct", "0", FCVAR_NONE);


PlayerLocomotion::PlayerLocomotion(INextBot *nextbot) :
	ILocomotion(nextbot)
{
	this->Reset();
}


void PlayerLocomotion::Reset()
{
	this->m_Player = static_cast<CBasePlayer *>(this->GetBot()->GetEntity());
	
	this->m_bJumping    = false;
	this->m_bClimbing   = false;
	this->m_bGapJumping = false;
	this->m_bHasJumped  = false;
	
	this->m_flDesiredSpeed = 0.0f;
	
	this->m_iLadderState = LS_NONE;
	this->m_NavLadder    = nullptr;
	this->m_NavArea      = nullptr;
	this->m_ctLadderDismount.Invalidate();
	
	this->m_flMinSpeedLimit = 0.0f;
	this->m_flMaxSpeedLimit = 1.0e+7f;
	
	ILocomotion::Reset();
}

void PlayerLocomotion::Update()
{
	if (this->TraverseLadder() == LS_NONE && (this->m_bGapJumping || this->m_bClimbing)) {
		this->SetMinimumSpeedLimit(this->GetRunSpeed());
		
		Vector dir = VectorXY(this->m_vecClimbJump - this->GetFeet()).Normalized();
		
		if (this->m_bHasJumped) {
			Vector look_at = this->GetBot()->GetEntity()->EyePosition() + (100.0f * dir);
			this->GetBot()->GetBodyInterface()->AimHeadTowards(look_at, IBody::PRI_OVERRIDE, 0.25f, nullptr, "Facing impending jump/climb");
			
			if (this->IsOnGround()) {
				this->m_bClimbing   = false;
				this->m_bGapJumping = false;
				
				this->SetMinimumSpeedLimit(0.0f);
			}
		} else {
			if (!this->IsClimbingOrJumping()) this->Jump();
			
			Vector new_velocity = this->GetBot()->GetEntity()->GetAbsVelocity();
			if (this->m_bGapJumping) {
				new_velocity.x = dir.x * this->GetRunSpeed();
				new_velocity.y = dir.y * this->GetRunSpeed();
			}
			
			this->GetBot()->GetEntity()->SetAbsVelocity(new_velocity);
			
			if (!this->IsOnGround()) {
				this->m_bHasJumped = true;
			}
		}
		
		this->Approach(this->m_vecClimbJump);
	}
	
	ILocomotion::Update();
}


void PlayerLocomotion::Approach(const Vector& dst, float f1)
{
	VPROF_BUDGET("PlayerLocomotion::Approach", "NextBot");
	
	// WTF: why is f1 disregarded?
	ILocomotion::Approach(dst, 1.0f);
	this->AdjustPosture(dst);
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		NDebugOverlay::Line(this->GetFeet(), dst, NB_RGB_YELLOW, true, 0.1f);
	}
	
	INextBot *nextbot = this->GetBot();
	INextBotPlayerInput *input = nullptr;
	if (nextbot == nullptr || (input = dynamic_cast<INextBotPlayerInput *>(nextbot)) == nullptr) {
		DevMsg("PlayerLocomotion::Approach: No INextBotPlayerInput\n");
		return;
	}
	
	Vector eye_xy    = VectorXY(EyeVectorsFwd(this->m_Player)).Normalized();
	Vector eye_xy_90 = Vector(eye_xy.y, -eye_xy.x, 0.0f);
	
	Vector feet_to_dst = VectorXY(dst - this->GetFeet()).Normalized();
	
	float fwd  = feet_to_dst.Dot(eye_xy);
	float side = feet_to_dst.Dot(eye_xy_90);
	
	if (this->m_Player->IsOnLadder() && this->IsUsingLadder() && (this->m_iLadderState == LS_ASCEND || this->m_iLadderState == LS_DESCEND)) {
		input->PressForwardButton();
		
		if (this->m_NavLadder != nullptr) {
			Vector ladder_dst;
			CalcClosestPointOnLine(this->GetFeet(), this->m_NavLadder->m_bottom, this->m_NavLadder->m_top, ladder_dst);
			
			Vector ladder_dir     = (this->m_NavLadder->m_top - this->m_NavLadder->m_bottom).Normalized();
			Vector ladder_to_feet = (this->GetFeet() - ladder_dst);
			Vector ladder_norm    = this->m_NavLadder->GetNormal();
			
			Vector cross;
			CrossProduct(ladder_norm, ladder_dir, cross);
			
			float crossdot = DotProduct(cross, ladder_to_feet);
			
			if (crossdot > 5.0f + (0.25f * this->GetBot()->GetBodyInterface()->GetHullWidth())) {
				if (crossdot / ladder_to_feet.Length() == 0.0f) {
					input->PressRightButton();
				} else {
					input->PressLeftButton();
				}
			}
		}
	} else {
		if (NextBotPlayerMoveDirect.GetBool() && feet_to_dst.Length() > 0.25f) {
			input->SetButtonScale(fwd, side);
		}
		
		if (fwd > 0.25f) {
			input->PressForwardButton();
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(), this->m_Player->GetAbsOrigin() + (50.0f * eye_xy), 15.0f, NB_RGBA_GREEN, true, 0.1f);
			}
		} else if (fwd < -0.25f) {
			input->PressBackwardButton();
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(), this->m_Player->GetAbsOrigin() - (50.0f * eye_xy), 15.0f, NB_RGBA_RED, true, 0.1f);
			}
		}
		
		if (side <= -0.25f) {
			input->PressLeftButton();
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(), this->m_Player->GetAbsOrigin() - (50.0f * eye_xy_90), 15.0f, NB_RGBA_MAGENTA, true, 0.1f);
			}
		} else if (side >= 0.25f) {
			input->PressRightButton();
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(), this->m_Player->GetAbsOrigin() + (50.0f * eye_xy_90), 15.0f, NB_RGBA_CYAN, true, 0.1f);
			}
		}
	}
	
	if (!this->IsRunning()) {
		input->PressWalkButton();
	}
}

void PlayerLocomotion::DriveTo(const Vector& dst)
{
	ILocomotion::DriveTo(dst);
	this->Approach(dst);
}


bool PlayerLocomotion::ClimbUpToLedge(const Vector& dst, const Vector& dir, const CBaseEntity *ent)
{
	if (!this->IsClimbPossible(this->GetBot(), ent)) return false;
	
	this->Jump();
	
	this->m_bClimbing  = true;
	this->m_bHasJumped = false;
	
	this->m_vecClimbJump = dst;
	
	return true;
}

void PlayerLocomotion::JumpAcrossGap(const Vector& dst, const Vector& dir)
{
	this->Jump();
	
	this->GetBot()->GetBodyInterface()->AimHeadTowards(dst, IBody::PRI_OVERRIDE, 1.0f, nullptr, "Looking forward while jumping a gap");
	
	this->m_bGapJumping = true;
	this->m_bHasJumped  = false;
	
	this->m_vecClimbJump = dst;
}

void PlayerLocomotion::Jump()
{
	this->m_bJumping = true;
	
	this->m_ctJump.Start(0.5f);
	
	auto input = dynamic_cast<INextBotPlayerInput *>(this->GetBot());
	if (input != nullptr) input->PressJumpButton();
}

bool PlayerLocomotion::IsClimbingOrJumping() const
{
	if (!this->m_bJumping) return false;
	
	if (this->m_ctJump.IsElapsed() && this->IsOnGround()) {
		this->m_bJumping = false;
		return false;
	}
	
	return true;
}


const Vector& PlayerLocomotion::GetGroundNormal() const
{
	static Vector up(0.0f, 0.0f, 1.0f);
	return up;
}


void PlayerLocomotion::ClimbLadder(const CNavLadder *ladder, const CNavArea *area)
{
	this->m_iLadderState = LS_ASCEND_APPROACH;
	
	this->m_NavLadder = ladder;
	this->m_NavArea   = area;
}

void PlayerLocomotion::DescendLadder(const CNavLadder *ladder, const CNavArea *area)
{
	this->m_iLadderState = LS_DESCEND_APPROACH;
	
	this->m_NavLadder = ladder;
	this->m_NavArea   = area;
}

bool PlayerLocomotion::IsAscendingOrDescendingLadder() const
{
	switch (this->m_iLadderState) {
	
	case LS_ASCEND:
	case LS_DESCEND:
	case LS_ASCEND_DISMOUNT:
	case LS_DESCEND_DISMOUNT:
		return true;
		
	case LS_ASCEND_APPROACH:
	case LS_DESCEND_APPROACH:
	case LS_NONE:
	default:
		return false;
	}
}

bool PlayerLocomotion::IsAbleToAutoCenterOnLadder() const
{
	if (!this->IsUsingLadder()) return false;
	
	return (this->m_iLadderState == LS_ASCEND || this->m_iLadderState == LS_DESCEND);
}


void PlayerLocomotion::FaceTowards(const Vector& vec)
{
	Vector look_at(vec.x, vec.y, this->GetBot()->GetEntity()->EyePosition().z);
	this->GetBot()->GetBodyInterface()->AimHeadTowards(Vector(vec.x, vec.y, this->GetBot()->GetEntity()->EyePosition().z), IBody::PRI_BORING, 0.1f, nullptr, "Body facing");
}

const QAngle& PlayerLocomotion::GetDesiredLean() const
{
	// BUG: wtf is this? the variable name is literally "junk" in the Valve source code
	
	static QAngle junk;
	return junk;
}


void PlayerLocomotion::AdjustPosture(const Vector& dst)
{
	IBody *body = this->GetBot()->GetBodyInterface();
	
	if (body->IsActualPosture(IBody::POSTURE_STAND) || body->IsActualPosture(IBody::POSTURE_CROUCH)) {
		ILocomotion::AdjustPosture(dst);
	}
}


bool PlayerLocomotion::IsClimbPossible(INextBot *nextbot, const CBaseEntity *ent)
{
	const PathFollower *path = this->GetBot()->GetCurrentPath();
	if (path != nullptr && !path->IsDiscontinuityAhead(this->GetBot(), Path::SEG_CLIMBJUMP, 75.0f)) {
		IPhysicsObject *pPhysicsObject;
		if (!(ent != nullptr && !const_cast<CBaseEntity *>(ent)->IsWorld() && (pPhysicsObject = ent->VPhysicsGetObject()) != nullptr && pPhysicsObject->IsMoveable())) {
			// BUG: why use GetLocomotionInterface when you could just use this?
			return this->GetBot()->GetLocomotionInterface()->IsStuck();
		}
	}
	
	return true;
}


bool PlayerLocomotion::TraverseLadder()
{
	switch (this->m_iLadderState) {
		
	case LS_ASCEND_APPROACH:
		this->m_iLadderState = this->ApproachAscendingLadder();
		return true;
		
	case LS_DESCEND_APPROACH:
		this->m_iLadderState = this->ApproachDescendingLadder();
		return true;
		
	case LS_ASCEND:
		this->m_iLadderState = this->AscendLadder();
		return true;
		
	case LS_DESCEND:
		this->m_iLadderState = this->DescendLadder();
		return true;
		
	case LS_ASCEND_DISMOUNT:
		this->m_iLadderState = this->DismountLadderTop();
		return true;
		
	case LS_DESCEND_DISMOUNT:
		this->m_iLadderState = this->DismountLadderBottom();
		return true;
		
	default:
		this->m_NavLadder = nullptr;
		
		if (this->GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER) {
			this->GetBot()->GetEntity()->SetMoveType(MOVETYPE_WALK);
		}
		
		return false;
	}
}

PlayerLocomotion::LadderState PlayerLocomotion::ApproachAscendingLadder()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (this->GetFeet().z >= (this->m_NavLadder->m_top.z - this->GetStepHeight())) {
		this->m_ctLadderDismount.Start(2.0f);
		return LS_ASCEND_DISMOUNT;
	}
	
	if (this->GetFeet().z <= (this->m_NavLadder->m_bottom.z - this->GetMaxJumpHeight())) {
		return LS_NONE;
	}
	
	this->FaceTowards(this->m_NavLadder->m_bottom);
	this->Approach(this->m_NavLadder->m_bottom, 1.0e+7f);
	
	if (this->GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER) {
		return LS_ASCEND;
	} else {
		if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
			NDebugOverlay::EntityText(this->GetBot()->GetEntity()->entindex(), 0, "Approach ascending ladder", 0.1f, NB_RGBA_WHITE);
		}
		
		return LS_ASCEND_APPROACH;
	}
}

PlayerLocomotion::LadderState PlayerLocomotion::ApproachDescendingLadder()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (this->GetFeet().z <= (this->m_NavLadder->m_bottom.z + this->GetMaxJumpHeight())) {
		this->m_ctLadderDismount.Start(2.0f);
		return LS_DESCEND_DISMOUNT;
	}
	
	Vector2D l_norm = this->m_NavLadder->GetNormal().AsVector2D();
	
	Vector2D dir = (this->m_NavLadder->m_top.AsVector2D() - this->GetFeet().AsVector2D()) + (l_norm * (0.25f * this->GetBot()->GetBodyInterface()->GetHullWidth()));
	float dist = Vector2DNormalize(dir);
	
	Vector goal;
	if (dist < 10.0f) {
		goal = this->GetFeet() + (100.0f * this->GetMotionVector());
	} else {
		if (DotProduct2D(dir, l_norm) < 0.0f) {
			goal = this->m_NavLadder->m_top - (100.0f * this->m_NavLadder->GetNormal());
		} else {
			goal = this->m_NavLadder->m_top + (100.0f * this->m_NavLadder->GetNormal());
		}
	}
	
	this->FaceTowards(goal);
	this->Approach(goal, 1.0e+7f);
	
	if (this->GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER) {
		return LS_DESCEND;
	} else {
		if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
			NDebugOverlay::EntityText(this->GetBot()->GetEntity()->entindex(), 0, "Approach descending ladder", 0.1f, NB_RGBA_WHITE);
		}
		
		return LS_DESCEND_APPROACH;
	}
}

PlayerLocomotion::LadderState PlayerLocomotion::AscendLadder()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (this->GetBot()->GetEntity()->GetMoveType() != MOVETYPE_LADDER) {
		this->m_NavLadder = nullptr;
		return LS_NONE;
	}
	
	if (this->GetFeet().z >= this->m_NavLadder->m_top.z) {
		this->m_ctLadderDismount.Start(2.0f);
		return LS_ASCEND_DISMOUNT;
	}
	
	Vector l_norm = VecPlusZ(this->m_NavLadder->GetNormal(), -2.0f);
	
	Vector goal = (-100.0f * l_norm) + this->GetFeet();
	
	this->GetBot()->GetBodyInterface()->AimHeadTowards(goal, IBody::PRI_OVERRIDE, 0.1f, nullptr, "Ladder");
	this->Approach(goal, 1.0e+7f);
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		NDebugOverlay::EntityText(this->GetBot()->GetEntity()->entindex(), 0, "Ascend", 0.1f, NB_RGBA_WHITE);
	}
	
	return LS_ASCEND;
}

PlayerLocomotion::LadderState PlayerLocomotion::DescendLadder()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (this->GetBot()->GetEntity()->GetMoveType() != MOVETYPE_LADDER) {
		this->m_NavLadder = nullptr;
		return LS_NONE;
	}
	
	if (this->GetFeet().z >= (this->m_NavLadder->m_bottom.z + this->GetStepHeight())) {
		this->m_ctLadderDismount.Start(2.0f);
		return LS_DESCEND_DISMOUNT;
	}
	
	Vector l_norm = VecPlusZ(this->m_NavLadder->GetNormal(), -2.0f);
	
	Vector goal = (100.0f * l_norm) + this->GetFeet();
	
	this->GetBot()->GetBodyInterface()->AimHeadTowards(goal, IBody::PRI_OVERRIDE, 0.1f, nullptr, "Ladder");
	this->Approach(goal, 1.0e+7f);
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		NDebugOverlay::EntityText(this->GetBot()->GetEntity()->entindex(), 0, "Descend", 0.1f, NB_RGBA_WHITE);
	}
	
	return LS_DESCEND;
}

PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderTop()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (this->m_ctLadderDismount.IsElapsed()) {
		this->m_NavLadder = nullptr;
		return LS_NONE;
	}
	
	IBody *body = this->GetBot()->GetBodyInterface();
	
	Vector dir = VectorXY(this->m_NavArea->GetCenter() - this->GetFeet());
	float dist = VectorNormalize(dir);
	
	dir.z = 1.0f;
	
	body->AimHeadTowards(body->GetEyePosition() + (100.0f * dir), IBody::PRI_OVERRIDE, 0.1f, nullptr, "Ladder dismount");
	
	this->Approach(this->GetFeet() + (100.0f * dir), 1.0e+7f);
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		NDebugOverlay::EntityText(this->GetBot()->GetEntity()->entindex(), 0, "Dismount top", 0.1f, NB_RGBA_WHITE);
		NDebugOverlay::HorzArrow(this->GetFeet(), this->m_NavArea->GetCenter(), 5.0f, NB_RGBA_YELLOW, true, 0.1f);
	}
	
	if (this->GetBot()->GetEntity()->GetLastKnownArea() == this->m_NavArea && dist < 10.0f) {
		this->m_NavLadder = nullptr;
		return LS_NONE;
	}
	
	return LS_ASCEND_DISMOUNT;
}

PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderBottom()
{
	if (this->m_NavLadder == nullptr) return LS_NONE;
	
	if (!this->m_ctLadderDismount.IsElapsed()) {
		if (this->GetBot()->GetEntity()->GetMoveType() != MOVETYPE_LADDER) return LS_NONE;
		
		this->GetBot()->GetEntity()->SetMoveType(MOVETYPE_WALK);
	}
	
	this->m_NavLadder = nullptr;
	return LS_NONE;
}
