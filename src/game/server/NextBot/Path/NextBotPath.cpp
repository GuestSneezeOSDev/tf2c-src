/* NextBotPath
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotPath.h"
#include "NextBotUtil.h"
#include "nav_area.h"
#include "vprof.h"
#include "fmtstr.h"


static ConVar NextBotPathDrawIncrement         ("nb_path_draw_inc",                 "100", FCVAR_CHEAT);
static ConVar NextBotPathDrawSegmentCount      ("nb_path_draw_segment_count",       "100", FCVAR_CHEAT);
static ConVar NextBotPathSegmentInfluenceRadius("nb_path_segment_influence_radius", "100", FCVAR_CHEAT);


float Path::GetLength() const
{
	if (!this->IsValid()) {
		return 0.0f;
	}
	
	return this->m_Segments[this->m_iSegCount - 1].distanceFromStart;
}


/* give me the location of the point at distance 'dist' along the path, measured
 * from the start of 'seg' (or the start of the path, if 'seg' is null) */
const Vector& Path::GetPosition(float dist, const Segment *seg) const
{
	if (!this->IsValid()) {
		return vec3_origin;
	}
	
	Vector  pos;
	Vector dpos;
	
	float  s = 0.0f;
	float ds = 0.0f;
	
	if (seg == nullptr) {
		seg = &this->m_Segments[0];
	} else {
		s = seg->distanceFromStart;
	}
	
	if (seg->distanceFromStart > dist) {
		return vec3_origin;
	}
	
	const Segment *seg_this = seg;
	const Segment *seg_next;
	
	while ((seg_next = this->NextSegment(seg_this)) != nullptr) {
		ds   = seg_this->length;
		pos  = seg_this->pos;
		dpos = seg_next->pos - pos;
		
		if (s + ds >= dist) {
			float fraction = (dist - s) / ds;
			this->m_vecGetPosition = pos + (dpos * fraction);
			return this->m_vecGetPosition;
		}
		
		s += ds;
		seg_this = seg_next;
	}
	
	return seg_this->pos;
}

/* get the closest point on the path to 'near', starting at segment 'seg' and
 * continuing for distance 'dist' along the path; if 'seg' is null, start at the
 * beginning of the path, and if 'dist' is zero, go all the way to the end of
 * the path */
const Vector& Path::GetClosestPosition(const Vector& near, const Segment *seg, float dist) const
{
	if (seg == nullptr) {
		seg = &this->m_Segments[0];
	}
	
	/* this isn't realistically possible */
	if (seg == nullptr) {
		return near;
	}
	
	this->m_vecGetClosestPosition = near;
	
	float closest_dist_sqr = 1.0e+11f;
	float s = 0.0f;
	
	const Segment *seg_this = seg;
	const Segment *seg_next;
	
	while ((seg_next = this->NextSegment(seg_this)) != nullptr) {
		if (dist != 0.0f && s > dist) {
			break;
		}
		
		Vector point;
		CalcClosestPointOnLineSegment(near, seg_this->pos, seg_next->pos, point);
		float this_dist_sqr = point.DistToSqr(near);
		
		if (this_dist_sqr < closest_dist_sqr) {
			this->m_vecGetClosestPosition = point;
			closest_dist_sqr = this_dist_sqr;
		}
		
		s += seg_this->length;
		seg_this = seg_next;
	}
	
	return this->m_vecGetClosestPosition;
}

const Vector& Path::GetStartPosition() const
{
	if (!this->IsValid()) {
		return vec3_origin;
	}
	
	return this->m_Segments[0].pos;
}

const Vector& Path::GetEndPosition() const
{
	if (!this->IsValid()) {
		return vec3_origin;
	}
	
	return this->m_Segments[this->m_iSegCount - 1].pos;
}


/* move the cursor to the closest point on the path to 'near', starting either
 * at the beginning of the path or the current cursor position based on 'stype',
 * and continuing for distance 'dist' along the path */
