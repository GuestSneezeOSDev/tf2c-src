//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "in_buttons.h"
#include "movevars_shared.h"
#include "collisionutils.h"
#include "debugoverlay_shared.h"
#include "baseobject_shared.h"
#include "particle_parse.h"
#include "baseobject_shared.h"
#include "coordsize.h"
#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_world.h"
#include "c_team.h"
#include "c_te_effect_dispatch.h"

#define CTeam C_Team
#else
#include "tf_player.h"
#include "team.h"
#include "te_effect_dispatch.h"
#endif

ConVar	tf_maxspeed( "tf_maxspeed", "400", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_showspeed( "tf_showspeed", "0", FCVAR_REPLICATED );
ConVar	tf_avoidteammates( "tf_avoidteammates", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_avoidteammates_pushaway( "tf_avoidteammates_pushaway", "1", FCVAR_REPLICATED, "Whether or not teammates push each other away when occupying the same space" );
ConVar  tf_solidobjects( "tf_solidobjects", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_clamp_back_speed( "tf_clamp_back_speed", "0.9", FCVAR_REPLICATED );
ConVar  tf_clamp_back_speed_min( "tf_clamp_back_speed_min", "100", FCVAR_REPLICATED );
ConVar	tf_clamp_airducks( "tf_clamp_airducks", "1", FCVAR_REPLICATED );

ConVar	tf2c_bunnyjump_max_speed_factor( "tf2c_bunnyjump_max_speed_factor", "1.2", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar  tf2c_autojump( "tf2c_autojump", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow player to continuously jump by holding down the jump button." );
ConVar  tf2c_duckjump( "tf2c_duckjump", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles jumping while ducked" );
ConVar  tf2c_groundspeed_cap( "tf2c_groundspeed_cap", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the max speed cap imposed when a player is standing on the ground." );
ConVar	tf2c_airdash_disable( "tf2c_airdash_disable", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disables scout doublejump");

// !!! foxysen grav
#ifdef TF2C_BETA
ConVar  tf2c_grav_boots_jumppad_cond_duration("tf2c_grav_boots_jumppad_cond_duration", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of jumppad condition (no fall damage and stomp) applied on double jump with anti-gravity boots");
#else
ConVar  tf2c_grav_boots_jumppad_cond_duration("tf2c_grav_boots_jumppad_cond_duration", "0.5", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of jumppad condition (no fall damage and stomp) applied on double jump with anti-gravity boots");
#endif
ConVar  tf2c_grav_boots_max_horizontal_velocity_clamp("tf2c_grav_boots_max_horizontal_velocity_clamp", "325", FCVAR_NOTIFY | FCVAR_REPLICATED, "Max velocity on horizontal plane that you get clamped down to on use of anti-gravity boots");
ConVar  tf2c_grav_boots_deadzone_negative_range("tf2c_grav_boots_deadzone_negative_range", "-150", FCVAR_NOTIFY | FCVAR_REPLICATED, "Deadzone lower range on anti-gravity boots");
ConVar  tf2c_grav_boots_deadzone_positive_range("tf2c_grav_boots_deadzone_positive_range", "400", FCVAR_NOTIFY | FCVAR_REPLICATED, "Deadzone upper range on anti-gravity boots");
ConVar  tf2c_grav_boots_deadzone_velocity("tf2c_grav_boots_deadzone_velocity", "-400", FCVAR_NOTIFY | FCVAR_REPLICATED, "Negative velocty applied within deadzone range on anti-gravity boots");

ConVar tf2c_self_knockback_min_velocity_for_event("tf2c_self_knockback_min_velocity_for_event", "550", FCVAR_NOTIFY | FCVAR_REPLICATED, "Velocity required for game to send event on self knockback");

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

#define TF_SHIELD_CHARGE_SPEED 720

#define TF_MAX_SPEED   720

#define TF_WATERJUMP_FORWARD  30
#define TF_WATERJUMP_UP       300
//ConVar	tf_waterjump_up( "tf_waterjump_up", "300", FCVAR_REPLICATED | FCVAR_CHEAT );
//ConVar	tf_waterjump_forward( "tf_waterjump_forward", "30", FCVAR_REPLICATED | FCVAR_CHEAT );

#define TF_MAX_AIR_DUCKS 2

class CTFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CTFGameMovement, CGameMovement );

	CTFGameMovement(); 

	virtual void PlayerMove();
	virtual void ShieldChargeMove();
	virtual unsigned int PlayerSolidMask( bool brushOnly = false );
	virtual void ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove );
	virtual bool CanAccelerate();
	virtual bool CheckJumpButton();
	virtual bool CheckWater( void );
	virtual void WaterMove( void );
	virtual void FullWalkMove();
	virtual void WalkMove( void );
	virtual void AirMove( void );
	virtual void FullTossMove( void );
	bool HighMaxSpeedMove( void );
	bool StunMove( void );
	void TauntMove( void );
	virtual void CategorizePosition( void );
	virtual void CheckFalling( void );
	virtual void Duck( void );
	virtual void HandleDuckingSpeedCrop();

	virtual float GetAirSpeedCap( void );
	virtual Vector GetPlayerViewOffset( bool ducked ) const;

	virtual void	TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );
	virtual CBaseHandle	TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );
	virtual void	StepMove( Vector &vecDestination, trace_t &trace );
	virtual bool	GameHasLadders() const;
	virtual void SetGroundEntity( trace_t *pm );
	virtual void PlayerRoughLandingEffects( float fvol );
protected:

	virtual void CheckWaterJump(void );
	void		 FullWalkMoveUnderwater();

private:

	bool		CheckWaterJumpButton( void );
	void		AirDash( void );
	void		PreventBunnyJumping();

private:

	Vector		m_vecWaterPoint;
	CTFPlayer  *m_pTFPlayer;
};


// Expose our interface.
static CTFGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );


// ---------------------------------------------------------------------------------------- //
// CTFGameMovement.
// ---------------------------------------------------------------------------------------- //

CTFGameMovement::CTFGameMovement()
{
	m_pTFPlayer = NULL;
}

//---------------------------------------------------------------------------------------- 
// Purpose: moves the player
//----------------------------------------------------------------------------------------
void CTFGameMovement::PlayerMove()
{
#if TF_PLAYER_SCRIPED_SEQUENCES
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
#ifdef GAME_DLL
		Vector vecNewPos;
		QAngle vecNewAngles;
		bool bFinished;

		if ( m_pTFPlayer->GetIntervalMovement( gpGlobals->frametime, bFinished, vecNewPos, vecNewAngles ) )
		{
			mv->SetAbsOrigin( vecNewPos );
			mv->m_vecAngles = vecNewAngles;
			m_pTFPlayer->m_flTauntYaw = vecNewAngles[YAW];
		}
#endif

		mv->m_bGameCodeMovedPlayer = true;
	}
#endif

	// call base class to do movement
	BaseClass::PlayerMove();

	// handle player's interaction with water
	int nNewWaterLevel = m_pTFPlayer->GetWaterLevel();
	if ( m_nOldWaterLevel != nNewWaterLevel )
	{
		if ( WL_NotInWater == m_nOldWaterLevel )
		{
			// The player has just entered the water.  Determine if we should play a splash sound.
			bool bPlaySplash = false;
					
			Vector vecVelocity = m_pTFPlayer->GetAbsVelocity();
			if ( vecVelocity.z <= -200.0f )
			{
				// If the player has significant downward velocity, play a splash regardless of water depth.  (e.g. Jumping hard into a puddle)
				bPlaySplash = true;
			}
			else
			{
				// Look at the water depth below the player.  If it's significantly deep, play a splash to accompany the sinking that's about to happen.
				Vector vecStart = m_pTFPlayer->GetAbsOrigin();
				Vector vecEnd = vecStart;
				vecEnd.z -= 20;	// roughly thigh deep
				trace_t tr;
				// see if we hit anything solid a little bit below the player
				UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID,m_pTFPlayer, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction >= 1.0f ) 
				{
					// some amount of water below the player, play a splash
					bPlaySplash = true;
				}
			}

			if ( bPlaySplash )
			{
				CPASAttenuationFilter filter( m_pTFPlayer, "Physics.WaterSplash" );
				filter.UsePredictionRules();
				CBaseEntity::EmitSound( filter, m_pTFPlayer->entindex(), "Physics.WaterSplash" );
			}
		}

		if ( m_nOldWaterLevel == WL_NotInWater && nNewWaterLevel > WL_NotInWater )
		{
			// Set when we do a transition to/from partially in water to completely out
			m_pTFPlayer->m_flWaterEntryTime = gpGlobals->curtime;
		}

		// If player is now up to his eyes in water and has entered the water very recently (not just bobbing eyes in and out), play a bubble effect.
		if ( nNewWaterLevel == WL_Eyes && gpGlobals->curtime - m_pTFPlayer->m_flWaterEntryTime < 0.5f )
		{
			CEffectData data;
			data.m_vStart = m_pTFPlayer->WorldSpaceCenter();
#ifdef CLIENT_DLL
			data.m_hEntity = m_pTFPlayer;
#else
			data.m_nEntIndex = m_pTFPlayer->entindex();
#endif
			CPVSFilter filter( m_pTFPlayer->GetAbsOrigin() );
			te->DispatchEffect( filter, 0.0f, m_pTFPlayer->GetAbsOrigin(), "WaterPlayerDive", data );
		}
		// If player was up to his eyes in water and is now out to waist level or less, play a water drip effect
		else if ( m_nOldWaterLevel == WL_Eyes && nNewWaterLevel < WL_Eyes )
		{
			CEffectData data;
#ifdef CLIENT_DLL
			data.m_hEntity = m_pTFPlayer;
#else
			data.m_nEntIndex = m_pTFPlayer->entindex();
#endif

			CPVSFilter filter( m_pTFPlayer->GetAbsOrigin() );
			te->DispatchEffect( filter, 0.0f, m_pTFPlayer->GetAbsOrigin(), "WaterPlayerEmerge", data );
		}
	}

	// Check if we're in front of a wall if we're super slow for some reason
	if( m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) && mv->m_vecVelocity.Length() < 100 )
		m_pTFPlayer->m_Shared.m_flChargeTooSlowTime += gpGlobals->frametime;
	else
		m_pTFPlayer->m_Shared.m_flChargeTooSlowTime = 0;

	if( m_pTFPlayer->m_Shared.m_flChargeTooSlowTime >= 0.15f )
	{
		float flRange = 48.0f;

		CTFWeaponBaseMelee *pProvider = dynamic_cast<CTFWeaponBaseMelee*>(m_pTFPlayer->m_Shared.GetConditionProvider(TF_COND_SHIELD_CHARGE));
		if( pProvider )
			flRange = pProvider->GetSwingRange();

		Vector vecForward; 
		AngleVectors( m_pTFPlayer->GetAbsAngles(), &vecForward );
		Vector vecEnd = m_pTFPlayer->GetAbsOrigin() + vecForward * flRange;
		// Try moving to the destination.
		trace_t trace;
		TracePlayerBBox( m_pTFPlayer->GetAbsOrigin(), vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		if( trace.fraction < 0.1f )
			m_pTFPlayer->m_Shared.StopCharge( true );
	}
}


void CTFGameMovement::ShieldChargeMove( void )
{
	mv->m_flForwardMove = TF_SHIELD_CHARGE_SPEED;
	mv->m_flMaxSpeed = TF_SHIELD_CHARGE_SPEED;
	mv->m_flSideMove = 0.0f;
	mv->m_flUpMove = 0.0f;

	// HACK HACK: This prevents Jump and crouch inputs, try to do it via functions later instead - Kay
	mv->m_nButtons &= ~IN_DUCK;
	mv->m_nButtons &= ~IN_JUMP;
}

Vector CTFGameMovement::GetPlayerViewOffset( bool ducked ) const
{
	return ducked ? VEC_DUCK_VIEW_SCALED( m_pTFPlayer ) : ( m_pTFPlayer->GetClassEyeHeight() );
}

float CTFGameMovement::GetAirSpeedCap( void )
{
	float flBaseAirSpeed = BaseClass::GetAirSpeedCap();

	if( m_pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) && flBaseAirSpeed < TF_SHIELD_CHARGE_SPEED )
		return TF_SHIELD_CHARGE_SPEED;

	return flBaseAirSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CTFGameMovement::PlayerSolidMask( bool brushOnly )
{
	unsigned int uMask = 0;

	if ( m_pTFPlayer )
	{
		switch ( m_pTFPlayer->GetTeamNumber() )
		{
			case TF_TEAM_RED:
				uMask = CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
				break;

			case TF_TEAM_BLUE:
				uMask = CONTENTS_REDTEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
				break;

			case TF_TEAM_GREEN:
				uMask = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_YELLOWTEAM;
				break;

			case TF_TEAM_YELLOW:
				uMask = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM;
				break;
		}
	}

	return ( uMask | BaseClass::PlayerSolidMask( brushOnly ) );
}

//-----------------------------------------------------------------------------
// Purpose: Overridden to allow players to run faster than the maxspeed
//-----------------------------------------------------------------------------
void CTFGameMovement::ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove )
{
	// Verify data.
	Assert( pBasePlayer );
	Assert( pMove );
	if ( !pBasePlayer || !pMove )
		return;

	// Reset point contents for water check.
	ResetGetPointContentsCache();

	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	// flag globally here once per usercmd cycle.
	m_iSpeedCropped = SPEED_CROPPED_RESET;

	// Get the current TF player.
	m_pTFPlayer = ToTFPlayer( pBasePlayer );
	player = m_pTFPlayer;
	mv = pMove;

	// The max speed is currently set to the scout - if this changes we need to change this!
	mv->m_flMaxSpeed = TF_MAX_SPEED; /*tf_maxspeed.GetFloat();*/

	// Run the command.
	HighMaxSpeedMove();

	// Handle player stun.
	StunMove();

	// Handle player taunt move
	TauntMove();

	if( m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) )
		ShieldChargeMove();

	// Run the command.
	PlayerMove();

	// Finish the command.
	FinishMove();


#ifdef GAME_DLL
	m_pTFPlayer->m_bBlastLaunched = false;
	m_pTFPlayer->m_bSelfKnockback = false;
#endif
}


