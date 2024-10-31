#include "cbase.h"
#include "econ_item_view.h"
#include "econ_item_system.h"
#include "activitylist.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#else
#include "dt_utlvector_send.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_ATTRIBUTES_SENT 20

#ifdef CLIENT_DLL
BEGIN_RECV_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	RecvPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_SENT, DT_EconItemAttribute )
END_RECV_TABLE()
#else
BEGIN_SEND_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	SendPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_SENT, DT_EconItemAttribute )
END_SEND_TABLE()
#endif

BEGIN_DATADESC_NO_BASE( CAttributeList )
END_DATADESC()

#ifdef CLIENT_DLL
BEGIN_RECV_TABLE_NOBASE( CEconItemView, DT_ScriptCreatedItem )
	RecvPropInt( RECVINFO( m_iItemDefinitionIndex ) ),
	RecvPropBool( RECVINFO( m_bOnlyIterateItemViewAttributes ) ),
	RecvPropDataTable( RECVINFO_DT( m_AttributeList ), 0, &REFERENCE_RECV_TABLE( DT_AttributeList ) ),
END_RECV_TABLE()
#else
BEGIN_SEND_TABLE_NOBASE( CEconItemView, DT_ScriptCreatedItem )
	SendPropInt( SENDINFO( m_iItemDefinitionIndex ) ),
	SendPropBool( SENDINFO( m_bOnlyIterateItemViewAttributes ) ),
	SendPropDataTable( SENDINFO_DT( m_AttributeList ), &REFERENCE_SEND_TABLE( DT_AttributeList ) ),
END_SEND_TABLE()
#endif

BEGIN_DATADESC_NO_BASE( CEconItemView )
	DEFINE_FIELD( m_iItemDefinitionIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityQuality, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityLevel, FIELD_INTEGER ),
	DEFINE_FIELD( m_bOnlyIterateItemViewAttributes, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_AttributeList ),
END_DATADESC()

#define FIND_ELEMENT( map, key, val )								\
		unsigned int index = map.Find( key );						\
		if ( index != map.InvalidIndex() )							\
			val = map.Element( index )				

