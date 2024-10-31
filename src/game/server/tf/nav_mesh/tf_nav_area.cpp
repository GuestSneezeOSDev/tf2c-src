/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_nav_area.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_bot.h"
#include "tf_team.h"
#include "fmtstr.h"


static ConVar tf_nav_show_incursion_distance   ("tf_nav_show_incursion_distance",        "0", FCVAR_CHEAT, "Display travel distances from current spawn room (1=red, 2=blue, 3=green, 4=yellow)");
static ConVar tf_nav_show_bomb_target_distance ("tf_nav_show_bomb_target_distance",      "0", FCVAR_CHEAT, "Display travel distances to bomb target (MvM mode)");
static ConVar tf_nav_show_turf_ownership       ("tf_nav_show_turf_ownership",            "0", FCVAR_CHEAT, "Color nav area by smallest incursion distance");
static ConVar tf_nav_in_combat_duration        ("tf_nav_in_combat_duration",            "30", FCVAR_CHEAT, "How long after gunfire occurs is this area still considered to be 'in combat'");
static ConVar tf_nav_combat_build_rate         ("tf_nav_combat_build_rate",           "0.05", FCVAR_CHEAT, "Gunfire/second increase (combat caps at 1.0)");
static ConVar tf_nav_combat_decay_rate         ("tf_nav_combat_decay_rate",          "0.022", FCVAR_CHEAT, "Decay/second toward zero");
static ConVar tf_show_sniper_areas             ("tf_show_sniper_areas",                  "0", FCVAR_CHEAT);
static ConVar tf_show_sniper_areas_safety_range("tf_show_sniper_areas_safety_range",  "1000", FCVAR_CHEAT);
static ConVar tf_show_incursion_range          ("tf_show_incursion_range",               "0", FCVAR_CHEAT, "1 = red, 2 = blue, 3 = green, 4 = yellow");
static ConVar tf_show_incursion_range_min      ("tf_show_incursion_range_min",           "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values");
static ConVar tf_show_incursion_range_max      ("tf_show_incursion_range_max",           "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values");


unsigned int CTFNavArea::s_MasterTFMark = 1;

