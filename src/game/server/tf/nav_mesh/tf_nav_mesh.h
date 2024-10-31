/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_NAV_MESH_H
#define TF_NAV_MESH_H
#ifdef _WIN32
#pragma once
#endif


#include "nav_mesh.h"
#include "tf_nav_area.h"

#include <utility>


class CBaseObject;


// FOUR-TEAM MAPS
// cp_cragg
// koth_4teamdevtest
// kotf_4grounds
// cp_hydro


class CTFNavMesh final : public CNavMesh
{
public:
	enum
	{
		TF2C_MAGIC = 0x43324654, // "TF2C"
		
		SUB_VERSION_MIN_VALID  = 0x0100, // lowest version that is even valid to begin with
		SUB_VERSION_MIN_COMPAT = 0x0100, // lowest version that we are backward-compatible with
		SUB_VERSION_CURRENT    = 0x0100, // version that we will currently generate
		// -------------------------------------------------------------------------------------------------------------
		// 0x0001: initial development version
		// 0x0002: rearranged CTFNavArea::AttributeType bits to better align with byte boundaries
		// -------------------------------------------------------------------------------------------------------------
		// 0x0100: INCOMPATIBLE WITH PREVIOUS VERSIONS
		//         moved TF2C magic number from CustomData to CustomDataPreArea
		//         incremented subversion number range substantially to ease identification of live TF2 nav files
		// -------------------------------------------------------------------------------------------------------------
	};
	
	CTFNavMesh();
	virtual ~CTFNavMesh() {}
	
	/* CNavMesh overrides */
	virtual void FireGameEvent(IGameEvent *event) override;
	virtual NavErrorType Load() override;
	virtual CNavArea *CreateArea() const override                { return new CTFNavArea(); }
	virtual void Update() override;
	virtual bool IsAuthoritative() const override                { return true; }
	virtual unsigned int GetSubVersionNumber() const override    { return SUB_VERSION_CURRENT; }
	virtual void SaveCustomData(CUtlBuffer& fileBuffer) const override;
	virtual NavErrorType LoadCustomData(CUtlBuffer& fileBuffer, unsigned int subVersion) override;
	virtual void SaveCustomDataPreArea(CUtlBuffer& fileBuffer) const override;
	virtual NavErrorType LoadCustomDataPreArea(CUtlBuffer& fileBuffer, unsigned int subVersion) override;
	virtual void OnServerActivate() override;
	virtual void OnRoundRestart() override;
	virtual unsigned int GetGenerationTraceMask() const override { return MASK_PLAYERSOLID_BRUSHONLY; }
	virtual void PostCustomAnalysis() override                   {}
	virtual void BeginCustomAnalysis(bool bIncremental) override {}
	virtual void EndCustomAnalysis() override                    {}
	
	// TODO: sort/categorize these
	void CollectAmbushAreas(CUtlVector<CTFNavArea *> *areas, CTFNavArea *area, int team, float maxRange, float f2) const; // TODO: param names: f2
	void CollectAreaWithinBombTravelRange(CUtlVector<CTFNavArea *> *areas, float f1, float f2) const;  // TODO: param names: f1, f2
	void CollectBuiltObjects(CUtlVector<CBaseObject *> *objects, int team);
	void CollectEnemySpawnRoomThresholdAreas(CUtlVector<CTFNavArea *> *areas, int team) const;
	const CUtlVector<CTFNavArea *> *GetControlPointAreas(int idx) const;
	CTFNavArea *GetControlPointCenterArea(int idx) const;
	CTFNavArea *GetControlPointRandomArea(int idx) const;
	bool IsSentryGunHere(CTFNavArea *area) const;
	void MarkSpawnRoomArea(CTFNavArea *area, int team);
	void OnBlockedAreasChanged();
	CTFNavArea *ChooseWeightedRandomArea(const CUtlVector<CTFNavArea *> *areas) const;
	
	void RecomputeBlockers();
	
	const CUtlVector<CTFNavArea *> *GetSpawnRoomAreas(int team) const;
	const CUtlVector<CTFNavArea *> *GetSpawnExitAreas(int team) const;
	const CUtlVector<CTFNavArea *> *GetRRVisualizerAreas(int team) const;

	void CollectSpawnExitAreas( CUtlVector<CTFNavArea *> *areas, int team ) const;
	
	void CollectEnemySpawnRoomAreas(CUtlVector<CTFNavArea *> *areas, int team) const;
	void CollectEnemySpawnExitAreas(CUtlVector<CTFNavArea *> *areas, int team) const;
	void CollectEnemyRRVisualizerAreas(CUtlVector<CTFNavArea *> *areas, int team) const;
	
	const TFNavAreaVector& GetSelectedTFSet() const     { return *reinterpret_cast<const TFNavAreaVector *>(&this->GetSelectedSet()); }
	CTFNavArea *GetMarkedTFArea() const                 { return static_cast<CTFNavArea *>(this->GetMarkedArea()); }
	CTFNavArea *GetSelectedTFArea() const               { return static_cast<CTFNavArea *>(this->GetSelectedArea()); }
	CTFNavArea *GetTFNavAreaByID(unsigned int id) const { return static_cast<CTFNavArea *>(this->GetNavAreaByID(id)); }
	
