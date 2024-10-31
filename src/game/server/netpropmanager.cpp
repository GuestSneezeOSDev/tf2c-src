//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Gets and sets SendTable/DataMap networked properties and caches results.
//
// Code contributions by and used with the permission of L4D2 modders:
// Neil Rao (neilrao42@gmail.com)
// Raymond Nondorf (rayman1103@aol.com)
//==========================================================================================//


#include "cbase.h"
//#include "../../engine/server.h"
#include "netpropmanager.h"
#include "stdstring.h"
#include "activitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern void SendProxy_StringT_To_String( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

//-----------------------------------------------------------------------------
CNetPropManager::~CNetPropManager()
{
	m_PropCache.PurgeAndDeleteElements();
}

SendProp *CNetPropManager::SearchSendTable( SendTable *pSendTable, const char *pszProperty ) const
{
	// Iterate through the send table and find the prop that we are looking for
	for ( int nPropIdx = 0; nPropIdx < pSendTable->GetNumProps(); nPropIdx++ )
	{
		SendProp *pSendProp = pSendTable->GetProp( nPropIdx );
		const char *pszPropName = pSendProp->GetName();

		// If we found the property, return the prop
		if ( pszPropName && V_strcmp( pszPropName, pszProperty ) == 0 )
			return pSendProp;

		// Search nested tables
		SendTable *pInternalSendTable = pSendProp->GetDataTable();
		if ( pInternalSendTable )
		{
			pSendProp = SearchSendTable( pInternalSendTable, pszProperty );
			if ( pSendProp )
				return pSendProp;
		}
	}

	return NULL;
}

inline typedescription_t *CNetPropManager::SearchDataMap( datamap_t *pMap, const char *pszProperty ) const
{
	while ( pMap )
	{
		for ( int field = 0; field < pMap->dataNumFields; field++ )
		{
			const char *fieldName = pMap->dataDesc[field].fieldName;
			if ( !fieldName )
				continue;
			
			if ( V_strcmp( pszProperty, fieldName ) == 0 )
				return &pMap->dataDesc[field];
			
			if ( pMap->dataDesc[field].td )
			{
				typedescription_t *td = SearchDataMap( pMap->dataDesc[field].td, pszProperty );
				if ( td )
					return td;
			}
		}

		pMap = pMap->baseMap;
	}
	
	return NULL; 
}

