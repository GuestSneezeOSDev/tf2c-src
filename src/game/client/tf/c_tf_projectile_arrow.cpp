//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_arrow.h"
#include "particles_new.h"
#include "SpriteTrail.h"
#include "c_tf_player.h"
#include "collisionutils.h"
#include "util_shared.h"
#include "c_rope.h"

//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TFProjectile_Arrow )

BEGIN_NETWORK_TABLE( C_TFProjectile_Arrow, DT_TFProjectile_Arrow )
	RecvPropBool( RECVINFO( m_bArrowAlight ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( RECVINFO( m_iProjectileType ) ),
END_NETWORK_TABLE()

#define NEAR_MISS_THRESHOLD		120


C_TFProjectile_Arrow::C_TFProjectile_Arrow( void )
{
	m_fAttachTime = 0.0f;
	m_nextNearMissCheck = 0.0f;
	m_bNearMiss = false;
	m_bArrowAlight = false;
	m_bCritical = true;
	m_pCritEffect = NULL;
	m_iCachedDeflect = false;
	m_flLifeTime = 40.0f;
}


C_TFProjectile_Arrow::~C_TFProjectile_Arrow( void )
{
}


void C_TFProjectile_Arrow::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		if ( m_bArrowAlight )
		{
			m_pBurningArrowEffect = ParticleProp()->Create( "flying_flaming_arrow", PATTACH_POINT_FOLLOW, "muzzle" );
		}
	}
	else if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		if ( m_bArrowAlight && m_pBurningArrowEffect == NULL )
		{
			m_pBurningArrowEffect = ParticleProp()->Create( "flying_flaming_arrow", PATTACH_POINT_FOLLOW, "muzzle" );
		}
	}

	if ( m_bCritical )
	{
		if ( updateType == DATA_UPDATE_CREATED || m_iCachedDeflect != m_iDeflected )
		{
			CreateCritTrail();
		}
	}

	m_iCachedDeflect = m_iDeflected;

	// Kill particles when static (prevent fire stuck in mid-air after player impact).
	if ( m_pBurningArrowEffect && GetMoveType() == MOVETYPE_NONE )
	{
		ParticleProp()->StopEmission( m_pBurningArrowEffect );
		m_pBurningArrowEffect = NULL;
	}
}


void C_TFProjectile_Arrow::NotifyBoneAttached( C_BaseAnimating* attachTarget )
{
	BaseClass::NotifyBoneAttached( attachTarget );

	m_fAttachTime = gpGlobals->curtime;
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TFProjectile_Arrow::ClientThink( void )
{
	// Perform a near-miss check.
	if ( !m_bNearMiss && ( gpGlobals->curtime > m_nextNearMissCheck ) )
	{
		CheckNearMiss();
		m_nextNearMissCheck = gpGlobals->curtime + 0.05f;
	}

	// Remove crit effect if we hit a wall.
	if ( GetMoveType() == MOVETYPE_NONE && m_pCritEffect )
	{
		ParticleProp()->StopEmission( m_pCritEffect );
		m_pCritEffect = NULL;
	}

	BaseClass::ClientThink();

	// DO THIS LAST: Destroy us automatically after a period of time.
	if ( m_pAttachedTo )
	{
		if ( gpGlobals->curtime - m_fAttachTime > m_flLifeTime )
		{
			C_TFPlayer *pOwner = dynamic_cast<C_TFPlayer *>( m_pAttachedTo.Get() );
			if( pOwner )
				pOwner->RemoveArrow( this );

			Release();
			return;
		}
		else if ( m_pAttachedTo->IsEffectActive( EF_NODRAW ) && !IsEffectActive( EF_NODRAW ) )
		{
			AddEffects( EF_NODRAW );
			UpdateVisibility();
		}
		else if ( !m_pAttachedTo->IsEffectActive( EF_NODRAW ) && IsEffectActive( EF_NODRAW ) && m_pAttachedTo != C_BasePlayer::GetLocalPlayer() )
		{
			RemoveEffects( EF_NODRAW );
			UpdateVisibility();
		}
	}

	if ( IsDormant() && !IsEffectActive( EF_NODRAW ) )
	{
		AddEffects( EF_NODRAW );
		UpdateVisibility();
	}
}


void C_TFProjectile_Arrow::CheckNearMiss( void )
{
	// Check against the local player. If we're near him play a near miss sound.
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
		return;

	// If we are attached to something or stationary we don't want to do near miss checks.
	if ( m_pAttachedTo || ( GetMoveType() == MOVETYPE_NONE ) )
	{
		m_bNearMiss = true;
		return;
	}

	// Can't hear near miss sounds from friendly arrows.
	if ( pLocalPlayer->GetTeamNumber() == GetTeamNumber() )
		return;

	Vector vecPlayerPos = pLocalPlayer->GetAbsOrigin();
	Vector vecArrowPos = GetAbsOrigin(), forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vecArrowDest = GetAbsOrigin() + forward * 200.f;

	// If the arrow is moving away from the player just stop checking.
	float dist1 = vecArrowPos.DistToSqr( vecPlayerPos );
	float dist2 = vecArrowDest.DistToSqr( vecPlayerPos );
	if ( dist2 > dist1 )
	{
		m_bNearMiss = true;
		return;
	}

	// Check to see if the arrow is passing near the player.
	Vector vecClosestPoint;
	float dist;
	CalcClosestPointOnLineSegment( vecPlayerPos, vecArrowPos, vecArrowDest, vecClosestPoint, &dist );
	dist = vecPlayerPos.DistTo( vecClosestPoint );
	if ( dist > NEAR_MISS_THRESHOLD )
		return;

	// The arrow is passing close to the local player.
	m_bNearMiss = true;
	SetNextClientThink( CLIENT_THINK_NEVER );

	// If the arrow is about to hit something, don't play the sound and stop this check.
	trace_t tr;
	UTIL_TraceLine( vecArrowPos, vecArrowPos + forward * 400.0f, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.DidHit() )
		return;

	// We're good for a near miss!
	float soundlen = 0;
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "Weapon_Arrow.Nearmiss";
	params.m_pflSoundDuration = &soundlen;
	CSingleUserRecipientFilter localFilter( pLocalPlayer );
	EmitSound( localFilter, pLocalPlayer->entindex(), params );
}


void C_TFProjectile_Arrow::CreateCritTrail( void )
{
	if ( IsDormant() )
		return;

	if ( m_pCritEffect )
	{
		ParticleProp()->StopEmission( m_pCritEffect );
		m_pCritEffect = NULL;
	}

	if ( m_bCritical )
	{
		const char *pszEffect = ConstructTeamParticle( "critical_rocket_%s", GetTeamNumber() );
		m_pCritEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	}
}