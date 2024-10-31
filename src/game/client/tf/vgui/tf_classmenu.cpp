//========= Copyright  1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_classmenu.h"
#include <vgui/IVGui.h>
#include "IGameUIFuncs.h" // for key bindings
#include <vgui_controls/ScrollBarSlider.h>
#include "fmtstr.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "engine/IEngineSound.h"
#include "tf_viewport.h"
#include "tf_gamerules.h"
#include "tf_mainmenu.h"
#include <vgui/ILocalize.h>

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

ConVar hud_classautokill( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Automatically kill player after choosing a new playerclass." );
ConVar _cl_classmenuopen( "_cl_classmenuopen", "0", FCVAR_NONE, "internal cvar used to tell server when class menu is open" );

ConVar tf2c_classmenu_music( "tf2c_classmenu_music", "2", FCVAR_ARCHIVE, "Plays a looping piece of music when you're in the class selection menu. 0 = Disabled, 1 = Enabled, 2 = Enabled + Gamemode Specific" );
ConVar tf2c_classmenu_jingle( "tf2c_classmenu_jingle", "2", FCVAR_ARCHIVE, "Plays a jingle whenever you hover over a playerclass. 0 = Disabled, 1 = Enabled, 2 = Enabled + Gamemode Specific" );
ConVar tf2c_classmenu_focus( "tf2c_classmenu_focus", "1", FCVAR_ARCHIVE, "Applies the 'ClassMenu_Only' soundmix." );

extern ConVar tf2c_allow_special_classes;

//=============================================================================//
// CTFClassMenu
//=============================================================================//

// menu buttons are not in the same order as the defines
static int iRemapButtonToClass[TF_CLASS_MENU_BUTTONS] =
{
	0,
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,
	TF_CLASS_CIVILIAN,
	0,
	TF_CLASS_RANDOM
};

#define TF_DEFAULT_CLASSMENU_MUSIC	"music.class_menu"
#define TF_VERSUS_CLASSMENU_MUSIC	"music.vs_class_menu"

// Background music, which can be different depending on the gamemode.
static const char *g_aClassMenuMusic[TF_GAMETYPE_COUNT] =
{
	TF_DEFAULT_CLASSMENU_MUSIC,			// UNDEFINED
	"music.ctf_class_menu",				// Capture the Flag
	"music.cp_class_menu",				// Control Point
	TF_DEFAULT_CLASSMENU_MUSIC,			// Payload (Race)
	TF_VERSUS_CLASSMENU_MUSIC,			// Arena
	"music.mvm_class_menu",				// Mann Vs. Machine
	TF_DEFAULT_CLASSMENU_MUSIC,			// Robot Destruction
	TF_DEFAULT_CLASSMENU_MUSIC,			// Passtime
	TF_VERSUS_CLASSMENU_MUSIC,			// Player Destruction
	"music.vip_class_menu",				// V.I.P
};

#define TF_DEFAULT_CLASSMENU_HOVER_SOUNDS { NULL, "music.class_menu_01", "music.class_menu_02", "music.class_menu_03", "music.class_menu_04", "music.class_menu_05", "music.class_menu_06", "music.class_menu_07", "music.class_menu_08", "music.class_menu_09", "music.class_menu_10", NULL, NULL, }

// Button hover sounds for each class.
static const char *g_aHoverupSounds[TF_GAMETYPE_COUNT][TF_CLASS_MENU_BUTTONS] =
{
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// UNDEFINED
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Capture the Flag
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Control Point
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Payload (Race)
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Arena
	{									// Mann Vs. Machine
		NULL,
		"music.mvm_class_menu_01",
		"music.mvm_class_menu_02",
		"music.mvm_class_menu_03",
		"music.mvm_class_menu_04",
		"music.mvm_class_menu_05",
		"music.mvm_class_menu_06",
		"music.mvm_class_menu_07",
		"music.mvm_class_menu_08",
		"music.mvm_class_menu_09",
		"music.mvm_class_menu_10",
		NULL,
		NULL,
	},
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Robot Destruction
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Passtime
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// Player Destruction
	TF_DEFAULT_CLASSMENU_HOVER_SOUNDS,	// V.I.P
};

// Medieval gets the special treatment.
#define TF_MEDIEVAL_CLASSMENU_MUSIC	"music.ml_class_menu"

int GetIndexForClass( int iClass )
{
	for ( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		if ( iRemapButtonToClass[i] == iClass )
		{
			return i;
		}
	}

	return 0;
}



CTFClassMenu::CTFClassMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_CLASS )
{
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	// initialize dialog
	SetTitle( "", true );

	// load the new scheme early!!
	SetScheme( ClientSchemesArray[SCHEME_CLIENT_STRING] );
	SetMoveable( false );
	SetSizeable( false );

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	m_iClassMenuKey = BUTTON_CODE_INVALID;
	m_iCurrentButtonIndex = TF_CLASS_HEAVYWEAPONS;

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#endif

	memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );

	// Make buttons for all normal classes + random.
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= TF_CLASS_COUNT; i++ )
	{
		m_pClassButtons[i] = new CExImageButton( this, g_aRawPlayerClassNames[i], "" );
	}

	m_pClassButtons[TF_CLASS_RANDOM] = new CExImageButton( this, "random", "" );
	m_pPlayerModel = new CTFPlayerModelPanel( this, "TFPlayerModel" );
	m_pTipsPanel = new CTFClassTipsPanel( this, "ClassTipsPanel" );

	m_pCancelButton = new CExButton( this, "CancelButton", "" );
	m_pLoadoutButton = new CExButton( this, "EditLoadoutButton", "" );
	m_pClassSelectLabel = new CExLabel( this, "ClassMenuSelect", "" );

