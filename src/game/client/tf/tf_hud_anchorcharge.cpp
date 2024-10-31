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
#include "tf_weapon_anchor.h"
#include <vgui_controls/AnimationController.h>
#include "tf_controls.h"

#include "tf_hud_itemeffectmeter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_hud_minmode;

using namespace vgui;

#if 0

class CHudAnchorChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudAnchorChargeMeter, EditablePanel);

public:
	CHudAnchorChargeMeter(const char* pElementName);

	virtual void	ApplySchemeSettings(IScheme* scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);

private:
	ContinuousProgressBar* m_pChargeMeter;
	ImagePanel* m_pAnchorIcon;

	bool m_bCharged;
	float m_flLastChargeValue;
};

DECLARE_HUDELEMENT(CHudAnchorChargeMeter);


CHudAnchorChargeMeter::CHudAnchorChargeMeter(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudAnchorCharge")
{
	Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pChargeMeter = new ContinuousProgressBar(this, "ChargeMeter");

	m_pAnchorIcon = NULL;

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	vgui::ivgui()->AddTickSignal(GetVPanel());

	m_bCharged = false;
	m_flLastChargeValue = 0;
	SetDialogVariable("charge", 0);
}


void CHudAnchorChargeMeter::ApplySchemeSettings(IScheme* pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudAnchorCharge.res");

	// The default icon.
	m_pAnchorIcon = dynamic_cast<ImagePanel*>(FindChildByName("HealthClusterIcon"));

	BaseClass::ApplySchemeSettings(pScheme);
}


bool CHudAnchorChargeMeter::ShouldDraw(void)
{
	return false;

	C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer || !pPlayer->IsAlive())
		return false;

	C_TFWeaponBase* pWpn = pPlayer->GetActiveTFWeapon();
	if (!pWpn)
		return false;

	if (pWpn->GetWeaponID() == TF_WEAPON_ANCHOR)
		return CHudElement::ShouldDraw();

	return false;
}


void CHudAnchorChargeMeter::OnTick(void)
{
	C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	C_TFWeaponBase* pActiveWpn = pPlayer->GetActiveTFWeapon();
	if (!pActiveWpn)
		return;

	m_pAnchorIcon = dynamic_cast<ImagePanel*>(FindChildByName("HealthClusterIcon"));
	if (m_pAnchorIcon)
	{
		m_pAnchorIcon->SetVisible(true);
	}

	C_TFAnchor* pAnchor = NULL;
	if (pActiveWpn->GetWeaponID() == TF_WEAPON_ANCHOR)
	{
		pAnchor = assert_cast<C_TFAnchor*>(pActiveWpn);
	}

	if (!pAnchor)
		return;

	float flCharge = pAnchor->GetProgress();

	SetDialogVariable("charge", (int)(flCharge * 100));

	if (flCharge != m_flLastChargeValue)
	{
		if (m_pChargeMeter)
		{
			m_pChargeMeter->SetProgress(flCharge);
		}

		if (!m_bCharged)
		{
			if (flCharge >= 1.0f)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudAnchorCharged");
				m_bCharged = true;
			}
		}
		else
		{
			if (flCharge < 1.0f)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudAnchorChargedStop");
				m_bCharged = false;
			}
		}
	}

	m_flLastChargeValue = flCharge;
}

#endif // tf2c_beta