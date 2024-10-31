/* NextBotKnownEntity
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTKNOWNENTITY_H
#define NEXTBOT_NEXTBOTKNOWNENTITY_H
#ifdef _WIN32
#pragma once
#endif


class CKnownEntity final
{
public:
	CKnownEntity(CBaseEntity *ent);
	~CKnownEntity() {}
	
	void Destroy();
	void UpdatePosition();
	CBaseEntity *GetEntity() const             { return this->m_hEntity; }
	const Vector& GetLastKnownPosition() const { return this->m_vecPosition; }
	bool HasLastKnownPositionBeenSeen() const  { return this->m_bLastKnownPosWasSeen; }
	void MarkLastKnownPositionAsSeen()         { this->m_bLastKnownPosWasSeen = true; }
	CNavArea *GetLastKnownArea() const         { return this->m_NavArea; }
	float GetTimeSinceLastKnown() const        { return (gpGlobals->curtime - this->m_flTimeLastKnown); }
	float GetTimeSinceBecameKnown() const      { return (gpGlobals->curtime - this->m_flTimeLastBecameKnown); }
	void UpdateVisibilityStatus(bool visible);
	bool IsVisibleInFOVNow() const             { return this->m_bVisible; }
	bool IsVisibleRecently() const;
	float GetTimeSinceBecameVisible() const    { return (gpGlobals->curtime - this->m_flTimeLastBecameVisible); }
	float GetTimeWhenBecameVisible() const     { return this->m_flTimeLastBecameVisible; }
	float GetTimeSinceLastSeen() const         { return (gpGlobals->curtime - this->m_flTimeLastVisible); }
	bool WasEverVisible() const                { return (this->m_flTimeLastVisible > 0.0f); }
	bool IsObsolete() const;
	bool operator==(const CKnownEntity& that) const;
	bool Is(CBaseEntity *ent) const;
	
private:
	CHandle<CBaseEntity> m_hEntity;
	Vector m_vecPosition;
	bool m_bLastKnownPosWasSeen = false;
	CNavArea *m_NavArea;
	float m_flTimeLastVisible       = -1.0f;
	float m_flTimeLastBecameVisible = -1.0f;
	float m_flTimeLastKnown;
	float m_flTimeLastBecameKnown;
	bool m_bVisible = false;
};


inline CKnownEntity::CKnownEntity(CBaseEntity *ent)
{
	// BUG: m_flTimeLastKnown isn't initialized; shouldn't it be initialized to gpGlobals->curtime?
	this->m_flTimeLastBecameKnown = gpGlobals->curtime;
	
	if (ent != nullptr) {
		this->m_hEntity = ent;
		this->UpdatePosition();
	}
}


inline void CKnownEntity::Destroy()
{
	this->m_hEntity  = nullptr;
	this->m_bVisible = false;
}

inline void CKnownEntity::UpdatePosition()
{
	if (this->m_hEntity != nullptr) {
		this->m_vecPosition = this->m_hEntity->GetAbsOrigin();
		
		if (this->m_hEntity->IsCombatCharacter()) {
			this->m_NavArea = ToBaseCombatCharacter(this->m_hEntity)->GetLastKnownArea();
		} else {
			this->m_NavArea = nullptr;
		}
		
		this->m_flTimeLastKnown = gpGlobals->curtime;
	}
}

inline void CKnownEntity::UpdateVisibilityStatus(bool visible)
{
	if (visible) {
		if (!this->m_bVisible) {
			this->m_flTimeLastBecameVisible = gpGlobals->curtime;
		}
		this->m_flTimeLastVisible = gpGlobals->curtime;
	}
	
	this->m_bVisible = visible;
}

inline bool CKnownEntity::IsVisibleRecently() const
{
	if (this->m_bVisible) {
		return true;
	} else {
		if (this->WasEverVisible() && this->GetTimeSinceLastSeen() < 3.0f) {
			return true;
		} else {
			return false;
		}
	}
}

inline bool CKnownEntity::IsObsolete() const
{
	if (this->GetEntity() == nullptr)          return true;
	if (!this->GetEntity()->IsAlive())         return true;
	if (this->GetTimeSinceLastKnown() > 10.0f) return true;
	
	return false;
}

inline bool CKnownEntity::operator==(const CKnownEntity& that) const
{
	if (this->GetEntity() == nullptr || that.GetEntity() == nullptr) {
		return false;
	}
	
	return (this->GetEntity() == that.GetEntity());
}

inline bool CKnownEntity::Is(CBaseEntity *ent) const
{
	if (ent == nullptr || this->GetEntity() == nullptr) {
		return false;
	}
	
	return (ent == this->GetEntity());
}


#endif
