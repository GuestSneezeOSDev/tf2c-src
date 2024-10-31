//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "func_forcefield.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST( CFuncForceField, DT_FuncForceField )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( func_forcefield, CFuncForceField );


void CFuncForceField::Spawn( void )
{
	BaseClass::Spawn();
	SetActive( IsOn() );
}


int CFuncForceField::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}


bool CFuncForceField::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Always open in post round time.
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return false;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch ( GetTeamNumber() )
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


void CFuncForceField::TurnOn( void )
{
	BaseClass::TurnOn();
	SetActive( true );
}


void CFuncForceField::TurnOff( void )
{
	BaseClass::TurnOff();
	SetActive( false );
}


void CFuncForceField::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Our ShouldCollide() will make
		// us non-solid to members of the same team.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );
	}
}
