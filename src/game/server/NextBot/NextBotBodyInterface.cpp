/* NextBotBodyInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotBodyInterface.h"
#include "NextBotInterface.h"
#include "NextBotKnownEntity.h"
#include "NextBotUtil.h"


bool IBody::SetPosition(const Vector& pos)
{
	this->GetBot()->GetEntity()->SetAbsOrigin(pos);
	return true;
}

const Vector& IBody::GetEyePosition() const
{
	static Vector eye;
	eye = this->GetBot()->GetEntity()->WorldSpaceCenter();
	return eye;
}

const Vector& IBody::GetViewVector() const
{
	static Vector view;
	view = AngleVecFwd(this->GetBot()->GetEntity()->EyeAngles());
	return view;
}

void IBody::AimHeadTowards(const Vector& vec, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	if (reply != nullptr) {
		reply->OnFail(this->GetBot(), INextBotReply::FAIL_UNIMPLEMENTED);
	}
}

void IBody::AimHeadTowards(CBaseEntity *ent, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	if (reply != nullptr) {
		reply->OnFail(this->GetBot(), INextBotReply::FAIL_UNIMPLEMENTED);
	}
}


float IBody::GetHullHeight() const
{
	switch (this->GetActualPosture()) {
		
	default:
	case POSTURE_STAND:
		return this->GetStandHullHeight();
		
	case POSTURE_CROUCH:
	case POSTURE_SIT:
		return this->GetCrouchHullHeight();
		
	case POSTURE_LIE:
		return 16.0f; // TODO: find the source of this magic number
	}
}


const Vector& IBody::GetHullMins() const
{
	static Vector hullMins;
	
	float h_width = this->GetHullWidth();
	
	hullMins.x = -0.5f * h_width;
	hullMins.y = -0.5f * h_width;
	hullMins.z = 0.0f;
	
	return hullMins;
}

const Vector& IBody::GetHullMaxs() const
{
	static Vector hullMaxs;
	
	float h_width  = this->GetHullWidth();
	float h_height = this->GetHullHeight();
	
	hullMaxs.x = 0.5f * h_width;
	hullMaxs.y = 0.5f * h_width;
	hullMaxs.z = h_height;
	
	return hullMaxs;
}
