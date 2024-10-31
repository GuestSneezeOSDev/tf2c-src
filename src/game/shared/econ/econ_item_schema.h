#ifndef ECON_ITEM_SCHEMA_H
#define ECON_ITEM_SCHEMA_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

enum
{
	ATTRIB_FORMAT_INVALID = -1,
	ATTRIB_FORMAT_PERCENTAGE = 0,
	ATTRIB_FORMAT_INVERTED_PERCENTAGE,
	ATTRIB_FORMAT_ADDITIVE,
	ATTRIB_FORMAT_ADDITIVE_PERCENTAGE,
	ATTRIB_FORMAT_OR,
};
	
enum
{
	ATTRIB_EFFECT_INVALID = -1,
	ATTRIB_EFFECT_UNUSUAL = 0,
	ATTRIB_EFFECT_STRANGE,
	ATTRIB_EFFECT_NEUTRAL,
	ATTRIB_EFFECT_POSITIVE,
	ATTRIB_EFFECT_NEGATIVE,
};

enum EEconItemQuality
{
	QUALITY_NORMAL,
	QUALITY_GENUINE,
	QUALITY_RARITY2,
	QUALITY_VINTAGE,
	QUALITY_RARITY3,
	QUALITY_UNUSUAL,
	QUALITY_UNIQUE,
	QUALITY_COMMUNITY,
	QUALITY_VALVE,
	QUALITY_SELFMADE,
	QUALITY_CUSTOMIZED,
	QUALITY_STRANGE,
	QUALITY_COMPLETED,
	QUALITY_HUNTED,
	QUALITY_COLLECTOR,
	QUALITY_DECORATED,
	QUALITY_RARITY_DEFAULT,
	QUALITY_RARITY_COMMON,
	QUALITY_RARITY_UNCOMMON,
	QUALITY_RARITY_RARE,
	QUALITY_RARITY_MYTHICAL,
	QUALITY_RARITY_LEGENDARY,
	QUALITY_RARITY_ANCIENT,
	QUALITY_COUNT
};

enum
{
	kAttachedModelDisplayFlag_WorldModel = 0x01,
	kAttachedModelDisplayFlag_ViewModel = 0x02,

	kAttachedModelDisplayFlag_MaskAll = kAttachedModelDisplayFlag_WorldModel | kAttachedModelDisplayFlag_ViewModel,
};

struct attachedmodel_t
{
	const char *m_pszModelName;
	int m_iModelDisplayFlags;
};

enum EHoliday
{
	kHoliday_None,
	kHoliday_TFBirthday,
	kHoliday_Halloween,
	kHoliday_Christmas,
	kHoliday_CommunityUpdate,
	kHoliday_EOTL,
	kHoliday_Valentines,
	kHoliday_MeetThePyro,
	kHoliday_FullMoon,
	kHoliday_HalloweenOrFullMoon,
	kHoliday_HalloweenOrFullMoonOrValentines,
	kHoliday_AprilFools,
	kHoliday_Soldier,

	kHolidayCount,
};

extern const char *g_szQualityColorStrings[];
extern const char *g_szQualityLocalizationStrings[];
extern const char *g_szItemBorders[QUALITY_COUNT][5];

const char *EconQuality_GetColorString( EEconItemQuality quality );
const char *EconQuality_GetLocalizationString( EEconItemQuality quality );

