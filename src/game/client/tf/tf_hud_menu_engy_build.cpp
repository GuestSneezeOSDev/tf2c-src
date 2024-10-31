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

#include "tf_hud_menu_engy_build.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Set to 1 to simulate xbox-style menu interaction
ConVar tf_build_menu_controller_mode( "tf_build_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller build menus. 1 = ON, 0 = OFF." );

//======================================

DECLARE_HUDELEMENT_DEPTH( CHudMenuEngyBuild, 40 );	// in front of engy building status


CHudMenuEngyBuild::CHudMenuEngyBuild( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuEngyBuild" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<8; i++ )
	{
		char buf[32];

		V_sprintf_safe( buf, "active_item_%d", i+1 );
		m_pAvailableObjects[i] = new EditablePanel( this, buf );

		V_sprintf_safe( buf, "already_built_item_%d", i+1 );
		m_pAlreadyBuiltObjects[i] = new EditablePanel( this, buf );

		V_sprintf_safe( buf, "cant_afford_item_%d", i+1 );
		m_pCantAffordObjects[i] = new EditablePanel( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pActiveSelection = NULL;

	m_iSelectedItem = -1;

	m_pBuildLabelBright = NULL;
	m_pBuildLabelDim = NULL;

	m_pDestroyLabelBright = NULL;
	m_pDestroyLabelDim = NULL;

	m_bInConsoleMode = false;

	RegisterForRenderGroup( "mid" );
}


void CHudMenuEngyBuild::ApplySchemeSettings( IScheme *pScheme )
{
	bool b360Style = ( IsConsole() || tf_build_menu_controller_mode.GetBool() );
	if ( b360Style )
	{
		LoadControlSettings( "resource/UI/build_menu_360/HudMenuEngyBuild.res" );

		// Load the already built images, destroyable
		m_pAlreadyBuiltObjects[0]->LoadControlSettings( "resource/UI/build_menu_360/sentry_already_built.res" );
		m_pAlreadyBuiltObjects[1]->LoadControlSettings( "resource/UI/build_menu_360/dispenser_already_built.res" );
		m_pAlreadyBuiltObjects[2]->LoadControlSettings( "resource/UI/build_menu_360/tele_entrance_already_built.res" );
		m_pAlreadyBuiltObjects[3]->LoadControlSettings( "resource/UI/build_menu_360/tele_exit_already_built.res" );
		m_pAlreadyBuiltObjects[4]->LoadControlSettings( "resource/UI/build_menu_360/jump_a_already_built.res" );
		m_pAlreadyBuiltObjects[5]->LoadControlSettings( "resource/UI/build_menu_360/jump_b_already_built.res" );
		m_pAlreadyBuiltObjects[6]->LoadControlSettings("resource/UI/build_menu_360/minidispenser_already_built.res");
		m_pAlreadyBuiltObjects[7]->LoadControlSettings("resource/UI/build_menu_360/flamesentry_already_built.res");

		m_pAvailableObjects[0]->LoadControlSettings( "resource/UI/build_menu_360/sentry_active.res" );
		m_pAvailableObjects[1]->LoadControlSettings( "resource/UI/build_menu_360/dispenser_active.res" );
		m_pAvailableObjects[2]->LoadControlSettings( "resource/UI/build_menu_360/tele_entrance_active.res" );
		m_pAvailableObjects[3]->LoadControlSettings( "resource/UI/build_menu_360/tele_exit_active.res" );
		m_pAvailableObjects[4]->LoadControlSettings( "resource/UI/build_menu_360/jump_a_active.res" );
		m_pAvailableObjects[5]->LoadControlSettings( "resource/UI/build_menu_360/jump_b_active.res" );
		m_pAvailableObjects[6]->LoadControlSettings("resource/UI/build_menu_360/minidispenser_active.res");
		m_pAvailableObjects[7]->LoadControlSettings("resource/UI/build_menu_360/flamesentry_active.res");

		m_pCantAffordObjects[0]->LoadControlSettings( "resource/UI/build_menu_360/sentry_cant_afford.res" );
		m_pCantAffordObjects[1]->LoadControlSettings( "resource/UI/build_menu_360/dispenser_cant_afford.res" );
		m_pCantAffordObjects[2]->LoadControlSettings( "resource/UI/build_menu_360/tele_entrance_cant_afford.res" );
		m_pCantAffordObjects[3]->LoadControlSettings( "resource/UI/build_menu_360/tele_exit_cant_afford.res" );
		m_pCantAffordObjects[4]->LoadControlSettings( "resource/UI/build_menu_360/jump_a_cant_afford.res" );
		m_pCantAffordObjects[5]->LoadControlSettings( "resource/UI/build_menu_360/jump_b_cant_afford.res" );
		m_pCantAffordObjects[6]->LoadControlSettings("resource/UI/build_menu_360/minidispenser_cant_afford.res");
		m_pCantAffordObjects[7]->LoadControlSettings("resource/UI/build_menu_360/flamesentry_cant_afford.res");

		m_pActiveSelection = dynamic_cast< EditablePanel *>( FindChildByName( "active_selection_bg" ) );

		m_pBuildLabelBright = dynamic_cast<CExLabel *>(FindChildByName( "BuildHintLabel_Bright" ) );
		m_pBuildLabelDim = dynamic_cast<CExLabel *>(FindChildByName( "BuildHintLabel_Dim" ) );
	
		m_pDestroyLabelBright = dynamic_cast<CExLabel *>( FindChildByName( "DestroyHintLabel_Bright" ) );
		m_pDestroyLabelDim = dynamic_cast<CExLabel *>( FindChildByName( "DestroyHintLabel_Dim" ) );

		// Reposition the activeselection to the default position
		m_iSelectedItem = -1;	// force reposition
		SetSelectedItem( 1 );
	}
	else
	{
		LoadControlSettings( "resource/UI/build_menu/HudMenuEngyBuild.res" );

		// Load the already built images, not destroyable
		m_pAlreadyBuiltObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_already_built.res" );
		m_pAlreadyBuiltObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_already_built.res" );
		m_pAlreadyBuiltObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_already_built.res" );
		m_pAlreadyBuiltObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_already_built.res" );
		m_pAlreadyBuiltObjects[4]->LoadControlSettings( "resource/UI/build_menu/jump_a_already_built.res" );
		m_pAlreadyBuiltObjects[5]->LoadControlSettings( "resource/UI/build_menu/jump_b_already_built.res" );
		m_pAlreadyBuiltObjects[6]->LoadControlSettings("resource/UI/build_menu/minidispenser_already_built.res");
		m_pAlreadyBuiltObjects[7]->LoadControlSettings("resource/UI/build_menu/flamesentry_already_built.res");

		m_pAvailableObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_active.res" );
		m_pAvailableObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_active.res" );
		m_pAvailableObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_active.res" );
		m_pAvailableObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_active.res" );
		m_pAvailableObjects[4]->LoadControlSettings( "resource/UI/build_menu/jump_a_active.res" );
		m_pAvailableObjects[5]->LoadControlSettings( "resource/UI/build_menu/jump_b_active.res" );
		m_pAvailableObjects[6]->LoadControlSettings("resource/UI/build_menu/minidispenser_active.res");
		m_pAvailableObjects[7]->LoadControlSettings("resource/UI/build_menu/flamesentry_active.res");

		m_pCantAffordObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_cant_afford.res" );
		m_pCantAffordObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_cant_afford.res" );
		m_pCantAffordObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_cant_afford.res" );
		m_pCantAffordObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_cant_afford.res" );
		m_pCantAffordObjects[4]->LoadControlSettings( "resource/UI/build_menu/jump_a_cant_afford.res" );
		m_pCantAffordObjects[5]->LoadControlSettings( "resource/UI/build_menu/jump_b_cant_afford.res" );
		m_pCantAffordObjects[6]->LoadControlSettings("resource/UI/build_menu/minidispenser_cant_afford.res");
		m_pCantAffordObjects[7]->LoadControlSettings("resource/UI/build_menu/flamesentry_cant_afford.res");

		m_pActiveSelection = NULL;

		m_pBuildLabelBright = NULL;
		m_pBuildLabelDim = NULL;

		m_pDestroyLabelBright = NULL;
		m_pDestroyLabelDim = NULL;
	}

	// Set the cost label.
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	CalculateCostOnAllPanels(pLocalPlayer);

	BaseClass::ApplySchemeSettings( pScheme );
}


bool CHudMenuEngyBuild::ShouldDraw( void )
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

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD );
}


