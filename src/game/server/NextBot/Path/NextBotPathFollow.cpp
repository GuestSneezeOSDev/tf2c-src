/* NextBotPathFollow
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotPathFollow.h"
#include "NextBotManager.h"
#include "NextBotUtil.h"
#include "BasePropDoor.h"


static ConVar NextBotSpeedLookAheadRange("nb_speed_look_ahead_range", "150", FCVAR_CHEAT);
static ConVar NextBotGoalLookAheadRange ("nb_goal_look_ahead_range",   "50", FCVAR_CHEAT);
static ConVar NextBotLadderAlignRange   ("nb_ladder_align_range",      "50", FCVAR_CHEAT);
static ConVar NextBotAllowAvoiding      ("nb_allow_avoiding",           "1", FCVAR_CHEAT);
static ConVar NextBotAllowClimbing      ("nb_allow_climbing",           "1", FCVAR_CHEAT);
static ConVar NextBotAllowGapJumping    ("nb_allow_gap_jumping",        "1", FCVAR_CHEAT);
static ConVar NextBotDebugClimbing      ("nb_debug_climbing",           "0", FCVAR_CHEAT);


PathFollower::~PathFollower()
{
	TheNextBots().NotifyPathDestruction(this);
}


void PathFollower::Invalidate()
{
	Path::Invalidate();
	
	this->m_CurrentGoal = nullptr;
	this->m_ctUnknown1.Invalidate();
	this->m_ctUnknown2.Invalidate();
	this->m_hHindrance = nullptr;
}


void PathFollower::Draw(const Segment *seg) const
{
	if (this->m_CurrentGoal == nullptr) return;
	
	if (this->m_bAvoidA) {
		// TODO: probably just fold this into vec3_angle or something
		QAngle angles(0.0f, 0.0f, 0.0f);
		
		if (this->m_bAvoidB) {
			NDebugOverlay::SweptBox(this->m_vecAvoid1Start, this->m_vecAvoid1End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, angles, NB_RGBA_GREEN, 0.1f);
		} else {
			NDebugOverlay::SweptBox(this->m_vecAvoid1Start, this->m_vecAvoid1End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, angles, NB_RGBA_RED,   0.1f);
		}
		
		if (this->m_bAvoidC) {
			NDebugOverlay::SweptBox(this->m_vecAvoid2Start, this->m_vecAvoid2End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, angles, NB_RGBA_GREEN, 0.1f);
		} else {
			NDebugOverlay::SweptBox(this->m_vecAvoid2Start, this->m_vecAvoid2End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, angles, NB_RGBA_RED,   0.1f);
		}
		
		this->m_bAvoidA = false;
	}
	
	if (this->m_CurrentGoal != nullptr) {
		NDebugOverlay::Sphere(this->m_CurrentGoal->pos, 5.0f, NB_RGB_YELLOW, true, 0.1f);
		
		Vector vecStart = this->m_CurrentGoal->m_portalCenter;
		Vector vecEnd   = this->m_CurrentGoal->m_portalCenter;
		if (this->m_CurrentGoal->how == GO_NORTH || this->m_CurrentGoal->how == GO_SOUTH) {
			vecStart.x -= this->m_CurrentGoal->m_portalHalfWidth;
			vecEnd.x   += this->m_CurrentGoal->m_portalHalfWidth;
		} else {
			vecStart.y -= this->m_CurrentGoal->m_portalHalfWidth;
			vecEnd.y   += this->m_CurrentGoal->m_portalHalfWidth;
		}
		NDebugOverlay::Line(vecStart, vecEnd, NB_RGB_MAGENTA, true, 0.1f);
		
		const Segment *seg_prev = this->PriorSegment(this->m_CurrentGoal);
		if (seg_prev != nullptr) {
			NDebugOverlay::Line(seg_prev->pos, this->m_CurrentGoal->pos, NB_RGB_YELLOW, true, 0.1f);
		}
	}
	
	Path::Draw(nullptr);
}


void PathFollower::Update(INextBot *nextbot)
{
	VPROF_BUDGET("PathFollower::Update", "NextBotSpiky");
	
	Assert(this->IsInitialized());
	
	nextbot->SetCurrentPath(this);
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	if (!this->IsValid())                return;
	if (this->m_CurrentGoal == nullptr)  return;
	if (!this->m_ctUnknown2.IsElapsed()) return;
	
	if (this->LadderUpdate(nextbot)) return;
	
	this->AdjustSpeed(nextbot);
	
	if (this->CheckProgress(nextbot) == PROGRESS_OK) return;
	
	Vector dir = (this->m_CurrentGoal->pos - loco->GetFeet());
	if (this->m_CurrentGoal->type == SEG_CLIMBJUMP) {
		const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
		if (seg_next != nullptr) {
			dir = (seg_next->pos - loco->GetFeet());
		}
	}
	dir = VectorXY(dir);
	
	float len = dir.NormalizeInPlace();
	Vector norm(-dir.y, dir.x, 0.0f);
	
	if (norm.IsZero()) {
		loco->GetBot()->OnMoveToFailure(this, INextBotEventResponder::FAIL_STUCK);
		
		if (this->GetAge() != 0.0f) {
			this->Invalidate();
		}
		
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			DevMsg("PathFollower: OnMoveToFailure( FAIL_STUCK ) because forward and left are ZERO\n");
		}
		
		return;
	}
	
	const Vector& ground = loco->GetGroundNormal();
	
	dir  =  CrossProduct(norm, ground);
	norm = -CrossProduct(dir,  ground);
	
	if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
		NDebugOverlay::Line(loco->GetFeet(), loco->GetFeet() + (25.0f * dir),    NB_RGB_RED,   true, 0.1f);
		NDebugOverlay::Line(loco->GetFeet(), loco->GetFeet() + (25.0f * ground), NB_RGB_GREEN, true, 0.1f);
		NDebugOverlay::Line(loco->GetFeet(), loco->GetFeet() + (25.0f * norm),   NB_RGB_BLUE,  true, 0.1f);
	}
	
	if (!this->Climbing(nextbot, this->m_CurrentGoal, dir, norm, len)) {
		if (!this->IsValid()) return;
		
		this->JumpOverGaps(nextbot, this->m_CurrentGoal, dir, norm, len);
	}
	
	if (!this->IsValid()) return;
	
	bool is_stairs = false;
	CNavArea *nb_lkarea = nextbot->GetEntity()->GetLastKnownArea();
	if (nb_lkarea != nullptr) {
		is_stairs = nb_lkarea->HasAttributes(NAV_MESH_STAIRS);
	}
	
	if (this->m_CurrentGoal->ladder == nullptr && !loco->IsClimbingOrJumping() && !is_stairs &&
		this->m_CurrentGoal->pos.z > loco->GetMaxJumpHeight() + loco->GetFeet().z) {
		Vector2D feet_xy = loco->GetFeet().AsVector2D();
		Vector2D goal_xy = this->m_CurrentGoal->pos.AsVector2D();
		
		if (loco->IsStuck() || goal_xy.DistToSqr(feet_xy) < Square(25.0f)) {
			const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
			
			if (loco->IsStuck() || seg_next == nullptr || seg_next->pos.z - loco->GetFeet().z > loco->GetMaxJumpHeight() ||
				!loco->IsPotentiallyTraversable(loco->GetFeet(), seg_next->pos, ILocomotion::TRAVERSE_BREAKABLE)) {
				loco->GetBot()->OnMoveToFailure(this, INextBotEventResponder::FAIL_FELL_OFF);
				
				if (this->GetAge() != 0.0f) {
					this->Invalidate();
				}
				
				if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
					DevMsg("PathFollower: OnMoveToFailure( FAIL_FELL_OFF )\n");
				}
				
				loco->ClearStuckStatus("Fell off path");
				
				return;
			}
		}
	}
	
	Vector goal_pos = this->m_CurrentGoal->pos;
	
	dir  = VectorXY(goal_pos - loco->GetFeet());
	len  = dir.NormalizeInPlace();
	norm = Vector(-dir.y, dir.x, 0.0f);
	
	if (len > 50.0f || this->m_CurrentGoal->type != SEG_CLIMBJUMP) {
		goal_pos = this->Avoid(nextbot, goal_pos, dir, norm);
	}
	
	if (loco->IsOnGround()) {
		loco->FaceTowards(goal_pos);
	}
	
	loco->Approach(goal_pos);
	
	if (this->m_CurrentGoal != nullptr) {
		switch (this->m_CurrentGoal->type) {
		case SEG_CLIMBJUMP:
		case SEG_GAPJUMP:
			nextbot->GetBodyInterface()->SetDesiredPosture(IBody::POSTURE_STAND);
			break;
		}
	}
	
	if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
		const Segment *goal = this->GetCurrentGoal();
		if (goal != nullptr) {
			goal = this->PriorSegment(goal);
		}
		this->Draw(goal);
		
		NDebugOverlay::Cross3D(goal_pos, 5.0f, NB_RGB_LTBLUE_96, true, 0.1f);
		NDebugOverlay::Line(nextbot->GetEntity()->WorldSpaceCenter(), goal_pos, NB_RGB_YELLOW, true, 0.1f);
	}
}


bool PathFollower::IsDiscontinuityAhead(INextBot *nextbot, SegmentType type, float max_dist) const
{
	if (this->m_CurrentGoal == nullptr) return false;
	
	const Segment *prev = this->PriorSegment(this->m_CurrentGoal);
	if (prev != nullptr && prev->type == type) return true;
	
	float dist = (this->m_CurrentGoal->pos - nextbot->GetLocomotionInterface()->GetFeet()).Length();
	for (const Segment *seg = this->m_CurrentGoal; seg != nullptr; seg = this->NextSegment(seg)) {
		if (dist > max_dist)   return false;
		if (seg->type == type) return true;
		
		dist += seg->length;
	}
	
	return false;
}


void PathFollower::Initialize(INextBot *nextbot)
{
	Assert(!this->m_bInitialized);
	
	this->SetMinLookAheadDistance(nextbot->GetDesiredPathLookAheadRange());
	this->Invalidate();
	
	this->m_bInitialized = true;
}


void PathFollower::AdjustSpeed(INextBot *nextbot)
{
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	float speed;
	if ((this->m_CurrentGoal != nullptr && this->m_CurrentGoal->type == SEG_GAPJUMP) || !loco->IsOnGround()) {
		speed = loco->GetRunSpeed();
	} else {
		this->MoveCursorToClosestPosition(nextbot->GetPosition(), SEEK_FROM_START, 0.0f);
		float curv_abs = abs(this->GetCursorData().curvature);
		
		speed = loco->GetRunSpeed() - (curv_abs * (loco->GetRunSpeed() - loco->GetWalkSpeed()));
	}
	
	loco->SetDesiredSpeed(speed);
}

Vector PathFollower::Avoid(INextBot *nextbot, const Vector& goal, const Vector& dir, const Vector& norm)
{
	VPROF_BUDGET("PathFollower::Avoid", "NextBotExpensive");
	
	if (!NextBotAllowAvoiding.GetBool()) return goal;
	if (!this->m_ctUnknown1.IsElapsed()) return goal;
	
	this->m_ctUnknown1.Start(0.5f);
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	if (loco->IsClimbingOrJumping() || !loco->IsOnGround()) return goal;
	
	this->m_hHindrance = this->FindBlocker(nextbot);
	if (this->m_hHindrance != nullptr) {
		this->m_ctUnknown2.Start(0.5f * RandomFloat(1.0f, 2.0f));
		return loco->GetFeet();
	}
	
	CNavArea *area = nextbot->GetEntity()->GetLastKnownArea();
	if (area != nullptr && area->HasAttributes(NAV_MESH_PRECISE)) return goal;
	
	this->m_bAvoidA = true;
	
	// TODO: move this down if possible
	NextBotTraceFilterOnlyActors filter1(nextbot);
	
	IBody *body = nextbot->GetBodyInterface();
	
	unsigned int solid_mask = body->GetSolidMask();
	float hull_width        = body->GetHullWidth();
	
	// TODO: rename this
	// TODO: magic numbers
	float run_factor = (loco->IsRunning() ? 50.0f : 30.0f) * nextbot->GetEntity()->GetModelScale();
	
	this->m_vecAvoidMins.x = -0.25f * hull_width;
	this->m_vecAvoidMins.y = -0.25f * hull_width;
	this->m_vecAvoidMins.z = loco->GetStepHeight() + 0.1f;
	
	this->m_vecAvoidMaxs.x = 0.25f * hull_width;
	this->m_vecAvoidMaxs.y = 0.25f * hull_width;
	this->m_vecAvoidMaxs.z = body->GetCrouchHullHeight();
	
	float norm_mult = (0.25f * hull_width) + 2.0f;
	this->m_vecAvoid1Start = loco->GetFeet() + (norm_mult * norm);
	this->m_vecAvoid1End   = this->m_vecAvoid1Start + (run_factor * dir);
	
	this->m_bAvoidB = true;
	
	NextBotTraversableTraceFilter filter2(nextbot, ILocomotion::TRAVERSE_BREAKABLE);
	
	trace_t tr;
	loco->TraceHull(this->m_vecAvoid1Start, this->m_vecAvoid1End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, solid_mask, &filter2, &tr);
	
	CBasePropDoor *door = nullptr;
	
	// NOTE: this block is highly suspect; it was a big pain to RE
	float frac1_reverse = 0.0f;
	if (tr.fraction < 1.0f || tr.startsolid) {
		if (tr.startsolid) {
			tr.fraction = 0.0f;
		}
		
		this->m_bAvoidB = false;
		
		frac1_reverse = Clamp(1.0f - tr.fraction, 0.0f, 1.0f);
		
		if (tr.DidHitNonWorldEntity()) {
			door = dynamic_cast<CBasePropDoor *>(tr.m_pEnt);
		}
	}
	
	this->m_vecAvoid2Start = loco->GetFeet() - (norm_mult * norm);
	this->m_vecAvoid2End   = this->m_vecAvoid2Start + (run_factor * dir);
	
	this->m_bAvoidC = true;
	
	loco->TraceHull(this->m_vecAvoid2Start, this->m_vecAvoid2End, this->m_vecAvoidMins, this->m_vecAvoidMaxs, solid_mask, &filter2, &tr);
	
	// NOTE: same uncertainty with this as with the block above
	float frac2_reverse = 0.0f;
	if (tr.fraction < 1.0f || tr.startsolid) {
		if (tr.startsolid) {
			tr.fraction = 0.0f;
		}
		
		this->m_bAvoidC = false;
		
		frac2_reverse = Clamp(1.0f - tr.fraction, 0.0f, 1.0f);
		
		if (door == nullptr && tr.DidHitNonWorldEntity()) {
			door = dynamic_cast<CBasePropDoor *>(tr.m_pEnt);
		}
	}
	
	if (door != nullptr && !this->m_bAvoidB && !this->m_bAvoidC) {
		Vector target = door->GetAbsOrigin() - (100.0f * AngleVecRight(door->GetAbsAngles()));
		
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			NDebugOverlay::Axis(door->GetAbsOrigin(), door->GetAbsAngles(), 20.0f, true, 10.0f);
			NDebugOverlay::Line(door->GetAbsOrigin(), target, NB_RGB_YELLOW, true, 10.0f);
		}
		
		this->m_ctUnknown1.Invalidate();
		
		return Vector(target.x, target.y, goal.z);
	}
	
	if (this->m_bAvoidB && this->m_bAvoidC) {
		return goal;
	}
	
	float norm_mult2;
	
	if (this->m_bAvoidB) {
		norm_mult2 = -frac2_reverse;
	} else if (this->m_bAvoidC) {
		norm_mult2 = frac1_reverse;
	} else {
		if (abs(frac2_reverse - frac1_reverse) < 0.01f) {
			return goal;
		}
		
		if (frac2_reverse <= frac1_reverse) {
			norm_mult2 = frac1_reverse;
		} else {
			norm_mult2 = -frac2_reverse;
		}
	}
	
	Vector diff = (0.5f * dir) - (norm_mult2 * norm);
	return loco->GetFeet() + (100.0f * diff.Normalized());
}

PathFollower::ProgressType PathFollower::CheckProgress(INextBot *nextbot)
{
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	const Segment *new_goal = nullptr;
	if (this->m_flMinLookAheadDistance > 0.0f) {
		const Segment *goal = this->m_CurrentGoal;
		const Vector& feet = loco->GetFeet();
		
		if (goal != nullptr && goal->type == SEG_GROUND) {
			do {
				if (!loco->IsOnGround())                                                 break;
				if (Square(this->m_flMinLookAheadDistance) <= feet.DistToSqr(goal->pos)) break;
				
				goal = this->NextSegment(goal);
				
				if (goal == nullptr)                              break;
				if (goal->type != SEG_GROUND)                     break;
				if (goal->pos.z > loco->GetStepHeight() + feet.z) break;
			} while (loco->IsPotentiallyTraversable(feet, goal->pos, ILocomotion::TRAVERSE_BREAKABLE) &&
				!loco->HasPotentialGap(feet, goal->pos) && goal->type == SEG_GROUND);
		}
		
		if (this->m_CurrentGoal != goal) {
			new_goal = goal;
		}
	}
	
	if (!this->IsAtGoal(nextbot)) {
		return PROGRESS_BAD;
	}
	
	if (new_goal != nullptr || (new_goal = this->NextSegment(this->m_CurrentGoal)) != nullptr) {
		this->m_CurrentGoal = new_goal;
		
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			if (!loco->IsPotentiallyTraversable(loco->GetFeet(), new_goal->pos, ILocomotion::TRAVERSE_BREAKABLE)) {
				Warning( "PathFollower: %s path to my goal is blocked by something\n", nextbot->GetDebugIdentifier() );
				NDebugOverlay::Line( nextbot->GetEntity()->WorldSpaceCenter(), this->m_CurrentGoal->pos, NB_RGB_RED, true, 3.0f );
				NDebugOverlay::Sphere(this->m_CurrentGoal->pos, 5.0f, NB_RGB_RED, true, 3.0f);
			}
		}
		
		return PROGRESS_BAD;
	}
	
	if (!loco->IsOnGround()) {
		return PROGRESS_BAD;
	}
	
	loco->GetBot()->OnMoveToSuccess(this);
	
	if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
		DevMsg("PathFollower::OnMoveToSuccess\n");
	}
	
	if (this->GetAge() != 0.0f) {
		this->Invalidate();
	}
	
	return PROGRESS_OK;
}

bool PathFollower::Climbing(INextBot *nextbot, const Segment *seg, const Vector& dir, const Vector& norm, float len)
{
	VPROF_BUDGET("PathFollower::Climbing", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
//	IBody *body       = nextbot->GetBodyInterface();
	
//	CNavArea *nb_lkarea = nextbot->GetEntity()->GetLastKnownArea();
	
	if (!loco->IsAbleToClimb())          return false;
	if (!NextBotAllowClimbing.GetBool()) return false;
	
	if (loco->IsClimbingOrJumping())           return false;
	if (loco->IsAscendingOrDescendingLadder()) return false;
	if (!loco->IsOnGround())                   return false;
	if (this->m_CurrentGoal == nullptr)        return false;
	
	if (TheNavMesh->IsAuthoritative()) {
		if (this->m_CurrentGoal->type != SEG_CLIMBJUMP) return false;
		
		const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
		if (seg_next       == nullptr) return false;
		if (seg_next->area == nullptr) return false;
		
		Vector dst;
		seg_next->area->GetClosestPointOnArea(loco->GetFeet(), &dst);
		
		Vector new_dir = VectorXY(dst - loco->GetFeet()).Normalized();
		return loco->ClimbUpToLedge(dst, new_dir, nullptr);
	}
	
	// NOTE: everything else in this function only matters if the nav mesh is
	// not authoritative, which isn't applicable for CTFNavMesh
	
	DevMsg("STUB: PathFollower::Climbing (non-authoritative nav mesh case)\n");
	
	Assert(false);
	return false;
}

CBaseEntity *PathFollower::FindBlocker(INextBot *nextbot)
{
	IIntention *intent = nextbot->GetIntentionInterface();
	
	// NOTE: the code here (in all versions: L4D, L4D2, Garry's Mod, TF2) actually
	// calls IIntention::IsHindrance with (CBaseEntity *)-1; this is extremely baffling
	// and doesn't seem to really be used for much of anything, and there's no
	// particularly valid reason to have 0xffffffff be a special pointer value...
	// so, we'll replace that nonsense with a nullptr argument in our version; much cleaner
	if (intent->IsHindrance(nextbot, nullptr) != TRS_TRUE) return nullptr;
	// FULL LIST OF CALLEES THAT ACTUALLY HANDLE THE -1 VALUE IN SOME WAY:
	// - TF2:  CTFBotSpyAttack      (returns TRS_NONE for -1 and for nullptr alike)
	// - L4D:  SurvivorBehavior     (returns TRS_TRUE for -1; doesn't even bother to check for nullptr)
	// - L4D2: SurvivorBehavior     (returns TRS_TRUE for -1; doesn't even bother to check for nullptr)
	// - L4D2: L4D1SurvivorBehavior (returns TRS_TRUE for -1; doesn't even bother to check for nullptr)
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	IBody *body       = nextbot->GetBodyInterface();
	
	NextBotTraceFilterOnlyActors filter(nextbot);
	
	float hull_width         = body->GetHullWidth();
	float step_height        = loco->GetStepHeight();
	float crouch_hull_height = body->GetCrouchHullHeight();
	
	Vector prev = loco->GetFeet();
	
	this->MoveCursorToClosestPosition(loco->GetFeet(), SEEK_FROM_START, 0.0f);
	
	const Segment *seg = this->GetCursorData().seg;
	if (seg == nullptr) return nullptr;
	
	Vector mins(-0.25f * hull_width, -0.25f * hull_width, step_height);
	Vector maxs( 0.25f * hull_width,  0.25f * hull_width, crouch_hull_height);
	
	// TODO: probably rearrange this into a for-loop based on seg
	float dist = 0.0f;
	while (dist < 750.0f) {
		Vector dir = seg->pos - prev;
		float len = VectorNormalize(dir);
		
		float ray_len = Max(len, 2.0f * body->GetHullWidth());
		
		Ray_t ray;
		ray.Init(prev, prev + (ray_len * dir), mins, maxs);
		
		trace_t tr;
		enginetrace->TraceRay(ray, body->GetSolidMask(), &filter, &tr);
		
		if (tr.DidHitNonWorldEntity()) {
			const Vector& my_feet    = nextbot->GetLocomotionInterface()->GetFeet();
			const Vector& ent_origin = tr.m_pEnt->GetAbsOrigin();
			
			// TODO: better names for vec1 and vecc2
			Vector vec1 = seg->pos - prev;
			Vector vec2 = ent_origin - my_feet;
			
			if (DotProduct(vec1, vec2) != 0.0f && intent->IsHindrance(nextbot, tr.m_pEnt) == TRS_TRUE) {
				if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
					NDebugOverlay::Circle(nextbot->GetLocomotionInterface()->GetFeet(), QAngle(-90.0f, 0.0f, 0.0f), 10.0f, NB_RGBA_RED, true, 1.0f);
					NDebugOverlay::HorzArrow(nextbot->GetLocomotionInterface()->GetFeet(), tr.m_pEnt->GetAbsOrigin(), 1.0f, NB_RGBA_RED, true, 1.0f);
				}
				
				return tr.m_pEnt;
			}
		}
		
		prev = seg->pos;
		dist += seg->length;
		
		seg = this->NextSegment(seg);
		if (seg == nullptr) break;
	}
	
	return nullptr;
}

bool PathFollower::GapJumping(INextBot *nextbot, const Segment *seg)
{
	VPROF_BUDGET("PathFollower::GapJumping", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	IBody *body       = nextbot->GetBodyInterface();
	
	Vector from = loco->GetFeet() + (seg->forward * (0.5f * body->GetHullWidth()));
	Vector to   = seg->forward;
	
	if (!loco->IsGap(from, to)) return false;
	
	const Segment *next = this->NextSegment(seg);
	if (next == nullptr) return false;
	
	loco->JumpAcrossGap(next->pos, next->forward);
	this->m_CurrentGoal = next;
	
	if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
		NDebugOverlay::Cross3D(this->m_CurrentGoal->pos, 5.0f, NB_RGB_CYAN, true, 5.0f);
		DevMsg("%3.2f: GAP JUMP\n", gpGlobals->curtime);
	}
	
	return true;
}

bool PathFollower::IsAtGoal(INextBot *nextbot) const
{
	VPROF_BUDGET("PathFollower::IsAtGoal", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	IBody *body       = nextbot->GetBodyInterface();
	
	const Segment *seg_prev = this->PriorSegment(this->m_CurrentGoal);
	if (seg_prev == nullptr) return true;
	
	if (this->m_CurrentGoal->type == SEG_FALL) {
		const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
		if (seg_next == nullptr) return true;
		
		return (loco->GetStepHeight() > (loco->GetFeet().z - seg_next->pos.z));
	}
	
	if (this->m_CurrentGoal->type == SEG_CLIMBJUMP) {
		const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
		if (seg_next == nullptr) return true;
		
		return (loco->GetFeet().z > (loco->GetStepHeight() + this->m_CurrentGoal->pos.z));
	}
	
	const Segment *seg_next = this->NextSegment(this->m_CurrentGoal);
	
	Vector dir = VectorXY(this->m_CurrentGoal->pos - loco->GetFeet());
	
	bool is_at_goal = dir.IsLengthLessThan(this->m_flGoalTolerance);
	
	if (seg_next == nullptr)                                                                                                 return is_at_goal;
	if ((this->m_CurrentGoal->forward + (seg_prev->ladder != nullptr ? seg_prev->forward : vec3_origin)).Dot(dir) < 0.0001f) return is_at_goal;
	if (body->GetStandHullHeight() > abs(this->m_CurrentGoal->pos.z - loco->GetFeet().z))                                    return is_at_goal;
	if (loco->GetStepHeight() > this->m_CurrentGoal->pos.z - loco->GetFeet().z)                                              return is_at_goal;
	if (loco->IsPotentiallyTraversable(loco->GetFeet(), seg_next->pos, ILocomotion::TRAVERSE_BREAKABLE))                     return is_at_goal;
	if (!loco->HasPotentialGap(loco->GetFeet(), seg_next->pos))                                                              return is_at_goal;
	
	return true;
}

bool PathFollower::JumpOverGaps(INextBot *nextbot, const Segment *seg, const Vector& dir, const Vector& norm, float len)
{
	VPROF_BUDGET("PathFollower::JumpOverGaps", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	IBody *body       = nextbot->GetBodyInterface();
	
	if (!loco->IsAbleToJumpAcrossGaps() || !NextBotAllowGapJumping.GetBool())                        return false;
	if (loco->IsClimbingOrJumping() || loco->IsAscendingOrDescendingLadder() || !loco->IsOnGround()) return false;
	if (!body->IsActualPosture(IBody::POSTURE_STAND) || this->m_CurrentGoal == nullptr)              return false;
	
	NextBotTraversableTraceFilter filter(nextbot, ILocomotion::TRAVERSE_DEFAULT);
	
	float hull_width = body->GetHullWidth();
	
	const Segment *prev = this->PriorSegment(this->m_CurrentGoal);
	if (prev == nullptr) return false;
	
	// TODO: integrate the first iteration of this loop
	if (prev->type != SEG_GAPJUMP) {
		prev = this->m_CurrentGoal;
		
		if (prev == nullptr)         return false;
		if (len > 2.0f * hull_width) return false;
		
		float accum = len;
		while (prev->type != SEG_GAPJUMP) {
			accum += prev->length;
			prev = this->NextSegment(prev);
			
			if (prev == nullptr)           return false;
			if (accum > 2.0f * hull_width) return false;
		}
	}
	
	return this->GapJumping(nextbot, prev);
}

bool PathFollower::LadderUpdate(INextBot *nextbot)
{
	VPROF_BUDGET("PathFollower::LadderUpdate", "NextBot");
	
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	IBody *body       = nextbot->GetBodyInterface();
	
	if (loco->IsUsingLadder()) {
		return true;
	}
	
	const Segment *goal = this->m_CurrentGoal;
	
	if (goal->ladder == nullptr) {
		if (nextbot->GetEntity()->GetMoveType() == MOVETYPE_LADDER) {
			const Segment *prev = this->PriorSegment(this->m_CurrentGoal);
			if (prev == nullptr) return false;
			
			goal = prev;
			
			// TODO: rework this loop condition
			while (true) {
				if (goal->ladder != nullptr && goal->how == GO_LADDER_DOWN) {
					if (goal->ladder->m_length > loco->GetMaxJumpHeight()) {
						if (loco->GetMaxJumpHeight() > abs(goal->pos.z - loco->GetFeet().z)) {
							this->m_CurrentGoal = goal;
							break;
						}
					}
				}
				
				goal = this->NextSegment(goal);
				if (goal == nullptr || (prev != goal && DistSqrXY(loco->GetFeet(), goal->pos) > Square(50.0f))) {
					goal = this->m_CurrentGoal;
					break;
				}
			}
		}
		
		if (goal->ladder == nullptr) {
			return false;
		}
	}
	
	const CNavLadder *ladder = goal->ladder;
	
	if (goal->how == GO_LADDER_UP) {
		if (!loco->IsUsingLadder()) {
			if (loco->GetFeet().z > (ladder->m_top.z - loco->GetStepHeight())) {
				this->m_CurrentGoal = this->NextSegment(this->m_CurrentGoal);
				return false;
			}
		}
		
		Vector look = VecPlusZ(ladder->m_top - (50.0f * ladder->GetNormal()), body->GetCrouchHullHeight());
		body->AimHeadTowards(look, IBody::PRI_CRITICAL, 2.0f, nullptr, "Mounting upward ladder");
		
		Vector2D delta = (ladder->m_bottom - loco->GetFeet()).AsVector2D();
		float delta_len = delta.Length();
		
		if (delta_len != 0.0f) {
			delta /= delta_len;
		} else {
			delta = Vector2D(0.0f, 0.0f);
		}
		
		float ladder_align_range = NextBotLadderAlignRange.GetFloat();
		if (ladder_align_range <= delta_len) {
			return false;
		}
		
		float dot_product = DotProduct2D(delta, ladder->GetNormal().AsVector2D());
		if (dot_product < -0.9f) {
			loco->Approach(ladder->m_bottom);
			
			if (delta_len < 25.0f) {
				loco->ClimbLadder(ladder, goal->area);
			}
		} else {
			if (dot_product < 0.0f) {
				ladder_align_range = 25.0f + (ladder_align_range - 25.0f) * (dot_product + 1.0f);
			}
			
			Vector target = VectorXY(ladder->m_bottom) - Vector2DTo3D(delta * ladder_align_range);
			if (DotProduct2D(delta, Vector2D(-ladder->GetNormal().y, ladder->GetNormal().x)) < 0.0f) {
				target.x -= (10.0f * delta.y);
				target.y += (10.0f * delta.x);
			} else {
				target.x += (10.0f * delta.y);
				target.y -= (10.0f * delta.x);
			}
			
			loco->Approach(target);
		}
		
		return true;
	}
	
	if (loco->GetFeet().z >= (ladder->m_bottom.z + loco->GetStepHeight())) {
		Vector target = ladder->m_top + ((0.5f * body->GetHullWidth()) * ladder->GetNormal());
		if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
			NDebugOverlay::Sphere(target, 5.0f, NB_RGB_MAGENTA, true, 0.1f);
		}
		
		Vector look = VecPlusZ(ladder->m_bottom + (50.0f * ladder->GetNormal()), body->GetCrouchHullHeight());
		body->AimHeadTowards(look, IBody::PRI_CRITICAL, 1.0f, nullptr, "Mounting downward ladder");
		
		if (DistSqrXY(loco->GetFeet(), target) >= Square(25.0f) && nextbot->GetEntity()->GetMoveType() != MOVETYPE_LADDER) {
			return false;
		}
	}
	
	this->m_CurrentGoal = this->NextSegment(this->m_CurrentGoal);
	return true;
}