// Redefinition parity with Live
enum TFNavAttributeType
{
	TF_NAV_INVALID						=	0,
	TF_NAV_BLOCKED						=	0X00000001,
	TF_NAV_SPAWN_ROOM_RED				=	0X00000002,
	TF_NAV_SPAWN_ROOM_BLUE				=	0X00000004,
	TF_NAV_SPAWN_ROOM_EXIT				=	0X00000008,
	TF_NAV_HAS_AMMO						=	0X00000010,
	TF_NAV_HAS_HEALTH					=	0X00000020,
	TF_NAV_CONTROL_POINT				=	0X00000040,
	TF_NAV_BLUE_SENTRY_DANGER			=	0X00000080,
	TF_NAV_RED_SENTRY_DANGER			=	0X00000100,
	TF_NAV_BLUE_SETUP_GATE				=	0X00000800,
	TF_NAV_RED_SETUP_GATE				=	0X00001000,
	TF_NAV_BLOCKED_AFTER_POINT_CAPTURE	=	0X00002000,
	TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE	=	0X00004000,
	TF_NAV_BLUE_ONE_WAY_DOOR			=	0X00008000,
	TF_NAV_RED_ONE_WAY_DOOR				=	0X00010000,
	TF_NAV_WITH_SECOND_POINT			=	0X00020000,
	TF_NAV_WITH_THIRD_POINT				=	0X00040000,
	TF_NAV_WITH_FOURTH_POINT			=	0X00080000,
	TF_NAV_WITH_FIFTH_POINT				=	0X00100000,
	TF_NAV_SNIPER_SPOT					=	0X00200000,
	TF_NAV_SENTRY_SPOT					=	0X00400000,
	TF_NAV_ESCAPE_ROUTE					=	0X00800000,
	TF_NAV_ESCAPE_ROUTE_VISIBLE			=	0X01000000,
	TF_NAV_NO_SPAWNING					=	0X02000000,
	TF_NAV_RESCUE_CLOSET				=	0X04000000,
	TF_NAV_BOMB_CAN_DROP_HERE			=	0X08000000,
	TF_NAV_DOOR_NEVER_BLOCKS			=	0X10000000,
	TF_NAV_DOOR_ALWAYS_BLOCKS			=	0X20000000,
	TF_NAV_UNBLOCKABLE					=	0X40000000,
	TF_NAV_PERSISTENT_ATTRIBUTES		=	TF_NAV_BLUE_SETUP_GATE | TF_NAV_RED_SETUP_GATE | TF_NAV_BLOCKED_AFTER_POINT_CAPTURE |
											TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE | TF_NAV_BLUE_ONE_WAY_DOOR |
											TF_NAV_RED_ONE_WAY_DOOR | TF_NAV_WITH_SECOND_POINT | TF_NAV_WITH_THIRD_POINT |
											TF_NAV_WITH_FOURTH_POINT | TF_NAV_WITH_FIFTH_POINT | TF_NAV_SNIPER_SPOT |
											TF_NAV_SENTRY_SPOT | TF_NAV_NO_SPAWNING | TF_NAV_RESCUE_CLOSET |
											TF_NAV_DOOR_NEVER_BLOCKS | TF_NAV_DOOR_ALWAYS_BLOCKS | TF_NAV_UNBLOCKABLE
};
BEGIN_SCRIPTENUM( ETFNavAttributeType, "Team Fortress nav attributes" )
	DEFINE_ENUMCONST( TF_NAV_INVALID, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_RED, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_BLUE, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_EXIT, "" )
	DEFINE_ENUMCONST( TF_NAV_HAS_AMMO, "" )
	DEFINE_ENUMCONST( TF_NAV_HAS_HEALTH, "" )
	DEFINE_ENUMCONST( TF_NAV_CONTROL_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_SENTRY_DANGER, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_SENTRY_DANGER, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_SETUP_GATE, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_SETUP_GATE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_ONE_WAY_DOOR, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_ONE_WAY_DOOR, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_SECOND_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_THIRD_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_FOURTH_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_FIFTH_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_SNIPER_SPOT, "" )
	DEFINE_ENUMCONST( TF_NAV_SENTRY_SPOT, "" )
	DEFINE_ENUMCONST( TF_NAV_ESCAPE_ROUTE, "" )
	DEFINE_ENUMCONST( TF_NAV_ESCAPE_ROUTE_VISIBLE, "" )
	DEFINE_ENUMCONST( TF_NAV_NO_SPAWNING, "" )
	DEFINE_ENUMCONST( TF_NAV_RESCUE_CLOSET, "" )
	DEFINE_ENUMCONST( TF_NAV_BOMB_CAN_DROP_HERE, "" )
	DEFINE_ENUMCONST( TF_NAV_DOOR_NEVER_BLOCKS, "" )
	DEFINE_ENUMCONST( TF_NAV_DOOR_ALWAYS_BLOCKS, "" )
	DEFINE_ENUMCONST( TF_NAV_UNBLOCKABLE, "" )
	DEFINE_ENUMCONST( TF_NAV_PERSISTENT_ATTRIBUTES, "" )
END_SCRIPTENUM()


BEGIN_SCRIPTDESC( CTFNavArea, CNavArea, "TF navigation area" )
	DEFINE_SCRIPTFUNC( SetAttributeTF, "Set TF-specific area attributes" )
	DEFINE_SCRIPTFUNC( HasAttributeTF, "Has TF-specific area attribute bits" )
	DEFINE_SCRIPTFUNC( ClearAttributeTF, "Clear TF-specific area attribute bits" )
	
	DEFINE_SCRIPTFUNC( FindRandomSpot, "Get random origin within extent of area" )
	DEFINE_SCRIPTFUNC( IsPotentiallyVisibleToTeam, "( team ) - Return true if any portion of this area is visible to anyone on the given team" )
	DEFINE_SCRIPTFUNC( IsCompletelyVisibleToTeam, "( team ) - Return true if given area is completely visible from somewhere in this area by someone on the team" )
	DEFINE_SCRIPTFUNC( IsBottleneck, "Returns true if area is a bottleneck" )
	DEFINE_SCRIPTFUNC( IsValidForWanderingPopulation, "Returns true if area is valid for wandering population" )
	DEFINE_SCRIPTFUNC( GetTravelDistanceToBombTarget, "Gets the travel distance to the MvM bomb target" )
	DEFINE_SCRIPTFUNC( IsReachableByTeam,"Is this area reachable by the given team?" )

	DEFINE_SCRIPTFUNC( IsTFMarked, "Is this nav area marked with the current marking scope?" )
	DEFINE_SCRIPTFUNC( TFMark, "Mark this nav area with the current marking scope." )
