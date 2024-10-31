//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

ConVar tf2c_pistol_old_firerate( "tf2c_pistol_old_firerate", "0", FCVAR_REPLICATED, "Restore old semi-automatic pistol behavior." );

//=============================================================================
//
// Weapon Pistol tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol, DT_TFPistol )

BEGIN_NETWORK_TABLE_NOBASE( CTFPistol, DT_PistolLocalData )
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flSoonestPrimaryAttack ) ),
	SendPropBool( SENDINFO( m_bStock ) ),
#else
	RecvPropTime( RECVINFO( m_flSoonestPrimaryAttack ) ),
	RecvPropBool( RECVINFO( m_bStock ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFPistol, DT_TFPistol )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "PistolLocalData", 0, &REFERENCE_SEND_TABLE( DT_PistolLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "PistolLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PistolLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol, CTFPistol );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol );

CREATE_SIMPLE_WEAPON_TABLE( TFPistol_Scout, tf_weapon_pistol_scout );

//=============================================================================
//
// Weapon Pistol functions.
//

CTFPistol::CTFPistol()
{
	m_flSoonestPrimaryAttack = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Use the old pistol behavior if the cvar is on and we're stock
//-----------------------------------------------------------------------------
bool CTFPistol::UsingSemiAutoMode() const
{
	return ( tf2c_pistol_old_firerate.GetBool() && m_bStock );
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CTFPistol::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( UsingSemiAutoMode() )
	{
		CBasePlayer *pOwner = GetPlayerOwner();
		if ( !pOwner )
			return;

		if ( IsReloading() )
			return;

		// Allow a refire as fast as the player can click.
		if ( !( pOwner->m_nButtons & IN_ATTACK ) && m_flSoonestPrimaryAttack <= gpGlobals->curtime )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime;
		}
	}
}


void CTFPistol::Spawn( void )
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	CEconItemDefinition *pItemDef = GetItem()->GetStaticData();
	m_bStock = ( !pItemDef || pItemDef->baseitem );
#endif
}


float CTFPistol::GetFireRate( void )
{
	if ( UsingSemiAutoMode() )
	{
		return 0.25f;
	}

	return BaseClass::GetFireRate();
}


CBaseEntity *CTFPistol::FireProjectile( CTFPlayer *pPlayer )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime + 0.1f;

	return BaseClass::FireProjectile( pPlayer );
}
