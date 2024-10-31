#include "cbase.h"
#include "attribute_manager.h"
#include "econ_item_schema.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ATTRIB_REAPPLY_PARITY_BITS 3
#define ATTRIB_REAPPLY_PARITY_MASK ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 )


//=============================================================================
// CAttributeManager
//=============================================================================

BEGIN_DATADESC_NO_BASE( CAttributeManager )
	DEFINE_FIELD( m_iReapplyProvisionParity, FIELD_INTEGER ),
	DEFINE_FIELD( m_hOuter, FIELD_EHANDLE ),
	DEFINE_FIELD( m_ProviderType, FIELD_INTEGER ),
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeManager, DT_AttributeManager )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()


template <>
string_t CAttributeManager::AttribHookValue<string_t>( string_t strValue, const char *pszClass, const CBaseEntity *pEntity )
{
	if ( !pEntity )
		return strValue;

	IHasAttributes *pAttribInteface = pEntity->GetHasAttributesInterfacePtr();
	if ( pAttribInteface )
	{
		string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( pszClass );
		strValue = pAttribInteface->GetAttributeManager()->ApplyAttributeStringWrapper( strValue, pEntity, strAttributeClass );
	}

	return strValue;
}

CAttributeManager::CAttributeManager()
{
	m_bParsingMyself = false;
	m_iReapplyProvisionParity = 0;
}

#ifdef CLIENT_DLL

void CAttributeManager::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
}


void CAttributeManager::OnDataChanged( DataUpdateType_t updateType )
{
	// If parity ever falls out of sync we can catch up here.
	if ( m_iReapplyProvisionParity != m_iOldReapplyProvisionParity )
	{
		if ( m_hOuter )
		{
			IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
			if ( pAttributes )
			{
				pAttributes->ReapplyProvision();
			}
			ClearCache();
			m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
		}
	}
}