END_SCRIPTDESC()


void CTFNavArea::OnServerActivate()
{
	CNavArea::OnServerActivate();
	
	this->ClearAllPVActors();
}

void CTFNavArea::OnRoundRestart()
{
	CNavArea::OnRoundRestart();
	
	this->ClearAllPVActors();
	this->m_flCombatLevel = 0.0f;
}

void CTFNavArea::Save(CUtlBuffer& fileBuffer, unsigned int version) const
{
	CNavArea::Save(fileBuffer, version);
	
	fileBuffer.PutUint64(this->GetStaticTFAttributes());
}

NavErrorType CTFNavArea::Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int subVersion)
{
	NavErrorType result = CNavArea::Load(fileBuffer, version, subVersion);
	if (result != NAV_OK) return result;
	
	this->m_nTFAttributes = fileBuffer.GetInt64();
	if (!fileBuffer.IsValid()) {
		Warning("Can't read TF2C nav area attributes!\n");
		return NAV_INVALID_FILE;
	}
//	if (subVersion == 1) {
//		/* in version 1, the attr bits currently at 16-31 were at 15-30 */
//		uint64 attr = (this->m_nTFAttributes & 0x000000007fff8000) << 1;
//		this->m_nTFAttributes &= ~0x00000000ffff8000;
//		this->m_nTFAttributes |= attr;
//	}
	if (this->HasAnyTFAttributes(~STATIC_ATTRS)) {
		if (this->HasAnyTFAttributes(DYNAMIC_ATTRS)) {
			Warning("NavArea on disk contains dynamic attribute bits! (%016llx)\n", this->GetDynamicTFAttributes());
		}
		if (this->HasAnyTFAttributes(~(STATIC_ATTRS | DYNAMIC_ATTRS))) {
			Warning("NavArea on disk contains unknown attribute bits! (%016llx)\n", (this->GetTFAttributes() & ~(STATIC_ATTRS | DYNAMIC_ATTRS)));
		}
		return NAV_CORRUPT_DATA;
	}
	
	return NAV_OK;
}

bool CTFNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
{
	if (this->HasAnyTFAttributes(UNBLOCKABLE)) return false;
	if (this->HasAnyTFAttributes(BLOCKED))     return true;
	
	if (teamID == TF_TEAM_RED    && this->HasAnyTFAttributes(ANY_ONE_WAY_DOOR ^    RED_ONE_WAY_DOOR)) return true;
	if (teamID == TF_TEAM_BLUE   && this->HasAnyTFAttributes(ANY_ONE_WAY_DOOR ^   BLUE_ONE_WAY_DOOR)) return true;
	if (teamID == TF_TEAM_GREEN  && this->HasAnyTFAttributes(ANY_ONE_WAY_DOOR ^  GREEN_ONE_WAY_DOOR)) return true;
	if (teamID == TF_TEAM_YELLOW && this->HasAnyTFAttributes(ANY_ONE_WAY_DOOR ^ YELLOW_ONE_WAY_DOOR)) return true;
	
	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

void CTFNavArea::Draw() const
{
	CNavArea::Draw();
	
	if (tf_nav_show_incursion_distance.GetBool()) {
		NDebugOverlay::Text(this->GetCenter(), CFmtStr("R:%3.1f B:%3.1f G:%3.1f Y:%3.1f",
			this->GetIncursionDistance(TF_TEAM_RED),   this->GetIncursionDistance(TF_TEAM_BLUE),
			this->GetIncursionDistance(TF_TEAM_GREEN), this->GetIncursionDistance(TF_TEAM_YELLOW)),
			false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
	}
	
	if (tf_nav_show_bomb_target_distance.GetBool()) {
		NDebugOverlay::Text(this->GetCenter(), CFmtStrN<64>("%3.1f", this->m_flBombTargetDistance), false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
	}
	
	if (tf_show_sniper_areas.GetBool()) {
		float range = tf_show_sniper_areas_safety_range.GetFloat();
		
		bool safe = false;
		int r = 0x00;
		int g = 0x00;
		int b = 0x00;
		
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_RED)    && this->IsAwayFromInvasionAreas(TF_TEAM_RED,    range)) {
			safe = true;
			r |= 0xff;
		}
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_BLUE)   && this->IsAwayFromInvasionAreas(TF_TEAM_BLUE,   range)) {
			safe = true;
			b |= 0xff;
		}
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_GREEN)  && this->IsAwayFromInvasionAreas(TF_TEAM_GREEN,  range)) {
			safe = true;
			g |= 0xff;
		}
		if (TFTeamMgr()->IsValidTeam(TF_TEAM_YELLOW) && this->IsAwayFromInvasionAreas(TF_TEAM_YELLOW, range)) {
			safe = true;
			r |= 0xff;
			g |= 0xff;
		}
		
		if (safe) {
			this->DrawFilled(r, g, b, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
		}
	}
	
	if (tf_show_incursion_range.GetInt() > 0) {
		int team = tf_show_incursion_range.GetInt() + 1;
		
		if (TFTeamMgr()->IsValidTeam(team)) {
			float dist = this->GetIncursionDistance(team);
			
			if (dist >= tf_show_incursion_range_min.GetFloat() && dist <= tf_show_incursion_range_max.GetFloat()) {
				this->DrawFilled(0x00, 0xff, 0x00, 0xff, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 5.0f);
			}
		}
	}
}

