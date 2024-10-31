/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_VISION_H
#define TF_BOT_VISION_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotVisionInterface.h"


class CTFBot;


class CTFBotVision final : public IVision
{
public:
	CTFBotVision(INextBot *nextbot);
	virtual ~CTFBotVision() {}
	
	virtual void Update() override;
	
	virtual void CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents) override;
	virtual float GetMaxVisionRange() const override;
	virtual float GetMinRecognizeTime() const override;
	virtual bool IsIgnored(CBaseEntity *ent) const override;
	virtual bool IsVisibleEntityNoticed(CBaseEntity *ent) const override;
	
private:
	void UpdatePotentiallyVisibleNPCVector();
	
	CTFBot *GetTFBot() const { return this->m_pTFBot; }
	CTFBot *m_pTFBot;
	
	CUtlVector<CHandle<CBaseCombatCharacter>> m_PVNPCs;
	CountdownTimer m_ctUpdatePVNPCs;
	CountdownTimer m_ctUpdate;
};


#endif