bool CTFGameMovement::CanAccelerate()
{
	// Only allow the player to accelerate when in certain states.
	int nCurrentState = m_pTFPlayer->m_Shared.GetState();
	if ( nCurrentState == TF_STATE_ACTIVE )
	{
		return player->GetWaterJumpTime() == 0;
	}
	else if ( player->IsObserver() )
	{
		return true;
	}
	else
	{	
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we are in water.  If so the jump button acts like a
// swim upward key.
//-----------------------------------------------------------------------------
bool CTFGameMovement::CheckWaterJumpButton( void )
{
	// See if we are water jumping.  If so, decrement count and return.
	if ( player->m_flWaterJumpTime )
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
		{
			player->m_flWaterJumpTime = 0;
		}

		return false;
	}

	// In water above our waist.
	if ( player->GetWaterLevel() >= 2 )
	{	
		// Swimming, not jumping.
		SetGroundEntity( NULL );

		// We move up a certain amount.
		if ( player->GetWaterType() == CONTENTS_WATER )
		{
			mv->m_vecVelocity[2] = 100;
		}
		else if ( player->GetWaterType() == CONTENTS_SLIME )
		{
			mv->m_vecVelocity[2] = 80;
		}

		// Play swiming sound.
		if ( player->m_flSwimSoundTime <= 0 )
		{
			// Don't play sound again for 1 second.
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	return true;
}

void CTFGameMovement::AirDash( void )
{
	// Apply approx. the jump velocity added to an air dash.
	Assert( sv_gravity.GetFloat() == 800.0f );

	// Get the wish direction.
	Vector vecForward, vecRight;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, NULL );
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;		
	VectorNormalize( vecForward );
	VectorNormalize( vecRight );

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;

	// Jump height attributes
	float flJumpHeightMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flJumpHeightMult, mod_jump_height );
	CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
	if ( pWpn )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flJumpHeightMult, mod_jump_height_from_weapon );
	}

	float flAirDashZ = 268.3281572999747f * flJumpHeightMult;

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection( ( ( vecForward.x * flForwardMove ) + ( vecRight.x * flSideMove ) ),
		                     ( ( vecForward.y * flForwardMove ) + ( vecRight.y * flSideMove ) ), 
							 flAirDashZ );
	
	// Update the velocity on the scout.
	mv->m_vecVelocity = vecWishDirection;

	m_pTFPlayer->m_Shared.SetAirDash( true );

	// Play the gesture.
	m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_DOUBLEJUMP );

	// Create the double jump effect puff.
	DispatchParticleEffect( "doublejump_puff", PATTACH_POINT_FOLLOW, m_pTFPlayer, "doublejumpfx" );
}

// Only allow bunny jumping up to 1.2x server / player maxspeed setting
//#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

