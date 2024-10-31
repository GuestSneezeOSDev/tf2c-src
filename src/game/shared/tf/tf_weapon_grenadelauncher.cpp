//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
#include "c_tf_viewmodeladdon.h"
#include "bone_setup.h"
#endif

ConVar tf2c_grenadelauncher_old_maxammo( "tf2c_grenadelauncher_old_maxammo", "0", FCVAR_REPLICATED );

#define TF_TUBE_COUNT 6

// X is time as a fraction of cProceduralBarrelRotationTime, which is in seconds.
// Y is rotation in degrees
// Z is slope at Y. 
// These are hermite spline control points that match maya.
const Vector cProceduralBarrelRotationAnimationPoints[] = 
{
	Vector( 0,			0,			0 ),
	Vector( 0.7519f,	63.546f,	0 ),
	Vector( 1.0f,		60,			0 )
};

static_assert( ARRAYSIZE( cProceduralBarrelRotationAnimationPoints ) > 1, "cProceduralBarrelRotationAnimationPoints must have at least two elements." );

const float cProceduralBarrelRotationTime = 0.2666f;

//=============================================================================
//
// Weapon Grenade Launcher tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_TFGrenadeLauncher );
BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_TFGrenadeLauncher )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iShellsLoaded ) ),
	RecvPropInt( RECVINFO( m_iCurrentTube ) ),	
	RecvPropInt( RECVINFO( m_iGoalTube ) ), 
#else
	SendPropInt( SENDINFO( m_iShellsLoaded ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCurrentTube ) ),	
	SendPropInt( SENDINFO( m_iGoalTube ) ), 
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeLauncher )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iShellsLoaded, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_iCurrentTube, FIELD_INTEGER ),
	DEFINE_FIELD( m_iGoalTube, FIELD_INTEGER )
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );

//=============================================================================

//=============================================================================
//
// Weapon Grenade Launcher functions.
//


CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;

#ifdef CLIENT_DLL
	m_iDrumPoseParameter = -1;
	m_iGrenadesBodygroup = -1;
	m_iOldClip = -1;
#endif
}


CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
}


bool CTFGrenadeLauncher::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		UpdateGrenadeDrum( true );
		return true;
	}

	return false;
}


void CTFGrenadeLauncher::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iCurrentTube = 0;
	m_iGoalTube = 0;
	m_bCurrentAndGoalTubeEqual = true;
}


bool CTFGrenadeLauncher::SendWeaponAnim( int iActivity )
{
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		m_iGoalTube = ( m_iCurrentTube + 1 ) % TF_TUBE_COUNT;
		m_flBarrelRotateBeginTime = gpGlobals->curtime;
	} 

	return BaseClass::SendWeaponAnim( iActivity );
}


void CTFGrenadeLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( !IsReloading() )
	{
		UpdateGrenadeDrum( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::ItemPreFrame( void )
{
#ifdef CLIENT_DLL
	UpdateBarrelMovement();
#endif

#ifdef GAME_DLL
	if ( gpGlobals->curtime > m_flBarrelRotateBeginTime + cProceduralBarrelRotationTime )
		m_iCurrentTube = m_iGoalTube;
#endif

	BaseClass::ItemPreFrame();
}


void CTFGrenadeLauncher::OnReloadSinglyUpdate( void )
{
	UpdateGrenadeDrum( true );
}


int CTFGrenadeLauncher::GetMaxAmmo( void )
{
	if ( tf2c_grenadelauncher_old_maxammo.GetBool() )
	{
		return 30;
	}

	return BaseClass::GetMaxAmmo();
}


float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	float flVelocity = 1200.0f;
	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );
	return flVelocity;
}


void CTFGrenadeLauncher::UpdateGrenadeDrum( bool bUpdateValue )
{
	if ( !UsesClipsForAmmo1() )
		return;

	if ( bUpdateValue )
	{
		m_iShellsLoaded = Clip1();
	}

#ifdef CLIENT_DLL
	if ( m_iGrenadesBodygroup != -1 )
	{
		SetBodygroup( m_iGrenadesBodygroup, m_iShellsLoaded );
	}

	if ( m_iDrumPoseParameter != -1 )
	{
		SetPoseParameter( m_iDrumPoseParameter, (float)m_iShellsLoaded / (float)GetMaxClip1() );
	}
#endif
}