bool CTFNavArea::IsPotentiallyVisibleToTeam(int team) const
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return false;
	
	return this->m_PVActors[team].Count() > 0;
}


void CTFNavArea::AddPotentiallyVisibleActor(CBaseCombatCharacter *actor)
{
	Assert(actor != nullptr);
	if (actor == nullptr) return;
	
	int team = actor->GetTeamNumber();
	if (team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT) return;
	
	/* what's the purpose of filtering out proxy-managed bots here? */
	CTFBot *bot = ToTFBot(actor);
	if (bot != nullptr && bot->HasAttribute(CTFBot::PROXY_MANAGED)) return;
	
	if (!this->m_PVActors[team].HasElement(actor)) {
		this->m_PVActors[team].AddToTail(actor);
	}
}

void CTFNavArea::RemovePotentiallyVisibleActor(CBaseCombatCharacter *actor)
{
	Assert(actor != nullptr);
	if (actor == nullptr) return;
	
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_PVActors[i].FindAndFastRemove(actor);
	}
}


void CTFNavArea::CollectPriorIncursionAreas(int team, CUtlVector<CTFNavArea *> *areas)
{
	float dist = this->GetIncursionDistance(team);
	
	for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
		const TFNavConnectVector& connections = *this->GetAdjacentTFAreas((NavDirType)dir);
		
		for (auto& connection : connections) {
			auto area = connection.GetTFArea();
			
			if (area->GetIncursionDistance(team) < dist) {
				areas->AddToTail(area);
			}
		}
	}
}

void CTFNavArea::CollectNextIncursionAreas(int team, CUtlVector<CTFNavArea *> *areas)
{
	float dist = this->GetIncursionDistance(team);
	
	for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
		const TFNavConnectVector& connections = *this->GetAdjacentTFAreas((NavDirType)dir);
		
		for (auto& connection : connections) {
			auto area = connection.GetTFArea();
			
			if (area->GetIncursionDistance(team) > dist) {
				areas->AddToTail(area);
			}
		}
	}
}

CTFNavArea *CTFNavArea::GetNextIncursionArea(int team) const
{
	float dist = this->GetIncursionDistance(team);
	CTFNavArea *next_area = nullptr;
	
	for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
		const TFNavConnectVector& connections = *this->GetAdjacentTFAreas((NavDirType)dir);
		
		for (auto& connection : connections) {
			auto area = connection.GetTFArea();
			
			if (area->GetIncursionDistance(team) > dist) {
				dist = area->GetIncursionDistance(team);
				next_area = area;
			}
		}
	}
	
	return next_area;
}

