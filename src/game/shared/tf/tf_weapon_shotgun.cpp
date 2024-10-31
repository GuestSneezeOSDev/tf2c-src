//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shotgun.h"

#ifdef CLIENT_DLL
#include "functionproxy.h"
#include "c_tf_player.h"
#endif

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Soldier, tf_weapon_shotgun_soldier )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_HWG, tf_weapon_shotgun_hwg )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Pyro, tf_weapon_shotgun_pyro )
CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )
//CREATE_SIMPLE_WEAPON_TABLE( TFShotgunBuildingRescue, tf_weapon_shotgun_building_rescue )

//=============================================================================
//
// Weapon Shotgun functions.
//


CTFShotgun::CTFShotgun()
{
	m_bReloadsSingly = true;
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Used for Rescue Ranger screen that shows metal amount.
//-----------------------------------------------------------------------------
class CProxyBuildingRescueLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		// Get the owning player.
		C_TFPlayer *pPlayer = ToTFPlayer( pEntity->GetOwnerEntity() );
		if ( !pPlayer )
			return;

		// Figure out how much metal is required to grab a building.
		int iMetalCost = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, iMetalCost, building_teleporting_pickup );
		if ( iMetalCost == 0 )
			return;

		float flScale = 1.0f;

		// We don't know the ammo counts of non-local players so just pretend they have full ammo.
		if ( pPlayer->IsLocalPlayer() )
		{
			int iMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

			// Shrink the texture as the metal value goes down. Show a flatline if they don't have enough metal to pick up a building.
			if ( iMetal >= iMetalCost )
			{
				flScale = RemapValClamped( iMetal,
					iMetalCost, pPlayer->GetMaxAmmo( TF_AMMO_METAL ),
					4.0f, 1.0f );
			}
			else
			{
				flScale = 10.0f;
			}
		}

		VMatrix mat, matTmp;
		MatrixBuildTranslation( mat, -0.5f, -0.5f, 0.0f );
		MatrixBuildScale( matTmp, 1.0f, flScale, 1.0f );
		MatrixMultiply( matTmp, mat, mat );
		MatrixBuildTranslation( matTmp, 0.5f, 0.5f, 0.0f );
		MatrixMultiply( matTmp, mat, mat );

		m_pResult->SetMatrixValue( mat );
	}
};

EXPOSE_INTERFACE( CProxyBuildingRescueLevel, IMaterialProxy, "BuildingRescueLevel" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif
