//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "econ_item_system.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "fx_explosion.h"
#include "tempentity.h"
#include "ragdollexplosionenumerator.h"

//--------------------------------------------------------------------------------------------------------------
extern CTFWeaponInfo *GetTFWeaponInfo( ETFWeaponID iWeapon );

static ConVar tf2c_explosion_ragdolls("tf2c_explosion_ragdolls", "1", FCVAR_ARCHIVE, "If enabled, ragdolls are affected by radial damage explosions");

void TFExplosionCallback( const Vector &vecOrigin, const Vector &vecNormal, ETFWeaponID iWeaponID, ClientEntityHandle_t hEntity, int iPlayerIndex, int iTeam, bool bCrit, int iItemID )
{
	// Get the weapon information.
	CTFWeaponInfo *pWeaponInfo = NULL;
	switch ( iWeaponID )
	{
	case TF_WEAPON_GRENADE_PIPEBOMB:
	case TF_WEAPON_GRENADE_DEMOMAN:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_PIPEBOMBLAUNCHER );
		break;
	case TF_WEAPON_GRENADE_MIRVBOMB:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_GRENADE_MIRV );
		break;
#ifdef TF2C_BETA
	case TF_WEAPON_GRENADE_HEAL:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_HEALLAUNCHER );
		break;
#endif
	default:
		pWeaponInfo = GetTFWeaponInfo( iWeaponID );
		break;
	}

	Assert( pWeaponInfo );

	bool bIsPlayer = false;
	if ( hEntity.Get() )
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance( hEntity );
		if ( pEntity && pEntity->IsPlayer() )
		{
			bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = ( UTIL_PointContents( vecOrigin ) & CONTENTS_WATER );
	bool bInAir = false;
	QAngle angExplosion( 0.0f, 0.0f, 0.0f );

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if ( fabs( vecNormal.x ) < 0.05f && fabs( vecNormal.y ) < 0.05f && fabs( vecNormal.z ) < 0.05f )
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles( vecNormal, angExplosion );
		bInAir = false;
	}

	// Base explosion effect and sound.
	const char *pszFormat = "explosion";
	const char *pszSound = "BaseExplosionEffect.Sound";
	bool bColored = false;

	if ( pWeaponInfo )
	{
		bColored = pWeaponInfo->m_bHasTeamColoredExplosions;

//		if ( !bColored ) {
			if ( bCrit )
			{
				if ( !pWeaponInfo->m_bHasCritExplosions )
				{
					// Not supporting crit explosions.
					bCrit = false;
				}
				else
				{
					bColored = true;
				}
			}
//		}

		// Explosions.
		if ( bIsWater )
		{
			pszFormat = pWeaponInfo->m_szExplosionEffects[TF_EXPLOSION_WATER];

			// stop Flare Gun causing big water splashes
			if ( V_stricmp( pszFormat, "flaregun_destroyed" ) != 0 ){
				// Water splash on surface
				WaterExplosionEffect().Create( vecOrigin, 180.0f, 10.0f, TE_EXPLFLAG_NODLIGHTS );
			}
		}
		else
		{
			if ( bIsPlayer || bInAir )
			{
				pszFormat = pWeaponInfo->m_szExplosionEffects[TF_EXPLOSION_AIR];
			}
			else
			{
				pszFormat = pWeaponInfo->m_szExplosionEffects[TF_EXPLOSION_WALL];
			}
		}

		// Sound.
		if ( pWeaponInfo->m_szExplosionSound[0] != '\0' )
		{
			pszSound = pWeaponInfo->m_szExplosionSound;
		}
	}

	bool bParticleOverride = false;
	if ( iItemID >= 0 )
	{
		// Allow schema to override explosion sound.
		CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemID );
		EconItemVisuals *pVisuals = pItemDef->GetVisuals( iTeam );
		if ( pItemDef )
		{
			if ( pVisuals )
			{
				if ( pVisuals->sound_weapons[SPECIAL1][0] != '\0' )
					pszSound = pVisuals->sound_weapons[SPECIAL1];

				if( pVisuals->explosion_effect[0] )
				{
					bParticleOverride = true;
					pszFormat = pVisuals->explosion_effect;
				}

				if( bCrit && pVisuals->explosion_effect_crit[0] )
				{
					bParticleOverride = true;
					pszFormat = pVisuals->explosion_effect_crit;
				}
			}
		}
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, &vecOrigin );

	char szEffect[128];

	if ( bParticleOverride )
	{
		V_strcpy_safe( szEffect, pszFormat );
	}
	else
	{
		if ( bCrit )
		{
			// Make a team-colored particle.
			V_sprintf_safe( szEffect, "%s_%s_crit", pszFormat, GetTeamSuffix( iTeam ) );
		}
		else if ( bColored )
		{
			V_sprintf_safe( szEffect, "%s_%s", pszFormat, GetTeamSuffix( iTeam ) );
		}
		else
		{
			// Just take the name as it is.
			V_strcpy_safe( szEffect, pszFormat );
		}
	}

	if ( tf2c_explosion_ragdolls.GetBool() )
	{
		float flRadius = 121.0f;
		if (pWeaponInfo && pWeaponInfo->m_flDamageRadius != 0.0f)
		{
			flRadius = pWeaponInfo->m_flDamageRadius;
		}

		CRagdollExplosionEnumerator pRagdollEnumerator(vecOrigin, flRadius, 100.0f);
		partition->EnumerateElementsInSphere(PARTITION_CLIENT_RESPONSIVE_EDICTS, vecOrigin, flRadius, false, &pRagdollEnumerator);
	}

	DispatchParticleEffect( szEffect, vecOrigin, angExplosion );
}


class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector			m_vecOrigin;
	Vector			m_vecNormal;
	ETFWeaponID		m_iWeaponID;
	int				m_iItemID;
	int				m_iPlayerIndex;
	int				m_iTeamNum;
	bool			m_bCritical;
	ClientEntityHandle_t m_hEntity;
};


C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_iItemID = -1;
	m_iPlayerIndex = 0;
	m_iTeamNum = TEAM_UNASSIGNED;
	m_bCritical = false;
	m_hEntity = INVALID_EHANDLE_INDEX;
}


void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity, m_iPlayerIndex, m_iTeamNum, m_bCritical, m_iItemID );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( RECVINFO( m_iItemID ) ),
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iTeamNum ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
END_RECV_TABLE()