float CTFNavArea::GetIncursionDistance(int team) const
{
	Assert( team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT );
	if ( team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT ) return -1.0f;
	return this->m_IncursionDistance[team];
}

void CTFNavArea::ResetIncursionDistances()
{
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_IncursionDistance[i] = -1.0f;
	}
}


void CTFNavArea::ComputeInvasionAreaVectors()
{
	class MarkInvasionAreas
	{
	public:
		MarkInvasionAreas(unsigned int searchMarker) :
			m_SearchMarker(searchMarker) {}
		
		bool operator()(CTFNavArea *area)
		{
			area->m_InvasionAreaMarker = this->m_SearchMarker;
			return true;
		}
		
	private:
		unsigned int m_SearchMarker;
	};
	
	class CollectInvasionAreas
	{
	public:
		CollectInvasionAreas(unsigned int searchMarker, CTFNavArea *area, CUtlVector<CTFNavArea *> *invasion_areas_red, CUtlVector<CTFNavArea *> *invasion_areas_blue,
			CUtlVector<CTFNavArea *> *invasion_areas_green, CUtlVector<CTFNavArea *> *invasion_areas_yellow ) :
			m_SearchMarker( searchMarker ), m_pArea( area ), m_InvasionAreasRed( invasion_areas_red ), m_InvasionAreasBlue( invasion_areas_blue ),
			m_InvasionAreasGreen( invasion_areas_green ), m_InvasionAreasYellow( invasion_areas_yellow ) {}
		
		bool operator()(CTFNavArea *area)
		{
			// TODO: 4-team logic (may be tough...?)
			// For now, just have GRN/YLW look up YLW/GRN as their enemy team.
			// That's good enough for most maps I tried
			
			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
				const TFNavConnectVector& connections = *area->GetAdjacentTFAreas((NavDirType)dir);
				
				for (auto& connection : connections) {
					auto conn_area = connection.GetTFArea();
					if (conn_area->m_InvasionAreaMarker == this->m_SearchMarker) continue;
					
					if (area->m_IncursionDistance[TF_TEAM_BLUE] >      conn_area->m_IncursionDistance[TF_TEAM_BLUE] &&
						area->m_IncursionDistance[TF_TEAM_BLUE] <= this->m_pArea->m_IncursionDistance[TF_TEAM_BLUE] + 100.0f) {
						this->m_InvasionAreasRed->AddToTail(conn_area);
					}
					
					if (area->m_IncursionDistance[TF_TEAM_RED] >      conn_area->m_IncursionDistance[TF_TEAM_RED] &&
						area->m_IncursionDistance[TF_TEAM_RED] <= this->m_pArea->m_IncursionDistance[TF_TEAM_RED] + 100.0f) {
						this->m_InvasionAreasBlue->AddToTail(conn_area);
					}

					if ( area->m_IncursionDistance[TF_TEAM_GREEN] >      conn_area->m_IncursionDistance[TF_TEAM_GREEN] &&
						area->m_IncursionDistance[TF_TEAM_GREEN] <= this->m_pArea->m_IncursionDistance[TF_TEAM_GREEN] + 100.0f ) {
						this->m_InvasionAreasYellow->AddToTail( conn_area );
					}

					if ( area->m_IncursionDistance[TF_TEAM_YELLOW] >      conn_area->m_IncursionDistance[TF_TEAM_YELLOW] &&
						area->m_IncursionDistance[TF_TEAM_YELLOW] <= this->m_pArea->m_IncursionDistance[TF_TEAM_YELLOW] + 100.0f ) {
						this->m_InvasionAreasGreen->AddToTail( conn_area );
					}
				}
			}
			
			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
				const TFNavConnectVector& connections = *area->GetIncomingTFConnections((NavDirType)dir);
				
				for (auto& connection : connections) {
					auto conn_area = connection.GetTFArea();
					if (conn_area->m_InvasionAreaMarker == this->m_SearchMarker) continue;
					
					if (area->m_IncursionDistance[TF_TEAM_BLUE] >      conn_area->m_IncursionDistance[TF_TEAM_BLUE] &&
						area->m_IncursionDistance[TF_TEAM_BLUE] <= this->m_pArea->m_IncursionDistance[TF_TEAM_BLUE] + 100.0f) {
						this->m_InvasionAreasRed->AddToTail(conn_area);
					}
					
					if (area->m_IncursionDistance[TF_TEAM_RED] >      conn_area->m_IncursionDistance[TF_TEAM_RED] &&
						area->m_IncursionDistance[TF_TEAM_RED] <= this->m_pArea->m_IncursionDistance[TF_TEAM_RED] + 100.0f) {
						this->m_InvasionAreasBlue->AddToTail(conn_area);
					}

					if ( area->m_IncursionDistance[TF_TEAM_GREEN] >      conn_area->m_IncursionDistance[TF_TEAM_GREEN] &&
						area->m_IncursionDistance[TF_TEAM_GREEN] <= this->m_pArea->m_IncursionDistance[TF_TEAM_GREEN] + 100.0f ) {
						this->m_InvasionAreasYellow->AddToTail( conn_area );
					}

					if ( area->m_IncursionDistance[TF_TEAM_YELLOW] >      conn_area->m_IncursionDistance[TF_TEAM_YELLOW] &&
						area->m_IncursionDistance[TF_TEAM_YELLOW] <= this->m_pArea->m_IncursionDistance[TF_TEAM_YELLOW] + 100.0f ) {
						this->m_InvasionAreasGreen->AddToTail( conn_area );
					}
				}
			}
			
			return true;
		}
		
	private:
		CTFNavArea *m_pArea;
		CUtlVector<CTFNavArea *> *m_InvasionAreasRed;
		CUtlVector<CTFNavArea *> *m_InvasionAreasBlue;
		CUtlVector<CTFNavArea *> *m_InvasionAreasGreen;
		CUtlVector<CTFNavArea *> *m_InvasionAreasYellow;
		unsigned int m_SearchMarker;
	};
	
	static unsigned int searchMarker = RandomInt(0, 1024 * 1024);
	++searchMarker;
	
	this->ClearAllInvasionAreas();
	
	MarkInvasionAreas func1(searchMarker);
	this->ForAllCompletelyVisibleTFAreas(func1);
	
	// TODO: 4-team logic (green and yellow)
	// AssertOnce(!TFGameRules()->IsFourTeamGame());
	CollectInvasionAreas func2( searchMarker, this, &this->m_InvasionAreas[TF_TEAM_RED], &this->m_InvasionAreas[TF_TEAM_BLUE], &this->m_InvasionAreas[TF_TEAM_GREEN], &this->m_InvasionAreas[TF_TEAM_YELLOW] );
	this->ForAllCompletelyVisibleTFAreas(func2);
}

