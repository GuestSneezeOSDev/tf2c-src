//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "tf_weapon_parse.h"

extern CTFWeaponInfo *GetTFWeaponInfo( ETFWeaponID iWeapon );

extern ConVar cl_ejectbrass;

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TF_EjectBrassCallback( const CEffectData &data )
{
	if ( !cl_ejectbrass.GetBool() )
		return;

	CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfo( (ETFWeaponID)data.m_nHitBox );
	if ( !pWeaponInfo )
		return;
	if ( !pWeaponInfo->m_szBrassModel[0] )
		return;

	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );
	
	Vector vecVelocity = random->RandomFloat( 130, 180 ) * vForward +
						 random->RandomFloat( -30, 30 ) * vRight +
						 random->RandomFloat( -30, 30 ) * vUp;

	float flLifeTime = 10.0f;

	const model_t *pModel = engine->LoadModel( pWeaponInfo->m_szBrassModel );
	if ( !pModel )
		return;
	
	int flags = FTENT_GRAVITY | FTENT_COLLIDEALL | FTENT_HITSOUND | FTENT_ROTATE | FTENT_FADEOUT;

	if ( (ETFWeaponID)data.m_nHitBox == TF_WEAPON_MINIGUN )
	{
		// More velocity for Jake
		vecVelocity = random->RandomFloat( 130, 250 ) * vForward +
			random->RandomFloat( -100, 100 ) * vRight +
			random->RandomFloat( -30, 80 ) * vUp;

		flLifeTime = 2.5f;
	}

	vecVelocity += data.m_vStart;

	Assert( pModel );	

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, data.m_vOrigin, vecShellAngles, vecVelocity, flLifeTime, FTENT_NEVERDIE );
	if ( pTemp == NULL )
		return;

	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat(-512,511);
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat(-255,255);

	pTemp->hitSound = ( data.m_nDamageType & DMG_BUCKSHOT ) ? BOUNCE_SHOTSHELL : BOUNCE_SHELL;

	// TF2C: Rapidfire shells only play bounce sounds rarely to avoid filling sound channels!
	if ( (ETFWeaponID)data.m_nHitBox == TF_WEAPON_MINIGUN || (ETFWeaponID)data.m_nHitBox == TF_WEAPON_SMG )
	{
		if ( RandomInt( 1, 10 ) <= 7 )
			pTemp->hitSound = TE_BOUNCE_NULL;
	}

	pTemp->SetGravity( 0.4 );

	pTemp->m_flSpriteScale = 10;

	pTemp->flags = flags;

	// ::ShouldCollide decides what this collides with
	pTemp->flags |= FTENT_COLLISIONGROUP;
	pTemp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
}

DECLARE_CLIENT_EFFECT( "TF_EjectBrass", TF_EjectBrassCallback );