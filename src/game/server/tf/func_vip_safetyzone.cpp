//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "func_vip_safetyzone.h"
#include "tf_gamerules.h"
#include "tf_player.h"

LINK_ENTITY_TO_CLASS( func_vip_safetyzone, CVIPSafetyZone );

BEGIN_DATADESC( CVIPSafetyZone )
	DEFINE_ENTITYFUNC( VIPTouch ),
END_DATADESC()


void CVIPSafetyZone::Spawn( void )
{
	InitTrigger();
	BaseClass::Spawn();
	SetTouch( &CVIPSafetyZone::VIPTouch );
}


void CVIPSafetyZone::VIPTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled || !pOther )
		return;

	// Only work in VIP.
	if ( !TFGameRules()->IsVIPMode() || !TFGameRules()->PointsMayBeCaptured() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer && pPlayer->IsVIP() && pPlayer->IsAlive() )
	{
		if ( IsNoInvuln() && pPlayer->m_Shared.IsInvulnerable() )
		{
			return;
		}

		if ( IsBlockable() )
		{
			// Abort if any enemy players are inside the zone.
			FOR_EACH_VEC( m_hTouchingEntities, i )
			{
				CBaseEntity *pOther = m_hTouchingEntities[i].Get();
				if ( pOther && pOther->IsPlayer() && pPlayer->IsEnemy( pOther ) )
					return;
			}
		}

		TFGameRules()->OnVIPEscaped( pPlayer, IsWin() );
	}
}
