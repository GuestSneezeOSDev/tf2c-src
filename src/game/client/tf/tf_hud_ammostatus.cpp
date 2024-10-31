//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>

#include "tf_controls.h"
#include "in_buttons.h"
#include "tf_imagepanel.h"
#include "c_team.h"
#include "c_tf_player.h"
// #include "ihudlcd.h"
#include "tf_hud_ammostatus.h"


using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hud_lowammowarning_threshold( "hud_lowammowarning_threshold", "0.40", FCVAR_NONE, "Percentage threshold at which the low ammo warning will become visible." );
ConVar hud_lowammowarning_maxposadjust( "hud_lowammowarning_maxposadjust", "5", FCVAR_NONE, "Maximum pixel amount to increase the low ammo warning image." );
ConVar hud_showcritchance("hud_showcritchance", "0", FCVAR_CHEAT, "Whether melee crit chance should be shown to the player");

DECLARE_HUDELEMENT( CTFHudWeaponAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudWeaponAmmo::CTFHudWeaponAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudWeaponAmmo" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	// hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	// hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	// hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	// hudlcd->SetGlobalStat( "(weapon_name)", "" );

	m_pInClip = NULL;
	m_pInClipShadow = NULL;
	m_pInReserve = NULL;
	m_pInReserveShadow = NULL;
	m_pNoClip = NULL;
	m_pNoClipShadow = NULL;
	m_pLowAmmoImage = NULL;

	m_iLowAmmoX = m_iLowAmmoY = m_iLowAmmoWide = m_iLowAmmoTall = 0;

	m_pWeaponBucket = NULL;

	m_nAmmo = -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;
	m_iXUberOffset = 0;
	m_iYUberOffset = 0;
	m_iXPosDefault = GetXPos();
	m_iYPosDefault = GetYPos();
}


void CTFHudWeaponAmmo::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

ConVar tf2c_ammobucket("tf2c_ammobucket", "0", FCVAR_ARCHIVE, "Shows weapon bucket in the ammo section. 1 = ON, 0 = OFF.");

void CTFHudWeaponAmmo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudAmmoWeapons.res" );

	m_pInClip = dynamic_cast<CExLabel *>(FindChildByName( "AmmoInClip" ));
	m_pInClipShadow = dynamic_cast<CExLabel *>(FindChildByName( "AmmoInClipShadow" ));

	m_pInReserve = dynamic_cast<CExLabel *>(FindChildByName( "AmmoInReserve" ));
	m_pInReserveShadow = dynamic_cast<CExLabel *>(FindChildByName( "AmmoInReserveShadow" ));

	m_pNoClip = dynamic_cast<CExLabel *>(FindChildByName( "AmmoNoClip" ));
	m_pNoClipShadow = dynamic_cast<CExLabel *>(FindChildByName( "AmmoNoClipShadow" ));

	m_pWeaponBucket = dynamic_cast<ImagePanel*>(FindChildByName("WeaponBucket"));

	m_pLowAmmoImage = dynamic_cast<ImagePanel *>(FindChildByName( "HudWeaponLowAmmoImage" ));
	if ( m_pLowAmmoImage )
	{
		m_pLowAmmoImage->GetBounds( m_iLowAmmoX, m_iLowAmmoY, m_iLowAmmoWide, m_iLowAmmoTall );
	}

	m_nAmmo = -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;

	m_iYPosDefault = GetYPos();
	m_iXPosDefault = GetXPos();

	CTFImagePanel *pHudBG = dynamic_cast<CTFImagePanel *>(FindChildByName( "HudWeaponAmmoBG" ));
	m_iXUberOffset = pHudBG->m_iHorizontalOffsetSpecial;
	m_iYUberOffset = pHudBG->m_iVerticalOffsetSpecial;

	UpdateAmmoLabels( false, false, false );
}


