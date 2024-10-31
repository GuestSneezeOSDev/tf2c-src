/* NextBotVisionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTVISIONINTERFACE_H
#define NEXTBOT_NEXTBOTVISIONINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotKnownEntity.h"
#include "NextBotComponentInterface.h"


class INextBotEntityFilter;


class IVision : public INextBotComponent
{
public:
	class IForEachKnownEntity
	{
	public:
		virtual bool Inspect(const CKnownEntity& known) = 0;
	};
	
	enum FieldOfViewCheckType
	{
		USE_FOV       = 0,
		DISREGARD_FOV = 1,
	};
	
	IVision(INextBot *nextbot) :
		INextBotComponent(nextbot) { this->Reset(); }
	virtual ~IVision() {}
	
	virtual void Reset() override;
	virtual void Update() override;
	
	// TODO: parameter names
	virtual bool ForEachKnownEntity(IForEachKnownEntity& functor);
	virtual void CollectKnownEntities(CUtlVector<CKnownEntity> *knowns);
	virtual const CKnownEntity *GetPrimaryKnownThreat(bool only_recently_visible = false) const;
	virtual float GetTimeSinceVisible(int teamnum) const;
	virtual const CKnownEntity *GetClosestKnown(int teamnum) const;
	virtual int GetKnownCount(int teamnum, bool only_recently_visible, float range) const;
	virtual const CKnownEntity *GetClosestKnown(INextBotEntityFilter&& filter) const;
	virtual const CKnownEntity *GetKnown(const CBaseEntity *ent) const;
	virtual void AddKnownEntity(CBaseEntity *ent);
	virtual void ForgetEntity(CBaseEntity *ent);
	virtual void ForgetAllKnownEntities();
	virtual void CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents);
	virtual float GetMaxVisionRange() const   { return 2000.0f; }
	virtual float GetMinRecognizeTime() const { return 0.0f; }
	virtual bool IsAbleToSee(CBaseEntity *ent, FieldOfViewCheckType ctype, Vector *v1) const;
	virtual bool IsAbleToSee(const Vector& vec, FieldOfViewCheckType ctype) const;
	virtual bool IsIgnored(CBaseEntity *ent) const              { return false; }
	virtual bool IsVisibleEntityNoticed(CBaseEntity *ent) const { return true; }
	virtual bool IsInFieldOfView(const Vector& vec) const;
	virtual bool IsInFieldOfView(CBaseEntity *ent) const;
	virtual float GetDefaultFieldOfView() const { return 90.0f; }
	virtual float GetFieldOfView() const        { return this->m_flFOV; }
	virtual void SetFieldOfView(float fov);
	virtual bool IsLineOfSightClear(const Vector& vec) const;
	virtual bool IsLineOfSightClearToEntity(const CBaseEntity *ent, Vector *endpos = nullptr) const;
	virtual bool IsLookingAt(const Vector& vec, float cos_half_fov) const;
	virtual bool IsLookingAt(const CBaseCombatCharacter *who, float cos_half_fov) const;
	
private:
	void UpdateKnownEntities();
	
//	CountdownTimer m_ctUnused;
	float m_flFOV;
	float m_flCosHalfFOV;
	CUtlVector<CKnownEntity> m_KnownEntities;
	mutable CHandle<CBaseEntity> m_hPrimaryThreat;
	float m_flLastUpdate;
	IntervalTimer m_itTeamVisible[MAX_TEAMS];
};


#endif
