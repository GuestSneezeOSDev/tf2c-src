//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Regenerate Zone.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "func_regenerate.h"
#include "tf_gamerules.h"
#include "eventqueue.h"

LINK_ENTITY_TO_CLASS( func_regenerate, CRegenerateZone );

#define TF_REGENERATE_SOUND				"Regenerate.Touch"
#define TF_REGENERATE_NEXT_USE_TIME		3.0f

//=============================================================================
//
// CTF Regenerate Zone tables.
//

BEGIN_DATADESC( CRegenerateZone )
	DEFINE_FIELD( m_hAssociatedModel, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszAssociatedModel, FIELD_STRING, "associatedmodel" ),

	// Functions.
	DEFINE_ENTITYFUNC( Touch ),
END_DATADESC();

//=============================================================================
//
// CTF Regenerate Zone functions.
//


CRegenerateZone::CRegenerateZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CRegenerateZone::Spawn( void )
{
	Precache();
	InitTrigger();
	SetTouch( &CRegenerateZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CRegenerateZone::Precache( void )
{
	PrecacheScriptSound( TF_REGENERATE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CRegenerateZone::Activate( void )
{
	BaseClass::Activate();

	if ( m_iszAssociatedModel != NULL_STRING )
	{
		CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, STRING( m_iszAssociatedModel ) );
		if ( !pEnt )
		{
			Warning( "%s(%s) unable to find associated model named '%s'!\n", GetClassname(), GetDebugName(), STRING( m_iszAssociatedModel ) );
		}
		else
		{
			m_hAssociatedModel = dynamic_cast<CDynamicProp *>( pEnt );
			if ( !m_hAssociatedModel )
			{
				Warning( "%s(%s) tried to use associated model named '%s', but it isn't a dynamic prop!\n", GetClassname(), GetDebugName(), STRING( m_iszAssociatedModel ) );
			}
		}	
	}
	else
	{
		Warning( "%s(%s) has no associated model!\n", GetClassname(), GetDebugName() );
	}
}


void CRegenerateZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer )
		{
			if ( pPlayer->GetNextRegenTime() > gpGlobals->curtime )
				return;

			if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
			{
				int iTeam = GetTeamNumber();
				if ( iTeam && pPlayer->GetTeamNumber() != iTeam )
					return;
			}
			// No health for the losing team, but all zones work for the winning team.
			else if ( TFGameRules()->GetWinningTeam() != pPlayer->GetTeamNumber() )
				return;

			if ( TFGameRules()->InStalemate() )
				return;

			Regenerate( pPlayer );
		}
	}
}


void CRegenerateZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}


void CRegenerateZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}


bool CRegenerateZone::IsDisabled( void )
{
	return m_bDisabled;
}


void CRegenerateZone::InputToggle( inputdata_t &inputdata )
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


void CRegenerateZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}


void CRegenerateZone::Regenerate( CTFPlayer *pPlayer )
{
	pPlayer->Regenerate();
	pPlayer->SetNextRegenTime( gpGlobals->curtime + TF_REGENERATE_NEXT_USE_TIME );

	CSingleUserRecipientFilter filter( pPlayer );
	EmitSound( filter, pPlayer->entindex(), TF_REGENERATE_SOUND );

	if ( m_hAssociatedModel )
	{
		variant_t tmpVar;
		tmpVar.SetString( MAKE_STRING( "open" ) );
		m_hAssociatedModel->AcceptInput( "SetAnimation", this, this, tmpVar, 0 );

		tmpVar.SetString( MAKE_STRING( "close" ) );
		g_EventQueue.AddEvent( m_hAssociatedModel, "SetAnimation", tmpVar, TF_REGENERATE_NEXT_USE_TIME - 1.0, this, this );
	}
}

LINK_ENTITY_TO_CLASS( func_restock, CRestockZone );

BEGIN_DATADESC( CRestockZone )
	DEFINE_KEYFIELD( m_bRestoreHealth, FIELD_BOOLEAN, "RestoreHealth" ),
	DEFINE_KEYFIELD( m_bRestoreAmmo, FIELD_BOOLEAN, "RestoreAmmo" ),
END_DATADESC()

CRestockZone::CRestockZone()
{
	m_bRestoreHealth = false;
	m_bRestoreAmmo = false;
}


void CRestockZone::Regenerate( CTFPlayer *pPlayer )
{
	pPlayer->Restock( m_bRestoreHealth, m_bRestoreAmmo );
	pPlayer->SetNextRegenTime( gpGlobals->curtime + TF_REGENERATE_NEXT_USE_TIME );

	CSingleUserRecipientFilter filter( pPlayer );
	EmitSound( filter, pPlayer->entindex(), TF_REGENERATE_SOUND );

	if ( m_hAssociatedModel )
	{
		variant_t tmpVar;
		tmpVar.SetString( MAKE_STRING( "open" ) );
		m_hAssociatedModel->AcceptInput( "SetAnimation", this, this, tmpVar, 0 );

		tmpVar.SetString( MAKE_STRING( "close" ) );
		g_EventQueue.AddEvent( m_hAssociatedModel, "SetAnimation", tmpVar, TF_REGENERATE_NEXT_USE_TIME - 1.0, this, this );
	}
}