bool CTFHudWeaponAmmo::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	C_TFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( !pWeapon )
	{
		return false;
	}

	//show the panel if the weapon can fire random crits
	if (pWeapon->IsMeleeWeapon() && pWeapon->CanFireRandomCrit()) 
	{
		if (!hud_showcritchance.GetBool()) 
		{
			return false;
		}
		return pWeapon->ShouldShowCritMeter();
	}

	// Hide the panel if the weapon uses no ammo.
	if (!tf2c_ammobucket.GetBool())
	{
		if (!pWeapon->UsesPrimaryAmmo())
		{
			return false;
		}
	}

	// Also hide it if it uses metal for ammo since Engineer already has HUD for metal amount.
	if ( pPlayer->IsPlayerClass(TF_CLASS_ENGINEER) && pWeapon->GetPrimaryAmmoType() == TF_AMMO_METAL )
	{
		return false;
	}

	bool bHideAmmoHud = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, bHideAmmoHud, hide_ammo_hud );
	if ( bHideAmmoHud )
		return false;

	CHudElement *pMedicCharge = GET_NAMED_HUDELEMENT( CHudElement, CHudMedicChargeMeter );

	if ( pMedicCharge && pMedicCharge->IsActive() )
	{
		if ( pWeapon->UsesPrimaryAmmo() )
		{
			if ( !m_bUberVisible )
			{
				m_bUberVisible = true;
				SetPos( m_iXPosDefault + m_iXUberOffset, m_iYPosDefault + m_iYUberOffset );
				// Force update
				UpdateAmmoLabels( true, true, false );
				m_flNextThink = gpGlobals->curtime;
				OnThink();
			}
		}
		else
		{
			return false;
		}
	}
	else if ( m_bUberVisible )
	{
		SetPos( m_iXPosDefault, m_iYPosDefault );
		m_bUberVisible = false;
		// Force update
		UpdateAmmoLabels( true, true, false );
		m_flNextThink = gpGlobals->curtime;
		OnThink();
	}

	return CHudElement::ShouldDraw();
}


void CTFHudWeaponAmmo::UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip )
{
	if ( m_pInClip && m_pInClipShadow )
	{
		if ( m_pInClip->IsVisible() != bPrimary )
		{
			m_pInClip->SetVisible( bPrimary );
			m_pInClipShadow->SetVisible( bPrimary );
		}
	}

	if ( m_pInReserve && m_pInReserveShadow )
	{
		if ( m_pInReserve->IsVisible() != bReserve )
		{
			m_pInReserve->SetVisible( bReserve );
			m_pInReserveShadow->SetVisible( bReserve );
		}
	}

	if ( m_pNoClip && m_pNoClipShadow )
	{
		if ( m_pNoClip->IsVisible() != bNoClip )
		{
			m_pNoClip->SetVisible( bNoClip );
			m_pNoClipShadow->SetVisible( bNoClip );
		}
	}
}