void Path::MoveCursorToClosestPosition(const Vector& near, SeekType stype, float dist) const
{
	if (!this->IsValid() || stype >= SEEK_MAX) {
		return;
	}
	
	const Segment *seg_this = &this->m_Segments[0];
	const Segment *seg_next;
	
	if (stype == SEEK_FROM_CURSOR) {
		seg_this = this->m_CursorData.seg;
		if (seg_this == nullptr) {
			seg_this = &this->m_Segments[0];
		}
	}
	
	this->m_CursorData.pos = near;
	this->m_CursorData.seg = seg_this;
	
	float closest_dist_sqr = 1.0e+11f;
	float s = 0.0f;
	
	while ((seg_next = this->NextSegment(seg_this)) != nullptr) {
		if (dist != 0.0f && s > dist) {
			break;
		}
		
		Vector point;
		CalcClosestPointOnLineSegment(near, seg_this->pos, seg_next->pos, point);
		float this_dist_sqr = point.DistToSqr(near);
		
		if (this_dist_sqr < closest_dist_sqr) {
			this->m_CursorData.pos = point;
			this->m_CursorData.seg = seg_this;
			closest_dist_sqr = this_dist_sqr;
		}
		
		s += seg_this->length;
		seg_this = seg_next;
	}
	
	const Segment *seg_cur = this->m_CursorData.seg;
	this->m_flCursorPosition = seg_cur->distanceFromStart + seg_cur->pos.DistTo(this->m_CursorData.pos);
	this->m_bCursorDataDirty = true;
}

void Path::MoveCursorToStart()
{
	this->m_flCursorPosition = 0.0f;
	this->m_bCursorDataDirty = true;
}

void Path::MoveCursorToEnd()
{
	this->m_flCursorPosition = this->GetLength();
	this->m_bCursorDataDirty = true;
}

void Path::MoveCursor(float dist, MoveCursorType mctype)
{
	if (mctype == MOVECUR_REL) {
		dist += this->m_flCursorPosition;
	}
	
	this->m_flCursorPosition = dist;
	
	if (dist < 0.0f) {
		this->m_flCursorPosition = 0.0f;
	} else if (dist > this->GetLength()) {
		this->m_flCursorPosition = this->GetLength();
	}
	
	this->m_bCursorDataDirty = true;
}


void Path::Invalidate()
{
	this->m_iSegCount = 0;
	
	this->m_CursorData.pos       = vec3_origin;
	this->m_CursorData.forward   = Vector(1.0f, 0.0f, 0.0f);
	this->m_CursorData.curvature = 0.0f;
	this->m_CursorData.seg       = nullptr;
	
	this->m_bCursorDataDirty = true;
	this->m_hSubject = nullptr;
}


void Path::Draw(const Segment *seg) const
{
	if (!this->IsValid()) {
		return;
	}
	
	int draw_seg_count = NextBotPathDrawSegmentCount.GetInt();
	
	if (seg == nullptr) {
		seg = this->FirstSegment();
		
		if (seg == nullptr) {
			return;
		}
	}
	
	const Segment *seg_this = seg;
	const Segment *seg_next;
	
	for (int i = 0; i < draw_seg_count; ++i) {
		if ((seg_next = this->NextSegment(seg_this)) == nullptr) {
			return;
		}
		
		int dx = abs((int)(seg_next->pos.x - seg_this->pos.x));
		int dy = abs((int)(seg_next->pos.y - seg_this->pos.y));
		int dz = abs((int)(seg_next->pos.z - seg_this->pos.z));
		
		int dxy = Max(dx, dy);
		
		int r, g, b;
		switch (seg_this->type) {
		default:
		case SEG_GROUND:
			r = 0xff; g = 0x4d; b = 0x00; break; // orange
		case SEG_FALL:
			r = 0xff; g = 0x00; b = 0xff; break; // magenta
		case SEG_CLIMBJUMP:
			r = 0x00; g = 0x00; b = 0xff; break; // blue
		case SEG_GAPJUMP:
			r = 0x00; g = 0xff; b = 0xff; break; // cyan
		case SEG_LADDER_UP:
			r = 0x00; g = 0xff; b = 0x00; break; // green
		case SEG_LADDER_DOWN:
			r = 0x00; g = 0x64; b = 0x00; break; // dark green
		}
		
		if (seg_this->ladder != nullptr) {
			NDebugOverlay::VertArrow(seg_this->ladder->m_bottom, seg_this->ladder->m_top, 5.0f, r, g, b, 0xff, true, 0.1f);
		} else {
			NDebugOverlay::Line(seg_this->pos, seg_next->pos, r, g, b, true, 0.1f);
		}
		
		if (dz >= dxy) {
			NDebugOverlay::VertArrow(seg_this->pos, seg_this->pos + (25.0f * seg_this->forward), 5.0f, r, g, b, 0xff, true, 0.1f);
		} else {
			NDebugOverlay::HorzArrow(seg_this->pos, seg_this->pos + (25.0f * seg_this->forward), 5.0f, r, g, b, 0xff, true, 0.1f);
		}
		
		NDebugOverlay::Text(seg_this->pos, CFmtStr("%d", i), true, 0.1f);
		
		seg_this = seg_next;
	}
}

