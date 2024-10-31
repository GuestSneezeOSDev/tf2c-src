/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_nav_mesh.h"
#include "NextBotManager.h"
#include "NextBotUtil.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "func_respawnroom.h"
#include "entity_tfstart.h"
#include "nav_pathfind.h"
#include "vprof.h"
#include "tf_bot_manager.h"
#include "filters.h"
#include "tf_control_point_master.h"
#include "entity_ammopack.h"
#include "entity_healthkit.h"


static ConVar tf_show_in_combat_areas                       ("tf_show_in_combat_areas",                             "0", FCVAR_CHEAT);
static ConVar tf_show_enemy_invasion_areas                  ("tf_show_enemy_invasion_areas",                        "0", FCVAR_CHEAT, "Highlight areas where the enemy team enters the visible environment of the local player");
static ConVar tf_show_blocked_areas                         ("tf_show_blocked_areas",                               "0", FCVAR_CHEAT, "Highlight areas that are considered blocked for TF-specific reasons");
static ConVar tf_show_incursion_flow                        ("tf_show_incursion_flow",                              "0", FCVAR_CHEAT, "1 = red, 2 = blue, 3 = green, 4 = yellow");
static ConVar tf_show_incursion_flow_range                  ("tf_show_incursion_flow_range",                      "150", FCVAR_CHEAT);
static ConVar tf_show_incursion_flow_gradient               ("tf_show_incursion_flow_gradient",                     "0", FCVAR_CHEAT, "1 = red, 2 = blue, 3 = green, 4 = yellow");
static ConVar tf_show_mesh_decoration                       ("tf_show_mesh_decoration",                             "0", FCVAR_CHEAT, "Highlight special areas");
static ConVar tf_show_mesh_decoration_manual                ("tf_show_mesh_decoration_manual",                      "0", FCVAR_CHEAT, "Highlight special areas marked by hand");
static ConVar tf_show_sentry_danger                         ("tf_show_sentry_danger",                               "0", FCVAR_CHEAT, "Show sentry danger areas. 1:Use m_SentryAreas. 2:Check all nav areas.");
static ConVar tf_show_actor_potential_visibility            ("tf_show_actor_potential_visibility",                  "0", FCVAR_CHEAT);
static ConVar tf_show_control_points                        ("tf_show_control_points",                              "0", FCVAR_CHEAT);
static ConVar tf_show_bomb_drop_areas                       ("tf_show_bomb_drop_areas",                             "0", FCVAR_CHEAT);
static ConVar tf_bot_min_setup_gate_defend_range            ("tf_bot_min_setup_gate_defend_range",                "750", FCVAR_CHEAT, "How close from the setup gate(s) defending bots can take up positions. Areas closer than this will be in cover to ambush.");
static ConVar tf_bot_max_setup_gate_defend_range            ("tf_bot_max_setup_gate_defend_range",               "2000", FCVAR_CHEAT, "How far from the setup gate(s) defending bots can take up positions");
static ConVar tf_bot_min_setup_gate_sniper_defend_range     ("tf_bot_min_setup_gate_sniper_defend_range",        "1500", FCVAR_CHEAT, "How far from the setup gate(s) a defending sniper will take up position");
static ConVar tf_show_gate_defense_areas                    ("tf_show_gate_defense_areas",                          "0", FCVAR_CHEAT);
static ConVar tf_show_point_defense_areas                   ("tf_show_point_defense_areas",                         "0", FCVAR_CHEAT);
static ConVar tf_select_ambush_areas_radius                 ("tf_select_ambush_areas_radius",                     "750", FCVAR_CHEAT);
static ConVar tf_select_ambush_areas_close_range            ("tf_select_ambush_areas_close_range",                "300", FCVAR_CHEAT);
static ConVar tf_select_ambush_areas_max_enemy_exposure_area("tf_select_ambush_areas_max_enemy_exposure_area", "500000", FCVAR_CHEAT);

#ifdef STAGING_ONLY
static ConVar tf_nav_mesh_always_update("tf_nav_mesh_always_update", "0", FCVAR_CHEAT, "For testing: always run CTFNavMesh::Update and CTFNavMesh::OnBlockedAreasChanged, regardless of whether TheNextBots().GetBotCount() is zero.");
static inline bool TFNavMesh_ShouldAlwaysUpdate() { return tf_nav_mesh_always_update.GetBool(); }
#else
static inline bool TFNavMesh_ShouldAlwaysUpdate() { return false; }
#endif


class CCollectAndLabelSpawnRoomAreas
{
public:
	CCollectAndLabelSpawnRoomAreas(CFuncRespawnRoom *room, CTFTeamSpawn *spawn) :
		m_pRoom(room), m_pSpawn(spawn) {}
	
	bool operator()(CTFNavArea *area)
	{
		// TODO: verify that StepHeight (18.0f from nav.h) is valid for TF
		static Vector stepHeight(0.0f, 0.0f, StepHeight);
		
		if (this->m_pRoom->PointIsWithin(area->GetCenter()           + stepHeight) ||
			this->m_pRoom->PointIsWithin(area->GetCorner(NORTH_WEST) + stepHeight) ||
			this->m_pRoom->PointIsWithin(area->GetCorner(NORTH_EAST) + stepHeight) ||
			this->m_pRoom->PointIsWithin(area->GetCorner(SOUTH_WEST) + stepHeight) ||
			this->m_pRoom->PointIsWithin(area->GetCorner(SOUTH_EAST) + stepHeight)) {
			TheTFNavMesh->MarkSpawnRoomArea(area, this->m_pSpawn->GetTeamNumber());
		}
		
		return true;
	}
	
private:
	CFuncRespawnRoom *m_pRoom;
	CTFTeamSpawn *m_pSpawn;
};


class ScanSelectAmbushAreas
{
public:
	// TODO: name for f1
	ScanSelectAmbushAreas(int team, float f1, CUtlVector<CTFNavArea *> *areas) :
		m_iTeam(team), m_flUnknown(f1), m_pAreas(areas) {}
	
	bool operator()(CNavArea *area)
	{
		CNavArea *parent = area->GetParent();
		if (parent != nullptr && !parent->IsContiguous(area)) return false;
		
		if (static_cast<CTFNavArea *>(area)->GetIncursionDistance(this->m_iTeam) > this->m_flUnknown) return false;
		
		int unconnected_dirs = 0;
		if (area->GetAdjacentAreas(NORTH)->Count() == 0) ++unconnected_dirs;
		if (area->GetAdjacentAreas(EAST )->Count() == 0) ++unconnected_dirs;
		if (area->GetAdjacentAreas(SOUTH)->Count() == 0) ++unconnected_dirs;
		if (area->GetAdjacentAreas(WEST )->Count() == 0) ++unconnected_dirs;
		if (unconnected_dirs < 1) return true;
		
		
		// TODO
		Assert(false);
		
		// this is non-inlined in ServerWin 20151007a
		
		// REMOVE ME
		return true;
	}
	
private:
	int m_iTeam;
	float m_flUnknown; // TODO: name for m_flUnknown
	CUtlVector<CTFNavArea *> *m_pAreas;
	
	// TODO: probably has static member vars
};


class DrawIncursionFlow
{
public:
	bool operator()(CNavArea *area)
	{
		int team = tf_show_incursion_flow.GetInt() + 1;
		if (!TFTeamMgr()->IsValidTeam(team)) return true;
		
		float area_incdist = static_cast<CTFNavArea *>(area)->GetIncursionDistance(team);
		
		for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
			int adj_count = area->GetAdjacentCount((NavDirType)dir);
			for (int i = 0; i < adj_count; ++i) {
				auto conn_area = static_cast<CTFNavArea *>(area)->GetAdjacentTFArea((NavDirType)dir, i);
				
				// TODO: find source of magic number 45.0f
				if (area->ComputeAdjacentConnectionHeightChange(conn_area) <= 45.0f) {
					float conn_incdist = conn_area->GetIncursionDistance(team);
					
					if (conn_incdist > area_incdist) {
						float c = 2.0f * fmod(conn_incdist - (gpGlobals->curtime * 0.333f * 2500.0f), 2500.0f) / 2500.0f;
						if (c > 1.0f) c = 2.0f - c;
						c *= 255.0f;
						
						int r = 0xff;
						int g = 0xff;
						int b = 0xff;
						
						switch (team) {
						case TF_TEAM_RED:    r =    c; g = 0x00; b = 0x00; break;
						case TF_TEAM_BLUE:   r = 0x00; g = 0x00; b =    c; break;
						case TF_TEAM_GREEN:  r = 0x00; g =    c; b = 0x00; break;
						case TF_TEAM_YELLOW: r =    c; g =    c; b = 0x00; break;
						}
						
						NDebugOverlay::HorzArrow(area->GetCenter(), conn_area->GetCenter(), 5.0f, r, g, b, 0xff, true, NDEBUG_PERSIST_TILL_NEXT_SERVER);
					}
				}
			}
		}
		
