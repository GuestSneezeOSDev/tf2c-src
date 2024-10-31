//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//
#include "cbase.h"
#include "tf_viewmodel.h"
#include "tf_shareddefs.h"
#include "tf_weapon_minigun.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"

// For spy material proxy.
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "prediction.h"
#include "view.h"
#include "eventlist.h"
#endif

#include "bone_setup.h"	//temp

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_viewmodel, CTFViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( TFViewModel, DT_TFViewModel )

BEGIN_NETWORK_TABLE( CTFViewModel, DT_TFViewModel )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOwnerEntity ) ),
	RecvPropInt( RECVINFO( m_iViewModelType ) ),
	RecvPropInt( RECVINFO( m_iViewModelAddonModelIndex ) ),
#else
	SendPropEHandle( SENDINFO( m_hOwnerEntity ) ),
	SendPropInt( SENDINFO( m_iViewModelType ), 3 ),
	SendPropModelIndex( SENDINFO( m_iViewModelAddonModelIndex ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFViewModel )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iViewModelType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iViewModelAddonModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
#endif
END_PREDICTION_DATA()


#ifdef CLIENT_DLL
CTFViewModel::CTFViewModel() : m_LagAnglesHistory("CPredictedViewModel::m_LagAnglesHistory")
{
	m_vLagAngles.Init();
	m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
	m_vLoweredWeaponOffset.Init();

	m_iViewModelType = VMTYPE_NONE;
	m_iViewModelAddonModelIndex = -1;

	m_bInvisibleArms = false;
}
#else
CTFViewModel::CTFViewModel()
{
	m_iViewModelType = VMTYPE_NONE;
	m_iViewModelAddonModelIndex = -1;
}
#endif

#ifdef CLIENT_DLL
void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CUtlVector<AttachedModelData_t> *vecAttachedModels, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags );

// TODO:  Turning this off by setting interp 0.0 instead of 0.1 for now since we have a timing bug to resolve
ConVar cl_wpn_sway_interp( "cl_wpn_sway_interp", "0.0", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar cl_wpn_sway_scale( "cl_wpn_sway_scale", "5.0", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar viewmodel_offset_x( "viewmodel_offset_x", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar viewmodel_offset_y( "viewmodel_offset_y", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar viewmodel_offset_z( "viewmodel_offset_z", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );

ConVar tf_use_min_viewmodels( "tf_use_min_viewmodels", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Use minimized viewmodels." );
ConVar tf2c_invisible_arms( "tf2c_invisible_arms", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Hides the arms used in the viewmodel." );
#endif

//-----------------------------------------------------------------------------
// Purpose:  Adds head bob for off hand models
//-----------------------------------------------------------------------------
void CTFViewModel::AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles )
{
#ifdef CLIENT_DLL
	// if we are an off hand view model (index 1) and we have a model, add head bob.
	// (Head bob for main hand model added by the weapon itself.)
	if ( ViewModelIndex() == 1 && GetModel() != null )
	{
		CalcViewModelBobHelper( owner, &m_BobState );
		AddViewModelBobHelper( eyePosition, eyeAngles, &m_BobState );
	}
#endif
}


void CTFViewModel::SetWeaponModel( const char *modelname, CBaseCombatWeapon *weapon )
{
	if ( !modelname || !modelname[0] )
	{
		RemoveViewmodelAddon();
	}

	BaseClass::SetWeaponModel( modelname, weapon );
}


void CTFViewModel::UpdateViewmodelAddon( int iModelIndex, CEconEntity *pEconEntity )
{
	m_iViewModelAddonModelIndex = iModelIndex;

#ifdef CLIENT_DLL
	C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
	m_bInvisibleArms = ( pPlayer && pPlayer->ShouldHaveInvisibleArms() );

	bool bDestroyArms = ( m_bInvisibleArms && GetViewModelType() == VMTYPE_L4D );

	C_ViewmodelAttachmentModel *pAddon = m_hViewmodelAddon.Get();
	if ( pAddon )
	{
		if ( bDestroyArms )
		{
			pAddon->Release();
		}
		else
		{
			pAddon->SetModelIndex( iModelIndex );
			pAddon->SetOuter( pEconEntity );
		}

		return;
	}

	if ( bDestroyArms )
		return;

	pAddon = new C_ViewmodelAttachmentModel();
	if ( !pAddon->InitializeAsClientEntity( NULL, RENDER_GROUP_VIEW_MODEL_OPAQUE ) )
	{
		pAddon->Release();
		return;
	}

	m_hViewmodelAddon = pAddon;

	pAddon->SetViewmodel( this );
	pAddon->FollowEntity( this );
	pAddon->SetModelIndex( iModelIndex );
	pAddon->SetOuter( pEconEntity );
	pAddon->UpdateVisibility();
#endif
}


void CTFViewModel::RemoveViewmodelAddon( void )
{
	m_iViewModelAddonModelIndex = -1;

#ifdef CLIENT_DLL
	if ( m_hViewmodelAddon.Get() )
	{
		m_hViewmodelAddon->Release();
	}
#endif
}


int CTFViewModel::GetViewModelType( void )
{
	CTFWeaponBase *pOwningWeapon = static_cast<CTFWeaponBase *>( GetOwningWeapon() );
	if ( !pOwningWeapon )
		return m_iViewModelType;

	// Cache the current type incase our weapon goes NULL out of thin air.
	m_iViewModelType = pOwningWeapon->GetViewModelType();
	return m_iViewModelType;
}

#ifdef CLIENT_DLL

void CTFViewModel::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
}


void CTFViewModel::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iViewModelAddonModelIndex != -1 )
	{
		UpdateViewmodelAddon( m_iViewModelAddonModelIndex, GetOwningWeapon() );
	}
	else
	{
		RemoveViewmodelAddon();
	}
}


void CTFViewModel::SetDormant( bool bDormant )
{
	if ( bDormant )
	{
		RemoveViewmodelAddon();
	}

	BaseClass::SetDormant( bDormant );
}


void CTFViewModel::UpdateOnRemove( void )
{
	RemoveViewmodelAddon();
	BaseClass::UpdateOnRemove();
}


int	CTFViewModel::LookupAttachment( const char *pAttachmentName )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon.Get();
		if ( pEnt )
			return pEnt->LookupAttachment( pAttachmentName );
	}

	return BaseClass::LookupAttachment( pAttachmentName );
}


