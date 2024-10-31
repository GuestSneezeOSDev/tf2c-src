//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#ifndef ENTITY_HEALTHKIT_H
#define ENTITY_HEALTHKIT_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "tf_gamerules.h"

//=============================================================================
//
// CTF HealthKit class.
//

class CHealthKit : public CTFPowerup, public TAutoList<CHealthKit>
{
	DECLARE_CLASS( CHealthKit, CTFPowerup );

public:
	void Spawn( void );
	void Precache( void );
	bool MyTouch( CBasePlayer *pPlayer );

	powerupsize_t GetPowerupSize( void ) { return POWERUP_FULL; }
	virtual const char *GetDefaultPowerupModel( void )
	{ 
		if ( TFGameRules()->IsInMedievalMode() )
			return "models/props_medieval/medieval_meat_large.mdl";
		
		if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
			return "models/props_halloween/halloween_medkit_large.mdl";

		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/medkit_large_bday.mdl";
		
		return "models/items/medkit_large.mdl";
	}

};

class CHealthKitSmall : public CHealthKit
{
	DECLARE_CLASS( CHealthKitSmall, CHealthKit );

public:
	powerupsize_t GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules()->IsInArenaMode() )
		{
			if ( GetTeamNumber() != TEAM_UNASSIGNED )
			{
				return "models/items/team_medkit_small.mdl";
			}
		}

		if ( TFGameRules()->IsInMedievalMode() )
			return "models/props_medieval/medieval_meat_small.mdl";
		
		if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
			return "models/props_halloween/halloween_medkit_small.mdl";

		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/medkit_small_bday.mdl";

		return "models/items/medkit_small.mdl";
	}

};

class CHealthKitMedium : public CHealthKit
{
	DECLARE_CLASS( CHealthKitMedium, CHealthKit );

public:
	powerupsize_t GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules()->IsInMedievalMode() )
			return "models/props_medieval/medieval_meat.mdl";
		
		if ( TFGameRules()->IsHolidayActive( TF_HOLIDAY_HALLOWEEN ) )
			return "models/props_halloween/halloween_medkit_medium.mdl";

		if ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) )
			return "models/items/medkit_medium_bday.mdl";

		return "models/items/medkit_medium.mdl";
	}

};
#endif // ENTITY_HEALTHKIT_H
