/* NextBotBehavior
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTBEHAVIOR_H
#define NEXTBOT_NEXTBOTBEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif


//#include "valve_minmax_off.h"

#include "NextBotInterface.h"
#include "NextBotUtil.h"
#include "fmtstr.h"
#include "utlstring.h"


extern ConVar NextBotDebugHistory;


// TODO:
// figure out how mat_queue_mode 2 makes serial DevMsg type calls end up out of order
// and hopefully find a way to fix that

// SEE: CMaterialSystem::AllowThreading (in current materialsystem DLL)
// was previously named Host_AllowQueuedMaterialSystem in engine DLL in 2007 engine
// POSSIBLE DIFFICULTY: will spew crap to console; also I think it may deny calls from main thread


enum ActionTransition
{
	ACT_T_CONTINUE    = 0,
	ACT_T_CHANGE_TO   = 1,
	ACT_T_SUSPEND_FOR = 2,
	ACT_T_DONE        = 3,
	ACT_T_SUSTAIN     = 4,
};

enum EventResultSeverity
{
	SEV_ZERO     = 0,
	SEV_LOW      = 1,
	SEV_MEDIUM   = 2,
	SEV_CRITICAL = 3,
};


template<typename T> class Action;


template<typename T>
struct ActionResult
{
	ActionResult() = default;
	ActionResult(ActionTransition transition, Action<T> *action, const char *reason) :
		transition(transition), action(action), reason(reason) {}
	
	ActionTransition transition = ACT_T_CONTINUE;
	Action<T> *action  = nullptr;
	const char *reason = nullptr;
	
	void Reset()
	{
		memset(this, 0x00, sizeof(*this));
	}
	
	const char *GetString_Transition() const
	{
		Assert(this->transition >= ACT_T_CONTINUE);
		Assert(this->transition <= ACT_T_SUSTAIN);
		
		switch (this->transition) {
		case ACT_T_CONTINUE:    return "CONTINUE";
		case ACT_T_CHANGE_TO:   return "CHANGE_TO";
		case ACT_T_SUSPEND_FOR: return "SUSPEND_FOR";
		case ACT_T_DONE:        return "DONE";
		case ACT_T_SUSTAIN:     return "SUSTAIN";
		default:                return "";
		}
	}
	const char *GetString_Action() const { return (this->action != nullptr ? this->action->GetName() : ""); }
	const char *GetString_Reason() const { return (this->reason != nullptr ? this->reason            : ""); }
	
	static ActionResult<T> Continue(Action<T> *current);
	static ActionResult<T> ChangeTo(Action<T> *current, Action<T> *next, const char *why = nullptr);
	static ActionResult<T> SuspendFor(Action<T> *current, Action<T> *next, const char *why = nullptr);
	static ActionResult<T> Done(Action<T> *current, const char *why = nullptr);
	static ActionResult<T> Sustain(Action<T> *current, const char *why = nullptr);
};


template<typename T>
struct EventDesiredResult : public ActionResult<T>
{
	EventDesiredResult() = default;
	EventDesiredResult(ActionTransition transition, Action<T> *action, const char *reason, EventResultSeverity severity) :
		ActionResult<T>(transition, action, reason), severity(severity) {}
	
	EventResultSeverity severity = SEV_ZERO;
	
	void Reset()
	{
		memset(this, 0x00, sizeof(*this));
	}
	
	static EventDesiredResult<T> Continue(Action<T> *current, EventResultSeverity level = SEV_LOW);
	static EventDesiredResult<T> ChangeTo(Action<T> *current, Action<T> *next, const char *why = nullptr, EventResultSeverity level = SEV_LOW);
	static EventDesiredResult<T> SuspendFor(Action<T> *current, Action<T> *next, const char *why = nullptr, EventResultSeverity level = SEV_LOW);
	static EventDesiredResult<T> Done(Action<T> *current, const char *why = nullptr, EventResultSeverity level = SEV_LOW);
	static EventDesiredResult<T> Sustain(Action<T> *current, const char *why = nullptr, EventResultSeverity level = SEV_LOW);
};


#define BEHAVIOR_HANDLE_QUERY(func, neutral, ...) \
	if (this->m_MainAction == nullptr) return neutral; \
	Action<T> *action = this->m_MainAction->GetDeepestActionChild(); \
	do { \
		Action<T> *parent = action->m_ActionParent; \
		while (action != nullptr) { \
			auto response = action->func(nextbot, ##__VA_ARGS__); \
			if (response != neutral) return response; \
			action = action->m_ActionWeSuspended; \
		} \
		action = parent; \
	} while (action != nullptr); \
	return neutral;


template<typename T>
class Behavior final : public INextBotEventResponder, public IContextualQuery
{
public:
	Behavior(Action<T> *main_action, const char *name = "") :
		m_MainAction(main_action), m_strName(name) {}
	virtual ~Behavior();
	
	/* INextBotEventResponder overrides */
	virtual INextBotEventResponder *FirstContainedResponder() const override final                            { return this->m_MainAction; }
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override final { return nullptr; }
	
	/* IContextualQuery overrides */
	virtual ThreeState_t ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const override final                                                                                                { BEHAVIOR_HANDLE_QUERY(ShouldPickUp, TRS_NONE, it); }
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override final                                                                                                                  { BEHAVIOR_HANDLE_QUERY(ShouldHurry, TRS_NONE); }
	virtual ThreeState_t ShouldRetreat(const INextBot *nextbot) const override final                                                                                                                { BEHAVIOR_HANDLE_QUERY(ShouldRetreat, TRS_NONE); }
	virtual ThreeState_t ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override final                                                                                     { BEHAVIOR_HANDLE_QUERY(ShouldAttack, TRS_NONE, threat); }
	virtual ThreeState_t IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override final                                                                                                 { BEHAVIOR_HANDLE_QUERY(IsHindrance, TRS_NONE, it); }
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override final                                                                                { BEHAVIOR_HANDLE_QUERY(SelectTargetPoint, vec3_origin, them); }
	virtual ThreeState_t IsPositionAllowed(const INextBot *nextbot, const Vector& pos) const override final                                                                                         { BEHAVIOR_HANDLE_QUERY(IsPositionAllowed, TRS_NONE, pos); }
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override final { BEHAVIOR_HANDLE_QUERY(SelectMoreDangerousThreat, nullptr, them, threat1, threat2); }
	
	void Update(T *actor, float dt);
	void DestroyAction(Action<T> *action) { this->m_DestroyedActions.AddToTail(action); }
	
	const char *GetName() const { return this->m_strName; }
	