		return true;
	}
};


static void TestAndBlockOverlappingAreas(CBaseEntity *ent)
{
	NextBotTraceFilterIgnoreActors filter;
	
	Extent ext;
	ext.Init(ent);
	
	CUtlVector<CTFNavArea *> areas;
	TheTFNavMesh->CollectAreasOverlappingExtent(ext, &areas);
	
	// TODO: verify everything below this point with the SSE code from ServerLinux 20151007a
	
	for (auto area : areas) {
		Vector vNW = area->GetCorner(NORTH_WEST);
		// vNW.x = m_nwCorner.x
		// vNW.y = m_nwCorner.y
		// vNW.z = m_nwCorner.z
		
		Vector vNE = area->GetCorner(NORTH_EAST);
		// vNE.x = m_seCorner.x
		// vNE.y = m_nwCorner.y
		// vNE.z = m_neZ
		
		Vector vSW = area->GetCorner(SOUTH_WEST);
		// vSW.x = m_nwCorner.x
		// vSW.y = m_seCorner.y
		// vSW.z = m_swZ
		
		Vector vSE = area->GetCorner(SOUTH_EAST);
		// vSE.x = m_seCorner.x
		// vSE.y = m_seCorner.y
		// vSE.z = m_seCorner.z

		const Vector vecMins( 0, 0, StepHeight );

		Vector vecStart, vecEnd, vecMaxs, vecTest;
		if ( abs( vNW.z - vNE.z ) >= 1.0f )
		{
			if ( abs( vSE.z - vSW.z ) >= 1.0f )
			{
				vecTest = vSE;
				vecMaxs.x = 1.0f;
				vecMaxs.y = 1.0f;
			}
			else
			{
				vecTest = vNE;
				vecMaxs.x = 0.0f;
				vecMaxs.y = vSE.y - vNE.y;
			}
		}
		else
		{
			vecTest = vSW;
			vecMaxs.x = vSE.x - vNW.x;
			vecMaxs.y = 0.0f;
		}
		vecStart = vNW;
		if ( vNW.z >= vecTest.z )
		{
			vecEnd = vecTest;
		}
		else
		{
			vecEnd = vNW;
			vecStart = vecTest;
		}

		vecMaxs.z = HalfHumanHeight;

		Ray_t ray;
		ray.Init( vecStart, vecEnd, vecMins, vecMaxs );

		trace_t tr;
		enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );
		
		if (tr.DidHit() && tr.m_pEnt != nullptr && tr.m_pEnt->ShouldBlockNav()) {
			area->MarkAsBlocked(TEAM_ANY, ent);
		}
	}
	
#if 0
	if (abs(vNW.z - vNE.z) < 1.0f) {
		maxs.x = vNE.x;
		maxs.y = vNE.y;
		maxs.z = 30.0f; // TODO: magic number
		
		mins.x = vNW.x;
		mins.y = vNW.y;
		mins.z = 18.0f; // TODO: magic number
		
		// maxs - mins: either
		// SE   - SW
		// NE   - NW  <---
		
		// m_Extents.x = (v?E.x - v?W.x) / 2
		// m_Extents.y = 0
		
		// m_StartOffset.x = -(v?E.x - v?W.x) / 2
		// m_StartOffset.y = -0
		
		
		// VERIFIED
		if (vSW.z > vNW.z) {
			start = vSW;
			end   = vNW;
		} else {
			start = vNW;
			end   = vSW;
		}
		
		ray.Init(start, end, mins, maxs);
	} else if (abs(vNW.z - vSW.z) < 1.0f) {
		maxs.x = vSW.x;
		maxs.y = vSW.y;
		maxs.z = 30.0f; // TODO: magic number
		
		mins.x = vNW.x;
		mins.y = vNW.y;
		mins.z = 18.0f; // TODO: magic number
		
		// maxs - mins: either
		// SE   - NE
		// SW   - NW  <---
		
		// m_Extents.x = 0
		// m_Extents.y = (vS?.y - vN?.y) / 2
		
		// m_StartOffset.x = -0
		// m_StartOffset.y = -(vS?.y - vN?.y) / 2
		
		
		// VERIFIED
		if (vNE.z > vNW.z) {
			start = vNE;
			end   = vNW;
		} else {
			start = vNW;
			end   = vNE;
		}
		
		ray.Init(start, end, mins, maxs);
	} else {
		// this seems like it's probably a degenerate case ...?
		
		maxs.x = 0.0f;
		maxs.y = 0.0f;
		maxs.z = 30.0f; // TODO: magic number
		
		mins.x = -1.0f;
		mins.y = -1.0f;
		mins.z = 18.0f; // TODO: magic number
		
		
		// VERIFIED
		if (vSE.z > vNW.z) {
			start = vSE;
			end   = vNW;
		} else {
			start = vNW;
			end   = vSE;
		}
		
		ray.Init(start, end, mins, maxs);
	}
	
	// always: (maxs.z + mins.z) / 2 = 24.0f
	// always: (maxs.z - mins.z) / 2 =  6.0f
	// => maxs.z = 30.0f
	// => mins.z = 18.0f
	
	
	// m_Delta       = end - start
	// m_Extents     =         (maxs - mins) / 2
	// m_StartOffset =        -(maxs + mins) / 2
	// m_Start       = start + (maxs + mins) / 2
#endif
}


CTFNavMesh::CTFNavMesh()
{
	this->ListenForGameEvent("teamplay_setup_finished");
	this->ListenForGameEvent("arena_round_start");
	this->ListenForGameEvent("teamplay_point_captured");
	this->ListenForGameEvent("teamplay_point_unlocked");
	this->ListenForGameEvent("teamplay_round_win");
	this->ListenForGameEvent("player_builtobject");
	this->ListenForGameEvent("player_dropobject");
	this->ListenForGameEvent("player_carryobject");
	this->ListenForGameEvent("object_detonated");
	this->ListenForGameEvent("object_destroyed");

	this->m_iControlPoint = 0;
	
	// TODO: check that all of these events have the same semantics in TF2C as in TF2
	// (and that they all actually exist etc)
}


void CTFNavMesh::FireGameEvent(IGameEvent *event)
{
	CNavMesh::FireGameEvent(event);
	
	const char *name = event->GetName();
	
	if (FStrEq(name, "teamplay_point_captured")) {
		this->ScheduleRecompute(RECOMPUTE_POINT_CAPTURED, event->GetInt("cp"));
	} else if (FStrEq(name, "teamplay_setup_finished") || FStrEq(name, "arena_round_start")) {
		this->ScheduleRecompute(RECOMPUTE_SETUP_FINISHED);
	} else if (FStrEq(name, "teamplay_point_unlocked")) {
		this->ScheduleRecompute(RECOMPUTE_POINT_UNLOCKED, event->GetInt("cp"));
	} else if (FStrEq(name, "teamplay_round_win")) {
		this->ScheduleRecompute(RECOMPUTE_ROUND_WIN);
	} else if (FStrEq(name, "player_builtobject") || FStrEq(name, "player_carryobject") || FStrEq(name, "object_detonated") || FStrEq(name, "object_destroyed")) {
		int obj_type;
		if (!event->IsEmpty("objecttype")) {
			obj_type = event->GetInt("objecttype");
		} else {
			obj_type = event->GetInt("object");
		}
		
		if (obj_type == OBJ_SENTRYGUN) {
			if (tf_show_sentry_danger.GetInt() != 0) {
				DevMsg("%s: Got sentrygun %s event\n", __FUNCTION__, name);
			}
			
			this->OnObjectChanged();
		}
	}
	
	// BUG: we listen for "player_dropobject" but then don't do anything with it here
}

NavErrorType CTFNavMesh::Load()
{
	return CNavMesh::Load();
}

