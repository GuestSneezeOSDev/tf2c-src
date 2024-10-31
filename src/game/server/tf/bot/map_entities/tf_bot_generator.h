/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_GENERATOR_H
#define TF_BOT_GENERATOR_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotGenerator : public CPointEntity
{
public:
	CTFBotGenerator();
	virtual ~CTFBotGenerator() {}
	
	DECLARE_CLASS(CTFBotGenerator, CPointEntity);
	DECLARE_DATADESC();
	
	virtual void Activate() override;
	
	void GeneratorThink();
	void SpawnBot();
	
	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	void InputSetSuppressFire(inputdata_t& inputdata);
	void InputSetDisableDodge(inputdata_t& inputdata);
	void InputSetDifficulty(inputdata_t& inputdata);
	void InputCommandGotoActionPoint(inputdata_t& inputdata);
	void InputSetAttentionFocus(inputdata_t& inputdata);
	void InputClearAttentionFocus(inputdata_t& inputdata);
	void InputSpawnBot(inputdata_t& inputdata);
	void InputRemoveBots(inputdata_t& inputdata);
	
	void OnSpawned(CTFBot *bot)   { this->m_onSpawned  .FireOutput(bot,  this); }
	void OnExpended()             { this->m_onExpended .FireOutput(this, this); }
	void OnBotKilled(CTFBot *bot) { this->m_onBotKilled.FireOutput(bot,  this); }
	
private:
	enum // m_iOnDeathAction
	{
		REMOVE_ON_DEATH           = 1,
		BECOME_SPECTATOR_ON_DEATH = 2,
	};
	
	bool m_bAutoClass         = false;      // +0x360
	bool m_bSuppressFire      = false;      // +0x361
	bool m_bDisableDodge      = false;      // +0x362
	bool m_bUseTeamSpawnpoint = false;      // +0x363
	bool m_bRetainBuildings   = false;      // +0x364
	bool m_bExpended          = false;      // +0x365
	int m_iOnDeathAction = REMOVE_ON_DEATH; // +0x368
	int m_spawnCount;                       // +0x36c
	int m_spawnFlags = 0;                   // +0x370
	int m_maxActiveCount;                   // +0x374
	float m_spawnInterval;                  // +0x378
	string_t m_className;                   // +0x37c
	string_t m_teamName;                    // +0x380
	string_t m_actionPointName;             // +0x384
	string_t m_initialCommand;              // +0x388
	CHandle<CBaseEntity> m_hActionPoint;    // +0x38c
	int m_difficulty               = -1;    // +0x390
	bool m_bSpawnOnlyWhenTriggered = false; // +0x394
	bool m_bEnabled                = true;  // +0x395
	COutputEvent m_onSpawned;               // +0x398
	COutputEvent m_onExpended;              // +0x3b0
	COutputEvent m_onBotKilled;             // +0x3c8
	CUtlVector<CHandle<CTFBot>> m_Bots;     // +0x3e0
};
// TODO: remove offsets from member vars


class CTFBotActionPoint : public CPointEntity
{
public:
	CTFBotActionPoint() {}
	virtual ~CTFBotActionPoint() {}
	
	DECLARE_CLASS(CTFBotActionPoint, CPointEntity);
	DECLARE_DATADESC();
	
	virtual void Activate() override;
	
	void ReachedActionPoint(CTFBot *bot);
	
	bool IsWithinRange(CBaseEntity *ent) const;
	
private:
	CHandle<CBaseEntity> m_hNextActionPoint; // +0x360
	
	float m_stayTime        = 0.0f; // +0x364
	float m_desiredDistance = 1.0f; // +0x368
	string_t m_nextActionPointName; // +0x36c
	string_t m_command;             // +0x370
	
	COutputEvent m_onReachedActionPoint; // +0x374
};
// TODO: remove offsets from member vars


#endif
