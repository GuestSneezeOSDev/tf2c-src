#ifndef ECON_ITEM_VIEW_H
#define ECON_ITEM_VIEW_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"

class CAttributeManager;
class IEconAttributeIterator;

class CEconItemHandle
{
public:
	// CEconItem *m_pItem; TODO: not implemented yet, get a GC going

	uint64 m_SteamID; // The owner of the item?
	int m_ItemID;
};

class CAttributeList
{
	DECLARE_CLASS_NOBASE( CAttributeList );
	DECLARE_EMBEDDED_NETWORKVAR();
public:
	DECLARE_DATADESC();
	CAttributeList();

	void Init( void );

	CEconItemAttribute const *GetAttribByID( int iNum );
	CEconItemAttribute const *GetAttribByName( char const *szName );
	void IterateAttributes( IEconAttributeIterator *iter ) const;

	bool SetRuntimeAttributeValue( const CEconAttributeDefinition *pAttrib, float flValue );
	bool RemoveAttribute( const CEconAttributeDefinition *pAttrib );
	bool RemoveAttribByIndex( int iIndex );
	void RemoveAllAttributes( void );

	void SetRuntimeAttributeRefundableCurrency( const CEconAttributeDefinition *pAttrib, int nRefundableCurrency );
	int	GetRuntimeAttributeRefundableCurrency( const CEconAttributeDefinition *pAttrib ) const;

	void SetManager( CAttributeManager *pManager );
	void NotifyManagerOfAttributeValueChanges( void );

	void operator=( CAttributeList const &rhs );
	
private:
	CUtlVector<CEconItemAttribute> m_Attributes;
	CAttributeManager *m_pManager;
};


#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ScriptCreatedItem )
#else
EXTERN_SEND_TABLE( DT_ScriptCreatedItem )
#endif

class CEconItemView
{
public:
	DECLARE_CLASS_NOBASE( CEconItemView );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();
	CEconItemView();
	CEconItemView( int iItemID );
	CEconItemView( const char *pszName );
	virtual ~CEconItemView();

	bool Init( int iItemID );
	bool Init( const char *pszName );

	CEconItemDefinition *GetStaticData( void ) const;

	const char *GetWorldDisplayModel( int iClass = 0 ) const;
	const char *GetPlayerDisplayModel( int iClass = 0 ) const;
	const char *GetExtraWearableModel() const;
	bool GetExtraWearableModelVisibilityRules();
	const char *GetEntityName( void );
	bool IsCosmetic( void );
	ETFLoadoutSlot GetLoadoutSlot( int iClass = TF_CLASS_UNDEFINED );
	ETFWeaponType GetAnimationSlot( void );
	Activity GetActivityOverride( int iTeamNumber, Activity actOriginalActivity );
	const char *GetActivityOverride( int iTeamNumber, const char *name );
	const char *GetSoundOverride( int iIndex, int iTeamNum = TEAM_UNASSIGNED ) const;

	bool HasCapability( const char *name );
	bool HasTag( const char *name );
	int GetClassIndex( void ) { return m_iClass; }
	void SetClassIndex( int iClass ) { m_iClass = iClass; }

	bool AddAttribute( CEconItemAttribute *pAttribute );
	void RemoveAttribute( CEconAttributeDefinition const *pAttribute );
	void SkipBaseAttributes( bool bSkip );
	CEconItemAttribute const *IterateAttributes( string_t strClass );
	CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }
	CAttributeList const *GetAttributeList( void ) const { return &m_AttributeList; }

	void OnAttributesChanged()
	{
		NetworkStateChanged();
	}

	void SetItemDefIndex( int iItemID ) { m_iItemDefinitionIndex = iItemID; }
	int GetItemDefIndex( void ) const { return m_iItemDefinitionIndex; }
	bool IsValid( void ) const { return ( m_iItemDefinitionIndex >= 0 ); }
	void Invalidate( void ) { m_iItemDefinitionIndex = -1; }

private:
	CNetworkVar( short, m_iItemDefinitionIndex );

	CNetworkVar( int, m_iEntityQuality ); // maybe an enum?
	CNetworkVar( int, m_iEntityLevel );

	CNetworkVar( int, m_iItemID );
	CNetworkVar( uint64, m_iAccountID );
	CNetworkVar( int, m_iInventoryPosition );

	CEconItemHandle m_ItemHandle; // The handle to the CEconItem on the GC

	CNetworkVar( int, m_iTeamNumber );
	CNetworkVar( int, m_iClass );
	//bool m_bInitialized; // ?

	//CUtlDict< EconItemAttribute, unsigned short > m_AttributeList;
	CNetworkVar( bool, m_bOnlyIterateItemViewAttributes );

	CNetworkVarEmbedded( CAttributeList, m_AttributeList );
};
#endif // ECON_ITEM_VIEW_H
