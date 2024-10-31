/* NextBotLocomotionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotInterface.h"
#include "NextBotUtil.h"
#include "vprof.h"
#include "nav_area.h"
#include "BasePropDoor.h"
#include "vscript/ivscript.h"

BEGIN_ENT_SCRIPTDESC( ILocomotion, INextBotComponent, "Next bot locomotion" )
	DEFINE_SCRIPTFUNC( Approach, "The primary locomotive method. Sets the goal destination for the bot" )
	DEFINE_SCRIPTFUNC( DriveTo, "Move the bot to the precise given position immediately, updating internal state" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptClimbUpToLedge, "ClimbUpToLedge", "Initiate a jump to an adjacent high ledge, returns false if climb can't start")
	DEFINE_SCRIPTFUNC( JumpAcrossGap, "Initiate a jump across an empty volume of space to far side" )
	DEFINE_SCRIPTFUNC( Jump, "Initiate a simple undirected jump in the air" )
	DEFINE_SCRIPTFUNC( IsClimbingOrJumping, "Is jumping in any form" )
	DEFINE_SCRIPTFUNC( IsClimbingUpToLedge, "Is climbing up to a high ledge" )
	DEFINE_SCRIPTFUNC( IsJumpingAcrossGap, "Is jumping across a gap to the far side" )
	DEFINE_SCRIPTFUNC( IsAscendingOrDescendingLadder, "Is using a ladder" )
	DEFINE_SCRIPTFUNC( IsScrambling, "Is in the middle of a complex action (climbing a ladder, climbing a ledge, jumping, etc) that shouldn't be interrupted" )
	DEFINE_SCRIPTFUNC( Run, "Set desired movement speed to running" )
	DEFINE_SCRIPTFUNC( Walk, "Set desired movement speed to walking" )
	DEFINE_SCRIPTFUNC( Stop, "Set desired movement speed to stopped" )
	DEFINE_SCRIPTFUNC( IsRunning, "Is running?" )
	DEFINE_SCRIPTFUNC( SetDesiredSpeed, "Set desired speed for locomotor movement" )
	DEFINE_SCRIPTFUNC( GetDesiredSpeed, "Get desired speed for locomotor movement" )
	DEFINE_SCRIPTFUNC( SetSpeedLimit, "Set maximum speed bot can reach, regardless of desired speed" )
	DEFINE_SCRIPTFUNC( GetSpeedLimit, "Get maximum speed bot can reach, regardless of desired speed" )
	DEFINE_SCRIPTFUNC( IsOnGround, "Returns true if standing on something" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptOnLeaveGround, "OnLeaveGround", "Manually run the OnLeaveGround callback. Typically invoked when bot leaves ground for any reason" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptOnLandOnGround, "OnLandOnGround", "Manually run the OnLandOnGround callback. Typically invoked when bot lands on the ground after being in the air" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetGround, "GetGround", "Returns the current ground entity or NULL if not on the ground" )
	DEFINE_SCRIPTFUNC( GetGroundNormal, "Surface normal of the ground we are in contact with" )
	DEFINE_SCRIPTFUNC( GetGroundSpeed, "Return current world space speed in XY plane" )
	DEFINE_SCRIPTFUNC( GetGroundMotionVector, "Returns unit vector in XY plane describing our direction of motion - even if we are currently not moving" )
	DEFINE_SCRIPTFUNC( FaceTowards, "Rotate body to face towards target" )
	DEFINE_SCRIPTFUNC( IsAbleToJumpAcrossGaps, "Returns true if this bot can jump across gaps in its path" )
	DEFINE_SCRIPTFUNC( IsAbleToClimb, "Returns true if this bot can climb arbitrary geometry it encounters" )
	DEFINE_SCRIPTFUNC( GetFeet, "Returns position of feet - the driving point where the bot contacts the ground" )
	DEFINE_SCRIPTFUNC( GetStepHeight, "If delta Z is greater than this, we have to jump to get up" )
	DEFINE_SCRIPTFUNC( GetMaxJumpHeight, "Returns maximum height of a jump" )
	DEFINE_SCRIPTFUNC( GetDeathDropHeight, "Distance at which we will die if we fall" )
	DEFINE_SCRIPTFUNC( GetRunSpeed, "Get maximum running speed" )
	DEFINE_SCRIPTFUNC( GetWalkSpeed, "Get maximum walking speed" )
	DEFINE_SCRIPTFUNC( GetMaxAcceleration, "Returns maximum acceleration of locomotor" )
	DEFINE_SCRIPTFUNC( GetMaxDeceleration, "Returns the maximum deceleration of locomotor" )
	DEFINE_SCRIPTFUNC( GetVelocity, "Returns current world spaee velocity" )
	DEFINE_SCRIPTFUNC( GetSpeed, "Returns current world space speed (magnitude of velocity)" )
	DEFINE_SCRIPTFUNC( GetMotionVector, "Returns unit vector describing our direction of motion - even if we are currently not moving" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsAreaTraversable, "IsAreaTraversable", "Returns true if given area can be used for navigation" )
	DEFINE_SCRIPTFUNC( GetTraversableSlopeLimit, "Returns Z component of unit normal of steepest traversable slope" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsPotentiallyTraversable, "IsPotentiallyTraversable", "Returns true if this locomotor could potentially move along the line given" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptFractionPotentiallyTraversable, "FractionPotentiallyTraversable", "If the locomotor could not move along the line given, returns the fraction of the walkable ray" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptHasPotentialGap, "HasPotentialGap", "Returns true if there is a possible gap that will need to be jumped over" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptFractionPotentialGap, "FractionPotentialGap", "If the locomotor cannot jump over the gap, returns the fraction of the jumpable ray" )
	DEFINE_SCRIPTFUNC( IsGap, "Return true if there is a gap here when moving in the given direction" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsEntityTraversable, "IsEntityTraversable", "Returns true if the entity handle is traversable" )
	DEFINE_SCRIPTFUNC( IsStuck, "Returns true if bot is stuck. If the locomot cannot make progress, it becomes stuck and can only leave this stuck state bu succesfully moving and becoming un-stuck." )
	DEFINE_SCRIPTFUNC( GetStuckDuration, "Returns how long we've been stuck" )
	DEFINE_SCRIPTFUNC( ClearStuckStatus, "Reset stuck status to un-stuck" )
	DEFINE_SCRIPTFUNC( IsAttemptingToMove, "Returns true if we have tried to Approach() or DriveTo() very recently" )
END_SCRIPTDESC()

void ILocomotion::Reset()
{
	INextBotComponent::Reset();
	
	this->m_vecMotion.Zero();
	this->m_vecGroundMotion.Zero();
	
	this->m_flSpeed       = 0.0f;
	this->m_flGroundSpeed = 0.0f;
	
	this->m_bStuck = false;
	this->m_itStuck.Invalidate();
	this->m_itApproach.Invalidate();
	
	this->m_vecStuck = vec3_origin;
}

void ILocomotion::Update()
{
	this->StuckMonitor();
	
	const Vector& vel = this->GetVelocity();
	
	this->m_flSpeed       = vel.Length();
	this->m_flGroundSpeed = vel.Length2D();
	
	if (this->m_flSpeed > 10.0f) {
		this->m_vecMotion = vel / this->m_flSpeed;
	}
	
	if (this->m_flGroundSpeed > 10.0f) {
		this->m_vecGroundMotion.x = vel.x / this->m_flGroundSpeed;
		this->m_vecGroundMotion.y = vel.y / this->m_flGroundSpeed;
		this->m_vecGroundMotion.z = 0.0f;
	}
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		NDebugOverlay::HorzArrow(this->GetFeet(), this->GetFeet() + (25.0f * this->m_vecGroundMotion), 3.0f, NB_RGBA_LIME,   true, 0.1f);
		NDebugOverlay::HorzArrow(this->GetFeet(), this->GetFeet() + (25.0f * this->m_vecMotion),       5.0f, NB_RGBA_YELLOW, true, 0.1f);
	}
}


bool ILocomotion::IsScrambling() const
{
	if (!this->IsOnGround() || this->IsClimbingOrJumping()) {
		return true;
	} else {
		return this->IsAscendingOrDescendingLadder();
	}
}


const Vector& ILocomotion::GetFeet() const
{
	return this->GetBot()->GetEntity()->GetAbsOrigin();
}


bool ILocomotion::IsAreaTraversable(const CNavArea *area) const
{
	return !area->IsBlocked(this->GetBot()->GetEntity()->GetTeamNumber());
}


bool ILocomotion::IsPotentiallyTraversable(const Vector& from, const Vector& to, TraverseWhenType when, float *pFraction) const
{
	VPROF_BUDGET("Locomotion::IsPotentiallyTraversable", "NextBotExpensive");
	
	Vector delta = (to - from);
	
	if (delta.z > (this->GetMaxJumpHeight() + 0.1f) && (delta.Normalized().z > this->GetTraversableSlopeLimit())) {
		if (pFraction != nullptr) *pFraction = 0.0f;
		return false;
	}
	
	NextBotTraversableTraceFilter filter(this->GetBot(), when);
	
	float step_height        = this->GetStepHeight();
	float hull_width         = this->GetBot()->GetBodyInterface()->GetHullWidth();
	float crouch_hull_height = this->GetBot()->GetBodyInterface()->GetCrouchHullHeight();
	unsigned int solid_mask  = this->GetBot()->GetBodyInterface()->GetSolidMask();
	
	Vector vecMins(-0.25f * hull_width, -0.25f * hull_width, step_height);
	Vector vecMaxs( 0.25f * hull_width,  0.25f * hull_width, crouch_hull_height);
	
	Ray_t ray;
	ray.Init(from, to, vecMins, vecMaxs);
	
	trace_t tr;
	enginetrace->TraceRay(ray, solid_mask, &filter, &tr);
	
	if (pFraction != nullptr) *pFraction = tr.fraction;
	
	return (tr.fraction >= 1.0f && !tr.startsolid);
}

bool ILocomotion::HasPotentialGap(const Vector& from, const Vector& to, float *pFraction) const
{
	VPROF_BUDGET("Locomotion::HasPotentialGap", "NextBot");
	
	float fraction;
	this->IsPotentiallyTraversable(from, to, TRAVERSE_DEFAULT, &fraction);
	
	float half_hull_width = 0.5f * this->GetBot()->GetBodyInterface()->GetHullWidth();
	
	Vector vecDelta = (to - from) * fraction;
	float max = half_hull_width * VectorNormalize(vecDelta);
	
	Vector vecFrom = from;
	Vector vecTo   = vecDelta;
	
	for (float dist = 0.0f; dist < max; dist += half_hull_width) {
		if (this->IsGap(vecFrom, vecTo)) {
			if (pFraction != nullptr) *pFraction = (dist - half_hull_width) / max;
			return true;
		}
		
		vecFrom += half_hull_width * vecDelta;
	}
	
	if (pFraction != nullptr) *pFraction = 1.0f;
	return false;
}

bool ILocomotion::IsGap(const Vector& from, const Vector& to) const
{
	VPROF_BUDGET("Locomotion::IsGap", "NextBotSpiky");
	
	unsigned int mask;
	IBody *body = this->GetBot()->GetBodyInterface();
	if (body != nullptr) {
		mask = body->GetSolidMask();
	} else {
		mask = MASK_PLAYERSOLID;
	}
	
	NextBotTraceFilterIgnoreActors filter(this->GetBot());
	
	Vector vecMins(0.0f, 0.0f, 0.0f);
	Vector vecMaxs(1.0f, 1.0f, 1.0f);
	
	Ray_t ray;
	ray.Init(from, VecPlusZ(from, this->GetStepHeight()), vecMins, vecMaxs);
	
	trace_t tr;
	enginetrace->TraceRay(ray, mask, &filter, &tr);
	
	return (tr.fraction >= 1.0f && !tr.startsolid);
}

bool ILocomotion::IsEntityTraversable(CBaseEntity *ent, TraverseWhenType when) const
{
	if (ent->IsWorld()) return false;
	
	if (ent->ClassMatches("prop_door*") || ent->ClassMatches("func_door*")) {
		auto door = dynamic_cast<CBasePropDoor *>(ent);
		if (door != nullptr) {
			return door->IsDoorOpen();
		}

		auto funcdoor = dynamic_cast<CBaseDoor *>( ent );
		if ( funcdoor != nullptr ) {
			return ( funcdoor->m_toggle_state == TS_GOING_UP || funcdoor->m_toggle_state == TS_AT_TOP );
		}
		
		return true;
	}
	
	if (ent->ClassMatches("func_brush")) {
		auto brush = static_cast<CFuncBrush *>(ent);
		
		switch (brush->m_iSolidity) {
		case CFuncBrush::BRUSHSOLID_TOGGLE: return true;
		case CFuncBrush::BRUSHSOLID_NEVER:  return true;
		case CFuncBrush::BRUSHSOLID_ALWAYS: return false;
		}
	}
	
	if (when != TRAVERSE_DEFAULT) {
		return this->GetBot()->IsAbleToBreak(ent);
	} else {
		return false;
	}
}


float ILocomotion::GetStuckDuration() const
{
	if (this->IsStuck()) {
		return this->m_itStuck.GetElapsedTime();
	} else {
		return 0.0f;
	}
}

void ILocomotion::ClearStuckStatus(char const *msg)
{
	if (this->IsStuck()) {
		this->m_bStuck = false;
		this->OnUnStuck();
	}
	
	this->m_vecStuck = this->GetFeet();
	this->m_itStuck.Start();
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
		DevMsg("%3.2f: ClearStuckStatus: %s %s\n", gpGlobals->curtime, this->GetBot()->GetDebugIdentifier(), msg);
	}
}

bool ILocomotion::IsAttemptingToMove() const
{
	if (this->m_itApproach.HasStarted()) {
		return this->m_itApproach.IsLessThan(0.25f);
	} else {
		return false;
	}
}


void ILocomotion::AdjustPosture(const Vector& dst)
{
	IBody *body = this->GetBot()->GetBodyInterface();
	
	if (!body->IsActualPosture(IBody::POSTURE_STAND) && !body->IsActualPosture(IBody::POSTURE_CROUCH)) {
		return;
	}
	
	// TODO: refactor and tighten up this function from here on ----------------
	
	Vector mins = body->GetHullMins();
	mins.z += this->GetStepHeight();
	
	float half_hull_width = 0.5f * body->GetHullWidth();
	Vector maxs(half_hull_width, half_hull_width, body->GetStandHullHeight());
	
	const Vector& feet          = this->GetFeet();
	const Vector& ground_normal = this->GetGroundNormal();
	
	Vector dir1 = (dst - feet);
	float dist = VectorNormalize(dir1);
	
	// TODO: figure out what overall mathematical operation is going on here
	
	float x =  dir1.x * ground_normal.z;
	float y = -dir1.y * ground_normal.z;
	
	// BUG: might have signs reversed here (i.e. z_a and z_b might be swapped)... check this!
	float z_a = -dir1.y * ground_normal.y;
	float z_b =  dir1.x * ground_normal.x;
	
	Vector dir2(x, y, z_a - z_b);
	VectorNormalize(dir2);
	
	NextBotTraversableTraceFilter filter(this->GetBot(), TRAVERSE_DEFAULT);
	
	unsigned int solid_mask = body->GetSolidMask();
	
	Vector start = feet;
	Vector end   = feet + (dist * dir2);
	
	Ray_t ray;
	ray.Init(start, end, mins, maxs);
	
	trace_t tr;
	enginetrace->TraceRay(ray, solid_mask, &filter, &tr);
	if (tr.fraction >= 1.0f && !tr.startsolid) {
		if (body->IsActualPosture(IBody::POSTURE_CROUCH)) {
			body->SetDesiredPosture(IBody::POSTURE_STAND);
		}
	} else {
		if (!body->IsActualPosture(IBody::POSTURE_CROUCH)) {
			maxs.z = body->GetCrouchHullHeight();
			ray.Init(start, end, mins, maxs);
			
			enginetrace->TraceRay(ray, solid_mask, &filter, &tr);
			if (tr.fraction >= 1.0f && !tr.startsolid) {
				body->SetDesiredPosture(IBody::POSTURE_CROUCH);
			}
		}
	}
}


void ILocomotion::StuckMonitor()
{
	if (this->m_itApproach.IsGreaterThan(0.25f)) {
		this->m_vecStuck = this->GetFeet();
		this->m_itStuck.Start();
		return;
	}
	
	if (this->IsStuck()) {
		if (this->GetBot()->IsRangeGreaterThan(this->m_vecStuck, 100.0f)) {
			this->ClearStuckStatus("UN-STUCK");
		} else if (this->m_ctStuckAlert.IsElapsed()) {
			this->m_ctStuckAlert.Start(1.0f);
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				DevMsg("%3.2f: %s STILL STUCK\n", gpGlobals->curtime, this->GetBot()->GetDebugIdentifier());
				NDebugOverlay::Circle(VecPlusZ(this->m_vecStuck, 5.0f), QAngle(-90.0f, 0.0f, 0.0f), 5.0f, NB_RGBA_RED, true, 1.0f);
			}
			
			this->GetBot()->OnStuck();
		}
	} else {
		if (this->GetBot()->IsRangeGreaterThan(this->m_vecStuck, 100.0f)) {
			this->m_vecStuck = this->GetFeet();
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::Cross3D(this->m_vecStuck, 3.0f, NB_RGB_MAGENTA, true, 3.0f);
			}
			
			this->m_itStuck.Start();
		} else {
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
				NDebugOverlay::Line(this->GetBot()->GetEntity()->WorldSpaceCenter(), this->m_vecStuck, NB_RGB_MAGENTA, true, 0.1f);
			}
			
			if (this->m_itStuck.IsGreaterThan(100.0f / ((this->GetDesiredSpeed() + 1.0f) * 0.1f))) {
				this->m_bStuck = true;
				
				if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOCOMOTION)) {
					DevMsg("%3.2f: %s STUCK at position( %3.2f, %3.2f, %3.2f )\n", gpGlobals->curtime, this->GetBot()->GetDebugIdentifier(), this->m_vecStuck.x, this->m_vecStuck.y, this->m_vecStuck.z);
					
					NDebugOverlay::Circle(VecPlusZ(this->m_vecStuck, 15.0f), QAngle(-90.0f, 0.0f, 0.0f), 3.0f, NB_RGBA_YELLOW, true, 1.0f);
					NDebugOverlay::Circle(VecPlusZ(this->m_vecStuck,  5.0f), QAngle(-90.0f, 0.0f, 0.0f), 5.0f, NB_RGBA_RED,    true, 1.0e+7f);
				}
				
				this->GetBot()->OnStuck();
			}
		}
	}
}


void ILocomotion::TraceHull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace)
{
	Ray_t ray;
	ray.Init(start + maxs, end + maxs, mins, maxs);
	
	enginetrace->TraceRay(ray, mask, filter, trace);
}

bool ILocomotion::ScriptClimbUpToLedge( const Vector &dst, const Vector &dir, HSCRIPT hEntity )
{
	return ClimbUpToLedge( dst, dir, ToEnt( hEntity ) );
}

void ILocomotion::ScriptOnLeaveGround( HSCRIPT hGround )
{
	OnLeaveGround( ToEnt( hGround ) );
}

void ILocomotion::ScriptOnLandOnGround( HSCRIPT hGround )
{
	OnLandOnGround( ToEnt( hGround ) );
}

HSCRIPT ILocomotion::ScriptGetGround()
{
	return ToHScript( GetGround() );
}

bool ILocomotion::ScriptIsAreaTraversable( HSCRIPT hArea )
{
	CNavArea *pArea = HScriptToClass<CNavArea>( hArea );
	if ( pArea )
		return IsAreaTraversable( pArea );

	return false;
}

float ILocomotion::ScriptIsPotentiallyTraversable( const Vector &from, const Vector &to, bool bImmediate )
{
	return IsPotentiallyTraversable( from, to, bImmediate ? TRAVERSE_DEFAULT : TRAVERSE_BREAKABLE );
}

float ILocomotion::ScriptFractionPotentiallyTraversable( const Vector &from, const Vector &to, bool bImmediate )
{
	float flFraction = 0.0f;
	IsPotentiallyTraversable( from, to, bImmediate ? TRAVERSE_DEFAULT : TRAVERSE_BREAKABLE, &flFraction );

	return flFraction;
}

float ILocomotion::ScriptHasPotentialGap( const Vector &from, const Vector &to )
{
	return HasPotentialGap( from, to );
}

float ILocomotion::ScriptFractionPotentialGap( const Vector &from, const Vector &to )
{
	float flFraction = 0.0f;
	HasPotentialGap( from, to, &flFraction );

	return flFraction;
}

bool ILocomotion::ScriptIsEntityTraversable( HSCRIPT hEntity, bool bImmediate )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if ( pEntity )
	{
		return IsEntityTraversable( pEntity, bImmediate ? TRAVERSE_DEFAULT : TRAVERSE_BREAKABLE );
	}

	return false;
}
