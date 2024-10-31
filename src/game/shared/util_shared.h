//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared util code between client and server.
//
//=============================================================================//

#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "cmodel.h"
#include "utlvector.h"
#include "networkvar.h"
#include "engine/IEngineTrace.h"
#include "engine/IStaticPropMgr.h"
#include "shared_classnames.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
#endif

#ifdef PORTAL
#include "portal_util_shared.h"
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CGameTrace;
class CBasePlayer;
typedef CGameTrace trace_t;

extern ConVar developer;	// developer mode


//-----------------------------------------------------------------------------
// Language IDs.
//-----------------------------------------------------------------------------
#define LANGUAGE_ENGLISH				0
#define LANGUAGE_GERMAN					1
#define LANGUAGE_FRENCH					2
#define LANGUAGE_BRITISH				3


//-----------------------------------------------------------------------------
// Pitch + yaw
//-----------------------------------------------------------------------------
float		UTIL_VecToYaw			(const Vector &vec);
float		UTIL_VecToPitch			(const Vector &vec);
float		UTIL_VecToYaw			(const matrix3x4_t& matrix, const Vector &vec);
float		UTIL_VecToPitch			(const matrix3x4_t& matrix, const Vector &vec);
Vector		UTIL_YawToVector		( float yaw );

//-----------------------------------------------------------------------------
// Shared random number generators for shared/predicted code:
// whenever generating random numbers in shared/predicted code, these functions
// have to be used. Each call should specify a unique "sharedname" string that
// seeds the random number generator. In loops make sure the "additionalSeed"
// is increased with the loop counter, otherwise it will always return the
// same random number
//-----------------------------------------------------------------------------
float	SharedRandomFloat( const char *sharedname, float flMinVal, float flMaxVal, int additionalSeed = 0 );
int		SharedRandomInt( const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed = 0 );
Vector	SharedRandomVector( const char *sharedname, float minVal, float maxVal, int additionalSeed = 0 );
QAngle	SharedRandomAngle( const char *sharedname, float minVal, float maxVal, int additionalSeed = 0 );

//-----------------------------------------------------------------------------
// Standard collision filters...
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass );
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask );


//-----------------------------------------------------------------------------
// Converts an IHandleEntity to an CBaseEntity
//-----------------------------------------------------------------------------
inline const CBaseEntity *EntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#endif
}

inline CBaseEntity *EntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#endif
}

typedef bool (*ShouldHitFunc_t)( IHandleEntity *pHandleEntity, int contentsMask );

//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterSimple );

	CTraceFilterSimple( const IHandleEntity *passentity, int collisionGroup, ShouldHitFunc_t pExtraShouldHitCheckFn = NULL );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity( const IHandleEntity *pPassEntity ) { m_pPassEnt = pPassEntity; }
	virtual void SetCollisionGroup( int iCollisionGroup ) { m_collisionGroup = iCollisionGroup; }

	const IHandleEntity *GetPassEntity( void ){ return m_pPassEnt;}

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
	ShouldHitFunc_t m_pExtraShouldHitCheckFunction;

};

class CTraceFilterSkipTwoEntities : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoEntities, CTraceFilterSimple );
	
	CTraceFilterSkipTwoEntities( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity2( const IHandleEntity *pPassEntity2 ) { m_pPassEnt2 = pPassEntity2; }

private:
	const IHandleEntity *m_pPassEnt2;
};