inline CNetPropManager::PropInfo_t CNetPropManager::GetEntityPropInfo( CBaseEntity* pBaseEntity, const char *pszProperty, int element )
{
	ServerClass *pServerClass       = pBaseEntity->GetServerClass();
	const char  *pszServerClassName = pServerClass->GetName();
	SendTable   *pSendTable         = pServerClass->m_pTable;
	datamap_t   *pDataMap           = pBaseEntity->GetDataDescMap();

	// First, search the cache and see if the property was looked up before
	int classIdx = m_PropCache.Find( pszServerClassName );
	if ( m_PropCache.IsValidIndex( classIdx ) )
	{
		const PropInfoDict_t &properties = *(m_PropCache[ classIdx ]);
		int propIdx = properties.Find( pszProperty );
		if ( element > 0 )
		{
			char pProperty[256];
			V_snprintf( pProperty, sizeof(pProperty), "%s%d", pszProperty, element );
			propIdx = properties.Find( pProperty );
		}
		if ( properties.IsValidIndex( propIdx ) )
			return properties[ propIdx ];
	}

	CUtlStringList szPropertyList;
	char pProperty[256];
	int offset = 0;
	
	typedescription_t *pTypeDesc = NULL;
	SendProp *pSendProp = NULL;

	V_SplitString( pszProperty, ".", szPropertyList );
	
	int nPropertyCount = szPropertyList.Count();
	if ( nPropertyCount )
	{
		// Search the SendTable for the prop, and if not found, search the datamap
		int iProperty = 0;
		int nRootStringLength = 0;

		while ( iProperty < nPropertyCount )
		{
			char *pszSearchProperty = szPropertyList[ iProperty ];

			pSendProp = SearchSendTable( pSendTable, pszSearchProperty );
			if ( !pSendProp )
			{
				// Try the full string remainder as a single property name
				const char *pszPropertyRemainder = pszProperty + nRootStringLength;
				pSendProp = SearchSendTable( pSendTable, pszPropertyRemainder );
				if ( pSendProp )
					offset += pSendProp->GetOffset();
				break;
			}

			// Handle nested properties
			++iProperty;
			offset += pSendProp->GetOffset();
			nRootStringLength += V_strlen( pszSearchProperty ) + iProperty;
		}

		if ( !pSendProp )
		{
			offset = 0;
			iProperty = 0;
			nRootStringLength = 0;

			while ( iProperty < nPropertyCount )
			{
				char *pszSearchProperty = szPropertyList[ iProperty ];

				pTypeDesc = SearchDataMap( pDataMap, pszSearchProperty );
				if ( !pTypeDesc )
				{
					// Try the full string remainder as a single property name
					const char *pszPropertyRemainder = pszProperty + nRootStringLength;
					pTypeDesc = SearchDataMap( pDataMap, pszPropertyRemainder );
					if ( pTypeDesc )
						offset += pTypeDesc->fieldOffset[0];
					break;
				}

				// handle nested properties
				++iProperty;
				offset += pTypeDesc->fieldOffset[0];
				nRootStringLength += V_strlen( pszSearchProperty ) + iProperty;
			}
		}
	}

	PropInfo_t  propInfo;
	if ( pSendProp )
	{
		if ( element < 0 )
		{
			propInfo.m_eType = Type_InvalidOrMax;
			propInfo.m_IsPropValid = false;
			return propInfo;
		}

		propInfo.m_nOffset	= offset;
		propInfo.m_nProps	= 0;
			
		if ( (NetPropType)pSendProp->GetType() == Type_DataTable )
		{
			SendTable pArrayTable = *pSendProp->GetDataTable();
			propInfo.m_nProps = pArrayTable.GetNumProps();

			if ( element >= pArrayTable.GetNumProps() )
			{
				propInfo.m_eType = Type_InvalidOrMax;
				propInfo.m_IsPropValid = false;
				return propInfo;
			}
			pSendProp = pArrayTable.GetProp( element );
			propInfo.m_nOffset += pSendProp->GetOffset();
		}

		NetPropType ePropType = (NetPropType)pSendProp->GetType();
		if ( ePropType == Type_String )
		{
			if ( pSendProp->GetProxyFn() != NULL )
			{
				Assert( pSendProp->GetProxyFn() == &SendProxy_StringT_To_String );
				ePropType = Type_String_t;
			}
		}
		propInfo.m_bIsSendProp = true;
		propInfo.m_eType       = ePropType;
		propInfo.m_nBitCount   = pSendProp->m_nBits;
		propInfo.m_nElements   = pSendProp->GetNumElements();
		propInfo.m_nTransFlags = pSendProp->GetFlags();
		propInfo.m_IsPropValid = true;

		if ( propInfo.m_eType == Type_String )
			propInfo.m_nPropLen = DT_MAX_STRING_BUFFERSIZE;
	}
	else if ( pTypeDesc && pTypeDesc->fieldSizeInBytes > 0 )
	{
		if ( element < 0 || element >= pTypeDesc->fieldSize )
		{
			propInfo.m_eType = Type_InvalidOrMax;
			propInfo.m_IsPropValid = false;
			return propInfo;
		}

		propInfo.m_bIsSendProp		= false;
		propInfo.m_IsPropValid		= true;
		propInfo.m_nOffset			= offset + ( element * ( pTypeDesc->fieldSizeInBytes / pTypeDesc->fieldSize ) );
		propInfo.m_nElements		= pTypeDesc->fieldSize;
		propInfo.m_nTransFlags		= pTypeDesc->flags;
		propInfo.m_nProps			= propInfo.m_nElements;

		switch (pTypeDesc->fieldType)
		{
		case FIELD_TICK:
		case FIELD_MODELINDEX:
		case FIELD_MATERIALINDEX:
		case FIELD_INTEGER:
		case FIELD_COLOR32:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_Int;
				break;
			}
		case FIELD_VECTOR:
		case FIELD_POSITION_VECTOR:
			{
				propInfo.m_nBitCount = 12;
				propInfo.m_eType = Type_Vector;
				break;
			}
		case FIELD_SHORT:
			{
				propInfo.m_nBitCount = 16;
				propInfo.m_eType = Type_Int;
				break;
			}
		case FIELD_BOOLEAN:
			{
				propInfo.m_nBitCount = 1;
				propInfo.m_eType = Type_Int;
				break;
			}
		case FIELD_CHARACTER:
			{
				if (pTypeDesc->fieldSize == 1)
				{
					propInfo.m_nBitCount = 8;
					propInfo.m_eType = Type_Int;
				}
				else
				{
					propInfo.m_nBitCount = 8 * pTypeDesc->fieldSize;
					propInfo.m_eType = Type_String;
				}

				break;
			}
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			{
				propInfo.m_nBitCount = sizeof(string_t);
				propInfo.m_eType = Type_String_t;
				break;
			}
		case FIELD_FLOAT:
		case FIELD_TIME:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_Float;
				break;
			}
		case FIELD_EHANDLE:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_Int;
				break;
			}
		default:
			{
				propInfo.m_IsPropValid = false;
				propInfo.m_eType = Type_InvalidOrMax;
			}
		}

		propInfo.m_nPropLen = pTypeDesc->fieldSize;
	}
	else
	{
		propInfo.m_eType = Type_InvalidOrMax;
		propInfo.m_IsPropValid = false;
		return propInfo;
	}

	// Cache the property
 	if ( !m_PropCache.IsValidIndex( classIdx ) )
	{
		classIdx = m_PropCache.Insert( pszServerClassName, new PropInfoDict_t );
	}
	PropInfoDict_t &properties = *(m_PropCache[ classIdx ]);
	if ( element > 0 )
	{
		V_snprintf( pProperty, sizeof( pProperty ), "%s%d", pszProperty, element );
		properties.Insert( pProperty, propInfo );
	}
	else
	{
		properties.Insert( pszProperty, propInfo );
	}

	return propInfo;
}

