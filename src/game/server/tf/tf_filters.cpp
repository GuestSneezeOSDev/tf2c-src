//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "filters.h"
#include "tf_control_point.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Team Fortress Team Filter
//
class CFilterTFTeam : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTFTeam, CBaseFilter );
	DECLARE_DATADESC();

	void InputRoundActivate( inputdata_t &inputdata );
	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:
	string_t m_iszControlPointName;
};

BEGIN_DATADESC( CFilterTFTeam )
DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_activator_tfteam, CFilterTFTeam );


bool CFilterTFTeam::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	// is the entity we're asking about on the winning 
	// team during the bonus time? (winners pass all filters)
	if (  TFGameRules() &&
		( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && 
		( TFGameRules()->GetWinningTeam() == pEntity->GetTeamNumber() ) )
	{
		// this should open all doors for the winners
		if ( m_bNegated )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return ( pEntity->GetTeamNumber() == GetTeamNumber() );
}


void CFilterTFTeam::InputRoundActivate( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		CTeamControlPoint *pControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( pControlPoint )
		{
			ChangeTeam( pControlPoint->GetTeamNumber() );
		}
		else
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}
}

class CFilterTFCanCap : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTFCanCap, CBaseFilter );

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( !pPlayer || !pPlayer->IsAlive() )
			return false;

		if ( pPlayer->GetTeamNumber() != GetTeamNumber() )
			return false;

		if ( pPlayer->m_Shared.IsStealthed() ||
			pPlayer->m_Shared.IsDisguised() ||
			pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) ||
			pPlayer->m_Shared.IsInvulnerable() )
			return false;

		return true;
	}
};

LINK_ENTITY_TO_CLASS( filter_tf_player_can_cap, CFilterTFCanCap );

class FilterDamagedByWeaponInSlot : public CBaseFilter
{
public:
	DECLARE_CLASS( FilterDamagedByWeaponInSlot, CBaseFilter );
	DECLARE_DATADESC();

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return true;
	}

	bool PassesDamageFilterImpl( const CTakeDamageInfo &info )
	{
		// Live TF2 checks CBaseCombatWeapon::GetSlot. Yet another thing I had to fix.
		CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
		if ( !pPlayer )
			return false;

		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
		if ( !pWeapon )
			return false;

		return ( pWeapon->GetItem()->GetLoadoutSlot( pPlayer->GetPlayerClass()->GetClassIndex() ) == m_iSlot );
	}

private:
	ETFLoadoutSlot m_iSlot;
};

BEGIN_DATADESC( FilterDamagedByWeaponInSlot )
DEFINE_KEYFIELD( m_iSlot, FIELD_INTEGER, "weaponSlot" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_damaged_by_weapon_in_slot, FilterDamagedByWeaponInSlot );

class CFilterTFCondition : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTFCondition, CBaseFilter );
	DECLARE_DATADESC();

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		return ( pPlayer && pPlayer->m_Shared.InCond( m_nCond ) );
	}

private:
	ETFCond m_nCond;
};

BEGIN_DATADESC( CFilterTFCondition )
DEFINE_KEYFIELD( m_nCond, FIELD_INTEGER, "condition" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_condition, CFilterTFCondition );

//=============================================================================
//
// Class filter
//
class CFilterTFClass : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTFClass, CBaseFilter );
	DECLARE_DATADESC();

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:
	int	m_iAllowedClass;
};

BEGIN_DATADESC( CFilterTFClass )
DEFINE_KEYFIELD( m_iAllowedClass, FIELD_INTEGER, "tfclass" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_class, CFilterTFClass );


bool CFilterTFClass::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEntity );
	if ( !pPlayer )
		return false;

	return pPlayer->IsPlayerClass( m_iAllowedClass, true );
}

//=============================================================================
//
// Class filter
//
class CFilterTFFlagHolder : public CBaseFilter
{
public:
	DECLARE_CLASS(CFilterTFFlagHolder, CBaseFilter);

	bool PassesFilterImpl(CBaseEntity* pCaller, CBaseEntity* pEntity)
	{
		CTFPlayer* pPlayer = ToTFPlayer(pEntity);
		if (!pPlayer)
			return false;

		return pPlayer->HasTheFlag();
	}
};

LINK_ENTITY_TO_CLASS(filter_tf_flagholder, CFilterTFFlagHolder);
