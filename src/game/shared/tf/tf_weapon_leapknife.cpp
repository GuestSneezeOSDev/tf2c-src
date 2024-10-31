//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"

#include <in_buttons.h>
#include "tf_weapon_leapknife.h"
#include "decals.h"
#include "tf_lagcompensation.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

#ifdef TF2C_BETA

//=============================================================================
//
// Weapon Knife tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFLeapKnife, DT_TFWeaponLeapKnife );

BEGIN_NETWORK_TABLE( CTFLeapKnife, DT_TFWeaponLeapKnife )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bReadyToBackstab ) ),
    RecvPropFloat( RECVINFO( m_flLeapCooldown ) ),
#else
	SendPropBool( SENDINFO( m_bReadyToBackstab ) ),
    SendPropFloat( SENDINFO( m_flLeapCooldown ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLeapKnife )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bReadyToBackstab, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_flLeapCooldown, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_leapknife, CTFLeapKnife );
PRECACHE_WEAPON_REGISTER( tf_weapon_leapknife );

//=============================================================================
//
// Weapon Knife functions.
//

ConVar tf2c_spy_leap_distance           ("tf2c_spy_leap_distance",              "220.0",    FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar tf2c_spy_leap_distance_vertical  ("tf2c_spy_leap_distance_vertical",     "128.0",    FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar tf2c_spy_leap_cooldown           ("tf2c_spy_leap_cooldown",              "4.0",      FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar tf2c_spy_leap_cloak_drain_percent("tf2c_spy_leap_cloak_drain_percent",   "25.0",     FCVAR_NOTIFY | FCVAR_REPLICATED);


CTFLeapKnife::CTFLeapKnife()
{
    m_flLeapCooldown = 0.0f;
    m_flNextErrorBuzzTime = 0.0f;
}

void CTFLeapKnife::Precache()
{
    BaseClass::Precache();
    PrecacheScriptSound("Weapon_Grenade_Mirv.Throw");
}

void CTFLeapKnife::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

    LeapThink();
}

float CTFLeapKnife::GetProgress()
{
    float flTime = tf2c_spy_leap_cooldown.GetFloat();
    return (flTime - (m_flLeapCooldown - gpGlobals->curtime)) / flTime;
}

void CTFLeapKnife::ItemBusyFrame()
{
    BaseClass::ItemBusyFrame();

    LeapThink();
}

void CTFLeapKnife::ItemHolsterFrame()
{
	BaseClass::ItemHolsterFrame();

    LeapThink();
}

void CTFLeapKnife::PlayErrorSoundIfWeShould()
{
#ifdef CLIENT_DLL
    if (gpGlobals->curtime >= m_flNextErrorBuzzTime)
    {
        CLocalPlayerFilter filter;
        EmitSound( filter, entindex(), "Player.DenyWeaponSelection" );
        m_flNextErrorBuzzTime = gpGlobals->curtime + 0.5f; // Only buzz every so often.
    }
#endif
}

void CTFLeapKnife::LeapThink()
{
    // TODO: item effect meter thing
    // TODO: check this math
    // TODO: impl m_flLastLeapTime
    // TODO: 8 second long cooldown
    CTFPlayer *pPlayerOwner = GetTFPlayerOwner();

    // Gone.
    if ( !pPlayerOwner )
        return;

    if ( pPlayerOwner->m_nButtons & IN_DUCK && pPlayerOwner->m_nButtons & IN_JUMP )
    {
        if ( !((pPlayerOwner->m_Shared.IsStealthed() || pPlayerOwner->m_Shared.InCond(TF_COND_STEALTHED_BLINK))) )
        {
            PlayErrorSoundIfWeShould();
            return;
        }

        float cloakAmt = pPlayerOwner->m_Shared.GetSpyCloakMeter();

        if (cloakAmt < tf2c_spy_leap_cloak_drain_percent.GetFloat())
        {
            // we dont have enough cloak!
            PlayErrorSoundIfWeShould();
            return;
        }

        if (m_flLeapCooldown >= gpGlobals->curtime)
        {
            PlayErrorSoundIfWeShould();
            return;
        }

        m_flLeapCooldown = gpGlobals->curtime + tf2c_spy_leap_cooldown.GetFloat();

        pPlayerOwner->m_Shared.SetSpyCloakMeter(cloakAmt - tf2c_spy_leap_cloak_drain_percent.GetFloat());

        QAngle angles = pPlayerOwner->EyeAngles();
        Vector forward = vec3_origin;
        Vector velocity = vec3_origin;

        AngleVectors(angles, &forward);

        forward.z = 1.0f;

        // make it usable
        float flDistance = tf2c_spy_leap_distance.GetFloat();
        Vector forwardScaled = vec3_origin;
        VectorMultiply(forward, flDistance, forwardScaled);

        // add it to the current velocity to avoid just being able to do full 180s
        velocity = pPlayerOwner->GetLocalVelocity();

        Vector newVelocity;
        VectorAdd(velocity, forwardScaled, newVelocity);

        float flDistanceVertical = tf2c_spy_leap_distance_vertical.GetFloat();

        newVelocity[2] += flDistanceVertical; // we always want to go a bit up

        EmitSound("Weapon_Grenade_Mirv.Throw");

        pPlayerOwner->SetAbsVelocity(newVelocity);
        pPlayerOwner->SetBaseVelocity(vec3_origin);

#ifdef GAME_DLL
        pPlayerOwner->m_bSelfKnockback = true;
#endif

        pPlayerOwner->m_Shared.AddCond(TF_COND_TELEPORTED_ALWAYS_SHOW, 5.0);
    }
}

#endif