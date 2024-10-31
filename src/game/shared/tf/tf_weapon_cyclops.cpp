//====== Copyright 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_cyclops.h" 
#include "tf_fx_shared.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif
#include <in_buttons.h>

//=============================================================================
//
// Weapon Brimstone Launcher tables.
//
#ifdef TF2C_BETA

IMPLEMENT_NETWORKCLASS_ALIASED(TFCyclops, DT_TFCyclops);
BEGIN_NETWORK_TABLE(CTFCyclops, DT_TFCyclops)

#ifdef GAME_DLL
SendPropInt(SENDINFO(m_iCyclopsGrenadeCount), 5, SPROP_UNSIGNED),
#else
RecvPropInt(RECVINFO(m_iCyclopsGrenadeCount)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFCyclops)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_iCyclopsGrenadeCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_cyclops, CTFCyclops);
PRECACHE_WEAPON_REGISTER(tf_weapon_cyclops);

CTFCyclops::CTFCyclops()
{
	m_bReloadsSingly = true;
}

void CTFCyclops::AddCyclopsGrenade(CBaseEntity* pCyclopsGrenade)
{
	CyclopsGrenadeHandle hHandle = pCyclopsGrenade;
	m_CyclopsGrenades.AddToTail(hHandle);
	m_iCyclopsGrenadeCount = m_CyclopsGrenades.Count();
}

void CTFCyclops::RemoveCyclopsGrenade(CBaseEntity* pCyclopsGrenade)
{
	CyclopsGrenadeHandle hHandle = assert_cast<CBaseEntity*>(pCyclopsGrenade);
	m_CyclopsGrenades.FindAndRemove(hHandle);
	m_iCyclopsGrenadeCount = m_CyclopsGrenades.Count();
}

void CTFCyclops::PrimaryAttack(void)
{
	if (m_iCyclopsGrenadeCount)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
		return;
	}

	BaseClass::PrimaryAttack();
}

bool CTFCyclops::Reload()
{
	// Can't reload if there's no one to reload us.
	CTFPlayer * pOwner = GetTFPlayerOwner();
	if (!pOwner)
		return false;

	// actual cyclops part
	if (m_iCyclopsGrenadeCount)
		return false;
	
	return BaseClass::Reload();
}

void CTFCyclops::GiveDefaultAmmo(void)
{
	if (m_iCyclopsGrenadeCount)
		return;

	BaseClass::GiveDefaultAmmo();
}
#endif