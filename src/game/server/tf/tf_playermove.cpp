//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"
#include "tf_player.h"
#include "iservervehicle.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;


//-----------------------------------------------------------------------------
// Sets up the move data for TF2
//-----------------------------------------------------------------------------
class CTFPlayerMove : public CPlayerMove
{
DECLARE_CLASS( CTFPlayerMove, CPlayerMove );

public:
	virtual void	StartCommand( CBasePlayer *player, CUserCmd *cmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

// PlayerMove Interface
static CTFPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

//-----------------------------------------------------------------------------
// Main setup, finish
//-----------------------------------------------------------------------------

void CTFPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	BaseClass::StartCommand( player, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: This is called pre player movement and copies all the data necessary
//          from the player for movement. (Server-side, the client-side version
//          of this code can be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	BaseClass::SetupMove( player, ucmd, pHelper, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->SetupMove( player, ucmd, pHelper, move );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This is called post player movement to copy back all data that
//          movement could have modified and that is necessary for future
//          movement. (Server-side, the client-side version of this code can 
//          be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// No pitch or roll allowed.
	move->m_vecAngles[PITCH] = move->m_vecAngles[ROLL] = 0.0f;

	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->FinishMove( player, ucmd, move );
	}

	CTFPlayer *pTFPlayer = ToTFPlayer( player );

	if ( pTFPlayer->m_Shared.IsMovementLocked() )
	{
		// Update eye angles, too, since TauntMove rotates them.
		pTFPlayer->SetEyeAngles( move->m_vecViewAngles );
	}

	// Calling this here since it updates player angles.
	pTFPlayer->UpdateAnimState();
}
