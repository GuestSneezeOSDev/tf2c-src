/* NextBotVisionInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotVisionInterface.h"
#include "NextBotKnownEntity.h"
#include "NextBotInterface.h"
#include "NextBotUtil.h"
#include "vprof.h"
#include "functorutils.h"
#include "nav_area.h"


static ConVar nb_blind               ("nb_blind",                "0", FCVAR_CHEAT, "Disable vision");
static ConVar nb_debug_known_entities("nb_debug_known_entities", "0", FCVAR_CHEAT, "Show the 'known entities' for the bot that is the current spectator target");


void IVision::Reset()
{
	INextBotComponent::Reset();
	
	this->m_KnownEntities.RemoveAll();
	this->m_flLastUpdate   = 0.0f;
	this->m_hPrimaryThreat = nullptr;
	
	this->SetFieldOfView(this->GetDefaultFieldOfView());
	
	for (int i = 0; i < MAX_TEAMS; ++i) {
		this->m_itTeamVisible[i].Invalidate();
	}
}

void IVision::Update()
{
	VPROF_BUDGET("IVision::Update", "NextBotExpensive");
	
	if (nb_blind.GetBool()) {
		this->m_KnownEntities.RemoveAll();
	} else {
		this->UpdateKnownEntities();
		this->m_flLastUpdate = gpGlobals->curtime;
	}
}


bool IVision::ForEachKnownEntity(IForEachKnownEntity& functor)
{
	for (auto& known : this->m_KnownEntities) {
		if (!known.IsObsolete() && known.GetTimeSinceBecameKnown() >= this->GetMinRecognizeTime()) {
			if (!functor.Inspect(known)) {
				return false;
			}
		}
	}
	
	return true;
}

void IVision::CollectKnownEntities(CUtlVector<CKnownEntity> *knowns)
{
	if (knowns != nullptr) {
		knowns->RemoveAll();
		
		for (auto& known : this->m_KnownEntities) {
			knowns->AddToTail(known);
		}
	}
}

const CKnownEntity *IVision::GetPrimaryKnownThreat(bool only_recently_visible) const
{
	if (this->m_KnownEntities.IsEmpty()) return nullptr;
	
	const CKnownEntity *threat = nullptr;
	for (const auto& known : this->m_KnownEntities) {
		if (known.IsObsolete())                                            continue;
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) continue;
		if (this->IsIgnored(known.GetEntity()))                            continue;
		if (!this->GetBot()->IsEnemy(known.GetEntity()))                   continue;
		if (only_recently_visible && !known.IsVisibleRecently())           continue;
		
		if (threat == nullptr) {
			threat = &known;
		} else {
			threat = this->GetBot()->GetIntentionInterface()->SelectMoreDangerousThreat(this->GetBot(), this->GetBot()->GetEntity(), threat, &known);
		}
	}
	
	if (threat != nullptr) {
		this->m_hPrimaryThreat = threat->GetEntity();
	} else {
		this->m_hPrimaryThreat = nullptr;
	}
	
	return threat;
}

float IVision::GetTimeSinceVisible(int teamnum) const
{
	if (teamnum == TEAM_ANY) {
		for (int i = 0; i < MAX_TEAMS; ++i) {
			const IntervalTimer *timer = this->m_itTeamVisible + i;
			if (timer->IsLessThan(1e+10f) && timer->HasStarted()) {
				// TODO take a second look at this in linux and win builds
				/* this code doesn't make any sense, it's probably unfinished */
				Assert(false);
			}
		}
		return 1.0e+10f;
	} else if (teamnum < MAX_TEAMS) {
		return this->m_itTeamVisible[teamnum].GetElapsedTime();
	} else {
		return 0.0f;
	}
}

const CKnownEntity *IVision::GetClosestKnown(int teamnum) const
{
	const CKnownEntity *closest = nullptr;
	float dist = 1.0e+9f;
	
	const Vector& pos_me = this->GetBot()->GetPosition();
	
	for (const auto& known : this->m_KnownEntities) {
		if (known.IsObsolete())                                                   continue;
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime())        continue;
		if (teamnum != TEAM_ANY && teamnum != known.GetEntity()->GetTeamNumber()) continue;
		
		const Vector& pos_them = known.GetLastKnownPosition();
		if (pos_me.DistTo(pos_them) < dist) {
			dist = pos_me.DistTo(pos_them);
			closest = &known;
		}
	}
	
	return closest;
}

