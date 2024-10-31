//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef ENTITY_AMMOPACK_H
#define ENTITY_AMMOPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "tf_gamerules.h"

#define TF_AMMOPACK_PICKUP_SOUND	"AmmoPack.Touch"

//=============================================================================
//
// CTF AmmoPack class.
//

class CAmmoPack : public CTFPowerup, public TAutoList<CAmmoPack>
{
	DECLARE_CLASS( CAmmoPack, CTFPowerup );

public:
	void Spawn( void );
	void Precache( void );
	bool MyTouch( CBasePlayer *pPlayer );

	powerupsize_t GetPowerupSize( void ) { return POWERUP_FULL; }
	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/ammopack_large_bday.mdl";

		return "models/items/ammopack_large.mdl";
	}

};

class CAmmoPackSmall : public CAmmoPack
{
	DECLARE_CLASS( CAmmoPackSmall, CAmmoPack );

public:
	powerupsize_t GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/ammopack_small_bday.mdl";

		return "models/items/ammopack_small.mdl";
	}

};

class CAmmoPackMedium : public CAmmoPack
{
	DECLARE_CLASS( CAmmoPackMedium, CAmmoPack );

public:
	powerupsize_t GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/ammopack_medium_bday.mdl";

		return "models/items/ammopack_medium.mdl";
	}

};
#endif // ENTITY_AMMOPACK_H
