//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_anchor.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

#ifdef TF2C_BETA

//=============================================================================
//
// Weapon Shovel tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFAnchor, DT_TFAnchor);
BEGIN_NETWORK_TABLE(CTFAnchor, DT_TFAnchor)

#ifdef GAME_DLL
SendPropFloat(SENDINFO(m_flChargeMeter), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN),
SendPropBool(SENDINFO(m_bReadyToEarthquake)),
SendPropBool(SENDINFO(m_bIsDraining)),
#else
RecvPropFloat(RECVINFO(m_flChargeMeter)),
RecvPropBool(RECVINFO(m_bReadyToEarthquake)),
RecvPropBool(RECVINFO(m_bIsDraining)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFAnchor)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_anchor, CTFAnchor);
PRECACHE_WEAPON_REGISTER(tf_weapon_anchor);

CTFAnchor::CTFAnchor()
{
	m_flChargeMeter = 0.0f;
	m_bSoundPlayed = false;
	m_bIsDraining = false;
}

void CTFAnchor::Precache(void)
{
	PrecacheScriptSound("Weapon_Anchor.Fall");
	BaseClass::Precache();
}

void CTFAnchor::WeaponReset(void)
{
	BaseClass::WeaponReset();
	m_flChargeMeter = 0.0f;
}

bool CTFAnchor::IsOwnerAirborne(void)
{
	CTFPlayer* pOwner = GetTFPlayerOwner();
	if (!pOwner)
		return false;

	return !pOwner->GetGroundEntity();
}

void CTFAnchor::ChargeThink(void)
{
	if ( IsOwnerAirborne() && !m_bIsDraining )
	{
		float flDuration = ANCHOR_TF2C_AIRBORNE_TIME;
		CALL_ATTRIB_HOOK_FLOAT(flDuration, mult_anchor_airborne_time);

		float flChargeAmount = gpGlobals->frametime / flDuration;
		float flNewLevel = max(m_flChargeMeter + flChargeAmount, 0.0f);
		m_flChargeMeter = min(flNewLevel, 1.0f);
	}
	else
	{
		if (m_flChargeMeter > 0.0f)
		{
			ChargeDrain();
			m_bIsDraining = true;
		}
		else
		{
			m_bIsDraining = false;
		}

		if (m_bSoundPlayed)
		{
			m_bSoundPlayed = false;
			StopSound("Weapon_Anchor.Fall");
		}
	}

	FallThink();
}

void CTFAnchor::ChargeDrain(void)
{
	float flChargeAmount = gpGlobals->frametime / 0.25f;
	float flNewLevel = max(m_flChargeMeter - flChargeAmount, 0.0f);
	m_flChargeMeter = max(flNewLevel, 0.0f);
}

void CTFAnchor::FallThink(void)
{
	if ( m_flChargeMeter >= 0.5f && !m_bSoundPlayed )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner && pOwner->GetAbsVelocity().z < 0.0f )
		{
			EmitSound("Weapon_Anchor.Fall");
			m_bSoundPlayed = true;
		}
	}
}

void CTFAnchor::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
	ChargeThink();
	ViewmodelThink();
}

void CTFAnchor::ItemBusyFrame(void)
{
	BaseClass::ItemBusyFrame();
	ChargeThink();
}

void CTFAnchor::ItemHolsterFrame(void)
{
	BaseClass::ItemHolsterFrame();
	m_flChargeMeter = 0.0f;
	if (m_bSoundPlayed)
	{
		StopSound("Weapon_Anchor.Fall");
		m_bSoundPlayed = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check for anchor raise conditions.
//-----------------------------------------------------------------------------
void CTFAnchor::ViewmodelThink(void)
{
	// reuse backstab activities bc why not - azzy
	if ( GetProgress() >= 0.75f )
	{
		if (GetActivity() == TranslateViewmodelHandActivity(ACT_VM_IDLE) || GetActivity() == TranslateViewmodelHandActivity(ACT_BACKSTAB_VM_IDLE))
		{
			if (!m_bReadyToEarthquake)
			{
				SendWeaponAnim(ACT_BACKSTAB_VM_UP);
			}
		}
	}
	else
	{
		if (m_bReadyToEarthquake)
		{
			SendWeaponAnim(ACT_BACKSTAB_VM_DOWN);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overriding so it doesn't do earthquake animation on crit.
//-----------------------------------------------------------------------------
void CTFAnchor::DoViewModelAnimation(void)
{
	Activity act = (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	SendWeaponAnim(act);
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to earthquake.
//-----------------------------------------------------------------------------
bool CTFAnchor::SendWeaponAnim(int iActivity)
{
	switch (iActivity)
	{
	case ACT_VM_IDLE:
		if (m_bReadyToEarthquake)
			iActivity = ACT_BACKSTAB_VM_IDLE;

		break;
	case ACT_BACKSTAB_VM_UP:
		m_bReadyToEarthquake = true;
		break;
	case ACT_BACKSTAB_VM_DOWN:
	default:
		m_bReadyToEarthquake = false;
		break;
	}

	return BaseClass::SendWeaponAnim(iActivity);
}

#endif // TF2C_BETA