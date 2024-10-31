//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "fx_impact.h"
#include "engine/IEngineSound.h"
#include "c_basetempentity.h"
#include "c_tf_player.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include <vphysics/constraints.h>
#include <engine/IEngineSound.h>
#include "physics_shared.h"
#include "tier0/vprof.h"
#include "tf_gamerules.h"
#include "decals.h"
#include "c_impact_effects.h"

static void SetImpactControlPoint( CNewParticleEffect *pEffect, int nPoint, const Vector &vecImpactPoint, const Vector &vecForward, C_BaseEntity *pEntity )
{
	Vector vecImpactY, vecImpactZ;
	VectorVectors( vecForward, vecImpactY, vecImpactZ );
	vecImpactY *= -1.0f;

	pEffect->SetControlPoint( nPoint, vecImpactPoint );
	pEffect->SetControlPointOrientation( nPoint, vecForward, vecImpactY, vecImpactZ );
	pEffect->SetControlPointEntity( nPoint, pEntity );
}

void TFPerformImpactEffects( const Vector &vecOrigin, trace_t &tr, int iMaterial, float flScale )
{
	// Compute the impact effect name
	const char *pImpactName;

	switch ( iMaterial )
	{
	case CHAR_TEX_ANTLION:
		pImpactName = "impact_antlion";
		break;
	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_TILE:
		pImpactName = "impact_concrete";
		break;
	case CHAR_TEX_DIRT:
	case CHAR_TEX_SAND:
	case CHAR_TEX_TFSNOW:
		pImpactName = "impact_dirt";
		break;
	case CHAR_TEX_METAL:
	case CHAR_TEX_VENT:
		pImpactName = "impact_metal";
		break;
	case CHAR_TEX_COMPUTER:
		pImpactName = "impact_computer";
		break;
	case CHAR_TEX_WOOD:
		pImpactName = "impact_wood";
		break;
	case CHAR_TEX_GLASS:
		pImpactName = "impact_glass";
		break;
	default:
		// No impact effect for this material.
		return;
	}

	CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pImpactName );
	if ( !pEffect->IsValid() )
		return;

	Vector vecImpactPoint = ( tr.fraction != 1.0f ) ? tr.endpos : vecOrigin;
	Vector vecShotDir = vecImpactPoint - tr.startpos;

	Vector	vecReflect;
	float	flDot = DotProduct( vecShotDir, tr.plane.normal );
	VectorMA( vecShotDir, -2.0f * flDot, tr.plane.normal, vecReflect );

	Vector vecShotBackward;
	VectorMultiply( vecShotDir, -1.0f, vecShotBackward );

	SetImpactControlPoint( pEffect.GetObject(), 0, vecImpactPoint, tr.plane.normal, tr.m_pEnt );
	SetImpactControlPoint( pEffect.GetObject(), 1, vecImpactPoint, vecReflect, tr.m_pEnt );
	SetImpactControlPoint( pEffect.GetObject(), 2, vecImpactPoint, vecShotBackward, tr.m_pEnt );
	pEffect->SetControlPoint( 3, Vector( flScale ) );
	if ( pEffect->m_pDef->ReadsControlPoint( 4 ) )
	{
		Vector vecColor;
		GetColorForSurface( &tr, &vecColor );
		pEffect->SetControlPoint( 4, vecColor );
	}
}

struct grouped_sound_t
{
	string_t	m_SoundName;
	Vector		m_vecPos;
	Vector		m_vecStart;
};

static float g_flLastImpactSoundTime = 0.0f;
static CUtlVector<grouped_sound_t> g_aGroupedSounds;
#define TF2C_IMPACT_DELAY		 0.2f

//-----------------------------------------------------------------------------
// Purpose: Play a sound for an impact. If tr contains a valid hit, use that. 
//			If not, use the passed in origin & surface.
//-----------------------------------------------------------------------------
void TFPlayImpactSound( CBaseEntity *pEntity, trace_t &tr, Vector &vecServerOrigin, int nServerSurfaceProp )
{
	VPROF( "TFPlayImpactSound" );
	surfacedata_t *pdata;
	Vector vecOrigin;

	// If the client-side trace hit a different entity than the server, or
	// the server didn't specify a surfaceprop, then use the client-side trace 
	// material if it's valid.
	if ( tr.DidHit() && ( pEntity != tr.m_pEnt || nServerSurfaceProp == 0 ) )
	{
		nServerSurfaceProp = tr.surface.surfaceProps;
	}
	pdata = physprops->GetSurfaceData( nServerSurfaceProp );
	if ( tr.fraction < 1.0 )
	{
		vecOrigin = tr.endpos;
	}
	else
	{
		vecOrigin = vecServerOrigin;
	}

	// Now play the sound
	if ( pdata->sounds.bulletImpact )
	{
		//if ( gpGlobals->curtime != g_flLastImpactSoundTime )
		if ( gpGlobals->curtime > g_flLastImpactSoundTime + TF2C_IMPACT_DELAY )
		{
			// Entering new frame, clear out grouped up sounds.
			// TF2C: Clear less often.
			g_aGroupedSounds.Purge();
			g_flLastImpactSoundTime = gpGlobals->curtime;
		}

		const char *pszSound = physprops->GetString( pdata->sounds.bulletImpact );

		// Don't play the sound if it's too close to another impact sound that comes from the same source.
		// TF2C: "too close" is now 300 HU instead of having 0 HU tolerance.
		FOR_EACH_VEC( g_aGroupedSounds, i )
		{
			grouped_sound_t *pSound = &g_aGroupedSounds[i];

			if ( tr.startpos.DistToSqr( pSound->m_vecStart ) < Square( 300.0f ) &&
				vecOrigin.DistToSqr( pSound->m_vecPos ) < Square( 300.0f ) &&
				V_stricmp( pSound->m_SoundName, pszSound ) == 0 )
				return;
		}

		// Ok, play the sound and add it to the list.
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, pdata->soundhandles.bulletImpact, &vecOrigin );

		int iSound = g_aGroupedSounds.AddToTail();
		g_aGroupedSounds[iSound].m_SoundName = pszSound;
		g_aGroupedSounds[iSound].m_vecPos = vecOrigin;
		g_aGroupedSounds[iSound].m_vecStart = tr.startpos;

		return;
	}

