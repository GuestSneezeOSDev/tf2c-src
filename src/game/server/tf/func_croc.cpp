//========= Copyright © 1996-2016, Valve LLC, All rights reserved. ============
//
//	Func Crocodile
//
//=============================================================================

#include "cbase.h"
#include "func_croc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CROC_MODEL "models/props_island/crocodile/crocodile.mdl"

BEGIN_DATADESC( CFuncCroc )

	// Outputs
	DEFINE_OUTPUT( m_OnEat, "OnEat" ),
	DEFINE_OUTPUT( m_OnEatRed, "OnEatRed" ),
	DEFINE_OUTPUT( m_OnEatBlue, "OnEatBlue" ),
	DEFINE_OUTPUT( m_OnEatGreen, "OnEatGreen" ),
	DEFINE_OUTPUT( m_OnEatYellow, "OnEatYellow" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_croc, CFuncCroc );


CFuncCroc::CFuncCroc()
{
}


void CFuncCroc::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	AddEffects( EF_NODRAW );
	
	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );
}


void CFuncCroc::Precache( void )
{
	PrecacheModel( CROC_MODEL );
}


void CFuncCroc::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled || !pOther )
		return;

	BaseClass::StartTouch( pOther );

	if ( IsTouching( pOther ) && !pOther->InSameTeam( this ) )
	{
		FireOutputs( ToTFPlayer( pOther ) );

		pOther->CalcAbsolutePosition();
		CEntityCroc *pCroc = (CEntityCroc *)Create( "entity_croc", pOther->GetAbsOrigin(), pOther->GetAbsAngles(), this );
		if ( pCroc )
		{
			pCroc->InitCroc();
		}

		CTakeDamageInfo info( this, this, 1000, DMG_SLASH | DMG_ALWAYSGIB, TF_DMG_CUSTOM_CROC );
		pOther->TakeDamage( info );
	}
}


void CFuncCroc::FireOutputs( CTFPlayer *pActivator )
{
	if ( !pActivator )
		return;

	m_OnEat.FireOutput( pActivator, this );

	switch ( pActivator->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_OnEatRed.FireOutput( pActivator, this );
		break;
	case TF_TEAM_BLUE:
		m_OnEatBlue.FireOutput( pActivator, this );
		break;
	case TF_TEAM_GREEN:
		m_OnEatGreen.FireOutput( pActivator, this );
		break;
	case TF_TEAM_YELLOW:
		m_OnEatYellow.FireOutput( pActivator, this );
		break;
	}
}


int CFuncCroc::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncCroc::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( m_bDisabled )
	{
		if ( GetTeamNumber() != TEAM_UNASSIGNED )
		{
			// Only transmit to enemy players
			CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
			if ( pRecipientEntity->GetTeamNumber() > LAST_SHARED_TEAM && !InSameTeam( pRecipientEntity ) )
				return FL_EDICT_ALWAYS;
		}
	}

	return FL_EDICT_DONTSEND;
}


bool CFuncCroc::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;

		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_GREEN:
			if ( !( contentsMask & CONTENTS_GREENTEAM ) )
				return false;
			break;

		case TF_TEAM_YELLOW:
			if ( !( contentsMask & CONTENTS_YELLOWTEAM ) )
				return false;
			break;
		}

		return true;
	}

	return false;
}

BEGIN_DATADESC( CEntityCroc )

	// Function Pointers
	DEFINE_FUNCTION( Think ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_croc, CEntityCroc );


void CEntityCroc::InitCroc( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	SetModel( CROC_MODEL );
	UseClientSideAnimation();

	ResetSequence( LookupSequence( "ref" ) );
	SetSequence( LookupSequence( "attack" ) );

	CrocAttack();
}


void CEntityCroc::Think( void )
{
	UTIL_Remove( this );

	SetThink( NULL );
	SetNextThink( gpGlobals->curtime );
}


void CEntityCroc::CrocAttack( void )
{
	SetTouch( NULL );
	SetThink( &CEntityCroc::Think );
	SetNextThink( gpGlobals->curtime + SequenceDuration() );
}