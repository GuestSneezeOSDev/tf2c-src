//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_beacon.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Beacon tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFBeacon, DT_TFBeacon)
BEGIN_NETWORK_TABLE(CTFBeacon, DT_TFBeacon)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iStoredBeaconCrit)),
RecvPropTime(RECVINFO(m_flLastAfterburnTime)),
#else
SendPropInt(SENDINFO(m_iStoredBeaconCrit), 8, SPROP_UNSIGNED),
SendPropTime(SENDINFO(m_flLastAfterburnTime)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFBeacon)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_iStoredBeaconCrit, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAfterburnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_beacon, CTFBeacon);
PRECACHE_WEAPON_REGISTER(tf_weapon_beacon);


#define TF2C_BEACON_CRIT_MAX					30 // Change bits too.

//=============================================================================
//
// Weapon Beacon functions.
//

CTFBeacon::~CTFBeacon()
{
#ifdef CLIENT_DLL
	if (m_pHealEffect)
	{
		C_BaseEntity* pEffectOwner = m_hHealEffectHost.Get();
		if (pEffectOwner)
		{
			pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pHealEffect);
			m_pHealEffect = NULL;
			m_hHealEffectHost = NULL;
		}
	}
#endif
}

void CTFBeacon::WeaponReset()
{
	BaseClass::WeaponReset();
	m_iStoredBeaconCrit = 0;
	m_bCurrentAttackUsesStoredBeaconCrit = false;
	m_flLastAfterburnTime = 0;
#ifdef CLIENT_DLL
	ManageHealEffect();
#endif
}



void CTFBeacon::Precache()
{
#ifdef GAME_DLL
	PrecacheParticleSystem("scythe_chargeV_parent");
	PrecacheParticleSystem("scythe_chargeW_parent");
#endif
	PrecacheTeamParticles("medicgun_invulnstatus_fullcharge_%s");
	PrecacheScriptSound("Weapon_Scythe.Bell");
	BaseClass::Precache();
}


float CTFBeacon::GetMeleeDamage(CBaseEntity *pTarget, ETFDmgCustom& iCustomDamage)
{
	CTFPlayer* pPlayer = ToTFPlayer(pTarget);
	CTFPlayer* pOwner = GetTFPlayerOwner();
	float flDamage = BaseClass::GetMeleeDamage(pTarget, iCustomDamage);

	// don't take away heat if enemy player either can't take damage or already takes crit
	/*if (m_bCurrentAttackUsesStoredBeaconCrit && IsStoredBeaconCritFilled() \
		&& pPlayer && !pPlayer->m_Shared.IsInvulnerable() && \
		!pPlayer->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB) \
		&& !pPlayer->m_Shared.InCond(TF_COND_TRANQUILIZED)) // players in 2.1.0 take crit from any melee weapon when under tranq
	{
		int iTargetTakesMeleeCrits = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pPlayer, iTargetTakesMeleeCrits, melee_taken_becomes_crit_wearer);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pPlayer->GetActiveTFWeapon(), iTargetTakesMeleeCrits, melee_taken_becomes_crit_active);
		if (!iTargetTakesMeleeCrits)
		{
			ClearStoredBeaconCrit();
		}
	}*/
	// Decided that it shouldn't aggressively help players

	if (m_bCurrentAttackUsesStoredBeaconCrit && IsStoredBeaconCritFilled() && pOwner && pPlayer && !pPlayer->m_Shared.IsInvulnerable()) // strong like world geometry
	{
		int iLifestealCritBeacon = 0.0f;
		CALL_ATTRIB_HOOK_INT( iLifestealCritBeacon, lifesteal_crit_beacon );
		if ( iLifestealCritBeacon )
		{
#ifdef GAME_DLL
			float flCritDamage = flDamage * 3.0f; // hack !!! fuck off !!!
			
			int iHealthToSteal = pPlayer->m_iMaxHealth < flCritDamage ? pPlayer->m_iMaxHealth : flCritDamage;
			//                              heavy          195                 300                 > 195 <
			//                              scout          195               > 125 <                 195

			int iHealthRestored = pOwner->TakeHealth( iHealthToSteal, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
			CTF_GameStats.Event_PlayerHealedOther(pOwner, (float)iHealthRestored);
#endif
		}
		int iLifestealCritBeaconSimple = 0.0f;
		CALL_ATTRIB_HOOK_INT(iLifestealCritBeaconSimple, heal_on_crit_beacon_simple);
		if (iLifestealCritBeaconSimple)
		{
#ifdef GAME_DLL
			int iHealthRestored = pOwner->TakeHealth(iLifestealCritBeaconSimple, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP);
			CTF_GameStats.Event_PlayerHealedOther(pOwner, (float)iHealthRestored);
#endif
		}
		ClearStoredBeaconCrit();
	}

	return flDamage;
}

void CTFBeacon::CalcIsAttackCritical(void)
{
	m_bCurrentAttackUsesStoredBeaconCrit = false;
	return BaseClass::CalcIsAttackCritical();
}


bool CTFBeacon::CalcIsAttackCriticalHelper(void)
{
	if (IsStoredBeaconCritFilled())
	{
		m_bCurrentAttackUsesStoredBeaconCrit = true;
		return true;
	}
	return BaseClass::CalcIsAttackCriticalHelper();
}


void CTFBeacon::AddStoredBeaconCrit(int iAmount)
{
	m_iStoredBeaconCrit = min(GetMaxBeaconTicks(), m_iStoredBeaconCrit + iAmount);
	m_flLastAfterburnTime = gpGlobals->curtime;
}

int CTFBeacon::GetStoredBeaconCrit()
{
	return m_iStoredBeaconCrit;
}

void CTFBeacon::ClearStoredBeaconCrit()
{
	m_iStoredBeaconCrit = 0;
}


bool CTFBeacon::IsStoredBeaconCritFilled()
{
	return m_iStoredBeaconCrit >= GetMaxBeaconTicks();
}


float CTFBeacon::GetProgress()
{
	return min(1, GetStoredBeaconCrit() / float(GetMaxBeaconTicks()));
}


#ifdef CLIENT_DLL
void CTFBeacon::ManageHealEffect( void )
{
	bool AreEnemiesBurning;

	AreEnemiesBurning = (gpGlobals->curtime - m_flLastAfterburnTime <= TF_BURNING_FREQUENCY + 0.1f);

	CTFPlayer* pOwner = GetTFPlayerOwner();

	if (pOwner && !IsHolstered() && AreEnemiesBurning)
	{
		if (!m_pHealEffect)
		{
			C_BaseEntity* pEffectOwner = GetWeaponForEffect();
			if (pEffectOwner)
			{
				if (pOwner->ShouldDrawThisPlayer())
				{
					m_pHealEffect = pEffectOwner->ParticleProp()->Create("scythe_chargeW_parent", PATTACH_ABSORIGIN_FOLLOW);
				}
				else
				{
					m_pHealEffect = pEffectOwner->ParticleProp()->Create("scythe_chargeV_parent", PATTACH_ABSORIGIN_FOLLOW);
				}
				m_hHealEffectHost = pEffectOwner;
			}
		}
	}
	else
	{
		if (m_pHealEffect)
		{
			C_BaseEntity* pEffectOwner = m_hHealEffectHost.Get();
			if (pEffectOwner)
			{
				// Kill charge effect instantly when holstering otherwise it looks bad.
				if (IsHolstered())
				{
					pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pHealEffect);
				}
				else
				{
					pEffectOwner->ParticleProp()->StopEmission(m_pHealEffect);
				}

				m_hHealEffectHost = NULL;
			}

			m_pHealEffect = NULL;
		}
	}
}



