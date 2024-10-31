/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_LOCOMOTION_H
#define TF_BOT_LOCOMOTION_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotPlayerLocomotion.h"


class CTFBot;


class CTFBotLocomotion final : public PlayerLocomotion
{
public:
	CTFBotLocomotion(INextBot *nextbot);
	virtual ~CTFBotLocomotion() {}
	
	virtual void Update() override;
	
	virtual void Approach(const Vector& dst, float f1 = 1.0f) override;
	virtual void Jump() override;
	virtual float GetMaxJumpHeight() const override   { return   72.0f; } // TODO: find source of magic number
	virtual float GetDeathDropHeight() const override { return 1000.0f; } // TODO: find source of magic number
	virtual float GetRunSpeed() const override; // TODO: possibly remove this
	virtual bool IsAreaTraversable(const CNavArea *area) const override;
	virtual bool IsEntityTraversable(CBaseEntity *ent, TraverseWhenType when) const override;
	virtual void AdjustPosture(const Vector& dst) override {}
	
private:
	CTFBot *GetTFBot() const { return this->m_pTFBot; }
	CTFBot *m_pTFBot;
};


#endif
