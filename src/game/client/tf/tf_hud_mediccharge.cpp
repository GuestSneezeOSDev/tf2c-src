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
#include "tf_weapon_medigun.h"
#include <vgui_controls/AnimationController.h>
#include "tf_controls.h"

#include "tf_hud_itemeffectmeter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_hud_minmode;

using namespace vgui;


class CHudMedicChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMedicChargeMeter, EditablePanel );

public:
	CHudMedicChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	ContinuousProgressBar *m_pChargeMeter;

	CExLabel *m_pIndividualChargesLabel;
	ContinuousProgressBar *m_pChargeMeter1;
	ContinuousProgressBar *m_pChargeMeter2;
	ContinuousProgressBar *m_pChargeMeter3;
	ContinuousProgressBar *m_pChargeMeter4;
	ImagePanel *m_pMedigunIcon;
	ImagePanel *m_pResistIcon;
	CExLabel *m_pRateBonusLabel;

	bool m_bCharged;
	float m_flLastChargeValue;

	bool m_bMedigunIconIsVisible;
};

DECLARE_HUDELEMENT( CHudMedicChargeMeter );


CHudMedicChargeMeter::CHudMedicChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMedicCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	m_pIndividualChargesLabel = new CExLabel( this, "IndividualChargesLabel", "" );
	m_pChargeMeter1 = new ContinuousProgressBar( this, "ChargeMeter1" );
	m_pChargeMeter2 = new ContinuousProgressBar( this, "ChargeMeter2" );
	m_pChargeMeter3 = new ContinuousProgressBar( this, "ChargeMeter3" );
	m_pChargeMeter4 = new ContinuousProgressBar( this, "ChargeMeter4" );
	m_pMedigunIcon = NULL;
	m_pResistIcon = new ImagePanel( this, "ResistIcon" );
	m_pRateBonusLabel = new CExLabel(this, "RateBonusLabel", ""	);

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bCharged = false;
	m_flLastChargeValue = 0;
	SetDialogVariable( "charge", 0 );
	SetDialogVariable( "buildratebonus", 0 );

	m_bMedigunIconIsVisible = false;
}


void CHudMedicChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudMedicCharge.res" );

	// Hide Vaccinator stuff.
	m_pIndividualChargesLabel->SetVisible( false );
	m_pChargeMeter1->SetVisible( false );
	m_pChargeMeter2->SetVisible( false );
	m_pChargeMeter3->SetVisible( false );
	m_pChargeMeter4->SetVisible( false );
	m_pResistIcon->SetVisible( false );

	// The default icon.
	m_pMedigunIcon = dynamic_cast<ImagePanel *>( FindChildByName( "HealthClusterIcon" ) );
	if ( m_pMedigunIcon )
	{
		m_bMedigunIconIsVisible = m_pMedigunIcon->IsVisible();
	}

	BaseClass::ApplySchemeSettings( pScheme );
}


bool CHudMedicChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	if ( !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		return false;

	C_TFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return false;

	ITFHealingWeapon *pMedigun = pPlayer->GetMedigun();
	if ( !pMedigun )
		return false;

	if ( pWpn == pMedigun->GetWeapon() || pWpn->GetWeaponID() == TF_WEAPON_BONESAW  || pWpn->GetWeaponID() == TF_WEAPON_TASER )
		return CHudElement::ShouldDraw();

	return false;
}


void CHudMedicChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	ITFHealingWeapon *pMedigun = pPlayer->GetMedigun();
	if ( !pMedigun )
		return;

	C_TFWeaponBase *pActiveWpn = pPlayer->GetActiveTFWeapon();
	if ( !pActiveWpn )
		return;

	if ( m_bMedigunIconIsVisible )
	{
		if ( m_pMedigunIcon )
		{
			m_pMedigunIcon->SetVisible( false );
		}

		switch ( pMedigun->GetMedigunType() )
		{
			case TF_MEDIGUN_STOCK:
				m_pMedigunIcon = dynamic_cast<ImagePanel *>( FindChildByName( "InvulnClusterIcon" ) );
				break;
			case TF_MEDIGUN_KRITZKRIEG:
				m_pMedigunIcon = dynamic_cast<ImagePanel *>( FindChildByName( "KritzClusterIcon" ) );
				break;
			default:
				m_pMedigunIcon = dynamic_cast<ImagePanel *>( FindChildByName( "HealthClusterIcon" ) );
				break;
		}

		if ( m_pMedigunIcon )
		{
			m_pMedigunIcon->SetVisible( true );
		}
	}

	if (pPlayer->GetActiveTFWeapon() == pMedigun->GetWeapon() || pActiveWpn->GetWeaponID() == TF_WEAPON_BONESAW || pActiveWpn->GetWeaponID() == TF_WEAPON_TASER)
	{
		float flCharge = pMedigun->GetChargeLevel();

		//int nBonusStacks = pMedigun->GetUberRateBonusStacks(); // -> %buildratebonusstacks%
		float flRateBonus = pMedigun->GetUberRateBonus() * 100;
		if ( flRateBonus < 1 )
		{
			m_pRateBonusLabel->SetVisible( false );
		}

		SetDialogVariable( "charge", (int)(flCharge * 100) );

		wchar_t wszChargeLevel[6];
		V_swprintf_safe( wszChargeLevel, L"%2.1f", flRateBonus );
		//g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), pBuf, 1, wszChargeLevel );

		SetDialogVariable( "buildratebonus", wszChargeLevel );

		if ( flCharge != m_flLastChargeValue )
		{
			if ( m_pChargeMeter )
			{
				m_pChargeMeter->SetProgress( flCharge );
			}

			if ( !m_bCharged )
			{
				if ( flCharge >= 1.0 )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicCharged" );
					m_bCharged = true;
				}
			}
			else
			{
				// we've got invuln charge or we're using our invuln
				if ( !pMedigun->IsReleasingCharge() )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicChargedStop" );
					m_bCharged = false;
				}
			}
		}

		m_flLastChargeValue = flCharge;
	}
}
