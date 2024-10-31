//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: A clientside, visual only model that's attached to players
//
//=============================================================================

#include "cbase.h"
#include "c_playerattachedmodel.h"
//#include "c_tf_player.h"

#define PAM_ANIMATE_TIME		0.075
#define PAM_ROTATE_TIME			0.075

#define PAM_SCALE_SPEED			7
#define PAM_MAX_SCALE			3
#define PAM_SPIN_SPEED			360


C_PlayerAttachedModel *C_PlayerAttachedModel::Create( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, const Vector &vecOffset, float flLifetime, int iFlags )
{
	C_PlayerAttachedModel *pModel = new C_PlayerAttachedModel();
	if ( !pModel )
		return NULL;

	if ( !pModel->Initialize( pszModelName, pParent, iAttachment, false, vecOffset, flLifetime, iFlags ) )
		return NULL;

	return pModel;
}

C_PlayerAttachedModel *C_PlayerAttachedModel::Create( const char *pszModelName, C_BaseEntity *pParent, float flLifetime, int iFlags )
{
	C_PlayerAttachedModel *pModel = new C_PlayerAttachedModel();
	if ( !pModel )
		return NULL;

	if ( !pModel->Initialize( pszModelName, pParent, -1, true, vec3_origin, flLifetime, iFlags ) )
		return NULL;

	return pModel;
}


bool C_PlayerAttachedModel::Initialize( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, bool bBoneMerge, const Vector &vecOffset, float flLifetime, int iFlags )
{
	AddEffects( EF_NOSHADOW );
	if ( InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		Release();
		return false;
	}

	if ( bBoneMerge )
	{
		FollowEntity( pParent );
		AddEffects( EF_BONEMERGE_FASTCULL );
	}
	else
	{
		SetParent( pParent, iAttachment );
		SetLocalOrigin( vecOffset );
		SetLocalAngles( vec3_angle );

		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	SetOwnerEntity( pParent );

	SetLifetime( flLifetime );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	SetCycle( 0 );

	m_iFlags = iFlags;
	m_flScale = 0;

	if ( m_iFlags & PAM_ROTATE_RANDOMLY )
	{
		m_flRotateAt = gpGlobals->curtime + PAM_ANIMATE_TIME;
	}
	if ( m_iFlags & PAM_ANIMATE_RANDOMLY )
	{
		m_flAnimateAt = gpGlobals->curtime + PAM_ROTATE_TIME;
	}

	return true;
}


void C_PlayerAttachedModel::SetLifetime( float flLifetime )
{
	if ( flLifetime == PAM_PERMANENT )
	{
		m_flExpiresAt = PAM_PERMANENT;
	}
	else
	{
		// Expire when the lifetime is up
		m_flExpiresAt = gpGlobals->curtime + flLifetime;
	}
}


void C_PlayerAttachedModel::ClientThink( void )
{
	if ( !GetMoveParent() || (m_flExpiresAt != PAM_PERMANENT && gpGlobals->curtime > m_flExpiresAt) )
	{
		Release();
		return;
	}

	if ( m_iFlags & PAM_ANIMATE_RANDOMLY && gpGlobals->curtime > m_flAnimateAt )
	{
		float flDelta = RandomFloat(0.2,0.4) * (RandomInt(0,1) == 1 ? 1 : -1);
		float flCycle = clamp( GetCycle() + flDelta, 0, 1 );
		SetCycle( flCycle );
		m_flAnimateAt = gpGlobals->curtime + PAM_ANIMATE_TIME;
	}

	if ( m_iFlags & PAM_ROTATE_RANDOMLY && gpGlobals->curtime > m_flRotateAt )
	{
		SetLocalAngles( QAngle(0,0,RandomFloat(0,360)) );
		m_flRotateAt = gpGlobals->curtime + PAM_ROTATE_TIME;
	}

	if ( m_iFlags & PAM_SPIN_Z )
	{
		float flAng = GetAbsAngles().y + (gpGlobals->frametime * PAM_SPIN_SPEED);
		SetLocalAngles( QAngle(0,flAng,0) );
	}

	if ( m_iFlags & PAM_SCALEUP )
	{
		m_flScale = Min<float>( m_flScale + (gpGlobals->frametime * PAM_SCALE_SPEED), PAM_MAX_SCALE );
	}
}


void C_PlayerAttachedModel::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	BaseClass::ApplyBoneMatrixTransform( transform );

	if ( !(m_iFlags & PAM_SCALEUP) )
		return;

	VectorScale( transform[0], m_flScale, transform[0] );
	VectorScale( transform[1], m_flScale, transform[1] );
	VectorScale( transform[2], m_flScale, transform[2] );
}


int C_PlayerAttachedModel::DrawModel( int flags )
{
	// We don't have a reason to be visible if we have no owner.
	CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
	if ( !pOwner || !pOwner->ShouldDrawThisPlayer() )
		return 0;

	/*
	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( ( pLocalPlayer == pOwner ) || !pLocalPlayer )
		return false;

	if ( ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) && ( pLocalPlayer->GetObserverTarget() == pOwner ) )
		return false;
	*/

	return BaseClass::DrawModel( flags );
}


bool C_PlayerAttachedModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use same lighting origin as player.
	if ( GetMoveParent() )
	{
		pInfo->pLightingOrigin = &GetMoveParent()->WorldSpaceCenter();
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}