bool CTFViewModel::GetAttachment( int number, matrix3x4_t &matrix )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon.Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, matrix );
	}

	return BaseClass::GetAttachment( number, matrix );
}


bool CTFViewModel::GetAttachment( int number, Vector &origin )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon.Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, origin );
	}

	return BaseClass::GetAttachment( number, origin );
}


bool CTFViewModel::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon.Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, origin, angles );
	}

	return BaseClass::GetAttachment( number, origin, angles );
}


bool CTFViewModel::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon.Get();
		if ( pEnt )
			return pEnt->GetAttachmentVelocity( number, originVel, angleVel );
	}

	return BaseClass::GetAttachmentVelocity( number, originVel, angleVel );
}
#endif


void CTFViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
		return;

	if ( cl_wpn_sway_interp.GetFloat() <= 0.0f )
		return;

	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= cl_wpn_sway_scale.GetFloat();
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;

#endif
}

#ifdef CLIENT_DLL
ConVar cl_gunlowerangle( "cl_gunlowerangle", "90" );
ConVar cl_gunlowerspeed( "cl_gunlowerspeed", "120" );
#endif


void CTFViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
#if defined( CLIENT_DLL )

	Vector vecNewOrigin = eyePosition;
	QAngle vecNewAngles = eyeAngles;

	if ( GetOwner() )
		owner = ToBasePlayer( GetOwner() );

	// Check for lowering the weapon
	C_TFPlayer *pPlayer = ToTFPlayer( owner );

	Assert( pPlayer );

	bool bLowered = pPlayer->IsWeaponLowered();

	QAngle vecLoweredAngles( 0, 0, 0 );

	m_vLoweredWeaponOffset.x = Approach( bLowered ? cl_gunlowerangle.GetFloat() : 0, m_vLoweredWeaponOffset.x, cl_gunlowerspeed.GetFloat() * gpGlobals->frametime );
	vecLoweredAngles.x += m_vLoweredWeaponOffset.x;

	vecNewAngles += vecLoweredAngles;

	// Viewmodel offset
	Vector vecForward, vecRight, vecUp;
	AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );

	vecNewOrigin += vecForward * pPlayer->GetViewModelOffset().x +
		vecRight * pPlayer->GetViewModelOffset().y +
		vecUp * pPlayer->GetViewModelOffset().z;

	C_TFWeaponBase *pOwningWeapon = static_cast<CTFWeaponBase *>( GetOwningWeapon() );
	if ( pOwningWeapon )
	{
		C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
		if ( pPlayer->ShouldUseMinimizedViewModels() )
		{
			vecNewOrigin += vecForward * pOwningWeapon->GetViewmodelOffset().x +
				vecRight * pOwningWeapon->GetViewmodelOffset().y +
				vecUp * pOwningWeapon->GetViewmodelOffset().z;
		}
	}

	BaseClass::CalcViewModelView( owner, vecNewOrigin, vecNewAngles );
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Don't render the weapon if its supposed to be lowered and we have 
// finished the lowering animation
//-----------------------------------------------------------------------------
int CTFViewModel::DrawModel( int flags )
{
	// Check for lowering the weapon, but don't draw viewmodels of dead players.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return 0;

	// Only draw viewmodel of the spectated player.
	if ( GetLocalObservedPlayer( true ) != pPlayer )
		return 0;

	// Stop drawing once we fully lower.
	if ( pPlayer->IsWeaponLowered() && fabs( m_vLoweredWeaponOffset.x - cl_gunlowerangle.GetFloat() ) < 0.1f )
		return 1;

	return BaseClass::DrawModel( flags );
}


