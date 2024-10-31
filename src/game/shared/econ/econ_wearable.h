//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#ifndef ECON_WEARABLE_H
#define ECON_WEARABLE_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

#define MAX_WEARABLES_SENT_FROM_SERVER	5
#define PARTICLE_MODIFY_STRING_SIZE		128

#if defined( CLIENT_DLL )
#define CEconWearable C_EconWearable
#endif


class CEconWearable : public CEconEntity, public IHasOwner
{
	DECLARE_CLASS( CEconWearable, CEconEntity );
	DECLARE_NETWORKCLASS();

public:
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	GiveTo( CBaseEntity *pEntity );
	virtual void	RemoveFrom( CBaseEntity *pEntity );
						 
#ifdef GAME_DLL			 
	virtual void	Equip( CBasePlayer *pPlayer );
	virtual void	UnEquip( CBasePlayer *pPlayer );
#else					 
	virtual	ShadowType_t	ShadowCastType( void );
	virtual bool			ShouldDraw( void );
#endif					 
						 
	static void		UpdateWearableBodygroups( CBasePlayer *pOwner, bool bForce = false );
						 
	virtual bool	IsWearable( void ) { return true; }
	virtual bool	IsViewModelWearable( void ) const { return false; }

	// IHasOwner
	virtual CBaseEntity *GetOwnerViaInterface( void ) { return GetOwnerEntity(); }

};
#endif