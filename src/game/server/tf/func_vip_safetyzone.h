//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef FUNC_VIP_SAFETYZONE_H
#define FUNC_VIP_SAFETYZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

// Spawnflags
#define SF_TFESCAPE_WIN			(1<<0)
#define SF_TFESCAPE_NOINVULN	(1<<1)
#define SF_TFESCAPE_BLOCKABLE	(1<<2)

class CVIPSafetyZone : public CBaseTrigger, public TAutoList<CVIPSafetyZone>
{
public:
	DECLARE_CLASS( CVIPSafetyZone, CBaseTrigger );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	VIPTouch( CBaseEntity *pOther );

	inline bool	IsWin( void ) { return ( m_spawnflags & SF_TFESCAPE_WIN ) ? true : false; }
	inline bool	IsNoInvuln( void ) { return ( m_spawnflags & SF_TFESCAPE_NOINVULN ) ? true : false; }
	inline bool	IsBlockable( void ) { return ( m_spawnflags & SF_TFESCAPE_BLOCKABLE ) ? true : false; }
};

#endif // FUNC_VIP_SAFETYZONE_H
