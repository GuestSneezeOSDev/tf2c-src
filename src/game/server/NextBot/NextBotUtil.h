/* NextBotUtil
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTUTIL_H
#define NEXTBOT_NEXTBOTUTIL_H
#ifdef _WIN32
#pragma once
#endif


#define NB_RGB_WHITE        0xff, 0xff, 0xff
#define NB_RGB_GRAY         0x64, 0x64, 0x64
#define NB_RGB_RED          0xff, 0x00, 0x00
#define NB_RGB_LTRED_64     0xff, 0x64, 0x64
#define NB_RGB_LTRED_96     0xff, 0x96, 0x96
#define NB_RGB_LTRED_C8     0xff, 0xc8, 0xc8
#define NB_RGB_DKRED_64     0x64, 0x00, 0x00
#define NB_RGB_ORANGE_64    0xff, 0x64, 0x00
#define NB_RGB_ORANGE_96    0xff, 0x96, 0x00
#define NB_RGB_YELLOW       0xff, 0xff, 0x00
#define NB_RGB_LTYELLOW_64  0xff, 0xff, 0x64
#define NB_RGB_LTYELLOW_96  0xff, 0xff, 0x96
#define NB_RGB_LTYELLOW_C8  0xff, 0xff, 0xc8
#define NB_RGB_DKYELLOW_64  0x64, 0x64, 0x00
#define NB_RGB_LIME         0x64, 0xff, 0x00
#define NB_RGB_GREEN        0x00, 0xff, 0x00
#define NB_RGB_LTGREEN_64   0x64, 0xff, 0x64
#define NB_RGB_LTGREEN_96   0x96, 0xff, 0x96
#define NB_RGB_LTGREEN_C8   0xc8, 0xff, 0xc8
#define NB_RGB_DKGREEN_64   0x00, 0x64, 0x00
#define NB_RGB_DKGREEN_C8   0x00, 0xc8, 0x00
#define NB_RGB_CYAN         0x00, 0xff, 0xff
#define NB_RGB_BLUE         0x00, 0x00, 0xff
#define NB_RGB_LTBLUE_64    0x64, 0x64, 0xff
#define NB_RGB_LTBLUE_96    0x96, 0x96, 0xff
#define NB_RGB_LTBLUE_C8    0xc8, 0xc8, 0xff
#define NB_RGB_DKBLUE_64    0x00, 0x00, 0x64
#define NB_RGB_MAGENTA      0xff, 0x00, 0xff
#define NB_RGB_LTMAGENTA_64 0xff, 0x64, 0xff
#define NB_RGB_DKMAGENTA_64 0x64, 0x00, 0x64
#define NB_RGB_DKMAGENTA_C8 0xc8, 0x00, 0xc8
#define NB_RGB_VIOLET       0x64, 0x00, 0xff

#define NB_RGBA_WHITE        NB_RGB_WHITE,        0xff
#define NB_RGBA_GRAY         NB_RGB_GRAY,         0xff
#define NB_RGBA_RED          NB_RGB_RED,          0xff
#define NB_RGBA_LTRED_64     NB_RGB_LTRED_64,     0xff
#define NB_RGBA_LTRED_96     NB_RGB_LTRED_96,     0xff
#define NB_RGBA_LTRED_C8     NB_RGB_LTRED_C8,     0xff
#define NB_RGBA_DKRED_64     NB_RGB_DKRED_64,     0xff
#define NB_RGBA_ORANGE_64    NB_RGB_ORANGE_64,    0xff
#define NB_RGBA_ORANGE_96    NB_RGB_ORANGE_96,    0xff
#define NB_RGBA_YELLOW       NB_RGB_YELLOW,       0xff
#define NB_RGBA_LTYELLOW_64  NB_RGB_LTYELLOW_64,  0xff
#define NB_RGBA_LTYELLOW_96  NB_RGB_LTYELLOW_96,  0xff
#define NB_RGBA_LTYELLOW_C8  NB_RGB_LTYELLOW_C8,  0xff
#define NB_RGBA_DKYELLOW_64  NB_RGB_DKYELLOW_64,  0xff
#define NB_RGBA_LIME         NB_RGB_LIME,         0xff
#define NB_RGBA_GREEN        NB_RGB_GREEN,        0xff
#define NB_RGBA_LTGREEN_64   NB_RGB_LTGREEN_64,   0xff
#define NB_RGBA_LTGREEN_96   NB_RGB_LTGREEN_96,   0xff
#define NB_RGBA_LTGREEN_C8   NB_RGB_LTGREEN_C8,   0xff
#define NB_RGBA_DKGREEN_64   NB_RGB_DKGREEN_64,   0xff
#define NB_RGBA_DKGREEN_C8   NB_RGB_DKGREEN_C8,   0xff
#define NB_RGBA_CYAN         NB_RGB_CYAN,         0xff
#define NB_RGBA_BLUE         NB_RGB_BLUE,         0xff
#define NB_RGBA_LTBLUE_64    NB_RGB_LTBLUE_64,    0xff
#define NB_RGBA_LTBLUE_96    NB_RGB_LTBLUE_96,    0xff
#define NB_RGBA_LTBLUE_C8    NB_RGB_LTBLUE_C8,    0xff
#define NB_RGBA_DKBLUE_64    NB_RGB_DKBLUE_64,    0xff
#define NB_RGBA_MAGENTA      NB_RGB_MAGENTA,      0xff
#define NB_RGBA_LTMAGENTA_64 NB_RGB_LTMAGENTA_64, 0xff
#define NB_RGBA_DKMAGENTA_64 NB_RGB_DKMAGENTA_64, 0xff
#define NB_RGBA_DKMAGENTA_C8 NB_RGB_DKMAGENTA_C8, 0xff
#define NB_RGBA_VIOLET       NB_RGB_VIOLET,       0xff

#define NB_COLOR_WHITE        Color(NB_RGBA_WHITE)
#define NB_COLOR_GRAY         Color(NB_RGBA_GRAY)
#define NB_COLOR_RED          Color(NB_RGBA_RED)
#define NB_COLOR_LTRED_64     Color(NB_RGBA_LTRED_64)
#define NB_COLOR_LTRED_96     Color(NB_RGBA_LTRED_96)
#define NB_COLOR_LTRED_C8     Color(NB_RGBA_LTRED_C8)
#define NB_COLOR_DKRED_64     Color(NB_RGBA_DKRED_64)
#define NB_COLOR_ORANGE_64    Color(NB_RGBA_ORANGE_64)
#define NB_COLOR_ORANGE_96    Color(NB_RGBA_ORANGE_96)
#define NB_COLOR_YELLOW       Color(NB_RGBA_YELLOW)
#define NB_COLOR_LTYELLOW_64  Color(NB_RGBA_LTYELLOW_64)
#define NB_COLOR_LTYELLOW_96  Color(NB_RGBA_LTYELLOW_96)
#define NB_COLOR_LTYELLOW_C8  Color(NB_RGBA_LTYELLOW_C8)
#define NB_COLOR_DKYELLOW_64  Color(NB_RGBA_DKYELLOW_64)
#define NB_COLOR_LIME         Color(NB_RGBA_LIME)
#define NB_COLOR_GREEN        Color(NB_RGBA_GREEN)
#define NB_COLOR_LTGREEN_64   Color(NB_RGBA_LTGREEN_64)
#define NB_COLOR_LTGREEN_96   Color(NB_RGBA_LTGREEN_96)
#define NB_COLOR_LTGREEN_C8   Color(NB_RGBA_LTGREEN_C8)
#define NB_COLOR_DKGREEN_64   Color(NB_RGBA_DKGREEN_64)
#define NB_COLOR_DKGREEN_C8   Color(NB_RGBA_DKGREEN_C8)
#define NB_COLOR_CYAN         Color(NB_RGBA_CYAN)
#define NB_COLOR_BLUE         Color(NB_RGBA_BLUE)
#define NB_COLOR_LTBLUE_64    Color(NB_RGBA_LTBLUE_64)
#define NB_COLOR_LTBLUE_96    Color(NB_RGBA_LTBLUE_96)
#define NB_COLOR_LTBLUE_C8    Color(NB_RGBA_LTBLUE_C8)
#define NB_COLOR_DKBLUE_64    Color(NB_RGBA_DKBLUE_64)
#define NB_COLOR_MAGENTA      Color(NB_RGBA_MAGENTA)
#define NB_COLOR_LTMAGENTA_64 Color(NB_RGBA_LTMAGENTA_64)
#define NB_COLOR_DKMAGENTA_64 Color(NB_RGBA_DKMAGENTA_64)
#define NB_COLOR_DKMAGENTA_C8 Color(NB_RGBA_DKMAGENTA_C8)
#define NB_COLOR_VIOLET       Color(NB_RGBA_VIOLET)


class INextBotEntityFilter
{
public:
	virtual bool IsAllowed(CBaseEntity *ent) const = 0;
};


class INextBotFilter
{
public:
	virtual bool IsSelected(const CBaseEntity *ent) const = 0;
};


inline bool IgnoreActorsTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
	
	/* BUG: in release builds of TF2, there's no nullptr check for the return
	 * value of EntityFromEntityHandle; for safety, we'll do it */
	if (pEntity == nullptr) return true;
	
	return !pEntity->IsCombatCharacter();
}

class NextBotTraceFilterIgnoreActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterIgnoreActors()                       : CTraceFilterSimple(nullptr,              COLLISION_GROUP_NONE, &IgnoreActorsTraceFilterFunction) {}
	NextBotTraceFilterIgnoreActors(std::nullptr_t)         : CTraceFilterSimple(nullptr,              COLLISION_GROUP_NONE, &IgnoreActorsTraceFilterFunction) {}
	NextBotTraceFilterIgnoreActors(INextBot *nextbot)      : CTraceFilterSimple(nextbot->GetEntity(), COLLISION_GROUP_NONE, &IgnoreActorsTraceFilterFunction) {}
	NextBotTraceFilterIgnoreActors(const CBaseEntity *ent) : CTraceFilterSimple(ent,                  COLLISION_GROUP_NONE, &IgnoreActorsTraceFilterFunction) {}
};


class NextBotTraceFilterOnlyActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterOnlyActors(INextBot *nextbot) :
		CTraceFilterSimple(nextbot->GetEntity(), COLLISION_GROUP_NONE) {}
	
	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask) override
	{
		if (!CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask)) {
			return false;
		}
		
		CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
		
		return (pEntity->MyNextBotPointer() != nullptr || pEntity->IsPlayer());
	}
	
	virtual TraceType_t GetTraceType() const override { return TRACE_ENTITIES_ONLY; }
};