void CTFBeacon::ThirdPersonSwitch(bool bThirdperson)
{
	BaseClass::ThirdPersonSwitch(bThirdperson);
	if (m_pHealEffect)
	{
		C_BaseEntity* pEffectOwner = m_hHealEffectHost.Get();
		if (pEffectOwner)
		{
			pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pHealEffect);
			m_pHealEffect = NULL;
			m_hHealEffectHost = NULL;
		}

		ManageHealEffect();
	}
}

void CTFBeacon::UpdateOnRemove(void)
{
	if (m_pHealEffect)
	{
		C_BaseEntity* pEffectOwner = m_hHealEffectHost.Get();
		if (pEffectOwner)
		{
			pEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately(m_pHealEffect);
			m_pHealEffect = NULL;
			m_hHealEffectHost = NULL;
		}
	}
	BaseClass::UpdateOnRemove();
}

#endif


void CTFBeacon::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// You've got blood on my Scythe, now I have to clean it off.
	bool bPlayerIsUnderwater = pPlayer->IsPlayerUnderwater();
	if ( m_bUnderwater != bPlayerIsUnderwater )
	{
		ResetCritBodyGroup();
		m_bUnderwater = bPlayerIsUnderwater;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only switch to crit bodygroup if the person wasn't invulnerable
//-----------------------------------------------------------------------------
bool CTFBeacon::ShouldSwitchCritBodyGroup( CBaseEntity *pEntity )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if( !pOwner->IsPlayerUnderwater() )
	{
		CTFPlayer *pVictim = ToTFPlayer( pEntity );
		if ( pVictim && (pVictim->m_Shared.IsInvulnerable() || pVictim->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB)) ) 
			return false;

		return BaseClass::ShouldSwitchCritBodyGroup( pEntity );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFBeacon::GetMaxBeaconTicks(void)
{
	int iMaxTicks = TF2C_BEACON_CRIT_MAX;
	CALL_ATTRIB_HOOK_INT(iMaxTicks, mod_beacon_max_ticks);

	if(iMaxTicks > 255) // max 8 bits
		iMaxTicks = 255;

	if(iMaxTicks < 1) // no div by 0
		iMaxTicks = 1;

	return iMaxTicks;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFBeacon::OnPreDataChanged(DataUpdateType_t type)
{
	BaseClass::OnPreDataChanged(type);

	m_bOldStoredBeaconCritFilled = IsStoredBeaconCritFilled();
}


//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFBeacon::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	C_TFPlayer* pOwner = GetTFPlayerOwner();
	if (pOwner && IsStoredBeaconCritFilled() != m_bOldStoredBeaconCritFilled)
	{
		pOwner->m_Shared.UpdateCritBoostEffect();

		if( IsStoredBeaconCritFilled() )
			EmitSound("Weapon_Scythe.Bell");
	}
	ManageHealEffect();
}

#endif