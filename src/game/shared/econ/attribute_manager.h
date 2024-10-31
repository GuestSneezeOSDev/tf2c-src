//=============================================================================
//
// Purpose: Applies attributes.
//
//=============================================================================

#ifndef ATTRIBUTE_MANAGER_H
#define ATTRIBUTE_MANAGER_H

#ifdef _WIN32
#pragma once
#endif


#include "gamestringpool.h"

// Client specific.
#ifdef CLIENT_DLL
	EXTERN_RECV_TABLE( DT_AttributeManager );
	EXTERN_RECV_TABLE( DT_AttributeContainer );
	EXTERN_RECV_TABLE( DT_AttributeContainerPlayer );
// Server specific.
#else
	EXTERN_SEND_TABLE( DT_AttributeManager );
	EXTERN_SEND_TABLE( DT_AttributeContainer );
	EXTERN_SEND_TABLE( DT_AttributeContainerPlayer );
#endif

typedef enum
{
	PROVIDER_ANY,
	PROVIDER_WEAPON
} provider_type_t;

class CAttributeManager
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CAttributeManager );
	DECLARE_DATADESC();

	CAttributeManager();

	template<typename type>
	static type AttribHookValue(type value, const char* pszClass, const CBaseEntity* pEntity)
	{
        if (!pEntity)
        {
        	return value;
        }
        IHasAttributes* pAttribInterface = pEntity->GetHasAttributesInterfacePtr();

        if (!pAttribInterface)
        {
            return value;
        }

        // this can crash if horrible things happen with healsoundmgr - see discord
        // -sappho
		CAttributeManager* pAttribMgr = pAttribInterface->GetAttributeManager();

        if ( pAttribInterface && pAttribMgr )
		{
			value = 
			(type)
			(
				pAttribMgr->ApplyAttributeFloatWrapper( (float)value, pEntity, AllocPooledString_StaticConstantStringPointer( pszClass ) )
			);
		}
		return value;
	}

	template<typename type>
	static type AttribHookValue( type value, const char *pszClass, CEconItemView *pItem )
	{
		if ( pItem )
		{
			float flResult = (float)value;

			CEconItemAttribute const *pAttribute = pItem->IterateAttributes( AllocPooledString_StaticConstantStringPointer( pszClass ) );
			if ( pAttribute )
			{
				switch ( pAttribute->GetStaticData()->description_format )
				{
					case ATTRIB_FORMAT_ADDITIVE:
					case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
						flResult += pAttribute->value;
						break;
					case ATTRIB_FORMAT_PERCENTAGE:
					case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
						flResult *= pAttribute->value;
						break;
					case ATTRIB_FORMAT_OR:
					{
						// Oh, man...
						int iValue = (int)flResult;
						int iAttrib = (int)pAttribute->value;
						iValue |= iAttrib;
						flResult = (float)iValue;
						break;
					}
				}
			}
			
			value = (type)flResult;
		}

		return value;
	}

#ifdef CLIENT_DLL
	virtual void		OnPreDataChanged( DataUpdateType_t updateType );
	virtual void		OnDataChanged( DataUpdateType_t updatetype );
#endif
	void				AddProvider( CBaseEntity *pEntity );
	void				RemoveProvider( CBaseEntity *pEntity );
	void				AddReceiver( CBaseEntity *pEntity );
	void				RemoveReceiver( CBaseEntity *pEntity );
	void				ProvideTo( CBaseEntity *pEntity );
	void				StopProvidingTo( CBaseEntity *pEntity );
	int					GetProviderType( void ) const	{ return m_ProviderType; }
	void				SetProvidrType( int type )		{ m_ProviderType = type; }
	virtual void		InitializeAttributes( CBaseEntity *pEntity );
	virtual float		ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	virtual string_t	ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );

	virtual void		OnAttributesChanged( void )
	{
		ClearCache();
	}

protected:
	CNetworkHandle( CBaseEntity, m_hOuter );
	bool m_bParsingMyself;

	CNetworkVar( int, m_iReapplyProvisionParity );
#ifdef CLIENT_DLL
	int m_iOldReapplyProvisionParity;
#endif

	CNetworkVarForDerived( int, m_ProviderType );

private:
	void				ClearCache();

	virtual float		ApplyAttributeFloatWrapper( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	virtual string_t	ApplyAttributeStringWrapper( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );

	CUtlVector<EHANDLE> m_vecAttributeProviders;
	CUtlVector<EHANDLE> m_vecAttributeReceivers;

	union cached_attribute_value_t
	{
		string_t iVal;
		float fVal;

		operator string_t const &( ) const { return iVal; }
		operator float() const { return fVal; }
	};
	struct cached_attribute_t
	{
		string_t	iAttribName;
		cached_attribute_value_t in;
		cached_attribute_value_t out;
	};
	CUtlVector<cached_attribute_t>	m_CachedAttribs;
};

template <>
string_t CAttributeManager::AttribHookValue<string_t>( string_t strValue, const char *pszClass, const CBaseEntity *pEntity );


class CAttributeContainer : public CAttributeManager
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS( CAttributeContainer, CAttributeManager );
	DECLARE_DATADESC();
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif

	CAttributeContainer();
	
	void		InitializeAttributes( CBaseEntity *pEntity );
	float		ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	string_t	ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	void		OnAttributesChanged( void );

	void SetItem( CEconItemView const &pItem ) { m_Item.CopyFrom( pItem ); }
	CEconItemView *GetItem( void ) { return &m_Item; }
	CEconItemView const *GetItem( void ) const { return &m_Item; }

protected:
	CNetworkVarEmbedded( CEconItemView, m_Item );
};

class CAttributeContainerPlayer : public CAttributeManager
{
public:
	DECLARE_CLASS( CAttributeContainerPlayer, CAttributeManager );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	void	OnAttributesChanged( void );

	CBasePlayer *GetPlayer( void ) const { return m_hPlayer; }
	void	SetPlayer( CBasePlayer *pPlayer ) { m_hPlayer = pPlayer; }

protected:
	CNetworkHandle( CBasePlayer, m_hPlayer );
};

#endif // ATTRIBUTE_MANAGER_H
