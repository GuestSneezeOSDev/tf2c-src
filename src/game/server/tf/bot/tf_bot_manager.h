/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_MANAGER_H
#define TF_BOT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotManager.h"


class CTFPlayer;
class CTFBot;


struct CStuckBotEvent
{
	Vector position;  // +0x00
	float duration;   // +0x0c
	Vector path_goal; // +0x10
	bool has_goal;    // +0x1c
};


struct CStuckBot
{
	char name[0x100];
	int entindex;
	CUtlVectorAutoPurge<CStuckBotEvent *> events;
};


class CTFBotManager final : public NextBotManager
{
public:
	CTFBotManager()          { NextBotManager::sInstance = this; }
	virtual ~CTFBotManager() { NextBotManager::sInstance = nullptr; }
	
	/* NextBotManager overrides */
	virtual void Update() override;
	virtual void OnMapLoaded() override;
	virtual void OnRoundRestart() override;
	
	void LevelShutdown();
	
	void ClearStuckBotData();
	void DrawStuckBotData(float duration);
	const CStuckBot *FindOrCreateStuckBot(int entindex, const char *name);
	
	bool IsInOfflinePractice() const;
	void SetIsInOfflinePractice(bool val);
	
	CTFBot *GetAvailableBotFromPool();
	bool RemoveBotFromTeamAndKick(int teamnum);
	
	void OnForceAddedBots(int count);
	void OnForceKickedBots(int count);
	
	bool IsAllBotTeam(int teamnum);
	bool IsMeleeOnly() const;
	bool IsRandomizer() const;
	
private:
	void MaintainBotQuota();
	void RevertOfflinePracticeConvars();
	
	void CollectKickableBots(CUtlVector<CTFBot *> *bots, int teamnum);
	CTFBot *SelectBotToKick(CUtlVector<CTFBot *> *bots);
	
	float m_flQuotaTime = 0.0f; // +0x50
	// 54 byte, seems unused
	// 58 CUtlVector<CBaseEntity *>
	// 6c CUtlVector<ArcherAssignmentInfo>
	// 80 CountdownTimer
	CUtlVectorAutoPurge<CStuckBot *> m_StuckBots; // +0x8c
	CountdownTimer m_ctDrawStuckBots;             // +0xa0
};
// TODO: remove offsets when done RE'ing this class
// TODO: convert m_flQuotaTime into an IntervalTimer perhaps?


CTFBotManager& TheTFBots();


#endif