#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::AddProvider( CBaseEntity *pEntity )
{
	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();

	m_vecAttributeProviders.AddToTail( pEntity );
	pAttributes->GetAttributeManager()->AddReceiver( m_hOuter.Get() );

	ClearCache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::RemoveProvider( CBaseEntity *pEntity )
{
	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();
	Assert( pAttributes );

	m_vecAttributeProviders.FindAndFastRemove( pEntity );
	pAttributes->GetAttributeManager()->RemoveReceiver( m_hOuter.Get() );

	ClearCache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::AddReceiver( CBaseEntity *pEntity )
{
	m_vecAttributeReceivers.AddToTail( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::RemoveReceiver( CBaseEntity *pEntity )
{
	m_vecAttributeReceivers.FindAndFastRemove( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Add this entity to target's providers list.
//-----------------------------------------------------------------------------
void CAttributeManager::ProvideTo( CBaseEntity *pEntity )
{
	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();
	if ( pAttributes )
	{
		pAttributes->GetAttributeManager()->AddProvider( m_hOuter.Get() );
	}

#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
#endif
		m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this entity from target's providers list.
//-----------------------------------------------------------------------------
void CAttributeManager::StopProvidingTo( CBaseEntity *pEntity )
{
	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();
	if ( pAttributes )
	{
		pAttributes->GetAttributeManager()->RemoveProvider( m_hOuter.Get() );
	}

#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
#endif
		m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::ClearCache( void )
{
	if ( m_bParsingMyself )
		return;

	m_CachedAttribs.Purge();

	m_bParsingMyself = true;

	// Tell the things we are providing to that they have been invalidated
	FOR_EACH_VEC( m_vecAttributeReceivers, i )
	{
		IHasAttributes *pAttribInterface = m_vecAttributeReceivers[i].Get() ? m_vecAttributeReceivers[i]->GetHasAttributesInterfacePtr() : NULL;
		if ( pAttribInterface )
		{
			pAttribInterface->GetAttributeManager()->ClearCache();
		}
	}

	// Also our owner
	IHasAttributes *pMyAttribInterface = m_hOuter.Get() ? m_hOuter->GetHasAttributesInterfacePtr() : NULL;
	if ( pMyAttribInterface )
	{
		pMyAttribInterface->GetAttributeManager()->ClearCache();
	}

	m_bParsingMyself = false;

#ifndef CLIENT_DLL
	m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ATTRIB_REAPPLY_PARITY_MASK;
	NetworkStateChanged();
#endif
}

void CAttributeManager::InitializeAttributes( CBaseEntity *pEntity )
{
	Assert( pEntity->GetHasAttributesInterfacePtr() );
	m_hOuter.Set( pEntity );
	m_bParsingMyself = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloatWrapper( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( pOutProviders == NULL )
	{
		FOR_EACH_VEC_BACK( m_CachedAttribs, i )
		{
			if ( m_CachedAttribs[i].iAttribName == strAttributeClass )
			{
				if ( flValue == m_CachedAttribs[i].in )
					return m_CachedAttribs[i].out;

				// We are looking for another attribute of the same name,
				// remove this cache so we can get a different value
				m_CachedAttribs.Remove( i );
				break;
			}
		}
	}

	// Uncached, loop our attribs now
	float result = ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );

	if ( pOutProviders == NULL )
	{
		// Cache it out
		int nCache = m_CachedAttribs.AddToTail();
		m_CachedAttribs[nCache].iAttribName = strAttributeClass;
		m_CachedAttribs[nCache].in.fVal = flValue;
		m_CachedAttribs[nCache].out.fVal = result;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CAttributeManager::ApplyAttributeStringWrapper( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( pOutProviders == NULL )
	{
		FOR_EACH_VEC_BACK( m_CachedAttribs, i )
		{
			if ( m_CachedAttribs[i].iAttribName == strAttributeClass )
			{
				if ( strValue == m_CachedAttribs[i].in )
					return m_CachedAttribs[i].out;

				// We are looking for another attribute of the same name,
				// remove this cache so we can get a different value
				m_CachedAttribs.Remove( i );
				break;
			}
		}
	}

	// Uncached, loop our attribs now
	string_t result = ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );

	if ( pOutProviders == NULL )
	{
		// Cache it out
		int nCache = m_CachedAttribs.AddToTail();
		m_CachedAttribs[nCache].iAttribName = strAttributeClass;
		m_CachedAttribs[nCache].in.iVal = strValue;
		m_CachedAttribs[nCache].out.iVal = result;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute on our providers.
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || !m_hOuter.Get() )
		return flValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	IHasAttributes *pMyAttributes = pEntity->GetHasAttributesInterfacePtr();

	for ( int i = 0, c = m_vecAttributeProviders.Count(); i < c; i++ )
	{
		CBaseEntity *pProvider = m_vecAttributeProviders[i].Get();
		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = pProvider->GetHasAttributesInterfacePtr();
		if ( pAttributes )
		{
			// Weapons can't provide to eachother
			if ( pAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
				 pMyAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
			{
				continue;
			}

			flValue = pAttributes->GetAttributeManager()->ApplyAttributeFloat( flValue, pEntity, strAttributeClass );
		}
	}

	IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
	CBaseEntity *pProvider = pAttributes->GetAttributeOwner();
	if ( pProvider )
	{
		pAttributes = pProvider->GetHasAttributesInterfacePtr();
		if ( pAttributes )
		{
			flValue = pAttributes->GetAttributeManager()->ApplyAttributeFloat( flValue, pEntity, strAttributeClass );
		}
	}

	m_bParsingMyself = false;

	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute on our providers.
//-----------------------------------------------------------------------------
string_t CAttributeManager::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || !m_hOuter.Get() )
		return strValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	IHasAttributes *pMyAttributes = pEntity->GetHasAttributesInterfacePtr();

	for ( int i = 0, c = m_vecAttributeProviders.Count(); i < c; i++ )
	{
		CBaseEntity *pProvider = m_vecAttributeProviders[i].Get();
		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = pProvider->GetHasAttributesInterfacePtr();
		if ( pAttributes )
		{
			// Weapons can't provide to eachother
			if ( pAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
				 pMyAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
			{
				continue;
			}

			strValue = pAttributes->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass );
		}
	}

	IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
	CBaseEntity *pProvider = pAttributes->GetAttributeOwner();
	if ( pProvider )
	{
		pAttributes = pProvider->GetHasAttributesInterfacePtr();
		if ( pAttributes )
		{
			strValue = pAttributes->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass );
		}
	}

	m_bParsingMyself = false;

	return strValue;
}


//=============================================================================
// CAttributeContainer
//=============================================================================

#if defined( CLIENT_DLL )
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#else
EXTERN_SEND_TABLE( DT_ScriptCreatedItem );
#endif

BEGIN_DATADESC( CAttributeContainer )
	DEFINE_EMBEDDED( m_Item ),
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainer, DT_AttributeContainer )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
	RecvPropDataTable( RECVINFO_DT( m_Item ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( m_Item ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
#endif
END_NETWORK_TABLE();

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA_NO_BASE( CAttributeContainer )
	DEFINE_PRED_FIELD( m_iReapplyProvisionParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

CAttributeContainer::CAttributeContainer()
{

}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute and apply its value.
//-----------------------------------------------------------------------------
float CAttributeContainer::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || !m_hOuter.Get() )
		return flValue;

	m_bParsingMyself = true;

	// This should only ever be used by econ entities.
	CEconItemAttribute const *pAttribute = m_Item.IterateAttributes( strAttributeClass );
	if ( pAttribute )
	{
		CEconAttributeDefinition *pStatic = pAttribute->GetStaticData();
		switch ( pStatic->description_format )
		{
			case ATTRIB_FORMAT_ADDITIVE:
			case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
				flValue += pAttribute->value;
				break;
			case ATTRIB_FORMAT_PERCENTAGE:
			case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
				flValue *= pAttribute->value;
				break;
			case ATTRIB_FORMAT_OR:
			{
				// Oh, man...
				int iValue = (int)flValue;
				int iAttrib = (int)pAttribute->value;
				iValue |= iAttrib;
				flValue = (float)iValue;
				break;
			}
		}
	}

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeFloat( flValue, pEntity, strAttributeClass );
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute and apply its value.
//-----------------------------------------------------------------------------
string_t CAttributeContainer::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || !m_hOuter.Get() )
		return strValue;

	m_bParsingMyself = true;

	// This should only ever be used by econ entities.
	CEconItemAttribute const *pAttribute = m_Item.IterateAttributes( strAttributeClass );
	if ( pAttribute )
	{
		strValue = AllocPooledString( pAttribute->value_string.Get() );
	}

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeString( strValue, pEntity, strAttributeClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeContainer::InitializeAttributes( CBaseEntity *pEntity )
{
	BaseClass::InitializeAttributes( pEntity );
	m_Item.GetAttributeList()->SetManager( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeContainer::OnAttributesChanged( void )
{
	BaseClass::OnAttributesChanged();
	m_Item.OnAttributesChanged();
}


BEGIN_DATADESC( CAttributeContainerPlayer )
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainerPlayer, DT_AttributeContainerPlayer )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
#endif
END_NETWORK_TABLE();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeContainerPlayer::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || m_hPlayer.Get() == NULL )
		return flValue;

	m_bParsingMyself = true;

	CEconAttributeDefinition const *pDefinition = GetItemSchema()->GetAttributeDefinitionByClass( STRING( strAttributeClass ) );
	if ( pDefinition )
	{
		CEconItemAttribute const *pAttribute = m_hPlayer->m_AttributeList.GetAttribByID( pDefinition->index );
		if ( pAttribute )
		{
			switch ( pDefinition->description_format )
			{
				case ATTRIB_FORMAT_ADDITIVE:
				case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
					flValue += pAttribute->value;
					break;
				case ATTRIB_FORMAT_PERCENTAGE:
				case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
					flValue *= pAttribute->value;
					break;
				case ATTRIB_FORMAT_OR:
				{
					// Oh, man...
					int iValue = (int)flValue;
					int iAttrib = (int)pAttribute->value;
					iValue |= iAttrib;
					flValue = (float)iValue;
					break;
				}
			}
		}
	}

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CAttributeContainerPlayer::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders )
{
	if ( m_bParsingMyself || m_hPlayer.Get() == NULL )
		return strValue;

	m_bParsingMyself = true;

	CEconAttributeDefinition const *pDefinition = GetItemSchema()->GetAttributeDefinitionByClass( STRING( strAttributeClass ) );
	if ( pDefinition )
	{
		CEconItemAttribute const *pAttribute = m_hPlayer->m_AttributeList.GetAttribByID( pDefinition->index );
		if ( pAttribute )
		{
			strValue = AllocPooledString( pAttribute->value_string.Get() );
		}
	}

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeContainerPlayer::OnAttributesChanged( void )
{
	BaseClass::OnAttributesChanged();
	if( m_hPlayer )
		m_hPlayer->NetworkStateChanged();
}