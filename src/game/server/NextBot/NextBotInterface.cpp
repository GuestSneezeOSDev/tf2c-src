/* NextBotInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotInterface.h"
#include "NextBotManager.h"
#include "NextBotUtil.h"
#include "vprof.h"
#include "team.h"
#include "props.h"
#include "fmtstr.h"


ConVar NextBotDebugHistory("nb_debug_history", "1", FCVAR_CHEAT, "If true, each bot keeps a history of debug output in memory");


INextBot::INextBot() :
	m_DebugLines(100)
{
	this->m_iManagerIndex = TheNextBots().Register(this);
}

INextBot::~INextBot()
{
	this->ResetDebugHistory();
	
	TheNextBots().UnRegister(this);
	
	if (this->m_IntentionInterface  != nullptr) delete this->m_IntentionInterface;
	if (this->m_LocomotionInterface != nullptr) delete this->m_LocomotionInterface;
	if (this->m_BodyInterface       != nullptr) delete this->m_BodyInterface;
	if (this->m_VisionInterface     != nullptr) delete this->m_VisionInterface;
}


void INextBot::Reset()
{
	this->m_iLastUpdateTick  = -999;
	this->m_vecLastPosition  = vec3_origin;
	this->m_Dword18          = 0;
	this->m_iDebugTextOffset = 0;
	this->m_itImmobileEpoch.Invalidate();
	this->m_ctImmobileCheck.Invalidate();
	
	for (INextBotComponent *component = this->m_ComponentList; component != nullptr; component = component->GetNextComponent()) {
		component->Reset();
	}
}

void INextBot::Update()
{
	VPROF_BUDGET("INextBot::Update", "NextBot");
	
	this->m_iDebugTextOffset = 0;
	if (this->IsDebugging(DEBUG_ANY)) {
		this->DisplayDebugText(CFmtStr("#%d", this->GetEntity()->entindex()));
	}
	
	this->UpdateImmobileStatus();
	
	for (INextBotComponent *component = this->m_ComponentList; component != nullptr; component = component->GetNextComponent()) {
		if ( component->ComputeUpdateInterval() ) {
			component->Update();
		}
	}
}

void INextBot::Upkeep()
{
	VPROF_BUDGET("INextBot::Upkeep", "NextBot");
	
	for (INextBotComponent *comp = this->m_ComponentList; comp != nullptr; comp = comp->GetNextComponent()) {
		comp->Upkeep();
	}
}


ILocomotion *INextBot::GetLocomotionInterface() const
{
	if (this->m_LocomotionInterface == nullptr) {
		this->m_LocomotionInterface = new ILocomotion(const_cast<INextBot *>(this));
	}
	
	return this->m_LocomotionInterface;
}

IBody *INextBot::GetBodyInterface() const
{
	if (this->m_BodyInterface == nullptr) {
		this->m_BodyInterface = new IBody(const_cast<INextBot *>(this));
	}
	
	return this->m_BodyInterface;
}

IIntention *INextBot::GetIntentionInterface() const
{
	if (this->m_IntentionInterface == nullptr) {
		this->m_IntentionInterface = new IIntention(const_cast<INextBot *>(this));
	}
	
	return this->m_IntentionInterface;
}

IVision *INextBot::GetVisionInterface() const
{
	if (this->m_VisionInterface == nullptr) {
		this->m_VisionInterface = new IVision(const_cast<INextBot *>(this));
	}
	
	return this->m_VisionInterface;
}


bool INextBot::SetPosition(const Vector& pos)
{
	IBody *body = this->GetBodyInterface();
	if (body != nullptr) {
		return body->SetPosition(pos);
	} else {
		this->GetEntity()->SetAbsOrigin(pos);
		return true;
	}
}

const Vector& INextBot::GetPosition() const
{
	return this->GetEntity()->GetAbsOrigin();
}


bool INextBot::IsEnemy(const CBaseEntity *ent) const
{
	if (ent == nullptr) return false;
	return (this->GetEntity()->GetTeamNumber() != ent->GetTeamNumber());
}

bool INextBot::IsFriend(const CBaseEntity *ent) const
{
	if (ent == nullptr) return false;
	return (this->GetEntity()->GetTeamNumber() == ent->GetTeamNumber());
}

bool INextBot::IsSelf(const CBaseEntity *ent) const
{
	if (ent == nullptr) return false;
	return (this->GetEntity()->entindex() == ent->entindex());
}


bool INextBot::IsAbleToClimbOnto(const CBaseEntity *ent) const
{
	if (ent == nullptr || !const_cast<CBaseEntity *>(ent)->IsAIWalkable()) {
		return false;
	}
	
	if (const_cast<CBaseEntity *>(ent)->ClassMatches("prop_door*") ||
		const_cast<CBaseEntity *>(ent)->ClassMatches("func_door*")) {
		return false;
	}
	
	return true;
}

bool INextBot::IsAbleToBreak(const CBaseEntity *ent) const
{
	if (ent == nullptr || ent->m_takedamage != DAMAGE_YES) {
		return false;
	}
	
	if (const_cast<CBaseEntity *>(ent)->ClassMatches("func_breakable") && ent->GetHealth() != 0) {
		return true;
	}
	
	if (const_cast<CBaseEntity *>(ent)->ClassMatches("func_breakable_surf")) {
		return true;
	}
	
	return (dynamic_cast<const CBreakableProp *>(ent) != nullptr);
}


void INextBot::ClearImmobileStatus()
{
	this->m_itImmobileEpoch.Invalidate();
	this->m_vecLastPosition = this->GetEntity()->GetAbsOrigin();
}

void INextBot::NotifyPathDestruction(const PathFollower *follower)
{
	if (this->m_CurrentPath == follower) {
		this->m_CurrentPath = nullptr;
	}
}


bool INextBot::IsRangeLessThan(CBaseEntity *ent, float dist) const
{
	if (ent == nullptr) return true;
	
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr) return true;
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return (ent->CollisionProp()->CalcDistanceFromPoint(point) < dist);
}

bool INextBot::IsRangeLessThan(const Vector& vec, float dist) const
{
	return (this->GetPosition() - vec).IsLengthLessThan(dist);
}

bool INextBot::IsRangeGreaterThan(CBaseEntity *ent, float dist) const
{
	if (ent == nullptr) return true;
	
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr) return true;
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return (ent->CollisionProp()->CalcDistanceFromPoint(point) > dist);
}

bool INextBot::IsRangeGreaterThan(const Vector& vec, float dist) const
{
	return (this->GetPosition() - vec).IsLengthGreaterThan(dist);
}


float INextBot::GetRangeTo(CBaseEntity *ent) const
{
	if (ent == nullptr) return 0.0f;
	
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr) return 0.0f;
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return ent->CollisionProp()->CalcDistanceFromPoint(point);
}

float INextBot::GetRangeTo(const Vector& vec) const
{
	return (this->GetPosition() - vec).Length();
}

float INextBot::GetRangeSquaredTo(CBaseEntity *ent) const
{
	if (ent == nullptr) return 0.0f;
	
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr) return 0.0f;
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return Square(ent->CollisionProp()->CalcDistanceFromPoint(point));
}

float INextBot::GetRangeSquaredTo(const Vector& vec) const
{
	return (this->GetPosition() - vec).LengthSqr();
}


bool INextBot::IsDebugging(unsigned int type) const
{
	if (!TheNextBots().IsDebugging(type)) return false;
	return TheNextBots().IsDebugFilterMatch(this);
}

char *INextBot::GetDebugIdentifier() const
{
	static char name[0x100];
	V_sprintf_safe(name, "%s(#%d)", this->GetEntity()->GetClassname(), this->GetEntity()->entindex());
	return name;
}

bool INextBot::IsDebugFilterMatch(const char *filter) const
{
	if (V_strnicmp(filter, this->GetDebugIdentifier(), strlen(filter)) == 0) {
		return true;
	}
	
	CTeam *team = this->GetEntity()->GetTeam();
	if (team != nullptr) {
		return (V_strnicmp(filter, team->GetName(), strlen(filter)) == 0);
	}
	
	return false;
}

void INextBot::DisplayDebugText(const char *text) const
{
	this->GetEntity()->EntityText(this->m_iDebugTextOffset++, text, 0.1f, NB_RGBA_WHITE);
}


void INextBot::RemoveEntity()
{
	UTIL_Remove(this->GetEntity());
}


void INextBot::DebugConColorMsg(NextBotDebugType type, const Color& color, const char *fmt, ...)
{
	static char buf[0x1000];
	
	va_list va;
	va_start(va, fmt);
	
	bool printed_to_console = false;
	if (developer.GetBool() && this->IsDebugging(type)) {
		V_vsprintf_safe(buf, fmt, va);
		ConColorMsg(color, "%s", buf);
		printed_to_console = true;
	}
	
	if (NextBotDebugHistory.GetBool()) {
		if (type != DEBUG_EVENTS) {
			if (!printed_to_console) {
				V_vsprintf_safe(buf, fmt, va);
			}
			
			if (this->m_DebugLines.Count() > 0 && this->m_DebugLines.Tail()->type == type && strchr(this->m_DebugLines.Tail()->buf, '\n') == nullptr) {
				V_strcat_safe(this->m_DebugLines.Tail()->buf, buf);
			} else {
				if (this->m_DebugLines.Count() == MAX_DEBUG_LINES) {
					this->m_DebugLines.RemoveMultipleFromHead(1);
				}
				
				auto line = new NextBotDebugLineType();
				line->type = type;
				V_strcpy_safe(line->buf, buf);
				
				this->m_DebugLines.AddToTail(line);
			}
		}
	} else {
		if (!this->m_DebugLines.IsEmpty()) {
			this->ResetDebugHistory();
		}
	}
	
	va_end(va);
}


void INextBot::RegisterComponent(INextBotComponent *component)
{
	component->SetNextComponent(this->m_ComponentList);
	this->m_ComponentList = component;
}


HSCRIPT INextBot::ScriptGetLocomotionInterface()
{
	ILocomotion *pLocomotion = GetLocomotionInterface();
	if ( pLocomotion )
		return pLocomotion->GetScriptInstance();

	return NULL;
}

HSCRIPT INextBot::ScriptGetBodyInterface()
{
	IBody *pBody = GetBodyInterface();
	if ( pBody )
		return pBody->GetScriptInstance();

	return NULL;
}

HSCRIPT INextBot::ScriptGetIntentionInterface()
{
	IIntention *pIntention = GetIntentionInterface();
	if ( pIntention )
		return pIntention->GetScriptInstance();

	return NULL;
}

HSCRIPT INextBot::ScriptGetVisionInterface()
{
	IVision *pVision = GetVisionInterface();
	if ( pVision )
		return pVision->GetScriptInstance();

	return NULL;
}

bool INextBot::ScriptIsEnemy( HSCRIPT hEntity )
{
	return IsEnemy( ToEnt( hEntity ) );
}

bool INextBot::ScriptIsFriend( HSCRIPT hEntity )
{
	return IsFriend( ToEnt( hEntity ) );
}

bool INextBot::BeginUpdate()
{
	if (!TheNextBots().ShouldUpdate(this)) return false;
	
	TheNextBots().NotifyBeginUpdate(this);
	return true;
}

void INextBot::EndUpdate()
{
	TheNextBots().NotifyEndUpdate(this);
}


void INextBot::UpdateImmobileStatus()
{
	if (!this->m_ctImmobileCheck.IsElapsed()) {
		return;
	}
	
	this->m_ctImmobileCheck.Start(1.0f);
	
	if ((this->GetEntity()->GetAbsOrigin() - this->m_vecLastPosition).IsLengthGreaterThan(this->GetImmobileSpeedThreshold())) {
		this->m_vecLastPosition = this->GetEntity()->GetAbsOrigin();
		this->m_itImmobileEpoch.Invalidate();
	} else if (!this->m_itImmobileEpoch.HasStarted()) {
		this->m_itImmobileEpoch.Start();
	}
}


void INextBot::GetDebugHistory(unsigned int mask, CUtlVector<const NextBotDebugLineType *> *dst) const
{
	if (dst != nullptr) {
		dst->RemoveAll();
		
		for (auto line : this->m_DebugLines) {
			if ((line->type & mask) != 0) {
				dst->AddToTail(line);
			}
		}
	}
}

void INextBot::ResetDebugHistory()
{
	for (auto line : this->m_DebugLines) {
		delete line;
	}
	this->m_DebugLines.RemoveAll();
}
