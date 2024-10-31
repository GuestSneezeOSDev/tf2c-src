//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_gamestats.h"
#else
#include "c_tf_player.h"
#endif

#include "engine/ivdebugoverlay.h"
#include "tf_weapon_riot.h"

//=============================================================================
//
// Weapon Riot tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFRiot, DT_TFRiot )
BEGIN_NETWORK_TABLE( CTFRiot, DT_TFRiot )
#ifdef GAME_DLL
	SendPropFloat( SENDINFO( m_flChargeMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bBroken ) ),
#else
	RecvPropFloat( RECVINFO(m_flChargeMeter) ),
	RecvPropBool( RECVINFO(m_bBroken) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRiot )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_riot, CTFRiot );
PRECACHE_WEAPON_REGISTER( tf_weapon_riot );

#define TF_SHIELD_BLOCK_ANGLE		tf2c_shield_block_angle.GetFloat()
#define TF_SHIELD_BLOCK_UP_ANGLE	tf2c_shield_block_up_angle.GetFloat()
#define TF_SHIELD_REGEN_COOLDOWN	tf2c_shield_regen_cooldown.GetFloat()
#define TF_SHIELD_REGEN_RATE		tf2c_shield_regen_rate.GetInt()
#define TF_SHIELD_CHARGE_DURATION	tf2c_shield_charge_duration.GetFloat()
#define TF_SHIELD_CHARGE_REGENTIME	tf2c_shield_charge_regen.GetFloat()

extern ConVar tf2c_infinite_ammo;
#ifdef GAME_DLL
extern ConVar tf_meleeattackforcescale;
#endif

#define TF_TROLL FCVAR_NONE // FCVAR_DEVELOPMENTONLY