inline void CNetPropManager::CollectNestedDataMaps( datamap_t *pMap, CBaseEntity *pBaseEntity, int nDepth, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	while ( pMap )
	{
		for ( int i = 0; i < pMap->dataNumFields; ++i )
		{
			typedescription_t td = pMap->dataDesc[i];
			if ( !( td.flags & ( FTYPEDESC_INPUT|FTYPEDESC_OUTPUT|FTYPEDESC_FUNCTIONTABLE ) ) )
			{
				if ( td.td )
				{
					ScriptVariant_t table;
					g_pScriptVM->CreateTable( table );

					CollectNestedDataMaps( td.td, pBaseEntity, td.fieldOffset[TD_OFFSET_NORMAL] + nDepth, table );

					g_pScriptVM->SetValue( hTable, td.fieldName, table );
					g_pScriptVM->ReleaseValue( table );
				}
				else
				{
					if ( td.fieldSize <= 1u )
					{
						StoreDataMapValue( &td, pBaseEntity, nDepth, -1, hTable );
					}
					else
					{
						ScriptVariant_t table;
						g_pScriptVM->CreateTable( table );
						for ( int j = 0; j < td.fieldSize; ++j )
						{
							int nElement = j * ( td.fieldSizeInBytes / td.fieldSize ) + nDepth;
							StoreDataMapValue( &td, pBaseEntity, nElement, j, table );
						}

						g_pScriptVM->SetValue( hTable, td.fieldName, table );
						g_pScriptVM->ReleaseValue( table );
					}
				}
			}
		}

		pMap = pMap->baseMap;
	}
}