int CTFViewModel::InternalDrawModel( int flags )
{
	CMatRenderContextPtr pRenderContext( materials );
	if ( ShouldFlipViewModel() )
	{
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );
	}

	int iRet = 0;

	// Draw the attachments together with the viewmodel so any effects applied to VM are applied to attachments as well.
	if ( !m_bInvisibleArms && GetViewModelType() == VMTYPE_TF2 )
	{
		iRet = BaseClass::InternalDrawModel( flags );
	}

	if ( m_hViewmodelAddon.Get() )
	{
		// Necessary for lighting to blend.
		m_hViewmodelAddon->CreateModelInstance();
		m_hViewmodelAddon->InternalDrawModel( flags );
	}

	if ( GetViewModelType() != VMTYPE_TF2 )
	{
		iRet = BaseClass::InternalDrawModel( flags );
	}

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	return iRet;
}


bool CTFViewModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use camera position for lighting origin.
	pInfo->pLightingOrigin = &MainViewOrigin();

	int iRet = 0;

	// Duplicate the info onto the attachment as well.
	if ( !m_bInvisibleArms && GetViewModelType() == VMTYPE_TF2 )
	{
		iRet = BaseClass::OnInternalDrawModel( pInfo );
	}

	if ( m_hViewmodelAddon.Get() )
	{
		m_hViewmodelAddon->OnInternalDrawModel( pInfo );
	}
	
	if ( GetViewModelType() != VMTYPE_TF2 )
	{
		iRet = BaseClass::OnInternalDrawModel( pInfo );
	}

	return iRet;
}


bool CTFViewModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnPostInternalDrawModel( pInfo ) )
		return false;

	if ( GetViewModelType() == VMTYPE_TF2 )
		return true;

	C_BaseCombatWeapon *pOwningWeapon = GetOwningWeapon();
	if ( !pOwningWeapon || pOwningWeapon && !pOwningWeapon->AttachmentModelsShouldBeVisible() )
		return true;

	// Only need to draw the attached models if the weapon doesn't want to override the viewmodel attachments.
	DrawEconEntityAttachedModels( this, &pOwningWeapon->m_vecAttachedModels, pInfo, kAttachedModelDisplayFlag_ViewModel );

	return true;
}


void CTFViewModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if ( GetViewModelType() != VMTYPE_TF2 )
	{
		C_BaseCombatWeapon *pOwningWeapon = GetOwningWeapon();
		if ( !pOwningWeapon ) 
			return;

		pOwningWeapon->ViewModelAttachmentBlending( hdr, pos, q, currentTime, boneMask, m_hViewmodelAddon.Get() );
	}
}