class CTraceFilterSimpleList : public CTraceFilterSimple
{
public:
	CTraceFilterSimpleList( int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

	void	AddEntityToIgnore( IHandleEntity *pEntity );
protected:
	CUtlVector<IHandleEntity*>	m_PassEntities;
};

class CTraceFilterOnlyNPCsAndPlayer : public CTraceFilterSimple
{
public:
	CTraceFilterOnlyNPCsAndPlayer( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterNoNPCsOrPlayer : public CTraceFilterSimple
{
public:
	CTraceFilterNoNPCsOrPlayer( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

//-----------------------------------------------------------------------------
// Purpose: Custom trace filter used for NPC LOS traces
//-----------------------------------------------------------------------------
class CTraceFilterLOS : public CTraceFilterSkipTwoEntities
{
public:
	CTraceFilterLOS( IHandleEntity *pHandleEntity, int collisionGroup, IHandleEntity *pHandleEntity2 = NULL );
	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterSkipClassname : public CTraceFilterSimple
{
public:
	CTraceFilterSkipClassname( const IHandleEntity *passentity, const char *pchClassname, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:

	const char *m_pchClassname;
};

class CTraceFilterSkipTwoClassnames : public CTraceFilterSkipClassname
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoClassnames, CTraceFilterSkipClassname );

	CTraceFilterSkipTwoClassnames( const IHandleEntity *passentity, const char *pchClassname, const char *pchClassname2, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	const char *m_pchClassname2;
};

class CTraceFilterSimpleClassnameList : public CTraceFilterSimple
{
public:
	CTraceFilterSimpleClassnameList( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

	void	AddClassnameToIgnore( const char *pchClassname );
private:
	CUtlVector<const char*>	m_PassClassnames;
};

class CTraceFilterChain : public CTraceFilter
{
public:
	CTraceFilterChain( ITraceFilter *pTraceFilter1, ITraceFilter *pTraceFilter2 );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	ITraceFilter	*m_pTraceFilter1;
	ITraceFilter	*m_pTraceFilter2;
};

#if defined(TF_CLASSIC) || defined(TF_CLASSIC_CLIENT)
class CTraceFilterFunction : public CTraceFilter
{
	typedef bool( *TraceFilterFunc_t )( IHandleEntity *pHandleEntity, int contentsMask, void *pUserData );

public:
	CTraceFilterFunction( TraceFilterFunc_t condition, void *pUserData )
		: m_condition(condition), m_pUserData(pUserData)
	{}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		return m_condition( pHandleEntity, contentsMask, m_pUserData );
	}

private:
	TraceFilterFunc_t	m_condition;
	void				*m_pUserData;
};
#endif

// helper
void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration );

extern ConVar r_visualizetraces;

inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}

inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 255, 0, true, -1.0f );
	}
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 255, 0, true, -1.0f );
	}
}

inline void UTIL_TraceRay( const Ray_t &ray, unsigned int mask, 
						  const IHandleEntity *ignore, int collisionGroup, trace_t *ptr, ShouldHitFunc_t pExtraShouldHitCheckFn = NULL )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup, pExtraShouldHitCheckFn );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
	
	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}


// Sweeps a particular entity through the world
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr );
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, ITraceFilter *pFilter, trace_t *ptr );
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *ignore, int collisionGroup, trace_t *ptr );

bool UTIL_EntityHasMatchingRootParent( CBaseEntity *pRootParent, CBaseEntity *pEntity );

inline int UTIL_PointContents( const Vector &vec )
{
	return enginetrace->GetPointContents( vec );
}

// Sweeps against a particular model, using collision rules 
void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CBaseEntity *pentModel, int collisionGroup, trace_t *ptr );

void UTIL_ClipTraceToPlayers( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *filter, trace_t *tr );

// Particle effect tracer
void		UTIL_ParticleTracer( const char *pszTracerEffectName, const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = 0, bool bWhiz = false );

// Old style, non-particle system, tracers
void		UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = TRACER_DONT_USE_ATTACHMENT, float flVelocity = 0, bool bWhiz = false, const char *pCustomTracerName = NULL, int iParticleID = 0 );

bool		UTIL_IsLowViolence( void );
bool		UTIL_ShouldShowBlood( int bloodColor );
void		UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount );

void		UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount );
void		UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor );
void		UTIL_DecalTrace( trace_t *pTrace, char const *decalName );
bool		UTIL_IsSpaceEmpty( CBaseEntity *pMainEnt, const Vector &vMin, const Vector &vMax );

