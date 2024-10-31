//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "view.h"
#include "collisionutils.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/IEngineSound.h"
#include "tf_weaponbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_drawtracers;
extern ConVar r_drawtracers_firstperson;

//-----------------------------------------------------------------------------
// Purpose: This is largely a copy of ParticleTracer with some fixes.
//-----------------------------------------------------------------------------
void TFParticleTracerCallback( const CEffectData &data )
{
	if ( !r_drawtracers.GetBool() )
		return;

	// Grab the data
	Vector vecStart = data.m_vStart;
	Vector vecEnd = data.m_vOrigin;
	C_BaseEntity *pEntity = data.GetEntity();

	// Move the starting point to an attachment if possible.
	if ( ( data.m_fFlags & TRACER_FLAG_USEATTACHMENT ) && pEntity && !pEntity->IsDormant() )
	{
		int iAttachment = data.m_nAttachmentIndex;
		bool bFirstPerson = false;

		// See if this is a weapon.
		if ( pEntity->IsBaseCombatWeapon() )
		{
			C_TFWeaponBase *pWeapon = static_cast<C_TFWeaponBase *>( pEntity );
			bFirstPerson = pWeapon->UsingViewModel();

			if ( bFirstPerson && !r_drawtracers_firstperson.GetBool() )
				return;

			// Attach it to viewmodel if we're in first person.
			pEntity = pWeapon->GetWeaponForEffect();

			// Need to get attachment index here since the server has no way of knowing
			// the correct index due to differing models.
			iAttachment = pWeapon->GetTracerAttachment();
		}

		pEntity->GetAttachment( iAttachment, vecStart );

		if ( bFirstPerson )
		{
			// Adjust view model tracers
			QAngle	vangles;
			Vector	vforward, vright, vup;

			engine->GetViewAngles( vangles );
			AngleVectors( vangles, &vforward, &vright, &vup );

			VectorMA( vecStart, 4, vright, vecStart );
			vecStart.z -= 0.5f;
		}
	}

	// Create the particle effect
	QAngle vecAngles;
	Vector vecToEnd = vecEnd - vecStart;
	VectorNormalize( vecToEnd );
	VectorAngles( vecToEnd, vecAngles );

	CEffectData	newData;

	newData.m_nHitBox = data.m_nHitBox;
	newData.m_vOrigin = vecStart;
	newData.m_vStart = vecEnd;
	newData.m_vAngles = vecAngles;

	newData.m_bCustomColors = data.m_bCustomColors;
	newData.m_CustomColors.m_vecColor1 = data.m_CustomColors.m_vecColor1;

	DispatchEffect( "ParticleEffect", newData );

	if ( data.m_fFlags & TRACER_FLAG_WHIZ )
	{
		FX_TracerSound( vecStart, vecEnd, TRACER_TYPE_DEFAULT );
	}
}

DECLARE_CLIENT_EFFECT( "TFParticleTracer", TFParticleTracerCallback );
