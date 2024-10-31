//====== Copyright Valve Corporation, All rights reserved. =======//
//
// Purpose: A modified game_player_equip that works with TF2C
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_wearable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
#define SF_TFEQUIP_CLEARWEAPONS		(1<<0)
#define SF_TFEQUIP_CLEARPDA			(1<<1)

class CTFPlayerEquip : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CTFPlayerEquip, CBaseEntity );

	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void		InputEquip( inputdata_t &inputdata );
	void		InputEquipAll( inputdata_t &inputdata );
	void		InputSetWeapon1( inputdata_t &inputdata );
	void		InputSetWeapon2( inputdata_t &inputdata );
	void		InputSetWeapon3( inputdata_t &inputdata );

	inline bool	ClearWeapons( void ) { return ( m_spawnflags & SF_TFEQUIP_CLEARWEAPONS ) ? true : false; }
	inline bool	ClearWeaponsPDA( void ) { return ( m_spawnflags & SF_TFEQUIP_CLEARPDA ) ? true : false; }

private:

	void		EquipPlayer( CBaseEntity *pPlayer );
	int			m_iWeaponNumber[TF_PLAYER_WEAPON_COUNT];

};

LINK_ENTITY_TO_CLASS( tf_player_equip, CTFPlayerEquip );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CTFPlayerEquip )

DEFINE_KEYFIELD( m_iWeaponNumber[0], FIELD_INTEGER, "weapon1" ),
DEFINE_KEYFIELD( m_iWeaponNumber[1], FIELD_INTEGER, "weapon2" ),
DEFINE_KEYFIELD( m_iWeaponNumber[2], FIELD_INTEGER, "weapon3" ),

DEFINE_INPUTFUNC( FIELD_VOID, "Equip", InputEquip ),
DEFINE_INPUTFUNC( FIELD_VOID, "EquipAll", InputEquipAll ),

DEFINE_INPUTFUNC( FIELD_INTEGER, "SetWeapon1", InputSetWeapon1 ),
DEFINE_INPUTFUNC( FIELD_INTEGER, "SetWeapon2", InputSetWeapon2 ),
DEFINE_INPUTFUNC( FIELD_INTEGER, "SetWeapon3", InputSetWeapon3 ),

END_DATADESC()

void CTFPlayerEquip::EquipPlayer( CBaseEntity *pEntity )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );

	if ( !pTFPlayer )
		return;

	if ( ClearWeapons() )
	{
		for ( int i = 0; i < TF_FIRST_COSMETIC_SLOT; i++ )
		{
			if ( i >= TF_LOADOUT_SLOT_BUILDING && !ClearWeaponsPDA() )
				continue;

			CEconEntity *pEntity = pTFPlayer->GetEntityForLoadoutSlot( (ETFLoadoutSlot)i );

			if ( pEntity )
			{
				if ( pEntity->IsBaseCombatWeapon() )
				{
					CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( pEntity );
					pWeapon->UnEquip();
				}
				else if ( pEntity->IsWearable() )
				{
					CEconWearable *pWearable = static_cast<CEconWearable *>( pEntity );
					pTFPlayer->RemoveWearable( pWearable );
				}
				else
				{
					DevWarning( 1, "Player had unknown entity in loadout slot %d.", i );
					UTIL_Remove( pEntity );
				}
			}
		}
	}

	for ( int i = 0; i < 3; i++ )
	{
		int iItemID = m_iWeaponNumber[i];

		if ( iItemID < 0 )
			continue;

		CEconItemView econItem( iItemID );
		CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemID );
		if ( !pItemDef )
			continue;

		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		ETFLoadoutSlot iSlot = pItemDef->GetLoadoutSlot( iClass );
		CEconEntity *pEntity = pTFPlayer->GetEntityForLoadoutSlot( iSlot );

		if ( pEntity )
		{
			if ( pEntity->IsBaseCombatWeapon() )
			{
				CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( pEntity );
				pWeapon->UnEquip();
			}
			else if ( pEntity->IsWearable() )
			{
				CEconWearable *pWearable = static_cast<CEconWearable *>( pEntity );
				pTFPlayer->RemoveWearable( pWearable );
			}
			else
			{
				DevWarning( 1, "Player had unknown entity in loadout slot %d.", iSlot );
				UTIL_Remove( pEntity );
			}
		}

		const char *pszClassname = pItemDef->item_class;
		pTFPlayer->GiveNamedItem( pszClassname, 0, &econItem, TF_GIVEAMMO_MAX );
	}
}

void CTFPlayerEquip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	EquipPlayer( pActivator );
}

void CTFPlayerEquip::InputEquip( inputdata_t &inputdata )
{
	EquipPlayer( inputdata.pActivator );
}

void CTFPlayerEquip::InputEquipAll( inputdata_t &inputdata )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		EquipPlayer( UTIL_PlayerByIndex( i ) );
	}
}

void CTFPlayerEquip::InputSetWeapon1( inputdata_t &inputdata )
{
	m_iWeaponNumber[0] = inputdata.value.Int();
}

void CTFPlayerEquip::InputSetWeapon2( inputdata_t &inputdata )
{
	m_iWeaponNumber[1] = inputdata.value.Int();
}

void CTFPlayerEquip::InputSetWeapon3( inputdata_t &inputdata )
{
	m_iWeaponNumber[2] = inputdata.value.Int();
}