#define FIND_ELEMENT_STRING( map, key, val )						\
		unsigned int index = map.Find( key );						\
		if ( index != map.InvalidIndex() )							\
			Q_snprintf( val, sizeof( val ), map.Element( index ) )


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView()
{
	Init( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( int iItemID )
{
	Init( iItemID );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( const char *pszName )
{
	Init( pszName );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemView::~CEconItemView()
{
}


bool CEconItemView::Init( int iItemID )
{
	CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemID );
	if ( pItemDef )
	{
		SetItemDefIndex( iItemID );
		m_AttributeList.Init();
		return true;
	}

	Invalidate();
	return false;
}


bool CEconItemView::Init( const char *pszName )
{
	return Init( GetItemSchema()->GetItemIndex( pszName ) );
}

//-----------------------------------------------------------------------------
// Purpose: Get static item definition from schema.
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemView::GetStaticData( void ) const
{
	return GetItemSchema()->GetItemDefinition( m_iItemDefinitionIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Get world model.
//-----------------------------------------------------------------------------
const char *CEconItemView::GetWorldDisplayModel( int iClass/* = 0*/ ) const
{
	const char *pszModelName = NULL;

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		if ( pStatic->IsAWearable() )
		{
			pszModelName = GetPlayerDisplayModel( iClass );
		}
		else
		{
			pszModelName = pStatic->model_world;

			if ( pStatic->model_world_per_class[iClass][0] )
				pszModelName = pStatic->model_world_per_class[iClass];

			// Assuming we're using same model for both 1st person and 3rd person view.
			if ( !pszModelName[0] && pStatic->attach_to_hands == 1 )
			{
				pszModelName = GetPlayerDisplayModel( iClass );
			}
		}
	}

	return pszModelName;
}

//-----------------------------------------------------------------------------
// Purpose: Get view model.
//-----------------------------------------------------------------------------
const char *CEconItemView::GetPlayerDisplayModel( int iClass/* = 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		if ( pStatic->model_player_per_class[iClass][0] )
			return pStatic->model_player_per_class[iClass];

		return pStatic->model_player;
	}

	return NULL;
}


const char *CEconItemView::GetExtraWearableModel() const
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return NULL;

	return pData->extra_wearable;
}

bool CEconItemView::GetExtraWearableModelVisibilityRules()
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return false;

	return pData->extra_wearable_hide_on_active;
}


const char *CEconItemView::GetEntityName()
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
		return pStatic->item_class;

	return NULL;
}


bool CEconItemView::IsCosmetic()
{
	bool bRet = false;

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->tags, "is_cosmetic", bRet );
	}

	return bRet;
}


ETFLoadoutSlot CEconItemView::GetLoadoutSlot( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
		return pStatic->GetLoadoutSlot( iClass );

	return TF_LOADOUT_SLOT_INVALID;
}


ETFWeaponType CEconItemView::GetAnimationSlot( void )
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
		return pStatic->anim_slot;

	return TF_WPN_TYPE_INVALID;
}


Activity CEconItemView::GetActivityOverride( int iTeamNumber, Activity actOriginalActivity )
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		int iOverridenActivity = ACT_INVALID;

		EconItemVisuals *pVisuals = pStatic->GetVisuals( iTeamNumber );
		FIND_ELEMENT( pVisuals->animation_replacement, actOriginalActivity, iOverridenActivity );

		if ( iOverridenActivity != ACT_INVALID )
			return (Activity)iOverridenActivity;
	}

	return actOriginalActivity;
}


const char *CEconItemView::GetActivityOverride( int iTeamNumber, const char *name )
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		int iOriginalAct = ActivityList_IndexForName( name );
		int iOverridenAct = ACT_INVALID;
		EconItemVisuals *pVisuals = pStatic->GetVisuals( iTeamNumber );

		FIND_ELEMENT( pVisuals->animation_replacement, iOriginalAct, iOverridenAct );

		if ( iOverridenAct != ACT_INVALID )
			return ActivityList_NameForIndex( iOverridenAct );
	}

	return name;
}


const char *CEconItemView::GetSoundOverride( int iIndex, int iTeamNum /*= 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		EconItemVisuals *pVisuals = pStatic->GetVisuals( iTeamNum );
		return pVisuals->sound_weapons[iIndex];
	}

	return NULL;
}


bool CEconItemView::HasCapability( const char* name )
{
	bool bRet = false;

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->capabilities, name, bRet );
	}

	return bRet;
}


bool CEconItemView::HasTag( const char* name )
{
	bool bRet = false;

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->tags, name, bRet );
	}

	return bRet;
}


bool CEconItemView::AddAttribute( CEconItemAttribute *pAttribute )
{
	// Make sure this attribute exists.
	CEconAttributeDefinition *pAttribDef = pAttribute->GetStaticData();
	if ( pAttribDef )
	{
		m_AttributeList.SetRuntimeAttributeValue( pAttribDef, pAttribute->value );
		return true;
	}

	return false;
}

void CEconItemView::RemoveAttribute( CEconAttributeDefinition const *pAttribute )
{
	m_AttributeList.RemoveAttribute( pAttribute );
}


void CEconItemView::SkipBaseAttributes( bool bSkip )
{
	m_bOnlyIterateItemViewAttributes = bSkip;
}

//-----------------------------------------------------------------------------
// Purpose: Find an attribute with the specified class.
//-----------------------------------------------------------------------------
CEconItemAttribute const *CEconItemView::IterateAttributes( string_t strClass )
{
	// Returning the first attribute found, 
	// this is not how Live TF2 does this but this will do for now.
	CEconAttributeDefinition const *pDefinition = GetItemSchema()->GetAttributeDefinitionByClass( STRING( strClass ) );
	if ( pDefinition )
	{
		CEconItemAttribute const *pAttribute = m_AttributeList.GetAttribByID( pDefinition->index );
		if ( pAttribute )
			return pAttribute;
	}

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic && !m_bOnlyIterateItemViewAttributes )
		return pStatic->IterateAttributes( strClass );

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAttributeList::CAttributeList()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::Init( void )
{
	m_Attributes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemAttribute const *CAttributeList::GetAttribByID( int iNum )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == iNum )
			return &m_Attributes[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemAttribute const *CAttributeList::GetAttribByName( char const *szName )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );

	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == pDefinition->index )
			return &m_Attributes[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::IterateAttributes( IEconAttributeIterator *iter ) const
{
	/*FOR_EACH_VEC( m_Attributes, i )
	{
		CEconAttributeDefinition const *pDefinition = m_Attributes[i].GetStaticData();
		if ( pDefinition == nullptr )
			continue;

		attrib_data_union_t value;
		value.flVal = m_Attributes[i].value;
		if ( !pDefinition->type->OnIterateAttributeValue( iter, pDefinition, value ) )
			break;
	}*/
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::SetRuntimeAttributeValue( const CEconAttributeDefinition *pDefinition, float flValue )
{
	Assert( pDefinition );
	if ( pDefinition == nullptr )
		return false;

	FOR_EACH_VEC( m_Attributes, i )
	{
		CEconItemAttribute *pAttrib = &m_Attributes[i];
		if ( pAttrib->GetStaticData() == pDefinition )
		{
			pAttrib->value = flValue;
			m_pManager->OnAttributesChanged();
			return true;
		}
	}

	CEconItemAttribute attrib( pDefinition->index, flValue );
	m_Attributes[ m_Attributes.AddToTail() ] = attrib;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::RemoveAttribute( const CEconAttributeDefinition *pDefinition )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData() == pDefinition )
		{
			m_Attributes.Remove( i );
			m_pManager->OnAttributesChanged();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::RemoveAttribByIndex( int iIndex )
{
	if( iIndex == m_Attributes.InvalidIndex() || iIndex > m_Attributes.Count() )
		return false;

	m_Attributes.Remove( iIndex );
	m_pManager->OnAttributesChanged();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Clear out dynamic attributes
//-----------------------------------------------------------------------------
void CAttributeList::RemoveAllAttributes( void )
{
	if( !m_Attributes.IsEmpty() )
	{
		m_Attributes.Purge();
		m_pManager->OnAttributesChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::SetRuntimeAttributeRefundableCurrency( CEconAttributeDefinition const *pAttrib, int iRefundableCurrency )
{
	for ( int i = 0; i < m_Attributes.Count(); i++ )
	{
		CEconItemAttribute *pAttribute = &m_Attributes[i];

		if ( pAttribute->m_iAttributeDefinitionIndex == pAttrib->index )
		{
			// Found existing attribute -- change value.
			pAttribute->m_nRefundableCurrency = iRefundableCurrency;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAttributeList::GetRuntimeAttributeRefundableCurrency( CEconAttributeDefinition const *pAttrib ) const
{
	for ( int i = 0; i < m_Attributes.Count(); i++ )
	{
		CEconItemAttribute const &pAttribute = m_Attributes[i];

		if ( pAttribute.m_iAttributeDefinitionIndex == pAttrib->index )
			return pAttribute.m_nRefundableCurrency;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::SetManager( CAttributeManager *pManager )
{
	m_pManager = pManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::NotifyManagerOfAttributeValueChanges( void )
{
	m_pManager->OnAttributesChanged();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::operator=( CAttributeList const &rhs )
{
	m_Attributes = rhs.m_Attributes;

	m_pManager = nullptr;
}