void CHudMenuEngyBuild::GetBuildingIDAndModeFromSlot( int iSlot, int &iBuildingID, int &iObjectMode )
{
	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer( );
	if ( !pLocalPlayer )
		return;

	int iHasJumpPads = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, iHasJumpPads, set_teleporter_mode );

	int iHasMiniDispenser = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasMiniDispenser, set_dispenser_mode);

	int iHasFlameSentry = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasFlameSentry, set_sentry_mode);

	switch ( iSlot )
	{
		case 1:
			iBuildingID = iHasFlameSentry ? OBJ_FLAMESENTRY : OBJ_SENTRYGUN;
			break;
		case 2:
			iBuildingID = iHasMiniDispenser ? OBJ_MINIDISPENSER : OBJ_DISPENSER;
			break;
		case 3:
			if (iHasJumpPads)
			{
				iBuildingID = OBJ_JUMPPAD;
				iObjectMode = JUMPPAD_TYPE_A;
			}
			else
			{
				iBuildingID = OBJ_TELEPORTER;
				iObjectMode = TELEPORTER_TYPE_ENTRANCE;
			}
			break;
		case 4:
			if (iHasJumpPads)
			{
				iBuildingID = OBJ_JUMPPAD;
				iObjectMode = JUMPPAD_TYPE_B;
			}
			else
			{
				iBuildingID = OBJ_TELEPORTER;
				iObjectMode = TELEPORTER_TYPE_EXIT;
			}
			break;

		default:
			Assert( !"What slot are we asking for and why?" );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuEngyBuild::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
		return 1;

	if ( !down )
		return 1;

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );
	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
			// jump to last
			iNewSelection = 4;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > 4 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
			// move selection to the left
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 4;
			break;

		case KEY_XBUTTON_A:
		case KEY_XBUTTON_RTRIGGER:
			// build selected item
			SendBuildMessage( m_iSelectedItem );
			return 0;

		case KEY_XBUTTON_Y:
		case KEY_XBUTTON_LTRIGGER:
			{
				// destroy selected item
				bool bSuccess = SendDestroyMessage( m_iSelectedItem );

				if ( bSuccess )
				{
					engine->ExecuteClientCmd( "lastinv" );
				}
			}
			return 0;

		case KEY_XBUTTON_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( iNewSelection );

		return 0;
	}
	else
	{
		int iSlot = 0;

		switch( keynum )
		{
		case KEY_1:
		case KEY_PAD_1:
			iSlot = 1;
			break;
		case KEY_2:
		case KEY_PAD_2:
			iSlot = 2;
			break;
		case KEY_3:
		case KEY_PAD_3:
			iSlot = 3;
			break;
		case KEY_4:
		case KEY_PAD_4:
			iSlot = 4;
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
			return 0;

		case KEY_0:
		case KEY_PAD_0:
		case KEY_XBUTTON_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		if ( iSlot > 0 )
		{
			SendBuildMessage( iSlot );
			return 0;
		}
	}

	return 1;	// key not handled
}

