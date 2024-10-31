/* func_nav_prerequisite
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "func_nav_prerequisite.h"


#pragma message("TODO: verify correctness of func_nav_prerequisite in the FGD!")
LINK_ENTITY_TO_CLASS(func_nav_prerequisite, CFuncNavPrerequisite);


BEGIN_DATADESC(CFuncNavPrerequisite)
	
	DEFINE_KEYFIELD(m_task,           FIELD_INTEGER, "Task"         ),
	DEFINE_KEYFIELD(m_taskEntityName, FIELD_STRING,  "Entity"       ),
	DEFINE_KEYFIELD(m_taskValue,      FIELD_FLOAT,   "Value"        ),
	DEFINE_KEYFIELD(m_isDisabled,     FIELD_BOOLEAN, "StartDisabled"),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable",  InputEnable ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	
END_DATADESC()


void CFuncNavPrerequisite::Spawn()
{
	this->AddSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS);
	
	CBaseTrigger::Spawn();
	this->InitTrigger();
}


CBaseEntity *CFuncNavPrerequisite::GetTaskEntity()
{
	CBaseEntity *ent = this->m_hTaskEntity;
	if (ent == nullptr) {
		ent = gEntList.FindEntityByName(nullptr, STRING(this->m_taskEntityName));
		this->m_hTaskEntity = ent;
	}
	return ent;
}
