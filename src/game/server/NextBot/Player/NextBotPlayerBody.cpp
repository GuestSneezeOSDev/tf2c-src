/* NextBotPlayerBody
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotPlayerBody.h"
#include "NextBotPlayerInput.h"
#include "NextBotInterface.h"
#include "NextBotUtil.h"


static ConVar nb_saccade_time            ("nb_saccade_time",              "0.1", FCVAR_CHEAT);
static ConVar nb_saccade_speed           ("nb_saccade_speed",            "1000", FCVAR_CHEAT);
static ConVar nb_head_aim_steady_max_rate("nb_head_aim_steady_max_rate",  "100", FCVAR_CHEAT);
static ConVar nb_head_aim_settle_duration("nb_head_aim_settle_duration",  "0.3", FCVAR_CHEAT);
static ConVar nb_head_aim_resettle_angle ("nb_head_aim_resettle_angle",   "100", FCVAR_CHEAT, "After rotating through this angle, the bot pauses to 'recenter' its virtual mouse on its virtual mousepad");
static ConVar nb_head_aim_resettle_time  ("nb_head_aim_resettle_time",    "0.3", FCVAR_CHEAT, "How long the bot pauses to 'recenter' its virtual mouse on its virtual mousepad");


PlayerBody::PlayerBody(INextBot *nextbot) :
	IBody(nextbot)
{
	this->m_Player = static_cast<CBasePlayer *>(nextbot->GetEntity());
}


void PlayerBody::Reset()
{
	this->m_iPosture = IBody::POSTURE_STAND;
	
	this->m_angLastEyeAngles  = vec3_angle;
	this->m_vecAimTarget      = vec3_origin;
	this->m_vecTargetVelocity = vec3_origin;
	this->m_vecLastEyeVectors = vec3_origin;
	
	this->m_hAimTarget   = nullptr;
	this->m_AimReply     = nullptr;
	this->m_iAimPriority = PRI_BORING;
	
	this->m_ctAimDuration.Invalidate();
	this->m_itAimStart.Invalidate();
	this->m_itHeadSteady.Invalidate();
	this->m_ctResettle.Invalidate();
	
	this->m_bHeadOnTarget = false;
	this->m_bSightedIn    = false;
	
	IBody::Reset();
}

void PlayerBody::Upkeep()
{
	// BUG: why use a ConVarRef??
	static ConVarRef bot_mimic("bot_mimic");
	if (bot_mimic.IsValid() && bot_mimic.GetBool()) return;
	
	if (bot_mimic.GetBool())            return;
	if (gpGlobals->frametime < 1.0e-5f) return;
	
	// BUG: what is the point of this? why not just use this->m_Player?
	auto actor = static_cast<CBasePlayer *>(this->GetBot()->GetEntity());
	
	QAngle eye_angles = actor->EyeAngles() + actor->GetPunchAngle();
	
	bool head_steady;
	if (abs(AngleDiff(eye_angles.x, this->m_angLastEyeAngles.x)) > (gpGlobals->frametime * nb_head_aim_steady_max_rate.GetFloat()) ||
		abs(AngleDiff(eye_angles.y, this->m_angLastEyeAngles.y)) > (gpGlobals->frametime * nb_head_aim_steady_max_rate.GetFloat())) {
		this->m_itHeadSteady.Invalidate();
		head_steady = false;
	} else {
		if (!this->m_itHeadSteady.HasStarted()) this->m_itHeadSteady.Start();
		head_steady = true;
	}
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT) && this->IsHeadSteady()) {
		float radius = 10.0f * Clamp(this->GetHeadSteadyDuration() / 3.0f, 0.0f, 1.0f);
		NDebugOverlay::Circle(actor->EyePosition(), radius, NB_RGBA_GREEN, true, 2.0f * gpGlobals->frametime);
	}
	
	this->m_angLastEyeAngles = eye_angles;
	
	if (this->m_bSightedIn && this->m_ctAimDuration.IsElapsed()) return;
	
	const Vector& eye_vectors = this->GetViewVector();
	if (RAD2DEG(acos(this->m_vecLastEyeVectors.Dot(eye_vectors))) > nb_head_aim_resettle_angle.GetFloat()) {
		this->m_ctResettle.Start(nb_head_aim_resettle_time.GetFloat() * RandomFloat(0.9f, 1.1f));
		this->m_vecLastEyeVectors = eye_vectors;
	} else if (!this->m_ctResettle.HasStarted() || this->m_ctResettle.IsElapsed()) {
		this->m_ctResettle.Invalidate();
		
		if (this->m_hAimTarget != nullptr) {
			if (this->m_ctAimTracking.IsElapsed()) {
				Vector target_point;
				if (this->m_hAimTarget->IsCombatCharacter()) {
					target_point = this->GetBot()->GetIntentionInterface()->SelectTargetPoint(this->GetBot(), this->m_hAimTarget->MyCombatCharacterPointer());
				} else {
					target_point = this->m_hAimTarget->WorldSpaceCenter();
				}
				
				// TODO: clean up the vector math here if possible
				
				Vector delta = target_point - this->m_vecAimTarget + (this->GetHeadAimSubjectLeadTime() * this->m_hAimTarget->GetAbsVelocity());
				float delta_len = VectorNormalize(delta);
				
				float track_interval = Max(this->GetHeadAimTrackingInterval(), gpGlobals->frametime);
				
				float scale = delta_len / track_interval;
				
				this->m_vecTargetVelocity = (scale * delta) + this->m_hAimTarget->GetAbsVelocity();
				
				this->m_ctAimTracking.Start(track_interval * RandomFloat(0.8f, 1.2f));
			}
			
			this->m_vecAimTarget += gpGlobals->frametime * this->m_vecTargetVelocity;
		}
	}
	
	Vector eye_to_target = (this->m_vecAimTarget - this->GetEyePosition()).Normalized();
	QAngle ang_to_target = VectorAngles(eye_to_target);
	
	if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
		NDebugOverlay::Line(this->GetEyePosition(), this->GetEyePosition() + (100.0f * eye_vectors), NB_RGB_YELLOW, false, 2.0f * gpGlobals->frametime);
		
		int r = (this->m_bHeadOnTarget         ? 0xff : 0x00);
		int g = (this->m_hAimTarget != nullptr ? 0xff : 0x00);
		NDebugOverlay::HorzArrow(this->GetEyePosition(), this->m_vecAimTarget, (head_steady ? 2.0f : 3.0f), r, g, 0xff, 0xff, false, 2.0f * gpGlobals->frametime);
	}
	
	float cos_error = eye_to_target.Dot(eye_vectors);
	
	/* must be within ~11.5 degrees to be considered on target */
	if (cos_error <= 0.98f) {
		this->m_bHeadOnTarget = false;
	} else {
		this->m_bHeadOnTarget = true;
		
		if (!this->m_bSightedIn) {
			this->m_bSightedIn = true;
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
				ConColorMsg(NB_COLOR_ORANGE_64, "%3.2f: %s Look At SIGHTED IN\n", gpGlobals->curtime, this->m_Player->GetPlayerName());
			}
		}
		
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnSuccess(this->GetBot());
			this->m_AimReply = nullptr;
		}
	}
	
	float max_angvel = this->GetMaxHeadAngularVelocity();
	
	/* adjust angular velocity limit based on aim error amount */
	if (cos_error > 0.7f) {
		max_angvel *= sin((3.14f / 2.0f) * (1.0f + ((-49.0f / 15.0f) * (cos_error - 0.7f))));
	}
	
	/* start turning gradually during the first quarter second */
	if (this->m_itAimStart.HasStarted() && this->m_itAimStart.IsLessThan(0.25f)) {
		max_angvel *= 4.0f * m_itAimStart.GetElapsedTime();
	}
	
	QAngle new_eye_angle(
		ApproachAngle(ang_to_target.x, eye_angles.x, (max_angvel * gpGlobals->frametime) * 0.5f),
		ApproachAngle(ang_to_target.y, eye_angles.y, (max_angvel * gpGlobals->frametime)),
		0.0f);
	
	new_eye_angle -= actor->GetPunchAngle();
	new_eye_angle.x = AngleNormalize(new_eye_angle.x);
	new_eye_angle.y = AngleNormalize(new_eye_angle.y);
	
	actor->SnapEyeAngles(new_eye_angle);
}


