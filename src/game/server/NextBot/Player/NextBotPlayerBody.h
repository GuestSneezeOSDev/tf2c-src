/* NextBotPlayerBody
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERBODY_H
#define NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERBODY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBodyInterface.h"


class PlayerBody : public IBody
{
public:
	PlayerBody(INextBot *nextbot);
	virtual ~PlayerBody() {}
	
	virtual void Reset() override;
	virtual void Upkeep() override;
	
	virtual bool SetPosition(const Vector& pos) override;
	virtual const Vector& GetEyePosition() const override;
	virtual const Vector& GetViewVector() const override;
	
	virtual void AimHeadTowards(const Vector& vec, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason) override;
	virtual void AimHeadTowards(CBaseEntity *ent, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason) override;
	
	virtual bool IsHeadAimingOnTarget() const override                { return this->m_bHeadOnTarget; }
	virtual bool IsHeadSteady() const override                        { return this->m_itHeadSteady.HasStarted(); }
	virtual float GetHeadSteadyDuration() const override;
	
	virtual void ClearPendingAimReply() override                      { this->m_AimReply = nullptr; }
	
	virtual float GetMaxHeadAngularVelocity() const override;
	
	virtual bool StartActivity(Activity a1, unsigned int i1) override { return false; }
	virtual Activity GetActivity() const override                     { return ACT_INVALID; }
	virtual bool IsActivity(Activity a1) const override               { return false; }
	virtual bool HasActivityType(unsigned int i1) const override      { return false; }
	
	virtual void SetDesiredPosture(PostureType posture) override      { this->m_iPosture = posture; }
	virtual PostureType GetDesiredPosture() const override            { return this->m_iPosture; }
	virtual bool IsDesiredPosture(PostureType posture) const override { return (posture == this->m_iPosture); }
	virtual bool IsInDesiredPosture() const override                  { return true;}
	
	virtual PostureType GetActualPosture() const override             { return this->m_iPosture;}
	virtual bool IsActualPosture(PostureType posture) const override  { return (posture == this->m_iPosture); }
	virtual bool IsPostureMobile() const override                     { return true; }
	virtual bool IsPostureChanging() const override                   { return false; }
	
	virtual void SetArousal(ArousalType arousal) override             { this->m_iArousal = arousal; }
	virtual ArousalType GetArousal() const override                   { return this->m_iArousal; }
	virtual bool IsArousal(ArousalType arousal) const override        { return (arousal == this->m_iArousal); }
	
	virtual float GetHullWidth() const override                       { return VEC_HULL_MAX_SCALED(this->m_Player).x - VEC_HULL_MIN_SCALED(this->m_Player).x; }
	virtual float GetHullHeight() const override;
	virtual float GetStandHullHeight() const override                 { return VEC_HULL_MAX_SCALED(this->m_Player).z - VEC_HULL_MIN_SCALED(this->m_Player).z; }
	virtual float GetCrouchHullHeight() const override                { return VEC_DUCK_HULL_MAX_SCALED(this->m_Player).z - VEC_DUCK_HULL_MIN_SCALED(this->m_Player).z; }
	virtual const Vector& GetHullMins() const override;
	virtual const Vector& GetHullMaxs() const override;
	
	virtual unsigned int GetSolidMask() const override;
	
	virtual CBaseCombatCharacter *GetEntity()                         { return this->m_Player; }
	
private:
	CBasePlayer *m_Player;
	PostureType m_iPosture;
	ArousalType m_iArousal;
	mutable Vector m_vecEyePosition;
	mutable Vector m_vecEyeVectors;
	mutable Vector m_vecHullMins;
	mutable Vector m_vecHullMaxs;
	Vector m_vecAimTarget;
	CHandle<CBaseEntity> m_hAimTarget;
	Vector m_vecTargetVelocity;
	CountdownTimer m_ctAimTracking;
	LookAtPriorityType m_iAimPriority;
	CountdownTimer m_ctAimDuration;
	IntervalTimer m_itAimStart;
	INextBotReply *m_AimReply;
	bool m_bHeadOnTarget;
	bool m_bSightedIn;
	IntervalTimer m_itHeadSteady;
	QAngle m_angLastEyeAngles;
	CountdownTimer m_ctResettle;
	Vector m_vecLastEyeVectors;
};


class PressFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressAltFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressSpecialFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressJumpButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};


#endif
