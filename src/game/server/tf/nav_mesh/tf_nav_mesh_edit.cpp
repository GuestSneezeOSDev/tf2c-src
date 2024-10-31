/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_nav_mesh.h"


struct TFNavAttributeLookup
{
	const char *name;
	CTFNavArea::AttributeType attribute;
};
#define TF_NAV_TABLE_ENTRY(attr) { #attr, CTFNavArea::attr }

/* only static attributes shall be in this list */
static const TFNavAttributeLookup s_TFAttributeTable[] = {
	TF_NAV_TABLE_ENTRY(UNBLOCKABLE),
	TF_NAV_TABLE_ENTRY(NO_SPAWNING),
	TF_NAV_TABLE_ENTRY(SNIPER_SPOT),
	TF_NAV_TABLE_ENTRY(SENTRY_SPOT),
	TF_NAV_TABLE_ENTRY(RESCUE_CLOSET),
	TF_NAV_TABLE_ENTRY(RED_SETUP_GATE),
	TF_NAV_TABLE_ENTRY(BLUE_SETUP_GATE),
	TF_NAV_TABLE_ENTRY(GREEN_SETUP_GATE),
	TF_NAV_TABLE_ENTRY(YELLOW_SETUP_GATE),
	TF_NAV_TABLE_ENTRY(RED_ONE_WAY_DOOR),
	TF_NAV_TABLE_ENTRY(BLUE_ONE_WAY_DOOR),
	TF_NAV_TABLE_ENTRY(GREEN_ONE_WAY_DOOR),
	TF_NAV_TABLE_ENTRY(YELLOW_ONE_WAY_DOOR),
	TF_NAV_TABLE_ENTRY(DOOR_NEVER_BLOCKS),
	TF_NAV_TABLE_ENTRY(DOOR_ALWAYS_BLOCKS),
	TF_NAV_TABLE_ENTRY(BLOCKED_AFTER_POINT_CAPTURE),
	TF_NAV_TABLE_ENTRY(BLOCKED_UNTIL_POINT_CAPTURE),
	TF_NAV_TABLE_ENTRY(WITH_SECOND_POINT),
	TF_NAV_TABLE_ENTRY(WITH_THIRD_POINT),
	TF_NAV_TABLE_ENTRY(WITH_FOURTH_POINT),
	TF_NAV_TABLE_ENTRY(WITH_FIFTH_POINT),
	
	{ nullptr, (CTFNavArea::AttributeType)0 },
};


static const char *TFAttributeToName(CTFNavArea::AttributeType attr)
{
	for (int i = 0; s_TFAttributeTable[i].name != nullptr; ++i) {
		if (s_TFAttributeTable[i].attribute == attr) {
			return s_TFAttributeTable[i].name;
		}
	}
	
	return "";
}

static CTFNavArea::AttributeType NameToTFAttribute(const char *name)
{
	for (int i = 0; s_TFAttributeTable[i].name != nullptr; ++i) {
		if (V_stricmp(s_TFAttributeTable[i].name, name) == 0) {
			return s_TFAttributeTable[i].attribute;
		}
	}
	
	return (CTFNavArea::AttributeType)0;
}


// TODO: rewrite this to be less horrendous
static int AttributeAutocomplete(const char *input, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	if (V_strlen(input) >= COMMAND_COMPLETION_ITEM_LENGTH) return 0;
	
	char command[COMMAND_COMPLETION_ITEM_LENGTH + 1];
	V_strcpy_safe(command, input);
	
	char *arg = V_strrchr(command, ' ');
	if (arg == nullptr) return 0;
	
	*arg = '\0';
	++arg;
	int arg_len = V_strlen(arg);
	
	int count = 0;
	
	for (int i = 0; s_TFAttributeTable[i].name != nullptr && count < COMMAND_COMPLETION_MAXITEMS; ++i) {
		if (V_strnicmp(s_TFAttributeTable[i].name, arg, arg_len) == 0) {
			V_snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", command, s_TFAttributeTable[i].name);
		}
	}
	
	return count;
}


class TFNavStaticAttributeClearer
{
public:
	bool operator()(CTFNavArea *area)
	{
		area->ClearStaticTFAttributes();
		return true;
	}
};

class TFNavAttributeAdder
{
public:
	TFNavAttributeAdder(CTFNavArea::AttributeType attr) : m_nTFAttributes(attr) {}
	
	bool operator()(CTFNavArea *area)
	{
		area->AddTFAttributes(this->m_nTFAttributes);
		return true;
	}
	
private:
	CTFNavArea::AttributeType m_nTFAttributes;
};

class TFNavAttributeRemover
{
public:
	TFNavAttributeRemover(CTFNavArea::AttributeType attr) : m_nTFAttributes(attr) {}
	
	bool operator()(CTFNavArea *area)
	{
		area->RemoveTFAttributes(this->m_nTFAttributes);
		return true;
	}
	
private:
	CTFNavArea::AttributeType m_nTFAttributes;
};

