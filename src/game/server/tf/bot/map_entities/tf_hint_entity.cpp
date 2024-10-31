/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_hint_entity.h"
#include "tf_player.h"
#include "tf_obj.h"


// TODO: put this somewhere
static CBaseObject *ToBaseObject(CBaseEntity *ent)
{
	if (ent != nullptr && ent->IsBaseObject()) {
		return assert_cast<CBaseObject *>(ent);
	}
	
	return nullptr;
}


BEGIN_DATADESC(CBaseTFBotHintEntity)
	
	DEFINE_KEYFIELD(m_isDisabled, FIELD_BOOLEAN, "StartDisabled"),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable",  InputEnable ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	
END_DATADESC()


bool CBaseTFBotHintEntity::OwnerObjectFinishBuilding() const
{
	CBaseObject *obj = ToBaseObject(this->GetOwnerEntity());
	if (obj == nullptr) return false;
	
	return !obj->IsBuilding();
}

bool CBaseTFBotHintEntity::OwnerObjectHasNoOwner() const
{
	CBaseObject *obj = ToBaseObject(this->GetOwnerEntity());
	if (obj == nullptr) return false;
	
	if (obj->GetBuilder() == nullptr) {
		return true;
	} else {
		if (!obj->GetBuilder()->IsPlayerClass(TF_CLASS_ENGINEER, true)) {
			Warning("Object has an owner that's not an engineer.\n");
		}
		
		return false;
	}
}
