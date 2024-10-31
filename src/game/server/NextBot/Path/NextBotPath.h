/* NextBotPath
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_PATH_NEXTBOTPATH_H
#define NEXTBOT_PATH_NEXTBOTPATH_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotInterface.h"
#include "vprof.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"


enum NavDirType;
enum NavTraverseType;
class CNavLadder;
class CFuncElevator;


class IPathCost
{
public:
	virtual float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder = nullptr, const CFuncElevator *elevator = nullptr, float length = -1.0f) const = 0;
};


class IPathOpenGoalSelector
{
public:
	virtual CNavArea *operator()(CNavArea *area, CNavArea *fromArea) const;
};


class Path
{
public:
	enum
	{
		MAX_SEGMENTS  = 256,
		MAX_ADJ_AREAS = 64,
	};
	
	enum SegmentType
	{
		SEG_GROUND      = 0, // ground movement
		SEG_FALL        = 1, // falling down
		SEG_CLIMBJUMP   = 2, // climbing up or jumping
		SEG_GAPJUMP     = 3, // gap jumping
		SEG_LADDER_UP   = 4, // ladder up
		SEG_LADDER_DOWN = 5, // ladder down
	};
	
	enum SeekType
	{
		SEEK_FROM_START  = 0,
		SEEK_FROM_CURSOR = 1,
		SEEK_MAX,
	};
	
	enum MoveCursorType
	{
		MOVECUR_ABS = 0,
		MOVECUR_REL = 1,
		MOVECUR_MAX,
	};
	
	enum ResultType
	{
		// TODO: better names for these enum values
		RESULT_SUCCESS = 0,
		RESULT_FAIL1   = 1,
		RESULT_FAIL2   = 2,
		RESULT_MAX,
		
		// preliminary findings:
		// RESULT_FAIL1 => NavAreaBuildPath failure
		// RESULT_FAIL2 => any other failure
		
		
		// Path::Copy ==========================================================
		// RESULT_SUCCESS always
		
		// Path::BuildTrivialPath ==============================================
		// RESULT_SUCCESS always
		
		// Path::AssemblePrecomputedPath =======================================
		// RESULT_FAIL2   if ComputePathDetails returns false
		// RESULT_SUCCESS otherwise
		
		// Path::Compute [goal] ================================================
		// RESULT_FAIL2   if nextbot->GetEntity()->GetLastKnownArea() is nullptr
		// RESULT_FAIL2   if ComputePathDetails returns false
		// RESULT_FAIL1   if NavAreaBuildPath returns false
		// RESULT_SUCCESS otherwise
		
		// Path::Compute [subject] =============================================
		// RESULT_FAIL2   if nextbot->GetEntity()->GetLastKnownArea() is nullptr
		// RESULT_FAIL2   if subject->GetLastKnownArea() is nullptr
		// RESULT_FAIL2   if ComputePathDetails returns false
		// RESULT_FAIL1   if NavAreaBuildPath returns false
		// RESULT_SUCCESS otherwise
	};
	
	/* easy mode: look @ Lua_PathSegmentToTable in Garry's Mod */
	struct Segment
	{
		// TODO: I made the pointers in here all be const, make them un-const if necessary
		
		const CNavArea *area;     // The navmesh area this segment occupies
		NavTraverseType how;      // The direction of travel to reach the end of this segment from the start, represented as a cardinal direction integer 0 to 3, or 9 for vertical movement
		Vector pos;               // The position of the end of this segment
		const CNavLadder *ladder; // The navmesh ladder this segment occupies, if any
		SegmentType type;         // The movement type of this segment, indicating how bots are expected to move along this segment
		Vector forward;           // The direction of travel to reach the end of this segment from the start, represented as a normalised vector
		float length;             // Length of this segment
		float distanceFromStart;  // Distance of this segment from the start of the path
		float curvature;
		Vector m_portalCenter;
		float m_portalHalfWidth;
	};
	
	/* easy mode: look @ PathFollower__GetCursorData in Garry's Mod */
	struct CursorData
	{
		Vector pos;
		Vector forward;
		float curvature;
		const Segment *seg = nullptr;
	};
	
	struct AdjacentArea
	{
		CNavArea *area;
		CNavLadder *ladder;
		NavTraverseType how;
	};
	
	Path() {}
	virtual ~Path() {}
	
	virtual float GetLength() const;
	
	virtual const Vector& GetPosition(float dist, const Segment *seg) const;
	virtual const Vector& GetClosestPosition(const Vector& near, const Segment *seg, float dist) const;
	virtual const Vector& GetStartPosition() const;
	virtual const Vector& GetEndPosition() const;
	
	virtual CBaseEntity *GetSubject() const       { return this->m_hSubject; }
	virtual const Segment *GetCurrentGoal() const { return nullptr; }
	virtual float GetAge() const                  { return this->m_itAge.GetElapsedTime(); }
	
	virtual void MoveCursorToClosestPosition(const Vector& near, SeekType stype, float dist) const;
	virtual void MoveCursorToStart();
	virtual void MoveCursorToEnd();
	virtual void MoveCursor(float dist, MoveCursorType mctype);
	
	virtual float GetCursorPosition() const         { return this->m_flCursorPosition; }
	virtual const CursorData& GetCursorData() const { return this->m_CursorData; }
	
	virtual bool IsValid() const { return (this->m_iSegCount > 0); }
	virtual void Invalidate();
	
	virtual void Draw(const Segment *seg) const;
	virtual void DrawInterpolated(float from, float to);
	
	virtual const Segment *FirstSegment() const;
	virtual const Segment *NextSegment(const Segment *seg) const;
	virtual const Segment *PriorSegment(const Segment *seg) const;
	virtual const Segment *LastSegment() const;
	
	virtual void OnPathChanged(INextBot *nextbot, ResultType rtype) {}
	
	virtual void Copy(INextBot *nextbot, const Path& that);
	
	virtual bool ComputeWithOpenGoal(INextBot *nextbot, const IPathCost& cost_func, const IPathOpenGoalSelector& sel, float f1 = 0.0f);
	virtual void ComputeAreaCrossing(INextBot *nextbot, const CNavArea *area, const Vector& from, const CNavArea *to, NavDirType dir, Vector *out) const;
	
	template<typename PathCost> bool Compute(INextBot *nextbot, const Vector& goal, const PathCost& cost_func, float maxPathLength = 0.0f, bool b1 = true);
	template<typename PathCost> bool Compute(INextBot *nextbot, CBaseCombatCharacter *subject, const PathCost& cost_func, float maxPathLength = 0.0f, bool b1 = true);
	
