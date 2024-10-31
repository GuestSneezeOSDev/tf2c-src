//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:			The "weapon" used to build objects
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "entitylist.h"
#include "in_buttons.h"
#include "tf_obj.h"
#include "sendproxy.h"
#include "tf_weapon_builder.h"
#include "vguiscreen.h"
#include "tf_gamerules.h"
#include "tf_obj_teleporter.h"
#include "NextBotManager.h"

EXTERN_SEND_TABLE( DT_BaseCombatWeapon )

BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBuilder, DT_BuilderLocalData )
	SendPropInt( SENDINFO( m_iObjectType ), Q_log2( OBJ_LAST ) + 1, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hObjectBeingBuilt ) ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST( CTFWeaponBuilder, DT_TFWeaponBuilder )
	SendPropInt( SENDINFO( m_iBuildState ), 4, SPROP_UNSIGNED ),
	SendPropDataTable( "BuilderLocalData", 0, &REFERENCE_SEND_TABLE( DT_BuilderLocalData ), SendProxy_SendLocalWeaponDataTable ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_builder, CTFWeaponBuilder );
PRECACHE_WEAPON_REGISTER( tf_weapon_builder );

// Sapper
BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponSapper, DT_SapperLocalData )
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST( CTFWeaponSapper, DT_TFWeaponSapper )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_sapper, CTFWeaponSapper );
PRECACHE_WEAPON_REGISTER( tf_weapon_sapper );


CTFWeaponBuilder::CTFWeaponBuilder()
{
	m_iObjectType.Set( OBJ_LAST );
}


CTFWeaponBuilder::~CTFWeaponBuilder()
{
	StopPlacement();
}


void CTFWeaponBuilder::SetSubType( int iSubType )
{
	m_iObjectType = iSubType;

	BaseClass::SetSubType( iSubType );
}


void CTFWeaponBuilder::SetObjectMode( int iObjectMode )
{
	m_iObjectMode = iObjectMode;
}


void CTFWeaponBuilder::Precache( void )
{
	BaseClass::Precache();

	// Precache all the viewmodels we could possibly be building.
	for ( int iObject = 0; iObject < OBJ_LAST; iObject++ )
	{
		const CObjectInfo *pInfo = GetObjectInfo( iObject );
		if ( pInfo )
		{
			if ( pInfo->m_pViewModel )
			{
				PrecacheModel( pInfo->m_pViewModel );
			}

			if ( pInfo->m_pPlayerModel )
			{
				PrecacheModel( pInfo->m_pPlayerModel );
			}
		}
	}
}


bool CTFWeaponBuilder::CanDeploy( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return false;

	if ( pPlayer->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
		return false;

	return BaseClass::CanDeploy();
}


bool CTFWeaponBuilder::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();
	if ( bDeploy )
	{
		SetCurrentState( BS_PLACING );
		StartPlacement(); 
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.35f;
		m_flNextSecondaryAttack = gpGlobals->curtime; // ASAP!

		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return false;

		pPlayer->SetNextAttack( gpGlobals->curtime );

		m_iViewModelIndex = modelinfo->GetModelIndex( GetViewModel(0) );
		m_iWorldModelIndex = modelinfo->GetModelIndex( GetWorldModel() );

		m_flNextDenySound = 0;

		if ( m_hObjectBeingBuilt && m_hObjectBeingBuilt->IsBeingCarried() )
		{
			// We just pressed attack2, don't immediately rotate it.
			m_bInAttack2 = true;
		}
	}

	return bDeploy;
}


Activity CTFWeaponBuilder::GetDrawActivity( void )
{
	// Use the one handed sapper deploy if we're invisible.
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && GetType() == OBJ_ATTACHMENT_SAPPER && ( pOwner->m_Shared.InCond( TF_COND_STEALTHED ) || GetViewModelType() == VMTYPE_TF2 ) )
		return ACT_VM_DRAW_DEPLOYED;
	
	return BaseClass::GetDrawActivity();
}


bool CTFWeaponBuilder::CanHolster( void ) const
{
	// If player is hauling a building he can't switch away without dropping it.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.IsCarryingObject() )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Stop placement when holstering
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID )
	{
		SetCurrentState( BS_IDLE );
	}

	StopPlacement();

	// Make sure hauling status is cleared.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.IsCarryingObject() )
	{
		pOwner->m_Shared.SetCarriedObject( NULL );
	}

	return BaseClass::Holster( pSwitchingTo );
}