int CTFViewModel::GetSkin()
{
	if ( m_iViewModelType == VMTYPE_TF2 )
		return GetArmsSkin();

	C_TFWeaponBase *pOwningWeapon = static_cast<C_TFWeaponBase *>( GetOwningWeapon() );
	if ( !pOwningWeapon ) 
		return 0;

	return pOwningWeapon->GetSkin();
}


int CTFViewModel::GetArmsSkin( void )
{
	C_BasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	return GetTeamSkin( pOwner->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Mimic the weapon's bodygroups.
//-----------------------------------------------------------------------------
int CTFViewModel::GetBody( void )
{
	C_BaseCombatWeapon *pOwningWeapon = GetOwningWeapon();
	if ( pOwningWeapon && pOwningWeapon->GetModelIndex() == GetModelIndex() )
	{
		return pOwningWeapon->GetBody();
	}

	return BaseClass::GetBody();
}

//-----------------------------------------------------------------------------
// Purpose: Mimic the weapon's pose parameters.
//-----------------------------------------------------------------------------
void CTFViewModel::GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] )
{
	C_BaseCombatWeapon *pOwningWeapon = GetOwningWeapon();
	if ( pOwningWeapon && pOwningWeapon->GetModelIndex() == GetModelIndex() )
	{
		return pOwningWeapon->GetPoseParameters( pOwningWeapon->GetModelPtr(), poseParameter );
	}

	return BaseClass::GetPoseParameters( pStudioHdr, poseParameter );
}

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void CTFViewModel::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// Don't process animevents if it's not drawn.
	C_TFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner || pOwner->ShouldDrawThisPlayer() )
		return;

	C_TFWeaponBase *pOwningWeapon = static_cast<C_TFWeaponBase *>( GetOwningWeapon() );
	if ( !pOwningWeapon )
		return;

	switch ( event )
	{
		case AE_CL_MAG_EJECT_VM:
		{
			// If they have auto-reload, only do this with an empty clip otherwise it looks nasty.
			if ( pOwner->ShouldAutoReload() && pOwningWeapon->Clip1() != 0 )
				return;

			Vector vecForce;
			UTIL_StringToVector( vecForce.Base(), options );

			pOwningWeapon->EjectMagazine( vecForce );
			break;
		}
		case AE_CL_MAG_EJECT_UNHIDE_VM:
		{
			pOwningWeapon->UnhideMagazine();
			break;
		}
		default:
			BaseClass::FireEvent( origin, angles, event, options );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CViewModelInvisProxy : public CEntityMaterialProxy
{
public:
	CViewModelInvisProxy( void );
	virtual ~CViewModelInvisProxy( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pPercentInvisible;

};


CViewModelInvisProxy::CViewModelInvisProxy( void )
{
	m_pPercentInvisible = NULL;
}


CViewModelInvisProxy::~CViewModelInvisProxy( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input : *pMaterial - 
//-----------------------------------------------------------------------------
bool CViewModelInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bFound;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bFound );

	return bFound;
}

ConVar tf_vm_min_invis( "tf_vm_min_invis", "0.22", FCVAR_ARCHIVE, "minimum invisibility value for view model", true, 0.0, true, 1.0 );
ConVar tf_vm_max_invis( "tf_vm_max_invis", "0.5", FCVAR_ARCHIVE, "maximum invisibility value for view model", true, 0.0, true, 1.0 );

//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CViewModelInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	if ( !pEnt )
		return;

	C_BaseViewModel *pVM = dynamic_cast<C_BaseViewModel *>( pEnt );
	if ( !pVM )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	C_TFPlayer *pPlayer = ToTFPlayer( pVM->GetOwner() );
	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	float flPercentInvisible = pPlayer->GetPercentInvisible();

	// Remap from 0.22 to 0.5,
	// but drop to 0.0 if we're not invis at all.
	m_pPercentInvisible->SetFloatValue( flPercentInvisible < 0.01f ? 0.0f : RemapVal( flPercentInvisible, 0.0f, 1.0f, tf_vm_min_invis.GetFloat(), tf_vm_max_invis.GetFloat() ) );
}

IMaterial *CViewModelInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}
EXPOSE_INTERFACE( CViewModelInvisProxy, IMaterialProxy, "vm_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif // CLIENT_DLL