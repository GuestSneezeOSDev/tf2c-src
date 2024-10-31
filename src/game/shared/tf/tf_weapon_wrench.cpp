//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "baseobject_shared.h"
#include "tf_weapon_grenade_mirv.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
// Server specific.
#else
	#include "tf_player.h"
#endif

extern ConVar tf2c_building_gun_mettle;

//=============================================================================
//
// Weapon Wrench tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFWrench, tf_weapon_wrench )

//=============================================================================
//
// Weapon Wrench functions.
//

class CTraceFilterWrench : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterWrench, CTraceFilterSimple );

	CTraceFilterWrench( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity->IsPlayer() )
			return false;

		if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
			return false;

		return true;
	}
};

class CTraceFilterGrenadeDefuse : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterGrenadeDefuse, CTraceFilterSimple );

	CTraceFilterGrenadeDefuse( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
		{
			CTFGrenadeMirvProjectile *pMirvProjectile = dynamic_cast<CTFGrenadeMirvProjectile *>( pEntity );
			if ( pMirvProjectile && pMirvProjectile->IsDeflectable() )
			{				
				return true;
			}
		}

		return false;
	}
};


CTFWrench::CTFWrench()
{
}

#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector vecHitPos )
{
	// Did this object hit do any work, repair, or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, vecHitPos );

	CDisablePredictionFiltering disabler;

	if ( bUsefulHit )
	{
		// play success sound
		WeaponSound( SPECIAL1 );
	}
	else
	{
		// play failure sound
		WeaponSound( SPECIAL2 );
	}
}
#endif

void CTFWrench::Smack( void )
{
	// See if we can hit an object with a higher range.

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->EyePosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// Prioritize defusing Dynamite Pack
	trace_t traceDefuse;

	CTraceFilterGrenadeDefuse traceDefuseFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceDefuseFilter, &traceDefuse );
	if ( traceDefuse.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceDefuseFilter, &traceDefuse );
	}

	if ( traceDefuse.fraction < 1.0f && traceDefuse.DidHitNonWorldEntity() && pPlayer->IsEnemy( traceDefuse.m_pEnt ) )
	{
#ifndef CLIENT_DLL
		// Do Damage.
		ETFDmgCustom iCustomDamage = TF_DMG_CUSTOM_NONE;
		float flDamage = GetMeleeDamage( traceDefuse.m_pEnt, iCustomDamage );
		int iDmgType = GetDamageType();

		{
			CDisablePredictionFiltering disabler;

			CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );
			CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * 80.0f );
			traceDefuse.m_pEnt->DispatchTraceAttack( info, vecForward, &traceDefuse );
			ApplyMultiDamage();
		}
#endif

		OnEntityHit( traceDefuse.m_pEnt );
		DoImpactEffect( traceDefuse, DMG_CLUB );

		return;
	}

	// Only trace against objects.

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterWrench traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0f )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );

		// Sometimes, like with short ceilings, we start inside a solid and miss our target.
		// Moving the hull forward a bit fixes being unable to upgrade the tele you're standing on in ctf_turbine vents.
		if ( trace.fractionleftsolid > 0.0f )
		{
			UTIL_TraceHull( vecSwingStart + ( vecForward * 0.5f * vecSwingMaxs ), vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
		}
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( static_cast<CBaseObject *>( trace.m_pEnt ), pPlayer, trace.endpos );
#endif
	}
	else
	{
		// If we cannot, Smack as usual for player hits.
		BaseClass::Smack();
	}
}


float CTFWrench::GetMeleeDamage( CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage )
{
	float flDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );
	iCustomDamage = TF_DMG_WRENCH_FIX;

	return flDamage;
}

ConVar tf_construction_build_rate_additive( "tf_construction_build_rate_additive", "1.5f", FCVAR_REPLICATED );
ConVar tf_construction_build_rate_multiplier( "tf_construction_build_rate_multiplier", "2.0f", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: Construction Speed
//-----------------------------------------------------------------------------
float CTFWrench::GetConstructionValue( void )
{
	float flValue = tf_construction_build_rate_multiplier.GetFloat();

	if ( tf2c_building_gun_mettle.GetBool() )
	{
		flValue = tf_construction_build_rate_additive.GetFloat();
	}
	else
	{
		flValue = tf_construction_build_rate_multiplier.GetFloat();
	}

	CALL_ATTRIB_HOOK_FLOAT( flValue, mult_construction_value );
	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: Repair Amount
//-----------------------------------------------------------------------------
float CTFWrench::GetRepairValue( void )
{
	float flValue = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flValue, mult_repair_value );
	return flValue;
}

#ifdef GAME_DLL
void CTFWrench::Equip(CBaseCombatCharacter* pOwner)
{
	BaseClass::Equip(pOwner);

	// Just wreck the teleporters.
	if (IsAirController())
	{
		DestroyBuildings(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE);
		DestroyBuildings(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT);
	}
	// Just wreck the dispensers.
	if (IsMiniDispenser())
	{
		DestroyBuildings(OBJ_DISPENSER);
	}

	// Just wreck the sentries
	if (IsFlameSentry())
	{
		DestroyBuildings(OBJ_SENTRYGUN);
	}
}
//-----------------------------------------------------------------------------
// Purpose: Destroy certain buildings depending on the PDA's attributes.
//-----------------------------------------------------------------------------
void CTFWrench::Detach(void)
{

	// Just wreck the teleporters.
	if (IsAirController())
	{
		DestroyBuildings(OBJ_JUMPPAD, JUMPPAD_TYPE_A);
		DestroyBuildings(OBJ_JUMPPAD, JUMPPAD_TYPE_B);

	}

	// Just wreck the mini dispensers.
	if (IsMiniDispenser())
	{
		DestroyBuildings(OBJ_MINIDISPENSER);
	}

	// Just wreck the flame sentries.
	if (IsFlameSentry())
	{
		DestroyBuildings(OBJ_FLAMESENTRY);
	}

	BaseClass::Detach();
}

//-----------------------------------------------------------------------------
// Purpose: Wreck everything.
//-----------------------------------------------------------------------------
void CTFWrench::DestroyBuildings(int iType /*= OBJ_LAST*/, int iMode /*= 0*/, bool bCheckForRemoteConstruction /*= false*/)
{
	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if (!pPlayer) 
	{
		return;
	} else {
		// Destroy everything.
		if (iType >= OBJ_LAST)
		{
			for (int i = 0; i < OBJ_LAST; i++)
			{
				CBaseObject* pBuilding = pPlayer->GetObjectOfType(i, iMode);
				if (pBuilding)
				{
					if (bCheckForRemoteConstruction)
					{
						if (pBuilding->InRemoteConstruction())
						{
							// Just have them start building if they're waiting for a remote signal.
							pBuilding->SetRemoteConstruction(false);
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
			CBaseObject* pBuilding = pPlayer->GetObjectOfType(iType, iMode);
			if (pBuilding)
			{
				if (bCheckForRemoteConstruction)
				{
					if (pBuilding->InRemoteConstruction())
					{
						// Just have them start building if they're waiting for a remote signal.
						pBuilding->SetRemoteConstruction(false);
					}

					return;
				}

				pBuilding->DetonateObject();
			}
		}
	}
}
#endif