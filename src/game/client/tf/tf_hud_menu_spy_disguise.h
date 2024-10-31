//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_SPY_DISGUISE_H
#define TF_HUD_MENU_SPY_DISGUISE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "tf_controls.h"
#include "hudelement.h"
#include "IconPanel.h"

// Our team count is + 1 Because we also have the Global Team Disguises
#define TF_DISGUISE_MENU_TEAMS ( TF_TEAM_COUNT - FIRST_GAME_TEAM + 1 )
#define TF_DISGUISE_MENU_CLASSES ( TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS + 1 )
#define TF_DISGUISE_MENU_CATEGORIES 3

class CHudMenuSpyDisguise : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuSpyDisguise, vgui::EditablePanel );

public:
	CHudMenuSpyDisguise( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	SetVisible( bool state );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority( void ) { return 50; }

private:
	void SetSelectedItem( int iSlot );

	void SelectDisguise( int iClass, int iTeam );
	void ToggleDisguiseTeam( void );
	void FlipDisguiseTeams( void );
	void ToggleSelectionIcons( bool bShowSubIcons );

	CON_COMMAND_MEMBER_F( CHudMenuSpyDisguise, "disguiseteam", DisguiseTeam, "Toggles the team in the Spy PDA", 0 )
private:
	struct disguise_item_t
	{
		vgui::EditablePanel *pPanel;
		CExLabel *pNumber;
		CExLabel *pNewNumber;
		CIconPanel *pKeyIcon;
	};

	disguise_item_t m_ClassItems[TF_DISGUISE_MENU_TEAMS][TF_DISGUISE_MENU_CLASSES];
	CExLabel *m_pNumbers[TF_DISGUISE_MENU_CATEGORIES];
	CIconPanel *m_pKeyIcons[TF_DISGUISE_MENU_CATEGORIES];

	vgui::EditablePanel *m_pActiveSelection;

	int m_iShowingTeam;
	int m_iSelectedItem;
	bool m_bInConsoleMode;
	int m_iSelectedCategory;
};

#endif	// TF_HUD_MENU_SPY_DISGUISE_H