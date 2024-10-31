//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "vscript_shared.h"
#include "econ_entity.h"
#include "econ_item_system.h"

#include "eventlist.h"

#ifdef CLIENT_DLL
#include "model_types.h"

#include "tf_weaponbase.h"
#include "tf_viewmodel.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( EconEntity, DT_EconEntity )

BEGIN_NETWORK_TABLE( CEconEntity, DT_EconEntity )
#ifdef CLIENT_DLL
	RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE( DT_AttributeContainer ) ),
#else
	SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE( DT_AttributeContainer ) ),
#endif
END_NETWORK_TABLE()

BEGIN_ENT_SCRIPTDESC( CEconEntity, CBaseAnimating, "Econ Entity" )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( AddAttribute, "Add an attribute to the entity" )
	DEFINE_SCRIPTFUNC( RemoveAttribute, "Remove an attribute from the entity" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptReapplyProvision, "ReapplyProvision", "Flush any attribute changes we provide onto our owner")
#endif
END_SCRIPTDESC()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_EconEntity )
	DEFINE_PRED_TYPEDESCRIPTION( m_AttributeManager, CAttributeContainer ),
END_PREDICTION_DATA()
#endif

#ifdef CLIENT_DLL

void UpdateEconEntityAttachedModels( CEconItemView *pItem, CUtlVector<AttachedModelData_t> *vecAttachedModels, int iTeamNumber = FIRST_GAME_TEAM )
{
	vecAttachedModels->Purge();

	CEconItemDefinition *pItemDef = pItem && pItem->IsValid() ? pItem->GetStaticData() : NULL;
	if ( pItemDef )
	{
		int iAttachedModels = pItemDef->GetNumAttachedModels( iTeamNumber );
		for ( int i = 0; i < iAttachedModels; i++ )
		{
			attachedmodel_t	*pModel = pItemDef->GetAttachedModelData( iTeamNumber, i );
			if ( !pModel )
				continue;

			int iModelIndex = modelinfo->GetModelIndex( pModel->m_pszModelName );
			if ( iModelIndex >= 0 )
			{
				AttachedModelData_t attachedModelData;
				attachedModelData.m_pModel			   = modelinfo->GetModel( iModelIndex );
				attachedModelData.m_iModelDisplayFlags = pModel->m_iModelDisplayFlags;
				vecAttachedModels->AddToTail( attachedModelData );
			}
		}
	}
}


void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CUtlVector<AttachedModelData_t> *vecAttachedModels, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags )
{
	if ( !pEnt || !pInfo )
		return;

	// Draw our attached models
	for ( int i = 0; i < vecAttachedModels->Size(); i++ )
	{
		const AttachedModelData_t &attachedModel = vecAttachedModels->Element( i );
		if ( attachedModel.m_pModel && ( attachedModel.m_iModelDisplayFlags & iMatchDisplayFlags ) )
		{
			ClientModelRenderInfo_t infoAttached = *pInfo;
			
			infoAttached.pRenderable	= pEnt;
			infoAttached.instance		= MODEL_INSTANCE_INVALID;
			infoAttached.entity_index	= pEnt->index;
			infoAttached.pModel			= attachedModel.m_pModel;
			infoAttached.pModelToWorld  = &infoAttached.modelToWorld;

			// Turns the origin + angles into a matrix
			AngleMatrix( infoAttached.angles, infoAttached.origin, infoAttached.modelToWorld );

			DrawModelState_t state;
			matrix3x4_t *pBoneToWorld;
			bool bMarkAsDrawn = modelrender->DrawModelSetup( infoAttached, &state, NULL, &pBoneToWorld );
			pEnt->DoInternalDrawModel( &infoAttached, ( bMarkAsDrawn && ( infoAttached.flags & STUDIO_RENDER ) ) ? &state : NULL, pBoneToWorld );
		}
	}
}
#endif // CLIENT_DLL


CEconEntity::CEconEntity()
{
	m_pAttributes = this;
}


CEconEntity::~CEconEntity()
{
    m_pAttributes = nullptr;
}

#ifdef CLIENT_DLL

void CEconEntity::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_AttributeManager.OnPreDataChanged( updateType );
}


void CEconEntity::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		InitializeAttributes();
	}

	BaseClass::OnDataChanged( updateType );

	m_AttributeManager.OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Only do this on entity creation.
		UpdateAttachmentModels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Self-explanatory.
//-----------------------------------------------------------------------------
void CEconEntity::UpdateAttachmentModels( void )
{
	// Update the state of additional model attachments
	UpdateEconEntityAttachedModels( GetItem(), &m_vecAttachedModels, GetTeamNumber() );
}