void Path::DrawInterpolated(float from, float to)
{
	if (!this->IsValid()) {
		return;
	}
	
	Vector v_begin = this->GetCursorData().pos;
	float dist = from;
	
	do {
		dist += NextBotPathDrawIncrement.GetFloat();
		
		this->MoveCursor(dist, MOVECUR_ABS);
		const CursorData& cur_data = this->GetCursorData();
		
		// TODO: better name for this
		float curvature_x3 = cur_data.curvature * 3.0f;
		
		int r = (int)(255.0f * (1.0f - curvature_x3));
		int g = (int)(255.0f * (1.0f + curvature_x3));
		
		NDebugOverlay::Line(v_begin, cur_data.pos, Clamp(r, 0, 0xff), Clamp(g, 0, 0xff), 0, true, 0.1f);
		
		v_begin = cur_data.pos;
	} while (dist < to);
}


const Path::Segment *Path::FirstSegment() const
{
	if (!this->IsValid()) {
		return nullptr;
	}
	
	return &this->m_Segments[0];
}

const Path::Segment *Path::NextSegment(const Segment *seg) const
{
	if (seg == nullptr || !this->IsValid()) {
		return nullptr;
	}
	
	int idx = (seg - this->m_Segments);
	if (idx >= 0 && idx < (this->m_iSegCount - 1)) {
		return &this->m_Segments[idx + 1];
	} else {
		return nullptr;
	}
}

const Path::Segment *Path::PriorSegment(const Segment *seg) const
{
	if (seg == nullptr || !this->IsValid()) {
		return nullptr;
	}
	
	int idx = (seg - this->m_Segments);
	if (idx > 0 && idx < this->m_iSegCount) {
		return &this->m_Segments[idx - 1];
	} else {
		return nullptr;
	}
}

const Path::Segment *Path::LastSegment() const
{
	if (!this->IsValid()) {
		return nullptr;
	}
	
	return &this->m_Segments[this->m_iSegCount - 1];
}


void Path::Copy(INextBot *nextbot, const Path& that)
{
	VPROF_BUDGET("Path::Copy", "NextBot");
	
	this->Invalidate();
	
	for (int i = 0; i < that.m_iSegCount; ++i) {
		this->m_Segments[i] = that.m_Segments[i];
	}
	
	this->m_iSegCount = that.m_iSegCount;
	this->OnPathChanged(nextbot, RESULT_SUCCESS);
}


bool Path::ComputeWithOpenGoal(INextBot *nextbot, const IPathCost& cost_func, const IPathOpenGoalSelector& sel, float f1)
{
	VPROF_BUDGET("ComputeWithOpenGoal", "NextBot");
	
	int nb_teamnum = nextbot->GetEntity()->GetTeamNumber();
	
	CNavArea *nb_area = nextbot->GetEntity()->GetLastKnownArea();
	if (nb_area == nullptr) {
		return false;
	}
	nb_area->SetParent(nullptr);
	
	CNavArea::ClearSearchLists();
	
	float cost = cost_func(nb_area, nullptr);
	if (cost < 0.0f) {
		return false;
	}
	nb_area->SetTotalCost(cost);
	nb_area->AddToOpenList();
	
	// TODO: verify that "goal" is actually an appropriate name for this
	CNavArea *goal = nullptr;
	
	CNavArea *area;
	while ((area = CNavArea::PopOpenList()) != nullptr) {
		area->Mark();
		
		if (area->IsBlocked(nb_teamnum)) {
			continue;
		}
		
		this->CollectAdjacentAreas(area);
		
		for (int i = 0; i < this->m_AdjacentAreaCount; ++i) {
			AdjacentArea& adj_area = this->m_AdjacentAreas[i];
			
			if (adj_area.area->IsClosed())            continue; // TODO: I think this is wrong
			if (adj_area.area->IsBlocked(nb_teamnum)) continue;
			
			if (f1 == 0.0f || !(adj_area.area->GetCenter() - nextbot->GetEntity()->GetAbsOrigin()).IsLengthGreaterThan(f1)) {
				cost = cost_func(adj_area.area, area, adj_area.ladder);
				if (cost >= 0.0f) {
					if (!adj_area.area->IsOpen()) {
						adj_area.area->SetTotalCost(cost);
						adj_area.area->SetCostSoFar(cost);
						adj_area.area->SetParent(area, adj_area.how);
						adj_area.area->AddToOpenList();
						
						goal = sel(goal, adj_area.area);
					} else if (cost < adj_area.area->GetTotalCost()) {
						adj_area.area->SetTotalCost(cost);
						adj_area.area->SetCostSoFar(cost);
						adj_area.area->SetParent(area, adj_area.how);
						adj_area.area->UpdateOnOpenList();
						
						goal = sel(goal, adj_area.area);
					}
				}
			}
		}
	}
	
	if (goal != nullptr) {
		this->AssemblePrecomputedPath(nextbot, goal->GetCenter(), goal);
		return true;
	} else {
		return false;
	}
}

