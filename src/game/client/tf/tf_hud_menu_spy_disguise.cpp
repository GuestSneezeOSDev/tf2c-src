//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_hud_menu_spy_disguise.h"
#include "hud.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT( CHudMenuSpyDisguise );


CHudMenuSpyDisguise::CHudMenuSpyDisguise( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuSpyDisguise" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
	{
		for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
		{
			const char *pszTeam = NULL;
			if( i == TF_DISGUISE_MENU_TEAMS - 1 )
				// For the global team just return red for now
				pszTeam = "global";
			else
				pszTeam = g_aTeamLowerNames[i + FIRST_GAME_TEAM];

			EditablePanel *pPanel = new EditablePanel( this, VarArgs( "class_item_%s_%d", pszTeam, j + 1 ) );
			m_ClassItems[i][j].pPanel = pPanel;
			m_ClassItems[i][j].pKeyIcon = new CIconPanel( pPanel, "NumberBg" );
			m_ClassItems[i][j].pNumber = new CExLabel( pPanel, "NumberLabel", "" );
			m_ClassItems[i][j].pNewNumber = new CExLabel( pPanel, "NewNumberLabel", "" );
		}
	}

	for ( int i = 0; i < TF_DISGUISE_MENU_CATEGORIES; i++ )
	{
		m_pKeyIcons[i] = new CIconPanel( this, VarArgs( "NumberBG%d", i + 1 ) );
		m_pNumbers[i] = new CExLabel( this, VarArgs( "NumberLabel%d", i + 1 ), "" );
	}

	m_pActiveSelection = NULL;

	m_iShowingTeam = 0;
	m_iSelectedItem = -1;
	m_bInConsoleMode = false;
	m_iSelectedCategory = -1;

	ListenForGameEvent( "spy_pda_reset" );

	InvalidateLayout( false, true );

	RegisterForRenderGroup( "mid" );
}

ConVar tf_disguise_menu_controller_mode( "tf_disguise_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller disguise menus. 1 = ON, 0 = OFF." );
ConVar tf_simple_disguise_menu( "tf_simple_disguise_menu", "0", FCVAR_ARCHIVE, "Use a more concise disguise selection menu." );

// menu classes are not in the same order as the defines
static int g_iRemapKeyToClass[TF_DISGUISE_MENU_CLASSES] =
{
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY
};


void CHudMenuSpyDisguise::ApplySchemeSettings( IScheme *pScheme )
{
	bool b360Style = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() );
	if ( b360Style )
	{
		// load control settings...
		LoadControlSettings( "resource/UI/disguise_menu_360/HudMenuSpyDisguise.res" );

		for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
		{
			for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
			{
				const char *pszClass = g_aPlayerClassNames_NonLocalized[g_iRemapKeyToClass[j]];

				const char *pszTeam = NULL;
				if( i == TF_DISGUISE_MENU_TEAMS - 1 )
					pszTeam = "global";
				else
					pszTeam = g_aTeamLowerNames[i + FIRST_GAME_TEAM];

				m_ClassItems[i][j].pPanel->LoadControlSettings( VarArgs( "resource/UI/disguise_menu_360/%s_%s.res", pszClass, pszTeam ) );
			}
		}

		m_pActiveSelection = dynamic_cast< EditablePanel * >( FindChildByName( "active_selection_bg" ) );

		// Reposition the activeselection to the default position
		m_iSelectedItem = -1;	// force reposition
		SetSelectedItem( 5 );
	}
	else
	{
		// load control settings...
		LoadControlSettings( "resource/UI/disguise_menu/HudMenuSpyDisguise.res" );

		for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
		{
			for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
			{
				const char *pszClass = g_aPlayerClassNames_NonLocalized[g_iRemapKeyToClass[j]];

				const char *pszTeam = NULL;
				if( i == TF_DISGUISE_MENU_TEAMS - 1 )
					pszTeam = "global";
				else
					pszTeam = g_aTeamLowerNames[i + FIRST_GAME_TEAM];

				m_ClassItems[i][j].pPanel->LoadControlSettings( VarArgs( "resource/UI/disguise_menu/%s_%s.res", pszClass, pszTeam ) );
			}
		}

		m_pActiveSelection = NULL;
	}

	ToggleSelectionIcons( false );

	BaseClass::ApplySchemeSettings( pScheme );
}


bool CHudMenuSpyDisguise::ShouldDraw( void )
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

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_SPY );
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuSpyDisguise::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
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
			iNewSelection = 9;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > 9 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
			// move selection to the right
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 9;
			break;

		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_A:
			{
				// select disguise
				int iClass = g_iRemapKeyToClass[m_iSelectedItem - 1];
				SelectDisguise( iClass, m_iShowingTeam );
			}
			return 0;

		case KEY_XBUTTON_Y:
			ToggleDisguiseTeam();
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
		switch( keynum )
		{
		case KEY_1:
		case KEY_PAD_1:
		case KEY_2:
		case KEY_PAD_2:
		case KEY_3:
		case KEY_PAD_3:
			if ( tf_simple_disguise_menu.GetBool() )
			{
				int iNum = (keynum >= KEY_PAD_0 ? keynum - KEY_PAD_1 : keynum - KEY_1);
				if ( m_iSelectedCategory != -1 )
				{
					int iClass = g_iRemapKeyToClass[iNum + m_iSelectedCategory * 3];
					SelectDisguise( iClass, m_iShowingTeam );
					return 0;
				}
				else
				{
					m_iSelectedCategory = iNum;
					ToggleSelectionIcons( true );
					engine->ExecuteClientCmd("pda_click");
					return 0;
				}
			}
		case KEY_4:
		case KEY_PAD_4:
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
			if ( !tf_simple_disguise_menu.GetBool() )
			{
				int iNum = (keynum >= KEY_PAD_0 ? keynum - KEY_PAD_1 : keynum - KEY_1);
				int iClass = g_iRemapKeyToClass[iNum];
				SelectDisguise( iClass, m_iShowingTeam );
			}
			return 0;

		case KEY_0:
		case KEY_PAD_0:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			if ( pszCurrentBinding && FStrEq( pszCurrentBinding, "+reload" ) )
			{
				ToggleDisguiseTeam();
				return 0;
			}

			return 1;	// key not handled
		}
	}
	

	return 1;	// key not handled
}


