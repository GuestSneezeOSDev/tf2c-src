//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_GRENADELAUNCHER_H
#define TF_WEAPON_GRENADELAUNCHER_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeLauncher C_TFGrenadeLauncher
#endif

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeLauncher : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFGrenadeLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrenadeLauncher();
	~CTFGrenadeLauncher();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_GRENADELAUNCHER; }

	virtual bool	Deploy( void );
	virtual void	WeaponReset( void );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual	void	ItemPostFrame( void );
	virtual void	ItemPreFrame( void );
	virtual void	OnReloadSinglyUpdate( void );
	virtual int		GetMaxAmmo( void );
	virtual float	GetProjectileSpeed( void );

	void			UpdateGrenadeDrum( bool bUpdateValue );

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual CStudioHdr	*OnNewModel( void );
	virtual void	StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	void			UpdateBarrelMovement( void );
	virtual void	ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment );
	virtual void	ThirdPersonSwitch( bool bThirdPerson );
#endif

#ifdef GAME_DLL
	virtual bool TFBot_IsExplosiveProjectileWeapon()     OVERRIDE { return true;  }
	virtual bool TFBot_IsContinuousFireWeapon()          OVERRIDE { return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()        OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies() OVERRIDE { return true;  }
	virtual bool TFBot_ShouldCompensateAimForGravity()   OVERRIDE { return true;  }
#endif

private:
	CNetworkVar( int, m_iShellsLoaded );

	// Barrel rotation needs to be in sync
	CNetworkVar( int, m_iCurrentTube );	// Which tube is the one we just fired out of
	CNetworkVar( int, m_iGoalTube );	// Which tube is the one we would like to fire out of next?

	int		m_iBarrelBone;
	float	m_flBarrelRotateBeginTime;	// What time did we begin the animation to rotate to the next barrel?
	float	m_flBarrelAngle;			// What is the current rotation of the barrel?
	bool	m_bCurrentAndGoalTubeEqual;

#ifdef CLIENT_DLL
	int m_iOldClip;

	int m_iDrumPoseParameter;
	int m_iGrenadesBodygroup;
#endif

	CTFGrenadeLauncher( const CTFGrenadeLauncher & ) {}
};

#endif // TF_WEAPON_GRENADELAUNCHER_H
