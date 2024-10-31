/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_ROSTER_H
#define TF_BOT_ROSTER_H
#ifdef _WIN32
#pragma once
#endif


class CTFBotRoster : public CPointEntity
{
public:
	CTFBotRoster() {}
	virtual ~CTFBotRoster() {}
	
	DECLARE_CLASS(CTFBotRoster, CPointEntity);
	DECLARE_DATADESC();
	
	void InputSetTeamName(inputdata_t& inputdata);
	void InputSetAllowClassChanges(inputdata_t& inputdata) { this->m_bAllowClassChanges                     = inputdata.value.Bool(); }
	void InputSetAllowScout(inputdata_t& inputdata)        { this->m_bAllowedClasses[TF_CLASS_SCOUT]        = inputdata.value.Bool(); }
	void InputSetAllowSniper(inputdata_t& inputdata)       { this->m_bAllowedClasses[TF_CLASS_SNIPER]       = inputdata.value.Bool(); }
	void InputSetAllowSoldier(inputdata_t& inputdata)      { this->m_bAllowedClasses[TF_CLASS_SOLDIER]      = inputdata.value.Bool(); }
	void InputSetAllowDemoman(inputdata_t& inputdata)      { this->m_bAllowedClasses[TF_CLASS_DEMOMAN]      = inputdata.value.Bool(); }
	void InputSetAllowMedic(inputdata_t& inputdata)        { this->m_bAllowedClasses[TF_CLASS_MEDIC]        = inputdata.value.Bool(); }
	void InputSetAllowHeavy(inputdata_t& inputdata)        { this->m_bAllowedClasses[TF_CLASS_HEAVYWEAPONS] = inputdata.value.Bool(); }
	void InputSetAllowPyro(inputdata_t& inputdata)         { this->m_bAllowedClasses[TF_CLASS_PYRO]         = inputdata.value.Bool(); }
	void InputSetAllowSpy(inputdata_t& inputdata)          { this->m_bAllowedClasses[TF_CLASS_SPY]          = inputdata.value.Bool(); }
	void InputSetAllowEngineer(inputdata_t& inputdata)     { this->m_bAllowedClasses[TF_CLASS_ENGINEER]     = inputdata.value.Bool(); }
	void InputSetAllowCivilian(inputdata_t& inputdata)     { this->m_bAllowedClasses[TF_CLASS_CIVILIAN]     = inputdata.value.Bool(); }
	
	const char *GetTeamName() const { return STRING(this->m_teamName); }
	bool IsClassChangeAllowed() const { return this->m_bAllowClassChanges; }
	bool IsClassAllowed(int iClassIndex) const;
	
private:
	string_t m_teamName;
	bool m_bAllowClassChanges;
	bool m_bAllowedClasses[TF_CLASS_COUNT_ALL] = {};
};


#endif
