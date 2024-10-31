//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ammo_pack.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "tf_powerup.h"
#include "entity_ammopack.h"
#include "tf_weaponbase.h"
#include "tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TF_PUMPKIN_LOOT_PICKUP_SOUND	"Halloween.PumpkinPickup"
#define TF_PUMPKIN_LOOT_MODEL			"models/props_halloween/pumpkin_loot.mdl"

extern ConVar tf2c_spy_cloak_ammo_refill;
ConVar tf2c_dropped_weapons_give_constant_metal("tf2c_dropped_weapons_give_constant_metal", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "1 - Dropped weapons give constant metal alike ammo packs. 0 - Dropped weapons give metal that killed player had remaining.");

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFAmmoPack, DT_AmmoPack )
	SendPropBool( SENDINFO( m_bPilotLight ) ),
	SendPropDataTable( SENDINFO_DT( m_Item ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFAmmoPack )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_ammo_pack, CTFAmmoPack );
PRECACHE_REGISTER( tf_ammo_pack );


CTFAmmoPack::CTFAmmoPack()
{
	m_flCreationTime = 0.0f;
	m_bAllowOwnerPickup = false;
	m_bHealing = false;
	m_bPumpkinLoot = false;
	m_bPilotLight = false;
}


void CTFAmmoPack::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	BaseClass::Spawn();

	// Give the ammo pack some health, so that trains can destroy it.
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	m_takedamage = DAMAGE_YES;
	SetHealth( 900 );

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFAmmoPack::FlyThink );

	SetTouch( &CTFAmmoPack::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// no ammo to start
	memset( m_iAmmo, 0, sizeof( m_iAmmo ) );

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );
}


void CTFAmmoPack::Precache( void )
{
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );
	if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
	{
		PrecacheModel( TF_PUMPKIN_LOOT_MODEL );
		PrecacheScriptSound( TF_PUMPKIN_LOOT_PICKUP_SOUND );
	}
}


CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTFWeaponBase *pWeapon /*= NULL*/, float flPackRatio /*= 0.5*/ )
{
	if ( !pWeapon )
		return NULL;

	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack *>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		if ( pWeapon->GetItem() )
		{
			pAmmoPack->SetItem( *pWeapon->GetItem() );
		}

		pAmmoPack->ChangeTeam( pOwner->GetTeamNumber() );
		pAmmoPack->SetModelName( AllocPooledString( pWeapon->GetWorldModel() ) );
		pAmmoPack->m_bHealing = pWeapon->HealsWhenDropped();
		pAmmoPack->m_flPackRatio = flPackRatio;

		if ( !Q_strcmp( pWeapon->GetClassname(), "tf_weapon_flamethrower" ) )
		{
			pAmmoPack->m_bPilotLight = true;
		}

		DispatchSpawn( pAmmoPack );
	}

	return pAmmoPack;
}


CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, float flPackRatio )
{
	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack *>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		pAmmoPack->SetModelName( AllocPooledString( pszModelName ) );
		pAmmoPack->m_flPackRatio = flPackRatio;
		DispatchSpawn( pAmmoPack );
	}

	return pAmmoPack;
}


int CTFAmmoPack::GiveAmmo( int iCount, int iAmmoType )
{
	if ( iAmmoType == -1 || iAmmoType >= TF_AMMO_COUNT )
	{
		Msg( "ERROR: Attempting to give unknown ammo type (%d)\n", iAmmoType );
		return 0;
	}

	m_iAmmo[iAmmoType] += iCount;

	return iCount;
}


void CTFAmmoPack::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}


void CTFAmmoPack::PackTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	// Don't let the person who threw this ammo pick it up until it hits the ground.
	// This way we can throw ammo to people, but not touch it as soon as we throw it ourselves.
	if ( GetOwnerEntity() == pOther && !m_bAllowOwnerPickup )
		return;

	// tf_ammo_pack (dropped weapons) originally packed killed player's ammo.
	// This was changed to make them act as medium ammo packs by default.
#if 0
	// Old ammo giving code.
	int iAmmoTaken = 0;

	int i;
	for ( i=0;i<TF_AMMO_COUNT;i++ )
	{
		iAmmoTaken += pPlayer->GiveAmmo( m_iAmmo[i], i );
	}

	if ( iAmmoTaken > 0 )
	{
		UTIL_Remove( this );
	}
