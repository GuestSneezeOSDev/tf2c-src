/* func_nav_prerequisite
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NAVMESHENTITIES_FUNC_NAV_PREREQUISITE_H
#define NEXTBOT_NAVMESHENTITIES_FUNC_NAV_PREREQUISITE_H
#ifdef _WIN32
#pragma once
#endif


#include "triggers.h"


class CFuncNavPrerequisite : public CBaseTrigger, public TAutoList<CFuncNavPrerequisite>
{
public:
	enum TaskType
	{
		TASK_NONE    = 0,
		TASK_DESTROY = 1,
		TASK_MOVE_TO = 2,
		TASK_WAIT    = 3,
	};
	
	CFuncNavPrerequisite() {}
	virtual ~CFuncNavPrerequisite() {}
	
	DECLARE_CLASS(CFuncNavPrerequisite, CBaseTrigger);
	DECLARE_DATADESC();
	
	virtual void Spawn() override;
	
	bool IsTask(TaskType task) const { return (this->m_task == task); }
	float GetTaskValue() const       { return this->m_taskValue;      }
	bool IsDisabled() const          { return this->m_isDisabled;     }
	CBaseEntity *GetTaskEntity();
	
	void InputEnable(inputdata_t& inputdata)  { this->m_isDisabled = true;  }
	void InputDisable(inputdata_t& inputdata) { this->m_isDisabled = false; }
	
private:
	TaskType             m_task = TASK_NONE; // +0x488
	string_t             m_taskEntityName;   // +0x48c
	float                m_taskValue;        // +0x490 TODO: are we SURE this is a float?
	bool                 m_isDisabled;       // +0x494 TODO: needs an initializer...?
	CHandle<CBaseEntity> m_hTaskEntity;      // +0x498
};
// TODO: remove offsets


#endif