void CTFNavMesh::Update()
{
	CNavMesh::Update();
	
	if (!TheNavAreas.IsEmpty()) {
		this->UpdateDebugDisplay();
		
		int bot_count = TheNextBots().GetBotCount();
		if (bot_count == 0 && TFNavMesh_ShouldAlwaysUpdate()) {
			bot_count = 1;
		}
		
		if (bot_count != 0) {
			if (this->m_LastNextBotCount == 0) {
				this->ScheduleRecompute(RECOMPUTE_DEFAULT);
			}
			
			if (this->m_ctRecompute.HasStarted() && this->m_ctRecompute.IsElapsed()) {
				this->m_ctRecompute.Invalidate();
				this->RecomputeInternalData();
			}
		}
		
		this->m_LastNextBotCount = bot_count;
	}
}

void CTFNavMesh::SaveCustomData(CUtlBuffer& fileBuffer) const
{
}

NavErrorType CTFNavMesh::LoadCustomData(CUtlBuffer& fileBuffer, unsigned int subVersion)
{
	return NAV_OK;
}

void CTFNavMesh::SaveCustomDataPreArea(CUtlBuffer& fileBuffer) const
{
	fileBuffer.PutUnsignedInt(TF2C_MAGIC);
}

NavErrorType CTFNavMesh::LoadCustomDataPreArea(CUtlBuffer& fileBuffer, unsigned int subVersion)
{
	unsigned int magic = fileBuffer.GetUnsignedInt();
	if (!fileBuffer.IsValid()) {
		Warning("Can't read nav file magic number!\n");
		return NAV_INVALID_FILE;
	}
	
	if (magic != TF2C_MAGIC) {
		Warning("This nav file was not built for TF2Classic!\n"
			"(If you are trying to use a nav file from live TF2, you need to convert it with tf2c_convert_nav_file.)\n");
		return NAV_INVALID_FILE;
	}
	
	if (subVersion < SUB_VERSION_MIN_VALID) {
		Warning("The NavMesh sub-version number is invalid!\n[Game version: %04x] [File version: %04x]\n",
			this->GetSubVersionNumber(), subVersion);
		return NAV_BAD_FILE_VERSION;
	}
	if (subVersion < SUB_VERSION_MIN_COMPAT) {
		Warning("The NavMesh sub-version number is too old for this game version!\n[Game version: %04x] [File version: %04x]\n",
			this->GetSubVersionNumber(), subVersion);
		return NAV_BAD_FILE_VERSION;
	}
	if (subVersion > this->GetSubVersionNumber()) {
		Warning("The NavMesh sub-version number is too new for this game version!\n[Game version: %04x] [File version: %04x]\n",
			this->GetSubVersionNumber(), subVersion);
		return NAV_BAD_FILE_VERSION;
	}
	
	return NAV_OK;
}


void CTFNavMesh::OnServerActivate()
{
	CNavMesh::OnServerActivate();
	
	this->m_SentryAreas.RemoveAll();
	this->ResetMeshAttributes(true);
	
	this->m_LastNextBotCount = 0;
	// TODO: CUtlVector @ +0x5f4 .RemoveAll()
	
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_SpawnRooms       [i].RemoveAll();
		this->m_SpawnExits       [i].RemoveAll();
		this->m_RRVisualizerAreas[i].RemoveAll();
	}
	
	for (int i = 0; i < MAX_CONTROL_POINTS; ++i) {
		this->m_ControlPointAreas [i].RemoveAll();
		this->m_ControlPointCenter[i] = nullptr;
	}
}

void CTFNavMesh::OnRoundRestart()
{
	CNavMesh::OnRoundRestart();
	
	this->ResetMeshAttributes(true);
	
	TheTFBots().OnRoundRestart();
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode()) {
		this->RecomputeInternalData();
	}
	
	DevMsg("CTFNavMesh: %d nav areas in mesh.\n", this->GetNavAreaCount());
}


// TODO: name for f2 (also in header)
void CTFNavMesh::CollectAmbushAreas(CUtlVector<CTFNavArea *> *areas, CTFNavArea *area, int team, float maxRange, float f2) const
{
	ScanSelectAmbushAreas functor(team, f2 + area->GetIncursionDistance(team), areas);
	SearchSurroundingAreas(area, area->GetCenter(), functor, maxRange);
}

void CTFNavMesh::CollectAreaWithinBombTravelRange(CUtlVector<CTFNavArea *> *areas, float f1, float f2) const
{
	// TODO (MvM stuff)
	Assert(false);
}

void CTFNavMesh::CollectBuiltObjects(CUtlVector<CBaseObject *> *objects, int team)
{
	objects->RemoveAll();
	
	for (auto obj : CBaseObject::AutoList()) {
		if (team == TEAM_ANY || team == obj->GetTeamNumber()) {
			objects->AddToTail(obj);
		}
	}
}

// NOTE: this function works differently from live TF2's CTFNavMesh::CollectSpawnRoomThresholdAreas;
// in particular, we actually try to get the areas at the threshold of the respawn room visualizer,
// which sometimes extends a fair distance past the actual edge of the respawn room brush itself
void CTFNavMesh::CollectEnemySpawnRoomThresholdAreas(CUtlVector<CTFNavArea *> *areas, int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	CUtlVector<CTFNavArea *> threshold_areas;
	
	// get all the nav areas with rr visualizers belonging to each enemy team
	ForEachEnemyTFTeam(team, [&](int enemy_team){
		for (auto enemy_rrv_area : this->m_RRVisualizerAreas[enemy_team]) {
			// get all adjacent areas in each direction from the areas containing rr visualizers
			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
				const TFNavConnectVector& connections = *enemy_rrv_area->GetAdjacentTFAreas((NavDirType)dir);
				for (auto& connection : connections) {
					auto conn_area = connection.GetTFArea();
					
					// ignore adjacent areas that contain rr visualizers of the enemy team or which are in their spawn room
					if (conn_area->HasEnemyRRVisualizer(team)) continue;
					if (conn_area->HasEnemySpawnRoom(team))    continue;
					
					// only consider adjacent areas whose enemy-team incursion distance is greater than the rr visualizer area
					if (conn_area->GetIncursionDistance(enemy_team) > enemy_rrv_area->GetIncursionDistance(enemy_team)) {
						if (!threshold_areas.HasElement(conn_area)) {
							threshold_areas.AddToTail(conn_area);
						}
					}
				}
			}
		}
		
		return true;
	});
	
	areas->AddVectorToTail(threshold_areas);
}

const CUtlVector<CTFNavArea *> *CTFNavMesh::GetControlPointAreas(int idx) const
{
	Assert(idx >= 0 && idx < MAX_CONTROL_POINTS);
	if (idx < 0 || idx >= MAX_CONTROL_POINTS) return nullptr;
	
	return &this->m_ControlPointAreas[idx];
}

CTFNavArea *CTFNavMesh::GetControlPointCenterArea(int idx) const
{
	Assert(idx >= 0 && idx < MAX_CONTROL_POINTS);
	if (idx < 0 || idx >= MAX_CONTROL_POINTS) return nullptr;
	
	return this->m_ControlPointCenter[idx];
}

CTFNavArea *CTFNavMesh::GetControlPointRandomArea(int idx) const
{
	Assert(idx >= 0 && idx < MAX_CONTROL_POINTS);
	if (idx < 0 || idx >= MAX_CONTROL_POINTS) return nullptr;
	
	if (!this->m_ControlPointAreas[idx].IsEmpty()) {
		return this->m_ControlPointAreas[idx].Random();
	} else {
		return nullptr;
	}
}

bool CTFNavMesh::IsSentryGunHere(CTFNavArea *area) const
{
	if (area->HasAnyTFAttributes(CTFNavArea::ANY_SENTRY)) {
		for (auto obj : CBaseObject::AutoList()) {
			if (obj->IsSentry() && this->GetNearestTFNavArea(obj) == area) {
				return true;
			}
		}
	}
	
	return false;
}

void CTFNavMesh::MarkSpawnRoomArea(CTFNavArea *area, int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	area->AddTFAttributes(CTFNavArea::GetAttr_SpawnRoom(team));
	this->m_SpawnRooms[team].AddToTail(area);
}

