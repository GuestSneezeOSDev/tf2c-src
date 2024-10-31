//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_rocket.h"
#include "particles_new.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_Rocket, DT_TFProjectile_Rocket )
END_NETWORK_TABLE()


C_TFProjectile_Rocket::C_TFProjectile_Rocket( void )
{
	m_pEffect = NULL;
	m_pCritEffect = NULL;
}


C_TFProjectile_Rocket::~C_TFProjectile_Rocket( void )
{
	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
		m_pEffect = NULL;
	}

	if ( m_pCritEffect )
	{
		ParticleProp()->StopEmission( m_pCritEffect );
		m_pCritEffect = NULL;
	}
}


void C_TFProjectile_Rocket::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateRocketTrails();
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		CreateRocketTrails();
	}
}

float UTIL_WaterLevel( const Vector &position, float minz, float maxz );


void C_TFProjectile_Rocket::CreateRocketTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_pEffect )
	{
		ParticleProp()->StopEmission( m_pEffect );
	}

	if ( enginetrace->GetPointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		m_pEffect = ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail" );
		if ( m_pEffect )
		{
			Vector vecSurface = GetAbsOrigin();
			vecSurface.z = UTIL_WaterLevel( GetAbsOrigin(), GetAbsOrigin().z, GetAbsOrigin().z + 256 );
			m_pEffect->SetControlPoint( 1, vecSurface );
		}
	}
	else
	{
		m_pEffect = ParticleProp()->Create( GetProjectileParticleName( "rockettrail", m_hLauncher ), PATTACH_POINT_FOLLOW, "trail");
	}

	if ( m_pCritEffect )
	{
		ParticleProp()->StopEmission( m_pCritEffect );
	}

	if ( m_bCritical )
	{
		const char *pszEffectName = GetProjectileParticleName( "critical_rocket_%s", m_hLauncher, m_bCritical );
		m_pCritEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
	else
	{
		m_pCritEffect = NULL;
	}
}