//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "tf_powerup.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "filesystem.h"

//=============================================================================
float PackRatios[POWERUP_SIZES] =
{
	0.2,	// SMALL	//Intended value instead of 0.2. Workaround for ceiling math function done floating point with cast to int giving us 41 metal and other weird ammo counts.
	0.5,	// MEDIUM
	1.0,	// FULL
};

//=============================================================================
//
// CTF Powerup tables.
//
BEGIN_DATADESC( CTFPowerup )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "powerup_model" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableWithEffect", InputEnableWithEffect ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableWithEffect", InputDisableWithEffect ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RespawnNow", InputRespawnNow ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnRespawn, "OnRespawn" ),
	DEFINE_OUTPUT( m_outputOn15SecBeforeRespawn, "On15SecBeforeRespawn" ),
	DEFINE_OUTPUT( m_outputOnTeam1Touch, "OnTeam1Touch" ),
	DEFINE_OUTPUT( m_outputOnTeam2Touch, "OnTeam2Touch" ),
	DEFINE_OUTPUT( m_outputOnTeam3Touch, "OnTeam3Touch" ),
	DEFINE_OUTPUT( m_outputOnTeam4Touch, "OnTeam4Touch" ),

	DEFINE_THINKFUNC( RespawnThink ),

END_DATADESC();

//=============================================================================
//
// CTF Powerup functions.
//


CTFPowerup::CTFPowerup()
{
	m_bDisabled = false;
	m_bRespawning = false;
	m_flOwnerPickupEnableTime = 0.0f;

	m_bDropped = false;
	m_bFire15SecRemain = false;

	m_iszModel = NULL_STRING;

	UseClientSideAnimation();
}


void CTFPowerup::Precache( void )
{
	BaseClass::Precache();
	PrecacheParticleSystem( "ExplosionCore_buildings" );
}


void CTFPowerup::Spawn( void )
{
	BaseClass::Spawn();

	//SetOriginalSpawnOrigin( GetLocalOrigin() );
	//SetOriginalSpawnAngles( GetAbsAngles() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );

	if ( !m_bDropped )
	{
		SetDisabled( m_bDisabled );
	}

	ResetSequence( LookupSequence( "idle" ) );
}


CBaseEntity* CTFPowerup::Respawn( void )
{
	m_bRespawning = true;

	HideOnPickedUp();
	SetTouch( NULL );

	RemoveAllDecals(); //remove any decals

	// Set respawn time.
	SetRespawnTime( g_pGameRules->FlItemRespawnTime( this ) );

	if ( !m_bDisabled )
	{
		SetContextThink( &CTFPowerup::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
	}

	return this;
}


void CTFPowerup::Materialize( void )
{
	m_bRespawning = false;
	UnhideOnRespawn();
	SetTouch( &CItem::ItemTouch );

	m_outputOnRespawn.FireOutput( this, this );
	SetContextThink( NULL, 0, "RespawnThinkContext" );
}


void CTFPowerup::HideOnPickedUp( void )
{
	AddEffects( EF_NODRAW );
}


void CTFPowerup::UnhideOnRespawn( void )
{
	EmitSound( "Item.Materialize" );
	RemoveEffects( EF_NODRAW );
}


bool CTFPowerup::ValidTouch( CBasePlayer *pPlayer )
{
	if ( IsRespawning() )
	{
		return false;
	}

	// Is the item enabled?
	if ( IsDisabled() )
	{
		return false;
	}

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Team number and does it match?
	int iTeam = GetTeamNumber();
	if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
	{
		CTFPlayer* pTFPlayer = ToTFPlayer(pPlayer);
		if ( pTFPlayer && pTFPlayer->m_Shared.DisguiseFoolsTeam( GetTeamNumber() ) )
			return true;

		return false;
	}

	return true;
}


bool CTFPowerup::MyTouch( CBasePlayer *pPlayer )
{
	return false;
}


bool CTFPowerup::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	// Owner can't pick it up for some time after dropping it.
	if ( pPlayer == GetOwnerEntity() && gpGlobals->curtime < m_flOwnerPickupEnableTime )
		return false;

	return BaseClass::ItemCanBeTouchedByPlayer( pPlayer );
}