void CTFWeaponBuilder::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// If we're building, and our team has lost, stop placing the object.
	if ( m_hObjectBeingBuilt.Get() && 
		TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		pOwner->GetTeamNumber() != TFGameRules()->GetWinningTeam() )
	{
		StopPlacement();
		return;
	}

	// Check that I still have enough resources to build this item.
	if ( pOwner->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
	{
		SwitchOwnersWeaponToLast();
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		PrimaryAttack();
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
		{
			SecondaryAttack();
		}
	}
	else
	{
		m_bInAttack2 = false;
	}

	WeaponIdle();
}
#include <misc_helpers.h>
//-----------------------------------------------------------------------------
// Purpose: Start placing or building the currently selected object.
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	int iCantSap = 0;
	CALL_ATTRIB_HOOK_FLOAT( iCantSap, sapper_cant_sap );
	if ( iCantSap >= 1)
		return;

	// Necessary so that we get the latest building position for the test, otherwise
	// we are one frame behind.
	UpdatePlacementState();

	// What state should we move to?
	switch ( m_iBuildState )
	{
		case BS_IDLE:
			{
				// Idle state starts selection.
				SetCurrentState( BS_SELECTING );
			}
			break;

		case BS_SELECTING:
			{
				// Do nothing, client handles selection.
				return;
			}
			break;

		case BS_PLACING:
			{
				if ( m_hObjectBeingBuilt )
				{
					int iFlags = m_hObjectBeingBuilt->GetObjectFlags();

					// Tricky, because this can re-calc the object position and change whether its a valid 
					// pos or not. Best not to do this only in debug, but we can be pretty sure that this
					// will give the same result as was calculated in UpdatePlacementState() above.
					Assert( IsValidPlacement() );

					// If we're placing an attachment, like a sapper, play a placement animation on the owner.
					if ( m_hObjectBeingBuilt->MustBeBuiltOnAttachmentPoint() )
					{
						pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
					}

					// Need to save this for later since StartBuilding will clear m_hObjectBeingBuilt.
					CBaseObject *pParentObject = m_hObjectBeingBuilt->GetParentObject();

					StartBuilding();

					TheNextBots().OnWeaponFired( pOwner, this );

					if ( GetType() == OBJ_ATTACHMENT_SAPPER )
					{
						// TFBot: make bots suspicious of spies shortly after a sap.
						CUtlVector<CTFPlayer *> players;
						CollectPlayers( &players, TEAM_ANY, true );
						for ( auto player : players )
						{
							player->OnSapperPlaced( pParentObject );
						}

						// Attaching a sapper to a teleporter automatically saps another end.
						CObjectTeleporter *pTeleporter = dynamic_cast<CObjectTeleporter *>( pParentObject );
						if ( pTeleporter )
						{
							CObjectTeleporter *pMatch = pTeleporter->GetMatchingTeleporter();

							// If the other end is not already sapped then place a sapper on it.
							if ( pMatch && !pMatch->IsPlacing() && !pMatch->HasSapper() )
							{
								SetCurrentState( BS_PLACING );
								StartPlacement();
								if ( m_hObjectBeingBuilt.Get() )
								{
									m_hObjectBeingBuilt->UpdateAttachmentPlacement( pMatch );
									StartBuilding();
								}
							}
						}
					}

					// Should we switch away?
					if ( iFlags & OF_ALLOW_REPEAT_PLACEMENT )
					{
						// Start placing another
						SetCurrentState( BS_PLACING );
						StartPlacement(); 
					}
					else
					{
						SwitchOwnersWeaponToLast();
					}
				}
			}
			break;

		case BS_PLACING_INVALID:
			{
                if (m_flNextDenySound < gpGlobals->curtime)
                {
                    static ConVarRef tf2c_no_arena_suddendeath_sentries("tf2c_no_arena_suddendeath_sentries");
                    static ConVarRef tf2c_no_sentries("tf2c_no_sentries");
                    if
                    (
                        (
                            tf2c_no_arena_suddendeath_sentries.GetBool()
                            && ( GetType() == OBJ_SENTRYGUN || GetType() == OBJ_FLAMESENTRY )
                            && ( TFGameRules()->GetGameType() == TF_GAMETYPE_ARENA || TFGameRules()->InStalemate() )
                        )
                        ||
                        tf2c_no_sentries.GetBool()
                    )
                    {
                        int rand = RandomInt(1, 3);
                        CSingleUserRecipientFilter filter(pOwner);
                        std::string response = fmt::format( FMT_STRING( "Engineer.No0{:d}"), rand );

                        // stupid bullshit hack to get volume to sound correct
                        EmitSound_t params = {};
                        params.m_pSoundName                     = response.c_str();
                        //params.m_nChannel                       = CHAN_AUTO;
                        //params.m_SoundLevel                     = (soundlevel_t)1;
                        //params.m_flVolume                       = VOL_NORM;
                        params.m_nFlags                         = SND_CHANGE_VOL;

                        EmitSound(filter, entindex(), params, params.m_hSoundScriptHandle);


                        m_flNextDenySound = gpGlobals->curtime + 2.5f;
                    }
                    else
                    {
					    CSingleUserRecipientFilter filter( pOwner );
					    EmitSound( filter, entindex(), "Player.DenyWeaponSelection" );

					    m_flNextDenySound = gpGlobals->curtime + 0.5f;
				    }
                }
			}
			break;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
}