bool CEconEntity::OnFireEvent( C_BaseViewModel *pViewModel, const Vector &origin, const QAngle &angles, int event, const char *options )
{
	if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		CTFViewModel *pTFViewModel = assert_cast<CTFViewModel *>( pViewModel );
		C_ViewmodelAttachmentModel *pViewModelAttachment = pTFViewModel ? pTFViewModel->m_hViewmodelAddon.Get() : NULL;
		if ( pViewModelAttachment )
		{
			pViewModelAttachment->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes can override this.
//-----------------------------------------------------------------------------
void CEconEntity::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment )
{
}


bool CEconEntity::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use same lighting origin as player when carried.
	if ( GetMoveParent() )
	{
		pInfo->pLightingOrigin = &GetMoveParent()->WorldSpaceCenter();
	}

	if ( !BaseClass::OnInternalDrawModel( pInfo ) )
		return false;

	if ( !AttachmentModelsShouldBeVisible() )
		return true;

	DrawEconEntityAttachedModels( this, &m_vecAttachedModels, pInfo, kAttachedModelDisplayFlag_WorldModel );

	return true;
}
#endif


void CEconEntity::SetItem( CEconItemView const &newItem )
{
	m_AttributeManager.SetItem( newItem );
}


bool CEconEntity::HasItemDefinition( void ) const
{
	return ( GetItem()->GetItemDefIndex() >= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut to get item ID.
//-----------------------------------------------------------------------------
int CEconEntity::GetItemID( void )
{
	return GetItem()->GetItemDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes need to override this.
//-----------------------------------------------------------------------------
void CEconEntity::GiveTo( CBaseEntity *pEntity )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add or remove this from owner's attribute providers list.
//-----------------------------------------------------------------------------
void CEconEntity::ReapplyProvision( void )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	CBaseEntity *pOldOwner = m_hOldOwner.Get();
	if ( pOwner != pOldOwner )
	{
		if ( pOldOwner )
		{
			m_AttributeManager.StopProvidingTo( pOldOwner );
		}

		if ( pOwner )
		{
			m_AttributeManager.ProvideTo( pOwner );
			m_hOldOwner = pOwner;
		}
		else
		{
			m_hOldOwner = NULL;
		}
	}
}

void CEconEntity::InitializeAttributes( void )
{
	m_AttributeManager.InitializeAttributes( this );
	m_AttributeManager.SetProvidrType( PROVIDER_WEAPON );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::AddAttribute( char const *pszAttribName, float flValue, float flDuration )
{
	CEconAttributeDefinition *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );
	if ( pAttribute )
	{
		GetAttributeList()->SetRuntimeAttributeValue( pAttribute, flValue );
		GetAttributeManager()->OnAttributesChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::RemoveAttribute( char const *pszAttributeName )
{
	CEconAttributeDefinition *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( pszAttributeName );
	if ( pAttribute )
	{
		GetAttributeList()->RemoveAttribute( pAttribute );
		GetAttributeManager()->OnAttributesChanged();
	}
}


void CEconEntity::UpdateOnRemove( void )
{
	SetOwnerEntity( NULL );
	ReapplyProvision();
	BaseClass::UpdateOnRemove();
}


void CEconEntity::UpdateWeaponBodygroups( CBasePlayer *pPlayer, bool bForce /*= false*/ )
{
	// Assume that pPlayer is a valid pointer.
	CBaseCombatWeapon *pWeapon;
	for ( int i = 0, c = pPlayer->WeaponCount(); i < c; i++ ) 
	{
		pWeapon = pPlayer->GetWeapon( i );
		if ( !pWeapon || pWeapon->IsDynamicModelLoading() )
			continue;

		pWeapon->UpdateBodygroups( pPlayer, bForce );
	}
}


bool CEconEntity::UpdateBodygroups( CBasePlayer *pOwner, bool bForce )
{
	// Assume that pPlayer is a valid pointer.
	CEconItemView *pItem = GetItem();
	if ( !pItem )
		return false;

	CEconItemDefinition *pStatic = pItem->GetStaticData();
	if ( !pStatic )
		return false;

	if ( pStatic->hide_bodygroups_deployed_only )
	{
		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>( this );
		if ( pWeapon && pWeapon->WeaponState() != WEAPON_IS_ACTIVE )
			return false;
	}

	EconItemVisuals *pVisuals = pStatic->GetVisuals( GetTeamNumber() );
	if ( !pVisuals )
		return false;

	const char *pszBodyGroupName;
	int iBodygroup;
	for ( unsigned int i = 0, c = pVisuals->player_bodygroups.Count(); i < c; i++ )
	{
		pszBodyGroupName = pVisuals->player_bodygroups.GetElementName( i );
		if ( pszBodyGroupName )
		{
			iBodygroup = pOwner->FindBodygroupByName( pszBodyGroupName );
			if ( iBodygroup == -1 )
				continue;

			pOwner->SetBodygroup( iBodygroup, pVisuals->player_bodygroups.Element( i ) );
		}
	}

	return true;
}
