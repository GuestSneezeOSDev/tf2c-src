//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"

#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_engy_build.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_BUILD_FACTORY( CEngyDestroyMenuItem );


//======================================

DECLARE_HUDELEMENT_DEPTH( CHudMenuEngyDestroy, 40 );	// in front of engy building status


CHudMenuEngyDestroy::CHudMenuEngyDestroy( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuEngyDestroy" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<8; i++ )
	{
		char buf[32];

		V_sprintf_safe( buf, "active_item_%d", i+1 );
		m_pActiveItems[i] = new CEngyDestroyMenuItem( this, buf );

		V_sprintf_safe( buf, "inactive_item_%d", i+1 );
		m_pInactiveItems[i] = new CEngyDestroyMenuItem( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: called whenever a new level's starting
//-----------------------------------------------------------------------------
void CHudMenuEngyDestroy::LevelInit( void )
{
	//RecalculateBuildingItemState( ALL_BUILDINGS );

	CHudElement::LevelInit();
}


void CHudMenuEngyDestroy::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/destroy_menu/HudMenuEngyDestroy.res" );

	m_pActiveItems[0]->LoadControlSettings( "resource/UI/destroy_menu/sentry_active.res" );
	m_pActiveItems[1]->LoadControlSettings( "resource/UI/destroy_menu/dispenser_active.res" );
	m_pActiveItems[2]->LoadControlSettings( "resource/UI/destroy_menu/tele_entrance_active.res" );
	m_pActiveItems[3]->LoadControlSettings( "resource/UI/destroy_menu/tele_exit_active.res" );
	m_pActiveItems[4]->LoadControlSettings( "resource/UI/destroy_menu/jump_a_active.res" );
	m_pActiveItems[5]->LoadControlSettings( "resource/UI/destroy_menu/jump_b_active.res" );
	m_pActiveItems[6]->LoadControlSettings("resource/UI/destroy_menu/minidispenser_active.res");
	m_pActiveItems[7]->LoadControlSettings("resource/UI/destroy_menu/flamesentry_active.res");

	m_pInactiveItems[0]->LoadControlSettings( "resource/UI/destroy_menu/sentry_inactive.res" );
	m_pInactiveItems[1]->LoadControlSettings( "resource/UI/destroy_menu/dispenser_inactive.res" );
	m_pInactiveItems[2]->LoadControlSettings( "resource/UI/destroy_menu/tele_entrance_inactive.res" );
	m_pInactiveItems[3]->LoadControlSettings( "resource/UI/destroy_menu/tele_exit_inactive.res" );
	m_pInactiveItems[4]->LoadControlSettings( "resource/UI/destroy_menu/jump_a_inactive.res" );
	m_pInactiveItems[5]->LoadControlSettings( "resource/UI/destroy_menu/jump_b_inactive.res" );
	m_pInactiveItems[6]->LoadControlSettings("resource/UI/destroy_menu/minidispenser_inactive.res");
	m_pInactiveItems[7]->LoadControlSettings("resource/UI/destroy_menu/flamesentry_inactive.res");

	BaseClass::ApplySchemeSettings( pScheme );
}


bool CHudMenuEngyDestroy::ShouldDraw( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
		return false;

	// Don't show the menu for first person spectator
	if ( pPlayer != pWpn->GetOwner() )
		return false;

	if ( !CHudElement::ShouldDraw() )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_DESTROY );
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuEngyDestroy::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 1;

	bool bHandled = false;

	int iSlot = -1;

	switch( keynum )
	{
	case KEY_1:
	case KEY_PAD_1:
	case KEY_XBUTTON_UP:
		iSlot = 0;
		bHandled = true;
		break;
	case KEY_2:
	case KEY_PAD_2:
	case KEY_XBUTTON_RIGHT:
		iSlot = 1;
		bHandled = true;
		break;
	case KEY_3:
	case KEY_PAD_3:
	case KEY_XBUTTON_DOWN:
		iSlot = 2;
		bHandled = true;
		break;
	case KEY_4:
	case KEY_PAD_4:
	case KEY_XBUTTON_LEFT:
		iSlot = 3;
		bHandled = true;
		break;

	case KEY_5:
	case KEY_PAD_5:
	case KEY_6:
	case KEY_PAD_6:
	case KEY_7:
	case KEY_PAD_7:
	case KEY_8:
	case KEY_PAD_8:
	case KEY_9:
	case KEY_PAD_9:
		// Eat these keys
		bHandled = true;
		break;

	case KEY_0:
	case KEY_PAD_0:
	case KEY_XBUTTON_B:
		engine->ExecuteClientCmd( "lastinv" );
		bHandled = true;
		break;

	default:
		break;
	}

	if ( iSlot >= 0 )
	{
		int iBuildingID = 0;
		int iMode = 0;

		GetBuildingIDAndModeFromSlot( iSlot + 1, iBuildingID, iMode );

		if ( pLocalPlayer->GetObjectOfType( iBuildingID, iMode ) != NULL )
		{
			char szCmd[128];
			V_sprintf_safe( szCmd, "destroy %d %d; lastinv", iBuildingID, iMode );
			engine->ExecuteClientCmd( szCmd );
		}
		else
		{
			ErrorSound();
			engine->ExecuteClientCmd( "pda_click" );
		}
	}

	// return 0 if we ate the key
	return ( bHandled == false );
}

void CHudMenuEngyDestroy::ErrorSound( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer )
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}


