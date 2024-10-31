//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_ITEMEFFECTMETER_H
#define C_TF_ITEMEFFECTMETER_H

#include "cbase.h"
#include "c_tf_player.h"
#include "c_tf_playerclass.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>

using namespace vgui;

class IReloadablePanel;
class CHudItemEffectMeter;
class CItemEffectMeterLogic;
class CItemEffectMeterManager;

extern CItemEffectMeterManager g_ItemEffectMeterManager;


class CItemEffectMeterManager : public CGameEventListener
{
public:
	~CItemEffectMeterManager();

	void			ClearExistingMeters();
	void			SetPlayer( C_TFPlayer *pPlayer );
	void			Update( C_TFPlayer *pPlayer );
	virtual void	FireGameEvent( IGameEvent *event );
	int				GetNumEnabled( void );

private:
	CUtlVector<vgui::DHANDLE<CHudItemEffectMeter>>	m_Meters;
	CUtlVector<IReloadablePanel *>					m_ReloadablePanels;

};


DECLARE_AUTO_LIST( IHudItemEffectMeterAutoList );
class CHudItemEffectMeter : public CHudElement, public EditablePanel, public IHudItemEffectMeterAutoList
{
	DECLARE_CLASS_SIMPLE( CHudItemEffectMeter, EditablePanel );

public:
	CHudItemEffectMeter( const char *pszElementName, C_TFPlayer *pPlayer );
	~CHudItemEffectMeter();

	static void		CreateHudElementsForClass( C_TFPlayer *pPlayer, CUtlVector< vgui::DHANDLE< CHudItemEffectMeter > >& outMeters );

	// Hud Element
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	PerformLayout();
	virtual bool	ShouldDraw( void );
	virtual void	Update( C_TFPlayer *pPlayer, const char *pSoundScript = "TFPlayer.ReCharged" );

	// Effect Meter Logic
	virtual bool		IsEnabled( void )			{ return m_bEnabled; }
	virtual const char *GetLabelText( void );
	virtual const char *GetIconName( void )			{ return "../hud/ico_stickybomb_red"; }
	virtual float		GetProgress( void );
	virtual bool		ShouldBeep( void )			{ return false; }
	virtual const char *GetResFile( void )			{ return "resource/UI/HudItemEffectMeter.res"; }
	virtual int			GetCount( void )			{ return -1; }
	virtual bool		ShouldFlash( void )			{ return false; }
	virtual bool		ShowPercentSymbol( void )	{ return false; }

	virtual Color		GetFgColor( void )			{ return Color( 255, 255, 255, 255 ); }

protected:
	vgui::Label					*m_pLabel;
	vgui::ContinuousProgressBar *m_pProgressBar;
	float						m_flOldProgress;

protected:
	CHandle<C_TFPlayer>			m_pPlayer;
	bool						m_bEnabled;

	CPanelAnimationVarAliasType( float, m_iXOffset, "x_offset", "0", "proportional_float" );

};

//-----------------------------------------------------------------------------
// Purpose: Template variation for weapon based meters.
//-----------------------------------------------------------------------------
template <class T>
class CHudItemEffectMeter_Weapon : public CHudItemEffectMeter
{
public:
	CHudItemEffectMeter_Weapon( const char *pszElementName, C_TFPlayer *pPlayer, ETFWeaponID iWeaponID, bool bBeeps = true, const char *pszResFile = NULL );

	T*					GetWeapon( void );

	virtual void		Update( C_TFPlayer *pPlayer, const char *pSoundScript = "TFPlayer.ReCharged" );

	// Effect Meter Logic
	virtual bool		IsEnabled( void );
	virtual const char *GetLabelText( void );
	virtual const char *GetIconName( void )			{ return "../hud/ico_stickybomb_red"; }
	virtual float		GetProgress( void );
	virtual bool		ShouldBeep( void )			{ return m_bBeeps; }
	virtual const char *GetResFile( void );
	virtual int			GetCount(void)				{ return m_pWeapon ? m_pWeapon->GetCount() : -1; }
	virtual bool		ShouldFlash( void )			{ return false; }
	virtual Color		GetFgColor( void )			{ return Color( 255, 255, 255, 255 ); }
	virtual bool		ShouldDraw( void );
	virtual bool		ShowPercentSymbol( void )	{ return false; }
	virtual void		ApplySchemeSettings( IScheme *scheme ) { CHudItemEffectMeter::ApplySchemeSettings( scheme ); };

private:
	CHandle<T>			m_pWeapon;
	ETFWeaponID			m_iWeaponID;
	bool				m_bBeeps;
	const char			*m_pszResFile;

};

//-----------------------------------------------------------------------------
// Purpose: Template variation for wearable based meters.
//-----------------------------------------------------------------------------
template <class T>
class CHudItemEffectMeter_Wearable : public CHudItemEffectMeter
{
public:
	CHudItemEffectMeter_Wearable( const char *pszElementName, C_TFPlayer *pPlayer, int iWearableNum, bool bBeeps = true, const char *pszResFile = NULL );

	T*					GetWearable( void );

	virtual void		Update( C_TFPlayer *pPlayer, const char *pSoundScript = "TFPlayer.ReCharged" );

	// Effect Meter Logic
	virtual bool		IsEnabled( void );
	virtual const char *GetLabelText( void )		{ return m_pWearable ? m_pWearable->GetEffectLabelText() : ""; }
	virtual const char *GetIconName( void )			{ return "../hud/ico_stickybomb_red"; }
	virtual float		GetProgress( void );
	virtual bool		ShouldBeep( void )			{ return m_bBeeps; }
	virtual const char *GetResFile( void );
	virtual int			GetCount( void )			{ return m_pWearable ? m_pWearable->GetCount() : -1; }
	virtual bool		ShouldFlash( void )			{ return false; }
	virtual Color		GetFgColor( void )			{ return Color( 255, 255, 255, 255 ); }
	virtual bool		ShouldDraw( void );
	virtual bool		ShowPercentSymbol( void )	{ return false; }
	virtual void		ApplySchemeSettings( IScheme *scheme ) { CHudItemEffectMeter::ApplySchemeSettings( scheme ); };

private:
	CHandle<T>			m_pWearable;
	int					m_iWearableNum;
	bool				m_bBeeps;
	const char			*m_pszResFile;

};
#endif