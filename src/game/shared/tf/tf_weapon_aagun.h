//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF2C Heavy Anti-Aircraft / Flak Cannon
//
//=============================================================================//
#ifndef TF_WEAPON_AAGUN_H
#define TF_WEAPON_AAGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_minigun.h"

#if defined( CLIENT_DLL )
#define CTFAAGun C_TFAAGun
#endif

//=============================================================================
//
// AA Gun class.
//
class CTFAAGun : public CTFMinigun
{
public:

	DECLARE_CLASS( CTFAAGun, CTFMinigun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFAAGun() {}

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_AAGUN; }

	virtual void	PlayWeaponShootSound( void );
	virtual void	GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bDoPassthroughCheck = false );
	virtual void	HandleFireOnEmpty( void ) OVERRIDE;
	virtual bool	SendWeaponAnim( int iActivity ) OVERRIDE;
	virtual void	WindDown( void ) OVERRIDE;
	virtual void	SharedAttack(void) OVERRIDE;

protected:
#ifdef CLIENT_DLL
	virtual void		Simulate( void );
	virtual void		WeaponSoundUpdate(void);
#endif

private:
	CTFAAGun( const CTFAAGun & );

#ifdef GAME_DLL
#else
	void StartBrassEffect() {};
	void StopBrassEffect() {};
	void HandleBrassEffect() {};

	void StartMuzzleEffect() {};
	void StopMuzzleEffect() {};
	void HandleMuzzleEffect() {};
#endif
};

#endif // TF_WEAPON_AAGUN_H