void Path::ComputeAreaCrossing(INextBot *nextbot, const CNavArea *area, const Vector& from, const CNavArea *to, NavDirType dir, Vector *out) const
{
	area->ComputeClosestPointInPortal(to, dir, from, out);
}


void Path::AssemblePrecomputedPath(INextBot *nextbot, const Vector& vec, CNavArea *area)
{
	VPROF_BUDGET("Path::AssemblePrecomputedPath", "NextBot");
	
	const Vector& nb_pos = nextbot->GetPosition();
	if (area != nullptr) {
		int n = 0;
		for (CNavArea *parent = area; parent != nullptr; parent = parent->GetParent()) {
			++n;
		}
		
		if (n < MAX_SEGMENTS) {
			if (n == 1) {
				this->BuildTrivialPath(nextbot, vec);
				return;
			}
		} else {
			n = MAX_SEGMENTS - 1;
		}
		
		this->m_iSegCount = n;
		
		int i = n;
		for (CNavArea *parent = area; parent != nullptr; parent = parent->GetParent()) {
			--i;
			Segment& seg = this->m_Segments[i];
			
			seg.area = parent;
			seg.type = SEG_GROUND;
			seg.how  = parent->GetParentHow();
			
			if (i == 0) break;
		}
		
		Segment& seg_last = this->m_Segments[n];
		seg_last.area   = area;
		seg_last.pos    = vec;
		seg_last.ladder = nullptr;
		seg_last.how    = NUM_TRAVERSE_TYPES;
		seg_last.type   = SEG_GROUND;
		
		++this->m_iSegCount;
		
		if (this->ComputePathDetails(nextbot, nb_pos)) {
			this->Optimize(nextbot);
			this->PostProcess();
			this->OnPathChanged(nextbot, RESULT_SUCCESS);
		} else {
			this->Invalidate();
			this->OnPathChanged(nextbot, RESULT_FAIL2);
		}
	}
}

bool Path::BuildTrivialPath(INextBot *nextbot, const Vector& dest)
{
	const Vector& nb_pos = nextbot->GetPosition();
	
	this->m_iSegCount = 0;
	
	CNavArea *nb_area = TheNavMesh->GetNearestNavArea(nb_pos);
	if (nb_area == nullptr) return false;
	
	CNavArea *dest_area = TheNavMesh->GetNearestNavArea(dest);
	if (dest_area == nullptr) return false;
	
	this->m_iSegCount = 2;
	
	this->m_Segments[0].area   = nb_area;
	this->m_Segments[0].pos.x  = nb_pos.x;
	this->m_Segments[0].pos.y  = nb_pos.y;
	this->m_Segments[0].pos.z  = nb_area->GetZ(nb_pos.x, nb_pos.y);
	this->m_Segments[0].ladder = nullptr;
	this->m_Segments[0].how    = NUM_TRAVERSE_TYPES;
	this->m_Segments[0].type   = SEG_GROUND;
	
	this->m_Segments[1].area   = dest_area;
	this->m_Segments[1].pos.x  = dest.x;
	this->m_Segments[1].pos.y  = dest.y;
	this->m_Segments[1].pos.z  = dest_area->GetZ(dest.x, dest.y);
	this->m_Segments[1].ladder = nullptr;
	this->m_Segments[1].how    = NUM_TRAVERSE_TYPES;
	this->m_Segments[1].type   = SEG_GROUND;
	
	this->m_Segments[0].distanceFromStart = 0.0f;
	this->m_Segments[0].curvature         = 0.0f;
	
	this->m_Segments[1].length    = 0.0f;
	this->m_Segments[1].curvature = 0.0f;
	
	Vector fwd = this->m_Segments[1].pos - this->m_Segments[0].pos;
	float len = VectorNormalize(fwd);
	
	this->m_Segments[0].forward = fwd;
	this->m_Segments[0].length  = len;
	
	this->m_Segments[1].forward           = fwd;
	this->m_Segments[1].distanceFromStart = len;
	
	this->OnPathChanged(nextbot, RESULT_SUCCESS);
	
	return true;
}

