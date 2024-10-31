//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Award Achievement Trigger.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "func_playermodelscale.h"

LINK_ENTITY_TO_CLASS( func_playermodelscale, CPlayerModelScale );

//=============================================================================
//
// CTF Award Achievement Trigger functions.
//

BEGIN_DATADESC( CPlayerModelScale )
	DEFINE_KEYFIELD( m_flPlayerModelScale, FIELD_FLOAT, "playermodelscale" ),
	DEFINE_KEYFIELD( m_flPlayerModelScaleDuration, FIELD_FLOAT, "playermodelscale_duration" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CPlayerModelScale::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL ); // so we can keep track of who is touching us
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching this zone
//-----------------------------------------------------------------------------
void CPlayerModelScale::StartTouch( CBaseEntity *pEntity )
{
	BaseClass::StartTouch( pEntity );

	CTFPlayer* pTFPlayer = ToTFPlayer( pEntity );
	if( !pTFPlayer )
		return;

	pTFPlayer->SetModelScale( m_flPlayerModelScale, m_flPlayerModelScaleDuration );
}