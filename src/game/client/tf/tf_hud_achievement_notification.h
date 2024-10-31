//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_ACHIEVEMENT_NOTIFICATION_H
#define TF_HUD_ACHIEVEMENT_NOTIFICATION_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"

using namespace vgui;

class CTFAchievementNotification : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFAchievementNotification, EditablePanel );

public:
	CTFAchievementNotification( const char *pElementName );

	virtual void	Init();
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	PerformLayout( void );
	virtual void	LevelInit( void ) { m_flHideTime = 0; }
	virtual void	FireGameEvent( IGameEvent * event );
	virtual void	OnTick( void );

	void AddNotification( const char *szIconBaseName, const wchar_t *pHeading, const wchar_t *pTitle );
	void UpdateNotification(const char* szIconBaseName, const wchar_t* pHeading, const wchar_t* pTitle, int iQueueIndex);

private:
	void ShowNextNotification();
	void SetXAndWide( Panel *pPanel, int x, int wide );

	float m_flHideTime;
	char m_szBaseName[255]; // used to allow achievements to find each other in order to overwrite

	Label *m_pLabelHeading;
	Label *m_pLabelTitle;
	EditablePanel *m_pPanelBackground;
	ImagePanel *m_pIcon;

	struct Notification_t
	{
		char szIconBaseName[255];
		wchar_t szHeading[255];
		wchar_t szTitle[255];
	};

	CUtlLinkedList<Notification_t> m_queueNotification;
};

#endif	// TF_HUD_ACHIEVEMENT_NOTIFICATION_H