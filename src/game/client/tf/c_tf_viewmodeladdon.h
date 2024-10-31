//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_TF_VIEWMODELENT_H
#define C_TF_VIEWMODELENT_H

#include "c_tf_player.h"

class C_TFViewModel;

class C_ViewmodelAttachmentModel : public C_BaseAnimating, public IHasOwner, public IModelGlowController
{
	DECLARE_CLASS( C_ViewmodelAttachmentModel, C_BaseAnimating );

public:
	virtual bool			InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );

	virtual int				GetSkin( void );
	virtual bool			ShouldReceiveProjectedTextures( int flags ) { return false; }

	void					SetViewmodel( C_TFViewModel *pViewModel );
	int						GetViewModelType( void );

	virtual void			FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld );
	virtual void			UncorrectViewModelAttachment( Vector &vOrigin );
	virtual bool			IsViewModel() const { return true; }
	virtual RenderGroup_t	GetRenderGroup( void ) { return RENDER_GROUP_VIEW_MODEL_OPAQUE; }
	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }

	virtual int				InternalDrawModel( int flags );
	virtual bool			OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	bool					OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void			StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	// IHasOwner, IModelGlowController
	virtual C_BaseEntity	*GetOwnerViaInterface( void );
	virtual bool			ShouldGlow( void ) { return GetViewModelType() == VMTYPE_L4D; }

	void					SetOuter( CEconEntity *pOuter );
	CEconEntity				*GetOuter( void ) { return m_hOuter.Get(); }

private:
	CHandle<C_TFViewModel>	m_hViewModel;
	CHandle<CEconEntity>	m_hOuter;
};

#endif