void CTFGameMovement::PreventBunnyJumping()
{
	if ( tf2c_bunnyjump_max_speed_factor.GetFloat() <= 0.0f )
		return;

	// Speed at which bunny jumping is limited
	float maxscaledspeed = m_pTFPlayer->m_flMaxspeed * tf2c_bunnyjump_max_speed_factor.GetFloat();

	// Current player speed
	float spd = mv->m_vecVelocity.Length();
	if ( spd <= maxscaledspeed )
		return;

	// Apply this cropping fraction to velocity
	float fraction = ( maxscaledspeed / spd );


	mv->m_vecVelocity *= fraction;
}

bool CTFGameMovement::CheckJumpButton()
{
	// Are we dead?  Then we cannot jump.
	if ( player->pl.deadflag )
		return false;

	// Check to see if we are in water.
	if ( !CheckWaterJumpButton() )
		return false;

	// Can't jump if our weapon disallows it.
	CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
	if ( pWpn && !pWpn->OwnerCanJump() )
		return false;

	// Can't jump while loserstatestunned
	if ( m_pTFPlayer->m_Shared.IsLoserStateStunned() )
		return false;

	if ( !m_pTFPlayer->CanJump() )
		return false;

	// Check to see if the player is a scout.
	bool bScout = m_pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT, true );
	bool bAirDash = false;
	bool bOnGround = ( player->GetGroundEntity() != NULL );

	bool bIsDuckJumpingSpy = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pTFPlayer, bIsDuckJumpingSpy, mod_knife_leap);
	if ( bIsDuckJumpingSpy && !m_pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
		bIsDuckJumpingSpy = false;

	// !!! foxysen grav
	int iGrav = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pTFPlayer, iGrav, anti_gravity_boots);
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pTFPlayer->GetActiveTFWeapon(), iGrav, anti_gravity_boots_active);

	int iNoDoubleJump = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pTFPlayer, iNoDoubleJump, set_scout_doublejump_disabled);
	CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pTFPlayer->GetActiveTFWeapon(), iNoDoubleJump, set_scout_doublejump_disabled_active);

	if ( tf2c_airdash_disable.GetBool() )
		iNoDoubleJump = 1;

	// Cannot jump while ducked.
	if ( player->GetFlags() & FL_DUCKING )
	{
		// Let a scout do it.
		bool bAllow = ((bScout || iGrav) && !iNoDoubleJump && !bOnGround) || tf2c_duckjump.GetBool() || bIsDuckJumpingSpy;	// !!! foxysen grav

		if ( !bAllow )
			return false;
	}

	if ( !tf2c_duckjump.GetBool() )
	{
		// Cannot jump will in the unduck transition.
		if ( player->m_Local.m_bDucking && ( player->GetFlags() & FL_DUCKING ) )
			return false;

		// Still updating the eye position.
		if ( player->m_Local.m_flDuckJumpTime > 0.0f )
			return false;
	}

	// Cannot jump again until the jump button has been released unless auto jump is enabled.
	if ( mv->m_nOldButtons & IN_JUMP )
	{
		if ( !bOnGround )
			return false;

		if ( !tf2c_autojump.GetBool() )
			return false;
	}

	// In air, so ignore jumps (unless you are a scout).
	if ( !bOnGround )
	{
		if ( bScout && !m_pTFPlayer->m_Shared.IsAirDashing() && !iNoDoubleJump )
		{
			bAirDash = true;
		}

		else if (iGrav && !m_pTFPlayer->m_Shared.IsAirDashing()) // !!! foxysen grav
		{
			Vector GrabNewVel = mv->m_vecVelocity;
			Vector2D GrabNewVelHorizontal;
			GrabNewVelHorizontal.x = GrabNewVel.x;
			GrabNewVelHorizontal.y = GrabNewVel.y;
			float flHorizontalVelLength = GrabNewVelHorizontal.Length();
			if (flHorizontalVelLength != 0)
			{
				float flClampedHorizontalVelLength = clamp(flHorizontalVelLength, -tf2c_grav_boots_max_horizontal_velocity_clamp.GetFloat(), tf2c_grav_boots_max_horizontal_velocity_clamp.GetFloat());
				GrabNewVelHorizontal *= (flClampedHorizontalVelLength / flHorizontalVelLength);
				GrabNewVel.x = GrabNewVelHorizontal.x;
				GrabNewVel.y = GrabNewVelHorizontal.y;
			}
			if (GrabNewVel.z > tf2c_grav_boots_deadzone_negative_range.GetFloat() && GrabNewVel.z < tf2c_grav_boots_deadzone_positive_range.GetFloat())
			{
				GrabNewVel.z = tf2c_grav_boots_deadzone_velocity.GetFloat();
			}
			else
			{
				GrabNewVel.z *= -1;
			}
			mv->m_vecVelocity = GrabNewVel;
#ifdef GAME_DLL
			m_pTFPlayer->EmitSound("TFPlayer.SafestLanding");
			//m_pTFPlayer->SetBlastJumpState(TF_PLAYER_ROCKET_JUMPED); // doing crits with mace by just running toward someone very funny
#endif
			m_pTFPlayer->m_Shared.SetAirDash(true);
			m_pTFPlayer->m_Shared.AddCond(TF_COND_LAUNCHED, tf2c_grav_boots_jumppad_cond_duration.GetFloat());

			return true;
		}

		else
		{
			mv->m_nOldButtons |= IN_JUMP;
			return false;
		}
	}

	// Check for an air dash.
	if ( bAirDash )
	{
		AirDash();
		return true;
	}

	PreventBunnyJumping();

	// Start jump animation and player sound (specific TF animation and flags).
	m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
	player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );
	m_pTFPlayer->m_Shared.SetJumping( true );

	// Set the player as in the air.
	SetGroundEntity( NULL );

	// Check the surface the player is standing on to see if it impacts jumping.
	float flGroundFactor = 1.0f;
	if ( player->m_pSurfaceData )
	{
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor; 
	}

	// Jump height attributes
	float flJumpHeightMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pTFPlayer, flJumpHeightMult, mod_jump_height );
	if ( pWpn )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flJumpHeightMult, mod_jump_height_from_weapon );
	}

	// fMul = sqrt( 2.0 * gravity * jump_height (21.0units) ) * GroundFactor
	Assert( sv_gravity.GetFloat() == 800.0f );
	float flMul = 289.0f * flGroundFactor * flJumpHeightMult;

	// Save the current z velocity.
	float flStartZ = mv->m_vecVelocity[2];

	// Acclerate upward
	if ( ( player->m_Local.m_bDucking ) || ( player->GetFlags() & FL_DUCKING ) )
	{
		// If we are ducking...
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		mv->m_vecVelocity[2] = flMul;  // 2 * gravity * jump_height * ground_factor
	}
	else
	{
		mv->m_vecVelocity[2] += flMul;  // 2 * gravity * jump_height * ground_factor
	}

	// Apply gravity.
	FinishGravity();

	// Save the output data for the physics system to react to if need be.
	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - flStartZ;
	mv->m_outStepHeight += 0.15f;

	// Flag that we jumped and don't jump again until it is released.
	mv->m_nOldButtons |= IN_JUMP;
	return true;
}

bool CTFGameMovement::CheckWater( void )
{
	Vector vecPlayerMin = GetPlayerMins();
	Vector vecPlayerMax = GetPlayerMaxs();

	Vector vecPoint( ( mv->GetAbsOrigin().x + ( vecPlayerMin.x + vecPlayerMax.x ) * 0.5f ),
				     ( mv->GetAbsOrigin().y + ( vecPlayerMin.y + vecPlayerMax.y ) * 0.5f ),
				     ( mv->GetAbsOrigin().z + vecPlayerMin.z + 1 ) );


	// Assume that we are not in water at all.
	int wl = WL_NotInWater;
	int wt = CONTENTS_EMPTY;

	// Check to see if our feet are underwater.
	int nContents = GetPointContentsCached( vecPoint, 0 );	
	if ( nContents & MASK_WATER )
	{
		// Clear our jump flag, because we have landed in water.
		m_pTFPlayer->m_Shared.SetJumping( false );

		// Set water type and level.
		wt = nContents;
		wl = WL_Feet;

		float flWaistZ = mv->GetAbsOrigin().z + ( vecPlayerMin.z + vecPlayerMax.z ) * 0.5f + 12.0f;

		// Now check eyes
		vecPoint.z = mv->GetAbsOrigin().z + player->GetViewOffset()[2];
		nContents = GetPointContentsCached( vecPoint, 1 );
		if ( nContents & MASK_WATER )
		{
			// In over our eyes
			wl = WL_Eyes;  
			VectorCopy( vecPoint, m_vecWaterPoint );
			m_vecWaterPoint.z = flWaistZ;
		}
		else
		{
			// Now check a point that is at the player hull midpoint (waist) and see if that is underwater.
			vecPoint.z = flWaistZ;
			nContents = GetPointContentsCached( vecPoint, 2 );
			if ( nContents & MASK_WATER )
			{
				// Set the water level at our waist.
				wl = WL_Waist;
				VectorCopy( vecPoint, m_vecWaterPoint );
			}
		}
	}

	player->SetWaterLevel( wl );
	player->SetWaterType( wt );

	// If we just transitioned from not in water to water, record the time for splashes, etc.
	if ( ( WL_NotInWater == m_nOldWaterLevel ) && ( wl >  WL_NotInWater ) )
	{
		m_flWaterEntryTime = gpGlobals->curtime;
	}
#ifdef GAME_DLL
	// If we made the opposite transition, then also record the time, but in the player itself
	// (for e.g. attribute "crit_vs_wet_players")
	else if ( m_nOldWaterLevel > WL_NotInWater && wl == WL_NotInWater )
	{
		m_pTFPlayer->SetWaterExitTime( gpGlobals->curtime );
	}
#endif

	return ( wl > WL_Feet );
}




