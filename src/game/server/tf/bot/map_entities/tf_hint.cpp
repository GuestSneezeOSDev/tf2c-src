/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_hint.h"
#include "tf_bot.h"
#include "tf_nav_mesh.h"


LINK_ENTITY_TO_CLASS(func_tfbot_hint, CTFBotHint);


BEGIN_DATADESC(CTFBotHint)
	
	DEFINE_KEYFIELD(m_team,       FIELD_INTEGER, "team"         ),
	DEFINE_KEYFIELD(m_hint,       FIELD_INTEGER, "hint"         ),
	DEFINE_KEYFIELD(m_isDisabled, FIELD_BOOLEAN, "StartDisabled"),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable",  InputEnable ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	
END_DATADESC()


void CTFBotHint::InputEnable(inputdata_t& inputdata)
{
	this->m_isDisabled = false;
	this->UpdateNavDecoration();
}

void CTFBotHint::InputDisable(inputdata_t& inputdata)
{
	this->m_isDisabled = true;
	this->UpdateNavDecoration();
}


void CTFBotHint::Spawn()
{
	CBaseEntity::Spawn();
	
	this->CollisionProp()->SetSolid(SOLID_BSP);
	this->CollisionProp()->AddSolidFlags(FSOLID_NOT_SOLID);
	
	this->SetMoveType(MOVETYPE_NONE);
	this->SetModel(STRING(this->GetModelName()));
	this->AddEffects(EF_NODRAW);
	this->SetCollisionGroup(COLLISION_GROUP_NONE);
	
	this->VPhysicsInitShadow(false, false);
	
	this->UpdateNavDecoration();
}

void CTFBotHint::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();
	this->UpdateNavDecoration();
}


bool CTFBotHint::IsFor(CTFBot *bot) const
{
	if (this->m_isDisabled)       return false;
	if (this->m_team == TEAM_ANY) return true;
	
	return (this->m_team == bot->GetTeamNumber());
}

void CTFBotHint::UpdateNavDecoration()
{
	Extent ext;
	ext.Init(this);
	
	CUtlVector<CTFNavArea *> areas;
	TheTFNavMesh->CollectAreasOverlappingExtent(ext, &areas);
	
	uint64 attr = 0;
	switch (this->m_hint) {
	case SNIPER_SPOT: attr = CTFNavArea::SNIPER_SPOT; break;
	case SENTRY_SPOT: attr = CTFNavArea::SENTRY_SPOT; break;
	}
	
	for (auto area : areas) {
		if (this->m_isDisabled) {
			area->RemoveTFAttributes(attr);
		} else {
			area->AddTFAttributes(attr);
		}
	}
}
