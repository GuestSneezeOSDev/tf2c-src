//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: PDA Weapon
//
//=============================================================================

#ifndef TF_WEAPON_PDA_H
#define TF_WEAPON_PDA_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

// Client specific.
#if defined( CLIENT_DLL ) 
	#define CTFWeaponPDA					C_TFWeaponPDA
	#define CTFWeaponPDA_Engineer_Build		C_TFWeaponPDA_Engineer_Build
	#define CTFWeaponPDA_Engineer_Destroy	C_TFWeaponPDA_Engineer_Destroy
	#define CTFWeaponPDA_Spy				C_TFWeaponPDA_Spy
#endif

class CTFWeaponPDA : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponPDA, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	CTFWeaponPDA();

	virtual void		Spawn();

#if !defined( CLIENT_DLL )
	virtual void		Precache();
	virtual void		GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
#else
	virtual float		CalcViewmodelBob( void );
#endif

	virtual bool		ShouldShowControlPanels( void );

	virtual void		PrimaryAttack();
	virtual void		SecondaryAttack();
	virtual ETFWeaponID	GetWeaponID( void ) const					{ return TF_WEAPON_PDA; }
	virtual bool		ShouldDrawCrosshair( void )						{ return false; }
	virtual bool		HasPrimaryAmmo()								{ return true; }
	virtual bool		CanBeSelected()									{ return true; }

	virtual const char	*GetPanelName() { return "pda_panel"; }

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
	CTFWeaponPDA( const CTFWeaponPDA & ) {}
};

class CTFWeaponPDA_Engineer_Build : public CTFWeaponPDA
{
	DECLARE_CLASS( CTFWeaponPDA_Engineer_Build, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual const char	*GetPanelName() { return ""; }
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PDA_ENGINEER_BUILD; }

#ifdef GAME_DLL
	virtual void		Equip( CBaseCombatCharacter *pOwner );
	virtual void		Detach();

	void				DestroyBuildings( int iType = OBJ_LAST, int iMode = 0, bool bCheckForRemoteConstruction = false );
#endif

	bool				IsAirController( void ) { int iMakesJumpPads = 0; CALL_ATTRIB_HOOK_INT( iMakesJumpPads, set_teleporter_mode ); return iMakesJumpPads == 1; }
	bool				IsMiniDispenser(void) { int iMakesMiniDispenser = 0; CALL_ATTRIB_HOOK_INT(iMakesMiniDispenser, set_dispenser_mode); return iMakesMiniDispenser == 1; }
	bool				IsFlameSentry(void) { int iMakesFlameSentry = 0; CALL_ATTRIB_HOOK_INT(iMakesFlameSentry, set_sentry_mode); return iMakesFlameSentry == 1; }
	bool				CanRemoteDeploy( void ) { int iRemoteDeploy = 0; CALL_ATTRIB_HOOK_INT( iRemoteDeploy, pda_remote_deploy ); return iRemoteDeploy == 1; }
};

#ifdef CLIENT_DLL
extern ConVar tf_build_menu_controller_mode;
#endif

class CTFWeaponPDA_Engineer_Destroy : public CTFWeaponPDA
{
	DECLARE_CLASS( CTFWeaponPDA_Engineer_Destroy, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual const char	*GetPanelName() { return ""; }
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_PDA_ENGINEER_DESTROY; }

	virtual bool		VisibleInWeaponSelection( void )
	{
		if ( IsConsole()
#ifdef CLIENT_DLL
			|| tf_build_menu_controller_mode.GetBool() 
#endif 
			)
		{
			return false;
		}

		return BaseClass::VisibleInWeaponSelection();
	}
};

class CTFWeaponPDA_Spy : public CTFWeaponPDA
{
	DECLARE_CLASS( CTFWeaponPDA_Spy, CTFWeaponPDA );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual const char	*GetPanelName() { return ""; }
	virtual ETFWeaponID	GetWeaponID(void) const { return TF_WEAPON_PDA_SPY; }

#ifdef CLIENT_DLL
	virtual bool		Deploy( void );
#endif
	// Reload does nothing since reload key is used for switching disguises.
	virtual bool		Reload( void ) { return false; }

};

#endif // TF_WEAPON_PDA_H