class NextBotTraversableTraceFilter : public CTraceFilterSimple
{
public:
	NextBotTraversableTraceFilter(INextBot *nextbot, ILocomotion::TraverseWhenType when) :
		CTraceFilterSimple(nextbot->GetEntity(), COLLISION_GROUP_NONE), m_pNextBot(nextbot), m_When(when) {}
	
	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask) override
	{
		CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
		
		if (this->m_pNextBot->IsSelf(pEntity))                                 return false;
		if (!CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask)) return false;
		
		return !this->m_pNextBot->GetLocomotionInterface()->IsEntityTraversable(pEntity, this->m_When);
	}
	
private:
	INextBot *m_pNextBot;
	ILocomotion::TraverseWhenType m_When;
};


inline bool VisionTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
	if (pEntity->IsCombatCharacter()) {
		return false;
	} else {
		return pEntity->BlocksLOS();
	}
}

class NextBotVisionTraceFilter : public CTraceFilterSimple
{
public:
	NextBotVisionTraceFilter(INextBot *nextbot) :
		CTraceFilterSimple(nextbot->GetEntity(), COLLISION_GROUP_NONE, &VisionTraceFilterFunction) {}
};


/* allow calling trace functions with filter instances that are temporaries */
inline void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter&& filter, trace_t *ptr)
{
	UTIL_TraceLine(vecAbsStart, vecAbsEnd, mask, &filter, ptr);
}
inline void UTIL_TraceHull(const Vector& vecAbsStart, const Vector& vecAbsEnd, const Vector& hullMin, const Vector& hullMax, unsigned int mask, ITraceFilter&& filter, trace_t *ptr)
{
	UTIL_TraceHull(vecAbsStart, vecAbsEnd, hullMin, hullMax, mask, &filter, ptr);
}
inline void UTIL_TraceEntity(CBaseEntity *pEntity, const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter&& filter, trace_t *ptr)
{
	UTIL_TraceEntity(pEntity, vecAbsStart, vecAbsEnd, mask, &filter, ptr);
}


/* make a new 3D vector out of a 2D vector */
inline Vector Vector2DTo3D(const Vector2D& vec2D) { return Vector(vec2D.x, vec2D.y, 0.0f); }

/* get just the 2D part of a 3D vector, in 3D vector form */
inline Vector VectorXY(const Vector& vec3D) { return Vector(vec3D.x, vec3D.y, 0.0f); }


