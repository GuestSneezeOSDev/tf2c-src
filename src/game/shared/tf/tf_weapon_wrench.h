//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_WRENCH_H
#define TF_WEAPON_WRENCH_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFWrench C_TFWrench
#endif

//=============================================================================
//
// Wrench class.
//

class CTFWrench : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFWrench, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWrench();
	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_WRENCH; }

	virtual void		Smack( void );
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage );

#ifdef GAME_DLL
	virtual void		OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector vecHitPos );
	virtual void		Equip(CBaseCombatCharacter* pOwner);
	virtual void		Detach();
	virtual void		DestroyBuildings(int iType = OBJ_LAST, int iMode = 0, bool bCheckForRemoteConstruction = false);
#endif

	float				GetConstructionValue( void );
	float				GetRepairValue( void );

	bool				IsAirController(void) { int iMakesJumpPads = 0; CALL_ATTRIB_HOOK_INT(iMakesJumpPads, set_teleporter_mode); return iMakesJumpPads == 1; }
	bool				IsMiniDispenser(void) { int iMakesMiniDispenser = 0; CALL_ATTRIB_HOOK_INT(iMakesMiniDispenser, set_dispenser_mode); return iMakesMiniDispenser == 1; }
	bool				IsFlameSentry(void) { int iMakesFlameSentry = 0; CALL_ATTRIB_HOOK_INT(iMakesFlameSentry, set_sentry_mode); return iMakesFlameSentry == 1; }

#ifdef GAME_DLL
	// TODO: maybe reevaluate or rework this; live TF2 has TF_WEAPON_WRENCH never increase the nav mesh combat level,
	// presumably because players will mostly be using the wrench to hit buildings rather than to attack enemies
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE { return false; }
#endif

private:
	CTFWrench( const CTFWrench & ) {}
};

#endif // TF_WEAPON_WRENCH_H