private:
	void AssemblePrecomputedPath(INextBot *nextbot, const Vector& vec, CNavArea *area);
	bool BuildTrivialPath(INextBot *nextbot, const Vector& dest);
	void CollectAdjacentAreas(CNavArea *area);
	bool ComputePathDetails(INextBot *nextbot, const Vector& vec);
	int FindNextOccludedNode(INextBot *nextbot, int index);
	void InsertSegment(const Segment& seg, int index);
	void Optimize(INextBot *nextbot) {}
	void PostProcess();
	
	Segment m_Segments[MAX_SEGMENTS];
	int m_iSegCount = 0;
	mutable Vector m_vecGetPosition;
	mutable Vector m_vecGetClosestPosition;
	mutable float m_flCursorPosition = 0.0f;
	mutable CursorData m_CursorData;
	mutable bool m_bCursorDataDirty = true;
	IntervalTimer m_itAge;
	CHandle<CBaseEntity> m_hSubject;
	AdjacentArea m_AdjacentAreas[MAX_ADJ_AREAS];
	int m_AdjacentAreaCount;
	
	// TODO: parameter names for ALL funcs
};


template<typename PathCost>
inline bool Path::Compute(INextBot *nextbot, const Vector& goal, const PathCost& cost_func, float maxPathLength, bool b1)
{
	VPROF_BUDGET("Path::Compute(goal)", "NextBotSpiky");
	
	this->Invalidate();
	
	const Vector& nb_pos = nextbot->GetPosition();
	
	CNavArea *startArea = nextbot->GetEntity()->GetLastKnownArea();
	if (startArea == nullptr) {
		this->OnPathChanged(nextbot, RESULT_FAIL2);
		return false;
	}
	
	CNavArea *goalArea = TheNavMesh->GetNearestNavArea(goal, true, 200.0f, true, true);
	if (startArea == goalArea) {
		this->BuildTrivialPath(nextbot, goal);
		return true;
	}
	
	// TODO: better name for goal2
	Vector goal2 = goal;
	if (goalArea != nullptr) {
		goal2.z = goalArea->GetZ(goal2.x, goal2.y);
	} else {
		TheNavMesh->GetGroundHeight(goal2, &goal2.z);
	}
	
	CNavArea *closestArea = nullptr;
	bool success = NavAreaBuildPath(startArea, goalArea, &goal, cost_func, &closestArea, maxPathLength, nextbot->GetEntity()->GetTeamNumber());
	
	if (closestArea == nullptr) {
		return false;
	}
	
	int num_segments = 0;
	for (CNavArea *area = closestArea; area != nullptr; area = area->GetParent()) {
		++num_segments;
		
		if (area == startArea)                break;
		if (num_segments >= MAX_SEGMENTS - 1) break;
	}
	
	if (num_segments == 1) {
		this->BuildTrivialPath(nextbot, goal);
		return success;
	}
	
	this->m_iSegCount = num_segments;
	
	CNavArea *area = closestArea;
	for (int i = num_segments; i != 0 && area != nullptr; --i, area = area->GetParent()) {
		Segment *seg = &this->m_Segments[i - 1];
		
		seg->area = area;
		seg->how  = area->GetParentHow();
		seg->type = SEG_GROUND;
	}
	
	// TODO: name for b1
	if (success || b1) {
		Segment *seg_last = &this->m_Segments[this->m_iSegCount];
		
		seg_last->area   = closestArea;
		seg_last->pos    = goal2;
		seg_last->ladder = nullptr;
		seg_last->how    = NUM_TRAVERSE_TYPES;
		seg_last->type   = SEG_GROUND;
		
		++this->m_iSegCount;
	}
	
	if (!this->ComputePathDetails(nextbot, nb_pos)) {
		this->Invalidate();
		this->OnPathChanged(nextbot, RESULT_FAIL2);
		return false;
	}
	
	this->Optimize(nextbot);
	this->PostProcess();
	
	this->OnPathChanged(nextbot, (success ? RESULT_SUCCESS : RESULT_FAIL1));
	
	return success;
}

