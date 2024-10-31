//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_REVOLVER_H
#define TF_WEAPON_REVOLVER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRevolver C_TFRevolver
#endif

//=============================================================================
//
// TF Weapon Revolver.
//
class CTFRevolver : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFRevolver, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFRevolver();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_REVOLVER; }
	
	virtual bool	Deploy( void );

	void			UpdateCylinder( void );
	
	virtual void DoFireEffects();
#ifdef CLIENT_DLL
	virtual CStudioHdr	*OnNewModel( void );
	virtual void	ThirdPersonSwitch( bool bThirdPerson );
#endif

private:
	CNetworkVar( int, m_iShotsFired );

#ifdef CLIENT_DLL
	int m_iCylinderPoseParameter;
#endif

	CTFRevolver( const CTFRevolver & );
};

#endif // TF_WEAPON_REVOLVER_H