void Path::CollectAdjacentAreas(CNavArea *area)
{
	this->m_AdjacentAreaCount = 0;
	
	const NavConnectVector& v_north = *area->GetAdjacentAreas(NORTH);
	for (int i = 0; i < v_north.Count() && this->m_AdjacentAreaCount < MAX_ADJ_AREAS; ++i) {
		this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
			v_north[i].area,
			nullptr,
			GO_NORTH,
		};
	}
	
	const NavConnectVector& v_south = *area->GetAdjacentAreas(SOUTH);
	for (int i = 0; i < v_south.Count() && this->m_AdjacentAreaCount < MAX_ADJ_AREAS; ++i) {
		this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
			v_south[i].area,
			nullptr,
			GO_SOUTH,
		};
	}
	
	const NavConnectVector& v_west = *area->GetAdjacentAreas(WEST);
	for (int i = 0; i < v_west.Count() && this->m_AdjacentAreaCount < MAX_ADJ_AREAS; ++i) {
		this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
			v_west[i].area,
			nullptr,
			GO_WEST,
		};
	}
	
	const NavConnectVector& v_east = *area->GetAdjacentAreas(EAST);
	for (int i = 0; i < v_east.Count() && this->m_AdjacentAreaCount < MAX_ADJ_AREAS; ++i) {
		this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
			v_east[i].area,
			nullptr,
			GO_EAST,
		};
	}
	
	const NavLadderConnectVector& v_up = *area->GetLadders(CNavLadder::LADDER_UP);
	for (int i = 0; i < v_up.Count(); ++i) {
		CNavLadder *ladder = v_up[i].ladder;
		
		if (ladder->m_topForwardArea != nullptr && this->m_AdjacentAreaCount < MAX_ADJ_AREAS) {
			this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
				ladder->m_topForwardArea,
				ladder,
				GO_LADDER_UP,
			};
		}
		
		if (ladder->m_topLeftArea != nullptr && this->m_AdjacentAreaCount < MAX_ADJ_AREAS) {
			this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
				ladder->m_topLeftArea,
				ladder,
				GO_LADDER_UP,
			};
		}
		
		if (ladder->m_topRightArea != nullptr && this->m_AdjacentAreaCount < MAX_ADJ_AREAS) {
			this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
				ladder->m_topRightArea,
				ladder,
				GO_LADDER_UP,
			};
		}
	}
	
	const NavLadderConnectVector& v_down = *area->GetLadders(CNavLadder::LADDER_DOWN);
	for (int i = 0; i < v_down.Count() && this->m_AdjacentAreaCount < MAX_ADJ_AREAS; ++i) {
		CNavLadder *ladder = v_down[i].ladder;
		
		if (ladder->m_bottomArea != nullptr) {
			this->m_AdjacentAreas[this->m_AdjacentAreaCount++] = {
				ladder->m_bottomArea,
				ladder,
				GO_LADDER_DOWN,
			};
		}
	}
}

