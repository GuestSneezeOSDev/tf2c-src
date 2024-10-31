/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_roster.h"


LINK_ENTITY_TO_CLASS(bot_roster, CTFBotRoster);


BEGIN_DATADESC(CTFBotRoster)
	
	DEFINE_KEYFIELD(m_teamName,                               FIELD_STRING,  "team"             ),
	DEFINE_KEYFIELD(m_bAllowClassChanges,                     FIELD_BOOLEAN, "allowClassChanges"),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_SCOUT],        FIELD_BOOLEAN, "allowScout"       ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_SNIPER],       FIELD_BOOLEAN, "allowSniper"      ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_SOLDIER],      FIELD_BOOLEAN, "allowSoldier"     ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_DEMOMAN],      FIELD_BOOLEAN, "allowDemoman"     ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_MEDIC],        FIELD_BOOLEAN, "allowMedic"       ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_HEAVYWEAPONS], FIELD_BOOLEAN, "allowHeavy"       ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_PYRO],         FIELD_BOOLEAN, "allowPyro"        ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_SPY],          FIELD_BOOLEAN, "allowSpy"         ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_ENGINEER],     FIELD_BOOLEAN, "allowEngineer"    ),
	DEFINE_KEYFIELD(m_bAllowedClasses[TF_CLASS_CIVILIAN],     FIELD_BOOLEAN, "allowCivilian"    ),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "SetTeamName",          InputSetTeamName         ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowClassChanges", InputSetAllowClassChanges),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowScout",        InputSetAllowScout       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowSniper",       InputSetAllowSniper      ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowSoldier",      InputSetAllowSoldier     ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowDemoman",      InputSetAllowDemoman     ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowMedic",        InputSetAllowMedic       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowHeavy",        InputSetAllowHeavy       ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowPyro",         InputSetAllowPyro        ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowSpy",          InputSetAllowSpy         ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowEngineer",     InputSetAllowEngineer    ),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAllowCivilian",     InputSetAllowCivilian    ),
	
END_DATADESC()


void CTFBotRoster::InputSetTeamName(inputdata_t& inputdata)
{
	const char *str = inputdata.value.String();
	
	if (FStrEq(str, "red")) {
		this->m_teamName = MAKE_STRING("red");
	} else if (FStrEq(str, "blue")) {
		this->m_teamName = MAKE_STRING("blue");
	} else if (FStrEq(str, "green")) {
		this->m_teamName = MAKE_STRING("green");
	} else if (FStrEq(str, "yellow")) {
		this->m_teamName = MAKE_STRING("yellow");
	}
}


bool CTFBotRoster::IsClassAllowed(int iClassIndex) const
{
	if (iClassIndex > TF_CLASS_UNDEFINED && iClassIndex < TF_CLASS_COUNT_ALL) {
		return this->m_bAllowedClasses[iClassIndex];
	}
	
	return false;
}
