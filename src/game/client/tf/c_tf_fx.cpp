//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_fx_shared.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include <cliententitylist.h>
#include "c_te_effect_dispatch.h"
#include "props_shared.h"
#include "c_tf_player.h"

void WaterPlayerDiveCallback( const CEffectData &data )
{
	C_TFPlayer *pPlayer = ToTFPlayer( data.GetEntity() );
	if ( !pPlayer || pPlayer->IsDormant() )
		return;

	CNewParticleEffect *pEffect = pPlayer->ParticleProp()->Create( "water_playerdive", PATTACH_ABSORIGIN_FOLLOW );
	pPlayer->ParticleProp()->AddControlPoint( pEffect, 1, NULL, PATTACH_WORLDORIGIN, NULL, data.m_vStart );
}
DECLARE_CLIENT_EFFECT( "WaterPlayerDive", WaterPlayerDiveCallback );

void WaterPlayerEmergeCallback( const CEffectData &data )
{
	C_TFPlayer *pPlayer = ToTFPlayer( data.GetEntity() );
	if ( !pPlayer || pPlayer->IsDormant() )
		return;

	CNewParticleEffect *pWaterExitEffect = pPlayer->ParticleProp()->Create( "water_playeremerge", PATTACH_ABSORIGIN_FOLLOW );
	pPlayer->ParticleProp()->AddControlPoint( pWaterExitEffect, 1, pPlayer, PATTACH_ABSORIGIN_FOLLOW );
	pPlayer->m_bWaterExitEffectActive = true;
}
DECLARE_CLIENT_EFFECT( "WaterPlayerEmerge", WaterPlayerEmergeCallback );

//-----------------------------------------------------------------------------
// Purpose: Live TF2 uses BreakModel user message but screw that.
//-----------------------------------------------------------------------------
void BreakModelCallback( const CEffectData &data )
{
	int nModelIndex = data.m_nMaterial;

	CUtlVector<breakmodel_t> aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( aGibs.IsEmpty() )
		return;

	Vector vecVelocity( 0, 0, 200 );
	AngularImpulse angularVelocity( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	breakablepropparams_t params( data.m_vOrigin, data.m_vAngles, vecVelocity, angularVelocity );
	CreateGibsFromList( aGibs, nModelIndex, NULL, params, NULL, -1, false );
}

DECLARE_CLIENT_EFFECT( "BreakModel", BreakModelCallback );
