/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_NAV_AREA_H
#define TF_NAV_AREA_H
#ifdef _WIN32
#pragma once
#endif


#include "nav_area.h"


#define BIT(x) (1ULL << (x))


struct TFNavConnect;

using TFNavConnectVector = CUtlVectorUltraConservative<TFNavConnect, CNavVectorAllocator>;
static_assert(sizeof(TFNavConnectVector) == sizeof(NavConnectVector), "");


class CTFNavArea final : public CNavArea
{
public:
	friend class CTFNavMesh;
	
	// NOTE: adding/removing/changing static attributes in this enum REQUIRES incrementing CTFNavMesh::SUB_VERSION!
	enum AttributeType : uint64
	{
		BLOCKED                     = BIT( 0), // dynamic
		UNBLOCKABLE                 = BIT( 1), // static
		
		RED_SPAWN_ROOM              = BIT( 2), // dynamic
		BLUE_SPAWN_ROOM             = BIT( 3), // dynamic
		GREEN_SPAWN_ROOM            = BIT( 4), // dynamic
		YELLOW_SPAWN_ROOM           = BIT( 5), // dynamic
		SPAWN_ROOM_EXIT             = BIT( 6), // dynamic
		NO_SPAWNING                 = BIT( 7), // static
		
		// BYTE boundary -------------------------------------------------------
		
		AMMO                        = BIT( 8), // dynamic
		HEALTH                      = BIT( 9), // dynamic
		
		CONTROL_POINT               = BIT(10), // dynamic
		
		SNIPER_SPOT                 = BIT(11), // static
		SENTRY_SPOT                 = BIT(12), // static
		
		RESCUE_CLOSET               = BIT(13), // static
		BOMB_DROP                   = BIT(14), // dynamic
		
		// UNUSED: bit 15
		
		// WORD boundary -------------------------------------------------------
		
		RED_SENTRY                  = BIT(16), // dynamic
		BLUE_SENTRY                 = BIT(17), // dynamic
		GREEN_SENTRY                = BIT(18), // dynamic
		YELLOW_SENTRY               = BIT(19), // dynamic
		
		RED_SETUP_GATE              = BIT(20), // static
		BLUE_SETUP_GATE             = BIT(21), // static
		GREEN_SETUP_GATE            = BIT(22), // static
		YELLOW_SETUP_GATE           = BIT(23), // static
		
		// BYTE boundary -------------------------------------------------------
		
		RED_ONE_WAY_DOOR            = BIT(24), // static
		BLUE_ONE_WAY_DOOR           = BIT(25), // static
		GREEN_ONE_WAY_DOOR          = BIT(26), // static
		YELLOW_ONE_WAY_DOOR         = BIT(27), // static
		
		DOOR_NEVER_BLOCKS           = BIT(28), // static
		DOOR_ALWAYS_BLOCKS          = BIT(29), // static
		
		BLOCKED_AFTER_POINT_CAPTURE = BIT(30), // static
		BLOCKED_UNTIL_POINT_CAPTURE = BIT(31), // static
		
		// DWORD boundary ------------------------------------------------------
		
		WITH_SECOND_POINT           = BIT(32), // static
		WITH_THIRD_POINT            = BIT(33), // static
		WITH_FOURTH_POINT           = BIT(34), // static
		WITH_FIFTH_POINT            = BIT(35), // static
		
		RED_RR_VISUALIZER           = BIT(36), // dynamic
		BLUE_RR_VISUALIZER          = BIT(37), // dynamic
		GREEN_RR_VISUALIZER         = BIT(38), // dynamic
		YELLOW_RR_VISUALIZER        = BIT(39), // dynamic
		
		// ---------------------------------------------------------------------
		
		/* bits from live TF2 with unknown purpose (still here just in case) */
		UNKNOWN_BIT09               = BIT(59), // dynamic (UNUSED)
		UNKNOWN_BIT10               = BIT(60), // dynamic (UNUSED)
		ESCAPE_ROUTE                = BIT(61), // dynamic
		ESCAPE_ROUTE_VISIBLE        = BIT(62), // dynamic
		UNKNOWN_BIT31               = BIT(63), // dynamic (UNUSED)
		
		// ---------------------------------------------------------------------
		
