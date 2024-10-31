//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bottle.h"
#ifdef CLIENT_DLL
#include "c_tf_fx.h"
#else
#include "tf_fx.h"
#endif

//=============================================================================
//
// Weapon Bottle tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBottle, DT_TFWeaponBottle )

BEGIN_NETWORK_TABLE( CTFBottle, DT_TFWeaponBottle )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bBroken ) )
#else
	SendPropBool( SENDINFO( m_bBroken ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBottle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bBroken, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bottle, CTFBottle );
PRECACHE_WEAPON_REGISTER( tf_weapon_bottle );

//=============================================================================
//
// Weapon Bottle functions.
//

CTFBottle::CTFBottle()
{
}


void CTFBottle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bBroken = false;
}


bool CTFBottle::Deploy( void )
{
	bool bRet = BaseClass::Deploy();

	if ( bRet )
	{
		SwitchBodyGroups();
	}

	return bRet;
}


void CTFBottle::SwitchBodyGroups( void )
{
	SetBodygroup( 1, m_bBroken ? 1 : 0 );
}


void CTFBottle::DoImpactEffect( trace_t &tr, int nDamageType )
{
	// Play different sounds when broken and intact.
	if ( !m_bBroken )
	{
		if ( tr.m_pEnt->IsPlayer() )
		{
			WeaponSound( SINGLE );
		}
		else
		{
			WeaponSound( SINGLE_NPC );
		}

		if ( IsCurrentAttackACrit() )
		{
			m_bBroken = true;
			WeaponSound( SPECIAL1 );
			SwitchBodyGroups();

			//DispatchParticleEffect( "impact_glass", tr.endpos, vec3_angle );
#ifdef CLIENT_DLL
			CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, "impact_glass" );
			if ( pEffect->IsValid() )
			{
				Vector vecImpactPoint = ( tr.fraction != 1.0f ) ? tr.endpos : tr.startpos;
				Vector vecShotDir = vecImpactPoint - tr.startpos;

				Vector	vecReflect;
				float	flDot = DotProduct( vecShotDir, tr.plane.normal );
				VectorMA( vecShotDir, -2.0f * flDot, tr.plane.normal, vecReflect );

				Vector vecShotBackward;
				VectorMultiply( vecShotDir, -1.0f, vecShotBackward );

				Vector vecImpactY, vecImpactZ;
				VectorVectors( tr.plane.normal, vecImpactY, vecImpactZ );
				vecImpactY *= -1.0f;

				// CP 0
				pEffect->SetControlPoint( 0, vecImpactPoint );
				pEffect->SetControlPointOrientation( 0, tr.plane.normal, vecImpactY, vecImpactZ );
				pEffect->SetControlPointEntity( 0, tr.m_pEnt );

				VectorVectors( vecReflect, vecImpactY, vecImpactZ );
				vecImpactY *= -1.0f;

				// CP 1
				pEffect->SetControlPoint( 1, vecImpactPoint );
				pEffect->SetControlPointOrientation( 1, vecReflect, vecImpactY, vecImpactZ );
				pEffect->SetControlPointEntity( 1, tr.m_pEnt );

				VectorVectors( vecShotBackward, vecImpactY, vecImpactZ );
				vecImpactY *= -1.0f;

				// CP 2
				pEffect->SetControlPoint( 2, vecImpactPoint );
				pEffect->SetControlPointOrientation( 2, vecShotBackward, vecImpactY, vecImpactZ );
				pEffect->SetControlPointEntity( 2, tr.m_pEnt );

				// CP 3 (scale)
				pEffect->SetControlPoint( 3, Vector( 1.0f ) );
			}
#endif //CLIENT_DLL
		}
	}
	else
	{
		if ( tr.m_pEnt->IsPlayer() )
		{
			WeaponSound( WPN_DOUBLE );
		}
		else
		{
			WeaponSound( DOUBLE_NPC );
		}
	}

	UTIL_ImpactTrace( &tr, nDamageType );
}
