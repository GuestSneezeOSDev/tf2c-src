//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "tf_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

#define TF_RUN_SPEED			320.0f
#define TF_WALK_SPEED			75.0f
#define TF_CROUCHWALK_SPEED		110.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CTFPlayerAnimState* CreateTFPlayerAnimState( CTFPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = TF_RUN_SPEED;
	movementData.m_flWalkSpeed = TF_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CTFPlayerAnimState *pRet = new CTFPlayerAnimState( pPlayer, movementData );

	// Specific TF player initialization.
	pRet->InitTF( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState()
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::~CTFPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::InitTF( CTFPlayer *pPlayer )
{
	m_pTFPlayer = pPlayer;
	m_bInAirWalk = false;
	m_flHoldDeployedPoseUntilTime = 0.0f;
	m_flTauntAnimTime = 0.0f;
}


void CTFPlayerAnimState::ClearAnimationState( void )
{
	m_bInAirWalk = false;
	m_flLastAimTurnTime = 0.0f;

	BaseClass::ClearAnimationState();
}

acttable_t m_acttableLoserState[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_LOSERSTATE,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_LOSERSTATE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_LOSERSTATE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_LOSERSTATE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_LOSERSTATE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_LOSERSTATE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_LOSERSTATE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_LOSERSTATE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_LOSERSTATE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_LOSERSTATE,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_LOSERSTATE,				false },
	{ ACT_MP_DOUBLEJUMP,		ACT_MP_DOUBLEJUMP_LOSERSTATE,		false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_LOSERSTATE, false },
};

acttable_t m_acttableBuildingDeployed[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_BUILDING_DEPLOYED, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_BUILDING_DEPLOYED, false },
	{ ACT_MP_RUN, ACT_MP_RUN_BUILDING_DEPLOYED, false },
	{ ACT_MP_WALK, ACT_MP_WALK_BUILDING_DEPLOYED, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_BUILDING_DEPLOYED, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_BUILDING_DEPLOYED, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_BUILDING_DEPLOYED, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_BUILDING_DEPLOYED, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_BUILDING_DEPLOYED, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_BUILDING_DEPLOYED, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_BUILDING_DEPLOYED, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_BUILDING_DEPLOYED, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED, false },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CTFPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	if ( GetTFPlayer()->m_Shared.IsLoser() )
	{
		int actCount = ARRAYSIZE( m_acttableLoserState );
		for ( int i = 0; i < actCount; i++ )
		{
			const acttable_t& act = m_acttableLoserState[i];
			if ( actDesired == act.baseAct )
				return (Activity)act.weaponAct;
		}
	}
	else if ( GetTFPlayer()->m_Shared.IsCarryingObject() )
	{
		int actCount = ARRAYSIZE( m_acttableBuildingDeployed );
		for ( int i = 0; i < actCount; i++ )
		{
			const acttable_t& act = m_acttableBuildingDeployed[i];
			if ( actDesired == act.baseAct )
				return (Activity)act.weaponAct;
		}
	}

	CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
	if ( pWeapon )
	{
		translateActivity = pWeapon->ActivityOverride( translateActivity, nullptr );

		// Live TF2 does this but is doing this after the above call correct?
		translateActivity = pWeapon->GetItem()->GetActivityOverride( GetTFPlayer()->GetTeamNumber(), translateActivity );
	}

	return translateActivity;
}


void CTFPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the TF player.
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( !pTFPlayer )
		return;

#ifdef SAPPHO_ANTIAIM
	// is this the final command from this player? if not theyre trying to antiaim
	if (!pTFPlayer->m_bIsFinalCommand)
	{
		return;
	}
#endif

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pTFPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	if ( pTFPlayer->GetPlayerClass()->HasCustomModel() )
	{
		if ( !pTFPlayer->GetPlayerClass()->CustomModelUsesClassAnimations() )
		{
			if ( pTFPlayer->GetPlayerClass()->CustomModelRotates() )
			{
				if ( pTFPlayer->GetPlayerClass()->CustomModelRotationSet() )
				{
					QAngle angRot = pTFPlayer->GetPlayerClass()->GetCustomModelRotation();
					m_angRender = angRot;
				}
				else
				{
					m_angRender = vec3_angle;
					m_angRender[YAW] = AngleNormalize( eyeYaw );
				}
			}

			// Restart our animation whenever we change models
			if ( pTFPlayer->GetPlayerClass()->CustomModelHasChanged() )
			{
				RestartMainSequence();
			}

			// Uncomment this for Live TF2's restricted usage of 'SetCustomModel'.
			/*ClearAnimationState();
			return;*/
		}
	}

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	bool bInTaunt = pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING );
	bool bIsImmobilized = bInTaunt || pTFPlayer->m_Shared.IsControlStunned();

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		if ( m_pTFPlayer->m_Shared.IsMovementLocked() )
		{
			// Keep feet directed forward.
			m_bForceAimYaw = true;
		}

		if ( !bIsImmobilized )
		{
			// Pose parameter - what direction are the player's legs running in.
			ComputePoseParam_MoveYaw( pStudioHdr );
		}

		if ( !bIsImmobilized || bInTaunt )
		{
			// Pose parameter - Torso aiming (up/down).
			ComputePoseParam_AimPitch( pStudioHdr );

			// Pose parameter - Torso aiming (rotation).
			ComputePoseParam_AimYaw( pStudioHdr );
		}
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		GetBasePlayer()->SetPlaybackRate( 1.0f );
	}
#endif
}


Activity CTFPlayerAnimState::CalcMainActivity( void )
{
	CheckStunAnimation();
	return BaseClass::CalcMainActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
		case PLAYERANIMEVENT_ATTACK_PRIMARY:
			{
				CTFPlayer *pPlayer = GetTFPlayer();
				if ( !pPlayer )
					return;

				CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
				bool bIsMinigun = ( pActiveWeapon && pActiveWeapon->IsMinigun() );
				bool bIsSniperRifle = ( pActiveWeapon && pActiveWeapon->IsSniperRifle() );

				// Heavy weapons primary fire.
				if ( bIsMinigun )
				{
					// Play standing primary fire.
					iGestureActivity = ACT_MP_ATTACK_STAND_PRIMARYFIRE;

					if ( m_bInSwim )
					{
						// Play swimming primary fire.
						iGestureActivity = ACT_MP_ATTACK_SWIM_PRIMARYFIRE;
					}
					else if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
					{
						// Play crouching primary fire.
						iGestureActivity = ACT_MP_ATTACK_CROUCH_PRIMARYFIRE;
					}

					if ( !IsGestureSlotPlaying( GESTURE_SLOT_ATTACK_AND_RELOAD, TranslateActivity( iGestureActivity ) ) )
					{
						RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );
					}
				}
				else if ( bIsSniperRifle && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
				{
					// Weapon primary fire, zoomed in
					if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
					{
						RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED );
					}
					else
					{
						RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED );
					}

					iGestureActivity = ACT_VM_PRIMARYATTACK;

					// Hold our deployed pose for a few seconds
					m_flHoldDeployedPoseUntilTime = gpGlobals->curtime + pActiveWeapon->DeployedPoseHoldTime();
				}
				else
				{
					// Weapon primary fire.
					if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
					{
						RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
					}
					else
					{
						RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
					}

					iGestureActivity = ACT_VM_PRIMARYATTACK;
				}

				break;
			}

		case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
			{
				if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
				}
				break;
			}
		case PLAYERANIMEVENT_ATTACK_SECONDARY:
			{
				// Weapon secondary fire.
				if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
				}

				iGestureActivity = ACT_VM_PRIMARYATTACK;
				break;
			}
		case PLAYERANIMEVENT_ATTACK_PRE:
			{
				CTFPlayer *pPlayer = GetTFPlayer();
				if ( !pPlayer )
					return;

				CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
				bool bIsMinigun = ( pActiveWeapon && pActiveWeapon->IsMinigun() );

				bool bAutoKillPreFire = false;
				if ( bIsMinigun )
				{
					bAutoKillPreFire = true;
				}

				if ( m_bInSwim && bIsMinigun )
				{
					// Weapon pre-fire. Used for minigun windup while swimming
					iGestureActivity = ACT_MP_ATTACK_SWIM_PREFIRE;
				}
				else if ( GetBasePlayer()->GetFlags() & FL_DUCKING ) 
				{
					// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
					iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
				}
				else
				{
					// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
					iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
				}

				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, bAutoKillPreFire );

				break;
			}
		case PLAYERANIMEVENT_ATTACK_POST:
			{
				CTFPlayer *pPlayer = GetTFPlayer();
				if ( !pPlayer )
					return;

				CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
				bool bIsMinigun  = ( pActiveWeapon && pActiveWeapon->IsMinigun() );

				if ( m_bInSwim && bIsMinigun )
				{
					// Weapon pre-fire. Used for minigun winddown while swimming
					iGestureActivity = ACT_MP_ATTACK_SWIM_POSTFIRE;
				}
				else if ( GetBasePlayer()->GetFlags() & FL_DUCKING ) 
				{
					// Weapon post-fire. Used for minigun winddown in crouch.
					iGestureActivity = ACT_MP_ATTACK_CROUCH_POSTFIRE;
				}
				else
				{
					// Weapon post-fire. Used for minigun winddown.
					iGestureActivity = ACT_MP_ATTACK_STAND_POSTFIRE;
				}

				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );

				break;
			}

		case PLAYERANIMEVENT_RELOAD:
			{
				// Weapon reload.
				if ( m_bInAirWalk )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK );
				}
				else
				{
					BaseClass::DoAnimationEvent( event, nData );
				}
				break;
			}
		case PLAYERANIMEVENT_RELOAD_LOOP:
			{
				// Weapon reload.
				if ( m_bInAirWalk )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_LOOP );
				}
				else
				{
					BaseClass::DoAnimationEvent( event, nData );
				}
				break;
			}
		case PLAYERANIMEVENT_RELOAD_END:
			{
				// Weapon reload.
				if ( m_bInAirWalk )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_END );
				}
				else
				{
					BaseClass::DoAnimationEvent( event, nData );
				}
				break;
			}
		case PLAYERANIMEVENT_DOUBLEJUMP:
			{
				// Check to see if we are jumping!
				if ( !m_bJumping )
				{
					m_bJumping = true;
					m_bFirstJumpFrame = true;
					m_flJumpStartTime = gpGlobals->curtime;
					RestartMainSequence();
				}

				// Force the air walk off.
				m_bInAirWalk = false;

				// Player the air dash gesture.
				if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP_CROUCH );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP );
				}
				break;
			}
		case PLAYERANIMEVENT_STUN_BEGIN:
			{
				RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_BEGIN );
				break;
			}
		case PLAYERANIMEVENT_STUN_MIDDLE:
			{
				RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_MIDDLE );
				break;
			}
		case PLAYERANIMEVENT_STUN_END:
			{
				RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_END );
				break;
			}
		default:
			{
				BaseClass::DoAnimationEvent( event, nData );
				break;
			}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID && GetBasePlayer() != C_BasePlayer::GetLocalPlayer() )
	{
		CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->SendWeaponAnim( iGestureActivity );
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
}

void CTFPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	CBaseCombatWeapon *pWeapon = m_pTFPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		iGestureActivity = pWeapon->GetItem()->GetActivityOverride( m_pTFPlayer->GetTeamNumber(), iGestureActivity );
	}

	BaseClass::RestartGesture( iGestureSlot, iGestureActivity, bAutoKill );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );
	if ( bInWater )
	{
		if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			CTFWeaponBase *pActiveWeapon = m_pTFPlayer->GetActiveTFWeapon();
			if ( pActiveWeapon && pActiveWeapon->IsMinigun() )
			{
				idealActivity = ACT_MP_SWIM_DEPLOYED;
			}
			// Check if Sniper deployed underwater - should only be when standing on something.
			else if ( pActiveWeapon && pActiveWeapon->IsSniperRifle() )
			{
				if ( m_pTFPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
				{
					idealActivity = ACT_MP_SWIM_DEPLOYED;
				}
			}
		}
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	// If we move, cancel the deployed anim hold.
	float flSpeed = GetOuterXYSpeed();
	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		m_flHoldDeployedPoseUntilTime = 0.0f;
	}

	if ( m_pTFPlayer->m_Shared.IsLoser() )
		return BaseClass::HandleMoving( idealActivity );

	if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) ) 
	{
		if ( flSpeed > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_DEPLOYED;
		}
		else
		{
			idealActivity = ACT_MP_DEPLOYED_IDLE;
		}
	}
	else if ( gpGlobals->curtime < m_flHoldDeployedPoseUntilTime )
	{
		// Unless we move, hold the deployed pose for a number of seconds after being deployed.
		idealActivity = ACT_MP_DEPLOYED_IDLE;
	}
	else 
		return BaseClass::HandleMoving( idealActivity );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED || m_pTFPlayer->m_Shared.IsLoser() )
		{
			idealActivity = ACT_MP_CROUCH_IDLE;		
			if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) || m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
			{
				idealActivity = ACT_MP_CROUCH_DEPLOYED_IDLE;
			}
		}
		else
		{
			idealActivity = ACT_MP_CROUCHWALK;		

			if ( m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
			{
				// Don't do this for the Heavy, we don't usually let him deployed crouch walk.
				CTFPlayer *pPlayer = GetTFPlayer();
				if ( !pPlayer || !( pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->IsMinigun() ) )
				{
					idealActivity = ACT_MP_CROUCH_DEPLOYED;
				}
			}
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
bool CTFPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Don't allow a firing heavy to jump or air walk.
	if ( m_pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS, true ) && m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		m_bJumping = false;
		m_bInAirWalk = false;
		return false;
	}
		
	// Handle air walking before handling jumping - air walking supersedes jump
	bool bOnGround = ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) != 0;
	bool bInWater = ( GetBasePlayer()->GetWaterLevel() >= WL_Waist );

	if ( vecVelocity.z > 300.0f || m_bInAirWalk )
	{
		// Check to see if we were in an airwalk and now we are basically on the ground.
		if ( bOnGround && m_bInAirWalk )
		{				
			m_bInAirWalk = false;
			RestartMainSequence();
			RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );	
		}
		else if ( bInWater )
		{
			// Turn off air walking and reset the animation.
			m_bInAirWalk = false;
			RestartMainSequence();
		}
		else if ( !bOnGround )
		{
			// In an air walk.
			idealActivity = ACT_MP_AIRWALK;
			m_bInAirWalk = true;
		}
	}
	// Jumping.
	else if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( bInWater )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( bOnGround )
			{
				m_bJumping = false;
				RestartMainSequence();
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
			if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
			{
				idealActivity = ACT_MP_JUMP_FLOAT;
			}
			else
			{
				idealActivity = ACT_MP_JUMP_START;
			}
		}
	}

	if ( m_bJumping || m_bInAirWalk )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Updates animation state if we're stunned.
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::CheckStunAnimation()
{
	CTFPlayer *pPlayer = GetTFPlayer();
	if ( !pPlayer )
		return;

	// do not play stun anims if in kart
	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	// State machine to determine the correct stun activity.
	if ( !pPlayer->m_Shared.IsControlStunned() && 
		 ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_LOOP ) )
	{
		// Clean up if the condition went away before we finished.
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );
		pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_NONE;
	}
	else if ( pPlayer->m_Shared.IsControlStunned() &&
		      ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_NONE ) &&
		      ( gpGlobals->curtime < pPlayer->m_Shared.GetStunExpireTime() ) )
	{
		// Play the start up animation.
		int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_BEGIN );
		pPlayer->m_Shared.m_flStunMid = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_BEGIN );
		pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_LOOP;
	}
	else if ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_LOOP )
	{
		// We are playing the looping part of the stun animation cycle.
		if ( gpGlobals->curtime > pPlayer->m_Shared.m_flStunFade )
		{
			// Gameplay is telling us to fade out. Time for the end anim.
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_END );
			pPlayer->m_Shared.SetStunExpireTime( gpGlobals->curtime + pPlayer->SequenceDuration( iSeq ) );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );
			pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_END;
		}
		else if ( gpGlobals->curtime > pPlayer->m_Shared.m_flStunMid )
		{
			// Loop again.
			int iSeq = pPlayer->SelectWeightedSequence( ACT_MP_STUN_MIDDLE );
			pPlayer->m_Shared.m_flStunMid = gpGlobals->curtime + pPlayer->SequenceDuration( iSeq );
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_MIDDLE );
		}
	}
	else if ( pPlayer->m_Shared.m_iStunAnimState == STUN_ANIM_END )
	{
		if ( gpGlobals->curtime > pPlayer->m_Shared.GetStunExpireTime() )
		{
			// The animation loop is over.
			pPlayer->m_Shared.m_iStunAnimState = STUN_ANIM_NONE;
		}
	}
}