		/* multi-team combinations */
		ANY_SPAWN_ROOM    = (RED_SPAWN_ROOM    | BLUE_SPAWN_ROOM    | GREEN_SPAWN_ROOM    | YELLOW_SPAWN_ROOM   ),
		ANY_SENTRY        = (RED_SENTRY        | BLUE_SENTRY        | GREEN_SENTRY        | YELLOW_SENTRY       ),
		ANY_SETUP_GATE    = (RED_SETUP_GATE    | BLUE_SETUP_GATE    | GREEN_SETUP_GATE    | YELLOW_SETUP_GATE   ),
		ANY_ONE_WAY_DOOR  = (RED_ONE_WAY_DOOR  | BLUE_ONE_WAY_DOOR  | GREEN_ONE_WAY_DOOR  | YELLOW_ONE_WAY_DOOR ),
		ANY_RR_VISUALIZER = (RED_RR_VISUALIZER | BLUE_RR_VISUALIZER | GREEN_RR_VISUALIZER | YELLOW_RR_VISUALIZER),
		
		/* other combinations */
		WITH_ANY_POINT = (WITH_SECOND_POINT | WITH_THIRD_POINT | WITH_FOURTH_POINT | WITH_FIFTH_POINT),
		ALL_UNKNOWN_BITS = (UNKNOWN_BIT09 | UNKNOWN_BIT10 | ESCAPE_ROUTE | ESCAPE_ROUTE_VISIBLE | UNKNOWN_BIT31),
		
		/* static attributes are generated/edited and are saved to file */
		STATIC_ATTRS = (UNBLOCKABLE | NO_SPAWNING | SNIPER_SPOT | SENTRY_SPOT |
			RESCUE_CLOSET | ANY_SETUP_GATE | ANY_ONE_WAY_DOOR | DOOR_NEVER_BLOCKS | DOOR_ALWAYS_BLOCKS |
			BLOCKED_AFTER_POINT_CAPTURE | BLOCKED_UNTIL_POINT_CAPTURE | WITH_ANY_POINT),
		
		/* dynamic attributes are updated at runtime and aren't saved */
		DYNAMIC_ATTRS = (BLOCKED | ANY_SPAWN_ROOM | SPAWN_ROOM_EXIT | AMMO | HEALTH |
			CONTROL_POINT | BOMB_DROP | ANY_SENTRY | ANY_RR_VISUALIZER | ALL_UNKNOWN_BITS),
	};
	static_assert((STATIC_ATTRS | DYNAMIC_ATTRS) == 0xf80000ffffff7fff, "");
	static_assert((STATIC_ATTRS ^ DYNAMIC_ATTRS) == 0xf80000ffffff7fff, "");
	static_assert((STATIC_ATTRS & DYNAMIC_ATTRS) == 0x0000000000000000, "");
	
	CTFNavArea() {}
	virtual ~CTFNavArea() {}
	