void CTFGameMovement::WaterMove( void )
{
	int i;
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	trace_t	pm;
	float speed, newspeed, addspeed, accelspeed;

	// Determine movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp );

	// Calculate the desired direction and speed.
	Vector vecWishVelocity;
	int iAxis;
	for ( iAxis = 0 ; iAxis < 3; ++iAxis )
	{
		vecWishVelocity[iAxis] = ( vecForward[iAxis] * mv->m_flForwardMove ) + ( vecRight[iAxis] * mv->m_flSideMove );
	}

	// Check for upward velocity (JUMP).
	if ( mv->m_nButtons & IN_JUMP )
	{
		if ( player->GetWaterLevel() == WL_Eyes )
		{
			vecWishVelocity[2] += mv->m_flClientMaxSpeed;
		}
	}
	// Sinking if not moving.
	else if ( !mv->m_flForwardMove && !mv->m_flSideMove && !mv->m_flUpMove )
	{
		vecWishVelocity[2] -= 60;
	}
	// Move up based on view angle.
	else
	{
		vecWishVelocity[2] += mv->m_flUpMove;
	}

	// Copy it over and determine speed
	VectorCopy( vecWishVelocity, wishdir );
	wishspeed = VectorNormalize( wishdir );

	// Cap speed.
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale( vecWishVelocity, mv->m_flMaxSpeed/wishspeed, vecWishVelocity );
		wishspeed = mv->m_flMaxSpeed;
	}

	// Slow us down a bit.
	wishspeed *= 0.8;
	
	// Water friction
	VectorCopy( mv->m_vecVelocity, temp );
	speed = VectorNormalize( temp );
	if ( speed )
	{
		newspeed = speed - gpGlobals->frametime * speed * sv_friction.GetFloat() * player->m_surfaceFriction;
		if ( newspeed < 0.1f )
		{
			newspeed = 0;
		}

		VectorScale (mv->m_vecVelocity, newspeed/speed, mv->m_vecVelocity);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if (wishspeed >= 0.1f)  // old !
	{
		addspeed = wishspeed - newspeed;
		if (addspeed > 0)
		{
			VectorNormalize(vecWishVelocity);
			accelspeed = sv_accelerate.GetFloat() * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}

			for (i = 0; i < 3; i++)
			{
				float deltaSpeed = accelspeed * vecWishVelocity[i];
				mv->m_vecVelocity[i] += deltaSpeed;
				mv->m_outWishVel[i] += deltaSpeed;
			}
		}
	}

	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, dest);
	
	TracePlayerBBox( mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	if ( pm.fraction == 1.0f )
	{
		VectorCopy( dest, start );
		if ( player->m_Local.m_bAllowAutoMovement )
		{
			start[2] += player->m_Local.m_flStepSize + 1;
		}
		
		TracePlayerBBox( start, dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		if (!pm.startsolid && !pm.allsolid)
		{	
#if 0
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit

			Vector vecNewWaterPoint;
			VectorCopy( m_vecWaterPoint, vecNewWaterPoint );
			vecNewWaterPoint.z += ( dest.z - mv->GetAbsOrigin().z );
			bool bOutOfWater = !( enginetrace->GetPointContents( vecNewWaterPoint ) & MASK_WATER );
			if ( bOutOfWater && ( mv->m_vecVelocity.z > 0.0f ) && ( pm.fraction == 1.0f )  )
			{
				// Check the waist level water positions.
				trace_t traceWater;
				UTIL_TraceLine( vecNewWaterPoint, m_vecWaterPoint, CONTENTS_WATER, player, COLLISION_GROUP_NONE, &traceWater );
				if( traceWater.fraction < 1.0f )
				{
					float flFraction = 1.0f - traceWater.fraction;

//					Vector vecSegment;
//					VectorSubtract( mv->GetAbsOrigin(), dest, vecSegment );
//					VectorMA( mv->GetAbsOrigin(), flFraction, vecSegment, mv->GetAbsOrigin() );
					float flZDiff = dest.z - mv->GetAbsOrigin().z;
					float flSetZ = mv->GetAbsOrigin().z + ( flFraction * flZDiff );
					flSetZ -= 0.0325f;

					VectorCopy (pm.endpos, mv->GetAbsOrigin());
					mv->GetAbsOrigin().z = flSetZ;
					VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
					mv->m_vecVelocity.z = 0.0f;
				}

			}
			else
			{
				VectorCopy (pm.endpos, mv->GetAbsOrigin());
				VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			}

			return;
#endif
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit
			mv->SetAbsOrigin( pm.endpos );
			VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			return;
		}

		// Try moving straight along out normal path.
		TryPlayerMove();
	}
	else
	{
		if ( !player->GetGroundEntity() )
		{
			TryPlayerMove();
			VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
			return;
		}

		StepMove( dest, pm );
	}
	
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}


void CTFGameMovement::WalkMove( void )
{
	// Get the movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp );
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;		
	VectorNormalize( vecForward );
	VectorNormalize( vecRight );

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;
	
	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection( ( ( vecForward.x * flForwardMove ) + ( vecRight.x * flSideMove ) ),
		                     ( ( vecForward.y * flForwardMove ) + ( vecRight.y * flSideMove ) ), 
							 0.0f );

	// If we're airblasted, prevent moving towards airblast position.
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIRBLASTED ) )
	{
		Vector vecBlastDir = mv->GetAbsOrigin() - m_pTFPlayer->m_Shared.GetAirblastPosition();
		vecBlastDir.z = 0.0f;
		VectorNormalize( vecBlastDir );

		float flDot = DotProduct( vecWishDirection, vecBlastDir );
		if ( flDot < 0.0f )
		{
			vecWishDirection -= vecBlastDir * flDot;
		}
	}

	// Calculate the speed and direction of movement, then clamp the speed.
	float flWishSpeed = VectorNormalize( vecWishDirection );
	flWishSpeed = clamp( flWishSpeed, 0.0f, mv->m_flMaxSpeed );

	// Accelerate in the x,y plane.
	mv->m_vecVelocity.z = 0;
	Accelerate( vecWishDirection, flWishSpeed, sv_accelerate.GetFloat() );
	Assert( mv->m_vecVelocity.z == 0.0f );

	// Clamp the players speed in x,y.
	bool bGroundSpeedCap = tf2c_groundspeed_cap.GetBool();
	float flNewSpeed, flScale;
	if ( bGroundSpeedCap )
	{
		flNewSpeed = VectorLength( mv->m_vecVelocity );
		if ( flNewSpeed > mv->m_flMaxSpeed )
		{
			flScale = ( mv->m_flMaxSpeed / flNewSpeed );
			mv->m_vecVelocity.x *= flScale;
			mv->m_vecVelocity.y *= flScale;
		}
	}

	// Now reduce their backwards speed to some percent of max, if they are travelling backwards
	// unless they are under some minimum, to not penalize deployed snipers or heavies
	if ( tf_clamp_back_speed.GetFloat() < 1.0f && VectorLength( mv->m_vecVelocity ) > tf_clamp_back_speed_min.GetFloat() )
	{
		// Are we moving backwards at all?
		float flDot = DotProduct( vecForward, mv->m_vecVelocity );
		if ( flDot < 0 )
		{
			Vector vecBackMove = vecForward * flDot;
			Vector vecRightMove = vecRight * DotProduct( vecRight, mv->m_vecVelocity );

			// Clamp the back move vector if it is faster than max.
			float flBackSpeed = VectorLength( vecBackMove );
			float flMaxBackSpeed = ( mv->m_flMaxSpeed * tf_clamp_back_speed.GetFloat() );

			if ( flBackSpeed > flMaxBackSpeed )
			{
				vecBackMove *= flMaxBackSpeed / flBackSpeed;
			}
			
			// Reassemble velocity.
			mv->m_vecVelocity = vecBackMove + vecRightMove;

			// Re-run this to prevent crazy values (clients can induce this via usercmd viewangles hacking).
			if ( bGroundSpeedCap )
			{
				flNewSpeed = VectorLength( mv->m_vecVelocity );
				if ( flNewSpeed > mv->m_flMaxSpeed )
				{
					flScale = ( mv->m_flMaxSpeed / flNewSpeed );
					mv->m_vecVelocity.x *= flScale;
					mv->m_vecVelocity.y *= flScale;
				}
			}
		}
	}

	// Add base velocity to the player's current velocity - base velocity = velocity from conveyors, etc.
	VectorAdd( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	// Calculate the current speed and return if we are not really moving.
	float flSpeed = VectorLength( mv->m_vecVelocity );
	if ( flSpeed < 1.0f )
	{
		// I didn't remove the base velocity here since it wasn't moving us in the first place.
		mv->m_vecVelocity.Init();
		return;
	}

	// Calculate the destination.
	Vector vecDestination;
	vecDestination.x = mv->GetAbsOrigin().x + ( mv->m_vecVelocity.x * gpGlobals->frametime );
	vecDestination.y = mv->GetAbsOrigin().y + ( mv->m_vecVelocity.y * gpGlobals->frametime );	
	vecDestination.z = mv->GetAbsOrigin().z;

	// Try moving to the destination.
	trace_t trace;
	TracePlayerBBox( mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( trace.fraction == 1.0f )
	{
		// Made it to the destination (remove the base velocity).
		mv->SetAbsOrigin( trace.endpos );
		VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

		// Save the wish velocity.
		mv->m_outWishVel += ( vecWishDirection * flWishSpeed );

		// Try and keep the player on the ground.
		// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
		// StayOnGround();

		return;
	}

	if( m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) && m_pTFPlayer->m_Shared.CheckShieldBash() )
		m_pTFPlayer->m_Shared.StopCharge( true );

	// Now try and do a step move.
	StepMove( vecDestination, trace );

	// Remove base velocity.
	Vector baseVelocity = player->GetBaseVelocity();
	VectorSubtract( mv->m_vecVelocity, baseVelocity, mv->m_vecVelocity );

	// Save the wish velocity.
	mv->m_outWishVel += ( vecWishDirection * flWishSpeed );

	// Try and keep the player on the ground.
	// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
	// StayOnGround();

#if 0
	// Debugging!!!
	Vector vecTestVelocity = mv->m_vecVelocity;
	vecTestVelocity.z = 0.0f;
	float flTestSpeed = VectorLength( vecTestVelocity );
	if ( baseVelocity.IsZero() && ( flTestSpeed > ( mv->m_flMaxSpeed + 1.0f ) ) )
	{
		Msg( "Step Max Speed < %f\n", flTestSpeed );
	}

	if ( tf_showspeed.GetBool() )
	{
		Msg( "Speed=%f\n", flTestSpeed );
	}

#endif
}


