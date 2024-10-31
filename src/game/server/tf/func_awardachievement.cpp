//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Award Achievement Trigger.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "func_awardachievement.h"

LINK_ENTITY_TO_CLASS( func_awardachievement, CAwardAchievement );

//=============================================================================
//
// CTF Award Achievement Trigger functions.
//

BEGIN_DATADESC( CAwardAchievement )
	DEFINE_KEYFIELD( m_iAchievementID, FIELD_INTEGER, "achievement_id" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CAwardAchievement::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL ); // so we can keep track of who is touching us
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching this zone
//-----------------------------------------------------------------------------
void CAwardAchievement::StartTouch( CBaseEntity *pEntity )
{
	BaseClass::StartTouch( pEntity );

	if (m_iAchievementID <= TF2C_ACHIEVEMENT_START || m_iAchievementID >= TF2C_ACHIEVEMENT_COUNT)
		return;

	CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
	if ( !pAchievementMgr )
		return;

	IAchievement *pAchievement = pAchievementMgr->GetAchievementByID( GetAchievementID() );
	if( !pAchievement )
		return;

	CTFPlayer* pTFPlayer = ToTFPlayer(pEntity);
	if( !pTFPlayer )
		return;

	pTFPlayer->AwardAchievement( GetAchievementID() );
}