	/* CNavArea overrides */
	virtual void OnServerActivate() override;
	virtual void OnRoundRestart() override;
	virtual void Save(CUtlBuffer& fileBuffer, unsigned int version) const override;
	virtual NavErrorType Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int subVersion) override;
	virtual void UpdateBlocked(bool force = false, int teamID = TEAM_ANY) override {}
	virtual bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;
	virtual void Draw() const override;
	virtual void CustomAnalysis(bool isIncremental = false) override {}
	virtual bool IsPotentiallyVisibleToTeam(int team) const override;
	
	void AddPotentiallyVisibleActor(CBaseCombatCharacter *actor);
	void RemovePotentiallyVisibleActor(CBaseCombatCharacter *actor);
	
	void CollectPriorIncursionAreas(int team, CUtlVector<CTFNavArea *> *areas);
	void CollectNextIncursionAreas(int team, CUtlVector<CTFNavArea *> *areas);
	CTFNavArea *GetNextIncursionArea(int team) const;
	float GetIncursionDistance(int team) const;
	void ResetIncursionDistances();
	bool IsReachableByTeam( int team ) const { return m_IncursionDistance[team] >= 0.0f; }
	
	void ComputeInvasionAreaVectors();
	bool IsAwayFromInvasionAreas(int team, float dist) const;
	const CUtlVector<CTFNavArea *>& GetInvasionAreas(int team) const;
	
	float GetCombatIntensity() const;
	bool IsInCombat() const { return (this->GetCombatIntensity() > 0.001f); }
	void OnCombat(int mult = 1);
	void AddCombatToSurroundingAreas(int mult = 1);

	float GetTravelDistanceToBombTarget( void ) const { return m_flBombTargetDistance; }
	
	bool IsValidForWanderingPopulation() const { return !this->HasAnyTFAttributes(RESCUE_CLOSET | NO_SPAWNING | BLUE_SPAWN_ROOM | RED_SPAWN_ROOM | BLOCKED); }
	
	static void MakeNewTFMarker() { ++s_MasterTFMark; }
	static void ResetTFMarker()   { s_MasterTFMark = 1; }
	
	void TFMark()           { this->m_TFMark = s_MasterTFMark; }
	bool IsTFMarked() const { return (this->m_TFMark == s_MasterTFMark); }
	
	uint64 GetTFAttributes() const             { return this->m_nTFAttributes; }
	bool HasAnyTFAttributes(uint64 bits) const { return ((this->m_nTFAttributes & bits) != 0); }
	bool HasAllTFAttributes(uint64 bits) const { return ((this->m_nTFAttributes & bits) == bits); }
	void AddTFAttributes(uint64 bits)          { this->m_nTFAttributes |=  bits; }
	void RemoveTFAttributes(uint64 bits)       { this->m_nTFAttributes &= ~bits; }
	
	uint64 GetDynamicTFAttributes() const { return (this->m_nTFAttributes & DYNAMIC_ATTRS); }
	uint64 GetStaticTFAttributes() const  { return (this->m_nTFAttributes &  STATIC_ATTRS); }
	void ClearDynamicTFAttributes()       { this->m_nTFAttributes &= ~DYNAMIC_ATTRS; }
	void ClearStaticTFAttributes()        { this->m_nTFAttributes &=  ~STATIC_ATTRS; }
	
	bool HasAnySpawnRoom() const    { return this->HasAnyTFAttributes(ANY_SPAWN_ROOM);    }
	bool HasAnySentry() const       { return this->HasAnyTFAttributes(ANY_SENTRY);        }
	bool HasAnySetupGate() const    { return this->HasAnyTFAttributes(ANY_SETUP_GATE);    }
	bool HasAnyOneWayDoor() const   { return this->HasAnyTFAttributes(ANY_ONE_WAY_DOOR);  }
	bool HasAnyRRVisualizer() const { return this->HasAnyTFAttributes(ANY_RR_VISUALIZER); }
	
	bool HasFriendlySpawnRoom(int team) const    { return this->HasAnyTFAttributes(GetAttr_SpawnRoom   (team)); }
	bool HasFriendlySentry(int team) const       { return this->HasAnyTFAttributes(GetAttr_Sentry      (team)); }
	bool HasFriendlySetupGate(int team) const    { return this->HasAnyTFAttributes(GetAttr_SetupGate   (team)); }
	bool HasFriendlyOneWayDoor(int team) const   { return this->HasAnyTFAttributes(GetAttr_OneWayDoor  (team)); }
	bool HasFriendlyRRVisualizer(int team) const { return this->HasAnyTFAttributes(GetAttr_RRVisualizer(team)); }
	
	bool HasEnemySpawnRoom(int team) const    { return this->HasAnyTFAttributes(GetInvAttr_SpawnRoom   (team)); }
	bool HasEnemySentry(int team) const       { return this->HasAnyTFAttributes(GetInvAttr_Sentry      (team)); }
	bool HasEnemySetupGate(int team) const    { return this->HasAnyTFAttributes(GetInvAttr_SetupGate   (team)); }
	bool HasEnemyOneWayDoor(int team) const   { return this->HasAnyTFAttributes(GetInvAttr_OneWayDoor  (team)); }
	bool HasEnemyRRVisualizer(int team) const { return this->HasAnyTFAttributes(GetInvAttr_RRVisualizer(team)); }
	
	bool HasFriendlySpawnRoom(const CBasePlayer *player) const    { return this->HasFriendlySpawnRoom   (player->GetTeamNumber()); }
	bool HasFriendlySentry(const CBasePlayer *player) const       { return this->HasFriendlySentry      (player->GetTeamNumber()); }
	bool HasFriendlySetupGate(const CBasePlayer *player) const    { return this->HasFriendlySetupGate   (player->GetTeamNumber()); }
	bool HasFriendlyOneWayDoor(const CBasePlayer *player) const   { return this->HasFriendlyOneWayDoor  (player->GetTeamNumber()); }
	bool HasFriendlyRRVisualizer(const CBasePlayer *player) const { return this->HasFriendlyRRVisualizer(player->GetTeamNumber()); }
	
	bool HasEnemySpawnRoom(const CBasePlayer *player) const    { return this->HasEnemySpawnRoom   (player->GetTeamNumber()); }
	bool HasEnemySentry(const CBasePlayer *player) const       { return this->HasEnemySentry      (player->GetTeamNumber()); }
	bool HasEnemySetupGate(const CBasePlayer *player) const    { return this->HasEnemySetupGate   (player->GetTeamNumber()); }
	bool HasEnemyOneWayDoor(const CBasePlayer *player) const   { return this->HasEnemyOneWayDoor  (player->GetTeamNumber()); }
	bool HasEnemyRRVisualizer(const CBasePlayer *player) const { return this->HasEnemyRRVisualizer(player->GetTeamNumber()); }
	
	static uint64 GetAttr_SpawnRoom(int team);
	static uint64 GetAttr_Sentry(int team);
	static uint64 GetAttr_SetupGate(int team);
	static uint64 GetAttr_OneWayDoor(int team);
	static uint64 GetAttr_RRVisualizer(int team);
	
	static uint64 GetInvAttr_SpawnRoom(int team)    { return GetAttr_SpawnRoom   (team) ^ ANY_SPAWN_ROOM;    }
	static uint64 GetInvAttr_Sentry(int team)       { return GetAttr_Sentry      (team) ^ ANY_SENTRY;        }
	static uint64 GetInvAttr_SetupGate(int team)    { return GetAttr_SetupGate   (team) ^ ANY_SETUP_GATE;    }
	static uint64 GetInvAttr_OneWayDoor(int team)   { return GetAttr_OneWayDoor  (team) ^ ANY_ONE_WAY_DOOR;  }
	static uint64 GetInvAttr_RRVisualizer(int team) { return GetAttr_RRVisualizer(team) ^ ANY_RR_VISUALIZER; }
	
	// TODO: are any of the non-virtual funcs in this class private?
	
	static CTFNavArea *PopOpenList() { return static_cast<CTFNavArea *>(CNavArea::PopOpenList()); }
	
	CTFNavArea *GetAdjacentTFArea(NavDirType dir, int i) const { return static_cast<CTFNavArea *>(this->GetAdjacentArea      (dir, i)); }
	CTFNavArea *GetRandomAdjacentTFArea(NavDirType dir) const  { return static_cast<CTFNavArea *>(this->GetRandomAdjacentArea(dir   )); }
	CTFNavArea *GetTFParent() const                            { return static_cast<CTFNavArea *>(this->GetParent            (      )); }
	
	const TFNavConnectVector *GetAdjacentTFAreas(NavDirType dir) const       { return reinterpret_cast<const TFNavConnectVector *>(this->GetAdjacentAreas      (dir)); }
	const TFNavConnectVector *GetIncomingTFConnections(NavDirType dir) const { return reinterpret_cast<const TFNavConnectVector *>(this->GetIncomingConnections(dir)); }
	const TFNavConnectVector& GetTFElevatorAreas() const                     { return reinterpret_cast<const TFNavConnectVector& >(this->GetElevatorAreas      (   )); }
	
	template<typename Functor> bool ForAllPotentiallyVisibleTFAreas(Functor&& func) { return this->ForAllPotentiallyVisibleAreas([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }); }
	template<typename Functor> bool ForAllCompletelyVisibleTFAreas (Functor&& func) { return this->ForAllCompletelyVisibleAreas ([&](CNavArea *area){ return func(static_cast<CTFNavArea *>(area)); }); }

	// ----------------------------------------------------------------------------
	// VScript accessors
	// ----------------------------------------------------------------------------
	virtual ScriptClassDesc_t *GetScriptDesc( void );

	void SetAttributeTF( int bits ) { m_nTFAttributes |= bits; }
	bool HasAttributeTF( int bits ) { return ( m_nTFAttributes & bits ) != 0; }
	void ClearAttributeTF( int bits ) { m_nTFAttributes &= ~bits; }

	Vector FindRandomSpot( void );
	bool IsBottleneck( void );
	