	template<typename... Args> CTFNavArea *GetTFNavArea       (Args&&... args) const { return static_cast<CTFNavArea *>(this->GetNavArea       (std::forward<Args>(args)...)); }
	template<typename... Args> CTFNavArea *GetNearestTFNavArea(Args&&... args) const { return static_cast<CTFNavArea *>(this->GetNearestNavArea(std::forward<Args>(args)...)); }
	
	template<typename Functor> bool ForAllSelectedTFAreas(Functor&& func)                                          { return this->ForAllSelectedAreas         ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }                    ); }
	template<typename Functor> bool ForAllTFAreas(Functor&& func)                                                  { return this->ForAllAreas                 ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }                    ); }
	template<typename Functor> bool ForAllTFAreas(Functor&& func) const                                            { return this->ForAllAreas                 ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }                    ); }
	template<typename Functor> bool ForAllTFAreasOverlappingExtent(Functor&& func, const Extent& extent)           { return this->ForAllAreasOverlappingExtent([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }, extent            ); }
	template<typename Functor> bool ForAllTFAreasInRadius(Functor&& func, const Vector& pos, float radius)         { return this->ForAllAreasInRadius         ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }, pos, radius       ); }
	template<typename Functor> bool ForAllTFAreasAlongLine(Functor&& func, CNavArea *startArea, CNavArea *endArea) { return this->ForAllAreasAlongLine        ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }, startArea, endArea); }
	
private:
	enum RecomputeMode
	{
		RECOMPUTE_DEFAULT               = 0, // no major change occurred
		RECOMPUTE_SETUP_FINISHED        = 1, // from event "teamplay_setup_finished"
		RECOMPUTE_POINT_CAPTURED        = 2, // from event "teamplay_point_captured"
		RECOMPUTE_POINT_UNLOCKED        = 3, // from event "teamplay_point_unlocked"
		RECOMPUTE_BLOCKED_AREAS_CHANGED = 4, // from OnBlockedAreasChanged
		RECOMPUTE_BLOCKERS_INTERFACE    = 5, // from CPointNavInterface via RecomputeBlockers
		RECOMPUTE_ROUND_WIN             = 6, // from event "teamplay_round_win"
	};
	
	// TODO: sort/categorize these
	void CollectAndMarkSpawnRoomExits(CTFNavArea *area, CUtlVector<CTFNavArea *> *areas);
	void ComputeBlockedAreas();
	void ComputeBombTargetDistance();
	void ComputeControlPointAreas();
	void ComputeIncursionDistances(CTFNavArea *startArea, int team);
	void ComputeIncursionDistances();
	void ComputeInvasionAreas();
	void ComputeLegalBombDropAreas();
	void DecorateMesh();
	void OnObjectChanged();
	void RecomputeInternalData();
	void RemoveAllMeshDecoration();
	void ResetMeshAttributes(bool recompute);
	void UpdateDebugDisplay() const;
	
	void ScheduleRecompute(RecomputeMode mode, int control_point = 0);
	
	CountdownTimer m_ctRecompute;                                      // +0x5cc
	RecomputeMode m_RecomputeMode;                                     // +0x5d8
	int m_iControlPoint;                                               // +0x5dc
	
	CUtlVector<CTFNavArea *> m_SentryAreas;                            // +0x5e0
	// 5f4 CUtlVector<CTFNavArea *>
	
	CUtlVector<CTFNavArea *> m_ControlPointAreas[MAX_CONTROL_POINTS];  // +0x608
	CTFNavArea *m_ControlPointCenter[MAX_CONTROL_POINTS];              // +0x6a8
	
	CUtlVector<CTFNavArea *> m_SpawnRooms[TF_TEAM_COUNT];              // +0x6c8
	CUtlVector<CTFNavArea *> m_SpawnExits[TF_TEAM_COUNT];              // +0x6f0
	CUtlVector<CTFNavArea *> m_RRVisualizerAreas[TF_TEAM_COUNT];       // NEW
	
	// 718?
	// 71c?
	// 720?
	
	int m_LastNextBotCount = 0;                                        // +0x724
};
// TODO: remove offsets once done RE'ing this


template<typename... Args> inline void CollectSurroundingTFAreas(CUtlVector<CTFNavArea *> *nearbyAreaVector, Args&&... args) { CollectSurroundingAreas(reinterpret_cast<CUtlVector<CNavArea *> *>(nearbyAreaVector), std::forward<Args>(args)...); }
template<typename... Args> inline void MarkSurroundingTFAreas   (                                            Args&&... args) { MarkSurroundingAreas   (                                                              std::forward<Args>(args)...); }


#define TheTFNavMesh (static_cast<CTFNavMesh *>(TheNavMesh))


#endif