void CHudMenuEngyBuild::SendBuildMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	int iBuilding = 0, iMode = 0;
	GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode );
	
	C_BaseObject *pObject = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
	if ( ( pObject && pObject->InRemoteConstruction() ) || ( !pObject && pLocalPlayer->GetAmmoCount( TF_AMMO_METAL ) >= CTFPlayerShared::CalculateObjectCost( pLocalPlayer, iBuilding ) ) )
	{
		char szCmd[128];
		V_sprintf_safe( szCmd, "build %d %d", iBuilding, iMode );
		engine->ClientCmd( szCmd );
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
		engine->ExecuteClientCmd( "pda_click" );
	}
}

bool CHudMenuEngyBuild::SendDestroyMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	bool bSuccess = false;

	int iBuilding = 0, iMode = 0;
	GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode );

	C_BaseObject *pObject = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
	if ( pObject )
	{
		char szCmd[128];
		V_sprintf_safe( szCmd, "destroy %d %d", iBuilding, iMode );
		engine->ClientCmd( szCmd );
		bSuccess = true; 
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	return bSuccess;
}

// needlessly hot function. fix this.
// -sappho
void CHudMenuEngyBuild::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if (!ShouldDraw())
	{
		return;
	}

	int iAccount = pLocalPlayer->GetAmmoCount( TF_AMMO_METAL );

	int iHasJumpPads = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasJumpPads, set_teleporter_mode);

	int iHasMiniDispenser = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasMiniDispenser, set_dispenser_mode);

	int iHasFlameSentry = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, iHasFlameSentry, set_sentry_mode);


	if (iHasFlameSentry)
	{
		UpdatePanel(7, pLocalPlayer, iAccount);
		SetPanelVisibility(0, false);
	}
	else
	{
		UpdatePanel(0, pLocalPlayer, iAccount);
		SetPanelVisibility(7, false);
	}

	if (iHasMiniDispenser)
	{
		UpdatePanel(6, pLocalPlayer, iAccount);
		SetPanelVisibility(1, false);
	}
	else
	{
		UpdatePanel(1, pLocalPlayer, iAccount);
		SetPanelVisibility(6, false);
	}

	if (iHasJumpPads)
	{
		UpdatePanel(4, pLocalPlayer, iAccount);
		UpdatePanel(5, pLocalPlayer, iAccount);
		SetPanelVisibility(2, false);
		SetPanelVisibility(3, false);
	}
	else
	{
		UpdatePanel(2, pLocalPlayer, iAccount);
		UpdatePanel(3, pLocalPlayer, iAccount);
		SetPanelVisibility(4, false);
		SetPanelVisibility(5, false);
	}
}

