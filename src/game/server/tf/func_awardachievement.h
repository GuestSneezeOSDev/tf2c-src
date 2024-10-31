//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Achievement Zone.
//
//=============================================================================//
#ifndef FUNC_ACHIEVEMENT_ZONE_H
#define FUNC_ACHIEVEMENT_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "achievements_tf.h"
#include "achievementmgr.h"
#include "baseachievement.h"

//=============================================================================
//
// CTF Award Achievement Trigger class.
//
class CAwardAchievement : public CBaseTrigger
{
	DECLARE_CLASS( CAwardAchievement, CBaseTrigger );

public:
	DECLARE_DATADESC();
	CAwardAchievement()
	{
		m_iAchievementID = -1;
	}

	void	Spawn( void );
	// Return true if the specified entity is touching this zone
	void	StartTouch( CBaseEntity *pOther );
	int		GetAchievementID( void ){ return m_iAchievementID; }

private:
	int		m_iAchievementID;
};

#endif // FUNC_ACHIEVEMENT_ZONE_H