void CHudMenuSpyDisguise::SelectDisguise( int iClass, int iTeam )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		char szCmd[64];
		V_sprintf_safe( szCmd, "disguise %d %d; lastinv", iClass, iTeam );
		engine->ExecuteClientCmd( szCmd );
	}
}


void CHudMenuSpyDisguise::FlipDisguiseTeams( void )
{
	// flip the teams
	m_iShowingTeam++;
	m_iShowingTeam %= GetNumberOfTeams() + ( GetNumberOfTeams() > TF_TEAM_GREEN ) - FIRST_GAME_TEAM;

	for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
	{
		for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
		{
			m_ClassItems[i][j].pPanel->SetVisible( i == m_iShowingTeam );
		}
	}
}


void CHudMenuSpyDisguise::ToggleDisguiseTeam( void )
{
	FlipDisguiseTeams();
	engine->ExecuteClientCmd( "pda_click" );
}


void CHudMenuSpyDisguise::ToggleSelectionIcons( bool bShowSubIcons )
{
	if ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() )
	{
		// Hide all numbers.
		for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
		{
			for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
			{
				m_ClassItems[i][j].pNumber->SetVisible( false );
				m_ClassItems[i][j].pNewNumber->SetVisible( false );
				m_ClassItems[i][j].pKeyIcon->SetVisible( false );
			}
		}

		for ( int i = 0; i < TF_DISGUISE_MENU_CATEGORIES; i++ )
		{
			m_pNumbers[i]->SetVisible( false );
			m_pKeyIcons[i]->SetVisible( false );
		}
	}
	else if ( tf_simple_disguise_menu.GetBool() )
	{
		// Toggle category key icons.
		for ( int i = 0; i < TF_DISGUISE_MENU_CATEGORIES; i++ )
		{
			m_pNumbers[i]->SetVisible( !bShowSubIcons );
			m_pKeyIcons[i]->SetVisible( !bShowSubIcons );
		}

		if ( bShowSubIcons )
		{
			// Show new class key icons of the selected category.
			for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
			{
				for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
				{
					int iRemap = j - 3 * m_iSelectedCategory;
					bool bSelected = ( iRemap >= 0 && iRemap < 3 );

					m_ClassItems[i][j].pNumber->SetVisible( false );
					m_ClassItems[i][j].pNewNumber->SetVisible( bSelected );
					m_ClassItems[i][j].pKeyIcon->SetVisible( bSelected );
				}
			}
		}
		else
		{
			for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
			{
				for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
				{
					m_ClassItems[i][j].pNumber->SetVisible( false );
					m_ClassItems[i][j].pNewNumber->SetVisible( false );
					m_ClassItems[i][j].pKeyIcon->SetVisible( false );
				}
			}
		}
	}
	else
	{
		// Hide category key icons.
		for ( int i = 0; i < TF_DISGUISE_MENU_CATEGORIES; i++ )
		{
			m_pNumbers[i]->SetVisible( false );
			m_pKeyIcons[i]->SetVisible( false );
		}

		// Show old class key icons.
		for ( int i = 0; i < TF_DISGUISE_MENU_TEAMS; i++ )
		{
			for ( int j = 0; j < TF_DISGUISE_MENU_CLASSES; j++ )
			{
				m_ClassItems[i][j].pNumber->SetVisible( true );
				m_ClassItems[i][j].pNewNumber->SetVisible( false );
				m_ClassItems[i][j].pKeyIcon->SetVisible( true );
			}
		}
	}
}


void CHudMenuSpyDisguise::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_ClassItems[TF_TEAM_BLUE][m_iSelectedItem - 1].pPanel->GetPos( x, y );
			m_pActiveSelection->SetPos( x, y );	
		}
	}
}


void CHudMenuSpyDisguise::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "spy_pda_reset") == 0 )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			// If we're in 4team, set it to the global team ( -1 so FlipDisguiseTeam puts it back )
			if( TFGameRules() && TFGameRules()->IsFourTeamGame() )
				m_iShowingTeam = TF_DISGUISE_MENU_TEAMS - 2;
			else
				// Otherwise set it to the team following player's team.
				m_iShowingTeam = pPlayer->GetTeamNumber() - FIRST_GAME_TEAM;
			FlipDisguiseTeams();
			m_iSelectedCategory = -1;
			ToggleSelectionIcons( false );
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}


void CHudMenuSpyDisguise::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() );

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

		// set the %disguiseteam% dialog var
		key = engine->Key_LookupBinding( "disguiseteam" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "disguiseteam", key );

		// set the %reload% dialog var
		key = engine->Key_LookupBinding( "+reload" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "reload", key );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}


void CHudMenuSpyDisguise::DisguiseTeam( const CCommand &args )
{
	if ( IsVisible() )
	{
		ToggleDisguiseTeam();
	}
}