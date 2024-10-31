//=============================================================================
//
//  Purpose: Used for giving other entities the ability to glow.
//
//=============================================================================

#ifndef TF_GLOW_H
#define TF_GLOW_H

#ifdef _WIN32
#pragma once
#endif
#include "baseentity.h"

//-----------------------------------------------------------------------------
// CTFGlow - Server implementation
//-----------------------------------------------------------------------------
class CTFGlow : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFGlow, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn() OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE;
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetGlowColor( inputdata_t &inputdata );

private:
	CNetworkVar( int, m_iMode );
	CNetworkVar( color32, m_glowColor );
	CNetworkVar( bool, m_bDisabled );
	CNetworkHandle( CBaseEntity, m_hTarget );
};

#endif // TF_GLOW_H