/* NextBotComponentInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTCOMPONENTINTERFACE_H
#define NEXTBOT_NEXTBOTCOMPONENTINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotEventResponderInterface.h"


class INextBotComponent : public INextBotEventResponder
{
public:
	INextBotComponent(INextBot *nextbot);
	virtual ~INextBotComponent();
	
	virtual void Reset();
	virtual void Update() = 0;
	virtual void Upkeep() {}
	virtual INextBot *GetBot() const { return this->m_NextBot; }
	
	float GetLastUpdateTime() const   { return this->m_flLastUpdate; }
	void SetLastUpdateTime(float val) { this->m_flLastUpdate = val; }
	
	float GetUpdateInterval() const   { return this->m_flTickInterval; }
	void SetUpdateInterval(float val) { this->m_flTickInterval = val; }
	
	INextBotComponent *GetNextComponent() const    { return this->m_NextComponent; }
	void SetNextComponent(INextBotComponent *next) { this->m_NextComponent = next; }

	bool ComputeUpdateInterval();

	virtual ScriptClassDesc_t *GetScriptDesc();
	HSCRIPT GetScriptInstance();
	
private:
	float m_flLastUpdate;
	float m_flTickInterval;
	INextBot *m_NextBot;
	INextBotComponent *m_NextComponent;
	HSCRIPT m_hScriptInstance;
};


#endif