#ifdef _DEBUG
	Msg( "***ERROR: PlayImpactSound() on a surface with 0 bulletImpactCount!\n" );
#endif //_DEBUG
}

static float g_flLastRicochetSoundTime = 0.0f;
#define TF2C_RICOCHET_DELAY		 0.4f
//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
void ImpactCallback( const CEffectData &data )
{
	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;

	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );
	if ( !pEntity )
		return;

	bool bHit = Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr );

	// If we hit, perform our custom effects and play the sound
	if ( bHit )
	{
		// Check for custom effects based on the Decal index
		TFPerformImpactEffects( vecOrigin, tr, iMaterial, 1.0f );

		// Play a ricochet sound some of the time.
		// TF2C: Limit too many playing at once to avoid Miniguns filling too many channels!
		if ( gpGlobals->curtime > ( g_flLastRicochetSoundTime + TF2C_RICOCHET_DELAY ) )
		{
			if ( ( iDamageType & DMG_BULLET ) && !( iDamageType & DMG_BUCKSHOT ) && RandomInt( 1, 10 ) <= 3 )
			{
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "Bounce.Shrapnel", &vecOrigin );
				g_flLastRicochetSoundTime = gpGlobals->curtime;
			}
		}

		// Play surface-specific impact sound.
		TFPlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
	}
}
DECLARE_CLIENT_EFFECT( "Impact", ImpactCallback );

//-----------------------------------------------------------------------------
// Purpose: Special case for nail impacts.
//-----------------------------------------------------------------------------
void NailImpactCallback( const CEffectData &data )
{
	C_BaseEntity *pEntity = data.GetEntity();
	if ( !pEntity )
		return;

	// Don't decal your teammates or objects on your team.
	int iTeam = data.m_nDamageType;
	if ( iTeam != TEAM_UNASSIGNED )
	{
		if ( iTeam == pEntity->GetTeamNumber() )
			return;

		// Don't impact spies disguised as the same team as this syringe/projectile.
		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer && pPlayer->IsDisguisedEnemy() )
		{
			if ( pPlayer->m_Shared.DisguiseFoolsTeam(iTeam) )
				return;
		}
	}

	CEffectData newData;
	newData.m_vOrigin = data.m_vOrigin;
	newData.m_vStart = data.m_vStart;
	newData.m_nSurfaceProp = data.m_nSurfaceProp;
	newData.m_nHitBox = data.m_nHitBox;
	newData.m_nDamageType = DMG_BULLET;
	newData.m_hEntity = pEntity;
	DispatchEffect( "Impact", newData );
}
DECLARE_CLIENT_EFFECT( "NailImpact", NailImpactCallback );


void TFSplashCallbackHelper( const CEffectData &data, const char *pszEffectName )
{
	Vector	normal;

	AngleVectors( data.m_vAngles, &normal );

	CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pszEffectName );
	if ( pEffect->IsValid() )
	{
		pEffect->SetSortOrigin( data.m_vOrigin );
		pEffect->SetControlPoint( 0, data.m_vOrigin );
		pEffect->SetControlPointOrientation( 0, Vector( 1, 0, 0 ), Vector( 0, 1, 0 ), Vector( 0, 0, 1 ) );
	}
}

void TFSplashCallback( const CEffectData &data )
{
	TFSplashCallbackHelper( data, "water_bulletsplash01" );
}
DECLARE_CLIENT_EFFECT( "tf_gunshotsplash", TFSplashCallback );

void TFSplashCallbackMinigun( const CEffectData &data )
{
	TFSplashCallbackHelper( data, "water_bulletsplash01_minigun" );
}
DECLARE_CLIENT_EFFECT( "tf_gunshotsplash_minigun", TFSplashCallbackMinigun );


IPhysicsObject *GetWorldPhysObject( void );