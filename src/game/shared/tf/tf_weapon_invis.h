//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: PDA Weapon
//
//=============================================================================

#ifndef TF_WEAPON_INVIS_H
#define TF_WEAPON_INVIS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"

// Client specific.
#if defined( CLIENT_DLL ) 
#define CTFWeaponInvis C_TFWeaponInvis
#endif

enum ETFWatchTypes
{
	TF_INVIS_STOCK = 0,
	TF_INVIS_FEIGN,		// Unimplemented.
	TF_INVIS_MOTION,	// Unimplemented.
	TF_INVIS_SPEED,
};

class CTFWeaponInvis : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponInvis, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponInvis();

	virtual void	Spawn();
	virtual void	Precache();
	virtual void	OnActiveStateChanged( int iOldState );
	virtual void	SecondaryAttack() { };
	virtual bool	Deploy( void );

	virtual void	HideThink( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual ETFWeaponID		GetWeaponID( void ) const				{ return TF_WEAPON_INVIS; }
	virtual bool	ShouldDrawCrosshair( void )						{ return false; }
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }

	virtual bool	VisibleInWeaponSelection( void )				{ return false; }

	virtual bool	ShouldShowControlPanels( void )					{ return true; }

	virtual void	SetWeaponVisible( bool bVisible );

	virtual void	ItemBusyFrame( void ) { };

	ETFWatchTypes	GetInvisType( void )							{ int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return (ETFWatchTypes)iMode; };
	// Unimplemented.
	/*virtual bool	HasFeignDeath( void )							{ return ( GetInvisType() == TF_INVIS_FEIGN ); }
	virtual bool	HasMotionCloak( void )							{ return ( GetInvisType() == TF_INVIS_MOTION ); }*/
	virtual bool	HasSpeedCloak( void )							{ return ( GetInvisType() == TF_INVIS_SPEED ); }

	virtual void	SetCloakRates( void );

	virtual bool	ActivateInvisibilityWatch( void );
	virtual void	CleanupInvisibilityWatch( void );

	virtual	bool	AllowsAutoSwitchTo( void )						{ return false; }
	virtual bool	CanDeploy( void )								{ return false; }

	virtual float	OwnerTakeDamage( const CTakeDamageInfo &info );

	float			GetProgress( void );
	const char		*GetEffectLabelText( void )						{ return "#TF_Cloak"; }
#ifdef CLIENT_DLL
	virtual void	FireGameEvent( IGameEvent * event ) override;
#endif
#ifndef CLIENT_DLL
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
#endif

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

private:
	CTFWeaponInvis( const CTFWeaponInvis & ) {}

};

#endif // TF_WEAPON_INVIS_H