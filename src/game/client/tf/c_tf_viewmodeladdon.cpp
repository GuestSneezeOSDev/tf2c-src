//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_viewmodeladdon.h"
#include "tf_viewmodel.h"
#include "tf_gamerules.h"
#include "view.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CUtlVector<AttachedModelData_t> *vecAttachedModels, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags );


bool C_ViewmodelAttachmentModel::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	if ( BaseClass::InitializeAsClientEntity( pszModelName, renderGroup ) )
	{
		// EF_NODRAW so it won't get drawn directly. We want to draw it from the viewmodel.
		AddEffects( EF_BONEMERGE | EF_BONEMERGE_FASTCULL | EF_NODRAW );
		return true;
	}

	return false;
}


void C_ViewmodelAttachmentModel::SetViewmodel( C_TFViewModel *pViewModel )
{
	m_hViewModel.Set( pViewModel );
}


int C_ViewmodelAttachmentModel::GetViewModelType( void )
{
	if ( m_hViewModel.Get() )
		return m_hViewModel->GetViewModelType();

	return VMTYPE_NONE;
}


int C_ViewmodelAttachmentModel::GetSkin( void )
{
	if ( m_hViewModel.Get() && GetViewModelType() != VMTYPE_TF2 )
		return m_hViewModel->GetArmsSkin();

	CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( GetOuter() );
	if ( !pWeapon )
		return 0;

	return pWeapon->GetSkin();
}

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );


void C_ViewmodelAttachmentModel::FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld )
{
	Vector vecOrigin;
	MatrixPosition( attachmentToWorld, vecOrigin );
	::FormatViewModelAttachment( vecOrigin, false );
	PositionMatrix( vecOrigin, attachmentToWorld );
}


void C_ViewmodelAttachmentModel::UncorrectViewModelAttachment( Vector &vOrigin )
{
	// Unformat the attachment.
	::FormatViewModelAttachment( vOrigin, true );
}


int C_ViewmodelAttachmentModel::InternalDrawModel( int flags )
{
	if ( GetViewModelType() != VMTYPE_TF2 )
		return BaseClass::InternalDrawModel( flags );

	CMatRenderContextPtr pRenderContext( materials );
	if ( m_hViewModel->ShouldFlipViewModel() )
	{
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );
	}

	int iRet = BaseClass::InternalDrawModel( flags );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	return iRet;
}


bool C_ViewmodelAttachmentModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		// Use camera position for lighting origin.
		pInfo->pLightingOrigin = &MainViewOrigin();
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}


bool C_ViewmodelAttachmentModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnPostInternalDrawModel( pInfo ) )
		return false;

	if ( GetViewModelType() != VMTYPE_TF2 )
		return true;

	C_EconEntity *pItem = GetOuter();
	if ( !pItem || ( pItem && !pItem->AttachmentModelsShouldBeVisible() ) )
		return true;

	DrawEconEntityAttachedModels( this, &pItem->m_vecAttachedModels, pInfo, kAttachedModelDisplayFlag_ViewModel );

	return true;
}


void C_ViewmodelAttachmentModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if ( GetOuter() && GetViewModelType() == VMTYPE_TF2 )
	{
		GetOuter()->ViewModelAttachmentBlending( hdr, pos, q, currentTime, boneMask, this );
	}
}


C_BaseEntity *C_ViewmodelAttachmentModel::GetOwnerViaInterface( void )
{
	return m_hViewModel->GetOwnerViaInterface();
}


void C_ViewmodelAttachmentModel::SetOuter( CEconEntity *pOuter )
{
	m_nBody = 0;
	m_hOuter = pOuter;
	SetOwnerEntity( pOuter );
}
