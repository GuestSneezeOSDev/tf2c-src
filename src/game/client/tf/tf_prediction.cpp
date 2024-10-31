//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "prediction.h"
#include "c_baseplayer.h"
#include "igamemovement.h"
#include "c_tf_player.h"
#include "in_buttons.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;


class CTFPrediction : public CPrediction
{
DECLARE_CLASS( CTFPrediction, CPrediction );

public:
	virtual void	SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};


void CTFPrediction::SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// Call the default SetupMove code.
	BaseClass::SetupMove( player, ucmd, pHelper, move );
}


void CTFPrediction::FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );

	CTFPlayer *pTFPlayer = ToTFPlayer( player );

	if ( pTFPlayer->m_Shared.IsMovementLocked() )
	{
		// Update eye angles, too, since TauntMove rotates them.
		pTFPlayer->SetEyeAngles( move->m_vecViewAngles );
	}
}


// Expose interface to engine
// Expose interface to engine
static CTFPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );

CPrediction *prediction = &g_Prediction;

