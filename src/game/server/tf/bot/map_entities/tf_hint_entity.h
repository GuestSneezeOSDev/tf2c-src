/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_HINT_ENTITY_H
#define TF_HINT_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


class CBaseTFBotHintEntity : public CPointEntity
{
public:
	enum HintType
	{
		TELEPORTER_EXIT = 0,
		SENTRY_GUN      = 1,
		ENGINEER_NEST   = 2,
	};
	
	DECLARE_CLASS(CBaseTFBotHintEntity, CPointEntity);
	DECLARE_DATADESC();
	
	virtual HintType GetHintType() const = 0;
	
	void InputEnable(inputdata_t& inputdata)  { this->m_isDisabled = false; }
	void InputDisable(inputdata_t& inputdata) { this->m_isDisabled = true;  }
	
	bool OwnerObjectFinishBuilding() const;
	bool OwnerObjectHasNoOwner() const;
	
	bool IsDisabled() const { return this->m_isDisabled; }
	
protected:
	CBaseTFBotHintEntity() {}
	virtual ~CBaseTFBotHintEntity() {}
	
	bool m_isDisabled = false; // +0x364
	// 368 CHandle<T> (unused)
};
// TODO: remove offsets


#endif