void CTFNavMesh::OnBlockedAreasChanged()
{
	VPROF_BUDGET("CTFNavMesh::OnBlockedAreasChanged", "NextBot");
	
	if (TheNextBots().GetBotCount() != 0 || TFNavMesh_ShouldAlwaysUpdate()) {
		this->ScheduleRecompute(RECOMPUTE_BLOCKED_AREAS_CHANGED);
	}
}

CTFNavArea *CTFNavMesh::ChooseWeightedRandomArea(const CUtlVector<CTFNavArea *> *areas) const
{
	Assert(areas != nullptr);
	if (areas == nullptr) return nullptr;
	
	float size = 0.0f;
	for (auto area : *areas) {
		size += (area->GetSizeX() * area->GetSizeY());
	}
	
	float rand_size = RandomFloat(0.0f, size - 1.0f);
	for (auto area : *areas) {
		rand_size -= (area->GetSizeX() * area->GetSizeY());
		
		if (rand_size <= 0.0f) {
			return area;
		}
	}
	
	return nullptr;
}


void CTFNavMesh::RecomputeBlockers()
{
	this->ScheduleRecompute(RECOMPUTE_BLOCKERS_INTERFACE);
}


const CUtlVector<CTFNavArea *> *CTFNavMesh::GetSpawnRoomAreas(int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return nullptr;
	
	return &this->m_SpawnRooms[team];
}

const CUtlVector<CTFNavArea *> *CTFNavMesh::GetSpawnExitAreas(int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return nullptr;
	
	return &this->m_SpawnExits[team];
}

const CUtlVector<CTFNavArea *> *CTFNavMesh::GetRRVisualizerAreas(int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return nullptr;
	
	return &this->m_RRVisualizerAreas[team];
}


void CTFNavMesh::CollectSpawnExitAreas( CUtlVector<CTFNavArea *> *areas, int team ) const
{
	Assert( team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT );
	if ( team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT ) return;

	areas->AddVectorToTail( this->m_SpawnExits[team] );
	return;
}


void CTFNavMesh::CollectEnemySpawnRoomAreas(CUtlVector<CTFNavArea *> *areas, int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	ForEachEnemyTFTeam(team, [=](int enemy_team){
		areas->AddVectorToTail(this->m_SpawnRooms[enemy_team]);
		return true;
	});
}

void CTFNavMesh::CollectEnemySpawnExitAreas(CUtlVector<CTFNavArea *> *areas, int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	ForEachEnemyTFTeam(team, [=](int enemy_team){
		areas->AddVectorToTail(this->m_SpawnExits[enemy_team]);
		return true;
	});
}

void CTFNavMesh::CollectEnemyRRVisualizerAreas(CUtlVector<CTFNavArea *> *areas, int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	ForEachEnemyTFTeam(team, [=](int enemy_team){
		areas->AddVectorToTail(this->m_RRVisualizerAreas[enemy_team]);
		return true;
	});
}


void CTFNavMesh::CollectAndMarkSpawnRoomExits(CTFNavArea *area, CUtlVector<CTFNavArea *> *areas)
{
	for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
		int adj_count = area->GetAdjacentCount((NavDirType)dir);
		for (int i = 0; i < adj_count; ++i) {
			auto conn_area = area->GetAdjacentTFArea((NavDirType)dir, i);
			
			if (!conn_area->HasAnyTFAttributes(CTFNavArea::ANY_SPAWN_ROOM)) {
				area->AddTFAttributes(CTFNavArea::SPAWN_ROOM_EXIT);
				areas->AddToTail(area);
				return;
			}
		}
	}
}

void CTFNavMesh::ComputeBlockedAreas()
{
	for (auto area : TheNavAreas) {
		area->UnblockArea();
	}
	
	FOR_EACH_ENT_BY_CLASSNAME("func_brush", CBaseEntity, brush) {
		// Defining func_brush left on Toggle setting as unable to block areas.
		// This fixes stairs on cp_foundry
		if (brush->IsSolid() && !IsEntityWalkable(brush, WALK_THRU_TOGGLE_BRUSHES)) {
			TestAndBlockOverlappingAreas(brush);
		}
	}
	
	FOR_EACH_ENT_BY_CLASSNAME("func_door*", CBaseDoor, door) {
		int filter_tfteam_teamnum = 0;
		bool door_overlaps_trigger = false;
		
		Extent ext_door;
		ext_door.Init(door);
		
		FOR_EACH_ENT_BY_CLASSNAME("trigger_multiple", CTriggerMultiple, trigger) {
			Extent ext_trigger;
			ext_trigger.Init(trigger);
			
			if (!ext_door.IsOverlapping(ext_trigger)) continue;
			if (trigger->m_bDisabled)                 continue;
			
			door_overlaps_trigger = true;
			
			if (!trigger->UsesFilter()) continue;
			
			CBaseFilter *filter = trigger->m_hFilter;
			if (filter->ClassMatches("filter_activator_tfteam")) {
				filter_tfteam_teamnum = filter->GetTeamNumber();
			}
		}
		
		bool door_closed = (door->m_toggle_state == TS_AT_BOTTOM || door->m_toggle_state == TS_GOING_DOWN);
		
		// TODO: instead of using collector, refactor this to use a lambda directly
		NavAreaCollector collector;
		TheTFNavMesh->ForAllAreasOverlappingExtent(collector, ext_door);
		
		for (auto area : collector.m_area) {
			auto tf_area = static_cast<CTFNavArea *>(area);
			
			bool block;
			if (tf_area->HasAnyTFAttributes(CTFNavArea::DOOR_ALWAYS_BLOCKS)) {
				block = !door_closed;
			} else {
				if (!door_closed || door_overlaps_trigger) {
					block = (filter_tfteam_teamnum == 0);
				} else {
					block = false;
				}
			}
			
			if (!block) {
				if (!tf_area->HasAnyTFAttributes(CTFNavArea::DOOR_NEVER_BLOCKS)) {
					ForEachEnemyTFTeam( filter_tfteam_teamnum, [=]( int enemy_team ){
						area->MarkAsBlocked( enemy_team, door );
						return true;
					} );	
				}
			} else {
				ForEachEnemyTFTeam( filter_tfteam_teamnum, [=]( int enemy_team ){
					area->UnblockArea( enemy_team );
					return true;
				} );
				
			}
		}
	}
}

void CTFNavMesh::ComputeBombTargetDistance()
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		// TODO (MvM stuff)
		Assert(false);
	}
}

void CTFNavMesh::ComputeControlPointAreas()
{
	for (int i = 0; i < MAX_CONTROL_POINTS; ++i) {
		this->m_ControlPointAreas [i].RemoveAll();
		this->m_ControlPointCenter[i] = nullptr;
	}
	
	if (g_hControlPointMasters.IsEmpty())     return;
	if (g_hControlPointMasters[0] == nullptr) return;
	
	FOR_EACH_ENT_BY_CLASSNAME("trigger_capture_area*", CTriggerAreaCapture, trigger) {
		if (trigger->GetControlPoint() == nullptr) continue;
		CTeamControlPoint *cp = trigger->GetControlPoint();
		
		Extent ext;
		ext.Init(trigger);
		
		ext.lo.z -= 35.5f; // TODO: magic number
		ext.hi.z += 35.5f; // TODO: magic number
		
		int idx = cp->GetPointIndex();
		
		CUtlVector<CTFNavArea *> *cp_areas = &this->m_ControlPointAreas[idx];
		TheTFNavMesh->CollectAreasOverlappingExtent(ext, cp_areas);
		
		this->m_ControlPointCenter[idx] = nullptr;
		
		float min_dist = FLT_MAX;
		for (auto area : *cp_areas) {
			float dist = area->GetCenter().DistTo(trigger->WorldSpaceCenter());
			
			if (min_dist > dist) {
				min_dist = dist;
				this->m_ControlPointCenter[idx] = area;
			}
		}
	}
}