inline void CNetPropManager::StoreDataMapValue( typedescription_t *td, CBaseEntity *pBaseEntity, int nOffset, int element, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	const char *pszProperty = td->fieldName;
	if ( element >= 0 )
		pszProperty = DT_ArrayElementNameForIdx( Max( 2048, element ) );

	uint8 *pEntityPropData = (uint8 *)pBaseEntity + td->fieldOffset[TD_OFFSET_NORMAL] + nOffset;
	switch ( td->fieldType )
	{
		case FIELD_FLOAT:
		case FIELD_TIME:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(float *)pEntityPropData );
			break;
		}
		case FIELD_STRING:
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(const char *)pEntityPropData );
			break;
		}
		case FIELD_VECTOR:
		case FIELD_POSITION_VECTOR:
		{
			ScriptVariant_t value( (Vector *)pEntityPropData, true );
			g_pScriptVM->SetValue( hTable, pszProperty, value );
			break;
		}
		case FIELD_INTEGER:
		case FIELD_COLOR32:
		case FIELD_TICK:
		case FIELD_MODELINDEX:
		case FIELD_MATERIALINDEX:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(int32 *)pEntityPropData );
			break;
		}
		case FIELD_BOOLEAN:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(bool *)pEntityPropData );
			break;
		}
		case FIELD_SHORT:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(int16 *)pEntityPropData );
			break;
		}
		case FIELD_CHARACTER:
		{
			if ( td->fieldSize != 1 )
			{
				g_pScriptVM->SetValue( hTable, pszProperty, SCRIPT_VARIANT_NULL );
			}
			else
			{
				g_pScriptVM->SetValue( hTable, pszProperty, *(int8 *)pEntityPropData );
			}
			break;
		}
		case FIELD_CUSTOM:
		{
			if ( !td->pSaveRestoreOps )
				break;

			if ( td->pSaveRestoreOps == GetStdStringDataOps() )
			{
				g_pScriptVM->SetValue( hTable, pszProperty, *(const char *)pEntityPropData );
			}
			else if ( td->pSaveRestoreOps == ActivityDataOps() )
			{
				g_pScriptVM->SetValue( hTable, pszProperty, *(int32 *)pEntityPropData );
			}
			break;
		}
		case FIELD_CLASSPTR:
		{
			CBaseEntity *pEntityProp = (CBaseEntity *)pEntityPropData;
			g_pScriptVM->SetValue( hTable, pszProperty, ToHScript( pEntityProp ) );
			break;
		}
		case FIELD_EHANDLE:
		{
			CBaseEntity *pEntityProp = CBaseEntity::Instance( *(CBaseHandle *)pEntityPropData );
			g_pScriptVM->SetValue( hTable, pszProperty, ToHScript( pEntityProp ) );
			break;
		}
	}
}

inline void CNetPropManager::CollectNestedSendProps( SendTable *pSendTable, CBaseEntity *pBaseEntity, int nDepth, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	for ( int i = 0; i < pSendTable->GetNumProps(); ++i )
	{
		SendProp *pProp = pSendTable->GetProp( i );
		if ( pProp->GetFlags() & SPROP_EXCLUDE )
			continue;


		Assert( static_cast<int>(DPT_Array) == static_cast<int>(Type_Array) );
		const char *pszProperty = pProp->m_pVarName;
		if ( pProp->GetDataTable() )
		{
			if ( !V_strcmp( pszProperty, "baseclass" ) )
				pszProperty = pProp->GetDataTable()->m_pNetTableName;

			ScriptVariant_t table;
			g_pScriptVM->CreateTable( table );
			CollectNestedSendProps( pProp->GetDataTable(), pBaseEntity, pProp->GetOffset() + nDepth, table );

			g_pScriptVM->SetValue( hTable, pszProperty, table );
			g_pScriptVM->ReleaseValue( table );
		}
		else if ( pProp->m_Type == DPT_Array )
		{
			ScriptVariant_t table;
			g_pScriptVM->CreateTable( table );
			for( int iProp = 0; iProp < pProp->m_nElements; ++iProp )
			{
				const int nOffset = iProp * pProp->m_ElementStride;
				StoreSendPropValue( pProp->m_pArrayProp, pBaseEntity, nOffset + nDepth, iProp, table );
			}

			g_pScriptVM->SetValue( hTable, pszProperty, table );
			g_pScriptVM->ReleaseValue( table );
		}
		else
		{
			StoreSendPropValue( pProp, pBaseEntity, nDepth, -1, hTable );
		}
	}
}

