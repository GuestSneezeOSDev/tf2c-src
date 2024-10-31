//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Regenerate Zone.
//
//=============================================================================//
#ifndef FUNC_REGENERATE_ZONE_H
#define FUNC_REGENERATE_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "props.h"

//=============================================================================
//
// CTF Regenerate Zone class.
//
class CRegenerateZone : public CBaseTrigger, public TAutoList<CRegenerateZone>
{
public:
	DECLARE_CLASS( CRegenerateZone, CBaseTrigger );
	DECLARE_DATADESC();

	CRegenerateZone();

	void	Spawn( void );
	void	Precache( void );
	void	Activate( void );
	void	Touch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );
	virtual void Regenerate( CTFPlayer *pPlayer );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

protected:
	CHandle<CDynamicProp>	m_hAssociatedModel;

private:
	bool					m_bDisabled;
	string_t				m_iszAssociatedModel;
};

class CRestockZone : public CRegenerateZone
{
public:
	DECLARE_CLASS( CRestockZone, CRegenerateZone );
	DECLARE_DATADESC();

	CRestockZone();

	virtual void Regenerate( CTFPlayer *pPlayer );

private:
	bool m_bRestoreHealth;
	bool m_bRestoreAmmo;
};

#endif // FUNC_REGENERATE_ZONE_H