void CTFPowerup::SetRespawnTime( float flDelay )
{
	m_flRespawnTime = gpGlobals->curtime + flDelay;
	m_bFire15SecRemain = ( flDelay >= 15.0f );
}


void CTFPowerup::RespawnThink( void )
{
	if ( m_bFire15SecRemain && m_flRespawnTime - gpGlobals->curtime <= 15.0f )
	{
		m_bFire15SecRemain = false;
		m_outputOn15SecBeforeRespawn.FireOutput( this, this );
		OnIncomingSpawn();
	}

	if ( gpGlobals->curtime >= m_flRespawnTime )
	{
		Materialize();
	}
	else
	{
		SetContextThink( &CTFPowerup::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
	}
}


void CTFPowerup::DropSingleInstance( const Vector &vecVelocity, CBaseCombatCharacter *pOwner, float flOwnerPickupDelay, float flRestTime, float flRemoveTime /*= 30.0f*/ )
{
	SetOwnerEntity( pOwner );
	AddSpawnFlags( SF_NORESPAWN );
	m_bDropped = true;
	DispatchSpawn( this );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetAbsVelocity( vecVelocity );
	SetSolid( SOLID_BBOX );

	if ( flRestTime != 0.0f )
		ActivateWhenAtRest( flRestTime );

	m_flOwnerPickupEnableTime = gpGlobals->curtime + flOwnerPickupDelay;

	// Remove after 30 seconds.
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + flRemoveTime, "PowerupRemoveThink" );
}


void CTFPowerup::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}


void CTFPowerup::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}


void CTFPowerup::InputEnableWithEffect( inputdata_t &inputdata )
{
	DispatchParticleEffect( "ExplosionCore_buildings", GetAbsOrigin(), vec3_angle );
	SetDisabled( false );
}


void CTFPowerup::InputDisableWithEffect( inputdata_t &inputdata )
{
	DispatchParticleEffect( "ExplosionCore_buildings", GetAbsOrigin(), vec3_angle );
	SetDisabled( true );
}


bool CTFPowerup::IsDisabled( void )
{
	return m_bDisabled;
}


void CTFPowerup::InputToggle( inputdata_t &inputdata )
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


void CTFPowerup::SetDisabled( bool bDisabled )
{
	// No items in Instagib.
	m_bDisabled = bDisabled;

	if ( m_bDisabled )
	{
		AddEffects( EF_NODRAW );
		SetContextThink( NULL, 0, "RespawnThinkContext" );
	}
	else
	{
		RemoveEffects( EF_NODRAW );
		
		if ( m_bRespawning )
		{
			HideOnPickedUp();
			m_bFire15SecRemain = ( m_flRespawnTime - gpGlobals->curtime >= 15.0f );
			SetContextThink( &CTFPowerup::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
		}
	}
}


void CTFPowerup::InputRespawnNow( inputdata_t &inputdata )
{
	if ( m_bRespawning )
	{
		Materialize();
	}
}


void CTFPowerup::FireOutputsOnPickup( CBasePlayer *pPlayer )
{
	switch ( pPlayer->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_outputOnTeam1Touch.FireOutput( pPlayer, this );
		break;
	case TF_TEAM_BLUE:
		m_outputOnTeam2Touch.FireOutput( pPlayer, this );
		break;
	case TF_TEAM_GREEN:
		m_outputOnTeam3Touch.FireOutput( pPlayer, this );
		break;
	case TF_TEAM_YELLOW:
		m_outputOnTeam4Touch.FireOutput( pPlayer, this );
		break;
	}
}


const char *CTFPowerup::GetPowerupModel( void )
{
	if ( m_iszModel != NULL_STRING )
	{
		if ( g_pFullFileSystem->FileExists( STRING( m_iszModel ), "GAME" ) )
		{
			return ( STRING( m_iszModel ) );
		}
	}

	return GetDefaultPowerupModel();
}