inline void CNetPropManager::StoreSendPropValue( SendProp *pProp, CBaseEntity *pBaseEntity, int nOffset, int element, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	const char *pszProperty = pProp->GetName();
	if ( element >= 0 )
	{
		pszProperty = DT_ArrayElementNameForIdx( Max( 2048, element ) );
	}

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	// All sendprops store an offset from the pointer to the base entity to
	// where the prop data actually is; the reason is because the engine needs
	// to relay data very quickly to all the clients, so it works with
	// offsets to make the ordeal faster
	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + pProp->GetOffset() + nOffset;
	bool bUnsigned = pProp->GetFlags() & SPROP_UNSIGNED;

	switch ( pProp->GetType() )
	{
		case Type_Float:
		{
			g_pScriptVM->SetValue( hTable, pszProperty, *(float *)pEntityPropData );
			break;
		}
		case Type_Vector:
		{
			ScriptVariant_t value( (Vector *)pEntityPropData, true );
			g_pScriptVM->SetValue( hTable, pszProperty, value );
			break;
		}
		case Type_String:
		{
			if ( pProp->GetProxyFn() == SendProxy_StringT_To_String )
			{
				g_pScriptVM->SetValue( hTable, pszProperty, STRING( *(string_t *)pEntityPropData ) );
			}
			else
			{
				g_pScriptVM->SetValue( hTable, pszProperty, (const char *)pEntityPropData );
			}
			break;
		}
		case Type_Int:
		{
			int nBits = pProp->m_nBits;
			if ( nBits == NUM_NETWORKED_EHANDLE_BITS )
			{
				CBaseEntity *pPropEntity = CBaseEntity::Instance( *(CBaseHandle *)pEntityPropData );
				if ( pPropEntity )
				{
					g_pScriptVM->SetValue( hTable, pszProperty, ToHScript( pPropEntity ) );
				}
				else
				{
					g_pScriptVM->SetValue( hTable, pszProperty, SCRIPT_VARIANT_NULL );
				}
			}
			else if ( nBits >= 17 )
			{
				g_pScriptVM->SetValue( hTable, pszProperty, *(int32 *)pEntityPropData );
			}
			else if ( nBits >= 9 )
			{
				if ( bUnsigned )
					g_pScriptVM->SetValue( hTable, pszProperty, *(uint16 *)pEntityPropData );
				else
					g_pScriptVM->SetValue( hTable, pszProperty, *(int16 *)pEntityPropData );
			}
			else if ( nBits >= 2 )
			{
				if ( bUnsigned )
					g_pScriptVM->SetValue( hTable, pszProperty, *(uint8 *)pEntityPropData );
				else
					g_pScriptVM->SetValue( hTable, pszProperty, *(int8 *)pEntityPropData );
			}
			else
			{
				g_pScriptVM->SetValue( hTable, pszProperty, *(bool *)pEntityPropData );
			}

			break;
		}
	}
}

int CNetPropManager::GetPropIntArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	// Get the base entity of the specified index
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return -1;

	// Find the requested property info (this will throw if the entity is
	// invalid, which is exactly what we want)
	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	// Property must be valid
	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Int )
		return -1;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return -1;
	}

	// All sendprops store an offset from the pointer to the base entity to
	// where the prop data actually is; the reason is because the engine needs
	// to relay data very quickly to all the clients, so it works with
	// offsets to make the ordeal faster
	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;
	bool bUnsigned = propInfo.m_nTransFlags & SPROP_UNSIGNED;

	// Thanks to SM for figuring out the types to use for bit counts.
	// All we are doing below is looking at how many bits are in the prop.
	// Since some values can be shorts, longs, ints, signed/unsigned,
	// boolean, etc., so we need to decipher exactly what the SendProp info
	// tells us in order to properly retrieve the right number of bytes.
	if (propInfo.m_nBitCount >= 17)
	{
		return *(int32 *)pEntityPropData;
	}
	else if (propInfo.m_nBitCount >= 9)
	{
		if (bUnsigned)
			return *(uint16 *)pEntityPropData;
		else
			return *(int16 *)pEntityPropData;
	}
	else if (propInfo.m_nBitCount >= 2)
	{
		if (bUnsigned)
			return *(uint8 *)pEntityPropData;
		else
			return *(int8 *)pEntityPropData;
	}
	else
	{
		return *(bool *)(pEntityPropData) ? 1 : 0;
	}

	return 0;
}

void CNetPropManager::SetPropIntArray( HSCRIPT hEnt, const char *pszProperty, int value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Int )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;
	bool bUnsigned = propInfo.m_nTransFlags & SPROP_UNSIGNED;

	if (propInfo.m_nBitCount >= 17)
	{
		*(int32 *)pEntityPropData = (int32)value;
	}
	else if (propInfo.m_nBitCount >= 9)
	{
		if (bUnsigned)
			*(uint16 *)pEntityPropData = (uint16)value;
		else
			*(int16 *)pEntityPropData = (int16)value;
	}
	else if (propInfo.m_nBitCount >= 2)
	{
		if (bUnsigned)
			*(uint8 *)pEntityPropData = (uint8)value;
		else
			*(int8 *)pEntityPropData = (int8)value;
	}
	else
	{
		*(bool *)pEntityPropData = value ? true : false;
	}

	// Network the prop change to connected clients (otherwise the network state won't
	// be updated until the engine re-transmits the entire table)
	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

float CNetPropManager::GetPropFloatArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return -1.0f;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Float )
		return -1.0f;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return -1.0f;
	}

	return *(float *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
}