// TODO: after finishing the initial RE pass on this func with ServerWin,
// go over the ENTIRE function with ServerLinux to verify we didn't miss anything
bool Path::ComputePathDetails(INextBot *nextbot, const Vector& vec)
{
	VPROF_BUDGET("Path::ComputePathDetails", "NextBot");
	
	if (this->m_iSegCount == 0) return false;
	
	IBody *body       = nextbot->GetBodyInterface();
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	float step_height;
	if (loco != nullptr) {
		step_height = loco->GetStepHeight();
	} else {
		// TODO: verify that StepHeight (18.0f from nav.h) is valid for TF
		step_height = StepHeight;
	}
	
	float hull_width;
	if (body != nullptr) {
		hull_width = body->GetHullWidth() + 5.0f; // TODO: magic number
	} else {
		hull_width = 1.0f; // TODO: magic number
	}
	
	if (this->m_Segments[0].area->Contains(vec)) {
		this->m_Segments[0].pos = vec;
	} else {
		this->m_Segments[0].pos = this->m_Segments[0].area->GetCenter();
	}
	
	this->m_Segments[0].ladder = nullptr;
	this->m_Segments[0].how    = NUM_TRAVERSE_TYPES;
	this->m_Segments[0].type   = SEG_GROUND;
	
	if (this->m_iSegCount <= 1) return true;
	
	for (int i = 1; i < this->m_iSegCount; ++i) {
		Segment *seg_this = &this->m_Segments[i];
		Segment *seg_prev = &this->m_Segments[i - 1];
		
		switch (seg_this->how) {
		
		case GO_NORTH:
		case GO_EAST:
		case GO_SOUTH:
		case GO_WEST:
		{
			seg_this->ladder = nullptr;
			
			seg_prev->area->ComputePortal(seg_this->area, (NavDirType)seg_this->how, &seg_this->m_portalCenter, &seg_this->m_portalHalfWidth);
			
			this->ComputeAreaCrossing(nextbot, seg_prev->area, seg_prev->pos, seg_this->area, (NavDirType)seg_this->how, &seg_this->pos);
			seg_this->pos.z = seg_prev->area->GetZ(seg_this->pos.x, seg_this->pos.y);
			
			float z_prev = seg_prev->area->GetZ(seg_prev->pos.x, seg_prev->pos.y);
			float z_this = seg_this->area->GetZ(seg_this->pos.x, seg_this->pos.y);
			
			Vector norm;
			seg_prev->area->ComputeNormal(&norm);
			
			Vector delta = Vector(seg_this->pos.x, seg_this->pos.y, z_this) - Vector(seg_prev->pos.x, seg_prev->pos.y, z_prev);
			
			if (-DotProduct(delta, norm) > loco->GetStepHeight()) {
				Vector2D dir;
				DirectionToVector2D((NavDirType)seg_this->how, &dir);
				
				float double_hull_width = hull_width * 2.0f;
				float half_hull_width   = hull_width * 0.5f;
				
				float crouch_hull_height;
				if (body != nullptr) {
					crouch_hull_height = body->GetCrouchHullHeight();
				} else {
					crouch_hull_height = 1.0f; // TODO: magic number
				}
				
				Vector rayStart;
				Vector rayEnd;
				
				Vector rayMins(-half_hull_width, -half_hull_width, step_height);
				Vector rayMaxs( half_hull_width,  half_hull_width, crouch_hull_height);
				
				float multiplier = 0.0f;
				do {
					rayStart = seg_this->pos + Vector2DTo3D(multiplier * dir);
					
					rayEnd = rayStart;
					rayEnd.z = z_this;
					
					trace_t tr;
					UTIL_TraceHull(rayStart, rayEnd, rayMins, rayMaxs, nextbot->GetBodyInterface()->GetSolidMask(), NextBotTraceFilterIgnoreActors(nextbot), &tr);
					
					if (tr.fraction >= 1.0f) break;
					
					multiplier += 10.0f;
				} while (multiplier <= double_hull_width);
				
				if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
					NDebugOverlay::Cross3D(rayStart, 5.0f, NB_RGB_MAGENTA, true, 5.0f);
					NDebugOverlay::Cross3D(rayEnd,   5.0f, NB_RGB_YELLOW, true, 5.0f);
					NDebugOverlay::VertArrow(rayStart, rayEnd, 5.0f, NB_RGBA_ORANGE_64, true, 5.0f);
				}
				
				float ground_height;
				if (TheNavMesh->GetGroundHeight(rayEnd, &ground_height) && ground_height + step_height < rayEnd.z) {
					seg_this->pos  = rayStart;
					seg_this->type = SEG_FALL;
					
					Segment seg_fall = *seg_this;
					seg_fall.pos.x = rayEnd.x;
					seg_fall.pos.y = rayEnd.y;
					seg_fall.pos.z = ground_height;
					seg_fall.type  = SEG_GROUND;
					
					this->InsertSegment(seg_fall, i);
					++i;
				}
			}
			
			break;
		}
		
		case GO_LADDER_UP:
		{
			bool found_ladder = false;
			
			const auto& ladders = *seg_prev->area->GetLadders(CNavLadder::LADDER_UP);
			FOR_EACH_VEC(ladders, j) {
				const CNavLadder *ladder = ladders[j].ladder;
				
				if (ladder->m_topForwardArea == seg_this->area ||
					ladder->m_topLeftArea    == seg_this->area ||
					ladder->m_topRightArea   == seg_this->area) {
					
					/* the magic number 32.0f seems to be equivalent to the
					 * minLadderClearance value in CNavMesh::CreateLadder */
					seg_this->ladder = ladder;
					seg_this->pos    = ladder->m_bottom + (32.0f * ladder->GetNormal());
					seg_this->type   = SEG_LADDER_UP;
					
					found_ladder = true;
					break;
				}
			}
			
			if (!found_ladder) return false;
			break;
		}
		
		case GO_LADDER_DOWN:
		{
			bool found_ladder = false;
			
			const auto& ladders = *seg_prev->area->GetLadders(CNavLadder::LADDER_DOWN);
			FOR_EACH_VEC(ladders, j) {
				const CNavLadder *ladder = ladders[j].ladder;
				
				if (ladder->m_bottomArea == seg_this->area) {
					/* the magic number 32.0f seems to be equivalent to the
					 * minLadderClearance value in CNavMesh::CreateLadder */
					seg_this->ladder = ladder;
					seg_this->pos    = ladder->m_top - (32.0f * ladder->GetNormal());
					seg_this->type   = SEG_LADDER_DOWN;
					
					found_ladder = true;
					break;
				}
			}
			
			if (!found_ladder) return false;
			break;
		}
		
		case GO_ELEVATOR_UP:
		case GO_ELEVATOR_DOWN:
		{
			seg_this->pos    = seg_this->area->GetCenter();
			seg_this->ladder = nullptr;
			
			break;
		}
		
		/* GO_JUMP:            not handled */
		/* NUM_TRAVERSE_TYPES: not handled */
		
		}
	}
	
	for (int i = 0; i < this->m_iSegCount - 1; ++i) {
		Segment *seg_this = &this->m_Segments[i];
		Segment *seg_next = &this->m_Segments[i + 1];
		
		if ((seg_this->how == NUM_TRAVERSE_TYPES || seg_this->how <= GO_WEST) &&
			seg_next->how <= GO_WEST && seg_next->type == SEG_GROUND) {
			// TODO: rename these
			Vector vec_next; seg_next->area->GetClosestPointOnArea(seg_this->pos, &vec_next);
			Vector vec_this; seg_this->area->GetClosestPointOnArea(vec_next, &vec_this);
			
			// BUG SITUATION:
			// we're ending up with some kind of memory corruption issue when
			// called from CTFBotSeekAndDestroy::RecomputeSeekPath via Path::Compute
			// basically, one of the path segments (index 0x1A is what I've seen)
			// gets overwritten with garbage that starts at [0x1A].forward and
			// continues partway through [0x1B]
			
			if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
				NDebugOverlay::Line(vec_this, vec_next, NB_RGB_MAGENTA, true, 5.0f);
			}
			
			Vector vec_delta = (vec_next - vec_this);
			
			// TODO: find source of magic number 47.5f
			if (vec_delta.Length2DSqr() > Square(47.5f) && vec_delta.Length2DSqr() > Square(0.5f * abs(vec_delta.z))) {
				// TODO: rename these
				Vector vec1; seg_next->area->GetClosestPointOnArea(seg_next->pos, &vec1);
				Vector vec2; seg_this->area->GetClosestPointOnArea(vec1, &vec2);
				
				// TODO: rename this
				Vector vec3 = (vec1 - vec2);
				VectorNormalize(vec3);
				vec3 *= (0.5f * hull_width);
				
				seg_next->pos = (vec1 + vec3);
				
				Segment seg_new = *seg_this;
				seg_new.type = SEG_GAPJUMP;
				seg_new.pos = (vec2 - vec3);
				
				this->InsertSegment(seg_new, i + 1);
				++i;
			} else {
				if (vec_delta.z > step_height) {
					seg_next->pos = seg_next->area->GetCenter();
					
					Segment seg_new = *seg_this;
					seg_new.type = SEG_CLIMBJUMP;
					seg_this->area->GetClosestPointOnArea(seg_next->pos, &seg_new.pos);
					
					if (nextbot->IsDebugging(INextBot::DEBUG_PATH)) {
						NDebugOverlay::Cross3D(seg_new.pos, 15.0f, NB_RGB_LTMAGENTA_64, true, 3.0f);
					}
					
					this->InsertSegment(seg_new, i + 1);
					++i;
				}
			}
		}
	}
	
	return true;
}

