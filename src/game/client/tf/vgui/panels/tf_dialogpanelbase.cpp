#include "cbase.h"
#include "tf_dialogpanelbase.h"
#include "tf_mainmenu.h"
#include "controls/tf_advbutton.h"
#include "controls/tf_advpanellistpanel.h"
#include "controls/tf_cvartogglecheckbutton.h"
#include "controls/tf_cvarslider.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Tooltip.h"
#include "inputsystem/iinputsystem.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include "tier1/convar.h"
#include <stdio.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFDialogPanelBase::CTFDialogPanelBase( vgui::Panel *parent, const char *panelName ) : CTFMenuPanelBase( parent, panelName )
{
	m_pListPanel = NULL;
	bEmbedded = false;
	m_PassUnhandledInput = false;
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFDialogPanelBase::~CTFDialogPanelBase()
{
	DestroyControls();
}

//-----------------------------------------------------------------------------
// Purpose: sets background color & border
//-----------------------------------------------------------------------------
void CTFDialogPanelBase::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	if ( bEmbedded )
	{
		OnCreateControls();
	}
	else
	{
		//Show();
	}
}


void CTFDialogPanelBase::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "vguicancel" ) )
	{
		PostActionSignal( new KeyValues( "CancelPressed" ) );
		OnResetData();
		Hide();
	}
	else if ( !V_stricmp( command, "Ok" ) )
	{
		PostActionSignal( new KeyValues( "OkPressed" ) );
		OnApplyChanges();
		Hide();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


void CTFDialogPanelBase::Show()
{
	BaseClass::Show();

	if ( !bEmbedded )
	{
		RequestFocus();
		MakePopup();
	}

	AppearAnimation();

	guiroot->ShowPanel( SHADEBACKGROUND_MENU );
}


void CTFDialogPanelBase::Hide()
{
	BaseClass::Hide();

	guiroot->HidePanel( SHADEBACKGROUND_MENU );
	guiroot->HideToolTip();
	guiroot->ShowPanel( guiroot->GetCurrentMainMenu() );

	if ( m_bShowSingle )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide" );
	}
}


void CTFDialogPanelBase::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		Hide();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


void CTFDialogPanelBase::OnKeyTyped( wchar_t key )
{
	if ( !bEmbedded )
	{
		Panel *pPanel = HasHotkey( key );
		if ( pPanel )
		{
			PostMessage( pPanel, new KeyValues( "Hotkey" ) );
		}
	}

	BaseClass::OnKeyTyped( key );
}


void CTFDialogPanelBase::AddControl( Panel* panel, objtype_t iType, const char* text /*= ""*/, const char *pszToolTip /*= ""*/, Label **pLabel /*= NULL*/ )
{
	if ( !m_pListPanel )
		return;

	mpcontrol_t	*pCtrl = new mpcontrol_t( m_pListPanel, "mpcontrol_t" );
	pCtrl->type = iType;
	HFont hFont = GETSCHEME()->GetFont( "TF2CMenuNormal", true );

	switch ( pCtrl->type )
	{
	case O_CATEGORY:
	{
		Label *pTitle = assert_cast<Label*>( panel );
		pTitle->MakeReadyForUse();

		pTitle->SetFont( GETSCHEME()->GetFont( "TF2CMenuHeader", true ) );
		pTitle->SetBorder( GETSCHEME()->GetBorder( "AdvSettingsTitleBorder" ) );
		pTitle->SetFgColor( GETSCHEME()->GetColor( "TanLight", COLOR_WHITE ) );
		break;
	}
	case O_BOOL:
	{
		CTFCheckButton *pBox = assert_cast<CTFCheckButton*>( panel );
		pBox->MakeReadyForUse();

		pBox->SetFont( hFont );
		pBox->SetToolTip( pszToolTip );
		break;
	}
	case O_SLIDER:
	{
		CTFSlider *pScroll = assert_cast<CTFSlider*>( panel );
		pScroll->MakeReadyForUse();

		pScroll->SetFont( "TF2CMenuNormal" );
		pScroll->SetToolTip( pszToolTip );
		break;
	}
	case O_LIST:
	{
		ComboBox *pCombo = assert_cast<ComboBox*>( panel );
		pCombo->MakeReadyForUse();

		pCombo->SetFont( hFont );
		break;
	}
	default:
		break;
	}

	if ( pCtrl->type != O_BOOL && pCtrl->type != O_CATEGORY )
	{
		pCtrl->pPrompt = new Label( pCtrl, "DescLabel", "" );
		pCtrl->pPrompt->MakeReadyForUse();

		pCtrl->pPrompt->SetFont( hFont );
		pCtrl->pPrompt->SetContentAlignment( vgui::Label::a_west );
		pCtrl->pPrompt->SetTextInset( 5, 0 );
		pCtrl->pPrompt->SetText( text );

		if ( pLabel )
			*pLabel = pCtrl->pPrompt;
	}

	panel->SetParent( pCtrl );
	pCtrl->pControl = panel;
	int h = m_pListPanel->GetTall() / 13.0; //(float)GetParent()->GetTall() / 15.0;
	pCtrl->SetSize( 800, h );
	m_pListPanel->AddItem( pCtrl );
}


void CTFDialogPanelBase::AppearAnimation()
{
	// Fade in.
	SetAlpha( 0 );
	GetAnimationController()->RunAnimationCommand( this, "Alpha", 255, 0.05f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE );

	// Offset the dialog and make it slide back into normal position.
	/*int x, y;
	GetPos( x, y );
	SetPos( x - YRES( 20 ), y );

	GetAnimationController()->RunAnimationCommand( this, "xpos", x, 0.0f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );*/
}


void CTFDialogPanelBase::CreateControls()
{
	DestroyControls();
}


void CTFDialogPanelBase::DestroyControls()
{
	if ( !m_pListPanel )
		return;

	m_pListPanel->DeleteAllItems();
}


void CTFDialogPanelBase::OnResetData()
{

}


void CTFDialogPanelBase::OnApplyChanges()
{

}


void CTFDialogPanelBase::OnSetDefaults()
{

}