private:
	Action<T> *m_MainAction;
	CUtlString m_strName; // was: CFmtStrN<32>
	T *m_Actor = nullptr;
	CUtlVectorAutoPurge<Action<T> *> m_DestroyedActions;
};


#define ACTION_HANDLE_EVENT(func, ...) \
	HandleEvent(this, #func, [&](Action<T> *action, T *actor){ return action->func(actor, ##__VA_ARGS__); }); \
	INextBotEventResponder::func(__VA_ARGS__)


template<typename T>
class Action : public INextBotEventResponder, public IContextualQuery
{
public:
	friend class Behavior<T>;
	
	Action() {}
	virtual ~Action();
	
	/* INextBotEventResponder overrides */
	virtual INextBotEventResponder *FirstContainedResponder() const override final                            { return this->m_ActionChild; }
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override final { return nullptr; }
	
	virtual void OnLeaveGround(CBaseEntity *ent) override final                                                       { ACTION_HANDLE_EVENT(OnLeaveGround, ent); }
	virtual void OnLandOnGround(CBaseEntity *ent) override final                                                      { ACTION_HANDLE_EVENT(OnLandOnGround, ent); }
	
	virtual void OnContact(CBaseEntity *ent, CGameTrace *trace) override final                                        { ACTION_HANDLE_EVENT(OnContact, ent, trace); }
	
	virtual void OnMoveToSuccess(const Path *path) override final                                                     { ACTION_HANDLE_EVENT(OnMoveToSuccess, path); }
	virtual void OnMoveToFailure(const Path *path, MoveToFailureType fail) override final                             { ACTION_HANDLE_EVENT(OnMoveToFailure, path, fail); }
	
	virtual void OnStuck() override final                                                                             { ACTION_HANDLE_EVENT(OnStuck); }
	virtual void OnUnStuck() override final                                                                           { ACTION_HANDLE_EVENT(OnUnStuck); }
	
	virtual void OnPostureChanged() override final                                                                    { ACTION_HANDLE_EVENT(OnPostureChanged); }
	virtual void OnAnimationActivityComplete(int i1) override final                                                   { ACTION_HANDLE_EVENT(OnAnimationActivityComplete, i1); }
	virtual void OnAnimationActivityInterrupted(int i1) override final                                                { ACTION_HANDLE_EVENT(OnAnimationActivityInterrupted, i1); }
	virtual void OnAnimationEvent(animevent_t *a1) override final                                                     { ACTION_HANDLE_EVENT(OnAnimationEvent, a1); }
	
	virtual void OnIgnite() override final                                                                            { ACTION_HANDLE_EVENT(OnIgnite); }
	virtual void OnInjured(const CTakeDamageInfo& info) override final                                                { ACTION_HANDLE_EVENT(OnInjured, info); }
	virtual void OnKilled(const CTakeDamageInfo& info) override final                                                 { ACTION_HANDLE_EVENT(OnKilled, info); }
	virtual void OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info) override final                 { ACTION_HANDLE_EVENT(OnOtherKilled, who, info); }
	
	virtual void OnSight(CBaseEntity *ent) override final                                                             { ACTION_HANDLE_EVENT(OnSight, ent); }
	virtual void OnLostSight(CBaseEntity *ent) override final                                                         { ACTION_HANDLE_EVENT(OnLostSight, ent); }
	
	virtual void OnSound(CBaseEntity *ent, const Vector& where, KeyValues *kv) override final                         { ACTION_HANDLE_EVENT(OnSound, ent, where, kv); }
	virtual void OnSpokeConcept(CBaseCombatCharacter *who, const char *aiconcept, AI_Response *response) override final { ACTION_HANDLE_EVENT(OnSpokeConcept, who, aiconcept, response); }
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon) override final                   { ACTION_HANDLE_EVENT(OnWeaponFired, who, weapon); }
	
	virtual void OnNavAreaChanged(CNavArea *area1, CNavArea *area2) override final                                    { ACTION_HANDLE_EVENT(OnNavAreaChanged, area1, area2); }
	virtual void OnModelChanged() override final                                                                      { ACTION_HANDLE_EVENT(OnModelChanged); }
	virtual void OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who) override final                                 { ACTION_HANDLE_EVENT(OnPickUp, ent, who); }
	virtual void OnDrop(CBaseEntity *ent) override final                                                              { ACTION_HANDLE_EVENT(OnDrop, ent); }
	virtual void OnActorEmoted(CBaseCombatCharacter *who, int emoteconcept) override final                            { ACTION_HANDLE_EVENT(OnActorEmoted, who, emoteconcept); }
	
	virtual void OnCommandAttack(CBaseEntity *ent) override final                                                     { ACTION_HANDLE_EVENT(OnCommandAttack, ent); }
	virtual void OnCommandApproach(const Vector& where, float f1) override final                                      { ACTION_HANDLE_EVENT(OnCommandApproach, where, f1); }
	virtual void OnCommandApproach(CBaseEntity *ent) override final                                                   { ACTION_HANDLE_EVENT(OnCommandApproach, ent); }
	virtual void OnCommandRetreat(CBaseEntity *ent, float f1) override final                                          { ACTION_HANDLE_EVENT(OnCommandRetreat, ent, f1); }
	virtual void OnCommandPause(float f1) override final                                                              { ACTION_HANDLE_EVENT(OnCommandPause, f1); }
	virtual void OnCommandResume() override final                                                                     { ACTION_HANDLE_EVENT(OnCommandResume); }
	virtual void OnCommandString(const char *cmd) override final                                                      { ACTION_HANDLE_EVENT(OnCommandString, cmd); }
	
	virtual void OnShoved(CBaseEntity *ent) override final                                                            { ACTION_HANDLE_EVENT(OnShoved, ent); }
	virtual void OnBlinded(CBaseEntity *ent) override final                                                           { ACTION_HANDLE_EVENT(OnBlinded, ent); }
	
	virtual void OnTerritoryContested(int idx) override final                                                         { ACTION_HANDLE_EVENT(OnTerritoryContested, idx); }
	virtual void OnTerritoryCaptured(int idx) override final                                                          { ACTION_HANDLE_EVENT(OnTerritoryCaptured, idx); }
	virtual void OnTerritoryLost(int idx) override final                                                              { ACTION_HANDLE_EVENT(OnTerritoryLost, idx); }
	
	virtual void OnWin() override final                                                                               { ACTION_HANDLE_EVENT(OnWin); }
	virtual void OnLose() override final                                                                              { ACTION_HANDLE_EVENT(OnLose); }
	
	
	virtual const char *GetName() const = 0;
	virtual bool IsNamed(const char *name) const { return FStrEq(this->GetName(), name); }
	virtual const char *GetFullName() const;
	
	virtual ActionResult<T> OnStart(T *actor, Action<T> *action)   { return ActionResult<T>::Continue(this); }
	virtual ActionResult<T> Update(T *actor, float dt)             { return ActionResult<T>::Continue(this); }
	virtual void OnEnd(T *actor, Action<T> *action)                {}
	
	virtual ActionResult<T> OnSuspend(T *actor, Action<T> *action) { return ActionResult<T>::Continue(this); }
	virtual ActionResult<T> OnResume(T *actor, Action<T> *action)  { return ActionResult<T>::Continue(this); }
	
	virtual Action<T> *InitialContainedAction(T *actor) { return nullptr; }
	
	virtual EventDesiredResult<T> OnLeaveGround(T *actor, CBaseEntity *ent)                                                       { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnLandOnGround(T *actor, CBaseEntity *ent)                                                      { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnContact(T *actor, CBaseEntity *ent, CGameTrace *trace)                                        { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnMoveToSuccess(T *actor, const Path *path)                                                     { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnMoveToFailure(T *actor, const Path *path, MoveToFailureType fail)                             { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnStuck(T *actor)                                                                               { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnUnStuck(T *actor)                                                                             { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnPostureChanged(T *actor)                                                                      { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnAnimationActivityComplete(T *actor, int i1)                                                   { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnAnimationActivityInterrupted(T *actor, int i1)                                                { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnAnimationEvent(T *actor, animevent_t *a1)                                                     { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnIgnite(T *actor)                                                                              { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnInjured(T *actor, const CTakeDamageInfo& info)                                                { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnKilled(T *actor, const CTakeDamageInfo& info)                                                 { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnOtherKilled(T *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)                 { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnSight(T *actor, CBaseEntity *ent)                                                             { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnLostSight(T *actor, CBaseEntity *ent)                                                         { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnSound(T *actor, CBaseEntity *ent, const Vector& where, KeyValues *kv)                         { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnSpokeConcept(T *actor, CBaseCombatCharacter *who, const char * aiconcept, AI_Response *response) { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnWeaponFired(T *actor, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)                   { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnNavAreaChanged(T *actor, CNavArea *area1, CNavArea *area2)                                    { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnModelChanged(T *actor)                                                                        { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnPickUp(T *actor, CBaseEntity *ent, CBaseCombatCharacter *who)                                 { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnDrop(T *actor, CBaseEntity *ent)                                                              { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnActorEmoted(T *actor, CBaseCombatCharacter *who, int emoteconcept)                            { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnCommandAttack(T *actor, CBaseEntity *ent)                                                     { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandApproach(T *actor, const Vector& where, float f1)                                      { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandApproach(T *actor, CBaseEntity *ent)                                                   { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandRetreat(T *actor, CBaseEntity *ent, float f1)                                          { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandPause(T *actor, float f1)                                                              { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandResume(T *actor)                                                                       { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnCommandString(T *actor, const char *cmd)                                                      { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnShoved(T *actor, CBaseEntity *ent)                                                            { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnBlinded(T *actor, CBaseEntity *ent)                                                           { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnTerritoryContested(T *actor, int idx)                                                         { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnTerritoryCaptured(T *actor, int idx)                                                          { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnTerritoryLost(T *actor, int idx)                                                              { return EventDesiredResult<T>::Continue(this); }
	
	virtual EventDesiredResult<T> OnWin(T *actor)                                                                                 { return EventDesiredResult<T>::Continue(this); }
	virtual EventDesiredResult<T> OnLose(T *actor)                                                                                { return EventDesiredResult<T>::Continue(this); }
	
	virtual bool IsAbleToBlockMovementOf(const INextBot *nextbot) const { return true; }
	
	void ClearPendingEventResult() { this->m_Result.Reset(); }
	
protected:
	Behavior<T> *GetBehavior() const { return this->m_Behavior; }
	T *GetActor() const              { return this->m_Actor; }
	
private:
	Action<T> *ApplyResult(T *actor, Behavior<T> *behavior, ActionResult<T> result);
	
	ActionResult<T> InvokeOnStart(T *actor, Behavior<T> *behavior, Action<T> *action1, Action<T> *action2);
	ActionResult<T> InvokeUpdate(T *actor, Behavior<T> *behavior, float dt);
	void InvokeOnEnd(T *actor, Behavior<T> *behavior, Action<T> *action);
	
	ActionResult<T> InvokeOnResume(T *actor, Behavior<T> *behavior, Action<T> *action);
	Action<T> *InvokeOnSuspend(T *actor, Behavior<T> *behavior, Action<T> *action);
	
	template<size_t LEN> const char *BuildDecoratedName(char (&buf)[LEN], const Action<T> *action) const;
	const char *DebugString() const;
	void PrintStateToConsole() const;
	
	template<typename Functor> static void HandleEvent(Action<T> *action, const char *name, Functor&& handler);
	
	void StorePendingEventResult(const EventDesiredResult<T>& result, const char *event);
	
	const Action<T> *GetDeepestActionParent() const;
	const Action<T> *GetDeepestActionChild() const;
	const Action<T> *GetDeepestActionWeSuspended() const;
	const Action<T> *GetDeepestActionSuspendedUs() const;
	
	Action<T> *GetDeepestActionParent()      { return const_cast<Action<T> *>(const_cast<const Action<T> *>(this)->GetDeepestActionParent()); }
	Action<T> *GetDeepestActionChild()       { return const_cast<Action<T> *>(const_cast<const Action<T> *>(this)->GetDeepestActionChild()); }
	Action<T> *GetDeepestActionWeSuspended() { return const_cast<Action<T> *>(const_cast<const Action<T> *>(this)->GetDeepestActionWeSuspended()); }
	Action<T> *GetDeepestActionSuspendedUs() { return const_cast<Action<T> *>(const_cast<const Action<T> *>(this)->GetDeepestActionSuspendedUs()); }
	
	Behavior<T> *m_Behavior        = nullptr;
	Action<T> *m_ActionParent      = nullptr;
	Action<T> *m_ActionChild       = nullptr;
	Action<T> *m_ActionWeSuspended = nullptr;
	Action<T> *m_ActionSuspendedUs = nullptr;
	T *m_Actor                     = nullptr;
	EventDesiredResult<T> m_Result;
	bool m_bStarted   = false;
	bool m_bSuspended = false;
};


template<typename T> inline Behavior<T>::~Behavior()
{
	if (this->m_MainAction != nullptr) {
		if (this->m_Actor != nullptr) {
			this->m_MainAction->InvokeOnEnd(this->m_Actor, this, nullptr);
			this->m_Actor = nullptr;
		}
		
		delete this->m_MainAction->GetDeepestActionWeSuspended();
	}
}


template<typename T> inline void Behavior<T>::Update(T *actor, float dt)
{
	if (actor == nullptr || this->m_MainAction == nullptr) return;
	
	this->m_Actor = actor;
	
	ActionResult<T> result = this->m_MainAction->InvokeUpdate(actor, this, dt);
	this->m_MainAction = this->m_MainAction->ApplyResult(actor, this, result);
	
	if (this->m_MainAction != nullptr && actor->IsDebugging(INextBot::DEBUG_BEHAVIOR)) {
		actor->DisplayDebugText(CFmtStr("%s: %s", this->m_strName.Get(), this->m_MainAction->DebugString()));
	}
	
	this->m_DestroyedActions.PurgeAndDeleteElements();
}


template<typename T> inline Action<T>::~Action()
{
	if (this->m_ActionParent != nullptr && this->m_ActionParent->m_ActionChild == this) {
		this->m_ActionParent->m_ActionChild = this->m_ActionWeSuspended;
	}
	
	if (this->m_ActionChild != nullptr) {
		Action<T> *suspended = this->m_ActionChild;
		do {
			Action<T> *next = suspended->m_ActionWeSuspended;
			delete suspended;
			suspended = next;
		} while (suspended != nullptr);
	}
	
	if (this->m_ActionWeSuspended != nullptr) {
		this->m_ActionWeSuspended->m_ActionSuspendedUs = nullptr;
	}
	
	if (this->m_ActionSuspendedUs != nullptr) {
		delete this->m_ActionSuspendedUs;
	}
	
	if (this->m_Result.action != nullptr) {
		delete this->m_Result.action;
	}
}


template<typename T> inline const char *Action<T>::GetFullName() const
{
	static char str[256];
	str[0] = '\0';
	
	const char *names[64];
	int count = 0;
	
	const Action<T> *action = this;
	while (action != nullptr && count < 64) {
		names[count++] = action->GetName();
		action = action->m_ActionParent;
	}
	
	for (int i = count - 1; i > 0; --i) {
		V_strcat_safe(str, names[i]);
		V_strcat_safe(str, "/");
	}
	V_strcat_safe(str, names[0]);
	
	return str;
}


template<typename T> inline Action<T> *Action<T>::ApplyResult(T *actor, Behavior<T> *behavior, ActionResult<T> result)
{
	if (result.transition == ACT_T_CHANGE_TO) {
		if (result.action == nullptr) {
			DevMsg("Error: Attempted CHANGE_TO to a NULL Action\n");
			return this;
		}
		
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
			
			if (result.action == this) {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_RED,   "START ");
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE, this->GetName());
			} else {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE, this->GetName());
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_RED,   " CHANGE_TO ");
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE, result.action->GetName());
			}
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTGREEN_96, "  (%s)\n", result.reason);
			} else {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,      "\n");
			}
		}
		
		this->InvokeOnEnd(actor, behavior, result.action);
		ActionResult<T> result2 = result.action->InvokeOnStart(actor, behavior, this, this->m_ActionWeSuspended);
		
		if (result.action != this) {
			behavior->DestroyAction(this);
		}
		
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR)) {
			result.action->PrintStateToConsole();
		}
		
		return result.action->ApplyResult(actor, behavior, result2);
	} else if (result.transition == ACT_T_SUSPEND_FOR) {
		Action<T> *action = this->GetDeepestActionSuspendedUs();
		
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_MAGENTA,     " caused ");
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       action->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_MAGENTA,     " to SUSPEND_FOR ");
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       result.action->GetName());
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTGREEN_96, "  (%s)\n", result.reason);
			} else {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,      "\n");
			}
		}
		
		Action<T> *next = action->InvokeOnSuspend(actor, behavior, result.action);
		ActionResult<T> result2 = result.action->InvokeOnStart(actor, behavior, next, next);
		
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR)) {
			result.action->PrintStateToConsole();
		}
		
		return result.action->ApplyResult(actor, behavior, result2);
	} else if (result.transition == ACT_T_DONE) {
		this->InvokeOnEnd(actor, behavior, this->m_ActionWeSuspended);
		
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
			
			if (this->m_ActionWeSuspended != nullptr) {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_GREEN, " DONE, RESUME ");
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE, this->m_ActionWeSuspended->GetName());
			} else {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_GREEN, " DONE.");
			}
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTGREEN_96, "  (%s)\n", result.reason);
			} else {
				actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,      "\n");
			}
		}
		
		if (this->m_ActionWeSuspended != nullptr) {
			ActionResult<T> result2 = this->m_ActionWeSuspended->InvokeOnResume(actor, behavior, this);
			
			if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR)) {
				this->m_ActionWeSuspended->PrintStateToConsole();
			}
			
			behavior->DestroyAction(this);
			return this->m_ActionWeSuspended->ApplyResult(actor, behavior, result2);
		} else {
			behavior->DestroyAction(this);
			return nullptr;
		}
	} else {
		/* ACT_T_CONTINUE */
		return this;
	}
}


template<typename T> inline ActionResult<T> Action<T>::InvokeOnStart(T *actor, Behavior<T> *behavior, Action<T> *action1, Action<T> *action2)
{
	// if due to SUSPEND_FOR:
	// action1: action which was suspended for us
	// action2: action which was suspended for us
	
	// if due to SUSPEND_FOR or DONE:
	// action1: the suspended action's previously suspended action, if any
	// action2: the suspended action's previously suspended action, if any
	
	// if due to CHANGE_TO:
	// action1: action which ended for us
	// action2: the ended action's previously suspended action, if any
	
	
	if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_GREEN,       " STARTING ");
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       "\n");
	}
	
	this->m_bStarted = true;
	this->m_Actor    = actor;
	this->m_Behavior = behavior;
	
	Action<T> *parent;
	if (action1 != nullptr) {
		parent = action1->m_ActionParent;
		this->m_ActionParent = parent;
	} else {
		parent = this->m_ActionParent;
	}
	
	if (parent != nullptr) {
		parent->m_ActionChild = this;
	}
	
	this->m_ActionWeSuspended = action2;
	if (action2 != nullptr) {
		action2->m_ActionSuspendedUs = this;
	}
	
	this->m_ActionSuspendedUs = nullptr;
	
	Action<T> *child = this->InitialContainedAction(actor);
	this->m_ActionChild = child;
	if (this->m_ActionChild != nullptr) {
		child->m_ActionParent = this;
		this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior, ActionResult<T>::ChangeTo(this, this->m_ActionChild, "Starting child Action"));
	}
	
	return this->OnStart(actor, action1);
}

template<typename T> inline ActionResult<T> Action<T>::InvokeUpdate(T *actor, Behavior<T> *behavior, float dt)
{
	// dt: behavior->m_flTickInterval
	
	
	Action<T> *suspended = this;
	while ((suspended = suspended->m_ActionWeSuspended) != nullptr) {
		if (suspended->m_Result.transition == ACT_T_CHANGE_TO || suspended->m_Result.transition == ACT_T_DONE) {
			return ActionResult<T>::Done(this, "Out of scope");
		}
	}
	
	if (this->m_bStarted) {
		if (this->m_Result.transition == ACT_T_CONTINUE || this->m_Result.transition == ACT_T_SUSTAIN) {
			suspended = this;
			while (suspended != nullptr) {
				if (suspended->m_Result.transition == ACT_T_SUSPEND_FOR) {
					ActionResult<T> result = suspended->m_Result;
					suspended->m_Result.Reset();
					return result;
				}
				
				suspended = suspended->m_ActionWeSuspended;
			}
			
			if (this->m_ActionChild != nullptr) {
				ActionResult<T> result = this->m_ActionChild->InvokeUpdate(actor, behavior, dt);
				this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior, result);
			}
			// HACK so we can obfuscate VPROF funcs
			VPROF_BUDGET("::InvokeUpdate", "NextBot");
			return this->Update(actor, dt);
		} else {
			ActionResult<T> result = this->m_Result;
			this->m_Result.Reset();
			return result;
		}
	} else {
		return ActionResult<T>::ChangeTo(this, this, "Starting Action");
	}
}

template<typename T> inline void Action<T>::InvokeOnEnd(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// if due to DONE:
	// action: the action we previously suspended, if any
	
	// if due to CHANGE_TO:
	// action: the action we're changing to
	
	// TF2C This often crashed when bots got removed.
	if ( actor == nullptr || behavior == nullptr || action == nullptr )
		return;
	
	if (this->m_bStarted) {
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_MAGENTA,     " ENDING ");
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
			actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       "\n");
		}
		
		this->m_bStarted = false;
		
		if (this->m_ActionChild != nullptr) {
			Action<T> *suspended = this->m_ActionChild;
			do {
				suspended->InvokeOnEnd(actor, behavior, action);
				suspended = suspended->m_ActionWeSuspended;
			} while (suspended != nullptr);
		}
		
		this->OnEnd(actor, action);
		
		if (this->m_ActionSuspendedUs != nullptr) {
			this->m_ActionSuspendedUs->InvokeOnEnd(actor, behavior, action);
		}
	}
}