void CTFHudWeaponAmmo::ShowLowAmmoIndicator( void )
{
	if ( m_pLowAmmoImage && !m_pLowAmmoImage->IsVisible() )
	{
		m_pLowAmmoImage->SetBounds( m_iLowAmmoX, m_iLowAmmoY, m_iLowAmmoWide, m_iLowAmmoTall );
		m_pLowAmmoImage->SetVisible( true );
		m_pLowAmmoImage->SetFgColor( Color( 255, 0, 0, 255 ) );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulse" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::OnThink()
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

	if ( m_flNextThink < gpGlobals->curtime )
	{
		if (m_hCurrentActiveWeapon != pWeapon)
		{
			bool bShowIcon = false;

			if (tf2c_ammobucket.GetBool() && pWeapon && m_pWeaponBucket)
			{
				int iItemID = pWeapon->GetItemID();
				CEconItemDefinition* pItemDefinition = GetItemSchema()->GetItemDefinition(iItemID);
				if (pItemDefinition)
				{
					char szImage[MAX_PATH];
					V_sprintf_safe(szImage, "../%s_large", pItemDefinition->image_inventory);
					m_pWeaponBucket->SetImage(szImage);
					bShowIcon = true;
				}
			}
			if (m_pWeaponBucket && m_pWeaponBucket->IsVisible() != bShowIcon)
			{
				m_pWeaponBucket->SetVisible(bShowIcon);
			}
		}

		// hudlcd->SetGlobalStat( "(weapon_print_name)", pWeapon ? pWeapon->GetPrintName() : " " );
		// hudlcd->SetGlobalStat( "(weapon_name)", pWeapon ? pWeapon->GetName() : " " );

		if ( !pWeapon || (!pWeapon->UsesPrimaryAmmo() && !pWeapon->IsMeleeWeapon()))
		{
			// hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
			// hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

			// turn off our ammo counts
			UpdateAmmoLabels( false, false, false );

			m_nAmmo = -1;
			m_nAmmo2 = -1;

			m_hCurrentActiveWeapon = pWeapon;

			if ( m_pLowAmmoImage && m_pLowAmmoImage->IsVisible() )
			{
				m_pLowAmmoImage->SetVisible( false );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulseStop" );
			}
		}
		else if (pWeapon->IsMeleeWeapon()) 
		{
			int nCritChance = int(15 * pPlayer->GetCritMult());
			char buffer[5];
			sprintf(buffer, "%d%%", nCritChance);
			m_hCurrentActiveWeapon = pWeapon;

			UpdateAmmoLabels(true, true, false);
			SetDialogVariable("Ammo", buffer);
			SetDialogVariable("AmmoInReserve", "CRIT");
		}
		else
		{
			// Get the ammo in our clip.
			int nAmmo1 = pWeapon->Clip1();
			int nAmmo2 = 0;
			// Clip ammo not used, get total ammo count.
			if ( nAmmo1 < 0 )
			{
				nAmmo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			// Clip ammo, so the second ammo is the total ammo.
			else
			{
				nAmmo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}

			// hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", nAmmo1 ) );
			// hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", nAmmo2 ) );

			if ( m_nAmmo != nAmmo1 || m_nAmmo2 != nAmmo2 || m_hCurrentActiveWeapon.Get() != pWeapon )
			{
				m_nAmmo = nAmmo1;
				m_nAmmo2 = nAmmo2;
				m_hCurrentActiveWeapon = pWeapon;

				if ( !(pPlayer->HasInfiniteAmmo() || pWeapon->HasInfiniteAmmo()) )
				{
					if ( pWeapon->UsesClipsForAmmo1() )
					{
						UpdateAmmoLabels( true, true, false );

						SetDialogVariable( "Ammo", m_nAmmo );
						SetDialogVariable( "AmmoInReserve", m_nAmmo2 );
					}
					else
					{
						UpdateAmmoLabels( false, false, true );
						SetDialogVariable( "Ammo", m_nAmmo );
					}
				}
				else
				{
					if ( pWeapon->UsesClipsForAmmo1() )
					{
						UpdateAmmoLabels( true, true, false );

						SetDialogVariable( "Ammo", m_nAmmo );
						SetDialogVariable( "AmmoInReserve", L"\u221E" );
					}
					else
					{
						UpdateAmmoLabels( true, false, false );
						SetDialogVariable( "Ammo", L"\u221E" );
					}
				}

				if ( m_pLowAmmoImage )
				{
					// Check low ammo warning pulse.
					// We want to include both clip and max ammo in calculation.
					int iTotalAmmo = m_nAmmo;
					int iMaxAmmo = pPlayer->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() );

					if ( pWeapon->UsesClipsForAmmo1() )
					{
						iTotalAmmo += m_nAmmo2;
						iMaxAmmo += pWeapon->GetMaxClip1();
					}

					int iWarningAmmo = iMaxAmmo * hud_lowammowarning_threshold.GetFloat();

					if ( iTotalAmmo < iWarningAmmo )
					{
						ShowLowAmmoIndicator();

						// Rescale the flashing border based on ammo count.
						int iScaleFactor = RemapValClamped( iTotalAmmo, iWarningAmmo, 0, 0, hud_lowammowarning_maxposadjust.GetFloat() );
						m_pLowAmmoImage->SetBounds(
							m_iLowAmmoX - iScaleFactor,
							m_iLowAmmoY - iScaleFactor,
							m_iLowAmmoWide + iScaleFactor * 2,
							m_iLowAmmoTall + iScaleFactor * 2 );
					}
					else if ( m_pLowAmmoImage->IsVisible() )
					{
						m_pLowAmmoImage->SetVisible( false );
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulseStop" );
					}
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
