//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BONESAW_H
#define TF_WEAPON_BONESAW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBonesaw C_TFBonesaw
#endif

//=============================================================================
//
// Bonesaw class.
//
class CTFBonesaw : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFBonesaw, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFBonesaw();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_BONESAW; }

	virtual bool		Deploy( void );
	virtual void		SecondaryAttack( void );
	virtual void		ItemPostFrame( void );
	virtual void		ItemBusyFrame( void );

	void				UpdateChargeLevel( void );

#ifdef CLIENT_DLL
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual CStudioHdr	*OnNewModel( void );
	virtual void		ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment );
	virtual void		ThirdPersonSwitch( bool bThirdPerson );
	void				UpdateChargePoseParam( void );
#endif

private:
	CTFBonesaw( const CTFBonesaw & );

	CNetworkVar( float, m_flChargeLevel );

#ifdef CLIENT_DLL
	int m_iChargePoseParameter;
#endif
};

#endif // TF_WEAPON_BONESAW_H