template<typename T> inline ActionResult<T> Action<T>::InvokeOnResume(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// if due to DONE:
	// action: the action that previously suspended us, who just ended
	
	
	if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_MAGENTA,     " RESUMING ");
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       "\n");
	}
	
	if (this->m_bSuspended && (this->m_Result.transition == ACT_T_CONTINUE || this->m_Result.transition == ACT_T_SUSTAIN)) {
		this->m_bSuspended = false;
		this->m_ActionSuspendedUs = nullptr;
		
		if (this->m_ActionParent != nullptr) {
			this->m_ActionParent->m_ActionChild = this;
		}
		
		if (this->m_ActionChild != nullptr) {
			ActionResult<T> result = this->m_ActionChild->InvokeOnResume(actor, behavior, action);
			this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior, result);
		}
		
		return this->OnResume(actor, action);
	} else {
		return ActionResult<T>::Continue(this);
	}
}

template<typename T> inline Action<T> *Action<T>::InvokeOnSuspend(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// if due to SUSPEND_FOR:
	// action: the action we're suspending for
	
	
	if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_LTYELLOW_64, "%3.2f: %s:%s: ", gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_MAGENTA,     " SUSPENDING ");
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       this->GetName());
		actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,       "\n");
	}
	
	if (this->m_ActionChild != nullptr) {
		this->m_ActionChild = this->m_ActionChild->InvokeOnSuspend(actor, behavior, action);
	}
	
	this->m_bSuspended = true;
	
	ActionResult<T> result = this->OnSuspend(actor, action);
	if (result.transition == ACT_T_DONE) {
		this->InvokeOnEnd(actor, behavior, nullptr);
		behavior->DestroyAction(this);
		return this->m_ActionWeSuspended;
	} else {
		return this;
	}
}


