//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PLAYERSTATUS_H
#define TF_HUD_PLAYERSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "tf_playermodelpanel.h"
#include "hudelement.h"
#include "GameEventListener.h"

class C_TFPlayer;


class CTFClassImage : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFClassImage, vgui::ImagePanel );

	CTFClassImage( vgui::Panel *parent, const char *name ) : ImagePanel( parent, name )
	{
	}

	void SetClass( int iTeam, int iClass, int iCloakstate );
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player class data
//-----------------------------------------------------------------------------
class CTFHudPlayerClass : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerClass, EditablePanel );

public:

	CTFHudPlayerClass( Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

protected:

	virtual void OnThink();
	virtual void UpdateModelPanel( void );

private:

	float				m_flNextThink;

	CTFClassImage		*m_pClassImage;
	CTFImagePanel		*m_pClassImageBG;
	CTFImagePanel		*m_pSpyImage; // used when spies are disguised
	CTFImagePanel		*m_pSpyOutlineImage;

	CTFPlayerModelPanel	*m_pClassModelPanel;
	CTFImagePanel		*m_pClassModelPanelBG;

	vgui::EditablePanel	*m_pCarryingWeaponPanel;

	int					m_nTeam;
	int					m_nClass;
	int					m_nDisguiseTeam;
	int					m_nDisguiseClass;
	int					m_nCloakLevel;
	CHandle<C_EconEntity>	m_hWeapon;
};

//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFHealthPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHealthPanel, vgui::Panel );

	CTFHealthPanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetHealth( float flHealth ){ m_flHealth = ( flHealth <= 1.0 ) ? flHealth : 1.0f; }

	int		m_iAlpha;

private:

	float	m_flHealth; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

enum BuffClass_t
{
	BUFF_CLASS_BULLET_RESIST,
	BUFF_CLASS_BLAST_RESIST,
	BUFF_CLASS_FIRE_RESIST,
	BUFF_CLASS_SOLDIER_OFFENSE,
	BUFF_CLASS_SOLDIER_DEFENSE,
	BUFF_CLASS_SOLDIER_HEALTHONHIT,
	DEBUFF_CLASS_STUNNED,
	DEBUFF_CLASS_SPY_MARKED,
	BUFF_CLASS_PARACHUTE,
	RUNE_CLASS_STRENGTH,
	RUNE_CLASS_HASTE,
	RUNE_CLASS_REGEN,
	RUNE_CLASS_RESIST,
	RUNE_CLASS_VAMPIRE,
	RUNE_CLASS_REFLECT,
	RUNE_CLASS_PRECISION,
	RUNE_CLASS_AGILITY,
	RUNE_CLASS_KNOCKOUT,
	RUNE_CLASS_KING,
	RUNE_CLASS_PLAGUE,
	RUNE_CLASS_SUPERNOVA,
	BUFF_CLASS_CIVILIAN_OFFENSE,
};

struct CTFBuffInfo
{
public:
	CTFBuffInfo( ETFCond eCond, BuffClass_t eClass, vgui::ImagePanel* pPanel, const char *pzsBlueImage = NULL, const char *pzsRedImage = NULL, const char *pzsYellowImage = NULL, const char *pzsGreenImage = NULL )
	{
		m_eCond = eCond;
		m_eClass = eClass;
		m_pImagePanel = pPanel;

		m_pzsRedImage = pzsRedImage;
		m_pzsBlueImage = pzsBlueImage;
		m_pzsGreenImage = pzsGreenImage;
		m_pzsYellowImage = pzsYellowImage;
	}

	void Update( C_TFPlayer *pPlayer );

	ETFCond				m_eCond;
	BuffClass_t			m_eClass;
	vgui::ImagePanel	*m_pImagePanel;

	const char *		m_pzsRedImage;
	const char *		m_pzsBlueImage;
	const char *		m_pzsGreenImage;
	const char *		m_pzsYellowImage;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player health data
//-----------------------------------------------------------------------------
class CTFHudPlayerHealth : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerHealth, EditablePanel );

public:
	CTFHudPlayerHealth( Panel *parent, const char *name );

	virtual const char *GetResFilename( void ) { return "resource/UI/HudPlayerHealth.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	void	SetHealth( int iNewHealth, int iMaxHealth, int iMaxBuffedHealth );
	void	HideHealthBonusImage( void );
	void	ShowBuildingBG( bool bShow );
	void	SetChildrenAlpha( int iAlpha );
	void	SetHealthbarScale( float flScale );
	void	SetHealthTextVisibility( bool bVisible );

protected:
	virtual void OnThink();
public:
	int					m_iAlpha;
protected:
	float				m_flNextThink;

private:
	CTFHealthPanel		*m_pHealthImage;
	vgui::ImagePanel	*m_pHealthBonusImage;
	vgui::ImagePanel	*m_pHealthImageBG;
	vgui::ImagePanel	*m_pBuildingHealthImageBG;
	CExLabel			*m_pHealthValueLabel;

	vgui::ImagePanel	*m_pBleedImage;
	vgui::ImagePanel	*m_pMarkedForDeathImage;
	vgui::ImagePanel	*m_pMarkedForDeathImageSilent;
	vgui::ImagePanel	*m_pSlowImage;
	vgui::ImagePanel	*m_pHasteImage;

	CUtlVector<CTFBuffInfo*> m_vecBuffInfo;

	int					m_nHealth;
	int					m_nMaxHealth;

	struct possize_t {
		int				m_OrigX;
		int				m_OrigY;
		int				m_OrigW;
		int				m_OrigH;
	};

	possize_t			m_BonusHealthPosSize;
	possize_t			m_HealthPosSize;
	possize_t			m_HealthBGPosSize;

	float				m_flHealthbarScale;

	CPanelAnimationVar( int, m_nHealthBonusPosAdj, "HealthBonusPosAdj", "25" );
	CPanelAnimationVar( float, m_flHealthDeathWarning, "HealthDeathWarning", "0.49" );
	CPanelAnimationVar( Color, m_clrHealthDeathWarningColor, "HealthDeathWarningColor", "HUDDeathWarning" );
};

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the player class/health displays
//-----------------------------------------------------------------------------
class CTFHudPlayerStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerStatus, vgui::EditablePanel );

public:
	CTFHudPlayerStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

private:

	CTFHudPlayerClass	*m_pHudPlayerClass;
	CTFHudPlayerHealth	*m_pHudPlayerHealth;
};

#endif	// TF_HUD_PLAYERSTATUS_H