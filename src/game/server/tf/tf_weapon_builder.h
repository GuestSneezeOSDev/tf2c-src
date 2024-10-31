//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WEAPON_BUILDER_H
#define TF_WEAPON_BUILDER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

class CBaseObject;

//=========================================================
// Builder Weapon
//=========================================================
class CTFWeaponBuilder : public CTFWeaponBase
{

	DECLARE_CLASS( CTFWeaponBuilder, CTFWeaponBase );
public:
	CTFWeaponBuilder();
	~CTFWeaponBuilder();

	DECLARE_SERVERCLASS();

	virtual void	SetSubType( int iSubType );
	virtual void	SetObjectMode( int iObjectMode );
	virtual void	Precache( void );
	virtual bool	CanDeploy( void );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual bool	Deploy( void );	
	virtual Activity GetDrawActivity( void );
	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual bool	AllowsAutoSwitchTo( void ) const;
	virtual bool	ForceWeaponSwitch( void ) const;

	virtual int		GetType( void ) { return m_iObjectType; }

	void	SetCurrentState( int iState );
	void	SwitchOwnersWeaponToLast();

	// Placement
	void	StartPlacement( void );
	void	StopPlacement( void );
	void	UpdatePlacementState( void );		// do a check for valid placement
	bool	IsValidPlacement( void );			// is this a valid placement pos?


	// Building
	void	StartBuilding( void );

	// Selection
	bool	HasAmmo( void );
	int		GetSlot( void ) const;
	int		GetPosition( void ) const;
	const char *GetPrintName( void ) const;

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_BUILDER; }

	virtual void	WeaponReset( void );

#ifdef GAME_DLL
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE { return false; }
	virtual bool TFBot_IsCombatWeapon()                      OVERRIDE { return false; }
	virtual bool TFBot_IsExplosiveProjectileWeapon()         OVERRIDE { return false; }
	virtual bool TFBot_IsContinuousFireWeapon()              OVERRIDE { return false; }
	virtual bool TFBot_IsBarrageAndReloadWeapon()            OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE { return true;  }
	virtual bool TFBot_ShouldFireAtInvulnerableEnemies()     OVERRIDE { return false; }
	virtual bool TFBot_ShouldAimForHeadshots()               OVERRIDE { return false; }
	virtual bool TFBot_ShouldCompensateAimForGravity()       OVERRIDE { return false; }
	virtual bool TFBot_IsSniperRifle()                       OVERRIDE { return false; }
	virtual bool TFBot_IsRocketLauncher()                    OVERRIDE { return false; }
#endif

public:
	CNetworkVar( int, m_iBuildState );
	CNetworkVar( unsigned int, m_iObjectType );
	CNetworkVar( unsigned int, m_iObjectMode );

	CNetworkHandle( CBaseObject, m_hObjectBeingBuilt );

	int m_iValidBuildPoseParam;

	float m_flNextDenySound;
};

//=========================================================
// Builder Weapon
//=========================================================
class CTFWeaponSapper : public CTFWeaponBuilder
{
	DECLARE_CLASS( CTFWeaponSapper, CTFWeaponBuilder );
public:
	CTFWeaponSapper();
	~CTFWeaponSapper();

	DECLARE_SERVERCLASS();

	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

};


#endif // TF_WEAPON_BUILDER_H
