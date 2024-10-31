//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bonesaw.h"
#include "tf_weapon_medigun.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "c_tf_viewmodeladdon.h"
#endif

#define UBERSAW_CHARGE_POSEPARAM		"syringe_charge_level"

//=============================================================================
//
// Weapon Bonesaw tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBonesaw, DT_TFBonesaw );

BEGIN_NETWORK_TABLE( CTFBonesaw, DT_TFBonesaw )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeLevel ) ),
#else
	SendPropFloat( SENDINFO( m_flChargeLevel ), -1, SPROP_NOSCALE, 0.0f, 1.0f ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBonesaw )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bonesaw, CTFBonesaw );
PRECACHE_WEAPON_REGISTER( tf_weapon_bonesaw );

CTFBonesaw::CTFBonesaw()
{
#ifdef CLIENT_DLL
	m_iChargePoseParameter = -1;
#endif
}


bool CTFBonesaw::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		UpdateChargeLevel();
		return true;
	}

	return false;
}


void CTFBonesaw::SecondaryAttack( void )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	int iSpecialTaunt = 0;
	CALL_ATTRIB_HOOK_INT( iSpecialTaunt, special_taunt );
	if ( iSpecialTaunt )
	{
		pPlayer->Taunt();
		return;
	}
#endif

	BaseClass::SecondaryAttack();
}


void CTFBonesaw::ItemPostFrame( void )
{
	UpdateChargeLevel();
	BaseClass::ItemPostFrame();
}


void CTFBonesaw::ItemBusyFrame( void )
{
	UpdateChargeLevel();
	BaseClass::ItemBusyFrame();
}


void CTFBonesaw::UpdateChargeLevel( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		m_flChargeLevel = pOwner->MedicGetChargeLevel();

#ifdef CLIENT_DLL
		UpdateChargePoseParam();
#endif
	}
}

#ifdef CLIENT_DLL

void CTFBonesaw::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( !GetPredictable() )
	{
		UpdateChargePoseParam();
	}
}


CStudioHdr *CTFBonesaw::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	m_iChargePoseParameter = LookupPoseParameter( UBERSAW_CHARGE_POSEPARAM );
	return hdr;
}


void CTFBonesaw::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment )
{
	// Support both styles here.
	if ( pAttachment && GetViewModelType() == VMTYPE_TF2 )
	{
		int iChargePoseParameter = pAttachment->LookupPoseParameter( UBERSAW_CHARGE_POSEPARAM );
		if ( iChargePoseParameter != -1 )
		{
			pAttachment->SetPoseParameter( iChargePoseParameter, m_flChargeLevel );
		}
	}
}


void CTFBonesaw::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );
	UpdateChargePoseParam();
}


void CTFBonesaw::UpdateChargePoseParam( void )
{
	if ( m_iChargePoseParameter == -1 )
		return;

	SetPoseParameter( m_iChargePoseParameter, m_flChargeLevel );
}
#endif