#define CALL_ATTRIB_HOOK_INT( value, name )						\
		value = CAttributeManager::AttribHookValue<int>( value, #name, this )

#define CALL_ATTRIB_HOOK_FLOAT( value, name )					\
		value = CAttributeManager::AttribHookValue<float>( value, #name, this )

#define CALL_ATTRIB_HOOK_STRING( value, name )					\
		value = CAttributeManager::AttribHookValue<string_t>( value, #name, this )

#define CALL_ATTRIB_HOOK_ENUM( value, name )					\
		value = (decltype( value ))CAttributeManager::AttribHookValue<int>( (int)value, #name, this )


#define CALL_ATTRIB_HOOK_INT_ON_OTHER( ent, value, name )		\
		value = CAttributeManager::AttribHookValue<int>( value, #name, ent )

#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( ent, value, name )		\
		value = CAttributeManager::AttribHookValue<float>( value, #name, ent )

#define CALL_ATTRIB_HOOK_STRING_ON_OTHER( ent, value, name )	\
		value = CAttributeManager::AttribHookValue<string_t>( value, #name, ent )

#define CALL_ATTRIB_HOOK_ENUM_ON_OTHER( ent, value, name )		\
		value = (decltype( value ))CAttributeManager::AttribHookValue<int>( (int)value, #name, ent )

#define INIT_STRING_PTR_ARRAY( name )							\
		for ( int i = 0, c = ARRAYSIZE( name ); i < c; i++ )	\
		{														\
			name[i] = "";										\
		}														\

struct EconQuality
{
	EconQuality()
	{
		value = 0;
	}

	int value;
};

struct EconColor
{
	EconColor()
	{
		color_name = "";
	}

	const char *color_name;
};

struct CEconAttributeDefinition
{
	CEconAttributeDefinition()
	{
		index = (unsigned short)-1;
		name = "unnamed";
		attribute_class = "";
		description_string = "";
		string_attribute = false;
		description_format = -1;
		hidden = false;
		custom_color = Color(0, 0, 0, 0);
		effect_type = -1;
		stored_as_integer = false;
		hidden_separator = false;
	}

	unsigned short index;
	const char *name;
	const char *attribute_class;
	const char *description_string;
	bool string_attribute;
	int description_format;
	int effect_type;
	Color custom_color;
	bool hidden;
	bool stored_as_integer;
	bool hidden_separator;
};

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_EconItemAttribute );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_EconItemAttribute );
#endif

class CEconItemAttribute
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CEconItemAttribute );

	CEconItemAttribute()
	{
		Init( -1, 0.0f );
	}
	CEconItemAttribute( int iIndex, float flValue )
	{
		Init( iIndex, flValue );
	}
	CEconItemAttribute( int iIndex, float flValue, const char *pszAttributeClass )
	{
		Init( iIndex, flValue, pszAttributeClass );
	}
	CEconItemAttribute( int iIndex, const char *pszValue, const char *pszAttributeClass )
	{
		Init( iIndex, pszValue, pszAttributeClass );
	}

	void Init( int iIndex, float flValue, const char *pszAttributeClass = NULL );
	void Init( int iIndex, const char *pszValue, const char *pszAttributeClass = NULL );
	CEconAttributeDefinition *GetStaticData( void ) const;

	CNetworkVar( int, m_iAttributeDefinitionIndex );
	CNetworkVar( float, value ); // m_iRawValue32
	CNetworkVar( int, m_nRefundableCurrency );
	CNetworkString( value_string, 128 );
	CNetworkString( attribute_class, 128 );
	string_t m_strAttributeClass;

};

struct EconItemStyle
{
	EconItemStyle()
	{
		name = "";
		model_player = "";
		image_inventory = "";
		skin_red = 0;
		skin_blu = 0;
		selectable = false;
	}

	int skin_red;
	int skin_blu;
	bool selectable;
	const char *name;
	const char *model_player;
	const char *image_inventory;
	//CUtlDict<const char*, unsigned short> model_player_per_class;
};

class EconItemVisuals
{
public:
	EconItemVisuals()
	{
		SetDefLessFunc( animation_replacement );
		INIT_STRING_PTR_ARRAY( sound_weapons );
		INIT_STRING_PTR_ARRAY( custom_sound );
		tracer_effect = "";
		explosion_effect = "";
		explosion_effect_crit = "";
		trail_effect = "";
		trail_effect_crit = "";
		beam_effect = "";
		beam_effect_crit = "";
	}

	CUtlDict<bool, unsigned short> player_bodygroups;
	CUtlMap<int, int> animation_replacement;
	const char *sound_weapons[NUM_SHOOT_SOUND_TYPES];
	const char *custom_sound[3];
	const char *tracer_effect;
	const char *explosion_effect;
	const char *explosion_effect_crit;
	const char *trail_effect;
	const char *trail_effect_crit;
	const char *beam_effect;
	const char *beam_effect_crit;

	CUtlVector<attachedmodel_t>	m_AttachedModels;

};

#ifdef ITEM_TAUNTING
class CTFTauntInfo
{
public:
	CTFTauntInfo()
	{
		// Team Fortress 2 Classic
		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		{
			taunt_force_weapon_slot_per_class[i] = TF_LOADOUT_SLOT_INVALID;
		}

		INIT_STRING_PTR_ARRAY( custom_taunt_prop_per_class );
		INIT_STRING_PTR_ARRAY( custom_taunt_prop_scene_per_class );
		INIT_STRING_PTR_ARRAY( custom_taunt_prop_outro_scene_per_class );
		is_hold_taunt = false;
		is_partner_taunt = false;
		taunt_attack = 0;
		taunt_attack_time = 0.0f;
		taunt_separation_forward_distance = 0.0f;
		stop_taunt_if_moved = false;
		taunt_move_speed = 0.0f;
		taunt_turn_speed = 0.0f;
		taunt_force_move_forward = false;
		taunt_success_sound = "";
		taunt_success_sound_loop = "";
		taunt_force_weapon_slot = TF_LOADOUT_SLOT_INVALID;
		taunt_mimic = false;
	}

	// Team Fortress 2 Classic
	ETFLoadoutSlot taunt_force_weapon_slot_per_class[TF_CLASS_COUNT_ALL];

	CUtlVector<const char *> custom_taunt_scene_per_class[TF_CLASS_COUNT_ALL];
	CUtlVector<const char *> custom_taunt_outro_scene_per_class[TF_CLASS_COUNT_ALL];
	CUtlVector<const char *> custom_partner_taunt_initiator_per_class[TF_CLASS_COUNT_ALL];
	CUtlVector<const char *> custom_partner_taunt_receiver_per_class[TF_CLASS_COUNT_ALL];
	const char *custom_taunt_prop_per_class[TF_CLASS_COUNT_ALL];
	const char *custom_taunt_prop_scene_per_class[TF_CLASS_COUNT_ALL];
	const char *custom_taunt_prop_outro_scene_per_class[TF_CLASS_COUNT_ALL];

	bool is_hold_taunt;
	bool is_partner_taunt;
	int taunt_attack;
	float taunt_attack_time;
	float taunt_separation_forward_distance;
	bool stop_taunt_if_moved;
	const char *taunt_success_sound;
	const char *taunt_success_sound_loop;
	float taunt_move_speed;
	float taunt_turn_speed;
	bool taunt_force_move_forward;
	ETFLoadoutSlot taunt_force_weapon_slot;
	bool taunt_mimic;

};
#endif

class CEconItemDefinition
{
public:
	CEconItemDefinition()
	{
		index = (unsigned short)-1;
		name = "";
		used_by_classes = 0;

		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		{
			item_slot_per_class[i] = TF_LOADOUT_SLOT_INVALID;
		}

		show_in_armory = false;
		testing = false;
		item_class = "";
		item_type_name = "";
		item_name = "";
		item_description = "";
		item_slot = TF_LOADOUT_SLOT_INVALID;
		anim_slot = TF_WPN_TYPE_INVALID;
		item_quality = QUALITY_NORMAL;
		item_name_color = Color(0, 0, 0, 0);
		bucket = -1;
		bucket_position = -1;
		baseitem = false;
		propername = false;
		item_logname = "";
		item_iconname = "";
		min_ilevel = 0;
		max_ilevel = 0;
		image_inventory = "";
		image_inventory_size_w = 128;
		image_inventory_size_h = 82;
		model_player = "";
		model_world = "";
		INIT_STRING_PTR_ARRAY( model_player_per_class );
		INIT_STRING_PTR_ARRAY( model_world_per_class );
		attach_to_hands = 0;
		flip_viewmodel = false;
		act_as_wearable = false;
		extra_wearable = "";
		extra_wearable_hide_on_active = false;
		hide_bodygroups_deployed_only = false;
		mouse_pressed_sound = "";
		drop_sound = "";
	}

	EconItemVisuals *GetVisuals( int iTeamNum = TEAM_UNASSIGNED );
	int GetNumAttachedModels( int iTeam );
	attachedmodel_t *GetAttachedModelData( int iTeam, int iIdx );

	ETFLoadoutSlot GetLoadoutSlot( int iClass = TF_CLASS_UNDEFINED );
	bool IsAWearable( void );
	const wchar_t *GenerateLocalizedFullItemName( void );
	CEconItemAttribute *IterateAttributes( string_t strClass );

	unsigned short index;
	const char *name;
	CUtlDict<bool, unsigned short> capabilities;
	CUtlDict<bool, unsigned short> tags;
	int used_by_classes;
	ETFLoadoutSlot item_slot_per_class[TF_CLASS_COUNT_ALL];
	bool show_in_armory;
	bool testing;
	const char *item_class;
	const char *item_type_name;
	const char *item_name;
	const char *item_description;
	ETFLoadoutSlot   item_slot;
	ETFWeaponType    anim_slot;
	EEconItemQuality item_quality;
	Color item_name_color;
	int bucket;
	int bucket_position;
	bool baseitem;
	bool propername;
	const char *item_logname;
	const char *item_iconname;
	int	 min_ilevel;
	int	 max_ilevel;
	const char *image_inventory;
	int	 image_inventory_size_w;
	int	 image_inventory_size_h;
	const char *model_player;
	const char *model_world;
	const char *model_player_per_class[TF_CLASS_COUNT_ALL];
	const char *model_world_per_class[TF_CLASS_COUNT_ALL];
	int attach_to_hands;
	bool flip_viewmodel;
	bool act_as_wearable;
	const char *extra_wearable;
	bool extra_wearable_hide_on_active;
	bool hide_bodygroups_deployed_only;
	CUtlVector<CEconItemAttribute> attributes;
	EconItemVisuals visual[TF_TEAM_COUNT];
#ifdef ITEM_TAUNTING
	CTFTauntInfo taunt;
#endif
	const char *mouse_pressed_sound;
	const char *drop_sound;

};
#endif // ECON_ITEM_SCHEMA_H