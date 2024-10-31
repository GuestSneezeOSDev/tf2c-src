//=============================================================================
//
//  Purpose: Used for giving other entities the ability to glow.
//
//=============================================================================

#include "cbase.h"
#include "c_tf_glow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFGlow, DT_TFGlow, CTFGlow )
	RecvPropInt(RECVINFO( m_iMode ) ),
	RecvPropInt( RECVINFO( m_glowColor ), 0, RecvProxy_IntToColor32 ),
	RecvPropBool( RECVINFO( m_bDisabled ) ),
	RecvPropEHandle( RECVINFO( m_hTarget ) ),
END_RECV_TABLE()


C_TFGlow::C_TFGlow()
{
}


C_TFGlow::~C_TFGlow()
{
	delete pGlow;
}


void C_TFGlow::CreateGlow()
{
	if ( pGlow )
	{
		delete pGlow;
		pGlow = nullptr;
	}

	if ( m_bDisabled || !m_hTarget )
	{
		return;
	}

	Vector cvec;
	color32 c = m_glowColor.Get();
	cvec[0] = c.r * (1.0f/255.0f);
	cvec[1] = c.g * (1.0f/255.0f);
	cvec[2] = c.b * (1.0f/255.0f);
	float a = c.a * (1.0f/255.0f);
	
	int iMode = m_iMode.Get();
	bool bDrawWhenOccluded = ( iMode == 0 ) || ( iMode == 1 );
	bool bDrawWhenVisible = ( iMode == 0 ) || ( iMode == 2 );
	Assert( bDrawWhenOccluded || bDrawWhenVisible );

	pGlow = new CGlowObject( m_hTarget, cvec, a, bDrawWhenOccluded, bDrawWhenVisible );
}


void C_TFGlow::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );
	
	// this could avoid recreating the glow object on every update, but it
	// wouldn't be noticeably more efficient and it would add a ton of code here.
	CreateGlow();
}
