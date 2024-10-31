/* NextBotBodyInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTBODYINTERFACE_H
#define NEXTBOT_NEXTBOTBODYINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotComponentInterface.h"


class INextBotReply
{
public:
	enum FailureReason
	{
		FAIL_REJECTED      = 0, // AimHeadTowards denied the aim
		FAIL_PREEMPTED     = 1, // a higher priority aim preempted our aim
		FAIL_UNIMPLEMENTED = 2, // subclass didn't override IBody::AimHeadTowards
	};
	
	virtual ~INextBotReply() {}
	
	virtual void OnSuccess(INextBot *nextbot) {}
	virtual void OnFail(INextBot *nextbot, FailureReason reason) {}
	
protected:
	INextBotReply() {}
};


class IBody : public INextBotComponent
{
public:
	enum LookAtPriorityType
	{
		PRI_BORING      = 0,
		PRI_INTERESTING = 1,
		PRI_IMPORTANT   = 2,
		PRI_CRITICAL    = 3,
		PRI_OVERRIDE    = 4,
	};
	
	enum PostureType
	{
		POSTURE_STAND  = 0,
		POSTURE_CROUCH = 1,
		POSTURE_SIT    = 2,
		// 3: some variation on POSTURE_STAND; not used anywhere that I could find
		POSTURE_LIE    = 4,
		
		// Analysis based on L4D ZombieBotBody:
		// #  ACTIVITY  MOBILE  HULLHEIGHT  TARGETPOINT  SETBY
		// 0  stand     yes     68/stand    up 60%       InfectedStandDazed
		// 1  stand     yes     32/crouch   up 25%       
		// 2  sit       no      32/crouch   up 25%       InfectedSitDown
		// 3  stand     yes     68/stand    up 70%       
		// 4  lie       no      16          up 25%       InfectedLieDown
	};
	
	enum ArousalType
	{
		ALERT_YES = 0,
		ALERT_NO  = 1,
	};
	
	IBody(INextBot *nextbot) :
		INextBotComponent(nextbot) {}
	virtual ~IBody() {}
	
	virtual void Reset() override  { INextBotComponent::Reset(); }
	virtual void Update() override {}
	
	virtual bool SetPosition(const Vector& pos);
	virtual const Vector& GetEyePosition() const;
	virtual const Vector& GetViewVector() const;
	
	virtual void AimHeadTowards(const Vector& vec, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason);
	virtual void AimHeadTowards(CBaseEntity *ent, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason);
	
	virtual bool IsHeadAimingOnTarget() const                { return false; }
	virtual bool IsHeadSteady() const                        { return true; }
	virtual float GetHeadSteadyDuration() const              { return 0.0f; }
	virtual float GetHeadAimSubjectLeadTime() const          { return 0.0f; }
	virtual float GetHeadAimTrackingInterval() const         { return 0.0f; }
	
	virtual void ClearPendingAimReply()                      {}
	
	virtual float GetMaxHeadAngularVelocity() const          { return 1000.0f; }
	
	virtual bool StartActivity(Activity a1, unsigned int i1 = 0) { return false; }
	virtual int SelectAnimationSequence(Activity a1) const   { return 0; }
	virtual Activity GetActivity() const                     { return ACT_INVALID; }
	virtual bool IsActivity(Activity a1) const               { return false; }
	virtual bool HasActivityType(unsigned int i1) const      { return false;}
	
	virtual void SetDesiredPosture(PostureType posture)      {}
	virtual PostureType GetDesiredPosture() const            { return POSTURE_STAND; }
	virtual bool IsDesiredPosture(PostureType posture) const { return true; }
	virtual bool IsInDesiredPosture() const                  { return true; }
	
	virtual PostureType GetActualPosture() const             { return POSTURE_STAND; }
	virtual bool IsActualPosture(PostureType posture) const  { return true; }
	virtual bool IsPostureMobile() const                     { return true; }
	virtual bool IsPostureChanging() const                   { return false; }
	
	virtual void SetArousal(ArousalType arousal)             {}
	virtual ArousalType GetArousal() const                   { return ALERT_YES; }
	virtual bool IsArousal(ArousalType arousal) const        { return true; }
	
	virtual float GetHullWidth() const                       { return 26.0f; } // TODO: find the source of this magic number
	virtual float GetHullHeight() const;
	virtual float GetStandHullHeight() const                 { return 68.0f; } // TODO: find the source of this magic number
	virtual float GetCrouchHullHeight() const                { return 32.0f; } // TODO: find the source of this magic number
	virtual const Vector& GetHullMins() const;
	virtual const Vector& GetHullMaxs() const;
	
	virtual unsigned int GetSolidMask() const { return MASK_NPCSOLID; }
	virtual int GetCollisionGroup() const     { return COLLISION_GROUP_NONE; }
};
// TODO: move some of these magic constant values into an enum, at the very least...


#endif