/* get a version of a vector with just the X- or Y- or Z-component modified */
inline Vector VecPlusX(const Vector& vec, float dx) { return vec + Vector(  dx, 0.0f, 0.0f); }
inline Vector VecPlusY(const Vector& vec, float dy) { return vec + Vector(0.0f,   dy, 0.0f); }
inline Vector VecPlusZ(const Vector& vec, float dz) { return vec + Vector(0.0f, 0.0f,   dz); }

/* 2D version of the above */
inline Vector2D VecPlusX(const Vector2D& vec, float dx) { return vec + Vector2D(  dx, 0.0f); }
inline Vector2D VecPlusY(const Vector2D& vec, float dy) { return vec + Vector2D(0.0f,   dy); }


/* XY-only distance functions */
inline float DistXY   (const Vector& from, const Vector& to) { return from.AsVector2D().DistTo   (to.AsVector2D()); }
inline float DistSqrXY(const Vector& from, const Vector& to) { return from.AsVector2D().DistToSqr(to.AsVector2D()); }


/* return-by-value versions of AngleVectors for when only one of the three vectors is needed */
inline Vector AngleVecFwd  (const QAngle& angles) { Vector vec; AngleVectors(angles,    &vec);                   return vec; }
inline Vector AngleVecRight(const QAngle& angles) { Vector vec; AngleVectors(angles, nullptr,    &vec, nullptr); return vec; }
inline Vector AngleVecUp   (const QAngle& angles) { Vector vec; AngleVectors(angles, nullptr, nullptr,    &vec); return vec; }

/* return-by-value version of VectorAngles, because pass-by-reference is annoying and unneeded */
inline QAngle VectorAngles(const Vector& forward) { QAngle ang; VectorAngles(forward, ang); return ang; }


/* return-by-value versions of CBaseEntity::GetVectors */
inline Vector GetVectorsFwd  (const CBaseEntity *ent) { Vector vec; ent->GetVectors(   &vec, nullptr, nullptr); return vec; }
inline Vector GetVectorsRight(const CBaseEntity *ent) { Vector vec; ent->GetVectors(nullptr,    &vec, nullptr); return vec; }
inline Vector GetVectorsUp   (const CBaseEntity *ent) { Vector vec; ent->GetVectors(nullptr, nullptr,    &vec); return vec; }

/* return-by-value versions of CBasePlayer::EyeVectors */
inline Vector EyeVectorsFwd  (CBasePlayer *player) { Vector vec; player->EyeVectors(   &vec, nullptr, nullptr); return vec; }
inline Vector EyeVectorsRight(CBasePlayer *player) { Vector vec; player->EyeVectors(nullptr,    &vec, nullptr); return vec; }
inline Vector EyeVectorsUp   (CBasePlayer *player) { Vector vec; player->EyeVectors(nullptr, nullptr,    &vec); return vec; }


/* make range-based for loops work with CUtlVectorUltraConservative */
template<class T> static       T* begin(      CUtlVectorUltraConservative<T>& utlvector) { return utlvector.m_pData->m_Elements; }
template<class T> static const T* begin(const CUtlVectorUltraConservative<T>& utlvector) { return utlvector.m_pData->m_Elements; }
template<class T> static       T* end  (      CUtlVectorUltraConservative<T>& utlvector) { return utlvector.m_pData->m_Elements + utlvector.m_pData->m_Size; }
template<class T> static const T* end  (const CUtlVectorUltraConservative<T>& utlvector) { return utlvector.m_pData->m_Elements + utlvector.m_pData->m_Size; }


/* efficiently append multiple copies of the same element to the end of a CUtlVector */
template<class T> inline void VectorAddToTailRepeatedly(CUtlVector<T>& vector, const T& src, int count)
{
	Assert(count >= 0);
	if (count <= 0) return; // not a typo
	
	/* slightly naughty: let me access GrowVector externally, dammit! */
	class CUtlVector_PublicGrow : public CUtlVector<T>
	{
	public: using CUtlVector<T>::GrowVector;
	};
	
	int lo = vector.Count();
	static_cast<CUtlVector_PublicGrow&>(vector).GrowVector(count);
	int hi = vector.Count();
	
	Assert((hi - lo) == count);
	
	for (int i = lo; i < hi; ++i) {
		vector[i] = src;
	}
}


#define FOR_EACH_ENT_BY_CLASSNAME(classname, type, ent) \
	for (auto ent = static_cast<type *>(gEntList.FindEntityByClassname(nullptr, classname)); ent != nullptr; \
		ent = static_cast<type *>(gEntList.FindEntityByClassname(ent, classname)))


#endif