#ifndef _X360
	char tempName[MAX_PATH];
	for ( int i = 0; i < CLASS_COUNT_IMAGES; ++i )
	{
		V_sprintf_safe( tempName, "countImage%d", i );
		m_ClassCountImages[i] = new CTFImagePanel( this, tempName );
	}

	m_pCountLabel = NULL;
#endif

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CTFClassMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions;
	if ( tf2c_allow_special_classes.GetBool() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_special_classes" );
	}
	else
	{
		pConditions = NULL;
	}

	LoadControlSettings( "Resource/UI/ClassSelection.res", NULL, NULL, pConditions );

	pConditions->deleteThis();
}


void CTFClassMenu::PerformLayout()
{
	BaseClass::PerformLayout();

#ifndef _X360
	m_pCountLabel = dynamic_cast<CExLabel *>( FindChildByName( "CountLabel" ) );

	if ( m_pCountLabel )
	{
		m_pCountLabel->SizeToContents();
	}
#endif
}


int CTFClassMenu::GetCurrentClass( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED )
		return pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();

	// Default to Heavy.
	return TF_CLASS_HEAVYWEAPONS;
}


void CTFClassMenu::SetData( KeyValues *data )
{
	SetTeam( data->GetInt( "team" ) );
}


void CTFClassMenu::ShowPanel( bool bShow )
{
	if ( bShow == IsVisible() )
		return;

	const char *pszClassMenuMusic;
	if ( tf2c_classmenu_music.GetInt() >= 2 )
	{
		if ( TFGameRules()->IsInMedievalMode() )
		{
			pszClassMenuMusic = TF_MEDIEVAL_CLASSMENU_MUSIC;
		}
		else if ( TFGameRules()->IsInArenaMode() )
		{
			pszClassMenuMusic = TF_VERSUS_CLASSMENU_MUSIC;
		}
		else
		{
			pszClassMenuMusic = g_aClassMenuMusic[TFGameRules()->GetGameType()];
		}
	}
	else
	{
		pszClassMenuMusic = g_aClassMenuMusic[TF_GAMETYPE_UNDEFINED];
	}

	if ( bShow )
	{
		engine->CheckPoint( "ClassMenu" );

		InvalidateLayout( true, true );

		Activate();
		SetMouseInputEnabled( true );

		ConVar *snd_musicvolume = cvar->FindVar( "snd_musicvolume" );
		bool bClassMenuMusic = tf2c_classmenu_music.GetBool();
		if ( bClassMenuMusic )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, pszClassMenuMusic );
		}
		m_pPlayerModel->SetSoundEventAllowed( !bClassMenuMusic || snd_musicvolume->GetFloat() <= 0.5f );

		m_iClassMenuKey = gameuifuncs->GetButtonCodeForBind( "changeclass" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		// Set button images.
		for ( int i = TF_FIRST_NORMAL_CLASS; i <= TF_CLASS_COUNT; i++ )
		{
			m_pClassButtons[i]->SetImageSelected( VarArgs( "class_sel_sm_%s_%s", g_aRawPlayerClassNamesShort[i], g_aTeamNamesShort[m_iTeamNum] ) );
			m_pClassButtons[i]->SetArmedSound( NULL );
		}

		m_pClassButtons[TF_CLASS_RANDOM]->SetImageSelected( VarArgs( "class_sel_sm_random_%s", g_aTeamNamesShort[m_iTeamNum] ) );
		m_pClassButtons[TF_CLASS_RANDOM]->SetArmedSound( NULL );

		// Show our current class
		SelectClass( GetCurrentClass() );
		m_pPlayerModel->SetLookAtCamera( true );
	}
	else
	{
		m_pPlayerModel->StopVCD();
		C_BaseEntity::StopSound( SOUND_FROM_UI_PANEL, pszClassMenuMusic );

		SetVisible( false );
		SetMouseInputEnabled( false );

		if ( tf2c_classmenu_focus.GetBool() )
		{
			m_bRevertSoundMixer = true;
		}
	}
}


void CTFClassMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( ( m_iClassMenuKey != BUTTON_CODE_INVALID && m_iClassMenuKey == code ) ||
		code == KEY_XBUTTON_BACK ||
		code == KEY_XBUTTON_B )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) )
		{
			ShowPanel( false );
		}
	}
	else if ( code >= KEY_1 && code <= KEY_9 )
	{
		int iButton = ( code - KEY_1 + 1 );
		OnCommand( VarArgs( "select %d", iRemapButtonToClass[iButton] ) );
	}
	else if ( code == KEY_SPACE || code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER )
	{
		ipanel()->SendMessage( GetFocusNavGroup().GetDefaultButton(), new KeyValues( "PressButton" ), GetVPanel() );
	}
	else if ( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT )
	{
		int iButton = m_iCurrentButtonIndex;
		int loopCheck = 0;

		do
		{
			loopCheck++;
			iButton++;
			iButton = ( iButton % TF_CLASS_MENU_BUTTONS );
		}
		while ( ( m_pClassButtons[iRemapButtonToClass[iButton]] == NULL || ( !tf2c_allow_special_classes.GetBool() && iRemapButtonToClass[iButton] > TF_LAST_NORMAL_CLASS && iRemapButtonToClass[iButton] < TF_CLASS_RANDOM ) ) && (loopCheck < TF_CLASS_MENU_BUTTONS ) );

		CExImageButton *pButton = m_pClassButtons[iRemapButtonToClass[iButton]];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if ( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT )
	{
		int iButton = m_iCurrentButtonIndex;
		int loopCheck = 0;

		do
		{
			loopCheck++;
			iButton--;
			if ( iButton <= 0 )
			{
				iButton = TF_CLASS_RANDOM;
			}
		}
		while ( ( m_pClassButtons[iRemapButtonToClass[iButton]] == NULL || ( !tf2c_allow_special_classes.GetBool() && iRemapButtonToClass[iButton] > TF_LAST_NORMAL_CLASS && iRemapButtonToClass[iButton] < TF_CLASS_RANDOM ) ) && (loopCheck < TF_CLASS_MENU_BUTTONS ) );

		CExImageButton *pButton = m_pClassButtons[iRemapButtonToClass[iButton]];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if ( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class tips up
		//PostMessage( m_pTipsPanel, new KeyValues( "MoveScrollBarDirect", "delta", 1 ) );
	}
	else if ( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class tips down
		//PostMessage( m_pTipsPanel, new KeyValues( "MoveScrollBarDirect", "delta", -1 ) );
	}
	else if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		GetTFViewPort()->ShowScoreboard( true, code );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


void CTFClassMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}


void CTFClassMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	// Check which button is armed and switch to class appropriately.
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		CExImageButton *pButton = m_pClassButtons[i];
		if ( pButton && pButton->IsArmed() )
		{
			bool bGamemodeSpecificJingles = ( tf2c_classmenu_jingle.GetInt() >= 2 );

			// Play the selection sound. Not putting it in SelectClass since we don't want to play sounds when the menu is opened.
			C_BaseEntity::StopSound( SOUND_FROM_UI_PANEL, g_aHoverupSounds[bGamemodeSpecificJingles ? TFGameRules()->GetGameType() : TF_GAMETYPE_UNDEFINED][iRemapButtonToClass[m_iCurrentButtonIndex]] );

			if ( tf2c_classmenu_jingle.GetBool() )
			{
				C_RecipientFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, g_aHoverupSounds[bGamemodeSpecificJingles ? TFGameRules()->GetGameType() : TF_GAMETYPE_UNDEFINED][i] );
			}

			SelectClass( i );
			break;
		}
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}


