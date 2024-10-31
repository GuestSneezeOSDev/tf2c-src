/* NextBotManager
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTMANAGER_H
#define NEXTBOT_NEXTBOTMANAGER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotInterface.h"


class NextBotManager
{
public:
	friend NextBotManager& TheNextBots();
	
	struct DebugFilter
	{
		int entindex;
		char str[0x80];
	};
	
	NextBotManager() {}
	virtual ~NextBotManager() {}
	
	virtual void Update();
	virtual void OnMapLoaded()        { this->Reset(); }
	virtual void OnRoundRestart()     { this->Reset(); }
	virtual void OnBeginChangeLevel() {}
	
	virtual void OnKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info);
	virtual void OnSound(CBaseEntity *ent, const Vector& where, KeyValues *kv);
	virtual void OnSpokeConcept(CBaseCombatCharacter *who, const char *aiconcept, AI_Response *response);
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon);
	
	void CollectAllBots(CUtlVector<INextBot *> *nextbots);
	
	INextBot *GetBotUnderCrosshair(CBasePlayer *player);
	bool IsDebugFilterMatch(const INextBot *nextbot) const;
	
	void DebugFilterAdd(const char *str);
	void DebugFilterAdd(int entindex);
	void DebugFilterClear();
	void DebugFilterRemove(const char *str);
	void DebugFilterRemove(int entindex);
	
	bool ShouldUpdate(INextBot *nextbot);
	void NotifyBeginUpdate(INextBot *nextbot);
	void NotifyEndUpdate(INextBot *nextbot);
	
	int Register(INextBot *nextbot);
	void UnRegister(INextBot *nextbot);
	
	void NotifyPathDestruction(const PathFollower *follower);
	
	template<typename Functor> bool ForEachBot(Functor&& func);
	template<typename Functor> bool ForEachCombatCharacter(Functor&& func);
	
	INextBot *GetSelectedBot() const       { return this->m_SelectedBot; }
	void SetSelectedBot(INextBot *nextbot) { this->m_SelectedBot = nextbot; }
	
	bool IsDebugging(unsigned int type) const { return (this->m_nDebugMask & type) != INextBot::DEBUG_NONE; }
	void SetDebugMask(unsigned int mask)      { this->m_nDebugMask = mask; }
	
	int GetBotCount() const { return this->m_NextBots.Count(); }
	
protected:
	static NextBotManager *sInstance;
	
private:
	void Reset();
	
	bool IsBotDead(CBaseCombatCharacter *bot) const;
	
	CUtlLinkedList<INextBot *> m_NextBots;
	int m_iTickRate = 0;
	double m_flUpdateBegin;
	double m_flUpdateFrame;
	unsigned int m_nDebugMask = INextBot::DEBUG_NONE;
	CUtlVector<DebugFilter> m_DebugFilters;
	INextBot *m_SelectedBot = nullptr;
};


template<typename Functor> inline bool NextBotManager::ForEachBot(Functor&& func)
{
	for (auto nextbot : this->m_NextBots) {
		if (!func(nextbot)) return false;
	}
	
	return true;
}

template<typename Functor> inline bool NextBotManager::ForEachCombatCharacter(Functor&& func)
{
	for (auto nextbot : this->m_NextBots) {
		if (!func(nextbot->GetEntity())) return false;
	}
	
	return true;
}


NextBotManager& TheNextBots();


#endif
