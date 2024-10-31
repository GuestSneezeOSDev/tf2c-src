/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_SQUAD_H
#define TF_BOT_SQUAD_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotEventResponderInterface.h"


class CTFBot;


class CTFBotSquad final : public INextBotEventResponder
{
public:
	class Iterator
	{
	public:
		friend class CTFBotSquad;
		
		CTFBot *operator*() const { return this->m_pBot; }
		
	private:
		Iterator() = default;
		Iterator(CTFBot *bot, int idx) : m_pBot(bot), m_iIndex(idx) {}
		
		CTFBot *m_pBot = nullptr;
		int m_iIndex = -1;
	};
	
	CTFBotSquad() {}
	virtual ~CTFBotSquad() {}
	
	virtual INextBotEventResponder *FirstContainedResponder() const override;
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override;
	
	int GetMemberCount() const;
	void CollectMembers(CUtlVector<CTFBot *> *members) const;
	
	Iterator GetFirstMember() const;
	Iterator GetNextMember(const Iterator& prev) const;
	
	CTFBot *GetLeader() const { return this->m_hLeader; }
	
	bool IsInFormation() const;
	float GetMaxSquadFormationError() const;
	bool ShouldSquadLeaderWaitForFormation() const;
	
	float GetSlowestMemberIdealSpeed(bool include_leader) const;
	float GetSlowestMemberSpeed(bool include_leader) const;
	
	void Join(CTFBot *bot);
	void Leave(CTFBot *bot);
	
	void DisbandAndDeleteSquad();
	
private:
	CUtlVector<CHandle<CTFBot>> m_Members; // +0x0004
	CHandle<CTFBot> m_hLeader;             // +0x0018
	float m_flFormationSize = -1.0f;       // +0x001c
	bool m_bShouldPreserveSquad = false;   // +0x0020
};
// TODO: remove offsets


#endif