void CTFClassMenu::Update()
{
	// Force them to pick a class if they haven't picked one yet.
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		m_pCancelButton->SetVisible( true );
		m_pClassSelectLabel->SetVisible( false );
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		m_pCancelButton->SetVisible( false );
		m_pClassSelectLabel->SetVisible( true );
#endif
	}
}

//-----------------------------------------------------------------------------
// Draw nothing
//-----------------------------------------------------------------------------
void CTFClassMenu::PaintBackground( void )
{
}

void CTFClassMenu::PostInit()
{
	// moved out of OnTick because it was HOT code
	// -sappho
	if (!snd_musicvolume)
	{
		snd_musicvolume = cvar->FindVar("snd_musicvolume");
	}
	if (!pSndMixer)
	{
		pSndMixer = cvar->FindVar("snd_soundmixer");
	}
}

//-----------------------------------------------------------------------------
// Do things that should be done often, eg number of players in the 
// selected class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnTick( void )
{
	// When a player changes teams, their class and team values don't get here 
	// necessarily before the command to update the class menu. This leads to the cancel button 
	// being visible and people cancelling before they have a class. check for class == TF_CLASS_UNDEFINED and if so
	// hide the cancel button

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	bool bVisible = IsVisible();

	// should never happen
	if (!snd_musicvolume || !pSndMixer)
	{
		PostInit();
	}

	// Forcing this crap here.
	if ( !tf2c_classmenu_focus.GetBool() || !bVisible || snd_musicvolume->GetFloat() <= 0.5f )
	{
		if ( m_bRevertSoundMixer && ( !pLocalPlayer || ( pLocalPlayer && pLocalPlayer->CanSetSoundMixer() ) ) )
		{
			pSndMixer->Revert();

			m_bRevertSoundMixer = false;
		}

		if ( !bVisible )
			return;
	}
	else if ( tf2c_classmenu_music.GetBool() && snd_musicvolume->GetFloat() > 0.5f )
	{
		pSndMixer->SetValue( "ClassMenu_Only" );
	}

#ifndef _X360
	// Force them to pick a class if they haven't picked one yet.
	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
	{
		m_pCancelButton->SetVisible( false );
		m_pClassSelectLabel->SetVisible( true );
	}

	UpdateNumClassLabels( m_iTeamNum );
#endif

	BaseClass::OnTick();
}


void CTFClassMenu::OnClose()
{
	ShowPanel( false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_HEALTH;
	}

	BaseClass::OnClose();
}


void CTFClassMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	m_KeyRepeat.Reset();

	if ( state )
	{
		engine->ServerCmd( "menuopen" );			// to the server
		engine->ClientCmd( "_cl_classmenuopen 1" );	// for other panels
	}
	else
	{
		engine->ServerCmd( "menuclosed" );
		engine->ClientCmd( "_cl_classmenuopen 0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "select ", 7 ) )
	{
		// God damn it, Valve, you do NOT expose enum indexes!
		int iClass = atoi( command + 7 );
		if ( iClass == 12 )
		{
			engine->ClientCmd( "joinclass random" );
		}
		else
		{
			engine->ClientCmd( VarArgs( "joinclass %s", g_aRawPlayerClassNames[Clamp<int>( iClass, 0, ARRAYSIZE( g_aRawPlayerClassNames ) )] ) );
		}
	}
	else if ( !V_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( command );
	}
	else if ( !V_stricmp( command, "openloadout" ) )
	{
		if ( guiroot )
		{
			guiroot->OpenLoadoutToClass( iRemapButtonToClass[m_iCurrentButtonIndex] );
		}
		return;
	}

	Close();

	gViewPortInterface->ShowBackGround( false );
}


void CTFClassMenu::SetTeam( int iTeam )
{
	// If we're in Arena mode then class menu may be opened while we're in spec. In that case, show RED menu.
	if ( iTeam < FIRST_GAME_TEAM )
	{
		m_iTeamNum = TF_TEAM_RED;
	}
	else
	{
		m_iTeamNum = iTeam;
	}
}

static ETFLoadoutSlot g_iClassSelectWeapons[TF_CLASS_COUNT_ALL] =
{
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_UNDEFINED
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_SCOUT
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_SNIPER
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_SOLDIER
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_DEMOMAN
	TF_LOADOUT_SLOT_SECONDARY, // TF_CLASS_MEDIC
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_HEAVYWEAPONS
	TF_LOADOUT_SLOT_PRIMARY, // TF_CLASS_PYRO
	TF_LOADOUT_SLOT_MELEE, // TF_CLASS_SPY
	TF_LOADOUT_SLOT_MELEE, // TF_CLASS_ENGINEER
	TF_LOADOUT_SLOT_MELEE, // TF_CLASS_CIVILIAN
};

//-----------------------------------------------------------------------------
// Purpose: Show the model and tips for this class.
//-----------------------------------------------------------------------------
void CTFClassMenu::SelectClass( int iClass )
{
	m_iCurrentButtonIndex = GetIndexForClass( iClass );
	
	// Select the button for this class and unselect all other ones.
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		CExImageButton *pButton = m_pClassButtons[i];
		if ( pButton )
		{
			pButton->SetSelected( i == iClass );

			if ( i == iClass )
			{
				pButton->SetAsDefaultButton( 1 );
			}
		}
	}

	if ( iClass == TF_CLASS_RANDOM )
	{
		m_pPlayerModel->SetToRandomClass( m_iTeamNum );
	}
	else
	{
		m_pPlayerModel->SetToPlayerClass( iClass );
		m_pPlayerModel->SetTeam( m_iTeamNum );
		m_pPlayerModel->LoadItems();

		// Attempt to play class select VCD (if your weapon allows it).
		if ( m_pPlayerModel->HoldItemInSlot( g_iClassSelectWeapons[iClass] ) )
		{
			const char *szVCD = "class_select";
			bool bDisableFancyAnim = false;

			CEconItemView *pItem = m_pPlayerModel->GetItemInSlot( g_iClassSelectWeapons[iClass] );
			if ( pItem )
			{
				CEconAttributeDefinition *pDisableFancyAnim = GetItemSchema()->GetAttributeDefinitionByName( "disable fancy class select anim" );
				CEconAttributeDefinition *pVCDOverride = GetItemSchema()->GetAttributeDefinitionByName( "class select override vcd" );

				for ( int i = 0; i < pItem->GetStaticData()->attributes.Count(); i++ )
				{
					CEconItemAttribute pAttribute = pItem->GetStaticData()->attributes[i];

					// Prioritize the disabling, over the replacing.
					if ( pDisableFancyAnim && !V_stricmp( pAttribute.attribute_class, pDisableFancyAnim->attribute_class ) )
					{
						bDisableFancyAnim = !!pAttribute.value;
						break;
					}
					else if ( pVCDOverride && !V_stricmp( pAttribute.attribute_class, pVCDOverride->attribute_class ) )
					{
						szVCD = pAttribute.value_string;
						break;
					}
				}
			}

			if ( !bDisableFancyAnim )
			{
				m_pPlayerModel->PlayVCD( VarArgs( "scenes/player/%s/low/%s.vcd", g_aPlayerClassNames_NonLocalized[iClass], szVCD ) );
			}
		}
	}

	// Update the tips list.
	m_pTipsPanel->SetClass( iClass );

	// Hide the loadout button if Random button is selected.
	m_pLoadoutButton->SetVisible( iClass <= TF_CLASS_COUNT );
}


