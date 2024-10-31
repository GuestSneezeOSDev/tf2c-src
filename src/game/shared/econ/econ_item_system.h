#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"

#include <steam/steam_api.h>
#include <steam/isteamhttp.h>

//#define RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP

class CEconSchemaParser;


class CEconItemSchema
{
	friend class CTFInventory;

public:
	CEconItemSchema();
	~CEconItemSchema();

	void Precache( void );

	void Reset( void );
	bool ParseItemSchema_Internal(KeyValues* pSchemaData);
	bool ParseItemSchema( const char *pszFile );
	bool ParseItemSchema( CUtlBuffer &bufRawData );
#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
	bool ParseRemoteItemSchema( const char *pszAddress );
#endif
	void ParseGameInfo( KeyValues *pKeyValuesData );
	void ParseQualities( KeyValues *pKeyValuesData );
	void ParseColors( KeyValues *pKeyValuesData );
	void ParsePrefabs( KeyValues *pKeyValuesData );
	void ParseItems( KeyValues *pKeyValuesData );
	void ParseAttributes( KeyValues *pKeyValuesData );
	bool ParseVisuals( KeyValues *pData, CEconItemDefinition *pItem, int iIndex );
	bool ParseItemRec( KeyValues *pData, CEconItemDefinition *pItem );

	CEconItemDefinition *GetItemDefinition( int id );
	int GetItemIndex( const char *name );
	CEconAttributeDefinition *GetAttributeDefinition( int id );
	CEconAttributeDefinition *GetAttributeDefinitionByName( const char *name );
	CEconAttributeDefinition *GetAttributeDefinitionByClass( const char *name );
	int GetAttributeIndex( const char *classname );

#ifdef GAME_DLL
#ifdef ITEM_TAUNTING
	void WriteScenes( void );
#endif
#endif

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
	void OnHTTPCompleted( HTTPRequestCompleted_t *arg, bool bFailed );
#endif

private:
	CUtlDict<int, unsigned short>				m_GameInfo;
	CUtlDict<EconQuality, unsigned short>		m_Qualities;
	CUtlDict<EconColor, unsigned short>			m_Colors;
	CUtlDict<KeyValues *, unsigned short>		m_PrefabsValues;
	CUtlMap<int, CEconItemDefinition *>			m_Items;
	CUtlMap<int, CEconAttributeDefinition *>		m_Attributes;

	KeyValues *m_pSchemaData;

#ifdef RETRIEVE_CUSTOM_ITEM_SCHEMA_USING_HTTP
	CCallResult<CEconItemSchema, HTTPRequestCompleted_t> callback;
#endif

#ifdef CLIENT_DLL
public:
	void PrintItemsToConsole();
	void PrintAttributesToConsole();
#endif
};

CEconItemSchema *GetItemSchema();

#endif // ECON_ITEM_SYSTEM_H
