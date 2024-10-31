//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_nail.h"


//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//
LINK_ENTITY_TO_CLASS( tf_projectile_syringe, CTFProjectile_Syringe );
PRECACHE_REGISTER( tf_projectile_syringe );


CTFProjectile_Syringe *CTFProjectile_Syringe::Create( int iType, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, bool bCritical )
{
	return static_cast<CTFProjectile_Syringe *>( CTFBaseNail::Create( "tf_projectile_syringe", vecOrigin, vecAngles, pOwner, iType, bCritical ) );
}

#ifdef GAME_DLL

ETFWeaponID CTFProjectile_Syringe::GetWeaponID( void ) const
{
	switch ( m_iType )
	{
		case TF_NAIL_NORMAL:
			return TF_WEAPON_NAILGUN;
		case TF_NAIL_SYRINGE:
		default:
			return TF_WEAPON_SYRINGEGUN_MEDIC;
	}
}
#endif
