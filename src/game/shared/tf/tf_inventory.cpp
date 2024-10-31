//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_inventory.h"
#include "econ_item_system.h"
#include "tf_gamerules.h"
#include "filesystem.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include <filesystem.h>

#include "panels/tf_loadoutpanel.h"
#include "tf_mainmenu.h"
#endif

#include <misc_helpers.h>
// #include "fmt/format.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TF_INVENTORY_FILE "tf_inventory.txt"

ConVar tf2c_item_testing( "tf2c_item_testing", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "1 = Enables items with the testing tag in the item schema. 2 = Enables all weapons in the item schema" );


static CTFInventory g_TFInventory;
CTFInventory *GetTFInventory()
{
	return &g_TFInventory;
}

void LoadDefaultItemSchema()
{
	GetItemSchema()->Reset();
	if (!GetItemSchema()->ParseItemSchema(DEFAULT_ITEM_SCHEMA_FILE))
	{
		Error("%s is missing or corrupt! Please verify your local game files.", DEFAULT_ITEM_SCHEMA_FILE);
	}
}

void ResetToDefaultItemSchema()
{
	LoadDefaultItemSchema();
	GetTFInventory()->InitInventorySys();
}

CTFInventory::CTFInventory() : CAutoGameSystem( "CTFInventory" )
{
#ifdef CLIENT_DLL
	m_pInventory = NULL;
#endif
	
	m_bInitialInitialization = false;
	m_bOverriddenSchema = false;
}


CTFInventory::~CTFInventory()
{
#if defined( CLIENT_DLL )
	m_pInventory->deleteThis();
#endif
}

void CTFInventory::PostInit()
{
	// only run on the instance of CTFInventory's PostInit getting called
	// typically it is only going to be the first one
	if (this == (CTFInventory*)&g_TFInventory) 
	{
#if defined (CLIENT_DLL) && (FLUSH_DLS)
		engine->ClientCmd_Unrestricted("flush_map_overrides");
#endif
	}
	GetItemSchema()->Reset();
	InitInventorySys();
}

//-----------------------------------------------------------------------------
// Purpose: Fill the item arrays with data from item schema.
//-----------------------------------------------------------------------------
void CTFInventory::InitInventorySys()
{
	if ( !m_bInitialInitialization )
	{
		// Always load the default Item Schema.
		LoadDefaultItemSchema();
	}
	
	// Clear it out and generate dummy base items.
	for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++ )
		{
			m_Items[iClass][iSlot].PurgeAndDeleteElements();
			m_Items[iClass][iSlot].AddToTail( NULL );
		}
	}

	// Generate item list.
	FOR_EACH_MAP( GetItemSchema()->m_Items, i )
	{
		int iItemID = GetItemSchema()->m_Items.Key( i );
		CEconItemDefinition *pItemDef = GetItemSchema()->m_Items.Element( i );

		if ( pItemDef->item_slot == TF_LOADOUT_SLOT_INVALID )
			continue;

		// Add it to each class that uses it.
		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			if ( pItemDef->used_by_classes & ( 1 << iClass ) )
			{
				// Show it if it's either base item or has show_in_armory flag.
				ETFLoadoutSlot iSlot = pItemDef->GetLoadoutSlot( iClass );
				CEconItemView *pNewItem = NULL;

				if ( pItemDef->baseitem )
				{
					CEconItemView *pBaseItem = m_Items[iClass][iSlot][0];
					if ( pBaseItem != NULL )
					{
						DevWarning( "[%s] Duplicate base item %d for class '%s' in slot '%s'!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT", iItemID, g_aPlayerClassNames_NonLocalized[iClass], g_LoadoutSlots[iSlot] );
						delete pBaseItem;
					}

					pNewItem = new CEconItemView( iItemID );
					m_Items[iClass][iSlot][0] = pNewItem;
				}
				else if ( pItemDef->show_in_armory || (tf2c_item_testing.GetInt() == 1 && pItemDef->testing) || tf2c_item_testing.GetInt() == 2 )
				{
					pNewItem = new CEconItemView( iItemID );
					m_Items[iClass][iSlot].AddToTail( pNewItem );
				}

				if ( pNewItem )
				{
					pNewItem->SetClassIndex( iClass );
				}
			}
		}
	}