bool PlayerBody::SetPosition(const Vector& pos)
{
	this->m_Player->SetAbsOrigin(pos);
	return true;
}

const Vector& PlayerBody::GetEyePosition() const
{
	this->m_vecEyePosition = this->m_Player->EyePosition();
	return this->m_vecEyePosition;
}

const Vector& PlayerBody::GetViewVector() const
{
	this->m_vecEyeVectors = EyeVectorsFwd(this->m_Player);
	return this->m_vecEyeVectors;
}


void PlayerBody::AimHeadTowards(const Vector& vec, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	if (duration <= 0.0f) duration = 0.1f;
	
	if (priority == this->m_iAimPriority) {
		if (!this->IsHeadSteady() || this->GetHeadSteadyDuration() < nb_head_aim_settle_duration.GetFloat()) {
			if (reply != nullptr) {
				reply->OnFail(this->GetBot(), INextBotReply::FAIL_REJECTED);
			}
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
				ConColorMsg(NB_COLOR_RED, "%3.2f: %s Look At '%s' rejected - previous aim not %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(), reason,
					(this->IsHeadSteady() ? "settled long enough" : "head-steady"));
			}
			
			return;
		}
	}
	
	if (priority >= this->m_iAimPriority || this->m_ctAimDuration.IsElapsed()) {
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnFail(this->GetBot(), INextBotReply::FAIL_PREEMPTED);
		}
		this->m_AimReply = reply;
		
		this->m_ctAimDuration.Start(duration);
		this->m_iAimPriority = priority;
		
		/* only update our aim if the target vector changed significantly */
		if (vec.DistToSqr(this->m_vecAimTarget) >= Square(1.0f)) {
			this->m_hAimTarget = nullptr;
			this->m_vecAimTarget = vec;
			this->m_itAimStart.Start();
			this->m_bHeadOnTarget = false;
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
				NDebugOverlay::Cross3D(vec, 2.0f, NB_RGB_LTYELLOW_64, true, 2.0f * duration);
				
				const char *pri_str = "";
				switch (priority) {
				case PRI_BORING:      pri_str = "Boring";      break;
				case PRI_INTERESTING: pri_str = "Interesting"; break;
				case PRI_IMPORTANT:   pri_str = "Important";   break;
				case PRI_CRITICAL:    pri_str = "Critical";    break;
				case PRI_OVERRIDE:    pri_str = "Override";    break;
				}
				
				ConColorMsg(NB_COLOR_ORANGE_64, "%3.2f: %s Look At ( %g, %g, %g ) for %3.2f s, Pri = %s, Reason = %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(), vec.x, vec.y, vec.z,
					duration, pri_str, (reason != nullptr ? reason : ""));
			}
		}
	} else {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(), INextBotReply::FAIL_REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
			ConColorMsg(NB_COLOR_RED, "%3.2f: %s Look At '%s' rejected - higher priority aim in progress\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason);
		}
	}
}