int Path::FindNextOccludedNode(INextBot *nextbot, int index)
{
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	if (loco == nullptr) {
		return this->m_iSegCount;
	}
	
	for (int i = index + 1; i < this->m_iSegCount; ++i) {
		Segment *seg = &this->m_Segments[i];
		
		if (seg->type != SEG_GROUND || seg->area->HasAttributes(NAV_MESH_PRECISE)) {
			return i;
		}
		
		// TODO: default value for IsPotentiallyTraversable param 3?
		if (!loco->IsPotentiallyTraversable(this->m_Segments[index].pos, this->m_Segments[i].pos, ILocomotion::TRAVERSE_DEFAULT) ||
			loco->HasPotentialGap(this->m_Segments[index].pos, this->m_Segments[i].pos)) {
			return i;
		}
	}
	
	return this->m_iSegCount;
}

void Path::InsertSegment(const Segment& seg, int index)
{
	if ((this->m_iSegCount + 1) >= MAX_SEGMENTS) {
		return;
	}
	
	if (index < this->m_iSegCount) {
		for (int i = this->m_iSegCount; i != index; --i) {
			/* member-by-member copy */
			this->m_Segments[i] = this->m_Segments[i - 1];
		}
	}
	
	/* member-by-member copy */
	this->m_Segments[index] = seg;
	
	++this->m_iSegCount;
}