int IVision::GetKnownCount(int teamnum, bool only_recently_visible, float range) const
{
	int count = 0;
	
	for (const auto& known : this->m_KnownEntities) {
		if (known.IsObsolete())                                                   continue;
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime())        continue;
		if (teamnum != TEAM_ANY && teamnum != known.GetEntity()->GetTeamNumber()) continue;
		if (only_recently_visible && !known.IsVisibleRecently())                  continue;
		
		if (range >= 0.0f && this->GetBot()->IsRangeLessThan(known.GetLastKnownPosition(), range)) {
			++count;
		}
	}
	
	return count;
}

const CKnownEntity *IVision::GetClosestKnown(INextBotEntityFilter&& filter) const
{
	const CKnownEntity *closest = nullptr;
	float dist = 1.0e+9f;
	
	const Vector& pos_me = this->GetBot()->GetPosition();
	
	for (const auto& known : this->m_KnownEntities) {
		if (known.IsObsolete())                                            continue;
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) continue;
		if (!filter.IsAllowed(known.GetEntity()))                          continue;
		
		const Vector& pos_them = known.GetLastKnownPosition();
		if (pos_me.DistTo(pos_them) < dist) {
			dist = pos_me.DistTo(pos_them);
			closest = &known;
		}
	}
	
	return closest;
}

const CKnownEntity *IVision::GetKnown(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return nullptr;
	}
	
	for (const auto& known : this->m_KnownEntities) {
		if (known.GetEntity() != nullptr && known.GetEntity() == ent && !known.IsObsolete()) {
			return &known;
		}
	}
	
	return nullptr;
}

void IVision::AddKnownEntity(CBaseEntity *ent)
{
	CKnownEntity known(ent);
	
	if (!this->m_KnownEntities.HasElement(known)) {
		this->m_KnownEntities.AddToTail(known);
	}
}

void IVision::ForgetEntity(CBaseEntity *ent)
{
	if (ent == nullptr) return;
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.GetEntity() != nullptr && known.GetEntity() == ent && !known.IsObsolete()) {
			this->m_KnownEntities.Remove(i);
			break;
		}
	}
}

void IVision::ForgetAllKnownEntities()
{
	this->m_KnownEntities.RemoveAll();
}

void IVision::CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents)
{
	class CollectFunctor : public IActorFunctor
	{
	public:
		CollectFunctor(CUtlVector<CBaseEntity *> *ents) :
			ents(ents) {}
		
		virtual bool operator()(CBaseCombatCharacter *them) override
		{
			ents->AddToTail(them);
			return true;
		}
		
	private:
		CUtlVector<CBaseEntity *> *ents;
	};
	
	CollectFunctor func(ents);
	ForEachActor(func);
}

bool IVision::IsAbleToSee(CBaseEntity *ent, FieldOfViewCheckType ctype, Vector *v1) const
{
	VPROF_BUDGET("IVision::IsAbleToSee", "NextBotExpensive");
	
	if (this->GetBot()->IsRangeGreaterThan(ent, this->GetMaxVisionRange()) ||
		this->GetBot()->GetEntity()->IsHiddenByFog(ent)) {
		return false;
	}
	
	if (ctype == USE_FOV && !this->IsInFieldOfView(ent)) {
		return false;
	}
	
	CBaseCombatCharacter *ent_cc = ent->MyCombatCharacterPointer();
	if (ent_cc != nullptr) {
		CNavArea *lastknown_ent  = ent_cc->GetLastKnownArea();
		CNavArea *lastknown_this = this->GetBot()->GetEntity()->GetLastKnownArea();
		
		// BUG: the code here uses precomputed nav mesh visibility information essentially as a rough first-pass filter
		// to rule out entities that are known to not even have a chance of being potentially visible.
		// The problem with this is that players who climb up on props or other parts of the map that are not covered by
		// nav areas (e.g. the rocks on mvm_bigrock) retain the LastKnownArea they had when they were still on the
		// ground, even if they are many 100's of HU away from that nav area.
		// So, in those cases, this function will return completely wrong information based on false assumptions, which
		// results in stuff like IVision::UpdateKnownEntities marking enemies that are RIGHT NEXT TO THE BOT as not
		// currently visible, and that of course is a major problem.
		// The solution to this problem might involve one or more of:
		// - simply reducing overzealous use of CNavArea::IsPotentiallyVisible and CNavArea::IsCompletelyVisible
		//   throughout the bot codebase
		// - overriding CBaseCombatCharacter::UpdateLastKnownArea so that in cases where it currently just returns, it
		//   instead first calls ClearLastKnownArea before returning (so that GetLastKnownArea will return nullptr,
		//   rather than a stale nav area ptr, in these cases where the player effectively goes off-mesh)
		if (lastknown_ent != nullptr && lastknown_this != nullptr && !lastknown_this->IsPotentiallyVisible(lastknown_ent)) {
			return false;
		}
	}
	
	return (this->IsLineOfSightClearToEntity(ent, nullptr) && this->IsVisibleEntityNoticed(ent));
}

