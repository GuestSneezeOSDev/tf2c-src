//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_CLASSMENU_H
#define TF_CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Frame.h>
#include "vgui_controls/KeyRepeat.h"
#include <tf_shareddefs.h>
#include "tf_controls.h"
#include "tf_playermodelpanel.h"
#include <game/client/iviewport.h>
#include <vgui_controls/PanelListPanel.h>

class CTFClassTipsPanel;
class CTFClassTipsListPanel;

#define CLASS_COUNT_IMAGES	12

//-----------------------------------------------------------------------------
// Purpose: Class selection menu
//-----------------------------------------------------------------------------
class CTFClassMenu : public vgui::Frame, public IViewPortPanel, public CAutoGameSystem
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassMenu, vgui::Frame );

public:
	CTFClassMenu( IViewPort *pViewPort );

	virtual const char *GetName( void ) { return PANEL_CLASS; }
	virtual void SetData( KeyValues *data );
	virtual void Reset() {}
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void SetVisible( bool state );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );

	virtual void OnClose();

	CON_COMMAND_MEMBER_F( CTFClassMenu, "join_class", Join_Class, "Send a joinclass command", 0 );

	ConVar* snd_musicvolume = nullptr;
	ConVar* pSndMixer		= nullptr;

	virtual void PostInit() override;


protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	int GetCurrentClass( void );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();
	
	void SetTeam( int iTeam );
	void SelectClass( int iClass );
	void UpdateNumClassLabels( int iTeam );

	MESSAGE_FUNC_INT( OnShowToTeam, "ShowToTeam", iTeam );
	MESSAGE_FUNC( OnLoadoutChanged, "LoadoutChanged" );

private:
	CExImageButton *m_pClassButtons[TF_CLASS_MENU_BUTTONS];
	CTFPlayerModelPanel *m_pPlayerModel;
	CTFClassTipsPanel *m_pTipsPanel;

	CExButton *m_pCancelButton;
	CExButton *m_pLoadoutButton;
	CExLabel *m_pClassSelectLabel;

#ifdef _X360
	CTFFooter		*m_pFooter;
#endif

	ButtonCode_t	m_iClassMenuKey;
	ButtonCode_t	m_iScoreBoardKey;
	vgui::CKeyRepeatHandler	m_KeyRepeat;
	int				m_iCurrentButtonIndex;
	int				m_iTeamNum;

#ifndef _X360
	CTFImagePanel *m_ClassCountImages[CLASS_COUNT_IMAGES];
	CExLabel *m_pCountLabel;
#endif

	bool			m_bRevertSoundMixer;
};

//-----------------------------------------------------------------------------
// Purpose: Panel used for the chalkboard showing tips for the current class.
//-----------------------------------------------------------------------------
class CTFClassTipsPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFClassTipsPanel, vgui::EditablePanel );

	CTFClassTipsPanel( vgui::Panel *pParent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void SetClass( int iClass );

private:
	CTFClassTipsListPanel *m_pTipList;
};

class CTFClassTipsListPanel : public vgui::PanelListPanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFClassTipsListPanel, vgui::PanelListPanel );

	CTFClassTipsListPanel( vgui::Panel *pParent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void OnThink( void );

	void SetScrollBarImagesVisible( bool bVisible );

private:
	vgui::ScalableImagePanel	*m_pUpArrow;
	vgui::ImagePanel			*m_pLine;
	vgui::ScalableImagePanel	*m_pDownArrow;
	vgui::ImagePanel			*m_pBox;
};

class CTFClassTipsItemPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFClassTipsItemPanel, vgui::EditablePanel );

	CTFClassTipsItemPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
	{
		m_pTipIcon = new vgui::ImagePanel( this, "TipIcon" );
		m_pTipLabel = new CExLabel( this, "TipLabel", "" );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		LoadControlSettings( "Resource/UI/ClassTipsItem.res" );
	}

	void SetTip( const char *pszImage, const wchar_t *pszText )
	{
		m_pTipIcon->SetImage( pszImage );
		m_pTipLabel->SetText( pszText );
	}

private:
	vgui::ImagePanel *m_pTipIcon;
	CExLabel *m_pTipLabel;
};

#endif // TF_CLASSMENU_H