#if defined( CLIENT_DLL )
	LoadInventory();
#endif
	
	m_bInitialInitialization = true;

	return;
}

void CTFInventory::Shutdown(void)
{
	GetItemSchema()->Reset();
}
void CTFInventory::LevelInitPreEntity( void )
{
	CheckForCustomSchema();
}


void CTFInventory::CheckForCustomSchema( bool bAlwaysReload /*= false*/, bool bSkipInitialization /*= false*/ )
{
	m_bOverriddenSchema = false;

	char szMapName[256];
#if !defined( CLIENT_DLL )
	V_snprintf( szMapName, sizeof( szMapName ), "maps/%s", STRING( gpGlobals->mapname ) );
#else
	V_strncpy( szMapName, engine->GetLevelName(), sizeof( szMapName ) );
#endif

	V_FixSlashes( szMapName );
	V_strlower( szMapName );
	
	char szScriptFile[512];
	V_StripExtension( szMapName, szScriptFile, sizeof( szScriptFile ) );
	V_strncat( szScriptFile, "_items_game.txt", sizeof( szScriptFile ) );

	const char *szGlobalScriptFile = "maps/custom_items_game.txt";

	// Always load the default Item Schema.
	LoadDefaultItemSchema();

	// Search the 'GAME' Path ID for this file.
	if ( g_pFullFileSystem->FileExists( szScriptFile, "GAME" ) || g_pFullFileSystem->FileExists( szGlobalScriptFile, "GAME" ) || bAlwaysReload )
	{
		DevMsg( "[%s] Loading map specific Item Schema... \n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT" );
	
		if ( !GetItemSchema()->ParseItemSchema( szScriptFile ) )
		{
			DevWarning( "[%s] Failed to load map specific Item Schema!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT" );

			if ( !GetItemSchema()->ParseItemSchema( szGlobalScriptFile ) )
			{
				DevWarning( "[%s] Failed to load custom Item Schema!\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT" );
			}
			else
			{
				m_bOverriddenSchema = true;
			}
		}
		else
		{
			m_bOverriddenSchema = true;
		}
	
		if ( !bSkipInitialization )
		{
			// Re-initialize the inventory system.
			InitInventorySys();

#ifdef CLIENT_DLL
			// If the item system reloads, also invalidate the loadout panel's layout
			// because the item selector now caches a tinsy bit to prevent hitches.
			GET_MAINMENUPANEL( CTFLoadoutPanel )->InvalidateLayout( true, true );
#endif
		}
	}

	if ( !bSkipInitialization )
	{
		GetItemSchema()->Precache();
	}
}


void CTFInventory::LevelShutdownPostEntity( void )
{
	if ( m_bOverriddenSchema )
	{
		// Always load the default Item Schema.
		ResetToDefaultItemSchema();

#ifdef CLIENT_DLL
		// Same as the comment found above.
		GET_MAINMENUPANEL( CTFLoadoutPanel )->InvalidateLayout( true, true );
#endif
	}
}


CEconItemView *CTFInventory::GetItem( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	if ( !CheckValidItem( iClass, iSlot, iPreset ) )
		return NULL;

	return m_Items[iClass][iSlot][iPreset];
}


bool CTFInventory::CheckValidSlot( int iClass, ETFLoadoutSlot iSlot )
{
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_CLASS_COUNT_ALL )
		return false;

	// Array bounds check.
	if ( iSlot >= TF_LOADOUT_SLOT_COUNT || iSlot < 0 )
		return false;

	// Slot must contain a base item.
	if ( !m_Items[iClass][iSlot][0] )
		return false;

	return true;
}


