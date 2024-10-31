//========= Copyright 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_spectatorgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include <in_buttons.h>

extern ConVar tf2c_spywalk;
ConVar tf2c_spywalk_hud("tf2c_spywalk_hud", "1", FCVAR_ARCHIVE, "Enables spywalk indicator whenever disguise HUD is active");

class CDisguiseStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDisguiseStatus, vgui::EditablePanel );

public:
	CDisguiseStatus( const char *pElementName );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	void			CheckWeapon( void );
	void			TurnOffSpywalk(void);

private:
	CPanelAnimationVar( vgui::HFont, m_hFont, "TextFont", "TargetID" );
	CTFImagePanel		*m_pDisguiseStatusBG;
	vgui::Label			*m_pDisguiseNameLabel;
	vgui::Label			*m_pWeaponNameLabel;
	CTFSpectatorGUIHealth	*m_pTargetHealth;

	CTFImagePanel		*m_pSpywalkBG;
	vgui::Label			*m_pSpywalkLabel;
	CTFImagePanel		*m_pSpywalkStatusIconInactive;
	CTFImagePanel		*m_pSpywalkStatusIconActive;
	bool				m_bCantSpywalk;
	bool				m_bSpywalkDisabled;
	bool				m_bSpywalkKeySet;
	bool				m_bSpywalkHUDBorked;
	//CEmbeddedItemModelPanel *m_pItemModelPanel;
};

DECLARE_HUDELEMENT( CDisguiseStatus );

using namespace vgui;


CDisguiseStatus::CDisguiseStatus( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "DisguiseStatus" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	ivgui()->AddTickSignal( GetVPanel() );

	m_bCantSpywalk = false;
	m_bSpywalkDisabled = false;
	m_bSpywalkKeySet = false;
	m_bSpywalkHUDBorked = false;
}


bool CDisguiseStatus::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->m_Shared.IsDisguised() )
		return false;

	return CHudElement::ShouldDraw();
}


void CDisguiseStatus::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	LoadControlSettings( "resource/UI/DisguiseStatusPanel.res" );

	//m_pTargetHealth = dynamic_cast< CTFSpectatorGUIHealth *>( FindChildByName( "SpectatorGUIHealth" ) );
	//m_pItemModelPanel = dynamic_cast< CEmbeddedItemModelPanel *>( FindChildByName( "CEmbeddedItemModelPanel" ) );
	m_pDisguiseStatusBG = dynamic_cast< CTFImagePanel * >( FindChildByName( "DisguiseStatusBG" ) );
	m_pDisguiseNameLabel = dynamic_cast< vgui::Label *>( FindChildByName( "DisguiseNameLabel" ) );
	m_pWeaponNameLabel = dynamic_cast< vgui::Label *>( FindChildByName( "WeaponNameLabel" ) );

	m_pSpywalkLabel = dynamic_cast<vgui::Label*>(FindChildByName("SpywalkLabel"));
	m_pSpywalkBG = dynamic_cast<CTFImagePanel*>(FindChildByName("SpywalkBG"));
	m_pSpywalkStatusIconInactive = dynamic_cast<CTFImagePanel*>(FindChildByName("SpywalkStatusIconInactive"));
	m_pSpywalkStatusIconActive = dynamic_cast<CTFImagePanel*>(FindChildByName("SpywalkStatusIconActive"));

	// fix dunmb crashes,
	if ( !m_pSpywalkLabel || !m_pSpywalkBG || !m_pSpywalkStatusIconInactive || !m_pSpywalkStatusIconActive )
		m_bSpywalkHUDBorked = true;

	SetPaintBackgroundEnabled( false );
}


void CDisguiseStatus::CheckWeapon( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	C_TFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
	if ( pDisguiseWeapon )
	{
		const wchar_t *pszDisguiseWeapon = g_pVGuiLocalize->Find( pDisguiseWeapon->GetTFWpnData().szPrintName );

		CEconItemView *pItem = pDisguiseWeapon->GetItem();
		if ( pItem && pItem->GetStaticData() )
		{
			pszDisguiseWeapon = pItem->GetStaticData()->GenerateLocalizedFullItemName();
		}

		SetDialogVariable( "weaponname", pszDisguiseWeapon );
	}
}


void CDisguiseStatus::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// We don't print anything until we're fully disguised.
	if ( !pPlayer->m_Shared.IsDisguised() )
		return;

	if ( m_pDisguiseStatusBG )
	{
		int iTeam = pPlayer->m_Shared.GetTrueDisguiseTeam();
		m_pDisguiseStatusBG->SetBGImage( iTeam == TF_TEAM_GLOBAL ? TEAM_UNASSIGNED : iTeam );
		//m_pDisguiseStatusBG->m_iBGTeam = iTeam;
		//m_pDisguiseStatusBG->UpdateBGImage();
	}

	if ( g_PR )
	{
		int iDisguiseIndex = pPlayer->m_Shared.GetDisguiseTargetIndex();
		Assert( iDisguiseIndex != 0 );
		SetDialogVariable( "disguisename", g_PR->GetPlayerName( iDisguiseIndex ) );
	}

	CheckWeapon();
	
	if ( m_pTargetHealth )
	{
		m_pTargetHealth->SetHealth( pPlayer->m_Shared.GetDisguiseHealth(), pPlayer->m_Shared.GetDisguiseMaxHealth(), pPlayer->m_Shared.GetDisguiseMaxBuffedHealth() );
	}

	// The "Spy-Walk"
	
	// this shouldnt happen
	if (m_bSpywalkHUDBorked)
		return;

	if ( !tf2c_spywalk.GetBool() || !tf2c_spywalk_hud.GetBool() )
	{
		if ( !m_bSpywalkDisabled )
		{
			TurnOffSpywalk();
			m_bSpywalkDisabled = true;
		}
		return;
	}

	m_bSpywalkDisabled = false;

	// this shouldnt happen
	if ( m_bSpywalkHUDBorked )
		return;

	int iDisguiseClass = pPlayer->m_Shared.GetDisguiseClass();
	if (iDisguiseClass == TF_CLASS_MEDIC || iDisguiseClass == TF_CLASS_SCOUT || iDisguiseClass == TF_CLASS_SPY)
	{
		if ( !m_bCantSpywalk )
		{
			TurnOffSpywalk();
			m_bCantSpywalk = true;
		}
		return;
	}

	// for some reason this doesnt work unless i put it on here. terribleness
	if ( !m_bSpywalkKeySet )
	{
		const char* key = engine->Key_LookupBinding("+speed");
		SetDialogVariable("spywalkbind", key ? key : "< not bound >");
		m_bSpywalkKeySet = true;
	}

	m_bCantSpywalk = false;

	m_pSpywalkLabel->SetVisible(true);
	m_pSpywalkBG->SetVisible(true);

	bool bIsSpywalking = pPlayer->IsSpywalkInverted() ? !(pPlayer->m_nButtons & IN_SPEED) : pPlayer->m_nButtons & IN_SPEED;

	m_pSpywalkStatusIconInactive->SetVisible(!bIsSpywalking);
	m_pSpywalkStatusIconActive->SetVisible(bIsSpywalking);
}

void CDisguiseStatus::TurnOffSpywalk()
{
	m_pSpywalkBG->SetVisible(false);
	m_pSpywalkStatusIconInactive->SetVisible(false);
	m_pSpywalkStatusIconActive->SetVisible(false);
	m_pSpywalkLabel->SetVisible(false);
}