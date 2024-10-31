//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
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
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_controls.h"

#include "tf_hud_itemeffectmeter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


class CHudDemomanPipes : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudDemomanPipes, EditablePanel );

public:
	CHudDemomanPipes( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::EditablePanel *m_pPipesPresent;
	vgui::EditablePanel *m_pNoPipesPresent;
	vgui::EditablePanel *m_pMinesPresent;
	vgui::EditablePanel *m_pNoMinesPresent;

	CExLabel *m_pChargeLabel;
	ContinuousProgressBar *m_pChargeMeter;

	bool m_bInitialIconVisibilityCheck;
	bool m_bPipeIconsVisible;
	bool m_bNoPipeIconsVisible;
};

DECLARE_HUDELEMENT( CHudDemomanPipes );


CHudDemomanPipes::CHudDemomanPipes( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDemomanPipes" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pPipesPresent = new EditablePanel( this, "PipesPresentPanel" );
	m_pNoPipesPresent = new EditablePanel( this, "NoPipesPresentPanel" );
	m_pMinesPresent = new EditablePanel( this, "MinesPresentPanel" );
	m_pNoMinesPresent = new EditablePanel( this, "NoMinesPresentPanel" );

	m_pChargeLabel = new CExLabel( this, "ChargeLabel", "" );
	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bInitialIconVisibilityCheck = false;
	m_bPipeIconsVisible = false;
	m_bNoPipeIconsVisible = false;
}


void CHudDemomanPipes::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDemomanPipes.res" );

	if ( !m_bInitialIconVisibilityCheck )
	{
		m_bPipeIconsVisible = m_pPipesPresent->IsVisible();
		m_bNoPipeIconsVisible = m_pNoPipesPresent->IsVisible();
		m_bInitialIconVisibilityCheck = true;
	}

	// This HUD element is also used for displaying Charging Targe's... well, charge. Brilliant.
	m_pChargeLabel->SetVisible( false );
	m_pChargeMeter->SetVisible( false );

	BaseClass::ApplySchemeSettings( pScheme );
}


bool CHudDemomanPipes::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) )
		return false;

	if ( !pPlayer->IsAlive() )
		return false;

	return CHudElement::ShouldDraw();
}


void CHudDemomanPipes::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN )
		return;

	EditablePanel *pPresentPanel = m_pPipesPresent;
	EditablePanel *pNotPresentPanel = m_pNoPipesPresent;

	if ( m_bPipeIconsVisible || m_bNoPipeIconsVisible )
	{
		m_pPipesPresent->SetVisible( false );
		m_pNoPipesPresent->SetVisible( false );

		m_pMinesPresent->SetVisible( false );
		m_pNoMinesPresent->SetVisible( false );

		int proxyMine = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ), proxyMine, mod_sticky_is_proxy );

		if ( proxyMine > 0 )
		{
			pPresentPanel = m_pMinesPresent;
			pNotPresentPanel = m_pNoMinesPresent;
		}

		pPresentPanel->SetVisible( m_pPipesPresent );
		pNotPresentPanel->SetVisible( m_bNoPipeIconsVisible );
	}

	int iPipes = pPlayer->GetNumActivePipebombs();

	pPresentPanel->SetDialogVariable( "activepipes", iPipes );
	pPresentPanel->SetVisible( iPipes > 0 );

	pNotPresentPanel->SetDialogVariable( "activepipes", iPipes );
	pNotPresentPanel->SetVisible( iPipes <= 0 );
}