// Assume that pLocalPlayer is valid
void CHudMenuEngyBuild::UpdatePanel(int iPanel, C_TFPlayer* pLocalPlayer, int iAccount)
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

	if (pObject && !pObject->IsPlacing() && !pObject->InRemoteConstruction())
	{
		m_pAlreadyBuiltObjects[iPanel]->SetVisible(true);
		m_pAvailableObjects[iPanel]->SetVisible(false);
		m_pCantAffordObjects[iPanel]->SetVisible(false);
	}
	// See if we can afford it.
	else if (iAccount < GetPanelCost(iPanel, pLocalPlayer) && !(pObject && pObject->InRemoteConstruction()))
	{
		m_pCantAffordObjects[iPanel]->SetVisible(true);
		m_pAvailableObjects[iPanel]->SetVisible(false);
		m_pAlreadyBuiltObjects[iPanel]->SetVisible(false);
	}
	// We can buy it!
	else
	{
		m_pAvailableObjects[iPanel]->SetVisible(true);
		m_pAlreadyBuiltObjects[iPanel]->SetVisible(false);
		m_pCantAffordObjects[iPanel]->SetVisible(false);
	}
}

void CHudMenuEngyBuild::SetPanelVisibility(int iPanel, bool bVisibility)
{
	m_pAvailableObjects[iPanel]->SetVisible(bVisibility);
	m_pAlreadyBuiltObjects[iPanel]->SetVisible(bVisibility);
	m_pCantAffordObjects[iPanel]->SetVisible(bVisibility);
}