void CTFClassMenu::UpdateNumClassLabels( int iTeam )
{
#ifndef _X360
	int nTotalCount = 0;

	// count how many of each class there are
	if ( !g_TF_PR )
		return;

	if ( iTeam < FIRST_GAME_TEAM || iTeam >= GetNumberOfTeams() ) // invalid team number
		return;

	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; i++ )
	{
		int iClass = iRemapButtonToClass[i];

		int classCount;
		if ( TFGameRules()->IsInArenaQueueMode() && GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		{
			// If we just joined the game in Arena don't show player counts since player is automatically assigned to a team.
			classCount = 0;
		}
		else
		{
			classCount = g_TF_PR->GetCountForPlayerClass( iTeam, iClass );
		}

		char szDialogVar[32];
		V_sprintf_safe( szDialogVar, "num%s", g_aPlayerClassNames_NonLocalized[iClass] );

		if ( classCount > 0 )
		{
			SetDialogVariable( szDialogVar, classCount );
		}
		else
		{
			SetDialogVariable( szDialogVar, "" );
		}

		if ( nTotalCount < CLASS_COUNT_IMAGES )
		{
			for ( int j = 0; j < classCount; ++j )
			{
				CTFImagePanel *pImage = m_ClassCountImages[nTotalCount];
				if ( pImage )
				{
					pImage->SetVisible( true );

					char szImage[128];
					V_sprintf_safe( szImage, "class_sel_sm_%s_%s", g_aRawPlayerClassNamesShort[iClass], g_aTeamNamesShort[iTeam] );
					pImage->SetImage( szImage );
				}

				nTotalCount++;
				if ( nTotalCount >= CLASS_COUNT_IMAGES )
					break;
			}
		}
	}

	if ( nTotalCount == 0 )
	{
		// no classes for our team yet
		if ( m_pCountLabel && m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( false );
		}
	}
	else
	{
		if ( m_pCountLabel && !m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( true );
		}
	}

	// turn off any unused images
	while ( nTotalCount < CLASS_COUNT_IMAGES )
	{
		CTFImagePanel *pImage = m_ClassCountImages[nTotalCount];
		if ( pImage )
		{
			pImage->SetVisible( false );
		}

		nTotalCount++;
	}
#endif
}


void CTFClassMenu::OnShowToTeam( int iTeam )
{
	SetTeam( iTeam );
	gViewPortInterface->ShowPanel( this, true );
}


