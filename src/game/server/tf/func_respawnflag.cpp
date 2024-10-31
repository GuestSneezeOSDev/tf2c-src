//======= Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Respawnflag.
//
//=============================================================================//

#include "cbase.h"
#include "tf_player.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "func_respawnflag.h"

LINK_ENTITY_TO_CLASS( func_respawnflag, CFuncRespawnFlagZone );

//=============================================================================
//
// CTF Respawnflag tables.
//

BEGIN_DATADESC(CFuncRespawnFlagZone)
	DEFINE_FUNCTION(Touch),
END_DATADESC();

//=============================================================================
//
// CTF Respawnflag functions.
//


bool PointInRespawnFlagZone( const Vector &vecFlagOrigin )
{
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname(pEntity, "func_respawnflag")) != NULL )
	{
		CFuncRespawnFlagZone *v3 = (CFuncRespawnFlagZone *)pEntity;

		if ( !v3->IsDisabled() && v3->PointIsWithin(vecFlagOrigin) )
		{
			return true;	
		}
	}
	return false; 
}


CFuncRespawnFlagZone::CFuncRespawnFlagZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Spawn(void)
{
	Precache();
	InitTrigger();
	SetTouch( &CFuncRespawnFlagZone::Touch );
}


void CFuncRespawnFlagZone::Activate(void)
{
	BaseClass::Activate();
}


void CFuncRespawnFlagZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer )
		{
			CCaptureFlag *pFlag = pPlayer->GetTheFlag();
			if ( pFlag )
			{
				pPlayer->DropFlag();
				pFlag->Reset();
				pFlag->ResetMessage();
			}
		}
	}
}


void CFuncRespawnFlagZone::InputEnable(inputdata_t &inputdata)
{
	SetDisabled(false);
}


void CFuncRespawnFlagZone::InputDisable(inputdata_t &inputdata)
{
	SetDisabled(true);
}


bool CFuncRespawnFlagZone::IsDisabled(void)
{
	return m_bDisabled;
}


void CFuncRespawnFlagZone::InputToggle(inputdata_t &inputdata)
{
	if (m_bDisabled)
	{
		SetDisabled(false);
	}
	else
	{
		SetDisabled(true);
	}
}


void CFuncRespawnFlagZone::SetDisabled(bool bDisabled)
{
	m_bDisabled = bDisabled;
}