void CTFNavMesh::ComputeIncursionDistances(CTFNavArea *startArea, int team)
{
	Assert(startArea != nullptr);
	if (startArea == nullptr) return;
	
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	CTFNavArea::ClearSearchLists();
	
	startArea->m_IncursionDistance[team] = 0.0f;
	startArea->AddToOpenList();
	startArea->SetParent(nullptr);
	startArea->Mark();
	
	CUtlVector<const TFNavConnect *, CUtlMemoryFixedGrowable<const TFNavConnect *, 64>> connections;
	
	while (!CTFNavArea::IsOpenListEmpty()) {
		CTFNavArea *area = CTFNavArea::PopOpenList();
		
		if (!TFGameRules()->IsMannVsMachineMode()) {
			if (!area->HasAnyTFAttributes(CTFNavArea::ANY_SETUP_GATE | CTFNavArea::SPAWN_ROOM_EXIT) && area->IsBlocked(team)) {
				continue;
			}
		}
		
		connections.RemoveAll();
		
		for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
			int adj_count = area->GetAdjacentCount((NavDirType)dir);
			for (int i = 0; i < adj_count; ++i) {
				connections.AddToTail(&(*area->GetAdjacentTFAreas((NavDirType)dir))[i]);
			}
		}
		
		for (const TFNavConnect *connect : connections) {
			auto conn_area = connect->GetTFArea();
			
			// TODO: find source of magic number 45.0f
			if (area->ComputeAdjacentConnectionHeightChange(conn_area) <= 45.0f) {
				float conn_incdist = conn_area->m_IncursionDistance[team];
				float area_incdist =      area->m_IncursionDistance[team];
				
				if (conn_incdist < 0.0f || conn_incdist > area_incdist + connect->length) {
					conn_area->m_IncursionDistance[team] = area_incdist + connect->length;
					
					conn_area->SetParent(nullptr);
					conn_area->Mark();
					
					if (!conn_area->IsOpen()) {
						conn_area->AddToOpenListTail();
					}
				}
			}
		}
	}
}

void CTFNavMesh::ComputeIncursionDistances()
{
	VPROF_BUDGET("CTFNavMesh::ComputeIncursionDistances", "NextBot");
	
	for (auto area : TheTFNavAreas) {
		area->ResetIncursionDistances();
	}
	
	bool red_ok = false;
	bool blu_ok = false;
	bool grn_ok = false;
	bool ylw_ok = false;

	for ( auto room : CFuncRespawnRoom::AutoList() ) {
		if ( !room->GetActive() ) continue;
		if ( room->m_bDisabled )  continue;

		for ( auto spawn : CTFTeamSpawn::AutoList() ) {
			if ( !spawn->IsTriggered( nullptr ) ) continue;
			if ( spawn->IsDisabled() )          continue;

			if ( spawn->GetTeamNumber() == TF_TEAM_RED && red_ok ) continue;
			if ( spawn->GetTeamNumber() == TF_TEAM_BLUE && blu_ok ) continue;
			if ( spawn->GetTeamNumber() == TF_TEAM_GREEN && grn_ok ) continue;
			if ( spawn->GetTeamNumber() == TF_TEAM_YELLOW && ylw_ok ) continue;

			if ( room->PointIsWithin( spawn->GetAbsOrigin() ) ) {
				CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea( spawn );
				if ( area != nullptr ) {
					this->ComputeIncursionDistances( area, spawn->GetTeamNumber() );

					if ( spawn->GetTeamNumber() == TF_TEAM_RED )    red_ok = true;
					if ( spawn->GetTeamNumber() == TF_TEAM_BLUE )   blu_ok = true;
					if ( spawn->GetTeamNumber() == TF_TEAM_GREEN )  grn_ok = true;
					if ( spawn->GetTeamNumber() == TF_TEAM_YELLOW ) ylw_ok = true;

					break;
				}
			}
		}
	}

	// TF2C Fall back to just spawns if no proper respawn room is set.
	// But I'll still show the warning below as mappers should fix it!
	for ( int i = 0; i < TF_TEAM_COUNT; ++i )
	{
		switch ( i )
		{
			case TF_TEAM_RED:
			{
				if ( !red_ok )
				{
					for ( auto area : this->m_SpawnRooms[i] )
					{
						this->ComputeIncursionDistances( area, i );
					}
				}
				break;
			}
			case TF_TEAM_BLUE:
			{
				if ( !blu_ok )
				{
					for ( auto area : this->m_SpawnRooms[i] )
					{
						this->ComputeIncursionDistances( area, i );
					}
				}
				break;
			}
			case TF_TEAM_GREEN:
			{
				if ( !grn_ok )
				{
					for ( auto area : this->m_SpawnRooms[i] )
					{
						this->ComputeIncursionDistances( area, i );
					}
				}
				break;
			}
			case TF_TEAM_YELLOW:
			{
				if ( !ylw_ok )
				{
					for ( auto area : this->m_SpawnRooms[i] )
					{
						this->ComputeIncursionDistances( area, i );
					}
				}
				break;
			}
			default:
				break;
		}
	}

	if ( TFTeamMgr()->IsValidTeam( TF_TEAM_RED ) && !red_ok ) {
		Warning( "Can't compute incursion distances from the Red spawn room(s). Bots will perform poorly.\n"
			"This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}
	if ( TFTeamMgr()->IsValidTeam( TF_TEAM_BLUE ) && !blu_ok ) {
		Warning( "Can't compute incursion distances from the Blue spawn room(s). Bots will perform poorly.\n"
			"This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}
	if ( TFTeamMgr()->IsValidTeam( TF_TEAM_GREEN ) && !grn_ok ) {
		Warning( "Can't compute incursion distances from the Green spawn room(s). Bots will perform poorly.\n"
			"This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}
	if ( TFTeamMgr()->IsValidTeam( TF_TEAM_YELLOW ) && !ylw_ok ) {
		Warning( "Can't compute incursion distances from the Yellow spawn room(s). Bots will perform poorly.\n"
			"This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}
	
	// this code is highly questionable
	// and doesn't make sense for 4-team logic anyway
#if 0
	if (!TFGameRules()->IsMannVsMachineMode()) {
		float max_blu = 0.0f;
		
		for (auto area : TheTFNavAreas) {
			max_blu = Max(max_blu, area->m_IncursionDistance[TF_TEAM_BLUE]);
		}
		
		for (auto area : TheTFNavAreas) {
			area->m_IncursionDistance[TF_TEAM_RED] = max_blu - area->m_IncursionDistance[TF_TEAM_BLUE];
		}
	}
#endif
}

void CTFNavMesh::ComputeInvasionAreas()
{
	VPROF_BUDGET("CTFNavMesh::ComputeInvasionAreas", "NextBot");
	
	for (auto area : TheTFNavAreas) {
		area->ComputeInvasionAreaVectors();
	}
}

void CTFNavMesh::ComputeLegalBombDropAreas()
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		// TODO (MvM stuff)
		Assert(false);
	}
}

void CTFNavMesh::DecorateMesh()
{
	VPROF_BUDGET("CTFNavMesh::DecorateMesh", "NextBot");
	
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_SpawnRooms[i].RemoveAll();
	}
	
	for (auto room : CFuncRespawnRoom::AutoList()) {
		if (!room->GetActive()) continue;
		if (room->m_bDisabled)  continue;
		
		for (auto spawn : CTFTeamSpawn::AutoList()) {
			if (!spawn->IsTriggered(nullptr)) continue;
			if (spawn->IsDisabled())          continue;
			
			/* some maps have info_player_teamspawn entities with teamnum 0;
			* this is naughty, however it's not really an issue,
			* since those teamspawns' teamnums will get set properly later,
			* and the nav mesh will eventually do a recompute;
			* so, just ignore them until they have a valid teamnum */
			if (spawn->GetTeamNumber() <  FIRST_GAME_TEAM) continue;
			if (spawn->GetTeamNumber() >= TF_TEAM_COUNT)   continue;
			
			if (room->PointIsWithin(spawn->GetAbsOrigin())) {
				Extent ext;
				ext.Init(room);
				
				CCollectAndLabelSpawnRoomAreas functor(room, spawn);
				this->ForAllTFAreasOverlappingExtent(functor, ext);
			}
		}
	}

	// TF2C If we failed to find a func_respawnroom, fall back to naively marking areas near spawns
	for ( int i = 0; i < TF_TEAM_COUNT; ++i )
	{
		if ( this->m_SpawnRooms[i].IsEmpty() )
		{
			for ( auto spawn : CTFTeamSpawn::AutoList() ) {
				if ( !spawn->IsTriggered( nullptr ) ) continue;
				if ( spawn->IsDisabled() ) continue;

				if ( spawn->GetTeamNumber() < FIRST_GAME_TEAM ) continue;
				if ( spawn->GetTeamNumber() >= TF_TEAM_COUNT ) continue;

				CTFNavArea *spawnfallbackArea = GetNearestTFNavArea( spawn, false, 500.0f );
				if (spawnfallbackArea)
				{
					TheTFNavMesh->MarkSpawnRoomArea(spawnfallbackArea, spawn->GetTeamNumber());
				}
			}
		}
	}
	
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_SpawnExits[i].RemoveAll();
		
		for (auto area : this->m_SpawnRooms[i]) {
			this->CollectAndMarkSpawnRoomExits(area, &this->m_SpawnExits[i]);
		}
	}
	
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_RRVisualizerAreas[i].RemoveAll();
	}
	
	// NOTE: would prefer to use AutoList iteration here, but CFuncRespawnRoomVisualizer is hidden away in a source file
	CFuncBrush *rr_visualizer = nullptr;
	while ((rr_visualizer = assert_cast<CFuncBrush *>(gEntList.FindEntityByClassname(rr_visualizer, "func_respawnroomvisualizer"))) != nullptr) {
		Extent ext;
		ext.Init(rr_visualizer);
		
		auto l_content_mask_for_team = [](int team){
			switch (team) {
			case TF_TEAM_RED:    return CONTENTS_REDTEAM;
			case TF_TEAM_BLUE:   return CONTENTS_BLUETEAM;
			case TF_TEAM_GREEN:  return CONTENTS_GREENTEAM;
			case TF_TEAM_YELLOW: return CONTENTS_YELLOWTEAM;
			default:             return CONTENTS_EMPTY;
			}
		};
		
		for (int i = 0; i < TF_TEAM_COUNT; ++i) {
			if (rr_visualizer->ShouldCollide(COLLISION_GROUP_PLAYER_MOVEMENT, l_content_mask_for_team(i))) {
				this->ForAllTFAreasOverlappingExtent([&](CTFNavArea *area){
					area->AddTFAttributes(CTFNavArea::GetAttr_RRVisualizer(i));
					this->m_RRVisualizerAreas[i].AddToTail(area);
					return true;
				}, ext);
			}
		}
	}
	
	for (auto pack : CAmmoPack::AutoList()) {
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(pack);
		if (area != nullptr) {
			area->AddTFAttributes(CTFNavArea::AMMO);
		}
	}
	
	for (auto kit : CHealthKit::AutoList()) {
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(kit);
		if (area != nullptr) {
			area->AddTFAttributes(CTFNavArea::HEALTH);
		}
	}
	
	for (int i = 0; i < MAX_CONTROL_POINTS; ++i) {
		for (auto area : this->m_ControlPointAreas[i]) {
			area->AddTFAttributes(CTFNavArea::CONTROL_POINT);
		}
	}
}

void CTFNavMesh::OnObjectChanged()
{
	this->ResetMeshAttributes(false);
	
	CUtlVector<CObjectSentrygun *> sentries(0, 16);
	
	for (auto obj : CBaseObject::AutoList()) {
		if (obj->IsSentry() && !obj->IsDying() && !obj->IsBeingCarried()) {
			sentries.AddToTail(assert_cast<CObjectSentrygun *>(obj));
		}
	}
	
	for (auto area : TheTFNavAreas) {
		for (auto sentry : sentries) {
			Vector vecPoint;
			area->GetClosestPointOnArea(sentry->GetAbsOrigin(), &vecPoint);
			
			// TODO: magic number 30.0f
			if (vecPoint.DistToSqr(sentry->GetAbsOrigin()) < Square(sentry->GetMaxRange()) && area->IsPartiallyVisible(VecPlusZ(sentry->GetAbsOrigin(), 30.0f), sentry)) {
				if (!area->HasAnyTFAttributes(CTFNavArea::ANY_SENTRY)) {
					this->m_SentryAreas.AddToTail(area);
				}
				
				switch (sentry->GetTeamNumber()) {
				case TF_TEAM_RED:    area->AddTFAttributes(CTFNavArea::RED_SENTRY   ); break;
				case TF_TEAM_BLUE:   area->AddTFAttributes(CTFNavArea::BLUE_SENTRY  ); break;
				case TF_TEAM_GREEN:  area->AddTFAttributes(CTFNavArea::GREEN_SENTRY ); break;
				case TF_TEAM_YELLOW: area->AddTFAttributes(CTFNavArea::YELLOW_SENTRY); break;
				}
			} 
		}
	}
	
	if (tf_show_sentry_danger.GetInt() != 0) {
		DevMsg("%s: sentries:%d areas count:%d\n", __FUNCTION__, sentries.Count(), this->m_SentryAreas.Count());
	}
}

void CTFNavMesh::RecomputeInternalData()
{
	this->ComputeControlPointAreas();
	this->RemoveAllMeshDecoration();
	this->DecorateMesh();
	this->ComputeBlockedAreas();
	this->ComputeIncursionDistances();
	this->ComputeInvasionAreas();
	this->ComputeLegalBombDropAreas();
	this->ComputeBombTargetDistance();
	
	switch (this->m_RecomputeMode) {
		
	case RECOMPUTE_DEFAULT:
	case RECOMPUTE_SETUP_FINISHED:
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED_UNTIL_POINT_CAPTURE)) {
				area->AddTFAttributes(CTFNavArea::BLOCKED);
			}
		}
		break;
		
	case RECOMPUTE_POINT_CAPTURED:
		DevMsg( "NavMesh RECOMPUTE unblocking point %i\n", this->m_iControlPoint );
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED_UNTIL_POINT_CAPTURE)) {
				bool unblock = false;
				if (area->HasAnyTFAttributes(CTFNavArea::WITH_SECOND_POINT)) {
					unblock = (this->m_iControlPoint > 0);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_THIRD_POINT)) {
					unblock = (this->m_iControlPoint > 1);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FOURTH_POINT)) {
					unblock = (this->m_iControlPoint > 2);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FIFTH_POINT)) {
					unblock = (this->m_iControlPoint > 3);
				} else {
					unblock = true;
				}
				
				if (unblock) {
					area->RemoveTFAttributes(CTFNavArea::BLOCKED);
				} else {
					area->AddTFAttributes( CTFNavArea::BLOCKED );
				}
			} else if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED_AFTER_POINT_CAPTURE)) {
				bool block = false;
				if (area->HasAnyTFAttributes(CTFNavArea::WITH_SECOND_POINT)) {
					block = (this->m_iControlPoint > 0);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_THIRD_POINT)) {
					block = (this->m_iControlPoint > 1);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FOURTH_POINT)) {
					block = (this->m_iControlPoint > 2);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FIFTH_POINT)) {
					block = (this->m_iControlPoint > 3);
				} else {
					block = true;
				}
				
				if (block) {
					area->AddTFAttributes(CTFNavArea::BLOCKED);
				}
			}
		}
		break;
	}
	
	// BUG: shouldn't modes 3, 4, 5 be handled in some way here?

	DevMsg( "NavMesh RECOMPUTE finished\n" );
	
	this->m_ctRecompute.Invalidate();
}

