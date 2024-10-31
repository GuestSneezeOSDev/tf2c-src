/* NextBotInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTINTERFACE_H
#define NEXTBOT_NEXTBOTINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotEventResponderInterface.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotIntentionInterface.h"
#include "NextBotVisionInterface.h"


class NextBotCombatCharacter;
class PathFollower;


class INextBot : public INextBotEventResponder
{
public:
	enum NextBotDebugType
	{
		DEBUG_NONE = 0x0000,
		DEBUG_ANY  = 0xffff,
		
		DEBUG_BEHAVIOR   = (1 << 0),
		DEBUG_LOOK_AT    = (1 << 1),
		DEBUG_PATH       = (1 << 2),
		DEBUG_ANIMATION  = (1 << 3),
		DEBUG_LOCOMOTION = (1 << 4),
		DEBUG_VISION     = (1 << 5),
		DEBUG_HEARING    = (1 << 6),
		DEBUG_EVENTS     = (1 << 7),
		DEBUG_ERRORS     = (1 << 8),
	};
	
	struct NextBotDebugLineType
	{
		NextBotDebugType type;
		char buf[0x100];
	};
	
	INextBot();
	virtual ~INextBot();
	
	virtual INextBotEventResponder *FirstContainedResponder() const override                            { return this->m_ComponentList; }
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override { return static_cast<INextBotComponent *>(prev)->GetNextComponent(); }
	
	virtual void Reset();
	virtual void Update();
	virtual void Upkeep();
	
	virtual bool IsRemovedOnReset() const { return true; }
	
	virtual CBaseCombatCharacter *GetEntity() const = 0;
	virtual NextBotCombatCharacter *GetNextBotCombatCharacter() const { return nullptr; }
	
	virtual ILocomotion *GetLocomotionInterface() const;
	virtual IBody *GetBodyInterface() const;
	virtual IIntention *GetIntentionInterface() const;
	virtual IVision *GetVisionInterface() const;
	
	virtual bool SetPosition(const Vector& pos);
	virtual const Vector& GetPosition() const;
	
	virtual bool IsEnemy(const CBaseEntity *ent) const;
	virtual bool IsFriend(const CBaseEntity *ent) const;
	virtual bool IsSelf(const CBaseEntity *ent) const;
	
	virtual bool IsAbleToClimbOnto(const CBaseEntity *ent) const;
	virtual bool IsAbleToBreak(const CBaseEntity *ent) const;
	virtual bool IsAbleToBlockMovementOf(const INextBot *nextbot) const { return true; }
	
	virtual bool ShouldTouch(const CBaseEntity *ent) const { return true; }
	
	virtual bool IsImmobile() const           { return this->m_itImmobileEpoch.HasStarted(); }
	virtual float GetImmobileDuration() const { return this->m_itImmobileEpoch.GetElapsedTime(); }
	virtual void ClearImmobileStatus();
	virtual float GetImmobileSpeedThreshold() const { return 30.0f; }
	
	virtual const PathFollower *GetCurrentPath() const        { return this->m_CurrentPath; }
	virtual void SetCurrentPath(const PathFollower *follower) { this->m_CurrentPath = follower; }
	virtual void NotifyPathDestruction(const PathFollower *follower);
	
	virtual bool IsRangeLessThan(CBaseEntity *ent, float dist) const;
	virtual bool IsRangeLessThan(const Vector& vec, float dist) const;
	virtual bool IsRangeGreaterThan(CBaseEntity *ent, float dist) const;
	virtual bool IsRangeGreaterThan(const Vector& vec, float dist) const;
	
	virtual float GetRangeTo(CBaseEntity *ent) const;
	virtual float GetRangeTo(const Vector& vec) const;
	virtual float GetRangeSquaredTo(CBaseEntity *ent) const;
	virtual float GetRangeSquaredTo(const Vector& vec) const;
	
	virtual bool IsDebugging(unsigned int type) const;
	virtual char *GetDebugIdentifier() const;
	virtual bool IsDebugFilterMatch(const char *filter) const;
	virtual void DisplayDebugText(const char *text) const;
	
	virtual float GetDesiredPathLookAheadRange() const = 0;
	
	virtual void RemoveEntity();
	
	void DebugConColorMsg(NextBotDebugType type, const Color& color, const char *fmt, ...);
	
	void RegisterComponent(INextBotComponent *component);
	
	int GetTicksSinceUpdate() const { return gpGlobals->tickcount - this->m_iLastUpdateTick; }
	void ResetLastUpdateTick()      { this->m_iLastUpdateTick = gpGlobals->tickcount; }
	
	bool IsFlaggedForUpdate() const { return this->m_bScheduledForNextTick; }
	void FlagForUpdate(bool sched)  { this->m_bScheduledForNextTick = sched; }
	
	int GetBotId() const { return this->m_iManagerIndex; }

	int GetTickLastUpdate() const { return this->m_iLastUpdateTick; }

	HSCRIPT ScriptGetLocomotionInterface();
	HSCRIPT ScriptGetBodyInterface();
	HSCRIPT ScriptGetIntentionInterface();
	HSCRIPT ScriptGetVisionInterface();
	bool ScriptIsEnemy( HSCRIPT hEntity );
	bool ScriptIsFriend( HSCRIPT hEntity );
	
protected:
	bool BeginUpdate();
	void EndUpdate();
	
private:
	enum
	{
		MAX_DEBUG_LINES = 100,
	};
	
	void UpdateImmobileStatus();
	
	void GetDebugHistory(unsigned int mask, CUtlVector<const NextBotDebugLineType *> *dst) const;
	void ResetDebugHistory();
	
	INextBotComponent *m_ComponentList = nullptr;
	const PathFollower *m_CurrentPath = nullptr;
	int m_iManagerIndex = -1;
	bool m_bScheduledForNextTick;
	int m_iLastUpdateTick = -999;
	int m_Dword18; // TODO: name
	mutable int m_iDebugTextOffset = 0;
	Vector m_vecLastPosition = vec3_origin;
	CountdownTimer m_ctImmobileCheck;
	IntervalTimer m_itImmobileEpoch;
	mutable ILocomotion *m_LocomotionInterface;
	mutable IBody *m_BodyInterface;
	mutable IIntention *m_IntentionInterface;
	mutable IVision *m_VisionInterface;
	CUtlVector<NextBotDebugLineType *> m_DebugLines;
};


inline INextBot *ToNextBot(CBaseEntity *ent)
{
	if (ent == nullptr) return nullptr;
	return ent->MyNextBotPointer();
}
inline const INextBot *ToNextBot(const CBaseEntity *ent)
{
	if (ent == nullptr) return nullptr;
	return const_cast<CBaseEntity *>(ent)->MyNextBotPointer();
}


#endif