private:
	void ClearAllPVActors();
	void ClearAllInvasionAreas();
	
	float m_IncursionDistance[TF_TEAM_COUNT];
	CUtlVector<CTFNavArea *> m_InvasionAreas[TF_TEAM_COUNT];
	unsigned int m_InvasionAreaMarker = 0xffffffff;
	uint64 m_nTFAttributes = 0;
	CUtlVector<CHandle<CBaseCombatCharacter>> m_PVActors[TF_TEAM_COUNT];
	float m_flCombatLevel = 0.0f;
	IntervalTimer m_itCombatTime;
	unsigned int m_TFMark = 0;
	float m_flBombTargetDistance = 0.0f;
	
	static unsigned int s_MasterTFMark;
};


struct TFNavConnect : public NavConnect
{
	CTFNavArea *GetTFArea() const { return static_cast<CTFNavArea *>(const_cast<CNavArea *>(this->area)); }
};
static_assert(sizeof(TFNavConnect) == sizeof(NavConnect), "");


using TFNavAreaVector = CUtlVector<CTFNavArea *>;
static_assert(sizeof(TFNavAreaVector) == sizeof(NavAreaVector), "");


#define TheTFNavAreas (*reinterpret_cast<TFNavAreaVector *>(&TheNavAreas))


#undef BIT


#endif