bool IVision::IsAbleToSee(const Vector& vec, FieldOfViewCheckType ctype) const
{
	VPROF_BUDGET("IVision::IsAbleToSee", "NextBotExpensive");
	
	if (this->GetBot()->IsRangeGreaterThan(vec, this->GetMaxVisionRange()) ||
		this->GetBot()->GetEntity()->IsHiddenByFog(vec)) {
		return false;
	}
	
	if (ctype == USE_FOV && !this->IsInFieldOfView(vec)) {
		return false;
	}
	
	return this->IsLineOfSightClear(vec);
}

bool IVision::IsInFieldOfView(const Vector& vec) const
{
	const Vector& view_vec = this->GetBot()->GetBodyInterface()->GetViewVector();
	const Vector& eye_pos  = this->GetBot()->GetBodyInterface()->GetEyePosition();
	
	return PointWithinViewAngle(eye_pos, vec, view_vec, this->m_flCosHalfFOV);
}

bool IVision::IsInFieldOfView(CBaseEntity *ent) const
{
	return (this->IsInFieldOfView(ent->WorldSpaceCenter()) || this->IsInFieldOfView(ent->EyePosition())); 
}

void IVision::SetFieldOfView(float fov)
{
	this->m_flFOV        = fov;
	this->m_flCosHalfFOV = cosf(DEG2RAD(fov * 0.5f));
}

bool IVision::IsLineOfSightClear(const Vector& vec) const
{
	VPROF_BUDGET("IVision::IsLineOfSightClear", "NextBot");
	VPROF_INCREMENT_COUNTER("IVision::IsLineOfSightClear", 1);
	
	trace_t tr;
	UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(), vec, MASK_VISIBLE_AND_NPCS, NextBotVisionTraceFilter(this->GetBot()), &tr);
	
	return (tr.fraction >= 1.0f && !tr.startsolid);
}

bool IVision::IsLineOfSightClearToEntity(const CBaseEntity *ent, Vector *endpos) const
{
	VPROF_BUDGET("IVision::IsLineOfSightClearToEntity", "NextBot");
	
	NextBotTraceFilterIgnoreActors filter(ent);
	
	trace_t tr;
	UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(), ent->WorldSpaceCenter(), MASK_VISIBLE_AND_NPCS, &filter, &tr);
	
	if (tr.DidHit()) {
		UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(), ent->EyePosition(), MASK_VISIBLE_AND_NPCS, &filter, &tr);
		
		if (tr.DidHit()) {
			UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(), ent->GetAbsOrigin(), MASK_VISIBLE_AND_NPCS, &filter, &tr);
		}
	}
	
	if (endpos != nullptr) {
		*endpos = tr.endpos;
	}
	
	return (tr.fraction >= 1.0f && !tr.startsolid);
}

bool IVision::IsLookingAt(const Vector& vec, float cos_half_fov) const
{
	const Vector& view_vec = this->GetBot()->GetBodyInterface()->GetViewVector();
	const Vector& eye_pos  = this->GetBot()->GetBodyInterface()->GetEyePosition();
	
	return PointWithinViewAngle(eye_pos, vec, view_vec, cos_half_fov);
}

bool IVision::IsLookingAt(const CBaseCombatCharacter *who, float cos_half_fov) const
{
	return this->IsLookingAt(who->EyePosition(), cos_half_fov);
}


