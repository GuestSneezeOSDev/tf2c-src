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
#include "tf_weaponbase.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


class CHudDemomanChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudDemomanChargeMeter, EditablePanel );

public:
	CHudDemomanChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::ContinuousProgressBar *m_pChargeMeter;
	Color	m_cFgSchemeColor;
	int		m_iXUberOffset;
	int		m_iYUberOffset; // Amount to offset this control when the uber bar is also visible. Fixes ammo HUD overlapping uber HUD.
	int		m_iXPosDefault;
	int		m_iYPosDefault;
	bool	m_bUberVisible;
};

DECLARE_HUDELEMENT( CHudDemomanChargeMeter );


CHudDemomanChargeMeter::CHudDemomanChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDemomanCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	m_iXUberOffset = 0;
	m_iYUberOffset = 0;
	m_iXPosDefault = GetXPos();
	m_iYPosDefault = GetYPos();
	m_bUberVisible = false;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CHudDemomanChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings ("resource/UI/HudDemomanCharge.res");

	BaseClass::ApplySchemeSettings(pScheme);

	m_iXPosDefault = GetXPos();
	m_iYPosDefault = GetYPos();
	m_bUberVisible = false;

	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>(FindChildByName( "ChargeMeter" ));
	m_iYUberOffset = pMeter->m_iVerticalOffsetSpecial;
	m_iXUberOffset = pMeter->m_iHorizontalOffsetSpecial;

	//m_cFgSchemeColor = GetFgColor();	// I haven't figured out why it's giving me grey color
	m_cFgSchemeColor = Color(255, 255, 255, 255);
}


bool CHudDemomanChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	if (( !pWpn->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) &&
		!pWpn->IsWeapon( TF_WEAPON_GRENADE_MIRV ) &&
		!pWpn->IsWeapon( TF_WEAPON_GRENADE_MIRV2 ) &&
		!pWpn->IsWeapon(TF_WEAPON_COMPOUND_BOW) &&
		!pWpn->IsWeapon(TF_WEAPON_COILGUN) &&
		// I don't like this one bit but, lets join the parade!
		!pWpn->IsWeapon(TF_WEAPON_RIOT_SHIELD)
		) && !pWpn->m_bUsesAmmoMeter )		// Weapons that use the ammo meter mechanic should always draw this bar.
	{
		return false;
	}

	//CHudElement *pMedicCharge = GET_NAMED_HUDELEMENT( CHudElement, CHudMedicChargeMeter );

	//if ( pMedicCharge && pMedicCharge->IsActive() )
	if ( pPlayer->GetMedigun() && pWpn == pPlayer->GetMedigun()->GetWeapon() && pWpn->UsesPrimaryAmmo() )
	{
		if ( !m_bUberVisible )
		{
			m_bUberVisible = true;
			SetPos( m_iXPosDefault + m_iXUberOffset, m_iYPosDefault + m_iYUberOffset );
			DevMsg("Shifting char meter up\n");
		}

	}
	else if ( m_bUberVisible )
	{
		SetPos( m_iXPosDefault, m_iYPosDefault );
		m_bUberVisible = false;
		DevMsg( "Shifting char meter back down\n" );
	}

	return CHudElement::ShouldDraw();
}


void CHudDemomanChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	if ( pWeapon )
	{
		m_pChargeMeter->SetProgress( pWeapon->GetEffectBarProgress() );
		m_pChargeMeter->SetFgColor( m_cFgSchemeColor );
		
	}

	ITFChargeUpWeapon *pChargeupWeapon = dynamic_cast<ITFChargeUpWeapon *>( pPlayer->GetActiveWeapon() );

	if( !pChargeupWeapon )
		return;

	if( m_pChargeMeter )
	{
		float flChargeMaxTime = pChargeupWeapon->GetChargeMaxTime();

		if( flChargeMaxTime != 0 )
		{
			float flChargeBeginTime = pChargeupWeapon->GetChargeBeginTime();

			if( flChargeBeginTime > 0 )
			{
				float flTimeCharged = Max( 0.0f, gpGlobals->curtime - flChargeBeginTime );
				float flPercentCharged = Min( 1.0f, flTimeCharged / flChargeMaxTime );

				m_pChargeMeter->SetProgress( flPercentCharged );
			}
			else
			{
				// If we don't use Charge begin time, take the current charge
				m_pChargeMeter->SetProgress( pChargeupWeapon->GetCurrentCharge() );
			}

			if( pChargeupWeapon->ChargeMeterShouldFlash() )
			{
				int iOffset = ( (int)(gpGlobals->realtime * 10) ) % 10;
				int iRed = 160 + ( iOffset * 10 );
				m_pChargeMeter->SetFgColor( Color(iRed, 0, 0, 255) );
			}
			else
			{
				m_pChargeMeter->SetFgColor( m_cFgSchemeColor );
			}
		}
	}
}
