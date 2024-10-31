//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pda.h"
#include "in_buttons.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "vguiscreen.h"
#include "tf_obj_teleporter.h"
#include "tf_weapon_builder.h"
// Client specific.
#else
#include "c_tf_player.h"
#include <igameevents.h>
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponPDA, DT_TFWeaponPDA )

BEGIN_NETWORK_TABLE( CTFWeaponPDA, DT_TFWeaponPDA )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponPDA )
END_PREDICTION_DATA()

CTFWeaponPDA::CTFWeaponPDA()
{
}

void CTFWeaponPDA::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Switch away from this PDA.
//-----------------------------------------------------------------------------
void CTFWeaponPDA::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	pOwner->SelectLastItem();
}

//-----------------------------------------------------------------------------
// Purpose: Activate class skill.
//-----------------------------------------------------------------------------
void CTFWeaponPDA::SecondaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Precache the VGUI screen.
//-----------------------------------------------------------------------------
void CTFWeaponPDA::Precache()
{
	BaseClass::Precache();
	PrecacheVGuiScreen( GetPanelName() );
}

//-----------------------------------------------------------------------------
// Purpose: Gets info about the control panels.
//-----------------------------------------------------------------------------
void CTFWeaponPDA::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = GetPanelName();
}
#else
//-----------------------------------------------------------------------------
// Purpose: No bobbing.
//-----------------------------------------------------------------------------
float CTFWeaponPDA::CalcViewmodelBob( void )
{
	return BaseClass::CalcViewmodelBob();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponPDA::ShouldShowControlPanels( void )
{
	return true;
}

//==============================

CREATE_SIMPLE_WEAPON_TABLE( TFWeaponPDA_Engineer_Build, tf_weapon_pda_engineer_build )
CREATE_SIMPLE_WEAPON_TABLE( TFWeaponPDA_Engineer_Destroy, tf_weapon_pda_engineer_destroy )
CREATE_SIMPLE_WEAPON_TABLE( TFWeaponPDA_Spy, tf_weapon_pda_spy )

#ifdef CLIENT_DLL

bool CTFWeaponPDA_Spy::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();
	if ( bDeploy )
	{
		// Let the spy pda menu know to reset.
		IGameEvent *event = gameeventmanager->CreateEvent( "spy_pda_reset" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	return bDeploy;
}
#else
//-----------------------------------------------------------------------------
// Purpose: Destroy certain buildings depending on the PDA's attributes.
//-----------------------------------------------------------------------------
void CTFWeaponPDA_Engineer_Build::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip(pOwner);

	// Just wreck the teleporters.
	if ( IsAirController() )
	{
		DestroyBuildings( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
		DestroyBuildings( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );

		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_TELEPORTER)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_JUMPPAD);
				pBuilder->SetObjectMode((TELEPORTER_TYPE_ENTRANCE) ? JUMPPAD_TYPE_A : JUMPPAD_TYPE_B);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}
	// Just wreck the dispensers.
	if (IsMiniDispenser())
	{
		DestroyBuildings(OBJ_DISPENSER);

		CTFPlayer* pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder* pBuilder = dynamic_cast<CTFWeaponBuilder*>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_DISPENSER)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_MINIDISPENSER);
				pBuilder->SetObjectMode(OBJECT_MODE_NONE);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}

	// Just wreck the sentries
	if (IsFlameSentry())
	{
		DestroyBuildings(OBJ_SENTRYGUN);

		CTFPlayer* pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder* pBuilder = dynamic_cast<CTFWeaponBuilder*>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_SENTRYGUN)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_FLAMESENTRY);
				pBuilder->SetObjectMode(OBJECT_MODE_NONE);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: Destroy certain buildings depending on the PDA's attributes.
//-----------------------------------------------------------------------------
void CTFWeaponPDA_Engineer_Build::Detach( void )
{
	// Blow everything up if they're in the remote construction state.
	if ( CanRemoteDeploy() )
	{
		DestroyBuildings( OBJ_LAST, 0, true );
		DestroyBuildings( OBJ_TELEPORTER, 1, true );
	}

	// Just wreck the teleporters.
	if ( IsAirController() )
	{
		DestroyBuildings( OBJ_JUMPPAD, JUMPPAD_TYPE_A );
		DestroyBuildings( OBJ_JUMPPAD, JUMPPAD_TYPE_B );

		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_JUMPPAD)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_TELEPORTER);
				pBuilder->SetObjectMode((JUMPPAD_TYPE_A) ? TELEPORTER_TYPE_ENTRANCE : TELEPORTER_TYPE_EXIT);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}

	// Just wreck the mini dispensers.
	if (IsMiniDispenser())
	{
		DestroyBuildings(OBJ_MINIDISPENSER);

		CTFPlayer* pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder* pBuilder = dynamic_cast<CTFWeaponBuilder*>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_MINIDISPENSER)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_DISPENSER);
				pBuilder->SetObjectMode(OBJECT_MODE_NONE);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}

	// Just wreck the flame sentries.
	if (IsFlameSentry())
	{
		DestroyBuildings(OBJ_FLAMESENTRY);

		CTFPlayer* pPlayer = GetTFPlayerOwner();
		if (pPlayer)
		{
			CTFWeaponBuilder* pBuilder = dynamic_cast<CTFWeaponBuilder*>(pPlayer->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			if (pBuilder && pBuilder->GetSubType() == OBJ_FLAMESENTRY)
			{
				pBuilder->StopPlacement();
				pBuilder->SetSubType(OBJ_SENTRYGUN);
				pBuilder->SetObjectMode(OBJECT_MODE_NONE);
				if (pBuilder == pPlayer->GetActiveTFWeapon())
					pBuilder->SwitchOwnersWeaponToLast();
			}
		}
	}

	BaseClass::Detach();
}

//-----------------------------------------------------------------------------
// Purpose: Wreck everything.
//-----------------------------------------------------------------------------
void CTFWeaponPDA_Engineer_Build::DestroyBuildings( int iType /*= OBJ_LAST*/, int iMode /*= 0*/, bool bCheckForRemoteConstruction /*= false*/ )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		// Destroy everything.
		if ( iType >= OBJ_LAST )
		{
			for ( int i = 0; i < OBJ_LAST; i++ )
			{
				CBaseObject *pBuilding = pPlayer->GetObjectOfType( i, iMode );
				if ( pBuilding )
				{
					if ( bCheckForRemoteConstruction )
					{
						if ( pBuilding->InRemoteConstruction() )
						{
							// Just have them start building if they're waiting for a remote signal.
							pBuilding->SetRemoteConstruction( false );
						}

						continue;
					}

					pBuilding->DetonateObject();
				}
			}
		}
		// Destroy this specific building.
		else
		{
			CBaseObject *pBuilding = pPlayer->GetObjectOfType( iType, iMode );
			if ( pBuilding )
			{
				if ( bCheckForRemoteConstruction )
				{
					if ( pBuilding->InRemoteConstruction() )
					{
						// Just have them start building if they're waiting for a remote signal.
						pBuilding->SetRemoteConstruction( false );
					}

					return;
				}

				pBuilding->DetonateObject();
			}
		}
	}
}
#endif
