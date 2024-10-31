//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_revolver.h"

//=============================================================================
//
// Weapon Revolver tables.
//
//CREATE_SIMPLE_WEAPON_TABLE( TFRevolver, tf_weapon_revolver )
IMPLEMENT_NETWORKCLASS_ALIASED( TFRevolver, DT_TFRevolver );
BEGIN_NETWORK_TABLE( CTFRevolver, DT_TFRevolver)
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iShotsFired ) ),
#else
	SendPropInt( SENDINFO( m_iShotsFired ), 4, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRevolver )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_revolver, CTFRevolver );
PRECACHE_WEAPON_REGISTER( tf_weapon_revolver );

CTFRevolver::CTFRevolver()
{
#ifdef CLIENT_DLL
	m_iCylinderPoseParameter = -1;
#endif
}


bool CTFRevolver::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		UpdateCylinder();
		return true;
	}

	return false;
}


void CTFRevolver::UpdateCylinder( void )
{
#ifdef CLIENT_DLL
	if ( m_iCylinderPoseParameter != -1 )
	{
		SetPoseParameter( m_iCylinderPoseParameter, (float)m_iShotsFired / (float)GetMaxClip1() );
	}
#endif
}


void CTFRevolver::DoFireEffects()
{
	BaseClass::DoFireEffects();
	++m_iShotsFired;
	if ( m_iShotsFired >= GetMaxClip1() )
	{
		m_iShotsFired = 0;
	}
	UpdateCylinder();
}

#ifdef CLIENT_DLL

CStudioHdr *CTFRevolver::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Check that the model is compatible.
	if ( m_pModelWeaponData && m_pModelWeaponData->GetInt( "revolver_clip", -2 ) == GetMaxClip1() )
	{
		m_iCylinderPoseParameter = LookupPoseParameter( "cylinder_spin" );
	}
	else
	{
		m_iCylinderPoseParameter = -1;
	}

	return hdr;
}


void CTFRevolver::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );
	UpdateCylinder();
}
#endif