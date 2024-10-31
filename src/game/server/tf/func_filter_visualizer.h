//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_FILTERVISUALIZER_H
#define FUNC_FILTERVISUALIZER_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "modelentities.h"

//-----------------------------------------------------------------------------
// Purpose: Visualizes a respawn room to the enemy team
//-----------------------------------------------------------------------------
class CFuncFilterVisualizer : public CFuncBrush
{
	DECLARE_CLASS( CFuncFilterVisualizer, CFuncBrush );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void Activate( void );

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool 	PassesTriggerFilters( CBaseEntity *pOther );
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask,  const IHandleEntity *pEnt ) const;

	void SetActive( bool bActive );
	
	string_t	m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;
};

#endif // FUNC_FILTERVISUALIZER_H