bool CTFNavArea::IsAwayFromInvasionAreas(int team, float dist) const
{
	for (auto area : this->GetInvasionAreas(team)) {
		if (this->GetCenter().DistToSqr(area->GetCenter()) < Square(dist)) {
			return false;
		}
	}
	
	return true;
}

const CUtlVector<CTFNavArea *>& CTFNavArea::GetInvasionAreas(int team) const
{
	Assert( team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT );
	if ( team < FIRST_GAME_TEAM || team >= TF_TEAM_COUNT ) team = TEAM_UNASSIGNED;
	return this->m_InvasionAreas[team];
}


float CTFNavArea::GetCombatIntensity() const
{
	return Max(0.0f, this->m_flCombatLevel - (this->m_itCombatTime.GetElapsedTime() * tf_nav_combat_decay_rate.GetFloat()));
}

void CTFNavArea::OnCombat(int mult)
{
	this->m_flCombatLevel = Min(1.0f, this->m_flCombatLevel + (mult * tf_nav_combat_build_rate.GetFloat()));
	this->m_itCombatTime.Start();
}

void CTFNavArea::AddCombatToSurroundingAreas(int mult)
{
	// TODO: probably should just move this cvar from tf_player.cpp into this file;
	// frankly I have basically no idea why the TF team decided to put it over there
	extern ConVar tf_nav_in_combat_range;
	
	CUtlVector<CTFNavArea *> areas;
	CollectSurroundingTFAreas(&areas, this, tf_nav_in_combat_range.GetFloat(), StepHeight, StepHeight);
	
	for (auto area : areas) {
		area->OnCombat(mult);
	}
}