class TFNavAttributeToggler
{
public:
	TFNavAttributeToggler(CTFNavArea::AttributeType attr) : m_nTFAttributes(attr) {}
	
	bool operator()(CTFNavArea *area)
	{
		if (TheTFNavMesh->IsSelectedSetEmpty() && area->HasAnyTFAttributes(this->m_nTFAttributes)) {
			area->RemoveTFAttributes(this->m_nTFAttributes);
		} else {
			area->AddTFAttributes(this->m_nTFAttributes);
		}
		return true;
	}
	
private:
	CTFNavArea::AttributeType m_nTFAttributes;
};


static void TF_EditClearAllAttributes()
{
	TFNavStaticAttributeClearer functor;
	TheTFNavMesh->ForAllSelectedTFAreas(functor);
	TheTFNavMesh->ClearSelectedSet();
}

static void TF_EditClearAttribute(const CCommand& args)
{
	if (args.ArgC() < 2) {
		Msg("Usage: %s <attribute1> [attribute2...]\n", args[0]);
		return;
	}
	
	for (int i = 1; i < args.ArgC(); ++i) {
		CTFNavArea::AttributeType tf_attr = NameToTFAttribute(args[i]);
		NavAttributeType nav_attr = NameToNavAttribute(args[i]);
		
		if (tf_attr != (CTFNavArea::AttributeType)0) {
			TFNavAttributeRemover functor(tf_attr);
			TheTFNavMesh->ForAllSelectedTFAreas(functor);
		} else if (nav_attr != (NavAttributeType)0) {
			NavAttributeClearer functor(nav_attr);
			TheTFNavMesh->ForAllSelectedAreas(functor);
		} else {
			Msg("Unknown attribute '%s'\n", args[i]);
		}
	}
	
	TheTFNavMesh->ClearSelectedSet();
}

static void TF_EditMarkAttribute(const CCommand& args)
{
	if (args.ArgC() < 2) {
		Msg("Usage: %s <attribute1> [attribute2...]\n", args[0]);
		return;
	}
	
	for (int i = 1; i < args.ArgC(); ++i) {
		CTFNavArea::AttributeType tf_attr = NameToTFAttribute(args[i]);
		NavAttributeType nav_attr = NameToNavAttribute(args[i]);
		
		if (tf_attr != (CTFNavArea::AttributeType)0) {
			TFNavAttributeToggler functor(tf_attr);
			TheTFNavMesh->ForAllSelectedTFAreas(functor);
		} else if (nav_attr != (NavAttributeType)0) {
			NavAttributeToggler functor(nav_attr);
			TheTFNavMesh->ForAllSelectedAreas(functor);
		} else {
			Msg("Unknown attribute '%s'\n", args[i]);
		}
	}
	
	TheTFNavMesh->ClearSelectedSet();
}

static void TF_EditSelectWithAttribute(const CCommand& args)
{
	TheTFNavMesh->ClearSelectedSet();
	
	if (args.ArgC() != 2) {
		Msg("Usage: %s <attribute>\n", args[0]);
		return;
	}
	
	int n_selected = 0;
	
	CTFNavArea::AttributeType tf_attr = NameToTFAttribute(args[1]);
	if (tf_attr != (CTFNavArea::AttributeType)0) {
		for (auto area : TheTFNavAreas) {
			if (area->HasAnyTFAttributes(tf_attr)) {
				++n_selected;
				TheTFNavMesh->AddToSelectedSet(area);
			}
		}
	} else {
		NavAttributeType nav_attr = NameToNavAttribute(args[1]);
		if (nav_attr != (NavAttributeType)0) {
			for (auto area : TheNavAreas) {
				if (area->HasAttributes(nav_attr)) {
					++n_selected;
					TheTFNavMesh->AddToSelectedSet(area);
				}
			}
		} else {
			Msg("Unknown attribute '%s'\n", args[1]);
		}
	}
	
	Msg("%d areas added to selection\n", n_selected);
}


static ConCommand ClearAllAttributes ("tf_wipe_attributes",       &TF_EditClearAllAttributes,  "Clear all TF-specific attributes of selected area.",         FCVAR_CHEAT);
static ConCommand ClearAttributeTF   ("tf_clear_attribute",       &TF_EditClearAttribute,      "Remove given attribute from all areas in the selected set.", FCVAR_CHEAT, &AttributeAutocomplete);
static ConCommand MarkAttribute      ("tf_mark",                  &TF_EditMarkAttribute,       "Set attribute of selected area.",                            FCVAR_CHEAT, &AttributeAutocomplete);
static ConCommand SelectWithAttribute("tf_select_with_attribute", &TF_EditSelectWithAttribute, "Selects areas with the given attribute.",                    FCVAR_CHEAT, &AttributeAutocomplete);