#ifdef CLIENT_DLL

void CTFGrenadeLauncher::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	m_iOldClip = m_iShellsLoaded;
}


void CTFGrenadeLauncher::OnDataChanged( DataUpdateType_t updateType )
{
	if ( m_bCurrentAndGoalTubeEqual && m_iCurrentTube != m_iGoalTube )
		m_flBarrelRotateBeginTime = gpGlobals->curtime;
	
	m_bCurrentAndGoalTubeEqual = ( m_iCurrentTube == m_iGoalTube );

	BaseClass::OnDataChanged( updateType );

	if ( m_iShellsLoaded != m_iOldClip )
	{
		UpdateGrenadeDrum( false );
	}
}


CStudioHdr *CTFGrenadeLauncher::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "procedural_chamber" );

	// Check that the model is compatible.
	if ( m_pModelWeaponData && m_pModelWeaponData->GetInt( "grenadelauncher_clip", -2 ) == GetMaxClip1() )
	{
		m_iDrumPoseParameter = LookupPoseParameter( "barrel_spin" );
		m_iGrenadesBodygroup = FindBodygroupByName( "grenades" );
	}
	else
	{
		m_iDrumPoseParameter = -1;
		m_iGrenadesBodygroup = -1;
	}

	return hdr;
}


void CTFGrenadeLauncher::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if ( m_iBarrelBone != -1 )
	{
		UpdateBarrelMovement();

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::UpdateBarrelMovement( void )
{
	if ( m_iGoalTube != m_iCurrentTube )
	{
		float flPartialRotationDeg = 0.0f;

		const float tVal = ( gpGlobals->curtime - m_flBarrelRotateBeginTime ) / cProceduralBarrelRotationTime;

		if ( tVal < 1.0f )
		{
			Assert( cProceduralBarrelRotationAnimationPoints[0].x == 0.0f );
			Assert( cProceduralBarrelRotationAnimationPoints[ARRAYSIZE( cProceduralBarrelRotationAnimationPoints ) - 1].x == 1.0f );

			const Vector *pFirst = NULL;
			const Vector *pSecond = NULL;

			for ( int i = 1; i < ARRAYSIZE( cProceduralBarrelRotationAnimationPoints ); ++i ) 
			{
				// Need to be increasing in time, or we won't find the right span. 
				Assert( cProceduralBarrelRotationAnimationPoints[i - 1].x < cProceduralBarrelRotationAnimationPoints[i].x );

				if ( tVal <= cProceduralBarrelRotationAnimationPoints[i].x )
				{
					pFirst = &cProceduralBarrelRotationAnimationPoints[i - 1];
					pSecond = &cProceduralBarrelRotationAnimationPoints[i];
					break;
				}
			}

			Assert( pFirst && pSecond );
			float flPartialT = ( tVal - pFirst->x ) / ( pSecond->x - pFirst->x );
			flPartialRotationDeg = Hermite_Spline( pFirst->y, pSecond->y, pFirst->z, pSecond->z, flPartialT );
		}
		else
		{
			m_iCurrentTube = m_iGoalTube;
			m_bCurrentAndGoalTubeEqual = true;
		}

		const float flBaseDeg = 60.0f * m_iCurrentTube;
		m_flBarrelAngle = DEG2RAD( flBaseDeg + flPartialRotationDeg );
	}
}


void CTFGrenadeLauncher::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		int iBarrelBone = Studio_BoneIndexByName( hdr, "procedural_chamber" );
		if ( iBarrelBone != -1 )
		{
			if ( hdr->boneFlags( iBarrelBone ) & boneMask )
			{
				RadianEuler a;
				QuaternionAngles( q[ iBarrelBone ], a );

				a.z = m_flBarrelAngle;

				AngleQuaternion( a, q[ iBarrelBone ] );
			}
		}
	}
}


void CTFGrenadeLauncher::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );
	UpdateGrenadeDrum( false );
}
#endif