template<typename PathCost>
inline bool Path::Compute(INextBot *nextbot, CBaseCombatCharacter *subject, const PathCost& cost_func, float maxPathLength, bool b1)
{
	VPROF_BUDGET("Path::Compute(subject)", "NextBotSpiky");
	
	this->Invalidate();
	
	this->m_hSubject = subject;
	
	const Vector& nb_pos = nextbot->GetPosition();
	
	CNavArea *startArea = nextbot->GetEntity()->GetLastKnownArea();
	if (startArea == nullptr) {
		this->OnPathChanged(nextbot, RESULT_FAIL2);
		return false;
	}
	
	CNavArea *goalArea = subject->GetLastKnownArea();
	if (goalArea == nullptr) {
		this->OnPathChanged(nextbot, RESULT_FAIL2);
		return false;
	}
	
	Vector goal = subject->GetAbsOrigin();
	
	if (startArea == goalArea) {
		this->BuildTrivialPath(nextbot, goal);
		return true;
	}
	
	CNavArea *closestArea = nullptr;
	bool success = NavAreaBuildPath(startArea, goalArea, &goal, cost_func, &closestArea, maxPathLength, nextbot->GetEntity()->GetTeamNumber());
	
	if (closestArea == nullptr) {
		return false;
	}
	
	int num_segments = 0;
	for (CNavArea *area = closestArea; area != nullptr; area = area->GetParent()) {
		++num_segments;
		
		if (area == startArea)                break;
		if (num_segments >= MAX_SEGMENTS - 1) break;
	}
	
	if (num_segments == 1) {
		this->BuildTrivialPath(nextbot, goal);
		return success;
	}
	
	this->m_iSegCount = num_segments;
	
	CNavArea *area = closestArea;
	for (int i = num_segments; i != 0 && area != nullptr; --i, area = area->GetParent()) {
		Segment *seg = &this->m_Segments[i - 1];
		
		seg->area = area;
		seg->how  = area->GetParentHow();
		seg->type = SEG_GROUND;
	}
	
	// TODO: name for b1
	if (success || b1) {
		Segment *seg_last = &this->m_Segments[this->m_iSegCount];
		
		seg_last->area   = closestArea;
		seg_last->pos    = goal;
		seg_last->ladder = nullptr;
		seg_last->how    = NUM_TRAVERSE_TYPES;
		seg_last->type   = SEG_GROUND;
		
		++this->m_iSegCount;
	}
	
	if (!this->ComputePathDetails(nextbot, nb_pos)) {
		this->Invalidate();
		this->OnPathChanged(nextbot, RESULT_FAIL2);
		return false;
	}
	
	this->Optimize(nextbot);
	this->PostProcess();
	
	this->OnPathChanged(nextbot, (success ? RESULT_SUCCESS : RESULT_FAIL1));
	
	return success;
}


#endif
