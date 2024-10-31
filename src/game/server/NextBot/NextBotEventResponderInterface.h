/* NextBotEventResponderInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTEVENTRESPONDERINTERFACE_H
#define NEXTBOT_NEXTBOTEVENTRESPONDERINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#define FOR_EACH_RESPONDER(func, ...) \
	for (auto responder = this->FirstContainedResponder(); responder != nullptr; responder = this->NextContainedResponder(responder)) { \
		responder->func(__VA_ARGS__); \
	}


class Path;


class INextBotEventResponder
{
public:
	enum MoveToFailureType
	{
		FAIL_INVALID_PATH = 0,
		FAIL_STUCK        = 1,
		FAIL_FELL_OFF     = 2,
	};
	
	INextBotEventResponder() {}
	virtual ~INextBotEventResponder() {}
	
	virtual INextBotEventResponder *FirstContainedResponder() const                            { return nullptr; }
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const { return nullptr; }
	
	virtual void OnLeaveGround(CBaseEntity *ent)                                                       { FOR_EACH_RESPONDER(OnLeaveGround, ent); }
	virtual void OnLandOnGround(CBaseEntity *ent)                                                      { FOR_EACH_RESPONDER(OnLandOnGround, ent); }
	
	virtual void OnContact(CBaseEntity *ent, CGameTrace *trace)                                        { FOR_EACH_RESPONDER(OnContact, ent, trace); }
	
	virtual void OnMoveToSuccess(const Path *path)                                                     { FOR_EACH_RESPONDER(OnMoveToSuccess, path); }
	virtual void OnMoveToFailure(const Path *path, MoveToFailureType fail)                             { FOR_EACH_RESPONDER(OnMoveToFailure, path, fail); }
	
	virtual void OnStuck()                                                                             { FOR_EACH_RESPONDER(OnStuck); }
	virtual void OnUnStuck()                                                                           { FOR_EACH_RESPONDER(OnUnStuck); }
	
	virtual void OnPostureChanged()                                                                    { FOR_EACH_RESPONDER(OnPostureChanged); }
	virtual void OnAnimationActivityComplete(int i1)                                                   { FOR_EACH_RESPONDER(OnAnimationActivityComplete, i1); }
	virtual void OnAnimationActivityInterrupted(int i1)                                                { FOR_EACH_RESPONDER(OnAnimationActivityInterrupted, i1); }
	virtual void OnAnimationEvent(animevent_t *a1)                                                     { FOR_EACH_RESPONDER(OnAnimationEvent, a1); }
	
	virtual void OnIgnite()                                                                            { FOR_EACH_RESPONDER(OnIgnite); }
	virtual void OnInjured(const CTakeDamageInfo& info)                                                { FOR_EACH_RESPONDER(OnInjured, info); }
	virtual void OnKilled(const CTakeDamageInfo& info)                                                 { FOR_EACH_RESPONDER(OnKilled, info); }
	virtual void OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)                 { FOR_EACH_RESPONDER(OnOtherKilled, who, info); }
	
	virtual void OnSight(CBaseEntity *ent)                                                             { FOR_EACH_RESPONDER(OnSight, ent); }
	virtual void OnLostSight(CBaseEntity *ent)                                                         { FOR_EACH_RESPONDER(OnLostSight, ent); }
	
	virtual void OnSound(CBaseEntity *ent, const Vector& where, KeyValues *kv)                         { FOR_EACH_RESPONDER(OnSound, ent, where, kv); }
	virtual void OnSpokeConcept(CBaseCombatCharacter *who, const char *aiconcept, AI_Response *response) { FOR_EACH_RESPONDER(OnSpokeConcept, who, aiconcept, response); }
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)                   { FOR_EACH_RESPONDER(OnWeaponFired, who, weapon); }
	
	virtual void OnNavAreaChanged(CNavArea *area1, CNavArea *area2)                                    { FOR_EACH_RESPONDER(OnNavAreaChanged, area1, area2); } 
	virtual void OnModelChanged()                                                                      { FOR_EACH_RESPONDER(OnModelChanged); } 
	virtual void OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)                                 { FOR_EACH_RESPONDER(OnPickUp, ent, who); }
	virtual void OnDrop(CBaseEntity *ent)                                                              { FOR_EACH_RESPONDER(OnDrop, ent); }
	virtual void OnActorEmoted(CBaseCombatCharacter *who, int emoteconcept)                                 { FOR_EACH_RESPONDER(OnActorEmoted, who, emoteconcept); }
	
	virtual void OnCommandAttack(CBaseEntity *ent)                                                     { FOR_EACH_RESPONDER(OnCommandAttack, ent); }
	virtual void OnCommandApproach(const Vector& where, float f1)                                      { FOR_EACH_RESPONDER(OnCommandApproach, where, f1); }
	virtual void OnCommandApproach(CBaseEntity *ent)                                                   { FOR_EACH_RESPONDER(OnCommandApproach, ent); }
	virtual void OnCommandRetreat(CBaseEntity *ent, float f1)                                          { FOR_EACH_RESPONDER(OnCommandRetreat, ent, f1); }
	virtual void OnCommandPause(float f1)                                                              { FOR_EACH_RESPONDER(OnCommandPause, f1); }
	virtual void OnCommandResume()                                                                     { FOR_EACH_RESPONDER(OnCommandResume); }
	virtual void OnCommandString(const char *cmd)                                                      { FOR_EACH_RESPONDER(OnCommandString, cmd); }
	
	virtual void OnShoved(CBaseEntity *ent)                                                            { FOR_EACH_RESPONDER(OnShoved, ent); }
	virtual void OnBlinded(CBaseEntity *ent)                                                           { FOR_EACH_RESPONDER(OnBlinded, ent); }
	
	virtual void OnTerritoryContested(int idx)                                                         { FOR_EACH_RESPONDER(OnTerritoryContested, idx); }
	virtual void OnTerritoryCaptured(int idx)                                                          { FOR_EACH_RESPONDER(OnTerritoryCaptured, idx); }
	virtual void OnTerritoryLost(int idx)                                                              { FOR_EACH_RESPONDER(OnTerritoryLost, idx); }
	
	virtual void OnWin()                                                                               { FOR_EACH_RESPONDER(OnWin); }
	virtual void OnLose()                                                                              { FOR_EACH_RESPONDER(OnLose); }
};


#endif