uint64 CTFNavArea::GetAttr_SpawnRoom(int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	
	switch (team) {
	case TF_TEAM_RED:    return RED_SPAWN_ROOM;
	case TF_TEAM_BLUE:   return BLUE_SPAWN_ROOM;
	case TF_TEAM_GREEN:  return GREEN_SPAWN_ROOM;
	case TF_TEAM_YELLOW: return YELLOW_SPAWN_ROOM;
	}
	
	return 0;
}

uint64 CTFNavArea::GetAttr_Sentry(int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	
	switch (team) {
	case TF_TEAM_RED:    return RED_SENTRY;
	case TF_TEAM_BLUE:   return BLUE_SENTRY;
	case TF_TEAM_GREEN:  return GREEN_SENTRY;
	case TF_TEAM_YELLOW: return YELLOW_SENTRY;
	}
	
	return 0;
}

uint64 CTFNavArea::GetAttr_SetupGate(int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	
	switch (team) {
	case TF_TEAM_RED:    return RED_SETUP_GATE;
	case TF_TEAM_BLUE:   return BLUE_SETUP_GATE;
	case TF_TEAM_GREEN:  return GREEN_SETUP_GATE;
	case TF_TEAM_YELLOW: return YELLOW_SETUP_GATE;
	}
	
	return 0;
}

uint64 CTFNavArea::GetAttr_OneWayDoor(int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	
	switch (team) {
	case TF_TEAM_RED:    return RED_ONE_WAY_DOOR;
	case TF_TEAM_BLUE:   return BLUE_ONE_WAY_DOOR;
	case TF_TEAM_GREEN:  return GREEN_ONE_WAY_DOOR;
	case TF_TEAM_YELLOW: return YELLOW_ONE_WAY_DOOR;
	}
	
	return 0;
}

uint64 CTFNavArea::GetAttr_RRVisualizer(int team)
{
	Assert(team >= FIRST_GAME_TEAM && team < TF_TEAM_COUNT);
	
	switch (team) {
	case TF_TEAM_RED:    return RED_RR_VISUALIZER;
	case TF_TEAM_BLUE:   return BLUE_RR_VISUALIZER;
	case TF_TEAM_GREEN:  return GREEN_RR_VISUALIZER;
	case TF_TEAM_YELLOW: return YELLOW_RR_VISUALIZER;
	}
	
	return 0;
}


ScriptClassDesc_t *CTFNavArea::GetScriptDesc( void )
{
	return ::GetScriptDesc( this );
}

Vector CTFNavArea::FindRandomSpot( void )
{
	static Vector spot;
	if ( GetSizeX() >= 50.0f && GetSizeY() >= 50.0f )
	{
		spot.x = m_nwCorner.x + 25.0f + RandomFloat( 0, GetSizeX() - 50.0f );
		spot.y = m_seCorner.y + 25.0f + RandomFloat( 0, GetSizeY() - 50.0f );
		spot.z = GetZ( spot.x, spot.y ) + 10.0f;
		return spot;
	}
	else
	{
		spot = GetCenter() + Vector(0, 0, 10);
	}
	return spot;
}

bool CTFNavArea::IsBottleneck( void )
{
	if ( GetAdjacentCount( NORTH ) > 0 && GetAdjacentCount( SOUTH ) > 0 && GetAdjacentCount( EAST ) == 0 && GetAdjacentCount( WEST ) == 0 )
	{
		if ( GetSizeX() < 52.5 )
			return true;
	}

	if ( GetAdjacentCount( NORTH ) == 0 && GetAdjacentCount( SOUTH ) == 0 && GetAdjacentCount( EAST ) > 0 && GetAdjacentCount( WEST ) > 0 )
	{
		if ( GetSizeY() < 52.5 )
			return true;
	}

	return false;
}

void CTFNavArea::ClearAllPVActors()
{
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_PVActors[i].RemoveAll();
	}
}

void CTFNavArea::ClearAllInvasionAreas()
{
	for (int i = 0; i < TF_TEAM_COUNT; ++i) {
		this->m_InvasionAreas[i].RemoveAll();
	}
}