void CTFWeaponBuilder::SecondaryAttack( void )
{
	if ( m_bInAttack2 )
		return;

	// Require a re-press.
	m_bInAttack2 = true;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	UpdatePlacementState();

	if ( !pOwner->DoClassSpecialSkill() && ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID ) )
	{
		if ( m_hObjectBeingBuilt )
		{
			pOwner->StopHintTimer( HINT_ALTFIRE_ROTATE_BUILDING );
			m_hObjectBeingBuilt->RotateBuildAngles();
		}
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}

//-----------------------------------------------------------------------------
// Purpose: Set the builder to the specified state
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetCurrentState( int iState )
{
	m_iBuildState = iState;
}

//-----------------------------------------------------------------------------
// Purpose: Set the owner's weapon and last weapon appropriately when we need to
// switch away from the builder weapon.  
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SwitchOwnersWeaponToLast()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// For Engineer, switch to wrench and set last weapon appropriately.
	if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER, true ) )
	{
		// Switch to wrench if possible, if not, then best weapon.
		CBaseCombatWeapon *pWeapon = pOwner->Weapon_GetSlot( 2 );

		// Don't store last weapon when we autoswitch off builder.
		CBaseCombatWeapon *pLastWeapon = pOwner->GetLastWeapon();

		if ( pWeapon )
		{
			pOwner->Weapon_Switch( pWeapon );
		}
		else
		{
			pOwner->SwitchToNextBestWeapon( NULL );
		}

		if ( pWeapon == pLastWeapon )
		{
			// We had the wrench out before we started building. Go ahead and set out last
			// weapon to our primary weapon.
			pWeapon = pOwner->Weapon_GetSlot( 0 );
			pOwner->Weapon_SetLast( pWeapon );
		}
		else
		{
			pOwner->Weapon_SetLast( pLastWeapon );
		}
	}
	else
	{
		// For all other classes, just switch to last weapon used.
		pOwner->Weapon_Switch( pOwner->GetLastWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the building postion and checks the new postion.
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::UpdatePlacementState( void )
{
	// This updates the building position.
	bool bValidPos = IsValidPlacement();

	// If we're in placement mode, update the placement model.
	switch ( m_iBuildState )
	{
		case BS_PLACING:
		case BS_PLACING_INVALID:
			{
				if ( bValidPos )
				{
					SetCurrentState( BS_PLACING );
				}
				else
				{
					SetCurrentState( BS_PLACING_INVALID );
				}
			}
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Idle updates the position of the build placement model.
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WeaponIdle( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start placing the object
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartPlacement( void )
{
	StopPlacement();

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( pOwner->m_Shared.GetCarriedObject() )
	{
		m_hObjectBeingBuilt = pOwner->m_Shared.GetCarriedObject();
	}
	else
	{
		// Create the slab
		m_hObjectBeingBuilt = (CBaseObject *)CreateEntityByName( GetObjectInfo( m_iObjectType )->m_pClassName );
	}

	if ( m_hObjectBeingBuilt )
	{
		// For attributes.
		m_hObjectBeingBuilt->SetBuilder( pOwner );

		bool bIsCarried = m_hObjectBeingBuilt->IsBeingCarried();
		if ( !bIsCarried )
		{
			m_hObjectBeingBuilt->SetObjectMode( m_iObjectMode );
		}

		m_hObjectBeingBuilt->Spawn();
		m_hObjectBeingBuilt->StartPlacement( pOwner );

		if ( !bIsCarried )
		{
			// Stomp this here in the same frame we make the object, so prevent clientside warnings that it's under attack.
			m_hObjectBeingBuilt->m_iHealth = OBJECT_CONSTRUCTION_STARTINGHEALTH;
		}
	}
}


void CTFWeaponBuilder::StopPlacement( void )
{
	if ( m_hObjectBeingBuilt )
	{
		m_hObjectBeingBuilt->StopPlacement();
		m_hObjectBeingBuilt = NULL;
	}
}


void CTFWeaponBuilder::WeaponReset( void )
{
	BaseClass::WeaponReset();

	StopPlacement();
}

//-----------------------------------------------------------------------------
// Purpose: Move the placement model to the current position.
// Return false if it's an invalid position.
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::IsValidPlacement( void )
{
	if ( !m_hObjectBeingBuilt )
		return false;

	CBaseObject *pObject = m_hObjectBeingBuilt.Get();
	pObject->UpdatePlacement();

	return pObject->IsValidPlacement();
}

//-----------------------------------------------------------------------------
// Purpose: Player holding this weapon has started building something.
// Assumes we are in a valid build position.
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartBuilding( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	CBaseObject *pObject = m_hObjectBeingBuilt.Get();
	if ( pPlayer && pPlayer->m_Shared.IsCarryingObject() )
	{
		Assert( pObject );

		pObject->DropCarriedObject( pPlayer );
	}

	Assert( pObject );

	pObject->StartBuilding( GetOwner() );

	m_hObjectBeingBuilt = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo.
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::HasAmmo( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	return pOwner->GetBuildResources() >= CTFPlayerShared::CalculateObjectCost( pOwner, m_iObjectType );
}


int CTFWeaponBuilder::GetSlot( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionSlot;
}


int CTFWeaponBuilder::GetPosition( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetPrintName( void ) const
{
	const CObjectInfo *pInfo = GetObjectInfo( m_iObjectType );
	if ( pInfo->m_AltModes.Count() > 0 )
		return pInfo->m_AltModes.Element( m_iObjectMode * 3 );

	return pInfo->m_pStatusName;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetViewModel( int iViewModel ) const
{
	if ( !GetPlayerOwner() )
		return BaseClass::GetViewModel();

	if ( m_iObjectType != OBJ_LAST )
		return DetermineViewModelType( GetObjectInfo( m_iObjectType )->m_pViewModel );

	return BaseClass::GetViewModel();
}


const char *CTFWeaponBuilder::GetWorldModel( void ) const
{
	if ( !GetPlayerOwner() )
		return BaseClass::GetWorldModel();

	if ( m_iObjectType != OBJ_LAST )
		return GetObjectInfo( m_iObjectType )->m_pPlayerModel;

	return BaseClass::GetWorldModel();
}


bool CTFWeaponBuilder::AllowsAutoSwitchTo( void ) const
{
	// Ask the object we're building.
	return GetObjectInfo( m_iObjectType )->m_bAutoSwitchTo;
}


bool CTFWeaponBuilder::ForceWeaponSwitch( void ) const
{
	if ( !CanHolster() )
		return true;

	return BaseClass::ForceWeaponSwitch();
}


CTFWeaponSapper::CTFWeaponSapper() : CTFWeaponBuilder()
{
}


CTFWeaponSapper::~CTFWeaponSapper()
{
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponSapper::GetViewModel( int iViewModel ) const
{
	return CTFWeaponBase::GetViewModel( iViewModel );
}


const char *CTFWeaponSapper::GetWorldModel( void ) const
{
	return CTFWeaponBase::GetWorldModel();
}
