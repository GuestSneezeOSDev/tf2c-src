//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF ChangeClass Zone.
//
//=============================================================================//

#include "cbase.h"
#include "viewport_panel_names.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "func_changeclass.h"

LINK_ENTITY_TO_CLASS( func_changeclass, CChangeClassZone );

#define TF_CHANGECLASS_SOUND				"ChangeClass.Touch"

//=============================================================================
//
// CTF ChangeClass Zone functions.
//


CChangeClassZone::CChangeClassZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CChangeClassZone::Spawn( void )
{
	Precache();
	InitTrigger();
	SetTouch( &CChangeClassZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CChangeClassZone::Precache( void )
{
	PrecacheScriptSound( TF_CHANGECLASS_SOUND );
}


void CChangeClassZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer )
		{
			int iTeam = GetTeamNumber();
			if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
				return;

			// bring up the player's changeclass menu
			CCommand args;
			args.Tokenize( "changeclass" );
			pPlayer->ClientCommand( args );

			CPASAttenuationFilter filter( pOther, TF_CHANGECLASS_SOUND );
			EmitSound( filter, pOther->entindex(), TF_CHANGECLASS_SOUND );
		}
	}
}


void CChangeClassZone::EndTouch( CBaseEntity *pOther )
{

}


void CChangeClassZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}


void CChangeClassZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}


bool CChangeClassZone::IsDisabled( void )
{
	return m_bDisabled;
}


void CChangeClassZone::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}


void CChangeClassZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}
