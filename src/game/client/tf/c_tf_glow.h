//=============================================================================
//
//  Purpose: Used for giving other entities the ability to glow.
//
//=============================================================================

#ifndef C_TF_GLOW_H
#define C_TF_GLOW_H

#ifdef _WIN32
#pragma once
#endif
#include "c_baseentity.h"

//-----------------------------------------------------------------------------
// C_TFGlow - Client implementation
//-----------------------------------------------------------------------------
class C_TFGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFGlow();
	virtual ~C_TFGlow();
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

private:
	void CreateGlow();

	CGlowObject *pGlow;
	CNetworkVar( int, m_iMode );
	CNetworkVar( color32, m_glowColor );
	CNetworkVar( bool, m_bDisabled );
	CNetworkHandle( CBaseEntity, m_hTarget );
};

#endif // C_TF_GLOW_H