bool CTFInventory::CheckValidItem( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_CLASS_COUNT_ALL )
		return false;

	// Array bounds check.
	// FIXME BUGBUG WHATEVER - This randomly fails for legit clients??
	// Shouldn't this be
	// if ( iPreset > m_Items[iClass][iSlot].Count() || iPreset < 0 )
	if ( iPreset >= m_Items[iClass][iSlot].Count() || iPreset < 0 )
		return false;

	// Don't allow switching if this class has no weapon at this position.
	if ( !m_Items[iClass][iSlot][iPreset] )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFInventory::CheckValidItemLoose( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_CLASS_COUNT_ALL )
		return false;

	// breaks if client is just coming from a custom item schema server
	// if ( iPreset > m_Items[iClass][iSlot].Count() || iPreset < 0 )
	// 	return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFInventory::GetNumItems( int iClass, ETFLoadoutSlot iSlot )
{
	// Slot must contain a base item.
	if ( !m_Items[iClass][iSlot][0] )
		return 0;

	return m_Items[iClass][iSlot].Count();
}

#if defined( CLIENT_DLL )

void CTFInventory::LoadInventory()
{
	bool bExist = filesystem->FileExists( TF_INVENTORY_FILE, "MOD" );
	if ( bExist )
	{
		if ( m_pInventory )
		{
			m_pInventory->deleteThis();
		}
		
		m_pInventory = new KeyValues( "Inventory" );
		m_pInventory->LoadFromFile( filesystem, TF_INVENTORY_FILE, "MOD" );
	}
	else
	{
		ResetInventory();
	}
}


void CTFInventory::SaveInventory()
{
	m_pInventory->SaveToFile( filesystem, TF_INVENTORY_FILE, "MOD" );
}

//-----------------------------------------------------------------------------
// Purpose: Create a default inventory file.
//-----------------------------------------------------------------------------
void CTFInventory::ResetInventory()
{
	if ( m_pInventory )
	{
		m_pInventory->deleteThis();
	}

	m_pInventory = new KeyValues( "Inventory" );

	for ( int i = TF_CLASS_UNDEFINED; i < TF_CLASS_COUNT_ALL; i++ )
	{
		KeyValues *pClassInv = new KeyValues( g_aPlayerClassNames_NonLocalized[i] );
		for ( int j = 0; j < TF_LOADOUT_SLOT_COUNT; j++ )
		{
			pClassInv->SetInt( g_LoadoutSlots[j], 0 );
		}

		m_pInventory->AddSubKey( pClassInv );
	}

#ifdef ITEM_ACKNOWLEDGEMENTS
	m_pInventory->SetString( "acknowledged_items", "0" );
#endif

	SaveInventory();
}
#if 0

bool CTFInventory::GetAllItemPresets( KeyValues *pKV )
{
	pKV->Clear();

	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; ++i )
	{
		KeyValues *pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[i] );
		if ( !pClass )
		{
			return false;
		}

		pKV->AddSubKey( pClass->MakeCopy() );
	}

	return true;
}
#endif

int CTFInventory::GetItemPreset( int iClass, ETFLoadoutSlot iSlot )
{
	KeyValues *pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	if ( !pClass )
	{
		// Cannot find class node.
		ResetInventory();
		return 0;
	}

	int iPreset = pClass->GetInt( g_LoadoutSlots[iSlot], -1 );
	if ( iPreset == -1 )
	{
		// Cannot find slot node.
		ResetInventory();
		return 0;
	}

	if ( !CheckValidItem( iClass, iSlot, iPreset ) )
		return 0;

	return iPreset;
}


void CTFInventory::SetItemPreset( int iClass, ETFLoadoutSlot iSlot, int iPreset )
{
	KeyValues *pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	if ( !pClass )
	{
		// Cannot find class node
		ResetInventory();
		pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	}

	pClass->SetInt( GetSlotName( iSlot ), iPreset );
	SaveInventory();
}


const char *CTFInventory::GetSlotName( ETFLoadoutSlot iSlot )
{
	return g_LoadoutSlots[iSlot];
}

#ifdef ITEM_ACKNOWLEDGEMENTS