void PlayerBody::AimHeadTowards(CBaseEntity *ent, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	Assert(ent != nullptr);
	if (ent == nullptr) return;
	
	if (duration <= 0.0f) duration = 0.1f;
	
	if (priority == this->m_iAimPriority) {
		if (!this->IsHeadSteady() || this->GetHeadSteadyDuration() < nb_head_aim_settle_duration.GetFloat()) {
			if (reply != nullptr) {
				reply->OnFail(this->GetBot(), INextBotReply::FAIL_REJECTED);
			}
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
				ConColorMsg(NB_COLOR_RED, "%3.2f: %s Look At '%s' rejected - previous aim not %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(), reason,
					(this->IsHeadSteady() ? "settled long enough" : "head-steady"));
			}
			
			return;
		}
	}
	
	if (priority >= this->m_iAimPriority || this->m_ctAimDuration.IsElapsed()) {
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnFail(this->GetBot(), INextBotReply::FAIL_PREEMPTED);
		}
		this->m_AimReply = reply;
		
		this->m_ctAimDuration.Start(duration);
		this->m_iAimPriority = priority;
		
		/* only update our aim if the target entity changed */
		if (this->m_hAimTarget == nullptr || this->m_hAimTarget != ent) {
			this->m_hAimTarget = ent;
			this->m_itAimStart.Start();
			this->m_bHeadOnTarget = false;
			
			if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
				NDebugOverlay::Cross3D(this->m_vecAimTarget, 2.0f, NB_RGB_GRAY, true, duration);
				
				const char *pri_str = "";
				switch (priority) {
				case PRI_BORING:      pri_str = "Boring";      break;
				case PRI_INTERESTING: pri_str = "Interesting"; break;
				case PRI_IMPORTANT:   pri_str = "Important";   break;
				case PRI_CRITICAL:    pri_str = "Critical";    break;
				case PRI_OVERRIDE:    pri_str = "Override";    break;
				}
				
				ConColorMsg(NB_COLOR_ORANGE_64, "%3.2f: %s Look At subject %s for %3.2f s, Pri = %s, Reason = %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(), ent->GetClassname(),
					duration, pri_str, (reason != nullptr ? reason : ""));
			}
		}
	} else {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(), INextBotReply::FAIL_REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(INextBot::DEBUG_LOOK_AT)) {
			ConColorMsg(NB_COLOR_RED, "%3.2f: %s Look At '%s' rejected - higher priority aim in progress\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason);
		}
	}
}