void CTFNavMesh::RemoveAllMeshDecoration()
{
	for (auto area : TheTFNavAreas) {
		area->ClearDynamicTFAttributes();
	}
	
	this->m_SentryAreas.RemoveAll();
	
	this->OnObjectChanged();
}

void CTFNavMesh::ResetMeshAttributes(bool recompute)
{
	for (auto area : this->m_SentryAreas) {
		area->RemoveTFAttributes(CTFNavArea::ANY_SENTRY);
	}
	this->m_SentryAreas.RemoveAll();
	
	if (recompute) {
		this->ScheduleRecompute(RECOMPUTE_DEFAULT);
	}
}

void CTFNavMesh::UpdateDebugDisplay() const
{
	if (engine->IsDedicatedServer()) return;
	
	CBasePlayer *host = UTIL_GetListenServerHost();
	if (host == nullptr) return;
	
	if (tf_show_in_combat_areas.GetBool()) {
		for (auto area : TheTFNavAreas) {
			if (area->IsInCombat()) {
				int r = 255.0f * area->GetCombatIntensity();
				area->DrawFilled(r, 0x00, 0x00, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_enemy_invasion_areas.GetBool()) {
		auto host_area = static_cast<CTFNavArea *>(host->GetLastKnownArea());
		if (host_area != nullptr) {
			for (auto area : host_area->GetInvasionAreas(host->GetTeamNumber())) {
				area->DrawFilled(NB_RGBA_RED, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_bomb_drop_areas.GetBool()) {
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(CTFNavArea::BOMB_DROP)) {
				area->DrawFilled(NB_RGBA_GREEN, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_blocked_areas.GetBool()) {
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED)) {
				area->DrawFilled(NB_RGBA_RED, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 0.0f);
			}
			
			bool blocked[TF_TEAM_COUNT];
			bool blocked_any   = false;
			bool blocked_all   = true;
			bool blocked_multi = false;
			
			for (int team = FIRST_GAME_TEAM; team < TF_TEAM_COUNT; ++team) {
				if (TFTeamMgr()->IsValidTeam(team)) {
					if (area->IsBlocked(team)) {
						blocked[team] = true;
						if (blocked_any) blocked_multi = true;
						blocked_any = true;
					} else {
						blocked[team] = false;
						blocked_all = false;
					}
				} else {
					blocked[team] = false;
				}
			}
			
			if (blocked_any) {
				char text[256];
				V_strcpy_safe(text, "Blocked for:");
				
				if (blocked_all) {
					V_strcat_safe(text, " All");
				} else {
					if (blocked[TF_TEAM_RED   ]) V_strcat_safe(text, " Red");
					if (blocked[TF_TEAM_BLUE  ]) V_strcat_safe(text, " Blue");
					if (blocked[TF_TEAM_GREEN ]) V_strcat_safe(text, " Green");
					if (blocked[TF_TEAM_YELLOW]) V_strcat_safe(text, " Yellow");
				}
				
				Color c;
				if (blocked_multi) {
					c = NB_COLOR_GRAY;
				} else {
					if (blocked[TF_TEAM_RED   ]) c = NB_COLOR_DKRED_64;
					if (blocked[TF_TEAM_BLUE  ]) c = NB_COLOR_DKBLUE_64;
					if (blocked[TF_TEAM_GREEN ]) c = NB_COLOR_DKGREEN_64;
					if (blocked[TF_TEAM_YELLOW]) c = NB_COLOR_DKYELLOW_64;
				}
				area->DrawFilled(c.r(), c.g(), c.b(), c.a(), NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), text, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
		}
	}
	
	if (tf_show_incursion_flow.GetInt() > 0 || tf_show_incursion_flow_gradient.GetInt() > 0) {
		Vector vecAbsStart = host->EyePosition();
		Vector vecAbsEnd   = host->EyePosition() + (2000.0f * AngleVecFwd(host->EyeAngles() + host->GetPunchAngle()));
		
		trace_t tr;
		UTIL_TraceLine(vecAbsStart, vecAbsEnd, MASK_NPCSOLID, CTraceFilterWalkableEntities(nullptr, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING), &tr);
		
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(tr.endpos, false, 500.0f);
		if (area != nullptr) {
			if (tf_show_incursion_flow.GetInt() > 0) {
				DrawIncursionFlow functor;
				SearchSurroundingAreas(area, area->GetCenter(), functor, tf_show_incursion_flow_range.GetFloat());
			} else if (tf_show_incursion_flow_gradient.GetInt() > 0) {
				int team = tf_show_incursion_flow_gradient.GetInt() + 1;
				if (TFTeamMgr()->IsValidTeam(team)) {
					int r = 0x00;
					int g = 0x00;
					int b = 0x00;
					
					switch (team) {
					case TF_TEAM_RED:    r = 0xff; g = 0x00; b = 0x00; break;
					case TF_TEAM_BLUE:   r = 0x00; g = 0x00; b = 0xff; break;
					case TF_TEAM_GREEN:  r = 0x00; g = 0xff; b = 0x00; break;
					case TF_TEAM_YELLOW: r = 0xff; g = 0xff; b = 0x00; break;
					}
					
					area->DrawFilled(r, g, b, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
					
					CUtlVector<CTFNavArea *> prior_areas;
					area->CollectPriorIncursionAreas(team, &prior_areas);
					for (auto prior_area : prior_areas) {
						prior_area->DrawFilled(r / 2, g / 2, b / 2, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
					}
					
					r = Min(0xff, r + 100);
					g = Min(0xff, g + 100);
					b = Min(0xff, b + 100);
					
					CUtlVector<CTFNavArea *> next_areas;
					area->CollectNextIncursionAreas(team, &next_areas);
					for (auto next_area : next_areas) {
						next_area->DrawFilled(r, g, b, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
					}
				}
			}
		}
	}
	
	// TODO: rework tf_show_mesh_decoration and get rid of _manual
	// 0 = off
	// 1 = auto
	// 2 = manual
	// 3 = both
	// or whatever; but not this pile of garbage
	
	// TODO: use EntityTextAtPosition and increment the line number to draw multiple text lines without overlapping
	
	if (tf_show_mesh_decoration.GetBool() && !tf_show_mesh_decoration_manual.GetBool()) {
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_RED)) {
			for (auto area : this->m_SpawnRooms[TF_TEAM_RED]) {
				if (area->HasAnyTFAttributes(CTFNavArea::SPAWN_ROOM_EXIT)) continue;
				
				area->DrawFilled(NB_RGBA_RED, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Red Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_SpawnExits[TF_TEAM_RED]) {
				area->DrawFilled(NB_RGBA_LTRED_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Red Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_RRVisualizerAreas[TF_TEAM_RED]) {
				area->DrawFilled(NB_RGBA_LTRED_C8, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Red Respawn Room Visualizer", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
		}
		
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_BLUE)) {
			for (auto area : this->m_SpawnRooms[TF_TEAM_BLUE]) {
				if (area->HasAnyTFAttributes(CTFNavArea::SPAWN_ROOM_EXIT)) continue;
				
				area->DrawFilled(NB_RGBA_BLUE, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Blue Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_SpawnExits[TF_TEAM_BLUE]) {
				area->DrawFilled(NB_RGBA_LTBLUE_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Blue Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_RRVisualizerAreas[TF_TEAM_BLUE]) {
				area->DrawFilled(NB_RGBA_LTBLUE_C8, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Blue Respawn Room Visualizer", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
		}
		
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_GREEN)) {
			for (auto area : this->m_SpawnRooms[TF_TEAM_GREEN]) {
				if (area->HasAnyTFAttributes(CTFNavArea::SPAWN_ROOM_EXIT)) continue;
				
				area->DrawFilled(NB_RGBA_GREEN, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Green Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_SpawnExits[TF_TEAM_GREEN]) {
				area->DrawFilled(NB_RGBA_LTGREEN_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Green Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_RRVisualizerAreas[TF_TEAM_GREEN]) {
				area->DrawFilled(NB_RGBA_LTGREEN_C8, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Green Respawn Room Visualizer", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
		}
		
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_YELLOW)) {
			for (auto area : this->m_SpawnRooms[TF_TEAM_YELLOW]) {
				if (area->HasAnyTFAttributes(CTFNavArea::SPAWN_ROOM_EXIT)) continue;
				
				area->DrawFilled(NB_RGBA_YELLOW, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Yellow Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_SpawnExits[TF_TEAM_YELLOW]) {
				area->DrawFilled(NB_RGBA_LTYELLOW_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Yellow Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			for (auto area : this->m_RRVisualizerAreas[TF_TEAM_YELLOW]) {
				area->DrawFilled(NB_RGBA_LTYELLOW_C8, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area == TheTFNavMesh->GetSelectedTFArea()) {
					NDebugOverlay::Text(area->GetCenter(), "Yellow Respawn Room Visualizer", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
		}
		
		for (auto area : TheTFNavAreas) {
			if (area->HasAllTFAttributes(CTFNavArea::AMMO | CTFNavArea::HEALTH)) {
				area->DrawFilled(NB_RGBA_MAGENTA, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Health & Ammo", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			} else if (area->HasAnyTFAttributes(CTFNavArea::AMMO)) {
				area->DrawFilled(NB_RGBA_GRAY, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Ammo", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			} else if (area->HasAnyTFAttributes(CTFNavArea::HEALTH)) {
				area->DrawFilled(NB_RGBA_LTRED_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Health", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::CONTROL_POINT)) {
				area->DrawFilled(NB_RGBA_GREEN, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Control Point", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::RED_ONE_WAY_DOOR)) {
				area->DrawFilled(NB_RGBA_LTRED_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::BLUE_ONE_WAY_DOOR)) {
				area->DrawFilled(NB_RGBA_LTBLUE_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::GREEN_ONE_WAY_DOOR)) {
				area->DrawFilled(NB_RGBA_LTGREEN_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::YELLOW_ONE_WAY_DOOR)) {
				area->DrawFilled(NB_RGBA_LTYELLOW_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_mesh_decoration_manual.GetBool()) {
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(CTFNavArea::SNIPER_SPOT)) {
				area->DrawFilled(NB_RGBA_YELLOW, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Sniper Spot", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::SENTRY_SPOT)) {
				area->DrawFilled(NB_RGBA_ORANGE_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Sentry Spot", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::NO_SPAWNING)) {
				area->DrawFilled(NB_RGBA_DKYELLOW_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "No Spawning", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::RESCUE_CLOSET)) {
				area->DrawFilled(NB_RGBA_CYAN, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Rescue Closet", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED_UNTIL_POINT_CAPTURE)) {
				area->DrawFilled(NB_RGBA_CYAN, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area->HasAnyTFAttributes(CTFNavArea::WITH_SECOND_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked Until Second Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_THIRD_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked Until Third Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FOURTH_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked Until Fourth Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FIFTH_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked Until Fifth Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else {
					NDebugOverlay::Text(area->GetCenter(), "Blocked Until First Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::BLOCKED_AFTER_POINT_CAPTURE)) {
				area->DrawFilled(NB_RGBA_YELLOW, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				if (area->HasAnyTFAttributes(CTFNavArea::WITH_SECOND_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked After Second Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_THIRD_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked After Third Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FOURTH_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked After Fourth Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else if (area->HasAnyTFAttributes(CTFNavArea::WITH_FIFTH_POINT)) {
					NDebugOverlay::Text(area->GetCenter(), "Blocked After Fifth Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				} else {
					NDebugOverlay::Text(area->GetCenter(), "Blocked After First Point Captured", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
				}
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::RED_SETUP_GATE)) {
				area->DrawFilled(NB_RGBA_DKRED_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Red Setup Gate", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::BLUE_SETUP_GATE)) {
				area->DrawFilled(NB_RGBA_DKBLUE_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Blue Setup Gate", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::GREEN_SETUP_GATE)) {
				area->DrawFilled(NB_RGBA_DKGREEN_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Green Setup Gate", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::YELLOW_SETUP_GATE)) {
				area->DrawFilled(NB_RGBA_DKYELLOW_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Yellow Setup Gate", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::DOOR_ALWAYS_BLOCKS)) {
				area->DrawFilled(NB_RGBA_DKMAGENTA_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Door Always Blocks", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			if (area->HasAnyTFAttributes(CTFNavArea::DOOR_NEVER_BLOCKS)) {
				area->DrawFilled(NB_RGBA_DKGREEN_64, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Door Never Blocks", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
			
			if (area->HasAnyTFAttributes(CTFNavArea::UNBLOCKABLE)) {
				area->DrawFilled(0x00, 0xc8, 0x64, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				NDebugOverlay::Text(area->GetCenter(), "Unblockable", false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}
		}
	}
	
	if (tf_show_sentry_danger.GetInt() != 0) {
		auto& areas = (tf_show_sentry_danger.GetInt() == 2 ? TheTFNavAreas : this->m_SentryAreas);
		for (auto area : areas) {
			if (area->HasAnyTFAttributes(CTFNavArea::ANY_SENTRY)) {
				int r = (area->HasAnyTFAttributes(CTFNavArea::RED_SENTRY   | CTFNavArea::YELLOW_SENTRY) ? 0xff : 0x00);
				int g = (area->HasAnyTFAttributes(CTFNavArea::GREEN_SENTRY | CTFNavArea::YELLOW_SENTRY) ? 0xff : 0x00);
				int b = (area->HasAnyTFAttributes(CTFNavArea::BLUE_SENTRY                             ) ? 0xff : 0x00);
				
				area->DrawFilled(r, g, b, 0x50, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_actor_potential_visibility.GetBool()) {
		for (auto area : TheTFNavAreas) {
			bool visible[TF_TEAM_COUNT];
			bool visible_any = false;
			
			for (int team = FIRST_GAME_TEAM; team < TF_TEAM_COUNT; ++team) {
				if (TFTeamMgr()->IsValidTeam(team) && area->IsPotentiallyVisibleToTeam(team)) {
					visible[team] = true;
					visible_any = true;
				} else {
					visible[team] = false;
				}
			}
			
			if (visible_any) {
				int r = ((visible[TF_TEAM_RED  ] || visible[TF_TEAM_YELLOW]) ? 0xff : 0x00);
				int g = ((visible[TF_TEAM_GREEN] || visible[TF_TEAM_YELLOW]) ? 0xff : 0x00);
				int b = ( visible[TF_TEAM_BLUE ]                             ? 0xff : 0x00);
				
				area->DrawFilled(r, g, b, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
	
	if (tf_show_control_points.GetBool()) {
		for (int i = 0; i < MAX_CONTROL_POINTS; ++i) {
			for (auto area : this->m_ControlPointAreas[i]) {
				if (area == this->m_ControlPointCenter[i]) {
					area->DrawFilled(NB_RGBA_YELLOW,    NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				} else {
					area->DrawFilled(NB_RGBA_ORANGE_96, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
				}
			}
		}
	}
}


// BUG: the way this works is fundamentally problematic
// successive schedulings will actually DELAY the recomputation if the timer is already ticking
// also they'll wipe out the control point index if a pending recomputation was depending on that
void CTFNavMesh::ScheduleRecompute( RecomputeMode mode, int control_point )
{
	if ( this->m_ctRecompute.IsElapsed() )
	{
		DevMsg( "NavMesh RECOMPUTE scheduled\n" );
		this->m_ctRecompute.Start( 2.0f );
		this->m_RecomputeMode = mode;
		this->m_iControlPoint = control_point;
	}
}


static void CMD_SelectAmbushAreas(const CCommand& args)
{
	// TODO
	Assert(false);
	
	// presumably what we have inside here is an inlined call to CollectAmbushAreas
	// rather than duplicated code
	
	#pragma message("STUB: CMD_SelectAmbushAreas")
	DevMsg("STUB: CMD_SelectAmbushAreas\n");
}


static ConCommand tf_select_ambush_areas("tf_select_ambush_areas", &CMD_SelectAmbushAreas, "Add good ambush spots to the selected set. For debugging.", FCVAR_CHEAT | FCVAR_GAMEDLL);


#ifdef STAGING_ONLY
CON_COMMAND_F(tf_nav_mesh_print_size_statistics, "Print nav area size stats; pass an arg for the number of groups (default: 10)", FCVAR_CHEAT)
{
	struct NavAreaStat
	{
		float size_x;
		float size_y;
		float size_area;
		
		unsigned int id;
		Vector pos;
		
		NavAreaStat(const CTFNavArea *area)
		{
			size_x    = area->GetSizeX();
			size_y    = area->GetSizeY();
			size_area = size_x * size_y;
			
			id  = area->GetID();
			pos = area->GetCenter();
		}
	};
	
	CUtlVector<NavAreaStat> stats;
	TheTFNavMesh->ForAllTFAreas([&](const CTFNavArea *area){
		/* why does CUtlVector not have an 'emplace' equivalent? what is this, the stone age? */
		stats.AddToTail(NavAreaStat(area));
		return true;
	});
	if (stats.IsEmpty()) return;
	stats.Sort([](const NavAreaStat *lhs, const NavAreaStat *rhs) -> int {
		if (lhs->size_area == rhs->size_area) return 0;
		return (lhs->size_area < rhs->size_area ? -1 : 1);
	});
	
	int n_groups = 10;
	if (args.ArgC() >= 2) {
		n_groups = V_atoi(args[1]);
	}
	
	auto l_quantile = [&](float r) -> float {
		Assert(r >= 0.0f && r <= 1.0f);
		r *= (stats.Count() - 1);
		return stats[(int)r].size_area;
	};
	
	Msg("Nav Mesh Area Stats\n");
	Msg("========================================\n");
	Msg("Min: %8.1f\n", l_quantile(0.00f));
	for (int i = 1; i < n_groups; ++i) {
		float r = RemapVal(i, 0, n_groups, 0.00f, 1.00f);
		Msg("%2.0f%%: %8.1f\n", 100.0f * r, l_quantile(r));
	}
	Msg("Max: %8.1f\n", l_quantile(1.00f));
	Msg("========================================\n");
}
#endif
