//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_flag.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFFlag, tf_weapon_flag );

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlag::CTFFlag()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlag::CanHolster( void ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer && pPlayer->GetTheFlag() && !m_bDropping )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlag::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	m_bDropping = true;
	pPlayer->SwitchToNextBestWeapon( NULL );

#ifdef GAME_DLL
	pPlayer->DropFlag();
#endif
	m_bDropping = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFlag::GetViewModel( int iViewModel /*= 0*/ ) const
{
	// Get the skin of the carried flag.
	CCaptureFlag *pFlag = GetOwnerFlag();
	if ( pFlag )
	{
		const char *pszModel = pFlag->GetViewModel();
		if ( pszModel[0] )
		{
			return DetermineViewModelType( pszModel );
		}
	}

	return BaseClass::GetViewModel( iViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureFlag *CTFFlag::GetOwnerFlag( void ) const
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return NULL;

	return pPlayer->GetTheFlag();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFFlag::GetSkin( void )
{
	// Get the skin of the carried flag.
	CCaptureFlag *pFlag = GetOwnerFlag();
	if ( !pFlag )
		return 0;

	switch ( pFlag->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		return 0;
	case TF_TEAM_BLUE:
		return 1;
	case TF_TEAM_GREEN:
		return 2;
	case TF_TEAM_YELLOW:
		return 3;
	default:
		return 4;
	}
}
#endif
