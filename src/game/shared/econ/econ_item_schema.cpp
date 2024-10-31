#include "cbase.h"
#include "econ_item_schema.h"
#include "econ_item_system.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *EconQuality_GetColorString( EEconItemQuality quality )
{
	if ( quality >= 0 && quality < QUALITY_COUNT )
		return g_szQualityColorStrings[quality];

	return NULL;
}

const char *EconQuality_GetLocalizationString( EEconItemQuality quality )
{
	if ( quality >= 0 && quality < QUALITY_COUNT )
		return g_szQualityLocalizationStrings[quality];

	return NULL;
}

#ifdef CLIENT_DLL
//=============================================================================
// CEconItemAttribute
//=============================================================================
void RecvProxy_AttributeClass( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	RecvProxy_StringToString( pData, pStruct, pOut );

	// Cache received attribute string.
	CEconItemAttribute *pAttrib = (CEconItemAttribute *)pStruct;
	pAttrib->m_strAttributeClass = AllocPooledString( pAttrib->attribute_class );
}
#endif

BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_EconItemAttribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAttributeDefinitionIndex ) ),
	RecvPropFloat( RECVINFO( value ) ),
	RecvPropInt( RECVINFO( m_nRefundableCurrency ) ),
	RecvPropString( RECVINFO( value_string ) ),
	RecvPropString( RECVINFO( attribute_class ), 0, RecvProxy_AttributeClass ),
#else
	SendPropInt( SENDINFO( m_iAttributeDefinitionIndex ) ),
	SendPropFloat( SENDINFO( value ) ),
	SendPropInt( SENDINFO( m_nRefundableCurrency ), -1, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( value_string ) ),
	SendPropString( SENDINFO( attribute_class ) ),
#endif
END_NETWORK_TABLE()



void CEconItemAttribute::Init( int iIndex, float flValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	value = flValue;
	value_string.GetForModify()[0] = '\0';

	if ( pszAttributeClass )
	{
		V_strncpy( attribute_class.GetForModify(), pszAttributeClass, sizeof( attribute_class ) );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			V_strncpy( attribute_class.GetForModify(), pAttribDef->attribute_class, sizeof( attribute_class ) );
		}
	}
}


void CEconItemAttribute::Init( int iIndex, const char *pszValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	value = 0.0f;
	V_strncpy( value_string.GetForModify(), pszValue, sizeof( value_string ) );

	if ( pszAttributeClass )
	{
		V_strncpy( attribute_class.GetForModify(), pszAttributeClass, sizeof( attribute_class ) );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			V_strncpy( attribute_class.GetForModify(), pAttribDef->attribute_class, sizeof( attribute_class ) );
		}
	}
}


CEconAttributeDefinition *CEconItemAttribute::GetStaticData( void ) const
{
	return GetItemSchema()->GetAttributeDefinition( m_iAttributeDefinitionIndex );
}


//=============================================================================
// CEconItemDefinition
//=============================================================================


EconItemVisuals *CEconItemDefinition::GetVisuals( int iTeamNum /*= TEAM_UNASSIGNED*/ )
{
	if ( iTeamNum > LAST_SHARED_TEAM && iTeamNum < TF_TEAM_COUNT )
		return &visual[iTeamNum];

	return &visual[TEAM_UNASSIGNED];
}


int CEconItemDefinition::GetNumAttachedModels( int iTeam )
{
	EconItemVisuals *pVisuals = GetVisuals( iTeam );
	Assert( pVisuals );
	if ( pVisuals )
		return pVisuals->m_AttachedModels.Count();

	return 0;
}


attachedmodel_t *CEconItemDefinition::GetAttachedModelData( int iTeam, int iIdx )
{
	EconItemVisuals *pVisuals = GetVisuals( iTeam );
	Assert( pVisuals );
	if ( iTeam < FIRST_GAME_TEAM || iTeam >= TF_TEAM_YELLOW || !pVisuals )
		return NULL;

	Assert( iIdx < pVisuals->m_AttachedModels.Count() );
	if ( iIdx >= pVisuals->m_AttachedModels.Count() )
		return NULL;

	return &pVisuals->m_AttachedModels[iIdx];
}


ETFLoadoutSlot CEconItemDefinition::GetLoadoutSlot( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	if ( iClass && item_slot_per_class[iClass] != TF_LOADOUT_SLOT_INVALID )
		return item_slot_per_class[iClass];

	return item_slot;
}


bool CEconItemDefinition::IsAWearable( void )
{
	return ( act_as_wearable || item_slot >= TF_FIRST_COSMETIC_SLOT );
}

//-----------------------------------------------------------------------------
// Purpose: Generate item name to show in UI with prefixes, qualities, etc...
//-----------------------------------------------------------------------------
const wchar_t *CEconItemDefinition::GenerateLocalizedFullItemName( void )
{
	static wchar_t wszFullName[256];

	wchar_t wszQuality[128] = L"";
	if ( item_quality == QUALITY_NORMAL || item_quality == QUALITY_UNIQUE )
	{
		// Attach "the" if necessary to unique items.
		if ( propername )
		{
			const wchar_t *pszPrepend = g_pVGuiLocalize->Find( "#TF_Unique_Prepend_Proper" );
			if ( pszPrepend )
			{
				V_wcscpy_safe( wszQuality, pszPrepend );
			}
		}
	}
	else
	{
		// Live TF2 apparently allows multiple qualities per item but eh, we don't need that for now.
		const char *pszLocale = EconQuality_GetLocalizationString( item_quality );
		if ( pszLocale )
		{	
			const wchar_t *pszQuality = g_pVGuiLocalize->Find( pszLocale );
			if ( pszQuality )
			{
				V_wcscpy_safe( wszQuality, pszQuality );
				// Add a space at the end.
				V_wcscat_safe( wszQuality, L" " );
			}
		}	
	}

	// Get base item name.
	wchar_t wszItemName[128];

	const wchar_t *pszLocalizedName = g_pVGuiLocalize->Find( item_name );
	if ( pszLocalizedName )
	{
		V_wcscpy_safe( wszItemName, pszLocalizedName );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( item_name, wszItemName, sizeof( wszItemName ) );
	}

	// Oh boy.
	wchar_t wszCraftNumber[128] = L"";
	wchar_t wszCraftSeries[128] = L"";
	wchar_t wszToolTarget[128] = L"";
	wchar_t wszRecipeComponent[128] = L"";

	const wchar_t *pszFormat = g_pVGuiLocalize->Find( "#ItemNameFormat" );
	if ( pszFormat )
	{
		g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), pszFormat, 6,
			wszQuality, wszItemName, wszCraftNumber, wszCraftSeries, wszToolTarget, wszRecipeComponent );
	}
	else
	{
		V_wcscpy_safe( wszFullName, L"Unlocalized" );
	}

	return wszFullName;
}

//-----------------------------------------------------------------------------
// Purpose: Find an attribute with the specified class.
//-----------------------------------------------------------------------------
CEconItemAttribute *CEconItemDefinition::IterateAttributes( string_t strClass )
{
	// Returning the first attribute found.
	CEconItemAttribute *pAttribute;
	for ( int i = 0, c = attributes.Count(); i < c; i++ )
	{
		pAttribute = &attributes[i];
		if ( pAttribute->m_strAttributeClass == strClass )
			return pAttribute;
	}

	return NULL;
}