void CTFGameMovement::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors ( mv->m_vecViewAngles, &forward, &right, &up );  // Determine movement angles

	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize( forward );  // Normalize remainder of vectors
	VectorNormalize( right );    // 

	for ( i = 0; i < 2; i++ )       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	// If we're airblasted, prevent moving towards airblast position.
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIRBLASTED ) )
	{
		Vector vecBlastDir = mv->GetAbsOrigin() - m_pTFPlayer->m_Shared.GetAirblastPosition();
		vecBlastDir.z = 0.0f;
		VectorNormalize( vecBlastDir );

		float flDot = DotProduct( wishvel, vecBlastDir );
		if ( flDot < 0.0f )
		{
			wishvel -= vecBlastDir * flDot;
		}
	}

	VectorCopy( wishvel, wishdir );   // Determine maginitude of speed of move
	wishspeed = VectorNormalize( wishdir );

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && ( wishspeed > mv->m_flMaxSpeed ) )
	{
		VectorScale ( wishvel, mv->m_flMaxSpeed/wishspeed, wishvel );
		wishspeed = mv->m_flMaxSpeed;
	}

	AirAccelerate( wishdir, wishspeed, sv_airaccelerate.GetFloat() );


	// Add in any base velocity to the current velocity.
	VectorAdd( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}

extern void TracePlayerBBoxForGround( const Vector& start, const Vector& end, const Vector& minsSrc,
							  const Vector& maxsSrc, IHandleEntity *player, unsigned int fMask,
							  int collisionGroup, trace_t& pm );

CBaseHandle CTFGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	if( tf_solidobjects.GetBool() == false )
		return BaseClass::TestPlayerPosition( pos, collisionGroup, pm );

	Ray_t ray;
	ray.Init( pos, pos, GetPlayerMins(), GetPlayerMaxs() );
	
	CTraceFilterObject traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );
	enginetrace->TraceRay( ray, PlayerSolidMask(), &traceFilter, &pm );

	if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt )
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE_INDEX;
	}
}