float PlayerBody::GetHeadSteadyDuration() const
{
	if (!this->m_itHeadSteady.HasStarted()) return 0.0f;
	
	return this->m_itHeadSteady.GetElapsedTime();
}


float PlayerBody::GetMaxHeadAngularVelocity() const
{
	return nb_saccade_speed.GetFloat();
}


float PlayerBody::GetHullHeight() const
{
	if (this->m_iPosture == IBody::POSTURE_CROUCH) {
		return this->GetCrouchHullHeight();
	} else {
		return this->GetStandHullHeight();
	}
}

const Vector& PlayerBody::GetHullMins() const
{
	if (this->m_iPosture == IBody::POSTURE_CROUCH) {
		this->m_vecHullMins = VEC_DUCK_HULL_MIN_SCALED(this->m_Player);
	} else {
		this->m_vecHullMins = VEC_HULL_MIN_SCALED(this->m_Player);
	}
	
	return this->m_vecHullMins;
}

const Vector& PlayerBody::GetHullMaxs() const
{
	if (this->m_iPosture == IBody::POSTURE_CROUCH) {
		this->m_vecHullMaxs = VEC_DUCK_HULL_MAX_SCALED(this->m_Player);
	} else {
		this->m_vecHullMaxs = VEC_HULL_MAX_SCALED(this->m_Player);
	}
	
	return this->m_vecHullMaxs;
}


unsigned int PlayerBody::GetSolidMask() const
{
	if (this->m_Player != nullptr) {
		return this->m_Player->PlayerSolidMask();
	} else {
		return MASK_PLAYERSOLID;
	}
}


void PressFireButtonReply::OnSuccess(INextBot *nextbot)
{
	auto input = dynamic_cast<INextBotPlayerInput *>(nextbot->GetEntity());
	if (input != nullptr) input->PressFireButton();
}

void PressAltFireButtonReply::OnSuccess(INextBot *nextbot)
{
	auto input = dynamic_cast<INextBotPlayerInput *>(nextbot->GetEntity());
	if (input != nullptr) input->PressAltFireButton();
}

void PressSpecialFireButtonReply::OnSuccess(INextBot *nextbot)
{
	auto input = dynamic_cast<INextBotPlayerInput *>(nextbot->GetEntity());
	if (input != nullptr) input->PressSpecialFireButton();
}

void PressJumpButtonReply::OnSuccess(INextBot *nextbot)
{
	auto input = dynamic_cast<INextBotPlayerInput *>(nextbot->GetEntity());
	if (input != nullptr) input->PressJumpButton();
}
