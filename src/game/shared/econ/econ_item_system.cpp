#include "cbase.h"
#include "utlbuffer.h"
#include "econ_item_system.h"
#include "activitylist.h"
#include <filesystem.h>
#include "tf_inventory.h"
#include "props_shared.h"

#ifdef GAME_DLL
#include "sceneentity.h"
#include "dt_send.h"
#else
#include "hud.h"
#include "dt_recv.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GET_STRING( copyto, from, name )																			\
		copyto->name = from->GetString( #name, copyto->name )

#define GET_BOOL( copyto, from, name )																				\
		copyto->name = from->GetBool( #name, copyto->name )

#define GET_FLOAT( copyto, from, name )																				\
		copyto->name = from->GetFloat( #name, copyto->name )

#define GET_INT( copyto, from, name )																				\
		copyto->name = from->GetInt( #name, copyto->name )

#define FIND_ELEMENT( map, key, val )																				\
		unsigned int index = map.Find( key );																		\
		if ( index != map.InvalidIndex() )																			\
			val = map.Element( index )				

#define FIND_ELEMENT_STRING( map, key, val )																		\
		unsigned int index = map.Find( key );																		\
		if ( index != map.InvalidIndex() )																			\
			Q_snprintf( val, sizeof( val ), map.Element( index ) )

#define IF_ELEMENT_FOUND( map, key )																				\
		unsigned int index = map.Find( key );																		\
		if ( index != map.InvalidIndex() )			

#define GET_VALUES_FAST_BOOL( dict, keys )																			\
		for ( KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey() )	\
		{																											\
			IF_ELEMENT_FOUND( dict, pKeyData->GetName() )															\
			{																										\
				dict.Element( index ) = pKeyData->GetBool();														\
			}																										\
			else																									\
			{																										\
				dict.Insert( pKeyData->GetName(), pKeyData->GetBool() );											\
			}																										\
		}


#define GET_VALUES_FAST_STRING( dict, keys )																		\
		for ( KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey() )	\
		{																											\
			dict.Insert( pKeyData->GetName(), strdup( pKeyData->GetString() ) );									\
		}

const char *g_TeamVisualSections[TF_TEAM_COUNT] =
{
	"visuals",			// TEAM_UNASSIGNED
	"",					// TEAM_SPECTATOR
	"visuals_red",		// TEAM_RED
	"visuals_blu",		// TEAM_BLUE
	"visuals_grn",		// TEAM_GREEN
	"visuals_ylw",		// TEAM_YELLOW
	//"visuals_mvm_boss"	// ???
};

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",
	"value_is_inverted_percentage",
	"value_is_additive",
	"value_is_additive_percentage",
	"value_is_or",
	"value_is_date",
	"value_is_account_id",
	"value_is_particle_index",
	"value_is_killstreakeffect_index",
	"value_is_killstreak_idleeffect_index",
	"value_is_item_def",
	"value_is_from_lookup_table"
};

const char *g_EffectTypes[] =
{
	"unusual",
	"strange",
	"neutral",
	"positive",
	"negative"
};

const char *g_szQualityStrings[] =
{
	"normal",
	"rarity1",
	"rarity2",
	"vintage",
	"rarity3",
	"rarity4",
	"unique",
	"community",
	"developer",
	"selfmade",
	"customized",
	"strange",
	"completed",
	"haunted",
	"collectors",
	"paintkitWeapon",
	"default",
	"common",
	"uncommon",
	"rare",
	"mythical",
	"legendary",
	"ancient",
};

const char *g_szQualityColorStrings[] =
{
	"QualityColorNormal",
	"QualityColorrarity1",
	"QualityColorrarity2",
	"QualityColorVintage",
	"QualityColorrarity3",
	"QualityColorrarity4",
	"QualityColorUnique",
	"QualityColorCommunity",
	"QualityColorDeveloper",
	"QualityColorSelfMade",
	"QualityColorSelfMadeCustomized",
	"QualityColorStrange",
	"QualityColorCompleted",
	"QualityColorHaunted",
	"QualityColorCollectors",
	"QualityColorPaintkitWeapon",
	"ItemRarityDefault",
	"ItemRarityCommon",
	"ItemRarityUncommon",
	"ItemRarityRare",
	"ItemRarityMythical",
	"ItemRarityLegendary",
	"ItemRarityAncient",
};

const char *g_szQualityLocalizationStrings[] =
{
	"#Normal",
	"#rarity1",
	"#rarity2",
	"#vintage",
	"#rarity3",
	"#rarity4",
	"#unique",
	"#community",
	"#developer",
	"#selfmade",
	"#customized",
	"#strange",
	"#completed",
	"#haunted",
	"#collectors",
	"#paintkitWeapon",
	"#Rarity_Default",
	"#Rarity_Common",
	"#Rarity_Uncommon",
	"#Rarity_Rare",
	"#Rarity_Mythical",
	"#Rarity_Legendary",
	"#Rarity_Ancient",
};

const char *g_szItemBorders[][5] =
{
	// Normal							// Mouseover								// Selected						// Disabled									// Disabled selected
	{ "BackpackItemBorder",					"BackpackItemMouseOverBorder",					"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder",					"BackpackItemGreyedOutSelectedBorder"					},
	{ "BackpackItemBorder_1",				"BackpackItemMouseOverBorder_1",				"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_1",				"BackpackItemGreyedOutSelectedBorder_1"					},
	{ "BackpackItemBorder_2",				"BackpackItemMouseOverBorder_2",				"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_2",				"BackpackItemGreyedOutSelectedBorder_2"					},
	{ "BackpackItemBorder_Vintage",			"BackpackItemMouseOverBorder_Vintage",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Vintage",			"BackpackItemGreyedOutSelectedBorder_Vintage"			},
	{ "BackpackItemBorder_3",				"BackpackItemMouseOverBorder_3",				"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_3",				"BackpackItemGreyedOutSelectedBorder_3"					},
	{ "BackpackItemBorder_4",				"BackpackItemMouseOverBorder_4",				"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_4",				"BackpackItemGreyedOutSelectedBorder_4"					},
	{ "BackpackItemBorder_Unique",			"BackpackItemMouseOverBorder_Unique",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Unique",			"BackpackItemGreyedOutSelectedBorder_Unique"			},
	{ "BackpackItemBorder_Community",		"BackpackItemMouseOverBorder_Community",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Community",		"BackpackItemGreyedOutSelectedBorder_Community"			},
	{ "BackpackItemBorder_Developer",		"BackpackItemMouseOverBorder_Developer",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Developer",		"BackpackItemGreyedOutSelectedBorder_Developer"			},
	{ "BackpackItemBorder_SelfMade",		"BackpackItemMouseOverBorder_SelfMade",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_SelfMade",			"BackpackItemGreyedOutSelectedBorder_SelfMade"			},
	{ "BackpackItemBorder_Customized",		"BackpackItemMouseOverBorder_Customized",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Customized",		"BackpackItemGreyedOutSelectedBorder_Customized"		},
	{ "BackpackItemBorder_Strange",			"BackpackItemMouseOverBorder_Strange",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Strange",			"BackpackItemGreyedOutSelectedBorder_Strange"			},
	{ "BackpackItemBorder_Completed",		"BackpackItemMouseOverBorder_Completed",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Completed",		"BackpackItemGreyedOutSelectedBorder_Completed"			},
	{ "BackpackItemBorder_Haunted",			"BackpackItemMouseOverBorder_Haunted",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Haunted",			"BackpackItemGreyedOutSelectedBorder_Haunted"			},
	{ "BackpackItemBorder_Collectors",		"BackpackItemMouseOverBorder_Collectors",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Collectors",		"BackpackItemGreyedOutSelectedBorder_Collectors"		},

	{ "BackpackItemBorder_PaintkitWeapon",	"BackpackItemMouseOverBorder_PaintkitWeapon",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_PaintkitWeapon",	"BackpackItemGreyedOutSelectedBorder_PaintkitWeapon"	},
	{ "BackpackItemBorder_RarityDefault",	"BackpackItemMouseOverBorder_RarityDefault",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityDefault",	"BackpackItemGreyedOutSelectedBorder_RarityDefault"		},
	{ "BackpackItemBorder_RarityCommon",	"BackpackItemMouseOverBorder_RarityCommon",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityCommon",		"BackpackItemGreyedOutSelectedBorder_RarityCommon"		},
	{ "BackpackItemBorder_RarityUncommon",	"BackpackItemMouseOverBorder_RarityUncommon",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityUncommon",	"BackpackItemGreyedOutSelectedBorder_RarityUncommon"	},
	{ "BackpackItemBorder_RarityRare",		"BackpackItemMouseOverBorder_RarityRare",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityRare",		"BackpackItemGreyedOutSelectedBorder_RarityRare"		},
	{ "BackpackItemBorder_RarityMythical",	"BackpackItemMouseOverBorder_RarityMythical",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityMythical",	"BackpackItemGreyedOutSelectedBorder_RarityMythical"	},
	{ "BackpackItemBorder_RarityLegendary",	"BackpackItemMouseOverBorder_RarityLegendary",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityLegendary",	"BackpackItemGreyedOutSelectedBorder_RarityLegendary"	},
	{ "BackpackItemBorder_RarityAncient",	"BackpackItemMouseOverBorder_RarityAncient",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityAncient",	"BackpackItemGreyedOutSelectedBorder_RarityAncient"		},
};

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
//-----------------------------------------------------------------------------
// Purpose: Attempts to grab the right ISteamHTTP interface.
//-----------------------------------------------------------------------------
static ISteamHTTP *GetISteamHTTP()
{
	if ( steamapicontext && steamapicontext->SteamHTTP() )
		return steamapicontext->SteamHTTP();

#ifndef CLIENT_DLL
	if ( steamgameserverapicontext )
		return steamgameserverapicontext->SteamHTTP();
#endif

	return NULL;
}

#ifdef GAME_DLL
static void RemoteItemSchemaCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVarRef remoteaddress( var );

	const char *pszNewString = remoteaddress.GetString();
#else
static void RemoteItemSchemaCallback( const CCommand &args )
{
	const char *pszNewString = args[1];
#endif
	if ( V_strcmp( pszNewString, "" ) )
	{
		CEconItemSchema *pItemSchema = GetItemSchema();
		if ( pItemSchema )
		{
			pItemSchema->ParseRemoteItemSchema( pszNewString );
		}
	}
	else
	{
		GetTFInventory()->CheckForCustomSchema( true );
	}

#ifdef GAME_DLL
	CBasePlayer *pPlayer;
	for ( int i = 1; i < gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			engine->ClientCommand( pPlayer->edict(), "_tf2c_remote_item_schema \"%s\"\n", pszNewString );
		}
	}
}
ConVar tf2c_remote_item_schema( "tf2c_remote_item_schema", "", FCVAR_NONE, "The remote address for the server and all clients to load in a custom item schema from", RemoteItemSchemaCallback );
#else
}

ConCommand tf2c_remote_item_schema( "_tf2c_remote_item_schema", RemoteItemSchemaCallback, "", FCVAR_HIDDEN | FCVAR_SERVER_CAN_EXECUTE );
#endif
#endif


void InitPerClassStringArray( KeyValues *pKeys, const char **pArray )
{
	if ( !pKeys )
		return;

	for ( KeyValues *pClassData = pKeys->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
	{
		const char *pszClass = pClassData->GetName();
		int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
		if ( iClass != -1 )
		{
			const char *pszValue = pClassData->GetString();
			if ( pszValue && pszValue[0] )
			{
				pArray[iClass] = pszValue;
			}
		}
	}
}


void InitPerClassStringVectorArray( KeyValues *pKeys, CUtlVector<const char *> *pVector )
{
	if ( !pKeys )
		return;

	for ( KeyValues *pClassData = pKeys->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
	{
		const char *pszClass = pClassData->GetName();
		int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
		if ( iClass != -1 )
		{
			// See if it's specified as the value.
			const char *pszValue = pClassData->GetString();
			if ( pszValue && pszValue[0] )
			{
				pVector[iClass].AddToTail( pszValue );
			}

			// Check subkeys.
			for ( KeyValues *pSubData = pClassData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
			{
				const char *pszSubValue = pSubData->GetString();
				if ( pszSubValue && pszSubValue[0] )
				{
					pVector[iClass].AddToTail( pszSubValue );
				}
			}
		}
	}
}


static CEconItemSchema g_EconItemSchema;
CEconItemSchema *GetItemSchema()
{
	return &g_EconItemSchema;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema()
{
	SetDefLessFunc( m_Items );
	SetDefLessFunc( m_Attributes );

	m_pSchemaData = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemSchema::~CEconItemSchema()
{
	Reset();
	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();
	
	if ( m_pSchemaData )
	{
		m_pSchemaData->deleteThis();
		m_pSchemaData = NULL;
	}
}


void CEconItemSchema::Reset( void )
{
	m_GameInfo.Purge();
	m_PrefabsValues.Purge();
	m_Qualities.Purge();
	m_Colors.Purge();
	m_Attributes.Purge();
	m_Items.Purge();
	
	if ( m_pSchemaData )
	{
		m_pSchemaData->deleteThis();
		m_pSchemaData = NULL;
	}
}

// KV leak somewhere in these two funcs...
// -sappho
bool CEconItemSchema::ParseItemSchema_Internal(KeyValues* pSchemaData)
{
	// Must register activities early so we can parse animation replacements.
	ActivityList_Free();
	ActivityList_RegisterSharedActivities();
	
	float flStartTime = engine->Time();
	
	KeyValues* pGameInfo = pSchemaData->FindKey("game_info");
	if (pGameInfo)
	{
		ParseGameInfo(pGameInfo);
	}
	
	KeyValues* pPrefabs = pSchemaData->FindKey("prefabs");
	if (pPrefabs)
	{
		ParsePrefabs(pPrefabs);
	}
	
	KeyValues* pQualities = pSchemaData->FindKey("qualities");
	if (pQualities)
	{
		ParseQualities(pQualities);
	}
	
	KeyValues* pColors = pSchemaData->FindKey("colors");
	if (pColors)
	{
		ParseColors(pColors);
	}
	
	KeyValues* pAttributes = pSchemaData->FindKey("attributes");
	if (pAttributes)
	{
		ParseAttributes(pAttributes);
	}
	
	KeyValues* pItems = pSchemaData->FindKey("items");
	if (pItems)
	{
		ParseItems(pItems);
	}
	
	// Add it into the overall data of everything.
	m_pSchemaData->AddSubKey(pSchemaData);
	
	float flEndTime = engine->Time();
	DevMsg("[%s] Processing of Item Schema took %.02fms. Total: %d items and %d attributes.\n",
		CBaseEntity::IsServer() ? "SERVER" : "CLIENT",
		(flEndTime - flStartTime) * 1000.0f,
		m_Items.Count(),
		m_Attributes.Count());
	
	return true;
}

// KV leak somewhere in these two funcs...
// -sappho
bool CEconItemSchema::ParseItemSchema( const char *pszFile )
{
	/*
	cache default schema eventually
	if (V_stricmp(pszFile, DEFAULT_ITEM_SCHEMA_FILE))
	{
		if (!default_item_schema_buf)
		{
			CUtlBuffer fileBuffer(4096, 1024 * 1024, CUtlBuffer::READ_ONLY);
			if (!filesystem->ReadFile(DEFAULT_ITEM_SCHEMA_FILE, "GAME", fileBuffer))	// this ignores .nav files embedded in the .bsp ...

			default_item_schema_buf = UTIL_LoadFileForMe(DEFAULT_ITEM_SCHEMA_FILE, NULL);
		}
		CUtlBuffer 
		ParseItemSchema(default_item_schema_buf)
	}
	*/
	if ( !m_pSchemaData )
	{
		m_pSchemaData = new KeyValues( "ItemData" );
	}


	KeyValues *pSchemaData = new KeyValues( "ItemData" );
	if ( !pSchemaData->LoadFromFile( filesystem, pszFile, "GAME" ) )
	{
		DevWarning( "[%s] %s is missing or corrupt!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT", pszFile );
		return false;
	}
	ParseItemSchema_Internal(pSchemaData);

	return true;
}


bool CEconItemSchema::ParseItemSchema( CUtlBuffer &bufRawData )
{
	if ( !m_pSchemaData )
	{
		m_pSchemaData = new KeyValues( "ItemData" );
	}
	
	KeyValues *pSchemaData = new KeyValues( "ItemData" );
	if ( !pSchemaData->LoadFromBuffer( NULL, bufRawData ) )
	{
		DevWarning( "[%s] Buffer is invalid or corrupt!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT" );
		return false;
	}

	ParseItemSchema_Internal(pSchemaData);

	return true;
}

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP

bool CEconItemSchema::ParseRemoteItemSchema( const char *pszAddress )
{
	ISteamHTTP *pHTTP = GetISteamHTTP();
	if ( !pHTTP )
	{
		DevWarning( "Can't get ISteamHTTP to request custom item schema!\n" );
		return false;
	}

	const char *pszRemoteAddress = pszAddress;
	HTTPRequestHandle hReq = pHTTP->CreateHTTPRequest( k_EHTTPMethodGET, pszRemoteAddress );
	pHTTP->SetHTTPRequestNetworkActivityTimeout( hReq, 10 );
	SteamAPICall_t hCall;
	if ( !pHTTP->SendHTTPRequest( hReq, &hCall ) )
	{
		DevWarning( "Failed to request custom item schema: Couldn't fetch from '%s'!\n", pszRemoteAddress );
		return false;
	}
	
#ifndef CLIENT_DLL
	if ( steamgameserverapicontext && steamgameserverapicontext->SteamHTTP() == pHTTP )
	{
		callback.SetGameserverFlag();
	}
#endif
	callback.Set( hCall, this, &CEconItemSchema::OnHTTPCompleted );
	return true;
}
#endif


void CEconItemSchema::ParseGameInfo( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		m_GameInfo.Insert( pSubData->GetName(), pSubData->GetFloat() );
	}
}


void CEconItemSchema::ParseQualities( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		EconQuality Quality;
		GET_INT( ( &Quality ), pSubData, value );
		m_Qualities.Insert( pSubData->GetName(), Quality );
	}
}


void CEconItemSchema::ParseColors( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		EconColor ColorDesc;
		GET_STRING( ( &ColorDesc ), pSubData, color_name );
		m_Colors.Insert( pSubData->GetName(), ColorDesc );
	}
}


void CEconItemSchema::ParsePrefabs( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		m_PrefabsValues.Insert( pSubData->GetName(), pSubData );
	}
}


void CEconItemSchema::ParseItems( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		// Skip over default item, not sure why it's there.
		if ( !V_stricmp( pSubData->GetName(), "default" ) )
			continue;

		CEconItemDefinition *Item = new CEconItemDefinition;
		Item->index = atoi( pSubData->GetName() );

		if ( ParseItemRec( pSubData, Item ) )
		{
			// 'InsertOrReplace' allows custom Item Schemas to override our existing weapons.
			m_Items.InsertOrReplace( Item->index, Item );
		}
		else
		{
			delete Item;
		}
	}
}


void CEconItemSchema::ParseAttributes( KeyValues *pKeyValuesData )
{
	FOR_EACH_SUBKEY( pKeyValuesData, pSubData )
	{
		CEconAttributeDefinition *pAttribute = new CEconAttributeDefinition;
		pAttribute->index = atoi( pSubData->GetName() );

		GET_STRING( pAttribute, pSubData, name );
		GET_STRING( pAttribute, pSubData, attribute_class );
		GET_STRING( pAttribute, pSubData, description_string );
		pAttribute->string_attribute = ( V_stricmp( pSubData->GetString( "attribute_type" ), "string" ) == 0 );

		const char *pszFormat = pSubData->GetString( "description_format" );
		pAttribute->description_format = UTIL_StringFieldToInt( pszFormat, g_AttributeDescriptionFormats, ARRAYSIZE( g_AttributeDescriptionFormats ) );
		if ( pAttribute->description_format == -1 )
		{
			pAttribute->description_format = ATTRIB_FORMAT_ADDITIVE;
		}

		const char *pszEffect = pSubData->GetString( "effect_type" );
		pAttribute->effect_type = UTIL_StringFieldToInt( pszEffect, g_EffectTypes, ARRAYSIZE( g_EffectTypes ) );

		Color pszCustomEffect = pSubData->GetColor("custom_color");
		pAttribute->custom_color = pszCustomEffect;

		GET_BOOL( pAttribute, pSubData, hidden );
		GET_BOOL( pAttribute, pSubData, stored_as_integer );
		GET_BOOL( pAttribute, pSubData, hidden_separator );

		m_Attributes.Insert( pAttribute->index, pAttribute );
	}
}


bool CEconItemSchema::ParseVisuals( KeyValues *pData, CEconItemDefinition* pItem, int iIndex )
{
	EconItemVisuals *pVisuals = &pItem->visual[iIndex];
	FOR_EACH_SUBKEY( pData, pVisualData )
	{
		const char *pszName = pVisualData->GetName();
		if ( !V_stricmp( pszName, "animation_replacement" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pReplacementData )
			{
				int key = ActivityList_IndexForName( pReplacementData->GetName() );
				int value = ActivityList_IndexForName( pReplacementData->GetString() );
				if ( key != kActivityLookup_Missing && value != kActivityLookup_Missing )
				{
					pVisuals->animation_replacement.Insert( key, value );
				}
			}
		}
		else if ( !V_stricmp( pszName, "attached_models" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pAttachedModelData )
			{
				int iAtt = pVisuals->m_AttachedModels.AddToTail();
				pVisuals->m_AttachedModels[iAtt].m_iModelDisplayFlags = pAttachedModelData->GetInt( "model_display_flags", kAttachedModelDisplayFlag_MaskAll );
				pVisuals->m_AttachedModels[iAtt].m_pszModelName = pAttachedModelData->GetString( "model", NULL );
			}
		}
		else if ( !V_strnicmp( pszName, "custom_sound", 12 ) )
		{
			int idx = atoi( pszName + 12 );
			if ( idx >= 0 && idx < ARRAYSIZE( pVisuals->custom_sound ) )
			{
				pVisuals->custom_sound[idx] = pVisualData->GetString();
			}
		}
		else if ( !V_strnicmp( pszName, "sound_", 6 ) )
		{
			// Fetching this similar to weapon script file parsing.
			// Advancing pointer past sound_ prefix... why couldn't they just make a subsection for sounds?
			int iSound = GetWeaponSoundFromString( pszName + 6 );
			if ( iSound != -1 )
			{
				pVisuals->sound_weapons[iSound] = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pszName, "player_bodygroups" ) )
		{
			GET_VALUES_FAST_BOOL( pVisuals->player_bodygroups, pVisualData );
		}
		else if ( !V_stricmp( pszName, "tracer_effect" ) )
		{
			if ( pVisualData->GetString() )
			{
				pVisuals->tracer_effect = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pszName, "explosion_effect" ) )
		{
			if ( pVisualData->GetString() )
			{
				pVisuals->explosion_effect = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pszName, "trail_effect" ) )
		{
			if ( pVisualData->GetString() )
			{
				pVisuals->trail_effect = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pszName, "explosion_effect_crit" ) )
		{
			if ( pVisualData->GetString() )
			{
				pVisuals->explosion_effect_crit = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pszName, "trail_effect_crit" ) )
		{
			if ( pVisualData->GetString() )
			{
				pVisuals->trail_effect_crit = pVisualData->GetString();
			}
		}
		else if (!V_stricmp(pszName, "beam_effect"))
		{
			if (pVisualData->GetString())
			{
				pVisuals->beam_effect = pVisualData->GetString();
			}
		}
		else if (!V_stricmp(pszName, "beam_effect_crit"))
		{
			if (pVisualData->GetString())
			{
				pVisuals->beam_effect_crit = pVisualData->GetString();
			}
		}
	}

	return true;
}


bool CEconItemSchema::ParseItemRec( KeyValues *pData, CEconItemDefinition* pItem )
{
	char prefab[128];
	V_strncpy( prefab, pData->GetString( "prefab" ), sizeof( prefab ) );	// Check if there's prefab for prefa- PREFABSEPTION!
	if ( prefab[0] != '\0' )
	{
		char *pch = strtok( prefab, " " );
		while ( pch != NULL )
		{
			KeyValues *pPrefabValues = NULL;
			FIND_ELEMENT( m_PrefabsValues, pch, pPrefabValues );
			if ( pPrefabValues )
			{
				pData->RecursiveMergeKeyValues( pPrefabValues );
			}

			pch = strtok( NULL, " " );
		}
	}

	GET_STRING( pItem, pData, name );
	GET_BOOL( pItem, pData, show_in_armory );
	GET_BOOL( pItem, pData, testing );

	GET_STRING( pItem, pData, item_class );
	GET_STRING( pItem, pData, item_name );
	GET_STRING( pItem, pData, item_description );
	GET_STRING( pItem, pData, item_type_name );

	const char *pszQuality = pData->GetString( "item_quality" );
	if ( pszQuality[0] )
	{
		int iQuality = UTIL_StringFieldToInt( pszQuality, g_szQualityStrings, ARRAYSIZE( g_szQualityStrings ) );
		if ( iQuality != -1 )
		{
			pItem->item_quality = (EEconItemQuality)iQuality;
		}
	}

	Color pszItemNameColor = pData->GetColor( "item_name_color" );
	pItem->item_name_color = pszItemNameColor;

	GET_STRING( pItem, pData, item_logname );
	GET_STRING( pItem, pData, item_iconname );
	GET_BOOL( pItem, pData, propername );

	const char *pszLoadoutSlot = pData->GetString( "item_slot" );
	if ( pszLoadoutSlot[0] )
	{
		pItem->item_slot = (ETFLoadoutSlot)UTIL_StringFieldToInt( pszLoadoutSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
	}

	if ( pItem->item_slot == -1 )
		return false;

	const char *pszAnimSlot = pData->GetString( "anim_slot" );
	if ( pszAnimSlot[0] )
	{
		if ( V_strcmp( pszAnimSlot, "FORCE_NOT_USED" ) != 0 )
		{
			pItem->anim_slot = (ETFWeaponType)UTIL_StringFieldToInt( pszAnimSlot, g_AnimSlots, TF_WPN_TYPE_COUNT );
		}
		else
		{
			pItem->anim_slot = TF_WPN_TYPE_NOT_USED;
		}
	}

	GET_INT( pItem, pData, bucket );
	GET_INT( pItem, pData, bucket_position );

	if ( !pItem->show_in_armory ) GET_BOOL( pItem, pData, baseitem );
	GET_INT( pItem, pData, min_ilevel );
	GET_INT( pItem, pData, max_ilevel );

	GET_STRING( pItem, pData, image_inventory );
	GET_INT( pItem, pData, image_inventory_size_w );
	GET_INT( pItem, pData, image_inventory_size_h );

	GET_STRING( pItem, pData, model_player );
	GET_STRING( pItem, pData, model_world );

	GET_INT( pItem, pData, attach_to_hands );

	GET_BOOL( pItem, pData, act_as_wearable );
	GET_STRING( pItem, pData, extra_wearable );
	GET_BOOL( pItem, pData, extra_wearable_hide_on_active );

	GET_BOOL( pItem, pData, hide_bodygroups_deployed_only );

	GET_BOOL( pItem, pData, flip_viewmodel );

	GET_STRING( pItem, pData, mouse_pressed_sound );
	GET_STRING( pItem, pData, drop_sound );

	FOR_EACH_SUBKEY( pData, pSubData )
	{
		if ( !V_stricmp( pSubData->GetName(), "capabilities" ) )
		{
			GET_VALUES_FAST_BOOL( pItem->capabilities, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "tags" ) )
		{
			GET_VALUES_FAST_BOOL( pItem->tags, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "model_player_per_class" ) )
		{
			InitPerClassStringArray( pSubData, pItem->model_player_per_class );
		}
		else if ( !V_stricmp( pSubData->GetName(), "model_world_per_class" ) )
		{
			InitPerClassStringArray( pSubData, pItem->model_world_per_class );
		}
		else if ( !V_stricmp( pSubData->GetName(), "used_by_classes" ) )
		{
			pItem->used_by_classes = 0;

			FOR_EACH_SUBKEY( pSubData, pClassData )
			{
				int iClass = UTIL_StringFieldToInt( pClassData->GetName(), g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
				if ( iClass != -1 )
				{
					pItem->used_by_classes |= ( 1 << iClass );

					const char *pszSlotname = pClassData->GetString();
					if ( pszSlotname[0] != '1' )
					{
						pItem->item_slot_per_class[iClass] = (ETFLoadoutSlot)UTIL_StringFieldToInt( pszSlotname, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
					}
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "attributes" ) )
		{
			FOR_EACH_SUBKEY( pSubData, pAttribData )
			{
				int iAttributeID = GetAttributeIndex( pAttribData->GetName() );
				if ( iAttributeID == -1 )
				{
					DevMsg( "[%s] Error processing item '%s' attribute: '%s' has no associated ID!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT", pItem->item_name, pAttribData->GetName() );
					continue;
				}

				CEconAttributeDefinition *pAttribDef = GetAttributeDefinition( iAttributeID );
				if ( pAttribDef->string_attribute )
				{
					CEconItemAttribute attribute( iAttributeID, pAttribData->GetString( "value" ), pAttribData->GetString( "attribute_class" ) );
					pItem->attributes.AddToTail( attribute );
				}
				else
				{
					CEconItemAttribute attribute( iAttributeID, pAttribData->GetFloat( "value" ), pAttribData->GetString( "attribute_class" ) );
					pItem->attributes.AddToTail( attribute );
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "static_attrs" ) )
		{
			FOR_EACH_SUBKEY( pSubData, pAttribData )
			{
				int iAttributeID = GetAttributeIndex( pAttribData->GetName() );
				if ( iAttributeID == -1 )
				{
					DevMsg( "[%s] Error processing item '%s' attribute: '%s' has no associated ID!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT", pItem->item_name, pAttribData->GetName() );
					continue;
				}

				CEconAttributeDefinition *pAttribDef = GetAttributeDefinition( iAttributeID );
				if ( pAttribDef->string_attribute )
				{
					CEconItemAttribute attribute( iAttributeID, pAttribData->GetString(), pAttribDef->attribute_class );
					pItem->attributes.AddToTail( attribute );
				}
				else
				{
					CEconItemAttribute attribute( iAttributeID, pAttribData->GetFloat(), pAttribDef->attribute_class );
					pItem->attributes.AddToTail( attribute );
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "visuals_mvm_boss" ) )
		{
			// Deliberately skipping this.
		}
		else if ( !V_strnicmp( pSubData->GetName(), "visuals", 7 ) )
		{
			// Figure out what team is this meant for.
			int iVisuals = UTIL_StringFieldToInt( pSubData->GetName(), g_TeamVisualSections, TF_TEAM_COUNT );
			if ( iVisuals != -1 )
			{
				if ( iVisuals == TEAM_UNASSIGNED )
				{
					// Hacky: for standard visuals block, assign it to all teams at once.
					for ( int i = 0; i < TF_TEAM_COUNT; i++ )
					{
						if ( i == TEAM_SPECTATOR )
							continue;

						ParseVisuals( pSubData, pItem, i );
					}
				}
				else
				{
					ParseVisuals( pSubData, pItem, iVisuals );
				}
			}
		}
#ifdef ITEM_TAUNTING
		else if ( !V_stricmp( pSubData->GetName(), "taunt" ) )
		{
			CTFTauntInfo *pTaunt = &pItem->taunt;

			// Team Fortress 2 Classic
			KeyValues *pTauntForceWeaponSlotPerClass = pSubData->FindKey( "taunt_force_weapon_slot_per_class" );
			if ( pTauntForceWeaponSlotPerClass )
			{
				FOR_EACH_SUBKEY( pSubData, pClassData )
				{
					int iClass = UTIL_StringFieldToInt( pClassData->GetName(), g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						pTaunt->taunt_force_weapon_slot_per_class[iClass] = (ETFLoadoutSlot)UTIL_StringFieldToInt( pClassData->GetString(), g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
					}
				}
			}

			InitPerClassStringVectorArray( pSubData->FindKey( "custom_taunt_scene_per_class" ), pTaunt->custom_taunt_scene_per_class );

			KeyValues *pClassPartnerKeys = pSubData->FindKey( "custom_partner_taunt_per_class" );
			if ( pClassPartnerKeys )
			{
				// For backwards compatibility: add them as both iniator and receiever scenes.
				InitPerClassStringVectorArray( pClassPartnerKeys, pTaunt->custom_partner_taunt_initiator_per_class );
				InitPerClassStringVectorArray( pClassPartnerKeys, pTaunt->custom_partner_taunt_receiver_per_class );
			}

			InitPerClassStringVectorArray( pSubData->FindKey( "custom_partner_taunt_initiator_per_class" ), pTaunt->custom_partner_taunt_initiator_per_class );
			InitPerClassStringVectorArray( pSubData->FindKey( "custom_partner_taunt_receiver_per_class" ), pTaunt->custom_partner_taunt_receiver_per_class );
			InitPerClassStringVectorArray( pSubData->FindKey( "custom_taunt_outro_scene_per_class" ), pTaunt->custom_taunt_outro_scene_per_class );

			InitPerClassStringArray( pSubData->FindKey( "custom_taunt_prop_per_class" ), pTaunt->custom_taunt_prop_per_class );
			InitPerClassStringArray( pSubData->FindKey( "custom_taunt_prop_scene_per_class" ), pTaunt->custom_taunt_prop_scene_per_class );
			InitPerClassStringArray( pSubData->FindKey( "custom_taunt_prop_outro_scene_per_class" ), pTaunt->custom_taunt_prop_outro_scene_per_class );

			const char *pszTauntAttack = pSubData->GetString( "taunt_attack_name" );
			if ( pszTauntAttack[0] )
			{
				int iTauntAttack = UTIL_StringFieldToInt( pszTauntAttack, taunt_attack_name, TAUNTATK_COUNT );
				if ( iTauntAttack != -1 )
				{
					pTaunt->taunt_attack = iTauntAttack;
				}
			}

			GET_BOOL( pTaunt, pSubData, is_hold_taunt );
			GET_BOOL( pTaunt, pSubData, is_partner_taunt );
			GET_FLOAT( pTaunt, pSubData, taunt_attack_time );
			GET_FLOAT( pTaunt, pSubData, taunt_separation_forward_distance );
			GET_BOOL( pTaunt, pSubData, stop_taunt_if_moved );
			GET_STRING( pTaunt, pSubData, taunt_success_sound );
			GET_STRING( pTaunt, pSubData, taunt_success_sound_loop );
			GET_FLOAT( pTaunt, pSubData, taunt_move_speed );
			GET_FLOAT( pTaunt, pSubData, taunt_turn_speed );
			GET_BOOL( pTaunt, pSubData, taunt_force_move_forward );
			GET_BOOL( pTaunt, pSubData, taunt_mimic );

			const char *pszWeaponSlot = pSubData->GetString( "taunt_force_weapon_slot", NULL );
			if ( pszWeaponSlot )
			{
				ETFLoadoutSlot iSlot = (ETFLoadoutSlot)UTIL_StringFieldToInt( pszWeaponSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
				if ( iSlot != TF_LOADOUT_SLOT_INVALID )
				{
					pTaunt->taunt_force_weapon_slot = iSlot;
				}
			}
		}
#endif
	}

	return true;
}

#ifdef GAME_DLL

void PrecacheScenesFromClassVectorArray( CUtlVector<const char *> *pScenes )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		FOR_EACH_VEC( pScenes[iClass], iScene )
		{
			PrecacheInstancedScene( pScenes[iClass][iScene] );
		}
	}
}


void PrecacheScenesFromClassArray( const char **pScenes )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		if ( pScenes[iClass][0] )
		{
			PrecacheInstancedScene( pScenes[iClass] );
		}
	}
}


void PrecacheModelsFromClassArray( const char **pModels )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		if ( pModels[iClass][0] )
		{
			CBaseEntity::PrecacheModel( pModels[iClass] );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Runs on level start, precaches models and sounds from schema.
//-----------------------------------------------------------------------------
void CEconItemSchema::Precache( void )
{
#ifdef GAME_DLL
	string_t strPrecacheAttribute = AllocPooledString( "custom_projectile_model" );
	string_t strPrecacheLunchboxAttribute = AllocPooledString( "custom_lunchbox_throwable_model" );
	string_t strPrecacheHandModelAttribute = AllocPooledString( "custom_hand_viewmodel" );
	string_t strMagazineModelAttribute = AllocPooledString("custom_magazine_model");
#endif

	// Precache everything from schema.
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];

#ifdef GAME_DLL
		// Precache models.
		if ( pItem->model_world[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_world );

		if ( pItem->model_player[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_player );

		PrecacheModelsFromClassArray( pItem->model_player_per_class );

		PrecacheModelsFromClassArray( pItem->model_world_per_class );

		if ( pItem->extra_wearable[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->extra_wearable );

		// Precache visuals.
		for ( int i = 0; i < TF_TEAM_COUNT; i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			EconItemVisuals *pVisuals = &pItem->visual[i];

			for ( int model = 0; model < pVisuals->m_AttachedModels.Count(); model++ )
			{
				CBaseEntity::PrecacheModel( pVisuals->m_AttachedModels[model].m_pszModelName );
			}

			// Precache sounds.
			for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
			{
				if ( pVisuals->sound_weapons[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->sound_weapons[i] );
			}

			for ( int i = 0; i < ARRAYSIZE( pVisuals->custom_sound ); i++ )
			{
				if ( pVisuals->custom_sound[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->custom_sound[i] );
			}

			if ( pVisuals->trail_effect[0] )
			{
				PrecacheParticleSystem( pVisuals->trail_effect );
			}

			if ( pVisuals->trail_effect_crit[0] )
			{
				PrecacheParticleSystem( pVisuals->trail_effect_crit );
			}

			if (pVisuals->beam_effect[0])
			{
				PrecacheParticleSystem(pVisuals->beam_effect);
			}

			if (pVisuals->beam_effect_crit[0])
			{
				PrecacheParticleSystem(pVisuals->beam_effect_crit);
			}
		}

#ifdef ITEM_TAUNTING
		// Precache taunt scenes.
		if ( pItem->item_slot == TF_LOADOUT_SLOT_TAUNT )
		{
			PrecacheScenesFromClassVectorArray( pItem->taunt.custom_taunt_scene_per_class );
			PrecacheScenesFromClassVectorArray( pItem->taunt.custom_taunt_outro_scene_per_class );
			PrecacheScenesFromClassVectorArray( pItem->taunt.custom_partner_taunt_initiator_per_class );
			PrecacheScenesFromClassVectorArray( pItem->taunt.custom_partner_taunt_receiver_per_class );
			PrecacheScenesFromClassArray( pItem->taunt.custom_taunt_prop_scene_per_class );
			PrecacheScenesFromClassArray( pItem->taunt.custom_taunt_prop_outro_scene_per_class );

			// Precache taunt prop models.
			PrecacheModelsFromClassArray( pItem->taunt.custom_taunt_prop_per_class );
		}
#endif
#endif

		// Cache all attrbute names.
		for ( int i = 0; i < pItem->attributes.Count(); i++ )
		{
			CEconItemAttribute *pAttribute = &pItem->attributes[i];
			pAttribute->m_strAttributeClass = AllocPooledString( pAttribute->attribute_class );

#ifdef GAME_DLL
			// Special case for custom_projectile_model attribute.
			if ( pAttribute->m_strAttributeClass == strPrecacheAttribute )
			{
				int iModel = CBaseEntity::PrecacheModel( pAttribute->value_string.Get() );
				PrecacheGibsForModel(iModel);
			}
			
			if ( pAttribute->m_strAttributeClass == strPrecacheLunchboxAttribute )
			{
				CBaseEntity::PrecacheModel( pAttribute->value_string.Get() );
			}

			if ( pAttribute->m_strAttributeClass == strPrecacheHandModelAttribute )
			{
				CBaseEntity::PrecacheModel( pAttribute->value_string.Get() );
			}

			if (pAttribute->m_strAttributeClass == strMagazineModelAttribute)
			{
				CBaseEntity::PrecacheModel( pAttribute->value_string.Get() );
			}
#endif
		}
	}
}


CEconItemDefinition *CEconItemSchema::GetItemDefinition( int id )
{
	if ( id < 0 )
		return NULL;

	CEconItemDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Items, id, itemdef );
	return itemdef;
}


int CEconItemSchema::GetItemIndex( const char *name )
{
	if ( !name || !name[0] )
		return -1;

	FOR_EACH_MAP_FAST( m_Items, i )
	{
		CEconItemDefinition *pItemDef = m_Items.Element( i );
		if ( V_stricmp( pItemDef->name, name ) == 0 )
			return m_Items.Key( i );
	}

	return -1;
}


CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int id )
{
	if ( id < 0 )
		return NULL;

	CEconAttributeDefinition *attribdef = NULL;
	FIND_ELEMENT( m_Attributes, id, attribdef );
	return attribdef;
}


CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *name )
{
	//unsigned int index = m_Attributes.Find(name);
	//if (index < m_Attributes.Count())
	//{
	//	return &m_Attributes[index];
	//}
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}


CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByClass( const char *classname )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->attribute_class, classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}


int CEconItemSchema::GetAttributeIndex( const char *name )
{
	if ( !name )
		return -1;

	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes.Key( i );
		}
	}

	return -1;
}

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP

void CEconItemSchema::OnHTTPCompleted( HTTPRequestCompleted_t *arg, bool bFailed )
{
	ISteamHTTP *pHTTP = GetISteamHTTP();
	Assert( pHTTP );
	if ( !pHTTP ) return;

	if ( arg->m_eStatusCode != k_EHTTPStatusCode200OK )
	{
		DevWarning( "Failed to request custom item schema: HTTP status %d!\n", arg->m_eStatusCode );
	}
	else
	{
		if ( !arg->m_bRequestSuccessful )
		{
			bFailed = true;
		}

		if ( !bFailed )
		{
			uint32 unBodySize;
			if ( !pHTTP->GetHTTPResponseBodySize( arg->m_hRequest, &unBodySize ) )
			{
				Assert( false );
				bFailed = true;
			}
			else
			{
				CUtlBuffer bufRawData;
				bufRawData.SetBufferType( true, true );
				bufRawData.SeekPut( CUtlBuffer::SEEK_HEAD, unBodySize );
				if ( pHTTP->GetHTTPResponseBodyData( arg->m_hRequest, (uint8 *)bufRawData.Base(), bufRawData.TellPut() ) )
				{
					CTFInventory *pInventory = GetTFInventory();
					if ( pInventory )
					{
						pInventory->CheckForCustomSchema( true, true );
						ParseItemSchema( bufRawData );
						pInventory->Init();
						Precache();
						pInventory->SetOverriddenSchema( true );
					}
				}
			}
		}

		if ( bFailed )
		{
			DevWarning( "Failed to request custom item schema!\n" );
		}
	}

	pHTTP->ReleaseHTTPRequest( arg->m_hRequest );
}
#endif

#ifdef GAME_DLL
#ifdef ITEM_TAUNTING
bool WriteSceneFile( const char *filename );


void WriteScenesFromClassVectorArray( CUtlVector<const char *> *pScenes )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		FOR_EACH_VEC( pScenes[iClass], iScene )
		{
			WriteSceneFile( pScenes[iClass][iScene] );
		}
	}
}


void WriteScenesFromClassArray( const char **pScenes )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		if ( pScenes[iClass][0] )
		{
			WriteSceneFile( pScenes[iClass] );
		}
	}
}


void CEconItemSchema::WriteScenes( void )
{
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];
		if ( pItem->item_slot != TF_LOADOUT_SLOT_TAUNT )
			continue;

		WriteScenesFromClassVectorArray( pItem->taunt.custom_taunt_scene_per_class );
		WriteScenesFromClassVectorArray( pItem->taunt.custom_taunt_outro_scene_per_class );
		WriteScenesFromClassVectorArray( pItem->taunt.custom_partner_taunt_initiator_per_class );
		WriteScenesFromClassVectorArray( pItem->taunt.custom_partner_taunt_receiver_per_class );
		WriteScenesFromClassArray( pItem->taunt.custom_taunt_prop_scene_per_class );
		WriteScenesFromClassArray( pItem->taunt.custom_taunt_prop_outro_scene_per_class );
	}
}
#endif
#endif

#ifdef CLIENT_DLL

CON_COMMAND(cl_print_item_schema_items, "Print all currently loaded item IDs and names in console")
{
	GetItemSchema()->PrintItemsToConsole();
}

void CEconItemSchema::PrintItemsToConsole()
{
	FOR_EACH_MAP(m_Items, i)
	{
		CEconItemDefinition* pItemDef = m_Items.Element(i);
		if (pItemDef)
		{
			Msg("%d: %s\n", pItemDef->index, pItemDef->name);
		}
	}
}

CON_COMMAND(cl_print_item_schema_attributes, "Print all currently loaded attributes in console")
{
	GetItemSchema()->PrintAttributesToConsole();
}

void CEconItemSchema::PrintAttributesToConsole()
{
	FOR_EACH_MAP(m_Attributes, i)
	{
		CEconAttributeDefinition* pItemDef = m_Attributes.Element(i);
		if (pItemDef)
		{
			Msg("%d: %s (%s)\n", pItemDef->index, pItemDef->name, pItemDef->attribute_class);
		}
	}
}


#endif