void CTFClassMenu::OnLoadoutChanged( void )
{
	if ( IsVisible() )
	{
		// Reselect the current class, so the model's items are updated.
		SelectClass( iRemapButtonToClass[m_iCurrentButtonIndex] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console command to select a class
//-----------------------------------------------------------------------------
void CTFClassMenu::Join_Class( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		OnCommand( VarArgs( "joinclass %s", args.Arg( 1 ) ) );
	}
}

//=============================================================================//
// CTFClassTipsPanel
//=============================================================================//
CTFClassTipsPanel::CTFClassTipsPanel( Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_pTipList = new CTFClassTipsListPanel( this, "ClassTipsListPanel" );
}


void CTFClassTipsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ClassTipsList.res" );
	m_pTipList->SetFirstColumnWidth( 0 );
}

static const char *g_aClassMenuNames[TF_CLASS_MENU_BUTTONS] =
{
	"",
	"#TF_Scout",
	"#TF_Sniper",
	"#TF_Soldier",
	"#TF_Demoman",
	"#TF_Medic",
	"#TF_HWGuy",
	"#TF_Pyro",
	"#TF_Spy",
	"#TF_Engineer",
	"#TF_Civilian",
	"",
	"#TF_Random",
};

//-----------------------------------------------------------------------------
// Purpose: Load tips for the specified class from the localization file.
//-----------------------------------------------------------------------------
void CTFClassTipsPanel::SetClass( int iClass )
{
	m_pTipList->DeleteAllItems();
	int iTipClass = iClass;

	if ( iClass == TF_CLASS_RANDOM )
	{
		// Ugh...
		iTipClass = 12;
	}
	/*else if ( iClass > TF_LAST_NORMAL_CLASS )
	{
		// No tips for special classes.
		SetDialogVariable( "classname", g_pVGuiLocalize->Find( g_aClassMenuNames[iClass] ) );
		SetDialogVariable( "classinfo", "" );
		return;
	}*/

	// Get the tips amount.
	wchar_t *wzTipCount = g_pVGuiLocalize->Find( CFmtStr( "ClassTips_%d_Count", iTipClass ) );
	int iClassTipCount = wzTipCount ? _wtoi( wzTipCount ) : 0;
	Assert( iClassTipCount > 0 );
	if ( iClassTipCount <= 0 )
		return;

	wchar_t wszAllTips[1024] = L"";

	for ( int i = 1; i <= iClassTipCount; i++ )
	{
		const wchar_t *wszTip = g_pVGuiLocalize->Find( CFmtStr( "#ClassTips_%d_%d", iTipClass, i ) );
		const wchar_t *wszIcon = g_pVGuiLocalize->Find( CFmtStr( "#ClassTips_%d_%d_Icon", iTipClass, i ) );
		if ( !wszTip || !wszIcon )
		{
			// Stop when we reach MvM tips.
			break;
		}

		V_wcscat_safe( wszAllTips, wszTip );
		V_wcscat_safe( wszAllTips, L"\n" );

		// Add a new panel with this tip to the list.
		char szIcon[256];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszIcon, szIcon, sizeof( szIcon ) );

		CTFClassTipsItemPanel *pTipPanel = new CTFClassTipsItemPanel( this, "ClassTipsItemPanel" );
		pTipPanel->MakeReadyForUse();
		pTipPanel->SetTip( szIcon, wszTip );
		m_pTipList->AddItem( NULL, pTipPanel );
	}

	// Fill in the old style chalkboard.
	SetDialogVariable( "classname", g_pVGuiLocalize->Find( g_aClassMenuNames[iClass] ) );
	SetDialogVariable( "classinfo", wszAllTips );
}

//=============================================================================//
// CTFClassTipsListPanel
//=============================================================================//
CTFClassTipsListPanel::CTFClassTipsListPanel( Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	SetCursor( dc_arrow );

	m_pUpArrow = new ScalableImagePanel( this, "UpArrow" );
	if ( m_pUpArrow )
	{
		//m_pUpArrow->SetShouldScaleImage( true );
		m_pUpArrow->SetImage( "chalkboard_scroll_up" );
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pUpArrow->SetAlpha( 255 );
		m_pUpArrow->SetVisible( false );
	}

	m_pLine = new ImagePanel( this, "Line" );
	if ( m_pLine )
	{
		m_pLine->SetShouldScaleImage( true );
		m_pLine->SetImage( "chalkboard_scroll_line" );
		m_pLine->SetVisible( false );
	}

	m_pDownArrow = new ScalableImagePanel( this, "DownArrow" );
	if ( m_pDownArrow )
	{
		//m_pDownArrow->SetShouldScaleImage( true );
		m_pDownArrow->SetImage( "chalkboard_scroll_down" );
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pDownArrow->SetAlpha( 255 );
		m_pDownArrow->SetVisible( false );
	}

	m_pBox = new ImagePanel( this, "Box" );
	if ( m_pBox )
	{
		m_pBox->SetShouldScaleImage( true );
		m_pBox->SetImage( "chalkboard_scroll_box" );
		m_pBox->SetVisible( false );
	}
}


void CTFClassTipsListPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBorder( pScheme->GetBorder( "NoBorder" ) );
	SetBgColor( pScheme->GetColor( "Blank", Color( 0, 0, 0, 0 ) ) );

	if ( m_pDownArrow )
	{
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	if ( m_pUpArrow )
	{
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	SetScrollBarImagesVisible( false );
}


void CTFClassTipsListPanel::ApplySettings( KeyValues *inResourceData )
{
	// RES file does not set autohide_scrollbar and and m_bAutoHideScrollbar is private. Let's fix this.
	inResourceData->SetBool( "autohide_scrollbar", true );
	BaseClass::ApplySettings( inResourceData );
}


void CTFClassTipsListPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( GetScrollbar() && GetScrollbar()->IsVisible() )
	{
		int nMin, nMax;
		GetScrollbar()->GetRange( nMin, nMax );

		int nScrollbarWide = GetScrollbar()->GetWide();

		int wide, tall;
		GetSize( wide, tall );

		m_pUpArrow->SetBounds( wide - nScrollbarWide, 0, nScrollbarWide, nScrollbarWide );
		m_pLine->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, tall - ( 2 * nScrollbarWide ) );
		m_pBox->SetBounds( wide - nScrollbarWide, m_pBox->GetYPos(), nScrollbarWide, m_pBox->GetTall() );
		m_pDownArrow->SetBounds( wide - nScrollbarWide, tall - nScrollbarWide, nScrollbarWide, nScrollbarWide );
	}
}


void CTFClassTipsListPanel::OnThink( void )
{
	if ( GetScrollbar()->IsVisible() )
	{
		GetScrollbar()->SetZPos( 500 );

		// turn off painting the vertical scrollbar
		GetScrollbar()->SetPaintBackgroundEnabled( false );
		GetScrollbar()->SetPaintBorderEnabled( false );
		GetScrollbar()->SetPaintEnabled( false );
		GetScrollbar()->SetScrollbarButtonsVisible( false );

		// turn on our own images
		SetScrollBarImagesVisible( true );

		int nMin, nMax;
		GetScrollbar()->GetRange( nMin, nMax );
		int nScrollPos = GetScrollbar()->GetValue();
		int nRangeWindow = GetScrollbar()->GetRangeWindow();
		int nBottom = nMax - nRangeWindow;
		if ( nBottom < 0 )
		{
			nBottom = 0;
		}

		// set the alpha on the up arrow
		int nAlpha = ( nScrollPos - nMin <= 0 ) ? 90 : 255;
		m_pUpArrow->SetAlpha( nAlpha );

		// set the alpha on the down arrow
		nAlpha = ( nScrollPos >= nBottom ) ? 90 : 255;
		m_pDownArrow->SetAlpha( nAlpha );

		if ( nRangeWindow > 0 )
		{
			ScrollBarSlider *pSlider = GetScrollbar()->GetSlider();

			int x, y, w, t, min, max;
			m_pLine->GetBounds( x, y, w, t );
			pSlider->GetNobPos( min, max );

			m_pBox->SetBounds( x, y + min, w, ( max - min ) );
		}
	}
	else
	{
		// turn off our images
		SetScrollBarImagesVisible( false );
	}
}


void CTFClassTipsListPanel::SetScrollBarImagesVisible( bool visible )
{
	m_pDownArrow->SetVisible( visible );
	m_pUpArrow->SetVisible( visible );
	m_pLine->SetVisible( visible );
	m_pBox->SetVisible( visible );
}
