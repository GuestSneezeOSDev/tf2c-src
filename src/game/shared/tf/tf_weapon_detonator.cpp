//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_detonator.h"
#include "IEffects.h"
#include "tf_lagcompensation.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "prediction.h"
// Server specific.
#else
	#include "tf_gamerules.h"
	#include "tf_player.h"
	#include "tf_gamestats.h"
	#include "tf_fx.h"
#endif

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion

ConVar tf2c_detonator_selfdamage("tf2c_detonator_selfdamage", "35", FCVAR_NOTIFY | FCVAR_REPLICATED, "Self-damage you take if you managed to kill someone with detonator");

IMPLEMENT_NETWORKCLASS_ALIASED(TFDetonator, DT_TFDetonator)
BEGIN_NETWORK_TABLE(CTFDetonator, DT_TFDetonator)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bKilledSomeone)),
#else
SendPropBool(SENDINFO(m_bKilledSomeone)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFDetonator)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_bKilledSomeone, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_detonator, CTFDetonator);
PRECACHE_WEAPON_REGISTER(tf_weapon_detonator);

//=============================================================================
//
// Weapon Detonator functions.
//



void CTFDetonator::Precache()
{
	PrecacheScriptSound("Weapon_Detonator.Charging");

	PrecacheTeamParticles("player_sparkles_%s");

	BaseClass::Precache();
}

void CTFDetonator::WeaponReset(void)
{
	m_bKilledSomeone = false;
	BaseClass::WeaponReset();
}

void CTFDetonator::PrimaryAttack(void)
{
	// Copy paste from base PrimaryAttack, PrimaryAttack doesn't return bool after all to check
	// Perhaps make it do so?
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	if (!CanAttack())
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// Swing the weapon.
	Swing(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired(this);
#endif
	//End of primary attack copy-paste

	pPlayer->m_Shared.AddCond(TF_COND_CANNOT_SWITCH_FROM_MELEE);
	pPlayer->m_Shared.AddCond(TF_COND_SPEEDBOOST_DETONATOR);

	// Make a sound
	CPASAttenuationFilter filter(pPlayer, "Weapon_ProxyBomb.LeadIn");

	EmitSound_t beep;
	beep.m_pSoundName = "Weapon_ProxyBomb.LeadIn";
	beep.m_flSoundTime = 0.0f;
	beep.m_pflSoundDuration = nullptr;
	beep.m_nPitch = PITCH_NORM + 5;
	beep.m_nFlags |= SND_CHANGE_PITCH;
	beep.m_bWarnOnDirectWaveReference = true;

	EmitSound("Weapon_ProxyBomb.LeadIn");

#ifdef GAME_DLL
	/*if (pPlayer->IsSpeaking())
	{
		CMultiplayer_Expresser *pExpresser = pPlayer->GetMultiplayerExpresser();
		if (pExpresser)
		{
			pExpresser->ForceNotSpeaking();
		}
	}
	pPlayer->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_DETONATOR_ACTIVATION);*/

	CPVSFilter filterp(GetAbsOrigin());

	const char *pszSparklesEffect = ConstructTeamParticle("player_sparkles_%s", GetTeamNumber());
	TE_TFParticleEffect(filterp, 0.0f, pszSparklesEffect, PATTACH_ABSORIGIN, pPlayer);

	EmitSound("Weapon_Detonator.Charging");
#endif

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired(this);
#endif
}

void CTFDetonator::Smack(void)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION(pPlayer, pPlayer->GetCurrentCommand());

#ifndef CLIENT_DLL
	StopSound("Weapon_Detonator.Charging");

	// Start with fancy effects
	CDisablePredictionFiltering disabler;

	Vector vecReported = pPlayer->WorldSpaceCenter();

	CPVSFilter filter(pPlayer->WorldSpaceCenter());
	//TE_TFExplosion(filter, 0.0f, pPlayer->WorldSpaceCenter(), vec3_origin, GetWeaponID(), pPlayer->entindex(), pPlayer, GetTeamNumber(), true, GetItemID());
	te->Explosion(filter, -1.0, // don't apply cl_interp delay
		&pPlayer->WorldSpaceCenter(),
		!(UTIL_PointContents(pPlayer->WorldSpaceCenter()) & MASK_WATER) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
		GetTFWpnData().m_flDamageRadius * .03,
		25,
		TE_EXPLFLAG_NONE,
		GetTFWpnData().m_flDamageRadius,
		GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage);
	EmitSound("BaseGrenade.Explode");
	// the funny explosion version


	trace_t	tr;
	Vector vecEnd = GetAbsOrigin() - Vector(0, 0, 60);
	UTIL_TraceLine(GetAbsOrigin(), vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (tr.m_pEnt && !tr.m_pEnt->IsPlayer())
		UTIL_DecalTrace(&tr, "Scorch");

	UTIL_ScreenShake(GetAbsOrigin(), 15.0, 150.0, 1.0, 400.0, SHAKE_START);

	// Blow up everyone
	float flDamage = GetTFWpnData().GetWeaponData(m_iWeaponMode).m_flDamage;
	int iDmgType = GetDamageType();
	if (IsCurrentAttackACrit())
	{
		iDmgType |= DMG_CRITICAL;
	}
	float flRadius = GetTFWpnData().m_flDamageRadius;
	if (flRadius <= 0.0f) flRadius = 64.0f;

	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info.Set(pPlayer, pPlayer, this, vec3_origin, pPlayer->WorldSpaceCenter(), flDamage, iDmgType, TF_DMG_CUSTOM_NONE, &vecReported);
	radiusInfo.m_vecSrc = pPlayer->WorldSpaceCenter();
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_pEntityIgnore = pPlayer;

	TFGameRules()->RadiusDamage(radiusInfo);

	if (pPlayer->IsAlive()) //check just in case
	{
		// And blow up yourself
		CTakeDamageInfo selfDamage;

		if (m_bKilledSomeone)
			selfDamage.Set(pPlayer, pPlayer, this, vec3_origin, pPlayer->WorldSpaceCenter(), tf2c_detonator_selfdamage.GetInt(), iDmgType, TF_DMG_CUSTOM_BUDDHA, &vecReported);
		else
			selfDamage.Set(pPlayer, pPlayer, this, vec3_origin, pPlayer->WorldSpaceCenter(), pPlayer->GetHealth() * 2, iDmgType, TF_DMG_CUSTOM_NONE, &vecReported);

		pPlayer->TakeDamage(selfDamage);
	}

#endif

	FINISH_LAG_COMPENSATION();

	pPlayer->m_Shared.RemoveCond(TF_COND_CANNOT_SWITCH_FROM_MELEE); 
	pPlayer->m_Shared.RemoveCond(TF_COND_SPEEDBOOST_DETONATOR);

	m_bKilledSomeone = false;
}