void		UTIL_StringToVector( float *pVector, const char *pString );
void		UTIL_StringToIntArray( int *pVector, int count, const char *pString );
void		UTIL_StringToFloatArray( float *pVector, int count, const char *pString );
void		UTIL_StringToColor32( color32 *color, const char *pString );

// Version of UTIL_StringToIntArray that doesn't set all untouched array elements to 0.
void		UTIL_StringToIntArray_PreserveArray( int *pVector, int count, const char *pString );

// Version of UTIL_StringToFloatArray that doesn't set all untouched array elements to 0.
void		UTIL_StringToFloatArray_PreserveArray( float *pVector, int count, const char *pString );

CBasePlayer *UTIL_PlayerByIndex( int entindex );

//=============================================================================
// HPE_BEGIN:
// [menglish] Added UTIL function for events in client win_panel which transmit the player as a user ID
//=============================================================================
CBasePlayer *UTIL_PlayerByUserId( int userID );
//=============================================================================
// HPE_END
//=============================================================================

// decodes a buffer using a 64bit ICE key (inplace)
void		UTIL_DecodeICE( unsigned char * buffer, int size, const unsigned char *key);


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position and a ray, return the shortest distance between the two.
 * If 'pos' is beyond either end of the ray, the returned distance is negated.
 */
inline float DistanceToRay( const Vector &pos, const Vector &rayStart, const Vector &rayEnd, float *along = NULL, Vector *pointOnRay = NULL )
{
	Vector to = pos - rayStart;
	Vector dir = rayEnd - rayStart;
	float length = dir.NormalizeInPlace();

	float rangeAlong = DotProduct( dir, to );
	if (along)
	{
		*along = rangeAlong;
	}

	float range;

	if (rangeAlong < 0.0f)
	{
		// off start point
		range = -(pos - rayStart).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayStart;
		}
	}
	else if (rangeAlong > length)
	{
		// off end point
		range = -(pos - rayEnd).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayEnd;
		}
	}
	else // within ray bounds
	{
		Vector onRay = rayStart + rangeAlong * dir;
		range = (pos - onRay).Length();

		if (pointOnRay)
		{
			*pointOnRay = onRay;
		}
	}

	return range;
}


#if defined(TF_CLASSIC) || defined(TF_CLASSIC_CLIENT)

// Add auto-list capability to class X by making it inherit from TAutoList<X>:
// 
//   class CWidget : public CBaseWidget, public TAutoList<CWidget>
//   {
//       ...
//   }
// 
// 
// To iterate the auto-list in code, you can use a C++11 range-based for loop:
// 
//   for (CWidget *pWidget : CWidget::AutoList())
//   {
//       ...
//   }
// 
// Or, you can use less-fancy mechanisms if you really insist on it:
// 
//   for (int i = 0; i < CWidget::AutoList().Count(); ++i)
//   {
//       CWidget *pWidget = CWidget::AutoList()[i];
//       ...
//   }

// NOTE: if a particular class has multiple TAutoList's in its inheritance, like so:
// 
//   class CBaseThing : public CBaseEntity, public TAutoList<CBaseThing> { ... };
//   class CThing     : public CBaseThing,  public TAutoList<CThing>     { ... };
// 
// Then we have an interesting situation.
// 
// If we want to iterate over the auto-list of CBaseThing's, we simply use CBaseThing::AutoList().
// But if we want to iterate over the auto-list of CThing's, CThing::AutoList() is ambiguous!
// CThing has an AutoList() function from CBaseThing, and ALSO an AutoList() function from CThing.
// 
// The workaround is to use TAutoList<CThing>, which unambiguously refers to the auto-list of CThing's.
// It's slightly ugly, but it works.
// 
// When I originally wrote this class, I didn't consider that more than one level in a class hierarchy
// might have auto-lists, so I didn't contemplate the implications of that situation.

template<class T>
class TAutoList {
public:
	typedef CUtlVector<T*> AutoListType;
	
	/* s_AutoList is a static function variable rather than a static class
	 * member variable so we can take advantage of "vague linkage" and avoid
	 * having to put ODR definitions in CPP files to make the damn thing link */
	/* NOTE: C++17 added inline variables as a feature, which would eliminate
	 * the need for this clunky workaround. In this header, we could simply
	 * declare e.g. 'static inline AutoListType AutoList;' and it'd Just Work.
	 * But alas, we do not have C++17... :^( */
	static AutoListType& AutoList()
	{
		static AutoListType s_AutoList;
		return s_AutoList;
	}
	
protected:
	TAutoList()          { AutoList().AddToTail        (static_cast<T*>(this)); }
	virtual ~TAutoList() { AutoList().FindAndFastRemove(static_cast<T*>(this)); }
};

/* CUtlVector<T> range-based for doesn't actually work on whatever crusty old
 * version of GCC we're using without these definitions */
template<class T> inline typename CUtlVector<T>::iterator begin(CUtlVector<T>& utlvector) { return utlvector.begin(); }
template<class T> inline typename CUtlVector<T>::iterator end  (CUtlVector<T>& utlvector) { return utlvector.end(); }

/* make base code that uses the old macros seamlessly use the template version
 * without having to actually modify their code */
#define DECLARE_AUTO_LIST(interfaceName)
#define IMPLEMENT_AUTO_LIST(interfaceName)

#define IPhysicsPropAutoList        TAutoList<CPhysicsProp>
#define ITriggerHurtAutoList        TAutoList<CTriggerHurt>
#define ITFTeamTrainWatcher         TAutoList<CTeamTrainWatcher>
#define ITriggerAreaCaptureAutoList TAutoList<CTriggerAreaCapture>
#define ITriggerHurtAutoList        TAutoList<CTriggerHurt>
#define IBaseProjectileAutoList     TAutoList<CBaseProjectile>
#define IFuncNoBuildAutoList		TAutoList<CFuncNoBuild>
#define IHudItemEffectMeterAutoList	TAutoList<CHudItemEffectMeter>
#define ITFMvMBossProgressUserAutoList TAutoList<C_TFMvMBossProgressUser>

#else

//--------------------------------------------------------------------------------------------------------------
/**
* Macro for creating an interface that when inherited from automatically maintains a list of instances
* that inherit from that interface.
*/