template<typename T> template<size_t LEN> inline const char *Action<T>::BuildDecoratedName(char (&buf)[LEN], const Action<T> *action) const
{
	V_strcat_safe(buf, action->GetName());
	
	if (action->m_ActionChild != nullptr) {
		V_strcat_safe(buf, "( ");
		this->BuildDecoratedName(buf, action->m_ActionChild);
		V_strcat_safe(buf, " )");
	}
	
	if (action->m_ActionWeSuspended != nullptr) {
		V_strcat_safe(buf, "<<");
		this->BuildDecoratedName(buf, action->m_ActionWeSuspended);
	}
	
	return buf;
}

template<typename T> inline const char *Action<T>::DebugString() const
{
	static char str[256];
	str[0] = '\0';
	
	return this->BuildDecoratedName(str, this->GetDeepestActionParent());
}

template<typename T> inline void Action<T>::PrintStateToConsole() const
{
	ConColorMsg(NB_COLOR_WHITE, "%s\n", this->DebugString());
}


template<typename T> template<typename Functor> inline void Action<T>::HandleEvent(Action<T> *action, const char *name, Functor&& handler)
{
	if (!action->m_bStarted) return;
	
	EventDesiredResult<T> result;
	
	while (true) {
		if (action->m_Actor != nullptr) {
			if (action->m_Actor->IsDebugging(INextBot::DEBUG_EVENTS) || NextBotDebugHistory.GetBool()) {
				action->m_Actor->DebugConColorMsg(INextBot::DEBUG_EVENTS, NB_COLOR_GRAY, "%3.2f: %s:%s: %s received EVENT %s\n", gpGlobals->curtime, action->m_Actor->GetDebugIdentifier(), action->m_Behavior->GetName(), action->GetFullName(), name);
			}
		}
		
		result = handler(action, action->m_Actor);
		
		if (result.transition != ACT_T_CONTINUE) {
			break;
		}
		
		action = action->m_ActionWeSuspended;
		if (action == nullptr) return;
	}
	
	if (action->m_Actor != nullptr && (action->m_Actor->IsDebugging(INextBot::DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool())) {
		if (result.transition == ACT_T_CHANGE_TO || result.transition == ACT_T_SUSPEND_FOR || result.transition == ACT_T_DONE) {
			action->m_Actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_YELLOW, "%3.2f: %s:%s: ", gpGlobals->curtime, action->m_Actor->GetDebugIdentifier(), action->m_Behavior->GetName());
			action->m_Actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_WHITE,  "%s ", action->GetFullName());
			action->m_Actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_YELLOW, "responded to EVENT %s with ", name);
			action->m_Actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_RED,    "%s %s ", result.GetString_Transition(), result.GetString_Action());
			action->m_Actor->DebugConColorMsg(INextBot::DEBUG_BEHAVIOR, NB_COLOR_GREEN,  "%s\n", result.GetString_Reason());
		}
	}
	
	action->StorePendingEventResult(result, name);
}