void Path::PostProcess()
{
	VPROF_BUDGET("Path::PostProcess", "NextBot");
	
	this->m_itAge.Reset();
	
	if (this->m_iSegCount == 0) return;
	
	if (this->m_iSegCount == 1) {
		this->m_Segments[0].length            = 0.0f;
		this->m_Segments[0].distanceFromStart = 0.0f;
		this->m_Segments[0].curvature         = 0.0f;
		this->m_Segments[0].forward           = vec3_origin;
		return;
	}
	
	float dist_from_start = 0.0f;
	
	for (int i = 0; i < this->m_iSegCount - 1; ++i) {
		Segment *seg_this = &this->m_Segments[i];
		Segment *seg_next = &this->m_Segments[i + 1];
		
		Vector delta = seg_next->pos - seg_this->pos;
		float len = VectorNormalize(delta);
		
		seg_this->forward = delta;
		seg_this->length  = len;
		
		seg_this->distanceFromStart = dist_from_start;
		dist_from_start += len;
	}
	
	for (int i = 1; i < this->m_iSegCount - 1; ++i) {
		Segment *seg_prev = &this->m_Segments[i - 1];
		Segment *seg_this = &this->m_Segments[i];
		
		if (seg_this->type != SEG_GROUND) {
			seg_this->curvature = 0.0f;
			continue;
		}
		
		Vector2D prev_fwd_xy = seg_prev->forward.AsVector2D();
		Vector2DNormalize(prev_fwd_xy);
		
		Vector2D this_fwd_xy = seg_this->forward.AsVector2D();
		if (this_fwd_xy.Length() != 0.0f) {
			Vector2DNormalize(this_fwd_xy);
			
			float dot1 = DotProduct2D(prev_fwd_xy, this_fwd_xy);
			float curv = (1.0f - dot1) * 0.5f;
			
			prev_fwd_xy.y *= -1.0f;
			float dot2 = DotProduct2D(prev_fwd_xy, this_fwd_xy);
			
			if (dot2 < 0.0f) {
				seg_this->curvature = curv;
			} else {
				seg_this->curvature = -curv;
			}
		} else {
			seg_this->curvature = 1.0f;
		}
	}
	
	this->m_Segments[0].curvature = 0.0f;
	
	Segment *seg_next_to_last = &this->m_Segments[this->m_iSegCount - 2];
	Segment *seg_last         = &this->m_Segments[this->m_iSegCount - 1];
	
	seg_last->forward           = seg_next_to_last->forward;
	seg_last->length            = 0.0f;
	seg_last->distanceFromStart = dist_from_start;
}
