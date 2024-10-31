//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_loadingscreen.h"
#include "ienginevgui.h"
#include <vgui/ISurface.h>
#include "tf_tips.h"
#include <vgui/ILocalize.h>
#include <filesystem.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadingScreen::CTFLoadingScreen() : EditablePanel( NULL, "TFStatsSummary",
	vgui::scheme()->LoadSchemeFromFile( ClientSchemesArray[SCHEME_CLIENT_PATHSTRINGTF2C], ClientSchemesArray[SCHEME_CLIENT_STRING] ) )
{
	//SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );

	m_pBackgroundImage = new ImagePanel( this, "MainBackground" );
	m_pClassImage = new ImagePanel( this, "ClassImage" );

	ListenForGameEvent( "serverinfo_received" );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Resets the dialog
//-----------------------------------------------------------------------------
void CTFLoadingScreen::Reset()
{
	SetDefaultSelections();
}

static const char* const s_ClassImages[] = {
	"../console/image_scout",				// SCOUT
	"../console/image_sniper",				// SNIPER
	"../console/image_soldier",				// SOLDIER
	"../console/image_demoman",				// DEMOMAN
	"../console/image_medic",				// MEDIC
	"../console/image_heavy_weapons",		// HEAVY_WEAPONS
	"../console/image_pyro",				// PYRO
	"../console/image_spy",					// SPY
	"../console/image_engineer",			// ENGINEER
	"../console/image_civilian",			// CIVILIAN
};

static const char* const s_ClassImagesWidescreen[] = {
	"../console/image_widescreen_scout",			// SCOUT
	"../console/image_widescreen_sniper",			// SNIPER
	"../console/image_widescreen_soldier",			// SOLDIER
	"../console/image_widescreen_demoman",			// DEMOMAN
	"../console/image_widescreen_medic",			// MEDIC
	"../console/image_widescreen_heavy_weapons",	// HEAVY_WEAPONS
	"../console/image_widescreen_pyro",				// PYRO
	"../console/image_widescreen_spy",				// SPY
	"../console/image_widescreen_engineer",			// ENGINEER
	"../console/image_widescreen_civilian",			// CIVILIAN
};

//-----------------------------------------------------------------------------
// Purpose: Sets all user-controllable dialog settings to default values
//-----------------------------------------------------------------------------
void CTFLoadingScreen::SetDefaultSelections()
{
	m_iSelectedClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_CLASS_COUNT ); //TF_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFLoadingScreen::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );
	LoadControlSettings( "Resource/UI/main_menu/LoadingScreen.res" );

	UpdateDialog();
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFLoadingScreen::ClearMapLabel()
{
	SetDialogVariable( "maplabel", "" );
	SetDialogVariable( "maptype", "" );
	SetDialogVariable( "mapauthor", "" );

	Label *pMapAuthorLabel = dynamic_cast<Label *>( FindChildByName( "MapAuthorLabel" ) );
	if ( pMapAuthorLabel && pMapAuthorLabel->IsVisible() )
	{
		pMapAuthorLabel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFLoadingScreen::UpdateDialog()
{
	RandomSeed( Plat_MSTime() );

	ClearMapLabel();

	// randomize the class and background
	SetDefaultSelections();
	// fill out class details
	UpdateClassDetails();
	// update the tip
	UpdateTip();

	// determine if we're in widescreen or not
	int width, height;
	surface()->GetScreenSize( width, height );

	float fRatio = (float)width / (float)height;
	bool bWidescreen = ( fRatio < 1.5 ? false : true );

	char szImageFile[MAX_PATH];

	GetRandomImage( szImageFile, sizeof( szImageFile ), bWidescreen );

	// set the background image
	m_pBackgroundImage->SetImage( szImageFile );

	// set the class image
	m_pClassImage->SetImage( bWidescreen ? s_ClassImagesWidescreen[m_iSelectedClass - 1] : s_ClassImages[m_iSelectedClass - 1] );
}

const char *FormatSeconds( int seconds );

//-----------------------------------------------------------------------------
// Purpose: Updates class details
//-----------------------------------------------------------------------------
void CTFLoadingScreen::UpdateClassDetails()
{
	ClassStats_t &stats = GetStatPanel()->GetClassStats( m_iSelectedClass );
	const char *pszPlayTime = FormatSeconds( stats.accumulated.m_iStat[TFSTAT_PLAYTIME] );
	SetDialogVariable( "playtime", pszPlayTime );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the tip
//-----------------------------------------------------------------------------
void CTFLoadingScreen::UpdateTip()
{
	SetDialogVariable( "tiptext", g_TFTips.GetNextClassTip( m_iSelectedClass ) );
}

const char *GetMapDisplayName( const char *mapName );
const char *GetMapType( const char *mapName, bool bFourTeam = false );
const char *GetMapAuthor( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFLoadingScreen::FireGameEvent( IGameEvent *event )
{
	bool bFourTeam = event->GetBool( "fourteam" );
	const char *pMapName = event->GetString( "mapname" );

	// set the map name in the UI
	wchar_t wzMapName[255] = L"";
	g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( pMapName ), wzMapName, sizeof( wzMapName ) );

	SetDialogVariable( "maplabel", wzMapName );
	SetDialogVariable( "maptype", g_pVGuiLocalize->Find( GetMapType( pMapName, bFourTeam ) ) );

#if 0
	// set the map author name in the UI
	const char *szMapAuthor = GetMapAuthor( pMapName );
	if ( szMapAuthor[0] != '\0' )
	{
		SetDialogVariable( "mapauthor", szMapAuthor );

		Label *pMapAuthorLabel = dynamic_cast<Label *>( FindChildByName( "MapAuthorLabel" ) );
		if ( pMapAuthorLabel && !pMapAuthorLabel->IsVisible() )
		{
			pMapAuthorLabel->SetVisible( true );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are activated during level load
//-----------------------------------------------------------------------------
void CTFLoadingScreen::OnActivate()
{
	ClearMapLabel();
	UpdateDialog();

	// Start flashing the window when the level starts loading.
	if ( !engine->IsActiveApp() )
	{
		engine->FlashWindow();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are deactivated at end of level load
//-----------------------------------------------------------------------------
void CTFLoadingScreen::OnDeactivate()
{
	ClearMapLabel();

	// Start flashing the window when the level is done loading.
	if ( !engine->IsActiveApp() )
	{
		engine->FlashWindow();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are deactivated at end of level load
//-----------------------------------------------------------------------------
void CTFLoadingScreen::GetRandomImage( char *pszBuf, int iBufLength, bool bWidescreen )
{
	pszBuf[0] = '\0';

	KeyValues *pVideoKeys = new KeyValues( "Images" );
	if ( pVideoKeys->LoadFromFile( filesystem, "scripts/loadingbackgrounds.txt", "MOD" ) == false )
		return;

	KeyValues *pGroupKey = pVideoKeys->FindKey( bWidescreen ? "widescreen" : "normal" );
	if ( !pGroupKey )
		return;

	CUtlVector<const char *> fileList;
	for ( KeyValues *pSubData = pGroupKey->GetFirstSubKey(); pSubData; pSubData = pSubData->GetNextKey() )
	{
		fileList.AddToTail( pSubData->GetString() );
	}

	if ( fileList.Count() == 0 )
		return;

	V_strncpy( pszBuf, fileList.Random(), iBufLength );
}