template<typename T> inline void Action<T>::StorePendingEventResult(const EventDesiredResult<T>& result, const char *event)
{
	if (result.severity < this->m_Result.severity) {
		if (result.action != nullptr) {
			delete result.action;
		}
	} else {
		if (this->m_Result.severity == SEV_CRITICAL && developer.GetInt() != 0) {
			DevMsg("%3.2f: WARNING: %s::%s() RESULT_CRITICAL collision\n", gpGlobals->curtime, this->GetName(), event);
		}
		
		if (this->m_Result.action != nullptr) {
			delete this->m_Result.action;
		}
		
		this->m_Result = result;
	}
}


template<typename T> inline const Action<T> *Action<T>::GetDeepestActionParent() const
{
	const Action<T> *action = this;
	while (action->m_ActionParent != nullptr) {
		action = action->m_ActionParent;
	}
	return action;
}

template<typename T> inline const Action<T> *Action<T>::GetDeepestActionChild() const
{
	const Action<T> *action = this;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	return action;
}

template<typename T> inline const Action<T> *Action<T>::GetDeepestActionWeSuspended() const
{
	const Action<T> *action = this;
	while (action->m_ActionWeSuspended != nullptr) {
		action = action->m_ActionWeSuspended;
	}
	return action;
}

template<typename T> inline const Action<T> *Action<T>::GetDeepestActionSuspendedUs() const
{
	const Action<T> *action = this;
	while (action->m_ActionSuspendedUs != nullptr) {
		action = action->m_ActionSuspendedUs;
	}
	return action;
}