// interface for entities that want to a auto maintained global list
#define DECLARE_AUTO_LIST( interfaceName ) \
	class interfaceName; \
	abstract_class interfaceName \
	{ \
	public: \
		interfaceName( bool bAutoAdd = true ); \
		virtual ~interfaceName(); \
		static void AddToAutoList( interfaceName *pElement ) { m_##interfaceName##AutoList.AddToTail( pElement ); } \
		static void RemoveFromAutoList( interfaceName *pElement ) { m_##interfaceName##AutoList.FindAndFastRemove( pElement ); } \
		static const CUtlVector< interfaceName* >& AutoList( void ) { return m_##interfaceName##AutoList; } \
	private: \
		static CUtlVector< interfaceName* > m_##interfaceName##AutoList; \
	};

// Creates the auto add/remove constructor/destructor...
// Pass false to the constructor to not auto add
#define IMPLEMENT_AUTO_LIST( interfaceName ) \
	CUtlVector< class interfaceName* > interfaceName::m_##interfaceName##AutoList; \
	interfaceName::interfaceName( bool bAutoAdd ) \
	{ \
		if ( bAutoAdd ) \
		{ \
			AddToAutoList( this ); \
		} \
	} \
	interfaceName::~interfaceName() \
	{ \
		RemoveFromAutoList( this ); \
	}

//--------------------------------------------------------------------------------------------------------------
// This would do the same thing without requiring casts all over the place. Yes, it's a template, but 
// DECLARE_AUTO_LIST requires a CUtlVector<T> anyway. TODO ask about replacing the macros with this.
//template<class T>
//class AutoList {
//public:
//	typedef CUtlVector<T*> AutoListType;
//	static AutoListType& All() { return m_autolist; }
//protected:
//	AutoList() { m_autolist.AddToTail(static_cast<T*>(this)); }
//	virtual ~AutoList() { m_autolist.FindAndFastRemove(static_cast<T*>(this)); }
//private:
//	static AutoListType m_autolist;
//};

#endif


#if defined(TF_CLASSIC) || defined(TF_CLASSIC_CLIENT)

// Valve was apparently too dumb to figure out that they needed an extern declaration for gpGlobals (since this header
// precedes the inclusion of util.h in the server DLL); and that the declaration needs to be different depending on
// whether this is a server or client DLL build
#ifdef GAME_DLL
extern CGlobalVars *gpGlobals;
#else
extern CGlobalVarsBase *gpGlobals;
#endif

// Improved version of IntervalTimer. Improvements:
// - 'IsLessThan' and 'IsGreaterThan' are spelled the way the English language intended
// - Doesn't use the ternary operator with 'true' and 'false' like a retard
// - Returns FLT_MAX for GetElapsedTime when not started (rather than 99999.9f, which can cause issues after 27 hours)
// - The IsLessThan and IsGreaterThan functions actually return sane results when the timer isn't started
// - Behaves correctly if gpGlobals->curtime is 0.0f when the timer is started
// - Uses actual member variable initializers instead of assignments in the constructor
// - Ugliness substantially reduced
class IntervalTimer FINAL
{
public:
	IntervalTimer()  = default;
	~IntervalTimer() = default;

	void Invalidate() { m_tEpoch = -1.0f; }
	void Reset()      { m_tEpoch = Now(); }
	void Start()      { m_tEpoch = Now(); }

	bool HasStarted() const { return ( m_tEpoch >= 0.0f ); }

	float GetElapsedTime() const
	{
		if ( !HasStarted() )
			return FLT_MAX;
		
		return ( Now() - m_tEpoch );
	}

	bool IsLessThan   ( float dtCompare ) const { return ( GetElapsedTime() < dtCompare ); }
	bool IsGreaterThan( float dtCompare ) const { return ( GetElapsedTime() > dtCompare ); }

private:
	float Now() const { return gpGlobals->curtime; }

	float m_tEpoch = -1.0f; // time at which the timer was started
};

// Improved version of CountdownTimer. Improvements:
// - The Now function is non-virtual, making class instances 33% smaller and dramatically increasing performance
// - Behaves correctly if gpGlobals->curtime is 0.0f when the timer is started
// - Uses actual member variable initializers instead of assignments in the constructor
// - Ugliness substantially reduced
class CountdownTimer FINAL
{
public:
	CountdownTimer()  = default;
	~CountdownTimer() = default;

	void Invalidate() { m_tElapse = -1.0f; }
	void Reset()      { m_tElapse = ( m_dtDuration + Now() ); }

	void Start( float dtDuration )
	{
		m_dtDuration = dtDuration;
		m_tElapse    = dtDuration + Now();
	}

	bool HasStarted() const { return ( m_tElapse >= 0.0f ); }
	bool IsElapsed()  const { return ( m_tElapse < Now() ); }

	float GetElapsedTime()   const { return ( m_dtDuration - GetRemainingTime() ); }
	float GetRemainingTime() const { return ( m_tElapse - Now() );                 }

	float GetCountdownDuration() const
	{
		if ( !HasStarted() )
			return 0.0f;
		
		return m_dtDuration;
	}

	/// 1.0 for newly started, 0.0 for elapsed
	float GetRemainingRatio( void ) const
	{
		if (HasStarted() && m_dtDuration > 0.0f)
		{
			float left = GetRemainingTime() / m_dtDuration;
			if (left < 0.0f)
				return 0.0f;
			if (left > 1.0f)
				return 1.0f;
			return left;
		}

		return 0.0f;
	}

private:
	float Now() const { return gpGlobals->curtime; }

	float m_tElapse    = -1.0f; // time after which the timer will be considered to have elapsed
	float m_dtDuration =  0.0f; // time delta between when timer was started and when timer elapses
};

// In TF2C, we have no need for RealTimeCountdownTimer. If we ever do need it, please do me a favor and don't use
// virtual functions like Valve did. It makes every single CountdownTimer 12 bytes instead of 8 bytes. And it makes
// every single internal call to CountdownTimer::Now a virtual function dispatch (expensive) instead of an inlined
// global variable access (cheap). In other words, just don't do completely idiotic stuff. Thanks.

#else

//--------------------------------------------------------------------------------------------------------------
/**
 * Simple class for tracking intervals of game time.
 * Upon creation, the timer is invalidated.  To measure time intervals, start the timer via Start().
 */
class IntervalTimer
{
public:
	IntervalTimer( void )
	{
		m_timestamp = -1.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now();
	}

	void Start( void )
	{
		m_timestamp = Now();
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	/// if not started, elapsed time is very large
	float GetElapsedTime( void ) const
	{
		return (HasStarted()) ? (Now() - m_timestamp) : 99999.9f;
	}

	bool IsLessThen( float duration ) const
	{
		return (Now() - m_timestamp < duration) ? true : false;
	}

	bool IsGreaterThen( float duration ) const
	{
		return (Now() - m_timestamp > duration) ? true : false;
	}

private:
	float m_timestamp;
	float Now( void ) const;		// work-around since client header doesn't like inlined gpGlobals->curtime
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Simple class for counting down a short interval of time.
 * Upon creation, the timer is invalidated.  Invalidated countdown timers are considered to have elapsed.
 */

class CountdownTimer
{
public:
	CountdownTimer( void )
	{
		m_timestamp = -1.0f;
		m_duration = 0.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now() + m_duration;
	}

	void Start( float duration )
	{
		m_timestamp = Now() + duration;
		m_duration = duration;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	bool IsElapsed( void ) const
	{
		return (Now() > m_timestamp);
	}

	float GetElapsedTime( void ) const
	{
		return Now() - m_timestamp + m_duration;
	}

	float GetRemainingTime( void ) const
	{
		return (m_timestamp - Now());
	}

	/// return original countdown time
	float GetCountdownDuration( void ) const
	{
		return (m_timestamp > 0.0f) ? m_duration : 0.0f;
	}

	/// 1.0 for newly started, 0.0 for elapsed
	float GetRemainingRatio( void ) const
	{
		if (HasStarted() && m_duration > 0.0f)
		{
			float left = GetRemainingTime() / m_duration;
			if (left < 0.0f)
				return 0.0f;
			if (left > 1.0f)
				return 1.0f;
			return left;
		}

		return 0.0f;
	}

private:
	float m_duration;
	float m_timestamp;
	virtual float Now( void ) const;		// work-around since client header doesn't like inlined gpGlobals->curtime
};

class RealTimeCountdownTimer : public CountdownTimer
{
	virtual float Now( void ) const OVERRIDE
	{
		return Plat_FloatTime();
	}
};

#endif

char* ReadAndAllocStringValue( KeyValues *pSub, const char *pName, const char *pFilename = NULL );

int UTIL_StringFieldToInt( const char *szValue, const char **pValueStrings, int iNumStrings );
int UTIL_CountNumBitsSet( unsigned int nVar );
int UTIL_CountNumBitsSet( uint64 nVar );

//-----------------------------------------------------------------------------
// Holidays
//-----------------------------------------------------------------------------

// Used at level change and round start to re-calculate which holiday is active
void				UTIL_CalculateHolidays();

bool				UTIL_IsHolidayActive( /*EHoliday*/ int eHoliday );
/*EHoliday*/ int	UTIL_GetHolidayForString( const char* pszHolidayName );

// This will return the first active holiday string it can find. In the case of multiple
// holidays overlapping, the list order will act as priority.
const char		   *UTIL_GetActiveHolidayString();


#endif // UTIL_SHARED_H