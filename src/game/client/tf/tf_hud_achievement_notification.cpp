//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include "tf_hud_achievement_notification.h"
#include "steam/steam_api.h"
#include "achievementmgr.h"
#include "fmtstr.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ACHIEVEMENT_NOTIFICATION_DURATION 7.0f



DECLARE_HUDELEMENT_DEPTH( CTFAchievementNotification, 100 );


CTFAchievementNotification::CTFAchievementNotification( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "AchievementNotificationPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flHideTime = 0;
	m_pPanelBackground = new EditablePanel( this, "Notification_Background" );
	m_pIcon = new ImagePanel( this, "Notification_Icon" );
	m_pLabelHeading = new Label( this, "HeadingLabel", "" );
	m_pLabelTitle = new Label( this, "TitleLabel", "" );

	m_pIcon->SetShouldScaleImage( true );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CTFAchievementNotification::Init()
{
	ListenForGameEvent( "achievement_event" );
	ListenForGameEvent( "achievement_event_update" );
	ListenForGameEvent( "achievement_earned_local" );
}


void CTFAchievementNotification::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/AchievementNotification.res" );
	
	BaseClass::ApplySchemeSettings( pScheme );
}


void CTFAchievementNotification::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Set background color of various elements.  Need to do this in code, if we do it in res file it gets slammed by the
	// scheme.  (Incl. label background: some products don't have label background colors set in their scheme and helpfully slam it to white.)
	SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pLabelHeading->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pLabelTitle->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pPanelBackground->SetBgColor( Color( 62,70,55, 200 ) );
}


