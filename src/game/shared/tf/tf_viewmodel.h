//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_VIEWMODEL_H
#define TF_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#include "c_tf_viewmodeladdon.h"
#endif

#if defined( CLIENT_DLL )
#define CTFViewModel C_TFViewModel
#endif

class CTFViewModel : public CBaseViewModel
#ifdef CLIENT_DLL
, public IModelGlowController
#endif
{
public:
	DECLARE_CLASS( CTFViewModel, CBaseViewModel );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFViewModel( void );

	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );
	virtual void AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles );

	virtual void SetWeaponModel( const char *pszModelname, CBaseCombatWeapon *weapon );

	void UpdateViewmodelAddon( int iModelIndex, CEconEntity *pEconEntity );
	void RemoveViewmodelAddon( void );

	int GetViewModelType( void );

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void SetDormant( bool bDormant );
	virtual void UpdateOnRemove( void );

	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual void StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	virtual int GetSkin( void );
	virtual int	GetArmsSkin( void );
	virtual int GetBody( void );
	virtual void GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );
	BobState_t	&GetBobState() { return m_BobState; }

	virtual int DrawModel( int flags );
	virtual int	InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual bool OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	CHandle<C_ViewmodelAttachmentModel> m_hViewmodelAddon;

	// Attachments
	virtual int				LookupAttachment( const char *pAttachmentName );
	virtual bool			GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool			GetAttachment( int number, Vector &origin );
	virtual	bool			GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool			GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );

	virtual void			FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	// IModelGlowController
	virtual bool			ShouldGlow( void ) { return GetViewModelType() == VMTYPE_L4D; }
#endif

private:
	CNetworkVar( int, m_iViewModelType );
	CNetworkVar( int, m_iViewModelAddonModelIndex );

#ifdef CLIENT_DLL
	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;
	BobState_t m_BobState;

	QAngle m_vLoweredWeaponOffset;

	bool m_bInvisibleArms;

	CTFViewModel( const CTFViewModel & );
#endif
};

#endif // TF_VIEWMODEL_H