bool CTFInventory::ItemIsAcknowledged( int iItemID )
{
	char szAcknowledgedItems[4096];
	V_strcpy( szAcknowledgedItems, m_pInventory->GetString( "acknowledged_items" ) );

	if ( szAcknowledgedItems[0] != '\0' )
	{
		char *szItemIDToken = strtok( szAcknowledgedItems, "," );
		while ( szItemIDToken != NULL )
		{
			if ( iItemID == atoi( szItemIDToken ) )
				return true;

			szItemIDToken = strtok( NULL, "," );
		}
	}
	else
	{
		m_pInventory->SetString( "acknowledged_items", "0" );
	}

	return false;
}


bool CTFInventory::MarkItemAsAcknowledged( int iItemID )
{
	if ( ItemIsAcknowledged( iItemID ) )
		return false;

	const char *pszAcknowledgedItems = m_pInventory->GetString( "acknowledged_items", "0" );
	if ( pszAcknowledgedItems )
	{
		char szNewAcknowledgedItems[4096];
		V_strcpy( szNewAcknowledgedItems, pszAcknowledgedItems );
		V_strcat( szNewAcknowledgedItems, ",", sizeof( szNewAcknowledgedItems ) );

		// '8' should be enough.
		char szItemID[8];
		V_snprintf( szItemID, sizeof( szItemID ), "%d", iItemID );
		V_strcat( szNewAcknowledgedItems, szItemID, sizeof( szNewAcknowledgedItems ) );

		m_pInventory->SetString( "acknowledged_items", szNewAcknowledgedItems );
		SaveInventory();

		return true;
	}
	else
	{
		m_pInventory->SetString( "acknowledged_items", "0" );
	}

	return false;
}
#endif
#endif

//-----------------------------------------------------------------------------
// Purpose: Reload the inventory
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CON_COMMAND_F( cl_reload_item_schema, "Reloads the Item Schema", FCVAR_CLIENTDLL )
#else
CON_COMMAND_F( sv_reload_item_schema, "Reloads the Item Schema", FCVAR_GAMEDLL )
#endif
{
#ifdef CLIENT_DLL
	engine->ClientCmd_Unrestricted( "cmd sv_reload_item_schema" );
#endif

	DevMsg( "[%s] Reloading the Item Schema...\n", CBaseEntity::IsServer() ? "SERVER" : "CLIENT" );

#ifdef CLIENT_DLL
	if ( engine->IsInGame() )
#endif
	{
		GetTFInventory()->CheckForCustomSchema( true );
	}
#ifdef CLIENT_DLL
	else
	{
		// Always load the default Item Schema.
		ResetToDefaultItemSchema();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Load custom schema
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CON_COMMAND_F(cl_load_custom_item_schema, "Loads a custom item schema", FCVAR_CLIENTDLL)
#else
CON_COMMAND_F(sv_load_custom_item_schema, "Loads a custom item schema", FCVAR_GAMEDLL)
#endif
{
	if (args.ArgC() < 2)
	{
		Warning("No item schema specified!\n");
		return;
	}

	std::string strSchema = fmt::format(FMT_STRING("maps/{:s}"), args[1]);

#ifdef CLIENT_DLL
	std::string strSchemaServer = fmt::format( FMT_STRING("cmd sv_load_custom_item_schema {:s}"), strSchema);
	engine->ClientCmd_Unrestricted( strSchemaServer.c_str() );
#endif

	if ( !g_pFullFileSystem->FileExists(strSchema.c_str(), "GAME"))
	{
		Warning("Item Schema not found!\n");
		return;
	}

	GetTFInventory()->SetOverriddenSchema(false);

	// Always load the default Item Schema.
	LoadDefaultItemSchema();

	if ( GetItemSchema()->ParseItemSchema(strSchema.c_str()) )
	{
		GetTFInventory()->SetOverriddenSchema(true);
	}
	else
	{
		Warning("Failed to parse custom item schema Item Schema!\n");
	}

	GetTFInventory()->InitInventorySys();

#ifdef CLIENT_DLL
	// If the item system reloads, also invalidate the loadout panel's layout
	// because the item selector now caches a tinsy bit to prevent hitches.
	GET_MAINMENUPANEL(CTFLoadoutPanel)->InvalidateLayout(true, true);
#endif

	GetItemSchema()->Precache();
}
