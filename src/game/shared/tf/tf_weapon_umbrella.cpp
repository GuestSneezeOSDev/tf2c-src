//====== Copyright ? 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_umbrella.h"

//=============================================================================
//
// Weapon Club tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFUmbrella, DT_TFUmbrella )

BEGIN_NETWORK_TABLE( CTFUmbrella, DT_TFUmbrella )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flNextBoostAttack ) ),
#else
	SendPropTime( SENDINFO( m_flNextBoostAttack ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFUmbrella )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_umbrella, CTFUmbrella );
PRECACHE_WEAPON_REGISTER( tf_weapon_umbrella );

extern ConVar tf2c_vip_boost_time;
extern ConVar tf2c_vip_boost_cooldown;
extern ConVar tf2c_vip_abilities;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFUmbrella::CTFUmbrella()
{
	m_flNextBoostAttack = gpGlobals->curtime + tf2c_vip_boost_cooldown.GetFloat();
}

#ifdef GAME_DLL
// #define SAPPHO_WIFELEAVER_CANE
void CTFUmbrella::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.IsLoser() )
		return;

	if ( gpGlobals->curtime >= m_flNextNoTargetTime && gpGlobals->curtime < m_flNextBoostAttack )
	{
		CSingleUserRecipientFilter singleFilter( pPlayer );
		EmitSound( singleFilter, pPlayer->entindex(), "Player.DenyWeaponSelection" );
		m_flNextNoTargetTime = gpGlobals->curtime + 0.5f;
		return;
	}

	// Can't snap while attacking or outside of VIP mode.
	if ( tf2c_vip_abilities.GetInt() > 1 && gpGlobals->curtime >= m_flNextBoostAttack )
	{
		// Look for the player underneath the crosshair.
		bool bSuccess = false;
		trace_t tr;
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

		// Go far out.
		Vector vecStart = pPlayer->Weapon_ShootPosition();
		Vector vecEnd = vecStart + vecForward * MAX_TRACE_LENGTH;

		int bTargetEnemies = 0;
		CALL_ATTRIB_HOOK_ENUM_ON_OTHER( GetOwner(), bTargetEnemies, add_civ_boost_enemies );

		ITraceFilter *filter = NULL;

		if( !!bTargetEnemies )
			// Ignore teammates that are potentially getting in the way.
			filter = new CTraceFilterIgnoreTeammates(pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());
		else
			// Ignore enemies that are potentially getting in the way.
			filter = new CTraceFilterIgnoreEnemiesExceptSpies (pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber());

		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, filter, &tr );
		if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
		{
			CBaseEntity *pEntity = tr.m_pEnt;
			if ( pEntity && pEntity->IsPlayer() )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
				bool bCorrectTeam = !!bTargetEnemies ^ ( pTFPlayer->InSameTeam( pPlayer ) || ( pTFPlayer->m_Shared.IsDisguised() && pTFPlayer->m_Shared.DisguiseFoolsTeam( pPlayer->GetTeamNumber() ) ) );

                if (pTFPlayer && bCorrectTeam && pTFPlayer->m_flPowerPlayTime <= gpGlobals->curtime)
                {
#ifdef SAPPHO_WIFELEAVER_CANE
                    auto& target = pTFPlayer;
                    // not on ground
                    if (target->GetGroundEntity() == NULL)
                    {
                        return;
                    }

                    // scale the civ's viewang vector 512 units towards us
                    Vector yankVector = {};
                    VectorScale(vecForward, -512, yankVector);

                    // get pos of target so we can trace ground normal
                    Vector pos      = target->EyePosition();
                    Vector normal   = {};

                    trace_t groundTrace;
                    // draw line straight down from player to hit ground, ignore teammates and themself
                    UTIL_TraceLine(pos, { pos[0], pos[1], -1024.0 }, MASK_SOLID, filter, &groundTrace);
                    DebugDrawLine(pos, { pos[0], pos[1], -1024.0 }, 255, 255, 255, true, 2.0f);

                    if (groundTrace.DidHitWorld())
                    {
                        normal = groundTrace.plane.normal;
                    }

                    // ok now we got the normal of the ground
                    // we need to do some math to scale it according to the normal so we can yank people when they're on hills
                    // right now if you yank someone on a hill they will just not go anywhere if you're trying to yank them uphill
                    // and if they're being yanked downhill they'll probably go too far
                    // at some point in the future we also need to inverse scale by distance and probably set a cut off point for how far away
                    // you can be while still being able to yank someone
                    // TLDR THIS CODE IS NOT FINISHED
                    yankVector[2] = 256;

                    // Actually apply our velocity!
                    target->VelocityPunch(yankVector);

                    // Honestly it's going to be way more predictable if we can just put the player directly in front of the civilian
                    // instead of doing tons of math that will be fuzzy and might yeet players too far or too close or not at all
                    // Is there a way to "teleport" players so they end up at a guarunteed position? if so, that would be way better
                    // target->Teleport(&pos, nullptr, nullptr);
#else
					// Give 'em a little boost!
					string_t strBoostCondOverride = NULL_STRING;
					CALL_ATTRIB_HOOK_STRING_ON_OTHER(GetOwner(), strBoostCondOverride, add_civ_boost_override);
					if (strBoostCondOverride != NULL_STRING)
					{
						float args[2];
						UTIL_StringToFloatArray(args, 2, strBoostCondOverride.ToCStr());
						DevMsg("civ_boost_override: %2.2f, %2.2f\n", args[0], args[1]);

						pTFPlayer->m_Shared.AddCond((ETFCond)Floor2Int(args[0]), tf2c_vip_boost_time.GetFloat() * args[1], GetOwner());


						IGameEvent* event = gameeventmanager->CreateEvent("vip_boost");
						if (event)
						{
							event->SetInt("provider", pPlayer->GetUserID());
							event->SetInt("target", pTFPlayer->GetUserID()); // someone should rename these two vars!
							event->SetInt("condition", (ETFCond)Floor2Int(args[0]));

							gameeventmanager->FireEvent(event);
						}
					}
					else
					{
						pTFPlayer->m_Shared.AddCond(TF_COND_DAMAGE_BOOST, tf2c_vip_boost_time.GetFloat(), GetOwner());

						IGameEvent* event = gameeventmanager->CreateEvent("vip_boost");
						if (event)
						{
							event->SetInt("provider", pPlayer->GetUserID());
							event->SetInt("target", pTFPlayer->GetUserID()); // someone should rename these two vars!
							event->SetInt("condition", TF_COND_DAMAGE_BOOST);

							gameeventmanager->FireEvent(event);
						}
					}
#endif
					if( !!bTargetEnemies )
					{
						// Ah, was ist los?
						pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE );

						// Lets get to the fun part >:)
						pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
					}
					else
					{
						pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );

						// Go! Go! Go!
						pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_GO );
					}

					// Cool it down.
					m_flNextBoostAttack = gpGlobals->curtime + tf2c_vip_boost_cooldown.GetFloat();

					// Visual feedback.
					SendWeaponAnim( ACT_VM_SECONDARYATTACK );

					// Play sound from targeted player.
					CPVSFilter filter( pTFPlayer->GetAbsOrigin() );
					EmitSound( filter, pTFPlayer->entindex(), "TFPlayer.VIPBoost" );

					bSuccess = true;
				}
			}
		}

		if ( !bSuccess && gpGlobals->curtime >= m_flNextNoTargetTime )
		{
			CSingleUserRecipientFilter singleFilter( pPlayer );
			EmitSound( singleFilter, pPlayer->entindex(), "Player.DenyWeaponSelection" );
			m_flNextNoTargetTime = gpGlobals->curtime + 0.5f;
		}
	}
}
#endif


void CTFUmbrella::WeaponReset( void )
{
	m_flNextBoostAttack = gpGlobals->curtime + tf2c_vip_boost_cooldown.GetFloat();
	m_flNextNoTargetTime = 0.0f;
}


float CTFUmbrella::GetProgress( void )
{
	if ( gpGlobals->curtime <= m_flNextBoostAttack )
		return RemapValClamped( m_flNextBoostAttack - gpGlobals->curtime, tf2c_vip_boost_cooldown.GetFloat() * 1.0f, 0.0f, 0.0f, 1.0f );

	return 1.0f;
}