void IVision::UpdateKnownEntities()
{
	VPROF_BUDGET("IVision::UpdateKnownEntities", "NextBot");
	
	CUtlVector<CBaseEntity *> vec_maybe_visible;
	this->CollectPotentiallyVisibleEntities(&vec_maybe_visible);
	
	CUtlVector<CBaseEntity *> vec_visible;
	
	for (auto ent : vec_maybe_visible) {
		VPROF_BUDGET("IVision::UpdateKnownEntities( collect visible )", "NextBot");
		
		if (ent == nullptr)                            continue;
		if (this->IsIgnored(ent))                      continue;
		if (!ent->IsAlive())                           continue;
		if (ent == this->GetBot()->GetEntity())        continue;
		if (!this->IsAbleToSee(ent, USE_FOV, nullptr)) continue;
		
		vec_visible.AddToTail(ent);
	}
	
	{
		VPROF_BUDGET("IVision::UpdateKnownEntities( update status )", "NextBot");
		
		FOR_EACH_VEC(this->m_KnownEntities, i) {
			CKnownEntity& known = this->m_KnownEntities[i];
			
			if (known.GetEntity() == nullptr || known.IsObsolete()) {
				this->m_KnownEntities.Remove(i);
				--i;
				continue;
			}
			
			bool known_is_visible = false;
			for (auto ent : vec_visible) {
				if (known.GetEntity()->entindex() == ent->entindex()) {
					known_is_visible = true;
					break;
				}
			}
			
			if (known_is_visible) {
				known.UpdatePosition();
				known.UpdateVisibilityStatus(true);
				
				if (known.GetTimeSinceBecameVisible() >= this->GetMinRecognizeTime()) {
					if ((this->m_flLastUpdate - known.GetTimeWhenBecameVisible()) <= this->GetMinRecognizeTime()) {
						if (this->GetBot()->IsDebugging(INextBot::DEBUG_VISION)) {
							ConColorMsg(NB_COLOR_GREEN, "%3.2f: %s caught sight of %s(#%d)\n",
								gpGlobals->curtime, this->GetBot()->GetDebugIdentifier(),
								known.GetEntity()->GetClassname(), known.GetEntity()->entindex());
							
							NDebugOverlay::Line(this->GetBot()->GetBodyInterface()->GetEyePosition(),
								known.GetLastKnownPosition(), NB_RGB_YELLOW, false, 0.2f);
						}
						
						this->GetBot()->OnSight(known.GetEntity());
					}
				}
				
				this->m_itTeamVisible[known.GetEntity()->GetTeamNumber()].Start();
			} else {
				if (known.IsVisibleInFOVNow()) {
					known.UpdateVisibilityStatus(false);
					
					if (this->GetBot()->IsDebugging(INextBot::DEBUG_VISION)) {
						ConColorMsg(NB_COLOR_RED, "%3.2f: %s lost sight of %s(#%d)\n",
							gpGlobals->curtime, this->GetBot()->GetDebugIdentifier(),
							known.GetEntity()->GetClassname(), known.GetEntity()->entindex());
					}
					
					this->GetBot()->OnLostSight(known.GetEntity());
				}
				
				if (!known.HasLastKnownPositionBeenSeen() && this->IsAbleToSee(known.GetLastKnownPosition(), USE_FOV)) {
					known.MarkLastKnownPositionAsSeen();
				}
			}
		}
	}
	
	{
		VPROF_BUDGET("IVision::UpdateKnownEntities( new recognizes )", "NextBot");
		
		for (auto visible : vec_visible) {
			bool visible_already_known = false;
			for (auto& known : this->m_KnownEntities) {
				if (known.GetEntity() == visible) {
					visible_already_known = true;
					break;
				}
			}
			
			if (!visible_already_known) {
				CKnownEntity k_new(visible);
				k_new.UpdateVisibilityStatus(true);
				this->m_KnownEntities.AddToTail(k_new);
			}
		}
	}
	
	if (nb_debug_known_entities.GetBool()) {
		CBasePlayer *host = UTIL_GetListenServerHost();
		if (host != nullptr) {
			CBaseEntity *spec_target = host->GetObserverTarget();
			if (this->GetBot()->IsSelf(spec_target)) {
				CUtlVector<CKnownEntity> vec_known;
				this->CollectKnownEntities(&vec_known);
				
				for (auto& known : vec_known) {
					bool is_friend      = this->GetBot()->IsFriend(known.GetEntity());
					bool is_recognized  = (known.GetTimeSinceBecameKnown() >= this->GetMinRecognizeTime());
					bool is_visible_now = known.IsVisibleInFOVNow();
					
					float width;
					bool bright;
					
					if (is_recognized) {
						if (is_visible_now) {
							width = 5.0f;
							bright = true;
						} else {
							width = 2.0f;
							bright = false;
						}
					} else {
						width = 1.0f;
						bright = false;
					}
					
					int r, g, b;
					
					if (is_friend) {
						r = 0;
						g = (bright ? 0xff : 0x64);
						b = 0;
					} else {
						r = (bright ? 0xff : 0x64);
						g = 0;
						b = 0;
					}
					
					NDebugOverlay::HorzArrow(this->GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), width, r, g, b, 0xff, true, 0.0f);
				}
			}
		}
	}
}