void CHudMenuEngyBuild::SetVisible( bool state )
{
	if ( state )
	{
		// Close the weapon selection menu.
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_build_menu_controller_mode.GetBool() );
		if ( bConsoleMode != m_bInConsoleMode )
		{
			InvalidateLayout( true, true );
			m_bInConsoleMode = bConsoleMode;
		}

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		// Set selection to the first available building that we can build.
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		int iDefaultSlot = 1;

		// Find the first slot that represents a building that we haven't built.
		int iSlot;
		for ( iSlot = 1; iSlot <= 4; iSlot++ )
		{
			int iBuilding = 0, iMode = 0;
			GetBuildingIDAndModeFromSlot( iSlot, iBuilding, iMode );
			C_BaseObject *pObject = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
			if ( !pObject )
			{
				iDefaultSlot = iSlot;
				break;
			}
		}

		m_iSelectedItem = -1; // Force redo.
		SetSelectedItem( iDefaultSlot );

		HideLowerPriorityHudElementsInGroup( "mid" );

		// Set the cost label.
		CalculateCostOnAllPanels(pLocalPlayer);
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

void CHudMenuEngyBuild::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// Move the selection item to the new position.
		if ( m_pActiveSelection )
		{
			// Move the selection background.
			int x, y;
			m_pAlreadyBuiltObjects[m_iSelectedItem - 1]->GetPos( x, y );

			x -= XRES( 4 );
			y -= XRES( 4 );

			m_pActiveSelection->SetPos( x, y );

			UpdateHintLabels();			
		}
	}
}

// We trust that we have got a valid pLocalPlayer pointer
void CHudMenuEngyBuild::CalculateCostOnAllPanels(C_TFPlayer* pLocalPlayer)
{
	for (int i = 0; i < 8; ++i)
		SetCostOnPanel(i, GetPanelCost(i, pLocalPlayer));
}

void CHudMenuEngyBuild::SetCostOnPanel(int iPanel, int iCost)
{
	m_pAvailableObjects[iPanel]->SetDialogVariable("metal", iCost);
	m_pAlreadyBuiltObjects[iPanel]->SetDialogVariable("metal", iCost);
	m_pCantAffordObjects[iPanel]->SetDialogVariable("metal", iCost);
}

int CHudMenuEngyBuild::GetPanelCost(int iPanel, C_TFPlayer* pLocalPlayer)
{
	int iObjectType = NULL;
	switch (iPanel)
	{
	case 0:
		iObjectType = OBJ_SENTRYGUN;
		break;
	case 1:
		iObjectType = OBJ_DISPENSER;
		break;
	case 2:
	case 3:
		iObjectType = OBJ_TELEPORTER;
		break;
	case 4:
	case 5:
		iObjectType = OBJ_JUMPPAD;
		break;
	case 6:
		iObjectType = OBJ_MINIDISPENSER;
		break;
	case 7:
		iObjectType = OBJ_FLAMESENTRY;
		break;
	}

	return CTFPlayerShared::CalculateObjectCost(pLocalPlayer, iObjectType);
}

void CHudMenuEngyBuild::UpdateHintLabels( void )
{
	// Highlight the action we can perform (build or destroy or neither).
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		bool bDestroyLabelBright = false;
		bool bBuildLabelBright = false;

		int iBuilding = 0, iMode = 0;
		GetBuildingIDAndModeFromSlot( m_iSelectedItem, iBuilding, iMode );

		C_BaseObject *pObject = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
		if ( pObject )
		{
			// Highlight destroy, we have a building.
			bDestroyLabelBright = true;
		}
		else if ( pLocalPlayer->GetAmmoCount( TF_AMMO_METAL ) >= CTFPlayerShared::CalculateObjectCost( pLocalPlayer, iBuilding ) ) // I can afford it!
		{
			// Highlight build, we can build this.
			bBuildLabelBright = true;
		}

		if ( m_pDestroyLabelBright && m_pDestroyLabelDim && m_pBuildLabelBright && m_pBuildLabelDim )
		{
			m_pDestroyLabelBright->SetVisible( bDestroyLabelBright );
			m_pDestroyLabelDim->SetVisible( !bDestroyLabelBright );

			m_pBuildLabelBright->SetVisible( bBuildLabelBright );
			m_pBuildLabelDim->SetVisible( !bBuildLabelBright );
		}
	}
}