void CNetPropManager::SetPropFloatArray( HSCRIPT hEnt, const char *pszProperty, float value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Float )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	*(float *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset) = value;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

Vector CNetPropManager::GetPropVectorArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	static Vector vAng = Vector(0, 0, 0);
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return vAng;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || (propInfo.m_eType != Type_Vector) )
		return vAng;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return vAng;
	}

	vAng = *(Vector *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	return vAng;
}

void CNetPropManager::SetPropVectorArray( HSCRIPT hEnt, const char *pszProperty, Vector value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Vector )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	Vector *pVec = (Vector *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	pVec->x = value.x;
	pVec->y = value.y;
	pVec->z = value.z;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

HSCRIPT CNetPropManager::GetPropEntityArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return NULL;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Int )
		return NULL;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return NULL;
	}

	CBaseHandle &baseHandle = *(CBaseHandle *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	CBaseEntity *pPropEntity = CBaseEntity::Instance( baseHandle );

	return ToHScript( pPropEntity );
}

void CNetPropManager::SetPropEntityArray( HSCRIPT hEnt, const char *pszProperty, HSCRIPT hPropEnt, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Int )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	CBaseHandle &baseHandle = *(CBaseHandle *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);

	CBaseEntity *pOtherEntity = ToEnt( hPropEnt );
	if ( !pOtherEntity )
	{
		baseHandle.Set( NULL );
	}
	else
	{
		baseHandle.Set( (IHandleEntity *)pOtherEntity );
	}

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

const char *CNetPropManager::GetPropStringArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return "";

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || (propInfo.m_eType != Type_String && propInfo.m_eType != Type_String_t && propInfo.m_eType != Type_Int) )
		return "";

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return "";
	}

	if ( propInfo.m_eType == Type_String_t )
	{
		string_t propString = *(string_t *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
		return (propString == NULL_STRING) ? "" : STRING(propString);
	}
	else
	{
		return (const char *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	}
}

void CNetPropManager::SetPropStringArray( HSCRIPT hEnt, const char *pszProperty, const char *value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || (propInfo.m_eType != Type_String && propInfo.m_eType != Type_String_t) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	if ( propInfo.m_eType == Type_String_t )
	{
		*(string_t *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset) = AllocPooledString(value);
	}
	else
	{
		char* strDest = (char *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
		V_strncpy( strDest, value, propInfo.m_nPropLen );
	}

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

bool CNetPropManager::GetPropBoolArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return false;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Float )
		return false;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return false;
	}

	return *(bool *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
}

void CNetPropManager::SetPropBoolArray( HSCRIPT hEnt, const char *pszProperty, bool value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid || propInfo.m_eType != Type_Int )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	*(bool *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset) = value;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}

bool CNetPropManager::GetPropInfo( HSCRIPT hEnt, const char *pszProperty, int element, HSCRIPT hTable )
{
	CBaseEntity *pEntity = ToEnt( hEnt );
	if ( pEntity == nullptr )
		return false;

	PropInfo_t info = GetEntityPropInfo( pEntity, pszProperty, element );
	if ( !info.m_IsPropValid )
		return false;

	g_pScriptVM->SetValue( hTable, "is_sendprop", info.m_bIsSendProp );
	g_pScriptVM->SetValue( hTable, "type", info.m_eType );
	g_pScriptVM->SetValue( hTable, "bits", info.m_nBitCount );
	g_pScriptVM->SetValue( hTable, "elements", info.m_nElements );
	g_pScriptVM->SetValue( hTable, "offset", info.m_nOffset );
	g_pScriptVM->SetValue( hTable, "length", info.m_nPropLen );
	g_pScriptVM->SetValue( hTable, "array_props", info.m_nProps );
	g_pScriptVM->SetValue( hTable, "flags", info.m_nTransFlags );
	return true;
}

void CNetPropManager::GetTable( HSCRIPT hEnt, int nTableType, HSCRIPT hTable )
{
	CBaseEntity *pEntity = ToEnt( hEnt );
	if ( pEntity == nullptr )
		return;

	if ( hTable )
	{
		if ( nTableType == DT_DATAMAP )
		{
			CollectNestedDataMaps( pEntity->GetDataDescMap(), pEntity, 0, hTable );
		}
		else
		{
			CollectNestedSendProps( pEntity->GetServerClass()->m_pTable, pEntity, 0, hTable );
		}
	}
}
