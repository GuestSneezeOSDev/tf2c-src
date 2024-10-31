//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Playermodelscale Zone.
//
//=============================================================================//
#ifndef FUNC_PLAYERMODELSCALE_ZONE_H
#define FUNC_PLAYERMODELSCALE_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//=============================================================================
//
// CTF Playermodelscale Trigger class.
//
class CPlayerModelScale : public CBaseTrigger
{
	DECLARE_CLASS( CPlayerModelScale, CBaseTrigger );

public:
	DECLARE_DATADESC();
	CPlayerModelScale()
	{
		m_flPlayerModelScale = 1.0f;
		m_flPlayerModelScaleDuration = 0.0f;
	}

	void		Spawn( void );
	// Return true if the specified entity is touching this zone
	void		StartTouch( CBaseEntity *pOther );
	float		GetPlayerModelScale( void ){ return m_flPlayerModelScale; }
	float		GetPlayerModelScaleDuration( void ){ return m_flPlayerModelScaleDuration; }

private:
	float		m_flPlayerModelScale;
	float		m_flPlayerModelScaleDuration;
};

#endif // FUNC_PLAYERMODELSCALE_ZONE_H