void CHudMenuEngyDestroy::GetBuildingIDAndModeFromSlot(int iSlot, int &iBuildingID, int &iObjectMode)
{
	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer( );
	if ( !pLocalPlayer )
		return;

	bool bHasJumpPads = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, bHasJumpPads, set_teleporter_mode );

	bool bHasMiniDispenser = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, bHasMiniDispenser, set_dispenser_mode);

	int iHasFlameSentry = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasFlameSentry, set_sentry_mode);


	switch( iSlot )
	{
	case 1:
		iBuildingID = iHasFlameSentry ? OBJ_FLAMESENTRY : OBJ_SENTRYGUN;
		break;
	case 2:
		iBuildingID = bHasMiniDispenser ? OBJ_MINIDISPENSER : OBJ_DISPENSER;
		break;
	case 3:
		iBuildingID = bHasJumpPads ? OBJ_JUMPPAD : OBJ_TELEPORTER;
		iObjectMode = TELEPORTER_TYPE_ENTRANCE;
		break;
	case 4:
		iBuildingID = bHasJumpPads ? OBJ_JUMPPAD : OBJ_TELEPORTER;
		iObjectMode = TELEPORTER_TYPE_EXIT;
		break;

	default:
		Assert( !"What slot are we asking for and why?" );
		break;
	}
}

void CHudMenuEngyDestroy::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return;
	
	int iHasJumpPads = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasJumpPads, set_teleporter_mode);

	int iHasMiniDispenser = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasMiniDispenser, set_dispenser_mode);

	int iHasFlameSentry = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasFlameSentry, set_sentry_mode);
	
	if (iHasFlameSentry)
	{
		UpdatePanel(7, pLocalPlayer);
		SetPanelVisibility(0, false);
	}
	else
	{
		UpdatePanel(0, pLocalPlayer);
		SetPanelVisibility(7, false);
	}

	if (iHasMiniDispenser)
	{
		UpdatePanel(6, pLocalPlayer);
		SetPanelVisibility(1, false);
	}
	else
	{
		UpdatePanel(1, pLocalPlayer);
		SetPanelVisibility(6, false);
	}

	if (iHasJumpPads)
	{
		UpdatePanel(4, pLocalPlayer);
		UpdatePanel(5, pLocalPlayer);
		SetPanelVisibility(2, false);
		SetPanelVisibility(3, false);
	}
	else
	{
		UpdatePanel(2, pLocalPlayer);
		UpdatePanel(3, pLocalPlayer);
		SetPanelVisibility(4, false);
		SetPanelVisibility(5, false);
	}
}



// Assume that pLocalPlayer is valid
void CHudMenuEngyDestroy::UpdatePanel(int iPanel, C_TFPlayer* pLocalPlayer)
{
	// Update this slot.
	// If the building is already built.

	C_BaseObject *pObject = nullptr;

	// Probably should be it's own function
	switch (iPanel)
	{
	case 0:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_SENTRYGUN);
		break;
	case 1:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_DISPENSER);
		break;
	case 2:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE);
		break;
	case 3:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT);
		break;
	case 4:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_JUMPPAD, JUMPPAD_TYPE_A);
		break;
	case 5:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_JUMPPAD, JUMPPAD_TYPE_B);
		break;
	case 6:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_MINIDISPENSER);
		break;
	case 7:
		pObject = pLocalPlayer->GetObjectOfType(OBJ_FLAMESENTRY);
		break;
	}

	// If the building is built, we can destroy it
	if (pObject != NULL && !pObject->IsPlacing())
	{
		m_pActiveItems[iPanel]->SetVisible(true);
		m_pInactiveItems[iPanel]->SetVisible(false);
	}
	else
	{
		m_pActiveItems[iPanel]->SetVisible(false);
		m_pInactiveItems[iPanel]->SetVisible(true);
	}
}

void CHudMenuEngyDestroy::SetPanelVisibility(int iPanel, bool bVisibility)
{
	m_pActiveItems[iPanel]->SetVisible(bVisibility);
	m_pInactiveItems[iPanel]->SetVisible(bVisibility);
}

void CHudMenuEngyDestroy::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	if ( state == true )
	{
		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		//RecalculateBuildingState( ALL_BUILDINGS );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}