//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
void CTFGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	if( tf_solidobjects.GetBool() == false )
		return BaseClass::TracePlayerBBox( start, end, fMask, collisionGroup, pm );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );
	
	CTraceFilterObject traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );

	enginetrace->TraceRay( ray, fMask, &traceFilter, &pm );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CTFGameMovement::CategorizePosition( void )
{
	// Observer.
	if ( player->IsObserver() )
		return;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	CheckWater();

	// If standing on a ladder we are not on ground.
	if ( player->GetMoveType() == MOVETYPE_LADDER )
	{
		SetGroundEntity( NULL );
		return;
	}



#ifdef GAME_DLL
	if (m_pTFPlayer->m_bSelfKnockback && player->GetGroundEntity() == NULL)
	{
		if (mv->m_vecVelocity.LengthSqr() >= Square(tf2c_self_knockback_min_velocity_for_event.GetFloat()))
		{
			IGameEvent* event = gameeventmanager->CreateEvent("weapon_knockback_jump");

			if (event)
			{
				event->SetInt("userid", m_pTFPlayer->GetUserID());

				gameeventmanager->FireEvent(event);
			}
		}
	}
		
#endif

	// Check for a jump.
	if ( mv->m_vecVelocity.z > 250.0f )
	{
		SetGroundEntity( NULL );

#ifdef GAME_DLL
		if ( m_pTFPlayer->m_bBlastLaunched )
			m_pTFPlayer->SetBlastJumpState( TF_PLAYER_ENEMY_BLASTED_ME, false );
#endif

		return;
	}

	// Calculate the start and end position.
	Vector vecStartPos = mv->GetAbsOrigin();
	Vector vecEndPos( mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, ( mv->GetAbsOrigin().z - 2.0f ) );

	// NOTE YWB 7/5/07:  Since we're already doing a traceline here, we'll subsume the StayOnGround (stair debouncing) check into the main traceline we do here to see what we're standing on
	bool bUnderwater = ( player->GetWaterLevel() >= WL_Eyes );
	bool bMoveToEndPos = false;
	if ( player->GetMoveType() == MOVETYPE_WALK && 
		player->GetGroundEntity() != NULL && !bUnderwater )
	{
		// if walking and still think we're on ground, we'll extend trace down by stepsize so we don't bounce down slopes
		vecEndPos.z -= player->GetStepSize();
		bMoveToEndPos = true;
	}

	trace_t trace;
	TracePlayerBBox( vecStartPos, vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );

	// Steep plane, not on ground.
	if ( trace.plane.normal.z < 0.7f )
	{
		// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on.
		TracePlayerBBoxForGround( vecStartPos, vecEndPos, GetPlayerMins(), GetPlayerMaxs(), mv->m_nPlayerHandle.Get(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		if ( trace.plane.normal[2] < 0.7f )
		{
			// Too steep.
			SetGroundEntity( NULL );
			if ( ( mv->m_vecVelocity.z > 0.0f ) && 
				( player->GetMoveType() != MOVETYPE_NOCLIP ) )
			{
				player->m_surfaceFriction = 0.25f;
			}
		}
		else
		{
			SetGroundEntity( &trace );
		}
	}
	else
	{
		// YWB:  This logic block essentially lifted from StayOnGround implementation
		if ( bMoveToEndPos &&
			!trace.startsolid &&				// not sure we need this check as fraction would == 0.0f?
			trace.fraction > 0.0f &&			// must go somewhere
			trace.fraction < 1.0f ) 			// must hit something
		{
			float flDelta = fabs( mv->GetAbsOrigin().z - trace.endpos.z );
			// HACK HACK:  The real problem is that trace returning that strange value 
			//  we can't network over based on bit precision of networking origins
			if ( flDelta > 0.5f * COORD_RESOLUTION )
			{
				Vector org = mv->GetAbsOrigin();
				org.z = trace.endpos.z;
				mv->SetAbsOrigin( org );
			}
		}
		SetGroundEntity( &trace );
	}
}


void CTFGameMovement::CheckWaterJump( void )
{
	Vector	flatforward;
	Vector	flatvelocity;
	float curspeed;

	// Jump button down?
	bool bJump = ( ( mv->m_nButtons & IN_JUMP ) != 0 );

	Vector forward, right;
	AngleVectors( mv->m_vecViewAngles, &forward, &right, NULL );  // Determine movement angles

	// Already water jumping.
	if (player->m_flWaterJumpTime)
		return;

	// Don't hop out if we just jumped in
	if (mv->m_vecVelocity[2] < -180)
		return; // only hop out if we are moving up

	// See if we are backing up
	flatvelocity[0] = mv->m_vecVelocity[0];
	flatvelocity[1] = mv->m_vecVelocity[1];
	flatvelocity[2] = 0;

	// Must be moving
	curspeed = VectorNormalize( flatvelocity );
	
#if 1
	// Copy movement amounts
	float fmove = mv->m_flForwardMove;
	float smove = mv->m_flSideMove;

	for ( int iAxis = 0; iAxis < 2; ++iAxis )
	{
		flatforward[iAxis] = forward[iAxis] * fmove + right[iAxis] * smove;
	}
#else
	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
#endif
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	// Are we backing into water from steps or something?  If so, don't pop forward
	if ( curspeed != 0.0 && ( DotProduct( flatvelocity, flatforward ) < 0.0 ) && !bJump )
		return;

	Vector vecStart;
	// Start line trace at waist height (using the center of the player for this here)
 	vecStart= mv->GetAbsOrigin() + (GetPlayerMins() + GetPlayerMaxs() ) * 0.5;

	Vector vecEnd;
	VectorMA( vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd );
	
	trace_t tr;
	TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
	if ( tr.fraction < 1.0 )		// solid at waist
	{
		IPhysicsObject *pPhysObj = tr.m_pEnt->VPhysicsGetObject();
		if ( pPhysObj && ( pPhysObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
			return;

		// Fix Civilian being too short to get out of water on e.g. cp_well
		// working minimum is 65 - Scout's height as found in g_TFClassViewVectors
		float eyeHeight = Max( player->GetViewOffset().z, 65.0f );

		vecStart.z = mv->GetAbsOrigin().z + eyeHeight + WATERJUMP_HEIGHT; 
		VectorMA( vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd );
		VectorMA( vec3_origin, -50.0f, tr.plane.normal, player->m_vecWaterJumpVel );

		TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
		if ( tr.fraction == 1.0 )		// open at eye level
		{
			// Now trace down to see if we would actually land on a standable surface.
			VectorCopy( vecEnd, vecStart );
			vecEnd.z -= 1024.0f;
			TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
			if ( ( tr.fraction < 1.0f ) && ( tr.plane.normal.z >= 0.7 ) )
			{
				mv->m_vecVelocity[2] = TF_WATERJUMP_UP/*tf_waterjump_up.GetFloat()*/;		// Push up
				mv->m_nOldButtons |= IN_JUMP;		// Don't jump again until released
				player->AddFlag( FL_WATERJUMP );
				player->m_flWaterJumpTime = 2000.0f;	// Do this for 2 seconds
			}
		}
	}
}


void CTFGameMovement::CheckFalling( void )
{
	// if we landed on the ground
	if ( player->GetGroundEntity() != NULL && !IsDead() )
	{
		// turn off the jumping flag if we're on ground after a jump
		if ( m_pTFPlayer->m_Shared.IsJumping() )
		{
			m_pTFPlayer->m_Shared.SetJumping( false );
		}
	}

	BaseClass::CheckFalling();
}


void CTFGameMovement::Duck( void )
{
	if ( player->m_Local.m_bDucked && player->m_Local.m_bDucking )
	{
		// Don't allow ducking back until fully unducked.
		mv->m_nButtons &= ~IN_DUCK;
	}
	else if ( ( ( player->GetWaterLevel() >= WL_Feet ) && ( player->GetGroundEntity() == NULL ) ) ||
		 player->GetWaterLevel() >= WL_Eyes )
	{
		// Don't allowing ducking in water.
		mv->m_nButtons &= ~IN_DUCK;
	}

	int buttonsChanged = ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed = buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"
	int buttonsReleased = buttonsChanged & mv->m_nOldButtons;		// The changed ones which were previously down are "released"

	// Check to see if we are in the air.
	bool bInAir = ( player->GetGroundEntity() == NULL );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;

	// If player is over air ducks limit he can't air duck again until he lands.
	bool bCanAirDuck = !tf_clamp_airducks.GetBool() || m_pTFPlayer->m_Shared.GetAirDucks() < TF_MAX_AIR_DUCKS;

	if ( mv->m_nButtons & IN_DUCK )
	{
		mv->m_nOldButtons |= IN_DUCK;
	}
	else
	{
		mv->m_nOldButtons &= ~IN_DUCK;
	}

	// Handle death.
	if ( IsDead() )
		return;
	
	if ( !m_pTFPlayer->CanDuck() )
		return;

	// Slow down ducked players.
	HandleDuckingSpeedCrop();

	// If the player is holding down the duck button, the player is in duck transition, ducking, or duck-jumping.
	if ( ( mv->m_nButtons & IN_DUCK ) || player->m_Local.m_bDucking || bInDuck )
	{
		// DUCK
		if ( ( mv->m_nButtons & IN_DUCK ) )
		{
			// Have the duck button pressed, but the player currently isn't in the duck position.
			if ( ( buttonsPressed & IN_DUCK ) && !bInDuck && ( !bInAir || bCanAirDuck ) )
			{
				player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
				player->m_Local.m_bDucking = true;
			}

			// The player is in duck transition and not duck-jumping.
			if ( player->m_Local.m_bDucking )
			{
				float flDuckMilliseconds = Max( 0.0f, GAMEMOVEMENT_DUCK_TIME - (float)player->m_Local.m_flDucktime );
				float flDuckSeconds = flDuckMilliseconds * 0.001f;

				// Finish in duck transition when transition time is over, in "duck", in air.
				if ( ( flDuckSeconds > TIME_TO_DUCK ) || bInDuck || bInAir )
				{
					FinishDuck();

					if ( bInAir && m_pTFPlayer->m_Shared.GetAirDucks() < TF_MAX_AIR_DUCKS )
					{
						// Ducked in mid-air, increment air ducks count.
						m_pTFPlayer->m_Shared.IncrementAirDucks();
					}
				}
				else
				{
					// Calc parametric time
					float flDuckFraction = SimpleSpline( flDuckSeconds / TIME_TO_DUCK );
					SetDuckedEyeOffset( flDuckFraction );
				}
			}
		}
		// UNDUCK (or attempt to...)
		else
		{
			// Try to unduck unless automovement is not allowed
			// NOTE: When not onground, you can always unduck
			if ( player->m_Local.m_bAllowAutoMovement || bInAir || player->m_Local.m_bDucking )
			{
				// We released the duck button, we aren't in "duck" and we are not in the air - start unduck transition.
				if ( ( buttonsReleased & IN_DUCK ) )
				{
					if ( bInDuck )
					{
						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
					}
					else if ( player->m_Local.m_bDucking && !player->m_Local.m_bDucked )
					{
						// Invert time if release before fully ducked!!!
						float unduckMilliseconds = 1000.0f * TIME_TO_UNDUCK;
						float duckMilliseconds = 1000.0f * TIME_TO_DUCK;
						float elapsedMilliseconds = GAMEMOVEMENT_DUCK_TIME - player->m_Local.m_flDucktime;

						float fracDucked = elapsedMilliseconds / duckMilliseconds;
						float remainingUnduckMilliseconds = fracDucked * unduckMilliseconds;

						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME - unduckMilliseconds + remainingUnduckMilliseconds;
					}
				}


				// Check to see if we are capable of unducking.
				if ( CanUnduck() )
				{
					// or unducking
					if ( ( player->m_Local.m_bDucking || player->m_Local.m_bDucked ) )
					{
						float flDuckMilliseconds = Max( 0.0f, GAMEMOVEMENT_DUCK_TIME - (float)player->m_Local.m_flDucktime );
						float flDuckSeconds = flDuckMilliseconds * 0.001f;

						// Finish ducking immediately if duck time is over or not on ground
						if ( flDuckSeconds > TIME_TO_UNDUCK || bInAir )
						{
							FinishUnDuck();
						}
						else
						{
							// Calc parametric time
							float flDuckFraction = SimpleSpline( 1.0f - ( flDuckSeconds / TIME_TO_UNDUCK ) );
							SetDuckedEyeOffset( flDuckFraction );
							player->m_Local.m_bDucking = true;
						}
					}
				}
				else
				{
					// Still under something where we can't unduck, so make sure we reset this timer so
					//  that we'll unduck once we exit the tunnel, etc.
					if ( player->m_Local.m_flDucktime != GAMEMOVEMENT_DUCK_TIME )
					{
						SetDuckedEyeOffset( 1.0f );
						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
						player->m_Local.m_bDucked = true;
						player->m_Local.m_bDucking = false;
						player->AddFlag( FL_DUCKING );
					}
				}
			}
		}
	}
	// HACK: (jimd 5/25/2006) we have a reoccuring bug (#50063 in Tracker) where the player's
	// view height gets left at the ducked height while the player is standing, but we haven't
	// been  able to repro it to find the cause.  It may be fixed now due to a change I'm
	// also making in UpdateDuckJumpEyeOffset but just in case, this code will sense the 
	// problem and restore the eye to the proper position.  It doesn't smooth the transition,
	// but it is preferable to leaving the player's view too low.
	//
	// If the player is still alive and not an observer, check to make sure that
	// his view height is at the standing height.
	else if ( !IsDead() && !player->IsObserver() && !player->IsInAVehicle() )
	{
		if ( ( player->m_Local.m_flDuckJumpTime == 0.0f ) && ( fabs( player->GetViewOffset().z - GetPlayerViewOffset( false ).z ) > 0.1 ) )
		{
			// we should rarely ever get here, so assert so a coder knows when it happens
			Assert( 0 );
			DevMsg( 2, "Restoring player view height\n" );

			// set the eye height to the non-ducked height
			SetDuckedEyeOffset( 0.0f );
		}
	}
}

void CTFGameMovement::HandleDuckingSpeedCrop( void )
{
	BaseClass::HandleDuckingSpeedCrop();

	if ( m_iSpeedCropped & SPEED_CROPPED_DUCK )
	{
		// Prevent moving while crouched in loser state.
		if ( m_pTFPlayer->m_Shared.IsLoser() )
		{
			mv->m_flForwardMove = 0.0f;
			mv->m_flSideMove = 0.0f;
			mv->m_flUpMove = 0.0f;

			// Early out here.
			return;
		}

		// Heavy can't crouchwalk while revved up.
		CTFWeaponBase *pActiveWeapon = m_pTFPlayer->GetActiveTFWeapon();
		int bCrouchWalkHeavy = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pActiveWeapon, bCrouchWalkHeavy, minigun_deployed_crouchwalk);

		if ( !bCrouchWalkHeavy && pActiveWeapon && pActiveWeapon->IsMinigun() && m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			mv->m_flForwardMove = 0.0f;
			mv->m_flSideMove = 0.0f;
			mv->m_flUpMove = 0.0f;

			// Ditto.
			return;
		}
	}
}

void CTFGameMovement::FullWalkMoveUnderwater()
{
	if ( player->GetWaterLevel() == WL_Waist )
	{
		CheckWaterJump();
	}

	// If we are falling again, then we must not trying to jump out of water any more.
	if ( mv->m_vecVelocity.z < 0.0f && player->m_flWaterJumpTime )
	{
		player->m_flWaterJumpTime = 0.0f;
	}

	// Was jump button pressed?
	if ( mv->m_nButtons & IN_JUMP )
	{
		CheckJumpButton();
	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	// Perform regular water movement.
	WaterMove();

	// Redetermine position vars.
	CategorizePosition();

	// If we are on ground, no downward velocity.
	if ( player->GetGroundEntity() != NULL )
	{
		mv->m_vecVelocity[2] = 0;			
	}
}


void CTFGameMovement::FullWalkMove()
{
	if ( !InWater() ) 
	{
		StartGravity();
	}

	// If we are leaping out of the water, just update the counters.
	if ( player->m_flWaterJumpTime )
	{
		// Try to jump out of the water (and check to see if we still are).
		WaterJump();
		TryPlayerMove();
		CheckWater();
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	// of, and, if so, start out jump, otherwise, if we are not moving up, then reset jump timer to 0.
	if ( InWater() ) 
	{
		FullWalkMoveUnderwater();
		return;
	}

	if ( mv->m_nButtons & IN_JUMP )
	{
		CheckJumpButton();
	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	// Make sure velocity is valid.
	CheckVelocity();

	if ( player->GetGroundEntity() != NULL )
	{
		mv->m_vecVelocity[2] = 0.0;
		Friction();
		WalkMove();
	}
	else
	{
		AirMove();
	}

	// Set final flags.
	CategorizePosition();

	// Add any remaining gravitational component if we are not in water.
	if ( !InWater() )
	{
		FinishGravity();
	}

	// If we are on ground, no downward velocity.
	if ( player->GetGroundEntity() != NULL )
	{
		mv->m_vecVelocity[2] = 0;
	}

	// Handling falling.
	CheckFalling();

	// Make sure velocity is valid.
	CheckVelocity();
}


void CTFGameMovement::FullTossMove( void )
{
	trace_t pm;
	Vector move;

	// Add velocity if player is moving.
	if ( mv->m_flForwardMove != 0.0f || mv->m_flSideMove != 0.0f || mv->m_flUpMove != 0.0f )
	{
		Vector forward, right, up;
		float fmove, smove;
		Vector wishdir, wishvel;
		float wishspeed;
		int i;

		AngleVectors( mv->m_vecViewAngles, &forward, &right, &up );  // Determine movement angles.

		// Copy movement amounts.
		fmove = mv->m_flForwardMove;
		smove = mv->m_flSideMove;

		VectorNormalize( forward );		// Normalize remainder of vectors.
		VectorNormalize( right );		// 

		for ( i = 0 ; i < 3 ; i++ )		// Determine x and y parts of velocity.
			wishvel[i] = forward[i]*fmove + right[i] * smove;

		wishvel[2] += mv->m_flUpMove;

		VectorCopy( wishvel, wishdir ); // Determine maginitude of speed of move.
		wishspeed = VectorNormalize( wishdir );

		//
		// Clamp to server defined max speed.
		//
		if ( wishspeed > mv->m_flMaxSpeed )
		{
			VectorScale( wishvel, mv->m_flMaxSpeed / wishspeed, wishvel );
			wishspeed = mv->m_flMaxSpeed;
		}

		// Set pmove velocity.
		Accelerate( wishdir, wishspeed, sv_accelerate.GetFloat() );
	}

	if ( mv->m_vecVelocity[2] > 0 )
	{
		SetGroundEntity( NULL );
	}

	// If on ground and not moving, return..
	if ( player->GetGroundEntity() != NULL )
	{
		if ( VectorCompare( player->GetBaseVelocity(), vec3_origin ) &&
			VectorCompare( mv->m_vecVelocity, vec3_origin ) )
			return;
	}

	CheckVelocity();

	// Add gravity.
	if ( player->GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		AddGravity();
	}

	// Move origin.
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account.
	VectorAdd( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	CheckVelocity();

	VectorScale( mv->m_vecVelocity, gpGlobals->frametime, move );
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	PushEntity( move, &pm );	// Should this clear basevelocity.

	CheckVelocity();

	if ( pm.allsolid )
	{	
		// Entity is trapped in another solid.
		SetGroundEntity( &pm );
		mv->m_vecVelocity.Init();
		return;
	}

	if ( pm.fraction != 1.0f )
	{
		PerformFlyCollisionResolution( pm, move );
	}

	// Check for in water.
	CheckWater();
}


bool CTFGameMovement::HighMaxSpeedMove( void )
{
	// HACK: Move cvars default to 450 but TF2 run speed can go up to 520. We need to get around that.
	if ( fabs( mv->m_flForwardMove ) < m_pTFPlayer->MaxSpeed() )
	{
		if ( AlmostEqual( mv->m_flForwardMove, cl_forwardspeed.GetFloat() ) )
		{
			mv->m_flForwardMove = m_pTFPlayer->MaxSpeed();
		}
		else if ( AlmostEqual( mv->m_flForwardMove, -cl_backspeed.GetFloat() ) )
		{
			mv->m_flForwardMove = -m_pTFPlayer->MaxSpeed();
		}
	}

	if ( fabs( mv->m_flSideMove ) < m_pTFPlayer->MaxSpeed() )
	{
		if ( AlmostEqual( mv->m_flSideMove, cl_sidespeed.GetFloat() ) )
		{
			mv->m_flSideMove = m_pTFPlayer->MaxSpeed();
		}
		else if ( AlmostEqual( mv->m_flSideMove, -cl_sidespeed.GetFloat() ) )
		{
			mv->m_flSideMove = -m_pTFPlayer->MaxSpeed();
		}
	}

	return true;
}


bool CTFGameMovement::StunMove( void )
{
	// Handle control stun.
	if ( m_pTFPlayer->m_Shared.IsControlStunned() 
		|| m_pTFPlayer->m_Shared.IsLoserStateStunned() )
	{
		// Can't fire or select weapons.
		if ( m_pTFPlayer->IsActiveTFWeapon( TF_WEAPON_MINIGUN ) || m_pTFPlayer->IsActiveTFWeapon( TF_WEAPON_AAGUN ) )
		{
			// Heavies can still spin their gun.
			if ( mv->m_nButtons & IN_ATTACK2 || mv->m_nButtons & IN_ATTACK )
			{
				mv->m_nButtons = IN_ATTACK2; // Turn off all other buttons.
			}
		}
		else
		{
			mv->m_nButtons = 0;
		}

		if ( m_pTFPlayer->m_Shared.IsControlStunned() )
		{
			mv->m_flForwardMove = 0.0f;
			mv->m_flSideMove = 0.0f;
			mv->m_flUpMove = 0.0f;
		}

		m_pTFPlayer->m_nButtons = mv->m_nButtons;
	}

	// Handle movement stuns
	float flStunAmount = m_pTFPlayer->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT );
	// Lerp to the desired amount
	if ( flStunAmount )
	{
		if ( m_pTFPlayer->m_Shared.m_flStunLerpTarget != flStunAmount )
		{
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = gpGlobals->curtime;
			m_pTFPlayer->m_Shared.m_flStunLerpTarget = flStunAmount;
			m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut = true;
		}

		mv->m_flForwardMove *= 1.f - flStunAmount;
		mv->m_flSideMove *= 1.f - flStunAmount;
		if ( m_pTFPlayer->m_Shared.GetStunFlags() & TF_STUN_MOVEMENT_FORWARD_ONLY )
		{
			mv->m_flForwardMove = 0.f;
		}

		return true;
	}
	else if ( m_pTFPlayer->m_Shared.m_flLastMovementStunChange )
	{
		// Lerp out to normal speed
		if ( m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut )
		{
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = gpGlobals->curtime;
			m_pTFPlayer->m_Shared.m_bStunNeedsFadeOut = false;
		}

		float flCurStun = RemapValClamped( ( gpGlobals->curtime - m_pTFPlayer->m_Shared.m_flLastMovementStunChange ), 0.2, 0.0, 0.0, 1.0 );
		if ( flCurStun )
		{
			float flRemap = m_pTFPlayer->m_Shared.m_flStunLerpTarget * flCurStun;
			mv->m_flForwardMove *= ( 1.0 - flRemap );
			mv->m_flSideMove *= ( 1.0 - flRemap );
			if ( m_pTFPlayer->m_Shared.GetStunFlags() & TF_STUN_MOVEMENT_FORWARD_ONLY )
			{
				mv->m_flForwardMove = 0.f;
			}
		}
		else
		{
			m_pTFPlayer->m_Shared.m_flStunLerpTarget = 0.f;
			m_pTFPlayer->m_Shared.m_flLastMovementStunChange = 0;
		}

		return true;
	}

	return false;
}


void CTFGameMovement::TauntMove( void )
{
	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
#ifdef ITEM_TAUNTING
		if ( m_pTFPlayer->m_bAllowMoveDuringTaunt )
		{
			if ( m_pTFPlayer->m_bTauntForceForward )
			{
				mv->m_flForwardMove = m_pTFPlayer->m_flTauntSpeed;
			}
			else
			{
				// Limit the speed to taunt's move speed.
				float flMoveSpeed = m_pTFPlayer->m_flTauntSpeed;
				mv->m_flForwardMove = clamp( mv->m_flForwardMove, -flMoveSpeed, flMoveSpeed );
			}

			// Don't strafe, turn instead.
			if ( mv->m_flSideMove != 0.0f )
			{
				// Invert turning direction when moving backwards.
				if ( mv->m_flForwardMove < 0.0f )
					mv->m_flSideMove = -mv->m_flSideMove;

				float flTurnSpeed = m_pTFPlayer->m_flTauntTurnSpeed;
				float flYaw = mv->m_vecViewAngles[YAW];
				flYaw -= clamp( mv->m_flSideMove, -flTurnSpeed, flTurnSpeed ) * gpGlobals->frametime;
				mv->m_vecViewAngles[YAW] = AngleNormalize( flYaw );
				mv->m_flSideMove = 0.0f;
			}
		}
		else
#endif
		{
			// Prevent any movement.
			mv->m_flForwardMove = 0.0f;
			mv->m_flSideMove = 0.0f;
			mv->m_flUpMove = 0.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the mv->GetAbsOrigin() and mv->m_vecVelocity.  It returns a new
//          new mv->GetAbsOrigin(), mv->m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CTFGameMovement::StepMove( Vector &vecDestination, trace_t &trace )
{
	trace_t saveTrace;
	saveTrace = trace;

	Vector vecEndPos;
	VectorCopy( vecDestination, vecEndPos );

	Vector vecPos, vecVel;
	VectorCopy( mv->GetAbsOrigin(), vecPos );
	VectorCopy( mv->m_vecVelocity, vecVel );

	bool bLowRoad = false;
	bool bUpRoad = true;

	// First try the "high road" where we move up and over obstacles
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		// Trace up by step height
		VectorCopy( mv->GetAbsOrigin(), vecEndPos );
		vecEndPos.z += player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		if ( !trace.startsolid && !trace.allsolid )
		{
			mv->SetAbsOrigin( trace.endpos );
		}

		// Trace over from there
		TryPlayerMove();

		// Then trace back down by step height to get final position
		VectorCopy( mv->GetAbsOrigin(), vecEndPos );
		vecEndPos.z -= player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox( mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
		// If the trace ended up in empty space, copy the end over to the origin.
		if ( !trace.startsolid && !trace.allsolid )
		{
			mv->SetAbsOrigin( trace.endpos );
		}

		// If we are not on the standable ground any more or going the "high road" didn't move us at all, then we'll also want to check the "low road"
		if ( ( trace.fraction != 1.0f && 
			trace.plane.normal[2] < 0.7 ) || VectorCompare( mv->GetAbsOrigin(), vecPos ) )
		{
			bLowRoad = true;
			bUpRoad = false;
		}
	}
	else
	{
		bLowRoad = true;
		bUpRoad = false;
	}

	if ( bLowRoad )
	{
		// Save off upward results
		Vector vecUpPos, vecUpVel;
		if ( bUpRoad )
		{
			VectorCopy( mv->GetAbsOrigin(), vecUpPos );
			VectorCopy( mv->m_vecVelocity, vecUpVel );
		}

		// Take the "low" road
		mv->SetAbsOrigin( vecPos );
		VectorCopy( vecVel, mv->m_vecVelocity );
		VectorCopy( vecDestination, vecEndPos );
		TryPlayerMove( &vecEndPos, &saveTrace );

		// Down results.
		Vector vecDownPos, vecDownVel;
		VectorCopy( mv->GetAbsOrigin(), vecDownPos );
		VectorCopy( mv->m_vecVelocity, vecDownVel );

		if ( bUpRoad )
		{
			float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
			float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	
			// decide which one went farther
			if ( flUpDist >= flDownDist )
			{
				mv->SetAbsOrigin( vecUpPos );
				VectorCopy( vecUpVel, mv->m_vecVelocity );

				// copy z value from the Low Road move
				mv->m_vecVelocity.z = vecDownVel.z;
			}
		}
	}

	float flStepDist = mv->GetAbsOrigin().z - vecPos.z;
	if ( flStepDist > 0 )
	{
		mv->m_outStepHeight += flStepDist;
	}
}

bool CTFGameMovement::GameHasLadders() const
{
	return true;
}

void CTFGameMovement::SetGroundEntity( trace_t *pm )
{
	BaseClass::SetGroundEntity( pm );

	CBaseEntity *pNewGround = pm ? pm->m_pEnt : NULL;
	if ( pNewGround )
	{
		m_pTFPlayer->m_Shared.SetAirDash( false );
		m_pTFPlayer->m_Shared.ResetAirDucks();
	}
}


void CTFGameMovement::PlayerRoughLandingEffects( float fvol )
{
	if ( !m_pTFPlayer )
		return;

	if ( m_pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT, true ) )
	{
		// Scouts don't play rumble unless they take damage.
		if ( fvol < 1.0 )
		{
			fvol = 0;
		}
	}

	BaseClass::PlayerRoughLandingEffects( fvol );
}
