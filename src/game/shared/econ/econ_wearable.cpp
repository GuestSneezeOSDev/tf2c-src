//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#include "cbase.h"
#include "econ_wearable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED( EconWearable, DT_EconWearable )

BEGIN_NETWORK_TABLE( CEconWearable, DT_EconWearable )
END_NETWORK_TABLE()


void CEconWearable::Spawn( void )
{
	InitializeAttributes();

	Precache();

	if ( GetItem()->GetPlayerDisplayModel() )
	{
		SetModel( GetItem()->GetPlayerDisplayModel() );
	}

	BaseClass::Spawn();

	AddEffects( EF_BONEMERGE_FASTCULL );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	SetBlocksLOS( false );
}


void CEconWearable::Precache( void )
{
	BaseClass::Precache();

	if ( GetItem()->GetPlayerDisplayModel() )
	{
		PrecacheModel( GetItem()->GetPlayerDisplayModel() );
	}
}


void CEconWearable::GiveTo( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );
	if ( pPlayer )
	{
		pPlayer->EquipWearable( this );
	}
#endif
}

void CEconWearable::RemoveFrom( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );

	if ( pPlayer )
	{
		pPlayer->RemoveWearable( this );
	}
#endif
}

#ifdef GAME_DLL

void CEconWearable::Equip( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		CBaseEntity *pToFollow = pPlayer;
		if ( IsViewModelWearable() )
			pToFollow = pPlayer->GetViewModel();

		FollowEntity( pToFollow, true );
		SetOwnerEntity( pPlayer );
		ChangeTeam( pPlayer->GetTeamNumber() );

		ReapplyProvision();
	}
}


void CEconWearable::UnEquip( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		StopFollowingEntity();

		SetOwnerEntity( NULL );
		ReapplyProvision();
	}
}
#else

ShadowType_t CEconWearable::ShadowCastType( void )
{
	if ( ShouldDraw() )
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;

	return SHADOWS_NONE;
}


bool CEconWearable::ShouldDraw( void )
{
	// We don't have a reason to be visible if we have no owner.
	CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
	if ( !pOwner || !pOwner->ShouldDrawThisPlayer() )
		return false;

	if ( !pOwner->IsAlive() )
		return false;

	return BaseClass::ShouldDraw();
}
#endif


void CEconWearable::UpdateWearableBodygroups( CBasePlayer *pPlayer, bool bForce /*= false*/ )
{
	// Assume that pPlayer is a valid pointer.
	CEconWearable *pWearable;
	for ( int i = 0, c = pPlayer->GetNumWearables(); i < c; ++i )
	{
		pWearable = pPlayer->GetWearable( i );
		if ( !pWearable || pWearable->IsDynamicModelLoading() )
			continue;

		pWearable->UpdateBodygroups( pPlayer, bForce );
	}
}