template<typename T> inline ActionResult<T> ActionResult<T>::Continue(Action<T> *current)
{
	return ActionResult<T>(ACT_T_CONTINUE, nullptr, nullptr);
}
template<typename T> inline ActionResult<T> ActionResult<T>::ChangeTo(Action<T> *current, Action<T> *next, const char *why)
{
	return ActionResult<T>(ACT_T_CHANGE_TO, next, why);
}
template<typename T> inline ActionResult<T> ActionResult<T>::SuspendFor(Action<T> *current, Action<T> *next, const char *why)
{
	current->ClearPendingEventResult();
	return ActionResult<T>(ACT_T_SUSPEND_FOR, next, why);
}
template<typename T> inline ActionResult<T> ActionResult<T>::Done(Action<T> *current, const char *why)
{
	return ActionResult<T>(ACT_T_DONE, nullptr, why);
}
template<typename T> inline ActionResult<T> ActionResult<T>::Sustain(Action<T> *current, const char *why)
{
	return ActionResult<T>(ACT_T_SUSTAIN, nullptr, why);
}


template<typename T> inline EventDesiredResult<T> EventDesiredResult<T>::Continue(Action<T> *current, EventResultSeverity level)
{
	return EventDesiredResult<T>(ACT_T_CONTINUE, nullptr, nullptr, level);
}
template<typename T> inline EventDesiredResult<T> EventDesiredResult<T>::ChangeTo(Action<T> *current, Action<T> *next, const char *why, EventResultSeverity level)
{
	return EventDesiredResult<T>(ACT_T_CHANGE_TO, next, why, level);
}
template<typename T> inline EventDesiredResult<T> EventDesiredResult<T>::SuspendFor(Action<T> *current, Action<T> *next, const char *why, EventResultSeverity level)
{
	current->ClearPendingEventResult();
	return EventDesiredResult<T>(ACT_T_SUSPEND_FOR, next, why, level);
}
template<typename T> inline EventDesiredResult<T> EventDesiredResult<T>::Done(Action<T> *current, const char *why, EventResultSeverity level)
{
	return EventDesiredResult<T>(ACT_T_DONE, nullptr, why, level);
}
template<typename T> inline EventDesiredResult<T> EventDesiredResult<T>::Sustain(Action<T> *current, const char *why, EventResultSeverity level)
{
	return EventDesiredResult<T>(ACT_T_SUSTAIN, nullptr, why, level);
}


#include <type_traits>
#define   Continue(...) return EventDesiredResult<typename std::remove_pointer<decltype(actor)>::type>::Continue  (this, ##__VA_ARGS__)
#define   ChangeTo(...) return EventDesiredResult<typename std::remove_pointer<decltype(actor)>::type>::ChangeTo  (this, ##__VA_ARGS__)
#define SuspendFor(...) return EventDesiredResult<typename std::remove_pointer<decltype(actor)>::type>::SuspendFor(this, ##__VA_ARGS__)
#define       Done(...) return EventDesiredResult<typename std::remove_pointer<decltype(actor)>::type>::Done      (this, ##__VA_ARGS__)
#define    Sustain(...) return EventDesiredResult<typename std::remove_pointer<decltype(actor)>::type>::Sustain   (this, ##__VA_ARGS__)


#endif