void CTFAchievementNotification::FireGameEvent( IGameEvent * event )
{
	const char *name = event->GetName();
	if ( !Q_strcmp( name, "achievement_event" )  )
	{
		const char *pchName = event->GetString( "achievement_name" );
		int iCur = event->GetInt( "cur_val" );
		int iMax = event->GetInt( "max_val" );
		wchar_t szLocalizedName[256] = L"";

		const wchar_t *pchLocalizedName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pchName );
		Assert( pchLocalizedName );
		if ( !pchLocalizedName || !pchLocalizedName[0] )
			return;

		Q_wcsncpy( szLocalizedName, pchLocalizedName, sizeof( szLocalizedName ) );

		// this is achievement progress, compose the message of form: "<name> (<#>/<max>)"
		wchar_t szFmt[128] = L"";
		wchar_t szText[512] = L"";
		wchar_t szNumFound[16] = L"";
		wchar_t szNumTotal[16] = L"";
		_snwprintf( szNumFound, ARRAYSIZE( szNumFound ), L"%i", iCur );
		_snwprintf( szNumTotal, ARRAYSIZE( szNumTotal ), L"%i", iMax );

		const wchar_t *pchFmt = g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress_Fmt" );
		if ( !pchFmt || !pchFmt[0] )
			return;

		Q_wcsncpy( szFmt, pchFmt, sizeof( szFmt ) );

		g_pVGuiLocalize->ConstructString( szText, sizeof( szText ), szFmt, 3, szLocalizedName, szNumFound, szNumTotal );
		AddNotification( pchName, g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress" ), szText );
	}
	else if ( !Q_strcmp( name, "achievement_earned_local" ) )
	{
		int iAchievementID = event->GetInt( "achievement" );
		wchar_t szLocalizedName[256] = L"";


		CBaseAchievement *pAchievement = engine->GetAchievementMgr()->GetAchievementByID( iAchievementID );

		const char* pchName = pAchievement->GetName();

		const wchar_t *pchLocalizedName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pchName );
		Assert( pchLocalizedName );
		if ( !pchLocalizedName || !pchLocalizedName[0] )
			return;

		Q_wcsncpy( szLocalizedName, pchLocalizedName, sizeof( szLocalizedName ) );

		wchar_t szFmt[128] = L"";
		wchar_t szText[512] = L"";
		const wchar_t *pchFmt = L"%s1";

		Q_wcsncpy( szFmt, pchFmt, sizeof( szFmt ) );

		g_pVGuiLocalize->ConstructString( szText, sizeof( szText ), szFmt, 1, szLocalizedName );
		AddNotification( pchName, g_pVGuiLocalize->Find( "#GameUI_Achievement_Awarded" ), szText );
	}
	else if (!Q_strcmp(name, "achievement_event_update"))
	{
		if (m_flHideTime <= 0.0f)
			return;

		const char* pchName = event->GetString("achievement_name");

		// but since it gets called often, let's first find if we have it present before further operations
		int iQueueItem = -2; // -2 - not found. -1 - currently shown. And the rest is index.
		if (!Q_strcmp(pchName, m_szBaseName))
		{
			iQueueItem = -1;
		}
		else
		{
			auto iInvalidIndex = m_queueNotification.InvalidIndex();
			for (auto i = m_queueNotification.Head(); i != iInvalidIndex; i = m_queueNotification.Next(i))
			{
				if (!Q_strcmp(pchName, m_queueNotification.Element(i).szIconBaseName))
				{
					iQueueItem = i;
					break;
				}
			}
		}
		if (iQueueItem == -2)
		{
			return;
		}

		int iCur = event->GetInt("cur_val");
		int iMax = event->GetInt("max_val");
		wchar_t szLocalizedName[256] = L"";

		const wchar_t* pchLocalizedName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR(pchName);
		Assert(pchLocalizedName);
		if (!pchLocalizedName || !pchLocalizedName[0])
			return;

		Q_wcsncpy(szLocalizedName, pchLocalizedName, sizeof(szLocalizedName));

		// this is achievement progress, compose the message of form: "<name> (<#>/<max>)"
		wchar_t szFmt[128] = L"";
		wchar_t szText[512] = L"";
		wchar_t szNumFound[16] = L"";
		wchar_t szNumTotal[16] = L"";
		_snwprintf(szNumFound, ARRAYSIZE(szNumFound), L"%i", iCur);
		_snwprintf(szNumTotal, ARRAYSIZE(szNumTotal), L"%i", iMax);

		const wchar_t* pchFmt = g_pVGuiLocalize->Find("#GameUI_Achievement_Progress_Fmt");
		if (!pchFmt || !pchFmt[0])
			return;

		Q_wcsncpy(szFmt, pchFmt, sizeof(szFmt));

		g_pVGuiLocalize->ConstructString(szText, sizeof(szText), szFmt, 3, szLocalizedName, szNumFound, szNumTotal);
		UpdateNotification(pchName, g_pVGuiLocalize->Find("#GameUI_Achievement_Progress"), szText, iQueueItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on each tick
//-----------------------------------------------------------------------------
void CTFAchievementNotification::OnTick( void )
{
	if ( ( m_flHideTime > 0 ) && ( m_flHideTime < gpGlobals->curtime ) )
	{
		m_flHideTime = 0;
		ShowNextNotification();
	}
}


bool CTFAchievementNotification::ShouldDraw( void )
{
	return ( ( m_flHideTime > 0 ) && ( m_flHideTime > gpGlobals->curtime ) && CHudElement::ShouldDraw() );
}


void CTFAchievementNotification::AddNotification( const char *szIconBaseName, const wchar_t *pHeading, const wchar_t *pTitle )
{
	// Adding new algorithm that overwrites older notifications of same achievement id.
	// This can be improved significantly by preferring more important notifications
	// like achievement fully earned, so they don't have to wait for overwrites of newer stuff.
	// But let's keep it simple.
	// After all, our only use-case so is `map itemtest`

	// we compare achievements with icon base names as it's the only non-localized part, 
	// seems simpler, originates from achievement name, constant among all progress and full

	int iQueueItem = -1;

	// first, check the one we currently have on display
	if (!Q_strcmp(szIconBaseName, m_szBaseName))
	{
		// Yep, it is, let's stay within code architecture by adding it to head
		// while making sure that algorithm replaces it immediately
		iQueueItem = m_queueNotification.AddToHead();
		m_flHideTime = 0;
	}
	else 
	{
		// It's not current display so let's search our queue from left to right
		// But our array class doesn't support functors so we will search manually
		// By copying their Find code and modifying so I know for sure I am not making any mistake
		// albeit it's supposed to be slower due to no access to PrivateNext
		// Why is it an overcomplicated list anyway
		// Hope no one runs TF2C on X360
		auto iInvalidIndex = m_queueNotification.InvalidIndex();
		for (auto i = m_queueNotification.Head(); i != iInvalidIndex; i = m_queueNotification.Next(i))
		{
			if (!Q_strcmp(szIconBaseName, m_queueNotification.Element(i).szIconBaseName))
			{
				iQueueItem = i;
				break;
				// Found it, let's get out of here because our algorithm makes sure there can be only one
			}
		}
	}

	// Still not found? Yep, just add to tail and move on
	if (iQueueItem == -1)
	{
		iQueueItem = m_queueNotification.AddToTail();
	}

	if (iQueueItem != -1)
	{
		Notification_t& notification = m_queueNotification[iQueueItem];
		Q_strncpy(notification.szIconBaseName, szIconBaseName, ARRAYSIZE(notification.szIconBaseName));
		Q_wcsncpy(notification.szHeading, pHeading, sizeof(notification.szHeading));
		Q_wcsncpy(notification.szTitle, pTitle, sizeof(notification.szTitle));
	}

	// if we are not currently displaying a notification, go ahead and show this one
	if ( 0 == m_flHideTime )
	{
		ShowNextNotification();
	}
}

// this function assumes that we gave a correct index
void CTFAchievementNotification::UpdateNotification(const char* szIconBaseName, const wchar_t* pHeading, const wchar_t* pTitle, int iQueueIndex)
{
	if (iQueueIndex != -1)
	{
		Notification_t& notification = m_queueNotification[iQueueIndex];
		Q_wcsncpy(notification.szHeading, pHeading, sizeof(notification.szHeading));
		Q_wcsncpy(notification.szTitle, pTitle, sizeof(notification.szTitle));
		return;
	}

	// if we are not currently displaying a notification, go ahead and show this one

	// set the text and icon in the dialog
	SetDialogVariable("heading", pHeading);
	SetDialogVariable("title", pTitle);

	// resize the panel so it always looks good

	// get fonts
	HFont hFontHeading = m_pLabelHeading->GetFont();
	HFont hFontTitle = m_pLabelTitle->GetFont();
	// determine how wide the text strings are
	int iHeadingWidth = UTIL_ComputeStringWidth(hFontHeading, pHeading);
	int iTitleWidth = UTIL_ComputeStringWidth(hFontTitle, pTitle);
	// use the widest string
	int iTextWidth = MAX(iHeadingWidth, iTitleWidth);
	// don't let it be insanely wide
	iTextWidth = MIN(iTextWidth, XRES(300));
	int iIconWidth = m_pIcon->GetWide();
	int iSpacing = XRES(10);
	int iPanelWidth = iSpacing + iIconWidth + iSpacing + iTextWidth + iSpacing;
	int iPanelX = GetWide() - iPanelWidth;
	int iIconX = iPanelX + iSpacing;
	int iTextX = iIconX + iIconWidth + iSpacing;
	// resize all the elements
	SetXAndWide(m_pPanelBackground, iPanelX, iPanelWidth);
	SetXAndWide(m_pIcon, iIconX, iIconWidth);
	SetXAndWide(m_pLabelHeading, iTextX, iTextWidth);
	SetXAndWide(m_pLabelTitle, iTextX, iTextWidth);
}

//-----------------------------------------------------------------------------
// Purpose: Shows next notification in queue if there is one
//-----------------------------------------------------------------------------
void CTFAchievementNotification::ShowNextNotification()
{
	// see if we have anything to do
	if ( 0 == m_queueNotification.Count() )
	{
		m_flHideTime = 0;
		return;
	}

	Notification_t &notification = m_queueNotification[ m_queueNotification.Head() ];

	m_flHideTime = gpGlobals->curtime + ACHIEVEMENT_NOTIFICATION_DURATION;

	// set the text and icon in the dialog
	SetDialogVariable( "heading", notification.szHeading );
	SetDialogVariable( "title", notification.szTitle );
	Q_strncpy(m_szBaseName, notification.szIconBaseName, ARRAYSIZE(m_szBaseName));
	if (m_szBaseName && m_szBaseName[0] )
	{
		m_pIcon->SetImage( CFmtStr( "achievements/%s.vmt", m_szBaseName) );
	}

	// Display generic achievement unlock icon
	if ( !g_pFullFileSystem->FileExists( CFmtStr( "materials/vgui/achievements/%s.vmt", m_szBaseName ) ) )
	{
		DevWarning( "MISSING ICON: %s\n ", m_szBaseName );
		m_pIcon->SetImage( "achievements/tf2c_achievement.vmt" );
	}

	// resize the panel so it always looks good

	// get fonts
	HFont hFontHeading = m_pLabelHeading->GetFont();
	HFont hFontTitle = m_pLabelTitle->GetFont();
	// determine how wide the text strings are
	int iHeadingWidth = UTIL_ComputeStringWidth( hFontHeading, notification.szHeading );
	int iTitleWidth = UTIL_ComputeStringWidth( hFontTitle, notification.szTitle );
	// use the widest string
	int iTextWidth = MAX( iHeadingWidth, iTitleWidth );
	// don't let it be insanely wide
	iTextWidth = MIN( iTextWidth, XRES( 300 ) );
	int iIconWidth = m_pIcon->GetWide();
	int iSpacing = XRES( 10 );
	int iPanelWidth = iSpacing + iIconWidth + iSpacing + iTextWidth + iSpacing;
	int iPanelX = GetWide() - iPanelWidth;
	int iIconX = iPanelX + iSpacing;
	int iTextX = iIconX + iIconWidth + iSpacing;
	// resize all the elements
	SetXAndWide( m_pPanelBackground, iPanelX, iPanelWidth );
	SetXAndWide( m_pIcon, iIconX, iIconWidth );
	SetXAndWide( m_pLabelHeading, iTextX, iTextWidth );
	SetXAndWide( m_pLabelTitle, iTextX, iTextWidth );

	m_queueNotification.Remove( m_queueNotification.Head() );
}


void CTFAchievementNotification::SetXAndWide( Panel *pPanel, int x, int wide )
{
	int xCur, yCur;
	pPanel->GetPos( xCur, yCur );
	pPanel->SetPos( x, yCur );
	pPanel->SetWide( wide );
}

CON_COMMAND_F( achievement_notification_test, "Test the hud notification UI", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY )
{
	static int iCount=0;

	CTFAchievementNotification *pPanel = GET_HUDELEMENT( CTFAchievementNotification );
	if ( pPanel )
	{		
		pPanel->AddNotification( "HL2_KILL_ODESSAGUNSHIP", L"Achievement Progress", ( 0 == ( iCount % 2 ) ? L"Test Notification Message A (1/10)" :
			L"Test Message B" ) );
	}

#if 0
	IGameEvent *event = gameeventmanager->CreateEvent( "achievement_event" );
	if ( event )
	{
		const char *szTestStr[] = { "TF_GET_HEADSHOTS", "TF_PLAY_GAME_EVERYMAP", "TF_PLAY_GAME_EVERYCLASS", "TF_GET_HEALPOINTS" };
		event->SetString( "achievement_name", szTestStr[iCount%ARRAYSIZE(szTestStr)] );
		event->SetInt( "cur_val", ( iCount%9 ) + 1 );
		event->SetInt( "max_val", 10 );
		gameeventmanager->FireEvent( event );
	}	
#endif

	iCount++;
}