ConVar tf2c_shield_regen_cooldown ( "tf2c_shield_regen_cooldown" , "1.5", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_regen_rate	  ( "tf2c_shield_regen_rate"	 , "30"	, FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_charge_duration( "tf2c_shield_charge_duration", "1"	, FCVAR_REPLICATED | TF_TROLL ); 
ConVar tf2c_shield_charge_regen	  ( "tf2c_shield_charge_regen"	 , "10"	, FCVAR_REPLICATED | TF_TROLL );  
ConVar tf2c_shield_forward( "tf2c_shield_forward", "20", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_min_x( "tf2c_shield_min_x", "-5", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_min_y( "tf2c_shield_min_y", "-25", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_min_z( "tf2c_shield_min_z", "-45", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_max_x( "tf2c_shield_max_x", "40", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_max_y( "tf2c_shield_max_y", "25", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_max_z( "tf2c_shield_max_z", "45", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_debug( "tf2c_shield_debug", "0", FCVAR_REPLICATED | TF_TROLL );
ConVar tf2c_shield_drawwireframe("tf2c_shield_drawwireframe", "0", FCVAR_REPLICATED | TF_TROLL);

CTFRiot::CTFRiot( void )
{
	m_flChargeMeter = 1.0f;
	m_bBroken = false;
#ifdef GAME_DLL
	m_flTimeSlow = 0.0f;
#endif
}

void CTFRiot::Precache( void )
{
	PrecacheScriptSound( "DemoCharge.HitFleshRange" );
	PrecacheScriptSound( "DemoCharge.HitWorld" );
	BaseClass::Precache();
}

#ifdef GAME_DLL
/*
	Vector *pVerts = new Vector[8];
	matrix3x4_t fRotateMatrix;
	AngleMatrix( angEyes, fRotateMatrix );

	
	Formerly used to create the angled box
	No longer needed since the angle is handled in the TraceBox function
	Remains here in case its ever required again - Kay
	Vector vecPos;
	for ( int i = 0; i < 8; ++i )
	{
		vecPos[0] = ( i & 0x1 ) ? vecShieldMaxs[0] : vecShieldMins[0];
		vecPos[1] = ( i & 0x2 ) ? vecShieldMaxs[1] : vecShieldMins[1];
		vecPos[2] = ( i & 0x4 ) ? vecShieldMaxs[2] : vecShieldMins[2];

		VectorRotate( vecPos, fRotateMatrix, pVerts[i] );
		pVerts[i] += vecShieldOrigin;
	}
*/

#define TF_SHIELD_DEBUG_TIME	10
#define TF_PLAYER_MID_ORIGIN (pOwner->GetAbsOrigin() + Vector(0, 0, pOwner->GetPlayerMaxs().z / 2))

//-----------------------------------------------------------------------------
// Purpose: The heart and soul of the shield, handles collision and damage absorption
//-----------------------------------------------------------------------------
int CTFRiot::AbsorbDamage( CTakeDamageInfo &info )
{
	// Don't bother blocking healing or no damage
	if( info.GetDamage() <= 0 )
		return 0;

	// Bleeding, burning, telefrags and fall damage don't get blocked
	if( IsDOTDmg(info)
		/*info.GetDamageCustom() == TF_DMG_CUSTOM_BLEEDING
		// WELL APPARENTLY all burning based weapons use this, not just afterburn soo
		// can't use this, thankfully afterburn misses still because its considered inside the player
		// || info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING*/
		|| info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG
		|| info.GetDamageCustom() == TF_DMG_FALL_DAMAGE )
		return 0;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if( !pOwner )
		return 0;

	// May god or any other benevolent entity have mercy for i have used Damage types to check if its a hitscan or not - Kay
	bool bHitscan = (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) && info.GetAttacker();

	Vector vecForward;
	QAngle angEyes = pOwner->EyeAngles();
	angEyes.x = max(min( angEyes.x, 30 ), -60); // Restrict the max angle our shield can have
	AngleVectors( angEyes, &vecForward ); // Create the forward vector based on our restricted eye angle
	
	// Move the shield slightly more forward/backward if we're looking up/down
	// This helps with making it properly stay in front of the heavy
	float flForward = RemapValClamped( angEyes.x, -60, 30, tf2c_shield_forward.GetFloat() * 2, tf2c_shield_forward.GetFloat() / 2 );

	Vector vecDamagePos = info.GetDamagePosition();

	// Damage pos from non hitscan weapons gets moved slightly outwards to aid in damage detection
	if( !bHitscan )
	{
		Vector vecDir = vecDamagePos;
		vecDir -= TF_PLAYER_MID_ORIGIN;
		VectorNormalize(vecDir);
		vecDamagePos = vecDamagePos + (vecDir * 200);
	}

	// Now get our shield origin
	Vector vecShieldOrigin = TF_PLAYER_MID_ORIGIN + (vecForward * flForward);

	// Construct the Mins/Maxs with our ConVars
	Vector vecShieldMins( tf2c_shield_min_x.GetFloat(), tf2c_shield_min_y.GetFloat(), tf2c_shield_min_z.GetFloat() );
	Vector vecShieldMaxs( tf2c_shield_max_x.GetFloat(), tf2c_shield_max_y.GetFloat(), tf2c_shield_max_z.GetFloat() );

	// Create the basic collision mesh as a Bounding box
	CPhysCollide *pShieldCollide = physcollision->BBoxToCollide( vecShieldMins, vecShieldMaxs );

	// Hitscans, as we know the exact place where they originate from
	// can use a direct trace from one to the other
	Vector vecStartTrace, vecEndTrace;
	if( bHitscan )
	{
		vecStartTrace = info.GetAttacker()->EyePosition();
		vecEndTrace = vecDamagePos;
	}
	// For non hitscan damage, from explosions, projectiles or otherwise
	// We only know the damage position
	// So all we can really do is compare it against the middle of the shielded player
	else
	{
		vecStartTrace = vecDamagePos;
		vecEndTrace = TF_PLAYER_MID_ORIGIN;
	}

	if( tf2c_shield_debug.GetBool() )
	{
		// Damage Position
		NDebugOverlay::Sphere( info.GetDamagePosition(), 10, 0, 255, 0, false, TF_SHIELD_DEBUG_TIME );
		// The shield itself
		NDebugOverlay::BoxAngles( vecShieldOrigin, vecShieldMins, vecShieldMaxs, angEyes, 0, 255, 0, 0, TF_SHIELD_DEBUG_TIME );
		// The original, non extended trace
		NDebugOverlay::Line( vecStartTrace, vecEndTrace, 0, 255, 0, true, TF_SHIELD_DEBUG_TIME );
	}

	// Since a straight line from the damage origin to the damage position
	// might not touch the shield itself, but just the heavy
	// we extend our line so it definitely crosses the shield
	Vector vecDir = vecEndTrace - vecStartTrace; // Get our direction vector
	VectorNormalize(vecDir); // Normalise it
	float flDist = FloatMakePositive(vecStartTrace.DistTo(vecEndTrace)); // Get the original distance
	Vector vecStartOriginal = vecStartTrace, vecEndOriginal = vecEndTrace;
	vecStartTrace = vecEndOriginal + (-vecDir * 3 * flDist); // Extend our starting point to be 3 times its original distance away from the end
	vecEndTrace = vecStartOriginal + (vecDir * 3 * flDist); // Extend our end point to be 3 times its original distance away from the start

	// Now, since we create a straight line through the heavy
	// damage from the back is gonna hit the shield
	// To detect if we actually hit the shield first before the heavy
	// we make one trace from the front, and one from the back
	// If the back is hit first, we know that the heavy was hit before the shield
	// So the damage should not be absorbed
	// If however the shield was hit first, we block the damage
	trace_t trace;
	trace_t trace_backwards;
	physcollision->TraceBox( vecStartTrace, vecEndTrace, Vector(-1,-1,-1),Vector(1,1,1), pShieldCollide, vecShieldOrigin, angEyes, &trace );
	physcollision->TraceBox( vecEndTrace, vecStartTrace, Vector(-1,-1,-1),Vector(1,1,1), pShieldCollide, vecShieldOrigin, angEyes, &trace_backwards );

	if( tf2c_shield_debug.GetBool() )
	{
		NDebugOverlay::Line( vecStartTrace, vecEndTrace, 255, 0, 0, true, TF_SHIELD_DEBUG_TIME );
		NDebugOverlay::Sphere( trace.endpos, 10, 0, 0, 255, false, TF_SHIELD_DEBUG_TIME );
		NDebugOverlay::Sphere( trace_backwards.endpos, 10, 255, 0, 0, false, TF_SHIELD_DEBUG_TIME );
	}

	// Collision detection has been done now, destroy the collision object
	physcollision->DestroyCollide(pShieldCollide);

	// If we didn't hit anything, just skip
	if( trace.fraction >= 1.0f && !trace.startsolid )
		return 0;

	// Make sure we hit the shield before the heavy
	if( trace.endpos.DistToSqr( TF_PLAYER_MID_ORIGIN ) < trace_backwards.endpos.DistToSqr( TF_PLAYER_MID_ORIGIN ) )
		return 0;

	// If we have infinite ammo, we're always gonna block all of the damage
	if( tf2c_infinite_ammo.GetBool() )
	{
		m_flBlockedDamage = info.GetDamage();
		return info.GetDamage();
	}

	// Max damage we can absorb
	int iRet = pOwner->GetAmmoCount( GetPrimaryAmmoType() );

	// Take the shield's health away
	pOwner->RemoveAmmo( info.GetDamage(), GetPrimaryAmmoType() );

	// Calculate how much we absorbed
	iRet -= pOwner->GetAmmoCount( GetPrimaryAmmoType() );

	// If we ran out of ammo, break the shield
	if( pOwner->GetAmmoCount( GetPrimaryAmmoType() ) <= 0 )
		BreakShield();

	// Give our little fella the support points they deserve :)
	CTF_GameStats.Event_PlayerBlockedDamage( pOwner, iRet );

	// Save the blocked damage so it can be used in the damage event
	m_flBlockedDamage = iRet;

	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose: When we have 0 shield health left, this triggers
//-----------------------------------------------------------------------------
void CTFRiot::BreakShield( void )
{
	// If the shield breaks, take twice as long to start regenerating
	m_flRegenStartTime = gpGlobals->curtime + (TF_SHIELD_REGEN_COOLDOWN * 2);
	m_bBroken = true;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if( pOwner /*&& !pOwner->Weapon_Switch( pOwner->GetLastWeapon() )*/ )
	{
		pOwner->m_Shared.StopCharge();
		pOwner->SwitchToNextBestWeapon( this );
	}
}
#else
void CTFRiot::DrawShieldWireframe( int iCurrentAmmo )
{
	if (!tf2c_shield_drawwireframe.GetBool())
		return;
	// Here we create a visualiser for our shield
	// in and of itself it does nothing
	// Its just a temporary solution to not having finished anims
	// or a finished model

	// Vectors here are identical to the AbsorbDamage function
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if( !pOwner )
		return;

	Vector vecForward;
	QAngle angEyes = pOwner->EyeAngles();
	angEyes.x = max(min( angEyes.x, 30 ), -60);
	AngleVectors( angEyes, &vecForward );
	
	float flForward = RemapValClamped( angEyes.x, -60, 30, tf2c_shield_forward.GetFloat() * 2, tf2c_shield_forward.GetFloat() / 2 );

	Vector vecShieldOrigin = ( pOwner->GetAbsOrigin() + Vector( 0, 0, pOwner->GetPlayerMaxs().z / 2 ) ) + ( vecForward * flForward );
	Vector vecShieldMins( tf2c_shield_min_x.GetFloat(), tf2c_shield_min_y.GetFloat(), tf2c_shield_min_z.GetFloat() );
	Vector vecShieldMaxs( tf2c_shield_debug.GetBool() ? tf2c_shield_max_x.GetFloat() : tf2c_shield_max_x.GetFloat() / 2.0f, tf2c_shield_max_y.GetFloat(), tf2c_shield_max_z.GetFloat() );	
	
	float flProgress = pOwner->IsLocalPlayer() ? GetEffectBarProgress() : (float)iCurrentAmmo / (float)pOwner->GetMaxAmmo( GetPrimaryAmmoType() );

	// Seperate functions for server and client because of course the client can't use NDebugOverlay
	if( debugoverlay )
		debugoverlay->AddBoxOverlay( 
			vecShieldOrigin,
			vecShieldMins,
			vecShieldMaxs,
			angEyes,
			Float2Int(255.0f * (1.0f - flProgress)), Float2Int(255.0f * flProgress), 0, 0, 
			gpGlobals->frametime );
	}
#endif

bool CTFRiot::CanHolster( void ) const
{
	// Don't holster while charging
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if( pOwner && pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Holster function, we're adding the regen delay here
//-----------------------------------------------------------------------------
bool CTFRiot::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Wait a bit before we start regenerating
	float flRegenTime = gpGlobals->curtime + TF_SHIELD_REGEN_COOLDOWN;

	// Unless our current regen time is longer than the default one
	if( flRegenTime > m_flRegenStartTime )
		m_flRegenStartTime = flRegenTime;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Charge on secondary attack
//-----------------------------------------------------------------------------
void CTFRiot::SecondaryAttack( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// If we're not charging already and we have a full charge meter
	// CHARGE!!
	if( !pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) && m_flChargeMeter >= 1.0f )
	{
		pPlayer->m_Shared.AddCond( TF_COND_SHIELD_CHARGE, -1.0f, this );

		// Sound currently just placeholder
		// Potentially to be moved onto a voice line
		pPlayer->EmitSound( "Heavy.BattleCry05" );
		return;
	}

	// Otherwise do our special class skill
	pPlayer->DoClassSpecialSkill();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: When we're ressuplying, regen our charge
//-----------------------------------------------------------------------------
void CTFRiot::WeaponRegenerate( void )
{
	BaseClass::WeaponRegenerate();

	m_flChargeMeter = 1.0f;
}
#endif

bool CTFRiot::UsesPrimaryAmmo( void )
{
	// Melees are coded to not use ammo by default, so we skip that
	return CBaseCombatWeapon::UsesPrimaryAmmo();
}

// The shield charge think is supposed to always be active
// So we forward the think call to the shield within all the think functions

// Think whenever the weapon cannot fire
// aka during deploy/reload etc
void CTFRiot::ItemBusyFrame( void )
{
	ShieldChargeThink();
	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Think function when weapon is idle
//-----------------------------------------------------------------------------
void CTFRiot::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if( !pOwner )
		return;

	ShieldChargeThink();
	BaseClass::ItemPostFrame();
}

void CTFRiot::ItemHolsterFrame( void )
{
#ifdef GAME_DLL
	// Regenerate shield health if our time has passed
	if( m_flRegenStartTime <= gpGlobals->curtime )
	{
		m_flAmmoToGive += gpGlobals->frametime * TF_SHIELD_REGEN_RATE;

		int iGivenAmmo = Floor2Int(m_flAmmoToGive);
		m_flAmmoToGive -= iGivenAmmo;
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if( pOwner )
			pOwner->GiveAmmo( iGivenAmmo, GetPrimaryAmmoType(), true );

		if( pOwner->GetAmmoCount(GetPrimaryAmmoType()) >= pOwner->GetMaxAmmo(GetPrimaryAmmoType(), true) )
			m_bBroken = false;
	}
#endif

	ShieldChargeThink();
	BaseClass::ItemHolsterFrame();
}

// This basically handles the charge meter
void CTFRiot::ShieldChargeThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if( !pOwner )
		return;

	// If we're charging, take away charge meter
	if( pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		float flChargeAmount = gpGlobals->frametime / TF_SHIELD_CHARGE_DURATION;
		float flNewLevel = max( m_flChargeMeter - flChargeAmount, 0.0 );
		m_flChargeMeter = flNewLevel;

		/* If we want crits for the shield, we can re-enable this 
		TF_SHIELD_CRIT_AT_PERCENT is how much percent ( in 1.0 to 0.0 ) you need to gain crits
		if ( m_flChargeMeter <= TF_SHIELD_CRIT_AT_PERCENT && !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
			pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );*/

		// If we ran out of meter, stop charging
		if( m_flChargeMeter <= 0.0f )
		{
			pOwner->m_Shared.RemoveCond( TF_COND_SHIELD_CHARGE );
			// pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
		}
	}
	// If we're out of meter and not charging, regenerate
	else if( m_flChargeMeter < 1.0f )
	{
		float flChargeAmount = gpGlobals->frametime / TF_SHIELD_CHARGE_REGENTIME;
		float flNewLevel = min( m_flChargeMeter + flChargeAmount, 1 );
		m_flChargeMeter = flNewLevel;
	}
}

void CTFRiot::Smack( void )
{
	BaseClass::Smack();
	// Stop our charge when our melee hit connects
	if( GetTFPlayerOwner() && GetTFPlayerOwner()->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		GetTFPlayerOwner()->m_Shared.StopCharge();
}

void CTFRiot::StopCharge( void )
{
	m_flChargeMeter = 0.0f;
#ifdef GAME_DLL
	m_flTimeSlow = 0.0f;
#endif
}

// Here start the various meter HUD interface functions

float CTFRiot::GetEffectBarProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if( pPlayer && pPlayer->GetMaxAmmo( GetPrimaryAmmoType() ) > 0 )
	{
		return (float)pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) / (float)pPlayer->GetMaxAmmo( GetPrimaryAmmoType() );
	}

	return 1.0f;
}

float CTFRiot::InternalGetEffectBarRechargeTime( void )
{
	return TF_SHIELD_CHARGE_REGENTIME;
}

float CTFRiot::GetChargeBeginTime( void )
{
	return 0;
}

float CTFRiot::GetChargeMaxTime( void )
{
	return TF_SHIELD_CHARGE_DURATION;
}

float CTFRiot::GetCurrentCharge( void )
{
	return m_flChargeMeter;
}
