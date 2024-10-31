//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_AMMOSTATUS_H
#define TF_HUD_AMMOSTATUS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudWeaponAmmo : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudWeaponAmmo, vgui::EditablePanel );

public:

	CTFHudWeaponAmmo( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip );
	void ShowLowAmmoIndicator( void );

private:

	float							m_flNextThink;

	CHandle<C_BaseCombatWeapon>		m_hCurrentActiveWeapon;
	int								m_nAmmo;
	int								m_nAmmo2;

	CExLabel						*m_pInClip;
	CExLabel						*m_pInClipShadow;
	CExLabel						*m_pInReserve;
	CExLabel						*m_pInReserveShadow;
	CExLabel						*m_pNoClip;
	CExLabel						*m_pNoClipShadow;
	vgui::ImagePanel				*m_pLowAmmoImage;
	vgui::ImagePanel				*m_pWeaponBucket;

	int								m_iLowAmmoX;
	int								m_iLowAmmoY;
	int								m_iLowAmmoWide;
	int								m_iLowAmmoTall;

	bool							m_bUberVisible;
	int								m_iYPosDefault;
	int								m_iXPosDefault;
	int								m_iXUberOffset;
	int								m_iYUberOffset;
};

#endif	// TF_HUD_AMMOSTATUS_H