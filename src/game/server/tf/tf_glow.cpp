//=============================================================================
//
//  Purpose: Used for giving other entities the ability to glow.
//
//=============================================================================

#include "cbase.h"
#include "tf_glow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTFGlow )
	DEFINE_KEYFIELD( m_iMode, FIELD_INTEGER, "Mode" ),
	DEFINE_KEYFIELD( m_glowColor, FIELD_COLOR32, "GlowColor" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFGlow, DT_TFGlow )
	SendPropInt( SENDINFO( m_glowColor ), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropEHandle( SENDINFO( m_hTarget ) ),
	SendPropInt( SENDINFO( m_iMode ), -1, SPROP_UNSIGNED ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_glow, CTFGlow );


void CTFGlow::Spawn()
{
	CBaseEntity *pEnt = gEntList.FindEntityByName( nullptr, m_target );
	if ( !pEnt ) 
	{
		Warning( "tf_glow: failed to find target %s\n", m_target.ToCStr() );
		UTIL_Remove( this );
		return;
	}

	m_hTarget = pEnt;
	m_hTarget.Get()->SetTransmitState( FL_EDICT_ALWAYS );

	if ( gEntList.FindEntityByName( pEnt, m_target ) )
	{
		Warning( "tf_glow: only one target is supported (%s)\n", m_target.ToCStr() );
	}
}


void CTFGlow::InputEnable( inputdata_t &inputdata )
{
	// The client will do all the work
	m_bDisabled = false; 
}


void CTFGlow::InputDisable( inputdata_t &inputdata )
{
	// Ditto ^
	m_bDisabled = true;
}


void CTFGlow::InputSetGlowColor( inputdata_t &inputdata )
{
	// Ditto ^^
	m_glowColor = inputdata.value.Color32();
}


int CTFGlow::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
