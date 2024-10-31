/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_PROXY_H
#define TF_BOT_PROXY_H
#ifdef _WIN32
#pragma once
#endif


class CTFBot;
class CTFBotActionPoint;


class CTFBotProxy : public CPointEntity
{
public:
	CTFBotProxy();
	virtual ~CTFBotProxy() {}
	
	DECLARE_CLASS(CTFBotProxy, CPointEntity);
	DECLARE_DATADESC();
	
	void InputSetTeam(inputdata_t& inputdata);
	void InputSetClass(inputdata_t& inputdata);
	void InputSetMovementGoal(inputdata_t& inputdata);
	void InputSpawn(inputdata_t& inputdata);
	void InputDelete(inputdata_t& inputdata);
	
	void OnSpawned()        { this->m_onSpawned       .FireOutput(this, this); }
	void OnInjured()        { this->m_onInjured       .FireOutput(this, this); } // TODO: find caller(s)
	void OnKilled()         { this->m_onKilled        .FireOutput(this, this); }
	void OnAttackingEnemy() { this->m_onAttackingEnemy.FireOutput(this, this); } // TODO: find caller(s)
	void OnKilledEnemy()    { this->m_onKilledEnemy   .FireOutput(this, this); } // TODO: find caller(s)
	
private:
	COutputEvent m_onSpawned;        // +0x360
	COutputEvent m_onInjured;        // +0x378
	COutputEvent m_onKilled;         // +0x390
	COutputEvent m_onAttackingEnemy; // +0x3a8
	COutputEvent m_onKilledEnemy;    // +0x3c0
	
	char m_botName  [0x40];     // +0x3d8
	char m_className[0x40];     // +0x418
	char m_teamName [0x40];     // +0x458
	int m_spawnOnStart;         // +0x498 TODO: unused???
	string_t m_actionPointName; // +0x49c TODO: unused???
	float m_respawnInterval;    // +0x4a0 TODO: unused???
	
	CHandle<CTFBot> m_hBot;                    // +0x4a4
	CHandle<CTFBotActionPoint> m_hActionPoint; // +0x4a8
};
// TODO: remove offsets from member vars


#endif