#else
	// Copy-paste from CAmmoPack code.
	bool bSuccess = false;

	if ( m_bPumpkinLoot ) 
	{
		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, 3.2f );
		EmitSound( TF_PUMPKIN_LOOT_PICKUP_SOUND );
		bSuccess = true;
	}

	if ( m_bHealing )
	{
		// Dropped sandviches heal instead.
		
		// Old behavior
		/*float flHealAmount = pPlayer->IsPlayerClass(TF_CLASS_SCOUT, true) ? 75.0f : 50.0f;
		if ( pPlayer->TakeHealth( flHealAmount, HEAL_NOTIFY ) > 0 )
		{
			bSuccess = true;

			IGameEvent *event = gameeventmanager->CreateEvent( "player_stealsandvich", true );
			if ( event )
			{
				CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
				event->SetInt( "owner", pOwner ? pOwner->GetUserID() : 0 );
				event->SetInt( "target", pPlayer->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
		}*/

		// Medkit copypasta, lite version
		// If you change anything, consider looking into CHealthKit::MyTouch
		int iHealthToAdd = Floor2Int(pPlayer->GetMaxHealth() * 0.5f);
		int iHealthRestored = pPlayer->TakeHealth(iHealthToAdd, HEAL_NOTIFY);

		if (iHealthRestored)
			bSuccess = true;

		// Restore disguise health.
		if (pPlayer->m_Shared.IsDisguised())
		{
			int iFakeHealthToAdd = pPlayer->m_Shared.GetDisguiseMaxHealth() * 0.5f;
			if (pPlayer->m_Shared.AddDisguiseHealth(iFakeHealthToAdd))
				bSuccess = true;
		}

		// Remove any negative conditions whether player got healed or not.
		if (pPlayer->m_Shared.HealNegativeConds())
			bSuccess = true;

		if (bSuccess)
		{
			CSingleUserRecipientFilter user(pPlayer);
			user.MakeReliable();
			EmitSound(user, entindex(), "HealthKit.Touch");

			CTFPlayer* pTFOwner = ToTFPlayer(GetOwnerEntity());
			if (pTFOwner)
			{
				if (!pTFOwner->IsEnemy(pPlayer))
				{
					// BONUS DUCKS!
					CTF_GameStats.Event_PlayerHealedOther(pTFOwner, (float)iHealthRestored);

					if (pTFOwner != pPlayer)
					{
						CTF_GameStats.Event_PlayerAwardBonusPoints(pTFOwner, pPlayer, 1);
						// Automatic heal thanks voice line
						// pTFPlayer->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_THANKS); // HEAVY IS DEAD
					}
				}
				else
				{
					// BONUS QUACKS!
					CTF_GameStats.Event_PlayerLeachedHealth(pPlayer, false, (float)iHealthRestored);
				}
			}

			// Show healing to the one who dropped the healthkit.
			if (iHealthRestored && pTFOwner && pTFOwner != pPlayer)
			{
				IGameEvent* event_healed = gameeventmanager->CreateEvent("player_healed");

				if (event_healed)
				{
					event_healed->SetInt("patient", pPlayer->GetUserID());
					event_healed->SetInt("healer", pTFOwner->GetUserID());
					event_healed->SetInt("amount", iHealthRestored);
					event_healed->SetInt("class", pPlayer->GetPlayerClass()->GetClassIndex());

					gameeventmanager->FireEvent(event_healed);
				}

				// VIP healing achievement.
				if (pPlayer && pPlayer->IsVIP())
				{
					IGameEvent* event_civilian_healed = gameeventmanager->CreateEvent("vip_healed");

					if (event_civilian_healed)
					{
						event_civilian_healed->SetInt("healer", pTFOwner->GetUserID());
						event_civilian_healed->SetInt("amount", iHealthRestored);

						gameeventmanager->FireEvent(event_civilian_healed);
					}
				}
			}

			IGameEvent* event = gameeventmanager->CreateEvent("player_stealsandvich", true);
			if (event)
			{
				CTFPlayer* pOwner = ToTFPlayer(GetOwnerEntity());
				event->SetInt("owner", pOwner ? pOwner->GetUserID() : 0);
				event->SetInt("target", pPlayer->GetUserID());
				gameeventmanager->FireEvent(event);
			}
		}
		// End of Medkit Copypasta
	}
	else
	{
		float flCannotPickupDroppedWeaponsWhileCloaked = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer, flCannotPickupDroppedWeaponsWhileCloaked, cannot_pickup_dropped_weapons_while_cloaked);
		if (!(flCannotPickupDroppedWeaponsWhileCloaked && pPlayer->m_Shared.GetPercentInvisible() >= flCannotPickupDroppedWeaponsWhileCloaked && 
			!(pPlayer->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF) || pPlayer->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING))))	// User buff invisibility doesn't count!
		{
			// Building gibs give ammo & cloak based on pack ratio.
			int iMaxPrimary = pPlayer->GetMaxAmmo(TF_AMMO_PRIMARY);


			float flPrimaryAmmoPackRatio = m_flPackRatio;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer, flPrimaryAmmoPackRatio, mult_primary_ammo_from_dropped_weapons);

			if (pPlayer->GiveAmmo(Ceil2Int(iMaxPrimary * flPrimaryAmmoPackRatio), TF_AMMO_PRIMARY))
			{
				bSuccess = true;
			}

			float flSecondaryAmmoPackRatio = m_flPackRatio;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer, flSecondaryAmmoPackRatio, mult_secondary_ammo_from_dropped_weapons);

			int iMaxSecondary = pPlayer->GetMaxAmmo(TF_AMMO_SECONDARY);
			if (pPlayer->GiveAmmo(Ceil2Int(iMaxSecondary * flSecondaryAmmoPackRatio), TF_AMMO_SECONDARY))
			{
				bSuccess = true;
			}

			if (tf2c_dropped_weapons_give_constant_metal.GetBool())
			{
				int iMaxMetal = pPlayer->GetMaxAmmo(TF_AMMO_METAL);
				if (pPlayer->GiveAmmo(Ceil2Int(iMaxMetal * m_flPackRatio), TF_AMMO_METAL))
				{
					bSuccess = true;
				}
			}
			else
			{
				//int iMaxMetal = pTFPlayer->GetPlayerClass()->GetData()->m_aAmmoMax[TF_AMMO_METAL];
				// Unlike other ammo, give fixed amount of metal that was given to us at spawn.
				if (pPlayer->GiveAmmo(m_iAmmo[TF_AMMO_METAL], TF_AMMO_METAL))
				{
					bSuccess = true;
				}
			}

			if (tf2c_spy_cloak_ammo_refill.GetBool())
			{
				if (pPlayer->m_Shared.AddSpyCloak(Min(50.0f, m_flPackRatio * 100.0f * 4.0f)) > 0.0f)
				{
					CSingleUserRecipientFilter filter(pPlayer);
					EmitSound(filter, entindex(), "BaseCombatCharacter.AmmoPickup");

					bSuccess = true;
				}
			}
		}

		if (bSuccess)
		{
			IGameEvent* event_picked = gameeventmanager->CreateEvent("player_picked_dropped_weapon");

			if (event_picked)
			{
				event_picked->SetInt("userid", pPlayer->GetUserID());

				gameeventmanager->FireEvent(event_picked);
			}
		}
	}

	// Did we give them anything?
	if ( bSuccess )
	{
		UTIL_Remove( this );
	}
#endif
}


unsigned int CTFAmmoPack::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}


void CTFAmmoPack::SetPumpKinLoot()
{
	SetModel( TF_PUMPKIN_LOOT_MODEL ); //Change model to pumpkin loot
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetAbsVelocity( this->GetAbsVelocity() + Vector( 0.0f, 0.0f, 200.0f ) ); //Make loot fly up a bit
	SetAbsAngles( vec3_angle );//Reset angles
	UseClientSideAnimation();
	ResetSequence( LookupSequence( "idle" ) );
	m_bPumpkinLoot = true;
}


void CTFAmmoPack::SetItem( CEconItemView &newItem )
{
	m_Item = newItem;
}


CEconItemView *CTFAmmoPack::GetItem( void )
{
	return &m_Item;
}
