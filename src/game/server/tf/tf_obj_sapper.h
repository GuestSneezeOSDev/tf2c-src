//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Upgrade that damages the object over time
//
//=============================================================================//

#ifndef TF_OBJ_SAPPER_H
#define TF_OBJ_SAPPER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj_baseupgrade_shared.h"

enum SapperMode_t
{
	SAPPER_MODE_PLACED = 0,
	SAPPER_MODE_PLACEMENT
};

// ------------------------------------------------------------------------ //
// Sapper upgrade
// ------------------------------------------------------------------------ //
class CObjectSapper : public CBaseObjectUpgrade
{
	DECLARE_CLASS( CObjectSapper, CBaseObjectUpgrade );

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CObjectSapper();

	virtual void	Spawn();
	virtual void	Precache();
	virtual bool	IsHostileUpgrade( void ) const { return true; }
	virtual void	FinishedBuilding( void );
	virtual void	SetupAttachedVersion( void );
	const char		*GetSapperModelName( SapperMode_t iModelType );
	virtual void	DetachObjectFromObject( void );
	virtual void	UpdateOnRemove( void );
	virtual void	OnGoActive( void );

	void			SapperThink( void );
	virtual int		GetBaseHealth( void ) const;
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );

	virtual int		CalculateTeamBalanceScore() { return 10; }

private:
	float m_flSapperDamageAccumulator;
	float m_flLastThinkTime;
	float m_flSappingStartTime;
};

#endif // TF_OBJ_SAPPER_H
